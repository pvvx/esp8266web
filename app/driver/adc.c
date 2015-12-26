/*
 * sar_adc.c Тест на определение скорости работы АЦП
 *
 *  Created on: 12/02/2015
 *      Author: PV`
 *
 */

#include "ets_sys.h"
#include "osapi.h"
#include "hw/esp8266.h"


uint32 wdrv_bufn DATA_IRAM_ATTR;
extern uint8 tout_dis_txpwr_track;

/* Инициализация SAR */
void ICACHE_FLASH_ATTR sar_init(uint32 clk_div, uint32 win_cnt)
{
	tout_dis_txpwr_track = 1;
	if(win_cnt > 8 || win_cnt == 0) win_cnt = 8;
	wdrv_bufn = win_cnt;
	if(clk_div > 23 || clk_div < 2) clk_div = 8;
	SAR_CFG = (SAR_CFG & 0xFFFF00E3) | ((wdrv_bufn-1) << 2) | (clk_div << 8);
	SAR_TIM1 = (SAR_TIM1 & 0xFF000000) | (clk_div * 5 + ((clk_div - 1) << 16) + ((clk_div - 1) << 8) - 1);
	SAR_TIM2 = (SAR_TIM2 & 0xFF000000) | (clk_div * 11 + ((clk_div * 3 - 1) << 8) + ((clk_div * 10 - 1) << 16) - 1);
	// включить SAR
	rom_i2c_writeReg_Mask(108,2,0,5,5,1);
	SAR_CFG1 |= 1 << 21;
	while((SAR_CFG >> 24) & 0x07); // wait r_state == 0
}

/* Отключить SAR */
void ICACHE_FLASH_ATTR sar_off(void)
{
	rom_i2c_writeReg_Mask(108,2,0,5,5,0);
    while((SAR_CFG >> 24) & 0x07); // wait r_state == 0
    SAR_CFG = 0x00908bc; // (SAR_CFG & (~(7<<2))) | (7 << 2) | (8 << 8);
	SAR_TIM1 = 0x0070727; // sar_clk_div 8 // ~3019/(8*8-1,36) = 46 кНz
	SAR_TIM2 = 0x04f1757; // sar_clk_div 8 // ~3019/(8*8-1,36) = 46 кНz
    SAR_CFG1 &= ~(1 << 21);
    uint32 x = SAR_CFG2 & (~1);
    SAR_CFG2 = x;
    SAR_CFG2 = x | 1;
	tout_dis_txpwr_track = 0;
}

void ICACHE_FLASH_ATTR read_adcs(uint16 *ptr, uint16 len, uint32 cfg)
{
    if(len != 0 && ptr != NULL) {
    	int i;
    	sar_init(cfg>>8, cfg & 15);
    	while(len--) {
    		// запуск нового замера SAR
    		uint32 x = SAR_CFG & (~(1 << 1));
    		SAR_CFG = x;
    		SAR_CFG = x | (1 << 1);
			uint16 sar_dout = 0;
    		volatile uint32 * sar_regs = &SAR_DATA; // считать 8 шт. значений SAR с адреса 0x60000D80
       		while((SAR_BASE[20] >> 24) & 0x07); // while(READ_PERI_REG(0x60000D50)&(0x7<<24)); // wait r_state == 0
			for(i = 0; i < wdrv_bufn; i++) {
				// коррекция значений SAR под утечки и т.д.
				int x = ~(*sar_regs++);
				int z = (x & 0xFF) - 21;
				x &= 0x700;
				if(z > 0) x = ((z * 279) >> 8) + x;
				sar_dout += x;
			}
        	*ptr++ = sar_dout;
    	};
    	sar_off();
    }
}



