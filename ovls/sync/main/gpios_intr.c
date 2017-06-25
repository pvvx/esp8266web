/*
 * 	gpio_isr.c
 *	По просьбе трудящихся.
 *      Author: pvvx
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
#define GPIO_INTR_INIT mdb_buf.ubuf[81] // флаг =1 - драйвер работает, =0 остановлен
#define GPIO_INT_Count1 mdb_buf.ubuf[82]
#define GPIO_TFS0 mdb_buf.ubuf[83] // время последнего INT
#define GPIO_TFS1 mdb_buf.ubuf[84] // время последнего INT
#define GPIO_TFS2 mdb_buf.ubuf[85] // время последнего INT
#define GPIO_TFS3 mdb_buf.ubuf[86] // время последнего INT


uint32 GPIO_INT_init_flg DATA_IRAM_ATTR;

#define GPIOs_intr_TASK_PRIO (USER_TASK_PRIO_1)
#define GPIOs_intr_TASK_QUEUE_LEN 2
#define GPIOs_intr_SIG_DELTA 7

ETSEvent GPIOs_intr_taskQueue[GPIOs_intr_TASK_QUEUE_LEN] DATA_IRAM_ATTR;
volatile uint32 old_tsf DATA_IRAM_ATTR;
extern sint32 delta_mac;

void ICACHE_FLASH_ATTR GPIOs_intr_handler(void)
{
//	ets_intr_lock();
	MEMW();
	uint32 gpio_status = GPIO_STATUS;
	GPIO_STATUS_W1TC = gpio_status;

	if(gpio_status & (1<<GPIO_TEST1)) {
//		uint32 cur_tsf = get_tsf_station(); // system_get_time();
//		cur_tsf += recv_tsf;
		MEMW();
/*		if(old_tsf != recv_tsf) {
			system_os_post(GPIOs_intr_TASK_PRIO, GPIOs_intr_SIG_DELTA, delta_mac);
			old_tsf = recv_tsf;
		} */
		uint32 cur_tsf = recv_tsf + *((volatile uint32 *)MAC_TIMER64BIT_COUNT_ADDR);
		system_os_post(GPIOs_intr_TASK_PRIO, GPIOs_intr_SIG_DELTA, cur_tsf - old_tsf);
		old_tsf = cur_tsf;
//		uint32 cur_tsf =  delta_mac; //
#if 0
		union {
			uint8 b[8];
			uint16 w[4];
			uint32 d[2];
			uint64 q;
		}x;
		x.q = get_tsf_station();
		GPIO_TFS0 = x.w[0];
		GPIO_TFS1 = x.w[1];
		GPIO_TFS2 = x.w[2];
		GPIO_TFS3 = x.w[3];
#endif
//		GPIO_INT_Count1++;


	    gpio_pin_intr_state_set(GPIO_TEST1, GPIO_PIN_INTR_NEGEDGE); // GPIO_PIN_INTR_ANYEDGE | GPIO_PIN_INTR_POSEDGE | GPIO_PIN_INTR_NEGEDGE ?
	}
//	else ets_intr_unlock();
}

void ICACHE_FLASH_ATTR task_GPIOs(os_event_t *e)
{
    if(e->sig == GPIOs_intr_SIG_DELTA) os_printf("%u,%u\n", e->par, (uint32)recv_tsf);
}

void ICACHE_FLASH_ATTR init_GPIOs_intr(void)
{
	system_os_task(task_GPIOs, GPIOs_intr_TASK_PRIO, GPIOs_intr_taskQueue, GPIOs_intr_TASK_QUEUE_LEN);
	if(GPIO_TEST1 > 15) {
		GPIO_TEST1 = 4;
	}
	uint32 pins_mask = (1<<GPIO_TEST1);
	gpio_output_set(0,0,0, pins_mask); // настроить GPIOx на ввод
	set_gpiox_mux_func_ioport(GPIO_TEST1); // установить функцию GPIOx в режим порта i/o
	ets_isr_attach(ETS_GPIO_INUM, GPIOs_intr_handler, NULL);
    gpio_pin_intr_state_set(GPIO_TEST1, GPIO_PIN_INTR_NEGEDGE); // GPIO_PIN_INTR_ANYEDGE | GPIO_PIN_INTR_POSEDGE | GPIO_PIN_INTR_NEGEDGE ?
    // разрешить прерывания GPIOs
	GPIO_STATUS_W1TC = pins_mask;
	ets_isr_unmask(1 << ETS_GPIO_INUM);
	GPIO_INTR_INIT = 1;
	GPIO_INT_init_flg = 1;
	os_printf("GPIOs_intr init (%d) ", GPIO_TEST1);
}

void ICACHE_FLASH_ATTR deinit_GPIOs_intr(void)
{
	if(GPIO_INT_init_flg) {
		ets_isr_mask(1 << ETS_GPIO_INUM); // запрет прерываний GPIOs
	    gpio_pin_intr_state_set(GPIO_TEST1, GPIO_PIN_INTR_DISABLE);
		GPIO_INTR_INIT = 0;
		GPIO_INT_init_flg = 0;
		os_printf("GPIOs_intr deinit (%d) ", GPIO_TEST1);
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

