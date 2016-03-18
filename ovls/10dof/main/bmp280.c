/*
 * BMP280.c
 *
 *  Created on: 26 янв. 2016 г.
 *      Author: PVV
 */
#include "user_config.h"
#include "os_type.h"
#include "osapi.h"
#include "bios.h"
#include "hw/esp8266.h"
#include "sdk/add_func.h"
#include "sdk/os_printf.h"
#include "hspi_master.h"
#include "bmp280.h"

#include "ovl_sys.h"
#include "ovl_config.h"

sbmp280 bmp280;
BMP280_S32_t t_fine DATA_IRAM_ATTR; /* t_fine carries fine temperature as global value */

#ifdef BMP280_DOUBLE_PRECISION
// Returns temperature in DegC, double precision. Output value of “51.23” equals 51.23 DegC.
double bmp280_compensate_T_double(sbmp280 *p) {
	double var1, var2, T;
	BMP280_S32_t adc_T = (p->reg.temp[0]<<12) | (p->reg.temp[1]<<4) | (p->reg.temp[2]>>4);
	var1 = (((double) adc_T) / 16384.0 - ((double)p->dig.T1)/1024.0) * ((double)p->dig.T2);
	var2 = ((((double) adc_T) / 131072.0 - ((double)p->dig.T1)/8192.0) *
	(((double)adc_T)/131072.0 - ((double) p->dig.T1)/8192.0)) * ((double)p->dig.T3);
	t_fine = (BMP280_S32_t) (var1 + var2);
	T = (var1 + var2) / 5120.0;
	return T;
}
// Returns pressure in Pa as double. Output value of “96386.2” equals 96386.2 Pa = 963.862 hPa
double bmp280_compensate_P_double(sbmp280 *p) {
	double var1, var2, x;
	var1 = ((double) t_fine / 2.0) - 64000.0;
	var2 = var1 * var1 * ((double) p->dig.P6) / 32768.0;
	var2 = var2 + var1 * ((double) p->dig.P5) * 2.0;
	var2 = (var2 / 4.0) + (((double) p->dig.P4) * 65536.0);
	var1 = (((double) p->dig.P3) * var1 * var1 / 524288.0
			+ ((double) p->dig.P2) * var1) / 524288.0;
	var1 = (1.0 + var1 / 32768.0) * ((double) p->dig.P1);
	if (var1 == 0.0) {
		return 0; // avoid exception caused by division by zero
	}
	BMP280_S32_t adc_P = (bmp280.reg.press[0]<<12) | (bmp280.reg.press[1]<<4) | (bmp280.reg.press[2]>>4);
	x = 1048576.0 - (double)adc_P;
	x = (x - (var2 / 4096.0)) * 6250.0 / var1;
	var1 = ((double) p->dig.P9) * x * x / 2147483648.0;
	var2 = x * ((double) p->dig.P8) / 32768.0;
	x = x + (var1 + var2 + ((double) p->dig.P7)) / 16.0;
	return x;
}

BMP280_U32_t bmp280CalcAltitude(sbmp280 *p) {
	//get altitude in dm
	return 44330 * (1 - pow(p->press / 101325, 0.19029)) * 100;
}
#endif
#if defined(BMP280_P_Q24T8_PRECISION) || defined(BMP280_S32_PRECISION)
/* Returns temperature in DegC, resolution is 0.01 DegC. Output value of “5123” equals 51.23 DegC. */
BMP280_S32_t bmp280_compensate_T_int32(sbmp280 *p) {
	BMP280_S32_t var1, var2, T;
	BMP280_S32_t adc_T = (p->reg.temp[0]<<12) | (p->reg.temp[1]<<4) | (p->reg.temp[2]>>4);
	var1 = ((((adc_T >> 3) - ((BMP280_S32_t)p->dig.T1 << 1))) * ((BMP280_S32_t)p->dig.T2)) >> 11;
	var2 = (((((adc_T >> 4) - ((BMP280_S32_t)p->dig.T1)) * ((adc_T >> 4) - ((BMP280_S32_t)p->dig.T1))) >> 12) *
			((BMP280_S32_t)p->dig.T3)) >> 14;
	t_fine = var1 + var2;
	T = (t_fine * 5 + 128) >> 8;
	return T;
}
#endif
#ifdef BMP280_P_Q24T8_PRECISION
// Returns pressure in Pa as unsigned 32 bit integer in Q24.8 format (24 integer bits and 8 fractional bits).
// Output value of “24674867” represents 24674867/256 = 96386.2 Pa = 963.862 hPa
BMP280_U32_t bmp280_compensate_P_int64(sbmp280 *p) {
	BMP280_S64_t var1, var2, x;
	var1 = ((BMP280_S64_t) t_fine) - 128000;
	var2 = var1 * var1 * (BMP280_S64_t) p->dig.P6;
	var2 = var2 + ((var1 * (BMP280_S64_t) p->dig.P5) << 17);
	var2 = var2 + (((BMP280_S64_t) p->dig.P4) << 35);
	var1 = ((var1 * var1 * (BMP280_S64_t) p->dig.P3) >> 8) + ((var1 * (BMP280_S64_t) p->dig.P2) << 12);
	var1 = (((((BMP280_S64_t) 1) << 47) + var1)) * ((BMP280_S64_t) p->dig.P1) >> 33;
	if (var1 == 0) {
		return 0; // avoid exception caused by division by zero
	}
	BMP280_S32_t adc_P = (bmp280.reg.press[0]<<12) | (bmp280.reg.press[1]<<4) | (bmp280.reg.press[2]>>4); // 3852
	x = 1048576 - adc_P;
	x = (((x << 31) - var2) * 3125) / var1;
	var1 = (((BMP280_S64_t) p->dig.P9) * (x >> 13) * (x >> 13)) >> 25;
	var2 = (((BMP280_S64_t) p->dig.P8) * x) >> 19;
	x = ((x + var1 + var2) >> 8) + (((BMP280_S64_t) p->dig.P7) << 4);
	return (BMP280_U32_t) x;
}
#endif
#ifdef BMP280_S32_PRECISION
/* Returns pressure in Pa as unsigned 32 bit integer. Output value of “96386” equals 96386 Pa = 963.86 hPa */
BMP280_U32_t bmp280_compensate_P_int32(sbmp280 *p) {
	BMP280_S32_t var1, var2;
	BMP280_U32_t x;
	BMP280_S32_t adc_P = (bmp280.reg.press[0]<<12) | (bmp280.reg.press[1]<<4) | (bmp280.reg.press[2]>>4); // 3852
	var1 = (((BMP280_S32_t) t_fine) >> 1) - (BMP280_S32_t) 64000;
	var2 = (((var1 >> 2) * (var1 >> 2)) >> 11) * ((BMP280_S32_t) p->dig.P6);
	var2 = var2 + ((var1 * ((BMP280_S32_t) p->dig.P5)) << 1);
	var2 = (var2 >> 2) + (((BMP280_S32_t) p->dig.P4) << 16);
	var1 = (((p->dig.P3 * (((var1 >> 2) * (var1 >> 2)) >> 13)) >> 3) + ((((BMP280_S32_t) p->dig.P2) * var1) >> 1)) >> 18;
	var1 = ((((32768 + var1)) * ((BMP280_S32_t) p->dig.P1)) >> 15);
	if (var1 == 0) {
		return 0; // avoid exception caused by division by zero
	}
	x = (((BMP280_U32_t)(((BMP280_S32_t) 1048576) - adc_P) - (var2 >> 12)))	* 3125;
	if (x < 0x80000000) {
		x = (x << 1) / ((BMP280_U32_t) var1);
	} else {
		x = (x / (BMP280_U32_t) var1) * 2;
	}
	var1 = (((BMP280_S32_t) p->dig.P9) * ((BMP280_S32_t) (((x >> 3) * (x >> 3)) >> 13))) >> 12;
	var2 = (((BMP280_S32_t) (x >> 2)) * ((BMP280_S32_t) p->dig.P8)) >> 13;
	x = (BMP280_U32_t)((BMP280_S32_t) x + ((var1 + var2 + p->dig.P7) >> 4));
	return x;
}
#endif

//-------------------------------------------------------------------------------
// init_bmp280
//-------------------------------------------------------------------------------
bool init_bmp280(void)
{
		hspi_cmd_read(CS_BMP280_PIN, BMP280_RA_ID | SPI_CMD_READ, (uint8 *)&bmp280.id, 1);
#if DEBUGSOO > 1
		os_printf("BMP280 id: %02x\n", bmp280.id);
#endif
		if(bmp280.id == BMP280_ID) {
			uint8 x = BMP280_RESET_VAL;
			hspi_cmd_write(CS_BMP280_PIN, BMP280_RA_RESET & 0x7F,(uint8 *)&x, 1);
			ets_delay_us(20);
#ifdef BMP280_BIT_STRUCT
			bmp280.reg.ctrl_meas.osrs_t = bmp280_Oversampling_x16;
			bmp280.reg.ctrl_meas.osrs_p = bmp280_Oversampling_x16;
			bmp280.reg.ctrl_meas.mode = bmp280_Normal_mode;
#else
			bmp280.reg.ctrl_meas = BMP280_NORMAL_MODE | BMP280_OVERSAMP_P_16 | BMP280_OVERSAMP_T_16;
#endif
			hspi_cmd_write(CS_BMP280_PIN, BMP280_RA_CTRL_MEANS & 0x7F,(uint8 *)&bmp280.reg.ctrl_meas, 1);
#ifdef BMP280_BIT_STRUCT
			bmp280.reg.config.t_sb = bmp280_Standby_0t5ms;
			bmp280.reg.config.nonebits = 0;
			bmp280.reg.config.filter = bmp280_Filter_16;
			bmp280.reg.config.spi3w_en = bmp280_spi3w_dis;
#else
			bmp280.reg.config = BMP280_SPI3W_DIS | BMP280_FILER_16 | BMP280_STANDBY_0T5MS;
#endif
			hspi_cmd_write(CS_BMP280_PIN, BMP280_RA_CONFIG & 0x7F,(uint8 *)&bmp280.reg.config, 1);
			hspi_cmd_read(CS_BMP280_PIN, BMP280_RA_T1L | SPI_CMD_READ, (uint8 *)&bmp280.dig, sizeof(bmp280.dig));
#if DEBUGSOO > 1
			os_printf("Dig BMP280:");
			uint8 * ptr = (uint8 *)&bmp280.dig;
			uint32 i = sizeof(bmp280.dig);
			while(i--) os_printf(" %02x", *ptr++);
			os_printf("\n");
#endif
			hspi_cmd_read(CS_BMP280_PIN, BMP280_RA_STATUS | SPI_CMD_READ, (uint8 *)&bmp280.reg.status, BMP280_RA_TXLSB - BMP280_RA_STATUS + 1);
#if DEBUGSOO > 2
			os_printf("Reg BMP280:");
			ptr = (uint8 *)&bmp280.reg;
			i = sizeof(bmp280.reg);
			while(i--)	os_printf(" %02x", *ptr++);
			os_printf("\n");
/*
			sblk_data.temp = bmp280_compensate_T_int32(&bmp280) - 600;
			sblk_data.press = bmp280_compensate_P_int32(&bmp280);
			os_printf("Temp: %d C, Press: %d Pa\n", sblk_data.temp, sblk_data.press); */
#endif
		}
		else return false;
		return true;
}
