#ifndef __WIFI_H__
#define __WIFI_H__
/***********************************
 * FileName: wifi.h
 * PV` 
 ***********************************/

#include "user_interface.h"
// default:
#ifndef WIFI_AP_NAME
	#define WIFI_AP_NAME	"ESP8266"
#endif
#ifndef WIFI_AP_PASSWORD
	#define WIFI_AP_PASSWORD	"0123456789"
#endif
#ifndef WIFI_ST_NAME
	#define WIFI_ST_NAME	"HOMEAP"
#endif
#ifndef WIFI_ST_PASSWORD
	#define WIFI_ST_PASSWORD	"0123456789"
#endif
#ifndef DEFAULT_WIFI_MODE
	#define DEFAULT_WIFI_MODE STATIONAP_MODE // SOFTAP_MODE // STATION_MODE
#endif
#ifndef WIFI_ST_AUTOCONNECT
	#define WIFI_ST_AUTOCONNECT 0
#endif
#ifndef PHY_MODE
	#define PHY_MODE PHY_MODE_11N // PHY_MODE_11G // PHY_MODE_11B
#endif

#ifndef DEF_WIFI_SLEEP
	#define DEF_WIFI_SLEEP NONE_SLEEP_T // MODEM_SLEEP_T; // LIGHT_SLEEP_T;
#endif
#ifndef DEF_WIFI_AUTH_MODE
	#define DEF_WIFI_AUTH_MODE AUTH_OPEN // AUTH_WPA_PSK, AUTH_WPA2_PSK, AUTH_WPA_WPA2_PSK
#endif

#ifndef DEF_ST_RECONNECT_TIME
	#define DEF_ST_RECONNECT_TIME 30 // следующая проба соединения ST произойдет через reconn_timeout секунд. При DEF_ST_RECONNECT_TIME == 1 данный алго отключен.
#endif

#define MAX_PHY_TPW 82 // maximum value of RF Tx Power, unit : 0.25dBm, range 0..82
#define DEF_MAX_PHY_TPW 82 // maximum value of RF Tx Power, unit : 0.25dBm, range 0..82


#ifndef DEBUGSOO
	#define DEBUGSOO 1
#endif

extern uint8 dhcps_flag;
extern uint8 dhcpc_flag;
extern struct dhcps_lease dhcps_lease;  // use new liblwip.a


struct bits_wifi_chg { // структура передачи изменений или ошибок
	unsigned mode		: 1;	//0  0x00000001
	unsigned phy		: 1;	//1  0x00000002
	unsigned chl		: 1;	//2  0x00000004
	unsigned sleep		: 1;	//3  0x00000008
	unsigned ap_ipinfo	: 1;	//4  0x00000010
	unsigned ap_config	: 1;	//5  0x00000020
	unsigned ap_dhcp	: 1;	//6  0x00000040
	unsigned ap_ipdhcp 	: 1;	//7  0x00000080
	unsigned ap_macaddr	: 1;	//8  0x00000100
	unsigned st_ipinfo	: 1;	//9  0x00000200
	unsigned st_config	: 1;	//10 0x00000400
	unsigned st_dhcp	: 1;	//11 0x00000800
	unsigned st_autocon	: 1;	//12 0x00001000
	unsigned st_macaddr	: 1;	//13 0x00002000
	unsigned st_hostname : 1;	//15 0x00004000
	unsigned maxtpw    	: 1;	//16 0x00008000
	unsigned st_connect : 1;	//14 0x00010000
	unsigned save_cfg 	: 1;	//30 0x00020000
	unsigned reboot 	: 1;	//31 0x00040000

} __attribute__((packed));

#define WIFI_MASK_ALL		0x0000FFFF // 0x0000FFFF
#define WIFI_MASK_MODE		0x00000001
#define WIFI_MASK_PHY		0x00000002
#define WIFI_MASK_CHL		0x00000004
#define WIFI_MASK_SLEEP		0x00000008
#define WIFI_MASK_APIP		0x00000010
#define WIFI_MASK_APCFG		0x00000020
#define WIFI_MASK_APDHCP	0x00000040
#define WIFI_MASK_APIPDHCP	0x00000080
#define WIFI_MASK_APMAC		0x00000100
#define WIFI_MASK_STIP		0x00000200
#define WIFI_MASK_STCFG		0x00000400
#define WIFI_MASK_STDHCP	0x00000800
#define WIFI_MASK_STACN		0x00001000
#define WIFI_MASK_STMAC		0x00002000
#define WIFI_MASK_STHNM		0x00004000
#define WIFI_MASK_STMTPW	0x00008000
#define WIFI_MASK_SAVE		0x00020000
#define WIFI_MASK_REBOOT	0x00040000

struct wifi_bits_cfg { // общие установки wifi
	unsigned mode	: 2;
	unsigned phy	: 2;
	unsigned chl	: 4;
	unsigned sleep	: 2;
	unsigned ap_dhcp_enable	: 1;
	unsigned st_dhcp_enable	: 1;
//	unsigned wait_reboot	: 1; // WiFi требет перезагрузки, после последних установок
} __attribute__((packed));

typedef union {
	struct bits_wifi_chg b;
	unsigned int ui;
}uwifi_chg;

struct wifi_config {	// структура конфигурации wifi
	struct wifi_bits_cfg b;
	struct {
		struct ip_info ipinfo;
		struct softap_config config;
		struct dhcps_lease ipdhcp;
		uint8 macaddr[6];
	}ap;
	struct {
//		int max_reconn; // если не удалось соединиться ST n-раз, тогда включается SOFTAP_MODE
		int reconn_timeout; // пауза в сек, если не удалось соединиться ST n-раз
		struct ip_info ipinfo;
		struct station_config config;
	    uint8  auto_connect;
	    uint8 macaddr[6];
	    uint8 hostname[32];
	}st;
	uint8 phy_max_tpw; // unit: 0.25dBm, range [0, 82], 34th byte esp_init_data_default.bin
//	uint16 phy_tpw_via_vdd33; //  Adjust RF TX Power according to VDD33, unit: 1/1024V, range [1900, 3300]
};

struct bss_scan_info { // структуры, сохранямые для вывода scan.xml (в iram)
    uint8 bssid[6];
    uint8 ssid[32];
    uint8 ssid_len;
    uint8 channel;
    sint8 rssi;
    AUTH_MODE authmode;
    uint8 is_hidden;
    sint16 freq_offset;
    sint16 freqcal_val;
	uint8 *esp_mesh_ie;
};

//#define total_scan_infos (*eraminfo.base) // #include "flash_header.h"
//#define ptr_scan_infos ((uint8 *)eraminfo.base + 4) // #include "flash_header.h"
#define max_scan_bss 32
extern uint32 total_scan_infos;
extern struct bss_scan_info buf_scan_infos[max_scan_bss];

extern struct wifi_config wificonfig;

void Setup_WiFi(void) ICACHE_FLASH_ATTR;
uint32 New_WiFi_config(uint32 set_mask) ICACHE_FLASH_ATTR; // return bits_wifi_chg/err
uint32 Read_WiFi_config(struct wifi_config *wcfg, uint32 set_mask) ICACHE_FLASH_ATTR; // return bits_wifi_chg/err
uint32 Set_WiFi(struct wifi_config *wcfg, uint32 wifi_set_mask) ICACHE_FLASH_ATTR; // return bits_wifi_chg/err
uint32 Cmp_WiFi_chg(struct wifi_config *wcfg) ICACHE_FLASH_ATTR; // return bits_wifi_chg/err
void Set_default_wificfg(struct wifi_config *wcfg, uint32 wifi_set_mask) ICACHE_FLASH_ATTR;
bool wifi_save_fcfg(uint32 rdmask) ICACHE_FLASH_ATTR;
bool wifi_read_fcfg(void) ICACHE_FLASH_ATTR;
void wifi_start_scan(void) ICACHE_FLASH_ATTR;
void WiFi_go_to_sleep(enum sleep_type mode, uint32 time_us) ICACHE_FLASH_ATTR;
void WiFi_up_from_sleep(void) ICACHE_FLASH_ATTR;

void get_macaddr_from_otp(uint8 *mac) ICACHE_FLASH_ATTR;

#if DEBUGSOO > 1
void print_wifi_config(void) ICACHE_FLASH_ATTR;
#endif
#endif // __WIFI_H__
