#ifndef INCLUDE_RS485_DRV_H_
/******************************************************************************
 * FileName: rs485drv.h
 * RS-485 low level driver
 * Полудуплексный прием/передача по шине RS-485
 * Created on: 02/11/2015
 * Author: PV`
 ******************************************************************************/
#define INCLUDE_RS485_DRV_H_

#include "user_config.h"
//#ifdef USE_RS485DRV
#include "os_type.h"
#include "queue.h"
#include "mdbtab.h"

#define RS485_TASK_QUEUE_LEN 7
#define RS485_TASK_PRIO (USER_TASK_PRIO_0) // + SDK_TASK_PRIO)

typedef enum {
	RS485_SIG_TIMEOUT = 0,	// Таймаут приема/передачи
	RS485_SIG_RX_OK, 		//	Принято ADU
	RS485_SIG_TX_OK 		//	ADU передано
} RS485_SIGS;

#ifdef MDB_RS485_MASTER
//	#define RS485_TIMEOUT_ALARM_ENA // включить оповещение о таймаутах
#endif

typedef enum {
	RS485_TX_RX_OFF,	// Драйвер отключен
	RS485_TX_ENA,		// Идет передача.
	RS485_RX_ENA		// Драйвера работает на прием.
}rs485status;

//#define PIN_TX 	1 // GPIO1 или GPIO15
#define PIN_TX_ENABLE 0xFF // GPIO2 или любой незадейстованный. > 15 -> не используется

#define MDB_SER_ADU_SIZE_MAX 256 // Максимальный размер пакета RTU
#define MDB_SER_ADU_SIZE_MIN 5 // Минимальный размер ADU для последовательных сетей (Response error 3 bytes + CRC 2 bytes)

#define RS485_MAX_BAUD 3076923 // Максимальная скорость интерфейса (baud-rate)
#define RS485_MIN_BAUD 300 // Минимальная скорость интерфейса (baud-rate)

#define RS485_DEF_BAUD 19200  // скорость по умолчанию
#define RS485_DEF_TWAIT  190  // ms

typedef SLIST_HEAD(listrsmsgs, srs485msg) srsmsgs;

typedef union  // Флаги конфигурации драйвера приеника/передатчика блоков по RS-485
{
   uint32  ui;
   struct
   {
	   uint32 parity			: 2; 	//00 =0..1 - нет, =2 - even, =3 - odd
	   uint32 none1				: 14;	//02 разметка под чтение словами в таблице Modbus
	   uint32 pin_ena 			: 5;	//16 номер пина GPIO для сигнала OUT_ENA, если больше 15 -> отсуствует.
	   uint32 none2				: 3;	//21 разметка под чтение словами в таблице Modbus
	   uint32 swap				: 1;	//24 =1 - swap uart0 pins (u0rxd <-> u0cts), (u0txd <-> u0rts)
	   uint32 spdtw				: 1;	//25 =0 - давать стандартную задержку при > 19200 baud 1750 us, =1 - всегда 3.5 символа
	   uint32 master			: 1;	//26 =1 - master, =0 - slave
	   uint32 none3				: 5;	//27 разметка под чтение словами в таблице Modbus
   }b;
}srs485cfgflg;

#define RS485CFG_FLG_PARITY 	0x00000003
#define RS485CFG_FLG_PIN_ENA 	0x001F0000
#define RS485CFG_FLG_SWAP		0x01000000
#define RS485CFG_FLG_SPDTW		0x02000000
#define RS485CFG_FLG_MASTER 	0x04000000
#ifdef MDB_RS485_MASTER
	#define RS485_DEF_FLG   RS485CFG_FLG_MASTER	  // parity, swap, ..., номер пина OE = 0, по умолчанию
#else
	#define RS485_DEF_FLG   0	  // parity, swap, ..., номер пина OE = 0, по умолчанию
#endif

typedef struct {	// конфигурации драйвера приеника/передатчика блоков по RS-485
	uint32 baud;		// скорость UART, 300..3 000 000 bps
	srs485cfgflg flg;	// флаги конфигурации
	uint16 timeout;		// предельное время в ms ожидания приема/передачи пакета (таймаут), 3…65535 мс
	uint16 pause;		// добавка времени в us между сообщениями, 0…65535 мкc
}srs485cfg;

extern srs485cfg rs485cfg; // конфигурация (сохраняется в flash)

typedef union  // Флаги драйвера приеника/передатчика блоков по RS-485
{
   uint16 tid; // Transaction Identifier
   uint32  ui;
   struct __attribute__ ((packed))
   {
	   uint16 tid; 		// Transaction Identifier
	   uint8 flg;		// Флаги
	   uint8 trn_num;	// Trn number
   }w;
   struct
   {
	   uint32 tid				: 16; // Transaction Identifier
	   uint32 tx_only			: 1; // после передачи не идет прием // конец задания по передаче // передача без последюущего ожидания приема
	   uint32 rx_chars  	 	: 1; // приняты символы после TOUT
	   uint32 int_msg			: 1; // внутренняя пересылка (tid = rx_addr, trn_num = номер trn)
	   uint32 rx_end			: 1; // был конец приема + пауза 24..32 бита
	   uint32 tx_ready  	 	: 1; // возможен старт следующей передачи (требует проверки UART_RXFIFO_CNT == 0 && rs485vars.flg.b.rx_chars == 0)
	   uint32 tx_end	 		: 1; // был конец передачи + пауза 24..32 бита
	   uint32 wait				: 1; // таймаут ожидания
	   uint32 trn_reset			: 1; // trn сброшен
	   uint32 trn_num			: 8; // Trn number
   }b;
}srs485flg;
#define RS485MSG_FLG_TID_MASK 	0x0000FFFF
#define RS485MSG_FLG_TX_ONLY 	0x00010000
#define RS485MSG_FLG_RX_CHARS 	0x00020000
#define RS485MSG_FLG_INT_MSG	0x00040000
#define RS485MSG_FLG_RX_END 	0x00080000
#define RS485MSG_FLG_TX_READY 	0x00100000
#define RS485MSG_FLG_TX_END 	0x00200000
#define RS485MSG_FLG_WAIT		0x00400000
#define RS485MSG_FLG_TRN_RES 	0x00800000
#define RS485MSG_FLG_TRN_NUM_MASK 	0xFF000000

typedef struct   // рабочая структура управления драйвером
{
	volatile uint32 status;		// статус драйвера
	srs485flg flg;		// флаги драйвера приеника/передатчика блоков по RS-485
    uint8 	out[4];		// вывод id, cmd
	uint8 * pbufo;		// рабочий указатель в буфере передачи (после вывода указывает на следующий за последним переданным байтом)
	uint8 * pbufi;		// указатель в буфере приема
	uint32 cntro;		// кол-во символов в буфере передачи (после вывода = 0)
	uint32 cntri;		// кол-во символов в буфере приема (после ввода = размеру msg)
//	uint32 tout_thrhd; 	// время задержки в тиках tout для ожидания паузы в конце сообщения
	uint32 pin_ena_mask; // битовая маска номера пина переключения направления передачи. При 0 - вывод отключен, не используется.
	uint32 waitust;		// время задержки в us для таймера ожидания тишины перед следующей передачей (часть паузы 3.5 символа)
	struct srs485msg * curtxmsg; // указатель на текущуй блок передачи (если не равен NULL - ожидание передачи или уже идет)
	srsmsgs txmsgs;		// указатель на очередь (fifo) блоков передачи
	os_timer_t timerp;	// таймер ожидания тишины после последнего rx/tx символа
	os_timer_t timerw;	// таймер предельного времени ожидания приема/передачи пакета
	ETSEvent taskQueue[RS485_TASK_QUEUE_LEN];
}srs485vars;

extern srs485vars rs485vars; // рабочий блок переменных

struct srs485msg	// блок сообщения в fifo (в очереди сообщений)
{
    SLIST_ENTRY(srs485msg) next; /* next element */
	srs485flg flg; // флаги пакета
	uint32 len; // длина пакета
	uint8 buf[MDB_SER_ADU_SIZE_MAX]; // буфер данных пакета (использется длина по сообщению!)
};
#define SIZE_RS485MSG_HEAD (3*4)

/* Инициализация драйвера rs485 */
void rs485_drv_init(void);
/* Запустить драйвер RS485 (прием)
 * Ожидает паузу в 3.5 символа перед первой передачей
 * Запускает таймаут ожидания */
void rs485_drv_start(void);
/* Остановить драйвер RS485 */
void rs485_drv_stop(void);
/* Проверить наличие пакета в очереди и начать передачу */
bool rs485_next_txmsg(void);

/* (Ре)Инициализация драйвера RS-485 на новый конфиг */
//void rs485_drv_new_cfg(srs485cfg * newcfg);
/* (Ре)Инициализация драйвера RS-485 по пинам
 * rs485cfg содержит новые параметры */
void rs485_drv_set_pins(void);
/* (Ре)Инициализация драйвера RS-485 по скорости
 * rs485cfg содержит новые параметры */
void rs485_drv_set_baud(void);
/* Удалить буфер передачи, исключить сообщение из очереди,
 * отметить что была передача (rs485vars.curtxmsg = NULL) */
void rs485_free_tx_msg(void);


// #endif // USE_RS485DRV
#endif /* INCLUDE_RS485_DRV_H_ */
