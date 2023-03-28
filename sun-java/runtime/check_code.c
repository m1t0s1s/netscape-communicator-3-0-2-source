/*
 * @(#)check_code.c	1.51 95/12/02
 *
 * Copyright (c) 1994 Sun Microsystems, Inc. All Rights Reserved.
 *
 * Permission to use, copy, modify, and distribute this software
 * and its documentation for NON-COMMERCIAL purposes and without
 * fee is hereby granted provided that this copyright notice
 * appears in all copies. Please refer to the file "copyright.html"
 * for further important copyright and licensing information.
 *
 * SUN MAKES NO REPRESENTATIONS OR WARRANTIES ABOUT THE SUITABILITY OF
 * THE SOFTWARE, EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED
 * TO THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
 * PARTICULAR PURPOSE, OR NON-INFRINGEMENT. SUN SHALL NOT BE LIABLE FOR
 * ANY DAMAGES SUFFERED BY LICENSEE AS A RESULT OF USING, MODIFYING OR
 * DISTRIBUTING THIS SOFTWARE OR ITS DERIVATIVES.
 */
 
/*-
 *      Verify that the code within a method block doesn't exploit any 
 *      security holes.
 *
 *      This code is still a work in progress.  All currently existing code
 *      passes the test, but so does a lot of bad code.
 */

#include <setjmp.h>

#include "oobj.h"
#include "interpreter.h"
#include "opcodes.h"
#include "opcodes.length"
#include "opcodes.in_out"
#include "bool.h"
#include "tree.h"
#include "byteorder.h"
#include "limits.h"
#include "sys_api.h"
#include "limits_md.h"

#include "str2id.h"
#include "prgc.h"


#define MAX_ARRAY_DIMENSIONS 255

#if defined(XP_PC) && !defined(_WIN32)
#if !defined(stderr)
extern FILE *stderr;
#endif
#if !defined(stdout)
extern FILE *stdout;
#endif
#endif

extern char *opnames[];	/* defined in the auto-generated file opcodes.c */

#ifdef DEBUG
    int verify_verbose = 0;
    static struct context_type *GlobalContext;
#endif

enum {
    ITEM_Bogus,
    ITEM_Void,			/* only as a function return value */
    ITEM_Integer,
    ITEM_Float,
    ITEM_Double,
    ITEM_Double_2,		/* 2nd word of double in register */
    ITEM_Long,
    ITEM_Long_2,		/* 2nd word of long in register */
    ITEM_Array,
    ITEM_Object,		/* Extra info field gives name. */
    ITEM_NewObject,		/* Like object, but uninitialized. */
    ITEM_InitObject,		/* "this" is init method, before call 
				    to super() */
    ITEM_ReturnAddress,		/* Extra info gives instr # of start pc */
    /* The following three are only used within array types. 
     * Normally, we use ITEM_Integer, instead. */
    ITEM_Byte,
    ITEM_Short,
    ITEM_Char
};


#define UNKNOWN_STACK_SIZE -1
#define UNKNOWN_REGISTER_COUNT -1
#define UNKNOWN_RET_INSTRUCTION -1

#undef MAX
#undef MIN 
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))

#define BITS_PER_INT   (CHAR_BIT * sizeof(int)/sizeof(char))
#define SET_BIT(flags, i)  (flags[(i)/BITS_PER_INT] |= \
			               ((unsigned)1 << ((i) % BITS_PER_INT)))
#define	IS_BIT_SET(flags, i) (flags[(i)/BITS_PER_INT] & \
			               ((unsigned)1 << ((i) % BITS_PER_INT)))

typedef unsigned long fullinfo_type;
typedef unsigned int *bitvector;

#define GET_ITEM_TYPE(thing) ((thing) & 0x1F)
#define GET_INDIRECTION(thing) (((thing) & 0xFFFF) >> 5)
#define GET_EXTRA_INFO(thing) (((unsigned long)(thing)) >> 16)
#define WITH_ZERO_INDIRECTION(thing) ((thing) & ~(0xFFE0l))
#define WITH_ZERO_EXTRA_INFO(thing) ((thing) & 0xFFFF)

#define MAKE_FULLINFO(type, indirect, extra) \
     ((type) + (((unsigned long)indirect) << 5) + (((unsigned long)extra) << 16))
#define MAKE_CLASSNAME_INFO(classname, addr) \
       MAKE_FULLINFO(ITEM_Object, 0, \
                      Str2ID(&context->classHash, (classname), (addr), FALSE))
#define MAKE_CLASSNAME_INFO_WITH_COPY(classname) \
       MAKE_FULLINFO(ITEM_Object, 0, \
                      Str2ID(&context->classHash, (classname), 0, TRUE))
#define MAKE_Object_ARRAY(indirect) \
       (context->object_info + (((unsigned long)indirect) << 5))

/*
 * Larger integer access macros (lifted from zip.c); drt 03/05/96
 */
#define UCP(x) ((unsigned char *)(x))
#define CH(b, n)  (UCP(b)[n])
#define USCH(b,n) ((unsigned short)CH(b,n))
#define SH(b, n)  ( (USCH(b, n) << 8) | CH(b, n+1) )
#define ULSH(b,n) ( (unsigned long)SH(b,n) )
#define LG(b, n)  ( (ULSH(b,n) << 16 ) | SH(b,n+2) )

#define NULL_FULLINFO MAKE_FULLINFO(ITEM_Object, 0, 0)

/* opc_invokenonvirtual calls to <init> need to be treated special */
#define opc_invokeinit 0x100	


struct context_type {
    /* these fields are per class */
    ClassClass *class;		/* current class */
    struct StrIDhash *classHash;
    fullinfo_type object_info;	/* fullinfo for java/lang/Object */
    fullinfo_type string_info;	/* fullinfo for java/lang/String */
    fullinfo_type throwable_info; /* fullinfo for java/lang/Throwable */

    fullinfo_type currentclass_info; /* fullinfo for context->class */
    fullinfo_type superclass_info;   /* fullinfo for superclass */

    /* these fields are per method */
    struct methodblock *mb;	/* current method */
    unsigned char *code;	/* current code object */
    short *code_data;		/* offset to instruction number */
    struct instruction_data_type *instruction_data; /* info about each */
    struct handler_info_type *handler_info;
    fullinfo_type *superClasses; /* null terminated superclasses */
    int32_t instruction_count;	/* number of instructions */
    fullinfo_type return_type;	/* function return type */
    fullinfo_type swap_table[4]; /* used for passing information */
    int bitmask_size;		/* words needed to hold bitmap of arguments */

    /* Used by the space allocator */
    struct CCpool *CCroot, *CCcurrent;
    char *CCfree_ptr;
    uint32_t CCfree_size;

    bool_t need_constructor_call; /* a constructor must call super() or this()*/

    /* Jump here on any error. */
    jmp_buf jump_buffer;
};


struct stack_info_type {
    struct stack_item_type *stack;
    int32_t stack_size;
};

struct register_info_type {
    int32_t register_count;		/* number of registers used */
    fullinfo_type *registers;
    int32_t mask_count;		/* number of masks in the following */
    struct mask_type *masks;
};

struct mask_type {
    codepos_t entry;
    int32_t *modifies;
};


struct instruction_data_type {
    opcode_type opcode;		/* may turn into "canonical" opcode */
    unsigned changed:1;		/* has it changed */
    unsigned protected:1;	/* must accessor be a subclass of "this" */
    union {
	int32_t i;			/* operand to the opcode */
	uint32_t u;
	int32_t *ip;
	fullinfo_type fi;
    } operand, operand2;
    fullinfo_type p;
    struct stack_info_type stack_info;
    struct register_info_type register_info;
};

struct handler_info_type {
    int start, end, handler;
    struct stack_info_type stack_info;
};

struct stack_item_type {
    fullinfo_type item;
    struct stack_item_type *next;
};


typedef struct context_type context_type;
typedef struct instruction_data_type instruction_data_type;
typedef struct stack_item_type stack_item_type;
typedef struct register_info_type register_info_type;
typedef struct stack_info_type stack_info_type;
typedef struct mask_type mask_type;

static void verify_method(context_type *context, struct methodblock *mb);
static void verify_field(context_type *context, struct fieldblock *fb);

static void verify_opcode_operands (context_type *, codepos_t inumber, codepos_t offset);
static void set_protected(context_type *, codepos_t inumber, cp_index_type key, opcode_type);
static bool_t isSuperClass(context_type *, fullinfo_type);

static void initialize_exception_table(context_type *);
static int32_t instruction_length(unsigned char *iptr, uint32_t offset);
static bool_t isLegalTarget(context_type *, codepos_t offset);
static void verify_constant_pool_type(context_type *, cp_index_type, unsigned);

static void initialize_dataflow(context_type *);
static void run_dataflow(context_type *context);
static void check_register_values(context_type *context, int inumber);
static void pop_stack(context_type *, int inumber, stack_info_type *);
static void update_registers(context_type *, int inumber, register_info_type *);
static void push_stack(context_type *, int inumber, stack_info_type *stack);

static void merge_into_successors(context_type *, int inumber, 
				  register_info_type *register_info,
				  stack_info_type *stack_info);
static void merge_into_one_successor(context_type *context, 
				     int32_t from_inumber, int32_t inumber, 
				     register_info_type *register_info,
				     stack_info_type *stack_info, 
				     bool_t isException);
static void merge_stack(context_type *, int32_t inumber, int32_t to_inumber, 
			stack_info_type *);
static void merge_registers(context_type *, int32_t inumber, int32_t to_inumber, 
			    register_info_type *);


static stack_item_type *copy_stack(context_type *, stack_item_type *);
static mask_type *copy_masks(context_type *, mask_type *masks, int32_t mask_count);
static mask_type *add_to_masks(context_type *, mask_type *, int32_t , int32_t);

static fullinfo_type decrement_indirection(fullinfo_type);

static fullinfo_type merge_fullinfo_types(context_type *context, 
					  fullinfo_type a, fullinfo_type b,
					  bool_t assignment);
static bool_t isAssignableTo(context_type *,fullinfo_type a, fullinfo_type b);

static ClassClass *object_fullinfo_to_classclass(context_type *, fullinfo_type);


#define NEW(type, count) \
        ((type *)CCalloc(context, (count)*(sizeof(type)), FALSE))
#define ZNEW(type, count) \
        ((type *)CCalloc(context, (count)*(sizeof(type)), TRUE))

static void CCinit(context_type *context);
static void CCreinit(context_type *context);
static void CCdestroy(context_type *context);
static void *CCalloc(context_type *context, uint32_t size, bool_t zero);

static char *cp_index_to_fieldname(context_type *context, cp_index_type cp_index);
static char *cp_index_to_signature(context_type *context, cp_index_type cp_index);
static fullinfo_type cp_index_to_class_fullinfo(context_type *, cp_index_type, bool_t);

static char signature_to_fieldtype(context_type *context, 
				   char **signature_p, fullinfo_type *info);

static void CCerror (context_type *, char *format, ...);

#ifdef DEBUG
static void print_stack (context_type *, stack_info_type *stack_info);
static void print_registers(context_type *, register_info_type *register_info);
static void print_formatted_fieldname(context_type *context, cp_index_type index);
#endif

/************************************************************************/

extern GCInfo *gcInfo;

void ScanVerifyContext(void *ptr)
{
    void (*liveObject)(void **base, int32 count);
    context_type *cx = (context_type*) ptr;

    liveObject = gcInfo->liveObject;

    if (cx->class) {
        (*liveObject)((void**) &cx->class, 1);
    }

    /* Scan over classHash because it has pointers to ClassClass's */
    if (cx->classHash) {
        StrIDhash *next, *hash = cx->classHash;
        while (hash) {
            next = hash->next;
            if (hash->param) {
                (*liveObject)(hash->param, STR2ID_HTSIZE);
            }
            hash = next;
        }
    }
}

extern void *AllocContext(size_t size);

/************************************************************************/
/* Called by verify_class.  Verify the code of each of the methods
 * in a class.
 */

bool_t verify_class_codes(ClassClass *cb) {
    context_type *context = AllocContext(sizeof(context_type));
    bool_t result = TRUE;
    void **addr;
    int i;

#ifdef DEBUG
    GlobalContext = context;
#endif

    /* Initialize the class-wide fields of the context. */
    context->class = cb;
    context->classHash = 0;
    context->object_info = MAKE_CLASSNAME_INFO(JAVAPKG "Object", &addr); 
    *addr = classJavaLangObject;
    context->string_info = MAKE_CLASSNAME_INFO(JAVAPKG "String", &addr);
    *addr = classJavaLangString;
    context->throwable_info = MAKE_CLASSNAME_INFO(JAVAPKG "Throwable", &addr);
    *addr = classJavaLangThrowable;

    context->currentclass_info = MAKE_CLASSNAME_INFO(cb->name, &addr);
    context->superClasses = NULL; /* filled in later */
    *addr = cb;
    if (cbSuperclass(cb) != 0) {
	ClassClass *super = unhand(cbSuperclass(cb));
	context->superclass_info = MAKE_CLASSNAME_INFO(super->name, &addr);
	*addr = super;
    } else { 
	context->superclass_info = 0;
    }
    
    /* Look at each method */
    if (!setjmp(context->jump_buffer)) {
	struct methodblock *mb;
	struct fieldblock *fb;
	CCinit(context);		/* ininitialize heap */
	for (i = cb->fields_count, fb = cb->fields; --i >= 0; fb++) 
	    verify_field(context, fb);
	for (i = cb->methods_count, mb = cb->methods; --i >= 0; mb++) 
		verify_method(context, mb);
	result = TRUE;
    } else { 
	result = FALSE;
    }

    /* Cleanup */
    Str2IDFree(&context->classHash);

#ifdef DEBUG
    GlobalContext = 0;
#endif
 
    if (context->superClasses != NULL) 
	sysFree(context->superClasses);
    CCdestroy(context);		/* destroy heap */

    /* Zap pointer to context to hide it from the GC */
    memset(&context, 0, sizeof(context));

    return result;
}

static void
verify_field(context_type *context, struct fieldblock *fb)
{
    #define BUF_LEN	1024
    char buf[BUF_LEN];
    int access_bits = fb->access;

    if (  ((access_bits & ACC_PUBLIC) != 0) && 
	  ((access_bits & (ACC_PRIVATE | ACC_PROTECTED)) != 0)) {

	jio_snprintf(buf, BUF_LEN, "VERIFIER ERROR %s.%s: Inconsistent access bits.\n", 
		     fieldclass(fb)->name, fb->name);
#ifdef DEBUG
	fprintf(stderr, buf);
#endif /* DEBUG */
	PrintToConsole(buf);
	longjmp(context->jump_buffer, 1);
    }
}

/* Verify the code of one method */

static void
verify_method(context_type *context, struct methodblock *mb)
{
    int access_bits = mb->fb.access;
    unsigned char *code = mb->code;
    codepos_t code_length = mb->code_length;
    short *code_data;
    instruction_data_type *idata = 0;
    int instruction_count;
    codepos_t offset;
    codepos_t inumber;			 
    int i;

    CCreinit(context);		/* initial heap */
    code_data = NEW(short, code_length);

#ifdef DEBUG
    if (verify_verbose) {
	printf("Looking at %s.%s%s 0x%x\n", 
	       fieldclass(&mb->fb)->name, mb->fb.name, mb->fb.signature, 
	       (long)mb);
    }
#endif
    /* Initialize enough of the context to be able to call CCerror */
    context->mb = mb;

    if (((access_bits & ACC_PUBLIC) != 0) && 
	((access_bits & (ACC_PRIVATE | ACC_PROTECTED)) != 0)) {
	CCerror(context, "Inconsistent access bits.");
    } 

    if ((access_bits & (ACC_NATIVE | ACC_ABSTRACT)) != 0) { 
	/* not much to do for abstract and native methods */
	return;
    }


    /* Run through the code.  Mark the start of each instruction, and give
     * the instruction a number */
    for (i = 0, offset = 0; offset < code_length; i++) {
	int32_t length = instruction_length(code, offset);
	codepos_t next_offset = offset + (codepos_t)length;
	if (length <= 0) 
	    CCerror(context, "Illegal instruction found at offset %d", offset);
	if (next_offset > code_length) 
	    CCerror(context, "Code stops in the middle of instruction "
		    " starting at offset %d", offset);
	code_data[offset] = i;
	while (++offset < next_offset)
	    code_data[offset] = -1; /* illegal location */
    }
    instruction_count = i;	/* number of instructions in code */
    
    /* Allocate a structure to hold info about each instruction. */
    idata = NEW(instruction_data_type, instruction_count);

    /* Initialize the heap, and other info in the context structure. */
    context->code = code;
    context->instruction_data = idata;
    context->code_data = code_data;
    context->instruction_count = instruction_count;
    context->handler_info = NEW(struct handler_info_type, 
				mb->exception_table_length);
    context->need_constructor_call = FALSE; /* almost always true */
    context->bitmask_size = (mb->nlocals + (BITS_PER_INT - 1))/BITS_PER_INT;
    
    if (instruction_count == 0) 
	CCerror(context, "Empty code");
	
    for (inumber = 0, offset = 0; offset < code_length; inumber++) {
	int32_t length = instruction_length(code, offset);
	instruction_data_type *this_idata = &idata[inumber];
	this_idata->opcode = code[offset];
	this_idata->stack_info.stack = NULL;
	this_idata->stack_info.stack_size  = UNKNOWN_STACK_SIZE;
	this_idata->register_info.register_count = UNKNOWN_REGISTER_COUNT;
	this_idata->changed = FALSE;  /* no need to look at it yet. */
	this_idata->protected = FALSE;  /* no need to look at it yet. */
	/* This also sets up this_data->operand.  It also makes the 
	 * xload_x and xstore_x instructions look like the generic form. */
	verify_opcode_operands(context, inumber, offset);
	offset += (codepos_t) length;
    }
    
    
    /* make sure exception table is reasonable. */
    initialize_exception_table(context);
    /* Set up first instruction, and start of exception handlers. */
    initialize_dataflow(context);
    /* Run data flow analysis on the instructions. */
    run_dataflow(context);
}


/* Look at a single instruction, and verify its operands.  Also, for 
 * simplicity, move the operand into the ->operand field.
 * Make sure that branches don't go into the middle of nowhere.
 */

static void
verify_opcode_operands(context_type *context, codepos_t inumber, codepos_t offset)
{
    instruction_data_type *idata = context->instruction_data;
    instruction_data_type *this_idata = &idata[inumber];
    short *code_data = context->code_data;
    struct methodblock *mb = context->mb;
    unsigned char *code = context->code;
    opcode_type opcode = this_idata->opcode;
    int var; 
    
    this_idata->operand.ip = NULL;
    this_idata->operand2.ip = NULL;

    switch (opcode) {

    case opc_jsr:
	/* instruction of ret statement */
	this_idata->operand2.i = UNKNOWN_RET_INSTRUCTION;
	/* FALLTHROUGH */
    case opc_ifeq: case opc_ifne: case opc_iflt: 
    case opc_ifge: case opc_ifgt: case opc_ifle:
    case opc_ifnull: case opc_ifnonnull:
    case opc_if_icmpeq: case opc_if_icmpne: case opc_if_icmplt: 
    case opc_if_icmpge: case opc_if_icmpgt: case opc_if_icmple:
    case opc_if_acmpeq: case opc_if_acmpne:   
    case opc_goto: {
	/* Set the ->operand to be the instruction number of the target. */
	int16_t jump = (int16_t)(SH(code,offset+1)); /* (((int16_t)(code[offset+1])) << 8) + code[offset+2]; */
	codepos_t target = offset + jump;
	if (!isLegalTarget(context, target))
	    CCerror(context, "Illegal target of jump or branch");
	this_idata->operand.i = code_data[target];
	break;
    }
	
    case opc_jsr_w:
	/* instruction of ret statement */
	this_idata->operand2.i = UNKNOWN_RET_INSTRUCTION;
	/* FALLTHROUGH */
    case opc_goto_w: {
	/* Set the ->operand to be the instruction number of the target. */
	int32_t jump = (int32_t)(LG(code, offset + 1));/* (((signed char)(code[offset+1])) << 24) + 
	             (code[offset+2] << 16) + (code[offset+3] << 8) + 
	             (code[offset + 4]); */
	codepos_t target = offset + jump;
	if (!isLegalTarget(context, target))
	    CCerror(context, "Illegal target of jump or branch");
	this_idata->operand.i = code_data[target];
	break;
    }

    case opc_tableswitch: 
    case opc_lookupswitch: {
	/* Set the ->operand to be a table of possible instruction targets. */
	int32_t *lpc = (int32_t *) REL_ALIGN(code, offset+1); /* UCALIGN(code + offset + 1); */
	int32_t *lptr;
	int32_t *saved_operand;
	int32_t keys;
	int32_t k, delta;
	codepos_t target;
	if (opcode == opc_tableswitch) {
	    keys = ntohl(lpc[2]) -  ntohl(lpc[1]) + 1;
	    delta = 1;
	} else { 
	    keys = ntohl(lpc[1]); /* number of pairs */
	    delta = 2;
	}
	saved_operand = NEW(int32_t, keys + 2);
	target = offset + ntohl(lpc[0]);
	if (!isLegalTarget(context, target)) 
	    CCerror(context, "Illegal default target in switch");
	saved_operand[keys + 1] = code_data[target];
	for (k = keys, lptr = &lpc[3]; --k >= 0; lptr += delta) {
	    target = offset + ntohl(lptr[0]);
	    if (!isLegalTarget(context, target))
		CCerror(context, "Illegal branch in opc_tableswitch");
	    saved_operand[k + 1] = code_data[target];
	}
	saved_operand[0] = keys + 1; /* number of successors */
	this_idata->operand.ip = saved_operand;
	break;
    }
	
    case opc_ldc: {   
	/* Make sure the constant pool item is the right type. */
	int key = code[offset + 1];
	int types = (1 << CONSTANT_Integer) | (1 << CONSTANT_Float) |
	                    (1 << CONSTANT_String);
	this_idata->operand.i = key;
	verify_constant_pool_type(context, key, types);
	break;
    }
	  
    case opc_ldc_w: {   
	/* Make sure the constant pool item is the right type. */
	int key = SH(code, offset+1); /* (code[offset + 1] << 8) + code[offset + 2]; */
	int types = (1 << CONSTANT_Integer) | (1 << CONSTANT_Float) |
	    (1 << CONSTANT_String);
	this_idata->operand.i = key;
	verify_constant_pool_type(context, key, types);
	break;
    }
	  
    case opc_ldc2_w: { 
	/* Make sure the constant pool item is the right type. */
	int key = SH(code, offset+1); /* (code[offset + 1] << 8) + code[offset + 2]; */
	int types = (1 << CONSTANT_Double) | (1 << CONSTANT_Long);
	this_idata->operand.i = key;
	verify_constant_pool_type(context, key, types);
	break;
    }

    case opc_getfield: case opc_putfield:
    case opc_getstatic: case opc_putstatic: {
	/* Make sure the constant pool item is the right type. */
	cp_index_type key = SH(code,offset+1); /* (code[offset + 1] << 8) + code[offset + 2]; */
	this_idata->operand.i = key;
	verify_constant_pool_type(context, key, 1 << CONSTANT_Fieldref);
	if (opcode == opc_getfield || opcode == opc_putfield) 
	    set_protected(context, inumber, key, opcode);
	break;
    }
	
    case opc_invokevirtual:
    case opc_invokenonvirtual: 
    case opc_invokestatic:
    case opc_invokeinterface: {
	/* Make sure the constant pool item is the right type. */
	cp_index_type key = SH(code, offset+1); /* (code[offset + 1] << 8) + code[offset + 2]; */
	char *methodname;
	fullinfo_type clazz_info;
	int kind = (opcode == opc_invokeinterface 
	                    ? 1 << CONSTANT_InterfaceMethodref
	                    : 1 << CONSTANT_Methodref);
	/* Make sure the constant pool item is the right type. */
	verify_constant_pool_type(context, key, kind);
	methodname = cp_index_to_fieldname(context, key);
	clazz_info = cp_index_to_class_fullinfo(context, key, TRUE);
	this_idata->operand.i = key;
	this_idata->operand2.fi = clazz_info;
	if (strcmp(methodname, "<init>") == 0) {
	    if (opcode != opc_invokenonvirtual)
		CCerror(context, 
			"Must call initializers using invokenonvirtual");
	    this_idata->opcode = opc_invokeinit;
	} else {
	    if (methodname[0] == '<') 
		CCerror(context, "Illegal call to internal method");
	    if (opcode == opc_invokenonvirtual 
		   && clazz_info != context->currentclass_info  
		   && clazz_info != context->superclass_info) {
		ClassClass *cb = context->class;
		for (; ; cb = unhand(cbSuperclass(cb))) { 
		    if (clazz_info == MAKE_CLASSNAME_INFO(cb->name, 0))
			break;
		    /* The optimizer make cause this to happen on local code */
		    if (cbSuperclass(cb) == 0) {
			/* optimizer make cause this to happen on local code */
			if (cbLoader(cb) != 0) 
			    CCerror(context, 
				    "Illegal use of nonvirtual function call");
			break;
		}
	    }
	}
	}
	if (opcode == opc_invokeinterface) { 
	    char *signature = cp_index_to_signature(context, key);
	    unsigned int args1 = Signature2ArgsSize(signature) + 1;
	    unsigned int args2 = code[offset + 3];
	    if (args1 != args2) {
		CCerror(context, 
			"Inconsistent args_size for opc_invokeinterface");	
	    } 
	} else if (opcode == opc_invokevirtual) 
	    set_protected(context, inumber, key, opcode);
	break;
    }
	

    case opc_instanceof: 
    case opc_checkcast: 
    case opc_new:
    case opc_anewarray: 
    case opc_multianewarray: {
	/* Make sure the constant pool item is a class */
	cp_index_type key = SH(code, offset + 1); /* (code[offset + 1] << 8) + code[offset + 2]; */
	fullinfo_type target;
	verify_constant_pool_type(context, key, 1 << CONSTANT_Class);
	target = cp_index_to_class_fullinfo(context, key, FALSE);
	if (GET_ITEM_TYPE(target) == ITEM_Bogus) 
	    CCerror(context, "Illegal type");
	switch(opcode) {
	case opc_anewarray:
	    if ((GET_INDIRECTION(target)) >= MAX_ARRAY_DIMENSIONS)
		CCerror(context, "Array with too many dimensions");
	    this_idata->operand.fi = MAKE_FULLINFO(GET_ITEM_TYPE(target),
						   GET_INDIRECTION(target) + 1,
						   GET_EXTRA_INFO(target));
	    break;
	case opc_new:
	    if (WITH_ZERO_EXTRA_INFO(target) !=
		             MAKE_FULLINFO(ITEM_Object, 0, 0))
		CCerror(context, "Illegal creation of multi-dimensional array");
	    /* operand gets set to the "unitialized object".  operand2 gets
	     * set to what the value will be after it's initialized. */
	    this_idata->operand.fi = MAKE_FULLINFO(ITEM_NewObject, 0, inumber);
	    this_idata->operand2.fi = target;
	    break;
	case opc_multianewarray:
	    this_idata->operand.fi = target;
	    this_idata->operand2.i = code[offset + 3];
	    if (GET_INDIRECTION(target) < this_idata->operand2.u)
		CCerror(context, "Dimension of array is too small");
	    break;
	default:
	    this_idata->operand.fi = target;
	}
	break;
    }
	
    case opc_newarray: {
	/* Cache the result of the opc_newarray into the operand slot */
	fullinfo_type full_info = 0;
	switch (code[offset + 1]) {
	    case T_INT:    
	        full_info = MAKE_FULLINFO(ITEM_Integer, 1, 0); break;
	    case T_LONG:   
		full_info = MAKE_FULLINFO(ITEM_Long, 1, 0); break;
	    case T_FLOAT:  
		full_info = MAKE_FULLINFO(ITEM_Float, 1, 0); break;
	    case T_DOUBLE: 
		full_info = MAKE_FULLINFO(ITEM_Double, 1, 0); break;
	    case T_BYTE: case T_BOOLEAN:
		full_info = MAKE_FULLINFO(ITEM_Byte, 1, 0); break;
	    case T_CHAR:   
		full_info = MAKE_FULLINFO(ITEM_Char, 1, 0); break;
	    case T_SHORT:  
		full_info = MAKE_FULLINFO(ITEM_Short, 1, 0); break;
	    default:
		CCerror(context, "Bad type passed to opc_newarray");
	}
	this_idata->operand.fi = full_info;
	break;
    }
	  
    /* Fudge iload_x, aload_x, etc to look like their generic cousin. */
    case opc_iload_0: case opc_iload_1: case opc_iload_2: case opc_iload_3:
	this_idata->opcode = opc_iload;
	var = opcode - opc_iload_0;
	goto check_local_variable;
	  
    case opc_fload_0: case opc_fload_1: case opc_fload_2: case opc_fload_3:
	this_idata->opcode = opc_fload;
	var = opcode - opc_fload_0;
	goto check_local_variable;

    case opc_aload_0: case opc_aload_1: case opc_aload_2: case opc_aload_3:
	this_idata->opcode = opc_aload;
	var = opcode - opc_aload_0;
	goto check_local_variable;

    case opc_lload_0: case opc_lload_1: case opc_lload_2: case opc_lload_3:
	this_idata->opcode = opc_lload;
	var = opcode - opc_lload_0;
	goto check_local_variable2;

    case opc_dload_0: case opc_dload_1: case opc_dload_2: case opc_dload_3:
	this_idata->opcode = opc_dload;
	var = opcode - opc_dload_0;
	goto check_local_variable2;

    case opc_istore_0: case opc_istore_1: case opc_istore_2: case opc_istore_3:
	this_idata->opcode = opc_istore;
	var = opcode - opc_istore_0;
	goto check_local_variable;
	
    case opc_fstore_0: case opc_fstore_1: case opc_fstore_2: case opc_fstore_3:
	this_idata->opcode = opc_fstore;
	var = opcode - opc_fstore_0;
	goto check_local_variable;

    case opc_astore_0: case opc_astore_1: case opc_astore_2: case opc_astore_3:
	this_idata->opcode = opc_astore;
	var = opcode - opc_astore_0;
	goto check_local_variable;

    case opc_lstore_0: case opc_lstore_1: case opc_lstore_2: case opc_lstore_3:
	this_idata->opcode = opc_lstore;
	var = opcode - opc_lstore_0;
	goto check_local_variable2;

    case opc_dstore_0: case opc_dstore_1: case opc_dstore_2: case opc_dstore_3:
	this_idata->opcode = opc_dstore;
	var = opcode - opc_dstore_0;
	goto check_local_variable2;

    case opc_wide: 
	this_idata->opcode = code[offset + 1];
	var = SH(code, offset+2); /* (code[offset + 2] << 8) + code[offset + 3]; */;
	switch(this_idata->opcode) {
	    case opc_lload:  case opc_dload: 
	    case opc_lstore: case opc_dstore:
	        goto check_local_variable2;
	    default:
	        goto check_local_variable;
	}

    case opc_iinc:		/* the increment amount doesn't matter */
    case opc_ret: 
    case opc_aload: case opc_iload: case opc_fload:
    case opc_astore: case opc_istore: case opc_fstore:
	var = code[offset + 1];
    check_local_variable:
	/* Make sure that the variable number isn't illegal. */
	this_idata->operand.i = var;
	if (var >= (int)mb->nlocals) 
	    CCerror(context, "Illegal local variable number");
	break;
	    
    case opc_lload: case opc_dload: case opc_lstore: case opc_dstore: 
	var = code[offset + 1];
    check_local_variable2:
	/* Make sure that the variable number isn't illegal. */
	this_idata->operand.i = var;
	if ((var + 1) >= (int)mb->nlocals)
	    CCerror(context, "Illegal local variable number");
	break;
	
    default:
	if (opcode >= opc_breakpoint) 
	    CCerror(context, "Quick instructions shouldn't appear yet.");
	break;
    } /* of switch */
}


static void 
set_protected(context_type *context, codepos_t inumber, cp_index_type key, opcode_type opcode) 
{
    fullinfo_type clazz_info = cp_index_to_class_fullinfo(context, key, TRUE);
    if (isSuperClass(context, clazz_info)) {
	char *name = cp_index_to_fieldname(context, key);
	char *signature = cp_index_to_signature(context, key);

	hash_t ID = NameAndTypeToHash(name, signature, context->class);
	ClassClass *calledClass = 
	    object_fullinfo_to_classclass(context, clazz_info);
	struct fieldblock *fb;

	if (opcode != opc_invokevirtual) { 
	    int n = calledClass->fields_count;
	    fb = cbFields(calledClass);
	    for (; --n >= 0; fb++) {
		if (fb->ID == ID) {
		    goto haveIt;
		}
	    }
	    return;
	} else { 
	    struct methodblock *mb = cbMethods(calledClass);
	    int n = calledClass->methods_count;
	    for (; --n >= 0; mb++) {
		if (mb->fb.ID == ID) {
		    fb = &mb->fb;
		    goto haveIt;
		}
	    }
	    return;
	}
    haveIt:
	if (IsProtected(fb->access)) {
	    if (IsPrivate(fb->access) ||
		    !IsSameClassPackage(calledClass, context->class))
		context->instruction_data[inumber].protected = TRUE;
	}
    }
}


static bool_t 
isSuperClass(context_type *context, fullinfo_type clazz_info) { 
    fullinfo_type *fptr = context->superClasses;
    if (fptr == NULL) { 
	ClassClass *cb;
	fullinfo_type *gptr;
	int i;
	/* Count the number of superclasses.  By counting ourselves, and
	 * not counting Object, we get the same number.    */
	for (i = 0, cb = context->class;
	     cb != classJavaLangObject; 
	     i++, cb = unhand(cbSuperclass(cb)));
	/* Can't go on context heap since it survives more than one method */
	context->superClasses = fptr 
	    = sysMalloc(sizeof(fullinfo_type)*(i + 1));
	if (fptr == NULL) {
	    CCerror(context, "Out of memory on verify");
	}
	for (gptr = fptr, cb = context->class; cb != classJavaLangObject; ) { 
	    void **addr;
	    cb = unhand(cbSuperclass(cb));
	    *gptr++ = MAKE_CLASSNAME_INFO(cb->name, &addr); 
	    *addr = cb;
	}
	*gptr = 0;
    } 
    for (; *fptr != 0; fptr++) { 
	if (*fptr == clazz_info)
	    return TRUE;
    }
    return FALSE;
}




/* Look through each item on the exception table.  Each of the fields must 
 * refer to a legal instruction. 
 */
static void
initialize_exception_table(context_type *context)
{
    struct methodblock *mb = context->mb;
    struct CatchFrame *exception_table = mb->exception_table;
    struct handler_info_type *handler_info = context->handler_info;
    short *code_data = context->code_data;
    int32_t i;
    for (i = (int32_t) mb->exception_table_length; 
	     --i >= 0; exception_table++, handler_info++) {
	codepos_t start = exception_table->start_pc;
	codepos_t end = exception_table->end_pc;
	codepos_t handler = exception_table->handler_pc;
	codepos_t catchType = exception_table->catchType;
	stack_item_type *stack_item = NEW(stack_item_type, 1);
	if (!(  start <= end
	      && isLegalTarget(context, start) 
	      && isLegalTarget(context, end))) {
	    CCerror(context, "Illegal exception table range");
	}
	if (!((handler > 0) && isLegalTarget(context, handler))) {
	    CCerror(context, "Illegal exception table handler");
	}

	handler_info->start = code_data[start];
	handler_info->end = code_data[end];
	handler_info->handler = code_data[handler];
	handler_info->stack_info.stack = stack_item;
	handler_info->stack_info.stack_size = 1;
	stack_item->next = NULL;
	if (catchType != 0) {
	    union cp_item_type *cp = context->class->constantpool;
	    char *classname;
	    verify_constant_pool_type(context, catchType, 1 << CONSTANT_Class);
	    classname = GetClassConstantClassName(cp, catchType);
	    stack_item->item = MAKE_CLASSNAME_INFO(classname, 0);
	} else {
	    stack_item->item = context->throwable_info;
	}
    }
}


/* Given a pointer to an instruction, return its length.  Use the table
 * opcode_length[] which is automatically built.
 */
static int32_t instruction_length(unsigned char *code, uint32_t offset)
{
    unsigned char instruction = code[offset];
    unsigned char *iptr = &code[offset];
    switch (instruction) {
        case opc_tableswitch: {
	    int32_t *lpc = (int32_t *) REL_ALIGN(code,offset+1); /* UCALIGN(iptr + 1); */
	    int32_t low = ntohl(lpc[1]);
	    int32_t high = ntohl(lpc[2]);
	    if ((low > high) || (low + 65535 < high)) 
		return -1;	/* illegal */
	    else 
		return (unsigned char *)(&lpc[(high - low + 1) + 3]) - iptr;
	}
	    
	case opc_lookupswitch: {
	    int32_t *lpc = (int32_t *) REL_ALIGN(code, offset+1); /* UCALIGN(iptr + 1); */
	    int32_t npairs = ntohl(lpc[1]);
	    if (npairs < 0 || npairs >= 1000) 
		return  -1;
	    else 
		return (unsigned char *)(&lpc[2 * (npairs + 1)]) - iptr;
	}

	case opc_wide: 
	    switch(iptr[1]) {
	        case opc_ret:
	        case opc_iload: case opc_istore: 
	        case opc_fload: case opc_fstore:
	        case opc_aload: case opc_astore:
	        case opc_lload: case opc_lstore:
	        case opc_dload: case opc_dstore: 
		    return 4;
		case opc_iinc:
		    return 6;
		default: 
		    return -1;
	    }

	default: {
	    /* A length of 0 indicates an error. */
	    int length = opcode_length[instruction];
	    return (length <= 0) ? -1 : length;
	}
    }
}


/* Given the target of a branch, make sure that it's a legal target. */
static bool_t 
isLegalTarget(context_type *context, codepos_t offset)
{
    struct methodblock *mb = context->mb;
    codepos_t code_length = mb->code_length;
    short *code_data = context->code_data;
    return (offset < code_length && code_data[offset] >= 0);
}


/* Make sure that an element of the constant pool really is of the indicated 
 * type.
 */
static void
verify_constant_pool_type(context_type *context, cp_index_type index, unsigned mask)
{
    union cp_item_type *constant_pool = context->class->constantpool;
    unsigned char *type_table = 
           constant_pool[CONSTANT_POOL_TYPE_TABLE_INDEX].p;
    unsigned type = CONSTANT_POOL_TYPE_TABLE_GET_TYPE(type_table, index);
    if ((mask & (1 << type)) == 0) 
	CCerror(context, "Illegal type in constant pool");
}
        

static void
initialize_dataflow(context_type *context) 
{
    instruction_data_type *idata = context->instruction_data;
    struct methodblock *mb = context->mb;
    fullinfo_type *reg_ptr;
    fullinfo_type full_info;
    char *p;

    /* Initialize the function entry, since we know everything about it. */
    idata[0].stack_info.stack_size = 0;
    idata[0].stack_info.stack = NULL;
    idata[0].register_info.register_count = mb->args_size;
    idata[0].register_info.registers = NEW(fullinfo_type, mb->args_size);
    idata[0].register_info.mask_count = 0;
    idata[0].register_info.masks = NULL;
    reg_ptr = idata[0].register_info.registers;

    if ((mb->fb.access & ACC_STATIC) == 0) {
	/* A non static method.  If this is an <init> method, the first
	 * argument is an uninitialized object.  Otherwise it is an object of
	 * the given class type.  java.lang.Object.<init> is special since
	 * we don't call its superclass <init> method.
	 */
	if (strcmp(mb->fb.name, "<init>") == 0 
	        && context->currentclass_info != context->object_info) {
	    *reg_ptr++ = MAKE_FULLINFO(ITEM_InitObject, 0, 0);
	    context->need_constructor_call = TRUE;
	} else {
	    *reg_ptr++ = context->currentclass_info;
	}
    }
    /* Fill in each of the arguments into the registers. */
    for (p = mb->fb.signature + 1; *p != SIGNATURE_ENDFUNC; ) {
	char fieldchar = signature_to_fieldtype(context, &p, &full_info);
	switch (fieldchar) {
	    case 'D': case 'L': 
	        *reg_ptr++ = full_info;
	        *reg_ptr++ = full_info + 1;
		break;
	    default:
	        *reg_ptr++ = full_info;
		break;
	}
    }
    p++;			/* skip over right parenthesis */
    if (*p == 'V') {
	context->return_type = MAKE_FULLINFO(ITEM_Void, 0, 0);
    } else {
	signature_to_fieldtype(context, &p, &full_info);
	context->return_type = full_info;
    }

    /* Indicate that we need to look at the first instruction. */
    idata[0].changed = TRUE;
}	


/* Run the data flow analysis, as long as there are things to change. */
static void
run_dataflow(context_type *context) {
    struct methodblock *mb = context->mb;
    int max_stack_size = mb->maxstack;
    instruction_data_type *idata = context->instruction_data;
    int32_t icount = context->instruction_count;
    bool_t work_to_do = TRUE;
    int inumber;

    /* Run through the loop, until there is nothing left to do. */
    while (work_to_do) {
	work_to_do = FALSE;
	for (inumber = 0; inumber < icount; inumber++) {
	    instruction_data_type *this_idata = &idata[inumber];
	    if (this_idata->changed) {
		register_info_type new_register_info;
		stack_info_type new_stack_info;
		
		this_idata->changed = FALSE;
		work_to_do = TRUE;
#ifdef DEBUG
		if (verify_verbose) {
		    int opcode = this_idata->opcode;
		    char *opname = (opcode == opc_invokeinit) ? 
				    "invokeinit" : opnames[opcode];
		    printf("Instruction %d: ", inumber);
		    print_stack(context, &this_idata->stack_info);
		    print_registers(context, &this_idata->register_info);
		    printf("  %s(%d)", opname, this_idata->operand.i);
		    fflush(stdout);
		}
#endif
		/* Make sure the registers can deal with this instruction */
		check_register_values(context, inumber);

		/* Make sure the stack can deal with this instruction */
		pop_stack(context, inumber, &new_stack_info);

		/* Update the registers */
		update_registers(context, inumber, &new_register_info);

		/* Update the stack. */
		push_stack(context, inumber, &new_stack_info);

		if (new_stack_info.stack_size > max_stack_size) 
		    CCerror(context, "Stack size too large");
#ifdef DEBUG
		if (verify_verbose) {
		    printf("  ");
		    print_stack(context, &new_stack_info);
		    print_registers(context, &new_register_info);
		    fflush(stdout);
		}
#endif
		/* Add the new stack and register information to any
		 * instructions that can follow this instruction.     */
		merge_into_successors(context, inumber, 
				      &new_register_info, &new_stack_info);

	    }
	}
    }
}


/* Make sure that the registers contain a legitimate value for the given
 * instruction.
*/

static void
check_register_values(context_type *context, int inumber)
{
    instruction_data_type *idata = context->instruction_data;
    instruction_data_type *this_idata = &idata[inumber];
    opcode_type opcode = this_idata->opcode;
    int32_t operand = this_idata->operand.i;
    int32_t register_count = this_idata->register_info.register_count;
    fullinfo_type *registers = this_idata->register_info.registers;
    bool_t double_word = FALSE;	/* default value */
    fullinfo_type type;
    
    switch (opcode) {
        default:
	    return;
        case opc_iload: case opc_iinc:
	    type = ITEM_Integer; break;
        case opc_fload:
	    type = ITEM_Float; break;
        case opc_aload:
	    type = ITEM_Object; break;
        case opc_ret:
	    type = ITEM_ReturnAddress; break;
        case opc_lload:	
	    type = ITEM_Long; double_word = TRUE; break;
        case opc_dload:
	    type = ITEM_Double; double_word = TRUE; break;
    }
    if (!double_word) {
	fullinfo_type reg = registers[operand];
	/* Make sure we don't have an illegal register or one with wrong type */
	if (operand >= register_count) {
	    CCerror(context, 
		    "Accessing value from uninitialized register %d", operand);
	} else if (WITH_ZERO_EXTRA_INFO(reg) == MAKE_FULLINFO(type, 0, 0)) {
	    /* the register is obviously of the given type */
	    return;
	} else if (GET_INDIRECTION(reg) > 0 && type == ITEM_Object) {
	    /* address type stuff be used on all arrays */
	    return;
	} else if (GET_ITEM_TYPE(reg) == ITEM_ReturnAddress) { 
	    CCerror(context, "Cannot load return address from register %d", 
		              operand);
	    /* alternativeively 
	              (GET_ITEM_TYPE(reg) == ITEM_ReturnAddress)
                   && (opcode == opc_iload) 
		   && (type == ITEM_Object || type == ITEM_Integer)
	       but this never occurs
	    */
	} else if (reg == ITEM_InitObject && type == ITEM_Object) {
	    return;
	} else if (WITH_ZERO_EXTRA_INFO(reg) == 
		        MAKE_FULLINFO(ITEM_NewObject, 0, 0) && 
		   type == ITEM_Object) {
	    return;
        } else {
	    CCerror(context, "Register %d contains wrong type", operand);
	}
    } else {
	/* Make sure we don't have an illegal register or one with wrong type */
	if ((operand + 1) >= register_count) {
	    CCerror(context, 
		    "Accessing value from uninitialized register pair %d/%d", 
		    operand, operand+1);
	} else {
	    if ((registers[operand] == MAKE_FULLINFO(type, 0, 0)) &&
		(registers[operand + 1] == MAKE_FULLINFO(type + 1, 0, 0))) {
		return;
	    } else {
		CCerror(context, "Register pair %d/%d contains wrong type", 
		        operand, operand+1);
	    }
	}
    } 
}


/* Make sure that the top of the stack contains reasonable values for the 
 * given instruction.  The post-pop values of the stack and its size are 
 * returned in *new_stack_info.
 */

static void 
pop_stack(context_type *context, int inumber, stack_info_type *new_stack_info)
{
    instruction_data_type *idata = context->instruction_data;
    instruction_data_type *this_idata = &idata[inumber];
    opcode_type opcode = this_idata->opcode;
    stack_item_type *stack = this_idata->stack_info.stack;
    int32_t stack_size = this_idata->stack_info.stack_size;
    char *stack_operands, *p;
    char buffer[257];		/* for holding manufactured argument lists */
    fullinfo_type stack_extra_info_buffer[256]; /* save info popped off stack */
    fullinfo_type *stack_extra_info = &stack_extra_info_buffer[256];
    fullinfo_type full_info, put_full_info;
		 
    switch(opcode) {
        default:
	    /* For most instructions, we just use a built-in table */
	    stack_operands = opcode_in_out[opcode][0];
	    break;

        case opc_putstatic: case opc_putfield: {
	    /* The top thing on the stack depends on the signature of
	     * the object.                         */
	    cp_index_type operand = (cp_index_type) this_idata->operand.i;
	    char *signature = cp_index_to_signature(context, operand);
	    char *ip = buffer;
#ifdef DEBUG
	    if (verify_verbose) {
		print_formatted_fieldname(context, operand);
	    }
#endif
	    if (opcode == opc_putfield)
		*ip++ = 'A';	/* object for putfield */
	    *ip++ = signature_to_fieldtype(context, &signature, &put_full_info);
	    *ip = '\0';
	    stack_operands = buffer;
	    break;
	}

	case opc_invokevirtual: case opc_invokenonvirtual:        
        case opc_invokeinit:	/* invokenonvirtual call to <init> */
	case opc_invokestatic: case opc_invokeinterface: {
	    /* The top stuff on the stack depends on the method signature */
	    cp_index_type operand = (cp_index_type) this_idata->operand.i;
	    char *signature = cp_index_to_signature(context, operand);
	    char *ip = buffer;
	    char *p;
#ifdef DEBUG
	    if (verify_verbose) {
		print_formatted_fieldname(context, operand);
	    }
#endif
	    if (opcode != opc_invokestatic) 
		/* First, push the object */
		*ip++ = (opcode == opc_invokeinit ? '@' : 'A');
	    for (p = signature + 1; *p != SIGNATURE_ENDFUNC; ) {
		*ip++ = signature_to_fieldtype(context, &p, &full_info);
		if (ip >= buffer + sizeof(buffer) - 1)
		    CCerror(context, "Signature %s has too many arguments", 
			    signature);
	    }
	    *ip = 0;
	    stack_operands = buffer;
	    break;
	}

	case opc_multianewarray: {
	    /* Count can't be larger than 255.  So can't overflow buffer */
	    uint32_t count = this_idata->operand2.u;	/* number of ints on stack */
	    memset(buffer, 'I', (size_t) count);
	    buffer[count] = '\0';
	    stack_operands = buffer;
	    break;
	}

    } /* of switch */

    /* Run through the list of operands >>backwards<< */
    for (   p = stack_operands + strlen(stack_operands);
	    p > stack_operands; 
	    stack = stack->next) {
	int type = *--p;
	fullinfo_type top_type = stack ? stack->item : 0;
	int size = (type == 'D' || type == 'L') ? 2 : 1;
	*--stack_extra_info = top_type;
	if (stack == NULL) 
	    CCerror(context, "Unable to pop operand off an empty stack");
	switch (type) {
	    case 'I': 
	        if (top_type != MAKE_FULLINFO(ITEM_Integer, 0, 0))
		    CCerror(context, "Expecting to find integer on stack");
		break;
		
	    case 'F': 
		if (top_type != MAKE_FULLINFO(ITEM_Float, 0, 0))
		    CCerror(context, "Expecting to find float on stack");
		break;
		
	    case 'A':		/* object or array */
		if (   (GET_ITEM_TYPE(top_type) != ITEM_Object) 
		    && (GET_INDIRECTION(top_type) == 0)
		    && (!((opcode == opc_astore) && 
			  (WITH_ZERO_EXTRA_INFO(top_type) == 
		              MAKE_FULLINFO(ITEM_ReturnAddress, 0, 0)))))
		    CCerror(context, "Expecting to find object/array on stack");
		break;

	    case '@': {		/* unitialized object, for call to <init> */
		fullinfo_type item_type = GET_ITEM_TYPE(top_type);
		if (item_type != ITEM_NewObject && item_type != ITEM_InitObject)
		    CCerror(context, 
			    "Expecting to find unitialized object on stack");
		break;
	    }

	    case 'O':		/* object, not array */
		if (WITH_ZERO_EXTRA_INFO(top_type) != 
		       MAKE_FULLINFO(ITEM_Object, 0, 0))
		    CCerror(context, "Expecting to find object on stack");
		break;

	    case 'a':		/* integer, object, or array */
		if (      (top_type != MAKE_FULLINFO(ITEM_Integer, 0, 0)) 
		       && (GET_ITEM_TYPE(top_type) != ITEM_Object)
		       && (GET_INDIRECTION(top_type) == 0))
		    CCerror(context, 
			    "Expecting to find object, array, or int on stack");
		break;

	    case 'D':		/* double */
		if (top_type != MAKE_FULLINFO(ITEM_Double, 0, 0))
		    CCerror(context, "Expecting to find double on stack");
		break;

	    case 'L':		/* long */
		if (top_type != MAKE_FULLINFO(ITEM_Long, 0, 0))
		    CCerror(context, "Expecting to find long on stack");
		break;

	    case ']':		/* array of some type */
		if (top_type == NULL_FULLINFO) { 
		    /* do nothing */
		} else switch(p[-1]) {
		    case 'I':	/* array of integers */
		        if (top_type != MAKE_FULLINFO(ITEM_Integer, 1, 0) && 
			    top_type != NULL_FULLINFO)
			    CCerror(context, 
				    "Expecting to find array of ints on stack");
			break;

		    case 'L':	/* array of longs */
		        if (top_type != MAKE_FULLINFO(ITEM_Long, 1, 0))
			    CCerror(context, 
				   "Expecting to find array of longs on stack");
			break;

		    case 'F':	/* array of floats */
		        if (top_type != MAKE_FULLINFO(ITEM_Float, 1, 0))
			    CCerror(context, 
				 "Expecting to find array of floats on stack");
			break;

		    case 'D':	/* array of doubles */
		        if (top_type != MAKE_FULLINFO(ITEM_Double, 1, 0))
			    CCerror(context, 
				"Expecting to find array of doubles on stack");
			break;

		    case 'A': {	/* array of addresses (arrays or objects) */
			fullinfo_type indirection = GET_INDIRECTION(top_type);
			if ((indirection == 0) || 
			    ((indirection == 1) && 
			        (GET_ITEM_TYPE(top_type) != ITEM_Object)))
			    CCerror(context, 
				"Expecting to find array of objects or arrays "
				    "on stack");
			break;
		    }
			
		    case 'B':	/* array of bytes */
		        if (top_type != MAKE_FULLINFO(ITEM_Byte, 1, 0))
			    CCerror(context, 
				  "Expecting to find array of bytes on stack");
			break;

		    case 'C':	/* array of characters */
		        if (top_type != MAKE_FULLINFO(ITEM_Char, 1, 0))
			    CCerror(context, 
				  "Expecting to find array of chars on stack");
			break;

		    case 'S':	/* array of shorts */
		        if (top_type != MAKE_FULLINFO(ITEM_Short, 1, 0))
			    CCerror(context, 
				 "Expecting to find array of shorts on stack");
			break;

		    case '?':	/* any type of array is okay */
		        if (GET_INDIRECTION(top_type) == 0) 
			    CCerror(context, 
				    "Expecting to find array on stack");
			break;

		    default:
			CCerror(context, "Internal error #1");
			sysAssert(FALSE);
			break;
		}
		p -= 2;		/* skip over [ <char> */
		break;

	    case '1': case '2': case '3': case '4': /* stack swapping */
		if (top_type == MAKE_FULLINFO(ITEM_Double, 0, 0) 
		    || top_type == MAKE_FULLINFO(ITEM_Long, 0, 0)) {
		    if ((p > stack_operands) && (p[-1] == '+')) {
			context->swap_table[type - '1'] = top_type + 1;
			context->swap_table[p[-2] - '1'] = top_type;
			size = 2;
			p -= 2;
		    } else {
			CCerror(context, 
				"Attempt to split long or double on the stack");
		    }
		} else {
		    context->swap_table[type - '1'] = stack->item;
		    if ((p > stack_operands) && (p[-1] == '+')) 
			p--;	/* ignore */
		}
		break;
	    case '+':		/* these should have been caught. */
	    default:
		CCerror(context, "Internal error #2");
		sysAssert(FALSE);
	}
	stack_size -= size;
    }
    
    /* For many of the opcodes that had an "A" in their field, we really 
     * need to go back and do a little bit more accurate testing.  We can, of
     * course, assume that the minimal type checking has already been done. 
     */
    switch (opcode) {
	default: break;
	case opc_aastore: {	/* array index object  */
	    fullinfo_type array_type = stack_extra_info[0];
	    fullinfo_type object_type = stack_extra_info[2];
	    fullinfo_type target_type = decrement_indirection(array_type);
	    if ((WITH_ZERO_EXTRA_INFO(target_type) == 
		             MAKE_FULLINFO(ITEM_Object, 0, 0)) &&
		 (WITH_ZERO_EXTRA_INFO(object_type) == 
		             MAKE_FULLINFO(ITEM_Object, 0, 0))) {
		/* I disagree.  But other's seem to think that we should allow
		 * an assignment of any Object to any array of any Object type.
		 * There will be an runtime error if the types are wrong.
		 */
		break;
	    }
	    if (!isAssignableTo(context, object_type, target_type))
		CCerror(context, "Incompatible types for storing into array of "
			            "arrays or objects");
	    break;
	}

	case opc_putfield: 
	case opc_getfield: 
        case opc_putstatic: {	
	    cp_index_type operand = (cp_index_type) this_idata->operand.i;
	    fullinfo_type stack_object = stack_extra_info[0];
	    if (opcode == opc_putfield || opcode == opc_getfield) {
		if (!isAssignableTo(context, 
				    stack_object, 
				cp_index_to_class_fullinfo(context, operand,
							     TRUE))) {
		CCerror(context, 
			"Incompatible type for getting or setting field");
	    }
		if (this_idata->protected && 
		    !isAssignableTo(context, stack_object, 
				    context->currentclass_info)) { 
		    CCerror(context, "Bad access to protected data");
		}
	    }
	    if (opcode == opc_putfield || opcode == opc_putstatic) { 
		int item = (opcode == opc_putfield ? 1 : 0);
		if (!isAssignableTo(context, 
				    stack_extra_info[item], put_full_info)) { 
		    CCerror(context, "Bad type in putfield/putstatic");
		}
	    }
	    break;
	}

        case opc_athrow: 
	    if (!isAssignableTo(context, stack_extra_info[0], 
				context->throwable_info)) {
		CCerror(context, "Can only throw Throwable objects");
	    }
	    break;

	case opc_aaload: {	/* array index */
	    /* We need to pass the information to the stack updater */
	    fullinfo_type array_type = stack_extra_info[0];
	    context->swap_table[0] = decrement_indirection(array_type);
	    break;
	}
	    
        case opc_invokevirtual: case opc_invokenonvirtual: 
        case opc_invokeinit:
	case opc_invokeinterface: case opc_invokestatic: {
	    cp_index_type operand = (cp_index_type) this_idata->operand.i;
	    char *signature = cp_index_to_signature(context, operand);
	    int item;
	    char *p;
	    if (opcode == opc_invokestatic) {
		item = 0;
	    } else if (opcode == opc_invokeinit) {
		fullinfo_type init_type = this_idata->operand2.fi;
		fullinfo_type object_type = stack_extra_info[0];
		context->swap_table[0] = object_type; /* save value */
		if (GET_ITEM_TYPE(stack_extra_info[0]) == ITEM_NewObject) {
		    /* We better be calling the appropriate init.  Find the
		     * inumber of the "opc_new" instruction", and figure 
		     * out what the type really is. 
		     */
		    fullinfo_type new_inumber = GET_EXTRA_INFO(stack_extra_info[0]);
		    fullinfo_type target_type = idata[new_inumber].operand2.fi;
		    context->swap_table[1] = target_type;
		    if (target_type != init_type) {
			CCerror(context, "Call to wrong initialization method");
		    }
		} else {
		    /* We better be calling super() or this(). */
		    if (init_type != context->superclass_info && 
			init_type != context->currentclass_info) {
			CCerror(context, "Call to wrong initialization method");
		    }
		    context->swap_table[1] = context->currentclass_info;
		}
		item = 1;
	    } else {
		fullinfo_type target_type = this_idata->operand2.fi;
		fullinfo_type object_type = stack_extra_info[0];
		if (!isAssignableTo(context, object_type, target_type)){
		    CCerror(context, 
			    "Incompatible object argument for function call");
		}
		if (this_idata->protected && 
		    !isAssignableTo(context, object_type, 
				    context->currentclass_info)) { 
		    CCerror(context, "Bad access to protected data");
		}
		item = 1;
	    }
	    for (p = signature + 1; *p != SIGNATURE_ENDFUNC; item++)
		if (signature_to_fieldtype(context, &p, &full_info) == 'A') {
		    if (!isAssignableTo(context, 
					stack_extra_info[item], full_info)) {
			CCerror(context, "Incompatible argument to function");
		    }
		}

	    break;
	}
	    
        case opc_return:
	    if (context->need_constructor_call) 
		CCerror(context, "Constructor must call super() or this()");
	    if (context->return_type != MAKE_FULLINFO(ITEM_Void, 0, 0)) 
		CCerror(context, "Wrong return type in function");
	    break;

	case opc_ireturn: case opc_lreturn: case opc_freturn: 
        case opc_dreturn: case opc_areturn: {
	    fullinfo_type target_type = context->return_type;
	    fullinfo_type object_type = stack_extra_info[0];
	    if (!isAssignableTo(context, object_type, target_type)) {
		CCerror(context, "Wrong return type in function");
	    }
	    break;
	}
    }

    new_stack_info->stack = stack;
    new_stack_info->stack_size = stack_size;
}


/* We've already determined that the instruction is legal.  Perform the 
 * operation on the registers, and return the updated results in
 * new_register_count_p and new_registers.
 */

static void
update_registers(context_type *context, int inumber, 
		 register_info_type *new_register_info)
{
    instruction_data_type *idata = context->instruction_data;
    instruction_data_type *this_idata = &idata[inumber];
    opcode_type opcode = this_idata->opcode;
    codepos_t operand = this_idata->operand.u;
    int32_t register_count = this_idata->register_info.register_count;
    fullinfo_type *registers = this_idata->register_info.registers;
    stack_item_type *stack = this_idata->stack_info.stack;
    int32_t mask_count = this_idata->register_info.mask_count;
    mask_type *masks = this_idata->register_info.masks;

    /* Use these as default new values. */
    int32_t            new_register_count = register_count;
    int32_t            new_mask_count = mask_count;
    fullinfo_type *new_registers = registers;
    mask_type     *new_masks = masks;

    enum { NONE, SINGLE, DOUBLE } access = NONE;
    int32_t i;
    
    /* Remember, we've already verified the type at the top of the stack. */
    switch (opcode) {
	default: break;
        case opc_istore: case opc_fstore: case opc_astore:
	    access = SINGLE;
	    goto continue_store;

        case opc_lstore: case opc_dstore:
	    access = DOUBLE;
	    goto continue_store;

	continue_store: {
	    /* We have a modification to the registers.  Copy them if needed. */
	    fullinfo_type stack_top_type = stack->item;
	    int32_t max_operand = operand + ((access == DOUBLE) ? 1 : 0);
	    
	    if (     max_operand < register_count 
		  && registers[operand] == stack_top_type
		  && ((access == SINGLE) || 
		         (registers[operand + 1]== stack_top_type + 1))) 
		/* No changes have been made to the registers. */
		break;
	    new_register_count = MAX(max_operand + 1, register_count);
	    new_registers = NEW(fullinfo_type, new_register_count);
	    for (i = 0; i < register_count; i++) 
		new_registers[i] = registers[i];
	    for (i = register_count; i < new_register_count; i++) 
		new_registers[i] = MAKE_FULLINFO(ITEM_Bogus, 0, 0);
	    new_registers[operand] = stack_top_type;
	    if (access == DOUBLE)
		new_registers[operand + 1] = stack_top_type + 1;
	    break;
	}
	 
        case opc_iload: case opc_fload: case opc_aload:
        case opc_iinc: case opc_ret:
	    access = SINGLE;
	    break;

        case opc_lload: case opc_dload:	
	    access = DOUBLE;
	    break;

        case opc_jsr: 
	    for (i = 0; i < new_mask_count; i++) 
		if (new_masks[i].entry == operand) 
		    CCerror(context, "Recursive call to jsr entry");
	    new_masks = add_to_masks(context, masks, mask_count, operand);
	    new_mask_count++; 
	    break;
	    
        case opc_invokeinit: {
	    /* An uninitialized object has been initialized.  Find all 
	     * occurrences of swap_table[0] in the registers, and replace
	     * them with swap_table[1];   */
	    fullinfo_type from = context->swap_table[0];
	    fullinfo_type to = context->swap_table[1];
	    int32_t i;
	    bool_t copied = FALSE;
	    if (from == MAKE_FULLINFO(ITEM_InitObject, 0, 0))
		context->need_constructor_call = FALSE;
	    for (i = 0; i < register_count; i++) {
		if (new_registers[i] == from) {
		    if (!copied) {
			new_registers = NEW(fullinfo_type, register_count);
			memcpy(new_registers, registers, 
			       (size_t) (register_count * sizeof(registers[0])));
			copied = TRUE;
		    }
		    new_registers[i] = to;
		}
	    }
	}
    }

    if ((access != NONE) && (new_mask_count > 0)) {
	int i, j;
	for (i = 0; i < new_mask_count; i++) {
	    int32_t *mask = new_masks[i].modifies;
	    if ((!IS_BIT_SET(mask, operand)) || 
		  ((access == DOUBLE) && !IS_BIT_SET(mask, operand + 1))) {
		new_masks = copy_masks(context, new_masks, mask_count);
		for (j = i; j < new_mask_count; j++) {
		    SET_BIT(new_masks[j].modifies, operand);
		    if (access == DOUBLE) 
			SET_BIT(new_masks[j].modifies, operand + 1);
		} 
		break;
	    }
	}
    }

    new_register_info->register_count = new_register_count;
    new_register_info->registers = new_registers;
    new_register_info->masks = new_masks;
    new_register_info->mask_count = new_mask_count;
}



/* We've already determined that the instruction is legal.  Perform the 
 * operation on the stack;
 *
 * new_stack_size_p and new_stack_p point to the results after the pops have
 * already been done.  Do the pushes, and then put the results back there. 
 */

static void
push_stack(context_type *context, int inumber, stack_info_type *new_stack_info)
{
    instruction_data_type *idata = context->instruction_data;
    instruction_data_type *this_idata = &idata[inumber];
    opcode_type opcode = this_idata->opcode;
    int32_t operand = this_idata->operand.i;

    int32_t stack_size = new_stack_info->stack_size;
    stack_item_type *stack = new_stack_info->stack;
    char *stack_results = NULL;
	
    fullinfo_type full_info = 0;
    char buffer[5], *p;		/* actually [2] is big enough */

    /* We need to look at all those opcodes in which either we can't tell the
     * value pushed onto the stack from the opcode, or in which the value
     * pushed onto the stack is an object or array.  For the latter, we need
     * to make sure that full_info is set to the right value.
     */
    switch(opcode) {
        default:
	    stack_results = opcode_in_out[opcode][1]; 
	    break;

	case opc_ldc: case opc_ldc_w: case opc_ldc2_w: {
	    /* Look to constant pool to determine correct result. */
	    union cp_item_type *cp = context->class->constantpool;
	    unsigned char *type_table = cp[CONSTANT_POOL_TYPE_TABLE_INDEX].p;
	    switch (CONSTANT_POOL_TYPE_TABLE_GET_TYPE(type_table, operand)) {
		case CONSTANT_Integer:   
		    stack_results = "I"; break;
		case CONSTANT_Float:     
		    stack_results = "F"; break;
		case CONSTANT_Double:    
		    stack_results = "D"; break;
		case CONSTANT_Long:      
		    stack_results = "L"; break;
		case CONSTANT_String: 
		    stack_results = "A"; 
		    full_info = context->string_info;
		    break;
		default:
		    CCerror(context, "Internal error #3");
		    sysAssert(FALSE);
	    }
	    break;
	}

        case opc_getstatic: case opc_getfield: {
	    /* Look to signature to determine correct result. */
	    cp_index_type operand = (cp_index_type)this_idata->operand.i;
	    char *signature = cp_index_to_signature(context, operand);
#ifdef DEBUG
	    if (verify_verbose) {
		print_formatted_fieldname(context, operand);
	    }
#endif
	    buffer[0] = signature_to_fieldtype(context, &signature, &full_info);
	    buffer[1] = '\0';
	    stack_results = buffer;
	    break;
	}

	case opc_invokevirtual: case opc_invokenonvirtual:
        case opc_invokeinit:
	case opc_invokestatic: case opc_invokeinterface: {
	    /* Look to signature to determine correct result. */
	    cp_index_type operand = (cp_index_type) this_idata->operand.i;
	    char *signature = cp_index_to_signature(context, operand);
	    char *result_signature = strchr(signature, SIGNATURE_ENDFUNC) + 1;
	    if (result_signature[0] == SIGNATURE_VOID) {
		stack_results = "";
	    } else {
		buffer[0] = signature_to_fieldtype(context, &result_signature, 
						   &full_info);
		buffer[1] = '\0';
		stack_results = buffer;
	    }
	    break;
	}
	    
	case opc_aconst_null:
	    stack_results = opcode_in_out[opcode][1]; 
	    full_info = NULL_FULLINFO; /* special NULL */
	    break;

	case opc_new: 
	case opc_checkcast: 
	case opc_newarray: 
	case opc_anewarray: 
        case opc_multianewarray:
	    stack_results = opcode_in_out[opcode][1]; 
	    /* Conventiently, this result type is stored here */
	    full_info = this_idata->operand.fi;
	    break;
			
	case opc_aaload:
	    stack_results = opcode_in_out[opcode][1]; 
	    /* pop_stack() saved value for us. */
	    full_info = context->swap_table[0];
	    break;
		
	case opc_aload:
	    stack_results = opcode_in_out[opcode][1]; 
	    /* The register hasn't been modified, so we can use its value. */
	    full_info = this_idata->register_info.registers[operand];
	    break;
    } /* of switch */
    
    for (p = stack_results; *p != 0; p++) {
	int type = *p;
	stack_item_type *new_item = NEW(stack_item_type, 1);
	new_item->next = stack;
	stack = new_item;
	switch (type) {
	    case 'I': 
	        stack->item = MAKE_FULLINFO(ITEM_Integer, 0, 0); break;
	    case 'F': 
		stack->item = MAKE_FULLINFO(ITEM_Float, 0, 0); break;
	    case 'D': 
		stack->item = MAKE_FULLINFO(ITEM_Double, 0, 0); 
		stack_size++; break;
	    case 'L': 
		stack->item = MAKE_FULLINFO(ITEM_Long, 0, 0); 
		stack_size++; break;
	    case 'R': 
		stack->item = MAKE_FULLINFO(ITEM_ReturnAddress, 0, operand);
		break;
	    case '1': case '2': case '3': case '4': {
		/* Get the info saved in the swap_table */
		fullinfo_type stype = context->swap_table[type - '1'];
		stack->item = stype;
		if (stype == MAKE_FULLINFO(ITEM_Long, 0, 0) || 
		    stype == MAKE_FULLINFO(ITEM_Double, 0, 0)) {
		    stack_size++; p++;
		}
		break;
	    }
	    case 'A': 
		/* full_info should have the appropriate value. */
		sysAssert(full_info != 0);
		stack->item = full_info;
		break;
	    default:
		CCerror(context, "Internal error #4");
		sysAssert(FALSE);

	    } /* switch type */
	stack_size++;
    } /* outer for loop */

    if (opcode == opc_invokeinit) {
	/* If there are any instances of "from" on the stack, we need to
	 * replace it with "to", since calling <init> initializes all versions
	 * of the object, obviously.     */
	fullinfo_type from = context->swap_table[0];
	stack_item_type *ptr;
	for (ptr = stack; ptr != NULL; ptr = ptr->next) {
	    if (ptr->item == from) {
		fullinfo_type to = context->swap_table[1];
		stack = copy_stack(context, stack);
		for (ptr = stack; ptr != NULL; ptr = ptr->next) 
		    if (ptr->item == from) ptr->item = to;
		break;
	    }
	}
    }

    new_stack_info->stack_size = stack_size;
    new_stack_info->stack = stack;
}


/* We've performed an instruction, and determined the new registers and stack 
 * value.  Look at all of the possibly subsequent instructions, and merge
 * this stack value into theirs. 
 */

static void
merge_into_successors(context_type *context, int inumber, 
		      register_info_type *register_info,
		      stack_info_type *stack_info)
{
    instruction_data_type *idata = context->instruction_data;
    instruction_data_type *this_idata = &idata[inumber];
    opcode_type opcode = this_idata->opcode;
    int32_t operand = this_idata->operand.i;
    struct handler_info_type *handler_info = context->handler_info;
    uint32_t handler_info_length = context->mb->exception_table_length;


    int32_t buffer[2];                  /* default value for successors */
    int32_t *successors = buffer;	/* table of successors */
    int32_t successors_count;
    int32_t i;
    
    switch (opcode) {
    default:
	successors_count = 1; 
	buffer[0] = inumber + 1;
	break;

    case opc_ifeq: case opc_ifne: case opc_ifgt: 
    case opc_ifge: case opc_iflt: case opc_ifle:
    case opc_ifnull: case opc_ifnonnull:
    case opc_if_icmpeq: case opc_if_icmpne: case opc_if_icmpgt: 
    case opc_if_icmpge: case opc_if_icmplt: case opc_if_icmple:
    case opc_if_acmpeq: case opc_if_acmpne:
	successors_count = 2;
	buffer[0] = inumber + 1; 
	buffer[1] = operand;
	break;

    case opc_jsr: case opc_jsr_w: 
	if (this_idata->operand2.i != UNKNOWN_RET_INSTRUCTION) 
	    idata[this_idata->operand2.i].changed = TRUE;
	/* FALLTHROUGH */
    case opc_goto: case opc_goto_w:
	successors_count = 1;
	buffer[0] = operand;
	break;


    case opc_ireturn: case opc_lreturn: case opc_return:	
    case opc_freturn: case opc_dreturn: case opc_areturn: 
    case opc_athrow:
	/* The testing for the returns is handled in pop_stack() */
	successors_count = 0;
	break;

    case opc_ret: {
	/* This is slightly slow, but good enough for a seldom used instruction.
	 * The EXTRA_ITEM_INFO of the ITEM_ReturnAddress indicates the
	 * address of the first instruction of the subroutine.  We can return
	 * to 1 after any instruction that jsr's to that instruction.
	 */
	if (this_idata->operand2.ip == NULL) {
	    fullinfo_type *registers = this_idata->register_info.registers;
	    fullinfo_type called_instruction = GET_EXTRA_INFO(registers[operand]);
	    int32_t i, count;
	    int32_t *ptr;
	    for (i = context->instruction_count, count = 0; --i >= 0; ) {
		if ((idata[i].opcode == opc_jsr) && 
		       (idata[i].operand.u == called_instruction)) 
		    count++;
	    }
	    this_idata->operand2.ip = ptr = NEW(int32_t, count + 1);
	    *ptr++ = count;
	    for (i = context->instruction_count, count = 0; --i >= 0; ) {
		if ((idata[i].opcode == opc_jsr) && 
		       (idata[i].operand.u == called_instruction)) 
		    *ptr++ = i + 1;
	    }
	}
	successors = this_idata->operand2.ip; /* use this instead */
	successors_count = *successors++;
	break;

    }

    case opc_tableswitch:
    case opc_lookupswitch: 
	successors = this_idata->operand.ip; /* use this instead */
	successors_count = *successors++;
	break;
    }

#ifdef DEBUG
    if (verify_verbose) { 
	printf(" [");
	for (i = handler_info_length; --i >= 0; handler_info++)
	    if (handler_info->start <= inumber && handler_info->end > inumber)
		printf("%d* ", handler_info->handler);
	for (i = 0; i < successors_count; i++)
	    printf("%d ", successors[i]);
	printf(  "]\n");
    }
#endif

    handler_info = context->handler_info;
    for (i = handler_info_length; --i >= 0; handler_info++) {
	if (handler_info->start <= inumber && handler_info->end > inumber) {
	    int handler = handler_info->handler;
	    merge_into_one_successor(context, inumber, handler, 
				     &this_idata->register_info, /* old vals */
				     &handler_info->stack_info, 
				     TRUE);
	}
    }
    if (successors_count > 0) {
	for (i = 0; i < successors_count; i++) {
	    int32_t target = successors[i];
	    if (target >= context->instruction_count) 
		CCerror(context, "Falling off the end of the code");
	    merge_into_one_successor(context, inumber, target, 
				     register_info, stack_info, FALSE);
	}
    }
}

/* We have a new set of registers and stack values for a given instruction.
 * Merge this new set into the values that are already there. 
 */

static void
merge_into_one_successor(context_type *context, 
			 int32_t from_inumber, int32_t to_inumber, 
			 register_info_type *new_register_info,
			 stack_info_type *new_stack_info,
			 bool_t isException)
{
    instruction_data_type *idata = context->instruction_data;
#ifdef DEBUG
    instruction_data_type *this_idata = &idata[to_inumber];
    register_info_type old_reg_info;
    stack_info_type old_stack_info;

    if (verify_verbose) {
	old_reg_info = this_idata->register_info;
	old_stack_info = this_idata->stack_info;
    }
#endif

    if (to_inumber <= from_inumber || isException) {
	int32_t register_count = new_register_info->register_count;
	fullinfo_type *registers = new_register_info->registers;
	stack_item_type *item;
	int32_t i;
	for (item = new_stack_info->stack; item != NULL; item = item->next) {
	    if (GET_ITEM_TYPE(item->item) == ITEM_NewObject)
		CCerror(context, "New object on stack in backwards branch");
	}
	for (i = 0; i < register_count; i++) {
	    if (GET_ITEM_TYPE(registers[i]) == ITEM_NewObject) {
		CCerror(context, "New object in registers in backwards branch");
	    }
	}
    }

    /* Returning from a subroutine is somewhat ugly.  The actual thing
     * that needs to get merged into the new instruction is a joinging
     * of info from the ret instruction with stuff in the jsr instruction 
     */
    if (idata[from_inumber].opcode == opc_ret && !isException) {
	int32_t new_register_count = new_register_info->register_count;
	fullinfo_type *new_registers = new_register_info->registers;
	int32_t new_mask_count = new_register_info->mask_count;
	mask_type *new_masks = new_register_info->masks;
	int32_t operand = idata[from_inumber].operand.i;
	codepos_t called_instruction = GET_EXTRA_INFO(new_registers[operand]);
	instruction_data_type *jsr_idata = &idata[to_inumber - 1];
	register_info_type *jsr_reginfo = &jsr_idata->register_info;
	if (jsr_idata->operand2.i != from_inumber) {
	    if (jsr_idata->operand2.i != UNKNOWN_RET_INSTRUCTION)
		CCerror(context, "Multiple returns to single jsr");
	    jsr_idata->operand2.i = from_inumber;
	}
	if (jsr_reginfo->register_count == UNKNOWN_REGISTER_COUNT) {
	    /* We don't want to handle the returned-to instruction until 
	     * we've dealt with the jsr instruction.   When we get to the
	     * jsr instruction (if ever), we'll re-mark the ret instruction
	     */
	    ;
	} else { 
	    int32_t register_count = jsr_reginfo->register_count;
	    fullinfo_type *registers = jsr_reginfo->registers;
	    int32_t max_registers = MAX(register_count, new_register_count);
	    fullinfo_type *new_set = NEW(fullinfo_type, max_registers);
	    int32_t *return_mask;
	    struct register_info_type new_new_register_info;
	    int32_t i;
	    /* Make sure the place we're returning from is legal! */
	    for (i = new_mask_count; --i >= 0; ) 
		if (new_masks[i].entry == called_instruction) 
		    break;
	    if (i < 0)
		CCerror(context, "Illegal return from subroutine");
	    /* pop the masks down to the indicated one.  Remember the mask
	     * we're popping off. */
	    return_mask = new_masks[i].modifies;
	    new_mask_count = i;
	    for (i = 0; i < max_registers; i++) {
		if (IS_BIT_SET(return_mask, i)) 
		    new_set[i] = i < new_register_count ? 
			  new_registers[i] : MAKE_FULLINFO(ITEM_Bogus, 0, 0);
		else 
		    new_set[i] = i < register_count ? 
			registers[i] : MAKE_FULLINFO(ITEM_Bogus, 0, 0);
	    }
	    new_new_register_info.register_count = max_registers;
	    new_new_register_info.registers      = new_set;
	    new_new_register_info.mask_count     = new_mask_count;
	    new_new_register_info.masks          = new_masks;


	    merge_stack(context, to_inumber - 1, to_inumber, new_stack_info);
	    merge_registers(context, to_inumber - 1, to_inumber, 
			    &new_new_register_info);
	}
    } else {
	merge_stack(context, from_inumber, to_inumber, new_stack_info);
	merge_registers(context, from_inumber, to_inumber, new_register_info);
    }

#ifdef DEBUG
    if (verify_verbose && idata[to_inumber].changed) {
	register_info_type *register_info = &this_idata->register_info;
	stack_info_type *stack_info = &this_idata->stack_info;
	if (memcmp(&old_reg_info, register_info, sizeof(old_reg_info)) ||
	    memcmp(&old_stack_info, stack_info, sizeof(old_stack_info))) {
	    printf("   %2d:", to_inumber);
	    print_stack(context, &old_stack_info);
	    print_registers(context, &old_reg_info);
	    printf(" => ");
	    print_stack(context, &idata[to_inumber].stack_info);
	    print_registers(context, &idata[to_inumber].register_info);
	    printf("\n");
	}
    }
#endif

}

static void
merge_stack(context_type *context, int32_t from_inumber, int32_t to_inumber, 
	    stack_info_type *new_stack_info)
{
    instruction_data_type *idata = context->instruction_data;
    instruction_data_type *this_idata = &idata[to_inumber];

    int32_t new_stack_size =  new_stack_info->stack_size;
    stack_item_type *new_stack = new_stack_info->stack;

    int32_t stack_size = this_idata->stack_info.stack_size;

    if (stack_size == UNKNOWN_STACK_SIZE) {
	/* First time at this instruction.  Just copy. */
	this_idata->stack_info.stack_size = new_stack_size;
	this_idata->stack_info.stack = new_stack;
	this_idata->changed = TRUE;
    } else if (new_stack_size != stack_size) {
	CCerror(context, "Inconsistent stack height %d != %d",
		new_stack_size, stack_size);
    } else { 
	stack_item_type *stack = this_idata->stack_info.stack;
	stack_item_type *old, *new;
	bool_t change = FALSE;
	for (old = stack, new = new_stack; old != NULL; 
	           old = old->next, new = new->next) {
	    if (!isAssignableTo(context, new->item, old->item)) {
		change = TRUE;
		break;
	    }
	}
	if (change) {
	    stack = copy_stack(context, stack);
	    for (old = stack, new = new_stack; old != NULL; 
		          old = old->next, new = new->next) {
		old->item = merge_fullinfo_types(context, old->item, new->item, 
						 FALSE);
	    }
	    this_idata->stack_info.stack = stack;
	    this_idata->changed = TRUE;
	}
    }
}

static void
merge_registers(context_type *context, int32_t from_inumber, int32_t to_inumber,
		 register_info_type *new_register_info)
{
    instruction_data_type *idata = context->instruction_data;
    instruction_data_type *this_idata = &idata[to_inumber];
    register_info_type	  *this_reginfo = &this_idata->register_info;

    int32_t            new_register_count = new_register_info->register_count;
    fullinfo_type *new_registers = new_register_info->registers;
    int32_t            new_mask_count = new_register_info->mask_count;
    mask_type     *new_masks = new_register_info->masks;
    

    if (this_reginfo->register_count == UNKNOWN_REGISTER_COUNT) {
	this_reginfo->register_count = new_register_count;
	this_reginfo->registers = new_registers;
	this_reginfo->mask_count = new_mask_count;
	this_reginfo->masks = new_masks;
	this_idata->changed = TRUE;
    } else {
	/* See if we've got new information on the register set. */
	int32_t register_count = this_reginfo->register_count;
	fullinfo_type *registers = this_reginfo->registers;
	int32_t mask_count = this_reginfo->mask_count;
	mask_type *masks = this_reginfo->masks;
	
	bool_t copy = FALSE;
	int i, j;
	if (register_count > new_register_count) {
	    /* Any register larger than new_register_count is now bogus */
	    this_reginfo->register_count = new_register_count;
	    register_count = new_register_count;
	    this_idata->changed = TRUE;
	}
	for (i = 0; i < register_count; i++) {
	    fullinfo_type prev_value = registers[i];
	    if ((i < new_register_count) 
		  ? (!isAssignableTo(context, new_registers[i], prev_value))
		  : (prev_value != MAKE_FULLINFO(ITEM_Bogus, 0, 0))) {
		copy = TRUE; 
		break;
	    }
	}
	
	if (copy) {
	    /* We need a copy.  So do it. */
	    fullinfo_type *new_set = NEW(fullinfo_type, register_count);
	    for (j = 0; j < i; j++) 
		new_set[j] =  registers[j];
	    for (j = i; j < register_count; j++) {
		if (i >= new_register_count) 
		    new_set[j] = MAKE_FULLINFO(ITEM_Bogus, 0, 0);
		else 
		    new_set[j] = merge_fullinfo_types(context, 
						      new_registers[j], 
						      registers[j], FALSE);
	    }
	    /* Some of the end items might now be bogus. This step isn't 
	     * necessary, but it may save work later. */
	    while (   register_count > 0
		   && GET_ITEM_TYPE(new_set[register_count-1]) == ITEM_Bogus) 
		register_count--;
	    this_reginfo->register_count = register_count;
	    this_reginfo->registers = new_set;
	    this_idata->changed = TRUE;
	}
	if (mask_count > 0) { 
	    /* If the target instruction already has a sequence of masks, then
	     * we need to merge new_masks into it.  We want the entries on
	     * the mask to be the longest common substring of the two.
	     *   (e.g.   a->b->d merged with a->c->d should give a->d)
	     * The bits set in the mask should be the or of the corresponding
	     * entries in eachof the original masks.
	     */
	    int32_t i, j, k;
	    int32_t matches = 0;
	    int32_t last_match = -1;
	    bool_t copy_needed = FALSE;
	    for (i = 0; i < mask_count; i++) {
		codepos_t entry = masks[i].entry;
		for (j = last_match + 1; j < new_mask_count; j++) {
		    if (new_masks[j].entry == entry) {
			/* We have a match */
			int32_t *prev = masks[i].modifies;
			int32_t *new = new_masks[j].modifies;
			matches++; 
			/* See if new_mask has bits set for "entry" that 
			 * weren't set for mask.  If so, need to copy. */
			for (k = context->bitmask_size - 1;
			       !copy_needed && k >= 0;
			       k--) 
			    if (~prev[k] & new[k])
				copy_needed = TRUE;
			last_match = j;
			break;
		    }
		}
	    }
	    if ((matches < mask_count) || copy_needed) { 
		/* We need to make a copy for the new item, since either the
		 * size has decreased, or new bits are set. */
		mask_type *copy = NEW(mask_type, matches);
		for (i = 0; i < matches; i++) {
		    copy[i].modifies = NEW(int32_t, context->bitmask_size);
		}
		this_reginfo->masks = copy;
		this_reginfo->mask_count = matches;
		this_idata->changed = TRUE;
		matches = 0;
		last_match = -1;
		for (i = 0; i < mask_count; i++) {
		    codepos_t entry = masks[i].entry;
		    for (j = last_match + 1; j < new_mask_count; j++) {
			if (new_masks[j].entry == entry) {
			    int32_t *prev1 = masks[i].modifies;
			    int32_t *prev2 = new_masks[j].modifies;
			    int32_t *new = copy[matches].modifies;
			    copy[matches].entry = entry;
			    for (k = context->bitmask_size - 1; k >= 0; k--) 
				new[k] = prev1[k] | prev2[k];
			    matches++;
			    last_match = j;
			    break;
			}
		    }
		}
	    }
	}
    }
}


/* Make a copy of a stack */

static stack_item_type *
copy_stack(context_type *context, stack_item_type *stack)
{
    int length;
    stack_item_type *ptr;
    
    /* Find the length */
    for (ptr = stack, length = 0; ptr != NULL; ptr = ptr->next, length++);
    
    if (length > 0) { 
	stack_item_type *new_stack = NEW(stack_item_type, length);
	stack_item_type *new_ptr;
	for (    ptr = stack, new_ptr = new_stack; 
	         ptr != NULL;
	         ptr = ptr->next, new_ptr++) {
	    new_ptr->item = ptr->item;
	    new_ptr->next = new_ptr + 1;
	}
	new_stack[length - 1].next = NULL;
	return new_stack;
    } else {
	return NULL;
    }
}


static mask_type *
copy_masks(context_type *context, mask_type *masks, int32_t mask_count)
{
    mask_type *result = NEW(mask_type, mask_count);
    int32_t bitmask_size = context->bitmask_size;
    int32_t *bitmaps = NEW(int32_t, mask_count * bitmask_size);
    int32_t i;
    for (i = 0; i < mask_count; i++) { 
	result[i].entry = masks[i].entry;
	result[i].modifies = &bitmaps[i * bitmask_size];
	memcpy(result[i].modifies, masks[i].modifies, (size_t)(bitmask_size * sizeof(bitmaps[0])));
    }
    return result;
}


static mask_type *
add_to_masks(context_type *context, mask_type *masks, int32_t mask_count, int32_t d)
{
    mask_type *result = NEW(mask_type, mask_count + 1);
    int32_t bitmask_size = context->bitmask_size;
    int32_t *bitmaps = NEW(int32_t, (mask_count + 1) * bitmask_size);
    int32_t i;
    for (i = 0; i < mask_count; i++) { 
	result[i].entry = masks[i].entry;
	result[i].modifies = &bitmaps[i * bitmask_size];
	memcpy(result[i].modifies, masks[i].modifies, (size_t)(bitmask_size * sizeof(bitmaps[0])));
    }
    result[mask_count].entry = d;
    result[mask_count].modifies = &bitmaps[mask_count * bitmask_size];
    memset(result[mask_count].modifies, 0, (size_t)(bitmask_size * sizeof(bitmaps[0])));
    return result;
}
    


/* We create our own storage manager, since we malloc lots of little items, 
 * and I don't want to keep trace of when they become free.  I sure wish that
 * we had heaps, and I could just free the heap when done. 
 */

#define CCSegSize 2000

struct CCpool {			/* a segment of allocated memory in the pool */
    struct CCpool *next;
    uint32_t segSize;		/* almost always CCSegSize */
#ifdef OSF1
    int pad;			/* keep space 8 byte aligned */
#endif
    char space[CCSegSize];
};

/* Initialize the context's heap. */
static void CCinit(context_type *context)
{
    struct CCpool *new = (struct CCpool *) sysMalloc(sizeof(struct CCpool));
	if (new == NULL) {
	    CCerror(context, "Out of memory on verify");
	}
    new->next = NULL;
    new->segSize = CCSegSize;
    context->CCroot = context->CCcurrent = new;
    context->CCfree_size = CCSegSize;
    context->CCfree_ptr = &new->space[0];
}


/* Reuse all the space that we have in the context's heap. */
static void CCreinit(context_type *context)
{
    struct CCpool *first = context->CCroot;
    context->CCcurrent = first;
    context->CCfree_size = CCSegSize;
    context->CCfree_ptr = &first->space[0];
}

/* Destroy the context's heap. */
static void CCdestroy(context_type *context)
{
    struct CCpool *this = context->CCroot;
    while (this) {
	struct CCpool *next = this->next;
	sysFree(this);
	this = next;
    }
    /* These two aren't necessary.  But can't hurt either */
    context->CCroot = context->CCcurrent = NULL;
    context->CCfree_ptr = 0;
}

/* Allocate an object of the given size from the context's heap. */
static void *
CCalloc(context_type *context, uint32_t size, bool_t zero)
{

    register char *p;
    /* Round CC to the size of a pointer */
    size = (size + (sizeof(void *) - 1)) & ~(sizeof(void *) - 1);

#ifdef SPACE_DEBUG
     p = sysMalloc(size);
     if (zero) 
	memcpy(p, 0, size);
     return p;
#endif    

    if (context->CCfree_size <  size) {
	struct CCpool *current = context->CCcurrent;
	struct CCpool *new;
	if (size > CCSegSize) {	/* we need to allocate a special block */
	    new = (struct CCpool *)sysMalloc((size_t)(sizeof(struct CCpool) + 
					     (size - CCSegSize)));
		if (new == NULL) {
		    CCerror(context, "Out of memory on verify");
		}
	    new->next = current->next;
	    new->segSize = size;
	    current->next = new;
	} else {
	    new = current->next;
	    if (new == NULL) {
		new = (struct CCpool *) sysMalloc(sizeof(struct CCpool));
		if (new == NULL) {
		    CCerror(context, "Out of memory on verify");
		}
		current->next = new;
		new->next = NULL;
		new->segSize = CCSegSize;
	    }
	}
	context->CCcurrent = new;
	context->CCfree_ptr = &new->space[0];
	context->CCfree_size = new->segSize;
    }
    p = context->CCfree_ptr;
    context->CCfree_ptr += size;
    context->CCfree_size -= size;
    if (zero) 
	memcpy(p, 0, (size_t) size);
    return p;
}


/* Get the signature associated with a particular field or method in 
 * the constant pool. 
 */
static char *
cp_index_to_signature(context_type *context, cp_index_type cp_index)
{
    union cp_item_type *cp = context->class->constantpool;
    int32_t index = cp[cp_index].i;     /* value of Fieldref field */
    int32_t key2 = index & 0xFFFF;	/* index to NameAndType  */
    int32_t signature_index = cp[key2].i & 0xFFFF; 
    char *signature = cp[signature_index].cp;
    return signature;
}

static char *
cp_index_to_fieldname(context_type *context, cp_index_type cp_index)
{
    union cp_item_type *cp = context->class->constantpool;
    int32_t index = cp[cp_index].i;     /* value of Fieldref field */
    int32_t key2 = index & 0xFFFF;	/* index to NameAndType  */
    int32_t name_index = (int32_t)(cp[key2].u >> 16);
    return cp[name_index].cp;
}


/* Get the class associated with a particular field or method or class in the
 * constant pool.  If is_field is true, we've got a field or method.  If 
 * false, we've got a class.
 */
static fullinfo_type
cp_index_to_class_fullinfo(context_type *context, cp_index_type cp_index, bool_t is_field)
{
    union cp_item_type *cp = context->class->constantpool;
    uint32_t classkey = is_field ? (cp[cp_index].u >> 16) : cp_index;
    char *classname = GetClassConstantClassName(cp, classkey);
    if (classname[0] == SIGNATURE_ARRAY) {
	fullinfo_type result;
	/* This make recursively call us, in case of a class array */
	signature_to_fieldtype(context, &classname, &result);
	return result;
    } else {
	return MAKE_CLASSNAME_INFO(classname, 0);
    }
}


static void 
CCerror (context_type *context, char *format, ...)
{
    struct methodblock *mb = context->mb;
    va_list args;
    #define BUF_LEN	1024
    char buf[BUF_LEN];
    int i = 0;
    i += jio_snprintf(buf, BUF_LEN, "VERIFIER ERROR %s.%s%s: ", 
		      fieldclass(&mb->fb)->name, mb->fb.name, mb->fb.signature);
    va_start(args, format);
	i += jio_vsnprintf(buf + i, BUF_LEN - i, format, args);
    va_end(args);
    i += jio_snprintf(buf + i, BUF_LEN - i, "\n");
#ifdef DEBUG
    fprintf(stderr, buf);
#endif /* DEBUG */
    PrintToConsole(buf);
    longjmp(context->jump_buffer, 1);
}


static char 
signature_to_fieldtype(context_type *context, 
		       char **signature_p, fullinfo_type *full_info_p)
{
    char *p = *signature_p;
    fullinfo_type full_info = MAKE_FULLINFO(0, 0, 0);
    char result;
    int array_depth = 0;
    
    for (;;) { 
	switch(*p++) {
            default:
		full_info = MAKE_FULLINFO(ITEM_Bogus, 0, 0);
		result = 0; 
		break;

            case SIGNATURE_BOOLEAN: case SIGNATURE_BYTE: 
		full_info = (array_depth > 0)
		           ? MAKE_FULLINFO(ITEM_Byte, 0, 0)
			   : MAKE_FULLINFO(ITEM_Integer, 0, 0);
	    result = 'I'; 
	    break;

            case SIGNATURE_CHAR:
		full_info = (array_depth > 0)
		           ? MAKE_FULLINFO(ITEM_Char, 0, 0)
			   : MAKE_FULLINFO(ITEM_Integer, 0, 0);
	    result = 'I'; 
	    break;

            case SIGNATURE_SHORT: 
		full_info = (array_depth > 0)
		           ? MAKE_FULLINFO(ITEM_Short, 0, 0)
			   : MAKE_FULLINFO(ITEM_Integer, 0, 0);
	    result = 'I'; 
	    break;

        case SIGNATURE_INT:
	    full_info = MAKE_FULLINFO(ITEM_Integer, 0, 0);
	    result = 'I'; 
	    break;

        case SIGNATURE_FLOAT:
	    full_info = MAKE_FULLINFO(ITEM_Float, 0, 0);
	    result = 'F'; 
	    break;

        case SIGNATURE_DOUBLE:
	    full_info = MAKE_FULLINFO(ITEM_Double, 0, 0);
	    result = 'D'; 
	    break;

        case SIGNATURE_LONG:
	    full_info = MAKE_FULLINFO(ITEM_Long, 0, 0);
	    result = 'L'; 
	    break;

            case SIGNATURE_ARRAY:
		array_depth++;
		continue;	/* only time we ever do the loop > 1 */

            case SIGNATURE_CLASS: {
		char buffer_space[256];
		char *buffer = buffer_space;
		char *finish = strchr(p, SIGNATURE_ENDCLASS);
		int length = finish - p;
		if (length + 1 > sizeof(buffer_space))
		    buffer = sysMalloc(length + 1);
		if (buffer == NULL) {
		    CCerror(context, "Out of memory on verify");
		}
	        memcpy(buffer, p, length);
		buffer[length] = '\0';
		full_info = MAKE_CLASSNAME_INFO_WITH_COPY(buffer);
		result = 'A';
		p = finish + 1;
		if (buffer != buffer_space) 
		    sysFree(buffer);
		break;
	    }
	} /* end of switch */
	break;
    }
    *signature_p = p;
    if (array_depth == 0 || result == 0) { 
	/* either not an array, or result is bogus */
	*full_info_p = full_info;
	return result;
    } else {
	if (array_depth > MAX_ARRAY_DIMENSIONS) 
	    CCerror(context, "Array with too many dimensions");
	*full_info_p = MAKE_FULLINFO(GET_ITEM_TYPE(full_info),
				     array_depth, 
				     GET_EXTRA_INFO(full_info));
	return 'A';
    }
}


/* Given an array type, create the type that has one less level of 
 * indirection.
 */

static fullinfo_type
decrement_indirection(fullinfo_type array_info)
{
    if (array_info == NULL_FULLINFO) { 
	return NULL_FULLINFO;
    } else { 
	int type = GET_ITEM_TYPE(array_info);
	int indirection = GET_INDIRECTION(array_info) - 1;
	int extra_info = GET_EXTRA_INFO(array_info);
	if (   (indirection == 0) 
	       && ((type == ITEM_Short || type == ITEM_Byte || type == ITEM_Char)))
	    type = ITEM_Integer;
	return MAKE_FULLINFO(type, indirection, extra_info);
    }
}


/* See if we can assign an object of the "from" type to an object
 * of the "to" type.
 */

static bool_t isAssignableTo(context_type *context, 
			     fullinfo_type from, fullinfo_type to)
{
    return (merge_fullinfo_types(context, from, to, TRUE) == to);
}

/* Given two fullinfo_type's, find their lowest common denominator.  If
 * the assignable_p argument is non-null, we're really just calling to find
 * out if "<target> := <value>" is a legitimate assignment.  
 *
 * We treat all interfaces as if they were of type java/lang/Object, since the
 * runtime will do the full checking.
 */
static fullinfo_type 
merge_fullinfo_types(context_type *context, 
		     fullinfo_type value, fullinfo_type target,
		     bool_t for_assignment)
{
    if (value == target) {
	/* If they're identical, clearly just return what we've got */
	return value;
    }

    /* Both must be either arrays or objects to go further */
    if (GET_INDIRECTION(value) == 0 && GET_ITEM_TYPE(value) != ITEM_Object)
	return MAKE_FULLINFO(ITEM_Bogus, 0, 0);
    if (GET_INDIRECTION(target) == 0 && GET_ITEM_TYPE(target) != ITEM_Object)
	return MAKE_FULLINFO(ITEM_Bogus, 0, 0);
    
    /* If either is NULL, return the other. */
    if (value == NULL_FULLINFO) 
	return target;
    else if (target == NULL_FULLINFO)
	return value;

    /* If either is java/lang/Object, that's the result. */
    if (target == context->object_info)
	return target;
    else if (value == context->object_info) {
	/* Minor hack.  For assignments, Interface := Object, return Interface
	 * rather than Object, so that isAssignableTo() will get the right
	 * result.      */
	if (for_assignment && (WITH_ZERO_EXTRA_INFO(target) == 
			          MAKE_FULLINFO(ITEM_Object, 0, 0))) {
	    ClassClass *cb = object_fullinfo_to_classclass(context, target);
	    if (cb && cbIsInterface(cb)) 
		return target;
	}
	return value;
    }
    if (GET_INDIRECTION(value) > 0 || GET_INDIRECTION(target) > 0) {
	/* At least one is an array.  Neither is java/lang/Object or NULL.
	 * Moreover, the types are not identical.
	 * The result must either be Object, or an array of some object type.
	 */
	fullinfo_type dimen_value = GET_INDIRECTION(value);
	fullinfo_type dimen_target = GET_INDIRECTION(target);
	
	/* First, if either item's base type isn't ITEM_Object, promote it up
         * to an object or array of object.  If either is elemental, we can
	 * punt.
    	 */
	if (GET_ITEM_TYPE(value) != ITEM_Object) { 
	    if (dimen_value == 0)
		return MAKE_FULLINFO(ITEM_Bogus, 0, 0);
	    dimen_value--;
	    value = MAKE_Object_ARRAY(dimen_value);
	    
	}
	if (GET_ITEM_TYPE(target) != ITEM_Object) { 
	    if (dimen_target == 0)
		return MAKE_FULLINFO(ITEM_Bogus, 0, 0);
	    dimen_target--;
	    target = MAKE_Object_ARRAY(dimen_target);
	}
	/* Both are now objects or arrays of some sort of object type */
	if (dimen_value == dimen_target) { 
            /* Arrays of the same dimension.  Merge their base types. */
	    fullinfo_type value_base = WITH_ZERO_INDIRECTION(value);
	    fullinfo_type target_base = WITH_ZERO_INDIRECTION(target);
	    fullinfo_type  result_base = 
		merge_fullinfo_types(context, value_base, target_base,
					    for_assignment);
	    if (result_base == MAKE_FULLINFO(ITEM_Bogus, 0, 0))
		/* bogus in, bogus out */
		return result_base;
	    return MAKE_FULLINFO(ITEM_Object, dimen_value,
				 GET_EXTRA_INFO(result_base));
	} else { 
            /* Arrays of different sizes.  Return Object, with a dimension
             * of the smaller of the two.
             */
	    fullinfo_type dimen = dimen_value < dimen_target ? dimen_value : dimen_target;
	    return MAKE_Object_ARRAY(dimen);
	}
    } else {
	/* Both are non-array objects. Neither is java/lang/Object or NULL */
	ClassClass *cb_value, *cb_target, *cb_super_value, *cb_super_target;
	void **addr;
	int value_info;

	/* Let's get the classes corresponding to each of these.  Treat 
	 * interfaces as if they were java/lang/Object.  See hack note above. */
	cb_target = object_fullinfo_to_classclass(context, target);
	if (cb_target == 0) 
	    return MAKE_FULLINFO(ITEM_Bogus, 0, 0);
	if (cbIsInterface(cb_target)) 
	    return for_assignment ? target : context->object_info;
	cb_value = object_fullinfo_to_classclass(context, value);
	if (cb_value == 0) 
	    return MAKE_FULLINFO(ITEM_Bogus, 0, 0);
	if (cbIsInterface(cb_value))
	    return context->object_info;
	
	/* If this is for assignment of target := value, we just need to see if
	 * cb_target is a superclass of cb_value.  Save ourselves a lot of 
	 * work.
	 */
	if (for_assignment) {
	    for (cb_super_value = cb_value; 
		 cbSuperclass(cb_super_value) != NULL; 
		 cb_super_value = unhand(cbSuperclass(cb_super_value))) {
		if (cb_super_value == cb_target) {
		    return target;
		}
	    }
	    return context->object_info;
	}

	/* Find out whether cb_value or cb_target is deeper in the class
	 * tree by moving both toward the root, and seeing who gets there
	 * first.                                                          */
	for (cb_super_value = cb_value, cb_super_target = cb_target;
	     cbSuperclass(cb_super_value) && cbSuperclass(cb_super_target); ) {
	    /* Optimization.  If either hits the other when going up looking
	     * for a parent, then might as well return the parent immediately */
	    if (cb_super_value == cb_target) 
		return target;
	    if (cb_super_target == cb_value) 
		return value;
	    cb_super_value= unhand(cbSuperclass(cb_super_value));
	    cb_super_target = unhand(cbSuperclass(cb_super_target));
	} 
	/* At most one of the following two while clauses will be executed. 
	 * Bring the deeper of cb_target and cb_value to the depth of the 
	 * shallower one. 
	 */
	while (cbSuperclass(cb_super_value)) { /* cb_value is deeper */
	    cb_super_value= unhand(cbSuperclass(cb_super_value)); 
	    cb_value= unhand(cbSuperclass(cb_value)); 
	}
	while (cbSuperclass(cb_super_target)) { /* cb_target is deeper */
	    cb_super_target= unhand(cbSuperclass(cb_super_target)); 
	    cb_target = unhand(cbSuperclass(cb_target)); 
	}
	
	/* Walk both up, maintaining equal depth, until a join is found.  We
	 * know that we will find one.  */
	while (cb_value != cb_target) { 
	    cb_value =  unhand(cbSuperclass(cb_value));
	    cb_target =  unhand(cbSuperclass(cb_target));
	}
	/* Get the info for this guy.  We know its cb_value, so we should
	 * fill that in, while we're at it.                      */
	value_info = Str2ID(&context->classHash, cb_value->name, &addr, FALSE);
	*addr = cb_value;
	return MAKE_FULLINFO(ITEM_Object, 0, value_info);
    } /* both items are classes */
}



/* Given a fullinfo_type corresponding to an Object, return the ClassClass *
 * structure of that type.
 *
 * This will return 0 on an illegal class.
 */

static ClassClass *
object_fullinfo_to_classclass(context_type *context, fullinfo_type classinfo)
{
    void **addr;
    ClassClass *cb;
	    
    fullinfo_type info = GET_EXTRA_INFO(classinfo);
    char *classname = ID2Str(context->classHash, (int16_t) info, &addr);
    if ((cb = *addr) != 0) {
	return cb;
    } else {
	*addr = cb = FindClassFromClass(0, classname, FALSE, context->class);
	if (cb == 0)
	    CCerror(context, "Cannot find class %s", classname);
	return cb;
    }
}

#ifdef DEBUG

/* Below are for debugging. */

static void print_fullinfo_type(context_type *, fullinfo_type, bool_t);

static void 
print_stack(context_type *context, stack_info_type *stack_info)
{
    stack_item_type *stack = stack_info->stack;
    if (stack_info->stack_size == UNKNOWN_STACK_SIZE) {
	printf("x");
    } else {
	printf("(");
	for ( ; stack != 0; stack = stack->next) 
	    print_fullinfo_type(context, stack->item, verify_verbose > 1);
	printf(")");
    }
}	

static void
print_registers(context_type *context, register_info_type *register_info)
{
    int32_t register_count = register_info->register_count;
    if (register_count == UNKNOWN_REGISTER_COUNT) {
	printf("x");
    } else {
	fullinfo_type *registers = register_info->registers;
	int32_t mask_count = register_info->mask_count;
	mask_type *masks = register_info->masks;
	uint32_t i, j;

	printf("{");
	for (i = 0; i < (uint32_t) register_count; i++) 
	    print_fullinfo_type(context, registers[i], verify_verbose > 1);
	printf("}");
	for (i = 0; i < (uint32_t) mask_count; i++) { 
	    char *separator = "";
	    int32_t *modifies = masks[i].modifies;
	    printf("<%d: ", masks[i].entry);
	    for (j = 0; j < context->mb->nlocals; j++) 
		if (IS_BIT_SET(modifies, j)) {
		    printf("%s%d", separator, j);
		    separator = ",";
		}
	    printf(">");
	}
    }
}


static void 
print_fullinfo_type(context_type *context, fullinfo_type type, bool_t verbose) 
{
    fullinfo_type i;
    fullinfo_type indirection = GET_INDIRECTION(type);
    for (i = indirection; i-- > 0; )
	printf("[");
    switch (GET_ITEM_TYPE(type)) {
        case ITEM_Integer:       
	    printf("I"); break;
	case ITEM_Float:         
	    printf("F"); break;
	case ITEM_Double:        
	    printf("D"); break;
	case ITEM_Double_2:      
	    printf("d"); break;
	case ITEM_Long:          
	    printf("L"); break;
	case ITEM_Long_2:        
	    printf("l"); break;
	case ITEM_ReturnAddress: 
	    printf("a"); break;
	case ITEM_Object:        
	    if (!verbose) {
		printf("A");
	    } else {
		unsigned short extra = (unsigned short) GET_EXTRA_INFO(type);
		if (extra == 0) {
		    printf("/Null/");
		} else {
		    char *name = ID2Str(context->classHash, extra, 0);
		    char *name2 = strrchr(name, '/');
		    printf("/%s/", name2 ? name2 + 1 : name);
		}
	    }
	    break;
	case ITEM_Char:
	    printf("C"); break;
	case ITEM_Short:
	    printf("S"); break;
	case ITEM_Byte:
	    printf("B"); break;
        case ITEM_NewObject:
	    if (!verbose) {
		printf("@");
	    } else {
		int32_t inum = (int32_t) GET_EXTRA_INFO(type);
		fullinfo_type real_type = 
		    context->instruction_data[inum].operand2.fi;
		printf(">");
		print_fullinfo_type(context, real_type, TRUE);
		printf("<");
	    }
	    break;
        case ITEM_InitObject:
	    printf(verbose ? ">/this/<" : "@");
	    break;

	default: 
	    printf("?"); break;
    }
    for (i = indirection; i-- > 0; )
	printf("]");
}


static void 
print_formatted_fieldname(context_type *context, cp_index_type index)
{
    union cp_item_type *constant_pool = context->class->constantpool;
    unsigned char *type_table = 
	    constant_pool[CONSTANT_POOL_TYPE_TABLE_INDEX].p;
    unsigned type = CONSTANT_POOL_TYPE_TABLE_GET_TYPE(type_table, index);

    uint32_t key = constant_pool[index].u; 
    uint32_t classkey = key >> 16; 
    uint32_t nametypekey = key & 0xFFFF;
    uint32_t nametypeindex = constant_pool[nametypekey].u;
    uint32_t fieldnameindex = nametypeindex >> 16;
    uint32_t fieldtypeindex = nametypeindex & 0xFFFF;
    printf("  <%s.%s%s%s>",
	    GetClassConstantClassName(constant_pool, classkey),
	    constant_pool[fieldnameindex].cp, 
	    type == CONSTANT_Fieldref ? " " : "",
	    constant_pool[fieldtypeindex].cp);
}

#endif /*DEBUG*/
