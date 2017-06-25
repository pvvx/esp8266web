del libmmain.a
md main.lib
cd main.lib
C:\Espressif\xtensa-lx106-elf\bin\xtensa-lx106-elf-ar x C:\Espressif\ESP8266_SDK\lib\libmain.a
@rem eagle_lib.o ets_timer.o mem_manager.o user_interface.o eagle_lwip_if.o
@rem del: spi_flash.o app_main.o ets_timer.o eagle_lwip_if.o mem_manager.o eagle_lib.o
C:\Espressif\xtensa-lx106-elf\bin\xtensa-lx106-elf-ar ru ..\libmmain.a user_interface.o
cd ..
rd /q /s main.lib
