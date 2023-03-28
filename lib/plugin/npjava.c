/* -*- Mode: C; tab-width: 4; -*- */

#if 0
/*
** Ok, so we changed our minds, and we're not doing this fancy plugin
** stuff for now. There are several reasons:
**
** 1. Plugins were defined as subclasses of applets, but never get their
** own thread of control as applets do. This means although they have all
** the applet features, they never get to start as applets do. Also other
** applets can't find them on the page because they're never started. So
** we decided not to make them applets for now.
**
** 2. All this stuff to give plugins an output stream that feeds back
** into the native NPP_Write proc probably isn't needed. It might be
** nice, but we're not sure. We should get more experience with Java
** reflections of plugins first.
**
** 3. It wasn't implemented anyway.
*/

#include "nppriv.h"
#include "gui.h"			/* For XP_AppCodeName */
#include "prlog.h"
#include "prmem.h"
#include "java.h"

#include "netscape_plugin_Plugin.h"
#ifdef XP_MAC
#include "n_a_PluginOutputStream.h"
#else
#include "netscape_plugin_PluginOutputStream.h"
#endif

/******************************************************************************/

void
netscape_plugin_PluginOutputStream_open(
	JavaEnv* env, struct Hnetscape_plugin_PluginOutputStream* self)
{
	np_stream* stream;
	stream = NULL;		/* XXX */
	set_netscape_plugin_PluginOutputStream_pluginStream(env, self,
														(jint)stream);
}

void
netscape_plugin_PluginOutputStream_close(
	JavaEnv* env, struct Hnetscape_plugin_PluginOutputStream* self)
{
/*
	np_stream* stream = (np_stream*)
		netscape_plugin_PluginOutputStream_pluginStream_get(env, self);
		*/
	/* XXX */
	set_netscape_plugin_PluginOutputStream_pluginStream(env, self, 0);
}

/******************************************************************************/

typedef struct MozillaEvent_PluginWriteBytes {
    MWContextEvent	ce;				/* must be first */
	np_instance*	instance;
	const jbyte*	bytes;
	jint			length;
} MozillaEvent_PluginWriteBytes;

static void
npj_HandleEvent_PluginWriteBytes(MozillaEvent_PluginWriteBytes* e)
{
	
}

static void
npj_writeBytes(
	JavaEnv* env, struct Hnetscape_plugin_PluginOutputStream* self,
	const jbyte* bytes, int length)
{
	struct Hnetscape_plugin_Plugin* plugin = 
		get_netscape_plugin_PluginOutputStream_plugin(env, self);
	NPP npp = (NPP)get_netscape_plugin_Plugin_npp(env, plugin);
	np_instance* instance = (np_instance*)npp->ndata;
	MozillaEvent_PluginWriteBytes* event =
		PR_NEW(MozillaEvent_PluginWriteBytes);
    PR_InitEvent(&event->ce.event,
				 (PRHandleEventProc)npj_HandleEvent_PluginWriteBytes,
				 (PRDestroyEventProc)free);
	event->ce.context = instance->cx;
	event->instance = instance;
	event->bytes = bytes;
	event->length = length;
	(void)PR_PostSynchronousEvent(mozilla_event_queue, &event->ce.event);
}

void
netscape_plugin_PluginOutputStream_write(
	JavaEnv* env, struct Hnetscape_plugin_PluginOutputStream* self,
	jint byte)
{
	jbyte b = (jbyte)byte;
	npj_writeBytes(env, self, &b, 1);
}

void
netscape_plugin_PluginOutputStream_writeBytes(
	JavaEnv* env, struct Hnetscape_plugin_PluginOutputStream* self,
	jbyteArray bytes, jint off, jint len)
{
	const jbyte* b;
	Java_getByteArrayBytes(env, bytes, off, len, &b);
	if (!Java_exceptionOccurred(env)) {
		npj_writeBytes(env, self, b, len);
	}
}

/******************************************************************************/

void
netscape_plugin_Plugin_print(
    JavaEnv* env, struct Hnetscape_plugin_Plugin* self)
{
	NPP npp = (NPP)get_netscape_plugin_Plugin_npp(env, plugin);
	np_instance* instance = (np_instance*)npp->ndata;
    np_handle *handle = instance->handle;
    void* pdata = NULL;	/* XXX */

    if (handle && handle->f && ISFUNCPTR(handle->f->print))
	CallNPP_PrintProc(handle->f->print, instance->npp, pdata);
}

struct Hjava_lang_String *
netscape_plugin_Plugin_getUserAgent(
    JavaEnv* env, struct Hnetscape_plugin_Plugin* self)
{
	extern char* npn_useragent(NPP);
	char* uabuf = npn_useragent(NULL);
    return Java_newStringUTF(env, uabuf, strlen(uabuf));
}

/******************************************************************************/
#endif /* 0 */
