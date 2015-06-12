/******************************************************************************
 * FileName: eagle_lwip_if.h
 * Description: Alternate SDK (libmain.a)
 * Author: PV`
 * (c) PV` 2015
*******************************************************************************/
#ifndef _INCLUDE_EAGLE_LWIP_IF_H_
#define _INCLUDE_EAGLE_LWIP_IF_H_

#include "libmain.h"
#include "lwip/pbuf.h"
#include "lwip/ip_addr.h"

struct myif_state {
    struct netif *myif;	//+0
    uint32_t padding[(176-4)>>2]; //+4
    uint32_t dhcps_if; //+176 // + 0xB0
};

err_t ieee80211_output_pbuf(struct netif *netif, struct pbuf *p);
struct netif *eagle_lwip_getif(int n) ICACHE_FLASH_ATTR;
struct netif *eagle_lwip_if_alloc(struct myif_state *state, u8_t hw[6], struct ip_info * ips) ICACHE_FLASH_ATTR;

extern uint8 * * phostname;
extern bool default_hostname; //  = true;


#endif /* _INCLUDE_EAGLE_LWIP_IF_H_ */
