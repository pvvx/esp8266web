/*
 * tcp_terminal.c
 *
 * TCP2UART
 *
 *  Created on: 27/12/2014
 *      Author: PV`
 */

#include "user_config.h"
#include "bios.h"
#include "add_sdk_func.h"
#include "c_types.h"
#include "osapi.h"
#include "lwip/tcp.h"
#include "flash_eep.h"
#include "hw/uart_register.h"
#include "web_iohw.h"
#include "wifi.h"
#include "tcp2uart.h"

void send_tx_buf(void) ICACHE_FLASH_ATTR; // TCP->UART
void loading_rx_buf(void);	// UART->TCP


#define mMIN(a, b)  ((a<b)?a:b)

os_timer_t uart0_rx_buf_timer;
os_timer_t uart0_tx_buf_timer;
uint32 old_time_send_tx;

TCP_SERV_CONN * tcp2uart_conn;
TCP_SERV_CFG * tcp2uart_servcfg;
//-------------------------------------------------------------------------------
// tcp2uart_int_rxtx_disable
//-------------------------------------------------------------------------------
void ICACHE_FLASH_ATTR tcp2uart_int_rxtx_disable(void)
{
    tcp2uart_conn = NULL;
	ets_intr_lock(); //	ETS_UART_INTR_DISABLE();
	MEMW();
    UART0_CONF1 &= ~UART_RX_FLOW_EN; // update RST
	UART0_INT_ENA &= ~(UART_RXFIFO_FULL_INT_ENA | UART_TXFIFO_EMPTY_INT_ENA);
    //clear rx and tx fifo, not ready
    uint32 conf0 = UART0_CONF0;
    UART0_CONF0 = conf0 | UART_RXFIFO_RST | UART_TXFIFO_RST;
    UART0_CONF0 = conf0 & (~ (UART_RXFIFO_RST | UART_TXFIFO_RST));
//	update_rts0();
	ets_intr_unlock(); // ETS_UART_INTR_ENABLE();
//?	WRITE_PERI_REG(UART_INT_CLR(UART0), UART_RXFIFO_FULL_INT_CLR | UART_TXFIFO_EMPTY_INT_CLR);
	os_timer_disarm(&uart0_tx_buf_timer);
	os_timer_setfn(&uart0_tx_buf_timer, (os_timer_func_t *)send_tx_buf, NULL);
	os_timer_disarm(&uart0_rx_buf_timer);
	os_timer_setfn(&uart0_rx_buf_timer, (os_timer_func_t *)loading_rx_buf, NULL);
}
//===============================================================================
// Timer: UART->TCP (UART->bufo->TCP)
// loading_rx_buf() чтение fifo UART rx в буфер передачи TCP
// Сигнал CTS/RTS пока не огранизован в связи с неясностью,
// на какую ногу модуля его делать
//-------------------------------------------------------------------------------
void ICACHE_FLASH_ATTR loading_rx_buf(void)
{
	TCP_SERV_CONN *conn = tcp2uart_conn;
	if(conn == NULL || conn->pbufo == NULL || conn->flag.user_flg1) return; // нет буфера + тест на повторное вхождение
	conn->flag.user_flg1 = 1;
	ets_intr_lock(); //	ETS_UART_INTR_DISABLE();
	MEMW();
	UART0_INT_ENA &= ~ UART_RXFIFO_FULL_INT_ENA; // запретить прерывание по приему символа
	ets_intr_unlock(); // ETS_UART_INTR_ENABLE();
	os_timer_disarm(&uart0_rx_buf_timer);
	if(conn->flag.busy_bufo) { // в данный момент bufo обрабатывается (передается LwIP-у)?
		// попробовать повторить через время
		ets_timer_arm_new(&uart0_rx_buf_timer, 10, 0, 0); // 10us
		conn->flag.user_flg1 = 0;
		return;
	}
	uint8 *pend = conn->pbufo + conn->sizeo;
	// дополнить буфер передачи символами из rx fifo
	while((conn->ptrtx + conn->cntro) < pend) {
		MEMW();
		if((UART0_STATUS >> UART_RXFIFO_CNT_S) & UART_RXFIFO_CNT) conn->ptrtx[conn->cntro++] = UART0_FIFO;
		else break;
	}
//	UART0_INT_CLR &= ~UART_RXFIFO_FULL_INT_CLR; // сбросить флаг прерывания приема (нет смысла)
	// если передача ещё не идет и есть данные для передачи размером с буфер у Lwip, то передать
	if((!conn->flag.wait_sent) && (conn->cntro)) {
		uint32 len = conn->pcb->snd_buf;
		uint32 time_us = IOREG(0x3FF20C00); // phy_get_mactime();
		if(((time_us - old_time_send_tx) > (MAX_WAIT_TX_BUF)) || len  <= conn->cntro) {
   			old_time_send_tx = time_us;
			conn->flag.busy_bufo = 1; // в данный момент bufo обрабатывается (передается LwIP-у)
	#if DEBUGSOO > 3
			os_printf("usnt: %u ", conn->cntro);
	#endif
			if(tcpsrv_int_sent_data(conn, conn->pbufo, mMIN(len, conn->cntro)) == ERR_OK) {
				// удалить из буфера переданные данные
				if(conn->ptrtx != conn->pbufo && conn->cntro != 0) os_memcpy(conn->pbufo, conn->ptrtx, conn->cntro);
				conn->ptrtx = conn->pbufo; // указатель на начало не переданных данных (начало буфера)
			}
			else { // ошибка (значит соединение закрыто в tcpsrv_int_sent_data() ) и обрабатывать нет смысла
				conn->flag.user_flg1 = 0; // в данный момент bufo не обрабатывается
	#if DEBUGSOO > 1
				os_printf("tcp2uart: err sent!\n");
	#endif
				return;
			};
		};
	};
	conn->flag.user_flg1 = 0; // в данный момент bufo не обрабатывается
	MEMW();
	uint32 buftimeout = (UART0_STATUS >> UART_RXFIFO_CNT_S) & UART_RXFIFO_CNT; // получить кол-во символов в FIFO RX uart
	if(buftimeout) {
		// если буфер полон, ждем передачи буфера иначе ждем набора fifo
		if(conn->cntro < conn->sizeo) { // буфер не забит
			if(buftimeout < (128 - RST_FIFO_CNT_SET - 1)) { // можем ещё принять до выставления RTS? да.
   				buftimeout = (128 - RST_FIFO_CNT_SET - 1) - buftimeout; // сколько символов можем принять до выставления RTS
				buftimeout = ((UART0_CLKDIV & UART_CLKDIV_CNT) * buftimeout) >> 3; // время передачи символа (10 бит) в us =  UART_CLKDIV / 8
				if(buftimeout < 128) buftimeout = 128; // быстрее работать не стоит
				else if(buftimeout > MAX_WAIT_TX_BUF) buftimeout = MAX_WAIT_TX_BUF; // низкая скорость и буфер будет заполнен более чем через 0.05 сек? ограничить
				// buftimeout -= 16; // вычесть время исполнения?
			}
			else buftimeout = 16; // буфер rx fifo заполнен
		}
		else  buftimeout = 1024; //если буфер забит, то это шаг ожидания передачи буфера. 1024us выбрано наобум
		ets_timer_arm_new(&uart0_rx_buf_timer, buftimeout, 0, 0);
	}
	else  { // пока не приняты новые символы в fifo rx UART
		// ожидать приема или если в буфере ещё есть символы, то и таймера раз буфер не заполнется за 0.05 секунды.
		if(conn->cntro) { // буфер ещё не пуст
#if DEBUGSOO > 3
				os_printf("term_flg2: %u\n", tcp2uart_conn->cntro);
#endif
				// ограничить время до отсылки неполного буфера, если символы передаются редко или низкая скорость UART
				ets_timer_arm_new(&uart0_rx_buf_timer, MAX_WAIT_TX_BUF, 0, 0);
		}
		ets_intr_lock(); //	ETS_UART_INTR_DISABLE();
		MEMW();
		UART0_INT_ENA |= UART_RXFIFO_FULL_INT_ENA; // зарядить прерывание UART rx
		ets_intr_unlock(); // ETS_UART_INTR_ENABLE();
		// <-- тут, сразу после ets_intr_unlock() прерывание и сработет :)
	}
}
//===============================================================================
// Timer: TCP->UART (TCP->bufi->UART)
// uart0_tx_buf_timer (передача буфера TCP в fifo UART (tx)
//-------------------------------------------------------------------------------
void ICACHE_FLASH_ATTR send_tx_buf(void)
{
	ets_intr_lock(); //	ETS_UART_INTR_DISABLE();
	MEMW();
	UART0_INT_ENA &= ~UART_TXFIFO_EMPTY_INT_ENA; // запретить прерывание по передаче fifo tx
	ets_intr_unlock(); // ETS_UART_INTR_ENABLE();
	os_timer_disarm(&uart0_tx_buf_timer);
   	TCP_SERV_CONN * conn = tcp2uart_conn;
   	if(conn == NULL || conn->pbufi == NULL) return;
	uint8 *pbuf = &conn->pbufi[conn->cntri];
	uint8 *pend = conn->pbufi + conn->sizei;
	int len = 0;
	while(pbuf < pend){
		MEMW();
		if (((UART0_STATUS >> UART_TXFIFO_CNT_S) & UART_TXFIFO_CNT) >= 127) {
			// не всё передано - не лезет в буфер fifo UART tx.
			ets_intr_lock(); //	ETS_UART_INTR_DISABLE();
			MEMW();
	    	UART0_INT_ENA |= UART_TXFIFO_EMPTY_INT_ENA; // установим прерывание на пустой fifo tx
    		ets_intr_unlock(); // ETS_UART_INTR_ENABLE();
			break;
		}
		UART0_FIFO = *pbuf++;
		len++;
	};
	if(len)	{
		conn->cntri += len;
		conn->unrecved_bytes -= len;
		tcp_recved(conn->pcb, len);
	};
}
/*
//-------------------------------------------------------------------------------
// TCP sent_cb (UART->bufo->TCP)
//-------------------------------------------------------------------------------
err_t ICACHE_FLASH_ATTR term_sent_cb(TCP_SERV_CONN *conn) {
#if DEBUGSOO > 3
	tcpsrv_sent_callback_default(conn);
#endif
	tcp2uart_conn = conn;
	loading_rx_buf(); // если в буфере приема UART0 есть новые символы, то передать
#if DEBUGSOO > 3
	os_printf("term_cb: %u # %u\n", tcp2uart_conn->cntro, tcp2uart_conn->pcb->snd_buf);
#endif
	return ERR_OK;
}
*/
//-------------------------------------------------------------------------------
// TCP recv  (TCP->bufi->UART)
//-------------------------------------------------------------------------------
err_t ICACHE_FLASH_ATTR term_recv(TCP_SERV_CONN *conn) {
#if DEBUGSOO > 2
	tcpsrv_received_data_default(conn);
#endif
	tcp2uart_conn = conn;
	send_tx_buf(); // передать в fifo tx UART0
	return ERR_OK;
}
//-------------------------------------------------------------------------------
// TCP listen (Start UART->bufo)
//-------------------------------------------------------------------------------
err_t ICACHE_FLASH_ATTR term_listen(TCP_SERV_CONN *conn) {
#if DEBUGSOO > 1
	tcpsrv_listen_default(conn);
#endif
	// инициировать процесс передачи и приема с UART0 в TCP
	tcp2uart_int_rxtx_disable();
    tcp2uart_conn = conn;
	if(conn->sizeo == 0) { // создать буфер передачи UART->TCP
		if(conn->pbufo == NULL) {
			conn->pbufo = os_malloc(TCP_SRV_SERVER_DEF_TXBUF);
#if DEBUGSOO > 2
			os_printf("memo[%d] %p ", conn->sizeo, conn->pbufo);
#endif
			if (conn->pbufo == NULL)
				return ERR_MEM;
		}
		conn->sizeo = TCP_SRV_SERVER_DEF_TXBUF;
		conn->cntro = 0;
		conn->ptrtx = conn->pbufo;
	}
	ets_intr_lock(); //	ETS_UART_INTR_DISABLE();
    //clear rx and tx fifo, not ready
/*	uint32 conf0 = UART0_CONF0;
    UART0_CONF0 = conf0 | UART_RXFIFO_RST | UART_TXFIFO_RST;
    UART0_CONF0 = conf0 & (~ (UART_RXFIFO_RST | UART_TXFIFO_RST)); */
	update_rts0();
    UART0_INT_ENA |= UART_RXFIFO_FULL_INT_ENA;
	ets_intr_unlock(); // ETS_UART_INTR_ENABLE();
	return ERR_OK;
}
//-------------------------------------------------------------------------------
// TCP disconnect
//-------------------------------------------------------------------------------
void ICACHE_FLASH_ATTR term_disconnect(TCP_SERV_CONN *conn) {
#if DEBUGSOO > 1
	tcpsrv_disconnect_calback_default(conn);
#endif
	tcp2uart_int_rxtx_disable();
}
//-------------------------------------------------------------------------------
// tcp2uart_close
//-------------------------------------------------------------------------------
void ICACHE_FLASH_ATTR tcp2uart_close(void)
{
	syscfg.tcp2uart_port = 0;
	if(tcp2uart_servcfg != NULL) {
		tcp2uart_int_rxtx_disable();
		tcpsrv_close(tcp2uart_servcfg);
		tcp2uart_servcfg = NULL;
		update_mux_uart0();
	}
}
//-------------------------------------------------------------------------------
// tcp2uart_init
//-------------------------------------------------------------------------------
err_t ICACHE_FLASH_ATTR tcp2uart_server_init(uint16 portn) {
	err_t err = ERR_USE;
	if(tcp2uart_servcfg != NULL
	&& (!(tcp2uart_servcfg->flag.client))
	&& tcp2uart_servcfg->port == portn) {
		return  ERR_USE;
	}
	tcp2uart_close();
	if(portn <= ID_CLIENTS_PORT) return ERR_OK;
	TCP_SERV_CFG *p = tcpsrv_init(portn);
	if (p != NULL) {
		tcp2uart_int_rxtx_disable();
		// изменим конфиг на наше усмотрение:
		p->flag.rx_buf = 1; // прием в буфер с его автосозданием.
		p->flag.nagle_disabled = 1; // отмена nagle
		p->max_conn = 1; // одно соединение (порт UART не многопользовательский!)
		p->time_wait_rec = syscfg.tcp2uart_twrec; // =0 -> вечное ожидание
		p->time_wait_cls = syscfg.tcp2uart_twcls; // =0 -> вечное ожидание
#if DEBUGSOO > 0
		os_printf("Max connection %d, time waits %d & %d, min heap size %d\n",
				p->max_conn, p->time_wait_rec, p->time_wait_cls, p->min_heap);
#endif
		p->func_discon_cb = term_disconnect;
		p->func_listen = term_listen;
		p->func_sent_cb = NULL;
//		p->func_sent_cb = term_sent_cb;
		p->func_recv = term_recv;
		err = tcpsrv_start(p);
		if (err != ERR_OK) {
			tcpsrv_close(p);
			p = NULL;
		}
		else  {
			syscfg.tcp2uart_port = portn;
			update_mux_uart0();
		}
	}
	else err = ERR_USE;
	tcp2uart_servcfg = p;
	return err;
}
//-------------------------------------------------------------------------------
// tcp2uart_client
//-------------------------------------------------------------------------------
err_t ICACHE_FLASH_ATTR tcp2uart_client_init(uint32 ip, uint16 portn)
{
	err_t err = ERR_USE;
	if(tcp2uart_servcfg != NULL
	&& tcp2uart_servcfg->flag.client
	&& tcp2uart_servcfg->conn_links != NULL
	&& tcp2uart_servcfg->conn_links->remote_ip.dw == ip
	&& tcp2uart_servcfg->conn_links->remote_port == portn) {
		return  err;
	}
	tcp2uart_close();
	if(portn <= ID_CLIENTS_PORT) return ERR_OK;
	TCP_SERV_CFG * p = tcpsrv_init_client1();
	if (p != NULL) {
		tcp2uart_int_rxtx_disable();
		// изменим конфиг на наше усмотрение:
		p->flag.rx_buf = 1; // прием в буфер с его автосозданием.
		p->flag.nagle_disabled = 1; // отмена nagle
		p->flag.client_reconnect = 1; // вечный реконнект
		p->max_conn = 0; // =0 - вечная попытка соединения
		p->time_wait_rec = syscfg.tcp2uart_twrec; // =0 -> вечное ожидание
		p->time_wait_cls = syscfg.tcp2uart_twcls; // =0 -> вечное ожидание
#if DEBUGSOO > 0
		os_printf("Max retry connection %d, time waits %d & %d, min heap size %d\n",
				p->max_conn, p->time_wait_rec, p->time_wait_cls, p->min_heap);
#endif
		p->func_discon_cb = term_disconnect;
		p->func_listen = term_listen;
		p->func_sent_cb = NULL;
//		p->func_sent_cb = term_sent_cb;
		p->func_recv = term_recv;
		err = tcpsrv_client_start(p, ip, portn);
		if (err != ERR_OK) {
			tcpsrv_close(p);
			p = NULL;
		}
		else  {
			syscfg.tcp2uart_port = portn;
			update_mux_uart0();
		}
	}
	tcp2uart_servcfg = p;
	return err;
}

//-------------------------------------------------------------------------------
// tcp2uart_start
//-------------------------------------------------------------------------------
err_t ICACHE_FLASH_ATTR tcp2uart_start(uint16 newportn)
{
		ip_addr_t ip;
		ip.addr = 0;
		if(newportn <= ID_CLIENTS_PORT) {
			tcp2uart_close();
			return ERR_OK;
		}
		if(tcp2uart_url != NULL) {
			ipaddr_aton(tcp2uart_url, &ip);
			if(ip.addr != 0x0100007f && ip.addr != 0) {
#if DEBUGSOO > 1
				os_printf("TCP2UART client ip:" IPSTR ", port: %u\n", IP2STR(&ip), newportn);
#endif
				return tcp2uart_client_init(ip.addr, newportn);
			}
		}
		return tcp2uart_server_init(newportn);
}
