/*
 * flash_header.c
 *
 *  Created on: 18/01/2015
 *      Author: PV`
 */
#include "user_config.h"
#include "bios.h"
#include "add_sdk_func.h"
#include "ets_sys.h"
#include "os_type.h"
#include "osapi.h"
#include "user_interface.h"
#include "flash.h"
#include "bios/spiflash.h"
#include "iram_info.h"

#if DEBUGSOO > 0
const char *txtSPIFlashInterface[] = {"QIO","QOUT","DIO","DOUT"};
const char *txtFSize[] = {"512K", "256K", "1M", "2M", "4M"};
const char *txtSFreq[] = {"40MHz","26MHz","20MHz","80MHz"};
#endif

#define ICACHE_RODATA_ATTR __attribute__((section(".irom.text")))

#define flash_read spi_flash_read

ERAMInfo eraminfo;

bool ICACHE_FLASH_ATTR get_eram_size(ERAMInfo *einfo) {
	struct SPIFlashHeader x;
	int i = 1;
	uint32 faddr = 0x000;
//	einfo->use = false;
	einfo->base = NULL;
	einfo->size = 0;
	if (flash_read(faddr, (uint32 *)&x, 8) != SPI_FLASH_RESULT_OK)
		return false;
	faddr += 8;
#if DEBUGSOO > 0
	os_printf("Flash Header:\n");
#endif
	if (x.head.id != 0xE9) {
#if DEBUGSOO > 0
		os_printf(" Bad Header!\n");
#endif
		return false;
	} else {
#if DEBUGSOO > 0
/*		uint32 speed = x.head.hsz.spi_freg;
		if(speed > 2) {
			if(speed == 15) speed = 3;
			else speed = 0;
		} */
		os_printf(
				" Number of segments: %u\n SPI Flash Interface: %s\n SPI CLK: %s\n Flash size: %s\n Entry point: %p\n",
				x.head.number_segs, txtSPIFlashInterface[x.head.spi_interface & 3], txtSFreq[x.head.hsz.spi_freg & 3],
				txtFSize[x.head.hsz.flash_size&3], x.entry_point);
#endif
		while (x.head.number_segs) {
			if (flash_read(faddr, (uint32 *)&x.seg, 8) != SPI_FLASH_RESULT_OK) {
#if DEBUGSOO > 0
				os_printf("flash read error!");
#endif
				return false;
			};
			if ((x.seg.memory_offset >= IRAM1_BASE)
					&& (x.seg.memory_offset < (IRAM1_BASE + IRAM1_SIZE))) {
				if (((x.seg.memory_offset + x.seg.segment_size) > (uint32)einfo->base)
						&& ((x.seg.memory_offset + x.seg.segment_size)
								< (IRAM1_BASE + IRAM1_SIZE))) {
					einfo->size = IRAM1_SIZE - x.seg.segment_size;
					einfo->base = (uint32 *)(x.seg.memory_offset + x.seg.segment_size);
				};
			};
#if DEBUGSOO > 0
			os_printf(" Segment %u: offset: %p, size: %u\n", i, x.seg.memory_offset,
					x.seg.segment_size);
#endif
			x.head.number_segs--;
			i++;
			faddr += x.seg.segment_size + 8;
		};
	};
#if DEBUGSOO > 0
	os_printf("Real Flash size: %u bytes\n", spi_flash_real_size());
#endif
	if((eraminfo.base)&&(eraminfo.size > MIN_GET_IRAM)) return true;
	return false;
}

void ICACHE_FLASH_ATTR eram_init(void) {
	if(get_eram_size(&eraminfo)) {
#if DEBUGSOO > 0
		os_printf("Found free IRAM: base:%p, size:%u bytes\n", eraminfo.base,  eraminfo.size);
#endif
		os_memset(eraminfo.base, 0, eraminfo.size);
	}
}
