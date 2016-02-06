/*
 * ESP8266 SPI registers
 */

#ifndef SPI_REGISTER_H_INCLUDED
#define SPI_REGISTER_H_INCLUDED

#define REG_SPI_BASE(i)			(0x60000200 - i*0x100)

#define SPI_CMD(i)				(REG_SPI_BASE(i) + 0x0) // 00000000/00000000
#define SPI_USR							(BIT(18))
#define SPI_READ						(BIT(31))
#define SPI_WREN						(BIT(30))
#define SPI_WRDI						(BIT(29))
#define SPI_RDID						(BIT(28))
#define SPI_RDSR						(BIT(27))
#define SPI_WRSR						(BIT(26))
#define SPI_PP							(BIT(25))
#define SPI_SE							(BIT(24))
#define SPI_BE							(BIT(23))
#define SPI_CE							(BIT(22))
#define SPI_DP							(BIT(21))
#define SPI_RES							(BIT(20))
#define SPI_HPM							(BIT(19))
#define SPI_USR							(BIT(18))

#define SPI_ADDR(i)				(REG_SPI_BASE(i) + 0x4) // 00000000/0801799a

#define SPI_CTRL(i)				(REG_SPI_BASE(i) + 0x8) // 0028b313/016ab000
#define SPI_WR_BIT_ORDER				(BIT(26))
#define SPI_RD_BIT_ORDER				(BIT(25))
#define SPI_QIO_MODE					(BIT(24))
#define SPI_DIO_MODE					(BIT(23))
#define SPI_TWO_BYTE_STATUS_EN			(BIT(22))
#define SPI_WP_REG						(BIT(21))
#define SPI_QOUT_MODE					(BIT(20))
#define SPI_SHARE_BUS					(BIT(19))
#define SPI_HOLD_MODE					(BIT(18))
#define SPI_ENABLE_AHB					(BIT(17))
#define SPI_SST_AAI						(BIT(16))
#define SPI_RESANDRES					(BIT(15))
#define SPI_DOUT_MODE					(BIT(14))
#define SPI_FASTRD_MODE					(BIT(13))
#define SPI_PRESCALER					0x0000FFFF
#define SPI_PRESCALER_S					0
// bit0..12: 80MHz = 0x1000, 40MHz = 0x0101, 26MHz = 0x0202, 20MHz = 0x0313
#define SPI_CTRL_F_MASK		(~0x1FFF)
#define SPI_CTRL_F80MHZ		0x1000		// + GPIO_MUX_CFG |= BIT(MUX_SPI0_CLK_BIT)
#define SPI_CTRL_F40MHZ		0x0101		// + GPIO_MUX_CFG &= ~(BIT(MUX_SPI0_CLK_BIT))
#define SPI_CTRL_F26MHZ		0x0202		// + GPIO_MUX_CFG &= ~(BIT(MUX_SPI0_CLK_BIT))
#define SPI_CTRL_F20MHZ		0x0313		// + GPIO_MUX_CFG &= ~(BIT(MUX_SPI0_CLK_BIT))

#define SPI_CTRL1(i)				(REG_SPI_BASE(i) + 0xC)  //+0x0C //5fff0120/5fff0120
#define SPI_CS_DELAY_OFF				0x0000000F
#define SPI_CS_DELAY_OFF_S				28
#define SPI_T_CSH						0x0000000F
#define SPI_T_CSH_S						28
#define SPI_T_RES						0x00000FFF
#define SPI_T_RES_S						16
#define SPI_BUS_TIMER_LIMIT				0x0000FFFF
#define SPI_BUS_TIMER_LIMIT_S			0

#define SPI_RD_STATUS(i)			(REG_SPI_BASE(i) + 0x10) //00000000/00000000
#define SPI_STATUS_EXT					0x000000FF
#define SPI_STATUS_EXT_S				24
#define SPI_WB_MODE						0x000000FF
#define SPI_WB_MODE_S					16
#define SPI_FLASH_STATUS_PRO_FLAG		(BIT(7))
#define SPI_FLASH_TOP_BOT_PRO_FLAG		(BIT(5))
#define SPI_FLASH_BP2					(BIT(4))
#define SPI_FLASH_BP1					(BIT(3))
#define SPI_FLASH_BP0					(BIT(2))
#define SPI_FLASH_WRENABLE_FLAG			(BIT(1))
#define SPI_FLASH_BUSY_FLAG				(BIT(0))

#define SPI_CTRL2(i)				(REG_SPI_BASE(i) + 0x14) //00000011/00000011
#define SPI_CS_DELAY_NUM				0x0000000F
#define SPI_CS_DELAY_NUM_S				28
#define SPI_CS_DELAY_MODE				0x00000003
#define SPI_CS_DELAY_MODE_S				26
#define SPI_MOSI_DELAY_NUM				0x00000007
#define SPI_MOSI_DELAY_NUM_S			23
#define SPI_MOSI_DELAY_MODE				0x00000003
#define SPI_MOSI_DELAY_MODE_S			21
#define SPI_MISO_DELAY_NUM				0x00000007
#define SPI_MISO_DELAY_NUM_S			18
#define SPI_MISO_DELAY_MODE				0x00000003
#define SPI_MISO_DELAY_MODE_S			16
#define SPI_CK_OUT_HIGH_MODE			0x0000000F
#define SPI_CK_OUT_HIGH_MODE_S			12
#define SPI_CK_OUT_LOW_MODE				0x0000000F
#define SPI_CK_OUT_LOW_MODE_S			8
#define SPI_HOLD_TIME					0x0000000F
#define SPI_HOLD_TIME_S					4
#define SPI_SETUP_TIME					0x0000000F
#define SPI_SETUP_TIME_S				0

#define SPI_CLOCK(i)				(REG_SPI_BASE(i) + 0x18) //80003043/80000000
#define SPI_CLK_EQU_SYSCLK				(BIT(31))
#define SPI_CLKDIV_PRE					0x00001FFF
#define SPI_CLKDIV_PRE_S				18
#define SPI_CLKCNT_N					0x0000003F
#define SPI_CLKCNT_N_S					12
#define SPI_CLKCNT_H					0x0000003F
#define SPI_CLKCNT_H_S					6
#define SPI_CLKCNT_L					0x0000003F
#define SPI_CLKCNT_L_S					0

#define SPI_USER(i)					(REG_SPI_BASE(i) + 0x1C) // 80000044/80000064
#define SPI_USR_COMMAND					(BIT(31))
#define SPI_USR_ADDR					(BIT(30))
#define SPI_USR_DUMMY					(BIT(29))
#define SPI_USR_MISO					(BIT(28))
#define SPI_USR_MOSI					(BIT(27))
#define SPI_USR_MOSI_HIGHPART			(BIT(25))
#define SPI_USR_MISO_HIGHPART			(BIT(24))
#define SPI_USR_PREP_HOLD				(BIT(23))
#define SPI_USR_CMD_HOLD				(BIT(22))
#define SPI_USR_ADDR_HOLD				(BIT(21))
#define SPI_USR_DUMMY_HOLD				(BIT(20))
#define SPI_USR_DIN_HOLD				(BIT(19))
#define SPI_USR_DOUT_HOLD				(BIT(18))
#define SPI_USR_HOLD_POL				(BIT(17))
#define SPI_SIO							(BIT(16))
#define SPI_FWRITE_QIO					(BIT(15))
#define SPI_FWRITE_DIO					(BIT(14))
#define SPI_FWRITE_QUAD					(BIT(13))
#define SPI_FWRITE_DUAL					(BIT(12))
#define SPI_WR_BYTE_ORDER				(BIT(11))
#define SPI_RD_BYTE_ORDER				(BIT(10))
#define SPI_AHB_ENDIAN_MODE				0x00000003
#define SPI_AHB_ENDIAN_MODE_S			8
#define SPI_CK_OUT_EDGE					(BIT(7))
#define SPI_CK_I_EDGE					(BIT(6))
#define SPI_CS_SETUP					(BIT(5)) // +1 такт перед CS
#define SPI_CS_HOLD						(BIT(4))
#define SPI_AHB_USR_COMMAND				(BIT(3))
#define SPI_FLASH_MODE					(BIT(2))
#define SPI_AHB_USR_COMMAND_4BYTE		(BIT(1))
#define SPI_DOUTDIN						(BIT(0))

#define SPI_USER1(i)				(REG_SPI_BASE(i) + 0x20) //5c000000/5c7e3f1f
#define SPI_USR_ADDR_BITLEN				0x0000003F
#define SPI_USR_ADDR_BITLEN_S			26
#define SPI_USR_MOSI_BITLEN				0x000001FF
#define SPI_USR_MOSI_BITLEN_S			17
#define SPI_USR_MISO_BITLEN				0x000001FF
#define SPI_USR_MISO_BITLEN_S			8
#define SPI_USR_DUMMY_CYCLELEN			0x000000FF
#define SPI_USR_DUMMY_CYCLELEN_S		0

#define SPI_USER2(i)				(REG_SPI_BASE(i) + 0x24) //70000000/70000000
#define SPI_USR_COMMAND_BITLEN			0x0000000F
#define SPI_USR_COMMAND_BITLEN_S		28
#define SPI_USR_COMMAND_VALUE			0x0000FFFF
#define SPI_USR_COMMAND_VALUE_S			0

#define SPI_WR_STATUS(i)			(REG_SPI_BASE(i) + 0x28) //00000000/00000000

#define SPI_PIN(i)					(REG_SPI_BASE(i) + 0x2C) //0000001e/0000001e
#define SPI_PIN_CS_0_					(BIT(31))	// CS = "0"
#define SPI_PIN_CS_0					(BIT(30))	// CS = "0"
#define SPI_PIN_CLK_INV					(BIT(29))	// инверсия сигнала CLK
#define SPI_PIN_CLK_CS					(BIT(19))	// CS = "0", CLK = "1"
#define SPI_PIN_MOSI_MISO				(BIT(16))	// сигнал MISO выводится на MOSI
#define SPI_PIN_CS_CLK					(BIT(11))	// сигнал на CS = CLK
#define SPI_PIN_CS_INV 					(BIT(6))	// инверсия сигнала CS
#define SPI_PIN_CLK_0 					(BIT(5))	// Выход CLK = "0"
#define SPI_CS2_DIS						(BIT(2))
#define SPI_CS1_DIS						(BIT(1))
#define SPI_CS0_DIS						(BIT(0))

#define SPI_SLAVE(i)				(REG_SPI_BASE(i)  + 0x30) // 00000200/00000210
#define SPI_SYNC_RESET					(BIT(31))
#define SPI_SLAVE_MODE					(BIT(30))
#define SPI_SLV_WR_RD_BUF_EN			(BIT(29))
#define SPI_SLV_WR_RD_STA_EN			(BIT(28))
#define SPI_SLV_CMD_DEFINE				(BIT(27))
#define SPI_TRANS_CNT					0x0000000F
#define SPI_TRANS_CNT_S					23
#define SPI_SLV_LAST_STATE				0x00000007
#define SPI_SLV_LAST_STATE_S			20
#define SPI_SLV_LAST_COMMAND			0x00000007
#define SPI_SLV_LAST_COMMAND_S			17
#define SPI_CS_I_MODE					0x00000003
#define SPI_CS_I_MODE_S					10
#define SPI_TRANS_DONE_EN				(BIT(9))
#define SPI_SLV_WR_STA_DONE_EN			(BIT(8))
#define SPI_SLV_RD_STA_DONE_EN			(BIT(7))
#define SPI_SLV_WR_BUF_DONE_EN			(BIT(6))
#define SPI_SLV_RD_BUF_DONE_EN			(BIT(5))
#define SLV_SPI_INT_EN					0x0000001f
#define SLV_SPI_INT_EN_S				5
#define SPI_TRANS_DONE					(BIT(4))
#define SPI_SLV_WR_STA_DONE				(BIT(3))
#define SPI_SLV_RD_STA_DONE				(BIT(2))
#define SPI_SLV_WR_BUF_DONE				(BIT(1))
#define SPI_SLV_RD_BUF_DONE				(BIT(0))

#define SPI_SLAVE1(i)				(REG_SPI_BASE(i) + 0x34) //02000000/02000000
#define SPI_SLV_STATUS_BITLEN			0x0000001F
#define SPI_SLV_STATUS_BITLEN_S			27
#define SPI_SLV_STATUS_FAST_EN			(BIT(26))
#define SPI_SLV_STATUS_READBACK			(BIT(25))
#define SPI_SLV_BUF_BITLEN				0x000001FF
#define SPI_SLV_BUF_BITLEN_S			16
#define SPI_SLV_RD_ADDR_BITLEN			0x0000003F
#define SPI_SLV_RD_ADDR_BITLEN_S		10
#define SPI_SLV_WR_ADDR_BITLEN			0x0000003F
#define SPI_SLV_WR_ADDR_BITLEN_S		4
#define SPI_SLV_WRSTA_DUMMY_EN			(BIT(3))
#define SPI_SLV_RDSTA_DUMMY_EN			(BIT(2))
#define SPI_SLV_WRBUF_DUMMY_EN			(BIT(1))
#define SPI_SLV_RDBUF_DUMMY_EN			(BIT(0))

#define SPI_SLAVE2(i)				(REG_SPI_BASE(i) + 0x38) //00000000/00000000
#define SPI_SLV_WRBUF_DUMMY_CYCLELEN	0x000000FF
#define SPI_SLV_WRBUF_DUMMY_CYCLELEN_S  24
#define SPI_SLV_RDBUF_DUMMY_CYCLELEN	0x000000FF
#define SPI_SLV_RDBUF_DUMMY_CYCLELEN_S  16
#define SPI_SLV_WRSTR_DUMMY_CYCLELEN	0x000000FF
#define SPI_SLV_WRSTR_DUMMY_CYCLELEN_S  8
#define SPI_SLV_RDSTR_DUMMY_CYCLELEN	0x000000FF
#define SPI_SLV_RDSTR_DUMMY_CYCLELEN_S  0

#define SPI_SLAVE3(i)				(REG_SPI_BASE(i) + 0x3C) //00000000/00000000
#define SPI_SLV_WRSTA_CMD_VALUE			0x000000FF
#define SPI_SLV_WRSTA_CMD_VALUE_S		24
#define SPI_SLV_RDSTA_CMD_VALUE			0x000000FF
#define SPI_SLV_RDSTA_CMD_VALUE_S		16
#define SPI_SLV_WRBUF_CMD_VALUE			0x000000FF
#define SPI_SLV_WRBUF_CMD_VALUE_S		8
#define SPI_SLV_RDBUF_CMD_VALUE			0x000000FF
#define SPI_SLV_RDBUF_CMD_VALUE_S		0

#define SPI_W0(i)					(REG_SPI_BASE(i) + 0x40)
#define SPI_W1(i)					(REG_SPI_BASE(i) + 0x44)
#define SPI_W2(i)					(REG_SPI_BASE(i) + 0x48)
#define SPI_W3(i)					(REG_SPI_BASE(i) + 0x4C)
#define SPI_W4(i)					(REG_SPI_BASE(i) + 0x50)
#define SPI_W5(i)					(REG_SPI_BASE(i) + 0x54)
#define SPI_W6(i)					(REG_SPI_BASE(i) + 0x58)
#define SPI_W7(i)					(REG_SPI_BASE(i) + 0x5C)
#define SPI_W8(i)					(REG_SPI_BASE(i) + 0x60)
#define SPI_W9(i)					(REG_SPI_BASE(i) + 0x64)
#define SPI_W10(i)					(REG_SPI_BASE(i) + 0x68)
#define SPI_W11(i)					(REG_SPI_BASE(i) + 0x6C)
#define SPI_W12(i)					(REG_SPI_BASE(i) + 0x70)
#define SPI_W13(i)					(REG_SPI_BASE(i) + 0x74)
#define SPI_W14(i)					(REG_SPI_BASE(i) + 0x78)
#define SPI_W15(i)					(REG_SPI_BASE(i) + 0x7C)

#define SPI_FLASH_EXT0(i)			(REG_SPI_BASE(i)  + 0xF0) // 800a0050/800a0050
#define SPI_T_PP_ENA					(BIT(31))
#define SPI_T_PP_SHIFT					0x0000000F
#define SPI_T_PP_SHIFT_S				16
#define SPI_T_PP_TIME					0x00000FFF
#define SPI_T_PP_TIME_S					0

#define SPI_FLASH_EXT1(i)			(REG_SPI_BASE(i) + 0xF4) // 800f0258/800f0258
#define SPI_T_ERASE_ENA					(BIT(31))
#define SPI_T_ERASE_SHIFT				0x0000000F
#define SPI_T_ERASE_SHIFT_S				16
#define SPI_T_ERASE_TIME				0x00000FFF
#define SPI_T_ERASE_TIME_S				0

#define SPI_FLASH_EXT2(i)			(REG_SPI_BASE(i) + 0xF8)
#define SPI_ST 							0x00000007
#define SPI_ST_S						0

#define SPI_EXT3(i)					(REG_SPI_BASE(i) + 0xFC) //00000000/00000000
#define SPI_INT_HOLD_ENA				0x00000003
#define SPI_INT_HOLD_ENA_S				0

#endif // SPI_REGISTER_H_INCLUDED
