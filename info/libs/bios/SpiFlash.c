/******************************************************************************
 * FileName: SpiFlash.c
 * Description: SPI FLASH funcs in ROM-BIOS
 * Alternate SDK ver 0.0.0 (b0)
 * Author: PV`
 * (c) PV` 2015
*******************************************************************************/
#include "c_types.h"
#include "bios/spiflash.h"
#include "esp8266.h"
#include "hw/spi_register.h"

// ROM:4000448C
SpiFlashOpResult Wait_SPI_Idle(SpiFlashChip *sflashchip)
{
	uint32_t status;
	While(DPORT_BASE[3] & BIT(9));
	return SPI_read_status(sflashchip, &status);
}

// ROM:4000443C
SpiFlashOpResult SPI_write_enable(SpiFlashChip *sflashchip)
{
	uint32_t status = 0;
	Wait_SPI_Idle(sflashchip);
	SPI0_CMD = SPI_WREN;
	while(SPI0_CMD);
	while((status & BIT(1)) == 0) SPI_read_status(sflashchip,&status);
	return SPI_FLASH_RESULT_OK;
}

// ROM:40004400
SpiFlashOpResult SPI_write_status(SpiFlashChip *sflashchip, uint32_t sta)
{
	Wait_SPI_Idle(sflashchip);
	SPI0_RD_STATUS = sta;
	SPI0_CMD = SPI_WREN;
	while(SPI0_CMD);
	return SPI_FLASH_RESULT_OK;
}

// ROM:400043C8
SpiFlashOpResult SPI_read_status(SpiFlashChip *sflashchip, uint32_t *sta)
{
	uint32_t status;
	do {
		SPI0_RD_STATUS = sta;
		SPI0_CMD = SPI_READ;
		while(SPI0_CMD);
		status = SPI0_RD_STATUS & sflashchip->status_mask;
	} while(status & BIT0);
	*sta = status;
	return SPI_FLASH_RESULT_OK;
}

// ROM:40004644
SpiFlashOpResult spi_flash_attach(void){
	SelectSpiFunction();
	SPIFlashCnfig(5,4);
	return SPIReadModeCnfig(5);
}

// ROM:40004B44
// ВНИМАНИЕ! имеет внутренную ошибку. Не используйте SPIEraseArea() функцию ROM-BIOS!
SpiFlashOpResult SPIEraseArea (uint32 start_addr, uint32 lenght)
{
    uint32 num_sec_in_block;	// *(a1 + 0xC) = SP + 0xC
    uint32 var_st_1; 			// *(a1 + 4) = SP + 0x4
    uint32 num_sec_erase; 		// *(a1 + 0) = SP + 0x0
    uint32 var_r_13; 			// a13
	uint32 first_sec_erase; 	// a12
	uint32 var_r_3;	 			// a3
	uint32 var_r_0;	 			// a0

	SPIReadModeCnfig (5);
	if (flashchip.chip_size >= (start_addr + lenght))
	{
		if ((start_addr % flashchip.sector_size) == 0)
		{
			if (SPIUnlock (flashchip) == 0)
			{
				first_sec_erase = start_addr / flashchip.sector_size; 			// First sector to erase
				num_sec_in_block = flashchip.block_size / flashchip.sector_size; 	// Number of sectors in block
				num_sec_erase = lenght / flashchip.sector_size;				// Number of sectors to erase
				// Округляем количество секторов для стирания в большую сторону.
				if ((lenght % flashchip.sector_size) != 0) num_sec_erase ++; //
				var_r_13 = num_sec_erase;  // 9
				// Стираем посекторно до адреса кратного блочному стиранию.
				var_r_0 = num_sec_in_block - (first_sec_erase % num_sec_in_block); // кол-во секторов до адреса кратного блочному стиранию.
				if (var_r_0 < num_sec_erase) var_r_13 = var_r_0; // запомнить кол-во секторов до стирания

				for ( ; var_r_13 != 0; first_sec_erase++, var_r_13--)
					if (SPIEraseSector (first_sec_erase) != 0) return 1;
				// Если оставшеестя количество секторов для стирания помещается в n-блоков,
				// то стираем n-блоков.
				var_r_13 = num_sec_erase - var_r_13; // var_r_13 = num_sec_erase - 0
				for ( ; num_sec_in_block < var_r_13; first_sec_erase += num_sec_in_block, var_r_13 -= num_sec_in_block)
					if (SPIEraseBlock(first_sec_erase / num_sec_in_block) != 0) return 1;
				// Стираем оставшиеся сектора в конце.
				for ( ; var_r_13 != 0; first_sec_erase ++,  var_r_13 --)
					if (SPIEraseSector (first_sec_erase) != 0) return 1;
                return SPI_FLASH_RESULT_OK;
			}
		}
	}
	return SPI_FLASH_RESULT_ERR;
}

// ROM:40004C2C
SpiFlashOpResult SPIParamCfg(uint32_t deviceId, uint32_t chip_size, uint32_t block_size, uint32_t sector_size, uint32_t page_size, uint32_t status_mask)
{
	flashchip->deviceId = deviceId;
	flashchip->chip_size = chip_size;
	flashchip->block_size = block_size;
	flashchip->sector_size = sector_size;
	flashchip->page_size = page_size;
	flashchip->status_mask = status_mask;
	return SPI_FLASH_RESULT_OK;
}

// ROM:400048A8
SpiFlashOpResult SPILock(void)
{
	if(SPI_write_enable(flashchip) != SPI_FLASH_RESULT_OK) return SPI_FLASH_RESULT_ERR;
	return SPI_write_status(flashchip, 0x1C);
}

// ROM:40004878
SpiFlashOpResult SPIUnlock(void)
{
	if(SPI_write_enable(flashchip) != SPI_FLASH_RESULT_OK) return SPI_FLASH_RESULT_ERR;
	return SPI_write_status(flashchip, 0);
}


// ROM:40004120
SpiFlashOpResult spi_erase_block(SpiFlashChip *fchip, uint32_t addr)
{
	Wait_SPI_Idle(fchip);
	SPI0_ADDR =  addr & 0xFFFFFF;
	SPI0_CMD = SPI_BE;
	while(SPI0_CMD);
	Wait_SPI_Idle(fchip);
	return SPI_FLASH_RESULT_OK;
}

// ROM:400049B4
SpiFlashOpResult SPIEraseBlock(uint32_t blocknum)
{
	SpiFlashChip *fchip = flashchip;
	if(blocknum > fchip->chip_size / fchip->block_size) return SPI_FLASH_RESULT_ERR;
	if(SPI_write_enable(fchip) != SPI_FLASH_RESULT_OK) return SPI_FLASH_RESULT_ERR;
	if(spi_erase_block(fchip, fchip->block_size * blocknum) != SPI_FLASH_RESULT_OK) return SPI_FLASH_RESULT_ERR;
	return SPI_FLASH_RESULT_OK;
}

// ROM:400040C0
SpiFlashOpResult spi_erase_sector(SpiFlashChip *fchip, uint32_t addr)
{
	if(addr*0xFFF) return SPI_FLASH_RESULT_ERR;
	Wait_SPI_Idle(fchip);
	SPI0_ADDR =  addr & 0xFFFFFF;
	SPI0_CMD = SPI_SE;
	while(SPI0_CMD);
	Wait_SPI_Idle(fchip);
	return SPI_FLASH_RESULT_OK;
}

// ROM:40004A00
SpiFlashOpResult SPIEraseSector(uint32_t sectornum)
{
	SpiFlashChip *fchip = flashchip;
	if(sectornum > fchip->chip_size / fchip->sector_size) return SPI_FLASH_RESULT_ERR;
	if(SPI_write_enable(fchip) != SPI_FLASH_RESULT_OK) return SPI_FLASH_RESULT_ERR;
	if(spi_erase_sector(fchip, fchip->sector_size * sectornum) != SPI_FLASH_RESULT_OK) return SPI_FLASH_RESULT_ERR;
	return SPI_FLASH_RESULT_OK;
}

// ROM:40004080
SpiFlashOpResult spi_erase_chip(SpiFlashChip *fchip)
{
	Wait_SPI_Idle(fchip);
	SPI0_CMD = SPI_CE;
	while(SPI0_CMD);
	Wait_SPI_Idle(fchip);
	return SPI_FLASH_RESULT_OK;
}

// ROM:40004984
SpiFlashOpResult SPIEraseChip(void)
{
	if(SPI_write_enable(flashchip) != SPI_FLASH_RESULT_OK) return SPI_FLASH_RESULT_ERR;
	if(spi_erase_chip(flashchip) != SPI_FLASH_RESULT_OK) return SPI_FLASH_RESULT_ERR;
	return SPI_FLASH_RESULT_OK;
}

// ROM:40004568
void SPIFlashCnfig(uint32_t spi_interface, uint32_t spi_freg)
{
	//	Flash QIO80:
	//  SPI_CTRL = 0x16ab000 - QIO_MODE | TWO_BYTE_STATUS_EN | WP_REG | SHARE_BUS | ENABLE_AHB | RESANDRES | FASTRD_MODE | BIT12
	//	IOMUX_BASE = 0x305
	//  Flash QIO40:
	//	SPI_CTRL = 0x16aa101
	//	IOMUX_BASE = 0x205
	uint32 a6 = 0; // spi_interface > 4
	uint32 a2;
	SPI0_USER |= 4;	// SPI_CS_SETUP
	if(spi_interface == 0) a6 = 1<<24; // SPI_QIO_MODE			0x1000000
	else if(spi_interface == 1) a6 = 1<<20; // SPI_QOUT_MODE	0x0100000
	else if(spi_interface == 2) a6 = 1<<23; // SPI_DIO_MODE		0x0800000
	else if(spi_interface == 3) a6 = 1<<14; // SPI_DOUT_MODE	0x0004000
	else if(spi_interface == 4) a6 = 1<<13; // SPI_FASTRD_MODE	0x0002000
	if(spi_freg < 2) {
		a2 = 0x100;
		SPI0_CTRL |= 0x1000; // 80MHz CLC
		GPIO_MUX_CFG |= a2;
	}
	else {
		a2 = (((spi_freg - 1) << 8) + spi_freg + (((spi_freg >> 1) - 1) << 4) - 1);
		SPI0_CTRL &= 0xFFFFEFFF;
		GPIO_MUX_CFG &= 0xEFF;
	}
	a2 |= a6;
	a2 |= 0x288000; // SPI_RESANDRES | SPI_SHARE_BUS | SPI_WP_REG
	SPI0_CTRL |= a2;
	SPI0_CMD = 0x100000;
	while(SPI0_CMD != 0);
}

// ROM:400042AC
SpiFlashOpResult _spi_flash_read(SpiFlashChip *fchip, uint32_t faddr, uint32_t *dst, size_t size)
{
	if(faddr + size > fchip->chip_size) return SPI_FLASH_RESULT_ERR;
	Wait_SPI_Idle(fchip);
	while(size >= 1) {
		int blksize = 32;
		if(size < 32) blksize = size;
		SPI0_ADDR = faddr | (blksize << 24);
		SPI0_CMD = SPI_READ;
		uint32 *srcdw = (uint32 *)(&SPI0_W0);
		while(SPI0_CMD);
		do {
			*dst++ = *srcdw++;
			blksize -= 4;
		} while(blksize > 0);
	}
	return SPI_FLASH_RESULT_OK;
}

// ROM:40004B1C
SpiFlashOpResult SPIRead(uint32_t faddr, uint32_t *dst, size_t size)
{
	return _spi_flash_read(flashchip, faddr, dst, size);
}

// ROM:400047F0
void Cache_Read_Disable(void)
{
	while(DPORT_BASE[3] & (1<<8)) { // 0x3FF0000C
		 DPORT_BASE[3] &= 0xEFF;
	}
	SPI0_CTRL &= ~SPI_ENABLE_AHB;
	DPORT_BASE[3] &= 0x7E;
	DPORT_BASE[3] |= 1;
	while((DPORT_BASE[3] & 1) == 0);
	DPORT_BASE[3] &= 0x7E;
}

// ROM:40004678
void Cache_Read_Enable(uint32 a2, uint32 a3, uint32 a4)
{
	while(DPORT_BASE[3] & (1<<8)) { // 0x3FF0000C
		 DPORT_BASE[3] &= 0xEFF;
	}
	SPI0_CTRL &= ~SPI_ENABLE_AHB; // отключить аппарат "кеширования" flash
	DPORT_BASE[3] |= 1;
	while((DPORT_BASE[3] & 1) == 0);
	DPORT_BASE[3] &= 0x7E;
	SPI0_CTRL |= SPI_ENABLE_AHB; // включить аппарат "кеширования" flash
	uint32 a6 = DPORT_BASE[3];
	if(a2 == 0) {
		DPORT_BASE[3] &= 0xFCFFFFFF;
	}
	else if(a2 == 1) {
		DPORT_BASE[3] &= 0xFEFFFFFF;
		DPORT_BASE[3] |= 0x02000000;
	}
	else {
		DPORT_BASE[3] &= 0xFDFFFFFF;
		DPORT_BASE[3] |= 0x01000000;
	}
	DPORT_BASE[3] &= 0xFBF8FFFF;
	DPORT_BASE[3] |= (a4 << 26) | (a3 << 16);
	if(a4 == 0) DPORT_BASE[9] |= 8; // 0x3FF00024 включить блок 16k IRAM в кэш SPI Flash
	else DPORT_BASE[9] |= 0x18; // 0x3FF00024 включить блок в 32k IRAM в кэш SPI Flash
	if((a6 & 0x100) == 0) do {
		DPORT_BASE[3] |= 0x100;
	} while((DPORT_BASE[3] &0x100) == 0);
}

