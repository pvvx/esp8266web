/*
 * China_bags_Espressif_SDK.c
 *
 */
#include "user_config.h"
#include "bios.h"
#include "user_interface.h"
#include "hw/esp8266.h"

void ICACHE_FLASH_ATTR system_phy_set_rfoption(uint8 option)
{
	phy_afterwake_set_rfoption(option);
}

void ICACHE_FLASH_ATTR phy_afterwake_set_rfoption(uint8 option)
{
	uint32 x = (RTC_RAM_BASE[0x60>>2] & 0xFFFF) | (option << 16);
	RTC_RAM_BASE[0x60>>2] = x; // 0x60001060
	RTC_RAM_BASE[0x78>>2] |= x; // 0x60001078 - это ячейка памяти RTC c китай-контролькой области от 0x60001000 до 0x60001078, получаемая путем OR всех слов этой области :)
	// но сколько не OR-ь все равно китай-програмер получит шиш в виде несовпадения, т.к.... :)
}

bool ICACHE_FLASH_ATTR system_deep_sleep_set_option(uint8 option)
{
	uint32 x = (RTC_RAM_BASE[0x60>>2] & 0xFFFF) | (option << 16);
	RTC_RAM_BASE[0x60>>2] = x; // 0x60001060
	rtc_mem_check(false); // пересчитать OR
	// return всегда fasle
}
/* CRC (c) ESPRSSIF :)
ESPRSSIF MIT License

Copyright (c) 2015 <ESPRESSIF SYSTEMS (SHANGHAI) PTE LTD>

Permission is hereby granted for use on ESPRESSIF SYSTEMS ESP8266 only, in which case, it is free of charge, to any person obtaining a copy of this software and associated documentation files (the Software), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED AS IS, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE. */
bool ICACHE_FLASH_ATTR rtc_mem_check(bool flg)
{
	volatile uint32 * ptr = &RTC_RAM_BASE[0];
	uint32 region_or_crc = 0;
	while(ptr != &RTC_RAM_BASE[0x78>>2]) region_or_crc |= *ptr++; // китай-контролька OR-ом! :)
	if(flg == false) {
		*ptr = region_or_crc; // RTC_RAM_BASE[0x78>>2] = region_or_crc
		return false;
	}
	return (*ptr != region_or_crc);
}

/*
 После отключения/включения питания и старта SDK:
60001000: 02ff0000 1e39153d 203d203c 00000000  ..я.=.9.< = ....
60001010: 00000000 07c2060d 074207c2 07430782  ......В.В.B.‚.C.
60001020: 0b03090b 070b0307 02060703 fe891009  ..............‰ю
60001030: 4e52800e 3840444a 0c0601fd 07031810  .ЂRNJD@8э.......
60001040: 1d15110b 140e0906 0d091f17 231b1711  ...............#
60001050: 00000000 0d330000 0ff4fe89 00000000  ......3.‰юф.....
60001060: 6f690000 42fa1a59 c9c400b4 229821e5  ..ioY.ъBґ.ДЙе!
60001070: f6b668f7 56f5cbde d0b4a8b8 016417fe  чh¶цЮЛхVёЁґРю.d.
Замечательная китай-конфигурация phy_rfoption 60001060 == 0x6f69 для WiFi :)

После deep_sleep:
60001000: 02ff0000 1e38153d 213d203c 00000000  ..я.=.8.< =!....
60001010: 00000000 0001050e 078107c1 078207c1  ........Б.Ѓ.Б.‚.
60001020: 0f03070d 070b0307 02070703 fe8c1009  ..............Њю
60001030: 4e52800e 3840444a 0d0603fd 07041911  .ЂRNJD@8э.......
60001040: 1d16120b 140e0a05 0e0a2018 231c1812  ......... .....#
60001050: 00000000 0d330000 0ff4fe89 00000000  ......3.‰юф.....
60001060: 00000001 42fa1a59 c9c400b4 229821e5  ....Y.ъBґ.ДЙе!
60001070: f6b668f7 56f5cbde ffffffff 016417fe  чh¶цЮЛхVяяяяю.d.

60001078 == 0xffffffff - замечательная китай контролька :)
 */


uint32 ICACHE_FLASH_ATTR rtc_mem_backup(uint32 *mem_start, uint32 *mem_end, uint32 off_ram_rtc)
{
	uint32 i = (((uint32)mem_end - (uint32)mem_start + 3) >> 2) + 1;
	volatile uint32 * ptr_reg = &RTC_RAM_BASE[off_ram_rtc>>2];
	uint32 ret = i << 2;
	while(i--) *ptr_reg++ = *mem_start++;
	return ret;
}

uint32 ICACHE_FLASH_ATTR rtc_mem_recovery(uint32 *mem_start, uint32 *mem_end, uint32 off_ram_rtc)
{
	uint32 i = (((uint32)mem_end - (uint32)mem_start + 3) >> 2) + 1;
	volatile uint32 * ptr_reg = &RTC_RAM_BASE[off_ram_rtc>>2];
	uint32 ret = i << 2;
	while(i--) *mem_start++ = *ptr_reg++;
	return ret;
}
