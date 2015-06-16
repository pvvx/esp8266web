/******************************************************************************
 * FileName: libmain.h
 * Description: Alternate SDK (libmain.a Bad Espressif SDK 1.1.0)
 * Author: PV`
 * (c) PV` 2015
*******************************************************************************/

#ifndef _INCLUDE_LIBMAIN_H_
#define _INCLUDE_LIBMAIN_H_

#include "c_types.h"

#define DEBUG_UART 0 // включить вывод в загрузчике сообщений

//=============================================================================
// extern data
//-----------------------------------------------------------------------------
extern uint8_t _bss_start, _bss_end;
extern SpiFlashChip * flashchip;
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
	uint32 x; 		// +08 = 0x00000119
	uint32 xx[2];	// +12 = 28, 28
	uint32 chk[2];	// +20 = 0x91, 0x91
};
//=============================================================================
// Extern data
//-----------------------------------------------------------------------------
struct s_wifi_store { // WiFi config flash addr: flashchip->chip_size - 0x3000 or -0x2000
	uint8	field_000[8];	//+000  g_ic+488 boot_version g_ic+0x1D9
	uint8 	wfmode[4];		//+008  g_ic+496 +0x1F0 // SDK1.1.0
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
}; // (sizeof+880)

#define wifi_config_size 0x370 // 880 bytes

struct	s_g_ic{
	void *	scan_pages;		//+0000 g_ic+0
	void *	gScanStruct;	//+0004 g_ic+4
	uint32	anonymous_0;	//+0008 g_ic+8
	uint32	anonymous_1;	//+000C g_ic+12
	struct netif **netif1;	//+0010 g_ic+16
	struct netif **netif2;	//+0014 g_ic+20
	uint32	anonymous_4;	//+0018 g_ic+24
	uint32	anonymous_5;	//+001C g_ic+28
	uint32	anonymous_6;	//+0020 g_ic+32
	uint32	anonymous_7;	//+0024 g_ic+36
	uint32	anonymous_8;	//+0028 g_ic+40
	uint8	field_2C[84];	//+002C g_ic+44
	uint32	anonymous_9;	//+0080 g_ic+128
	uint8	field_84[200];	//+0084 g_ic+132
	void *	anonymous_10;	//+014C g_ic+332
	uint32	ratetable;		//+0150 g_ic+336
	uint8	field_154[44];	//+0154 g_ic+340 boot_version; //+0159
	uint32	field_180;		//+0180 g_ic+384
	void *	anonymous_12;	//+0184 g_ic+388
	uint32	field_188;		//+0188 g_ic+392
	uint32	field_18C;		//+018C g_ic+396
	uint32	anonymous_13;	//+0190 g_ic+400
	uint32	field_194;		//+0194 g_ic+404
	uint32	field_198;		//+0198 g_ic+408
	uint32	field_19C;		//+019C g_ic+412
	uint32	anonymous_14;	//+01A0 g_ic+416
	uint32	field_1A4;		//+01A4 g_ic+420
	uint32	field_1A8;		//+01A8 g_ic+424
	uint32	field_1AC;		//+01AC g_ic+428
	uint32	field_1B0;		//+01B0 g_ic+432
	uint32	field_1B4;		//+01B4 g_ic+436
	uint32	field_1B8;		//+01B8 g_ic+440
	uint32	field_1BC;		//+01BC g_ic+444
	uint32	field_1C0;		//+01C0 g_ic+448
	uint32	field_1C4;		//+01C4 g_ic+452
#if 0 // SDK < 1.1.0
	uint8	field_1C8[16];	//+01C8 g_ic+456
#else // SDK >= 1.1.0
	uint8	field_1C8[32];	//+01C8 g_ic+456
#endif
	struct s_wifi_store wifi_store;	//g_ic+488
}; // (sizeof 1368 (488+880)) // new 1368 SDK v1.1.0, sizeof 1352 = v1.0.1

typedef union _u_g_ic{
	struct s_g_ic g;
	uint8 c[1368];
	uint16 w[1368/2];
	uint32 d[1668/4];
}u_g_ic;

extern u_g_ic g_ic;

struct s_info {
	uint32 ap_ip;
	uint32 ap_mask;
	uint32 ap_gw;
	uint32 st_ip;
	uint32 st_mask;
	uint32 st_gw;
	uint8 ap_mac[6];
	uint8 st_mac[6];
} __attribute__((packed));

#endif /* _INCLUDE_LIBMAIN_H_ */
