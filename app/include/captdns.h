/*
 * captdns.h
 */

#ifndef _INCLUDE_CAPTDNS_H_
#define _INCLUDE_CAPTDNS_H_

#define CAPTDNS_PORT 53

bool captdns_init(void) ICACHE_FLASH_ATTR;
void captdns_close(void) ICACHE_FLASH_ATTR;

extern struct udp_pcb *pcb_cdns;
extern const char HostNameLocal[];
extern const char httpHostNameLocal[];

#endif /* _INCLUDE_CAPTDNS_H_ */
