/******************************************************************************
 * FileName: rom2ram.c
 * Description: Alternate SDK (libmain.a)
 * (c) PV` 2015
*******************************************************************************/
#include "user_config.h"
#include "bios.h"
#include "hw/esp8266.h"
#include "sdk/rom2ram.h"
#include "osapi.h"
#include "sdk/add_func.h"

#ifndef ICACHE_RAM_ATTR
#define ICACHE_RAM_ATTR
#endif
#ifndef ICACHE_FLASH_ATTR
#define ICACHE_FLASH_ATTR
#endif

extern uint32 _text_start[]; // start addr IRAM
extern uint32 _lit4_start[]; // addr data buf IRAM

/* Определение размера свободного буфера в IRAM и очистка BSS IRAM (сегмент DATA_IRAM_ATTR) */
int ICACHE_FLASH_ATTR iram_buf_init(void)
{
	 uint32 * end = &_text_start[((((DPORT_BASE[9]>>3)&3)==3)? (0x08000 >> 2) : (0x0C000 >> 2))];
	 eraminfo.size = (int32)((uint32)(end) - (uint32)eraminfo.base);
	 if(eraminfo.size > 0) {
		 uint32 * ptr = _lit4_start;
		 while(ptr < end) *ptr++ = 0;
	 }
#if DEBUGSOO > 0
	 else {
		 os_printf("No free IRAM!");
	 }
#endif	 
	 return eraminfo.size;
}

/* Копирует данные из области align(4) (flash, iram, registers) в область align(1) (ram) */
int ICACHE_RAM_ATTR copy_s4d1(unsigned char * pd, void * ps, unsigned int len)
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
	return 0;
}
/* Копирует данные из области align(1) (ram) в область align(4) (flash, iram, registers) */
int ICACHE_RAM_ATTR copy_s1d4(void * pd, unsigned char * ps, unsigned int len)
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
	return 0;
}
//extern void copy_s4d1(uint8 * pd, void * ps, uint32 len);
/* Чтение буфера в IRAM */
bool ICACHE_RAM_ATTR eRamRead(uint32 addr, uint8 *pd, uint32 len)
{
	if (addr + len > eraminfo.size) return false;
	copy_s4d1(pd, (void *)((uint32)eraminfo.base + addr), len);
	return true;
}

//extern void copy_s1d4(void * pd, uint8 * ps, uint32 len);
/* Запись буфера в IRAM */
bool ICACHE_RAM_ATTR eRamWrite(uint32 addr, uint8 *ps, uint32 len)
{
	if (addr + len > eraminfo.size) return false;
	copy_s1d4((void *)((uint32)eraminfo.base + addr), ps, len);
	return true;
}
/* strlen() для данных в сегментах flash и IRAM */
unsigned int ICACHE_RAM_ATTR rom_strlen(const char * ps)
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
/* strcpy() из сегментов flash и IRAM */
char * ICACHE_RAM_ATTR rom_strcpy(char * pd_, void * ps, unsigned int maxlen)
{
	if(pd_ == (0) || ps == (0) || maxlen == 0) return (0);
	union {
		unsigned char uc[4];
		unsigned int ud;
	}tmp;
	char * pd = pd_;
	unsigned int *p = (unsigned int *)((unsigned int)ps & (~3));
	unsigned int xlen = (unsigned int)ps & 3;
#if 1
	while(1) {
		tmp.ud = *p++;
		do {
			if(maxlen-- == 0) {
				*pd = 0;
				return pd_;
			}
			if((*pd++ = tmp.uc[xlen++]) == 0) return pd_;
			xlen &= 3;
		} while(xlen);
	}
#else
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
#endif
}
/* xstrcpy() из сегментов flash и IRAM с возвратом размера строки:
   на выходе размер строки, без учета терминатора '\0' */
unsigned int ICACHE_RAM_ATTR rom_xstrcpy(char * pd, const char * ps)
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
/* сравнение строки в ram со строкой в сегменте flash и IRAM
  = 1 если шаблон совпадает */
int ICACHE_RAM_ATTR rom_xstrcmp(char * pd, const char * ps)
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
char * ICACHE_RAM_ATTR rom_strchr(const char * ps, char c)
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
			if(tmp.uc[xlen] == c) return (char *)((unsigned int)p + xlen);
			else if(tmp.uc[xlen] == 0) return (0);
			xlen++;
		} while((xlen & 4) == 0);
		xlen = 0;
		p++;
	}
}

char ICACHE_RAM_ATTR get_align4_chr(const char *ps)
{
	return (*((unsigned int *)((unsigned int)ps & (~3))))>>(((unsigned int)ps & 3) << 3);
/*	asm(
		"movi.n	a3, -4"
		"and	a3, a2, a3"
		"l32i.n	a3, a3, 0"
		"extui	a2, a2, 0, 2"
		"slli	a2, a2, 3"
		"ssr	a2"
		"srl	a2, a3"
		"extui	a2, a2, 0, 8"
		"ret.n"
	); */
}

void ICACHE_RAM_ATTR write_align4_chr(unsigned char *pd, unsigned char c)
{
	union {
		unsigned char uc[4];
		unsigned int ud;
	}tmp;
	unsigned int *p = (unsigned int *)((unsigned int)pd & (~3));
	unsigned int xlen = (unsigned int)pd & 3;
	tmp.ud = *p;
	tmp.uc[xlen] = c;
	*p = tmp.ud;
}

#if 0


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

