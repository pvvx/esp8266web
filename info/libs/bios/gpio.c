/******************************************************************************
 * FileName: gpio.c
 * Description: gpio funcs in ROM-BIOS
 * Alternate SDK ver 0.0.0 (b1)
 * Author: PV`
 * (c) PV` 2015
*******************************************************************************/
#include "c_types.h"
#include "bios.h"
#include "hw/esp8266.h"

// ROM:40004D90
void gpio_pin_intr_state_set(uint32 i, GPIO_INT_TYPE intr_state)
{
	ets_intr_lock();
	volatile uint32 * gpio_pinx = &GPIO_PIN0;
	uint32 x = gpio_pinx[i] & 0xC7F;
	gpio_pinx[i] = x | intr_state << 7;
	ets_intr_unlock();
}
