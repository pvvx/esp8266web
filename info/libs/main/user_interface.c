/******************************************************************************
 * FileName: user_interface.c
 * Description: disasm user_interface functions SDK 1.1.0 (libmain.a + libpp.a)
 * Author: PV`
 * (c) PV` 2015
 ******************************************************************************/
#include "user_config.h"
#include "bios.h"
#include "hw/esp8266.h"
#include "hw/specreg.h"
#include "hw/gpio_register.h"
//#include "user_interface.h"
#include "add_sdk_func.h"
#include "os_type.h"
#include "user_interface.h"

extern uint32 WdevTimOffSet;

uint8_t os_flg[4];
#define os_pint_ena 	os_flg[0]
#define timer2_ms_flag 	os_flg[1]
#define dhcps_flag 		os_flg[2]
#define dhcpc_flag 		os_flg[3]


extern uint8 * hostname; // in eagle_lwip_if.c
bool default_hostname = true;

uint8 * wifi_station_get_hostname(void)
{
	uint32 opmode = wifi_get_opmode();
	if(opmode == 1 || opmode == 3) {
		return hostname;
	}
	return NULL;
}

void wifi_station_set_default_hostname(uint8 * mac)
{
	if(hostname != NULL)	{
		vPortFree(hostname);
		hostname = NULL;
	}
	hostname = pvPortMalloc(32);
	if(hostname == NULL) {
		ets_sprintf(hostname,"ESP_%02X%02X%02X", mac[3], mac[4], mac[5]);
	}
}

bool wifi_station_set_hostname(uint8 * name)
{
	if(name == NULL) return false;
	uint32 len = ets_strlen(name);
	if(len > 32) return false;
	uint32 opmode = wifi_get_opmode();
	if(opmode == 1 || opmode == 3) {
		default_hostname = false;
		if(hostname.phostname != NULL) {
			vPortFree(hostname.phostname);
			hostname.phostname = NULL;
		}
		hostname.phostname = pvPortMalloc(len);
		if(hostname.phostname == NULL) return false;
		struct netif * nif = eagle_lwip_getif(0);
		ets_strcpy(hostname.phostname, name);
		if(nif != NULL) {
			nif->hostname = hostname.phostname;
		}
		return true;
	}
	return false;
}


uint32 system_phy_temperature_alert(void)
{
	return phy_get_check_flag(0); // in libphy.a
}

uint32 system_phy_set_max_tpw(uint32 tpw)
{
	return phy_set_most_tpw(tpw);
}

uint32 system_get_time(void)
{
	return WdevTimOffSet + IOREG(0x3FF20C00);
}

bool system_os_post(uint8 prio, os_signal_t sig, os_param_t par)
{
	if(prio >= USER_TASK_PRIO_MAX) {
		os_printf_plus("err: post prio < %d\n", USER_TASK_PRIO_MAX);
		return false;
	}
	return ets_post((prio + 22) & 0xFF, sig, par); // in ets.h
}

bool system_os_task(os_task_t task, uint8 prio, os_event_t *queue, uint8 qlen)
{
	if(prio < USER_TASK_PRIO_MAX) {
		os_printf("err: task prio < %d\n", USER_TASK_PRIO_MAX);
		return false;
	}
	if(qlen == 0 || queue == NULL) {
		os_printf("err: task queue error\n");
		return false;
	}
	ets_task(task, prio +22, queue, qlen);
	return true;
}

bool system_rtc_mem_write(uint8 des_addr, const void *src_addr, uint16 save_size)
{
	int idx;
	const uint32_t *d = src_addr;

	if ((des_addr >= (RTC_MEM_SIZE>>2)) || ((uint32_t)src_addr & 0x3) || ((des_addr << 2) + save_size > RTC_MEM_SIZE)) {
		return 0;
	}
	if(save_size & 3) {
		save_size += 4;
		save_size &= ~3;
	}

	des_addr <<= 2;

	for (idx = 0; idx < save_size; idx += 4) {
		RTC_MEM(des_addr + idx) = *d++;
	}

	return 1;
}


bool system_rtc_mem_read(uint8 src_addr, void *des_addr, uint16 load_size)
{
	int idx;
	uint32_t *d = des_addr;

	if ((src_addr >= (RTC_MEM_SIZE>>2)) || ((uint32_t)des_addr & 0x3) || ((src_addr << 2) + load_size > RTC_MEM_SIZE)) {
		return 0;
	}

	if(load_size & 3) {
		load_size += 4;
		load_size &= ~3;
	}

	src_addr <<= 2;

	for (idx = 0; idx < load_size; idx += 4) {
		*d++ = RTC_MEM(src_addr + idx);
	}

	return 1;
}


uint8 ICACHE_FLASH_ATTR system_get_os_print(void)
{
	return os_pint_ena;
}

void system_pp_recycle_rx_pkt(pkt * pkt)
{
	ppRecycleRxPkt(pkt);
}

uint16 system_adc_read(void)
{
	return test_tout(0); // uint16 test_tout(bool debug) in libphy,  0 - debug sar summ data out off, 1 - debug sar summ data out on
}

uint16 system_get_vdd33(void)
{
	return phy_get_vdd33() & 0x3FF; // uint32 phy_get_vdd33(void) in libphy
}

ETSTimer sta_con_timer;

extern void system_restart_local(void);

void system_restart(void)
{
	int mode = wifi_get_opmode();
	if(mode == STATIONAP_MODE || mode == STATION_MODE ) wifi_station_stop();
	if(mode == STATIONAP_MODE || mode == SOFTAP_MODE ) wifi_softap_stop();
	ets_timer_disarm(sta_con_timer);
	ets_timer_setfn(sta_con_timer, system_restart_local, 0);
	ets_timer_arm_new(sta_con_timer, 110, 0 ,1);
}

void system_restore(void)
{
	uint32 * a12 = pvPortMalloc(0x370);
	ets_memset(a12, 0xff, 0x370);
	ets_memcpy(a12, 0x3FFF17E8, 8);
	wifi_param_save_protect(a12);
	vPortFree(a12);
}

/*
void _wait_con_timer(uint8 x) // ????
{
	if (pm_is_open(x) == 0xff) return;
	if((*((uint8 *) (0x3FFE9E64 + 0xFC))) == 0) {
		ets_timer_setfn(0x3FFE9E64, loc_4024079C);
		*((uint8 *) (0x3FFE9E64 + 0xFC)) = 1;
	}
	pm_is_waked();
	uint8 x = *((uint8 *) (0x3FFE9E64 + 0xFC + 0x18));
	if(x == 0 || x == 1)
	{
		if(x==0) {
			pm_post(1);
			ets_timer_disarm(0x3FFE9E64);
			ets_timer_arm_new(0x3FFE9E64, 10, 0, 1);
			*((uint8 *) (0x3FFE9E64 + 0xFC + 0x18)) = 1;

			os_printf_plus("DEFERRED FUNC NUMBER IS BIGGER THAN 10\n");

		}
		x = *((uint8 *) (0x3FFE9E64 + 0xFC + 0x19));
		x ++;
		*((uint8 *) (0x3FFE9E64 + 0xFC + 0x19)) = x;
		if(x == 10) {
			.......
		}
	}
	.....
}
*/
void sta_con_timer_fn(void *timer_arg)
{
	if(_wait_con_timer(4) != 0xff) {

		WDT_REG1 = 0;
		WDT_CTRL |= 0x38;
		uint32 x = WDT_CTRL;
		x &= 0x79;
		x |= 4;
		WDT_CTRL = x;
		WDT_CTRL |= 0x01;

		while(1) ets_delay_us(20000);
	}
}


/* Никчемные функции
uint8 system_get_boot_version(void)
{
	return *((uint8 *)(0x3FFF1769+0x80)) & 0x1F;
}
bool _os_cmp_boot_ver_bl3(void)
{
	uint8 x = system_get_boot_version()
	if(x < 3 || x == 0x1F) {
		os_printf_plus("failed: need boot >= 1.3\n");
	}
}
uint32 system_get_test_result(void)
{
	_os_cmp_boot_ver_bl3();
	0x3FFF1769
	...
}
uint32 system_get_userbin_addr(void)
{

}
uint8 system_get_boot_mode(void)
{
	uint8 x = *((uint8 *) (0x3FFF1769 + 0x80));
	x &= 0x1F;
	if(x < 3 || x == 0x1F) return 1;
	return (*((uint8 *) (0x3FFF1769 + 0x80))) >> 7;
}
bool system_restart_enhance(uint8 bin_type, uint32 bin_addr)
{
	if(!_os_cmp_boot_ver_bl3()) return false;
	if(bin_type == 0) {
		uint32 userbin_addr = system_get_userbin_addr();
		os_printf_plus("restart to use user bin @ %x\n", userbin_addr);
		0x3FFF1769
		.....
		wifi_param_save_protect();
		system_restart();
		return 1;
	}
}
bool system_upgrade_userbin_set(uint32 flag)
{
}
uint8 system_upgrade_userbin_check(void)
{
}
void system_upgrade_flag_set(uint8 flag)
{
}
uint8 system_upgrade_flag_check(void)
{
}
void system_upgrade_reboot(void)
{
}
*/
uint32 pm_usec2rtc(uint32 us, uint32 cf)
{
	if(cf == 0) {
		cf = 5;
		return us/cf;
	}
	if(us <= 0xFFFFF) {
		us <<= 12;
		return us/cf;
	}
	else {
		return (us/cf) << 12;
	}
}

void pm_set_sleep_cycles(uint32 cycles)
{
	IO_RTC_SLP_VAL = IO_RTC_SLP_CNT_VAL + cycles;
	if(cycles > 5000) periodic_cal_sat = 1;
	else periodic_cal_sat = 0;
}

void pm_set_sleep_time(int timer_us_arg)
{
	pm_set_sleep_cycles(pm_usec2rtc(timer_us_arg, _coef_usec2rtc));
}

int _div_a2_a3(uint32 x, uint32 qmhz)
{
	return x/qmhz;
}

pm_rtc_clock_cali(uint32 _coef_usec2rtc) // struct xxx *xxx; // struct xxx { uint32 x; uint32 _coef_usec2rtc; ...}
{
	RTC_CALIB_VALUE  &= ~ RTC_CALIB_RDY; // 0x7FFFFFFF;
	RTC_CALIB_SYNC = 0x101; // RTC_PERIOD_NUM = 257
	RTC_CALIB_VALUE |= RTC_CALIB_RDY;
	ets_delay_us(100);
	while((RTC_CALIB_VALUE & RTC_CALIB_RDY) == 0);
	uint8 q = q_mhz; // chip6_phy_init_ctrl + 1 // = 40, 26
	uint32 qmhz = 40;
	if(q == 1) qmhz = 26;
	else if(q == 2) qmhz = 24;
	uint32 x = _div_a2_a3((RTC_CALIB_VALUE & 0xFFFFFFF) >> 4, qmhz);
	if(x == 0) return 0;
	if(_coef_usec2rtc == 0) {
		_coef_usec2rtc = x;
		return x;
	}
	uint32 t;
	if(_coef_usec2rtc >= x) t = _coef_usec2rtc - x;
	else t = x - _coef_usec2rtc;
	if(t > 511) return x;
	_coef_usec2rtc = (x * 5 + _coef_usec2rtc * 3) >> 3;
	return _coef_usec2rtc;
}

int pm_rtc_clock_cali_proc(void)
{
	rom_i2c_writeReg(106,2,8,0);
	pm_rtc_clock_cali(_coef_usec2rtc);
	return _coef_usec2rtc;
}

uint8 system_option[250];
#define deep_sleep_option system_option[241]

void _sys_deep_sleep_timer(void *timer_arg)
{
	os_printf_plus("deep sleep %ds\n\n", timer_arg/1000000);
	deep_sleep_set_option(deep_sleep_option);
//	while(READ_PERI_REG(UART_STATUS(0))  & (UART_TXFIFO_CNT<<UART_TXFIFO_CNT_S));
//	while(READ_PERI_REG(UART_STATUS(1))  & (UART_TXFIFO_CNT<<UART_TXFIFO_CNT_S));
	user_uart_wait_tx_fifo_empty(0, 500000);
	user_uart_wait_tx_fifo_empty(1, 500000);

	IO_RTC_0 = 0;
	IO_RTC_0 &= ~BIT14;
	IO_RTC_0 |= 0x30;
	RTC_BASE[17] = 4; //0x60000744 = 4
	IO_RTC_3 = 0x10010; // 	HWREG(PERIPHS_RTC_BASEADDR, 0x0C) = 0x10010;
	RTC_BASE[18] = (RTC_BASE[18] &  0xFFFF01FF) | 0xFC00; // HWREG(PERIPHS_RTC_BASEADDR, 0x48) = (HWREG(PERIPHS_RTC_BASEADDR,0x48) & 0xFFFF01FF) | 0xFC00;
	RTC_BASE[18] = (RTC_BASE[18] &  0xE00) | 0x80; // HWREG(PERIPHS_RTC_BASEADDR, 0x48) = (HWREG(PERIPHS_RTC_BASEADDR, 0x48) & 0xE00) | 0x80;
	IO_RTC_SLP_VAL = IO_RTC_SLP_CNT_VAL + 136; //	HWREG(PERIPHS_RTC_BASEADDR, 0x04) = HWREG(PERIPHS_RTC_BASEADDR, 0x1C) + 0x88;
	IO_RTC_6 = 8; // HWREG(PERIPHS_RTC_BASEADDR, 0x18) = 8;
	IO_RTC_2 = 0x100000; // HWREG(PERIPHS_RTC_BASEADDR, 0x08) = 0x100000;
	ets_delay_us(200);
	RTC_GPI2_CFG = 0x11; //	HWREG(PERIPHS_RTC_BASEADDR, 0x9C) = 0x11;
	IO_PAD_XPD_DCDC_CONF = 0x03; // HWREG(PERIPHS_RTC_BASEADDR, 0xA0) = 0x03;
	IO_RTC_3 = 0x640C8; // HWREG(PERIPHS_RTC_BASEADDR, 0x0C) = 0x640C8;
	IO_RTC_0 &= 0xFCF; // HWREG(PERIPHS_RTC_BASEADDR, 0x00) &= 0xFCF;
	uint32 clpr = pm_rtc_clock_cali_proc();
	pm_set_sleep_time(timer_arg);
	RTC_GPI2_CFG &= 0x11; //	HWREG(PERIPHS_RTC_BASEADDR, 0x9C) &= 0x11;
	IO_PAD_XPD_DCDC_CONF = 0x03; // HWREG(PERIPHS_RTC_BASEADDR, 0xA0) = 0x03;

	INTC_EDGE_EN &= 0x7E; // HWREG(PERIPHS_DPORT_BASEADDR, 4) &= 0x7E; // WDT int off
	ets_isr_mask(1<<8); // Disable WDT isr

	RTC_BASE[16] = 0x7F; // HWREG(PERIPHS_RTC_BASEADDR, 0x40) = 0x7F;
	RTC_BASE[16] = 0x20; // HWREG(PERIPHS_RTC_BASEADDR, 0x44) = 0x20;
	IO_RTC_4 = 0; // HWREG(PERIPHS_RTC_BASEADDR, 0x10) = 0x00;

	if(clpr == 0) {
		IO_RTC_6 = 0; //	HWREG(PERIPHS_RTC_BASEADDR, 0x18) = 0x00;
	}
	else {
		IO_RTC_6 = 8;	//	HWREG(PERIPHS_RTC_BASEADDR, 0x18) = 0x08;
	}
	ets_intr_lock();
	Cache_Read_Disable();
	SPI0_CMD = 0x200000;
	while(SPI0_CMD);
	struct rst_info rst_info;
	system_rtc_mem_read(0, rst_info, sizeof(rst_info));
	ets_memset(rst_info, 0, sizeof(rst_info));
	system_rtc_mem_write(0, &rst_info, sizeof(rst_info));
	IO_RTC_2 = 1<<20; // rtc_enter_sleep()	HWREG(PERIPHS_RTC_BASEADDR, 0x08) = 0x100000;
}

uint8 deep_sleep_flag;

void system_deep_sleep(uint32 time_in_us)
{
	if(wifi_get_opmode() != SOFTAP_MODE) wifi_station_stop();
	if(wifi_get_opmode() != STATION_MODE) wifi_softap_stop();
	deep_sleep_flag = 1;
	ets_timer_disarm(sta_con_timer);
	ets_timer_setfn(sta_con_timer, _sys_deep_sleep_timer, 0);
	ets_timer_arm_new(sta_con_timer,100, 0, 1);
}

uint8 deep_sleep_option;

bool system_deep_sleep_set_option(uint8 option)
{
	switch(option) {
	case 0:
	case 1:
	case 2:
	case 4:
		deep_sleep_option  = option; // 0x3FFE9E60 + 0xF1 = option;
		return true;
	}
	return false;
}
/* Никчемные функции
bool system_update_cpu_freq(uint8 freq)
{
	if(freq == 80) {
		HWREG(PERIPHS_DPORT_BASEADDR,0x14) &= 0x7E;
	 	system_update_cpu_freq(80);
	 	return true;
	}
	if(freq == 160) {
		HWREG(PERIPHS_DPORT_BASEADDR,0x14) |= 0x01;
	 	system_update_cpu_freq(160);
	 	return true;
	}
	return false;
}
uint8 system_get_cpu_freq(void)
{
	return ets_get_cpu_frequency();
}

#define cpu_overclock system_option[0xF2]

bool system_overclock(void)
{
	if(system_get_cpu_freq()!=80) {
		cpu_overclock = 1;
	 	system_update_cpu_freq(160);
	 	return true;
	}
	return false;
}
bool system_restoreclock(void)
{
	if(system_get_cpu_freq() == 160) {
		if(cpu_overclock) {
			system_update_cpu_freq(80);
			return true;
		}
	}
	return false;
}
*/
void system_timer_reinit(void)
{
	timer2_ms_flag = 0;
	TIMER1_CTRL = 0x84;
}


uint32 system_relative_time(void)
{
	return *((uint32*)0x3FF20C00);
}
void system_print_meminfo(void)
{
	os_printf_plus("data  : 0x%x ~ 0x%x, len: %d\n", 0x3FFE8000, _data_end, _data_end - 0x3FFE8000);
	os_printf_plus("rodata: 0x%x ~ 0x%x, len: %d\n", _rodata_start, _bss_start, _bss_start - _rodata_start);
	os_printf_plus("bss   : 0x%x ~ 0x%x, len: %d\n", _bss_start, _heap_start, _heap_start - _bss_start);
	os_printf_plus("heap  : 0x%x ~ 0x%x, len: %d\n", _heap_start, 0x3FFFC000 , 0x3FFFC000 - _heap_start);
}
uint32 xPortGetFreeHeapSize(void)
{
	return FreeHeapSize; // *((uint32*)0x3FFE9E50);  *((uint32*)0x3FFE9CB0);
}
uint32 system_get_free_heap_size(void)
{
	xPortGetFreeHeapSize();
}
uint32 system_get_chip_id(void)
{
	return (HWREG(PERIPHS_DPORT_BASEADDR,0x50) &  0xFF000000) | (HWREG(PERIPHS_DPORT_BASEADDR, 0x54) & 0xFFFFFF);
}
uint32 system_rtc_clock_cali_proc(void)
{
	return pm_rtc_clock_cali_proc();
}
uint32 system_get_rtc_time(void)
{
	return IO_RTC_SLP_CNT_VAL; // 0x6000071C
}
/*
uint64 system_mktime(uint32 year, uint32 mon, uint32 day, uint32 hour, uint32 min, uint32 sec)
{
.........
}
*/
void system_init_done_cb(init_done_cb_t cb)
{
    done_cb = cb;	//*((init_done_cb_t *)(&system_option[0xF4])) = cb;
}

bool system_os_task(os_task_t task, uint8 prio, os_event_t *queue, uint8 qlen)
{
	if(prio < 3) {
		os_printf("err: task prio < %d\n", 3);
		return false;
	}
	if(qlen == 0 || queue == NULL) {
		os_printf("err: task queue error\n");
		return false;
	}
	ets_task(task, prio +22, queue, qlen);
	return true;
}

void system_uart_swap(void)
{
	user_uart_wait_tx_fifo_empty(0, 500000);
	user_uart_wait_tx_fifo_empty(1, 500000);

	HWREG(IOMUX_BASE,0x08) &= 0xECF; // PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTCK_U, FUNC_UART0_CTS); ?
	HWREG(IOMUX_BASE,0x08) |= 0x100;
	HWREG(IOMUX_BASE,0x10) &= 0xECF; // PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTDO_U, FUNC_UART0_RTS); ?
	HWREG(IOMUX_BASE,0x10) |= 0x100;
	HWREG(PERIPHS_DPORT_BASEADDR,0x28) |= BIT2;
}
const char *system_get_sdk_version(void)
{
	return "1.1.2";
}

/* WiFi функции
system_get_checksum
system_station_got_ip_set
wifi_softap_dhcps_start
wifi_softap_dhcps_stop
wifi_softap_dhcps_status
wifi_station_dhcpc_start
wifi_station_dhcpc_stop
wifi_station_dhcpc_status
wifi_get_opmode
wifi_get_opmode_default
wifi_get_broadcast_if
wifi_set_broadcast_if
wifi_set_opmode
wifi_set_opmode_current
wifi_param_save_protect
wifi_station_get_config
wifi_station_get_config_default
wifi_station_get_ap_info
wifi_station_ap_number_set
wifi_station_set_config
wifi_station_set_config_current
wifi_station_get_current_ap_id
wifi_station_ap_check
wifi_station_ap_change
wifi_station_scan
wifi_station_get_auto_connect
wifi_station_set_auto_connect
wifi_station_connect
wifi_station_disconnect
wifi_station_get_connect_status
wifi_softap_cacl_mac
wifi_softap_set_default_ssid
wifi_softap_get_config
wifi_softap_get_config_default
wifi_softap_set_config
wifi_softap_set_config_current
wifi_softap_set_station_info
wifi_softap_get_station_info
wifi_softap_free_station_info
wifi_softap_deauth
wifi_get_phy_mode
wifi_set_phy_mode
wifi_set_sleep_type
wifi_get_sleep_type
wifi_get_channel
wifi_set_channel
wifi_promiscuous_set_mac
wifi_promiscuous_enable
wifi_set_promiscuous_rx_cb
wifi_get_ip_info
wifi_set_ip_info
wifi_get_macaddr
wifi_set_macaddr
wifi_status_led_install
wifi_status_led_uninstall
wifi_set_status_led_output_level
*/
