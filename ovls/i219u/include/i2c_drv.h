/*
 * i2c_drv.h
 *
 *   Created on: 05.03.2016
 *      Author: PVV
 */

#ifndef _I2C_DRV_H_
#define _I2C_DRV_H_

int i2c_init(uint32 scl, uint32 sda, uint32 time); // 400кГц CPU 160MHz time=92
int i2c_deinit(void);

uint32 i2c_test_sda(void);
void i2c_start(void);
void i2c_stop(void);
uint32 i2c_wrd8ba(uint32 data);

void i2c_set_sda(uint32 x);
void i2c_set_scl(uint32 x);
uint32 i2c_step_scl_sda(uint32 x);

#endif /* _I2C_DRV_H_ */
