 /******************************************************************************
 * FileName: web_utils.h
 * Alternate SDK 
 * Author: PV`
 * (c) PV` 2015
*******************************************************************************/

#ifndef _INCLUDE_WEB_UTILS_H_
#define _INCLUDE_WEB_UTILS_H_

int rom_atoi(const char *s) ICACHE_FLASH_ATTR;
void copy_align4(void *ptrd, void *ptrs, uint32 len);
uint32 hextoul(uint8 *s) ICACHE_FLASH_ATTR;
uint32 ahextoul(uint8 *s) ICACHE_FLASH_ATTR;
uint8 * cmpcpystr(uint8 *pbuf, uint8 *pstr, uint8 a, uint8 b, uint16 len) ICACHE_FLASH_ATTR;
uint8 * web_strnstr(const uint8* buffer, const uint8* token, int n) ICACHE_FLASH_ATTR;
bool base64decode(const uint8 *in, int len, uint8_t *out, int *outlen) ICACHE_FLASH_ATTR;
size_t base64encode(char* target, size_t target_len, const char* source, size_t source_len) ICACHE_FLASH_ATTR;
void strtomac(uint8 *s, uint8 *macaddr) ICACHE_FLASH_ATTR;
//uint32 strtoip(uint8 *s) ICACHE_FLASH_ATTR; // ipaddr_addr();
int urldecode(uint8 *d, uint8 *s, uint16 lend, uint16 lens) ICACHE_FLASH_ATTR;
//int urlencode(uint8 *d, uint8 *s, uint16 lend, uint16 lens) ICACHE_FLASH_ATTR;
int htmlcode(uint8 *d, uint8 *s, uint16 lend, uint16 lens) ICACHE_FLASH_ATTR;
void print_hex_dump(uint8 *buf, uint32 len, uint8 k) ICACHE_FLASH_ATTR;
// char* str_to_upper_case(char* text) ICACHE_FLASH_ATTR;
uint32 str_array(uint8 *s, uint32 *buf, uint32 max_buf) ICACHE_FLASH_ATTR;
uint32 str_array_w(uint8 *s, uint16 *buf, uint32 max_buf) ICACHE_FLASH_ATTR;
uint32 str_array_b(uint8 *s, uint8 *buf, uint32 max_buf) ICACHE_FLASH_ATTR;
char* word_to_lower_case(char* text) ICACHE_FLASH_ATTR;

#endif /* _INCLUDE_WEB_UTILS_H_ */
