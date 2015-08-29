/*
 * wifi_events.c
 * Author: pvvx
 */
#include "user_config.h"
#include "bios/ets.h"
#include "sdk/add_func.h"
#include "sdk/libmain.h"
#include "ets_sys.h"
#include "os_type.h"
#include "osapi.h"
#include "user_interface.h"
#include "wifi.h"
#include "flash_eep.h"
#include "tcp2uart.h"
#ifdef USE_SNTP
#include "sntp.h"
#endif
#ifdef USE_CAPTDNS
#include "captdns.h"
#endif
#include "web_srv.h"

#if 0
#undef DEBUGSOO
#define DEBUGSOO 2
#endif

#define MAX_RESCONN_ST 1

int st_reconn_count DATA_IRAM_ATTR;

#if DEF_SDK_VERSION >= 1303 // ждем следующий patch от китайцев
// (При выполнении wifi_station_disconnect() в событии EVENT_STAMODE_DISCONNECTED иногда AP отваливается на всегда. Требуется использовать обход данного глюка...
os_timer_t st_disconn_timer DATA_IRAM_ATTR;

/******************************************************************************
 * FunctionName : wifi_handle_event_cb
 ******************************************************************************/
void ICACHE_FLASH_ATTR station_connect_timer(void)
{
	if(st_reconn_count != 0 && (wifi_get_opmode() & STATION_MODE) && wifi_station_get_auto_connect() != 0) {
#if DEBUGSOO > 1
			os_printf("New connect ST\n");
#endif
			st_reconn_count = MAX_RESCONN_ST - 1;
//			wifi_set_opmode_current(wificonfig.b.mode);
			wifi_station_connect();
//			wifi_set_opmode_current(wificonfig.b.mode); ->
//			wifi_station_set_auto_connect(1);
//			wifi_station_set_reconnect_policy(true);
	}
}
#endif
#if	DEF_SDK_VERSION < 1303

extern void cnx_connect_timeout(void);

LOCAL void ICACHE_FLASH_ATTR stop_scan_st(void)
{
	ets_set_idle_cb(NULL, NULL);
	ets_intr_unlock();
	if(wificonfig.b.mode & SOFTAP_MODE) {
		if(wificonfig.st.reconn_timeout != 0) {
			// задать новую задержку до подключения ST
#if DEBUGSOO > 1
			os_printf("Set reconnect after %d sec\n", wificonfig.st.reconn_timeout);
#endif
			uint32 * ptr = (uint32 *)(g_ic.d[4]);
			if(ptr != NULL) {
				os_timer_t *tt = (os_timer_t *)(&ptr[1]);
				ets_timer_disarm(tt);
				ets_timer_setfn(tt, (ETSTimerFunc *)cnx_connect_timeout, NULL);
				ets_timer_arm_new(tt, wificonfig.st.reconn_timeout * 1000, 0, 1);
			}
		}
		else {
			// Восстановить режим AP
#if DEBUGSOO > 1
			os_printf("Reconnect off\n");
#endif
			wifi_station_disconnect();
		}
		wifi_softap_set_config(&wificonfig.ap.config);
	}
}
#else // ждем patch
// #warning "Bag none AP fixed?"
#endif
/******************************************************************************
 * FunctionName : wifi_handle_event_cb
 ******************************************************************************/
void ICACHE_FLASH_ATTR wifi_handle_event_cb(System_Event_t *evt)
{
#if DEBUGSOO > 1
	os_printf("WiFi event %x\n", evt->event);
#endif
	switch (evt->event) {
		case EVENT_STAMODE_CONNECTED:
		{
#if DEBUGSOO > 1
			os_printf("Connect to ssid %s, channel %d\n",
					evt->event_info.connected.ssid,
					evt->event_info.connected.channel);
#endif
			st_reconn_count = 0;
#if DEF_SDK_VERSION >= 1303 // ждем patch
			ets_timer_disarm(&st_disconn_timer);
#endif
			break;
		}
		case EVENT_STAMODE_DISCONNECTED:
		{
			st_reconn_count++;
#if DEBUGSOO > 1
			os_printf("Disconnect from ssid %s, reason %d, count %d\n",
					evt->event_info.disconnected.ssid,
					evt->event_info.disconnected.reason, st_reconn_count);
#endif
			if(st_reconn_count >= MAX_RESCONN_ST && (wifi_get_opmode() & STATION_MODE)) {
				if(wificonfig.st.reconn_timeout != 1) {
#if DEF_SDK_VERSION >= 1303 // ждем patch
						wifi_station_disconnect();
// #warning "Bag none AP fixed?"
#else
						ets_set_idle_cb(stop_scan_st, NULL);
#endif
				}
				if(wificonfig.st.reconn_timeout > 1 && wifi_station_get_auto_connect() != 0) {
#if DEF_SDK_VERSION >= 1303 // ждем patch
						ets_timer_disarm(&st_disconn_timer);
						os_timer_setfn(&st_disconn_timer, (os_timer_func_t *)station_connect_timer, NULL);
						ets_timer_arm_new(&st_disconn_timer, wificonfig.st.reconn_timeout * 1000, 0, 1);
#endif
				}
			}
			int i = wifi_softap_get_station_num(); // Number count of stations which connected to ESP8266 soft-AP
			if(i == 0) tcp2uart_close();
			break;
		}
		case EVENT_STAMODE_AUTHMODE_CHANGE:
		{
			st_reconn_count = 0;
#if DEF_SDK_VERSION >= 1303 // ждем patch
			ets_timer_disarm(&st_disconn_timer);
#endif
#if DEBUGSOO > 1
			os_printf("New AuthMode: %d -> %d\n",
					evt->event_info.auth_change.old_mode,
					evt->event_info.auth_change.new_mode);
#endif
			break;
		}
		case EVENT_STAMODE_GOT_IP:
		{
			st_reconn_count = 0;
#if DEF_SDK_VERSION >= 1303 // ждем patch
			ets_timer_disarm(&st_disconn_timer);
#endif
#if DEBUGSOO > 1
			os_printf("Station ip:" IPSTR ", mask:" IPSTR ", gw:" IPSTR "\n",
					IP2STR(&evt->event_info.got_ip.ip),
					IP2STR(&evt->event_info.got_ip.mask),
					IP2STR(&evt->event_info.got_ip.gw));
#endif
#ifdef USE_SNTP
			if(syscfg.cfg.b.sntp_ena && get_sntp_time() == 0) sntp_inits();
#endif
			if(tcp2uart_servcfg == NULL) tcp2uart_start(syscfg.tcp2uart_port);
			break;
		}
		case EVENT_SOFTAPMODE_STACONNECTED:
		{
			int i = wifi_softap_get_station_num(); // Number count of stations which connected to ESP8266 soft-AP
#if DEBUGSOO > 1
			os_printf("Station[%u]: " MACSTR " join, AID = %d\n",
					i,
					MAC2STR(evt->event_info.sta_connected.mac),
					evt->event_info.sta_connected.aid);
#endif
			if(i == 1) {
#ifdef USE_SNTP
					if(syscfg.cfg.b.sntp_ena && get_sntp_time() == 0) sntp_inits();
#endif
					if(tcp2uart_servcfg == NULL) tcp2uart_start(syscfg.tcp2uart_port);
#ifdef USE_CAPTDNS
					if(syscfg.cfg.b.cdns_ena) {
						captdns_init();
					}
#endif
			}
			break;
		}
		case EVENT_SOFTAPMODE_STADISCONNECTED:
		{
			int i = wifi_softap_get_station_num(); // Number count of stations which connected to ESP8266 soft-AP
#if DEBUGSOO > 1
			os_printf("Station[%u]: " MACSTR " leave, AID = %d\n",
					i,
					MAC2STR(evt->event_info.sta_disconnected.mac),
					evt->event_info.sta_disconnected.aid);
#endif
			if(i == 1) {
#ifdef USE_CAPTDNS
				captdns_close();
#endif
				if(wifi_station_get_connect_status() != STATION_GOT_IP) {
					tcp2uart_close();
				}
			}
			break;
		}
/*		default:
			break; */
		}
}



