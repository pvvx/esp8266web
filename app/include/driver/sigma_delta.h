/*
 * sigma_delta.h
 *
 *  Created on: 14/02/2015 г.
 *      Author: PV`
 */

#ifndef _DRIVER_SIGMA_DELTA_H_
#define _DRIVER_SIGMA_DELTA_H_

void sigma_delta_setup(uint32 GPIO_NUM) ICACHE_FLASH_ATTR;
void sigma_delta_close(uint32 GPIO_NUM) ICACHE_FLASH_ATTR;
void set_sigma_duty_312KHz(uint8 duty) ICACHE_FLASH_ATTR;

#endif /* _DRIVER_SIGMA_DELTA_H_ */
