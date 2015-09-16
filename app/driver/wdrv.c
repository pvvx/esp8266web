/*
 * wdrv.c
 *
 *  Created on: 02 июля 2015 г.
 *      Author: PVV
 */
#include "user_interface.h"
#ifdef USE_WDRV
#include "osapi.h"
#include "bios/ets.h"
#include "hw/esp8266.h"
#include "lwip/err.h"
#include "lwip/udp.h"
#include "sdk/mem_manager.h"
#include "sdk/rom2ram.h"
#include "driver/wdrv.h"


#define i2c_bbpll							0x67 // 103
#define i2c_bbpll_en_audio_clock_out		4
#define i2c_bbpll_en_audio_clock_out_msb	7
#define i2c_bbpll_en_audio_clock_out_lsb	7
#define i2c_bbpll_hostid					4

#define i2c_saradc							0x6C // 108
#define i2c_saradc_hostid					2
#define i2c_saradc_en_test					0
#define i2c_saradc_en_test_msb				5
#define i2c_saradc_en_test_lsb				5

//extern int rom_i2c_writeReg_Mask(int block, int host_id, int reg_add, int Msb, int Lsb, int indata);
//extern int rom_i2c_readReg_Mask(int block, int host_id, int reg_add, int Msb, int Lsb);
//extern void read_sar_dout(uint16 * buf);
//i2c_writeReg_Mask_def(i2c_bbpll, i2c_bbpll_en_audio_clock_out, 1); //Enable clock to i2s subsystem

#define i2c_writeReg_Mask(block, host_id, reg_add, Msb, Lsb, indata) \
    rom_i2c_writeReg_Mask(block, host_id, reg_add, Msb, Lsb, indata)

#define i2c_readReg_Mask(block, host_id, reg_add, Msb, Lsb) \
    rom_i2c_readReg_Mask(block, host_id, reg_add, Msb, Lsb)

#define i2c_writeReg_Mask_def(block, reg_add, indata) \
    i2c_writeReg_Mask(block, block##_hostid, reg_add, reg_add##_msb, reg_add##_lsb, indata)

#define i2c_readReg_Mask_def(block, reg_add) \
    i2c_readReg_Mask(block, block##_hostid, reg_add, reg_add##_msb, reg_add##_lsb)

/*-----------------------------------------------------------------------------*/

uint16 *out_wave_pbuf DATA_IRAM_ATTR;
uint32 wdrv_buf_wr_idx DATA_IRAM_ATTR;
ETSEvent wdrv_taskQueue[WDRV_TASK_QUEUE_LEN] DATA_IRAM_ATTR;
struct udp_pcb *pcb_wdrv DATA_IRAM_ATTR;
uint32 wdrv_sample_rate DATA_IRAM_ATTR;
uint16 wdrv_host_port;
ip_addr_t wdrv_host_ip;

/*-----------------------------------------------------------------------------*/

void timer0_isr(void)
{
	TIMER0_INT = 0;
	if(out_wave_pbuf != NULL) {
		uint16 sardata[8];
	//	while((SAR_BASE[20] >> 24) & 0x07); // wait r_state == 0
		volatile uint32 * sar_regs = &SAR_BASE[32]; // считать 8 шт. значений SAR с адреса 0x60000D80
		int i;
		for(i = 0; i < 8; i++) sardata[i] = ~(*sar_regs++);
		// запуск нового замера SAR
		uint32 x = SAR_BASE[20] & (~(1 << 1));
		SAR_BASE[20] = x;
		SAR_BASE[20] = x | (1 << 1);
		if((wdrv_buf_wr_idx >> 31) == 0) {
			// если второе и последующее прерывание, данные в SAR готовы
			uint16 sar_dout = 0;
			for(i = 0; i < 8; i++) {
				// коррекция значений SAR под утечки и т.д.
				int x = sardata[i];
				int z = (x & 0xFF) - 21;
				x &= 0x700;
				if(z > 0) x = ((z * 279) >> 8) + x;
				sar_dout += x;
			}
			out_wave_pbuf[wdrv_buf_wr_idx++] = sar_dout; // 14 бит 0..16384
			if(wdrv_buf_wr_idx == (WDRV_OUT_BUF_SIZE >>1) || wdrv_buf_wr_idx == WDRV_OUT_BUF_SIZE) {
				system_os_post(WDRV_TASK_PRIO, WDRV_SIG_DATA, wdrv_buf_wr_idx);
			}
		}
		if(wdrv_buf_wr_idx >= WDRV_OUT_BUF_SIZE) wdrv_buf_wr_idx = 0;
	}
}

void ICACHE_FLASH_ATTR wdrv_stop(void)
{
	ets_isr_mask(BIT(ETS_FRC_TIMER0_INUM));
	INTC_EDGE_EN &= ~BIT(1);
	if(out_wave_pbuf != NULL) {
		os_free(out_wave_pbuf);
		out_wave_pbuf = NULL;
#if DEBUGSOO > 1
		os_printf("wdrv: stop()\n");
#endif
		// отключить SAR
    	i2c_writeReg_Mask_def(i2c_saradc, i2c_saradc_en_test, 0);
    	while((SAR_BASE[20] >> 24) & 0x07);
    	SAR_BASE[23] &= 1 << 21;
    	uint32 x = SAR_BASE[24] & (~1);
    	SAR_BASE[24] = x;
    	SAR_BASE[24] = x | 1;
	}
}

bool ICACHE_FLASH_ATTR wdrv_start(uint32 sample_rate)
{
	if(pcb_wdrv != NULL) {
		wdrv_stop();
		if(sample_rate <= 20000)	{
			if(sample_rate == 0) sample_rate = wdrv_sample_rate;
			else wdrv_sample_rate = sample_rate;
			out_wave_pbuf = os_malloc(WDRV_OUT_BUF_SIZE<<1);
			if(out_wave_pbuf != NULL) {
#if DEBUGSOO > 1
				os_printf("wdrv: start(%u)\n", sample_rate);
#endif
				wdrv_buf_wr_idx = 1<<31; // флаг для пропуска считывания SAR при первом прерывания
				// включить SAR
				i2c_writeReg_Mask_def(i2c_saradc, i2c_saradc_en_test, 1); //select test mux
				SAR_BASE[23] |= 1 << 21;
				while((SAR_BASE[20] >> 24) & 0x07);
				// включить TIMER0
				TIMER0_CTRL =   TM_DIVDED_BY_16
				              | TM_AUTO_RELOAD_CNT
				              | TM_ENABLE_TIMER
				              | TM_EDGE_INT;
				TIMER0_LOAD = ((APB_CLK_FREQ>>4)+(sample_rate>>1))/sample_rate;
				ets_isr_attach(ETS_FRC_TIMER0_INUM, timer0_isr, out_wave_pbuf);
				INTC_EDGE_EN |= BIT(1);
				ets_isr_unmask(BIT(ETS_FRC_TIMER0_INUM));
				return true;
			}
		}
	}
#if DEBUGSOO > 1
	os_printf("wdrv: error start(%u)\n", sample_rate);
#endif
	return false;
}

void ICACHE_FLASH_ATTR wdrv_tx(uint32 sample_idx)
{
#if DEBUGSOO > 1
	os_printf("wdrv: txi(%u)\n", sample_idx>>10);
#endif
	if(pcb_wdrv == NULL ||out_wave_pbuf == NULL) return;
	void * pudpbuf;
	if(wdrv_buf_wr_idx >= (WDRV_OUT_BUF_SIZE>>1)) {
		pudpbuf = &out_wave_pbuf[0];
		if(sample_idx == WDRV_OUT_BUF_SIZE) {
#if DEBUGSOO > 1
			os_printf("wdrv: err tx\n");
#endif
			return;
		}
	}
	else {
		pudpbuf = &out_wave_pbuf[WDRV_OUT_BUF_SIZE>>1];
		if(sample_idx == (WDRV_OUT_BUF_SIZE>>1)) {
#if DEBUGSOO > 1
			os_printf("wdrv: err tx\n");
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
static const char WDRV_ver_str[] ICACHE_RODATA_ATTR = "WDRV: 0.0.2";
static const char WDRV_error_str[] ICACHE_RODATA_ATTR = "WDRV: error";
static const char WDRV_stop_str[] ICACHE_RODATA_ATTR = "WDRV: stop";
static const char WDRV_start_str[] ICACHE_RODATA_ATTR = "WDRV: start, freg %u Hz";
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
        		if(wdrv_start(freq)) {
        			udpbuflen = ets_sprintf(pudpbuf, WDRV_start_str, freq);
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

bool ICACHE_FLASH_ATTR wdrv_init(uint32 portn)
{
	wdrv_stop();
	if(pcb_wdrv != NULL) {
		udp_disconnect(pcb_wdrv);
		udp_remove(pcb_wdrv);
	}
	pcb_wdrv = NULL;
	if(portn != 0) {
		pcb_wdrv = udp_new();
		if(pcb_wdrv == NULL || (udp_bind(pcb_wdrv, IP_ADDR_ANY, portn) != ERR_OK)) {
#if DEBUGSOO > 0
			os_printf("wdrv: error init port %u\n", portn);
#endif
			udp_disconnect(pcb_wdrv);
			udp_remove(pcb_wdrv);
			pcb_wdrv = NULL;
			return false;
		}
#if DEBUGSOO > 1
		os_printf("wdrv: init port %u\n", portn);
#endif
		udp_recv(pcb_wdrv, wdrv_recv, pcb_wdrv);
	}
	else {
#if DEBUGSOO > 1
		os_printf("wdrv: close\n");
#endif
	}
	wdrv_host_port = portn;
	return true;
}

void ICACHE_FLASH_ATTR task_wdrv(os_event_t *e){
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

void init_wdrv(void)
{
    system_os_task(task_wdrv, WDRV_TASK_PRIO, wdrv_taskQueue, WDRV_TASK_QUEUE_LEN);
    wdrv_host_ip.addr = DEFAULT_WDRV_HOST_IP;
    wdrv_sample_rate = DEFAULT_SAMPLE_RATE_HZ;
    wdrv_host_port = DEFAULT_WDRV_HOST_PORT;
    // wdrv_init(DEFAULT_WDRV_HOST_PORT);
}

#endif // USE_WDRV
