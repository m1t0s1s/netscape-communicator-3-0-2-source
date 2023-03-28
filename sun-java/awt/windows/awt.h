/*
 * @(#)awt.h	1.20 95/11/29 Patrick P. Chan
 *
 * Copyright (c) 1995 Sun Microsystems, Inc. All Rights Reserved.
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
#ifndef _AWT_H_
#define _AWT_H_

#include "stdafx.h"

extern "C" {
#include <native.h>
#include <monitor.h>
#include <oobj.h>
#include <interpreter.h>
#include <tree.h>
#include <javaString.h>

#include "java_awt_Button.h"
#include "java_awt_Canvas.h"
#include "java_awt_Checkbox.h"
#include "java_awt_CheckboxMenuItem.h"
#include "java_awt_Choice.h"
#include "java_awt_Color.h"
#include "java_awt_Component.h"
#include "java_awt_Event.h"
#include "java_awt_FileDialog.h"
#include "java_awt_Font.h"
#include "java_awt_FontMetrics.h"
#include "java_awt_Frame.h"
#include "java_awt_Graphics.h"
#include "java_awt_Image.h"
#include "java_awt_Label.h"
#include "java_awt_List.h"
#include "java_awt_Menu.h"
#include "java_awt_MenuBar.h"
#include "java_awt_MenuItem.h"
#include "java_awt_Rectangle.h"
#include "java_awt_Scrollbar.h"
#include "java_awt_TextArea.h"
#include "java_awt_TextField.h"
#include "java_awt_Window.h"
#include "java_awt_Insets.h"
#include "java_awt_FileDialog.h"
#include "java_awt_image_ColorModel.h"
#include "java_awt_image_IndexColorModel.h"
#include "sun_awt_image_ImageRepresentation.h"
#include "sun_awt_image_OffScreenImageSource.h"
#include "sun_awt_win32_MCanvasPeer.h"
#include "sun_awt_win32_MButtonPeer.h"
#include "sun_awt_win32_MCheckboxPeer.h"
#include "sun_awt_win32_MCheckboxMenuItemPeer.h"
#include "sun_awt_win32_MChoicePeer.h"
#include "sun_awt_win32_MComponentPeer.h"
#include "sun_awt_win32_MDialogPeer.h" 
#include "sun_awt_win32_MFileDialogPeer.h"
#include "sun_awt_win32_MFramePeer.h"
#include "sun_awt_win32_MLabelPeer.h"
#include "sun_awt_win32_MListPeer.h"
#include "sun_awt_win32_MMenuBarPeer.h"
#include "sun_awt_win32_MMenuItemPeer.h"
#include "sun_awt_win32_MMenuPeer.h"
#include "sun_awt_win32_MScrollbarPeer.h"
#include "sun_awt_win32_MTextAreaPeer.h"
#include "sun_awt_win32_MTextFieldPeer.h"
#include "sun_awt_win32_MToolkit.h"
#include "sun_awt_win32_MWindowPeer.h"
#include "sun_awt_win32_Win32FontMetrics.h"
#include "sun_awt_win32_Win32Graphics.h"
#include "sun_awt_win32_Win32Image.h"


//temp
typedef int Hjava_awt_MessageDialog;
//typedef int Hjava_awt_ChoiceHandler;
typedef int Hawt_WServer;
typedef int Hawt_WSFontMetrics;
typedef int Classjava_awt_DIBitmap;
typedef int Hjava_awt_DIBitmap;


#define UNIMPLEMENTED() \
SignalError(0, JAVAPKG "NullPointerException","unimplemented")


// This stuff enables heap checking before and after every awt call.
//#define HEAPCHECKING_ENABLED
#ifdef HEAPCHECKING_ENABLED
#include <malloc.h>
#define CHECKHEAP {int code = _heapchk(); ASSERT(code == _HEAPOK && AfxCheckMemory()); }
#else
#define CHECKHEAP ((void)0)
#endif

#if defined(NETSCAPE) 
#if defined(DEBUG)
#include "prlog.h"
#include "prprf.h"

extern int DebugPrintf(const char *fmt, ...);
#define AWT_TRACE(__args) if (GetApp()->bTraceAwt) DebugPrintf __args

#else

#define AWT_TRACE(__args) 
#endif /* !DEBUG */

#else   
#define AWT_TRACE(__args) if (GetApp()->bTraceAwt) printf __args
#endif  /* !NETSCAPE */

#define PDATA(T, x) ((struct T *)(unhand(x)->pData))
#define SET_PDATA(x, d) (unhand(x)->pData = (int)d)
#define PEER_PDATA(T, T2, x) ((struct T *)(unhand((struct T2 *)unhand(x)->peer)->pData))

#define CHECK_NULL(p, m) {\
	if (p) {\
		SignalError(0, JAVAPKG "NullPointerException", m);\
		return;\
	}\
}
#define CHECK_NULL_RETURN(p, m) {\
	if (p) {\
		SignalError(0, JAVAPKG "NullPointerException", m);\
		return 0;\
	}\
}

#define GetApp() ((CAwtApp *)AfxGetApp())

} // extern "C"

//
// Maintain a list of all allocated AWT objects.  This list is dumped out
// when the DLL is being unloaded
//
#if defined(NETSCAPE) && defined(DEBUG)
#include "prclist.h"

struct AwtCList : PRCList {
    char *name;
};

extern AwtCList __AllAwtObjects;

#define INITIALIZE_AWT_LIST     PR_INIT_CLIST( &__AllAwtObjects );
#define DEFINE_AWT_LIST_LINK    AwtCList link;
#define INITIALIZE_AWT_LIST_LINK(__n )                                      \
                                PR_INIT_CLIST( &link );                     \
                                PR_APPEND_LINK( &link, &__AllAwtObjects );  \
                                link.name = __n;
#define REMOVE_AWT_LIST_LINK    PR_REMOVE_LINK( &link );

#else   /* !NETSCAPE */
#define INITIALIZE_AWT_LIST
#define DEFINE_AWT_LIST_LINK
#define INITIALIZE_AWT_LIST_LINK(name)
#define REMOVE_AWT_LIST_LINK

#endif /* NETSCAPE */

#endif           /* _AWT_H_ */
