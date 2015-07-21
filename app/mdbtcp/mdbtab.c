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
#include "wifi.h"
#include "modbusrtu.h"

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
	{100,0x7FFF, (uint8 *)&wificonfig, MdbWordRW},
	{0xffff,0xffff,0,0}
};

#endif // USE_MODBUS
