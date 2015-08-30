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

#if DEBUGSOO > 1
#define PRINT_EVENT_REASON_ENABLE 
#endif

#ifdef PRINT_EVENT_REASON_ENABLE

static const uint8 txt_reason_undef[] ICACHE_RODATA_ATTR             		= "Unknown";	             
static const uint8 txt_reason_unspecified[] ICACHE_RODATA_ATTR             	= "Unspecified";             // = 1,
static const uint8 txt_reason_auth_expire[] ICACHE_RODATA_ATTR             	= "Auth_expire";             // = 2,
static const uint8 txt_reason_auth_leave[] ICACHE_RODATA_ATTR              	= "Auth_leave";              // = 3,
static const uint8 txt_reason_assoc_expire[] ICACHE_RODATA_ATTR            	= "Assoc_expire";            // = 4,
static const uint8 txt_reason_assoc_toomany[] ICACHE_RODATA_ATTR           	= "Assoc_toomany";           // = 5,
static const uint8 txt_reason_not_authed[] ICACHE_RODATA_ATTR              	= "Not_authed";              // = 6,
static const uint8 txt_reason_not_assoced[] ICACHE_RODATA_ATTR             	= "Not_assoced";             // = 7,
static const uint8 txt_reason_assoc_leave[] ICACHE_RODATA_ATTR             	= "Assoc_leave";             // = 8,
static const uint8 txt_reason_assoc_not_authed[] ICACHE_RODATA_ATTR        	= "Assoc_not_authed";        // = 9,
static const uint8 txt_reason_disassoc_pwrcap_bad[] ICACHE_RODATA_ATTR     	= "Disassoc_pwrcap_bad";     // = 10,  /* 11h */
static const uint8 txt_reason_disassoc_supchan_bad[] ICACHE_RODATA_ATTR    	= "Disassoc_supchan_bad";    // = 11,  /* 11h */
static const uint8 txt_reason_ie_invalid[] ICACHE_RODATA_ATTR              	= "Ie_invalid";              // = 13,  /* 11i */
static const uint8 txt_reason_mic_failure[] ICACHE_RODATA_ATTR             	= "Mic_failure";             // = 14,  /* 11i */
static const uint8 txt_reason_4way_handshake_timeout[] ICACHE_RODATA_ATTR  	= "4way_handshake_timeout";  // = 15,  /* 11i */
static const uint8 txt_reason_group_key_update_timeout[] ICACHE_RODATA_ATTR	= "Group_key_update_timeout";// = 16,  /* 11i */
static const uint8 txt_reason_ie_in_4way_differs[] ICACHE_RODATA_ATTR      	= "Ie_in_4way_differs";      // = 17,  /* 11i */
static const uint8 txt_reason_group_cipher_invalid[] ICACHE_RODATA_ATTR    	= "Group_cipher_invalid";    // = 18,  /* 11i */
static const uint8 txt_reason_pairwise_cipher_invalid[] ICACHE_RODATA_ATTR 	= "Pairwise_cipher_invalid"; // = 19,  /* 11i */
static const uint8 txt_reason_akmp_invalid[] ICACHE_RODATA_ATTR            	= "Akmp_invalid";            // = 20,  /* 11i */
static const uint8 txt_reason_unsupp_rsn_ie_version[] ICACHE_RODATA_ATTR   	= "Unsupp_rsn_ie_version";   // = 21,  /* 11i */
static const uint8 txt_reason_invalid_rsn_ie_cap[] ICACHE_RODATA_ATTR      	= "Invalid_rsn_ie_cap";      // = 22,  /* 11i */
static const uint8 txt_reason_802_1x_auth_failed[] ICACHE_RODATA_ATTR      	= "802_1x_auth_failed";      // = 23,  /* 11i */
static const uint8 txt_reason_cipher_suite_rejected[] ICACHE_RODATA_ATTR   	= "Cipher_suite_rejected";   // = 24,  /* 11i */
                                                      
static const uint8 txt_reason_beacon_timeout[] ICACHE_RODATA_ATTR          	= "Beacon_timeout";          // = 200,
static const uint8 txt_reason_no_ap_found[] ICACHE_RODATA_ATTR             	= "No_ap_found";             // = 201,

struct tab_event_reason {
	const uint8 * txt;
};
const struct tab_event_reason tab_event_reason_1_24[] ICACHE_RODATA_ATTR = {
	{txt_reason_unspecified},
	{txt_reason_auth_expire},
	{txt_reason_auth_leave},
	{txt_reason_assoc_expire},
	{txt_reason_assoc_toomany},
	{txt_reason_not_authed},
	{txt_reason_not_assoced},
	{txt_reason_assoc_leave},
	{txt_reason_assoc_not_authed},
	{txt_reason_disassoc_pwrcap_bad},
	{txt_reason_disassoc_supchan_bad},
	{txt_reason_ie_invalid},
	{txt_reason_mic_failure},
	{txt_reason_4way_handshake_timeout},
	{txt_reason_group_key_update_timeout},
	{txt_reason_ie_in_4way_differs},
	{txt_reason_group_cipher_invalid},
	{txt_reason_pairwise_cipher_invalid},
	{txt_reason_akmp_invalid},
	{txt_reason_unsupp_rsn_ie_version},
	{txt_reason_invalid_rsn_ie_cap},
	{txt_reason_802_1x_auth_failed},
	{txt_reason_cipher_suite_rejected} };

/* static const * tab_event_reason_200_201 ICACHE_RODATA_ATTR = {
	txt_reason_beacon_timeout, 
	txt_reason_no_ap_found }; */

void ICACHE_FLASH_ATTR print_event_reason(int reason)
{
    const uint8 * txt_reason = txt_reason_unspecified;
	if (reason >= 1 && reason <= 24) txt_reason = tab_event_reason_1_24[reason].txt;
	else if (reason == 200) txt_reason = txt_reason_beacon_timeout;
	else if (reason == 201) txt_reason = txt_reason_no_ap_found;
	else txt_reason = txt_reason_undef;
	os_printf_plus(txt_reason);
}

#endif // PRINT_EVENT_REASON_ENABLE

#define COUNT_RESCONN_ST 3

/* Ждем следующий patch от китайцев (все SDK включая 1.3.0):
 * При выполнении wifi_station_disconnect() в событии EVENT_STAMODE_DISCONNECTED
 * часто AP отваливается навсегда. Требуется использовать обход данного глюка...
 * При выполнеии там wifi_set_opmode() ведет к Fatal exception (28)
 */
#define DEF_SDK_ST_RECONNECT_BAG 1

int st_reconn_count DATA_IRAM_ATTR;
int st_reconn_flg DATA_IRAM_ATTR;

#ifdef DEF_SDK_ST_RECONNECT_BAG

os_timer_t st_disconn_timer DATA_IRAM_ATTR;

/******************************************************************************
 * FunctionName : SDK_ST_RECONNECT_BAG
 ******************************************************************************/
void ICACHE_FLASH_ATTR station_reconnect_off(void)
{
	st_reconn_count = 0;
	st_reconn_flg = 0;
	ets_timer_disarm(&st_disconn_timer);
}

void ICACHE_FLASH_ATTR station_connect_timer(void)
{
	if(st_reconn_count != 0
	&& (wifi_get_opmode() & STATION_MODE)
	&& wifi_station_get_auto_connect() != 0) {
		if(wifi_softap_get_station_num() == 0) { // Number count of stations which connected to ESP8266 soft-AP) {
#if DEBUGSOO > 1
			os_printf("New connect ST\n");
#endif
			st_reconn_count = 0;
			wifi_station_connect();

		}
		else {
#if DEBUGSOO > 1
			os_printf("Wait disconnect AP client...\n");
#endif
			st_reconn_flg = 1;
		}
	}
}

LOCAL void ICACHE_FLASH_ATTR stop_scan_st(void)
{
	ets_set_idle_cb(NULL, NULL);
	ets_intr_unlock();
	int opmode = wifi_get_opmode();
	if((opmode & STATION_MODE) && wifi_station_get_auto_connect() != 0)	{
		wifi_station_disconnect();
//		wifi_set_opmode_current(opmode & SOFTAP_MODE);
//		wifi_set_opmode_current(wificonfig.b.mode);
		if(wificonfig.st.reconn_timeout > 1) {
#if DEBUGSOO > 1
			os_printf("Set reconnect after %d sec\n", wificonfig.st.reconn_timeout);
#endif
			ets_timer_disarm(&st_disconn_timer);
			os_timer_setfn(&st_disconn_timer, (os_timer_func_t *)station_connect_timer, NULL);
			ets_timer_arm_new(&st_disconn_timer, wificonfig.st.reconn_timeout * 1000, 0, 1);
		}
#if DEBUGSOO > 1
		else {
			os_printf("Reconnect off\n");
		}
#endif
	}
}
#else // DEF_SDK_ST_RECONNECT_BAG  ждем patch
 #warning "DEF_SDK_ST_RECONNECT_BAG"
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
			station_reconnect_off();
			break;
		}
		case EVENT_STAMODE_DISCONNECTED:
		{
			st_reconn_count++;
#ifdef PRINT_EVENT_REASON_ENABLE
			os_printf("Disconnect from ssid %s, reason(%d): ", evt->event_info.disconnected.ssid, evt->event_info.disconnected.reason);
			print_event_reason(evt->event_info.disconnected.reason);
			os_printf(", count %d\n", st_reconn_count);
#else
#if DEBUGSOO > 1
			os_printf("Disconnect from ssid %s, reason %d, count %d\n",
					evt->event_info.disconnected.ssid,
					evt->event_info.disconnected.reason, st_reconn_count);
#endif
#endif // PRINT_EVENT_REASON_ENABLE

			if(wificonfig.st.reconn_timeout != 1 && st_reconn_count >= COUNT_RESCONN_ST && (wifi_get_opmode() & STATION_MODE)) {
#ifdef DEF_SDK_ST_RECONNECT_BAG
						ets_set_idle_cb(stop_scan_st, NULL);
#else
						wifi_station_disconnect();
						....
#endif
			}
			int i = wifi_softap_get_station_num(); // Number count of stations which connected to ESP8266 soft-AP
			if(i == 0) tcp2uart_close();
			break;
		}
		case EVENT_STAMODE_AUTHMODE_CHANGE:
		{
			station_reconnect_off();
#if DEBUGSOO > 1
			os_printf("New AuthMode: %d -> %d\n",
					evt->event_info.auth_change.old_mode,
					evt->event_info.auth_change.new_mode);
#endif
			break;
		}
		case EVENT_STAMODE_GOT_IP:
		{
			station_reconnect_off();
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
					if(st_reconn_flg != 0) {
						if((wifi_get_opmode() & STATION_MODE) && wifi_station_get_auto_connect() != 0) {
							station_reconnect_off();
							wifi_station_connect();
						}
					}
				}
			}
			break;
		}
/*		default:
			break; */
		}
}



