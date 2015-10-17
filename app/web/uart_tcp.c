/******************************************************************************
 * FileName: uart_tcp.c
 * Description: UART-TCP driver ESP8266
 * Author: PV`
 * (c) PV` 2015
*******************************************************************************/
#include "user_config.h"
#include "bios.h"
#include "hw/esp8266.h"
#include "hw/uart_register.h"
#include "sdk/add_func.h"
#include "sdk/flash.h"
#include "flash_eep.h"
#include "tcp2uart.h"
//=============================================================================
extern void uart1_write_char(char c);
extern void uart0_write_char(char c);
//=============================================================================
// Функции для установки pins в режим UART
#define VAL_FUNC_U1TX   (1<<GPIO_MUX_FUN_BIT1)
#define VAL_FUNC_U0TX	0
#define VAL_FUNC_U0RX	0
#define VAL_FUNC_U0DTR	(1<<GPIO_MUX_FUN_BIT2)
#define VAL_FUNC_U0CTS	(1<<GPIO_MUX_FUN_BIT2)
#define VAL_FUNC_U0DSR	(1<<GPIO_MUX_FUN_BIT2)
#define VAL_FUNC_U0RTS	(1<<GPIO_MUX_FUN_BIT2)

#define PULLUP		(1<<GPIO_MUX_PULLUP_BIT)
#define PULLDOWN	(1<<GPIO_MUX_PULLDOWN_BIT)
#define PULLDIS		0
// Функции для установки pins в режим SDK
#define VAL_MUX_TX_UART1_OFF	VAL_MUX_GPIO2_IOPORT
#define VAL_MUX_TX_UART0_OFF	VAL_MUX_GPIO1_IOPORT
#define VAL_MUX_RX_UART0_OFF	VAL_MUX_GPIO3_IOPORT
#define VAL_MUX_RTS_UART0_OFF	VAL_MUX_GPIO15_IOPORT
#define VAL_MUX_CTS_UART0_OFF	VAL_MUX_GPIO13_IOPORT
// Регистры MUX для используемых pins
#define MUX_TX_UART1	GPIO2_MUX
#define MUX_TX_UART0	GPIO1_MUX
#define MUX_RX_UART0	GPIO3_MUX
#define MUX_RTS_UART0	GPIO15_MUX
#define MUX_CTS_UART0	GPIO13_MUX
// Маска MUX
#define MASK_MUX ((1<<GPIO_MUX_FUN_BIT0)|(1<<GPIO_MUX_FUN_BIT1)|(1<<GPIO_MUX_FUN_BIT2)|(1<<GPIO_MUX_PULLDOWN_BIT)|(1<<GPIO_MUX_PULLUP_BIT))
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
#ifdef USE_TCP2UART
void uart_intr_handler(void *para);
#endif
//=============================================================================
// uart0_set_flow()
//-----------------------------------------------------------------------------
void ICACHE_FLASH_ATTR uart0_set_flow(bool flow_en)
{
	uart0_flow_ctrl_flg = flow_en;
	ets_intr_lock(); //	ETS_UART_INTR_DISABLE();
	MEMW();
	uint32 conf0 = UART0_CONF0 & (~(UART_TX_FLOW_EN | UART_SW_RTS | UART_SW_DTR));
	uint32 conf1  = UART0_CONF1 & (~(UART_RX_FLOW_EN));
	if(flow_en) {
		conf0 |= UART_TX_FLOW_EN;
#ifdef USE_TCP2UART
		if(tcp2uart_conn != NULL)
#endif
		{
			conf1 |= UART_RX_FLOW_EN;
//			conf0 |= UART_SW_RTS;
		}
	}
	UART0_CONF0 = conf0;
	UART0_CONF1 = conf1;
	update_mux_uart0();
    ets_intr_unlock(); // ETS_UART_INTR_ENABLE();
}
//=============================================================================
// update_rts0() включение/отключение RTS UART0
// в зависимости от установок флага uart0_flow_ctrl_flg
//-----------------------------------------------------------------------------
void ICACHE_FLASH_ATTR update_rts0(void)
{
//	if(UART0_CONF0 & UART_TX_FLOW_EN) {
	if(uart0_flow_ctrl_flg) {
#ifdef USE_TCP2UART
		if(tcp2uart_conn != NULL) {
#endif
			MEMW();
			UART0_CONF1 |= UART_RX_FLOW_EN;
#ifdef USE_TCP2UART
		}
		else {
			MEMW();
			UART0_CONF1 &= ~UART_RX_FLOW_EN;
		}
#endif
	}
}
//=============================================================================
// Обновить mux выводов UART0
// update_mux_txd1()
//-----------------------------------------------------------------------------
void ICACHE_FLASH_ATTR update_mux_uart0(void)
{
#ifdef USE_TCP2UART
	if(syscfg.tcp2uart_port == 0) { // UART0 не включена на pins. Все pins используемые UART0 - ioport.
		MUX_TX_UART0 = VAL_MUX_TX_UART0_OFF;
		MUX_RX_UART0 = VAL_MUX_RX_UART0_OFF;
		MUX_RTS_UART0 = VAL_MUX_RTS_UART0_OFF;
		MUX_CTS_UART0 = VAL_MUX_CTS_UART0_OFF;
	}
	else
#endif
	{
		MUX_TX_UART0 = VAL_FUNC_U0TX; // GPIO1/TX0, output
		MUX_RX_UART0 = VAL_FUNC_U0RX | ((UART0_CONF0 & UART_RXD_INV)? PULLDOWN : PULLUP);  // GPIO3/RX0, input
		if(uart0_flow_ctrl_flg) { // включен flow
	    	update_rts0();
	    	MUX_RTS_UART0 = VAL_FUNC_U0RTS | PULLDIS; // GPIO15/RTS, output
	    	MUX_CTS_UART0 = VAL_FUNC_U0CTS | ((UART0_CONF0 & UART_CTS_INV)? PULLDOWN : PULLUP); // GPIO13/CTS, input
		}
		else {
			MUX_RTS_UART0 = VAL_MUX_RTS_UART0_OFF;
			MUX_CTS_UART0 = VAL_MUX_CTS_UART0_OFF;
		}
	}
}

//=============================================================================
// Обновить вывод TXD1
// update_mux_txd1()
//-----------------------------------------------------------------------------
void ICACHE_FLASH_ATTR update_mux_txd1(void)
{
	MEMW();
	uint32 x = MUX_TX_UART1 & (~MASK_MUX);
	if(syscfg.cfg.b.debug_print_enable) {
		x |= VAL_FUNC_U1TX;
	}
	else {
		x |= VAL_MUX_TX_UART1_OFF;
	}
	MUX_TX_UART1 = x;
}
//=============================================================================
// Инверсия входов/выходов RXD, TXD, RTS, CTS, DTR, DSR
// set_uartx_invx()
//-----------------------------------------------------------------------------
void ICACHE_FLASH_ATTR set_uartx_invx(uint8 uartn, uint8 set, uint32 bit_mask)
{
	uint32 invx = (set)? bit_mask : 0;
	ets_intr_lock(); //	ETS_UART_INTR_DISABLE();
	MEMW();
	if(uartn != UART0) {
		UART1_CONF0 = (UART1_CONF0 & (~ bit_mask)) | invx;
		if(bit_mask & UART_TXD_INV) update_mux_txd1();
	}
	else {
		UART0_CONF0 = (UART0_CONF0 & (~ bit_mask)) | invx;
		update_mux_uart0();
	}
    ets_intr_unlock(); // ETS_UART_INTR_ENABLE();
}
/******************************************************************************
 * FunctionName : uart_init
 * Description  : user interface for init uart
 * Parameters   : defbaud - default baudrate
 * Returns      : NONE
*******************************************************************************/
void ICACHE_FLASH_ATTR uart_init(void)
{
	struct  UartxCfg ux;
		//disable all UARTs interrupt
#ifdef USE_TCP2UART
		ets_isr_mask(1 << ETS_UART_INUM);
		tcp2uart_int_rxtx_disable();
#endif
// UART0
		if(flash_read_cfg(&ux, ID_CFG_UART0, sizeof(ux)) != sizeof(ux)) {
			ux.baud = UART0_DEFBAUD;
			ux.cfg.dw = UART0_REGCONFIG0DEF;
		};
		uart0_flow_ctrl_flg = ux.cfg.b.flow_en;
		ux.cfg.dw &= UART0_REGCONFIG0MASK;
		UART0_INT_ENA = 0;
	    UART0_CONF0 = ux.cfg.dw;
		    //set rx fifo trigger
#ifdef USE_TCP2UART
		UART0_CONF1 = ((0x01 & UART_RXFIFO_FULL_THRHD) << UART_RXFIFO_FULL_THRHD_S)
			| ((0x10 & UART_TXFIFO_EMPTY_THRHD) << UART_TXFIFO_EMPTY_THRHD_S)
			| (((128 - RST_FIFO_CNT_SET) & UART_RX_FLOW_THRHD) << UART_RX_FLOW_THRHD_S)
			| ((0x01 & UART_RX_TOUT_THRHD) << UART_RX_TOUT_THRHD_S) // | UART_RX_TOUT_EN
		;
#else
		UART1_CONF0 = 0x01707070;
#endif
		update_mux_uart0(); // включение/отключение RTS UART0 в зависимости от установок флага uart0_flow_ctrl_flg
		uart_div_modify(UART0, UART_CLK_FREQ / ux.baud); // WRITE_PERI_REG(UART_CLKDIV(num), ux.baud) + clear rx and tx fifo, not ready =
	    // clear all interrupt UART0
		UART0_INT_CLR &= ~0xffff;
// UART1
		if(flash_read_cfg(&ux, ID_CFG_UART1, sizeof(ux)) != sizeof(ux)) {
			ux.baud = UART1_DEFBAUD;
			ux.cfg.dw = UART1_REGCONFIG0DEF;
		};
		ux.cfg.dw &= UART1_REGCONFIG0MASK;
    	UART1_INT_ENA = 0;
		UART1_CONF0 = ux.cfg.dw;
	    UART1_CONF1 = 0x01707070;
		update_mux_txd1();
		uart_div_modify(UART1, UART_CLK_FREQ / ux.baud); // WRITE_PERI_REG(UART_CLKDIV(num), ux.baud) + clear rx and tx fifo, not ready =
		// clear all interrupt
		UART1_INT_CLR &= ~0xffff;

		MEMW();

	    os_install_putc1((void *)uart1_write_char); // install uart1 putc callback
#ifdef USE_TCP2UART
		ets_isr_attach(ETS_UART_INUM, uart_intr_handler, NULL);
		ets_isr_unmask(1 << ETS_UART_INUM);
#endif
}
//=============================================================================
// uart_read_fcfg()
// bit0 = 1 - Read UART0
// bit1 = 1 - Read UART1
//-----------------------------------------------------------------------------
void ICACHE_FLASH_ATTR uart_read_fcfg(uint8 set)
{
	struct  UartxCfg ux;
	if(set&1) {
		if(flash_read_cfg(&ux, ID_CFG_UART0, sizeof(ux)) != sizeof(ux)) {
			ux.baud = UART0_DEFBAUD;
			ux.cfg.dw = UART0_REGCONFIG0DEF; //8N1 + ?
		}
		uart_div_modify(UART0, UART_CLK_FREQ / ux.baud);
		UART0_CONF0 = ux.cfg.dw & UART0_REGCONFIG0MASK;
		update_mux_uart0();
//		uart0_set_flow(ux.cfg.b.flow_en);
	}
	if(set&2) {
		if(flash_read_cfg(&ux, ID_CFG_UART1, sizeof(ux)) != sizeof(ux)) {
			ux.baud = UART1_DEFBAUD;
			ux.cfg.dw = UART1_REGCONFIG0DEF; //8N1
		}
		uart_div_modify(UART1, UART_CLK_FREQ / ux.baud);
		UART1_CONF0 = ux.cfg.dw & UART1_REGCONFIG0MASK;
		update_mux_txd1();
	}
}
//=============================================================================
// uart_save_fcfg()
// bit0 = 1 - Save UART0
// bit1 = 1 - Save UART1
//-----------------------------------------------------------------------------
void ICACHE_FLASH_ATTR uart_save_fcfg(uint8 set)
{
	struct  UartxCfg ux;
	MEMW();
	if(set&1) {
		ux.baud = UART_CLK_FREQ / (UART0_CLKDIV & UART_CLKDIV_CNT);
		ux.cfg.dw = UART0_CONF0 & UART0_REGCONFIG0MASK;
		if(uart0_flow_ctrl_flg) ux.cfg.b.flow_en = 1;
		flash_save_cfg(&ux, ID_CFG_UART0, sizeof(ux));
	}
	if(set&2) {
		ux.baud = UART_CLK_FREQ / (UART1_CLKDIV & UART_CLKDIV_CNT);
		ux.cfg.dw = UART1_CONF0 & UART1_REGCONFIG0MASK;
		flash_save_cfg(&ux, ID_CFG_UART1, sizeof(ux));
	}
}
#ifdef USE_TCP2UART
/******************************************************************************
 * FunctionName : uart_intr_handler
 * Description  : Internal used function
 *                UART0 interrupt handler, add self handle code inside
 *                uart0 and uart1 intr combine together, when interrupt occur,
 *                see reg 0x3ff20020, bit2, bit0 represents uart1 and uart0 respectively
 * Parameters   : void *para - point to ETS_UART_INTR_ATTACH's arg
 * Returns      : NONE
*******************************************************************************/
void uart_intr_handler(void *para)
{
    // uart0 and uart1 intr combine togther, when interrupt occur, see reg 0x3ff20020, bit2, bit0 represents
    // uart1 and uart0 respectively
	MEMW(); // синхронизация и ожидание отработки fifo-write на шинах CPU,  на всякий случай
    uint32 ints = UART0_INT_ST;
    if(ints) {
    	if (ints & UART_TXFIFO_EMPTY_INT_ST) { // fifo tx пусто?
    		UART0_INT_ENA &= ~UART_TXFIFO_EMPTY_INT_ENA;
    		if(tcp2uart_conn != NULL) ets_timer_arm_new(&uart0_tx_buf_timer, 10, 0, 0); // 10 us
    	};
    	if (ints & UART_RXFIFO_FULL_INT_ST) { // прерывание по приему символов? да
    		UART0_INT_ENA &= ~UART_RXFIFO_FULL_INT_ENA;
       		if(tcp2uart_conn != NULL && (!tcp2uart_conn->flag.user_flg1)) {
       			ets_timer_disarm(&uart0_rx_buf_timer);
       			uint32 buftimeout = (UART0_STATUS >> UART_RXFIFO_CNT_S) & UART_RXFIFO_CNT; // кол-во принятых символов в rx fifo
       			if(buftimeout < (128 - RST_FIFO_CNT_SET - 1)) { // можем ещё принять до выставления RTS? да.
       				buftimeout = (128 - RST_FIFO_CNT_SET - 1) - buftimeout; // сколько символов можем принять до выставления RTS
       				buftimeout = ((UART0_CLKDIV & UART_CLKDIV_CNT) * buftimeout) >> 3; // время передачи символа (10 бит) в us =  UART_CLKDIV / 8
       				if(buftimeout < 128) buftimeout = 128; // быстрее работать не стоит
       				else if(buftimeout > MAX_WAIT_TX_BUF) buftimeout = MAX_WAIT_TX_BUF; // низкая скорость и буфер будет заполнен более чем через 0.05 сек? ограничить
       				// buftimeout -= 16; // вычесть время исполнения?
				}
    			else buftimeout = 32; // буфер rx fifo заполнен
   				ets_timer_arm_new(&uart0_rx_buf_timer, buftimeout, 0, 0);
       		};
        };
    	UART0_INT_CLR = ints;
    }
    else {
    	UART1_INT_ENA = 0;
    	UART1_INT_CLR = UART1_INT_ST;
    }
}
#endif // USE_TCP2UART
