@echo off
@if %1x==x goto aaa_end
@if not exist bin/%1.bin goto err_end
@C:/Python27/python.exe esptool.py -p %2 -b %3 write_flash -ff 80m -fm qio -fs 4m 0x00000 bin/0x00000.bin %1 bin/%1.bin
goto end
:aaa_end
echo Use Eclipse! 
goto end
:err_end
echo Error!
:end
