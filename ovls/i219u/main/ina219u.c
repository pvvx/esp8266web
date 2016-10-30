/*
 * wdrv.c
 *
 *  Created on: 02 июля 2015 г.
 *      Author: PV`
 */
#include "user_interface.h"
#include "osapi.h"
#include "bios/ets.h"
#include "hw/esp8266.h"
#include "lwip/err.h"
#include "lwip/udp.h"
#include "sdk/mem_manager.h"
#include "sdk/rom2ram.h"
#include "udp_ina219.h"
#include "i2c_drv.h"
#include "ina219.h"
#ifdef USE_TIMER0
	#include "sdk/add_func.h"
#endif

#include "ovl_sys.h"

/*-----------------------------------------------------------------------------*/

uint32 wdrv_init_flg;

uint16 *out_wave_pbuf DATA_IRAM_ATTR;
uint32 wdrv_buf_wr_idx DATA_IRAM_ATTR;
ETSEvent wdrv_taskQueue[WDRV_TASK_QUEUE_LEN] DATA_IRAM_ATTR;
struct udp_pcb *pcb_wdrv DATA_IRAM_ATTR; // = NULL -> wdrv закрыт
#define wdrv_sample_rate ((uint32 *)(mdb_buf.ubuf))[70>>1]
#define wdrv_host_ip ((ip_addr_t *)mdb_buf.ubuf)[72>>1]
#define wdrv_host_port mdb_buf.ubuf[74]
#define wdrv_remote_port mdb_buf.ubuf[75]
#define wdrv_init_usr mdb_buf.ubuf[76]

#define KSHUNT	8192 // default 8192

#define ina_pin_sda	mdb_buf.ubuf[60]
#define ina_pin_scl	mdb_buf.ubuf[61]
#define ina_count	mdb_buf.ubuf[62]
#define ina_errflg	mdb_buf.ubuf[63]
#define ina_voltage	mdb_buf.ubuf[64] // Voltage LSB = 1mV per bit (step 4 mV)
#define ina_current	mdb_buf.ubuf[65] // Current LSB = 50uA per bit
#define ina_power	mdb_buf.ubuf[66] // Power LSB = 1mW per bit
#define ina_shunt	mdb_buf.ubuf[67] // 10uV 25uA

uint32 ina_init_flg DATA_IRAM_ATTR;
os_timer_t test_timer DATA_IRAM_ATTR;

#define ina_wr(a,b) i2c_wrword((INA219_ADDRESS<<24)|((a)<<16)|(b))
#define ina_rd(a,b) if(i2c_setaddr((INA219_ADDRESS<<8)|(a))) b = i2c_rdword(INA219_ADDRESS | INA219_READ)


unsigned int inafuncs DATA_IRAM_ATTR; // номер функции
unsigned int timeout DATA_IRAM_ATTR;

//-------------------------------------------------------------------------------
// writes a word
bool ICACHE_FLASH_ATTR i2c_wrword(uint32 w)
{
	i2c_start();
	int x = 3;
	do {
		int i = 7;
	    do {
	    	i2c_step_scl_sda(w & 0x80000000);
	    	w <<= 1;
	    } while(i--);
	    if(i2c_step_scl_sda(1) != 0) { //check ack
	    	i2c_stop();
	    	return false;
	    }
	} while(x--);
	i2c_stop();
	return true;
}

//-------------------------------------------------------------------------------
// set addr
bool ICACHE_FLASH_ATTR i2c_setaddr(uint32 addr)
{
	i2c_start();
	int x = 1;
	do {
		int i = 7;
	    do {
	    	i2c_step_scl_sda(addr & 0x8000);
	    	addr <<= 1;
	    } while(i--);
	    if(i2c_step_scl_sda(1) != 0) { //check ack
	    	i2c_stop();
	    	return false;
	    }
	} while(x--);
	i2c_stop();
	return true;
}
//-------------------------------------------------------------------------------
// read a word
uint32 ICACHE_FLASH_ATTR i2c_rdword(uint32 addr)
{
	int i = 7;
	i2c_start();
	do {
    	i2c_step_scl_sda(addr & 0x80);
    	addr <<= 1;
    } while(i--);
    i2c_step_scl_sda(0);
    uint32 w = 0;
    i = 15;
    do {
    	w <<= 1;
    	w |= i2c_step_scl_sda(1);
    	if(i == 8) i2c_step_scl_sda(0);
    } while(i--);
    i2c_step_scl_sda(1);
	i2c_stop();
	return w;
}

//----------------------------------------------------------------------------------
uint16 ICACHE_FLASH_ATTR IntReadIna219(uint32 num)
//----------------------------------------------------------------------------------
{
	switch(num & 1){
	case 0:
		ina_voltage = i2c_rdword(INA219_ADDRESS | INA219_READ) >> 1;
		i2c_setaddr((INA219_ADDRESS<<8)|INA219_REG_SHUNTVOLTAGE);
		return ina_voltage;
	default:
		ina_count++;
		ina_shunt = i2c_rdword(INA219_ADDRESS | INA219_READ);
		i2c_setaddr((INA219_ADDRESS<<8)|INA219_REG_BUSVOLTAGE);
		return ina_shunt;
	}
/*
	switch(num & 3){
	case 0:
		ina_voltage = i2c_rdword(INA219_ADDRESS | INA219_READ) >> 1;
		i2c_setaddr((INA219_ADDRESS<<8)|INA219_REG_POWER);
		return ina_voltage;
	case 1:
		ina_power = i2c_rdword(INA219_ADDRESS | INA219_READ);
		i2c_setaddr((INA219_ADDRESS<<8)|INA219_REG_CURRENT);
		return ina_power;
	case 2:
		ina_current = i2c_rdword(INA219_ADDRESS | INA219_READ);
		i2c_setaddr((INA219_ADDRESS<<8)|INA219_REG_SHUNTVOLTAGE);
		return ina_current; 
	default:
		ina_count++;
		ina_shunt = i2c_rdword(INA219_ADDRESS | INA219_READ);
		i2c_setaddr((INA219_ADDRESS<<8)|INA219_REG_BUSVOLTAGE);
		return ina_shunt;
	}
*/
}

/*-----------------------------------------------------------------------------
 Для использования NMI прерывания от аппаратного таймера раскомментировать
 в include\sdk\sdk_config.h :
 //#define USE_TIMER0
 //#define TIMER0_USE_NMI_VECTOR
 Предел при NMI примерно до 50 кГц из-за прервания обработки WiFi...
 ------------------------------------------------------------------------------*/
void timer0_isr(void)
{
	if(out_wave_pbuf != NULL) {
		out_wave_pbuf[wdrv_buf_wr_idx] = IntReadIna219(wdrv_buf_wr_idx);
		if(++wdrv_buf_wr_idx == (WDRV_OUT_BUF_SIZE >>1) || wdrv_buf_wr_idx == WDRV_OUT_BUF_SIZE) {
		// одна из половин буфера заполнена - данные готовы на передачу
//				system_os_post(WDRV_TASK_PRIO, WDRV_SIG_DATA, wdrv_buf_wr_idx);
				ets_post(WDRV_TASK_PRIO + SDK_TASK_PRIO, WDRV_SIG_DATA, wdrv_buf_wr_idx);
		}
		if(wdrv_buf_wr_idx >= WDRV_OUT_BUF_SIZE) wdrv_buf_wr_idx = 0;
	}
#ifndef USE_TIMER0
	TIMER0_INT = 0;
#endif
}
//----------------------------------------------------------------------------------
// Initialize Sensor
//----------------------------------------------------------------------------------
int ICACHE_FLASH_ATTR OpenINA219(void)
{
		if(ina_pin_scl > 15 || ina_pin_sda > 15 || ina_pin_scl == ina_pin_sda) { // return 1;
			ina_pin_scl = 4;
			ina_pin_sda = 5;
		}
		if(i2c_init(ina_pin_scl, ina_pin_sda, 54)) {	// (4,5,54); // 354
			ina_errflg = -3; // драйвер не инициализирован - ошибки параметров инициализации
			return 1;
		}
		i2c_stop();
		// 16V@40mV (G=1) -> 8192
		if( ina_wr(INA219_REG_CALIBRATION, KSHUNT) && ina_wr(INA219_REG_CONFIG,
					INA219_CONFIG_BVOLTAGERANGE_16V | // INA219_CONFIG_BVOLTAGERANGE_32V
					INA219_CONFIG_GAIN_1_40MV | // INA219_CONFIG_GAIN_8_320MV | //
					INA219_CONFIG_BADCRES_12BIT |
					INA219_CONFIG_SADCRES_12BIT_2S_1060US | // INA219_CONFIG_SADCRES_12BIT_128S_69MS |
					INA219_CONFIG_MODE_SANDBVOLT_CONTINUOUS)) {
			i2c_setaddr((INA219_ADDRESS<<8)|INA219_REG_BUSVOLTAGE);
			ina_count = -1;
	        ina_errflg = 0; // драйвер запущен
		    ina_init_flg = 1;
			return 0;
		}
        ina_errflg = -2; // неисправность
		return -2;
}

void wdrv_stop(void)
{
#ifdef USE_TIMER0
	timer0_stop();	
#else
	ets_isr_mask(BIT(ETS_FRC_TIMER0_INUM));
	INTC_EDGE_EN &= ~BIT(1);
#endif
	if(out_wave_pbuf != NULL) {
		os_free(out_wave_pbuf);
		out_wave_pbuf = NULL;
#if DEBUGSOO > 1
		os_printf("WDRV: stop()\n");
#endif
//		sar_off(); // отключить SAR
	}
}

bool ICACHE_FLASH_ATTR wdrv_start(uint32 sample_rate)
{
	if(pcb_wdrv != NULL) {
		wdrv_stop();
						
		
		if(sample_rate != 0 && sample_rate <= MAX_SAMPLE_RATE && OpenINA219() == 0)	{
			//wdrv_sample_rate = sample_rate;
			((uint32 *)(mdb_buf.ubuf))[70>>1] = sample_rate;
			out_wave_pbuf = os_malloc(WDRV_OUT_BUF_SIZE<<1);
			if(out_wave_pbuf != NULL) {
#if DEBUGSOO > 1
				os_printf("WDRV: start(%u)\n", sample_rate);
#endif
				wdrv_buf_wr_idx = 0;
				// включить TIMER0
#ifdef USE_TIMER0
#ifdef TIMER0_USE_NMI_VECTOR
				timer0_init(timer0_isr, 0, true);
#else
				timer0_init(timer0_isr, NULL);
#endif				
				timer0_start(8, true);
				TIMER0_LOAD = ((APB_CLK_FREQ>>4)+(sample_rate>>1))/sample_rate;
#else
				TIMER0_CTRL =   TM_DIVDED_BY_16
				              | TM_AUTO_RELOAD_CNT
				              | TM_ENABLE_TIMER
				              | TM_EDGE_INT;
				TIMER0_LOAD = ((APB_CLK_FREQ>>4)+(sample_rate>>1))/sample_rate;
				ets_isr_attach(ETS_FRC_TIMER0_INUM, timer0_isr, out_wave_pbuf);
				INTC_EDGE_EN |= BIT(1);
				ets_isr_unmask(BIT(ETS_FRC_TIMER0_INUM)); 
#endif				
				return true;
			}
		}
	}
#if DEBUGSOO > 1
	os_printf("WDRV: error start(%u)\n", sample_rate);
#endif
	return false;
}

void ICACHE_FLASH_ATTR wdrv_tx(uint32 sample_idx)
{
#if DEBUGSOO > 2
	os_printf("WDRV: txi(%u)\n", sample_idx>>10);
#endif
	if(pcb_wdrv == NULL ||out_wave_pbuf == NULL) return;
	void * pudpbuf;
	if(wdrv_buf_wr_idx >= (WDRV_OUT_BUF_SIZE>>1)) {
		pudpbuf = &out_wave_pbuf[0];
		if(sample_idx == WDRV_OUT_BUF_SIZE) {
#if DEBUGSOO > 1
			os_printf("WDRV: err tx\n");
#endif
			return;
		}
	}
	else {
		pudpbuf = &out_wave_pbuf[WDRV_OUT_BUF_SIZE>>1];
		if(sample_idx == (WDRV_OUT_BUF_SIZE>>1)) {
#if DEBUGSOO > 1
			os_printf("WDRV: err tx\n");
#endif
			return;
		}
	}
	struct pbuf *z = pbuf_alloc(PBUF_TRANSPORT, WDRV_OUT_BUF_SIZE, PBUF_RAM);
	if(z != NULL) {
    	err_t err = pbuf_take(z, pudpbuf, WDRV_OUT_BUF_SIZE);
    	if(err == ERR_OK) {
    	    udp_sendto(pcb_wdrv, z, &wdrv_host_ip, wdrv_host_port);
    	}
  	    pbuf_free(z);
    	return;
	}
}

extern uint32 ahextoul(uint8 *s);
static const char WDRV_ver_str[] ICACHE_RODATA_ATTR = "WDRV: 0.0.5";
static const char WDRV_error_str[] ICACHE_RODATA_ATTR = "WDRV: error";
static const char WDRV_stop_str[] ICACHE_RODATA_ATTR = "WDRV: stop";
static const char WDRV_freq_str[] ICACHE_RODATA_ATTR = "WDRV: freq %u Hz";
//static const char WDRV_start_str[] ICACHE_RODATA_ATTR = "WDRV: start";
static const char WDRV_ip_str[] ICACHE_RODATA_ATTR = "WDRV: set ip " IPSTR ", port %u";
#define mMIN(a, b)  ((a<b)?a:b)
void ICACHE_FLASH_ATTR wdrv_recv(void *arg, struct udp_pcb *upcb, struct pbuf *p,
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
    		udpbuflen = rom_xstrcpy(pudpbuf, WDRV_ver_str);
    		break;
    	case 'F':
    		if(length > 3 && usrdata[1]== '=') {
        		uint32 freq = ahextoul(&usrdata[2]);
        		if(freq > 0 && freq <= MAX_SAMPLE_RATE) {
        			wdrv_sample_rate = freq;
        			udpbuflen = ets_sprintf(pudpbuf, WDRV_freq_str, freq);
        		}
        		else {
        			udpbuflen = rom_xstrcpy(pudpbuf, WDRV_error_str);
        		}
    		}
    		else {
    			udpbuflen = rom_xstrcpy(pudpbuf, WDRV_error_str);
    		}
    		break;
    	case 'A':
    		wdrv_host_ip = *addr;
    		wdrv_host_port = port;
			udpbuflen = ets_sprintf(pudpbuf, WDRV_ip_str, IP2STR(&wdrv_host_ip), port);
    		break;
    	case 'S':
    		wdrv_stop();
    		udpbuflen = rom_xstrcpy(pudpbuf, WDRV_stop_str);
    		break;
    	case 'G':
    		wdrv_start(wdrv_sample_rate);
    		udpbuflen = rom_xstrcpy(pudpbuf, WDRV_stop_str);
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

int ICACHE_FLASH_ATTR wdrv_init(uint32 portn)
{
	wdrv_stop();
	if(pcb_wdrv != NULL) {
		udp_disconnect(pcb_wdrv);
		udp_remove(pcb_wdrv);
		pcb_wdrv = NULL;
	}
	if(portn != 0) {
		pcb_wdrv = udp_new();
		if(pcb_wdrv == NULL || (udp_bind(pcb_wdrv, IP_ADDR_ANY, portn) != ERR_OK)) {
#if DEBUGSOO > 0
			os_printf("WDRV: error init port %u\n", portn);
#endif
			udp_disconnect(pcb_wdrv);
			udp_remove(pcb_wdrv);
			pcb_wdrv = NULL;
			wdrv_init_usr = 0;
//			wdrv_remote_port = 0;
			return -1;
		}
#if DEBUGSOO > 1
		os_printf("WDRV: init port %u\n", portn);
#endif
		udp_recv(pcb_wdrv, wdrv_recv, pcb_wdrv);
	}
	else {
#if DEBUGSOO > 1
		os_printf("WDRV: close\n");
#endif
	}
	wdrv_remote_port = portn;
	wdrv_init_usr = 1;
	return 1;
}

void ICACHE_FLASH_ATTR task_wdrv(os_event_t *e)
{
    switch(e->sig) {
    	case WDRV_SIG_DATA:
    		wdrv_tx(e->par);
    		break;
    	case WDRV_SIG_INIT:
    		wdrv_init(e->par);
    		break;
    	case WDRV_SIG_STOP:
    		wdrv_stop();
    		break;
    	case WDRV_SIG_START:
    		wdrv_start(e->par);
    		break;
    }
}
/*
void init_wdrv(void)
{
	system_os_task(task_wdrv, WDRV_TASK_PRIO, wdrv_taskQueue, WDRV_TASK_QUEUE_LEN);
    wdrv_host_ip.addr = DEFAULT_WDRV_HOST_IP;
    wdrv_host_port = DEFAULT_WDRV_HOST_PORT;
    wdrv_sample_rate = DEFAULT_SAMPLE_RATE_HZ;
    wdrv_bufn = 8;
}
*/
void ICACHE_FLASH_ATTR close_wdrv(void)
{
	if(wdrv_init_flg) {
		wdrv_stop();
		if(pcb_wdrv != NULL) {
			udp_disconnect(pcb_wdrv);
			udp_remove(pcb_wdrv);
			pcb_wdrv = NULL;
		}
		ets_memset(&ets_tab_task[WDRV_TASK_PRIO + SDK_TASK_PRIO], 0 , sizeof(ss_task));
		wdrv_init_flg = 0;
		wdrv_init_usr = 0;
	}
}

//=============================================================================
//=============================================================================
int ovl_init(int flg)
{
int i = 0;
	switch(flg) {
		case 1:
			close_wdrv();
			if(wdrv_host_port == 0)  wdrv_host_port = DEFAULT_WDRV_HOST_PORT;
			if(wdrv_host_ip.addr == 0) wdrv_host_ip.addr = DEFAULT_WDRV_HOST_IP;
			if(wdrv_sample_rate == 0) wdrv_sample_rate = DEFAULT_SAMPLE_RATE_HZ;
			i = OpenINA219();
			if(i != 0) return i;
			ets_task(task_wdrv, WDRV_TASK_PRIO + SDK_TASK_PRIO, wdrv_taskQueue, WDRV_TASK_QUEUE_LEN);
			wdrv_init_flg = 1;
			if(wdrv_remote_port < 5) return wdrv_init(DEFAULT_WDRV_REMOTE_PORT);
			return wdrv_init(wdrv_remote_port);
		case 2:
			if(wdrv_init_flg) {
				wdrv_stop();
			}
			break;
		case 3:
		{
			ina_rd(INA219_REG_BUSVOLTAGE, i);
			return i;
		}
		case 4:
		{
			ina_rd(INA219_REG_SHUNTVOLTAGE, i);
			return i;
		}
		case 5:
			if(wdrv_init_flg) {
				return wdrv_start(wdrv_sample_rate);
			}
			break;
		default:
			switch(flg>>16) {
				case 1:
					ina_wr(INA219_REG_CALIBRATION, flg & 0xFFFF);
					return 0;
				case 2:
					ina_wr(INA219_REG_CONFIG, flg & 0xFFFF);
					return 0;
				default:
#if DEBUGSOO > 1
				os_printf("WDRV: close\n");
#endif
				close_wdrv();
			}
	}
	return 0;
}
