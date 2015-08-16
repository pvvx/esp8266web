/******************************************************************************
 * FileName: main-vectors.c
 * Description: vectors CPU
 * Author: PV` 2015
 ******************************************************************************/
#include "sdk/sdk_config.h"
#include "bios.h"
#include "hw/esp8266.h"
#include "hw/corebits.h"
#include "hw/specreg.h"
#include "user_interface.h"
#include "sdk/app_main.h"


struct nmi_store_regs_t
{
	uint32 sar;	// +0x00
	uint32 nn04;	// +0x04
	uint32 nn08;	// +0x08
	uint32 epc1; // +0x0c
	uint32 exccause;	// +0x10
	uint32 excvaddr;	// +0x14
	uint32 excsave_1;	// +0x18
	uint32 nn1c;	// +0x1c
	uint32 a0;	// +0x20
	uint32 a1;	// +0x24
	uint32 a2;	// +0x28
	uint32 a3;	// +0x2c
	uint32 a4;	// +0x30
	uint32 a5;	// +0x34
	uint32 a6;	// +0x38
	uint32 a7;	// +0x3c
	uint32 a8;	// +0x40
	uint32 a9;	// +0x44
	uint32 a10;	// +0x48
	uint32 a11;	// +0x4c
	uint32 a12;	// +0x50
	uint32 a13;	// +0x54
	uint32 a14;	// +0x58
	uint32 a15;	// +0x5c
};

typedef void (*nmi_func_t)(void);
extern nmi_func_t pNmiFunc;

#ifdef USE_NMI_VECTOR // пока в режиме отладки, но уже работает c текущим тестом

void NMI_Handler(void)
{
	do {
		DPORT_BASE[0] = (DPORT_BASE[0] & 0x60) | 0x0E;
		MEMW();
	} while (DPORT_BASE[0]&1);
	// User code
	// ...
	uart0_write_char('*');
	xthal_set_intclear(8);
	TIMER0_INT &= 0xFFE;
	if((TIMER0_CTRL & TM_AUTO_RELOAD_CNT) == 0) {
		INTC_EDGE_EN &= ~BIT(1);
	}
}

void ICACHE_FLASH_ATTR NmiSetFunc(nmi_func_t func)
{
	DPORT_BASE[0] = (DPORT_BASE[0] & 0x60) | 0x0F;
	pNmiFunc = func;
}

void ICACHE_FLASH_ATTR init_nmi(bool flg)
{
	os_printf("init_nmi(%d)\n", flg);
    INTC_EDGE_EN &= ~BIT(1);
    if (flg) TIMER0_CTRL =   TM_DIVDED_BY_16
                      | TM_ENABLE_TIMER
					  | TM_AUTO_RELOAD_CNT
                      | TM_EDGE_INT;
    else	TIMER0_CTRL =   TM_DIVDED_BY_16
                  | TM_ENABLE_TIMER
                  | TM_EDGE_INT;
    NmiSetFunc(NMI_Handler);
    ets_isr_unmask(BIT(ETS_FRC_TIMER0_INUM));
}

void ICACHE_FLASH_ATTR stop_nmi(void)
{
	os_printf("stop_nmi()\n");
	ets_isr_mask(BIT(ETS_FRC_TIMER0_INUM));
	TIMER0_CTRL = 0;
	INTC_EDGE_EN &= ~BIT(1);
	DPORT_BASE[0] = (DPORT_BASE[0] & 0x60) | 0x0E;
}

void ICACHE_FLASH_ATTR start_nmi(uint32 us)
{
	os_printf("start_nmi(%d)\n", us);
    if(us != 0 && us <= 1677721 ) {
    	TIMER0_LOAD = us * 5;
    	INTC_EDGE_EN |= BIT(1);
    }
    else {
    	stop_nmi();
    }
}
#else
void ICACHE_FLASH_ATTR init_nmi(bool flg)
{

}
void ICACHE_FLASH_ATTR NmiSetFunc(nmi_func_t func)
{

}
void ICACHE_FLASH_ATTR stop_nmi(void)
{

}
void ICACHE_FLASH_ATTR start_nmi(uint32 us)
{

}

#endif

/* In eagle.app.v6.ld:
  .text : ALIGN(4)
  {
    _stext = .;
    _text_start = ABSOLUTE(.);
    *(.vectors.text)
    *(.entry.text)
    ..... */
void __attribute__((section(".vectors.text"))) call_user_start(void)
{
	__asm__ __volatile__ (
#ifdef USE_NMI_VECTOR
			"movi	a2, 0x401\n"
			"slli	a2, a2, 20\n" // a2 = 0x40100000
			"wsr.vecbase	a2\n"
#endif
			"j			call_user_start1\n"
#ifdef USE_NMI_VECTOR
			".align 	16\n"
//			".global	_DebugExceptionVector\n"
"_DebugExceptionVector:\n"	// +0x10
			"1:	waiti	2\n" // XCHAL_DEBUGLEVEL // unexpected debug exception, loop in low-power mode
			"j			1b\n" 		// infinite loop - unexpected debug exception
			".align 	4\n"
			".global	pNmiFunc\n"
"pNmiFunc:	.word	NMI_Handler\n"
"p_dport_:	.word	0x3ff00000\n"
			".align 	16\n"
//			".global	_NMIExceptionVector\n"
"_NMIExceptionVector:\n"	// +0x20
			"wsr.excsave3 a0\n"         // preserve original a0 register
			"j 		_NMILevelVector\n"
			".align 	16\n"
//			".global	_KernelExceptionVector\n"
"_KernelExceptionVector:\n"	// +0x30
			"2:	break	1,0\n"	// unexpected kernel exception
			"j			2b\n"			// infinite loop - unexpected kernel exception
			".align		16\n"
//			".global	ptab_user_exception_vector\n"
"ptab_user_exception_vector:	.word 0x3FFFC000\n"
			".align 	16\n"
//			".global	_UserExceptionVector\n"
"_UserExceptionVector:\n" // +0x50
			"addmi			a1, a1, -0x100\n"
			"s32i.n			a2, a1, 0x14\n"
			"s32i.n			a3, a1, 0x18\n"
			"l32r			a3, ptab_user_exception_vector\n"
			"rsr.exccause	a2\n"
			"addx4			a3, a2, a3\n"
			"l32i.n			a3, a3, 0\n"
			"s32i.n			a4, a1, 0x1C\n"
			"jx				a3\n"
			".align 	16\n"
//			".global	_DoubleExceptionVector\n"
"_DoubleExceptionVector:\n"	// +0x70
			"3:	break	1,4\n"	// unexpected exception
			"j			3b\n"	// infinite loop - unexpected kernel exception
			".align 	4\n"
"_NMILevelVector:\n"
			"mov	a0, a1\n"
			"addmi	a1, a1, -108\n"
			"s32i.n	a2, a1, 40\n"
			"s32i.n	a0, a1, 36\n" // a1
			"movi.n	a2, 0\n"
			"s32i.n	a3, a1, 44\n"
			"xsr.excsave3	a2\n"
			"s32i.n	a4, a1, 48\n"
			"s32i.n	a2, a1, 32\n" // a0
//			"rsr.epc1	a3\n"
//			"rsr.exccause	a4\n"
//			"s32i.n	a3, a1, 12\n"
//			"s32i.n	a4, a1, 16\n"
//			"rsr.excvaddr	a3\n"
//			"s32i.n	a3, a1, 20\n"
//			"rsr.excsave1	a4\n"
//			"s32i.n	a4, a1, 24\n"
			"s32i.n	a5, a1, 52\n"
			"s32i.n	a6, a1, 56\n"
			"s32i.n	a7, a1, 60\n"
			"s32i.n	a8, a1, 64\n"
			"s32i.n	a9, a1, 68\n"
			"s32i.n	a10, a1, 72\n"
			"s32i.n	a11, a1, 76\n"
			"s32i.n	a12, a1, 80\n"
			"s32i.n	a13, a1, 84\n"
			"s32i.n	a14, a1, 88\n"
			"s32i.n	a15, a1, 92\n"
			"movi.n	a0, 0\n"
			"movi.n	a2, 35\n"
			"wsr.ps	a2\n"
			"rsync\n"
			"rsr.sar	a14\n"
			"s32i.n	a14, a1, 0\n"
			"l32r	a13, pNmiFunc\n"
			"callx0	a13\n"
			"l32i.n	a15, a1, 0\n"
			"wsr.sar	a15\n"
			"movi.n	a2, 51\n"
			"wsr.ps	a2\n"
			"rsync\n"
			"l32i.n	a4, a1, 48\n"
			"l32i.n	a5, a1, 52\n"
			"l32i.n	a6, a1, 56\n"
			"l32i.n	a7, a1, 60\n"
			"l32i.n	a8, a1, 64\n"
			"l32i.n	a9, a1, 68\n"
			"l32i.n	a10, a1, 72\n"
			"l32i.n	a11, a1, 76\n"
			"l32i.n	a12, a1, 80\n"
			"l32i.n	a13, a1, 84\n"
			"l32i.n	a14, a1, 88\n"
			"l32i.n	a15, a1, 92\n"
//			"l32i.n	a2, a1, 12\n"
//			"l32i.n	a3, a1, 16\n"
//			"wsr.epc1	a2\n"
//			"wsr.exccause	a3\n"
//			"l32i.n	a2, a1, 20\n"
//			"wsr.excvaddr	a2\n"
//			"l32i.n	a3, a1, 24\n"
//			"wsr.excsave1	a3\n"
			"l32i.n	a0, a1, 32\n"
			"l32r 	a2, p_dport_\n" // DPORT_BASE[0] = 0x0F;
			"movi.n	a3, 15\n"
			"s32i.n	a3, a2, 0\n"
			"l32i.n	a2, a1, 40\n"
			"l32i.n	a3, a1, 44\n"
			"l32i.n	a1, a1, 36\n"
			"rfi	3\n"
#endif
		);
}

