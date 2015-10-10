/******************************************************************************
 * FileName: MdbTab.c 
 * ModBus RTU / TCP
 * Author: PV`
 * (c) PV` 2015
*******************************************************************************/
#include "user_config.h"
#ifdef USE_MODBUS
#include "ets_sys.h"
#include "os_type.h"
#include "osapi.h"
#include "bios.h"
#include "hw/esp8266.h"
#include "wifi.h"
#include "sdk/add_func.h"
#include "driver/adc.h"
#include "modbusrtu.h"

uint32 gpio_fun_pin_num DATA_IRAM_ATTR;
uint32 arg_funcs DATA_IRAM_ATTR;

uint32 MdbUserFunc(unsigned char * mdb, unsigned char * buf, uint32 rwflg)
{
	if(rwflg&0x10000) { // Запись?
        unsigned int x = ((mdb[1])<<8)|(mdb[0]);
		switch(x) {
		case 1:
			system_deep_sleep(arg_funcs *1000);
			break;
		case 2:
			// ...
			break;
		default:
			return MDBERRDATA;
		}
	}
	return MDBERRNO;
}

uint32 MdbVcc(unsigned char * mdb, unsigned char * buf, uint32 rwflg)
{
	if(rwflg&0x10000) { // Запись?
		return MDBERRDATA;
	}
	while(rwflg--) {
		uint32 x = readvdd33();
		*mdb++ = x; // *buf++;
		*mdb++ = x >> 8; // *buf++;
	}
	return MDBERRNO;
}


uint32 MdbAdc(unsigned char * mdb, unsigned char * buf, uint32 rwflg)
{
	if(rwflg&0x10000) { // Запись?
		return MDBERRDATA;
	}
	uint32 x = 0;
	while(rwflg--) {
		read_adcs((uint16 *)&x, 1);
		*mdb++ = x; // *buf++;
		*mdb = x >> 8; // *buf++;
	}
	return MDBERRNO;
}

uint32 MdbGpioFunc(unsigned char * mdb, unsigned char * buf, uint32 rwflg)
{
    if(gpio_fun_pin_num > 15) return MDBERRDATA;
	if(rwflg&0x10000) { // Запись?
		SET_PIN_FUNC(gpio_fun_pin_num, mdb[0] );
	}
	uint32 x = GET_PIN_FUNC(gpio_fun_pin_num);
	x = (x & 3) | ((x >> 2) & 4);
    *mdb++ = x; // *buf++;
    *mdb = 0; // *buf++;
	return MDBERRNO;
}

uint32 MdbOut55AA(unsigned char * mdb, unsigned char * buf, uint32 rwflg)
{
	if(rwflg&0x10000) { // Запись?
	    // rwflg &= 0xFFFF;
	    return MDBERRDATA;
	}
    while(rwflg--) {
        *mdb++ = 0x55; // *buf++;
   	    *mdb++ = 0xAA; // *buf++;
   	}
	return MDBERRNO;
}            

// таблица переменных ModBus
// Чтение/запись 40001, 30001, ...
const smdbtabaddr mdbtabaddr[]=
{
	{0,99, NULL, MdbOut55AA},
	{100,100, (uint8 *)&GPIO_IN, MdbWordR},
	{101,101, (uint8 *)&GPIO_OUT, MdbWordRW},
	{102,102, (uint8 *)&GPIO_OUT_W1TS, MdbWordRW},
	{103,103, (uint8 *)&GPIO_OUT_W1TC, MdbWordRW},
	{104,105, (uint8 *)&GPIO_ENABLE, MdbWordRW},
	{105,105, (uint8 *)&gpio_fun_pin_num, MdbWordRW},
	{106,106, (uint8 *)&gpio_fun_pin_num, MdbGpioFunc},
	{107,107, (uint8 *)NULL, MdbAdc},
	{108,108, (uint8 *)NULL, MdbVcc},
	{109,109, (uint8 *)&arg_funcs, MdbWordRW},
	{110,110, (uint8 *)&arg_funcs, MdbUserFunc},
	{111,0x7FFF, (uint8 *)&wificonfig, MdbWordRW},
	{0xffff,0xffff,0,0}
};

#endif // USE_MODBUS
