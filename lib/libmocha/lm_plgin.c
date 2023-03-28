/* -*- Mode: C; tab-width: 4; -*- */
/*
** Mocha reflection of Navigator Plugins.
**
** Created: Brendan Eich, 9/8/95
** Implemented: Chris Bingham, 2/16/96
**
** This file contains implementations of several Mocha objects:
**
**     + MochaPluginList, a lazily-filled array of all installed
**       plug-ins, based on the information exported from lib/plugin
**       by the functions in np.h.  The plug-in properties in the
**       array are named by plug-in name, so you can do an array
**       lookup by name, e.g. plugins["Shockwave"].
**
**     + MochaMIMETypeList, a lazily-filled array of all handled
**       MIME types, based in the information exported from libnet
**       by the functions in net.h.  The type properties in the
**       array are named by type, so you can do an array lookup by
**       type, e.g. mimetypes["video/quicktime"].
**
**     + MochaPlugin, the reflection of an installed plug-in, with
**       static properties for the plug-in's name, file name, etc.
**       and dynamic properties for the MIME types supported by
**       the plug-in.
**
**     + MochaMIMEType, the reflection of an individual MIME type,
**       with properties for the type, file extensions, platform-
**       specific file type, description of the type, and enabled
**		 plug-in.  The enabled plug-in property is a weak object
**		 reference marked as MDF_BACKEDGE to break the cycle of
**		 references from plug-in to mime type to plug-in.
**
*/

#include "lm.h"
#include "prmem.h"
#include "np.h"
#include "net.h"


/*
** -----------------------------------------------------------------------
**
** Data types
**
** -----------------------------------------------------------------------
*/

typedef struct MochaMIMEType
{
    MochaDecoder	       *decoder;
    MochaObject			   *obj;
    MochaAtom	           *type;				/* MIME type itself, e.g. "text/html" */
    MochaAtom	           *description;		/* English description of MIME type */
    MochaAtom	           *suffixes;			/* Comma-separated list of file suffixes */
    MochaObject			   *enabledPluginObj;	/* Plug-in enabled for this MIME type */
    void				   *fileType;			/* Platform-specific file type */
} MochaMIMEType;

typedef struct MochaPlugin
{
    MochaDecoder	       *decoder;
    MochaObject			   *obj;
    MochaAtom	           *name;           	/* Name of plug-in */
    MochaAtom	           *filename;       	/* Name of file on disk */
    MochaAtom	           *description;    	/* Descriptive HTML (version, etc.) */
    NPReference				plugref;			/* Opaque reference to private plugin object */
    uint32					length;         	/* Total number of mime types we have */
    XP_Bool					reentrant;			/* Flag to halt recursion of get_property */
} MochaPlugin;


typedef struct MochaPluginList
{
    MochaDecoder		   *decoder;
    MochaObject		       *obj;
    uint32					length;				/* Total number of plug-ins */
    XP_Bool					reentrant;			/* Flag to halt recursion of get_property */
} MochaPluginList;


typedef struct MochaMIMETypeList
{
    MochaDecoder   		   *decoder;
    MochaObject	   		   *obj;
    uint32 					length;				/* Total number of mime types */
    XP_Bool					reentrant;			/* Flag to halt recursion of get_property */
} MochaMIMETypeList;



/*
** -----------------------------------------------------------------------
**
** Function protos (all private to this file)
**
** -----------------------------------------------------------------------
*/

static MochaMIMEType*		mimetype_create_self(MochaContext *mc, MochaDecoder* decoder, char* type,
								char* description, char** suffixes, uint32 numExtents, void* fileType);
static MochaBoolean			mimetype_get_property(MochaContext *mc, MochaObject *obj, MochaSlot slot,
								MochaDatum *dp);
static void					mimetype_finalize(MochaContext *mc, MochaObject *obj);

static MochaPlugin* 		plugin_create_self(MochaContext *mc, MochaDecoder* decoder, char* name,
								char* filename, char* description, NPReference plugref);
static MochaMIMEType*		plugin_create_mimetype(MochaContext* mc, MochaPlugin* plugin,
								XP_Bool byName, const char* targetName, MochaSlot targetSlot);
static MochaBoolean			plugin_get_property(MochaContext *mc, MochaObject *obj, MochaSlot slot,
								MochaDatum *dp);
static MochaBoolean			plugin_resolve_name(MochaContext *mc, MochaObject *obj, const char *name);
static void 				plugin_finalize(MochaContext *mc, MochaObject *obj);

static MochaPluginList*		pluginlist_create_self(MochaContext *mc, MochaDecoder* decoder);
static MochaPlugin*			pluginlist_create_plugin(MochaContext *mc, MochaPluginList *pluginlist,
								XP_Bool byName, const char* targetName, MochaSlot targetSlot);
static MochaBoolean			pluginlist_get_property(MochaContext *mc, MochaObject *obj, MochaSlot slot,
                        		MochaDatum *dp);
static MochaBoolean			pluginlist_resolve_name(MochaContext *mc, MochaObject *obj, const char *name);
static void					pluginlist_finalize(MochaContext *mc, MochaObject *obj);
static MochaBoolean			pluginlist_refresh(MochaContext *mc, MochaObject *obj,
								uint argc, MochaDatum *argv, MochaDatum *rval);

static MochaMIMETypeList*	mimetypelist_create_self(MochaContext *mc, MochaDecoder* decoder);
static MochaMIMEType*		mimetypelist_create_mimetype(MochaContext* mc, MochaMIMETypeList* mimetypelist,
								XP_Bool byName, const char* targetName, MochaSlot targetSlot);
static MochaBoolean			mimetypelist_get_property(MochaContext *mc, MochaObject *obj, MochaSlot slot,
								MochaDatum *dp);
static MochaBoolean			mimetypelist_resolve_name(MochaContext *mc, MochaObject *obj, const char *name);
static void					mimetypelist_finalize(MochaContext *mc, MochaObject *obj);




/*
** -----------------------------------------------------------------------
**
** Reflection of a MIME type.
**
** -----------------------------------------------------------------------
*/

enum mimetype_slot
{
    MIMETYPE_TYPE           = -1,
    MIMETYPE_DESCRIPTION    = -2,
    MIMETYPE_SUFFIXES       = -3,
    MIMETYPE_ENABLEDPLUGIN  = -4
};


static MochaPropertySpec mimetype_props[] =
{
    {"type",        	MIMETYPE_TYPE,			MDF_ENUMERATE | MDF_READONLY |
												MDF_TAINTED},
    {"description", 	MIMETYPE_DESCRIPTION,	MDF_ENUMERATE | MDF_READONLY |
												MDF_TAINTED},
    {"suffixes",    	MIMETYPE_SUFFIXES,		MDF_ENUMERATE | MDF_READONLY |
												MDF_TAINTED},
    {"enabledPlugin",   MIMETYPE_ENABLEDPLUGIN,	MDF_BACKEDGE | MDF_ENUMERATE |
												MDF_READONLY | MDF_TAINTED},
    {0}
};


static MochaClass mimetype_class =
{
	"MimeType",
    mimetype_get_property, mimetype_get_property, MOCHA_ListPropStub,
    MOCHA_ResolveStub, MOCHA_ConvertStub, mimetype_finalize
};


/* Constructor method for a MochaMIMEType object */
static MochaMIMEType*
mimetype_create_self(MochaContext *mc, MochaDecoder* decoder,
					 char* type, char* description, char** suffixes,
                     uint32 numExtents, void* fileType)
{
    MochaObject *obj;
    MochaMIMEType *mimetype;

    mimetype = MOCHA_malloc(mc, sizeof(MochaMIMEType));
    if (!mimetype)
        return NULL;
    XP_BZERO(mimetype, sizeof *mimetype);

    obj = MOCHA_NewObject(mc, &mimetype_class, mimetype, NULL, NULL,
						  mimetype_props, NULL);
    if (!obj)
        goto error1;

    mimetype->decoder = decoder;
    mimetype->obj = obj;
    mimetype->fileType = fileType;

    mimetype->type = MOCHA_HoldAtom(mc, MOCHA_Atomize(mc, type));
    if (! mimetype->type)
        goto error2;

    mimetype->description = MOCHA_HoldAtom(mc, MOCHA_Atomize(mc, description));
    if (! mimetype->description)
        goto error3;


    /*
     * Assemble the list of file extensions into a string.
     * The extensions are stored in an array of individual strings, so we
     * first traverse the array to see how big the concatenated string will
     * be, then allocate the memory and re-traverse the array to build the
     * string.  Each extension is seperated by trailing comma and space.
     */
    {
        uint32 index;
        uint32 totalSize = 0;
        char *extStr;

        if (numExtents > 0)
        {
            for (index = 0; index < numExtents; index++)
                totalSize += XP_STRLEN(suffixes[index]);

            /* Add space for ', ' after each extension */
            totalSize += (2 * (numExtents - 1));
        }

        /* Total size plus terminator */
        extStr = XP_ALLOC(totalSize + 1);
        if (! extStr)
            goto error4;

        extStr[0] = 0;

        for (index = 0; index < numExtents; index++)
        {
            extStr = strcat(extStr, suffixes[index]);
            if (index < numExtents - 1)
                extStr = strcat(extStr, ", ");
        }
        mimetype->suffixes = MOCHA_HoldAtom(mc, MOCHA_Atomize(mc, extStr));
        XP_FREE(extStr);
        if (! mimetype->suffixes)
            goto error4;
    }

	/*
	 * Conjure up the Mocha object for the plug-in enabled for this
	 * MIME type.  First we get the name of the plug-in from libplugin;
	 * then we look up the plugin object by name in the global plug-in
	 * list (it's actually a "resolve", not a "lookup", so that the
	 * plug-in object will be created if it doesn't already exist).
	 */
	{
		char* pluginName = NPL_FindPluginEnabledForType(type);
		if (pluginName)
		{
			/* Look up the global plugin list object */
			MochaDatum datum;
			if (MOCHA_LookupName(mc, decoder->navigator, "plugins", &datum))
			{
				/* Look up the desired plugin by name in the list */
				MochaObject* pluginListObj = datum.u.obj;
				if (MOCHA_ResolveName(mc, pluginListObj, pluginName, &datum))
					mimetype->enabledPluginObj = datum.u.obj;
			}

			XP_FREE(pluginName);
		}
	}

    LM_LINK_OBJECT(mimetype->decoder, obj,
                   MOCHA_GetAtomName(mc, mimetype->type));
    return mimetype;


    /* Early exit */
  error4:
    MOCHA_DropAtom(mc, mimetype->description);
  error3:
    MOCHA_DropAtom(mc, mimetype->type);
  error2:
    MOCHA_DestroyObject(mc, obj);
  error1:
    MOCHA_free(mc, mimetype);
    return 0;
}


static MochaBoolean
mimetype_get_property(MochaContext *mc, MochaObject *obj, MochaSlot slot, MochaDatum *dp)
{
    MochaMIMEType *mimetype = obj->data;

	switch (slot)
    {
        case MIMETYPE_TYPE:
            MOCHA_INIT_DATUM(mc, dp, MOCHA_STRING, u.atom, mimetype->type);
            break;

        case MIMETYPE_DESCRIPTION:
            MOCHA_INIT_DATUM(mc, dp, MOCHA_STRING, u.atom, mimetype->description);
            break;

        case MIMETYPE_SUFFIXES:
            MOCHA_INIT_DATUM(mc, dp, MOCHA_STRING, u.atom, mimetype->suffixes);
            break;

        case MIMETYPE_ENABLEDPLUGIN:
            MOCHA_INIT_DATUM(mc, dp, MOCHA_OBJECT, u.obj, mimetype->enabledPluginObj);
            return MOCHA_TRUE;
            break;

        default:
            /* don't mess with user-defined props. */
            return MOCHA_TRUE;
    }

    if (!dp->u.atom)            /* Paranoia */
        dp->u.atom = MOCHA_empty.u.atom;
    if ((dp->flags & MDF_TAINTED) && dp->taint == MOCHA_TAINT_IDENTITY)
	    dp->taint = MOCHA_TAINT_MAX;
    return MOCHA_TRUE;
}


static void
mimetype_finalize(MochaContext *mc, MochaObject *obj)
{
    MochaMIMEType *mimetype = obj->data;

	LM_UNLINK_OBJECT(mimetype->decoder, obj);
	MOCHA_DropAtom(mc, mimetype->type);
	MOCHA_DropAtom(mc, mimetype->description);
	MOCHA_DropAtom(mc, mimetype->suffixes);
	MOCHA_free(mc, mimetype);
}






/*
** -----------------------------------------------------------------------
**
** Reflection of an installed plug-in.
**
** -----------------------------------------------------------------------
*/

enum plugin_slot
{
    PLUGIN_NAME         = -1,
    PLUGIN_FILENAME     = -2,
    PLUGIN_DESCRIPTION  = -3,
    PLUGIN_ARRAY_LENGTH = -4
};


static MochaPropertySpec plugin_props[] =
{
    {"name",        PLUGIN_NAME,            MDF_ENUMERATE | MDF_READONLY |
											MDF_TAINTED},
    {"filename",    PLUGIN_FILENAME,        MDF_ENUMERATE | MDF_READONLY |
											MDF_TAINTED},
    {"description", PLUGIN_DESCRIPTION,     MDF_ENUMERATE | MDF_READONLY |
											MDF_TAINTED},
    {"length",      PLUGIN_ARRAY_LENGTH,    MDF_ENUMERATE | MDF_READONLY |
											MDF_TAINTED},
    {0}
};



static MochaClass plugin_class =
{
	"Plugin",
    plugin_get_property, plugin_get_property, MOCHA_ListPropStub,
    plugin_resolve_name, MOCHA_ConvertStub, plugin_finalize
};


/* Constructor method for a MochaPlugin object */
static MochaPlugin*
plugin_create_self(MochaContext *mc, MochaDecoder* decoder, char* name,
				   char* filename, char* description, NPReference plugref)
{
    MochaObject *obj;
    MochaPlugin *plugin;

    plugin = MOCHA_malloc(mc, sizeof(MochaPlugin));
    if (!plugin)
        return NULL;
    XP_BZERO(plugin, sizeof *plugin);

    obj = MOCHA_NewObject(mc, &plugin_class, plugin, NULL, NULL,
						  plugin_props, NULL);
    if (!obj)
        goto error1;

    /* Fill out static property fields */
    plugin->decoder = decoder;
    plugin->obj = obj;
    plugin->plugref = plugref;

    plugin->name = MOCHA_HoldAtom(mc, MOCHA_Atomize(mc, name));
    if (! plugin->name)
        goto error2;

    plugin->filename = MOCHA_HoldAtom(mc, MOCHA_Atomize(mc, filename));
    if (! plugin->filename)
        goto error3;

    plugin->description = MOCHA_HoldAtom(mc, MOCHA_Atomize(mc, description));
    if (! plugin->description)
        goto error4;

	/* Count how many MIME types we have */
	{
        NPReference typeref = NPRefFromStart;
        while (NPL_IteratePluginTypes(&typeref, plugref, NULL, NULL, NULL, NULL))
        	plugin->length++;
	}

    LM_LINK_OBJECT(plugin->decoder, obj, MOCHA_GetAtomName(mc, plugin->name));
    return plugin;

    /* Early exit */
  error4:
    MOCHA_DropAtom(mc, plugin->filename);
  error3:
    MOCHA_DropAtom(mc, plugin->name);
  error2:
    MOCHA_DestroyObject(mc, obj);
  error1:
    MOCHA_free(mc, plugin);
    return 0;
}


/* Create a mimetype property for a plugin for a specified slot or name */
static MochaMIMEType*
plugin_create_mimetype(MochaContext* mc, MochaPlugin* plugin, XP_Bool byName,
					  const char* targetName, MochaSlot targetSlot)
{
    NPMIMEType type = NULL;
    char** suffixes = NULL;
    char* description = NULL;
    void* fileType = NULL;
    NPReference typeref = NPRefFromStart;
    MochaSlot slot = 0;
    XP_Bool found = FALSE;

	/* Search for the type (by type name or slot number) */
    while (NPL_IteratePluginTypes(&typeref, plugin->plugref, &type,
    							  &suffixes, &description, &fileType))
	{
		if (byName)
			found = (type && (XP_STRCMP(targetName, type) == 0));
		else
			found = (targetSlot == slot);

		if (found)
			break;

        slot++;
	}

	/* Found the mime type, so create an object and property */
	if (found)
    {
    	MochaMIMEType* mimetype;
    	uint32 numExtents = 0;

    	while (suffixes[numExtents])
    		numExtents++;

        mimetype = mimetype_create_self(mc, plugin->decoder, type, description,
        								suffixes, numExtents, fileType);
		if (mimetype)
		{
			MochaDatum datum;
		    MOCHA_INIT_FULL_DATUM(mc, &datum, MOCHA_OBJECT,
								  MDF_ENUMERATE | MDF_READONLY,
		    					  MOCHA_TAINT_IDENTITY,
								  u.obj, mimetype->obj);
		    MOCHA_SetProperty(mc, plugin->obj,
		    				  MOCHA_GetAtomName(mc, mimetype->type),
		                      slot, datum);
		}

		return mimetype;
   	}
	return NULL;
}


static MochaBoolean
plugin_get_property(MochaContext *mc, MochaObject *obj, MochaSlot slot, MochaDatum *dp)
{
    MochaPlugin *plugin = obj->data;

	/* Prevent infinite recursion through call to GetSlot below */
	if (plugin->reentrant)
		return MOCHA_TRUE;

    switch (slot)
    {
        case PLUGIN_NAME:
            MOCHA_INIT_DATUM(mc, dp, MOCHA_STRING, u.atom, plugin->name);
            break;

        case PLUGIN_FILENAME:
            MOCHA_INIT_DATUM(mc, dp, MOCHA_STRING, u.atom, plugin->filename);
            break;

        case PLUGIN_DESCRIPTION:
            MOCHA_INIT_DATUM(mc, dp, MOCHA_STRING, u.atom, plugin->description);
            break;

        case PLUGIN_ARRAY_LENGTH:
            MOCHA_INIT_DATUM(mc, dp, MOCHA_NUMBER, u.fval, plugin->length);
            return MOCHA_TRUE;

        default:
            /* Don't mess with a user-defined property. */
            if (slot < 0)
                break;

            /* Ignore requests for slots greater than the number of mime types */
            if (slot < plugin->length)
            {
			    /* Search for an existing MochaMIMEType for this slot */
			    MochaObject* mimetypeObj = NULL;
				MochaDatum datum;

				plugin->reentrant = TRUE;
			    if (MOCHA_GetSlot(mc, obj, slot, &datum) && datum.u.obj)
			    	mimetypeObj = datum.u.obj;
			    else
			    {
				    MochaMIMEType* mimetype;
                	mimetype = plugin_create_mimetype(mc, plugin, FALSE, NULL, slot);
                	if (mimetype)
                		mimetypeObj = mimetype->obj;
                }
				plugin->reentrant = FALSE;

				MOCHA_INIT_DATUM(mc, dp, MOCHA_OBJECT, u.obj, mimetypeObj);
                return MOCHA_TRUE;
            }
            else
      			return MOCHA_FALSE;
    }

	if (!dp->u.atom)           /* Paranoia */
		dp->u.atom = MOCHA_empty.u.atom;
	if ((dp->flags & MDF_TAINTED) && dp->taint == MOCHA_TAINT_IDENTITY)
	    dp->taint = MOCHA_TAINT_MAX;
	return MOCHA_TRUE;
}


static MochaBoolean
plugin_resolve_name(MochaContext *mc, MochaObject *obj, const char *name)
{
	MochaDatum datum;
    MochaPlugin* plugin = obj->data;
    if (!plugin)
    	return MOCHA_FALSE;

	/* Prevent infinite recursion through call to LookupName below */
	if (plugin->reentrant)
		return MOCHA_TRUE;

    /* Look for a MochaMIMEType object by this name already in our list */
    plugin->reentrant = TRUE;
    if (MOCHA_LookupName(mc, obj, name, &datum) && datum.u.obj)
    {
		plugin->reentrant = FALSE;
    	return MOCHA_TRUE;
    }
	plugin->reentrant = FALSE;

	/* We don't already have the object, so make a new one */
	if (plugin_create_mimetype(mc, plugin, TRUE, name, 0))
		return MOCHA_TRUE;

    /* Still no match for the name?  Must be some other property, or invalid. */
    return MOCHA_TRUE;
}


static void
plugin_finalize(MochaContext *mc, MochaObject *obj)
{
    MochaPlugin* plugin = obj->data;

	LM_UNLINK_OBJECT(plugin->decoder, obj);
	MOCHA_DropAtom(mc, plugin->name);
	MOCHA_DropAtom(mc, plugin->filename);
	MOCHA_DropAtom(mc, plugin->description);
	MOCHA_free(mc, plugin);
}






/*
** -----------------------------------------------------------------------
**
** Reflection of the list of installed plug-ins.
** The only static property is the array length;
** the array elements (MochaPlugins) are added
** lazily when referenced.
**
** -----------------------------------------------------------------------
*/

enum pluginlist_slot
{
    PLUGINLIST_ARRAY_LENGTH = -1
};


static MochaPropertySpec pluginlist_props[] =
{
    {"length",  PLUGINLIST_ARRAY_LENGTH,    MDF_ENUMERATE | MDF_READONLY},
    {0}
};


static MochaClass pluginlist_class =
{
	"PluginArray",
    pluginlist_get_property, pluginlist_get_property, MOCHA_ListPropStub,
    pluginlist_resolve_name, MOCHA_ConvertStub, pluginlist_finalize
};


static MochaFunctionSpec pluginlist_methods[] =
{
    {"refresh", pluginlist_refresh, 0},
    {0}
};


static MochaBoolean
PluginList(MochaContext *mc, MochaObject *obj,
	 uint argc, MochaDatum *argv, MochaDatum *rval)
{
    return MOCHA_TRUE;
}


/* Constructor method for a MochaPluginList object */
static MochaPluginList*
pluginlist_create_self(MochaContext *mc, MochaDecoder* decoder)
{
    MochaObject *obj;
    MochaPluginList *pluginlist;

    pluginlist = MOCHA_malloc(mc, sizeof(MochaPluginList));
    if (!pluginlist)
        return NULL;
    XP_BZERO(pluginlist, sizeof *pluginlist);

    obj = MOCHA_InitClass(mc, decoder->window_object, &pluginlist_class, pluginlist,
    					  PluginList, 0, pluginlist_props, pluginlist_methods, 0, 0);
    if (!obj)
    {
        MOCHA_free(mc, pluginlist);
        return NULL;
    }

    pluginlist->decoder = decoder;
    pluginlist->obj = obj;

    {
        /* Compute total number of plug-ins (potential slots) */
        NPReference ref = NPRefFromStart;
        while (NPL_IteratePluginFiles(&ref, NULL, NULL, NULL))
            pluginlist->length++;
    }

    LM_LINK_OBJECT(pluginlist->decoder, obj, "aPluginList");
    return pluginlist;
}


/* Look up the plugin for the specified slot of the plug-in list */
static MochaPlugin*
pluginlist_create_plugin(MochaContext *mc, MochaPluginList *pluginlist, XP_Bool byName,
					  	 const char* targetName, MochaSlot targetSlot)
{
    char* plugname = NULL;
    char* filename = NULL;
    char* description = NULL;
    NPReference plugref = NPRefFromStart;
    MochaSlot slot = 0;
    XP_Bool found = FALSE;

	/* Search for the plug-in (by name or slot number) */
    while (NPL_IteratePluginFiles(&plugref, &plugname, &filename, &description))
    {
		if (byName)
			found = (plugname && (XP_STRCMP(targetName, plugname) == 0));
		else
			found = (targetSlot == slot);

		if (found)
			break;

        slot++;
    }

	/* Found the plug-in, so create an object and property */
    if (found)
    {
		MochaPlugin* plugin;
		plugin = plugin_create_self(mc, pluginlist->decoder, plugname,
									filename, description, plugref);
        if (plugin)
        {
			MochaDatum datum;
		    MOCHA_INIT_FULL_DATUM(mc, &datum, MOCHA_OBJECT,
								  MDF_ENUMERATE | MDF_READONLY,
		    					  MOCHA_TAINT_IDENTITY,
								  u.obj, plugin->obj);
		    MOCHA_SetProperty(mc, pluginlist->obj,
		    				  MOCHA_GetAtomName(mc, plugin->name),
		                      slot, datum);
        }

        return plugin;
    }
    else
    	return NULL;
}


static MochaBoolean
pluginlist_get_property(MochaContext *mc, MochaObject *obj, MochaSlot slot, MochaDatum *dp)
{
    MochaPluginList *pluginlist = obj->data;

	/* Prevent infinite recursion through call to GetSlot below */
	if (pluginlist->reentrant)
		return MOCHA_TRUE;

    switch (slot)
    {
        case PLUGINLIST_ARRAY_LENGTH:
            MOCHA_INIT_DATUM(mc, dp, MOCHA_NUMBER, u.fval, pluginlist->length);
            break;

        default:
            /* Don't mess with a user-defined property. */
            if (slot < 0)
                break;

            /* Ignore requests for slots greater than the number of plug-ins */
            if (slot < pluginlist->length)
            {
			    /* Search for an existing MochaPlugin for this slot */
			    MochaObject* pluginObj = NULL;
				MochaDatum datum;

				pluginlist->reentrant = TRUE;
			    if (MOCHA_GetSlot(mc, obj, slot, &datum) && datum.u.obj)
			    	pluginObj = datum.u.obj;
			    else
			    {
			    	MochaPlugin* plugin;
                	plugin = pluginlist_create_plugin(mc, pluginlist, FALSE, NULL, slot);
                	if (plugin)
                		pluginObj = plugin->obj;
                }
				pluginlist->reentrant = FALSE;

				MOCHA_INIT_DATUM(mc, dp, MOCHA_OBJECT, u.obj, pluginObj);
                return MOCHA_TRUE;
            }
            break;
    }

    return MOCHA_TRUE;
}


static MochaBoolean
pluginlist_resolve_name(MochaContext *mc, MochaObject *obj, const char *name)
{
	MochaDatum datum;
    MochaPluginList* pluginlist = obj->data;
    if (!pluginlist)
    	return MOCHA_FALSE;

	/* Prevent infinite recursion through call to LookupName below */
	if (pluginlist->reentrant)
		return MOCHA_TRUE;

    /* Look for a MochaMIMEType object by this name already in our list */
	pluginlist->reentrant = TRUE;
    if (MOCHA_LookupName(mc, obj, name, &datum) && datum.u.obj)
    {
		pluginlist->reentrant = FALSE;
    	return MOCHA_TRUE;
    }
	pluginlist->reentrant = FALSE;

	/* We don't already have the object, so make a new one */
	if (pluginlist_create_plugin(mc, pluginlist, TRUE, name, 0))
		return MOCHA_TRUE;

    /* Still no match for the name?  Must be some other property, or invalid. */
    return MOCHA_TRUE;
}


static void
pluginlist_finalize(MochaContext *mc, MochaObject *obj)
{
    MochaPluginList* pluginlist = obj->data;

	LM_UNLINK_OBJECT(pluginlist->decoder, obj);
	MOCHA_free(mc, pluginlist);
}


MochaBoolean
pluginlist_refresh(MochaContext *mc, MochaObject *obj,
	  uint argc, MochaDatum *argv, MochaDatum *rval)
{
	MochaBoolean value = MOCHA_FALSE;
	MochaPluginList* pluginlist;
	MochaObject* navigator;
	MochaDecoder* decoder;
	NPError error;

    if (!MOCHA_InstanceOf(mc, obj, &pluginlist_class, argv[-1].u.fun))
        return MOCHA_FALSE;
	pluginlist = (MochaPluginList*) obj->data;
	decoder = pluginlist->decoder;

	/*
	 * Evaluate the parameter (if any).  If the parameter
	 * is missing or can't be evaluated, default to FALSE.
	 */
	if (argc > 0)
		(void) MOCHA_DatumToBoolean(mc, argv[0], &value);

	error = NPL_RefreshPluginList(value);
	if (error != NPERR_NO_ERROR)
	{
		/* XXX should MOCHA_ReportError() here, but can't happen currently */
		return MOCHA_FALSE;
	}

	/*
	 * Remove references to the existing navigator object,
	 * and make a new one.  If scripts have outstanding
	 * references to the old objects, they'll still be
	 * valid, but if they reference navigator.plugins
	 * anew they'll see any new plug-ins registered by
	 * NPL_RefreshPlugins.
	 */
	navigator = decoder->navigator;
	decoder->navigator = NULL;		/* Prevent lm_DefineNavigator from short-circuiting */
	decoder->navigator = lm_DefineNavigator(decoder);
	if (!decoder->navigator)
	{
		/*
		 * We failed to create a new navigator object, so
		 * restore the old one (saved in a local variable).
		 */
		decoder->navigator = navigator;
		return MOCHA_FALSE;
	}
	/*
	 * We succeeded in creating a new navigator object,
	 * so we can drop the old one.
	 */
	MOCHA_DropObject(mc, navigator);
	return MOCHA_TRUE;
}



/*
** -----------------------------------------------------------------------
**
** Reflection of the list of handled MIME types.
**
** -----------------------------------------------------------------------
*/

enum mimetypelist_slot
{
    MIMETYPELIST_ARRAY_LENGTH   = -1
};


static MochaPropertySpec mimetypelist_props[] =
{
    {"length",  MIMETYPELIST_ARRAY_LENGTH,  MDF_ENUMERATE | MDF_READONLY},
    {0}
};


static MochaClass mimetypelist_class =
{
	"MimeTypeArray",
    mimetypelist_get_property, mimetypelist_get_property, MOCHA_ListPropStub,
    mimetypelist_resolve_name, MOCHA_ConvertStub, mimetypelist_finalize
};


/* Constructor method for a MochaMIMETypeList object */
static MochaMIMETypeList*
mimetypelist_create_self(MochaContext *mc, MochaDecoder* decoder)
{
    MochaObject *obj;
    MochaMIMETypeList *mimetypelist;

    mimetypelist = MOCHA_malloc(mc, sizeof(MochaMIMETypeList));
    if (!mimetypelist)
        return NULL;
    XP_BZERO(mimetypelist, sizeof *mimetypelist);

    obj = MOCHA_NewObject(mc, &mimetypelist_class, mimetypelist, NULL, NULL,
                          mimetypelist_props, NULL);
    if (!obj)
    {
        MOCHA_free(mc, mimetypelist);
        return NULL;
    }

    mimetypelist->decoder = decoder;
    mimetypelist->obj = obj;

	/*
	 * Count the number of types in the list.
	 * We can't just go by the number of items
	 * in the list, since it contains encodings, too.
	 */
	{
		XP_List* cinfoList = cinfo_MasterListPointer();
		NET_cdataStruct* cdata = NULL;
    	mimetypelist->length = 0;
    	while (cinfoList)
    	{
    		cdata = cinfoList->object;
    		if (cdata && cdata->ci.type)
    			mimetypelist->length++;
	        cinfoList = cinfoList->next;
	    }
	}

    LM_LINK_OBJECT(mimetypelist->decoder, obj, "aMimeTypeList");
    return mimetypelist;
}


/* Create a mimetype property for a specified slot or name */
static MochaMIMEType*
mimetypelist_create_mimetype(MochaContext* mc, MochaMIMETypeList* mimetypelist,
					   		 XP_Bool byName, const char* targetName, MochaSlot targetSlot)
{
	XP_List* cinfoList = cinfo_MasterListPointer();
	NET_cdataStruct* cdata = NULL;
	XP_Bool found = FALSE;

	if (byName)
	{
		/* Look up by name */
		targetSlot = 0;
	    while (cinfoList)
	    {
        	cdata = cinfoList->object;
        	if (cdata)
        	{
				char* type = cdata->ci.type;
		        if (type && (XP_STRCMP(type, targetName) == 0))
		        {
			       	found = TRUE;
		        	break;
		        }
		    }
	        targetSlot++;
	        cinfoList = cinfoList->next;
		}
    }
    else
    {
    	/*
    	 * Look up by slot.
    	 * The slot number does not directly correspond to list index,
    	 * since not all items in the list correspond to properties
    	 * (encodings are in list list as well as types).
    	 */
    	uint32 count = targetSlot + 1;
    	while (cinfoList)
    	{
    		cdata = cinfoList->object;
    		if (cdata && cdata->ci.type)
    			count--;
    			
    		if (count == 0)
    		{
    			found = TRUE;
    			break;
    		}

	        cinfoList = cinfoList->next;
	    }
    }

	if (found)
	{
	    MochaMIMEType* mimetype;
        mimetype = mimetype_create_self(mc, mimetypelist->decoder, cdata->ci.type,
        								cdata->ci.desc, cdata->exts, cdata->num_exts, NULL);
        if (mimetype)
		{
			MochaDatum datum;
		    MOCHA_INIT_FULL_DATUM(mc, &datum, MOCHA_OBJECT,
								  MDF_ENUMERATE | MDF_READONLY,
		    					  MOCHA_TAINT_IDENTITY,
								  u.obj, mimetype->obj);
		    MOCHA_SetProperty(mc, mimetypelist->obj,
		    				  MOCHA_GetAtomName(mc, mimetype->type),
		                      targetSlot, datum);
		}

		return mimetype;
	}

	return NULL;
}


static MochaBoolean
mimetypelist_get_property(MochaContext *mc, MochaObject *obj, MochaSlot slot,
                          MochaDatum *dp)
{
    MochaMIMETypeList *mimetypelist = obj->data;

	/* Prevent infinite recursion through call to GetSlot below */
	if (mimetypelist->reentrant)
		return MOCHA_TRUE;

    switch (slot)
    {
        case MIMETYPELIST_ARRAY_LENGTH:
            MOCHA_INIT_DATUM(mc, dp, MOCHA_NUMBER, u.fval, mimetypelist->length);
            break;

        default:
            /* Don't mess with a user-defined property. */
            if (slot < 0)
                break;

            /* Ignore requests for slots greater than the number of mime types */
            if (slot < mimetypelist->length)
            {
			    /* Search for an existing MochaMIMEType for this slot */
			    MochaObject* mimetypeObj = NULL;
				MochaDatum datum;

				mimetypelist->reentrant = TRUE;
			    if (MOCHA_GetSlot(mc, obj, slot, &datum) && datum.u.obj)
			    	mimetypeObj = datum.u.obj;
			    else
			    {
			    	MochaMIMEType* mimetype;
                	mimetype = mimetypelist_create_mimetype(mc, mimetypelist, FALSE, NULL, slot);
					if (mimetype)
						mimetypeObj = mimetype->obj;
				}
				mimetypelist->reentrant = FALSE;

				MOCHA_INIT_DATUM(mc, dp, MOCHA_OBJECT, u.obj, mimetypeObj);
                return MOCHA_TRUE;
            }
            break;
    }

    return MOCHA_TRUE;
}


static MochaBoolean
mimetypelist_resolve_name(MochaContext *mc, MochaObject *obj, const char *name)
{
	MochaDatum datum;
    MochaMIMETypeList* mimetypelist = obj->data;
    if (!mimetypelist)
    	return MOCHA_FALSE;

	/* Prevent infinite recursion through call to LookupName below */
	if (mimetypelist->reentrant)
		return MOCHA_TRUE;

    /* Look for a MochaMIMEType object by this name already in our list */
    mimetypelist->reentrant = TRUE;
    if (MOCHA_LookupName(mc, obj, name, &datum) && datum.u.obj)
    {
		mimetypelist->reentrant = FALSE;
    	return MOCHA_TRUE;
    }
	mimetypelist->reentrant = FALSE;

	/* We don't already have the object, so make a new one */
	if (mimetypelist_create_mimetype(mc, mimetypelist, TRUE, name, 0))
		return MOCHA_TRUE;

    /* Still no match for the name?  Must be some other property, or invalid. */
    return MOCHA_TRUE;
}


static void
mimetypelist_finalize(MochaContext *mc, MochaObject *obj)
{
    MochaMIMETypeList *mimetypelist = obj->data;

	LM_UNLINK_OBJECT(mimetypelist->decoder, obj);
	MOCHA_free(mc, mimetypelist);
}




/*
 * Exported entry point, called from lm_nav.c.
 * This function creates the MochaPluginList object.  The
 * properties for the installed plug-ins are instantiated
 * lazily in pluginlist_resolve_name.
 */
MochaObject*
lm_NewPluginList(MochaContext *mc)
{
    MochaDecoder* decoder;
	MochaPluginList* pluginlist;

	decoder = MOCHA_GetGlobalObject(mc)->data;
	pluginlist = pluginlist_create_self(mc, decoder);
	return (pluginlist ? pluginlist->obj : NULL);
}




/*
 * Exported entry point to this file, called from lm_nav.c.
 * This function creates the MochaMIMETypeList object.  The
 * properties for the MIME types are instantiated
 * lazily in mimetypelist_resolve_name.
 */
MochaObject*
lm_NewMIMETypeList(MochaContext *mc)
{
    MochaDecoder* decoder;
	MochaMIMETypeList* mimetypelist;

	decoder = MOCHA_GetGlobalObject(mc)->data;
	mimetypelist = mimetypelist_create_self(mc, decoder);
	return (mimetypelist ? mimetypelist->obj : NULL);
}


