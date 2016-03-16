udptp.ovl

Тестовый UDP Wave port. 
Передает по UDP поток со входа ADC до 192 кГц.
Выходной поток замеров в 2 байта 14..10 бит - зависит от скорости.

Инициализация:
mdb[70] - dword ADC sample rate 1..192000 SPS
mdb[72] - dword host ip
mdb[74] - word host port
mdb[75] - word remote port
Переменные:
mdb[76] - Флаг:	
			=0 - драйвер закрыт
			=1 - драйвер установлен и работает 

ovl$=2 - start передачи
ovl$=3 - stop передачи
			
См. WEBFilesTCP2UART\protect\udpwave.htm

Запрос / Выходная информация или команда
'?'	WDRV: ver	
'F=fx'	WDRV: freg fx Hz	
'A'	WDRV: set remote ip , remote port	
'S'	WDRV: stop	
'G'	Start wave
