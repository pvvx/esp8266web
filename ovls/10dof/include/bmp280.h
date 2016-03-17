/*
 * bmp280.h
 *
 *  Created on: 26 янв. 2016 г.
 *      Author: PV`
 */

#ifndef _BMP280_H_
#define _BMP280_H_

//#define BMP280_BIT_STRUCT
//#define BMP280_DOUBLE_PRECISION
#define BMP280_S32_PRECISION
//#define BMP280_P_Q24T8_PRECISION

#define BMP280_S32_t sint32
#define BMP280_U32_t uint32
#define BMP280_S64_t sint64

//
#define BMP280_ID 0x58
//
// registers address
#define BMP280_RA_T1L		0x08
#define BMP280_RA_T1H		0x09
#define BMP280_RA_T2L		0x0A
#define BMP280_RA_T2H		0x0B
#define BMP280_RA_T3L		0x0C
#define BMP280_RA_T3H		0x0D
#define BMP280_RA_P1L		0x0E
#define BMP280_RA_P1H		0x0F
#define BMP280_RA_P2L		0x10
#define BMP280_RA_P2H		0x11
#define BMP280_RA_P3L		0x12
#define BMP280_RA_P3H		0x13
#define BMP280_RA_P4L		0x14
#define BMP280_RA_P4H		0x15
#define BMP280_RA_P5L		0x16
#define BMP280_RA_P5H		0x17
#define BMP280_RA_P6L		0x18
#define BMP280_RA_P6H		0x19
#define BMP280_RA_P7L		0x1A
#define BMP280_RA_P7H		0x1B
#define BMP280_RA_P8L		0x1C
#define BMP280_RA_P8H		0x1D
#define BMP280_RA_P9L		0x1E
#define BMP280_RA_P9H		0x1F
#define BMP280_RA_RA0		0x20 // reserved
#define BMP280_RA_RA1		0x21 // reserved
//
#define BMP280_RA_ID		0x50
#define BMP280_RA_RESET		0x60
#define BMP280_RA_STATUS	0x73
#define BMP280_RA_CTRL_MEANS   0x74
#define BMP280_RA_CONFIG	0x75
#define BMP280_RA_RF6		0x76
#define BMP280_RA_PMSB		0x77
#define BMP280_RA_PLSB		0x78
#define BMP280_RA_PXLSB		0x79
#define BMP280_RA_TMSB		0x7A
#define BMP280_RA_TLSB		0x7B
#define BMP280_RA_TXLSB		0x7C
//
#define BMP280_RESET_VAL 	0x36
//
// struct resgisters OTP BMP280
struct BMP280_dig
{
	uint16 T1;
	sint16 T2;
	sint16 T3;
	uint16 P1;
	sint16 P2;
	sint16 P3;
	sint16 P4;
	sint16 P5;
	sint16 P6;
	sint16 P7;
	sint16 P8;
	sint16 P9;
}__attribute__ ((packed));
//
#ifdef BMP280_BIT_STRUCT
//
// registers bits
enum {
	bmp280_Sleep_mode = 0,
	bmp280_Forced_mode,
	bmp280_Normal_mode = 3
}eBMP280_mode;
enum {
	bmp280_Skipped = 0,
	bmp280_Oversampling_x1,
	bmp280_Oversampling_x2,
	bmp280_Oversampling_x4,
	bmp280_Oversampling_x8,
	bmp280_Oversampling_x16
}eBMP280_oversampling;
enum {
	bmp280_Standby_0t5ms = 0,
	bmp280_Standby_62t5ms,
	bmp280_Standby_125ms,
	bmp280_Standby_250ms,
	bmp280_Standby_500ms,
	bmp280_Standby_1sec,
	bmp280_Standby_2sec,
	bmp280_Standby_4sec
}eBMP280_standby_time;
enum {
	bmp280_Filter_off = 0,
	bmp280_Filter_2,
	bmp280_Filter_4,
	bmp280_Filter_8,
	bmp280_Filter_16
}eBMP280_filter;
enum {
	bmp280_spi3w_dis = 0,
	bmp280_spi3w_ena
}eBMP280__spi3w;
//
// registers struct
struct BMP280_reg_status
{
	uint8 im_update : 1;
	uint8 nonebits1 : 2;
	uint8 measuring : 1;
	uint8 nonebits2 : 4;
}__attribute__ ((packed));
struct BMP280_reg_ctrl_meas
{
	uint8 mode 		: 2;
	uint8 osrs_p 	: 3;
	uint8 osrs_t 	: 3;
}__attribute__ ((packed));
struct BMP280_reg_config
{
	uint8 spi3w_en	: 1;
	uint8 nonebits  : 1;
	uint8 filter 	: 3;
	uint8 t_sb 		: 3;
}__attribute__ ((packed));
struct BMP280_reg_pt
{
	unsigned datah	: 8;
	unsigned datam	: 8;
	unsigned datal	: 8;
}__attribute__ ((packed));
struct BMP280_reg{
	struct BMP280_reg_status status;
	struct BMP280_reg_ctrl_meas ctrl_meas;
	struct BMP280_reg_config config;
	uint8  none;
	uint8 press[3]; //	struct BMP280_reg_pt press;
	uint8 temp[3]; //	struct BMP280_reg_pt temp;
}__attribute__ ((packed));
//
// struct BMP280
typedef struct __attribute__ ((packed)){
	struct BMP280_dig dig;
	struct BMP280_reg reg;
	uint8  id;
}sbmp280;
//
#else
//
// STATUS
#define BMP280_IM_UPDATE		1
#define BMP280_MEASURING		(1<<3)
//
// CTRL_MEANS
#define BMP280_SLEEP_MODE		0
#define BMP280_FORCED_MODE		1
#define BMP280_NORMAL_MODE		3
#define BMP280_OVERSAMP_P_NONE	0
#define BMP280_OVERSAMP_P_1		(1<<2)
#define BMP280_OVERSAMP_P_2		(2<<2)
#define BMP280_OVERSAMP_P_4		(3<<2)
#define BMP280_OVERSAMP_P_8		(4<<2)
#define BMP280_OVERSAMP_P_16	(5<<2)
#define BMP280_OVERSAMP_T_NONE	0
#define BMP280_OVERSAMP_T_1		(1<<5)
#define BMP280_OVERSAMP_T_2		(2<<5)
#define BMP280_OVERSAMP_T_4		(3<<5)
#define BMP280_OVERSAMP_T_8		(4<<5)
#define BMP280_OVERSAMP_T_16	(5<<5)
//
// CONFIG
#define BMP280_SPI3W_DIS		0
#define BMP280_SPI3W_ENA		1
#define BMP280_FILER_OFF		0
#define BMP280_FILER_2			(1<<2)
#define BMP280_FILER_4			(2<<2)
#define BMP280_FILER_8			(3<<2)
#define BMP280_FILER_16			(4<<2)
#define BMP280_STANDBY_0T5MS	0
#define BMP280_STANDBY_62T5MS	(1<<5)
#define BMP280_STANDBY_125MS	(2<<5)
#define BMP280_STANDBY_250MS	(3<<5)
#define BMP280_STANDBY_500MS	(4<<5)
#define BMP280_STANDBY_1SEC		(5<<5)
#define BMP280_STANDBY_2SEC		(6<<5)
#define BMP280_STANDBY_4SEC		(7<<5)
//
// struct resgisters BMP280
struct uBMP280_reg{
	uint8 status;
	uint8 ctrl_meas;
	uint8 config;
	uint8 none;
	uint8 press[3];
	uint8 temp[3];
}__attribute__ ((packed));
//
// struct BMP280
typedef struct __attribute__ ((packed)){
	struct BMP280_dig dig;
	struct uBMP280_reg reg;
	uint8  id;
}sbmp280;
#endif

extern sbmp280 bmp280;

#ifdef BMP280_DOUBLE_PRECISION
double bmp280_compensate_T_double(sbmp280 *p);
double bmp280_compensate_P_double(sbmp280 *p);
#endif

#ifdef BMP280_S32_PRECISION
extern BMP280_S32_t bmp280_compensate_T_int32(sbmp280 *p);
#ifdef BMP280_P_Q24T8_PRECISION
extern BMP280_U32_t bmp280_compensate_P_int64(sbmp280 *p);
#else
extern BMP280_U32_t bmp280_compensate_P_int32(sbmp280 *p);
#endif
#endif

bool init_bmp280(void);

#endif /* _BMP280_H_ */
