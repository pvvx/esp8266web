/******************************************************************************
 * FileName: iram_info.h
 * Description: IRAM
 * Alternate SDK 
 * Author: PV`
 * (c) PV` 2015
*******************************************************************************/

#ifndef __EXRTA_RAM_H_
#define __EXRTA_RAM_H_

#define IRAM1_BASE	0x40100000
#define IRAM1_SIZE	0x00008000
#define MIN_GET_IRAM 2048 // минимальный размер iram, чтобы с ним возиться

typedef struct t_eraminfo // описание свободной области iram
{
//	bool	use; // true - есть минмум MIN_GET_IRAM байт
	uint32 *base;
	uint32 size;
}ERAMInfo;

//#define eram_base (*(volatile uint32*)(IRAM1_BASE + IRAM1_SIZE - 8))
//#define eram_size (*(volatile uint32*)(IRAM1_BASE + IRAM1_SIZE - 4))

void eram_init(void) ICACHE_FLASH_ATTR;
extern ERAMInfo eraminfo;

#endif /* __EXRTA_RAM_H_ */
