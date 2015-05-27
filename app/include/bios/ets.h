/******************************************************************************
 * FileName: ets_bios.h
 * Description: funcs in ROM-BIOS
 * Alternate SDK ver 0.0.0 (b0)
 * Author: PV`
 * (c) PV` 2015
*******************************************************************************/
#ifndef _INCLUDE_BIOS_ETS_SYS_H_
#define _INCLUDE_BIOS_ETS_SYS_H_

#include "c_types.h"
#include "ets_sys.h"

/* This copies characters from the string from (up to and including the terminating null character) into the string
  to. Like memcpy, this function has undefined results if the strings overlap. The return value is the value of to. */
//char * ets_strcpy(char *restrict to, const char *restrict from); /* копируемые области не должны пересекаться */
char *ets_strcpy(char *to, const char *from); /* копируемые области не должны пересекаться */
/* This function is similar to strcpy but always copies exactly size characters into to.
 If the length of from is more than size, then strncpy copies just the first size characters. Note that in this case
 there is no null terminator written into to.
 If the length of from is less than size, then strncpy copies all of from, followed by enough null characters to add
 up to size characters in all. This behavior is rarely useful, but it is specified by the ISO C standard.
 The behavior of strncpy is undefined if the strings overlap.
 Using strncpy as opposed to strcpy is a way to avoid bugs relating to writing past the end of the allocated
 space for to. However, it can also make your program much slower in one common case: copying a string
 which is probably small into a potentially large buffer. In this case, size may be large, and when it is, strncpy will
 waste a considerable amount of time copying null characters. */
//char *ets_strncpy(char *restrict to, const char *restrict from, size_t size);
char *ets_strncpy(char *to, const char *from, size_t size); /* копируемые области не должны пересекаться */
/* The strlen function returns the length of the null-terminated string
 s in bytes. (In other words, it returns the offset of the terminating
 null character within the array.) */
size_t ets_strlen(const char *s);
/* The strcmp function compares the string s1 against s2, returning a value that has the same sign as the difference
 between the first differing pair of characters (interpreted as unsigned char objects, then promoted to int).
 If the two strings are equal, strcmp returns 0.
 A consequence of the ordering used by strcmp is that if s1 is an initial substring of s2, then s1 is considered to be
 ``less than'' s2.
 strcmp does not take sorting conventions of the language the strings are written in into account. To get that one
 has to use strcoll. */
int ets_strcmp(const char *s1, const char *s2);
/* This function is the similar to strcmp, except that no more than size
 characters are compared. In other words, if the two strings are the
 same in their first size characters, the return value is zero. */
int ets_strncmp(const char *s1, const char *s2, size_t size);
/* This is like strchr, except that it searches haystack for a substring needle rather than just a single character. It
 returns a pointer into the string haystack that is the first character of the substring, or a null pointer if no match
 was found. If needle is an empty string, the function returns haystack. */
char *ets_strstr(const char *haystack, const char *needle);
/* The function memcmp compares the size bytes of memory beginning at a1 against the size bytes of memory
 beginning at a2. The value returned has the same sign as the difference between the first differing pair of bytes
 (interpreted as unsigned char objects, then promoted to int).
 If the contents of the two blocks are equal, memcmp returns 0. */
int ets_memcmp(const void *a1, const void *a2, size_t size);
/* The memcpy function copies size bytes from the object beginning at from into the object beginning at to. The
 behavior of this function is undefined if the two arrays to and from overlap; use memmove instead if overlapping
 is possible.
The value returned by memcpy is the value of to. */
//void *ets_memcpy(void *restrict to, const void *restrict from, size_t size);
void *ets_memcpy(void *to, const void *from, size_t size); /* копируемые области не должны пересекаться */
/* memmove copies the size bytes at from into the size bytes at to, even if those two blocks of space overlap. In the
 case of overlap, memmove is careful to copy the original values of the bytes in the block at from, including those
 bytes which also belong to the block at to.
 The value returned by memmove is the value of to. */
void *ets_memmove(void *to, const void *from, size_t size);
/* This function copies the value of c (converted to an unsigned char) into each of the first size bytes of the object
 beginning at block. It returns the value of block. */
void *ets_memset(void *block, int c, size_t size);
void *ets_bzero(void *block, size_t size); // ets_memset(block, 0, size);

void ets_install_putc1(void *routine); // { putc1_routine = routine; }
void ets_install_putc2(void *routine); // { putc2_routine = routine; }
void ets_install_uart_printf(void); // { ets_install_putc1(_putc1); }
// void ets_install_external_printf();
uint32_t est_get_printf_buf_remain_len(void); // { return printf_buf_len; }
void est_reset_printf_buf_len(void); // { printf_buf_len = 0; }
void _putc2_def(char ch); // { *printf_pbuf++ = ch; printf_buf_len-- } // printf_buf_len 0x3FFFDD58, printf_pbuf 0x3FFFDD5C
void ets_putc(char ch); // { uart_tx_one_char(ch) }
void ets_getc(char *ch); // { *ch = uart_rx_one_char_block(); }
void ets_write_char(char ch); // { if(putc1_routine != NULL) putc1_routine(ch); else if(putc2_routine != NULL) putc2_routine(ch); }

void ets_update_cpu_frequency(uint32_t clk_mhz); // { cpu_clk_mhz = clk_mhz; }
uint32_t ets_get_cpu_frequency(void); // { return cpu_clk_mhz; }
void ets_set_user_start(void *routine); // { user_start_proc = routine; } user_start_proc 0x3FFFDCD0
// idle_cb(idle_arg) вызывает ets_run()
void ets_set_idle_cb(void *routine, void *arg); // { idle_cb = routine; idle_arg = opt } // idle_cb 0x3FFFDAB0, idle_arg 0x3FFFDAB4
void ets_run(void);
void ets_delay_us(uint32 us); /*{
	uint32 x = xthal_get_ccount();
	uint32 y = cpu_clk_mhz * us;
	while(x - xthal_get_ccount() > y); }*/
typedef char * va_list;
/* Print formatted data from variable argument list to stdout
 Writes the C string pointed by format to the standard output (stdout), replacing any format specifier in the same
 way as printf does, but using the elements in the variable argument list identified by arg instead of additional
 function arguments.
 Internally, the function retrieves arguments from the list identified by arg as if va_arg was used on it, and thus
 the state of arg is likely altered by the call.
 In any case, arg should have been initialized by va_start at some point before the call, and it is expected to be
 released by va_end at some point after the call. */
int ets_vprintf(void *routine, const char *format, va_list arg); // Print formatted data from variable argument list to stdout
/*void form_printf(const char * format, ... )
{
  va_list args;
  va_start (args, format);
  vprintf (format, args);
  va_end (args);
}*/
/* Writes the C string pointed by format to the standard output (stdout). If format includes format specifiers
(subsequences beginning with %), the additional arguments following format are formatted and inserted in the
resulting string replacing their respective specifiers. */
int ets_printf(const char *format, ...); // Print formatted data to stdout
int ets_uart_printf(const char *format, ...); // Print formatted data to uart (_putc1())
//void ets_external_printf(void *routine, void *r3, void *r4); // { putc1_external=r3; if(r3!=NULL) ets_install_putc2(r3); else ets_install_putc2(_putc2_def); putc2_external=a4; }
int ets_external_printf(const char *format, ...); // Print formatted data to eprintf_buf (_putc2_def())
void eprintf_init_buf(void * x); // struct x {uint16 size; uint8 * ptr; };
int eprintf(const char *format, ...); // Print formatted data to eprintf_buf
int eprintf_to_host(void); // eprintf_buf sip_send()
void ets_rtc_int_register(void); //
/* eagle.rom.addr.v6.ld
 PROVIDE ( ets_get_cpu_frequency = 0x40002f0c );
 PROVIDE ( ets_update_cpu_frequency = 0x40002f04 );
 PROVIDE ( cpu_clk_mhz = 0x3FFFC704);
 PROVIDE ( ets_set_user_start = 0x40000fbc );
 PROVIDE ( user_start_proc = 0x3FFFDCD0 );
 PROVIDE ( ets_set_idle_cb = 0x40000dc0 );

 PROVIDE ( ets_delay_us = 0x40002ecc );

 PROVIDE ( ets_run = 0x40000e04 );

 PROVIDE ( ets_vprintf = 0x40001f00 );
 PROVIDE ( ets_printf = 0x400024cc );

 PROVIDE ( eprintf = 0x40001d14 );
 PROVIDE ( eprintf_init_buf = 0x40001cb8 );
 PROVIDE ( eprintf_to_host = 0x40001d48 );
 PROVIDE ( est_get_printf_buf_remain_len = 0x40002494 );
 PROVIDE ( est_reset_printf_buf_len = 0x4000249c );
 PROVIDE ( ets_uart_printf = 0x40002544 );
 PROVIDE ( ets_external_printf = 0x40002578 );
PROVIDE ( ets_str2macaddr = 0x40002af8 );
PROVIDE ( ets_char2xdigit = 0x40002b74 );

PROVIDE ( ets_install_external_printf = 0x40002450 );
 PROVIDE ( ets_install_putc1 = 0x4000242c );
 PROVIDE ( ets_install_putc2 = 0x4000248c );
 PROVIDE ( ets_install_uart_printf = 0x40002438 );
 PROVIDE ( ets_getc = 0x40002bcc );
 PROVIDE ( ets_putc = 0x40002be8 );
 PROVIDE ( _putc2_def = 0x400024A8);
 PROVIDE ( ets_write_char = 0x40001da0 );

 PROVIDE ( ets_intr_lock = 0x40000f74 );
 PROVIDE ( ets_intr_unlock = 0x40000f80 );
 PROVIDE ( ets_isr_attach = 0x40000f88 );
 PROVIDE ( ets_isr_mask = 0x40000f98 );
 PROVIDE ( ets_isr_unmask = 0x40000fa8 );

 PROVIDE ( ets_memcmp = 0x400018d4 );
 PROVIDE ( ets_memcpy = 0x400018b4 );
 PROVIDE ( ets_memmove = 0x400018c4 );
 PROVIDE ( ets_memset = 0x400018a4 );
 PROVIDE ( ets_bzero = 0x40002ae8 );

 PROVIDE ( ets_strcmp = 0x40002aa8 );
 PROVIDE ( ets_strcpy = 0x40002a88 );
 PROVIDE ( ets_strlen = 0x40002ac8 );
 PROVIDE ( ets_strncmp = 0x40002ab8 );
 PROVIDE ( ets_strncpy = 0x40002a98 );
 PROVIDE ( ets_strstr = 0x40002ad8 );

 PROVIDE ( ets_task = 0x40000dd0 );
 PROVIDE ( ets_post = 0x40000e24 );

 PROVIDE ( ets_timer_arm = 0x40002cc4 );
 PROVIDE ( ets_timer_disarm = 0x40002d40 );
 PROVIDE ( ets_timer_done = 0x40002d80 );
 PROVIDE ( ets_timer_handler_isr = 0x40002da8 );
 PROVIDE ( ets_timer_init = 0x40002e68 );
 PROVIDE ( ets_timer_setfn = 0x40002c48 );
 PROVIDE ( timer_insert = 0x40002c64 );

 PROVIDE ( ets_wdt_disable = 0x400030f0 );
 PROVIDE ( ets_wdt_enable = 0x40002fa0 );
 PROVIDE ( ets_wdt_get_mode = 0x40002f34 );
 PROVIDE ( ets_wdt_init = 0x40003170 );
 PROVIDE ( ets_wdt_restore = 0x40003158 );

 PROVIDE ( wdt_info = 0x3FFFC708 };
*/
#ifndef _ETS_SYS_H
typedef uint32_t ETSSignal;
typedef uint32_t ETSParam;

typedef struct ETSEventTag ETSEvent;

struct ETSEventTag {
    ETSSignal sig;
    ETSParam  par;
};

typedef void (*ETSTask)(ETSEvent *e);
#endif

typedef uint8_t ETSPriority;

bool ets_post(uint32_t prio, ETSSignal sig, ETSParam par);
void ets_task(ETSTask task, uint32_t prio, ETSEvent * queue, uint8 qlen);

void ets_isr_attach(uint32_t, void*, void*);
void ets_isr_mask(uint32_t);
void ets_isr_unmask(uint32_t);

void ets_intr_lock();
void ets_intr_unlock();

#ifndef _ETS_SYS_H
/* timer related */
typedef uint32_t ETSHandle;
typedef void ETSTimerFunc(void *timer_arg);

typedef struct _ETSTIMER_ {
    struct _ETSTIMER_    *timer_next;
    uint32_t              timer_expire;
    uint32_t              timer_period;
    ETSTimerFunc         *timer_func;
    void                 *timer_arg;
} ETSTimer;
#endif

void ets_timer_handler_isr(void);
void ets_timer_arm(ETSTimer *ptimer, uint32_t us_ms, bool repeat_flag);
void timer_insert(uint32 tim_count, ETSTimer *ptimer);
void ets_timer_disarm(ETSTimer *ptimer);
void ets_timer_setfn(ETSTimer *ptimer, ETSTimerFunc *pfunction, void *parg);
void ets_timer_init(void);
void ets_timer_done(ETSTimer *ptimer);

struct swdt_info{
	int wdt_mode; // 0xFFFFFFFF
	uint32 a; 	  // 0x06 // ~ равно периоду в WDT_REG1
	uint32 b;	  // 0x0C // ~ равно периоду в WDT_REG2
};
// RAM_BIOS:3FFFC708
extern struct swdt_info wdt_info;
uint32_t ets_wdt_get_mode(void); // { return wdt_mode; } // [0x3FFFC708] wdt_mode, [0x3FFFC70C] a, [3FFFC710] b
void ets_wdt_enable(int mode, uint32_t a, uint32_t b);
int ets_wdt_disable(void); // return wdt_mode
void ets_wdt_init(void);
void ets_wdt_restore(int mode); // if(flg) ets_wdt_enable(mode, wdt_info.a, wdt_info.b);

void software_reset(void); // [0x60000700] |= 0x80000000;

// RAM_BIOS: 0x3fffc704
extern uint8 cpu_clk_mhz;

#endif /* _INCLUDE_BIOS_ETS_SYS_H_ */
