/* -*- Mode: C; tab-width: 8 -*-
   menu.c --- menu creating utilities
   Copyright © 1995 Netscape Communications Corporation, all rights reserved.
   Created: Terry Weissman <terry@netscape.com>, 14-Jun-95.
 */

#include "mozilla.h"
#include "xfe.h"
#include "menu.h"


static Widget
fe_CreateIntlSubmenu(Widget parent, struct fe_button *buttons, MWContext *context)
{
	int			ac;
	Arg			av[20];
	Boolean			b;
	Widget			button;
	fe_button_closure	*closure;
	Colormap		cmap;
	Cardinal		depth;
	Widget			menu;
	Visual			*v;
	int			w;

	if (!buttons || !buttons[0].name)
	{
		return NULL;
	}

	v = 0;
	cmap = 0;
	depth = 0;
	XtVaGetValues(CONTEXT_DATA(context)->widget, XtNvisual, &v,
		XtNcolormap, &cmap, XtNdepth, &depth, 0);

	ac = 0;
	XtSetArg(av[ac], XmNvisual, v); ac++;
	XtSetArg(av[ac], XmNcolormap, cmap); ac++;
	XtSetArg(av[ac], XmNdepth, depth); ac++;
	menu = XmCreatePulldownMenu(parent, buttons[0].name, av, ac);

	w = 1;
	while (buttons[w].name)
	{
		ac = 0;
		if (!*buttons[w].name)
		{
			button = XmCreateSeparatorGadget(menu, "separator",
				av, ac);
		}
		else
		{
			closure = XP_NEW(fe_button_closure);
			if (!closure)
			{
				return NULL;
			}
			closure->context = context;
			closure->button = &buttons[w];
			(*buttons[w].callback)(NULL, closure, &b);
			XtSetArg(av[ac], XmNset, b); ac++;
			button = XmCreateToggleButtonGadget(menu,
				buttons[w].name, av, ac);
			XtAddCallback(button, XmNvalueChangedCallback,
				buttons[w].callback, (void *) closure);
		}
		XtManageChild(button);
		w++;
	}

	return menu;
}


int
fe_CreateSubmenu(Widget menu, struct fe_button *buttons, MWContext *context,
			XtPointer closure, XtPointer data)
{
	int			ac, ac1;
	Arg			av[20], av1[5];
	Widget			submenu;
	Colormap		cmap;
	Cardinal		depth;
	Visual			*v;
	int			w, step;

	if (!buttons || !buttons[0].name)
	{
		return 1;
	}

	v = 0;
	cmap = 0;
	depth = 0;
	w = 0;
	while (buttons[w].name)
	{
	    Widget menu_button;
	    step = 1;
	    if (!*buttons[w].name) {	/* "" means do a line */
		ac = 0;
		menu_button = XmCreateSeparatorGadget(menu, "separator",
						      av, ac);
	    }
#ifdef __sgi
	    else if ((!strcmp("sgi", buttons[w].name) ||
		      !strcmp("adobe", buttons[w].name))
		     && !fe_VendorAnim) {
		w++;
		continue;
	    }
#endif
	    else {
		char *name = buttons[w].name;
		void *arg;
		ac = 0;
		if (buttons[w].userdata) {
		  XtSetArg(av[ac], XmNuserData, buttons[w].userdata); ac++;
		}		  

		if (buttons[w].type == SELECTABLE ||
		      buttons[w].type == UNSELECTABLE) {
		    if (buttons[w].type == UNSELECTABLE)
			XtSetArg(av[ac], XtNsensitive, False), ac++;
		    menu_button =
			XmCreatePushButtonGadget(menu, name, av, ac);
		    arg = closure;
		    XtAddCallback(menu_button,
				  XmNactivateCallback,
				  buttons[w].callback,
				  arg);
		}
		else if (buttons[w].type == TOGGLE ||
			 buttons[w].type == RADIO) {
		    Boolean *var =
			(Boolean *) ((char *) CONTEXT_DATA(context) +
				     (int) buttons[w].var);
		    arg = closure;
		    if (var)
		      XtSetArg(av[ac], XmNset, !!(*var));
		    else
		      XtSetArg(av[ac], XmNset, False);
		    ac++;
		    if (buttons[w].type == RADIO) {
		      XtSetArg(av[ac], XmNindicatorType, XmONE_OF_MANY);
		      ac++;
		    }		      
		    menu_button =
			XmCreateToggleButtonGadget(menu, name, av, ac);
		    XtAddCallback(menu_button,
				  XmNvalueChangedCallback,
				  buttons[w].callback,
				  arg);
		}
		else if (buttons[w].type == INTL_CASCADE) {
		    submenu = fe_CreateIntlSubmenu(menu,
			(struct fe_button *) buttons[w].var, context);
		    XtSetArg(av[ac], XmNsubMenuId, submenu); ac++;
		    menu_button =
		    	XmCreateCascadeButtonGadget(menu, name, av, ac);
		} else if (buttons[w].type == CASCADE) {
		    arg = closure;
		    if (buttons[w+1].name) {
			/* Create submenu only if there are items */
			XtVaGetValues(CONTEXT_DATA(context)->widget, XtNvisual,
				&v, XtNcolormap, &cmap, XtNdepth, &depth, 0);

			ac1 = 0;
			XtSetArg(av1[ac1], XmNvisual, v); ac1++;
			XtSetArg(av1[ac1], XmNcolormap, cmap); ac1++;
			XtSetArg(av1[ac1], XmNdepth, depth); ac1++;
			submenu = XmCreatePulldownMenu(menu, buttons[w].name,
							av1, ac1);
			step = fe_CreateSubmenu(submenu, &buttons[w+1], context,
						closure, data);
			XtSetArg(av[ac], XmNsubMenuId, submenu); ac++;
		    }
		    step += 1;	/* to gobble the current entry */
		    menu_button =
		    	XmCreateCascadeButtonGadget(menu, name, av, ac);
		    if (buttons[w].callback)
			XtAddCallback(menu_button, XmNcascadingCallback,
		    			buttons[w].callback, arg);
		} else {
		    abort();
		}
	    }
	    if (buttons[w].offset) {
		Widget* foo = (Widget*)
				(((char*) data) + (int) (buttons[w].offset));
		*foo = menu_button;
	    }
	    XtManageChild(menu_button);
	    w+=step;
	}
	return w+1;
}


Widget
fe_PopulateMenubar(Widget parent, struct fe_button* buttons,
		   MWContext* context, void* closure, void* data,
		   XtCallbackProc pulldown_cb)
{
    Widget menubar;
    Widget menu;
    Widget menubar_button;
    Widget kids[20];
    int i;
    Arg av[20];
    int ac;
    Visual *v = 0;
    Colormap cmap = 0;
    Cardinal depth = 0;
    int w = 0;
    int step;
    char *menu_name;

    XtVaGetValues(XtParent(parent), XtNvisual, &v, XtNcolormap, &cmap,
		  XtNdepth, &depth, 0);
    
    ac = 0;
    XtSetArg(av[ac], XmNskipAdjust, True); ac++;
    XtSetArg(av[ac], XmNseparatorOn, False); ac++;
    XtSetArg(av[ac], XmNvisual, v); ac++;
    XtSetArg(av[ac], XmNdepth, depth); ac++;
    XtSetArg(av[ac], XmNcolormap, cmap); ac++;
    menubar = XmCreateMenuBar(parent, "menuBar", av, ac);

    i = 0;			/* Which menu we're on. */
    while (buttons[w].name) {
	menu_name = buttons[w].name;
	ac = 0;
	XtSetArg(av[ac], XmNvisual, v); ac++;
	XtSetArg(av[ac], XmNcolormap, cmap); ac++;
	XtSetArg(av[ac], XmNdepth, depth); ac++;
	menu = XmCreatePulldownMenu(menubar, menu_name, av, ac);
	ac = 0;
	XtSetArg(av[ac], XmNsubMenuId, menu); ac++;

#if 1
	/* Create XmCreateCascadeButton always as we want to call action
	   CleanupMenubar() later and this is available only with the
	   Button and not with the Gadget. */
	menubar_button = XmCreateCascadeButton(menubar, menu_name, av, ac);
#else /* 1 */
#ifdef _HPUX_SOURCE
	menubar_button = XmCreateCascadeButton      (menubar, menu_name, av, ac);
#else /* !HPUX */
	menubar_button = XmCreateCascadeButtonGadget(menubar, menu_name, av, ac);
#endif /* !HPUX */
#endif /* 1 */

#ifdef _HPUX_SOURCE
	/* HP-SUX!
	   In order to break the legs off of the HP color management
	   garbage (which we needed to do in order to not get
	   brown-on-brown widgets) we had to put the application class
	   in all of the color specifications in the resource file.
	   BUT, that's not enough for the menubar, oh no.  HP uses a
	   different pallette for that, and consequently you can't set
	   the color of it using a resource no matter what you do -
	   even if the entire path to the RowColumn is fully
	   qualified.

	   So on HPs, we make the buttons in the menubar be Widgets
	   instead of Gadgets (so that they can have a background
	   color) and then force the background color of the menubar
	   to be the background color of the first menu button on it.  */

	if (i == 0) {
	    Pixel bg = 0, tsc = 0, bsc = 0;
	    XtVaGetValues(menubar_button,
			  XmNbackground, &bg,
			  XmNtopShadowColor, &tsc,
			  XmNbottomShadowColor, &bsc,
			  0);
	    XtVaSetValues(menubar,
			  XmNbackground, bg,
			  XmNtopShadowColor, tsc,
			  XmNbottomShadowColor, bsc,
			  0);
	}
#endif

	if (buttons[w].offset) {
	    Widget* foo = (Widget*)
		(((char*) data) + (int) (buttons[w].offset));
	    *foo = menu;
	}

	XtAddCallback(menubar_button, XmNcascadingCallback,
		      pulldown_cb, (XtPointer) closure);

	/*
	 *    Allow menu by menu callbacks, instead of calling
	 *    back on every menu pulldown (slow).
	 */
	if (buttons[w].callback)
	    XtAddCallback(
			  menubar_button,
			  XmNcascadingCallback,
			  buttons[w].callback,
			  (XtPointer)closure
			  );

	kids[i] = menubar_button;

	w++;
	step = fe_CreateSubmenu(menu, &buttons[w], context, closure, data);
	if (!strcmp("help", menu_name)) {
	    XtVaSetValues(menubar, XmNmenuHelpWidget, menubar_button, 0);
	}
	w+=step;
	i++;
    }
    XtManageChildren(kids, i);
    return menubar;
}
