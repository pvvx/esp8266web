/*
 * sar_adc.c
 *
 *  Created on: 12/02/2015
 *      Author: PV`
 *
 */

#include "ets_sys.h"
#include "osapi.h"
#include "hw/esp8266.h"


uint32 wdrv_bufn DATA_IRAM_ATTR; // кол-во накоплений в буфере ADC (1..8 замеров)
extern uint8 tout_dis_txpwr_track; // флаг отключения процедуры подстройки питания для WiFi с использованием SAR

/* Инициализация SAR sps ~= 3 000 000 / (clk_div * win_cnt)
	clk_div от 1 до 23, win_cnt от 1 до 8 */
void ICACHE_FLASH_ATTR sar_init(uint32 clk_div, uint32 win_cnt)
{
	tout_dis_txpwr_track = 1; // флаг отключения процедуры подстройки питания для WiFi
	if(win_cnt > 8 || win_cnt == 0) win_cnt = 8; // 8 - значение по умолчанию
	wdrv_bufn = win_cnt; // запомнить кол-во накоплений в буфере ADC (1..8 замеров)
	if(clk_div > 23 || clk_div < 2) clk_div = 8; // 8 - значение по умолчанию
	// установить кол-во накоплений в буфер ADC
	SAR_CFG = (SAR_CFG & 0xFFFF00E3) | ((wdrv_bufn-1) << 2) | (clk_div << 8); 
	// установить делители частоты SAR
	SAR_TIM1 = (SAR_TIM1 & 0xFF000000) | (clk_div * 5 + ((clk_div - 1) << 16) + ((clk_div - 1) << 8) - 1);
	SAR_TIM2 = (SAR_TIM2 & 0xFF000000) | (clk_div * 11 + ((clk_div * 3 - 1) << 8) + ((clk_div * 11 - 1) << 16) - 1);
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
    // задать значения по умолчанию для SDK
    SAR_CFG = 0x00908bc; // (SAR_CFG & (~(7<<2))) | (7 << 2) | (8 << 8);
	SAR_TIM1 = 0x0070727; // sar_clk_div 8  
	SAR_TIM2 = 0x04f1757; // sar_clk_div 8 -> 3000000/8/8 = 46875 Hz
    SAR_CFG1 &= ~(1 << 21); 
    uint32 x = SAR_CFG2 & (~1);
    SAR_CFG2 = x;
    SAR_CFG2 = x | 1;
	tout_dis_txpwr_track = 0; // разрешить подстройку питания для WiFi с использованием SAR
}

void ICACHE_FLASH_ATTR read_adcs(uint16 *ptr, uint16 len, uint32 cfg)
{
	if(len != 0 && ptr != NULL) {
		int i;
		sar_init(cfg>>8, cfg & 15); // инициализация, cfg обычно 0x0808 -> 3000000/8/8 = 46875 Hz
		while(len--) {
			// запуск нового замера SAR
			uint32 x = SAR_CFG & (~(1 << 1));
			SAR_CFG = x;
			SAR_CFG = x | (1 << 1);
			uint16 sar_dout = 0;
			volatile uint32 * sar_regs = &SAR_DATA; // указатель для считывания до 8 шт. накопленных
													// значений SAR из аппаратного буфера в 0x60000D80
			while((SAR_CFG >> 24) & 0x07); // while(READ_PERI_REG(0x60000D50)&(0x7<<24)); // wait r_state == 0
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

/* Полная инициализация ADC без SDK:
    // Базовая инициализация CPU на 160 MHz после BIOS-ROM
	IO_RTC_4 = 0; // отключить тактирование всего и WiFi (не обязательно, если используется, другие биты отключают другую периферию)
	GPIO0_MUX = 0; // отключить вывод Q_CLK
	// Установить PLL CLK CPU в 160 MHz для кварца 26MHz
	rom_i2c_writeReg(103, 4, 1, 136);
	rom_i2c_writeReg(103, 4, 2, 145);
	CLK_PRE_PORT |= 1; // отключить делитель CLK на 2
	ets_update_cpu_frequency(80 * 2); // обозначить частоту в MHz для других процедур
	// Инициализация уже самого SAR
	IO_RTC_4 |= 0x06000000; // подключить источник тактирования SAR к PLL 80MHz // SET_PERI_REG_MASK(0x60000710,0x06000000);
	rom_sar_init();	// находится в BIOS-ROM:
					//	IO_RTC_4 |= 0x02000000; 
					//	i2c_writeReg_Mask(108,2,0,4,4,1); 
					//	i2c_writeReg_Mask(108,2,1,1,0,2);
	rom_i2c_writeReg_Mask(98,1,3,7,4,15);
	DPORT_BASE[0x18>>2] |= 0x038f0000; // SET_PERI_REG_MASK(0x3FF00018,0x038f0000);
	HDRF_BASE[0x0e8>>2] |= 0x01800000; // SET_PERI_REG_MASK(0x600005e8,0x01800000);
	sar_init(8,8); // 8,8 - значение по умолчанию, как в SDK
*/

