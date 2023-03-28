/*
 *  nppg.h $Revision: 1.13 $
 *  Prototypes for functions exported by the FEs and called by libplugin.
 *  Some (perhaps all) of these prototypes could be moved to fe_proto.h.
 *  Protypes for functions exported by libplugin are in np.h.
 */

#ifndef _NPPG_H
#define _NPPG_H

#include "npapi.h"
#include "npupp.h"

XP_BEGIN_PROTOS

extern void 			FE_RegisterPlugins(void);
extern NPPluginFuncs*	FE_LoadPlugin(void *plugin, NPNetscapeFuncs *);
extern void				FE_UnloadPlugin(void *plugin);
extern void				FE_EmbedURLExit(URL_Struct *urls, int status, MWContext *cx);
extern void				FE_ShowScrollBars(MWContext *context, XP_Bool show);
#ifdef XP_WIN
extern void 			FE_FreeEmbedSessionData(MWContext *context, NPEmbeddedApp* pApp);
#endif
#ifdef XP_MAC
extern void 			FE_PluginProgress(MWContext *context, const char *message);
extern void 			FE_ResetRefreshURLTimer(MWContext *context);
#endif
#ifdef XP_UNIX
extern NPError			FE_PluginGetValue(void *pdesc, NPNVariable variable, void *r_value);
#endif /* XP_UNIX */

#ifdef XP_WIN32
void*	WFE_BeginSetModuleState();
void	WFE_EndSetModuleState(void*);
#endif

XP_END_PROTOS

#endif /* _NPPG_H */
