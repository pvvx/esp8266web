/*
 * FileName: rom2ram.c
 * Description: Alternate SDK (main.a)
 * (c) pvvx 2015
 */

#include "user_config.h"
#include "bios.h"
#include "hw/esp8266.h"
#include "rom2ram.h"

extern char * _text_start; // start addr IRAM 

#ifndef ICACHE_RAM_ATTR
#define ICACHE_RAM_ATTR
#endif
#ifndef ICACHE_FLASH_ATTR
#define ICACHE_FLASH_ATTR
#endif

int ICACHE_FLASH_ATTR iram_buf_init(void)
{
	 eraminfo.size = (uint32)(&_text_start) + ((((DPORT_BASE[9]>>3)&3)==3)? 0x08000 : 0x0C000) - (int)eraminfo.base;
	 ets_memset(eraminfo.base, 0, eraminfo.size);
	 return eraminfo.size;
}


void ICACHE_RAM_ATTR copy_s4d1(unsigned char * pd, void * ps, unsigned int len)
{
	union {
		unsigned char uc[4];
		unsigned int ud;
	}tmp;
	unsigned int *p = (unsigned int *)((unsigned int)ps & (~3));

	unsigned int xlen = (unsigned int)ps & 3;
	if(xlen) {
		tmp.ud = *p++;
		while (len)  {
			len--;
			*pd++ = tmp.uc[xlen++];
			if(xlen & 4) break;
		}
	}
	xlen = len >> 2;
	while(xlen) {
		tmp.ud = *p++;
		*pd++ = tmp.uc[0];
		*pd++ = tmp.uc[1];
		*pd++ = tmp.uc[2];
		*pd++ = tmp.uc[3];
		xlen--;
	}
	if(len & 3) {
		tmp.ud = *p;
		pd[0] = tmp.uc[0];
		if(len & 2) {
			pd[1] = tmp.uc[1];
			if(len & 1) {
				pd[2] = tmp.uc[2];
			}
		}
	}
}


void ICACHE_RAM_ATTR copy_s1d4(void * pd, unsigned char * ps, unsigned int len)
{
	union {
		unsigned char uc[4];
		unsigned int ud;
	}tmp;
	unsigned int *p = (unsigned int *)(((unsigned int)pd) & (~3));
	unsigned int xlen = (unsigned int)pd & 3;
	if(xlen) {
		tmp.ud = *p;
		while (len)  {
			len--;
			tmp.uc[xlen++] = *ps++;
			if(xlen & 4) break;
		}
		*p++ = tmp.ud;
	}
	xlen = len >> 2;
	while(xlen) {
		tmp.uc[0] = *ps++;
		tmp.uc[1] = *ps++;
		tmp.uc[2] = *ps++;
		tmp.uc[3] = *ps++;
		*p++ = tmp.ud;
		xlen--;
	}
	if(len & 3) {
		tmp.ud = *p;
		tmp.uc[0] = ps[0];
		if(len & 2) {
			tmp.uc[1] = ps[1];
			if(len & 1) {
				tmp.uc[2] = ps[2];
			}
		}
		*p = tmp.ud;
	}
}

//extern void copy_s4d1(uint8 * pd, void * ps, uint32 len);

bool ICACHE_RAM_ATTR eRamRead(uint32 addr, uint8 *pd, uint32 len)
{
	if (addr + len > eraminfo.size) return false;
	copy_s4d1(pd, (void *)((uint32)eraminfo.base + addr), len);
	return true;
}

//extern void copy_s1d4(void * pd, uint8 * ps, uint32 len);

bool ICACHE_RAM_ATTR eRamWrite(uint32 addr, uint8 *ps, uint32 len)
{
	if (addr + len > eraminfo.size) return false;
	copy_s1d4((void *)((uint32)eraminfo.base + addr), ps, len);
	return true;
}


unsigned int ICACHE_RAM_ATTR rom_strlen(void * ps)
{
	union {
		unsigned char uc[4];
		unsigned int ud;
	}tmp;
	if(ps == 0) return (0);
	unsigned int len = 0;
	unsigned int *p = (unsigned int *)((unsigned int)ps & (~3));
	unsigned int xlen = (unsigned int)ps & 3;
	while(1) {
		tmp.ud = *p++;
		do {
			if(tmp.uc[xlen++] == 0) return len;
			len++;
			xlen &= 3;
		} while(xlen);
	}
}

char * ICACHE_RAM_ATTR rom_strcpy(char * pd_, void * ps, unsigned int maxlen)
{
	if(pd_ == (0) || ps == (0) || maxlen == 0) return (0);
	union {
		unsigned char uc[4];
		unsigned int ud;
	}tmp;
	char * pd = pd_;
	unsigned int *p = (unsigned int *)((unsigned int)ps & (~3));
	*pd = 0;
	char c;
	unsigned int xlen = (unsigned int)ps & 3;
	if(xlen) {
		tmp.ud = *p++;
		while (maxlen)  {
			maxlen--;
			c = *pd++ = tmp.uc[xlen++];
			if(c == 0) return pd_;
			if(xlen & 4) break;
		}
	}
	xlen = maxlen >> 2;
	while(xlen) {
		tmp.ud = *p++;
		c = *pd++ = tmp.uc[0];
		if(c == 0) return pd_;
		c = *pd++ = tmp.uc[1];
		if(c == 0) return pd_;
		c = *pd++ = tmp.uc[2];
		if(c == 0) return pd_;
		c = *pd++ = tmp.uc[3];
		if(c == 0) return pd_;
		xlen--;
	}
	if(maxlen & 3) {
		tmp.ud = *p;
		c = pd[0] = tmp.uc[0];
		if(c == 0) return pd_;
		if(maxlen & 2) {
			c = pd[1] = tmp.uc[1];
			if(c == 0) return pd_;
			if(maxlen & 1) {
				c = pd[2] = tmp.uc[2];
				if(c == 0) return pd_;
			}
		}
	}
	return pd_;
}

unsigned int ICACHE_RAM_ATTR rom_xstrcpy(char * pd, void * ps)
{
	union {
		unsigned char uc[4];
		unsigned int ud;
	}tmp;
	if(ps == 0 || pd == 0) return (0);
	*pd = 0;
	unsigned int len = 0;
	unsigned int *p = (unsigned int *)((unsigned int)ps & (~3));
	unsigned int xlen = (unsigned int)ps & 3;
	while(1) {
		tmp.ud = *p++;
		do {
			if((*pd++ = tmp.uc[xlen++]) == 0) return len;
			len++;
			xlen &= 3;
		} while(xlen);
	}
}

int ICACHE_RAM_ATTR rom_cpy_label(char * pd, void * ps)
{
	union {
		unsigned char uc[4];
		unsigned int ud;
	}tmp;
	if(ps == 0 || pd == 0) return 0;
	unsigned int *p = (unsigned int *)((unsigned int)ps & (~3));
	unsigned int xlen = (unsigned int)ps & 3;
	while(1) {
		tmp.ud = *p++;
		do {
			if(tmp.uc[xlen] == 0) return 1;
			if(tmp.uc[xlen++] != *pd || *pd++ == 0) return 0;
			xlen &= 3;
		} while(xlen);
	}
}


#if 0

char ICACHE_RAM_ATTR get_rom_chr(const char *ps)
{
	return (*((unsigned int *)((unsigned int)ps & (~3))))>>(((unsigned int)ps & 3) << 3);
}


/*
Name: strchr
Prototype: char * strchr (const char *string, int c)
Description:
The strchr function finds the first occurrence of the character c (converted to a char) in the null-terminated string
 beginning at string. The return value is a pointer to the located character, or a null pointer if no match was
 found.

For example,
         strchr ("hello, world", 'l')
              "llo, world"
         strchr ("hello, world", '?')
              NULL


The terminating null character is considered to be part of the string, so you can use this function get a pointer to
 the end of a string by specifying a null character as the value of the c argument. It would be better (but less
 portable) to use strchrnul in this case, though. */
const char * ICACHE_RAM_ATTR rom_strchr(const char * ps, char c)
{
	union {
		unsigned char uc[4];
		unsigned int ud;
	}tmp;
	if(ps == 0) return (0);
	unsigned int *p = (unsigned int *)((unsigned int)ps & (~3));
	unsigned int xlen = (unsigned int)ps & 3;
	while(1) {
		tmp.ud = *p;
		do {
			if(tmp.uc[xlen] == c) return (const char *)((unsigned int)p + xlen);
			else if(tmp.uc[xlen] == 0) return (0);
			xlen++;
		} while((xlen & 4) == 0);
		xlen = 0;
		p++;
	}
}

int ICACHE_RAM_ATTR rom_memcmp( void * ps, const char * pd_, unsigned int len)
{
	union {
		unsigned char uc[4];
		unsigned int ud;
	}tmp;
	char * pd = (void *) pd_;
	unsigned int *p = (unsigned int *)((unsigned int)ps & (~3));
	char c;

	unsigned int xlen = (unsigned int)ps & 3;
	if(xlen) {
		tmp.ud = *p++;
		while (len)  {
			len--;
			if(*pd++ != tmp.uc[xlen++]) return -1;
			if(xlen & 4) break;
		}
	}
	xlen = len >> 2;
	while(xlen) {
		tmp.ud = *p++;
		if(*pd++ != tmp.uc[0]) return -1;
		if(*pd++ != tmp.uc[1]) return -1;
		if(*pd++ != tmp.uc[2]) return -1;
		if(*pd++ != tmp.uc[3]) return -1;
		xlen--;
	}
	if(len & 3) {
		tmp.ud = *p;
		if(pd[0] != tmp.uc[0]) return -1;
		if(len & 2) {
			if(pd[1] != tmp.uc[1]) return -1;
			if(len & 1) {
				if(pd[2] != tmp.uc[2]) return -1;
			}
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
char * ICACHE_RAM_ATTR ets_strrchr(const char *string, int c)
{
	union {
		unsigned char uc[4];
		unsigned int ud;
	}tmp;
	char * pret = (0);
	if(string != 0) {
		unsigned int *p = (unsigned int *)((unsigned int)string & (~3));
		unsigned int xlen = (unsigned int)string & 3;
		while(1) {
			tmp.ud = *p;
			do {
				if(tmp.uc[xlen] == c) pret = (char *)((unsigned int)p + xlen);
				else if(tmp.uc[xlen] == 0) break;
				xlen++;
			} while((xlen & 4) == 0);
			xlen = 0;
			p++;
		}
	}
	return pret;
}

#endif

