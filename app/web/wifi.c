/***********************************
 * FileName: wifi.c
 * PV`
 ***********************************/
#include "user_config.h"
#include "bios/ets.h"
#include "sdk/add_func.h"
#include "ets_sys.h"
#include "os_type.h"
#include "osapi.h"
#include "user_interface.h"
#include "hw/esp8266.h"
#include "sdk/flash.h"
#include "wifi.h"
#include "flash_eep.h"
#include "sdk/rom2ram.h"
#include "sdk/sys_const.h"
#include "wifi_events.h"
#include "lwip/netif.h"
#include "web_iohw.h"
#if DEF_SDK_VERSION == 1019
#include "../main/include/libmain.h"
#include "../main/include/app_main.h"
#elif DEF_SDK_VERSION > 1019
// #warning "LIGHT mode?"
#endif

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
struct wifi_config wificonfig;

int flg_wifi_sleep_enable DATA_IRAM_ATTR; // default = false

static const char wifi_ap_name[] ICACHE_RODATA_ATTR = WIFI_AP_NAME;
static const char wifi_ap_password[] ICACHE_RODATA_ATTR = WIFI_AP_PASSWORD;
static const char wifi_st_name[] ICACHE_RODATA_ATTR = WIFI_ST_NAME;
static const char wifi_st_password[] ICACHE_RODATA_ATTR = WIFI_ST_PASSWORD;

/******************************************************************************
 * FunctionName : read_macaddr_from_otp
 ******************************************************************************/
#if 0 // in system/app_main.c 
void ICACHE_FLASH_ATTR read_macaddr_from_otp(uint8 *mac)
{
	mac[0] = 0x18;
	mac[1] = 0xfe;
	mac[2] = 0x34;

	if ((!(OTP_CHIPID & (1 << 15)))||((OTP_MAC0 == 0) && (OTP_MAC1 == 0))) {
		mac[3] = 0x18;
		mac[4] = 0xfe;
		mac[5] = 0x34;
	}
	else {
		mac[3] = (OTP_MAC1 >> 8) & 0xff;
		mac[4] = OTP_MAC1 & 0xff;
		mac[5] = (OTP_MAC0 >> 24) & 0xff;
	}
}
#endif

/******************************************************************************
 * FunctionName : WiFi_go_to_sleep
 * mode: LIGHT_SLEEP_T or MODEM_SLEEP_T
 * time_us = 0xFFFFFFFF - вечный если LIGHT_SLEEP_T
 ******************************************************************************/
void ICACHE_FLASH_ATTR WiFi_go_to_sleep(enum sleep_type mode, uint32 time_us)
{
	wifi_set_opmode_current(NULL_MODE);
	wifi_fpm_set_sleep_type(mode); //	wifi_fpm_set_sleep_type(LIGHT_SLEEP_T);
	wifi_fpm_open();
	wifi_fpm_do_sleep(time_us);
	flg_wifi_sleep_enable = true;
//	Select_CLKx1(); // REG_CLR_BIT(0x3ff00014, BIT(0));
//	ets_update_cpu_frequency(80);
}
/******************************************************************************
 * FunctionName : WiFi_up_from_sleep
 ******************************************************************************/
void ICACHE_FLASH_ATTR WiFi_up_from_sleep(void)
{
	if(flg_wifi_sleep_enable) {
		flg_wifi_sleep_enable = false;
		set_cpu_clk();
		wifi_fpm_do_wakeup();
		wifi_fpm_close();
		wifi_set_opmode_current(wificonfig.b.mode);
	}
}
/******************************************************************************
 * FunctionName : Cmp_wifi_chg
 ******************************************************************************/
uint32 ICACHE_FLASH_ATTR Cmp_WiFi_chg(struct wifi_config *wcfg) {
	uwifi_chg wchg;
	wchg.ui = 0;
	WiFi_up_from_sleep();
	uint8 opmode = wifi_get_opmode();
	if (opmode != wcfg->b.mode)	wchg.b.mode = 1;
	if (wifi_get_phy_mode() != wcfg->b.phy)	wchg.b.phy = 1;
//	if (wifi_get_channel() != wcfg->b.chl)	wchg.b.chl = 1; // for sniffer
	if (wcfg->b.sleep != wifi_get_sleep_type())	wchg.b.sleep = 1;
	if ((opmode & STATION_MODE) | (wcfg->b.mode & STATION_MODE)) {
		if (wcfg->st.auto_connect != wifi_station_get_auto_connect())
			wchg.b.st_autocon = 1;
		if (wcfg->b.st_dhcp_enable != ((dhcpc_flag == 0) ? 0 : 1))
			wchg.b.st_dhcp = 1;
		{
			struct station_config st_config;
			os_memset(&st_config, 0, sizeof(st_config));
			if (wifi_station_get_config(&st_config)
				&& (os_memcmp((void*) &st_config, &wcfg->st.config,	sizeof(st_config))))
				wchg.b.st_config = 1;
		}
		{
			struct ip_info ipinfo;
			if (wifi_get_ip_info(STATION_IF, &ipinfo)
				&& (os_memcmp((void*) &ipinfo, &wcfg->st.ipinfo, sizeof(ipinfo))))
				wchg.b.st_ipinfo = 1;
		}
		{
			uint8 macaddr[6];
			if (wifi_get_macaddr(STATION_IF, macaddr)
				&& (os_memcmp(macaddr, wcfg->st.macaddr, sizeof(macaddr))))
				wchg.b.st_macaddr = 1;
		}
	}
	if ((opmode & SOFTAP_MODE) | (wcfg->b.mode & SOFTAP_MODE)) {
		if (wcfg->b.ap_dhcp_enable != ((dhcps_flag == 0) ? 0 : 1))
			wchg.b.ap_dhcp = 1;
		{
			struct softap_config ap_config;
			os_memset(&ap_config, 0, sizeof(ap_config));
			if (wifi_softap_get_config(&ap_config)
				&& (os_memcmp((void*) &ap_config, &wcfg->ap.config,	sizeof(ap_config))))
				wchg.b.ap_config = 1;
		}
		{
			struct ip_info ipinfo;
			if (wifi_get_ip_info(SOFTAP_IF, &ipinfo)
				&& (os_memcmp((void*) &ipinfo, &wcfg->ap.ipinfo, sizeof(ipinfo))))
				wchg.b.ap_ipinfo = 1;
		}
#ifndef USE_OPEN_DHCPS
		struct dhcps_lease dhcpslease;
		wifi_softap_get_dhcps_lease(&dhcpslease);
		if ((wcfg->ap.ipdhcp.start_ip.addr != dhcpslease.start_ip.addr)
				|| (wcfg->ap.ipdhcp.end_ip.addr != dhcpslease.end_ip.addr))
#else
		if ((wcfg->ap.ipdhcp.start_ip.addr != dhcps_lease.start_ip.addr)
				|| (wcfg->ap.ipdhcp.end_ip.addr != dhcps_lease.end_ip.addr))
#endif
			wchg.b.ap_ipdhcp = 1;
		{
			uint8 macaddr[6];
			if (wifi_get_macaddr(SOFTAP_IF, macaddr)
				&& (os_memcmp(macaddr, wcfg->ap.macaddr, sizeof(macaddr))))
				wchg.b.ap_macaddr = 1;
		}
	}
	if (wcfg->phy_max_tpw > MAX_PHY_TPW) wcfg->phy_max_tpw = MAX_PHY_TPW;
	if (phy_in_most_power != wcfg->phy_max_tpw) wchg.b.maxtpw = 1;
	if (hostname == NULL || (ets_strcmp(wcfg->st.hostname, hostname) != 0)) wchg.b.st_hostname = 1;
	return wchg.ui;
}
/******************************************************************************
 * FunctionName : Read_wifi_config
 ******************************************************************************/
uint32 ICACHE_FLASH_ATTR Read_WiFi_config(struct wifi_config *wcfg,
		uint32 wifi_set_mask) {
	uwifi_chg werr; werr.ui = 0;
	uwifi_chg wset;
	WiFi_up_from_sleep();
	uint8 opmode = wifi_get_opmode();
	wset.ui = wifi_set_mask;
	wifi_read_fcfg();
	if (wset.b.mode) wcfg->b.mode = opmode;
	if (wset.b.phy) wcfg->b.phy = wifi_get_phy_mode();
//	if (wset.b.chl)	wcfg->b.chl = wifi_get_channel(); // for sniffer
	if (wset.b.sleep) wcfg->b.sleep = wifi_get_sleep_type();
	if (opmode & SOFTAP_MODE) {
		if (wset.b.ap_dhcp)
			wcfg->b.ap_dhcp_enable = (dhcps_flag == 0) ? 0 : 1; // DHCP_ENABLE =0
		if ((wset.b.ap_config) && (!wifi_softap_get_config(&wcfg->ap.config)))
			werr.b.ap_config = 1;
		if ((wset.b.ap_ipinfo) && (!wifi_get_ip_info(SOFTAP_IF, &wcfg->ap.ipinfo)))
			werr.b.ap_ipinfo = 1;
		if ((wset.b.ap_macaddr) && (!wifi_get_macaddr(SOFTAP_IF, wcfg->ap.macaddr)))
			werr.b.ap_macaddr = 1;
		if (wset.b.ap_ipdhcp) {
#ifndef USE_OPEN_DHCPS
			wifi_softap_get_dhcps_lease(&wcfg->ap.ipdhcp);
#else
			wcfg->ap.ipdhcp = dhcps_lease;
#endif
		}
	}
	if (opmode & STATION_MODE) {
		if (wset.b.st_dhcp)
			wcfg->b.st_dhcp_enable = (dhcpc_flag == 0) ? 0 : 1; // DHCP_ENABLE =0
		if ((wset.b.st_config) && (!wifi_station_get_config(&wcfg->st.config)))
			werr.b.st_config = 1;
		if ((wset.b.st_ipinfo) && (!wifi_get_ip_info(STATION_IF, &wcfg->st.ipinfo)))
			werr.b.st_ipinfo = 1;
		if ((wset.b.st_macaddr) && (!wifi_get_macaddr(STATION_IF, wcfg->st.macaddr)))
			werr.b.st_macaddr = 1;
		if (wset.b.st_autocon)
			wcfg->st.auto_connect = wifi_station_get_auto_connect();
	}
	if (wset.b.maxtpw) wcfg->phy_max_tpw = phy_in_most_power;
	if (wset.b.st_hostname) {
		if (hostname == NULL) {
			wifi_station_set_default_hostname(info.st_mac);
		}
		ets_strcpy(wcfg->st.hostname, hostname);
	}
	return werr.ui;
}
/******************************************************************************
 * FunctionName : Set_wifi
 ******************************************************************************/
uint32 ICACHE_FLASH_ATTR Set_WiFi(struct wifi_config *wcfg, uint32 wifi_set_mask) {
	uwifi_chg wset; wset.ui = wifi_set_mask;
	uwifi_chg werr; werr.ui = 0;
	WiFi_up_from_sleep();
	if ((wset.b.mode)
			&& (!(wifi_set_opmode(wcfg->b.mode)))) werr.b.mode = 1;
	uint8 opmode = wifi_get_opmode();
	if ((wset.b.phy)
			&& (!(wifi_set_phy_mode(wcfg->b.phy)))) werr.b.phy = 1;
//	if ((wset.b.chl)
//			&& (!(wifi_set_channel(wcfg->b.chl))))	werr.b.chl = 1; // for sniffer
	if (wset.b.sleep) {
		if(!(wifi_set_sleep_type(wcfg->b.sleep))) werr.b.sleep = 1;
#if DEF_SDK_VERSION <= 1019
		else if(wcfg->b.sleep == 1) {
			extern ETSTimer check_timeouts_timer;
			ets_timer_disarm(&check_timeouts_timer);
			// wifi_set_sleep_type: NONE = 25, LIGHT = 3000 + reset_noise_timer(3000), MODEM = 25 + reset_noise_timer(100);
			ets_timer_arm_new(&check_timeouts_timer, 100, 1, 1);
		}
#else
// #warning "Test LIGHT mode?"
#endif
	}
	if ((wset.b.ap_config) || (wset.b.ap_ipinfo) || (wset.b.ap_ipdhcp)) {
		wifi_softap_dhcps_stop();
		if (wset.b.ap_config) {
#if (DEF_SDK_VERSION < 1000)  // исправление для SDK 0.9.5...
			struct softap_config ap_config;
			os_memset(&ap_config, 0, sizeof(ap_config));
			if (wifi_softap_get_config(&ap_config)
				&& (!os_memcmp((void*) &ap_config, &wcfg->ap.config, 32+64))
				&& (ap_config.authmode != wcfg->ap.config.authmode)) {
			    	ap_config.authmode = wcfg->ap.config.authmode;
			    	ap_config.password[1]^=0x33;
			    	wifi_softap_set_config(&ap_config);
			}
#endif
			if(!(wifi_softap_set_config(&wcfg->ap.config))) werr.b.ap_config = 1;
		}
		if ((wset.b.ap_ipinfo)
				&& (!(wifi_set_ip_info(SOFTAP_IF, &wcfg->ap.ipinfo)))) werr.b.ap_ipinfo = 1;
		if (wset.b.ap_ipdhcp) {
			if(!(wifi_softap_set_dhcps_lease(&wcfg->ap.ipdhcp))) werr.b.ap_ipdhcp = 1;
		}
		wset.b.ap_dhcp = 1;
	};
	if ((wset.b.ap_macaddr)
			&& (!(wifi_set_macaddr(SOFTAP_IF, wcfg->ap.macaddr)))) werr.b.ap_macaddr = 1;
	if (wset.b.ap_dhcp) {
		if (wcfg->b.ap_dhcp_enable) {
			if (!(wifi_softap_dhcps_start())) werr.b.ap_dhcp = 1;
		} else {
			if (!(wifi_softap_dhcps_stop())) werr.b.ap_dhcp = 1;
		};
	};
	if(opmode & STATION_MODE) {
		wifi_station_set_reconnect_policy(true);
		if(opmode == STATION_MODE) {
			wset.b.st_connect = 1;
			wset.b.st_autocon = 1;
			wcfg->st.auto_connect = 1;
		}
		if(!wcfg->b.st_dhcp_enable) wset.b.st_ipinfo = 1;
		if (wset.b.st_hostname) {
	      	  if(hostname != NULL) {
	      		  os_free(hostname);
	      		  hostname = NULL;
	      	  };
	      	  hostname = os_malloc(ets_strlen(wcfg->st.hostname) + 1);
	      	  if(hostname != NULL) {
	          	  extern struct netif * eagle_lwip_getif(int index);
	          	  struct netif * nif = eagle_lwip_getif(0);
	          	  ets_strcpy(hostname, wcfg->st.hostname);
	          	  if(nif != NULL) {
	          		  nif->hostname = hostname;
	          		  default_hostname = false;
	          	  };
	      	  };
		};
	}
	if (wset.b.maxtpw) {
		if(phy_in_most_power != wcfg->phy_max_tpw) system_phy_set_max_tpw(wcfg->phy_max_tpw);
		if(phy_in_most_power != wcfg->phy_max_tpw) werr.b.maxtpw = 1;
	}
	if ((wset.b.st_config) || (wset.b.st_ipinfo)) {
		if(opmode & STATION_MODE) {
			wifi_station_set_auto_connect(0);
			wifi_station_dhcpc_stop();
			wifi_station_disconnect();
			if(wcfg->b.st_dhcp_enable) wset.b.st_dhcp = 1;
			if(wcfg->st.auto_connect) {
				wset.b.st_connect = 1;
				wset.b.st_autocon = 1;
			}
		}
		if ((wset.b.st_config)
				&& (!(wifi_station_set_config(&wcfg->st.config)))) werr.b.st_config = 1;
		if ((wset.b.st_ipinfo)
				&& (!(wifi_set_ip_info(STATION_IF, &wcfg->st.ipinfo)))) werr.b.st_ipinfo = 1;
	};
	if (wset.b.st_dhcp) {
		if (wcfg->b.st_dhcp_enable) {
			if (!(wifi_station_dhcpc_start())) werr.b.st_dhcp = 1;
		} else {
			if ((!wset.b.st_config) && (!wset.b.st_ipinfo) && (!(wifi_station_dhcpc_stop()))) werr.b.st_dhcp = 1;
		};
	};
	if(wset.b.st_connect || wset.b.st_autocon) {
		station_reconnect_off();
		if(wcfg->st.auto_connect) {
			if(!wifi_station_connect()) werr.b.st_connect = 1;
		}
		else {
			if(!wifi_station_disconnect()) werr.b.st_connect = 1;
		}
		if(wset.b.st_autocon && (!wifi_station_set_auto_connect(wcfg->st.auto_connect))) werr.b.st_autocon = 1;
	}
#if DEBUGSOO > 1
	if(werr.ui) os_printf("ErrWiFiSet: %p\n", werr.ui);
#endif
	return werr.ui;
}
/******************************************************************************
 * FunctionName : Set_def_swifi
 ******************************************************************************/
void ICACHE_FLASH_ATTR Set_default_wificfg(struct wifi_config *wcfg,
		uint32 wifi_set_mask) {
	os_memset(wcfg, 0, sizeof(wificonfig));
	uwifi_chg wset;
	wset.ui = wifi_set_mask;
	if (wset.b.mode) wcfg->b.mode = WIFI_MODE;
	if (wset.b.phy)	wcfg->b.phy = PHY_MODE;
//	if (wset.b.chl)
		wcfg->b.chl = 1; // for sniffer
	if (wset.b.sleep) wcfg->b.sleep = DEF_WIFI_SLEEP;
	if (wset.b.ap_config) {
		wcfg->ap.config.ssid_len = rom_xstrcpy(wcfg->ap.config.ssid, wifi_ap_name);
		rom_xstrcpy(wcfg->ap.config.password, wifi_ap_password);
		wcfg->ap.config.authmode = DEF_WIFI_AUTH_MODE;
		wcfg->ap.config.ssid_hidden = 0; // no
		wcfg->ap.config.channel = 1; // wcfg->b.chl;
		wcfg->ap.config.max_connection = 4; // default
		wcfg->ap.config.beacon_interval = 100;
	}
	if (wset.b.ap_ipinfo) {
/*
		IP4_ADDR(&wcfg->ap.ipinfo.ip, 192, 168, 4, 1);
		IP4_ADDR(&wcfg->ap.ipinfo.gw, 192, 168, 4, 1);
		IP4_ADDR(&wcfg->ap.ipinfo.netmask, 255, 255, 255, 0);
*/
		wcfg->ap.ipinfo.ip.addr = WEB_DEFAULT_SOFTAP_IP;
		wcfg->ap.ipinfo.gw.addr = WEB_DEFAULT_SOFTAP_GW;
		wcfg->ap.ipinfo.netmask.addr = WEB_DEFAULT_SOFTAP_MASK;
	}
	if (wset.b.ap_ipdhcp) {
/*
		IP4_ADDR(&wcfg->ap.ipdhcp.start_ip, 192, 168, 4, 2);
		IP4_ADDR(&wcfg->ap.ipdhcp.end_ip, 192, 168, 4, 10);
*/
#if (WEB_DEFAULT_SOFTAP_IP < 0x80000000)
		wcfg->ap.ipdhcp.start_ip.addr = WEB_DEFAULT_SOFTAP_IP + 0x01000000;
		wcfg->ap.ipdhcp.end_ip.addr = WEB_DEFAULT_SOFTAP_IP + 0x09000000;
#else
		wcfg->ap.ipdhcp.start_ip.addr = (WEB_DEFAULT_SOFTAP_IP & 0x00FFFFFF) + 0x02000000;
		wcfg->ap.ipdhcp.end_ip.addr = (WEB_DEFAULT_SOFTAP_IP & 0x00FFFFFF) + 0x0A000000;
#endif
	}
	// if(mode & SOFTAP_MODE) {
	if (wset.b.ap_dhcp)
		wcfg->b.ap_dhcp_enable = 1; // enable
	//}
	if (wset.b.st_config) {
		rom_xstrcpy(wcfg->st.config.ssid,wifi_st_name);
		rom_xstrcpy(wcfg->st.config.password, wifi_st_password);
		wcfg->st.config.bssid_set = 0;
		os_memset(wcfg->st.config.bssid, 0xff, sizeof(wificonfig.st.config.bssid));
	}
	//if(mode & STATIONAP_MODE) {
	if (wset.b.st_dhcp)
		wcfg->b.st_dhcp_enable = 1;
	if (wset.b.st_autocon)
		wcfg->st.auto_connect = WIFI_ST_AUTOCONNECT;
	if (wset.b.ap_macaddr)
		read_macaddr_from_otp(wcfg->ap.macaddr);
	    wcfg->ap.macaddr[0]+=2;
	if (wset.b.st_macaddr) {
		read_macaddr_from_otp(wcfg->st.macaddr);
	}
	if (wset.b.st_ipinfo) {
/*
  		IP4_ADDR(&wcfg->st.ipinfo.ip, 192, 168, 1, 50);
		IP4_ADDR(&wcfg->st.ipinfo.gw, 192, 168, 1, 1);
		IP4_ADDR(&wcfg->st.ipinfo.netmask, 255, 255, 255, 0);
 */
		wcfg->st.ipinfo.ip.addr = WEB_DEFAULT_STATION_IP;
		wcfg->st.ipinfo.gw.addr = WEB_DEFAULT_STATION_GW;
		wcfg->st.ipinfo.netmask.addr = WEB_DEFAULT_STATION_MASK;
	}
	wcfg->st.reconn_timeout = DEF_ST_RECONNECT_TIME;
	if (wset.b.maxtpw) wcfg->phy_max_tpw = DEF_MAX_PHY_TPW;
	if (wset.b.st_hostname) {
		wifi_station_set_default_hostname(info.st_mac);
		ets_strcpy(wcfg->st.hostname, hostname);
	}
	//}
}
/******************************************************************************
 * FunctionName : print_wifi_config *Debug
 ******************************************************************************/
#if DEBUGSOO > 1
void ICACHE_FLASH_ATTR print_wifi_config(void) {
	os_printf("WiFi mode:%u phy:%u dhcp:%u/%u\n", wificonfig.b.mode,
			wificonfig.b.phy, wificonfig.b.ap_dhcp_enable,
			wificonfig.b.st_dhcp_enable);
	os_printf("AP:%s[%u] hiden(%u) psw:[%s] au:%u chl:%u maxcon:%u beacon:%u\n",
			wificonfig.ap.config.ssid, wificonfig.ap.config.ssid_len,
			wificonfig.ap.config.ssid_hidden, wificonfig.ap.config.password,
			wificonfig.ap.config.authmode, wificonfig.ap.config.channel,
			wificonfig.ap.config.max_connection, wificonfig.ap.config.beacon_interval);
	os_printf("ip:" IPSTR " gw:" IPSTR " msk:" IPSTR " mac:" MACSTR "\n",
			IP2STR(&wificonfig.ap.ipinfo.ip), IP2STR(&wificonfig.ap.ipinfo.gw),
			IP2STR(&wificonfig.ap.ipinfo.netmask),
			MAC2STR(wificonfig.ap.macaddr));
	os_printf("DHCP ip:" IPSTR ".." IPSTR"\n",
			IP2STR(&wificonfig.ap.ipdhcp.start_ip), IP2STR(&wificonfig.ap.ipdhcp.end_ip));
	os_printf("ST:[%s] psw:[%s] b:%u " MACSTR " ac:%u\n", wificonfig.st.config.ssid,
			wificonfig.st.config.password, wificonfig.st.config.bssid_set,
			MAC2STR(wificonfig.st.config.bssid), wificonfig.st.auto_connect);
	os_printf("ip:" IPSTR " gw:" IPSTR " msk:" IPSTR " mac:" MACSTR "\n",
			IP2STR(&wificonfig.st.ipinfo.ip), IP2STR(&wificonfig.st.ipinfo.gw),
			IP2STR(&wificonfig.st.ipinfo.netmask),
			MAC2STR(wificonfig.st.macaddr));
	os_printf("sleep:%u, rect:%u, maxtpw:%u, sthn:[%s]\n", wificonfig.b.sleep, wificonfig.st.reconn_timeout, wificonfig.phy_max_tpw, wificonfig.st.hostname);
}
#endif
/******************************************************************************
 * FunctionName : New_WiFi_config
 ******************************************************************************/
uint32 ICACHE_FLASH_ATTR New_WiFi_config(uint32 set_mask) {
	uint32 uiwset = Cmp_WiFi_chg(&wificonfig) & set_mask;
#if DEBUGSOO > 1
	os_printf("WiFi_set(%p)=%p\n", set_mask, uiwset);
	print_wifi_config();
#endif
	if (uiwset != 0) uiwset = Set_WiFi(&wificonfig, uiwset);
	if(set_mask & WIFI_MASK_SAVE) flash_save_cfg(&wificonfig, ID_CFG_WIFI, sizeof(wificonfig));
	if(set_mask & WIFI_MASK_REBOOT)  system_restart(); // software_reset();
	return uiwset;
}
/******************************************************************************
 * FunctionName : Setup_wifi
 ******************************************************************************/
void ICACHE_FLASH_ATTR Setup_WiFi(void) {
	total_scan_infos = 0;
	wifi_read_fcfg();
	if (wificonfig.b.mode == 0) Set_default_wificfg(&wificonfig, WIFI_MASK_ALL);
	Set_WiFi(&wificonfig, Cmp_WiFi_chg(&wificonfig) & (WIFI_MASK_SLEEP|WIFI_MASK_STDHCP|WIFI_MASK_APIPDHCP));
	wifi_set_event_handler_cb(wifi_handle_event_cb);
	return;
}
/******************************************************************************
 * FunctionName : wifi_save_fcfg
 ******************************************************************************/
bool ICACHE_FLASH_ATTR wifi_save_fcfg(uint32 rdmask)
{
	if(rdmask) Read_WiFi_config(&wificonfig, rdmask);
#if DEBUGSOO > 3
	print_wifi_config();
#endif
	return flash_save_cfg(&wificonfig, ID_CFG_WIFI, sizeof(wificonfig));
}
/******************************************************************************
 * FunctionName : wifi_read_fcfg
 ******************************************************************************/
bool ICACHE_FLASH_ATTR wifi_read_fcfg(void)
{
	if(flash_read_cfg(&wificonfig, ID_CFG_WIFI, sizeof(wificonfig)) != sizeof(wificonfig)) {
		Set_default_wificfg(&wificonfig, WIFI_MASK_ALL);
#if DEBUGSOO > 3
	print_wifi_config();
#endif
		return false;
	};
#if DEBUGSOO > 3
	print_wifi_config();
#endif
	return true;
}
/******************************************************************************
 * FunctionName : wifi_scan_cb
 * Description  : processing the scan result
 * Parameters   : arg -- Additional argument to pass to the callback function
 *                status -- scan status
 * Returns      : none
*******************************************************************************/
uint32 total_scan_infos DATA_IRAM_ATTR;
struct bss_scan_info buf_scan_infos[max_scan_bss] DATA_IRAM_ATTR;

#if	(DEF_SDK_VERSION >= 1200 && DEF_SDK_VERSION <= 1500)
LOCAL void ICACHE_FLASH_ATTR quit_scan(void)
{
	ets_set_idle_cb(NULL, NULL);
	ets_intr_unlock();
	New_WiFi_config(WIFI_MASK_MODE | WIFI_MASK_STACN); // проверить что надо восстановить и восстановить в правильной последовательности
}
#else
// #warning "Bag Fatal exception (28) over wifi_set_opmode() fixed?"
#endif
LOCAL void ICACHE_FLASH_ATTR wifi_scan_cb(void *arg, STATUS status)
{
#if DEBUGSOO > 1
	uint8 ssid[33];
	ssid[32] = '\0';
	os_printf("\nWifi scan done:\n", arg, status);
#endif
	if (status == OK) {
		struct bss_info * bss_link = (struct bss_info *) arg;
		int total_scan = 0;
		uint8 *ptr = (uint8 *)&buf_scan_infos; // ptr_scan_infos;
		while (bss_link != NULL && total_scan < max_scan_bss) {
			ets_memcpy(ptr, &bss_link->bssid, sizeof(struct bss_scan_info));
			ptr += sizeof(struct bss_scan_info);
			total_scan++;
#if DEBUGSOO > 1
			ets_memcpy(ssid, bss_link->ssid, 32);
			if(bss_link != NULL)
#if	DEF_SDK_VERSION >= 1400
				os_printf("%d: Au:%d, %d, " MACSTR ", Ch:%d, '%s', F:%u/%u, Mesh:%u\n",
					total_scan, bss_link->authmode,
					bss_link->rssi, MAC2STR(bss_link->bssid),
					bss_link->channel, ssid, bss_link->freq_offset,
					bss_link->freqcal_val, bss_link->esp_mesh_ie);
#else
				os_printf("%d: Au:%d, '%s', %d, " MACSTR ", Ch:%d\n",
					total_scan, bss_link->authmode,
					ssid, bss_link->rssi, MAC2STR(bss_link->bssid),
					bss_link->channel);
#endif
#endif // DEBUGSOO > 1
			bss_link = bss_link->next.stqe_next;
		}
		total_scan_infos = total_scan;
#if DEBUGSOO > 1
		os_printf("Found %d APs, saved in iram:%p\n", total_scan_infos, buf_scan_infos );
#endif
	}
	if(wifi_get_opmode() != wificonfig.b.mode) {
#if	(DEF_SDK_VERSION >= 1200 && DEF_SDK_VERSION <= 1500)
		ets_set_idle_cb(quit_scan, NULL);
#else
// #warning "Bag Fatal exception (28) over wifi_set_opmode() fixed?"
		New_WiFi_config(WIFI_MASK_MODE | WIFI_MASK_STACN); // проверить что надо восстановить и восстановить в правильной последовательности
#endif
	}
}
/******************************************************************************
 * FunctionName : wifi_start_scan
 ******************************************************************************/
void ICACHE_FLASH_ATTR wifi_start_scan(void)
{
	total_scan_infos = 0;
#if DEBUGSOO > 1
	os_printf("\nStart Wifi Scan...");
#endif
	int x = wifi_get_opmode();
	if(!(x&1)) {
		wifi_station_set_auto_connect(0);
		wifi_set_opmode_current(x|STATION_MODE);
	}
    if(! wifi_station_scan(NULL, wifi_scan_cb)) {
#if DEBUGSOO > 1
    	os_printf("Error!\n");
#endif
    }
#if DEBUGSOO > 1
    else os_printf("\n");
#endif
}


