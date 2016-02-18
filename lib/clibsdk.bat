@echo off
@if not %1x==x goto aaa
if exist ..\CreateLib\MinEspSDKLib.lib cd ..\CreateLib
if exist ..\AutoMake\MinEspSDKLib.lib cd ..\AutoMake
call ..\lib\clibsdk.bat MinEspSDKLib.lib
cd ..\lib
goto end
:aaa
if not exist %1 goto err_end
if not exist ..\lib\libmmain.a goto err_end
if not exist ..\lib\libmphy.a goto err_end
if not exist ..\lib\libmwpa.a goto err_end
if not exist ..\lib\libnet80211.a goto err_end
if not exist ..\lib\libpp.a goto err_end
if not exist ..\lib\libmgcc.a goto err_end
if not exist ..\lib\libcrypto.a goto err_end
if not exist _temp md _temp
cd _temp
c:\Espressif\xtensa-lx106-elf\bin\xtensa-lx106-elf-ar xo  ..\%1
c:\Espressif\xtensa-lx106-elf\bin\xtensa-lx106-elf-ar xo  ..\..\lib\libmmain.a
c:\Espressif\xtensa-lx106-elf\bin\xtensa-lx106-elf-ar xo  ..\..\lib\libmphy.a
c:\Espressif\xtensa-lx106-elf\bin\xtensa-lx106-elf-ar xo  ..\..\lib\libmwpa.a
c:\Espressif\xtensa-lx106-elf\bin\xtensa-lx106-elf-ar xo  ..\..\lib\libnet80211.a
c:\Espressif\xtensa-lx106-elf\bin\xtensa-lx106-elf-ar xo  ..\..\lib\libpp.a
c:\Espressif\xtensa-lx106-elf\bin\xtensa-lx106-elf-ar xo  ..\..\lib\libmgcc.a
c:\Espressif\xtensa-lx106-elf\bin\xtensa-lx106-elf-ar xo  ..\..\lib\libcrypto.a
if exist ..\..\lib\libsdk.a del ..\..\lib\libsdk.a
c:\Espressif\xtensa-lx106-elf\bin\xtensa-lx106-elf-ar ru ..\..\lib\libsdk.a *.o
cd ..
rd /q /s _temp
if exist ..\lib\libsdk.a goto end
:err_end
echo "Error!"
:end