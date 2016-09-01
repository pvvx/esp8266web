del libmmain.a
md main.lib
cd main.lib
C:\Espressif\xtensa-lx106-elf\bin\xtensa-lx106-elf-ar x C:\Espressif\ESP8266_SDK\lib\libmain.a
@rem app_main.o debug-vector.o double-vector.o eagle_lib.o eagle_lwip_if.o ets_timer.o 
@rem kernel-vector.o mem_manager.o nmi-vector.o spi_flash.o spi_overlap.o time.o 
@rem user-vector.o user_interface.o vector.o
C:\Espressif\xtensa-lx106-elf\bin\xtensa-lx106-elf-ar ru ..\libmmain.a user_interface.o
cd ..
rd /q /s main.lib
