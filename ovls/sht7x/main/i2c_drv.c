/*
 * i2c_drv.c
 *
 *  Created on: 05.03.2016
 *      Author: PVV
 */
#include "user_config.h"
#include "os_type.h"
#include "hw/esp8266.h"
#include "bios.h"

uint32 pin_scl 	DATA_IRAM_ATTR;
uint32 pin_sda 	DATA_IRAM_ATTR;
uint32 bit_pin_scl	DATA_IRAM_ATTR;
uint32 bit_pin_sda	DATA_IRAM_ATTR;
uint32 i2c_time		DATA_IRAM_ATTR;

#define GET_CCOUNT(x) __asm__ __volatile__("rsr.ccount %0" : "=r"(x))

void gpiox_mux_io(uint32 PIN_NUM)
{
	ets_intr_lock();
	SET_PIN_FUNC(PIN_NUM, (MUX_FUN_IO_PORT(PIN_NUM) | (1 << GPIO_MUX_PULLUP_BIT)));
	ets_intr_unlock();
}
/******************************************************************************
*******************************************************************************/
int i2c_init(uint32 scl, uint32 sda, uint32 time)
{
	if(scl > 15 || sda > 15 || scl == sda || time == 0) return 1;
	i2c_time = time; // 54 - 387kHz/195kHz , 354 - 100kHz
	pin_scl = scl;
	pin_sda = sda;
	bit_pin_scl = 1 << scl;
	bit_pin_sda = 1 << sda;
	GPIOx_PIN(scl) = GPIO_PIN_DRIVER;
	GPIOx_PIN(sda) = GPIO_PIN_DRIVER;
	gpiox_mux_io(scl);
	gpiox_mux_io(sda);
	GPIO_OUT_W1TS = bit_pin_sda | bit_pin_scl; // scl = 1, sda = 1
	GPIO_ENABLE_W1TS = bit_pin_sda | bit_pin_scl;
	return 0;
}
/******************************************************************************
*******************************************************************************/
int i2c_deinit(void)
{
	if(pin_scl == 0 || pin_sda == 0 || i2c_time == 0) return 1;
	GPIO_ENABLE_W1TC = bit_pin_sda | bit_pin_scl;
	GPIOx_PIN(pin_scl) = 0;
	GPIOx_PIN(pin_sda) = 0;
//	SET_PIN_FUNC_DEF_SDK(pin_scl);
//	SET_PIN_FUNC_DEF_SDK(pin_sda);
	return 0;
}
/******************************************************************************
*******************************************************************************/
void i2c_set_scl(uint32 x)
{
	uint32 t1,t2;
	if(x) GPIO_OUT_W1TS = bit_pin_scl;
	else GPIO_OUT_W1TC = bit_pin_scl;
	GET_CCOUNT(t1);
	do GET_CCOUNT(t2);
	while(t2-t1 <= i2c_time);
}
/******************************************************************************
*******************************************************************************/
void i2c_set_sda(uint32 x)
{
	uint32 t1,t2;
	if(x) GPIO_OUT_W1TS = bit_pin_sda;
	else GPIO_OUT_W1TC = bit_pin_sda;
	GET_CCOUNT(t1);
	do GET_CCOUNT(t2);
	while(t2-t1 <= i2c_time);
}
/******************************************************************************
*******************************************************************************/
uint32 i2c_get_sda(void)
{
	uint32 t1, t2;
	GET_CCOUNT(t1);
	uint32 ret = 0;
	do GET_CCOUNT(t2);
	while(t2-t1 < i2c_time);
	if(GPIO_IN & bit_pin_sda) ret = 1;
	return ret;
}
/******************************************************************************
*******************************************************************************/
uint32 i2c_step_scl_sda(uint32 x)
{
	i2c_set_sda(x); // sda x, scl 0
    i2c_set_scl(1); // sda x, scl 1
    x = i2c_get_sda(); // time + rd sda
    i2c_set_scl(0); // sda x, scl 0
    return x;
}
/******************************************************************************
*******************************************************************************/
uint32 i2c_test_sda(void)
{
	if(GPIO_IN & bit_pin_sda) return 1;
	return 0;
}
/******************************************************************************
// generates a transmission start
//           ____
// I2CDAT:       |_____
//             ____
// I2C_SCL : ~~    |____
*******************************************************************************/
void i2c_start(void)
{
	i2c_set_sda(1); // sda 1, scl x
	i2c_set_scl(1); // sda 1, scl 1
	i2c_set_sda(0); // sda 0, scl 1
	i2c_set_scl(0); // sda 0, scl 0
}
/******************************************************************************
// generates a transmission stop
//                ___
// I2CDAT:   ____|
//              _____
// I2C_SCL : __|
*******************************************************************************/
void i2c_stop(void)
{
    GPIO_OUT_W1TC = bit_pin_sda; // sda 0
	i2c_set_scl(0); // sda 0, scl 0
	i2c_set_scl(1); // sda 0, scl 1
	i2c_set_sda(1); // sda 1, scl 1
}
/******************************************************************************
*******************************************************************************/
uint32 i2c_wrd8ba(uint32 b)
{
    uint32 ret = (b << 1) | ((b & 0x100) >> 8);
	int i = 8;
    do {
    	ret <<= 1;
    	i2c_set_sda(ret & 0x200); // sda x, scl 0
        i2c_set_scl(1); // sda x, scl 1
        ret |= i2c_get_sda(); // time + rd sda
        i2c_set_scl(0); // sda x, scl 0
    } while(i--);
    GPIO_OUT_W1TS = bit_pin_sda;
    ret = ((ret >> 1) & 0x0ff) | ((ret & 1) << 8);
    return ret;
}

