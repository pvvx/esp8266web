/******************************************************************************
 * MODULE Name : set_sigma

EACH PIN CAN CONNET TO A SIGMA-DELTA , ALL PINS SHEARS THE SAME SIGMA-DELTA SOURCE.

THE TARGET DUTY AND FREQUENCY CAN BE MODIFIED VIA THE REG ADDR GPIO_SIGMA_DELTA

THE TARGET FREQUENCY IS DEFINED AS:

FREQ = 80,000,000/prescale * target /256  HZ,     0<target<128
FREQ = 80,000,000/prescale * (256-target) /256  HZ,     128<target<256
target: duty ,0-255
prescale: clk_div,0-255
so the target and prescale will both affect the freq.


YOU CAN DO THE TEST LIKE THIS:
1. INIT :  sigma_delta_setup(uint32 GPIO_NUM);
2. USE 312K:   set_sigma_duty_312KHz(2);
3. DEINIT AND DISABLE: sigma_delta_close(uint32 GPIO_NUM), eg.sigma_delta_close(2)

*******************************************************************************/

#include "c_types.h"
#include "hw/esp8266.h"
#include "hw/gpio_register.h"
#include "bios/gpio.h"
#include "web_iohw.h"

/******************************************************************************
 * FunctionName : sigma_delta_setup
 * Description  : Init Pin Config for Sigma_delta , change pin source to sigma-delta
 * Parameters   : uint32 GPIO_NUM (0...15)
 * Returns      : none
*******************************************************************************/
void ICACHE_FLASH_ATTR
sigma_delta_setup(uint32 GPIO_NUM)
{
    //============================================================================
    //STEP 1: SIGMA-DELTA CONFIG;REG SETUP
	GPIO_SIGMA_DELTA = (GPIO_SIGMA_DELTA & (~SIGMA_DELTA_SETTING_MASK)) | SIGMA_DELTA_ENABLE;
	//============================================================================
    //STEP 2: PIN FUNC CONFIG :SET PIN TO GPIO MODE AND ENABLE OUTPUT
	set_gpiox_mux_func_ioport(GPIO_NUM);
	gpio_output_set(0,0,1 << GPIO_NUM,0);
    //============================================================================
    //STEP 3: CONNECT SIGNAL TO GPIO PAD
	GPIO_PIN(GPIO_NUM) |= GPIO_PIN_SOURCE;
    //============================================================================
    //ets_printf("test reg gpio mtdi : 0x%08x \n",GPIO_REG_READ(GPIO_PIN_ADDR(GPIO_ID_PIN(GPIO_NUM))));
}
/******************************************************************************
 * FunctionName : sigma_delta_close
 * Description  : DEinit Pin ,from Sigma_delta mode to GPIO input mode
 * Parameters   : uint32 GPIO_NUM (0...15)
 * Returns      : none
*******************************************************************************/
void ICACHE_FLASH_ATTR
    sigma_delta_close(uint32 GPIO_NUM)
{
    //============================================================================
    //STEP 1: SIGMA-DELTA DEINIT
	GPIO_SIGMA_DELTA &= ~ SIGMA_DELTA_SETTING_MASK;
    //ets_printf("test reg gpio sigma : 0x%08x \n",GPIO_REG_READ(GPIO_PIN_ADDR(GPIO_ID_PIN(GPIO_SIGMA_DELTA_NUM))));
    //============================================================================
    //STEP 2: GPIO OUTPUT DISABLE
	gpio_output_set(0,0,0, 1 << GPIO_NUM);
    //============================================================================
    //STEP 3: CONNECT GPIO TO PIN PAD
	GPIO_PIN(GPIO_NUM) &= ~ GPIO_PIN_SOURCE;
    //============================================================================
	set_gpiox_mux_func_default(GPIO_NUM);
}
/******************************************************************************
 * FunctionName : set_sigma_duty_312KHz
 * Description  : 312K CONFIG EXAMPLE
 * Parameters   : uint8 duty, TARGET DUTY FOR 312K,  0-255
 * Returns      : none
*******************************************************************************/
void ICACHE_FLASH_ATTR  set_sigma_duty_312KHz(uint8 duty)
{
	 uint8 target = (duty>128)?(256-duty):duty;
	 uint8 prescale = (target==0)? 0:(target-1);

	 //freq = 80000 (khz) /256 /duty_target * (prescale+1)
	 GPIO_SIGMA_DELTA = SIGMA_DELTA_ENABLE
			| (duty << SIGMA_DELTA_TARGET_S)
			| (prescale << SIGMA_DELTA_PRESCALAR_S);
}
