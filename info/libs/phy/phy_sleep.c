/******************************************************************************
 * FileName: phy_sleep.c
 * Description: Alternate SDK (libphy.a)
 * Author: PV`
 * (c) PV` 2015
*******************************************************************************/

#include "user_config.h"
#include "bios.h"
#include "hw/esp8266.h"
#include "phy/phy.h"

// pm_rtc_clock_cali()
// clockgate_watchdog()
// pm_usec2rtc()
// pm_rtc2usec()
//+ pm_set_sleep_cycles()
// pm_sleep_opt()
// pm_wakeup_opt()
// get_chip_version()
// pm_sleep_opt_bb_off()
// pm_sleep_opt_bb_on()
// pm_set_pll_xtal_wait_time()
// pm_prepare_to_sleep()
// pm_sdio_nidle()
// chg_lslp_mem_opt_8266()
// pm_goto_sleep()
//+ pm_wait4wakeup()
// pm_open_rf()
// pm_sleep_set_mac()
// pm_set_wakeup_mac()
// pm_check_mac_idle()
// pm_set_sleep_btco()
// pm_set_wakeup_btco()
// pm_set_sleep_mode()
// pm_unmask_bt()
//+ pm_wakeup_init()
// sleep_opt_8266()
// sleep_opt_bb_on_8266()
// sleep_reset_analog_rtcreg_8266()


extern uint32 chip6_sleep_params;

void ICACHE_FLASH_ATTR pm_set_sleep_cycles(uint32 x)
{
	RTC_BASE[1] = x + RTC_BASE[7];
	if(x <= 5000) x = 1;
	else x = 0;
    periodic_cal_sat = x;
}

void ICACHE_FLASH_ATTR pm_wakeup_opt(uint32 a2, uint32 a3)
{
	RTC_BASE[0x18>>2] = (RTC_BASE[0x18>>2] & 0xFC0) | a2;
	RTC_BASE[0xA8>>2] = (RTC_BASE[0xA8>>2] & 0x7E) | a3;
}

extern uint8 software_slp_reject;
extern uint8 hardware_reject;

void ICACHE_FLASH_ATTR pm_wait4wakeup(uint32 a2)
{
	if((a2 == 1 || a2 == 2) && (software_slp_reject == 0)) {
		while((RTC_BASE[0x28>>2] & 3) == 0);
		hardware_reject = RTC_BASE[0x28>>2] & 2;
	}
}

void pm_prepare_to_sleep(void)
{
	if(chip6_phy_init_ctrl[57] != 2 && chip6_phy_init_ctrl[58] != 1 && chip6_phy_init_ctrl[58] != 3) {
		GPIO16_FUN = 2; // 0x600007A0 = 2
	}
}





