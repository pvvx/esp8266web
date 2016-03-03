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

#ifdef USE_TIMER0

typedef void (*nmi_func_t)(uint32 par);

extern nmi_func_t timer0_cb;
extern uint32 timer0_arg;

#ifdef TIMER0_USE_NMI_VECTOR

/*
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

void NMI_Handler(void)
{
	do {
		DPORT_BASE[0] = (DPORT_BASE[0] & 0x60) | 0x0E;
		MEMW();
	} while (DPORT_BASE[0]&1);
	// User code
	// ...	uart0_write_char('*');
	TIMER0_INT &= 0xFFE;
	if((TIMER0_CTRL & TM_AUTO_RELOAD_CNT) == 0) {
		INTC_EDGE_EN &= ~BIT(1);
	}
	xthal_set_intclear(8);
}
*/
#endif

void _timer0_isr(void * arg)
{
	if(timer0_cb != NULL) {
		timer0_cb(timer0_arg);
		if((TIMER0_CTRL & TM_AUTO_RELOAD_CNT) == 0) {
			INTC_EDGE_EN &= ~BIT(1);
		}
	}
	else {
		INTC_EDGE_EN &= ~BIT(1);
		TIMER0_CTRL = 0;
	}
	TIMER0_INT &= (~1);
}

void ICACHE_FLASH_ATTR timer0_stop(void)
{
	ets_isr_mask(BIT(ETS_FRC_TIMER0_INUM));
	TIMER0_CTRL = 0;
	INTC_EDGE_EN &= ~BIT(1);
}

void ICACHE_FLASH_ATTR timer0_start(uint32 us, bool repeat_flg)
{
	TIMER0_LOAD = 0xFFFFFFFF;
	if(repeat_flg) {
		TIMER0_CTRL =   TM_DIVDED_BY_16
		                  | TM_AUTO_RELOAD_CNT
		                  | TM_ENABLE_TIMER
		                  | TM_EDGE_INT;
	}
	else {
		TIMER0_CTRL =   TM_DIVDED_BY_16
		                  | TM_ENABLE_TIMER
		                  | TM_EDGE_INT;
	}
	if(us < 5) {
		TIMER0_LOAD = 40;
	}
	else if(us <= 1677721) {
		TIMER0_LOAD = us * 5;
	}
	ets_isr_unmask(BIT(ETS_FRC_TIMER0_INUM));
	INTC_EDGE_EN |= BIT(1);
}

#ifdef TIMER0_USE_NMI_VECTOR
void timer0_init(void *func, uint32 par, bool nmi_flg)
#else
void timer0_init(void *func, uint32 par)
#endif
{
#if DEBUGSOO > 3
	os_printf("timer0_init(%d)\n", flg);
#endif
	timer0_stop();
	timer0_cb = func;
	timer0_arg = par;
#ifdef TIMER0_USE_NMI_VECTOR
	if(nmi_flg) {
		DPORT_BASE[0] |= 0x0F;
	}
	else
#endif
	{
		DPORT_BASE[0] = 0;
		ets_isr_attach(ETS_FRC_TIMER0_INUM, _timer0_isr, NULL);
	}
	ets_isr_unmask(BIT(ETS_FRC_TIMER0_INUM));
}
#endif // USE_TIMER0
/* In eagle.app.v6.ld:
  .text : ALIGN(4)
  {
    _stext = .;
    _text_start = ABSOLUTE(.);
    *(.vectors.text) <--
    *(.entry.text)
    ..... */
void __attribute__((section(".vectors.text"))) jump_boot(void)
{
	__asm__ __volatile__ (
// +0x00 0x40100000
			"j			call_jump_boot\n"
#if defined(TIMER0_USE_NMI_VECTOR)
			".align 	16\n"
//			".global	_DebugExceptionVector\n"
"_DebugExceptionVector:\n"	// +0x10
			"1:	waiti	2\n" // XCHAL_DEBUGLEVEL // unexpected debug exception, loop in low-power mode
			"j			1b\n" 		// infinite loop - unexpected debug exception
#endif
#if defined(USE_TIMER0)
			".align 	4\n"
			".global	timer0_cb, timer0_arg\n"
"timer0_cb:	.word	0\n"
"timer0_arg:.word	0\n"
#endif
#if defined(USE_TIMER0) && defined(TIMER0_USE_NMI_VECTOR)
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
"ptimer_:	.word timer_\n"
"pxthal_set_intclear: 	.word xthal_set_intclear\n"
"p_dport_:	.word	0x3ff00000\n"
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
			"l32r 	a0, p_dport_\n" // DPORT_BASE[0] = 0x0F;
			"movi.n	a3, 0x0E\n"
			"s32i.n	a3, a0, 0\n"
"4:\n"
			"memw\n"
			"l32i.n a3, a0, 0\n"	// while(DPORT_BASE[0]&1);
			"bbsi	a3, 0, 4b\n"

			"l32r	a0, timer0_cb\n" // if(timer0_cb !=0) timer0_cb(timer0_arg)
			"beqz	a0, 5f\n"
			"l32r	a2, timer0_arg\n"
			"callx0	a0\n"
"5:\n"
			"l32r	a0, ptimer_\n"	// TIMER0_INT &= 0xFFE;
			"l32i.n	a2, a0, 12\n"
			"movi.n	a3, -2\n"
			"and	a2, a2, a3\n"
			"s32i.n	a2, a0, 12\n"

			"l32i.n	a2, a0, 8\n" // if((TIMER0_CTRL & TM_AUTO_RELOAD_CNT) == 0)
			"bbsi	a2, 6, 6f\n"

			"l32r 	a0, p_dport_\n" // INTC_EDGE_EN &= ~BIT(1);
			"l32i.n	a2, a0, 4\n"
			"movi.n	a3, -3\n"
			"and	a2, a2, a3\n"
			"s32i.n	a2, a0, 4\n"
"6:"
			"l32r	a0, pxthal_set_intclear\n" // xthal_set_intclear(8);
			"movi.n	a2, 8\n"
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

