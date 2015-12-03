#ifndef _user_config_h_
#define _user_config_h_

#include "sdk/sdk_config.h"

#define SYS_VERSION "0.5.4"

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#define USE_WEB		80 // включить в трансялцию порт Web, если =0 - по умолчанию выключен

#define USE_TCP2UART 12345 // включить в трансялцию драйвер TCP2UART, номер порта по умолчанию (=0 - отключен)

#if DEBUGSOO > 0
#ifndef USE_TCP2UART
// #define USE_GDBSTUB // UDK пока не поддерживает GDB
#endif
#define UDP_TEST_PORT	1025 // включить в трансялцию контрольный порт UDP, (=0 - отключен)
#endif

#define USE_CPU_SPEED  160 // установить частоту CPU по умолчанию 80 или 160 MHz

#define USE_NETBIOS	1 // включить в трансялцию драйвер NETBIOS, если =0 - по умолчанию выключен.

#define USE_SNTP	1 // включить в трансялцию драйвер SNTP, если =0 - по умолчанию выключен, = 1 - по умолчанию включен.

#define USE_MODBUS	502 // включить в трансялцию Modbus TCP, если =0 - по умолчанию выключен

#define USE_WDRV 	10201 // включить в трансялцию вывод Wave ADC по UDP, номер порта управления по умолчанию (=0 - отключен)

#define USE_CAPTDNS	0	// включить в трансялцию NDS отвечающий на всё запросы клиента при соединении к AP модуля
						// указанием на данный WebHttp (http://aesp8266/), если =0 - по умолчанию выключен

#endif // _user_config_h_


