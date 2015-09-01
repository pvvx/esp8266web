
	.begin	literal_prefix	.KernelExceptionVector
	.section		.KernelExceptionVector.text, "ax"

	.align 4
	.global _KernelExceptionVector

_KernelExceptionVector:

1:	break	1,0			// unexpected kernel exception
	j	1b			// infinite loop - unexpected kernel exception

	.align 4
	.global puev_store_regs

puev_store_regs: .word	0x3FFFC000

	.size	_KernelExceptionVector, . - _KernelExceptionVector

	.end	literal_prefix



