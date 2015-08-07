/******************************************************************************
 * FileName: eagle_lwip_if.c (libmain.a)
 * Description: eagle_lwip_if for SDK 1.2.0
 * (c) PV` 2015
*******************************************************************************/

#include "user_config.h"

#ifdef USE_OPEN_LWIP

#include "user_interface.h"
#include "sdk/add_func.h"
#include "bios.h"
#include "hw/esp8266.h"
#include "ets_sys.h"
#include "lwip/pbuf.h"
#include "lwip/netif.h"
#include "netif/etharp.h"
#include "lwip/app/dhcpserver.h"
#include "sdk/libmain.h"
#include "lwip/pbuf.h"
#include "lwip/ip_addr.h"
#include "netif/wlan_lwip_if.h"

#define QUEUE_LEN 10
#ifdef IP_FRAG_MAX_MTU
#define DEFAULT_MTU IP_FRAG_MAX_MTU // (TCP_MSS+40) ?
#else
#define DEFAULT_MTU 1500
#endif

extern uint8 dhcps_flag;
extern void ppRecycleRxPkt(void *esf_buf); // struct pbuf -> eb

uint8 * hostname LWIP_DATA_IRAM_ATTR;
bool default_hostname; //  = true;

ETSEvent *lwip_if_queues[2] LWIP_DATA_IRAM_ATTR;

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
    myif->hwaddr_len = 6; // +46
    myif->mtu = DEFAULT_MTU; //+44 1500
    myif->flags = NETIF_FLAG_IGMP | NETIF_FLAG_ETHARP | NETIF_FLAG_LINK_UP | NETIF_FLAG_BROADCAST; // +53 = 0x0B2
    return 0;
}

struct netif * ICACHE_FLASH_ATTR eagle_lwip_getif(int index)
{
	struct netif **ret;
    if (index == 0) {
    	ret = g_ic.g.netif1;
    }
    else if (index == 1) {
    	ret = g_ic.g.netif2;
    }
    else return NULL;
	if(ret != NULL) return *ret;
	return NULL;
}

struct netif * ICACHE_FLASH_ATTR eagle_lwip_if_alloc(struct ieee80211_conn *conn, uint8 *macaddr, struct ip_info *info)
{
	struct netif *myif = conn->myif;
    if (myif == NULL) {
        myif = (void *) pvPortMalloc(sizeof(struct netif)); // SDK 1.1.2 : pvPortZalloc(64)
        conn->myif = myif;
    }
    if(myif == NULL) return NULL;
    if (conn->dhcps_if == 0) { // +176
        if(default_hostname) {
    		wifi_station_set_default_hostname(macaddr);
    	}
    	myif->hostname = hostname; // +40
    }
    else myif->hostname = NULL; // +40

    myif->state = conn; // +28
    myif->name[0] = 'e'; // + 54
    myif->name[1] = 'w'; // + 55
    myif->output = etharp_output; // +20
    myif->linkoutput = ieee80211_output_pbuf; // +24
    ets_memcpy(myif->hwaddr, macaddr, 6); // +47

	ETSEvent *queue = (void *) pvPortMalloc(sizeof(struct ETSEventTag) * QUEUE_LEN); // pvPortZalloc(80)
	if(queue == NULL) return NULL;

    if (conn->dhcps_if != 0) { // +176
	    lwip_if_queues[1] = queue;
	    netif_set_addr(myif, &info->ip, &info->netmask, &info->gw);
	    ets_task(task_if1, LWIP_IF1_PRIO, (ETSEvent *)lwip_if_queues[1], QUEUE_LEN);
	    netif_add(myif, &info->ip, &info->netmask, &info->gw, conn, init_fn, ethernet_input);
	    if(dhcps_flag) {
	    	dhcps_start(info);
	    	os_printf("dhcp server start:(ip:" IPSTR ",mask:" IPSTR ",gw:" IPSTR ")\n", IP2STR(&info->ip), IP2STR(&info->netmask), IP2STR(&info->gw));
	    }
    }
    else {
    	lwip_if_queues[0] = queue;
	    ets_task(task_if0, LWIP_IF0_PRIO, (ETSEvent *)lwip_if_queues[0], QUEUE_LEN);
	    struct ip_info ipn;
		if(wifi_station_dhcpc_status()) {
		    ipn.ip.addr = 0;
		    ipn.netmask.addr = 0;
		    ipn.gw.addr = 0;
		}
		else {
			ipn =  *info;
		}
	    netif_add(myif, &ipn.ip, &ipn.netmask, &ipn.gw, conn, init_fn, ethernet_input);
    }
    return myif;
}

void ICACHE_FLASH_ATTR eagle_lwip_if_free(struct ieee80211_conn *conn)
{
	if(conn->dhcps_if == 0) {
		netif_remove(conn->myif);
//		if(lwip_if_queues[0] != NULL)
			vPortFree(lwip_if_queues[0]);
	}
	else {
		if(dhcps_flag) dhcps_stop();
		netif_remove(conn->myif);
//		if(lwip_if_queues[1] != NULL)
			vPortFree(lwip_if_queues[1]);
	}
	if(conn->myif != NULL) {
		vPortFree(conn->myif);
		conn->myif = NULL;
	}
}

#endif
