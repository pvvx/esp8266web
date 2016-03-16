tcp_client.ovl

Пример TCP клиента.
Считывает температуру воздуха из HTML по адресу http://weather-in.ru/sankt-peterbyrg/107709


Переменные:
mdb[50] - время в минутах до следующего чтения данных, = 0 один раз
mdb[51] - Флаг: =0 - драйвер закрыт, =1 - драйвер установлен
mdb[52] - Флаг: =1 - данные актуальны, другое значение -> данные ещё не считаны
mdb[53] - Считанная с сайта температура (минимум, со знаком)
mdb[54] - Считанная с сайта температура (максимум, со знаком)

Лог с загрузкой оверлея:
srv[80] 192.168.1.2:55676 [1] listen
srv[80] 192.168.1.2:55676 [1] read: 1460 of254[/fsupload] content_len = 1503 of 1189 'ESP8266:0123456789' POST f[/fsupload] filename:'tcp_client.ovl' ...
srv[80] 192.168.1.2:55676 [1] read: 1316 Wr:0x40106300[0x0000041c] Wr:0x3fffdf60[0x00000074] Wr:0x3fffdfd4[0x00000040] Run0x401066a0 head[82]:302 dis
srv[80] 192.168.1.2:55676 [1] disconnect
cf254 srv[4097] 62.149.0.160:80 [0] start client - Ok
srv[4097] 62.149.0.160:80 [1] send 62 bytes
srv[80] 192.168.1.2:55677 [1] listen
srv[80] 192.168.1.2:55677 [1] read: 169 of1[disk_ok.htm] GET f[/disk_ok.htm] head[201]:200 send: of2[timer.inc] cf2 of2[footer.inc] cf2 cf1 722 dis
srv[4097] 62.149.0.160:80 [1] received, buffer 311 bytes
srv[4097] 62.149.0.160:80 [1] received, buffer 796 bytes
srv[4097] 62.149.0.160:80 [1] received, buffer 1477 bytes
srv[4097] 62.149.0.160:80 [1] received, buffer 1477 bytes
srv[4097] 62.149.0.160:80 [1] received, buffer 1278 bytes
srv[4097] 62.149.0.160:80 [1] received, buffer 1477 bytes
srv[4097] 62.149.0.160:80 [1] received, buffer 1953 bytes
"Температура воздуха, &#176;C" -1..3 srv[80] 192.168.1.2:55677 [1] disconnect
srv[4097] 62.149.0.160:80 [1] disconnect
