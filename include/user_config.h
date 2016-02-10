#ifndef _user_config_h_
#define _user_config_h_

#include "sdk/sdk_config.h"

#define SYS_VERSION "0.5.5"

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#define WEB_DEFAULT_SOFTAP_IP	DEFAULT_SOFTAP_IP // ip 192.168.4.1
#define WEB_DEFAULT_SOFTAP_MASK	DEFAULT_SOFTAP_MASK // mask 255.255.255.0
#define WEB_DEFAULT_SOFTAP_GW	DEFAULT_SOFTAP_IP // gw 192.168.4.1

#define WEB_DEFAULT_STATION_IP    0x3201A8C0 // ip 192.168.1.50
#define WEB_DEFAULT_STATION_MASK  0x00FFFFFF // mask 255.255.255.0
#define WEB_DEFAULT_STATION_GW    0x0101A8C0 // gw 192.168.1.1

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#define USE_WEB		80 // включить в трансялцию порт Web, если =0 - по умолчанию выключен
#define WEBSOCKET_ENA  // включить WEBSOCKET

//#define USE_TCP2UART 12345 // включить в трансялцию драйвер TCP2UART, номер порта по умолчанию (=0 - отключен)

#if DEBUGSOO > 0
#ifndef USE_TCP2UART
// #define USE_GDBSTUB // UDK пока не поддерживает GDB
#endif
//#define UDP_TEST_PORT	1025 // включить в трансялцию контрольный порт UDP, (=0 - отключен)
#endif

#define USE_CPU_SPEED  160 // установить частоту CPU по умолчанию 80 или 160 MHz

//#define USE_NETBIOS	1 // включить в трансялцию драйвер NETBIOS, если =0 - по умолчанию выключен.

#define USE_SNTP	1 // включить в трансялцию драйвер SNTP, если =0 - по умолчанию выключен, = 1 - по умолчанию включен.

#ifndef USE_TCP2UART
//#define USE_RS485DRV	// использовать RS-485 драйвер
//#define MDB_RS485_MASTER // Modbus RTU RS-485 master & slave
#endif
//#define USE_MODBUS	502 // включить в трансялцию Modbus TCP, если =0 - по умолчанию выключен
//#define MDB_ID_ESP 50 // номер устройства ESP на шине modbus

//#define USE_WDRV 	10201 // включить в трансялцию вывод Wave ADC по UDP, номер порта управления по умолчанию (=0 - отключен)

//#define USE_CAPTDNS	0	// включить в трансялцию NDS отвечающий на всё запросы клиента при соединении к AP модуля
						// указанием на данный WebHttp (http://aesp8266/), если =0 - по умолчанию выключен

//#define USE_GPIOs_intr // включение примера с счетчиками срабатывания прерываний на GPOIs (использует ~sys_ucnst_1~ и ~sys_ucnst_2~ для сохранения в flash)
						// смотреть переменные ~count1~, ~count2~, ~sys_ucnst_1~, ~sys_ucnst_2~  и их запись...
						// файлы: \app\web\gpios_intr.c \app\include\gpios_intr.h

#endif // _user_config_h_


