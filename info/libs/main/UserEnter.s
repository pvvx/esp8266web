
	.begin	literal_prefix	.UserEnter
	.section	.UserEnter.text, "ax"

	.align 4
	.global call_user_start

call_user_start:
		movi	a2, 0x401
		slli	a2, a2, 20
		wsr.vecbase	a2
		j	call_user_start1

//	.size	call_user_start, . - call_user_start
	
	.end	literal_prefix



