/*
 * ESP8266 UART registers
 */

#ifndef UART_REGISTER_H_INCLUDED
#define UART_REGISTER_H_INCLUDED
#define REG_UART_BASE( i )  	(0x60000000+(i)*0xf00)
//version value:32'h062000

#define UART_FIFO( i )			(REG_UART_BASE( i ) + 0x0)
#define UART_RXFIFO_RD_BYTE			0x000000FF
#define UART_RXFIFO_RD_BYTE_S		0			// R/W share the same address

#define UART_INT_RAW( i )		(REG_UART_BASE( i ) + 0x4)
#define UART_RXFIFO_TOUT_INT_RAW	(BIT(8))	// The interrupt raw bit for Rx time-out interrupt(depands on the UART_RX_TOUT_THRHD)
#define UART_BRK_DET_INT_RAW		(BIT(7))	// The interrupt raw bit for Rx byte start error
#define UART_CTS_CHG_INT_RAW		(BIT(6))	// The interrupt raw bit for CTS changing level
#define UART_DSR_CHG_INT_RAW		(BIT(5))	// The interrupt raw bit for DSR changing level
#define UART_RXFIFO_OVF_INT_RAW		(BIT(4))	// The interrupt raw bit for rx fifo overflow
#define UART_FRM_ERR_INT_RAW		(BIT(3))	// The interrupt raw bit for other rx error
#define UART_PARITY_ERR_INT_RAW		(BIT(2))	// The interrupt raw bit for parity check error
#define UART_TXFIFO_EMPTY_INT_RAW	(BIT(1))	// The interrupt raw bit for tx fifo empty interrupt(depands on UART_TXFIFO_EMPTY_THRHD bits)
#define UART_RXFIFO_FULL_INT_RAW	(BIT(0))	// The interrupt raw bit for rx fifo full interrupt(depands on UART_RXFIFO_FULL_THRHD bits)

#define UART_INT_ST( i )		(REG_UART_BASE( i ) + 0x8)
#define UART_RXFIFO_TOUT_INT_ST		(BIT(8))	// The interrupt state bit for Rx time-out event
#define UART_BRK_DET_INT_ST			(BIT(7))	// The interrupt state bit for rx byte start error
#define UART_CTS_CHG_INT_ST			(BIT(6))	// The interrupt state bit for CTS changing level
#define UART_DSR_CHG_INT_ST			(BIT(5))	// The interrupt state bit for DSR changing level
#define UART_RXFIFO_OVF_INT_ST		(BIT(4))	// The interrupt state bit for RX fifo overflow
#define UART_FRM_ERR_INT_ST			(BIT(3))	// The interrupt state for other rx error
#define UART_PARITY_ERR_INT_ST		(BIT(2))	// The interrupt state bit for rx parity error
#define UART_TXFIFO_EMPTY_INT_ST	(BIT(1))	// The interrupt state bit for TX fifo empty
#define UART_RXFIFO_FULL_INT_ST		(BIT(0))	// The interrupt state bit for RX fifo full event

#define UART_INT_ENA( i )		(REG_UART_BASE( i ) + 0xC)
#define UART_RXFIFO_TOUT_INT_ENA	(BIT(8))	// The interrupt enable bit for rx time-out interrupt
#define UART_BRK_DET_INT_ENA		(BIT(7))	// The interrupt enable bit for rx byte start error
#define UART_CTS_CHG_INT_ENA		(BIT(6))	// The interrupt enable bit for CTS changing level
#define UART_DSR_CHG_INT_ENA		(BIT(5))	// The interrupt enable bit for DSR changing level
#define UART_RXFIFO_OVF_INT_ENA		(BIT(4))	// The interrupt enable bit for rx fifo overflow
#define UART_FRM_ERR_INT_ENA		(BIT(3))	// The interrupt enable bit for other rx error
#define UART_PARITY_ERR_INT_ENA		(BIT(2))	// The interrupt enable bit for parity error
#define UART_TXFIFO_EMPTY_INT_ENA	(BIT(1))	// The interrupt enable bit for tx fifo empty event
#define UART_RXFIFO_FULL_INT_ENA	(BIT(0))	// The interrupt enable bit for rx fifo full event

#define UART_INT_CLR( i )		(REG_UART_BASE( i ) + 0x10)
#define UART_RXFIFO_TOUT_INT_CLR	(BIT(8))	// Set this bit to clear the rx time-out interrupt
#define UART_BRK_DET_INT_CLR		(BIT(7))	// Set this bit to clear the rx byte start interrupt
#define UART_CTS_CHG_INT_CLR		(BIT(6))	// Set this bit to clear the CTS changing interrupt
#define UART_DSR_CHG_INT_CLR		(BIT(5))	// Set this bit to clear the DSR changing interrupt
#define UART_RXFIFO_OVF_INT_CLR		(BIT(4))	// Set this bit to clear the rx fifo over-flow interrupt
#define UART_FRM_ERR_INT_CLR		(BIT(3))	// Set this bit to clear other rx error interrupt
#define UART_PARITY_ERR_INT_CLR		(BIT(2))	// Set this bit to clear the parity error interrupt
#define UART_TXFIFO_EMPTY_INT_CLR	(BIT(1))	// Set this bit to clear the tx fifo empty interrupt
#define UART_RXFIFO_FULL_INT_CLR	(BIT(0))	// Set this bit to clear the rx fifo full interrupt

#define UART_CLKDIV( i )		(REG_UART_BASE( i ) + 0x14)
#define UART_CLKDIV_CNT				0x000FFFFF
#define UART_CLKDIV_S				0			// BAUDRATE = UART_CLK_FREQ / UART_CLKDIV

#define UART_AUTOBAUD( i )		(REG_UART_BASE( i ) + 0x18)
#define UART_GLITCH_FILT			0x000000FF
#define UART_GLITCH_FILT_S			8
#define UART_AUTOBAUD_EN			(BIT(0))	// Set this bit to enable baudrate detect

#define UART_STATUS( i )		(REG_UART_BASE( i ) + 0x1C)
#define UART_TXD					(BIT(31))	// The level of the uart txd pin
#define UART_RTSN					(BIT(30))	// The level of uart rts pin
#define UART_DTRN					(BIT(29))	// The level of uart dtr pin
#define UART_TXFIFO_CNT				0x000000FF
#define UART_TXFIFO_CNT_S			16			// Number of data in UART TX fifo (0..127)
#define UART_RXD					(BIT(15))	// The level of uart rxd pin
#define UART_CTSN					(BIT(14))	// The level of uart cts pin
#define UART_DSRN					(BIT(13))	// The level of uart dsr pin
#define UART_RXFIFO_CNT				0x000000FF
#define UART_RXFIFO_CNT_S			0		// Number of data in uart rx fifo (0..127)

#define UART_CONF0( i )			(REG_UART_BASE( i ) + 0x20)
#define UART_DTR_INV				(BIT(24))	// Set this bit to inverse uart dtr level
#define UART_RTS_INV				(BIT(23))	// Set this bit to inverse uart rts level
#define UART_TXD_INV				(BIT(22))	// Set this bit to inverse uart txd level
#define UART_DSR_INV				(BIT(21))	// Set this bit to inverse uart dsr level
#define UART_CTS_INV				(BIT(20))	// Set this bit to inverse uart cts level
#define UART_RXD_INV				(BIT(19))	// Set this bit to inverse uart rxd level
#define UART_TXFIFO_RST				(BIT(18))	// Set this bit to reset uart tx fifo
#define UART_RXFIFO_RST				(BIT(17))	// Set this bit to reset uart rx fifo
#define UART_IRDA_EN				(BIT(16))
#define UART_TX_FLOW_EN				(BIT(15))	// Set this bit to enable uart tx hardware flow control
#define UART_LOOPBACK				(BIT(14))	// Set this bit to enable uart loopback test mode (not flow control)
#define UART_IRDA_RX_INV			(BIT(13))
#define UART_IRDA_TX_INV			(BIT(12))
#define UART_IRDA_WCTL				(BIT(11))
#define UART_IRDA_TX_EN				(BIT(10))
#define UART_IRDA_DPLX				(BIT(9))
#define UART_TXD_BRK				(BIT(8))	// Set this bit to send a tx break signal(need fifo reset first)
#define UART_SW_DTR					(BIT(7))	// sw dtr
#define UART_SW_RTS					(BIT(6))	// sw rts
#define UART_STOP_BIT_NUM			0x00000003
#define UART_STOP_BIT_NUM_S			4			// Set stop bit: 1:1bit  2:1.5bits  3:2bits
#define UART_BIT_NUM				0x00000003
#define UART_BIT_NUM_S				2			// Set bit num:  0:5bits 1:6bits 2:7bits 3:8bits
#define UART_PARITY_EN				(BIT(1))	// Set this bit to enable uart parity check
#define UART_PARITY 				(BIT(0))	// Set parity check:  0:even 1:odd

#define UART_CONF1( i )			(REG_UART_BASE( i ) + 0x24)
#define UART_RX_TOUT_EN 			(BIT(31))	// Set this bit to enable rx time-out function
#define UART_RX_TOUT_THRHD 			0x0000007F
#define UART_RX_TOUT_THRHD_S 		24			// The config bits for for rx time-out threshold, 0-127
#define UART_RX_FLOW_EN 			(BIT(23))	// Set this bit to enable rx flow control threshold
#define UART_RX_FLOW_THRHD			0x0000007F
#define UART_RX_FLOW_THRHD_S 		16			// The config bits for rx flow control threshold, 0-127
#define UART_TXFIFO_EMPTY_THRHD		0x0000007F
#define UART_TXFIFO_EMPTY_THRHD_S	8			// The config bits for tx fifo empty threshold, 0-127
#define UART_RXFIFO_FULL_THRHD 		0x0000007F
#define UART_RXFIFO_FULL_THRHD_S 	0			// The config bits for rx fifo full threshold, 0-127

#define UART_LOWPULSE( i )		(REG_UART_BASE( i ) + 0x28)
#define UART_LOWPULSE_MIN_CNT		0x000FFFFF
#define UART_LOWPULSE_MIN_CNT_S		0			// used in baudrate detect

#define UART_HIGHPULSE( i )		(REG_UART_BASE( i ) + 0x2C)
#define UART_HIGHPULSE_MIN_CNT		0x000FFFFF
#define UART_HIGHPULSE_MIN_CNT_S	0			// used in baudrate detect

#define UART_PULSE_NUM( i )		(REG_UART_BASE( i ) + 0x30)
#define UART_PULSE_NUM_CNT			0x0003FF
#define UART_PULSE_NUM_CNT_S		0			// used in baudrate detect

#define UART_DATE( i )			(REG_UART_BASE( i ) + 0x78)
#define UART_ID( i )			(REG_UART_BASE( i ) + 0x7C)
#endif // UART_REGISTER_H_INCLUDED
