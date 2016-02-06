#ifndef _ModBusTcp_H_
//===============================================================================
// ModBusTCP.h  11.2010 pvvx
//===============================================================================
#define _ModBusTcp_H_

#include "lwip/tcp.h"
#include "tcp_srv_conn.h"

#define DEFAULT_MDB_PORT USE_MODBUS // 502
#define DEFAULT_MDB_ID 50 // номер устройства ESP8266 по шине modbus

extern TCP_SERV_CFG * mdb_tcp_servcfg;
extern TCP_SERV_CONN * mdb_tcp_conn;

err_t mdb_tcp_start(uint16 portn) ICACHE_FLASH_ATTR;
void mdb_tcp_close(void) ICACHE_FLASH_ATTR;
uint32 WrMdbData(uint8 * dbuf, uint16 addr, uint32 len) ICACHE_FLASH_ATTR;
uint32 RdMdbData(uint8 * mdbbuf, uint32 addr, uint32 len) ICACHE_FLASH_ATTR;

#endif //_ModBusTcp_H_
