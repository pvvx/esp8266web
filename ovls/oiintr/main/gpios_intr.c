/*
 * 	gpio_isr.c
 *	По просьбе трудящихся.
 *      Author: PV`
 */
#include "user_config.h"
#include "bios.h"
#include "hw/esp8266.h"
#include "hw/eagle_soc.h"
#include "user_interface.h"
#include "flash_eep.h"
#include "web_iohw.h"
#include "gpios_intr.h"

#include "ovl_sys.h"

#define GPIO_TEST1 mdb_buf.ubuf[80] // по умолчанию GPIO4
#define GPIO_TEST2 mdb_buf.ubuf[81] // по умолчанию GPIO5
#define GPIO_INTR_INIT mdb_buf.ubuf[82] // флаг =1 - драйвер работает, =0 остановлен


ETSEvent GPIOs_intr_taskQueue[GPIOs_intr_TASK_QUEUE_LEN] DATA_IRAM_ATTR;

uint32 GPIO_INT_init_flg DATA_IRAM_ATTR;
uint32 GPIO_INT_Count1, GPIO_INT_Count2 DATA_IRAM_ATTR;
uint32 GPIO_INT_Counter1, GPIO_INT_Counter2;

/* ТЗ :) из темы http://esp8266.ru/forum/threads/ne-poluchaetsja-sobrat-proekty-ot-pvvx-pod-linux.926/#post-14214
 Counter1 и Counter2 доступны, как вэб переменные sys_ucnst_1 и sys_ucnst_2

При старте считать Counter1 и Counter2 из EEPROM (sys_ucnst_1 и sys_ucnst_2).
Приаттачить прерывание на GPIO 4
Приаттачить прерывание на GPIO 5

Обработчик прерывания по фронту на GPIO 4
     Count1++;
     if (Count1 == 10) {
         Count1=0;
         Counter1++;
         Сохранить значение Counter1 в EEPROM (sys_ucnst_1);
      }

Обработчик прерывания по фронту на GPIO 5
     Count2++;
     if (Count2 == 10) {
         Count2=0;
         Counter2++;
         Сохранить значение Counter2 в EEPROM (sys_ucnst_2);
      }
*/

static void GPIOs_intr_handler(void)
{
	uint32 gpio_status = GPIO_STATUS;
	GPIO_STATUS_W1TC = gpio_status;
	if(gpio_status & (1<<GPIO_TEST1)) {
		if(++GPIO_INT_Count1 > 10) {
			GPIO_INT_Count1 = 0;
			system_os_post(GPIOs_intr_TASK_PRIO, GPIOs_intr_SIG_SAVE, 1);
		}
	    gpio_pin_intr_state_set(GPIO_TEST1, GPIO_PIN_INTR_ANYEDGE); // GPIO_PIN_INTR_POSEDGE | GPIO_PIN_INTR_NEGEDGE ?
	}
	if(gpio_status & (1<<GPIO_TEST2)) {
		if(++GPIO_INT_Count2 > 10) {
			GPIO_INT_Count2 = 0;
			system_os_post(GPIOs_intr_TASK_PRIO, GPIOs_intr_SIG_SAVE, 2);
		}
	    gpio_pin_intr_state_set(GPIO_TEST2, GPIO_PIN_INTR_ANYEDGE); // GPIO_PIN_INTR_POSEDGE | GPIO_PIN_INTR_NEGEDGE ?
	}
}

static void ICACHE_FLASH_ATTR task_GPIOs_intr(os_event_t *e)
{
    switch(e->sig) {
    	case GPIOs_intr_SIG_SAVE:
    		if(e->par == 1) {
        		write_user_const(1, ++GPIO_INT_Counter1); // Запись пользовательских констант (0 < idx < 4)
    		}
    		else if(e->par == 2) {
        		write_user_const(2, ++GPIO_INT_Counter2); // Запись пользовательских констант (0 < idx < 4)
    		}
    		break;
    }
}

void ICACHE_FLASH_ATTR init_GPIOs_intr(void)
{
	if(GPIO_TEST1 == GPIO_TEST2 || GPIO_TEST1 > 15 || GPIO_TEST1 > 15) {
		GPIO_TEST1 = 4;
		GPIO_TEST2 = 5;
	}
//	ets_isr_mask(1 << ETS_GPIO_INUM); // запрет прерываний GPIOs
	// чтение пользовательских констант (0 < idx < 4) из записей в Flash
	GPIO_INT_Counter1 = read_user_const(1);
	GPIO_INT_Counter2 = read_user_const(2);

	system_os_task(task_GPIOs_intr, GPIOs_intr_TASK_PRIO, GPIOs_intr_taskQueue, GPIOs_intr_TASK_QUEUE_LEN);

	uint32 pins_mask = (1<<GPIO_TEST1) | (1<<GPIO_TEST2);
	gpio_output_set(0,0,0, pins_mask); // настроить GPIOx на ввод
	set_gpiox_mux_func_ioport(GPIO_TEST1); // установить функцию GPIOx в режим порта i/o
	set_gpiox_mux_func_ioport(GPIO_TEST2); // установить функцию GPIOx в режим порта i/o
//	GPIO_ENABLE_W1TC = pins_mask; // GPIO OUTPUT DISABLE отключить вывод в портах
	ets_isr_attach(ETS_GPIO_INUM, GPIOs_intr_handler, NULL);
    gpio_pin_intr_state_set(GPIO_TEST1, GPIO_PIN_INTR_ANYEDGE); // GPIO_PIN_INTR_POSEDGE | GPIO_PIN_INTR_NEGEDGE ?
    gpio_pin_intr_state_set(GPIO_TEST2, GPIO_PIN_INTR_ANYEDGE); // GPIO_PIN_INTR_POSEDGE | GPIO_PIN_INTR_NEGEDGE ?
    // разрешить прерывания GPIOs
	GPIO_STATUS_W1TC = pins_mask;
	ets_isr_unmask(1 << ETS_GPIO_INUM);
	GPIO_INTR_INIT = 1;
	GPIO_INT_init_flg = 1;
	os_printf("GPIOs_intr init (%d,%d) ", GPIO_TEST1, GPIO_TEST2);
}

void ICACHE_FLASH_ATTR deinit_GPIOs_intr(void)
{
	if(GPIO_INT_init_flg) {
		ets_isr_mask(1 << ETS_GPIO_INUM); // запрет прерываний GPIOs
	    gpio_pin_intr_state_set(GPIO_TEST1, GPIO_PIN_INTR_DISABLE);
	    gpio_pin_intr_state_set(GPIO_TEST2, GPIO_PIN_INTR_DISABLE);
//	    ets_isr_attach(ETS_GPIO_INUM, NULL, NULL);
		GPIO_INTR_INIT = 0;
		GPIO_INT_init_flg = 0;
		os_printf("GPIOs_intr deinit (%d,%d) ", GPIO_TEST1, GPIO_TEST2);
	}
}
//=============================================================================
//=============================================================================
int ovl_init(int flg)
{
	if(flg == 1) {
		if(GPIO_INT_init_flg) deinit_GPIOs_intr();
		init_GPIOs_intr();
		return 0;
	}
	else {
		deinit_GPIOs_intr();
		return 0;
	}
}


