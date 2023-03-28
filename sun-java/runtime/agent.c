#ifdef BREAKPTS
/*
 * @(#)agent.c	1.44 96/02/16 Arthur van Huff, Thomas Ball
 *
 * Copyright (c) 1995 Sun Microsystems, Inc. All Rights Reserved.
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
 *	Routines to support the debugging agent.  
 */


#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include "oobj.h"
#include "tree.h"
#include "interpreter.h"
#include "javaThreads.h"
#include "signature.h"
#include "javaString.h"
#include "opcodes.h"
#include "typecodes.h"

#include "java_lang_Thread.h"
#include "java_lang_System.h"
#include "java_lang_Throwable.h"
#include "sun_tools_java_Constants.h"

#include "sun_tools_debug_Agent.h"
#include "sun_tools_debug_AgentConstants.h"
#include "sun_tools_debug_BreakpointHandler.h"
#include "sun_tools_debug_Field.h"
#include "sun_tools_debug_StackFrame.h"
#include "sun_tools_debug_LocalVariable.h"
#include "sun_tools_debug_LineNumber.h"

#define get_classblock(o) ((obj_classblock(o) == get_classClass()) ? \
			   (ClassClass *)unhand(o) : obj_classblock(o))
#define obj_getoffset(o, off) (*(OBJECT *)((char *)unhand(o)+(off)))

#ifdef DEBUG
int be_verbose = 0;

int dprintf(char *fmt, ...)
{
    int ret = 0;

    if (be_verbose) {
	va_list args;
	va_start(args, fmt);
	ret = fprintf(stderr, "agent.c: ");
	vfprintf(stderr, fmt, args);
	va_end(args);
    }
    return ret;
}
#else
void dprintf(char *fmt, ...) {}
#endif
extern void setDebugState(void);

static Hsun_tools_debug_Agent *agentInstance;

void handleExit(void)
{
    execute_java_dynamic_method(EE(), (HObject *)agentInstance, 
                                "reportAppExit", "()V");
    sleep(1);
}

void sun_tools_debug_Agent_initAgent(Hsun_tools_debug_Agent *this)
{
    setDebugState();
    agentInstance = this;
    sysAtexit(handleExit);
}

long sun_tools_debug_Agent_objectId(Hsun_tools_debug_Agent *this, 
				     HObject *obj)
{
    return (long)obj;
}

void sun_tools_debug_Agent_runMain(Hsun_tools_debug_Agent *this, 
				    HClass *c, HArrayOfString *argv) 
{
    execute_java_static_method(0, unhand(c), "main", 
			       "([Ljava/lang/String;)V", argv);
}

static void
RPI_decode_stack_frame(JavaFrame	*frame,
		       HClass		**clazz,
		       HString		**className,
		       HString		**methodName,
		       long		*lineno,
		       long		*pc,
		       HArrayOfObject	**localVars)
{
    struct methodblock	*mb = frame->current_method;
    if (mb != 0) {
	char		buf[128];
	HObject		**localVarsData;
	int		i;
	int		nLocalVars;
	struct localvar	*lv;
	Hsun_tools_debug_LineNumber *ln;

	/* a normal method */
	ClassClass *cb = fieldclass(&mb->fb);

	*clazz = cbHandle(cb);
	classname2string(classname(cb), buf, sizeof(buf));
	*className = makeJavaString(buf, strlen(buf));
	strcpy(buf, fieldname(&mb->fb));
	/*strcat(buf, fieldsig(&mb->fb)); */
	*methodName = makeJavaString(buf, strlen(buf));
	*pc = frame->lastpc - mb->code;
	*lineno = (long)pc2lineno(mb, *pc);

	nLocalVars = (int)mb->localvar_table_length;
	*localVars = (HArrayOfObject *) ArrayAlloc(T_CLASS, nLocalVars);
        if (localVars == 0) {
            SignalError(0, JAVAPKG "OutOfMemoryError", 0);
            return;
        }
	localVarsData = (HObject **)unhand(*localVars);
	localVarsData[nLocalVars] = (HObject *)get_classObject();

	lv = mb->localvar_table;
	for (i = 0; i < nLocalVars; i++, lv++) {
	    long currentPC = (long)frame->lastpc - (long)mb->code;
	    Hsun_tools_debug_LocalVariable *hlocalvar;
	    char *p;
	    
	    hlocalvar = (Hsun_tools_debug_LocalVariable*)
		execute_java_constructor(0, 
                    "sun/tools/debug/LocalVariable", 0, "()");

	    if (!hlocalvar) {
	        dprintf("agent.c: hlocalvar==0\n");
	        SignalError(0, JAVAPKG "NullPointerException", 0);
	    }
            unhand(hlocalvar)->methodArgument = 0;

	    if (currentPC >= lv->pc0 && 
		currentPC <= (lv->pc0 + lv->length)) {
		unhand(hlocalvar)->slot      = lv->slot;
                if (lv->slot < mb->args_size) {
                    unhand(hlocalvar)->methodArgument = TRUE;
                }
	    } else {
		unhand(hlocalvar)->slot      = -1;
	    }
		p = frame->constant_pool[lv->nameoff].cp;
		unhand(hlocalvar)->name      = makeJavaString(p, strlen(p));
		p = frame->constant_pool[lv->sigoff].cp;
		unhand(hlocalvar)->signature = makeJavaString(p, strlen(p));

	    localVarsData[i] = (HObject *)hlocalvar;
	}

    } else {
	/* a foreign function */
	HObject		**localVarsData;
	*lineno = -1;
	*pc = -1;
	*localVars = (HArrayOfObject *) ArrayAlloc(T_CLASS, 0);
        if (localVars == 0) {
            SignalError(0, JAVAPKG "OutOfMemoryError", 0);
            return;
        }
	localVarsData = (HObject **)unhand(*localVars);
	localVarsData[0] = (HObject *)get_classObject();
	dprintf("native method stack frame\n");
    }
}

JavaFrame *
RPI_get_stack_frame(Hjava_lang_Thread *h, long framenum)
{
    ExecEnv	*henv = (ExecEnv *)unhand(h)->eetop;
    JavaFrame	*jframe = henv->current_frame;
    int		fr = framenum;    

    while (jframe) {
        /* skip C stack frames */
        if (jframe->current_method == 0) {
            jframe = jframe->prev;
        }
        if (--fr >= 0 && jframe) {
            jframe = jframe->prev;
        } else {
            break;
        }
    }
    return jframe;
}

Hsun_tools_debug_StackFrame *
sun_tools_debug_Agent_getStackFrame(Hsun_tools_debug_Agent *hagent,
				     Hjava_lang_Thread *t,
				     long framenum)
{
    Hsun_tools_debug_StackFrame	*hframe;
    struct javaframe			*jframe;
    ExecEnv				*ee;

    if (unhand(t)->eetop == 0) {
	return NULL;
    }

    jframe = RPI_get_stack_frame(t, framenum);
    if (!jframe) {
	dprintf("agent.c: jframe==0 framenum=%d\n", framenum);
	SignalError(0, JAVAPKG "NullPointerException", 0);
        return NULL;
    }

    hframe = (Hsun_tools_debug_StackFrame*)
	     execute_java_constructor(0, "sun/tools/debug/StackFrame", 
				      0, "()");
    if (!hframe) {
	dprintf("agent.c: hframe==0\n");
	SignalError(0, JAVAPKG "NullPointerException", 0);
        return NULL;
    }

    RPI_decode_stack_frame(jframe,
			   &(unhand(hframe)->clazz),
			   &(unhand(hframe)->className),
			   &(unhand(hframe)->methodName),
			   &(unhand(hframe)->lineno),
			   &(unhand(hframe)->pc),
			   &(unhand(hframe)->localVariables));

    return hframe;
}


HObject *
sun_tools_debug_Agent_getSlotObject(Hsun_tools_debug_Agent *this, 
				     HObject *o, long slot)
{
    ClassClass *cb;
    struct fieldblock *fb, **slots;
    int nslots;

    if (o == 0) {
	SignalError(0, JAVAPKG "NullPointerException", 0);
	return NULL;
    }

    cb = get_classblock(o);
    if (makeslottable(cb) == NULL) {
	SignalError(0, JAVAPKG "OutOfMemoryError", 0);
	return NULL;
    }
    nslots = cbSlotTableSize(cb);
    slots = cbSlotTable(cb);
    if (slot >= nslots || slot < 0 || (fb = slots[slot]) == 0
	    || fieldsig(fb)[0] != SIGNATURE_CLASS) {
	SignalError(0, JAVAPKG "IllegalArgumentException", "not an object");
	return NULL;
    }
    return (fb->access & ACC_STATIC
	    ? *(HObject **) normal_static_address(fb)
	    : (HObject *) obj_getoffset(o, fb->u.offset));
}

static struct fieldblock *
getFieldBlock (HObject *o, long slot) {
    ClassClass *cb;
    struct fieldblock *fb, **slots;
    int nslots;

    if (o == 0) {
	SignalError(0, JAVAPKG "NullPointerException", 0);
	return NULL;
    }

    cb = get_classblock(o);
    if (makeslottable(cb) == NULL) {
	SignalError(0, JAVAPKG "OutOfMemoryError", 0);
	return NULL;
    }
    nslots = cbSlotTableSize(cb);
    slots = cbSlotTable(cb);
    if (slot >= nslots || slot < 0 || (fb = slots[slot]) == 0) {
	return NULL;
    }

    return fb;
}

static int signature2Type(int signature) {
    static struct { 
	int sig;
	int type;
    } table[] = {
	SIGNATURE_ANY, -1,
	SIGNATURE_ARRAY, sun_tools_java_Constants_TC_ARRAY,
	SIGNATURE_BYTE, sun_tools_java_Constants_TC_BYTE,
	SIGNATURE_CHAR, sun_tools_java_Constants_TC_CHAR,
	SIGNATURE_CLASS, sun_tools_debug_AgentConstants_TP_OBJECT,
	SIGNATURE_ENUM, -1,
	SIGNATURE_FLOAT, sun_tools_java_Constants_TC_FLOAT,
	SIGNATURE_DOUBLE, sun_tools_java_Constants_TC_DOUBLE,
	SIGNATURE_FUNC, -1,
	SIGNATURE_INT, sun_tools_java_Constants_TC_INT,
	SIGNATURE_LONG, sun_tools_java_Constants_TC_LONG,
	SIGNATURE_SHORT, sun_tools_java_Constants_TC_SHORT,
	SIGNATURE_VOID, sun_tools_java_Constants_TC_VOID,
	SIGNATURE_BOOLEAN, sun_tools_java_Constants_TC_BOOLEAN,
	0
    };

    int i;

    for (i = 0; table[i].sig != 0; i++) {
	if (signature == table[i].sig) {
	    return table[i].type;
	}
    }
    return -1;
}
	

HArrayOfInt *
sun_tools_debug_Agent_getSlotSignature(Hsun_tools_debug_Agent * this, 
					HClass * c, long slot)
{
    OBJECT 		*v;
    struct fieldblock	*fb;
    HArrayOfInt		*ret;
    ArrayOfInt		*val;
    int			i;
    char		*sig;

    fb = getFieldBlock((HObject *)c, slot);
    if (fb == 0) {
	return NULL;
    }

    sig = fieldsig(fb);
    ret = (HArrayOfInt *) ArrayAlloc(T_INT, strlen(sig));
    if (ret == 0) {
        SignalError(0, JAVAPKG "OutOfMemoryError", 0);
        return NULL;
    }
    val = (ArrayOfInt *)unhand(ret);
    for (i = 0; i < (int)strlen(sig); i++) {
	val->body[i] = signature2Type(sig[i]);
    }
    return ret;
}

long 
sun_tools_debug_Agent_getSlotBoolean(Hsun_tools_debug_Agent * this, 
				      HObject * o, long slot)
{
    OBJECT *v;
    struct fieldblock *fb;
    int sig;

    fb = getFieldBlock(o, slot);
    if (fb == 0) {
	return 0;
    }

    if (fieldsig(fb)[0] != SIGNATURE_BOOLEAN) {
	SignalError(0, JAVAPKG "IllegalAccessError","invalid slot type");
	return 0;
    }
    v = (fb->access & ACC_STATIC
	 ? normal_static_address(fb)
	 : &obj_getoffset(o, fb->u.offset));
    return (long)*v;
}


long
sun_tools_debug_Agent_getSlotInt(Hsun_tools_debug_Agent * this, 
				  HObject * o, long slot)
{
    OBJECT *v;
    struct fieldblock *fb;
    int sig;

    fb = getFieldBlock(o, slot);
    if (fb == 0) {
	return 0;
    }

    sig = fieldsig(fb)[0];
    v = (fb->access & ACC_STATIC
	 ? (sig == SIGNATURE_DOUBLE || sig == SIGNATURE_LONG
	    ? twoword_static_address(fb)
	    : normal_static_address(fb))
	 : &obj_getoffset(o, fb->u.offset));

    switch (sig) {
      case SIGNATURE_BOOLEAN:
      case SIGNATURE_BYTE:
      case SIGNATURE_CHAR:
      case SIGNATURE_SHORT:
      case SIGNATURE_INT:
	return (long)*v;
      case SIGNATURE_VOID:
	return 0;

      default:
	SignalError(0, JAVAPKG "IllegalAccessError","invalid slot type");
	return 0;
    }
}

float
sun_tools_debug_Agent_getSlotFloat(Hsun_tools_debug_Agent * this, 
				    HObject * o, long slot)
{
    OBJECT *v;
    struct fieldblock *fb;
    int sig;
    Java8 t;

    fb = getFieldBlock(o, slot);
    if (fb == 0) {
	SignalError(0, JAVAPKG "IllegalAccessError","invalid slot type");
	return (float)0.0;
    }

    sig = fieldsig(fb)[0];
    v = (fb->access & ACC_STATIC
	 ? (sig == SIGNATURE_DOUBLE
	    ? twoword_static_address(fb)
	    : normal_static_address(fb))
	 : &obj_getoffset(o, fb->u.offset));

    if (sig == SIGNATURE_FLOAT) {
	return *(float *)v;
    } else if (sig == SIGNATURE_DOUBLE) {
	return (float)GET_DOUBLE(t, v);
    } else {
	SignalError(0, JAVAPKG "IllegalAccessError","invalid slot type");
	return 0;
    }
}

int64_t 
sun_tools_debug_Agent_getSlotLong(Hsun_tools_debug_Agent * this,
				   HObject * o, long slot)
{
    OBJECT *v;
    struct fieldblock *fb;
    int sig;
    Java8 t;

    fb = getFieldBlock(o, slot);
    if (fb == 0) {
	SignalError(0, JAVAPKG "IllegalAccessError","invalid slot type");
	return ll_zero_const;
    }

    if (fieldsig(fb)[0] != SIGNATURE_LONG) {
	SignalError(0, JAVAPKG "IllegalAccessError","invalid slot type");
	return ll_zero_const;
    }

    v = (fb->access & ACC_STATIC
	 ? twoword_static_address(fb)
	 : &obj_getoffset(o, fb->u.offset));
    return GET_INT64(t, v);
}

HObject *
sun_tools_debug_Agent_getSlotArray(Hsun_tools_debug_Agent *this, 
				    HObject *o, long slot)
{
    ClassClass *cb;
    struct fieldblock *fb, **slots;
    int nslots;

    if (o == 0) {
	SignalError(0, JAVAPKG "NullPointerException", 0);
	return NULL;
    }

    cb = get_classblock(o);
    if (makeslottable(cb) == NULL) {
	SignalError(0, JAVAPKG "OutOfMemoryError", 0);
	return NULL;
    }
    nslots = cbSlotTableSize(cb);
    slots = cbSlotTable(cb);
    if (slot >= nslots || slot < 0 || (fb = slots[slot]) == 0
	    || fieldsig(fb)[0] != SIGNATURE_ARRAY) {
	SignalError(0, JAVAPKG "IllegalArgumentException", "not an object");
	return NULL;
    }
    return (fb->access & ACC_STATIC
	    ? *(HObject **) normal_static_address(fb)
	    : (HObject *) obj_getoffset(o, fb->u.offset));
}

long
sun_tools_debug_Agent_getSlotArraySize(Hsun_tools_debug_Agent *this, 
					HObject *o, long slot)
{
    HObject *obj;
    int size;

    obj = sun_tools_debug_Agent_getSlotArray(this, o, slot);
    size = (obj == NULL) ? 0 : obj_length(obj);
    return size;
}

HArrayOfObject *
sun_tools_debug_Agent_getMethods(Hsun_tools_debug_Agent *this, HClass *c)
{
    int nmethods;
    struct methodblock *mb;
    HArrayOfObject *ret;
    HObject **retData;
    int i;
    Hsun_tools_debug_Field *hfield;
    struct fieldblock *fb;

    if (c == 0) {
	SignalError(0, JAVAPKG "NullPointerException", 0);
	return NULL;
    }

    nmethods = unhand(c)->methods_count;
    mb = cbMethods(unhand(c));
    ret = (HArrayOfObject *)ArrayAlloc(T_CLASS, nmethods);
    if (ret == 0) {
	SignalError(0, JAVAPKG "OutOfMemoryError", 0);
	return NULL;
    }
    retData = (HObject **)unhand(ret)->body;

    retData[nmethods] = (HObject *)FindClass(0, "sun/tools/debug/Field", TRUE);
    for (i = 0; i < nmethods; i++, mb++) {
	fb = &(mb->fb);
	hfield = (Hsun_tools_debug_Field*)
	    execute_java_constructor(0, "sun/tools/debug/Field", 0, "()");
	if (!hfield) {
	    dprintf("agent.c: hfield==0\n");
	    SignalError(0, JAVAPKG "NullPointerException", 0);
            return NULL;
	}

	unhand(hfield)->slot = i;
	unhand(hfield)->name = makeJavaString(fieldname(fb), 
					      strlen(fieldname(fb)));
	unhand(hfield)->signature = makeJavaString(fieldsig(fb), 
						   strlen(fieldsig(fb)));
	unhand(hfield)->access = fb->access;
	unhand(hfield)->clazz = (HObject *)cbHandle(fieldclass(fb));
	retData[i] = (HObject *)hfield;
    }
    return ret;
}

HArrayOfObject *
sun_tools_debug_Agent_getFields(Hsun_tools_debug_Agent *this, HClass *c)
{
    int nslots = 0;
    int i;
    HArrayOfObject *ret;
    HObject **retData;
    struct fieldblock **slots;
    Hsun_tools_debug_Field *hfield;

    if (c == 0) {
	SignalError(0, JAVAPKG "NullPointerException", 0);
	return NULL;
    }

    if (makeslottable(unhand(c)) == NULL) {
	SignalError(0, JAVAPKG "OutOfMemoryError", 0);
	return NULL;
    }
    nslots = cbSlotTableSize(unhand(c));
    slots = cbSlotTable(unhand(c));
    ret = (HArrayOfObject *)ArrayAlloc(T_CLASS, nslots);
    if (ret == 0) {
	SignalError(0, JAVAPKG "OutOfMemoryError", 0);
	return NULL;
    }
    retData = (HObject **)unhand(ret)->body;
    retData[nslots] = (HObject *)FindClass(0, "sun/tools/debug/Field", TRUE);

    for (i = 0; i < nslots; i++)
	if (slots[i]) {
	    hfield = (Hsun_tools_debug_Field*)
		execute_java_constructor(0, "sun/tools/debug/Field", 0, "()");
	    if (!hfield) {
		dprintf("agent.c: hfield==0\n");
		SignalError(0, JAVAPKG "NullPointerException", 0);
                return NULL;
	    }

	    unhand(hfield)->slot = i;
	    unhand(hfield)->name = 
		makeJavaString(fieldname(slots[i]), 
			       strlen(fieldname(slots[i])));
	    unhand(hfield)->signature = 
		makeJavaString(fieldsig(slots[i]), 
			       strlen(fieldsig(slots[i])));
	    unhand(hfield)->access = slots[i]->access;
	    unhand(hfield)->clazz = (HObject *)cbHandle(fieldclass(slots[i]));
	    retData[i] = (HObject *)hfield;
	}
    return ret;
}

HArrayOfObject *
sun_tools_debug_Agent_getClasses(Hsun_tools_debug_Agent *this)
{
    int i;
    ClassClass **pcb, *cb;
    HArrayOfObject *ret;
    HObject **retData;
    long nClasses;

    nClasses = get_nbinclasses();
    ret = (HArrayOfObject *)ArrayAlloc(T_CLASS, nClasses);
    if (ret == 0) {
	SignalError(0, JAVAPKG "OutOfMemoryError", 0);
	return NULL;
    }
    retData = (HObject **)unhand(ret)->body;
    retData[nClasses] = (HObject *)get_classClass();

    pcb = get_binclasses();
    for (i = 0; i < nClasses; i++, pcb++) {
	cb = *pcb;
	retData[i] = (HObject *)cb->HandleToSelf;
    }
    return ret;
}

long
sun_tools_debug_BreakpointHandler_setBreakpoint(
		       Hsun_tools_debug_BreakpointHandler *hbk,
		       long	   pc)
{
    unsigned char *code = (unsigned char *) pc;
    int	op_code;

    dprintf("setting breakpoint at pc=%d\n", pc);
    op_code = *code;
    if (op_code == opc_breakpoint) {
	SignalError(0, "sun/tools/debug/InvalidPCException", 0);
	return -1;
    }
    *code = opc_breakpoint;

    dprintf("breakpoint set, old opcode = %d\n", op_code);
    return op_code;
}


void
sun_tools_debug_BreakpointHandler_clrBreakpoint(
			       Hsun_tools_debug_BreakpointHandler *hbk,
			       long	   pc,
			       long	   old_opcode)
{
    unsigned char *code = (unsigned char *)pc;

    if (*code != opc_breakpoint) {
	SignalError(0, "sun/tools/debug/InvalidPCException", 0);
	return;
    }
    *code = old_opcode;
}

Hsun_tools_debug_LineNumber *
sun_tools_debug_Agent_lineno2pc(Hsun_tools_debug_Agent * this, 
				 HClass * c, long lineno)
{
    ClassClass *cls;
    struct methodblock *mb;
    struct lineno *ln;
    int nMethods, nLines;
    int i, j;
    Hsun_tools_debug_LineNumber *hline;

    if (!c) {
        dprintf("agent.c: HClass c==0\n");
        SignalError(0, JAVAPKG "NullPointerException", 0);
        return NULL;
    }

    hline = (Hsun_tools_debug_LineNumber*)
        execute_java_constructor(0, "sun/tools/debug/LineNumber", 0, "()");
    if (!hline) {
        dprintf("agent.c: hline==0\n");
        SignalError(0, JAVAPKG "NullPointerException", 0);
        return NULL;
    }
    unhand(hline)->startPC = unhand(hline)->endPC = -1;

    cls = (ClassClass *)unhand(c);
    mb = cls->methods;
    nMethods = (int)(cls->methods_count);
    while (nMethods > 0) {
	nLines = mb->line_number_table_length;
	ln = mb->line_number_table;
	while (nLines > 0) {
	    if (ln->line_number == (unsigned)lineno) {
		unhand(hline)->clazz = c;
		unhand(hline)->line_number = ln->line_number;
                if ((unsigned long)unhand(hline)->startPC > 
                    (unsigned long)(mb->code + ln->pc)) {
                    /* (unsigned long)-1 is always greater than any startPC */
                    unhand(hline)->startPC = (long)(mb->code + ln->pc);
                }
		if (nLines > 1) {
		    ln++;
                    if (unhand(hline)->endPC < (long)(mb->code + ln->pc - 1)) {
                        unhand(hline)->endPC = (long)(mb->code + ln->pc - 1);
                    }
		} else {
                    if (unhand(hline)->endPC < ((long)mb->code + mb->code_length)) {
                        unhand(hline)->endPC = (long)mb->code + mb->code_length;
                    }
		}
	    }
	    nLines--;
	    ln++;
	}
	nMethods--;
	mb++;
    }
    if (unhand(hline)->startPC == -1) {
        /* no line entry found */
        return NULL;
    }
    return hline;
}

long
sun_tools_debug_Agent_method2pc(Hsun_tools_debug_Agent * this, 
				 HClass * c, long method_slot)
{
    ClassClass *cls = (ClassClass *)unhand(c);
    struct methodblock *mb;

    if (method_slot < 0 || method_slot >= (long)(cls->methods_count)) {
	SignalError(0, JAVAPKG "IllegalAccessError","invalid slot index");
	return 0;
    }
    mb = cbMethods(cls) + method_slot;
    return (long)mb->code;
}

long
sun_tools_debug_Agent_pc2lineno(Hsun_tools_debug_Agent * this, 
				HClass * c, long pc)
{
    ClassClass *cls = (ClassClass *)unhand(c);
    struct methodblock *mb = cls->methods;
    struct lineno *ln;
    int nMethods, nLines;
    int i, j;
    Hsun_tools_debug_LineNumber *hline;

    nMethods = (int)(cls->methods_count);
    while (nMethods > 0) {
	if (pc >= (long)mb->code && pc < (long)(mb->code + mb->code_length)) {
	    nLines = mb->line_number_table_length;
	    ln = mb->line_number_table;
	    while (nLines > 0) {
	        if (pc >= (long)(mb->code + ln->pc)) {
		    if (nLines > 1) {
		        if (pc < (long)(mb->code +(ln+1)->pc)) {
			    return ln->line_number;
	    	        }
		    } else {
		        return ln->line_number;
		    }
	        }
	        nLines--;
	        ln++;
	    }
 	}
	nMethods--;
	mb++;
    }
    return -1;
}

stack_item *
RPI_getStackValue (HThread *t, long frameNum, long slot)
{
    JavaFrame *frame;

    if (unhand(t)->eetop == 0) {
	SignalError(0, JAVAPKG "IllegalAccessError","invalid slot");
	return NULL;
    }

    frame = RPI_get_stack_frame(t, frameNum);
    if (!frame) {
	dprintf("agent.c: jframe==0 framenum=%d\n", frameNum);
	SignalError(0, JAVAPKG "NullPointerException", 0);
	return NULL;
    }

    if (slot == -1) {
	return NULL;
    }
    return frame->vars + slot;
}

HObject *
sun_tools_debug_Agent_getStackObject(Hsun_tools_debug_Agent *this, 
                                     HThread *h, long frameNum, long slot)
{
    stack_item *si = RPI_getStackValue(h, frameNum, slot);
    return (si == NULL) ? NULL : (HObject *)si->o;
}

long
sun_tools_debug_Agent_getStackBoolean(Hsun_tools_debug_Agent *this, 
                                      HThread *h, long frameNum, long slot)
{
    stack_item *si = RPI_getStackValue(h, frameNum, slot);
    return (si == NULL) ? 0L : (long)si->i;
}

long
sun_tools_debug_Agent_getStackInt(Hsun_tools_debug_Agent *this, 
                                  HThread *h, long frameNum, long slot)
{
    stack_item *si = RPI_getStackValue(h, frameNum, slot);
    return (si == NULL) ? 0L : (long)si->i;
}

int64_t 
sun_tools_debug_Agent_getStackLong(Hsun_tools_debug_Agent *this, 
                                   HThread *h, long frameNum, long slot)
{
    stack_item *si = RPI_getStackValue(h, frameNum, slot);
    Java8 t;

    return (si == NULL) ? ll_zero_const : GET_INT64(t, si);
}

float 
sun_tools_debug_Agent_getStackFloat(Hsun_tools_debug_Agent *this, 
                                    HThread *h, long frameNum, long slot)
{
    stack_item *si = RPI_getStackValue(h, frameNum, slot);
    Java8 t;

    return (si == NULL) ? 0.0 : (float)GET_DOUBLE(t, si);
}

HString *
sun_tools_debug_Agent_exceptionStackTrace(Hsun_tools_debug_Agent *this,
					   Hjava_lang_Throwable *exc) {
    int         n;
    HString *hstr, *hret;
    char *p;
    char buf[128];
    HString *hstring;
    char outbuf[2048];
    struct HArrayOfInt *backtrace = (HArrayOfInt *)(unhand(exc)->backtrace);
    int32_t *real_data = unhand(backtrace)->body;
    unsigned char **data = (unsigned char **)(real_data);
    unsigned char **enddata = 
	(unsigned char **)(real_data + obj_length(backtrace));

    p = classname2string(classname(obj_classblock(exc)), buf, sizeof(buf));
    strcpy(outbuf, p);
    hstr = unhand(exc)->detailMessage;
    if (hstr) {
	strcat(outbuf, " ");
	strcat(outbuf, javaString2CString(hstr, buf, sizeof(buf)));
        strcat(outbuf, "\n");
    }
    for (; data < enddata; data++) { 
	if (*data != 0) {
	    char        where[100];
	    pc2string(*data, 0, where, where + sizeof where);
            if ((int)(strlen(outbuf) + strlen(where) + 5) >= 2048) {
                break;	/* out of buffer space, return what's here */
            }
	    strcat(outbuf, "\tat ");
	    strcat(outbuf, where);
            strcat(outbuf, "\n");
	}
    }
    return makeJavaString(outbuf, strlen(outbuf));
}

extern void set_single_stepping(bool_t);

void sun_tools_debug_Agent_setSingleStep(Hsun_tools_debug_Agent *this,
					  Hjava_lang_Thread *thread,
					  /*boolean*/ long step)
{
    unhand(thread)->single_step = step;
    set_single_stepping((bool_t)step);
}

HString *
sun_tools_debug_Agent_getClassSourceName(Hsun_tools_debug_Agent *this, HClass *c) {
    char *name = unhand(c)->source_name;
    if (name == 0) {
        name = "";
    }
    return makeJavaString(name, strlen(name));
}

void 
sun_tools_debug_Agent_setSlotBoolean(Hsun_tools_debug_Agent *this,
                                     HObject *o, long slot, long value)
{
    long *v;
    struct fieldblock *fb;
    int sig;

    fb = getFieldBlock(o, slot);
    if (fb == 0 || fieldsig(fb)[0] != SIGNATURE_BOOLEAN) {
	SignalError(0, JAVAPKG "IllegalAccessError","invalid slot type");
	return;
    }
    v = (long*)(fb->access & ACC_STATIC
	 ? normal_static_address(fb)
	 : &obj_getoffset(o, fb->u.offset));
    *v = value;
}

void 
sun_tools_debug_Agent_setSlotInt(Hsun_tools_debug_Agent *this,
                                     HObject *o, long slot, long value)
{
    long *v;
    struct fieldblock *fb;
    int sig;

    fb = getFieldBlock(o, slot);
    if (fb == 0) {
	SignalError(0, JAVAPKG "IllegalAccessError","invalid slot");
	return;
    }
    sig =  fieldsig(fb)[0];
    if (sig != SIGNATURE_BYTE && sig != SIGNATURE_SHORT && 
        sig != SIGNATURE_INT && sig != SIGNATURE_CHAR) {
	SignalError(0, JAVAPKG "IllegalAccessError","invalid slot type");
	return;
    }
    v = (long*)(fb->access & ACC_STATIC
	 ? normal_static_address(fb)
	 : &obj_getoffset(o, fb->u.offset));
    *v = value;
}

void 
sun_tools_debug_Agent_setSlotLong(Hsun_tools_debug_Agent *this,
                                     HObject *o, long slot, int64_t value)
{
    OBJECT *v;
    struct fieldblock *fb;
    int sig;
    Java8 t;

    fb = getFieldBlock(o, slot);
    if (fb == 0 || fieldsig(fb)[0] != SIGNATURE_LONG) {
	SignalError(0, JAVAPKG "IllegalAccessError","invalid slot type");
	return;
    }

    v = (fb->access & ACC_STATIC
	 ? twoword_static_address(fb)
	 : &obj_getoffset(o, fb->u.offset));
    SET_INT64(t, v, value);
}

void 
sun_tools_debug_Agent_setSlotDouble(Hsun_tools_debug_Agent *this,
                                    HObject *o, long slot, double value)
{
    OBJECT *v;
    struct fieldblock *fb;
    int sig;
    Java8 t;

    fb = getFieldBlock(o, slot);
    if (fb == 0) {
	SignalError(0, JAVAPKG "IllegalAccessError","invalid slot type");
	return;
    }

    sig = fieldsig(fb)[0];
    v = (fb->access & ACC_STATIC
	 ? (sig == SIGNATURE_DOUBLE
	    ? twoword_static_address(fb)
	    : normal_static_address(fb))
	 : &obj_getoffset(o, fb->u.offset));

    if (sig == SIGNATURE_FLOAT) {
	*(float *)v = (float)value;
    } else if (sig == SIGNATURE_DOUBLE) {
        SET_DOUBLE(t, v, value);
    } else {
	SignalError(0, JAVAPKG "IllegalAccessError","invalid slot type");
	return;
    }
}

void 
sun_tools_debug_Agent_setStackBoolean(Hsun_tools_debug_Agent *this,
                                      HThread *h, long frameNum, long slot,
                                      long value)
{
    stack_item *si = RPI_getStackValue(h, frameNum, slot);
    if (si != NULL) {
        si->i = (int)value;
    }
}

void 
sun_tools_debug_Agent_setStackInt(Hsun_tools_debug_Agent *this, HThread *h, 
                                  long frameNum, long slot, long value)
{
    stack_item *si = RPI_getStackValue(h, frameNum, slot);
    if (si != NULL) {
        si->i = (int)value;
    }
}

void
sun_tools_debug_Agent_setStackLong(Hsun_tools_debug_Agent *this, 
                                   HThread *h, long frameNum, long slot,
                                   int64_t value)
{
    stack_item *si = RPI_getStackValue(h, frameNum, slot);
    Java8 t;
    if (si != NULL) {
        SET_INT64(t, si, value);
    }
}

void
sun_tools_debug_Agent_setStackDouble(Hsun_tools_debug_Agent *this, 
                                     HThread *h, long frameNum, long slot,
                                     double value)
{
    stack_item *si = RPI_getStackValue(h, frameNum, slot);
    Java8 t;
    if (si != NULL) {
        SET_DOUBLE(t, si, value);
    }
}

/*  WARNING: I've never really compiled this native method in here with
    proper prototypes and type declarations.  This file doesn't really 
    compile with the current headers :-/... and I'm just dropping this here
    as a place holder.  A nearly identical function is sitting in 
    ./appletStubs.c (call function in ./system.c)... so I'm pretty sure 
    this is right... but the headers needs to be set up properly.
    (jar 7-22-96)
*/
void 
sun_tools_debug_Agent_setSystemIO(struct Hsun_tools_debug_Agent *self,
                                  struct Hjava_io_InputStream *in,
                                  struct Hjava_io_PrintStream *out,
                                  struct Hjava_io_PrintStream *err) 
{
    /* Forward call to native method in System.java */
    java_lang_System_setSystemIO(0, in, out, err);
}
#endif /* BREAKPTS */
