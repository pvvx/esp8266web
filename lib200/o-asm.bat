for %%a in (*.a) do (
md %%a.o
cd %%a.o
C:\Espressif\xtensa-lx106-elf\bin\xtensa-lx106-elf-ar x ..\%%a
for %%o in (*.o) do C:\Espressif\xtensa-lx106-elf\bin\xtensa-lx106-elf-objdump -S %%o > %%o.asm
cd ..
)

