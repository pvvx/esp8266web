/******************************************************************************
 * FileName: tcp2uart.h
 * Alternate SDK 
 * Author: PV`
 * (c) PV` 2015
*******************************************************************************/
#ifndef _TCP2UART_H_
#define _TCP2UART_H_

#include "user_config.h"
#include "c_types.h"
#include "hw/esp8266.h"
#include "lwip/err.h"
#include "os_type.h"
#ifdef USE_TCP2UART
#include "tcp_srv_conn.h"
#endif

#define DEFAULT_TCP2UART_PORT USE_TCP2UART // 12345

void uarts_init(void) ICACHE_FLASH_ATTR;

void uart_save_fcfg(uint8 set) ICACHE_FLASH_ATTR;
void uart_read_fcfg(uint8 set) ICACHE_FLASH_ATTR;

#define uart0_flow_ctrl_flg (UartDev.flow_ctrl)

#ifndef USE_RS485DRV
void uart0_set_flow(bool flow_en) ICACHE_FLASH_ATTR;
void update_rts0(void) ICACHE_FLASH_ATTR;
void update_mux_uart0(void) ICACHE_FLASH_ATTR;
#endif
void update_mux_txd1(void) ICACHE_FLASH_ATTR;
void set_uartx_invx(uint8 uartn, uint8 set, uint32 bit_mask) ICACHE_FLASH_ATTR;

#define RST_FIFO_CNT_SET 8 // при остатке в fifo места для 16 символов срабатывает RTS

#ifdef USE_TCP2UART

#define UART_RX_BUF_MAX 8192 // размер приемного буфера (не менее TCP_MSS*2 + ...)
#define UART_TASK_QUEUE_LEN 3
#define UART_TASK_PRIO (USER_TASK_PRIO_0) // + SDK_TASK_PRIO)

typedef void uart_rx_blk_func(uint8 *buf, uint32 count); // функция обработки принятых блоков из UART
typedef void uart_tx_next_chars_func(void); // запрос на передачу следующих символов блока в UART

typedef enum {
	UART_RX_CHARS = 1,
	UART_TX_CHARS,
} UART_SIGS;

typedef struct {
	uint8	* uart_rx_buf; // указатель на буфер [UART_RX_BUF_MAX], если равер NULL, драйвер отключен
	uint32	uart_rx_buf_count; // указатель принимаемых с UART символов в буфере
	uint32	uart_out_buf_count; // кол-во переданных байт из буфера на обработку
	uint32	uart_nsnd_buf_count; // кол-во ещё не переданных байт из буфера (находящихся в ожидании к передаче)
	uart_rx_blk_func * uart_send_rx_blk; // функция обработки принятых блоков из UART
	uart_tx_next_chars_func * uart_tx_next_chars; // запрос на передачу следующих символов блока в UART
	ETSEvent taskQueue[UART_TASK_QUEUE_LEN];
}suart_drv;

void uart0_set_tout(void);
void uart_del_rx_chars(uint32 len);
uint32 uart_tx_buf(uint8 *buf, uint32 count);
bool uart_drv_start(void);
void uart_drv_close(void);


err_t tcp2uart_write(uint8 *pblk, uint16 len);
err_t tcp2uart_start(uint16 newportn);
void tcp2uart_close(void);

extern suart_drv uart_drv;
extern TCP_SERV_CONN * tcp2uart_conn;
extern TCP_SERV_CFG * tcp2uart_servcfg;

#endif // USE_TCP2UART
#endif /* _TCP2UART_H_ */
