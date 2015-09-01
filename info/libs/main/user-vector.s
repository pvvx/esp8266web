	//  Vector code
	.begin	literal_prefix	.UserExceptionVector
	.section		.UserExceptionVector.text, "ax"

	.align 4
	.global _UserExceptionVector

_UserExceptionVector:

//	wsr.excsave1  a0
	addmi           a1, a1, -0x100
	s32i.n          a2, a1, 0x14
	s32i.n          a3, a1, 0x18
	l32r            a3, puev_store_regs
	rsr.exccause    a2
	addx4           a3, a2, a3
	l32i.n          a3, a3, 0
	s32i.n          a4, a1, 0x1C
	jx              a3

	.end	literal_prefix

