/*
 * $QNXLicenseC: 
 * Copyright 2008, QNX Software Systems.  
 *  
 * Licensed under the Apache License, Version 2.0 (the "License"). You  
 * may not reproduce, modify or distribute this software except in  
 * compliance with the License. You may obtain a copy of the License  
 * at: http://www.apache.org/licenses/LICENSE-2.0  
 *  
 * Unless required by applicable law or agreed to in writing, software  
 * distributed under the License is distributed on an "AS IS" basis,  
 * WITHOUT WARRANTIES OF ANY KIND, either express or implied. 
 * 
 * This file may contain contributions from others, either as  
 * contributors under the License or as licensors under other terms.   
 * Please review this entire file for other proprietary rights or license  
 * notices, as well as the QNX Development Suite License Guide at  
 * http://licensing.qnx.com/license-guide/ for other information. 
 * $
 */

#include <audio_driver.h>
#include <string.h>

#include <hw/i2c.h>

#include "sgtl5000.h"

#define LOCAL static

/* SGTL5000 codec defintion, from SGTL5000 datasheet */
#define SGTL5000_32000  0
#define SGTL5000_44100  1
#define SGTL5000_48000  2
#define SGTL5000_96000  3
#define PLL_DEFAULT     196608000
#define PLL_44100       180633600
#define MCLK_THRESH     17000000
#define MCLK_MIN        8000000
#define MCLK_MAX        27000000

#define SCALE ((unsigned long)2048)

LOCAL int sgtl5000_setpll (MIXER_CONTEXT_T * sgtl5000);

LOCAL int32_t pcm_devices[2] = {
	0, 1
};

LOCAL snd_mixer_voice_t stereo_voices[2] = {
	{SND_MIXER_VOICE_LEFT, 0},
	{SND_MIXER_VOICE_RIGHT, 0}
};

LOCAL struct snd_mixer_element_volume1_range hp_range[2] = {
	{0, 0x7F, -5150, 1200},
	{0, 0x7F, -5150, 1200},
};

LOCAL struct snd_mixer_element_volume1_range lineout_range[2] = {
	{0, 0x1F, 0, 1500},
	{0, 0x1F, 0, 1500},
};

LOCAL struct snd_mixer_element_volume1_range dac_range[2] = {
	{0x3C, 0xF0, 0, 9000},
	{0x3C, 0xF0, 0, 9000},
};

LOCAL struct snd_mixer_element_volume1_range adc_range[2] = {
	{0, 0xF, 0, 2250},
	{0, 0xF, 0, 2250},
};

LOCAL struct snd_mixer_element_volume1_range mic_range[2] = {
	{0, 3, 0, 4000},
	{0, 3, 0, 4000},
};

/*********************************************** 
 * read/write interface to sgtl5000 codec *
 ***********************************************/
/* read codec */
LOCAL uint32_t
codec_read (MIXER_CONTEXT_T * mixer, uint32_t reg)
{ 
    struct {
        i2c_send_t      hdr;
        uint16_t        reg;
    } msgreg;
    struct {
        i2c_recv_t      hdr;
        uint16_t        val;
    } msgval;
    int rbytes;

    msgreg.hdr.slave.addr = mixer->adr0cs ? SGTL5000_SLAVE_ADDR_1 : SGTL5000_SLAVE_ADDR_0;
    msgreg.hdr.slave.fmt  = I2C_ADDRFMT_7BIT;
    msgreg.hdr.len        = 2;
    msgreg.hdr.stop       = 1;
    msgreg.reg            = ((reg & 0xff) << 8) | ((reg & 0xff00) >> 8);

    if (devctl(mixer->i2c_fd, DCMD_I2C_SEND, &msgreg, 
                        sizeof(msgreg), NULL)) {
        ado_error ("CODEC I2C_READ ADDR failed");
	return (-1);
    }

    msgval.hdr.slave.addr = mixer->adr0cs ? SGTL5000_SLAVE_ADDR_1 : SGTL5000_SLAVE_ADDR_0;
    msgval.hdr.slave.fmt  = I2C_ADDRFMT_7BIT;
    msgval.hdr.len        = 2;
    msgval.hdr.stop       = 1;

    if (devctl(mixer->i2c_fd, DCMD_I2C_RECV, &msgval, 
                        sizeof(msgval), &rbytes)) {
        ado_error ("CODEC I2C_READ DATA failed");
	return (-1);
    }

    msgval.val = ((msgval.val & 0xff) << 8) | ((msgval.val & 0xff00) >> 8);

    ado_debug (DB_LVL_MIXER, "CODEC: Read [0x%02x] = 0x%02x", reg, msgval.val);

    return (msgval.val);
}

/* write codec */
LOCAL int
codec_write(MIXER_CONTEXT_T * mixer, uint32_t reg, uint32_t val)
{
    struct {
        i2c_send_t      hdr;
        uint16_t        reg;
        uint16_t        val;
    } msg;

    msg.hdr.slave.addr = mixer->adr0cs ? SGTL5000_SLAVE_ADDR_1 : SGTL5000_SLAVE_ADDR_0;
    msg.hdr.slave.fmt  = I2C_ADDRFMT_7BIT;
    msg.hdr.len        = 4;
    msg.hdr.stop       = 1;
    msg.reg            = ((reg & 0xff) << 8) | ((reg & 0xff00) >> 8);
    msg.val            = ((val & 0xff) << 8) | ((val & 0xff00) >> 8);

    if (devctl (mixer->i2c_fd, DCMD_I2C_SEND, &msg, sizeof(msg), NULL))
    {
        ado_error ("CODEC I2C_WRITE failed");
    }
    else
    {
        ado_debug (DB_LVL_MIXER, "CODEC: Wrote [0x%02x] = 0x%02x", reg, val);
    }

    return (EOK);
}

/************************************* 
 *  sgtl5000 codec internal control  *
 *************************************/
/* Headphone control */
LOCAL int32_t 
sgtl5000_hp_mute_control (MIXER_CONTEXT_T * sgtl5000, 
	ado_mixer_delement_t * element, uint8_t set, uint32_t * vol, 
	void *instance_data)
{
	int32_t altered = 0;
	uint16_t chip_ana_ctrl;

	chip_ana_ctrl = codec_read (sgtl5000, CHIP_ANA_CTRL);

	if (set)
	{
		altered = ( (vol[0]&1) != ((chip_ana_ctrl >> 4) & 1) );
		
		if (altered)
		{
			chip_ana_ctrl &= 0xFFEF;
			chip_ana_ctrl |= (vol[0]&1) << 4;
			codec_write (sgtl5000, CHIP_ANA_CTRL, chip_ana_ctrl);
		}
	}
	else
		vol[0] = (chip_ana_ctrl >> 4) & 1;

	return (altered);
}

LOCAL int32_t 
sgtl5000_hp_vol_control (MIXER_CONTEXT_T * sgtl5000, 
	ado_mixer_delement_t * element, uint8_t set, uint32_t * vol, 
	void *instance_data)
{
	int32_t altered = 0;
	uint16_t chip_ana_hp_ctrl;
	int range = ado_mixer_element_vol_range_min (element)
		+ ado_mixer_element_vol_range_max (element);
	
	chip_ana_hp_ctrl = codec_read (sgtl5000, CHIP_ANA_HP_CTRL);

	if (set)
	{
		altered = ( (vol[0] != ( (range - chip_ana_hp_ctrl) & 0x7F) )
				|| (vol[1] !=  ( (range - (chip_ana_hp_ctrl >> 8)) & 0x7F) ));
		if (altered) 
			codec_write (sgtl5000, CHIP_ANA_HP_CTRL, 
					((range - vol[0])&0x7F) | (((range - vol[1])&0x7F)<<8));
	}
	else
	{
		vol[0] = (range - chip_ana_hp_ctrl) & 0x7F;
		vol[1] = (range - (chip_ana_hp_ctrl >> 8)) & 0x7F;
	}

	return (altered);
}

/* Line out control */
LOCAL int32_t 
sgtl5000_lineout_mute_control (MIXER_CONTEXT_T * sgtl5000, 
	ado_mixer_delement_t * element, uint8_t set, uint32_t * vol, 
	void *instance_data)
{
	int32_t altered = 0;
	uint16_t chip_ana_ctrl;

	chip_ana_ctrl = codec_read (sgtl5000, CHIP_ANA_CTRL);

	if (set)
	{
		altered = ( (vol[0]&1) != ((chip_ana_ctrl >> 8) & 1) );
		
		if (altered)
		{
			chip_ana_ctrl &= 0xFF7F;
			chip_ana_ctrl |= (vol[0]&1) << 8;
			codec_write (sgtl5000, CHIP_ANA_CTRL, chip_ana_ctrl);
		}
	}
	else
		vol[0] = (chip_ana_ctrl >> 8) & 1;

	return (altered);
}

LOCAL int32_t 
sgtl5000_lineout_vol_control (MIXER_CONTEXT_T * sgtl5000, 
	ado_mixer_delement_t * element, uint8_t set, uint32_t * vol, 
	void *instance_data)
{
	int32_t altered = 0;
	uint16_t chip_line_out_vol;
	
	chip_line_out_vol = codec_read (sgtl5000, CHIP_LINE_OUT_VOL);

	if (set)
	{
		altered = ( (vol[0] != (chip_line_out_vol & 0x1F))
				|| (vol[1] !=  ((chip_line_out_vol >> 8) & 0x1F)));
		if (altered) 
			codec_write (sgtl5000, CHIP_LINE_OUT_VOL, 
					(vol[0]&0x1F) | ((vol[1]&0x1F)<<8));
	}
	else
	{
		vol[0] = chip_line_out_vol & 0x1F;
		vol[1] = (chip_line_out_vol >> 8) & 0x1F;
	}

	return (altered);
}

/* DAC control */
LOCAL int32_t 
sgtl5000_dac_mute_control (MIXER_CONTEXT_T * sgtl5000, 
	ado_mixer_delement_t * element, uint8_t set, uint32_t * vol, 
	void *instance_data)
{
	int32_t altered = 0;
	uint16_t chip_adcdac_ctrl = 0;

	chip_adcdac_ctrl = codec_read (sgtl5000, CHIP_ADCDAC_CTRL);

	if (set)
	{
		altered = ( (vol[0] & 3) != ((chip_adcdac_ctrl >> 2) & 3) );
		
		if (altered)
		{
			chip_adcdac_ctrl &= 0xFFF3;
			chip_adcdac_ctrl |= ((vol[0]&3) << 2 );
			sgtl5000->chip_adcdac_ctrl = chip_adcdac_ctrl;	/* save current mute state */
			codec_write (sgtl5000, CHIP_ADCDAC_CTRL, chip_adcdac_ctrl);
		}
	}
	else
	{
		vol[0] = (sgtl5000->chip_adcdac_ctrl >> 2) & 3;
	}
	
	return (altered);
}

LOCAL int32_t 
sgtl5000_dac_vol_control (MIXER_CONTEXT_T * sgtl5000, 
	ado_mixer_delement_t * element, uint8_t set, uint32_t * vol, 
	void *instance_data)
{
	int32_t altered = 0;
	uint16_t chip_dac_vol;
	int range = ado_mixer_element_vol_range_min (element)
		+ ado_mixer_element_vol_range_max (element);

	chip_dac_vol = codec_read (sgtl5000, CHIP_DAC_VOL);

	if (set)
	{
		altered = ( (vol[0] != ((range - chip_dac_vol) & 0xFF))
				|| (vol[1] !=  ((range - (chip_dac_vol >> 8))& 0xFF)));
		if (altered) 
			codec_write (sgtl5000, CHIP_DAC_VOL, 
				((range - vol[0])&0xFF) | (((range - vol[1])&0xFF)<<8));
	}
	else
	{
		vol[0] = (range - chip_dac_vol) & 0xFF;
		vol[1] = (range - (chip_dac_vol >> 8)) & 0xFF;
	}

	return (altered);
}

/* ADC control */
LOCAL int32_t 
sgtl5000_adc_mute_control (MIXER_CONTEXT_T * sgtl5000, 
	ado_mixer_delement_t * element, uint8_t set, uint32_t * vol, 
	void *instance_data)
{
	int32_t altered = 0;
	uint16_t chip_ana_ctrl = 0;

	chip_ana_ctrl = codec_read (sgtl5000, CHIP_ANA_CTRL);

	if (set)
	{
		altered = ( (vol[0]&1) != (chip_ana_ctrl&1) );
		
		if (altered)
		{
			chip_ana_ctrl &= 0xFFFE;
			chip_ana_ctrl |= (vol[0]&1);
			codec_write (sgtl5000, CHIP_ANA_CTRL, chip_ana_ctrl);
		}
	}
	else
		vol[0] = chip_ana_ctrl & 1;

	return (altered);
}

LOCAL int32_t 
sgtl5000_adc_vol_control (MIXER_CONTEXT_T * sgtl5000, 
	ado_mixer_delement_t * element, uint8_t set, uint32_t * vol, 
	void *instance_data)
{
	int32_t altered = 0;
	uint16_t chip_ana_adc_vol;
	
	chip_ana_adc_vol = codec_read (sgtl5000, CHIP_ANA_ADC_CTRL);

	if (set)
	{
		altered = ( (vol[0] != (chip_ana_adc_vol & 0xF))
				|| (vol[1] !=  ((chip_ana_adc_vol >> 4) & 0xF)));
		if (altered) 
		{
			chip_ana_adc_vol &= 0xFF00;
			chip_ana_adc_vol |= (vol[0]&0xF) | ((vol[1]&0xF)<<4);
			codec_write (sgtl5000, CHIP_ANA_ADC_CTRL, chip_ana_adc_vol);
		}
	}
	else
	{
		vol[0] = chip_ana_adc_vol & 0xF;
		vol[1] = (chip_ana_adc_vol >> 4) & 0xF;
	}

	return (altered);
}

/* Mic control */
LOCAL int32_t 
sgtl5000_mic_vol_control (MIXER_CONTEXT_T * sgtl5000, 
	ado_mixer_delement_t * element, uint8_t set, uint32_t * vol, 
	void *instance_data)
{
	int32_t altered = 0;
	uint16_t chip_mic_ctrl;
	
	chip_mic_ctrl = codec_read (sgtl5000, CHIP_MIC_CTRL);

	if (set)
	{
		altered = ( (vol[0] != (chip_mic_ctrl & 0x3)) );
		if (altered) 
		{
			chip_mic_ctrl &= 0xFFFC;
			chip_mic_ctrl |= (vol[0]&0x3);
			codec_write (sgtl5000, CHIP_MIC_CTRL, chip_mic_ctrl);
		}
	}
	else
		vol[0] = chip_mic_ctrl & 0x3;

	return (altered);
}

/* Input Mux */
LOCAL int32_t 
sgtl5000_input_mux_control (MIXER_CONTEXT_T * sgtl5000, 
	ado_mixer_delement_t * element, uint8_t set, ado_mixer_delement_t ** inelements, 
	void *instance_data)
{
	int32_t altered = 0;
	int currentinput = 1;
	uint16_t chip_ana_ctrl;

	chip_ana_ctrl = codec_read (sgtl5000, CHIP_ANA_CTRL);

	if (set)
	{
		if (inelements[0] == sgtl5000->mic)
		{
			currentinput = 0;	/* Mic Input */
			inelements[0] = sgtl5000->mic;
		}
		else 
		{
			currentinput = 1;	/* Line in Input */
			inelements[0] = sgtl5000->line;
		}
		
		altered = ( ((chip_ana_ctrl >> 2) & 1) != currentinput );

		chip_ana_ctrl &= 0xFFFB;
		chip_ana_ctrl |= currentinput << 2;
	
		codec_write (sgtl5000, CHIP_ANA_CTRL, chip_ana_ctrl);
	}
	else
	{
		if ( ((chip_ana_ctrl >> 2) & 1) == 0)
			inelements[0] = sgtl5000->mic;
		else 
			inelements[0] = sgtl5000->line;
	}
	
	return (altered);
}

/* get/set linein-hp */
LOCAL int32_t
sgtl5000_linein_hp_get (MIXER_CONTEXT_T * sgtl5000,
	ado_dswitch_t * dswitch, snd_switch_t * cswitch,
	void *instance_data)
{
	uint32_t data;

	data = codec_read (sgtl5000, CHIP_ANA_CTRL);
	cswitch->type = SND_SW_TYPE_BOOLEAN;
	cswitch->value.enable = (data >> 6 ) & 1;

	return (0);
}

LOCAL int32_t
sgtl5000_linein_hp_set (MIXER_CONTEXT_T * sgtl5000,
	ado_dswitch_t * dswitch, snd_switch_t * cswitch,
	void *instance_data)
{
	uint32_t data;
	int32_t altered = 0;

	data = codec_read (sgtl5000, CHIP_ANA_CTRL);
	altered = (cswitch->value.enable != ((data >> 6 ) & 1));
	data &= 0xFFBF;
	data |= cswitch->value.enable << 6;
	codec_write (sgtl5000, CHIP_ANA_CTRL, data);

	return (altered);
}

/* get/set adc-dac */
LOCAL int32_t
sgtl5000_adc_dac_get (MIXER_CONTEXT_T * sgtl5000,
	ado_dswitch_t * dswitch, snd_switch_t * cswitch,
	void *instance_data)
{
	uint32_t data;

	data = codec_read (sgtl5000, CHIP_SSS_CTRL);
	cswitch->type = SND_SW_TYPE_BOOLEAN;
	cswitch->value.enable = (1 - (data >> 4)) & 1;

	return (0);
}

LOCAL int32_t
sgtl5000_adc_dac_set (MIXER_CONTEXT_T * sgtl5000,
	ado_dswitch_t * dswitch, snd_switch_t * cswitch,
	void *instance_data)
{
	uint32_t data;
	int32_t altered = 0;

	data = codec_read (sgtl5000, CHIP_SSS_CTRL);
	altered = (cswitch->value.enable != ((1 - (data >> 4)) & 1) );
	data &= 0xFFEF;
	data |= (1 - cswitch->value.enable) << 4;
	codec_write (sgtl5000, CHIP_SSS_CTRL, data);

	return (altered);
}

/* get/set mic-bias */
LOCAL int32_t
sgtl5000_micbias_get (MIXER_CONTEXT_T * sgtl5000,
	ado_dswitch_t * dswitch, snd_switch_t * cswitch,
	void *instance_data)
{
	uint32_t data;

	data = (codec_read (sgtl5000, CHIP_MIC_CTRL) >> 8) & 3;
	cswitch->type = SND_SW_TYPE_BOOLEAN;
	cswitch->value.enable = data?1:0;

	return (0);
}

LOCAL int32_t
sgtl5000_micbias_set (MIXER_CONTEXT_T * sgtl5000,
	ado_dswitch_t * dswitch, snd_switch_t * cswitch,
	void *instance_data)
{
	uint32_t data, bias;
	int32_t altered = 0;

	data = codec_read (sgtl5000, CHIP_MIC_CTRL);
	bias = (data >> 8) & 3;
	altered = (cswitch->value.enable != (bias?1:0) );
	data &= 0xFCFF;
	if (cswitch->value.enable)
	{
		data |= cswitch->value.enable << 8;
	}
	codec_write (sgtl5000, CHIP_MIC_CTRL, data);

	return (altered);
}

void
codec_playback_mute(MIXER_CONTEXT_T * sgtl5000, int val)
{
	uint16_t chip_adcdac_ctrl = codec_read (sgtl5000, CHIP_ADCDAC_CTRL);

	if (val)
	{
		/* Mute */
		chip_adcdac_ctrl |= (DAC_MUTE_RIGHT | DAC_MUTE_LEFT );
		codec_write (sgtl5000, CHIP_ADCDAC_CTRL, chip_adcdac_ctrl);

		/* Wait for Mute operation to complete */
		while (codec_read (sgtl5000, CHIP_ADCDAC_CTRL) & (VOL_BUSY_DAC_RIGHT | VOL_BUSY_DAC_LEFT))
		{
			delay(1);
		}
	}
	else
	{
		/* Restore mute to user configured state */
		if (!(sgtl5000->chip_adcdac_ctrl & DAC_MUTE_RIGHT))
			chip_adcdac_ctrl &= ~(DAC_MUTE_RIGHT);
		if (!(sgtl5000->chip_adcdac_ctrl & DAC_MUTE_LEFT))
			chip_adcdac_ctrl &= ~(DAC_MUTE_LEFT);
		codec_write (sgtl5000, CHIP_ADCDAC_CTRL, chip_adcdac_ctrl);
	}
}

/* reset sgtl5000 codec */
LOCAL int
sgtl5000_reset (MIXER_CONTEXT_T * sgtl5000)
{
	uint16_t ref_volt = 0;
	uint16_t chip_ref_ctrl = 0;
	uint16_t ana_power;

	/* If there is no external VDDD supply then VDDD can be internally driven by codec */
	if (sgtl5000->sgtl5000conf->vddd_voltage_mv == 0)
	{
		/* 
		 * Datasheet unclear about whether or not to clear ANAPOWER_STARTUP_POWERUP
		 * so don't touch this bit for now.
		 */
		/* VDDD is internally driven, configure VDDD level to 1.2V */
		codec_write (sgtl5000, CHIP_LINREG_CTRL, 0x0008);

		ana_power = codec_read(sgtl5000, CHIP_ANA_POWER);

		/* Power up internal line regulator */
		ana_power |= ANAPOWER_LINEREG_D_POWERUP;

		codec_write (sgtl5000, CHIP_ANA_POWER, ana_power);
	}
	/* Turn off startup power supplies to save power */
	else
	{
		ana_power = codec_read(sgtl5000, CHIP_ANA_POWER);
		ana_power &= ~(ANAPOWER_LINREG_SIMPLE_POWERUP | ANAPOWER_STARTUP_POWERUP);
		codec_write (sgtl5000, CHIP_ANA_POWER, ana_power);
	}

	if ((sgtl5000->sgtl5000conf->vdda_voltage_mv < 3100) &&
			(sgtl5000->sgtl5000conf->vddio_voltage_mv < 3100))
	{
		/* Enable internal oscillator for the charge pump */
		codec_write (sgtl5000, CHIP_CLK_CTRL, 0x800);

		/* Enable charge pump */
		ana_power = codec_read(sgtl5000, CHIP_ANA_POWER);
		ana_power |= ANAPOWER_VDDC_CHRGPMP_POWERUP;
		codec_write (sgtl5000, CHIP_ANA_POWER, ana_power);
	}

	if ((sgtl5000->sgtl5000conf->vdda_voltage_mv > 3100) &&
			(sgtl5000->sgtl5000conf->vddio_voltage_mv > 3100))
	{
		/* Configure the charge pump to use the VDDIO rail) */
		codec_write (sgtl5000, CHIP_LINREG_CTRL, 0x006C);
	}

	/* Set reference voltage */
	ref_volt = ((sgtl5000->sgtl5000conf->vdda_voltage_mv / 2) - 800) / 25;

	/* Max reference voltage is 0x1F, i.e. 1.575V */
	ref_volt = (ref_volt > 0x1f) ? 0x1f : ref_volt;

	chip_ref_ctrl = (ref_volt << REFCTRL_VAG_VAL) | (0x7 << REFCTRL_BIAS_CTRL);

	/* Set reference voltage and bias configuration */
	codec_write (sgtl5000, CHIP_REF_CTRL, chip_ref_ctrl);

	/* Set LINEOUT reference voltage to VDDIO/2 and
	 * bias current to recommended 0.36mA for 10Kohm load with 1.0nf capacitance.
	 * 0 = 0.800V, 3.3V / 2 = 1.65 - 0.800 = 0.85V / 0.025V per step = 34 = 0x22
	 */

	ref_volt = ((sgtl5000->sgtl5000conf->vddio_voltage_mv / 2) - 800) / 25;

	/* Max reference voltage is 0x23, i.e. 1.675V */
	ref_volt = (ref_volt > 0x23) ? 0x23 : ref_volt;

	codec_write (sgtl5000, CHIP_LINE_OUT_CTRL, LINEOUT_36MA_BIAS | ref_volt);

	/* Reduce pop */
	codec_write (sgtl5000, CHIP_REF_CTRL, chip_ref_ctrl | REFCTRL_SMALL_POP);

	/* Disable short detect */
	codec_write (sgtl5000, CHIP_SHORT_CTRL, 0x0);

	/* Mute everything and enable zero-cross */
	codec_write (sgtl5000, CHIP_ANA_CTRL, 0x0133);

	/* Enable power to various components */
	ana_power = codec_read(sgtl5000, CHIP_ANA_POWER);
	ana_power |= ANAPOWER_DAC_POWERUP | ANAPOWER_ADC_POWERUP | ANAPOWER_HEADPHONE_POWERUP |
				ANAPOWER_VAG_POWERUP | ANAPOWER_VCOAMP_POWERUP | ANAPOWER_PLL_POWERUP;

	/*
	 * Some boards such as Sabre-Lite don't appear to support the capless headphone mode.
	 * In fact, if capless headphone mode is enabled Sabre-Lite current draw increases by > 200mA!
	 * Therefore we disable capless headphone mode.
	 */
	ana_power &= ~ANAPOWER_CAPLESS_HEADPHONE_POWERUP;

	codec_write (sgtl5000, CHIP_ANA_POWER, ana_power);

	/* Power Digital block -  ADC, DAC, I2S in, I2S out */
	codec_write (sgtl5000, CHIP_DIG_POWER, 0x0063);

	/* Configure the PLL */
	sgtl5000_setpll (sgtl5000);

	/* I2S_IN to DAC, I2S_DOUT to ADC */
	codec_write (sgtl5000, CHIP_SSS_CTRL, 0x0010);

	/* 32Fs, Master, SCLK rise edge, 16 bits, I2S mode, 1 bit delay, LRCLK 0-left */
	codec_write (sgtl5000, CHIP_I2S_CTRL, 0x01b0);

	/* Mute DAC, Enable linear volume ramp */
	sgtl5000->chip_adcdac_ctrl = (VOL_RAMP_EN | DAC_MUTE_RIGHT | DAC_MUTE_LEFT);
	codec_write (sgtl5000, CHIP_ADCDAC_CTRL, sgtl5000->chip_adcdac_ctrl);

	/* Set DAC volume */
	codec_write (sgtl5000, CHIP_DAC_VOL, 0x3C3C);

	/* Set ADC volume */
	codec_write (sgtl5000, CHIP_ANA_ADC_CTRL, 0x0000);

	/* Set Headphone volume */
	codec_write (sgtl5000, CHIP_ANA_HP_CTRL, 0x1818);

	/* Power on Mic bias (set resistor and voltage) */
	codec_write (sgtl5000, CHIP_MIC_CTRL, 0x0101);

	if (sgtl5000->input_mux == INPUT_MUX_LINE_IN)
		codec_write (sgtl5000, CHIP_ANA_CTRL, 0x0004);		/* Unmute all outputs, Line in to ADC */
	else
		codec_write (sgtl5000, CHIP_ANA_CTRL, 0x0000);		/* Unmute all outputs, Mic in to ADC */

	/* Set Pad strength to reset value */
	codec_write(sgtl5000, CHIP_PAD_STRENGTH, 0x015f);

	/* Unmute DAC */
	sgtl5000->chip_adcdac_ctrl &= ~(DAC_MUTE_RIGHT | DAC_MUTE_LEFT);
	codec_write (sgtl5000, CHIP_ADCDAC_CTRL, sgtl5000->chip_adcdac_ctrl);

	return 0;
}

/* 
 * Configure the inside PLL of SGTL5000 according to SGTL5000 codec datasheet, sector 6.4.2
 */
LOCAL int
sgtl5000_setpll (MIXER_CONTEXT_T * sgtl5000)
{

	unsigned int input_freq_div2 = 0;
	unsigned int pll_input_freq = sgtl5000->mclk;
	unsigned int pll_output_freq = PLL_DEFAULT;
	unsigned int int_divisor;
	unsigned int frac_divisor;

	if (sgtl5000->mclk > MCLK_THRESH) 
	{
		input_freq_div2 = 1;
		pll_input_freq = sgtl5000->mclk/2;
	}

	if (sgtl5000->samplerate == 44100)
	{
		pll_output_freq = PLL_44100;
	}
	
	int_divisor = pll_output_freq/pll_input_freq;
	
	/* calculate frac_divisor by integer point */ 
	frac_divisor = (pll_output_freq/(pll_input_freq/2048)) - int_divisor * 2048;

#if 0
	/* calculate frac_divisor by float point, kept since this is the original fomula from datasheet */
	//frac_divisor = (unsigned int) ((((double)pll_output_freq/(double)pll_input_freq) - (double)int_divisor) * 2048);
#endif

	/* 1 time SYS_FS, USE PLL */
	codec_write (sgtl5000, CHIP_CLK_CTRL, 3 | (sgtl5000->rate_set << 2));

	codec_write (sgtl5000, CHIP_CLK_TOP_CTRL, (input_freq_div2 & 1) << 3);	
	codec_write (sgtl5000, CHIP_PLL_CTRL, ((int_divisor & 0x1f) << 11) | (frac_divisor & 0x7ff));

	return (EOK);
}

LOCAL int
sgtl5000_destroy (MIXER_CONTEXT_T * sgtl5000)
{
	codec_write (sgtl5000, CHIP_ADCDAC_CTRL, 0x000C);	/* Mute DAC */
	codec_write (sgtl5000, CHIP_ANA_CTRL, 0x0091);		/* Mute all outputs, Line in to ADC */
	codec_write (sgtl5000, CHIP_DIG_POWER, 0x0000);		/* Power off ADC, DAC, I2S in, I2S out */

	/* disconnect connect between SoC and codec */
	if (sgtl5000->i2c_fd != -1) 
		close (sgtl5000->i2c_fd);
	
	ado_debug (DB_LVL_MIXER, "Destroying SGTL5000 mixer");
	ado_free (sgtl5000);
	
	return (0);
}

LOCAL int32_t
build_sgtl5000_mixer (MIXER_CONTEXT_T * sgtl5000, ado_mixer_t * mixer)
{
	/* Output elements */
	ado_mixer_delement_t *hp_mute = NULL, *hp_vol = NULL, *hp_io = NULL;
	ado_mixer_delement_t *lineout_mute = NULL, *lineout_vol = NULL, *lineout_io = NULL;
	ado_mixer_delement_t *dac_mute = NULL, *dac_vol = NULL, *dac_pcm = NULL;
	ado_mixer_dgroup_t *hp_grp = NULL, *lineout_grp = NULL, *dac_grp = NULL;

	/* Input elements */
	ado_mixer_delement_t *adc_vol = NULL, *adc_mute = NULL, *adc_pcm = NULL;
	ado_mixer_delement_t *input_mux = NULL, *mic_io = NULL;
	ado_mixer_dgroup_t *adc_grp = NULL, *mic_grp = NULL, *linein_grp = NULL;

	/* ################# */
	/*  PLAYBACK GROUPS  */
	/* ################# */
	/* SGTL5000: Headphone Output */
	if ((hp_mute = ado_mixer_element_sw2 (mixer, "HP Mute",
				(void *)sgtl5000_hp_mute_control, NULL, NULL)) == NULL)
		return (-1);

	if ((hp_vol = ado_mixer_element_volume1 (mixer, "HP Volume",
				2, hp_range, (void *)sgtl5000_hp_vol_control, NULL, NULL)) == NULL)
		return (-1);

	if (ado_mixer_element_route_add (mixer, hp_mute, hp_vol) != 0)
		return (-1);

	if ((hp_io = ado_mixer_element_io (mixer, "HP OUT", SND_MIXER_ETYPE_OUTPUT, 
				0, 2, stereo_voices)) == NULL)
        	return (-1);

	if (ado_mixer_element_route_add (mixer, hp_vol, hp_io) != 0)
		return (-1);
	
	if ((hp_grp = ado_mixer_playback_group_create (mixer, SND_MIXER_PCM_OUT,
				SND_MIXER_CHN_MASK_STEREO, hp_vol, hp_mute)) == NULL)
		return (-1);

	/* SGTL5000: Line Out */
	if ((lineout_mute = ado_mixer_element_sw2 (mixer, "Line Out Mute",
				(void *)sgtl5000_lineout_mute_control, NULL, NULL)) == NULL)
		return (-1);

	if ((lineout_vol = ado_mixer_element_volume1 (mixer, "Line Out Volume",
				2, lineout_range, (void *)sgtl5000_lineout_vol_control, NULL, NULL)) == NULL)
		return (-1);

	if (ado_mixer_element_route_add (mixer, lineout_mute, lineout_vol) != 0)
		return (-1);

	if ((lineout_io = ado_mixer_element_io (mixer, "Line OUT", SND_MIXER_ETYPE_OUTPUT, 
				0, 2, stereo_voices)) == NULL)
		return (-1);

	if (ado_mixer_element_route_add (mixer, lineout_vol, lineout_io) != 0)
		return (-1);
	
	if ((lineout_grp = ado_mixer_playback_group_create (mixer, "Line Out",
				SND_MIXER_CHN_MASK_STEREO, lineout_vol, lineout_mute)) == NULL)
		return (-1);

	/* SGTL5000: DAC Gain Control */
	if ((dac_mute = ado_mixer_element_sw1 (mixer, "DAC Mute", 2,
				(void *)sgtl5000_dac_mute_control, NULL, NULL)) == NULL)
		return (-1);

	if ((dac_vol = ado_mixer_element_volume1 (mixer, "DAC Volume",
				2, dac_range, (void *)sgtl5000_dac_vol_control, NULL, NULL)) == NULL)
		return (-1);

	if (ado_mixer_element_route_add (mixer, dac_mute, dac_vol) != 0)
		return (-1);

	if ((dac_pcm = ado_mixer_element_pcm1 (mixer, "DAC PCM",
				SND_MIXER_ETYPE_PLAYBACK1, 1, &pcm_devices[0])) == NULL)
		return (-1);

	if (ado_mixer_element_route_add (mixer, dac_pcm, dac_mute) != 0)
		return (-1);
	
	if ((dac_grp = ado_mixer_playback_group_create (mixer, "DAC Gain",
				SND_MIXER_CHN_MASK_STEREO, dac_vol, dac_mute)) == NULL)
		return (-1);

	if (ado_mixer_element_route_add (mixer, dac_vol, hp_mute) != 0)
		return (-1);

	if (ado_mixer_element_route_add (mixer, dac_vol, lineout_mute) != 0)
		return (-1);

	/* ################ */
	/*  CAPTURE GROUPS  */
	/* ################ */
	/* SGTL5000: ADC Gain Control */
	if ((adc_vol = ado_mixer_element_volume1 (mixer,
				"ADC Volume", 2, adc_range,
				(void *)sgtl5000_adc_vol_control, NULL, NULL)) == NULL)
		return (-1);
	
	if ((adc_mute = ado_mixer_element_sw1 (mixer, "ADC Mute", 1,
				(void *)sgtl5000_adc_mute_control, NULL, NULL)) == NULL)
		return (-1);
	
	if (ado_mixer_element_route_add (mixer, adc_vol, adc_mute) != 0)
		return (-1);

	if ((adc_pcm = ado_mixer_element_pcm1 (mixer, "ADC PCM",
				SND_MIXER_ETYPE_CAPTURE1, 1, &pcm_devices[0])) == NULL)
		return (-1);
	
	if (ado_mixer_element_route_add (mixer, adc_mute, adc_pcm) != 0)
		return (-1);	
	
	if ((adc_grp = ado_mixer_capture_group_create (mixer, SND_MIXER_GRP_IGAIN, 
				SND_MIXER_CHN_MASK_STEREO, adc_vol, adc_mute, NULL, NULL)) == NULL)
		return (-1);		

	/* Input Multiplexer */
	if ((input_mux = ado_mixer_element_mux1 (mixer,
				SND_MIXER_ELEMENT_INPUT_MUX, 0, 1,
				(void *)sgtl5000_input_mux_control, NULL, NULL)) == NULL)
		return (-1);

	/* Microphone Group */
	if ((sgtl5000->mic = ado_mixer_element_volume1 (mixer, "MIC Volume", 1, mic_range,
				(void *)sgtl5000_mic_vol_control, NULL, NULL)) == NULL)
		return (-1);

	if (ado_mixer_element_route_add (mixer, sgtl5000->mic, input_mux) != 0)
		return (-1);

	if ((mic_io = ado_mixer_element_io (mixer, "MIC In", SND_MIXER_ETYPE_INPUT,
				0, 2, stereo_voices)) == NULL)
		return (-1);

	if (ado_mixer_element_route_add (mixer, mic_io, sgtl5000->mic) != 0)
		return (-1);

	if ((mic_grp = ado_mixer_capture_group_create (mixer, SND_MIXER_MIC_IN,
				SND_MIXER_CHN_MASK_STEREO, sgtl5000->mic, NULL, input_mux, sgtl5000->mic)) == NULL)
		return (-1);

	/* Line in Group */	
	if ((sgtl5000->line = ado_mixer_element_io (mixer, "Line In", SND_MIXER_ETYPE_INPUT,
				0, 2, stereo_voices)) == NULL)
		return (-1);

	if (ado_mixer_element_route_add (mixer, sgtl5000->line, input_mux) != 0)
		return (-1);

	if ((linein_grp = ado_mixer_capture_group_create (mixer, SND_MIXER_LINE_IN,
				SND_MIXER_CHN_MASK_STEREO, NULL, NULL, input_mux, sgtl5000->line)) == NULL)
		return (-1);

	/* ####################### */
	/* SWITCHES                */
	/* ####################### */
	if (ado_mixer_switch_new (mixer, "Line In - HP out", SND_SW_TYPE_BOOLEAN,
			0 , (void *)sgtl5000_linein_hp_get, (void *)sgtl5000_linein_hp_set, 
			NULL, NULL) == NULL)
		return (-1);

	if (ado_mixer_switch_new (mixer, "ADC - DAC", SND_SW_TYPE_BYTE,
			0 , (void *)sgtl5000_adc_dac_get, (void *)sgtl5000_adc_dac_set, NULL, NULL) == NULL)
		return (-1);
		
	if (ado_mixer_switch_new (mixer, "MIC BIAS", SND_SW_TYPE_BYTE,
			0 , (void *)sgtl5000_micbias_get, (void *)sgtl5000_micbias_set, NULL, NULL) == NULL)
		return (-1);

	return (0);		
}

LOCAL char *sgtl5000_opts[] = {
#define I2CDEV       0
	"i2cdev",
#define ADR0CS       1
	"adr0cs",
#define MCLK         2
	"mclk",
#define SAMPLERATE   3
	"samplerate", 
#define INPUTMUX     4
	"input_mux",
#define INFO         5
	"info",		/* info must be last in the option list */
	NULL
};

LOCAL char *opt_help[] = {
	"=[#]                 : i2c device number, default 0 -> /dev/i2c0",
	"=[#]                 : adr0cs pin selection, 0->i2c addr 0xa, 1->i2c addr 0x2a, default 0",
	"=[#]                 : external SYS_MCLK, default 12288000",
	"=[#]                 : samplerate of codec, default 48000",
	"=[line_in | mic_in]  : input mux select, default line_in",
	"                     : display the mixer options"
};

/* Parse mixer options */
LOCAL int 
sgtl5000_parse_options (MIXER_CONTEXT_T * sgtl5000, char * args)
{
	char * value = NULL;
	int opt;
	int idx;
	char *p;

	sgtl5000->i2c_num = 1;
	sgtl5000->adr0cs = 0;
	sgtl5000->mclk = 12288000;
	sgtl5000->samplerate = 48000; 
	sgtl5000->input_mux = INPUT_MUX_LINE_IN;

	while ((p = strchr(args, ':')) != NULL)
		*p = ',';

	while (*args != '\0')
	{
		opt = getsubopt (&args, sgtl5000_opts, &value);
		switch (opt) {
			case I2CDEV:
				if (value != NULL) 
					sgtl5000->i2c_num = strtoul (value, NULL, 0);
				break;
			case ADR0CS:
				if (value != NULL) 
					sgtl5000->adr0cs = strtoul (value, NULL, 0);
				break;
			case MCLK:
				if (value != NULL) 
					sgtl5000->mclk = strtoul (value, NULL, 0);
				break;
			case SAMPLERATE:
				if (value == NULL) 
					ado_error ("SGTL5000: Must pass a value to samplerate");
				else
					sgtl5000->samplerate = strtoul (value, NULL, 0);
				break;
			case INPUTMUX:
				if (value != NULL)
				{
					if (!strcmp(value, "line_in"))
					{
						sgtl5000->input_mux = INPUT_MUX_LINE_IN;
					}
					else if (!strcmp(value, "mic_in"))
					{
						sgtl5000->input_mux = INPUT_MUX_MIC_IN;
					}
				}
				break;
			case INFO:
				idx = 0;
				printf ("SGTL5000 mixer options\n");

				/* display all the mixer options before "info" */
				while (idx <= opt)
				{
					printf("%10s%s\n", sgtl5000_opts[idx], opt_help[idx]);
					idx++;
				}
				printf("\nExample: io-audio -d [driver] mixer=i2cdev=0:adr0cs=0:mclk=1228800\n");
				break;
			default:
				ado_error ("SGTL5000: Unrecognized option \"%s\"", value);
				break;
		}
	}

	return (EOK);
}

/* Called by audio i2s controller */
int 
sgtl5000_mixer (ado_card_t * card, ado_mixer_t ** mixer, char * args, ado_pcm_t * pcm1, 
					sgtl5000_config *sgtl5000conf)
{
	sgtl5000_context_t *sgtl5000;
	int32_t status;
	char i2c_dev[_POSIX_PATH_MAX];

	ado_debug (DB_LVL_MIXER, "SGTL5000 mixer");

	if ((sgtl5000 = (sgtl5000_context_t *)
		ado_calloc (1, sizeof (sgtl5000_context_t))) == NULL)
	{
		ado_error ("SGTL5000: no memory (%s)", strerror (errno));
		return (-1);
	}

	sgtl5000->sgtl5000conf = sgtl5000conf;

	if ((status = ado_mixer_create (card, "sgtl5000", mixer, (void *)sgtl5000)) != EOK)
	{
		ado_error ("SGTL5000: Fail to creat mixer", strerror (errno));
		ado_free (sgtl5000);
		return (status);
	}

	if (sgtl5000_parse_options (sgtl5000, args)!=EOK)
	{
		ado_error ("SGTL5000: Fail to parse mixer options");
		ado_free (sgtl5000);
		return (-1);
	}

	/* check samplerate option */
	switch (sgtl5000->samplerate) {
		case 32000: 
			sgtl5000->rate_set = SGTL5000_32000;
			break;
		case 44100: 
			sgtl5000->rate_set = SGTL5000_44100;
			break;
		case 48000: 
			sgtl5000->rate_set = SGTL5000_48000;
			break;
		case 96000: 
			sgtl5000->rate_set = SGTL5000_96000;
			break;
		default:
			slogf (_SLOG_SETCODE (_SLOGC_AUDIO, 0), _SLOG_ERROR, 
				"SGTL5000: Unsupported audio sample rate. This codec supports 32000, 44100, 48000, 96000 sample rates. Use 48000 as default");
			sgtl5000->rate_set = SGTL5000_48000;
			sgtl5000->samplerate = 48000;
	}

	/* establish connection between SoC and sgtl5000 via I2C resource manager */
	sprintf(i2c_dev, "/dev/i2c%d", sgtl5000->i2c_num);
	sgtl5000->i2c_fd = open (i2c_dev, O_RDWR);
	if (sgtl5000->i2c_fd == -1)
	{
		ado_error ("SGTL5000 I2C_INIT: Failed to open I2C device %s", i2c_dev);
		ado_free (sgtl5000);
		return (-1);
	}
	
	if (build_sgtl5000_mixer (sgtl5000, *mixer))
	{
		ado_error ("SGTL5000: Failed to build mixer");
		close (sgtl5000->i2c_fd);
		ado_free (sgtl5000);
		return (-1);
	}
	
	sgtl5000_reset (sgtl5000);

	ado_mixer_set_reset_func (*mixer, (void *)sgtl5000_reset);
	ado_mixer_set_destroy_func (*mixer, (void *)sgtl5000_destroy);

	/* setup mixer controls for playback and capture */
	ado_pcm_chn_mixer (pcm1,
		ADO_PCM_CHANNEL_PLAYBACK, *mixer,
		ado_mixer_find_element (*mixer, SND_MIXER_ETYPE_PLAYBACK1,
			SND_MIXER_PCM_OUT, 0),
		ado_mixer_find_group (*mixer, SND_MIXER_PCM_OUT, 0));

	ado_pcm_chn_mixer (pcm1,
		ADO_PCM_CHANNEL_CAPTURE, *mixer,
		ado_mixer_find_element (*mixer, SND_MIXER_ETYPE_CAPTURE1,
			SND_MIXER_ELEMENT_CAPTURE, 0), 
		ado_mixer_find_group (*mixer, SND_MIXER_GRP_IGAIN, 0));

	/* allow audio chip to support multiple simultaneous streams */
	if (ado_pcm_sw_mix (card, pcm1, *mixer))
	{
		ado_error ("SGTL5000: Failed to build software mixer");
		close (sgtl5000->i2c_fd);
		ado_free (sgtl5000);
		return -1;
	}

	return (0);
}

