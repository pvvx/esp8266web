del liblwipif.a
md main.lib
cd main.lib
C:\Espressif\xtensa-lx106-elf\bin\xtensa-lx106-elf-ar x C:\Espressif\ESP8266_SDK\lib\libmain.a
C:\Espressif\xtensa-lx106-elf\bin\xtensa-lx106-elf-ar ru ..\liblwipif.a eagle_lwip_if.o
cd ..
rd /q /s main.lib
