#ifndef __INT_TIME_US__
#define __INT_TIME_US__

void int_us_init(uint32 us) ICACHE_FLASH_ATTR;
void int_us_disable(void) ICACHE_FLASH_ATTR;

void set_new_time_int_us(uint32 us) ICACHE_FLASH_ATTR;

#endif

