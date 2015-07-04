del libmphy.a
md libphy.lib
cd libphy.lib
C:\Espressif\xtensa-lx106-elf\bin\xtensa-lx106-elf-ar x C:\Espressif\ESP8266_SDK\lib\libphy.a
@rem phy.o phy_chip_v6.o phy_chip_v6_ana.o phy_chip_v6_cal.o  phy_sleep.o
@rem delete: ate_test.o phy_chip_v6_unused.o
C:\Espressif\xtensa-lx106-elf\bin\xtensa-lx106-elf-ar ru ..\libmphy.a phy.o phy_chip_v6.o phy_chip_v6_ana.o phy_chip_v6_cal.o  phy_sleep.o
cd ..
rd /q /s libphy.lib
