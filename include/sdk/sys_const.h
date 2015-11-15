 /******************************************************************************
 * FileName: sys_const.h
 * Description: read/write sdk_sys_cont (esp_init_data_default.bin)
 * Author: PV`
 *******************************************************************************/
#ifndef _INCLUDE_SYS_CONST_H_
#define _INCLUDE_SYS_CONST_H_

#include "sdk_config.h"
#include "hw/esp8266.h"

#define MAX_IDX_SYS_CONST 128
#define SIZE_SYS_CONST 128
#define SIZE_SAVE_SYS_CONST 756 // размер сохранения блока системных констант в секторе с номером (max_flash - 4). SDK 1.4.0

#ifdef USE_FIX_SDK_FLASH_SIZE
#define esp_init_data_default_sec (0x7C)
#define esp_init_data_default_addr (0x7C000)
#else
#define esp_init_data_default_sec (flashchip->chip_size - 4)
#define esp_init_data_default_addr (flashchip->chip_size - 4 * SPI_FLASH_SEC_SIZE)
#endif

#define	sys_const_spur_freq_cfg		26 // rx_param25: spur_freq=spur_freq_cfg/spur_freq_cfg_div
#define	sys_const_spur_freq_cfg_div	27	// rx_param26
#define	sys_const_spur_freq_en_h	28  // rx_param27: each bit for 1 channel, 1 to select the spur_freq if in band, else 40
#define	sys_const_spur_freq_en_l	29  // rx_param28
#define	sys_const_target_power_qdb_0	34	// tx_param5: 82 means target power is 82/4=20.5dbm
#define	sys_const_target_power_qdb_1	35	// tx_param6: 78 means target power is 78/4=19.5dbm 
#define	sys_const_target_power_qdb_2	36  // tx_param7: 74 means target power is 74/4=18.5dbm 
#define	sys_const_target_power_qdb_3	37  // tx_param8: 68 means target power is 68/4=17dbm 
#define	sys_const_target_power_qdb_4	38  // tx_param9: 64 means target power is 64/4=16dbm 
#define	sys_const_target_power_qdb_5	39  // tx_param10: 56 means target power is 56/4=14dbm
#define	sys_const_target_power_index_mcs0	40 //  tx_param11: target power index is 0, means target power is target_power_qdb_0 20.5dbm;     (1m,2m,5.5m,11m,6m,9m) 
#define	sys_const_target_power_index_mcs1	41	// tx_param12: target power index is 0, means target power is target_power_qdb_0 20.5dbm;     (12m)
#define	sys_const_target_power_index_mcs2	42  // tx_param13: target power index is 1, means target power is target_power_qdb_1 19.5dbm;     (18m)
#define	sys_const_target_power_index_mcs3	43  // tx_param14: target power index is 1, means target power is target_power_qdb_1 19.5dbm;     (24m)
#define	sys_const_target_power_index_mcs4	44  // tx_param15: target power index is 2, means target power is target_power_qdb_2 18.5dbm;     (36m)
#define	sys_const_target_power_index_mcs5	45  // tx_param16: target power index is 3, means target power is target_power_qdb_3 17dbm;        (48m)
#define	sys_const_target_power_index_mcs6	46  // tx_param17: target power index is 4, means target power is target_power_qdb_4 16dbm;        (54m)
#define	sys_const_target_power_index_mcs7	47  // tx_param18: target power index is 5, means target power is target_power_qdb_5 14dbm
#define	sys_const_crystal_26m_en	48	// soc_param0: 0: 40MHz, 1: 26MHz, 2: 24MHz
#define	sys_const_sdio_configure	50	// soc_param2: 
										// 0: Auto by pin strapping
										// 1: SDIO dataoutput is at negative edges (SDIO V1.1)
										// 2: SDIO dataoutput is at positive edges (SDIO V2.0)
#define	sys_const_bt_configure		51	// soc_param3: 
										// 0: None,no bluetooth
										// 1: GPIO0 -> WLAN_ACTIVE/ANT_SEL_WIFI
										//    MTMS -> BT_ACTIVE
										//    MTCK  -> BT_PRIORITY
										//    U0RXD -> ANT_SEL_BT
										// 2: None, have bluetooth
										// 3: GPIO0 -> WLAN_ACTIVE/ANT_SEL_WIFI
										//    MTMS -> BT_PRIORITY
										//    MTCK  -> BT_ACTIVE
										//    U0RXD -> ANT_SEL_BT
#define	sys_const_bt_protocol		52	// soc_param4: 
										// 0: WiFi-BT are not enabled. Antenna is for WiFi
										// 1: WiFi-BT are not enabled. Antenna is for BT
										// 2: WiFi-BT 2-wire are enabled, (only use BT_ACTIVE), independent ant
										// 3: WiFi-BT 3-wire are enabled, (when BT_ACTIVE = 0, BT_PRIORITY must be 0), independent ant
										// 4: WiFi-BT 2-wire are enabled, (only use BT_ACTIVE), share ant
										// 5: WiFi-BT 3-wire are enabled, (when BT_ACTIVE = 0, BT_PRIORITY must be 0), share ant
#define	sys_const_dual_ant_configure	53	// soc_param5:
										// 0: None
										// 1: dual_ant (antenna diversity for WiFi-only): GPIO0 + U0RXD
										// 2: T/R switch for External PA/LNA:  GPIO0 is high and U0RXD is low during Tx
										// 3: T/R switch for External PA/LNA:  GPIO0 is low and U0RXD is high during Tx  
#define	sys_const_share_xtal		55	// soc_param7  
										// This option is to share crystal clock for BT
										// The state of Crystal during sleeping
										// 0: Off
										// 1: Forcely On
										// 2: Automatically On according to XPD_DCDC
										// 3: Automatically On according to GPIO2
#define	sys_const_spur_freq_cfg_2  		64  // rx_param29: spur_freq_2=spur_freq_cfg_2/spur_freq_cfg_div_2
#define	sys_const_spur_freq_cfg_div_2	65  // rx_param30 
#define	sys_const_spur_freq_en_h_2  	66  // rx_param31: each bit for 1 channel, and use [spur_freq_en, spur_freq_en_2] to select the spur's priority
#define	sys_const_spur_freq_en_l_2  	67  // rx_param32
#define	sys_const_spur_freq_cfg_msb  	68  // rx_param33
#define	sys_const_spur_freq_cfg_2_msb	69  // rx_param34
#define	sys_const_spur_freq_cfg_3_low	70  // rx_param35: spur_freq_3=((spur_freq_cfg_3_high<<8)+spur_freq_cfg_3_low)/10+2400
#define	sys_const_spur_freq_cfg_3_high	71  // rx_param36
#define	sys_const_spur_freq_cfg_4_low	72  // rx_param37: spur_freq_4=((spur_freq_cfg_4_high<<8)+spur_freq_cfg_4_low)/10+2400
#define	sys_const_spur_freq_cfg_4_high	73  // rx_param38
#define	sys_const_low_power_en		93  // tx_param24: 0: disable low power mode, 1: enable low power mode
#define	sys_const_lp_rf_stg10		94  // tx_param25: the attenuation of RF gain stage 0 and 1, 0xf: 0db,  0xe: -2.5db,  0xd: -6db,  0x9: -8.5db, 0xc: -11.5db,  0x8: -14db,  0x4: -17.5, 0x0: -23
#define	sys_const_lp_bb_att_ext		95  // tx_param26: the attenuation of BB gain, 0: 0db,  1: -0.25db,  2: -0.5db,  3: -0.75db,  4: -1db,   5: -1.25db, 6: -1.5db, 7: -1.75db, 8: -2db ...(max valve is 24(-6db))
#define	sys_const_pwr_ind_11b_en	96  // tx_param27: 0: 11b power is same as mcs0 and 6m, 1: enable 11b power different with ofdm
#define	sys_const_pwr_ind_11b_0		97  // tx_param28: 1m, 2m power index [0~5]
#define	sys_const_pwr_ind_11b_1		98  // tx_param29: 5.5m, 11m power index [0~5]
#define	sys_const_vdd33_const  	107	// tx_param37: the voltage of PA_VDD
									// x=0xff: it can measure VDD33,
									// 18<=x<=36: use input voltage,  the value is voltage*10, 33 is 3.3V,   30 is 3.0V,
									// x<18 or x>36: default voltage is 3.3V
#define	sys_const_freq_correct_en 	112 // tx_param42:
										// bit[0]:0->do not correct frequency offset , 1->correct frequency offset .
										// bit[1]:0->bbpll is 168M, it can correct + and - frequency offset,  1->bbpll is 160M, it only can correct + frequency offset
										// bit[2]:0->auto measure frequency offset and correct it, 1->use 113 byte force_freq_offset to correct frequency offset. 
										// 0: do not correct frequency offset.
										// 1: auto measure frequency offset and correct it,  bbpll is 168M, it can correct + and - frequency offset.
										// 3: auto measure frequency offset and correct it,  bbpll is 160M, it only can correct + frequency offset.
										// 5: use 113 byte force_freq_offset to correct frequency offset, bbpll is 168M, it can correct + and - frequency offset.
										// 7: use 113 byte force_freq_offset to correct frequency offset, bbpll is 160M , it only can correct + frequency offset . 
#define	sys_const_force_freq_offset 113 // tx_param43: signed, unit is 8khz

#define get_sys_const(a)  ((*((unsigned int *)((unsigned int)((a) + FLASH_BASE + FLASH_SYSCONST_ADR) & (~3))))>>(((unsigned int)a & 3) << 3))

extern uint8 chip6_phy_init_ctrl[128]; // 
extern uint8 phy_in_most_power; // system_phy_set_max_tpw()

#endif /* _INCLUDE_SYS_CONST_H_ */
