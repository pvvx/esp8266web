 /******************************************************************************
 * FileName: sys_const_utils.h
 * Description: read/write sdk_sys_cont (esp_init_data_default.bin)
 * Author: PV`
 *******************************************************************************/
#ifndef _INCLUDE_SYS_CONST_U_H_
#define _INCLUDE_SYS_CONST_U_H_

#include "hw/esp8266.h"
#include "sdk/sys_const.h"

uint8 read_sys_const(uint8 idx);
bool write_sys_const(uint8 idx, uint8 data);

#endif /* _INCLUDE_SYS_CONST_U_H_ */
