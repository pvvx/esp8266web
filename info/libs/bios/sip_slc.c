/******************************************************************************
 * FileName: sip_slc.c
 * Description: functions in ROM-BIOS
 * Alternate SDK
 * Author: PV`
*******************************************************************************/
#include "c_types.h"
#include "hw/esp8266.h"
#include "hw/slc_register.h"

struct sdio_queue
{
	uint32	blocksize:12;
	uint32	datalen:12;
	uint32	unused:5;
	uint32	sub_sof:1;
	uint32 	eof:1;
	uint32	owner:1;

	uint32	buf_ptr;		// +0x08
	uint32	next_link_ptr;	// +0x0c
};

struct sdio_slave_status_element
{
	uint32 wr_busy:1;
	uint32 rd_empty :1;
	uint32 comm_cnt :3;
	uint32 intr_no :3;
	uint32 rx_length:16;
	uint32 res:8;
};

typedef void (* slc_func_var_int)(int);
struct s_slc_ptr {
	uint32 unk0x00;
	uint32 tx_buf_ptr;	// +0x04;
	uint32 rx_buf_ptr;	// +0x08;
	struct sdio_queue *  unk0x0c;		// +0x0C
	uint32 unk0x10;		// +0x10
	slc_func_var_int func_rx; // +0x14
	slc_func_var_int func_tx; // +0x18
	uint32 unk0x1C;		// +0x1C
	void * func_var;	// +0x20
	uint8 	ena_host;	// +0x24
	uint32 gpio_mode;	// +0x28
	uint16 gpio_mask;	// +0x2C
};


// RAM_BIOS:3FFFE1D0
//uint8 slc_ptr[];
struct s_slc_ptr slc_ptr;

// RAM_BIOS:3FFFE1E4
uint32 slc_data[];

// ROM:40005C50
void slc_init_attach(uint32 a2, uint32 a3, uint32 a4, uint32 a5)
{
	uint32 x

	slc_data[0] = a2;
	slc_data[1] = a3;
	slc_data[3] = a4;

	if(a5 == 0) {
		SDIO_BASE[1] = 0x2321007; // *0x60000A04 = 0x2321007; // bit12 =1 SDIO dataoutput is at negative edges (SDIO V1.1)
	}
	else if(a5 == 1) {
		SDIO_BASE[1] = 0x2320007; // *0x60000A04 = 0x2320007;
	}
	else if(a5 == 2) {
		SDIO_BASE[1] = (SDIO_BASE[1] & 0xFFFF0000) | 3;
	}
	else if(a5 == 3) {
		SDIO_BASE[1] = (SDIO_BASE[1] & 0xFFFF0000) | 0x2003; // bit13 =1 SDIO dataoutput is at positive edges (SDIO V2.0)
	}
	SDIO_BASE[0] = 0x11116666;
	slc_set_host_io_max_window();
	SLC_CONF0 = SLC_RXLINK_RST | SLC_TXLINK_RST;
	uint32 x = SLC_CONF0;
	SLC_CONF0 = 0;
	SLC_CONF0;
	SLC_CONF0 |= SLC_TX_LOOP_TEST | SLC_DATA_BURST_EN | SLC_DSCR_BURST_EN;
	slc_ptr.unk0x00 = 0x104;
	lldesc_build_chain(*0x3FFFE200, 0x60, 0x3FFFE260, 0x820);

//  ?????
//	slc_ptr.unk0x0c = ?;
//	slc_ptr.tx_buf_ptr = ?; // a1 + 0x10

	SLC_TX_LINK = SLC_TXLINK_STOP;
	while(SLC_TX_LINK);

	SLC_TX_LINK = ((slc_ptr.tx_buf_ptr & SLC_TXLINK_DESCADDR_MASK) << SLC_TXLINK_ADDR_S) | SLC_TXLINK_START;
	SLC_TX_LINK;
	SLC_RX_DSCR_CONF |= SLC_RX_FILL_MODE;
	SLC_HOST_INTR_ENA = SLC_RX_NEW_PACKET_INT_ENA;
	SLC_HOST_INTR_CLR = 0x7F;
	SLC_INT_CLR = 0x7F;
}

// ROM:40005D90
void slc_enable(void)
{
	ets_isr_attach(ETS_SLC_INUM, slc_isr_int, NULL);
	SLC_INT_ENA = SLC_RX_UDF_INT_ENA | SLC_TX_EOF_INT_ENA | SLC_RX_EOF_INT_ENA | SLC_TX_DSCR_ERR_INT_ENA | SLC_TX_DSCR_EMPTY_INT_ENA; // *0x60000B0C = 0x2A8400
	ets_isr_unmask(1 << ETS_SLC_INUM);
}

// ROM:40005DB8
uint32 slc_select_tohost_gpio_mode(void)
{
	return slc_ptr.gpio_mode;
}

// ROM:40005DC0
void slc_select_tohost_gpio(uint32 x)
{
	if(x >= 16) {
		slc_ptr.gpio_mask = 0;
	}
	else slc_ptr.gpio_mask = 1 << x;
}

// ROM:40005DE4
slc_send_to_host_chain(uint32 rx_buf_ptr, uint32 a3)
{
	slc_ptr.rx_buf_ptr = rx_buf_ptr;
	slc_ptr.unk0x10 = a3;
	SLC_RX_LINK = (rx_buf_ptr & SLC_RXLINK_DESCADDR_MASK) | SLC_RXLINK_START;
	if(slc_ptr.gpio_mode == 1){
		if((gpio_input_get() &	slc_ptr.gpio_mask) & 0xFFFF) {
			GPIO_OUT_W1TC = slc_ptr.gpio_mask;
		}
		else {
			GPIO_OUT_W1TS = slc_ptr.gpio_mask;
		}
	}
	else if(slc_ptr.gpio_mode == 2){
		SLC_INT_ENA |= SLC_RX_DONE_INT_ENA;
		GPIO_OUT_W1TS = slc_ptr.gpio_mask;
	}
	else if(slc_ptr.gpio_mode == 3){
		SLC_INT_ENA |= SLC_RX_DONE_INT_ENA;
		GPIO_OUT_W1TC = slc_ptr.gpio_mask;
	}
}
// ROM:40005E94
void slc_from_host_chain_recycle(uint32 tx_buf_ptr, struct sdio_queue * gg, uint32 x)
{
	bool flg;
	if(slc_ptr.unk0x0c != NULL) {
		ets_intr_lock();
		slc_ptr.unk0x0c->buf_ptr = tx_buf_ptr;
		slc_ptr.unk0x0c = gg;
		ets_intr_unlock();
	}
	else {
		ets_intr_lock();
		slc_ptr.tx_buf_ptr = tx_buf_ptr;
		slc_ptr.unk0x0c = gg;
		ets_intr_unlock();
		SLC_TX_LINK = (tx_buf_ptr & SLC_TXLINK_DESCADDR_MASK) | SLC_TXLINK_START;
	}
	slc_add_credits(x);
}

// ROM:40005F10
void slc_to_host_chain_recycle(uint32 buf_ptr, struct sdio_queue * rx)
{
	buf_ptr = slc_ptr.rx_buf_ptr;
	rx = (struct sdio_queue *) SLC_RX_EOF_DES_ADDR;
}

// ROM:40005F24
void slc_from_host_chain_fetch(uint32 buf_ptr, struct sdio_queue * tx)
{
	buf_ptr = slc_ptr.tx_buf_ptr;
	tx = (struct sdio_queue*) SLC_TX_EOF_DES_ADDR;
	slc_ptr.tx_buf_ptr = tx->buf_ptr;
	tx->next_link_ptr = NULL;
}

// ROM:40005F50
void slc_isr_int(void)
{
	uint32 istat;
	do {
		istat = SLC_INT_STATUS;
		if(istat) {
			if(istat & SLC_RX_EOF_INT_ST) {
				SLC_INT_CLR = SLC_RX_EOF_INT_CLR;
				SLC_INT_CLR;
				slc_ptr.func_rx(slc_ptr.func_var);
			}
			if(istat & SLC_TX_EOF_INT_ST) {
				SLC_INT_CLR = SLC_TX_EOF_INT_CLR;
				slc_ptr.func_tx(slc_ptr.func_var);
			}
			if(istat & SLC_RX_DONE_INT_ST) {
				if(slc_ptr.gpio_mode == 2) {
					GPIO_OUT_W1TC = slc_ptr.gpio_mask;
				}
				else if(slc_ptr.gpio_mode == 3) {
					GPIO_OUT_W1TS = slc_ptr.gpio_mask;
				}
				SLC_INT_ENA &= ~SLC_RX_DONE_INT_ENA;
				SLC_INT_CLR = SLC_RX_DONE_INT_CLR;
				SLC_INT_CLR;
			}
			if(istat & (SLC_TX_DSCR_EMPTY_INT_ST | SLC_TX_DSCR_ERR_INT_ST)) break;
		}
	}
	while((istat & SLC_RX_UDF_INT_ST)==0);
}

// ROM:40006014
uint32 slc_pause_from_host(void)
{
	slc_ptr.ena_host = 0;
	SLC_INT_ENA &= ~SLC_TX_EOF_INT_ENA;
	return SLC_INT_ENA;
}

// ROM:4000603C
uint32 slc_resume_from_host(void)
{
	slc_ptr.ena_host = 1;
	SLC_INT_ENA |= SLC_TX_EOF_INT_ENA;
	return SLC_INT_ENA;
}

// ROM:40006068
uint32 slc_set_host_io_max_window(void)
{
	SLC_BRIDGE_CONF = (SLC_BRIDGE_CONF & 0xFFFFF7C0) | 0x720;
	return SLC_BRIDGE_CONF;
}

// ROM:4000608C
void  slc_init_credit()
{
	SLC_TOKEN1 = SLC_TOKEN1_LOCAL_WR;
	slc_add_credits(8);
}

// ROM:400060AC
void slc_add_credits(uint32 x)
{
	SLC_TOKEN1 = ((x & SLC_TOKEN1_LOCAL_WDATA) << SLC_TOKEN1_LOCAL_WDATA_S) | SLC_TOKEN1_LOCAL_INC_MORE;
}
