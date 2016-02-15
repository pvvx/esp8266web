/******************************************************************************
 * FileName: ModbusTCP.c
 * ModBus TCP
 * Author: PV`
 * (c) PV` 2015
*******************************************************************************/
#include "user_config.h"
#ifdef USE_MODBUS
#include "bios.h"
#include "sdk/add_func.h"
#include "c_types.h"
#include "osapi.h"
#include "lwip/tcp.h"
#include "tcp_srv_conn.h"
#include "flash_eep.h"
#include "driver/rs485drv.h"
#include "modbusrtu.h"
#include "modbustcp.h"
#include "mdbrs485.h"

extern void Swapws(uint8 *bufw, uint32 lenw);

//#define mMIN(a, b)  ((a<b)?a:b)

TCP_SERV_CFG * mdb_tcp_servcfg DATA_IRAM_ATTR;
TCP_SERV_CONN * mdb_tcp_conn DATA_IRAM_ATTR;

//-------------------------------------------------------------------------------
// TCP disconnect
//-------------------------------------------------------------------------------
void ICACHE_FLASH_ATTR mdb_tcp_disconnect(TCP_SERV_CONN *conn) {
#if DEBUGSOO > 1
	tcpsrv_disconnect_calback_default(conn);
#endif
	mdb_tcp_conn = NULL;
}
//-------------------------------------------------------------------------------
// TCP listen
//-------------------------------------------------------------------------------
err_t ICACHE_FLASH_ATTR mdb_tcp_listen(TCP_SERV_CONN *conn) {
#if DEBUGSOO > 1
	tcpsrv_listen_default(conn);
#endif
	mdb_tcp_conn = conn;
	return ERR_OK;
}
//-------------------------------------------------------------------------------
// TCP recv  (TCP->bufi)
//-------------------------------------------------------------------------------
err_t ICACHE_FLASH_ATTR mdb_tcp_recv(TCP_SERV_CONN *conn) {
#if DEBUGSOO > 1
	tcpsrv_received_data_default(conn);
#endif
	err_t err = ERR_OK;
	if (conn == NULL || conn->pbufi == NULL) return err;
	if (conn->sizei < sizeof(smdbmbap)) return err;
	smdbtcp * p = (smdbtcp *)conn->pbufi;
	Swapws((uint8 *)p, sizeof(smdbmbap)>>1); // перекинем hi<>lo
	// у блока MBAP нет данных или это что-то не то? да -> ждем новый MBAP
	if (p->mbap.pid != MDB_TCP_PID || p->mbap.len == 0 || p->mbap.len > MDB_TCP_ADU_SIZE_MAX) {
		os_free(conn->pbufi);
		return err;
	}
	// не полный или некорректный запрос?
	uint32 len = p->mbap.len + sizeof(smdbmbap);
	if (len < conn->sizei || len < 3) {
		return err;
	};
	if(system_get_free_heap_size() < MDB_TCP_RS485_MIN_HEAP) {
		len = SetMdbErr(&p->adu, MDBERRBUSY);
		if(len) {
			p->mbap.len = len;
			Swapws((uint8 *)p, sizeof(smdbmbap)>>1); // перекинем hi<>lo
			err = tcpsrv_int_sent_data(conn, (uint8 *)p, len + sizeof(smdbmbap));
		}
		return err;
	};
#ifdef USE_RS485DRV
	if(rs485vars.status != RS485_TX_RX_OFF && (
/* отвечаем всегда? (ESP из TCP всегда slave)
#ifdef MDB_RS485_MASTER
			rs485cfg.flg.b.master != 0 ||
#endif */
			p->adu.id != syscfg.mdb_id)) { // запрос не RTU ESP?
			// запрос не к RTU ESP -> к RS-485
	        if (rs485_new_tx_msg(p, true) == NULL) { // переслать в RS-485
			return ERR_MEM;
		};
	};
#endif
	if(
/*#ifdef MDB_RS485_MASTER
			rs485cfg.flg.b.master == 0 &&
#endif */
			(p->adu.id == syscfg.mdb_id || p->adu.id == 0)) { // номер устройства = ?
		smdbtcp * o = (smdbtcp *) os_malloc(sizeof(smdbtcp));
		if(o != NULL) {
			os_memcpy(o, conn->pbufi, len);
			os_free(conn->pbufi);
			conn->pbufi = NULL;
			o->mbap.len = MdbFunc(&o->adu, o->mbap.len); // обработать сообщение PDU
			len = o->mbap.len + sizeof(smdbmbap);
			Swapws((uint8 *)o, sizeof(smdbmbap)>>1); // перекинем hi<>lo
#if DEBUGSOO > 1
	        tcpsrv_print_remote_info(conn);
	        os_printf("send %u bytes\n", len);
#endif
	        err = tcpsrv_int_sent_data(conn, (uint8 *)o, len); // передать ADU в стек TCP/IP
	        os_free(o);
		}
		else {
			err = ERR_MEM;
		};
	};
	return err;
}
//-------------------------------------------------------------------------------
// mdb_tcp_close
//-------------------------------------------------------------------------------
void ICACHE_FLASH_ATTR mdb_tcp_close(void)
{
	if(mdb_tcp_servcfg != NULL) {
		tcpsrv_close(mdb_tcp_servcfg);
		mdb_tcp_servcfg = NULL;
#if DEBUGSOO > 1
		os_printf("MDB: close\n");
#endif
	}
}
//-------------------------------------------------------------------------------
// mdb_tcp_start
//-------------------------------------------------------------------------------
err_t ICACHE_FLASH_ATTR mdb_tcp_start(uint16 portn)
{
	err_t err = ERR_USE;
	ip_addr_t ip;
	ip.addr = 0;
	bool flg_client = false;
	if(portn <= ID_CLIENTS_PORT) {
		mdb_tcp_close();
		syscfg.mdb_port = 0;
		return ERR_OK;
	}
#ifndef USE_TCP2UART
	if(tcp_client_url != NULL) {
		ipaddr_aton(tcp_client_url, &ip);
		if(ip.addr != 0x0100007f && ip.addr != 0) {
#if DEBUGSOO > 1
			os_printf("MDB: client ip:" IPSTR ", port: %u\n", IP2STR(&ip), portn);
#endif
			flg_client = true;
		}
	}
#endif
	// проверка на повторное открытие уже открытого
	if(mdb_tcp_servcfg != NULL) {
		// уже запущен
		if(flg_client) {
			// client
			if(mdb_tcp_servcfg->flag.client // соединение не сервер, а клиент
			&& mdb_tcp_servcfg->conn_links != NULL // соединение активно
			&& mdb_tcp_servcfg->conn_links->remote_ip.dw == ip.addr
			&& mdb_tcp_servcfg->conn_links->remote_port == portn) { // порт совпадает
				return err; // ERR_USE параметры не изменились
			}
		}
		else {
			// server
			if((!(mdb_tcp_servcfg->flag.client)) // соединение сервер
				&& mdb_tcp_servcfg->port == portn) { // порт совпадает
				return err; // ERR_USE параметры не изменились
			}
		}
	}
	// соединения нет или смена параметров соединения
	mdb_tcp_close(); // закрыть прошлое
	if(portn <= ID_CLIENTS_PORT) return ERR_OK;
	TCP_SERV_CFG *p;
	if(flg_client) p = tcpsrv_init_client2(); // tcpsrv_init(2)
	else p = tcpsrv_init(portn);
	if (p != NULL) {
		// изменим конфиг на наше усмотрение:
//		p->flag.rx_buf = 1; // прием в буфер с его автосозданием. ????
		p->flag.nagle_disabled = 1; // отмена nagle
		if(flg_client) {
			p->flag.client_reconnect = 1; // вечный реконнект
			p->max_conn = 0; // =0 - вечная попытка соединения
		}
		else {
			if(syscfg.cfg.b.mdb_reopen) p->flag.srv_reopen = 1;
			p->max_conn = 1; // одно соединение (порт UART не многопользовательский!)
		}
		p->time_wait_rec = syscfg.mdb_twrec; // =0 -> вечное ожидание
		p->time_wait_cls = syscfg.mdb_twcls; // =0 -> вечное ожидание
#if DEBUGSOO > 3
		os_printf("Max connection %d, time waits %d & %d, min heap size %d\n",
				p->max_conn, p->time_wait_rec, p->time_wait_cls, p->min_heap);
#endif
		p->func_discon_cb = mdb_tcp_disconnect;
		p->func_listen = mdb_tcp_listen;
		p->func_sent_cb = NULL;
//		p->func_sent_cb = mdb_tcp_sent_cb;
		p->func_recv = mdb_tcp_recv;
		if(flg_client) err = tcpsrv_client_start(p, ip.addr, portn);
		else err = tcpsrv_start(p);
		if (err != ERR_OK) {
			tcpsrv_close(p);
			p = NULL;
		}
		else  {
			syscfg.mdb_port = portn;
			if(flg_client)	{
#if DEBUGSOO > 5
				os_printf("MDB: client init\n");
#endif
			}
			else {
#if DEBUGSOO > 1
				os_printf("MDB: init port %u\n", portn);
#endif
			}
		}
	}
	else err = ERR_USE;
	mdb_tcp_servcfg = p;
	return err;
}

#endif // USE_MODBUS
