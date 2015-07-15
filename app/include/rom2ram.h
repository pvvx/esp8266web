/******************************************************************************
 * FileName: rom2ram.h
 * Description: Alternate SDK (libmain.a)
 * Author: PV`
 * (c) PV` 2015
*******************************************************************************/

#ifndef _INCLUDE_ROM2RAM_H_
#define _INCLUDE_ROM2RAM_H_

void copy_s4d1(unsigned char * pd, void * ps, unsigned int len);
void copy_s1d4(void * pd, unsigned char * ps, unsigned int len);
unsigned int rom_strlen(void * ps);
char * rom_strcpy(char * pd_, void * ps, unsigned int maxlen);
unsigned int rom_xstrcpy(char * pd, void * ps);
int rom_cpy_label(char * pd, void * ps);

int iram_buf_init(void);
bool eRamRead(uint32 addr, uint8 *pd, uint32 len);
bool eRamWrite(uint32 addr, uint8 *ps, uint32 len);

char * rom_strchr(const char * ps, char c);

typedef struct t_eraminfo // описание свободной области в iram
{
	uint32 *base;
	int32 size;
}ERAMInfo;

extern ERAMInfo eraminfo;

//const char *rom_strchr(const char * ps, char c);
//char * ets_strrchr(const char *string, int c);

#endif /* _INCLUDE_ROM2RAM_H_ */
