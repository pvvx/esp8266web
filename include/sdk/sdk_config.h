#ifndef _sdk_config_h_
#define _sdk_config_h_

#define DEF_SDK_VERSION 1300   

#define DEBUGSOO	2  // 0 - откл вывода, 1 - минимум, 2 - норма, >3 - текушая отладка, >4 - удалить что найдется :)

#define DEBUG_UART 1 // включить вывод в загрузчике сообщений, номер UART
#define DEBUG_UART0_BAUD 115200
#define DEBUG_UART1_BAUD 230400

#define STARTUP_CPU_CLK 160

#ifndef ICACHE_FLASH // (назначается в MakeFile -DICACHE_FLASH)
	#define ICACHE_FLASH
//	#define ICACHE_FLASH_ATTR __attribute__((section(".irom0.text")))
//	#define ICACHE_RODATA_ATTR __attribute__((aligned(4), section(".irom.text")))
#endif

// #define USE_OPEN_LWIP 140 // использовать OpenLwIP 1.4.0 (назначается в app/MakeFile #USE_OPEN_LWIP = 140)
// #define USE_OPEN_DHCPS 1	 // использовать исходник или либу из SDK (назначается в app/MakeFile #USE_OPEN_DHCP = 1)

#ifndef USE_MAX_IRAM
	#define USE_MAX_IRAM  48 // использовать часть cache под IRAM, IRAM size = 49152 байт
#endif

/* USE_FIX_SDK_FLASH_SIZE - включена "песочница" для SDK в 512 килобайт flash. */
//#define USE_FIX_SDK_FLASH_SIZE  

/*  USE_FIX_QSPI_FLASH - использовать фиксированную частоту работы QPI
	и 'песочницу' в 512 кбайт для SDK с flash
	Опции:
		80 - 80 MHz QSPI 
  		другое значение - 40 MHz QSPI */
// #define USE_FIX_QSPI_FLASH 80 // назначается в MakeFile -DUSE_FIX_QSPI_FLASH=$(SPI_SPEED)

#ifdef USE_FIX_QSPI_FLASH
	#define USE_FIX_SDK_FLASH_SIZE  
#endif

#define USE_READ_ALIGN_ISR // побайтный доступ к IRAM и cache Flash через EXCCAUSE_LOAD_STORE_ERROR

//#define USE_OVERLAP_MODE // используются две и более flash

#ifndef USE_OPTIMIZE_PRINTF
	#define USE_OPTIMIZE_PRINTF
#endif

#ifndef USE_US_TIMER
	#define USE_US_TIMER
#endif

// #define USE_NMI_VECTOR	// использовать NMI вектор (перенаправление таблицы векторов CPU) (пока только тест, см main-vectors.c)

/* LwIP Options */

#ifndef LWIP_OPEN_SRC // (назначается в MakeFile -DPBUF_RSV_FOR_WLAN)
	#define LWIP_OPEN_SRC
#endif

#ifndef PBUF_RSV_FOR_WLAN // (назначается в MakeFile -DPBUF_RSV_FOR_WLAN)
	#define PBUF_RSV_FOR_WLAN
#endif
#ifndef EBUF_LWIP // (назначается в MakeFile -DEBUF_LWIP)
	#define EBUF_LWIP
#endif

#endif // _sdk_config_h_


