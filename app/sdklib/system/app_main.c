/******************************************************************************
 * FileName: app_main.c
 * Description: Alternate SDK (libmain.a)
 * Author: PV`
 * (c) PV` 2015
*******************************************************************************/
#include "sdk/sdk_config.h"
#include "bios.h"
#include "user_interface.h"
#include "hw/esp8266.h"
#include "hw/spi_register.h"
#include "hw/uart_register.h"
#include "hw/corebits.h"
#include "hw/specreg.h"
#include "sdk/fatal_errs.h"
#include "sdk/app_main.h"
#include "sdk/add_func.h"
#include "sdk/flash.h"
#include "sdk/wdt.h"
#include "lwip/init.h"
#include "lwip/netif.h"
#include "lwip/timers.h"
#include "phy/phy.h"
#include "sdk/sys_const.h"
#include "sdk/rom2ram.h"
//=============================================================================
// Define
//-----------------------------------------------------------------------------
#if defined(USE_MAX_IRAM) 
 #if USE_MAX_IRAM == 48
	#define Cache_Read_Enable_def() Cache_Read_Enable(0, 0, 0)
 #else
	#define Cache_Read_Enable_def() Cache_Read_Enable(0, 0, 1)
 #endif
#else
	#define Cache_Read_Enable_def() Cache_Read_Enable(0, 0, 1)
#endif
//=============================================================================
// Data
//-----------------------------------------------------------------------------
struct s_info info; // ip,mask,gw,mac AP, ST
ETSTimer check_timeouts_timer DATA_IRAM_ATTR; // timer_lwip
uint8 user_init_flag;

#if DEF_SDK_VERSION >= 1400
extern int chip_v6_set_chan_offset(int, int);
extern uint8 phy_rx_gain_dc_flag;
extern uint8 * phy_rx_gain_dc_table;
extern sint16 TestStaFreqCalValInput;
extern struct rst_info rst_inf;
#endif
#if DEF_SDK_VERSION >= 1200
uint8 SDK_VERSION[] = {SDK_VERSION_TXT};
uint16 lwip_timer_interval;
#endif


#ifdef USE_OPEN_LWIP
extern bool default_hostname; // in eagle_lwip_if.c
#endif
//=============================================================================
// Init data (flash)
//=============================================================================
//  esp_init_data_default.bin
//-----------------------------------------------------------------------------
#if DEF_SDK_VERSION >= 1400
const uint8 esp_init_data_default[128] ICACHE_RODATA_ATTR = {
	    5,    0,    4,    2,    5,    5,    5,    2,    5,    0,    4,    5,    5,    4,    5,    5,
	    4, 0xFE, 0xFD, 0xFF, 0xF0, 0xF0, 0xF0, 0xE0, 0xE0, 0xE0, 0xE1,  0xA, 0xFF, 0xFF, 0xF8,    0,
	 0xF8, 0xF8, 0x52, 0x4E, 0x4A, 0x44, 0x40, 0x38,    0,    0,    1,    1,    2,    3,    4,    5,
	    1,    0,    0,    0,    0,    0,    2,    0,    0,    0,    0,    0,    0,    0,    0,    0,
	 0xE1,  0xA,    0,    0,    0,    0,    0,    0,    0,    0,    1, 0x93, 0x43,    0,    0,    0,
	    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
	    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
	    3,    0,    2,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0
};
#elif DEF_SDK_VERSION >= 1200
const uint8 esp_init_data_default[128] ICACHE_RODATA_ATTR = {
	    5,    0,    4,    2,    5,    5,    5,    2,    5,    0,    4,    5,    5,    4,    5,    5,
	    4, 0xFE, 0xFD, 0xFF, 0xF0, 0xF0, 0xF0, 0xE0, 0xE0, 0xE0, 0xE1,  0xA, 0xFF, 0xFF, 0xF8,    0,
	 0xF8, 0xF8, 0x52, 0x4E, 0x4A, 0x44, 0x40, 0x38,    0,    0,    1,    1,    2,    3,    4,    5,
	    1,    0,    0,    0,    0,    0,    2,    0,    0,    0,    0,    0,    0,    0,    0,    0,
	 0xE1,  0xA,    0,    0,    0,    0,    0,    0,    0,    0,    1, 0x93, 0x43,    0,    0,    0,
	    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
	    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
	    3,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0
};
#endif
#define esp_init_data_default_size 128
//=============================================================================
// extern funcs
//-----------------------------------------------------------------------------
//=============================================================================
// IRAM code
//-----------------------------------------------------------------------------
void call_user_start(void);
#ifdef USE_FIX_QSPI_FLASH
//=============================================================================
// Инициализация QSPI и cache
//-----------------------------------------------------------------------------
static void set_qspi_flash_cache(void)
{
	SPI0_USER |= SPI_CS_SETUP; // +1 такт перед CS = 0x80000064
	// SPI на 80 MHz
#if USE_FIX_QSPI_FLASH == 80
	GPIO_MUX_CFG |= BIT(MUX_SPI0_CLK_BIT); // QSPI = 80 MHz
	SPI0_CTRL = (SPI0_CTRL & SPI_CTRL_F_MASK) | SPI_CTRL_F80MHZ;
#else  // SPI на 40 MHz
	GPIO_MUX_CFG &= ~(1<< MUX_SPI0_CLK_BIT);
	SPI0_CTRL = (SPI0_CTRL & SPI_CTRL_F_MASK) | SPI_CTRL_F40MHZ;
#endif
#ifdef USE_ALTBOOT

#endif
	flashchip->chip_size = 512*1024; // песочница для SDK в 512 килобайт flash
	// Всё - включаем кеширование, далее можно вызывать процедуры из flash
	Cache_Read_Enable_def();
}
#else
//-----------------------------------------------------------------------------
// Установка скорости QSPI
//  0 = 40MHz, 1 = 26MHz, 2 = 20MHz, >2 = 80MHz
//-----------------------------------------------------------------------------
void sflash_something(uint32 flash_speed)
{
	//	Flash QIO80:
	//  SPI_CTRL = 0x16ab000 : QIO_MODE | TWO_BYTE_STATUS_EN | WP_REG | SHARE_BUS | ENABLE_AHB | RESANDRES | FASTRD_MODE | BIT12
	//	IOMUX_BASE = 0x305
	//  Flash QIO40:
	//	SPI_CTRL = 0x16aa101
	//	IOMUX_BASE = 0x205
	uint32 xreg = (SPI0_CTRL >> 12) << 12; //  & 0xFFFFF000
	uint32 value;
	if (flash_speed > 2) { // 80 MHz
		value = BIT(12); // 0x60000208 |= 0x1000
		GPIO_MUX_CFG |= (1<< MUX_SPI0_CLK_BIT); // HWREG(IOMUX_BASE, 0) |= BIT(8);  // 80 MHz
	}
	else { // 0:40, 1:26, 2:20 MHz // 0x101, 0x202, 0x313
		value = 1 + flash_speed + ((flash_speed + 1) << 8) + ((flash_speed >> 1)<< 4);
		xreg &= ~BIT(12);  // 0x60000208 &= 0xffffefff
		GPIO_MUX_CFG &= MUX_CFG_MASK & (~(1<< MUX_SPI0_CLK_BIT)); // HWREG(IOMUX_BASE, 0) &= ~(BIT(8));  // 0x60000800 &=  0xeff
	}
	SPI0_CTRL = xreg | value;
}
//=============================================================================
// Инициализация QSPI и cache
//-----------------------------------------------------------------------------
static void set_qspi_flash_cache(void)
{
    // Загрузка заголовка flash
    struct SPIFlashHead fhead;
    Cache_Read_Disable();
	SPI0_USER |= SPI_CS_SETUP; // +1 такт перед CS
	SPIRead(0, (uint32_t *)&fhead, sizeof(fhead));
	// Установка размера Flash от 256Kbytes до 32Mbytes
	// High four bits fhead.hsz.flash_size: 0 = 512K, 1 = 256K, 2 = 1M, 3 = 2M, 4 = 4M, ... 7 = 32M
    uint32 fsize = fhead.hsz.flash_size & 7;
	if(fsize < 2) flashchip->chip_size = (8 >> fsize) << 16;
	else flashchip->chip_size = (4 << fsize) << 16;
    uint32 fspeed = fhead.hsz.spi_freg;
	// Установка:
	// SPI Flash Interface (0 = QIO, 1 = QOUT, 2 = DIO, 0x3 = DOUT)
	// and Speed QSPI: 0 = 40MHz, 1= 26MHz, 2 = 20MHz, ... = 80MHz
	sflash_something(fspeed);
	// SPIFlashCnfig(fhead.spi_interface & 3, (speed > 2)? 1 : speed + 2);
	// SPIReadModeCnfig(5); // in ROM
	// Всё - включаем кеширование, далее можно вызывать процедуры из flash
	Cache_Read_Enable_def();
}
#endif
//=============================================================================
// call_user_start() - вызывается из заголовка, загрузчиком
// Default entry point:  ENTRY(call_user_start) in eagle.app.v6.ld
//-----------------------------------------------------------------------------
void call_user_start(void)
{
		// Инициализация QSPI
		set_qspi_flash_cache();
		// Инициализация
		startup();
		// Очистка стека и передача управления в ROM-BIOS
		asm volatile (
				"movi	a2, 1;"
				"slli   a1, a2, 30;"
				);
		ets_run();
}
//=============================================================================
// ROM-BIOS запускает код с адреса 0x40100000
// при опцииях перезагрузки 'Jump Boot', если:
// GPIO0 = "0", GPIO1 = "1", GPIO2 = "0" (boot mode:(2,x))
// jump_boot() перенесено в main-vectors.c
//-----------------------------------------------------------------------------
#ifdef USE_RAPID_LOADER
typedef void (*tloader)(uint32 addr);
#else
void ICACHE_FLASH_ATTR loader(uint32 addr)
{
	__asm__ __volatile__ (
		"l32i.n	a3, a2, 0\n"	// SPIFlashHeader.head : bit0..7: = 0xE9, bit8..15: Number of segments, ...
		"l32i.n	a7, a2, 4\n"	// Entry point
		"extui	a3, a3, 8, 4\n" // Number of segments & 0x0F
		"addi.n	a2, a2, 8\n"	// p SPIFlashHeadSegment
		"j		4f\n"
	"1:\n"
		"l32i.n	a5, a2, 0\n"	// Memory offset
		"addi.n	a4, a2, 8\n"	// p start data
		"l32i.n	a2, a2, 4\n"	// Segment size
		"srli	a2, a2, 2\n"	// size >> 2
		"addx4	a2, a2, a4\n"	// + (size >> 2)
		"j		3f\n"
	"2:\n"
		"l32i.n	a6, a4, 0\n"	// flash data
		"addi.n	a4, a4, 4\n"
		"s32i.n	a6, a5, 0\n"	// Memory data
		"addi.n	a5, a5, 4\n"
	"3:\n"
		"bne	a2, a4, 2b\n"	// next SPIFlashHeadSegment != cur
		"4:\n"
		"addi.n	a3, a3, -1\n"	// Number of segments - 1
		"bnei	a3, -1, 1b\n"	// end segments ?

		"movi.n	a2, 1\n"
		"slli	a1, a2, 30\n"	// стек в 0x400000000

		"jx		a7\n" 			// callx0 a7"
	);
}
#endif
void call_jump_boot(void)
{
	ets_intr_lock();
	ets_isr_mask(0x3FE); // запретить прерывания 1..9
	IO_RTC_4 = 0; // Отключить блок WiFi (уменьшение потребления на время загрузки)
	WDT_FEED = WDT_FEED_MAGIC;
	SelectSpiFunction();
	SPIFlashCnfig(5,4);
	SPIReadModeCnfig(0);
	set_qspi_flash_cache();
	Wait_SPI_Idle(flashchip);
#ifdef DEBUG_UART
	os_printf("Jump Boot...\n");
#endif
#ifdef USE_RAPID_LOADER
	tloader loader = (tloader)USE_RAPID_LOADER;
	loader(USE_RAPID_LOADER+0x40);
#else
	loader(0x40200000); // загрузка с начала flash
#endif
}
/*
void restart(void)
{
	ets_uart_printf("Restart...");
	call_jump_boot();
}*/
//=============================================================================
// Стандартный вывод putc (UART0)
//-----------------------------------------------------------------------------
void uart0_write_char(char c)
{
	if (c != '\r') {
		do {
			MEMW();
			if(((UART0_STATUS >> UART_TXFIFO_CNT_S) & UART_TXFIFO_CNT) <= 125) break;
		} while(1);
		if (c != '\n') UART0_FIFO = c;
		else {
			UART0_FIFO = '\r';
			UART0_FIFO = '\n';
		}
	}
}
//=============================================================================
// Стандартный вывод putc (UART1)
//-----------------------------------------------------------------------------
void uart1_write_char(char c)
{
	if (c != '\r') {
		do {
			MEMW();
			if(((UART1_STATUS >> UART_TXFIFO_CNT_S) & UART_TXFIFO_CNT) <= 125) break;
		} while(1);
		if (c != '\n') UART1_FIFO = c;
		else {
		    UART1_FIFO = '\r';
		    UART1_FIFO = '\n';
		}
	}
}
#ifdef USE_READ_ALIGN_ISR
const char rd_align_txt[] ICACHE_RODATA_ATTR = "Read align4";
#define LOAD_MASK   0x00f00fu
#define L8UI_MATCH  0x000002u
#define L16UI_MATCH 0x001002u
#define L16SI_MATCH 0x009002u
// Hardware exception handling
struct exception_frame
{
  uint32_t epc;
  uint32_t ps;
  uint32_t sar;
  uint32_t unused;
  union {
    struct {
      uint32_t a0;
      // note: no a1 here!
      uint32_t a2;
      uint32_t a3;
      uint32_t a4;
      uint32_t a5;
      uint32_t a6;
      uint32_t a7;
      uint32_t a8;
      uint32_t a9;
      uint32_t a10;
      uint32_t a11;
      uint32_t a12;
      uint32_t a13;
      uint32_t a14;
      uint32_t a15;
    };
    uint32_t a_reg[15];
  };
  uint32_t cause;
};
void read_align_exception_handler(struct exception_frame *ef, uint32_t cause)
{
  /* If this is not EXCCAUSE_LOAD_STORE_ERROR you're doing it wrong! */
  (void)cause;

  uint32_t epc1 = ef->epc;
  uint32_t excvaddr;
  uint32_t insn;
  asm (
    "rsr   %0, EXCVADDR;"    /* read out the faulting address */
    "movi  a4, ~3;"          /* prepare a mask for the EPC */
    "and   a4, a4, %2;"      /* apply mask for 32bit aligned base */
    "l32i  a5, a4, 0;"       /* load part 1 */
    "l32i  a6, a4, 4;"       /* load part 2 */
    "ssa8l %2;"              /* set up shift register for src op */
    "src   %1, a6, a5;"      /* right shift to get faulting instruction */
    :"=r"(excvaddr), "=r"(insn)
    :"r"(epc1)
    :"a4", "a5", "a6"
  );

  uint32_t valmask = 0;
  uint32_t what = insn & LOAD_MASK;

  if (what == L8UI_MATCH)
    valmask = 0xffu;
  else if (what == L16UI_MATCH || what == L16SI_MATCH)
    valmask = 0xffffu;
  else
  {
die:
    /* Turns out we couldn't fix this, trigger a system break instead
     * and hang if the break doesn't get handled. This is effectively
     * what would happen if the default handler was installed. */
	fatal_error(FATAL_ERR_ALIGN, (void *)ef->epc, (void *)rd_align_txt);
//    asm ("break 1, 1");
//    while (1) {}
  }

  /* Load, shift and mask down to correct size */
  uint32_t val = (*(uint32_t *)(excvaddr & ~0x3));
  val >>= (excvaddr & 0x3) * 8;
  val &= valmask;

  /* Sign-extend for L16SI, if applicable */
  if (what == L16SI_MATCH && (val & 0x8000))
    val |= 0xffff0000;

  int regno = (insn & 0x0000f0u) >> 4;
  if (regno == 1)
    goto die;              /* we can't support loading into a1, just die */
  else if (regno != 0)
    --regno;               /* account for skipped a1 in exception_frame */

  ef->a_reg[regno] = val;  /* carry out the load */
  ef->epc += 3;            /* resume at following instruction */
}
#endif

//=============================================================================
// FLASH code (ICACHE_FLASH_ATTR)
//=============================================================================
// Чтение MAC из OTP
//-----------------------------------------------------------------------------
void ICACHE_FLASH_ATTR read_macaddr_from_otp(uint8 *mac)
{
	mac[0] = 0x18;
	mac[1] = 0xfe;
	mac[2] = 0x34;
	if ((!(OTP_CHIPID & (1 << 15)))||((OTP_MAC0 == 0) && (OTP_MAC1 == 0))) {
		mac[3] = 0x18;
		mac[4] = 0xfe;
		mac[5] = 0x34;
	}
	else {
		mac[3] = (OTP_MAC1 >> 8) & 0xff;
		mac[4] = OTP_MAC1 & 0xff;
		mac[5] = (OTP_MAC0 >> 24) & 0xff;
	}
}
//-----------------------------------------------------------------------------
// Очистка сегмента bss
//-----------------------------------------------------------------------------
/*void ICACHE_FLASH_ATTR mem_clr_bss(void)
{
	uint8 * ptr = &_bss_start;
	while(ptr < &_bss_end) *ptr++ = 0;
}*/
//-----------------------------------------------------------------------------
// Тест конфигурации для WiFi
//-----------------------------------------------------------------------------
void ICACHE_FLASH_ATTR tst_cfg_wifi(void)
{
    struct s_wifi_store * wifi_config = &g_ic.g.wifi_store;
	wifi_softap_set_default_ssid();
	wifi_station_set_default_hostname(info.st_mac);
	if(wifi_config->wfmode[0] == 0xff) wifi_config->wfmode[0] = SOFTAP_MODE;
	else wifi_config->wfmode[0] &= 3;
	wifi_config->wfmode[1] = 0;
	if(wifi_config->wfchl >= 14 || wifi_config->wfchl == 0) {
		wifi_config->wfchl = 1;
	}
	if(wifi_config->beacon >= 6000 || wifi_config->beacon < 100){ //
		wifi_config->beacon = 100;
	}
	wDev_Set_Beacon_Int((wifi_config->beacon/100)*102400);
	if(wifi_config->field_310 >= 5 || wifi_config->field_310 == 1){
		wifi_config->field_310 = 0;
		ets_bzero(wifi_config->ap_passw, 64);
	}
	if(wifi_config->field_311 > 2) wifi_config->field_311 = 0;
	if(wifi_config->field_312 > 8) wifi_config->field_312 = 4;
	if(wifi_config->st_ssid_len == 0xffffffff) {
		ets_bzero(&wifi_config->st_ssid_len, 36);
		ets_bzero(&wifi_config->st_passw, 64);
	}
	wifi_config->field_880 = 0;
	wifi_config->field_884 = 0;

//	g_ic.c[257] = 0; // ?

	if(wifi_config->field_316 >= 6) wifi_config->field_316 = 1;
	if(wifi_config->field_169 >= 2) wifi_config->field_169 = 0; // +169
	if(wifi_config->phy_mode >= 4 || wifi_config->phy_mode == 0 ) wifi_config->phy_mode = 3; // phy_mode
}
//=============================================================================
//-----------------------------------------------------------------------------
void ICACHE_FLASH_ATTR read_wifi_config(void)
{
	struct ets_store_wifi_hdr hbuf;
	spi_flash_read(flashchip->chip_size - 0x1000,(uint32 *)(&hbuf), sizeof(hbuf));
	uint32 store_cfg_addr = flashchip->chip_size - 0x3000 + ((hbuf.bank)? 0x1000 : 0);
	struct s_wifi_store * wifi_config = &g_ic.g.wifi_store;
	spi_flash_read(store_cfg_addr,(uint32 *)wifi_config, wifi_config_size);
	if (hbuf.flag != 0x55AA55AA || system_get_checksum((uint8 *)wifi_config, hbuf.xx[(hbuf.bank)? 1 : 0]) != hbuf.chk[(hbuf.bank)? 1 : 0]) {
#ifdef DEBUG_UART
		os_printf("\nError wifi_config! Clear.\n");
#endif
		ets_memset(wifi_config, 0xff, wifi_config_size);
	};
}

#ifdef DEBUG_UART
void ICACHE_FLASH_ATTR startup_uart_init(void)
{
	ets_isr_mask(1 << ETS_UART_INUM);
	UART0_INT_ENA = 0;
	UART1_INT_ENA = 0;
	UART0_INT_CLR = 0xFFFFFFFF;
	UART1_INT_CLR = 0xFFFFFFFF;
	UART0_CONF0 = 0x000001C;
	UART1_CONF0 = 0x000001C;
	UART0_CONF1 = 0x01707070;
	UART1_CONF1 = 0x01707070;
	uart_div_modify(0, UART_CLK_FREQ / DEBUG_UART0_BAUD);
	uart_div_modify(1, UART_CLK_FREQ / DEBUG_UART1_BAUD);
#if DEBUG_UART==1
	GPIO2_MUX = (1 << GPIO_MUX_FUN_BIT1);
	ets_install_putc1(uart1_write_char);
#else
	ets_install_putc1(uart0_write_char);
#endif
}
#endif
//=============================================================================
// startup()
//-----------------------------------------------------------------------------
void ICACHE_FLASH_ATTR startup(void)
{
	ets_isr_mask(0x3FE); // запретить прерывания 1..9
	ets_set_user_start(jump_boot); // установить адрес для возможной перезагрузки по доп. веткам ROM-BIOS
	// cтарт на модуле с кварцем в 26MHz, а ROM-BIOS выставил 40MHz?
	if(rom_i2c_readReg(103,4,1) != 136) { // 8: 40MHz, 136: 26MHz
		if(get_sys_const(sys_const_crystal_26m_en) == 1) { // soc_param0: 0: 40MHz, 1: 26MHz, 2: 24MHz
			// set 80MHz PLL CPU
			rom_i2c_writeReg(103,4,1,136);
			rom_i2c_writeReg(103,4,2,145);
		}
	}
#if	STARTUP_CPU_CLK == 160
	system_overclock(); // set CPU CLK 160 MHz
#endif
#ifdef DEBUG_UART
	startup_uart_init();
	os_printf("\n\nmeSDK %s\n", SDK_VERSION);
#endif
	// Очистка сегмента bss //	mem_clr_bss();
	uint8 * ptr = &_bss_start;
	while(ptr < &_bss_end) *ptr++ = 0;
//	user_init_flag = false; // итак всё равно false из-за обнуления данных в сегменте
	//
#if defined(TIMER0_USE_NMI_VECTOR)
	__asm__ __volatile__ (
			"movi	a2, 0x401\n"
			"slli	a2, a2, 20\n" // a2 = 0x40100000
			"wsr.vecbase	a2\n"
			);
#endif
	_xtos_set_exception_handler(EXCCAUSE_UNALIGNED, default_exception_handler);
	_xtos_set_exception_handler(EXCCAUSE_ILLEGAL, default_exception_handler);
	_xtos_set_exception_handler(EXCCAUSE_INSTR_ERROR, default_exception_handler);
#ifdef USE_READ_ALIGN_ISR
	_xtos_set_exception_handler(EXCCAUSE_LOAD_STORE_ERROR, read_align_exception_handler);
#else
	_xtos_set_exception_handler(EXCCAUSE_LOAD_STORE_ERROR, default_exception_handler);
#endif
	_xtos_set_exception_handler(EXCCAUSE_LOAD_PROHIBITED, default_exception_handler);
	_xtos_set_exception_handler(EXCCAUSE_STORE_PROHIBITED, default_exception_handler);
	_xtos_set_exception_handler(EXCCAUSE_PRIVILEGED, default_exception_handler);
	//
#if	DEF_SDK_VERSION < 1400
	if((RTC_RAM_BASE[0x60>>2]>>16) > 4) { // проверка опции phy_rfoption = deep_sleep_option
#ifdef DEBUG_UART
		os_printf("\nError phy_rfoption! Set default = 0.\n");
#endif
		RTC_RAM_BASE[0x60>>2] &= 0xFFFF;
		RTC_RAM_BASE[0x78>>2] = 0; // обнулить Espressif OR контрольку области 0..0x78 RTC_RAM
	}
#endif
	//
	iram_buf_init(); // определить и разметить свободную IRAM
	//
	prvHeapInit(); // инициализация менеджера памяти heap
	//
	read_wifi_config(); // чтение последних установок wifi (последние 3 сектора flash)
	//
#ifdef USE_OPEN_LWIP	
	default_hostname = true; // используется default_hostname
#endif	
	//
	// IO_RTC_4 = 0xfe000000;
	sleep_reset_analog_rtcreg_8266();
	// создать два MAC адреса для AP и SP
	read_macaddr_from_otp(info.st_mac);
	wifi_softap_cacl_mac(info.ap_mac, info.st_mac);
	// начальный IP, mask, gw для AP
	info.ap_gw = info.ap_ip = DEFAULT_SOFTAP_IP; // 0x104A8C0; ip 192.168.4.1
	info.ap_mask = DEFAULT_SOFTAP_MASK; // 0x00FFFFFF; 255.255.255.0
	ets_timer_init();
	lwip_init();
//	espconn_init(); // данный баг не используется
#if DEF_SDK_VERSION >= 1200
	lwip_timer_interval = 25; // 25 ms
	ets_timer_setfn(&check_timeouts_timer, (ETSTimerFunc *) sys_check_timeouts, NULL);
#else
	// set up a timer for lwip
	ets_timer_disarm(&check_timeouts_timer);
	ets_timer_setfn(&check_timeouts_timer, (ETSTimerFunc *) sys_check_timeouts, NULL);
	ets_timer_arm_new(&check_timeouts_timer, 25, 1, 1);
	// при wifi_set_sleep_type: NONE = 25, LIGHT = 3000 + reset_noise_timer(3000), MODEM = 25 + reset_noise_timer(100);
#endif
	//
	tst_cfg_wifi(); // Проверить переменные установок wifi
	//
#ifdef USE_DUAL_FLASH
	overlap_hspi_init(); // не используется для модулей с одной flash!
#endif
	//
#if DEF_SDK_VERSION >= 1400
	uint8 * buf = os_malloc(SIZE_SAVE_SYS_CONST);
	spi_flash_read(esp_init_data_default_addr,(uint32 *)buf, SIZE_SAVE_SYS_CONST); // esp_init_data_default.bin + ???
#if DEF_SDK_VERSION >= 1410
	if(buf[112] == 3) g_ic.c[471] = 1; // esp_init_data_default: freq_correct_en[112]
	else g_ic.c[471] = 0;
#endif
	buf[0xf8] = 0;
	phy_rx_gain_dc_table = &buf[0x100];
	phy_rx_gain_dc_flag = 0;
	// **
	// user_rf_pre_init(); // не использется, т.к. мождно вписать что угодно и тут :)
    //	system_phy_set_powerup_option(0);
	//	system_phy_set_rfoption(1);
	// **
#elif DEF_SDK_VERSION >= 1300
	uint8 *buf = (uint8 *)os_malloc(256); // esp_init_data_default.bin
	spi_flash_read(esp_init_data_default_addr,(uint32 *)buf, esp_init_data_default_size); // esp_init_data_default.bin
#else
	uint8 *buf = (uint8 *)os_malloc(esp_init_data_default_size); // esp_init_data_default.bin
	spi_flash_read(esp_init_data_default_addr,(uint32 *)buf, esp_init_data_default_size); // esp_init_data_default.bin
#endif
	//
	if(buf[0] != 5) { // первый байт esp_init_data_default.bin не равен 5 ? - бардак!
#ifdef DEBUG_UART
		os_printf("\nError esp_init_data! Set default.\n");
#endif
		ets_memcpy(buf, esp_init_data_default, esp_init_data_default_size);
	}
//	system_restoreclock(); // STARTUP_CPU_CLK
	init_wifi(buf, info.st_mac); // инициализация WiFi
#if DEF_SDK_VERSION >= 1400
	if(buf[0xf8] == 1 || phy_rx_gain_dc_flag == 1) { // сохранить новые калибровки RF/VCC33 ?
#ifdef DEBUG_UART
		os_printf("\nSave rx_gain_dc table (%u, %u)\n", buf[0xf8], phy_rx_gain_dc_flag );
#endif
		wifi_param_save_protect_with_check(esp_init_data_default_sec, flashchip_sector_size, buf, SIZE_SAVE_SYS_CONST);
	}
#endif
	os_free(buf);
	//
#if DEF_SDK_VERSION >= 1400 // (SDK 1.4.0)
	system_rtc_mem_read(0, &rst_if, sizeof(rst_if));
//	os_printf("RTC_MEM(0) = %u,%u,%p \n", rst_if.reason, IO_RTC_SCRATCH0, RTC_RAM_BASE[0x78>>2]);
#if DEF_SDK_VERSION >= 1520
	{
		uint32 reset_reason = IO_RTC_SCRATCH0;
		if(reset_reason >= REASON_EXCEPTION_RST && reset_reason < REASON_DEEP_SLEEP_AWAKE) {
			// reset_reason == REASON_EXCEPTION_RST, REASON_SOFT_WDT_RST, REASON_SOFT_RESTART, REASON_DEEP_SLEEP_AWAKE
			TestStaFreqCalValInput = RTC_RAM_BASE[0x78>>2]>>16;
			chip_v6_set_chan_offset(1, TestStaFreqCalValInput);
		}
		else {
			TestStaFreqCalValInput = 0;
			RTC_RAM_BASE[0x78>>2] &= 0xFFFF;
			if(reset_reason == REASON_DEFAULT_RST) {
				reset_reason = rtc_get_reset_reason();
				if(reset_reason == 1) { // =1 - ch_pd
					ets_memset(&rst_if, 0, sizeof(rst_if)); // rst_if.reason = REASON_DEFAULT_RST
				}
				else if(reset_reason == 2) { // =2 - reset
					if(rst_if.reason != REASON_DEEP_SLEEP_AWAKE
					 ||	rst_if.epc1 != 0
					 || rst_if.excvaddr != 0) {
						ets_memset(&rst_if, 0, sizeof(rst_if));
						rst_if.reason = REASON_EXT_SYS_RST;
						RTC_MEM(0) = REASON_EXT_SYS_RST;
					}
				}
			}
			else if(reset_reason > REASON_EXT_SYS_RST) {
				ets_memset(&rst_if, 0, sizeof(rst_if)); // rst_if.reason = REASON_DEFAULT_RST
			}
		}
	}
#else
	if (rst_if.reason >= REASON_EXCEPTION_RST && rst_if.reason < REASON_DEEP_SLEEP_AWAKE) { // >= 2 < 5
		// 2,3,4 REASON_EXCEPTION_RST, REASON_SOFT_WDT_RST, REASON_SOFT_RESTART
		TestStaFreqCalValInput = RTC_RAM_BASE[0x78>>2]>>16; // *((volatile uint32 *)0x60001078) >> 16
		chip_v6_set_chan_offset(1, TestStaFreqCalValInput);
	}
	else {
		TestStaFreqCalValInput = 0;
		RTC_RAM_BASE[0x78>>2] &= 0xFFFF; // *((volatile uint32 *)0x60001078) &= &0xFFFF;
		if(rst_if.reason > REASON_EXT_SYS_RST) rst_if.reason = REASON_DEFAULT_RST;
		if(rst_if.reason != REASON_DEEP_SLEEP_AWAKE && rtc_get_reset_reason() == 2) {
			rst_if.reason = REASON_EXT_SYS_RST; // = 6
		}
//		else if(rst_if.reason == REASON_WDT_RST && rtc_get_reset_reason() == 1) rst_if.reason = REASON_DEFAULT_RST;
	}
#endif
	//
#ifdef DEBUG_UART
	os_print_reset_error(); // вывод фатальных ошибок, вызвавших рестарт. см. в модуле wdt
#endif
	RTC_MEM(0) = 0; //	system_rtc_mem_write(0, &rst_if, sizeof(rst_if));
#endif
//	DPORT_BASE[0] = (DPORT_BASE[0] & 0x60) | 0x0F; // ??
#if DEF_SDK_VERSION >= 1119 // (SDK 1.1.1)
	wdt_init(1);
#else
	wdt_init();
#endif
#ifdef DEBUG_UART
	uart_wait_tx_fifo_empty();
#endif
	user_init();
	user_init_flag = true;
#if DEF_SDK_VERSION >= 1200
	ets_timer_disarm(&check_timeouts_timer);
	ets_timer_arm_new(&check_timeouts_timer, lwip_timer_interval, 1, 1);
#endif
	WDT_FEED = WDT_FEED_MAGIC; // WDT
	//
	int wfmode = g_ic.g.wifi_store.wfmode[0]; // g_ic.c[0x214] (+532) SDK 1.2.0 // SDK 1.3.0 g_ic.c[472]
	wifi_mode_set(wfmode);
	if(wfmode & 1)  wifi_station_start();
#if DEF_SDK_VERSION >= 1200
	if(wfmode == 2) {
#if DEF_SDK_VERSION >= 1400
		if(g_ic.c[470] != 2) wifi_softap_start(0);
#else
		if(g_ic.c[446] != 2) wifi_softap_start(0);
#endif
		else wifi_softap_start(1);
	}
	else if(wfmode == 3) {
		wifi_softap_start(0);
	}
#else
	if(wfmode & 2)  wifi_softap_start();
#endif

#if DEF_SDK_VERSION >= 1110
	if(wfmode == 1) netif_set_default(*g_ic.g.netif1);	// struct netif *
#else
	if(wfmode) {
		struct netif * * p;
		if(wfmode == 1) p = g_ic.g.netif1; // g_ic+0x10;
		else p = g_ic.g.netif2; // g_ic+0x14;
		netif_set_default(*p);	// struct netif *
	}
#endif
	if(wifi_station_get_auto_connect()) wifi_station_connect();
	if(done_cb != NULL) done_cb();
}
//-----------------------------------------------------------------------------
#ifdef DEBUG_UART
void ICACHE_FLASH_ATTR puts_buf(uint8 ch)
{
	if(UartDev.trx_buff.TrxBuffSize < (TX_BUFF_SIZE-1)) {
		UartDev.trx_buff.pTrxBuff[UartDev.trx_buff.TrxBuffSize++] = ch;
		UartDev.trx_buff.pTrxBuff[UartDev.trx_buff.TrxBuffSize] = 0;
	}
}
#endif
//=============================================================================
// init_wifi()
//-----------------------------------------------------------------------------
const char aFATAL_ERR_R6PHY[] ICACHE_RODATA_ATTR = "register_chipv6_phy";
void ICACHE_FLASH_ATTR init_wifi(uint8 * init_data, uint8 * mac)
{
#ifdef DEBUG_UART
	uart_wait_tx_fifo_empty();
	UartDev.trx_buff.TrxBuffSize = 0;
	ets_install_putc1(puts_buf);
#endif
	if(register_chipv6_phy(init_data)){
		fatal_error(FATAL_ERR_R6PHY, init_wifi, (void *)aFATAL_ERR_R6PHY);
	}
#ifdef DEBUG_UART
	startup_uart_init();
	if(UartDev.trx_buff.TrxBuffSize) os_printf_plus(UartDev.trx_buff.pTrxBuff);
	UartDev.trx_buff.TrxBuffSize = TX_BUFF_SIZE;
#endif
	phy_disable_agc();
	ieee80211_phy_init(g_ic.g.wifi_store.phy_mode); // phy_mode
	lmacInit();
	wDev_Initialize(mac);
	pp_attach();
	ieee80211_ifattach(&g_ic);
	ets_isr_attach(0, wDev_ProcessFiq, NULL);
	ets_isr_unmask(1);
	pm_attach();
	fpm_attach();
	phy_enable_agc();
	cnx_attach(&g_ic);
	wDevEnableRx();
}
//=============================================================================
// uart_wait_tx_fifo_empty()
//-----------------------------------------------------------------------------
void ICACHE_FLASH_ATTR uart_wait_tx_fifo_empty(void)
{
	while((UART0_STATUS >> UART_TXFIFO_CNT_S) & UART_TXFIFO_CNT);
	while((UART1_STATUS >> UART_TXFIFO_CNT_S) & UART_TXFIFO_CNT);
}
//=============================================================================
// user_uart_wait_tx_fifo_empty()
// Use SDK Espressif
//-----------------------------------------------------------------------------
void user_uart_wait_tx_fifo_empty(uint32 uart_num, uint32 x)
{
	if(uart_num) while((UART1_STATUS >> UART_TXFIFO_CNT_S) & UART_TXFIFO_CNT);
	else while(((UART0_STATUS >> UART_TXFIFO_CNT_S) & UART_TXFIFO_CNT) && (x--));
}
