/******************************************************************************
 * FileName: ets_run_new.c
 * Description: Alternate SDK
 * (c) PV` 2015
 ******************************************************************************/

#include "sdk/sdk_config.h"
#ifdef USE_ETS_RUN_NEW
#include "bios.h"
#include "hw/esp8266.h"
#include "hw/spi_register.h"
#include "sdk/add_func.h"

#define SDK_TASKS_WAIT  20480 // 20 ms. Максимальное время между запусками обработки задач SDK/WiFi при использовании run_sdk_tasks() в цикле
#define SDK_TASKS_RUNT  1024 // 1 ms. Время для обработки задач SDK/WiFi при использования run_sdk_tasks() в цикле
#define MIN_DELAY_US    512 // 512 us. Минимальное время для отработки task_delay_us(us), если меньше то вызывается ets_delay_us(us);


uint32 old_task_time DATA_IRAM_ATTR; // штамп времени поледнего запуска обработки задач SDK/WiFi
volatile uint32 ets_run_ret DATA_IRAM_ATTR; // флаг выхода из ets_run_new()

void ICACHE_IRAM_ATTR ets_run_new(void) {
	uint8 bnum;
	uint8 bctp = ets_bit_task_priority = 0;
	while (1) {
		while (1) {
			ets_intr_lock();
			bnum = ets_bit_count_task;
			asm volatile ("nsau %0, %1;" :"=r"(bnum) : "r"(bnum)); // Normalization Shift Amount Unsigned
			bnum = 32 - bnum;
			if (bctp < bnum)
				break;
			if (SPI0_CTRL & SPI_ENABLE_AHB) { // 'cache flash' включена? Можно вызывать callbacks.
				if (ets_idle_cb != NULL)
					ets_idle_cb(ets_idle_arg);
				else if (ets_run_ret) {
					ets_run_ret = 0;
					ets_intr_unlock();
					return;
				}
			}
			asm volatile ("waiti 0;"); // Wait for Interrupt
			ets_intr_unlock();
		};
		ss_task * cur_task = &ets_tab_task[bnum - 1];
		sargc_tsk * argc = &cur_task->argc[cur_task->cnts++];
		if (cur_task->size == cur_task->cnts)
			cur_task->cnts = 0;
		if (--cur_task->cnte == 0) {
			ets_bit_count_task &= ~cur_task->bitn;
		}
		ets_bit_task_priority = bnum;
		ets_intr_unlock();
		cur_task->func(argc);
		ets_bit_task_priority = bctp;
	};
}

void delay_wait_cb(void *px) {
	old_task_time = phy_get_mactime();
	ets_run_ret = 1;
}

void ICACHE_FLASH_ATTR task_delay_us(uint32 us) {
	if (us < MIN_DELAY_US)
		ets_delay_us(us);
	else {
		ETSTimer delay_timer;
		ets_timer_disarm(&delay_timer);
		ets_timer_setfn(&delay_timer, (ETSTimerFunc *) (delay_wait_cb), NULL);
		ets_timer_arm_new(&delay_timer, us - 128, 0, 0);
		ets_run_new();
	}
}

void ICACHE_FLASH_ATTR run_sdk_tasks(void) {
	uint32 t = phy_get_mactime();
	if (t - old_task_time > SDK_TASKS_WAIT) {
		task_delay_us(SDK_TASKS_RUNT);
	}
}

#endif // USE_ETS_RUN_NEW
