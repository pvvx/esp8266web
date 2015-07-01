#ifndef __MEM_H__
#define __MEM_H__

// в модуле mem_manager.o

void *pvPortMalloc( size_t xWantedSize );
void vPortFree( void *pv );
void *pvPortZalloc(size_t size);
void *pvPortCalloc(unsigned int n, unsigned int count);
void *pvPortRealloc(void * p, size_t size);
size_t xPortGetFreeHeapSize(void);
void prvHeapInit(void) ICACHE_FLASH_ATTR ;

#define os_malloc   pvPortMalloc
#define os_free     vPortFree
#define os_zalloc   pvPortZalloc
#define os_calloc   pvPortCalloc
#define os_realloc  pvPortRealloc
#define system_get_free_heap_size xPortGetFreeHeapSize

#endif // __MEM_H__

