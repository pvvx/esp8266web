/******************************************************************************
 * FileName: hspi_master.c
 * HSPI master
 * Created on: 24/01/2016
 * Author: PV`
 ******************************************************************************/
#include "bios.h"
#include "hw/esp8266.h"
#include "hw/spi_register.h"
#include "hw/pin_mux_register.h"
#include "hw/gpio_register.h"
#include "bios/gpio.h"
#include "sdk/rom2ram.h"

#include "ovl_sys.h"

//-------------------------------------------------------------------------------
// hspi_cmd_write
//-------------------------------------------------------------------------------
void ICACHE_FLASH_ATTR  hspi_cmd_write(uint32 cs_pin, uint32 cmd, uint8 * data, uint32 len)
{
	GPIO_OUT_W1TC = 1 << cs_pin;
//	while(SPI1_CMD & SPI_USR);
	uint32 reg_user = (SPI1_USER & (~ (SPI_USR_ADDR | SPI_USR_MISO | SPI_USR_MOSI))) | SPI_USR_COMMAND;
	SPI1_USER2 = (SPI1_USER2 & (~(SPI_USR_COMMAND_VALUE << SPI_USR_COMMAND_VALUE_S))) | cmd;
	if(data != NULL && len != 0) {
		if(len > SPI_USR_MOSI_BITLEN + 1) len = SPI_USR_MOSI_BITLEN + 1;
		copy_s1d4((void *)&SPI1_W0, data, len);
		SPI1_USER1 = (SPI1_USER1 & (~(SPI_USR_MOSI_BITLEN)<< SPI_USR_MOSI_BITLEN_S)) | (((len << 3)-1) << SPI_USR_MOSI_BITLEN_S);
		reg_user |= SPI_USR_MOSI;
	}
	SPI1_USER = reg_user;
	SPI1_CMD = SPI_USR;
	while(SPI1_CMD & SPI_USR);
	GPIO_OUT_W1TS = 1 << cs_pin;
	if((reg_user & (SPI_DOUTDIN | SPI_USR_MOSI)) == (SPI_DOUTDIN | SPI_USR_MOSI)) {
		copy_s4d1(data, (void *)&SPI1_W0, len);
	}
	ets_delay_us(10);
}
//-------------------------------------------------------------------------------
// hspi_cmd_read
//-------------------------------------------------------------------------------
void ICACHE_FLASH_ATTR  hspi_cmd_read(uint32 cs_pin, uint32 cmd, uint8 * data, uint32 len)
{
	GPIO_OUT_W1TC = 1 << cs_pin;
//	while(SPI1_CMD & SPI_USR);
	uint32 reg_user = (SPI1_USER & (~ (SPI_USR_ADDR | SPI_USR_MISO | SPI_USR_MOSI))) | SPI_USR_COMMAND;
	SPI1_USER2 = (SPI1_USER2 & (~(SPI_USR_COMMAND_VALUE << SPI_USR_COMMAND_VALUE_S))) | cmd;
	if(data != NULL && len != 0) {
		if(len > SPI_USR_MISO_BITLEN + 1) len = SPI_USR_MISO_BITLEN + 1;
		SPI1_USER1 = (SPI1_USER1 & (~(SPI_USR_MISO_BITLEN << SPI_USR_MISO_BITLEN_S))) | (((len << 3)-1) << SPI_USR_MISO_BITLEN_S);
		reg_user |= SPI_USR_MISO;
	}
	SPI1_USER = reg_user;
	SPI1_CMD = SPI_USR;
	while(SPI1_CMD & SPI_USR);
	GPIO_OUT_W1TS = 1 << cs_pin;
	if(len) copy_s4d1(data, (void *)&SPI1_W0, len);
//	ets_delay_us(10);
}
/*****************************************************************************************************
 * hspi_master_init(0x0f071011,10000000)
 * Биты flg:
 * Бит 0..3 - SETUP_TIME
 * Бит 4..7 - HOLD_TIME
 * Бит 8..9 - SPI MODE 0,1,2,3:
 * Бит 8 - CPHA, фаза CLK
 * Бит 9 - CPOL, инверсия CLK
 * Бит 10 - передача и прием данных
 * Бит 12 - СS =0 none, =1 GPIO15
 * Бит 16..19 - COMMAND_BITLEN-1
 * Бит 24..29 - ADDR_BITLEN-1
 *****************************************************************************************************/
uint32 ICACHE_FLASH_ATTR hspi_master_init(uint32 flg, uint32 clock)
{
	GPIO_MUX_CFG &= ~ SPI1_CLK_EQU_SYS_CLK; // |= SPI0_CLK_EQU_SYS_CLK | SPI1_CLK_EQU_SYS_CLK | 5; // по умолчанию в SDK тут в регистре значение 0x305 = (SPI0_CLK_EQU_SYS_CLK | SPI1_CLK_EQU_SYS_CLK | 5)
	SET_PIN_FUNC(12, FUNC_HSPIQ_MISO); 	// set_gpiox_mux_func(12, FUNC_HSPIQ_MISO); // MISO
	SET_PIN_FUNC(13, FUNC_HSPID_MOSI); 	// set_gpiox_mux_func(13, FUNC_HSPID_MOSI); // MOSI
	SET_PIN_FUNC(14, FUNC_HSPI_CLK); 	// set_gpiox_mux_func(14, FUNC_HSPI_CLK); // CLK
	// СS enable ?
	uint32 reg_val = 0x1F;
	if((flg&(1<<12))!=0) {
		SET_PIN_FUNC(15, FUNC_HSPI_CS0); // set_gpiox_mux_func(15, FUNC_HSPI_CS0);
		reg_val = 0x1E; //~(SPI_CS0_DIS);
	}
    // CPOL clock polarity
    if ((flg & 0x200) != 0)  {
    	flg ^= 0x100; // смена CPHA
    	reg_val |= SPI_PIN_CLK_INV;
    }
    SPI1_PIN = reg_val;
    // CS_SETUP
    if((flg & 0x0F) != 0) {
    	flg--; // иначе вставит лишний такт !
    	reg_val = SPI_CS_SETUP; // + такты перед CLK полсе включения CS
    }
    else reg_val = 0;
    // CS_HOLD
    if((flg & 0xF0) != 0) reg_val |= SPI_CS_HOLD; // + такты после CLK перед снятием CS
    // CPHA clock phase
    if ((flg & 0x100) != 0)	reg_val |= SPI_CK_OUT_EDGE | SPI_CK_I_EDGE; // set clock phase
    //
//    if ((flg & 0x400) != 0)	reg_val |= SPI_USR_MOSI_HIGHPART | SPI_USR_MISO_HIGHPART; // чтение/запись с адреса SPI_W8
    // out & in
    if ((flg & 0x400) != 0)	reg_val |= SPI_DOUTDIN;
    SPI1_USER = reg_val;
    // ADDR_BITLEN
	SPI1_USER1 = (((flg>>24) & SPI_USR_ADDR_BITLEN)<<SPI_USR_ADDR_BITLEN_S)
				|((7 & SPI_USR_MOSI_BITLEN)<<SPI_USR_MOSI_BITLEN_S)
				|((7 & SPI_USR_MISO_BITLEN)<<SPI_USR_MISO_BITLEN_S)
				|((0 & SPI_USR_DUMMY_CYCLELEN)<<SPI_USR_DUMMY_CYCLELEN_S);
	// COMMAND_BITLEN
	SPI1_USER2 = ((flg>>16) & SPI_USR_COMMAND_BITLEN) << SPI_USR_COMMAND_BITLEN_S;
	//
	SPI1_CTRL = 0;
	// SETUP_TIME & HOLD_TIME
	SPI1_CTRL2 = flg & 0x0FF; // ((0 & SPI_SETUP_TIME) << SPI_SETUP_TIME_S) | ((1 & SPI_HOLD_TIME) << SPI_HOLD_TIME_S);
	// SPI_CLK
#define MIN_SPI_CLK (APB_CLK_FREQ+8192*64)/8192/64 // 80000000/8192/64 = 152.587891
	uint32 clk_pre = 1;
	uint32 clk_cnt = 1;
	if(clock > (APB_CLK_FREQ/2)) {
		SPI1_CLOCK = SPI_CLK_EQU_SYSCLK;
	}
	else {
		if(clock < MIN_SPI_CLK) clock = MIN_SPI_CLK;
		uint32 clk_div = APB_CLK_FREQ / clock;
		while(++clk_cnt < 64) {
			clk_pre = clk_div / clk_cnt;
			if(clk_pre == 0) break;
			if(clk_div >= clk_cnt * clk_pre) break;
		}
	}
	// SPI clock:
	// 0,1,0,1 - 40 MHz;	1,1,0,1 - 20 MHz;	0,63,31,63 - 1.25 MHz
	// f = 80000000 / (CLKDIV * CLKCNT_N)
	SPI1_CLOCK = ((clk_pre - 1)<<SPI_CLKDIV_PRE_S)
				|(((clk_cnt-1) & SPI_CLKCNT_N)<<SPI_CLKCNT_N_S)
				|((((clk_cnt >> 1)-1) & SPI_CLKCNT_H)<<SPI_CLKCNT_H_S)
				|(((clk_cnt-1) & SPI_CLKCNT_L)<<SPI_CLKCNT_L_S); // clear bit 31, set SPI clock div
	return (APB_CLK_FREQ/(clk_cnt*clk_pre));
}

