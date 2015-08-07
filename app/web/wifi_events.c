/*
 * wifi_events.c
 * Author: pvvx
 */
#include "user_config.h"
#include "bios/ets.h"
#include "sdk/add_func.h"
#include "ets_sys.h"
#include "os_type.h"
#include "osapi.h"
#include "user_interface.h"
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
/*			os_printf("ST info ip:" IPSTR ",mask:" IPSTR ",gw:" IPSTR "\n",
					IP2STR(((struct ip_addr *)&info.st_ip)),
					IP2STR((struct ip_addr *)&info.st_mask),
					IP2STR((struct ip_addr *)&info.st_gw)); */
#endif
			break;
		}
		case EVENT_STAMODE_DISCONNECTED:
		{
#if DEBUGSOO > 1
			os_printf("Disconnect from ssid %s, reason %d\n",
					evt->event_info.disconnected.ssid,
					evt->event_info.disconnected.reason);
#endif
			int i = wifi_softap_get_station_num(); // Number count of stations which connected to ESP8266 soft-AP
			if(i == 0) tcp2uart_close();
			break;
		}
		case EVENT_STAMODE_AUTHMODE_CHANGE:
		{
#if DEBUGSOO > 1
			os_printf("New AuthMode: %d -> %d\n",
					evt->event_info.auth_change.old_mode,
					evt->event_info.auth_change.new_mode);
#endif
			break;
		}
		case EVENT_STAMODE_GOT_IP:
		{
#if DEBUGSOO > 1
			os_printf("Station ip:" IPSTR ", mask:" IPSTR ", gw:" IPSTR "\n",
					IP2STR(&evt->event_info.got_ip.ip),
					IP2STR(&evt->event_info.got_ip.mask),
					IP2STR(&evt->event_info.got_ip.gw));
#endif
#ifdef USE_SNTP
					if(syscfg.cfg.b.sntp_ena && get_sntp_time() == 0) sntp_init();
#endif
					if(tcp2uart_servcfg == NULL) tcp2uart_start(syscfg.tcp2uart_port);
/*			os_printf("ST info ip:" IPSTR ",mask:" IPSTR ",gw:" IPSTR "\n",
					IP2STR(((struct ip_addr *)&info.st_ip)),
					IP2STR((struct ip_addr *)&info.st_mask),
					IP2STR((struct ip_addr *)&info.st_gw));
			struct ip_info ipinfo;
			if(wifi_get_ip_info(STATION_IF, &ipinfo))
				os_printf("ST wifi_get_ip_info() ip:" IPSTR ",mask:" IPSTR ",gw:" IPSTR "\n",
					IP2STR(((struct ip_addr *)&ipinfo.ip)),
					IP2STR((struct ip_addr *)&ipinfo.netmask),
					IP2STR((struct ip_addr *)&ipinfo.gw));
			else os_printf("wifi_get_ip_info() error!"); */
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
/*			os_printf("AP info ip:" IPSTR ",mask:" IPSTR ",gw:" IPSTR "\n",
					IP2STR(((struct ip_addr *)&info.ap_ip)),
					IP2STR((struct ip_addr *)&info.ap_mask),
					IP2STR((struct ip_addr *)&info.ap_gw)); */
#endif
			if(i == 1) {
#ifdef USE_SNTP
					if(syscfg.cfg.b.sntp_ena && get_sntp_time() == 0) sntp_init();
#endif
					if(tcp2uart_servcfg == NULL) tcp2uart_start(syscfg.tcp2uart_port);
#ifdef USE_CAPTDNS
					if(syscfg.cfg.b.cdns_ena) {
						captdns_init();
						// webserver_init(443);
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
					// webserver_close(443);
			}
			break;
		}
/*		default:
			break; */
		}
}



