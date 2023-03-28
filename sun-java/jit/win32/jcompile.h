// $Header: /m/src/ns/sun-java/jit/win32/Attic/jcompile.h,v 1.1 1996/06/10 08:21:41 jg Exp $
/*
 * Copyright (c) 1996 Borland International. All Rights Reserved.
 *
 * AppAccelerator(tm) for x86
 *
 * JCompile.h, R. Crelier, 5/30/96
 *
*/
typedef struct Item
{
    ModReg  mr;
    char    scale;
    char    size;
    long    offset;
    int     var;    // -1 if not a local

}   Item;


typedef struct Context
{
    long    espLevel;
    int     cachedVar[MR_EBP+1];	// -1 if not valid
    ModReg  *varCachingReg; // points to an array[mb->nlocals] of ModReg
			    // contents must always be a valid register number
    ModReg  *varhCachingReg;// points to an array[mb->nlocals] of ModReg
			    // contents must always be a valid register number
    int     regRefCnt[MR_ECX+1];	// registers ref count
    ModReg  roundRobin;	    // last allocated register
    Item    *top;	    // topmost valid operand is at top-1
    Item    base[1];	    // actually base[mb->maxstack]

}   Context;


typedef struct CompEnv
{
    struct methodblock	*mb;
    struct execenv	*ee;
    union  RangePtr	*rp;		// bytecode pc to range mapping
    struct RangeHdr	*epilogueRh;	// epilogue range header
    struct CodeInfo	*codeInfo;	// info about compiled code
    char	*compiledCode;	    // Intel code block and entry
    char	*pc;		    // code generator pointer
    char	*pcRedZone;	    // PC_RED_ZONE before end of code block
    long	*varOff;	    // pointer to an array[mb->nlocals]
				    // <0: -mr of reg, never MR_EAX (==0)
				    // >=0: offset relative to esp
    long	*varFreq;	    // pointer to an array[mb->nlocals]
				    // estimating the use frequency
    Context	*ctxt;		    // current context
    long	xctxt;		    // current exception context
    void	*cases; 	    // for deallocation after an exception
    char	*err;		    // NULL if no error so far

}   CompEnv;


