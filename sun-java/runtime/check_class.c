/*
 * @(#)check_class.c	1.26 95/11/29  
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
 *      code for verifying the date in a ClassClass structure for internal
 *      consistency.
 */

#include <ctype.h>

#include "oobj.h"
#include "interpreter.h"
#include "bool.h"
#include "utf.h"
#include "tree.h"
#include "sys_api.h"

extern bool_t verify_class_codes(ClassClass *cb);

static bool_t verify_constant_pool(ClassClass *cb);

static bool_t is_legal_fieldname(ClassClass *cb, char *name, int type);
static bool_t is_legal_method_signature(ClassClass *cb, char *name, char *signature);
static bool_t is_legal_field_signature(ClassClass *cb, char *name, char *signature);

static char *skip_over_fieldname(char *name, bool_t slash_okay);
static char *skip_over_field_signature(char *name, bool_t void_okay);

static void CCerror (ClassClass *cb, char *format, ...);


/* Argument for is_legal_fieldname */
enum { LegalClass, LegalField, LegalMethod };




bool_t
VerifyClass(ClassClass *cb)
{
    bool_t result = TRUE;
    struct methodblock *mb;
    struct fieldblock *fb;
    int32_t i;
    if (CCIs(cb, Verified)) 
	return TRUE;
    if (!verify_constant_pool(cb)) 
        return FALSE;
    /* Make sure all the method names and signatures are okay */
    for (i = (int32_t) cb->methods_count, mb = cb->methods; --i >= 0; mb++) {
	char *name = mb->fb.name;
	char *signature = mb->fb.signature;
	if (! (is_legal_fieldname(cb, name, LegalMethod)  &&
	       is_legal_method_signature(cb, name, signature)))
	    result = FALSE;
    }
    /* Make sure all the field names and signatures are okay */
    for (i = (int32_t)cb->fields_count, fb = cb->fields; --i >= 0; fb++) {
	if (!  (is_legal_fieldname(cb, fb->name, LegalField) &&
		is_legal_field_signature(cb, fb->name, fb->signature))) 
	    result = FALSE;
    }
    /* Make sure we are not overriding any final methods or classes*/
    if (cbSuperclass(cb)) {
	ClassClass *super_cb;
	struct methodblock *mb;
	unsigned bitvector_size = (unsigned)(cbMethodTableSize(cb) + 31) >> 5;
	long *bitvector = sysCalloc(bitvector_size, sizeof(long));
	if (bitvector == NULL) {
		return FALSE;
	}
	for (super_cb = unhand(cbSuperclass(cb)); ;
	     super_cb = unhand(cbSuperclass(super_cb))) {

	    sysAssert(cbMethodTableSize(cb) >= cbMethodTableSize(super_cb));
	    sysAssert(cbMethodTableSize(super_cb)); /* all resolved classes have *some* methods */
	    
	    if (cbAccess(super_cb) & ACC_FINAL) {
		CCerror(cb, "Class %s is subclass of final class %s",
			classname(cb), classname(super_cb));
		result = FALSE;
	    }
	    mb = cbMethods(super_cb);
	    for (i = (int32_t)super_cb->methods_count; --i >= 0; mb++) {
		if (mb->fb.access & ACC_FINAL) {
		    uint32_t offset = mb->fb.u.offset;
		    sysAssert(offset < cbMethodTableSize(cb));
		    bitvector[offset >> 5] |= (1 << (offset & 0x1F));
		}
	    }
	    if (cbSuperclass(super_cb) == 0) break;
	}
	for (i = (int32_t)cb->methods_count, mb = cbMethods(cb); --i >= 0; mb++) {
	    fb_offset_t offset = mb->fb.u.offset;
	    if ((offset > 0) 
		   && bitvector[offset >> 5] & (1 << (offset & 0x1F))) {
		CCerror(cb, "Class %s overrides final method %s.%s",
			classname(cb), mb->fb.name, mb->fb.signature);
		result = FALSE;
	    }
	}
	sysFree(bitvector);
    } else if (cb != classJavaLangObject) {
	CCerror(cb, "Class %s does not have superclass", classname(cb));
	result = FALSE;
    }
	
    if (result)
	result = verify_class_codes(cb);
    if (result) 
	CCSet(cb, Verified);
    return result;
}


static bool_t
verify_constant_pool(ClassClass *cb)
{
    union cp_item_type *cp = cb->constantpool;
    cp_index_type cp_count = cb->constantpool_count;
    unsigned char *type_table = cp[CONSTANT_POOL_TYPE_TABLE_INDEX].p;
    cp_index_type i;
	int type;
    
    const int utf8_resolved = (CONSTANT_Utf8 | CONSTANT_POOL_ENTRY_RESOLVED);
    /* Let's make two quick passes over the constant pool. The first one 
     * checks that everything is of the right type.            */
    for (i = 1; i < cp_count; i++) {
	switch(type = type_table[i]) {
	    case CONSTANT_String:
	    case CONSTANT_Class: {
		cp_index_type index = cp[i].i;
		if (   (index < 1) 
		       || (index >= cp_count)
		       || (type_table[index] != utf8_resolved)) {
		    CCerror(cb, "Bad index in constant pool #%d", i);
		    return FALSE;
		}
		break;
	    }
		
	    case CONSTANT_String | CONSTANT_POOL_ENTRY_RESOLVED:
		/* This can only happen if a string is the "initial" value of
		 * some final static String.  We assume that the checking has
		 * already been done.
		 */
		break;

	    case CONSTANT_Fieldref:
	    case CONSTANT_Methodref:
	    case CONSTANT_InterfaceMethodref: 
	    case CONSTANT_NameAndType: {
		cp_index_type index = cp[i].u;
		cp_index_type key1 = index >> 16;
		cp_index_type key2 = index & 0xFFFF;
		if (key1 < 1 || key1 >= cp_count 
		      || key2 < 1 || key2 >= cp_count) {
		    CCerror(cb, "Bad index in constant pool #%d", i);
		    return FALSE;
		}
		if (type == CONSTANT_NameAndType) {
		    if (   (type_table[key1] != utf8_resolved) 
			 && (type_table[key2] != utf8_resolved)) {
			CCerror(cb, "Bad index in constant pool.");
			return FALSE;
		    }
		} else {
		    if (     ((type_table[key1] & CONSTANT_POOL_ENTRY_TYPEMASK) 
			        != CONSTANT_Class)
			  && ((type_table[key2] != CONSTANT_NameAndType))) {
			CCerror(cb, "Bad index in constant pool #%d", i);
			return FALSE;
		    }
		}
		break;
	    }
		
	    case CONSTANT_Class | CONSTANT_POOL_ENTRY_RESOLVED:
	        /* The only entry that should be resolved if "this" */
	        if (cp[i].p == cb) break;
	        /* FALL THROUGH */
	    case CONSTANT_Fieldref | CONSTANT_POOL_ENTRY_RESOLVED:
	    case CONSTANT_Methodref | CONSTANT_POOL_ENTRY_RESOLVED:
	    case CONSTANT_InterfaceMethodref | CONSTANT_POOL_ENTRY_RESOLVED:
	    case CONSTANT_NameAndType | CONSTANT_POOL_ENTRY_RESOLVED:
	        CCerror(cb, "Improperly resolved constant pool #%d", i);
	        return FALSE;


	    case CONSTANT_Utf8 | CONSTANT_POOL_ENTRY_RESOLVED:
	    case CONSTANT_Integer | CONSTANT_POOL_ENTRY_RESOLVED:
	    case CONSTANT_Float | CONSTANT_POOL_ENTRY_RESOLVED:
		break;

	    case CONSTANT_Long | CONSTANT_POOL_ENTRY_RESOLVED:
	    case CONSTANT_Double | CONSTANT_POOL_ENTRY_RESOLVED:
		if ((i + 1 >= cp_count) || 
		    (type_table[i + 1] != CONSTANT_POOL_ENTRY_RESOLVED)) {
		    CCerror(cb, "Improper constant pool long/double #%d", i);
		    return FALSE;
		} else {
		    i++;	
		    break;	    
		}

	    case CONSTANT_Integer:
	    case CONSTANT_Float:
	    case CONSTANT_Long:
	    case CONSTANT_Double:
	    case CONSTANT_Utf8:
	        CCerror(cb, "Improperly unresolved constant pool #%d", i);
	        return FALSE;


	    default:
	        CCerror(cb, "Illegal constant pool type at #%d", i);
	        return FALSE;


	}
    }
    for (i = 1; i < cp_count; i++) {
	switch(type = type_table[i]) {
	    case CONSTANT_Class: {
		cp_index_type index = cp[i].i;
		if (!is_legal_fieldname(cb, cp[index].cp, LegalClass)) 
		    return FALSE;
		break;
	    }
	      
	    case CONSTANT_Fieldref:
	    case CONSTANT_Methodref:
	    case CONSTANT_InterfaceMethodref: {
		cp_index_type index = cp[i].u;
		cp_index_type name_type_index = index & 0xFFFF;
		cp_index_type name_type_key = cp[name_type_index].u;
		cp_index_type name_index = name_type_key >> 16;
		cp_index_type signature_index = name_type_key & 0xFFFF;
		char *name = cp[name_index].cp;
		char *signature = cp[signature_index].cp;

		if (type == CONSTANT_Fieldref) {
		    if (! (is_legal_fieldname(cb, name, LegalField) &&
			   is_legal_field_signature(cb, name, signature)))
		        return FALSE;
		} else {
		    if (! (is_legal_fieldname(cb, name, LegalMethod) &&
			   is_legal_method_signature(cb, name, signature)))
		        return FALSE;
		}
		break;
	    }
	}
    }
    return TRUE;
}
	    

/* Return true if the entire second argument consists of a legal fieldname 
 * (or classname, if the third argument is LegalClass). 
 */

static bool_t 
is_legal_fieldname(ClassClass *cb, char *name, int type)
{
    bool_t result;
    if (name[0] == '<') {
	result = (type == LegalMethod) && 
	         ((strcmp(name, "<init>") == 0) || 
		  (strcmp(name, "<clinit>") == 0));
    } else {
	char *p;
	if (type == LegalClass && name[0] == SIGNATURE_ARRAY) {
	    p = skip_over_field_signature(name, FALSE);
	} else {
	    p = skip_over_fieldname(name, type == LegalClass);
	}
	result = (p != 0 && p[0] == '\0');
    }
    if (!result) {
	char *thing =    (type == LegalField) ? "Field" 
	               : (type == LegalMethod) ? "Method" : "Class";
			 
	CCerror(cb, "Illegal %s name \"%s\"", thing, name);
	return FALSE;
    } else {
        return TRUE;
    
    }
}

/* Return true if the entire string consists of a legal field signature */
static bool_t 
is_legal_field_signature(ClassClass *cb, char *fieldname, char *signature) 
{
    char *p = skip_over_field_signature(signature, FALSE);
    if (p != 0 && p[0] == '\0') {
	return TRUE;
    } else {
	CCerror(cb, "Field \"%s\" has illegal signature \"%s\"", 
	       fieldname, signature);
	return FALSE;
    }
}


static bool_t 
is_legal_method_signature(ClassClass *cb, char *methodname, char *signature)
{
    char *p = signature;
    char *next_p;
    /* The first character must be a '(' */
    if (*p++ == SIGNATURE_FUNC) {
	/* Skip over however many legal field signatures there are */
	while ((next_p = skip_over_field_signature(p, FALSE)) != 0) 
	    p = next_p;
	/* The first non-signature thing better be a ')' */
	if (*p++ == SIGNATURE_ENDFUNC) {
	    if (methodname[0] == '<') {
		/* All internal methods must return void */
		if ((p[0] == SIGNATURE_VOID) && (p[1] == '\0'))
		    return TRUE;
	    } else {
		/* Now, we better just have a return value. */
		next_p =  skip_over_field_signature(p, TRUE);
		if (next_p && next_p[0] == '\0')
		    return TRUE;
	    }
	}
    }
    CCerror(cb, "Method \"%s\" has illegal signature \"%s\"", 
	   methodname, signature);
    return FALSE;
}
	

/* Take pointer to a string.  Skip over the longest part of the string that
 * could be taken as a fieldname.  Allow '/' if slash_okay is TRUE.
 *
 * Return a pointer to just past the fieldname.  Return NULL if no fieldname
 * at all was found. 
 */
static char *
skip_over_fieldname(char *name, bool_t slash_okay)
{
    bool_t first;
    char *p;
    for (p = name, first = TRUE; ; first = FALSE) {
	char *old_p = p;
	unicode ch = next_utf2unicode(&p);
	if (isalpha(ch) || (!first && isdigit(ch)) || (slash_okay && ch == '/' && !first)
	      || ch == '_' || ch == '$' || ch >= 0x00c0) {
	    /* donothing */
	} else {
	    return first ? 0 : old_p;
	}
    }
}



/* Take pointer to a string.  Skip over the longest part of the string that
 * could be taken as a field signature.  Allow "void" if void_okay.
 *
 * Return a pointer to just past the signature.  Return NULL if no legal
 * signature is found.
 */

static char *
skip_over_field_signature(char *name, bool_t void_okay)
{
    for (;;) {
	switch (name[0]) {
            case SIGNATURE_VOID:
		if (!void_okay) return 0;
		/* FALL THROUGH */
            case SIGNATURE_BOOLEAN:
            case SIGNATURE_BYTE:
            case SIGNATURE_CHAR:
            case SIGNATURE_SHORT:
            case SIGNATURE_INT:
            case SIGNATURE_FLOAT:
            case SIGNATURE_LONG:
            case SIGNATURE_DOUBLE:
		return name + 1;

	    case SIGNATURE_CLASS: {
		/* Skip over the classname, if one is there. */
		char *p = skip_over_fieldname(name + 1, TRUE);
		/* The next character better be a semicolon. */
		if (p && p[0] == ';') 
		    return p + 1;
		return 0;
	    }
	    
	    case SIGNATURE_ARRAY: 
		/* The rest of what's there better be a legal signature.  */
		name++;
		void_okay = FALSE;
		break;

	    default:
		return 0;
	}
    }
}

static void 
CCerror (ClassClass *cb, char *format, ...)
{
    va_list args;
    #define BUF_LEN	1024
    char buf[BUF_LEN];
    int i = 0;
    i += jio_snprintf(buf, BUF_LEN, "VERIFIER CLASS ERROR %s:\n", 
		      classname(cb));
    va_start(args, format);
	i += jio_vsnprintf(buf + i, BUF_LEN - i, format, args);
    va_end(args);
    i += jio_snprintf(buf + i, BUF_LEN - i, "\n");
#ifdef DEBUG
    fprintf(stderr, buf);
#endif /* DEBUG */
    PrintToConsole(buf);
}
