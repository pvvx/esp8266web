/******************************************************************************
 * FileName: mem.h
 * Description: mem funcs in ROM-BIOS
 * Alternate SDK
 * Author: PV`
 * (c) PV` 2015
*******************************************************************************/
#ifndef _BIOS_MEM_H_
#define _BIOS_MEM_H_

#include "c_types.h"
/* WARNING !!! mem_xxx use size < 4096 !!! */
extern void mem_init(void * start_addr); // uint8 *
extern void * mem_malloc(uint16 size); // size < 4096
extern void * mem_calloc(uint16 n, uint16 count); // n*count < 4096
extern void * mem_zalloc(uint16 size); // size < 4096, = mem_calloc(1, size);
extern void * mem_realloc(void * p, uint16 size);
extern void * mem_trim(void * p, uint16 size);
extern void mem_free(uint8 * x);

/*
PROVIDE ( mem_calloc = 0x40001c2c );
PROVIDE ( mem_free = 0x400019e0 );
PROVIDE ( mem_init = 0x40001998 );
PROVIDE ( mem_malloc = 0x40001b40 );
PROVIDE ( mem_realloc = 0x40001c6c );
PROVIDE ( mem_trim = 0x40001a14 );
PROVIDE ( mem_zalloc = 0x40001c58 );
*/
#endif /* _BIOS_MEM_H_ */
