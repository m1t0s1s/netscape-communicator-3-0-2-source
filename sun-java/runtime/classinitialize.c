/*
 * @(#)classinitialize.c	1.62 95/12/02  
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
 *      Runtime routines to support classes.
 */

#include <ctype.h>
#include <string.h>
#include <sys/types.h>
#include "timeval.h"
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>

#include "oobj.h"
#include "tree.h"
#include "interpreter.h"
#include "bool.h"
#include "javaThreads.h"
#include "monitor.h"
#include "exceptions.h"
#include "javaString.h"
#include "path.h"
#include "byteorder.h"
#include "utf.h"

bool_t VerifyClass(ClassClass *cb);

void InitializeForCompiler_md(ClassClass *cb);
void InitializeForCompiler(ClassClass *cb);
static bool_t Locked_ResolveClassConstantField(unsigned type,
					       ClassClass *class,
					       cp_item_type *constant_pool, 
					       cp_index_type index, 
					       struct execenv *ee);
static bool_t Locked_ResolveClassConstant(ClassClass *Class, cp_item_type *, 
					  cp_index_type, struct execenv *, unsigned);



bool_t VerifyClassAccess(ClassClass *, ClassClass *, bool_t);
bool_t VerifyFieldAccess(ClassClass *, ClassClass *, int, bool_t);
bool_t IsSameClassPackage(ClassClass *class1, ClassClass *class2); 


#undef CONSTANT_POOL_TYPE_TABLE_PUT
#undef CONSTANT_POOL_TYPE_TABLE_SET_RESOLVED
void CONSTANT_POOL_TYPE_TABLE_PUT(unsigned char *cp,int i,int v) {
sysAssert(cp != 0);
sysAssert(i>=0);
sysAssert(i<=0xFFFF);
    CONSTANT_POOL_TYPE_TABLE_GET(cp,i) = (v);
}
void CONSTANT_POOL_TYPE_TABLE_SET_RESOLVED(unsigned char *cp,cp_index_type i) {
sysAssert(cp != 0);
/*sysAssert(i>=0);*/	/* i is unsigned */
sysAssert(i<=0xFFFF);
    CONSTANT_POOL_TYPE_TABLE_GET(cp,i) |= CONSTANT_POOL_ENTRY_RESOLVED;
}

int verify_in_progress;
#include "prmon.h"

/*  
    Perform runtime-specific initialization on the class.  The
    definition of this routine if different depending on if we are
    compiling the interpreter or not.  Resolve the instance
    prototype, load class constants, and run the static
    initializers, if any.  
*/
char *
RuntimeInitClass(struct execenv *ee, ClassClass * cb)
{
    InitializeInvoker(cb);
    InitializeForCompiler(cb);
    if ((verifyclasses == VERIFY_ALL) ||
	  ((verifyclasses == VERIFY_REMOTE) && (cbLoader(cb) != NULL))) {
	if (!VerifyClass(cb))
	    return JAVAPKG "VerifyError";
    }
    return RunStaticInitializers(cb);
}

/* ResolveClassStringConstant is called by the classloader to resolve the
 * constant pool entry, before assigning a value to a final [i.e. constant]
 * String.
 *
 * For the interpreter, we want this to be a String object with the
 * appropriate initial value. 
 */

bool_t
ResolveClassStringConstant(ClassClass *cb, cp_index_type index, struct execenv *ee)
{
    /* This function is called before the verifier has ever had a chance
     * to look at the class.  So we should be extra careful.
     */
    union cp_item_type *constant_pool = cbConstantPool(cb);
    unsigned char *type_table = constant_pool[CONSTANT_POOL_TYPE_TABLE_INDEX].p;
    if (type_table[index] == CONSTANT_String | CONSTANT_POOL_ENTRY_RESOLVED)
	return TRUE; 
    else if (type_table[index] == CONSTANT_String) { 
    cp_index_type string_index = CPIndexAt(constant_pool,index);
    if ((string_index < 1) || 
	(string_index >= (cp_index_type)cb->constantpool_count) ||
	(type_table[string_index] != 
	     (CONSTANT_Utf8 | CONSTANT_POOL_ENTRY_RESOLVED)))
	return FALSE;
	return ResolveClassConstant(constant_pool, index, ee, 
				    1 << CONSTANT_String);
    } else { 
	return FALSE;
    }
}

void 
ResolveInit()
{
}

bool_t 
ResolveClassConstantFromClass(ClassClass *class, unsigned index, 
			      struct execenv *ee, unsigned mask)
{
    cp_item_type *constant_pool = cbConstantPool(class);
    bool_t ret;

    monitorEnter(obj_monitor(class));
    ret = Locked_ResolveClassConstant(class, constant_pool, index, ee, mask);
    monitorExit(obj_monitor(class));
    return ret;
}
    

bool_t
ResolveClassConstant(cp_item_type *constant_pool, cp_index_type index, 
		     struct execenv *ee, unsigned mask)
{
    bool_t ret;
    ClassClass *class = 
	(ee && ee->current_frame && ee->current_frame->current_method) ?
	    fieldclass(&ee->current_frame->current_method->fb) : 0;

    monitorEnter(obj_monitor(class));
    ret = Locked_ResolveClassConstant(class, constant_pool, index, ee, mask);
    monitorExit(obj_monitor(class));
    return ret;
}


/*
 * This routine resolves class constants like field names,
 * methodnames, and class names, to their runtime-specific values.
 * Field names are resolved to offsets from the beginning og an
 * instance, methodnames are resolved to pointers to code, and class
 * names are resolved to pointers to the class.
 */
static bool_t
Locked_ResolveClassConstant(ClassClass *current_class, 
			    cp_item_type *constant_pool, 
			    cp_index_type index, 
			    struct execenv *ee, unsigned mask)
{
    unsigned char *type_table = constant_pool[CONSTANT_POOL_TYPE_TABLE_INDEX].p;

    if (!CONSTANT_POOL_TYPE_TABLE_IS_RESOLVED(type_table, index)) {
	unsigned type = CONSTANT_POOL_TYPE_TABLE_GET_TYPE(type_table, index);
	sysAssert((mask & (1 << type)) != 0);

        /*
        ** XXX: There is a race between storing the Class into the constant pool
        **      and setting the constant_pool_type_is_resolved flag which allows
        **      the GC to mark the constant pool slot as a root.
        **
        **      See gc_md.c ScanClassClass(...)
        */
	switch(type) {
	  case CONSTANT_Class: {
	      cp_index_type name_index = CPIndexAt(constant_pool,index);
	      char *name = constant_pool[name_index].cp;
	      ClassClass *class;

	      class = FindClassFromClass(ee, name, TRUE, 
					 current_class);
	      if (CONSTANT_POOL_TYPE_TABLE_IS_RESOLVED(type_table, index)) {
		  return TRUE;
	      } else if (class == 0) {
		  if (!ee) ee = EE();
		  if (!exceptionOccurred(ee)) {
		      /* Force exception to NoClassDefFoundError only when
			 an exception was not already set */
		      SignalError(ee, JAVAPKG "NoClassDefFoundError", name);
		  }
		  return FALSE;
	      } else if (VerifyClassAccess(current_class, class, TRUE)) {
		  constant_pool[index].p = class;
	      } else {
		  SignalError(ee, JAVAPKG "IllegalAccessError", name);
		  return FALSE;
	      }
	      if (class->name[0] == SIGNATURE_ARRAY)
		  ResolveClassConstantFromClass(class, 
						CONSTANT_POOL_ARRAY_CLASS_INDEX,
						ee, 
						1 << CONSTANT_Class | 
						1 << CONSTANT_Integer);
	      break;
	  }

	  case CONSTANT_String:  {
	      cp_index_type value_index = CPIndexAt(constant_pool, index);
	      char *value = constant_pool[value_index].cp;
	      int length = utfstrlen(value);
	      int i;
	      /* Allocate a byte array.  Copy bytes in. */
	      HArrayOfChar *so = (HArrayOfChar *)ArrayAlloc(T_CHAR, length);
	      if (so == 0) {
		  SignalError(ee, JAVAPKG "OutOfMemoryError", 0);
		  return FALSE;
	      }
	      utf2unicode(value, unhand(so)->body, length, &i);
	      sysAssert(i == length);
	      constant_pool[index].p = 
		  execute_java_constructor(ee, 0, classJavaLangString, "([C)", so);
	      if (ee == 0)
		  ee = EE();
	      if (exceptionOccurred(ee)) 
		  return FALSE;
	      break;
	  }
	    
	  case CONSTANT_NameAndType: {
	      uint32_t key = constant_pool[index].u;
	      cp_index_type name_index = key >> 16;
	      cp_index_type type_index = key & 0xFFFF;
	      char *name = constant_pool[name_index].cp;
	      char *type = constant_pool[type_index].cp;
	      constant_pool[index].i = NameAndTypeToHash(name, type,
                                                         current_class);
	      break;
	  }

	  case CONSTANT_InterfaceMethodref: {
	      /* The verifier needs to worry about the class.  We don't care. */
		  uint32_t key = constant_pool[index].i;
		  cp_index_type name_type_index = key & 0xFFFF;
	      Locked_ResolveClassConstant(current_class, 
					  constant_pool, name_type_index, ee, 
					  1 << CONSTANT_NameAndType);
	      return TRUE;
	  }

	  case CONSTANT_Fieldref:
	  case CONSTANT_Methodref: 
	      if (!Locked_ResolveClassConstantField(type, current_class, 
						    constant_pool, index, ee))
		  return FALSE;
	      break;
	}
	CONSTANT_POOL_TYPE_TABLE_SET_RESOLVED(type_table, index);
    } 
    return TRUE;
} 

static bool_t
Locked_ResolveClassConstantField(unsigned type,
				 ClassClass *class, 
				 cp_item_type *constant_pool, 
				 cp_index_type index, 
				 struct execenv *ee)
{
    uint32_t key = constant_pool[index].i;
    cp_index_type class_index = key >> 16;
    cp_index_type name_type_index = key & 0xFFFF;
    uint32_t ID;
    char buf[256];
    int len;
    ClassClass *cb;

    if (!Locked_ResolveClassConstant(class, constant_pool, class_index, ee, 
				    1 << CONSTANT_Class))
        return FALSE;
    Locked_ResolveClassConstant(class, constant_pool, name_type_index, ee, 
				1 << CONSTANT_NameAndType);

    ID = constant_pool[name_type_index].i;
    cb = constant_pool[class_index].p;
    switch (type) {
        case CONSTANT_Methodref: {
	    int n = cb->methods_count;
	    struct methodblock *mb = cbMethods(cb);
	    for (; --n >= 0; mb++) {
		if (mb->fb.ID == ID) {
		    if (VerifyFieldAccess(class, cb, mb->fb.access, TRUE)) {
			constant_pool[index].p = mb;
			return TRUE;
		    } else {
			classname2string(classname(cb), buf, sizeof(buf));
			len = strlen(buf);
			/* On buffer overflow truncate string quietly */
			(void) jio_snprintf(buf + len, sizeof(buf)-len, ".%s",
				IDToNameAndTypeString(ID));
			SignalError(ee, JAVAPKG "IllegalAccessError", buf);
			return FALSE;
		    }
		}
	    }
	    classname2string(classname(cb), buf, sizeof(buf));
	    len = strlen(buf);
	    /* On buffer overflow truncate string quietly */
	    (void) jio_snprintf(buf + len, sizeof(buf)-len,
		    ": method %s not found",
		    IDToNameAndTypeString(ID));
	    SignalError(ee, JAVAPKG "NoSuchMethodError", buf);
	    return FALSE;
	}
	
        case CONSTANT_Fieldref: {
	    int n = cb->fields_count;
	    struct fieldblock *fb = cbFields(cb);
	    for (; --n >= 0; fb++) {
		if (fb->ID == ID) {
		    if (VerifyFieldAccess(class, cb, fb->access, TRUE)) {
			constant_pool[index].p = fb;
			return TRUE;
		    } else {
			classname2string(classname(cb), buf, sizeof(buf));
			len = strlen(buf);
			/* Reporting error, so truncate string quietly */
			(void) jio_snprintf(buf + len, sizeof(buf)-len, ".%s",
				IDToNameAndTypeString(ID));
			SignalError(ee, JAVAPKG "IllegalAccessError", buf);
			return FALSE;
		    }
		}
	    }
	    classname2string(classname(cb), buf, sizeof(buf));
	    len = strlen(buf);
	    /* Reporting error, so truncate string quietly */
	    (void) jio_snprintf(buf + len, sizeof(buf)-len, ": field %s not found",
		    IDToNameAndTypeString(ID));
	    SignalError(ee, JAVAPKG "NoSuchFieldError", buf);
	    return FALSE;
	}
    }

    return FALSE;
}

/* Verify that current_class can access new_class.  If the classloader_only
 * flag is set, we automatically allow any accesses in which current_class
 * doesn't have a classloader.
 */

bool_t
VerifyClassAccess(ClassClass *current_class, ClassClass *new_class, 
		  bool_t classloader_only) 
{
    if (current_class == 0) {
	return TRUE;
    } else 
	return ((classloader_only && (cbLoader(current_class) == 0))
		|| (current_class == new_class)
		|| IsPublic(cbAccess(new_class))
		|| IsSameClassPackage(current_class, new_class));
}    



/* Verify that current_class can a field of new_class, where that field's 
 * access bits are "access".  We assume that we've already verified that
 * class can access field_class.
 *
 * If the classloader_only flag is set, we automatically allow any accesses
 * in which current_class doesn't have a classloader.
 */


bool_t 
VerifyFieldAccess(ClassClass *current_class, ClassClass *field_class, 
		  int access, bool_t classloader_only)
{
    if (current_class == 0) {
	return IsPublic(access);
    }
    if ((current_class == field_class) || IsPublic(access))
	return TRUE;
    else if (classloader_only && (cbLoader(current_class) == 0))
	return TRUE;
    if (IsProtected(access)) {
	/* See if current_class is a subclass of field_class */
	ClassClass *cb;
	for (  cb = unhand(cbSuperclass(current_class)); ;
	       cb = unhand(cbSuperclass(cb))) {
	    if (cb == field_class) 
		return TRUE;
	    if (cbSuperclass(cb) == 0) 
		break;
	}
    }
    return (!IsPrivate(access)
	    && IsSameClassPackage(current_class, field_class));
}

