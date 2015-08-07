/*
 * FileName: wpa_debug.c
 * Description: Alternate SDK (libwpa.a)
 * (c) PV` 2015
 */
#include "user_config.h"
#include "bios.h"

unsigned int ICACHE_FLASH_ATTR eloop_cancel_timeout(void)
{
	return 0;
}

unsigned int ICACHE_FLASH_ATTR eloop_register_timeout(void)
{
	return 0;
}
