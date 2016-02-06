/******************************************************************************
 * FileName: mdbtab.h
 * ModBus TCP RTU <> RS-485
 * Author: PV`
 * (c) PV` 2016
*******************************************************************************/
#ifndef _INCLUDE_MDBTAB_H_
#define _INCLUDE_MDBTAB_H_

#include "user_config.h"
#ifdef MDB_RS485_MASTER

#ifndef MDB_BUF_MAX
	#define MDB_BUF_MAX 1000 // размер буфера Modbus переменных для обмена между интерфейсами Web<->RS-485<->TCP
#endif
#define MDB_SYS_VAR_ADDR (MDB_BUF_MAX + 100) // адрес старта системных переменных буфера Modbus для обмена между интерфейсами Web<->RS-485<->TCP

typedef struct __attribute__ ((packed)) {	//
	uint16	start_flg;  //0 флаг пуска
	uint16	id;			//1 Адрес внешнего устройства
	uint16	cmd;		//2 Номер команды
	uint16	len;		//3 Кол-во слов передачи
	uint16	ext_addr;	//4 Адрес данных во внешнем устройстве
	uint16	int_addr;	//5 Адрес данных во внутреннем устройстве (ESP)
	uint16	timer_set;	//6 таймерная установка
	uint16	rx_err;		//7 счетчик запросов без ответа (до 0xffff)
	uint16	fifo_cnt;	//8 счетчик неотработанных сообщений в очереди
	uint16	timer_cnt;  //9 таймерный счетчик
}smdbtrn;

#define MDB_TRN_MAX 	10	// кол-во таблиц управления транзакциями в режиме master Modbus RTU RS-485

typedef struct {	//
	uint16 ubuf[MDB_BUF_MAX]; // буфер переменных Modbus для обмена между интерфейсами Web<->RS-485<->TCP
	smdbtrn trn[MDB_TRN_MAX];
}smdb_ubuf;

bool mdb_start_trn(uint32 num);
void init_mdbtab(void);

#else

#ifndef MDB_BUF_MAX
	#define MDB_BUF_MAX 100 // размер буфера Modbus переменных для обмена между интерфейсами Web<->RS-485<->TCP
#endif
#define MDB_SYS_VAR_ADDR (1100) // адрес старта системных переменных буфера Modbus для обмена между интерфейсами Web<->RS-485<->TCP

typedef struct {	//
	uint16 ubuf[MDB_BUF_MAX]; // буфер переменных Modbus для обмена между интерфейсами Web<->RS-485<->TCP
}smdb_ubuf;

#endif

extern smdb_ubuf mdb_buf; // буфер переменных Modbus для обмена между интерфейсами Web<->RS-485<->TCP

bool mbd_fini(uint8 * cfile);

#endif /* _INCLUDE_MDBTAB_H_ */
