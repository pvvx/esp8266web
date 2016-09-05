/******************************************************************************
 * FileName: startup.c
 * Description: disasm user_interface functions SDK 1.4.0... (libmain.a)
 * Author: PV`
 * (c) PV` 2015
 ******************************************************************************/
#include "user_config.h"
#include "bios.h"
#include "hw/esp8266.h"
#include "hw/specreg.h"
#include "hw/gpio_register.h"
#include "hw/spi_register.h"
#include "hw/uart_register.h"
#include "hw/corebits.h"
#include "hw/specreg.h"
//#include "user_interface.h"
#include "sdk/add_func.h"
#include "os_type.h"
#include "user_interface.h"
#include "sys_const.h"

#define FLASH_ID_X1   0x001640C8
#define FLASH_ID_X2   0x001840C8
#define FLASH_ID_XMASK 0x00FFFFFF

const uint8 esp_init_data_default[128] ICACHE_RODATA_ATTR = {
	    5,    0,    4,    2,    5,    5,    5,    2,    5,    0,    4,    5,    5,    4,    5,    5,
	    4, 0xFE, 0xFD, 0xFF, 0xF0, 0xF0, 0xF0, 0xE0, 0xE0, 0xE0, 0xE1,  0xA, 0xFF, 0xFF, 0xF8,    0,
	 0xF8, 0xF8, 0x52, 0x4E, 0x4A, 0x44, 0x40, 0x38,    0,    0,    1,    1,    2,    3,    4,    5,
	    1,    0,    0,    0,    0,    0,    2,    0,    0,    0,    0,    0,    0,    0,    0,    0,
	 0xE1,  0xA,    0,    0,    0,    0,    0,    0,    0,    0,    1, 0x93, 0x43,    0,    0,    0,
	    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
	    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
	    3,    0,    2,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0
};
#define esp_init_data_default_size 128

void __attribute__((section(".vectors.text"))) __attribute__ ((noreturn)) call_user_start(void)
{
	__asm__ __volatile__ (
"vectors_base:	.word	0x40100000\n"
			"ldr	a2, vectors_base\n"
			"wsr.vecbase	a2\n"
			"call0	call_user_start_local\n"
			);
}

void call_user_start_local(void)
{
	startup();
	ets_run();
}

LOCAL void sflash_something(uint32 flash_speed)
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

uint8 dual_flash_flag = 0xff;

int printf_s_spiread(const char *s)
{
	uint8 buf[36];
	SPIRead((uint32)s - 0x40200000, buf, 36);
	return ets_printf(buf);
}

// Очистка сегмента bss //	mem_clr_bss();
void mem_clr_bss(void) {
	uint8 * ptr = &_bss_start;
	while(ptr < &_bss_end) *ptr++ = 0;
}

LOCAL void SystemParamErr(void) // uint32 fsector)
{
	os_printf("system param error\n");
/*	// далее нет в SDK 2.0.0
	print_hex_flash_buf(fsector, sizeof(struct s_wifi_store));
	print_hex_flash_buf(fsector+1, sizeof(struct s_wifi_store));
	print_hex_flash_buf(fsector+2, sizeof(struct ets_store_wifi_hdr));
*/
}

LOCAL void sdk_tx_char(int c)
{
	uint32 t = system_get_time();

	while(((UART1_STATUS >> UART_TXFIFO_CNT_S) & UART_TXFIFO_CNT) <= 125) {
		if((system_get_time() - t) > 100000) return;
	}
	UART1_FIFO = c;
	return;
}

LOCAL void sdk_putc1(char c)
{
	if (c != '\r') {
		if (c != '\n') sdk_tx_char(c);
		else {
			sdk_tx_char('\r');
			sdk_tx_char('\n');
		}
	}
}
/* в libmain.a: spi_flash.o
LOCAL int spi_flash_enable_qmode(void)
{
	Cache_Read_Disable_2();
	int ret = Enable_QMode(flashchip);
	Wait_SPI_Idle();
	Cache_Read_Enable_2();
	return ret;
}
*/
LOCAL void startup(void)
{
	uint8 mac[6];
	struct SPIFlashHead fhead;
	struct ets_store_wifi_hdr hbuf;
	struct s_wifi_store wscfg;
	uint32 xspeed;

	ets_install_putc1(sdk_putc1);
	SPI0_USER |= SPI_CS_SETUP;
	read_mac(mac);
	SPIRead(0, (uint32_t *)&fhead, sizeof(fhead));
	if(((GPIO_IN >> 16) & 7) != 5) {
		if(fhead.hsz.spi_freg < 3) xspeed = fhead.hsz.spi_freg + 2;
		else {
			if(fhead.hsz.spi_freg == 0x0f) xspeed = 1;
			else xspeed = 2;
		}
	}
	else xspeed = 2;
	uint32 flash_size = 0x80000;
	switch(fhead.hsz.flash_size)
	{
	case 1:
		flashchip->chip_size = 0x40000;
		dual_flash_flag = 0;
		break;
	case 2:
		flashchip->chip_size = 0x100000;
		dual_flash_flag = 0;
		break;
	case 3:
		flashchip->chip_size = 0x200000;
		dual_flash_flag = 0;
		break;
	case 4:
		flashchip->chip_size = 0x400000;
		dual_flash_flag = 0;
		break;
	case 5:
		flashchip->chip_size = 0x200000;
		break;
	case 6:
		flashchip->chip_size = 0x40000;
		break;
	case 0:
	default:
		flashchip->chip_size = 0x80000;
		dual_flash_flag = 0;
		break;
	}
	sflash_something(xspeed);

	uint32 cfg_flash_addr = (flashchip->chip_size/flashchip->sector_size) - 3;
	SPIRead((cfg_flash_addr + 2)*flashchip->sector_size, &hbuf, sizeof(hbuf)); // read 32 bytes ets_store_wifi_hdr
	SPIRead((cfg_flash_addr + ((hbuf.bank)? 1 : 0))*flashchip->sector_size, &wscfg, sizeof(wscfg)); // 1156 байт для SDK 1.4.0, 1172 для SDK 2.0.0
	if(fhead.hsz.flash_size != 5 && fhead.hsz.flash_size != 6) {
		if((wscfg.boot_info[1]&0x1F) > 4 || (wscfg.boot_info[1]&0x1F) < 32 ) dual_flash_flag = 1;
		else {
			printf_s_spiread("need boot 1.4+\n");
			while(1);
		}
	}
	Cache_Read_Enable_New();
	mem_clr_bss();
	ets_install_putc1(sdk_putc1); // повторно !
	if(hbuf.flag != 0x55AA55AA || hbuf.flag != 0xFFFFFFFF) SystemParamErr(cfg_flash_addr);
	else if(hbuf.flag2 == 0xAA55AA55 || hbuf.flag2 == 0xFFFFFFFF) {
		if(system_get_checksum((uint8 *)wscfg, hbuf.xx[(hbuf.bank)? 1 : 0]) != hbuf.chk[(hbuf.bank)? 1 : 0])
			SystemParamErr();
	}
	else SystemParamErr();

	uint32 flash_id = spi_flash_get_id(); // в libmain.a: spi_flash.o
	bool ret;
	if((flash_id & 0xFF) == 0x9D || (flash_id & 0xFF) == 0xC2) {
			ret = (spi_flash_issi_enable_QIO_mode() == 1); // в libmain.a: spi_flash.o
	}
	else if((flash_id & FLASH_ID_XMASK) == FLASH_ID_X1 || (flash_id & FLASH_ID_XMASK) == FLASH_ID_X2) {
			ret = (flash_gd25q32c_enable_QIO_mode() == 1); // в libmain.a: spi_flash.o
	}
	else ret = (spi_flash_enable_qmode() == 0); // в libmain.a: spi_flash.o
	if(ret) {
		SPI0_CTRL &= 0xFE6F9FFF;
		SPI0_CTRL |= 0x1002000;
	};
	ets_memcpy(g_ic.g.wifi_store, wscfg, sizeof(wscfg));
	sdk_init();
}

void ICACHE_FLASH_ATTR tst_cfg_wifi(void)
{
    struct s_wifi_store * wifi_config = &g_ic.g.wifi_store;
    int flg = 0;
    if(g_ic.c[540] == 0xff) {
    	flg = 1;
    	g_ic.c[540] = 2;
    }
	wifi_softap_set_default_ssid();
	wifi_station_set_default_hostname(info.st_mac);
	if(wifi_config->wfmode[0] == 0xff) wifi_config->wfmode[0] = SOFTAP_MODE;
	else wifi_config->wfmode[0] &= 3;
	wifi_config->wfmode[1] = 0;
	if(wifi_config->wfchl >= 14 || wifi_config->wfchl == 0) {
		wifi_config->wfchl = 1;
	}
	if(wifi_config->beacon >= 6000 || wifi_config->beacon < 100){ //
		wifi_config->beacon = 100;
	}
	wDev_Set_Beacon_Int((wifi_config->beacon/100)*102400);
	if(wifi_config->field_310 >= 5 || wifi_config->field_310 == 1){
		wifi_config->field_310 = 0;
		ets_bzero(wifi_config->ap_passw, 64);
	}
	if(wifi_config->field_311 > 2) wifi_config->field_311 = 0;
	if(wifi_config->field_312 > 8) wifi_config->field_312 = 4;
	if(wifi_config->st_ssid_len == 0xffffffff) {
		ets_bzero(&wifi_config->st_ssid_len, 36);
		ets_bzero(&wifi_config->st_passw, 64);
	}
	wifi_config->field_880 = 0;
	wifi_config->field_884 = 0;
	if(wifi_config->field_316 > 6) wifi_config->field_316 = 1;
	if(wifi_config->field_169 > 2) wifi_config->field_169 = 0; // +169
	wifi_config->phy_mode &= 3;
	if(wifi_config->phy_mode == 0 ) wifi_config->phy_mode = 3; // phy_mode
	if(flg) {
		// sub_402100C4():
		{
			uint8 * tst_ch = &g_ic.c[1688];
			if(tst_ch[0] != 0 && tst_ch[0] != 1) tst_ch[0] = 0;
			if(tst_ch[1] == 1) {
				if(!((tst_ch[6] == 0x49 || tst_ch[6] == 0x4F || tst_ch[6] == 0x20) // 'I','O',' '
				&& ((tst_ch[10] + tst_ch[11]) < 0x10)))
				{
					uint16 * tst_u16 = (uint16 *) tst_ch;
					tst_u16[1] = 0x81;
					tst_u16[2] = 0x9C;
					tst_ch[6] = 0x20;
					tst_ch[7] = 0x43; //'C'
					tst_ch[8] = 0x4E; //'N'
					tst_ch[9] = 1;
					tst_ch[10] = 1;
					tst_ch[11] = 0x0E;
				}
				if(ieee80211_chan_in_regdomain(wifi_config->wfchl) == 0) wifi_config->wfchl = tst_ch[10];
			}
			else ets_memset(tst_ch, 0, 14);
		}
		system_param_save_with_protect((flashchip->chip_size/flashchip->sector_size) - 3, wifi_config, wifi_config_size);
	}
}

int flash_data_check(uint8 *check_buf)
{
	uint32 cbuf[32];
	uint32 *pcbuf = cbuf;
	uint8 *pbuf = check_buf;
	int i = 27;
	while(i--) {
		*pcbuf = pbuf[0] | (pbuf[1]<<8) | (pbuf[2]<<16) | (pbuf[3]<<24);
		pcbuf++;
		pbuf+=4;
	}
	cbuf[24] = (OTP_MAC1 & 0xFFFFFFF) | ((OTP_CHIPID & 0xF000)<<16);
	cbuf[25] = (dport_[23] & 0xFFFFFFF) | (OTP_MAC0 & 0xFF000000);
	pcbuf = cbuf;
	uint32 xsum = 0;
	do	xsum += *pcbuf++;
	while(pcbuf != &cbuf[26]);
	xsum ^= 0xFFFFFFFF;
	if (cbuf[26] != xsum) return 1;
	return 0;
}

extern uint8 phy_rx_gain_dc_flag;
extern uint8 * phy_rx_gain_dc_table;
extern sint16 TestStaFreqCalValInput;
uint8  SDK_VERSION = "2.0.0";
extern struct rst_info rst_inf;
extern uint8 freq_trace_enable;

void ICACHE_FLASH_ATTR sdk_init(void)
{
	_xtos_set_exception_handler(EXCCAUSE_UNALIGNED, default_exception_handler);
	_xtos_set_exception_handler(EXCCAUSE_ILLEGAL, default_exception_handler);
	_xtos_set_exception_handler(EXCCAUSE_INSTR_ERROR, default_exception_handler);
	_xtos_set_exception_handler(EXCCAUSE_LOAD_STORE_ERROR, default_exception_handler);
	_xtos_set_exception_handler(EXCCAUSE_LOAD_PROHIBITED, default_exception_handler);
	_xtos_set_exception_handler(EXCCAUSE_STORE_PROHIBITED, default_exception_handler);
	_xtos_set_exception_handler(EXCCAUSE_PRIVILEGED, default_exception_handler);
	//
	sleep_reset_analog_rtcreg_8266();
	//
	read_macaddr_from_otp(info.st_mac);
	wifi_softap_cacl_mac(info.ap_mac, info.st_mac);
	//
	info.ap_gw = info.ap_ip = 0x104A8C0; // ip 192.168.4.1
	info.ap_mask = 0x00FFFFFF; // 255.255.255.0
	//
	ets_timer_init();
	lwip_init();
	espconn_init();
	//
	lwip_timer_interval = 25;
	ets_timer_setfn(&check_timeouts_timer, (ETSTimerFunc *) sys_check_timeouts, NULL);
	//
	tst_cfg_wifi();
	//
	if(PERI_IO_SWAP & 1) system_uart_swap();
	else system_uart_de_swap();
	//
	while((UART0_STATUS >> UART_TXFIFO_CNT_S) & UART_TXFIFO_CNT);
	while((UART1_STATUS >> UART_TXFIFO_CNT_S) & UART_TXFIFO_CNT);
	//
	GPIO0_MUX &= 0xfffffeff;
	GPIO2_MUX &= 0xfffffeff;
	//
	struct rst_info * ri = (struct rst_info *) os_zalloc(sizeof(struct rst_info));
	system_rtc_mem_read(0, &rst_if, sizeof(struct rst_info));
	uint32 x = IOREG(0x60000730); // IO_RTC_SCRATCH0
	if(x == 0) {
		if (rtc_get_reset_reason() == 1) { // ch_pd
			ets_memset(&rst_if, 0, sizeof(struct rst_info));
		}
		else {
			if(rtc_get_reset_reason() == 2) { // pin reset
				if(rst_if.reason != REASON_DEEP_SLEEP_AWAKE || rst_if.epc1 != 0 || rst_if.excvaddr != 0) {
					ets_memset(&rst_if, 0, sizeof(struct rst_info));
					rst_if.reason = REASON_EXT_SYS_RST;
				}
			}
			else rtc_get_reset_reason(); // ??? 8)
		}
	} else if(x > 7) ets_memset(&rst_if, 0, sizeof(struct rst_info));
	system_rtc_mem_write(0, &rst_if, sizeof(struct rst_info));
	//
	rf_cal_sec = user_rf_cal_sector_set();
	int fsec_all = flashchip->chip_size/flashchip->sector_size;
	if(rf_cal_sec == 0 || rf_cal_sec > fsec_all)  rf_cal_sec = fsec_all - 5;
	//
	uint8 * pbuf = os_malloc(756);
	spi_flash_read(((flashchip->chip_size/flashchip->sector_size) - 4)*flashchip->sector_size,(uint32 *)pbuf, 128); // esp_init_data_default.bin
	//
	spi_flash_read(rf_cal_sec*flashchip->sector_size + 128,(uint32 *)pbuf + 128, 628); // rf_cal_sec
	//
	phy_rx_gain_dc_table = &pbuf[0x100];
	int xflg;
	if(flash_data_check(&pbuf[128]) != 0 || phy_check_data_table(phy_rx_gain_dc_table, 125, 1) != 0) xflg = 1;
	else xflg = 0;
	if(rst_if.reason != REASON_DEEP_SLEEP_AWAKE) {
		phy_afterwake_set_rfoption(1);
	}
	if(rst_if.reason != REASON_DEEP_SLEEP_AWAKE && xflg == 0) {
		write_data_to_rtc(&pbuf[128]);
	}
	pbuf[0xf8] = 0;
//	phy_rx_gain_dc_flag = 0;
	//
	user_rf_pre_init();
	//
	if(pbuf[0] != 5) { // esp_init_data_default[0] != 5
		os_printf("rf_cal[0] !=0x05,is 0x%02X\n", pbuf[0]);
		ets_delay_us(10000);
		system_restart_local();
		ets_memcpy(pbuf, esp_init_data_default, esp_init_data_default_size);
	}
	else {
		int ffoff = (signed char)pbuf[sys_const_force_freq_offset];
		if(freq_trace_enable != 0) {
			if(freq_trace_enable == 1) {
				if(pbuf[sys_const_freq_correct_en] >= 2) {
					if(pbuf[sys_const_freq_correct_en] == 3) pbuf[sys_const_freq_correct_en] = 3;
					else if(pbuf[sys_const_freq_correct_en] == 5) {
						if(ffoff < 0) pbuf[sys_const_freq_correct_en] = 11;
						else if(ffoff >= 7) pbuf[sys_const_freq_correct_en] = 9;
						else {
							pbuf[sys_const_force_freq_offset] = 0;
							pbuf[sys_const_freq_correct_en] = 3;
						}
					}
					else if(pbuf[sys_const_freq_correct_en] == 7) {
						if(ffoff < 0) {
							pbuf[sys_const_freq_correct_en] = 11;
						}
						else {
							pbuf[sys_const_force_freq_offset] = 0;
							pbuf[sys_const_freq_correct_en] = 3;
						}
					}
					else if(pbuf[sys_const_freq_correct_en] == 9) {
						if(ffoff < 0) pbuf[sys_const_freq_correct_en] = 11;
						else if(ffoff >= 7) pbuf[sys_const_freq_correct_en] = 9;
						else {
							pbuf[sys_const_force_freq_offset] = 0;
							pbuf[sys_const_freq_correct_en] = 3;
						};
					}
					else if(pbuf[sys_const_freq_correct_en] == 11) {
						if(ffoff < 0) {
							pbuf[sys_const_freq_correct_en] = 11;
						}
						else {
							pbuf[sys_const_force_freq_offset] = 0;
							pbuf[sys_const_freq_correct_en] = 3;
						}
					}
				}
			}
			else if(pbuf[sys_const_freq_correct_en] >= 0) pbuf[sys_const_freq_correct_en] = 0;
			else if(pbuf[sys_const_freq_correct_en] == 3) pbuf[sys_const_freq_correct_en] = 0;
			else if(pbuf[sys_const_freq_correct_en] == 5) {
				if(ffoff < 0) {
					pbuf[sys_const_freq_correct_en] = 7;
				}
				else if(ffoff >= 7) pbuf[sys_const_freq_correct_en] = 5;
				else {
					pbuf[sys_const_freq_correct_en] = 0;
					pbuf[sys_const_force_freq_offset] = 0;
				}
			}
			else if(pbuf[sys_const_freq_correct_en] == 7) {
				if(ffoff < 0) {
					pbuf[sys_const_freq_correct_en] = 7;
				}
				else {
					pbuf[sys_const_force_freq_offset] = 0;
					pbuf[sys_const_freq_correct_en] = 0;
				}
			}
		}
	}
	//
	init_wifi(pbuf, info.st_mac);
	//
	g_ic.c[491] = 0;
	if((pbuf[sys_const_freq_correct_en] & 5) == 1) g_ic.c[491] = 1;
	os_printf("rf cal sector: %d\n",rf_cal_sec);
	os_printf("rf[112] : %02x\n", pbuf[112]); // sys_const_freq_correct_en
	os_printf("rf[113] : %02x\n", pbuf[113]); // sys_const_force_freq_offset
	os_printf("rf[114] : %02x\n", pbuf[114]); // ?
	if(xflg != 0) {
		if((rtc_get_reset_reason() == 1) // ch_pd
		|| ((rst_if.reason != REASON_DEEP_SLEEP_AWAKE) && (rtc_get_reset_reason() == 2))) { // pin reset
			os_printf("w_flash\n");
			get_data_from_rtc(pbuf+128);
			wifi_param_save_protect_with_check(rf_cal_sec, flashchip->sector_size, pbuf + 128, 628);
		}
	}

	os_free(pbuf);

	os_printf("\nSDK ver: %s compiled @ Aug  9 2016 15:12:27\n", SDK_VERSION);
	os_printf("phy ver: %d, pp ver: %d.%d\n\n", (*((volatile uint32 *)0x6000107C))>>16, ((*((volatile uint32 *)0x600011F8))>>8)&0xFF, (*((volatile uint32 *)0x600011F8))&0xFF);


	if((g_ic.c[491] & 1) != 0) {
		if(rst_if.reason == REASON_DEEP_SLEEP_AWAKE) {
			TestStaFreqCalValInput = RTC_RAM_BASE[30]>>16; //(*((volatile int *)0x60001078))/65536;
			chip_v6_set_chan_offset(1, TestStaFreqCalValInput);
		}
		else {
			TestStaFreqCalValInput = 0;
			RTC_RAM_BASE[30] &= 0xFFFF; // *((volatile uint32 *)0x60001078) &= &0xFFFF;
		}
	}
	system_rtc_mem_write(0, ri, sizeof(struct rst_info));
	os_free(ri);
	//
	wdt_init(1);
	//
	user_init();
	//
	ets_timer_disarm(check_timeouts_timer);
	ets_timer_arm_new(check_timeouts_timer, lwip_timer_interval, 1, 1);
	//
	WDT_FEED = 0x73;
	user_init_flag = 1;
	//
	int wfmode = g_ic.g.wifi_store.wfmode[0]; // g_ic.c[0x214] (+532) SDK 1.2.0 // SDK 1.3.0 g_ic.c[472] // SDK 1.4.0 g_ic.c[508]
	wifi_mode_set(wfmode);
	if(wfmode & 1)  wifi_station_start();
	if(wfmode == 2) {
		if(g_ic.c[490] != 2) wifi_softap_start(0); // g_ic.c[470] SDK 1.5.4
		else wifi_softap_start(1);
	}
	else if(wfmode == 3) {
		wifi_softap_start(0);
	}
	wifi_mode_set();
	//
	if(wfmode == 1) netif_set_default(*g_ic.g.netif1);	// struct netif *
	if(wifi_station_get_auto_connect()) wifi_station_connect();
	if(done_cb != NULL) done_cb();
}

uint8 mai[] = "mai";

void ICACHE_FLASH_ATTR init_wifi(uint8 * init_data, uint8 * mac)
{
	if(register_chipv6_phy(init_data)){
		ets_printf("%s %u\n", "mai", 225);
		while(1);
	}
	uart_div_modify(0, 1068);
	uart_div_modify(1, 1068);
	phy_disable_agc();
	ieee80211_phy_init(g_ic.g.wifi_store.phy_mode); // phy_mode
	lmacInit();
	wDev_Initialize(mac);
	pp_attach();
	ieee80211_ifattach(&g_ic);
	ets_isr_attach(0, wDev_ProcessFiq, NULL);
	ets_isr_unmask(1);
	pm_attach();
	fpm_attach();
	phy_enable_agc();
	cnx_attach(&g_ic);
	wDevEnableRx();
}
