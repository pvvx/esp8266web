/******************************************************************************
 * FileName: rom_phy.h
 * Description: rtc & dtm funcs in ROM-BIOS
 * Alternate SDK
 * Author: PV`
 * (c) PV` 2015
 *******************************************************************************/

#ifndef _ROM_PHY_H_
#define _ROM_PHY_H_

typedef int (* func_2arg)(int, int);

//
//[0x3fffc730]=0x3fffc734  phy_get_romfuncs(void)
struct phy_func_tab {
	void * abs_temp;				// +000 rom_abs_temp
	void * chip_v5_disable_cca;		// +004 rom_chip_v5_disable_cca
	void * chip_v5_enable_cca;		// +008 rom_chip_v5_enable_cca
	void * chip_v5_sense_backoff;	// +012 rom_chip_v5_sense_backoff
	void * dc_iq_est;				// +016 rom_dc_iq_est
	void * fun20;					// +020 NULL ??
	void * en_pwdet;				// +024 rom_en_pwdet
	void * get_bb_atten;			// +028 rom_get_bb_atten
	void * get_corr_power;			// +032 rom_get_corr_power
	void * get_fm_sar_dout;			// +036 ram_get_fm_sar_dout / rom_get_fm_sar_dout
	void * get_noisefloor;			// +040 ram_get_noisefloor / rom_get_noisefloor
	void * get_power_db;			// +044 rom_get_power_db
	void * iq_est_disable;			// +048 rom_iq_est_disable
	void * iq_est_enable;			// +052 rom_iq_est_enable
	void * linear_to_db;			// +056 rom_linear_to_db
	void * set_txclk_en;			// +060 rom_set_txclk_en
	void * set_rxclk_en;			// +064 rom_set_rxclk_en
	func_2arg mhz2ieee;				// +068 rom_mhz2ieee
	void * rxiq_get_mis;			// +072 ram_rxiq_get_mis / rom_rxiq_get_mis
	void * sar_init;				// +076 rom_sar_init
	void * set_ana_inf_tx_scale;	// +080 rom_set_ana_inf_tx_scale
	void * set_loopback_gain;		// +084 rom_set_loopback_gain
	void * set_noise_floor;			// +088 ram_set_noise_floor / rom_set_noise_floor
	void * fun92;					// +092 NULL ??
	void * fun96;					// +096 NULL ??
	void * start_noisefloor;		// +100 ram_start_noisefloor / rom_start_noisefloor
	void * start_tx_tone;			// +104 rom_start_tx_tone
	void * stop_tx_tone;			// +108 rom_stop_tx_tone
	void * txtone_linear_pwr;		// +112 rom_txtone_linear_pwr
	void * tx_mac_disable;			// +116 ram_tx_mac_disable / rom_tx_mac_disable
	void * tx_mac_enable;			// +120 ram_tx_mac_enable
	void * ana_inf_gating_en;		// +124 ram_ana_inf_gating_en / rom_ana_inf_gating_en
	void * set_channel_freq;		// +128 rom_set_channel_freq
	void * chip_50_set_channel;		// +132 rom_chip_50_set_channel
	void * chip_v6_rx_init;			// +136 ram_chip_v6_rx_init / rom_chip_v5_rx_init
	void * chip_v5_tx_init;			// +140 rom_chip_v5_tx_init
	void * i2c_readReg;				// +144 rom_i2c_readReg
	void * i2c_readReg_Mask;		// +148 rom_i2c_readReg_Mask
	void * i2c_writeReg;			// +152 rom_i2c_writeReg
	void * i2c_writeReg_Mask;		// +156 rom_i2c_writeReg_Mask
	void * pbus_debugmode;			// +160 ram_pbus_debugmode / rom_pbus_debugmode
	void * pbus_enter_debugmode;	// +164 rom_pbus_enter_debugmode
	void * pbus_exit_debugmode;		// +168 rom_pbus_exit_debugmode
	void * pbus_force_test;			// +172 rom_pbus_force_test
	void * pbus_rd;					// +176 rom_pbus_rd
	void * pbus_set_rxgain;			// +180 rom_pbus_set_rxgain
	void * pbus_set_txgain;			// +184 rom_pbus_set_txgain
	void * pbus_workmode;			// +188 rom_pbus_workmode
	void * pbus_xpd_rx_off;			// +192 rom_pbus_xpd_rx_off
	void * pbus_xpd_rx_on;			// +196 rom_pbus_xpd_rx_on
	void * pbus_xpd_tx_off;			// +200 rom_pbus_xpd_tx_off
	void * pbus_xpd_tx_on;			// +204 rom_pbus_xpd_tx_on
	void * pbus_xpd_tx_on__low_gain;// +208 rom_pbus_xpd_tx_on__low_gain
	void * phy_reset_req;			// +212 rom_phy_reset_req
	void * restart_cal;				// +216 ram_restart_cal / rom_restart_cal
	void * rfpll_reset;				// +220 rom_rfpll_reset
	void * write_rfpll_sdm;			// +224 rom_write_rfpll_sdm
	void * rfpll_set_freq;			// +228 rom_rfpll_set_freq
	void * cal_tos_v60;				// +232 ram_cal_tos_v60 / rom_cal_tos_v50
	void * pbus_dco___SA2;			// +236 rom_pbus_dco___SA2
	void * rfcal_pwrctrl;			// +240 rom_rfcal_pwrctrl
	void * rfcal_rxiq;				// +244 rom_rfcal_rxiq
	void * rfcal_rxiq_set_reg;		// +248 rom_rfcal_rxiq_set_reg
	void * rfcal_txcap;				// +252 rom_rfcal_txcap
	void * rfcal_txiq;				// +256 rom_rfcal_txiq
	void * rfcal_txiq_cover;		// +260 rom_rfcal_txiq_cover
	void * rfcal_txiq_set_reg;		// +264 rom_rfcal_txiq_set_reg
	void * rxiq_cover_mg_mp;		// +268 ram_rxiq_cover_mg_mp / rom_rxiq_cover_mg_mp
	void * set_txbb_atten;			// +272 rom_set_txbb_atten
	void * set_txiq_cal;			// +276 rom_set_txiq_cal
	void * xxx_end;					// +280 NULL
};


struct phy_func_tab * phy_get_romfuncs(void);

#endif /* _ROM_PHY_H_ */

