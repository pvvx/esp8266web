/*
 * flash_eep.c
 *
 *  Created on: 19/01/2015
 *      Author: PV`
 */

#include "user_config.h"
#include "bios/ets.h"
#include "sdk/mem_manager.h"
#include "ets_sys.h"
#include "os_type.h"
#include "osapi.h"
#include "sdk/flash.h"
#include "flash_eep.h"
#include "hw/esp8266.h"
//#ifdef USE_WEB
#include "web_srv.h"
//#endif
#ifdef USE_TCP2UART
#include "tcp2uart.h"
#endif
#ifdef USE_NETBIOS
#include "netbios.h"
#endif
#ifdef USE_MODBUS
#include "modbustcp.h"
#endif

//-----------------------------------------------------------------------------

#define mMIN(a, b)  ((a<b)?a:b)

typedef union // заголовок объекта сохранения
{
	struct {
	uint16 size;
	uint16 id;
	} __attribute__((packed)) n;
	uint32 x;
} __attribute__((packed)) fobj_head;

#define fobj_head_size 4
#define fobj_x_free 0xffffffff
#define MAX_FOBJ_SIZE 512 // максимальный размер сохраняемых объeктов

//-----------------------------------------------------------------------------

#define align(a) ((a + 3) & 0xFFFFFFFC)

//-----------------------------------------------------------------------------
#if ((FMEMORY_SCFG_BASE_ADDR + (FMEMORY_SCFG_BANK_SIZE*FMEMORY_SCFG_BANKS)) < FLASH_CACHE_MAX_SIZE)
#include "sdk/rom2ram.h"
#define flash_read(a, d, s) (copy_s4d1((unsigned char *)(d), (void *)((a) + FLASH_BASE), s) != 0)
#else
#define flash_read(a, d, s) (spi_flash_read(a, (uint32 *)(d), s) != SPI_FLASH_RESULT_OK)
#endif
#define flash_write(a, d, s) (spi_flash_write(a, (uint32 *)(d), s) != SPI_FLASH_RESULT_OK)
#define flash_erase_sector(a) (spi_flash_erase_sector(a>>12) != SPI_FLASH_RESULT_OK)
//-----------------------------------------------------------------------------
// FunctionName : get_addr_bscfg
// Опции:
//  fasle - поиск текушего сегмента
//  true - поиск нового сегмента для записи (pack)
// Returns     : новый адрес сегмента для записи
//-----------------------------------------------------------------------------
LOCAL ICACHE_FLASH_ATTR uint32 get_addr_bscfg(bool flg)
{
	uint32 x1 = (flg)? 0 : 0xFFFFFFFF, x2;
	uint32 faddr = FMEMORY_SCFG_BASE_ADDR;
	uint32 reta = FMEMORY_SCFG_BASE_ADDR;
	do {
		if(flash_read(faddr, &x2, 4)) return 1;
		if(flg) { // поиск нового сегмента для записи (pack)
			if(x2 > x1 || x2 == 0xFFFFFFFF) {
				x1 = x2;
				reta = faddr; // новый адрес сегмента для записи
			}
		}
		else if(x2 < x1) { // поиск текушего сегмента
			x1 = x2;
			reta = faddr; // новый адрес сегмента для записи
		};
		faddr += FMEMORY_SCFG_BANK_SIZE;
	} while(faddr < (FMEMORY_SCFG_BASE_ADDR + FMEMORY_SCFG_BANKS * FMEMORY_SCFG_BANK_SIZE));

	if((!flg)&&(x1 == 0xFFFFFFFF)&&(reta == FMEMORY_SCFG_BASE_ADDR)) {
		x1--;
		if(flash_write(reta, &x1, 4)) return 1;
	}
#if DEBUGSOO > 3
		if(flg) os_printf("bsseg:%p ", reta);
		else os_printf("brseg:%p ", reta);
#endif
	return reta;
}
//-----------------------------------------------------------------------------
// FunctionName : get_addr_fobj
// Опции:
//  false - Поиск последней записи объекта по id и size
//  true - Поиск присуствия записи объекта по id и size
// Returns : адрес записи данных объекта
// 0 - не найден
//-----------------------------------------------------------------------------
LOCAL ICACHE_FLASH_ATTR uint32 get_addr_fobj(uint32 base, fobj_head *obj, bool flg)
{
//	if(base == 0) return 0;
	fobj_head fobj;
	uint32 faddr = base + 4;
	uint32 fend = base + FMEMORY_SCFG_BANK_SIZE - align(fobj_head_size);
	uint32 reta = 0;
	do {
		if(flash_read(faddr, &fobj, fobj_head_size)) return 1;
		if(fobj.x == fobj_x_free) break;
		if(fobj.n.size <= MAX_FOBJ_SIZE) {
			if(fobj.n.id == obj->n.id) {
				if(flg) {
					return faddr;
				}
				obj->n.size = fobj.n.size;
				reta = faddr;
			}
			faddr += align(fobj.n.size + fobj_head_size);
		}
		else faddr += align(MAX_FOBJ_SIZE + fobj_head_size);
	}
	while(faddr < fend);
	return reta;
}
//-----------------------------------------------------------------------------
// FunctionName : get_addr_fend
// Поиск последнего адреса в сегменте для записи объекта
// Returns : адрес для записи объекта
//-----------------------------------------------------------------------------
LOCAL ICACHE_FLASH_ATTR uint32 get_addr_fobj_save(uint32 base, fobj_head obj)
{
	fobj_head fobj;
	uint32 faddr = base + 4;
	uint32 fend = base + FMEMORY_SCFG_BANK_SIZE - align(obj.n.size + fobj_head_size);
	do {
		if(flash_read(faddr, &fobj, fobj_head_size)) return 1; // ошибка
		if(fobj.x == fobj_x_free) {
			if(faddr < fend) {
				return faddr;
			}
			return 0; // не влезет, на pack
		}
		if(fobj.n.size <= MAX_FOBJ_SIZE) {
			faddr += align(fobj.n.size + fobj_head_size);
		}
		else faddr += align(MAX_FOBJ_SIZE + fobj_head_size);
	}
	while(faddr < fend);
	return 0; // не влезет, на pack
}
//=============================================================================
// FunctionName : pack_cfg_fmem
// Returns      : адрес для записи объекта
//-----------------------------------------------------------------------------
LOCAL ICACHE_FLASH_ATTR uint32 pack_cfg_fmem(fobj_head obj)
{
	fobj_head fobj;
	uint8 buf[align(MAX_FOBJ_SIZE + fobj_head_size)];
	uint32 fnewseg = get_addr_bscfg(true); // поиск нового сегмента для записи (pack)
	if(fnewseg < 4) return fnewseg; // error
	uint32 foldseg = get_addr_bscfg(false); // поиск текушего сегмента
	if(foldseg < 4) return fnewseg; // error
	uint32 faddr = foldseg;
	uint32 xaddr;
	uint16 len;
	if(flash_erase_sector(fnewseg)) return 1;
	faddr += 4;
	do {
		if(flash_read(faddr, &fobj, fobj_head_size)) return 1; // последовательное чтение id из старого сегмента
		if(fobj.x == fobj_x_free) break;
		if(fobj.n.size > MAX_FOBJ_SIZE) len = align(MAX_FOBJ_SIZE + fobj_head_size);
		else len = align(fobj.n.size + fobj_head_size);
		if(fobj.n.id != obj.n.id &&  fobj.n.size <= MAX_FOBJ_SIZE) { // объект валидный
			if(get_addr_fobj(fnewseg, &fobj, true) == 0) { // найдем, сохранили ли мы его уже? нет
				xaddr = get_addr_fobj(foldseg, &fobj, false); // найдем последнее сохранение объекта в старом сенгменте, size изменен
				if(xaddr < 4) return xaddr; // ???
				// прочитаем заголовок с телом объекта в буфер
				os_memcpy(buf, &fobj, fobj_head_size);
				if(flash_read(xaddr + fobj_head_size, &buf[fobj_head_size], fobj.n.size)) return 1;
				xaddr = get_addr_fobj_save(fnewseg, fobj); // адрес для записи объекта
				if(xaddr < 4) return xaddr; // ???
				// запишем заголовок с телом объекта во flash
				if(flash_write(xaddr, buf, align(fobj.n.size + fobj_head_size))) return 1;
			};
		};
		faddr += len;
	} while(faddr  < (foldseg + FMEMORY_SCFG_BANK_SIZE - align(fobj_head_size+1)));

	if(flash_read(foldseg, &xaddr, 4))	return 1;
	xaddr--;
	if(flash_write(fnewseg, &xaddr, 4))	return 1;
	return get_addr_fobj_save(fnewseg, obj); // адрес для записи объекта;
}
//=============================================================================
//- Сохранить объект в flash --------------------------------------------------
//  Returns	: false/true
//-----------------------------------------------------------------------------
bool ICACHE_FLASH_ATTR flash_save_cfg(void *ptr, uint16 id, uint16 size)
{
	if(size > MAX_FOBJ_SIZE) return false;
	uint8 buf[align(MAX_FOBJ_SIZE + fobj_head_size)];
	fobj_head fobj;
	fobj.n.id = id;
	fobj.n.size = size;

	uint32 faddr = get_addr_bscfg(false);
	if(faddr < 4) return false;
	{
		uint32 xfaddr = get_addr_fobj(faddr, &fobj, false);
		if(xfaddr > 3 && size == fobj.n.size) {
			if((size)&&(flash_read(xfaddr, buf, size + fobj_head_size))) return false; // error
			if(!os_memcmp(ptr, &buf[fobj_head_size], size)) return true; // уже записано то-же самое
		}
	}
	fobj.n.size = size;
#if DEBUGSOO > 2
	os_printf("save-id:%02x[%u] ", id, size);
#endif
	faddr = get_addr_fobj_save(faddr, fobj);
	if(faddr == 0) {
		faddr = pack_cfg_fmem(fobj);
		if(faddr == 0) return false; // error
	}
	else if(faddr < 4) return false; // error
	uint16 len = align(size + fobj_head_size);
	os_memcpy(buf, &fobj, fobj_head_size);
	os_memcpy(&buf[fobj_head_size], ptr, size);
	if(flash_write(faddr, (uint32 *)buf, len)) return false; // error
#if DEBUGSOO > 2
	os_printf("ok ");
#endif
	return true;
}
//=============================================================================
//- Прочитать объект из flash -------------------------------------------------
//  Параметры:
//   prt - указатель, куда сохранить
//   id - идентификатор искомого объекта
//   maxsize - сколько байт сохранить максимум из найденного объекта, по ptr
//  Returns:
//  -3 - error
//  -2 - flash rd/wr/clr error
//  -1 - не найден
//   0..MAX_FOBJ_SIZE - ok, сохраненный размер объекта
//-----------------------------------------------------------------------------
sint16 ICACHE_FLASH_ATTR flash_read_cfg(void *ptr, uint16 id, uint16 maxsize)
{
	if(maxsize > MAX_FOBJ_SIZE) return -3; // error
	fobj_head fobj;
	fobj.n.id = id;
	fobj.n.size = 0;
#if DEBUGSOO > 2
	os_printf("read-id:%02x[%u] ", id, maxsize);
#endif
	uint32 faddr = get_addr_bscfg(false);
	if(faddr < 4) return -faddr-1;
	faddr = get_addr_fobj(faddr, &fobj, false);
	if(faddr < 4) return -faddr-1;
	maxsize = mMIN(fobj.n.size, maxsize);
	if((maxsize)&&(flash_read(faddr + fobj_head_size, ptr, maxsize))) return -2; // error
#if DEBUGSOO > 2
		os_printf("ok,size:%u ", fobj.n.size);
#endif
	return fobj.n.size;
}
//=============================================================================
//-- Сохранение системных настроек -------------------------------------------
//=============================================================================
struct SystemCfg syscfg;
#if defined(USE_TCP2UART) || defined(USE_MODBUS)
uint8 * tcp_client_url;
#endif
//-----------------------------------------------------------------------------
// Чтение системных настроек
//-----------------------------------------------------------------------------
bool ICACHE_FLASH_ATTR sys_read_cfg(void) {
#if defined(USE_TCP2UART) || defined(USE_MODBUS)
	read_tcp_client_url();
#endif
	if(flash_read_cfg(&syscfg, ID_CFG_SYS, sizeof(syscfg)) != sizeof(syscfg)) {
		syscfg.cfg.w = 0
				| SYS_CFG_PIN_CLR_ENA
#ifdef USE_TCP2UART
				| SYS_CFG_T2U_REOPEN
#endif
#ifdef USE_MODBUS
				| SYS_CFG_MDB_REOPEN
#endif
#ifdef USE_CPU_SPEED
				| SYS_CFG_HI_SPEED
#endif
#if DEBUGSOO > 0
				| SYS_CFG_DEBUG_ENA
#endif
#ifdef USE_NETBIOS
	#if USE_NETBIOS
				| SYS_CFG_NETBIOS_ENA
	#endif
#endif
#ifdef USE_SNTP
	#if USE_SNTP
				| SYS_CFG_SNTP_ENA
	#endif
#endif
#ifdef USE_CAPTDNS
	#if USE_CAPTDNS
				| SYS_CFG_CDNS_ENA
	#endif
#endif
				;
#ifdef USE_TCP2UART
		syscfg.tcp2uart_port = DEFAULT_TCP2UART_PORT;
		syscfg.tcp2uart_twrec = 0;
		syscfg.tcp2uart_twcls = 0;
#endif
		syscfg.tcp_client_twait = 5000;
#ifdef USE_WEB
		syscfg.web_port = DEFAULT_WEB_PORT;
		syscfg.web_twrec = 5;
		syscfg.web_twcls = 5;
#endif
#ifdef USE_MODBUS
		syscfg.mdb_twrec = 10;
		syscfg.mdb_twcls = 10;
		syscfg.mdb_port = DEFAULT_MDB_PORT;	// (=0 - отключен)
		syscfg.mdb_id = DEFAULT_MDB_ID;
#endif
		return false;
	};
//	if(syscfg.web_port == 0) syscfg.cfg.b.pin_clear_cfg_enable = 1;
	return true;
}
//-----------------------------------------------------------------------------
// Сохранение системных настроек
//-----------------------------------------------------------------------------
bool ICACHE_FLASH_ATTR sys_write_cfg(void) {
	return flash_save_cfg(&syscfg, ID_CFG_SYS, sizeof(syscfg));
}

#if defined(USE_TCP2UART) || defined(USE_MODBUS)
//-------------------------------------------------------------------------------
// new_tcp_client_url()
//-------------------------------------------------------------------------------
bool ICACHE_FLASH_ATTR new_tcp_client_url(uint8 *url)
{
	if(tcp_client_url != NULL) {
		if (os_strcmp(tcp_client_url, url) == 0) {
			return false;
		}
		os_free(tcp_client_url);
	}
	uint32 len = os_strlen(url);
	if(len < VarNameSize || len != 0) {
		tcp_client_url = os_zalloc(len+1);
		if(tcp_client_url == NULL) return false;
		os_memcpy(tcp_client_url, url, len);
		if(flash_save_cfg(tcp_client_url, ID_CFG_UURL, len)) return true;
		os_free(tcp_client_url);
	}
	tcp_client_url = NULL;
	return false;
}
//-------------------------------------------------------------------------------
// read_tcp_client_url()
//-------------------------------------------------------------------------------
bool ICACHE_FLASH_ATTR read_tcp_client_url(void)
{
	uint8 url[VarNameSize] = {0};
	uint32 len = flash_read_cfg(&url, ID_CFG_UURL, VarNameSize);
	if(len != 0) {
		if(tcp_client_url != NULL) os_free(tcp_client_url);
		uint32 len = os_strlen(url);
		if(len < VarNameSize || len != 0) {
			tcp_client_url = os_zalloc(len+1);
			if(tcp_client_url == NULL) return false;
			os_memcpy(tcp_client_url, url, len);
			if(flash_save_cfg(tcp_client_url, ID_CFG_UURL, len)) return true;
			os_free(tcp_client_url);
		}
	}
	tcp_client_url = NULL;
	return false;
}
#endif

/*
 *  Чтение пользовательских констант (0 < idx < 4)
 */
uint32 ICACHE_FLASH_ATTR read_user_const(uint8 idx) {
#ifdef USE_FIX_SDK_FLASH_SIZE
	uint32 ret = 0xFFFFFFFF;
	if (idx < MAX_IDX_USER_CONST) {
		if (flash_read_cfg(&ret, ID_CFG_KVDD + idx, 4) != 4) {
			if(idx == 0) ret = 102400; // константа делителя для ReadVDD
		}
	}
	return ret;
#endif
}
/*
 * Запись пользовательских констант (0 < idx < 4)
 */
bool ICACHE_FLASH_ATTR write_user_const(uint8 idx, uint32 data) {
	if (idx >= MAX_IDX_USER_CONST)	return false;
	uint32 ret = data;
	return flash_save_cfg(&ret, ID_CFG_KVDD + idx, 4);
}



