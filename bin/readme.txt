Flash files:
   Addr - file name
Program codes
0x00000 - bin\0x00000.bin
0x40000 - bin\0x40000.bin
WebFileSystem
0x0A000 - webbin\WEBFiles.bin
Clear old settings (EEP area):
0x79000 - bin\clear_eep.bin
Default config (Clear SDK settings):
0x7E000 - bin\blank.bin
0x7C000 - bin\esp_init_data_default.bin

SDK: The "bin" files are as follows:
1)blank.bin, provided by Espressif, burn to 0x7E000 address;
2)eagle.app.v6.flash.bin, as compiled, burn to 0x0000 address;
3)eagle.app.v6.irom0text.bin, as compiled, burn to 0x40000 address;
4)esp_init_data_default.bin, provided by Espressif, burn to 0x7c000 address 


