/******************************************************************************
 * FileName: UartDev.c
 * Description: UART funcs in ROM-BIOS
 * Alternate SDK ver 0.0.0 (b0)
 * Author: PV`
 * (c) PV` 2015
*******************************************************************************/
#include "bios/uart.h"
#include "hw/esp8266.h"
#include "hw/uart_register.h"

//uint8 RcvBuff[256];
//uint8 TrxBuff[100];
#define RcvBuff  ((uint8 *)0x3fffde60)
#define TrxBuff  (&RcvBuff[0x100]) // ((uint8 *)0x3fffdf60)

// ROM:40003BBC
void uart_rx_intr_handler(void *par)
{
	volatile uint32_t *uartregs = REG_UART_BASE(UartDev.buff_uart_no);
	if((uartregs[IDX_UART_INT_ST] & UART_RXFIFO_FULL_INT_ST) == 0) return; // UART_INT_ST(x) & UART_RXFIFO_FULL_INT_ST
	uartregs[IDX_UART_INT_CLR] = UART_RXFIFO_FULL_INT_ST; // UART_INT_CLR
	while(uartregs[IDX_UART_STATUS] & UART_RXFIFO_CNT) { // UART_STATUS(x) & UART_RXFIFO_CNT
		RcvMsgBuff * rcvmsg = (RcvMsgBuff *)par;
		register uint8 c = uartregs[IDX_UART_FIFO]; // UART_FIFO(x)
		if(c == '\n') rcvmsg->BuffState = WRITE_OVER;
		*rcvmsg->pWritePos++ = c;
		if(rcvmsg->pWritePos >= rcvmsg->pRcvMsgBuff + rcvmsg->RcvBuffSize) {
			rcvmsg->pWritePos = rcvmsg->pRcvMsgBuff;
		}
	}
}

// ROM:4000383C
void uartAttach(void)
{
	// &UartDev.RcvMsgBuff;
	UartDev.baut_rate = BIT_RATE_115200;
	UartDev.data_bits = EIGHT_BITS;
	UartDev.exist_parity = STICK_PARITY_DIS;
	UartDev.parity = NONE_BITS;
	UartDev.stop_bits = ONE_STOP_BIT;
	UartDev.flow_ctrl = NONE_CTRL;
	UartDev.rcv_buff.RcvBuffSize = RX_BUFF_SIZE;
	UartDev.rcv_buff.pRcvMsgBuff = RcvBuff; //	&UartDev + 0x50
	UartDev.rcv_buff.pWritePos = RcvBuff;
	UartDev.rcv_buff.pReadPos = RcvBuff;
	UartDev.rcv_buff.TrigLvl = 1;
	UartDev.rcv_buff.BuffState = EMPTY;
	UartDev.trx_buff.TrxBuffSize = TX_BUFF_SIZE;
	UartDev.trx_buff.pTrxBuff =	TrxBuff;
	UartDev.rcv_state = BAUD_RATE_DET;
	UartDev.received = 0;
	UartDev.buff_uart_no = UART0;

	ets_isr_mask(1 << ETS_UART_INUM);
	ets_isr_attach(ETS_UART_INUM, uart_rx_intr_handler, &UartDev.rcv_buff);
}

// ROM:40003B8C
int uart_rx_one_char(uint8 *ch)
{
	volatile uint32_t *uartregs = REG_UART_BASE(UartDev.buff_uart_no);
	if(uartregs[IDX_UART_STATUS] & UART_RXFIFO_CNT) {
		*ch = uartregs[0];
		return 0;
	}
	return 1;
}

// ROM:40003B64
uint8 uart_rx_one_char_block(void)
{
	volatile uint32_t *uartregs = REG_UART_BASE(UartDev.buff_uart_no);
	while((uartregs[IDX_UART_STATUS] & UART_RXFIFO_CNT)==0);
	return uartregs[IDX_UART_FIFO];
}

// ROM:40003EC8
int uart_rx_readbuff(RcvMsgBuff * rcvmsg, uint8 *dst)
{
	if(rcvmsg->pReadPos == rcvmsg->pWritePos) return 1;
	*dst = *rcvmsg->pReadPos++;
	if(rcvmsg->pReadPos >= rcvmsg->pRcvMsgBuff + rcvmsg->RcvBuffSize) {
				rcvmsg->pReadPos = rcvmsg->pRcvMsgBuff;
	}
	return 0;
}

// ROM:40003B30
int uart_tx_one_char(uint8 ch)
{
	volatile uint32_t *uartregs = REG_UART_BASE(UartDev.buff_uart_no);
	while(uartregs[IDX_UART_STATUS] & (UART_TXFIFO_CNT << UART_TXFIFO_CNT_S));
	uartregs[IDX_UART_FIFO] = ch;
	return 0;
}

// ROM:400038A4
void uart_buff_switch(uint8 uartnum)
{
	if(uartnum) {
		UartDev.buff_uart_no = uartnum;
		return;
	}
	volatile uint32_t *uartregs = REG_UART_BASE(UartDev.buff_uart_no);
	uartregs[IDX_UART_INT_CLR] = 0xFFFF; // UART_INT_CLR(UartDev.buff_uart_no)
	uartregs[IDX_UART_INT_ENA] &= 0xE00;
	uint8 ch;
	while(uart_rx_one_char(&ch) == 0 &&	uart_rx_readbuff(&UartDev.rcv_buff, &ch));
	UartDev.rcv_buff.BuffState = EMPTY;
}

// ROM:400039D8
void uart_div_modify(uint32 uart_num, uint32 div_baud)
{
	volatile uint32_t *uartregs = REG_UART_BASE(uart_num);
	uartregs[IDX_UART_CLKDIV] = div_baud;
	uartregs[IDX_UART_CONF0] |= UART_TXFIFO_RST | UART_RXFIFO_RST;
	uartregs[IDX_UART_CONF0] &= (~(UART_TXFIFO_RST | UART_RXFIFO_RST));
}

// ROM:40003924
uint32 uart_baudrate_detect(uint32 uart_num, uint32 flg)
{
	volatile uint32_t *uartregs = REG_UART_BASE(uart_num);
	if(UartDev.rcv_state != BAUD_RATE_DET) {
		uartregs[IDX_UART_AUTOBAUD] &= 0x7E; // UART_AUTOBAUD(uart_num)
		uartregs[IDX_UART_AUTOBAUD] = 0x801; // UART_AUTOBAUD(uart_num)
		UartDev.rcv_state = WAIT_SYNC_FRM;
	}
	while(uartregs[IDX_UART_PULSE_NUM] >= 29) { // UART_PULSE_NUM(uart_num)
		if(flg) return 0;
		ets_delay_us(1000);
	}
	uint32 ret = (((uartregs[IDX_UART_LOWPULSE] & 0xFFFFF) + (uartregs[IDX_UART_HIGHPULSE] & 0xFFFFF)) >> 1) + 12;  // UART_LOWPULSE(uart_num) + UART_HIGHPULSE(uart_num)
	uartregs[IDX_UART_AUTOBAUD] &= 0x7E; // UART_AUTOBAUD(uart_num)
	return ret;
}

// ROM:40003EF4
int UartGetCmdLn(uint8 *buf)
{
	if(UartDev.rcv_buff.BuffState == WRITE_OVER) {
		uint8 ch;
		if(uart_rx_readbuff(&UartDev.rcv_buff, &ch) == 0) {
			do {
				*buf++ = ch;
			} while(uart_rx_readbuff(&UartDev.rcv_buff, &ch)==0);
			*buf = '\0';
			UartDev.rcv_buff.BuffState = EMPTY;
			return 0;
		}
	}
	return 1;
}

// ROM:40003F4C
UartDevice *GetUartDevice(void)
{
	return UartDev;
}

// ROM:40003C80
int send_packet(uint8 *packet, uint8 size)
{
	uart_tx_one_char(0xC0);
	while(size--) {
		if(*packet == 0xC0) {
			uart_tx_one_char(0xDB);
			uart_tx_one_char(0xDC);
		}
		else if(*packet == 0xDB) {
			uart_tx_one_char(0xDB);
			uart_tx_one_char(0xDD);
		}
		else {
			uart_tx_one_char(*packet);
		}
		packet++;
	}
	return uart_tx_one_char(0xC0);
}

// ROM:40003CF4
int SendMsg(uint8 *msg, uint8 size)
{
	send_packet(msg, size);
	return 0;
}

// ROM:40001DCC
void _putc1(uint8 ch)
{
	if(ch == '\n') {
		uart_tx_one_char('\r');
		uart_tx_one_char('\n');
	}
	else if(ch != '\r') uart_tx_one_char(ch);
}

/*
// ROM:40003D08
recv_packet()
{
	...;
}

// ROM:40003EAC
RcvMsg()
{
	recv_packet();
}
*/
