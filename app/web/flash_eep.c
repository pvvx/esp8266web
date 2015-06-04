/*
 * flash_eep.c
 *
 *  Created on: 19/01/2015
 *      Author: PV`
 */

#include "user_config.h"
#include "bios/ets.h"
//#include "add_sdk_func.h"
#include "ets_sys.h"
#include "os_type.h"
#include "osapi.h"
#include "flash.h"
#include "flash_eep.h"

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
		if(spi_flash_read(faddr, &x2, 4) != SPI_FLASH_RESULT_OK) return 1;
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
		if(spi_flash_write(reta, &x1, 4) != SPI_FLASH_RESULT_OK)	return 1;
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
		if(spi_flash_read(faddr,(uint32 *) &fobj, fobj_head_size) != SPI_FLASH_RESULT_OK) {
			return 1;
		}
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
		if(spi_flash_read(faddr, (uint32 *) &fobj, fobj_head_size) != SPI_FLASH_RESULT_OK) {
			return 1; // ошибка
		}
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
// FunctionName : spi_flash_save_struct
// Returns      : false/true
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
	if(spi_flash_erase_sector(fnewseg>>12) != SPI_FLASH_RESULT_OK) return 1;
	faddr += 4;
	do {
		if(spi_flash_read(faddr, (uint32 *) &fobj, fobj_head_size) != SPI_FLASH_RESULT_OK) return 1; // последовательное чтение id из старого сегмента
		if(fobj.x == fobj_x_free) break;
		if(fobj.n.size > MAX_FOBJ_SIZE) len = align(MAX_FOBJ_SIZE + fobj_head_size);
		else len = align(fobj.n.size + fobj_head_size);
		if(fobj.n.id != obj.n.id &&  fobj.n.size <= MAX_FOBJ_SIZE) { // объект валидный
			if(get_addr_fobj(fnewseg, &fobj, true) == 0) { // найдем, сохранили ли мы его уже? нет
				xaddr = get_addr_fobj(foldseg, &fobj, false); // найдем последнее сохранение объекта в старом сенгменте, size изменен
				if(xaddr < 4) return xaddr; // ???
				// прочитаем заголовок с телом объекта в буфер
				os_memcpy(buf, &fobj, fobj_head_size);
				if((spi_flash_read(xaddr + fobj_head_size,(uint32 *) &buf[fobj_head_size], fobj.n.size) != SPI_FLASH_RESULT_OK)) return 1;
				xaddr = get_addr_fobj_save(fnewseg, fobj); // адрес для записи объекта
				if(xaddr < 4) return xaddr; // ???
				// запишем заголовок с телом объекта во flash
				if((spi_flash_write(xaddr, (uint32 *)buf, align(fobj.n.size + fobj_head_size)) != SPI_FLASH_RESULT_OK)) return 1;
			};
		};
		faddr += len;
	} while(faddr  < (foldseg + FMEMORY_SCFG_BANK_SIZE - align(fobj_head_size+1)));

	if(spi_flash_read(foldseg, &xaddr, 4) != SPI_FLASH_RESULT_OK)	return 1;
	xaddr--;
	if(spi_flash_write(fnewseg, &xaddr, 4) != SPI_FLASH_RESULT_OK)	return 1;
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
			if((size)&&(spi_flash_read(xfaddr, (uint32 *) buf, size + fobj_head_size) != SPI_FLASH_RESULT_OK)) return false; // error
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
	spi_flash_write(faddr, (uint32 *)buf, len);
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
	if((maxsize)&&(spi_flash_read(faddr + fobj_head_size, ptr, maxsize) != SPI_FLASH_RESULT_OK)) return -2; // error
#if DEBUGSOO > 2
		os_printf("ok,size:%u ", fobj.n.size);
#endif
	return fobj.n.size;
}
//=============================================================================
//-- Сохранение системных настроек -------------------------------------------
//=============================================================================
struct SystemCfg syscfg;
//-----------------------------------------------------------------------------
// Чтение системных настроек
//-----------------------------------------------------------------------------
bool ICACHE_FLASH_ATTR sys_read_cfg(void) {
	if(flash_read_cfg(&syscfg, ID_CFG_SYS, sizeof(syscfg)) != sizeof(syscfg)) {
		syscfg.cfg.w = 0
				| SYS_CFG_PIN_CLR_ENA
#ifdef 	USE_CPU_SPEED
				| SYS_CFG_HI_SPEED
#endif
#if DEBUGSOO > 0
				| SYS_CFG_DEBUG_ENA
#endif
#ifdef USE_NETBIOS
				| SYS_CFG_NETBIOS_ENA
#endif
#ifdef USE_SNTP
				| SYS_CFG_SNTP_ENA
#endif
				;
#ifdef TCP2UART_PORT_DEF
		syscfg.tcp2uart_port = TCP2UART_PORT_DEF;
#else
		syscfg.tcp2uart_port = 12345;
#endif
		syscfg.tcp2uart_twrec = 0;
		syscfg.tcp2uart_twcls = 0;
#ifdef USE_SRV_WEB_PORT
		syscfg.web_port = USE_SRV_WEB_PORT;
#else
		syscfg.web_port = 80;
#endif
#ifdef UDP_TEST_PORT
		syscfg.udp_port = UDP_TEST_PORT;
#else
		syscfg.udp_port = 1025;
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
//	set_gpio_io_pin();
	return flash_save_cfg(&syscfg, ID_CFG_SYS, sizeof(syscfg));
}




