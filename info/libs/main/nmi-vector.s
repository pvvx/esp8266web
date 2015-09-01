// The NMI exception vector handles non-maskable interrupts.

	.begin	literal_prefix	.NMIExceptionVector
	.section	.NMIExceptionVector.text, "ax"

	.align	4
	.global _NMIExceptionVector

_NMIExceptionVector:

//		rfi		3 
		wsr.excsave3 a0		//	wsr	a0, EXCSAVE+XCHAL_NMILEVEL
		call0 _NMILevelVector

	.size	_NMIExceptionVector, . - _NMIExceptionVector

	.end	literal_prefix

	.begin	literal_prefix	.NMILevelVector
	.section	.NMILevelVector.text, "ax"

	.align	4
			.global _NMILevelVector
	
_NMILevelVector:
			l32r	a0, _store_regs						        
			s32i.n	a2, a0, 40									 
			l32r	a2, _xxx_a
			s32i.n	a1, a0, 36									 
			l32i.n	a2, a2, 0									  
			s32i.n	a3, a0, 44									 
			xsr.excsave3	a2									 
			s32i.n	a4, a0, 48									 
			s32i.n	a2, a0, 32									 
			rsr.epc1	a3									     
			rsr.exccause	a4									 
			s32i.n	a3, a0, 12									 
			s32i.n	a4, a0, 16									 
			rsr.excvaddr	a3									 
			s32i.n	a3, a0, 20									 
			rsr.excsave1	a4									 
			s32i.n	a4, a0, 24									 
			s32i.n	a5, a0, 52									 
			s32i.n	a6, a0, 56									 
			s32i.n	a7, a0, 60									 
			s32i	a8, a0, 64									 
			s32i	a9, a0, 68									 
			s32i	a10, a0, 72									
			s32i	a11, a0, 76									
			s32i	a12, a0, 80									
			s32i	a13, a0, 84									
			s32i	a14, a0, 88									
			s32i	a15, a0, 92									
			l32r	a1, _store_regs						        
			movi.n	a0, 0									      
			movi.n	a2, 35									     
			wsr.ps	a2									         
			rsync												 
			rsr.sar	a14									    
			s32i.n	a14, a1, 0									 
			l32r	a13, pNmiFunc						          
			callx0	a13									        
			l32i.n	a15, a1, 0									 
			wsr.sar	a15									    
			movi.n	a2, 51									     
			wsr.ps	a2									         
			rsync												 
			l32i.n	a4, a1, 48									 
			l32i.n	a5, a1, 52									 
			l32i.n	a6, a1, 56									 
			l32i.n	a7, a1, 60									 
			l32i	a8, a1, 64									 
			l32i	a9, a1, 68									 
			l32i	a10, a1, 72									
			l32i	a11, a1, 76									
			l32i	a12, a1, 80									
			l32i	a13, a1, 84									
			l32i	a14, a1, 88									
			l32i	a15, a1, 92									
			l32i.n	a2, a1, 12									 
			l32i.n	a3, a1, 16									 
			wsr.epc1	a2									     
			wsr.exccause	a3									 
			l32i.n	a2, a1, 20									 
			wsr.excvaddr	a2									 
			l32i.n	a3, a1, 24									 
			wsr.excsave1	a3									 
			l32i.n	a0, a1, 32									 
			rsr.sar	a3									     
			l32r 	a2, p_dport_						           
			movi	a2, 0x3ff									  
			slli	a2, a2, 20									 
			wsr.sar	a3									     
			movi.n	a3, 15									     
			s32i.n	a3, a2, 0									  
			l32i.n	a2, a1, 40									 
			l32i.n	a3, a1, 44									 
			l32i.n	a1, a1, 36									 
			rfi	3
			          
