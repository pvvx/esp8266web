/******************************************************************************
 * FileName: web_fs_ini.c
 * Description: The web server start configuration.
*******************************************************************************/
#include "user_config.h"
#ifdef USE_WEB
#include "bios.h"
#include "sdk/add_func.h"
#include "hw/esp8266.h"
#include "tcp_srv_conn.h"
#include "sdk/rom2ram.h"
#include "sdk/app_main.h"

#include "web_srv_int.h"
#include "web_utils.h"
#include "web_iohw.h"
#include "webfs.h"

#define CRLF "\r\n"
#define FINI_BUF_SIZE 512

struct buf_fini
{
	TCP_SERV_CONN ts_conn;
	WEB_SRV_CONN web_conn;
	uint8 buf[FINI_BUF_SIZE+1];
};

/******************************************************************************
*******************************************************************************/
LOCAL uint16 ICACHE_FLASH_ATTR find_crlf(uint8 * chrbuf, uint16 len) {
  int i;
  for(i = 0; i < len; i++) {
	  if(chrbuf[i] == '\n' || chrbuf[i] == '\r' || chrbuf[i] == '\0')  return i;
  }
  return len;
}
/******************************************************************************
*******************************************************************************/
void ICACHE_FLASH_ATTR web_fini(const uint8 * fname)
{
	struct buf_fini *p = (struct buf_fini *) os_zalloc(sizeof(struct buf_fini));
	if(p == NULL) {
#if DEBUGSOO > 1
		os_printf("Error mem!\n");
#endif
		return;
	}
	TCP_SERV_CONN * ts_conn = &p->ts_conn;
	WEB_SRV_CONN * web_conn = &p->web_conn;
	web_conn->bffiles[0] = WEBFS_INVALID_HANDLE;
	web_conn->bffiles[1] = WEBFS_INVALID_HANDLE;
	web_conn->bffiles[2] = WEBFS_INVALID_HANDLE;
	web_conn->bffiles[3] = WEBFS_INVALID_HANDLE;
	ts_conn->linkd = (uint8 *)web_conn;
	ts_conn->sizeo = FINI_BUF_SIZE;
	ts_conn->pbufo = p->buf;
	rom_strcpy(ts_conn->pbufo, (void *)fname, MAX_FILE_NAME_SIZE);
#if DEBUGSOO > 1
	os_printf("Run ini file: %s\n", ts_conn->pbufo);
#endif
	if(!web_inc_fopen(ts_conn, ts_conn->pbufo)) {
#if DEBUGSOO > 1
		os_printf("file not found!\n");
#endif
		return;
	}
	if(fatCache.flags & WEBFS_FLAG_ISZIPPED) {
#if DEBUGSOO > 1
		os_printf("\nError: file is ZIPped!\n");
#endif
		web_inc_fclose(web_conn);
		return;
	}
	user_uart_wait_tx_fifo_empty(1,1000);
	while(1) {
		web_conn->msgbufsize = ts_conn->sizeo;
		web_conn->msgbuflen = 0;
		uint8 *pstr = web_conn->msgbuf = ts_conn->pbufo;
		if(CheckSCB(SCB_RETRYCB)) { // повторный callback? да
#if DEBUGSOO > 2
			os_printf("rcb ");
#endif
			if(web_conn->func_web_cb != NULL) web_conn->func_web_cb(ts_conn);
			if(CheckSCB(SCB_RETRYCB)) break; // повторить ещё раз? да.
		}
		uint16 len = WEBFSGetArray(web_conn->webfile, pstr, FINI_BUF_SIZE);
#if DEBUGSOO > 3
		os_printf("ReadF[%u]=%u\n", web_conn->webfile, len);
#endif
		if(len) { // есть байты в файле
			pstr[len] = '\0';
			int sslen = 0;
			while(pstr[sslen] == '\n' || pstr[sslen] == '\r') sslen++;
			if(sslen == 0) {
				int nslen = find_crlf(pstr, len);
				if(nslen != 0) {
					pstr[nslen] = '\0'; // закрыть string calback-а
					while((nslen < len) && (pstr[nslen] == '\n' || pstr[nslen] == '\r')) nslen++;
#if DEBUGSOO > 3
					os_printf("String:%s\n", pstr);
#endif

					if(!os_memcmp((void*)pstr, "inc:", 4)) { // "inc:file_name"
						if(!web_inc_fopen(ts_conn, &pstr[4])) {
#if DEBUGSOO > 1
							os_printf("file not found!");
#endif
						};
					}
					else web_int_callback(ts_conn, pstr);
				};
				sslen = nslen;
			};
			// откат файла
			WEBFSStubs[web_conn->webfile].addr -= len;
			WEBFSStubs[web_conn->webfile].bytesRem += len;
			// передвинуть указатель в файле на считанные байты с учетом маркера, без добавки длины для передачи
			WEBFSStubs[web_conn->webfile].addr += sslen;
			WEBFSStubs[web_conn->webfile].bytesRem -= sslen;
		}
		else if(web_inc_fclose(web_conn)) {
			if(web_conn->web_disc_cb != NULL) web_conn->web_disc_cb(web_conn->web_disc_par);
			return;
		}
	}
}

#endif // USE_WEB
