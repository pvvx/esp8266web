/******************************************************************************
 * FileName: wdt.c
 * Description: wdt funcs in ROM-BIOS
 * Alternate SDK ver 0.0.1
 * Author: PV`
 * (c) PV` 2015
*******************************************************************************/
#include "c_types.h"
#include "bios.h"
#include "hw/esp8266.h"

// RAM_BIOS:3FFFC708
struct swdt_info wdt_info;

// RAM_BIOS:3FFFDDE0
ETSTimer wdt_timer;

// ROM:40002F34
uint32_t ets_wdt_get_mode(void)
{
	return wdt_info.wdt_mode;
}

// ROM:40002F14
int _wdt_abcode(int b)
{
	if(b == 3) return 11;
	if(b == 6) return 12;
	if(b == 12) return 13;
	return 0;
}

// ROM:40002FA0
void ets_wdt_enable(int mode, uint32_t a, uint32_t b)
{
	if(wdt_info.wdt_mode != 2) ets_isr_mask(0x100);
	wdt_info.a = a;
	wdt_info.b = b;
	WDT_CTRL &= 0x7E; // Disable WDT
	if(mode == 1) {
		ets_timer_setfn(wdt_timer, wdt_timer_proc, NULL);
		ets_timer_arm(wdt_timer, 10, 1);
		WDT_CTRL = 0x3C;
		WDT_REG1 = _wdt_abcode(b);
		WDT_CTRL |= 1;
		wdt_info.wdt_mode = mode;
	}
	else if(mode == 2 || mode == 4) {
		WDT_CTRL = 0x38;
		int x = _wdt_abcode(a);
		WDT_REG1 = x;
		WDT_REG2 = x;
		if(mode != 2) {
			WDT_CTRL |= 1;
			wdt_info.wdt_mode = mode;
		}
		else {
			ets_isr_unmask(0x100);
			WDT_CTRL |= 1;
			wdt_info.wdt_mode = mode;
		}
	}
	else if(mode == 3) {
		WDT_CTRL = 0x3C;
		WDT_REG1 = _wdt_abcode(a);
		WDT_CTRL |= 1;
		wdt_info.wdt_mode = mode;
	}
	else {
		WDT_CTRL |= 1;
		wdt_info.wdt_mode = mode;
	}
}

// ROM:40003158
void ets_wdt_restore(int mode)
{
	if(mode) ets_wdt_enable(mode, wdt_info.a, wdt_info.b);
}


// ROM:40002F3C
void _wdt_timer_proc(void)
{
	int mode = ets_wdt_get_mode();
	if(mode == 1)	{
		WDT_FEED = WDT_FEED_MAGIC;
		WDT_BASE[4]; // просто чтение 0x60000910
	}
	else if(mode == 2) {
		if(WDT_BASE[4] == 1) {
			WDT_BASE[6] = WDT_FEED_MAGIC; // 0x60000918
			WDT_FEED = WDT_FEED_MAGIC;
		}
	}
}

// ROM:400030F0
int ets_wdt_disable(void)
{
	WDT_CTRL &= 0x7E;
	WDT_FEED = WDT_FEED_MAGIC;
	int mode = ets_wdt_get_mode();
	if(mode == 1) {
		ets_timer_disarm(wdt_timer);
	}
	else if(mode == 2) {
		ets_isr_mask(0x100);
	}
	return mode;
}

// ROM:40002F88
void wdt_timer_proc(void)
{
	_wdt_timer_proc();
}

// ROM:40003170
void ets_wdt_init(void)
{
	WDT_CTRL &= 0xFFE;
	ets_isr_attach(8, wdt_timer_proc, 0);
	INTC_EDGE_EN |= 1;
}

