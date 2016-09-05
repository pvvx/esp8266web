/******************************************************************************
 * FileName: spi_flash.h
 * Description: FLASH
 * Alternate SDK
 * Author: PV`
 * (c) PV` 2015
*******************************************************************************/

#ifndef SPI_FLASH_H
#define SPI_FLASH_H

#include "sdk_config.h"
#include "hw/esp8266.h"
#include "bios/spiflash.h"

#define SPI_FLASH_SEC_SIZE      (4*1024)

#ifdef FIX_SDK_FLASH_SIZE	// использовать 'песочницу' для SDK (262144..FLASH_MAX_SIZE, шаг SPI_FLASH_SEC_SIZE)

#if FIX_SDK_FLASH_SIZE > FLASH_CACHE_MAX_SIZE
#error 'FIX_SDK_FLASH_SIZE > FLASH_CACHE_MAX_SIZE !'
#endif

#define flashchip_sector_size   SPI_FLASH_SEC_SIZE
#define sdk_flashchip_size 		FIX_SDK_FLASH_SIZE // размер 'песочницы' для SDK (стандарт = 512 килобайт)

#define open_16m() 			flashchip->chip_size = FLASH_MAX_SIZE
#define close_16m() 		flashchip->chip_size = FIX_SDK_FLASH_SIZE

#else

#define flashchip_sector_size 	flashchip->sector_size
#define sdk_flashchip_size		flashchip->chip_size

#define open_16m()
#define close_16m()

#endif

#define fsec_esp_init_data_default (sdk_flashchip_size / flashchip_sector_size - 4)
#define faddr_esp_init_data_default (sdk_flashchip_size - 4 * flashchip_sector_size)
#define fsec_sdk_wifi_cfg (sdk_flashchip_size / flashchip_sector_size - 3)
#define fsec_sdk_wifi_cfg_head (sdk_flashchip_size / flashchip_sector_size - 1)
#define faddr_sdk_wifi_cfg (sdk_flashchip_size - 3 * flashchip_sector_size)
#define faddr_sdk_wifi_cfg_head (sdk_flashchip_size - 1 * flashchip_sector_size)

#if DEF_SDK_VERSION >= 2000
#define fsec_rf_cal 	(sdk_flashchip_size / flashchip_sector_size - 5)
#define faddr_rf_cal 	(sdk_flashchip_size - 5 * flashchip_sector_size)
#define SDK_CFG_FLASH_SEC 5	// SDK с v2.0.0 использует 5 последних секторов во Flash для сохранения установок
#else
#define SDK_CFG_FLASH_SEC 4 // SDK до v2.0.0 использует 4 последних сектора во Flash для сохранения установок
#endif


uint32 spi_flash_get_id(void);
SpiFlashOpResult spi_flash_read_status(uint32_t * sta);
SpiFlashOpResult spi_flash_write_status(uint32_t sta);
SpiFlashOpResult spi_flash_erase_sector(uint16 sec);
SpiFlashOpResult spi_flash_write(uint32 faddr, uint32 *src_addr, uint32 size);
SpiFlashOpResult spi_flash_read(uint32 faddr, void *des, uint32 size);
SpiFlashOpResult spi_flash_erase_block(uint32 blk);

uint32 spi_flash_real_size(void) ICACHE_FLASH_ATTR; // реальный размер Flash

#ifdef USE_OVERLAP_MODE

typedef SpiFlashOpResult (* user_spi_flash_read)(
		SpiFlashChip *spi,
		uint32 src_addr,
		uint32 *des_addr,
        uint32 size);

extern user_spi_flash_read flash_read;

void spi_flash_set_read_func(user_spi_flash_read read);

#endif

#define spi_flash_read_byte(faddr, dest) spi_flash_read(faddr, dest, 1);

#endif
