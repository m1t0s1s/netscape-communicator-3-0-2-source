// $Header: /m/src/ns/sun-java/jit/win32/borland/jcompile.c,v 1.13.6.2 1996/09/30 02:45:40 dhopwood Exp $
/*
 * Copyright (c) 1996 Borland International. All Rights Reserved.
 *
 * AppAccelerator(tm) for x86
 *
 * JCompile.c, R. Crelier, 9/24/96
 *
*/
#include "opcodes.h"
#include "oobj.h"
#include "interpreter.h"
#include "exceptions.h"

#include "jinterf.h"
#include "jcodegen.h"
#include "jcompile.h"
#include "jcompsup.h"

#define ALIGN UCALIGN

#ifdef	DEBUG
#include "opcodes.length"
#define check(assertion)    sysAssert(assertion)
static long bc_count = 0;
static long nc_count = 0;
#else
#define check(assertion)    (assertion ? (void)0 : ByteCodeError() )
#endif

/* Spill fpu results for better compatibility to the VM (slower) */
#define STRICT_FLOATING

/* Intel to bytecode size factor, used to allocate the compiled code array */
#define INIT_CODE_SIZE_FACTOR 6
static long codeSizeFactor = INIT_CODE_SIZE_FACTOR;

/* Unit of frequency for variable access */
#define FREQ_UNIT_LEV 10
#define FREQ_UNIT (1 << FREQ_UNIT_LEV)

/* Minimum remaining space required for compiling one opcode.
 * This is to avoid too frequent checking.
 * (opc_tableswitch and opc_lookupswitch are a special case)
 */
#define PC_RED_ZONE 256
static char pcInRedZone[] = "try again";

#define FRAME_WITH_XHANDLER_SIZE 36

#define OPSTACK_HEIGHT_EXCEPTION    0xcafebad0
#define BAD_BYTECODE_EXCEPTION	    0xcafebad1
#define INTERNAL_EXCEPTION	    0xcafebad2

#define IsQuadAccessibleOpnd(_n)					       \
    ((_n)->mr == MR_FST && (_n)->size == 8                                     \
     || (IsMem((_n)->mr) && (_n)->mr == ((_n)+1)->mr                           \
         && ((_n)+1)->offset - (_n)->offset == 4))

#define swap(_x)    (((_x) << 24) |					       \
                    (((_x) & 0x0000ff00) << 8) |                               \
                    (((_x) & 0x00ff0000) >> 8) |                               \
                    (((unsigned long)((_x) & 0xff000000)) >> 24))

#define pc2signedshort(pc)  ((((signed char *)(pc))[1] << 8) | pc[2])
#define pc2signedlong(pc)   ((((signed char *)(pc))[1] << 24) | (pc[2]	<< 16) \
                            | (pc[3] << 8) | (pc[4]))

#define NOT_A_VAR (-1)
#define VAR_HANDLE (0x80000000)
#define UNDEF_RH ((RangeHdr *)(-1))

#define OpndOffset(_ce, _n) ((_ce)->codeInfo->baseOff + 4*((_n) - (_ce)->ctxt->base))

typedef char* Label;

typedef enum RangeFlags
{
    RF_START	    = 0x01, // force creation of a range
    RF_JSR	    = 0x02, // target of jsr
    RF_XHANDLER     = 0x04, // exception handler entry
    RF_SET_XCTXT    = 0x08, // reachable from a different exception context
    RF_VISITED	    = 0x10, // range was visited by WalkControlFlow
    RF_XHVISITED    = 0x20, // range was visited by WalkXHControlFlow
    RF_DATABLOCK    = 0x40, // range contains data not opcodes (ie. jump tables)

}   RangeFlags;


typedef struct RangeHdr
{
    struct RangeHdr *nextRh;	// link list of all ranges in the code
    struct RangeHdr *targRh;	// target range reachable from this range
    struct RangeHdr *contRh;	// continuation range reachable from this range
    char	    *pc;	// intel pc of block entry
    char	    *xpc;	// intel pc of block entry from a block
				// with a different xctxt
    unsigned char   rangeFlags; // flags
    char	    joinCnt;	// 0: normal flow or single target,
				// -1: dead code, >0: join point
    long	    freq;	// execution frequency estimate
    int 	    topOnEntry; // absolute operand stack height on entry
    int 	    topOnExit;	// relative operand stack height on exit
    long	    bcpc;	// rel byte code pc for this block
    long	    xIndex;	// index of this handler in the exception table
    long	    xctxt;	// exception context
    Context	    *ctxt;	// opStack, regs, vars
    Label	    link;	// links all the forward branches to pc
    Label	    xlink;	// links all the forward branches to xpc

}   RangeHdr;


typedef union RangePtr
{
    struct
    {
	unsigned char	rangeFlags;
	char		joinCnt;
	unsigned char	tryEnterCnt, tryExitCnt;
    }	mark;

    RangeHdr *hdr;

}   RangePtr;


typedef struct Case
{
    long	key;
    RangeHdr	*targRh;
}   Case;


static void
OpStackHeightError(void)
{
    sysAssert(0);
    RaiseException(OPSTACK_HEIGHT_EXCEPTION, EXCEPTION_NONCONTINUABLE, 0, NULL);
}


static void
ByteCodeError(void)
{
    sysAssert(0);
    RaiseException(BAD_BYTECODE_EXCEPTION, EXCEPTION_NONCONTINUABLE, 0, NULL);
}


static void
InternalError(void)
{
    sysAssert(0);
    RaiseException(INTERNAL_EXCEPTION, EXCEPTION_NONCONTINUABLE, 0, NULL);
}


static	void
FlushVarCache(Context *cx)
{
    ModReg  mr;
    int     *var;

    for (mr = MR_ESP, var = &cx->cachedVar[MR_EAX];
	    --mr >= MR_EAX; var++)
	*var = NOT_A_VAR;
}


static bool_t
AllocContext(CompEnv *ce, Context **ctxt, int top)
{
    struct methodblock *mb = ce->mb;
    Context *cx;
    Item    *opnd;

    check((unsigned)top <= mb->maxstack);

    *ctxt = cx = (*p_malloc)(sizeof(Context) +
		    (mb->maxstack - 1 + 2)*sizeof(Item));
    // +2 to avoid block overrun in Body in case of a bytecode error
    // that may be detected too late (after an opcode has pushed 2 opnds).

    if (cx)
    {
	if  (mb->nlocals)
	{
	    cx->varCachingReg = (*p_calloc)(mb->nlocals, sizeof(ModReg));
	    cx->varhCachingReg = (*p_calloc)(mb->nlocals, sizeof(ModReg));
        }
	else
	{
	    cx->varCachingReg = NULL;
	    cx->varhCachingReg = NULL;
        }
	FlushVarCache(cx);
	cx->espLevel = 0;
	cx->regRefCnt[MR_EAX] = 0;
	cx->regRefCnt[MR_EDX] = 0;
	cx->regRefCnt[MR_ECX] = 0;
	cx->roundRobin = MR_ECX;
	cx->top = cx->base + top;
	for (opnd = cx->base; --top >= 0; opnd++)
	{
	    opnd->mr = MR_BASE + MR_ESP;
	    opnd->offset = OpndOffset(ce, opnd);
	    opnd->size = 4;
	    opnd->var = NOT_A_VAR;
	}
    }

    if (!cx || (mb->nlocals > 0 &&
	    (!cx->varCachingReg || !cx->varhCachingReg)))
    {
	ce->err = "Not enough memory";
	return FALSE;
    }

    return TRUE;
}


static bool_t
CopyContext(CompEnv *ce, RangeHdr *rh, int *clone)
{
    struct methodblock *mb;
    Context *dst_cx, *src_cx;
    long    cx_size, cache_size;

    if (*clone == 0)
    {
	rh->ctxt = ce->ctxt;
	*clone = 1;
    }
    else
    {
	mb = ce->mb;
	src_cx = ce->ctxt;
	cx_size = sizeof(Context) + (mb->maxstack - 1)*sizeof(Item);
	rh->ctxt = dst_cx = (*p_malloc)(cx_size);
	if  (dst_cx)
	{
	    memcpy(dst_cx, src_cx, cx_size);
	    cache_size = sizeof(ModReg)*mb->nlocals;
	    if (cache_size)
	    {
		dst_cx->varCachingReg = (*p_malloc)(cache_size);
		memcpy(dst_cx->varCachingReg, src_cx->varCachingReg, cache_size);
		dst_cx->varhCachingReg = (*p_malloc)(cache_size);
		memcpy(dst_cx->varhCachingReg, src_cx->varhCachingReg, cache_size);
	    }
	    dst_cx->top = dst_cx->base + (src_cx->top - src_cx->base);
	}
	if  (!dst_cx ||	(mb->nlocals > 0 &&
		(!dst_cx->varCachingReg || !dst_cx->varhCachingReg)))
	{
	    ce->err = "Not enough memory";
	    return FALSE;
	}
    }
    return TRUE;
}


static bool_t
InitializeCompEnv(CompEnv *ce, struct methodblock *mb, ExecEnv *ee)
{
    long blockSize;

    memset(ce, 0, sizeof(*ce));
    ce->mb = mb;
    ce->ee = ee;

    if  (mb->nlocals < mb->args_size)
	RaiseException(BAD_BYTECODE_EXCEPTION, EXCEPTION_NONCONTINUABLE, 0, NULL);

    ce->rp = (*p_calloc)(mb->code_length + 1, sizeof(RangePtr));
	/* +1: sentinel for marking end of exception ranges and epilogue */

    ce->xctxt = -1;
    blockSize = mb->code_length * codeSizeFactor + 64 + PC_RED_ZONE;
    ce->compiledCode = (*p_malloc)(blockSize);
    ce->codeInfo = (*p_malloc)(sizeof(CodeInfo));
    ce->pc = ce->compiledCode;
    ce->pcRedZone = ce->compiledCode + blockSize - PC_RED_ZONE;
    ce->epilogueRh = (*p_calloc)(1, sizeof(RangeHdr));

    if (mb->nlocals > 0)
    {
	ce->varOff = (*p_calloc)(mb->nlocals, sizeof(long));
	// calloc: init to 0 for MarkBranchTargAndVars
        ce->varFreq = (*p_calloc)(mb->nlocals, sizeof(long));
	// calloc: init to 0 for CountVarFreq
    }
    if (    !ce->rp
	||  !ce->compiledCode
	||  !ce->codeInfo
	||  !ce->epilogueRh
	||  ((!ce->varOff || !ce->varFreq) && mb->nlocals > 0)
	)
    {
	ce->err = "Not enough memory";
	return FALSE;
    }
    // ce->ctxt allocated in Prologue
    ce->epilogueRh->bcpc = mb->code_length;
    ce->epilogueRh->xctxt = -1; // epilogue does not execute in a catch frame
    // ce->rp[mb->code_length].hdr = ce->epilogueRh;
    // not yet: union! and not needed anyway
    return TRUE;
}


static void
FreeContext(Context *ctxt)
{
    if (ctxt->varCachingReg)
	(*p_free)(ctxt->varCachingReg);
    if (ctxt->varhCachingReg)
	(*p_free)(ctxt->varhCachingReg);

    (*p_free)(ctxt);
}


static void
FinalizeCompEnv(CompEnv *ce)
{
    RangeHdr *rh, *nextRh;

    // free all temporary allocated storage
    if (ce->rp)
    {
	rh = ce->rp[0].hdr;
	while (rh)
	{
	    nextRh = rh->nextRh;
	    if (rh->ctxt)
	    {
		FreeContext(rh->ctxt);
	    }
	    (*p_free)(rh);
	    rh = nextRh;
	}
	(*p_free)(ce->rp);
    }
    if (ce->epilogueRh)
	(*p_free)(ce->epilogueRh);
    if (ce->varOff)
	(*p_free)(ce->varOff);
    if (ce->varFreq)
	(*p_free)(ce->varFreq);
    if (ce->cases)
	(*p_free)(ce->cases);
    if (ce->ctxt)
	FreeContext(ce->ctxt);
}


/* Returns the register variable being assigned to at hintpc if any.
 * Returns the register of the low part only if the variable is long.
 * We ignore the wide form of store (not enough regs for 256 variables).
 */
static RegSet
GetHint(CompEnv *ce, unsigned char *hintpc)
{
    int     var;
    long    off;

    switch  (*hintpc)
    {
    case opc_istore:
    case opc_astore:
    case opc_lstore:
	var = hintpc[1]; break;

    case opc_istore_0:
    case opc_astore_0:
    case opc_lstore_0:
	var = 0; break;

    case opc_istore_1:
    case opc_astore_1:
    case opc_lstore_1:
	var = 1; break;

    case opc_istore_2:
    case opc_astore_2:
    case opc_lstore_2:
	var = 2; break;

    case opc_istore_3:
    case opc_astore_3:
    case opc_lstore_3:
	var = 3; break;

    default:
	return RS_CALLER_SAVED;
    }

    off = ce->varOff[var];
    if (off < 0)
	return regSetOfMr[-off];

    return RS_CALLER_SAVED;
}


/* Returns the register variable being assigned to at hintpc if any.
 * Returns the register of the high part only.
 * We ignore the wide form of store (not enough regs for 256 variables).
 */
static RegSet
GetHintHigh(CompEnv *ce, unsigned char *hintpc)
{
    int     var;
    long    off;

    switch  (*hintpc)
    {
    case opc_lstore:
	var = hintpc[1] + 1; break;

    case opc_lstore_0:
	var = 1; break;

    case opc_lstore_1:
	var = 2; break;

    case opc_lstore_2:
	var = 3; break;

    case opc_lstore_3:
	var = 4; break;

    default:
	return RS_CALLER_SAVED;
    }

    off = ce->varOff[var];
    if (off < 0)
	return regSetOfMr[-off];

    return RS_CALLER_SAVED;
}


static void
FreeRegs(CompEnv *ce, ModReg mr)
{
    int reg;

    if (mr < MR_ABS)
    {
    	reg = BaseRegOf(mr);
	if (reg <= MR_ECX)
	{
	    ce->ctxt->regRefCnt[reg]--;
	    sysAssert(ce->ctxt->regRefCnt[reg] >= 0);
	}
	if (IsBasedIndexed(mr))
	{
    	    reg = ((mr - MR_BINX) >> 3) & 7;
	    if (reg <= MR_ECX)
	    {
		ce->ctxt->regRefCnt[reg]--;
		sysAssert(ce->ctxt->regRefCnt[reg] >= 0);
	    }
	}
    }
}


static void
IncRefCnt(CompEnv *ce, ModReg mr)
{
    int reg;

    if (mr < MR_ABS)
    {
    	reg = BaseRegOf(mr);
	if (reg <= MR_ECX)
	    ce->ctxt->regRefCnt[reg]++;
	if (IsBasedIndexed(mr))
	{
    	    reg = ((mr - MR_BINX) >> 3) & 7;
	    if (reg <= MR_ECX)
		ce->ctxt->regRefCnt[reg]++;
	}
    }
}


/* Return the set of registers that would be free after a call to FreeRegs(ce, mr),
 * but do not do it.
 */
static RegSet
FreeRegSet(CompEnv *ce, ModReg mr)
{
    int reg;
    int *rr = ce->ctxt->regRefCnt;
    int regRefCnt[MR_ECX+1];
    RegSet rs = RS_EMPTY;

    regRefCnt[MR_EAX] = rr[MR_EAX];
    regRefCnt[MR_EDX] = rr[MR_EDX];
    regRefCnt[MR_ECX] = rr[MR_ECX];
    if (mr < MR_ABS)
    {
    	reg = BaseRegOf(mr);
	if (reg <= MR_ECX)
	{
	    regRefCnt[reg]--;
	    sysAssert(regRefCnt[reg] >= 0);
	}
	if (IsBasedIndexed(mr))
	{
    	    reg = ((mr - MR_BINX) >> 3) & 7;
	    if (reg <= MR_ECX)
	    {
		regRefCnt[reg]--;
		sysAssert(regRefCnt[reg] >= 0);
	    }
	}
    }
    if (regRefCnt[MR_EAX] == 0)
    	rs = RS_EAX;
    if (regRefCnt[MR_EDX] == 0)
    	rs |= RS_EDX;
    if (regRefCnt[MR_ECX] == 0)
    	rs |= RS_ECX;
    return rs;
}


static void
LoadOpnd(CompEnv *ce, Item *n, RegSet targ);


/* Spill the operand n to the operand stack in order to free the registers
 * it is using.
 * n must point into the current operand stack, n->var is therefore valid and
 * NOT_A_VAR, because the item of a local cannot use a caller-saved register,
 * so this function should not be called to spill it.
 * Be careful to preserve both condition codes (FPU and integer)
 */
static void
SpillOpnd(CompEnv *ce, Item *n)
{
    Item    mem;
    ModReg  mr;
    int	    *rr;

    sysAssert(n >= ce->ctxt->base && n < ce->ctxt->top
	      && n->var == NOT_A_VAR && n->mr != MR_FST);

    mem.mr = MR_BASE + MR_ESP;
    mem.offset = OpndOffset(ce, n);
    mem.size = 4;
    FreeRegs(ce, n->mr);
    if (IsIntReg(n->mr))
        GenStore(ce, &mem, n->mr);
    else
    {
	for (mr = MR_EAX, rr = &ce->ctxt->regRefCnt[MR_EAX]; mr <= MR_ECX; mr++, rr++)
	    if (*rr == 0)
		break;

	if (mr > MR_ECX)
	{
	    // all regs are in use
	    // we must push the opnd and pop it to its position on the opnd stack
	    if (mem.offset == 0 && ce->ctxt->espLevel == 0)
		// special case: spare a pop
		GenAddImmRL(ce, MR_ESP, 4);

	    if (IsIntReg(n->mr))
		GenByte(ce, PUSHR_L + regMap[n->mr]);
	    else
	    {
		GenByte(ce, 0xFF);
		GenEA(ce, PUSH_L, n);
	    }
	    if (mem.offset != 0 || ce->ctxt->espLevel != 0)
	    {
		GenByte(ce, 0x8F);
		GenEA(ce, POP_L, &mem); // esp += 4; [esp+off] = [esp-4];
	    }
	}
	else
	{
            ce->ctxt->cachedVar[mr] = NOT_A_VAR;	// invalidate cache
	    GenLoad(ce, mr, n);
	    GenStore(ce, &mem, mr);
	}
    }
    n->mr = MR_BASE + MR_ESP;
    n->offset = mem.offset;
}


/* This function allocates a caller-saved register only.
 * If targ is a callee-saved register, we return.
 * The returned register must be an element of the targ set parameter.
 * If all regs in targ are in use, the operand on the opstack using
 * a reg of the targ set is spilled.
 */
static	ModReg
GetReg(CompEnv *ce, RegSet targ)
{
    Context *cx = ce->ctxt;
    Item    *opnd;
    int     i;
    ModReg  rRb;
    RegSet  rs;

    if ((targ & RS_CALLEE_SAVED) != RS_EMPTY)
    {
	sysAssert(bitCnt[targ] == 1);
	return lowestBit[targ] + MR_EAX;
    }

    rRb = cx->roundRobin;
    while (1)
    {
	if (rRb == MR_ECX && cx->regRefCnt[MR_EAX] == 0 && (targ & RS_EAX) != RS_EMPTY)
	{
	    cx->regRefCnt[MR_EAX] = 1;
	    cx->cachedVar[MR_EAX] = NOT_A_VAR;	// invalidate this caching register
	    // caching regs can be used once only, this simplifies the cache
	    // invalidating mechanism which is centralized here, in inlined
	    // GetReg in LoadOpnd, and in SpillOpnd.
	    // LookupVarCache may decide to revalidate the register.
            cx->roundRobin = MR_EAX;
	    return MR_EAX;
	}
	if (rRb != MR_EDX && cx->regRefCnt[MR_EDX] == 0 && (targ & RS_EDX) != RS_EMPTY)
	{
	    cx->regRefCnt[MR_EDX] = 1;
	    cx->cachedVar[MR_EDX] = NOT_A_VAR;
            cx->roundRobin = MR_EDX;
	    return MR_EDX;
	}
	if (cx->regRefCnt[MR_ECX] == 0 && (targ & RS_ECX) != RS_EMPTY)
	{
	    cx->regRefCnt[MR_ECX] = 1;
	    cx->cachedVar[MR_ECX] = NOT_A_VAR;
            cx->roundRobin = MR_ECX;
	    return MR_ECX;
	}
	if (cx->regRefCnt[MR_EAX] == 0 && (targ & RS_EAX) != RS_EMPTY)
	{
	    cx->regRefCnt[MR_EAX] = 1;
	    cx->cachedVar[MR_EAX] = NOT_A_VAR;
            cx->roundRobin = MR_EAX;
	    return MR_EAX;
	}
	if (cx->regRefCnt[MR_EDX] == 0 && (targ & RS_EDX) != RS_EMPTY)
	{
	    cx->regRefCnt[MR_EDX] = 1;
	    cx->cachedVar[MR_EDX] = NOT_A_VAR;
            cx->roundRobin = MR_EDX;
	    return MR_EDX;
	}
	// All regs of targ are in use
	// Spill an opnd that uses a reg of targ.
	// Other opnd may use the same reg, repeat spilling if necessary.
	for (opnd = cx->base, i = cx->top - opnd; --i >= 0; opnd++)
	{
    	    rs = regSetOfMr[opnd->mr];
	    if (rs & targ)
		break;
	}
	if (i < 0)
	    InternalError(); // a register was not released

	if (bitCnt[rs] > 1 && FreeRegSet(ce, opnd->mr))
	// opnd is using several registers and can be loaded without spill.
	// Loading it will free at least one register. This may avoid a spill.
    	    LoadOpnd(ce, opnd, RS_CALLER_SAVED);
	else
	    SpillOpnd(ce, opnd);
    }
}


static	void
CacheVar(CompEnv *ce, int var, ModReg mr)
{
    if (IsCallerSaved(mr))
    {
    	sysAssert(var >= 0 && var < ce->mb->nlocals);
	ce->ctxt->varCachingReg[var] = mr;
	ce->ctxt->cachedVar[mr] = var;
    }
}


static	void
CacheVarH(CompEnv *ce, int var, ModReg mr)
{
    if (IsCallerSaved(mr))
    {
    	sysAssert(var >= 0 && var < ce->mb->nlocals);
	ce->ctxt->varhCachingReg[var] = mr;
	ce->ctxt->cachedVar[mr] = var + VAR_HANDLE;
    }
}


/* Check whether n is a variable currently cached by a register.
 * If it is the case and if the register is free, allocate it and change n->mr
 * to this register. There is no need to free n, because n is a local.
 */
static	int
LookupVarCache(CompEnv *ce, Item *n, int var, RegSet targ)
{
    ModReg  mr;
    Context *cx = ce->ctxt;
    Item    dup;

    sysAssert(IsIntReg(n->mr) && n->mr == -ce->varOff[var]
	      || n->mr == MR_BASE + MR_ESP && n->offset == ce->varOff[var]);
    mr = cx->varCachingReg[var];
    if  (cx->cachedVar[mr] == var)
    {
	sysAssert(IsCallerSaved(mr) && var >= 0 && var < ce->mb->nlocals);
	if ((targ & RS_CALLEE_SAVED) == RS_EMPTY)
	{
    	    // mr may be modified
	    if (cx->regRefCnt[mr] == 0)
	    {
                // GetReg(ce, targOfMr[mr]);
		cx->regRefCnt[mr]++;
		cx->cachedVar[mr] = NOT_A_VAR;
            }
            else
	    {
    		dup.mr = mr;
		mr = GetReg(ce, targ);
		// GetReg may have trashed the caching register by spilling the only operand
		// using it. Check that the cache is still valid. If not, give up.
		if (cx->cachedVar[dup.mr] != var)
		{
    		    FreeRegs(ce, mr);
    		    return 0;
		}
		GenLoad(ce, mr, &dup);
            }
        }
	else
            // GetReg(ce, targOfMr[mr]); with no invalidate
	    cx->regRefCnt[mr]++;	
	n->mr = mr;
	return 1;
    }
    return 0;
}


/* Check whether n is a variable whose handle is currently cached by a reg.
 * If it is the case and if the register is free, allocate it and change n->mr
 * to this register. There is no need to free n, because n is a local.
 */
static	int
LookupVarHCache(CompEnv *ce, Item *n, int var)
{
    ModReg  mr;
    Context *cx = ce->ctxt;

    sysAssert(IsIntReg(n->mr) && n->mr == -ce->varOff[var]
	      || n->mr == MR_BASE + MR_ESP && n->offset == ce->varOff[var]);
    mr = cx->varhCachingReg[var];
    if  (cx->cachedVar[mr] == (signed)(var | VAR_HANDLE))
    {
	sysAssert(IsCallerSaved(mr) && var >= 0 && var < ce->mb->nlocals);
	// GetReg(ce, targOfMr[mr]); with no invalidate: a handle is read-only
	cx->regRefCnt[mr]++;	
	n->mr = mr;
	return 1;
    }
    return 0;
}


static void
Link(CompEnv *ce, Label *link)
{
    char *next;

    next = *link;
    *link = ce->pc;
    *((char **)ce->pc)++ = next;
}


static void
FixLink(CompEnv *ce, Label link)
{
    char *next;
    long dist;

    // eliminate the last JMP rel32 if rel32 is 0
    if (link)
    {
	dist = ce->pc - link - 4;
	if  (dist == 0 && *((unsigned char *)link - 1) == JMPN)
	{
	    ce->pc -= 5;
	    link = *(char **)link;
	}
    }
    while (link)
    {
	next = *(char **)link;
	*(char **)link = (char *)(ce->pc - link - 4);
	link = next;
    }
}


static void
ShortLink(CompEnv *ce, Label *link)
{
    char *next;
    long dist;

    next = *link;
    if (next)
	dist = next - ce->pc;
    else
	dist = 0;

    *link = ce->pc;
    sysAssert(ByteRange(dist));
    *((char *)ce->pc)++ = (char)dist;
}


static void
FixShortLink(CompEnv *ce, Label link)
{
    long dist, rel8;

    while (link)
    {
	dist = *(signed char *)link;
	rel8 = ce->pc - link - 1;
	sysAssert(ByteRange(rel8));
	*link = (char)rel8;
	link = dist ? link + dist : 0;
    }
}


/* Store operand n into opstack memory if not yet flushed. Be careful to
 * preserve the integer condition code since flushing may happen between a
 * compare and a branch! (The fpu condition code may be modified).
 */
static void
FlushOpnd(CompEnv *ce, Item *n)
{
    ModReg  mr;
    long    off;

    sysAssert(n >= ce->ctxt->base && n < ce->ctxt->top);
    off = OpndOffset(ce, n);
    if (n->mr == MR_BASE + MR_ESP && n->offset == off)
	return;

    mr = n->mr;
    if (mr == MR_FST)
    {
	if  (n->size == 0)
	    return; // the high part of a double is flushed with its low part
	n->mr = MR_BASE + MR_ESP;
	n->offset = off;
	GenFStore(ce, n, 0);
	if  (n->size == 8)
	{
	    (n + 1)->mr = MR_BASE + MR_ESP;
	    (n + 1)->offset = off + 4;
	    (n + 1)->size = 4;
	}
	n->size = 4;	// now in memory, both parts have size 4
#ifdef	DEBUG
	// we must flush top-dowm, because of operands on the FPU stack
	{
	    Item *opnd, *top = ce->ctxt->top;
	    for (opnd = n + 1; opnd < top; opnd++)
		sysAssert(opnd->mr != MR_FST);
	}
#endif
    }
    else
    {
	if  (mr == MR_IMM)
	{
	    mr = GetReg(ce, RS_CALLER_SAVED);
	    GenByte(ce, LOADI_L + regMap[mr]);	// XOR by GenLoad modifies cc!
	    GenLong(ce, n->offset);
	}
	else if (!IsIntReg(mr))
	{
    	    LoadOpnd(ce, n, RS_CALLER_SAVED);
	    mr = n->mr;
	}
	n->mr = MR_BASE + MR_ESP;
	n->offset = off;
	n->var = NOT_A_VAR; // item is loaded on the operand stack now
	GenStore(ce, n, mr);
	FreeRegs(ce, mr);
    }
}


/* Flush all operands below n on the opstack to opstack memory. This function is
 * used to merge different contexts at a join point.
 */
static void
FlushOpStack(CompEnv *ce, Item *n)
{
    Item *base = ce->ctxt->base;

    while (--n >= base)   // top-down because of MR_FST operands!
	FlushOpnd(ce, n);
}


static void
PinRVarAliases(CompEnv *ce, RegSet var, Item *n);


static void
MakeFOpndAccessible(CompEnv *ce, Item *n, int size)
{
    sysAssert(n >= ce->ctxt->base && n < ce->ctxt->top);

    if (size == 8)
    {
	if  (!IsQuadAccessibleOpnd(n))
	{
	    // may happen for opc_l2d (n, n+1 in regs)
	    FlushOpnd(ce, n);
	    FlushOpnd(ce, n + 1);
	    sysAssert(IsQuadAccessibleOpnd(n));
	}
    }
    else if (IsIntReg(n->mr) || n->mr == MR_IMM)
	FlushOpnd(ce, n);

}


/* Load an operand from the opstack into a register of targ. Modify its item.
 * Allocate a cache register if cache hit.
 * Make a copy of n if n is a register variable not in targ or
 * if n is a caller-saved referenced more than once
 */
static void
LoadOpnd(CompEnv *ce, Item *n, RegSet targ)
{
    ModReg mr, rRb;
    Item *opnd;
    int i;
    Context *cx = ce->ctxt;
    // locals for inline code below
    int reg, var;
    int *rr = cx->regRefCnt;
    int regRefCnt[MR_ECX+1];
    RegSet rs, t;

    sysAssert(n >= cx->base && n < cx->top);

    mr = n->mr;
    sysAssert((var = n->var) == NOT_A_VAR
	      || IsIntReg(mr) && (mr == -ce->varOff[var] || mr == cx->varCachingReg[var])
	      || mr == MR_BASE + MR_ESP && n->offset == ce->varOff[var]);

    if (IsCallerSaved(mr) && rr[mr] > 1 && (targ & RS_CALLEE_SAVED) == RS_EMPTY)
    	// we need a copy of multi-referenced n, because a caller-saved
	// reg was requested, i.e. a reg that will be modified.
	// if targ is RS_ALL, we can safely use the same reg.
	targ &= ~regSetOfMr[mr];

    if (MrFitsTarget(mr, targ))
    {
    	// n is already in a reg
        if ((targ & RS_CALLER_SAVED) == RS_EMPTY)
	{
	    sysAssert(bitCnt[targ] == 1);
	    // we want to load n into its own register var (e.g. LAddOp)
	    // we need to make a copy of this reg var if it is already loaded
	    // on the operand stack below n before using it
	    PinRVarAliases(ce, targ, n);
	}
	else if ((targ & RS_CALLEE_SAVED) == RS_EMPTY)
            cx->cachedVar[mr] = NOT_A_VAR;	// n is not read-only, invalidate cache

	n->var = NOT_A_VAR; // item is loaded on the operand stack now
	return;
    }

    var = n->var;
    if (var != NOT_A_VAR)
	LookupVarCache(ce, n, var, targ);
    mr = n->mr;
    if (!MrFitsTarget(mr, targ))
    {
	if ((targ & RS_CALLER_SAVED) == RS_EMPTY)
	{
	    sysAssert(bitCnt[targ] == 1);   // we want to load n into a register var
	    mr = lowestBit[targ] + MR_EAX;
	    // we need to make a copy of this reg var if it is already loaded
	    // on the operand stack below n before overwriting it with n
	    PinRVarAliases(ce, targ, n);
	    FreeRegs(ce, n->mr);
	}
	else
	{
	    t = targ & RS_CALLER_SAVED;
	    rRb = cx->roundRobin;
	    // make sure not to free regs too early;
	    // the following situation could occur:
	    // we want to load [eax+ecx] into ecx, an other opnd also uses [ecx]
	    // we cannot free eax and ecx, and get ecx, because eax would be
	    // trashed for spilling [ecx]
	    while (1)
	    {

/* inline and optimize the following 3 calls for efficiency:
		if (FreeRegSet(ce, mr) & t)
 		{
 		    FreeRegs(ce, mr);
 		    mr = GetReg(ce, t); // no spill
 		    break;
 		}
*/

/* start inline */
		regRefCnt[MR_EAX] = rr[MR_EAX];
		regRefCnt[MR_EDX] = rr[MR_EDX];
		regRefCnt[MR_ECX] = rr[MR_ECX];
		if (mr < MR_ABS)
		{
		    reg = BaseRegOf(mr);
		    if (reg <= MR_ECX)
		    {
			regRefCnt[reg]--;
			sysAssert(regRefCnt[reg] >= 0);
		    }
		    if (IsBasedIndexed(mr))
		    {
			reg = ((mr - MR_BINX) >> 3) & 7;
			if (reg <= MR_ECX)
			{
			    regRefCnt[reg]--;
			    sysAssert(regRefCnt[reg] >= 0);
			}
		    }
		}
		if (rRb == MR_ECX && regRefCnt[MR_EAX] == 0 && (t & RS_EAX) != RS_EMPTY)
		{
    		    cx->regRefCnt[MR_EAX] = 1;
                    cx->regRefCnt[MR_EDX] = regRefCnt[MR_EDX];
                    cx->regRefCnt[MR_ECX] = regRefCnt[MR_ECX];
		    cx->cachedVar[MR_EAX] = NOT_A_VAR;
		    mr = MR_EAX;
		    break;
		}
		if (rRb != MR_EDX && regRefCnt[MR_EDX] == 0 && (t & RS_EDX) != RS_EMPTY)
		{
    		    cx->regRefCnt[MR_EAX] = regRefCnt[MR_EAX];
                    cx->regRefCnt[MR_EDX] = 1;
                    cx->regRefCnt[MR_ECX] = regRefCnt[MR_ECX];
		    cx->cachedVar[MR_EDX] = NOT_A_VAR;
		    mr = MR_EDX;
		    break;
		}
		if (regRefCnt[MR_ECX] == 0 && (t & RS_ECX) != RS_EMPTY)
                {
    		    cx->regRefCnt[MR_EAX] = regRefCnt[MR_EAX];
                    cx->regRefCnt[MR_EDX] = regRefCnt[MR_EDX];
                    cx->regRefCnt[MR_ECX] = 1;
		    cx->cachedVar[MR_ECX] = NOT_A_VAR;
		    mr = MR_ECX;
		    break;
		}
		if (regRefCnt[MR_EAX] == 0 && (t & RS_EAX) != RS_EMPTY)
		{
    		    cx->regRefCnt[MR_EAX] = 1;
                    cx->regRefCnt[MR_EDX] = regRefCnt[MR_EDX];
                    cx->regRefCnt[MR_ECX] = regRefCnt[MR_ECX];
		    cx->cachedVar[MR_EAX] = NOT_A_VAR;
		    mr = MR_EAX;
		    break;
		}
		if (regRefCnt[MR_EDX] == 0 && (t & RS_EDX) != RS_EMPTY)
		{
    		    cx->regRefCnt[MR_EAX] = regRefCnt[MR_EAX];
                    cx->regRefCnt[MR_EDX] = 1;
                    cx->regRefCnt[MR_ECX] = regRefCnt[MR_ECX];
		    cx->cachedVar[MR_EDX] = NOT_A_VAR;
		    mr = MR_EDX;
		    break;
		}
/* end inline */
		for (opnd = cx->base, i = cx->top - opnd; --i >= 0; opnd++)
		{
    		    rs = regSetOfMr[opnd->mr];
		    if (rs & t)
			break;
		}

		if (i < 0)
		    InternalError(); // a register was not released

		if (bitCnt[rs] > 1 && FreeRegSet(ce, opnd->mr))
		// opnd is using several registers and can be loaded without spill.
		// Loading it will free at least one register. This may avoid a spill.
		    LoadOpnd(ce, opnd, RS_CALLER_SAVED);
		else
		    SpillOpnd(ce, opnd);
		mr = n->mr; // in case n was spilled
            }
	    cx->roundRobin = mr;
	}
	GenLoad(ce, mr, n);
	n->mr = mr;
	// if mr is holding a read-only variable, validate the cache
	if (n->var != NOT_A_VAR && targ == RS_ALL)
	    CacheVar(ce, n->var, mr);
    }
    sysAssert((targ & RS_CALLEE_SAVED) != RS_EMPTY || cx->cachedVar[mr] == NOT_A_VAR);
    n->var = NOT_A_VAR; // item is loaded on the operand stack now
}


/* Load a floating point operand from the opstack on top of the floating point
 * stack. Modify its item.
 * A loaded double opnd has its size changed from <4, 4> to <8, 0>.
 * See FlushOpnd.
 */
static void
LoadFOpnd(CompEnv *ce, Item *n, int size)
{
    sysAssert(n >= ce->ctxt->base && n < ce->ctxt->top);

    if (n->mr != MR_FST)
    {
	MakeFOpndAccessible(ce, n, size);
	FreeRegs(ce, n->mr);
	if  (size == 8)
	    FreeRegs(ce, (n + 1)->mr);
	n->size = size;
	GenFLoad(ce, n);
	n->mr = MR_FST;
	if  (size == 8)
	{
	    (n + 1)->mr = MR_FST;
	    (n + 1)->size = 0;
	}
    }
    else
	sysAssert(n->size == size);
}


static	void
Push(CompEnv *ce, Item *n)
{
    if (IsIntReg(n->mr))
	GenByte(ce, PUSHR_L + regMap[n->mr]);
    else if (n->mr == MR_IMM)
    {
	if  (ByteRange(n->offset))
	    Gen2Bytes(ce, PUSHI_B, n->offset);
	else
	    GenByteLong(ce, PUSHI_L, n->offset);
    }
    else
    {
	GenByte(ce, 0xFF);
	GenEA(ce, PUSH_L, n);
    }
    ce->ctxt->espLevel -= 4;
    FreeRegs(ce, n->mr);
}


/* Push an operand from the opstack on top of the procedure stack for parameter
 * passing. Modify its item, which should not be used any more.
 */
static void
PushOpnd(CompEnv *ce, Item *n)
{
    int var;

    sysAssert(n >= ce->ctxt->base && n < ce->ctxt->top);

    if (!IsIntReg(n->mr) && (var = n->var) != NOT_A_VAR)
	LookupVarCache(ce, n, var, RS_ALL);
    Push(ce, n);
    // avoid spilling released operand
    n->mr = MR_IMM;
}


/* Push a floating point operand from the opstack on top of the procedure stack
 * for parameter passing. Convert between float and double as indicated by
  * opndSize and paramSize. Modify its item, which should not be used any more.
 */
static void
PushFOpnd(CompEnv *ce, Item *n, int opndSize, int paramSize)
{
    sysAssert(n >= ce->ctxt->base && n < ce->ctxt->top);

    if (opndSize != paramSize && n->mr != MR_FST)
	LoadFOpnd(ce, n, opndSize);

    if (n->mr == MR_FST)
    {
	GenAddImmRL(ce, MR_ESP, -paramSize);
	ce->ctxt->espLevel -= paramSize;
	GenIndirEA(ce, paramSize == 4 ? 0xD9 : 0xDD, 0x18, MR_ESP, 0);
	n->mr = MR_IMM; // avoid spilling released operand
	if  (opndSize == 8)
	    (n + 1)->mr = MR_IMM;
    }
    else
    {
	if  (paramSize == 8)
	    PushOpnd(ce, n + 1);
	PushOpnd(ce, n);
    }
}


/* Make sure all operands below n on the opstack do not use caller-saved regs
 * or the floating point stack, and that they don't reference non-local memory.
 * This function is called before a function call.
 */
static void
PinTempMemAliases(CompEnv *ce, Item *n)
{
    Item *base = ce->ctxt->base;

    while (--n >= base)   // in this order because of MR_FST operands!
    {
	sysAssert(n->mr != MR_CC);
	if  ((regSetOfMr[n->mr] & RS_CALLER_SAVED) != RS_EMPTY || n->mr == MR_FST
	     || IsMem(n->mr) && n->mr != MR_BASE + MR_ESP)
	    FlushOpnd(ce, n);
    }
}


/* The register variable var is to be assigned. If register var is used by
 * operands loaded on the stack below n, we need to make a copy of these
 * operands before the assignment to var.
 */
static void
PinRVarAliases(CompEnv *ce, RegSet var, Item *n)
{
    Item    *base = ce->ctxt->base;

    while (--n >= base)
	if  (regSetOfMr[n->mr] & var)
	    LoadOpnd(ce, n, RS_CALLER_SAVED);	// make a copy
}


/* The variable var in memory is to be assigned. If var is aliased by operands
 * on the stack below n, we need to make a copy of these operands before
 * the assignment to var.
 */
static void
PinMVarAliases(CompEnv *ce, int var, Item *n)
{
    Item    *base = ce->ctxt->base;
    long    off = ce->varOff[var];

    sysAssert(off >= 0);
    while (--n >= base)
	if  (n->mr == MR_BASE + MR_ESP && n->offset == off)
	    FlushOpnd(ce, n);
	    // LoadOpnd does not work here:
	    //	1) type unknown (integer, float, or double)
	    //	2) FST would be out of order
}


/* Some assignment is to be done in non-local memory. Make sure that no operand
 * below n on the operand stack references non-local memory. Flush if necessary.
 */
static void
PinMemAliases(CompEnv *ce, Item *n)
{
    Item *base = ce->ctxt->base;

    while (--n >= base)
	if  (IsMem(n->mr) && n->mr != MR_BASE + MR_ESP)
	    FlushOpnd(ce, n);
}


/* Prepare operand stack for dup and swap opcodes. Do not allow more than one
 * operand on FST above and included the from parameter, flushing top-down
 */
static void
FlushMultipleFST(CompEnv *ce, Item *from)
{
    Item *topFST, *n;

    topFST = 0;
    for (n = ce->ctxt->top; --n >= from; )
    {
	if  (n->mr == MR_FST)
	{
	    if (topFST)
	    {
		FlushOpnd(ce, topFST);
		if  (topFST->size == 0)     // high part
		{
		    sysAssert(n + 1 == topFST && n->size == 8);
		    FlushOpnd(ce, n);
		    topFST = 0;
		    continue;
		}
	    }
	    topFST = n;
	}
    }
}


/* Move 1 (or 2) stack operand from src (and src + 1) to dst (and dst + 1).
 * Operands may overlap.
 */
static void
MoveOpnd(CompEnv *ce, Item *dst, Item *src, int size, RegSet hint)
{
    RegSet  targ;
    int     inc;
    Item    *s, *d, *end;

    if (size == 4 || dst < src)
    {	// move low part first
	inc = 1;
	s = src;
	d = dst;
	end = src + size/4;
    }
    else
    {	// move up 2 operands, move high part first
	inc = -1;
	s = src + 1;
	d = dst + 1;
	end = src - 1;
    }
    for ( ; s != end; s += inc, d += inc)
    {
	if  (s == src)
	// set targ to hint, hint is only for lower part
	    targ = hint;
	else
	    targ = RS_CALLER_SAVED;

	*d = *s;
	if  (s->mr == MR_BASE + MR_ESP
	     &&	s->offset == OpndOffset(ce, s))
	    // s is flushed, we have to really move it
	    LoadOpnd(ce, d, targ);
	    // d can stay in a reg even if it is a floating point,
	    // it will be flushed before being used
    }
}


/* Copy 1 or 2 stack operands from src to dst. If src is on FST, a copy is
 * spilled into memory. The topmost operand of either src or dst becomes then
 * FST, the other one is in memory. Operands may not overlap.
 */
static void
CopyOpnd(CompEnv *ce, Item *dst, Item *src, int size, RegSet hint)
{
    Item *mem, *end;

    end = src + size/4;
    for ( ; src != end; src++, dst++, hint = RS_CALLER_SAVED)
    {
	// reset hint to default, was only for lower part
	dst->size = src->size;
	dst->var = src->var;
	if  (src->mr == MR_FST)
	{
	    if (src < dst)
	    {
		mem = src;
		dst->mr = MR_FST;
	    }
	    else
		mem = dst;

	    mem->mr = MR_BASE + MR_ESP;
	    mem->offset = OpndOffset(ce, mem);
	    // flush the operand to bottom-most position of src and dst
	    // without popping it from FST
	    if (src->size > 0)
	    {
		check(src->size <= size);
		GenFStore(ce, mem, 1);	// no pop
	    }
	    else
		check(src == end - 1 && size == 8);

	    mem->size = 4;  // was 0, 4, or 8
	}
	else if (src->mr == MR_IMM)
	{
	    dst->mr = MR_IMM;
	    dst->offset = src->offset;
	}
	else
	{
	    sysAssert(src->mr < MR_IMM);
    	    if (!IsIntReg(src->mr) && size == 4)
	    {
    		// we load the operand in a reg to spare a second load
		// worse for single float in memory, should not be frequent
		LoadOpnd(ce, src, RS_CALLER_SAVED);
                dst->var = NOT_A_VAR;
            }
	    if (src->mr == MR_BASE + MR_ESP
		    && src->offset == OpndOffset(ce, src))
	    {
		// we can make a copy in a reg even if src is floating point,
		// it will be flushed before being used
		dst->mr = GetReg(ce, hint);
		GenLoad(ce, dst->mr, src);
		sysAssert(dst->var == NOT_A_VAR);
	    }
	    else
	    {
    		// src is not physically on the opstack, so we may
		// duplicate the item
		*dst = *src;
                IncRefCnt(ce, dst->mr);
	    }
	}
    }
}


/* Swap x and y on the operand stack.
 * x is just below y, which is top of stack; size is 4.
 */
static void
SwapOpnd(CompEnv *ce, Item *x, Item *y, RegSet hint)
{
    Item    h;

    FlushMultipleFST(ce, x);
    if (x->mr == MR_FST)
    {
	check(x->size > 0);
	if  (x->size == 8)
	    FlushOpnd(ce, x);	// swap high and low part of a double (?)
    }
    check(y->mr != MR_FST || y->size == 4);

    if (y->mr == MR_BASE + MR_ESP
	&&  y->offset == OpndOffset(ce, y)) // y is flushed
	LoadOpnd(ce, y, RS_CALLER_SAVED);   // this can flush x

    if (x->mr == MR_BASE + MR_ESP
	&&  x->offset == OpndOffset(ce, x)) // x is flushed
	LoadOpnd(ce, x, hint);	// this cannot flush y (== top)

    // now that the following 2 conditions hold, we can swap the items:
    //	- at most one of the operands is on FST and of size 4
    //	- none of the operands is flushed
    h = *y;
    *y = *x;
    *x = h;

    // an operand that was in memory or on FST can stay in a register,
    // because it will be flushed before being used as a float
}


static void
SymBinOp(CompEnv *ce, Item *x, Item *y, int op, int opimm, RegSet hint)
{
    // may be called from LSymBinOp with hint != GetHint(hintpc)
    Item    *l, *r;

    if (x->mr == MR_IMM && y->mr == MR_IMM)
    {
	switch	(opimm)
	{
	case ADDI:
	    x->offset += y->offset; break;
	case ANDI:
	    x->offset &= y->offset; break;
	case ORI:
	    x->offset |= y->offset; break;
	case XORI:
	    x->offset ^= y->offset; break;
	}
    }
    else
    {
	if  (x->mr == MR_IMM || MrFitsTarget(y->mr, hint))
	{
	    l = y;
	    r = x;
	}
	else
	{
	    l = x;
	    r = y;
	}
	if  ((regSetOfMr[r->mr] & hint & RS_CALLEE_SAVED) != RS_EMPTY)
	    // l cannot be loaded into callee-saved hint, because r is using it
	    hint = RS_CALLER_SAVED;

	LoadOpnd(ce, l, hint);
	FreeRegs(ce, r->mr);	// this cannot free caller-saved regs of l
	if  (r->mr == MR_IMM)
	    if (opimm == ADDI)
		GenAddImmRL(ce, l->mr, r->offset);
	    else
		GenOpImmR(ce, opimm, l->mr, r->offset, 4);
	else
	    GenOpSizRegEA(ce, op, l->mr, r);

	x->mr = l->mr;
    }
    x->var = NOT_A_VAR;
}


static void
SubOp(CompEnv *ce, Item *x, Item *y, unsigned char *hintpc)
{
    RegSet  hint = GetHint(ce, hintpc);

    if (x->mr == MR_IMM && y->mr == MR_IMM)
	x->offset -= y->offset;
    else
    {
	if  ((regSetOfMr[y->mr] & hint & RS_CALLEE_SAVED) != RS_EMPTY)
	    // x cannot be loaded into callee-saved hint, because y is using it
	    hint = RS_CALLER_SAVED;

	LoadOpnd(ce, x, hint);
	FreeRegs(ce, y->mr);	// this cannot free caller-saved regs of x
	if  (y->mr == MR_IMM)
	    GenOpImmR(ce, SUBI, x->mr, y->offset, 4);
	else
	    GenOpSizRegEA(ce, SUBR, x->mr, y);
    }
    x->var = NOT_A_VAR;
}


static void
MulOp(CompEnv *ce, Item *x, Item *y, unsigned char *hintpc)
{
    RegSet  hint = GetHint(ce, hintpc);
    Item    *l, *r;
    ModReg  mr;

    if (x->mr == MR_IMM && y->mr == MR_IMM)
	x->offset *= y->offset;
    else
    {
	if  (x->mr == MR_IMM || MrFitsTarget(y->mr, hint))
	{
	    l = y;
	    r = x;
	}
	else
	{
	    l = x;
	    r = y;
	}
	if  (r->mr == MR_IMM)
	{
	    if (r->offset > 0 && IsPower(r->offset))
	    {
		LoadOpnd(ce, l, hint);
		mr = l->mr;
		ShiftLeft(ce, mr, PowerOf(r->offset));
	    }
	    else
	    {
		// LoadOpnd not necessary with IMUL r32,r/m32,imm
		if  ((hint & RS_CALLEE_SAVED) != RS_EMPTY)
		{
		    PinRVarAliases(ce, hint, x);
		    FreeRegs(ce, l->mr);
		    mr = GetReg(ce, hint);	// no spill
                }
		else if (FreeRegSet(ce, l->mr) & hint)
		{
		    FreeRegs(ce, l->mr);
		    mr = GetReg(ce, hint);	// no spill
		}
		else
                {
    		    // the spill could destroy the freed l->mr
		    mr = GetReg(ce, hint);	// spill
                    FreeRegs(ce, l->mr);
                }
		if  (ByteRange(r->offset))
		{
		    GenOpSizRegEA(ce, IMULIB-1, mr, l);
		    GenByte(ce, r->offset);
		}
		else
		{
		    GenOpSizRegEA(ce, IMULI-1, mr, l);
		    GenLong(ce, r->offset);
		}
	    }
	}
	else
	{
	    if ((regSetOfMr[r->mr] & hint & RS_CALLEE_SAVED) != RS_EMPTY)
		// l cannot be loaded into callee-saved hint, r is using it
		hint = RS_CALLER_SAVED;

	    LoadOpnd(ce, l, hint);
	    mr = l->mr;
	    FreeRegs(ce, r->mr);    // this cannot free caller-saved regs of l
	    Gen2Bytes(ce, 0x0F, IMUL);
	    GenEA(ce, reg3Map[mr], r);
	}
	x->mr = mr;
    }
    x->var = NOT_A_VAR;
}


static void
DivRemOp(CompEnv *ce, Item *x, Item *y, unsigned char *hintpc, int div)
{
    RegSet  hint = GetHint(ce, hintpc);
    int     power;
    Label   skipLab;

    if (y->mr == MR_IMM && IsPower(y->offset) && y->offset != 0)
    {
	LoadOpnd(ce, x, hint);
	power = PowerOf(y->offset);
	if  (div)
	{
	    if (power == 1)
	    {
		ShiftRight(ce, x->mr, power, 4, 1);
		skipLab = 0;
		GenByte(ce, 0x70 + CC_NS);
		ShortLink(ce, &skipLab);
		GenOpImmR(ce, ADCI, x->mr, 0, 4);
		FixShortLink(ce, skipLab);  // keep same context
	    }
	    else
	    {
		GenOpSizRegEA(ce, TESTM, x->mr, x);
		skipLab = 0;
		GenByte(ce, 0x70 + CC_NS);
		ShortLink(ce, &skipLab);
		GenAddImmRL(ce, x->mr, y->offset-1);
		FixShortLink(ce, skipLab);  // keep same context
		ShiftRight(ce, x->mr, power, 4, power != 4*8-1);
	    }
	}
	else
	{
	    /*	Now, things get tricky:
		If the sign bit is off, we want to clear
		the bits to left, if it's on, we want to
		fill up with one bits. So we do this:

		and	n,%100..11
		jns	@@skip
		dec	n
		or	n,%011..00
		inc	n
	      @@skip:
	    */
	    unsigned long mask;

	    mask = (1UL << power) - 1;
	    mask |= 1UL << (4*8-1);
	    GenOpImm(ce, ANDI, x, mask);
	    skipLab = 0;
	    GenByte(ce, 0x70 + CC_NS);
	    ShortLink(ce, &skipLab);
	    GenByte(ce, DECR_L + regMap[x->mr]);
	    GenOpImm(ce, ORI, x, ~((1UL << power) - 1));
	    GenByte(ce, INCR_L + regMap[x->mr]);
	    FixShortLink(ce, skipLab);	// keep same context
	}
    }
    else
    {
	LoadOpnd(ce, x, RS_EAX);
	if  (y->mr == MR_IMM || (regSetOfMr[y->mr] & RS_EDX) != RS_EMPTY)
	    LoadOpnd(ce, y, RS_CALLER_SAVED - RS_EDX);

	GetReg(ce, RS_EDX);	// spill edx if necessary
	GenByte(ce, CDQ);
	GenOpSizEA(ce, 0xF6, IDIV, y);
	FreeRegs(ce, y->mr);
	if  (div)
	    FreeRegs(ce, MR_EDX);
	else
	{
	    FreeRegs(ce, MR_EAX);
	    x->mr = MR_EDX;
	}
    }
    x->var = NOT_A_VAR;
}


/* Increment variable var by val.
 * Watch for aliases of var on operand stack.
 */
static void
IncOp(CompEnv *ce, int var, long val)
{
    Item    n;
    ModReg  mr;
    long    off;

    off = ce->varOff[var];
    if (off < 0)
    {
	mr = -off;
	PinRVarAliases(ce, regSetOfMr[mr], ce->ctxt->top);
	GenAddImmRL(ce, mr, val);
    }
    else
    {
	n.mr = MR_BASE + MR_ESP;
	n.offset = off;
	n.size = 4;
	if  (val == -1 || val == +1)
	{
	    PinMVarAliases(ce, var, ce->ctxt->top);
	    GenOpSizEA(ce, 0xFE, val > 0 ? INC2 : DEC2, &n);
	    // memory var may be cached by a caller-saved reg,
	    // invalidate the caching reg:
	    mr = ce->ctxt->varCachingReg[var];
	    ce->ctxt->cachedVar[mr] = NOT_A_VAR;
	}
	else
	{
	    mr = GetReg(ce, RS_CALLER_SAVED);
	    GenLoad(ce, mr, &n);
	    PinMVarAliases(ce, var, ce->ctxt->top);
	    GenAddImmRL(ce, mr, val);
	    GenStore(ce, &n, mr);
	    FreeRegs(ce, mr);
	    CacheVar(ce, var, mr);
	}
    }
}


/* Add long <y, y + 1> to long <x, x + 1>
 */
static void
LAddOp(CompEnv *ce, Item *x, Item *y, unsigned char *hintpc)
{
    Item    *l, *r;
    RegSet  hint;
    ModReg  mr;
    int     carry, CFvalid;

    carry = 0;
    CFvalid = 0;
    // add low parts
    if (x->mr == MR_IMM && y->mr == MR_IMM)
    {
	carry = (unsigned long)x->offset > (unsigned long)(-1 - y->offset);
	x->offset += y->offset;
    }
    else
    {
	hint = GetHint(ce, hintpc);
	if  (x->mr == MR_IMM || MrFitsTarget(y->mr, hint))
	{
	    l = y;
	    r = x;
	}
	else
	{
	    l = x;
	    r = y;
	}
	if  (((regSetOfMr[r->mr] |
		regSetOfMr[(x + 1)->mr] | regSetOfMr[(y + 1)->mr])
	    & hint & RS_CALLEE_SAVED) != RS_EMPTY)
	    // l cannot be loaded into the callee-saved hint, because r or the
	    // higher operand is using it
	    hint = RS_CALLER_SAVED;

	LoadOpnd(ce, l, hint);
	FreeRegs(ce, r->mr);	// this cannot free caller-saved regs of l
	if  (r->mr == MR_IMM)
	    GenOpImmR(ce, ADDI, l->mr, r->offset, 4);
	else
	    GenOpSizRegEA(ce, ADDR, l->mr, r);

	CFvalid = 1;
	x->mr = l->mr;
    }
    x->var = NOT_A_VAR;
    y->mr = MR_IMM; // avoid spilling released operand

    // add high parts
    x++;
    y++;
    hint = GetHintHigh(ce, hintpc);
    if (x->mr == MR_IMM && y->mr == MR_IMM)
    {
	if  (CFvalid)
	{
	    mr = GetReg(ce, hint);
	    GenByte(ce, LOADI_L + regMap[mr]);	// XOR by GenLoad modifies cc!
	    GenLong(ce, 0);
	    GenOpImmR(ce, ADCI, mr, y->offset + x->offset, 4);
	    x->mr = mr;
	}
	else
	    x->offset += y->offset + carry;
    }
    else
    {
	if  (x->mr == MR_IMM || MrFitsTarget(y->mr, hint))
	{
	    l = y;
	    r = x;
	}
	else
	{
	    l = x;
	    r = y;
	}
	if  ((regSetOfMr[r->mr] & hint & RS_CALLEE_SAVED) != RS_EMPTY)
	    // l cannot be loaded into callee-saved hint, because r is using it
	    hint = RS_CALLER_SAVED;

	// If x - 1 is using the hint, it will be moved automatically
	sysAssert(l->mr != MR_IMM); // otherwise, CF could be modified
	LoadOpnd(ce, l, hint);
	FreeRegs(ce, r->mr);	// this cannot free caller-saved regs of l
	if  (r->mr == MR_IMM)
	    GenOpImmR(ce, CFvalid ? ADCI : ADDI, l->mr, r->offset + carry, 4);
	else
	{
	    GenOpSizRegEA(ce, CFvalid ? ADCR : ADDR, l->mr, r);
	    if (carry)
		GenByte(ce, INCR_L + regMap[l->mr]);
	}
	x->mr = l->mr;
    }
    x->var = NOT_A_VAR;
}


/* Subtract long <y, y + 1> from long <x, x + 1>
 */
static void
LSubOp(CompEnv *ce, Item *x, Item *y, unsigned char *hintpc)
{
    RegSet  hint;
    ModReg  mr;
    int     carry, CFvalid;

    carry = 0;
    CFvalid = 0;
    // subtract low parts
    if (x->mr == MR_IMM && y->mr == MR_IMM)
    {
	carry = (unsigned long)x->offset < (unsigned long)y->offset;
	x->offset -= y->offset;
    }
    else
    {
	hint = GetHint(ce, hintpc);
	if  (((regSetOfMr[y->mr] |
		regSetOfMr[(x + 1)->mr] | regSetOfMr[(y + 1)->mr])
	    & hint & RS_CALLEE_SAVED) != RS_EMPTY)
	    // x cannot be loaded into the callee-saved hint, because y or the
	    // higher operand is using it
	    hint = RS_CALLER_SAVED;

	LoadOpnd(ce, x, hint);
	FreeRegs(ce, y->mr);	// this cannot free caller-saved regs of x
	if  (y->mr == MR_IMM)
	    GenOpImmR(ce, SUBI, x->mr, y->offset, 4);
	else
	    GenOpSizRegEA(ce, SUBR, x->mr, y);

	CFvalid = 1;
    }
    x->var = NOT_A_VAR;
    y->mr = MR_IMM; // avoid spilling released operand

    // subtract high parts
    x++;
    y++;
    hint = GetHintHigh(ce, hintpc);
    if (x->mr == MR_IMM && y->mr == MR_IMM)
    {
	if  (CFvalid)
	{
	    mr = GetReg(ce, hint);
	    GenByte(ce, LOADI_L + regMap[mr]);	// XOR by GenLoad modifies cc!
	    GenLong(ce, 0);
	    GenOpImmR(ce, SBBI, mr, y->offset - x->offset, 4);
	    x->mr = mr;
	}
	else
	    x->offset -= y->offset + carry;
    }
    else
    {
	if  ((regSetOfMr[y->mr] & hint & RS_CALLEE_SAVED) != RS_EMPTY)
	    // x cannot be loaded into callee-saved hint, because y is using it
	    hint = RS_CALLER_SAVED;

	// If x - 1 is using the hint, it will be moved automatically
	if  (CFvalid && x->mr == MR_IMM)
	{
	    mr = GetReg(ce, hint);
	    GenByte(ce, LOADI_L + regMap[mr]);
	    GenLong(ce, x->offset);
	    x->mr = mr;
	    // otherwise, CF could be modified by LoadOpnd
	}
	else
	    LoadOpnd(ce, x, hint);

	FreeRegs(ce, y->mr);	// this cannot free caller-saved regs of x
	if  (y->mr == MR_IMM)
	    GenOpImmR(ce, CFvalid ? SBBI : SUBI, x->mr, y->offset + carry, 4);
	else
	{
	    GenOpSizRegEA(ce, CFvalid ? SBBR : SUBR, x->mr, y);
	    if (carry)
		GenByte(ce, DECR_L + regMap[x->mr]);
	}
    }
    x->var = NOT_A_VAR;
}


static void
LNegOp(CompEnv *ce, Item *x, unsigned char *hintpc)
{
    RegSet  hint;
    int     carry, CFvalid;
    ModReg  mr;

    carry = 0;
    CFvalid = 0;
    // negate low part
    if (x->mr == MR_IMM)
    {
	carry = x->offset != 0;
	x->offset = -x->offset;
    }
    else
    {
	hint = GetHint(ce, hintpc);
	if  ((regSetOfMr[(x + 1)->mr] & hint & RS_CALLEE_SAVED) != RS_EMPTY)
	    // x cannot be loaded into the callee-saved hint, because the
	    // higher operand is using it
	    hint = RS_CALLER_SAVED;

	LoadOpnd(ce, x, hint);
	GenOpSizEA(ce, 0xF6, NEG, x);
	CFvalid = 1;
    }
    x->var = NOT_A_VAR;

    // negate high part
    x++;
    hint = GetHintHigh(ce, hintpc);
    if (CFvalid)
    {
	if  ((regSetOfMr[x->mr] & hint & RS_CALLEE_SAVED) != RS_EMPTY)
	    // zero cannot be loaded into the callee-saved hint, because x is
	    // using it
	    hint = RS_CALLER_SAVED;

	// If x - 1 is using the hint, it will be moved automatically
	mr = GetReg(ce, hint);
	GenByte(ce, LOADI_L + regMap[mr]);  // XOR modifies cc!
	GenLong(ce, 0);
	if  (x->mr == MR_IMM)
	    GenOpImmR(ce, SBBI, mr, x->offset, 4);
	else
	{
	    GenOpSizRegEA(ce, SBBR, mr, x);
	    FreeRegs(ce, x->mr);
	}
	x->mr = mr;
    }
    else
    {
	if  (x->mr == MR_IMM)
	    x->offset = -(x->offset + carry);
	else
	{
	    LoadOpnd(ce, x, hint);
	    GenOpSizEA(ce, 0xF6, NEG, x);
	    if (carry)
		GenByte(ce, DECR_L + regMap[x->mr]);
	}
    }
    x->var = NOT_A_VAR;
}


static void
LSymBinOp(CompEnv *ce, Item *x, Item *y, int op, int opimm,
    unsigned char *hintpc)
{
    RegSet hint;

    hint = GetHint(ce, hintpc);
    if ((  (regSetOfMr[(x + 1)->mr] | regSetOfMr[(y + 1)->mr])
	 &  hint & RS_CALLEE_SAVED) != RS_EMPTY)
	// hint is useless, because the higher operand uses the hint register
	hint = RS_CALLER_SAVED;

    SymBinOp(ce, x, y, op, opimm, hint);
    y->mr = MR_IMM; // avoid spilling released operand
    hint = GetHintHigh(ce, hintpc);
    SymBinOp(ce, x + 1, y + 1, op, opimm, hint);
}


static void
ShiftOp(CompEnv *ce, Item *x, Item *y, int left, int signedFlag,
    unsigned char *hintpc)
{
    RegSet  hint = GetHint(ce, hintpc);
    long    cnt;
    char    byte2;

    if (y->mr == MR_IMM)
    {
	cnt = y->offset & 0x1F;
	LoadOpnd(ce, x, hint);
	if  (left)
	    ShiftLeft(ce, x->mr, cnt);
	else
	    ShiftRight(ce, x->mr, cnt, 4, signedFlag);
    }
    else
    {
	if  ((regSetOfMr[x->mr] & RS_ECX) == RS_EMPTY)
	{
	    LoadOpnd(ce, y, RS_ECX);
	    LoadOpnd(ce, x, hint);
	}
	else
	{
	    hint &= ~RS_ECX;
	    if ((regSetOfMr[y->mr] & hint & RS_CALLEE_SAVED) != RS_EMPTY)
		hint = RS_CALLER_SAVED - RS_ECX;

	    LoadOpnd(ce, x, hint);
	    LoadOpnd(ce, y, RS_ECX);
	}
	byte2 = sibModRmTab[x->mr];
	if  (left)
	    byte2 += SHL;
	else if (signedFlag)
	    byte2 += SAR;
	else
	    byte2 += SHR;
	Gen2Bytes(ce, 0xD3, byte2);
	FreeRegs(ce, MR_ECX);
    }
    x->var = NOT_A_VAR;
}


static void
RealOp(CompEnv *ce, Item *x, Item *y, int op, int opr, int notSym, int size)
{
    int swap;

    if (x->mr == MR_FST)
    {
	sysAssert(x->size == size);
	MakeFOpndAccessible(ce, y, size);
	y->size = size;
	GenFOpEA(ce, op, opr, y);
	FreeRegs(ce, y->mr);
	x->var = NOT_A_VAR;
	if  (size == 8)
        {
            FreeRegs(ce, (y + 1)->mr);
	    (x + 1)->var = NOT_A_VAR;
	}
    }
    else
    {
	LoadFOpnd(ce, y, size);
	MakeFOpndAccessible(ce, x, size);
	x->size = size;
	swap = notSym*(FSUBR - FSUB);
	GenFOpEA(ce, op + swap, opr - swap, x);
	FreeRegs(ce, x->mr);
	x->mr = MR_FST;
	x->var = NOT_A_VAR;
	if (size == 8)
	{
            FreeRegs(ce, (x + 1)->mr);
	    (x + 1)->mr = MR_FST;
	    (x + 1)->size = 0;
	    (x + 1)->var = NOT_A_VAR;
	}
    }
}


static void
CallCompSupport(CompEnv *ce, char *funcAddr, Item *firstPar, long parSize)
{
    PinTempMemAliases(ce, firstPar);
    GenByteLong(ce, CALLN, funcAddr - ce->pc - 5);
    ce->ctxt->espLevel += parSize;
    FlushVarCache(ce->ctxt);
    sysAssert(ce->ctxt->regRefCnt[MR_EAX] == 0
	      && ce->ctxt->regRefCnt[MR_EDX] == 0
	      && ce->ctxt->regRefCnt[MR_ECX] == 0);
}


static void
PushImm(CompEnv *ce, long val)
{
    if (ByteRange(val))
	Gen2Bytes(ce, PUSHI_B, val);
    else
	GenByteLong(ce, PUSHI_L, val);
    ce->ctxt->espLevel -= 4;
}


/* Store a 32-bit dword from the operand stack to a variable
 */
static void
StoreOpnd(CompEnv *ce, int var, Item *n)
{
    long    off;
    Item    v;
    ModReg  mr;
    RegSet  targ;

    off = ce->varOff[var];
    if (off < 0)   // var is a callee-saved reg
    {
	targ = regSetOfMr[-off];
	// PinRVarAliases(ce, targ, n); called in LoadOpnd
	LoadOpnd(ce, n, targ);
    }
    else if (n->mr != MR_BASE + MR_ESP || n->offset != off)
    {
	v.mr = MR_BASE + MR_ESP;
	v.offset = off;
	v.size = 4;
	LoadOpnd(ce, n, RS_ALL);
	mr = n->mr;
	PinMVarAliases(ce, var, n);
	GenStore(ce, &v, mr);
	FreeRegs(ce, mr);
	CacheVar(ce, var, mr);
    }
}


/* Store a float (or a double) from the operand stack to one (or two) variables
 */
static void
StoreFOpnd(CompEnv *ce, int var, Item *n, int size)
{
    long    off;
    Item    v;

    // calling MakeFOpndAccessible is not optimal here: we recode it inline
    // using StoreOpnd instead of FlushOpnd and we are done
    if (size == 8 && (IsIntReg(n->mr) || IsIntReg((n + 1)->mr)))
    {
	sysAssert(n->size == 4 && (n + 1)->size == 4);
	StoreOpnd(ce, var, n);
	StoreOpnd(ce, var + 1, n + 1);
    }
    else if (size == 4 && IsIntReg(n->mr))
    {
	sysAssert(n->size == 4);
	StoreOpnd(ce, var, n);
    }
    else
    {
	off = ce->varOff[var];
	sysAssert(off >= 0);
	if  (n->mr != MR_BASE + MR_ESP || n->offset != off)
	{
	    // we use the fpu
	    PinMVarAliases(ce, var, n);
	    if (size == 8)
	    {
		PinMVarAliases(ce, var + 1, n);
		if  (!IsQuadAccessibleOpnd(n))
		{
		    // when ??
		    FlushOpnd(ce, n);
		    FlushOpnd(ce, n + 1);
		    sysAssert(IsQuadAccessibleOpnd(n));
		}
	    }
	    v.mr = MR_BASE + MR_ESP;
	    v.offset = off;
	    v.size = size;
	    if (n->mr != MR_FST)
	    {
		n->size = size; // may change from 4 to 8
		FreeRegs(ce, n->mr);
		if  (size == 8)
		    FreeRegs(ce, (n + 1)->mr);
		GenFLoad(ce, n);
	    }
	    GenFStore(ce, &v, 0);
	}
    }
}


static void
Int2BW(CompEnv *ce, Item *n, int size, int op, unsigned char *hintpc)
{
    RegSet  hint = GetHint(ce, hintpc);
    ModReg  mr;

    mr = n->mr;
    if (mr == MR_IMM)
    	if (size == 1)
	    n->offset = (signed char)n->offset;		// opc_int2byte
	else if (op == 0xB7)
            n->offset = (unsigned short)n->offset;	// opc_int2char
	else
            n->offset = (signed short)n->offset;	// opc_int2short
    else
    {
	if (!IsByteAccessible(mr))
	{
	    LoadOpnd(ce, n, RS_BYTE & RS_CALLER_SAVED);
	    mr = n->mr;
	}
	else    // memory or byte register
	{
	    if  ((hint & RS_CALLEE_SAVED) != RS_EMPTY)
	    {
		PinRVarAliases(ce, hint, n);
		FreeRegs(ce, mr);
		mr = GetReg(ce, hint);	// no spill
	    }
	    else if (FreeRegSet(ce, mr) & hint)
	    {
		FreeRegs(ce, mr);
		mr = GetReg(ce, hint);	// no spill
	    }
	    else
	    {
		// the spill could destroy the freed n->mr
		mr = GetReg(ce, hint);	// spill
		FreeRegs(ce, n->mr);
	    }
        }
	n->size = size;
	Gen2Bytes(ce, 0x0F, op);
	GenEA(ce, reg3Map[mr], n);	// mov(s|z)x erx, byte|word
	n->mr = mr;
    }
    n->size = 4;
    n->var = NOT_A_VAR;
}


static void
Jcc(CompEnv *ce, CondCode cc, RangeHdr *rh, Item *top)
{
    long    dist;
    char    *targetpc;
    Label   *targetlink;


    if (rh->pc)
    {
	// backward jump
	sysAssert(rh->pc < ce->pc);
	FlushOpStack(ce, top);

	if  (ce->xctxt != rh->xctxt)
	{
	    sysAssert(rh->rangeFlags & RF_SET_XCTXT);
	    targetpc = rh->xpc;
	}
	else
	    targetpc = rh->pc;

	dist = targetpc - ce->pc;
	if  (cc == CC_JSR)
	    GenByteLong(ce, CALLN, dist - 5);
	else if (ByteRange(dist - 2))
	{
	    if (cc == CC_JMP)
		GenByte(ce, 0xEB);
	    else
		GenByte(ce, 0x70 + cc);

	    GenByte(ce, dist - 2);
	}
	else
	{
	    if (cc == CC_JMP)
	    {
		GenByte(ce, 0xE9);
		GenLong(ce, dist - 5);
	    }
	    else
	    {
		Gen2Bytes(ce, 0x0F, 0x80 + cc);
		GenLong(ce, dist - 6);
	    }
	}
    }
    else
    {
	// forward jump
	if  (rh->joinCnt > 0)
	    FlushOpStack(ce, top);

	if  (ce->xctxt != rh->xctxt)
	{
	    sysAssert(rh->rangeFlags & RF_SET_XCTXT);
	    targetlink = &rh->xlink;
	}
	else
	    targetlink = &rh->link;

	if  (cc == CC_JSR)
	    GenByte(ce, CALLN);
	else if (cc == CC_JMP)
	    GenByte(ce, 0xE9);
	else
	    Gen2Bytes(ce, 0x0F, 0x80 + cc);

	Link(ce, targetlink);
    }
}


/* special version of Jcc for switch tables:
 * always 5 bytes total, no call to FlushOpStack (done in Body)
 */
static void
SwitchTableJmp(CompEnv *ce, RangeHdr *rh)
{
    char    *targetpc;
    Label   *targetlink;


    if (rh->pc)
    {
	// backward jump
	sysAssert(rh->pc < ce->pc);

	if  (ce->xctxt != rh->xctxt)
	{
	    sysAssert(rh->rangeFlags & RF_SET_XCTXT);
	    targetpc = rh->xpc;
	}
	else
	    targetpc = rh->pc;

	GenByte(ce, 0xE9);
	GenLong(ce, targetpc - ce->pc - 4);
    }
    else
    {
	// forward jump
	if  (ce->xctxt != rh->xctxt)
	{
	    sysAssert(rh->rangeFlags & RF_SET_XCTXT);
	    targetlink = &rh->xlink;
	}
	else
	    targetlink = &rh->link;

	GenByte(ce, 0xE9);
	Link(ce, targetlink);
    }
}


static CondCode
GetCondition(unsigned char *pc)
{
    switch  (*pc)
    {
    case opc_ifeq:
    case opc_ifnull:
	return CC_E;

    case opc_ifne:
    case opc_ifnonnull:
	return CC_NE;

    case opc_iflt:
	return CC_L;

    case opc_ifle:
	return CC_LE;

    case opc_ifgt:
	return CC_G;

    case opc_ifge:
	return CC_GE;

    }
    return CC_NONE;
}


/* Compare x with y, both float or double. y is on top of x on the operand stack.
 * Both x and y may be on FST or in memory, but y cannot be below x on FST.
 */
static void
FCompare(CompEnv *ce, Item *x, Item *y, int size, int NaNres,
    unsigned char *nextpc)
{
    CondCode	cc;
    RangeHdr	*rh;
    Label	loadm1, loadp1, done;

    cc = GetCondition(nextpc);
#ifdef	STRICT_FLOATING
    if (cc == CC_NONE || cc == CC_E || cc == CC_NE)
    {
    	// we must use 64-bit precision to conform to the spec,	i.e. flush the
	// operands to memory, extended precision would not satisfy
	// 1.0e+308d + Double.MAX_VALUE == Double.POSITIVE_INFINITY
	if (y->mr == MR_FST)
	    FlushOpnd(ce, y);
        if (x->mr == MR_FST)
	    FlushOpnd(ce, x);
    }
#endif
    LoadFOpnd(ce, y, size);	// make sure y is on FST(0)
    MakeFOpndAccessible(ce, x, size);
    x->size = size;
    GenFOpEA(ce, FCOMP, FCOMP, x);
    FreeRegs(ce, x->mr);
    if (size == 8)
	FreeRegs(ce, (x + 1)->mr);
    GetReg(ce, RS_EAX);     // must preserve the fpu condition code!
    Gen2Bytes(ce, 0xDF, FNSTSTWAX);
    GenByte(ce, SAHF);
    // the condition code holds the result of y compared to x (unsigned)
    // y <  x:	-   -	CF
    // y >  x:	-   -	-
    // y == x:	ZF  -	-
    // NaN:	ZF  PF	CF
    if (cc != CC_NONE)
    {
	// the next opcode compiles to a Jcc instruction
	FreeRegs(ce, MR_EAX);
	rh = ce->rp[nextpc + pc2signedshort(nextpc) - ce->mb->code].hdr;

	if  (rh->joinCnt > 0)
	    FlushOpStack(ce, x);    // before jump! var cache flushed later

	cc = xsgnCC[swapCC[cc]];    // swapped operands and unsigned test
	x->size = cc;
	x->mr = MR_CC;
	x->offset = 0;	// continuation link

	// if NaN:
	//	JB, JBE, JE, JAE are taken
	//	JNE, JA are not taken
	// if NaNres < 0
	//	JB, JBE, JA do what we want
	//	JE, JAE, JNE need a correction:
	//	    JNE 	-> JPE target
	//	    JE, JAE	-> JPE continue
	// if NaNres > 0
	//	JAE do what we want
	//	JB, JBE, JE, JNE, JA need a correction:
	//	    JNE, JA	    -> JPE target
	//	    JB, JBE, JE     -> JPE continue

	if  (cc == CC_NE
	     || NaNres > 0 && cc == CC_A)
	{
	    Jcc(ce, CC_PE, rh, x);	// jpe target
	}
	else if (cc == CC_E
		 || NaNres < 0 && cc == CC_AE
		 || NaNres > 0 && (cc == CC_B || cc == CC_BE))
	{
	    GenByte(ce, 0x70 + CC_PE);	    // jpe continue
	    ShortLink(ce, &((Label)x->offset));
	}
    }
    else
    {
	// We need to generate -1, 0, or +1.
	loadm1 = 0;
	loadp1 = 0;
	done = 0;
	if  (NaNres < 0)
	{
	    // Emit JB first so that NaN yields -1
	    GenByte(ce, 0x70 + CC_B);
	    ShortLink(ce, &loadm1);
	    GenByte(ce, 0x70 + CC_A);
	    ShortLink(ce, &loadp1);
	}
	else
	{
	    // Emit JA first so that NaN yields +1
	    GenByte(ce, 0x70 + CC_A);
	    ShortLink(ce, &loadp1);
	    GenByte(ce, 0x70 + CC_B);
	    ShortLink(ce, &loadm1);
	}
	GenOpRegReg(ce, XORR_L, MR_EAX, MR_EAX);
	GenByte(ce, 0xEB);  // jmp done
	ShortLink(ce, &done);
	FixShortLink(ce, loadm1);   // keep same context
	GenOpImmR(ce, ORI, MR_EAX, -1, 4);
	GenByte(ce, 0xEB);  // jmp done
	ShortLink(ce, &done);
	FixShortLink(ce, loadp1);   // keep same context
	GenByte(ce, LOADI_L + regMap[MR_EAX]);
	GenLong(ce, 1);
	FixShortLink(ce, done);     // keep same context
	x->mr = MR_EAX;
	x->size = 4;
    }
    x->var = NOT_A_VAR;
}


/* Compare x with y, both long. y is on top of x on the operand stack.
 */
static void
LCompare(CompEnv *ce, Item *x, Item *y, unsigned char *nextpc)
{
    CondCode	cc;
    RangeHdr	*rh;
    Label	cont, loadm1, loadp1, done;
    ModReg	mr;

    LoadOpnd(ce, x + 1, RS_ALL);
    GenCmpRegEA(ce, (x + 1)->mr, y + 1);    // cmp high(x), high(y)
    FreeRegs(ce, (x + 1)->mr);
    (x + 1)->mr = MR_IMM;   // avoid spilling released operand
    FreeRegs(ce, (y + 1)->mr);
    (y + 1)->mr = MR_IMM;   // avoid spilling released operand

    // Do not try to load x before the compare above (register pressure)
    // Emit spill code before jumping and do not destroy condition code:
    mr = x->mr;
    if (mr == MR_IMM)
    {
	mr = GetReg(ce, RS_CALLER_SAVED);
	GenByte(ce, LOADI_L + regMap[mr]);  // XOR by GenLoad modifies cc!
	GenLong(ce, x->offset);
	x->mr = mr;
    }
    else if (!IsIntReg(mr))
    	LoadOpnd(ce, x, RS_CALLER_SAVED);

    x->var = NOT_A_VAR; // item is loaded on the operand stack now
    cc = GetCondition(nextpc);
    if (cc != CC_NONE)
    {
	// the next opcode compiles to a Jcc instruction
	rh = ce->rp[nextpc + pc2signedshort(nextpc) - ce->mb->code].hdr;
	cont = 0;

	if  (rh->joinCnt > 0)
	    FlushOpStack(ce, x);    // before jump!

	if  (cc == CC_E)
	{
	    GenByte(ce, 0x70 + CC_NE);
	    ShortLink(ce, &cont);
	}
	else if (cc == CC_NE)
	    Jcc(ce, CC_NE, rh, x);
	else
	{
	    Jcc(ce, highCC[cc], rh, x);
	    GenByte(ce, 0x70 + revhCC[cc]);
	    ShortLink(ce, &cont);
	}
	// no need to flush var cache because read only and no new caching regs
	sysAssert(IsIntReg(x->mr));
	GenCmpRegEA(ce, x->mr, y);  // cmp low(x), low(y)
	FreeRegs(ce, x->mr);
	FreeRegs(ce, y->mr);
	x->mr = MR_CC;
	x->size = xsgnCC[cc];	// unsigned compare
	x->offset = (long)cont;
    }
    else
    {
	// We need to generate -1, 0, or +1.
	loadm1 = 0;
	loadp1 = 0;
	done = 0;
	mr = GetReg(ce, RS_CALLER_SAVED);   // must preserve the condition code
	// GetReg must be called now to avoid spill code between jumps, which
	// would modify the context
	GenByte(ce, 0x70 + CC_L);
	ShortLink(ce, &loadm1);
	GenByte(ce, 0x70 + CC_G);
	ShortLink(ce, &loadp1);
	// no need to flush var cache because read only and no new caching regs
	sysAssert(IsIntReg(x->mr));
	GenCmpRegEA(ce, x->mr, y);  // cmp low(x), low(y)
	FreeRegs(ce, x->mr);
	FreeRegs(ce, y->mr);
	GenByte(ce, 0x70 + CC_B);
	ShortLink(ce, &loadm1);
	GenByte(ce, 0x70 + CC_A);
	ShortLink(ce, &loadp1);
	GenOpRegReg(ce, XORR_L, mr, mr);
	GenByte(ce, 0xEB);  // jmp done
	ShortLink(ce, &done);
	FixShortLink(ce, loadm1);   // keep same context
	GenOpImmR(ce, ORI, mr, -1, 4);
	GenByte(ce, 0xEB);  // jmp done
	ShortLink(ce, &done);
	FixShortLink(ce, loadp1);   // keep same context
	GenByte(ce, LOADI_L + regMap[mr]);
	GenLong(ce, 1);
	FixShortLink(ce, done);     // keep same context
	x->mr = mr;
	x->size = 4;
    }
    x->var = NOT_A_VAR;
}


static void
Ifcc(CompEnv *ce, Item *n, CondCode cc, unsigned char *pc)
{
    RangeHdr *rh = ce->rp[pc + pc2signedshort(pc) - ce->mb->code].hdr;

    if (n->mr == MR_CC)
    {
	// n->offset is the continuation link, n->size is the corrected cc
	// stack is flushed already
	Jcc(ce, n->size, rh, n);
	FixShortLink(ce, (Label)n->offset); // continue here, keep same context
    }
    else if (n->mr == MR_IMM)
    {
	// bytecode is not optimal, bytecode compiler should do constant folding
	if ((cc == CC_E) == (n->offset == 0))
	    Jcc(ce, CC_JMP, rh, n);
    }
    else
    {
	GenOpImm(ce, CMPI, n, 0);
	FreeRegs(ce, n->mr);
	Jcc(ce, cc, rh, n);
    }
}


static void
IfCmpcc(CompEnv *ce, Item *x, Item *y, CondCode cc, unsigned char *pc)
{
    RangeHdr	*rh = ce->rp[pc + pc2signedshort(pc) - ce->mb->code].hdr;
    Item	*l, *r;

    if (x->mr == MR_IMM || IsIntReg(y->mr))
    {
	l = y;
	r = x;
	cc = swapCC[cc];
    }
    else
    {
	l = x;
	r = y;
    }
    LoadOpnd(ce, l, RS_ALL);
    if (r->mr == MR_IMM)
	GenOpImmR(ce, CMPI, l->mr, r->offset, 4);
    else
    {
	GenOpSizRegEA(ce, CMPR, l->mr, r);
	FreeRegs(ce, r->mr);
    }
    FreeRegs(ce, l->mr);
    Jcc(ce, cc, rh, x);
}


static void
SortCases(Case *start, Case *end)	// sort [start..end[
{
    Case	*a, *b;
    long	key;
    RangeHdr	*rh;

    a = start;
    while (++a < end) // sort keys in increasing order (insertion sort)
    {
	key = a->key;
	rh = a->targRh;
	b = a;
	while	(--b >= start && b->key > key)
	    b[1] = b[0];
	b[1].key = key;
	b[1].targRh = rh;
    }
}


static void
TableJmp(CompEnv *ce, ModReg mr,
		Case *firstCase, Case *lastCase, RangeHdr *defRh)
{
    long    lo, hi;
    Item    table, *top;

    top = ce->ctxt->base;   // stack is flushed already
    lo = firstCase->key;
    hi = lastCase->key;
    if (lo > 0 && lo <= 2)
	lo = 0;
    GenAddImmRL(ce, mr, -lo);
    hi -= lo;
    GenOpImmR(ce, CMPI, mr, hi, 4);
    Jcc(ce, CC_A, defRh, top);
    // lea mr, table[4*mr + mr]
    table.mr = MR_BINX + (mr << 3) + mr;
    table.size = 4;
    table.scale = 2;
    table.offset = (long)ce->pc + 9;
    GenByte(ce, LEA_L);
    GenEA(ce, reg3Map[mr], &table);
    // jmp mr
    Gen2Bytes(ce, 0xFF, JMPI + sibModRmTab[mr]);
    hi = lo;
    for (;;)
    {
	while	(hi < firstCase->key)
	{
	    SwitchTableJmp(ce, defRh);
	    hi++;
	}
	SwitchTableJmp(ce, firstCase->targRh);
	hi++;
	if  (firstCase != lastCase)
	    firstCase++;
	else
	    break;
    }
}


static void
SeqJmp(CompEnv *ce, ModReg mr, long siz,
	    Case *firstCase, Case *lastCase, RangeHdr *defRh)
{
    long    key, prev, sub;
    Item    *top;
    int     ccSet;

    top = ce->ctxt->base;   // stack is flushed already
    prev = 0;
    ccSet = 0;
    for (;;)
    {
	key = firstCase->key;
	sub = key - prev;
	if  (sub == 0)
	{
	    if (ccSet)
		Jcc(ce, CC_E, firstCase->targRh, top);
	    else
	    {
		GenSubImmRSetCC(ce, mr, 1, siz);
		Jcc(ce, CC_B, firstCase->targRh, top);
		prev = key + 1;
		ccSet = 1;
	    }
	}
	else
	{
	    if (sub == 1)
	    {
		if  (siz == 4)
		    GenByte(ce, DECR_L + regMap[mr]);
		else
		    Gen2Bytes(ce, 0xFE + GenSiz(ce, siz), DEC2 + sibModRmTab[mr]);
	    }
	    else
		GenSubImmRSetCC(ce, mr, sub, siz);

	    Jcc(ce, CC_E, firstCase->targRh, top);
	    prev = key;
	    ccSet = 0;
	}
	if  (firstCase != lastCase)
	    firstCase++;
	else
	    break;
    }
    Jcc(ce, CC_JMP, defRh, top);
}


static void
Switch(CompEnv *ce, ModReg mr,
	    Case *firstCase, Case *lastCase, RangeHdr *defRh);

static void
BinJmp(CompEnv *ce, ModReg mr,
	    Case *firstCase, Case *lastCase, RangeHdr *defRh)
{
    Case    *middle;
    Label   upperLab;
    Item    *top;

    top = ce->ctxt->base;   // stack is flushed already
    middle = firstCase + ((lastCase - firstCase) / 2);
    GenOpImmR(ce, CMPI, mr, middle->key, 4);
    upperLab = 0;
    Gen2Bytes(ce, 0x0F, 0x80 + CC_G);
    Link(ce, &upperLab);
    Jcc(ce, CC_E, middle->targRh, top);
    Switch(ce, mr, firstCase, middle - 1, defRh);
    FixLink(ce, upperLab);
    Switch(ce, mr, middle + 1, lastCase, defRh);
}


static void
Switch(CompEnv *ce, ModReg mr,
	    Case *firstCase, Case *lastCase, RangeHdr *defRh)
{
    long    range;
    long    labCnt;

    range = lastCase->key - firstCase->key + 1;
    labCnt = lastCase - firstCase + 1;
    if (labCnt <= 4)
	SeqJmp(ce, mr, 4, firstCase, lastCase, defRh);
    else if (range > 4*labCnt)
	BinJmp(ce, mr, firstCase, lastCase, defRh);
    else
	TableJmp(ce, mr, firstCase, lastCase, defRh);
}


static void
PutField(CompEnv *ce, Item *obj, long offset, Item *val, int size)
{
    Item mem;	// do not use opnd obj as a temp
#ifndef	HANDLE_IN_OBJECT
    int var;
#endif

    sysAssert(obj >= ce->ctxt->base && obj < ce->ctxt->top);
    PinMemAliases(ce, obj);
#ifdef	HANDLE_IN_OBJECT
    LoadOpnd(ce, obj, RS_ALL);
    mem.offset = offset + 8;
#else
    var = obj->var;
    if (var == NOT_A_VAR || !LookupVarHCache(ce, obj, var))
    {
	LoadOpnd(ce, obj, RS_ALL);
	obj->mr += MR_BASE;
	obj->offset = offsetof(JHandle, obj);
	LoadOpnd(ce, obj, RS_ALL);
	if (var != NOT_A_VAR)
	    CacheVarH(ce, var, obj->mr);
    }
    mem.offset = offset;
#endif
    mem.mr = obj->mr + MR_BASE;
    mem.size = 4;
    obj->mr = MR_IMM;	// avoid spilling released operand
    if (val->mr != MR_FST)
    {
	LoadOpnd(ce, val, RS_ALL);
	GenStore(ce, &mem, val->mr);
	FreeRegs(ce, val->mr);
        val->mr = MR_IMM;	// avoid spilling released operand
	if  (size == 8)
	{
	    mem.offset += 4;
	    LoadOpnd(ce, val + 1, RS_ALL);
	    GenStore(ce, &mem, (val + 1)->mr);
	    FreeRegs(ce, (val + 1)->mr);
	}
    }
    else
    {
	sysAssert(val->size == size);
	mem.size = size;
	GenFStore(ce, &mem, 0);
    }
    FreeRegs(ce, mem.mr);
}


static void
GetField(CompEnv *ce, Item *top, long offset, int size, unsigned char *hintpc)
{
    RegSet hint;
#ifndef	HANDLE_IN_OBJECT
    int	   var;
#endif

#ifdef	HANDLE_IN_OBJECT
    if (FreeRegSet(ce, top->mr))
	// use a caller-saved register for better register
	// cache hit rate in following accesses
	hint = RS_ALL;
    else
	// use the hint to avoid a spill
	hint = GetHint(ce, hintpc);

    LoadOpnd(ce, top, hint);
    top->offset = offset + 8;
#else
    var = top->var;
    if (var == NOT_A_VAR  || !LookupVarHCache(ce, top, var))
    {
    	if (FreeRegSet(ce, top->mr))
    	    // use a caller-saved register for better register
	    // cache hit rate in following accesses
            hint = RS_ALL;
        else
    	    // use the hint to avoid a spill
	    hint = GetHint(ce, hintpc);

	LoadOpnd(ce, top, hint);
	top->mr += MR_BASE;
	top->offset = offsetof(JHandle, obj);
	LoadOpnd(ce, top, hint);
	if (var != NOT_A_VAR)
	    CacheVarH(ce, var, top->mr);
    }
    top->offset = offset;
#endif
    top->mr += MR_BASE;
    top->var = NOT_A_VAR;
    if (size == 8)
    {
	(top + 1)->mr = top->mr;
	(top + 1)->offset = offset + 4;
	(top + 1)->size = 4;
	(top + 1)->var = NOT_A_VAR;
        IncRefCnt(ce, top->mr);
	// If the base is a callee-saved register and if the higher part of the
	// qword field is to be assigned to this base (which we don't know,
	// because the hint is for the lower part), the code will not be
	// efficient because PinRVarAliases will flush the lower part before
	// assigning the higher part. This should not happen very often, since
	// a local should not be used as an object and as the higher part of
	// a long at the same time.
	// On the other hand, if the lower part is to be assigned to the base
	// (which can happen more often, because of the hint asking to use
	// this callee-saved as the base, thereby using one less register), we
	// don't have aliasing problems, since the higher part is assigned
	// first to another register or memory.
	// Note that the higher part MUST be assigned first by StoreOpnd for
	// PinRVarAliases to work correctly! (top is assumed to be n)
    }
}


static void
PutStatic(CompEnv *ce, long adr, Item *val, int size)
{
    Item mem;

    sysAssert(val >= ce->ctxt->base && val < ce->ctxt->top);
    PinMemAliases(ce, val);
    mem.mr = MR_ABS;
    mem.offset = adr;
    if (val->mr != MR_FST)
    {
	mem.size = 4;
	LoadOpnd(ce, val, RS_ALL);
	GenStore(ce, &mem, val->mr);
	FreeRegs(ce, val->mr);
	if  (size == 8)
	{
	    mem.offset += 4;
	    LoadOpnd(ce, val + 1, RS_ALL);
	    GenStore(ce, &mem, (val + 1)->mr);
	    FreeRegs(ce, (val + 1)->mr);
	}
    }
    else
    {
	sysAssert(val->size == size);
	mem.size = size;
	GenFStore(ce, &mem, 0);
    }
}


static void
GetStatic(long offset, Item *top, int size)
{
    top->mr = MR_ABS;
    top->offset = offset;
    top->size = 4;
    top->var = NOT_A_VAR;
    if (size == 8)
    {
	(top + 1)->mr = MR_ABS;
	(top + 1)->offset = offset + 4;
	(top + 1)->size = 4;
	(top + 1)->var = NOT_A_VAR;
    }
}


static int
ResultSize(char *sig)
{
    char    *p;

    for (p = sig + 1; *p != SIGNATURE_ENDFUNC; p++);
    if (p[1] == SIGNATURE_LONG || p[1] == SIGNATURE_DOUBLE)
	return 2;
    else if (p[1] == SIGNATURE_VOID)
	return 0;
    else
	return 1;
}


/* Pop the method arguments from the operand stack and push them onto the
 * procedure stack according to the signature sig. Since it is not possible
 * to parse the signature backwards (SIGNATURE_CLASS may be part of a class
 * name), we recursively traverse the signature and pop the arguments in
 * reverse order when coming back from the recursion.
 * Return the new operand stack top and sets *resSig to the result signature.
 * "this" is not included in the signature and therefore not pushed.
 */
static Item *
PopArgs(CompEnv *ce, Item *top, char *sig, char *resSig)
{
    if (*sig == SIGNATURE_ENDFUNC)
	*resSig = sig[1];
    else
    {
	switch	(*sig)
	{
	case SIGNATURE_BOOLEAN:
	case SIGNATURE_BYTE:
	case SIGNATURE_CHAR:
	case SIGNATURE_SHORT:
	case SIGNATURE_INT:
	    top = PopArgs(ce, top, sig + 1, resSig);
	    PushOpnd(ce, --top);
	    break;

	case SIGNATURE_FLOAT:
	    top = PopArgs(ce, top, sig + 1, resSig);
	    PushFOpnd(ce, --top, 4, 4);
	    break;

	case SIGNATURE_CLASS:
	    while (*sig != SIGNATURE_ENDCLASS) sig++;
	    top = PopArgs(ce, top, sig + 1, resSig);
	    PushOpnd(ce, --top);
	    break;

	case SIGNATURE_ARRAY:
	    while (*sig == SIGNATURE_ARRAY) sig++;
	    if (*sig == SIGNATURE_CLASS)
		while	(*sig != SIGNATURE_ENDCLASS) sig++;
	    top = PopArgs(ce, top, sig + 1, resSig);
	    PushOpnd(ce, --top);
	    break;

	case SIGNATURE_LONG:
	    top = PopArgs(ce, top, sig + 1, resSig);
	    PushOpnd(ce, --top);    // high
	    PushOpnd(ce, --top);    // low
	    break;

	case SIGNATURE_DOUBLE:
	    top = PopArgs(ce, top, sig + 1, resSig);
	    top -= 2;
	    PushFOpnd(ce, top, 8, 8);
	    break;

	default:
	    check(0);
	}
    }
    return top;
}


/* Push the method result to the operand stack according to the signature sig.
 * Return the new operand stack top.
 */
static Item *
PushResult(CompEnv *ce, Item *top, char sig)
{
    switch  (sig)
    {
    default:	// 4-byte integer word
	top->mr = GetReg(ce, RS_EAX);
	top->size = 4;
	top->var = NOT_A_VAR;
	top++;
	break;

    case SIGNATURE_LONG:
	(top + 0)->mr = GetReg(ce, RS_EAX);
	(top + 0)->size = 4;
	(top + 0)->var = NOT_A_VAR;
	(top + 1)->mr = GetReg(ce, RS_EDX);
	(top + 1)->size = 4;
	(top + 1)->var = NOT_A_VAR;
	top += 2;
	break;

    case SIGNATURE_FLOAT:
	top->mr = MR_FST;
	top->size = 4;
	top->var = NOT_A_VAR;
	top++;
	break;

    case SIGNATURE_DOUBLE:
	(top + 0)->mr = MR_FST;
	(top + 0)->size = 8;
	(top + 0)->var = NOT_A_VAR;
	(top + 1)->mr = MR_FST;
	(top + 1)->size = 0;
	(top + 1)->var = NOT_A_VAR;
	top += 2;
	break;

    case SIGNATURE_VOID:
	break;
    }
    return top;
}


static int
InvokeMethod(CompEnv *ce, struct methodblock *mb_type, Item *top,
    int isVirtual, int isStatic)
{
    Item    *new_top, mtab, meth, code;
    ModReg  thisR;
    char    *sig = fieldsig(&mb_type->fb);
    char    resSig;

    new_top = PopArgs(ce, top, sig + 1, &resSig);
    if (!isStatic)
    {
	--new_top;
	if  (isVirtual)
	{
	    // If the called method is virtual, "this" is pushed via a reg,
	    // because "this" is also used to access the method table.
	    // Since edx must hold mb, it cannot hold "this".
	    // If we need to load "this" or move it from edx, we use eax
	    if (!IsIntReg(new_top->mr) || new_top->mr == MR_EDX)
		LoadOpnd(ce, new_top, RS_EAX);
	    thisR = new_top->mr;
	    PushOpnd(ce, new_top);	    // new_top->mr changed to MR_IMM
	    GetReg(ce, regSetOfMr[thisR]);  // thisR not available in PinTempMemAliases
	}
	else
	    PushOpnd(ce, new_top);
    }
    sysAssert(top - new_top == mb_type->args_size);
    PinTempMemAliases(ce, new_top);
    if (isVirtual)
    {
	FreeRegs(ce, thisR);
	sysAssert(ce->ctxt->regRefCnt[MR_EAX] == 0
		  && ce->ctxt->regRefCnt[MR_EDX] == 0
		  && ce->ctxt->regRefCnt[MR_ECX] == 0);
	mtab.mr = MR_BASE + thisR;
	mtab.offset = offsetof(Hjava_lang_Object, methods);
	mtab.size = 4;
	GenLoad(ce, MR_EDX, &mtab);
	if  (fieldclass(&mb_type->fb) == classJavaLangObject)
	    GenByteLong(ce, CALLN,
		(char *)CompSupport_invokevirtualobject - ce->pc - 5);
	meth.mr = MR_BASE + MR_EDX;
	meth.offset = offsetof(struct methodtable, methods)
	    + 4*mb_type->fb.u.offset;
	meth.size = 4;
	GenLoad(ce, MR_EDX, &meth);
    }
    else    // nonvirtual or static
    {
	sysAssert(ce->ctxt->regRefCnt[MR_EAX] == 0
		  && ce->ctxt->regRefCnt[MR_EDX] == 0
		  && ce->ctxt->regRefCnt[MR_ECX] == 0);
	GenByte(ce, LOADI_L + regMap[MR_EDX]);
	GenLong(ce, (long)mb_type);
    }
    GenByte(ce, LOADI_L + regMap[MR_ECX]);
    GenLong(ce, (long)ce->mb);	// caller mb
    if (isVirtual)
    {
	code.mr = MR_BASE + MR_EDX;
	code.offset = offsetof(struct methodblock, CompiledCode);
    }
    else    // nonvirtual or static
    {
	code.mr = MR_ABS;
	code.offset = (long)mb_type + offsetof(struct methodblock, CompiledCode);
    }
    code.size = 4;
    GenByte(ce, 0xFF);
    GenEA(ce, CALLI, &code);
    ce->ctxt->espLevel += 4*(top - new_top);
    sysAssert(ce->ctxt->espLevel == 0);		// otherwise we cannot walk the stack
    FlushVarCache(ce->ctxt);
    new_top = PushResult(ce, new_top, resSig);
    return new_top - top;
}


static int
InvokeInterface(CompEnv *ce, unsigned ID, Item *top)
{
    Item    *new_top;
    char    *sig, resSig;

    sig = (*p_IDToType)(ID);
    new_top = PopArgs(ce, top, sig + 1, &resSig);
    PushOpnd(ce, --new_top);
    PinTempMemAliases(ce, new_top);	// must be done before pushing the hint for patching to work
    GenByteLong(ce, PUSHI_L, (long)ce->mb);	// caller mb
    GenByteLong(ce, PUSHI_L, ID);		// ID
    GenByteLong(ce, PUSHI_L, top - new_top);	// nargs
    GenByteLong(ce, PUSHI_L, 0);		// mslot hint, at return address - 9
    CallCompSupport(ce, (char *)CompSupport_invokeinterface, new_top, 4*(top - new_top));
    new_top = PushResult(ce, new_top, resSig);
    return new_top - top;
}


/* Set elem to a[x<<elem->scale], elem->scale must be defined on entry,
 * generate bounds checking
 */
static void
ArrayIndex(CompEnv *ce, Item *a, Item *x, Item *elem)
{
    ModReg  lenR, arrR;
    Item    arrh;
    Label   skipLab;

    if (!IsIntReg(a->mr) && a->var != NOT_A_VAR)
    	LookupVarCache(ce, a, a->var, RS_ALL);  // look for a cache hit before GetReg

    lenR = GetReg(ce, RS_CALLER_SAVED);
    LoadOpnd(ce, a, RS_ALL);
    arrh.mr = MR_BASE + a->mr;
    a->mr = MR_IMM;	// protect register of arrh from being spilled
    arrh.offset = offsetof(HArrayOfObject, methods);
    arrh.size = 4;
    GenLoad(ce, lenR, &arrh);
    ShiftRight(ce, lenR, METHOD_FLAG_BITS, 4, 0);
    if (x->mr != MR_IMM && FreeRegSet(ce, x->mr))
	LoadOpnd(ce, x, RS_ALL); // no spill, otherwise loading x later is fine
    GenCmpRegEA(ce, lenR, x);
    GenByte(ce, 0x70 + CC_A);
    skipLab = 0;
    ShortLink(ce, &skipLab);
    // push the bad index, call	support routine, and don't return
    IncRefCnt(ce, x->mr);
    Push(ce, x);
    GenByteLong(ce, CALLN, (char *)CompSupport_throwArrayIndexOutOfBounds - ce->pc - 5);
    ce->ctxt->espLevel += 4;
    FixShortLink(ce, skipLab);
#ifdef	HANDLE_IN_OBJECT
    FreeRegs(ce, lenR);
    arrR = arrh.mr - MR_BASE;
#else
    FreeRegs(ce, arrh.mr);
    arrR = lenR;
    arrh.offset = offsetof(HArrayOfObject, obj);
    GenLoad(ce, arrR, &arrh);
#endif
    if (x->mr != MR_IMM)
    {
	LoadOpnd(ce, x, RS_ALL);
	elem->mr = MR_BINX + (x->mr << 3) + arrR;
	x->mr = MR_IMM; // protect register of x from being spilled
	elem->offset = offsetof(ArrayOfObject, body);
    }
    else
    {
	elem->mr = MR_BASE + arrR;
	elem->offset = offsetof(ArrayOfObject, body) + (x->offset << elem->scale);
    }
#ifdef	HANDLE_IN_OBJECT
    elem->offset += 8;
#endif
}


static void
ArrayLoad(CompEnv *ce, Item *a, Item *x, int scale, int size, int signedFlag,
    unsigned char *hintpc)
{
    RegSet  hint = GetHint(ce, hintpc);
    ModReg  elemR;
    Item    elem;
    int     op;

    elem.scale = scale;
    ArrayIndex(ce, a, x, &elem);
    if (size == 8)
    {
	sysAssert(scale == 3);
	(a + 1)->mr = elem.mr;
        (a + 1)->offset = elem.offset + 4;
        (a + 1)->scale = scale;
	(a + 1)->size = 4;
	(a + 1)->var = NOT_A_VAR;
	IncRefCnt(ce, elem.mr);
	sysAssert(IsCallerSaved(BaseRegOf(elem.mr)));
    }
    if (scale <= 1)	    // we need to load and extend the element
    {
	if  ((hint & RS_CALLEE_SAVED) != RS_EMPTY)
	{
	    PinRVarAliases(ce, hint, a);
	    FreeRegs(ce, elem.mr);
	    elemR = GetReg(ce, hint);	// no spill
	}
	else if (FreeRegSet(ce, elem.mr) & hint)
	{
	    FreeRegs(ce, elem.mr);
	    elemR = GetReg(ce, hint);	// no spill
	}
	else
	{
	    // the spill could destroy the freed elem.mr
	    // a, x, and elem cannot be spilled:
	    // a and x are MR_IMM, elem is not visible (not on opnd stack)
	    sysAssert(a->mr == MR_IMM && x->mr == MR_IMM);
	    elemR = GetReg(ce, hint);	// spill
	    // always at least one caller-saved reg to be spilled
      	    FreeRegs(ce, elem.mr);
	}
       	if  (scale == 0)
	{
	    elem.size = 1;
	    op = 0xBE;	// movsx erx, byte
	}
	else if (signedFlag)
	{
	    elem.size = 2;
	    op = 0xBF;	// movsx erx, word
	}
	else
	{
	    elem.size = 2;
	    op = 0xB7;	// movzx erx, word
	}
	Gen2Bytes(ce, 0x0F, op);
	GenEA(ce, reg3Map[elemR], &elem);   // mov(s|z)x erx, byte|word
	(a + 0)->mr = elemR;
    }
    else
    {
	(a + 0)->mr = elem.mr;
	(a + 0)->offset = elem.offset;
	(a + 0)->scale = scale;
    }
    (a + 0)->size = 4;
    (a + 0)->var = NOT_A_VAR;
}


static void
ArrayStore(CompEnv *ce, Item *a, Item *x, Item *val, int scale, int size)
{
    ModReg  elemR;
    Item    elem;

    PinMemAliases(ce, a);
    elem.scale = scale;
    ArrayIndex(ce, a, x, &elem);
    if (val->mr == MR_FST)
    {
	check(val->size == size);
	elem.size = size;
	GenFStore(ce, &elem, 0);
    }
    else
    {
	if  (size == 8)
	{
	    sysAssert(scale == 3);
	    elemR = (val + 1)->mr;
	    if (!IsIntReg(elemR))
	    {
    		LoadOpnd(ce, val + 1, RS_CALLER_SAVED);
		elemR = (val + 1)->mr;
	    }
	    elem.size = 4;
	    elem.offset += 4;	// high
	    GenStore(ce, &elem, elemR);
	    FreeRegs(ce, elemR);
	    elem.offset -= 4;	// low again
	}
	elemR = val->mr;
	if  (scale <= 1)	// we need to truncate the element
	{
	    if (!IsByteReg(elemR))
	    {
    		LoadOpnd(ce, val, RS_BYTE & RS_CALLER_SAVED);
		elemR = val->mr;
	    }
	    elem.size = 1 << scale;
	}
	else
	{
	    if (!IsIntReg(elemR))
	    {
    		LoadOpnd(ce, val, RS_CALLER_SAVED);
		elemR = val->mr;
	    }
	    elem.size = 4;
	}
	GenStore(ce, &elem, elemR);
	FreeRegs(ce, elemR);
    }
    FreeRegs(ce, elem.mr);
}


#define SET_RANGE_FLAG(t, k) rp[t].mark.rangeFlags |= k;
#define INC_JOIN_CNT(t) if (rp[t].mark.joinCnt < 127) rp[t].mark.joinCnt++
#define DEC_JOIN_CNT(t) rp[t].mark.joinCnt-- // no guard, called once at most


static bool_t
MarkExceptionRanges(CompEnv *ce)
{
    struct CatchFrame *cf = ce->mb->exception_table;
    signed long cnt = ce->mb->exception_table_length;
    RangePtr *rp = ce->rp;
    unsigned char n, m;

    for (; --cnt >= 0; cf += 1)
    {
	n = rp[cf->start_pc].mark.tryEnterCnt + 1;
	m = rp[cf->end_pc].mark.tryExitCnt + 1;
	rp[cf->start_pc].mark.tryEnterCnt = n;
	rp[cf->end_pc].mark.tryExitCnt = m;
	SET_RANGE_FLAG(cf->handler_pc, RF_XHANDLER | RF_SET_XCTXT);
	// INC_JOIN_CNT(cf->handler_pc); no, to avoid stack flushing
	if  (n == 0 || m == 0)
	{
	    ce->err = "Too many nested try statements";
	    return FALSE;
	}
    }
    return TRUE;
}


/* Find and mark branch targets.
 * Mark variables that cannot be allocated in registers, i.e. float or double.
 */
static bool_t
MarkBranchTargAndVars(CompEnv *ce)
{
    unsigned char   *initial_pc = ce->mb->code;
    unsigned char   *max_pc = initial_pc + ce->mb->code_length;
    unsigned char   *pc;
    RangePtr	    *rp = ce->rp;
    long	    relpc, dist;
    long	    target;
    int 	    var;

    SET_RANGE_FLAG(0, RF_START);
    pc = initial_pc;
    while (pc < max_pc)
    {
	switch (*pc)
	{
	case opc_dload_0:
	case opc_dstore_0:
	    ce->varOff[1] = 1;
	    // fall thru
	case opc_fload_0:
	case opc_fstore_0:
	    ce->varOff[0] = 1;
	    pc++;
	    continue;

	case opc_dload_1:
	case opc_dstore_1:
	    ce->varOff[2] = 1;
	    // fall thru
	case opc_fload_1:
	case opc_fstore_1:
	    ce->varOff[1] = 1;
	    pc++;
	    continue;

	case opc_dload_2:
	case opc_dstore_2:
	    ce->varOff[3] = 1;
	    // fall thru
	case opc_fload_2:
	case opc_fstore_2:
	    ce->varOff[2] = 1;
	    pc++;
	    continue;

	case opc_dload_3:
	case opc_dstore_3:
	    ce->varOff[4] = 1;
	    // fall thru
	case opc_fload_3:
	case opc_fstore_3:
	    ce->varOff[3] = 1;
	    pc++;
	    continue;

	case opc_dload:
	case opc_dstore:
	    ce->varOff[pc[1]+1] = 1;
	    // fall thru
	case opc_fload:
	case opc_fstore:
	    ce->varOff[pc[1]] = 1;
	    // fall thru
	case opc_lload:
	case opc_lstore:
	case opc_aload:
	case opc_astore:
	case opc_iload:
	case opc_istore:
	case opc_newarray:
	case opc_bipush:
	case opc_ldc:
	case opc_ldc_quick:
	    pc += 2;
	    continue;

	case opc_iinc:
	case opc_anewarray:
	case opc_anewarray_quick:
	case opc_sipush:
	case opc_ldc_w:
	case opc_ldc_w_quick:
	case opc_ldc2_w:
	case opc_ldc2_w_quick:
	case opc_instanceof:
	case opc_instanceof_quick:
	case opc_checkcast:
	case opc_checkcast_quick:
	case opc_new:
	case opc_new_quick:
	case opc_putstatic:
	case opc_putstatic_quick:
	case opc_putstatic2_quick:
	case opc_getstatic:
	case opc_getstatic_quick:
	case opc_getstatic2_quick:
	case opc_putfield:
	case opc_putfield_quick:
	case opc_putfield_quick_w:
	case opc_putfield2_quick:
	case opc_getfield:
	case opc_getfield_quick:
	case opc_getfield_quick_w:
	case opc_getfield2_quick:
	case opc_invokevirtual:
	case opc_invokevirtual_quick:
	case opc_invokevirtual_quick_w:
	case opc_invokenonvirtual:
	case opc_invokenonvirtual_quick:
        case opc_invokesuper_quick:
	case opc_invokestatic:
	case opc_invokestatic_quick:
	case opc_invokevirtualobject_quick:
	    pc += 3;
	    continue;

	case opc_invokeinterface:
	case opc_invokeinterface_quick:
	    pc += 5;
	    continue;

	case opc_multianewarray:
	case opc_multianewarray_quick:
	    pc += 4;
	    continue;

	case opc_jsr:
	    relpc = pc - initial_pc;
	    target = relpc + pc2signedshort(pc);
	    SET_RANGE_FLAG(relpc + 3, RF_START);
	    SET_RANGE_FLAG(target, RF_JSR);
	    INC_JOIN_CNT(target);
	    pc += 3;
	    continue;

	case opc_jsr_w:
	    relpc = pc - initial_pc;
	    target = relpc + pc2signedlong(pc);
	    SET_RANGE_FLAG(relpc + 5, RF_START);
	    SET_RANGE_FLAG(target, RF_JSR);
	    INC_JOIN_CNT(target);
	    pc += 5;
	    continue;

	case opc_ret:
	    relpc = pc - initial_pc;
	    DEC_JOIN_CNT(relpc + 2);
	    pc += 2;
	    continue;

	case opc_ifeq:
	case opc_ifge:
	case opc_ifgt:
	case opc_ifle:
	case opc_iflt:
	case opc_ifne:
	case opc_if_icmpeq:
	case opc_if_icmpne:
	case opc_if_icmpge:
	case opc_if_icmpgt:
	case opc_if_icmple:
	case opc_if_icmplt:
	case opc_if_acmpeq:
	case opc_if_acmpne:
	case opc_ifnull:
	case opc_ifnonnull:
	    dist = pc2signedshort(pc);
	    relpc = pc - initial_pc;
	    target = relpc + dist;
	    SET_RANGE_FLAG(target, RF_START);
	    INC_JOIN_CNT(target);
	    SET_RANGE_FLAG(relpc + 3, RF_START);
	    pc += 3;
	    continue;

	case opc_goto:
	    dist = pc2signedshort(pc);
	    relpc = pc - initial_pc;
	    target = relpc + dist;
	    SET_RANGE_FLAG(target, RF_START);
	    INC_JOIN_CNT(target);
	    DEC_JOIN_CNT(relpc + 3);
	    SET_RANGE_FLAG(relpc + 3, RF_START);
	    pc += 3;
	    continue;

	case opc_goto_w:
	    dist = pc2signedlong(pc);
	    relpc = pc - initial_pc;
	    target = relpc + dist;
	    SET_RANGE_FLAG(target, RF_START);
	    INC_JOIN_CNT(target);
	    DEC_JOIN_CNT(relpc + 5);
	    SET_RANGE_FLAG(relpc + 5, RF_START);
	    pc += 5;
	    continue;

	case opc_return:
	case opc_areturn:
	case opc_ireturn:
	case opc_lreturn:
	case opc_freturn:
	case opc_dreturn:
	    // ce->epilogueRh->joinCnt++;	return code optimized now
	    relpc = pc - initial_pc;
	    DEC_JOIN_CNT(relpc + 1);
	    pc++;
	    continue;

	case opc_wide:
	    var = GET_INDEX(pc + 2);

	    switch(pc[1])
	    {
	    case opc_dload:
	    case opc_dstore:
		ce->varOff[var+1] = 1;
		// fall thru
	    case opc_fload:
	    case opc_fstore:
		ce->varOff[var] = 1;
		// fall thru
	    case opc_aload:
	    case opc_astore:
	    case opc_iload:
	    case opc_istore:
	    case opc_lload:
	    case opc_lstore:
		pc += 4;
		continue;

	    case opc_iinc:
		pc += 6;
		continue;

	    case opc_ret:
		relpc = pc - initial_pc;
		DEC_JOIN_CNT(relpc + 4);
		pc += 4;
		continue;

	    default:
		sysAssert(0);
		ce->err = "Undefined opcode";
		return FALSE;
	    }

	case opc_tableswitch:
	{
	    long *ltbl = (long *) ALIGN((int)pc + 1);
	    long def = swap(ltbl[0]);
	    long low = swap(ltbl[1]);
	    long high = swap(ltbl[2]);
	    long i;

	    relpc = pc - initial_pc;
	    target = relpc + def;
	    SET_RANGE_FLAG(target, RF_START);
	    INC_JOIN_CNT(target);
	    for (i = high - low + 1; --i >= 0; )
	    {
		dist = swap(ltbl[i + 3]);
		target = relpc + dist;
		// special range header needed for computing stack height:
		SET_RANGE_FLAG(relpc + i + 1, RF_DATABLOCK);
		SET_RANGE_FLAG(target, RF_START);
		INC_JOIN_CNT(target);
	    }

	    pc = (unsigned char *)(ltbl + 3 + high - low + 1);
	    DEC_JOIN_CNT(pc - initial_pc);
	}
	continue;

	case opc_lookupswitch:
	{
	    long *ltbl = (long *) ALIGN((int)pc + 1);
	    long def = swap(ltbl[0]);
	    long npairs = swap(ltbl[1]);
	    long i;

	    relpc = pc - initial_pc;
	    target = relpc + def;
	    SET_RANGE_FLAG(target, RF_START);
	    INC_JOIN_CNT(target);
	    for (i = npairs; --i >= 0; )
	    {
		dist = swap(ltbl[i * 2 + 3]);
		target = relpc + dist;
		// special range header needed for computing stack height:
		SET_RANGE_FLAG(relpc + i + 1, RF_DATABLOCK);
		SET_RANGE_FLAG(target, RF_START);
		INC_JOIN_CNT(target);
	    }

	    pc = (unsigned char *)(ltbl + 2 + npairs * 2);
	    DEC_JOIN_CNT(pc - initial_pc);
	}
	continue;

	case opc_athrow:
	    relpc = pc - initial_pc;
	    DEC_JOIN_CNT(relpc + 1);
	    pc++;
	    continue;

	case opc_breakpoint:
	// a method containing breakpoints should be interpreted
	// or should we compile a "throw BreakpointException"?
	    ce->err = "Breakpoint found in code";
	    return FALSE;

	//case opc_software:
        //case opc_hardware:
	case opc_invokeignored_quick:
	    ce->err = "opcode not implemented";
	    return FALSE;

	default:
	    sysAssert(opcode_length[*pc] == 1);
	    pc++;
	}
    }
    return TRUE;
}


static bool_t
AllocRangeHeaders(CompEnv *ce)
{
    long	    pc, exceptLev, tryEnterCnt, tryExitCnt;
    const long	    code_length = ce->mb->code_length;
    long	    xctxt;
    RangePtr	    *rp = ce->rp;
    RangeHdr	    *rh, *lastrh;

    lastrh = 0;
    xctxt = -1;
    exceptLev = 0;
    for (pc = 0; pc < code_length; pc++)
    {
	if  (rp[pc].hdr) // (rangeFlags || joinCnt || tryEnterCnt || tryExitCnt)
	{
	    tryEnterCnt = rp[pc].mark.tryEnterCnt;
	    tryExitCnt = rp[pc].mark.tryExitCnt;
	    if (tryEnterCnt != 0 || tryExitCnt != 0)
	    {
		exceptLev += tryEnterCnt - tryExitCnt;
		if  (exceptLev == 0)
		    xctxt = -1;
		else
		    xctxt = pc;
	    }

	    rh = (*p_calloc)(1, sizeof(RangeHdr));
	    if (rh == 0)
	    {
		ce->err = "Not enough memory";
		return FALSE;
	    }

	    if (lastrh)
		lastrh->nextRh = rh;

	    lastrh = rh;
	    rh->bcpc = pc;
	    rh->rangeFlags = rp[pc].mark.rangeFlags;
	    rh->joinCnt = rp[pc].mark.joinCnt;
	    rh->xctxt = xctxt;
	    rp[pc].hdr = rh;	// do this last: union!
	}
    }
    return TRUE;
}


/* This recursive function walks the control flow graph in order to determine
 * the absolute opstack height on entry for each range.
 * The relative opstack height on exit is already computed.
 * Each range is visited once.
 * The execution frequency of each range is estimated.
 */
static void
WalkControlFlow(RangeHdr *rh, long freqLev)
{
    RangeHdr *targRh, *contRh;
    long topOnExit;
    long f;

    while (1)
    {
	rh->rangeFlags |= RF_VISITED;
	topOnExit = rh->topOnExit;
        check(topOnExit >= 0);
	freqLev += rh->joinCnt;	// dead code (joinCnt == -1) not visited
	if (freqLev > 30)
    	    freqLev = 30;
	else if (freqLev < 0)
    	    freqLev = 0;
	rh->freq = 1 << freqLev;
	targRh = rh->targRh;
        contRh = rh->contRh;
	if  (targRh)
	{
	    if ((targRh->rangeFlags & RF_VISITED) == 0)
	    {
		targRh->topOnEntry = topOnExit;
		targRh->topOnExit += topOnExit;
		if (targRh->bcpc <= rh->bcpc)	// backwards jump
		    f = freqLev + 1;
		else if (contRh)
    		    f = freqLev - 1;	// e.g. if statement, divide freq by 2
		else
    		    f = freqLev;	// e.g. fwd jump to a loop condition
		WalkControlFlow(targRh, f);
	    }
	    else if (targRh->topOnEntry != topOnExit)
		OpStackHeightError();
	}
	rh = contRh;
	if  (rh)
	{
	    if ((rh->rangeFlags & RF_VISITED) == 0)
	    {
		rh->topOnEntry = topOnExit;
		rh->topOnExit += topOnExit;
		if (targRh)
    		    freqLev--;
		continue;
	    }
	    else if (rh->topOnEntry != topOnExit)
		OpStackHeightError();
	}
	break;
    }
}


/* For each range, this function computes the relative operand stack height
 * on exit, assuming that the height on entry is 0. It also determines whether
 * a range has to set its exception context on entry.
 * The constant pool has to be resolved to determine the size of some operands.
 * Set ce->codeInfo->baseOff to 0, 4, or 8. This allocates a place holder for the
 * result of invoked methods in order not to destroy the operand stack in case where
 * the result is larger than the arguments. We check that on a range basis,
 * which is a little bit too conservative, but easier.
 * Calls to CompSupport functions are not dangerous.
 */
static bool_t
ComputeOpstackHeight(CompEnv *ce)
{
#define SIZE_AND_STACK(size, stack) pc += size; top += stack; continue
#define SIZE_AND_STACK_BUT(size, stack) pc += size; top += stack
#define NEXT_RANGE()							       \
    top = 0;                                                                   \
    rh = nextRh;                                                               \
    nextRh = rh->nextRh;                                                       \
    if  (nextRh == NULL)                                                       \
        nextRhPc = max_pc;                                                     \
    else                                                                       \
        nextRhPc = initial_pc + nextRh->bcpc

#define SIZE_STACK_TARG_CONT(size, stack, targ, cont)			       \
    pc += size;                                                                \
    top += stack;                                                              \
    rh->targRh = targ;                                                         \
    rh->contRh = cont;                                                         \
    rh->topOnExit = top;                                                       \
    if  (targ && rh->xctxt != targ->xctxt)                                     \
        targ->rangeFlags |= RF_SET_XCTXT;                                      \
    if  (cont && rh->xctxt != cont->xctxt)                                     \
        cont->rangeFlags |= RF_SET_XCTXT;                                      \
    continue

#define SIZE_STACK_TARG_NOCONT(size, stack, targ)			       \
    pc += size;                                                                \
    top += stack;                                                              \
    rh->targRh = targ;                                                         \
    rh->contRh = 0;                                                            \
    rh->topOnExit = top;                                                       \
    if  (targ && rh->xctxt != targ->xctxt)                                     \
        targ->rangeFlags |= RF_SET_XCTXT;                                      \
    continue

#define SIZE_STACK_NOTARG_NOCONT(size, stack)				       \
    pc += size;                                                                \
    top += stack;                                                              \
    rh->targRh = 0;                                                            \
    rh->contRh = 0;                                                            \
    rh->topOnExit = top;                                                       \
    continue

#define SIZE_STACK_TARG_CONT_BUT(size, stack, targ, cont)		       \
    pc += size;                                                                \
    top += stack;                                                              \
    rh->targRh = targ;                                                         \
    rh->contRh = cont;                                                         \
    rh->topOnExit = top;                                                       \
    if  (targ && rh->xctxt != targ->xctxt)                                     \
        targ->rangeFlags |= RF_SET_XCTXT;                                      \
    if  (cont && rh->xctxt != cont->xctxt)                                     \
        cont->rangeFlags |= RF_SET_XCTXT

    struct methodblock *mb = ce->mb;
    struct execenv *ee = ce->ee;
    ClassClass *cb = fieldclass(&mb->fb);
    ClassClass *array_cb, *new_cb;
    union cp_item_type *cpool = cbConstantPool(cb);
    unsigned char *type_table = cpool[CONSTANT_POOL_TYPE_TABLE_INDEX].p;
    struct fieldblock *fb;
    struct methodblock *mb_type;
    char	    isig, *sig;
    unsigned char   *initial_pc = mb->code;
    unsigned char   *max_pc = initial_pc + mb->code_length;
    unsigned char   *pc, *nextRhPc;
    RangeHdr	    *rh, *nextRh, *targRh;
    unsigned	    index, ID;
    unsigned char   opcode;
    int 	    top, push, maxPush = 0;

    pc = initial_pc;
    rh = NULL;
    nextRh = ce->rp[0].hdr;
    nextRhPc = initial_pc;

    while (pc < max_pc)
    {
	if  (pc >= nextRhPc)
	{
	    // a new range starts here
	    sysAssert(pc == nextRhPc);
	    if (rh && rh->contRh == UNDEF_RH)
	    {
		// normal control flow from rh to nextRh
		rh->contRh = nextRh;
		rh->topOnExit = top;
		if  (rh->xctxt != nextRh->xctxt)
		    nextRh->rangeFlags |= RF_SET_XCTXT;
	    }
	    NEXT_RANGE();
	    sysAssert((rh->rangeFlags & RF_DATABLOCK) == 0);
	    rh->contRh = UNDEF_RH;
	    if (rh->rangeFlags & RF_JSR)
		top++;
	}
	opcode = *pc;
	switch (opcode)
	{
	case opc_nop:
	    SIZE_AND_STACK(1, 0);

	case opc_aload:
	case opc_iload:
	case opc_fload:
	    SIZE_AND_STACK(2, 1);

	case opc_lload:
	case opc_dload:
	    SIZE_AND_STACK(2, 2);

	case opc_iload_0:
	case opc_aload_0:
	case opc_fload_0:
	case opc_iload_1:
	case opc_aload_1:
	case opc_fload_1:
	case opc_iload_2:
	case opc_aload_2:
	case opc_fload_2:
	case opc_iload_3:
	case opc_aload_3:
	case opc_fload_3:
	    SIZE_AND_STACK(1, 1);

	case opc_lload_0:
	case opc_dload_0:
	case opc_lload_1:
	case opc_dload_1:
	case opc_lload_2:
	case opc_dload_2:
	case opc_lload_3:
	case opc_dload_3:
	    SIZE_AND_STACK(1, 2);

	case opc_istore:
	case opc_astore:
	case opc_fstore:
	    SIZE_AND_STACK(2, -1);

	case opc_lstore:
	case opc_dstore:
	    SIZE_AND_STACK(2, -2);

	case opc_istore_0:
	case opc_astore_0:
	case opc_fstore_0:
	case opc_istore_1:
	case opc_astore_1:
	case opc_fstore_1:
	case opc_istore_2:
	case opc_astore_2:
	case opc_fstore_2:
	case opc_istore_3:
	case opc_astore_3:
	case opc_fstore_3:
	    SIZE_AND_STACK(1, -1);

	case opc_lstore_0:
	case opc_dstore_0:
	case opc_lstore_1:
	case opc_dstore_1:
	case opc_lstore_2:
	case opc_dstore_2:
	case opc_lstore_3:
	case opc_dstore_3:
	    SIZE_AND_STACK(1, -2);

	case opc_return:
	case opc_areturn:
	case opc_ireturn:
	case opc_lreturn:
	case opc_freturn:
	case opc_dreturn:
	    targRh = ce->epilogueRh;
	    SIZE_STACK_TARG_NOCONT(1, 0, targRh);

	case opc_i2f:
	case opc_f2i:
	case opc_l2d:
	case opc_d2l:
	case opc_int2byte:
	case opc_int2char:
	case opc_int2short:
	    SIZE_AND_STACK(1, 0);

	case opc_i2l:
	case opc_i2d:
	case opc_f2l:
	case opc_f2d:
	    SIZE_AND_STACK(1, 1);

	case opc_l2i:
	case opc_l2f:
	case opc_d2i:
	case opc_d2f:
	    SIZE_AND_STACK(1, -1);

	case opc_fcmpl:
	case opc_fcmpg:
	    SIZE_AND_STACK(1, -1);

	case opc_lcmp:
	case opc_dcmpl:
	case opc_dcmpg:
	    SIZE_AND_STACK(1, -3);

	case opc_bipush:
	    SIZE_AND_STACK(2, 1);

	case opc_sipush:
	    SIZE_AND_STACK(3, 1);

	case opc_ldc:
	    if (!(*p_ResolveClassConstantFromClass)(cb, pc[1], ee,
		     (1 << CONSTANT_Integer) |
		     (1 << CONSTANT_Float) |
		     (1 << CONSTANT_String)))
	    {
		ce->err = "Unable to resolve class constant";
		return FALSE;
	    }
	    // break thru

	case opc_ldc_quick:
	    SIZE_AND_STACK(2, 1);

	case opc_ldc_w:
	    if (!(*p_ResolveClassConstantFromClass)(cb, GET_INDEX(pc + 1), ee,
		     (1 << CONSTANT_Integer) |
		     (1 << CONSTANT_Float) |
		     (1 << CONSTANT_String)))
	    {
		ce->err = "Unable to resolve class constant";
		return FALSE;
	    }
	    // break thru

	case opc_ldc_w_quick:
	    SIZE_AND_STACK(3, 1);

	case opc_ldc2_w:
	    if (!(*p_ResolveClassConstantFromClass)(cb, GET_INDEX(pc + 1), ee,
		 (1 << CONSTANT_Double) | (1 << CONSTANT_Long)))
	    {
		ce->err = "Unable to resolve class constant";
		return FALSE;
	    }
	    // break thru

	case opc_ldc2_w_quick:
	    SIZE_AND_STACK(3, 2);

	case opc_aconst_null:
	case opc_iconst_m1:
	case opc_iconst_0:
	case opc_iconst_1:
	case opc_iconst_2:
	case opc_iconst_3:
	case opc_iconst_4:
	case opc_iconst_5:
	case opc_fconst_0:
	case opc_fconst_1:
	case opc_fconst_2:
	    SIZE_AND_STACK(1, 1);

	case opc_dconst_0:
	case opc_dconst_1:
	case opc_lconst_0:
	case opc_lconst_1:
	    SIZE_AND_STACK(1, 2);

	case opc_iadd:
	case opc_isub:
	case opc_iand:
	case opc_ior:
	case opc_ixor:
	case opc_imul:
	case opc_idiv:
	case opc_irem:
	    SIZE_AND_STACK(1, -1);

	case opc_ladd:
	case opc_lsub:
	case opc_land:
	case opc_lor:
	case opc_lxor:
	case opc_lmul:
	case opc_ldiv:
	case opc_lrem:
	    SIZE_AND_STACK(1, -2);

	case opc_ishl:
	case opc_ishr:
	case opc_iushr:
	case opc_lshl:
	case opc_lshr:
	case opc_lushr:
	    SIZE_AND_STACK(1, -1);

	case opc_ineg:
	case opc_lneg:
	case opc_fneg:
	case opc_dneg:
	    SIZE_AND_STACK(1, 0);

	case opc_iinc:
	    SIZE_AND_STACK(3, 0);

	case opc_fadd:
	case opc_fsub:
	case opc_fmul:
	case opc_fdiv:
	case opc_frem:
	    SIZE_AND_STACK(1, -1);

	case opc_dadd:
	case opc_dsub:
	case opc_dmul:
	case opc_ddiv:
	case opc_drem:
	    SIZE_AND_STACK(1, -2);

	case opc_jsr:
	    targRh = ce->rp[pc + pc2signedshort(pc) - initial_pc].hdr;
	    SIZE_STACK_TARG_CONT_BUT(3, 0, targRh, nextRh);
	    if (rh->xctxt != targRh->xctxt)
		nextRh->rangeFlags |= RF_SET_XCTXT; // restore xctxt after return
	    continue;
	    // 0: ret addr is not pushed onto the opstack

	case opc_jsr_w:
	    targRh = ce->rp[pc + pc2signedlong(pc) - initial_pc].hdr;
	    SIZE_STACK_TARG_CONT_BUT(5, 0, targRh, nextRh);
	    if (rh->xctxt != targRh->xctxt)
		nextRh->rangeFlags |= RF_SET_XCTXT; // restore xctxt after return
	    continue;
	    // 0: ret addr is not pushed onto the opstack

	case opc_goto:
	    targRh = ce->rp[pc + pc2signedshort(pc) - initial_pc].hdr;
	    SIZE_STACK_TARG_NOCONT(3, 0, targRh);

	case opc_goto_w:
	    targRh = ce->rp[pc + pc2signedlong(pc) - initial_pc].hdr;
	    SIZE_STACK_TARG_NOCONT(5, 0, targRh);

	case opc_ret:
	    SIZE_STACK_NOTARG_NOCONT(2, 0);

	case opc_if_icmpeq:
	case opc_if_icmpne:
	case opc_if_icmplt:
	case opc_if_icmpgt:
	case opc_if_icmple:
	case opc_if_icmpge:
	case opc_if_acmpeq:
	case opc_if_acmpne:
	    targRh = ce->rp[pc + pc2signedshort(pc) - initial_pc].hdr;
	    SIZE_STACK_TARG_CONT(3, -2, targRh, nextRh);

	case opc_ifeq:
	case opc_ifne:
	case opc_iflt:
	case opc_ifgt:
	case opc_ifle:
	case opc_ifge:
	case opc_ifnull:
	case opc_ifnonnull:
	    targRh = ce->rp[pc + pc2signedshort(pc) - initial_pc].hdr;
	    SIZE_STACK_TARG_CONT(3, -1, targRh, nextRh);

	case opc_pop:
	    SIZE_AND_STACK(1, -1);

	case opc_pop2:
	    SIZE_AND_STACK(1, -2);

	case opc_dup:
	case opc_dup_x1:
	case opc_dup_x2:
	    SIZE_AND_STACK(1, 1);

	case opc_dup2:
	case opc_dup2_x1:
	case opc_dup2_x2:
	    SIZE_AND_STACK(1, 2);

	case opc_swap:
	    SIZE_AND_STACK(1, 0);

	case opc_arraylength:
	    SIZE_AND_STACK(1, 0);

	case opc_tableswitch:
	{
	    long *ltbl = (long *) ALIGN((int)pc + 1);
	    long def = swap(ltbl[0]);
	    long low = swap(ltbl[1]);
	    long high = swap(ltbl[2]);
	    long npairs = high - low + 1;
	    long i, relpc, dist;

	    relpc = pc - initial_pc;
	    targRh = ce->rp[relpc + def].hdr;
	    SIZE_STACK_TARG_CONT_BUT(0, -1, targRh, nextRh);
	    for (i = 0; i < npairs; i++)
	    {
		NEXT_RANGE();
		dist = swap(ltbl[i + 3]);
		targRh = ce->rp[relpc + dist].hdr;
		SIZE_STACK_TARG_CONT_BUT(0, 0, targRh, nextRh);
	    }
	    pc = (unsigned char *)(ltbl + 3 + npairs);
	    rh->contRh = 0;
	}
	continue;

	case opc_lookupswitch:
	{
	    long *ltbl = (long *) ALIGN((int)pc + 1);
	    long def = swap(ltbl[0]);
	    long npairs = swap(ltbl[1]);
	    long i, relpc, dist;

	    relpc = pc - initial_pc;
	    targRh = ce->rp[relpc + def].hdr;
	    SIZE_STACK_TARG_CONT_BUT(0, -1, targRh, nextRh);
	    for (i = 0; i < npairs; i++)
	    {
		NEXT_RANGE();
		dist = swap(ltbl[i * 2 + 3]);
		targRh = ce->rp[relpc + dist].hdr;
		SIZE_STACK_TARG_CONT_BUT(0, 0, targRh, nextRh);
	    }
	    pc = (unsigned char *)(ltbl + 2 + npairs * 2);
	    rh->contRh = 0;
	}
	continue;

	case opc_athrow:
	    SIZE_STACK_NOTARG_NOCONT(1, -1);

	case opc_getfield:
	case opc_putfield:
	case opc_getstatic:
	case opc_putstatic:
	    index = GET_INDEX(pc + 1);
	    if (!(*p_ResolveClassConstantFromClass)(cb, index, ee,
		    1 << CONSTANT_Fieldref))
	    {
                exceptionClear(ee);
		ce->err = "Unable to resolve field ref";
		return FALSE;
	    }
	    fb = cpool[index].p;
	    if (    ((opcode == opc_getstatic || opcode == opc_putstatic)
		     == ((fb->access & ACC_STATIC) == 0))
		||
		    ((opcode == opc_putstatic || opcode == opc_putfield)
		     && (fb->access & ACC_FINAL)
                     && (!mb || (fieldclass(fb) != fieldclass(&mb->fb)))))
	    {
		ce->err = "Incompatible class change";
		return FALSE;
		// will be reported by the interpreter
	    }
	    isig = fieldsig(fb)[0];
	    if (isig == SIGNATURE_LONG || isig == SIGNATURE_DOUBLE)
	    {
		switch	(opcode)
		{
		case opc_getfield:
		    push = 1; break;
		case opc_putfield:
		    push = -3; break;
		case opc_getstatic:
		    push = 2; break;
		case opc_putstatic:
		    push = -2; break;
		}
	    }
	    else
	    {
		switch	(opcode)
		{
		case opc_getfield:
		    push = 0; break;
		case opc_putfield:
		    push = -2; break;
		case opc_getstatic:
		    push = 1; break;
		case opc_putstatic:
		    push = -1; break;
		}
	    }
	    SIZE_AND_STACK(3, push);

	case opc_putfield_quick:
	    SIZE_AND_STACK(3, -2);

	case opc_getfield_quick:
	    SIZE_AND_STACK(3, 0);

	case opc_putfield2_quick:
	    SIZE_AND_STACK(3, -3);

	case opc_getfield2_quick:
	    SIZE_AND_STACK(3, 1);

	case opc_putstatic_quick:
	    SIZE_AND_STACK(3, -1);

	case opc_getstatic_quick:
	    SIZE_AND_STACK(3, 1);

	case opc_putstatic2_quick:
	    SIZE_AND_STACK(3, -2);

	case opc_getstatic2_quick:
	    SIZE_AND_STACK(3, 2);

	case opc_putfield_quick_w:
	    fb = cpool[GET_INDEX(pc + 1)].p;
	    isig = fieldsig(fb)[0];
	    if (isig == SIGNATURE_LONG || isig == SIGNATURE_DOUBLE)
	    {
		SIZE_AND_STACK(3, -3);
	    }
	    else
	    {
		SIZE_AND_STACK(3, -2);
	    }

	case opc_getfield_quick_w:
	    fb = cpool[GET_INDEX(pc + 1)].p;
	    isig = fieldsig(fb)[0];
	    if (isig == SIGNATURE_LONG || isig == SIGNATURE_DOUBLE)
	    {
		SIZE_AND_STACK(3, 1);
	    }
	    else
	    {
		SIZE_AND_STACK(3, 0);
	    }

	case opc_invokevirtual:
	case opc_invokenonvirtual:
	case opc_invokestatic:
	    index = GET_INDEX(pc + 1);
	    if (!(*p_ResolveClassConstantFromClass)(cb, index, ee,
		    1 << CONSTANT_Methodref))
	    {
                exceptionClear(ee);
		ce->err = "Unable to resolve method ref";
		return FALSE;
	    }
	    mb_type = cpool[index].p;
	    if (    (opcode == opc_invokestatic)
		==  ((mb_type->fb.access & ACC_STATIC) == 0))
	    {
		ce->err = "Incompatible class change";
		return FALSE;
		// will be reported by the interpreter
	    }
	    push = ResultSize(fieldsig(&mb_type->fb)) - mb_type->args_size;
	    if (push > maxPush)
		maxPush = push;
	    SIZE_AND_STACK(3, push);

	case opc_invokeinterface:
	    index = GET_INDEX(pc + 1);
	    if (!(*p_ResolveClassConstantFromClass)(cb, index, ee,
		    1 << CONSTANT_InterfaceMethodref))
	    {
		ce->err = "Unable to resolve interface ref";
		return FALSE;
	    }
	    ID = cpool[cpool[index].i & 0xFFFF].i;
	    sig = (*p_IDToType)(ID);
	    push = ResultSize(sig) - pc[3];
	    if (push > maxPush)
		maxPush = push;
	    SIZE_AND_STACK(5, push);

	case opc_invokenonvirtual_quick:
	case opc_invokevirtual_quick_w:
	case opc_invokestatic_quick:
	    mb_type = cpool[GET_INDEX(pc + 1)].p;
	    push = ResultSize(fieldsig(&mb_type->fb)) - mb_type->args_size;
	    if (push > maxPush)
		maxPush = push;
	    SIZE_AND_STACK(3, push);

	case opc_invokesuper_quick:
	    mb_type = cbMethodTable(unhand(cbSuperclass(cb)))
		->methods[GET_INDEX(pc + 1)];
	    push = ResultSize(fieldsig(&mb_type->fb)) - mb_type->args_size;
	    if (push > maxPush)
		maxPush = push;
	    SIZE_AND_STACK(3, push);

	case opc_invokeinterface_quick:
	    ID = cpool[GET_INDEX(pc + 1)].i;
	    sig = (*p_IDToType)(ID);
	    push = ResultSize(sig) - pc[3];
	    if (push > maxPush)
		maxPush = push;
	    SIZE_AND_STACK(5, push);

	case opc_invokevirtualobject_quick:
	    ce->err = "opc_invokevirtualobject_quick cannot be compiled";
	    return FALSE;
	    // push = ResultSize(sig which is unknown) - pc[2];
	    // if   (push > maxPush)
	    //	maxPush = push;
	    // SIZE_AND_STACK(3, push);

	case opc_invokevirtual_quick:
	    ce->err = "opc_invokevirtual_quick cannot be compiled";
	    return FALSE;
	    // push = ResultSize(sig which is unknown) - pc[2];
	    // if   (push > maxPush)
	    //	maxPush = push;
	    // SIZE_AND_STACK(3, push);

	case opc_instanceof:
	case opc_checkcast:
	    if (!(*p_ResolveClassConstantFromClass)(cb, GET_INDEX(pc + 1), ee,
		     1 << CONSTANT_Class))
	    {
                exceptionClear(ee);
		ce->err = "Unable to resolve class constant";
		return FALSE;
	    }
	    // break thru

	case opc_instanceof_quick:
	case opc_checkcast_quick:
	    SIZE_AND_STACK(3, 0);

	case opc_new:
	    index = GET_INDEX(pc + 1);
	    if (!(*p_ResolveClassConstantFromClass)(cb, index, ee,
		     1 << CONSTANT_Class))
	    {
                exceptionClear(ee);
		ce->err = "Unable to resolve class constant";
		return FALSE;
	    }
	    new_cb = cpool[GET_INDEX(pc + 1)].p;
	    if (cbAccess(new_cb) & (ACC_INTERFACE | ACC_ABSTRACT))
	    {
		ce->err = "Instantiation error";
		return FALSE;
		// will be reported by the interpreter
	    }
	    if (!(*p_VerifyClassAccess)(cb, new_cb, FALSE))
	    {
		ce->err = "Illegal access error";
		return FALSE;
		// will be reported by the interpreter
	    }
	    // break thru

	case opc_new_quick:
	    SIZE_AND_STACK(3, 1);

	case opc_anewarray:
	    if (!(*p_ResolveClassConstantFromClass)(cb, GET_INDEX(pc + 1), ee,
		     1 << CONSTANT_Class))
	    {
                exceptionClear(ee);
		ce->err = "Unable to resolve class constant";
		return FALSE;
	    }
	    // break thru

	case opc_anewarray_quick:
	    SIZE_AND_STACK(3, 0);

	case opc_multianewarray:
	    if (!(*p_ResolveClassConstantFromClass)(cb, GET_INDEX(pc + 1), ee,
		     1 << CONSTANT_Class))
	    {
                exceptionClear(ee);
		ce->err = "Unable to resolve class constant";
		return FALSE;
	    }
	    // break thru

	case opc_multianewarray_quick:
	    array_cb = cpool[GET_INDEX(pc + 1)].p;
	    (*p_ResolveClassConstantFromClass)(array_cb,
		CONSTANT_POOL_ARRAY_CLASS_INDEX, ee, 1 << CONSTANT_Class);
	    push = 1 - pc[3];
	    SIZE_AND_STACK(4, push);

	case opc_newarray:
	    SIZE_AND_STACK(2, 0);

	case opc_iaload:
	case opc_faload:
	case opc_aaload:
	case opc_baload:
	case opc_caload:
	case opc_saload:
	    SIZE_AND_STACK(1, -1);

	case opc_laload:
	case opc_daload:
	    SIZE_AND_STACK(1, 0);

	case opc_iastore:
	case opc_fastore:
	case opc_aastore:
	case opc_bastore:
	case opc_castore:
	case opc_sastore:
	    SIZE_AND_STACK(1, -3);

	case opc_lastore:
	case opc_dastore:
	    SIZE_AND_STACK(1, -4);

	case opc_monitorenter:
	case opc_monitorexit:
	    SIZE_AND_STACK(1, -1);

	case opc_breakpoint:
	    check(0);	// should be found in first pass

	case opc_wide:
	    switch(pc[1])
	    {
	    case opc_aload:
	    case opc_iload:
	    case opc_fload:
		SIZE_AND_STACK(4, 1);

	    case opc_lload:
	    case opc_dload:
		SIZE_AND_STACK(4, 2);

	    case opc_istore:
	    case opc_astore:
		SIZE_AND_STACK(4, -1);

	    case opc_fstore:
		SIZE_AND_STACK(4, -1);

	    case opc_lstore:
		SIZE_AND_STACK(4, -2);

	    case opc_dstore:
		SIZE_AND_STACK(4, -2);

	    case opc_iinc:
		SIZE_AND_STACK(6, 0);

	    case opc_ret:
		SIZE_STACK_NOTARG_NOCONT(4, 0);

	    default:
		sysAssert(0);
		ce->err = "Undefined opcode";
		return FALSE;
	    }

	default:
	    sysAssert(0);
	    ce->err = "Undefined opcode";
	    return FALSE;
	}
    }
    sysAssert(maxPush >= 0 && maxPush <= 2);
    ce->codeInfo->baseOff = 4*maxPush;
    // ce->epilogueRh->joinCnt--;	return code optimized now
    rh = ce->rp[0].hdr;
    rh->topOnEntry = 0;
    WalkControlFlow(rh, FREQ_UNIT_LEV);
    return TRUE;
}


/* Estimate the frequency of use of each variable.
 */
static bool_t
EstimateVarFreq(CompEnv *ce)
{
#define ACCESS_VAR(v)							       \
    freq = varFreq[v] + rFreq;						       \
    if (freq < 0)							       \
	freq = 0x40000000;						       \
    varFreq[v] = freq						

    RangeHdr *rh, *nextRh;
    unsigned char *pc, *nextRhPc;
    unsigned char *initial_pc;
    struct methodblock *mb = ce->mb;
    int var;
    long *varFreq, rFreq, freq;

    initial_pc = mb->code;
    pc = initial_pc;
    rh = ce->rp[0].hdr;
    varFreq = ce->varFreq;
    while (rh)
    {
	// ignore any ranges that do not contain code...
	if (rh->rangeFlags & RF_DATABLOCK)
	{
	    rh = rh->nextRh;
	    continue;
	}
	nextRh = rh->nextRh;
	if (nextRh == NULL)
	    nextRhPc = initial_pc + mb->code_length;
	else
	    nextRhPc = initial_pc + nextRh->bcpc;

	rFreq = rh->freq;
	sysAssert(rFreq > 0);
	while (pc < nextRhPc)
	{
	    switch (*pc)
	    {
	    case opc_lload_0:
	    case opc_dload_0:
	    case opc_lstore_0:
	    case opc_dstore_0:
		ACCESS_VAR(1);
		// fall thru
	    case opc_iload_0:
	    case opc_aload_0:
	    case opc_fload_0:
	    case opc_istore_0:
	    case opc_astore_0:
	    case opc_fstore_0:
		ACCESS_VAR(0);
		pc++;
		break;

	    case opc_lload_1:
	    case opc_dload_1:
	    case opc_lstore_1:
	    case opc_dstore_1:
		ACCESS_VAR(2);
		// fall thru
	    case opc_iload_1:
	    case opc_aload_1:
	    case opc_fload_1:
	    case opc_istore_1:
	    case opc_astore_1:
	    case opc_fstore_1:
		ACCESS_VAR(1);
		pc++;
		continue;

	    case opc_lload_2:
	    case opc_dload_2:
	    case opc_lstore_2:
	    case opc_dstore_2:
		ACCESS_VAR(3);
		// fall thru
	    case opc_iload_2:
	    case opc_aload_2:
	    case opc_fload_2:
	    case opc_istore_2:
	    case opc_astore_2:
	    case opc_fstore_2:
		ACCESS_VAR(2);
		pc++;
		continue;

	    case opc_lload_3:
	    case opc_dload_3:
	    case opc_lstore_3:
	    case opc_dstore_3:
		ACCESS_VAR(4);
		// fall thru
	    case opc_iload_3:
	    case opc_aload_3:
	    case opc_fload_3:
	    case opc_istore_3:
	    case opc_astore_3:
	    case opc_fstore_3:
		ACCESS_VAR(3);
		pc++;
		continue;

	    case opc_lload:
	    case opc_dload:
	    case opc_lstore:
	    case opc_dstore:
		ACCESS_VAR(pc[1]+1);
		// fall thru
	    case opc_iload:
	    case opc_aload:
	    case opc_fload:
	    case opc_istore:
	    case opc_astore:
	    case opc_fstore:
		ACCESS_VAR(pc[1]);
		pc += 2;
		continue;

	    case opc_newarray:
	    case opc_bipush:
	    case opc_ldc:
	    case opc_ldc_quick:
	    case opc_ret:
		pc += 2;
		continue;

	    case opc_iinc:
	    case opc_anewarray:
	    case opc_anewarray_quick:
	    case opc_sipush:
	    case opc_ldc_w:
	    case opc_ldc_w_quick:
	    case opc_ldc2_w:
	    case opc_ldc2_w_quick:
	    case opc_instanceof:
	    case opc_instanceof_quick:
	    case opc_checkcast:
	    case opc_checkcast_quick:
	    case opc_new:
	    case opc_new_quick:
	    case opc_putstatic:
	    case opc_putstatic_quick:
	    case opc_putstatic2_quick:
	    case opc_getstatic:
	    case opc_getstatic_quick:
	    case opc_getstatic2_quick:
	    case opc_putfield:
	    case opc_putfield_quick:
	    case opc_putfield_quick_w:
	    case opc_putfield2_quick:
	    case opc_getfield:
	    case opc_getfield_quick:
	    case opc_getfield_quick_w:
	    case opc_getfield2_quick:
	    case opc_invokevirtual:
	    case opc_invokevirtual_quick:
	    case opc_invokevirtual_quick_w:
	    case opc_invokenonvirtual:
	    case opc_invokenonvirtual_quick:
	    case opc_invokesuper_quick:
	    case opc_invokestatic:
	    case opc_invokestatic_quick:
	    case opc_invokevirtualobject_quick:
	    case opc_jsr:
	    case opc_ifeq:
	    case opc_ifge:
	    case opc_ifgt:
	    case opc_ifle:
	    case opc_iflt:
	    case opc_ifne:
	    case opc_if_icmpeq:
	    case opc_if_icmpne:
	    case opc_if_icmpge:
	    case opc_if_icmpgt:
	    case opc_if_icmple:
	    case opc_if_icmplt:
	    case opc_if_acmpeq:
	    case opc_if_acmpne:
	    case opc_ifnull:
	    case opc_ifnonnull:
	    case opc_goto:
		pc += 3;
		continue;

	    case opc_invokeinterface:
	    case opc_invokeinterface_quick:
	    case opc_jsr_w:
	    case opc_goto_w:
		pc += 5;
		continue;

	    case opc_multianewarray:
	    case opc_multianewarray_quick:
		pc += 4;
		continue;

	    case opc_wide:
		var = GET_INDEX(pc + 2);

		switch(pc[1])
		{
		case opc_lload:
		case opc_dload:
		case opc_lstore:
		case opc_dstore:
		    ACCESS_VAR(var+1);
		    // fall thru
		case opc_iload:
		case opc_aload:
		case opc_fload:
		case opc_istore:
		case opc_astore:
		case opc_fstore:
		    ACCESS_VAR(var);
		    pc += 4;
		    continue;

		case opc_ret:
		    pc += 4;
		    continue;

		case opc_iinc:
		    pc += 6;
		    continue;

		default:
		    sysAssert(0);
		    ce->err = "Undefined opcode";
		    return FALSE;
		}

	    case opc_tableswitch:
	    {
		long *ltbl = (long *) ALIGN((int)pc + 1);
		long low = swap(ltbl[1]);
		long high = swap(ltbl[2]);

		pc = (unsigned char *)(ltbl + 3 + high - low + 1);
	    }
	    continue;

	    case opc_lookupswitch:
	    {
		long *ltbl = (long *) ALIGN((int)pc + 1);
		long npairs = swap(ltbl[1]);

		pc = (unsigned char *)(ltbl + 2 + npairs * 2);
	    }
	    continue;

	    default:
		sysAssert(opcode_length[*pc] == 1);
		pc++;
	    }
	}
	rh = nextRh;
    }
    return TRUE;
}


/* Mark variables that cannot be allocated in registers, because they are used
 * in exception handlers.
 */
static bool_t
MarkXHVars(CompEnv *ce, unsigned char *pc, unsigned char *max_pc)
{
    int var;

    while (pc < max_pc)
    {
	switch (*pc)
	{
	case opc_lload_0:
	case opc_dload_0:
	case opc_lstore_0:
	case opc_dstore_0:
	    ce->varOff[1] = 1;
	    // fall thru
	case opc_iload_0:
	case opc_aload_0:
	case opc_fload_0:
	case opc_istore_0:
	case opc_astore_0:
	case opc_fstore_0:
	    ce->varOff[0] = 1;
	    pc++;
	    continue;

	case opc_lload_1:
	case opc_dload_1:
	case opc_lstore_1:
	case opc_dstore_1:
	    ce->varOff[2] = 1;
	    // fall thru
	case opc_iload_1:
	case opc_aload_1:
	case opc_fload_1:
	case opc_istore_1:
	case opc_astore_1:
	case opc_fstore_1:
	    ce->varOff[1] = 1;
	    pc++;
	    continue;

	case opc_lload_2:
	case opc_dload_2:
	case opc_lstore_2:
	case opc_dstore_2:
	    ce->varOff[3] = 1;
	    // fall thru
	case opc_iload_2:
	case opc_aload_2:
	case opc_fload_2:
	case opc_istore_2:
	case opc_astore_2:
	case opc_fstore_2:
	    ce->varOff[2] = 1;
	    pc++;
	    continue;

	case opc_lload_3:
	case opc_dload_3:
	case opc_lstore_3:
	case opc_dstore_3:
	    ce->varOff[4] = 1;
	    // fall thru
	case opc_iload_3:
	case opc_aload_3:
	case opc_fload_3:
	case opc_istore_3:
	case opc_astore_3:
	case opc_fstore_3:
	    ce->varOff[3] = 1;
	    pc++;
	    continue;

	case opc_lload:
	case opc_dload:
	case opc_lstore:
	case opc_dstore:
	    ce->varOff[pc[1]+1] = 1;
	    // fall thru
	case opc_iload:
	case opc_aload:
	case opc_fload:
	case opc_istore:
	case opc_astore:
	case opc_fstore:
	    ce->varOff[pc[1]] = 1;
	    pc += 2;
	    continue;

	case opc_newarray:
	case opc_bipush:
	case opc_ldc:
	case opc_ldc_quick:
	case opc_ret:
	    pc += 2;
	    continue;

	case opc_iinc:
	case opc_anewarray:
	case opc_anewarray_quick:
	case opc_sipush:
	case opc_ldc_w:
	case opc_ldc_w_quick:
	case opc_ldc2_w:
	case opc_ldc2_w_quick:
	case opc_instanceof:
	case opc_instanceof_quick:
	case opc_checkcast:
	case opc_checkcast_quick:
	case opc_new:
	case opc_new_quick:
	case opc_putstatic:
	case opc_putstatic_quick:
	case opc_putstatic2_quick:
	case opc_getstatic:
	case opc_getstatic_quick:
	case opc_getstatic2_quick:
	case opc_putfield:
	case opc_putfield_quick:
	case opc_putfield_quick_w:
	case opc_putfield2_quick:
	case opc_getfield:
	case opc_getfield_quick:
	case opc_getfield_quick_w:
	case opc_getfield2_quick:
	case opc_invokevirtual:
	case opc_invokevirtual_quick:
	case opc_invokevirtual_quick_w:
	case opc_invokenonvirtual:
	case opc_invokenonvirtual_quick:
	case opc_invokesuper_quick:
	case opc_invokestatic:
	case opc_invokestatic_quick:
	case opc_invokevirtualobject_quick:
	case opc_jsr:
	case opc_ifeq:
	case opc_ifge:
	case opc_ifgt:
	case opc_ifle:
	case opc_iflt:
	case opc_ifne:
	case opc_if_icmpeq:
	case opc_if_icmpne:
	case opc_if_icmpge:
	case opc_if_icmpgt:
	case opc_if_icmple:
	case opc_if_icmplt:
	case opc_if_acmpeq:
	case opc_if_acmpne:
	case opc_ifnull:
	case opc_ifnonnull:
	case opc_goto:
	    pc += 3;
	    continue;

	case opc_invokeinterface:
	case opc_invokeinterface_quick:
	case opc_jsr_w:
	case opc_goto_w:
	    pc += 5;
	    continue;

	case opc_multianewarray:
	case opc_multianewarray_quick:
	    pc += 4;
	    continue;

	case opc_wide:
	    var = GET_INDEX(pc + 2);

	    switch(pc[1])
	    {
	    case opc_lload:
	    case opc_dload:
	    case opc_lstore:
	    case opc_dstore:
		ce->varOff[var+1] = 1;
		// fall thru
	    case opc_iload:
	    case opc_aload:
	    case opc_fload:
	    case opc_istore:
	    case opc_astore:
	    case opc_fstore:
		ce->varOff[var] = 1;
		pc += 4;
		continue;

	    case opc_ret:
		pc += 4;
		continue;

	    case opc_iinc:
		pc += 6;
		continue;

	    default:
		sysAssert(0);
		ce->err = "Undefined opcode";
		return FALSE;
	    }

	case opc_tableswitch:
	{
	    long *ltbl = (long *) ALIGN((int)pc + 1);
	    long low = swap(ltbl[1]);
	    long high = swap(ltbl[2]);

	    pc = (unsigned char *)(ltbl + 3 + high - low + 1);
	}
	continue;

	case opc_lookupswitch:
	{
	    long *ltbl = (long *) ALIGN((int)pc + 1);
	    long npairs = swap(ltbl[1]);

	    pc = (unsigned char *)(ltbl + 2 + npairs * 2);
	}
	continue;

	default:
	    sysAssert(opcode_length[*pc] == 1);
	    pc++;
	}
    }
    return TRUE;
}


/* This recursive function walks the control flow graph of exception handlers
 * in order to mark used variables. Each reachable range is visited once.
 */
static void
WalkXHControlFlow(CompEnv *ce, RangeHdr *rh)
{
    RangeHdr	    *targRh, *nextRh;
    unsigned char   *nextRhPc, *initial_pc = ce->mb->code;

    while (1)
    {
	rh->rangeFlags |= RF_XHVISITED;
	nextRh = rh->nextRh;
	if  (nextRh == NULL)
	    nextRhPc = initial_pc + ce->mb->code_length;
	else
	    nextRhPc = initial_pc + nextRh->bcpc;

	// ignore any ranges that do not contain code...
	if ((rh->rangeFlags & RF_DATABLOCK) == 0)
	    MarkXHVars(ce, initial_pc + rh->bcpc, nextRhPc);

	targRh = rh->targRh;
	if  (targRh)
	{
	    if ((targRh->rangeFlags & RF_XHVISITED) == 0)
		WalkXHControlFlow(ce, targRh);
	}
	rh = rh->contRh;
	if  (rh)
	{
	    if ((rh->rangeFlags & RF_XHVISITED) == 0)
		continue;
	}
	break;
    }
}


/* Write the index number of each catch frame in the entry range of the handler.
 * This will be used in Body to store the compiled address of the handler in the
 * exception table.
 */
static bool_t
IndexExceptionHandlers(CompEnv *ce)
{
    struct CatchFrame *cf = ce->mb->exception_table;
    long cnt = ce->mb->exception_table_length;
    long i;
    RangeHdr *rh;

    for (i = 0; i < cnt; i++, cf += 1)
    {
	rh = ce->rp[cf->handler_pc].hdr;
	rh->xIndex = i;
	rh->topOnEntry = 1;
	rh->topOnExit++;
	WalkControlFlow(rh, FREQ_UNIT_LEV);
	WalkXHControlFlow(ce, rh);
    }
    return TRUE;
}


/* Variables that cannot be allocated in a register because they are accessed
 * as float, or double (long OK), or because they are used in an exception
 * handler are marked by a non-zero offset and their varFreq is reset to 0.
 * This function allocates mostly used variables in registers.
 * Theory:
 * We assume that a memory access costs 3 register accesses.
 * The cost of allocating a parameter in a register is equivalent to 3 memory
 * accesses: save the callee-saved reg, load the parameter (passed on the
 * stack), restore the reg. Therefore, a parameter must be used more than 4.5
 * times to be allocated in a register: 3 + n/3 < n => n > 4.5
 * The cost of allocating a local variable in a reg is equivalent to 2 memory
 * accesses (save + restore).
 * Therefore, a local variable must be used more than 3 times to be
 * allocated in a register: 2 + n/3 = n => n > 3
 * Well, theory does not work on a Pentium (better on an 486).
 * Experience shows better results on Pentium with the following code:
 */
static bool_t
AllocVariables(CompEnv *ce)
{
    struct methodblock *mb = ce->mb;
    int     nargs = mb->args_size;
    int     nlocals = mb->nlocals;
    int     var, maxVar, nRegVCand;
    ModReg  reg;
    long    *voffp, off;
    long    frameSize;
    long    *vfreqp, maxFreq;

    /* Count the number of qualifying locals and reset varFreq of locals
     * marked with varOff == 1 and of locals with a too low freq
     */
    nRegVCand = nlocals;
    /* Visit parameters first */
    for (var = nargs, voffp = &ce->varOff[0], vfreqp = &ce->varFreq[0];
	 --var >= 0; voffp++, vfreqp++)
    {
    	if (*voffp || *vfreqp <= FREQ_UNIT)
	{
	    *vfreqp = 0;
	    nRegVCand--;
	}
	else
	    sysAssert(*vfreqp > 0);
    }
    /* Visit local variables next */
    for (var = nlocals - nargs; --var >= 0; voffp++, vfreqp++)
    {
    	if (*voffp || *vfreqp <= 0)
	{
	    *vfreqp = 0;
	    nRegVCand--;
	}
	else
	    sysAssert(*vfreqp > 0);
    }
    if (nRegVCand <= MR_ESP - MR_EBX)
    {
    	/* enough regs for everybody */
	reg = MR_EBX;
	for (var = nlocals, voffp = &ce->varOff[0], vfreqp = &ce->varFreq[0];
	     --var >= 0; voffp++, vfreqp++)
	    if (*vfreqp)
		*voffp = -reg++;
    }
    else
    {
	/* allocate mostly used variables in registers */
	for (reg = MR_EBX; reg < MR_ESP && nRegVCand > 0; reg++)
	{
	    maxFreq = 0;
	    maxVar = -1;
	    for (var = 0, vfreqp = &ce->varFreq[0]; var < nlocals; var++, vfreqp++)
	    {
		if (*vfreqp > maxFreq)
		{
		    maxFreq = *vfreqp;
		    maxVar = var;
		}
	    }
	    sysAssert(maxVar >= 0 && maxVar < nlocals);
	    ce->varFreq[maxVar] = 0;
	    ce->varOff[maxVar] = -reg;
	    nRegVCand--;
	}
    }
    /* reg is live */

    /* allocate remaining arguments in memory */
    off = 0;
    for (var = nargs, voffp = &ce->varOff[0]; --var >= 0; voffp++)
    {
	if (*voffp >= 0)	// 0 cannot indicate a reg (MR_EAX not used here)
	    *voffp = off;	// allocate in memory
	off += 4;
    }

    /* allocate remaining locals relative to esp */
    off = ce->codeInfo->baseOff + 4*mb->maxstack;
    for (var = nlocals - nargs, voffp = &ce->varOff[nargs]; --var >= 0; voffp++)
    {
	if  (*voffp >= 0)	// 0 cannot indicate a reg (MR_EAX not used here)
	{
	    *voffp = off;	// allocate in memory
	    off += 4;
	}
    }

    if (mb->exception_table_length > 0 || (mb->fb.access & ACC_SYNCHRONIZED))
	frameSize = FRAME_WITH_XHANDLER_SIZE;
    else
	frameSize = 4*(reg - MR_EBX) + 4;   // callee-saved and return address

#ifdef IN_NETSCAPE_NAVIGATOR
    off += 4;
#endif
    ce->codeInfo->localSize = off;
    ce->codeInfo->frameSize = frameSize;

    /* adjust offset of arguments relative to esp */
    off += frameSize;
    for (var = nargs, voffp = &ce->varOff[0]; --var >= 0; voffp++)
	if  (*voffp >= 0)
	    *voffp += off;	// allocated in memory

    return TRUE;
}


static bool_t
Prologue(CompEnv *ce)
{
    struct methodblock *mb = ce->mb;
    ModReg  reg;
    int     nSaved, var;
    int     nargs = mb->args_size;
    long    *voffp, off, xctxt;
    ModReg  mr;
    Item    arg;

    AllocContext(ce, &ce->ctxt, 0);
#ifdef	DEBUG
    GenByteLong(ce, TESTI_AL + 1, (long)mb); // to see mb value in disassembly
#endif
    if (mb->exception_table_length > 0 || (mb->fb.access & ACC_SYNCHRONIZED))
    {
/* method prologue (with exception handling, or synchronized):
    push    ebx
    push    esi
    push    edi
    xor     eax, eax
    push    ebp
    push    offset HandleException
    push    dword ptr fs:[eax]
    mov     dword ptr fs:[eax], esp
    push    xctxt		// xctxt of first opcode
    push    edx 		// mb parameter

    sub     esp, localSize
or
    push    0			// for annotation in Netscape Navigator
    sub     esp, localSize-4

    esp has to point to top of operand stack which may be trashed
    by native call results (this is safe)
*/
	ce->codeInfo->xframe2esp = -ce->codeInfo->localSize - 8;
	GenByte(ce, PUSHR_L + RC_EBX);
	GenByte(ce, PUSHR_L + RC_ESI);
	GenByte(ce, PUSHR_L + RC_EDI);
	GenOpRegReg(ce, XORR_L, MR_EAX, MR_EAX);
	GenByte(ce, PUSHR_L + RC_EBP);
	GenByteLong(ce, PUSHI_L, (long)HandleException);
	GenByte(ce, FSSEG); GenIndirEA(ce, 0xFF, PUSH_L, MR_EAX, 0);
	GenByte(ce, FSSEG); GenIndirEA(ce, STORE_L, reg3Map[MR_ESP], MR_EAX, 0);
	xctxt = ce->rp[0].hdr->xctxt;
	if (xctxt == 0)
	    GenByte(ce, PUSHR_L + RC_EAX);
        else
	    GenByteLong(ce, PUSHI_L, xctxt);
	GenByte(ce, PUSHR_L + RC_EDX);

#ifdef IN_NETSCAPE_NAVIGATOR
	GenByte(ce, PUSHR_L + RC_EAX);
#endif
    }
    else
    {
/* method prologue (without exception handling, and not synchronized):
    push    callee-saved    // as used
    push    edx 	    // if mb used (never if jit compilation, see below)

    sub     esp, localSize
or
    push    0			// for annotation in Netscape Navigator
    sub     esp, localSize-4
*/
    /* Actually, mb is never saved since its value is known at compile time and
     * can be inserted in the code wherever needed as an immediate value.
     * This only works with a just-in-time compiler.
     */
	ce->codeInfo->xframe2esp = 0;	// no xframe
	for (nSaved = (ce->codeInfo->frameSize - 4) >> 2, reg = MR_EBX; --nSaved >= 0; )
	    GenByte(ce, PUSHR_L + regMap[reg++]);

#ifdef IN_NETSCAPE_NAVIGATOR
	Gen2Bytes(ce, PUSHI_B, 0);
#endif
    }
    off = ce->codeInfo->localSize;
#ifdef IN_NETSCAPE_NAVIGATOR
    off -= 4;
#endif
    GenAddImmRL(ce, MR_ESP, -off);

    /* load parameters into their permanent registers */
    arg.mr = MR_BASE + MR_ESP;
    arg.offset = ce->codeInfo->localSize + ce->codeInfo->frameSize;
    arg.size = 4;
    for (var = nargs, voffp = &ce->varOff[0]; --var >= 0; voffp++)
    {
	mr = -*voffp;
	if  (mr > 0)	    // allocated in register
	{
	    GenByte(ce, LOAD_L);
	    GenEA(ce, reg3Map[mr], &arg);
	}
	arg.offset += 4;
    }

    if (mb->fb.access & ACC_SYNCHRONIZED)
    {
	if (mb->fb.access & ACC_STATIC)
	    // CompSupport_monitorenter(cbHandle(fieldclass(&mb->fb)));
	    PushImm(ce, (long)(cbHandle(fieldclass(&mb->fb))));
	else
	{
	    // CompSupport_monitorenter(this);
	    off = ce->varOff[0];
	    if (off < 0)
		arg.mr = -off;
	    else
	    // no var reg if used in exception handler
	    {
		arg.mr = MR_BASE + MR_ESP;
		arg.offset = off;
	    }
	    arg.size = 4;
	    Push(ce, &arg);
	}
	CallCompSupport(ce, (char *)CompSupport_monitorenter,
	    ce->ctxt->top, 4);
    }
    ce->rp[0].hdr->ctxt = ce->ctxt; // for Body
    ce->ctxt = 0;
    return TRUE;
}


static bool_t
Epilogue(CompEnv *ce)
{
    struct methodblock *mb = ce->mb;
    ModReg	reg;
    int 	nSaved;
    Item	*top, arg;
    long	off;
    RangeHdr	*rh = ce->epilogueRh;
    Label	a, b, h;

    if (!(rh->rangeFlags & RF_VISITED))
	return TRUE;	    // method is an endless loop, no epilogue necessary

    ce->ctxt = rh->ctxt;
    ce->xctxt = -1; // not used anyway
    sysAssert(rh->xctxt == -1);
    rh->ctxt = 0;
    FlushVarCache(ce->ctxt);
    top = ce->ctxt->top;
    a = rh->link;
    b = rh->xlink;  // keep the exception context of opc_return
    // fix shortest dist (i.e. higher link) first, because of jump optimization
    if (b > a) { h = a; a = b; b = h; }
    FixLink(ce, a);
    FixLink(ce, b);

    if (mb->fb.access & ACC_SYNCHRONIZED)
    {
    	PushImm(ce, ce->codeInfo->localSize + 3*4);	// obj, obj2xctxt, mb
	if (mb->fb.access & ACC_STATIC)
	    // CompSupport_monitorexit(cbHandle(fieldclass(&mb->fb)), obj2xctxt);
	    PushImm(ce, (long)(cbHandle(fieldclass(&mb->fb))));
	else
	{
	    // CompSupport_monitorexit(this, obj2xctxt);
	    if (mb->exception_table_length > 0)
    		// this register can be trashed
    		off = ce->codeInfo->localSize + ce->codeInfo->frameSize;
	    else
		off = ce->varOff[0];

	    if (off < 0)
		arg.mr = -off;
	    else
	    // no var reg if used in exception handler
	    {
		arg.mr = MR_BASE + MR_ESP;
		arg.offset = off;
	    }
	    arg.size = 4;
	    Push(ce, &arg);
	}
	CallCompSupport(ce, (char *)CompSupport_monitorexit, top, 8);

	switch  (mb->CompiledCodeFlags & 7)
	{
	case 0: // int
	    LoadOpnd(ce, top - 1, RS_EAX);
	    break;

	case 1: // long
	    sysAssert((regSetOfMr[(top - 1)->mr] & RS_EAX) == RS_EMPTY);
	    LoadOpnd(ce, top - 2, RS_EAX);
	    LoadOpnd(ce, top - 1, RS_EDX);
	    break;

	case 2: // float
	    LoadFOpnd(ce, top - 1, 4);
	    break;

	case 3: // double
	    LoadFOpnd(ce, top - 2, 8);
	    break;
	}
    }
    // else result is already loaded

    if (ce->codeInfo->frameSize == FRAME_WITH_XHANDLER_SIZE)
    {
/* method epilogue (with exception handling, or synchronized):
    add     esp, -xframe2esp
    pop     dword ptr fs:[0]
    pop     ecx
    pop     ebp
    pop     edi
    pop     esi
    pop     ebx
    ret     4*nargs
*/
	GenAddImmRL(ce, MR_ESP, ce->codeInfo->localSize + 8);
	GenByte(ce, FSSEG); Gen2BytesLong(ce, 0x8F, POP_L + 0x05, 0);
	GenByte(ce, POPR_L + RC_ECX);
	GenByte(ce, POPR_L + RC_EBP);
	GenByte(ce, POPR_L + RC_EDI);
	GenByte(ce, POPR_L + RC_ESI);
	GenByte(ce, POPR_L + RC_EBX);
    }
    else
    {
/*
method epilogue (without exception handling, and not synchronized):
    add     esp, localSize  // (+4 if mb used) if non-zero
    pop     callee-saved    // as used
    ret     4*nargs
*/
	GenAddImmRL(ce, MR_ESP, ce->codeInfo->localSize);

	for (nSaved = (ce->codeInfo->frameSize - 4) >> 2, reg = MR_EBX + nSaved;
		--nSaved >= 0; )
	    GenByte(ce, POPR_L + regMap[--reg]);
    }
    if (mb->args_size)
    {
	GenByte(ce, RETP); GenWord(ce, 4*mb->args_size);
    }
    else
	GenByte(ce, RET);

    return TRUE;
}


static bool_t
Body(CompEnv *ce)
{
#define SIZE_AND_STACK(size, stack) pc += size; top += stack; continue
#define SIZE_AND_STACK_BUT(size, stack) pc += size; top += stack
    struct methodblock *mb = ce->mb;
    struct execenv *ee = ce->ee;
    ClassClass *cb = fieldclass(&mb->fb);
    union cp_item_type *cpool = cbConstantPool(cb);
    unsigned char *type_table = cpool[CONSTANT_POOL_TYPE_TABLE_INDEX].p;
    struct CatchFrame *cf = mb->exception_table;
    struct fieldblock *fb;
    struct methodblock *mb_type;
    char	    isig;
    unsigned char   *initial_pc = mb->code;
    unsigned char   *max_pc = initial_pc + mb->code_length;
    unsigned char   *pc, *nextRhPc;
    RangeHdr	    *rh, *nextRh, *targRh, *contRh;
    int 	    var, clone, push, dimensions;
    unsigned	    index, type;
    long	    off;
    unsigned char   opcode;
    Item	    *top, arg;

    top = NULL;
    rh = NULL;
    nextRh = ce->rp[0].hdr;
    nextRhPc = initial_pc;

    pc = initial_pc;
    while (pc <= max_pc)
    // instead of pc < max_pc: finalize the last range before the epilogue
    {
	if  (ce->pc >= ce->pcRedZone)
	{
	    ce->err = pcInRedZone;
	    return FALSE;
	}

	if  (pc >= nextRhPc)
	{
	    // a new range starts here
	    sysAssert(pc == nextRhPc);
	    if (rh)
	    {
		// finalize the current range
		ce->ctxt->top = top;
		targRh = rh->targRh;
		contRh = rh->contRh;
		clone = 0;
		if  (targRh != 0)
		{
		    // no need to flush the opstack, it was done before jumping
		    // to the target (if targRh->joinCnt > 0)
		    if (targRh->pc == 0 && targRh->ctxt == 0)
		    {
			// forward target, context not set yet
			if  (!CopyContext(ce, targRh, &clone))
			    return FALSE;

			sysAssert(targRh->joinCnt == 0
                                  || (targRh->ctxt->regRefCnt[MR_EAX] == 0
				      && targRh->ctxt->regRefCnt[MR_EDX] == 0
				      && targRh->ctxt->regRefCnt[MR_ECX] == 0));
		    }
		}
		if  (contRh != 0)
		{
		    if (contRh->joinCnt > 0)
			FlushOpStack(ce, top);

		    if (contRh->ctxt == 0)
		    {
			// (always forward) continuation context not set yet
			if  (!CopyContext(ce, contRh, &clone))
			    return FALSE;
		    }
		}
		/* does not really speed up loops
		else
		{
    		    while ((long)ce->pc & 15)
    			GenByte(ce, 0x90);	// align loops on a 16-byte boundaries
		}
		*/
		if  (clone == 0)
		    FreeContext(ce->ctxt);
		ce->ctxt = 0;
	    }
	    rh = nextRh;
	    if (rh == 0)
		// we are done
		return TRUE;

	    nextRh = rh->nextRh;
	    if (nextRh == NULL)
		nextRhPc = max_pc;
	    else
		nextRhPc = initial_pc + nextRh->bcpc;

	    sysAssert((rh->rangeFlags & RF_DATABLOCK) == 0);
	    sysAssert(rh->rangeFlags & RF_VISITED);
	    ce->ctxt = rh->ctxt;
	    rh->ctxt = 0;
	    if (ce->ctxt == 0)
	    {
		// this range is the target of one or several backward jumps or
		// the range of an exception handler:
		// the context is still unknown, except for the opstack height
		// which was computed in the first pass. We assume all operands
		// are flushed (by Jcc if backwards jump).
		if  (!AllocContext(ce, &ce->ctxt, rh->topOnEntry))
		    return FALSE;
	    }
	    else
	    {
		// this range is the target of at least one forward jump or
		// is a continuation range
		if  (rh->joinCnt > 0)
		{
		    // opstack is flushed at jump point or in previous range
		    FlushVarCache(ce->ctxt);
		    sysAssert(ce->ctxt->regRefCnt[MR_EAX] == 0
			      && ce->ctxt->regRefCnt[MR_EDX] == 0
			      && ce->ctxt->regRefCnt[MR_ECX] == 0);
		}
	    }

	    ce->xctxt = rh->xctxt;
	    top = ce->ctxt->top;
	    sysAssert(top == ce->ctxt->base + rh->topOnEntry);

	    if (rh->rangeFlags & RF_XHANDLER)
	    {
		(top - 1)->mr = MR_EAX;
		(top - 1)->size = 4;
		(top - 1)->var = NOT_A_VAR;
                ce->ctxt->regRefCnt[MR_EAX] = 1;
                ce->ctxt->regRefCnt[MR_EDX] = 0;
                ce->ctxt->regRefCnt[MR_ECX] = 0;
		(cf + rh->xIndex)->compiled_CatchFrame =
		    (void *)(ce->pc - ce->compiledCode);
		// compiled_CatchFrame is actually the relative
		// address of the compiled handler
	    }
	    if (rh->rangeFlags & RF_SET_XCTXT)
	    {
		FixLink(ce, rh->xlink);
		rh->xpc = ce->pc;
		if (*pc == opc_return
		    // only opcode in this range
		    || (*pc == opc_pop && pc[1] == opc_return))
		    // only 2 opcodes in this xhandler
    		    ;	// optimization: not necessary to set xctxt
		else
		{
		    if (rh->rangeFlags & RF_JSR)
			off = 4; // for the return address, espLevel not corrected
		    else
			off = 0;

		    GenIndirEA(ce, MOVI_L, 0, MR_ESP,
			ce->codeInfo->localSize + 4 - ce->ctxt->espLevel + off);
		    GenLong(ce, rh->xctxt);
		}
	    }
	    FixLink(ce, rh->link);
	    rh->pc = ce->pc;
	    if (rh->rangeFlags & RF_JSR)
	    {
    		// continue and break statements are allowed in finally,
		// so we have to restore espLevel by popping the ret addr
		sysAssert(ce->ctxt->espLevel == 0);
		switch  (*pc)
		{
		case opc_astore:
		    var = pc[1];
		    pc += 2;
		    break;
	
		case opc_astore_0:
		    var = 0;
		    pc++;
		    break;
	
		case opc_astore_1:
		    var = 1;
		    pc++;
		    break;
	
		case opc_astore_2:
		    var = 2;
		    pc++;
		    break;
	
		case opc_astore_3:
		    var = 3;
		    pc++;
		    break;
	
                case opc_wide:
    		    if (pc[1] == opc_astore)
		    {
			var = GET_INDEX(pc + 2);
			pc += 4;
			break;
                    }
		    // fall thru
		default:
    		    var = -1;
		}
		if (var != -1)
		{
		    off = ce->varOff[var];
		    if (off < 0)
                        GenByte(ce, POPR_L + regMap[-off]);
                    else
		    {
			arg.size = 4;
			arg.mr = MR_BASE + MR_ESP;
			arg.offset = off;
			GenByte(ce, 0x8F);
			GenEA(ce, POP_L, &arg); // esp += 4; [esp+off] = [esp-4];
                    }
		    if  (pc >= nextRhPc)
			// next opcode belongs to the next range
			continue;
		}
		else
		{
    		    top++; // top was not incremented for the return address at opc_jsr
		    (top - 1)->size = 4;
		    (top - 1)->mr = MR_BASE + MR_ESP;
		    (top - 1)->offset = OpndOffset(ce, top - 1);
		    GenByte(ce, 0x8F);
		    GenEA(ce, POP_L, top - 1); // esp += 4; [esp+off] = [esp-4];
                }
	    }
	}

	check((unsigned)(top - ce->ctxt->base) <= mb->maxstack);

	ce->ctxt->top = top;
	opcode = *pc;
	switch (opcode)
	{
	case opc_nop:
	    SIZE_AND_STACK(1, 0);

#define DO_LOAD_DWORD(top, num) 					       \
            off = ce->varOff[num];                                             \
            if  (off < 0)                                                      \
                (top)->mr = -off;                                              \
            else                                                               \
            {                                                                  \
                (top)->mr = MR_BASE + MR_ESP;                                  \
                (top)->offset = off;                                           \
            }                                                                  \
            (top)->size = 4;                                                   \
            (top)->var = num;

#define DO_LOAD_QWORD(top, num) 					       \
            DO_LOAD_DWORD(top + 0, num + 0)                                    \
            DO_LOAD_DWORD(top + 1, num + 1)

	case opc_aload:
	case opc_iload:
	case opc_fload:
	    var = pc[1];
	    DO_LOAD_DWORD(top, var)
	    SIZE_AND_STACK(2, 1);

	case opc_lload:
	case opc_dload:
	    var = pc[1];
	    DO_LOAD_QWORD(top, var)
	    SIZE_AND_STACK(2, 2);

#define OPC_DO_LOAD_DWORD_n(num)					       \
        case opc_iload_##num:                                                  \
        case opc_aload_##num:                                                  \
        case opc_fload_##num:                                                  \
            DO_LOAD_DWORD(top, num)                                            \
            SIZE_AND_STACK(1, 1);

#define OPC_DO_LOAD_QWORD_n(num)					       \
        case opc_lload_##num:                                                  \
        case opc_dload_##num:                                                  \
            DO_LOAD_QWORD(top, num)                                            \
            SIZE_AND_STACK(1, 2);

	OPC_DO_LOAD_DWORD_n(0)
	OPC_DO_LOAD_DWORD_n(1)
	OPC_DO_LOAD_DWORD_n(2)
	OPC_DO_LOAD_DWORD_n(3)

	OPC_DO_LOAD_QWORD_n(0)
	OPC_DO_LOAD_QWORD_n(1)
	OPC_DO_LOAD_QWORD_n(2)
	OPC_DO_LOAD_QWORD_n(3)

	case opc_istore:
	case opc_astore:
	    StoreOpnd(ce, pc[1], top - 1);
	    SIZE_AND_STACK(2, -1);

	case opc_fstore:
	    StoreFOpnd(ce, pc[1], top - 1, 4);
	    SIZE_AND_STACK(2, -1);

	case opc_lstore:
	    StoreOpnd(ce, pc[1] + 1, top - 1);	// high first for anti-aliasing
	    StoreOpnd(ce, pc[1] + 0, top - 2);
	    SIZE_AND_STACK(2, -2);

	case opc_dstore:
	    StoreFOpnd(ce, pc[1], top - 2, 8);
	    SIZE_AND_STACK(2, -2);

#define OPC_DO_STORE_DWORD_n(num)					       \
        case opc_istore_##num:                                                 \
        case opc_astore_##num:                                                 \
            StoreOpnd(ce, num, top - 1);                                       \
            SIZE_AND_STACK(1, -1);

#define OPC_DO_STORE_LONG_n(num)					       \
        case opc_lstore_##num:                                                 \
            StoreOpnd(ce, num + 1, top - 1);                                   \
            StoreOpnd(ce, num + 0, top - 2);                                   \
            SIZE_AND_STACK(1, -2);

#define OPC_DO_STORE_FLOAT_n(num)					       \
        case opc_fstore_##num:                                                 \
            StoreFOpnd(ce, num, top - 1, 4);                                   \
            SIZE_AND_STACK(1, -1);

#define OPC_DO_STORE_DOUBLE_n(num)					       \
        case opc_dstore_##num:                                                 \
            StoreFOpnd(ce, num, top - 2, 8);                                   \
            SIZE_AND_STACK(1, -2);

	OPC_DO_STORE_DWORD_n(0)
	OPC_DO_STORE_DWORD_n(1)
	OPC_DO_STORE_DWORD_n(2)
	OPC_DO_STORE_DWORD_n(3)

	OPC_DO_STORE_LONG_n(0)
	OPC_DO_STORE_LONG_n(1)
	OPC_DO_STORE_LONG_n(2)
	OPC_DO_STORE_LONG_n(3)

	OPC_DO_STORE_FLOAT_n(0)
	OPC_DO_STORE_FLOAT_n(1)
	OPC_DO_STORE_FLOAT_n(2)
	OPC_DO_STORE_FLOAT_n(3)

	OPC_DO_STORE_DOUBLE_n(0)
	OPC_DO_STORE_DOUBLE_n(1)
	OPC_DO_STORE_DOUBLE_n(2)
	OPC_DO_STORE_DOUBLE_n(3)

	case opc_return:
	case opc_areturn:
	case opc_ireturn:
	case opc_lreturn:
	case opc_freturn:
	case opc_dreturn:
	    /* The following code avoids to flush the return value,
	     * epilogueRh->jointCnt is always 0 now
	     */
	    if (mb->fb.access & ACC_SYNCHRONIZED)
                FlushOpStack(ce, top);
	    else
	    {
		switch  (mb->CompiledCodeFlags & 7)
		{
		case 0: // int
		    LoadOpnd(ce, top - 1, RS_EAX);
		    break;
	
		case 1: // long
		    if ((regSetOfMr[(top - 1)->mr] & RS_EAX) == RS_EMPTY)
		    {
			LoadOpnd(ce, top - 2, RS_EAX);
			LoadOpnd(ce, top - 1, RS_EDX);
		    }
		    else if ((regSetOfMr[(top - 2)->mr] & RS_EDX) == RS_EMPTY)
		    {
			LoadOpnd(ce, top - 1, RS_EDX);
			LoadOpnd(ce, top - 2, RS_EAX);
		    }
		    else
		    {
			FlushOpnd(ce, top - 2);
			FlushOpnd(ce, top - 1);
			LoadOpnd(ce, top - 2, RS_EAX);
			LoadOpnd(ce, top - 1, RS_EDX);
		    }
		    break;
	
		case 2: // float
		    LoadFOpnd(ce, top - 1, 4);
		    break;
	
		case 3: // double
		    LoadFOpnd(ce, top - 2, 8);
		    break;
		}
	    }
	    GenByte(ce, 0xE9);
	    Link(ce, &ce->epilogueRh->link);
	    SIZE_AND_STACK(1, 0);

	case opc_i2f:
	    MakeFOpndAccessible(ce, top - 1, 4);
	    GenByte(ce, 0xDB);
	    GenEA(ce, FILD, top - 1);
	    FreeRegs(ce, (top - 1)->mr);
	    (top - 1)->mr = MR_FST;
	    (top - 1)->size = 4;
	    (top - 1)->var = NOT_A_VAR;
#ifdef	STRICT_FLOATING
	    FlushOpnd(ce, top - 1);
#endif
	    SIZE_AND_STACK(1, 0);

	case opc_i2l:
	    LoadOpnd(ce, top - 1, RS_EAX);
	    (top - 0)->size = 4;
	    (top - 0)->mr = GetReg(ce, RS_EDX);
	    (top - 0)->var = NOT_A_VAR;
	    GenByte(ce, CDQ);
	    SIZE_AND_STACK(1, 1);

	case opc_i2d:
	    MakeFOpndAccessible(ce, top - 1, 4);
	    GenByte(ce, 0xDB);
	    GenEA(ce, FILD, top - 1);
	    FreeRegs(ce, (top - 1)->mr);
	    (top - 1)->mr = MR_FST;
	    (top - 0)->mr = MR_FST;
	    (top - 1)->size = 8;
	    (top - 0)->size = 0;
	    (top - 1)->var = NOT_A_VAR;
	    (top - 0)->var = NOT_A_VAR;
#ifdef	STRICT_FLOATING
	    ce->ctxt->top = top + 1;	// for sysAssert in FlushOpnd
	    FlushOpnd(ce, top - 1);
	    FlushOpnd(ce, top - 0);
#endif
	    SIZE_AND_STACK(1, 1);

	case opc_f2i:
	    PushFOpnd(ce, top - 1, 4, 4);
	    CallCompSupport(ce, (char *)CompSupport_f2i, top - 1, 4);
	    (top - 1)->mr = GetReg(ce, RS_EAX);
	    (top - 1)->size = 4;
	    (top - 1)->var = NOT_A_VAR;
	    SIZE_AND_STACK(1, 0);

	case opc_f2l:
	    PushFOpnd(ce, top - 1, 4, 4);
	    CallCompSupport(ce, (char *)CompSupport_f2l, top - 1, 4);
	    (top - 1)->mr = GetReg(ce, RS_EAX);
	    (top - 0)->mr = GetReg(ce, RS_EDX);
	    (top - 1)->size = 4;
	    (top - 0)->size = 4;
	    (top - 1)->var = NOT_A_VAR;
	    (top - 0)->var = NOT_A_VAR;
	    SIZE_AND_STACK(1, 1);

	case opc_f2d:
	    LoadFOpnd(ce, top - 1, 4);
	    (top - 1)->size = 8;
	    (top - 1)->var = NOT_A_VAR;
	    (top - 0)->mr = MR_FST;
	    (top - 0)->size = 0;
	    (top - 0)->var = NOT_A_VAR;
	    SIZE_AND_STACK(1, 1);

	case opc_l2i:
	    FreeRegs(ce, (top - 1)->mr);
	    (top - 2)->var = NOT_A_VAR;
	    SIZE_AND_STACK(1, -1);

	case opc_l2f:
	    MakeFOpndAccessible(ce, top - 2, 8);
	    GenByte(ce, 0xDF);
	    GenEA(ce, FILD_Q, top - 2);
	    FreeRegs(ce, (top - 2)->mr);
	    FreeRegs(ce, (top - 1)->mr);
	    (top - 2)->mr = MR_FST;
	    (top - 2)->size = 4;
	    (top - 2)->var = NOT_A_VAR;
#ifdef	STRICT_FLOATING
	    FlushOpnd(ce, top - 2);
#endif
	    SIZE_AND_STACK(1, -1);

	case opc_l2d:
	    MakeFOpndAccessible(ce, top - 2, 8);
	    GenByte(ce, 0xDF);
	    GenEA(ce, FILD_Q, top - 2);
	    FreeRegs(ce, (top - 2)->mr);
	    FreeRegs(ce, (top - 1)->mr);
	    (top - 2)->mr = MR_FST;
	    (top - 2)->size = 8;
	    (top - 2)->var = NOT_A_VAR;
	    (top - 1)->mr = MR_FST;
	    (top - 1)->size = 0;
	    (top - 1)->var = NOT_A_VAR;
#ifdef	STRICT_FLOATING
	    FlushOpnd(ce, top - 2);
	    FlushOpnd(ce, top - 1);
#endif
	    SIZE_AND_STACK(1, 0);

	case opc_d2i:
	    PushFOpnd(ce, top - 2, 8, 8);
	    CallCompSupport(ce, (char *)CompSupport_d2i, top - 2, 8);
	    (top - 2)->mr = GetReg(ce, RS_EAX);
	    (top - 2)->size = 4;
	    (top - 2)->var = NOT_A_VAR;
	    SIZE_AND_STACK(1, -1);

	case opc_d2f:
	    LoadFOpnd(ce, top - 2, 8);
	    (top - 2)->size = 4;
	    (top - 2)->var = NOT_A_VAR;
#ifdef	STRICT_FLOATING
	    (top - 1)->mr = MR_IMM;	// for debug checking in FlushOpnd
	    FlushOpnd(ce, top - 2);
#endif
	    SIZE_AND_STACK(1, -1);

	case opc_d2l:
	    PushFOpnd(ce, top - 2, 8, 8);
	    CallCompSupport(ce, (char *)CompSupport_d2l, top - 2, 8);
	    (top - 2)->mr = GetReg(ce, RS_EAX);
	    (top - 1)->mr = GetReg(ce, RS_EDX);
	    (top - 2)->size = 4;
	    (top - 1)->size = 4;
	    (top - 2)->var = NOT_A_VAR;
	    (top - 1)->var = NOT_A_VAR;
	    SIZE_AND_STACK(1, 0);

	case opc_int2byte:
	    Int2BW(ce, top - 1, 1, 0xBE, pc + 1);   // movsx erx, byte
	    SIZE_AND_STACK(1, 0);

	case opc_int2char:
	    Int2BW(ce, top - 1, 2, 0xB7, pc + 1);   // movzx erx, word
	    SIZE_AND_STACK(1, 0);

	case opc_int2short:
	    Int2BW(ce, top - 1, 2, 0xBF, pc + 1);   // movsx erx, word
	    SIZE_AND_STACK(1, 0);

	case opc_fcmpl:
	case opc_fcmpg:
	    FCompare(ce, top - 2, top - 1, 4, opcode == opc_fcmpl ? -1 : 1,
		pc + 1);
	    SIZE_AND_STACK(1, -1);

	case opc_lcmp:
	    LCompare(ce, top - 4, top - 2, pc + 1);
	    SIZE_AND_STACK(1, -3);

	case opc_dcmpl:
	case opc_dcmpg:
	    FCompare(ce, top - 4, top - 2, 8, opcode == opc_dcmpl ? -1 : 1,
		pc + 1);
	    SIZE_AND_STACK(1, -3);

	case opc_bipush:
	    top->mr = MR_IMM;
	    top->offset = (signed char)(pc[1]);
	    top->size = 4;
	    top->var = NOT_A_VAR;
	    SIZE_AND_STACK(2, 1);

	case opc_sipush:
	    top->mr = MR_IMM;
	    top->offset = pc2signedshort(pc);
	    top->size = 4;
	    top->var = NOT_A_VAR;
	    SIZE_AND_STACK(3, 1);

	case opc_ldc:
	case opc_ldc_quick:
	    index = pc[1];
	    type = CONSTANT_POOL_TYPE_TABLE_GET_TYPE(type_table, index);
	    if (type == CONSTANT_Float)
	    {
		top->mr = MR_ABS;
		top->offset = (long)(&cpool[index].p);
	    }
	    else
	    {
		top->mr = MR_IMM;
		top->offset = (long)cpool[index].p;
	    }
	    top->size = 4;
	    top->var = NOT_A_VAR;
	    SIZE_AND_STACK(2, 1);

	case opc_ldc_w:
	case opc_ldc_w_quick:
	    index = GET_INDEX(pc + 1);
	    type = CONSTANT_POOL_TYPE_TABLE_GET_TYPE(type_table, index);
	    if (type == CONSTANT_Float)
	    {
		top->mr = MR_ABS;
		top->offset = (long)(&cpool[index].p);
	    }
	    else
	    {
		top->mr = MR_IMM;
		top->offset = (long)cpool[index].p;
	    }
	    top->size = 4;
	    top->var = NOT_A_VAR;
	    SIZE_AND_STACK(3, 1);

	case opc_ldc2_w:
	case opc_ldc2_w_quick:
	    index = GET_INDEX(pc + 1);
	    type = CONSTANT_POOL_TYPE_TABLE_GET_TYPE(type_table, index);
	    if (type == CONSTANT_Double)
	    {
		(top + 0)->mr = MR_ABS;
		(top + 0)->offset = (long)(&cpool[index].p);
		(top + 1)->mr = MR_ABS;
		(top + 1)->offset = (long)(&cpool[index + 1].p);
	    }
	    else
	    {
		(top + 0)->mr = MR_IMM;
		(top + 0)->offset = (long)cpool[index].p;
		(top + 1)->mr = MR_IMM;
		(top + 1)->offset = (long)cpool[index + 1].p;
	    }
	    (top + 0)->size = 4;
	    (top + 0)->var = NOT_A_VAR;
	    (top + 1)->size = 4;
	    (top + 1)->var = NOT_A_VAR;
	    SIZE_AND_STACK(3, 2);

#define CONSTANT_OP(opcode, value)					       \
        case opcode:                                                           \
            top->mr = MR_IMM;                                                  \
            top->offset = value;                                               \
            top->size = 4;                                                     \
            top->var = NOT_A_VAR;                                              \
            SIZE_AND_STACK(1, 1);

	CONSTANT_OP(opc_aconst_null,	0)
	CONSTANT_OP(opc_iconst_m1,     -1)
	CONSTANT_OP(opc_iconst_0,	0)
	CONSTANT_OP(opc_iconst_1,	1)
	CONSTANT_OP(opc_iconst_2,	2)
	CONSTANT_OP(opc_iconst_3,	3)
	CONSTANT_OP(opc_iconst_4,	4)
	CONSTANT_OP(opc_iconst_5,	5)

#define FCONSTANT_OP(opcode, name)					       \
        case opcode:                                                           \
            top->mr = MR_ABS;                                                  \
            top->offset = (long)(&name);                                       \
            top->size = 4;                                                     \
            top->var = NOT_A_VAR;                                              \
            SIZE_AND_STACK(1, 1);

	FCONSTANT_OP(opc_fconst_0, CompSupport_fconst_0)
	FCONSTANT_OP(opc_fconst_1, CompSupport_fconst_1)
	FCONSTANT_OP(opc_fconst_2, CompSupport_fconst_2)

#define DCONSTANT_OP(opcode, name)					       \
        case opcode:                                                           \
            (top + 0)->mr = MR_ABS;                                            \
            (top + 0)->offset = (long)(&name);                                 \
            (top + 0)->size = 4;                                               \
            (top + 0)->var = NOT_A_VAR;                                        \
            (top + 1)->mr = MR_ABS;                                            \
            (top + 1)->offset = (long)(&name) + 4;                             \
            (top + 1)->size = 4;                                               \
            (top + 1)->var = NOT_A_VAR;                                        \
            SIZE_AND_STACK(1, 2);

	DCONSTANT_OP(opc_dconst_0, CompSupport_dconst_0)
	DCONSTANT_OP(opc_dconst_1, CompSupport_dconst_1)

#define LCONSTANT_OP(opcode, value)					       \
        case opcode:                                                           \
            (top + 0)->mr = MR_IMM;                                            \
            (top + 0)->offset = value; /* little endian! */                    \
            (top + 0)->size = 4;                                               \
            (top + 0)->var = NOT_A_VAR;                                        \
            (top + 1)->mr = MR_IMM;                                            \
            (top + 1)->offset = 0;                                             \
            (top + 1)->size = 4;                                               \
            (top + 1)->var = NOT_A_VAR;                                        \
            SIZE_AND_STACK(1, 2);

	LCONSTANT_OP(opc_lconst_0, 0)
	LCONSTANT_OP(opc_lconst_1, 1)

	case opc_iadd:
	    SymBinOp(ce, top - 2, top - 1, ADDR, ADDI, GetHint(ce, pc + 1));
	    SIZE_AND_STACK(1, -1);

	case opc_iand:
	    SymBinOp(ce, top - 2, top - 1, ANDR, ANDI, GetHint(ce, pc + 1));
	    SIZE_AND_STACK(1, -1);

	case opc_ior:
	    SymBinOp(ce, top - 2, top - 1, ORR, ORI, GetHint(ce, pc + 1));
	    SIZE_AND_STACK(1, -1);

	case opc_ixor:
	    SymBinOp(ce, top - 2, top - 1, XORR, XORI, GetHint(ce, pc + 1));
	    SIZE_AND_STACK(1, -1);

	case opc_isub:
	    SubOp(ce, top - 2, top - 1, pc + 1);
	    SIZE_AND_STACK(1, -1);

	case opc_imul:
	    MulOp(ce, top - 2, top - 1, pc + 1);
	    SIZE_AND_STACK(1, -1);

	case opc_idiv:
	    DivRemOp(ce, top - 2, top - 1, pc + 1, 1);
	    SIZE_AND_STACK(1, -1);

	case opc_irem:
	    DivRemOp(ce, top - 2, top - 1, pc + 1, 0);
	    SIZE_AND_STACK(1, -1);

	case opc_ladd:
	    LAddOp(ce, top - 4, top - 2, pc + 1);
	    SIZE_AND_STACK(1, -2);

	case opc_lsub:
	    LSubOp(ce, top - 4, top - 2, pc + 1);
	    SIZE_AND_STACK(1, -2);

	case opc_lmul:
	    PushOpnd(ce, top - 1);
	    PushOpnd(ce, top - 2);
	    PushOpnd(ce, top - 3);
	    PushOpnd(ce, top - 4);
	    CallCompSupport(ce, (char *)CompSupport_lmul, top - 4, 16);
	    (top - 4)->mr = GetReg(ce, RS_EAX);
	    (top - 3)->mr = GetReg(ce, RS_EDX);
	    (top - 4)->var = NOT_A_VAR;
	    (top - 3)->var = NOT_A_VAR;
	    SIZE_AND_STACK(1, -2);

	case opc_ldiv:
	    PushOpnd(ce, top - 1);
	    PushOpnd(ce, top - 2);
	    PushOpnd(ce, top - 3);
	    PushOpnd(ce, top - 4);
	    CallCompSupport(ce, (char *)CompSupport_ldiv, top - 4, 16);
	    (top - 4)->mr = GetReg(ce, RS_EAX);
	    (top - 3)->mr = GetReg(ce, RS_EDX);
	    (top - 4)->var = NOT_A_VAR;
	    (top - 3)->var = NOT_A_VAR;
	    SIZE_AND_STACK(1, -2);

	case opc_lrem:
	    PushOpnd(ce, top - 1);
	    PushOpnd(ce, top - 2);
	    PushOpnd(ce, top - 3);
	    PushOpnd(ce, top - 4);
	    CallCompSupport(ce, (char *)CompSupport_lrem, top - 4, 16);
	    (top - 4)->mr = GetReg(ce, RS_EAX);
	    (top - 3)->mr = GetReg(ce, RS_EDX);
	    (top - 4)->var = NOT_A_VAR;
	    (top - 3)->var = NOT_A_VAR;
	    SIZE_AND_STACK(1, -2);

	case opc_land:
	    LSymBinOp(ce, top - 4, top - 2, ANDR, ANDI, pc + 1);
	    SIZE_AND_STACK(1, -2);

	case opc_lor:
	    LSymBinOp(ce, top - 4, top - 2, ORR, ORI, pc + 1);
	    SIZE_AND_STACK(1, -2);

	case opc_lxor:
	    LSymBinOp(ce, top - 4, top - 2, XORR, XORI, pc + 1);
	    SIZE_AND_STACK(1, -2);

	case opc_ishl:
	    ShiftOp(ce, top - 2, top - 1, 1, 1, pc + 1);
	    SIZE_AND_STACK(1, -1);

	case opc_ishr:
	    ShiftOp(ce, top - 2, top - 1, 0, 1, pc + 1);
	    SIZE_AND_STACK(1, -1);

	case opc_iushr:
	    ShiftOp(ce, top - 2, top - 1, 0, 0, pc + 1);
	    SIZE_AND_STACK(1, -1);

	case opc_lshl:
	    PushOpnd(ce, top - 1);
	    PushOpnd(ce, top - 2);
	    PushOpnd(ce, top - 3);
	    CallCompSupport(ce, (char *)CompSupport_lshl, top - 3, 12);
	    (top - 3)->mr = GetReg(ce, RS_EAX);
	    (top - 2)->mr = GetReg(ce, RS_EDX);
	    (top - 3)->var = NOT_A_VAR;
	    (top - 2)->var = NOT_A_VAR;
	    SIZE_AND_STACK(1, -1);

	case opc_lshr:
	    PushOpnd(ce, top - 1);
	    PushOpnd(ce, top - 2);
	    PushOpnd(ce, top - 3);
	    CallCompSupport(ce, (char *)CompSupport_lshr, top - 3, 12);
	    (top - 3)->mr = GetReg(ce, RS_EAX);
	    (top - 2)->mr = GetReg(ce, RS_EDX);
	    (top - 3)->var = NOT_A_VAR;
	    (top - 2)->var = NOT_A_VAR;
	    SIZE_AND_STACK(1, -1);

	case opc_lushr:
	    PushOpnd(ce, top - 1);
	    PushOpnd(ce, top - 2);
	    PushOpnd(ce, top - 3);
	    CallCompSupport(ce, (char *)CompSupport_lushr, top - 3, 12);
	    (top - 3)->mr = GetReg(ce, RS_EAX);
	    (top - 2)->mr = GetReg(ce, RS_EDX);
	    (top - 3)->var = NOT_A_VAR;
	    (top - 2)->var = NOT_A_VAR;
	    SIZE_AND_STACK(1, -1);

	case opc_ineg:
	    LoadOpnd(ce, top - 1, GetHint(ce, pc + 1));
	    GenOpSizEA(ce, 0xF6, NEG, top - 1);
	    SIZE_AND_STACK(1, 0);

	case opc_lneg:
	    LNegOp(ce, top - 2, pc + 1);
	    SIZE_AND_STACK(1, 0);

	case opc_iinc:
	    IncOp(ce, pc[1], ((signed char *) pc)[2]);
	    SIZE_AND_STACK(3, 0);

#define BINARY_FLOAT_OP(name, op, opr, notSym)				       \
        case opc_f##name:                                                      \
            RealOp(ce, top - 2, top - 1, op, opr, notSym, 4);                  \
            SIZE_AND_STACK(1, -1);                                             \
        case opc_d##name:                                                      \
            RealOp(ce, top - 4, top - 2, op, opr, notSym, 8);                  \
            SIZE_AND_STACK(1, -2);

	BINARY_FLOAT_OP(add, FADD, FADD,  0)
	BINARY_FLOAT_OP(sub, FSUB, FSUBR, 1)
	BINARY_FLOAT_OP(mul, FMUL, FMUL,  0)
	BINARY_FLOAT_OP(div, FDIV, FDIVR, 1)

	case opc_frem:
	    PushFOpnd(ce, top - 1, 4, 8);   // divisor
	    PushFOpnd(ce, top - 2, 4, 8);   // dividend
	    CallCompSupport(ce, (char *)CompSupport_drem, top - 2, 16);
	    (top - 2)->mr = MR_FST;
	    (top - 2)->size = 4;
	    (top - 2)->var = NOT_A_VAR;
	    SIZE_AND_STACK(1, -1);

	case opc_drem:
	    PushFOpnd(ce, top - 2, 8, 8);   // divisor
	    PushFOpnd(ce, top - 4, 8, 8);   // dividend
	    CallCompSupport(ce, (char *)CompSupport_drem, top - 4, 16);
	    (top - 4)->mr = MR_FST;
	    (top - 4)->size = 8;
	    (top - 4)->var = NOT_A_VAR;
	    (top - 3)->mr = MR_FST;
	    (top - 3)->size = 0;
	    (top - 3)->var = NOT_A_VAR;
	    SIZE_AND_STACK(1, -2);

	case opc_fneg:
	    LoadFOpnd(ce, top - 1, 4);
	    Gen2Bytes(ce, 0xD9, FCHS);
	    (top - 1)->var = NOT_A_VAR;
	    SIZE_AND_STACK(1, 0);

	case opc_dneg:
	    LoadFOpnd(ce, top - 2, 8);
	    Gen2Bytes(ce, 0xD9, FCHS);
	    (top - 2)->var = NOT_A_VAR;
	    (top - 1)->var = NOT_A_VAR;
	    SIZE_AND_STACK(1, 0);

	case opc_jsr:
	    PinTempMemAliases(ce, top);
	    Jcc(ce, CC_JSR,
		ce->rp[pc + pc2signedshort(pc) - initial_pc].hdr, top);
	    FlushVarCache(ce->ctxt);
	    sysAssert(ce->ctxt->regRefCnt[MR_EAX] == 0
		      && ce->ctxt->regRefCnt[MR_EDX] == 0
		      && ce->ctxt->regRefCnt[MR_ECX] == 0);
	    SIZE_AND_STACK(3, 0);   // 0: ret addr is not pushed

	case opc_jsr_w:
	    PinTempMemAliases(ce, top);
	    Jcc(ce, CC_JSR,
		ce->rp[pc + pc2signedlong(pc) - initial_pc].hdr, top);
	    FlushVarCache(ce->ctxt);
	    sysAssert(ce->ctxt->regRefCnt[MR_EAX] == 0
		      && ce->ctxt->regRefCnt[MR_EDX] == 0
		      && ce->ctxt->regRefCnt[MR_ECX] == 0);
	    SIZE_AND_STACK(5, 0);   // 0: ret addr is not pushed

	case opc_goto:
	    Jcc(ce, CC_JMP,
		ce->rp[pc + pc2signedshort(pc) - initial_pc].hdr, top);
	    SIZE_AND_STACK(3, 0);

	case opc_goto_w:
	    Jcc(ce, CC_JMP,
		ce->rp[pc + pc2signedlong(pc) - initial_pc].hdr, top);
	    SIZE_AND_STACK(5, 0);

	case opc_ret:
	    sysAssert(ce->ctxt->espLevel == 0);
    	    var = pc[1];
	    off = ce->varOff[var];
	    if (off < 0)
		Gen2Bytes(ce, 0xFF, JMPI + sibModRmTab[-off]);
	    else
	    {
		arg.size = 4;
		arg.mr = MR_BASE + MR_ESP;
		arg.offset = off;
		GenByte(ce, 0xFF);
		GenEA(ce, JMPI, &arg);
            }
	    SIZE_AND_STACK(2, 0);

#define COMPARISON_OP(name, cc) 					       \
        case opc_if_icmp##name:                                                \
            IfCmpcc(ce, top - 2, top - 1, cc, pc);                             \
            SIZE_AND_STACK(3, -2);                                             \
        case opc_if##name:                                                     \
            Ifcc(ce, top - 1, cc, pc);                                         \
            SIZE_AND_STACK(3, -1);

#define COMPARISON_OP2(name, nullname, cc)				       \
        COMPARISON_OP(name, cc)                                                \
        case opc_if_acmp##name:                                                \
            IfCmpcc(ce, top - 2, top - 1, cc, pc);                             \
            SIZE_AND_STACK(3, -2);                                             \
        case opc_if##nullname:                                                 \
            Ifcc(ce, top - 1, cc, pc);                                         \
            SIZE_AND_STACK(3, -1);

	COMPARISON_OP2(eq, null, CC_E)	// also generate acmp_eq and acmp_ne
	COMPARISON_OP2(ne, nonnull, CC_NE)
	COMPARISON_OP(lt, CC_L)
	COMPARISON_OP(gt, CC_G)
	COMPARISON_OP(le, CC_LE)
	COMPARISON_OP(ge, CC_GE)

	case opc_pop:
	    if ((top - 1)->mr == MR_FST && (top - 1)->size != 0)
		Gen2Bytes(ce, 0xDD, 0xD8);  // FSTP ST(0)
	    FreeRegs(ce, (top - 1)->mr);
	    SIZE_AND_STACK(1, -1);

	case opc_pop2:
	    if ((top - 1)->mr == MR_FST && (top - 1)->size != 0)
		Gen2Bytes(ce, 0xDD, 0xD8);  // FSTP ST(0)
	    if ((top - 2)->mr == MR_FST && (top - 2)->size != 0)
		Gen2Bytes(ce, 0xDD, 0xD8);  // FSTP ST(0)
	    FreeRegs(ce, (top - 2)->mr);
	    FreeRegs(ce, (top - 1)->mr);
	    SIZE_AND_STACK(1, -2);

	case opc_dup:
	    CopyOpnd(ce, top - 0, top - 1, 4, GetHint(ce, pc + 1));
	    SIZE_AND_STACK(1, 1);

	case opc_dup2:
	    FlushMultipleFST(ce, top - 2);
	    CopyOpnd(ce, top - 0, top - 2, 8, GetHint(ce, pc + 1));
	    SIZE_AND_STACK(1, 2);

	case opc_dup_x1:
	    FlushMultipleFST(ce, top - 2);
	    ce->ctxt->top = top + 1;	// push a dummy (MR_IMM) operand
	    (top + 0)->mr = MR_IMM;	// for checking (LoadOpnd) in MoveOpnd
	    MoveOpnd(ce, top - 0, top - 1, 4, GetHint(ce, pc + 1));
	    MoveOpnd(ce, top - 1, top - 2, 4, RS_CALLER_SAVED);
	    CopyOpnd(ce, top - 2, top - 0, 4, RS_CALLER_SAVED);
	    SIZE_AND_STACK(1, 1);

	case opc_dup_x2:
	    FlushMultipleFST(ce, top - 3);
	    ce->ctxt->top = top + 1;	// push a dummy (MR_IMM) operand
	    (top + 0)->mr = MR_IMM;	// for checking (LoadOpnd) in MoveOpnd
	    MoveOpnd(ce, top - 0, top - 1, 4, GetHint(ce, pc + 1));
	    MoveOpnd(ce, top - 2, top - 3, 8, RS_CALLER_SAVED);
	    CopyOpnd(ce, top - 3, top - 0, 4, RS_CALLER_SAVED);
	    SIZE_AND_STACK(1, 1);

	case opc_dup2_x1:
	    FlushMultipleFST(ce, top - 3);
	    ce->ctxt->top = top + 2;	// push 2 dummy (MR_IMM) operands
	    (top + 0)->mr = MR_IMM;	// for checking (LoadOpnd) in MoveOpnd
	    (top + 1)->mr = MR_IMM;
	    MoveOpnd(ce, top - 0, top - 2, 8, GetHint(ce, pc + 1));
	    MoveOpnd(ce, top - 1, top - 3, 4, RS_CALLER_SAVED);
	    CopyOpnd(ce, top - 3, top - 0, 8, RS_CALLER_SAVED);
	    SIZE_AND_STACK(1, 2);

	case opc_dup2_x2:
	    FlushMultipleFST(ce, top - 4);
	    ce->ctxt->top = top + 2;	// push 2 dummy (MR_IMM) operands
	    (top + 0)->mr = MR_IMM;	// for checking (LoadOpnd) in MoveOpnd
	    (top + 1)->mr = MR_IMM;
	    MoveOpnd(ce, top - 0, top - 2, 8, GetHint(ce, pc + 1));
	    MoveOpnd(ce, top - 2, top - 4, 8, RS_CALLER_SAVED);
	    CopyOpnd(ce, top - 4, top - 0, 8, RS_CALLER_SAVED);
	    SIZE_AND_STACK(1, 2);

	case opc_swap:
	    SwapOpnd(ce, top - 2, top - 1, GetHint(ce, pc + 1));
	    SIZE_AND_STACK(1, 0);

	case opc_arraylength:
	    LoadOpnd(ce, top - 1, RS_ALL);
	    (top - 1)->mr += MR_BASE;
	    (top - 1)->offset = offsetof(HArrayOfObject, methods);
	    arg.mr = MR_IMM;
	    arg.offset = METHOD_FLAG_BITS;
	    // ShiftOp will always load top - 1
	    ShiftOp(ce, top - 1, &arg, 0, 0, pc + 1);
	    SIZE_AND_STACK(1, 0);

	case opc_tableswitch:
	{
	    long *ltbl = (long *) ALIGN((int)pc + 1);
	    long def = swap(ltbl[0]);
	    long low = swap(ltbl[1]);
	    long high = swap(ltbl[2]);
	    long npairs = high - low + 1;
	    long i;
	    RangeHdr	*defRh;
	    Case	*cases, *line;

	    defRh = rh->targRh;
	    if (npairs > 0)
	    {
		FlushOpStack(ce, top - 1);
		// actually only needed if a target is really a join point,
		// but joinCnt of target is not accurate, since several labels
		// for the same target result in a joinCnt > 0

		LoadOpnd(ce, top - 1, RS_CALLER_SAVED);
		// set correct ctxt->top and ctxt->regRefCnt for targets:
		ce->ctxt->top = top - 1;
		FreeRegs(ce, (top - 1)->mr);
		sysAssert(ce->ctxt->regRefCnt[MR_EAX] == 0
			  && ce->ctxt->regRefCnt[MR_EDX] == 0
			  && ce->ctxt->regRefCnt[MR_ECX] == 0);

		// check code size, PC_RED_ZONE not good enough
		if  (ce->pc + 5*npairs >= ce->pcRedZone)
		{
		    ce->err = pcInRedZone;
		    return FALSE;
		}

		cases = (*p_malloc)(npairs*sizeof(Case));
		if  (cases == 0)
		{
		    ce->err = "Not enough memory";
		    return FALSE;
		}
		ce->cases = cases;  // to be deallocated in case of an exception
		clone = 1;
		for (i = 0, line = cases; i < npairs; i++, line++)
		{
		    NEXT_RANGE();
		    targRh = rh->targRh;
		    if (targRh->pc == 0 && targRh->ctxt == 0)
		    {
			// forward target, context not set yet
			if  (!CopyContext(ce, targRh, &clone))
			    return FALSE;
		    }
		    line->key = low + i;
		    line->targRh = targRh;
		}
		SortCases(cases, cases + npairs);
		top = ce->ctxt->top;
		Switch(ce, top->mr, cases, cases + npairs - 1, defRh);
		(*p_free)(cases);
		ce->cases = NULL;
	    }
	    else
		Jcc(ce, CC_JMP, defRh, --top);

	    pc = (unsigned char *)(ltbl + 3 + npairs);
	}
	continue;

	case opc_lookupswitch:
	{
	    long *ltbl = (long *) ALIGN((int)pc + 1);
	    long def = swap(ltbl[0]);
	    long npairs = swap(ltbl[1]);
	    long i;
	    RangeHdr	*defRh;
	    Case	*cases, *line;

	    defRh = rh->targRh;
	    if (npairs > 0)
	    {
		FlushOpStack(ce, top - 1);
		// actually only needed if a target is really a join point,
		// but joinCnt of target is not accurate, since several labels
		// for the same target result in a joinCnt > 0

		LoadOpnd(ce, top - 1, RS_CALLER_SAVED);

		// set correct ctxt->top and ctxt->regRefCnt for targets:
		ce->ctxt->top = top - 1;
		FreeRegs(ce, (top - 1)->mr);
		sysAssert(ce->ctxt->regRefCnt[MR_EAX] == 0
			  && ce->ctxt->regRefCnt[MR_EDX] == 0
			  && ce->ctxt->regRefCnt[MR_ECX] == 0);

		// check code size, PC_RED_ZONE not good enough
		if  (ce->pc + 11*npairs >= ce->pcRedZone)
		{
		    ce->err = pcInRedZone;
		    return FALSE;
		}

		cases = (*p_malloc)(npairs*sizeof(Case));
		if  (cases == 0)
		{
		    ce->err = "Not enough memory";
		    return FALSE;
		}
		ce->cases = cases;  // to be deallocated in case of an exception
		clone = 1;
		for (i = 0, line = cases; i < npairs; i++, line++)
		{
		    NEXT_RANGE();
		    targRh = rh->targRh;
		    if (targRh->pc == 0 && targRh->ctxt == 0)
		    {
			// forward target, context not set yet
			if  (!CopyContext(ce, targRh, &clone))
			    return FALSE;
		    }
		    line->key = swap(ltbl[i * 2 + 2]);
		    line->targRh = targRh;
		}
		SortCases(cases, cases + npairs);
		top = ce->ctxt->top;
		Switch(ce, top->mr, cases, cases + npairs - 1, defRh);
		(*p_free)(cases);
		ce->cases = NULL;
	    }
	    else
		Jcc(ce, CC_JMP, defRh, --top);

	    pc = (unsigned char *)(ltbl + 2 + npairs * 2);
	}
	continue;

	case opc_athrow:
	    PushOpnd(ce, top - 1);
	    CallCompSupport(ce, (char *)CompSupport_athrow, top - 1, 4);
	    SIZE_AND_STACK(1, -1);

	case opc_getstatic:
	    fb = cpool[GET_INDEX(pc+1)].p;
	    isig = fieldsig(fb)[0];
	    if (isig == SIGNATURE_LONG || isig == SIGNATURE_DOUBLE)
	    {
		GetStatic((long)twoword_static_address(fb), top, 8);
		push = 2;
	    }
	    else
	    {
		GetStatic((long)normal_static_address(fb), top, 4);
		push = 1;
	    }
	    SIZE_AND_STACK(3, push);

	case opc_putstatic:
	    fb = cpool[GET_INDEX(pc+1)].p;
	    isig = fieldsig(fb)[0];
	    if (isig == SIGNATURE_LONG || isig == SIGNATURE_DOUBLE)
	    {
		PutStatic(ce, (long)twoword_static_address(fb), top - 2, 8);
		push = -2;
	    }
	    else
	    {
		PutStatic(ce, (long)normal_static_address(fb), top - 1, 4);
		push = -1;
	    }
	    SIZE_AND_STACK(3, push);

	case opc_putfield_quick:
	    PutField(ce, top - 2, 4*pc[1], top - 1, 4);
	    SIZE_AND_STACK(3, -2);

	case opc_getfield_quick:
	    GetField(ce, top - 1, 4*pc[1], 4, pc + 3);
	    SIZE_AND_STACK(3, 0);

	case opc_putfield2_quick:
	    PutField(ce, top - 3, 4*pc[1], top - 2, 8);
	    SIZE_AND_STACK(3, -3);

	case opc_getfield2_quick:
	    GetField(ce, top - 1, 4*pc[1], 8, pc + 3);
	    SIZE_AND_STACK(3, 1);

	case opc_putstatic_quick:
	    fb = cpool[GET_INDEX(pc+1)].p;
	    PutStatic(ce, (long)normal_static_address(fb), top - 1, 4);
	    SIZE_AND_STACK(3, -1);

	case opc_getstatic_quick:
	    fb = cpool[GET_INDEX(pc+1)].p;
	    GetStatic((long)normal_static_address(fb), top, 4);
	    SIZE_AND_STACK(3, 1);

	case opc_putstatic2_quick:
	    fb = cpool[GET_INDEX(pc+1)].p;
	    PutStatic(ce, (long)twoword_static_address(fb), top - 2, 8);
	    SIZE_AND_STACK(3, -2);

	case opc_getstatic2_quick:
	    fb = cpool[GET_INDEX(pc+1)].p;
	    GetStatic((long)twoword_static_address(fb), top, 8);
	    SIZE_AND_STACK(3, 2);

	case opc_putfield:
	case opc_putfield_quick_w:
	    fb = cpool[GET_INDEX(pc+1)].p;
	    off = fb->u.offset;
	    isig = fieldsig(fb)[0];
	    if (isig == SIGNATURE_LONG || isig == SIGNATURE_DOUBLE)
	    {
		PutField(ce, top - 3, off, top - 2, 8);
		SIZE_AND_STACK(3, -3);
	    }
	    else
	    {
		PutField(ce, top - 2, off, top - 1, 4);
		SIZE_AND_STACK(3, -2);
	    }

	case opc_getfield:
	case opc_getfield_quick_w:
	    fb = cpool[GET_INDEX(pc+1)].p;
	    isig = fieldsig(fb)[0];
	    if (isig == SIGNATURE_LONG || isig == SIGNATURE_DOUBLE)
	    {
		GetField(ce, top - 1, fb->u.offset, 8, pc + 3);
		SIZE_AND_STACK(3, 1);
	    }
	    else
	    {
		GetField(ce, top - 1, fb->u.offset, 4, pc + 3);
		SIZE_AND_STACK(3, 0);
	    }

	case opc_invokevirtual:
	case opc_invokevirtual_quick_w:
	    mb_type = cpool[GET_INDEX(pc+1)].p;
	    push = InvokeMethod(ce, mb_type, top, 1, 0);
	    SIZE_AND_STACK(3, push);

	case opc_invokenonvirtual:
	case opc_invokenonvirtual_quick:
	    mb_type = cpool[GET_INDEX(pc+1)].p;
	    push = InvokeMethod(ce, mb_type, top, 0, 0);
	    SIZE_AND_STACK(3, push);

	case opc_invokesuper_quick:
	    mb_type = cbMethodTable(unhand(cbSuperclass(cb)))
		->methods[GET_INDEX(pc + 1)];
	    push = InvokeMethod(ce, mb_type, top, 0, 0);
	    SIZE_AND_STACK(3, push);

	case opc_invokestatic:
	case opc_invokestatic_quick:
	    mb_type = cpool[GET_INDEX(pc+1)].p;
	    push = InvokeMethod(ce, mb_type, top, 0, 1);
	    SIZE_AND_STACK(3, push);

	case opc_invokeinterface:
	    index = cpool[GET_INDEX(pc+1)].i & 0xFFFF;
	    push = InvokeInterface(ce, cpool[index].i, top);
	    SIZE_AND_STACK(5, push);

	case opc_invokeinterface_quick:
	    push = InvokeInterface(ce, cpool[GET_INDEX(pc + 1)].i, top);
	    SIZE_AND_STACK(5, push);

	case opc_invokevirtualobject_quick:
	case opc_invokevirtual_quick:
	    check(0);	// must be detected in a previous pass

	case opc_instanceof:
	case opc_instanceof_quick:
	    PushOpnd(ce, top - 1);  // handle
	    arg.mr = MR_IMM;
	    arg.offset = cpool[GET_INDEX(pc + 1)].i;
	    Push(ce, &arg);	    // cb
	    CallCompSupport(ce, (char *)CompSupport_instanceof, top - 1, 8);
	    (top - 1)->mr = GetReg(ce, RS_EAX);
	    (top - 1)->size = 4;
	    (top - 1)->var = NOT_A_VAR;
	    SIZE_AND_STACK(3, 0);

	case opc_checkcast:
	case opc_checkcast_quick:
	    PushOpnd(ce, top - 1);  // handle
	    arg.mr = MR_IMM;
	    arg.offset = cpool[GET_INDEX(pc + 1)].i;
	    Push(ce, &arg);	    // cb
	    CallCompSupport(ce, (char *)CompSupport_checkcast, top - 1, 8);
	    (top - 1)->mr = GetReg(ce, RS_EAX);
	    (top - 1)->size = 4;
	    (top - 1)->var = NOT_A_VAR;
	    SIZE_AND_STACK(3, 0);

	case opc_new:
	case opc_new_quick:
	    arg.mr = MR_IMM;
	    arg.offset = cpool[GET_INDEX(pc + 1)].i;
	    Push(ce, &arg);	// cb
	    CallCompSupport(ce, (char *)CompSupport_new, top, 4);
	    top->mr = GetReg(ce, RS_EAX);
	    top->size = 4;
	    top->var = NOT_A_VAR;
	    SIZE_AND_STACK(3, 1);

	case opc_anewarray:
	case opc_anewarray_quick:
	    PushOpnd(ce, top - 1);  // size
	    arg.mr = MR_IMM;
	    arg.offset = cpool[GET_INDEX(pc + 1)].i;
	    Push(ce, &arg);	    // cb
	    CallCompSupport(ce, (char *)CompSupport_anewarray, top - 1, 8);
	    (top - 1)->mr = GetReg(ce, RS_EAX);
	    (top - 1)->size = 4;
	    (top - 1)->var = NOT_A_VAR;
	    SIZE_AND_STACK(3, 0);

	case opc_multianewarray:
	case opc_multianewarray_quick:
	    dimensions = pc[3];
	    for (push = 1; push <= dimensions; push++)
		PushOpnd(ce, top - push);
	    arg.mr = MR_IMM;
	    arg.offset = dimensions;
	    Push(ce, &arg);	    // dimensions
	    arg.mr = MR_IMM;
	    arg.offset = cpool[GET_INDEX(pc + 1)].i;
	    Push(ce, &arg);	    // cb
	    CallCompSupport(ce, (char *)CompSupport_multianewarray,
		top - dimensions, 4*dimensions + 8);
            GenAddImmRL(ce, MR_ESP, 4*dimensions + 8);	// __cdecl
	    (top - dimensions)->mr = GetReg(ce, RS_EAX);
	    (top - dimensions)->size = 4;
	    (top - dimensions)->var = NOT_A_VAR;
	    SIZE_AND_STACK(4, 1 - dimensions);

	case opc_newarray:
	    PushOpnd(ce, top - 1);  // size
	    arg.mr = MR_IMM;
	    arg.offset = pc[1];
	    Push(ce, &arg);	    // type
	    CallCompSupport(ce, (char *)CompSupport_newarray, top - 1, 8);
	    (top - 1)->mr = GetReg(ce, RS_EAX);
	    (top - 1)->size = 4;
	    (top - 1)->var = NOT_A_VAR;
	    SIZE_AND_STACK(2, 0);

	case opc_iaload:
	case opc_faload:
	case opc_aaload:
	    ArrayLoad(ce, top - 2, top - 1, 2, 4, 0, pc + 1);
	    SIZE_AND_STACK(1, -1);

	case opc_baload:
	    ArrayLoad(ce, top - 2, top - 1, 0, 4, 1, pc + 1);
	    SIZE_AND_STACK(1, -1);

	case opc_caload:
	    ArrayLoad(ce, top - 2, top - 1, 1, 4, 0, pc + 1);
	    SIZE_AND_STACK(1, -1);

	case opc_saload:
	    ArrayLoad(ce, top - 2, top - 1, 1, 4, 1, pc + 1);
	    SIZE_AND_STACK(1, -1);

	case opc_laload:
	case opc_daload:
	    ArrayLoad(ce, top - 2, top - 1, 3, 8, 0, pc + 1);
	    SIZE_AND_STACK(1, 0);

	case opc_iastore:
	case opc_fastore:
	    ArrayStore(ce, top - 3, top - 2, top - 1, 2, 4);
	    SIZE_AND_STACK(1, -3);

	case opc_bastore:
	    ArrayStore(ce, top - 3, top - 2, top - 1, 0, 4);
	    SIZE_AND_STACK(1, -3);

	case opc_castore:
	case opc_sastore:
	    ArrayStore(ce, top - 3, top - 2, top - 1, 1, 4);
	    SIZE_AND_STACK(1, -3);

	case opc_lastore:
	case opc_dastore:
	    ArrayStore(ce, top - 4, top - 3, top - 2, 3, 8);
	    SIZE_AND_STACK(1, -4);

	case opc_aastore:
	    PushOpnd(ce, top - 1);
	    PushOpnd(ce, top - 2);
	    PushOpnd(ce, top - 3);
	    CallCompSupport(ce, (char *)CompSupport_aastore, top - 3, 12);
	    SIZE_AND_STACK(1, -3);

	case opc_monitorenter:
	    PushOpnd(ce, top - 1);
	    CallCompSupport(ce, (char *)CompSupport_monitorenter, top - 1, 4);
	    SIZE_AND_STACK(1, -1);

	case opc_monitorexit:
    	    PushImm(ce, 0);
	    PushOpnd(ce, top - 1);
	    CallCompSupport(ce, (char *)CompSupport_monitorexit, top - 1, 8);
	    SIZE_AND_STACK(1, -1);

	case opc_wide:
	    var = GET_INDEX(pc + 2);
	    switch(pc[1])
	    {
	    case opc_aload:
	    case opc_iload:
	    case opc_fload:
		DO_LOAD_DWORD(top, var);
		SIZE_AND_STACK(4, 1);

	    case opc_lload:
	    case opc_dload:
		DO_LOAD_QWORD(top, var);
		SIZE_AND_STACK(4, 2);

	    case opc_istore:
	    case opc_astore:
		StoreOpnd(ce, var, top - 1);
		SIZE_AND_STACK(4, -1);

	    case opc_fstore:
		StoreFOpnd(ce, var, top - 1, 4);
		SIZE_AND_STACK(4, -1);

	    case opc_lstore:
		StoreOpnd(ce, var + 1, top - 1); // high first for anti-aliasing
		StoreOpnd(ce, var + 0, top - 2);
		SIZE_AND_STACK(4, -2);

	    case opc_dstore:
		StoreFOpnd(ce, var, top - 2, 8);
		SIZE_AND_STACK(4, -2);

	    case opc_iinc:
		IncOp(ce, var, (((signed char *)pc)[4] << 8) + pc[5]);
		SIZE_AND_STACK(6, 0);

	    case opc_ret:
		sysAssert(ce->ctxt->espLevel == 0);
		off = ce->varOff[var];
		if (off < 0)
		    Gen2Bytes(ce, 0xFF, JMPI + sibModRmTab[-off]);
		else
		{
		    arg.size = 4;
		    arg.mr = MR_BASE + MR_ESP;
		    arg.offset = off;
		    GenByte(ce, 0xFF);
		    GenEA(ce, JMPI, &arg);
		}
		SIZE_AND_STACK(4, 0);

	    default:
		sysAssert(0);
		ce->err = "Undefined opcode";
		return FALSE;
	    }

	default:
	    sysAssert(0);
	    ce->err = "Undefined opcode";
	    return FALSE;
	}
    }
    check(0);
}


bool_t
jitCompile(struct methodblock *mb, char **code, CodeInfo **info, char **err, ExecEnv *ee)
{
    CompEnv ce;
    char *cc;

    do
    {
	__try
	{
	    if (    InitializeCompEnv(&ce, mb, ee)
		&&  MarkExceptionRanges(&ce)
		&&  MarkBranchTargAndVars(&ce)
		&&  AllocRangeHeaders(&ce)
		&&  ComputeOpstackHeight(&ce)
		&&  IndexExceptionHandlers(&ce)
		&&  EstimateVarFreq(&ce)
		&&  AllocVariables(&ce)
		&&  Prologue(&ce)
		&&  Body(&ce)
		&&  Epilogue(&ce)
	       )
	    {
		// Truncate code block.
		// We have at least PC_RED_ZONE bytes to give back.
		cc = ce.compiledCode;
#ifdef	DEBUG
		nc_count += ce.pc - cc;
		bc_count += mb->code_length;
#endif
		ce.codeInfo->start_pc = cc;
                ce.codeInfo->end_pc = ce.pc;
                ce.compiledCode = (*p_realloc)(cc, ce.pc - cc);
		if  (ce.compiledCode != cc)
		    ce.err = "Truncating realloc failed";
	    }
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
	    if (GetExceptionCode() == OPSTACK_HEIGHT_EXCEPTION)
            {
		ce.err = "Stack height cannot be determined statically";
		cprintf(stderr, "JIT compiler: %s,\n        Recompile %s with a newer compiler.\n",
		    ce.err, ce.mb->fb.clazz->source_name);
	    }
	    else if (GetExceptionCode() == BAD_BYTECODE_EXCEPTION)
	    {
		ce.err = "Bytecode does not conform to the specification";
		cprintf(stderr, "JIT compiler: %s,\n        Recompile %s with a newer compiler.\n",
		    ce.err, ce.mb->fb.clazz->source_name);
	    }
	    else
		ce.err = "Internal error";
	}
	if  (ce.err)
	{
	    if (ce.compiledCode)
	    {
		(*p_free)(ce.compiledCode);
		ce.compiledCode = NULL;
	    }
	    if (ce.codeInfo)
	    {
	        (*p_free)(ce.codeInfo);
		ce.codeInfo = NULL;
	    }
	    if (ce.err == pcInRedZone)
		codeSizeFactor++;
	}
	FinalizeCompEnv(&ce);
    }
    while (ce.err == pcInRedZone);
    *err = ce.err;
    *code = ce.compiledCode;
    *info = ce.codeInfo;
    return (ce.err == 0);
}


void InitCompiler(void)
{
    InitCodeGen();
}
