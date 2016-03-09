/******************************************************************************
 * FileName: spiflash_bios.h
 * Description: SPI in ROM-BIOS
 * Alternate SDK
 * Author: PV`
 * (c) PV` 2015
*******************************************************************************/

#ifndef _INCLUDE_BIOS_SPI_H_
#define _INCLUDE_BIOS_SPI_H_

#include "c_types.h"

struct SPIFlashHsz {
	uint8 spi_freg: 4; // Low four bits: 0 = 40MHz, 1= 26MHz, 2 = 20MHz, 0x3 = 80MHz
	uint8 flash_size: 4; // High four bits: 0 = 512K, 1 = 256K, 2 = 1M, 3 = 2M, 4 = 4M,
}__attribute__((packed));

struct SPIFlashHead { // заголовок flash (использует загрузчик BIOS)
	uint8 id; // = 0xE9
	uint8 number_segs; // Number of segments
	uint8 spi_interface; // SPI Flash Interface (0 = QIO, 1 = QOUT, 2 = DIO, 0x3 = DOUT)
	struct SPIFlashHsz hsz; // options
}  __attribute__((packed));

#define LOADER_HEAD_ID 0xE9

struct SPIFlashHeadSegment {
	uint32 memory_offset; // Memory offset
	uint32 segment_size; // Segment size
};

struct SPIFlashHeader { // полный заголовок flash (использует загрузчик BIOS)
	struct SPIFlashHead head;
	uint32 entry_point; // Entry point
}  __attribute__((packed));

typedef struct{
	uint32_t	deviceId;		//+00
	uint32_t	chip_size;    	//+04 chip size in byte
	uint32_t	block_size;		//+08
	uint32_t	sector_size;	//+0c
	uint32_t	page_size;		//+10
	uint32_t	status_mask;	//+14
} SpiFlashChip;

typedef enum {
    SPI_FLASH_RESULT_OK,
    SPI_FLASH_RESULT_ERR,
    SPI_FLASH_RESULT_TIMEOUT
} SpiFlashOpResult;

extern SpiFlashChip * flashchip; // in RAM-BIOS: 0x3fffc714

void Cache_Read_Disable(void);
void Cache_Read_Enable(uint32_t a, uint32_t b, uint32_t c);

SpiFlashOpResult SPI_read_status(SpiFlashChip *sflashchip, uint32_t *sta);
SpiFlashOpResult SPI_write_status(SpiFlashChip *sflashchip, uint32_t sta);
SpiFlashOpResult SPI_write_enable(SpiFlashChip *sflashchip);

SpiFlashOpResult Wait_SPI_Idle(SpiFlashChip *sflashchip);

SpiFlashOpResult SPIEraseArea(uint32_t start_addr, size_t lenght); // ВНИМАНИЕ! имеет внутренную ошибку. Не используйте эту функцию ROM-BIOS!
SpiFlashOpResult SPIEraseBlock(uint32_t blocknum);
SpiFlashOpResult SPIEraseSector(uint32_t sectornum);
SpiFlashOpResult SPIEraseChip(void);
SpiFlashOpResult SPILock(void);
SpiFlashOpResult SPIUnlock(void);

SpiFlashOpResult SPIRead(uint32_t faddr, uint32_t *dst, size_t size);
int SPIReadModeCnfig(uint32_t mode);
void SPIFlashCnfig(uint32_t spi_interface, uint32_t spi_freg);
SpiFlashOpResult SPIWrite(uint32_t faddr, const uint32_t *src, size_t size);
SpiFlashOpResult SPIParamCfg(uint32_t deviceId, uint32_t chip_size, uint32_t block_size, uint32_t sector_size, uint32_t page_size, uint32_t status_mask); // Set flashchip
SpiFlashOpResult spi_flash_attach(void); // SelectSpiFunction; SPIFlashCnfig(5,4); SPIReadModeCnfig(5);
SpiFlashOpResult SelectSpiFunction(void); // GPIO7..11 fun = QSPI, *0x60000D48 = 0, return 0

/* in eagle.rom.addr.v6.ld :
PROVIDE ( Cache_Read_Disable = 0x400047f0 );
PROVIDE ( Cache_Read_Enable = 0x40004678 );

PROVIDE ( SPI_read_status = 0x400043c8 );
PROVIDE ( SPI_write_status = 0x40004400 );
PROVIDE ( SPI_write_enable = 0x4000443c );

PROVIDE ( Wait_SPI_Idle = 0x4000448c );
PROVIDE ( SPIEraseArea = 0x40004b44 );
PROVIDE ( SPIEraseBlock = 0x400049b4 );
PROVIDE ( SPIEraseChip = 0x40004984 );
PROVIDE ( SPIEraseSector = 0x40004a00 );
PROVIDE ( SPILock = 0x400048a8 );
PROVIDE ( SPIUnlock = 0x40004878 );
PROVIDE ( SPIParamCfg = 0x40004c2c );
PROVIDE ( SPIRead = 0x40004b1c );
PROVIDE ( SPIReadModeCnfig = 0x400048ec );
PROVIDE ( SPIWrite = 0x40004a4c );

PROVIDE ( SelectSpiFunction = 0x40003f58 );
PROVIDE ( spi_flash_attach = 0x40004644 );

+PROVIDE ( SPIFlashCnfig = 0x40004568);

PROVIDE ( flashchip = 0x3fffc714);
*/

#endif /* _INCLUDE_BIOS_SPI_H_ */
