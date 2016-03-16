#ifndef _FOST02_H_
#define _FOST02_H_

#define HDT14BIT 1   // =1 12/14bit, =0 8/12bit
#define PTATD1 39.9  // = 39.6 if Vdd = 3.3V, = 39.9 if Vdd = 5V

#define HTD_MT  0b00000011 //Read Measure Temperature
#define HTD_MH  0b00000101 //Read Measure Humidity
#define HTD_RS  0b00000111 //Read Status Register
#define HTD_WS  0b00000110 //Write Status Register
#define HTD_SR  0b00011110 //Soft reset

//extern adc4value T,RH,Dp;

int OpenHMSdrv(void);
int CloseHMSdrv(void);
void s_connectionreset(void);

void ReadHMS(void);

#endif //_FOST02_H_
