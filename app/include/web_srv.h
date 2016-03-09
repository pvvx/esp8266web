/*
 * File: web_srv.h
 * Description: The web server configration.
 * Small WEB server ESP8266EX
 * Author: PV` 
 */

#ifndef _INCLUDE_WEB_SRV_H_
#define _INCLUDE_WEB_SRV_H_

#include "tcp_srv_conn.h"
#ifdef WEBSOCKET_ENA
#include "websock.h"
#endif

#define WEB_SVERSION "0.1.4"
#define DEFAULT_WEB_PORT USE_WEB // 80

/****************************************************************************
  Section:
        Commands and Server Responses
  ***************************************************************************/

// File type definitions
typedef enum
{
        HTTP_TXT = 0,     // File is a text document
        HTTP_HTML,        // File is HTML (extension .htm)
        HTTP_CGI,         // File is HTML (extension .cgi)
        HTTP_XML,         // File is XML (extension .xml)
        HTTP_CSS,         // File is stylesheet (extension .css)
        HTTP_ICO,         // File is ICO  vnd.microsoft.icon
        HTTP_GIF,         // File is GIF image (extension .gif)
        HTTP_PNG,         // File is PNG image (extension .png)
        HTTP_JPG,         // File is JPG image (extension .jpg)
		HTTP_SVG,		  // File is SVG image (extension .svg)
        HTTP_JAVA,        // File is java (extension .js)
        HTTP_SWF,         // File is ShockWave-Flash (extension .swf)
        HTTP_WAV,         // File is audio (extension .wav)
        HTTP_PDF,         // File is PDF (extension .pdf)
        HTTP_ZIP,         // File is ZIP (extension .zip)
        HTTP_BIN,         // File is BIN (extension .bin)
        HTTP_UNKNOWN      // File type is unknown
} HTTP_FILE_TYPE;


// extended state data for each connection
#define FileNameSize 64
#define VarNameSize 64
#define CmdNameSize 32

typedef struct
{
    uint16 httpStatus;        // Request method/status
    HTTP_FILE_TYPE fileType;       // File type to return with Content-Type
    uint8 pFilename[FileNameSize];
	uint8 *puri; 		// указатель на строку с переменными запроса к файлу
	uint16 uri_len;
	uint8 *phead; 		// HTTP Headers
	uint16 head_len;
	uint8 *pcookie; 	// cookie
	uint16 cookie_len;
	uint8 *pcontent; 	// content
	uint32 content_len; //
	uint8 httpver;  // версия HTTP клиента в BCD (0x00 = неизвестен; 0x09 = HTTP/0.9; 0x10 = HTTP/1.0; 0x11 = HTTP/1.1)
} HTTP_CONN;


typedef void (* web_func_cb)(TCP_SERV_CONN *ts_conn);
typedef uint32 (* web_func_disc_cb)(uint32 flg); // отложенная функция, когда соединение закрыто

typedef struct
{
	uint32 webflag;		// флаги для http/web сервера
	uint8  bffiles[4];	// четыре Files pointers для оработки вложенных файлов include
	uint32 udata_start;	// udata "start=0x..."
	uint32 udata_stop;	// udata "stop=0x..."
	uint8  *msgbuf;		// указатель на текущий буфер вывода
	uint16 msgbuflen;	// кол-во занятых байт в буфере msgbuf
	uint16 msgbufsize;	// размер буфера
	web_func_cb func_web_cb; // calback функция у httpd для обработки приема/передачи кусками
	uint32 content_len; // размер файла для передачи (GET/POST) или приема, если принимается внешний файл (POST + SCB_RXDATA)
	web_func_disc_cb web_disc_cb; // функция вызываемая по закрытию соединения
	uint32 web_disc_par; // параметры функции вызываемой по закрытию соединения
#ifdef WEBSOCKET_ENA
	WS_FRSTAT ws;	// параметры websoc
#endif
} WEB_SRV_CONN;

typedef enum
{
	WEBFS_MAX_HANDLE = 251,
	WEBFS_NODISK_HANDLE,
	WEBFS_WEBCGI_HANDLE,
	WEBFS_UPLOAD_HANDLE
} WEBFS_NUM_HANDLE;

// webflag:

#define  SCB_CLOSED		0x00001 // соединение закрыто
#define  SCB_DISCONNECT	0x00002 // выход на DISCONNECT
#define  SCB_FCLOSE		0x00004 // закрыть файлы
#define  SCB_FOPEN		0x00008 // файл(ы) открыт(ы)
#define  SCB_FCALBACK	0x00010 // file use ~calback~
#define  SCB_FGZIP		0x00020 // файл GZIP
#define  SCB_CHUNKED	0x00040 // передача шинковкой
#define  SCB_RETRYCB	0x00080 // вызвать повтор CalBack
#define  SCB_POST		0x00100 // POST
#define  SCB_GET		0x00200 // GET
#define  SCB_AUTH		0x00400 // необходима авторизация
#define  SCB_FINDCB		0x00800 // используется парсингом ~calback~
#define  SCB_RXDATA		0x01000 // прием данных (файла)
#define  SCB_HEAD_OK	0x02000 // заголовок HTTP принят и обработан
#define  SCB_BNDR		0x04000 // прилеплен Content-Type: multipart/form-data; boundary="..."
#define  SCB_REDIR		0x08000 // Redirect 302
#define  SCB_WEBSOC		0x10000 // WebSocket
#define  SCB_WSDATA		0x20000 // WebSocket data
#define  SCB_SYSSAVE	0x40000 // по закрытию соединения вызвать sys_write_cfg()


#define  SCB_OPEN       0

#define  SetSCB(a) web_conn->webflag |= a
#define  FreeSCB() web_conn->webflag = SCB_FREE
#define  SetNextFunSCB(a) web_conn->func_web_cb = a
#define  ClrSCB(a) web_conn->webflag &= ~(a)
#define  CheckSCB(a) (web_conn->webflag & (a))

#define  FreeSCB() web_conn->webflag = SCB_FREE
#define  OpenSCB() web_conn->webflag = SCB_OPEN

#define MAXLENBOUNDARY 64
typedef struct s_http_upload
{
  uint16 status;
  uint8 boundary[MAXLENBOUNDARY+1];
  uint16 sizeboundary;
  uint8  name[VarNameSize];
  uint8  filename[VarNameSize];
#ifdef USE_OVERLAY
  uint32 segs; // кол-во сегментов оверлея // пока в web_conn->web_disc_par
  uint32 start; // адрес запуска оверлея
#endif
  uint32 fsize;
  uint32 faddr;
  uint8 *pbndr;
  uint8 *pnext;
} HTTP_UPLOAD;

typedef struct s_http_response
{
  uint32 status;
  uint32 flag;
  const char * headers;
  const char * default_content;
} HTTP_RESPONSE;

// HTTP_RESPONSE.flags:
#define HTTP_RESP_FLG_END       0x8000
#define HTTP_RESP_FLG_NONE      0x0000
#define HTTP_RESP_FLG_FINDFILE  0x0001
#define HTTP_RESP_FLG_REDIRECT  0x0002

#define tcp_put(a) web_conn->msgbuf[web_conn->msgbuflen++] = a
#define tcp_htmlstrcpy(str, len) web_conn->msgbuflen += htmlcode(&web_conn->msgbuf[web_conn->msgbuflen], str, web_conn->msgbufsize - web_conn->msgbuflen - 1, len)
//#define tcp_urlstrcpy(str, len) web_conn->msgbuflen += urlencode(&web_conn->msgbuf[web_conn->msgbuflen], str, web_conn->msgbufsize - web_conn->msgbuflen - 1, len)
#define tcp_puts(...) web_conn->msgbuflen += ets_sprintf((char *)&web_conn->msgbuf[web_conn->msgbuflen], __VA_ARGS__)
#define tcp_puts_fd(fmt, ...) do { \
		static const char flash_str[] ICACHE_RODATA_ATTR = fmt;	\
		web_conn->msgbuflen += ets_sprintf((char *)&web_conn->msgbuf[web_conn->msgbuflen], (char *)flash_str, ##__VA_ARGS__); \
		} while(0)
//#define tcp_strcpy(a) web_conn->msgbuflen += ets_strlen((char *)ets_strcpy((char *)&web_conn->msgbuf[web_conn->msgbuflen], (char *)a))
#define tcp_strcpy(a) web_conn->msgbuflen += rom_xstrcpy((char *)&web_conn->msgbuf[web_conn->msgbuflen], (const char *)a)
#define tcp_strcpy_fd(fmt) do { \
		static const char flash_str[] ICACHE_RODATA_ATTR = fmt;	\
		web_conn->msgbuflen += rom_xstrcpy((char *)&web_conn->msgbuf[web_conn->msgbuflen], (char *)flash_str); \
		} while(0)
uint32 ahextoul(uint8 *s) ICACHE_FLASH_ATTR;
err_t webserver_init(uint16 portn) ICACHE_FLASH_ATTR;
err_t webserver_close(uint16 portn) ICACHE_FLASH_ATTR;
err_t webserver_reinit(uint16 portn) ICACHE_FLASH_ATTR;

#endif /* _INCLUDE_WEB_SRV_H_ */
