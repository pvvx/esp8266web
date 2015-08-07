del libmlwip.a
md lwip.lib
cd lwip.lib
C:\Espressif\xtensa-lx106-elf\bin\xtensa-lx106-elf-ar x C:\Espressif\ESP8266_SDK\lib\liblwip.a
@rem sockets.o api_lib.o api_msg.o err.o netbuf.o netdb.o netifapi.o tcpip.o def.o mem.o stats.o sys.o sys_arch.o autoip.o inet.o ip_frag.o
@rem del: mdns.o dhcpserver.o
C:\Espressif\xtensa-lx106-elf\bin\xtensa-lx106-elf-ar ru ..\libmlwip.a dhcp.o dns.o init.o memp.o netif.o pbuf.o tcp.o tcp_in.o tcp_out.o timers.o udp.o icmp.o igmp.o inet_chksum.o ip.o ip_addr.o etharp.o raw.o 
cd ..
rd /q /s lwip.lib
