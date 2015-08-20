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
	uint32 a0;	// +0x00
	uint32 a1;	// +0x04
	uint32 a2;	// +0x08
	uint32 a3;	// +0x0c
	uint32 a4;	// +0x10
	uint32 a5;	// +0x14
	uint32 a6;	// +0x18
	uint32 a7;	// +0x1c
	uint32 a8;	// +0x20
	uint32 a9;	// +0x24
	uint32 a10;	// +0x28
	uint32 a11;	// +0x2c
//	uint32 a12;	// +0x30
//	uint32 a13;	// +0x34
//	uint32 a14;	// +0x38
//	uint32 a15;	// +0x3c
//	uint32 sar;	// +0x40
//	uint32 epc1; // +0x44
//	uint32 exccause;	// +0x48
//	uint32 excvaddr;	// +0x4c
//	uint32 excsave1;	// +0x50
//	uint32 excsave2;	// +0x54
//	uint32 excsave3;	// +0x58
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
//			"wsr.excsave3 a0\n"         // preserve original a0 register
			"addmi	a1, a1, -108\n"
			"s32i.n	a0, a1, 0x00\n" // a0
			"s32i.n	a2, a1, 0x08\n" // a2
			"addmi	a2, a1, 108\n"
			"s32i.n	a2, a1, 0x04\n" // a1
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
			"s32i.n	a3, a1, 0x0c\n" // a3
			"s32i.n	a4, a1, 0x10\n" // a4
			"s32i.n	a5, a1, 0x14\n" // a5
			"s32i.n	a6, a1, 0x18\n" // a6
			"s32i.n	a7, a1, 0x1c\n" // a7
			"s32i.n	a8, a1, 0x20\n" // a8
			"s32i.n	a9, a1, 0x24\n" // a9
			"s32i.n	a10, a1, 0x28\n" // a10
			"s32i.n	a11, a1, 0x2c\n" // a11
//			"s32i.n	a12, a1, 0x30\n" // a12
//			"s32i.n	a13, a1, 0x34\n" // a13
//			"s32i.n	a14, a1, 0x38\n" // a14
//			"s32i.n	a15, a1, 0x3c\n" // a15
			"movi.n	a3, 0x23\n"
			"wsr.ps	a3\n"
			"rsync\n"
			"rsr.sar	a4\n"
			"s32i.n	a4, a1, 0x30\n" // sar
			"l32r	a0, pNmiFunc\n"
			"callx0	a0\n"
			"l32i.n	a4, a1, 0x30\n" // sar
			"wsr.sar	a4\n"
			"movi.n	a2, 0x33\n"
			"wsr.ps	a2\n"
			"rsync\n"
			"l32i.n	a3, a1, 0x0c\n" // a3
			"l32i.n	a4, a1, 0x10\n" // a4
			"l32i.n	a5, a1, 0x14\n" // a5
			"l32i.n	a6, a1, 0x18\n" // a6
			"l32i.n	a7, a1, 0x1c\n" // a7
			"l32i.n	a8, a1, 0x20\n" // a8
			"l32i.n	a9, a1, 0x24\n" // a9
			"l32i.n	a10, a1, 0x28\n" // a10
			"l32i.n	a11, a1, 0x2c\n" // a11
//			"l32i.n	a12, a1, 0x30\n" // a12
//			"l32i.n	a13, a1, 0x34\n" // a13
//			"l32i.n	a14, a1, 0x38\n" // a14
//			"l32i.n	a15, a1, 0x3c\n" // a15
			"l32r 	a0, p_dport_\n" // DPORT_BASE[0] = 0x0F;
			"movi.n	a2, 0x0F\n"
			"s32i.n	a2, a0, 0\n"
			"l32i.n	a0, a1, 0x00\n" // a0
			"l32i.n	a2, a1, 0x08\n" // a2
			"l32i.n	a1, a1, 0x04\n" // a1
			"rfi	3\n"
#endif
		);
}

