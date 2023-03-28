/**********************************************************************
 e_kit.h
 By Daniel Malmer
 10 Jan 1996

**********************************************************************/

#ifndef __e_kit_h
#define __e_kit_h

#include <X11/Intrinsic.h>

#include "menu.h"
#include "icondata.h"

extern void ekit_Initialize(Widget toplevel);
extern void ekit_DefaultPrefs(XFE_GlobalPrefs* prefs);
extern void ekit_LockPrefs(void);
extern void ekit_AddDirectoryMenu(Widget toplevel);
extern void ekit_AddHelpMenu(Widget toplevel, MWContextType context_type);
extern struct fe_button* ekit_GetDirectoryButtons(void);
extern int ekit_NumDirectoryButtons(void);

extern char* ekit_AnimationFile(void);
extern char* ekit_LogoUrl(void);
extern char* ekit_HomePage(void);
extern char* ekit_UserAgent(void);
extern char* ekit_AutoconfigUrl(void);
extern char* ekit_PopServer(void);
extern char* ekit_PopName(void);
extern Boolean ekit_MsgSizeLimit(void);

extern char* ekit_AboutData(void);
extern Boolean ekit_Enabled(void);

#endif
