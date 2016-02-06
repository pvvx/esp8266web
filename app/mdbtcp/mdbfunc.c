/******************************************************************************
 * FileName: MdbFunc.c 
 * ModBus RTU
 * Author: PV`
 * (c) PV` 2015
*******************************************************************************/
#include "user_config.h"
#ifdef USE_MODBUS
#include "ets_sys.h"
#include "os_type.h"
#include "osapi.h"
#include "bios.h"
#include "sdk/rom2ram.h"
#include "modbusrtu.h"

extern smdbtabaddr mdbtabaddr[]; // таблица переменных ModBus

void Swapws(uint8 *bufw, uint32 lenw)
{
	uint8 *p = (uint8 *)bufw;
	while(lenw--) {
		register uint32 x = *p;
		*p = p[1];
		p[1] = x;
		p += 2;
	}
}

uint32 mdbiosize DATA_IRAM_ATTR; // размер принятого и отправляемого сообщений ModBus

uint32 MdbWordR(uint8 * mdb, uint8 * buf, uint32 rwflg)
{
	if (rwflg & 0x10000) return MDBERRDATA; // Запись?
	copy_s4d1(mdb,(void *) buf, rwflg << 1);
	return MDBERRNO;
}

uint32 MdbWordRW(uint8 * mdb, uint8 * buf, uint32 rwflg)
{
#if DEBUGSOO > 5
	os_printf("MdbWordRW(%p, %p, %u)\n", mdb, buf, rwflg);
#endif
	if (rwflg & 0x10000) {	// Запись?
		rwflg &= 0x7FFF;
		copy_s1d4((void *) buf, mdb, rwflg << 1);
	}
	copy_s4d1(mdb,(void *) buf, rwflg << 1);
	return MDBERRNO;
}
/*
uint32 ICACHE_FLASH_ATTR MdbWordRW(uint8 * mdb, uint8 * buf, uint32 rwflg) 
{
	if (rwflg & 0x10000) {	// Запись?
		rwflg &= 0xFFFF;
		while(rwflg--) {
			*buf++ = *mdb++;
			*buf++ = *mdb++;
      	}
	} else {
		while(rwflg--) {
			*mdb++ = *buf++;
			*mdb++ = *buf++;
      	}
	}
	return MDBERRNO;
}

uint32 ICACHE_FLASH_ATTR MdbWordR(uint8 * mdb, uint8 * buf, uint32 rwflg) 
{
	if (rwflg & 0x10000) return MDBERRDATA; // Запись?
	else {
		while(rwflg--) {
           *mdb++ = *buf++;
      	   *mdb++ = *buf++;
      	}
	}
	return MDBERRNO;
}
*/
uint32 ICACHE_FLASH_ATTR SetMdbErr(smdbadu * mdbbuf, uint32 err)
{
	if (mdbbuf->id != 0) {
		mdbbuf->fx.hd.fun |= 0x80;
		mdbbuf->err.exc = (uint8) err;
		mdbiosize = 3;
	} else
		mdbiosize = 0;
	return mdbiosize;
}

uint32 ICACHE_FLASH_ATTR RdMdbData(uint8 * wbuf, uint16 addr, uint32 len)
{
#if DEBUGSOO > 3
		os_printf("mdbrd:%u[%u]\n", addr, len);
#endif
	smdbtabaddr * ptr = (smdbtabaddr *) &mdbtabaddr[0]; // указатель на первую структуру в таблице
	do {
		while (addr > ptr->addre) ptr++;
		if (ptr->addrs == 0xFFFF) return MDBERRADDR; // не найден
		if (addr < ptr->addrs)	return MDBERRADDR; // не найден
		uint32 i = ptr->addre - addr + 1; // размерчик пересылки в этом блоке
		if (len < i) i = len; // больше чем запрос? -> ограничить
		uint8 * rbuf; // входной буфер (или адрес в блоке, если в таблице buf=null)
        rbuf = &ptr->buf[(addr - ptr->addrs)<<1]; // расчитать указатель на данные
		uint32 x;
		if (ptr->func) {
			if ((x = ptr->func(wbuf, rbuf, i)) != MDBERRNO) return x;
		} else {	// Если NULL -> функция не вызывается
			if (ptr->buf) { // данные передаются из блока приема?
				if ((x = MdbWordRW(wbuf, rbuf, i)) != MDBERRNO)	return x;
			}
			else {
				x = i;
				uint8 * p = wbuf;
				while(x--) {
		           *p++ = 0;
		      	   *p++ = 0;
		      	}
					os_memset(wbuf, 0, i << 1); // передать нули
			}
		};
		len -= i; // вычесть кол-во данных этой пересылки
		addr += i;  // шаг адреса
		wbuf += i << 1; // шаг в блоке выходных данных ответа mdb
		ptr++; // на следюший указатель в таблице
	} while (len);
	return MDBERRNO;
}

uint32 ICACHE_FLASH_ATTR ReadMdbData(smdbadu * mdbbuf, uint32 addr, uint32 len)
{
	if (mdbbuf->id != 0) {
//          if(len > 125) return MDBERRDATA;
		mdbiosize = (len << 1) + 3; // полный размер ADU
		mdbbuf->o3o4.hd.cnt = (uint8) (len << 1); // размер блока данных в байтах
		uint32 err = RdMdbData((uint8 *)&mdbbuf->o3o4.data, addr, len);
		if(err != MDBERRNO) return err;
		Swapws((uint8 *)mdbbuf->o3o4.data, mdbbuf->o3o4.hd.cnt >> 1);
	} else
		mdbiosize = 0;
	return MDBERRNO;
}

uint32 ICACHE_FLASH_ATTR WrMdbData(uint8 * dbuf, uint16 addr, uint32 len)
{
#if DEBUGSOO > 3
		os_printf("mdbwr:%u[%u]\n", addr, len);
#endif
	smdbtabaddr * ptr = (smdbtabaddr *) &mdbtabaddr[0]; // указатель на первую структуру в таблице
	do {
		while (addr > ptr->addre) ptr++;
		if (ptr->addrs == 0xFFFF) return MDBERRADDR; // не найден
		if (addr < ptr->addrs) return MDBERRADDR; // не найден
		uint32 i = ptr->addre - addr + 1; // размерчик пересылки в этом блоке
		if (len < i) i = len; // больше чем запрос? -> ограничить
		uint8 * wbuf = &ptr->buf[(addr - ptr->addrs) << 1]; // расчитать указатель на данные
		if (ptr->func) {
			uint32 x;
			if ((x = ptr->func(dbuf, wbuf, i | 0x10000)) != MDBERRNO)
				return x;
		};
		len -= i; // вычесть кол-во данных этой пересылки
		addr += i;  // шаг адреса
		dbuf += i << 1; // шаг в блоке данных запроса mdb
		ptr++; // на следюший указатель в таблице
	} while (len);
	return MDBERRNO;
}

uint32 ICACHE_FLASH_ATTR MdbFunc(smdbadu * mdbbuf, uint32 len) 
{
	uint32 i;
	mdbiosize = 0;
	switch (mdbbuf->fx.hd.fun) {
	case 03: // Read Holding Registers 40000
	case 04: // Read Input Registers 30000
		if (len < sizeof(mdbbuf->f3f4))	break;
		Swapws((uint8 *)&mdbbuf->f3f4.hd.addr, 2);
		if (mdbbuf->f3f4.hd.len > MDB_ADU_F3F4_DATA_MAX
		 || mdbbuf->f3f4.hd.len == 0
		/*||(mdbbuf->f3f4.addr > MDBMAXADDR4X)*/) {
			SetMdbErr(mdbbuf, MDBERRADDR);
			break;
		}
        if((i=ReadMdbData(mdbbuf, mdbbuf->f3f4.hd.addr, mdbbuf->f3f4.hd.len)) != MDBERRNO)
        {
           SetMdbErr(mdbbuf,i);
           break;
        };
		break;
	case 06: // Write Single Register 40000
		if (len < sizeof(mdbbuf->f6)) break;
		Swapws((uint8 *)&mdbbuf->f6.hd.addr, 2);
		/*if(mdbbuf->f6.addr > MDBMAXADDR4X)
		 {
		 SetMdbErr(mdbbuf,MDBERRADDR);
		 break;
		 };*/
		if ((i = WrMdbData((uint8 *) &mdbbuf->f6.data, mdbbuf->f6.hd.addr, 1))	!= MDBERRNO) {
			SetMdbErr(mdbbuf, i);
			break;
		}
		if (mdbbuf->id != 0) {
			mdbiosize = 6;
			Swapws((uint8 *)&mdbbuf->f6.hd.addr, 2);
		}
		else mdbiosize = 0;
		break;
	case 16: // Write Multiple Registers 40000
		if (len < mdbbuf->f16.hd.cnt + sizeof(mdbbuf->f16.hd)) break;
		Swapws((uint8 *)&mdbbuf->f16.hd.addr, 2);
		if ((mdbbuf->f16.hd.len > (sizeof(mdbbuf->f16.data)>>1))
		 || (mdbbuf->f16.hd.len == 0)
		 || (mdbbuf->f16.hd.cnt != (mdbbuf->f16.hd.len << 1))
		/*||(mdbbuf->f16.hd.addr > MDBMAXADDR4X)*/) {
			SetMdbErr(mdbbuf, MDBERRADDR);
			break;
		}
		Swapws((uint8 *)mdbbuf->f16.data, mdbbuf->f16.hd.len);
		if ((i = WrMdbData((uint8 *) mdbbuf->f16.data, mdbbuf->f16.hd.addr,
				mdbbuf->f16.hd.len)) != MDBERRNO) {
			SetMdbErr(mdbbuf, i);
			break;
		}
		Swapws((uint8 *)&mdbbuf->f16.hd.addr, 2);
		if (mdbbuf->id != 0) mdbiosize = 6;
		else mdbiosize = 0;
		break;
	case 23: // Read/Write Multiple Registers 40000
		if (len < mdbbuf->f23.hd.cnt + sizeof(mdbbuf->f23.hd)) break;
		Swapws((uint8 *)&mdbbuf->f23.hd.raddr, 4);
		if ((mdbbuf->f23.hd.rlen > (sizeof(mdbbuf->f23.data)>>1))
		 || (mdbbuf->f23.hd.rlen == 0)
		 || (mdbbuf->f23.hd.cnt != (mdbbuf->f23.hd.wlen << 1))
		 || (mdbbuf->f23.hd.wlen > sizeof(mdbbuf->f23.data))
		 || (mdbbuf->f23.hd.wlen == 0)
		/*||(mdbbuf->f23.hd.raddr > MDBMAXADDR4X)
		 ||(mdbbuf->f23.hd.waddr > MDBMAXADDR4X)*/) {
			SetMdbErr(mdbbuf, MDBERRADDR);
			break;
		}
		Swapws((uint8 *)mdbbuf->f23.data, mdbbuf->f23.hd.wlen);
		if (((i = WrMdbData((uint8 *) mdbbuf->f23.data, mdbbuf->f23.hd.waddr, 	mdbbuf->f23.hd.wlen)) != MDBERRNO)
		 || ((i = ReadMdbData(mdbbuf, mdbbuf->f23.hd.raddr, mdbbuf->f23.hd.rlen)) != MDBERRNO)) {
			SetMdbErr(mdbbuf, i);
			break;
		}
//         Swapws((uint8 *)mdbbuf->out.data,mdbbuf->out.cnt>>1);
		break;
		/*     case 08: // Diagnostics (Serial Line only) 08
		 switch(mdbbuf->f8.hd.subf)
		 {
		 case 00: // 00 Return Query Data
		 mdbiosize = 8;
		 break;
		 case 01: // 01 Restart Communications Option
		 mdbiosize = 8;
		 break;
		 case 02: // 02 Return Diagnostic Register
		 case 03: // 03 Change ASCII Input Delimiter
		 case 04: // 04 Force Listen Only Mode
		 case 10: // 10 Clear Counters and Diagnostic Register
		 case 11: // 11 Return Bus Message Count
		 case 12: // 12 Return Bus Communication Error Count
		 case 13: // 13 Return Bus Exception Error Count
		 case 14: // 14 Return Slave Message Count
		 case 15: // 15 Return Slave No Response Count
		 case 16: // 16 Return Slave NAK Count
		 case 17: // 17 Return Slave Busy Count
		 case 18: // 18 Return Bus Character Overrun Count
		 case 20: // 20 Clear Overrun Counter and Flag
		 mdbbuf->f08.data = 0;
		 mdbiosize = 8;
		 break;
		 default:
		 SetMdbErr(mdbbuf,MDBERRFUNC);
		 break;
		 };
		 if(mdbbuf->hd.id == 0) mdbiosize = 0;
		 break;
		 case 20: // 20 (0x14) Read File Record
		 case 21: // 21 (0x15) Write File Record
		 SetMdbErr(MDBERRFUNC);
		 break; */
	default:
		SetMdbErr(mdbbuf, MDBERRFUNC);
		break;
	};
	return (mdbiosize);
}

#endif // USE_MODBUS
