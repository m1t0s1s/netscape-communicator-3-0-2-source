// $Header: /m/src/ns/sun-java/jit/win32/borland/jcodegen.h,v 1.1.10.1 1996/09/29 20:54:07 jg Exp $ 
/*
 * Copyright (c) 1996 Borland International. All Rights Reserved.
 *
 * AppAccelerator(tm) for x86
 *
 * JCodeGen.h, R. Crelier, 1/29/96
 *
*/

/*
    Addressing modes.
    Note that the arrangement is such that the base register number
    is always the last three bits.
*/
typedef enum
{
    MR_NONE = -1,

    MR_EAX,		    /* AL, AX, EAX */
    MR_EDX,
    MR_ECX,
    MR_EBX,
    MR_ESI,
    MR_EDI,
    MR_EBP,
    MR_ESP,

    MR_INDIR,		    /* [reg]			    */

    MR_BASE = MR_INDIR + 8, /* [reg + disp]		    */

    MR_INX  = MR_BASE + 8,  /* [reg*scale + disp]	    */

    MR_BINX = MR_INX + 8,   /* [reg1 + reg2*scale + disp]   */

    MR_ABS  = MR_BINX + 8*8,/* [disp32] 		    */

    MR_IMM,		    /* Immediate		    */

    MR_FST,		    /* on floating point stack	    */

    MR_TOS,		    /* spilled to the stack	    */

    MR_FSABS,		    /* addressed via fs: (exception handling) */

    MR_CC,		    /* in condition code	    */

    MR_LAST,

}   ModReg;

#define IsByteReg(mr)	    ((mr) <= MR_EBX)
#define IsIntReg(mr)	    ((mr) <= MR_ESP)
#define IsCallerSaved(mr)   ((mr) <= MR_ECX)
#define IsIndir(mr)	    ((unsigned)((mr) - MR_INDIR) < 8)
#define IsBased(mr)	    ((unsigned)((mr) - MR_BASE)  < 8)
#define IsIndexed(mr)	    ((unsigned)((mr) - MR_INX)	 < 8)
#define IsBasedOrIndexed(mr)((unsigned)((mr) - MR_BASE)  < 16)
#define IsBasedIndexed(mr)  ((unsigned)((mr) - MR_BINX)  < 64)
#define IsRelMem(mr)	    ((unsigned)((mr) - MR_BASE)  < MR_ABS - MR_BASE)
#define IsMem(mr)	    ((unsigned)((mr) - MR_INDIR) < MR_IMM - MR_INDIR)
#define HasIndex(mr)	    ((unsigned)((mr) - MR_INX)	 < MR_ABS - MR_INX)
#define BaseRegOf(mr)	    ((mr) & 0x07)

#define ByteRange(l)	((long)(char)(l) == (l))

#define IsSubSet(s1,s2) (((s1) & ~(s2)) == 0)
#define IsPower(l)	(((l) & (l-1)) == 0)

#define RegOfMr(mr)	(regNumOfMr[mr])
#define RegSetOfMr(mr)	(regSetOfMr[mr])
#define MrFitsTarget(mr,t)  (targOfMr[mr] & (t))

#define IsByteAccessible(mr)	(!MrFitsTarget(mr, (RS_ALL - RS_BYTE)))


typedef unsigned    RegSet;

enum
{
    RS_EAX  = 1<<MR_EAX,
    RS_EDX  = 1<<MR_EDX,
    RS_ECX  = 1<<MR_ECX,
    RS_EBX  = 1<<MR_EBX,

    RS_ESI  = 1<<MR_ESI,
    RS_EDI  = 1<<MR_EDI,
    RS_EBP  = 1<<MR_EBP,
    RS_ESP  = 1<<MR_ESP,

    RS_BYTE	    = RS_EAX|RS_EDX|RS_ECX|RS_EBX,
    RS_ALL	    = RS_BYTE|RS_ESI|RS_EDI|RS_EBP|RS_ESP,
    RS_CALLEE_SAVED = RS_EBX|RS_ESI|RS_EDI |RS_EBP,
    RS_CALLER_SAVED = RS_EAX|RS_EDX|RS_ECX,

    RS_EMPTY= 0x0000,

};

/*
    this table finds the set of registers associated with an
    address mode. This is useful for freeing up registers
    used in address modes.
*/
extern	RegSet	regSetOfMr[MR_LAST];

/*
    this table gives the target register set corresponding
    to a given address mode. this is used for determining
    whether a given address mode fits a target.
*/
extern	RegSet	targOfMr[MR_LAST];

extern	char	lowestBit[256];
extern	char	bitCnt[256];


enum
{
    ADCI	= 0x10,     /* 2nd byte adc r/m,imm */
    ADCR	= 0x12,     /* adc r,r/m	    */
    ADDM	= 0x00,     /* add r/m,r	    */
    ADDR	= 0x02,     /* add r,r/m	    */
    ADDR_L	= 0x03,     /* add r32,r/m32	    */
    ADDI	= 0x00,     /* 2nd byte add r/m,imm */
    ADDI_AL	= 0x04,     /* add al,imm8	    */
    ADDI_EAX	= 0x05,     /* add eax,imm32	    */
    ADDIB_L	= 0x83,     /* add r/m32,imm8	    */
    ADDI_L	= 0x81,     /* add r/m32,imm32	    */
    ANDM	= 0x20,     /* and r/m,r	    */
    ANDR	= 0x22,     /* and r,r/m	    */
    ANDI	= 0x20,     /* 2nd byte add r/m,imm */
    BOUND	= 0x62,     /* bound		    */
    BT		= 0xA3,     /* 2nd byte of bt ,r    */
    BTR 	= 0xB3,     /* 2nd byte of btr ,r   */
    BTS 	= 0xAB,     /* 2nd byte of bts ,r   */
    BTSI	= 0xBA,     /* 2nd byte of bts ,imm */
    CALLI	= 0x10,     /* 2nd byte call r/m32  */
    CALLN	= 0xE8,     /* call rel32	    */
    CBW 	= 0x98,     /* cbw		    */
    CDQ 	= 0x99,     /* cdq		    */
    CWD 	= 0x99,     /* cwd		    */
    CMPI	= 0x38,     /* 2nd byte cmp r/m,imm */
    CMPI_AL	= 0x3C,     /* cmp al,imm	    */
    CMPR	= 0x3A,     /* cmp r,r/m	    */
    CMPSB	= 0xA6,     /* cmpsb		    */
    DEC2	= 0x08,     /* 2nd byte dec r/m     */
    DECR_L	= 0x48,     /* dec r32		    */
    DIV2	= 0x30,     /* 2nd byte div ax,r/m  */
    FABS	= 0xE1,     /* 2nd byte fabs	    */
    FADD	= 0x00,     /* 2nd byte fadd	    */
    FCHS	= 0xE0,     /* 2nd byte fchs	    */
    FCOMP	= 0x18,     /* 2nd byte fcomp	    */
    FDIV	= 0x30,     /* 2nd byte fdiv	    */
    FDIVR	= 0x38,     /* 2nd byte fdivr	    */
    FILD	= 0x00,     /* 2nd byte fild	    */
    FILD_Q	= 0x28,     /* 2nd byte fild qword  */
    FLD1	= 0xE8,     /* 2nd byte fld1	    */
    FLDLN2	= 0xED,     /* 2nd byte fldln2	    */
    FMUL	= 0x08,     /* 2nd byte fmul	    */
    FNSTSTWAX	= 0xE0,     /* 2nd byte fnstsw ax   */
    FPATAN	= 0xF3,     /* 2nd byte fpatan	    */
    FSQRT	= 0xFA,     /* 2nd byte fsqrt	    */
    FSSEG	= 0x64,     /* FS: segment override */
    FSUB	= 0x20,     /* 2nd byte fsub	    */
    FSUBR	= 0x28,     /* 2nd byte fsubr	    */
    FWAIT	= 0x9B,     /* fwait		    */
    FXCH1	= 0xC9,     /* 2nd byte fxch st(1)  */
    FYL2X	= 0xF1,     /* 2nd byte fyl2x	    */
    IDIV	= 0x38,     /* 2nd byte idiv ax,r/m */
    IMUL_AL	= 0x28,     /* 2nd byte imul r/m8   */
    IMUL	= 0xAF,     /* 2nd byte imul r,r/m  */
    IMULI	= 0x69,     /* imul r32,r/m32,imm32 */
    IMULIB	= 0x6B,     /* imul r32,r/m32,imm8  */
    INC2	= 0x00,     /* 2nd byte inc r/m     */
    INCR_L	= 0x40,     /* inc r32		    */
    JMPI	= 0x20,     /* jmp r/m32	    */
    JMPN	= 0xE9,     /* jmp rel32	    */
    JB		= 0x72,     /* jb rel8		    */
    JBE 	= 0x76,     /* jbe rel8 	    */
    JNB 	= 0x73,     /* jnb rel8 	    */
    JNO 	= 0x71,     /* jno rel8 	    */
    JNS 	= 0x79,     /* jns rel8 	    */
    JNZ 	= 0x75,     /* jnz rel8 	    */
    LEA_L	= 0x8D,     /* lea r32,mem	    */
    LEAVE	= 0xC9,     /* leave		    */
    LOADA_AL	= 0xA0,     /* mov al,r/m8	    */
    LOADI_B	= 0xB0,     /* mov r8,imm8	    */
    LOADI_L	= 0xB8,     /* mov r32,imm32	    */
    LOAD_B	= 0x8A,     /* mov r8,r/m8	    */
    LOAD_L	= 0x8B,     /* mov r32,r/m32	    */
    LOAD_SEG	= 0x8E,     /* mov seg,r/m32	    */
    MOVI	= 0xC6,     /* mov r/m8,imm8	    */
    MOVI_L	= 0xC7,     /* mov r/m32,imm32	    */
    MOVSB	= 0xA4,     /* movsb		    */
    MOVSD	= 0xA5,     /* movsd		    */
    NEG 	= 0x18,     /* 2nd byte neg r/m     */
    NOT2	= 0x10,     /* 2nd byte not r/m     */
    OPNDSIZE	= 0x66,     /* opnd size prefix     */
    ORM 	= 0x08,     /* or r/m,r 	    */
    ORR 	= 0x0A,     /* or r,r/m 	    */
    ORR_B	= 0x0A,     /* or r8,r/m8	    */
    ORI 	= 0x08,     /* 2nd byte or r/m,imm  */
    POP_L	= 0x00,     /* 2nd byte pop m32     */
    POPFD	= 0x9D,     /* popfd		    */
    POPR_L	= 0x58,     /* pop r32		    */
    PUSHFD	= 0x9C,     /* pushfd		    */
    PUSHR_L	= 0x50,     /* push r32 	    */
    PUSH_L	= 0x30,     /* 2nd byte push r/m32  */
    PUSHI_B	= 0x6A,     /* push imm8	    */
    PUSHI_L	= 0x68,     /* push imm32	    */
    REP 	= 0xF3,     /* rep		    */
    REPE	= 0xF3,     /* repe		    */
    REPNE	= 0xF2,     /* repne		    */
    RET 	= 0xC3,     /* ret		    */
    RETF	= 0xCB,     /* retf		    */
    RETP	= 0xC2,     /* ret imm16	    */
    SAHF	= 0x9E,     /* sahf		    */
    SAR 	= 0x38,     /* 2nd byte of sar	    */
    SBBI	= 0x18,     /* 2nd byte sbb r/m,imm */
    SBBR	= 0x1A,     /* sbb r,r/m	    */
    SBBR_L	= 0x1B,     /* sbb r32,r/m32	    */
    SCASB	= 0xAE,     /* scasb		    */
    SET 	= 0x90,     /* 2nd byte of set	    */
    SHL 	= 0x20,     /* 2nd byte of shl	    */
    SHR 	= 0x28,     /* 2nd byte of shr	    */
    STOREA_AL	= 0xA2,     /* mov r/m8,al	    */
    STORE_B	= 0x88,     /* mov r/m8,r8	    */
    STORE_L	= 0x89,     /* mov r/m32,r32	    */
    STORE_SEG	= 0x8C,     /* mov r/m32,seg	    */
    SUBM	= 0x28,     /* sub r/m,r	    */
    SUBR	= 0x2A,     /* sub r,r/m	    */
    SUBR_L	= 0x2B,     /* sub r32,r/m32	    */
    SUBI_AL	= 0x2C,     /* sub al,imm	    */
    SUBI	= 0x28,     /* 2nd byte sub r/m,imm */
    TESTI	= 0x00,     /* 2nd byte test r/m,imm*/
    TESTI_AL	= 0xA8,     /* test al,imm8	    */
    TESTM	= 0x84,     /* test r/m8,r8	    */
    XCHG_EAX	= 0x90,     /* xchg eax,r32	    */
    XCHG_B	= 0x86,     /* xchg r8,r/m8	    */
    XCHG_L	= 0x87,     /* xchg r32,r/m32	    */
    XORM	= 0x30,     /* xor r/m,r	    */
    XORR	= 0x32,     /* xor r,r/m	    */
    XORI	= 0x30,     /* 2nd byte xor r/m,imm */
    XORI_AL	= 0x34,     /* xor al,imm8	    */
    XORR_L	= 0x33,     /* xor r32,r/m32	    */
};

enum	RegisterEncoding
{
    RC_EAX  =	0x00,
    RC_ECX  =	0x01,
    RC_EDX  =	0x02,
    RC_EBX  =	0x03,
    RC_ESP  =	0x04,
    RC_EBP  =	0x05,
    RC_ESI  =	0x06,
    RC_EDI  =	0x07,

    RC_AL   =	0x00,
    RC_CL   =	0x01,
    RC_DL   =	0x02,
    RC_BL   =	0x03,
    RC_AH   =	0x04,
    RC_CH   =	0x05,
    RC_DH   =	0x06,
    RC_BH   =	0x07,
};

/* this table contains the register encoding */
extern char regMap[MR_ESP+1];

/* this table contains the register encoding shifted left three bits */
extern char reg3Map[MR_ESP+1];

/* this table facilitates encoding of address modes.	*/
/* the low byte contains part of the mod r/m byte,	*/
/* the high byte is the low 6 bits of the s-i-b byte	*/
/* bit 15 on means no s-i-b, -1 is illegal address mode */
unsigned short	sibModRmTab[MR_LAST];

enum	EncodingMask
{
    EM_REG	= 0x38,
    EM_MODRM	= 0xC7,
};

#ifdef	CC_NONE     // from wingdi.h
#undef	CC_NONE
#endif

typedef enum	CondCode
{
    CC_NONE = -1,

    CC_O    = 0x00,
    CC_NO   = 0x01,
    CC_B    = 0x02,
    CC_AE   = 0x03,
    CC_E    = 0x04,
    CC_NE   = 0x05,
    CC_BE   = 0x06,
    CC_A    = 0x07,
    CC_S    = 0x08,
    CC_NS   = 0x09,
    CC_PE   = 0x0A,
    CC_PO   = 0x0B,
    CC_L    = 0x0C,
    CC_GE   = 0x0D,
    CC_LE   = 0x0E,
    CC_G    = 0x0F,

    CC_JMP  = 0x10,
    CC_JSR  = 0x11,

}   CondCode;

extern CondCode swapCC[16];
extern CondCode xsgnCC[16];
extern CondCode highCC[16];
extern CondCode revhCC[16];

typedef struct CompEnv;
typedef struct Item;


void GenByte(struct CompEnv *ce, char b);
void Gen2Bytes(struct CompEnv *ce, char b1, char b2);
void Gen3Bytes(struct CompEnv *ce, char b1, char b2, char b3);
void GenByteLong(struct CompEnv *ce, char b1, long l2);
void Gen2BytesLong(struct CompEnv *ce, char b1, char b2, long l2);
void GenWord(struct CompEnv *ce, short w);
void GenLong(struct CompEnv *ce, long l);
void GenEA(struct CompEnv *ce, int r, struct Item *n);
int  GenSiz(struct CompEnv *ce, long siz);
void GenOpSizEA(struct CompEnv *ce, int byte1, int byte2, struct Item *n);
void GenOpSizReg(struct CompEnv *ce, int byte1, int byte2, ModReg mr, long siz);
void GenOpSizRegEA(struct CompEnv *ce, int byte1, ModReg mr, struct Item *n);
void GenOpSizRegReg(struct CompEnv *ce, int byte1, ModReg mr1, ModReg mr2, long siz);
void GenOpRegReg(struct CompEnv *ce, int byte1, ModReg mr1, ModReg mr2);
void GenFLoad(struct CompEnv *ce, struct Item *n);
void GenFStore(struct CompEnv *ce, struct Item *n, int dontPopFlag);
void GenFOpEA(struct CompEnv *ce, int byte2, int byte2reg, struct Item *n);
void GenRegRegMove(struct CompEnv *ce, ModReg dMr, ModReg sMr);
void GenAddImmRL(struct CompEnv *ce, ModReg mr, long val);
void GenLea(struct CompEnv *ce, ModReg mr, struct Item *n);
void GenIndirEA(struct CompEnv *ce, int byte1, int byte2, ModReg ptrReg, long off);
void GenImmVal(struct CompEnv *ce, long val, long siz);
void GenOpImmR(struct CompEnv *ce, int opimm, ModReg mr, long val, long siz);
void GenOpImm(struct CompEnv *ce, int opimm, struct Item *n, long val);
void GenSubImmRSetCC(struct CompEnv *ce, ModReg mr, long val, long siz);
void GenLoad(struct CompEnv *ce, ModReg dMr, struct Item *s);
void GenStore(struct CompEnv *ce, struct Item *d, ModReg sMr);
void GenExg(struct CompEnv *ce, ModReg mr1, ModReg mr2);
void GenCmpRegEA(struct CompEnv *ce, ModReg mr, struct Item *r);
void ShiftLeft(struct CompEnv *ce, ModReg mr, long power);
void ShiftRight(struct CompEnv *ce, ModReg mr, long power, long size, int signedFlag);

int PowerOf(unsigned long val);

void InitCodeGen(void);

