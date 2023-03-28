/* XXX we leak private/secret data via js_toString_stub and others */
/* XXX need to check for taint and throw exception in js_GetMember_stub etc. */

/* XXX remove when pure JRI if possible */
#include "oobj.h"
#include "interpreter.h"
#include "tree.h"


#include "mocha.h"
#include "mochaapi.h"
#include "mo_java.h"
#include "mo_cntxt.h"   /* for MochaContext.javaEnv */

#include "java.h"
#include "jri.h"
#include "lj.h"

#include "prmon.h"	/* for PR_XLock and PR_XUnlock */
#include "prprf.h"	/* for PR_snprintf */
#include "prthread.h"
#include "prlog.h"

#include "structs.h"    /* for MWContext.type */

#define IMPLEMENT_netscape_javascript_JSObject
#include "netscape_javascript_JSObject.h"
#ifndef XP_MAC
#include "netscape_javascript_JSException.h"
#else
#include "n_javascript_JSException.h"
#endif

#define IMPLEMENT_netscape_applet_AppletClassLoader
#define IMPLEMENT_netscape_applet_MozillaAppletContext
#ifndef XP_MAC
/* for acl.context: */
#include "netscape_applet_AppletClassLoader.h"
/* for acx.frameMWContext:  */
#include "netscape_applet_MozillaAppletContext.h"
#else
#include "n_applet_AppletClassLoader.h"
#include "n_applet_MozillaAppletContext.h"
#endif

/*
 * utility functions, some overlap with jrijdk.c
 */
#define NaEnv2EE(env)		\
	((ExecEnv*)((char*)(env) - offsetof(ExecEnv, nativeInterface)))
#define java_MochaContext(env) ((MochaContext *) (NaEnv2EE(env)->mochaContext))
#define java_SetMochaContext(env, mc) \
	(* (MochaContext **) &(NaEnv2EE(env)->mochaContext) = (mc))

#if 0
static JRIGlobalRef js_JSExceptionClass = 0;
#endif

#define JAVA_MOCHA_STACK_SIZE 8192

typedef struct {
    JRIEnv *env;
    MochaErrorReporter reporter;
} SavedMochaContext;

typedef struct JSErrorState JSErrorState;
struct JSErrorState {
    char *message;
    MochaErrorReport error;
    JSErrorState *next;
};

static void
js_throwJSException(JRIEnv *env, char *detail)
{
    jref message = JRI_NewStringUTF(env, detail, strlen(detail));
    jref exc =
      netscape_javascript_JSException_new_1(env,
                         class_netscape_javascript_JSException(env),
                         /*Java_getGlobalRef(env, js_JSExceptionClass),*/
                         message);

    JRI_Throw(env, exc);
}

static void *
js_currentClassLoader(JRIEnv *env)
{
    struct javaframe *frame, frame_buf;
    ClassClass	*cb;
    ExecEnv *ee = (ExecEnv *) env;
    void *loader = 0;

    /* XXX this should all be put in a convenience function
     * in security.c! */

    /* otherwise, look up the stack for a classloader */
    for (frame = ee->current_frame ; frame != 0 ;) {
	if (frame->current_method != 0) {
	    cb = fieldclass(&frame->current_method->fb);
	    if (cb && cbLoader(cb)) {
                loader = cbLoader(cb);
                break;
	    }
            frame = frame->current_method->fb.access & ACC_MACHINE_COMPILED ?
		CompiledFramePrev(frame, &frame_buf) : frame->prev;
	} else {
	    frame = frame->prev;
        }
    }
    return loader;
}


/* check to see if it is ok to use JS from this Java context.
 * if not, throw an exception and return false */
static Bool
js_checkMayScript(JRIEnv *env)
{
    jref loader;

    loader = js_currentClassLoader(env);

    if (!loader || netscape_applet_AppletClassLoader_mayScript(env, loader))
        return TRUE;

    js_throwJSException(env, "MAYSCRIPT is not enabled for this applet");
    return FALSE;
}

/* find the current classloader and associated MochaContext
 * if possible */
/* this must be called only if both the mozilla and the java thread
 * are under control to avoid race conditions */
static MochaContext *
js_findMochaContext(JRIEnv *env, char **errp)
{
    ExecEnv *ee = (ExecEnv *) env;
    void *loader;

    *errp = 0;

    /* XXX daemon threads can't call mocha */

    loader = js_currentClassLoader(env);

    if (!loader) {
        extern MochaContext *lm_crippled_context; /* XXX kill me */
        /* no classloader, use the crippled MochaContext */
        PR_LOG(Moja, debug,
               ("called JavaScript with no classloader"));
        return lm_crippled_context;
    }

    if (obj_classblock((HObject*)loader)
        == FindClass(ee, "netscape/applet/AppletClassLoader", FALSE)) {
        netscape_applet_AppletClassLoader *aloader = loader;
        netscape_applet_MozillaAppletContext *acx;
        MWContext *mwcontext;
        MochaContext *mc;

        /* ok, we were called through an applet.  find the associated
         * applet context */
        acx = get_netscape_applet_AppletClassLoader_context(env, aloader);

        /* get the MWContext from the applet context */
        mwcontext = (MWContext *)
            get_netscape_applet_MozillaAppletContext_frameMWContext(env, acx);

        if (!mwcontext) {
            PR_LOG(Moja, warn,
                   ("no MWContext for MozillaAppletContext 0x%x",
                    acx));
            *errp = "JavaScript unavailable: no window found";
            return 0;
        }

        /* we only allow mocha calls in browser windows */
        /* XXX this could probably be relaxed a bit */
        if (mwcontext->type != MWContextBrowser) {
            PR_LOG(Moja, warn,
                   ("tried to call JavaScript on a context of type %d",
                    mwcontext->type));
            *errp = "JavaScript unavailable: bad window type";
            return 0;
        }

        /* ... and the MochaContext from there... */
        if (!(mc = mwcontext->mocha_context)) {
            PR_LOG(Moja, warn,
                   ("no MochaContext for MWContext 0x%x", mwcontext));
            *errp = "JavaScript unavailable in this window";
            return 0;
        }

        if (!mc->globalObject) {
            PR_LOG(Moja, warn,
                   ("no global object for MochaContext 0x%x", mc));
            *errp = "JavaScript unavailable in this window";
            return 0;
        }
        return mc;
    }

    PR_LOG(Moja, warn, ("unknown ClassLoader subclass %s found",
                        classname(obj_classblock((HObject*)loader))));
    *errp = "JavaScript unavailable: illegal ClassLoader";
    return 0;
}

/***********************************************************************/

/*
 *  this is the gateway from java to mocha.
 */

/* all callbacks used with js_CallMocha must start like this */
typedef struct {
    JRIEnv *env;
    MochaObject *mo;
} js_CallMocha_data_prefix;

typedef struct {
    JRIEnv *env;
    MochaObject *mo;
    LJCallback doit;
    js_CallMocha_data_prefix *data;
    char *err;
} js_CallMocha_data;

/* this extra level of indirection (double-callback) is
 * unfortunate but necessary to make sure that we can mess
 * with the MochaContext without interfering with mozilla.
 * another approach would be to make lj_thred.c mocha-specific.
 */
static void
js_CallMocha_stub(void *d)
{
    js_CallMocha_data *data = (js_CallMocha_data *) d;
    SavedMochaContext save;
    JRIEnv *env = data->env;
    MochaContext *mc;

    data->err = 0;

    mc = js_findMochaContext(env, &data->err);

    /* if we couldn't get the mocha context return and throw an
     * exception with detail message data->err */
    if (!mc) return;

    /* save some info and change the error reporter */
    save.env = (JRIEnv *) mc->javaEnv;
    save.reporter = MOCHA_SetErrorReporter(mc, mocha_js_ErrorReporter);

    mc->javaEnv = (void*) env;

    /* XXX should save env.mochaContext as well? */
    java_SetMochaContext(env, mc);

    data->data->env = data->env;
    data->data->mo = data->mo;
    data->doit(data->data);

    /* restore saved info */
    mc->javaEnv = (void*) save.env;
    MOCHA_SetErrorReporter(mc, save.reporter);

    mocha_MErrorToJException(mc, (ExecEnv*)env);
}

/* this function calls doit on the mozilla thread with a
 * valid mocha context */
/* it is called from a java thread */
static void
js_CallMocha(JRIEnv *env, struct netscape_javascript_JSObject* self,
             LJCallback doit, void *d)
{
    js_CallMocha_data data;
    MochaObject *mo;

    if (!self ||
        !(mo = (MochaObject *)
          get_netscape_javascript_JSObject_internal(env, self))) {
        /* XXX use a better exception */
        JRI_ThrowNew(env, JRI_FindClass(env, "java/lang/Exception"),
                      "not a JavaScript object");
        return;
    }

    PR_LOG(Moja, debug, ("clearing exceptions in ee=0x%x", env));
    exceptionClear((ExecEnv *)env);

    data.env = env;
    data.mo = mo;
    data.doit = doit;
    data.data = d;

    PR_LOG_BEGIN(Moja, debug, ("entering mocha mo=0x%x", mo));
    LJ_CallMozilla(env, js_CallMocha_stub, &data);
    PR_LOG_END(Moja, debug, ("leaving mocha mo=0x%x", mo));

    /* data.err holds errors from the stub itself - errors from
     * mocha will already have been thrown by now */
    if (data.err) {
        js_throwJSException(env, data.err);
    }
}

/***********************************************************************/

typedef struct {
    JRIEnv *env;
    MochaObject *mo;
    char *cstr;
    MochaSlot slot;
    jref ret;
} js_GetMember_data;

static void
js_GetMember_stub(void *d)
{
    js_GetMember_data *data = (js_GetMember_data*) d;
    MochaDatum datum;
    JRIEnv *env = data->env;
    MochaContext *mc = java_MochaContext(env);
    int cost = 0;

    data->ret = NULL;
    if (data->cstr ? MOCHA_ResolveName(mc, data->mo, data->cstr, &datum)
		   : MOCHA_GetSlot(mc, data->mo, data->slot, &datum))
        mocha_convertMDatumToJObject(/*XXX*/(HObject**)&data->ret, mc, &datum,
                                     0, 0, MOCHA_FALSE, &cost);
}

/*	*	*	*	*	*	*	*	*/

JRI_PUBLIC_API(struct java_lang_Object *)
native_netscape_javascript_JSObject_getMember(
    JRIEnv* env,
    struct netscape_javascript_JSObject* self,
    struct java_lang_String *name)
{
    js_GetMember_data data;
    const char *cstr = 0;
    jref member;

    if (!js_checkMayScript(env))
        return 0;

    if (name != NULL &&
        (cstr = JRI_GetStringUTFChars(env, name))) {
        data.cstr = (char*)cstr;
        js_CallMocha(env, self, js_GetMember_stub, &data);
        member = data.ret;
    } else {
        /* XXX use a better exception */
        JRI_ThrowNew(env, JRI_FindClass(env, "java/lang/Exception"),
                      "illegal member name");
        member = NULL;
    }
    return member;
}

/* public native getSlot(I)Ljava/lang/Object; */
JRI_PUBLIC_API(struct java_lang_Object *)
native_netscape_javascript_JSObject_getSlot(
    JRIEnv* env,
    struct netscape_javascript_JSObject* self,
    jint slot)
{
    js_GetMember_data data;

    if (!js_checkMayScript(env))
        return 0;

    PR_LOG(Moja, debug,
           ("JSObject.getSlot(%d)\n", slot));

    data.cstr = 0;
    data.slot = slot;
    js_CallMocha(env, self, js_GetMember_stub, &data);
    return data.ret;
}

/***********************************************************************/

typedef struct {
    JRIEnv *env;
    MochaObject *mo;
    char *cstr;
    MochaSlot slot;
    jref value;
} js_SetMember_data;

static void
js_SetMember_stub(void *d)
{
    js_SetMember_data *data = (js_SetMember_data*) d;
    JRIEnv *env = data->env;
    MochaContext *mc = java_MochaContext(env);
    MochaDatum datum, saveDatum;
    MochaObject *obj = data->mo;
    MochaSymbol *sym = 0;
    char buf[24], *name;
    MochaAtom *atom;

    if (!mocha_convertJObjectToMDatum(mc, &datum,
                                      /*XXX*/(HObject*)data->value)) {
	return;
    }

    name = data->cstr;
    if (!name) {
	PR_snprintf(buf, sizeof buf, "%d", data->slot);
	name = buf;
    }
    atom = MOCHA_Atomize(mc, name);
    if (!atom)
	return;
    saveDatum = datum;
    MOCHA_HoldRef(mc, &saveDatum);
    MOCHA_HoldAtom(mc, atom);
    if (mocha_LookupSymbol(mc, obj->scope, atom, MLF_SET, &sym)) {
	if (!sym || sym->type != SYM_PROPERTY) {
	    sym = mocha_SetProperty(mc, obj->scope, atom,
				    data->cstr ? data->mo->scope->minslot-1
					       : data->slot,
				    datum);
	}
	if (sym) {
	    MochaProperty *prop = sym_property(sym);
	    MochaDatum oldDatum = prop->datum;

	    if (prop->datum.flags & MDF_READONLY) {
		MOCHA_ReportError(mc, "%s cannot be set by assignment",
				  atom_name(sym_atom(sym)));
	    } else if ((*prop->setter)(mc, obj, sym->slot, &datum)) {
                /* Store value, taking care not to smash nrefs and flags. */
                MOCHA_INIT_FULL_DATUM(mc, &prop->datum, datum.tag,
                                      prop->datum.flags, datum.taint,
                                      u, datum.u);

		MOCHA_HoldRef(mc, &prop->datum);
		MOCHA_DropRef(mc, &oldDatum);
	    }
	}
    }
    MOCHA_DropRef(mc, &saveDatum);
    MOCHA_DropAtom(mc, atom);

    PR_LOG(Moja, debug,
	   ("JSObject.setMember(%s, 0x%x)\n", data->cstr, data->value));
}

/*	*	*	*	*	*	*	*	*/

JRI_PUBLIC_API(void)
native_netscape_javascript_JSObject_setMember(
    JRIEnv* env,
    struct netscape_javascript_JSObject* self,
    struct java_lang_String *name,
    struct java_lang_Object *value)
{
    js_SetMember_data data;
    const char *cstr = 0;

    if (!js_checkMayScript(env))
        return;

    if (name != NULL &&
        (cstr = JRI_GetStringUTFChars(env, name))) {
        data.cstr = (char*)cstr;
        data.slot = -1;
        data.value = value;
        js_CallMocha(env, self, js_SetMember_stub, &data);
    } else {
        /* XXX use a better exception */
        JRI_ThrowNew(env, JRI_FindClass(env, "java/lang/Exception"),
                      "illegal member name");
    }
}

JRI_PUBLIC_API(void)
native_netscape_javascript_JSObject_setSlot(
    JRIEnv* env,
    struct netscape_javascript_JSObject* self,
    jint slot,
    struct java_lang_Object *value)
{
    js_SetMember_data data;

    if (!js_checkMayScript(env))
        return;

    PR_LOG(Moja, debug,
           ("JSObject.setSlot(%d, 0x%x)\n", slot, value));

    data.cstr = 0;
    data.slot = slot;
    data.value = value;
    js_CallMocha(env, self, js_SetMember_stub, &data);
}

/***********************************************************************/

typedef struct {
    JRIEnv *env;
    MochaObject *mo;
    char *cstr;
} js_RemoveMember_data;

static void
js_RemoveMember_stub(void *d)
{
    js_RemoveMember_data *data = (js_RemoveMember_data*) d;
    JRIEnv *env = data->env;
    MochaContext *mc = java_MochaContext(env);

    MOCHA_RemoveProperty(mc, data->mo, data->cstr);

    PR_LOG(Moja, debug,
           ("JSObject.removeMember(%s)\n", data->cstr));
}

/*	*	*	*	*	*	*	*	*/

JRI_PUBLIC_API(void)
native_netscape_javascript_JSObject_removeMember(
    JRIEnv* env,
    struct netscape_javascript_JSObject* self,
    struct java_lang_String * name)
{
    js_RemoveMember_data data;
    const char *cstr = 0;

    if (!js_checkMayScript(env))
        return;

    if (name != NULL &&
        (cstr = JRI_GetStringUTFChars(env, name))) {
        data.cstr = (char*)cstr;
        js_CallMocha(env, self, js_RemoveMember_stub, &data);
        PR_LOG(Moja, debug, ("JSObject.removeMember(%s)", cstr));
    } else {
        /* XXX use a better exception */
        JRI_ThrowNew(env, JRI_FindClass(env, "java/lang/Exception"),
                      "illegal member name");
    }
}

/***********************************************************************/

typedef struct {
    JRIEnv *env;
    MochaObject *mo;
    char *method;
    jref args;
    PRBool success;
    jref ret;
} js_Call_data;

static void
js_Call_stub(void *d)
{
    js_Call_data *data = (js_Call_data*) d;
    MochaDatum *argv;
    int argc, i;
    JRIEnv *env = data->env;
    MochaContext *mc = java_MochaContext(env);
    int cost = 0;

    if (data->args) {
        argc = JRI_GetObjectArrayLength(env, data->args);
        argv = sysMalloc(argc * sizeof(MochaDatum));
    } else {
        argc = 0;
        argv = 0;
    }

    data->success = PR_TRUE;
    for (i = 0; i < argc; i++) {
        jref arg = JRI_GetObjectArrayElement(env, data->args, i);

        data->success =
          mocha_convertJObjectToMDatum(mc, argv + i,
                                       /*XXX*/(HObject*)arg);
        if (!data->success)
            break;
    }
    /* XXX if argument 2 fails argument 1 will never be freed! */
    if (data->success) {
        MochaDatum rval;

	MOCHA_MIX_TAINT(mc, mc->taintInfo->accum, MOCHA_TAINT_JAVA);
        if (!MOCHA_CallMethod(mc, data->mo, data->method, argc, argv, &rval)
            || !mocha_convertMDatumToJObject((HObject **)&data->ret, mc, &rval,
                                             0, 0, MOCHA_FALSE, &cost))
            data->success = PR_FALSE;
    }

    sysFree(argv);
}

/*      *       *       *       *       *       *       *       */

/* XXX should check for self=null everywhere! */

JRI_PUBLIC_API(struct java_lang_Object *)
native_netscape_javascript_JSObject_call(
    JRIEnv* env,
    struct netscape_javascript_JSObject* self,
    struct java_lang_String *method,
    jobjectArray args)
{
    const char *cstr;
    js_Call_data data;

    if (!js_checkMayScript(env))
        return 0;

    if (!method ||
        !(cstr = JRI_GetStringUTFChars(env, method))) {
        /* XXX use a better exception */
        JRI_ThrowNew(env, JRI_FindClass(env, "java/lang/Exception"),
                      "not a method");
        return NULL;
    }

    data.method = (char*)cstr;
    data.args = args;
    PR_LOG_BEGIN(Moja, debug, ("calling mocha \"%s\"", cstr));
    js_CallMocha(env, self, js_Call_stub, &data);
    PR_LOG_END(Moja, debug, ("returned from mocha \"%s\"", cstr));
    if (data.success) {
        return data.ret;
    } else {
        /* exception should be thrown inside CallMozilla */
        return NULL;
    }
}

/***********************************************************************/

typedef struct {
    JRIEnv *env;
    MochaObject *mo;
    char *source;
    size_t sourcelen;
    jref ret;
} js_Eval_data;

static void
js_Eval_stub(void *d)
{
    js_Eval_data *data = (js_Eval_data*) d;
    MochaDatum rval;
    JRIEnv *env = data->env;
    MochaContext *mc = java_MochaContext(env);
    int cost = 0;

    data->ret = NULL;
    MOCHA_MIX_TAINT(mc, mc->taintInfo->accum, MOCHA_TAINT_JAVA);
    if (MOCHA_EvaluateBuffer(mc, data->mo, data->source, data->sourcelen,
                             "(java call)", 0, &rval))
        mocha_convertMDatumToJObject((HObject**)&data->ret, mc, &rval,
                                     0, 0, MOCHA_FALSE, &cost);
}

/*	*	*	*	*	*	*	*	*/

JRI_PUBLIC_API(struct java_lang_Object *)
native_netscape_javascript_JSObject_eval(
    JRIEnv* env,
    struct netscape_javascript_JSObject* self,
    struct java_lang_String *s)
{
    js_Eval_data data;
    const char *source;
    jref ret = NULL;

    if (!js_checkMayScript(env))
        return 0;

    if (s &&
        (source = JRI_GetStringUTFChars(env, s))) {
        data.source = (char*)source;
        /* subtract 1 for the NUL */
        data.sourcelen = JRI_GetStringUTFLength(env, s) - 1;

        js_CallMocha(env, self, js_Eval_stub, &data);

        ret = data.ret;
    } else {
        /* XXX use a better exception */
        JRI_ThrowNew(env, JRI_FindClass(env, "java/lang/Exception"),
                      "bad script");
    }
    return ret;
}

/***********************************************************************/

typedef struct {
    JRIEnv *env;
    MochaObject *mo;
} js_Finalize_data;

static void
js_Finalize_stub(void *d)
{
    js_Finalize_data *data = (js_Finalize_data*) d;
    JRIEnv *env = data->env;
    MochaContext *mc = java_MochaContext(env);

    mocha_removeReflection(data->mo);
    MOCHA_DropObject(mc, data->mo);
}

/*	*	*	*	*	*	*	*	*/

JRI_PUBLIC_API(void)
native_netscape_javascript_JSObject_finalize(
    JRIEnv* env,
    struct netscape_javascript_JSObject* self)
{
    js_Finalize_data data;

    /* XXX checkMayScript here?  what would that accomplish? */

    js_CallMocha(env, self, js_Finalize_stub, &data);
}

/***********************************************************************/


typedef struct {
    JRIEnv *env;
    MochaObject *mo;
    char *ret;
} js_toString_data;

static void
js_toString_stub(void *d)
{
    js_toString_data *data = (js_toString_data*) d;
    JRIEnv *env = data->env;
    MochaContext *mc = java_MochaContext(env);
    MochaDatum datum;
    MochaAtom *atom;

    MOCHA_INIT_FULL_DATUM(mc, &datum, MOCHA_OBJECT, 0, MOCHA_TAINT_IDENTITY,
			  u.obj, data->mo);

    if (!MOCHA_DatumToString(mc, datum, &atom))
        data->ret = 0;

    /* freed once the stub returns */
    data->ret = strdup(MOCHA_GetAtomName(mc, atom));
    MOCHA_DropAtom(mc, atom);
}

/*	*	*	*	*	*	*	*	*/

JRI_PUBLIC_API(struct java_lang_String *)
native_netscape_javascript_JSObject_toString(
    JRIEnv* env,
    struct netscape_javascript_JSObject* self)
{
    js_toString_data data;
    jref ret;

    if (!js_checkMayScript(env))
        return 0;

    js_CallMocha(env, self, js_toString_stub, &data);

    if (!data.ret) {
        /* XXX could grab the clazz name from the MochaObject */
        return JRI_NewStringUTF(env, "*JSObject*", strlen("*JSObject*"));
    }
    /* make a java String from str */
    ret = JRI_NewStringUTF(env, data.ret, strlen(data.ret));

    free(data.ret);
    return ret;
}

/***********************************************************************/

typedef struct {
    JRIEnv *env;
    jref applet;
    jref ret;
} js_getWindow_data;

static void
js_getWindow_stub(void *d)
{
    js_getWindow_data *data = (js_getWindow_data*) d;

    data->ret = LJ_GetAppletMochaWindow(data->env, data->applet);
}

/*	*	*	*	*	*	*	*	*/

JRI_PUBLIC_API(struct netscape_javascript_JSObject*)
native_netscape_javascript_JSObject_getWindow(
    JRIEnv* env,
    struct java_lang_Class* clazz,
    struct java_applet_Applet* applet)
{
    js_getWindow_data data;

    if (!js_checkMayScript(env))
        return 0;

    if (!applet) {
        JRI_ThrowNew(env, JRI_FindClass(env, "java/lang/NullPointerException"),
                     0);
        return 0;
    }

    if (!MOCHA_JavaGlueEnabled()) {
        js_throwJSException(env, "Java/JavaScript communication is disabled");
        return 0;
    }

    data.env = env;
    data.applet = applet;
    LJ_CallMozilla(env, js_getWindow_stub, &data);

    return data.ret;
}

/***********************************************************************/

JRI_PUBLIC_API(void)
native_netscape_javascript_JSObject_initClass(
    JRIEnv* env,
    struct java_lang_Class* myClazz)
{
    struct java_lang_Class* clazz;
    /* this is to initialize the field lookup indices, and protect
     * the classes from gc.  */
    /* XXX it would be nice to ensure that this gets called 
     * with mozenv, since if a different env is used these globals
     * may go byebye. */
    clazz = use_netscape_javascript_JSObject(env);
    JRI_NewGlobalRef(env, clazz);
    clazz = use_netscape_javascript_JSException(env);
    JRI_NewGlobalRef(env, clazz);

    /* XXX i really don't think this is the right place to do this */
    clazz = use_netscape_applet_AppletClassLoader(env);
    JRI_NewGlobalRef(env, clazz);
}

