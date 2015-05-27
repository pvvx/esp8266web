/* tpm2net.c
* Author: PV` */

#include "user_config.h"
#if USE_TMP2NET_PORT
#include "c_types.h"
#include "bios.h"
#include "user_interface.h"
#include "osapi.h"
#include "add_sdk_func.h"
#include "lwip/err.h"
#include "arch/cc.h"
#include "lwip/mem.h"
#include "lwip/pbuf.h"
#include "lwip/udp.h"
#include "hw/esp8266.h"
#include "ws2812.h"

struct sframe_head {
    uint8 id;
    uint8 blocktype;
    uint16 framelength;
    uint8 packagenum;
    uint8 numpackages;
}  __attribute__((packed));


unsigned char framebuffer[1536]; //max 512 rgb pixels
uint16_t framebuffer_len;

#define mMIN(a, b)  ((a<b)?a:b)
#define MAX_FRAME_DATA_LEN mMIN(1536, sizeof(framebuffer))

#define PIN_WS2812 0 // GPIO0

#define GPIO_OUTPUT_SET(gpio_no, bit_value) gpio_output_set(bit_value<<gpio_no, ((~bit_value)&0x01)<<gpio_no, 1<<gpio_no,0)

void ICACHE_FLASH_ATTR SEND_WS_0()
{
	MEMW();	uint32 x = GPIO_OUT;
	x |= 1 << PIN_WS2812;
	int time = 4; while(time--) { // 0.35us
		MEMW();	GPIO_OUT = x;
	}
	x &= ~(1 << PIN_WS2812);
	time = 8; while(time--) { // 0.8us
		MEMW();	GPIO_OUT = x;
	}
}

void ICACHE_FLASH_ATTR SEND_WS_1()
{
	MEMW();	uint32 x = GPIO_OUT;
	x |= 1 << PIN_WS2812;
	int time = 9; while(time--) { // 0.7us
		MEMW(); GPIO_OUT = x;
	}
	x &= ~(1 << PIN_WS2812);
	time = 5; while(time--) { // 0.6us
		MEMW();	GPIO_OUT = x;
	}
}

void ICACHE_FLASH_ATTR ws2812_strip(uint8_t * buffer, uint16_t length)
{
	int i;
	os_intr_lock();
	MEMW();	WDT_FEED = WDT_FEED_MAGIC;
	MEMW();
	__asm__ __volatile__("extw");
	// max time (0.8+0.6) * 1536 us = 2150.4 us
	for( i = 0; i < length; i++ )
	{
		uint8_t mask = 0x80;
		uint8_t byte = buffer[i];
		while (mask) {
			( byte & mask ) ? SEND_WS_1() : SEND_WS_0();
			mask >>= 1;
       	}
	}
	MEMW();
	os_intr_unlock();
}

void ICACHE_FLASH_ATTR ws2812_init()
{
//	ets_wdt_disable(); No!!!
//	WDT_CTRL &= 0x7e; // Disable WDT
	GPIO_OUT &= ~(1 << PIN_WS2812);
	GPIO_ENABLE = 1 << PIN_WS2812;
	int time = 8; while(time--) { // fill fifo
		MEMW();	GPIO_OUT &= ~(1 << PIN_WS2812);;
	}
	uint8 outbuffer[] = { 0x00, 0x00, 0x00 };
	ws2812_strip( outbuffer, sizeof(outbuffer) );
}

void ICACHE_FLASH_ATTR tpm2net_recv(void *arg, struct udp_pcb *upcb, struct pbuf *p, ip_addr_t *addr, u16_t port)
{
    struct sframe_head fhead;
    if (p == NULL)  return;
    os_printf("\nUDP recved %u bytes\n", p->tot_len);
    if(p->tot_len > sizeof(fhead)) {
        if( pbuf_copy_partial(p, &fhead, sizeof(fhead), 0) == sizeof(fhead)) {
        	fhead.framelength = (fhead.framelength << 8) | (fhead.framelength >> 8);
            os_printf("Packet id = %02x, type = %02x, len = %u, pack = %u, num = %u\n", fhead.id, fhead.blocktype, fhead.framelength, fhead.packagenum, fhead.numpackages);
            if( fhead.id == 0x9C // header identifier (packet start)
                && fhead.blocktype == 0xDA // block type
                && fhead.framelength <= MAX_FRAME_DATA_LEN // check overflow
                && p->tot_len >= (fhead.framelength + sizeof(fhead) + 1) // input data len
                && pbuf_get_at(p, fhead.framelength + sizeof(fhead)) == 0x36 ) { // header end (packet stop)
                if (fhead.numpackages == 0x01) { // no frame split found
                    uint8 * fdata = (uint8 *) pvPortMalloc(fhead.framelength);
                    if(fdata != NULL) {
                        if(pbuf_copy_partial(p, fdata, fhead.framelength , sizeof(fhead)) == fhead.framelength) {
                        	os_printf("ws2812 out %u bytes\n", fhead.framelength);
                        	ws2812_strip(fdata, fhead.framelength); // send data to strip
                        }
                        else os_printf("Error copy\n");
                        vPortFree(fdata);
                    }
                    else os_printf("Error mem\n");
                }
                else { //frame split is found
                    if(framebuffer_len + fhead.framelength <= sizeof(framebuffer)) {
                        if(pbuf_copy_partial(p, &framebuffer[framebuffer_len], fhead.framelength , sizeof(fhead)) == fhead.framelength) {
                            framebuffer_len += fhead.framelength;
                            os_printf("ws2812 add buf %u bytes\n", fhead.framelength);
                            if (fhead.packagenum == fhead.numpackages) { // all packets found
                            	os_printf("ws2812 out %u bytes\n", framebuffer_len);
                            	ws2812_strip(framebuffer, framebuffer_len); // send data to strip
                            	framebuffer_len = 0;
                           };
                        }
                        else os_printf("Error copy\n");
                    }
                    else os_printf("Framebuffer overflow!\n");
                };
            }
            else os_printf("Error packet\n");
        }
        else os_printf("Error copy\n");
    }
    else os_printf("Error packet\n");
    pbuf_free(p);
}

void ICACHE_FLASH_ATTR tpm2net_init(void)
{
    struct udp_pcb *pcb;
    os_printf("\nInit TMP2NET... ");
    framebuffer_len = 0;
    pcb = udp_new();
    if(pcb != NULL) {
        err_t err = udp_bind(pcb, IP_ADDR_ANY, USE_TMP2NET_PORT);
        if(err == ERR_OK) {
        	udp_recv(pcb, tpm2net_recv, pcb);
            ws2812_init();
            os_printf("Ok\n");
        }
        else os_printf("Error bind\n");
    }
    else os_printf("Error mem\n");
}

#endif // USE_TMP2NET

