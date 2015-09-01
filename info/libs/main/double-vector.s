
	.begin	literal_prefix	.DoubleExceptionVector
	.section	.DoubleExceptionVector.text, "ax"

	.align 4
	.global _DoubleExceptionVector

_DoubleExceptionVector:

1:	break	1,4
	j	1b

	.size	_DoubleExceptionVector, . - _DoubleExceptionVector

	.end	literal_prefix



