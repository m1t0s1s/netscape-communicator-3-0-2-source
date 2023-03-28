	.code
	
ret_cr16
	.proc
	.callinfo frame=0, no_calls
	.export ret_cr16	
	.enter
	bv r0(rp)
	mfctl cr16,ret0
	.leave
	.procend
