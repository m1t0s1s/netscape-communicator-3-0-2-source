/*
 * @(#)classresolver.c	1.54 96/01/11  
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
 *      Routines for loading and resolving class definitions.
 *      These routines should not depending up on the interpreter or
 *      garbage collector.
 *
 */

#include <ctype.h>
#include <string.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <stdlib.h>

#include "timeval.h"
#include "oobj.h"
#include "interpreter.h"
#include "signature.h"
#include "tree.h"
#include "monitor.h"
#include "bool.h"
#include "path.h"
#include "sys_api.h"

ClassClass *classJavaLangObject;
ClassClass *classJavaLangClass;
ClassClass *classJavaLangString;
ClassClass *classJavaLangThrowable;
ClassClass *classJavaLangException;
ClassClass *classJavaLangRuntimeException;
ClassClass *classJavaLangThreadDeath;

ClassClass *interfaceJavaLangCloneable;


char *RuntimeInitClass(struct execenv *, ClassClass *);

static ClassClass *Locked_FindClassFromClass(struct execenv *ee, char *name, bool_t resolve, ClassClass *from);
static char *Locked_InitializeClass(ClassClass * cb, char **detail);
static char *Locked_ResolveClass(ClassClass * cb, char **detail);

static sys_mon_t *_class_lock = NULL;
 int class_lock_depth = 0;


static struct fieldblock **
addslots(struct fieldblock ** fb, ClassClass * clb)
{
    long n = clb->fields_count;
    struct fieldblock *sfb = cbFields(clb);
    if (cbSuperclass(clb))
	fb = addslots(fb, unhand(cbSuperclass(clb)));
    while (--n >= 0) {
	*fb++ = sfb;
	sfb++;
    }
    return fb;
}

struct fieldblock **
makeslottable(ClassClass * clb)
{
    ClassClass *sclb;
    uint32_t    sizeOfAllocation;
    int         nslots = 0;
    if (cbSlotTable(clb)) {
	return cbSlotTable(clb);
    }
    sclb = clb;
    while (sclb) {
	long        n = sclb->fields_count;
	struct fieldblock *fb = cbFields(sclb);
	while (--n >= 0) {
	    nslots++; /* XXX: Change to using add, instead of counting :-/ */ 
	    fb++;
	}
	if (cbSuperclass(sclb) == 0) {
	    break;
	}
	sclb = unhand(cbSuperclass(sclb));
    }
    cbSlotTableSize(clb) = nslots;
    if (nslots == 0) {
	nslots++;
    }

    sizeOfAllocation = nslots * sizeof(struct fieldblock *);
    PR_ARENA_ALLOCATE(cbSlotTable(clb), &(clb->classStorageArena->pool), sizeOfAllocation);
		    
    if (cbSlotTable(clb) == 0) {
	return NULL;
    }
    addslots(cbSlotTable(clb), clb);
    return cbSlotTable(clb);
}

#if 0
static char *
copyclassname(char *src, char *dst)
{
    sysAssert(*src == SIGNATURE_CLASS);
    src++;
    while (*src && *src != SIGNATURE_ENDCLASS)
	*dst++ = *src++;
    *dst = 0;
    return src;
}
#endif

static void
ResolveFields(ClassClass *cb, unsigned slot)
{
    struct fieldblock *fb;
#if TOTAL_HASH
    int thishash = 0;
    char *s;
    int c;
#endif
    int size;

    fb = cbFields(cb);
    for (size = 0; size < (int) cb->fields_count; size++, fb++) {
	char *signature = fieldsig(fb);
#ifdef OSF1
	int slots = 1;		/* everything fits in an OBJECT */
#else
	int slots = (((signature[0] == SIGNATURE_LONG || 
		      signature[0] == SIGNATURE_DOUBLE)) ? 2 : 1);
#endif
	fb->ID = NameAndTypeToHash(fb->name, signature, cb);
	if (fb->access & ACC_STATIC) {
	    /* Do nothing.  Handled when the class is loaded. */
	} else {
	    int bytes = slots * sizeof(OBJECT);
	    if (slot & (bytes - 1)) {
		/* Need to add in padding */
		slot = slot + bytes - (slot & (bytes - 1));
	    }
	    fb->u.offset = slot;
	    slot += bytes;
	}
#if TOTAL_HASH
	if ((fb->access & (ACC_STATIC | ACC_TRANSIENT)) == 0) {
	    for (s = fieldname(fb); (c = *s++) != 0;)
		thishash = thishash * 7 + c;
	    for (s = fieldsig(fb); (c = *s++) != 0;)
		thishash = thishash * 7 + c;
	}
#endif
    }
    cbInstanceSize(cb) = slot;
#if TOTAL_HASH
    cbThisHash(cb) = thishash;
    if (cbSuperclass(cb))
	cbTotalHash(cb) = thishash - cbTotalHash(unhand(cbSuperclass(cb)));
    else
        cbTotalHash(cb) = thishash;
    if (cbTotalHash(cb) < N_TYPECODES)
	cbTotalHash(cb) += N_TYPECODES;
#endif
}


char *
ResolveMethods(ClassClass *cb)
{
    struct methodblock *mb;
    int size;
    struct methodtable *new_table;
    struct methodblock **super_methods;
    uint32_t sizeOfAllocation;
    unsigned mslot, super_methods_count;
    void *ptr;
    static hash_t finalizerID = 0;
    
    if (finalizerID == 0)
	finalizerID = NameAndTypeToHash(FINALIZER_METHOD_NAME,
					FINALIZER_METHOD_SIGNATURE, 0);
    if (cbSuperclass(cb) != NULL) { 
	ClassClass *super = unhand(cbSuperclass(cb));
	mslot = cbMethodTableSize(super);
	if (!mslot) {
	    char * ret;
#ifdef DEBUG_jar
	    fprintf(stderr, "Recursively resolving base class %s in %p\n", classname(cb), EE());
#endif
	    ret = ResolveMethods(super);
#ifdef DEBUG_jar
	    fprintf(stderr, " Finished resolving base class %s in %p, %d\n", classname(cb), EE(), !ret);
#endif
	    if (ret) return ret;
	    mslot = cbMethodTableSize(super);
	    sysAssert(mslot);
        }
	super_methods = cbMethodTable(super)->methods;
        super_methods_count = cbMethodTableSize(super);
	/* Inherit one's parent's finalizer, if it has one */
	cb->finalizer = unhand(cb->superclass)->finalizer;
    } else { 
	mslot = 1;
	super_methods = NULL;
	super_methods_count = 0;
	cb->finalizer = NULL;
    }

    mb = cbMethods(cb); 
    for (size = 0; size < (int) cb->methods_count; size++, mb++) {
	unsigned long ID = NameAndTypeToHash(mb->fb.name, mb->fb.signature,
                                             cb);
	struct methodblock **super_methods_p;
	int count;

	mb->fb.ID = ID;
	mb->fb.u.offset = 0;

	if (mb->fb.access & ACC_STATIC) 
	    continue;
	if (strcmp(mb->fb.name, "<init>") == 0)
	    continue;

	/* If this item has its own finalizer method, grab it */
	if (mb->fb.ID == finalizerID && cb != classJavaLangObject) 
	    cb->finalizer = mb;

	for (super_methods_p = super_methods, count = super_methods_count;
	       count > 0;
	       super_methods_p++, count--) {
	    struct methodblock *sm = *super_methods_p;

            if ((sm != NULL) && (sm->fb.ID == ID)
                && VerifyFieldAccess(cb, sm->fb.clazz, sm->fb.access, FALSE)) {
		mb->fb.u.offset = (*super_methods_p)->fb.u.offset;
		break;
	    }
	}

	if (mb->fb.u.offset == 0) { 
	    mb->fb.u.offset = mslot;
	    mslot++;
	}
    }

    /*
	This should be the only place that method tables are
	allocated.  We allocate more than we need here and mask the
	resulting pointer because the methodtable pointer field in
	object handles is overloaded and sometimes hold array types
	and array lengths.  We must ensure that method table pointers
	are allocated on an appropriate boundary so we don't lose any
	address bits when we mask off the type portion of the pointer.
    */
    
    sizeOfAllocation = sizeof(struct methodtable)
		       + (mslot - 1)* sizeof(struct methodblock *)
		       + FLAG_MASK;
    PR_ARENA_ALLOCATE(ptr, &(cb->classStorageArena->pool), sizeOfAllocation);
		    
    if (ptr == NULL) {
	CCSet(cb, Resolved);
	return "OutOfMemoryError";
    }
    new_table = (struct methodtable *)((((long)ptr) + FLAG_MASK) & LENGTH_MASK);
    new_table->classdescriptor = cb;
    memset((char *)new_table->methods, 0, mslot * sizeof(struct methodblock *));
    if (super_methods) 
	memcpy((char *) new_table->methods,
	       (char *) super_methods,
	       super_methods_count * sizeof(struct methodblock *));
    mb = cbMethods(cb);
    for (size = 0; size < (int) cb->methods_count; size++, mb++) {
	int32_t offset = (int32_t)mb->fb.u.offset;
	if (offset > 0) { 
	    sysAssert((uint32)offset < mslot);
	    mt_slot(new_table, offset) = mb;
	}
    }

    cbMethodTable(cb) = new_table;
    cbMethodTableSize(cb) = mslot;

    return 0;
}



char *
InitializeClass(ClassClass * cb, char **detail) 
{ 
    char *result;

    monitorEnter(obj_monitor(cb));
    result = Locked_InitializeClass(cb, detail);
    monitorExit(obj_monitor(cb));
    return result;
}

char *
ResolveClass(ClassClass *cb, char **detail) 
{
    char *result;

    monitorEnter(obj_monitor(cb));
    result = Locked_ResolveClass(cb, detail);
    monitorExit(obj_monitor(cb));
    return result;
}



static char *
Locked_InitializeClass(ClassClass * cb, char **detail)
{
	ClassClass *super = 0;
	char *ret = 0;
        bool_t noLoader;
	
	*detail = 0;
	if (CCIs(cb, Initialized))
		return 0;
	
#ifndef TRIMMED
    if (verbose)
		fprintf(stderr, "[Initializing %s]\n", classname(cb));
#endif
	
	noLoader = (cbLoader(cb) == 0);
	if (cb->fields_count > 2000) {
		return JAVAPKG "ClassFormatError";
	}
	if ((strcmp(classname(cb), CLS_RESLV_INIT_CLASS) == 0) && noLoader) {
                ClassClass * volatile tmpClass;

		classJavaLangClass = cb;
		cb->flags |= CCF_Sticky;

/*
** This macro is necessary for preemptive threading environments.  Since the 
** classes are being stored into global variables, they DO NOT show up on the
** C-Stack and are vulnerable to GC until they are made sticky...
*/
#define STORE_GLOBAL_STICKY_CLASS(var, name)                        \
                tmpClass = FindClass(NULL, JAVAPKG #name, TRUE);    \
		tmpClass->flags |= CCF_Sticky;                      \
                var = tmpClass;

		STORE_GLOBAL_STICKY_CLASS(classJavaLangString,           String);
		STORE_GLOBAL_STICKY_CLASS(classJavaLangThreadDeath,      ThreadDeath);
		STORE_GLOBAL_STICKY_CLASS(classJavaLangThrowable,        Throwable);
		STORE_GLOBAL_STICKY_CLASS(classJavaLangException,        Exception);
		STORE_GLOBAL_STICKY_CLASS(classJavaLangRuntimeException, RuntimeException);
		STORE_GLOBAL_STICKY_CLASS(interfaceJavaLangCloneable,    Cloneable);

#undef STORE_GLOBAL_STICKY_CLASS

	} else if ((strcmp(classname(cb), CLS_RESLV_INIT_OBJECT) == 0) && noLoader){
		classJavaLangObject = cb;
                cb->flags |= CCF_Sticky;
	}
	
	cbClassnameArray(cb) = MakeString(classname(cb), strlen(classname(cb)));
	
	if (cbHandle(cb) == 0) {
		cbHandle(cb) = (HClass *) AllocHandle(mkatype(T_BYTE, 1), (ClassObject *) cb);
		if (cbHandle(cb) == 0)
			return JAVAPKG "OutOfMemoryError";
	}
	if ((strcmp(classname(cb), CLS_RESLV_INIT_REF) == 0) && noLoader) { 
		CCSet(cb, SoftRef);
	}
	if (cbSuperclass(cb) == 0) {
		if (cb->super_name) {
			if ((super = FindClassFromClass(0, classsupername(cb), FALSE, cb))) {
				sysAssert(CCIs(super, Initialized));
				cbSuperclass(cb) = cbHandle(super);
				if (CCIs(super, SoftRef))
					CCSet(cb, SoftRef);
			} else {
				ret = JAVAPKG "NoClassDefFoundError";
				*detail = classsupername(cb);
				cbSuperclass(cb) = 0;
			} 
		} else if (cb == classJavaLangObject) {
			cbSuperclass(cb) = 0;	    
		} else {
			*detail = classname(cb);
		return JAVAPKG "ClassFormatError";
		}
	}
	CCSet(cb, Initialized);
	
	/* Make sure we know what classJavaLangClass is, and that it's method table
	* is filled in.  Don't worry about java_lang_Class from being unloaded since
        * every loaded class holds a reference to it...
	*/
	if (classJavaLangClass == 0) {
		classJavaLangClass = 
		FindClassFromClass(0, CLS_RESLV_INIT_CLASS, TRUE, cb);
		if (classJavaLangClass == 0) {
			return JAVAPKG "NoClassDefFoundError";
		}
	}

	/* The following may not do anything useful if classJavaLangClass hasn't
	* been completely resolved.  But we clean up in ResolveClass 
	*/
	
	cbHandle(cb)->methods = cbMethodTable(classJavaLangClass);
	return ret;
}

static char *
Locked_ResolveClass(ClassClass * cb, char **detail)
{
    ClassClass *super = 0;
    unsigned slot = 0;
    char *ret = 0;

    *detail = 0;
    if (CCIs(cb, Error)) {
	return JAVAPKG "NoClassDefFoundError";
    }
   
    sysAssert(CCIs(cb, Initialized));

    if (CCIs(cb, Resolved)) {
	return 0;
    }

    cbInstanceSize(cb) = (unsigned short) -1;

    if (cbSuperclass(cb)) {
	/* If this object has a superclass. . . */
	super = unhand(cbSuperclass(cb));
	if (!CCIs(super, Resolved)) {
	    if ((ret = ResolveClass(super, detail)) != 0) {
		CCSet(cb, Error);
		return ret;
	    }
	}
	sysAssert(CCIs(super, Resolved));
	slot = cbInstanceSize(super);
	if ((int) slot < 0) {
#ifndef TRIMMED
	    fprintf(stderr, "%s: Can't resolve class because of interdependant static initializers\n", classname(cb));
#endif
	    CCSet(cb, Error);
	    return JAVAPKG "NoClassDefFoundError";
	}
    }
    
#ifndef TRIMMED
    if (verbose)
        fprintf(stderr, "[Resolving %s]\n", classname(cb));
#endif
    CCSet(cb, Resolved);
    ResolveFields(cb, slot);
   
    ret = ResolveMethods(cb);
    if (ret != NULL) {	/* May return "OutOfMemoryError */
	*detail = cb->name;
	CCClear(cb, Resolved);
	CCSet(cb, Error);
    	return ret;
    }

    /*	Perform runtime-specific initialization on the class.  The
	definition of this routine if different depending on if we
	are compiling the interpreter or not.
    */
    if ((ret = RuntimeInitClass(0, cb)) != 0) {
	*detail = cb->name;
	CCClear(cb, Resolved);
	CCSet(cb, Error);
        return ret;
    }

    /* We need this for bootstrapping.  We can't set the Handle's class block
     * pointer in Object or Class until Class has been resolved.
     */
    if (cb == classJavaLangClass) {
	int32_t         j;
	ClassClass **pcb;
	for (j = nbinclasses, pcb = binclasses; --j >= 0; pcb++) {
            ClassClass *cc = *pcb;
            if (!cc) continue;
	    cbHandle(cc)->methods = cbMethodTable(cb);
	}
    }
    return 0;
}


/* Find an already loaded class with the given name.  If no such class exists
 * then return 0.
 *
 * We attempt to resolve it, also, if the "resolve" argument is true.
 * Otherwise, we only perform a minimal initialization on it, such that
 * it points to its superclass, which points to its superclass. . . .
 */

#ifdef DEBUG
void
PrintLoadedClasses()
{
    int32_t         j;
    ClassClass **pcb, *cb;
    pcb = binclasses;
    for (j = nbinclasses; --j >= 0; pcb++) {
	cb = *pcb;
        if (!cb) continue;
        printf("%ld %s 0x%p loader=0x%p\n", j, classname(cb), cb, cb->loader);
    }
}
#endif


/* 
 * Check to see if the named class was loaded by any loader.
 * This is used to be sure that the contents of the class path 
 * haven't changed, and that a network copy isn't loaded
 * before a local (classpath) copy
 */

/* 
 * This is terrible and time consuming... binclasses should 
 * really be a hash table... so I put in one tiny optimization
 * 'cause this has got to be a time sync 
 */
static bool_t
IsLoadedAnywhere(char *name)
{
    int32_t         j;
    ClassClass **pcb, *cb;
    char *existing_name;
    char first_char_of_name = name[0]; /* optimization to avoid strcmp() calls */

    sysAssert(class_lock_depth > 0);

    pcb = binclasses;
    for (j = nbinclasses; --j >= 0; pcb++) {
	cb = *pcb;
        if (!cb) 
          continue;
        existing_name = classname(cb);
        if (first_char_of_name != existing_name[0])
          continue;
	if (strcmp(name, existing_name))
          continue;
        return TRUE;
    }
    return FALSE;
}


/*
 * Check for the existance in binclasses of a given name/loader
 * pair.  Return null if it is not present.
 * This method has NO SIDE EFFECTS.
 * This method is called as a last-line-of-defense against
 * defining two classes with identical names in the same 
 * class loader.
 */

ClassClass *
FindPreviouslyLoadedClass(char *name, HObject *loader) {
    int32_t         j;
    ClassClass **pcb, *cb;

    sysAssert(!_class_lock || class_lock_depth > 0);

    pcb = binclasses;
    for (j = nbinclasses; --j >= 0; pcb++) {
	cb = *pcb;
        if (!cb) continue;
	if ((cb->loader == loader) && (strcmp(name, classname(cb)) == 0))
          return cb;
    }
    return NULL;
}


static ClassClass *
FindLoadedClass(char *name, bool_t resolve, HObject *loader)
{
    ClassClass *cb;
    char *err, *detail;

    sysAssert(class_lock_depth > 0);

/*     printf("FindLoadedClass %s %p\n", name, loader); */
    cb = FindPreviouslyLoadedClass(name, loader);
    if (!cb) return 0;

    if (!CCIs(cb, Initialized)) {
      if ((err = InitializeClass(cb, &detail))) {
        SignalError(0, err, detail);
        return 0;
      }
    }
    if (resolve) {
      /* bug/hack: The following used to assume one level of unlock_classes()
         will release the lock... but it may take many levels :-( 
         This is a strange thing to do, as it releases a lock that our
         calling stack was relying on!!!*/
      int old_depth = class_lock_depth, old_depth2 = class_lock_depth ;
      while (0 < old_depth--)
        unlock_classes();
      err = ResolveClass(cb, &detail);
      while (0 < old_depth2--)
        lock_classes();
      if (err) {
        SignalError(0, err, detail);
        return 0;
      }
    }
    return cb;
}

/* Find the class with the given name.  
 * If "resolve" is true, then we completely resolve the class.  Otherwise,
 * we only make sure that it points to its superclass, which points to its
 * superclass, . . . .
 */
ClassClass *(*class_loader_func)(HObject *loader, char *name, long resolve) = 0;

/*
** XXX
** Make this a JRI_PUBLIC_API for now. It really isn't, but awt isn't following the
** rules yet, and it does need this API.
*/
JRI_PUBLIC_API(ClassClass *)
FindClass(struct execenv *ee, char *name, bool_t resolve)
{
    return FindClassFromClass(ee, name, resolve,
    	    (ee && ee->current_frame && ee->current_frame->current_method) ?
  	          fieldclass(&ee->current_frame->current_method->fb) : 0);
}

static ClassClass *
FindClassLoaderArrayClass(struct execenv *ee, char *name, 
			  bool_t resolve, ClassClass *from);


void
lock_classes()
{
    static bool_t initialized = FALSE;
    if (!initialized) {
	_class_lock = (sys_mon_t *) sysMalloc(sysMonitorSizeof());
	memset((char *) _class_lock, 0, sysMonitorSizeof());
	monitorRegister(_class_lock, "Class lock");
	initialized = TRUE;
    }
    sysMonitorEnter(_class_lock);
    class_lock_depth++;
}

void
unlock_classes()
{
    class_lock_depth--;
    sysMonitorExit(_class_lock);
}

static ClassClass *
Locked_FindClassFromClass(struct execenv *ee, char *name, bool_t resolve, ClassClass *from)
{
    ClassClass *cb = 0;

    sysAssert(class_lock_depth > 0);

    if (from && from->loader && class_loader_func) {
		if (name[0] == SIGNATURE_ARRAY) 
		    cb = FindClassLoaderArrayClass(ee, name, resolve, from);
		else {
		    /* unlock the classes while the class is located by
	 	     * the class loader. The class loader should do this
		     * in a synchronized method   XXX THIS IS REAL DANGEROUS!!!
		     */
		    int i, old_depth = class_lock_depth;
		    for (i = 0; i < old_depth; i++) {
				unlock_classes();
		    }
		    cb = (*class_loader_func)(from->loader, name, resolve);
		    while (--old_depth >= 0) {
			    lock_classes();
			}
	    }
    }
    if (cb == 0) {
		cb = FindLoadedClass(name, resolve, 0);
		if (cb == 0 && !IsLoadedAnywhere(name)) {
		    DoImport(name, 0);
		    cb = FindLoadedClass(name, resolve, 0);
                    /*
                    ** If the class was found, then clear its CCF_IsResolving
                    ** bit since it is fully initialized now...
                    */
                    if (cb) {
                        sysAssert( CCIs(cb, Resolving) );
                        CCClear(cb, Resolving);
                    }
		}
    }
    return cb;
}

JRI_PUBLIC_API(ClassClass *)
FindClassFromClass(struct execenv *ee, char *name, bool_t resolve, ClassClass *from) {
    ClassClass *cb;
    lock_classes();
    cb = Locked_FindClassFromClass(ee, name, resolve, from);
    unlock_classes();
    return cb;
}

static ClassClass *
FindClassLoaderArrayClass(struct execenv *ee, char *name, 
			  bool_t resolve, ClassClass *from)
{
    ClassClass *cb;
    char *ptr;
    HObject *loader = from->loader;
    if ((cb = FindLoadedClass(name, resolve, loader)) != NULL)
	return cb;
    for (ptr = name; *ptr == SIGNATURE_ARRAY; ptr++)
        ;
    if (*ptr != SIGNATURE_CLASS) {
	return NULL;
    } else {
	ClassClass *inner_cb;
	char buffer[256];
        int name_len;

	jio_snprintf(buffer, sizeof(buffer), "%s", ptr+1);  /* XXX use strncpy() */
        name_len = strlen(buffer) - 1;
        if (name_len <= 0 || buffer[name_len] != ';') 
          return NULL;
        buffer[name_len] = '\0';  /* Delete trailing ';' */

	inner_cb = FindClassFromClass(ee, buffer, FALSE, from);
	if (inner_cb == NULL || inner_cb->loader != loader) 
	    return NULL;
	cb = (ClassClass *)createFakeArrayClass(name);
	cb->loader = loader;
	AddBinClass(cb);
	/* this initializes */
        cb = FindLoadedClass(name, resolve, loader) ;
        if (cb) {
            /*
            ** If the class was found, then clear its CCF_IsResolving
            ** bit since it is fully initialized now...
            */
            sysAssert(CCIs(cb, Resolving));
            CCClear(cb, Resolving);
        }
        return cb;
    }
}	

/* Nothing outside this file should rely on the format of IDs.
 * internalIDHash and classLoaderForType are private. */

/* Note: previously this was defined in classinitialize.c */
struct StrIDhash *interfaceHash; /* allocated on first use */

static HObject * classLoaderForType(char *type, ClassClass *from);

/*-
 * Return a unique ID for a given name and type. If 'from' is null, treat
 * argument and result classes as being local. Otherwise, they will be
 * found in the context of the class 'from'.
 * Calling this function with the wrong value of 'from' is insecure.
 *
 * Note: despite the function name, this is not a hash - it is an ID.
 *
 * XXX M12N: This function is not part of the JRI (the equivalents are
 * JRI_GetMethodID and JRI_GetFieldID). We need to remove any calls to it
 * from AWT, etc.
 */
JRI_PUBLIC_API(hash_t)
NameAndTypeToHash(char *name, char *type, ClassClass *from)
{
    /* This is a little ugly. There is a lot of code in util.c for
     * interning strings, that we want to re-use. For each string it
     * returns a 16-bit integer.
     *
     * When only the name and type needed to be considered in creating
     * the ID, the resulting two 16-bit integers could fit in 32 bits.
     * Now, we have to consider a name, type, and ClassLoader. So we
     * do something like this:
     *
     * string-to-id(name + " " + type) << 16 +
     *     string-to-id(long-to-hex-string((long)classLoaderForType(from)))
     *
     * Because we garbage-collect ClassLoaders, the address of a ClassLoader
     * may be re-used. This is OK (I think) since by that time there are
     * no remaining classes that were loaded by the ClassLoader.
     */

    /* A pointer converted to hex is at most 16 characters (worst case is a
     * 64-bit system). */
    char loader_id[17];
    char * nameandtype;
    hash_t classloader_key;
    hash_t nameandtype_key;
    HObject * classloader;

#ifdef DEBUG_newids
    if (from && cbLoader(from))
        fprintf(stderr, "method %s %s from %p\n", name, type, from);
#endif
    classloader = classLoaderForType(type, from);

    jio_snprintf(loader_id, sizeof(loader_id), "%lX", (long)classloader);

    nameandtype = sysMalloc(strlen(name)+strlen(type)+2);
        /* +2 is for space and NUL */
    if (!nameandtype) {
        out_of_memory();
        /* NOT REACHED */ return 0;
    }
    strcpy(nameandtype, name);
    /* space is illegal in method names and types.
     * This is enforced by the verifier for untrusted classes (it is not
     * checked in calls to execute_java_* or the JRI). */
    strcat(nameandtype, " ");
    strcat(nameandtype, type);

    classloader_key = Str2ID(&interfaceHash, loader_id, 0, TRUE);
    nameandtype_key = Str2ID(&interfaceHash, nameandtype, 0, TRUE);
    sysFree(nameandtype);

    return (((unsigned long)classloader_key) << 16) + nameandtype_key;
}

/*-
 * Return the name and type for a given ID, in UTF8 format, separated by a
 * space. This is needed for error and debugging messages.
 * The string will be valid for at least as long as the corresponding
 * ClassLoader.
 */
JRI_PUBLIC_API(char *)
IDToNameAndTypeString(hash_t ID) {
    return ID2Str(interfaceHash, ID & 0xFFFF, 0);
}

/*-
 * Return the type for a given ID, in UTF8 format. The JIT compiler needs this
 * in order to find out how to call a method.
 * The string will be valid for at least as long as the corresponding
 * ClassLoader.
 */
JRI_PUBLIC_API(char *)
IDToType(hash_t ID) {
    char *nameandtype = ID2Str(interfaceHash, ID & 0xFFFF, 0);
    char *space = strchr(nameandtype, ' ');
    sysAssert(space); /* string must contain a space */
    return space+1;
}

static HObject * classLoaderForType(char *type, ClassClass *from) {
    int bufsize = 10;      /* half of the initial size of classname buffer */
    char *classname = 0;   /* the name of an argument or result */
    ClassClass *clazz;     /* ClassClass for an argument or result */
    ExecEnv *ee;           /* the current execution environment */
    char *p, *start, *end;

    /* 0 indicates that we already know this is universal. */
    if (from == 0)
        return 0;

    /* Local classes can only access universal methods/fields. */
    if (cbLoader(from) == 0)
        return 0;

    ee = EE();

    /* Check whether any of the types mentioned have ClassLoaders.
     * We ignore array markers (for this to work, an array of objects must
     * always have the same ClassLoader as the innermost element class).
     *
     * We don't actually need any of the argument or result classes - we
     * only need to know whether they are local - but there is no API for
     * that now.
     */
    for (p = type; *p != 0; p++) {
        if (*p == SIGNATURE_CLASS) {
            /* remember where the class name starts */
            start = p+1;

            /* skip to the terminating semicolon */
	    for (; *p != ';'; p++) {
                sysAssert(*p != 0);
            }
            end = p;

            /* start is the first character of the class name.
             * end is the first character after it.
             * Make sure we have enough room for the name. */
            if (!classname || end-start >= bufsize) {
                bufsize *= 2;
                if (end-start >= bufsize)
                    bufsize = end-start+1;
                if (classname)
                    sysFree(classname);
                classname = sysMalloc(bufsize);
                if (!classname) {
                    out_of_memory();
                    /* NOT REACHED */ return 0;
		}
	    }
            strncpy(classname, start, end-start);
            classname[end-start] = 0; /* terminate with NUL */
#ifdef DEBUG_newids
            fprintf(stderr, "  class %s", classname);
#endif
            /* FALSE means don't try to resolve the class. */
#if 0 /* XXX correct code  that *could* support wild class-loaders is removed  */
	    clazz = FindClassFromClass(ee, classname, FALSE, from);
#else /* XXX hack to avoid recursion on the class-lookup */
            clazz = FindClassFromClass(ee, classname, FALSE, 0 );
#endif /* 0 */
            if (!clazz) {
#ifdef DEBUG_newids
                fprintf(stderr, " NOT FOUND\n");
#endif
		if (classname)
		    sysFree(classname);
                return cbLoader(from);
            }
#if 0 /* XXX while above hack is in place */
            if (cbLoader(clazz)) {
                /* One of the arguments or result has a ClassLoader, which
                 * must be the same as cbLoader(from). */
                sysAssert(cbLoader(clazz) == cbLoader(from));
#ifdef DEBUG_newids
                fprintf(stderr, " has loader %p\n", cbLoader(clazz));
#endif
		if (classname)
		    sysFree(classname);
                return cbLoader(from);
	    }
#else
	    sysAssert(!cbLoader(clazz)); /* XXX only while above hack is in place*/
#endif /* 0 */

#ifdef DEBUG_newids
	    fprintf(stderr, " is local\n");
#endif
            continue; /* for */
        }
        /* *p != SIGNATURE_CLASS */
        sysAssert(strchr("()[VZBCSIJFD", *p));
    } /* for */

    if (classname)
	sysFree(classname);

    /* No type loaded by a ClassLoader was found, so this must be a universal
     * method. */
    return 0;
}
