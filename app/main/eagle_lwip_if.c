/******************************************************************************
 * FileName: eagle_lwip_if.c (libmain.a)
 * Description: eagle_lwip_if for SDK 1.1.0
 * Author: PV`
 * Old ver: kadamski
 * https://github.com/kadamski/esp-lwip/blob/esp8266-1.4.1/our/eagle_lwip_if.c
*******************************************************************************/
#if 1
#include "user_config.h"
#include "user_interface.h"
#include "add_sdk_func.h"
#include "bios.h"
#include "hw/esp8266.h"
#include "ets_sys.h"
#include "lwip/pbuf.h"
#include "lwip/netif.h"
#include "netif/etharp.h"
#include "lwip/app/dhcpserver.h"
#include "eagle_lwip_if.h"

#define QUEUE_LEN 10
#define TASK_IF0_PRIO 28
#define TASK_IF1_PRIO 29
#define DEFAULT_MTU 1500

err_t ICACHE_FLASH_ATTR ieee80211_output_pbuf(struct netif *netif, struct pbuf *p);

extern uint8 dhcps_flag;
extern void ppRecycleRxPkt(void *esf_buf); // struct pbuf -> eb

ETSEvent *lwip_if_queues[2];

struct netif *eagle_lwip_getif(int n);

static void ICACHE_FLASH_ATTR task_if0(struct ETSEventTag *e)
{
    struct netif *myif = eagle_lwip_getif(0);
    if (e->sig == 0) {
    	if(myif != NULL) {
        	myif->input((struct pbuf *)e->par, myif);
    	}
    	else {
    		struct pbuf *p = (struct pbuf *)e->par;
    		ppRecycleRxPkt(p->eb); // esf_buf
    		pbuf_free(p);
    	}
    }
}

static void ICACHE_FLASH_ATTR task_if1(struct ETSEventTag *e)
{
    struct netif *myif = eagle_lwip_getif(1);
    if (e->sig == 0) {
    	if(myif != NULL) {
        	myif->input((struct pbuf *)e->par, myif);
    	}
    	else {
    		struct pbuf *p = (struct pbuf *)e->par;
    		ppRecycleRxPkt(p->eb);
    		pbuf_free(p);
    	}
    }
}

static err_t ICACHE_FLASH_ATTR init_fn(struct netif *myif)
{
    myif->hwaddr_len = 6; //+42
    myif->mtu = DEFAULT_MTU; //+40 1500
    myif->flags = NETIF_FLAG_IGMP | NETIF_FLAG_ETHARP | NETIF_FLAG_LINK_UP | NETIF_FLAG_BROADCAST; // +49 = 0x0B2
    return 0;
}

struct netif * ICACHE_FLASH_ATTR eagle_lwip_getif(int n)
{
	struct netif **ret;
    if (n==0) {
    	ret = g_ic.g.netif1;
    }
    else if (n == 1) {
    	ret = g_ic.g.netif2;
    }
    else return (void *) n;
	if(ret != NULL) return *ret;
	return NULL;
}

struct netif * ICACHE_FLASH_ATTR
eagle_lwip_if_alloc(struct myif_state *state, u8_t hw[6], struct ip_info * ips)
{
#if 1 // optimize ...
    struct netif *myif = state->myif;
    if (myif == NULL) {
        myif = (void *) pvPortMalloc(sizeof(struct netif)); // pvPortZalloc(60)
        state->myif = myif;
    }
    myif->state = state; // +28
    myif->name[0] = 'e'; // +50
    myif->name[1] = 'w'; // +51
    myif->linkoutput = ieee80211_output_pbuf; // +24
    myif->output = etharp_output; // +20
    ets_memcpy(myif->hwaddr, hw, 6); // +43

	ETSEvent *queue = (void *) pvPortMalloc(sizeof(struct ETSEventTag) * QUEUE_LEN); // pvPortZalloc(80)
    if (state->dhcps_if != 0) { // +176
	    lwip_if_queues[1] = queue;
	    netif_set_addr(myif, &ips->ip, &ips->netmask, &ips->gw);
	    ets_task(task_if1, TASK_IF1_PRIO, (ETSEvent *)lwip_if_queues[1], QUEUE_LEN);
	    netif_add(myif, &ips->ip, &ips->netmask, &ips->gw, state, init_fn, ethernet_input);
	    if(dhcps_flag) {
	    	dhcps_start(ips);
//	    	os_printf("dhcp server start:(ip:%08x,mask:%08x,gw:%08x)\n", ips->ip.addr, ips->netmask.addr, ips->gw.addr);
	    }
    }
    else {
    	lwip_if_queues[0] = queue;
	    ets_task(task_if0, TASK_IF0_PRIO, (ETSEvent *)lwip_if_queues[0], QUEUE_LEN);
	    struct ip_info ipn;
		if(wifi_station_dhcpc_status()) {
			ipn =  *ips;
		}
		else {
		    ipn.ip.addr = 0;
		    ipn.netmask.addr = 0;
		    ipn.gw.addr = 0;
		}
	    netif_add(myif, &ipn.ip, &ipn.netmask, &ipn.gw, state, init_fn, ethernet_input);
    }
    return myif;
#else // source
	ETSEvent *queue;
    ip_addr_t ipaddr;
    ip_addr_t netmask;
    ip_addr_t gw;
    struct netif *myif = state->myif;

    if (myif == NULL) {
        myif = (void *) pvPortMalloc(sizeof(struct netif)); // pvPortZalloc(60)
        state->myif = myif;
    }
    myif->state = state; // +28
    myif->name[0] = 'e'; // +50
    myif->name[1] = 'w'; // +51
    myif->linkoutput = ieee80211_output_pbuf; // +24
    myif->output = etharp_output; // +20
    ets_memcpy(myif->hwaddr, hw, 6); // +43

    if (state->dhcps_if != 0) { // +176
	    ipaddr = ips->ip;
	    netmask = ips->netmask;
	    gw = ips->gw;
	    netif_set_addr(myif, &ipaddr, &netmask, &gw);
	    queue = (void *) pvPortMalloc(sizeof(struct ETSEventTag) * QUEUE_LEN); // pvPortZalloc(80)
	    lwip_if_queues[1] = queue;
	    ets_task(task_if1, TASK_IF1_PRIO, (ETSEvent *)queue, QUEUE_LEN);
	    netif_add(myif, &ipaddr, &netmask, &gw, state, init_fn, ethernet_input);
	    if(dhcps_flag) {
	    	dhcps_start(ips);
//	    	os_printf("dhcp server start:(ip:%08x,mask:%08x,gw:%08x)\n", ipaddr.addr, netmask.addr, gw.addr);
	    }

    }
    else {
    	if(wifi_station_dhcpc_status()) {
    	    ipaddr = ips->ip;
    	    netmask = ips->netmask;
    	    gw = ips->gw;
    	}
    	else {
        	ipaddr.addr = 0;
        	netmask.addr = 0;
        	gw.addr = 0;
    	}
	    queue = (void *) pvPortMalloc(sizeof(struct ETSEventTag) * QUEUE_LEN); // pvPortZalloc(80)
	    lwip_if_queues[0] = queue;
	    ets_task(task_if0, TASK_IF0_PRIO, (ETSEvent *)queue, QUEUE_LEN);
	    netif_add(myif, &ipaddr, &netmask, &gw, state, init_fn, ethernet_input);
    }
    return myif;
#endif
}

void eagle_lwip_if_free(struct myif_state *state)
{
	if(state->dhcps_if == 0) {
		netif_remove(state->myif);
//		if(lwip_if_queues[0] != NULL)
			vPortFree(lwip_if_queues[0]);
	}
	else {
		if(dhcps_flag) dhcps_stop();
		netif_remove(state->myif);
//		if(lwip_if_queues[1] != NULL)
			vPortFree(lwip_if_queues[1]);
	}
	if(state->myif != NULL) {
		vPortFree(state->myif);
		state->myif = NULL;
	}
}

#endif
