/******************************************************************************
 * PV` FileName: udp_debug_log.c
*******************************************************************************/

#include "user_config.h"


#include "c_types.h"
#include "bios.h"
#include "user_interface.h"
#include "osapi.h"
#include "lwip/err.h"
#include "arch/cc.h"
#include "lwip/mem.h"
#include "lwip/tcp.h"
#include "lwip/tcp_impl.h"
#include "lwip/udp.h"

#include "sdk/flash.h"
#include "sdk/add_func.h"

#include "tcp_srv_conn.h"
#include "sdk/libmain.h"
#include "sdk/app_main.h"
#include "sdk/rom2ram.h"

#include "ovl_sys.h"
#include "ovl_config.h"

#define UDP_BUF_SIZE 1024


uint8 *udpbuf DATA_IRAM_ATTR;
uint32 udpbufsize;
uint32 drv_init_flg DATA_IRAM_ATTR;
os_timer_t test_timer DATA_IRAM_ATTR;

struct udp_pcb *pcb_udp_drv DATA_IRAM_ATTR;

extern uint32 _lit4_start[]; // addr start BSS in IRAM
extern uint32 _lit4_end[]; // addr end BSS in IRAM

extern int rom_atoi(const char *);
#define atoi rom_atoi

#define mMIN(a, b)  ((a<b)?a:b)

const char * const tcp_state_str[] = {
  "CLOSED",
  "LISTEN",
  "SYN_SENT",
  "SYN_RCVD",
  "ESTABLISHED",
  "FIN_WAIT_1",
  "FIN_WAIT_2",
  "CLOSE_WAIT",
  "CLOSING",
  "LAST_ACK",
  "TIME_WAIT"
};

/******************************************************************************
 * FunctionName : debug
 * Parameters   :
 * Returns      :
*******************************************************************************/
static void print_udp_psc(void)
{
  struct udp_pcb *pcb;
  bool prt_none = true;
  os_printf("UDP pcbs:\n");
  for(pcb = udp_pcbs; pcb != NULL; pcb = pcb->next) {
	  os_printf("flg:%02x\t" IPSTR ":%u\t" IPSTR ":%u\trecv:%08x\n", pcb->flags, IP2STR(&pcb->local_ip), pcb->local_port, IP2STR(&pcb->remote_ip), pcb->remote_port, pcb->recv );
	  prt_none = false;
  }
  if(prt_none) os_printf("none\n");
}
/******************************************************************************
 * FunctionName : debug
 * Parameters   :
 * Returns      :
*******************************************************************************/
static void print_tcp_psc(void)
{
  struct tcp_pcb *pcb;
  os_printf("Active PCB states:\n");
  bool prt_none = true;
  for(pcb = tcp_active_pcbs; pcb != NULL; pcb = pcb->next) {
     os_printf("Port %u|%u\tflg:%02x\ttmr:%04x\t%s\n", pcb->local_port, pcb->remote_port, pcb->flags, pcb->tmr, tcp_state_str[pcb->state]);
     prt_none = false;
  }
  if(prt_none) os_printf("none\n");
  os_printf("Listen PCB states:\n");
  prt_none = true;
  for(pcb = (struct tcp_pcb *)tcp_listen_pcbs.pcbs; pcb != NULL; pcb = pcb->next) {
    os_printf("Port %u|%u\tflg:%02x\ttmr:%04x\t%s\n", pcb->local_port, pcb->remote_port, pcb->flags, pcb->tmr, tcp_state_str[pcb->state]);
    prt_none = false;
  }
  if(prt_none) os_printf("none\n");
  os_printf("TIME-WAIT PCB states:\n");
  prt_none = true;
  for(pcb = tcp_tw_pcbs; pcb != NULL; pcb = pcb->next) {
    os_printf("Port %u|%u\tflg:%02x\ttmr:%04x\t%s\n", pcb->local_port, pcb->remote_port, pcb->flags, pcb->tmr, tcp_state_str[pcb->state]);
    prt_none = false;
  }
  if(prt_none) os_printf("none\n");
}
/******************************************************************************
 * FunctionName : debug
 * Parameters   :
 * Returns      :
*******************************************************************************/
	static char *msg_conns_state[] =
        {
          "NONE",
          "CLOSEWAIT",
		  "CLIENT",
          "LISTEN",
          "CONNECT",
          "CLOSED"
        };
//------------------------------------------------------------------------------
static void chow_tcp_connection_info(void)
{
        os_printf("TCP Server connections:\n");
        TCP_SERV_CFG * p;
        TCP_SERV_CONN * ts_conn;
        bool prt_none = true;
        for(p  = phcfg; p != NULL; p = p->next) {
        	for(ts_conn  = p->conn_links; ts_conn != NULL; ts_conn = ts_conn->next) {
        		os_printf("%u "IPSTR ":%u %s\n", p->port, ts_conn->remote_ip.b[0], ts_conn->remote_ip.b[1], ts_conn->remote_ip.b[2], ts_conn->remote_ip.b[3], ts_conn->remote_port, msg_conns_state[ts_conn->state] );
        	    prt_none = false;
        	}
        }
        if(prt_none) os_printf("none\n");
}
/******************************************************************************
 * FunctionName : udp_test_port_recv
 * Returns      : none
*******************************************************************************/
LOCAL void ICACHE_FLASH_ATTR
udp_port_recv(void *arg, struct udp_pcb *upcb, struct pbuf *p, ip_addr_t *addr, u16_t port)
{
	uint8 usrdata[32];
    if (p == NULL)  return;
    if(p->tot_len < 2) {
        pbuf_free(p);
    	return;
    }
    uint16 length = mMIN(p->tot_len, sizeof(usrdata)-1);
    length = pbuf_copy_partial(p, usrdata, length, 0);
    pbuf_free(p);

    int x = 0;
    if(length>2) x = atoi((char *)&usrdata[2]);
    if ((length>1)&&(usrdata[1]=='?')) switch(usrdata[0])
    {
      case 'A':
		  drv_host_ip = *addr;
		  drv_host_port = port;
		  os_printf("DRV: set ip " IPSTR ", port %u\n", IP2STR(&drv_host_ip), drv_host_port);
		  break;
      case 'M':
          os_printf("System memory:\n");
          system_print_meminfo();
          os_printf("bssi  : 0x%x ~ 0x%x, len: %d\n", &_lit4_start, &_lit4_end, (uint32)(&_lit4_end) - (uint32)(&_lit4_start));
          os_printf("free  : 0x%x ~ 0x%x, len: %d\n", (uint32)(&_lit4_end), (uint32)(eraminfo.base) + eraminfo.size, (uint32)(eraminfo.base) + eraminfo.size - (uint32)(&_lit4_end));
          os_printf("heapsize: %d\n", system_get_free_heap_size());
          break;
      case 'W':
        {
          os_printf("\nChip_id: %08x Flash_id: %08x\n", system_get_chip_id(), spi_flash_get_id());
          struct softap_config wiconfig;
          wifi_softap_get_config(&wiconfig);
          os_printf("OPMode:%u SSID:'%s' Pwd:'%s' Ch:%u Authmode:%u MaxCon:%u Phu:%u ACon:%u\n", wifi_get_opmode(), wiconfig.ssid, wiconfig.password, wiconfig.channel, wiconfig.authmode, wiconfig.max_connection, wifi_get_phy_mode(), wifi_station_get_auto_connect());
          os_printf("Connect status:%u, Station hostname:'%s'\n", wifi_station_get_connect_status(), wifi_station_get_hostname());
       	  struct station_config config[5];
       	  x = wifi_station_get_ap_info(config);
       	  os_printf("APs recorded %u\n", x);
       	  int i;
       	  for(i = 0; i < x; i++) {
       		  os_printf("AP[%u] SSID:'%s', PSW:'%s'\n", i, config[i].ssid, config[i].password);
       	  }
          break;
        }
      case 'I':
          print_udp_psc();
          print_tcp_psc();
          chow_tcp_connection_info();
          break;
      case 'R':
          system_restart();
          break;
      case 'G':
       	  os_printf("g_ic = %p\n", &g_ic);
       	  break;
      default:
          os_printf("A,M,W,I,R,G.\n");
    }
}
//-------------------------------------------------------------------------------
//
//-------------------------------------------------------------------------------
void send_buf(void)
{
	if(udpbufsize) {
		struct pbuf *z = pbuf_alloc(PBUF_TRANSPORT, udpbufsize, PBUF_RAM);
		if(z != NULL) {
			err_t err = pbuf_take(z, udpbuf, udpbufsize);
			if(err == ERR_OK) udp_sendto(pcb_udp_drv, z, &drv_host_ip, drv_host_port);
			    pbuf_free(z);
		}
		udpbufsize = 0;
	}
}
//-------------------------------------------------------------------------------
//
//-------------------------------------------------------------------------------
void puts_buf(uint8 ch)
{
	if(udpbuf == NULL) return;
	if(udpbufsize < UDP_BUF_SIZE - 1) udpbuf[udpbufsize++] = ch;
	if(udpbufsize >= UDP_BUF_SIZE
	|| (udpbufsize > (UDP_BUF_SIZE/2) && ch == '\n')) send_buf();
	else if (ch == '\n') {
		ets_timer_arm_new(&test_timer, 10000, 0, 0); // 10 ms
	}
}
//-------------------------------------------------------------------------------
// init_udp_drv
//-------------------------------------------------------------------------------
bool init_udp_drv(void)
{
	if(drv_host_port == 0 || drv_host_ip.addr == 0) {
		drv_host_port = DEFAULT_UDP_PORT;
		drv_host_ip.addr = 0xFFFFFFFF;
	}
	pcb_udp_drv = udp_new();
	if(pcb_udp_drv == NULL || (udp_bind(pcb_udp_drv, IP_ADDR_ANY, drv_host_port) != ERR_OK)) {
		udp_remove(pcb_udp_drv);
		pcb_udp_drv = NULL;
		return false;
	}
	udp_recv(pcb_udp_drv, udp_port_recv, pcb_udp_drv);
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
//=============================================================================
int ovl_init(int flg)
{
	if(flg == 1) {
		if(drv_init_flg == 0) {
			drv_init_usr = 0;
			if(drv_host_port == 0) drv_host_port = DEFAULT_UDP_PORT;
			if(!init_udp_drv()) {
#if DEBUGSOO > 1
				os_printf("UDP-Log: Init UDP Error!\n");
#endif
				drv_error = -1;
			}
			else if((udpbuf = os_malloc(UDP_BUF_SIZE)) != NULL) {
				udpbufsize = 0;
				ets_install_putc1((void *)puts_buf); // install uart1 putc callback
				ets_timer_disarm(&test_timer);
				ets_timer_setfn(&test_timer, (os_timer_func_t *)send_buf, NULL);
#if DEBUGSOO > 1
				os_printf("UDP-Log: Init Ok\n");
#endif
				drv_init_usr = 1;
				drv_init_flg = 1;
				drv_error = 0;
			}
			else {
				close_udp_drv();
#if DEBUGSOO > 1
				os_printf("UDP-Log: Mem Error!\n");
#endif
				drv_error = -2;
			}
		}
		else drv_error = 1;
	}
	else /* if(flg == 0) */ {
		ets_timer_disarm(&test_timer);
		close_udp_drv();
		ets_install_putc1((void *)uart1_write_char); // install uart1 putc callback
		if(udpbuf != NULL) {
			os_free(udpbuf);
			udpbuf = NULL;
		}
#if DEBUGSOO > 1
		os_printf("UDP-Log: Close\n");
#endif
		drv_init_flg = 0;
		drv_init_usr = 0;
		drv_error = 2;
	}
	return drv_error;
}
