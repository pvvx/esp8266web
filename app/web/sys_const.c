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
#include "sdk/rom2ram.h"
#include "sdk/flash.h"
#include "sys_const_utils.h"
#include "sdk/mem_manager.h"

extern SpiFlashChip * flashchip;

uint8 * ICACHE_FLASH_ATTR rd_buf_sec_blk(uint32 faddr, uint32 size)
{
	uint8 * pbuf = NULL;
	pbuf = os_malloc(size);
#ifdef USE_FIX_SDK_FLASH_SIZE
	if(pbuf != NULL) copy_s4d1(pbuf, (void *)(faddr + FLASH_BASE), size );
#else
	if(pbuf != NULL) {
		if(spi_flash_read(faddr, pbuf, size)) != SPI_FLASH_RESULT_OK) {
			os_free(pbuf);
			pbuf = NULL;
		}
	}
#endif
	return pbuf;
}

bool ICACHE_FLASH_ATTR wr_buf_sec_blk(uint32 faddr, uint32 size, uint8 *buf)
{
	bool ret = false;
	if((spi_flash_erase_sector((uint32)(faddr >> 12)) == SPI_FLASH_RESULT_OK)
	&&(spi_flash_write(faddr, (uint32 *)buf, size) == SPI_FLASH_RESULT_OK)) {
		ret = true;
	}
	os_free(buf);
	return ret;
}

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
	uint32 faddr = esp_init_data_default_addr;
	if (get_sys_const(idx) != data) {
		uint8 * buf = rd_buf_sec_blk(faddr, SIZE_SAVE_SYS_CONST);
		if(buf != NULL) {
			buf[idx] = data;
			return wr_buf_sec_blk(faddr, SIZE_SAVE_SYS_CONST, buf);
		}
	}
	else return true;
	return false;
}
