/*
 * @(#)classloader.c	1.79 95/11/16  
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

#include <string.h>
#include <sys/types.h>
#include <stddef.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <setjmp.h>

#include "oobj.h"
#include "path.h"
#include "interpreter.h"
#include "tree.h"
#include "bool.h"
#include "signature.h"

#ifdef XP_MAC
#include "sysmacros_md.h"
#endif

#include "sys_api.h"
#include "byteorder_md.h"
#include "io_md.h"

#include "prlog.h"
PR_LOG_DEFINE(Load);

int verbose_class_loading;

int LoadFile(char *fn, char *dir, char *SourceHint);
int DoImport(char *name, char *SourceHint);
char *stat_source(ClassClass *cb, struct stat *s, char *pathbuf, int maxlen);

/* XXX: Should move declaration to interpreter.h... and include that header file */
extern ClassClass * FindPreviouslyLoadedClass(char *name, HObject *loader);


void
AddBinClass(ClassClass * cb)
{
    int32_t i;
    ClassClass **tcb = 0;

    /* 
    First phase worries about compacting the binclass list, since
    the GC *could* put a hole in the list.
    Better would be to mark the need for a compaction, and then really compact
    when a lock is acquired (i.e., remove all holes).
    Current code *looks* for holes when placing system classes.. which takes
    lots of time when there are no holes.  :-(
    Worse yet, it only looks for holes for placing system classes... which means most
    holes don't get filled (there are only so many system classes)... and the array 
    grows endlessly ('cause it always grows as a non-system class is added).
    */
    if (cb->loader == 0) {
	for (i = nbinclasses; --i >= 0; ) {
            if (!binclasses[i]) {
                tcb = &binclasses[i];
                break;
            }
#if 0
            /* XXX why did they do this? */
            if (strcmp(classname(binclasses[i]), classname(cb)) == 0) {
                tcb = &binclasses[i];
                printf("Duplicate class name found: %s\n", classname(cb));
            }
#endif
        }
    }

    /* Second phase: If compacting insertion can't be done, just append */
    if (!tcb) {
	if (nbinclasses >= sizebinclasses)
	    if (binclasses == 0)
		binclasses = (ClassClass **)
		    sysMalloc(sizeof(ClassClass *) * (sizebinclasses = 50));
	    else
		binclasses = (ClassClass **)
		    realloc(binclasses, sizeof(ClassClass *)
			    * (size_t)(sizebinclasses = nbinclasses * 2));
	if (binclasses == 0) {
		SignalError(EE(), JAVAPKG "OutOfMemoryError", NULL);
	    return;
        }
	tcb = &binclasses[nbinclasses++];
    }

    *tcb = cb;

    /*
    ** Set the CCF_IsResolving bit on the class to prevent it from being 
    ** GCed during class initialization.  This is necessary, because at
    ** various points during the initialization process there are NO
    ** references to the new class where the GC looks...
    **
    ** Once the class has been fully initialized, the CCF_IsResolving bit
    ** is cleared.
    */
    sysAssert( !CCIs(cb, Resolving) );
    CCSet(cb, Resolving);

    /* printf("Loaded %s %d %s with %s   %p\n",
                   cb->loader ? " " : "*", 
                      tcb-binclasses, 
                         (tcb != &binclasses[nbinclasses-1]) ? "Hole" : "    ",
                                 classname(cb),
                                      cb->loader); */
}

void
DelBinClass(ClassClass * cb)
{
    register int32_t i;
    for (i = nbinclasses; --i >= 0; )
	if (binclasses[i] == cb) {
	    if (verbose_class_loading)
		fprintf(stderr, "Deleting class %s #%ld\n", classname(cb), i);
	    binclasses[i] = binclasses[--nbinclasses];
	    break;
	}
}

/* LoadFile is shared by the compiler and the interpreter.  It does
   different things for each.

   When the compiler askes to load a file for a particular class,
   in the end all it's looking for is a classinfo structure to be
   created for that class.  It can get that either by loading a
   .class file, and creating the classinfo from that, OR, by
   reading a source file and compiling it.

   When the interpreter askes for a file, in the end it's looking
   for a classblock structure to be created.  The only way it can
   get one of those is by loading a compiled class.

   OpenCode tries to open a .class file first.  -2 means it failed
   because it couldn't open the file, -1 means the requested file
   is being compiled.  Of course, in the case of the interpreter,
   OpenCode never returns -1, only a valid fd or -2.

   If OpenCode returns a valid fd, that means it is a .class file.
   If SkipCheckSource is false (meaning we're checking source
   files), and the source file as specified in the compiled .class
   file is more recent than the .class file, then the loaded class
   is thrown away, and OpenCode is called again, only this time,
   with arguments which will cause it to recompile the source.  In
   the case of java, it forks javac to do this.  When this happens,
   it has to reload the resulting .class file.

   Returns 1 when it succeeds, 0 when it fails. */

extern int OpenCode(char *, char *, char *, struct stat*);
int
LoadFile(char *fn, char *dir, char *SourceHint)
{
    struct stat st;
    ClassClass *cb = 0;
    int codefd = -1;
    int	retryCount = 0;
    unsigned char *external_class;

    codefd = OpenCode(fn, SourceHint, dir, &st);

again:
    switch (codefd) {
      case -2:		    /* open failed */
	return 0;

      case -1:
	return 1;       /* already compiled (or compiling) source */
    }

    /* Snarf the file into memory. */
    external_class = (unsigned char *)sysMalloc(((size_t)st.st_size));
    if (external_class == 0)
        goto failed;
    if (sysRead(codefd, (char *)external_class, st.st_size) != st.st_size)
        goto failed;
    sysClose(codefd);
    codefd = -1;
    cb = AllocClass();
    if (!createInternalClass(external_class, external_class + st.st_size, cb)) {
	PR_LOG(Load, error, ("Can't load %s.", fn));
	sysFree(external_class);
	goto failed;
    }
    sysFree(external_class);
    /* Valid class - let's put it in the class table. */
    AddBinClass(cb);
    
    /* Now, if we're checking sources, and there's a source file
       name in the .class file we just loaded, check to see if we
       should recompile this class.  If so, call try again. */

    if (!SkipSourceChecks && cb->source_name) {
	struct stat st2;
	char pathbuf[255];

	if (cb->major_version != JAVA_VERSION) {
	    fprintf(stderr, "Warning: Not using %s because it was\n"
		    "\t compiled with version %d.%d of javac.  "
		    "Current version is %d.%d.\n",
		    classname(cb),
		    cb->major_version, cb->minor_version, 
		    JAVA_VERSION, JAVA_MINOR_VERSION);
	}
	if (stat_source(cb, &st2, pathbuf, sizeof(pathbuf)) != 0
	    && (st2.st_mtime > st.st_mtime || cb->major_version != JAVA_VERSION)
	    && ++retryCount < 2) {
	    if (cb->major_version != JAVA_VERSION) {
		fprintf(stderr, "Warning: Attempting to recompile from %s...\n",
			classsrcname(cb));
		unlink(fn);
	    }
	    /* remove the class we just defined - we're going again;  */
	    codefd = OpenCode(fn, pathbuf, dir, &st);
	    DelBinClass(cb);
	    goto again;
	} else if (cb->major_version != JAVA_VERSION) {
	    /* remove the class we just defined - we're going
	       to try again (once) */
	    DelBinClass(cb);
	    goto failed;
	}
    }
#ifdef DEBUG
    if (verbose) {
	fprintf(stderr, "[Loaded %s]\n", fn);
    } else if (verbose_class_loading) {
	fprintf(stderr, "Loaded %s\n", fn);
    }
#endif
    return 1;
failed:
    if (codefd >= 0)
	sysClose(codefd);
    return 0;
}

int
LoadFileZip(char *fn, zip_t *zip)
{
    struct stat st;
    ClassClass *cb;
    unsigned char *clazz;

    if (!zip_stat(zip, fn, &st)) {
		return 0;
    }
    if ((clazz = sysMalloc((size_t)st.st_size)) == 0) {
		return 0;
    }
    if (!zip_get(zip, fn, clazz, st.st_size)) {
	sysFree(clazz);
		return 0;
    }
    if ((cb = AllocClass()) == 0) {
		sysFree(clazz);
		return 0;
    }
    if (!createInternalClass(clazz, clazz + st.st_size, cb)) {
		sysFree(clazz);
		/* sysFree(cb); */
		return 0;
    }
    sysFree(clazz);
    AddBinClass(cb);
#ifdef DEBUG
    if (verbose) {
	fprintf(stderr, "[Loaded %s from %s]\n", fn, zip->fn);
    } else if (verbose_class_loading) {
	fprintf(stderr, "ZipLoaded %s #%ld\n", fn, nbinclasses-1);
    }
#endif
    return 1;
}

int 
DoImport(char *name, char *SourceHint)
{
    cpe_t **cpp;

    if (name[0] == SIGNATURE_ARRAY) {
	AddBinClass(createFakeArrayClass(name));
	return 1;
    }
    if (name[0] == DIR_SEPARATOR) {
	return 0;
    }
    for (cpp = sysGetClassPath(); cpp && *cpp != 0; cpp++) {
	cpe_t *cpe = *cpp;
	char path[255];
	if (cpe->type == CPE_DIR) {
	    if (jio_snprintf(path, sizeof(path), "%s%c%s." JAVAOBJEXT, cpe->u.dir,
			 LOCAL_DIR_SEPARATOR, name) == -1) {
		return 0;
	    }
	    if (LoadFile(sysNativePath(path), cpe->u.dir, SourceHint)) {
		return 1;
	    }
	} else if (cpe->type == CPE_ZIP) {
	    if (jio_snprintf(path, sizeof(path), "%s." JAVAOBJEXT, name) == -1) {
		return 0;
	    }
	    if (LoadFileZip(path, cpe->u.zip)) {
		return 1;
	    }
	}
    }
    return 0;
}

char *
stat_source(ClassClass *cb, struct stat *s, char *pathbuf, int maxlen)
{
#define NAMEBUFLEN 255
    char nm[NAMEBUFLEN];
    char *p, *q, *lp;
    cpe_t **cpp;

    /* don't bother searching if absolute */
    /* REMIND: only here for compatibility */
    if (sysIsAbsolute(classsrcname(cb))) {
	if (sysStat(classsrcname(cb), s) == 0) {
	    if (jio_snprintf(pathbuf, maxlen, "%s", classsrcname(cb)) == -1) {
		return 0;
	    }
	    return pathbuf;
	} else {
	    return 0;
	}
    }

    /* parse the package name */
    p = classname(cb);
    if (strlen(p) > NAMEBUFLEN - 1) {
	return 0;
    }
    for (q = lp = nm ; *p ; p++) {
	if (*p == DIR_SEPARATOR) {
	    *q++ = LOCAL_DIR_SEPARATOR;
	    lp = q;
	} else {
	    *q++ = *p;
	}
    }

    /* append the source file name */
    p = classsrcname(cb);
    if (strlen(p) + (lp - nm) > NAMEBUFLEN - 1) {
	return 0;
    }
    for (; *p ; p++) {
	*lp++ = (*p == DIR_SEPARATOR ? LOCAL_DIR_SEPARATOR : *p);
    }
    *lp = '\0';

    /* search the class path */
    for (cpp = sysGetClassPath() ; cpp && *cpp != 0 ; cpp++) {
	cpe_t *cpe = *cpp;
	if (cpe->type == CPE_DIR) {
	    if (jio_snprintf(pathbuf, maxlen, "%s%c%s",
			 cpe->u.dir, LOCAL_DIR_SEPARATOR, nm) == -1) {
		return 0;
	    }
	    if (sysStat(pathbuf, s) == 0) {
		return pathbuf;
	    }
	}
    }
    return 0;
}

struct CICcontext {
    unsigned char *ptr;
    unsigned char *end_ptr;
    ClassClass *cb;
    jmp_buf jump_buffer;
    char **detail;
};

typedef struct CICcontext CICcontext;

static unsigned long get1byte(CICcontext *);
static unsigned long get2bytes(CICcontext *);
static unsigned long get4bytes(CICcontext *);
#define get1byteI8(x)\
	((char)(get1byte(x)))
#define get1byteU8(x)\
	((unsigned char)(get1byte(x)))
#define get2bytesI16(x)\
	((int16_t)(get2bytes(x)))
#define get2bytesU16(x)\
	((uint16_t)(get2bytes(x)))
#define get4bytesI32(x)\
	((int32_t)(get4bytes(x)))
static char *getAsciz(CICcontext *);
static char *getAscizFromClass(CICcontext *, cp_index_type i);
static void ValidateClassName(CICcontext *context, char *name) ;
static void getNbytes(CICcontext *, int32_t count, char *buffer);
static bool_t LoadConstantPool(CICcontext *);
static bool_t ReadInCode(CICcontext *, struct methodblock *);
static bool_t ReadLineTable(CICcontext *, struct methodblock *mb);
static bool_t ReadLocalVars(CICcontext *, struct methodblock *mb);
static bool_t InitializeStaticVar(struct fieldblock *fb, CICcontext *context);


#undef  ERROR		/* Avoid conflict on PC */
#if defined(DEBUG_jar) && defined(XP_UNIX)
#define ERROR(name) { *(context->detail) = name; \
                        printf("classloader.c ERROR: %s\n", name );\
                        longjmp(context->jump_buffer, 1); }
#else
#define ERROR(name) { *(context->detail) = name; \
                        longjmp(context->jump_buffer, 1); }
#endif

bool_t 
createInternalClass(unsigned char *ptr, unsigned char *end_ptr, ClassClass *cb)
{
    int32_t i, j;
    union cp_item_type *constant_pool;
    unsigned char *type_table;
    uint16_t attribute_count;
    unsigned fields_count;
    struct methodblock *mb;
    struct fieldblock *fb;
    struct CICcontext context_block;
    struct CICcontext *context = &context_block;
    char *detail = 0;
    char * expected_name = cb->name;

    cb->name = NULL;   /* avoid dangling pointers */

    context->ptr = ptr;
    context->end_ptr = end_ptr;
    context->cb = cb;
    context->detail = &detail;
    if (setjmp(context->jump_buffer)) {
	/* Indicate an error of some sort */
	PR_LOG(Load, error, ("%s", *(context->detail)));
	return FALSE;
    }
    
    if (get4bytes(context) != JAVA_CLASSFILE_MAGIC) 
	ERROR("Bad magic number");

    cb->minor_version = get2bytesU16(context);
    cb->major_version = get2bytesU16(context);
    if (cb->major_version != JAVA_VERSION) 
	ERROR("Bad major version number");

    if (!LoadConstantPool(context)) {
 	SignalError(EE(), JAVAPKG "OutOfMemoryError", NULL);
  	return FALSE;
    }
    constant_pool = cb->constantpool;
    type_table = constant_pool[CONSTANT_POOL_TYPE_TABLE_INDEX].p;

    cb->access = get2bytesU16(context) & ACC_WRITTEN_FLAGS;

    /* Get the name of the class */
    i = get2bytesU16(context);	/* index in constant pool of class */
    cb->name = getAscizFromClass(context, i);
    if (expected_name && strcmp(expected_name, cb->name)) {
            /* printf("Mismatch: Inner name: %s, file name %s\n", cb->name, expected_name); */
            ERROR("Internal/External class name mismatch");
    }
    ValidateClassName(context, cb->name);

    if (FindPreviouslyLoadedClass(cb->name, cb->loader)) {
            ERROR("Class already loaded");  /* internal bug (unicode? race?) + (probable) security attack */
    }

    constant_pool[i].p = cb;
    CONSTANT_POOL_TYPE_TABLE_SET_RESOLVED(type_table, i);

    /* Get the super class name. */
    i = get2bytesU16(context);	/* index in constant pool of class */
    if (i > 0) {
	cb->super_name = getAscizFromClass(context, i);
        /* printf("%s super class  = %s\n", cb->name, 
               cb->super_name? cb->super_name : "Missing name"); */
        if (cb->super_name)
          ValidateClassName(context, cb->super_name);
    }
    else
      /* should be strcmp cb->name with java/lang/Object?? */
      ;

    i = cb->implements_count = get2bytesU16(context);
    if (i > 0) {
	uint32_t j;
	uint32_t sizeOfAllocation = i * sizeof(short);
	
	PR_ARENA_ALLOCATE(cb->implements, &(cb->classStorageArena->pool), sizeOfAllocation);
	if (cb->implements == NULL) {
	    SignalError(EE(), JAVAPKG "OutOfMemoryError", NULL);
  	    return FALSE;
   	}
   	else {
   	    memset(cb->implements, 0, sizeOfAllocation);
   	}

	for (j = 0; j < i; j++) {
            cp_index_type temp =
              cb->implements[j] = get2bytesI16(context); /* XXX: We should use get2bytesU16 and "implements" should be cp_index_type */
            /* printf(" %s has interface %s\n", cb->name, getAscizFromClass(context, temp)); */
            ValidateClassName(context, getAscizFromClass(context, temp));
	}
    }

    fields_count = cb->fields_count = get2bytesU16(context);
    if (fields_count > 0) { 
 	uint32_t sizeOfAllocation = cb->fields_count * sizeof(struct fieldblock);
 	
 	PR_ARENA_ALLOCATE(cb->fields, &(cb->classStorageArena->pool), sizeOfAllocation);
	if (cb->fields == NULL) {
	    SignalError(EE(), JAVAPKG "OutOfMemoryError", NULL);
  	    return FALSE;
	}
	else {
	    memset(cb->fields, 0, sizeOfAllocation);
	}
	
    }
    for (i = fields_count, fb = cb->fields; --i >= 0; fb++) {
	fieldclass(fb) = cb;
	fb->access = get2bytesU16(context) & ACC_WRITTEN_FLAGS;
	fb->name = getAsciz(context);
	fb->signature = getAsciz(context);
	attribute_count = get2bytesU16(context);
	for (j = 0; j < (int32_t) attribute_count; j++) {
	    char *name = getAsciz(context);
	    int32_t length = get4bytesI32(context);
	    if ((fb->access & ACC_STATIC) 
		 && strcmp(name, "ConstantValue") == 0) {
		if (length != 2) 
		    ERROR("Wrong size for VALUE attribute");
		fb->access |= ACC_VALKNOWN;
		/* we'll change this below */
		fb->u.offset = get2bytesU16(context); 
	    } else {
		getNbytes(context, length, NULL);
	    }
	}
	if (fb->access & ACC_STATIC) {
	    cb->flags |= CCF_HasStatics;
	    if (!InitializeStaticVar(fb, context)) {
		SignalError(EE(), JAVAPKG "OutOfMemoryError", NULL);
  	 	return FALSE;
	    }
	}
    }

    if ((cb->methods_count = get2bytesU16(context)) > 0) {
 	uint32_t sizeOfAllocation = cb->methods_count * sizeof(struct methodblock);
 	
 	PR_ARENA_ALLOCATE(cb->methods, &(cb->classStorageArena->pool), sizeOfAllocation);
	if (cb->methods == NULL) {
	    SignalError(EE(), JAVAPKG "OutOfMemoryError", NULL);
 	    return FALSE;
 	}
	else {
	    memset(cb->methods, 0, sizeOfAllocation);
	}

    }
    cb->classStorageArena->methods = cb->methods;
    cb->classStorageArena->methods_count = cb->methods_count;
    for (i = cb->methods_count, mb = cb->methods; --i >= 0; mb++) {
	fieldclass(&mb->fb) = cb;
	mb->fb.access = get2bytesU16(context) & ACC_WRITTEN_FLAGS;
	mb->fb.name = getAsciz(context);
	mb->fb.signature = getAsciz(context);
	mb->args_size = Signature2ArgsSize(mb->fb.signature) 
	                + ((mb->fb.access & ACC_STATIC) ? 0 : 1);
	attribute_count = get2bytesU16(context);
	for (j = 0; j < (int32_t) attribute_count; j++) {
	    char *name = getAsciz(context);
	    if ((strcmp(name, "Code") == 0) 
		&& ((mb->fb.access & (ACC_NATIVE | ACC_ABSTRACT))==0)) {
			if (!ReadInCode(context, mb)) {
	  		     SignalError(EE(), JAVAPKG "OutOfMemoryError", NULL);
			     return FALSE;
			}
	    } else {
		int32_t length = get4bytes(context);
		getNbytes(context, length, NULL);
	    }
	}
    }

    /* See if there are class attributes */
    attribute_count = get2bytesU16(context); 
    for (j = 0; j < (int32_t) attribute_count; j++) {
	char *name = getAsciz(context);
	int32_t length = get4bytes(context);
	if (strcmp(name, "SourceFile") == 0) {
	    if (length != 2) {
		ERROR("Wrong size for VALUE attribute");
	    }
	    cb->source_name = getAsciz(context);
	} else {
	    getNbytes(context, length, NULL);
	}
    }
    return TRUE;
}

static bool_t InitializeStaticVar(struct fieldblock *fb, CICcontext *context) {
#if defined(HAVE_ALIGNED_DOUBLES)
    Java8 t1, t2;
#endif
    char isig = fb->signature[0];
    if (fb->access & ACC_VALKNOWN) { 
	ClassClass *cb = context->cb;
	cp_index_type index = fb->u.offset; /* set by "VALUE" attribute */
	union cp_item_type *constant_pool = cb->constantpool;
	unsigned char *type_table = 
	      constant_pool[CONSTANT_POOL_TYPE_TABLE_INDEX].p;

	if (index <= 0 || index >= cb->constantpool_count) 
	    ERROR("Bad initial value");
	switch (isig) { 
	    case SIGNATURE_DOUBLE: 
		if (type_table[index] != 
		    (CONSTANT_Double | CONSTANT_POOL_ENTRY_RESOLVED))
		    ERROR("Bad index into constant pool");
		PR_ARENA_ALLOCATE(twoword_static_address(fb), &(context->cb->classStorageArena->pool), (size_t)(2 * sizeof(int32_t)));
		if (twoword_static_address(fb) == NULL)
			return FALSE;
		SET_DOUBLE(t1, twoword_static_address(fb),
			   GET_DOUBLE(t2, &constant_pool[index]));
	    break;

	    case SIGNATURE_LONG: 
		if (type_table[index] != 
		    (CONSTANT_Long | CONSTANT_POOL_ENTRY_RESOLVED))
		    ERROR("Bad index into constant pool");
		PR_ARENA_ALLOCATE(twoword_static_address(fb), &(context->cb->classStorageArena->pool), (size_t)(2 * sizeof(int32_t)));
		if (twoword_static_address(fb) == NULL)
			return FALSE;
		SET_INT64(t1, twoword_static_address(fb),
			  GET_INT64(t2, &constant_pool[index]));
		break;

	    case SIGNATURE_CLASS:
		if (strcmp(fb->signature, "L" JAVAPKG "String;") != 0) {
		    ERROR("Cannot set initial value for object");
		} else if (!ResolveClassStringConstant(cb, index, 0)) { 
		    ERROR("Unable to resolve string");
		} else {
		    *(void **)normal_static_address(fb) = 
			   constant_pool[index].p;
		}
		break;

	    case SIGNATURE_BYTE: 
		if (type_table[index] != 
		    (CONSTANT_Integer | CONSTANT_POOL_ENTRY_RESOLVED))
		    ERROR("Bad index into constant pool");
		*(int *)normal_static_address(fb) = 
		        (signed char)(constant_pool[index].i);
		break;

	    case SIGNATURE_CHAR:
		if (type_table[index] != 
		    (CONSTANT_Integer | CONSTANT_POOL_ENTRY_RESOLVED))
		    ERROR("Bad index into constant pool");
		*(int *)normal_static_address(fb) = 
		        (unsigned short)(constant_pool[index].i);
		break;

	    case SIGNATURE_SHORT:
		if (type_table[index] != 
		    (CONSTANT_Integer | CONSTANT_POOL_ENTRY_RESOLVED))
		    ERROR("Bad index into constant pool");
		*(int *)normal_static_address(fb) = 
		        (signed short)(constant_pool[index].i);
		break;
	    case SIGNATURE_BOOLEAN:
		if (type_table[index] != 
		    (CONSTANT_Integer | CONSTANT_POOL_ENTRY_RESOLVED))
		    ERROR("Bad index into constant pool");
		*(int *)normal_static_address(fb) = 
		       (constant_pool[index].i != 0);
		break;

	    case SIGNATURE_INT:  
		if (type_table[index] != 
		    (CONSTANT_Integer | CONSTANT_POOL_ENTRY_RESOLVED))
		    ERROR("Bad index into constant pool");
		*(int32_t *)normal_static_address(fb) = constant_pool[index].i;
		break;
		
	    case SIGNATURE_FLOAT:
		if (type_table[index] != 
		        (CONSTANT_Float | CONSTANT_POOL_ENTRY_RESOLVED))
		    ERROR("Bad index into constant pool");
		*(float *)normal_static_address(fb) = constant_pool[index].f;
		break;
			
	    default: 
		ERROR("Unable to set initial value");
	}
    } else { 
	switch (isig) { 
	    default:
		*(int *)normal_static_address(fb) = 0;
		break;

	    case SIGNATURE_FLOAT:
		*(float *)normal_static_address(fb) = (float)0.0;
		break;

	    case SIGNATURE_LONG: 
		PR_ARENA_ALLOCATE(twoword_static_address(fb), &(context->cb->classStorageArena->pool), (size_t)(2 * sizeof(int32_t)));
		if (twoword_static_address(fb) == NULL)
			return FALSE;
		SET_INT64(t1, twoword_static_address(fb), ll_zero_const);
		break;

	    case SIGNATURE_DOUBLE: 
		PR_ARENA_ALLOCATE(twoword_static_address(fb), &(context->cb->classStorageArena->pool), (size_t)(2 * sizeof(int32_t)));
		if (twoword_static_address(fb) == NULL)
			return FALSE;
		SET_DOUBLE(t1, twoword_static_address(fb), 0.0);
		break;

	    case SIGNATURE_CLASS:
		*normal_static_address(fb) = 0;
		break;
	}
    }
    PR_CompactArenaPool(&(context->cb->classStorageArena->pool));
    return TRUE;
}




static bool_t LoadConstantPool(CICcontext *context) 
{
    ClassClass *cb = context->cb;
    uint16_t nconstants = get2bytesU16(context);
    unsigned char *initial_ptr = context->ptr;
    cp_item_type *constant_pool;
    unsigned char *type_table;
    char *malloced_space;
    uint16_t malloc_size;
    uint32_t sizeOfAllocation;
    uint16_t i;
#if defined(HAVE_ALIGNED_DOUBLES)
    Java8 t1;
#endif
    
    if (nconstants < CONSTANT_POOL_UNUSED_INDEX) {
	ERROR("Illegal constant pool size");
    }
    
    sizeOfAllocation = nconstants * sizeof(cp_item_type);
    PR_ARENA_ALLOCATE(constant_pool, &(context->cb->classStorageArena->pool), sizeOfAllocation);
    if (constant_pool == NULL)
    	return FALSE;
    else
        memset(constant_pool, 0, sizeOfAllocation);
    	
    sizeOfAllocation = nconstants * sizeof(char);
    PR_ARENA_ALLOCATE(type_table, &(context->cb->classStorageArena->pool), sizeOfAllocation);
    if (type_table == NULL)
    	return FALSE;
     else
        memset(type_table, 0, sizeOfAllocation);
   	
    cb->constantpool = constant_pool;
    cb->constantpool_count = nconstants;

    constant_pool[CONSTANT_POOL_TYPE_TABLE_INDEX].p = type_table;
    for (malloc_size = 0, i = CONSTANT_POOL_UNUSED_INDEX; i < nconstants; i++) {
	int type = get1byteI8(context);
	switch (type) {
	  case CONSTANT_Utf8: {
	      uint16_t length = get2bytesU16(context);
	      malloc_size += length + 1;
	      getNbytes(context, length, NULL);
	      break;
	  }
	    
	  case CONSTANT_Class:
	  case CONSTANT_String:
	      getNbytes(context, 2, NULL);
	      break;

	  case CONSTANT_Fieldref:
	  case CONSTANT_Methodref:
	  case CONSTANT_InterfaceMethodref:
	  case CONSTANT_NameAndType:
	  case CONSTANT_Integer:
	  case CONSTANT_Float:
	      getNbytes(context, 4, NULL);
	      break;

	  case CONSTANT_Long:
	  case CONSTANT_Double: 
	      i++;
	      getNbytes(context, 8, NULL);
	      break;

	  default: 
	      ERROR("Illegal constant pool type");
	}
    }

    PR_ARENA_ALLOCATE(malloced_space, &(context->cb->classStorageArena->pool), (size_t)malloc_size);
    if (malloced_space == NULL)
    	return FALSE;
    
    context->ptr = initial_ptr;

    for (i = CONSTANT_POOL_UNUSED_INDEX; i < nconstants; i++) {
	int type = get1byteI8(context);
	CONSTANT_POOL_TYPE_TABLE_PUT(type_table, i, type);
	switch (type) {
	  case CONSTANT_Utf8: {
	      uint16_t length = get2bytesU16(context);
	      char *result = malloced_space;
	      malloced_space += (length + 1);
	      getNbytes(context, length, result);
	      result[length] = '\0';
	      constant_pool[i].p = result;
	      CONSTANT_POOL_TYPE_TABLE_SET_RESOLVED(type_table, i);
	      break;
	  }
	
	  case CONSTANT_Class:
	  case CONSTANT_String:
	    constant_pool[i].i = get2bytes(context);
	    break;

	  case CONSTANT_Fieldref:
	  case CONSTANT_Methodref:
	  case CONSTANT_InterfaceMethodref:
	  case CONSTANT_NameAndType:
	    constant_pool[i].i = get4bytes(context);
	    break;
	    
	  case CONSTANT_Integer:
	  case CONSTANT_Float:
	    constant_pool[i].i = get4bytes(context);
	    CONSTANT_POOL_TYPE_TABLE_SET_RESOLVED(type_table, i);
	    break;

	  case CONSTANT_Long:
	  case CONSTANT_Double: {
	      uint32_t high = get4bytes(context);
	      uint32_t low = get4bytes(context);
	      int64_t value;
	      value = ll_add(ll_shl(uint2ll(high), 32), uint2ll(low));
	      SET_INT64(t1, &constant_pool[i], value);
	      CONSTANT_POOL_TYPE_TABLE_SET_RESOLVED(type_table, i);
	      i++;		/* increment i for loop, too */
	      /* Indicate that the next object in the constant pool cannot
	       * be accessed independently.    */
	      CONSTANT_POOL_TYPE_TABLE_PUT(type_table, i, 0);
	      CONSTANT_POOL_TYPE_TABLE_SET_RESOLVED(type_table, i);
	      break;
	  }

	  default:
	      ERROR("Illegal constant pool type");
	}
    }
   	return TRUE;

}



static bool_t ReadInCode(CICcontext *context, struct methodblock *mb)
{
    uint32_t attribute_length = get4bytes(context);
    unsigned char *end_ptr = context->ptr + attribute_length;
    uint16_t attribute_count;
    uint32_t code_length;
    uint32_t i;

    if (context->cb->minor_version <= 2) { 
	mb->maxstack = (uint16_t) get1byte(context);
	mb->nlocals = (uint16_t) get1byte(context);
	code_length = mb->code_length = get2bytes(context);
    } else { 
	mb->maxstack = get2bytesU16(context);
	mb->nlocals = get2bytesU16(context);
	code_length = mb->code_length = get4bytes(context);
    }

    PR_ARENA_ALLOCATE(mb->code, &(context->cb->classStorageArena->pool), code_length);
    if (mb->code == NULL)
        return FALSE;

    getNbytes(context, code_length, (char *)mb->code);
    if ((mb->exception_table_length = get2bytes(context)) > 0) {
	uint32_t exception_table_size = mb->exception_table_length 
	                                  * sizeof(struct CatchFrame);

	PR_ARENA_ALLOCATE(mb->exception_table, &(context->cb->classStorageArena->pool), (size_t)exception_table_size);
	if (mb->exception_table == NULL)
		return FALSE;

	for (i = 0; i < mb->exception_table_length; i++) {
	    mb->exception_table[i].start_pc = get2bytes(context);
	    mb->exception_table[i].end_pc = get2bytes(context);		
	    mb->exception_table[i].handler_pc = get2bytes(context); 
	    mb->exception_table[i].catchType = get2bytesI16(context);
	}
    }
    attribute_count = get2bytesU16(context);
    for (i = 0; i < attribute_count; i++) {
	char *name = getAsciz(context);
	if (strcmp(name, "LineNumberTable") == 0) {
	    if (!ReadLineTable(context, mb))
	    	return FALSE;
	} else if (strcmp(name, "LocalVariableTable") == 0) {
	    if (!ReadLocalVars(context, mb))
	    	return FALSE;
	} else {
	    int32_t length = get4bytes(context);
	    getNbytes(context, length, NULL);
	}
    }
    if (context->ptr != end_ptr) 
	ERROR("Code segment was wrong length");
	return TRUE;
}


static bool_t ReadLineTable(CICcontext *context, struct methodblock *mb)
{
    int32_t attribute_length = get4bytes(context);
    unsigned char *end_ptr = context->ptr  + attribute_length;
    int32_t i;
    if ((mb->line_number_table_length = get2bytes(context)) > 0) {
	struct lineno *ln = NULL;
	PR_ARENA_ALLOCATE(ln, &(context->cb->classStorageArena->pool), (size_t)(mb->line_number_table_length * sizeof(struct lineno)));
	if (ln == NULL)
		return FALSE;
	mb->line_number_table = ln;
	for (i = (int32_t) mb->line_number_table_length; --i >= 0; ln++) {
	    ln->pc = get2bytes(context);
	    ln->line_number = get2bytes(context);
	}
    }
    if (context->ptr != end_ptr)
	ERROR("Line number table was wrong length?");
	return TRUE;
}	

static bool_t ReadLocalVars(CICcontext *context, struct methodblock *mb)
{
    int32_t attribute_length = get4bytes(context);
    unsigned char *end_ptr = context->ptr  + attribute_length;
    int32_t i;
    if ((mb->localvar_table_length = get2bytes(context)) > 0) {
	struct localvar *lv = NULL;
	PR_ARENA_ALLOCATE(lv, &(context->cb->classStorageArena->pool), (size_t)(mb->localvar_table_length * sizeof(struct localvar)));
	if (lv == NULL)
		return FALSE;
	mb->localvar_table = lv;
	for (i = (int32_t) mb->localvar_table_length; --i >= 0; lv++) {
	    lv->pc0 = get2bytes(context);
	    lv->length = get2bytes(context);
	    lv->nameoff = get2bytesI16(context);
	    lv->sigoff = get2bytesI16(context);
	    lv->slot = get2bytes(context);
	}
    }
    if (context->ptr != end_ptr)
	ERROR("Local variables table was wrong length?");
	return TRUE;
}	

unsigned Signature2ArgsSize(char *method_signature)
{
    char *p;
    int args_size = 0;
    for (p = method_signature; *p != SIGNATURE_ENDFUNC; p++) {
	switch (*p) {
	  case SIGNATURE_BOOLEAN:
	  case SIGNATURE_BYTE:
	  case SIGNATURE_CHAR:
	  case SIGNATURE_SHORT:
	  case SIGNATURE_INT:
	  case SIGNATURE_FLOAT:
	    args_size += 1;
	    break;
	  case SIGNATURE_CLASS:
	    args_size += 1;
	    while (*p != SIGNATURE_ENDCLASS) p++;
	    break;
	  case SIGNATURE_ARRAY:
	    args_size += 1;
	    while ((*p == SIGNATURE_ARRAY)) p++;
	    /* If an array of classes, skip over class name, too. */
	    if (*p == SIGNATURE_CLASS) { 
		while (*p != SIGNATURE_ENDCLASS) 
		  p++;
	    } 
	    break;
	  case SIGNATURE_DOUBLE:
	  case SIGNATURE_LONG:
	    args_size += 2;
	    break;
	  case SIGNATURE_FUNC:	/* ignore initial (, if given */
	    break;
	  default:
	    /* Indicate an error. */
	    return 0;
	}
    }
    return args_size;
}


ClassClass*
createFakeArrayClass(char *name) 
{
    char *name_p;
    ClassClass *cb = AllocClass();
    int depth, base_type = 0;
    char buffer_space[256];
    char *buffer = buffer_space;
    cp_item_type *constant_pool = NULL;
    unsigned char *type_table = NULL;
    uint32_t sizeOfAllocation;
    
    sizeOfAllocation = CONSTANT_POOL_ARRAY_LENGTH * sizeof(cp_item_type);
	PR_ARENA_ALLOCATE(constant_pool, &(cb->classStorageArena->pool), sizeOfAllocation);
	if (constant_pool == NULL)
		return NULL;
	else
		memset(constant_pool, 0, sizeOfAllocation);
		
    sizeOfAllocation = CONSTANT_POOL_ARRAY_LENGTH * sizeof(char);
	PR_ARENA_ALLOCATE(type_table, &(cb->classStorageArena->pool), sizeOfAllocation);
	if (type_table == NULL)
		return NULL;
	else
		memset(type_table, 0, sizeOfAllocation);
    
    sysAssert(name[0] == SIGNATURE_ARRAY);

    if (strlen(name) + 1 > sizeof(buffer_space)) { 
	/* don't allow buffer overflow  */
	buffer = sysMalloc((size_t)strlen(name));
    }

    for (name_p = name, depth = 0; *name_p == SIGNATURE_ARRAY; 
	 name_p++, depth++);
    switch(*name_p) {
        case SIGNATURE_INT:    base_type = T_INT; break;
	case SIGNATURE_LONG:   base_type = T_LONG; break;
	case SIGNATURE_FLOAT:  base_type = T_FLOAT; break;
	case SIGNATURE_DOUBLE: base_type = T_DOUBLE; break;
	case SIGNATURE_BOOLEAN:base_type = T_BOOLEAN; break; 
	case SIGNATURE_BYTE:   base_type = T_BYTE; break;
	case SIGNATURE_CHAR:   base_type = T_CHAR; break;
	case SIGNATURE_SHORT:  base_type = T_SHORT; break;
	case SIGNATURE_CLASS: {
	    char *bp;
	    for (bp = buffer, name_p++; /* skip over SIGNATURE_ARRAY */
		 *name_p != SIGNATURE_ENDCLASS; *bp++ = *name_p++);
	    *bp = '\0';
	    base_type = T_CLASS;
	}
    }
    cb->major_version = JAVA_VERSION;
    cb->minor_version = JAVA_MINOR_VERSION;
    cb->access = ACC_FINAL | ACC_ABSTRACT | ACC_PUBLIC;

    sizeOfAllocation = strlen(name) + 1;
	PR_ARENA_ALLOCATE(cb->name, &(cb->classStorageArena->pool), sizeOfAllocation);
	if (cb->name == NULL)
		return NULL;
	else
		strcpy(cb->name, name);

    cb->super_name = JAVAPKG "Object";
    cb->constantpool = constant_pool;
    cb->constantpool_count = CONSTANT_POOL_ARRAY_LENGTH;

    constant_pool[CONSTANT_POOL_TYPE_TABLE_INDEX].p = type_table;
    constant_pool[CONSTANT_POOL_ARRAY_DEPTH_INDEX].i = depth;     
    constant_pool[CONSTANT_POOL_ARRAY_TYPE_INDEX].i = base_type;
    type_table[CONSTANT_POOL_ARRAY_DEPTH_INDEX] = 
	type_table[CONSTANT_POOL_ARRAY_TYPE_INDEX] = 
	    CONSTANT_Integer | CONSTANT_POOL_ENTRY_RESOLVED;
    
    if (base_type == T_CLASS) {
	constant_pool[CONSTANT_POOL_ARRAY_CLASS_INDEX].i = 
	      CONSTANT_POOL_ARRAY_CLASSNAME_INDEX;
	      
    sizeOfAllocation = strlen(buffer) + 1;
	PR_ARENA_ALLOCATE(constant_pool[CONSTANT_POOL_ARRAY_CLASSNAME_INDEX].cp, &(cb->classStorageArena->pool), sizeOfAllocation);
	if (constant_pool[CONSTANT_POOL_ARRAY_CLASSNAME_INDEX].cp == NULL)
		return NULL;
	else
		strcpy(constant_pool[CONSTANT_POOL_ARRAY_CLASSNAME_INDEX].cp, buffer);

	type_table[CONSTANT_POOL_ARRAY_CLASS_INDEX] = CONSTANT_Class;
	type_table[CONSTANT_POOL_ARRAY_CLASSNAME_INDEX] = 
	      CONSTANT_Utf8 | CONSTANT_POOL_ENTRY_RESOLVED;
    } else {
	type_table[CONSTANT_POOL_ARRAY_CLASS_INDEX] = 
	    type_table[CONSTANT_POOL_ARRAY_CLASSNAME_INDEX] = 
		CONSTANT_Integer | CONSTANT_POOL_ENTRY_RESOLVED;
    }
    if (buffer != buffer_space) { 
	/* free buffer, if we malloc'ed it */
	sysFree(buffer);
    } 
    PR_CompactArenaPool(&(cb->classStorageArena->pool));
    return cb;
}

static unsigned long get1byte(CICcontext *context)
{
    unsigned char *ptr = context->ptr;
    if (context->end_ptr - ptr < 1) {
	ERROR("Truncated class file");
    } else {
	unsigned char *ptr = context->ptr;
	unsigned long value = ptr[0];
	(context->ptr) += 1;
	return value;
    }
}

#define U32(x)\
	((uint32_t)x)

static unsigned long get2bytes(CICcontext *context)
{
    unsigned char *ptr = context->ptr;
    if (context->end_ptr - ptr < 2) {
	ERROR("Truncated class file");
    } else {
	unsigned long value = (U32(ptr[0]) << 8) + ptr[1];
	(context->ptr) += 2;
	return value;
    }
}


static unsigned long get4bytes(CICcontext *context)
{
    unsigned char *ptr = context->ptr;
    if (context->end_ptr - ptr < 4) {
	ERROR("Truncated class file");
    } else {
	unsigned long value = 
	    ( U32(ptr[0]) << 24)
	  + ( U32(ptr[1]) << 16)
	  + ( U32(ptr[2]) << 8) 
	  + ptr[3];
#ifdef OSF1
	value &= 0xffffffff;
#endif
	(context->ptr) += 4;
	return value;
    }
}

static void getNbytes(CICcontext *context, int32_t count, char *buffer) 
{
    unsigned char *ptr = context->ptr;
    if (context->end_ptr - ptr < count)
	ERROR("Truncated class file");
    if (buffer != NULL) 
	memcpy(buffer, ptr, (size_t) count);
    (context->ptr) += count;
}


static char *getAsciz(CICcontext *context) 
{
    ClassClass *cb = context->cb;
    union cp_item_type *constant_pool = cb->constantpool;
    cp_index_type nconstants = cb->constantpool_count;
    unsigned char *type_table = constant_pool[CONSTANT_POOL_TYPE_TABLE_INDEX].p;

    cp_index_type value = get2bytesU16(context);
    if ((value == 0) || (value >= nconstants) || 
	   type_table[value] != (CONSTANT_Utf8 | CONSTANT_POOL_ENTRY_RESOLVED))
	ERROR("Illegal constant pool index");
    return constant_pool[value].cp;
}

static char *getAscizFromClass(CICcontext *context, cp_index_type value) 
{
    ClassClass *cb = context->cb;
    union cp_item_type *constant_pool = cb->constantpool;
    cp_index_type nconstants = cb->constantpool_count;
    unsigned char *type_table = constant_pool[CONSTANT_POOL_TYPE_TABLE_INDEX].p;
    if ((value <= 0) || (value >= nconstants) 
	|| (type_table[value] != CONSTANT_Class))
	ERROR("Illegal constant pool index");
    value = constant_pool[value].i;
    if ((value <= 0) || (value >= nconstants) ||
	 (type_table[value] != (CONSTANT_Utf8 | CONSTANT_POOL_ENTRY_RESOLVED)))
	ERROR("Illegal constant pool index");
    return constant_pool[value].cp;
}

static void ValidateClassName(CICcontext *context, char *name) 
{
    if (!name || !name[0] || '[' == name[0]) { 
        /* printf("Validation error on %s\n", name ? name:"No Name"); */
        ERROR("Invalid class name in byte code"); 
    }
}


/* In certain cases, we only need a class's name.  We don't want to resolve
 * the class reference if it isn't, already. 
 */

char *GetClassConstantClassName(cp_item_type *constant_pool, cp_index_type index)
{
    unsigned char *type_table = constant_pool[CONSTANT_POOL_TYPE_TABLE_INDEX].p;
    switch(type_table[index]) {
        case CONSTANT_Class | CONSTANT_POOL_ENTRY_RESOLVED: {
	    ClassClass *cb = constant_pool[index].p;
	    return classname(cb);
	}

        case CONSTANT_Class: {
	    cp_index_type name_index = constant_pool[index].i;
	    return constant_pool[name_index].cp;
	}
	    
	default:
	    return (char *)0;
    }
}
