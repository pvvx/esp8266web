/******************************************************************************
 * FileName: app_main.c
 * Description: Alternate SDK (libmain.a)
 * (c) PV` 2015
 * disasm SDK 1.2.0
*******************************************************************************/
#include "user_config.h"

#ifdef USE_OVERLAP_MODE
// не используется для модулей с одной flash!
#include "bios.h"
#include "hw/esp8266.h"
#include "hw/spi_register.h"
#include "flash.h"

user_spi_flash_read flash_read DATA_IRAM_ATTR;
//=============================================================================
// overlap_hspi_read_data()
//-----------------------------------------------------------------------------
void spi_flash_set_read_func(user_spi_flash_read read)
{
	flash_read = read;
}
//=============================================================================
// overlap_hspi_init()
//-----------------------------------------------------------------------------
void overlap_hspi_init(void)
{
	while((SPI0_EXT2) || (SPI1_CMD & 0xFFFC0000)); // 0x600001F8 || 0x60000100
	PERI_IO_SWAP |= 0x80; // two spi masters on cspi
	SPI0_EXT3 |= 1;
	SPI1_EXT3 |= SPI_INT_HOLD_ENA;
	SPI1_USER |= SPI_CS_SETUP;
	SPI1_PIN = (SPI1_PIN & 0xfffffffe) | (SPI_CS2_DIS | SPI_CS1_DIS); // SPI_CS0_ENA
//	SPI1_CLOCK &= ~SPI_CLK_EQU_SYSCLK; // deleted in SDKv1.1.0 libmain_patch_01.a
}
//=============================================================================
// overlap_hspi_deinit()
//-----------------------------------------------------------------------------
void overlap_hspi_deinit(void)
{
	while((SPI0_EXT2) || (SPI1_CMD & 0xFFFC0000)); // 0x600001F8 || 0x60000100
	PERI_IO_SWAP &= 0xFFFFFF7F; // two spi masters on cspi
	SPI0_EXT3 &= 0xFFFFFFFE;
	SPI1_EXT3 &= 0xFFFFFFFC;
	SPI1_USER &= 0xFFFFFFDF;
	SPI1_PIN = (SPI1_PIN & 0xfffffffe) | (SPI_CS2_DIS | SPI_CS1_DIS);
//	SPI1_CLOCK &= ~SPI_CLK_EQU_SYSCLK; // deleted in SDKv1.1.0 libmain_patch_01.a
}
//=============================================================================
// overlap_hspi_read_data()
//-----------------------------------------------------------------------------
#define SPI_FBLK 32
int overlap_hspi_read_data(SpiFlashChip *fchip, uint32 faddr, void *des, uint32 size)
{
	SPI1_PIN = (SPI1_PIN & 0x7E) |  (SPI_CS2_DIS | SPI_CS1_DIS);
	if(fchip->chip_size < faddr + size) return SPI_FLASH_RESULT_OK;
	if(des == NULL) return SPI_FLASH_RESULT_ERR;
	if(size < 1) return SPI_FLASH_RESULT_ERR;
	uint32 blksize = (uint32)des & 3;
	if(blksize) {
		blksize = 4 - blksize;
		if(size < blksize) blksize = size;
		SPI1_ADDR = faddr | (blksize << 24);
		SPI1_CMD = SPI_READ;
		size -= blksize;
		faddr += blksize;
		while(SPI1_CMD);
		register uint32 data_buf = SPI1_W0;
		do {
			*(uint8 *)des = data_buf;
			des = (uint8 *)des + 1;
			data_buf >>= 8;
		} while(--blksize);
	}
	while(size) {
		if(size < SPI_FBLK) blksize = size;
		else blksize = SPI_FBLK;
		SPI1_ADDR = faddr | (blksize << 24);
		SPI1_CMD = SPI_READ;
		size -= blksize;
		faddr += blksize;
		while(SPI1_CMD);
		// volatile uint32 *srcdw = (volatile uint32 *)(SPI0_BASE+0x40);
		uint32 *srcdw = (uint32 *)(&SPI1_W0);
//			uint32 *srcdw = (uint32 *)(SPI0_BASE+0x40);
		while(blksize >> 2) {
			*((uint32 *)des) = *srcdw++;
			des = ((uint32 *)des) + 1;
			blksize -= 4;
		}
		if(blksize) {
			uint32 data_buf = *srcdw;
			do {
				*(uint8 *)des = data_buf;
				des = (uint8 *)des + 1;
				data_buf >>= 8;
			} while(--blksize);
			break;
		}
	}
	return SPI_FLASH_RESULT_OK;
}


#endif
