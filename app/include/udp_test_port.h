#ifndef __UDP_TSET_PORT_H__
#define __UDP_TSET_PORT_H__

#include "user_config.h"

#ifdef UDP_TEST_PORT
#define DEFAULT_UDP_TEST_PORT UDP_TEST_PORT // 1025
void udp_test_port_init(uint16 portn); // portn = 0 -> Close
#endif

#endif // __UDP_TSET_PORT_H__
