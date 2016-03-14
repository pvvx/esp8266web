/******************************************************************************
 * FileName: web_websocket.c
 * Description: websocket for web ESP8266
 * Author: PV`
 * (c) PV` 2016
*******************************************************************************/
#include "user_config.h"
#ifdef WEBSOCKET_ENA
#include "bios.h"
#include "osapi.h"
#include "sdk/rom2ram.h"
#include "lwip/tcp.h"
#include "tcp_srv_conn.h"
#include "web_srv_int.h"
#include "web_utils.h"
#include "web_websocket.h"

#define mMIN(a, b)  ((a<b)?a:b)

#define MAX_RX_BUF_SIZE 8192

const char txt_wsping[] ICACHE_RODATA_ATTR = "ws:ping";
const char txt_wspong[] ICACHE_RODATA_ATTR = "ws:pong";

//=============================================================================
// websock_tx_frame() - передача фрейма
//=============================================================================
err_t ICACHE_FLASH_ATTR
websock_tx_frame(TCP_SERV_CONN *ts_conn, uint32 opcode, uint8 *raw_data, uint32 raw_len)
{
	err_t err = WebsocketTxFrame(ts_conn, opcode, raw_data, raw_len);
	if(err != ERR_OK) {
#if DEBUGSOO > 3
		os_printf("ws%utx[%u] error %d!\n", opcode, raw_len, err);
#endif
		((WEB_SRV_CONN *)ts_conn->linkd)->webflag |= SCB_DISCONNECT;
	}
	else {
		if((opcode & WS_OPCODE_BITS) == WS_OPCODE_CLOSE) {
			((WEB_SRV_CONN *)ts_conn->linkd)->ws.flg |= WS_FLG_CLOSE;
		}
	}
	return err;
}
//=============================================================================
// websock_tx_close_err() - вывод сообщения закрытия или ошибки
//=============================================================================
err_t ICACHE_FLASH_ATTR
websock_tx_close_err(TCP_SERV_CONN *ts_conn, uint32 err)
{
	uint8 uc[2];
	uc[1] = err;
	uc[0] = err>>8;
	return  websock_tx_frame(ts_conn, WS_OPCODE_CLOSE | WS_FRAGMENT_FIN, uc, 2);
}
//=============================================================================
// websock_rx_data() прием данных
//=============================================================================
#define MAX_WS_DATA_BLK_SIZE  (tcp_sndbuf(ts_conn->pcb)-8) // (TCP_MSS - 8)
//=============================================================================
bool ICACHE_FLASH_ATTR
websock_rx_data(TCP_SERV_CONN *ts_conn)
{
//	HTTP_CONN *CurHTTP;
	WEB_SRV_CONN *web_conn = (WEB_SRV_CONN *)ts_conn->linkd;
	if(web_conn == NULL) return false;
	WS_FRSTAT *ws = &web_conn->ws;
	uint16 len;
	uint8 *pstr;

#if DEBUGSOO > 3
	os_printf("ws_rx[%u]%u ", ts_conn->sizei, ts_conn->cntri);
#endif
	if(ts_conn->sizei == 0) return true; // докачивать
	tcpsrv_unrecved_win(ts_conn);
	if((ws->flg & WS_FLG_CLOSE) != 0) {
		// убить буфер ts_conn->pbufi,  конец давно :)
		web_feee_bufi(ts_conn);
		SetSCB(SCB_DISCONNECT);
		return false;
	}
	if(ts_conn->sizei > MAX_RX_BUF_SIZE) {
#if DEBUGSOO > 0
		os_printf("ws:rxbuf_full! ");
#endif
		// убить буфер ts_conn->pbufi и ответить ошибкой WS_CLOSE_UNEXPECTED_ERROR
		web_feee_bufi(ts_conn);
		websock_tx_close_err(ts_conn, WS_CLOSE_MESSAGE_TOO_BIG); // WS_CLOSE_UNEXPECTED_ERROR);
		SetSCB(SCB_DISCONNECT);
		return false;
	}
	pstr = ts_conn->pbufi;// + ts_conn->cntri;
	len = ts_conn->sizei;// - ts_conn->cntri;
	while(ts_conn->cntri < ts_conn->sizei || (ws->flg & WS_FLG_FIN) != 0) {
		pstr = ts_conn->pbufi;// + ts_conn->cntri;
		len = ts_conn->sizei;// - ts_conn->cntri;
		if((ws->flg & WS_FLG_FIN) != 0 // обработка
			|| ws->frame_len > ws->cur_len) {
			ws->flg &= ~WS_FLG_FIN;
			len = mMIN(ws->frame_len - ws->cur_len, mMIN(MAX_WS_DATA_BLK_SIZE, len));
			// размаскировать
			if((ws->flg & WS_FLG_MASK) != 0) WebsocketMask(ws, pstr, len);
#if DEBUGSOO > 3
			os_printf("wsfr[%u]blk[%u]at:%u ", ws->frame_len, len, ws->cur_len);
#endif
			switch(ws->status) {
				case sw_frs_binary:
#if DEBUGSOO > 1
					os_printf("ws:bin ");
#endif
					if(ws->frame_len != 0) {
						// пока просто эхо
						uint32 opcode = WS_OPCODE_BINARY;
						if(ws->cur_len != 0) opcode = WS_OPCODE_CONTINUE;
						if(ws->frame_len == ws->cur_len + len) opcode |= WS_FRAGMENT_FIN;
						if(websock_tx_frame(ts_conn, opcode, pstr, len) != ERR_OK) {
							return false; // не докачивать, ошибка или закрытие
						}
					}
					ws->cur_len += len;
					ts_conn->cntri += len;
					break;
				case sw_frs_text:
#if DEBUGSOO > 1
					os_printf("ws:txt ");
#if DEBUGSOO > 2
					if(ws->frame_len != 0) {
						uint8 tt = pstr[len];
						pstr[len] = 0;
						os_printf("'%s' ", pstr);
						pstr[len] = tt;
					}
#endif
#endif
					if(ws->frame_len == ws->cur_len + len && ws->frame_len != 0) { // полное соо
						web_conn->msgbufsize = tcp_sndbuf(ts_conn->pcb); // сколько можем выввести сейчас?
						if (web_conn->msgbufsize < MIN_SEND_SIZE) {
#if DEBUGSOO > 0
							os_printf("ws:sndbuf=%u! ", web_conn->msgbufsize);
#endif
							websock_tx_close_err(ts_conn, WS_CLOSE_UNEXPECTED_ERROR);
							SetSCB(SCB_FCLOSE|SCB_DISCONNECT);
							return false;
						}
						if(ws->frame_len == (sizeof(txt_wsping)-1) && rom_xstrcmp(pstr, txt_wsping) != 0){
							copy_s4d1(pstr, (void *)txt_wspong, sizeof(txt_wspong) - 1);
							if(websock_tx_frame(ts_conn, WS_OPCODE_TEXT | WS_FRAGMENT_FIN, pstr, sizeof(txt_wspong) - 1) != ERR_OK) {
								return false; // не докачивать, ошибка или закрытие
							}
						}
						else {
							web_conn->msgbuf = (uint8 *) os_malloc(web_conn->msgbufsize);
							if (web_conn->msgbuf == NULL) {
#if DEBUGSOO > 0
								os_printf("ws:mem!\n");
#endif
								websock_tx_close_err(ts_conn, WS_CLOSE_UNEXPECTED_ERROR);
								SetSCB(SCB_FCLOSE|SCB_DISCONNECT);
								return false;
							};
							web_conn->msgbuflen = 0;
							uint32 opcode;
							if(CheckSCB(SCB_RETRYCB)) { // повторный callback? да
								if(web_conn->func_web_cb != NULL) web_conn->func_web_cb(ts_conn);
								if(!CheckSCB(SCB_RETRYCB)) {
									ClrSCB(SCB_FCLOSE | SCB_DISCONNECT);
									opcode = WS_OPCODE_CONTINUE | WS_FRAGMENT_FIN;
								}
								else opcode = WS_OPCODE_CONTINUE;
							}
							else {
								pstr[len] = '\0';
								uint8 *vstr = os_strchr(pstr, '=');
								if(vstr != NULL) {
									*vstr++ = '\0';
									web_int_vars(ts_conn, pstr, vstr);
								}
								else {
									web_conn->msgbuf[0] = 0;
									web_int_callback(ts_conn, pstr);
								}
								if(CheckSCB(SCB_RETRYCB)) opcode = WS_OPCODE_TEXT;
								else {
									ClrSCB(SCB_FCLOSE | SCB_DISCONNECT);
									opcode = WS_OPCODE_TEXT | WS_FRAGMENT_FIN;
								}
							}
							if(web_conn->msgbuflen != 0) {
								if(websock_tx_frame(ts_conn, opcode, web_conn->msgbuf, web_conn->msgbuflen) != ERR_OK) {
									os_free(web_conn->msgbuf);
									web_conn->msgbuf = NULL;
									return false; // не докачивать, ошибка или закрытие
								}
							}
							os_free(web_conn->msgbuf);
							web_conn->msgbuf = NULL;
							if(CheckSCB(SCB_RETRYCB)) return false;
						}
					}
/*
					if(0) {
						uint32 opcode = WS_OPCODE_TEXT;
						if(ws->cur_len != 0) opcode = WS_OPCODE_CONTINUE;
						if(ws->frame_len == ws->cur_len + len) opcode |= WS_FRAGMENT_FIN;
						if(websock_tx_frame(ts_conn, opcode, pstr, len) != ERR_OK) {
							return false; // не докачивать, ошибка или закрытие
						}
					}
*/
					ws->cur_len += len;
					ts_conn->cntri += len;
					return true; // докачивать
//					break;
//					break;
				case sw_frs_ping:
#if DEBUGSOO > 1
					os_printf("ws:ping ");
#endif
					{
						uint32 opcode = WS_OPCODE_PONG;
						if(ws->cur_len != 0) opcode = WS_OPCODE_CONTINUE;
						if(ws->frame_len == ws->cur_len + len) opcode |= WS_FRAGMENT_FIN;
						if(websock_tx_frame(ts_conn, opcode, pstr, len) != ERR_OK) {
								return false; // не докачивать, ошибка или закрытие
						}
					}
					ws->cur_len += len;
					ts_conn->cntri += len;
					return true; // докачивать
//					break;
				case sw_frs_pong:
#if DEBUGSOO > 1
					os_printf("ws:pong ");
#endif
					ws->cur_len += len;
					ts_conn->cntri += len;
					break;
//					return true;
				case sw_frs_close:
#if DEBUGSOO > 1
				os_printf("ws:close ");
#endif
//				if((ws->flg & WS_FLG_CLOSE) == 0) {
				{
					if(len >= 2) {
					uint32 close_code = (pstr[0]<<8) | pstr[1];
#if DEBUGSOO > 1
						os_printf("code:%d ", close_code);
#endif
						if(close_code == WS_CLOSE_NORMAL)	websock_tx_close_err(ts_conn, WS_CLOSE_NORMAL);
						// else websock_tx_frame(ts_conn, WS_OPCODE_CLOSE | WS_FRAGMENT_FIN, NULL, 0);
					}
					else
					{
						websock_tx_close_err(ts_conn, WS_CLOSE_NORMAL);
						// websock_tx_frame(ts_conn, WS_OPCODE_CLOSE | WS_FRAGMENT_FIN, NULL, 0);
					}
				}
				ts_conn->flag.pcb_time_wait_free = 1;
				SetSCB(SCB_DISCONNECT);
//					ts_conn->cntri = ts_conn->sizei;
/*				ws->cur_len += len;
				ts_conn->cntri += len; */
				return false;
			default:
#if DEBUGSOO > 0
				os_printf("ws:f?! ");
#endif
				websock_tx_close_err(ts_conn, WS_CLOSE_UNEXPECTED_ERROR);
				SetSCB(SCB_DISCONNECT);
		//		ts_conn->cntri = ts_conn->sizei;
				return false;
			}
		}
		else
		if(ws->cur_len >= ws->frame_len) { // прием и разбор нового фрейма
			if((ws->flg & WS_FLG_FIN) != 0) { // обработка
#if DEBUGSOO > 3
				os_printf("ws_rx:fin=%u ", ws->cur_len);
#endif
			}
			else {
				uint32 ret = WebsocketHead(ws, pstr, len);
				if(ret >= WS_CLOSE_NORMAL) { // error или close

#if DEBUGSOO > 0
					os_printf("ws:txerr=%u ", ret);
#endif
					websock_tx_close_err(ts_conn, ret);
//					ts_conn->cntri = ts_conn->sizei; // убить буфер ts_conn->pbufi
					return false; // error
				}
				else if(ret == 0) {
#if DEBUGSOO > 3
					os_printf("ws_rx... ");
#endif
					return true; // докачивать
				}
				ts_conn->cntri += ws->head_len; // вычесть заголовок
/*
				switch(ws->status) {
					case sw_frs_binary:
						break;
					case sw_frs_text:
						if(ws->frame_len > MAX_RX_BUF_SIZE) {
							websock_tx_close_err(ts_conn, WS_CLOSE_MESSAGE_TOO_BIG);
							return false;
						}
						break;
				}
*/
			}
		}
#if DEBUGSOO > 3
		os_printf("trim%u-%u ", ts_conn->sizei, ts_conn->sizei - ts_conn->cntri );
#endif
		if(!web_trim_bufi(ts_conn, &ts_conn->pbufi[ts_conn->cntri], ts_conn->sizei - ts_conn->cntri)) {
#if DEBUGSOO > 0
			os_printf("ws:trim_err! ");
#endif
			// убить буфер ts_conn->pbufi и ответить ошибкой WS_CLOSE_UNEXPECTED_ERROR
			websock_tx_close_err(ts_conn, WS_CLOSE_UNEXPECTED_ERROR);
			SetSCB(SCB_DISCONNECT);
//			ts_conn->cntri = ts_conn->sizei;
			return false;
		};
	}
	return false; // не докачивать, ошибка или закрытие
}
//=============================================================================
//=============================================================================
#endif // WEBSOCKET_ENA


