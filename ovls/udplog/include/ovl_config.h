/*
 * ovl_config.h
 *
 *  Created on: 17 марта 2016 г.
 *      Author: PVV
 */

#ifndef _INCLUDE_OVL_CONFIG_H_
#define _INCLUDE_OVL_CONFIG_H_

#define drv_host_ip 	((ip_addr_t *)mdb_buf.ubuf)[90>>1]
#define drv_host_port 	mdb_buf.ubuf[92]
#define drv_init_usr	mdb_buf.ubuf[93] // Флаг: =0 - драйвер закрыт, =1 - драйвер установлен и работает
#define drv_error		mdb_buf.ubuf[94] // Ошибки: =0 - нет ошибок

#define DEFAULT_UDP_PORT 1025

#endif /* _INCLUDE_OVL_CONFIG_H_ */
