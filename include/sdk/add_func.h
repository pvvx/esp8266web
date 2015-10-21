
#ifndef _INCLUDE_ADD_FUNC_H_
#define _INCLUDE_ADD_FUNC_H_

#include "user_config.h"
#include "ets_sys.h"
#include "mem_manager.h"
#include "sdk/ets_run_new.h"

struct s_info {
	uint32 ap_ip;	//+00
	uint32 ap_mask;	//+04
	uint32 ap_gw;	//+08
	uint32 st_ip;	//+0C
	uint32 st_mask;	//+10
	uint32 st_gw;	//+14
	uint8 ap_mac[6];	//+18
	uint8 st_mac[6];	//+1E
}  __attribute__((packed, aligned(4)));

extern struct s_info info;

extern uint8 * hostname; // wifi_station_get_hostname(), wlan_lwip_if.h
extern bool default_hostname; // eagle_lwip_if.c

#if DEF_SDK_VERSION > 999 // SDK > 0.9.6 b1
//uint32 system_adc_read(void); // user_interface.h
//void system_deep_sleep(uint32 time_in_us); // user_interface.h
//bool system_deep_sleep_set_option(uint8 option); // user_interface.h
//uint32 system_get_boot_mode(void); // user_interface.h
//uint32 system_get_boot_version(void); // user_interface.h
uint32 system_get_checksum(uint8 *ptr, uint32 len);
//uint32 system_get_chip_id(void); // user_interface.h
//uint32 system_get_cpu_freq(void); // user_interface.h // ets_get_cpu_frequency
//uint32 system_get_free_heap_size(void); // user_interface.h
bool system_get_os_print(void); // return os_print_enable
uint32 system_get_rtc_time(void); // return (uint32)(*((uint32*)0x6000071C))
//const uint8 *system_get_sdk_version(void); // user_interface.h
uint32 system_get_test_result(void);
//uint32 system_get_time(void); // x1 us, return WdevTimOffSet + (uint32)(*((uint32*)0x3FF20C00)) // user_interface.h
uint32 phy_get_mactime(void); // x1 us, return (uint32)(*((uint32*)0x3FF20C00))
uint32 system_get_userbin_addr(void);
//uint32 system_get_vdd33(void); // user_interface.h
//void system_init_done_cb(init_done_cb_t cb); typedef void (* init_done_cb_t)(void); // user_interface.h
//uint64 system_mktime(uint32 year, uint32 mon, uint32 day, uint32 hour, uint32 min, uint32 sec); // user_interface.h
//bool system_os_post(uint8 prio, os_signal_t sig, os_param_t par); // user_interface.h
//bool system_os_task(os_task_t task, uint8 prio, os_event_t *queue, uint8 qlen); // user_interface.h
bool system_overclock(void); // if(system_get_cpu_freq()==80) { cpu_overclock = 1, system_update_cpu_freq(160) }
//?system_param_error();
uint32 system_phy_temperature_alert(void);   // phy_get_check_flag(0);
void system_pp_recycle_rx_pkt(void *eb); //?system_pp_recycle_rx_pkt(); // ppRecycleRxPkt()-> lldesc_num2link() wDev_AppendRxBlocks() esf_buf_recycle()
//void system_print_meminfo(void); // user_interface.h
uint32 system_relative_time(uint32 x); // (*((uint32*)0x3FF20C00))- x
//void system_restart(void); // user_interface.h
//void system_restart_enhance(uint32 *bin); // system_get_boot_version() "failed: need boot >= 1.3\n" system_get_userbin_addr(), "restart to use user bin", wifi_param_save_protect(), system_restart()
//void system_restore(void); // user_interface.h
bool system_restoreclock(void); // if(cpu_overclock) system_update_cpu_freq(80) else return 0
//uint32 system_rtc_clock_cali_proc(void); // user_interface.h
//bool system_rtc_mem_read(uint8 src_addr, void *des_addr, uint16 load_size); // user_interface.h
//bool system_rtc_mem_write(uint8 des_addr, const void *src_addr, uint16 save_size); // user_interface.h
//void system_set_os_print(uint8 onoff); // user_interface.h
//void system_station_got_ip_set(ip_addr_t * ip_addr, ip_addr_t *sn_mask, ip_addr_t *gw_addr); //?system_station_got_ip_set(a,b,c); // print"ip:%d.%d.%d.%d,mask:%d.%d.%d.%d,gw:%d.%", ets_timer_disarm(sta_con_timer)
//void system_timer_reinit(void); // user_interface.h // устанавливает делитель таймера на :16 вместо :256 и т.д.
//void system_uart_swap(void); // user_interface.h
//bool system_update_cpu_freq(uint32 cpu_freq); // ets_update_cpu_frequency + bit0 0x3FF00014 // user_interface.h
//uint8 system_upgrade_flag_check(); // user_interface.h
//void system_upgrade_flag_set(uint8 flag); // user_interface.h
//void system_upgrade_reboot(void); // user_interface.h
//uint8 system_upgrade_userbin_check(void); // user_interface.h
bool system_upgrade_userbin_set(uint32 flag); // system_get_boot_version(), store flags
#endif

int atoi(const char *str) ICACHE_FLASH_ATTR;

int os_printf_plus(const char *format, ...) ICACHE_FLASH_ATTR;
int ets_sprintf(char *str, const char *format, ...) ICACHE_FLASH_ATTR;

void wifi_softap_set_default_ssid(void) ICACHE_FLASH_ATTR;
void wDev_Set_Beacon_Int(uint32_t) ICACHE_FLASH_ATTR;
extern void wDev_ProcessFiq(void) ICACHE_FLASH_ATTR;
void ets_timer_arm_new(ETSTimer *ptimer, uint32_t milliseconds, int repeat_flag, int isMstimer);
void sleep_reset_analog_rtcreg_8266(void) ICACHE_FLASH_ATTR;
void wifi_softap_cacl_mac(uint8 *mac_out, uint8 *mac_in) ICACHE_FLASH_ATTR;
void user_init(void);
int wifi_mode_set(int mode) ICACHE_FLASH_ATTR;
int wifi_station_start(void) ICACHE_FLASH_ATTR;

#if DEF_SDK_VERSION >= 1200
int wifi_softap_start(int) ICACHE_FLASH_ATTR;
int wifi_softap_stop(int) ICACHE_FLASH_ATTR;
#else
int wifi_softap_start(void) ICACHE_FLASH_ATTR;
#endif
int register_chipv6_phy(uint8 * esp_init_data) ICACHE_FLASH_ATTR; // esp_init_data_default[128]
void ieee80211_phy_init(int phy_mode) ICACHE_FLASH_ATTR; // ieee80211_setup_ratetable()
void lmacInit(void) ICACHE_FLASH_ATTR;
void wDev_Initialize(uint8 * mac) ICACHE_FLASH_ATTR;
void pp_attach(void) ICACHE_FLASH_ATTR;
void ieee80211_ifattach(void *_g_ic) ICACHE_FLASH_ATTR; // g_ic in main\Include\libmain.h
void pm_attach(void) ICACHE_FLASH_ATTR;
int fpm_attach(void) ICACHE_FLASH_ATTR; // all return  1
void cnx_attach(void *_g_ic) ICACHE_FLASH_ATTR; // g_ic in main\Include\libmain.h
void wDevEnableRx(void) ICACHE_FLASH_ATTR; // io(0x3FF20004) |= 0x80000000;

/* in mem_manager.h
void *pvPortMalloc(size_t xWantedSize) ICACHE_FLASH_ATTR;
void *pvPortRealloc(void * rmem, size_t newsize);
void vPortFree(void *pv) ICACHE_FLASH_ATTR;
size_t xPortGetFreeHeapSize(void) ICACHE_FLASH_ATTR;
void vPortInitialiseBlocks(void) ICACHE_FLASH_ATTR;
void *pvPortZalloc(size_t size) ICACHE_FLASH_ATTR;
*/
uint32 readvdd33(void) ICACHE_FLASH_ATTR;
int get_noisefloor_sat(void) ICACHE_FLASH_ATTR;
int read_hw_noisefloor(void) ICACHE_FLASH_ATTR;
int ram_get_fm_sar_dout(int) ICACHE_FLASH_ATTR;
// noise_init(), rom_get_noisefloor(), ram_set_noise_floor(), noise_check_loop(), ram_start_noisefloor()
// void sys_check_timeouts(void *timer_arg) ICACHE_FLASH_ATTR; // lwip
uint32 system_get_checksum(uint8 *ptr, uint32 len) ICACHE_FLASH_ATTR;
void read_macaddr_from_otp(uint8 *mac);

void wifi_station_set_default_hostname(uint8 * mac);

void user_init(void);

#ifdef USE_TIMER0
void timer0_start(uint32 us, bool repeat_flg);
void timer0_stop(void);

#ifdef TIMER0_USE_NMI_VECTOR
void timer0_init(void *func, uint32 par, bool nmi_flg);
#else
void timer0_init(void *func, void *par);
#endif
#endif

//void wifi_param_save_protect_with_check(uint16 startsector, int sectorsize, void *pdata, uint16 len);
void wifi_param_save_protect_with_check(int startsector, int sectorsize, void *pdata, int len);

#if DEF_SDK_VERSION >= 1300
#define deep_sleep_option ((RTC_RAM_BASE[0x6C>>2] >> 16) & 0xFF)
#else
#define deep_sleep_option (RTC_RAM_BASE[0x60>>2] >> 16)
#endif

extern struct rst_info rst_if;

#endif //_INCLUDE_ADD_FUNC_H_




