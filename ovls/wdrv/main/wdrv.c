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
#include "wdrv.h"
#include "driver/adc.h"
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
//uint32 wdrv_sample_rate DATA_IRAM_ATTR;
//uint32 wdrv_remote_port DATA_IRAM_ATTR; // = 0 -> wdrv не используется
//ip_addr_t wdrv_host_ip;
// uint16 wdrv_host_port;
#define wdrv_sample_rate ((uint32 *)(mdb_buf.ubuf))[70>>1]
#define wdrv_host_ip ((ip_addr_t *)mdb_buf.ubuf)[72>>1]
#define wdrv_host_port mdb_buf.ubuf[74]
#define wdrv_remote_port mdb_buf.ubuf[75]
#define wdrv_init_usr mdb_buf.ubuf[76]

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
		uint16 sardata[8];
		volatile uint32 * sar_regs = &SAR_DATA; // указатель для считывания до 8 шт. накопленных
												// значений SAR из аппаратного буфера в 0x60000D80
		int i;
		for(i = 0; i < wdrv_bufn; i++) sardata[i] = ~(*sar_regs++); // скопировать накопленные значения
		// запуск нового замера SAR
		uint32 x = SAR_CFG & (~(1 << 1));
		SAR_CFG = x;
		SAR_CFG = x | (1 << 1);
		// запись значениями SAR в один из двух половин буфера в памяти
		if((wdrv_buf_wr_idx >> 31) == 0) {
			// если второе и последующее прерывание, данные в SAR готовы
			uint16 sar_dout = 0;
			for(i = 0; i < wdrv_bufn; i++) {
				// коррекция значений SAR под утечки и т.д.
				int x = sardata[i];
				int z = (x & 0xFF) - 21;
				x &= 0x700;
				if(z > 0) x = ((z * 279) >> 8) + x;
				sar_dout += x;
			}
			out_wave_pbuf[wdrv_buf_wr_idx++] = sar_dout; 
			if(wdrv_buf_wr_idx == (WDRV_OUT_BUF_SIZE >>1) || wdrv_buf_wr_idx == WDRV_OUT_BUF_SIZE) {
				// одна из половин буфера заполнена - данные готовы на передачу
//				system_os_post(WDRV_TASK_PRIO, WDRV_SIG_DATA, wdrv_buf_wr_idx);
				ets_post(WDRV_TASK_PRIO + SDK_TASK_PRIO, WDRV_SIG_DATA, wdrv_buf_wr_idx);
			}
		}
		if(wdrv_buf_wr_idx >= WDRV_OUT_BUF_SIZE) wdrv_buf_wr_idx = 0;
	}
#ifndef USE_TIMER0
	TIMER0_INT = 0;
#endif
}

void ICACHE_FLASH_ATTR wdrv_stop(void)
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
		sar_off(); // отключить SAR
	}
}

bool ICACHE_FLASH_ATTR wdrv_start(uint32 sample_rate)
{
	if(pcb_wdrv != NULL) {
		wdrv_stop();
		if(sample_rate != 0 && sample_rate <= MAX_SAMPLE_RATE)	{
			//wdrv_sample_rate = sample_rate;
			((uint32 *)(mdb_buf.ubuf))[70>>1] = sample_rate;
			out_wave_pbuf = os_malloc(WDRV_OUT_BUF_SIZE<<1);
			if(out_wave_pbuf != NULL) {
#if DEBUGSOO > 1
				os_printf("WDRV: start(%u)\n", sample_rate);
#endif
				wdrv_buf_wr_idx = 1<<31; // флаг для пропуска считывания SAR при первом прерывания
				sar_init(8, SAR_SAMPLE_RATE / sample_rate);
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
void ICACHE_FLASH_ATTR  wdrv_recv(void *arg, struct udp_pcb *upcb, struct pbuf *p,
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
//	if(wdrv_init_flg) {
//		if(wdrv_remote_port == portn) return 0;
	wdrv_stop();
//	}
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
void close_wdrv(void)
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
	switch(flg) {
		case 1:
			close_wdrv();
			if(wdrv_host_port == 0)  wdrv_host_port = DEFAULT_WDRV_HOST_PORT;
			if(wdrv_host_ip.addr == 0) wdrv_host_ip.addr = DEFAULT_WDRV_HOST_IP;
			if(wdrv_sample_rate == 0) {
				wdrv_sample_rate = DEFAULT_SAMPLE_RATE_HZ;
	//		    wdrv_bufn = 8;
			}
			ets_task(task_wdrv, WDRV_TASK_PRIO + SDK_TASK_PRIO, wdrv_taskQueue, WDRV_TASK_QUEUE_LEN);
			wdrv_init_flg = 1;
			if(wdrv_remote_port < 5) return wdrv_init(DEFAULT_WDRV_REMOTE_PORT);
			return wdrv_init(wdrv_remote_port);
		case 2:
			if(wdrv_init_flg) {
				return wdrv_start(wdrv_sample_rate);
			}
			break;
		case 3:
			if(wdrv_init_flg) {
				wdrv_stop();
			}
			break;
		default:
#if DEBUGSOO > 1
				os_printf("WDRV: close\n");
#endif
				close_wdrv();
	}
	return 0;
}


