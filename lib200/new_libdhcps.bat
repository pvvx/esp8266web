del libdhcps.a
md main.lib
cd main.lib
C:\Espressif\xtensa-lx106-elf\bin\xtensa-lx106-elf-ar x C:\Espressif\ESP8266_SDK\lib\liblwip.a
C:\Espressif\xtensa-lx106-elf\bin\xtensa-lx106-elf-ar ru ..\libdhcps.a dhcpserver.o
cd ..
rd /q /s main.lib
