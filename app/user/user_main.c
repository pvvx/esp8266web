/******************************************************************************
 * PV` FileName: user_main.c
 *******************************************************************************/

#include "user_config.h"
#include "bios.h"
#include "add_sdk_func.h"
#include "hw/esp8266.h"
#include "user_interface.h"
#include "tcp_srv_conn.h"
#include "flash_eep.h"
#include "wifi.h"
#include "tcp2uart.h"
#include "hw/spi_register.h"
#include "rom2ram.h"
#include "web_iohw.h"
#include "ws2812.h"

#ifdef USE_SRV_WEB_PORT
#include "web_srv.h"
#endif

#ifdef UDP_TEST_PORT
#include "udp_test_port.h"
#endif

#ifdef USE_NETBIOS
#include "netbios.h"
#endif

#ifdef USE_SNTP
#include "sntp.h"
#endif


#if DEBUGSOO > 1
//#define TEST_TIMER 1
//#include "driver/int_time_us.h"

//#define TEST_RTC_RTNTN 1
#endif

#if DEF_SDK_VERSION > 959
//uint32 ICACHE_FLASH_ATTR espconn_init(uint32 x) {
//	return 1;
//}
#endif

#ifdef TEST_RTC_RTNTN

bool ICACHE_FLASH_ATTR test_rtc_mem(void) {
	uint32 x[128];
	uint32 i, t = 0x43545240;
	bool chg = false;
#if DEBUGSOO > 1
	os_printf("Test rtc memory retention... ");
#endif
	if(!system_rtc_mem_read(64, x, sizeof(x))) {
#if DEBUGSOO > 1
		os_printf("read error!\n");
#endif
		return false;
	}
	for(i = 0; i < 128; i++) {
		if(x[i] != t) {
			x[i] = t;
			chg = true;
		};
		t++;
	};
	if(chg) {
		if(!system_rtc_mem_write(64, x, sizeof(x))) {
#if DEBUGSOO > 1
			os_printf("write error!\n");
			return false;
#endif
		};
#if DEBUGSOO > 1
		os_printf("changes, new write\n");
#endif
		return false;
	};
#if DEBUGSOO > 1
	os_printf("Ok.\n");
#endif
	return true;
}
#endif
void ICACHE_FLASH_ATTR init_done_cb(void)
{
    os_printf("\nSDK Init - Ok\nCurrent 'heap' size: %d bytes\n", system_get_free_heap_size());
#ifdef TEST_RTC_RTNTN
	test_rtc_mem();
#endif
}

/******************************************************************************
 * FunctionName : user_init
 * Description  : entry of user application, init user function here
 * Parameters   : none
 * Returns      : none
 *******************************************************************************/
void ICACHE_FLASH_ATTR user_init(void) {
	sys_read_cfg();
	if(!syscfg.cfg.b.debug_print_enable) system_set_os_print(0);
	uart_init();
#if USE_TMP2NET_PORT
	GPIO0_MUX = 0;
#else
	GPIO0_MUX = VAL_MUX_GPIO0_SDK_DEF;
#endif
	GPIO4_MUX = VAL_MUX_GPIO4_SDK_DEF;
	GPIO5_MUX = VAL_MUX_GPIO5_SDK_DEF;
	GPIO12_MUX = VAL_MUX_GPIO12_SDK_DEF;
	GPIO14_MUX = VAL_MUX_GPIO14_SDK_DEF;
	system_timer_reinit();
#if DEBUGSOO > 0
	os_printf("\nSimple WEB version: " WEB_SVERSION "\nOpenLoaderSDK v1.2\n");
#endif
	if(syscfg.cfg.b.pin_clear_cfg_enable) test_pin_clr_wifi_config();
	set_cpu_clk(); // select cpu frequency 80 or 160 MHz
	iram_buf_init();
#if DEBUGSOO > 0
	if(eraminfo.size > 1024) os_printf("Found free IRAM: base: %p, size: %d bytes\n", eraminfo.base,  eraminfo.size);
	os_printf("System memory:\n");
    system_print_meminfo();
    os_printf("Current 'heap' size: %d bytes\n", system_get_free_heap_size());
#endif
#if DEBUGSOO > 0
	os_printf("Set CPU CLK: %u MHz\n", ets_get_cpu_frequency());
#endif
	Setup_WiFi();
#if USE_TMP2NET_PORT
	tpm2net_init();
#endif
#ifdef USE_NETBIOS
	if(syscfg.cfg.b.netbios_ena) netbios_init();
#endif
/* #ifdef USE_SNTP
	if(syscfg.cfg.b.sntp_ena) sntp_init();
#endif */
#ifdef UDP_TEST_PORT
	if(syscfg.udp_port) udp_test_port_init(syscfg.udp_port);
#endif
	// инициализация и запуск tcp серверa(ов)
#ifdef USE_SRV_WEB_PORT
    if(syscfg.web_port) webserver_init(syscfg.web_port);
#endif
///    if(syscfg.tcp2uart_port) tcp2uart_init(syscfg.tcp2uart_port);
	system_deep_sleep_set_option(0);
	system_init_done_cb(init_done_cb);
}
