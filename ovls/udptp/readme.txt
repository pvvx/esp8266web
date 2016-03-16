udptp.ovl

Тестовый UDP port.

Инициализация:
mdb[90] - remote port
Переменные:
mdb[91] - Флаг:	
			=0 - драйвер закрыт
			=1 - драйвер установлен и работает 
			=-1 - ошибка инициализации (номер порта)
			=-2 - ошибка инициализации (нехватает памяти)
			=-3 - ошибки в параметрах инициализации (номера пинов SCL, SDA)

См. WEBFilesTCP2UART\protect\udptst.htm 

Запрос / Выходная информация или команда
'A?' ChipID, WiFi...
'I?' UDP + TCP connections
'U?' UDP connections
'T?' TCP connections
'S?' WEB connections
'H?' 'heap' size
'R?' Sysmem restart
... .....