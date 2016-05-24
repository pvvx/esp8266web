/******************************************************************************
 * FileName: sar_read_fast.c
 * Description: Alternate SDK (libphy.a)
 * (c) PV` 2015
*******************************************************************************/
#include "ets_sys.h"
#include "osapi.h"
#include "hw/esp8266.h"
/*
 * uint16 *adc_addr: ADC sample output address
 * uint16 adc_num: ADC sample count, range [1, 65535]?
 * uint8 adc_clk_div: ADC sample collection clock=80M/adc_clk_div, range[2, 32]? Тут ошибка у китайцев.
 * Example(Continue to collect 100 ADC samples):
 * uint16 adc_out[100];
 * phy_adc_read_fast(&adc_out[0], 100, 8);
 */
extern uint8 tout_dis_txpwr_track;

void phy_adc_read_fast(uint16 *adc_addr, uint16 adc_num, uint8 adc_clk_div)
{
	tout_dis_txpwr_track = 1;
	if(adc_clk_div < 2) adc_clk_div = 2;
	uint32 save_20 = SAR_BASE[20];
	uint32 save_21 = SAR_BASE[21];
	uint32 save_22 = SAR_BASE[22];
	SAR_BASE[20] = (SAR_BASE[20] & 0xFFFF00FF) | ((adc_clk_div & 0xFF) << 8);
	SAR_BASE[21] = (SAR_BASE[21] & 0xFF000000) | (adc_clk_div * 5 + ((adc_clk_div - 1) << 16) + ((adc_clk_div - 1) << 8) - 1);
	SAR_BASE[22] = (SAR_BASE[22] & 0xFF000000) | (adc_clk_div * 11 + ((adc_clk_div * 3 - 1) << 8) + ((adc_clk_div * 11 - 1) << 16) - 1);
	SAR_BASE[20] &= 0xFFFFFFE3;
	rom_i2c_writeReg_Mask(108,2,0,5,5,1);
	SAR_BASE[23] |= 1 << 21;
	while((SAR_BASE[20] >> 24) & 0x07);	// while(READ_PERI_REG(0x60000D50)&(0x7<<24)); // wait r_state == 0
	while(adc_num--) {
		SAR_BASE[20] &= 0xFFFFFFE3;
		SAR_BASE[20] |= 1 << 1;
		ets_delay_us(1);
		while((SAR_BASE[20] >> 24) & 0x07);	// while(READ_PERI_REG(0x60000D50)&(0x7<<24)); // wait r_state == 0

		uint32 x = ~(SAR_BASE[32]);
		uint32 z = x & 0x7FF;
		x &= 0xFF;
		z &= 0xF00;
		x -= 21;
		if((signed int) x > 0) {
			x = (x * 279) >> 8;
			if(x > 0xff) x = 0xff;
			z += x;
		}
		z++;
		z >>= 1;
		if(chip6_phy_init_ctrl[108] == 0xff) z = 0xFFFF;
		*adc_addr++ = z;
	}
	rom_i2c_writeReg_Mask(108,2,0,5,5,0);


	SAR_BASE[20] = save_20;
	SAR_BASE[21] = save_21;
	SAR_BASE[22] = save_22;
	while((SAR_BASE[20] >> 24) & 0x07);	// while(READ_PERI_REG(0x60000D50)&(0x7<<24)); // wait r_state == 0
	SAR_BASE[23] &= ~(1 << 21);
	SAR_BASE[24] &= ~1;
	SAR_BASE[24] |= 1;
	tout_dis_txpwr_track = 0;
}

