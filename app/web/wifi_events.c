/*
 * wifi_events.c
 * Author: pvvx
 */
#include "user_config.h"
#include "bios/ets.h"
#include "add_sdk_func.h"
#include "ets_sys.h"
#include "os_type.h"
#include "osapi.h"
#include "user_interface.h"
#include "flash_eep.h"
#include "tcp2uart.h"
#ifdef USE_SNTP
#include "sntp.h"
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
#if DEBUGSOO > 1
		case EVENT_STAMODE_CONNECTED:
			os_printf("Connect to ssid %s, channel %d\n",
					evt->event_info.connected.ssid,
					evt->event_info.connected.channel);
/*			os_printf("ST info ip:" IPSTR ",mask:" IPSTR ",gw:" IPSTR "\n",
					IP2STR(((struct ip_addr *)&info.st_ip)),
					IP2STR((struct ip_addr *)&info.st_mask),
					IP2STR((struct ip_addr *)&info.st_gw)); */
			break;
		case EVENT_STAMODE_DISCONNECTED:
			os_printf("Disconnect from ssid %s, reason %d\n",
					evt->event_info.disconnected.ssid,
					evt->event_info.disconnected.reason);
			break;
		case EVENT_STAMODE_AUTHMODE_CHANGE:
			os_printf("New AuthMode: %d -> %d\n",
					evt->event_info.auth_change.old_mode,
					evt->event_info.auth_change.new_mode);
			break;
#endif
		case EVENT_STAMODE_GOT_IP:
#if DEBUGSOO > 1
			os_printf("Station ip:" IPSTR ", mask:" IPSTR ", gw:" IPSTR "\n",
					IP2STR(&evt->event_info.got_ip.ip),
					IP2STR(&evt->event_info.got_ip.mask),
					IP2STR(&evt->event_info.got_ip.gw));
#endif
#ifdef USE_SNTP
					if(syscfg.cfg.b.sntp_ena && get_sntp_time() == 0) sntp_init();
#endif
					tcp2uart_start(syscfg.tcp2uart_port);
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
		case EVENT_SOFTAPMODE_STACONNECTED:
#if DEBUGSOO > 1
			os_printf("Station[%u]: " MACSTR "join, AID = %d\n",
					wifi_softap_get_station_num(),
					MAC2STR(evt->event_info.sta_connected.mac),
					evt->event_info.sta_connected.aid);
/*			os_printf("AP info ip:" IPSTR ",mask:" IPSTR ",gw:" IPSTR "\n",
					IP2STR(((struct ip_addr *)&info.ap_ip)),
					IP2STR((struct ip_addr *)&info.ap_mask),
					IP2STR((struct ip_addr *)&info.ap_gw)); */
#endif
#ifdef USE_SNTP
					if(syscfg.cfg.b.sntp_ena && get_sntp_time() == 0) sntp_init();
#endif
					tcp2uart_start(syscfg.tcp2uart_port);
			break;
#if DEBUGSOO > 1
		case EVENT_SOFTAPMODE_STADISCONNECTED:
				os_printf("Station[%u]: " MACSTR "leave, AID = %d\n",
						wifi_softap_get_station_num(),
						MAC2STR(evt->event_info.sta_disconnected.mac),
						evt->event_info.sta_disconnected.aid);
			break;
/*		default:
			break; */
#endif
		}
}



