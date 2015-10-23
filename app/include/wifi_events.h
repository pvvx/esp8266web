/*
 * wifi_events.h
 *  Author: pvvx
 */

#ifndef _INCLUDE_WIFI_EVENTS_H_
#define _INCLUDE_WIFI_EVENTS_H_

#include "user_config.h"
#include "user_interface.h"

struct s_probe_requests {
	uint8 mac[6];
	sint8 rssi_min;
	sint8 rssi_max;
} __attribute__((packed));

#define MAX_COUNT_BUF_PROBEREQS 64
extern struct s_probe_requests buf_probe_requests[MAX_COUNT_BUF_PROBEREQS];
extern uint32 probe_requests_count;

void wifi_handle_event_cb(System_Event_t *evt) ICACHE_FLASH_ATTR;

extern int flg_open_all_service; // default = false

extern int st_reconn_count;
extern os_timer_t st_disconn_timer;
void station_reconnect_off(void) ICACHE_FLASH_ATTR;

void close_all_service(void) ICACHE_FLASH_ATTR;
void open_all_service(int flg) ICACHE_FLASH_ATTR;

#endif // _INCLUDE_WIFI_EVENTS_H_
