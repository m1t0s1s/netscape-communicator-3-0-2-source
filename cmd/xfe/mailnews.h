/* -*- Mode: C; tab-width: 8 -*-
   mailnews.h --- includes for the X front end mail/news stuff.
   Copyright © 1995 Netscape Communications Corporation, all rights reserved.
   Created: Terry Weissman <terry@netscape.com>, 20-Jun-95.
 */
#ifndef __xfe_mailnews_h_
#define __xfe_mailnews_h_

extern void fe_msg_command(Widget, XtPointer, XtPointer);
extern void fe_undo_cb(Widget, XtPointer, XtPointer);
extern void fe_redo_cb(Widget, XtPointer, XtPointer);
extern Boolean fe_folder_datafunc(Widget, void*, int row,
				  fe_OutlineDesc* data);
extern void fe_folder_clickfunc(Widget, void*, int row, int column,
				char* header, int button, int clicks,
				Boolean shift, Boolean ctrl);
extern void fe_folder_dropfunc(Widget dropw, void* closure, fe_dnd_Event,
			       fe_dnd_Source*, XEvent* event);
extern void fe_folder_iconfunc(Widget, void*, int row);

extern Boolean fe_message_datafunc(Widget, void*, int row,
				   fe_OutlineDesc* data);
extern void fe_message_clickfunc(Widget, void*, int row, int column,
				 char* header, int button, int clicks,
				 Boolean shift, Boolean ctrl);
extern void fe_message_dropfunc(Widget dropw, void* closure, fe_dnd_Event,
			       fe_dnd_Source*, XEvent* event);
extern void fe_message_iconfunc(Widget, void*, int row);

extern void fe_mn_getnewmail(Widget, XtPointer, XtPointer);

extern void fe_mc_dropfunc(Widget dropw, void* closure, fe_dnd_Event type,
		  fe_dnd_Source* source, XEvent* event);
extern char* fe_mn_getmailbox(void);

#endif /* __xfe_mailnews_h_ */
