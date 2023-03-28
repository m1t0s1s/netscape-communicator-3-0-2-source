/*
 * @(#)decode.c	1.57 95/11/16  
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
 *      decode an java object file
 */

#include <sys/types.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "typecodes.h"
#include "opcodes.h"
#include "oobj.h"
#include "decode.h"
#include "tree.h"
#include "typecodes.h"
#include "signature.h"
#include <stddef.h>
#include "interpreter.h"
#include "utf.h"
#include <ctype.h>
#include "byteorder.h"
#include "sys_api.h"

bool_t VerifyClass(ClassClass *cb);

#undef putchar
#define putchar(ch)  printf("%c", ch)

/* initialize opnames[256]; */
#include "opcodes.init"

extern char *progname;
static char *currentclassname;
int decode_verbose;
int retcode;

static void PrintConstant(ClassClass *cb, int index);
static void PrintUtfReadable(FILE *fd, char *utfstring);
static void PrintInitialValue(struct fieldblock *fb);
static char *PrintableClassname(char *classname);

char *
SprintReverseType(char *s, char *fname, char *out)
{
    char buf[1000];
    register char *tn = "?";
    if (fname == 0)
	fname = "";
    switch (*s++) {
      default:
	switch (s[-1]) {
	  case SIGNATURE_BYTE:
	    tn = "byte";
	    break;
	  case SIGNATURE_SHORT:
	    tn = "short";
	    break;
	  case SIGNATURE_CHAR:
	    tn = "char";
	    break;
	  case SIGNATURE_ENUM:
	    tn = "enum";
	    break;
	  case SIGNATURE_FLOAT:
	    tn = "float";
	    break;
	  case SIGNATURE_DOUBLE:
	    tn = "double";
	    break;
	  case SIGNATURE_INT:
	    tn = "int";
	    break;
	  case SIGNATURE_LONG:
	    tn = "long";
	    break;
	  case SIGNATURE_CLASS:
	    tn = buf;
	    if (PrintAsC) {
		register char *sp = "struct Class";
		while (*sp)
		    *tn++ = *sp++;
	    }
	    for (; *s && *s != SIGNATURE_ENDCLASS;) {
		char c = *s++;
		*tn++ = (c == '/') ? '.' : c;
	    }
	    *tn = 0;
	    if (*s == SIGNATURE_ENDCLASS)
		s++;
	    tn = buf;
	    break;
	  case SIGNATURE_VOID:
	    tn = "void";
	    break;
	  case SIGNATURE_BOOLEAN:
	    tn = "boolean";
	    break;
	}
	sprintf(out, *fname ? "%s %s" : "%s", tn, fname);
	break;
      case 0:
	strcpy(out, fname);
	s--;
	break;
      case SIGNATURE_ARRAY:
	if (*s == SIGNATURE_CHAR) {
	    s++;
	    if (PrintAsC)
		sprintf(out, *fname ? "char *%s" : "char *", fname);
	    else
		sprintf(out, *fname ? "char %s[]" : "char []", fname);
	} else {
	    sprintf(buf, "%s[]", fname);
	    s = SprintReverseType(s, buf, out);
	}
	break;
      case SIGNATURE_FUNC:
	{
	    int constructor = strcmp(fname, "<init>") == 0;
	    sprintf(buf, "%s%s(",
		    currentclassname && PrintAsC ? 
		        PrintableClassname(currentclassname) : "",
		    constructor ? PrintAsC ? "Initializor" : 
		        PrintableClassname(currentclassname) : fname);
	    fname = buf + strlen(buf);
	    if (PrintAsC && currentclassname) {
		sprintf(fname, "Class%s%s", 
			PrintableClassname(currentclassname),
			*s != SIGNATURE_ENDFUNC ? "," : "");
		fname += strlen(fname);
	    }
	    while (*s != SIGNATURE_ENDFUNC && *s) {
		s = SprintReverseType(s, (char *) 0, fname);
		fname += strlen(fname);
		if (*s != SIGNATURE_ENDFUNC)
		    *fname++ = ',';
	    }
	    *fname++ = ')';
	    *fname++ = 0;
	    s = SprintReverseType(constructor ? "" : *s ? s + 1 : s, buf, out);
	}
	break;
    }
    return s;
}

static void
PrintType(char *signature, char *fieldname)
{
    char buf[1000];

    (void) SprintReverseType(signature, fieldname, buf);
    printf("%s", buf);
}

void
PrintEscapedString(unsigned short *s, long len)
{
    register c;
    while (--len >= 0) {
	if ((c = *s++) >= 040 && c < 0177)
	    printf("%c", c);
	else
	    switch (c) {
	      case '\n':
		printf("\\n");
		break;
	      case '\t':
		printf("\\t");
		break;
	      case '\r':
		printf("\\r");
		break;
	      case '\b':
		printf("\\b");
		break;
	      case '\f':
		printf("\\f");
		break;
	      default:
		printf("\\<%X>", c);
		break;
	    }

    }
}

void
PrintCodeSequence(struct methodblock *mb)
{
    ClassClass *cb = fieldclass(&mb->fb);
    unsigned char *initial_pc = mb->code;
    unsigned char *max_pc = initial_pc + mb->code_length;
    unsigned char *pc;

    for (pc = initial_pc; pc < max_pc; ) {
	int opcode = pc[0];
	int base_offset = pc - initial_pc;
	if (opcode == opc_wide) 
	    opcode = 256 + pc[1];
	
	printf("%4d %s", pc - initial_pc, opnames[opcode & 0xFF]);
	switch (opcode) {
	    case opc_aload: case opc_astore:
	    case opc_fload: case opc_fstore:
	    case opc_iload: case opc_istore:
	    case opc_lload: case opc_lstore:
	    case opc_dload: case opc_dstore:
	    case opc_ret:
		printf(" %d", pc[1]);
		pc += 2;
		break;
	    
	    case opc_aload + 256: case opc_astore + 256:
	    case opc_fload + 256: case opc_fstore + 256:
	    case opc_iload + 256: case opc_istore + 256:
	    case opc_lload + 256: case opc_lstore + 256:
	    case opc_dload + 256: case opc_dstore + 256:
	    case opc_ret + 256:
		printf("_w %d", (pc[2] << 8) | pc[3]);
		pc += 4;
		break;

	    case opc_iinc:
		printf(" %d %d", pc[1], ((char *)pc)[2]);
		pc += 3;
		break;

	    case opc_iinc + 256:
		printf("_w %d %d", ((pc[2] << 8) + pc[3]), 
		       (short) ((pc[4] << 8) | pc[5]));
		pc += 6;
		break;


	    case opc_tableswitch:{
		long *ltbl = (long *) REL_ALIGN(initial_pc, pc-initial_pc + 1); /* UCALIGN((int)pc + 1); */
		int default_skip = ntohl(ltbl[0]); /* default skip pamount */
		int low = ntohl(ltbl[1]);
		int high = ntohl(ltbl[2]);
		int count = high - low;
		int i;
	     
		printf(" %d to %d: default=%d", 
		       low, high, default_skip + base_offset);
		for (i = 0, ltbl += 3; i <= count; i++, ltbl++) 
		    printf("\n\t%5d: %d", i + low, 
			   ntohl(ltbl[0]) + base_offset);
		pc = (unsigned char *) ltbl;
		break;
	    }

	    case opc_lookupswitch:{
		register long *ltbl = (long *) REL_ALIGN(initial_pc, pc-initial_pc + 1); /* UCALIGN((int)pc + 1); */
		int skip = ntohl(ltbl[0]); /* default skip amount */
		int npairs = ntohl(ltbl[1]);
		int i;
		printf(" %d: default=%d", npairs, skip + base_offset);
		for (i = 0, ltbl += 2; i < npairs; i++, ltbl += 2) 
		    printf("\n\t%5d: %d", ntohl(ltbl[0]),
			   ntohl(ltbl[1]) + base_offset);
		pc = (unsigned char *) ltbl;
		break;
	    }

	    case opc_newarray:
		switch (pc[1]) {
		    case T_INT:    printf(" int");    break;
		    case T_LONG:   printf(" long");   break;
		    case T_FLOAT:  printf(" float");  break;
		    case T_DOUBLE: printf(" double"); break;
		    case T_CHAR:   printf(" char");   break;
		    case T_SHORT:  printf(" short");  break;
		    case T_BYTE:   printf(" byte");   break;
		    default:       printf(" BOGUS TYPE"); break;
		}
		pc += 2;
		break;
		

	    case opc_anewarray: {
		int index =  (pc[1] << 8) | pc[2];
		printf(" class #%d ", index);
		PrintConstant(cb, index);
		pc += 3;
		break;
	    }

	      
	    case opc_sipush:
		printf(" %d", (short) ((pc[1] << 8) | pc[2]));
		pc += 3;
		break;

	    case opc_bipush:
		printf(" %d", (char) pc[1]);
		pc += 2;
		break;

	    case opc_ldc: {
		int index = pc[1];
		printf(" #%d ", index);
		PrintConstant(cb, index);
		pc += 2;
		break;
	    }

	    case opc_ldc_w: case opc_ldc2_w:
	    case opc_instanceof: case opc_checkcast:
	    case opc_new:
	    case opc_putstatic: case opc_getstatic:
	    case opc_putfield: case opc_getfield:
	    case opc_invokevirtual:
	    case opc_invokenonvirtual:
	    case opc_invokestatic: {
		int index = (pc[1] << 8) | pc[2];
		printf(" #%d ", index);
		PrintConstant(cb, index);
		pc += 3;
		break;
	    }

	    case opc_invokeinterface: {
		int index = (pc[1] << 8) | pc[2];
		printf(" (args %d) #%d ", pc[3], index);
		PrintConstant(cb, index);
		pc += 5;
		break;
	    }

	    case opc_multianewarray: {
		int index = (pc[1] << 8) | pc[2];
		printf(" #%d dim #%d ", index, pc[3]);
		PrintConstant(cb, index);
		pc += 4;
		break;
	    }

	    case opc_jsr: case opc_goto:
	    case opc_ifeq: case opc_ifge: case opc_ifgt:
	    case opc_ifle: case opc_iflt: case opc_ifne:
	    case opc_if_icmpeq: case opc_if_icmpne: case opc_if_icmpge:
	    case opc_if_icmpgt: case opc_if_icmple: case opc_if_icmplt:
	    case opc_if_acmpeq: case opc_if_acmpne:
	    case opc_ifnull: case opc_ifnonnull:
		printf(" %d", base_offset + (short) ((pc[1] << 8) | pc[2]));
		pc += 3;
		break;

	    case opc_jsr_w:
	    case opc_goto_w:
		printf(" %d", base_offset +  
		 (long) ((pc[1] << 24) | (pc[2] << 16) | (pc[3] << 8) + pc[4]));
		pc += 5;
		break;
		
	    default:
		pc++;
		break;
	}
	putchar('\n');
    }

    if (mb->exception_table_length > 0) { 
	struct CatchFrame *cf = mb->exception_table;
	int cnt = mb->exception_table_length;
	ClassClass *cb = fieldclass(&mb->fb);
	printf("Exception table:\n   from   to  target type\n");
	for (; --cnt >= 0; cf += 1) {
	    short catchType = cf->catchType;
	    printf("  %4d  %4d  %4d   ", 
		   cf->start_pc, cf->end_pc, cf->handler_pc);
	    if (cf->catchType == 0) 
		printf("any\n");
	    else {
		PrintConstant(cb, catchType);
		printf("\n");
	    }
	}
    }
}

void
PrintLocalVariableTable(struct methodblock *mb)
{
    ClassClass *cb = fieldclass(&mb->fb);
    struct localvar *lv = mb->localvar_table;
    union cp_item_type *constant_pool = cb->constantpool;
    int i;

    for (i = 0; i < (int)mb->localvar_table_length; i++, lv++) {
	printf("   ");
	PrintType(constant_pool[lv->sigoff].cp, constant_pool[lv->nameoff].cp);
	printf("  pc=%d, length=%d, slot=%d\n", lv->pc0, lv->length, lv->slot);
    }
}

void
PrintLineNumberTable(struct methodblock *mb)
{
    struct lineno *ln = mb->line_number_table;
    int i;

    for (i = 0; i < (int)mb->line_number_table_length; i++, ln++) {
	printf("   line %d: %d\n", ln->line_number, ln->pc);
    }
}

void PrintConstant(ClassClass *cb, int index)
{
    union cp_item_type *constant_pool = cb->constantpool;
    unsigned char *type_data = constant_pool[CONSTANT_POOL_TYPE_TABLE_INDEX].p;
    Java8 t;

    switch(type_data[index] & CONSTANT_POOL_ENTRY_TYPEMASK) {
        case CONSTANT_Utf8:
	    printf("<\"%s\">", constant_pool[index].cp);
	    break;
	
        case CONSTANT_Integer:
	    printf("<Integer %d>", constant_pool[index].i);
	    break;

        case CONSTANT_Float:
	    printf("<Real %f>", *(float *)&constant_pool[index]);
	    break;

        case CONSTANT_Long: {
	    char buf[32];
	    int64_t value = GET_INT64(t, &constant_pool[index]);
	    printf("<Long %s>", int642CString(value, buf, sizeof(buf)));
	    break;
	}

        case CONSTANT_Double:
	    printf("<Double %f>", GET_DOUBLE(t,&constant_pool[index]));
	    break;

        case CONSTANT_Class:
	    printf("<Class %s>", PrintableClassname(
		GetClassConstantClassName(constant_pool, index)));
	    break;
	
        case CONSTANT_String: {
	    unsigned key = constant_pool[index].i;
	    printf("<String \"");
	    PrintUtfReadable(stdout, constant_pool[key].cp);
	    printf("\">");
	    break;
	}

	case CONSTANT_Methodref:
        case CONSTANT_Fieldref:  
	case CONSTANT_InterfaceMethodref: {
	    unsigned type = type_data[index];
	    unsigned key = constant_pool[index].i; 
	    unsigned classkey = key >> 16; 
	    unsigned nametypekey = key & 0xFFFF;
	    unsigned nametypeindex = constant_pool[nametypekey].i;
	    unsigned fieldnameindex = nametypeindex >> 16;
	    unsigned fieldtypeindex = nametypeindex & 0xFFFF;
	    char *type_name =  type == CONSTANT_Methodref ? "Method" 
	                    : type == CONSTANT_Fieldref ?  "Field"
			    : "InterfaceMethod";
	    
	    printf("<%s %s.%s%s%s>",
		   type_name,
		   PrintableClassname(GetClassConstantClassName(
			constant_pool, classkey)),
		   constant_pool[fieldnameindex].cp, 
		   type == CONSTANT_Fieldref ? " " : "",
		   constant_pool[fieldtypeindex].cp);
	    break;
	}
	    
        case CONSTANT_NameAndType: {
	    unsigned key = constant_pool[index].i; 
	    unsigned fieldnameindex = key >> 16;
	    unsigned fieldtypeindex = key & 0xFFFF;
	    printf("<NameAndType %s %s>",
		   constant_pool[fieldnameindex].cp, 
		   constant_pool[fieldtypeindex].cp);
	    break;

	default:
	    printf("<Unknown %d (0x%x)>", type_data[index], type_data[index]);
	    break;
	}
    }
}

void
PrintClassSequence(register ClassClass * cb)
{
    register struct fieldblock *fb;
    register struct methodblock *mb;
    register nslots;
    currentclassname = classname(cb);
    printf("%s%s%s%s%s%s %s ", 
	   (cb->access & ACC_PROTECTED)    ? "protected " : "",
	   (cb->access & ACC_PUBLIC)       ? "public " : "",
	   (cb->access & ACC_PRIVATE)      ? "private " : "",
	   (cb->access & ACC_FINAL)        ? "final ": "",
	   (cb->access & ACC_ABSTRACT)     ? "abstract ": "",
	   (cb->access & ACC_INTERFACE)    ? "interface" : "class",
	   PrintableClassname(currentclassname));
    if (cb->super_name) {
        printf("extends %s ", PrintableClassname(classsupername(cb)));
    }
    if (cb->implements_count > 0) {
	short *p = cb->implements;
	unsigned i;
	unsigned count = cb->implements_count;
	for (i = 0; i < count; i++) {
	    int class_index = p[i];
	    int string_index = cb->constantpool[class_index].i;
	    char *class_name = cb->constantpool[string_index].cp;
	    printf("%s %s", i == 0 ? "implements" : ",", class_name);
	}
	printf(" ");
    }
    printf("{\n");
    for (nslots = cb->fields_count, fb = cbFields(cb); --nslots >= 0; fb++) {
	if ((fb->access & ACC_PRIVATE) == 0 || PrintPrivate) {
	    printf("    %s%s%s%s%s%s%s",
		   (fb->access & ACC_PUBLIC)    ? "public " : "",
		   (fb->access & ACC_PROTECTED)    ? "protected " : "",
		   (fb->access & ACC_PRIVATE)   ? "private " : "",
		   (fb->access & ACC_STATIC)    ? "static " : "",
		   (fb->access & ACC_FINAL)     ? "final " : "",
		   (fb->access & ACC_TRANSIENT) ? "transient " : "",
		   (fb->access & ACC_THREADSAFE)  ? "threadsafe " : "");
	    PrintType(fieldsig(fb), fieldname(fb));
	    if (fb->access & ACC_VALKNOWN)
		PrintInitialValue(fb);
	    printf(";\n");
	}
    }
    for (nslots = cb->methods_count, mb = cbMethods(cb); --nslots >= 0; mb++) {
	fb = &mb->fb;
	if ((fb->access & ACC_PRIVATE) == 0 || PrintPrivate) {
	    printf("    %s%s%s%s%s%s%s%s", 
		   (fb->access & ACC_PROTECTED)    ? "protected " : "",
		   (fb->access & ACC_PUBLIC)       ? "public " : "",
		   (fb->access & ACC_PRIVATE)      ? "private " : "",
		   (fb->access & ACC_FINAL)        ? "final ": "",
		   (fb->access & ACC_NATIVE)       ? "native ": "",
		   (fb->access & ACC_ABSTRACT)     ? "abstract ": "",
		   (fb->access & ACC_STATIC)       ? "static " : "",
		   (fb->access & ACC_SYNCHRONIZED) ? "synchronized ": ""
		   );
	    PrintType(fieldsig(fb), fieldname(fb));
	    printf(";\n");
	    if (decode_verbose)
	       printf("\t/* Stack=%d, Locals=%d, Args_size=%d */\n",
		      mb->maxstack, mb->nlocals, mb->args_size);
	}
    }
    if (PrintConstantPool) {
	printf("\n");
	for (nslots = 1; nslots < cb->constantpool_count; nslots++) {
	    printf("Constant %d = ", nslots);
	    PrintConstant(cb, nslots);
	    printf("\n");
	}
    }
    if (PrintCode) {
	printf("\n");
	for (nslots = cb->methods_count, mb = cbMethods(cb);
		--nslots >= 0; mb++) {
	    fb = &mb->fb;
	    if (mb->code_length > 0) {
		printf("Method ");
		PrintType(fieldsig(fb), fieldname(fb));
		printf("\n");
		PrintCodeSequence(mb);
		printf("\n");
	    } 
	}
    }
    if (PrintLineTable) {
	printf("\n");
	for (nslots = cb->methods_count, mb = cbMethods(cb);
		--nslots >= 0; mb++) {
	    fb = &mb->fb;
	    if (mb->line_number_table_length > 0) {
		printf("Line numbers for method ");
		PrintType(fieldsig(fb), fieldname(fb));
		printf("\n");
		PrintLineNumberTable(mb);
		printf("\n");
	    } 
	}
    }
    if (PrintLocalTable) {
	printf("\n");
	for (nslots = cb->methods_count, mb = cbMethods(cb);
		--nslots >= 0; mb++) {
	    fb = &mb->fb;
	    if (mb->localvar_table_length > 0) {
		printf("Local variables for method ");
		PrintType(fieldsig(fb), fieldname(fb));
		printf("\n");
		PrintLocalVariableTable(mb);
		printf("\n");
	    } 
	}
    }
    printf("}\n");
}

static void 
PrintInitialValue(struct fieldblock *fb)
{
    Java8 t;
    switch (fb->signature[0]) {
      case SIGNATURE_FLOAT:
	printf(" = %g", *(float *)normal_static_address(fb));
	break;

      case SIGNATURE_DOUBLE:
	printf(" = %f", GET_DOUBLE(t, twoword_static_address(fb)));
	break;
	   
      case SIGNATURE_LONG: {
	char buf[32];
	printf(" = %s", int642CString(GET_INT64(t, twoword_static_address(fb)), 
				      buf, sizeof(buf)));
	break;
      }
	
      case SIGNATURE_CLASS:
	/* ResolveClassStringConstant() below causes the UTF string to be
	   stored into normal_static_address   */
	printf(" = \"");
	PrintUtfReadable(stdout, *(char **)normal_static_address(fb));
	printf("\"");
	break;

      default:
	printf(" = %d", *(int *)normal_static_address(fb));
	break;
    }
}


void
PrintClassSequenceAsC(register ClassClass * cb)
{
    register struct fieldblock *fb;
    register struct methodblock *mb;
    register nslots;

    currentclassname = classname(cb);
    printf("\n\ntypedef struct Class%s {\n    struct ClassObject header;\n",
	   PrintableClassname(currentclassname));
    for (nslots = cb->fields_count, fb = cbFields(cb); --nslots >= 0; fb++)
	if ((fb->access & ACC_PRIVATE) == 0 || PrintPrivate) {
	    printf("    ");
	    PrintType(fieldsig(fb), fieldname(fb));
	    printf(";\n");
	}
    printf("} Class%s;\n", PrintableClassname(currentclassname));
    for (nslots = cb->methods_count, mb = cbMethods(cb); --nslots >= 0; mb++) {
	fb = &mb->fb;
	if (mb->code_length == 0) {
	    printf("    extern ");
	    PrintType(fieldsig(fb), fieldname(fb));
	    printf(";\n");
	}
    }
}


void
DecodeFile(fn)
    register char *fn;
{
    ClassClass *cb;
    SkipSourceChecks = TRUE;
    cb = FindClass(0, fn, verifyclasses ? TRUE : FALSE);
    if (cb == NULL) {
	retcode = 1;
	return;
    }
    if (verifyclasses != VERIFY_NONE) {
	char *class_name = PrintableClassname(classname(cb));
	bool_t result;
	if (decode_verbose) {
	    printf("%s %s ", 
		   (cb->access & ACC_INTERFACE) ? "interface" : "class",
		   class_name);
	    if (cb->super_name)
		printf("extends %s ", classsupername(cb));
	    printf("\n");
	}
	result = VerifyClass(cb);
	printf("Class %s %s\n", class_name, result ? "succeeds" : "fails");
	retcode = 0;
	return;
    }
    if (PrintAsC) {
	char define[1000];
	register char *s, *d;
	register c;
	if (cb->source_name)
	    s = classsrcname(cb);
	else s = "SourceFileInfoMissing";
	printf("/* Header generated from %s */\n\n", s);
	for (d = define; (c = *s++);)
	    if (('a' <= c && c <= 'z')
		    || ('A' <= c && c <= 'Z')
		    || ('0' <= c && c <= '9'))
		*d++ = c;
	    else
		*d++ = '_';
	*d++ = 0;
	printf("#ifndef _FN_%s\n#define _FN_%s\n\n", define, define);
    } else if (cb->source_name)
	printf("Compiled from %s\n", classsrcname(cb));
    if (PrintAsC) {
	PrintClassSequenceAsC(cb);
	printf("#endif\n");
    } else {
	PrintClassSequence(cb);
    }
}


static char *PrintableClassname(char *class_name)
{
    char *p;
    static char class_copy[257];
    strncpy(class_copy, class_name, 256);

    /* Convert all slashes in the classname to periods */
    for (p = class_copy; ((p = strchr(p, '/')) != 0); *p++ = '.');
    return class_copy;
}

static void PrintUtfReadable(FILE *fd, char *utfstring) 
{

    if (is_simple_utf(utfstring)) {
	fprintf(fd, "%s", utfstring);
    } else {
	while (*utfstring != 0) {
	    unicode ch = next_utf2unicode(&utfstring);
	    if ((ch <= 0x7f) && isprint(ch)) {
		putc(ch, fd);
	    } else {
		switch (ch) {
		    case '\n': fprintf(fd, "\\n"); break;
		    case '\t': fprintf(fd, "\\t"); break;
		    case '\r': fprintf(fd, "\\r"); break;
		    case '\b': fprintf(fd, "\\b"); break;
		    case '\f': fprintf(fd, "\\f"); break;
		    default:   fprintf(fd, "\\%o", ch); break;
		  }
	    }
	}
    }
}

/* ResolveClassStringConstant is called by the classloader to resolve the
 * constant pool entry, before assigning a value to a final [i.e. constant]
 * String.
 *
 * For the decoder, we just need a pointer to the UTF string. 
 */

bool_t
ResolveClassStringConstant(ClassClass *cb, cp_index_type index, struct execenv *ee)
{
    union cp_item_type *constant_pool = cbConstantPool(cb);
    unsigned char *type_table = constant_pool[CONSTANT_POOL_TYPE_TABLE_INDEX].p;
    if (!CONSTANT_POOL_TYPE_TABLE_IS_RESOLVED(type_table, index)) {
	unsigned value_index = constant_pool[index].i;
	char *value = constant_pool[value_index].cp;
	constant_pool[index].cp = value;
	CONSTANT_POOL_TYPE_TABLE_SET_RESOLVED(type_table, index);	
    }
    return TRUE;
}

void
DumpThreads() { }

HObject *
AllocHandle(struct methodtable *mptr, ClassObject *p)
{
    JHandle *h = sysMalloc(sizeof(JHandle));

    h->methods = mptr;
    h->obj = (ClassObject *) p;
    return (HObject *) h;
}

long
CallInterpreted(register struct methodblock * mb, void *obj,...)
{
    return 0;
}

