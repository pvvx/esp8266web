#include "user_interface.h"
#ifdef USE_CAPTDNS
#include "osapi.h"
#include "bios/ets.h"
#include "hw/esp8266.h"
#include "lwip/err.h"
#include "lwip/udp.h"
#include "sdk/rom2ram.h"
#include "captdns.h"
#include "lwip/ip_addr.h"
#include "sdk/add_func.h"


/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * Jeroen Domburg <jeroen@spritesmods.com> wrote this file. As long as you retain 
 * this notice you can do whatever you want with this stuff. If we meet some day, 
 * and you think this stuff is worth it, you can buy me a beer in return. 
 * ----------------------------------------------------------------------------
 * added, editing, update: pvvx
 */

/*
This is a 'captive portal' DNS server: it basically replies with a fixed IP (in this case:
the one of the SoftAP interface of this ESP module) for any and all DNS queries. This can 
be used to send mobile phones, tablets etc which connect to the ESP in AP mode directly to
the internal webserver.
*/

const char HostNameLocal[] = "aesp8266";
const char httpHostNameLocal[] ICACHE_RODATA_ATTR = "http://%s/";

typedef struct __attribute__ ((packed)) {
	uint16_t id;
	uint8_t flags;
	uint8_t rcode;
	uint16_t qdcount;
	uint16_t ancount;
	uint16_t nscount;
	uint16_t arcount;
} DnsHeader;


typedef struct __attribute__ ((packed)) {
	uint8_t len;
	uint8_t data;
} DnsLabel;


typedef struct __attribute__ ((packed)) {
	//before: label
	uint16_t type;
	uint16_t class;
} DnsQuestionFooter;


typedef struct __attribute__ ((packed)) {
	//before: label
	uint16_t type;
	uint16_t class;
	uint32_t ttl;
	uint16_t rdlength;
	//after: rdata
} DnsResourceFooter;

typedef struct __attribute__ ((packed)) {
	uint16_t prio;
	uint16_t weight;
} DnsUriHdr;


#define FLAG_QR (1<<7)
#define FLAG_AA (1<<2)
#define FLAG_TC (1<<1)
#define FLAG_RD (1<<0)

#define QTYPE_A  1
#define QTYPE_NS 2
#define QTYPE_CNAME 5
#define QTYPE_SOA 6
#define QTYPE_WKS 11
#define QTYPE_PTR 12
#define QTYPE_HINFO 13
#define QTYPE_MINFO 14
#define QTYPE_MX 15
#define QTYPE_TXT 16
#define QTYPE_URI 256

#define QCLASS_IN 1
#define QCLASS_ANY 255
#define QCLASS_URI 256

struct udp_pcb *pcb_cdns DATA_IRAM_ATTR;

//Function to put unaligned 16-bit network values
static void ICACHE_FLASH_ATTR setn16(void *pp, int16_t n) {
	char *p=pp;
	*p++=(n>>8);
	*p++=(n&0xff);
}

//Function to put unaligned 32-bit network values
static void ICACHE_FLASH_ATTR setn32(void *pp, int32_t n) {
	char *p=pp;
	*p++=(n>>24)&0xff;
	*p++=(n>>16)&0xff;
	*p++=(n>>8)&0xff;
	*p++=(n&0xff);
}

static uint16_t ICACHE_FLASH_ATTR _ntohs(uint16_t *in) {
	char *p=(char*)in;
	return ((p[0]<<8)&0xff00)|(p[1]&0xff);
}


//Parses a label into a C-string containing a dotted 
//Returns pointer to start of next fields in packet
static char* ICACHE_FLASH_ATTR labelToStr(char *packet, char *labelPtr, int packetSz, char *res, int resMaxLen) {
	int i, j, k;
	char *endPtr=NULL;
	i=0;
	do {
		if ((*labelPtr&0xC0)==0) {
			j=*labelPtr++; //skip past length
			//Add separator period if there already is data in res
			if (i<resMaxLen && i!=0) res[i++]='.';
			//Copy label to res
			for (k=0; k<j; k++) {
				if ((labelPtr-packet)>packetSz) return NULL;
				if (i<resMaxLen) res[i++]=*labelPtr++;
			}
		} else if ((*labelPtr&0xC0)==0xC0) {
			//Compressed label pointer
			endPtr=labelPtr+2;
			int offset=_ntohs(((uint16_t *)labelPtr))&0x3FFF;
			//Check if offset points to somewhere outside of the packet
			if (offset>packetSz) return NULL;
			labelPtr=&packet[offset];
		}
		//check for out-of-bound-ness
		if ((labelPtr-packet)>packetSz) return NULL;
	} while (*labelPtr!=0);
	res[i]=0; //zero-terminate
	if (endPtr==NULL) endPtr=labelPtr+1;
	return endPtr;
}


//Converts a dotted hostname to the weird label form dns uses.
static char ICACHE_FLASH_ATTR *strToLabel(char *str, char *label, int maxLen) {
	char *len=label; //ptr to len byte
	char *p=label+1; //ptr to next label byte to be written
	while (1) {
		if (*str=='.' || *str==0) {
			*len=((p-len)-1);	//write len of label bit
			len=p;				//pos of len for next part
			p++;				//data ptr is one past len
			if (*str==0) break;	//done
			str++;
		} else {
			*p++=*str++;	//copy byte
//			if ((p-label)>maxLen) return NULL;	//check out of bounds
		}
	}
	*len=0;
	return p; //ptr to first free byte in resp
}


//Receive a DNS packet and maybe send a response back
static int ICACHE_FLASH_ATTR _cdns_recv(char *reply, unsigned short length)
{
	char buff[512];
//	char reply[512];
	int i;
	char *p=reply;
	char *rend = &reply[length];
	DnsHeader *hdr=(DnsHeader*)p;
	DnsHeader *rhdr=(DnsHeader*)&reply[0];
	p+=sizeof(DnsHeader);
//	os_printf("DNS packet: id 0x%X flags 0x%X rcode 0x%X qcnt %d ancnt %d nscount %d arcount %d len %d\n", 
//		_ntohs(&hdr->id), hdr->flags, hdr->rcode, _ntohs(&hdr->qdcount), _ntohs(&hdr->ancount), _ntohs(&hdr->nscount), _ntohs(&hdr->arcount), length);
	//Some sanity checks:
	if (hdr->ancount || hdr->nscount || hdr->arcount) return 0;	//this is a reply, don't know what to do with it
	if (hdr->flags&FLAG_TC) return 0;								//truncated, can't use this
	//Reply is basically the request plus the needed data
//	os_memcpy(reply, pusrdata, length);
	rhdr->flags|=FLAG_QR;
	for (i=0; i<_ntohs(&hdr->qdcount); i++) {
		//Grab the labels in the q string
		p=labelToStr(reply, p, length, buff, sizeof(buff));
		if (p==NULL) return 0;
		DnsQuestionFooter *qf=(DnsQuestionFooter*)p;
		p+=sizeof(DnsQuestionFooter);
#if DEBUGSOO > 1
		os_printf("cdns: Q (type 0x%X class 0x%X) for %s\n", _ntohs(&qf->type), _ntohs(&qf->class), buff);
#endif
		if (_ntohs(&qf->type)==QTYPE_A) {
			//They want to know the IPv4 address of something.
			//Build the response.
			rend=strToLabel(buff, rend, sizeof(reply)-(rend-reply)); //Add the label
			if (rend==NULL) return 0;
			DnsResourceFooter *rf=(DnsResourceFooter *)rend;
			rend+=sizeof(DnsResourceFooter);
			setn16(&rf->type, QTYPE_A);
			setn16(&rf->class, QCLASS_IN);
			setn32(&rf->ttl, 1);
			setn16(&rf->rdlength, 4); //IPv4 addr is 4 bytes;
			//Grab the current IP of the softap interface
			struct ip_addr ipx;
			if(rom_xstrcmp(buff, "www.msftncsi.com")) { // www.msftncsi.com/ncsi.txt
				IP4_ADDR(&ipx, 131,107,255,255);
			}
			else ipx.addr = info.ap_ip;
//			else if(rom_xstrcmp(buff, "clients3.google.com")) { //clients3.google.com/generate_204
//				// IP4_ADDR(&info.ip, 173,194,112,233);
//			}
//			else {
//				wifi_get_ip_info(SOFTAP_IF, &info);
//			}
			*rend++=ip4_addr1(&ipx);
			*rend++=ip4_addr2(&ipx);
			*rend++=ip4_addr3(&ipx);
			*rend++=ip4_addr4(&ipx);
			setn16(&rhdr->ancount, _ntohs(&rhdr->ancount)+1);
//			os_printf("Added A rec to resp. Resp len is %d\n", (rend-reply));
		} else if (_ntohs(&qf->type)==QTYPE_NS) {
			//Give ns server. Basically can be whatever we want because it'll get resolved to our IP later anyway.
			rend=strToLabel(buff, rend, sizeof(reply)-(rend-reply)); //Add the label
			DnsResourceFooter *rf=(DnsResourceFooter *)rend;
			rend+=sizeof(DnsResourceFooter);
			setn16(&rf->type, QTYPE_NS);
			setn16(&rf->class, QCLASS_IN);
			setn16(&rf->ttl, 1);
			setn16(&rf->rdlength, 4);
			*rend++=2;
			*rend++='n';
			*rend++='s';
			*rend++=0;
			setn16(&rhdr->ancount, _ntohs(&rhdr->ancount)+1);
//			os_printf("Added NS rec to resp. Resp len is %d\n", (rend-reply));
		} else if (_ntohs(&qf->type)==QTYPE_URI) {
			//Give uri to us
			rend=strToLabel(buff, rend, sizeof(reply)-(rend-reply)); //Add the label
			DnsResourceFooter *rf=(DnsResourceFooter *)rend;
			rend+=sizeof(DnsResourceFooter);
			DnsUriHdr *uh=(DnsUriHdr *)rend;
			rend+=sizeof(DnsUriHdr);
			setn16(&rf->type, QTYPE_URI);
			setn16(&rf->class, QCLASS_URI);
			setn16(&rf->ttl, 1);
			setn16(&rf->rdlength, 4+16);
			setn16(&uh->prio, 10);
			setn16(&uh->weight, 1);
			rend += ets_sprintf(rend, httpHostNameLocal, HostNameLocal) - 1; // os_memcpy(, "http://aesp8266"-"/", 15);
			setn16(&rhdr->ancount, _ntohs(&rhdr->ancount)+1);
//			os_printf("Added NS rec to resp. Resp len is %d\n", (rend-reply));
		}
	}
	return rend-reply;
}

static void ICACHE_FLASH_ATTR cdns_recv(void *arg, struct udp_pcb *upcb, struct pbuf *p, ip_addr_t *addr, u16_t port)
{

	uint32 length = p->tot_len;
	if(length > 512 || length < sizeof(DnsHeader)) return;	//Packet is longer than DNS implementation allows or Packet is too short
	uint8 * pb = os_malloc(length+256);
	if(pb == NULL) return;
	length = pbuf_copy_partial(p, pb, length, 0);
    pbuf_free(p);
    length = _cdns_recv(pb, length);
	if(length != 0) {
		//Send the response
		struct pbuf *z = pbuf_alloc(PBUF_TRANSPORT, length, PBUF_RAM);
		if(z != NULL) {
	    	err_t err = pbuf_take(z, pb, length);
	    	if(err == ERR_OK) {
	    	    udp_sendto(pcb_cdns, z, addr, port);
	    	}
	  	    pbuf_free(z);
		}
	}
	os_free(pb);
}

void ICACHE_FLASH_ATTR captdns_close(void)
{
	if(pcb_cdns != NULL) {
		udp_disconnect(pcb_cdns);
		udp_remove(pcb_cdns);
		pcb_cdns = NULL;
#if DEBUGSOO > 1
		os_printf("cdns: close\n");
#endif
	}
}

bool ICACHE_FLASH_ATTR captdns_init(void)
{
//	captdns_close();
	if(pcb_cdns == NULL) {
		pcb_cdns = udp_new();
		if(pcb_cdns == NULL || (udp_bind(pcb_cdns, IP_ADDR_ANY, CAPTDNS_PORT) != ERR_OK)) {
	#if DEBUGSOO > 0
			os_printf("cdns: error init\n");
	#endif
			captdns_close();
			return false;
		}
	#if DEBUGSOO > 1
		os_printf("cdns: init port %u\n", CAPTDNS_PORT);
	#endif
		udp_recv(pcb_cdns, cdns_recv, pcb_cdns);
	}
	return true;
}
#endif // USE_CAPTDNS
