   Addr - file name 			- Info
--------------------------------------------------------   
0x00000 - bin/0x00000.bin		- Program codes IRAM/RAM
0x07000 - bin/0x07000.bin		- Program codes Cache Flash
0x79000 - bin/clear_eep.bin		- Clear settings EEP area
0x7C000 - bin/esp_init_data_default.bin - RF SDK options
0x7E000 - bin/blank.bin			- Default SDK WiFi config

bin/rapid_loader.bin - loader FlashSPI CLK x2 (80MHz)
bin/rapid_loader_m40.bin - loader FlashSPI CLK x1 (40MHz)

If Flash > 512 kbytes:
0x80000 - webbin/WEBFiles.bin
Use PVFS2.exe or http://192.168.4.1/fsupload.	



