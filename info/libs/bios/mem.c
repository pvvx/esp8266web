/******************************************************************************
 * FileName: mem.c
 * Description: mem funcs in ROM-BIOS
 * Alternate SDK ver 0.0.0 (b0)
 * Author: PV`
 * (c) PV` 2015
*******************************************************************************/
#include "c_types.h"
#include "esp8266.h"

struct _smem_ { // RAM_BIOS:3FFFDD30
	uint8 * x0;
	uint8 * x1;
	uint8 * x2;
}_mem_;

void mem_init(uint8 *a2)
{
	uint8 * a4 = (a2 + 3) & 0xFFFFFFFC;
	a4[1] = 16;
	a4[2] = 0;
	a4[3] = 0;
	a4[4] = 0;
	uint8 * a7 = a4 + 0x1000;
	_mem_.x0 = a4;
	_mem_.x1 = a7;
	_mem_.x2 = a4;
	a4 += 0xf84;
	a4[124] = (uint8)a7;
	a4[125] = 16;
	a4[126] = (uint8)a7;
	a4[127] = 16;
	a4[128] = 1;
}
