/******************************************************************************
 * FileName: mdbrs485.h
 * ModBus TCP RTU <> RS-485
 * Author: PV`
 * (c) PV` 2015
*******************************************************************************/
#ifndef _INCLUDE_MODBUSTCPRS485_H_
#define _INCLUDE_MODBUSTCPRS485_H_

#include "user_config.h"
//#ifdef USE_RS485DRV
//#include "driver/rs485drv.h"
#include "modbusrtu.h"
#include "mdbtab.h"
//#include "modbustcp.h"

#define MDB_TCP_RS485_MIN_HEAP 24576

uint32 CalkCRC16tab(uint8 * blk, uint32 len);
struct srs485msg * rs485_new_tx_msg(smdbtcp * p, bool flg_out) ICACHE_FLASH_ATTR;
uint32 rs485_test_msg(uint8 * pbufi, uint32 len);
bool rs485_rx_msg(struct srs485msg *msg);

//#endif USE_RS485DRV
#endif /* _INCLUDE_MODBUSTCPRS485_H_ */
