/******************************************************************************
 * FileName: xthal_bios.h
 * Description: HAL funcs in ROM-BIOS
 * Alternate SDK
 * Author: PV`
 * (c) PV` 2015
*******************************************************************************/
#ifndef _INCLUDE_BIOS_XTAL_H_
#define _INCLUDE_BIOS_XTAL_H_

#ifndef XTENSA_HAL_H // xtensa-lx106-elf/include/xtensa/hal.h

// Instruction/Data RAM/ROM Access
extern void* xthal_memcpy(void *dst, const void *src, unsigned len);
extern void* xthal_bcopy(const void *src, void *dst, unsigned len);

/* set and get CCOMPAREn registers (if not present, get returns 0) */
extern void     xthal_set_ccompare(int x, unsigned r); // { if(x) return; wsr.ccompare0 = r }
extern unsigned xthal_get_ccompare(int x); // { if(x) return 0; else return rsr.ccompare0 }

/* get CCOUNT register (if not present return 0) */
extern unsigned xthal_get_ccount(void); // { return rsr.ccount}

/*  INTENABLE,INTERRUPT,INTSET,INTCLEAR register access functions:  */
extern unsigned  xthal_get_interrupt( void ); // { return rsr.interrupt }
#define xthal_get_intread	xthal_get_interrupt	/* backward compatibility */
extern void      xthal_set_intclear( unsigned ); // { return wsr.intclear }

// Register Windows
/*  This spill any live register windows (other than the caller's):
 *  (NOTE:  current implementation require privileged code, but
 *   a user-callable implementation is possible.)  */
extern void      xthal_window_spill( void ); // { return; }
extern void      xthal_window_spill_nw( void ); // { return 0; }

extern unsigned xthal_spill_registers_into_stack_nw(void); // { return 0; }


/* eagle.rom.addr.v6.ld
PROVIDE ( xthal_bcopy = 0x40000688 );
-PROVIDE ( xthal_copy123 = 0x4000074c );
PROVIDE ( xthal_get_ccompare = 0x4000dd4c );
PROVIDE ( xthal_get_ccount = 0x4000dd38 );
PROVIDE ( xthal_get_interrupt = 0x4000dd58 );
PROVIDE ( xthal_get_intread = 0x4000dd58 );
PROVIDE ( xthal_memcpy = 0x400006c4 );
PROVIDE ( xthal_set_ccompare = 0x4000dd40 );
PROVIDE ( xthal_set_intclear = 0x4000dd60 );
PROVIDE ( xthal_spill_registers_into_stack_nw = 0x4000e320 );
PROVIDE ( xthal_window_spill = 0x4000e324 );
PROVIDE ( xthal_window_spill_nw = 0x4000e320 );
*/

#endif /* XTENSA_HAL_H */
#endif /* _INCLUDE_BIOS_XTAL_H_ */
