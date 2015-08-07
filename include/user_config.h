#ifndef _user_config_h_
#define _user_config_h_

#include "sdk/sdk_config.h"

#define SYS_VERSION "0.4.0"

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#if DEBUGSOO > 0
	#define UDP_TEST_PORT		1025 // включить контрольный порт UDP
#endif

#define USE_CPU_SPEED  160 // установить частоту CPU по умолчанию 80 или 160 MHz

#define USE_NETBIOS	1 // включить драйвер NETBIOS

#define USE_SNTP	1 // включить драйвер SNTP

#define USE_SRV_WEB_PORT    	80 // порт Web по умолчанию

#define USE_TMP2NET_PORT 		0 // 65506 // =0 not use

//#define USE_MODBUS // включить пример Modbus TCP

#define USE_WDRV // включить пример Modbus TCP

//#define USE_CAPTDNS // NDS отвечающий на всё запросы клиента при соединении к AP модуля указанием на данный WebHttp (http://aesp8266/)

#endif // _user_config_h_


