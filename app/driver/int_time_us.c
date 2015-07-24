/******************************************************************************
* PV` * Test intr TIMER0 (0.2..1677721.4 us)
*******************************************************************************/
#if 0
#include "bios/ets.h"
#include "hw/esp8266.h"
#include "user_interface.h"

#define TEST_PIN_IO 12

#define XS_TO_RTC_TIMER_TICKS(t, prescaler, period)	\
     (((t) > (0xFFFFFFFF/(APB_CLK_FREQ >> prescaler))) ?	\
      (((t) >> 2) * ((APB_CLK_FREQ >> prescaler)/(period>>2)) + ((t) & 0x3) * ((APB_CLK_FREQ >> prescaler)/period))  :	\
      (((t) * (APB_CLK_FREQ >> prescaler)) / period))

/******************************************************************************
 * Менее 2us при 160MHz CPU - перегрузка системы по WDT
 * Более 1.5 sec - перегрузка системы по WDT (WDT 1.6 сек)
*******************************************************************************/
void ICACHE_FLASH_ATTR set_new_time_int_us(uint32 us)
{
    // максимальный делитель у таумера 0x007fffff
    // пределы 0.2..1677721.4 us при DIVDED_BY_16
#if ((APB_CLK_FREQ>>4)%1000000)
     TIMER0_LOAD = XS_TO_RTC_TIMER_TICKS(us, 4, 1000);
#else
     TIMER0_LOAD = us * (APB_CLK_FREQ>>4)/1000000; // = us * 5
#endif
}
/******************************************************************************
 * tst_tim1_intr_handler()
*******************************************************************************/
void tst_tm0_intr_handler(void)
{
    TIMER0_INT = 0;
    GPIO_OUT_W1TS = 1 << TEST_PIN_IO;
    GPIO_OUT_W1TC = 1 << TEST_PIN_IO;
}
/******************************************************************************
 * int_us_disable()
*******************************************************************************/
void ICACHE_FLASH_ATTR int_us_disable(void)
{
	ets_isr_mask(BIT(ETS_FRC_TIMER0_INUM));
	INTC_EDGE_EN &= ~BIT(1);
}
/******************************************************************************
 * int_us_init()
*******************************************************************************/
void ICACHE_FLASH_ATTR int_us_init(uint32 us)
{
    int_us_disable();

    SET_PIN_FUNC_IOPORT(TEST_PIN_IO);
	GPIO_ENABLE = 1 << TEST_PIN_IO;
    TIMER0_CTRL =   TM_DIVDED_BY_16
                  | TM_AUTO_RELOAD_CNT
                  | TM_ENABLE_TIMER
                  | TM_EDGE_INT;
    set_new_time_int_us(us);
    ets_isr_attach(ETS_FRC_TIMER0_INUM, tst_tm0_intr_handler, NULL);
    INTC_EDGE_EN |= BIT(1);
    ets_isr_unmask(BIT(ETS_FRC_TIMER0_INUM));
}

#endif
