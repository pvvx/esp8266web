/******************************************************************************
 * FileName: uartdev.h
 * Description: UART funcs in ROM-BIOS
 * Alternate SDK
 * Author: PV`
 * (c) PV` 2015
*******************************************************************************/
#ifndef _INCLUDE_BIOS_UARTDEV_H_
#define _INCLUDE_BIOS_UARTDEV_H_

#include "c_types.h"

#define DEFAULT_BIOS_UART_BAUD 74880

#define CALK_UART_CLKDIV(B) (UART_CLK_FREQ + B / 2) / B
#define CALK_UART_CLKDIV_26QZ(B) (52000000 + B / 2) / B

#define RX_BUFF_SIZE    0x100
#define TX_BUFF_SIZE    100
#define UART0   0
#define UART1   1

typedef enum {
    FIVE_BITS = 0x0,
    SIX_BITS = 0x1,
    SEVEN_BITS = 0x2,
    EIGHT_BITS = 0x3
} UartBitsNum4Char;

typedef enum {
    ONE_STOP_BIT             = 0,
    ONE_HALF_STOP_BIT        = BIT(2),
    TWO_STOP_BIT             = BIT(2)
} UartStopBitsNum;

typedef enum {
    NONE_BITS = 0,
    ODD_BITS   = 0,
    EVEN_BITS = BIT(4)
} UartParityMode;

typedef enum {
    STICK_PARITY_DIS   = 0,
    STICK_PARITY_EN    = BIT(3) | BIT(5)
} UartExistParity;

typedef enum {
    BIT_RATE_9600     = 9600,
    BIT_RATE_19200   = 19200,
    BIT_RATE_38400   = 38400,
    BIT_RATE_57600   = 57600,
    BIT_RATE_74880   = 74880,
    BIT_RATE_115200 = 115200,
    BIT_RATE_230400 = 230400,
	BIT_RATE_256000 = 256000,
    BIT_RATE_460800 = 460800,
    BIT_RATE_921600 = 921600
} UartBautRate;

typedef enum {
    NONE_CTRL,
    HARDWARE_CTRL,
    XON_XOFF_CTRL
} UartFlowCtrl;

typedef enum {
    EMPTY,
    UNDER_WRITE,
    WRITE_OVER
} RcvMsgBuffState;

typedef struct {
    uint32     RcvBuffSize; 	//+0 	//+18 = 0x100 = 256 (dec)
    uint8     *pRcvMsgBuff; 	//+4 	//+1C = 0x3fffde60
    uint8     *pWritePos;  		//+8 	//+20 = 0x3fffde60
    uint8	  *pReadPos; 		//+C 	//+24 = 0x3fffde60
    uint8      TrigLvl; 		//+10 	//+28
    RcvMsgBuffState  BuffState; //+14	//+2C
} RcvMsgBuff;

typedef struct {
    uint32   TrxBuffSize; //+30 0x64 = 100 (dec)
    uint8   *pTrxBuff;    //+34 0x3fffdf60
} TrxMsgBuff;

typedef enum {
    BAUD_RATE_DET,
    WAIT_SYNC_FRM,
    SRCH_MSG_HEAD,
    RCV_MSG_BODY,
    RCV_ESC_CHAR,
} RcvMsgState;

typedef struct {
    UartBautRate 	    baut_rate; 		//+00 Init Value
    UartBitsNum4Char	data_bits; 		//+04 = 0x03
    UartExistParity		exist_parity; 	//+08 = 0x0
    UartParityMode		parity;    		//+0C = 0x0
    UartStopBitsNum		stop_bits; 		//+10 = 0x0
    UartFlowCtrl		flow_ctrl; 		//+14 = 0x0
    RcvMsgBuff			rcv_buff;		//+18
    TrxMsgBuff			trx_buff;		//+30
    RcvMsgState			rcv_state; 		//+38 = 0x00
    int		received;					//+3C = 0x00
    int		buff_uart_no;  				//+40 = 0x00
} UartDevice; // 0x3fffde10

extern UartDevice UartDev; // UartDev is defined and initialized in rom code.

/* eagle.rom.addr.v6.ld
PROVIDE ( UartConnCheck = 0x40003230 );
PROVIDE ( UartConnectProc = 0x400037a0 );
PROVIDE ( UartDwnLdProc = 0x40003368 );
PROVIDE ( UartRegReadProc = 0x4000381c );
PROVIDE ( UartRegWriteProc = 0x400037ac );
PROVIDE ( UartRxString = 0x40003c30 );
PROVIDE ( Uart_Init = 0x40003a14 );
PROVIDE ( recv_packet = 0x40003d08 );
PROVIDE ( RcvMsg = 0x40003eac );

PROVIDE ( GetUartDevice = 0x40003f4c );
PROVIDE ( UartGetCmdLn = 0x40003ef4 );
PROVIDE ( SendMsg = 0x40003cf4 );
PROVIDE ( send_packet = 0x40003c80 );

PROVIDE ( UartDev = 0x3fffde10 );
PROVIDE ( uartAttach = 0x4000383c );
PROVIDE ( uart_baudrate_detect = 0x40003924 );
PROVIDE ( uart_buff_switch = 0x400038a4 );
PROVIDE ( uart_div_modify = 0x400039d8 );
PROVIDE ( uart_rx_intr_handler = 0x40003bbc );
PROVIDE ( uart_rx_one_char = 0x40003b8c );
PROVIDE ( uart_rx_one_char_block = 0x40003b64 );
PROVIDE ( uart_rx_readbuff = 0x40003ec8 );
PROVIDE ( uart_tx_one_char = 0x40003b30 );

PROVIDE ( _putc1 = 0x40001dcc);
*/

UartDevice *GetUartDevice(void);
void uartAttach(void);
void uart_rx_intr_handler(void *par);
int uart_rx_one_char(uint8 *ch);
uint8 uart_rx_one_char_block(void);
int uart_rx_readbuff(RcvMsgBuff * rcvmsg, uint8 *dst);
int uart_tx_one_char(uint8 ch);
void uart_div_modify(uint32 uart_num, uint32 div_baud);
uint32 uart_baudrate_detect(uint32 uart_num, uint32 flg);
void uart_buff_switch(uint32 uart_num);

int UartGetCmdLn(uint8 *buf);
int SendMsg(uint8 *msg, uint8 size);
int send_packet(uint8 *packet, uint8 size);

void _putc1(uint8 ch);

#endif /* _INCLUDE_BIOS_UARTDEV_H_ */
