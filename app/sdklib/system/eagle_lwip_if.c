/******************************************************************************
 * FileName: eagle_lwip_if.c (libmain.a)
 * Description: eagle_lwip_if for SDK 1.2.0...
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
extern void wifi_station_dhcpc_event(void);

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
    myif->hwaddr_len = 6; // +46 //+50 SDK 1.4.0
    myif->mtu = DEFAULT_MTU; //+44 1500 //+48 SDK 1.4.0
    myif->flags = NETIF_FLAG_IGMP | NETIF_FLAG_ETHARP | NETIF_FLAG_LINK_UP | NETIF_FLAG_BROADCAST; // = 0x0B2  +53 // +57 SDK 1.4.0
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

struct netif * ICACHE_FLASH_ATTR eagle_lwip_if_alloc(struct ieee80211_conn *conn, uint8 *macaddr, struct ip_info *ipinfo)
{
	struct netif *myif = conn->myif;
    if (myif == NULL) {
        myif = (void *) os_zalloc(sizeof(struct netif)); // SDK 1.1.2 : os_zalloc(64), SDK 1.4.0: os_zalloc(68)
        conn->myif = myif;
    }
    if(myif == NULL) return NULL;
    if (conn->dhcps_if == 0) { // +176 // SDK 1.4.0 +200
        if(default_hostname) {
    		wifi_station_set_default_hostname(macaddr);
    	}
    	myif->hostname = hostname; // +40 // +44 SDK 1.4.0
    }
    else myif->hostname = NULL; // +40 // +44 SDK 1.4.0

    myif->state = conn; // +28
    myif->name[0] = 'e'; // + 54 // SDK 1.4.0 +58
    myif->name[1] = 'w'; // + 55 // SDK 1.4.0 +59
    myif->output = etharp_output; // +20
    myif->linkoutput = ieee80211_output_pbuf; // +24
    ets_memcpy(myif->hwaddr, macaddr, 6); // +47 // SDK 1.4.0 +51

	ETSEvent *queue = (void *) os_zalloc(sizeof(struct ETSEventTag) * QUEUE_LEN); // os_zalloc(80)
	if(queue == NULL) return NULL;

    if (conn->dhcps_if != 0) { // +176 // +200 SDK 1.4.0
	    lwip_if_queues[1] = queue;
	    netif_set_addr(myif, &ipinfo->ip, &ipinfo->netmask, &ipinfo->gw);
	    ets_task(task_if1, LWIP_IF1_PRIO, (ETSEvent *)lwip_if_queues[1], QUEUE_LEN);
	    netif_add(myif, &ipinfo->ip, &ipinfo->netmask, &ipinfo->gw, conn, init_fn, ethernet_input);
	    if(dhcps_flag) {
	    	dhcps_start(ipinfo);
	    	os_printf("dhcp server start:(ip:" IPSTR ",mask:" IPSTR ",gw:" IPSTR ")\n", IP2STR(&ipinfo->ip), IP2STR(&ipinfo->netmask), IP2STR(&ipinfo->gw));
	    }
    }
    else {
#if DEF_SDK_VERSION >= 1400 // (SDK 1.4.0)
    	myif->dhcp_event = wifi_station_dhcpc_event; // +
#endif
    	lwip_if_queues[0] = queue;
	    ets_task(task_if0, LWIP_IF0_PRIO, (ETSEvent *)lwip_if_queues[0], QUEUE_LEN);
	    struct ip_info ipn;
		if(wifi_station_dhcpc_status()) {
		    ipn.ip.addr = 0;
		    ipn.netmask.addr = 0;
		    ipn.gw.addr = 0;
		}
		else {
			ipn =  *ipinfo;
		}
	    netif_add(myif, &ipn.ip, &ipn.netmask, &ipn.gw, conn, init_fn, ethernet_input);
    }
    return myif;
}

void ICACHE_FLASH_ATTR eagle_lwip_if_free(struct ieee80211_conn *conn)
{
	if(conn->dhcps_if == 0) { // SDK 1.4.0 +200
		netif_remove(conn->myif);
//		if(lwip_if_queues[0] != NULL)
		os_free(lwip_if_queues[0]);
	}
	else {
		if(dhcps_flag) dhcps_stop();
		netif_remove(conn->myif);
//		if(lwip_if_queues[1] != NULL)
		os_free(lwip_if_queues[1]);
	}
	if(conn->myif != NULL) {
		os_free(conn->myif);
		conn->myif = NULL;
	}
}

#endif
