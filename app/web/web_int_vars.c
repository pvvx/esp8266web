/******************************************************************************
 * FileName: webserver.c
 * Description: The web server mode configuration.
*******************************************************************************/

#include "user_config.h"
#ifdef USE_WEB
#include "bios.h"
#include "sdk/add_func.h"
#include "hw/esp8266.h"
#include "hw/eagle_soc.h"
#include "hw/uart_register.h"
#include "os_type.h"
#include "osapi.h"
#include "user_interface.h"

#include "lwip/ip.h"
#include "lwip/ip_addr.h"
#include "lwip/tcp.h"

#include "tcp_srv_conn.h"
#include "web_srv_int.h"
#include "web_utils.h"
#include "wifi.h"
#include "flash_eep.h"
#include "driver/sigma_delta.h"
#include "sys_const_utils.h"
#include "sdk/rom2ram.h"
#include "sdk/app_main.h"
#include "tcp2uart.h"
#include "web_iohw.h"
#include "wifi_events.h"
//#include "sdk/app_main.h"

#ifdef USE_NETBIOS
#include "netbios.h"
#endif

#ifdef USE_SNTP
#include "sntp.h"
#endif

#ifdef USE_LWIP_PING
#include "lwip/app/ping.h"
struct ping_option pingopt; // for test
#endif

#ifdef USE_CAPTDNS
#include "captdns.h"
#endif

#ifdef USE_MODBUS
#include "modbustcp.h"
#include "mdbtab.h"
#endif

#ifdef USE_RS485DRV
#include "driver/rs485drv.h"
#include "mdbrs485.h"
#endif

#ifdef USE_OVERLAY
#include "overlay.h"
#endif

extern int rom_atoi(const char *);
#define atoi rom_atoi

typedef uint32 (* call_func)(uint32 a, uint32 b, uint32 c);

void ICACHE_FLASH_ATTR reg_sct_bits(volatile uint32 * addr, uint32 bits, uint32 val)
{
	uint32 x = *addr;
	if(val == 3) x ^= bits;
	else if(val) x |= bits;
	else x &= ~ bits;
	*addr =  x;
}
#ifdef USE_TIMER0
// тест
void timer0_tst_isr(void *arg)
{
	GPIO_OUT = GPIO_OUT^1;
	GPIO_OUT = GPIO_OUT^1;
//	uart0_write_char(((uint32)arg == 0)? '*' : '@');
}
#endif

/******************************************************************************
 * FunctionName : go_deep_sleep
 * Description  : system_deep_sleep
 * Parameters   : time_us
 * Returns      : none
*******************************************************************************/
void ICACHE_FLASH_ATTR go_deep_sleep(uint32 time_us)
{
	close_all_service();
/*
	uart0_set_flow(0);
	GPIO12_MUX  = 0x80;
	GPIO13_MUX  = 0x80;
	GPIO14_MUX  = 0x80;
	GPIO15_MUX  = 0x80;
	GPIO_STATUS = 0;
	GPIO_OUT = 0;
	UART0_CONF0 = UART0_REGCONFIG0DEF;
	UART0_CONF1 = ((0x01 & UART_RXFIFO_FULL_THRHD) << UART_RXFIFO_FULL_THRHD_S)
		| ((0x01 & UART_TXFIFO_EMPTY_THRHD) << UART_TXFIFO_EMPTY_THRHD_S)
		| (((128 - RST_FIFO_CNT_SET) & UART_RX_FLOW_THRHD) << UART_RX_FLOW_THRHD_S)
		| ((0x04 & UART_RX_TOUT_THRHD) << UART_RX_TOUT_THRHD_S) // | UART_RX_TOUT_EN
	;
*/
	system_deep_sleep(time_us);
}
/******************************************************************************
 * FunctionName : parse_url
 * Description  : parse the received data from the server
 * Parameters   : CurHTTP -- the result of parsing the url
 * Returns      : none
*******************************************************************************/
// #define ifcmp(a)  if(!os_memcmp((void*)cstr, a, sizeof(a)))
#define ifcmp(a)  if(rom_xstrcmp(cstr, a))

void ICACHE_FLASH_ATTR web_int_vars(TCP_SERV_CONN *ts_conn, uint8 *pcmd, uint8 *pvar)
{
    WEB_SRV_CONN *web_conn = (WEB_SRV_CONN *)ts_conn->linkd;
	uint32 val = ahextoul(pvar);
	char *cstr = pcmd;
#if DEBUGSOO > 1
    os_printf("[%s=%s]\n", pcmd, pvar);
#endif
	ifcmp("start") 		web_conn->udata_start = val;
	else ifcmp("stop") 	web_conn->udata_stop = val;
#ifdef USE_MODBUS
	else ifcmp("mdb") {
		cstr+=3;
		if((cstr[1]>='0')&&(cstr[1]<='9')) {
    		cstr++;
    		uint32 addr = ahextoul(cstr);
        	if(addr < 0x10000) {
        		if(cstr[-1]=='w') {
        			uint16 buf[32];
        			os_memset(buf,0,sizeof(buf));
        			WrMdbData((uint8 *)&buf, addr, str_array_w(pvar, buf, 32));
        		}
        		else if(cstr[-1]=='d') {
        			uint32 buf[32];
        			os_memset(buf,0,sizeof(buf));
        			WrMdbData((uint8 *)&buf, addr, str_array(pvar, buf, 32) << 1);
        		}
        		else if(cstr[-1]=='b') {
        			uint8 buf[32];
        			os_memset(buf,0,sizeof(buf));
        			WrMdbData((uint8 *)&buf, addr, str_array_b(pvar, buf, 32));
        		}
        		else if(cstr[-1]=='a') {
        			uint32 i = 0;
        			while(i < 32) {
        				val = (pvar[i+1]<< 8) + pvar[i];
        				if(WrMdbData((uint8 *)&val, addr++, 1) != 0) break;
        				if(pvar[i] == 0 ||  pvar[i+1] == 0) break;
        				i+=2;
        			}
        		}
#if defined(USE_RS485DRV) && defined(MDB_RS485_MASTER)
        		else if(cstr[-1]=='t' && addr < MDB_TRN_MAX) { // && rs485cfg.flg.b.master == 1) {
        			str_array_w(pvar, &mdb_buf.trn[addr].id, 9);
           			mdb_start_trn(addr);
        		}
#endif
        	}
    	}
		else ifcmp("fini") mbd_fini(pvar);
	}
#endif
	else ifcmp("sys_") {
		cstr+=4;
		ifcmp("restart") {
			if(val == 12345) web_conn->web_disc_cb = (web_func_disc_cb)system_restart;
		}
		else ifcmp("reset") {
			if(val == 12345) web_conn->web_disc_cb = (web_func_disc_cb)_ResetVector;
		}
		else ifcmp("ram") { uint32 *ptr = (uint32 *)(ahextoul(cstr+3)&(~3)); str_array(pvar, ptr, 32); }
		else ifcmp("debug") 	system_set_os_print(val);
#ifdef USE_LWIP_PING
		else ifcmp("ping") {
//			struct ping_option *pingopt = (struct ping_option *)UartDev.rcv_buff.pRcvMsgBuff;
			pingopt.ip = ipaddr_addr(pvar);
			pingopt.count = 3;
			pingopt.recv_function=NULL;
			pingopt.sent_function=NULL;
			ping_start(&pingopt);
		}
#endif
		else ifcmp("sleep_") {
			cstr += 6;
			ifcmp("option") 	system_deep_sleep_set_option(val);
			else ifcmp("us") {
				web_conn->web_disc_cb = (web_func_disc_cb)go_deep_sleep;
				web_conn->web_disc_par = val;
			}
#if DEBUGSOO > 5
			else os_printf(" - none!\n");
#endif
		}
		else ifcmp("const_")	write_sys_const(ahextoul(cstr+6), val);
		else ifcmp("ucnst_") 	write_user_const(ahextoul(cstr+6), val);
		else ifcmp("clkcpu") {
			if(val > 80) {
				CLK_PRE_PORT |= 1;
				os_update_cpu_frequency(160);
			}
			else
			{
				CLK_PRE_PORT &= ~1;
				os_update_cpu_frequency(80);
			}
		}
#if DEBUGSOO > 5
		else os_printf(" - none!\n");
#endif
    }
	else ifcmp("cfg_") {
		cstr += 4;
		ifcmp("web_") {
			cstr += 4;
			ifcmp("port") {
				if(syscfg.web_port != val) {
	        		web_conn->web_disc_par = syscfg.web_port; // ts_conn->pcfg->port
					syscfg.web_port = val;
	    			web_conn->web_disc_cb = (web_func_disc_cb)webserver_reinit;
				}
			}
			else ifcmp("twd") {
				if(val) {
					syscfg.cfg.b.web_time_wait_delete = 1;
					ts_conn->pcfg->flag.pcb_time_wait_free = 1;
				}
				else {
					syscfg.cfg.b.web_time_wait_delete = 0;
					ts_conn->pcfg->flag.pcb_time_wait_free = 0;
				}
			}
	   		else ifcmp("twrec") {
	   			syscfg.web_twrec = val;
   				ts_conn->pcfg->time_wait_rec = val;
	   		}
	   		else ifcmp("twcls") {
	   			syscfg.web_twcls = val;
   				ts_conn->pcfg->time_wait_cls = val;
	   		}
#if DEBUGSOO > 5
			else os_printf(" - none!\n");
#endif
		}
	    else ifcmp("tcp_") {
	    	cstr+=4;
	   		ifcmp("tcrec") {
	   			if(val < 100) syscfg.tcp_client_twait = 100;
	   			else if(val > 65000) syscfg.tcp_client_twait = 65000;
	   			else syscfg.tcp_client_twait = val;
	   		}
#ifdef USE_TCP2UART
	   		else ifcmp("port") tcp2uart_start(val);
	   		else ifcmp("twrec") {
	   			syscfg.tcp2uart_twrec = val;
	   			if(tcp2uart_servcfg != NULL) {
	   				tcp2uart_servcfg->time_wait_rec = val;
	   			}
	   		}
	   		else ifcmp("twcls") {
	   			syscfg.tcp2uart_twcls = val;
	   			if(tcp2uart_servcfg != NULL) {
	   				tcp2uart_servcfg->time_wait_cls = val;
	   			}
	   		}
			else ifcmp("url") {
				if(new_tcp_client_url(pvar))
					tcp2uart_start(syscfg.tcp2uart_port);
			}
        	else ifcmp("reop") {
	   			syscfg.cfg.b.tcp2uart_reopen = (val)? 1 : 0;
	   			if(tcp2uart_servcfg != NULL) {
	   				tcp2uart_servcfg->flag.srv_reopen = (val)? 1 : 0;
	   			}
        	}
#endif
	    }
#ifdef USE_MODBUS
	    else ifcmp("mdb_") {
	    	cstr+=4;
	    	ifcmp("port") mdb_tcp_start(val);
	   		else ifcmp("twrec") {
	   			syscfg.mdb_twrec = val;
	   			if(mdb_tcp_servcfg != NULL) {
	   				mdb_tcp_servcfg->time_wait_rec = val;
	   			}
	   		}
	   		else ifcmp("twcls") {
	   			syscfg.mdb_twcls = val;
	   			if(mdb_tcp_servcfg != NULL) {
	   				mdb_tcp_servcfg->time_wait_cls = val;
	   			}
	   		}
#ifndef USE_TCP2UART
			else ifcmp("url") {
				if(new_tcp_client_url(pvar))
					mdb_tcp_start(syscfg.mdb_port);
			}
#endif
        	else ifcmp("reop") {
	   			syscfg.cfg.b.mdb_reopen =  (val)? 1 : 0;
	   			if(mdb_tcp_servcfg != NULL) {
	   				mdb_tcp_servcfg->flag.srv_reopen = (val)? 1 : 0;
	   			}
        	}
        	else ifcmp("id") {
        		syscfg.mdb_id = val;
        	}
#if DEBUGSOO > 5
        	else os_printf(" - none!\n");
#endif
	    }
#endif
		else ifcmp("overclk") 	syscfg.cfg.b.hi_speed_enable = (val)? 1 : 0;
		else ifcmp("pinclr") 	syscfg.cfg.b.pin_clear_cfg_enable = (val)? 1 : 0;
		else ifcmp("debug") {
			val &= 1;
			syscfg.cfg.b.debug_print_enable = val;
			system_set_os_print(val);
			update_mux_txd1();
		}
		else ifcmp("save") {
			if(val == 2) SetSCB(SCB_SYSSAVE); // по закрытию соединения вызвать sys_write_cfg()
			else if(val == 1) sys_write_cfg();
		}
#ifdef USE_NETBIOS
		else ifcmp("netbios") {
			syscfg.cfg.b.netbios_ena = (val)? 1 : 0;
			if(syscfg.cfg.b.netbios_ena) netbios_init();
			else netbios_off();
		}
#endif
#ifdef USE_SNTP
		else ifcmp("sntp") {
			syscfg.cfg.b.sntp_ena = (val)? 1 : 0;
			if(syscfg.cfg.b.sntp_ena) sntp_inits();
			else sntp_close();
		}
#endif
#ifdef USE_CAPTDNS
		else ifcmp("cdns") {
			syscfg.cfg.b.cdns_ena = (val)? 1 : 0;
			if(syscfg.cfg.b.cdns_ena && wifi_softap_get_station_num()) captdns_init();
			else captdns_close();
		}
#endif
#if DEBUGSOO > 5
		else os_printf(" - none!\n");
#endif
		// sys_write_cfg();
	}
    else ifcmp("wifi_") {
      cstr+=5;
      ifcmp("rdcfg") web_conn->udata_stop = Read_WiFi_config(&wificonfig, val);
      else ifcmp("newcfg") {
    	  web_conn->web_disc_cb = (web_func_disc_cb)New_WiFi_config;
    	  web_conn->web_disc_par = val;
/*    	  web_conn->udata_stop = New_WiFi_config(val);
#if DEBUGSOO > 2
          os_printf("New_WiFi_config(0x%p) = 0x%p ", val, web_conn->udata_stop);
#endif */
      }
      else ifcmp("mode")	wificonfig.b.mode = ((val&3)==0)? 3 : val;
      else ifcmp("phy")  	wificonfig.b.phy = val;
//      else ifcmp("chl") 	wificonfig.b.chl = val; // for sniffer
      else ifcmp("sleep") 	wificonfig.b.sleep = val;
      else ifcmp("scan") {
//    	  web_conn->web_disc_par = val;
    	  web_conn->web_disc_cb = (web_func_disc_cb)wifi_start_scan;
      }
      else ifcmp("save") { wifi_save_fcfg(val); }
      else ifcmp("read") {
    	  wifi_read_fcfg();
    	  if(val) {
    		  web_conn->udata_stop = Set_WiFi(&wificonfig, val);
#if DEBUGSOO > 2
    		  os_printf("WiFi set:%p\n", web_conn->udata_stop);
#endif
    	  }
    	  else web_conn->udata_stop = 0;
      }
      else ifcmp("rfopt") system_phy_set_rfoption(val); // phy_afterwake_set_rfoption(val); // phy_afterwake_set_rfoption(option);
      else ifcmp("vddpw") system_phy_set_tpw_via_vdd33(val); // = pphy_vdd33_set_tpw(vdd_x_1000); Adjust RF TX Power according to VDD33, unit: 1/1024V, range [1900, 3300]
      else ifcmp("maxpw") wificonfig.phy_max_tpw = (val > MAX_PHY_TPW)? MAX_PHY_TPW : val;
      else ifcmp("aps_") {
    	  cstr+=4;
    	  ifcmp("cnt") wifi_station_ap_number_set(val);
    	  else ifcmp("id") wifi_station_ap_change(val);
      }
      else ifcmp("ap_") {
    	  cstr+=3;
          ifcmp("ssid") {
        	  if(pvar[0]!='\0'){
        		  os_memset(wificonfig.ap.config.ssid, 0, sizeof(wificonfig.ap.config.ssid));
        		  int len = os_strlen(pvar);
        		  if(len > sizeof(wificonfig.ap.config.ssid)) {
        			  len = sizeof(wificonfig.ap.config.ssid);
        		  }
        		  os_memcpy(wificonfig.ap.config.ssid, pvar, len);
        		  wificonfig.ap.config.ssid_len = len;
#ifdef USE_NETBIOS
        		  netbios_set_name(wificonfig.ap.config.ssid);
#endif
        	  }
          }
          else ifcmp("psw") {
    		  int len = os_strlen(pvar);
    		  if(len > (sizeof(wificonfig.ap.config.password) - 1)) {
    			  len = sizeof(wificonfig.ap.config.password) - 1;
    		  }
    		  os_memset(&wificonfig.ap.config.password, 0, sizeof(wificonfig.ap.config.password));
    		  os_memcpy(wificonfig.ap.config.password, pvar, len);
          }
          else ifcmp("dhcp")	wificonfig.b.ap_dhcp_enable = val;
          else ifcmp("chl") 	wificonfig.ap.config.channel = val;
          else ifcmp("auth") 	wificonfig.ap.config.authmode = val;
          else ifcmp("hssid") 	wificonfig.ap.config.ssid_hidden = val;
          else ifcmp("mcns") 	wificonfig.ap.config.max_connection = val;
    	  else ifcmp("bint") 	wificonfig.ap.config.beacon_interval = val;
    	  else ifcmp("ip") 		wificonfig.ap.ipinfo.ip.addr = ipaddr_addr(pvar);
          else ifcmp("gw") 		wificonfig.ap.ipinfo.gw.addr = ipaddr_addr(pvar);
          else ifcmp("msk") 	wificonfig.ap.ipinfo.netmask.addr = ipaddr_addr(pvar);
          else ifcmp("mac") 	strtomac(pvar,wificonfig.ap.macaddr);
    	  else ifcmp("sip") 	wificonfig.ap.ipdhcp.start_ip.addr = ipaddr_addr(pvar);
    	  else ifcmp("eip") 	wificonfig.ap.ipdhcp.end_ip.addr = ipaddr_addr(pvar);
#if DEBUGSOO > 2
          else os_printf(" - none! ");
#endif
      }
      else ifcmp("st_") {
    	  cstr+=3;
          ifcmp("dhcp") 		wificonfig.b.st_dhcp_enable = val;
          else ifcmp("aucn") 	wificonfig.st.auto_connect = val;
          else ifcmp("rect") 	{
        	  if(val > 8192) val = 8192;
        	  wificonfig.st.reconn_timeout = val;
        	  // wificonfig.st.max_reconn = val;
          }
          else ifcmp("ssid") {
    		  os_memset(wificonfig.st.config.ssid, 0, sizeof(wificonfig.st.config.ssid));
        	  if(pvar[0]!='\0'){
        		  int len = os_strlen(pvar);
        		  if(len > sizeof(wificonfig.st.config.ssid)) {
        			  len = sizeof(wificonfig.st.config.ssid);
        		  }
        		  os_memcpy(wificonfig.st.config.ssid, pvar, len);
        	  }
          }
          else ifcmp("psw") {
    		  int len = os_strlen(pvar);
    		  if(len > (sizeof(wificonfig.st.config.password) - 1)) {
    			  len = sizeof(wificonfig.st.config.password) - 1;
    		  }
    		  os_memset(&wificonfig.st.config.password, 0, sizeof(wificonfig.st.config.password));
    		  os_memcpy(wificonfig.st.config.password, pvar, len);
          }
          else ifcmp("sbss") 	wificonfig.st.config.bssid_set = val;
          else ifcmp("bssid") 	strtomac(pvar, wificonfig.st.config.bssid);
    	  else ifcmp("ip") 		wificonfig.st.ipinfo.ip.addr = ipaddr_addr(pvar);
          else ifcmp("gw") 		wificonfig.st.ipinfo.gw.addr = ipaddr_addr(pvar);
          else ifcmp("msk") 	wificonfig.st.ipinfo.netmask.addr = ipaddr_addr(pvar);
          else ifcmp("mac") 	strtomac(pvar,wificonfig.st.macaddr);
          else ifcmp("hostname") rom_strcpy(wificonfig.st.hostname, pvar, sizeof(wificonfig.st.hostname)-1);
#if DEBUGSOO > 5
          else os_printf(" - none!\n");
#endif
      }
#if DEBUGSOO > 5
      else os_printf(" - none!\n");
#endif
    }
    else ifcmp("uart_") {
        cstr+=5;
        ifcmp("save") uart_save_fcfg(val);
        else ifcmp("read") uart_read_fcfg(val);
        else {
#ifdef USE_RS485DRV
        	// только UART1 !
        	if(cstr[1] != '_' || cstr[0] != '1') {
#if DEBUGSOO > 5
            	os_printf(" - none! ");
#endif
            	return;
        	}
            int n = 1;
#else
            if(cstr[1] != '_' || cstr[0]<'0' || cstr[0]>'1' ) {
#if DEBUGSOO > 5
            	os_printf(" - none! ");
#endif
            	return;
            }
            int n = cstr[0] & 1;
#endif
            cstr += 2;
            ifcmp("baud") {
//                UartDev.baut_rate = val;
                uart_div_modify(n, UART_CLK_FREQ / val);
#ifdef USE_TCP2UART
                if(n == 0) uart0_set_tout();
#endif
            }
            else ifcmp("parity") 	WRITE_PERI_REG(UART_CONF0(n), (READ_PERI_REG(UART_CONF0(n)) & (~UART_PARITY_EN)) | ((val)? UART_PARITY_EN : 0));
            else ifcmp("even") 	 	WRITE_PERI_REG(UART_CONF0(n), (READ_PERI_REG(UART_CONF0(n)) & (~UART_PARITY)) | ((val)? UART_PARITY : 0));
            else ifcmp("bits") 		WRITE_PERI_REG(UART_CONF0(n), (READ_PERI_REG(UART_CONF0(n)) & (~(UART_BIT_NUM << UART_BIT_NUM_S))) | ((val & UART_BIT_NUM)<<UART_BIT_NUM_S));
            else ifcmp("stop") 		WRITE_PERI_REG(UART_CONF0(n), (READ_PERI_REG(UART_CONF0(n)) & (~(UART_STOP_BIT_NUM << UART_STOP_BIT_NUM_S))) | ((val & UART_STOP_BIT_NUM)<<UART_STOP_BIT_NUM_S));
            else ifcmp("loopback") 	WRITE_PERI_REG(UART_CONF0(n), (READ_PERI_REG(UART_CONF0(n)) & (~UART_LOOPBACK)) | ((val)? UART_LOOPBACK : 0));
#ifndef USE_RS485DRV
            else ifcmp("flow") {
            	if(n == 0) uart0_set_flow((val != 0));
            }
#endif
        	else ifcmp("swap") {
        		int mask = PERI_IO_UART0_PIN_SWAP;
        		if(n != 0) mask = PERI_IO_UART1_PIN_SWAP;
        		if(val) PERI_IO_SWAP |= mask;
        		else  PERI_IO_SWAP &= ~mask;
#ifndef USE_RS485DRV
        		if(n == 0) update_mux_uart0();
#endif
        	}
            else ifcmp("rts_inv") set_uartx_invx(n, val, UART_RTS_INV);
            else ifcmp("dtr_inv") set_uartx_invx(n, val, UART_DTR_INV);
            else ifcmp("cts_inv") set_uartx_invx(n, val, UART_CTS_INV);
            else ifcmp("rxd_inv") set_uartx_invx(n, val, UART_RXD_INV);
            else ifcmp("txd_inv") set_uartx_invx(n, val, UART_TXD_INV);
            else ifcmp("dsr_inv") set_uartx_invx(n, val, UART_DSR_INV) ;
#if DEBUGSOO > 5
            else os_printf(" - none! ");
#endif
        }
#if DEBUGSOO > 5
        else os_printf(" - none! ");
#endif
    }
    else ifcmp("gpio") {
        cstr+=4;
    	if((*cstr>='0')&&(*cstr<='9')) {
    		uint32 n = atoi(cstr);
    		cstr++;
    		if(*cstr != '_') cstr++;
    		if(*cstr == '_' && n < 16) {
    			cstr++;
	    		ifcmp("set") { if(val) GPIO_OUT_W1TS = 1 << n; }
	            else ifcmp("clr") { if(val) GPIO_OUT_W1TC = 1 << n; }
	            else ifcmp("out") {
	            	if(val == 3) {
	            		if(GPIO_OUT &(1<<n)) GPIO_OUT_W1TC = 1 << n;
	            		else GPIO_OUT_W1TS = 1 << n;
	            	}
	            	else if(val == 1) GPIO_OUT_W1TS = 1 << n;
	            	else GPIO_OUT_W1TC = 1 << n;
	            }
	            else ifcmp("dir") {
	            	if(val == 3) {
	            		if(GPIO_ENABLE & (1<<n)) GPIO_ENABLE_W1TC = 1 << n;
	            		else GPIO_ENABLE_W1TS = 1 << n;
	            	}
	            	else if(val == 1) GPIO_ENABLE_W1TS = 1 << n;
	            	else GPIO_ENABLE_W1TC =  1 << n;
	            }
    			else ifcmp("ena")	GPIO_ENABLE_W1TS = 1 << n;
    			else ifcmp("dis")	GPIO_ENABLE_W1TC = 1 << n;
	            else ifcmp("fun")	{ set_gpiox_mux_func(n,val); }
	            else ifcmp("io") 	{ set_gpiox_mux_func_ioport(n); }
	            else ifcmp("def") 	{ set_gpiox_mux_func_default(n); }
	            else ifcmp("sgs") 	{ sigma_delta_setup(n); set_sigma_duty_312KHz(val); }
	            else ifcmp("sgc") 	sigma_delta_close(n);
	            else ifcmp("od") 	reg_sct_bits(&GPIOx_PIN(n), GPIO_PIN_DRIVER, val);
	            else ifcmp("pu") 	reg_sct_bits(get_addr_gpiox_mux(n), 1<<GPIO_MUX_PULLUP_BIT, val);
	            else ifcmp("pd") 	reg_sct_bits(get_addr_gpiox_mux(n), 1<<GPIO_MUX_PULLDOWN_BIT, val);
    		}
    	}
    	else if(*cstr == '_') {
    		cstr++;
    		ifcmp("set") 		GPIO_OUT_W1TS = val;
            else ifcmp("clr") 	GPIO_OUT_W1TC = val;
            else ifcmp("out") 	GPIO_OUT = val;
            else ifcmp("ena") 	GPIO_ENABLE_W1TS = val;
            else ifcmp("dis") 	GPIO_ENABLE_W1TC = val;
            else ifcmp("dir") 	GPIO_ENABLE = val;
            else ifcmp("sgn") 	set_sigma_duty_312KHz(val);
    	}
    }
    else ifcmp("hexdmp") {
    	if(web_conn->bffiles[0]==WEBFS_WEBCGI_HANDLE && CheckSCB(SCB_GET)) {
    		if(val > 0) {
    	    	if(cstr[6]=='d') ts_conn->flag.user_option1 = 1;
    	    	else ts_conn->flag.user_option1 = 0;
    			uint32 x = ahextoul(cstr+7);
    			if(x >= 0x20000000) {
    				web_conn->udata_start = x;
    			};
    			web_conn->udata_stop = val + web_conn->udata_start;
    			SetSCB(SCB_RETRYCB | SCB_FCALBACK);
    			SetNextFunSCB(web_hexdump);
    		};
    	}
    }
#ifdef USE_RS485DRV
	else ifcmp("rs485_") {
   		cstr+=6;
		ifcmp("baud") {
			if(rs485cfg.baud != val) {
				rs485cfg.baud = val;
	       		rs485_drv_set_baud();
			}
       	}
       	else ifcmp("timeout") {
       		if(val == 0) val = 1;
       		rs485cfg.timeout = val;
       	}
       	else ifcmp("pause") {
       		rs485cfg.pause = val;
       		rs485_drv_set_baud();
       	}
       	else ifcmp("parity") {
       		val &= 0x03;
       		if(rs485cfg.flg.b.parity != val) {
       			rs485cfg.flg.b.parity = val;
           		rs485_drv_set_baud();
       		}
       	}
       	else ifcmp("pinre") {
       		if(val > 15) val = 0x10;
       		if(rs485cfg.flg.b.pin_ena != val) {
       			rs485cfg.flg.b.pin_ena = val;
       			rs485_drv_set_pins();
       		}
       	}
       	else ifcmp("swap") {
       		val &= 0x01;
       		if(rs485cfg.flg.b.swap != val) {
       			rs485cfg.flg.b.swap = val;
       			rs485_drv_set_pins();
       		}
       	}
       	else ifcmp("spdtw") {
       		val &= 0x01;
       		if(rs485cfg.flg.b.spdtw != val) {
       			rs485cfg.flg.b.spdtw = val;
       			rs485_drv_set_baud();
       		}
       	}
#ifdef MDB_RS485_MASTER
       	else ifcmp("master") rs485cfg.flg.b.master = val&1;
#endif
        else ifcmp("save") {
        	uart_save_fcfg(1);
        }
        else ifcmp("read") {
        	uart_read_fcfg(1);
        }
        else ifcmp("start") {
        	rs485_drv_start();
        }
        else ifcmp("stop") {
        	rs485_drv_stop();
        }
#if DEBUGSOO > 5
        else os_printf(" - none!\n");
#endif
	}
#endif
#if 0
	else ifcmp("call") {
		call_func ptr = (call_func)(ahextoul(cstr+4)&0xfffffffc);
		web_conn->udata_stop = ptr(val, web_conn->udata_start, web_conn->udata_stop);
#if DEBUGSOO > 0
		os_printf("%p=call_func() ", web_conn->udata_stop);
#endif
	}
#endif
#if 0
    else ifcmp("web_") {
    	cstr+=4;
    	ifcmp("port") {
    			web_conn->web_disc_cb = (web_func_disc_cb)webserver_init;
        		web_conn->web_disc_par = val;
    	}
    	else ifcmp("close") {
			web_conn->web_disc_cb = (web_func_disc_cb)webserver_close;
			web_conn->web_disc_par = val;
    	}
    	else ifcmp("twrec") ts_conn->pcfg->time_wait_rec = val;
    	else ifcmp("twcls") ts_conn->pcfg->time_wait_cls = val;
#if DEBUGSOO > 5
    	else os_printf(" - none! ");
#endif
    }
#endif
#if 0
#ifdef	USE_TCP2UART
    else ifcmp("tcp_") {
    	cstr+=4;
   		ifcmp("twrec") {
   			if(tcp2uart_servcfg != NULL) {
   				tcp2uart_servcfg->time_wait_rec = val;
   			}
   		}
   		else ifcmp("twcls") {
   			if(tcp2uart_servcfg != NULL) {
   			   	tcp2uart_servcfg->time_wait_cls = val;
   			}
   		}
    }
#endif
#endif
#ifdef USE_OVERLAY
		else ifcmp("ovl") {
			cstr += 3;
			if(*cstr == 0) {
				if (ovl_loader(pvar) == 0) {
					if(CheckSCB(SCB_WEBSOC)) {
						ovl_call(1);
					}
					else {
						web_conn->web_disc_cb = (web_func_disc_cb)ovl_call; // адрес старта оверлея
						web_conn->web_disc_par = 1; // параметр функции - инициализация
					}
				}
			}
			else if(*cstr == '$') {
    			if(ovl_call != NULL) ovl_call(ahextoul(pvar));
    		}
			else if(*cstr == '@') {
    			if(ovl_call != NULL) ovl_call((int)pvar);
			}
		}
#endif
#ifdef USE_TIMER0
	else ifcmp("tinit") {
#ifdef TIMER0_USE_NMI_VECTOR
		timer0_init(timer0_tst_isr, val, val);
#else
		timer0_init(timer0_tst_isr, NULL);
#endif
	}
	else ifcmp("ttik") {
		if(val == 0) timer0_stop();
		timer0_start(val, 0);
	}
	else ifcmp("trep") {
		if(val == 0) timer0_stop();
		timer0_start(val, 1);
	}
#endif // USE_TIMER0
#if DEBUGSOO > 5
    else os_printf(" - none! ");
#endif
}

#endif // USE_WEB
