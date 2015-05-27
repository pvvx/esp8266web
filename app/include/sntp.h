#ifndef __SNTP_H__
#define __SNTP_H__

#include "lwip/ip_addr.h"
#include <time.h>

bool sntp_init(void);
void sntp_close(void);
time_t get_sntp_time(void);

void sntp_send_request(ip_addr_t *server_addr) ICACHE_FLASH_ATTR;

#endif /* __SNTP_H__ */
