@if exist .\app\.output\eagle\image\eagle.app.v6.out C:\Espressif\xtensa-lx106-elf\bin\xtensa-lx106-elf-objdump -S .\app\.output\eagle\image\eagle.app.v6.out > eagle.app.v6.asm
@if exist .\AutoMake\esp8266web.elf C:\Espressif\xtensa-lx106-elf\bin\xtensa-lx106-elf-objdump -S .\AutoMake\esp8266web.elf > esp8266web.asm


