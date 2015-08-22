/*
 * ets_run_new.h
 *
 *  Created on: 22 авг. 2015 г.
 *      Author: PVV
 */

#ifndef _SDK_ETS_RUN_NEW_H_
#define _SDK_ETS_RUN_NEW_H_

#include "sdk/sdk_config.h"
#ifdef USE_ETS_RUN_NEW

void ets_run_new(void);
void task_delay_us(uint32 us);
void run_sdk_tasks(void);

#endif

#endif /* _SDK_ETS_RUN_NEW_H_ */
