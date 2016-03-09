/*
 * File: web_srv_int.h
 * Description: The web server configration.
 * Small WEB server ESP8266EX
 *
 * Author: PV` 12/2014
 */

#ifndef _INCLUDE_WEB_SRV_INT_H_
#define _INCLUDE_WEB_SRV_INT_H_

#include "web_srv.h"

#define WEB_NAME_VERSION "PVs/0.1"

// #define WEBSOCKET_ENA 1

// lifetime (sec) of static responses as string 60*60*24*14=1209600"
#define FILE_CACHE_MAX_AGE_SEC  3600 // время для кеша файлов, ставить 0 пока тест!

#define MAX_HTTP_HEAD_BUF 3070 // максимальный размер HTTP запроса (GET)

#define RESCHKS_SEND_SIZE 16
#define RESCHKE_SEND_SIZE 8
#define RESCHK_SEND_SIZE (RESCHKS_SEND_SIZE + RESCHKE_SEND_SIZE)

#define MIN_SEND_SIZE (256 + RESCHK_SEND_SIZE) // минимальный размер буфера для передачи файла
#define MAX_SEND_SIZE ((TCP_MSS*4) + RESCHK_SEND_SIZE) // максимальный размер буфера для передачи 4*MSS = 5840 (MSS=1460)

#define HTTP_SEND_SIZE 384 // минимальный размер буфера для передачи заголовка HTTP
#define SCB_SEND_SIZE  128 // минимальный резерв в буфере для callback

#define  webfile bffiles[0]        // File pointer for main file

//-----------------------------------------------------------------------------

void web_int_vars(TCP_SERV_CONN *ts_conn, uint8 *pcmd, uint8 *pvar) ICACHE_FLASH_ATTR;
void web_int_cookie(HTTP_CONN *CurHTTP, TCP_SERV_CONN *ts_conn) ICACHE_FLASH_ATTR;
void web_int_callback(TCP_SERV_CONN *ts_conn, uint8 *cstr) ICACHE_FLASH_ATTR;

void web_hexdump(TCP_SERV_CONN *ts_conn) ICACHE_FLASH_ATTR;
bool web_inc_fopen(TCP_SERV_CONN *ts_conn, uint8 *cFile) ICACHE_FLASH_ATTR;
bool web_inc_fclose(WEB_SRV_CONN *web_conn) ICACHE_FLASH_ATTR;

bool web_trim_bufi(TCP_SERV_CONN *ts_conn, uint8 *pdata, uint32 data_len) ICACHE_FLASH_ATTR;
bool web_feee_bufi(TCP_SERV_CONN *ts_conn) ICACHE_FLASH_ATTR;
//uint8 * head_find_ctr(HTTP_CONN *CurHTTP, const uint8 * c, int clen, int dlen) ICACHE_FLASH_ATTR;

#endif /* _INCLUDE_WEB_SRV_INT_H_ */
