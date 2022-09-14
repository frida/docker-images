/*
 * $QNXLicenseC: 
 * Copyright 2012, 2013 QNX Software Systems.  
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

#include "startup.h"
#include "mx6x_epit.h"
#include "board.h"

/* Global variables used when toggling GPIO reset lines */
uint32_t 	timer_value;
uint32_t	gpio_base;
uint32_t	gpio_pin;
uint32_t	gpio_level;

uint32_t verify_gpioparams(uint32_t base, uint32_t pin)
{
	if ((base != MX6X_GPIO1_BASE) && (base != MX6X_GPIO2_BASE) && (base != MX6X_GPIO3_BASE) &&
		(base != MX6X_GPIO4_BASE) && (base != MX6X_GPIO5_BASE) && (base != MX6X_GPIO6_BASE) &&
		(base != MX6X_GPIO7_BASE))
	{
		kprintf("Error - GPIO base address 0x%x is invalid. ", base);
		return FALSE;
	}
	
	if (pin > 31)
	{
		kprintf("Error - GPIO pin number %d is invalid. ", pin);
		return FALSE;
	}
	
	return TRUE;
}

/*
 * Name:	mx6x_set_gpio_output
 * Description:	This function configures a GPIO pin as an output.
 * Parameters:
 * 		base -Base address (physical address) of GPIO bank
 *		pin - GPIO pin (0 - 31) e.g. 9 for pin #9
 *		level - GPIO output value
 * Return Value: Whether or not the GPIO was successfully updated
 */
uint32_t mx6x_set_gpio_output(uint32_t base, uint32_t pin, uint32_t level)
{
	uint32_t reg;
	
	if (!verify_gpioparams(base, pin))
	{
		kprintf("%s failed - invalid params\n", __FUNCTION__);
		return FALSE;
	}
	reg = in32(base + MX6X_GPIO_DR);

	/* Set GPIO output level */
	if (level)
		reg |= (1<<pin);
	else
		reg &= ~(1<<pin);
	out32(base + MX6X_GPIO_DR, reg);

	/* Set the GPIO pin to output mode */
	reg = in32(base + MX6X_GPIO_GDIR);
	reg |= (1<<pin);
	out32(base + MX6X_GPIO_GDIR, reg);

	return TRUE;
}

/*
 * Name:	mx6x_set_gpio_input
 * Description:	This function configures a GPIO pin as an input.
 * Parameters:
 * 		base -Base address (physical address) of GPIO bank
 *		pin - GPIO pin (0 - 31) e.g. 9 for pin #9
 * Return Value: Whether or not the GPIO was succcessfully configured as an input
 */
uint32_t mx6x_set_gpio_input(uint32_t base, uint32_t pin)
{
	uint32_t reg;

	if (!verify_gpioparams(base, pin))
	{
		kprintf("%s failed - invalid params\n", __FUNCTION__);
		return FALSE;
	}

	/* Set the GPIO pin to input mode */
	reg = in32(base + MX6X_GPIO_GDIR);
	reg &= ~(1<<pin);
	out32(base + MX6X_GPIO_GDIR, reg);

	return TRUE;
}

/*
 * Name:	mx6x_get_gpio_value
 * Description:	This function determines the value of a GPIO pin
 * Parameters:
 * 		base     - Base address (physical address) of GPIO bank
 *		pin      - GPIO pin (0 - 31) e.g. 9 for pin #9
 *              gpio_val - Value of GPIOx_DR register's bit corresponding to the pin.
 * Return Value: Whether or not the GPIO value was successfully determined.
 */
uint32_t mx6x_get_gpio_value(uint32_t base, uint32_t pin, uint32_t *gpio_val)
{
	uint32_t reg;

	if (!verify_gpioparams(base, pin))
	{
		kprintf("%s failed - invalid params\n", __FUNCTION__);
		return FALSE;
	}

	reg = in32(base + MX6X_GPIO_DR);
	reg &= (1<<pin);
	reg >>= pin;
	*gpio_val = reg;

	return TRUE;
}

/*
 * Name:	 mx6x_reset_gpio_pin
 * Description:	 This function configures a GPIO as an output, sets the output value, and sets
 *               up a timer. mx6x_reset_gpio_pin and mx6x_reset_gpio_pin_fin can be used to put a chip
 *		 in reset for a duration of time without blocking other startup code from running.
 * Parameters:
 * 		base -Base address (physical address) of GPIO bank
 *		pin - GPIO pin (0 - 31) e.g. 9 for pin #9
 *		level - GPIO output value
 * Return Value: Whether or not GPIO was successfully put into reset
 */
uint32_t mx6x_reset_gpio_pin(uint32_t base, uint32_t pin, uint32_t level)
{

	if (!verify_gpioparams(base, pin))
	{
		kprintf("%s failed - invalid params\n", __FUNCTION__);
		return FALSE;
	}

	/* Global variables needed by mx6x_reset_gpio_pin_fin */
	gpio_base = base;
	gpio_pin = pin;
	gpio_level = level;

	/* Configure GPIO as output, and put chip into reset */
	mx6x_set_gpio_output(base, pin, gpio_level);

	/* Start the counter */
	timer_value = mx6_epit_get_timer_val();

	return TRUE;
}

/*
 * Name:	 mx6x_reset_gpio_pin_fin
 * Description:	 This function takes a chip out of reset after a specified duration of time
 *               has elapsed.
 * Parameters:
 * 		 dur - the amount of time that the chip must be in reset, value specified
 *		      in microseconds.
 * Return Value: void
 */
uint32_t mx6x_reset_gpio_pin_fin(uint32_t dur)
{
	uint32_t reg;


	if (!verify_gpioparams(gpio_base, gpio_pin))
	{
		kprintf("%s failed - invalid params\n", __FUNCTION__);
		return FALSE;
	}

	dur = dur * IPG_CLKS_IN_ONE_US;
	while ((timer_value - mx6_epit_get_timer_val()) <= dur)
		__asm__ __volatile__("nop");

	/* Bring chip out of reset */
	reg = in32(gpio_base + MX6X_GPIO_DR);

	if (gpio_level)
		reg &= ~(1<<gpio_pin);
	else
		reg |= (1<<gpio_pin);

	out32(gpio_base + MX6X_GPIO_DR, reg);

	return TRUE;
}



#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn/product/branches/6.5.0/trunk/hardware/startup/boards/imx6x/mx6x_config_gpios.c $ $Rev: 737484 $")
#endif
