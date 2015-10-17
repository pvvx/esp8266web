/******************************************************************************
 * FileName: MdbFunc.c 
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
#include "modbusrtu.h"

extern void Swapws(uint16 *bufw, uint32 lenw);

//#define mMIN(a, b)  ((a<b)?a:b)

TCP_SERV_CFG * mdb_tcp_servcfg DATA_IRAM_ATTR;

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
	smdbmbap * p = (smdbmbap *)conn->pbufi;
	Swapws((uint16 *)p, sizeof(smdbmbap)>>1); // перекинем hi<>lo
	// у блока MBAP нет данных или это что-то не то? да -> ждем новый MBAP
	if (p->pid != 0 || p->len == 0 || p->len > 253) {
		return err;
	}
	// не полный или некорректный запрос?
	uint32 len = p->len + sizeof(smdbmbap);
	if (len < conn->sizei) {
		return err;
	}
	smdbtcp * o = (smdbtcp *) os_malloc(sizeof(smdbtcp));
	if(o != NULL) {
		os_memcpy(o, conn->pbufi, len);
		os_free(conn->pbufi);
		conn->pbufi = NULL;
		if (o->adu.id <= 1) { // номер устройства = 1
			o->adu.id = 1; // всегда к нам, мы не сетевой бридж...
			o->mbap.len = MdbFunc(&o->adu, o->mbap.len); // обработать сообщение PDU
			len = o->mbap.len + sizeof(smdbmbap);
			Swapws((uint16 *)o, sizeof(smdbmbap)>>1); // перекинем hi<>lo
#if DEBUGSOO > 1
	        tcpsrv_print_remote_info(conn);
	        os_printf("send %u bytes\n", len);
#endif
	        err = tcpsrv_int_sent_data(conn, (uint8 *)o, len); // передать ADU в стек TCP/IP
	        os_free(o);
		}
	}
	else err = ERR_MEM;
	return err;
}
//-------------------------------------------------------------------------------
// mdb_tcp_close
//-------------------------------------------------------------------------------
void ICACHE_FLASH_ATTR mdb_tcp_close(void)
{
	if(mdb_tcp_servcfg != NULL) {
		tcpsrv_close(mdb_tcp_servcfg);
#if DEBUGSOO > 1
			os_printf("MDB: close\n");
#endif
	}
}
//-------------------------------------------------------------------------------
// mdb_tcp_init
//-------------------------------------------------------------------------------
err_t ICACHE_FLASH_ATTR mdb_tcp_init(uint16 portn) {
	err_t err = ERR_USE;
	if(mdb_tcp_servcfg != NULL
	&& (!(mdb_tcp_servcfg->flag.client))
	&& mdb_tcp_servcfg->port == portn) {
		return  ERR_USE;
	}
	mdb_tcp_close();
	if(portn <= ID_CLIENTS_PORT) return ERR_OK;
	TCP_SERV_CFG *p = tcpsrv_init(portn);
	if (p != NULL) {
		// изменим конфиг на наше усмотрение:
//		p->flag.rx_buf = 1; // прием в буфер с его автосозданием. ????
		p->flag.nagle_disabled = 1; // отмена nagle
		if(syscfg.cfg.b.mdb_reopen) p->flag.srv_reopen = 1;
		p->max_conn = 1; // одно соединение (порт UART не многопользовательский!)
		p->time_wait_rec = syscfg.mdb_twrec; // =0 -> вечное ожидание
		p->time_wait_cls = syscfg.mdb_twcls; // =0 -> вечное ожидание
#if DEBUGSOO > 3
		os_printf("Max connection %d, time waits %d & %d, min heap size %d\n",
				p->max_conn, p->time_wait_rec, p->time_wait_cls, p->min_heap);
#endif
//		p->func_discon_cb = mdb_tcp_disconnect;
//		p->func_listen = mdb_tcp_listen;
		p->func_sent_cb = NULL;
//		p->func_sent_cb = mdb_tcp_sent_cb;
		p->func_recv = mdb_tcp_recv;
		err = tcpsrv_start(p);
		if (err != ERR_OK) {
			tcpsrv_close(p);
			p = NULL;
		}
		else  {
			syscfg.mdb_remote_port = portn;
#if DEBUGSOO > 1
			os_printf("MDB: init port %u\n", portn);
#endif
		}
	}
	else err = ERR_USE;
	mdb_tcp_servcfg = p;
	return err;
}

#endif // USE_MODBUS
