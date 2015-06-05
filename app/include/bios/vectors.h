/******************************************************************************
 * FileName: Vectors.h
 * Description: Vectors funcs in ROM-BIOS
 * Alternate SDK
 * Author: PV`
 * (c) PV` 2015
*******************************************************************************/

#ifndef _INCLUDE_BIOS_VECTORS_H_
#define _INCLUDE_BIOS_VECTORS_H_

/*
PROVIDE ( _DebugExceptionVector = 0x40000010 );
PROVIDE ( _NMIExceptionVector = 0x40000020 );
PROVIDE ( _KernelExceptionVector = 0x40000030 );
PROVIDE ( _UserExceptionVector = 0x40000050 );
PROVIDE ( _DoubleExceptionVector = 0x40000070 );
#PROVIDE ( _ResetVector = 0x40000080 );
PROVIDE ( _ResetHandler = 0x400000a4 );
*/

void _ResetVector(void);

#endif /* _INCLUDE_BIOS_VECTORS_H_ */
