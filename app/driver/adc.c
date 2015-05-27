/*
 * adc.c Тест на определение скорости работы АЦП
 *
 *  Created on: 12/02/2015
 *      Author: PV`
 *
 */

#include "ets_sys.h"
#include "osapi.h"
#include "hw/esp8266.h"

#define i2c_bbpll							0x67 // 103
#define i2c_bbpll_en_audio_clock_out		4
#define i2c_bbpll_en_audio_clock_out_msb	7
#define i2c_bbpll_en_audio_clock_out_lsb	7
#define i2c_bbpll_hostid					4

#define i2c_saradc							0x6C // 108
#define i2c_saradc_hostid					2
#define i2c_saradc_en_test					0
#define i2c_saradc_en_test_msb				5
#define i2c_saradc_en_test_lsb				5

extern int rom_i2c_writeReg_Mask(int block, int host_id, int reg_add, int Msb, int Lsb, int indata);
//extern int rom_i2c_readReg_Mask(int block, int host_id, int reg_add, int Msb, int Lsb);
//extern void read_sar_dout(uint16 * buf);

#define i2c_writeReg_Mask(block, host_id, reg_add, Msb, Lsb, indata) \
    rom_i2c_writeReg_Mask(block, host_id, reg_add, Msb, Lsb, indata)

#define i2c_readReg_Mask(block, host_id, reg_add, Msb, Lsb) \
    rom_i2c_readReg_Mask(block, host_id, reg_add, Msb, Lsb)

#define i2c_writeReg_Mask_def(block, reg_add, indata) \
    i2c_writeReg_Mask(block, block##_hostid, reg_add, reg_add##_msb, reg_add##_lsb, indata)

#define i2c_readReg_Mask_def(block, reg_add) \
    i2c_readReg_Mask(block, block##_hostid, reg_add, reg_add##_msb, reg_add##_lsb)

void ICACHE_FLASH_ATTR mread_sar_dout(uint16 * buf)
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
/*
#ifdef ADC_DEBUG
#define ADC_DBG os_printf
#else
#define ADC_DBG
#endif

uint16 ICACHE_FLASH_ATTR adc_read(void)
{
    uint8 i;
    uint16 sar_dout, tout, sardata[8];

    i2c_writeReg_Mask_def(i2c_saradc, i2c_saradc_en_test, 1); //select test mux

    //PWDET_CAL_EN=0, PKDET_CAL_EN=0
    SET_PERI_REG_MASK(0x60000D5C, 0x200000);

    while (GET_PERI_REG_BITS(0x60000D50, 26, 24) > 0); //wait r_state == 0

    CLEAR_PERI_REG_MASK(0x60000D50, 0x02);    //force_en=0
    SET_PERI_REG_MASK(0x60000D50, 0x02);    //force_en=1

    os_delay_us(2);

    sar_dout = 0;
    while (GET_PERI_REG_BITS(0x60000D50, 26, 24) > 0); //wait r_state == 0

    read_sar_dout(sardata);

    for (i = 0; i < 8; i++) {
        sar_dout += sardata[i];
        ADC_DBG("%d, ", sardata[i]);
    }

    tout = (sar_dout + 8) >> 4;   // tout is 10 bits fraction

    i2c_writeReg_Mask_def(i2c_saradc, i2c_saradc_en_test, 0); //select test mux

    while (GET_PERI_REG_BITS(0x60000D50, 26, 24) > 0); // wait r_state == 0

    CLEAR_PERI_REG_MASK(0x60000D5C, 0x200000);
    SET_PERI_REG_MASK(0x60000D60, 0x1);    //force_en=1
    CLEAR_PERI_REG_MASK(0x60000D60, 0x1);    //force_en=1

    return tout;      //tout is 10 bits fraction
}
*/

void ICACHE_FLASH_ATTR read_adcs(uint16 *ptr, uint16 len)
{
    if(len != 0 && ptr != NULL) {
    	uint16 sar_dout, sardata[8];
#if (0) // в web этой инициализации из system_read_adc() (= test_tout(0)) / не требуется // при test_tout(1) - в SDK выводится отладка сумм SAR
		uint32 store_reg710 = READ_PERI_REG(0x60000710);
		uint32 store_reg58e = READ_PERI_REG(0x600005e8);
		uint32 store018 = READ_PERI_REG(0x3FF00018);
    	if((store_reg710 & 0xfe000000) != 0xfe000000) {
    		SET_PERI_REG_MASK(0x3FF00018,0x038f0000);
    		SET_PERI_REG_MASK(0x60000710,0xfe000000);
    		rom_i2c_writeReg_Mask(98,1,3,7,4,15);
    		rom_sar_init();
    		ets_delay_us(2);
    		SET_PERI_REG_MASK(0x600005e8,0x01800000);
    		ets_delay_us(2);
    	}
    	else pm_set_sleep_mode(4);
#endif
//    	i2c_writeReg_Mask(108,2,1,1,0,1);
    	i2c_writeReg_Mask_def(i2c_saradc, i2c_saradc_en_test, 1); //select test mux
    	SAR_BASE[23] |= 1 << 21; //    	SET_PERI_REG_MASK(0x60000D5C, 1<<21); // 0x200000
   		while((SAR_BASE[20] >> 24) & 0x07); // while(READ_PERI_REG(0x60000D50)&(0x7<<24)); // wait r_state == 0
    	while(len--) {
    		uint32 x = SAR_BASE[20] & (~(1 << 1));
    		SAR_BASE[20] = x; // CLEAR_PERI_REG_MASK(0x60000D50,2);
    		SAR_BASE[20] = x | (1 << 1); // SET_PERI_REG_MASK(0x60000D50,2);
//        	ets_delay_us(2);
       		sar_dout = 0;
       		while((SAR_BASE[20] >> 24) & 0x07); // while(READ_PERI_REG(0x60000D50)&(0x7<<24)); // wait r_state == 0
       		mread_sar_dout(sardata);
        	int i;
        	for(i = 0; i < 8; i++) sar_dout += sardata[i];
        	*ptr++ = sar_dout;
    	};
    	// I assume this code re-enables interrupts
    	i2c_writeReg_Mask_def(i2c_saradc, i2c_saradc_en_test, 0);
    	while((SAR_BASE[20] >> 24) & 0x07);	// while(READ_PERI_REG(0x60000D50)&(0x7<<24)); // wait r_state == 0
    	SAR_BASE[23] &= 1 << 21; // CLEAR_PERI_REG_MASK(0x60000D5C,1<<21); // 0x200000
    	uint32 x = SAR_BASE[24] & (~1); //	CLEAR_PERI_REG_MASK(0x60000D60,1);
    	SAR_BASE[24] = x;
    	SAR_BASE[24] = x | 1;	// SET_PERI_REG_MASK(0x60000D60,1);
#if (0) // в web этой деинициализации не требуется
		if((store_reg710 & 0xfe000000) != 0xfe000000) {
			WRITE_PERI_REG(0x600005e8,((READ_PERI_REG(0x600005e8) & 0xfe7fffff) | 0x00800000));
			rom_i2c_writeReg_Mask(98,1,3,7,4,0);
			WRITE_PERI_REG(0x60000710, store_reg710);
			WRITE_PERI_REG(0x3FF00018, store018);
		}
		else	pm_wakeup_init(4,0);
#endif
    }
}




