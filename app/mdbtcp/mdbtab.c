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
#include "web_iohw.h"
#include "tcp2uart.h"
#include "webfs.h"
#include "web_iohw.h"
#ifdef USE_SNTP
#include "sntp.h"
#endif

#include "driver/rs485drv.h"

uint32 gpio_fun_pin_num DATA_IRAM_ATTR;
uint32 arg_funcs DATA_IRAM_ATTR;
uint32 ret_funcs DATA_IRAM_ATTR;
uint64 mdb_mactime DATA_IRAM_ATTR;
smdb_ubuf mdb_buf; // буфер переменных Modbus для обмена между интерфейсами Web<->RS-485<->TCP

#ifdef USE_SNTP
uint32 lock_sntp_time DATA_IRAM_ATTR;
uint32 ICACHE_FLASH_ATTR MdbGetSntpTimeLw(unsigned char * mdb, unsigned char * buf, uint32 rwflg)
{
	lock_sntp_time = get_sntp_time();
	return MdbWordR(mdb,buf,rwflg);
}
#endif

uint32 ICACHE_FLASH_ATTR MdbUserFunc(unsigned char * mdb, unsigned char * buf, uint32 rwflg)
{
	uint32 ret_funcs = 0;
	if(rwflg&0x10000) { // Запись?
        unsigned int x = ((mdb[1])<<8)|(mdb[0]);
		switch(x) {
		case 1: // deep_sleep
			system_deep_sleep(arg_funcs *1000);
			break;
		case 2:
			// ...
			break;
		default:
			return MDBERRDATA;
		}
	}
	*mdb++ = ret_funcs;
	*mdb = ret_funcs >> 8;
	return MDBERRNO;
}

#ifdef USE_RS485DRV

uint32 ICACHE_FLASH_ATTR MdbSetCfg(unsigned char * mdb, unsigned char * buf, uint32 rwflg)
{
	if(rwflg&0x10000) { // Запись?
        unsigned int x = ((mdb[1])<<8)|(mdb[0]);
		if(x&1) {
			rs485_drv_set_pins();
			rs485_drv_set_baud();
		}
		if(x&2) uart_save_fcfg(1);
		if(x&4) uart_read_fcfg(1);
	}
	*mdb++ = 0;
	*mdb = 0;
	return MDBERRNO;
}
#endif

uint32 ICACHE_FLASH_ATTR MdbVcc(unsigned char * mdb, unsigned char * buf, uint32 rwflg)
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


uint32 ICACHE_FLASH_ATTR MdbAdc(unsigned char * mdb, unsigned char * buf, uint32 rwflg)
{
	if(rwflg&0x10000) { // Запись?
		return MDBERRDATA;
	}
	uint32 x = 0;
	while(rwflg--) {
		read_adcs((uint16 *)&x, 1, 0x0808);
		*mdb++ = x; // *buf++;
		*mdb = x >> 8; // *buf++;
	}
	return MDBERRNO;
}

uint32 ICACHE_FLASH_ATTR MdbGpioFunc(unsigned char * mdb, unsigned char * buf, uint32 rwflg)
{
    if(gpio_fun_pin_num > 15) return MDBERRDATA;
	if(rwflg&0x10000) { // Запись?
		set_gpiox_mux_func(gpio_fun_pin_num, mdb[0] );
	}
	uint32 x = GET_PIN_FUNC(gpio_fun_pin_num);
    *mdb++ = x; // *buf++;
    *mdb = 0; // *buf++;
	return MDBERRNO;
}

uint32 ICACHE_FLASH_ATTR MdbGpioMux(unsigned char * mdb, unsigned char * buf, uint32 rwflg)
{
    if(gpio_fun_pin_num > 15) return MDBERRDATA;
    volatile uint32 * ptr = get_addr_gpiox_mux(gpio_fun_pin_num);
	if(rwflg&0x10000) { // Запись?
		*ptr = ((mdb[1])<<8)|(mdb[0]);
	}
	uint32 x = *ptr;
    *mdb++ = x; // *buf++;
    *mdb = x >> 8; // *buf++;
	return MDBERRNO;
}

uint32 ICACHE_FLASH_ATTR MdbGpioPullUp(unsigned char * mdb, unsigned char * buf, uint32 rwflg)
{
    if(gpio_fun_pin_num > 15) return MDBERRDATA;
    volatile uint32 * ptr = get_addr_gpiox_mux(gpio_fun_pin_num);
	if(rwflg&0x10000) { // Запись?
		if(mdb[0]) *ptr |= 1 << GPIO_MUX_PULLUP_BIT;
		else *ptr &= ~(1 << GPIO_MUX_PULLUP_BIT);
	}
    *mdb++ = (*ptr >> GPIO_MUX_PULLUP_BIT) & 1; // *buf++;
    *mdb = 0; // *buf++;
	return MDBERRNO;
}

uint32 ICACHE_FLASH_ATTR MdbGpioOd(unsigned char * mdb, unsigned char * buf, uint32 rwflg)
{
    if(gpio_fun_pin_num > 15) return MDBERRDATA;
    volatile uint32 * ptr = get_addr_gpiox_mux(gpio_fun_pin_num);
	if(rwflg&0x10000) { // Запись?
		if(mdb[0]) *ptr |= 1 << GPIO_MUX_SLEEP_OE_BIT8;
		else *ptr &= ~(1 << GPIO_MUX_SLEEP_OE_BIT8);
	}
    *mdb++ = (*ptr >> GPIO_MUX_SLEEP_OE_BIT8) & 1; // *buf++;
    *mdb = 0; // *buf++;
	return MDBERRNO;
}

uint32 ICACHE_FLASH_ATTR MdbMacTime(unsigned char * mdb, unsigned char * buf, uint32 rwflg)
{
	mdb_mactime = get_mac_time();
	return MdbWordR(mdb,buf,rwflg);
}

uint32 ICACHE_FLASH_ATTR MdbOut55AA(unsigned char * mdb, unsigned char * buf, uint32 rwflg)
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


#if MDB_BUF_MAX >= MDB_SYS_VAR_ADDR
	#error "MDB_BUF_MAX >= MDB_SYS_VAR_ADDR!"
#endif
// таблица переменных ModBus
// Чтение/запись 40001, 30001, ...
const smdbtabaddr mdbtabaddr[]=
{
	{0,(sizeof(mdb_buf)>>1)-1, (uint8 *)&mdb_buf, MdbWordRW},
	{sizeof(mdb_buf)>>1,MDB_SYS_VAR_ADDR-1, NULL, MdbOut55AA},
	{MDB_SYS_VAR_ADDR+0,MDB_SYS_VAR_ADDR+0, (uint8 *)&GPIO_IN, MdbWordR},
	{MDB_SYS_VAR_ADDR+1,MDB_SYS_VAR_ADDR+1, (uint8 *)&GPIO_OUT, MdbWordRW},
	{MDB_SYS_VAR_ADDR+2,MDB_SYS_VAR_ADDR+2, (uint8 *)&GPIO_OUT_W1TS, MdbWordRW},
	{MDB_SYS_VAR_ADDR+3,MDB_SYS_VAR_ADDR+3, (uint8 *)&GPIO_OUT_W1TC, MdbWordRW},
	{MDB_SYS_VAR_ADDR+4,MDB_SYS_VAR_ADDR+4, (uint8 *)&GPIO_ENABLE, MdbWordRW},
	{MDB_SYS_VAR_ADDR+5,MDB_SYS_VAR_ADDR+5, (uint8 *)&gpio_fun_pin_num, MdbWordRW},
	{MDB_SYS_VAR_ADDR+6,MDB_SYS_VAR_ADDR+6, (uint8 *)&gpio_fun_pin_num, MdbGpioFunc},
	{MDB_SYS_VAR_ADDR+7,MDB_SYS_VAR_ADDR+7, (uint8 *)&gpio_fun_pin_num, MdbGpioPullUp},
	{MDB_SYS_VAR_ADDR+8,MDB_SYS_VAR_ADDR+8, (uint8 *)&gpio_fun_pin_num, MdbGpioOd},
	{MDB_SYS_VAR_ADDR+9,MDB_SYS_VAR_ADDR+9, (uint8 *)&gpio_fun_pin_num, MdbGpioMux},
	{MDB_SYS_VAR_ADDR+10,MDB_SYS_VAR_ADDR+10, (uint8 *)NULL, MdbAdc},
	{MDB_SYS_VAR_ADDR+11,MDB_SYS_VAR_ADDR+11, (uint8 *)NULL, MdbVcc},
	{MDB_SYS_VAR_ADDR+12,MDB_SYS_VAR_ADDR+12, (uint8 *)&arg_funcs, MdbWordRW},
	{MDB_SYS_VAR_ADDR+13,MDB_SYS_VAR_ADDR+13, (uint8 *)&arg_funcs, MdbUserFunc},
#ifdef USE_SNTP
	{MDB_SYS_VAR_ADDR+14,MDB_SYS_VAR_ADDR+14, (uint8 *)&lock_sntp_time, MdbGetSntpTimeLw},
	{MDB_SYS_VAR_ADDR+15,MDB_SYS_VAR_ADDR+15, ((uint8 *)&lock_sntp_time) + 2, MdbWordR},
#else
	{MDB_SYS_VAR_ADDR+14,MDB_SYS_VAR_ADDR+15, NULL, NULL},
#endif
	{MDB_SYS_VAR_ADDR+16,MDB_SYS_VAR_ADDR+16, (uint8 *)&mdb_mactime, MdbMacTime}, // lo 16 бит от MacTime 64 бита - счетчик со старта в us
	{MDB_SYS_VAR_ADDR+17,MDB_SYS_VAR_ADDR+19, ((uint8 *)&mdb_mactime) + 2, MdbWordR}, // hi 48 бит MacTime - счетчик со старта в us
#ifdef USE_RS485DRV
	{MDB_SYS_VAR_ADDR+20,MDB_SYS_VAR_ADDR+99, NULL, MdbOut55AA}, // вывод заполнителя
	{MDB_SYS_VAR_ADDR+100,MDB_SYS_VAR_ADDR+105, (uint8 *)&rs485cfg.baud, MdbWordRW}, // конфигурация RS-485
	{MDB_SYS_VAR_ADDR+106,MDB_SYS_VAR_ADDR+106, (uint8 *)&rs485cfg, MdbSetCfg},
	{MDB_SYS_VAR_ADDR+107,MDB_SYS_VAR_ADDR+199, NULL, MdbOut55AA}, // с адреса 1207 до 1300 отображать 0x55AA (резерв) и запись запрещена.
#else
	{MDB_SYS_VAR_ADDR+20,MDB_SYS_VAR_ADDR+199, NULL, MdbOut55AA}, // с адреса 1207 до 1300 отображать 0x55AA (резерв) и запись запрещена.
#endif
	{MDB_SYS_VAR_ADDR+200,MDB_SYS_VAR_ADDR+455, (uint8 *)(&RTC_MEM_BASE[64]), MdbWordRW}, // // с адреса 1300 до 1555 блок в 256 слов (16 бит) RTC памяти
	{MDB_SYS_VAR_ADDR+456,0x7FFF, NULL, NULL}, // с адреса 1300 до 32767 отображать нули и запись запрещена.
	{0xffff,0xffff,0,0}
};

bool ICACHE_FLASH_ATTR mbd_fini(uint8 * cfile)
{
	WEBFS_HANDLE fp = WEBFSOpen(cfile);
#if DEBUGSOO > 1
	os_printf("of%d[%s] ", fp, cfile);
#endif
	if(fp != WEBFS_INVALID_HANDLE) {
		if((fatCache.flags & WEBFS_FLAG_ISZIPPED) != 0 // файл сжат!
		|| fatCache.len > sizeof(mdb_buf)) {
				WEBFSClose(fp);
#if DEBUGSOO > 1
			os_printf("%s - error!\n", cfile);
#endif
			return false;
		}
	}
	else { // File not found
#if DEBUGSOO > 1
			os_printf("%s - not found.\n", cfile);
#endif
	    return false;
	};
	WEBFSGetArray(fp,(uint8 *)&mdb_buf, fatCache.len);
#if DEBUGSOO > 1
	os_printf("%s - loaded.\n", cfile);
#endif
	WEBFSClose(fp);
	return true;
}
#ifdef USE_RS485DRV
/* -------------------------------------------------------------------------
 * ------------------------------------------------------------------------- */
os_timer_t timertrn;	// таймер
/* -------------------------------------------------------------------------
 * ------------------------------------------------------------------------- */
void mdb_timertrn_isr(smdbtrn * trn)
{
	if(rs485vars.status != RS485_TX_RX_OFF /* && rs485cfg.flg.b.master != 0 */) { // пусть и в slave работает :)
		uint32 i;
		for(i = 0; i < MDB_TRN_MAX; i++) {
			if(trn->start_flg != 0) { // запустил кто-то ?
				if(mdb_start_trn(i)) {
					trn->start_flg = 0;
//					trn->timer_cnt = trn->timer_set;
				}
			}
			else if(trn->timer_set != 0) { // установлен на таймер?
				if(trn->fifo_cnt == 0) { // свободен?
					if(trn->timer_cnt == 0) {
						mdb_start_trn(i);
						trn->timer_cnt = trn->timer_set;
					}
					trn->timer_cnt--;
				}
//				else trn->timer_cnt = 0;
			}
			trn++;
		}
	}
}
/* -------------------------------------------------------------------------
 * ------------------------------------------------------------------------- */
void ICACHE_FLASH_ATTR init_mdbtab(void)
{
	ets_timer_disarm(&timertrn);
	ets_timer_setfn(&timertrn, (os_timer_func_t *)mdb_timertrn_isr, (void *)&mdb_buf.trn[0]);
	ets_timer_arm_new(&timertrn, 50, 1, 1);
}
#endif // USE_RS485DRV

#endif // USE_MODBUS
