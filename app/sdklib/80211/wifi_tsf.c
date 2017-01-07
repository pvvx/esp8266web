/*
 * wifi_tsf.c
 *
 *  Created on: 28 дек. 2016 г.
 *      Author: pvvx
 */
#if 1

#include "user_config.h"
#include "bios.h"
#include "hw/esp8266.h"
#include "hw/eagle_soc.h"
#include "sdk/add_func.h"
#include "ets_sys.h"
#include "osapi.h"


struct ieee80211_scanparams {
	uint8_t		status;		/* +0 bitmask of IEEE80211_BPARSE_* */
	uint8_t		chan;		/* +1 channel # from FH/DSPARMS */
	uint8_t		bchan;		/* +2 curchan's channel # */
	uint8_t		fhindex;    // +3
	uint16_t	fhdwell;	/* +4 FHSS dwell interval */
	uint16_t	capinfo;	/* +6  802.11 capabilities */
	uint16_t	erp;		/* +8 NB: 0x100 indicates ie present */
	uint16_t	bintval;    // +a
	uint8_t		timoff;     // +c
	uint8_t		*ies;		/* +10 all captured ies */
	size_t		ies_len;	/* +14 length of all captured ies */
	uint8_t		*tim;       // +18
	uint8_t		*tstamp;	// +1c
	uint8_t		*country;
	uint8_t		*ssid;
	uint8_t		*rates;
	uint8_t		*xrates;
	uint8_t		*doth;
	uint8_t		*wpa;
	uint8_t		*rsn;
	uint8_t		*wme;
	uint8_t		*htcap;
	uint8_t		*htinfo;
	uint8_t		*ath;
	uint8_t		*tdma;
	uint8_t		*csa;
	uint8_t		*quiet;
	uint8_t		*meshid;
	uint8_t		*meshconf;
	uint8_t		*spare[3];
};

volatile uint64 recv_tsf; // принятый TSF от внешней AP
volatile uint32 recv_tsf_time;	// время приема TSF (младшие 32 бита - считем, что для времязависимых приложений не ставят время следования beacon более 4294967296 us :) )

// #define MAC_TIMER64BIT_COUNT_ADDR 0x3ff21048
//===============================================================================
// get_mac_time() = get_tsf_ap() -  TSF AP
//-------------------------------------------------------------------------------
uint64 ICACHE_FLASH_ATTR get_mac_time(void)
{
	union {
		volatile uint32 dw[2];
		uint64 dd;
	}ux;
	ets_intr_lock();
	volatile uint32 * ptr = (volatile uint32 *)MAC_TIMER64BIT_COUNT_ADDR;
	ux.dw[0] = ptr[0];
	ux.dw[1] = ptr[1];
	if(ux.dw[1] != ptr[1]) {
		ux.dw[0] = ptr[0];
		ux.dw[1] = ptr[1];
	}
	ets_intr_unlock();
	return ux.dd;
}

extern void cnx_update_bss_mor_(int a2,  struct ieee80211_scanparams *scnp, void *a4);
//===============================================================================
// save_tsf_station()
//-------------------------------------------------------------------------------
void ICACHE_FLASH_ATTR cnx_update_bss_more(int a2,  struct ieee80211_scanparams *scnp, void *a4)
{
//?	ets_intr_lock();
	recv_tsf_time = *((volatile uint32 *)MAC_TIMER64BIT_COUNT_ADDR);
	os_memcpy((void *)&recv_tsf, (void *)scnp->tstamp, 8);
//?	ets_intr_unlock();
	cnx_update_bss_mor_(a2, scnp, a4);
}
//===============================================================================
// get_tsf_station()
//-------------------------------------------------------------------------------
uint64 ICACHE_FLASH_ATTR get_tsf_station(void)
{
	ets_intr_lock();
	uint32 cur_mac_time = *((volatile uint32 *)MAC_TIMER64BIT_COUNT_ADDR) - recv_tsf_time;
	uint64 cur_tsf = recv_tsf + cur_mac_time;
	ets_intr_unlock();
	return cur_tsf;
}

#endif
