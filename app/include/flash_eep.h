/******************************************************************************
 * FileName: flash_eep.h
 * Description: FLASH
 * Alternate SDK 
 * Author: PV`
 * (c) PV` 2015
*******************************************************************************/
#ifndef __FLASH_EEP_H_
#define __FLASH_EEP_H_

#include "user_config.h"
//-----------------------------------------------------------------------------

#define FMEMORY_SCFG_BASE_ADDR 0x79000 // 0x3B000, 0x3C000, 0x3D000 / 0x79000, 0x7A000, 0x7B000
#define FMEMORY_SCFG_BANK_SIZE 0x01000 // размер сектора, 4096 bytes
#define FMEMORY_SCFG_BANKS 3 // кол-во секторов для сохранения

//-----------------------------------------------------------------------------

#define ID_CFG_WIFI  0x6977 // id для сохранения установок WiFi (wificonfig)
#define ID_CFG_UART1 0x3175 // id для сохранения установок UART1
#define ID_CFG_UART0 0x3075 // id для сохранения установок UART0
#define ID_CFG_SYS   0x7373 // id для сохранения ситемной конфигурации (syscfg)
#define ID_CFG_UURL  0x5552 // id для сохранения строки tcp_client_url
#define ID_CFG_KVDD  0x564B // id для сохранения калибровочных констант (делитель для VDD)

//-----------------------------------------------------------------------------

struct uartx_bits_config {
	uint32 parity     	: 1;	//0  0x0000001
	uint32 exist_parity : 1;	//1  0x0000002
	uint32 data_bits  	: 2;	//2..3  0x000000c
	uint32 stop_bits  	: 2;	//4..5  0x0000030
	uint32 sw_rts 		: 1;	//6  0x0000040 -
	uint32 sw_dtr 		: 1;	//7  0x0000080 -
	uint32 tx_brk 		: 1;	//8  0x0000100 -
	uint32 irda_dplx 	: 1;	//9  0x0000200 -
	uint32 irda_tx_en 	: 1;	//10 0x0000400 -
	uint32 irda_wctl 	: 1;	//11 0x0000800 -
	uint32 irda_tx_inv 	: 1;	//12 0x0001000 -
	uint32 irda_rx_inv 	: 1;	//13 0x0002000 -
	uint32 loopback		: 1;	//14 0x0004000
	uint32 flow_en		: 1;	//15 0x0008000 rx + tx flow
	uint32 irda_en		: 1;	//16 0x0010000 -
	uint32 rxfifo_rst	: 1;	//17 0x0020000 -
	uint32 txfifo_rst	: 1;	//18 0x0040000 -
	uint32 rxd_inv		: 1;	//19 0x0080000
	uint32 cts_inv		: 1;	//20 0x0100000
	uint32 dsr_inv		: 1;	//21 0x0200000
	uint32 txd_inv		: 1;	//22 0x0400000
	uint32 rts_inv		: 1;	//23 0x0800000
	uint32 dtr_inv		: 1;	//24 0x1000000
	uint32 swap			: 1;	//25 0x2000000 // swap uart0 pins (u0rxd <-> u0cts), (u0txd <-> u0rts)
} __attribute__((packed));

#define UART0_REGCONFIG0MASK	0x1F8C03F
#define UART1_REGCONFIG0MASK	0x1F8C03F
#define UART0_REGCONFIG0DEF		0x000001C
#define UART1_REGCONFIG0DEF		0x000001C
#define UART0_DEFBAUD  115200
#define UART1_DEFBAUD  230400

struct  UartxCfg { // структура сохранения настроек Uart в Flash
	uint32 baud;
	union {
	struct uartx_bits_config b;
	uint32 dw;
	}cfg;
};

struct sys_bits_config {
	uint16 hi_speed_enable		: 1;	//0  0x0000001 =1 Set CPU 160 MHz ...
	uint16 pin_clear_cfg_enable : 1;	//1  0x0000002 =1 Проверять ножку RX на сброс конфигурации WiFi
	uint16 debug_print_enable	: 1;	//2  0x0000004 =1 Вывод отладочной информации на GPIO2
	uint16 web_time_wait_delete	: 1;	//3  0x0000008 =1 Закрывать соединение и убивать pcb c TIME_WAIT
	uint16 netbios_ena			: 1;	//4  0x0000010 =1 включить NetBios
	uint16 sntp_ena				: 1;	//5  0x0000020 =1 включить SNTP
	uint16 cdns_ena				: 1;	//6  0x0000040 =1 включить CAPDNS
	uint16 tcp2uart_reopen		: 1;	//7  0x0000080 =1 открытие нового соединения tcp2uart ведет к закрытию старого соединения (в режиме tcp2uart = сервер)
	uint16 mdb_reopen			: 1;	//8  0x0000100 =1 открытие нового соединения modbus ведет к закрытию старого соединения (в режиме modbus = сервер)
};

#define SYS_CFG_HI_SPEED	0x0000001	// Set CPU 160 MHz ...
#define SYS_CFG_PIN_CLR_ENA	0x0000002	// Проверять ножку RX на сброс конфигурации WiFi
#define SYS_CFG_DEBUG_ENA 	0x0000004	// Вывод отладочной информации на GPIO2
#define SYS_CFG_TWPCB_DEL 	0x0000008	// Закрывать соединение и убивать pcb c TIME_WAIT
#define SYS_CFG_NETBIOS_ENA	0x0000010	// включить NetBios
#define SYS_CFG_SNTP_ENA	0x0000020	// включить SNTP
#define SYS_CFG_CDNS_ENA	0x0000040	// включить CAPDNS
#define SYS_CFG_T2U_REOPEN	0x0000080	// открытие нового соединения tcp2uart ведет к закрытию старого соединения (сервер)
#define SYS_CFG_MDB_REOPEN	0x0000100	// открытие нового соединения modbus ведет к закрытию старого соединения (сервер)

struct SystemCfg { // структура сохранения системных настроек в Flash
	union {
		struct sys_bits_config b;
		uint16 w;
	}cfg;
	uint16 tcp_client_twait;	// время (миллисек) до повтора соединения клиента
#ifdef USE_TCP2UART
	uint16 tcp2uart_port;	// номер порта TCP-UART (=0 - отключен)
	uint16 tcp2uart_twrec;	// время (сек) стартового ожидания приема/передачи первого пакета, до авто-закрытия соединения
	uint16 tcp2uart_twcls;	// время (сек) до авто-закрытия соединения после приема или передачи
#endif
#ifdef USE_WEB
	uint16 web_port;	// номер порта WEB (=0 - отключен)
	uint16 web_twrec;	// время (сек) стартового ожидания приема/передачи первого пакета, до авто-закрытия соединения
	uint16 web_twcls;	// время (сек) до авто-закрытия соединения после приема или передачи
#endif
#ifdef USE_MODBUS
	uint16 mdb_port;	// =0 - отключен
	uint16 mdb_twrec;	// время (сек) стартового ожидания приема/передачи первого пакета, до авто-закрытия соединения
	uint16 mdb_twcls;	// время (сек) до авто-закрытия соединения после приема или передачи
	uint8  mdb_id;	// номер устройства ESP8266 по шине modbus
#endif
} __attribute__((packed));


//-----------------------------------------------------------------------------

sint16 flash_read_cfg(void *ptr, uint16 id, uint16 maxsize) ICACHE_FLASH_ATTR; // возврат: размер объекта последнего сохранения, -1 - не найден, -2 - error
bool flash_save_cfg(void *ptr, uint16 id, uint16 size) ICACHE_FLASH_ATTR;

extern struct SystemCfg syscfg;
#if defined(USE_TCP2UART) || defined(USE_MODBUS)
extern uint8 * tcp_client_url;
#endif

bool sys_write_cfg(void) ICACHE_FLASH_ATTR; // пишет из struct SystemCfg *scfg
bool sys_read_cfg(void) ICACHE_FLASH_ATTR; // читет в struct SystemCfg *scfg
bool new_tcp_client_url(uint8 *url) ICACHE_FLASH_ATTR;
bool read_tcp_client_url(void) ICACHE_FLASH_ATTR;

#define MAX_IDX_USER_CONST 4

uint32 read_user_const(uint8 idx); // чтение пользовательских констант (0 < idx < 4)
bool write_user_const(uint8 idx, uint32 data); // запись пользовательских констант (0 < idx < 4)

#endif /* __FLASH_EEP_H_ */
