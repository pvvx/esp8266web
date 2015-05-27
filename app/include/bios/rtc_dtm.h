/******************************************************************************
 * FileName: gpio_bios.h
 * Description: rtc & dtm funcs in ROM-BIOS
 * Alternate SDK ver 0.0.0 (b0)
 * Author: PV`
 * (c) PV` 2015
*******************************************************************************/
#ifndef _BIOS_RTC_DTM_H_
#define _BIOS_RTC_DTM_H_

#include "c_types.h"

/*
 PROVIDE ( ets_rtc_int_register = 0x40002a40 );
 PROVIDE ( ets_enter_sleep = 0x400027b8 );
 PROVIDE ( rtc_enter_sleep = 0x40002870 );
 PROVIDE ( rtc_get_reset_reason = 0x400025e0 );
 PROVIDE ( rtc_intr_handler = 0x400029ec );
 PROVIDE ( rtc_set_sleep_mode = 0x40002668 );
 PROVIDE ( dtm_get_intr_mask = 0x400026d0 );
 PROVIDE ( dtm_params_init = 0x4000269c );
 PROVIDE ( dtm_set_intr_mask = 0x400026c8 );
 PROVIDE ( dtm_set_params = 0x400026dc );
 PROVIDE ( software_reset = 0x4000264c );
 PROVIDE ( save_rxbcn_mactime = 0x400027a4 );
 PROVIDE ( save_tsf_us = 0x400027ac );
 */

void software_reset(void);
void rtc_set_sleep_mode(uint32 a, uint32 t, uint32 m);
uint32 rtc_get_reset_reason(void);
void save_rxbcn_mactime(uint32 t);
void save_tsf_us(uint32 us);
void dtm_set_intr_mask(uint32 mask);
uint32 dtm_get_intr_mask(void);
void dtm_params_init(int a, int b);
void dtm_set_params(int a2, int a3, int a4, int a5, int a6);
void rtc_intr_handler(void);
void rtc_enter_sleep(void);
void ets_rtc_int_register(void);
void ets_enter_sleep(void); // { ets_set_idle_cb(rtc_enter_sleep, 0); }

#endif /* _BIOS_RTC_DTM_H_ */
