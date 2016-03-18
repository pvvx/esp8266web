#include "user_config.h"
#include "os_type.h"
#include "osapi.h"
#include "bios.h"
#include "hw/esp8266.h"
#include "sdk/add_func.h"
#include "sdk/rom2ram.h"
#include "sdk/os_printf.h"
#include "lwip/err.h"
#include "lwip/udp.h"

#include "hspi_master.h"
#include "bmp280.h"
#include "mpu9250.h"

#include "web_iohw.h"

#include "ovl_sys.h"
#include "ovl_config.h"

#define DRV_TASK_PRIO (USER_TASK_PRIO_1)

os_timer_t test_timer DATA_IRAM_ATTR;

#define timer_arg(pt,arg) do { \
	uint32 * ptimer_arg = (uint32 *)&pt.timer_arg; \
	*ptimer_arg = arg; \
	} while(0)


#if DEBUGSOO > 1
static uint32 tim_cnt DATA_IRAM_ATTR;
#endif
uint32 drv_init_flg DATA_IRAM_ATTR; // флаг инициализации
uint32 drv_udp_start DATA_IRAM_ATTR; // флаг старта передачи

tsblk_data *sblk_data DATA_IRAM_ATTR;
uint32 sample_number DATA_IRAM_ATTR;
uint32 data_blk_idx DATA_IRAM_ATTR;
struct udp_pcb *pcb_udp_drv DATA_IRAM_ATTR;

static const char DRV_ver_str[] ICACHE_RODATA_ATTR = "UDRV: 0.0.2";
static const char DRV_stop_str[] ICACHE_RODATA_ATTR = "UDRV: stop";
static const char DRV_start_str[] ICACHE_RODATA_ATTR = "UDRV: start";
static const char DRV_ip_str[] ICACHE_RODATA_ATTR = "UDRV: set ip " IPSTR ", port %u";
#define mMIN(a, b)  ((a<b)?a:b)

//-------------------------------------------------------------------------------
// drv_recv
//-------------------------------------------------------------------------------
void ICACHE_FLASH_ATTR  drv_recv(void *arg, struct udp_pcb *upcb, struct pbuf *p,
    ip_addr_t *addr, u16_t port)
{
	uint8 usrdata[32];
	uint8 pudpbuf[128];
	uint32 udpbuflen = 0;
	if (p == NULL)  return;
	if(p->tot_len < 1) {
		pbuf_free(p);
		return;
	}
	uint32 length = mMIN(p->tot_len, sizeof(usrdata)-1);
    length = pbuf_copy_partial(p, usrdata, length, 0);
    pbuf_free(p);
	usrdata[length] = 0;
    switch(usrdata[0])
    {
    	case '?':
    		udpbuflen = rom_xstrcpy(pudpbuf, DRV_ver_str);
    		break;
    	case 'A':
    		drv_host_ip = *addr;
    		drv_host_port = port;
			udpbuflen = ets_sprintf(pudpbuf, DRV_ip_str, IP2STR(&drv_host_ip), port);
    		break;
    	case 'S':
    		drv_udp_start = 0;
    		udpbuflen = rom_xstrcpy(pudpbuf, DRV_stop_str);
    		break;
    	case 'G':
    		drv_udp_start = 1;
    		udpbuflen = rom_xstrcpy(pudpbuf, DRV_start_str);
    		break;
    }
    if(udpbuflen) {
    	struct pbuf *z = pbuf_alloc(PBUF_TRANSPORT, udpbuflen, PBUF_RAM);
    	if(z != NULL) {
        	if(pbuf_take(z, pudpbuf, udpbuflen) == ERR_OK) {
        	    udp_sendto(upcb, z, addr, port);
        	}
      	    pbuf_free(z);
        	return;
    	}
    }
}
//-------------------------------------------------------------------------------
// test_timer_isr
//-------------------------------------------------------------------------------
void test_timer_isr(uint32 flg)
{
	struct __attribute__ ((packed)) {
		uint8 accel[6];
//		uint8 temp[2];
		uint8 gyro[6];
		uint8 mag[8];
	}fifo;
	uint8 uc[2];
	uint32 fifo_count;
	hspi_cmd_read(CS_MPU9250_PIN, SPI_CMD_READ | MPU9250_RA_FIFO_COUNTH , uc, 2);
	fifo_count = (uc[0]<<8) | uc[1];
	if(fifo_count == 0xFFFF) {
		drv_error = -6;
		return;
	}
	fifo_count /= sizeof(fifo);
	if(fifo_count) {
		if(data_blk_idx == 0) sblk_data->number = sample_number;
		fifo_count += data_blk_idx;
		if(fifo_count > MAX_TX_BLK_DATA) {
			fifo_count = MAX_TX_BLK_DATA;
		}
		for(; data_blk_idx < fifo_count; data_blk_idx++) {
			sample_number++;
			uint32 z = sizeof(fifo);
			uint8 * ptr = (uint8 *)&fifo;
			while(z--) {
				hspi_cmd_read(CS_MPU9250_PIN, SPI_CMD_READ | MPU9250_RA_FIFO_R_W, ptr++, 1);
	//			ets_delay_us(100);
			}
			el_sblk_data * pblk = &sblk_data->data[data_blk_idx];
			uint8 * des = (uint8 *)pblk->accel;
			ptr = (uint8 *)&fifo;
			des[0] = ptr[1];
			des[1] = ptr[0];
			des[2] = ptr[3];
			des[3] = ptr[2];
			des[4] = ptr[5];
			des[5] = ptr[4];
			des[6] = ptr[7];
			des[7] = ptr[6];
			des[8] = ptr[9];
			des[9] = ptr[8];
			des[10] = ptr[11];
			des[11] = ptr[10];
			ptr+=13;
			sint16 magx = (ptr[1] << 8) | ptr[0];
			sint16 magy = (ptr[3] << 8) | ptr[2];
			sint16 magz = (ptr[5] << 8) | ptr[4];
			pblk->mag[0] = (((sint32)magx * kasa[0])/256) + (sint32)magx;
			pblk->mag[1] = (((sint32)magy * kasa[1])/256) + (sint32)magy;
			pblk->mag[2] = (((sint32)magz * kasa[2])/256) + (sint32)magz;
		}
		if(data_blk_idx >= MAX_TX_BLK_DATA){
			hspi_cmd_read(CS_BMP280_PIN, BMP280_RA_STATUS | SPI_CMD_READ, (uint8 *)&bmp280.reg.status, BMP280_RA_TXLSB - BMP280_RA_STATUS + 1);
			drv_temp = sblk_data->temp = bmp280_compensate_T_int32(&bmp280) - 600;
			drv_press = sblk_data->press = bmp280_compensate_P_int32(&bmp280);
			if(drv_udp_start) {
				uint32 len = fifo_count * sizeof(el_sblk_data) + 2;
				struct pbuf *z = pbuf_alloc(PBUF_TRANSPORT, len, PBUF_RAM);
				if(z != NULL) {
			    	err_t err = pbuf_take(z, sblk_data, len);
			    	if(err == ERR_OK) udp_sendto(pcb_udp_drv, z, &drv_host_ip, drv_host_port);
			    	else drv_error = -7;
			  	    pbuf_free(z);
				}
				else drv_error = -8;
			}
			data_blk_idx = 0;
#if DEBUGSOO > 1
			if(++tim_cnt > 10) {
					tim_cnt = 0;
					os_printf("F(%u): %d,%d,", sblk_data->number, sblk_data->temp, sblk_data->press);
					os_printf("M(%d,%d,%d),", sblk_data->data[0].mag[0], sblk_data->data[0].mag[1], sblk_data->data[0].mag[2]);
					os_printf("A(%d,%d,%d),", sblk_data->data[0].accel[0], sblk_data->data[0].accel[1], sblk_data->data[0].accel[2]);
					os_printf("G(%d,%d,%d)\n", sblk_data->data[0].gyro[0], sblk_data->data[0].gyro[1], sblk_data->data[0].gyro[2]);
			}
#endif
		}
	}
}

//-------------------------------------------------------------------------------
// init_udp_drv
//-------------------------------------------------------------------------------
bool init_udp_drv(void)
{
	if(drv_host_port == 0 || drv_host_ip.addr == 0) {
		drv_host_port = 7777;
		drv_host_ip.addr = 0xFFFFFFFF;
	}
	pcb_udp_drv = udp_new();
	if(pcb_udp_drv == NULL || (udp_bind(pcb_udp_drv, IP_ADDR_ANY, drv_host_port) != ERR_OK)) {
		udp_remove(pcb_udp_drv);
		pcb_udp_drv = NULL;
		return false;
	}
	udp_recv(pcb_udp_drv, drv_recv, pcb_udp_drv);
	return true;
}

//-------------------------------------------------------------------------------
// close_udp_drv
//-------------------------------------------------------------------------------
void close_udp_drv(void)
{
	if(pcb_udp_drv != NULL) {
		udp_disconnect(pcb_udp_drv);
		udp_remove(pcb_udp_drv);
		pcb_udp_drv = NULL;
	}
}
//=============================================================================
// ovl_init
//=============================================================================
int ovl_init(int flg)
{
	if(flg == 1) {
		if(drv_init_flg == 0) {
			drv_init_usr = 0;
			sample_number = 0;
			data_blk_idx = 0;
			if(CS_BMP280_PIN == CS_MPU9250_PIN || CS_MPU9250_PIN > 15 || CS_BMP280_PIN > 15) {
				CS_BMP280_PIN = 5;
				CS_MPU9250_PIN = 4;
			}
			uint32 mskcs = (1<<CS_BMP280_PIN) | (1<<CS_MPU9250_PIN);
			GPIO_OUT_W1TS = mskcs;
			GPIO_ENABLE_W1TS = mskcs;
			set_gpiox_mux_func_ioport(CS_BMP280_PIN);	//	SET_PIN_FUNC_IOPORT(CS_BMP280_PIN);
			set_gpiox_mux_pull(CS_BMP280_PIN, 0);		//	SET_PIN_PULLUP_DIS(CS_BMP280_PIN);
			set_gpiox_mux_func_ioport(CS_MPU9250_PIN);	//	SET_PIN_FUNC_IOPORT(CS_MPU9250_PIN);
			set_gpiox_mux_pull(CS_MPU9250_PIN, 0);		//	SET_PIN_PULLUP_DIS(CS_MPU9250_PIN);
#if DEBUGSOO > 1
			os_printf("HSPI CLK = %u Hz\n", hspi_master_init(0x07070300,1000000)); // 8 bit addr, 8 bit cmd, CS none, SPI MODE 3, SETUP + no HOLD, 1 MHz
			os_printf("Init BMP280...\n");
#else
			hspi_master_init(0x07070300,1000000)); // 8 bit addr, 8 bit cmd, CS none, SPI MODE 3, SETUP + no HOLD, 1 MHz
#endif
			if(!init_bmp280()) {
#if DEBUGSOO > 1
				os_printf("BMP280 Error!\n");
#endif
				drv_error = -1;
				return drv_error;
			}
#if DEBUGSOO > 1
			os_printf("Init MPU9250...\n");
#endif
			if(!init_mpu9250()) {
#if DEBUGSOO > 1
				os_printf("MPU9250 Error!\n");
#endif
				drv_error = -2;
				return drv_error;
			}
			if(!init_udp_drv()) {
#if DEBUGSOO > 1
				os_printf("UDP Error!\n");
#endif
				drv_error = -3;
				return drv_error;
			}
			sblk_data = (tsblk_data *) os_malloc(sizeof(tsblk_data));
			if(sblk_data == NULL) {
				close_udp_drv();
#if DEBUGSOO > 1
				os_printf("Mem Error!\n");
#endif
				drv_error = -4;
				return drv_error;
			}
			ets_timer_disarm(&test_timer);
			ets_timer_setfn(&test_timer, (os_timer_func_t *)test_timer_isr, NULL);
			ets_timer_arm_new(&test_timer, 10000, 1, 0); // 10 ms, 100 раз в сек
			drv_init_flg = 1;
			drv_init_usr = 1;
			drv_error = 0;
		}
		else {
			drv_error = 1;
			return drv_error;
		}
	}
	else if(flg == 2) {
		if(drv_init_flg) {
			drv_udp_start = 1;
		}
		else drv_error = -5;
	}
	else if(flg == 3) {
		if(drv_init_flg) {
			drv_udp_start = 0;
		}
		else drv_error = -5;
	}
	else {
		ets_timer_disarm(&test_timer);
		close_udp_drv();
		if(sblk_data != NULL) {
			os_free(sblk_data);
			sblk_data = NULL;
		}
		drv_init_flg = 0; // ???
		drv_init_usr = 0;
		drv_error = 2;
	}
	return drv_error;
}
