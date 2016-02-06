/*
 * File: websock.h
 * Small WEB server ESP8266EX
 * Author: PV`
 */

#ifndef _WEBSOCK_H_
#define _WEBSOCK_H_

//#define WS_NONBLOCK 0x02

/*
    0               1               2               3
	7 6 5 4 3 2 1 0 7 6 5 4 3 2 1 0 7 6 5 4 3 2 1 0 7 6 5 4 3 2 1 0
   +-+-+-+-+-------+-+-------------+-------------------------------+
   |F|R|R|R| опкод |М| Длина тела  |    Расширенная длина тела     |
   |I|S|S|S|(4бита)|А|   (7бит)    |           (2 байта)           |
   |N|V|V|V|       |С|             |(если длина тела==126 или 127) |
   | |1|2|3|       |К|             |                               |
   | | | | |       |А|             |                               |
   +-+-+-+-+-------+-+-------------+ - - - - - - - - - - - - - - - +
   |4               5               6               7              |
   |  Продолжение расширенной длины тела, если длина тела = 127    |
   + - - - - - - - - - - - - - - - +-------------------------------+
   |8               9               10              11             |
   |                               |  Ключ маски, если МАСКА = 1   |
   +-------------------------------+-------------------------------+
   |12              13              14              15             |
   | Ключ маски (продолжение)      |       Данные фрейма ("тело")  |
   +-------------------------------- - - - - - - - - - - - - - - - +
   |16              17              18              19             |
   :                     Данные продолжаются ...                   :
   + - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - +
   |                     Данные продолжаются ...                   |
   +---------------------------------------------------------------+

*/

#define WS_FRAGMENT_FIN		0x80
//#define WS_RSVD_BITS		(7 << 4)
#define WS_OPCODE_BITS		0x7F
#define WS_OPCODE_CONTINUE	0x0 // фрейм-продолжение для фрагментированного сообщения
#define WS_OPCODE_TEXT		0x1	// текстовый фрейм
#define WS_OPCODE_BINARY	0x2 // двоичный фрейм
#define WS_OPCODE_CLOSE		0x8 // закрытие соединения
#define WS_OPCODE_PING		0x9
#define WS_OPCODE_PONG		0xa
#define WS_MASK_FLG			(1 << 7)
#define WS_SIZE1_BITS		0x7F


#define WS_CLOSE_NORMAL				1000
#define WS_CLOSE_GOING_AWAY			1001
#define WS_CLOSE_PROTOCOL_ERROR		1002
#define WS_CLOSE_NOT_ALLOWED		1003
#define WS_CLOSE_RESERVED			1004
#define WS_CLOSE_NO_CODE			1005
#define WS_CLOSE_DIRTY				1006
#define WS_CLOSE_WRONG_TYPE			1007
#define WS_CLOSE_POLICY_VIOLATION	1008
#define WS_CLOSE_MESSAGE_TOO_BIG	1009
#define WS_CLOSE_UNEXPECTED_ERROR	1011

typedef struct _WS_FRSTAT
{
	uint32	frame_len;	// размер данных в заголовке фрейма
	uint32	cur_len;	// счетчик обработанных данных
	union {
		unsigned char uc[4];
		unsigned int ud;
	}mask;	// маска принимаемых данных
	uint8	status;
	uint8	flg;
	uint8	head_len;
}WS_FRSTAT;

#define WS_FLG_MASK			0x01
#define WS_FLG_FIN			0x02
#define WS_FLG_CLOSE		0x04 // уже передано WS_CLOSE

enum WS_FRAME_STATE {
	sw_frs_none = 0,
	sw_frs_text,
	sw_frs_binary,
	sw_frs_close,
	sw_frs_ping,
	sw_frs_pong
};

extern const uint8 WebSocketHTTPOkKey[]; // ICACHE_RODATA_ATTR = "HTTP/1.1 101 Switching Protocols\r\nUpgrade: websocket\r\nConnection: Upgrade\r\nSec-WebSocket-Accept:%s\r\n\r\n"
extern const uint8 WebSocketAddKey[]; // ICACHE_RODATA_ATTR = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
#define sizeWebSocketAddKey 36
#define sizeWebSocketKey 24
#define sizeWebSocketAcKey 28
extern const uint8 *HTTPUpgrade; // = "Upgrade:";
#define sizeHTTPUpgrade 8
extern const uint8 *HTTPwebsocket; // = "websocket";
#define sizeHTTPwebsocket 9
extern const uint8 *HTTPSecWebSocketKey; // = "Sec-WebSocket-Key:";
#define sizeHTTPSecWebSocketKey 18


bool WebSocketAcceptKey(uint8* dkey, uint8* skey);
void WebsocketMask(WS_FRSTAT *ws, uint8 *raw_data, uint32 raw_len);
uint32 WebsocketHead(WS_FRSTAT *ws, uint8 *raw_data, uint32 raw_len);
err_t WebsocketTxFrame(TCP_SERV_CONN *ts_conn, uint32 opcode, uint8 *raw_data, uint32 raw_len);

#endif /* _WEBSOCK_H_ */
