/******************************************************************************
* PV` * Test intr TM1
*******************************************************************************/
#include "bios/ets.h"
#include "hw/esp8266.h"
#include "hw/timer_register.h"
#include "user_interface.h"

#define TEST_PIN_IO 12

//XXX: 0xffffffff/(80000000/16)=35A
// 0x35A=858
// ((858>>2)*(80000000>>4)/250000) + (858&3)*(80000000>>4)/1000000 = 4290
// 858*(80000000>>4)/1000000 = 4290
// 4290*16/80000000 = 0.000858
#define US_TO_RTC_TIMER_TICKS(t)          \
    ((t) ?                                   \
     (((t) > 0x35A) ?                   \
      (((t)>>2) * ((APB_CLK_FREQ>>4)/250000) + ((t)&0x3) * ((APB_CLK_FREQ>>4)/1000000))  :    \
      (((t) *(APB_CLK_FREQ>>4)) / 1000000)) :    \
     0)

//FRC1
#define FRC1_ENABLE_TIMER  		BIT7
#define AUTO_RELOAD_CNT_TIMER	BIT6

typedef enum {
    DIVDED_BY_1 = 0,
    DIVDED_BY_16 = 4,
    DIVDED_BY_256 = 8,
} TIMER_PREDIVED_MODE;

typedef enum {
    TM_EDGE_INT	= 0,
    TM_LEVEL_INT = 1
} TIMER_INT_MODE;
/******************************************************************************
 * Менее 2us при 160MHz CPU - перегрузка системы (WDT)
*******************************************************************************/
void ICACHE_FLASH_ATTR set_new_time_int_us(uint32 us)
{
    // максимальный делитель у таумера 0x007fffff
    // пределы 0.2..1677721.4 us при DIVDED_BY_16
    TIMER1_LOAD = US_TO_RTC_TIMER_TICKS(us);
}
/******************************************************************************
 * tst_tim1_intr_handler()
*******************************************************************************/
void tst_tim1_intr_handler(void)
{
    TIMER1_INT &= ~ FRC1_INT_CLR_MASK;
    GPIO_OUT_W1TS = 1 << TEST_PIN_IO;
    GPIO_OUT_W1TC = 1 << TEST_PIN_IO;
    /*    asm volatile (
    "s32i %0, %1, 4\n\t"
    "s32i %0, %1, 8"
    : : "r"(0x00001000), "r"(0x60000300) : "memory");
    //    GPIO_REG_WRITE(GPIO_OUT_ADDRESS, READ_PERI_REG(GPIO_IN_ADDRESS) ^ TEST_PIN_IO);

    volatile register uint32 *ioaddr = (uint32 *)(PERIPHS_GPIO_BASEADDR + GPIO_OUT_ADDRESS);
    uint32 ioclr = READ_PERI_REG(PERIPHS_GPIO_BASEADDR + GPIO_IN_ADDRESS) & (~TEST_PIN_IO);
	uint32 ioset = ioclr | TEST_PIN_IO;

    *ioaddr = ioclr;
    *ioaddr = ioset;
    *ioaddr = ioclr;
    *ioaddr = ioset;
    *ioaddr = ioclr;
    *ioaddr = ioset;   */
}
/******************************************************************************
 * int_us_disable()
*******************************************************************************/
void ICACHE_FLASH_ATTR int_us_disable(void)
{
	ETS_FRC1_INTR_DISABLE();
	TM1_EDGE_INT_DISABLE();
}
/******************************************************************************
 * int_us_init()
*******************************************************************************/
void ICACHE_FLASH_ATTR int_us_init(uint32 us)
{
    int_us_disable();

    SET_PIN_FUNC_IOPORT(TEST_PIN_IO);
	GPIO_ENABLE = 1 << TEST_PIN_IO;
    TIMER1_CTRL =
                  DIVDED_BY_16
                  | AUTO_RELOAD_CNT_TIMER
                  | FRC1_ENABLE_TIMER
                  | TM_EDGE_INT;
    set_new_time_int_us(us);
    ets_isr_attach(ETS_FRC_TIMER1_INUM, tst_tim1_intr_handler, NULL);
    INTC_EDGE_EN |= BIT(ETS_FRC_TIMER1_INUM);
    ets_isr_unmask(BIT(ETS_FRC_TIMER1_INUM));
}
