# esp8266web
Small WEB server on ESP8266 + TCP2UART or Modbus RS-485 
---

HTTP-Web supports more than hundred variables -<br>
All I/O, Wifi, system, ... variables.<br>
GET/POST/websocket, cookie, load/upload data/Flash, multipart/form-data,...<br>
TCP2UART: 300..3000000 Baud, Flow Control On/Off, Inverse On/Of all signals, ...<br>
RS-485: half-duplex 300..1000000 Baud <br>
HTTP-Web services to more than 10 simultaneous open connections.<br>
Websocket open uri: '/web.cgi'<br>
Supports Overlay drivers.<br>

TCP/HTTP transfer speed:<br>
~1 Mbytes/sec (debug off).<br>

UDP Wave server (Integrated SAR ADC):<br>
Sends 14-bit  samples at 1 Hz .. 48 kHz ([max 192 kHz 12 bits](https://github.com/pvvx/esp8266web/blob/master/ESP-ADC-192kHz.gif)).<br>

Modbus TCP port 502:<br>
 GPIOs, ADC, VDD.<br>
RS-485 chematics: 
![SCH](https://github.com/pvvx/esp8266web/blob/master/ESP-01_RS-485_sch.gif)

Supported options 48 kbytes IRAM.<br>
Supported '[Rapid Loader](https://github.com/pvvx/Rapid_Loader/)' and Flash 512 кbytes - 16 Mbytes.<br>
Поддержка расширенной памяти IRAM в 48 килобайт,<br>
Flash от 512 килобайт до 16 Мегабайт и ускоряющего загрузку SDK 'лоадера'.<br>

From [Espressif SDK](http://bbs.espressif.com/) ver 1.5.2 used only:<br> 
libpp.a, libwpa.a, libnet80211.a, parts libphy.a, libcrypto.a, user_interface.o<br>
Из [Espressif SDK](http://bbs.espressif.com/) ver 1.5.2 используются только описанные части.<br>
Остальные части даны с исходными кодами.<br>
LwIP based on [Open source LWIP for ESP_IOT_SDK_V1.4.0](http://bbs.espressif.com/viewtopic.php?f=46&t=1221).<br> 

Options programming Flash:<br> 

SPI_SPEED: 40MHz or 80MHz.<br>
SPI_MODE: QIO only.<br>
FLASH_SIZE: Always set the size to 512 KB flash.<br>
			Automatic determination of the real size of the flash.<br>
При заливке прошивки в модуль всегда устанавливайте размер Flash в 512 килобайт.<br> 
Реальный размер Flash определяется автоматически во время старта SDK.<br>

Make WEBFS CmdLine:<br>

WEBFS22.exe -h "*.htm, *.html, *.cgi, *.xml, *.bin, *.txt, *.wav" -z "mdbini.bin, *.inc, *.ini, snmp.bib" .\WEBFiles .\webbin WEBFiles.bin<br>

Upload WEBFS:<br>

1) Connect AP 'ESP8266', password '0123456789'<br>
2) Explorer: http://192.168.4.1/fsupload (name and password from WiFi AP)<br>
3) Select WEBFiles.bin file. Upload.<br>

Для компиляции SDK используется [Unofficial Developer Kit](http://esp8266.ru/forum/forums/devkit/).<br><br>

Переключение проекта с TCP2UART на MODBUS RS-485 производится в include/user_config.h.<br>
Файлы для web-диска для проектов находятся в разных папках.<br> 
Желательно переместить используемые для проекта в папку WEBFiles.<br>  
Но возможно использование и make_webfs_rs485.bat или make_webfs_tcp2uart.bat.<br>

В Eclipse заданы 3 опции Manage Configurations:<br>
1. AutoMake (собрать проект для прошивки, используются установки в Eclipse)<br>
2. CreateLib (собрать библиотеку libsdk.a (meSDK), используются установки в Eclipse)<br>
3. Default (собрать проект для прошивки, используя makefile)<br>

[Forum esp8266web Web_Base](http://esp8266.ru/forum/threads/razrabotka-biblioteki-malogo-webservera-na-esp8266.56/)
[Forum Modbus TCP / RTU RS-485 + WEB server](http://esp8266.ru/forum/threads/modbus-tcp-rtu-rs-485-web-server.911/)
