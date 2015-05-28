/******************************************************************************
 * FileName: wdt.c
 * Description: Alternate SDK (libmain.a)
 * Author: PV`
 * (c) PV` 2015
 * ver 0.0.0 (b0)
*******************************************************************************/

#include "user_config.h"
#include "bios.h"
#include "hw/esp8266.h"
#include "hw/specreg.h"
#include "wdt.h"
#include "fatal_errs.h"
#include "user_interface.h"
#include "add_sdk_func.h"

bool wdt_flg;

// struct rst_info rst_inf; // SDK 1.1.0 + libmain_patch_01.a

ETSEvent wdt_eventq;

void store_exception_error(uint32_t errn)
{
		uint32_t *ptr = (uint32_t *)(RTC_MEM_BASE);
			*ptr++ = errn;
			*ptr++ = RSR(EXCCAUSE);
			*ptr++ = RSR(EPC1);
			*ptr++ = RSR(EPC2);
			*ptr++ = RSR(EPC3);
			*ptr++ = RSR(EXCVADDR);
			*ptr++ = RSR(DEPC);
		if(errn > RST_EVENT_WDT) do _ResetVector(); while(1);
}

void wdt_feed(void)
{
	if (RTC_MEM(0) < RST_EVENT_WDT) {
		store_exception_error(RST_EVENT_WDT);
		wdt_flg = true;
		ets_post(0x1e, 0, 0);
	}
}

void wdt_task(ETSEvent *e)
{
	ets_intr_lock();
	if(wdt_flg) {
		wdt_flg = false;
		if (RTC_MEM(0) <= RST_EVENT_WDT) RTC_MEM(0) = 0;
		WDT_FEED = WDT_FEED_MAGIC;
	}
	ets_intr_unlock();
}

void ICACHE_FLASH_ATTR wdt_init(void)
{
	RTC_MEM(0) = 0;
//	wdt_flg = true;
	ets_task(&wdt_task, 0x1e, &wdt_eventq, 1);

	WDT_CTRL &= 0x7e; // Disable WDT

	ets_isr_attach(ETS_WDT_INUM , wdt_feed, NULL);

	INTC_EDGE_EN |= 1; // 0x3ff00004 |= 1
	WDT_REG1 = 0xb; // WDT timeot
	WDT_REG2 = 0xb;
	WDT_CTRL = (WDT_CTRL | 0x38) & 0x79; // WDT cfg
	WDT_CTRL |= 1;	// Enable WDT
	ets_isr_unmask(1 << ETS_WDT_INUM); // Enable WDT isr
}

void default_exception_handler(void)
{
	store_exception_error(RST_EVENT_EXP);
}

void fatal_error(uint32_t errn, void *addr, void *txt)
{
		uint32_t *ptr = (uint32_t *)(RTC_MEM_BASE);
			*ptr++ = errn;
			*ptr++ = (uint32_t)addr;
			*ptr++ = (uint32_t)txt;
		do _ResetVector(); while(1);
}

//RAM_BIOS:3FFFD814 aFatalException .ascii "Fatal exception (%d): \n"
#define aFatalException ((const char *)(0x3FFFD814))
//RAM_BIOS:3FFFD7CC aEpc10x08xEpc20 .ascii "epc1=0x%08x, epc2=0x%08x, epc3=0x%08x, excvaddr=0x%08x, depc=0x%08x\n"
#define aEpc10x08xEpc20 ((const char *)(0x3FFFD7CC))
//RAM_BIOS:3FFFD870 aNull .ascii "<null>"
#define aNull ((char *)(0x3FFFD870))

void ICACHE_FLASH_ATTR os_print_reset_error(void)
{
	uint32 errn = RTC_MEM(0);
	if(errn >= RST_EVENT_WDT && errn <= RST_EVENT_MAX) {
		struct rst_info rst_inf;
		system_rtc_mem_read(0, &rst_inf, sizeof(rst_inf));
		os_printf("Old reset: ");
		switch(errn) {
		case RST_EVENT_WDT:
			os_printf("WDT (%d):\n", rst_inf.exccause);
			os_printf_plus(aEpc10x08xEpc20, rst_inf.epc1, rst_inf.epc2, rst_inf.epc3, rst_inf.excvaddr, rst_inf.depc);
			break;
		case RST_EVENT_EXP:
			os_printf_plus(aFatalException, rst_inf.exccause);
			os_printf_plus(aEpc10x08xEpc20, rst_inf.epc1, rst_inf.epc2, rst_inf.epc3, rst_inf.excvaddr, rst_inf.depc);
			break;
		case RST_EVENT_SOFT_RESET:
			os_printf("SoftReset\n");
			break;
		case RST_EVENT_DEEP_SLEEP:
			os_printf("Deep_Sleep\n");
			break;
		default: {
			char * txt = (char *)rst_inf.epc1;
			if(txt == NULL) txt = aNull;
			os_printf("Error (%u): addr=0x%08x, ", errn, rst_inf.exccause);
			os_printf_plus(txt);
			os_printf("\n");
			}
		}
	}
	RTC_MEM(0) = 0;
//	rst_inf.reason = 0; // SDK 1.1.0 + libmain_patch_01.a
}

// SDK 1.1.0 + libmain_patch_01.a
/* struct rst_info *system_get_rst_info(void){
	return rst_inf;
}*/

