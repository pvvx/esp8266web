/******************************************************************************
 * FileName: MdbRS485.c
 * ModBus RTU RS-485
 * Прием/передача сообщений Modbus к драйверу RS-485
 * Author: PV`
 * (c) PV` 2015
*******************************************************************************/
#include "user_config.h"
#ifdef USE_RS485DRV
#include "bios.h"
#include "sdk/add_func.h"
#include "c_types.h"
#include "osapi.h"
#include "driver/rs485drv.h"
#include "modbusrtu.h"
#include "modbustcp.h"
#include "mdbrs485.h"
#include "flash_eep.h"

extern void Swapws(uint8 *bufw, uint32 lenw);

static const uint16 crc16tabA001[256] = {
   0X0000, 0XC0C1, 0XC181, 0X0140, 0XC301, 0X03C0, 0X0280, 0XC241,
   0XC601, 0X06C0, 0X0780, 0XC741, 0X0500, 0XC5C1, 0XC481, 0X0440,
   0XCC01, 0X0CC0, 0X0D80, 0XCD41, 0X0F00, 0XCFC1, 0XCE81, 0X0E40,
   0X0A00, 0XCAC1, 0XCB81, 0X0B40, 0XC901, 0X09C0, 0X0880, 0XC841,
   0XD801, 0X18C0, 0X1980, 0XD941, 0X1B00, 0XDBC1, 0XDA81, 0X1A40,
   0X1E00, 0XDEC1, 0XDF81, 0X1F40, 0XDD01, 0X1DC0, 0X1C80, 0XDC41,
   0X1400, 0XD4C1, 0XD581, 0X1540, 0XD701, 0X17C0, 0X1680, 0XD641,
   0XD201, 0X12C0, 0X1380, 0XD341, 0X1100, 0XD1C1, 0XD081, 0X1040,
   0XF001, 0X30C0, 0X3180, 0XF141, 0X3300, 0XF3C1, 0XF281, 0X3240,
   0X3600, 0XF6C1, 0XF781, 0X3740, 0XF501, 0X35C0, 0X3480, 0XF441,
   0X3C00, 0XFCC1, 0XFD81, 0X3D40, 0XFF01, 0X3FC0, 0X3E80, 0XFE41,
   0XFA01, 0X3AC0, 0X3B80, 0XFB41, 0X3900, 0XF9C1, 0XF881, 0X3840,
   0X2800, 0XE8C1, 0XE981, 0X2940, 0XEB01, 0X2BC0, 0X2A80, 0XEA41,
   0XEE01, 0X2EC0, 0X2F80, 0XEF41, 0X2D00, 0XEDC1, 0XEC81, 0X2C40,
   0XE401, 0X24C0, 0X2580, 0XE541, 0X2700, 0XE7C1, 0XE681, 0X2640,
   0X2200, 0XE2C1, 0XE381, 0X2340, 0XE101, 0X21C0, 0X2080, 0XE041,
   0XA001, 0X60C0, 0X6180, 0XA141, 0X6300, 0XA3C1, 0XA281, 0X6240,
   0X6600, 0XA6C1, 0XA781, 0X6740, 0XA501, 0X65C0, 0X6480, 0XA441,
   0X6C00, 0XACC1, 0XAD81, 0X6D40, 0XAF01, 0X6FC0, 0X6E80, 0XAE41,
   0XAA01, 0X6AC0, 0X6B80, 0XAB41, 0X6900, 0XA9C1, 0XA881, 0X6840,
   0X7800, 0XB8C1, 0XB981, 0X7940, 0XBB01, 0X7BC0, 0X7A80, 0XBA41,
   0XBE01, 0X7EC0, 0X7F80, 0XBF41, 0X7D00, 0XBDC1, 0XBC81, 0X7C40,
   0XB401, 0X74C0, 0X7580, 0XB541, 0X7700, 0XB7C1, 0XB681, 0X7640,
   0X7200, 0XB2C1, 0XB381, 0X7340, 0XB101, 0X71C0, 0X7080, 0XB041,
   0X5000, 0X90C1, 0X9181, 0X5140, 0X9301, 0X53C0, 0X5280, 0X9241,
   0X9601, 0X56C0, 0X5780, 0X9741, 0X5500, 0X95C1, 0X9481, 0X5440,
   0X9C01, 0X5CC0, 0X5D80, 0X9D41, 0X5F00, 0X9FC1, 0X9E81, 0X5E40,
   0X5A00, 0X9AC1, 0X9B81, 0X5B40, 0X9901, 0X59C0, 0X5880, 0X9841,
   0X8801, 0X48C0, 0X4980, 0X8941, 0X4B00, 0X8BC1, 0X8A81, 0X4A40,
   0X4E00, 0X8EC1, 0X8F81, 0X4F40, 0X8D01, 0X4DC0, 0X4C80, 0X8C41,
   0X4400, 0X84C1, 0X8581, 0X4540, 0X8701, 0X47C0, 0X4680, 0X8641,
   0X8201, 0X42C0, 0X4380, 0X8341, 0X4100, 0X81C1, 0X8081, 0X4040 };

/* -------------------------------------------------------------------------
 * ------------------------------------------------------------------------- */
uint32 CalkCRC16tab(uint8 * blk, uint32 len)
{
	uint32 crc = 0xFFFF;
	while(len--) crc = (crc >> 8) ^ crc16tabA001[(uint8)(*blk++ ^ crc)];
	return (crc);
}
/* -------------------------------------------------------------------------
 * Обработка передачи
 * создание сообщения и передача его в очередь
 * flg_out = true -  поместить в конец очереди, пробовать начинать передачу
 * flg_out = false - не начинать передачу, поместить в начало очереди
 * ------------------------------------------------------------------------- */
struct srs485msg * ICACHE_FLASH_ATTR rs485_new_tx_msg(smdbtcp * p, bool flg_out)
{
	uint32 len = p->mbap.len;
//	if(len > MDB_SER_ADU_SIZE_MAX - 2) return NULL;
	struct srs485msg *msg = os_malloc(len + SIZE_RS485MSG_HEAD + 2);
	if(msg != NULL) {
		os_memcpy(msg->buf, &p->adu.uc[0], len);
		uint32 crc = CalkCRC16tab(msg->buf, len);
		msg->buf[len] = (uint8)crc;
		msg->buf[len+1] = (uint8)(crc >> 8);
		msg->len = len + 2;
		msg->flg.ui = p->mbap.tid; // Transaction Identifier
		if(p->adu.id == 0 || (!flg_out)) msg->flg.ui |= RS485MSG_FLG_TX_ONLY;
   		if(flg_out) {
			// поместить сообшение в конец очереди
   			struct srs485msg *old;
   			old = SLIST_FIRST(&rs485vars.txmsgs);
			if(old != NULL) { // есть блоки для передачи?
   				while(SLIST_NEXT(old, next) != NULL) old = SLIST_NEXT(old, next); // найти последний блок для передачи
   				SLIST_NEXT(msg, next) = NULL;
   				SLIST_NEXT(old, next) = msg;
   			}
			else SLIST_INSERT_HEAD(&rs485vars.txmsgs, msg, next);
   			//  Старт передачи, если не занят
   			rs485_next_txmsg(); //  Проверить наличие пакета в очереди и начать передачу, если возможно
   		}
   		else {
			// поместить сообшение в начало очереди
   			SLIST_INSERT_HEAD(&rs485vars.txmsgs, msg, next);
   		}
	}
#if DEBUGSOO > 1
	else os_printf("rs485tx - out of mem!\n");
#endif
	return msg;
}
/* -------------------------------------------------------------------------
 * Тест принятого блока
 * сообщение в буфере pbufi, длиной len
 * ------------------------------------------------------------------------- */
uint32 ICACHE_FLASH_ATTR rs485_test_msg(uint8 * pbufi, uint32 len)
{
//	if(len >= MDB_SER_ADU_SIZE_MIN) { // уже есть перед вызовом
		len -= 2;
		uint32 crc = CalkCRC16tab(pbufi, len);
		if(pbufi[len] == (uint8)crc && pbufi[len+1] == (uint8)(crc >> 8)) {
			// сообщение принято, crc = ok
#ifdef MDB_RS485_MASTER
			if(rs485vars.flg.b.int_msg != 0) { // был запрос от транзактора?
				smdbadu * adu = (smdbadu *)pbufi;
				if(rs485vars.out[0] == adu->id && rs485vars.out[1] == (adu->err.err & 0x7F)) { // id и команда совпадают?
					// id и команда принятого сообщения совпадают
					if((adu->err.err & 0x80) != 0) { // это сообщение об ошибке?
						ets_intr_lock();
						rs485vars.flg.b.wait = 1; // пока так передать ошибку, просто для счета
						ets_intr_unlock();
						return 1; // ok, перейти к следующей транзакции
					}
					switch(adu->fx.hd.fun) {
					//	case 01: // Read Coils 00000
					//	case 02: // Read Discrete Inputs 10000
						case 03: // Read Holding Registers 40000
						case 04: // Read Input Registers 30000
							if(len == adu->o3o4.hd.cnt + sizeof(adu->o3o4.hd)) {
								uint32 i = (adu->o3o4.hd.cnt + 1) >> 1;
								if(i != 0) {
									Swapws((uint8 *)&adu->o3o4.data[0], i); // перекинем hi<>lo
									if(WrMdbData((uint8 *)adu->o3o4.data, rs485vars.flg.tid, i) != MDBERRNO) {
#if DEBUGSOO > 1
										os_printf("rs485rx - wr error (%u:%u,%u[%u])!\n", adu->o3o4.hd.id, adu->o3o4.hd.fun, rs485vars.flg.tid, adu->o3o4.hd.cnt);
#endif
									}
								}
								return 1; // ok, перейти к следующей транзакции
							}
//							return 0; // ошибка в размере сообщения, ждать далее до таймаута.
							break;
					//	case 05: // Write Single Coil 00000
						case 06: // Write Single Register 40000
					//	case 15: // Write Multiple Coils 00000
						case 16: // Write Multiple Registers 40000
							// пришел ответ регистры записны.
							if(len == sizeof(adu->o6o16)) { // ok ?
								return 1; // ok, перейти к следующей транзакции
							}
//							return 0; // ошибка в размере сообщения, ждать далее до таймаута.
							break;
						default: // пользовательская команда -> просто переписать данные из пакета в указанную память

							if(len >= sizeof(adu->fx.hd)) {
								uint32 i = (len - sizeof(adu->fx.hd) + 1) >> 1;
								if(i != 0) {
									Swapws((uint8 *)&adu->fx.data[0], i >> 1); // перекинем hi<>lo
									if(WrMdbData((uint8 *)&adu->fx.data[0], rs485vars.flg.tid, i) != MDBERRNO) {
#if DEBUGSOO > 1
										os_printf("rs485rx - wr error (%u:%u,%u[%u])!\n", adu->fx.hd.id, adu->fx.hd.fun, rs485vars.flg.tid, i);
#endif
									}
								}
								return 1; // ok, перейти к следующей транзакции
							}
//							return 0; // ошибка в размере сообщения, ждать далее до таймаута.
					}
					return 0; // ошибка в размере сообщения или команде, ждать далее до таймаута.
				}
				// ответ не на ту команду (надо ли ждать далее?)
				return 0; // ждать далее до таймаута.
			}

#endif
			// запрос/ответ с RS-485 не от транзактора
			int flg = 0;
			TCP_SERV_CONN *conn = mdb_tcp_conn;
			if(
#ifdef MDB_RS485_MASTER
					rs485cfg.flg.b.master == 0 &&
#endif
					(((smdbadu *)pbufi)->id == syscfg.mdb_id || ((smdbadu *)pbufi)->id == 0)) {
				// id и команда принятого сообщения совпадают c id назначенном RTU EPS и режим не мастер?
				flg = 1;
			}
			if(conn != NULL && (((smdbadu *)pbufi)->id != syscfg.mdb_id
#ifdef MDB_RS485_MASTER
					|| rs485cfg.flg.b.master != 0
#endif
					)) {
				// есть соединение пакет не к нам или режим мастер (второй мастер на TCP?)
				flg |= 2;
			}
			if(flg) { // надо что-то передавать?
				if (system_get_free_heap_size() < MDB_TCP_RS485_MIN_HEAP) {
					rs485_free_tx_msg(); // Удалить буфер передачи, если есть, исключить сообщение из очереди
					return 1;
				};
				smdbtcp * o = (smdbtcp *) os_malloc(sizeof(smdbtcp)); // sizeof(smdbmbap) + len);
				if(o != NULL) {
					os_memcpy(&o->adu, pbufi, len);
					o->mbap.tid = rs485vars.flg.tid; // Transaction Identifier
					o->mbap.pid = MDB_TCP_PID; // Protocol Identifier
					o->mbap.len = len;
					Swapws((uint8 *)o, sizeof(smdbmbap)>>1); // перекинем hi<>lo
	   				if(flg&2) { // есть соединение пакет не к нам или режим мастер (второй мастер на TCP? :))
#if DEBUGSOO > 1
	   					// есть соединение пакет не к нам или режим мастер (второй мастер на TCP? :))
	   					tcpsrv_print_remote_info(conn);
	   					os_printf("send %u bytes\n", len + sizeof(smdbmbap));
#endif
	   					tcpsrv_int_sent_data(conn, (uint8 *)o, len + sizeof(smdbmbap)); // передать ADU в стек TCP/IP
	    			}
	   				if(flg&1) { // режим slave, пакет к нам -> отвечаем
	   					// режим slave, пакет к нам -> отвечаем
	    				o->mbap.len = MdbFunc(&o->adu, o->mbap.len); // обработать сообщение PDU
#if DEBUGSOO > 2
	    				os_printf("mdb rx-tx %u-%u bytes\n", len, o->mbap.len);
#endif
						if(o->mbap.len) rs485_new_tx_msg(o, false);
					}
	   				os_free(o);
				}
				else {
#if DEBUGSOO > 1
					os_printf("rs485rx %u bytes, mem err!\n", len);
#endif
				}
				return 1; // перейти к следующей транзакции
			}
//			return 0; // ждать далее до таймаута.
		} // test CRC
		else {
			// error CRC
#if DEBUGSOO > 2
			os_printf("rs485rx %u bytes, crc %08x err!\n", len, crc);
#endif
		}
//	}
		return 0; // error CRC, ждать далее до таймаута или следующего сообщения (зависит от режима и ...).
}

#ifdef MDB_RS485_MASTER
/* -------------------------------------------------------------------------
 * ------------------------------------------------------------------------- */
bool ICACHE_FLASH_ATTR mdb_start_trn(uint32 num)
{
	bool ret = false;
	smdbtrn *p = &mdb_buf.trn[num];
/*	if(p->fifo_cnt != 0) { // занят?
		return ret; // да.
	} */
	struct srs485msg *msg = NULL;
	smdbadu * adu;
	uint32 len = p->len << 1;
	switch (p->cmd) {
//	case 01: // Read Coils 00000
//	case 02: // Read Discrete Inputs 10000
	case 03: // Read Holding Registers 40000
	case 04: // Read Input Registers 30000
		if(len == 0 || len > sizeof(adu->o3o4.data)) {
#if DEBUGSOO > 1
			os_printf("rs485tx - error rd/wr data len!\n");
#endif
			break;
		}
		msg = os_malloc(SIZE_RS485MSG_HEAD + 2 + sizeof(adu->f3f4));
		if(msg != NULL) {
			adu = (smdbadu *)msg->buf;
			adu->f3f4.hd.id = p->id;
			adu->f3f4.hd.fun = p->cmd;
			adu->f3f4.hd.addr = p->ext_addr;
			adu->f3f4.hd.len = p->len;
			Swapws((uint8 *)&adu->f3f4.hd.addr, 2); // перекинем hi<>lo
			adu->ui[sizeof(adu->f3f4)>>1] = CalkCRC16tab(adu->uc, sizeof(adu->f3f4));
			msg->len = sizeof(adu->f3f4) + 2; // + CRC
	   		ret = true;
		}
		else {
#if DEBUGSOO > 1
			os_printf("rs485tx - out of mem!\n");
#endif
		}
		break;
//	case 15: // Write Multiple Coils 00000
	case 16: // Write Multiple Registers 40000
		if(len == 0 || len > sizeof(adu->f16.data)) {
#if DEBUGSOO > 1
			os_printf("rs485tx - error rd/wr data len!\n");
#endif
			break;
		};
		if(len != 2) { // переход к команде запись одного регистра?
			// запись нескольких регистров
			msg = os_malloc(len + SIZE_RS485MSG_HEAD + 2 + sizeof(adu->f16.hd));
			if(msg != NULL) {
				adu = (smdbadu *)msg->buf;
				uint32 err = RdMdbData((uint8 *)adu->f16.data, p->int_addr, p->len);
				if(err != MDBERRNO) break;
				Swapws((uint8 *)&adu->f16.data[0], p->len); // перекинем hi<>lo
				adu->f16.hd.id = p->id;
				adu->f16.hd.fun = p->cmd;
				adu->f16.hd.addr = p->ext_addr;
				adu->f16.hd.len = p->len;
				adu->f16.hd.cnt = len;
				Swapws((uint8 *)&adu->f3f4.hd.addr, 2); // перекинем hi<>lo
				len += sizeof(adu->f16.hd);
				uint32 crc = CalkCRC16tab(adu->uc, len);
				msg->buf[len] = (uint8)crc;
				msg->buf[len+1] = (uint8)(crc >> 8);
				msg->len = len + 2; // + CRC
		   		ret = true;
			}
			else {
	#if DEBUGSOO > 1
				os_printf("rs485tx - out of mem!\n");
	#endif
			}
			break;
		} // no break!!! - переход к команде запись одного регистра!
//	case 05: // Write Single Coil 00000
	case 06: // Write Single Register 40000
		msg = os_malloc(SIZE_RS485MSG_HEAD + 2 + sizeof(adu->f6));
		if(msg != NULL) {
			adu = (smdbadu *)msg->buf;
			uint32 err = RdMdbData((uint8 *)&adu->f6.data, p->int_addr, 1);
			if(err != MDBERRNO) break;
			adu->f6.hd.id = p->id;
			adu->f6.hd.fun = p->cmd;
			adu->f6.hd.addr = p->ext_addr;
			Swapws((uint8 *)&adu->f6.hd.addr, 2); // перекинем hi<>lo
			adu->ui[sizeof(adu->f6)>>1] = CalkCRC16tab(adu->uc, sizeof(adu->f6));
			msg->len = sizeof(adu->f6) + 2; // + CRC
			ret = true;
		}
		else {
#if DEBUGSOO > 1
			os_printf("rs485tx - out of mem!\n");
#endif
		}
		break;
/*	case 23: // Read/Write Multiple Registers 40000
		break; */
	default:
		if(len > sizeof(adu->fx.data)) {
#if DEBUGSOO > 1
			os_printf("rs485tx - error rd/wr data len!\n");
#endif
			break;
		};
		msg = os_malloc(len + SIZE_RS485MSG_HEAD + 2 + sizeof(adu->fx.hd));
		if(msg != NULL) {
			adu = (smdbadu *)msg->buf;
			if(len != 0) {
				uint32 err = RdMdbData((uint8 *)adu->fx.data, p->int_addr, p->len);
				if(err != MDBERRNO) break;
				Swapws((uint8 *)&adu->fx.data[0], p->len); // перекинем hi<>lo
			}
			adu->fx.hd.id = p->id;
			adu->fx.hd.fun = p->cmd;
			len += sizeof(adu->fx.hd);
			adu->ui[len>>1] = CalkCRC16tab(adu->uc, sizeof(adu->f6));
			msg->len = len + 2; // + CRC
	   		ret = true;
		}
		else {
#if DEBUGSOO > 1
			os_printf("rs485tx - out of mem!\n");
#endif
		}
		break;
	}
	if(ret) {
		msg->flg.ui = p->int_addr | RS485MSG_FLG_INT_MSG | (num << 24); // Transaction Identifier
		if(p->id == 0) msg->flg.ui |= RS485MSG_FLG_TX_ONLY;
		else p->fifo_cnt++; // счетчик запросов
		// поместить сообшение в очередь
		SLIST_INSERT_HEAD(&rs485vars.txmsgs, msg, next);
		//  Старт передачи, если не занят
		rs485_next_txmsg(); //  Проверить наличие пакета в очереди и начать передачу, если возможно
	}
	else if(msg != NULL) os_free(msg);
	return ret;
}
#endif // MDB_RS485_MASTER

#endif // USE_RS485DRV
