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
#ifdef USE_RS485DRV
#include "driver/rs485drv.h"
#endif
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
#ifdef USE_TCP2UART

#define UART_RX_ERR_INTS (UART_BRK_DET_INT_ENA | UART_RXFIFO_OVF_INT_ENA | UART_FRM_ERR_INT_ENA | UART_PARITY_ERR_INT_ENA)
 #define DEBUG_OUT(x)  // UART1_FIFO = x

#define os_post ets_post
#define os_task ets_task
#define mMIN(a, b)  ((a<b)?a:b)

suart_drv uart_drv DATA_IRAM_ATTR;

void uart_intr_handler(void *para);

#endif
//-----------------------------------------------------------------------------
#ifndef USE_RS485DRV
//=============================================================================
// uart0_set_flow()
//-----------------------------------------------------------------------------
void ICACHE_FLASH_ATTR uart0_set_flow(bool flow_en)
{
	uart0_flow_ctrl_flg = flow_en;
	ets_intr_lock(); //	ETS_UART_INTR_DISABLE();
	MEMW();
	uint32 conf0 = UART0_CONF0 & (~(UART_TX_FLOW_EN | UART_SW_RTS | UART_SW_DTR));
	uint32 conf1 = UART0_CONF1 & (~(UART_RX_FLOW_EN));
	if(flow_en) {
		conf0 |= UART_TX_FLOW_EN;
#ifdef USE_TCP2UART
		if(uart_drv.uart_rx_buf != NULL)
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
		MEMW();
#ifdef USE_TCP2UART
		if(uart_drv.uart_rx_buf != NULL) {
#endif
			UART0_CONF1 |= UART_RX_FLOW_EN;
#ifdef USE_TCP2UART
		}
		else {
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
		if(PERI_IO_SWAP & PERI_IO_UART0_PIN_SWAP) { // swap uart0 pins (u0rxd <-> u0cts), (u0txd <-> u0rts) ?
			MUX_RTS_UART0 = VAL_FUNC_U0RTS | PULLDIS; // GPIO15/TX0, output
			MUX_CTS_UART0 = VAL_FUNC_U0CTS | ((UART0_CONF0 & UART_RXD_INV)? PULLDOWN : PULLUP); // GPIO13/RX0, input
			if(uart0_flow_ctrl_flg) { // включен flow
		    	update_rts0();
		    	MUX_TX_UART0 = VAL_FUNC_U0TX | PULLDIS; // GPIO1/RTS, output
		    	MUX_RX_UART0 = VAL_FUNC_U0RX | ((UART0_CONF0 & UART_CTS_INV)? PULLDOWN : PULLUP); // GPIO3/CTS, input
			}
			else {
				MUX_TX_UART0 = VAL_MUX_TX_UART0_OFF;
				MUX_RX_UART0 = VAL_MUX_RX_UART0_OFF;
			}
		}
		else {
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
}
#endif
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
#ifndef USE_RS485DRV
	else {
		UART0_CONF0 = (UART0_CONF0 & (~ bit_mask)) | invx;
		update_mux_uart0();
	}
#endif
    ets_intr_unlock(); // ETS_UART_INTR_ENABLE();
}
//=============================================================================
// uart_read_fcfg()
// bit0 = 1 - Read UART0
// bit1 = 1 - Read UART1
//-----------------------------------------------------------------------------
void ICACHE_FLASH_ATTR uart_read_fcfg(uint8 set)
{
	struct  UartxCfg ux;
	if(set&2) {
		if(flash_read_cfg(&ux, ID_CFG_UART1, sizeof(ux)) != sizeof(ux)) {
			ux.baud = UART1_DEFBAUD;
			ux.cfg.dw = UART1_REGCONFIG0DEF; //8N1
		}
		UART1_CONF0 = ux.cfg.dw & UART1_REGCONFIG0MASK;
		UART1_CONF1 = ((0x01 & UART_RXFIFO_FULL_THRHD) << UART_RXFIFO_FULL_THRHD_S)
			| ((0x01 & UART_TXFIFO_EMPTY_THRHD) << UART_TXFIFO_EMPTY_THRHD_S)
			| (((128 - RST_FIFO_CNT_SET) & UART_RX_FLOW_THRHD) << UART_RX_FLOW_THRHD_S)
			| ((0x07 & UART_RX_TOUT_THRHD) << UART_RX_TOUT_THRHD_S) // | UART_RX_TOUT_EN
		;
//		if(ux.cfg.b.swap) PERI_IO_SWAP |= PERI_IO_UART1_PIN_SWAP;
//		else PERI_IO_SWAP &= ~PERI_IO_UART1_PIN_SWAP;
		update_mux_txd1();
		uart_div_modify(UART1, UART_CLK_FREQ / ux.baud);
	}
#ifdef USE_RS485DRV
	if(set&1) {
		if(flash_read_cfg(&rs485cfg, ID_CFG_UART0, sizeof(rs485cfg)) != sizeof(rs485cfg)) {
			rs485cfg.baud = RS485_DEF_BAUD;
			rs485cfg.flg.ui = RS485_DEF_FLG;
			rs485cfg.timeout = RS485_DEF_TWAIT;
		}
		rs485_drv_set_pins();
		rs485_drv_set_baud();
/*
		srs485cfg rcfg;
		if(flash_read_cfg(&rcfg, ID_CFG_UART0, sizeof(rcfg)) != sizeof(rcfg)) {
			rcfg.baud = RS485_DEF_BAUD;
			rcfg.flg.ui = RS485_DEF_FLG;
			rcfg.timeout = RS485_DEF_TWAIT;
		}
		rs485_drv_new_cfg(&rcfg); */
	}
#else
	if(set&1) {
		if(flash_read_cfg(&ux, ID_CFG_UART0, sizeof(ux)) != sizeof(ux)) {
			ux.baud = UART0_DEFBAUD;
			ux.cfg.dw = UART0_REGCONFIG0DEF; //8N1
		}
		uart0_flow_ctrl_flg = ux.cfg.b.flow_en;
		UART0_CONF0 = ux.cfg.dw & UART0_REGCONFIG0MASK;
		UART0_CONF1 = ((128 - RST_FIFO_CNT_SET - RST_FIFO_CNT_SET) << UART_RXFIFO_FULL_THRHD_S)
#ifdef USE_TCP2UART
			| ((0x08 & UART_TXFIFO_EMPTY_THRHD) << UART_TXFIFO_EMPTY_THRHD_S)
#else
			| ((0x01 & UART_TXFIFO_EMPTY_THRHD) << UART_TXFIFO_EMPTY_THRHD_S)
#endif
			| (((128 - RST_FIFO_CNT_SET) & UART_RX_FLOW_THRHD) << UART_RX_FLOW_THRHD_S)
			| ((0x07 & UART_RX_TOUT_THRHD) << UART_RX_TOUT_THRHD_S) //| UART_RX_TOUT_EN
		;
		if(ux.cfg.b.swap) PERI_IO_SWAP |= PERI_IO_UART0_PIN_SWAP;
		else PERI_IO_SWAP &= ~PERI_IO_UART0_PIN_SWAP;
		update_mux_uart0();
		uart_div_modify(UART0, UART_CLK_FREQ / ux.baud);
#ifdef USE_TCP2UART
		uart0_set_tout();
#endif
	}
#endif
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
#ifdef USE_RS485DRV
		flash_save_cfg(&rs485cfg, ID_CFG_UART0, sizeof(rs485cfg));
#else
		ux.baud = UART_CLK_FREQ / (UART0_CLKDIV & UART_CLKDIV_CNT);
		ux.cfg.dw = UART0_CONF0 & UART0_REGCONFIG0MASK;
		if(uart0_flow_ctrl_flg) ux.cfg.b.flow_en = 1;
		if(PERI_IO_SWAP & PERI_IO_UART0_PIN_SWAP) ux.cfg.b.swap = 1; // swap uart0 pins (u0rxd <-> u0cts), (u0txd <-> u0rts)
		flash_save_cfg(&ux, ID_CFG_UART0, sizeof(ux));
#endif
	}
	if(set&2) {
		ux.baud = UART_CLK_FREQ / (UART1_CLKDIV & UART_CLKDIV_CNT);
		ux.cfg.dw = UART1_CONF0 & UART1_REGCONFIG0MASK;
//		if(PERI_IO_SWAP & PERI_IO_UART1_PIN_SWAP) ux.cfg.b.swap = 1; // swap uart1 pins (u1rxd <-> u1cts), (u1txd <-> u1rts)
		flash_save_cfg(&ux, ID_CFG_UART1, sizeof(ux));
	}
}
#ifdef USE_TCP2UART
/******************************************************************************
 *
 *******************************************************************************/
void uart0_set_tout(void)
{
	uint32 tout_thrhd = UART0_CLKDIV;
	if(tout_thrhd < 200) tout_thrhd = 127;
	else if(tout_thrhd < (UART_CLK_FREQ/19200)) {
		tout_thrhd = 25000/tout_thrhd;
		tout_thrhd = (tout_thrhd>>1)+(tout_thrhd&1);
		if(tout_thrhd > 125) tout_thrhd = 125;
		tout_thrhd += 2;
	}
	else tout_thrhd = 5;
	UART0_CONF1 = (UART0_CONF1 & (~(UART_RX_TOUT_THRHD << UART_RX_TOUT_THRHD_S))) | (tout_thrhd << UART_RX_TOUT_THRHD_S);
}
/* =========================================================================
 * uart_tx_buf
 * ------------------------------------------------------------------------- */
uint32 uart_tx_buf(uint8 *buf, uint32 count)
{
//	DEBUG_OUT('T');
	int len = 0;
	while(len < count){
		MEMW(); // синхронизация и ожидание отработки fifo-write на шине CPU
		if (((UART0_STATUS >> UART_TXFIFO_CNT_S) & UART_TXFIFO_CNT) >= 127) {
			// не всё передано - не лезет в буфер fifo UART tx.
//			DEBUG_OUT('#');
			ets_intr_lock(); //	ETS_UART_INTR_DISABLE();
			UART0_INT_ENA |= UART_TXFIFO_EMPTY_INT_ENA; // установим прерывание на пустой fifo tx
			ets_intr_unlock(); // ETS_UART_INTR_ENABLE();
			break;
		}
		UART0_FIFO = buf[len++];
	};
	return len;
}
/* =========================================================================
 * uart_drv_close
 * ------------------------------------------------------------------------- */
void ICACHE_FLASH_ATTR uart_drv_close(void)
{
	DEBUG_OUT('C');
	UART0_INT_ENA = 0;
	//clear rx and tx fifo, not ready
	uint32 conf0 = UART0_CONF0;
	UART0_CONF0 = conf0 | UART_RXFIFO_RST | UART_TXFIFO_RST;
	UART0_CONF0 = conf0 & (~ (UART_RXFIFO_RST | UART_TXFIFO_RST));
	UART0_CONF1 &= ~UART_RX_TOUT_EN;
	if(uart_drv.uart_rx_buf != NULL) {
		os_free(uart_drv.uart_rx_buf);
		uart_drv.uart_rx_buf = NULL;
	}
	UART0_INT_CLR = 0xffff;
	update_rts0(); // update RST
}
/* =========================================================================
 * uart_drv_close
 * ------------------------------------------------------------------------- */
bool ICACHE_FLASH_ATTR uart_drv_start(void)
{
		uart_drv_close();
		DEBUG_OUT('S');
		if(uart_drv.uart_tx_next_chars  == NULL || uart_drv.uart_send_rx_blk  == NULL) return false;
		uart_drv.uart_rx_buf = os_malloc(UART_RX_BUF_MAX);
		uart_drv.uart_rx_buf_count = 0;
		uart_drv.uart_nsnd_buf_count = 0;
		uart_drv.uart_out_buf_count = 0;
		if(uart_drv.uart_rx_buf != NULL) {
			update_rts0(); // update RST
			UART0_CONF1 |= UART_RX_TOUT_EN;
			UART0_INT_ENA = UART_RXFIFO_FULL_INT_ENA | UART_RXFIFO_TOUT_INT_ENA;
			return true;
		}
		return false;
}
/* =========================================================================
 * uart_del_rx_chars
 * ------------------------------------------------------------------------- */
void uart_del_rx_chars(uint32 len)
{
//	if(len) {
//		if(len > uart_drv.uart_nsnd_buf_count) len = uart_drv.uart_nsnd_buf_count;
		ets_intr_lock();
		uart_drv.uart_nsnd_buf_count -= len;
		uart_drv.uart_rx_buf_count -= len;
		uart_drv.uart_out_buf_count -= len;
		uint32 i;
		for(i = 0; i < uart_drv.uart_rx_buf_count; i++) {
			uart_drv.uart_rx_buf[i] = uart_drv.uart_rx_buf[len + i];
		}
		if(uart_drv.uart_nsnd_buf_count < UART_RX_BUF_MAX - (TCP_MSS*2) ) {
			UART0_INT_ENA |= UART_RXFIFO_FULL_INT_ENA | UART_RXFIFO_TOUT_INT_ENA;
		}
		ets_intr_unlock();
//	}
}
/* =========================================================================
 * uart_task
 * ------------------------------------------------------------------------- */
static void uart_task(os_event_t *e){
	if(uart_drv.uart_rx_buf != NULL)
	{
		switch(e->sig) {
			case UART_RX_CHARS:	//	Приняты символы (в uart_rx_buf), передать
			{
//				DEBUG_OUT('r');
#if DEBUGSOO > 5
				os_printf("uart_rx: new=%u, ns=%u, bl=%u\n", e->par, uart_drv.uart_nsnd_buf_count, uart_drv.uart_nsnd_buf_count);
#endif
				if(e->par) uart_drv.uart_send_rx_blk(uart_drv.uart_rx_buf, e->par);
			}
			break;
			case UART_TX_CHARS:	//	Передать символы в UART
			{
//				DEBUG_OUT('w');
				uart_drv.uart_tx_next_chars();
			}
			break;
		}
	}
	else uart_drv_close();
}
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
//	MEMW(); // синхронизация и ожидание отработки fifo-write на шинах CPU
	if(DPORT_OFF20 & (1<<0)) { // uart0 and uart1 intr combine togther, when interrupt occur, see reg 0x3ff20020, bit2, bit0 represents
//		DEBUG_OUT('i');
		uint32 ints = UART0_INT_ST;
		if(ints) {
			if(UART0_INT_RAW & UART_RX_ERR_INTS) { // ошибки при приеме?
				DEBUG_OUT('E');
				UART0_INT_CLR = UART_RX_ERR_INTS;
				//uart_rx_clr_buf(); // сбросить rx fifo, ошибки приема и буфер
			}
			if(uart_drv.uart_rx_buf != NULL) { // соединение открыто?
				if (ints & (UART_RXFIFO_FULL_INT_ST | UART_RXFIFO_TOUT_INT_ST)) { // прерывание по приему символов или Rx time-out event? да
					uint32 uart_rx_buf_count = uart_drv.uart_rx_buf_count;
//					DEBUG_OUT('R');
					if(uart_drv.uart_nsnd_buf_count >= UART_RX_BUF_MAX //) { // накопился не переданный большой блок?
						|| uart_rx_buf_count >= UART_RX_BUF_MAX) { // буфер заполнен?
						DEBUG_OUT('@');
						// отключим прерывание приема, должен выставиться RTS
						UART0_INT_ENA &= ~(UART_RXFIFO_FULL_INT_ENA | UART_RXFIFO_TOUT_INT_ENA);
					}
					else { // продожим прием в буфер
						if((UART0_STATUS >> UART_RXFIFO_CNT_S) & UART_RXFIFO_CNT) {
							do {
								if(uart_rx_buf_count >= UART_RX_BUF_MAX) { // буфер заполнен?
									DEBUG_OUT('#');
									// отключим прерывание приема, должен выставиться RTS
									UART0_INT_ENA &= ~(UART_RXFIFO_FULL_INT_ENA | UART_RXFIFO_TOUT_INT_ENA);
									break;
								}
								// скопируем символ в буфер
								uart_drv.uart_rx_buf[uart_rx_buf_count++] = UART0_FIFO;
							} while((UART0_STATUS >> UART_RXFIFO_CNT_S) & UART_RXFIFO_CNT);
						};
						uart_drv.uart_rx_buf_count = uart_rx_buf_count; // счет в буфере
						//
						uart_rx_buf_count -=  uart_drv.uart_out_buf_count; // кол-во принятых и ещё не переданных из прерывания UART символов
						if(uart_rx_buf_count != 0) {
							if((ints & UART_RXFIFO_TOUT_INT_ST) != 0) { // межсимвольная пауза (конец блока)?
								DEBUG_OUT('|');
								os_post(UART_TASK_PRIO + SDK_TASK_PRIO, UART_RX_CHARS, uart_rx_buf_count); // пришла добавочка
								uart_drv.uart_nsnd_buf_count += uart_rx_buf_count; // счет отосланного
								uart_drv.uart_out_buf_count = uart_drv.uart_rx_buf_count;
							}
							else if(uart_drv.uart_nsnd_buf_count < (TCP_MSS*2) // ещё влезет в буфер передачи TCP?
								&& uart_rx_buf_count + uart_drv.uart_nsnd_buf_count >= (TCP_MSS*2)) { // накопился большой не переданный блок в TCP?
								uart_rx_buf_count = (TCP_MSS*2) - uart_drv.uart_nsnd_buf_count;	// добавочка до (TCP_MSS*2)
								os_post(UART_TASK_PRIO + SDK_TASK_PRIO, UART_RX_CHARS, uart_rx_buf_count); // добавочка до (TCP_MSS*2)
								uart_drv.uart_nsnd_buf_count = (TCP_MSS*2); // счет отосланного
								uart_drv.uart_out_buf_count += uart_rx_buf_count;
							};
						};
					};
				};
				if (ints & UART_TXFIFO_EMPTY_INT_ST) { // fifo tx пусто?
//					DEBUG_OUT('W');
					os_post(UART_TASK_PRIO + SDK_TASK_PRIO, UART_TX_CHARS, ints);
					UART0_INT_ENA &= ~UART_TXFIFO_EMPTY_INT_ENA;
				};
			}
			else UART0_INT_ENA = 0;
	    };
		UART0_INT_CLR = ints;
	}
    else {
    	UART1_INT_ENA = 0;
    	UART1_INT_CLR = 0xffff;
    };
}
#endif // USE_TCP2UART
/******************************************************************************
 * FunctionName : uart_init
 * Description  : user interface for init uart
 * Parameters   : defbaud - default baudrate
 * Returns      : NONE
*******************************************************************************/
void ICACHE_FLASH_ATTR uarts_init(void)
{
		//disable all UARTs interrupt
#ifdef USE_TCP2UART
		ets_isr_mask(1 << ETS_UART_INUM);
		uart_drv_close();
#endif
// UART0
#ifndef USE_RS485DRV
		UART0_INT_ENA = 0;
		uart_read_fcfg(1);
	    // clear all interrupt UART0
		UART0_INT_CLR = 0xffff;
#else
		if(flash_read_cfg(&rs485cfg, ID_CFG_UART0, sizeof(rs485cfg)) != sizeof(rs485cfg)) {
			rs485cfg.baud = RS485_DEF_BAUD;
			rs485cfg.flg.ui = RS485_DEF_FLG;
			rs485cfg.timeout = RS485_DEF_TWAIT;
		}
		rs485_drv_init();
#endif
// UART1
    	UART1_INT_ENA = 0;
		uart_read_fcfg(2);
		// clear all interrupt
		UART1_INT_CLR &= ~0xffff;

	    os_install_putc1((void *)uart1_write_char); // install uart1 putc callback
#ifdef USE_TCP2UART
		ets_isr_attach(ETS_UART_INUM, uart_intr_handler, NULL);
		ets_isr_unmask(1 << ETS_UART_INUM);
		os_task(uart_task, UART_TASK_PRIO + SDK_TASK_PRIO, uart_drv.taskQueue, UART_TASK_QUEUE_LEN);
#endif
}
