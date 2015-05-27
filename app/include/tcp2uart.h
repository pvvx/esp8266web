/*
 * tcp2uart.h
 *
 *  Created on: 02 янв. 2015 г.
 *      Author: PV`
 */

#ifndef _TCP2UART_H_
#define _TCP2UART_H_

#include "c_types.h"
#include "hw/esp8266.h"
#include "lwip/err.h"
#include "os_type.h"
#include "tcp_srv_conn.h"

#define TCP2UART_PORT_DEF 12345

void uart1_write_char(char c);
void uart0_write_char(char c);

void uart_init(void) ICACHE_FLASH_ATTR;

void uart_save_fcfg(uint8 set) ICACHE_FLASH_ATTR;
void uart_read_fcfg(uint8 set) ICACHE_FLASH_ATTR;

#define uart0_flow_ctrl_flg (UartDev.flow_ctrl)

void uart0_set_flow(bool flow_en) ICACHE_FLASH_ATTR;
void update_rts0(void) ICACHE_FLASH_ATTR;
void update_mux_txd1(void) ICACHE_FLASH_ATTR;
void update_mux_uart0(void) ICACHE_FLASH_ATTR;
void set_uartx_invx(uint8 uartn, uint8 set, uint32 bit_mask) ICACHE_FLASH_ATTR;

extern os_timer_t uart0_rx_buf_timer;
extern os_timer_t uart0_tx_buf_timer;
extern TCP_SERV_CONN * tcp2uart_conn;
extern uint32 wait_send_tx;

void loading_rx_buf(void) ICACHE_FLASH_ATTR;
void send_tx_buf(void) ICACHE_FLASH_ATTR;

err_t tcp2uart_write(uint8 *pblk, uint16 len) ICACHE_FLASH_ATTR;
err_t tcp2uart_init(uint16 portn) ICACHE_FLASH_ATTR;
err_t tcp2uart_close(uint16 portn) ICACHE_FLASH_ATTR;

void tcp2uart_int_rxtx_disable(void) ICACHE_FLASH_ATTR;

#define RST_FIFO_CNT_SET 16 // при остатке в fifo места для 16 символов срабатывает RTS

#define MAX_WAIT_TX_BUF 50000ul // 50 ms

#endif /* _TCP2UART_H_ */
