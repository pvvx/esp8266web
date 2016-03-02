/*
 * wdrv.h
 *
 *  Created on: 02 июля 2015 г.
 *      Author: PVV
 */

#ifndef _INCLUDE_WDRV_H_
#define _INCLUDE_WDRV_H_

#ifdef USE_WDRV

#define WDRV_TASK_QUEUE_LEN 3
#if defined(USE_RS485DRV) || defined(USE_TCP2UART)
#define WDRV_TASK_PRIO (USER_TASK_PRIO_1) // + SDK_TASK_PRIO)
#else
#define WDRV_TASK_PRIO (USER_TASK_PRIO_0) // + SDK_TASK_PRIO)
#endif
#define WDRV_SIG_DATA 1
#define WDRV_SIG_INIT 2
#define WDRV_SIG_STOP 3
#define WDRV_SIG_START 4

#define WDRV_OUT_BUF_SIZE  1024 // max MSS (1460)

#define DEFAULT_SAMPLE_RATE_HZ 1000 // min 0 max 200000
#define DEFAULT_WDRV_HOST_IP IPADDR_BROADCAST;
#define DEFAULT_WDRV_HOST_PORT 10201;
#define DEFAULT_WDRV_REMOTE_PORT USE_WDRV; // 10201;

#define MAX_SAMPLE_RATE 192000
#define SAR_SAMPLE_RATE 375000 //3000000/8

void init_wdrv(void);

extern struct udp_pcb *pcb_wdrv; // = NULL -> wdrv закрыт
extern uint32 wdrv_remote_port; // = 0 -> wdrv не работает
extern uint32 wdrv_sample_rate; // по умолчанию = DEFAULT_SAMPLE_RATE_HZ
extern uint16 wdrv_host_port; // по умолчанию = DEFAULT_WDRV_HOST_PORT
extern ip_addr_t wdrv_host_ip; // по умолчанию = DEFAULT_WDRV_HOST_IP

bool wdrv_start(uint32 sample_rate);
bool wdrv_init(uint32 portn);
void wdrv_stop(void);

// system_os_post(WDRV_TASK_PRIO, WDRV_SIG_INIT, portn); // if portn == 0 -> wdrv_close
// system_os_post(WDRV_TASK_PRIO, WDRV_SIG_START, sample_rate); // sample_rate = min 1 max 200000 Hz, if sample_rate = 0 -> DEFAULT_SAMPLE_RATE_HZ
// system_os_post(WDRV_TASK_PRIO, WDRV_SIG_STOP, 0);


#endif // USE_WDRV
#endif /* _INCLUDE_WDRV_H_ */
