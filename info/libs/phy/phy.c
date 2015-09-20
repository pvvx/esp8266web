/******************************************************************************
 * FileName: phy.c
 * Description: Reverse SDK 1.0.0 (libphy.a)
 * Author: PV`
*******************************************************************************/
#include "user_config.h"
#include "os_type.h"
#include "hw/esp8266.h"
#include "phy/phy.h"


struct phy_funcs * ptr_phy; // off_3FFEA990

// used register_chipv6_phy()
void ICACHE_FLASH_ATTR register_phy_ops(struct phy_funcs * phy_base)
{
	ptr_phy = phy_base;
	phy_init(1, 0);
}

extern void chip_v6_initialize_bb(int);
extern int chip_v6_rf_init(int ch, int n);

void ICACHE_FLASH_ATTR phy_init(int8 ch, int n) //(int8 ch, b)???
{
//	ptr_phy->rf_init(ch, n);
//	ptr_phy->initialize_bb(n);

	chip_v6_rf_init(ch, n);
	chip_v6_initialize_bb(n);
}

// used register_chipv6_phy() (reduce_current_init())
void ICACHE_FLASH_ATTR register_get_phy_addr(struct phy_funcs * ptrf)
{
	ptr_phy = ptrf;
}

// used chm_set_current_channel()
int phy_change_channel(int chfr)
{
//	ptr_phy->set_chanfreq(chfr);
	chip_v6_set_chanfreq(chfr);
	return 0;
}

#if 0
// not used
uint32 phy_get_mactime(void)
{
	MEMW();
	return IOREG(0x3FF20C00); // phy_mactime
}
#endif

/* в phy_chip_v6.o
extern void chip_v6_set_chan(int frch);
int ICACHE_FLASH_ATTR chip_v6_set_chanfreq(uint32 chfr)
{
	chip_v6_set_chan(g_phyFuns->mhz2ieee(chfr, 0x80)); // chip_v6_set_chan(rom_mhz2ieee(x, 0x80))
	return 0;
}
*/

#if 0
// used slop_test()
void ICACHE_FLASH_ATTR RFChannelSel(int8 ch)
{
	ptr_phy->set_chan(ch);
}

// not used
void ICACHE_FLASH_ATTR phy_delete_channel(void)
{
	ptr_phy->unset_chanfreq();
}
#endif

extern void rom_chip_v5_enable_cca(void);
extern void rom_chip_v5_disable_cca(void);
// used init_wifi()
void ICACHE_FLASH_ATTR phy_enable_agc(void)
{
//	MEMW();
//	IOREG(0x60009B00) &= ~0x10000000;
//	ptr_phy->enable_cca();
	rom_chip_v5_enable_cca();
}

// used init_wifi()
void ICACHE_FLASH_ATTR phy_disable_agc(void)
{
//	IOREG(0x60009B00) |= 0x10000000;
//	ptr_phy->disable_cca();
	rom_chip_v5_disable_cca();
}

#if 0
// not used
void ICACHE_FLASH_ATTR phy_initialize_bb(int n)
{
	ptr_phy->initialize_bb(n);
}

// not used
void ICACHE_FLASH_ATTR phy_set_sense(void)
{
	ptr_phy->set_sense();
}

void bb_init(void )
{
	ptr_phy->initialize_bb();
}

void rf_init(int8 ch)
{
	ptr_phy->rf_init(ch);
}
#endif

void phy_set_powerup_option(int option)
{
	IO_RTC_POWERUP = option; // 0x6000073C = option
}
