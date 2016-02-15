#ifndef _ModBus_H_
//===============================================================================
// ModBus.h  11.2010 pvvx
//===============================================================================
#define _ModBus_H_

/*************************************
!!! Старший байт передается первым !!!
Чтение данных:
Адрес и количество данных задаются 16-битными числами, старший байт каждого из них передается первым.
Запрос состоит из адреса первого элемента таблицы, значение которого требуется прочитать,
и количества считываемых элементов. Адрес и количество данных задаются 16-битными числами,
старший байт каждого из них передается первым.
В ответе передаются запрошенные данные. Количество байт данных зависит от количества запрошенных элементов.
Перед данными передается один байт, значение которого равно количеству байт данных.
http://ru.wikipedia.org/wiki/Modbus
*************************************/
// Список кодов ошибок ModBus
#define MDBERRNO   0 // OK
#define MDBERRFUNC 1 // ILLEGAL FUNCTION Принятый код функции не может быть обработан на подчиненном.
#define MDBERRADDR 2 // ILLEGAL DATA ADDRESS Адрес данных указанный в запросе не доступен данному подчиненному.
#define MDBERRDATA 3 // ILLEGAL DATA VALUE Величина содержащаяся в поле данных запроса является не допустимой величиной для подчиненного.
#define MDBERRDEDF 4 // SLAVE DEVICE FAILURE Невосстанавливаемая ошибка имела место пока подчиненный пытался выполнить затребованное действие.
#define MDBERRACK  5 // ACKNOWLEDGE Подчиненный принял запрос и обрабатывает его, но это требует много времени.
// Этот ответ предохраняет главного от генерации ошибки таймаута.
// Главный может выдать команду Poll Program Complete для обнаружения завершения обработки команды.
#define MDBERRBUSY 6 // SLAVE DEVICE BUSY Подчиненный занят обработкой команды.
// Главный должен повторить сообщение позже, когда подчиненный освободится.
#define MDBERRNACK 7 // NEGATIVE ACKNOWLEDGE Подчиненный не может выполнить программную функцию, принятую в запросе.
// Этот код возвращается для неудачного программного запроса, использующего функции с номерами 13 или 14.
// Главный должен запросить диагностическую информацию или информацию обошибках с подчиненного.
#define MDBERRMPAR 8 // MEMORY PARITY ERROR Подчиненный пытается читать расширенную память, но обнаружил ошибку паритета.
// Главный может повторить запрос, но обычно в таких случаях требуется ремонт.
#define MDBERRPATH 10 // 10 GATEWAY PATH UNAVAILABLE
#define MDBERRRESP 11 // 11 GATEWAY TARGET DEVICE FAILED TO RESPOND

#define MDBERRCRC 33 // Ответ устройтва есть, но CRC

// Максимальный размер ADU для последовательных сетей
// RS232/RS485 - 256 байт, для сетей TCP - 260 байт.
// RS232 / RS485 ADU = 253 bytes + Server address (1 byte) + CRC (2 bytes) = 256 bytes.
// TCP MODBUS ADU = 253 bytes + MBAP (7 bytes) = 260 bytes.
#define MDB_TCP_ADU_SIZE_MAX 253 // Максимальный размер пакета RTU в TCP
#define MDB_ADU_F3F4_DATA_MAX 125 // Максимальное кол-во переменных в запросе/ответе в команде 3 или 4 для RS-485 и TCP
#define MDB_ADU_F6_DATA_MAX 1 // Максимальное кол-во переменных в запросе/ответе в команде 6 для RS-485 и TCP
#define MDB_ADU_F16_DATA_MAX 123 // Максимальное кол-во переменных в запросе/ответе в команде 16 для RS-485 и TCP

#define MDB_TCP_PID 0 // Protocol Identifier

typedef union __attribute__ ((packed)) // Application Data Unit (ADU) of Serial line
{
	uint16 ui[128];
	uint8 uc[256];
	uint8 id; /* Адрес подчинённого устройства, к которому адресован запрос.
	 RS-485: Ведомые устройства отвечают только на запросы, поступившие в их адрес.
	 Ответ также начинается с адреса отвечающего ведомого устройства, который может
	 изменяться от 1 до 247. Адрес 0 используется для широковещательной передачи,
	 его распознаёт каждое устройство, адреса в диапазоне 248...255 - зарезервированы;
	 TCP/IP: обычно игнорируется, если соединение установлено с конкретным устройством.
	 Может использоваться, если соединение установлено с бриджом, который выводит нас,
	 например, в сеть RS485.*/
	struct __attribute__ ((packed)) // (RX/TX) Общая форма / User functions  simple Protocol Data Unit (PDU)
	{
		struct	__attribute__ ((packed))
		{
			uint8 id;
			uint8 fun; // Функция
		} hd;
		uint16 data[126]; //(256-2-2)/2=126
//		uint16 crc;  // Контрольная сумма (!) для TCP/IP не используется
	} fx; // (RX/TX) User functions  simple Protocol Data Unit (PDU)
	struct __attribute__ ((packed)) // (RX) Read Holding Registers 03 / (RX) Read Input Register 04
	{
		struct	__attribute__ ((packed))
		{
			uint8 id;
			uint8 fun; // Функция
			uint16 addr; // Адрес регистра
			uint16 len; // кол-во
		} hd;
	} f3f4;
	struct __attribute__ ((packed)) // (RX) Write Single Register 06 /  (TX) Response data
	{
		struct	__attribute__ ((packed))
		{
			uint8 id;
			uint8 fun; // Функция
			uint16 addr; // Адрес регистра
		} hd;
		uint16 data; // Данные
	} f6; // (RX) Write Single Register 06 /  (TX) Response data
	struct	__attribute__ ((packed)) // (RX) Write Multiple Registers 16
	{
		struct	__attribute__ ((packed))
		{
			uint8 id;
			uint8 fun; // Функция
			uint16 addr; // Адрес регистра
			uint16 len; // кол-во
			uint8 cnt; // Byte Count
		} hd;
		uint16 data[MDB_ADU_F16_DATA_MAX]; //(256-2-7)/2=123.5
	} f16; // (RX) Write Multiple Registers 16
	struct __attribute__ ((packed)) // (RX) Read/Write Multiple Registers 23
	{
		struct	__attribute__ ((packed))
		{
			uint8 id;
			uint8 fun; // Функция
			uint16 raddr; // Адрес регистра
			uint16 rlen; // кол-во
			uint16 waddr; // Адрес регистра
			uint16 wlen; // кол-во
			uint8 cnt; // Byte Count
		} hd;
		uint16 data[121]; //(256-2-11)/2=121.5
	} f23; // (RX) Read/Write Multiple Registers 23
/*   struct __attribute__ ((packed)) // (RX) Diagnostics (Serial Line only) 08
	 {
		struct	__attribute__ ((packed))
		{
			uint8 fun; // Функция
			uint16 subf; // Субфункция
		} hd;
		uint16 data;
	 }f08; // (RX) Diagnostics (Serial Line only) 08 */
	struct __attribute__ ((packed)) // (TX) Response 01,02,03,04
	{
		struct	__attribute__ ((packed))
		{
			uint8 id;
			uint8 fun; // Функция
			uint8 cnt; // Byte Count
		} hd;
		uint16 data[MDB_ADU_F3F4_DATA_MAX];  //(256-2-3)/2=125.5
	} o3o4; // (TX) Response 01,02,03,04
	struct __attribute__ ((packed)) // (TX) Response 05,06,15,16
	{
		struct	__attribute__ ((packed))
		{
			uint8 id;
			uint8 fun; // Функция
			uint16 addr; // Адрес регистра
			uint16 len; // кол-во
		} hd;
	} o6o16; // (TX) Response 05,06,15,16
	struct __attribute__ ((packed)) // (TX) Response error
	{
			uint8 id;
			uint8 err; // Error code (fun | 0x80)
			uint8 exc; // Exception code (01 or 02 or 03 or 04)
	} err; // (TX) Response error
}smdbadu;

typedef struct __attribute__ ((packed)) // MBAP header (MODBUS Application Protocol header)
{
   uint16 tid; // Transaction Identifier
   uint16 pid; // Protocol Identifier
   uint16 len; // Length  (длина следующей за этим полем части пакета)
}smdbmbap;

typedef struct __attribute__ ((packed))
{
	smdbmbap mbap;
	smdbadu	  adu;	
}smdbtcp;


typedef struct  // Таблица соответствия и функций обрабоки переменных ModBus
{
   uint16 addrs;  // Стартовый адрес блока переменных
   uint16 addre;  // Последний адрес блока переменных (до 0xFFFF)
   uint8 * buf; // Указатель на буфер данных для пересылки.
                // Если указатель = NULL, то получаем в ответе все нули.
   uint32 (* const func)(uint8 * mdb, uint8 * buf, uint32 wr_len);
   // Если NULL -> функция не вызывается и данные при чтении передаются в mdb равные 0x0000 или из буфера приема.
}smdbtabaddr;

// Стандартные функции для smdbtabaddr
uint32 MdbWordRW(uint8 * mdb, uint8 * buf, uint32 rwflg); // WORD Read/Write
uint32 MdbWordR(uint8 * mdb, uint8 * buf, uint32 rwflg); // WORD Read Only
//
uint32 MdbFunc(smdbadu * mdbbuf, uint32 len); // Обработка сообщения ModBus (без CRC)
uint32 SetMdbErr(smdbadu * mdbbuf, uint32 err);

#endif //_ModBus_H_
