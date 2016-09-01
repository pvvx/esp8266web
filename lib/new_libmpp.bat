del libmpp.a
md libpp.lib
cd libpp.lib
C:\Espressif\xtensa-lx106-elf\bin\xtensa-lx106-elf-ar x C:\Espressif\ESP8266_SDK\lib\libpp.a
@rem esf_buf.o if_hwctrl.o lmac.o mac_frame.o pm.o pm_for_bcn_only_mode.o pp.o rate_control.o trc.o wdev.o
@del mac_frame.o
C:\Espressif\xtensa-lx106-elf\bin\xtensa-lx106-elf-ar ru ..\libmpp.a *.o
cd ..
rd /q /s libpp.lib
