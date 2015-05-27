/***********************************
 * FileName: wifi.c
 * PV` ver1.0 25/12/2014  SDK 0.9.6
 ***********************************/
#include "user_config.h"
#include "bios/ets.h"
#include "add_sdk_func.h"
#include "ets_sys.h"
#include "os_type.h"
#include "osapi.h"
#include "user_interface.h"
#include "hw/esp8266.h"
#include "flash.h"
#include "wifi.h"
#include "flash_eep.h"
#include "iram_info.h"
#if SDK_VERSION == 1019
#include "../main/include/libmain.h"
#include "../main/include/app_main.h"
#elif SDK_VERSION > 1019
// #warning "LIGHT mode?"
#endif

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
struct wifi_config wificonfig;

LOCAL void wifi_handle_event_cb(System_Event_t *evt) ICACHE_FLASH_ATTR;
/******************************************************************************
 * FunctionName : read_macaddr_from_otp
 ******************************************************************************/
void ICACHE_FLASH_ATTR get_macaddr_from_otp(uint8 *mac)
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
/******************************************************************************
 * FunctionName : Cmp_wifi_chg
 ******************************************************************************/
uint32 ICACHE_FLASH_ATTR Cmp_WiFi_chg(struct wifi_config *wcfg) {
	uwifi_chg wchg;
	wchg.ui = 0;
	uint8 opmode = wifi_get_opmode();
	if (opmode != wcfg->b.mode)	wchg.b.mode = 1;
	if (wifi_get_phy_mode() != wcfg->b.phy)	wchg.b.phy = 1;
	if (wifi_get_channel() != wcfg->b.chl)	wchg.b.chl = 1;
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
		if ((wcfg->ap.ipdhcp.start_ip.addr != dhcps_lease.start_ip.addr)
				|| (wcfg->ap.ipdhcp.end_ip.addr != dhcps_lease.end_ip.addr))
			wchg.b.ap_ipdhcp = 1;
		{
			uint8 macaddr[6];
			if (wifi_get_macaddr(SOFTAP_IF, macaddr)
				&& (os_memcmp(macaddr, wcfg->ap.macaddr, sizeof(macaddr))))
				wchg.b.ap_macaddr = 1;
		}
	}
	return wchg.ui;
}
/******************************************************************************
 * FunctionName : Read_wifi_config
 ******************************************************************************/
uint32 ICACHE_FLASH_ATTR Read_WiFi_config(struct wifi_config *wcfg,
		uint32 wifi_set_mask) {
	uwifi_chg werr; werr.ui = 0;
	uwifi_chg wset;
	uint8 opmode = wifi_get_opmode();
	wset.ui = wifi_set_mask;
	wifi_read_fcfg();
	if (wset.b.mode) wcfg->b.mode = opmode;
	if (wset.b.phy) wcfg->b.phy = wifi_get_phy_mode();
	if (wset.b.chl)	wcfg->b.chl = wifi_get_channel();
	if (wset.b.sleep) wcfg->b.sleep = wifi_get_sleep_type();
	if (opmode & STATION_MODE) {
		if (wset.b.ap_dhcp)
			wcfg->b.ap_dhcp_enable = (dhcps_flag == 0) ? 0 : 1; // DHCP_ENABLE =0
		if ((wset.b.ap_config) && (!wifi_softap_get_config(&wcfg->ap.config)))
			werr.b.ap_config = 1;
		if ((wset.b.ap_ipinfo) && (!wifi_get_ip_info(SOFTAP_IF, &wcfg->ap.ipinfo)))
			werr.b.ap_ipinfo = 1;
		if ((wset.b.ap_macaddr) && (!wifi_get_macaddr(SOFTAP_IF, wcfg->ap.macaddr)))
			werr.b.ap_macaddr = 1;
		if (wset.b.ap_ipdhcp) {
			wcfg->ap.ipdhcp = dhcps_lease;
		}
	}
	if (opmode & SOFTAP_MODE) {
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
	return werr.ui;
}
/******************************************************************************
 * FunctionName : Set_wifi
 ******************************************************************************/
uint32 ICACHE_FLASH_ATTR Set_WiFi(struct wifi_config *wcfg, uint32 wifi_set_mask) {
	uwifi_chg wset; wset.ui = wifi_set_mask;
	uwifi_chg werr; werr.ui = 0;
	if ((wset.b.mode)
			&& (!(wifi_set_opmode(wcfg->b.mode)))) werr.b.mode = 1;
	uint8 opmode = wifi_get_opmode();
	if ((wset.b.phy)
			&& (!(wifi_set_phy_mode(wcfg->b.phy)))) werr.b.phy = 1;
	if ((wset.b.chl)
			&& (!(wifi_set_channel(wcfg->b.chl))))	werr.b.chl = 1;
	if ((wset.b.ap_config) || (wset.b.ap_ipinfo) || (wset.b.ap_ipdhcp)) {
		wifi_softap_dhcps_stop();
		if (wset.b.ap_config) {
#if (SDK_VERSION < 1000)  // исправление для SDK 0.9.5...
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
	if (wset.b.sleep) {
		if(!(wifi_set_sleep_type(wcfg->b.sleep))) werr.b.sleep = 1;
#if SDK_VERSION <= 1019
		else if(wcfg->b.sleep == 1) {
			ets_timer_disarm(&check_timeouts_timer);
			// wifi_set_sleep_type: NONE = 25, LIGHT = 3000 + reset_noise_timer(3000), MODEM = 25 + reset_noise_timer(100);
			ets_timer_arm_new(&check_timeouts_timer, 100, 1, 1);
		}
#elif SDK_VERSION > 1019
//#error "LIGHT mode?"
#endif
	}
	if (wset.b.st_dhcp) {
		if (wcfg->b.st_dhcp_enable) {
			if (!(wifi_station_dhcpc_start())) werr.b.st_dhcp = 1;
		} else {
			if ((!wset.b.st_config) && (!wset.b.st_ipinfo) && (!(wifi_station_dhcpc_stop()))) werr.b.st_dhcp = 1;
		};
	};
	if(wset.b.st_connect || wset.b.st_autocon) {
		if(wcfg->st.auto_connect) {
			if(!wifi_station_connect()) werr.b.st_connect = 1;
		}
		else {
			if(!wifi_station_disconnect()) werr.b.st_connect = 1;
		}
		if(wset.b.st_autocon && (!wifi_station_set_auto_connect(wcfg->st.auto_connect))) werr.b.st_autocon = 1;
	}
#if DEBUGSOO > 1
		if(werr.ui) os_printf("ErrWiFiSet: 0x%x\n", werr.ui);
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
	if (wset.b.mode)
		wcfg->b.mode = WIFI_MODE;
	if (wset.b.phy)
		wcfg->b.phy = PHY_MODE;
	if (wset.b.chl)
		wcfg->b.chl = 1;
	if (wset.b.sleep)
		wcfg->b.sleep = NONE_SLEEP_T; // LIGHT_SLEEP_T; // NONE_SLEEP_T MODEM_SLEEP_T
	if (wset.b.ap_config) {
		wcfg->ap.config.ssid_len = os_sprintf(wcfg->ap.config.ssid,
		WIFI_AP_NAME);
		os_sprintf(wcfg->ap.config.password, WIFI_AP_PASSWORD);
		wcfg->ap.config.authmode = AUTH_OPEN;
		wcfg->ap.config.ssid_hidden = 0; // no
		wcfg->ap.config.channel = wcfg->b.chl;
		wcfg->ap.config.max_connection = 4; // default
		wcfg->ap.config.beacon_interval = 100;
	}
	if (wset.b.ap_ipinfo) {
		IP4_ADDR(&wcfg->ap.ipinfo.ip, 192, 168, 4, 1);
		IP4_ADDR(&wcfg->ap.ipinfo.gw, 192, 168, 4, 1);
		IP4_ADDR(&wcfg->ap.ipinfo.netmask, 255, 255, 255, 0);
	}
	if (wset.b.ap_ipdhcp) {
		IP4_ADDR(&wcfg->ap.ipdhcp.start_ip, 192, 168, 4, 2);
		IP4_ADDR(&wcfg->ap.ipdhcp.end_ip, 192, 168, 4, 10);
	}
	// if(mode & SOFTAP_MODE) {
	if (wset.b.ap_dhcp)
		wcfg->b.ap_dhcp_enable = 1; // enable
	//}
	if (wset.b.st_config) {
		os_sprintf(wcfg->st.config.ssid, WIFI_ST_NAME);
		os_sprintf(wcfg->st.config.password, WIFI_ST_PASSWORD);
		wcfg->st.config.bssid_set = 0;
		os_memset(wcfg->st.config.bssid, 0xff, sizeof(wificonfig.st.config.bssid));
	}
	//if(mode & STATIONAP_MODE) {
	if (wset.b.st_dhcp)
		wcfg->b.st_dhcp_enable = 1;
	if (wset.b.st_autocon)
		wcfg->st.auto_connect = WIFI_ST_AUTOCONNECT;
	if (wset.b.ap_macaddr)
		get_macaddr_from_otp(wcfg->ap.macaddr);
	    wcfg->ap.macaddr[0]+=2;
	if (wset.b.st_macaddr) {
		get_macaddr_from_otp(wcfg->st.macaddr);
	}
	//}
}
/******************************************************************************
 * FunctionName : print_wifi_config *Debug
 ******************************************************************************/
#if DEBUGSOO > 1
void ICACHE_FLASH_ATTR print_wifi_config(void) {
	os_printf("WiFi mode:%u chl:%u phy:%u dhcp:%u/%u\n", wificonfig.b.mode,
			wificonfig.b.chl, wificonfig.b.phy, wificonfig.b.ap_dhcp_enable,
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
	os_printf("sleep:%u\n", wificonfig.b.sleep);
}
#endif
/******************************************************************************
 * FunctionName : New_WiFi_config
 ******************************************************************************/
uint32 ICACHE_FLASH_ATTR New_WiFi_config(uint32 set_mask) {
	uint32 uiwset = Cmp_WiFi_chg(&wificonfig) & set_mask;
#if DEBUGSOO > 1
	os_printf("WiFi_set(0x%x)=0x%x\n", set_mask, uiwset);
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
uint32 ICACHE_FLASH_ATTR Setup_WiFi(void) {
#if 0
 #if DEBUGSOO > 1
	os_printf("Default Start WiFi:\n");
	Read_WiFi_config(&wificonfig, WIFI_MASK_ALL);
	print_wifi_config();
 #endif
#endif

#if DEBUGSOO > 1
	os_printf("\nSetup WiFi:\n");
#endif
	total_scan_infos = 0;
	wifi_read_fcfg();
	if (wificonfig.b.mode == 0) Set_default_wificfg(&wificonfig, WIFI_MASK_ALL);
	uint32 wifi_set_err = New_WiFi_config(WIFI_MASK_ALL);
/*	uint32 wifi_set_err = Set_WiFi(&wificonfig, WIFI_MASK_ALL);
#if DEBUGSOO > 1
	print_wifi_config();
#endif */
	wifi_set_event_handler_cb(wifi_handle_event_cb);
	return wifi_set_err;
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
LOCAL void ICACHE_FLASH_ATTR wifi_scan_cb(void *arg, STATUS status)
{
#if DEBUGSOO > 1
	uint8 ssid[33];
	ssid[32] = '\0';
	os_printf("\nWifi scan done:\n", arg, status);
#endif
	if (status == OK) {
		struct bss_info * bss_link = (struct bss_info *) arg;
		bss_link = bss_link->next.stqe_next;
		int total_scan = 0;
		int max_scan = eraminfo.size / (sizeof(struct bss_scan_info));
		uint8 *ptr = ptr_scan_infos;
		while (bss_link != NULL && total_scan < max_scan) {
			ets_memcpy(ptr, &bss_link->bssid, sizeof(struct bss_scan_info));
			ptr += sizeof(struct bss_scan_info);
			total_scan++;
#if DEBUGSOO > 1
			ets_memcpy(ssid, bss_link->ssid, 32);
			if(bss_link != NULL) os_printf("%d: Au:%d, '%s', %d, " MACSTR ", Ch:%d\n", total_scan, bss_link->authmode,
					ssid, bss_link->rssi, MAC2STR(bss_link->bssid),
					bss_link->channel);
#endif
			bss_link = bss_link->next.stqe_next;
		}
		total_scan_infos = total_scan;
#if DEBUGSOO > 1
		os_printf("Found %d APs, saved in iram:%p\n", total_scan_infos, ptr_scan_infos );
#endif
	}
	if(wifi_get_opmode() != wificonfig.b.mode) {
		New_WiFi_config(WIFI_MASK_MODE | WIFI_MASK_STACN); // проверить что надо восстановить и восстановить в правильной последовательности
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
		wifi_set_opmode(x|1);
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
/******************************************************************************
 * FunctionName : wifi_handle_event_cb
 ******************************************************************************/
LOCAL void ICACHE_FLASH_ATTR wifi_handle_event_cb(System_Event_t *evt)
{
#if DEBUGSOO > 1
	os_printf("WiFi event %x\n", evt->event);
	switch (evt->event) {
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
		case EVENT_STAMODE_GOT_IP:
			os_printf("Station ip:" IPSTR ",mask:" IPSTR ",gw:" IPSTR "\n",
					IP2STR(&evt->event_info.got_ip.ip),
					IP2STR(&evt->event_info.got_ip.mask),
					IP2STR(&evt->event_info.got_ip.gw));
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
			os_printf("Station: " MACSTR "join, AID = %d\n",
					MAC2STR(evt->event_info.sta_connected.mac),
					evt->event_info.sta_connected.aid);
/*			os_printf("AP info ip:" IPSTR ",mask:" IPSTR ",gw:" IPSTR "\n",
					IP2STR(((struct ip_addr *)&info.ap_ip)),
					IP2STR((struct ip_addr *)&info.ap_mask),
					IP2STR((struct ip_addr *)&info.ap_gw)); */
			break;
		case EVENT_SOFTAPMODE_STADISCONNECTED:
				os_printf("Station: " MACSTR "leave, AID = %d\n",
						MAC2STR(evt->event_info.sta_disconnected.mac),
						evt->event_info.sta_disconnected.aid);
			break;
/*		default:
			break; */
		}
#endif
}

