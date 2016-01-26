/*
 * 	gpio_isr.c
 *	По просьбе трудящихся.
 *      Author: PV`
 */
#include "user_config.h"
#ifdef USE_GPIOs_intr
#include "bios.h"
#include "hw/esp8266.h"
#include "hw/eagle_soc.h"
#include "user_interface.h"
#include "flash_eep.h"
#include "web_iohw.h"
#include "gpios_intr.h"

ETSEvent GPIOs_intr_taskQueue[GPIOs_intr_TASK_QUEUE_LEN] DATA_IRAM_ATTR;

uint32 GPIO_INT_Count1, GPIO_INT_Count2 DATA_IRAM_ATTR;
uint32 GPIO_INT_Counter1, GPIO_INT_Counter2;

/* Counter1 и Counter2 доступны, как вэб переменные

При старте считать Counter1 и Counter2 из EEPROM.
Приаттачить прерывание на GPIO 4
Приаттачить прерывание на GPIO 5

Обработчик прерывания по фронту на GPIO 4
     Count1++;
     if (Count1 == 10) {
         Count1=0;
         Counter1++;
         Сохранить значение Counter1 в EEPROM;
      }

Обработчик прерывания по фронту на GPIO 5
     Count2++;
     if (Count2 == 10) {
         Count2=0;
         Counter2++;
         Сохранить значение Counter2 в EEPROM;
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
	os_printf("GPIOs_inrt init...\n");
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
}

#endif // USE_GPIOs_intr
