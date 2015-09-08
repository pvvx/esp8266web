/*
 * phy_get_vdd33.c
 *
 *  Created on: 08 сент. 2015 г.
 *      Author: PVV
 */

#include "user_config.h"
#include "bios.h"
#include "hw/esp8266.h"
#include "sys_const.h"

#define i2c_saradc							0x6C // 108
#define i2c_saradc_hostid					2
#define i2c_saradc_en_test					0
#define i2c_saradc_en_test_msb				5
#define i2c_saradc_en_test_lsb				5

#define i2c_writeReg_Mask(block, host_id, reg_add, Msb, Lsb, indata) \
    rom_i2c_writeReg_Mask(block, host_id, reg_add, Msb, Lsb, indata)

#define i2c_readReg_Mask(block, host_id, reg_add, Msb, Lsb) \
    rom_i2c_readReg_Mask(block, host_id, reg_add, Msb, Lsb)

#define i2c_writeReg_Mask_def(block, reg_add, indata) \
    i2c_writeReg_Mask(block, block##_hostid, reg_add, reg_add##_msb, reg_add##_lsb, indata)

#define i2c_readReg_Mask_def(block, reg_add) \
    i2c_readReg_Mask(block, block##_hostid, reg_add, reg_add##_msb, reg_add##_lsb)


extern uint8 sleep_mode_flag;

void read_sar_dout(uint16 * buf)
{
	volatile uint32 * sar_regs = &SAR_BASE[32]; // 8 шт. с адреса 0x60000D80
	int i;
	for(i = 0; i < 8; i++) {
		int x = ~(*sar_regs++);
		int z = (x & 0xFF) - 21;
		x &= 0x700;
		if(z > 0) x = ((z * 279) >> 8) + x;
		buf[i] = x;
	}
}

int readvdd33(int flg)
{
	uint16 sardata[8], sar_out = 0;
	if(flg) {
		HDRF_BASE[37] |= 1; // 0x60000594 |= 1;
	}
	int pb6_1 = pbus_rd(6,1); // 	g_phyFuns->pbus_rd(6,1);
	int r107 = i2c_readReg_Mask(107, 2, 9, 2, 0); // g_phyFuns->i2c_readReg_Mask(107, 2, 9, 2, 0);
	int r108 = i2c_readReg_Mask(108, 2, 0, 5, 5); // g_phyFuns->i2c_readReg_Mask(108, 2, 0, 5, 5);
	pbus_force_test(6, 1, 2 | pb6_1); // g_phyFuns->pbus_force_test(6, 1, (2 | pb6_1) & 0xffff );
	i2c_writeReg_Mask(107, 2, 9, 7, 7, 1); // g_phyFuns->i2c_writeReg_Mask(107, 2, 9, 7, 7, 1)
	i2c_writeReg_Mask(107, 2, 9, 2, 0, 0); // g_phyFuns->i2c_writeReg_Mask(107, 2, 9, 2, 0, 0)
	i2c_writeReg_Mask_def(i2c_saradc, i2c_saradc_en_test, 1); // g_phyFuns->i2c_writeReg_Mask(i2c_saradc, 2, 0, 5, 5, 1);
	SAR_BASE[23] |= 1<<23; // 0x60000D5C |= 0x800000
	SAR_BASE[23] &= ~(1<<21); // 0x60000D5C &= 0xFFDFFFFF
	if((SAR_BASE[20]>>24)&0xFF)	while((SAR_BASE[20]>>24)&0x7); // 0x60000D50
	SAR_BASE[20] &= 0xFFD;	// 0x60000D50 &= 0xFFD
	SAR_BASE[20] |= 1<<1;	// 0x60000D50 |= 2
	ets_delay_us(25);
	read_sar_dout(sardata);
	int i;
	for(i = 0; i < 8; i++) sar_out += sardata[i];
	pbus_force_test(6, 1, pb6_1);
	i2c_writeReg_Mask(107, 2, 9, 2, 0, r107);
	i2c_writeReg_Mask_def(i2c_saradc, i2c_saradc_en_test, r108); // i2c_writeReg_Mask(108, 2, 0, 5, 5, r108);
	if(flg) {
		HDRF_BASE[37] &= 0x7E; // 0x60000594 &= 0x7E
	}
	sar_out <<= 10;
	sar_out += 0x800;
	return ((sar_out >> 12) & 0xFFFF);
}

int phy_get_vdd33(void)
{
	int vdd33 = 0xFFFF;
	if(sleep_mode_flag == 0) pm_set_sleep_mode(4);
	if(chip6_phy_init_ctrl[108] == 0xff) {
		vdd33 = readvdd33(1);
	}
	while((SAR_BASE[20]>>24)&0x7); // 0x60000D50
	SAR_BASE[23] &= ~(1<<21);	// 0x60000D5C
	SAR_BASE[23] &= ~(1<<23);
	SAR_BASE[24] &= 0xFFE;		// 0x60000D60
	SAR_BASE[24] |= 0x001;
	if(sleep_mode_flag == 0) {
		pm_wakeup_init(4,0);
	}
	return vdd33;
}

uint16 system_get_vdd33(void)
{
	return phy_get_vdd33() & 0x3FF;
}



