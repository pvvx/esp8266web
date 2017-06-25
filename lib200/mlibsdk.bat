@echo off
if not exist ..\app\sdklib\.output\eagle\lib\libsdk.a goto err_end
del libsdk.a
md sdklib
cd sdklib
c:\Espressif\xtensa-lx106-elf\bin\xtensa-lx106-elf-ar xo  ..\..\app\sdklib\.output\eagle\lib\libsdk.a
c:\Espressif\xtensa-lx106-elf\bin\xtensa-lx106-elf-ar xo  ..\libmmain.a
c:\Espressif\xtensa-lx106-elf\bin\xtensa-lx106-elf-ar xo  ..\libmphy.a
c:\Espressif\xtensa-lx106-elf\bin\xtensa-lx106-elf-ar xo  ..\libmwpa.a
c:\Espressif\xtensa-lx106-elf\bin\xtensa-lx106-elf-ar xo  ..\libcrypto.a
c:\Espressif\xtensa-lx106-elf\bin\xtensa-lx106-elf-ar xo  ..\libnet80211.a
c:\Espressif\xtensa-lx106-elf\bin\xtensa-lx106-elf-ar xo  ..\libpp.a
c:\Espressif\xtensa-lx106-elf\bin\xtensa-lx106-elf-ar xo  ..\libmgcc.a
c:\Espressif\xtensa-lx106-elf\bin\xtensa-lx106-elf-ar ru ..\libsdk.a *.o
cd ..
rd /q /s sdklib
goto end
:err_end
echo Error!
:end