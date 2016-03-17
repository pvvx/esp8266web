/*
 * mpu9250.h
 *
 *  Created on: 27 янв. 2016 г.
 *      Author: PVV
 */

#ifndef _MPU9250_H_
#define _MPU9250_H_

#include "ak8963.h"

#define MPU9250_ADDRESS_AD0_LOW     0x68 // address pin low (GND), default for InvenSense evaluation board
#define MPU9250_ADDRESS_AD0_HIGH    0x69 // address pin high (VCC)
#define MPU9250_ADDRESS     MPU9250_ADDRESS_AD0_LOW
#define MPU9250_DEV_ID 				0x71 // MPU9250_RA_WHO_AM_I
/////////////////////////////////////////////////////////////
//
// MPU-9250 mode register map for Gyroscope and Accelerometer
//
#define MPU9250_RA_SELF_TEST_X_GYRO	0x00 // XG_ST_DATA[7:0]
#define MPU9250_RA_SELF_TEST_Y_GYRO	0x01 // YG_ST_DATA[7:0]
#define MPU9250_RA_SELF_TEST_Z_GYRO	0x02 // ZG_ST_DATA[7:0]
#define MPU9250_RA_X_FINE_GAIN      0x03 //[7:0] X_FINE_GAIN
#define MPU9250_RA_Y_FINE_GAIN      0x04 //[7:0] Y_FINE_GAIN
#define MPU9250_RA_Z_FINE_GAIN      0x05 //[7:0] Z_FINE_GAIN
#define MPU9250_RA_XA_OFFS_H        0x06 //[15:0] XA_OFFS
#define MPU9250_RA_XA_OFFS_L_TC     0x07
#define MPU9250_RA_YA_OFFS_H        0x08 //[15:0] YA_OFFS
#define MPU9250_RA_YA_OFFS_L_TC     0x09
#define MPU9250_RA_ZA_OFFS_H        0x0A //[15:0] ZA_OFFS
#define MPU9250_RA_ZA_OFFS_L_TC     0x0B
//
#define MPU9250_RA_XG_OFFS_USRH     0x13 //[15:0] XG_OFFS_USR
#define MPU9250_RA_XG_OFFS_USRL     0x14
#define MPU9250_RA_YG_OFFS_USRH     0x15 //[15:0] YG_OFFS_USR
#define MPU9250_RA_YG_OFFS_USRL     0x16
#define MPU9250_RA_ZG_OFFS_USRH     0x17 //[15:0] ZG_OFFS_USR
#define MPU9250_RA_ZG_OFFS_USRL     0x18
#define MPU9250_RA_SMPLRT_DIV       0x19 // SMPLRT_DIV[7:0]  SAMPLE_RATE= Internal_Sample_Rate / (1 + SMPLRT_DIV)
#define MPU9250_RA_CONFIG           0x1A // Configuration
#define MPU9250_RA_GYRO_CONFIG      0x1B
#define MPU9250_RA_ACCEL_CONFIG     0x1C
#define MPU9250_RA_ACCEL_CONFIG2    0x1D
#define MPU9250_LP_ACCEL_ODR        0x1E
#define MPU9250_RA_WOM_THR          0x1F
#define MPU9250_RA_MOT_DUR          0x20
#define MPU9250_RA_ZRMOT_THR        0x21
#define MPU9250_RA_ZRMOT_DUR        0x22
#define MPU9250_RA_FIFO_EN          0x23
#define MPU9250_RA_I2C_MST_CTRL     0x24
#define MPU9250_RA_I2C_SLV0_ADDR    0x25
#define MPU9250_RA_I2C_SLV0_REG     0x26
#define MPU9250_RA_I2C_SLV0_CTRL    0x27
#define MPU9250_RA_I2C_SLV1_ADDR    0x28
#define MPU9250_RA_I2C_SLV1_REG     0x29
#define MPU9250_RA_I2C_SLV1_CTRL    0x2A
#define MPU9250_RA_I2C_SLV2_ADDR    0x2B
#define MPU9250_RA_I2C_SLV2_REG     0x2C
#define MPU9250_RA_I2C_SLV2_CTRL    0x2D
#define MPU9250_RA_I2C_SLV3_ADDR    0x2E
#define MPU9250_RA_I2C_SLV3_REG     0x2F
#define MPU9250_RA_I2C_SLV3_CTRL    0x30
#define MPU9250_RA_I2C_SLV4_ADDR    0x31
#define MPU9250_RA_I2C_SLV4_REG     0x32
#define MPU9250_RA_I2C_SLV4_DO      0x33
#define MPU9250_RA_I2C_SLV4_CTRL    0x34
#define MPU9250_RA_I2C_SLV4_DI      0x35
#define MPU9250_RA_I2C_MST_STATUS   0x36
#define MPU9250_RA_INT_PIN_CFG      0x37
#define MPU9250_RA_INT_ENABLE       0x38
#define MPU9250_RA_DMP_INT_STATUS   0x39
#define MPU9250_RA_INT_STATUS       0x3A
#define MPU9250_RA_ACCEL_XOUT_H     0x3B
#define MPU9250_RA_ACCEL_XOUT_L     0x3C
#define MPU9250_RA_ACCEL_YOUT_H     0x3D
#define MPU9250_RA_ACCEL_YOUT_L     0x3E
#define MPU9250_RA_ACCEL_ZOUT_H     0x3F
#define MPU9250_RA_ACCEL_ZOUT_L     0x40
#define MPU9250_RA_TEMP_OUT_H       0x41 // Temperature in degrees C = TEMP_OUT/333.87 + 21.0
#define MPU9250_RA_TEMP_OUT_L       0x42
#define MPU9250_RA_GYRO_XOUT_H      0x43
#define MPU9250_RA_GYRO_XOUT_L      0x44
#define MPU9250_RA_GYRO_YOUT_H      0x45
#define MPU9250_RA_GYRO_YOUT_L      0x46
#define MPU9250_RA_GYRO_ZOUT_H      0x47
#define MPU9250_RA_GYRO_ZOUT_L      0x48
#define MPU9250_RA_EXT_SENS_DATA_00 0x49
#define MPU9250_RA_EXT_SENS_DATA_01 0x4A
#define MPU9250_RA_EXT_SENS_DATA_02 0x4B
#define MPU9250_RA_EXT_SENS_DATA_03 0x4C
#define MPU9250_RA_EXT_SENS_DATA_04 0x4D
#define MPU9250_RA_EXT_SENS_DATA_05 0x4E
#define MPU9250_RA_EXT_SENS_DATA_06 0x4F
#define MPU9250_RA_EXT_SENS_DATA_07 0x50
#define MPU9250_RA_EXT_SENS_DATA_08 0x51
#define MPU9250_RA_EXT_SENS_DATA_09 0x52
#define MPU9250_RA_EXT_SENS_DATA_10 0x53
#define MPU9250_RA_EXT_SENS_DATA_11 0x54
#define MPU9250_RA_EXT_SENS_DATA_12 0x55
#define MPU9250_RA_EXT_SENS_DATA_13 0x56
#define MPU9250_RA_EXT_SENS_DATA_14 0x57
#define MPU9250_RA_EXT_SENS_DATA_15 0x58
#define MPU9250_RA_EXT_SENS_DATA_16 0x59
#define MPU9250_RA_EXT_SENS_DATA_17 0x5A
#define MPU9250_RA_EXT_SENS_DATA_18 0x5B
#define MPU9250_RA_EXT_SENS_DATA_19 0x5C
#define MPU9250_RA_EXT_SENS_DATA_20 0x5D
#define MPU9250_RA_EXT_SENS_DATA_21 0x5E
#define MPU9250_RA_EXT_SENS_DATA_22 0x5F
#define MPU9250_RA_EXT_SENS_DATA_23 0x60
#define MPU9250_RA_MOT_DETECT_STATUS    0x61
//
#define MPU9250_RA_I2C_SLV0_DO      0x63
#define MPU9250_RA_I2C_SLV1_DO      0x64
#define MPU9250_RA_I2C_SLV2_DO      0x65
#define MPU9250_RA_I2C_SLV3_DO      0x66
#define MPU9250_RA_I2C_MST_DELAY_CTRL   0x67
#define MPU9250_RA_SIGNAL_PATH_RESET    0x68
#define MPU9250_RA_MOT_DETECT_CTRL      0x69
#define MPU9250_RA_USER_CTRL        0x6A
#define MPU9250_RA_PWR_MGMT_1       0x6B
#define MPU9250_RA_PWR_MGMT_2       0x6C
#define MPU9250_RA_BANK_SEL         0x6D
#define MPU9250_RA_MEM_START_ADDR   0x6E
#define MPU9250_RA_MEM_R_W          0x6F
#define MPU9250_RA_DMP_CFG_1        0x70
#define MPU9250_RA_DMP_CFG_2        0x71
#define MPU9250_RA_FIFO_COUNTH      0x72
#define MPU9250_RA_FIFO_COUNTL      0x73
#define MPU9250_RA_FIFO_R_W         0x74
#define MPU9250_RA_WHO_AM_I         0x75 // Should return 0x71
//
#define MPU9250_XA_OFFSET_H			0x77
#define MPU9250_XA_OFFSET_L			0x78
//
#define MPU9250_YA_OFFSET_H			0x7A
#define MPU9250_YA_OFFSET_L			0x7B
//
#define MPU9250_ZA_OFFSET_H			0x7D
#define MPU9250_ZA_OFFSET_L			0x7E
//
//
// Configuration MPU9250_RA_CONFIG:FIFO_MODE
#define MPU9250_FIFO_MODE_REPLACING      (0x0<<6)
#define MPU9250_FIFO_MODE_NOT_REPLACING  (0x1<<6)
// Configuration MPU9250_RA_CONFIG:EXT_SYNC_SET[2:0]
#define MPU9250_EXT_SYNC_DISABLED       (0x0<<3)
#define MPU9250_EXT_SYNC_TEMP_OUT_L     (0x1<<3)
#define MPU9250_EXT_SYNC_GYRO_XOUT_L    (0x2<<3)
#define MPU9250_EXT_SYNC_GYRO_YOUT_L    (0x3<<3)
#define MPU9250_EXT_SYNC_GYRO_ZOUT_L    (0x4<<3)
#define MPU9250_EXT_SYNC_ACCEL_XOUT_L   (0x5<<3)
#define MPU9250_EXT_SYNC_ACCEL_YOUT_L   (0x6<<3)
#define MPU9250_EXT_SYNC_ACCEL_ZOUT_L   (0x7<<3)
// Configuration MPU9250_RA_CONFIG:DLPF_CFG[2:0] + FCHOICE
#define MPU9250_DLPF_BW_250         0x00
#define MPU9250_DLPF_BW_184         0x01
#define MPU9250_DLPF_BW_92          0x02
#define MPU9250_DLPF_BW_41          0x03
#define MPU9250_DLPF_BW_20          0x04
#define MPU9250_DLPF_BW_10          0x05
#define MPU9250_DLPF_BW_5           0x06
#define MPU9250_DLPF_BW_3600        0x07
//
// Gyroscope Configuration MPU9250_RA_GYRO_CONFIG:Fchoice_b[1:0]
#define MPU9250_FCHOICE_BW_8800		0
#define MPU9250_FCHOICE_BW_3600		1
#define MPU9250_FCHOICE_BW_DLPF		3
// Gyroscope Configuration MPU9250_RA_GYRO_CONFIG:GYRO_FS_SEL[1:0]
#define MPU9250_GYRO_FS_250DPS	(0<<3)
#define MPU9250_GYRO_FS_500DPS	(1<<3)
#define MPU9250_GYRO_FS_1000DPS	(2<<3)
#define MPU9250_GYRO_FS_2000DPS	(3<<3)
// Gyroscope Configuration MPU9250_RA_GYRO_CONFIG:XGYRO_Cten,YGYRO_Cten,ZGYRO_Cten
#define MPU9250_XGYRO_Cten		(1<<7)
#define MPU9250_YGYRO_Cten		(1<<6)
#define MPU9250_ZGYRO_Cten		(1<<5)
//
// Accelerometer Configuration MPU9250_RA_ACCEL_CONFIG:ACCEL_FS_SEL[1:0]
#define MPU9250_ACCEL_FS_2G		(0<<3)
#define MPU9250_ACCEL_FS_4G		(1<<3)
#define MPU9250_ACCEL_FS_8G		(2<<3)
#define MPU9250_ACCEL_FS_16G	(3<<3)
//
// Accelerometer Configuration MPU9250_RA_ACCEL_CONFIG
#define MPU9250_AX_ST_EN		(1<<7)
#define MPU9250_AY_ST_EN		(1<<6)
#define MPU9250_AZ_ST_EN		(1<<5)
//
// Accelerometer Configuration 2 MPU9250_RA_ACCEL_CONFIG2: A_DLPFCFG[0:2] + ACCEL_FCHOICE
//#define MPU9250_A_DLPFCFG_460
#define MPU9250_A_DLPFCFG_5		0
#define MPU9250_A_DLPFCFG_10	1
#define MPU9250_A_DLPFCFG_20	2
#define MPU9250_A_DLPFCFG_41	3
#define MPU9250_A_DLPFCFG_92	4
#define MPU9250_A_DLPFCFG_184	5
#define MPU9250_A_DLPFCFG_460	6
#define MPU9250_A_DLPFCFG_1130	7
#define MPU9250_ACCEL_FCHOICE_B (1<<3)
//
// Low Power Accelerometer ODR Control MPU9250_LP_ACCEL_ODR: lposc_clksel[3:0]
#define MPU9250_LPOSC_CLKSEL_0t24	0
#define MPU9250_LPOSC_CLKSEL_0t49	1
#define MPU9250_LPOSC_CLKSEL_0t98	2
#define MPU9250_LPOSC_CLKSEL_1t95	3
#define MPU9250_LPOSC_CLKSEL_3t91	4
#define MPU9250_LPOSC_CLKSEL_7t81	5
#define MPU9250_LPOSC_CLKSEL_15t63	6
#define MPU9250_LPOSC_CLKSEL_31t25	7
#define MPU9250_LPOSC_CLKSEL_62t50	8
#define MPU9250_LPOSC_CLKSEL_125	9
#define MPU9250_LPOSC_CLKSEL_250	10
#define MPU9250_LPOSC_CLKSEL_500	11
#define MPU9250_LPOSC_CLKSEL_RESERVED	12 // 12..15
//
// Wake-on Motion Threshold MPU9250_RA_WOM_THR [0:7]
#define MPU9250_WOM_Threshold_mg	4 // 4mg		Range is 0mg to 1020mg.
//
// FIFO Enable MPU9250_RA_FIFO_EN
#define MPU9250_FIFO_EN_SLV_0		(1<<0)
#define MPU9250_FIFO_EN_SLV_1		(1<<1)
#define MPU9250_FIFO_EN_SLV_2		(1<<2)
#define MPU9250_FIFO_EN_ACCEL		(1<<3)
#define MPU9250_FIFO_EN_GYRO_ZOUT	(1<<4)
#define MPU9250_FIFO_EN_GYRO_YOUT	(1<<5)
#define MPU9250_FIFO_EN_GYRO_XOUT	(1<<6)
#define MPU9250_FIFO_EN_TEMP_OUT	(1<<7)
//
// I2C Master Control MPU9250_RA_I2C_MST_CTRL
#define MPU9250_MULT_MST_EN		(1<<7)
#define MPU9250_WAIT_FOR_ES		(1<<6)
#define MPU9250_SLV_3_FIFO_EN	(1<<5)
#define MPU9250_I2C_MST_P_NSR	(1<<4)
// I2C Master Control MPU9250_RA_I2C_MST_CTRL:I2C_MST_CLK [3:0]
#define MPU9250_I2C_MST_CLK_348		0
#define MPU9250_I2C_MST_CLK_333		1
#define MPU9250_I2C_MST_CLK_320		2
#define MPU9250_I2C_MST_CLK_308		3
#define MPU9250_I2C_MST_CLK_296		4
#define MPU9250_I2C_MST_CLK_286		5
#define MPU9250_I2C_MST_CLK_276		6
#define MPU9250_I2C_MST_CLK_267		7
#define MPU9250_I2C_MST_CLK_258		8
#define MPU9250_I2C_MST_CLK_500		9
#define MPU9250_I2C_MST_CLK_471		10
#define MPU9250_I2C_MST_CLK_444		11
#define MPU9250_I2C_MST_CLK_421		12
#define MPU9250_I2C_MST_CLK_400		13
#define MPU9250_I2C_MST_CLK_381		14
#define MPU9250_I2C_MST_CLK_364		15
//
// I2C_SLVx_ADDR
#define MPU9250_I2C_SLVx_RNW	(1<<7)
//
// I2C_SLVx_CTRL
#define MPU9250_I2C_SLVx_EN			(1<<7)
#define MPU9250_I2C_SLVx_BYTE_SW	(1<<6)
#define MPU9250_I2C_SLVx_REG_DIS	(1<<5)
#define MPU9250_I2C_SLVx_GRP		(1<<5)
#define MPU9250_I2C_SLVx_LENG		0x0F
//
// I2C_SLV4_CTRL
#define MPU9250_I2C_SLV4_EN				(1<<7)
#define MPU9250_I2C_SLV4_DONE_INT_EN	(1<<6)
#define MPU9250_I2C_SLV4_REG_DIS		(1<<5)
#define MPU9250_I2C_MST_DLY				0x1F
//
// I2C Master Status I2C_MST_STATUS
#define MPU9250_PASS_THROUGH		(1<<7)
#define MPU9250_I2C_SLV4_DONE		(1<<6)
#define MPU9250_I2C_LOST_ARB		(1<<5)
#define MPU9250_I2C_SLV4_NACK		(1<<4)
#define MPU9250_I2C_SLV3_NACK		(1<<3)
#define MPU9250_I2C_SLV2_NACK		(1<<2)
#define MPU9250_I2C_SLV1_NACK		(1<<1)
#define MPU9250_I2C_SLV0_NACK		(1<<0)
//
// INT Pin / Bypass Enable Configuration INT_PIN_CFG
#define MPU9250_ACTL				(1<<7)
#define MPU9250_OPEN				(1<<6)
#define MPU9250_LATCH_INT_EN		(1<<5)
#define MPU9250_INT_ANYRD_2CLEAR	(1<<4)
#define MPU9250_ACTL_FSYNC			(1<<3)
#define MPU9250_FSYNC_INT_MODE_EN	(1<<2)
#define MPU9250_BYPASS_EN			(1<<1)
//
// Interrupt Enable INT_ENABLE
#define MPU9250_WOM_EN				(1<<6)
#define MPU9250_FIFO_OVERFLOW_EN	(1<<4)
#define MPU9250_FSYNC_INT_EN		(1<<3)
#define MPU9250_RAW_RDY_EN			(1<<0)
//
// Interrupt Status INT_STATUS
#define MPU9250_WOM_INT				(1<<6)
#define MPU9250_FIFO_OVERFLOW_INT	(1<<4)
#define MPU9250_FSYNC_INT			(1<<3)
#define MPU9250_RAW_DATA_RDY_INT	(1<<0)
//
// I2C Master Delay Control I2C_MST_DELAY_CTRL
#define MPU9250_DELAY_ES_SHADOW		(1<<7)
#define MPU9250_I2C_SLV4_DLY_EN		(1<<4)
#define MPU9250_I2C_SLV3_DLY_EN		(1<<3)
#define MPU9250_I2C_SLV2_DLY_EN		(1<<2)
#define MPU9250_I2C_SLV1_DLY_EN		(1<<1)
#define MPU9250_I2C_SLV0_DLY_EN		(1<<0)
//
// Signal Path Reset SIGNAL_PATH_RESET
#define MPU9250_GYRO_RST			(1<<2)
#define MPU9250_ACCEL_RST			(1<<1)
#define MPU9250_TEMP_RST			(1<<0)
//
// Accelerometer Interrupt Control MOT_DETECT_CTRL
#define MPU9250_ACCEL_INTEL_EN		(1<<7)
#define MPU9250_ACCEL_INTEL_MODE	(1<<6)
//
// User Control USER_CTRL
#define MPU9250_FIFO_EN			(1<<6)
#define MPU9250_I2C_MST_EN		(1<<5)
#define MPU9250_I2C_IF_DIS		(1<<4)
#define MPU9250_FIFO_RST		(1<<2)
#define MPU9250_I2C_MST_RST		(1<<1)
#define MPU9250_SIG_COND_RST	(1<<0)
//
// Power Management 1 MPU9250_RA_PWR_MGMT_1
#define MPU9250_H_RESET			(1<<7)
#define MPU9250_SLEEP			(1<<6)
#define MPU9250_CYCLE			(1<<5)
#define MPU9250_GYRO_STANDBY	(1<<4)
#define MPU9250_PD_PTAT			(1<<3)
// Power Management 1 MPU9250_RA_PWR_MGMT_1:CLKSEL[2:0]
#define MPU9250_CLKSEL_INT20MHZ	0x00
#define MPU9250_CLKSEL_INTOSC	0x01
#define MPU9250_CLKSEL_STOP		0x07
//
// Power Management 2 PWR_MGMT_2
#define MPU9250_DISABLE_XA		(1<<5)
#define MPU9250_DISABLE_YA		(1<<4)
#define MPU9250_DISABLE_ZA		(1<<3)
#define MPU9250_DISABLE_XG		(1<<2)
#define MPU9250_DISABLE_YG		(1<<1)
#define MPU9250_DISABLE_ZG		(1<<0)
//
//
struct sMPU9250_regs 
{
	uint8 self_test_x_gyro;
	uint8 self_test_y_gyro;
	uint8 self_test_z_gyro;
	uint8 x_fine_gain;
	uint8 y_fine_gain;
	uint8 z_fine_gain;
	uint8 xa_offs_h;
	uint8 xa_offs_l_tc;
	uint8 ya_offs_h;
	uint8 ya_offs_l_tc;
	uint8 za_offs_h;
	uint8 za_offs_l_tc;
	uint8 none0C_12[7];
	uint8 xg_offs_usrh;
	uint8 xg_offs_usrl;
	uint8 yg_offs_usrh;
	uint8 yg_offs_usrl;
	uint8 zg_offs_usrh;
	uint8 zg_offs_usrl;
	uint8 smplrt_div;
	uint8 config;
	uint8 gyro_config;
	uint8 accel_config;
	uint8 accel_config2;
	uint8 accel_odr;
	uint8 wom_thr;
	uint8 mot_dur;
	uint8 zrmot_thr;
	uint8 zrmot_dur;
	uint8 fifo_en;
	uint8 i2c_mst_ctrl;
	uint8 i2c_slv0_addr;
	uint8 i2c_slv0_reg;
	uint8 i2c_slv0_ctrl;
	uint8 i2c_slv1_addr;
	uint8 i2c_slv1_reg;
	uint8 i2c_slv1_ctrl;
	uint8 i2c_slv2_addr;
	uint8 i2c_slv2_reg;
	uint8 i2c_slv2_ctrl;
	uint8 i2c_slv3_addr;
	uint8 i2c_slv3_reg;
	uint8 i2c_slv3_ctrl;
	uint8 i2c_slv4_addr;
	uint8 i2c_slv4_reg;
	uint8 i2c_slv4_do;
	uint8 i2c_slv4_ctrl;
	uint8 i2c_slv4_di;
	uint8 i2c_mst_status;
	uint8 int_pin_cfg;
	uint8 int_enable;
	uint8 dmp_int_status;
	uint8 int_status;
	uint8 accel_xout_h;
	uint8 accel_xout_l;
	uint8 accel_yout_h;
	uint8 accel_yout_l;
	uint8 accel_zout_h;
	uint8 accel_zout_l;
	uint8 temp_out_h;
	uint8 temp_out_l;
	uint8 gyro_xout_h;
	uint8 gyro_xout_l;
	uint8 gyro_yout_h;
	uint8 gyro_yout_l;
	uint8 gyro_zout_h;
	uint8 gyro_zout_l;
	uint8 ext_sens_data_00;
	uint8 ext_sens_data_01;
	uint8 ext_sens_data_02;
	uint8 ext_sens_data_03;
	uint8 ext_sens_data_04;
	uint8 ext_sens_data_05;
	uint8 ext_sens_data_06;
	uint8 ext_sens_data_07;
	uint8 ext_sens_data_08;
	uint8 ext_sens_data_09;
	uint8 ext_sens_data_10;
	uint8 ext_sens_data_11;
	uint8 ext_sens_data_12;
	uint8 ext_sens_data_13;
	uint8 ext_sens_data_14;
	uint8 ext_sens_data_15;
	uint8 ext_sens_data_16;
	uint8 ext_sens_data_17;
	uint8 ext_sens_data_18;
	uint8 ext_sens_data_19;
	uint8 ext_sens_data_20;
	uint8 ext_sens_data_21;
	uint8 ext_sens_data_22;
	uint8 ext_sens_data_23;
	uint8 mot_detect_status;
	uint8 none62;
	uint8 i2c_slv0_do;
	uint8 i2c_slv1_do;
	uint8 i2c_slv2_do;
	uint8 i2c_slv3_do;
	uint8 i2c_mst_delay_ctr;
	uint8 signal_path_reset;
	uint8 mot_detect_ctrl;
	uint8 user_ctrl;
	uint8 pwr_mgmt_1;
	uint8 pwr_mgmt_2;
	uint8 bank_sel;
	uint8 mem_start_addr;
	uint8 mem_r_w;
	uint8 dmp_cfg_1;
	uint8 dmp_cfg_2;
	uint8 fifo_counth;
	uint8 fifo_countl;
	uint8 fifo_r_w;
	uint8 who_am_i;
	uint8 none76;
	uint8 x_offset_h;
	uint8 x_offset_l;
	uint8 none79;
	uint8 y_offset_h;
	uint8 y_offset_l;
	uint8 none7C;
	uint8 z_offset_h;
	uint8 z_offset_l;
};

union uMPU9250_regs
{
	struct sMPU9250_regs r;
	uint8 uc[256];
};

extern sint32 kasa[3]; // mag_regs.asa{x,y,z}-128;

extern struct sAK8963_regs mag_regs;
//extern union uMPU9250_regs mpu_regs;

bool init_mpu9250(void);

#endif /* _MPU9250_H_ */
