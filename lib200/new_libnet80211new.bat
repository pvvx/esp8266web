del libnet80211_new.a
cd libnet80211_new
C:\Espressif\xtensa-lx106-elf\bin\xtensa-lx106-elf-ar x ..\libnet80211.a
del wl_cnx.o
C:\Espressif\xtensa-lx106-elf\bin\xtensa-lx106-elf-ar ru ..\libnet80211_new.a *.o
cd ..

