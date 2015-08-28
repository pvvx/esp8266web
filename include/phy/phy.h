/******************************************************************************
 * FileName: phy.h
 * Description: Reverse SDK 1.0.0/1.1.0 (phy.a)
 * Author: PV`
 *******************************************************************************/

#ifndef _LIBS_PHY_H_
#define _LIBS_PHY_H_

#include "bios/rom_phy.h"

typedef void (* phy_func_var_int_int)(int ch, int n);
typedef void (* phy_func_var_int)(int);
typedef void (* phy_func_void)(void);

struct phy_funcs{	
	phy_func_var_int_int rf_init; 		// +00 = int chip_v6_rf_init(int ch, int n);
	phy_func_var_int set_chanfreq; 		// +04 = void chip_v6_set_chanfreq(int chfr);
	phy_func_var_int set_chan;			// +08 = void chip_v6_set_chan(int ch);
	phy_func_void unset_chanfreq;		// +12 = int chip_v6_unset_chanfreq(void) // return 0;
	phy_func_void enable_cca;			// +16 = void rom_chip_v5_enable_cca(void) // *0x60009B00 &= ~0x10000000;
	phy_func_void disable_cca;			// +20 = void rom_chip_v5_disable_cca(void) // *0x60009B00 |= 0x10000000;
	phy_func_var_int initialize_bb; 	// +24 = int chip_v6_initialize_bb(void)
	phy_func_void set_sense; 			// +28 = void chip_v6_set_sense(void)
};

extern struct phy_func_tab * g_phyFuns; 
//extern uint8 chip6_phy_init_ctrl[128]; // in sys_const.h

void register_phy_ops(struct phy_funcs * phy_base);
void register_get_phy_addr(struct phy_funcs * prt);
int phy_change_channel(int chfr);
uint32 phy_get_mactime(void);
int chip_v6_set_chanfreq(uint32 chfr)  ICACHE_FLASH_ATTR;
void phy_init(int8 ch, int n) ICACHE_FLASH_ATTR; 
void RFChannelSel(int8 ch) ICACHE_FLASH_ATTR;
void phy_delete_channel(void) ICACHE_FLASH_ATTR;
void phy_enable_agc(void) ICACHE_FLASH_ATTR;
void phy_disable_agc(void) ICACHE_FLASH_ATTR;
void phy_initialize_bb(int n) ICACHE_FLASH_ATTR;
void phy_set_sense(void) ICACHE_FLASH_ATTR;


#endif /* _LIBS_PHY_H_ */

