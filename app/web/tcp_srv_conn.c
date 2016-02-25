/******************************************************************************
 * FileName: tcp_srv_conn.c
 * TCP сервачек для ESP8266
 * PV` ver1.0 20/12/2014
 ******************************************************************************/
#include "user_config.h"
#include "bios.h"
#include "sdk/add_func.h"
#include "osapi.h"
#include "user_interface.h"
#include "lwip/tcp.h"
#include "lwip/tcp_impl.h"
#include "lwip/memp.h"
#include "flash_eep.h"
#include "tcp_srv_conn.h"
#include "web_iohw.h"

#include "wifi.h"

#if 0
#undef DEBUGSOO
#define DEBUGSOO 4
#endif
// Lwip funcs - http://www.ecoscentric.com/ecospro/doc/html/ref/lwip.html

TCP_SERV_CFG *phcfg DATA_IRAM_ATTR; // = NULL; // начальный указатель в памяти на структуры открытых сервачков

#if DEBUGSOO > 0
const uint8 txt_tcpsrv_NULL_pointer[] ICACHE_RODATA_ATTR = "tcpsrv: NULL pointer!\n";
const uint8 txt_tcpsrv_already_initialized[] ICACHE_RODATA_ATTR = "tcpsrv: already initialized!\n";
const uint8 txt_tcpsrv_out_of_mem[] ICACHE_RODATA_ATTR = "tcpsrv: out of mem!\n";
#endif

#define mMIN(a, b)  ((a<b)?a:b)
// пред.описание...
static void tcpsrv_list_delete(TCP_SERV_CONN * ts_conn) ICACHE_FLASH_ATTR;
static void tcpsrv_disconnect_successful(TCP_SERV_CONN * ts_conn) ICACHE_FLASH_ATTR;
static void tcpsrv_close_cb(TCP_SERV_CONN * ts_conn) ICACHE_FLASH_ATTR;
static void tcpsrv_server_close(TCP_SERV_CONN * ts_conn) ICACHE_FLASH_ATTR;
static err_t tcpsrv_server_poll(void *arg, struct tcp_pcb *pcb) ICACHE_FLASH_ATTR;
static void tcpsrv_error(void *arg, err_t err) ICACHE_FLASH_ATTR;
static err_t tcpsrv_connected(void *arg, struct tcp_pcb *tpcb, err_t err) ICACHE_FLASH_ATTR;
static void tcpsrv_client_connect(TCP_SERV_CONN * ts_conn) ICACHE_FLASH_ATTR;
static void tcpsrv_client_reconnect(TCP_SERV_CONN * ts_conn) ICACHE_FLASH_ATTR;
/******************************************************************************
 * FunctionName : tcpsrv_print_remote_info
 * Description  : выводит remote_ip:remote_port [conn_count] os_printf("srv x.x.x.x:x [n] ")
 * Parameters   : TCP_SERV_CONN * ts_conn
 * Returns      : none
 *******************************************************************************/
void ICACHE_FLASH_ATTR tcpsrv_print_remote_info(TCP_SERV_CONN *ts_conn) {
//#if DEBUGSOO > 0
	uint16 port;
	if(ts_conn->pcb != NULL) port = ts_conn->pcb->local_port;
//	else if(ts_conn->flag.client) port = ?;
	else port = ts_conn->pcfg->port;
	os_printf("srv[%u] " IPSTR ":%d [%d] ", port,
			ts_conn->remote_ip.b[0], ts_conn->remote_ip.b[1],
			ts_conn->remote_ip.b[2], ts_conn->remote_ip.b[3],
			ts_conn->remote_port, ts_conn->pcfg->conn_count);
//#endif
}
/******************************************************************************
 * Demo functions
 ******************************************************************************/
//------------------------------------------------------------------------------
void ICACHE_FLASH_ATTR tcpsrv_disconnect_calback_default(TCP_SERV_CONN *ts_conn) {
	ts_conn->pcb = NULL;
#if DEBUGSOO > 1
	tcpsrv_print_remote_info(ts_conn);
	os_printf("disconnect\n");
#endif
}
//------------------------------------------------------------------------------
err_t ICACHE_FLASH_ATTR tcpsrv_listen_default(TCP_SERV_CONN *ts_conn) {
#if DEBUGSOO > 1
	tcpsrv_print_remote_info(ts_conn);
	os_printf("listen\n");
#endif
	return ERR_OK;
}
//------------------------------------------------------------------------------
err_t ICACHE_FLASH_ATTR tcpsrv_connected_default(TCP_SERV_CONN *ts_conn) {
#if DEBUGSOO > 1
	tcpsrv_print_remote_info(ts_conn);
	os_printf("connected\n");
#endif
	return ERR_OK;
}
//------------------------------------------------------------------------------
err_t ICACHE_FLASH_ATTR tcpsrv_sent_callback_default(TCP_SERV_CONN *ts_conn) {
#if DEBUGSOO > 1
	tcpsrv_print_remote_info(ts_conn);
	os_printf("sent_cb\n");
#endif
	return ERR_OK;
}
//------------------------------------------------------------------------------
err_t ICACHE_FLASH_ATTR tcpsrv_received_data_default(TCP_SERV_CONN *ts_conn) {
#if DEBUGSOO > 1
	tcpsrv_print_remote_info(ts_conn);
	os_printf("received, buffer %d bytes\n", ts_conn->sizei);
#endif
	return ERR_OK;
}
/******************************************************************************
 * FunctionName : find_pcb
 * Description  : поиск pcb в списках lwip
 * Parameters   : TCP_SERV_CONN * ts_conn
 * Returns      : *pcb or NULL
 *******************************************************************************/
struct tcp_pcb * ICACHE_FLASH_ATTR find_tcp_pcb(TCP_SERV_CONN * ts_conn) {
	struct tcp_pcb *pcb;
	uint16 remote_port = ts_conn->remote_port;
	uint16 local_port = ts_conn->pcfg->port;
	uint32 ip = ts_conn->remote_ip.dw;
	for (pcb = tcp_active_pcbs; pcb != NULL; pcb = pcb->next) {
		if ((pcb->remote_port == remote_port) && (pcb->local_port == local_port)
				&& (pcb->remote_ip.addr == ip))
			return pcb;
	}
	for (pcb = tcp_tw_pcbs; pcb != NULL; pcb = pcb->next) {
		if ((pcb->remote_port == remote_port) && (pcb->local_port == local_port)
				&& (pcb->remote_ip.addr == ip))
			return pcb;
	}
	return NULL;
}
/******************************************************************************
 * FunctionName : tcpsrv_disconnect
 * Description  : disconnect
 * Parameters   : TCP_SERV_CONN * ts_conn
 * Returns      : none
 *******************************************************************************/
void ICACHE_FLASH_ATTR tcpsrv_disconnect(TCP_SERV_CONN * ts_conn) {
	if (ts_conn == NULL || ts_conn->state == SRVCONN_CLOSEWAIT) return; // уже закрывается
	ts_conn->pcb = find_tcp_pcb(ts_conn); // ещё жива данная pcb ?
	if (ts_conn->pcb != NULL) {
		tcpsrv_server_close(ts_conn);
	}
}
/******************************************************************************
 * FunctionName : internal fun: tcpsrv_int_sent_data
 * Description  : передача данных (не буферизированная! только передача в tcp Lwip-у)
 * 				  вызывать только из call back с текущим pcb!
 * Parameters   : TCP_SERV_CONN * ts_conn
 *                uint8* psent - буфер с данными
 *                uint16 length - кол-во передаваемых байт
 * Returns      : tcp error
 ******************************************************************************/
err_t ICACHE_FLASH_ATTR tcpsrv_int_sent_data(TCP_SERV_CONN * ts_conn, uint8 *psent, uint16 length) {
	err_t err = ERR_ARG;
	if(ts_conn == NULL) return err;
	if(ts_conn->pcb == NULL || ts_conn->state == SRVCONN_CLOSEWAIT) return ERR_CONN;
	ts_conn->flag.busy_bufo = 1; // буфер bufo занят
	struct tcp_pcb *pcb = ts_conn->pcb;  // find_tcp_pcb(ts_conn);
	if(tcp_sndbuf(pcb) < length) {
#if DEBUGSOO > 1
		os_printf("sent_data length (%u) > sndbuf (%u)!\n", length, tcp_sndbuf(pcb));
#endif
		return err;
	}
	if (length) {
		if(ts_conn->flag.nagle_disabled) tcp_nagle_disable(pcb);
		err = tcp_write(pcb, psent, length, 0);
		if (err == ERR_OK) {
			ts_conn->ptrtx = psent + length;
			ts_conn->cntro -= length;
			ts_conn->flag.wait_sent = 1; // ожидать завершения передачи (sent_cb)
			err = tcp_output(pcb); // передать что влезло
		} else { // ts_conn->state = SRVCONN_CLOSE;
#if DEBUGSOO > 1
			os_printf("tcp_write(%p, %p, %u) = %d! pbuf = %u\n", pcb, psent, length, err, tcp_sndbuf(pcb));
#endif
			ts_conn->flag.wait_sent = 0;
			tcpsrv_server_close(ts_conn);
		};
	} else { // создать вызов tcpsrv_server_sent()
		tcp_nagle_enable(pcb);
		err = tcp_output(pcb); // передать пустое
	}
	ts_conn->flag.busy_bufo = 0; // буфер bufo свободен
	return err;
}
/******************************************************************************
 * FunctionName : tcpsrv_server_sent
 * Description  : Data has been sent and acknowledged by the remote host.
 * This means that more data can be sent.
 * Parameters   : arg -- Additional argument to pass to the callback function
 *                pcb -- The connection pcb for which data has been acknowledged
 *                len -- The amount of bytes acknowledged
 * Returns      : ERR_OK: try to send some data by calling tcp_output
 *                ERR_ABRT: if you have called tcp_abort from within the function!
 ******************************************************************************/
static err_t ICACHE_FLASH_ATTR tcpsrv_server_sent(void *arg, struct tcp_pcb *pcb, u16_t len) {
	sint8 ret_err = ERR_OK;
	TCP_SERV_CONN * ts_conn = arg;
	if (ts_conn == NULL || pcb == NULL)	return ERR_ARG;
	ts_conn->pcb = pcb; // запомнить pcb
	ts_conn->state = SRVCONN_CONNECT;
	ts_conn->recv_check = 0;
	ts_conn->flag.wait_sent = 0; // блок передан
	if ((ts_conn->flag.tx_null == 0)
			&& (ts_conn->pcfg->func_sent_cb != NULL)) {
		ret_err = ts_conn->pcfg->func_sent_cb(ts_conn);
	}
	return ret_err;
}
/******************************************************************************
 *
 ******************************************************************************/
static err_t ICACHE_FLASH_ATTR recv_trim_bufi(TCP_SERV_CONN * ts_conn, uint32 newadd)
{
	uint32 len = 0;
	ts_conn->flag.busy_bufi = 1; // идет обработка bufi
	if(newadd) {
		if(ts_conn->pbufi != NULL && (ts_conn->flag.rx_buf) && ts_conn->cntri < ts_conn->sizei) {
				len = ts_conn->sizei - ts_conn->cntri; // размер необработанных данных
				if(ts_conn->cntri > newadd) {
					os_memcpy(ts_conn->pbufi, &ts_conn->pbufi[ts_conn->cntri], len );
					ts_conn->sizei = len;
					len += newadd;
					ts_conn->pbufi = (uint8 *)mem_realloc(ts_conn->pbufi, len + 1);	//mem_trim(ts_conn->pbufi, len);
					if(ts_conn->pbufi == NULL) {
#if DEBUGSOO > 2
						os_printf("memtrim err %p[%d]  ", ts_conn->pbufi, len + 1);
#endif
						return ERR_MEM;
					}
#if DEBUGSOO > 2
					os_printf("memi%p[%d] ", ts_conn->pbufi,  len);
#endif
					ts_conn->pbufi[len] = '\0'; // вместо os_zalloc;
					ts_conn->cntri = 0;
					ts_conn->flag.busy_bufi = 0; // обработка bufi окончена
					return ERR_OK;
				}
		}
		else {
			// не тот режим -> удалить неизвестный буфер
			os_free(ts_conn->pbufi);
			ts_conn->pbufi = NULL;
		}
		uint8 * newbufi = (uint8 *) os_malloc(len + newadd + 1); // подготовка буфера
		if (newbufi == NULL) {
#if DEBUGSOO > 2
			os_printf("memerr %p[%d]  ", len + newadd, ts_conn->pbufi);
#endif
			ts_conn->flag.busy_bufi = 0; // обработка bufi окончена
			return ERR_MEM;
		}
#if DEBUGSOO > 2
		os_printf("memi%p[%d] ", ts_conn->pbufi,  len + newadd);
#endif
		newbufi[len + newadd] = '\0'; // вместо os_zalloc
		if(len)	{
			os_memcpy(newbufi, &ts_conn->pbufi[ts_conn->cntri], len);
			os_free(ts_conn->pbufi);
		};
		ts_conn->pbufi = newbufi;
//		ts_conn->cntri = 0;
		ts_conn->sizei = len;
	}
	else {
		if((!ts_conn->flag.rx_buf) || ts_conn->cntri >= ts_conn->sizei)  {
			ts_conn->sizei = 0;
			if (ts_conn->pbufi != NULL) {
				os_free(ts_conn->pbufi);  // освободить буфер.
				ts_conn->pbufi = NULL;
			};
		}
		else if(ts_conn->cntri) {
			len = ts_conn->sizei - ts_conn->cntri;
			os_memcpy(ts_conn->pbufi, &ts_conn->pbufi[ts_conn->cntri], len );
			ts_conn->sizei = len;
			ts_conn->pbufi = (uint8 *)mem_realloc(ts_conn->pbufi, len + 1);	//mem_trim(ts_conn->pbufi, len);
			if(ts_conn->pbufi == NULL) {
#if DEBUGSOO > 2
				os_printf("memtrim err %p[%d]  ", ts_conn->pbufi, len + 1);
#endif
				return ERR_MEM;
			}
			ts_conn->pbufi[len] = '\0'; // вместо os_zalloc;
		}
//		ts_conn->cntri = 0;
	}
	ts_conn->cntri = 0;
	ts_conn->flag.busy_bufi = 0; // обработка bufi окончена
	return ERR_OK;
}
/******************************************************************************
 * FunctionName : tcpsrv_server_recv
 * Description  : Data has been received on this pcb.
 * Parameters   : arg -- Additional argument to pass to the callback function
 *                pcb -- The connection pcb which received data
 *                p -- The received data (or NULL when the connection has been closed!)
 *                err -- An error code if there has been an error receiving
 * Returns      : ERR_ABRT: if you have called tcp_abort from within the function!
 ******************************************************************************/
static err_t ICACHE_FLASH_ATTR tcpsrv_server_recv(void *arg, struct tcp_pcb *pcb, struct pbuf *p, err_t err) {
	// Sets the callback function that will be called when new data arrives on the connection associated with pcb.
	// The callback function will be passed a NULL pbuf to indicate that the remote host has closed the connection.
	TCP_SERV_CONN * ts_conn = arg;
	if (ts_conn == NULL) return ERR_ARG;

	if(syscfg.cfg.b.hi_speed_enable) set_cpu_clk();

	ts_conn->pcb = pcb;
	if (p == NULL || err != ERR_OK) { // the remote host has closed the connection.
		tcpsrv_server_close(ts_conn);
		return err;
	};
	// если нет функции обработки или ожидаем закрытия соединения, то принимаем данные в трубу
	if ((ts_conn->flag.rx_null != 0) || (ts_conn->pcfg->func_recv == NULL)
			|| (ts_conn->state == SRVCONN_CLOSEWAIT)) { // соединение закрыто? нет.
		tcp_recved(pcb, p->tot_len + ts_conn->unrecved_bytes); // сообщает стеку, что съели len и можно посылать ACK и принимать новые данные.
		ts_conn->unrecved_bytes = 0;
#if DEBUGSOO > 3
		os_printf("rec_null %d of %d\n", ts_conn->cntri, p->tot_len);
#endif
		pbuf_free(p); // данные выели
		return ERR_OK;
	};
	ts_conn->state = SRVCONN_CONNECT; // был прием
	ts_conn->recv_check = 0; // счет времени до авто-закрытия с нуля
	if(p->tot_len) {
		err = recv_trim_bufi(ts_conn, p->tot_len);
		if(err != ERR_OK) return err;
		// добавление всего что отдал Lwip в буфер
		uint32 len = pbuf_copy_partial(p, &ts_conn->pbufi[ts_conn->sizei], p->tot_len, 0);
		ts_conn->sizei += len;
		pbuf_free(p); // все данные выели
		if(!ts_conn->flag.rx_buf) {
			 tcp_recved(pcb, len); // сообщает стеку, что съели len и можно посылать ACK и принимать новые данные.
		}
		else ts_conn->unrecved_bytes += len;
#if DEBUGSOO > 3
		os_printf("rec %d of %d :\n", p->tot_len, ts_conn->sizei);
#endif
		err = ts_conn->pcfg->func_recv(ts_conn);
		err_t err2 = recv_trim_bufi(ts_conn, 0);
		if(err2 != ERR_OK) return err2;
	}
	return err;
}
/******************************************************************************
 * FunctionName : tcpsrv_unrecved_win
 * Description  : Update the TCP window.
 * This can be used to throttle data reception (e.g. when received data is
 * programmed to flash and data is received faster than programmed).
 * Parameters   : TCP_SERV_CONN * ts_conn
 * Returns      : none
 * После включения throttle, будет принято до 5840 (MAX WIN) + 1460 (MSS) байт?
 ******************************************************************************/
void ICACHE_FLASH_ATTR tcpsrv_unrecved_win(TCP_SERV_CONN *ts_conn) {
	if (ts_conn->unrecved_bytes) {
		// update the TCP window
#if DEBUGSOO > 3
		os_printf("recved_bytes=%d\n", ts_conn->unrecved_bytes);
#endif
#if 1
		if(ts_conn->pcb != NULL) tcp_recved(ts_conn->pcb, ts_conn->unrecved_bytes);
#else
		struct tcp_pcb *pcb = find_tcp_pcb(ts_conn); // уже закрыто?
		if(pcb != NULL)	tcp_recved(ts_conn->pcb, ts_conn->unrecved_bytes);
#endif
	}
	ts_conn->unrecved_bytes = 0;
}
/******************************************************************************
 * FunctionName : tcpsrv_disconnect
 * Description  : disconnect with host
 * Parameters   : arg -- Additional argument to pass to the callback function
 * Returns      : none
 ******************************************************************************/
static void ICACHE_FLASH_ATTR tcpsrv_disconnect_successful(TCP_SERV_CONN * ts_conn) {
	struct tcp_pcb *pcb = ts_conn->pcb;
	if (pcb != NULL
			&& pcb->state == TIME_WAIT
			&& ts_conn->flag.pcb_time_wait_free
			&& syscfg.cfg.b.web_time_wait_delete
			&& ts_conn->pcfg->flag.pcb_time_wait_free ) { // убить TIME_WAIT?
		// http://www.serverframework.com/asynchronousevents/2011/01/time-wait-and-its-design-implications-for-protocols-and-scalable-servers.html
#if DEBUGSOO > 3
		tcpsrv_print_remote_info(ts_conn);
		os_printf("tcp_pcb_remove!\n");
#endif
		tcp_pcb_remove(&tcp_tw_pcbs, pcb);
		memp_free(MEMP_TCP_PCB, pcb);
	};
	// remove the node from the server's connection list
	if(ts_conn->flag.client && ts_conn->flag.client_reconnect) tcpsrv_client_reconnect(ts_conn);
	else tcpsrv_list_delete(ts_conn); // remove the node from the server's connection list
}
/******************************************************************************
 * FunctionName : tcpsrv_close_cb
 * Description  : The connection has been successfully closed.
 * Parameters   : arg -- Additional argument to pass to the callback function
 * Returns      : none
 ******************************************************************************/
static void ICACHE_FLASH_ATTR tcpsrv_close_cb(TCP_SERV_CONN * ts_conn) {
//	if (ts_conn == NULL) return;
	struct tcp_pcb *pcb = find_tcp_pcb(ts_conn); // ts_conn->pcb;
	ts_conn->pcb = pcb;
	if (pcb == NULL || pcb->state == CLOSED || pcb->state == TIME_WAIT) {
		/*remove the node from the server's active connection list*/
		tcpsrv_disconnect_successful(ts_conn);
	} else {
		if (++ts_conn->recv_check > TCP_SRV_CLOSE_WAIT) { // счет до принудительного закрытия  120*0.25 = 30 cек
#if DEBUGSOO > 2
			tcpsrv_print_remote_info(ts_conn);
			os_printf("tcp_abandon!\n");
#endif
			tcp_poll(pcb, NULL, 0);
			tcp_err(pcb, NULL);
			tcp_abandon(pcb, 0);
			ts_conn->pcb = NULL;
			// remove the node from the server's active connection list
			tcpsrv_disconnect_successful(ts_conn);
		} else
			os_timer_arm(&ts_conn->ptimer, TCP_FAST_INTERVAL*2, 0); // ждем ещё 250 ms
	}
}
/******************************************************************************
 * FunctionName : tcpsrv_server_close
 * Description  : The connection shall be actively closed.
 * Parameters   : arg -- Additional argument to pass to the callback function
 *                pcb -- the pcb to close
 * Returns      : none
 ******************************************************************************/
static void ICACHE_FLASH_ATTR tcpsrv_server_close(TCP_SERV_CONN * ts_conn) {

	struct tcp_pcb *pcb = ts_conn->pcb;
//	if(pcb == NULL) return;
	ts_conn->state = SRVCONN_CLOSEWAIT;
	ts_conn->recv_check = 0;

	ts_conn->flag.wait_sent = 0; // блок передан
	ts_conn->flag.rx_null = 1; // отключение вызова func_received_data() и прием в null
	ts_conn->flag.tx_null = 1; // отключение вызова func_sent_callback() и передача в null
	tcp_recv(pcb, NULL); // отключение приема
	tcp_sent(pcb, NULL); // отключение передачи
	tcp_poll(pcb, NULL, 0); // отключение poll
	if (ts_conn->pbufo != NULL) {
		os_free(ts_conn->pbufo);
		ts_conn->pbufo = NULL;
	}
	ts_conn->sizeo = 0;
	ts_conn->cntro = 0;
	if (ts_conn->pbufi != NULL)	{
		os_free(ts_conn->pbufi);
		ts_conn->pbufi = NULL;
	}
	ts_conn->sizei = 0;
	ts_conn->cntri = 0;
	if(ts_conn->unrecved_bytes) {
		tcp_recved(ts_conn->pcb, ts_conn->unrecved_bytes);
		ts_conn->unrecved_bytes = 0;
	}
	tcp_recved(ts_conn->pcb, TCP_WND);
	err_t err = tcp_close(pcb); // послать закрытие соединения
	// The function may return ERR_MEM if no memory was available for closing the connection.
	// If so, the application should wait and try again either by using the acknowledgment callback or the polling functionality.
	// If the close succeeds, the function returns ERR_OK.
	os_timer_disarm(&ts_conn->ptimer);
	os_timer_setfn(&ts_conn->ptimer, (ETSTimerFunc *)tcpsrv_close_cb, ts_conn);
	if (err != ERR_OK) {
#if DEBUGSOO > 1
		tcpsrv_print_remote_info(ts_conn);
		os_printf("+ncls+\n", pcb);
#endif
		// closing failed, try again later
	}
//	else { } // closing succeeded
	os_timer_arm(&ts_conn->ptimer, TCP_FAST_INTERVAL*2, 0); // если менее, то Lwip не успевает pcb->state = TIME_WAIT ?
}
/******************************************************************************
 * FunctionName : espconn_server_poll
 * Description  : The poll function is called every 1 second.
 * If there has been no data sent (which resets the retries) in time_wait_rec seconds, close.
 * If the last portion of a file has not been recv/sent in time_wait_cls seconds, close.
 *
 * This could be increased, but we don't want to waste resources for bad connections.
 * Parameters   : arg -- Additional argument to pass to the callback function
 *                pcb -- The connection pcb for which data has been acknowledged
 * Returns      : ERR_OK: try to send some data by calling tcp_output
 *                ERR_ABRT: if you have called tcp_abort from within the function!
 *******************************************************************************/
static err_t ICACHE_FLASH_ATTR tcpsrv_server_poll(void *arg, struct tcp_pcb *pcb) {
	TCP_SERV_CONN * ts_conn = arg;
	if (ts_conn == NULL) {
#if DEBUGSOO > 3
		os_printf("poll, ts_conn = NULL! - abandon\n");
#endif
		tcp_poll(pcb, NULL, 0);
		tcp_abandon(pcb, 0);
		return ERR_ABRT;
	}
#if DEBUGSOO > 3
	tcpsrv_print_remote_info(ts_conn);
	os_printf("poll: %d #%d, %d,%d\n", ts_conn->recv_check, pcb->state, ts_conn->pcfg->time_wait_rec, ts_conn->pcfg->time_wait_cls);
#endif
	if (ts_conn->state != SRVCONN_CLOSEWAIT) {
		ts_conn->pcb = pcb;
		if (pcb->state == ESTABLISHED) {
			if (ts_conn->state == SRVCONN_LISTEN) {
				if ((ts_conn->pcfg->time_wait_rec)
					&& (++ts_conn->recv_check > ts_conn->pcfg->time_wait_rec))
					tcpsrv_server_close(ts_conn);
			}
			else if (ts_conn->state == SRVCONN_CONNECT) {
				if ((ts_conn->pcfg->time_wait_cls)
					&& (++ts_conn->recv_check > ts_conn->pcfg->time_wait_cls))
					tcpsrv_server_close(ts_conn);
			}
		} else
			tcpsrv_server_close(ts_conn);
	} else
		tcp_poll(pcb, NULL, 0); // отключить tcpsrv_server_poll
	return ERR_OK;
}
/******************************************************************************
 * FunctionName : tcpsrv_list_delete
 * Description  : remove the node from the connection list
 * Parameters   : ts_conn
 * Returns      : none
 *******************************************************************************/
static void ICACHE_FLASH_ATTR tcpsrv_list_delete(TCP_SERV_CONN * ts_conn) {
//	if (ts_conn == NULL) return;
	if(ts_conn->state != SRVCONN_CLOSED) {
		ts_conn->state = SRVCONN_CLOSED; // исключить повторное вхождение из запросов в func_discon_cb()
		if (ts_conn->pcfg->func_discon_cb != NULL)
			ts_conn->pcfg->func_discon_cb(ts_conn);
		if(phcfg == NULL || ts_conn->pcfg == NULL) return;  // если в func_discon_cb() было вызвано tcpsrv_close_all() и т.д.
	}
	TCP_SERV_CONN ** p = &ts_conn->pcfg->conn_links;
	TCP_SERV_CONN *tcpsrv_cmp = ts_conn->pcfg->conn_links;
	while (tcpsrv_cmp != NULL) {
		if (tcpsrv_cmp == ts_conn) {
			*p = ts_conn->next;
			ts_conn->pcfg->conn_count--;
			os_timer_disarm(&ts_conn->ptimer);
			if (ts_conn->linkd != NULL) {
				os_free(ts_conn->linkd);
				ts_conn->linkd = NULL;
			}
			if (ts_conn->pbufo != NULL) {
				os_free(ts_conn->pbufo);
				ts_conn->pbufo = NULL;
			}
			if (ts_conn->pbufi != NULL) {
				os_free(ts_conn->pbufi);
				ts_conn->pbufi = NULL;
			}
			os_free(ts_conn);
			return; // break;
		}
		p = &tcpsrv_cmp->next;
		tcpsrv_cmp = tcpsrv_cmp->next;
	};
}
/******************************************************************************
 *******************************************************************************/
 static void ICACHE_FLASH_ATTR tcpsrv_client_reconnect(TCP_SERV_CONN * ts_conn)
{
	if (ts_conn != NULL) {
		os_timer_disarm(&ts_conn->ptimer);
		if(ts_conn->state != SRVCONN_CLIENT) { // не установка соединения (клиент)?
#if DEBUGSOO > 3
			os_printf("Client free\n");
#endif
			if(ts_conn->state != SRVCONN_CLOSED) {
				ts_conn->state = SRVCONN_CLOSED; // исключить повторное вхождение из запросов в func_discon_cb()
				if (ts_conn->pcfg->func_discon_cb != NULL) ts_conn->pcfg->func_discon_cb(ts_conn);
			}
			if (ts_conn->linkd != NULL) {
				os_free(ts_conn->linkd);
				ts_conn->linkd = NULL;
			}
			if (ts_conn->pbufo != NULL) {
				os_free(ts_conn->pbufo);
				ts_conn->pbufo = NULL;
			}
			if (ts_conn->pbufi != NULL) {
				os_free(ts_conn->pbufi);
				ts_conn->pbufi = NULL;
			}
			ts_conn->cntri = 0;
			ts_conn->cntro = 0;
			ts_conn->sizei = 0;
			ts_conn->sizeo = 0;
			ts_conn->unrecved_bytes = 0;
			ts_conn->ptrtx = NULL;
			ts_conn->flag = ts_conn->pcfg->flag;
		}
#if DEBUGSOO > 1
		os_printf("Waiting next connection %u ms...\n", TCP_CLIENT_NEXT_CONNECT_MS);
#endif
		os_timer_setfn(&ts_conn->ptimer, (ETSTimerFunc *)tcpsrv_client_connect, ts_conn);
		os_timer_arm(&ts_conn->ptimer, TCP_CLIENT_NEXT_CONNECT_MS, 0); // попробовать соединиться через 5 секунды
	}
}
/******************************************************************************
 * FunctionName : tcpsrv_error (server and client)
 * Description  : The pcb had an error and is already deallocated.
 *		The argument might still be valid (if != NULL).
 * Parameters   : arg -- Additional argument to pass to the callback function
 *		err -- Error code to indicate why the pcb has been closed
 * Returns      : none
 *******************************************************************************/
#if DEBUGSOO > 2
static const char srvContenErr00[] ICACHE_RODATA_ATTR = "Ok";						// ERR_OK          0
static const char srvContenErr01[] ICACHE_RODATA_ATTR = "Out of memory error";		// ERR_MEM        -1
static const char srvContenErr02[] ICACHE_RODATA_ATTR = "Buffer error";				// ERR_BUF        -2
static const char srvContenErr03[] ICACHE_RODATA_ATTR = "Timeout";					// ERR_TIMEOUT    -3
static const char srvContenErr04[] ICACHE_RODATA_ATTR = "Routing problem";			// ERR_RTE        -4
static const char srvContenErr05[] ICACHE_RODATA_ATTR = "Operation in progress";	// ERR_INPROGRESS -5
static const char srvContenErr06[] ICACHE_RODATA_ATTR = "Illegal value";			// ERR_VAL        -6
static const char srvContenErr07[] ICACHE_RODATA_ATTR = "Operation would block";	// ERR_WOULDBLOCK -7
static const char srvContenErr08[] ICACHE_RODATA_ATTR = "Connection aborted";		// ERR_ABRT       -8
static const char srvContenErr09[] ICACHE_RODATA_ATTR = "Connection reset";			// ERR_RST        -9
static const char srvContenErr10[] ICACHE_RODATA_ATTR = "Connection closed";		// ERR_CLSD       -10
static const char srvContenErr11[] ICACHE_RODATA_ATTR = "Not connected";			// ERR_CONN       -11
static const char srvContenErr12[] ICACHE_RODATA_ATTR = "Illegal argument";			// ERR_ARG        -12
static const char srvContenErr13[] ICACHE_RODATA_ATTR = "Address in use";			// ERR_USE        -13
static const char srvContenErr14[] ICACHE_RODATA_ATTR = "Low-level netif error";	// ERR_IF         -14
static const char srvContenErr15[] ICACHE_RODATA_ATTR = "Already connected";		// ERR_ISCONN     -15
static const char srvContenErrX[] ICACHE_RODATA_ATTR = "?";
const char * srvContenErr[]  =  {
	srvContenErr00,
	srvContenErr01,
	srvContenErr02,
	srvContenErr03,
	srvContenErr04,
	srvContenErr05,
	srvContenErr06,
	srvContenErr07,
	srvContenErr08,
	srvContenErr09,
	srvContenErr10,
	srvContenErr11,
	srvContenErr12,
	srvContenErr13,
	srvContenErr14,
	srvContenErr15
};
#endif
static void ICACHE_FLASH_ATTR tcpsrv_error(void *arg, err_t err) {
	TCP_SERV_CONN * ts_conn = arg;
//	struct tcp_pcb *pcb = NULL;
	if (ts_conn != NULL) {
#if DEBUGSOO > 2
		if(system_get_os_print()) {
			tcpsrv_print_remote_info(ts_conn);
			char serr[24];
			if((err > -16) && (err < 1)) {
				ets_memcpy(serr, srvContenErr[-err], 24);
			}
			else {
				serr[0] = '?';
				serr[1] = '\0';
			}
			os_printf("error %d (%s)\n", err, serr);
		}
#elif DEBUGSOO > 1
		tcpsrv_print_remote_info(ts_conn);
		os_printf("error %d\n", err);
#endif
		if (ts_conn->state != SRVCONN_CLOSEWAIT) {
			if(ts_conn->flag.client &&
			(ts_conn->flag.client_reconnect
					|| (ts_conn->state == SRVCONN_CLIENT
							&& (ts_conn->pcfg->max_conn == 0
									|| ts_conn->recv_check < ts_conn->pcfg->max_conn)))) {
				ts_conn->recv_check++;
#if DEBUGSOO > 3
				os_printf("go_reconnect\n");
#endif
				tcpsrv_client_reconnect(ts_conn);
			}
			else tcpsrv_list_delete(ts_conn); // remove the node from the server's connection list
		};
	}
}

/******************************************************************************
 * FunctionName : tcpsrv_client_connect
 * Returns      : ts_conn->pcb
 *******************************************************************************/
static void ICACHE_FLASH_ATTR tcpsrv_client_connect(TCP_SERV_CONN * ts_conn)
{
#if DEBUGSOO > 3
		os_printf("tcpsrv_client_connect()\n");
#endif
		if (ts_conn != NULL) {
		struct tcp_pcb *pcb;
		if(ts_conn->pcb != NULL) {
			pcb = find_tcp_pcb(ts_conn);
			if(pcb != NULL) {
				tcp_abandon(ts_conn->pcb, 0);
//				ts_conn->pcb = NULL;
			}
		}
		pcb = tcp_new();
		if(pcb != NULL) {
			ts_conn->pcb = pcb;
			ts_conn->state = SRVCONN_CLIENT; // установка соединения (клиент)
			ts_conn->recv_check = 0;
			err_t err = tcp_bind(pcb, IP_ADDR_ANY, 0); // Binds pcb to a local IP address and new port number. // &netif_default->ip_addr
#if DEBUGSOO > 2
			os_printf("tcp_bind() = %d\n", err);
#endif
			if (err == ERR_OK) { // If another connection is bound to the same port, the function will return ERR_USE, otherwise ERR_OK is returned.
				ts_conn->pcfg->port = ts_conn->pcb->local_port;
				tcp_arg(pcb, ts_conn); // Allocate client-specific session structure, set as callback argument
				// Set up the various callback functions
				tcp_err(pcb, tcpsrv_error);
				err = tcp_connect(pcb, (ip_addr_t *)&ts_conn->remote_ip, ts_conn->remote_port, tcpsrv_connected);
#if DEBUGSOO > 2
				os_printf("tcp_connect() = %d\n", err);
#endif
				if(err == ERR_OK) {
#if DEBUGSOO > 1
					tcpsrv_print_remote_info(ts_conn);
					os_printf("start client - Ok\n");
#endif
					return;
				}
			}
			tcp_abandon(pcb, 0);
		}
		ts_conn->pcb = NULL;
	}
}
/******************************************************************************
 tcpsrv_connected_fn (client)
 *******************************************************************************/
static err_t ICACHE_FLASH_ATTR tcpsrv_connected(void *arg, struct tcp_pcb *tpcb, err_t err)
{
	TCP_SERV_CONN * ts_conn = arg;
	err_t merr = ERR_OK;
	if (ts_conn != NULL) {
		os_timer_disarm(&ts_conn->ptimer);
		tcp_err(tpcb, tcpsrv_error);
		ts_conn->state = SRVCONN_LISTEN;
		ts_conn->recv_check = 0;
		tcp_sent(tpcb, tcpsrv_server_sent);
		tcp_recv(tpcb, tcpsrv_server_recv);
		tcp_poll(tpcb, tcpsrv_server_poll, 8); // every 1 seconds
		if(ts_conn->pcfg->func_listen != NULL) merr = ts_conn->pcfg->func_listen(ts_conn);
		else {
#if DEBUGSOO > 2
			if(system_get_os_print()) {
				tcpsrv_print_remote_info(ts_conn);
				char serr[24];
				if((err > -16) && (err < 1)) {
					ets_memcpy(serr, srvContenErr[-err], 24);
				}
				else {
					serr[0] = '?';
					serr[1] = '\0';
				}
				os_printf("error %d (%s)\n", err, serr);
			}
#elif DEBUGSOO > 1
			tcpsrv_print_remote_info(ts_conn);
			os_printf("connected, error %d\n", err);
#endif
			// test
			// tcpsrv_int_sent_data(ts_conn, wificonfig.st.config.password, os_strlen(wificonfig.st.config.password));
		}
	}
	return merr;
}
/******************************************************************************
 * FunctionName : tcpsrv_tcp_accept
 * Description  : A new incoming connection has been accepted.
 * Parameters   : arg -- Additional argument to pass to the callback function
 *		pcb -- The connection pcb which is accepted
 *		err -- An unused error code, always ERR_OK currently
 * Returns      : acception result
 *******************************************************************************/
static err_t ICACHE_FLASH_ATTR tcpsrv_server_accept(void *arg, struct tcp_pcb *pcb, err_t err) {
	struct tcp_pcb_listen *lpcb = (struct tcp_pcb_listen*) arg;
	TCP_SERV_CFG *p = tcpsrv_server_port2pcfg(pcb->local_port);
	TCP_SERV_CONN * ts_conn;

	if (p == NULL)	return ERR_ARG;

	if (system_get_free_heap_size() < p->min_heap) {
#if DEBUGSOO > 1
		os_printf("srv[%u] new listen - low heap size!\n", p->port);
#endif
		return ERR_MEM;
	}
	if (p->conn_count >= p->max_conn) {
		if(p->flag.srv_reopen) {
#if DEBUGSOO > 1
			os_printf("srv[%u] reconnect!\n", p->port);
#endif
			ts_conn = p->conn_links;
			if (p->conn_links != NULL) {
				ts_conn->pcb = find_tcp_pcb(ts_conn);
				os_timer_disarm(&ts_conn->ptimer);
				if (ts_conn->pcb != NULL) {
					tcp_arg(ts_conn->pcb, NULL);
					tcp_recv(ts_conn->pcb, NULL);
					tcp_err(ts_conn->pcb, NULL);
					tcp_poll(ts_conn->pcb, NULL, 0);
					tcp_sent(ts_conn->pcb, NULL);
					tcp_recved(ts_conn->pcb, TCP_WND);
					tcp_close(ts_conn->pcb);
				};
				tcpsrv_list_delete(ts_conn);
//				ts_conn = p->conn_links;
			};
		}
		else {
#if DEBUGSOO > 1
			os_printf("srv[%u] new listen - max connection!\n", p->port);
#endif
			return ERR_CONN;
		}
	}
	ts_conn = (TCP_SERV_CONN *) os_zalloc(sizeof(TCP_SERV_CONN));
	if (ts_conn == NULL) {
#if DEBUGSOO > 1
		os_printf("srv[%u] new listen - out of mem!\n", ts_conn->pcfg->port);
#endif
		return ERR_MEM;
	}
	ts_conn->pcfg = p;
	// перенести флаги по умолчанию на данное соединение
	ts_conn->flag = p->flag;
	tcp_accepted(lpcb); // Decrease the listen backlog counter
	//  tcp_setprio(pcb, TCP_PRIO_MIN); // Set priority ?
	// init/copy data ts_conn
	ts_conn->pcb = pcb;
	ts_conn->remote_port = pcb->remote_port;
	ts_conn->remote_ip.dw = pcb->remote_ip.addr;
	ts_conn->state = SRVCONN_LISTEN;
//	*(uint16 *) &ts_conn->flag = 0; //zalloc
//	ts_conn->recv_check = 0; //zalloc
//  ts_conn->linkd = NULL; //zalloc
	// Insert new ts_conn
	ts_conn->next = ts_conn->pcfg->conn_links;
	ts_conn->pcfg->conn_links = ts_conn;
	ts_conn->pcfg->conn_count++;
	// Tell TCP that this is the structure we wish to be passed for our callbacks.
	tcp_arg(pcb, ts_conn);
	// Set up the various callback functions
	tcp_err(pcb, tcpsrv_error);
	tcp_sent(pcb, tcpsrv_server_sent);
	tcp_recv(pcb, tcpsrv_server_recv);
	tcp_poll(pcb, tcpsrv_server_poll, 8); /* every 1 seconds (SDK 0.9.4) */
	if (p->func_listen != NULL)
		return p->func_listen(ts_conn);
	return ERR_OK;
}
/******************************************************************************
 * FunctionName : tcpsrv_server_port2pcfg
 * Description  : поиск конфига servera по порту
 * Parameters   : номер порта
 * Returns      : указатель на TCP_SERV_CFG или NULL
 *******************************************************************************/
TCP_SERV_CFG * ICACHE_FLASH_ATTR tcpsrv_server_port2pcfg(uint16 portn) {
	TCP_SERV_CFG * p;
	for (p = phcfg; p != NULL; p = p->next)
		if (p->port == portn
		&& (!(p->flag.client)))
			return p;
	return NULL;
}
/******************************************************************************
 * FunctionName : tcpsrv_client_ip_port2conn
 * Description  : поиск конфига clienta по ip + порту
 * Parameters   : номер порта
 * Returns      : указатель на TCP_SERV_CFG или NULL
 *******************************************************************************/
TCP_SERV_CFG * ICACHE_FLASH_ATTR tcpsrv_client_ip_port2conn(uint32 ip, uint16 portn) {
	TCP_SERV_CFG * p;
	for (p = phcfg; p != NULL; p = p->next)
		if (p->flag.client
		&& p->conn_links != NULL
		&& p->conn_links->remote_ip.dw == ip
		&& p->conn_links->remote_port == portn)
				return p;
	return NULL;
}
/******************************************************************************
 tcpsrv_init server or client.
 client -> port = 1 ?
 *******************************************************************************/
TCP_SERV_CFG * ICACHE_FLASH_ATTR tcpsrv_init(uint16 portn) {
	//	if (portn == 0)	portn = 80;
	if (portn == 0)	return NULL;
	TCP_SERV_CFG * p;
	for (p = phcfg; p != NULL; p = p->next) {
		if (p->port == portn) {
#if DEBUGSOO > 0
		os_printf_plus(txt_tcpsrv_already_initialized);
#endif
			return NULL;
		}
	}
	p = (TCP_SERV_CFG *) os_zalloc(sizeof(TCP_SERV_CFG));
	if (p == NULL) {
#if DEBUGSOO > 0
		os_printf_plus(txt_tcpsrv_out_of_mem);
#endif
		return NULL;
	}
	p->port = portn;
	p->conn_count = 0;
	p->min_heap = TCP_SRV_MIN_HEAP_SIZE;
	p->time_wait_rec = TCP_SRV_RECV_WAIT;
	p->time_wait_cls = TCP_SRV_END_WAIT;
	// p->phcfg->conn_links = NULL; // zalloc
	// p->pcb = NULL; // zalloc
	// p->lnk = NULL; // zalloc
	if(portn > ID_CLIENTS_PORT) {
		p->max_conn = TCP_SRV_MAX_CONNECTIONS;
		p->func_listen = tcpsrv_listen_default;
	}
	else {
		p->max_conn = TCP_CLIENT_MAX_CONNECT_RETRY;
		p->flag.client = 1; // данное соединение не сервер, а клиент!
		// insert new tcpsrv_config
		p->next = phcfg;
		phcfg = p;
	}
	p->func_discon_cb = tcpsrv_disconnect_calback_default;
	p->func_sent_cb = tcpsrv_sent_callback_default;
	p->func_recv = tcpsrv_received_data_default;
	return p;
}
/******************************************************************************
 tcpsrv_start
 *******************************************************************************/
err_t ICACHE_FLASH_ATTR tcpsrv_start(TCP_SERV_CFG *p) {
	err_t err = ERR_OK;
	if (p == NULL) {
#if DEBUGSOO > 0
		os_printf_plus(txt_tcpsrv_NULL_pointer);
#endif
		return ERR_ARG;
	}
	if (p->pcb != NULL) {
#if DEBUGSOO > 0
		os_printf_plus(txt_tcpsrv_already_initialized);
#endif
		return ERR_USE;
	}
	p->pcb = tcp_new();

	if (p->pcb != NULL) {
		err = tcp_bind(p->pcb, IP_ADDR_ANY, p->port); // Binds pcb to a local IP address and port number.
		if (err == ERR_OK) { // If another connection is bound to the same port, the function will return ERR_USE, otherwise ERR_OK is returned.
			p->pcb = tcp_listen(p->pcb); // Commands pcb to start listening for incoming connections.
			// When an incoming connection is accepted, the function specified with the tcp_accept() function
			// will be called. pcb must have been bound to a local port with the tcp_bind() function.
			// The tcp_listen() function returns a new connection identifier, and the one passed as an
			// argument to the function will be deallocated. The reason for this behavior is that less
			// memory is needed for a connection that is listening, so tcp_listen() will reclaim the memory
			// needed for the original connection and allocate a new smaller memory block for the listening connection.
			// tcp_listen() may return NULL if no memory was available for the listening connection.
			// If so, the memory associated with pcb will not be deallocated.
			if (p->pcb != NULL) {
				tcp_arg(p->pcb, p->pcb);
				// insert new tcpsrv_config
				p->next = phcfg;
				phcfg = p;
				// initialize callback arg and accept callback
				tcp_accept(p->pcb, tcpsrv_server_accept);
				return err;
			}
		}
		tcp_abandon(p->pcb, 0);
		p->pcb = NULL;
	} else
		err = ERR_MEM;
#if DEBUGSOO > 0
		os_printf("tcpsrv: not new tcp!\n");
#endif
	return err;
}
/******************************************************************************
 tcpsrv_start_client
 TCP_SERV_CFG * p = tcpsrv_init(ID_CLIENTS_PORT);
			// insert new tcpsrv_config
			p->next = phcfg;
			phcfg = p;
 *******************************************************************************/
err_t ICACHE_FLASH_ATTR tcpsrv_client_start(TCP_SERV_CFG * p, uint32 remote_ip, uint16 remote_port) {
	err_t err = ERR_MEM;
	if (p == NULL) return err;
	if (system_get_free_heap_size() >= p->min_heap) {
		TCP_SERV_CONN * ts_conn = (TCP_SERV_CONN *) os_zalloc(sizeof(TCP_SERV_CONN));
		if (ts_conn != NULL) {
			ts_conn->flag = p->flag; // перенести флаги по умолчанию на данное соединение
			ts_conn->pcfg = p;
			ts_conn->state = SRVCONN_CLIENT; // установка соединения (клиент)
			ts_conn->remote_port = remote_port;
			ts_conn->remote_ip.dw = remote_ip;
			tcpsrv_client_connect(ts_conn);
			if(ts_conn->pcb != NULL) {
				// Insert new ts_conn
				ts_conn->next = p->conn_links;
				p->conn_links = ts_conn;
				p->conn_count++;
				err = ERR_OK;
			} else {
#if DEBUGSOO > 0
				tcpsrv_print_remote_info(ts_conn);
				os_printf("tcpsrv: connect error %d\n", err);
#endif
				os_free(ts_conn);
				err = ERR_OK;
			};
		}
		else {
#if DEBUGSOO > 0
			os_printf_plus(txt_tcpsrv_out_of_mem);
#endif
			err = ERR_MEM;
		};
	} else {
#if DEBUGSOO > 0
		os_printf("tcpsrv: low heap size!\n");
#endif
		err = ERR_MEM;
	};
	return err;
}
/******************************************************************************
 tcpsrv_close
 *******************************************************************************/
err_t ICACHE_FLASH_ATTR tcpsrv_close(TCP_SERV_CFG *p) {
	if (p == NULL) {
#if DEBUGSOO > 0
		os_printf_plus(txt_tcpsrv_NULL_pointer);
#endif
		return ERR_ARG;
	};
	TCP_SERV_CFG **pwr = &phcfg;
	TCP_SERV_CFG *pcmp = phcfg;
	while (pcmp != NULL) {
		if (pcmp == p) {
			*pwr = p->next;
			TCP_SERV_CONN * ts_conn = p->conn_links;
			while (p->conn_links != NULL) {
				ts_conn->pcb = find_tcp_pcb(ts_conn);
				os_timer_disarm(&ts_conn->ptimer);
				if (ts_conn->pcb != NULL) {
					tcp_arg(ts_conn->pcb, NULL);
					tcp_recv(ts_conn->pcb, NULL);
					tcp_err(ts_conn->pcb, NULL);
					tcp_poll(ts_conn->pcb, NULL, 0);
					tcp_sent(ts_conn->pcb, NULL);
					tcp_recved(ts_conn->pcb, TCP_WND);
					tcp_abort(ts_conn->pcb);
				};
				tcpsrv_list_delete(ts_conn);
				ts_conn = p->conn_links;
			};
			if(p->pcb != NULL) tcp_close(p->pcb);
			os_free(p);
			p = NULL;
			return ERR_OK; // break;
		}
		pwr = &pcmp->next;
		pcmp = pcmp->next;
	};
#if DEBUGSOO > 2
	os_printf("tcpsrv: srv_cfg no find!\n");
#endif
	return ERR_CONN;
}
/******************************************************************************
 tcpsrv_close_port
 *******************************************************************************/
err_t ICACHE_FLASH_ATTR tcpsrv_close_port(uint16 portn)
{
	if(portn) return tcpsrv_close(tcpsrv_server_port2pcfg(portn));
	else return ERR_ARG;
}
/******************************************************************************
 tcpsrv_close_all
 *******************************************************************************/
err_t ICACHE_FLASH_ATTR tcpsrv_close_all(void)
{
	err_t err = ERR_OK;
	while(phcfg != NULL && err == ERR_OK) err = tcpsrv_close(phcfg);
	return err;
}
