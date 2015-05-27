 /******************************************************************************
 * FileName: sys_const.c
 * Description: read/write sdk_sys_cont (esp_init_data_default.bin)
 * Author: PV`
 * ver1.1 02/04/2015  SDK 1.0.1
 *******************************************************************************/
#ifndef _INCLUDE_SYS_CONST_H_
#define _INCLUDE_SYS_CONST_H_

#define MAX_IDX_SYS_CONST 128
#define MAX_IDX_USER_CONST 4
#define SIZE_USER_CONST (MAX_IDX_SYS_CONST + MAX_IDX_USER_CONST*4)

#define esp_init_data_default_addr (flashchip->chip_size - 4 * SPI_FLASH_SEC_SIZE)

uint8 read_sys_const(uint8 idx);
bool write_sys_const(uint8 idx, uint8 data);
uint32 read_user_const(uint8 idx);
bool write_user_const(uint8 idx, uint32 data);

#define	sys_const_rx_param25  26
#define	sys_const_rx_param26  27
#define	sys_const_rx_param27  28
#define	sys_const_rx_param28  29
#define	sys_const_tx_param5   34
#define	sys_const_tx_param6   35
#define	sys_const_tx_param7   36
#define	sys_const_tx_param8   37
#define	sys_const_tx_param9   38
#define	sys_const_tx_param10  39
#define	sys_const_tx_param11  40
#define	sys_const_tx_param12  41
#define	sys_const_tx_param13  42
#define	sys_const_tx_param14  43
#define	sys_const_tx_param15  44
#define	sys_const_tx_param16  45
#define	sys_const_tx_param17  46
#define	sys_const_tx_param18  47
#define	sys_const_soc_param0  48
#define	sys_const_soc_param2  50
#define	sys_const_soc_param3  51
#define	sys_const_soc_param4  52
#define	sys_const_soc_param5  53
#define	sys_const_soc_param7  55
#define	sys_const_rx_param29  64
#define	sys_const_rx_param30  65
#define	sys_const_rx_param31  66
#define	sys_const_rx_param32  67
#define	sys_const_rx_param33  68
#define	sys_const_rx_param34  69
#define	sys_const_rx_param35  70
#define	sys_const_rx_param36  71
#define	sys_const_rx_param37  72
#define	sys_const_rx_param38  73
#define	sys_const_tx_param24  93
#define	sys_const_tx_param25  94
#define	sys_const_tx_param26  95
#define	sys_const_tx_param27  96
#define	sys_const_tx_param28  97
#define	sys_const_tx_param29  98
#define	sys_const_tx_param37  107
#define	sys_const_tx_param42  112
#define	sys_const_tx_param43  113

#endif /* _INCLUDE_SYS_CONST_H_ */
