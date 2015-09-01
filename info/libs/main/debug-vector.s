	//  This code goes at the debug exception vector

	.begin	literal_prefix	.DebugExceptionVector
	.section		.DebugExceptionVector.text, "ax"

	.align 4
	.global	_DebugExceptionVector

_DebugExceptionVector:

1:	waiti	2 // XCHAL_DEBUGLEVEL // unexpected debug exception, loop in low-power mode
	j	1b // infinite loop - unexpected debug exception

	.global pNmiFunc
	.align 4

pNmiFunc: .word 0

	.size	_DebugExceptionVector, . - _DebugExceptionVector

	.end	literal_prefix



