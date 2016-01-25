/******************************************************************************
 * FileName: boot_1_2.c
 * Description: Reverse (disasm) boot_v1.2.bin SDK 1.5.1
 * Author: PV`
*******************************************************************************/
#include "bios.h"
#include "hw/esp8266.h"

enum
{
SPEED_40MHZ = 0,
SPEED_26MHZ = 1,
SPEED_20MHZ = 2,
SPEED_80MHZ = 15,
} SPEED_MHZS;

enum
{
MODE_QIO = 0,
MODE_QOUT = 1,
MODE_DIO = 2,
MODE_DOUT = 15,
} MODE_SPI;

enum
{
SIZE_4MBIT = 0,
SIZE_2MBIT = 1,
SIZE_8MBIT = 2,
SIZE_16MBIT = 3,
SIZE_32MBIT = 4,
} SIZE_BITS;

#define FSECTOR_SIZE 0x1000

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

struct BootConfig {
    uint8 boot_number;
    uint8 boot_version;
    uint8 none[6];
}__attribute__((packed));

struct StoreWifiHdr {
	uint8 bank; //  = 0, 1 // WiFi config flash addr: 0 - flashchip->chip_size-0x3000 (0x7D000), 1 - flashchip->chip_size-0x2000
    uint8 none[3];
}__attribute__((packed));

//=============================================================================
// get_seg_size
//-----------------------------------------------------------------------------
uint32 get_seg_size(uint32_t faddr)
{
	struct SPIFlashHeader sffh;
	SPIRead(faddr, &sffh, sizeof(sffh));
	if(sffh.head.id == 0xe9) return 0;
	if(sffh.head.id != 0xea
	|| sffh.head.number_segs != 4) { // Number of segments
		ets_printf("get flash_addr error!\n");
		return 0xffffffff;
	}
	return sffh.seg.segment_size;
}
//=============================================================================
// call_user_start
//-----------------------------------------------------------------------------
void call_user_start(void)
{
	struct SPIFlashHead sfh; // заголовок flash
	struct StoreWifiHdr wifihdr; // заголовок из последнего сектора flash с индексом на сохранение последней конфигурации WiFi
    struct BootConfig bootcfg;	// начало блока сохранения последней конфигурации WiFi, часть с boot параметрами

	ets_printf("\n2nd boot version : 1.2\n");
	SPIRead(0, &sfh, sizeof(sfh));
	ets_printf("  SPI Speed      : ");
    switch (sfh.hsz.spi_freg)
	{
	    case SPEED_40MHZ:
	    	ets_printf("40MHz\n");
	    	break;
	    case SPEED_26MHZ:
	    	ets_printf("26.7MHz\n");
	    	break;
	    case SPEED_20MHZ:
	    	ets_printf("20MHz\n");
	    	break;
	    case SPEED_80MHZ:
	    	ets_printf("80MHz\n");
	    	break;
	}
	ets_printf("  SPI Mode       : ");
	switch (sfh.spi_interface)
    {
	    case MODE_QIO:
	    	ets_printf("QIO\n");
	    	break;
	    case MODE_QOUT:
	    	ets_printf("QOUT\n");
	    	break;
	    case MODE_DIO:
	    	ets_printf("DIO\n");
	    	break;
	    case MODE_DOUT:
	    	ets_printf("DOUT\n");
	    	break;
    }
	ets_printf("  SPI Flash Size : ");
	uint32 sector;
	switch (sfh.hsz.flash_size)
	{
		case SIZE_4MBIT:
			ets_printf("4Mbit\n");
			sector = 128-4;
			break;
	    case SIZE_2MBIT:
	    	ets_printf("2Mbit\n");
	    	sector = 64-4;
	    	break;
	    case SIZE_8MBIT:
	    	ets_printf("8Mbit\n");
	    	sector = 256-4;
	    	break;
	    case SIZE_16MBIT:
	    	ets_printf("16Mbit\n");
	    	sector = 512-4;
	    	break;
	    case SIZE_32MBIT:
	    	ets_printf("32Mbit\n");
	    	sector = 1024-4;
	    	break;
	    default:
			ets_printf("4Mbit\n");
			sector = 128-4;
			break;
	}
	uint32 addr = sector * FSECTOR_SIZE;
	SPIRead(addr + 3 * FSECTOR_SIZE, &wifihdr.bank, sizeof(wifihdr));
	if(wifihdr.bank == 0) {
		SPIRead(addr + FSECTOR_SIZE, &bootcfg, sizeof(bootcfg));
	}
	else {
		SPIRead(addr + 2 * FSECTOR_SIZE, &bootcfg, sizeof(bootcfg));
	}
	if(bootcfg.boot_version == 0xff) {
		bootcfg.boot_number = 0;
	}
	if(bootcfg.boot_version != 2) {
			bootcfg.boot_version = 2;
			if(wifihdr.bank == 0) wifihdr.bank = 1;
			else wifihdr.bank = 0;
			SPIEraseSector(sector+wifihdr.bank+1);
			SPIWrite((sector+wifihdr.bank+1) * FSECTOR_SIZE, &bootcfg, sizeof(bootcfg));
			SPIEraseSector(sector+3);
			SPIWrite(addr + 3 * FSECTOR_SIZE, &wifihdr.bank, sizeof(wifihdr));
	}
	ets_memcpy((void *)0x4010800, &code_blk, size_code_blk); // загрузчик не прикреплен!
	ets_printf("jump to run user");
	switch(bootcfg.boot_number & 0x0f)
	{
		case 0:
			ets_printf("1\n\n");
			uint32 seg_size = get_seg_size(FSECTOR_SIZE);
			if(seg_size == 0xffffffff) return;
			if(seg_size == 0) {
				0x4010800C(FSECTOR_SIZE);
			} else {
				0x4010800C(seg_size + 0x1010);
			}
			break;
		case 1:
			ets_printf("2\n\n");
			if(sector == 512 - 4 || sector == 1024 - 4) sector = 256 - 4;
			get_seg_size(((sector + 4)>>1)*FSECTOR_SIZE + FSECTOR_SIZE);
			if(seg_size == 0xffffffff) return;
			if(seg_size == 0) {
				0x4010800C(FSECTOR_SIZE);
			} else {
				0x4010800C(seg_size + 0x1010);
			}
			break;
		default:
			ets_printf("error user bin flag, flag = %x\n", bootcfg.boot_number & 0x0f);
	}
}

void x0x4010800C(uint32 x)
{
	// загрузчик - не интересен
}
