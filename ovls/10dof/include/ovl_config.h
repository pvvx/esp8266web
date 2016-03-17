/*
 * ovl_config.h
 *
 *  Created on: 17 марта 2016 г.
 *      Author: PVV
 */

#ifndef _INCLUDE_OVL_CONFIG_H_
#define _INCLUDE_OVL_CONFIG_H_

#define	CS_BMP280_PIN	mdb_buf.ubuf[40]
#define	CS_MPU9250_PIN	mdb_buf.ubuf[41]
#define drv_host_ip 	((ip_addr_t *)mdb_buf.ubuf)[42>>1]
#define drv_host_port 	mdb_buf.ubuf[44]
#define drv_init_usr	mdb_buf.ubuf[45] // Флаг: =0 - драйвер закрыт, =1 - драйвер установлен и работает
#define drv_error		mdb_buf.ubuf[46] // Ошибки: =0 - нет ошибок
#define drv_temp		mdb_buf.ubuf[47]
#define drv_press		((sint32 *)(mdb_buf.ubuf))[48>>1] // long

typedef struct __attribute__ ((packed)) {
	sint32 mag[3];		//00..11
	sint16 accel[3];	//12..17
	sint16 gyro[3];		//18..23
} el_sblk_data;

#define MAX_TX_BLK_DATA 10 // ((1024-10)/sizeof(el_sblk_data))

// формат UDP посылки
typedef struct {
	uint16	number;		// номер остчета
	sint32 temp;		// temperature in DegC, resolution is 0.01 DegC.
	uint32 press;		// pressure in Pa as unsigned, resolution is 0.01 hPa.
	el_sblk_data data[MAX_TX_BLK_DATA];
}tsblk_data;

//extern tsblk_data sblk_data;

#endif /* _INCLUDE_OVL_CONFIG_H_ */
