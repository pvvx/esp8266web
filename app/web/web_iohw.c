/******************************************************************************
 * FileName: web_iohw.c
 * Description: Small WEB server
 * Author: PV`
 ******************************************************************************/
#include "user_config.h"
#include "bios.h"
#include "hw/esp8266.h"
#include "hw/eagle_soc.h"
#include "hw/uart_register.h"
#include "sdk/add_func.h"
#include "ets_sys.h"
#include "osapi.h"
#include "flash_eep.h"
#include "web_iohw.h"
#include "tcp2uart.h"
//=============================================================================
// get_addr_gpiox_mux(pin_num)
//-----------------------------------------------------------------------------
volatile uint32 * ICACHE_FLASH_ATTR get_addr_gpiox_mux(uint8 pin_num)
{
	return &GPIOx_MUX(pin_num & 0x0F);
}
//=============================================================================
// get_gpiox_mux(pin_num)
//-----------------------------------------------------------------------------
uint32 ICACHE_FLASH_ATTR get_gpiox_mux(uint8 pin_num)
{
	return *(get_addr_gpiox_mux(pin_num));
}
//=============================================================================
// set_gpiox_mux_func(pin_num, func)
//-----------------------------------------------------------------------------
void ICACHE_FLASH_ATTR set_gpiox_mux_func(uint8 pin_num, uint8 func)
{
	volatile uint32 *goio_mux = get_addr_gpiox_mux(pin_num); // volatile uint32 *goio_mux = &GPIOx_MUX(PIN_NUM)
	*goio_mux = (*goio_mux & (~GPIO_MUX_FUN_MASK)) | ((((func & 7) + 0x0C) & 0x013) << GPIO_MUX_FUN_BIT0);
}
//=============================================================================
// get_gpiox_mux_func(pin_num, func)
//-----------------------------------------------------------------------------
uint32 ICACHE_FLASH_ATTR get_gpiox_mux_func(uint8 pin_num)
{
	uint32 io_mux = get_gpiox_mux(pin_num); // volatile uint32 *goio_mux = &GPIOx_MUX(PIN_NUM)
	return (((io_mux >> GPIO_MUX_FUN_BIT0) & 3) | ((io_mux >> (GPIO_MUX_FUN_BIT2 - 2)) & 4));
}
//=============================================================================
// set_gpiox_mux_pull(pin_num, pull)
//-----------------------------------------------------------------------------
void ICACHE_FLASH_ATTR set_gpiox_mux_pull(uint8 pin_num, uint8 pull)
{
	volatile uint32 *goio_mux = get_addr_gpiox_mux(pin_num); // volatile uint32 *goio_mux = &GPIOx_MUX(PIN_NUM)
	*goio_mux = (*goio_mux & (~(3 << GPIO_MUX_PULLDOWN_BIT))) | ((pull & 3) << GPIO_MUX_PULLDOWN_BIT);
}
//=============================================================================
// set_gpiox_mux_func_ioport(pin_num)
//-----------------------------------------------------------------------------
void ICACHE_FLASH_ATTR set_gpiox_mux_func_ioport(uint8 pin_num)
{
    set_gpiox_mux_func(pin_num, MUX_FUN_IO_PORT(pin_num)); // ((uint32)(_FUN_IO_PORT >> (pin_num << 1)) & 0x03));
}
//=============================================================================
// set_gpiox_mux_func_ioport(pin_num)
//-----------------------------------------------------------------------------
void ICACHE_FLASH_ATTR set_gpiox_mux_func_default(uint8 pin_num)
{
    set_gpiox_mux_func(pin_num, MUX_FUN_DEF_SDK(pin_num));
}
//=============================================================================
// select cpu frequency 80 or 160 MHz
//-----------------------------------------------------------------------------
void ICACHE_FLASH_ATTR set_cpu_clk(void)
{
	ets_intr_lock();
	if(syscfg.cfg.b.hi_speed_enable) {
		Select_CLKx2(); // REG_SET_BIT(0x3ff00014, BIT(0));
		ets_update_cpu_frequency(160);
	}
	else {
		Select_CLKx1(); // REG_CLR_BIT(0x3ff00014, BIT(0));
		ets_update_cpu_frequency(80);
	}
	ets_intr_unlock();
}
//=============================================================================
//  Пристартовый тест пина RX для сброса конфигурации
//=============================================================================

#define GPIO_TEST0 3 // GPIO3 (RX)
#define GPIO_TEST1 13 // GPIO13 (RX)

void GPIO_intr_handler(void * test_edge)
{
	uint32 gpio_status = GPIO_STATUS;
	GPIO_STATUS_W1TC = gpio_status;
	uint32 mask = (PERI_IO_SWAP & PERI_IO_UART0_PIN_SWAP)? (1<<GPIO_TEST1) : (1<<GPIO_TEST0);
	if(gpio_status & mask) *((uint8 *)test_edge) = 1; // test_edge++;
//    gpio_pin_intr_state_set(GPIO_TEST, GPIO_PIN_INTR_ANYEDGE);
}

void ICACHE_FLASH_ATTR test_pin_clr_wifi_config(void)
{
	uint32 x = 0;
	uint8 test_edge = 0;
	uint32 pin_num = (PERI_IO_SWAP & PERI_IO_UART0_PIN_SWAP)? GPIO_TEST1 : GPIO_TEST0;
	uint32 pin_mask = 1<<pin_num;
	if(UART1_CONF0 & UART_RXD_INV) x = pin_mask;
	gpio_output_set(0,0,0, pin_mask);
	uint32 old_ioe = GPIO_ENABLE; // запомнить вход или выход
	GPIO_ENABLE_W1TC = pin_mask; // GPIO OUTPUT DISABLE отключить вывод в порту GPIO3
	uint32 old_mux = get_gpiox_mux(pin_num); // запомнить функцию
	set_gpiox_mux_func_ioport(pin_num); // установить RX (GPIO3) в режим порта i/o
	if((GPIO_IN & pin_mask) == x) {
		ets_isr_mask(1 << ETS_GPIO_INUM);
		ets_isr_attach(ETS_GPIO_INUM, GPIO_intr_handler, (void *)&test_edge);
        gpio_pin_intr_state_set(pin_num, GPIO_PIN_INTR_ANYEDGE);
		GPIO_STATUS_W1TC = pin_mask;
		ets_isr_unmask(1 << ETS_GPIO_INUM);
		ets_delay_us(25000); //25 ms
		ets_isr_mask(1 << ETS_GPIO_INUM);
	    if(test_edge == 0) { // изменений не было
#if DEBUGSOO > 0
			os_printf("WiFi configuration reset\n");
#endif
	    	flash_save_cfg(&x, ID_CFG_WIFI, 0); // создать запись нулевой длины
	//    	flash_save_cfg(&x, ID_CFG_UART0, 0);
	//    	flash_save_cfg(&x, ID_CFG_SYS, 0);
	    }
	}
	if(old_ioe & pin_mask) GPIO_ENABLE_W1TS = pin_mask; // восстановить если был выход
	*get_addr_gpiox_mux(pin_num) = old_mux; // восстановить mux
}

//===============================================================================
// get_mac_time()
//-------------------------------------------------------------------------------
uint64 ICACHE_FLASH_ATTR get_mac_time(void)
{
	union {
		volatile uint32 dw[2];
		uint64 dd;
	}ux;
	volatile uint32 * ptr = (volatile uint32 *)MAC_TIMER64BIT_COUNT_ADDR;
	ux.dw[0] = ptr[0];
	ux.dw[1] = ptr[1];
	if(ux.dw[1] != ptr[1]) {
		ux.dw[0] = ptr[0];
		ux.dw[1] = ptr[1];
	}
	return ux.dd;
}



