/******************************************************************************
 * FileName: phy_chip_v6.c
 * Description: Alternate SDK (libmain.a)
 * Author: PV`
 * (c) PV` 2015
 * ver 0.0.1
*******************************************************************************/

#include "user_config.h"
#include "bios.h"
#include "hw/esp8266.h"
#include "phy/phy.h"

extern uint8 periodic_cal_sat;
extern uint8 soc_param0; // chip6_phy_init_ctrl +1, берется из esp_init_data_default.bin +50, 0: 40MHz, 1: 26MHz, 2: 24MHz

void pm_set_sleep_cycles(uint32 x)
{
	RTC_BASE[1] = x + RTC_BASE[7];
	if(x <= 5000) x = 1;
	else x = 0;
    periodic_cal_sat = x;
}
void pm_wakeup_opt(uint32 a2, uint32 a3)
{
	RTC_BASE[0x18>>2] = (RTC_BASE[0x18>>2] & 0xFC0) | a2;
	RTC_BASE[0xA8>>2] = (RTC_BASE[0xA8>>2] & 0x7E) | a3;
}

extern uint8 software_slp_reject;
extern uint8 hardware_reject;

void pm_wait4wakeup(uint32 a2)
{
	if((a2 == 1 || a2 == 2) && (software_slp_reject == 0)) {
		while((RTC_BASE[0x28>>2] & 3) == 0);
		hardware_reject = RTC_BASE[0x28>>2] & 2;
	}
}

// вызывается, для двух вариантов soc_param0 = 1: 26MHz и soc_param0 = 2: 24MHz
void change_bbpll160_sleep(void)
{
	RTC_BASE[0x24>>2] = 0x7F;
	RTC_BASE[0x0C>>2] = 0;
	rom_i2c_writeReg(106,2,8,0); // g_phyFuns->i2c_writeReg(106,2,8,0);
	pm_set_sleep_cycles(3);
	RTC_BASE[0x40>>2] = 0;
	RTC_BASE[0x44>>2] = 0;
	RTC_BASE[0xA8>>2] &= 0x7E;
	uint32 save_04 = RTC_BASE[0];
	RTC_BASE[0] = 0x800070;
	pm_wakeup_opt(8,0);
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
	pm_wait4wakeup(1);
	RTC_BASE[0] = save_04;
}

void change_bbpll160(void)
{
	int chver = get_chip_version();
	if(soc_param0) {
		if(nonamexx_byte1 == 1) change_bbpll160_sleep();
		else if(nonamexx_byte1 == 0 && chver != 1 && chver != 0) change_bbpll160_sleep();
	}
}

extern uint32 chip6_sleep_params;

void set_crystal_uart(void)
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
