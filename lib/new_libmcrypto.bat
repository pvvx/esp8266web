del libmcrypto.a
md libcrypto.lib
cd libcrypto.lib
C:\Espressif\xtensa-lx106-elf\bin\xtensa-lx106-elf-ar x C:\Espressif\ESP8266_SDK\lib\libcrypto.a
@rem aes-cbc.o aes-internal.o aes-internal-dec.o aes-internal-enc.o aes-wrap.o bignum.o 
@rem crypto_internal.o crypto_internal-cipher.o crypto_internal-modexp.o crypto_internal-rsa.o 
@rem dh_group5.o dh_groups.o sha256.o sha256-internal.o
C:\Espressif\xtensa-lx106-elf\bin\xtensa-lx106-elf-ar ru ..\libmcrypto.a aes-wrap.o aes-internal-enc.o
cd ..
rd /q /s libcrypto.lib
