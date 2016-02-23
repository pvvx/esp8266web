/******************************************************************************
 * FileName: libmain.h
 * Description: Alternate SDK (libmain.a Bad Espressif SDK 1.1.0)
 * Author: PV`
 * (c) PV` 2015
*******************************************************************************/

#ifndef _INCLUDE_LIBMAIN_H_
#define _INCLUDE_LIBMAIN_H_

#include "sdk/sdk_config.h"
#include "c_types.h"
#include "user_interface.h"

//=============================================================================
// extern data
//-----------------------------------------------------------------------------
extern uint8_t _bss_start, _bss_end;
//=============================================================================
// struct
//-----------------------------------------------------------------------------
struct esp_flash_hsz {
	uint8 spi_freg: 4; // Low four bits: 0 = 40MHz, 1= 26MHz, 2 = 20MHz, 0x3 = 80MHz
	uint8 flash_size: 4; // High four bits: 0 = 512K, 1 = 256K, 2 = 1M, 3 = 2M, 4 = 4M,
}__attribute__((packed));

struct ets_FlashHeader { // Header flash (Use BIOS)
	uint8 id; // = 0xE9
	uint8 number_segs; // Number of segments
	uint8 spi_interface; // SPI Flash Interface (0 = QIO, 1 = QOUT, 2 = DIO, 0x3 = DOUT)
	struct esp_flash_hsz hsz; // options
}  __attribute__((packed));

struct ets_store_wifi_hdr { // Sector flash addr flashchip->chip_size-0x1000  (0x4027F000)
    uint8 bank;     // +00 = 0, 1 // WiFi config flash addr: 0 - flashchip->chip_size-0x3000 (0x7D000), 1 - flashchip->chip_size-0x2000
	uint32 flag;	// +04 = 0x55AA55AA
	uint32 wr_cnt;	// +08 = 0x00000119
	uint32 xx[2];	// +12 = 28, 28
	uint32 chk[2];	// +20 = 0x91, 0x91
};
//=============================================================================
// Extern data
//-----------------------------------------------------------------------------
struct s_wifi_store { // WiFi config flash addr: flashchip->chip_size - 0x3000 or -0x2000
#if DEF_SDK_VERSION >= 1400 // SDK >= 1.4.0
	uint8	boot_info[8];	//+000  g_ic+? 504 boot_version SDK 1.5.2 // 0x3FFEF164
	uint8 	wfmode[4];		//+008  g_ic.c[0x1D8] (+472) SDK 1.3.0 // 3FFF083C
	uint32	st_ssid_len;	//+012  g_ic.c +004
	uint8	st_ssid[32];	//+016  g_ic.c +008
	uint8	field_048[7];	//+048  g_ic.c +
	uint8	st_passw[64];	//+055
	uint8	field_119;		//+119
	uint8	data_120[32];	//+120
	uint8	field_152[17];	//+152
	uint8	field_169;		//+169
	uint8	field_170[6];	//+170
	uint32	ap_ssid_len;	//+176
	uint8	ap_ssid[32];	//+180
	uint8	ap_passw[64];	//+212
	uint8	field_276[32];	//+276
	uint8	field_308;		//+308
	uint8	wfchl;			//+309
	AUTH_MODE	authmode;	//+310
	uint8	field_311;		//+311
	uint8	field_312;		//+312
	uint8	field_313;		//+313
	uint8	field_314;		//+314
	uint8	field_315;		//+315
	uint16	field_316;		//+316
	uint8	field_318[2];	//+318
	uint32	st1ssid_len;	//+320
	uint8	st1ssid[32];	//+324
	uint8	st1passw[64];	//+356
	uint8	field_420[400];	//+420
	uint32	field_820[3];	//+820
	uint8	field_832[4];	//+832  wifi_station_set_auto_connect
	uint32  phy_mode;		//+836 // g_ic+1300 (+514h) // 3FFF0B78
	uint8	field_840[36];	//+840
	uint16	beacon;			//+876 // 0x3FFF289C g_ic+1400
	uint8	field_878[2];	//+878
	uint32  field_880;		//+880
	uint32  field_884;		//+884
#elif DEF_SDK_VERSION >= 1299 // SDK >= 1.3.0
	uint8	boot_info[8];	//+000  g_ic+? 464 boot_version // 3FFF0834
	uint8 	wfmode[4];		//+008  g_ic.c[0x1D8] (+472) SDK 1.3.0 // 3FFF083C
	uint32	st_ssid_len;	//+012  g_ic+?
	uint8	st_ssid[32];	//+016
	uint8	field_048[7];	//+048
	uint8	st_passw[64];	//+055
	uint8	field_119;		//+119
	uint8	data_120[32];	//+120
	uint8	field_152[17];	//+152
	uint8	field_169;		//+169
	uint8	field_170[6];	//+170
	uint32	ap_ssid_len;	//+176
	uint8	ap_ssid[32];	//+180
	uint8	ap_passw[64];	//+212
	uint8	field_276[32];	//+276
	uint8	field_308;		//+308
	uint8	wfchl;			//+309
	uint8	field_310;		//+310
	uint8	field_311;		//+311
	uint8	field_312;		//+312
	uint8	field_313;		//+313
	uint8	field_314;		//+314
	uint8	field_315;		//+315
	uint16	field_316;		//+316
	uint8	field_318[2];	//+318
	uint32	st1ssid_len;	//+320
	uint8	st1ssid[32];	//+324
	uint8	st1passw[64];	//+356
	uint8	field_420[400];	//+420
	uint32	field_820[4];	//+820
	uint32  phy_mode;		//+836 // g_ic+1300 (+514h) // 3FFF0B78
	uint8	field_840[36];	//+840
	uint16	beacon;			//+876 // 0x3FFF289C g_ic+1400
	uint8	field_878[2];	//+878
	uint32  field_880;		//+880
	uint32  field_884;		//+880
#elif DEF_SDK_VERSION >= 1200 // SDK >= 1.2.0
	uint8	boot_info[8];	//+000  g_ic+? 488 boot_version // 0x3FFF2530
	uint8 	wfmode[4];		//+008  g_ic.c[0x214] (+532) SDK 1.2.0 // 3FFF2538
	uint32	st_ssid_len;	//+012  g_ic+? 500
	uint8	st_ssid[32];	//+016
	uint8	field_048[7];	//+048
	uint8	st_passw[64];	//+055
	uint8	field_119;		//+119
	uint8	data_120[32];	//+120
	uint8	field_152[24];	//+152
	uint32	ap_ssid_len;	//+176
	uint8	ap_ssid[32];	//+180
	uint8	ap_passw[64];	//+212
	uint8	field_276[32];	//+276
	uint8	field_308[8];	//+308
	uint16	field_316;		//+316
	uint8	field_318[2];	//+318
	uint32	st1ssid_len;	//+320
	uint8	st1ssid[32];	//+324
	uint8	st1passw[64];	//+356
	uint8	field_420[400];	//+420
	uint8	field_820[16];	//+820
	uint32  phy_mode;		//+836 // g_ic+1360 (+550h) 0x3FFF2874?
	uint8	field_840[36];	//+840
	uint16	beacon;			//+876 // 0x3FFF289C g_ic+1400
	uint8	field_878[10];	//+878
#else
	uint8	boot_info[8];	//+000  //+000  g_ic+488 boot_version g_ic+0x1D9
	uint8 	wfmode[4];		//+008  g_ic+496 +0x1F0 SDK1.1.0
	uint32	st_ssid_len;	//+012  g_ic+500
	uint8	st_ssid[32];	//+016
	uint8	field_048[7];	//+048
	uint8	st_passw[64];	//+055
	uint8	field_119;		//+119
	uint8	data_120[32];	//+120
	uint8	field_152[24];	//+152
	uint32	ap_ssid_len;	//+176
	uint8	ap_ssid[32];	//+180
	uint8	ap_passw[64];	//+212
	uint8	field_276[32];	//+276
	uint8	field_308[8];	//+308
	uint16	field_316;		//+316
	uint8	field_318[2];	//+318
	uint32	st1ssid_len;	//+320
	uint8	st1ssid[32];	//+324
	uint8	st1passw[64];	//+356
	uint8	field_420[400];	//+420
	uint8	field_820[16];	//+820
	uint32  phy_mode;		//+836
	uint8	field_840[36];	//+840
	uint16	beacon;			//+876
	uint8	field_878[2];	//+878
#endif
}; // (sizeof+880)

/* SDK 1.4.0:
  g_ic: 0x3FFEF924
  g_ic.g.wifi_store: 0x3FFEFB18 = g_ic + 500
  size struct s_wifi_store: 1156
  1156 + 500 = 1656 байт
*/
#if DEF_SDK_VERSION >= 1500 // SDK >= 1.5.0
#define wifi_config_size 0x484 // 1156 bytes
#define g_ic_size (wifi_config_size + 504) // 1660 bytes
#elif DEF_SDK_VERSION >= 1400 // SDK >= 1.4.0
#define wifi_config_size 0x484 // 1156 bytes
#define g_ic_size (wifi_config_size + 500) // 1656 bytes
#elif DEF_SDK_VERSION >= 1300 // SDK >= 1.3.0
#define wifi_config_size 0x378 // 888 bytes
#define g_ic_size (wifi_config_size + 464) // 1352 bytes
#elif DEF_SDK_VERSION >= 1200 // SDK >= 1.2.0
#define wifi_config_size 0x378 // 888 bytes
//#define g_ic_size (wifi_config_size + ??) // ? bytes
#elif DEF_SDK_VERSION >= 1100 // SDK >= 1.1.0
#define wifi_config_size 0x370 // 880 bytes
#define g_ic_size (wifi_config_size + 488) // ? bytes
#endif

struct	s_g_ic{
	uint32	boot_info;		//+0000 g_ic+0	// 0x3FFF2324 boot_version?
	uint32	field_004;		//+0004 g_ic+4
	uint32	field_008;	//+0008 g_ic+8
	uint32	field_00C;	//+000C g_ic+12
	struct netif **netif1;	//+0010 g_ic+16
	struct netif **netif2;	//+0014 g_ic+20
	uint32	field_018;	//+0018 g_ic+24
	uint32	field_01C;	//+001C g_ic+28
	uint32	field_020;	//+0020 g_ic+32
	uint32	field_024;	//+0024 g_ic+36
	uint32	field_028;	//+0028 g_ic+40
	uint8	field_02C[84];	//+002C g_ic+44
	uint32	field_080;	//+0080 g_ic+128
	uint8	field_084[200];	//+0084 g_ic+132
	void *	field_14C;	//+014C g_ic+332
	uint32	ratetable;		//+0150 g_ic+336
	uint8	field_154[44];	//+0154 g_ic+340
	uint32	field_180;		//+0180 g_ic+384
	void *	field_184;	//+0184 g_ic+388
	uint32	field_188;		//+0188 g_ic+392
	uint32	field_18C;		//+018C g_ic+396
	uint32	field_190;	//+0190 g_ic+400
	uint32	field_194;		//+0194 g_ic+404
	uint32	field_198;		//+0198 g_ic+408
	uint32	field_19C;		//+019C g_ic+412
	uint32	field_1A0;	//+01A0 g_ic+416
	uint32	field_1A4;		//+01A4 g_ic+420
	uint32	field_1A8;		//+01A8 g_ic+424
	uint32	field_1AC;		//+01AC g_ic+428
	uint32	field_1B0;		//+01B0 g_ic+432
	uint32	field_1B4;		//+01B4 g_ic+436
	uint32	field_1B8;		//+01B8 g_ic+440
	uint32	field_1BC;		//+01BC g_ic+444
	uint32	field_1C0;		//+01C0 g_ic+448
	uint32	field_1C4;		//+01C4 g_ic+452
#if DEF_SDK_VERSION >= 1500 // SDK >= 1.4.0
	uint32	field_1C8[12];	//+01C8 g_ic+456 504-456
#elif DEF_SDK_VERSION >= 1400 // SDK >= 1.4.0
	uint32	field_1C8[11];	//+01C8 g_ic+456 500-456
#elif DEF_SDK_VERSION >= 1300 // SDK >= 1.3.0
	uint8	field_1C8[8];	//+01C8 g_ic+456 464-456
#elif DEF_SDK_VERSION >= 1200 // SDK >= 1.2.0
	uint8	field_1C8[68];	//+01C8 g_ic+456
#elif DEF_SDK_VERSION < 1100 // SDK < 1.1.0
	uint8	field_1C8[16];	//+01C8 g_ic+456
#else // SDK >= 1.1.0
	uint8	field_1C8[32];	//+01C8 g_ic+456
#endif
	struct s_wifi_store wifi_store;	//g_ic+???
};

typedef union _u_g_ic{
	struct s_g_ic g;
	uint8 c[g_ic_size];
	uint16 w[g_ic_size/2];
	uint32 d[g_ic_size/4];
}u_g_ic;


extern u_g_ic g_ic;

/* перенесено в add_sdk_func.h
struct s_info {
	uint32 ap_ip;	//+00
	uint32 ap_mask;	//+04
	uint32 ap_gw;	//+08
	uint32 st_ip;	//+0C
	uint32 st_mask;	//+10
	uint32 st_gw;	//+14
	uint8 ap_mac[6];	//+18
	uint8 st_mac[6];	//+1E
} __attribute__((packed));
*/

#endif /* _INCLUDE_LIBMAIN_H_ */
