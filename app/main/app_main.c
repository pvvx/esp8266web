/******************************************************************************
 * FileName: app_main.c
 * Description: Alternate SDK (libmain.a)
 * Author: PV`
 * (c) PV` 2015
 * ver 0.0.1
*******************************************************************************/
#include "user_config.h"
#include "bios.h"
#include "user_interface.h"
#include "hw/esp8266.h"
#include "hw/spi_register.h"
#include "hw/uart_register.h"
#include "fatal_errs.h"
#include "app_main.h"
#include "add_sdk_func.h"
#include "flash.h"
#include "lwip/init.h"
#include "lwip/netif.h"
#include "lwip/timers.h"
#include "wdt.h"
#include "phy/phy.h"
#include "sys_const.h"
//=============================================================================
// Define
//-----------------------------------------------------------------------------
#define NO_SET_UART_BAUD

#ifdef USE_MAX_IRAM
	#define Cache_Read_Enable_def() Cache_Read_Enable(0, 0, 0)
#else
	#define Cache_Read_Enable_def() Cache_Read_Enable(0, 0, 1)
#endif
//=============================================================================
// Data
//-----------------------------------------------------------------------------
struct s_info info; // ip,mask,gw,mac AP, ST
ETSTimer check_timeouts_timer; // timer_lwip
uint8 user_init_flag;

#ifdef USE_OPEN_LWIP
extern bool default_hostname; // in eagle_lwip_if.c
#endif
//=============================================================================
// Init data (flash)
//=============================================================================
//  esp_init_data_default.bin
//-----------------------------------------------------------------------------
const uint8 esp_init_data_default[128] ICACHE_RODATA_ATTR = {
	    5,    0,    4,    2,    5,    5,    5,    2,    5,    0,    4,    5,    5,    4,    5,    5,
	    4, 0xFE, 0xFD, 0xFF, 0xF0, 0xF0, 0xF0, 0xE0, 0xE0, 0xE0, 0xE1,  0xA, 0xFF, 0xFF, 0xF8,    0,
	 0xF8, 0xF8, 0x52, 0x4E, 0x4A, 0x44, 0x40, 0x38,    0,    0,    1,    1,    2,    3,    4,    5,
	    1,    0,    0,    0,    0,    0,    2,    0,    0,    0,    0,    0,    0,    0,    0,    0,
	 0xE1,  0xA,    0,    0,    0,    0,    0,    0,    0,    0,    1, 0x93, 0x43,    0,    0,    0,
	    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
	    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
	    3,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0
};
#define esp_init_data_default_size 128
//=============================================================================
// extern funcs
//-----------------------------------------------------------------------------
//=============================================================================
// IRAM code
//=============================================================================
// call_user_start() - вызов из заголовка, загрузчиком
// ENTRY(call_user_start) in eagle.app.v6.ld
//-----------------------------------------------------------------------------
void call_user_start(void)
{
	    // Загрузка заголовка flash
	    struct SPIFlashHead fhead;
		SPI0_USER |= SPI_CS_SETUP; // +1 такт перед CS
		SPIRead(0, (uint32_t *)&fhead, sizeof(fhead));
		// Установка размера Flash от 256Kbytes до 32Mbytes
		// High four bits fhead.hsz.flash_size: 0 = 512K, 1 = 256K, 2 = 1M, 3 = 2M, 4 = 4M, ... 7 = 32M
	    uint32 fsize = fhead.hsz.flash_size & 7;
		if(fsize < 2) flashchip->chip_size = (8 >> fsize) << 16;
		else flashchip->chip_size = (4 << fsize) << 16;
	    uint32 fspeed = fhead.hsz.spi_freg;
		// Установка:
		// SPI Flash Interface (0 = QIO, 1 = QOUT, 2 = DIO, 0x3 = DOUT)
		// and Speed QSPI: 0 = 40MHz, 1= 26MHz, 2 = 20MHz, ... = 80MHz
		sflash_something(fspeed);
		// SPIFlashCnfig(fhead.spi_interface & 3, (speed > 2)? 1 : speed + 2);
		// SPIReadModeCnfig(5); // in ROM
		// Всё - включаем кеширование, далее можно вызывать процедуры из flash
		Cache_Read_Enable_def();
		// Инициализация
		startup();
		// Передача управления ROM-BIOS
		ets_run();
}
//-----------------------------------------------------------------------------
// Установка скорости QSPI
//  0 = 40MHz, 1 = 26MHz, 2 = 20MHz, >2 = 80MHz
//-----------------------------------------------------------------------------
void sflash_something(uint32 flash_speed)
{
	//	Flash QIO80:
	//  SPI_CTRL = 0x16ab000 : QIO_MODE | TWO_BYTE_STATUS_EN | WP_REG | SHARE_BUS | ENABLE_AHB | RESANDRES | FASTRD_MODE | BIT12
	//	IOMUX_BASE = 0x305
	//  Flash QIO40:
	//	SPI_CTRL = 0x16aa101
	//	IOMUX_BASE = 0x205
	uint32 xreg = (SPI0_CTRL >> 12) << 12; //  & 0xFFFFF000
	uint32 value;
	if (flash_speed > 2) { // 80 MHz
		value = BIT(12); // 0x60000208 |= 0x1000
		GPIO_MUX_CFG |= (1<< MUX_SPI0_CLK_BIT); // HWREG(IOMUX_BASE, 0) |= BIT(8);  // 80 MHz
	}
	else { // 0:40, 1:26, 2:20 MHz // 0x101, 0x202, 0x313
		value = 1 + flash_speed + ((flash_speed + 1) << 8) + ((flash_speed >> 1)<< 4);
		xreg &= ~BIT(12);  // 0x60000208 &= 0xffffefff
		GPIO_MUX_CFG &= MUX_CFG_MASK & (~(1<< MUX_SPI0_CLK_BIT)); // HWREG(IOMUX_BASE, 0) &= ~(BIT(8));  // 0x60000800 &=  0xeff
	}
	SPI0_CTRL = xreg | value;
}
//=============================================================================
// FLASH code (ICACHE_FLASH_ATTR)
//=============================================================================
// Чтение MAC из OTP
//-----------------------------------------------------------------------------
void ICACHE_FLASH_ATTR read_macaddr_from_otp(uint8 *mac)
{
	mac[0] = 0x18;
	mac[1] = 0xfe;
	mac[2] = 0x34;
	if ((!(OTP_CHIPID & (1 << 15)))||((OTP_MAC0 == 0) && (OTP_MAC1 == 0))) {
		mac[3] = 0x18;
		mac[4] = 0xfe;
		mac[5] = 0x34;
	}
	else {
		mac[3] = (OTP_MAC1 >> 8) & 0xff;
		mac[4] = OTP_MAC1 & 0xff;
		mac[5] = (OTP_MAC0 >> 24) & 0xff;
	}
}
//-----------------------------------------------------------------------------
// Очистка сегмента bss
//-----------------------------------------------------------------------------
/*void ICACHE_FLASH_ATTR mem_clr_bss(void)
{
	uint8 * ptr = &_bss_start;
	while(ptr < &_bss_end) *ptr++ = 0;
}*/
//-----------------------------------------------------------------------------
// Тест конфигурации для WiFi (будет переделан)
//-----------------------------------------------------------------------------
void ICACHE_FLASH_ATTR tst_cfg_wifi(void)
{
    struct s_wifi_store * wifi_config = &g_ic.g.wifi_store;
	wifi_softap_set_default_ssid();
	if(wifi_config->wfmode[0] == 0xff) wifi_config->wfmode[0] = 2;
	else wifi_config->wfmode[0] &= 3;
	wifi_config->wfmode[1] = 0;
	if(wifi_config->field_308[1] >= 14 || wifi_config->field_308[1] == 0) {
		wifi_config->field_308[1] = 1;
	}
	if(wifi_config->beacon >= 6000 || wifi_config->beacon < 100){ //
		wifi_config->beacon = 100;
	}
	wDev_Set_Beacon_Int((wifi_config->beacon/100)*102400);
	if(wifi_config->field_308[2] >= 5 || wifi_config->field_308[2] == 1){
		wifi_config->field_308[2] = 0;
		ets_bzero(wifi_config->ap_passw, 64);
	}
	if(wifi_config->field_308[3] > 2) wifi_config->field_308[3] = 0;
	if(wifi_config->field_308[4] > 4) wifi_config->field_308[4] = 4;
	if(wifi_config->st_ssid_len == 0xffffffff) {
		ets_bzero(&wifi_config->st_ssid_len, 36);
		ets_bzero(&wifi_config->st_passw, 64);
	}
	if(wifi_config->field_152[17] > 2) wifi_config->field_152[17] = 0; // +169
	if(wifi_config->field_316 > 6) wifi_config->field_316 = 1;
	wifi_config->phy_mode &= 3;
	if(wifi_config->phy_mode == 0 ) wifi_config->phy_mode = 3; // phy_mode
}
//=============================================================================
// Чтение wifi_config (последние сектора в заданном заголовком размере flash)
//-----------------------------------------------------------------------------
/* system_get_checksum() находится в другом модуле
uint32 ICACHE_FLASH_ATTR system_get_checksum(uint8 *ptr, uint32 len)
{
	uint8 checksum = 0xEF;
	while(len--) checksum ^= *ptr++;
	return checksum;
}*/
//-----------------------------------------------------------------------------
void ICACHE_FLASH_ATTR read_wifi_config(void)
{
	struct ets_store_wifi_hdr hbuf;

	spi_flash_read(flashchip->chip_size - 0x1000,(uint32 *)(&hbuf), sizeof(hbuf));
	uint32 store_cfg_addr = flashchip->chip_size - 0x3000 + ((hbuf.bank)? 0x1000 : 0);
	struct s_wifi_store * wifi_config = &g_ic.g.wifi_store;
	spi_flash_read(store_cfg_addr,(uint32 *)wifi_config, wifi_config_size);
	if(hbuf.flag != 0x55AA55AA || system_get_checksum((uint8 *)wifi_config, hbuf.xx[(hbuf.bank)? 1 : 0]) != hbuf.chk[(hbuf.bank)? 1 : 0]) {
#ifdef DEBUG_UART
		os_printf("\nError wifi_config! Clear.\n");
#endif
		ets_memset(wifi_config, 0xff, wifi_config_size);
	};
}
//=============================================================================
// startup()
//-----------------------------------------------------------------------------
void ICACHE_FLASH_ATTR startup(void)
{
	ets_set_user_start(call_user_start);
	// Очистка сегмента bss //	mem_clr_bss();
	uint8 * ptr = &_bss_start;
	while(ptr < &_bss_end) *ptr++ = 0;
//	user_init_flag = false; // mem_clr_bss
	//
	_xtos_set_exception_handler(9, default_exception_handler);
	_xtos_set_exception_handler(0, default_exception_handler);
	_xtos_set_exception_handler(2, default_exception_handler);
	_xtos_set_exception_handler(3, default_exception_handler);
	_xtos_set_exception_handler(28, default_exception_handler);
	_xtos_set_exception_handler(29, default_exception_handler);
	_xtos_set_exception_handler(8, default_exception_handler);
	// cтарт на модуле с кварцем в 26MHz, а ROM-BIOS выставил 40MHz?
	if(rom_i2c_readReg(103,4,1) == 8) { // 8: 40MHz, 136: 26MHz
#ifdef DEBUG_UART
		if(read_sys_const(sys_const_soc_param0) == 1) { // soc_param0: 0: 40MHz, 1: 26MHz, 2: 24MHz
#ifdef NO_SET_UART_BAUD
			// set 80MHz PLL CPU
			rom_i2c_writeReg(103,4,1,136);
			rom_i2c_writeReg(103,4,2,145);
			UART0_CLKDIV = (UART0_CLKDIV * 394) >> 8;
			UART1_CLKDIV = (UART1_CLKDIV * 394) >> 8;
			ets_delay_us(150);
//			ets_update_cpu_frequency(80); // set clk cpu (rom-bios set default 80)
#else
			ets_update_cpu_frequency(80); // указание правильной частоты для подсчета времени в ets_delay_us(), т.к. она считает такты CPU и множит на указанное число в MHz...
			if(UART0_CLKDIV == CALK_UART_CLKDIV(115200)) {
				UART0_CLKDIV = CALK_UART_CLKDIV_26QZ(DEFAULT_BIOS_UART_BAUD);
				UART1_CLKDIV = CALK_UART_CLKDIV_26QZ(DEFAULT_BIOS_UART_BAUD);
			}
//			os_printf("\nSet UARTs_CLKDIV 74880 Baud (BIOS set: %u)\n", UartDev.baut_rate);
#endif
		}
#endif
	}
/*	//
	if(rtc_get_reset_reason() < 2) { // 1 - power/ch_pd, 2 - reset, 3 - software, 4 - wdt ...)
		if((RTC_RAM_BASE[24] & 0xffff) == 0) { // *((uint32 *)0x60001060 bit0..15: старт был по =1 reset, =0 ch_pd, bit16..31: deep_sleep_option
		// старт со сбитыми в RTC данными, если не подключен VCC_RTC
		}
	} */
//	os_printf("Reset reason: %u/%u, rf/deep_sleep option: %u.\n", rtc_get_reset_reason(), RTC_RAM_BASE[0x60>>2]&0xFFFF, RTC_RAM_BASE[0x60>>2]>>16);
	// Тест системных данных в RTС
	if((RTC_RAM_BASE[0x60>>2]>>16) > 4) { // проверка опции phy_rfoption = deep_sleep_option
#ifdef DEBUG_UART
		os_printf("\nError phy_rfoption! Set default = 0.\n");
#endif
		RTC_RAM_BASE[0x60>>2] &= 0xFFFF;
		RTC_RAM_BASE[0x78>>2] = 0; // обнулить Espressif OR контрольку области 0..0x78 RTC_RAM
	}
	//
	read_wifi_config();
#ifdef USE_OPEN_LWIP	
	default_hostname = true;
#endif	
	//
	sleep_reset_analog_rtcreg_8266();
	//
	read_macaddr_from_otp(info.st_mac);
	wifi_softap_cacl_mac(info.ap_mac, info.st_mac);
	//
	info.ap_gw = info.ap_ip = 0x104A8C0; // ip 192.168.4.1
	info.ap_mask = 0x00FFFFFF; // 255.255.255.0
	ets_timer_init();
	lwip_init();
//	espconn_init();
	// set up a timer for lwip
	ets_timer_disarm(&check_timeouts_timer);
	ets_timer_setfn(&check_timeouts_timer, (ETSTimerFunc *) sys_check_timeouts, NULL);
	// wifi_set_sleep_type: NONE = 25, LIGHT = 3000 + reset_noise_timer(3000), MODEM = 25 + reset_noise_timer(100);
	ets_timer_arm_new(&check_timeouts_timer, 25, 1, 1);
	//
	tst_cfg_wifi();
#ifdef USE_DUAL_FLASH
	overlap_hspi_init(); // не используется для модулей с одной flash!
#endif
	//
	uint8 *buf = (uint8 *)pvPortMalloc(esp_init_data_default_size); // esp_init_data_default.bin
	spi_flash_read(flashchip->chip_size - 0x4000,(uint32 *)buf, esp_init_data_default_size); // esp_init_data_default.bin
	//
	if(buf[0] != 5) {
#ifdef DEBUG_UART
		os_printf("\nError esp_init_data! Set default.\n");
#endif
		ets_memcpy(buf, esp_init_data_default, esp_init_data_default_size);
	}
	init_wifi(buf, info.st_mac);
	vPortFree(buf);
	// print sdk version
#ifdef DEBUG_UART
	os_printf("\nOpenLoaderSDK v1.2\n");
	//
	os_print_reset_error(); // вывод фатальных ошибок, вызвавших рестарт. см. в модуле wdt
	//
#endif
//	DPORT_BASE[0] = (DPORT_BASE[0] & 0x60) | 0x0F; // ??
#if SDK_VERSION >= 1119 // (SDK 1.1.1)
	wdt_init(1);
#else
	wdt_init();
#endif
	user_init();
	user_init_flag = true;
	WDT_FEED = WDT_FEED_MAGIC; // WDT
	//
	int wfmode = g_ic.g.wifi_store.wfmode[0];
	wifi_mode_set(wfmode);
	if(wfmode & 1)  wifi_station_start();
	if(wfmode & 2)  wifi_softap_start();
	if(wfmode) {
		struct netif * * p;
		if(wfmode == 1) p = g_ic.g.netif1; // g_ic+0x10;
		else p = g_ic.g.netif2; // g_ic+0x14;
		netif_set_default(*p);	// struct netif *
	}
	if(wifi_station_get_auto_connect()) wifi_station_connect();
	if(done_cb != NULL) done_cb();
}
//-----------------------------------------------------------------------------
#ifdef DEBUG_UART
void ICACHE_FLASH_ATTR puts_buf(uint8 ch)
{
	if(UartDev.trx_buff.TrxBuffSize < TX_BUFF_SIZE)
		UartDev.trx_buff.pTrxBuff[UartDev.trx_buff.TrxBuffSize++] = ch;
}
#endif
//=============================================================================
// init_wifi()
//-----------------------------------------------------------------------------
const char aFATAL_ERR_R6PHY[] ICACHE_RODATA_ATTR = "register_chipv6_phy";
void ICACHE_FLASH_ATTR init_wifi(uint8 * init_data, uint8 * mac)
{
#ifdef DEBUG_UART
	uart_wait_tx_fifo_empty();
#ifndef NO_SET_UART_BAUD
	UartDev.trx_buff.TrxBuffSize = 0;
	ets_install_putc1(puts_buf);
#endif
#endif
	if(register_chipv6_phy(init_data)){
		fatal_error(FATAL_ERR_R6PHY, init_wifi, (void *)aFATAL_ERR_R6PHY);
	}
   	ets_update_cpu_frequency(80);
#ifdef DEBUG_UART
#ifndef NO_SET_UART_BAUD
   	{
   		uint32 x;
   	  	if(UART0_CLKDIV != CALK_UART_CLKDIV(115200)) {
   	  		x = (UART0_CLKDIV * 394) >> 8;
   	   	}
   	  	else x = CALK_UART_CLKDIV(DEFAULT_BIOS_UART_BAUD);
       	UART0_CLKDIV = x;
       	UART1_CLKDIV = x;
   	}
	ets_install_uart_printf();
	if(UartDev.trx_buff.TrxBuffSize) ets_printf(UartDev.trx_buff.pTrxBuff);
	UartDev.trx_buff.TrxBuffSize = TX_BUFF_SIZE;
#endif
#endif
	phy_disable_agc();
	ieee80211_phy_init(g_ic.g.wifi_store.phy_mode); // phy_mode
	lmacInit();
	wDev_Initialize(mac);
	pp_attach();
	ieee80211_ifattach(&g_ic);
	ets_isr_attach(0, wDev_ProcessFiq, NULL);
	ets_isr_unmask(1);
	pm_attach();
	phy_enable_agc();
	cnx_attach(&g_ic);
	wDevEnableRx();
}
//=============================================================================
// uart_wait_tx_fifo_empty()
//-----------------------------------------------------------------------------
void ICACHE_FLASH_ATTR uart_wait_tx_fifo_empty(void)
{
	while((UART0_STATUS >> UART_TXFIFO_CNT_S) & UART_TXFIFO_CNT);
	while((UART1_STATUS >> UART_TXFIFO_CNT_S) & UART_TXFIFO_CNT);
}
//=============================================================================
// user_uart_wait_tx_fifo_empty()
// Use SDK Espressif
//-----------------------------------------------------------------------------
void user_uart_wait_tx_fifo_empty(uint32 uart_num, uint32 x)
{
	if(uart_num) while((UART1_STATUS >> UART_TXFIFO_CNT_S) & UART_TXFIFO_CNT);
	else while(((UART0_STATUS >> UART_TXFIFO_CNT_S) & UART_TXFIFO_CNT) && (x--));
}


