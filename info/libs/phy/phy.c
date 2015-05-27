/******************************************************************************
 * FileName: phy.c
 * Description: Reverse SDK 1.0.0 (phy.a)
 * Author: PV`
 * ver1.0
*******************************************************************************/
#include "user_config.h"
#include "ets.h"
#include "ets_sys.h"
#include "os_type.h"
#include "osapi.h"
#include "user_interface.h"
#include "phy/phy.h"


struct phy_funcs * ptr_phy; // off_3FFEA990

void register_phy_ops(struct phy_funcs * phy_base)
{
	ptr_phy = phy_base;
	phy_init(1, 0);
}

void register_get_phy_addr(struct phy_funcs * phy_base)
{
	ptr_phy = phy_base;
}

int phy_change_channel(int chfr)
{
	ptr_phy->set_chanfreq(chfr);
	return 0;
}

int chip_v6_set_chanfreq(uint32 chfr)
{
	chip_v6_set_chan(g_phyfuns->mhz2ieee(chfr, 0x80)); // chip_v6_set_chan(rom_mhz2ieee(x, 0x80))
	return 0;
}

uint32 phy_get_mactime(void)
{
	return READ_PERI_REG(0x3FF20C00); // phy_mactime
}

void phy_init(int8 ch) //(int8 ch, b)???
{
	ptr_phy->rf_init(ch);
	ptr_phy->initialize_bb();
}

void RFChannelSel(int ch)
{
	ptr_phy->set_chan(ch);
}

void phy_delete_channel(void)
{
	ptr_phy->unset_chanfreq();
}

void phy_enable_agc(void)
{
	ptr_phy->enable_cca();
}

void phy_disable_agc(void) // 1
{
	ptr_phy->disable_cca();
}

void phy_initialize_bb(void)
{
	ptr_phy->initialize_bb();
}

void phy_set_sense(void)
{
	ptr_phy->set_sense();
}

#if 0
void bb_init(void )
{
	ptr_phy->initialize_bb();
}

void rf_init(int8 ch)
{
	ptr_phy->rf_init(ch);
}
#endif

