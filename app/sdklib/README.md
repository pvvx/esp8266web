# MinEspSDK (meSDK)
Minimalist SDK on ESP8266ex v1.5.4
---

A complete set of Wi-Fi and [LwIP](http://savannah.nongnu.org/projects/lwip/) functions.<br>
Имеет полный набор функций работы с WiFi и UDP/TCP (LwIP ver1.4.0).<br>
Данная сборка не содержит espconn и SSL.<br>
Проектируется для работы с датчиками и будет содержать расширения для быстрого<br> 
старта после deep-sleep с возможностями управления дальнейшей загрузки SDK или<br> 
опроса датчиков и нового перехода в режим deep-sleep.<br>
В целях экономии питания, время от просыпания после deep-sleep до старта опроса<br> 
датчиков и для принятия решения повторного засыпания или загрузки полного SDK<br>
для коммуникации и передачи накопленных данных будет составлять 30..40 мс.<br>  
В текщей версии, при стандартных настройках по умолчанию, после события подачи<br>
питания, reset или deep-sleep соединение по TCP при фиксированном ip модуля в<br> 
режиме STATION к модулю в режиме SOFTAP устанавливается примерно не более чем<br> 
через 540 мс. Основное время занимает инициализации SDK в части WiFi.<br>
Далее полудуплексный трафик TCP составляет более 1-го Мегабайта в секунду.<br>    

From [Espressif SDK](http://bbs.espressif.com/) ver 1.5.4 used only:<br> 
libpp.a, libwpa.a, libcrypto.a, libnet80211.a, parts libphy.a, user_interface.o<br>
Из [Espressif SDK](http://bbs.espressif.com/) ver 1.5.4 используются только описанные части.<br>
Остальные части даны с исходными кодами.<br>
LwIP based on [Open source LWIP for ESP_IOT_SDK_V1.5.2](http://bbs.espressif.com/viewtopic.php?f=46&t=1221).<br>

Supported options 48 kbytes IRAM.<br>
Supported '[Rapid Loader](https://github.com/pvvx/Rapid_Loader/)' and Flash 512 кbytes - 16 Mbytes.<br>
Поддержка расширенной памяти IRAM в 48 килобайт (опция USE_MAX_IRAM 48),<br>
Flash от 512 килобайт до 16 Мегабайт и ускоряющего загрузку SDK 'лоадера'.<br>

Free IRAM : 12 or 28 kbytes (option USE_MAX_IRAM) <br>
Free Heap : 55 kbytes<br>
Total Free RAM : 83 kbytes<br>

Options programming Flash:<br> 

SPI_SPEED: 40MHz or 80MHz.<br>
SPI_MODE: QIO only.<br>
FLASH_SIZE: Always set the size to 512 KB flash.<br>
			Automatic determination of the real size of the flash.<br>
При заливке прошивки в модуль всегда устанавливайте размер Flash в 512 килобайт.<br> 
Реальный размер Flash определяется автоматически во время старта SDK.<br> 

Для компиляции SDK используется [Unofficial Developer Kit](http://esp8266.ru/forum/forums/devkit/).<br>

В Eclipse заданы 3 опции Manage Configurations:<br>
1. AutoMake (собрать проект для прошивки, используются установки в Eclipse)<br>
2. CreateLib (собрать библиотеку libsdk.a, используются установки в Eclipse)<br>
3. Default (собрать проект для прошивки, используя makefile)<br>

Полный комплект для сборки проекта с помощью SDK библиотеки:<br>
libsdk.a + [libmicroc.a](https://github.com/anakod/esp_microc) и include<br>     

