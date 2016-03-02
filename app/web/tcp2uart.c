/*
 * tcp_terminal.c
 *
 * TCP2UART
 *
 *  Created on: 27/12/2014
 *      Author: PV`
 */

#include "user_config.h"
#ifdef USE_TCP2UART
#include "bios.h"
#include "sdk/add_func.h"
#include "c_types.h"
#include "osapi.h"
#include "lwip/tcp.h"
#include "flash_eep.h"
#include "hw/uart_register.h"
#include "web_iohw.h"
#include "wifi.h"
#include "tcp2uart.h"



#define mMIN(a, b)  ((a<b)?a:b)

TCP_SERV_CONN * tcp2uart_conn DATA_IRAM_ATTR;
TCP_SERV_CFG * tcp2uart_servcfg DATA_IRAM_ATTR;
/*
//-------------------------------------------------------------------------------
// tcp2uart_get_max_send
//-------------------------------------------------------------------------------
uint32 ICACHE_FLASH_ATTR tcp2uart_get_max_send(void)
{
	TCP_SERV_CONN *conn = tcp2uart_conn;
	if(conn == NULL || conn->pcb == NULL) return 0; // нет соединения
	return conn->pcb->snd_buf;
}
*/
//-------------------------------------------------------------------------------
// tcp2uart_send_rxbuf
// передача буфера rx uart в TCP
//-------------------------------------------------------------------------------
//volatile uint8 tst_flg;
void tcp2uart_send_rxbuf(uint8 *buf, uint32 count)
{
	TCP_SERV_CONN *conn = tcp2uart_conn;
	if(conn != NULL) {
		if(conn->cntro == 0 && conn->flag.wait_sent == 0) {
			// не ожидает завершения передачи (sent_cb)
			conn->cntro = count;
			uint32 len = mMIN(conn->pcb->snd_buf, conn->cntro);
#if DEBUGSOO > 5
			os_printf(" sent1 %u bytes\n", len);
#endif
			if(tcpsrv_int_sent_data(conn, buf, len) != ERR_OK) {
#if DEBUGSOO > 1
				os_printf("tcp2uart: err sent!\n");
#endif
			}
			if(len) uart_del_rx_chars(len);
		}
		else conn->cntro += count; // не передавать, ждать sent_cb
	}
}
//-------------------------------------------------------------------------------
// TCP sent_cb (UART->bufo->TCP)
//-------------------------------------------------------------------------------
err_t ICACHE_FLASH_ATTR tcp2uart_sent_cb(TCP_SERV_CONN *conn) {
#if DEBUGSOO > 3
	tcpsrv_sent_callback_default(conn);
#endif
	tcp2uart_conn = conn;
	if(conn->cntro) {
		uint32 len = mMIN(conn->pcb->snd_buf, conn->cntro);
#if DEBUGSOO > 5
		os_printf(" sent2 %u bytes\n", len);
#endif
		if(tcp_write(conn->pcb, uart_drv.uart_rx_buf, len, 0)!= ERR_OK) {
#if DEBUGSOO > 1
				os_printf("tcp2uart: err sent!\n");
#endif
		}
		if(len) {
			uart_del_rx_chars(len);
			conn->cntro -= len;
		}
	}
	return ERR_OK;
}
//===============================================================================
// tcp2uart_send_tx_buf()
// передача буфера TCP в tx fifo UART
//-------------------------------------------------------------------------------
void tcp2uart_send_tx_buf(void)
{
   	TCP_SERV_CONN * conn = tcp2uart_conn;
   	if(conn == NULL || conn->pbufi == NULL) return;
	uint32 len = conn->sizei - conn->cntri;
	if(len) {
		len = uart_tx_buf(&conn->pbufi[conn->cntri], len);
		if(len)	{
			conn->cntri += len;
			tcp_recved(conn->pcb, len);
/*			if(len < TCP_MSS) {
				conn->unrecved_bytes = TCP_WND;
				tcpsrv_unrecved_win(conn);
			} */
		}
	}
}
//-------------------------------------------------------------------------------
// TCP recv  (TCP->bufi->UART)
//-------------------------------------------------------------------------------
err_t ICACHE_FLASH_ATTR tcp2uart_recv(TCP_SERV_CONN *conn) {
#if DEBUGSOO > 2
	tcpsrv_received_data_default(conn);
#endif
	tcp2uart_conn = conn;
	conn->flag.busy_bufi = 0;
	tcp2uart_send_tx_buf(); // передать в fifo tx UART0
	conn->flag.busy_bufi = 1;
	return ERR_OK;
}
//-------------------------------------------------------------------------------
// TCP listen (Start UART->bufo)
//-------------------------------------------------------------------------------
err_t ICACHE_FLASH_ATTR tcp2uart_listen(TCP_SERV_CONN *conn) {
#if DEBUGSOO > 1
	tcpsrv_listen_default(conn);
#endif
	// инициировать процесс передачи и приема с UART0 в TCP
    tcp2uart_conn = conn;
	tcp_nagle_disable(conn->pcb);
	uart_drv.uart_send_rx_blk = tcp2uart_send_rxbuf;
	uart_drv.uart_tx_next_chars = tcp2uart_send_tx_buf;
	if(!uart_drv_start()) {
#if DEBUGSOO > 0
		os_printf("tcp2uart: error mem!\n");
#endif
		return ERR_MEM;
	}
	return ERR_OK;
}
//-------------------------------------------------------------------------------
// TCP disconnect
//-------------------------------------------------------------------------------
void ICACHE_FLASH_ATTR tcp2uart_disconnect(TCP_SERV_CONN *conn) {
#if DEBUGSOO > 1
	tcpsrv_disconnect_calback_default(conn);
#endif
	uart_drv_close();
}
//-------------------------------------------------------------------------------
// tcp2uart_close
//-------------------------------------------------------------------------------
void ICACHE_FLASH_ATTR tcp2uart_close(void)
{
//	syscfg.tcp2uart_port = 0;
	if(tcp2uart_servcfg != NULL) {
		uart_drv_close();
		tcpsrv_close(tcp2uart_servcfg);	
		tcp2uart_servcfg = NULL;
		update_mux_uart0();
#if DEBUGSOO > 3
		os_printf("TCP2UART: close\n");
#endif
	}
}
//-------------------------------------------------------------------------------
// tcp2uart_start
//-------------------------------------------------------------------------------
err_t ICACHE_FLASH_ATTR tcp2uart_start(uint16 portn)
{
		err_t err = ERR_USE;
		ip_addr_t ip;
		ip.addr = 0;
		bool flg_client = false;
		if(portn <= ID_CLIENTS_PORT) {
			tcp2uart_close();
			syscfg.tcp2uart_port = 0;
			update_mux_uart0();
			return ERR_OK;
		}
		if(tcp_client_url != NULL) {
			ipaddr_aton(tcp_client_url, &ip);
			if(ip.addr != 0x0100007f && ip.addr != 0) {
#if DEBUGSOO > 1
				os_printf("TCP2UART: client ip:" IPSTR ", port: %u\n", IP2STR(&ip), portn);
#endif
				flg_client = true;
			}
		}
		// проверка на повторное открытие уже открытого
		if(tcp2uart_servcfg != NULL) {
			// уже запущен
			if(flg_client) {
				// client
				if(tcp2uart_servcfg->flag.client // соединение не сервер, а клиент
				&& tcp2uart_servcfg->conn_links != NULL // соединение активно
				&& tcp2uart_servcfg->conn_links->remote_ip.dw == ip.addr
				&& tcp2uart_servcfg->conn_links->remote_port == portn) { // порт совпадает
					return err; // ERR_USE параметры не изменились
				}
			}
			else {
				// server
				if((!(tcp2uart_servcfg->flag.client)) // соединение сервер
					&& tcp2uart_servcfg->port == portn) { // порт совпадает
					return err; // ERR_USE параметры не изменились
				}
			}
		}
		// соединения нет или смена параметров соединения
		tcp2uart_close(); // закрыть прошлое
		if(portn <= ID_CLIENTS_PORT) return ERR_OK;
		TCP_SERV_CFG * p;
		if(flg_client) p = tcpsrv_init_client1();  // tcpsrv_init(1)
		else p = tcpsrv_init(portn);
		if (p != NULL) {
			// изменим конфиг на наше усмотрение:
			if(flg_client) {
				p->flag.client_reconnect = 1; // вечный реконнект
				p->max_conn = 0; // =0 - вечная попытка соединения
			}
			else {
				if(syscfg.cfg.b.tcp2uart_reopen) p->flag.srv_reopen = 1;
				p->max_conn = 1; // одно соединение (порт UART не многопользовательский!)
			}
			p->flag.rx_buf = 1; // прием в буфер с его автосозданием.
			p->flag.nagle_disabled = 1; // отмена nagle
			p->time_wait_rec = syscfg.tcp2uart_twrec; // =0 -> вечное ожидание
			p->time_wait_cls = syscfg.tcp2uart_twcls; // =0 -> вечное ожидание
#if DEBUGSOO > 3
			os_printf("Max retry connection %d, time waits %d & %d, min heap size %d\n",
						p->max_conn, p->time_wait_rec, p->time_wait_cls, p->min_heap);
#endif
			p->func_discon_cb = tcp2uart_disconnect;
			p->func_listen = tcp2uart_listen;
	//		p->func_sent_cb = NULL;
			p->func_sent_cb = tcp2uart_sent_cb;
			p->func_recv = tcp2uart_recv;
			if(flg_client) err = tcpsrv_client_start(p, ip.addr, portn);
			else err = tcpsrv_start(p);
			if (err != ERR_OK) {
				tcpsrv_close(p);
				p = NULL;
			}
			else  {
				syscfg.tcp2uart_port = portn;
				update_mux_uart0();
#if DEBUGSOO > 5
				if(flg_client)	{
					os_printf("TCP2UART: client init\n");
				}
				else {
					os_printf("TCP2UART: init port %u\n", portn);
				}
#endif
			}
		}
		else err = ERR_USE;
		tcp2uart_servcfg = p;
		return err;
}

#endif // USE_TCP2UART
