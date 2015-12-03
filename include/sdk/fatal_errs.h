/******************************************************************************
 * FileName: libmain.h
 * Description: Alternate SDK (libmain.a)
 * Author: PV`
 * (c) PV` 2015
*******************************************************************************/

#ifndef _INCLUDE_FATAL_ERRS_H_
#define _INCLUDE_FATAL_ERRS_H_


#define RST_EVENT_WDT 1
#define RST_EVENT_EXP 2
#define RST_EVENT_SOFT_WDT 3
#define RST_EVENT_SOFT_RESET 4
#define RST_EVENT_DEEP_SLEEP 5
#define RST_EXT_SYS 6
#define RST_EVENT_RES 8
#define FATAL_ERR_R6PHY 9 // "register_chipv6_phy"
#define FATAL_ERR_ALIGN 10 // "Read align4"
#define RST_EVENT_MAX 32


void fatal_error(uint32 errn, void *addr, void *txt);

#endif // _INCLUDE_FATAL_ERRS_H_
