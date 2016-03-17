/*
 * mpu9250.c
 *
 *  Created on: 30 янв. 2016 г.
 *      Author: PV`
 */
#include "user_config.h"
#include "bios.h"
#include "sdk/add_func.h"
#include "sdk/os_printf.h"
#include "os_type.h"
//#include "osapi.h"
#include "hspi_master.h"
#include "bmp280.h"
#include "mpu9250.h"

#include "ovl_sys.h"
#include "ovl_config.h"

#define INIT_BRK_PAUSE 0xFF
typedef enum {
	INIT_BRK_END = 0,
//	INIT_BRK_WAIT_RESET = 254,
	INIT_BRK_I2C_MAG = 249,
	INIT_BRK_I2C_READY,
	INIT_BRK_I2C_READY0,
	INIT_BRK_I2C_READY1,
	INIT_BRK_I2C_READY2,
	INIT_BRK_I2C_READY3,
	INIT_BRK_I2C_READY4
}EINIT_BRK;

struct smpu9250_config {
	uint8 addr;
	uint8 data;
} __attribute__ ((packed));

struct sAK8963_regs mag_regs;
//union uMPU9250_regs mpu_regs;

sint32 kasa[3] DATA_IRAM_ATTR; // mag_regs.asa{x,y,z}-128;

//-------------------------------------------------------------------------------
// init_mpu9250
//-------------------------------------------------------------------------------
struct smpu9250_config init_mpu9250_tab[] = {
		// read MPU9250 id
		{ SPI_CMD_READ | MPU9250_RA_WHO_AM_I, 1 }, // read 1 byte MPU9250 id
		// resest MPU9250
		{ MPU9250_RA_PWR_MGMT_1, MPU9250_H_RESET | MPU9250_CLKSEL_INT20MHZ }, // toggle reset device
		{ INIT_BRK_PAUSE, 10 }, // pause x ms
//		{ MPU9250_RA_PWR_MGMT_1, MPU9250_CLKSEL_INTOSC }, // toggle reset device
//		{ MPU9250_RA_PWR_MGMT_2, MPU9250_DISABLE_ZG }, // ?
//		{ INIT_BRK_PAUSE, 5 }, // pause x ms
//		{ MPU9250_RA_PWR_MGMT_1, MPU9250_CLKSEL_INT20MHZ },
//		{ MPU9250_RA_GYRO_CONFIG, 0},
//		{ MPU9250_RA_ACCEL_CONFIG, 0},


		{ MPU9250_RA_USER_CTRL,  MPU9250_I2C_IF_DIS | MPU9250_I2C_MST_RST | MPU9250_FIFO_RST | MPU9250_SIG_COND_RST },
//		{ INIT_BRK_PAUSE, 10 }, // pause x ms
//		{ SPI_CMD_READ | MPU9250_RA_PWR_MGMT_1, 1 }, // read 1 byte


//		{ MPU9250_RA_PWR_MGMT_1, MPU9250_CLKSEL_INTOSC }, // internal oscillator
		{ MPU9250_RA_CONFIG, MPU9250_DLPF_BW_184 }, // Configuration MPU9250_RA_CONFIG:DLPF_CFG[2:0] + FCHOICE

		// I2C config
//		{ MPU9250_RA_USER_CTRL, MPU9250_I2C_MST_EN | MPU9250_I2C_IF_DIS }, // I2C Master mode
//		{ INIT_BRK_PAUSE, 200 }, // pause x ms
		{ MPU9250_RA_USER_CTRL, MPU9250_I2C_MST_EN },
		{ MPU9250_RA_I2C_MST_CTRL, MPU9250_I2C_MST_CLK_400 }, // | MPU9250_I2C_MST_P_NSR | MPU9250_WAIT_FOR_ES }, // I2C 400 kHz WAIT_FOR_ES
		{ MPU9250_RA_I2C_MST_DELAY_CTRL, MPU9250_I2C_SLV0_DLY_EN | MPU9250_I2C_SLV1_DLY_EN | MPU9250_I2C_SLV2_DLY_EN | MPU9250_I2C_SLV3_DLY_EN },
		{ MPU9250_RA_INT_ENABLE, MPU9250_RAW_RDY_EN },
//		{ INIT_BRK_PAUSE, 20 }, // pause x ms


		// Read AK8963 ID
		{ MPU9250_RA_I2C_SLV4_ADDR, AK8963_ADDRESS | SPI_CMD_READ },
		{ MPU9250_RA_I2C_SLV4_REG, AK8963_RA_WIA },
		{ MPU9250_RA_I2C_SLV4_CTRL, MPU9250_I2C_SLV4_EN | MPU9250_I2C_SLV4_DONE_INT_EN | 0 },
		{ INIT_BRK_PAUSE, INIT_BRK_I2C_READY4 },
//		{ INIT_BRK_PAUSE, 10 }, // pause x ms
		{ SPI_CMD_READ | MPU9250_RA_I2C_SLV4_DI, 1 }, // read 1 byte

/*
		// Reset AK8963
		{ MPU9250_RA_I2C_SLV3_ADDR, AK8963_ADDRESS },
		{ MPU9250_RA_I2C_SLV3_REG, AK8963_RA_CNTL2 },
		{ MPU9250_RA_I2C_SLV3_DO, AK8963_CNTL2_SRST},
		{ MPU9250_RA_I2C_SLV3_CTRL, MPU9250_I2C_SLVx_EN + 1 },
		{ INIT_BRK_PAUSE, INIT_BRK_I2C_READY },
		{ INIT_BRK_PAUSE, 200 }, // pause x ms

		{ MPU9250_RA_I2C_SLV2_ADDR, AK8963_ADDRESS },
		{ MPU9250_RA_I2C_SLV2_REG, AK8963_RA_CNTL1 },
		{ MPU9250_RA_I2C_SLV2_DO, AK8963_CNTL1_PD_MODE | AK8963_CNTL1_16BITS },
		{ MPU9250_RA_I2C_SLV2_CTRL, MPU9250_I2C_SLVx_EN + 1 },
		{ INIT_BRK_PAUSE, INIT_BRK_I2C_READY },
*/
//		{ INIT_BRK_PAUSE, 20 }, // pause x ms
/*
		// Read all reg AK8963
		{ MPU9250_RA_I2C_SLV0_ADDR, AK8963_ADDRESS | SPI_CMD_READ },
		{ MPU9250_RA_I2C_SLV0_REG, AK8963_RA_WIA },
		{ MPU9250_RA_I2C_SLV0_CTRL, MPU9250_I2C_SLVx_EN + 15},
		{ INIT_BRK_PAUSE, INIT_BRK_I2C_READY },
		{ SPI_CMD_READ | MPU9250_RA_EXT_SENS_DATA_00, 15 }, // read 15 byte

		{ INIT_BRK_PAUSE, 200 }, // pause 2 ms
*/
/*
		// AK8963 selftest
		// set power down mode
		{ MPU9250_RA_I2C_SLV4_ADDR, AK8963_ADDRESS }, // AK8963_RA_CNTL1
		{ MPU9250_RA_I2C_SLV4_REG, AK8963_RA_CNTL1 },
		{ MPU9250_RA_I2C_SLV4_DO, AK8963_CNTL1_PD_MODE | AK8963_CNTL1_16BITS },
		{ MPU9250_RA_I2C_SLV4_CTRL, MPU9250_I2C_SLV4_EN | MPU9250_I2C_SLV4_DONE_INT_EN | 0 },
		{ INIT_BRK_PAUSE, INIT_BRK_I2C_READY4 },
		// set ASTC
		{ MPU9250_RA_I2C_SLV3_REG, AK8963_RA_ASTC },
		{ MPU9250_RA_I2C_SLV3_DO, AK8963_ASTC_SELF },
		{ MPU9250_RA_I2C_SLV3_CTRL, MPU9250_I2C_SLVx_EN + 1 },
		{ INIT_BRK_PAUSE, INIT_BRK_I2C_READY3 },
		{ MPU9250_RA_I2C_SLV3_CTRL, 0 },
		// start selftest
		{ MPU9250_RA_I2C_SLV4_DO, AK8963_CNTL1_ST_MODE | AK8963_CNTL1_16BITS },
		{ MPU9250_RA_I2C_SLV4_CTRL, MPU9250_I2C_SLV4_EN | MPU9250_I2C_SLV4_DONE_INT_EN | 0 },
		{ INIT_BRK_PAUSE, INIT_BRK_I2C_READY4 },
		// wait
		{ INIT_BRK_PAUSE, 10 }, // pause >4 ms
		// read
		{ MPU9250_RA_I2C_SLV0_ADDR, AK8963_ADDRESS | SPI_CMD_READ }, // AK8963_RA_WIA [15]
		{ MPU9250_RA_I2C_SLV0_REG, AK8963_RA_HXL },
		{ MPU9250_RA_I2C_SLV0_CTRL, MPU9250_I2C_SLVx_EN + 6},
		{ INIT_BRK_PAUSE, INIT_BRK_I2C_READY },
		{ MPU9250_RA_I2C_SLV0_CTRL, 6},
		{ SPI_CMD_READ | MPU9250_RA_EXT_SENS_DATA_00, 6 }, // read 6 byte
		// clear ASTC
		{ MPU9250_RA_I2C_SLV3_DO, 0 },
		{ MPU9250_RA_I2C_SLV3_CTRL, MPU9250_I2C_SLVx_EN + 1 },
		{ INIT_BRK_PAUSE, INIT_BRK_I2C_READY3 },
		{ MPU9250_RA_I2C_SLV3_CTRL, 0 },

//		{ MPU9250_RA_USER_CTRL, MPU9250_I2C_MST_EN | MPU9250_I2C_MST_RST}, // I2C Master mode
//		{ INIT_BRK_PAUSE, INIT_BRK_I2C_READY4 },
*/
		// AK8963 config
		//  Sensor is measured periodically in 100Hz 16 bits.
		// Reset AK8963
		{ MPU9250_RA_I2C_SLV4_ADDR, AK8963_ADDRESS },
		{ MPU9250_RA_I2C_SLV4_REG, AK8963_RA_CNTL2 },
		{ MPU9250_RA_I2C_SLV4_DO, AK8963_CNTL2_SRST},
		{ MPU9250_RA_I2C_SLV4_CTRL, MPU9250_I2C_SLV4_EN | MPU9250_I2C_SLV4_DONE_INT_EN | 1 },
		{ INIT_BRK_PAUSE, INIT_BRK_I2C_READY4 },
//		{ INIT_BRK_PAUSE, 2 }, // pause x ms
//		{ MPU9250_RA_I2C_SLV4_DO, 0 },
//		{ MPU9250_RA_I2C_SLV4_CTRL, MPU9250_I2C_SLV4_EN | MPU9250_I2C_SLV4_DONE_INT_EN | 1 },
//		{ INIT_BRK_PAUSE, INIT_BRK_I2C_READY4 },
		{ MPU9250_RA_I2C_SLV4_REG, AK8963_RA_CNTL1 },
//		{ MPU9250_RA_I2C_SLV4_DO, AK8963_CNTL1_PD_MODE | AK8963_CNTL1_16BITS },
//		{ MPU9250_RA_I2C_SLV4_CTRL, MPU9250_I2C_SLV4_EN | MPU9250_I2C_SLV4_DONE_INT_EN | 1 },
//		{ INIT_BRK_PAUSE, INIT_BRK_I2C_READY4 },
		{ MPU9250_RA_I2C_SLV4_DO, AK8963_CNTL1_CM1_MODE | AK8963_CNTL1_16BITS },
		{ MPU9250_RA_I2C_SLV4_CTRL, MPU9250_I2C_SLV4_EN | MPU9250_I2C_SLV4_DONE_INT_EN | 1 },
//		{ INIT_BRK_PAUSE, INIT_BRK_I2C_READY4 },
//		{ INIT_BRK_PAUSE, 10 }, // pause x ms

/*
		{ MPU9250_RA_I2C_SLV4_ADDR, AK8963_ADDRESS | SPI_CMD_READ }, // AK8963_RA_CNTL1
		{ MPU9250_RA_I2C_SLV4_REG, AK8963_RA_CNTL1 },
		{ MPU9250_RA_I2C_SLV4_CTRL, MPU9250_I2C_SLV4_EN | MPU9250_I2C_SLV4_DONE_INT_EN | 0 },
		{ INIT_BRK_PAUSE, INIT_BRK_I2C_READY4 },
		{ SPI_CMD_READ | MPU9250_RA_I2C_SLV4_DI, 1 }, // read 1 byte
*/
		{ MPU9250_RA_I2C_SLV0_ADDR, AK8963_ADDRESS | SPI_CMD_READ }, // AK8963_RA_WIA [15]
		{ MPU9250_RA_I2C_SLV0_REG, AK8963_RA_WIA },
		{ MPU9250_RA_I2C_SLV0_CTRL, MPU9250_I2C_SLVx_EN + 15},
//		{ INIT_BRK_PAUSE, INIT_BRK_I2C_READY },
//		{ INIT_BRK_PAUSE, 2 }, // pause x ms
		// read OTP
		{ MPU9250_RA_I2C_SLV1_ADDR, AK8963_ADDRESS | SPI_CMD_READ },
		{ MPU9250_RA_I2C_SLV1_REG, AK8963_RA_ASAX },
		{ MPU9250_RA_I2C_SLV1_CTRL, MPU9250_I2C_SLVx_EN + 3},
		{ INIT_BRK_PAUSE, 10 }, // pause x ms
		{ INIT_BRK_PAUSE, INIT_BRK_I2C_MAG },

		{ MPU9250_RA_I2C_SLV0_ADDR, AK8963_ADDRESS | SPI_CMD_READ }, // AK8963_RA_WIA [15]
		{ MPU9250_RA_I2C_SLV0_REG, AK8963_RA_ST1 },
		{ MPU9250_RA_I2C_SLV0_CTRL, MPU9250_I2C_SLVx_EN + 8},
		{ INIT_BRK_PAUSE, 10 }, // pause x ms
//		{ INIT_BRK_PAUSE, INIT_BRK_I2C_READY },
		{ SPI_CMD_READ | MPU9250_RA_EXT_SENS_DATA_00, 8 + 3 }, // read 3 byte
		{ MPU9250_RA_I2C_SLV1_CTRL, 0},
		//
		{ MPU9250_RA_USER_CTRL, MPU9250_FIFO_EN | MPU9250_I2C_MST_EN }, // Enable FIFO
		{ MPU9250_RA_FIFO_EN, /* MPU9250_FIFO_EN_TEMP_OUT
				|*/ MPU9250_FIFO_EN_GYRO_XOUT
				| MPU9250_FIFO_EN_GYRO_YOUT
				| MPU9250_FIFO_EN_GYRO_ZOUT
				| MPU9250_FIFO_EN_ACCEL
				| MPU9250_FIFO_EN_SLV_0 },
		{ MPU9250_RA_SMPLRT_DIV, 9 }, // 1000/(MPU9250_RA_SMPLRT_DIV+1) в сек
		{ INIT_BRK_PAUSE, INIT_BRK_END}
};
//=============================================================================
bool init_mpu9250(void)
{
	struct smpu9250_config * p = init_mpu9250_tab;
	uint8 buf[32];
	while(1) {
		if(p->addr == INIT_BRK_PAUSE) {
			if(p->data == INIT_BRK_I2C_READY) {
				int i = 0;
				do {
					hspi_cmd_read(CS_MPU9250_PIN, SPI_CMD_READ | MPU9250_RA_INT_STATUS, buf, 1);
					if(++i & 0x100) {
#if DEBUGSOO > 2
						os_printf("INT: %02x\n",  buf[0]);
#endif
						return false;
//						break;
					}
				} while ((buf[0] & MPU9250_RAW_DATA_RDY_INT) == 0);
				hspi_cmd_read(CS_MPU9250_PIN, SPI_CMD_READ | MPU9250_RA_I2C_MST_STATUS, &buf[1], 1);
#if DEBUGSOO > 2
				os_printf("INT: %02x %02x, %u\n",  buf[0], buf[1], i);
#endif
			}
			else if (p->data == INIT_BRK_I2C_MAG) {
				hspi_cmd_read(CS_MPU9250_PIN, SPI_CMD_READ | MPU9250_RA_EXT_SENS_DATA_00, (uint8 *)&mag_regs, sizeof(mag_regs));
				if(mag_regs.wia != AK8963_WIA_ID) return false;
#if DEBUGSOO > 1
				os_printf("Mag:");
				uint8 * ptr = (uint8 *)&mag_regs;
				int i = sizeof(mag_regs);
				while(i--)	os_printf(" %02x", *ptr++);
				os_printf("\n");
#endif
				kasa[0] = mag_regs.asax-128;
				kasa[1] = mag_regs.asay-128;
				kasa[2] = mag_regs.asaz-128;
			}
			else if (p->data > INIT_BRK_I2C_READY) {
				int i = 0;
				uint8 x = MPU9250_I2C_SLV4_DONE;
				if(p->data != INIT_BRK_I2C_READY4) x = 1<<(p->data - INIT_BRK_I2C_READY0);
				do {
					hspi_cmd_read(CS_MPU9250_PIN, SPI_CMD_READ | MPU9250_RA_I2C_MST_STATUS, buf, 1);
					if(++i & 0x100) {
#if DEBUGSOO > 2
						os_printf("MST: %02x\n",  buf[0]);
#endif
						return false;
//						break;
					}
				} while ((buf[0] & x)==0);
#if DEBUGSOO > 2
				os_printf("MST: %02x, %u\n",  buf[0], i);
#endif
			}
			else if (p->data == 0) return true;
			else ets_delay_us(p->data*1000);
		}
		else {
			if(p->addr & SPI_CMD_READ) {
				hspi_cmd_read(CS_MPU9250_PIN, p->addr, buf, p->data);
//				ets_delay_us(1000);
#if DEBUGSOO > 2
				os_printf("R%02x(%u):", p->addr & (~(SPI_CMD_READ)), p->addr & (~(SPI_CMD_READ)));
				uint8 * ptr = buf;
				int i = p->data;
				while(i--)	os_printf(" %02x", *ptr++);
				os_printf("\n");
#endif
			}
			else {
				hspi_cmd_write(CS_MPU9250_PIN, p->addr, &p->data, 1);
				ets_delay_us(200); // 1000
			}
		}
		p++;
	}
}


