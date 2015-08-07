/*
 * ws2812.h
 *
 *  Created on: 04 мая 2015 г.
 *      Author: PVV
 */

#ifndef _WS2812_H_
#define _WS2812_H_

#include "user_config.h"

#if USE_TMP2NET_PORT
#include "ets_sys.h"

void ws2812_strip( uint8_t * buffer, uint16_t length ) ICACHE_FLASH_ATTR;
void ws2812_init(void) ICACHE_FLASH_ATTR;

void tpm2net_init(void) ICACHE_FLASH_ATTR;
#endif // USE_TMP2NET
#endif // _WS2812_H_
