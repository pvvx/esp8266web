/******************************************************************************
 * FileName: webserver.c
 * Description: Small WEB server + WebSocket in ESP8266EX
 * Author: PV`
 * ver1.0 25/12/2014  SDK 0.9.4
 * ver1.1 02/04/2015  SDK 1.0.0
*******************************************************************************/
#include "user_config.h"
#ifdef USE_WEB
#include "bios.h"
#include "sdk/add_func.h"
#include "ets_sys.h"
#include "os_type.h"
#include "osapi.h"
#include "lwip/tcp.h"
#include "user_interface.h"
#include "webfs.h"
#include "tcp_srv_conn.h"
#include "web_srv_int.h"
#include "web_utils.h"
#include "web_iohw.h"
#include "sdk/flash.h"
#include "flash_eep.h"
#include "hw/esp8266.h"
#include "sys_const_utils.h"
#include "wifi.h"
#include "sdk/rom2ram.h"

#ifdef WEBSOCKET_ENA
#include "web_websocket.h"
#endif

#ifdef USE_CAPTDNS
#include "captdns.h"
#endif

#ifdef USE_OVERLAY
#include "overlay.h"
#endif

#define USE_WEB_NAGLE // https://en.wikipedia.org/wiki/Nagle%27s_algorithm
#define MIN_REQ_LEN  7  // Minimum length for a valid HTTP/0.9 request: "GET /\r\n" -> 7 bytes
#define CRLF "\r\n"

#define max_len_buf_write_flash 2048 // размер буфера при записи flash. Увеличение/уменньшение размера (до сектора 4096) ускорения не дает (1..2%)

#define mMIN(a, b)  ((a<b)?a:b)
#define mMAX(a, b)  ((a>b)?a:b)

extern int rom_atoi(const char *);
#define atoi(s) rom_atoi(s)

LOCAL void web_print_headers(HTTP_CONN *CurHTTP, TCP_SERV_CONN *ts_conn) ICACHE_FLASH_ATTR ;

//LOCAL void webserver_discon(void *arg) ICACHE_FLASH_ATTR;
//LOCAL void webserver_recon(void *arg, sint8 err) ICACHE_FLASH_ATTR;
LOCAL void webserver_send_fdata(TCP_SERV_CONN *ts_conn) ICACHE_FLASH_ATTR;
LOCAL void web_int_disconnect(TCP_SERV_CONN *ts_conn)  ICACHE_FLASH_ATTR;
LOCAL bool webserver_open_file(HTTP_CONN *CurHTTP, TCP_SERV_CONN *ts_conn) ICACHE_FLASH_ATTR;
LOCAL void webserver_file_ext(HTTP_CONN *CurHTTP, uint8 *pfname) ICACHE_FLASH_ATTR;

const char http_default_file[] ICACHE_RODATA_ATTR = "index.htm";
const char web_cgi_fname[] ICACHE_RODATA_ATTR = "web.cgi";
const char fsupload_fname[] ICACHE_RODATA_ATTR = "fsupload";
#ifdef USE_CAPTDNS
const char ncsi_txt_fname[] ICACHE_RODATA_ATTR = "ncsi.txt";
//const char generate_204_fname[] ICACHE_RODATA_ATTR = "generate_204";
const char *HTTPHost ="Host:";
#define sizeHTTPHost 5
#endif
#define ProtectedFilesName		"protect"

#define MAX_NO_DATA_BUF_SIZE (8192) // if(ts_conn->sizei > MAX_NO_DATA_BUF_SIZE) CurHTTP->httpStatus = 418; // 418: Out of Coffee

/****************************************************************************
  Section:
        File and Content Type Settings
  ***************************************************************************/
        // File type extensions corresponding to HTTP_FILE_TYPE
static const char *httpFileExtensions[] = {
		"txt",          // HTTP_TXT
        "htm",          // HTTP_HTML
        "cgi",          // HTTP_CGI
        "xml",          // HTTP_XML
        "css",          // HTTP_CSS
        "ico",          // HTTP_ICO
        "gif",          // HTTP_GIF
        "png",          // HTTP_PNG
        "jpg",          // HTTP_JPG
		"svg",			// HTTP_SVG
        "js",           // HTTP_JAVA
        "swf",          // HTTP_SWF
        "wav",          // HTTP_WAV
        "pdf",          // HTTP_PDF
        "zip",          // HTTP_ZIP
        "bin",          // HTTP_BIN
        "\0\0\0"        // HTTP_UNKNOWN
};

// Content-type strings corresponding to HTTP_FILE_TYPE
static const char *httpContentTypes[] = {
	"text/plain",               // HTTP_TXT       "txt",
	"text/html",                // HTTP_HTM       "htm",
    "magnus-internal/cgi",      // HTTP_CGI       "cgi",
    "text/xml",                 // HTTP_XML       "xml",
    "text/css",                 // HTTP_CSS       "css",
    "image/vnd.microsoft.icon", // HTTP_ICO       "ico",
    "image/gif",                // HTTP_GIF       "gif",
    "image/png",                // HTTP_PNG       "png",
    "image/jpeg",               // HTTP_JPG       "jpg",
	"image/svg+xml",			// HTTP_SVG       "svg",
    "text/javascript",          // HTTP_JAVA      "js",
    "application/x-shockwave-flash", // HTTP_SWF  "swf",
    "audio/x-wave",             // HTTP_WAV       "wav",
    "application/pdf",          // HTTP_PDF       "pdf",
    "application/zip",          // HTTP_ZIP       "zip",
    "application/octet-stream", // HTTP_BIN       "bin",
    ""  // HTTP_UNKNOWN
};
/****************************************************************************
  Section:
        Commands and Server Responses
  ***************************************************************************/
const char HTTPresponse_200_head[] ICACHE_RODATA_ATTR = "OK";
const char HTTPresponse_302_head[] ICACHE_RODATA_ATTR = "Found";
const char HTTPresponse_304_head[] ICACHE_RODATA_ATTR = "Not Modified";
const char HTTPresponse_400_head[] ICACHE_RODATA_ATTR = "Bad Request";
const char HTTPresponse_401_head[] ICACHE_RODATA_ATTR = "Unauthorized\r\nWWW-Authenticate: Basic realm=\"Protected\"";
const char HTTPresponse_404_head[] ICACHE_RODATA_ATTR = "Not found";
const char HTTPresponse_411_head[] ICACHE_RODATA_ATTR = "Length Required";
const char HTTPresponse_413_head[] ICACHE_RODATA_ATTR = "Request Entity Too Large";
const char HTTPresponse_414_head[] ICACHE_RODATA_ATTR = "Request-URI Too Long";
const char HTTPresponse_418_head[] ICACHE_RODATA_ATTR = "I'm a teapot";
const char HTTPresponse_429_head[] ICACHE_RODATA_ATTR = "Too Many Requests\r\nRetry-After: 30";
const char HTTPresponse_500_head[] ICACHE_RODATA_ATTR = "Internal Server Error";
const char HTTPresponse_501_head[] ICACHE_RODATA_ATTR = "Not Implemented\r\nAllow: GET, POST";

const char HTTPresponse_401_content[] ICACHE_RODATA_ATTR = "401 Unauthorized: Password required\r\n";
const char HTTPresponse_404_content[] ICACHE_RODATA_ATTR = "404: File not found\r\n";
const char HTTPresponse_411_content[] ICACHE_RODATA_ATTR = "411 The request must have a content length\r\n";
const char HTTPresponse_413_content[] ICACHE_RODATA_ATTR = "413 Request Entity Too Large: There's too many letters :)\r\n";
const char HTTPresponse_414_content[] ICACHE_RODATA_ATTR = "414 Request-URI Too Long: Buffer overflow detected\r\n";
const char HTTPresponse_418_content[] ICACHE_RODATA_ATTR = "418: Out of Coffee\r\n";
const char HTTPresponse_500_content[] ICACHE_RODATA_ATTR = "500 Internal Server Error\r\n";
const char HTTPresponse_501_content[] ICACHE_RODATA_ATTR = "501 Not Implemented: Only GET and POST supported\r\n";

// Initial response strings (Corresponding to HTTP_STATUS)
static const HTTP_RESPONSE ICACHE_RODATA_ATTR HTTPResponse[] ICACHE_RODATA_ATTR = {
        { 200, HTTP_RESP_FLG_NONE,
        		HTTPresponse_200_head,
                NULL },
         // успешный запрос. Если клиентом были запрошены какие-либо данные, то они находятся в заголовке и/или теле сообщения.
        { 302, HTTP_RESP_FLG_NONE | HTTP_RESP_FLG_REDIRECT,
        		HTTPresponse_302_head,
                NULL },
// "HTTP/1.1 302 Found\r\nConnection: close\r\nLocation: ",
         // 302 Found, 302 Moved Temporarily - запрошенный документ временно
         // доступен по другому URI, указанному в заголовке в поле Location.
         // Этот код может быть использован, например, при управляемом сервером
         // согласовании содержимого. Некоторые клиенты некорректно ведут себя
         // при обработке данного кода.
        { 304, HTTP_RESP_FLG_NONE,
        		HTTPresponse_304_head,
                NULL },
///"304 Redirect: ",   // If-Modified-Since If-None-Match
         // сервер возвращает такой код, если клиент запросил документ методом GET,
         // использовал заголовок If-Modified-Since или If-None-Match и документ
         // не изменился с указанного момента. При этом сообщение сервера не должно содержать тела.
        { 400, HTTP_RESP_FLG_FINDFILE,
        		HTTPresponse_400_head,
                NULL} ,
         // сервер обнаружил в запросе клиента синтаксическую ошибку.
        { 401, HTTP_RESP_FLG_FINDFILE,
        		HTTPresponse_401_head,
				HTTPresponse_401_content },
         // для доступа к запрашиваемому ресурсу требуется аутентификация.
         // В заголовке ответ должен содержать поле WWW-Authenticate с перечнем
         // условий аутентификации. Клиент может повторить запрос,
         // включив в заголовок сообщения поле Authorization с требуемыми для аутентификации данными.
//"HTTP/1.1 403 Forbidden\r\nConnection: close\r\n\r\n403 Forbidden: SSL Required - use HTTPS\r\n"
        { 404, HTTP_RESP_FLG_FINDFILE,
        		HTTPresponse_404_head,
				HTTPresponse_404_content },
         // Сервер понял запрос, но не нашёл соответствующего ресурса по указанному URI.
        { 411, HTTP_RESP_FLG_FINDFILE,
        		HTTPresponse_411_head,
				HTTPresponse_411_content },
         // для указанного ресурса клиент должен указать Content-Length в заголовке запроса.
         // Без указания этого поля не стоит делать повторную попытку запроса к серверу по данному URI.
         // Такой ответ естественен для запросов типа POST и PUT.
         // Например, если по указанному URI производится загрузка файлов, а на сервере стоит
         // ограничение на их объём. Тогда разумней будет проверить в самом начале заголовок
         // Content-Length и сразу отказать в загрузке, чем провоцировать бессмысленную нагрузку,
         // разрывая соединение, когда клиент действительно пришлёт слишком объёмное сообщение.
        { 413, HTTP_RESP_FLG_FINDFILE,
        		HTTPresponse_413_head,
				HTTPresponse_413_content },
         // возвращается в случае, если сервер отказывается обработать запрос
         // по причине слишком большого размера тела запроса. Сервер может закрыть соединение,
         // чтобы прекратить дальнейшую передачу запроса.
        { 414, HTTP_RESP_FLG_FINDFILE,
        		HTTPresponse_414_head,
				HTTPresponse_414_content },
         // сервер не может обработать запрос из-за слишком длинного указанного URL.
         // Такую ошибку можно спровоцировать, например, когда клиент пытается передать длинные
         // параметры через метод GET, а не POST.
        { 429, HTTP_RESP_FLG_NONE,
        		HTTPresponse_429_head,
                NULL },
         // клиент попытался отправить слишком много запросов за короткое время, что может указывать,
         // например, на попытку DoS-атаки. Может сопровождаться заголовком Retry-After, указывающим,
         // через какое время можно повторить запрос.
        { 501, HTTP_RESP_FLG_FINDFILE,
        		HTTPresponse_501_head,
				HTTPresponse_501_content },
         // сервер не поддерживает возможностей, необходимых для обработки запроса.
         // Типичный ответ для случаев, когда сервер не понимает указанный в запросе метод. + см 405
        { 418, HTTP_RESP_FLG_FINDFILE,
        		HTTPresponse_418_head,
				HTTPresponse_418_content },
         // http://en.wikipedia.org/wiki/Hyper_Text_Coffee_Pot_Control_Protocol
		{ 500, HTTP_RESP_FLG_END,
				HTTPresponse_500_head,
				HTTPresponse_500_content }
		// любая внутренняя ошибка сервера, которая не входит в рамки остальных ошибок класса.
};
const char HTTPfsupload[] ICACHE_RODATA_ATTR = "<html><body style='margin:100px'><form method='post' action='/fsupload' enctype='multipart/form-data'><b>File Upload</b><p><input type='file' name='file' size=40> <input type='submit' value='Upload'></form></body></html>";
#define sizeHTTPfsupload 220
const char HTTPdefault[] ICACHE_RODATA_ATTR = "<html><h3>ESP8266 Built-in Web server <sup><i>&copy</i></sup></h3></html>";
#define sizeHTTPdefault 73
const char HTTPfserror[] ICACHE_RODATA_ATTR = "<html><h3>Web-disk error. Upload the WEBFiles.bin!</h3></html>";
#define sizeHTTPfserror 62

const char HTTPAccessControlAllowOrigin[] ICACHE_RODATA_ATTR = "Access-Control-Allow-Origin: *\r\n";
//        const uint8 *HTTPCacheControl = "Cache-Control:";
const char *HTTPContentLength = "Content-Length:";
#define sizeHTTPContentLength 15
//        const uint8 *HTTPConnection = "Connection: ";
//        #define sizeHTTPConnection 12
//        const uint8 *HTTPkeepalive = "keep-alive";
//        #define sizeHTTPkeepalive 10
//        const uint8 *HTTPIfNoneMatch = "If-None-Match:"
//        #define sizeHTTPIfNoneMatch 14
const char *HTTPContentType = "Content-Type:";
#define sizeHTTPContentType 13
const char *HTTPmultipartformdata = "multipart/form-data";
#define sizeHTTPmultipartformdata 19
const char *HTTPboundary = "boundary=";
#define sizeHTTPboundary 9
const char *HTTPAuthorization = "Authorization:";
#define sizeHTTPAuthorization 14
const char *HTTPCookie = "Cookie:";
#define sizeHTTPCookie 7

/******************************************************************************
 * FunctionName : Close_web_conn
 * Description  : Free  ts_conn
 * Parameters   : struct TCP_SERV_CONN
 * Returns      : none
*******************************************************************************/
LOCAL void ICACHE_FLASH_ATTR Close_web_conn(TCP_SERV_CONN *ts_conn)
{
	WEB_SRV_CONN *web_conn = (WEB_SRV_CONN *)ts_conn->linkd;
	int i = 0;
     do {
       if(web_conn->bffiles[i] != WEBFS_INVALID_HANDLE) {
#if DEBUGSOO > 1
	os_printf("cf%d ", web_conn->bffiles[i]);
#endif
         if(web_conn->bffiles[i] <= WEBFS_MAX_HANDLE) WEBFSClose(web_conn->bffiles[i]);
         web_conn->bffiles[i] = WEBFS_INVALID_HANDLE;
       };
       i++;
     }while(i < 4);
     ClrSCB(SCB_FOPEN | SCB_FGZIP | SCB_FCALBACK);
}
/******************************************************************************
 * FunctionName : ReNew_web_conn
 * Description  :
 * Parameters   : struct TCP_SERV_CONN
 * Returns      : none
*******************************************************************************/
LOCAL WEB_SRV_CONN * ICACHE_FLASH_ATTR ReNew_web_conn(TCP_SERV_CONN *ts_conn)
{
	WEB_SRV_CONN *web_conn = (WEB_SRV_CONN *)ts_conn->linkd;
	if(web_conn == NULL) {
		web_conn = (WEB_SRV_CONN *)os_zalloc(sizeof(WEB_SRV_CONN));
		if(web_conn != NULL) {
			web_conn->bffiles[0] = WEBFS_INVALID_HANDLE;
			web_conn->bffiles[1] = WEBFS_INVALID_HANDLE;
			web_conn->bffiles[2] = WEBFS_INVALID_HANDLE;
			web_conn->bffiles[3] = WEBFS_INVALID_HANDLE;
		//  web_conn->webflag = 0; //zalloc
		//  web_conn->func_web_cb = NULL; //zalloc
			OpenSCB(); // сбросить флаги
			ts_conn->linkd = (void *)web_conn;
		};
	}
    return web_conn;
}
//=============================================================================
// Authorization: Basic QWxhZGRpbjpvcGVuIHNlc2FtZQ==\n"
// The resulting string is then encoded using the RFC2045-MIME variant of Base64,
// except not limited to 76 uint8/line
// /ssl/crypto/ssl_crypto_misc.c:
// EXP_FUNC int STDCALL base64_decode(const uint8 *in,  int len, uint8_t *out, int *outlen);
// Username and password are combined into a string "username:password"
LOCAL bool ICACHE_FLASH_ATTR CheckAuthorization(uint8* base64str)
{
	uint8 *pcmp = base64str;
	int len = 0;
	while(*pcmp++ >= '+') len++;
//	struct softap_config apcfg;
	uint8 pbuf[77];
	int declen = 76;
	if((len >= 4)&&(len <= 128)
	&&(base64decode(base64str, len, pbuf, &declen))
/*
		&&(wifi_softap_get_config(&apcfg))
		&&(apcfg.ssid_len <= 31))
*/	) {
		pbuf[declen]='\0';
		uint8 ppsw[32+64+1];
		cmpcpystr(ppsw, wificonfig.ap.config.ssid, '\0','\0', 32);
		len = ets_strlen((char*)ppsw);
		ppsw[len++] = ':';
		cmpcpystr(&ppsw[len], wificonfig.ap.config.password, '\0','\0', 64);
#if DEBUGSOO > 1
		os_printf("'%s' ", pbuf);
#endif
#if DEBUGSOO > 2
		os_printf("<%s>[%u] ", ppsw, declen);
#endif
		if(os_strncmp(pbuf, (char *)ppsw , declen) == 0) return true;
	};
	return false;
}
//=============================================================================

//=============================================================================
LOCAL void ICACHE_FLASH_ATTR
web_parse_cookie(HTTP_CONN *CurHTTP, TCP_SERV_CONN *ts_conn)
{
	if((CurHTTP->pcookie == NULL)||(CurHTTP->cookie_len == 0)) return;
	uint8 pcmd[CmdNameSize];
	uint8 pvar[VarNameSize*3];
	uint8 *pcmp = CurHTTP->pcookie - 1;
	do {
		pcmp = cmpcpystr(pvar, ++pcmp, '\0', '=', sizeof(pvar)-1);
		if(pcmp == NULL) return;
		urldecode(pcmd, pvar, CmdNameSize - 1, sizeof(pvar));
		pcmp = cmpcpystr(pvar, pcmp, '=', ';', sizeof(pvar)-1);
		if(pcmd[0]!='\0') {
			urldecode(pvar, pvar, VarNameSize - 1, sizeof(pvar));
			web_int_vars(ts_conn, pcmd, pvar);
	    }
	} while(pcmp != NULL);
}
//=============================================================================
LOCAL void ICACHE_FLASH_ATTR
web_parse_uri_vars(HTTP_CONN *CurHTTP, TCP_SERV_CONN *ts_conn)
{
	if((CurHTTP->puri == NULL)||(CurHTTP->uri_len == 0)) return;
	uint8 pcmd[CmdNameSize];
	uint8 pvar[VarNameSize*3];
	uint8 *pcmp = CurHTTP->puri;
	uint8 c = '?';
	pcmp = cmpcpystr(NULL, pcmp, '\0', c, CurHTTP->uri_len);
	while(pcmp != NULL) {
		pcmp = cmpcpystr(pvar, pcmp, c, '=', sizeof(pvar)-1);
		if(pcmp == NULL) return;
		urldecode(pcmd, pvar, CmdNameSize - 1, sizeof(pvar));
		c = '&';
		pcmp = cmpcpystr(pvar, pcmp, '=', c, sizeof(pvar)-1);
		if(pcmd[0]!='\0') {
			urldecode(pvar, pvar, VarNameSize - 1, sizeof(pvar));
			web_int_vars(ts_conn, pcmd, pvar);
	    }
	};
}
//=============================================================================
LOCAL void ICACHE_FLASH_ATTR
web_parse_content(HTTP_CONN *CurHTTP, TCP_SERV_CONN *ts_conn)
{
	if((CurHTTP->pcontent == NULL)||(CurHTTP->content_len == 0)) return;
	uint8 pcmd[CmdNameSize];
	uint8 pvar[VarNameSize*3];
	uint8 *pcmp = CurHTTP->pcontent;
	uint8 c = '\0';
	do {
		pcmp = cmpcpystr(pvar, pcmp, c, '=', sizeof(pvar)-1);
		if(pcmp == NULL) return;
		urldecode(pcmd, pvar, CmdNameSize - 1, sizeof(pvar));
		c = '&';
		pcmp = cmpcpystr(pvar, pcmp, '=', c, sizeof(pvar)-1);
		if(pcmd[0]!='\0') {
			urldecode(pvar, pvar, VarNameSize - 1, sizeof(pvar));
			web_int_vars(ts_conn, pcmd, pvar);
	    }
	} while(pcmp != NULL);
}
//=============================================================================
// Разбор имени файла и перевод в вид относительного URI.
// (выкидывание HTTP://Host)
// Проверка на обращение в папку или имя файла требующее пароль
//=============================================================================
LOCAL void ICACHE_FLASH_ATTR
web_parse_fname(HTTP_CONN *CurHTTP, TCP_SERV_CONN *ts_conn)
{
    if(CurHTTP->puri == NULL) return;
    if(CurHTTP->uri_len < 2) { // = "/"?
    	CurHTTP->pFilename[0] = CurHTTP->puri[0];
    	return;
    }
    {
        uint8 cbuf[FileNameSize+16];
        uint8 *pcbuf = cbuf;
        urldecode(pcbuf, CurHTTP->puri, sizeof(cbuf) - 1, CurHTTP->uri_len);
    	if((os_strncmp((char *)pcbuf, "HTTP://", 7) == 0)||(os_strncmp((char *)pcbuf, "http://", 7) == 0)) {
    		pcbuf += 7;
    		uint8 *pcmp = os_strchr((char *)pcbuf, '/');
    		if(pcmp != NULL) pcbuf = pcmp;
    	};
        cmpcpystr(CurHTTP->pFilename, pcbuf, '\0', '?', FileNameSize);
    };
    { // Проверка на обращение в папку или имя файла требующее пароль
    	uint8 *pcmp = web_strnstr(CurHTTP->pFilename, ProtectedFilesName, os_strlen(CurHTTP->pFilename));
        if(pcmp != NULL) {
        	WEB_SRV_CONN *web_conn = (WEB_SRV_CONN *)ts_conn->linkd;
        	SetSCB(SCB_AUTH);
        }
    };
}
//=============================================================================
//=============================================================================
uint8 * ICACHE_FLASH_ATTR head_find_ctr(HTTP_CONN *CurHTTP, const uint8 * c, int clen, int dlen)
{
	if(CurHTTP->head_len < clen + dlen + 2) return NULL; // + "\r\n"
	uint8 * pstr = web_strnstr((char *)CurHTTP->phead, c, CurHTTP->head_len);
	if(pstr != NULL) {
		pstr += clen;
		uint8 *pend = web_strnstr(pstr, CRLF, CurHTTP->phead + CurHTTP->head_len - pstr);
		if(pend == NULL) {
			CurHTTP->httpStatus = 400; // 400 Bad Request
			return NULL;
		}
		while(*pstr == ' ' && pstr < pend) pstr++;
		if(pend - pstr < dlen) {
			CurHTTP->httpStatus = 400; // 400 Bad Request
			return NULL;
		}
    }
	return pstr;
}
//=============================================================================
// Func: parse_header
// Разбирает докачан или нет заголовок HTTP, что там принято, GET или POST,
// открывает файл и проверяет content, если это POST и не прием файла.
//=============================================================================
LOCAL bool ICACHE_FLASH_ATTR
parse_header(HTTP_CONN *CurHTTP, TCP_SERV_CONN *ts_conn)
{
	WEB_SRV_CONN *web_conn = (WEB_SRV_CONN *)ts_conn->linkd;
	CurHTTP->httpStatus = 501; // 501 Not Implemented (not a GET or POST command)

    uint8 *pstr = ts_conn->pbufi;
    uint8 *pend = &ts_conn->pbufi[ts_conn->sizei];
    CurHTTP->pcontent = pend;

    if(pstr == NULL) {
      CurHTTP->httpStatus =  500; // 500 Internal Server Error
      return false;
    };
    if(ts_conn->sizei < MIN_REQ_LEN) return false; // 501 Not Implemented (not a GET or POST command)
    uint8 *pnext = web_strnstr(pstr, CRLF, ts_conn->sizei); // "\r\n"
//    if(pnext != NULL) *pnext = '\0';
    if(pnext == NULL) {
       CurHTTP->httpStatus = 400; // 400 Bad Request
       return false;
    };
    pnext += 2;
    if(pnext - pstr < MIN_REQ_LEN) return false; // 501 размер строки запроса менее "GET /"
    if(os_strncmp(pstr, "GET ", 4) == 0) {
      SetSCB(SCB_GET);
      CurHTTP->httpStatus = 200;
      pstr += 4;
    }
    else if(os_strncmp(pstr, "POST ", 5) == 0) {
      SetSCB(SCB_POST);
      CurHTTP->httpStatus = 200;
      pstr += 5;
    }
    else return false;  // 501 Not Implemented (not a GET or POST command)
	CurHTTP->puri = pstr;
	CurHTTP->uri_len = pnext - pstr;

	if(CurHTTP->uri_len > 10) {  // "/ HTTP/1.0\r\n"
		pstr = web_strnstr(CurHTTP->puri, " HTTP/", CurHTTP->uri_len);
		if(pstr != NULL) {
			if((pstr[7] == '.')&&(pstr[6] <= '9')&&(pstr[6] >= '0')&&(pstr[8] >= '0')&&(pstr[8] <= '9'))
				CurHTTP->httpver = ((pstr[6]-'0')<<4) + pstr[8]-'0';
			// else CurHTTP->ver = 0x00;
		};
	};
#if DEBUGSOO > 3
	os_printf("http_ver=%02x ", CurHTTP->httpver);
#endif
	if(CurHTTP->httpver < 0x10) { // HTTP/0.9 ?
		if(CheckSCB(SCB_POST)) {
			CurHTTP->httpStatus = 400; // 400 HTTP/0.9 does not support POST
			return false; // HTTP/0.9
	    };
	};
	// здесь уже надо глядеть - следует или нет докачивать данные
	pstr = web_strnstr(pnext-2, CRLF CRLF, pend - pnext + 2 ); // find "\r\n\r\n"
	if(pstr == NULL) return true; // докачивать!
	// разбираем дальше Header, раз уже скачан
	pstr += 2;
	if(pstr != pnext) { // есть Headers
		CurHTTP->phead = pnext;
		CurHTTP->head_len = pstr - pnext;
		if(CheckSCB(SCB_POST)){
			pstr += 2;
			CurHTTP->pcontent = pstr;
			CurHTTP->content_len = pend - pstr;
		};
	};
	if(!CheckSCB(SCB_FOPEN)) { // файл уже открыт? нет
		web_parse_fname(CurHTTP, ts_conn);
		if(!webserver_open_file(CurHTTP, ts_conn)) {
			CurHTTP->httpStatus = 404; // "404: File not found"
			return false; //
		};
	};
	if((CurHTTP->phead == NULL)||(CurHTTP->head_len == 0)) {
		// если требуется авторизация, но нет передачи пароля...
		if(CheckSCB(SCB_AUTH)) CurHTTP->httpStatus = 401; // 401 Unauthorized
		return false; // нет Header
	};
	if(CheckSCB(SCB_POST)) {
    	pstr = head_find_ctr(CurHTTP, HTTPContentLength, sizeHTTPContentLength, 1);
    	if(pstr == NULL || CurHTTP->httpStatus != 200) {
    		CurHTTP->httpStatus = 411; // no "Content Length:", 411 Length Required
    		return false;
    	}
        uint32 cnlen = atoi(pstr);
#if DEBUGSOO > 1
        os_printf("content_len = %d of %d ", cnlen, CurHTTP->content_len);
#endif
        if(cnlen) {
    		web_conn->content_len = cnlen; // запомнить размер, для приема файла
        	if(!CheckSCB(SCB_BNDR) && (CurHTTP->head_len > sizeHTTPContentType + sizeHTTPmultipartformdata + sizeHTTPboundary + 2 + 2)) { //"x\r\n"
            	pstr = head_find_ctr(CurHTTP, HTTPContentType, sizeHTTPContentType, sizeHTTPmultipartformdata + sizeHTTPboundary + 2);
            	if(CurHTTP->httpStatus != 200) return false;
            	if(pstr != NULL) {
            		pend = web_strnstr(pstr, CRLF, CurHTTP->phead + CurHTTP->head_len - pstr);
                    pstr = web_strnstr(pstr, HTTPmultipartformdata, pend - pstr);
                    if(pstr != NULL) {
                    	pstr += sizeHTTPmultipartformdata;
                        pstr = web_strnstr(pstr, HTTPboundary, pend - pstr);
                        if(pstr != NULL) {
                        	// сохраним этот "мультипаспорт" (с) 5-ый элемент :)
                   			pstr += sizeHTTPboundary;
                   			HTTP_UPLOAD *pupload = (HTTP_UPLOAD *)os_zalloc(sizeof(HTTP_UPLOAD));
                   			if(pupload == NULL) {
                   				CurHTTP->httpStatus =  500; // 500 Internal Server Error
                   				return false;
                   			}
                        	uint8 x = *pend;
                        	*pend = '\0';
#if DEBUGSOO > 4
                        	os_printf("[%s] ", pstr);
#endif
                   			os_memcpy(pupload->boundary, pstr, MAXLENBOUNDARY);
                        	*pend = x;
                   			pupload->sizeboundary = os_strlen(pupload->boundary);
                   			ts_conn->pbufo = (uint8 *)pupload;
                   			SetSCB(SCB_BNDR);
//                          if(cnlen > ((pupload->sizeboundary * 2) + 18)) {
                        		SetSCB(SCB_RXDATA);
//                          }
                        };
                    };
            	};
            };
        	if((!CheckSCB(SCB_BNDR)) && cnlen > CurHTTP->content_len) { // обычный контент и недокачан заголовок? да.
        		CurHTTP->content_len = cnlen;
#if DEBUGSOO > 2
            	os_printf("wait content ");
#endif
        		CurHTTP->httpStatus = 413; // 413 Request Entity Too Large // пока так
        		return true; // докачивать
           	};
        }
        else CurHTTP->content_len = cnlen; // уточнить, что Content Length = 0
    };
    if(CheckSCB(SCB_AUTH)) {
    	pstr = head_find_ctr(CurHTTP, HTTPAuthorization, sizeHTTPAuthorization, 5 + 3); // "Authorization: Basic 1234\r\n"
    	if(pstr == NULL || CurHTTP->httpStatus != 200) {
    		CurHTTP->httpStatus = 401; // 401 Unauthorized
    		return false;
    	}
        if(os_strncmp(pstr, "Basic", 5) == 0) { // The authorization method and a space i.e. "Basic" is then put before the encoded string.
        	pstr += 5;
        	while(*pstr == ' ') pstr++;
        	if(CheckAuthorization(pstr)) ClrSCB(SCB_AUTH);
            else {
    	   		CurHTTP->httpStatus = 401; // 401 Unauthorized
    	   		return false;
    		};
        }
        else {
	   		CurHTTP->httpStatus = 401; // 401 Unauthorized
	   		return false;
		};
    };

    if(CurHTTP->head_len > sizeHTTPCookie + 4) { // "Cookie: a=\r\n"
    	pstr = head_find_ctr(CurHTTP, HTTPCookie, sizeHTTPCookie, 2);
        if(pstr != NULL) {
    		pend = web_strnstr(pstr, CRLF, CurHTTP->phead + CurHTTP->head_len - pstr);
        	if(pend != NULL) {
        		CurHTTP->pcookie = pstr;
        		CurHTTP->cookie_len = pend - pstr;
#if DEBUGSOO > 3
        		*pend = '\0';
				os_printf("cookie:[%s] ", pstr);
        		*pend = '\r';
#endif
        	}
#if DEBUGSOO > 3
           	else os_printf("cookie not crlf! ");
#endif
        };
    };
#ifdef WEBSOCKET_ENA
    if(CheckSCB(SCB_GET) && web_conn->bffiles[0] == WEBFS_WEBCGI_HANDLE) {
    	if(CurHTTP->head_len > sizeHTTPUpgrade +  sizeHTTPwebsocket + 2 + sizeHTTPSecWebSocketKey + sizeWebSocketKey + 2) { // + "\r\n"
        	pstr = head_find_ctr(CurHTTP, HTTPUpgrade, sizeHTTPUpgrade, sizeHTTPwebsocket);
        	if(CurHTTP->httpStatus != 200) return false;
        	if(pstr != NULL) {
            	if(os_memcmp(word_to_lower_case(pstr), HTTPwebsocket, sizeHTTPwebsocket)) {
                    CurHTTP->httpStatus = 400; // 400 Bad Request
                    return false;
            	}
            	pstr = head_find_ctr(CurHTTP, HTTPSecWebSocketKey, sizeHTTPSecWebSocketKey, sizeWebSocketKey);
            	if(pstr == NULL || CurHTTP->httpStatus != 200) return false;
            	{
            		if(WebSocketAcceptKey(CurHTTP->pFilename, pstr)) SetSCB(SCB_WEBSOC);
            	}
        	}
        }
    }
#endif
    return false;
}
/******************************************************************************
 * FunctionName : web_inc_fp
 * Parameters   : fp
*******************************************************************************/
LOCAL void ICACHE_FLASH_ATTR web_inc_fp(WEB_SRV_CONN *web_conn, WEBFS_HANDLE fp)
{
	if(web_conn->bffiles[3] != WEBFS_INVALID_HANDLE) {
	#if DEBUGSOO > 1
		os_printf("cf%d ", web_conn->bffiles[3]);
	#endif
		if(web_conn->bffiles[3] <= WEBFS_MAX_HANDLE) {
			web_conn->content_len -= WEBFSGetBytesRem(web_conn->bffiles[3]);
			WEBFSClose(web_conn->bffiles[3]);
		}
	};
    web_conn->bffiles[3] = web_conn->bffiles[2];
    web_conn->bffiles[2] = web_conn->bffiles[1];
    web_conn->bffiles[1] = web_conn->bffiles[0];
	web_conn->bffiles[0] = fp;
	SetSCB(SCB_FOPEN); // файл открыт
}
/******************************************************************************
 * FunctionName : web_inc_fopen
 * Description  : web include file open
 * Parameters   : struct
 * Returns      : true - open OK
*******************************************************************************/
bool ICACHE_FLASH_ATTR web_inc_fopen(TCP_SERV_CONN *ts_conn, uint8 *cFile)
{
	WEB_SRV_CONN *web_conn = (WEB_SRV_CONN *)ts_conn->linkd;
	if(CheckSCB(SCB_FOPEN) && (!CheckSCB(SCB_FCALBACK))) { // файл уже открыт и он не парсится?
	    return false; // такое не поддерживается в "~inc:filename~"
	};
	WEBFS_HANDLE fp = WEBFSOpen(cFile);
#if DEBUGSOO > 1
	os_printf("of%d[%s] ", fp, cFile);
#endif
	if(fp != WEBFS_INVALID_HANDLE) {
		if(fatCache.flags & WEBFS_FLAG_HASINDEX) SetSCB(SCB_FCALBACK); // файл надо парсить
		web_conn->content_len += WEBFSGetBytesRem(fp); // указать размер файла для вывода
		if(fatCache.flags & WEBFS_FLAG_ISZIPPED) {
			if(CheckSCB(SCB_FOPEN)) { // файл уже открыт и "~inc:filename~" не поддерживает GZIP!
				WEBFSClose(fp);
#if DEBUGSOO > 1
				os_printf("Not inc GZIP! ");
#endif
				return false;
			};
			SetSCB(SCB_FGZIP); // файл сжат GZIP
		}
	}
	else { // File not found
	    return false;
	};
	web_inc_fp(web_conn, fp);
	return true;
};
/******************************************************************************
 * FunctionName : web_inc_file
 * Description  : web include file close
 * Parameters   : struct
 * Returns      : true - все файлы закрыты
*******************************************************************************/
bool ICACHE_FLASH_ATTR web_inc_fclose(WEB_SRV_CONN *web_conn)
{
	if(web_conn->bffiles[0] != WEBFS_INVALID_HANDLE) {
#if DEBUGSOO > 1
		os_printf("cf%d ", web_conn->bffiles[0]);
#endif
		if(web_conn->bffiles[0] <= WEBFS_MAX_HANDLE) {
			WEBFSClose(web_conn->bffiles[0]);
			ClrSCB(SCB_FGZIP);
		}
        web_conn->bffiles[0] = web_conn->bffiles[1];
        web_conn->bffiles[1] = web_conn->bffiles[2];
        web_conn->bffiles[2] = web_conn->bffiles[3];
        web_conn->bffiles[3] = WEBFS_INVALID_HANDLE;
   		if(web_conn->bffiles[0] != WEBFS_INVALID_HANDLE) return false;
	};
	ClrSCB(SCB_FOPEN | SCB_FGZIP | SCB_FCALBACK);
	return true; // больше нет файлов
};
/******************************************************************************
 * FunctionName : webserver_open_file
 * Description  : Compare to known extensions to determine Content-Type
 * Parameters   : filename -- file name
 * Returns      : 1 - open, 0 - no
*******************************************************************************/
LOCAL void ICACHE_FLASH_ATTR webserver_file_ext(HTTP_CONN *CurHTTP, uint8 *pfname)
{
	uint8 *pfext = NULL;
	while(*pfname >= ' ') if(*pfname++ == '.') pfext = pfname;
	if(pfext != NULL) {
		for(CurHTTP->fileType = HTTP_TXT; CurHTTP->fileType < HTTP_UNKNOWN; CurHTTP->fileType++)
			if(rom_xstrcmp(pfext, httpFileExtensions[CurHTTP->fileType]))	break;
	};
}
/*----------------------------------------------------------------------*/
#ifdef USE_CAPTDNS
/* = flase, если включен redirect, и запрос от ip адреса из подсети AP,
 * и Host name не равен aesp8266 или ip AP. */
LOCAL bool ICACHE_FLASH_ATTR web_cdns_no_redir(HTTP_CONN *CurHTTP, TCP_SERV_CONN *ts_conn)
{
	if(syscfg.cfg.b.cdns_ena
	  && pcb_cdns != NULL
	  &&((ts_conn->pcb->remote_ip.addr ^ info.ap_ip) & info.ap_mask) == 0
	  && CurHTTP->phead != NULL
	  && CurHTTP->head_len != 0) {
		uint8 * ps = head_find_ctr(CurHTTP, HTTPHost, sizeHTTPHost, 7);
		if(ps != NULL) {
#if DEBUGSOO > 1
		os_printf("Host: '%s' ", ps);
#endif
		uint8 strip[4*4];
		os_sprintf_fd(strip, IPSTR, IP2STR(&info.ap_ip));
		if((rom_xstrcmp(ps, HostNameLocal) == 0) && (rom_xstrcmp(ps, strip) == 0))   {
				ets_sprintf(CurHTTP->pFilename, httpHostNameLocal, HostNameLocal); // "http://esp8266/"
				WEB_SRV_CONN *web_conn = (WEB_SRV_CONN *)ts_conn->linkd;
				SetSCB(SCB_REDIR);
				return false;
			}
		}
	}
	return true;
}
#endif
/******************************************************************************
 * FunctionName : webserver_open_file
 * Description  : Open file
 * Parameters   : filename -- file name
 * Returns      : 1 - open, 0 - no
*******************************************************************************/
LOCAL bool ICACHE_FLASH_ATTR webserver_open_file(HTTP_CONN *CurHTTP, TCP_SERV_CONN *ts_conn)
{
	WEB_SRV_CONN *web_conn = (WEB_SRV_CONN *)ts_conn->linkd;
	uint8 pbuf[MAX_FILE_NAME_SIZE];
	uint8 *pstr = pbuf;
	if(CurHTTP->pFilename[0] == '/') {
		if(CurHTTP->pFilename[1] == '\0') {
			if(isWEBFSLocked) {
				web_inc_fp(web_conn, WEBFS_NODISK_HANDLE); // желательно дописать ответ, что нет диска.
				web_conn->content_len = sizeHTTPfserror;
				CurHTTP->fileType = HTTP_HTML;
#if DEBUGSOO > 1
				os_printf("of%d[%s] ", web_conn->webfile, CurHTTP->pFilename);
#endif
				return true;
			}
			else {
#ifdef USE_CAPTDNS
				if(web_cdns_no_redir(CurHTTP, ts_conn)) rom_xstrcpy(pstr, http_default_file);
				else return false;
#else
				rom_xstrcpy(pstr, http_default_file);
#endif
			}
		}
		else {
			os_memcpy(pstr, &CurHTTP->pFilename[1], MAX_FILE_NAME_SIZE-1);
			if(rom_xstrcmp(pstr, web_cgi_fname)) {
				web_inc_fp(web_conn, WEBFS_WEBCGI_HANDLE);
				web_conn->content_len = sizeHTTPdefault;
				CurHTTP->fileType = HTTP_HTML;
#if DEBUGSOO > 1
				os_printf("of%d[%s] ", web_conn->webfile, CurHTTP->pFilename);
#endif
				return true;
			}
			else if(rom_xstrcmp(pstr, fsupload_fname)) {
				SetSCB(SCB_AUTH);
				web_inc_fp(web_conn, WEBFS_UPLOAD_HANDLE);
				web_conn->content_len = sizeHTTPfsupload;
				CurHTTP->fileType = HTTP_HTML;
#if DEBUGSOO > 1
				os_printf("of%d[%s] ", web_conn->webfile, CurHTTP->pFilename);
#endif
				return true;
			}
		}
		if(isWEBFSLocked) return false;
		// поиск файла на диске
		if(!web_inc_fopen(ts_conn, pstr)) {
			uint32 i = os_strlen(pbuf);
			if(i + sizeof(http_default_file) < MAX_FILE_NAME_SIZE - 1) {
				// добавить к имени папки "/index.htm"
				pbuf[i] = '/';
				rom_xstrcpy(&pbuf[i+1], http_default_file);
				if(!web_inc_fopen(ts_conn, pstr)) {
#ifdef USE_CAPTDNS
					web_cdns_no_redir(CurHTTP, ts_conn);
#endif
					return false;
				}
			};
		};
		// Compare to known extensions to determine Content-Type
		webserver_file_ext(CurHTTP, pstr);
		return true;
	};
	return false; // файл не открыт
}
/******************************************************************************
*******************************************************************************/
LOCAL void ICACHE_FLASH_ATTR web_send_fnohanle(TCP_SERV_CONN *ts_conn) {
	WEB_SRV_CONN *web_conn = (WEB_SRV_CONN *)ts_conn->linkd;
	uint32 pdata = 0;
	uint8 pbuf[mMAX(mMAX(sizeHTTPdefault,sizeHTTPfserror), sizeHTTPfsupload)];
	uint32 size = 0;
	switch(web_conn->webfile) {
	case WEBFS_WEBCGI_HANDLE:
		pdata = (uint32)((void *)HTTPdefault);
		size = sizeHTTPdefault;
		break;
	case WEBFS_UPLOAD_HANDLE:
		pdata = (uint32)((void *)HTTPfsupload);
		size = sizeHTTPfsupload;
		break;
	case WEBFS_NODISK_HANDLE:
		pdata = (uint32)((void *)HTTPfserror);
		size = sizeHTTPfserror;
		break;
	}
	if(pdata != 0 && size != 0) {
		spi_flash_read(pdata & MASK_ADDR_FLASH_ICACHE_DATA, pbuf, size);
		tcpsrv_int_sent_data(ts_conn, pbuf, size);
	}
#if DEBUGSOO > 1
	os_printf("%u ", size);
#endif
	SetSCB(SCB_FCLOSE|SCB_DISCONNECT);
}
/******************************************************************************
*******************************************************************************/
LOCAL int ICACHE_FLASH_ATTR web_find_cbs(uint8 * chrbuf, uint32 len) {
  int i;
  for(i = 0; i < len; i++)  if(chrbuf[i] == '~')  return i;
  return -1;
}
/******************************************************************************
 * FunctionName : webserver_send_fdata
 * Description  : Sent callback function to call for this espconn when data
 *                is successfully sent
 * Parameters   : arg -- Additional argument to pass to the callback function
 * Returns      : none
*******************************************************************************/
LOCAL void ICACHE_FLASH_ATTR webserver_send_fdata(TCP_SERV_CONN *ts_conn) {
	WEB_SRV_CONN *web_conn = (WEB_SRV_CONN *)ts_conn->linkd;
	if(web_conn->webfile == WEBFS_INVALID_HANDLE) {
		SetSCB(SCB_FCLOSE|SCB_DISCONNECT);
		return;
	}
#if DEBUGSOO > 1
	os_printf("send: ");
#endif
	set_cpu_clk();

	web_conn->msgbufsize = tcp_sndbuf(ts_conn->pcb);

#if DEBUGSOO > 5
	os_printf("sndbuf=%u ", web_conn->msgbufsize);
#endif

	if (web_conn->msgbufsize < MIN_SEND_SIZE) {
#if DEBUGSOO > 1
		os_printf("sndbuf=%u! ", web_conn->msgbufsize);
		if(ts_conn->flag.wait_sent) os_printf("wait_sent! "); // блок передан?
#endif
		ts_conn->pcb->flags &= ~TF_NODELAY;
		tcpsrv_int_sent_data(ts_conn, (uint8 *)ts_conn, 0);
		return;
	}
	if((web_conn->webfile > WEBFS_MAX_HANDLE)&&(!CheckSCB(SCB_RETRYCB)))  {
		web_send_fnohanle(ts_conn);
		return;
	}
	web_conn->msgbufsize = mMIN(MAX_SEND_SIZE, web_conn->msgbufsize);
	uint8 *pbuf = (uint8 *) os_malloc(web_conn->msgbufsize);
	if (pbuf == NULL) {
#if DEBUGSOO > 0
		os_printf("out of memory - disconnect!\n");
#endif
		SetSCB(SCB_FCLOSE|SCB_DISCONNECT);
		return;
	};
	web_conn->msgbuf = pbuf;
	web_conn->msgbuflen = 0;
	if (CheckSCB(SCB_CHUNKED)) { // is chunked
		web_conn->msgbuf += RESCHKS_SEND_SIZE;
		web_conn->msgbufsize -= RESCHK_SEND_SIZE;
	};
	if(CheckSCB(SCB_FCALBACK) == 0) { // передача файла без парсинга
		// Get/put as many bytes as possible
		web_conn->msgbuflen = WEBFSGetArray(web_conn->webfile, web_conn->msgbuf, web_conn->msgbufsize);
		if(web_conn->msgbuflen < web_conn->msgbufsize ) SetSCB(SCB_FCLOSE | SCB_DISCONNECT);
	}
	else { // парсинг потока передачи
		do { // начинаем с пустого буфера
			if(CheckSCB(SCB_RETRYCB)) { // повторный callback? да
#if DEBUGSOO > 2
				os_printf("rcb ");
#endif
				if(web_conn->func_web_cb != NULL) web_conn->func_web_cb(ts_conn);
				if(CheckSCB(SCB_RETRYCB)) break; // повторить ещё раз? да.
			}
			else {
				uint8 *pstr = &web_conn->msgbuf[web_conn->msgbuflen]; // указатель в буфере
				// запомнить указатель в файле. ftell(fp)
				uint32 max = mMIN(web_conn->msgbufsize - web_conn->msgbuflen, SCB_SEND_SIZE); // читаем по 128 байт ?
				uint32 len = WEBFSGetArray(web_conn->webfile, pstr, max);
				// прочитано len байт в буфер по указателю &sendbuf[msgbuflen]
				if(len) { // есть байты для передачи, ищем string "~calback~"
					int cmp = web_find_cbs(pstr, len);
					if(cmp >= 0) { // найден calback
						// откат файла
						WEBFSStubs[web_conn->webfile].addr -= len;
						WEBFSStubs[web_conn->webfile].bytesRem += len;
						// передвинуть указатель в файле на считанные байты с учетом маркера, без добавки длины для передачи
						WEBFSStubs[web_conn->webfile].addr += cmp+1;
						WEBFSStubs[web_conn->webfile].bytesRem -= cmp+1;
						// это второй маркер?
						if(CheckSCB(SCB_FINDCB)) { // в файле найден закрывающий маркер calback
							ClrSCB(SCB_FINDCB); // прочитали string calback-а
							if(cmp != 0) { // это дубль маркера ? нет.
								// запустить calback
								pstr[cmp] = '\0'; // закрыть string calback-а
								if(!os_memcmp((void*)pstr, "inc:", 4)) { // "inc:file_name"
									if(!web_inc_fopen(ts_conn, &pstr[4])) {
										tcp_strcpy_fd("file not found!");
									};
								}
								else web_int_callback(ts_conn, pstr);
							}
							else { // Дубль маркера.
								web_conn->msgbuflen++; // передать только маркер ('~')
							};
						}
						else {
							SetSCB(SCB_FINDCB); // в файле найден стартовый маркер calback
							web_conn->msgbuflen += cmp;  // передать до стартового маркера calback
						};
					}
					else {  // просто данные
						ClrSCB(SCB_FINDCB);
						if(len < max) {
							if(web_inc_fclose(web_conn)) SetSCB(SCB_FCLOSE | SCB_DISCONNECT); // файл(ы) закончилсь совсем? да.
						};
						web_conn->msgbuflen += len; // добавить кол-во считанных байт для передачи.
					};
				}
				else if(web_inc_fclose(web_conn)) SetSCB(SCB_FCLOSE | SCB_DISCONNECT); // файл(ы) закончилсь совсем? да.
			};  // not SCB_RETRYCB
		} // набираем буфер
		while((web_conn->msgbufsize - web_conn->msgbuflen >= SCB_SEND_SIZE)&&(!CheckSCB(SCB_FCLOSE | SCB_RETRYCB | SCB_DISCONNECT)));
	};
#if DEBUGSOO > 3
	os_printf("#%04x %d ", web_conn->webflag, web_conn->msgbuflen);
#elif DEBUGSOO > 1
	os_printf("%u ", web_conn->msgbuflen);
#endif
	if(web_conn->msgbuflen) {
		web_conn->content_len -= web_conn->msgbuflen; // пока только для инфы
		if(CheckSCB(SCB_CHUNKED)) { // greate chunked
			uint8 cbuf[16];
			static const char chunks[] ICACHE_RODATA_ATTR = "\r\n%X\r\n";
			unsigned int len = ets_sprintf(cbuf, chunks, web_conn->msgbuflen);
			web_conn->msgbuf -= len;
			os_memcpy(web_conn->msgbuf, cbuf, len);
			web_conn->msgbuflen += len;
			if(CheckSCB(SCB_FCLOSE)) { // close file? -> add 'end chunked'
				tcp_strcpy_fd("\r\n0\r\n\r\n");
			};
		};
		ts_conn->pcb->flags |= TF_NODELAY;
		tcpsrv_int_sent_data(ts_conn, web_conn->msgbuf, web_conn->msgbuflen);
	};
	os_free(pbuf);
	web_conn->msgbuf = NULL;
}
/******************************************************************************
 * FunctionName : web_print_headers
 * Description  : Print HTTP Response Header
 * Parameters   : *
 * Returns      : none
*******************************************************************************/
LOCAL void ICACHE_FLASH_ATTR
web_print_headers(HTTP_CONN *CurHTTP, TCP_SERV_CONN *ts_conn)
{
	WEB_SRV_CONN *web_conn = (WEB_SRV_CONN *)ts_conn->linkd;
	HTTP_RESPONSE *CurResp = (HTTP_RESPONSE *)HTTPResponse;
#if DEBUGSOO > 3
    os_printf("prh#%04x,%d,%d ", web_conn->webflag, CurHTTP->httpStatus, CurHTTP->fileType);
#endif
    web_conn->msgbuf = (uint8 *)os_malloc(HTTP_SEND_SIZE);
    if(web_conn->msgbuf == NULL)
    {
#if DEBUGSOO == 1
        os_printf("web: out of memory!\n");
#elif DEBUGSOO > 1
        os_printf("out of memory! ");
#endif
    	SetSCB(SCB_FCLOSE | SCB_DISCONNECT);
        return;
    }
    web_conn->msgbufsize = HTTP_SEND_SIZE;
    web_conn->msgbuflen = 0;

    if(CheckSCB(SCB_REDIR)) {
    	CurHTTP->httpStatus = 302; // редирект
    }
#ifdef WEBSOCKET_ENA
    if(CheckSCB(SCB_WEBSOC) && CurHTTP->httpStatus == 200) {
#if DEBUGSOO > 1
    	CurHTTP->httpStatus = 101;
#endif
    	tcp_puts(WebSocketHTTPOkKey, CurHTTP->pFilename);
    }
    else {
#endif
    	while(!(CurResp->flag & HTTP_RESP_FLG_END)) {
          if(CurResp->status == CurHTTP->httpStatus) break;
          CurResp++;
        };
        tcp_puts_fd("HTTP/1.1 %u ", CurResp->status);
        tcp_strcpy(CurResp->headers);
        tcp_strcpy_fd("\r\nServer: " WEB_NAME_VERSION "\r\nConnection: close\r\n");
        if(CheckSCB(SCB_REDIR)) {
        	tcp_puts_fd("Location: %s\r\n\r\n", CurHTTP->pFilename);
        	ts_conn->flag.pcb_time_wait_free = 1; // закрыть соединение
        	SetSCB(SCB_DISCONNECT);
        }
        else {
            if(CurResp->status != 200) {
            	web_inc_fclose(web_conn);
            	ClrSCB(SCB_FCALBACK | SCB_FGZIP | SCB_CHUNKED | SCB_RXDATA | SCB_FCLOSE);
                if(CurResp->flag & HTTP_RESP_FLG_FINDFILE) {
                  os_sprintf_fd(CurHTTP->pFilename, "/%u.htm", CurResp->status);
                  webserver_open_file(CurHTTP, ts_conn);
            //      CurHTTP->httpStatus = CurResp->status; // вернуть статус!
                };
            }
            if((!CheckSCB(SCB_FOPEN)) && (CurResp->default_content != NULL) ) {
                tcp_puts_fd("%s %u\r\n%s %s\r\n\r\n", HTTPContentLength, rom_strlen(CurResp->default_content),
                  HTTPContentType, httpContentTypes[HTTP_TXT]);
                tcp_strcpy(CurResp->default_content);
                SetSCB(SCB_DISCONNECT);
            }
            else if(CheckSCB(SCB_FOPEN)) {
            	if(web_conn->content_len) {
                	// Указать, что данные могут пользовать все (очень актуально для XML, ...)
            		tcp_strcpy_fd("Access-Control-Allow-Origin: *\r\n");
                    if(CurHTTP->fileType != HTTP_UNKNOWN) {
                    	if(web_conn->bffiles[0] == WEBFS_WEBCGI_HANDLE && CheckSCB(SCB_FCALBACK)) CurHTTP->fileType = HTTP_TXT;
                    	tcp_puts_fd("Content-Type: %s\r\n", httpContentTypes[CurHTTP->fileType]);
                    };
                    // Output the cache-control + ContentLength
                    if(CheckSCB(SCB_FCALBACK)) { // длина неизветсна
                    	// file is callback index
                    	tcp_strcpy_fd("Cache-Control: no-store, no-cache, must-revalidate, max-age=0\r\n");
                    	if(CurHTTP->httpver >= 0x11) SetSCB(SCB_CHUNKED);
                    }
                    else { // длина изветсна
                    	tcp_puts_fd("%s %d\r\n", HTTPContentLength, web_conn->content_len);
                    	if(CurResp->status == 200 && (!isWEBFSLocked) && web_conn->bffiles[0] != WEBFS_WEBCGI_HANDLE) {
                    		// lifetime (sec) of static responses as string 60*60*24*14=1209600"
                        	tcp_puts_fd("Cache-Control: smax-age=%d\r\n", FILE_CACHE_MAX_AGE_SEC);
                    	}
                    	else {
                    		tcp_strcpy_fd("Cache-Control: no-store, no-cache, must-revalidate, max-age=0\r\n");
                    	}
                    };
                    if(CheckSCB(SCB_FGZIP)) {
                    	// Output the gzip encoding header if needed
                    	tcp_strcpy_fd("Content-Encoding: gzip\r\n");
                    }
                    else if(CheckSCB(SCB_CHUNKED)) {
                    	tcp_strcpy_fd("Transfer-Encoding: chunked\r\n");
                    }
                    if(!CheckSCB(SCB_CHUNKED)) tcp_strcpy_fd(CRLF);
                }
                else {
                	tcp_puts_fd("%s 0\r\n\r\n", HTTPContentLength);
                	SetSCB(SCB_FCLOSE|SCB_DISCONNECT);
                }
            }
            else SetSCB(SCB_DISCONNECT);
        } // CheckSCB(SCB_REDIR)
#ifdef WEBSOCKET_ENA
    }
#endif
#if DEBUGSOO > 3
    os_printf("#%04x (%d) %d ", web_conn->webflag, web_conn->msgbuflen, CurHTTP->httpStatus);
#elif DEBUGSOO > 1
    os_printf("head[%d]:%d ", web_conn->msgbuflen, CurHTTP->httpStatus);
#endif
    if(web_conn->msgbuflen) {
        if(CheckSCB(SCB_DISCONNECT)) SetSCB(SCB_CLOSED);
        tcpsrv_int_sent_data(ts_conn, web_conn->msgbuf, web_conn->msgbuflen);
#ifdef USE_WEB_NAGLE
        ts_conn->flag.nagle_disabled = 1;
#endif
    };
    os_free(web_conn->msgbuf);
    web_conn->msgbuf = NULL;
}
/******************************************************************************/
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// поиск boundary
// 0 - разделитель (boundary) не найден, докачивать или ...
// 1 - boundary найден
// 200 - найден завершаюший boundary
// 400 - неверный формат
// ...
//-----------------------------------------------------------------------------
/* Пример M-Explorer: Load blk len: 399
-----------------------------7df22f37711be\r\n
Content-Disposition: form-data; name="test"; filename="readme.txt"\r\n
Content-Type: text/plain\r\n\r\n
1234567890\r\n
-----------------------------7df22f37711be\r\n
Content-Disposition: form-data; name="start"\r\n\r\n
0x1B000\r\n
-----------------------------7df22f37711be\r\n
Content-Disposition: form-data; name="stop"\r\n\r\n
0x1B000\r\n
-----------------------------7df22f37711be--\r\n */
/* Пример Google Chrome: Load blk len: 391
------WebKitFormBoundaryugGNBVFOk6qxfe22\r\n
Content-Disposition: form-data; name="test"; filename="readme.txt"\r\n
Content-Type: text/plain\r\n\r\n
1234567890\r\n
------WebKitFormBoundaryugGNBVFOk6qxfe22\r\n
Content-Disposition: form-data; name="start"\r\n\r\n
0x1B000\r\n
------WebKitFormBoundaryugGNBVFOk6qxfe22\r\n
Content-Disposition: form-data; name="stop"\r\n\r\n
0x1B000\r\n
------WebKitFormBoundaryugGNBVFOk6qxfe22--\r\n */
//-----------------------------------------------------------------------------
const char crlf_end_boundary[] ICACHE_RODATA_ATTR = "--" CRLF;
LOCAL int ICACHE_FLASH_ATTR find_boundary(HTTP_UPLOAD *pupload, uint8 *pstr, uint32 len)
{
	int x = len - 6 - pupload->sizeboundary;
	if(x <= 0) return 0; // разделитель (boundary) не найден - докачивать буфер
	int i;
	uint8 *pcmp;
	for(i = 0; i <= x; i++) {
		if(pstr[i] == '-' && pstr[i+1] == '-') {
			pcmp = pstr + i;
//			if((pstr + len - pcmp) < pupload->sizeboundary + 6) return 0; // разделитель (boundary) не найден - докачивать буфер
			pupload->pbndr = pcmp; // указатель на заголовок boundary (конец блока данных);
			pcmp += 2;
			if(os_memcmp(pcmp, pupload->boundary, pupload->sizeboundary)) return 0; // разделитель (boundary) не найден
			pcmp += pupload->sizeboundary;
			if(rom_xstrcmp(pcmp, crlf_end_boundary)) {
				pcmp += 4;
				pupload->pnext = pcmp; // указатель в заголовке boundary (описание новых данных);
				return 200; // найден завершающий разделитель
			}
			if(pcmp[0] != '\r' || pcmp[1] != '\n') return 400; // неверный формат
			pcmp += 2;
			pupload->pnext = pcmp; // указатель в заголовке boundary (описание новых данных);
			return 1;
		};
	};
	return 0; // разделитель (boundary) не найден - докачивать буфер
}
//-----------------------------------------------------------------------------
// Function: cmp_next_boundary
// return:
// 0 - разделитель (boundary) не найден, докачивать
// 1 - далее обработка данных
// 200 - найден завершающий разделитель: "\r\n--boundary--"
// 400 - неизвестный формат content-а
//-----------------------------------------------------------------------------
const char disk_ok_filename[] ICACHE_RODATA_ATTR = "/disk_ok.htm";
const char disk_err1_filename[] ICACHE_RODATA_ATTR = "/disk_er1.htm";
const char disk_err2_filename[] ICACHE_RODATA_ATTR = "/disk_er2.htm";
const char disk_err3_filename[] ICACHE_RODATA_ATTR = "/disk_er3.htm";
const char sysconst_filename[] ICACHE_RODATA_ATTR = "sysconst";
#ifdef USE_OVERLAY
const char overlay_filename[] ICACHE_RODATA_ATTR = "overlay";
#endif
const char sector_filename[] ICACHE_RODATA_ATTR = "fsec_";
#define sector_filename_size 5
const char file_label[] ICACHE_RODATA_ATTR = "file";

LOCAL int ICACHE_FLASH_ATTR upload_boundary(TCP_SERV_CONN *ts_conn) // HTTP_UPLOAD pupload, uint8 pstr, uint16 len)
{
	HTTP_UPLOAD *pupload = (HTTP_UPLOAD *)ts_conn->pbufo;
	WEB_SRV_CONN *web_conn = (WEB_SRV_CONN *)ts_conn->linkd;
	if(pupload == NULL) return 500; // ошибка сервера
	uint32 ret;
	uint32 len;
	uint8 *pnext;
	uint8 *pstr;
	while(web_conn->content_len && ts_conn->pbufi != NULL) {
		pstr = ts_conn->pbufi;
		len = ts_conn->sizei;
#if DEBUGSOO > 4
		os_printf("bufi[%u]%u, cont:%u ", ts_conn->sizei, ts_conn->cntri, web_conn->content_len);
#endif
		if(len < (8 + pupload->sizeboundary)) return 0; // разделитель (boundary) не влезет - докачивать буфер
		switch(pupload->status) {
			case 0: // поиск boundary
			{
#if DEBUGSOO > 4
				os_printf("find_bndr ");
#endif
				pnext = web_strnstr(pstr, CRLF CRLF , len);
				if(pnext == NULL) return 0; // докачивать
				len = pnext - pstr;
				ret = find_boundary(pupload, pstr, len);
#if DEBUGSOO > 4
				os_printf("len=%u,ret=%u ", len, ret );
#endif
				if(ret != 1) return ret;
				pstr = pupload->pnext; // +"\r\n" адрес за заголовком boundary
				pupload->name[0] = '\0';
				pupload->filename[0] = '\0';
				pstr = web_strnstr(pstr, "name=", pnext - pstr);
				if(pstr == NULL) return 400; // неизвестный формат content-а
				pstr += 5;
				if(pstr >= pnext) return 400; // неизвестный формат content-а
				uint8 *pcmp = cmpcpystr(pupload->name, pstr, '"', '"', VarNameSize);
				if(pcmp == NULL) {
				 	pcmp = cmpcpystr(pupload->name, pstr, 0x22, 0x22, VarNameSize);
				 	if(pcmp == NULL) return 400; // неизвестный формат content-а
				};
				pstr = pcmp;
#if DEBUGSOO > 4
				os_printf("name:'%s' ", pupload->name);
#endif
				if(pstr >= pnext) return 400; // неизвестный формат content-а
				pcmp = web_strnstr(pstr, "filename=", pnext - pstr);
				if(pcmp != NULL) {
					pcmp += 9;
					if(pcmp < pnext) {
						if(cmpcpystr(pupload->filename, pcmp, '"', '"', VarNameSize) == NULL)
							cmpcpystr(pupload->filename, pcmp, 0x22, 0x22, VarNameSize);
					};
#if DEBUGSOO > 1
					if(pupload->filename[0]!= '\0') os_printf("filename:'%s' ", pupload->filename);
#endif
				};
				len += 4;
				pupload->status++;
#if DEBUGSOO > 4
					os_printf("trim#%u\n", len );
#endif
				ts_conn->cntri += len; // далее идут данные
				if(!web_trim_bufi(ts_conn, &ts_conn->pbufi[len], ts_conn->sizei - len)) return 500;
				web_conn->content_len -= len;
				break;
			}
			case 1: // прием данных, первый заход, проверка форматов и т.д.
			{
#if DEBUGSOO > 4
				os_printf("tst,fn='%s' ", pupload->filename);
#endif
				if(pupload->filename[0]!='\0') { // загрузка файла?
					if(rom_xstrcmp(pupload->name, file_label)) { // !os_memcmp((void*)pupload->name, "file", 4)
						if(len < sizeof(WEBFS_DISK_HEADER)) return 0; // докачивать
						WEBFS_DISK_HEADER *dhead = (WEBFS_DISK_HEADER *)pstr;
						if(dhead->id != WEBFS_DISK_ID || dhead->ver != WEBFS_DISK_VER
								|| (web_conn->content_len - pupload->sizeboundary - 8 < dhead->disksize)) {
							if(isWEBFSLocked) return 400;
							SetSCB(SCB_REDIR);
							rom_xstrcpy(pupload->filename, disk_err1_filename); // os_memcpy(pupload->filename,"/disk_er1.htm\0",14); // неверный формат
							return 200;
						};
						if(dhead->disksize > WEBFS_max_size()) {
							if(isWEBFSLocked) return 400;
							SetSCB(SCB_REDIR);
							rom_xstrcpy(pupload->filename, disk_err2_filename); // os_memcpy(pupload->filename,"/disk_er2.htm\0",14); // не влезет
							return 200;
						};
						pupload->fsize = dhead->disksize;
						pupload->faddr = WEBFS_base_addr();
#if DEBUGSOO > 4
						os_printf("updisk[%u]=ok ", dhead->disksize);
#endif
						pupload->status = 3; // = 3 загрузка WebFileSystem во flash
						isWEBFSLocked = true;
						break;
					}
#ifdef USE_OVERLAY
					else if(rom_xstrcmp(pupload->name, overlay_filename)) {
						if(len < sizeof(struct SPIFlashHeader)) return 0; // докачивать
						struct SPIFlashHeader *fhead = (struct SPIFlashHeader *)pstr;
						if(web_conn->content_len - pupload->sizeboundary < sizeof(fhead)
						|| fhead->head.id != LOADER_HEAD_ID) {
							if(isWEBFSLocked) return 400;
							SetSCB(SCB_REDIR);
							rom_xstrcpy(pupload->filename, disk_err1_filename); // os_memcpy(pupload->filename,"/disk_er1.htm\0",14); // неверный формат
							return 200;
						};
						if(fhead->entry_point >= IRAM_BASE && ovl_call != NULL) {
							ovl_call(0); // close прошлый оверлей
							ovl_call = NULL;
						}
						pupload->start = fhead->entry_point;
						pupload->segs = fhead->head.number_segs;
						if(pupload->segs) {
							pupload->fsize = sizeof(struct SPIFlashHeadSegment);
							pupload->status = 5; // = 5 загрузка файла оверлея, начать с загрузки заголовка сегмента
						}
						else {
							pupload->fsize = 0;
							pupload->status = 4; // = 4 загрузка файла оверлея, запуск entry_point
						}
						//
						len = sizeof(struct SPIFlashHeader);
						ts_conn->cntri += len;
						if(!web_trim_bufi(ts_conn, &ts_conn->pbufi[len], ts_conn->sizei - len)) return 500;
						web_conn->content_len -= len;
						//
						break;
					}
#endif
					else if(rom_xstrcmp(pupload->name, sysconst_filename)) {
						pupload->fsize = SIZE_SYS_CONST;
						pupload->faddr = esp_init_data_default_addr;
						pupload->status = 2; // = 2 загрузка файла во flash
						break;
					}
					else if(rom_xstrcmp(pupload->name, sector_filename)) {
						pupload->fsize = SPI_FLASH_SEC_SIZE;
						pupload->faddr = ahextoul(&pupload->name[sector_filename_size]) << 12;
						pupload->status = 2; // = 2 загрузка файла сектора во flash
						break;
					};
					if(isWEBFSLocked) return 400;
					SetSCB(SCB_REDIR);
					rom_xstrcpy(pupload->filename, disk_err3_filename); // os_memcpy(pupload->filename,"/disk_er3.htm\0",14); // неизвестный тип
					return 200;
				}
				else {
					uint8 *pcmp = web_strnstr(pstr, CRLF, len);
					if(pcmp == NULL) return 0; // докачивать
					ret = find_boundary(pupload, pstr, len);
#if DEBUGSOO > 4
					os_printf("ret=%u ", ret );
#endif
					if((ret != 1 && ret != 200)) { // не найден конец или новый boundary?
						return ret; // догружать
					}
					*pcmp = '\0';
					web_int_vars(ts_conn, pupload->name, pstr);
					if(ret == 200) return ret;
					// найден следующий boundary
					len = pupload->pbndr - ts_conn->pbufi;
					pupload->status = 0; // = 0 найден следующий boundary
#if DEBUGSOO > 4
					os_printf("trim#%u\n", len );
#endif
					ts_conn->cntri += len; // далее идут данные
					if(!web_trim_bufi(ts_conn, &ts_conn->pbufi[len], ts_conn->sizei - len)) return 500;
					web_conn->content_len -= len;
					break;
				}
//				return 400;
			}
//			default:
			case 2: // загрузка файла во flash
			case 3: // загрузка WebFileSystem во flash (скорость записи W25Q128 ~175 килобайт в сек, полный диск на 15,5МБ пишется 90..100 сек )
			{
#if DEBUGSOO > 4
				os_printf("fdata ");
#endif
				uint32 block_size = mMIN(max_len_buf_write_flash + 8 + pupload->sizeboundary, web_conn->content_len);
				if(ts_conn->sizei < block_size) return 0; // докачивать
				ret = find_boundary(pupload, pstr, block_size);
#if DEBUGSOO > 4
				os_printf("ret=%u ", ret);
#endif
				if((ret == 1 || ret == 200)) { // найден конец или новый boundary?
					len = mMIN(block_size, pupload->pbndr - 2 - ts_conn->pbufi);
				}
				else {
					len = mMIN(max_len_buf_write_flash, web_conn->content_len - 8 - pupload->sizeboundary);
				}
#if DEBUGSOO > 4
				os_printf("\nlen=%d, block_size=%d, content_len=%d, sizeboundary= %d, ret=%d, data = %d, load=%d", len, block_size, web_conn->content_len, pupload->sizeboundary, ret, pupload->pbndr - ts_conn->pbufi, ts_conn->sizei);
#endif
				if(pupload->fsize < len) block_size = pupload->fsize;
				else block_size = len;
#if (DEF_SDK_VERSION >= 1000) && (DEF_SDK_VERSION <= 1012)
#define SET_CPU_CLK_SPEED 1
				if(syscfg.cfg.b.hi_speed_enable) {
					ets_intr_lock();
					REG_CLR_BIT(0x3ff00014, BIT(0));
					os_update_cpu_frequency(80);
					ets_intr_unlock();
				}
#endif
				if(block_size) { // идут данные файла
//					tcpsrv_unrecved_win(ts_conn); // для ускорения, пока стрирается-пишется уже обновит окно (включено в web_rx_buf)
					if(pupload->faddr >= FLASH_MIN_SIZE && pupload->status == 3) {
						if((pupload->faddr & 0x0000FFFF)==0) {

#if DEBUGSOO > 2
							os_printf("Clear flash page addr %p... ", pupload->faddr);
#endif
							spi_flash_erase_block(pupload->faddr>>16);
						}
					}
					else if((pupload->faddr & 0x00000FFF) == 0) {
#if DEBUGSOO > 2
						os_printf("Clear flash sector addr %p... ", pupload->faddr);
#endif
						spi_flash_erase_sector(pupload->faddr>>12);
					}
#if DEBUGSOO > 2
					os_printf("Write flash addr:%p[0x%04x]\n", pupload->faddr, block_size);
#endif
					spi_flash_write(pupload->faddr, (uint32 *)pstr, (block_size + 3)&(~3));

					pupload->fsize -= block_size;
					pupload->faddr += block_size;
				}
#if DEBUGSOO > 4
				os_printf("trim#%u\n", len);
#endif
				if(len) {
					ts_conn->cntri += len;
					if(!web_trim_bufi(ts_conn, &ts_conn->pbufi[len], ts_conn->sizei - len)) return 500;
					web_conn->content_len -= len;
				}
#ifdef SET_CPU_CLK_SPEED
				if(syscfg.cfg.b.hi_speed_enable) set_cpu_clk();
#endif
				if((ret == 1 || ret == 200)) { // найден конец или новый boundary?
					if(pupload->status == 3) WEBFSInit();
					if(pupload->fsize != 0) {
						if(!isWEBFSLocked) {
							SetSCB(SCB_REDIR);
							rom_xstrcpy(pupload->filename, disk_err1_filename); // os_memcpy(pupload->filename,"/disk_er1.htm\0",14); // не всё передано или неверный формат
							return 200;
						}
						return 400; //  не всё передано или неверный формат
					}
					else {
						if(!isWEBFSLocked) {
							SetSCB(SCB_REDIR);
							rom_xstrcpy(pupload->filename, disk_ok_filename); // os_memcpy(pupload->filename,"/disk_ok.htm\0",13);
						};
					};
					if(ret == 1) pupload->status = 0; // = 0 найден следующий boundary
					if(ret == 200)	return ret;
				}
				break;
			}
#ifdef USE_OVERLAY
			case 4: // загрузка данных/кода оверлея
			case 5: // загрузка заголовка данных оверлея
			{
				uint32 block_size = mMIN(max_len_buf_write_flash + 8 + pupload->sizeboundary, web_conn->content_len);
				if(ts_conn->sizei < block_size) return 0; // докачивать
				ret = find_boundary(pupload, pstr, block_size);
				if((ret == 1 || ret == 200)) { // найден конец или новый boundary?
					len = mMIN(block_size, pupload->pbndr - 2 - ts_conn->pbufi);
				}
				else {
					len = mMIN(max_len_buf_write_flash, web_conn->content_len - 8 - pupload->sizeboundary);
				}
				block_size = len;
				while(block_size) {
#if DEBUGSOO > 5
					os_printf("blk:%d,st:%d,fs:%d,%d  ", block_size, pupload->status, pupload->fsize, pupload->segs);
#endif
					if(pupload->status == 5) {
						if(block_size >= sizeof(struct SPIFlashHeadSegment)) { // размер данных
							if(pupload->segs) { //
								os_memcpy(&pupload->faddr, pstr, 4);
								os_memcpy(&pupload->fsize, &pstr[4], 4);
#if DEBUGSOO > 4
								os_printf("New seg ovl addr:%p[%p] ", pupload->faddr, pupload->fsize);
#endif
							}
						}
						else if(ret != 1 && ret != 200) { // не найден конец или boundary?
							return 0; // докачивать
						}
						else {
#if DEBUGSOO > 5
							os_printf("err_load_fseg ");
#endif
//						if(block_size < sizeof(struct SPIFlashHeadSegment)
//						|| pupload->segs == 0 //
//						|| pupload->fsize > USE_OVERLAY) {
							if(!isWEBFSLocked) {
								SetSCB(SCB_REDIR);
								rom_xstrcpy(pupload->filename, disk_err1_filename); // os_memcpy(pupload->filename,"/disk_er1.htm\0",14); // не всё передано или неверный формат
								return 200;
							}
							return 400; //  не всё передано или неверный формат
						}
						pupload->segs--; // счет сегментов
						pupload->status = 4; // загрузка данных/кода оверлея
						pstr += sizeof(struct SPIFlashHeadSegment);
						block_size -= sizeof(struct SPIFlashHeadSegment);
					};
					uint32 i = mMIN(pupload->fsize, block_size);
					if(i) {
#if DEBUGSOO > 1
						os_printf("Wr:%p[%p] ", pupload->faddr, i);
#endif
						copy_s1d4((void *)pupload->faddr, pstr, i);
						block_size -= i;
						pupload->faddr += i;
						pstr += i;
						pupload->fsize -= i;
					};
					if(pupload->fsize == 0) {
						if(pupload->segs) { // все сегменты загружены?
							pupload->status = 5; // загрузка заголовка данных оверлея
						}
						else { // все сегменты загружены
							block_size = 0;
							break; // break while(block_size)
						}
					};
				}; // while(block_size)
				if(len) {
					ts_conn->cntri += len;
					if(!web_trim_bufi(ts_conn, &ts_conn->pbufi[len], ts_conn->sizei - len)) return 500;
					web_conn->content_len -= len;
				};
				if((ret == 1 || ret == 200)) { // найден конец или новый boundary?
#if DEBUGSOO > 5
					os_printf("fs:%d,%d ", pupload->fsize, pupload->segs);
#endif
					if(pupload->fsize != 0 || pupload->segs != 0) { //
						if(!isWEBFSLocked) {
							SetSCB(SCB_REDIR);
							rom_xstrcpy(pupload->filename, disk_err1_filename); // os_memcpy(pupload->filename,"/disk_er1.htm\0",14); // не всё передано или неверный формат
							return 200;
						}
						return 400; //  не всё передано или неверный формат
					}
					else {
#if DEBUGSOO > 1
						os_printf("Run%p ", pupload->start);
#endif
						if(pupload->start >= IRAM_BASE) {
							ovl_call = (tovl_call *)pupload->start;
							web_conn->web_disc_cb = (web_func_disc_cb)pupload->start; // адрес старта оверлея
							web_conn->web_disc_par = 1; // параметр функции - инициализация
						}
						if(!isWEBFSLocked) {
							SetSCB(SCB_REDIR);
							rom_xstrcpy(pupload->filename, disk_ok_filename); // os_memcpy(pupload->filename,"/disk_ok.htm\0",13);
						};
					};
					if(ret == 1) pupload->status = 0; // = 0 найден следующий boundary
					if(ret == 200)	return ret;
				};
				break;
			};
#endif
		};
	};
	return 0; //
}
//-----------------------------------------------------------------------------
// web_rx_buf
//
//-----------------------------------------------------------------------------
LOCAL bool ICACHE_FLASH_ATTR web_rx_buf(HTTP_CONN *CurHTTP, TCP_SERV_CONN *ts_conn)
{
	WEB_SRV_CONN *web_conn = (WEB_SRV_CONN *)ts_conn->linkd;
	ts_conn->flag.rx_buf = 1; // указать, что всегда в режиме докачивать
//	CurHTTP->fileType = HTTP_UNKNOWN;
//	ts_conn->pbufi, ts_conn->cntri;
#if DEBUGSOO > 3
	os_printf("rx:%u[%u] ", web_conn->content_len, ts_conn->sizei);
#endif
	if(ts_conn->sizei == 0) return true; // докачивать
	tcpsrv_unrecved_win(ts_conn);
	int ret = upload_boundary(ts_conn);
	if(ret > 1) {
		CurHTTP->httpStatus = ret;
		web_conn->content_len = 0;
		if(ret == 200) {
			if(CheckSCB(SCB_REDIR)) {
				HTTP_UPLOAD *pupload = (HTTP_UPLOAD *)ts_conn->pbufo;
				if(pupload != NULL) {
					os_memcpy(CurHTTP->pFilename, pupload->filename, VarNameSize);
//					SetSCB(SCB_DISCONNECT);
				}
			}
			else if((!isWEBFSLocked) && CheckSCB(SCB_FOPEN)
					&& web_conn->webfile <= WEBFS_MAX_HANDLE
					&& WEBFSGetFilename(web_conn->webfile, CurHTTP->pFilename, FileNameSize)) {
					SetSCB(SCB_REDIR);
//					web_conn->content_len = WEBFSGetBytesRem(web_conn->webfile); // WEBFSGetSize(web_conn->webfile);
//					webserver_file_ext(CurHTTP, CurHTTP->pFilename);
//					return false; // ok 200 + file
			}
		}
		SetSCB(SCB_DISCONNECT);
		return false; // неизвестный content или end
	}
	else {
#if DEBUGSOO > 2
		os_printf("no boundary ");
#endif
		if(ts_conn->sizei > MAX_NO_DATA_BUF_SIZE) {
			CurHTTP->httpStatus = 418; // 418: Out of Coffee
			SetSCB(SCB_DISCONNECT);
			return false; // неизвестный content или end
		}
	};
	if(web_conn->content_len > ts_conn->cntri) return true; // докачивать
	CurHTTP->httpStatus = 400;
	SetSCB(SCB_DISCONNECT);
	web_conn->content_len = 0;
	return false; // неизвестный content
}
//-----------------------------------------------------------------------------
//--- web_trim_bufi -----------------------------------------------------------
//-----------------------------------------------------------------------------
bool ICACHE_FLASH_ATTR web_trim_bufi(TCP_SERV_CONN *ts_conn, uint8 *pdata, uint32 data_len)
{
    if(pdata != NULL && data_len != 0 && ts_conn->sizei > data_len) {
        	os_memcpy(ts_conn->pbufi, pdata, data_len); // переместим кусок в начало буфера
        	ts_conn->pbufi = (uint8 *)mem_realloc(ts_conn->pbufi, data_len + 1); // mem_trim(ts_conn->pbufi, data_len + 1);
        	if(ts_conn->pbufi != NULL) {
            	ts_conn->sizei = data_len; // размер куска
            	ts_conn->cntri = 0;
        	}
        	else return false; // CurHTTP.httpStatus = 500; // 500 Internal Server Error
    }
    else if(ts_conn->pbufi != NULL) {
		os_free(ts_conn->pbufi);
		ts_conn->pbufi = NULL;
		ts_conn->sizei = 0;
		ts_conn->cntri = 0;
	};
    return true;
}
/******************************************************************************
 * web_feee_bufi
 *  освободить приемный буфер
*******************************************************************************/
bool ICACHE_FLASH_ATTR web_feee_bufi(TCP_SERV_CONN *ts_conn)
{
	if(ts_conn->pbufi != NULL) {
		os_free(ts_conn->pbufi);
		ts_conn->pbufi = NULL;
		ts_conn->sizei = 0;
		ts_conn->cntri = 0;
		return true;
	}
	return false;
}
/******************************************************************************
 * FunctionName : webserver_recv
 * Description  : Processing the received data from the server
 * Parameters   : arg -- Additional argument to pass to the callback function
 *                pusrdata -- The received data (or NULL when the connection has been closed!)
 *                length -- The length of received data
 * Returns      : none
 *
 * For HTTP 1.0, this should normally only happen once (if the request fits in one packet).
 *
*******************************************************************************/
LOCAL err_t ICACHE_FLASH_ATTR webserver_received_data(TCP_SERV_CONN *ts_conn)
{
#if DEBUGSOO > 1
    tcpsrv_print_remote_info(ts_conn);
    os_printf("read: %d ", ts_conn->sizei);
#endif
    HTTP_CONN CurHTTP;     // Current HTTP connection state
    WEB_SRV_CONN *web_conn = ReNew_web_conn(ts_conn);
    if(web_conn == NULL) {
#if DEBUGSOO > 1
        os_printf("err mem!\n");
#endif
    	return ERR_MEM;
    }
    if(CheckSCB(SCB_CLOSED | SCB_DISCONNECT | SCB_FCLOSE )) // обрабатывать нечего
    	return ERR_OK;
    if(!CheckSCB(SCB_WEBSOC)) {
    	web_conn->udata_start = 0;
    	web_conn->udata_stop = 0;
    }
    os_memset(&CurHTTP, 0, sizeof(CurHTTP));
    CurHTTP.httpStatus = 200; // OK
    CurHTTP.fileType = HTTP_UNKNOWN;
    // прием и обработка заголовка HHTP
    if(!CheckSCB(SCB_HEAD_OK)) { // заголовок уже принят и обработан? нет
		ts_conn->flag.rx_buf = 1; // докачивать буфер
		tcpsrv_unrecved_win(ts_conn);
		// разбираем докачан или нет заголовок HTTP, что там принято GET или POST и открываем файл и прием content, если это POST и не прием файла.
    	if(parse_header(&CurHTTP, ts_conn)) { // заголовок полный? нет
    		if(ts_conn->sizei < MAX_HTTP_HEAD_BUF) {
#if DEBUGSOO > 4
				os_printf("buf");
#endif
#if DEBUGSOO > 1
				os_printf("...\n");
#endif
				return ERR_OK; // будем принимать ещё.
    		};
    	   	CurHTTP.httpStatus = 413; // 413 Request Entity Too Large // пока так
		};
    	// разбор заголовка
#if DEBUGSOO > 1
#ifdef WEBSOCKET_ENA
		os_printf("%s f[%s] ", (CheckSCB(SCB_POST))? "POST" : (CheckSCB(SCB_WEBSOC))? "WEBSOC" : "GET", CurHTTP.pFilename);
#else
		os_printf("%s f[%s] ", (CheckSCB(SCB_POST))? "POST" : "GET", CurHTTP.pFilename);
#endif
#endif
#if DEBUGSOO > 3
		os_printf("hcn:%p[%d],wcn:%d ", CurHTTP.pcontent, CurHTTP.content_len, web_conn->content_len);
#endif
	    if(CurHTTP.httpStatus == 200) { // && CheckSCB(SCB_FOPEN)) { // если файл открыт и всё OK
			if(CurHTTP.cookie_len != 0) web_parse_cookie(&CurHTTP, ts_conn);
			web_parse_uri_vars(&CurHTTP, ts_conn);
			if(CurHTTP.pcontent != NULL) {
				if(CheckSCB(SCB_RXDATA)) {
					if(web_conn->content_len) { // с заголовком приняли кусок данных файла?
#if DEBUGSOO > 3
						os_printf("trim:%u[%u] ", web_conn->content_len, CurHTTP.content_len);
#endif
						if(!web_trim_bufi(ts_conn, CurHTTP.pcontent, CurHTTP.content_len)) {
#if DEBUGSOO > 1
							os_printf("trim error!\n");
#endif
							CurHTTP.httpStatus = 500;
						};
					};
				}
				else {
					if(CurHTTP.content_len != 0) web_parse_content(&CurHTTP, ts_conn);
				};
			};
		};
    	SetSCB(SCB_HEAD_OK); // заголовок принят и обработан
	};
#if DEBUGSOO > 3
   	os_printf("tst_rx: %u, %u, %u ", CurHTTP.httpStatus, (CheckSCB(SCB_RXDATA) != 0), web_conn->content_len );
#endif
   	// проверка на прием данных (content)
    if(CurHTTP.httpStatus == 200 && CheckSCB(SCB_RXDATA) && (web_conn->content_len) && web_rx_buf(&CurHTTP,ts_conn)) {
#if DEBUGSOO > 1
    	os_printf("...\n");
#endif
    	return ERR_OK; // докачивать content
    };
#ifdef WEBSOCKET_ENA
	if(CheckSCB(SCB_WEBSOC) && CurHTTP.httpStatus == 200 && (!CheckSCB(SCB_REDIR))) {
		if(!CheckSCB(SCB_WSDATA)) {
			// создание и вывод заголовка ответа websock
			ClrSCB(SCB_RXDATA);
			Close_web_conn(ts_conn); // закрыть все файлы
			web_print_headers(&CurHTTP, ts_conn);
			if(CheckSCB(SCB_DISCONNECT)) {
			    ts_conn->flag.rx_null = 1; // всё - больше не принимаем!
				ts_conn->flag.rx_buf = 0; // не докачивать буфер
				if(web_feee_bufi(ts_conn)) tcpsrv_unrecved_win(ts_conn); // уничтожим буфер
			}
			else {
				SetSCB(SCB_WSDATA);
				ts_conn->flag.rx_buf = 1; // указать, что всегда в режиме докачивать
				tcpsrv_unrecved_win(ts_conn);
				tcp_output(ts_conn->pcb);

				if(web_feee_bufi(ts_conn)) tcpsrv_unrecved_win(ts_conn); // уничтожим буфер
/*
				if(ts_conn->pbufi != NULL && ts_conn->sizei != 0) { // что-то ещё есть в буфере?
#if DEBUGSOO > 1
					os_printf("ws_rx[%u]? ", ts_conn->sizei);
#endif
					websock_rx_data(ts_conn);
				}
*/
			}
		}
		else {
			websock_rx_data(ts_conn);
		}
	}
	else
#endif
	{
	    ts_conn->flag.rx_null = 1; // всё - больше не принимаем!
		ts_conn->flag.rx_buf = 0; // не докачивать буфер
		if(web_feee_bufi(ts_conn)) tcpsrv_unrecved_win(ts_conn); // уничтожим буфер
	    if(tcp_sndbuf(ts_conn->pcb) >= HTTP_SEND_SIZE) { // возможна втавка ответа?
			// создание и вывод заголовка ответа.
			web_print_headers(&CurHTTP, ts_conn);
	        // начало предачи файла, если есть
	        if((!CheckSCB(SCB_CLOSED | SCB_DISCONNECT | SCB_FCLOSE))&&CheckSCB(SCB_FOPEN)) webserver_send_fdata(ts_conn);
	    }
	    else {
#if DEBUGSOO > 1
	    	os_printf("sndbuf=%u! ", tcp_sndbuf(ts_conn->pcb));
#endif
	    	SetSCB(SCB_FCLOSE | SCB_DISCONNECT);
	    };
	}
    if(CheckSCB(SCB_FCLOSE))  {
        tcp_output(ts_conn->pcb);
    	Close_web_conn(ts_conn);
    	SetSCB(SCB_DISCONNECT);
    }
    if(CheckSCB(SCB_DISCONNECT)) web_int_disconnect(ts_conn);
#if DEBUGSOO > 1
    else  os_printf("...\n");
#endif
    return ERR_OK;
}
/******************************************************************************
 * web_int_disconnect
*******************************************************************************/
LOCAL void ICACHE_FLASH_ATTR web_int_disconnect(TCP_SERV_CONN *ts_conn)
{
#if DEBUGSOO > 1
    os_printf("dis\n");
#endif
	WEB_SRV_CONN *web_conn = (WEB_SRV_CONN *)ts_conn->linkd;
	ts_conn->flag.tx_null = 1;
	ts_conn->flag.rx_null = 1;
	tcpsrv_unrecved_win(ts_conn);
	if(ts_conn->flag.pcb_time_wait_free) tcpsrv_disconnect(ts_conn);
	SetSCB(SCB_CLOSED);
}
/******************************************************************************
 * FunctionName : webserver_sent_cb
 * Description  : Sent callback function to call for this espconn when data
 *                is successfully sent
 * Parameters   : arg -- Additional argument to pass to the callback function
 * Returns      : none
*******************************************************************************/
LOCAL err_t ICACHE_FLASH_ATTR webserver_sent_callback(TCP_SERV_CONN *ts_conn)
{
#if DEBUGSOO > 1
    tcpsrv_print_remote_info(ts_conn);
#endif
	WEB_SRV_CONN *web_conn = (WEB_SRV_CONN *)ts_conn->linkd;
	if(web_conn == NULL) return ERR_ARG;
    if(CheckSCB(SCB_CLOSED) == 0) { // No SCB_CLOSED
    	if(!CheckSCB(SCB_DISCONNECT)) {
#ifdef WEBSOCKET_ENA
        	if(CheckSCB(SCB_WSDATA)) {
        		websock_rx_data(ts_conn);
        	}
        	else
#endif
        		if((!CheckSCB(SCB_FCLOSE))&&CheckSCB(SCB_FOPEN)) webserver_send_fdata(ts_conn);
    	}
        if(CheckSCB(SCB_FCLOSE))  {
        	Close_web_conn(ts_conn);
        	SetSCB(SCB_DISCONNECT);
        }
        if(CheckSCB(SCB_DISCONNECT))  web_int_disconnect(ts_conn);
    #if DEBUGSOO > 1
        else  os_printf("...\n");
    #endif
    }
    else { //  SCB_CLOSED
#if DEBUGSOO > 1
      os_printf("#%04x ?\n", web_conn->webflag);
#endif
      ts_conn->flag.tx_null = 1;
      ts_conn->flag.rx_null = 1;
    };
    return ERR_OK;
}
/******************************************************************************
 * FunctionName : webserver_disconnect
 * Description  : calback disconnect
 * Parameters   : arg -- Additional argument to pass to the callback function
 * Returns      : none
*******************************************************************************/
LOCAL void ICACHE_FLASH_ATTR webserver_disconnect(TCP_SERV_CONN *ts_conn)
{
#if DEBUGSOO > 1
    tcpsrv_disconnect_calback_default(ts_conn);
#endif
	WEB_SRV_CONN *web_conn = (WEB_SRV_CONN *)ts_conn->linkd;
	if(web_conn == NULL) return;
    Close_web_conn(ts_conn);
	if(CheckSCB(SCB_SYSSAVE)) {
		ClrSCB(SCB_SYSSAVE);
		sys_write_cfg();
	}
	if(web_conn->web_disc_cb != NULL) {
		web_conn->web_disc_cb(web_conn->web_disc_par);
	}
}
/******************************************************************************
 * FunctionName : webserver_init
 * Description  : Открытие сервера
 * Parameters   : arg -- port N
 * Returns      : none
*******************************************************************************/
err_t ICACHE_FLASH_ATTR webserver_init(uint16 portn)
{
//	WEBFSInit(); // файловая система

	err_t err = ERR_OK;

	TCP_SERV_CFG *p = tcpsrv_init(portn);
	if (p != NULL) {
		// изменим конфиг на наше усмотрение:
		if(syscfg.cfg.b.web_time_wait_delete) p->flag.pcb_time_wait_free = 1; // пусть убивает, для теста и проксей
		p->max_conn = 99; // сработает по heap_size
#if DEBUGSOO > 3
		os_printf("Max connection %d, time waits %d & %d, min heap size %d\n",
				p->max_conn, p->time_wait_rec, p->time_wait_cls, p->min_heap);
#endif
		p->time_wait_rec = syscfg.web_twrec; // =0 -> вечное ожидание
		p->time_wait_cls = syscfg.web_twcls; // =0 -> вечное ожидание
		// слинкуем с желаемыми процедурами:
	 	p->func_discon_cb = webserver_disconnect;
//	 	p->func_listen = webserver_listen; // не требуется
	 	p->func_sent_cb = webserver_sent_callback;
		p->func_recv = webserver_received_data;
		err = tcpsrv_start(p);
		if (err != ERR_OK) {
			tcpsrv_close(p);
			p = NULL;
		}
		else {
#if DEBUGSOO > 1
			os_printf("WEB: init port %u\n", portn);
#endif
		}
	}
	else err = ERR_MEM;
	return err;
}

/******************************************************************************
 * FunctionName : webserver_close
 * Description  : закрытие сервера
 * Parameters   : arg -- port N
 * Returns      : none
*******************************************************************************/
err_t ICACHE_FLASH_ATTR webserver_close(uint16 portn)
{
	err_t err = ERR_ARG;
	if(portn != 0) err = tcpsrv_close(tcpsrv_server_port2pcfg(portn));
#if DEBUGSOO > 1
	if(err == ERR_OK) os_printf("WEB: close\n");
#endif
	return err;
}
/******************************************************************************
 * FunctionName : webserver_reinit
 * Description  : закрытие сервера и открытие нового
 * Parameters   : arg -- port N открытого порта
 * Returns      : none
*******************************************************************************/
err_t ICACHE_FLASH_ATTR webserver_reinit(uint16 portn)
{
	err_t err = ERR_OK;
//	if(portn == syscfg.web_port) return err;
	if(portn) err = tcpsrv_close(tcpsrv_server_port2pcfg(portn)); // закрыть старый порт
	if(syscfg.web_port) err = webserver_init(syscfg.web_port); // открыть новый
	return err;
}

#endif // USE_WEB
