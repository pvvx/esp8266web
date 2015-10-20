/******************************************************************************
 * FileName: xtos_bios.h
 * Description: xtos funcs in ROM-BIOS
 * Alternate SDK
 * Author: PV`
 * (c) PV` 2015
*******************************************************************************/
#ifndef _INCLUDE_BIOS_XTOS_H_
#define _INCLUDE_BIOS_XTOS_H_

#ifndef XTRUNTIME_H

/*typedef void (_xtos_timerdelta_func)(int);*/
#ifdef __cplusplus
typedef void (_xtos_handler_func)(...);
#else
typedef void (_xtos_handler_func)();
#endif
typedef _xtos_handler_func *_xtos_handler;


extern unsigned int	_xtos_ints_off( unsigned int mask );
extern unsigned int	_xtos_ints_on( unsigned int mask );
extern _xtos_handler	_xtos_set_interrupt_handler( int n, _xtos_handler f );
extern _xtos_handler	_xtos_set_interrupt_handler_arg( int n, _xtos_handler f, void *arg );
extern _xtos_handler	_xtos_set_exception_handler( int n, _xtos_handler f );
extern unsigned		_xtos_set_intlevel( int intlevel );
extern unsigned		_xtos_set_min_intlevel( int intlevel );
extern unsigned		_xtos_set_vpri( unsigned vpri );

extern unsigned	_xtos_restore_intlevel( unsigned restoreval );


/*
 PROVIDE ( _xtos_ints_off = 0x4000bda4 );
 PROVIDE ( _xtos_ints_on = 0x4000bd84 );
 PROVIDE ( _xtos_restore_intlevel = 0x4000056c );
 PROVIDE ( _xtos_set_exception_handler = 0x40000454 );
 PROVIDE ( _xtos_set_interrupt_handler = 0x4000bd70 );
 PROVIDE ( _xtos_set_interrupt_handler_arg = 0x4000bd28 );
 PROVIDE ( _xtos_set_intlevel = 0x4000dbfc );
 PROVIDE ( _xtos_set_min_intlevel = 0x4000dc18 );
 PROVIDE ( _xtos_set_vpri = 0x40000574 );
PROVIDE ( _xtos_return_from_exc = 0x4000dc54 ); // \xtensa-elf\src\xtos\exc-return.S

PROVIDE ( _xtos_alloca_handler = 0x4000dbe0 );
PROVIDE ( _xtos_c_wrapper_handler = 0x40000598 );
PROVIDE ( _xtos_cause3_handler = 0x40000590 );
PROVIDE ( _xtos_l1int_handler = 0x4000048c );
PROVIDE ( _xtos_p_none = 0x4000dbf8 );
PROVIDE ( _xtos_syscall_handler = 0x4000dbe4 );
PROVIDE ( _xtos_unhandled_exception = 0x4000dc44 );
PROVIDE ( _xtos_unhandled_interrupt = 0x4000dc3c );
*/
/*
_rom_store:
_xtos_unhandled_exception   // 0 IllegalInstruction
_xtos_syscall_handler       // 1 Syscall
_xtos_unhandled_exception   // 2 InstructionFetchError
_xtos_unhandled_exception   // 3 LoadStoreError
_xtos_l1int_handler         // 4 Level1Interrupt
_xtos_alloca_handler		// 5 Alloca (MOVSP)
_xtos_unhandled_exception   // 6 IntegerDivideByZero
_xtos_unhandled_exception   // 7 Speculation
_xtos_unhandled_exception   // 8 Privileged
_xtos_unhandled_exception   // 9 Unaligned
_xtos_unhandled_exception   //10 (reserved for Tensilica)
_xtos_unhandled_exception   //11 (reserved for Tensilica)
_xtos_cause3_handler        //12 PIF data error on fetch
_xtos_cause3_handler        //13 PIF data error on ld/st
_xtos_cause3_handler        //14 PIF address error on fetch
_xtos_cause3_handler        //15 PIF address error on ld/st
_xtos_unhandled_exception   //16 InstTLBMiss
_xtos_unhandled_exception   //17 InstTLBMultiHit
_xtos_unhandled_exception   //18 InstFetchPrivilege
_xtos_unhandled_exception   //19 (reserved for Tensilica)
_xtos_unhandled_exception   //20 InstFetchProhibited
_xtos_unhandled_exception   //21 (reserved for Tensilica)
_xtos_unhandled_exception   //22 (reserved for Tensilica)
_xtos_unhandled_exception   //23 (reserved for Tensilica)
_xtos_unhandled_exception   //24 LoadStoreTLBMiss
_xtos_unhandled_exception   //25 LoadStoreTLBMultiHit
_xtos_unhandled_exception   //26 LoadStorePrivilege
_xtos_unhandled_exception   //27 (reserved for Tensilica)
_xtos_unhandled_exception   //28 LoadProhibited
_xtos_unhandled_exception   //29 StoreProhibited
_xtos_unhandled_exception   //30 (reserved for Tensilica)
_xtos_unhandled_exception   //31 (reserved for Tensilica)
....
_xtos_p_none
....
*/
#endif // XTRUNTIME_H
#endif // _INCLUDE_BIOS_XTOS_H_
