del libmc.a
md libc
cd libc
C:\Espressif\xtensa-lx106-elf\bin\xtensa-lx106-elf-ar x ..\libc.a
@rem delete: lib_a-rand.o lib_a-strlen.o lib_a-memcpy.o lib_a-memset.o
C:\Espressif\xtensa-lx106-elf\bin\xtensa-lx106-elf-ar ru ..\libmc.a @..\libmc_list_files.txt
cd ..
rd /q /s libc
