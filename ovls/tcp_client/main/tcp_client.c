/******************************************************************************
 * PV` FileName: tcp_client.c
*******************************************************************************/
#include "user_config.h"
#include "bios.h"
#include "sdk/add_func.h"
#include "c_types.h"
#include "osapi.h"
#include "user_interface.h"
#include "lwip/tcp.h"
#include "lwip/dns.h"
#include "tcp_srv_conn.h"
#include "web_utils.h"
#include "wifi_events.h"
#include "ovl_sys.h"

#define DEFAULT_TC_HOST_PORT 80

#define tc_timeout		mdb_buf.ubuf[50] // повтор опроса через tc_timeout минут
#define tc_init_usr		mdb_buf.ubuf[51] // Флаг: =0 - драйвер закрыт, =1 - драйвер установлен
#define tc_error		mdb_buf.ubuf[52] // Флаг: =1 - данные актуальны, другое значение -> данные ещё не считаны
#define tc_temp1		mdb_buf.ubuf[53] // считанная температура минимум (со знаком)
#define tc_temp2		mdb_buf.ubuf[54] // считанная температура максимум (со знаком)
//#define	tc_url    		((uint8 *)(mdb_buf.ubuf+20))

uint8 tc_url[] = "weather-in.ru";
uint8 tc_get[] = "GET /sankt-peterbyrg/107709 HTTP/1.0/\r\nHost: weather-in.ru\r\n\r\n";
uint8 key_http_ok[] = "HTTP/1.1 200 OK\r\n";
//uint8 key_chunked[] = "Transfer-Encoding: chunked";

uint8 key_td_info[] = "<td class=\"info\">";
uint32 next_start_time DATA_IRAM_ATTR;

//uint32 content_chunked DATA_IRAM_ATTR;
os_timer_t error_timer DATA_IRAM_ATTR;

ip_addr_t tc_remote_ip;
int tc_init_flg DATA_IRAM_ATTR; // внутренние флаги инициализации
#define TC_START_INIT 	(1<<0)
#define TC_INIT_WIFI_CB (1<<1)
#define TC_FLG_RUN_ON 	(1<<2)

uint32 tc_head_size DATA_IRAM_ATTR; // используется для разбора HTTP ответа

#define mMIN(a, b)  ((a<b)?a:b)

TCP_SERV_CFG * tc_servcfg DATA_IRAM_ATTR;

void wifi_event_cb(System_Event_t *evt);
void tc_close(void);
void run_error_timer(uint32 tsec);
//-------------------------------------------------------------------------------
// TCP sent_cb
//-------------------------------------------------------------------------------
/*
err_t ICACHE_FLASH_ATTR tc_sent_cb(TCP_SERV_CONN *ts_conn) {
#if DEBUGSOO > 1
	tcpsrv_sent_callback_default(ts_conn);
#endif
	tc_sconn = ts_conn;
	return ERR_OK;
}
*/
//-------------------------------------------------------------------------------
// TCP recv
//-------------------------------------------------------------------------------
err_t ICACHE_FLASH_ATTR tc_recv(TCP_SERV_CONN *ts_conn) {
#if DEBUGSOO > 1
	tcpsrv_received_data_default(ts_conn);
#endif
	tcpsrv_unrecved_win(ts_conn);
    uint8 *pstr = ts_conn->pbufi;
    sint32 len = ts_conn->sizei;
    if(tc_head_size == 0) {
        if(len < sizeof(key_http_ok) +2 -1) {
        	return ERR_OK;
        }
        if(os_memcmp(key_http_ok, pstr, sizeof(key_http_ok)-1)==0) {
        	tc_head_size = sizeof(key_http_ok)-1;
        	len -= sizeof(key_http_ok)-1;
        	pstr += sizeof(key_http_ok)-1;
        }
        else {
        	tc_close();
        	return ERR_OK;
        }
        uint8 * nstr = web_strnstr(pstr, "\r\n\r\n", len);
        if(nstr == NULL) return ERR_OK;
        *nstr = '\0';
        tc_head_size = nstr - ts_conn->pbufi + 4;
        ts_conn->cntri = tc_head_size;
#if DEBUGSOO > 4
    	os_printf("head[%d]: '%s' ", tc_head_size, ts_conn->pbufi);
#endif
/*        if(web_strnstr(pstr, key_chunked, len) != NULL) {
        	content_chunked = 1;
        }
        else content_chunked = 0; */
        pstr = &ts_conn->pbufi[ts_conn->cntri];
        len = ts_conn->sizei - ts_conn->cntri;
#if DEBUGSOO > 3
    	os_printf("str: '%s' ", ts_conn->pbufi);
#endif
    }
    if(tc_head_size != 0 && len > sizeof(key_td_info) + 1) {
#if DEBUGSOO > 5
    	os_printf("content[%d] ", len);
#endif
    	uint8 * nstr = web_strnstr(pstr, key_td_info, len);
    	if(nstr == NULL) {
    		ts_conn->cntri = ts_conn->sizei - sizeof(key_td_info) + 1;
        	return ERR_OK;
    	}
    	else {
    		pstr = nstr + sizeof(key_td_info) - 1;
    		len = &ts_conn->pbufi[ts_conn->sizei] - pstr;
    		if(len > 20) nstr = web_strnstr(pstr, "</td>", len);
    		else return ERR_OK;
    		if(nstr != NULL && nstr[-1] == 'C') {
        		*nstr++ ='\0';
#if DEBUGSOO > 1
        		os_printf("\"%s\" ", pstr);
#endif
        		pstr = nstr + 5;
        		len = &ts_conn->pbufi[ts_conn->sizei] - pstr;
           		if(len > 20) nstr = web_strnstr(pstr, "</td>", len);
           		else return ERR_OK;
           		if(nstr != NULL) {
               		*nstr ='\0';
               		while(pstr < nstr && *pstr++ != '>');
      				tc_temp1 = rom_atoi(pstr);
               		while(pstr < nstr && *pstr++ != '>');
      				tc_temp2 = rom_atoi(pstr);
#if DEBUGSOO > 1
               		os_printf("%d..%d\n", (sint16)tc_temp1, (sint16)tc_temp2);
#endif
               		nstr++;
               		ts_conn->flag.rx_null = 1;
               		// данные приянты
               		tc_init_flg &= ~TC_FLG_RUN_ON;
               		if(tc_timeout) {
               			run_error_timer(60); // повтор через 60 секунд * next_start_time
               		}
               		else {
                   		ets_timer_disarm(&error_timer);
               			next_start_time = 0;
               		}
           			tc_error = 1;
           		}
    		}
   			ts_conn->cntri = nstr - ts_conn->pbufi;
    	}
    }
	return ERR_OK;
}
//-------------------------------------------------------------------------------
// TCP listen
//-------------------------------------------------------------------------------
err_t ICACHE_FLASH_ATTR tc_listen(TCP_SERV_CONN *ts_conn) {
#if DEBUGSOO > 1
	tcpsrv_print_remote_info(ts_conn);
	os_printf("send %d bytes\n", sizeof(tc_get)-1);
#endif
	return tcpsrv_int_sent_data(ts_conn, tc_get, sizeof(tc_get)-1);
}
//-------------------------------------------------------------------------------
// TCP disconnect
//-------------------------------------------------------------------------------
void ICACHE_FLASH_ATTR tc_disconnect(TCP_SERV_CONN *ts_conn) {
#if DEBUGSOO > 1
	tcpsrv_disconnect_calback_default(ts_conn);
#endif
}
//-------------------------------------------------------------------------------
// tc_start
//-------------------------------------------------------------------------------
err_t ICACHE_FLASH_ATTR tc_init(void)
{
	err_t err = ERR_USE;
	tc_close();

	TCP_SERV_CFG * p = tcpsrv_init_client3();  // tcpsrv_init(3)
	if (p != NULL) {
		// изменим конфиг на наше усмотрение:
		p->max_conn = 3; // =0 - вечная попытка соединения
		p->flag.rx_buf = 1; // прием в буфер с его автосозданием.
		p->flag.nagle_disabled = 1; // отмена nagle
//		p->time_wait_rec = tc_twrec; // по умолчанию 5 секунд
//		p->time_wait_cls = tc_twcls; // по умолчанию 5 секунд
#if DEBUGSOO > 5
		os_printf("TC: Max retry connection %d, time waits %d & %d, min heap size %d\n",
					p->max_conn, p->time_wait_rec, p->time_wait_cls, p->min_heap);
#endif
		p->func_discon_cb = tc_disconnect;
		p->func_listen = tc_listen;
		p->func_sent_cb = NULL;
		p->func_recv = tc_recv;
		err = ERR_OK;
	}
	tc_servcfg = p;
	return err;
}
//-------------------------------------------------------------------------------
// tc_close
//-------------------------------------------------------------------------------
void tc_close(void)
{
	if(tc_servcfg != NULL) {
		tcpsrv_close(tc_servcfg);
		tc_servcfg = NULL;
	}
}
//-------------------------------------------------------------------------------
// dns_found_callback
//-------------------------------------------------------------------------------
void tc_dns_found_callback(uint8 *name, ip_addr_t *ipaddr, void *callback_arg)
{
#if DEBUGSOO > 5
	os_printf("clb:%s, " IPSTR " ", name, IP2STR(ipaddr));
#endif
	if(tc_servcfg != NULL) {
		if(ipaddr != NULL && ipaddr->addr != 0) {
			tc_remote_ip = *ipaddr;
			tc_head_size = 0;
			err_t err = tcpsrv_client_start(tc_servcfg, tc_remote_ip.addr, DEFAULT_TC_HOST_PORT);
			if (err != ERR_OK) {
#if DEBUGSOO > 5
				os_printf("goerr=%d ", tc_error);
#endif
				tc_close();
			}
		}
	}
	run_error_timer(15); // таймаут на выполнение в 15 секунд
}
//-------------------------------------------------------------------------------
// TCP client start
//-------------------------------------------------------------------------------
err_t tc_go(void)
{
	err_t err = ERR_USE;
	if(next_start_time) {
		run_error_timer(60); // повторить через паузу в 60 секунд
	}
	else {
		err = tc_init(); // инициализация TCP
		if(err == ERR_OK) {
			tc_init_flg |= TC_FLG_RUN_ON; // процесс запущен
			err = dns_gethostbyname(tc_url, &tc_remote_ip, (dns_found_callback)tc_dns_found_callback, NULL);
#if DEBUGSOO > 5
			os_printf("dns_gethostbyname(%s)=%d ", tc_url, err);
#endif
			if(err == ERR_OK) {	// Адрес разрешен из кэша или локальной таблицы
				tc_head_size = 0;
				err = tcpsrv_client_start(tc_servcfg, tc_remote_ip.addr, DEFAULT_TC_HOST_PORT);
				if(err != ERR_OK) {
					tc_init_flg &= ~TC_FLG_RUN_ON; // процесс не запущен
					run_error_timer(7); // повторить через паузу в 7 секунд
				}
				else run_error_timer(15); // таймаут на выполнение в 15 секунд
			}
			else if(err == ERR_INPROGRESS) { // Запущен процесс разрешения имени с внешнего DNS
				err = ERR_OK;
			}
			if (err != ERR_OK) {
				tc_init_flg &= ~TC_FLG_RUN_ON; // процесс не запущен
//				tc_close();
				run_error_timer(7); // повторить через паузу в 7 секунд
			}
		}
		else {
			run_error_timer(7); // повторить через паузу в 7 секунд
		}
	}
	return err;
}
//-------------------------------------------------------------------------------
// close_dns_found
//-------------------------------------------------------------------------------
void close_dns_found(void){
	ets_timer_disarm(&error_timer);
	if(tc_init_flg & TC_FLG_RUN_ON) { // ожидание dns_found_callback() ?
		// убить вызов  tc_dns_found_callback()
		int i;
		for (i = 0; i < DNS_TABLE_SIZE; ++i) {
			if(dns_table[i].found == (dns_found_callback)tc_dns_found_callback) {
				/* flush this entry */
				dns_table[i].found = NULL;
				dns_table[i].state = DNS_STATE_UNUSED;
			}
		}
		tc_init_flg &= ~TC_FLG_RUN_ON;
	}
	tc_close();
}
//-------------------------------------------------------------------------------
// wifi_event_cb
//-------------------------------------------------------------------------------
void wifi_event_cb(System_Event_t *evt)
{
	wifi_handle_event_cb(evt);
	if(evt->event == EVENT_STAMODE_GOT_IP) {
		ovl_init(1);
	}
	else if(evt->event == EVENT_STAMODE_DISCONNECTED) {
		close_dns_found();
	}
}
//-------------------------------------------------------------------------------
// run_error_timer
//-------------------------------------------------------------------------------
void run_error_timer(uint32 tsec)
{
	if(tsec < 60) next_start_time = 0;
	else {
		if(next_start_time == 0) next_start_time = tc_timeout - 1;
		else next_start_time--;
	}
	ets_timer_disarm(&error_timer);
	ets_timer_setfn(&error_timer, (os_timer_func_t *)tc_go, NULL);
	ets_timer_arm_new(&error_timer, tsec*1000, 0, 1); // таймер на 5 или 10 секунд
}
//=============================================================================
//=============================================================================
int ovl_init(int flg)
{
	int ret = 0;
	if(flg == 1) {
#if DEBUGSOO > 1
		os_printf("TC: Start\n");
#endif
		if((tc_init_flg & TC_START_INIT) == 0) {
			tc_error = 0; // данные не актуальны
			tc_init_flg |= TC_START_INIT;
		}
 		if((tc_init_flg & TC_FLG_RUN_ON) == 0) { // процесс не запущен?
			if((tc_init_flg & TC_INIT_WIFI_CB) == 0) {
				wifi_set_event_handler_cb(wifi_event_cb); // перехватить wifi_event
				tc_init_flg |= TC_INIT_WIFI_CB;
			}
			if(flg_open_all_service && wifi_station_get_connect_status() == STATION_GOT_IP) {
				// есть соединение с AP?
				if(tc_go() != ERR_OK) ret = -1;
			}
			tc_init_usr = 1; // драйвер инициализирован
		}
		else ret = 1; // процесс уже идет
	}
	else { // закрытие драйвера
		if(tc_init_flg & TC_INIT_WIFI_CB) {
			wifi_set_event_handler_cb(wifi_handle_event_cb); // вернуть адрес процедуры wifi_event
		}
		close_dns_found();
		tc_init_flg = TC_START_INIT; // драйвер закрыт
#if DEBUGSOO > 1
		os_printf("TC: Close\n");
#endif
		tc_init_usr = 0; // драйвер закрыт
	}
	return ret;
}

