/*
 * hspi_master.h
 *
 *  Created on: 25 янв. 2016 г.
 *      Author: PVV
 */

#ifndef _HSPI_MASTER_H_
#define _HSPI_MASTER_H_

#define SPI_CMD_READ 0x80

//void hspi_cmd_addr_write(uint32 cs_pin, uint32 cmd, uint32 addr, uint8 * data, uint32 len);
//void hspi_cmd_addr_read(uint32 cs_pin, uint32 cmd, uint32 addr, uint8 * data, uint32 len);

void hspi_cmd_write(uint32 cs_pin, uint32 cmd, uint8 * data, uint32 len);
void hspi_cmd_read(uint32 cs_pin, uint32 cmd, uint8 * data, uint32 len);

//void hspi_byte_write(uint8 *data, uint32 len);
//void hspi_byte_read(uint8 *data, uint32 len);

//void hspi_cs_enable(uint32 pin_num);
//void hspi_cs_disable(uint32 pin_num);

uint32 hspi_master_init(uint32 flg, uint32 clock);

#endif /* _HSPI_MASTER_H_ */
