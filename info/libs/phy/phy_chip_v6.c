/******************************************************************************
 * FileName: phy_chip_v6.c
 * Description: Alternate SDK (libphy.a)
 * Author: PV`
 * (c) PV` 2015
*******************************************************************************/

#include "user_config.h"
#include "bios.h"
#include "hw/esp8266.h"
#include "phy/phy.h"

extern uint8 periodic_cal_sat;
extern uint8 soc_param0; // chip6_phy_init_ctrl +1, берется из esp_init_data_default.bin +50, 0: 40MHz, 1: 26MHz, 2: 24MHz

void ICACHE_FLASH_ATTR ram_tx_mac_enable(void)
{
	// ret.n
}

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

// set_cal_rxdc()
// set_rx_gain_cal_iq()
// gen_rx_gain_table()
//- pbus_set_rxbbgain()
// set_rx_gain_testchip_50() // phy_bb_rx_cfg() // periodic_cal()
//- ram_get_corr_power()
//- check_data_func()
//- do_noisefloor_lsleep_v50()
// do_noisefloor() // noise_init()
// start_dig_rx() // chip_v6_set_chan()
// stop_dig_rx() // chip_v6_set_chan()
// chip_v6_set_chanfreq()
// tx_cap_init() // chip_v6_initialize_bb
// target_power_add_backoff() // tx_pwctrl_init_cal/set_most_pwr_reg
// tx_pwctrl_init_cal() // tx_pwctrl_init
// tx_atten_set_interp()
// tx_pwctrl_init()
// ram_get_noisefloor()
// get_noisefloor_sat()
// ram_set_noise_floor()
// ram_start_noisefloor()
// read_hw_noisefloor()
// noise_check_loop()
// noise_init()
// target_power_backoff()
// sdt_on_noise_start()
// chip_v6_set_chan_rx_cmp()
// chip_v6_set_chan_misc()
// phy_dig_spur_set()
// phy_dig_spur_prot()
// chip_v6_rxmax_ext_dig()
// chip_v6_rxmax_ext()
// phy_bb_rx_cfg()
//- uart_wait_idle()
// phy_pbus_soc_cfg()
// phy_gpio_cfg()
// tx_cont_en()
// tx_cont_dis()
// tx_cont_cfg()
// chip_v6_initialize_bb()
// periodic_cal()
// bbpll_cal()
// periodic_cal_top()
// register_chipv6_phy_init_param()
//+ change_bbpll160_sleep()
//+ change_bbpll160()
//+ set_crystal_uart()
// ant_switch_init()
// reduce_current_init()
// rtc_mem_check()
// phy_afterwake_set_rfoption()
// deep_sleep_set_option()
// register_chipv6_phy()
// set_dpd_bypass()
// set_rf_gain_stage10()
// get_vdd33_offset()
// get_phy_target_power()
// set_most_pwr_reg()
// phy_set_most_tpw()
// phy_vdd33_set_tpw()
// get_adc_rand()
// phy_get_rand()

// вызывается, для двух вариантов soc_param0 = 1: 26MHz и soc_param0 = 2: 24MHz
void ICACHE_FLASH_ATTR change_bbpll160_sleep(void)
{
	RTC_BASE[0x24>>2] = 0x7F;
	RTC_BASE[0x0C>>2] = 0;
	rom_i2c_writeReg(106,2,8,0); // g_phyFuns->i2c_writeReg(106,2,8,0);
	pm_set_sleep_cycles(3); // IO_RTC_SLP_VAL = IO_RTC_SLP_CNT_VAL + 3;
	RTC_BASE[0x40>>2] = 0;
	RTC_BASE[0x44>>2] = 0;
	RTC_BASE[0xA8>>2] &= 0x7E;
	uint32 save_04 = RTC_BASE[0];
	RTC_BASE[0] = 0x800070;
	pm_wakeup_opt(8,0); // RTC_BASE[0x18>>2] = (RTC_BASE[0x18>>2] & 0xFC0) | 8;	RTC_BASE[0xA8>>2] = RTC_BASE[0xA8>>2] & 0x7E;
	uint32 save_00 = RTC_BASE[0x08>>2] | 0x100000;
	if(soc_param0 == 1) { // 1: 26MHz
		rom_i2c_writeReg(103,4,1,136); // g_phyFuns->i2c_writeReg(103,4,1,136);
		rom_i2c_writeReg(103,4,2,145); // g_phyFuns->i2c_writeReg(103,4,2,145);
	}
	else if(soc_param0 == 2) { // 2: 24MHz
		rom_i2c_writeReg_Mask(103,4,2,7,5,2); // g_phyFuns->i2c_writeReg_Mask(103,4,2,7,5,2);
		// rom_i2c_writeReg_Mask(block, host_id, reg_add, Msb, Lsb, indata)
	}
	RTC_BASE[0x08>>2] = save_00;
	pm_wait4wakeup(1); // while((RTC_BASE[0x28>>2] & 3) == 0);
	RTC_BASE[0] = save_04;
}

void ICACHE_FLASH_ATTR change_bbpll160(void)
{
	int chver = get_chip_version();
	if(soc_param0) {
		if(nonamexx_byte1 == 1) change_bbpll160_sleep();
		else if(nonamexx_byte1 == 0 && chver != 1 && chver != 0) change_bbpll160_sleep();
	}
}

extern uint32 chip6_sleep_params;

void ICACHE_FLASH_ATTR set_crystal_uart(void)
{
	if(soc_param0 != 0) { // != 0: 40MHz
		if((chip6_sleep_params & (1<<27)) == 0) {
			change_bbpll160_sleep();
			if(GPIO_IN & (1<<18)) {
				if(((GPIO_IN >> 23) & 7) == 2) {
					if(GPIO7_MUX & (1<<8)) { // Func == U1TXD
						Uart_Init(1);
						uart_buff_switch(1);
					}
				}
			}
		}
	}
}
/*
void rom_i2c_writeReg_Mask(block, host_id, reg_add, Msb, Lsb, indata);
void rom_i2c_writeReg(uint32 a2, uint32 a3, uint32 a4, uint32 a5)
{
	volatile uint32 * ptr_reg = ((volatile uint32 *)0x60000D00 + (a3<<2));
	*ptr_reg = (a5 << 16) | (a4 << 8) | a2 | 0x1000000;
	while((*ptr_reg >> 25) & 1);
}
*/
