/*
 * GPIOs_intr.h
 *
 *  Created on: 26 янв. 2016 г.
 *      Author: PVV
 */

#ifndef _GPIOS_INTR_H_
#define _GPIOS_INTR_H_

#define GPIOs_intr_TASK_PRIO (USER_TASK_PRIO_1)
#define GPIOs_intr_TASK_QUEUE_LEN 2
#define GPIOs_intr_SIG_SAVE 1

void init_GPIOs_intr(void);
extern uint32 GPIO_INT_Count1, GPIO_INT_Count2;
extern uint32 GPIO_INT_Counter1, GPIO_INT_Counter2;

#endif /* _GPIOS_INTR_H_ */
