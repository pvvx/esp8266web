#ifndef __NETBIOS_H__
#define __NETBIOS_H__

#include "user_interface.h"

#ifdef USE_NETBIOS

/** default port number for "NetBIOS Name service */
#define NETBIOS_PORT     137

/** size of a NetBIOS name */
#define NETBIOS_NAME_LEN 16

extern uint8 netbios_name[NETBIOS_NAME_LEN + 1];

void netbios_init(void) ICACHE_FLASH_ATTR;
struct udp_pcb * ICACHE_FLASH_ATTR netbios_pcb(void) ICACHE_FLASH_ATTR;
void netbios_set_name(uint8 * name)  ICACHE_FLASH_ATTR;
bool netbios_off(void) ICACHE_FLASH_ATTR;

#endif

#endif /* __NETBIOS_H__ */
