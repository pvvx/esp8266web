10dof.ovl

Передача данных по UDP от 10-DOF GY-91
(акселерометр, гироскоп, магнитометр, температура, давление)
Опрос по SPI 100 замеров x,y,z (через FIFO) в секунду,
вывод 10 UDP пакетов в секунду, в каждом 10 замеров.

Подключение GY-91 - ESP8266:

VIN	- не использован
3V3 - 3V3
GND - GND
SCL - HSPICLK/GPIO14
SDA - HSPIQ MISO/GPIO13
SDO/SAO - HSPID MOSI/GPIO12
NCS - GPIO4 (задается)
CSB - GPIO5 (задается)

Инициализация:
mdb[40] - Номер пина (0..15) для сигнала CS_BMP280, по умолчанию GPIO5
mdb[41] - Номер пина (0..15) для сигнала CS_MPU9250, по умолчанию GPIO4
mdb[42] - dword host ip, по умолчанию 255.255.255.255 (всем)
mdb[44] - word host port, по умолчанию 7777
Переменные:
mdb[45] - Флаг:	
			=0 - драйвер закрыт
			=1 - драйвер установлен и работает 
mdb[46] - Ошибки: =0 - нет ошибок
mdb[47] - temperature in DegC, resolution is 0.01 DegC, signed.
mdb[48] - pressure in Pa as unsigned long, resolution is 0.01 hPa.

Доп.команды:

ovl$=2 - start передачи по UDP
ovl$=3 - stop передачи по UDP
			

Запрос / Выходная информация или команда по UDP
'?'	UDRV: ver	
'A'	UDRV: set remote ip , remote port	
'S'	UDRV: stop	
'G'	UDRV: start

Формат UDP посылки см. в ovl_config.h
