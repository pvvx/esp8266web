//===============================================================================
//===============================================================================
#include "user_config.h"
#include "os_type.h"
#include "bios.h"
#include "i2c_drv.h"
#include "ina219.h"
#include "sdk/rom2ram.h"
#include "sdk/add_func.h"
#include "ovl_sys.h"
#include <math.h>

#define KSHUNT	8192 // default 8192

#define ina_pin_sda	mdb_buf.ubuf[60]
#define ina_pin_scl	mdb_buf.ubuf[61]
#define ina_count	mdb_buf.ubuf[62]
#define ina_errflg	mdb_buf.ubuf[63]
#define ina_voltage	mdb_buf.ubuf[64] // Voltage LSB = 1mV per bit (step 4 mV)
#define ina_current	mdb_buf.ubuf[65] // Current LSB = 50uA per bit
#define ina_power	mdb_buf.ubuf[66] // Power LSB = 1mW per bit
#define ina_shunt	mdb_buf.ubuf[67] // 10uV

uint32 ina_init_flg DATA_IRAM_ATTR;
os_timer_t test_timer DATA_IRAM_ATTR;

#define ina_wr(a,b) i2c_wrword((INA219_ADDRESS<<24)|((a)<<16)|(b))
#define ina_rd(a,b) if(i2c_setaddr((INA219_ADDRESS<<8)|(a))) b = i2c_rdword(INA219_ADDRESS | INA219_READ)


unsigned int inafuncs DATA_IRAM_ATTR; // номер функции
unsigned int timeout DATA_IRAM_ATTR;

//-------------------------------------------------------------------------------
// writes a word
bool i2c_wrword(uint32 w)
{
	i2c_start();
	int x = 3;
	do {
		int i = 7;
	    do {
	    	i2c_step_scl_sda(w & 0x80000000);
	    	w <<= 1;
	    } while(i--);
	    if(i2c_step_scl_sda(1) != 0) { //check ack
	    	i2c_stop();
	    	return false;
	    }
	} while(x--);
	i2c_stop();
	return true;
}

//-------------------------------------------------------------------------------
// set addr
bool i2c_setaddr(uint32 addr)
{
	i2c_start();
	int x = 1;
	do {
		int i = 7;
	    do {
	    	i2c_step_scl_sda(addr & 0x8000);
	    	addr <<= 1;
	    } while(i--);
	    if(i2c_step_scl_sda(1) != 0) { //check ack
	    	i2c_stop();
	    	return false;
	    }
	} while(x--);
	i2c_stop();
	return true;
}
//-------------------------------------------------------------------------------
// read a word
uint32 i2c_rdword(uint32 addr)
{
	int i = 7;
	i2c_start();
	do {
    	i2c_step_scl_sda(addr & 0x80);
    	addr <<= 1;
    } while(i--);
    i2c_step_scl_sda(0);
    uint32 w = 0;
    i = 15;
    do {
    	w <<= 1;
    	w |= i2c_step_scl_sda(1);
    	if(i == 8) i2c_step_scl_sda(0);
    } while(i--);
    i2c_step_scl_sda(1);
	i2c_stop();
	return w;
}

//----------------------------------------------------------------------------------
void IntReadIna219(void)
//----------------------------------------------------------------------------------
{
	ina_count++;
	switch(ina_count&3){
	case 0:
		ina_voltage = i2c_rdword(INA219_ADDRESS | INA219_READ) >> 1;
		i2c_setaddr((INA219_ADDRESS<<8)|INA219_REG_POWER);
		break;
	case 1:
		ina_power = i2c_rdword(INA219_ADDRESS | INA219_READ);
		i2c_setaddr((INA219_ADDRESS<<8)|INA219_REG_CURRENT);
		break;
	case 2:
		ina_current = i2c_rdword(INA219_ADDRESS | INA219_READ);
		i2c_setaddr((INA219_ADDRESS<<8)|INA219_REG_SHUNTVOLTAGE);
		break;
	default:
		ina_shunt = i2c_rdword(INA219_ADDRESS | INA219_READ);
		i2c_setaddr((INA219_ADDRESS<<8)|INA219_REG_BUSVOLTAGE);
		break;
	}
}
//----------------------------------------------------------------------------------
// Initialize Sensor driver
int OpenINA219drv(void)
//----------------------------------------------------------------------------------
{
		if(ina_pin_scl > 15 || ina_pin_sda > 15 || ina_pin_scl == ina_pin_sda) { // return 1;
			ina_pin_scl = 4;
			ina_pin_sda = 5;
		}
		if(i2c_init(ina_pin_scl, ina_pin_sda, 54)) {	// (4,5,54); // 354
			ina_errflg = -3; // драйвер не инициализирован - ошибки параметров инициализации
			return 1;
		}
		i2c_stop();
		ets_timer_disarm(&test_timer);
		// 16V@40mV (G=1) -> 8192
		if( ina_wr(INA219_REG_CALIBRATION, KSHUNT) && ina_wr(INA219_REG_CONFIG,
					INA219_CONFIG_BVOLTAGERANGE_32V |
					INA219_CONFIG_GAIN_8_320MV |
					INA219_CONFIG_BADCRES_12BIT |
					INA219_CONFIG_SADCRES_12BIT_128S_69MS |
					INA219_CONFIG_MODE_SANDBVOLT_CONTINUOUS)) {
			i2c_setaddr((INA219_ADDRESS<<8)|INA219_REG_BUSVOLTAGE);
			ina_count = -1;
			ets_timer_setfn(&test_timer, (os_timer_func_t *)IntReadIna219, NULL);
			ets_timer_arm_new(&test_timer, 10, 1, 1); // 100 раз в сек
	        ina_errflg = 0; // драйвер запущен
		    ina_init_flg = 1;
			return 0;
		}
        ina_errflg = -2; // неисправность
		return -2;
}
//----------------------------------------------------------------------------------
// Close Sensor driver
int CloseINA219drv(void)
//----------------------------------------------------------------------------------
{
	ets_timer_disarm(&test_timer);
	int ret = i2c_deinit();
    ina_errflg = -1; // драйвер не инициализирован
    ina_init_flg = 0;
    return ret;
}

//=============================================================================
//=============================================================================
int ovl_init(int flg)
{
	int x = 0;
	switch(flg) {
	case 1:
		if(ina_init_flg) CloseINA219drv();
		return OpenINA219drv();
	case 2:
		ets_timer_disarm(&test_timer);
		return 0;
	case 3:
	{
		ina_rd(INA219_REG_BUSVOLTAGE,x);
		return x;
	}
	case 4:
	{
		ina_rd(INA219_REG_SHUNTVOLTAGE,x);
		return x;
	}
	default:
		switch(flg>>16) {
		case 1:
			ina_wr(INA219_REG_CALIBRATION, flg & 0xFFFF);
			return 0;
		case 2:
			ina_wr(INA219_REG_CONFIG, flg & 0xFFFF);
			return 0;
		default:
			return CloseINA219drv();
		}
	}
}

