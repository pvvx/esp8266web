/*
 * adc.h
 *
 *  Created on: 12/02/2015
 *      Author: PV`
 */

#ifndef _INCLUDE_DRIVER_ADC_H_
#define _INCLUDE_DRIVER_ADC_H_

extern uint32 wdrv_bufn;

/* Инициализация SAR */
void sar_init(uint32 clk_div, uint32 win_cnt) ICACHE_FLASH_ATTR;
/* Отключить SAR */
void sar_off(void) ICACHE_FLASH_ATTR;
/* Чтение SAR */
void read_adcs(uint16 *ptr, uint16 len, uint32 cfg) ICACHE_FLASH_ATTR;

#endif /* _INCLUDE_DRIVER_ADC_H_ */
