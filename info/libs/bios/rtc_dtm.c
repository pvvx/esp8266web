/******************************************************************************
 * FileName: rts_dtm.c
 * Description: functions in ROM-BIOS
 * Alternate SDK ver 0.0.1
 * Author: PV`
*******************************************************************************/
#include "c_types.h"
#include "hw/esp8266.h"
#include "bios/rtc_dtm.h"


// ROM:4000264C
void software_reset(void)
{
	rtc_[0] |= 1 << 31; // IOREG(0x60000700) |= 0x80000000;
}

// ROM:40002668
void rtc_set_sleep_mode(uint32 a, uint32 t, uint32 m)
{

	IO_RTC_SLP_VAL = IO_RTC_SLP_CNT_VAL + t * 100000; // 0x6000071C - 0x60000704
	rtc_[6] = m; // 0x60000718
	rtc_[2] |= a; // 0x60000708
}

// ROM:400025E0 //  =1 - ch_pd,  =2 - reset, =4 - Wdt Reset ... > 7 unknown reset
uint32 rtc_get_reset_reason(void)
{
	uint32 x = rtc_[5] & 7; // IOREG(0x60000714) & 7;
	if(x == 5) {
		return x;
	}
	else {
		x = (rtc_[6]>>8) & 0x3F; // (IOREG(0x60000718)>>8) & ((1<<6)-1);
		if(x == 1) x = 6;
		else if(x == 8) x = 0;
	}
	rtc_[2] &= ~(1<<21); // IOREG(0x60000708) &= ~(1<<21);
	return x;
}

//ROM:400027A4
void save_rxbcn_mactime(uint32 t)
{
	dtm_params.rxbcn_mactime = t;
}

// ROM:400027AC
void save_tsf_us(uint32 us)
{
	dtm_params.tsf_us = us;
}

// ROM:400026C8
void dtm_set_intr_mask(uint32 mask)
{
	dtm_params.intr_mask = mask;
}

// ROM:400026D0
uint32 dtm_get_intr_mask(void)
{
	return dtm_params.intr_mask;
}

// ROM:4000269C
void dtm_params_init(void * sleep_func, void * int_func)
{
	ets_memset(&dtm_params, 0, sizeof(dtm_params)); // 68
	dtm_params.sleep_func = sleep_func;
	dtm_params.int_func = int_func;
}
// ROM:400026DC
void dtm_set_params(int mode, int time_ms_a3, int a4, int cycles, int a6)
{
	dtm_params.mode = mode;
	dtm_params.dtm_2C = a4;
	dtm_params.time_ms = time_ms_a3*1000;
	dtm_params.dtm_14 = a6;
	if(cycles + 1) cycles = 0;
	dtm_params.cycles = cycles;
	if(a2 & 1) {
		__floatunsidf(__floatsidf(rand()))
		..
		dtm_params.sleep_time = ?;
	}
	else dtm_params.sleep_time = time_ms_a3*1000;

	if(a2&2) {
		__floatunsidf(__floatsidf(rand()))
		..
		dtm_params.timer_us = ?;
	}
	else {
		dtm_params.timer_us = a4;
	}
}

// ROM:400027D4
void dtm_rtc_intr_()
{
	if(dtm_params.int_func != NULL) dtm_params.int_func();
	if(dtm_params.intr_mask) ets_isr_unmask(dtm_params.intr_mask);
	ets_timer_disarm(&dtm_params.timer);
	if(dtm_params.cycles != 1) {
		ets_timer_setfn(&dtm_params.timer, ets_enter_sleep, NULL);
		x = dtm_params.dtm_2C;
		if(dtm_params.mode & 2) {__floatunsidf(__floatsidf(rand())) ... }
		dtm_params.timer_us = x;
		ets_timer_arm(&dtm_params, x, 0);
		if((dtm_params.cycles) >= 2) {
			(dtm_params.cycles)--;
		}
	}
}

// ROM:400029EC
void rtc_intr_handler(void)
{
	uint32 x = IO_RTC_INT_ST & 7; // IOREG(0x60000728) & 7;
	IO_RTC_6; // read 0x60000718
	ets_set_idle_cb(NULL, NULL);
	IO_RTC_INT_CLR |= x; // IOREG(0x60000724) |= x;
	IO_RTC_INT_ENA &= 0x78; // IOREG(0x60000720) &= 0x78;
	dtm_rtc_intr_();
}

// ROM:40002A40
void ets_rtc_int_register(void)
{
	IO_RTC_INT_ENA &= 0xFF8; // IOREG(0x60000720)
	ets_isr_attach(ETS_RTC_INUM, rtc_intr_handler, 0); // ETS_RTC_INUM = 3
	IO_RTC_INT_CLR |= 7; // OREG(0x60000724)
	ets_isr_unmask(1<<ETS_RTC_INUM);
}

//RAM_BIOS:3FFFC700
uint32 rtc_claib; // ~ = 0x7073 

//ROM:40002870
void rtc_enter_sleep(void)
{
	rtc_[4] = 0; // IOREG(0x60000710) = 0;
	RTC_CALIB_SYNC = 9;
	RTC_CALIB_SYNC |= 1<<31;
	uint32 intr_mask = dtm_params.intr_mask;
	if(intr_mask) ets_isr_mask(intr_mask);
	if(gpio_input_get() & 2) gpio_pin_wakeup_enable(2, GPIO_PIN_INTR_LOLEVEL);
	else gpio_pin_wakeup_enable(2, GPIO_PIN_INTR_HILEVEL);
	rtc_[6]  = 0x18; // IOREG(0x60000718)
	RTC_GPIO5_CFG  = 1; //	IOREG(0x600007A8) = 0x1;
	while((RTC_CALIB_VALUE & 1<<31)==0) // IOREG(0x60000370)
	rtc_claib = RTC_CALIB_VALUE & 0xFFFFF;
	if(dtm_params.sleep_func != NULL) dtm_params.sleep_func();
	x = dtm_params.time_ms;
	if(dtm_params.mode & 1) {
		rand() ....
		x = ?
	}
	dtm_params.sleep_time = x;
	x -= dtm_params.tsf_us;
	x -= (dtm_params.dtm_14 << 3) << 1; // *16
	x += x << 2; // *5
	x <<= 6;
	x -= 76800;
	dtm_params.dtm_44 += 1;
	IO_RTC_SLP_VAL = IO_RTC_SLP_CNT_VAL + __udivsi3(x, rtc_claib) + 3791; // 0x6000071C
	IO_RTC_INT_CLR |= 3; // 0x60000724
	IO_RTC_INT_ST |= 3; // 0x60000720
	rtc_[2] |= 1 << 20; // 0x60000708
}

// ROM:400027B8
void ets_enter_sleep(void)
{
	ets_set_idle_cb(rtc_enter_sleep, NULL);
}

