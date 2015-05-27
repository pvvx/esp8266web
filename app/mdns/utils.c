/*
 * utils.c
 *
 *  Created on: Jul 29, 2010
 *      Author: Daniel Toma
 */
#include <string.h>
#include "utils/ustdlib.h"
#include "puck_def.h"

char* Itoa(int value, char* str, int radix) {
    static char dig[] =
        "0123456789abcdefghijklmnopqrstuvwxyz";
    int n = 0, neg = 0;
    unsigned int v;
    char* p, *q;
    char c;

    if (radix == 10 && value < 0) {
        value = -value;
        neg = 1;
    }
    v = value;
    do {
        str[n++] = dig[v%radix];
        v /= radix;
    } while (v);
    if (neg)
        str[n++] = '-';
    str[n] = '\0';

    for (p = str, q = p + (n-1); p < q; ++p, --q)
        c = *p, *p = *q, *q = c;
    return str;
}
/**
 * Converts a 4 byte array of unsigned bytes to an long
 * @param b an array of 4 unsigned bytes
 * @return a long representing the unsigned int
 */
static long unsignedIntToLong(char *b)
{

    long l = 0;
	l |= b[0] & 0xFF;
	l <<= 8;
	l |= b[1] & 0xFF;
	l <<= 8;
	l |= b[2] & 0xFF;
	l <<= 8;
	l |= b[3] & 0xFF;
	return l;

}
void get_serial_number(char* serial, char *data)
{

      int count = 0;
      int i;

	  count = 0;

	  for (i = SER_NUM_OFFSET; i<NAME_OFFSET; i++)
	  	  {
		    serial[count] = data[i];
			count++;
	  	  }
}
/**
 * Initialize the resolver: set up the UDP pcb and configure the default server
 * (NEW IP).
 */
void get_host_name(char* hostname, char *data)
{
	  unsigned char c;
      unsigned long serialNumber,len;
      int           i,count = 0;
      char          serial[SER_NUM_LEN];
      char          serialBuf [33];

	  for (i = NAME_OFFSET; i < PUCK_DATASHEET_SIZE; i++)
	  {

		c = data[i];
		if ((c!='\0')&&(c!='"')){
			if(c!=' '){
				hostname[count] = c;
			}else hostname[count] = '_';
		count++;
		}
	  }
	  hostname[count] = '_';
	  count++;
	  get_serial_number(serial, data);
	  serialNumber = unsignedIntToLong(serial);
	  Itoa (serialNumber,serialBuf,10);
      len = serialNumber;
      i = 0;
	  while(len > 0){
		if (serialBuf[i]!='\0'){
		hostname[count] = serialBuf[i];
		count++;
		}
		len = len / 10;
		i++;
	  }
	  hostname[count] = '\0';
}


