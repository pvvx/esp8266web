/******************************************************************************
 * FileName: webserver.c
 * Description: The web server mode configration.
*******************************************************************************/

#include "user_config.h"
#include "bios.h"
#include "add_sdk_func.h"
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
#include "tcp2uart.h"
#include "wifi.h"
#include "flash_eep.h"
#include "driver/sigma_delta.h"
#include "sys_const.h"

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

extern TCP_SERV_CONN * tcp2uart_conn;

typedef uint32 (* call_func)(uint32 a, uint32 b, uint32 c);

void ICACHE_FLASH_ATTR reg_sct_bits(volatile uint32 * addr, uint32 bits, uint32 val)
{
	uint32 x = *addr;
	if(val == 3) x ^= bits;
	else if(val) x |= bits;
	else x &= ~ bits;
	*addr =  x;
}
/******************************************************************************
 * FunctionName : parse_url
 * Description  : parse the received data from the server
 * Parameters   : CurHTTP -- the result of parsing the url
 * Returns      : none
*******************************************************************************/
void ICACHE_FLASH_ATTR web_int_vars(TCP_SERV_CONN *ts_conn, uint8 *pcmd, uint8 *pvar)
{
    WEB_SRV_CONN *web_conn = (WEB_SRV_CONN *)ts_conn->linkd;
	uint32 val = ahextoul(pvar);
	char *cstr = pcmd;
#if DEBUGSOO > 2
    os_printf("[%s=%s]\n", pcmd, pvar);
#endif
	if(!os_memcmp((void*)cstr, "start", 5)) web_conn->udata_start = val;
	else if(!os_memcmp((void*)cstr, "stop", 4)) web_conn->udata_stop = val;
	else if(!os_memcmp((void*)cstr, "sys_", 4)) {
		cstr+=4;
		if(!os_memcmp((void*)cstr, "restart", 7)) {
			if(val == 12345) web_conn->web_disc_cb = (web_func_disc_cb)system_restart;
		}
		else if(!os_memcmp((void*)cstr, "reset", 5)) {
			if(val == 12345) web_conn->web_disc_cb = (web_func_disc_cb)_ResetVector;
		}
#ifdef USE_ESPCONN
		else if(!os_memcmp((void*)cstr, "maxcns", 6)) espconn_tcp_set_max_con(((val<=5)&&(val>0))? val : 5);
#endif
		else if(!os_memcmp((void*)cstr, "ram", 3)) { uint32 ptr = ahextoul(cstr+3)&0xfffffffc; *((uint32 *)ptr) = val; }
		else if(!os_memcmp((void*)cstr, "debug", 5)) system_set_os_print(val);
#ifdef USE_LWIP_PING
		else if(!os_memcmp((void*)cstr, "ping", 4)) {
//			struct ping_option *pingopt = (struct ping_option *)UartDev.rcv_buff.pRcvMsgBuff;
			pingopt.ip = ipaddr_addr(pvar);
			pingopt.count = 3;
			pingopt.recv_function=NULL;
			pingopt.sent_function=NULL;
			ping_start(&pingopt);
		}
#endif
		else if(!os_memcmp((void*)cstr, "sleep_", 6)) {
			cstr += 6;
			if(!os_memcmp((void*)cstr, "option", 6)) system_deep_sleep_set_option(val);
			else if(!os_memcmp((void*)cstr, "us", 2)) {
				web_conn->web_disc_cb = (web_func_disc_cb)system_deep_sleep;
				web_conn->web_disc_par = val;
			}
#if DEBUGSOO > 5
			else os_printf(" - none!\n");
#endif
		}
		else if(!os_memcmp((void*)cstr, "const_", 6)) write_sys_const(ahextoul(cstr+6), val);
		else if(!os_memcmp((void*)cstr, "ucnst_", 6)) write_user_const(ahextoul(cstr+6), val);
		else if(!os_memcmp((void*)cstr, "clkcpu", 6)) {
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
	else if(!os_memcmp((void*)cstr, "cfg_", 4)) {
		cstr += 4;
		if(!os_memcmp((void*)cstr, "web_", 4)) {
			cstr += 4;
			if(!os_memcmp((void*)cstr, "port", 4)) {
				if(syscfg.web_port != val) {
	        		web_conn->web_disc_par = syscfg.web_port; // ts_conn->pcfg->port
					syscfg.web_port = val;
	    			web_conn->web_disc_cb = (web_func_disc_cb)webserver_reinit;
				}
			}
			else if(!os_memcmp((void*)cstr, "twd", 3)) {
				if(val) {
					syscfg.cfg.b.web_time_wait_delete = 1;
					ts_conn->pcfg->flag.pcb_time_wait_free = 1;
				}
				else {
					syscfg.cfg.b.web_time_wait_delete = 0;
					ts_conn->pcfg->flag.pcb_time_wait_free = 0;
				}
			}
#if DEBUGSOO > 5
			else os_printf(" - none!\n");
#endif
		}
	    else if(!os_memcmp((void*)cstr, "tcp_", 4)) {
	    	cstr+=4;
	    	if(!os_memcmp((void*)cstr,"port", 4)) {
	    		if(val) {
	    			if(syscfg.tcp2uart_port != val) {
	    				if(syscfg.tcp2uart_port) tcp2uart_close(syscfg.tcp2uart_port);
	    				tcp2uart_init(val);
	    			}
	    		}
	    		else {
	    			if(syscfg.tcp2uart_port) tcp2uart_close(syscfg.tcp2uart_port);
	    		}
	    	}
	   		else if(!os_memcmp((void*)cstr,"twrec", 5)) {
	   			syscfg.tcp2uart_twrec = val;
	   			if(syscfg.tcp2uart_port) {
	   				TCP_SERV_CFG *p = tcpsrv_port2pcfg(syscfg.tcp2uart_port);
	   				if(p != NULL) p->time_wait_rec = val;
	   			}
	   		}
	   		else if(!os_memcmp((void*)cstr,"twcls", 5)) {
	   			syscfg.tcp2uart_twcls = val;
	   			if(syscfg.tcp2uart_port) {
	   				TCP_SERV_CFG *p = tcpsrv_port2pcfg(syscfg.tcp2uart_port);
	   				if(p != NULL) p->time_wait_cls = val;
	   			}
	   		}
	    }
	    else if(!os_memcmp((void*)cstr, "udp_", 4)) {
	    	cstr+=4;
	    	if(!os_memcmp((void*)cstr,"port", 4)) syscfg.udp_port = val;
	    }
		else if(!os_memcmp((void*)cstr, "overclk", 7)) syscfg.cfg.b.hi_speed_enable = (val)? 1 : 0;
		else if(!os_memcmp((void*)cstr, "pinclr", 6)) syscfg.cfg.b.pin_clear_cfg_enable = (val)? 1 : 0;
		else if(!os_memcmp((void*)cstr, "debug", 5)) {
			val &= 1;
			syscfg.cfg.b.debug_print_enable = val;
			system_set_os_print(val);
			update_mux_txd1();
		}
		else if(!os_memcmp((void*)cstr, "save", 4)) {
			if(val == 2) SetSCB(SCB_SYSSAVE); // по закрытию соединения вызвать sys_write_cfg()
			else if(val == 1) sys_write_cfg();
		}
#ifdef USE_NETBIOS
		else if(!os_memcmp((void*)cstr, "netbios", 7)) {
			syscfg.cfg.b.netbios_ena = (val)? 1 : 0;
			if(syscfg.cfg.b.netbios_ena) netbios_init();
			else netbios_off();
		}
#endif
#ifdef USE_SNTP
		else if(!os_memcmp((void*)cstr, "sntp", 4)) {
			syscfg.cfg.b.sntp_ena = (val)? 1 : 0;
			if(syscfg.cfg.b.sntp_ena) sntp_init();
			else sntp_close();
		}
#endif
#if DEBUGSOO > 5
		else os_printf(" - none!\n");
#endif
		// sys_write_cfg();
	}
    else if(!os_memcmp((void*)cstr, "wifi_", 5)) {
      cstr+=5;
      if(!os_memcmp((void*)cstr, "rdcfg", 5)) web_conn->udata_stop = Read_WiFi_config(&wificonfig, val);
      else if(!os_memcmp((void*)cstr, "newcfg", 6)) {
    	  web_conn->web_disc_cb = (web_func_disc_cb)New_WiFi_config;
    	  web_conn->web_disc_par = val;
/*    	  web_conn->udata_stop = New_WiFi_config(val);
#if DEBUGSOO > 2
          os_printf("New_WiFi_config(0x%p) = 0x%p ", val, web_conn->udata_stop);
#endif */
      }
      else if(!os_memcmp((void*)cstr, "mode", 4)) wificonfig.b.mode = ((val&3)==0)? 3 : val;
      else if(!os_memcmp((void*)cstr, "phy", 3)) wificonfig.b.phy = val;
      else if(!os_memcmp((void*)cstr, "chl", 3)) wificonfig.b.chl = val;
      else if(!os_memcmp((void*)cstr, "sleep", 5)) wificonfig.b.sleep = val;
      else if(!os_memcmp((void*)cstr, "scan", 4)) {
//    	  web_conn->web_disc_par = val;
    	  web_conn->web_disc_cb = (web_func_disc_cb)wifi_start_scan;
      }
      else if(!os_memcmp((void*)cstr, "save", 4)) { wifi_save_fcfg(val); }
      else if(!os_memcmp((void*)cstr, "read", 4)) {
    	  wifi_read_fcfg();
    	  if(val) {
    		  web_conn->udata_stop = Set_WiFi(&wificonfig, val);
#if DEBUGSOO > 2
    		  os_printf("WiFi set:%p\n", web_conn->udata_stop);
#endif
    	  }
    	  else web_conn->udata_stop = 0;
      }
      else if(!os_memcmp((void*)cstr, "rfopt", 5)) system_phy_set_rfoption(val); // phy_afterwake_set_rfoption(val); // phy_afterwake_set_rfoption(option);
      else if(!os_memcmp((void*)cstr, "vddpw", 5)) system_phy_set_tpw_via_vdd33(val); // = pphy_vdd33_set_tpw(vdd_x_1000); Adjust RF TX Power according to VDD33, unit: 1/1024V, range [1900, 3300]
      else if(!os_memcmp((void*)cstr, "maxpw", 5)) system_phy_set_max_tpw(val); // = phy_set_most_tpw(pow_db); unit: 0.25dBm, range [0, 82], 34th byte esp_init_data_default.bin
      else if(!os_memcmp((void*)cstr, "ap_", 3)) {
    	  cstr+=3;
          if(!os_memcmp((void*)cstr, "ssid", 4)) {
        	  if(pvar[0]!='\0'){
        		  os_memset(wificonfig.ap.config.ssid, 0, sizeof(wificonfig.ap.config.ssid));
        		  int len = os_strlen(pvar);
        		  if(len > sizeof(wificonfig.ap.config.ssid)) {
        			  len = sizeof(wificonfig.ap.config.ssid);
        		  }
        		  os_memcpy(wificonfig.ap.config.ssid, pvar, len);
        		  wificonfig.ap.config.ssid_len = len;
        	  }
          }
          else if(!os_memcmp((void*)cstr, "psw", 3)) {
    		  int len = os_strlen(pvar);
    		  if(len > (sizeof(wificonfig.ap.config.password) - 1)) {
    			  len = sizeof(wificonfig.ap.config.password) - 1;
    		  }
    		  os_memset(&wificonfig.ap.config.password, 0, sizeof(wificonfig.ap.config.password));
    		  os_memcpy(wificonfig.ap.config.password, pvar, len);
          }
          else if(!os_memcmp((void*)cstr, "dncp", 5)) wificonfig.b.ap_dhcp_enable = val;
          else if(!os_memcmp((void*)cstr, "chl", 3)) wificonfig.ap.config.channel = val;
          else if(!os_memcmp((void*)cstr, "aum", 3)) wificonfig.ap.config.authmode = val;
          else if(!os_memcmp((void*)cstr, "hssid", 5)) wificonfig.ap.config.ssid_hidden = val;
          else if(!os_memcmp((void*)cstr, "mcns", 4)) wificonfig.ap.config.max_connection = val;
    	  else if(!os_memcmp((void*)cstr, "bint", 4)) wificonfig.ap.config.beacon_interval = val;
    	  else if(!os_memcmp((void*)cstr, "ip", 2)) wificonfig.ap.ipinfo.ip.addr = ipaddr_addr(pvar);
          else if(!os_memcmp((void*)cstr, "gw", 2)) wificonfig.ap.ipinfo.gw.addr = ipaddr_addr(pvar);
          else if(!os_memcmp((void*)cstr, "msk", 3)) wificonfig.ap.ipinfo.netmask.addr = ipaddr_addr(pvar);
          else if(!os_memcmp((void*)cstr, "mac", 3)) strtomac(pvar,wificonfig.ap.macaddr);
    	  else if(!os_memcmp((void*)cstr, "sip", 3)) wificonfig.ap.ipdhcp.start_ip.addr = ipaddr_addr(pvar);
    	  else if(!os_memcmp((void*)cstr, "eip", 3)) wificonfig.ap.ipdhcp.end_ip.addr = ipaddr_addr(pvar);
#if DEBUGSOO > 2
          else os_printf(" - none! ");
#endif
      }
      else if(!os_memcmp((void*)cstr, "st_", 3)) {
    	  cstr+=3;
          if(!os_memcmp((void*)cstr, "dncp", 5)) wificonfig.b.st_dhcp_enable = val;
          else if(!os_memcmp((void*)cstr, "aucn", 4)) wificonfig.st.auto_connect = val;
          if(!os_memcmp((void*)cstr, "ssid", 4)) {
        	  if(pvar[0]!='\0'){
        		  os_memset(wificonfig.st.config.ssid, 0, sizeof(wificonfig.st.config.ssid));
        		  int len = os_strlen(pvar);
        		  if(len > sizeof(wificonfig.st.config.ssid)) {
        			  len = sizeof(wificonfig.st.config.ssid);
        		  }
        		  os_memcpy(wificonfig.st.config.ssid, pvar, len);
        	  }
          }
          else if(!os_memcmp((void*)cstr, "psw", 3)) {
    		  int len = os_strlen(pvar);
    		  if(len > (sizeof(wificonfig.st.config.password) - 1)) {
    			  len = sizeof(wificonfig.st.config.password) - 1;
    		  }
    		  os_memset(&wificonfig.st.config.password, 0, sizeof(wificonfig.st.config.password));
    		  os_memcpy(wificonfig.st.config.password, pvar, len);
          }
          else if(!os_memcmp((void*)cstr, "sbss", 4)) wificonfig.st.config.bssid_set = val;
          else if(!os_memcmp((void*)cstr, "bssid", 5)) strtomac(pvar, wificonfig.st.config.bssid);
    	  else if(!os_memcmp((void*)cstr, "ip", 2)) wificonfig.st.ipinfo.ip.addr = ipaddr_addr(pvar);
          else if(!os_memcmp((void*)cstr, "gw", 2)) wificonfig.st.ipinfo.gw.addr = ipaddr_addr(pvar);
          else if(!os_memcmp((void*)cstr, "msk", 3)) wificonfig.st.ipinfo.netmask.addr = ipaddr_addr(pvar);
          else if(!os_memcmp((void*)cstr, "mac", 3)) strtomac(pvar,wificonfig.st.macaddr);
#if DEBUGSOO > 5
          else os_printf(" - none!\n");
#endif
      }
#if DEBUGSOO > 5
      else os_printf(" - none!\n");
#endif
    }
    else if(!os_memcmp((void*)cstr, "uart_", 5)) {
        cstr+=5;
        if(!os_memcmp((void*)cstr, "save", 4)) uart_save_fcfg(val);
        else if(!os_memcmp((void*)cstr, "read", 4)) uart_read_fcfg(val);
        else {
            int n = 0;
            if(cstr[1] != '_' || cstr[0]<'0' || cstr[0]>'1' ) {
#if DEBUGSOO > 5
            	os_printf(" - none! ");
#endif
            }
            if(cstr[0] != '0') n++;
            cstr += 2;
            if(!os_memcmp((void*)cstr, "baud", 4)) {
//                UartDev.baut_rate = val;
                uart_div_modify(n, UART_CLK_FREQ / val);
            }
            else if(!os_memcmp((void*)cstr, "parity", 6)) WRITE_PERI_REG(UART_CONF0(n), (READ_PERI_REG(UART_CONF0(n)) & (~UART_PARITY_EN)) | ((val)? UART_PARITY_EN : 0));
            else if(!os_memcmp((void*)cstr, "even", 4)) WRITE_PERI_REG(UART_CONF0(n), (READ_PERI_REG(UART_CONF0(n)) & (~UART_PARITY)) | ((val)? UART_PARITY : 0));
            else if(!os_memcmp((void*)cstr, "bits", 4)) WRITE_PERI_REG(UART_CONF0(n), (READ_PERI_REG(UART_CONF0(n)) & (~(UART_BIT_NUM << UART_BIT_NUM_S))) | ((val & UART_BIT_NUM)<<UART_BIT_NUM_S));
            else if(!os_memcmp((void*)cstr, "stop", 4)) WRITE_PERI_REG(UART_CONF0(n), (READ_PERI_REG(UART_CONF0(n)) & (~(UART_STOP_BIT_NUM << UART_STOP_BIT_NUM_S))) | ((val & UART_STOP_BIT_NUM)<<UART_STOP_BIT_NUM_S));
            else if(!os_memcmp((void*)cstr, "loopback", 8)) WRITE_PERI_REG(UART_CONF0(n), (READ_PERI_REG(UART_CONF0(n)) & (~UART_LOOPBACK)) | ((val)? UART_LOOPBACK : 0));
            else if(!os_memcmp((void*)cstr, "flow", 4)) {
            	if(n==0) uart0_set_flow((val!=0));
            }
            else if(!os_memcmp((void*)cstr, "rts_inv", 7)) set_uartx_invx(n, val, UART_RTS_INV);
            else if(!os_memcmp((void*)cstr, "dtr_inv", 7)) set_uartx_invx(n, val, UART_DTR_INV);
            else if(!os_memcmp((void*)cstr, "cts_inv", 7)) set_uartx_invx(n, val, UART_CTS_INV);
            else if(!os_memcmp((void*)cstr, "rxd_inv", 7)) set_uartx_invx(n, val, UART_RXD_INV);
            else if(!os_memcmp((void*)cstr, "txd_inv", 7)) set_uartx_invx(n, val, UART_TXD_INV);
            else if(!os_memcmp((void*)cstr, "dsr_inv", 7)) set_uartx_invx(n, val, UART_DSR_INV) ;
#if DEBUGSOO > 5
            else os_printf(" - none! ");
#endif
        }
#if DEBUGSOO > 5
        else os_printf(" - none! ");
#endif
    }
    else if(!os_memcmp((void*)cstr, "gpio", 4)) {
        cstr+=4;
    	if((*cstr>='0')&&(*cstr<='9')) {
    		uint32 n = atoi(cstr);
    		cstr++;
    		if(*cstr != '_') cstr++;
    		if(*cstr == '_' && n < 16) {
    			cstr++;
	    		if(!os_memcmp((void*)cstr, "set", 3)) { if(val) GPIO_OUT_W1TS = 1 << n; }
	            else if(!os_memcmp((void*)cstr, "clr", 3)) { if(val) GPIO_OUT_W1TC = 1 << n; }
	            else if(!os_memcmp((void*)cstr, "out", 3)) {
	            	if(val == 3) {
	            		if(GPIO_OUT &(1<<n)) GPIO_OUT_W1TC = 1 << n;
	            		else GPIO_OUT_W1TS = 1 << n;
	            	}
	            	else if(val == 1) GPIO_OUT_W1TS = 1 << n;
	            	else GPIO_OUT_W1TC = 1 << n;
	            }
	            else if(!os_memcmp((void*)cstr, "ena", 3)) { if(val) GPIO_ENABLE_W1TS = 1 << n; }
	            else if(!os_memcmp((void*)cstr, "dis", 3)) { if(val) GPIO_ENABLE_W1TC = 1 << n; }
	            else if(!os_memcmp((void*)cstr, "dir", 3)) {
	            	if(val == 3) {
	            		if(GPIO_ENABLE & (1<<n)) GPIO_ENABLE_W1TC = 1 << n;
	            		else GPIO_ENABLE_W1TS = 1 << n;
	            	}
	            	else if(val == 1) GPIO_ENABLE_W1TS = 1 << n;
	            	else GPIO_ENABLE_W1TC =  1 << n;
	            }
	            else if(!os_memcmp((void*)cstr, "fun", 3)) { SET_PIN_FUNC(n,val); }
	            else if(!os_memcmp((void*)cstr, "io", 2)) { SET_PIN_FUNC_IOPORT(n); }
	            else if(!os_memcmp((void*)cstr, "def", 3)) { SET_PIN_FUNC_DEF_SDK(n); }
	            else if(!os_memcmp((void*)cstr, "sgs", 3)) { sigma_delta_setup(n); set_sigma_duty_312KHz(val); }
	            else if(!os_memcmp((void*)cstr, "sgc", 3)) sigma_delta_close(n);
	            else if(!os_memcmp((void*)cstr, "sgn", 3)) set_sigma_duty_312KHz(val);
	            else if(!os_memcmp((void*)cstr, "od", 2)) reg_sct_bits(&GPIOx_PIN(n), BIT2, val);
	            else if(!os_memcmp((void*)cstr, "pu", 2)) reg_sct_bits(&GPIOx_MUX(n), BIT7, val);
	            else if(!os_memcmp((void*)cstr, "pd", 2)) reg_sct_bits(&GPIOx_MUX(n), BIT6, val);
    		}
    	}
    	else if(*cstr == '_') {
    		cstr++;
    		if(!os_memcmp((void*)cstr, "set", 3)) GPIO_OUT_W1TS = val;
            else if(!os_memcmp((void*)cstr, "clr", 3)) GPIO_OUT_W1TC = val;
            else if(!os_memcmp((void*)cstr, "out", 3)) GPIO_OUT = val;
            else if(!os_memcmp((void*)cstr, "ena", 3)) GPIO_ENABLE_W1TS = val;
            else if(!os_memcmp((void*)cstr, "dis", 3)) GPIO_ENABLE_W1TC = val;
            else if(!os_memcmp((void*)cstr, "dir", 3)) GPIO_ENABLE = val;
    	}
    }
    else if(!os_memcmp((void*)cstr, "hexdmp", 6)) {
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
	else if(!os_memcmp((void*)cstr, "call", 4)) {
		call_func ptr = (call_func)(ahextoul(cstr+4)&0xfffffffc);
		web_conn->udata_stop = ptr(val, web_conn->udata_start, web_conn->udata_stop);
#if DEBUGSOO > 0
		os_printf("%p=call_func() ", web_conn->udata_stop);
#endif
	}
    else if(!os_memcmp((void*)cstr, "web_", 4)) {
    	cstr+=4;
    	if(!os_memcmp((void*)cstr,"port", 4)) {
    			web_conn->web_disc_cb = (web_func_disc_cb)webserver_init;
        		web_conn->web_disc_par = val;
    	}
    	else if(!os_memcmp((void*)cstr,"close", 4)) {
			web_conn->web_disc_cb = (web_func_disc_cb)webserver_close;
			web_conn->web_disc_par = val;
    	}
    	else if(!os_memcmp((void*)cstr,"twrec", 5)) ts_conn->pcfg->time_wait_rec = val;
    	else if(!os_memcmp((void*)cstr,"twcls", 5)) ts_conn->pcfg->time_wait_cls = val;
#if DEBUGSOO > 5
    	else os_printf(" - none! ");
#endif
    }
    else if(!os_memcmp((void*)cstr, "tcp_", 4)) {
    	cstr+=4;
    	if(!os_memcmp((void*)cstr,"disconnect", 10)) {
    		if(tcp2uart_conn == NULL) tcp_abort(tcp2uart_conn->pcb);
    	}
   		else if(!os_memcmp((void*)cstr,"twrec", 5)) {
   			if(syscfg.tcp2uart_port) {
   				TCP_SERV_CFG *p = tcpsrv_port2pcfg(syscfg.tcp2uart_port);
   				if(p != NULL) p->time_wait_rec = val;
   			}
   		}
   		else if(!os_memcmp((void*)cstr,"twcls", 5)) {
   			if(syscfg.tcp2uart_port) {
   				TCP_SERV_CFG *p = tcpsrv_port2pcfg(syscfg.tcp2uart_port);
   				if(p != NULL) p->time_wait_cls = val;
   			}
   		}
    }
	else if(!os_memcmp((void*)cstr,"test", 5)) {
		TCP_SERV_CFG * p = tcpsrv_init_client();
		p->time_wait_rec = 60;
		p->time_wait_cls = 60;
		tcpsrv_client_start(p, val, 80);
	}
#if DEBUGSOO > 5
    else os_printf(" - none! ");
#endif
}

