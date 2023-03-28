// $Header: /m/src/ns/sun-java/jit/win32/Attic/jcodegen.c,v 1.2 1996/06/16 02:15:17 jg Exp $ 
/*
 * Copyright (c) 1996 Borland International. All Rights Reserved.
 *
 * AppAccelerator(tm) for x86
 *
 * JCodeGen.c, R. Crelier, 1/29/96
 *
*/
#include "jcodegen.h"
#include "jcompile.h"

/* defining sysAssert: */
#include "debug.h"
#include "sysmacros_md.h"

/*
    This finds the base register number for register direct and indirect
    address modes. this can also be used to figure out whether there
    is actually a base register in an address mode.
*/
static char regNumOfMr[MR_LAST] =
{
    MR_EAX, MR_EDX, MR_ECX, MR_EBX, MR_ESI, MR_EDI, MR_EBP, MR_ESP,
    MR_EAX, MR_EDX, MR_ECX, MR_EBX, MR_ESI, MR_EDI, MR_EBP, MR_ESP,
    MR_EAX, MR_EDX, MR_ECX, MR_EBX, MR_ESI, MR_EDI, MR_EBP, MR_ESP,

    -1,     -1,     -1,     -1,     -1,     -1,     -1,     -1,

    MR_EAX, MR_EDX, MR_ECX, MR_EBX, MR_ESI, MR_EDI, MR_EBP, MR_ESP,
    MR_EAX, MR_EDX, MR_ECX, MR_EBX, MR_ESI, MR_EDI, MR_EBP, MR_ESP,
    MR_EAX, MR_EDX, MR_ECX, MR_EBX, MR_ESI, MR_EDI, MR_EBP, MR_ESP,
    MR_EAX, MR_EDX, MR_ECX, MR_EBX, MR_ESI, MR_EDI, MR_EBP, MR_ESP,

    MR_EAX, MR_EDX, MR_ECX, MR_EBX, MR_ESI, MR_EDI, MR_EBP, MR_ESP,
    MR_EAX, MR_EDX, MR_ECX, MR_EBX, MR_ESI, MR_EDI, MR_EBP, MR_ESP,
    MR_EAX, MR_EDX, MR_ECX, MR_EBX, MR_ESI, MR_EDI, MR_EBP, MR_ESP,
    MR_EAX, MR_EDX, MR_ECX, MR_EBX, MR_ESI, MR_EDI, MR_EBP, MR_ESP,

    -1,     -1,     -1,     -1,     -1,     -1,

};


/*
    this table finds the set of registers associated with an
    address mode. This is useful for freeing up registers
    used in address modes.
*/
RegSet	regSetOfMr[MR_LAST] =
{
#define AX  RS_EAX
#define DX  RS_EDX
#define CX  RS_ECX
#define BX  RS_EBX
#define SI  RS_ESI
#define DI  RS_EDI
#define BP  RS_EBP
#define SP  RS_ESP

    AX,     DX,     CX,     BX,     SI,     DI,     BP,     SP,
    AX,     DX,     CX,     BX,     SI,     DI,     BP,     SP,
    AX,     DX,     CX,     BX,     SI,     DI,     BP,     SP,
    AX,     DX,     CX,     BX,     SI,     DI,     BP,     SP,

    AX|AX,  AX|DX,  AX|CX,  AX|BX,  AX|SI,  AX|DI,  AX|BP,  AX|SP,
    DX|AX,  DX|DX,  DX|CX,  DX|BX,  DX|SI,  DX|DI,  DX|BP,  DX|SP,
    CX|AX,  CX|DX,  CX|CX,  CX|BX,  CX|SI,  CX|DI,  CX|BP,  CX|SP,
    BX|AX,  BX|DX,  BX|CX,  BX|BX,  BX|SI,  BX|DI,  BX|BP,  BX|SP,

    SI|AX,  SI|DX,  SI|CX,  SI|BX,  SI|SI,  SI|DI,  SI|BP,  SI|SP,
    DI|AX,  DI|DX,  DI|CX,  DI|BX,  DI|SI,  DI|DI,  DI|BP,  DI|SP,
    BP|AX,  BP|DX,  BP|CX,  BP|BX,  BP|SI,  BP|DI,  BP|BP,  BP|SP,
    SP|AX,  SP|DX,  SP|CX,  SP|BX,  SP|SI,  SP|DI,  SP|BP,  SP|SP,

#undef	AX
#undef	DX
#undef	CX
#undef	BX
#undef	SI
#undef	DI
#undef	BP
#undef	SP

    0,	    0,	    0,	    0,	    0,	    0,
};

/*
    this table gives the target register set corresponding
    to a given address mode. this is used for determining
    whether a given address mode fits a target.
*/
RegSet	targOfMr[MR_LAST] =
{
    RS_EAX, RS_EDX, RS_ECX, RS_EBX, RS_ESI, RS_EDI, RS_EBP, RS_ESP,

    0,	    0,	    0,	    0,	    0,	    0,	    0,	    0,
    0,	    0,	    0,	    0,	    0,	    0,	    0,	    0,
    0,	    0,	    0,	    0,	    0,	    0,	    0,	    0,

    0,	    0,	    0,	    0,	    0,	    0,	    0,	    0,
    0,	    0,	    0,	    0,	    0,	    0,	    0,	    0,
    0,	    0,	    0,	    0,	    0,	    0,	    0,	    0,
    0,	    0,	    0,	    0,	    0,	    0,	    0,	    0,

    0,	    0,	    0,	    0,	    0,	    0,	    0,	    0,
    0,	    0,	    0,	    0,	    0,	    0,	    0,	    0,
    0,	    0,	    0,	    0,	    0,	    0,	    0,	    0,
    0,	    0,	    0,	    0,	    0,	    0,	    0,	    0,

    0,	    0,	    0,	    0,	    0,	    0,
};


CondCode    swapCC[] =
{
    CC_O,   /*	CC_O	= 0x00	*/
    CC_NO,  /*	CC_NO	= 0x01	*/
    CC_A,   /*	CC_B	= 0x02	*/
    CC_BE,  /*	CC_AE	= 0x03	*/
    CC_E,   /*	CC_E	= 0x04	*/
    CC_NE,  /*	CC_NE	= 0x05	*/
    CC_AE,  /*	CC_BE	= 0x06	*/
    CC_B,   /*	CC_A	= 0x07	*/
    CC_S,   /*	CC_S	= 0x08	*/
    CC_NS,  /*	CC_NS	= 0x09	*/
    CC_PE,  /*	CC_PE	= 0x0A	*/
    CC_PO,  /*	CC_PO	= 0x0B	*/
    CC_G,   /*	CC_L	= 0x0C	*/
    CC_LE,  /*	CC_GE	= 0x0D	*/
    CC_GE,  /*	CC_LE	= 0x0E	*/
    CC_L,   /*	CC_G	= 0x0F	*/
};


CondCode    xsgnCC[] =
{
    CC_O,   /*	CC_O	= 0x00	*/
    CC_NO,  /*	CC_NO	= 0x01	*/
    CC_L,   /*	CC_B	= 0x02	*/
    CC_GE,  /*	CC_AE	= 0x03	*/
    CC_E,   /*	CC_E	= 0x04	*/
    CC_NE,  /*	CC_NE	= 0x05	*/
    CC_LE,  /*	CC_BE	= 0x06	*/
    CC_G,   /*	CC_A	= 0x07	*/
    CC_S,   /*	CC_S	= 0x08	*/
    CC_NS,  /*	CC_NS	= 0x09	*/
    CC_PE,  /*	CC_PE	= 0x0A	*/
    CC_PO,  /*	CC_PO	= 0x0B	*/
    CC_B,   /*	CC_L	= 0x0C	*/
    CC_AE,  /*	CC_GE	= 0x0D	*/
    CC_BE,  /*	CC_LE	= 0x0E	*/
    CC_A,   /*	CC_G	= 0x0F	*/
};


CondCode    highCC[] =
{
    CC_O,   /*	CC_O	= 0x00	*/
    CC_NO,  /*	CC_NO	= 0x01	*/
    CC_B,   /*	CC_B	= 0x02	*/
    CC_A,   /*	CC_AE	= 0x03	*/
    CC_NE,  /*	CC_E	= 0x04	*/
    CC_NE,  /*	CC_NE	= 0x05	*/
    CC_B,   /*	CC_BE	= 0x06	*/
    CC_A,   /*	CC_A	= 0x07	*/
    CC_S,   /*	CC_S	= 0x08	*/
    CC_NS,  /*	CC_NS	= 0x09	*/
    CC_PE,  /*	CC_PE	= 0x0A	*/
    CC_PO,  /*	CC_PO	= 0x0B	*/
    CC_L,   /*	CC_L	= 0x0C	*/
    CC_G,   /*	CC_GE	= 0x0D	*/
    CC_L,   /*	CC_LE	= 0x0E	*/
    CC_G,   /*	CC_G	= 0x0F	*/
};


CondCode    revhCC[] =
{
    CC_O,   /*	CC_O	= 0x00	*/
    CC_NO,  /*	CC_NO	= 0x01	*/
    CC_A,   /*	CC_B	= 0x02	*/
    CC_B,   /*	CC_AE	= 0x03	*/
    CC_NE,  /*	CC_E	= 0x04	*/
    CC_E,   /*	CC_NE	= 0x05	*/
    CC_A,   /*	CC_BE	= 0x06	*/
    CC_B,   /*	CC_A	= 0x07	*/
    CC_S,   /*	CC_S	= 0x08	*/
    CC_NS,  /*	CC_NS	= 0x09	*/
    CC_PE,  /*	CC_PE	= 0x0A	*/
    CC_PO,  /*	CC_PO	= 0x0B	*/
    CC_G,   /*	CC_L	= 0x0C	*/
    CC_L,   /*	CC_GE	= 0x0D	*/
    CC_G,   /*	CC_LE	= 0x0E	*/
    CC_L,   /*	CC_G	= 0x0F	*/
};


void GenByte(CompEnv *ce, char b)
{
    *ce->pc++ = b;
}


void Gen2Bytes(CompEnv *ce, char b1, char b2)
{
    char *s;

    s = ce->pc;
    *s++ = b1;
    *s++ = b2;
    ce->pc = s;
}


void Gen3Bytes(CompEnv *ce, char b1, char b2, char b3)
{
    char *s;

    s = ce->pc;
    *s++ = b1;
    *s++ = b2;
    *s++ = b3;
    ce->pc = s;
}


void GenByteLong(CompEnv *ce, char b1, long l2)
{
    char    *s;
    long    *l;

    s = ce->pc;
    *s++ = b1;
    l = (long *)s;
    *l++ = l2;
    ce->pc = (char *)l;
}


void Gen2BytesLong(CompEnv *ce, char b1, char b2, long l2)
{
    char    *s;
    long    *l;

    s = ce->pc;
    *s++ = b1;
    *s++ = b2;
    l = (long *)s;
    *l++ = l2;
    ce->pc = (char *)l;
}


void GenWord(CompEnv *ce, short w)
{
    *((short *)ce->pc)++ = w;
}


void GenLong(CompEnv *ce, long l)
{
    *((long *)ce->pc)++ = l;
}


/* this table contains the register encoding */
char	regMap[MR_ESP+1] =
    { 0x00, 0x02, 0x01, 0x03, 0x06, 0x07, 0x05, 0x04 };

/* this table contains the register encoding shifted left three bits */
char	reg3Map[MR_ESP+1] =
    { 0x00, 0x10, 0x08, 0x18, 0x30, 0x38, 0x28, 0x20 };

/* this table facilitates encoding of address modes.	*/
/* the low byte contains part of the mod r/m byte,	*/
/* the high byte is the low 6 bits of the s-i-b byte	*/
/* bit 15 on means no s-i-b, -1 is illegal address mode */
unsigned short	sibModRmTab[MR_LAST] =
{
    0x00C0, 0x00C2, 0x00C1, 0x00C3, 0x00C6, 0x00C7, 0x00C5, 0x00C4,
    0x8000, 0x8002, 0x8001, 0x8003, 0x8006, 0x8007, 0x8005, 0x2404,
    0x8000, 0x8002, 0x8001, 0x8003, 0x8006, 0x8007, 0x8005, 0x2404,
    0x0504, 0x1504, 0x0D04, 0x1D04, 0x3504, 0x3D04, 0x2D04, 0xFFFF,

    0x0004, 0x0204, 0x0104, 0x0304, 0x0604, 0x0704, 0x0504, 0x0404,
    0x1004, 0x1204, 0x1104, 0x1304, 0x1604, 0x1704, 0x1504, 0x1404,
    0x0804, 0x0A04, 0x0904, 0x0B04, 0x0E04, 0x0F04, 0x0D04, 0x0C04,
    0x1804, 0x1A04, 0x1904, 0x1B04, 0x1E04, 0x1F04, 0x1D04, 0x1C04,
    0x3004, 0x3204, 0x3104, 0x3304, 0x3604, 0x3704, 0x3504, 0x3404,
    0x3804, 0x3A04, 0x3904, 0x3B04, 0x3E04, 0x3F04, 0x3D04, 0x3C04,
    0x2804, 0x2A04, 0x2904, 0x2B04, 0x2E04, 0x2F04, 0x2D04, 0x2C04,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
};


void GenEA(CompEnv *ce, int r, Item *n)
{
    ModReg  mr;
    long    offs;

    /*
	emit the mod r/m byte, s-i-b byte if necessary
	and the offset (no fixups).
    */
    sysAssert((r & EM_MODRM) == 0);

    mr = n->mr;
    if (IsIntReg(mr) || IsIndir(mr))
    {
	sysAssert(n->size > 1 || IsByteAccessible(mr));
	if  (mr == MR_INDIR + MR_EBP)
	    Gen2Bytes(ce, sibModRmTab[mr] + r + 0x40, 0);
	else
	    GenByte(ce, sibModRmTab[mr] + r);
    }
    else if (mr == MR_ABS)
	GenByteLong(ce, 0x05 + r, n->offset);
    else
    {
	sysAssert(mr < MR_IMM);
	offs = n->offset;
	if  (RegOfMr(mr) == MR_ESP)
	    offs -= ce->ctxt->espLevel;
	if  (offs == 0)
	{
	    if (RegOfMr(mr) == MR_EBP)
		r += 0x40;		/* need 8 bit offset */
	}
	else if (ByteRange(offs))
	    r += 0x40;			/* need 8 bit offset */
	else
	    r += 0x80;			/* need 32 bit offset */

	/*
	    if there is an index with no scaling, but no base,
	    map that to just a base and save a byte.
	*/
	if  (IsIndexed(mr) && n->scale == 0)
	{
	    mr -= MR_INX - MR_BASE;
	    r += 0x80;
	}
	r += sibModRmTab[mr];
	if  ((r & 0x8000) == 0)
	{
	    if (HasIndex(mr))
		GenWord(ce, (n->scale << 14) + r);
	    else
		GenWord(ce, r);
	}
	else
	    GenByte(ce, r);

	if  (r & 0x40)
	    GenByte(ce, offs);
	else if (r & 0x80)
	    GenLong(ce, offs);
    }
}


/* generate an operand size override if necessary and return size bit */
int GenSiz(CompEnv *ce, long siz)
{
    switch  (siz)
    {
    default:	sysAssert(0);
    case    1:			    return(0);
    case    2:	GenByte(ce, OPNDSIZE);	/* fall thru */
    case    3:
    case    4:			    return(1);
    }
}


void GenOpSizEA(CompEnv *ce, int byte1, int byte2, Item *n)
{
    GenByte(ce, byte1 + GenSiz(ce, n->size));
    GenEA(ce, byte2, n);
}


void GenOpSizReg(CompEnv *ce, int byte1, int byte2, ModReg mr, long siz)
{
    sysAssert(siz > 1 || IsByteAccessible(mr));
    Gen2Bytes(ce, byte1 + GenSiz(ce, siz), byte2 + sibModRmTab[mr]);
}


void GenOpSizRegEA(CompEnv *ce, int byte1, ModReg mr, Item *n)
{
    sysAssert(n->size > 1 || IsByteAccessible(mr));
    GenByte(ce, byte1 + GenSiz(ce, n->size));
    GenEA(ce, reg3Map[mr], n);
}


void GenOpSizRegReg(CompEnv *ce, int byte1, ModReg mr1, ModReg mr2, long siz)
{
    sysAssert(siz > 1 || (IsByteAccessible(mr1) && IsByteAccessible(mr2)));
    Gen2Bytes(ce, byte1 + GenSiz(ce, siz), reg3Map[mr1] + sibModRmTab[mr2]);
}


void GenOpRegReg(CompEnv *ce, int byte1, ModReg mr1, ModReg mr2)
{
    Gen2Bytes(ce, byte1, reg3Map[mr1] + sibModRmTab[mr2]);
}


void GenFLoad(CompEnv *ce, Item *n)
{
    sysAssert(n->mr != MR_FST);
    sysAssert(!IsIntReg(n->mr));
    GenByte(ce, n->size == 4 ? 0xD9 : 0xDD);
    GenEA(ce, 0, n);
}


void GenFStore(CompEnv *ce, Item *n, int dontPopFlag)
{
    sysAssert(n->mr != MR_FST);
    sysAssert(!IsIntReg(n->mr));
    GenByte(ce, n->size == 4 ? 0xD9 : 0xDD);
    GenEA(ce, dontPopFlag ? 0x10 : 0x18, n);

/*  FWAIT is necessary if we want to get floating point exceptions at the right
    spot (or at all if the last fpu operation of the process raised the exception).
    Java disables all fpu exceptions anyway.

    GenByte(ce, FWAIT);
*/
}


void GenFOpEA(CompEnv *ce, int byte2, int byte2reg, Item *n)
{
    if (n->mr == MR_FST)
	Gen2Bytes(ce, 0xDE, 0xC1 + byte2reg);
    else
    {
	GenByte(ce, n->size == 4 ? 0xD8 : 0xDC);
	GenEA(ce, byte2, n);
    }
}


void GenRegRegMove(CompEnv *ce, ModReg dMr, ModReg sMr)
{
    if (sMr == dMr)	/* move to same reg? suppress */
	return;
    GenOpRegReg(ce, LOAD_L, dMr, sMr);
}


void GenAddImmRL(CompEnv *ce, ModReg mr, long val)
{
    if (val == 0)
	return;
    else if (val == 1)
	GenByte(ce, INCR_L + regMap[mr]);
    else if (val == -1)
	GenByte(ce, DECR_L + regMap[mr]);
    else if (ByteRange(val))
	Gen3Bytes(ce, ADDIB_L, sibModRmTab[mr] + 0x00, val);
    else if (mr == MR_EAX)
	GenByteLong(ce, ADDI_EAX, val);
    else
	Gen2BytesLong(ce, ADDI_L, sibModRmTab[mr] + 0x00, val);
}


void GenLea(CompEnv *ce, ModReg mr, Item *n)
{
    long    offs;
    /*
	optimize the cases where lea can be replaced by
	shorter or faster instructions:
	MR_ABS	is done by mov mr,imm32
	MR_BASE is done by add mr,imm8/imm32 if mr == RegOfMr(n->mr)
	MR_BASE is done by mov mr1,mr2 if offs == 0
    */
    if (n->mr == MR_ABS)
    {
	GenByteLong(ce, LOADI_L + regMap[mr], n->offset);
	return;
    }

    if (IsIndir(n->mr))
    {
	GenRegRegMove(ce, mr, BaseRegOf(n->mr));
	return;
    }
    else if (IsBased(n->mr))
    {
	offs = n->offset;
	if  (BaseRegOf(n->mr) == MR_ESP)
	    offs -= ce->ctxt->espLevel;
	if  (mr == BaseRegOf(n->mr))
	{
	    GenAddImmRL(ce, mr, offs);
	    return;
	}
	else if (offs == 0)
	{
	    GenRegRegMove(ce, mr, BaseRegOf(n->mr));
	    return;
	}
    }
    else if (IsIndexed(n->mr) && n->scale == 0 && BaseRegOf(n->mr) == mr)
    {
	if  (mr == MR_EAX)
	    GenByteLong(ce, ADDI_EAX, n->offset);
	else
	    Gen2BytesLong(ce, ADDI_L, sibModRmTab[mr], n->offset);
	return;
    }
    else if (  IsBasedIndexed(n->mr) && n->scale == 0
	    && BaseRegOf(n->mr) == mr && n->offset == 0)
    {
	/* Replace LEA reg1,[reg1+reg2] by ADD reg1,reg2 */
	GenOpRegReg(ce, ADDR_L, RegOfMr(n->mr), (n->mr - MR_BINX)>>3);
	return;
    }
    GenByte(ce, LEA_L);
    GenEA(ce, reg3Map[mr], n);
}


void GenIndirEA(CompEnv *ce, int byte1, int byte2, ModReg ptrReg, long off)
{
    GenByte(ce, byte1);
    byte2 += sibModRmTab[MR_BASE + ptrReg];
    if (off == 0 && ptrReg != MR_EBP)
	;
    else if (ByteRange(off))
	byte2 += 0x40;
    else
	byte2 += 0x80;
    if ((byte2 & 0x8000) == 0)
	GenWord(ce, byte2);
    else
	GenByte(ce, byte2);
    if (byte2 & 0x40)
	GenByte(ce, off);
    else if (byte2 & 0x80)
	GenLong(ce, off);
}


void GenImmVal(CompEnv *ce, long val, long siz)
{
    switch  (siz)
    {
    case    1:	GenByte(ce, val);   break;
    case    2:	GenWord(ce, val);   break;
    case    4:	GenLong(ce, val);   break;
    default:	sysAssert(0);
    }
}


void GenOpImmR(CompEnv *ce, int opimm, ModReg mr, long val, long siz)
{
    int     op;

    op = 0x81;
    if (GenSiz(ce, siz) == 0)
	op = 0x80;
    else if (ByteRange(val))
	op = 0x83;
    if (mr == MR_EAX && op != 0x83)
    {
	GenByte(ce, op - 0x80 + 0x04 + opimm);
	GenImmVal(ce, val, siz);
    }
    else
    {
	Gen2Bytes(ce, op, opimm + sibModRmTab[mr]);
	if  (op == 0x83)
	    GenByte(ce, val);
	else
	    GenImmVal(ce, val, siz);
    }
}


void GenOpImm(CompEnv *ce, int opimm, Item *n, long val)
{
    int     op;

    op = 0x81;
    if (GenSiz(ce, n->size) == 0)
	op = 0x80;
    else if (ByteRange(val))
	op = 0x83;
    if (n->mr == MR_EAX && op != 0x83)
    {
	GenByte(ce, op - 0x80 + 0x04 + opimm);
	GenImmVal(ce, val, n->size);
    }
    else
    {
	GenByte(ce, op);
	GenEA(ce, opimm, n);
	if  (op == 0x83)
	    GenByte(ce, val);
	else
	    GenImmVal(ce, val, n->size);
    }
}


void GenSubImmRSetCC(CompEnv *ce, ModReg mr, long val, long siz)
{
    if (val == 0)
	GenOpSizRegReg(ce, TESTM, mr, mr, siz);
    else
	GenOpImmR(ce, SUBI, mr, val, siz);
}


void GenLoad(CompEnv *ce, ModReg dMr, Item *s)
{
    sysAssert(s->size > 1 || !MrFitsTarget(dMr, (RS_ALL - RS_BYTE)));

    if (dMr == s->mr)	    /* move to same reg? suppress */
	return;

    if (IsIntReg(s->mr))   /* load from reg? move a long */
    {
	GenOpRegReg(ce, LOAD_L, dMr, s->mr);
	return;
    }

    if (s->mr == MR_FSABS)
    {
	GenByte(ce, FSSEG);
	s->mr = MR_ABS;
    }

    if (dMr == MR_EAX && s->mr == MR_ABS)
	GenByteLong(ce, LOADA_AL + GenSiz(ce, s->size), s->offset);

    else if (s->mr == MR_IMM)
    {
	if  (s->offset == 0)
	{
	    GenOpRegReg(ce, XORR_L, dMr, dMr);
	    return;
	}
	else if (s->offset == -1)
	{
	    Gen3Bytes(ce, 0x83, ORI + sibModRmTab[dMr], -1);
	    return;
	}
	GenByte(ce, LOADI_B + (GenSiz(ce, s->size)<<3) + regMap[dMr]);
	GenImmVal(ce, s->offset, s->size);
    }
    else
    {
	GenByte(ce, LOAD_B + GenSiz(ce, s->size));
	GenEA(ce, reg3Map[dMr], s);
    }
}


void GenStore(CompEnv *ce, Item *d, ModReg sMr)
{
    int     sizBit;

    if (sMr == d->mr)	    /* move to same reg? suppress */
	return;

    if (d->mr == MR_FSABS)
    {
	GenByte(ce, FSSEG);
	d->mr = MR_ABS;
    }

    sizBit = GenSiz(ce, d->size);
    if (sMr == MR_EAX && d->mr == MR_ABS)
	GenByteLong(ce, STOREA_AL + sizBit, d->offset);

    else
    {
	GenByte(ce, STORE_B + sizBit);
	sysAssert(d->size > 1 || !MrFitsTarget(sMr, (RS_ALL - RS_BYTE)));
	GenEA(ce, reg3Map[sMr], d);
    }
}


void GenExg(CompEnv *ce, ModReg mr1, ModReg mr2)
{
    if (mr1 == MR_EAX)
	GenByte(ce, XCHG_EAX + regMap[mr2]);
    else if (mr2 == MR_EAX)
	GenByte(ce, XCHG_EAX + regMap[mr1]);
    else
	GenOpRegReg(ce, XCHG_L, mr1, mr2);
}


void GenCmpRegEA(CompEnv *ce, ModReg mr, Item *r)
{
    char    siz;

    if (r->mr == MR_IMM)
    {
	siz = GenSiz(ce, r->size);
	if  (mr == MR_EAX)
	    GenByte(ce, CMPI_AL + siz);
	else
	    Gen2Bytes(ce, 0x80 + siz, CMPI + sibModRmTab[mr]);
	GenImmVal(ce, r->offset, r->size);
    }
    else
	GenOpSizRegEA(ce, CMPR, mr, r);
}


void ShiftLeft(CompEnv *ce, ModReg mr, long power)
{
    if (power == 0)
	;
    else if (power == 1)
	GenOpRegReg(ce, ADDR_L, mr, mr);
    else
	Gen3Bytes(ce, 0xC1,  SHL + sibModRmTab[mr], power);
}


void ShiftRight(CompEnv *ce, ModReg mr, long power, long size, int signedFlag)
{
    char    byte1, byte2;

    byte1 = GenSiz(ce, size);

    byte2 = SHR + sibModRmTab[mr];
    if (signedFlag)
	byte2 += SAR - SHR;
    if (power == 1)
	Gen2Bytes(ce, byte1 + 0xD0, byte2);
    else
	Gen3Bytes(ce, byte1 + 0xC0, byte2, power);
}


int PowerOf(unsigned long val)
{
    int power;

    for (power = 0; (val & 1) == 0; val >>= 1)
	power++;
    return(power);
}


char	lowestBit[256];
char	bitCnt[256];

static int LowestBit(int i)
{
    int     b;

    if (i == 0)
	return(-1);
    for (b = 0; (i & 1) == 0; i >>= 1, b++)
	;
    return(b);
}


static int BitCnt(int i)
{
    int     b;

    for (b = 0; i; i >>= 1)
	b += i & 1;
    return(b);
}


void InitCodeGen(void)
{
    int     i;

    for (i = 0; i < 256; i++)
    {
	lowestBit[i] = LowestBit(i);
	bitCnt[i] = BitCnt(i);
    }
}



