/******************************************************************************
 * FileName: rom2ram.h
 * Description: Alternate SDK (libmain.a)
 * Author: PV`
 * (c) PV` 2015
*******************************************************************************/

#ifndef _INCLUDE_ROM2RAM_H_
#define _INCLUDE_ROM2RAM_H_

/* Копирует данные из области align(4) (flash, iram, registers) в область align(1) (ram) */
int copy_s4d1(unsigned char * pd, void * ps, unsigned int len);
/* Копирует данные из области align(1) (ram) в область align(4) (flash, iram, registers) */
int copy_s1d4(void * pd, unsigned char * ps, unsigned int len);
/* strlen() для строк в сегментах flash и IRAM */
unsigned int rom_strlen(const char * ps);
/* strcpy() из сегментов flash и IRAM */
char * rom_strcpy(char * pd_, void * ps, unsigned int maxlen);
/* xstrcpy() из сегментов flash и IRAM с возвратом размера строки:
   на выходе размер строки, без учета терминатора '\0' */
unsigned int rom_xstrcpy(char * pd, const char * ps);
/* сравнение строки в ram со строкой в сегменте flash и IRAM
  = 1 если шаблон совпадает */
int rom_xstrcmp(char * pd, const char * ps);
/* Чтение буфера в IRAM */
bool eRamRead(uint32 addr, uint8 *pd, uint32 len);
/* Запись буфера в IRAM */
bool eRamWrite(uint32 addr, uint8 *ps, uint32 len);
/* strchr() поиск символа в строках находящихся в сегментах flash и IRAM  */
char * rom_strchr(const char * ps, char c);

#define read_align4_chr(a) (*((*((unsigned int *)((unsigned int)a & (~3))))>>(((unsigned int)a & 3) << 3)))

char get_align4_chr(const char *ps);
void write_align4_chr(unsigned char *pd, unsigned char c);

//const char *rom_strchr(const char * ps, char c);
//char * ets_strrchr(const char *string, int c);

typedef struct t_eraminfo // описание свободной области в iram
{
	uint32 *base;	// адрес начала буфера
	int32 size; 	// размер в байтах
}ERAMInfo;

/* описание свободной области (буфера) в iram */
extern ERAMInfo eraminfo;
/* Определение размера свободного буфера в IRAM и очистка BSS IRAM (сегмент DATA_IRAM_ATTR) */
int iram_buf_init(void);

#endif /* _INCLUDE_ROM2RAM_H_ */
