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
	RTC_RAM_BASE[0x78>>2] |= x; // 0x60001078 - это €чейка пам€ти RTC c китай-контролькой области от 0x60001000 до 0x60001078, получаема€ путем OR всех слов этой области :)
	// но сколько не OR-ь все равно китай-програмер получит шиш в виде несовпадени€, т.к.... :)
}

bool ICACHE_FLASH_ATTR system_deep_sleep_set_option(uint8 option)
{
	uint32 x = (RTC_RAM_BASE[0x60>>2] & 0xFFFF) | (option << 16);
	RTC_RAM_BASE[0x60>>2] = x; // 0x60001060
	rtc_mem_check(false); // пересчитать OR
	// return всегда fasle
}

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
 ѕосле отключени€/включени€ питани€ и старта SDK:
60001000: 02ff0000 1e39153d 203d203c 00000000  ..€.=.9.< = ....
60001010: 00000000 07c2060d 074207c2 07430782  ......¬.¬.B.В.C.
60001020: 0b03090b 070b0307 02060703 fe891009  ..............Йю
60001030: 4e52800e 3840444a 0c0601fd 07031810  .АRNJD@8э.......
60001040: 1d15110b 140e0906 0d091f17 231b1711  ...............#
60001050: 00000000 0d330000 0ff4fe89 00000000  ......3.Йюф.....
60001060: 6f690000 42fa1a59 c9c400b4 229821e5  ..ioY.ъBі.ƒ…е!
60001070: f6b668f7 56f5cbde d0b4a8b8 016417fe  чhґцёЋхVЄ®і–ю.d.
«амечательна€ китай-конфигураци€ phy_rfoption 60001060 == 0x6f69 дл€ WiFi :)

ѕосле deep_sleep:
60001000: 02ff0000 1e38153d 213d203c 00000000  ..€.=.8.< =!....
60001010: 00000000 0001050e 078107c1 078207c1  ........Ѕ.Б.Ѕ.В.
60001020: 0f03070d 070b0307 02070703 fe8c1009  ..............Мю
60001030: 4e52800e 3840444a 0d0603fd 07041911  .АRNJD@8э.......
60001040: 1d16120b 140e0a05 0e0a2018 231c1812  ......... .....#
60001050: 00000000 0d330000 0ff4fe89 00000000  ......3.Йюф.....
60001060: 00000001 42fa1a59 c9c400b4 229821e5  ....Y.ъBі.ƒ…е!
60001070: f6b668f7 56f5cbde ffffffff 016417fe  чhґцёЋхV€€€€ю.d.

60001078 == 0xffffffff - замечательна€ китай контролька :)
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
