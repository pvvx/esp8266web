/******************************************************************************
 * FileName: gpio_bios.h
 * Description: GPIO funcs in ROM-BIOS
 * Alternate SDK
 * Author: PV`
 * (c) PV` 2015
*******************************************************************************/
#ifndef _GPIO_H_
#define _GPIO_H_

#include "c_types.h"

#define GPIO_PIN_ADDR(i) (GPIO_PIN0_ADDRESS + i*4)

#define GPIO_ID_IS_PIN_REGISTER(reg_id) \
    ((reg_id >= GPIO_ID_PIN0) && (reg_id <= GPIO_ID_PIN(GPIO_PIN_COUNT-1)))

#define GPIO_REGID_TO_PINIDX(reg_id) ((reg_id) - GPIO_ID_PIN0)

typedef enum {
    GPIO_PIN_INTR_DISABLE = 0,
    GPIO_PIN_INTR_POSEDGE = 1,
    GPIO_PIN_INTR_NEGEDGE = 2,
    GPIO_PIN_INTR_ANYEDGE = 3,
    GPIO_PIN_INTR_LOLEVEL = 4,
    GPIO_PIN_INTR_HILEVEL = 5
} GPIO_INT_TYPE;

#define GPIO_OUTPUT_SET(gpio_no, bit_value) \
    gpio_output_set(bit_value<<gpio_no, ((~bit_value)&0x01)<<gpio_no, 1<<gpio_no,0)
#define GPIO_DIS_OUTPUT(gpio_no) 	gpio_output_set(0,0,0, 1<<gpio_no)
#define GPIO_INPUT_GET(gpio_no)     ((gpio_input_get()>>gpio_no)&BIT0)

/* GPIO interrupt handler, registered through gpio_intr_handler_register */
typedef void (* gpio_intr_handler_fn_t)(uint32 intr_mask, void *arg);


/*
 * Initialize GPIO.  This includes reading the GPIO Configuration DataSet
 * to initialize "output enables" and pin configurations for each gpio pin.
 * Must be called once during startup.
 */
void gpio_init(void);

/*
 * Change GPIO pin output by setting, clearing, or disabling pins.
 * In general, it is expected that a bit will be set in at most one
 * of these masks.  If a bit is clear in all masks, the output state
 * remains unchanged.
 *
 * There is no particular ordering guaranteed; so if the order of
 * writes is significant, calling code should divide a single call
 * into multiple calls.
 */
void gpio_output_set(uint32 set_mask,
                     uint32 clear_mask,
                     uint32 enable_mask,
                     uint32 disable_mask);

/*
 * Sample the value of GPIO input pins and returns a bitmask.
 */
uint32 gpio_input_get(void);

/*
 * Set the specified GPIO register to the specified value.
 * This is a very general and powerful interface that is not
 * expected to be used during normal operation.  It is intended
 * mainly for debug, or for unusual requirements.
 */
void gpio_register_set(uint32 reg_id, uint32 value);

/* Get the current value of the specified GPIO register. */
uint32 gpio_register_get(uint32 reg_id);

/*
 * Register an application-specific interrupt handler for GPIO pin
 * interrupts.  Once the interrupt handler is called, it will not
 * be called again until after a call to gpio_intr_ack.  Any GPIO
 * interrupts that occur during the interim are masked.
 *
 * The application-specific handler is called with a mask of
 * pending GPIO interrupts.  After processing pin interrupts, the
 * application-specific handler may wish to use gpio_intr_pending
 * to check for any additional pending interrupts before it returns.
 */
void gpio_intr_handler_register(gpio_intr_handler_fn_t fn, void *arg);

/* Determine which GPIO interrupts are pending. */
uint32 gpio_intr_pending(void); // return *((void *)(0x3FFFDFD0));

/*
 * Acknowledge GPIO interrupts.
 * Intended to be called from the gpio_intr_handler_fn.
 */
void gpio_intr_ack(uint32 ack_mask);

void gpio_pin_wakeup_enable(uint32 i, GPIO_INT_TYPE intr_state);

void gpio_pin_wakeup_disable();

void gpio_pin_intr_state_set(uint32 i, GPIO_INT_TYPE intr_state);

/*
 PROVIDE ( gpio_init = 0x40004c50 );
 PROVIDE ( gpio_input_get = 0x40004cf0 );
 PROVIDE ( gpio_intr_ack = 0x40004dcc );
 PROVIDE ( gpio_intr_handler_register = 0x40004e28 );
 PROVIDE ( gpio_intr_pending = 0x40004d88 );
 PROVIDE ( gpio_intr_test = 0x40004efc );
 PROVIDE ( gpio_output_set = 0x40004cd0 );
 PROVIDE ( gpio_pin_intr_state_set = 0x40004d90 );
 PROVIDE ( gpio_pin_wakeup_disable = 0x40004ed4 );
 PROVIDE ( gpio_pin_wakeup_enable = 0x40004e90 );
 PROVIDE ( gpio_register_get = 0x40004d5c );
 PROVIDE ( gpio_register_set = 0x40004d04 );
*/

void gpio_intr_test(uint32 ack_mask); /* {
	if(gpio_input_get() & 1<<3) gpio_output_set(2, 0x7C, 3, 0);
	else gpio_output_set(0, 0x7E, 2, 0)
	gpio_intr_ack(ack_mask); } */

#endif // _GPIO_H_



