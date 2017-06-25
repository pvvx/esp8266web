del libnet80211_new.a
rem md libnet80211.lib
cd libnet80211_new
@rem C:\Espressif\xtensa-lx106-elf\bin\xtensa-lx106-elf-ar x C:\Espressif\ESP8266_SDK\lib\libnet80211.a
@rem phy.o phy_chip_v6.o phy_chip_v6_ana.o phy_chip_v6_cal.o  phy_sleep.o
@rem delete: ate_test.o phy_chip_v6_unused.o
C:\Espressif\xtensa-lx106-elf\bin\xtensa-lx106-elf-ar ru ..\libnet80211_new.a *.o
cd ..
rem rd /q /s libnet80211.lib
