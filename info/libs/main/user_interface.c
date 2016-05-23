/******************************************************************************
 * FileName: user_interface.c
 * Description: disasm user_interface functions SDK 1.1.0..1.4.0 (libmain.a + libpp.a)
 * Author: PV`
 * (c) PV` 2015
 ******************************************************************************/
#include "user_config.h"
#include "bios.h"
#include "hw/esp8266.h"
#include "hw/specreg.h"
#include "hw/gpio_register.h"
//#include "user_interface.h"
#include "sdk/add_func.h"
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
		os_free(hostname);
		hostname = NULL;
	}
	hostname = os_malloc(32);
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
		if(hostname != NULL) {
			os_free(hostname);
			hostname = NULL;
		}
		hostname = os_malloc(len);
		if(hostname == NULL) return false;
		struct netif * nif = eagle_lwip_getif(0);
		ets_strcpy(hostname, name);
		if(nif != NULL) {
			nif->hostname = hostname;
		}
		return true;
	}
	return false;
}


uint32 system_phy_temperature_alert(void)
{
	return phy_get_check_flag(0); // in libphy.a
}
// получить текущее значение tpw мождно использовав phy_in_most_power
uint32 system_phy_set_max_tpw(uint32 tpw)
{
	return phy_set_most_tpw(tpw);
}
// uint16 vdd33 : VDD33, unit : 1/1024V, range [1900, 3300]
void system_phy_set_tpw_via_vdd33(uint16 vdd33)
{
	return phy_vdd33_set_tpw(vdd33);
}

uint32 system_get_time(void)
{
	return WdevTimOffSet + *((uint32*)0x3FF20C00);
}

uint32 system_relative_time(void)
{
	return *((uint32*)0x3FF20C00);
}

bool system_os_post(uint8 prio, os_signal_t sig, os_param_t par)
{
	if(prio >= USER_TASK_PRIO_MAX) {
		os_printf_plus("err: post prio < %d\n", USER_TASK_PRIO_MAX);
		return false;
	}
//	return ets_post((prio + 22) & 0xFF, sig, par); // in ets.h, + 22 SDK 1.4.1
	return ets_post((prio + 2) & 0xFF, sig, par); // in ets.h, + 2 SDK 1.5.2
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
	ets_task(task, prio + 22, queue, qlen); // + 22 SDK 1.4.1,  + 2 SDK 1.5.2
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


void ICACHE_FLASH_ATTR system_restart(void)
{
	int mode = wifi_get_opmode();
	if(mode == STATIONAP_MODE || mode == STATION_MODE ) wifi_station_stop();
	if(mode == STATIONAP_MODE || mode == SOFTAP_MODE ) wifi_softap_stop();
	ets_timer_disarm(sta_con_timer);
	ets_timer_setfn(sta_con_timer, system_restart_local, 0);
	ets_timer_arm_new(sta_con_timer, 100, 0 ,1);
}

void system_restore(void)
{
	uint8 * buf_a12 = os_malloc(sizeof(struct s_wifi_store));
	ets_memset(buf_a12, 0xff, sizeof(struct s_wifi_store));
	ets_memcpy(buf_a12, &g_ic.g.wifi_store, 8);
	uint32 sctrs = flashchip->chip_size/flashchip->sector_size;
	sctrs -= 4;
	wifi_param_save_protect(sctrs, buf_a12, sizeof(struct s_wifi_store));
	os_free(buf_a12);
}

void system_restart_core(void)
{
	ets_intr_lock();
	Wait_SPI_Idle(flashchip);
	Cache_Read_Disable();
	DPORT_OFF24 &= 0x67; // 0x3FF00024 &= 0x67;
	Call _ResetVector(); // ROM:0x40000080
}

int sub_40240CD8(int x)
{
	if(fpm_allow_tx() == 0) wifi_fpm_do_wakeup();
	if(pm_is_open() == 0) return 0;
	if(byte_3FFE97A0 == 0) { // status_led_output_level???
		ets_timer_setfn(&dword_3FFE97A4, loc_40240D9C, 0);
		status_led_output_level = 1;
	}
	pm_is_waked();
	if(pm_is_waked() == 0 && byte_3FFE97B8 == 1) {
		if(byte_3FFE97B8 != 0) {
			pm_post(1);
			ets_timer_disarm(&dword_3FFE97A4);
			ets_timer_arm_new(&dword_3FFE97A4, 10, 0, 1)
			byte_3FFE97B8 = 1;
		}
		if(++byte_3FFE97B9 > 10 ) {
			os_printf("DEFERRED FUNC NUMBER IS BIGGER THAN 10\n");
		}
		if(byte_3FFE97BA + byte_3FFE97B9 <= 10) {
			unk_3FFE984C[0x79 + byte_3FFE97BA + byte_3FFE97B9] = x; // tab unk_3FFE98D0 ?
		}
		else unk_3FFE984C[0x83] = x;
		return -1;

	}
	else return 0;
}


void ICACHE_FLASH_ATTR system_restart_local(void)
{
	struct rst_info rst_inf;
	if(sub_40240CD8(4) == -1) {
		clockgate_watchdog(0);
		DPORT_OFF18 = 0xFFFF00FF;
		pm_open_rf();
	}
	system_rtc_mem_read(0, &rst_inf, sizeof(rst_inf));
	if(rst_inf.reason != REASON_EXCEPTION_RST && rst_inf.reason != REASON_SOFT_WDT_RST) {
		ets_memset(&rst_inf, 0, sizeof(rst_inf));
		IO_RTC_SCRATCH0 = REASON_SOFT_RESTART;
		rst_inf.reason = REASON_SOFT_RESTART;
		system_rtc_mem_write(0, &rst_inf, sizeof(rst_inf));
	}
	system_restart_hook(&rst_inf); // ret.n
	user_uart_wait_tx_fifo_empty(0, 500000);
	user_uart_wait_tx_fifo_empty(1, 500000);
	ets_intr_lock();
	SAR_BASE[0x48>>2] |= 3;
	DPORT_OFF18 |= 0x100;
	SAR_BASE[0x48>>2] &= 0x7C;
	DPORT_OFF18 &= 0xEFF;
	system_restart_core();
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


uint8 system_get_boot_version(void)
{
	return g_ic.g.boot_info[1]&0x1F; // SDK 1.5.2
}
uint8 system_get_boot_mode(void) {
	uint8 bm = g_ic.g.boot_info[1];
	if((bm&0x1F) < 3 || (bm&0x1F)== 0x1F) return 1;
	else return bm>>7;
}

enum flash_size_map system_get_flash_size_map(void)
{
	struct SPIFlashHead fh;
	spi_flash_read(0, &fh, sizeof(fh));
	return fh.hsz.flash_size;
}
uint32 system_get_userbin_addr(void)
{
	uint32 sector;
	if(g_ic.g.boot_info[1]&0x80) {
		enum flash_size_map sm = system_get_flash_size_map();
		if((sector = system_upgrade_userbin_check()) != 0) {
			if(sm == FLASH_SIZE_4M_MAP_256_256) {
				sector = 0x41;
			}
			else if(sm >= FLASH_SIZE_16M_MAP_1024_1024) {
				if(sm <= FLASH_SIZE_32M_MAP_1024_1024) {
					sector = 0x101;
				}
			}
			else if(sm >= FLASH_SIZE_8M_MAP_512_512) {
				sector = 0x81;
			}
			return sector<<12;
		}
		else {
			if((g_ic.g.boot_info[1]&0x1F)== 0x1F) return 0;
			else return 0x1000;
		}
	}
	else {
		if((g_ic.g.boot_info[0]&0x04)==0) {
			return (g_ic.g.boot_info[4]<<16)|(g_ic.g.boot_info[3]<<8)|g_ic.g.boot_info[2];
		}
		else {
			return (g_ic.g.boot_info[7]<<16)|(g_ic.g.boot_info[6]<<8)|g_ic.g.boot_info[5];
		}
	}
}
bool _Tst_NeedBoot13(void)
{
	uint8 bm = system_get_boot_version();
	if((bm&0x1F) < 3|| (bm&0x1F)== 0x1F) {
		os_printf("failed: need boot >= 1.3\n");
		return false;
	}
	else return true;
}
uint8 system_get_test_result(void)
{
	if(_Tst_NeedBoot13()) return 127;
	return (g_ic.g.boot_info[1]>>5)&1;
}
bool system_restart_enhance(uint8 bin_type,uint32 bin_addr)
{
	if(_Tst_NeedBoot13()) return false;
	if(bin_type == SYS_BOOT_ENHANCE_MODE) {
		struct SPIFlashHead fh;
		 	 spi_flash_read(0, &fh, sizeof(fh));
		 	 switch(fh.hsz.flash_size) {
		 	 	case 0: // 512kbytes
		 	 	case 2: // 1Mbytes
		 	 	case 3: // 2Mbytes
		 	 	case 4: // 4Mbytes
				break;
			default:
				os_printf("don't supported flash map.\n");
				return false;
			}
			uint32 getbin_addr = system_get_userbin_addr();

			os_printf("restart to use user bin @ %x\n", bin_addr);
			g_ic.g.boot_info[7] = bin_addr >> 16;
			g_ic.g.boot_info[6] = bin_addr >> 8;
			g_ic.g.boot_info[5] = bin_addr;
			g_ic.g.boot_info[4] = getbin_addr >> 16;
			g_ic.g.boot_info[3] = getbin_addr >> 8;
			g_ic.g.boot_info[2] = getbin_addr;
			g_ic.g.boot_info[1] = g_ic.g.boot_info[1]&0x7F;
			g_ic.g.boot_info[0] = (g_ic.g.boot_info[0]&0xFB) | 0x04;
			system_param_save_with_protect((flashchip->chip_size/flashchip->sector_size)-3, &g_ic.g.wifi_store, sizeof(struct s_wifi_store));
			system_restart();
			return true;
	}
	else if(bin_type == SYS_BOOT_NORMAL_MODE) {
		if(system_get_test_result()) {
			os_printf("reboot to use test bin @ %x\n", bin_addr);
			g_ic.g.boot_info[4] = bin_addr >> 16;
			g_ic.g.boot_info[3] = bin_addr >> 8;
			g_ic.g.boot_info[2] = bin_addr;
			g_ic.g.boot_info[1] = g_ic.g.boot_info[1]&0xBF;
			system_param_save_with_protect((flashchip->chip_size/flashchip->sector_size)-3, &g_ic.g.wifi_store, sizeof(struct s_wifi_store));
			system_restart();
			return true;
		 }
		 else {
			 os_printf("test already passed.\n");
			 return false;
		 }
	}
	else {
		os_printf("don't supported type.\n");
		return false;
	}
}
bool system_upgrade_userbin_set(uint32 flag)
{
	uint8 bm = system_get_boot_version();
	if(flag > 2) return false;
	if(bm == 2 || bm == 0x1F) {
		g_ic.g.boot_info[0] = (g_ic.g.boot_info[0] & 0xF0) | (flag&0x0F);
		return true;
	}
	else {
		g_ic.g.boot_info[0] = (g_ic.g.boot_info[0] & 0xFC) | (flag&0x03);
		return true;
	}
	return true;
}
bool system_upgrade_userbin_check(void)
{
	uint8 bv = system_get_boot_version();
	if(bv == 2 || bv == 0x1F) {
		if((g_ic.g.boot_info[0]&0x0F) != 1) return false;
		return true;
	}
	else {
		if((g_ic.g.boot_info[0]&0x03) != 1) {
			if(g_ic.g.boot_info[0]&(1<<2)) return false;
			return true;
		}
		else {
			if(g_ic.g.boot_info[0]&(1<<2)) return true;
			return false;
		}
	}
}
uint8 _upgrade_flag;
bool system_upgrade_flag_set(uint8 flag)
{
	if(flag > 3) return false;
	_upgrade_flag = flag;
}
uint8 system_upgrade_flag_check(void)
{
	return _upgrade_flag;
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

void system_deep_sleep_local_2(void)
{
	ets_intr_lock();
	Cache_Read_Disable();
	SPI0_CMD = 0x200000;
	while(SPI0_CMD);
	struct rst_info rst_info;
	ets_memset(rst_info, 0, sizeof(rst_info));
	rst_info.reason = REASON_DEEP_SLEEP_AWAKE;
	system_rtc_mem_write(0, &rst_info, sizeof(rst_info));
	IO_RTC_2 = 1<<20; // rtc_enter_sleep()	HWREG(PERIPHS_RTC_BASEADDR, 0x08) = 0x100000;
}


void system_deep_sleep_instant(void *timer_arg) // В ранних SDK _sys_deep_sleep_timer()
{
	os_printf_plus("deep sleep %ds\n\n", timer_arg/1000000);
	deep_sleep_set_option(deep_sleep_option);
	dw0x3FF20DE0 = 0x3333;
	ets_delay_us(20);
	user_uart_wait_tx_fifo_empty(0, 500000);
	user_uart_wait_tx_fifo_empty(1, 500000);
	IO_RTC_0 = 0;	// 0x60000700 = 0
	IO_RTC_0 &= ~BIT14;
	IO_RTC_0 |= 0x30;
	RTC_BASE[17] = 4; //0x60000744 = 4
	IO_RTC_3 = 0x10010; // 	HWREG(PERIPHS_RTC_BASEADDR, 0x0C) = 0x10010;
	RTC_BASE[18] = (RTC_BASE[18] &  0xFFFF01FF) | 0xFC00; // HWREG(PERIPHS_RTC_BASEADDR, 0x48) = (HWREG(PERIPHS_RTC_BASEADDR,0x48) & 0xFFFF01FF) | 0xFC00;
	RTC_BASE[18] = (RTC_BASE[18] &  0xE00) | 0x80; // HWREG(PERIPHS_RTC_BASEADDR, 0x48) = (HWREG(PERIPHS_RTC_BASEADDR, 0x48) & 0xE00) | 0x80;
	IO_RTC_4 = 0; //0x60000710 = 0
	IO_RTC_4 = 0; //0x6000071C = 0
	IO_RTC_SLP_VAL = IO_RTC_SLP_CNT_VAL + 136 + 256; //	HWREG(PERIPHS_RTC_BASEADDR, 0x04) = HWREG(PERIPHS_RTC_BASEADDR, 0x1C) + 0x88;
	IO_RTC_6 = 8; // HWREG(PERIPHS_RTC_BASEADDR, 0x18) = 8;
	IO_RTC_2 = 0x100000; // HWREG(PERIPHS_RTC_BASEADDR, 0x08) = 0x100000;
	ets_delay_us(200);
	RTC_GPI2_CFG = 0x11; //	HWREG(PERIPHS_RTC_BASEADDR, 0x9C) = 0x11;
	IO_PAD_XPD_DCDC_CONF = 0x03; // HWREG(PERIPHS_RTC_BASEADDR, 0xA0) = 0x03;
	IO_RTC_3 = 0x640C8; // HWREG(PERIPHS_RTC_BASEADDR, 0x0C) = 0x640C8;
	IO_RTC_0 &= 0xFCF; // HWREG(PERIPHS_RTC_BASEADDR, 0x00) &= 0xFCF;
	RTC_GPI2_CFG = 0x11; // HWREG(PERIPHS_RTC_BASEADDR, 0x9C) = 0x11;
	IO_PAD_XPD_DCDC_CONF = 0x03; // HWREG(PERIPHS_RTC_BASEADDR, 0xA0) = 0x03;

	INTC_EDGE_EN &= 0x7E; // HWREG(PERIPHS_DPORT_BASEADDR, 4) &= 0x7E; // WDT int off
	ets_isr_mask(1<<8); // Disable WDT isr

	DPORT_BASE[0] = (DPORT_BASE[0]&0x60)|0x0e; // nmi int
	while(DPORT_BASE[0]&1);

	RTC_BASE[16] = 0xFFF; // HWREG(PERIPHS_RTC_BASEADDR, 0x40) = 0xFFF;
	RTC_BASE[17] = 0x20; // HWREG(PERIPHS_RTC_BASEADDR, 0x44) = 0x20;

	uint32 clpr = pm_rtc_clock_cali_proc();
	pm_set_sleep_time(timer_arg);

//	IO_RTC_4 = 0; // HWREG(PERIPHS_RTC_BASEADDR, 0x10) = 0x00;

	if(clpr == 0) {
		IO_RTC_6 = 0; //	HWREG(PERIPHS_RTC_BASEADDR, 0x18) = 0x00;
	}
	else {
		IO_RTC_6 = 8;	//	HWREG(PERIPHS_RTC_BASEADDR, 0x18) = 0x08;
	}
	system_deep_sleep_local_2();
}

uint8 deep_sleep_flag;

void system_deep_sleep(uint32 time_in_us)
{
	if(wifi_get_opmode() != SOFTAP_MODE) wifi_station_stop();
	if(wifi_get_opmode() != STATION_MODE) wifi_softap_stop();
	deep_sleep_flag = 1;
	ets_timer_disarm(sta_con_timer);
	ets_timer_setfn(sta_con_timer, system_deep_sleep_instant, time_in_us);
	ets_timer_arm_new(sta_con_timer,100, 0, 1);
}

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

void system_uart_swap(void)
{
	user_uart_wait_tx_fifo_empty(0, 500000);
	user_uart_wait_tx_fifo_empty(1, 500000);

	GPIO13_MUX = (GPIO13_MUX & 0xECF) | 0x100;
	GPIO15_MUX = (GPIO15_MUX & 0xECF) | 0x100;
	PERI_IO_SWAP |= BIT2;
}

void system_uart_de_swap(void)
{
	user_uart_wait_tx_fifo_empty(0, 500000);
	user_uart_wait_tx_fifo_empty(1, 500000);
	PERI_IO_SWAP &= 0x7B;
}

const char *system_get_sdk_version(void)
{
	return "1.3.0";
}

uint32 ICACHE_FLASH_ATTR system_get_checksum(uint8 *ptr, uint32 len)
{
	uint8 checksum = 0xEF;
	while(len--) checksum ^= *ptr++;
	return checksum;
}

bool wifi_softap_dhcps_start(void)
{
	int opmode = wifi_get_opmode();
	if(opmode == STATION_MODE || opmode == NULL_MODE || g_ic.c[0x180+0x3E] == 0) return false;
	struct netif * nif;
	nif = eagle_lwip_getif(1);
	if(nif == NULL && (!dhcps_flag)) {
		struct ip_info *ipinfo;
		wifi_get_ip_info(SOFTAP_IF, &ipinfo);
		dhcps_start(ipinfo);
	}
	dhcps_flag = true;
	return true;
}

bool wifi_softap_dhcps_stop(void)
{
	int opmode = wifi_get_opmode();
	if(opmode == STATION_MODE || opmode == NULL_MODE || g_ic.c[0x180+0x3E] != 0) return false;
	struct netif * nif;
	nif = eagle_lwip_getif(1);
	if(nif != NULL && dhcps_flag) dhcps_stop();
	dhcps_flag = false;
	return true;
}

enum dhcp_status wifi_softap_dhcps_status(void)
{
	return dhcps_flag;
}

bool wifi_station_dhcpc_start(void)
{
	int opmode = wifi_get_opmode();
	if(opmode == STATION_MODE || opmode == NULL_MODE || g_ic.c[0x180+0x3E] != 0) return false;
	struct netif * nif;
	nif = eagle_lwip_getif(0);
	if(nif == NULL && (!dhcpc_flag)) {
		if(nif->flags & 1) { // nif->[+0x35]
			nif->ip_addr.addr = 0;
			nif->gw.addr = 0;
			nif->netmask.addr = 0;
			if( dhcp_start() != ERR_OK) return false;
		}
	}
	dhcpc_flag = true;
	return true;
}

bool wifi_station_dhcpc_stop(void){
	int opmode = wifi_get_opmode();
	if(opmode == STATION_MODE || opmode == NULL_MODE || g_ic.c[0x180+0x3E] != 0) return false; // g_ic.c[0x1BE] g_ic +446
	struct netif * nif;
	nif = eagle_lwip_getif(0);
	if(nif == NULL && (!dhcpc_flag)) {
		dhcp_stop();
	}
	dhcpc_flag = false;
	return true;
}
enum dhcp_status wifi_station_dhcpc_status(void)
{
	return dhcpc_flag;
}


bool system_param_save_with_protect(uint16 start_sec, void *param, uint16 len)
{
	struct ets_store_wifi_hdr whd;
	if(param == NULL) return false;
	if(flashchip->sector_size < len) return false;
	spi_flash_read((start_sec + 2)*flashchip->sector_size, &whd, sizeof(whd));
	if(whd.bank == 0) whd.bank = 1;
	wifi_param_save_protect_with_check(start_sec + whd.bank, flashchip->sector_size, param, len);
	whd.flag = 0x55AA55AA;
	if(++whd.wr_cnt == 0) whd.wr_cnt = 1;
	whd.xx[whd.bank] = 28;
	whd.chk[whd.bank] = system_get_checksum(param, 28);
	wifi_param_save_protect_with_check(start_sec + 2, flashchip->sector_size, &whd, 28);
	return true;
}

void wifi_param_save_protect_with_check(uint16 startsector, int sectorsize, void *pdata, uint16 len)
{
	uint8 * pbuf = os_malloc(len);
	int i;
	if(pbuf == NULL) return;
	do {
		spi_flash_erase_sector(startsector);
		spi_flash_write(startsector*sectorsize, pdata, len);
		spi_flash_read(startsector*sectorsize, pbuf, len);
		i = ets_memcmp(pdata, pbuf, len);
		if(i) {
			os_printf("[W]sec %x error\n", startsector);
		}
	} while(i != 0);
	os_free(pbuf);
}

bool system_param_load(uint16 start_sec, uint16 offset, void *param, uint16 len)
{
	struct ets_store_wifi_hdr whd;
	if(param == NULL || (len + offset) > flashchip->sector_size) return false;
	spi_flash_read((start_sec + 2)*flashchip->sector_size, &whd, sizeof(whd));
	uint32 data_sec = start_sec;
	if(whd.bank) data_sec++;
	spi_flash_read(data_sec * flashchip->sector_size + offset, param, len);
	return true;
}

uint8 wifi_get_channel(void)
{
	uint8 *ptr = chm_get_current_channel();
	return ptr[6];
}

bool wifi_set_channel(uint8 channel)
{
	if(channel > 13) return false;
	ets_intr_lock();
	uint32 * p = &g_ic;
	p[300] = channel*6 + 0x78; // ???
	ets_intr_unlock();
	chm_set_current_channel(channel);
	return true;
}

enum sleep_type wifi_get_sleep_type(void)
{
	return pm_get_sleep_type();
}

bool ICACHE_FLASH_ATTR wifi_set_sleep_type(enum sleep_type type)
{
	if(type > MODEM_SLEEP_T) return false;
	pm_set_sleep_type_from_upper(type);
	return true;
}

bool ICACHE_FLASH_ATTR wifi_promiscuous_set_mac(const uint8_t *address)
{
	if(g_ic.c[0x180+0x3e] != 1) return false;
	volatile uint32 * preg = (volatile uint32 *)0x3FF20000;
	preg[27] |= 1;
	preg[27] |= 2;
	preg[27] |= 4;
	wDev_SetMacAddress(0, address);
	return true;
}

wifi_promiscuous_cb_t promiscuous_cb;
void ICACHE_FLASH_ATTR wifi_set_promiscuous_rx_cb(wifi_promiscuous_cb_t cb)
{
	promiscuous_cb = cb;
}

uint8 info_st_mac[6];

int ICACHE_FLASH_ATTR wifi_promiscuous_enable(uint8 promiscuous)
{
	int opmode	= wifi_get_opmode();
	if(user_init_flag & opmode == 1) {
		if(g_ic.c[446] != 3) {
			if(promiscuous == 0) {
				if(g_ic.c[446] != 0) {
					wDevDisableRx();
					wdev_exit_sniffer();
					volatile uint32 * preg = (volatile uint32 *)0x3FF20000;
					preg[27] |= 1;
					preg[27] |= 2;
					preg[27] |= 4;
					wDev_SetMacAddress(0, info_st_mac);
					g_ic.c[446] = 0;
					wDevEnableRx();
					return true;
				}
				return true; //??
			}
			else {
				if(g_ic.c[446] == 1) return true;;
				wifi_station_disconnect();
				wDevDisableRx();
				volatile uint32 * preg = (volatile uint32 *)0x3FF20000;
				preg[27] &= 0xFFE;
				preg[27] &= 0xFFD;
				preg[27] &= 0x7B;
				g_ic.c[446]= 1;
				wdev_go_sniffer();
				wDevEnableRx();
				return true;
			}
		}
	}
	return 0;
}

typedef struct {
    uint32 event;
    Event_StaMode_Got_IP_t info;
} System_Event_goy_ip;

wifi_event_handler_cb_t event_cb;

void ICACHE_FLASH_ATTR system_station_got_ip_set(ip_addr_t ip, ip_addr_t netmask, ip_addr_t gw)
{
	if(event_cb != NULL || g_ic.g.netif1 != NULL) {
		if(g_ic.g.netif1->ip_addr != ip
		|| g_ic.g.netif1->netmask != netmask
		|| g_ic.g.netif1->gw != gw) {
			System_Event_goy_ip ev_par;
			ev_par.info.ip = g_ic.g.netif1->ip_addr;
			ev_par.info.mask = g_ic.g.netif1->netmask;
			ev_par.info.gw = g_ic.g.netif1->gw;
			ev_par.event = EVENT_STAMODE_GOT_IP;
			event_cb((System_Event_t *)&ev_par);
		}
		// os_printf("ip:%d.%d.%d.%d,mask:%d.%d.%d.%d,gw:%d.% ...", g_ic.g.netif1->ip_addr, ... );
		g_ic.g.netif1[186] = 5; //??
		g_ic.g.netif1[184] = 5; //??
		if(g_ic.g.wifi_store.wfmode[1] == 1 && g_ic.g.wifi_store.wfmode[0] == 1) {
			ets_timer_disarm(sta_con_timer);
			g_ic.c[90];
			if(status_led_output_level & 1) {
				// sll 1 ??
			}
			gpio_output_set(, (status_led_output_level & 1)? 0: 1, c,0);
		}
	}
}

extern struct rst_info rst_if;
struct rst_info* ICACHE_FLASH_ATTR system_get_rst_info(void)
{
	return &rst_if;
}

uint8 ICACHE_FLASH_ATTR system_get_data_of_array_8(void *ps)
{
	return (*((unsigned int *)((unsigned int)ps & (~3))))>>(((unsigned int)ps & 3) << 3);
}

uint16 ICACHE_FLASH_ATTR system_get_data_of_array_16(void *ps)
{
	// В данной функции ошибка у китаё-программеров от Espressif!
	// Она не используется, как и многие другие, но занимет место в Flash.
}

bool ICACHE_FLASH_ATTR wifi_set_phy_mode(enum phy_mode mode)
{

}

void ICACHE_FLASH_ATTR wifi_status_led_install(uint8 gpio_id, uint32 gpio_name, uint8 gpio_func)
{
	g_ic.g.wifi_store.wfmode[3] = gpio_id;
	g_ic.g.wifi_store.wfmode[2] = g_ic.g.wifi_store.wfmode[1] = 1;
	volatile uint32 * ptr = (volatile uint32 *)	gpio_name;
	*ptr = (*ptr & 0xECF) | ((((gpio_func & 4) << 2) | (gpio_func & 3)) << 4);
}

void ICACHE_FLASH_ATTR wifi_status_led_uninstall(void)
{
	if(g_ic.g.wifi_store.wfmode[1] == 1) {
		g_ic.g.wifi_store.wfmode[1] = 0;
		ets_timer_disarm(sta_con_timer);
	}
}

uint8 status_led_output_level;
void ICACHE_FLASH_ATTR wifi_set_status_led_output_level(int x)
{
	if(x == 1)	status_led_output_level = 1;
	else status_led_output_level = 0;
}

void ICACHE_FLASH_ATTR wifi_set_event_handler_cb(wifi_event_handler_cb_t cb)
{
	event_cb = cb;
}


/* WiFi функции
uint8 wifi_get_broadcast_if(void);
bool wifi_set_broadcast_if(uint8 interface);
bool wifi_set_opmode(uint8 opmode);
bool wifi_set_opmode_current(uint8 opmode);
uint8 wifi_station_get_ap_info(struct station_config config[]);
bool wifi_station_ap_number_set(uint8 ap_number);
bool wifi_station_get_config(struct station_config *config);
bool wifi_station_get_config_default(struct station_config *config);
bool wifi_station_set_config(struct station_config *config);
bool wifi_station_set_config_current(struct station_config *config);
uint8 wifi_station_get_current_ap_id(void);
wifi_station_ap_check
bool wifi_station_ap_change(uint8 current_ap_id);
bool wifi_station_scan(struct scan_config *config, scan_done_cb_t cb);
uint8 wifi_station_get_auto_connect(void);
bool wifi_station_set_auto_connect(uint8 set);
bool wifi_station_connect(void);
bool wifi_station_disconnect(void)
uint8 wifi_station_get_connect_status(void);
void wifi_softap_cacl_mac(uint8 *mac_out, uint8 *mac_in)
void wifi_softap_set_default_ssid(void)
bool wifi_softap_get_config(struct softap_config *config);
bool wifi_softap_get_config_default(struct softap_config *config);
bool wifi_softap_set_config(struct softap_config *config);
bool wifi_softap_set_config_current(struct softap_config *config);
uint8 wifi_softap_get_station_num(void);
struct station_info * wifi_softap_get_station_info(void);
void wifi_softap_free_station_info(void);
int wifi_softap_set_station_info(uint8_t * chaddr, struct ip_addr *ip);
wifi_softap_deauth

bool wifi_get_ip_info(uint8 if_index, struct ip_info *info);
bool wifi_set_ip_info(uint8 if_index, struct ip_info *info);
bool wifi_get_macaddr(uint8 if_index, uint8 *macaddr);
bool wifi_set_macaddr(uint8 if_index, uint8 *macaddr);
*/

void ICACHE_FLASH_ATTR wifi_enable_6m_rate(uint8 x)
{
	g_ic.c[461] = x;
}

uint8 ICACHE_FLASH_ATTR wifi_get_user_fixed_rate(uint8 * a, uint8 * b)
{
	if(a == NULL || b == NULL) return 0x7F;
	*b = g_ic.c[463];
	*a = g_ic.c[462];
	return 0;
}

uint8 ICACHE_FLASH_ATTR wifi_set_user_fixed_rate(uint8 a, uint8 b)
{
	if(b >= 32) return 0x7F;
	if(a > 4) return 0x7E;
	g_ic.c[463] = b;
	g_ic.c[462] = a;
	return 0;
}

uint8 ICACHE_FLASH_ATTR wifi_send_pkt_freedom(void *a, uint8 b)
{
	if(a == NULL || b > 23) return 0x7F;
	int opmode = wifi_get_opmode();
	if(opmode == 1) {
		if(g_ic.g.netif1 == NULL) return 0x76;
		return ieee80211_freedom_output(g_ic.g.netif1, b, a);
	}
	else if(opmode > 4 || opmode < 2) return 0x76;
	else {
		if(g_ic.g.netif2 == NULL) return 0x76;
		return ieee80211_freedom_output(g_ic.g.netif2, b, a);
	}
}

extern struct netif *netif_default;
extern uint8 default_interface;
uint8 ICACHE_FLASH_ATTR wifi_get_broadcast_if(void)
{
	int opmode = wifi_get_opmode();
	if(opmode == 3) return opmode;
	if(default_interface != 0) return default_interface;
	struct netif * nif = eagle_lwip_getif(0);
	if(netif_default->next == nif) return 1;
	else return 2;
}

uint8 ICACHE_FLASH_ATTR system_get_boot_version(void)
{
	return (g_ic.c[464]>>8) & 31; // boot_version
}

static uint8 ICACHE_FLASH_ATTR _wifi_get_opmode(bool flg)
{
	uint8 opmode;
	struct s_wifi_store * wifi_store;
	if(flg != true) {
		wifi_store = (struct s_wifi_store *)os_malloc(sizeof(struct s_wifi_store)); // 888 байт в SDK 1.3.0
		system_param_load((flashchip->chip_size/flashchip->sector_size)-3, 0, wifi_store,  sizeof(struct s_wifi_store));
	}
	else wifi_store = &g_ic.g.wifi_store;
	opmode = wifi_store->wfmode[0];
	if(opmode > STATIONAP_MODE) opmode = 2;
	if(flg != true) os_free(wifi_store);
	return opmode;
}

uint8 ICACHE_FLASH_ATTR wifi_get_opmode(void)
{
	return _wifi_get_opmode(true);
}

uint8 ICACHE_FLASH_ATTR wifi_get_opmode_default(void)
{
	return _wifi_get_opmode(false);
}

struct s_wekap { // 0x3FFEA410 ???
	uint8 flg_timer; //+00 byte_
	uint8 x[3]; //+01
	ETSTimer timer;	 //+04
	uint8 c[16];
	uint8 flg_0x18;	//+24
	uint8 cnt_0x19;	//+25
	uint8 cnt_0x1A;	//+26
} __attribute__((packed));

extern struct s_wekap swekap;

void ICACHE_FLASH_ATTR fn_timer_wekap(void * arg); // ETSTimerFunc

uint8 unk_3FFEA4CC[11];

static int ICACHE_FLASH_ATTR _wekap(int x)
{
	if(fpm_allow_tx()) fpm_do_wakeup();
	if(pm_is_open() == 0) return 0;
	if (swekap.flg_timer == 0) {
		ets_timer_setfn(&swekap.timer, (ETSTimerFunc *) fn_timer_wekap, NULL);
		swekap.flg_timer = 1;

	}
	uint8 z = pm_is_waked();
	if(z != 0 || swekap.flg_0x18 != 1) return 0;
	if(swekap.flg_0x18 == 0) {
		pm_post(1);
		ets_timer_disarm(swekap.timer);
		ets_timer_arm_new(swekap.timer, 10, 0, 1);
		swekap.flg_0x18 = 1;
	}
	if(++swekap.cnt_0x19 > 10) {
		os_printf("DEFERRED FUNC NUMBER IS BIGGER THAN 10\n");
		swekap.cnt_0x19 = 10;
	}
	if(swekap.cnt_0x1A + swekap.cnt_0x19 <= 10) { // ??
		unk_3FFEA4CC[swekap.cnt_0x1A + swekap.cnt_0x19] = x;
	}
	unk_3FFEA4CC[10] = x;
	return 0x7F;
}

static void ICACHE_FLASH_ATTR _new_opmode(int opmode) {
	.... // TODO
}

uint8 unk_0x3FFEA493;
uint8 unk_0x3FFEA494;
uint8 unk_0x3FFF083C;
extern uint8 OpmodChgIsOnGoing;

static bool ICACHE_FLASH_ATTR _wifi_set_opmode(uint8 opmode, bool flg)
{
	if(opmode > STATIONAP_MODE || g_ic.c[446] != 0) return false;
	if(_wekap(5) != 0xFF) { // _wekap() не может возвратить 0xFF !!!
		if(g_ic.g.wifi_store.wfmode[0] == opmode) return true;
		g_ic.g.wifi_store.wfmode[0] = opmode;
		OpmodChgIsOnGoing = 1;
		if(user_init_flag == 1) {
			_new_opmode(opmode);
		}
		OpmodChgIsOnGoing = 0;
		if(flg == 1) {
			system_param_save_with_protect((flashchip->chip_size/flashchip->sector_size)-3, &g_ic.g.wifi_store, sizeof(struct s_wifi_store));
		}
		return true;
	}
	unk_0x3FFEA493 = opmode;
	unk_0x3FFEA494 = flg;
	return true;
}

bool ICACHE_FLASH_ATTR wifi_set_opmode(uint8 opmode)
{
	return _wifi_set_opmode(opmode, true);
}

bool ICACHE_FLASH_ATTR wifi_set_opmode_current(uint8 opmode)
{
	return _wifi_set_opmode(opmode, true);
}

extern void * bcn_ie; // ??

void * ICACHE_FLASH_ATTR wifi_get_user_ie(int x)
{
	if(x) return NULL;
	else return bcn_ie;
}

enum phy_mode wifi_get_phy_mode(void){
	return g_ic.g.wifi_store.phy_mode;
}

bool wifi_set_phy_mode(enum phy_mode mode)
{
	if(mode < PHY_MODE_11B || mode > PHY_MODE_11N) return false;
	if(g_ic.c[446]) return false;
	if(g_ic.g.wifi_store.phy_mode == mode) return true;
	int opmode = wifi_get_opmode();
	g_ic.g.wifi_store.phy_mode = mode;
	system_param_save_with_protect((flashchip->chip_size/flashchip->sector_size)-3, &g_ic.g.wifi_store, sizeof(struct s_wifi_store));
	if(user_init_flag == 1) {
		wifi_station_stop();
		wifi_softap_stop();
	}
	ieee80211_phy_init(mode);
	g_ic.g.wifi_store.field_820 = 0;
	if(mode == PHY_MODE_11N) ieee80211_ht_attach(&g_ic);
	if(user_init_flag != 1) return true;
	if(opmode != 1) return true;
	if(opmode == 1 || opmode == 3) {
		wifi_station_start();
		wifi_station_connect();
	}
	if(opmode == 2 || opmode == 3) {
		wifi_softap_start();
	}
	if(opmode == 1 && g_ic.g.netif1 != NULL) {
		netif_set_default(*g_ic.g.netif1);
	}
	return true;
}

void system_phy_set_powerup_option(uint8 option)
{
	phy_set_powerup_option(option); // { 0x6000073C = IO_RTC_POWERUP = option }
}

void ICACHE_FLASH_ATTR system_phy_set_rfoption(uint8 option)
{
	phy_afterwake_set_rfoption(option); // { 0x6000106C = option }
}

void ICACHE_FLASH_ATTR phy_afterwake_set_rfoption(uint8 option)
{
#if SDK ver1.4.0	
	uint32 x = RTC_RAM_BASE[0x6C>>2] & 0xFF00FFFF;	// 0x6000106C
	RTC_RAM_BASE[0x6C>>2] = x | (option << 16);
#else	
	uint32 x = (RTC_RAM_BASE[0x60>>2] & 0xFFFF) | (option << 16);
	RTC_RAM_BASE[0x60>>2] = x; // 0x60001060
	RTC_RAM_BASE[0x78>>2] |= x; // 0x60001078 
#endif	
}

void ICACHE_FLASH_ATTR phy_set_powerup_option(int option)
{
	RTC_BASE[0x3C>>2] = option; // 0x6000073C
}	

bool ICACHE_FLASH_ATTR system_deep_sleep_set_option(uint8 option)
{
#if SDK >= ver1.3.0	
	switch(option) {
		case 0:
		case 1:
		case 2:
		case 4:
		_deep_sleep_mode = option;
		return true;
	}
	return false;
#else	
	uint32 x = (RTC_RAM_BASE[0x60>>2] & 0xFFFF) | (option << 16);
	RTC_RAM_BASE[0x60>>2] = x; // 0x60001060
	rtc_mem_check(false); // пересчитать OR
	// return всегда fasle
#endif	
}

#if SDK >= ver1.3.0
bool ICACHE_FLASH_ATTR deep_sleep_set_option(uint8 option)
{
	RTC_RAM_BASE[0x6C>>2] = (RTC_RAM_BASE[0x6C>>2] & 0xFF00FFFF)  | ((option&0xFF) << 16); // 0x6000106C
	rtc_mem_check(false); // пересчитать OR
	// return всегда fasle
}
#endif

bool ICACHE_FLASH_ATTR rtc_mem_check(bool flg)
{
	volatile uint32 * ptr = &RTC_RAM_BASE[0];
	uint32 region_or_crc = 0;
#if SDK >= ver1.3.0	
	while(ptr != &RTC_RAM_BASE[0x68>>2]) region_or_crc += *ptr++; // SDK 1.4.0
	region_or_crc ^= 0x7F;
#else	
	while(ptr != &RTC_RAM_BASE[0x78>>2]) region_or_crc |= *ptr++; // Old SDK
#endif	
	if(flg == false) {
		*ptr = region_or_crc; // RTC_RAM_BASE[0x78>>2] = region_or_crc
		return false;
	}
	return (*ptr != region_or_crc);
}

uint32 ICACHE_FLASH_ATTR rtc_mem_backup(uint32 *mem_start, uint32 *mem_end, uint32 off_ram_rtc)
{
	uint32 i = (((uint32)mem_end - (uint32)mem_start + 3) >> 2) + 1;
	volatile uint32 * ptr_reg = &RTC_RAM_BASE[off_ram_rtc>>2];
	uint32 ret = i << 2;
	while(i--) *ptr_reg++ = *mem_start++;
	return ret;
}

uint32 ICACHE_FLASH_ATTR rtc_mem_recovery(uint32 *mem_start, uint32 *mem_end, uint32 off_ram_rtc)
{
	uint32 i = (((uint32)mem_end - (uint32)mem_start + 3) >> 2) + 1;
	volatile uint32 * ptr_reg = &RTC_RAM_BASE[off_ram_rtc>>2];
	uint32 ret = i << 2;
	while(i--) *mem_start++ = *ptr_reg++;
	return ret;
}

