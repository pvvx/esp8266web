/*
 * Xtensa Special Register symbolic names
 */

#ifndef __SPECREG_H
#define __SPECREG_H

/*  Special registers:  */
#define LBEG		0
#define LEND		1
#define LCOUNT		2
#define SAR			3
#define BR			4
#define LITBASE		5
#define SCOMPARE1	12
#define ACCLO		16
#define ACCHI		17
#define MR_0		32
#define MR_1		33
#define MR_2		34
#define MR_3		35
#define PREFCTL		40
#define WINDOWBASE	72
#define WINDOWSTART	73
#define PTEVADDR	83
#define RASID		90
#define ITLBCFG		91
#define DTLBCFG		92
#define IBREAKENABLE	96
#define CACHEATTR	98
#define DDR			104
#define IBREAKA_0	128
#define IBREAKA_1	129
#define DBREAKA_0	144
#define DBREAKA_1	145
#define DBREAKC_0	160
#define DBREAKC_1	161
#define EPC_1		177
#define EPC_2		178
#define EPC_3		179
#define EPC_4		180
#define EPC_5		181
#define EPC_6		182
#define EPC_7		183
#define DEPC		192
#define EPS_2		194
#define EPS_3		195
#define EPS_4		196
#define EPS_5		197
#define EPS_6		198
#define EPS_7		199
#define EXCSAVE_1	209
#define EXCSAVE_2	210
#define EXCSAVE_3	211
#define EXCSAVE_4	212
#define EXCSAVE_5	213
#define EXCSAVE_6	214
#define EXCSAVE_7	215
#define CPENABLE	224
#define INTERRUPT	226
#define INTREAD		INTERRUPT	/* alternate name for backward compatibility */
#define INTSET		INTERRUPT	/* alternate name for backward compatibility */
#define INTCLEAR	227
#define INTENABLE	228
#define PS			230
#define VECBASE		231
#define EXCCAUSE	232
#define DEBUGCAUSE	233
#define CCOUNT		234
#define PRID		235
#define ICOUNT		236
#define ICOUNTLEVEL	237
#define EXCVADDR	238
#define CCOMPARE_0	240
#define CCOMPARE_1	241
#define CCOMPARE_2	242
#define MISC_REG_0	244
#define MISC_REG_1	245
#define MISC_REG_2	246
#define MISC_REG_3	247

/*  Special cases (bases of special register series):  */
#define MR			32
#define IBREAKA		128
#define DBREAKA		144
#define DBREAKC		160
#define EPC			176
#define EPS			192
#define EXCSAVE		208
#define CCOMPARE	240
#define MISC_REG	244

#define __stringify_1(x...)  #x
#define __stringify(x...)  __stringify_1(x)
#define RSR(sr) ({uint32_t r; asm volatile ("rsr %0,"__stringify(sr) : "=a"(r)); r;})

#endif /* __SPECREG_H */
