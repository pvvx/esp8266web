# esp8266web
Small web server on ESP8266
---
Make WEBFS CmdLine:
PVFS2.exe -h "*.htm, *.html, *.cgi, *.xml, *.bin, *.txt, *.wav" -z "*.inc, snmp.bib" ./WEBFiles ./webbin ./WEBFiles.bin

Upload WEBFS:
1) Connect AP 'ESP8266', password '0123456789'
2) Explorer: http://192.168.4.1/fsupload (name and password from WiFi AP)
3) Selet WEBFiles.bin file. Upload.

esp8266web Web_Base
http://esp8266.ru/forum/threads/razrabotka-biblioteki-malogo-webservera-na-esp8266.56/

UDK
http://esp8266.ru/forum/forums/devkit/