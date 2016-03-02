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
#include "wifi_events.h"
#include "flash_eep.h"
#include "tcp2uart.h"
#include "web_srv.h"
#ifdef USE_SNTP
#include "sntp.h"
#endif
#ifdef USE_CAPTDNS
#include "captdns.h"
#endif
#ifdef USE_WDRV
#include "driver/wdrv.h"
#endif
#ifdef UDP_TEST_PORT
#include "udp_test_port.h"
#endif
#ifdef USE_NETBIOS
#include "netbios.h"
#endif
#ifdef USE_MODBUS
#include "modbustcp.h"
#endif

struct s_probe_requests buf_probe_requests[MAX_COUNT_BUF_PROBEREQS] DATA_IRAM_ATTR;
uint32 probe_requests_count DATA_IRAM_ATTR;

#if DEBUGSOO > 2
 #define PRINT_EVENT_REASON_ENABLE
#endif

#ifdef PRINT_EVENT_REASON_ENABLE

static const uint8 txt_reason_undef[] ICACHE_RODATA_ATTR             		= "Unknown";	             // = 0,
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
static const uint8 txt_reason_auth_fail[] ICACHE_RODATA_ATTR				= "Auth_fail";				 // = 202,
static const uint8 txt_reason_assoc_fail[] ICACHE_RODATA_ATTR				= "Assoc_fail";				 // = 203,
static const uint8 txt_reason_handshake_timeout[] ICACHE_RODATA_ATTR		= "Handshake_timeout";		 // = 204


struct tab_event_reason {
	const uint8 * txt;
};
const struct tab_event_reason tab_event_reason_1_24_200[] ICACHE_RODATA_ATTR = {
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
	{txt_reason_cipher_suite_rejected},
// 200
	{txt_reason_beacon_timeout},
	{txt_reason_no_ap_found},
	{txt_reason_auth_fail},
	{txt_reason_assoc_fail},
	{txt_reason_handshake_timeout}
};

/* static const * tab_event_reason_200_201 ICACHE_RODATA_ATTR = {
	txt_reason_beacon_timeout, 
	txt_reason_no_ap_found }; */

void ICACHE_FLASH_ATTR print_event_reason(int reason)
{
    const uint8 * txt_reason = txt_reason_unspecified;
	if (reason >= 1 && reason <= 24) txt_reason = tab_event_reason_1_24_200[reason - 1].txt;
	else if (reason >= 200 && reason <= 204) txt_reason = tab_event_reason_1_24_200[reason - 200 + 23].txt;
	else txt_reason = txt_reason_undef;
	os_printf_plus(txt_reason);
}

#endif // PRINT_EVENT_REASON_ENABLE

#define COUNT_RESCONN_ST 3 // кол-во непрерывных повторов попытки соединения ST модуля к внешней AP

int flg_open_all_service DATA_IRAM_ATTR; // default = false
int st_reconn_count DATA_IRAM_ATTR;
int st_reconn_flg DATA_IRAM_ATTR;
int flg_sleep DATA_IRAM_ATTR;

os_timer_t st_disconn_timer DATA_IRAM_ATTR;

/******************************************************************************
 * FunctionName : station_reconnect_off
 ******************************************************************************/
void ICACHE_FLASH_ATTR station_reconnect_off(void)
{
	st_reconn_count = 0;
	st_reconn_flg = 0;
	ets_timer_disarm(&st_disconn_timer);
}
/******************************************************************************
 * FunctionName : station_connect_timer
 * Таймер включения новой попытки соединения ST модуля к внешней AP
 ******************************************************************************/
void ICACHE_FLASH_ATTR station_connect_timer(void)
{
	WiFi_up_from_sleep();
	if(st_reconn_count != 0
	&& (wifi_get_opmode() & STATION_MODE)
	&& wifi_station_get_auto_connect() != 0) {
		if(wifi_softap_get_station_num() == 0) { // Number count of stations which connected to ESP8266 soft-AP) {
#if DEBUGSOO > 1
			os_printf("New connect ST...\n");
#endif
			st_reconn_count = 0;
			wifi_station_connect();
		}
		else {
#if DEBUGSOO > 1
			os_printf("ST wait disconnect AP client...\n");
#endif
			st_reconn_flg = 1;
		}
	}
}
/******************************************************************************
 * FunctionName : add_next_probe_requests
 * Запись нового probe_requests в буфер buf_probe_requests
 ******************************************************************************/
void ICACHE_FLASH_ATTR add_next_probe_requests(Event_SoftAPMode_ProbeReqRecved_t *pr)
{
	uint32 i;
	uint32 e = probe_requests_count;
	if(e > MAX_COUNT_BUF_PROBEREQS) e = MAX_COUNT_BUF_PROBEREQS;
	uint32 * ptrs = (uint32 *) (&pr->mac);
	for(i = 0; i < e; i++) {
		uint32 * ptrd = (uint32 *) (&buf_probe_requests[i]);
		if(ptrs[0] == ptrd[0] && (ptrs[1]<<16) == (ptrd[1]<<16)) {
			sint8 min = ptrd[1]>>16;
			sint8 max = ptrd[1]>>24;
			if(min > pr->rssi) min = pr->rssi;
			else if(max < pr->rssi) max = pr->rssi;
			ptrd[1] = (ptrd[1] & 0xFFFF) | ((min<<16) & 0xFF0000) | ((max<<24) & 0xFF000000);
			return;
		}
	}
	uint32 * ptrd = (uint32 *) (&buf_probe_requests[probe_requests_count & (MAX_COUNT_BUF_PROBEREQS-1)]);
	ptrd[0] = ptrs[0];
	ptrd[1] = (ptrs[1] & 0xFFFF) | ((pr->rssi << 16) & 0xFF0000) | ((pr->rssi << 24) & 0xFF000000);
	if(++probe_requests_count == 0) probe_requests_count |= 0x80000000;
}
/******************************************************************************
 * FunctionName : close_all_service
 * Отключение интерфейсов для снижения потребления и т.д.
 ******************************************************************************/
void ICACHE_FLASH_ATTR close_all_service(void)
{
#if DEBUGSOO > 3
	os_printf("Close all:\n");
#endif
	if(flg_open_all_service) {
#ifdef USE_NETBIOS
		netbios_off();
#endif
#ifdef USE_TCP2UART
		tcp2uart_close();
#endif
#ifdef USE_MODBUS
		mdb_tcp_close(); // mdb_tcp_init(0);
#endif
#ifdef USE_WEB
		if(syscfg.web_port) webserver_close(syscfg.web_port); // webserver_init(0);
#endif
#ifdef UDP_TEST_PORT
		udp_test_port_init(0);
#endif
#ifdef USE_WDRV
		system_os_post(WDRV_TASK_PRIO, WDRV_SIG_INIT, 0);
#endif
		tcpsrv_close_all();
		flg_open_all_service = false;
	}
}
/******************************************************************************
 * FunctionName : open_all_service
 * if flg = 1 -> reopen
 ******************************************************************************/
void ICACHE_FLASH_ATTR open_all_service(int flg)
{
#if DEBUGSOO > 3
	os_printf("Open all (%u,%u):\n", flg, flg_open_all_service);
#endif
#ifdef USE_TCP2UART
	if(tcp2uart_servcfg == NULL) tcp2uart_start(syscfg.tcp2uart_port);
#endif
#ifdef USE_MODBUS
	if(mdb_tcp_servcfg == NULL) mdb_tcp_start(syscfg.mdb_port);
#endif
#ifdef USE_SNTP
	if(syscfg.cfg.b.sntp_ena && get_sntp_time() == 0) sntp_inits();
#endif
	if(flg == 0 || flg_open_all_service == false) {
#ifdef USE_WEB
	    if(syscfg.web_port) {
	    	TCP_SERV_CFG *p = tcpsrv_server_port2pcfg(syscfg.web_port);
	    	if(p != NULL) {
	    		if(p->port != syscfg.web_port) {
	    			tcpsrv_close(p);
	    			p = NULL;
	    		}
	    	}
	    	if(p == NULL) {
	    		webserver_init(syscfg.web_port);
	    	}
	    }
#endif
#ifdef USE_NETBIOS
	    if(syscfg.cfg.b.netbios_ena) netbios_init();
#endif
#ifdef UDP_TEST_PORT
	    if(syscfg.udp_test_port) udp_test_port_init(syscfg.udp_test_port);
#endif
#ifdef USE_WDRV
	    if(syscfg.wdrv_remote_port) system_os_post(WDRV_TASK_PRIO, WDRV_SIG_INIT, syscfg.wdrv_remote_port);
#endif
	}
    flg_open_all_service = true;
}
/******************************************************************************
 * FunctionName : wifi_handle_event_cb
 ******************************************************************************/
void ICACHE_FLASH_ATTR wifi_handle_event_cb(System_Event_t *evt)
{
#if DEBUGSOO > 1
	os_printf("WiFi event(%u): ", evt->event);
#endif
	switch (evt->event) {
		case EVENT_SOFTAPMODE_PROBEREQRECVED:
		{
#if DEBUGSOO > 1
			os_printf("Probe Request (MAC:" MACSTR ", RSSI:%d)\n",
					MAC2STR(evt->event_info.ap_probereqrecved.mac),
					evt->event_info.ap_probereqrecved.rssi);
#endif
			add_next_probe_requests(&evt->event_info.ap_probereqrecved);
			break;
		}
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
			int opmode = wifi_get_opmode();
			if(wificonfig.st.reconn_timeout != 1
			 && st_reconn_count >= COUNT_RESCONN_ST
			 && (opmode & STATION_MODE) ) {
				if(wifi_station_get_auto_connect() != 0)	{
					wifi_station_disconnect();
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
					if(opmode == STATION_MODE) {
						if(wificonfig.b.sleep != LIGHT_SLEEP_T)	WiFi_go_to_sleep(MODEM_SLEEP_T, 0xFFFFFFFF);
						else WiFi_go_to_sleep(LIGHT_SLEEP_T, wificonfig.st.reconn_timeout * 1000000);
					}
				}
			}
			if(wifi_softap_get_station_num() == 0) { // Number count of stations which connected to ESP8266 soft-AP
				close_all_service();
			}
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
				open_all_service((wifi_softap_get_station_num() == 0)? 0: 1);
			break;
		}
		case EVENT_SOFTAPMODE_STACONNECTED:
		{
			int i = wifi_softap_get_station_num(); // Number count of stations which connected to ESP8266 soft-AP
			int cs = wifi_station_get_connect_status();
#if DEBUGSOO > 1
			os_printf("Station[%u]: " MACSTR " join, AID = %d, %u\n",
					i,
					MAC2STR(evt->event_info.sta_connected.mac),
					evt->event_info.sta_connected.aid, cs);
#endif
			open_all_service((i == 1 && (!(cs == STATION_GOT_IP || cs == STATION_CONNECTING)))? 0 : 1);
#ifdef USE_CAPTDNS
			if(syscfg.cfg.b.cdns_ena) {
					captdns_init();
			}
#endif
			break;
		}
		case EVENT_SOFTAPMODE_STADISCONNECTED:
		{
			int i = wifi_softap_get_station_num(); // Number count of stations which connected to ESP8266 soft-AP
			int cs = wifi_station_get_connect_status();
#if DEBUGSOO > 1
			os_printf("Station[%u]: " MACSTR " leave, AID = %d, %u\n",
					i,
					MAC2STR(evt->event_info.sta_disconnected.mac),
					evt->event_info.sta_disconnected.aid, cs);
#endif
			if(i == 0) {
#ifdef USE_CAPTDNS
				captdns_close();
#endif
				if(!(cs == STATION_CONNECTING || cs == STATION_GOT_IP)) {
					close_all_service();
					if(st_reconn_flg != 0) {
						if((wifi_get_opmode() & STATION_MODE) && wifi_station_get_auto_connect() != 0) {
							station_reconnect_off();
#if DEBUGSOO > 1
							os_printf("New connect ST...\n");
#endif
							wifi_station_connect();
						}
					}
				}
			}
			break;
		}
#if DEBUGSOO > 1
		case EVENT_STAMODE_DHCP_TIMEOUT:
			os_printf("DHCP timeot\n");
			break;
		default:
			os_printf("?\n");
			break;
#endif
		}
}
