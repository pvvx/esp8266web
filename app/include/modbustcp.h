#ifndef _ModBusTcp_H_
//===============================================================================
// ModBusTCP.h  11.2010 pvvx
//===============================================================================
#define _ModBusTcp_H_

err_t mdb_tcp_init(uint16 portn) ICACHE_FLASH_ATTR;
uint32 WrMdbData(uint8 * dbuf, uint16 addr, uint32 len) ICACHE_FLASH_ATTR;
uint32 RdMdbData(uint8 * mdbbuf, uint32 addr, uint32 len) ICACHE_FLASH_ATTR;

#endif //_ModBusTcp_H_
