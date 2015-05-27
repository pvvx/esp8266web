/******************************************************************************
 * FileName: wdt.c
 * Description: disasm WDT funcs SDK 1.1.0 (libmain.a + libpp.a)
 * Author: PV`
 * (c) PV` 2015
 ******************************************************************************/
#include "user_config.h"
#include "bios.h"
#include "hw/esp8266.h"
#include "hw/specreg.h"
//#include "user_interface.h"
#include "add_sdk_func.h"

ETSTimer *SoftWdtTimer;
int soft_wdt_interval; // default = 1600 // wifi_set_sleep_type() (pm_set_sleep_type_from_upper()) set 1600 или 3000 в зависимости от режима sleep WiFi (периода timeouts_timer, noise_timer)

uint8 t0x3FFEB460[16]; // 16?

int pp_post(int x)
{
	ets_intr_lock();
	if(t0x3FFEB460[x] == 0) {
		ets_intr_unlock();
		t0x3FFEB460[x]++;
		return ets_post(32, x, 0);
	}
	ets_intr_unlock();
	return 0;
}

void pp_soft_wdt_feed()
{
	struct rst_info rst_info;
	rst_info.exccause = RSR(EXCCAUSE);
	rst_info.epc1 = RSR(EPC1);
	rst_info.epc2 = RSR(EPC2);
	rst_info.epc3 = RSR(EPC3);
	rst_info.excvaddr = RSR(EXCVADDR);
	rst_info.depc = RSR(DEPC);
	system_rtc_mem_write(0, &rst_info, sizeof(rst_info));
	if(wdt_flg == true) {
		Cache_Read_Disable();
		Cache_Read_Enable_New();
		system_restart_local();
	}
	else {
		ets_timer_disarm(SoftWdtTimer);
		ets_timer_arm_new(SoftWdtTimer, soft_wdt_interval, 0, 1);
		wdt_flg = true;
		pp_post(12);
	}
}

void pp_soft_wdt_stop(void)
{
	// ret.n
}


void pp_soft_wdt_restart(void)
{
	// ret.n
}

void pp_soft_wdt_init(void)
{
	ets_timer_setfn(SoftWdtTimer, (ETSTimerFunc *)pp_soft_wdt_feed, NULL);
	ets_timer_arm_new(SoftWdtTimer, soft_wdt_interval, 0, 1);
}

void wdt_init(int flg) // wdt_init(1) вызывается в стартовом блоке libmain.a
{
	if(flg) {
		WDT_CTRL &= 0x7e; // Disable WDT  // 0x60000900
		INTC_EDGE_EN |= 1; // 0x3ff00004 |= 1
		WDT_REG1 = 0xb; // WDT timeot
		WDT_REG2 = 0xd;
		WDT_CTRL |= 0x38;
		WDT_CTRL &= 0x79;
		WDT_CTRL |= 1;	// Enable WDT
	}
	else pp_soft_wdt_init();
}
