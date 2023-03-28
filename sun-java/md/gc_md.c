#include "typedefs.h"
#include "oobj.h"
#include "monitor.h"
#include "javaThreads.h"
#include "finalize.h"
#include "prmem.h"
#include "prlog.h"
#include "prmon.h"
#include "prprf.h"
#include "prthread.h"
#include "prarena.h"
#include "prgc.h"
#include "utf.h"
#include "prlink.h"
#if XP_MAC
#include "prmacos.h"
#endif

static void PR_CALLBACK ScanJavaHandle(void GCPTR *obj);
static void PR_CALLBACK FinalJavaHandle(void GCPTR *obj);
static void PR_CALLBACK DumpJavaHandle(FILE *out, void GCPTR *obj, PRBool detailed, int indent);
static void PR_CALLBACK SummarizeJavaHandle(void GCPTR *obj, size_t bytes);

static void PR_CALLBACK ScanClassClass(ClassClass *cb);
static void PR_CALLBACK FreeClassClass(ClassClass *cb);
static void PR_CALLBACK DumpClassClass(FILE *out, void GCPTR *ptr, PRBool detailed, int indent);
static void PR_CALLBACK SummarizeClassClass(void GCPTR *obj, size_t bytes);

extern void ScanVerifyContext(void *ptr);

GCInfo *gcInfo = 0;

extern int verbose_class_loading;

PR_LOG_DEFINE(UnloadClass);
PR_LOG_DEFINE(Allocation);

/*
** GC type's for java. Java uses handles (the GC doesn't) so handles are
** allocated out of the front of an object. All "pointers" in java's are
** actually handle addresses so there normally will be no direct pointers
** to the java object. However, because C code is used to implement java
** sometimes there will be a C pointer into the java object. The GC
** handles this "offset pointer" (because it has to) and can find the
** appropriate handle.
**
** We use 3 types for java objects: java objects which are not scanned
** (things which have no embedded pointers like array's of int's), java
** objects which are scanned but not finalized, and java objects which
** are scanned and finalized.
*/
GCType unscannedType = {
    0,
    0,
    DumpJavaHandle,
    SummarizeJavaHandle,
    0,
};
int unscannedTIX;

GCType scannedType = {
    ScanJavaHandle,
    0,
    DumpJavaHandle,
    SummarizeJavaHandle,
    0,
};
int scannedTIX;

GCType scannedFinalType = {
    ScanJavaHandle,
    FinalJavaHandle,
    DumpJavaHandle,
    SummarizeJavaHandle,
    0,
};
int scannedFinalTIX;

GCType classClassType = {
    (void (*)(void*)) ScanClassClass,
    0,
    DumpClassClass,
    SummarizeClassClass,
    (void (*)(void*)) FreeClassClass,
};
int classClassTIX;

GCType verifyContextType = {
    (void (*)(void*)) ScanVerifyContext,
    0,
    0,
    0,
};
int verifyContextTIX;

static const char *TYPEOF(HObject *h)
{
    static char buf[200];
    static char *tag[] =
    {
	"??? object",
	"??? 1",
	"object",
	"??? 3",

	"boolean",
	"char",
	"float",
	"double",

	"byte",
	"short",
	"int",
	"long",

	"unsigned byte",
	"unsigned short",
	"unsigned int",
	"unsigned long",
    };

    if (h)
    {
	switch (obj_flags(h))
	{
	  case T_NORMAL_OBJECT:
	    return obj_classblock(h)->name;
	  case T_CLASS:
	  {
	      ArrayOfObject *array = (ArrayOfObject *)unhand(h);
	      ClassClass *fromClass = (ClassClass *)(array->body[obj_length(h)]);
	      char *fromName = fromClass->name;

	      sprintf(buf, "%s[%d]", fromName, obj_length(h));
	      return buf;
	  }
	  case T_BOOLEAN:
	  case T_CHAR:
	  case T_FLOAT:
	  case T_DOUBLE:
	  case T_BYTE:
	  case T_SHORT:
	  case T_INT:
	  case T_LONG:
	    sprintf(buf, "%s[%d]", tag[obj_flags(h) & 15], obj_length(h));
	    return buf;
	  default:
	    sprintf(buf, "UNKNOWN KIND %d", obj_flags(h));
	    return buf;
	}
    }
    else
	return "null";
}

#include "prgc.h"

/************************************************************************/

void
RefTable_processRoots(RefTable* refTable)
{
    if (refTable->element) {
#ifdef xDEBUG_warren
	/*
	** Change the above ifdef to see what's left in the global ref
	** tables during gc.
	*/
	void RefTable_describe(RefTable* refTable);
	RefTable_describe(refTable);
#endif /* DEBUG_warren */
        (*gcInfo->processRoot)((void**)refTable->element, refTable->top);
    }
}

/*
** Scan a java thread looking for GC roots. This doesn't scan the java
** thread object (the java thread object is marked by the gc code when
** the nspr thread object's private data areas are processed)
*/
static int PR_CALLBACK ScanJavaThread(PRThread *t, void *notused)
{
    JHandle *p = (JHandle *) sysThreadGetBackPtr(t);/* XXX inline */
    void **ssc, **limit;
    ExecEnv *ee;
    JavaFrame *frame;
    void (*liveObject)(void **base, int32 count);

    PR_ASSERT(p != 0);
    
	/*	Its possible that a thread has exited, and is waiting
		to be killed (its a zombie), but we do a garbage collection.
		In  this case, PrivateInfo will be null, we will throw 
		the following assert, and will exit (bad).  So when
		PrivateInfo is 0, do nothing... the thread is dead. */

    if (THREAD(p)->PrivateInfo == 0)
    	return 1;
    	
    PR_ASSERT(THREAD(p)->PrivateInfo == (long)t);

    PR_PROCESS_ROOT_LOG(("Scanning thread %s", t->name));

    /* Mark thread object */

    ee = (ExecEnv *)THREAD(p)->eetop;
    liveObject = gcInfo->liveObject;
    if (ee != 0) {
	if (((frame = ee->current_frame) != 0)) {
	    struct methodblock *mb = frame->current_method;
	    stack_item *top_top_stack = mb 
		? &frame->ostack[mb->maxstack]
		: frame->optop;
	    PR_ASSERT(frame->optop >= &frame->ostack[0]);
	    if (mb) {
		PR_ASSERT(frame->optop <= &frame->ostack[mb->maxstack]);
	    }
	    limit = (void **)top_top_stack;
	    for (;;) {
		if (mb) {
		    /* Mark the class that contains the method as busy
		       when we are busy executing code in it */
		    PR_PROCESS_ROOT_LOG(("Marking class %s busy while in method %s", classname(fieldclass(&mb->fb)), fieldname(&mb->fb)));
                    (*liveObject)((void**) &fieldclass(&mb->fb), 1);
		}
                ssc = (void **)(frame->ostack);
                if (ssc < limit) {
                    (*liveObject)(ssc, limit - ssc);
                }
		ssc = (void **)(frame->vars);
		if (ssc) {
		    limit = (void **)frame;
                    if (limit > ssc) {
                        (*liveObject)(ssc, limit - ssc);
                    }
		}
		frame = frame->prev;
		if (!frame) break;
		limit = (void **)(frame->optop);
		mb = frame->current_method;
	    }
	}

	/*
	** Mark the exception object (if any)...
	*/
	if (ee->exception.exc) {
            (*liveObject)((void**)&(ee->exception.exc), 1);
        }

	/*
	** JRI Stuff: Scavenge the global refTable
	*/
	if (ee->globalRefs.element)
	    RefTable_processRoots(&ee->globalRefs);
    }
    return 1;
}

/*
** Root finder to find root's into the GC heap from outside the GC heap.
** This is used to specialy handle the thread execution stack.
*/
void PR_CALLBACK ScanJavaThreads(void *notused)
{
    JHandle *self;

    self = (JHandle *) sysThreadGetBackPtr(sysThreadSelf());
    (void) sysThreadEnumerateOver(ScanJavaThread, (void *) self);
}

/************************************************************************/

/*
** Code for GC scanning verifier objects.
*/

void *AllocContext(size_t size)
{
    void *cx;
    cx = (void*) PR_AllocMemory(size, verifyContextTIX, PR_ALLOC_CLEAN);
    return cx;
}

/************************************************************************/

extern int class_lock_depth;

/*
** Scan an instance of ClassClass.
*/
void PR_CALLBACK ScanClassClass(ClassClass *cb)
{
    int32_t i;
    void (*liveObject)(void **base, int32 count);
    struct fieldblock *fb;

    liveObject = gcInfo->liveObject;

    if (CCIs(cb, Resolved)) {
        union cp_item_type *constant_pool = cbConstantPool(cb);
        union cp_item_type *cpp = constant_pool+ CONSTANT_POOL_UNUSED_INDEX;
        union cp_item_type *end_cpp = &constant_pool[cb->constantpool_count];
        unsigned char *type_tab = (unsigned char *)
            constant_pool[CONSTANT_POOL_TYPE_TABLE_INDEX].p;

        PR_PROCESS_ROOT_LOG(("Scanning class %s constant pool",
                             classname(cb)));
        for (i = CONSTANT_POOL_UNUSED_INDEX; cpp < end_cpp; cpp++, i++) {
            ClassClass *cb2;
            switch (type_tab[i]) {
              case CONSTANT_String:
              case CONSTANT_String|CONSTANT_POOL_ENTRY_RESOLVED:
                PR_PROCESS_ROOT_LOG(("Constant pool slot %d: String %p",
                                     i, (*cpp).p));
                (*liveObject)(&(*cpp).p, 1);
                break;
              case CONSTANT_Class|CONSTANT_POOL_ENTRY_RESOLVED:
                cb2 = (ClassClass*) (*cpp).p;
                if (cb2 != cb) {
                    PR_PROCESS_ROOT_LOG(("Constant pool slot %d: Class %s",
                                         i, classname(cb2)));
                    (*liveObject)(&(*cpp).p, 1);
                }
                break;
              case CONSTANT_Long|CONSTANT_POOL_ENTRY_RESOLVED:
              case CONSTANT_Double|CONSTANT_POOL_ENTRY_RESOLVED:
                i++;
                cpp++;
                break;
            }
        }
    }

    /* Scan class definitions looking for statics */
    for (i = (int32_t) cb->fields_count, fb = cbFields(cb); --i >= 0; fb++) {
        if ((fieldIsArray(fb) || fieldIsClass(fb))
            && (fb->access & ACC_STATIC)) {
            PR_PROCESS_ROOT_LOG(("Scanning class %s static %s",
                                 classname(cb), fieldname(fb)));
            (*liveObject)((void**) normal_static_address(fb), 1);
        }
    }

    /* Scan superclass, HandleToSelf, loader and classname_array */
    (*liveObject)((void**) &cb->superclass, 4);
}

ClassArena *free_class_arenas;

extern void CompilerFreeMethod(struct methodblock *mb);

void FreeClassArenas(void) {
    ClassArena *arena;

    while (free_class_arenas) {
	/* Steal away one of the free arenas (under the cover of the GC lock) */
	PR_EnterMonitor(gcInfo->lock);
	arena = free_class_arenas;
	if (arena) {
	    free_class_arenas = arena->next;
	}
	PR_ExitMonitor(gcInfo->lock);

	if (arena) {
	    int i;

	    /* Free up JIT memory */
	    for (i = 0; i < arena->methods_count; i++) {
		struct methodblock* mb = &arena->methods[i];
		CompilerFreeMethod(mb);
		/* Free native method libraries too */
		if (mb->lib != NULL) {
		    int s = PR_UnloadLibrary(mb->lib);
		    PR_ASSERT(s == 0);
		}
	    }

	    /* Now free it */
	    PR_FinishArenaPool(&arena->pool);
	    free(arena);
	}
    }
}


#if XP_MAC
unsigned char ClassArenaFlusherProc(size_t size)
{
        FreeClassArenas();
        return 0;	/* We don't know how much we freed */
}
#endif

ClassClass *AllocClass(void)
{
    ClassClass *cb;
    ClassArena *arena;

    /* Free up any unloaded classes arenas */
    FreeClassArenas();

    cb = (ClassClass*) PR_AllocMemory(sizeof(ClassClass), classClassTIX,
				      PR_ALLOC_CLEAN);
    
    if (cb != NULL) {
	arena = PR_NEWZAP(ClassArena);
	if (!arena) {
	    return 0;
	}
	PR_InitArenaPool(&arena->pool, "class pool", 1 * 512, sizeof(void*));
	cb->classStorageArena = arena;
    }
    return cb;
}

/*
** Prepare classes for GC'ing. This is run before any pointers are
** examined.
*/
void PR_CALLBACK PrepareClassesForGC(void)
{
    ClassClass **pcb;
    int j;

    /* Clear the instance counting stuff */
    pcb =  binclasses;
    for (j = nbinclasses; --j >= 0; pcb++) {
	ClassClass *cb = *pcb;
        if (!cb) continue;
	cb->instances = 0;
    }
}

/*
** Scan class table for GC roots. Only sticky classes constitute roots.
*/
void PR_CALLBACK ProcessRootsInClassTable(void *notused)
{
    ClassClass **pcb;
    int j;
    void (*liveObject)(void **base, int32 count);

    liveObject = gcInfo->liveObject;

    /* Clear the instance counting stuff */
    pcb =  binclasses;
    for (j = nbinclasses; --j >= 0; pcb++) {
	ClassClass *cb = *pcb;
        if (!cb) continue;
        if ((cb->flags & (CCF_Sticky | CCF_IsResolving)) ||
            (!cb->loader && (cb->flags & CCF_HasStatics))) {
            /*
            ** Make a root reference to everything that a sticky class
            ** refers to. We also make references to anything that has
            ** static variables (that doesn't have a class loader).
            **
            ** XXX the unloading policy is currently in design review!
            */
            for (;;) {
                (*liveObject)((void**) &cb, 1);
                if (!cbSuperclass(cb))
                    break;
                cb = unhand(cbSuperclass(cb));
            }
        }
    }
}

/*
** This is called by the GC for a ClassClass that has no references and
** is ready to be released.
**
** NOTE: this is called during the sweep cycle so no GC operations are
** allowed to be performed. When this call returns the memory referred to 
** by "cb" must no longer be referenced (which means we need to carefully 
** remove it from the binclasses table).
**
** NOTE 2: because we are conservatively collecting, we know that there
** are no other references to the cb anywhere, except for the table. This
** means that even if the table is locked it is safe to zero out the cell
** in the table because no thread has dereferenced it (yet).
*/
void PR_CALLBACK FreeClassClass(ClassClass *cb)
{
    ClassClass **pcb, **epcb;

    pcb =  binclasses;
    epcb =  pcb + nbinclasses;
    for (; pcb < epcb; pcb++) {
        if (*pcb == cb) {
	    /* Found a class to unload */
#ifdef DEBUG
            PR_LOG(UnloadClass, out,
		   ("FreeClassClass: %s ( 0x%p )", classname(cb), cb));
            if (verbose_class_loading)
                fprintf(stderr, "Unloaded %s (%x)\n", classname(cb), cb);
#endif
	    /* Rip it out of the table */
            *pcb = 0;

	    /*
	     * Put the indirectly held memory (the arena) on the freelist
	     * of classes. We can't free the indirectly held memory right
	     * now because some other suspended thread may have the malloc
	     * lock (thus causing us to deadlock).
	     */
	    cb->classStorageArena->next = free_class_arenas;
	    free_class_arenas = cb->classStorageArena;
            return;
	}
    }
}

static void PR_CALLBACK DumpClassClass(FILE *out, void GCPTR *ptr, PRBool detailed, int indent)
{
    ClassClass *cb = (ClassClass *) ptr;
    fprintf(out, "Class record <%s>\n", classname(cb));
}

/************************************************************************/

/*
** Scan a java object for pointers to other java objects.
**
** Note: the code below handles a few special cases:
**      (1) the handle has been allocated but not fully initialized
**          because the GC was run before the handle object and methodtable
**          cells were filled in. Either h->obj or h->methodtable can be
**          NULL.
**      (2) the handle was created via AllocHandle. In this case the
**          methodtable will be T_BYTE and the object will be the ClassClass
**          (again, it might not be filled in yet). In the latter case
**          processing the handles object pointer will mark the ClassClass
**          object busy (which is what we want). Later, the class code will
**          change the methodtable pointer for the class to refer to the
**          methods for java.lang.Class at which point the T_NORMAL_OBJECT
**          case in the switch below will be used.
*/
static void PR_CALLBACK ScanJavaHandle(void GCPTR *gch)
{
    JHandle *h = (JHandle *) gch; 
    ClassClass *cb;
    void **sub;
    ClassObject *p;
    void (*liveObject)(void **base, int32 count);
    int32 n;

    liveObject = gcInfo->liveObject;

    (*liveObject)((void**) &h->obj, 1);
    p = unhand(h);

    switch (obj_flags(h)) {
      case T_NORMAL_OBJECT:
        if (obj_methodtable(h)) {
            cb = obj_classblock(h);

            /* Count the direct instance */
            if (cb->instances++ == 0) {
                /* Mark class as alive */
                (*liveObject)((void**) &cb, 1);
            }

            do {
                struct fieldblock *fb = cbFields(cb);

                /* Scan the fields of the class */
                n = cb->fields_count;
                while (--n >= 0) {
                    if ((fieldIsArray(fb) || fieldIsClass(fb))
                        && !(fb->access & ACC_STATIC)) {
                        sub = (void **) ((char *) p + fb->u.offset);
                        (*liveObject)(sub, 1);
                    }
                    fb++;
                }
                if (cbSuperclass(cb) == 0) {
                    break;
                }
                cb = unhand(cbSuperclass(cb));
            } while (cb);
        } else {
            /*
            ** If the object doesn't yet have it's method table, we can't
            ** scan it. This means the GC examined the handle before the
            ** allocation code (realObjAlloc/AllocHandle) has initialized
            ** it.
            */
        }
        break;

      case T_CLASS:             /* an array of objects */
        /*
        ** Have the GC scan all of the elements and the ClassClass*
        ** stored at the end.
        */
        n = obj_length(h) + 1;
        (*liveObject)((void**)((ArrayOfObject *) p)->body, n);
        break;
    }
}

/************************************************************************/

ExecEnv java_finalizer_ee;

/*
** Finalize a java handle and it's object. Called by the nspr finalizer
** thread.
*/
static void PR_CALLBACK FinalJavaHandle(void GCPTR *gch)
{
    JHandle GCPTR *h = (JHandle GCPTR *) gch;

    if (h->obj != 0) {
	struct methodblock *mb;

	PR_ASSERT(obj_flags(h) == T_NORMAL_OBJECT);
	mb = obj_classblock(h)->finalizer;
	PR_ASSERT(mb != 0);
	GCTRACE(GC_FINAL, ("java final of %p (type=%s)", h, TYPEOF(h)));
	do_execute_java_method(&java_finalizer_ee, h, 0, 0, mb, FALSE);
	if(exceptionOccurred(&java_finalizer_ee)) {
#if 0
	    /* XXX Probably should get treated the same way as static
	       class initializers */
	    exceptionDescribe(&java_finalizer_ee);
#endif
	    exceptionClear(&java_finalizer_ee);
	}
    }
}

/*
** Reflect the nspr finalizer thread into java
*/
void InitializeFinalizerThread()
{
    ClassClass *cb;
    HThread *finalizer;
    struct Classjava_lang_Thread *tid;
    ExecEnv *ee;
    extern ClassClass *Thread_classblock;
    char *name = "Finalizer thread";

    if (java_finalizer_ee.thread) {
	return;
    }

    cb = Thread_classblock;
    finalizer = (HThread*) ObjAlloc(cb, 0);
    if (finalizer == 0) {
	out_of_memory();
    }
    tid = THREAD(finalizer);
    memset((char *)tid, 0, cbInstanceSize(cb));

    InitializeExecEnv(&java_finalizer_ee, (JHandle *)finalizer);

    /* 
    ** Setup linkage between system thread and java thread for the Garbage Collector 
    ** If these pointers are not set the GC will ignore any java stack frames that
    ** may be present in the java_finalizer_ee
    */
    unhand(finalizer)->PrivateInfo = (long) gcInfo->finalizer;
    sysThreadSetBackPtr(gcInfo->finalizer, finalizer);

    /* 
    ** Get the EE from the java thread... This is not the java_finalizer_ee 
    ** because this method is called by the primordial thread during java initialization.
    */
    ee = EE();
    sysAssert(ee != &java_finalizer_ee);

    if (ee->initial_stack == 0) {
	out_of_memory();
    }

    /*
    ** Invoke thread class constructor so that finalizer get's added to
    ** the system thread group
    */
    do_execute_java_method(ee, finalizer, "<init>", "()V", 0, 0);

    PR_ASSERT(unhand(finalizer)->group != 0);

    /* Setup linkage between system thread and java thread */
    unhand(finalizer)->name = MakeString(name, strlen(name));
    unhand(finalizer)->daemon = 1;
#if defined(XP_PC) && !defined(_WIN32)
    unhand(finalizer)->priority = MaximumPriority;
    threadSetPriority(finalizer, MaximumPriority);
#else
    unhand(finalizer)->priority = MinimumPriority;
    threadSetPriority(finalizer, MinimumPriority);
#endif
    /* Point to java dump routine */
    gcInfo->finalizer->dump = sysThreadDumpInfo;
}

void runFinalization(void)
{
    PR_ForceFinalize();
}

/************************************************************************/

static void DumpUnicodeString(FILE *out, const unicode *str, int length)
{
    fputc('\'', out);
    while (length-- > 0)
    {
	unicode ch = *str++;
	if (ch == '\n')
	    fprintf(out, "\\n");
	else if (ch == '\0')
	    fprintf(out, "\\0");
	else if (ch < ' ' || ch > 0x7E)
	    fprintf(out, "\\u%.4X", ch);
	else if (ch == '\'')
	    fprintf(out, "\\'");
	else if (ch == '\\')
	    fprintf(out, "\\\\");
	else
	    fputc(ch, out);
    }
    fputc('\'', out);
}


static void DumpObjectContentsShallow(FILE *out, ClassObject *p, ClassClass *cb, int indent)
{
    struct fieldblock *fb = cbFields(cb);
    long n = cb->fields_count;

    /* Scan the fields of the class */
    while (--n >= 0)
    {
	if (!(fb->access & ACC_STATIC))
	{
	    char fieldKind = fieldsig(fb)[0];
	    char *fp = (char *)p + fb->u.offset;

	    PR_DumpIndent(out, indent);
	    fprintf(out, "+0x%.3X: .%s = ", fb->u.offset, fieldname(fb));
	    switch (fieldKind)
	    {
	      case SIGNATURE_ARRAY:
	      case SIGNATURE_CLASS:
	      {
		  JHandle *v = *(JHandle **)fp;
		  fprintf(out, "<%s> 0x%p", TYPEOF(v), v);
	      }
	      break;
	      case SIGNATURE_BYTE:
		fprintf(out, "<byte> %d", *(int8 *)fp);
		break;
	      case SIGNATURE_SHORT:
		fprintf(out, "<short> %d", *(int16 *)fp);
		break;
	      case SIGNATURE_INT:
		fprintf(out, "<int> %d", *(int32 *)fp);
		break;
	      case SIGNATURE_LONG:
	      {
		  char str[64];
		  ll2str(*(int64 *)fp, str, str + 64);
		  fprintf(out, "<long> %s", str);
	      }
	      break;
	      case SIGNATURE_CHAR:
		fprintf(out, "<char> ");
		DumpUnicodeString(out, (unicode *)fp, 1);
		break;
	      case SIGNATURE_BOOLEAN:
	      {
		  uint8 v = *(uint8 *)fp;
		  fprintf(out, "<boolean> ");
		  switch (v)
		  {
		    case 0:
		      fprintf(out, "false");
		      break;
		    case 1:
		      fprintf(out, "true");
		      break;
		    default:
		      fprintf(out, "%d", v);
		      break;
		  }
	      }
	      break;
	      case SIGNATURE_FLOAT:
		fprintf(out, "<float> %d", *(float *)fp);
		break;
	      case SIGNATURE_DOUBLE:
		fprintf(out, "<double> %d", *(double *)fp);
		break;
	      default:
		fprintf(out, "UNKNOWN FIELD KIND '%c'", fieldKind);
	    }
	    fputc('\n', out);
	}
	fb++;
    }
}


static void DumpObjectContents(FILE *out, JHandle *h, int indent)
{
    ClassObject *p = unhand(h);
    ClassClass *cb = obj_classblock(h);
    ClassClass *superclasses[256];
    int nSuperclasses = 0;

    while (cb && nSuperclasses != 256)
    {
	HClass *s;
	superclasses[nSuperclasses++] = cb;
	s = cbSuperclass(cb);
	if (!s)
	    break;
	cb = unhand(s);
    }
    while (nSuperclasses--)
	DumpObjectContentsShallow(out, p, superclasses[nSuperclasses], indent);
}


static void FormatNextObject(char *out, const void **src)
{
    JHandle *elt = *(*(JHandle *const **)src)++;
    if (elt)
	sprintf(out, "%s @ %p", TYPEOF(elt), elt);
    else
	strcpy(out, "null");
}


static void FormatNextByte(char *out, const void **src)
{
    int8 elt = *(*(const int8 **)src)++;
    sprintf(out, "%d", elt);
}


static void FormatNextShort(char *out, const void **src)
{
    int16 elt = *(*(const int16 **)src)++;
    sprintf(out, "%d", elt);
}


static void FormatNextInt(char *out, const void **src)
{
    int32 elt = *(*(const int32 **)src)++;
    sprintf(out, "%ld", elt);
}


static void FormatNextLong(char *out, const void **src)
{
    int64 elt = *(*(const int64 **)src)++;
    ll2str(elt, out, out + 256);
}


static void FormatNextFloat(char *out, const void **src)
{
    float elt = *(*(const float **)src)++;
    sprintf(out, "%g", elt);
}


static void FormatNextDouble(char *out, const void **src)
{
    double elt = *(*(const double **)src)++;
    sprintf(out, "%g", elt);
}


static void DumpArrayEltRange(FILE *out, uint32 startIndex, uint32 stopIndex, const char *str, int indent)
{
    PR_DumpIndent(out, indent);
    stopIndex--;
    if (stopIndex == startIndex)
	fprintf(out, "[%d]", startIndex);
    else
	fprintf(out, "[%d..%d]", startIndex, stopIndex);
    fprintf(out, " = %s\n", str);
}


static void DumpArrayContents(FILE *out, JHandle *h, void (*formatNext)(char *out, const void **src), int indent)
{
    uint32 nElts = obj_length(h);
    const void *p = unhand(h);
    uint32 n = 0;
    int lastEltStrIndex = -1;
    uint32 firstRepeatN = 0;
    char eltStr[2][256];

    while (n != nElts)
    {
	int thisEltStrIndex = !lastEltStrIndex;
	(*formatNext)(eltStr[thisEltStrIndex], &p);
	if (lastEltStrIndex < 0 || strcmp(eltStr[0], eltStr[1]))
	{
	    if (lastEltStrIndex >= 0)
		DumpArrayEltRange(out, firstRepeatN, n, eltStr[lastEltStrIndex], indent);
	    lastEltStrIndex = thisEltStrIndex;
	    firstRepeatN = n;
	}
	n++;
    }
    if (lastEltStrIndex >= 0)
	DumpArrayEltRange(out, firstRepeatN, n, eltStr[lastEltStrIndex], indent);
}


static void DumpArrayHandle(FILE *out, JHandle *h, void (*formatNext)(char *out, const void **src), PRBool detailed, int indent)
{
    fprintf(out, "Array <%s>  handle=0x%p, data=0x%p\n", TYPEOF(h), h, h->obj);
    if (detailed)
	DumpArrayContents(out, h, formatNext, indent+22);
}

static void PR_CALLBACK DumpJavaHandle(FILE *out, void GCPTR *gch, PRBool detailed, int indent)
{
    JHandle *h = (JHandle *) gch;

    if (obj_methodtable(h))
    {
	uint32 flags = obj_flags(h);
	switch (flags)
	{
	  case T_NORMAL_OBJECT:
	    fprintf(out, "Object <%s>  handle=0x%p, data=0x%p, methods=0x%p\n", TYPEOF(h), h, h->obj, h->methods);
	    if (detailed)
		DumpObjectContents(out, h, indent+22);
	    break;
	  case T_CLASS:
	    DumpArrayHandle(out, h, &FormatNextObject, detailed, indent);
	    break;
	  case T_BOOLEAN:
	  case T_BYTE:
	    DumpArrayHandle(out, h, &FormatNextByte, detailed, indent);
	    break;
	  case T_CHAR:
	    fprintf(out, "Array <%s>  handle=0x%p, data=0x%p  ", TYPEOF(h), h, h->obj);
	    DumpUnicodeString(out, unhand((HArrayOfChar*)h)->body, obj_length(h));
	    fputc('\n', out);
	    break;
	  case T_FLOAT:
	    DumpArrayHandle(out, h, &FormatNextFloat, detailed, indent);
	    break;
	  case T_DOUBLE:
	    DumpArrayHandle(out, h, &FormatNextDouble, detailed, indent);
	    break;
	  case T_SHORT:
	    DumpArrayHandle(out, h, &FormatNextShort, detailed, indent);
	    break;
	  case T_INT:
	    DumpArrayHandle(out, h, &FormatNextInt, detailed, indent);
	    break;
	  case T_LONG:
	    DumpArrayHandle(out, h, &FormatNextLong, detailed, indent);
	    break;
	  default:
	    fprintf(out, "UNKNOWN OBJECT  flags=%lu\n", flags);
	    break;
	}
    }
    else
	fprintf(out, "UNFINISHED JAVA OBJECT %p\n", gch);
}

/************************************************************************/

#include "prhash.h"

PRHashTable* summaryTable = NULL;
PRSummaryEntry* summaries = NULL;
int32 summarySize = 0;
int32 summaryIndex = 0;
#define SUMMARY_INCREMENT	1024

/* XXX Move this to prhash? */
PRHashNumber
PR_HashAddress(const void* key)
{
    return (PRHashNumber)key;
}

static void
Summarize(ClassClass* clazz, size_t bytes)
{
    PRSummaryEntry* entry = NULL;

    if (!summaryTable) {
	summaryTable = PR_NewHashTable(1024, PR_HashAddress,
				       PR_CompareValues, PR_CompareValues,
				       NULL, NULL);
	if (!summaryTable) return;
    } else {
	entry = PR_HashTableLookup(summaryTable, clazz);
    }

    if (!entry) {
	PRHashEntry *he;
	if (!summaries) {
	    summarySize = SUMMARY_INCREMENT;
	    summaries = (PRSummaryEntry*)calloc(summarySize, sizeof(PRSummaryEntry));
	    if (!summaries) return;
	    summaryIndex = 0;
	}
	if (summaryIndex == summarySize) {
	    PRSummaryEntry* s =
		(PRSummaryEntry*)realloc(summaries, summarySize + SUMMARY_INCREMENT);
	    if (!s) return;
	    summaries = s;
	    summarySize += SUMMARY_INCREMENT;
	}
	entry = &summaries[summaryIndex++];
	entry->clazz = clazz;
	entry->instancesCount = 0;
	entry->totalSize = 0;
	he = PR_HashTableAdd(summaryTable, clazz, entry);
    }
    PR_ASSERT(entry->clazz == clazz);
    entry->instancesCount++;
    entry->totalSize += bytes;
}

static void PR_CALLBACK 
SummarizeJavaHandle(void GCPTR *obj, size_t bytes)
{
    extern HClass* java_lang_Object_getClass(HObject *p); /* from object.c */
    Summarize(unhand(java_lang_Object_getClass((JHandle*)obj)), bytes);
}

static void PR_CALLBACK 
SummarizeClassClass(void GCPTR *obj, size_t bytes)
{
    Summarize(get_classClass(), bytes);
}

static void PR_CALLBACK
PrintSummary(FILE *out, void* closure)
{
    int32 i;
    fprintf(out, "Count          Total Size    Average Size  Class\n");
    for (i = 0; i < summaryIndex; i++) {
	PRSummaryEntry* entry = &summaries[i];
	fprintf(out, "%12ld   %12ld   %12ld   %s\n", entry->instancesCount,
		entry->totalSize, (entry->totalSize / entry->instancesCount),
		classname((ClassClass*)entry->clazz));
    }
    fprintf(out, "End of memory summary.\n");
    PR_HashTableDestroy(summaryTable);
    summaryTable = NULL;
    free(summaries);
    summaries = NULL;
}

/************************************************************************/

/*
 * Rough counter of heap modification events: a lack of known permu-
 * tations of the heap is used to suppress async GC.  While this isn't
 * a perfect indicator of good times to GC, it is unlikely that there 
 * will be many good times to GC when we aren't turning over memory.
 * Current incrementers of heap_memory_changes include realObjectAlloc()
 * and execute_finalizer().
 */
int heap_memory_changes = 0;

int tracegc = 0;

struct arrayinfo arrayinfo[] = {
    { 0,	    0,			"N/A",          0},
    { 0,	    0,			"N/A",          0},
    { T_CLASS,      SIGNATURE_CLASS,	"class[]",      sizeof(OBJECT)},
    { 0,	    0, 		        "N/A",          0},
    { T_BOOLEAN,    SIGNATURE_BOOLEAN,	"bool[]",       sizeof(char)},
    { T_CHAR,       SIGNATURE_CHAR,	"char[]",       sizeof(unicode)},
    { T_FLOAT,      SIGNATURE_FLOAT,	"float[]",      sizeof(float)},
    { T_DOUBLE,     SIGNATURE_DOUBLE,	"double[]",     sizeof(double)},
    { T_BYTE,       SIGNATURE_BYTE,	"byte[]",       sizeof(char)},
    { T_SHORT,      SIGNATURE_SHORT,	"short[]",      sizeof(short)},
    { T_INT,        SIGNATURE_INT,	"int[]",        sizeof(int32_t)},
    { T_LONG,       SIGNATURE_LONG,	"long[]",       sizeof(int64_t)},
    { 0,	    0,			"Obsolete",	0},
    { 0,	    0,			"Obsolete",	0},
    { 0,	    0,			"Obsolete",	0},
    { 0,	    0,			"Obsolete",	0}
};

#define ARRAYTYPES 	(sizeof(arrayinfo) / sizeof(arrayinfo[0]))

void prof_heap(FILE *fp)
{
}

/*
 * Support for DumpMonitors(): we want to print whose monitor this is
 * if it is associated with an object.  To check whether that's so we
 * need to see whether the handle is in the handle pool, and the limits
 * of the handle pool aren't visible out of gc.c.
 */ 
void
monitorDumpHelper(monitor_t *mid, void *name)
{
    unsigned int key = mid->key;

    if (mid && (mid->flags & MON_IN_USE) != 0) {
	if (name == 0) {
	    if (key) {
		name = Object2CString((JHandle *) key);
	    } else {
		name = "unknown key";
	    }
	}
	fprintf(stderr, "    %s (key=%p): ", name, key);
	sysMonitorDumpInfo((sys_mon_t*) mid->mid);
    }
    return;
}

/*
 * Calculate the size in bytes of an array of type t and length l.
 */
int32_t sizearray(int32_t t, int32_t l)
{
    int size = 0;

    switch(t){
    case T_CLASS:
	size = sizeof(OBJECT);
	break;
    default:
	size = T_ELEMENT_SIZE(t);
	break;
    }
    size *= l;
    return size;
}

/*
** In order to return instances of class Class, the classresolver code
** allocates a handle whose mptr equals the class Class mptr and whose
** obj value points to the runtime version of the class. This call is
** here for just that purpose.
*/
HObject *AllocHandle(struct methodtable *mptr, ClassObject *p)
{
	JHandle *hp;

    hp = (JHandle*) PR_AllocMemory(sizeof(JHandle), scannedTIX,
                                   PR_ALLOC_CLEAN);
    if (hp) {
        /*
        ** Note: If the gc runs before these two stores complete, it's ok
        ** because it will find the references to the new handle and to
        ** the ClassClass object on the C stack.
        */
	hp->methods = mptr;
	hp->obj = (ClassObject *) p;
    }
    return (HObject *) hp;
}

/*
** Allocate a handle and object memory for an object. bytes is the size
** of the object memory. Allocate them as a single unit since java only
** deals with handles not objects...The underyling GC has to be able to
** deal with offset pointers anyway (that is, pointers which point
** somewhere in the middle of an object), so it's ok.
**
** Caller arranges to clean the memory returned.
*/
static HObject *realObjAlloc(struct methodtable *mptr, long bytes)
{
    HObject *h;
    int tix, flags;

    heap_memory_changes++;

    /* See if the object requires finalization or scanning */
    switch (atype((unsigned long) mptr)) {
      case T_NORMAL_OBJECT:
	/* Normal object that may require finalization or double alignment. */
        /* XXX pessimistic assumption for now */
	flags = PR_ALLOC_DOUBLE | PR_ALLOC_CLEAN;
	if (mptr->classdescriptor->finalizer) {
	    tix = scannedFinalTIX;
	} else {
	    tix = scannedTIX;
	}
	break;

      case T_CLASS:		/* An array of objects */
	flags =  PR_ALLOC_CLEAN;
	tix = scannedTIX;
	break;

      case T_LONG:		/* An array of java long's */
      case T_DOUBLE:		/* An array of double's */
	flags = PR_ALLOC_DOUBLE | PR_ALLOC_CLEAN;
	tix = unscannedTIX;
	break;

      default:
	/* An array of something (char, byte, ...) */
	flags = PR_ALLOC_CLEAN;
	tix = unscannedTIX;
	break;
    }

    if (flags & PR_ALLOC_DOUBLE) {
	/* Double align bytes */
	bytes = (bytes + BYTES_PER_DWORD - 1) >> BYTES_PER_DWORD_LOG2;
	bytes <<= BYTES_PER_DWORD_LOG2;
    } else {
	/* Word align bytes */
	bytes = (bytes + BYTES_PER_WORD - 1) >> BYTES_PER_WORD_LOG2;
	bytes <<= BYTES_PER_WORD_LOG2;
    }

    /*
    ** Add in space for the handle (which is two words so we won't mess
    ** up the double alignment)
    */
    bytes += sizeof(JHandle);

    /* Allocate object and handle memory */
    h = (JHandle*) PR_AllocMemory(bytes, tix, flags);
    if (!h) {
	return 0;
    }

    /*
    ** Fill in handle.
    **
    ** Note: if the gc runs before these two stores happen, it's ok
    ** because it will find the reference to the new object on the C
    ** stack. The reference to the class object will be found in the
    ** calling frames C stack (see ObjAlloc below).
    */
    h->methods = mptr;
    h->obj = (ClassObject*) (h + 1);

    GCTRACE(GC_ALLOC, ("handle=%p type=%s %s",
		       ((prword_t*)h) - 1, TYPEOF(h),
		       (tix == scannedFinalTIX) ? "final" : ""));
    return h;
}

/*
 * Allocate an object.  This routine is in flux.  For now, the second 
 * parameter should always be zero and the first must alwaysd point to
 * a valid classblock.  This routine should not be used to allocate
 * arrays; use ArrayAlloc for that.
 */
HObject *ObjAlloc(ClassClass *cb, long n0)
{
    HObject *handle;

    n0 = cbInstanceSize(cb);
    handle = realObjAlloc(cbMethodTable(cb), n0);

#ifdef LOG_JAVA_ALLOCS
    if (handle)
    {
	PR_LOG(Allocation, debug, ("Allocated object %p\n", handle));
	sysThreadDumpInfo(sysThreadSelf(), stderr);
    }
#endif
    return handle;
}

JRI_PUBLIC_API(HObject *)
ArrayAlloc(int32_t t, int32_t l)
{
    HObject *handle;

    PR_ASSERT(t >= T_CLASS && t < T_MAXNUMERIC);
/*
 *  Uncomment me to find all the places where the code creates zero length
 *  arrays.
 *
 *  if (l == 0) {
 *	extern void DumpThreads();
 *	printf("zero length array created %s[%d]\n", arrayinfo[t].name, l);
 *	DumpThreads();
 *  }
 */
    handle = realObjAlloc((struct methodtable *) mkatype(t, l),
	   sizearray(t, l) + (t == T_CLASS ? sizeof(OBJECT) : 0));

#ifdef LOG_JAVA_ALLOCS
    if (handle)
    {
	fprintf(stderr, "Allocated array %p\n", handle);
	sysThreadDumpInfo(sysThreadSelf(), stderr);
    }
#endif
    return handle;
}

void InitializeAlloc(long max_request, long min_request)
{
    PR_InitGC(0, min_request);
    gcInfo = PR_GetGCInfo();

    PR_SetBeginGCHook((GCBeginGCHook*) PrepareClassesForGC, 0);

    PR_RegisterRootFinder(ScanJavaThreads, "scan java threads", 0);
    PR_RegisterRootFinder(ProcessRootsInClassTable, "scan class table", 0);

    unscannedTIX = PR_RegisterType(&unscannedType);
    scannedTIX = PR_RegisterType(&scannedType);
    scannedFinalTIX = PR_RegisterType(&scannedFinalType);
    classClassTIX = PR_RegisterType(&classClassType);
    verifyContextTIX = PR_RegisterType(&verifyContextType);
#if XP_MAC
    InstallMemoryCacheFlusher(&ClassArenaFlusherProc);
#endif
    PR_RegisterSummaryPrinter(PrintSummary, NULL);
}

void gc(int async_call, unsigned int free_space_goal)
{
    /* XXX async_call ignored */
    /* XXX free_space_goal ignored */
    PR_GC();
}

int64 TotalObjectMemory(void)
{
    int32 sum = gcInfo->busyMemory + gcInfo->freeMemory;
    int64 rv;
    LL_I2L(rv, sum);
    return rv;
}

int64 FreeObjectMemory(void)
{
    int64 rv;
    LL_I2L(rv, gcInfo->freeMemory);
    return rv;
}

int64 TotalHandleMemory(void)
{
    return ll_zero_const;
}

int64 FreeHandleMemory(void)
{
    return ll_zero_const;
}
