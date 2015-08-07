 /******************************************************************************
 * FileName: sys_const.c
 * Description: read/write sdk_sys_cont (esp_init_data_default.bin)
 * Author: PV`
 * ver1.1 02/04/2015  SDK 1.0.1
 *******************************************************************************/
#include "user_config.h"
#include "ets_sys.h"
#include "os_type.h"
#include "osapi.h"
#include "sdk/flash.h"
#include "sys_const_utils.h"

extern SpiFlashChip * flashchip;

uint8 ICACHE_FLASH_ATTR read_sys_const(uint8 idx) {
#ifdef USE_FIX_SDK_FLASH_SIZE	
	return get_sys_const(idx);
#else	
	uint8 ret = 0xff;
	if (idx < MAX_IDX_SYS_CONST)
		spi_flash_read(esp_init_data_default_addr + idx, (uint32 *)&ret, 1);
	return ret;
#endif	
}

bool ICACHE_FLASH_ATTR write_sys_const(uint8 idx, uint8 data) {
	if (idx >= MAX_IDX_SYS_CONST)
		return false;
	uint8 buf[SIZE_USYS_CONST];
	uint32 faddr = esp_init_data_default_addr;
//	os_memset(buf, 0xff, sizeof(buf));
	if ((spi_flash_read(faddr, (uint32 *)buf, sizeof(buf)) != SPI_FLASH_RESULT_OK)
		&& (spi_flash_read(faddr, (uint32 *)buf, sizeof(buf)) != SPI_FLASH_RESULT_OK))
		return false;
	if (buf[idx] == data)
		return true;
	if ((buf[idx] & data) != data) {
		spi_flash_erase_sector((uint32)(faddr >> 12));
	}
	buf[idx] = data;
	if (spi_flash_write(faddr, (uint32 *)buf, sizeof(buf)) != SPI_FLASH_RESULT_OK)
		return false;
	return true;
}

uint32 ICACHE_FLASH_ATTR read_user_const(uint8 idx) {
#ifdef USE_FIX_SDK_FLASH_SIZE	
	return get_user_const(idx);
#else
	uint32 ret = ~0;
	if (idx < MAX_IDX_USER_CONST)
		spi_flash_read(esp_init_data_default_addr + MAX_IDX_SYS_CONST + (idx<<2), (uint32 *)&ret, 4);
	return ret;
#endif	
}

bool ICACHE_FLASH_ATTR write_user_const(uint8 idx, uint32 data) {
	if (idx >= MAX_IDX_USER_CONST)
		return false;
	uint8 buf[SIZE_USYS_CONST];
	uint32 faddr = esp_init_data_default_addr;
//	os_memset(buf, 0xff, sizeof(buf));
	if ((spi_flash_read(faddr, (uint32 *)buf, sizeof(buf)) != SPI_FLASH_RESULT_OK)
		&& (spi_flash_read(faddr, (uint32 *)buf, sizeof(buf)) != SPI_FLASH_RESULT_OK))
		return false;
	uint32 *ptr = (uint32 *)((uint8 *)&buf[MAX_IDX_SYS_CONST]);
	if (ptr[idx] == data)
		return true;
	if ((ptr[idx] & data) != data) {
		spi_flash_erase_sector((uint32)(faddr >> 12));
	}
	ptr[idx] = data;
	if (spi_flash_write(faddr, (uint32 *)buf, sizeof(buf)) != SPI_FLASH_RESULT_OK)
		return false;
	return true;
}

