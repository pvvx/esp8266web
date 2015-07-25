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
#include "modbusrtu.h"

extern smdbtabaddr mdbtabaddr[]; // таблица переменных ModBus

void ICACHE_FLASH_ATTR Swapws(uint16 *bufw, uint32 lenw) 
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

void ICACHE_FLASH_ATTR SetMdbErr(smdbadu * mdbbuf, uint32 err) 
{
	if (mdbbuf->id != 0) {
		mdbbuf->fx.fun |= 0x80;
		mdbbuf->err.exc = (uint8) err;
		mdbiosize = 3;
	} else
		mdbiosize = 0;
}

uint32 ICACHE_FLASH_ATTR RdMdbData(smdbadu * mdbbuf, uint32 addr, uint32 len) 
{
	if (mdbbuf->id != 0) {
//          if(len > 125) return MDBERRDATA;
		mdbiosize = (len << 1) + 3; // полный размер ADU
		mdbbuf->out.cnt = (uint8) (len << 1); // размер блока данных в байтах
		uint8 * wbuf;
		wbuf = (uint8 *) mdbbuf->out.data; // указатель на данные ответа mdb (для записи)
		smdbtabaddr * ptr;
		ptr = (smdbtabaddr *) &mdbtabaddr[0]; // указатель на первую структуру в таблице
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
		Swapws(&mdbbuf->out.data[0], mdbbuf->out.cnt >> 1);
	} else
		mdbiosize = 0;
	return MDBERRNO;
}

uint16 ICACHE_FLASH_ATTR WrMdbData(uint8 * dbuf, uint16 addr, uint32 len) 
{
	smdbtabaddr * ptr;
	ptr = (smdbtabaddr *) &mdbtabaddr[0]; // указатель на первую структуру в таблице
	do {
		while (addr > ptr->addre) ptr++;
		if (ptr->addrs == 0xFFFF) return MDBERRADDR; // не найден
		if (addr < ptr->addrs) return MDBERRADDR; // не найден
		uint32 i = ptr->addre - addr + 1; // размерчик пересылки в этом блоке
		if (len < i) i = len; // больше чем запрос? -> ограничить
		uint8 * wbuf; // адрес в буфере записи (или адрес в блоке, если в таблице buf=null)
        wbuf = &ptr->buf[(addr - ptr->addrs) << 1]; // расчитать указатель на данные
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
	switch (mdbbuf->fx.fun) {
	case 03: // Read Holding Registers 40000
	case 04: // Read Input Registers 30000
		if (len < sizeof(mdbbuf->f3f4))	break;
		Swapws(&mdbbuf->f3f4.addr, 2);
		if (mdbbuf->f3f4.len > 125 
		 || mdbbuf->f3f4.len == 0
		/*||(mdbbuf->f3f4.addr > MDBMAXADDR4X)*/) {
			SetMdbErr(mdbbuf, MDBERRADDR);
			break;
		}
        if((i=RdMdbData(mdbbuf, mdbbuf->f3f4.addr, mdbbuf->f3f4.len))!=MDBERRNO)
        {
           SetMdbErr(mdbbuf,i);
           break;
        };
		break;
	case 06: // Write Single Register 40000
		if (len < sizeof(mdbbuf->f6)) break;
		Swapws(&mdbbuf->f6.addr, 2);
		/*if(mdbbuf->f6.addr > MDBMAXADDR4X)
		 {
		 SetMdbErr(mdbbuf,MDBERRADDR);
		 break;
		 };*/
		if ((i = WrMdbData((uint8 *) &mdbbuf->f6.data, mdbbuf->f6.addr, 1))
				!= MDBERRNO) {
			SetMdbErr(mdbbuf, i);
			break;
		}
		if (mdbbuf->id != 0) {
			mdbiosize = 6;
			Swapws(&mdbbuf->f6.addr, 2);
		}
		else mdbiosize = 0;
		break;
	case 16: // Write Multiple Registers 40000
		if (len < mdbbuf->f16.cnt + 7) break;
		Swapws(&mdbbuf->f16.addr, 2);
		if ((mdbbuf->f16.len > 123) 
		 || (mdbbuf->f16.len == 0)
		 || (mdbbuf->f16.cnt != (mdbbuf->f16.len << 1))
		/*||(mdbbuf->f16.addr > MDBMAXADDR4X)*/) {
			SetMdbErr(mdbbuf, MDBERRADDR);
			break;
		}
		Swapws(mdbbuf->f16.data, mdbbuf->f16.len);
		if ((i = WrMdbData((uint8 *) mdbbuf->f16.data, mdbbuf->f16.addr,
				mdbbuf->f16.len)) != MDBERRNO) {
			SetMdbErr(mdbbuf, i);
			break;
		}
		Swapws(&mdbbuf->f16.addr, 2);
		mdbiosize = 6;
		break;
	case 23: // Read/Write Multiple Registers 40000
		if (len < mdbbuf->f23.cnt + 11) break;
		Swapws(&mdbbuf->f23.raddr, 4);
		if ((mdbbuf->f23.rlen > 125) 
		 || (mdbbuf->f23.rlen == 0)
		 || (mdbbuf->f23.cnt != (mdbbuf->f23.wlen << 1))
		 || (mdbbuf->f23.wlen > 121) 
		 || (mdbbuf->f23.wlen == 0)
		/*||(mdbbuf->f23.raddr > MDBMAXADDR4X)
		 ||(mdbbuf->f23.waddr > MDBMAXADDR4X)*/) {
			SetMdbErr(mdbbuf, MDBERRADDR);
			break;
		}
		Swapws(mdbbuf->f23.data, mdbbuf->f23.wlen);
		if (((i = WrMdbData((uint8 *) mdbbuf->f23.data, mdbbuf->f23.waddr,
				mdbbuf->f23.wlen)) != MDBERRNO)
				|| ((i = RdMdbData(mdbbuf, mdbbuf->f23.raddr, mdbbuf->f23.rlen))
						!= MDBERRNO)) {
			SetMdbErr(mdbbuf, i);
			break;
		}
//         Swapws(mdbbuf->out.data,mdbbuf->out.cnt>>1);
		break;
		/*     case 08: // Diagnostics (Serial Line only) 08
		 switch(mdbbuf->f8.subf)
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
		 if(mdbbuf->id == 0) mdbiosize = 0;
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
