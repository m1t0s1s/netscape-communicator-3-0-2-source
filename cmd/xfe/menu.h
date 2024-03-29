/* -*- Mode: C; tab-width: 8 -*-
   menu.h --- menu creating utilities
   Copyright � 1995 Netscape Communications Corporation, all rights reserved.
   Created: Terry Weissman <terry@netscape.com>, 14-Jun-95.
 */

#ifndef __menu_h
#define __menu_h

/*
 * Structure describing a single button or menu title.
 * fe_PopulateMenubar is passed an array of these.  The first entry in
 * each menu is for the menu title.  If name is "", it means a separator
 * line.  If name is NULL, it means the end of a menu.  If two entries in
 * a row have a name of NULL, it marks the end of the array.
 */

typedef struct fe_button 
{
  char *name;
  XtCallbackProc callback;
  int type;
  void* var;
  void* offset;
  void* userdata;
} fe_button;

typedef struct fe_button_closure
{
  MWContext *context;
  fe_button *button;
} fe_button_closure;

#define UNSELECTABLE 0
#define SELECTABLE   1
#define TOGGLE       2
#define RADIO	     3		/* Just like TOGGLE, but with a radio button
				   look.*/
#define CASCADE      4
#define INTL_CASCADE      5	/* This is temporary. Will go away. */

#define FE_MENU_SEPARATOR          { "",0,UNSELECTABLE,0}
#define FE_MENU_BEGIN(l)           { (l), 0, SELECTABLE, 0 }
#define FE_MENU_END                { 0 }
#define FE_NOT_IMPLEMENTED         0

extern Widget fe_PopulateMenubar(Widget parent, struct fe_button* buttons,
				 MWContext* context, void* closure, void* data,
				 XtCallbackProc pulldown_cb);

/* Creates the buttons as child of menu until if finds a {0}
 * button. Passes back the number of button elements it used.
 * Can do push-buttons, toggle-buttons, radio-buttons & cascades too.
 */
extern int fe_CreateSubmenu(Widget menu, struct fe_button *buttons,
			MWContext *context, XtPointer closure, XtPointer data);

#endif
