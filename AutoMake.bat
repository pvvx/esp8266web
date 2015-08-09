@echo off
@if %1x==x goto aaa_end
@if not exist %1 goto err_end
@echo ------------------------------------------------------------------------------
@C:\Python27\python.exe  ..\esptool.py elf2image -o ..\bin\ -ff 80m -fm qio -fs 4m %1
@echo ------------------------------------------------------------------------------
@if not exist ..\bin\0x00000.bin goto err_end
@echo Add rapid_loader...
@if exist ..\bin\0.bin del ..\bin\0.bin >nul
@rename ..\bin\0x00000.bin 0.bin >nul
@copy /b ..\bin\rapid_loader.bin + ..\bin\0.bin ..\bin\0x00000.bin >nul
goto end
:aaa_end
echo Use Eclipse Manage Configurations: AutoMake!
goto end
:err_end
echo Error!
:end
