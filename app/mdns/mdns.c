/**
 * lwip MDNS resolver file.
 *
 * Created on: Jul 29, 2010
 * Author: Daniel Toma
 *

 * ported from uIP resolv.c Copyright (c) 2002-2003, Adam Dunkels.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote
 *    products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**

 * This file implements a MDNS host name and PUCK service registration.

 *-----------------------------------------------------------------------------
 * Includes
 *----------------------------------------------------------------------------*/
#include <string.h>
#include <stdlib.h>
#include "lwip/opt.h"

// #if LWIP_MDNS /* don't build if not configured for use in lwipopts.h */
#include "mdns.h"
#include "utils.h"
#include "puck_def.h"
#include "puck/memory/fs.h"
#include "lwip/udp.h"
#include "lwip/mem.h"
#include "lwip/igmp.h"
#include "utils/lwiplib.h"
#include "utils/uartstdio.h"
#include "utils/ustdlib.h"
#include "driverlib/systick.h"

/** DNS server IP address */
#ifndef DNS_MULTICAST_ADDRESS
#define DNS_MULTICAST_ADDRESS        inet_addr("224.0.0.251") /* resolver1.opendns.com */
#endif

/** DNS server IP address */
#ifndef MDNS_LOCAL
#define MDNS_LOCAL                "local" /* resolver1.opendns.com */
#endif

/** DNS server port address */
#ifndef DNS_MDNS_PORT
#define DNS_MDNS_PORT           5353
#endif

/** DNS maximum number of retries when asking for a name, before "timeout". */
#ifndef DNS_MAX_RETRIES
#define DNS_MAX_RETRIES           4
#endif

/** DNS resource record max. TTL (one week as default) */
#ifndef DNS_MAX_TTL
#define DNS_MAX_TTL               604800
#endif

/* DNS protocol flags */
#define DNS_FLAG1_RESPONSE        0x84
#define DNS_FLAG1_OPCODE_STATUS   0x10
#define DNS_FLAG1_OPCODE_INVERSE  0x08
#define DNS_FLAG1_OPCODE_STANDARD 0x00
#define DNS_FLAG1_AUTHORATIVE     0x04
#define DNS_FLAG1_TRUNC           0x02
#define DNS_FLAG1_RD              0x01
#define DNS_FLAG2_RA              0x80
#define DNS_FLAG2_ERR_MASK        0x0f
#define DNS_FLAG2_ERR_NONE        0x00
#define DNS_FLAG2_ERR_NAME        0x03

/* DNS protocol states */
#define DNS_STATE_UNUSED          0
#define DNS_STATE_NEW             1
#define DNS_STATE_ASKING          2
#define DNS_STATE_DONE            3

/* MDNS registration type */
#define MDNS_HOSTNAME_REG         0
#define MDNS_SERVICE_REG          1

/* MDNS registration type */
#define MDNS_REG_ANSWER           1
#define MDNS_SD_ANSWER            2
#define MDNS_SERVICE_REG_ANSWER   3


/* MDNS registration time */
#define MDNS_HOST_TIME            120
#define MDNS_SERVICE_TIME         3600

/** MDNS name length with "." at the beginning and end of name*/
#ifndef MDNS_LENGTH_ADD
#define MDNS_LENGTH_ADD           2
#endif
PACK_STRUCT_BEGIN
/** DNS message header */
struct mdns_hdr {
  PACK_STRUCT_FIELD(u16_t id);
  PACK_STRUCT_FIELD(u8_t flags1);
  PACK_STRUCT_FIELD(u8_t flags2);
  PACK_STRUCT_FIELD(u16_t numquestions);
  PACK_STRUCT_FIELD(u16_t numanswers);
  PACK_STRUCT_FIELD(u16_t numauthrr);
  PACK_STRUCT_FIELD(u16_t numextrarr);
} PACK_STRUCT_STRUCT;
PACK_STRUCT_END

#define SIZEOF_DNS_HDR 12


PACK_STRUCT_BEGIN
/** MDNS query message structure */
struct mdns_query {
  /* MDNS query record starts with either a domain name or a pointer
     to a name already present somewhere in the packet. */
  PACK_STRUCT_FIELD(u16_t type);
  PACK_STRUCT_FIELD(u16_t class);
} PACK_STRUCT_STRUCT;
PACK_STRUCT_END

#define SIZEOF_DNS_QUERY 4

PACK_STRUCT_BEGIN
/** MDNS answer message structure */
struct mdns_answer {
  /* MDNS answer record starts with either a domain name or a pointer
     to a name already present somewhere in the packet. */
  PACK_STRUCT_FIELD(u16_t type);
  PACK_STRUCT_FIELD(u16_t class);
  PACK_STRUCT_FIELD(u32_t ttl);
  PACK_STRUCT_FIELD(u16_t len);
} PACK_STRUCT_STRUCT;
PACK_STRUCT_END
#define SIZEOF_DNS_ANSWER 10

PACK_STRUCT_BEGIN
/** MDNS answer message structure */
struct mdns_auth {
  PACK_STRUCT_FIELD(u32_t src);
} PACK_STRUCT_STRUCT;
PACK_STRUCT_END

#define SIZEOF_MDNS_AUTH 4
PACK_STRUCT_BEGIN
/** MDNS service registration message structure */
struct mdns_service {
  PACK_STRUCT_FIELD(u16_t prior);
  PACK_STRUCT_FIELD(u16_t weight);
  PACK_STRUCT_FIELD(u16_t port);
} PACK_STRUCT_STRUCT;
PACK_STRUCT_END

#define SIZEOF_MDNS_SERVICE 6


/* forward declarations */
static void mdns_recv(void *s, struct udp_pcb *pcb, struct pbuf *p, struct ip_addr *addr, u16_t port);

/*-----------------------------------------------------------------------------
 * Globales
 *----------------------------------------------------------------------------*/

/* MDNS variables */
static char                    host_name[MDNS_NAME_LENGTH];
static char                    service_name[MDNS_NAME_LENGTH];
static char                    puck_datasheet[PUCK_DATASHEET_SIZE];
static struct udp_pcb          *mdns_pcb;


static struct ip_addr          multicast_addr;
static struct ip_addr          host_addr;

#if (DNS_USES_STATIC_BUF == 1)
static u8_t                    mdns_payload[DNS_MSG_SIZE];
#endif /* (MDNS_USES_STATIC_BUF == 1) */

//*****************************************************************************
//
// Delay for the specified number of seconds.  Depending upon the current
// SysTick value, the delay will be between N-1 and N seconds (i.e. N-1 full
// seconds are guaranteed, along with the remainder of the current second).
//
//*****************************************************************************
void
Delay(unsigned long ulSeconds)
{
    //
    // Loop while there are more seconds to wait.
    //
    while(ulSeconds--)
    {
        //
        // Wait until the SysTick value is less than 1000.
        //
        while(SysTickValueGet() > 1000)
        {
        }

        //
        // Wait until the SysTick value is greater than 1000.
        //
        while(SysTickValueGet() < 1000)
        {
        }
    }
}
/*
 *  Function to set the UDP pcb used to send the mDNS packages

void getPcb(struct udp_pcb *pcb)
{
	mdns_pcb = pcb;
}
*/
#if DNS_DOES_NAME_CHECK
/**
 * Compare the "dotted" name "query" with the encoded name "response"
 * to make sure an answer from the DNS server matches the current mdns_table
 * entry (otherwise, answers might arrive late for hostname not on the list
 * any more).
 *
 * @param query hostname (not encoded) from the mdns_table
 * @param response encoded hostname in the DNS response
 * @return 0: names equal; 1: names differ
 */
static u8_t
mdns_compare_name(unsigned char *query, unsigned char *response)
{
  unsigned char n;

  do {
    n = *response++;
    /** @see RFC 1035 - 4.1.4. Message compression */
    if ((n & 0xc0) == 0xc0) {
      /* Compressed name */
      break;
    } else {
      /* Not compressed name */
      while (n > 0) {
        if ((*query) != (*response)) {
          return 1;
        }
        ++response;
        ++query;
        --n;
      };
      ++query;
    }
  } while (*response != 0);

  return 0;
}
#endif /* DNS_DOES_NAME_CHECK */


static err_t
mdns_register(u8_t type, const char* name, u8_t id)
{
  err_t err;
  struct mdns_hdr        	*hdr;
  struct mdns_query     	qry;
  struct mdns_answer 	    ans;
  struct mdns_auth			auth;
  struct mdns_service       serv;
  struct pbuf 				*p;
  char 						*query, *nptr;
  const char 				*pHostname;
  static char tmpBuf[PUCK_DATASHEET_SIZE+PUCK_SERVICE_LENGTH];
  u8_t n;
  u16_t length = 0;
  /* if here, we have either a new query or a retry on a previous query to process */
  p = pbuf_alloc(PBUF_TRANSPORT, SIZEOF_DNS_HDR + DNS_MAX_NAME_LENGTH +
                 SIZEOF_DNS_QUERY, PBUF_RAM);
  if (p != NULL) {
    LWIP_ASSERT("pbuf must be in one piece", p->next == NULL);
    /* fill dns header */
    hdr = (struct mdns_hdr*)p->payload;
    memset(hdr, 0, SIZEOF_DNS_HDR);
    hdr->id = htons(id);
    hdr->flags1 = DNS_FLAG1_RD;
    hdr->numquestions = htons(1);
    hdr->numauthrr = htons(1);
    query = (char*)hdr + SIZEOF_DNS_HDR;
    pHostname = name;
    --pHostname;

    /* convert hostname into suitable query format. */
    do {
      ++pHostname;
      nptr = query;
      ++query;
      for(n = 0; *pHostname != '.' && *pHostname != 0; ++pHostname) {
        *query = *pHostname;
        ++query;
        ++n;
      }
      *nptr = n;
    } while(*pHostname != 0);
    *query++='\0';

    /* fill dns query */
    qry.type  = htons(DNS_RRTYPE_ANY);
    qry.class = htons(DNS_RRCLASS_IN);

    MEMCPY( query, &qry, SIZEOF_DNS_QUERY);

    /* resize the query */
    query = query + SIZEOF_DNS_QUERY;

    /* set the name of the authority field.
     * The same name as the Query using the offset address*/
    *query++= DNS_OFFSET_FLAG;
	*query++= DNS_DEFAULT_OFFSET;

	/* fill dns authority */
	if(type == MDNS_HOSTNAME_REG ){
		ans.type   = htons(DNS_RRTYPE_A);
		ans.class  = htons(DNS_RRCLASS_IN);
		ans.ttl    = htonl(MDNS_HOST_TIME);
		ans.len    = htons(DNS_IP_ADDR_LEN);
		length     = DNS_IP_ADDR_LEN;
	}
	else{
		ans.type   = htons(DNS_RRTYPE_SRV);
		ans.class  = htons(DNS_RRCLASS_IN);
		ans.ttl    = htonl(MDNS_SERVICE_TIME);
		strcpy(tmpBuf, host_name);
		strcat(tmpBuf, ".");
		strcat(tmpBuf, MDNS_LOCAL);
		length     = strlen(tmpBuf) + MDNS_LENGTH_ADD;
		ans.len    = htons(SIZEOF_MDNS_SERVICE + length);
		length     = 0;
	}
	MEMCPY( query, &ans, SIZEOF_DNS_ANSWER);

	/* resize the query */
	query = query + SIZEOF_DNS_ANSWER;
	/* fill the payload of the mDNS message */
	if(type == MDNS_HOSTNAME_REG ){
		/* set the local IP address */
		auth.src    = host_addr.addr;
		MEMCPY( query, &auth, SIZEOF_MDNS_AUTH);
	}
	else{
		serv.prior  = htons(0);
		serv.weight = htons(0);
		serv.port   = htons(PUCK_PORT);
		MEMCPY( query, &serv, SIZEOF_MDNS_SERVICE);
		/* resize the query */
		query = query + SIZEOF_MDNS_SERVICE;

		pHostname = tmpBuf;
		--pHostname;

		/* convert hostname into suitable query format. */
		do {
		  ++pHostname;
		  nptr = query;
		  ++query;
		  for(n = 0; *pHostname != '.' && *pHostname != 0; ++pHostname) {
			*query = *pHostname;
			++query;
			++n;
		  }
		  *nptr = n;
		} while(*pHostname != 0);
		*query++='\0';
	}
    /* resize pbuf to the exact dns query */
    pbuf_realloc(p, (query + length) - ((char*)(p->payload)));

    /* send dns packet */
    err = udp_sendto(mdns_pcb, p, &multicast_addr, DNS_MDNS_PORT);

    /* free pbuf */
    pbuf_free(p);
  } else {
    err = ERR_MEM;
  }

  return err;
}
/**
 * Send a mDNS answer packet.
 *
 * @param type of answer hostname and service registration or service
 * @param name to query
 * @param id transaction ID in the DNS query packet
 * @return ERR_OK if packet is sent; an err_t indicating the problem otherwise
 */
static err_t
mdns_answer(u16_t type, const char* name, u8_t id)
{
  err_t err;
  struct mdns_hdr        	*hdr;
  struct mdns_answer 	    ans;
  struct mdns_auth			auth;
  struct mdns_service       serv;
  struct pbuf 				*p;
  char 						*query, *nptr;
  const char 				*pHostname;
  static char tmpBuf[PUCK_DATASHEET_SIZE+PUCK_SERVICE_LENGTH];
  u8_t n;
  u16_t length = 0;
  /* if here, we have either a new query or a retry on a previous query to process */
  p = pbuf_alloc(PBUF_TRANSPORT, SIZEOF_DNS_HDR + DNS_MAX_NAME_LENGTH +
                 SIZEOF_DNS_QUERY, PBUF_RAM);
  if (p != NULL) {
    LWIP_ASSERT("pbuf must be in one piece", p->next == NULL);
    /* fill dns header */
    hdr = (struct mdns_hdr*)p->payload;
    memset(hdr, 0, SIZEOF_DNS_HDR);
    hdr->id = htons(id);
    hdr->flags1 = DNS_FLAG1_RESPONSE;

    if(type == MDNS_SD_ANSWER ){
    	pHostname = DNS_SD_SERVICE;
    	hdr->numanswers = htons(1);
    }else if(type == MDNS_SERVICE_REG_ANSWER ){
    	pHostname = PUCK_SERVICE;
    	hdr->numanswers = htons(type);
    }else{
		pHostname = name;
		hdr->numanswers = htons(type);
    }
    query = (char*)hdr + SIZEOF_DNS_HDR;
    --pHostname;
    /* convert hostname into suitable query format. */
    do {
      ++pHostname;
      nptr = query;
      ++query;
      for(n = 0; *pHostname != '.' && *pHostname != 0; ++pHostname) {
        *query = *pHostname;
        ++query;
        ++n;
      }
      *nptr = n;
    } while(*pHostname != 0);
    *query++='\0';

    /* fill dns query */

    if(type == MDNS_REG_ANSWER ){

		ans.type   = htons(DNS_RRTYPE_A);
		ans.class  = htons(DNS_RRCLASS_IN);
		ans.ttl    = htonl(MDNS_SERVICE_TIME);
		ans.len    = htons(DNS_IP_ADDR_LEN);
		length     = DNS_IP_ADDR_LEN;

		MEMCPY( query, &ans, SIZEOF_DNS_ANSWER);

		/* resize the query */
		query = query + SIZEOF_DNS_ANSWER;

		/* set the local IP address */
		auth.src    = host_addr.addr ;
		MEMCPY( query, &auth, SIZEOF_MDNS_AUTH);
    }
    if(type == MDNS_SD_ANSWER ){

    	ans.type   = htons(DNS_RRTYPE_PTR);
		ans.class  = htons(DNS_RRCLASS_IN);
		ans.ttl    = htonl(MDNS_SERVICE_TIME);
		ans.len    = htons(sizeof(PUCK_SERVICE) +1 );
		length     = 0;

		MEMCPY( query, &ans, SIZEOF_DNS_ANSWER);

		/* resize the query */
		query = query + SIZEOF_DNS_ANSWER;
		pHostname = PUCK_SERVICE;
		--pHostname;

		/* convert hostname into suitable query format. */
		do {
		  ++pHostname;
		  nptr = query;
		  ++query;
		  for(n = 0; *pHostname != '.' && *pHostname != 0; ++pHostname) {
			*query = *pHostname;
			++query;
			++n;
		  }
		  *nptr = n;
		} while(*pHostname != 0);
		*query++='\0';
	}

    if(type == MDNS_SERVICE_REG_ANSWER ){

		ans.type   = htons(DNS_RRTYPE_PTR);
		ans.class  = htons(DNS_RRCLASS_IN);
		ans.ttl    = htonl(MDNS_SERVICE_TIME);
		strcpy(tmpBuf, name);
		strcat(tmpBuf, ".");
		strcat(tmpBuf, PUCK_SERVICE);

		length     = strlen(tmpBuf) + MDNS_LENGTH_ADD;
		//UARTprintf("service name length %d \n",length);
		ans.len    = htons(length);
		length     = 0;

		MEMCPY( query, &ans, SIZEOF_DNS_ANSWER);

		/* resize the query */
		query = query + SIZEOF_DNS_ANSWER;

		pHostname = tmpBuf;
		--pHostname;

		/* convert hostname into suitable query format. */
		do {
		  ++pHostname;
		  nptr = query;
		  ++query;
		  for(n = 0; *pHostname != '.' && *pHostname != 0; ++pHostname) {
			*query = *pHostname;
			++query;
			++n;
		  }
		  *nptr = n;
		} while(*pHostname != 0);
		*query++='\0';

		/* Service query*/
		pHostname = name;
		--pHostname;

		/* convert hostname into suitable query format. */
		do {
		  ++pHostname;
		  nptr = query;
		  ++query;
		  for(n = 0; *pHostname != '.' && *pHostname != 0; ++pHostname) {
			*query = *pHostname;
			++query;
			++n;
		  }
		  *nptr = n;
		} while(*pHostname != 0);

		/* Add to the service name the service local
		 * pointing to the beginning of the mDNS message*/
		*query++= DNS_OFFSET_FLAG;
		*query++= DNS_DEFAULT_OFFSET;

		/* fill the query */

		ans.type   = htons(DNS_RRTYPE_SRV);
		ans.class  = htons(DNS_RRCLASS_FLUSH_IN);
		ans.ttl    = htonl(MDNS_SERVICE_TIME);
		strcpy(tmpBuf, host_name);
		strcat(tmpBuf, ".");
		strcat(tmpBuf, MDNS_LOCAL);

		length     = strlen(tmpBuf) + MDNS_LENGTH_ADD;
		ans.len    = htons(SIZEOF_MDNS_SERVICE + length);
		length     = 0;
		MEMCPY( query, &ans, SIZEOF_DNS_ANSWER);

		/* resize the query */
		query = query + SIZEOF_DNS_ANSWER;
		/* fill the service properties */

		serv.prior  = htons(0);
		serv.weight = htons(0);
		serv.port   = htons(PUCK_PORT);
		MEMCPY( query, &serv, SIZEOF_MDNS_SERVICE);
		/* resize the query */
		query = query + SIZEOF_MDNS_SERVICE;

		pHostname = tmpBuf;
		--pHostname;

		/* convert hostname into suitable query format. */
		do {
		  ++pHostname;
		  nptr = query;
		  ++query;
		  for(n = 0; *pHostname != '.' && *pHostname != 0; ++pHostname) {
			*query = *pHostname;
			++query;
			++n;
		  }
		  *nptr = n;
		} while(*pHostname != 0);
		*query++='\0';

		/* TXT answer */
		pHostname = name;
		--pHostname;

		/* convert hostname into suitable query format. */
		do {
		  ++pHostname;
		  nptr = query;
		  ++query;
		  for(n = 0; *pHostname != '.' && *pHostname != 0; ++pHostname) {
			*query = *pHostname;
			++query;
			++n;
		  }
		  *nptr = n;
		} while(*pHostname != 0);

		/* Add to the service name the service local
		 * pointing to the beginning of the mDNS message*/
		*query++= DNS_OFFSET_FLAG;
		*query++= DNS_DEFAULT_OFFSET;

		/* fill the answer */
		ans.type   = htons(DNS_RRTYPE_TXT);
		ans.class  = htons(DNS_RRCLASS_IN);
		ans.ttl    = htonl(MDNS_SERVICE_TIME);
		length     = sizeof(SERVICE_DESCRIPTION);
		ans.len    = htons(length);
		length = 0;
		MEMCPY( query, &ans, SIZEOF_DNS_ANSWER);

		/* resize the query */
		query = query + SIZEOF_DNS_ANSWER;

		pHostname = SERVICE_DESCRIPTION;
		--pHostname;

		/* convert hostname into suitable query format. */
		do {
		  ++pHostname;
		  nptr = query;
		  ++query;
		  for(n = 0; *pHostname != '.' && *pHostname != 0; ++pHostname) {
			*query = *pHostname;
			++query;
			++n;
		  }
		  *nptr = n;
		} while(*pHostname != 0);
		*query++='\0';
	}
    /* resize pbuf to the exact dns query */
    pbuf_realloc(p, (query + length) - ((char*)(p->payload)));

    /* send dns packet */
    err = udp_sendto(mdns_pcb, p, &multicast_addr, DNS_MDNS_PORT);

    /* free pbuf */
    pbuf_free(p);
  } else {
    err = ERR_MEM;
  }

  return err;
}

/**
 * Send a mDNS service answer packet.
 *
 * @param name service name to query
 * @param id transaction ID in the DNS query packet
 * @return ERR_OK if packet is sent; an err_t indicating the problem otherwise
 */
static err_t
mdns_send_service(const char* name, u8_t id)
{
  err_t err;
  struct mdns_hdr  			*hdr;
  struct mdns_answer 	    ans;
  struct mdns_service       serv;
  struct mdns_auth			auth;
  struct pbuf 				*p;
  char 						*query, *nptr;
  const char 				*pHostname;
  u8_t n;
  u16_t length = 0;
  u8_t addr1 = 12, addr2 = 12;
  static char tmpBuf[PUCK_DATASHEET_SIZE+PUCK_SERVICE_LENGTH];

  /* if here, we have either a new query or a retry on a previous query to process */
  p = pbuf_alloc(PBUF_TRANSPORT, SIZEOF_DNS_HDR + DNS_MAX_NAME_LENGTH +
                 SIZEOF_DNS_QUERY, PBUF_RAM);
  if (p != NULL) {
    LWIP_ASSERT("pbuf must be in one piece", p->next == NULL);
    /* fill dns header */
    hdr = (struct mdns_hdr*)p->payload;
    memset(hdr, 0, SIZEOF_DNS_HDR);
    hdr->id = htons(id);
    hdr->flags1 = DNS_FLAG1_RESPONSE;
    hdr->numanswers = htons(4);
    query = (char*)hdr + SIZEOF_DNS_HDR;

    strcpy(tmpBuf, name);
	strcat(tmpBuf, ".");
	strcat(tmpBuf, PUCK_SERVICE);

    pHostname = tmpBuf;
    --pHostname;

    /* convert hostname into suitable query format. */
    do {
      ++pHostname;
      nptr = query;
      ++query;
      ++addr1;
      ++addr2;
      for(n = 0; *pHostname != '.' && *pHostname != 0; ++pHostname) {
        *query = *pHostname;
        ++query;
        ++addr1;
	    ++addr2;
        ++n;
      }
      *nptr = n;
    } while(*pHostname != 0);
    *query++='\0';
    length = sizeof(MDNS_LOCAL);
    addr1-=length;
    length = sizeof(PUCK_SERVICE);
    addr2-=length;

    ans.type   = htons(DNS_RRTYPE_SRV);
	ans.class  = htons(DNS_RRCLASS_FLUSH_IN);
	ans.ttl    = htonl(MDNS_SERVICE_TIME);
	strcpy(tmpBuf, host_name);
	strcat(tmpBuf, ".");
	strcat(tmpBuf, MDNS_LOCAL);
	length     = strlen(tmpBuf) + MDNS_LENGTH_ADD;
	ans.len    = htons(SIZEOF_MDNS_SERVICE + length);
	length     = 0;
	MEMCPY( query, &ans, SIZEOF_DNS_ANSWER);

	/* resize the query */
	query = query + SIZEOF_DNS_ANSWER;

	serv.prior  = htons(0);
	serv.weight = htons(0);
	serv.port   = htons(PUCK_PORT);
	MEMCPY( query, &serv, SIZEOF_MDNS_SERVICE);
	/* resize the query */
	query = query + SIZEOF_MDNS_SERVICE;

	pHostname = tmpBuf;
	--pHostname;
	do {
	      ++pHostname;
	      nptr = query;
	      ++query;
	      for(n = 0; *pHostname != '.' && *pHostname != 0; ++pHostname) {
	        *query = *pHostname;
	        ++query;
	        ++n;
	      }
	      *nptr = n;
	    } while(*pHostname != 0);
	    *query++='\0';
    /* set the name of the authority field.
	 * The same name as the Query using the offset address*/
	*query++= DNS_OFFSET_FLAG;
	*query++= DNS_DEFAULT_OFFSET;

	/* fill the answer */
	ans.type   = htons(DNS_RRTYPE_TXT);
	ans.class  = htons(DNS_RRCLASS_IN);
	ans.ttl    = htonl(MDNS_SERVICE_TIME);
	length     = sizeof(SERVICE_DESCRIPTION);
	ans.len    = htons(length);
	length = 0;
	MEMCPY( query, &ans, SIZEOF_DNS_ANSWER);

	/* resize the query */
	query = query + SIZEOF_DNS_ANSWER;

	pHostname = SERVICE_DESCRIPTION;
	--pHostname;

	/* convert hostname into suitable query format. */
	do {
	  ++pHostname;
	  nptr = query;
	  ++query;
	  for(n = 0; *pHostname != '.' && *pHostname != 0; ++pHostname) {
		*query = *pHostname;
		++query;
		++n;
	  }
	  *nptr = n;
	} while(*pHostname != 0);

	pHostname = host_name;
	--pHostname;
	do {
		  ++pHostname;
		  nptr = query;
		  ++query;
		  for(n = 0; *pHostname != '.' && *pHostname != 0; ++pHostname) {
			*query = *pHostname;
			++query;
			++n;
		  }
		  *nptr = n;
		} while(*pHostname != 0);
	/* set the name of the authority field.
	 * The same name as the Query using the offset address*/
	*query++= DNS_OFFSET_FLAG;
	*query++=addr1;
	ans.type   = htons(DNS_RRTYPE_A);
	ans.class  = htons(DNS_RRCLASS_IN);
	ans.ttl    = htonl(MDNS_HOST_TIME);
	ans.len    = htons(DNS_IP_ADDR_LEN);


	MEMCPY( query, &ans, SIZEOF_DNS_ANSWER);

	/* resize the query */
	query = query + SIZEOF_DNS_ANSWER;

	/* fill the payload of the mDNS message */
	/* set the local IP address */
	auth.src    = host_addr.addr; //ipAddr;
	MEMCPY( query, &auth, SIZEOF_MDNS_AUTH);
	/* resize the query */
	query = query + SIZEOF_MDNS_AUTH;

	/* set the name of the authority field.
	 * The same name as the Query using the offset address*/
	*query++= DNS_OFFSET_FLAG;
	*query++=addr2;
	ans.type   = htons(DNS_RRTYPE_PTR);
	ans.class  = htons(DNS_RRCLASS_IN);
	ans.ttl    = htonl(MDNS_SERVICE_TIME);
	strcpy(tmpBuf, name);
	strcat(tmpBuf, ".");
	strcat(tmpBuf, PUCK_SERVICE);
	length     = strlen(tmpBuf) + MDNS_LENGTH_ADD;
	ans.len    = htons(length);
	length     = 0;

	MEMCPY( query, &ans, SIZEOF_DNS_ANSWER);

	/* resize the query */
	query = query + SIZEOF_DNS_ANSWER;

	pHostname = tmpBuf;
	--pHostname;
	/* convert hostname into suitable query format. */
	do {
	  ++pHostname;
	  nptr = query;
	  ++query;
	  for(n = 0; *pHostname != '.' && *pHostname != 0; ++pHostname) {
		*query = *pHostname;
		++query;
		++n;
	  }
	  *nptr = n;
	} while(*pHostname != 0);
	*query++='\0';

    /* resize pbuf to the exact dns query */
    pbuf_realloc(p, (query ) - ((char*)(p->payload)));

    /* send dns packet */
    err = udp_sendto(mdns_pcb, p, &multicast_addr, DNS_MDNS_PORT);

    /* free pbuf */
    pbuf_free(p);
  } else {
    err = ERR_MEM;
  }

  return err;
}

/**
 * Receive input function for DNS response packets arriving for the dns UDP pcb.
 *
 * @params see udp.h
 */
static void
mdns_recv(void *arg, struct udp_pcb *pcb, struct pbuf *p, struct ip_addr *addr, u16_t port)
{
  u8_t i;
  struct mdns_hdr *hdr;
  u8_t nquestions;

  LWIP_UNUSED_ARG(arg);
  LWIP_UNUSED_ARG(pcb);
  LWIP_UNUSED_ARG(addr);
  LWIP_UNUSED_ARG(port);

  /* is the dns message too big ? */
  if (p->tot_len > DNS_MSG_SIZE) {
    LWIP_DEBUGF(DNS_DEBUG, ("dns_recv: pbuf too big\n"));
    /* free pbuf and return */
    goto memerr1;
  }

  /* is the dns message big enough ? */
  if (p->tot_len < (SIZEOF_DNS_HDR + SIZEOF_DNS_QUERY + SIZEOF_DNS_ANSWER)) {
    LWIP_DEBUGF(DNS_DEBUG, ("dns_recv: pbuf too small\n"));
    /* free pbuf and return */
    goto memerr1;
  }

  /* copy dns payload inside static buffer for processing */
  if (pbuf_copy_partial(p, mdns_payload, p->tot_len, 0) == p->tot_len) {
    /* The ID in the DNS header should be our entry into the name table. */

    hdr = (struct mdns_hdr*)mdns_payload;

    i = htons(hdr->id);
    if (i < DNS_TABLE_SIZE) {

    	nquestions = htons(hdr->numquestions);
    	//nanswers   = htons(hdr->numanswers);
    	/* if we have a question send an answer if necessary */
      if(nquestions > 0) {
        /* MDNS_DS_DOES_NAME_CHECK */
        /* Check if the name in the "question" part match with the name of the MDNS DS service. */
        if (mdns_compare_name((unsigned char *)DNS_SD_SERVICE, (unsigned char *)mdns_payload + SIZEOF_DNS_HDR) == 0) {
            /* respond with the puck service*/
        	mdns_answer(MDNS_SD_ANSWER, PUCK_SERVICE, 0);
        } else
        if (mdns_compare_name((unsigned char *)PUCK_SERVICE, (unsigned char *)mdns_payload + SIZEOF_DNS_HDR) == 0) {
			/* respond with the puck service*/
        	mdns_send_service(service_name, 0);
		} else goto memerr2;
      }
    }
  }
  goto memerr2;
memerr2:
  mem_free(mdns_payload);
memerr1:
  /* free pbuf */
  pbuf_free(p);
  return;
}

void mdns_serv_reg()
{
	static char tmpBuf[PUCK_DATASHEET_SIZE+PUCK_SERVICE_LENGTH];

	strcpy(tmpBuf, host_name);
	strcat(tmpBuf, ".");
	strcat(tmpBuf, PUCK_SERVICE);
	  /* Send the puck service registration of the system with delay between */
	  for(int i = 0; i<3 ;i++){
		  mdns_register(MDNS_SERVICE_REG,tmpBuf, 0);
		  Delay(1);
		  //SysCtlDelay(4000000);
	  }
	  /* Send the puck service response
	   * This is the first time that puck service is registered for this instrument*/
	  for(int i = 0; i<2 ;i++){
		  mdns_answer(MDNS_SERVICE_REG_ANSWER, service_name, 0);
		  Delay(1);
		  		  //SysCtlDelay(4000000);
	  }
}

void mdns_reg(void)
{
	static char tmpBuf[PUCK_DATASHEET_SIZE+PUCK_SERVICE_LENGTH];

	strcpy(tmpBuf, host_name);
	strcat(tmpBuf, ".");
	strcat(tmpBuf, MDNS_LOCAL);

	 /* initialize MDNS registration and puck service registration
	   * Send the MDNS registration of the system with delay between */
	  for(int i = 0; i<3 ;i++){
		  mdns_register(MDNS_HOSTNAME_REG, tmpBuf, 0);
		  Delay(1);
		  		  //SysCtlDelay(4000000);
	  }
	  /* Send the MDNS response if no other system with the same name was received */
	  for(int i = 0; i<2 ;i++){
		  mdns_answer(MDNS_REG_ANSWER, tmpBuf, 0);
		  Delay(1);
		  		  //SysCtlDelay(4000000);
	  }
	  /*
	   * Register the PUCK service
	   */
	  mdns_serv_reg();
}
/**
 * close the UDP pcb .
 */
void mdns_close()
{
	if(mdns_pcb != NULL)
	udp_remove(mdns_pcb);
}
/**
 * close the UDP pcb .
 */
void mdns_set_name(const char *name)
{
	//strcpy(host_name, name);
	strcpy(service_name, name);
}

/**
 * Initialize the resolver: set up the UDP pcb and configure the default server
 * (NEW IP).
 */
void mdns_init(unsigned long ipAddr)
{

  /* initialize default DNS server address */
  multicast_addr.addr = DNS_MULTICAST_ADDRESS;
  host_addr.addr      = ipAddr;

  LWIP_DEBUGF(DNS_DEBUG, ("dns_init: initializing\n"));
  //get the datasheet from PUCK
  MEMCPY( puck_datasheet, fs_read_datasheet(), PUCK_DATASHEET_SIZE);
  // get the host name as instrumentName_serialNumber for MDNS
  get_host_name(host_name,puck_datasheet);
  // set the name of the service, the same as host name
  mdns_set_name(host_name);
  /* initialize mDNS */
  mdns_pcb = udp_new();

  if (mdns_pcb != NULL) {
		/* join to the multicast address 224.0.0.251 */
		if(igmp_joingroup(&host_addr, &multicast_addr)!= ERR_OK) {
			//UARTprintf("udp_join_multigrup failed!\n");
			return;
		};
		/* join to any IP address at the port 5353 */
		if(udp_bind(mdns_pcb, IP_ADDR_ANY, DNS_MDNS_PORT)!= ERR_OK) {
			//UARTprintf("udp_bind failed!\n");
			return;
		};

		/*loopback function for the multicast(224.0.0.251) messages received at port 5353*/
		udp_recv(mdns_pcb, mdns_recv, NULL);
        /*
         * Register the name of the instrument
         */
		mdns_reg();
  }
}

//#endif /* LWIP_MDNS */
