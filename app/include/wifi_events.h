/*
 * wifi_events.h
 *  Author: pvvx
 */

#ifndef _INCLUDE_WIFI_EVENTS_H_
#define _INCLUDE_WIFI_EVENTS_H_

#include "user_config.h"
#include "user_interface.h"

void wifi_handle_event_cb(System_Event_t *evt) ICACHE_FLASH_ATTR;

extern int st_reconn_count;
extern os_timer_t st_disconn_timer;
void station_reconnect_off(void) ICACHE_FLASH_ATTR;


#endif // _INCLUDE_WIFI_EVENTS_H_
