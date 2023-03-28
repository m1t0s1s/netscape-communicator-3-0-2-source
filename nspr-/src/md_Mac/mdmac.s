						EXPORT 	.ll_udivmod32
						EXPORT 	.CountLeadingZeros
						EXPORT 	.memset
						EXPORT	.LL_MUL32

################################################################################
#
#	ll_udivmod32
#
################################################################################

.ll_udivmod32:			
						cmpi		cr1, r7, 0
	
						li			r11, 0
						li			r9, 32
						
						#	If someone tried to divide by zero, then they lose
						#	big time.
						
						beqlr		cr1

						#	If the denominator has the high-bit set, we need
						#	to do some fancy foot work.  Otherwise, procede
						#	with division as usual.

						bge			cr1, repeatdivide

highbitset:
						#	Shift the denominator down by a power of two.  
						#	This will allow us to proceed with the 
						#	normal division algorithm, and just clean up
						#	at the end.  Also remember if the low bit 
						#	of the denominator is set.

						andi.		r8, r7, 1
						cmpi		cr0, r8, 1
						
						srwi		r7, r7, 1
						
						#	Keep both pieces of information that we need
						#	about the denominator (i.e. whether it's low and
						#	bits were set) in CR1... we will need them 
						#	later.
						
						crand		6, 2, 2
						
repeatdivide:			
						cntlzw		r10, r5
						
						cmplw		r10, r9
						ble			dontpinshift
				
						mr			r10, r9
						
dontpinshift:			
						slw			r5, r5, r10
						
						li			r8, 32
						sub			r8, r8, r10

						srw			r8, r6, r8
						slw			r6, r6, r10
						
						or			r6, r6, r8
						or			r5, r5, r8
						
						divwu		r12, r5, r7

						mullw		r8, r7, r12
						sub			r5, r5, r8

						sub.		r9, r9, r10
						slw			r12, r12, r9
						add			r11, r11, r12
						
						bne			repeatdivide

finishup:

						#	If the high bit of the denominator was not
						#	originally set, then we are done.  If it was,
						#	then we need to adjust the result for the 
						#	fact that we shifted the denominator down by one
						#	bit.

						bge			cr1, highBitWasntSet
						
						#	If the low bit of the result.hi was set, make
						#	sure to add the shifted denominator back into the
						#	remainder once before we shift result.hi down
						#	to compensate for the fact that we shifted 
						#	the it down originally.
						
						andi.		r10, r11, 1
						beq			dontAddShiftedDivisorToRemainder
						
						add			r5, r5, r7
						
dontAddShiftedDivisorToRemainder:

						#	Finally shift result.lo down. 

						srwi		r11, r11, 1
						
						#	If the low bit of our denominator was originally
						#	set, then we are still not done.  We may have
						#	overestimated how many times the it could
						#	go into the numerator
						
						bne			cr1, lowBitWasntSet
						
						#	Restore the denominator to its original glory
						
						slwi		r7, r7, 1
						ori			r7, r7, 1
						
						#	Subtract result.hi from result.lo.  This will
						#	give us an adjusted remainder that includes the
						#	lowest significant bit of the denominator
						
						sub.		r10, r5, r11
						
						#	It is possible that the remainder will no longer
						#	be in the proper range (it went below zero)
						#	Add the denominator to it until
						#	it goes possible (this can only happen twice),
						#	each time decrementing result.hi
						
						cmplw		r10, r5
						ble			noMoreAdjustments
						
						subi		r11, r11, 1
						add.		r5, r10, r7
						
						cmplw		r5, r10
						ble			noMoreAdjustments2
						
						subi		r11, r11, 1
						add.		r10, r5, r7
												
noMoreAdjustments:
						mr			r5, r10
						
noMoreAdjustments2:
highBitWasntSet:
lowBitWasntSet:

						stw			r11, 0(r3)
						stw			r5, 0(r4)
						
						blr


################################################################################
#
#	CountLeadingZeros
#
################################################################################

.LL_MUL32	
						mullw	r6, r3, r4 
						stw		r6, 4(r5)
						mulhwu	r6, r3, r4
						stw		r6, 0(r5)
						blr

################################################################################
#
#	CountLeadingZeros
#
################################################################################

.CountLeadingZeros:		
						cntlzw		r3, r3
						blr
						
						
################################################################################
#
#	.memset
#
################################################################################
					
.memset

						cmplwi   	r5, 0
						mr			r7, r3
						li			r8, 0
						li			r6, 0
						beqlr
						
						#	If we are filling with zeros, then we don’t have
						#	to create a four-byte copy of the fill byte,
						#	we already have it:  0x00000000.
						
						rlwinm.  	r0,r4,0,24,31
						beq			checkalignment
						
						#	Create a four-byte copy of the fill byte, put
						#	it in r6.
						
						rlwinm   	r0,r4,0,24,31
						extsh    	r0,r0
						rlwinm   	r6,r4,0,24,31
						rlwinm   	r0,r0,8,0,23
						or       	r8,r6,r0
						rlwinm   	r0,r8,0,16,31
						rlwinm   	r6,r8,0,16,31
						rlwinm   	r0,r0,16,0,15
						or       	r6,r6,r0
						
						b			checkalignment

alignbegin:
						#	Try to align the destination pointer to a 
						#	word boundary.

						stb      	r4,0(r7)
						subi     	r5,r5,1
						addi     	r7,r7,1
						
checkalignment:

						cmplwi   	r5,$0000
						beq      	alignedfillheader
						
						rlwinm.  	r0,r7,0,30,31
						bne      	alignbegin
						
alignedfillheader:

						rlwinm.  	r0,r5,27,5,31
						beq			alignend
						
alignedfill:			stw      	r6,0(r7)
						stwu    	r6,4(r7)
						stwu    	r6,4(r7)
						stwu    	r6,4(r7)
						stwu    	r6,4(r7)
						stwu     	r6,4(r7)
						stwu     	r6,4(r7)
						stwu    	r6,4(r7)
						addi     	r7,r7,4
						subic.   	r0,r0,1
						bne			alignedfill

alignend:
						rlwinm   	r0,r5,0,27,31
						rlwinm.  	r0,r0,30,2,31
						beq			fillLongsAtEndSkip
						
fillLongsAtEnd:
						stw      	r6,0(r7)
						addi 		r7,r7,4
						subic.   	r0,r0,1
						bne			fillLongsAtEnd
						
fillLongsAtEndSkip:

						rlwinm.  	r0,r5,0,30,30
						beq			fillWordAtEndSkip

fillWordsAtEnd:

						sth      	r8,0(r7)
						addi     	r7,r7,2

fillWordAtEndSkip:
						rlwinm.  	r0,r5,0,31,31
						beqlr
						
						stb      	r4,0(r7)
						blr
						
						