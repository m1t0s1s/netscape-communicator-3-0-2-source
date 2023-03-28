/*
** Mocha reflection of the Navigator top-level object.
**
** Brendan Eich, 11/16/95
*/
#include "lm.h"
#include "prmem.h"
#include "java.h"
#include "gui.h"

enum nav_slot {
    NAV_USER_AGENT      = -1,
    NAV_APP_CODE_NAME   = -2,
    NAV_APP_VERSION     = -3,
    NAV_APP_NAME        = -4,
    NAV_PLUGINLIST      = -5,
    NAV_MIMETYPELIST    = -6
};

static MochaPropertySpec nav_props[] = {
    {"userAgent",       NAV_USER_AGENT,         MDF_ENUMERATE | MDF_READONLY},
    {"appCodeName",     NAV_APP_CODE_NAME,      MDF_ENUMERATE | MDF_READONLY},
    {"appVersion",      NAV_APP_VERSION,        MDF_ENUMERATE | MDF_READONLY},
    {"appName",         NAV_APP_NAME,           MDF_ENUMERATE | MDF_READONLY},
    {"plugins",         NAV_PLUGINLIST,         MDF_ENUMERATE | MDF_READONLY},
    {"mimeTypes",       NAV_MIMETYPELIST,       MDF_ENUMERATE | MDF_READONLY},
    {0}
};

typedef struct MochaNavigator {
    MochaDecoder        *decoder;
    MochaAtom           *userAgent;
    MochaAtom           *appCodeName;
    MochaAtom           *appVersion;
    MochaAtom           *appName;
    MochaObject         *pluginList;
    MochaObject         *mimeTypeList;
} MochaNavigator;

static MochaBoolean
nav_get_property(MochaContext *mc, MochaObject *obj, MochaSlot slot,
                 MochaDatum *dp)
{
    MochaNavigator *nav = obj->data;
    enum nav_slot nav_slot;

    nav_slot = slot;
    switch (nav_slot) {
      case NAV_USER_AGENT:
        MOCHA_INIT_DATUM(mc, dp, MOCHA_STRING, u.atom, nav->userAgent);
        break;
      case NAV_APP_CODE_NAME:
        MOCHA_INIT_DATUM(mc, dp, MOCHA_STRING, u.atom, nav->appCodeName);
        break;
      case NAV_APP_VERSION:
        MOCHA_INIT_DATUM(mc, dp, MOCHA_STRING, u.atom, nav->appVersion);
        break;
      case NAV_APP_NAME:
        MOCHA_INIT_DATUM(mc, dp, MOCHA_STRING, u.atom, nav->appName);
        break;
      case NAV_PLUGINLIST:
        MOCHA_INIT_DATUM(mc, dp, MOCHA_OBJECT, u.obj, nav->pluginList);
        return MOCHA_TRUE;
      case NAV_MIMETYPELIST:
        MOCHA_INIT_DATUM(mc, dp, MOCHA_OBJECT, u.obj, nav->mimeTypeList);
        return MOCHA_TRUE;

      default:
        /* Don't mess with user-defined or method properties. */
        return MOCHA_TRUE;
    }

    if (!dp->u.atom)
        dp->u.atom = MOCHA_empty.u.atom;
    return MOCHA_TRUE;
}


static void
nav_finalize(MochaContext *mc, MochaObject *obj)
{
    MochaNavigator *nav = obj->data;

    LM_UNLINK_OBJECT(nav->decoder, obj);
    MOCHA_DropAtom(mc, nav->userAgent);
    MOCHA_DropAtom(mc, nav->appCodeName);
    MOCHA_DropAtom(mc, nav->appVersion);
    MOCHA_DropAtom(mc, nav->appName);
    MOCHA_DropObject(mc, nav->pluginList);
    MOCHA_DropObject(mc, nav->mimeTypeList);
    XP_DELETE(nav);
}

static MochaClass nav_class = {
    "Navigator",
    nav_get_property, nav_get_property, MOCHA_ListPropStub,
    MOCHA_ResolveStub, MOCHA_ConvertStub, nav_finalize
};

static MochaBoolean
Navigator(MochaContext *mc, MochaObject *obj,
          uint argc, MochaDatum *argv, MochaDatum *rval)
{
    return MOCHA_TRUE;
}

static MochaBoolean
nav_java_enabled(MochaContext *mc, MochaObject *obj,
                 uint argc, MochaDatum *argv, MochaDatum *rval)
{
#ifdef JAVA
    MOCHA_INIT_DATUM(mc, rval, MOCHA_BOOLEAN, u.bval, LJ_GetJavaEnabled());
#else
    *rval = MOCHA_false;
#endif
    return MOCHA_TRUE;
}

static MochaBoolean
nav_taint_enabled(MochaContext *mc, MochaObject *obj,
                 uint argc, MochaDatum *argv, MochaDatum *rval)
{
    MOCHA_INIT_DATUM(mc, rval, MOCHA_BOOLEAN, u.bval, lm_data_tainting);
    return MOCHA_TRUE;
}

static MochaFunctionSpec nav_methods[] = {
    {"javaEnabled",     nav_java_enabled,       0},
    {"taintEnabled",    nav_taint_enabled,      0},
    {0}
};

MochaObject *
lm_DefineNavigator(MochaDecoder *decoder)
{
    MochaObject *obj;
    MochaContext *mc;
    MochaNavigator *nav;
    char *userAgent;

    obj = decoder->navigator;
    if (obj)
        return obj;

    mc = decoder->mocha_context;
    nav = MOCHA_malloc(mc, sizeof *nav);
    if (!nav)
        return 0;
    obj = MOCHA_InitClass(mc, decoder->window_object, &nav_class, nav,
                          Navigator, 0, nav_props, nav_methods, 0, 0);
    if (!obj) {
        MOCHA_free(mc, nav);
        return 0;
    }
    if (!MOCHA_DefineObject(mc, decoder->window_object, "navigator", obj,
                            MDF_ENUMERATE | MDF_READONLY)) {
        return 0;
    }

    nav->decoder = decoder;

    /* XXX bail on null returns from PR_smprintf and MOCHA_Atomize */
    userAgent = PR_smprintf("%s/%s", XP_AppCodeName, XP_AppVersion);
    nav->userAgent = MOCHA_Atomize(mc, userAgent);
    PR_FREEIF(userAgent);
    MOCHA_HoldAtom(mc, nav->userAgent);

    nav->appCodeName = MOCHA_Atomize(mc, XP_AppCodeName);
    MOCHA_HoldAtom(mc, nav->appCodeName);

    nav->appVersion = MOCHA_Atomize(mc, XP_AppVersion);
    MOCHA_HoldAtom(mc, nav->appVersion);

    nav->appName = MOCHA_Atomize(mc, XP_AppName);
    MOCHA_HoldAtom(mc, nav->appName);

    /* Ask lm_plgin.c to create objects for plug-in and MIME-type arrays */
    nav->pluginList = lm_NewPluginList(mc);
    MOCHA_HoldObject(mc, nav->pluginList);

    nav->mimeTypeList = lm_NewMIMETypeList(mc);
    MOCHA_HoldObject(mc, nav->mimeTypeList);

    decoder->navigator = MOCHA_HoldObject(mc, obj);
    obj->parent = decoder->window_object;
    LM_LINK_OBJECT(decoder, obj, "navigator");
    return obj;
}
