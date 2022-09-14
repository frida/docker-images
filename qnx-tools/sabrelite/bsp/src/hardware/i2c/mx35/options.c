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


#include "proto.h"


int
query_hwi_device(mx35_dev_t *dev, unsigned unit)
{
	unsigned 	hwi_off = hwi_find_bus(HWI_ITEM_BUS_I2C, unit);
	if(hwi_off != HWI_NULL_OFF) {
			hwi_tag *tag = hwi_tag_find(hwi_off, HWI_TAG_NAME_location, 0);
			if(tag){
					dev->physbase = tag->location.base;
					dev->intr = hwitag_find_ivec(hwi_off, NULL);
			}
			return 1;
 	}
 	/*
 	* No default device, the base address and irq have been specified
 	*/
 	return 0;
 }
 
int
mx35_options(mx35_dev_t *dev, int argc, char *argv[])
{
    int     			c;
    int     			prev_optind;
    int     			done = 0;
    unsigned long		interface = 1;


    /* defaults */
    dev->intr = MX35_I2C1_IRQ;
    dev->iid = -1;
    dev->physbase = MX35_I2C1_BASE_ADDR;
    dev->reglen = MX35_I2C_REGLEN;
    dev->own_addr = MX35_I2C_OWN_ADDR;
    dev->restart = 0;
    dev->slave_addr = 0;
    dev->options = 0;
    dev->input_clk = MX35_I2C_INPUT_CLOCK;

    /* Getting the I2C Base addresss and irq from the Hwinfo Section if available */
	 
    if(dev->unit != -1)
    {
	query_hwi_device(dev, dev->unit);
    }

    while (!done) {
        prev_optind = optind;
        c = getopt(argc, argv, "I:a:s:vc:p:i:");
        switch (c) {
        case 'I':
            interface = strtoul(optarg, &optarg, NULL);
	    switch (interface){
		case 1:
			dev->intr = MX35_I2C1_IRQ;
			dev->physbase = MX35_I2C1_BASE_ADDR;
			break;
		case 2:
			dev->intr = MX35_I2C2_IRQ;
			dev->physbase = MX35_I2C2_BASE_ADDR;
			break;
		case 3:
			dev->intr = MX35_I2C3_IRQ;
			dev->physbase = MX35_I2C3_BASE_ADDR;
			break;
		default:
			dev->intr = MX35_I2C1_IRQ;
			dev->physbase = MX35_I2C1_BASE_ADDR;
			break;
	    }
            break;
	case 'a':
            dev->own_addr = strtoul(optarg, &optarg, NULL);
	    break;
        case 's':
            dev->slave_addr = strtoul(optarg, &optarg, NULL);
            break;

        case 'v':
            dev->options |= MX35_OPT_VERBOSE;
            break;

        case 'c':
            dev->input_clk = strtoul(optarg, &optarg, NULL);
            break;

       	case 'p':
   	    dev->physbase = strtoul(optarg, &optarg, NULL);
	    break;

	case 'i':
	    dev->intr = strtol(optarg, &optarg, NULL);
	    break;

        case '?':
	    
            if (optopt == '-') {
                ++optind;
                break;
            }
            return -1;

        case -1:
            if (prev_optind < optind) /* -- */
                return -1;

            if (argv[optind] == NULL) {
                done = 1;
                break;
            } 
            if (*argv[optind] != '-') {
                ++optind;
                break;
            }
            return -1;

        case ':':
        default:
            return -1;
        }
    }

    return 0;
}

__SRCVERSION( "$URL: http://svn/product/branches/6.5.0/trunk/hardware/i2c/mx35/options.c $ $Rev: 390064 $" );
