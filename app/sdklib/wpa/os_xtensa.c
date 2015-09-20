/*
 * FileName: os_xtenca.c
 * Description: Alternate SDK (libwpa.a)
 * (c) PV` 2015
 */

#include "user_config.h"
#include "bios.h"

/*
char * ICACHE_FLASH_ATTR ets_strdup(char * s)
{
	char * pret = (0);
	if(s != (0)) {
		uint32 len = ets_strlen(s) + 1;
		pret = (char *)os_malloc(len);
		if(pret != (0)) ets_memcpy(pret, s, len);
	}
	return pret;
}
*/

uint32 ICACHE_FLASH_ATTR os_get_time(void)
{
	return 0;
}

extern uint32 phy_get_rand(void); // return *((uint32 *)(&g_phyFuns+0x290)) ^ *((uint32 *)0x3FF20E44);

uint32 ICACHE_FLASH_ATTR r_rand(void)
{
	return phy_get_rand();
}

uint32 ICACHE_FLASH_ATTR os_random(void)
{
	return phy_get_rand();
}

/* os_get_random - Get cryptographically strong pseudo random data :
 * Buffer for pseudo random data :
 * Length of the buffer Returns:
 * 0 on success, -1 on failure  */
int ICACHE_FLASH_ATTR os_get_random(unsigned char *  buf,  size_t  len)
{
	if(buf != NULL) {
		while(len) {
			uint32 z = phy_get_rand();
			do {
				*buf++ = (char)z;
				z >>= 8;
			} while(--len & 3);
		}
	}
	return 0;
}

/*
Name: strrchr
Prototype: char * strrchr (const char *string, int c)
Description:
The function strrchr is like strchr, except that it searches backwards
 from the end of the string string (instead of forwards from the
 front).
For example,
         strrchr ("hello, world", 'l')
              "ld"	*/
char * ICACHE_FLASH_ATTR ets_strrchr(const char *string, int c)
{
	char * pret = NULL;
	if(string != NULL) {
		char ch;
		do {
			ch = *string;
			if(ch == c) pret = (char *)string;
			string ++;
		} while(ch != 0);
	}
	return pret;
}
