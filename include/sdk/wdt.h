/******************************************************************************
 * FileName: wdt.h
 * Description: Alternate SDK (libmain.a)
 * Author: PV`
 * (c) PV` 2015
 * ver 0.0.0 (b0)
*******************************************************************************/

#ifndef _INCLUDE_WDT_H_
#define _INCLUDE_WDT_H_

#include "ets_sys.h"
#include "sdk/fatal_errs.h"

#if DEF_SDK_VERSION >= 1119 // (SDK 1.1.1..1.1.2)
void wdt_init(int flg) ICACHE_FLASH_ATTR;
#else
void wdt_init(void) ICACHE_FLASH_ATTR;
void wdt_feed(void);
void wdt_task(ETSEvent *e);
#endif

// #define DEBUG_EXCEPTION // для отладки

#ifdef DEBUG_EXCEPTION
struct exception_frame
{
  uint32 epc;
  uint32 ps;
  uint32 sar;
  uint32 unused;
  union {
    struct {
      uint32 a0;
      // note: no a1 here!
      uint32 a2;
      uint32 a3;
      uint32 a4;
      uint32 a5;
      uint32 a6;
      uint32 a7;
      uint32 a8;
      uint32 a9;
      uint32 a10;
      uint32 a11;
      uint32 a12;
      uint32 a13;
      uint32 a14;
      uint32 a15;
    };
    uint32 a_reg[15];
  };
  uint32 cause;
};
void default_exception_handler(struct exception_frame *ef, uint32 cause);
#else
void default_exception_handler(void);
#endif

void store_exception_error(uint32 errn);

void os_print_reset_error(void) ICACHE_FLASH_ATTR;


#endif /* _INCLUDE_WDT_H_ */
