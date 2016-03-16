iointr.ovl

По просьбе трудящихся.

Инициализация:
mdb[80] - номер пина (0..15) для счетчика Counter1, по умолчанию GPIO4
mdb[81] - номер пина (0..15) для счетчика Counter2, по умолчанию GPIO5
Переменные:
mdb[82] - флаг =1 - драйвер работает, =0 остановлен

ТЗ :) из темы 
http://esp8266.ru/forum/threads/ne-poluchaetsja-sobrat-proekty-ot-pvvx-pod-linux.926/#post-14214


 
Counter1 и Counter2 доступны, как вэб переменные sys_ucnst_1 и sys_ucnst_2

При старте считать Counter1 и Counter2 из EEPROM (sys_ucnst_1 и sys_ucnst_2).
Приаттачить прерывание на GPIO 4
Приаттачить прерывание на GPIO 5

Обработчик прерывания по фронту на GPIO 4
     Count1++;
     if (Count1 == 10) {
         Count1=0;
         Counter1++;
         Сохранить значение Counter1 в EEPROM (sys_ucnst_1);
      }

Обработчик прерывания по фронту на GPIO 5
     Count2++;
     if (Count2 == 10) {
         Count2=0;
         Counter2++;
         Сохранить значение Counter2 в EEPROM (sys_ucnst_2);
      }
