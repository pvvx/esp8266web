/******************************************************************************
 * FileName: wdt.c
 * Description: Alternate SDK (libmain.a) (SDK 1.1.0 no patch!)
 * (c) PV` 2015
 ******************************************************************************/

#include "user_config.h"
#include "bios.h"
#include "hw/esp8266.h"
#include "hw/specreg.h"
#include "sdk/wdt.h"
#include "sdk/fatal_errs.h"
#include "user_interface.h"
#include "sdk/add_func.h"
#include "sdk/app_main.h"

#if DEF_SDK_VERSION >= 1119 // (SDK 1.1.1..1.1.2)

//extern struct rst_info rst_inf; // SDK 1.1.0 + libmain_patch_01.a
//extern int soft_wdt_interval; // default = 1600 // wifi_set_sleep_type() (pm_set_sleep_type_from_upper()) set 1600 или 3000 в зависимости от режима sleep WiFi (периода timeouts_timer, noise_timer)
extern void pp_soft_wdt_init(void);

void ICACHE_FLASH_ATTR wdt_init(int flg)
{
	if(flg != 0) {
		RTC_MEM(0) = 0;
		WDT_CTRL &= 0x7e; // Disable WDT
		INTC_EDGE_EN |= 1; // 0x3ff00004 |= 1
		WDT_REG1 = 0xb; // WDT timeot
		WDT_REG2 = 0xd;
		WDT_CTRL |= 0x38; // WDT cfg
		WDT_CTRL &= 0x79; // WDT cfg
		WDT_CTRL |= 1;	// Enable WDT
	}
	pp_soft_wdt_init();
}

#elif DEF_SDK_VERSION != 0
#error Check WDT!
#else

#define WDT_TASK_PRIO 0x1e

bool wdt_flg;
ETSEvent wdt_eventq;

// каждые 1680403 us
void wdt_feed(void)
{
	if (RTC_MEM(0) < RST_EVENT_WDT) {
		store_exception_error(RST_EVENT_WDT);
		wdt_flg = true;
		ets_post(WDT_TASK_PRIO, 0, 0);
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
//	RTC_MEM(0) = 0;
	ets_task(wdt_task, WDT_TASK_PRIO, &wdt_eventq, 1);
	ets_isr_attach(ETS_WDT_INUM , wdt_feed, NULL);
	INTC_EDGE_EN |= 1; // 0x3ff00004 |= 1
	ets_wdt_enable(2,3,3); // mode 2 (wdt isr), step 1680403 us
}

#endif

void store_exception_error(uint32 errn)
{
		uint32 *ptr = (uint32 *)(RTC_MEM_BASE);
			*ptr++ = errn;
			*ptr++ = RSR(EXCCAUSE);
			*ptr++ = RSR(EPC1);
			*ptr++ = RSR(EPC2);
			*ptr++ = RSR(EPC3);
			*ptr++ = RSR(EXCVADDR);
			*ptr++ = RSR(DEPC);
		if(errn > RST_EVENT_WDT) {
			_ResetVector();
		}
}

void fatal_error(uint32 errn, void *addr, void *txt)
{
		uint32 *ptr = (uint32 *)(RTC_MEM_BASE);
			*ptr++ = errn;
			*ptr++ = (uint32)addr;
			*ptr++ = (uint32)txt;
		_ResetVector();
}

//RAM_BIOS:3FFFD814 aFatalException .ascii "Fatal exception (%d): \n"
#define aFatalException ((const char *)(0x3FFFD814))
//RAM_BIOS:3FFFD7CC aEpc10x08xEpc20 .ascii "epc1=0x%08x, epc2=0x%08x, epc3=0x%08x, excvaddr=0x%08x, depc=0x%08x\n"
#define aEpc10x08xEpc20 ((const char *)(0x3FFFD7CC))
//RAM_BIOS:3FFFD870 aNull .ascii "<null>"
#define aNull ((char *)(0x3FFFD870))

void ICACHE_FLASH_ATTR os_print_reset_error(void)
{
	struct rst_info * rst_inf = (struct rst_info *)&RTC_MEM(0);
//	system_rtc_mem_read(0, &rst_inf, sizeof(struct rst_info));
//	if(rst_inf->reason >= RST_EVENT_WDT
//	 && rst_inf->reason <= RST_EVENT_MAX
//	 && (!(rst_inf->reason == RST_EVENT_WDT && rst_inf->epc1 == 0x40000f68))) {
	if(rst_inf->reason > RST_EVENT_WDT && rst_inf->reason <= RST_EVENT_MAX) {
		os_printf("Old reset: ");
		switch(rst_inf->reason) {
/*		case RST_EVENT_WDT:
			os_printf("WDT (%d):\n", rst_inf->exccause);
			os_printf_plus((const char *)aEpc10x08xEpc20, rst_inf->epc1, rst_inf->epc2, rst_inf->epc3, rst_inf->excvaddr, rst_inf->depc);
			break; */
		case RST_EVENT_SOFT_WDT:
			os_printf("SoftWdt\n");
		case RST_EVENT_EXP:
			os_printf_plus((const char *)aFatalException, rst_inf->exccause);
			os_printf_plus((const char *)aEpc10x08xEpc20, rst_inf->epc1, rst_inf->epc2, rst_inf->epc3, rst_inf->excvaddr, rst_inf->depc);
			break;
		case RST_EVENT_SOFT_RESET:
			os_printf("SoftReset\n");
			break;
		case RST_EVENT_DEEP_SLEEP:
			os_printf("DeepSleep\n");
			break;
		case RST_EXT_SYS:
			os_printf("ExtReset\n");
			break;
		default: {
			char * txt = (char *)rst_inf->epc1;
			if(txt == NULL) txt = aNull;
			os_printf("Error (%u): addr=0x%08x,", rst_inf->reason, rst_inf->exccause);
			os_printf_plus(txt);
			os_printf("\n");
			}
		}
		uart_wait_tx_fifo_empty();
	}
	// rst_inf->reason = 0;
}

#ifdef DEBUG_EXCEPTION

void default_exception_handler(struct exception_frame *ef, uint32 cause)
{
	(void)cause;
	uint32 * a1;
	asm volatile ("mov %0, a1": "=a"(a1));
	a1 += 12;
	store_exception_error(0);
	struct rst_info * rst_inf = (struct rst_info *)&RTC_MEM(0);
	rst_inf->reason = RST_EVENT_EXP;
	ets_intr_unlock();
	ets_printf((const char *)aFatalException, rst_inf->exccause);
	ets_printf((const char *)aEpc10x08xEpc20, rst_inf->epc1, rst_inf->epc2, rst_inf->epc3, rst_inf->excvaddr, rst_inf->depc);
	ets_printf((const char *)" a0=%p a1=%p", ef->a0, a1);
	int i = 2;
	uint32 * ptr = &ef->a2;
	while(i < 16) {
		os_printf_plus((const char *)" a%u=%p", i++, *ptr++);
		if((i&3)==0) os_printf_plus("\n");
	}
	i = 0;
	uint32 ss[2];
	ss[0] = 0x30257830; // "0x%08x"
	while(i < 128) {
		i++;
		if((i&7)==0) ss[1] = 0x0a7838;
		else ss[1] = 0x207838;
		os_printf_plus((uint8 *)&ss[0], *a1++);
	}
	ets_delay_us(1000000);
	_ResetVector();
}
#else
void default_exception_handler(void)
{
	store_exception_error(RST_EVENT_EXP);
}
#endif


// SDK 1.1.0 + libmain_patch_01.a
/* struct rst_info *system_get_rst_info(void){
	return rst_inf;
}*/

