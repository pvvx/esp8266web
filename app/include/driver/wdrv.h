/*
 * wdrv.h
 *
 *  Created on: 02 июля 2015 г.
 *      Author: PVV
 */

#ifndef _INCLUDE_WDRV_H_
#define _INCLUDE_WDRV_H_


#define WDRV_TASK_QUEUE_LEN 2
#define WDRV_TASK_PRIO USER_TASK_PRIO_0
#define WDRV_SIG_DATA 1
#define WDRV_SIG_INIT 2
#define WDRV_SIG_STOP 3
#define WDRV_SIG_START 4

#define WDRV_OUT_BUF_SIZE  1024 // max MSS (1460)

#define DEFAULT_SAMPLE_RATE_HZ 1000 // min 0 max 200000

void init_wdrv(void);

// system_os_post(WDRV_TASK_PRIO, WDRV_SIG_INIT, portn); // if portn == 0 -> wdrv_close
// system_os_post(WDRV_TASK_PRIO, WDRV_SIG_START, sample_rate); // sample_rate = min 1 max 200000 Hz, if sample_rate = 0 -> DEFAULT_SAMPLE_RATE_HZ
// system_os_post(WDRV_TASK_PRIO, WDRV_SIG_STOP, 0);


#endif /* _INCLUDE_WDRV_H_ */
