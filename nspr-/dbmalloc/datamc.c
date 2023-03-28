
/*
 * (c) Copyright 1990, 1991, 1992 Conor P. Cahill (cpcahil@virtech.vti.com)
 *
 * This software may be distributed freely as long as the following conditions
 * are met:
 * 		* the distribution, or any derivative thereof, may not be
 *		  included as part of a commercial product
 *		* full source code is provided including this copyright
 *		* there is no charge for the software itself (there may be
 *		  a minimal charge for the copying or distribution effort)
 *		* this copyright notice is not modified or removed from any
 *		  source file
 */

/*
 * datamc.c - this module contains functions designed to efficiently 
 *	      copy memory areas in a portable fasion
 *
 * The configure script will usually override this module with a copy
 * of the system suplied memcpy() utility if it can figure out how to
 * convert the module name to DataMS.
 */
#ifndef lint
static
char rcs_hdr[] = "$Id: datamc.c,v 1.1 1996/06/18 03:29:13 warren Exp $";
#endif

#include <stdio.h>
#include "mallocin.h"

typedef int word;

#define wordmask (sizeof(word)-1)

#ifndef DONT_USE_ASM
#ifdef i386
#define ASM_MEMCPY	1

/*
 * DataMemcpy() - asm version of memcpy for use on 386 architecture systems.
 *		  This is much faster than performing the operation directly.
 *		  Note that the operation is performed by the use of word
 *		  moves followed by byte moves and it doesn't matter that
 *		  the word moves are not on a word aligned offset.
 *
 * This function differs from the system memcpy in that it correcly handles
 * overlapping memory regions.  This is a requirement here because this is
 * the same function that is called from memmove, memcpy, and bcopy.
 */
/*
 * DataMemcpy(tgt,src,len)
 */
	asm("	.text");
	asm("	.align 4");
#ifdef USE_UNDERSCORE
	asm("	.globl _DataMC");
	asm("_DataMC:");
#else
	asm("	.globl DataMC");
	asm("DataMC:");
#endif
	asm("	pushl  %edi");
	asm("	pushl  %esi");
	asm("	pushl  %ebx");

	/*
	 * get tgt -> edi
	 *     src -> esi
	 *     len -> ecx
	 */
	asm("	movl   16(%esp),%edi");
	asm("	movl   20(%esp),%esi");
	asm("	movl   24(%esp),%ecx");

	/*
	 * compare target to src
	 */
 	asm("	cmpl   %edi,%esi");
	/*
	 * if( tgt == src ) nothing to do, so return
	 */
	asm("   je     .memdone"); 
	/*
	 * if(    (tgt > src)
	 */
	asm("	jg     .movenorm");
	/*
	 *     &&  tgt < (src + len)
	 */
	asm("	movl	%esi,%eax");
	asm("	addl	%ecx,%eax");
	asm("	cmpl	%edi,%eax");
	asm("   jl	.movenorm");

	/*
	 * {
	 */
		/*
		 * move the pointers to the end of the data area to be 
		 * moved and set the direction flag so that 
		 */

		/*
		 *     && ( len >= 4 ) )
		 */
		asm("	cmpl	$4,%ecx");
		asm("	jl	.moveshort");
		/*
		 * {
		 */

			asm("	addl    %ecx,%edi");
			asm("	subl	$4,%edi");
			asm("	addl    %ecx,%esi");
			asm("	subl	$4,%esi");
			asm("	xor	%ebx,%ebx");
			asm("	jmp	.moveback");
		/* 
	 	 * }
		 * else
		 * {
	 	 */
			asm("	addl    %ecx,%edi");
			asm("	addl    %ecx,%esi");
			asm(".moveshort:");
			asm("	movl	$1,%ebx");
		/* 
		 * }
	 	 */
		asm(".moveback:");
		asm("	std");
		asm("	jmp	.movedata");
	/*
	 * }
	 * else
	 * {
 	 */
		asm(".movenorm:");
		asm("	mov	$1,%ebx");

	/*
	 * }
	 */

	/*
	 * now go move the data
	 */
	asm(".movedata:");
	asm("	movl   %edi,%eax");
	asm("	movl   %ecx,%edx");
	asm("	shrl   $02,%ecx");
#ifdef USE_REPE
	asm("	repe");
#else
	asm("	repz");
#endif
	asm("	movsl ");

	/*
	 * if we were performing a negative move, adjust the pointer
	 * so that it points to the previous byte (right now it points
 	 * to the previous word
	 */
	asm("	cmpl	$0,%ebx");
	asm("   jnz	.movedata2");
	/*
	 * {
	 */
		asm("	addl	$3,%edi");
		asm("	addl	$3,%esi");
	/* 
	 * }
	 */

	asm(".movedata2:");	
	asm("	movl   %edx,%ecx");
	asm("	andl   $03,%ecx");
#ifdef USE_REPE
	asm("	repe");
#else
	asm("	repz");
#endif
	asm("	movsb ");

	asm("	cld");

	/*
	 * return
	 */
	asm(".memdone:");
	asm("	popl	%ebx");
	asm("	popl	%esi");
	asm("	popl	%edi");
	asm("	ret");


#endif /* i386 */
#endif /* DONT_USE_ASM */

#ifndef ASM_MEMCPY

void
DataMC(ptr1,ptr2,len)
	MEMDATA			* ptr1;
	CONST MEMDATA		* ptr2;
	MEMSIZE			  len;
{
	register MEMSIZE	  i;
	register char		* myptr1;
	register CONST char	* myptr2;

	if( len <= 0 )
	{
		return;
	}

	myptr1 = ptr1;
	myptr2 = ptr2;

	/*
	 * while the normal memcpy does not guarrantee that it will 
	 * handle overlapping memory correctly, we will try...
	 */
	if( (myptr1 > myptr2) && (myptr1 < (myptr2+len)) )
	{
		myptr1 += len;
		myptr2 += len;

		if( (((long)myptr1)&wordmask) == (((long)myptr2)&wordmask))
		{
			/*
			 * if the pointer is not on an even word boundary
			 */
			if( (i = (((long)myptr1) & wordmask)) != 0 )
			{
				/*
				 * process an extra byte at the word boundary
				 * itself
				 */
				while( (i-- > 0) && (len > 0) )
				{
					*(--myptr1) = *(--myptr2);
					len--;
				}
			}

			/*
			 * convert remaining number of bytes to number of words
			 */
			i = len >> (sizeof(word)/2);

			/*
			 * and copy them
			 */
			while( i-- > 0 )
			{
				myptr1 -= sizeof(word);
				myptr2 -= sizeof(word);
				*(word *)myptr1 = *(CONST word *)myptr2;
			}

			/*
			 * and now handle any trailing bytes
			 */
			if( (i = (len & wordmask)) != 0 )
			{
				while( i-- > 0 )
				{
					*(--myptr1) = *(--myptr2);
				}
			}
		}
		/*
		 * else we have to do it on a byte by byte basis
		 */
		else
		{
			while( len-- > 0 )
			{
				*(--myptr1) = *(--myptr2);
			}
		}
	}
	else
	{
		/*
		 * if we can make this move on even boundaries
		 */
		if( (((long)myptr1)&wordmask) == (((long)myptr2)&wordmask) )
		{
			/*
			 * if the pointer is not on an even word boundary
			 */
			if( (i = (((long)myptr1) & wordmask)) != 0 )
			{
				while( (i++ < sizeof(word)) && (len > 0) )
				{
					*(myptr1++) = *(myptr2++);
					len--;
				}
			}

			/*
			 * convert remaining number of bytes to number of words
			 */
			i = len >> (sizeof(word)/2);

			/*
			 * and copy them
			 */
			while( i-- > 0 )
			{
				*(word *)myptr1 = *(CONST word *)myptr2;
				myptr1 += sizeof(word);
				myptr2 += sizeof(word);
			}

			/*
			 * and now handle any trailing bytes
			 */
			if( (i = (len & wordmask)) != 0 )
			{
				while( i-- > 0 )
				{
					*(myptr1++) = *(myptr2++);
				}
			}
		}
		/*
		 * else we have to handle it a byte at a time
		 */
		else
		{
			while( len-- > 0 )
			{
				*(myptr1++) = *(myptr2++);
			}
		}
	}
	
	return;

} /* DataMC(... */

#endif /* ASM_MEMCPY */

