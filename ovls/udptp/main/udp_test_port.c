/******************************************************************************
 * PV` FileName: udp_test_port.c
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

#include "ovl_sys.h"

#define DEFAULT_UDP_TEST_PORT 1025

struct udp_pcb *udp_test_pcb DATA_IRAM_ATTR;

#define udpbufsize 1024

extern int rom_atoi(const char *);
#define atoi rom_atoi

#define udp_puts(fmt, ...) do { \
		udpbuflen += ets_sprintf((char *)&pudpbuf[udpbuflen], (char *)fmt, ##__VA_ARGS__); \
		} while(0)

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
unsigned int ICACHE_FLASH_ATTR
print_udp_psc(uint8 *pudpbuf, int max_size)
{
  struct udp_pcb *pcb;
  unsigned int udpbuflen = 0;
  if(max_size < 10 + 4) return udpbuflen;
  bool prt_none = true;
  udp_puts("UDP pcbs:\n");
  for(pcb = udp_pcbs; pcb != NULL; pcb = pcb->next) {
	  if(max_size - udpbuflen < 72) goto errrl;
	  udp_puts("flg:%02x\t" IPSTR ":%u\t" IPSTR ":%u\trecv:%08x\n", pcb->flags, IP2STR(&pcb->local_ip), pcb->local_port, IP2STR(&pcb->remote_ip), pcb->remote_port, pcb->recv );
	  prt_none = false;
  }
  if(prt_none) udp_puts("none\n");
  return udpbuflen;
 errrl:
  if(max_size - udpbuflen < 4) return udpbuflen;
  udp_puts("...\n");
  return udpbuflen;
}
/******************************************************************************
 * FunctionName : debug
 * Parameters   :
 * Returns      :
*******************************************************************************/
unsigned int ICACHE_FLASH_ATTR
print_tcp_psc(uint8 *pudpbuf, int max_size)
{
  struct tcp_pcb *pcb;
  unsigned int udpbuflen = 0;
  if(max_size < 20 + 4) return udpbuflen;
  udp_puts("Active PCB states:\n");
  bool prt_none = true;
  for(pcb = tcp_active_pcbs; pcb != NULL; pcb = pcb->next) {
     if(max_size - udpbuflen < 52) goto errrl;
     udp_puts("Port %u|%u\tflg:%02x\ttmr:%04x\t%s\n", pcb->local_port, pcb->remote_port, pcb->flags, pcb->tmr, tcp_state_str[pcb->state]);
     prt_none = false;
  }
  if(prt_none) udp_puts("none\n");
  if(max_size - udpbuflen < 20 + 4) goto errrl;
  udp_puts("Listen PCB states:\n");
  prt_none = true;
  for(pcb = (struct tcp_pcb *)tcp_listen_pcbs.pcbs; pcb != NULL; pcb = pcb->next) {
    if(max_size - udpbuflen < 52) goto errrl;
    udp_puts("Port %u|%u\tflg:%02x\ttmr:%04x\t%s\n", pcb->local_port, pcb->remote_port, pcb->flags, pcb->tmr, tcp_state_str[pcb->state]);
    prt_none = false;
  }
  if(prt_none) udp_puts("none\n");
  if(max_size - udpbuflen < 23 + 4) goto errrl;
  udp_puts("TIME-WAIT PCB states:\n");
  prt_none = true;
  for(pcb = tcp_tw_pcbs; pcb != NULL; pcb = pcb->next) {
    if(max_size - udpbuflen < 52) goto errrl;
    udp_puts("Port %u|%u\tflg:%02x\ttmr:%04x\t%s\n", pcb->local_port, pcb->remote_port, pcb->flags, pcb->tmr, tcp_state_str[pcb->state]);
    prt_none = false;
  }
  if(prt_none) udp_puts("none\n");
  return udpbuflen;
errrl:
  if(max_size - udpbuflen < 4) return udpbuflen;
  udp_puts("...\n");
  return udpbuflen;
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
unsigned int ICACHE_FLASH_ATTR chow_tcp_connection_info(uint8 *pudpbuf, int max_size)
{
        unsigned int udpbuflen = 0;
        if(max_size < 24 + 4) return udpbuflen;
        udp_puts("TCP Server connections:\n");
        TCP_SERV_CFG * p;
        TCP_SERV_CONN * ts_conn;
        bool prt_none = true;
        for(p  = phcfg; p != NULL; p = p->next) {
        	for(ts_conn  = p->conn_links; ts_conn != NULL; ts_conn = ts_conn->next) {
        		if(max_size - udpbuflen < 60) goto errrl;
        		udp_puts("%u "IPSTR ":%u %s\n", p->port, ts_conn->remote_ip.b[0], ts_conn->remote_ip.b[1], ts_conn->remote_ip.b[2], ts_conn->remote_ip.b[3], ts_conn->remote_port, msg_conns_state[ts_conn->state] );
        	    prt_none = false;
        	}
        }
        if(prt_none) udp_puts("none\n");
        return udpbuflen;
      errrl:
        if(max_size - udpbuflen < 4) return udpbuflen;
        udp_puts("...\n");
        return udpbuflen;
    return udpbuflen;
}

/******************************************************************************
 * FunctionName : udp_test_port_recv
 * Returns      : none
*******************************************************************************/
LOCAL void ICACHE_FLASH_ATTR
udp_test_port_recv(void *arg, struct udp_pcb *upcb, struct pbuf *p, ip_addr_t *addr, u16_t port)
{
	uint8 usrdata[32];
    if (p == NULL)  return;
    if(p->tot_len < 2) {
        pbuf_free(p);
    	return;
    }
    uint16 length = mMIN(p->tot_len, sizeof(usrdata)-1);
#if DEBUGSOO > 0
    os_printf("udp " IPSTR ":%u [%d]\n", IP2STR(addr), port, p->tot_len);
#endif
    length = pbuf_copy_partial(p, usrdata, length, 0);
    pbuf_free(p);

    uint8 *pudpbuf = (uint8 *)os_zalloc(udpbufsize+1);
    if(pudpbuf == NULL) return;
    uint16 udpbuflen = 0;
    int x = 0;
    if(length>2) x = atoi((char *)&usrdata[2]);
    if ((length>1)&&(usrdata[1]=='?')) switch(usrdata[0])
    {
      case 'M':
          system_print_meminfo();
      case 'A':
        {
          udp_puts("\nChip_id: %08x Flash_id: %08x\n", system_get_chip_id(), spi_flash_get_id());
          struct softap_config wiconfig;
          wifi_softap_get_config(&wiconfig);
          udp_puts("OPMode:%u SSID:'%s' Pwd:'%s' Ch:%u Authmode:%u MaxCon:%u Phu:%u ACon:%u\n", wifi_get_opmode(), wiconfig.ssid, wiconfig.password, wiconfig.channel, wiconfig.authmode, wiconfig.max_connection, wifi_get_phy_mode(), wifi_station_get_auto_connect());
          udp_puts("Connect status:%u, Station hostname:'%s'\n", wifi_station_get_connect_status(), wifi_station_get_hostname());
       	  struct station_config config[5];
       	  x = wifi_station_get_ap_info(config);
       	  udp_puts("APs recorded %u\n", x);
       	  int i;
       	  for(i = 0; i < x; i++) {
       		  udp_puts("AP[%u] SSID:'%s', PSW:'%s'\n", i, config[i].ssid, config[i].password);
       	  }
          udp_puts("heapsize: %d\n", system_get_free_heap_size() + udpbufsize);
          break;
        }
      case 'I':
          udpbuflen += print_udp_psc(pudpbuf+udpbuflen, udpbufsize-udpbuflen);
          udpbuflen += print_tcp_psc(pudpbuf+udpbuflen, udpbufsize-udpbuflen);
          udpbuflen += chow_tcp_connection_info(pudpbuf+udpbuflen, udpbufsize-udpbuflen);
          break;
      case 'H':
          udp_puts("heapsize: %d\n", system_get_free_heap_size() + udpbufsize);
          break;
      case 'U':
          udpbuflen = print_udp_psc(pudpbuf+udpbuflen, udpbufsize-udpbuflen);
          break;
      case 'T':
          udpbuflen = print_tcp_psc(pudpbuf+udpbuflen, udpbufsize-udpbuflen);
          break;
#ifdef USE_WEB
      case 'S':
          udpbuflen = chow_tcp_connection_info(pudpbuf+udpbuflen, udpbufsize-udpbuflen);
          break;
#endif
      case 'R':
          system_restart();
          break;
      case 'P':
          udp_puts("debug_print(%u)\n", x);
          system_set_os_print(x);
          break;
      case 'O':
          udp_puts("set_opmode(%u):%u\n", x, wifi_set_opmode(x));
          break;
      case 'B':
          udp_puts("st_set_auto_connect(%u):%u\n", x, wifi_station_set_auto_connect(x));
          break;
      case 'D':
          switch(x) {
          case 0:
              udp_puts("st_dhcpc_start:%u\n", wifi_station_dhcpc_start());
              break;
          case 1:
              udp_puts("st_dhcpc_stop:%u\n", wifi_station_dhcpc_stop());
              break;
          case 2:
              udp_puts("ap_dhcps_start:%u\n",wifi_softap_dhcps_start());
              break;
          case 3:
              udp_puts("ap_dhcps_stop:%u\n", wifi_softap_dhcps_stop());
              break;
          default:
              udp_puts("D(%u)?\n", x);
          }
          break;
          case 'G':
        	  udp_puts("g_ic = %p \n", &g_ic);
        	  break;
      default:
          udp_puts("???\n");
    }
    if(udpbuflen) {
    	struct pbuf *z = pbuf_alloc(PBUF_TRANSPORT, udpbuflen, PBUF_RAM);
    	if(z != NULL) {
        	err_t err = pbuf_take(z, pudpbuf, udpbuflen);
        	os_free(pudpbuf);
        	if(err == ERR_OK) {
        	    udp_sendto(upcb, z, addr, port);
        	}
      	    pbuf_free(z);
        	return;
    	}
    }
    os_free(pudpbuf);
}
/******************************************************************************
 * FunctionName : udp_test_port_init
 * if portn = 0 -> close
 * Returns      : none
*******************************************************************************/
int ICACHE_FLASH_ATTR udp_test_port_init(uint16 portn)
{
	mdb_buf.ubuf[91] = 0;
	if(udp_test_pcb != NULL) {
		if(udp_test_pcb->local_port == portn) {
#if DEBUGSOO > 2
			os_printf("UDP Test port %d already init!\n", portn);
#endif
			mdb_buf.ubuf[91] = 1;
			return 1;
		}
#if DEBUGSOO > 2
		os_printf("UDP Test port closed\n");
#endif
		udp_remove(udp_test_pcb);
		udp_test_pcb = NULL;
	}
	if(portn == 0) {
#if DEBUGSOO > 0
			os_printf("UDPT: close\n");
#endif
		return 0;
	}
	udp_test_pcb = udp_new();
	if(udp_test_pcb != NULL) {
		err_t err = udp_bind(udp_test_pcb, IP_ADDR_ANY, portn);
		if(err != ERR_OK) {
#if DEBUGSOO > 0
			os_printf("UDPT: Error bind\n");
#endif
			udp_remove(udp_test_pcb);
			mdb_buf.ubuf[91] = -1;
			return -1;
		};
		udp_recv(udp_test_pcb, udp_test_port_recv, udp_test_pcb);
	}
	else {
#if DEBUGSOO > 0
		os_printf("UDPT: Error mem\n");
#endif
		mdb_buf.ubuf[91] = -2;
		return -2;
	}
#if DEBUGSOO > 0
	os_printf("UDPT: init port %d\n", portn);
#endif
	mdb_buf.ubuf[91] = 1;
	return 0;
}

//=============================================================================
//=============================================================================
int ovl_init(int flg)
{
	if(flg == 1) {
	    if(mdb_buf.ubuf[90] == 0) mdb_buf.ubuf[90] = DEFAULT_UDP_TEST_PORT;
		return udp_test_port_init(mdb_buf.ubuf[90]);
	}
	else {
		return udp_test_port_init(0);
	}
}
