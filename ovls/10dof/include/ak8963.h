/*
 * ak8963.h
 *
 *  Created on: 27 янв. 2016 г.
 *      Author: PVV
 */

#ifndef _AK8963_H_
#define _AK8963_H_

#define AK8963_ADDRESS		0x0C
/////////////////////////////////////////////////////////////
//
// Register Map for Magnetometer (AK8963)
//
#define AK8963_RA_WIA		0x00
#define AK8963_RA_INFO		0x01
#define AK8963_RA_ST1		0x02
#define AK8963_RA_HXL		0x03
#define AK8963_RA_HXH		0x04
#define AK8963_RA_HYL		0x05
#define AK8963_RA_HYH		0x06
#define AK8963_RA_HZL		0x07
#define AK8963_RA_HZH		0x08
#define AK8963_RA_ST2		0x09
#define AK8963_RA_CNTL1		0x0A
#define AK8963_RA_CNTL2		0x0B
#define AK8963_RA_ASTC		0x0C
#define AK8963_RA_TS1		0x0D
#define AK8963_RA_TS2		0x0E
#define AK8963_RA_I2CDIS	0x0F
#define AK8963_RA_ASAX		0x10
#define AK8963_RA_ASAY		0x11
#define AK8963_RA_ASAZ		0x12
//
// WIA: Device ID
#define AK8963_WIA_ID		0x48
//
// ST1: Status 1
#define AK8963_ST1_DRDY	(1<<0)
//
// ST2: Status 2
#define AK8963_ST2_BITM		(1<<4)
#define AK8963_ST2_HOFL		(1<<3)
//
// CNTL1: Control 1
#define AK8963_CNTL1_16BITS		(1<<4)
#define AK8963_CNTL1_PD_MODE		0
#define AK8963_CNTL1_SM_MODE		1 // Sensor is measured for one time and data is output. Transits to Power-down mode automatically after measurement ended.
#define AK8963_CNTL1_CM1_MODE		2 // Sensor is measured periodically in 8Hz.
#define AK8963_CNTL1_CM2_MODE		6 // Sensor is measured periodically in 100Hz.
#define AK8963_CNTL1_EXT_MODE		4 // Sensor is measured for one time by external trigger.
#define AK8963_CNTL1_ST_MODE		8 // Sensor is self-tested and the result is output. Transits to Power-down mode automatically.
#define AK8963_CNTL1_FR_MODE		0x0F // Fuse ROM access mode Turn on the circuit needed to read out Fuse ROM.
//
// CNTL2: Control 2
#define AK8963_CNTL2_SRST		(1<<0)
//
// ASTC: Self-Test Control
#define AK8963_ASTC_SELF		(1<<6)
//
// I2CDIS: I2C Disable
#define AK8963_I2CDIS7		(1<<7)
#define AK8963_I2CDIS6		(1<<6)
#define AK8963_I2CDIS5		(1<<5)
#define AK8963_I2CDIS4		(1<<4)
#define AK8963_I2CDIS3		(1<<3)
#define AK8963_I2CDIS2		(1<<2)
#define AK8963_I2CDIS1		(1<<1)
#define AK8963_I2CDIS0		(1<<0)

struct AK8963_st1 {
	uint8	drdy		: 1;
	uint8	dor			: 1;
	uint8	reserved	: 6;
}__attribute__ ((packed));
struct AK8963_st2 {
	uint8	reserved1	: 3;
	uint8	holf 		: 1;
	uint8	bitm 		: 1;
	uint8	reserved2	: 3;
}__attribute__ ((packed));
struct AK8963_cntl1 {
	uint8	mode		: 4;
	uint8	bit			: 1;
	uint8	reserved	: 3;
}__attribute__ ((packed));
struct AK8963_cntl2 {
	uint8	srst 		: 1;
	uint8	reserved 	: 7;
}__attribute__ ((packed));
struct AK8963_astc {
	uint8	reserved1 	: 6;
	uint8	srst 		: 1;
	uint8	reserved2 	: 1;
}__attribute__ ((packed));

struct sAK8963_regs {
	uint8	wia;
	uint8	info;
	uint8	st1;
	uint8	hxl;
	uint8	hxh;
	uint8	hyl;
	uint8	hyh;
	uint8	hzl;
	uint8	hzh;
	uint8	st2;
	uint8	cntl1;
	uint8	cntl2;
	uint8	astc;
	uint8	ts1;
	uint8	ts2;
	uint8	i2cdis;
	uint8	asax;
	uint8	asay;
	uint8	asaz;
//	uint8	rsv;
}__attribute__ ((packed));

#endif /* _AK8963_H_ */
