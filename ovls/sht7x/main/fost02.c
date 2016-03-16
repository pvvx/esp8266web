//===============================================================================
// Интерфейс для работы с датчиками влажности по I2C
// типа: FOST02, SHT1x, SHT7x
//===============================================================================
#include "user_config.h"
#include "os_type.h"
#include "bios.h"
#include "i2c_drv.h"
#include "fost02.h"
#include "sdk/rom2ram.h"
#include "sdk/add_func.h"
#include "ovl_sys.h"
#include <math.h>

#define hms_pin_sda	mdb_buf.ubuf[60]
#define hms_pin_scl	mdb_buf.ubuf[61]
#define hms_count	mdb_buf.ubuf[62]
#define hms_errflg	mdb_buf.ubuf[63]
#define hms_tmp		mdb_buf.ubuf[64]
#define hms_rh		mdb_buf.ubuf[65]

uint32 hms_init_flg DATA_IRAM_ATTR;

#define SetTimeOut(a) timeout = (a/10)+1
#define TestTimeOut() (timeout == 0)

os_timer_t test_timer DATA_IRAM_ATTR;

typedef union
{
  float d;
  unsigned long ul;
  signed long l;
}u4byte;

u4byte T,RH DATA_IRAM_ATTR;
unsigned int hmioerr, hmerrcnt, hmfuncs DATA_IRAM_ATTR; // флаг ошибки, счетчик ошибок, номер функции
unsigned int timeout DATA_IRAM_ATTR;

unsigned int crc8, reg_tmp, reg_rh DATA_IRAM_ATTR;
//-------------------------------------------------------------------------------
const unsigned char CRC8Table[256] = {
0, 49, 98, 83, 196, 245, 166, 151, 185, 136, 219, 234, 125, 76, 31, 46,
67, 114, 33, 16, 135, 182, 229, 212, 250, 203, 152, 169, 62, 15, 92, 109,
134, 183, 228, 213, 66, 115, 32, 17, 63, 14, 93, 108, 251, 202, 153, 168,
197, 244, 167, 150, 1, 48, 99, 82, 124, 77, 30, 47, 184, 137, 218, 235,
61, 12, 95, 110, 249, 200, 155, 170, 132, 181, 230, 215, 64, 113, 34, 19,
126, 79, 28, 45, 186, 139, 216, 233, 199, 246, 165, 148, 3, 50, 97, 80,
187, 138, 217, 232, 127, 78, 29, 44, 2, 51, 96, 81, 198, 247, 164, 149,
248, 201, 154, 171, 60, 13, 94, 111, 65, 112, 35, 18, 133, 180, 231, 214,
122, 75, 24, 41, 190, 143, 220, 237, 195, 242, 161, 144, 7, 54, 101, 84,
57, 8, 91, 106, 253, 204, 159, 174, 128, 177, 226, 211, 68, 117, 38, 23,
252, 205, 158, 175, 56, 9, 90, 107, 69, 116, 39, 22, 129, 176, 227, 210,
191, 142, 221, 236, 123, 74, 25, 40, 6, 55, 100, 85, 194, 243, 160, 145,
71, 118, 37, 20, 131, 178, 225, 208, 254, 207, 156, 173, 58, 11, 88, 105,
4, 53, 102, 87, 192, 241, 162, 147, 189, 140, 223, 238, 121, 72, 27, 42,
193, 240, 163, 146, 5, 52, 103, 86, 120, 73, 26, 43, 188, 141, 222, 239,
130, 179, 224, 209, 70, 119, 36, 21, 59, 10, 89, 104, 255, 206, 157, 172};
/*void CalkCRC8(unsigned char x)
{
  crc8 = CRC8Table[x ^ crc8];
}*/
//----------------------------------------------------------------------------------
// writes a byte on the Sensibus and checks the acknowledge
// hrioerr++ in case of no acknowledge
void s_write_byte(unsigned int val)
{
    crc8 = CRC8Table[val];
	int i = 7;
    do {
    	i2c_step_scl_sda(val & 0x80);
    	val <<= 1;
    } while(i--);
    if(i2c_step_scl_sda(1) != 0) hmioerr++; //check ack (I2CDAT will be pulled down by SHT11)
}
//----------------------------------------------------------------------------------
unsigned int s_read_byte(void) //  ack On
//----------------------------------------------------------------------------------
// reads a byte form the Sensibus and gives an acknowledge in case of "ack=1"
{
	uint32 x = 0;
	int i = 7;
    do {
    	x <<= 1;
    	x |= i2c_step_scl_sda(1);
    } while(i--);
    i2c_step_scl_sda(0);
    crc8 = CRC8Table[x^crc8];
	return x;
}
//-------------------------------------------------------------------------------
unsigned char s_read_crc(void) // ack Off
//----------------------------------------------------------------------------------
// reads a byte form the Sensibus and gives an acknowledge in case of "ack=1"
{
	uint32 x = 0;
	int i = 7;
    do {
    	x >>= 1;
    	x |= i2c_step_scl_sda(1) << 7;
    } while(i--);
    if(i2c_step_scl_sda(1) == 0) hmioerr++;
//    ets_printf("0x%02x#0x%02x ", x, crc8);
	return (x^crc8);
}
void s_transstart(void)
//----------------------------------------------------------------------------------
// generates a transmission start
//           _____         ________
// I2CDAT:        |_______|
//               ___     ___
// I2C_SCL : ___|   |___|   |______
{
	i2c_set_sda(1);
	i2c_set_scl(1);
	i2c_set_sda(0);
	i2c_set_scl(0);

	i2c_set_scl(0);
	i2c_set_scl(1);
	i2c_set_sda(1);
	i2c_set_scl(0);
	i2c_set_scl(0);
    if(i2c_test_sda() == 0) hmioerr++;
}
//----------------------------------------------------------------------------------
void s_connectionreset(void)
//----------------------------------------------------------------------------------
// communication reset: I2CDAT-line=1 and at least 9 I2C_SCL cycles followed by transstart
//           _____________________________________________________         ________
// I2CDAT:                                                        |_______|
//              _    _    _    _    _    _    _    _    _        ___     ___
// I2C_SCL : __| |__| |__| |__| |__| |__| |__| |__| |__| |______|   |___|   |______
{
	int i = 8;
    do {
    	i2c_step_scl_sda(1);
    } while(i--);
	s_transstart();
}
//----------------------------------------------------------------------------------
void ReadHMS(void)
//----------------------------------------------------------------------------------
{
	if(timeout) timeout--;
        if(hmioerr)
        {
          if(++hmerrcnt>3) // больше трех ошибок подряд?
          {
            hmerrcnt=0; // сбросить счетчик ошибок
            hms_errflg = -2; // неисправность сенсора влажности
//            ets_printf("*");
          };
          hmioerr = 0; // сбросить флаг ошибки
          hmfuncs = 0;  // далее сбросить датчик
          SetTimeOut(50); // зададим таймаут в 50 ms
        };
        switch(hmfuncs)
        {
          default:
           if(!TestTimeOut()) break; // ожидание паузы
           s_connectionreset(); // 26t
//           s_transstart();      // 8t transmission start
           if(hmioerr) break;
           s_write_byte(HTD_WS);// 18t Status Register Write  [0x06]
           if(hmioerr) break;
           s_write_byte(0x00);  // 18t
           if(hmioerr) break;
           hmioerr=0; // сбросить флаг ошибки (см. InitHMS())
           hmfuncs=1; // далее на запрос температуры датчика
           break; // 26+8+16+18 = 68t -> 68*1.25=85 us

          case 2:  // чтение данных температуры с датчика и запрос данных о влажности
          case 5:  // чтение данных температуры с датчика и запрос данных о влажности
           if(i2c_test_sda())
           {
             if(TestTimeOut()) hmioerr++;
             break;
           }
           reg_tmp = s_read_byte() << 8; // 19t
           reg_tmp |= s_read_byte(); // 19t
           if (s_read_crc()) // 19t
           {
             hmioerr++;
             break;
           };
           s_transstart(); // 8t transmission start
           s_write_byte(HTD_MH); // 18t Read Measure Humidity [0x05] ... 0.06133 s
           hmfuncs++; // след. функция
           SetTimeOut(120); // зададим таймаут в 120 ms
           break; // 19*3+8+18=83t  83*1.25=103.75us=0.00010375 sec

          case 3: // чтение данных о влажности
          case 6: // чтение данных о влажности
           if(i2c_test_sda())
           {
            if(TestTimeOut()) hmioerr++;
            break;
           }
           reg_rh = s_read_byte()<<8; // 19t
           reg_rh |= s_read_byte();
           if (s_read_crc()) // 19t  // ~75us
           {
             hmioerr++;
             break;
           };
          case 1: // запрос температуры датчика       (цикл опроса 0.2744s)
           s_transstart(); // 8t transmission start
           s_write_byte(HTD_MT); // 18t Read Measure Temperature  [0x03] ... 0.2128 s
           hmfuncs++; // далее на чтение данных температуры с датчика
           SetTimeOut(350); // зададим таймаут в 350 ms
           break;

          case 4: // сумма, проход 1
           T.ul = reg_tmp;
           RH.ul = reg_rh;
           hmfuncs++;
           break;

          case 7: // сумма, проход 2
           T.ul += reg_tmp;
//           mouts.rt = T.ui[0];
           RH.ul += reg_rh;
//           mouts.rh = RH.ui[0];
           hmfuncs++;
           break;

          case 8: // расчет, часть 1
#if HDT14BIT
           T.d=((float)(T.ul))*0.005 - PTATD1; //calc. Temperature from ticks to [C]      36.92 0.3
#else
           T.d=((float)(T.ul))*0.002 - PTATD1; //calc. Temperature from ticks to [C]
#endif
           RH.d=(float)(RH.ul)*0.5;
#if HDT14BIT
           RH.d=(T.d-25.0)*(0.01+0.00008*RH.d)-0.0000028*RH.d*RH.d+0.0405*RH.d-4.0;
#else
           RH.d=(T.d-25.0)*(0.01+0.00128*RH.d)-0.00072*RH.d*RH.d+0.648*RH.d-4.0;
#endif
           if(RH.d>100.0) RH.d=100.0;
           else if(RH.d<0.1) RH.d=0.1;
/*           hmfuncs++;
           break;
          case 9: // расчет, часть 2
           Dp.d = (log10(RH.d)-2)/0.4343 + (17.62*T.d)/(243.12+T.d);
           Dp.d = 243.12*Dp.d/(17.62-Dp.d);


           ets_printf("T=%d, RH=%d, DP=%d\n", (int)(T.d*100.0), (int)(RH.d*100.0), (int)(Dp.d*100.0) );
*/
           // перевод float в int.01
           hms_tmp = (int)(T.d*100.0);
           hms_rh = (int)(RH.d*100.0);
           hms_count++;
       	   hms_errflg = 0; // сбросить  неисправность сенсора влажности
//           ets_printf("T=%d, RH=%d\n", hms_tmp, hms_rh);
           hmerrcnt=0; // сбросить счетчик ошибок
           hmfuncs=2; // на начало опроса
           break;
        }
}
//----------------------------------------------------------------------------------
// Initialize Humidity Sensor driver
int OpenHMSdrv(void)
//----------------------------------------------------------------------------------
{
		if(hms_pin_scl > 15 || hms_pin_sda > 15 || hms_pin_scl == hms_pin_sda) { // return 1;
			hms_pin_scl = 4;
			hms_pin_sda = 5;
		}
		if(i2c_init(hms_pin_scl, hms_pin_sda, 54)) {	// 4,5,54); // 354
			hms_errflg = -3; // драйвер не инициализирован - ошибки параметров инициализации
			return 1;
		}
		hmerrcnt = 0;
        hmioerr = 0;
        hmfuncs = 0;
        hms_errflg = 1; // драйвер запущен
        SetTimeOut(50); // зададим таймаут в 50 ms
		ets_timer_disarm(&test_timer);
		ets_timer_setfn(&test_timer, (os_timer_func_t *)ReadHMS, NULL);
		ets_timer_arm_new(&test_timer, 10, 1, 1); // 100 раз в сек
	    hms_init_flg = 1;
		return 0;
}
//----------------------------------------------------------------------------------
// Close Humidity Sensor driver
int CloseHMSdrv(void)
//----------------------------------------------------------------------------------
{
	ets_timer_disarm(&test_timer);
	int ret = i2c_deinit();
    hms_errflg = -1; // драйвер не инициализирован
    hms_init_flg = 0;
    return ret;
}

//=============================================================================
//=============================================================================
int ovl_init(int flg)
{
	if(flg == 1) {
		if(hms_init_flg) CloseHMSdrv();
		return OpenHMSdrv();
	}
	else {
		return CloseHMSdrv();
	}
}

