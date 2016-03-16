/*
* Переназначения для работы ovl с библиотекой web
*/

#include "mdbtab.h"

// Переназначение для размещения процедур в IRAM
#undef ICACHE_FLASH_ATTR
#define ICACHE_FLASH_ATTR

// Переназначение для размещения переменных в RAM: rodata
#undef ICACHE_RODATA_ATTR
#define ICACHE_RODATA_ATTR

// Переназначение для размещения переменных в IRAM в другой секции
#undef DATA_IRAM_ATTR
#define DATA_IRAM_ATTR __attribute__((aligned(4), section(".iram.data")))

// #undef USE_OPTIMIZE_PRINTF

// Переназначение для размещения строк в RAM: rodata
#undef os_printf
#define os_printf __wrap_os_printf_plus

int ovl_init(int flg);
