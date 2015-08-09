/*
 * FileName: pplibadd.c
 * Description: Alternate SDK (libpp.a)
 * (c) PV` 2015
 */
#include "c_types.h"
#include "user_config.h"

#if DEF_SDK_VERSION >= 1300
/* bit_popcount() используется из SDK libpp.a: if_hwctrl.o и trc.o */
uint32 ICACHE_FLASH_ATTR bit_popcount(uint32 x)
{
	uint32 ret = 0;
	while(x) {
		ret += x & 1;
		x >>= 1;
	}
	return ret;
}
#endif
