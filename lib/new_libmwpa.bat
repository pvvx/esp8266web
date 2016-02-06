del libmwpa.a
md wpa.lib
cd wpa.lib
C:\Espressif\xtensa-lx106-elf\bin\xtensa-lx106-elf-ar x C:\Espressif\ESP8266_SDK\lib\libwpa.a
@rem aes-internal-enc.o aes-wrap.o ap_config.o common.o ieee802_1x.o sta_info.o wpa.o wpa_auth.o wpa_auth_ie.o wpa_common.o wpa_ie.o wpa_main.o wpabuf.o wpas_glue.o
@rem del: wpa_debug.o os_xtensa.o
C:\Espressif\xtensa-lx106-elf\bin\xtensa-lx106-elf-ar ru ..\libmwpa.a  ap_config.o common.o ieee802_1x.o sta_info.o wpa.o wpa_auth.o wpa_auth_ie.o wpa_common.o wpa_ie.o wpa_main.o wpabuf.o wpas_glue.o
@rem none: aes-internal-enc.o aes-wrap.o 
cd ..
rd /q /s wpa.lib
