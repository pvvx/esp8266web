del libmgcc.a
md libgcc
cd libgcc
C:\Espressif\xtensa-lx106-elf\bin\xtensa-lx106-elf-ar x ..\libgcc.a
@rem delete:
@rem _fixunsdfsi.o _umoddi3.o _umodsi3.o _extendsfdf2.o _fixdfsi.o _divsi3.o _divdf3.o _divdi3.o _fixunssfsi.o
@rem _floatsidf.o _floatsisf.o _floatunsidf.o _floatunsisf.o _muldf3.o _muldi3.o _mulsf3.o _truncdfsf2.o
@rem _udivdi3.o _udivsi3.o _umulsidi3.o _addsubdf3.o _addsubsf3.o
C:\Espressif\xtensa-lx106-elf\bin\xtensa-lx106-elf-ar ru ..\libmgcc.a @..\libmgcc_list_files.txt
cd ..
rd /q /s libgcc
