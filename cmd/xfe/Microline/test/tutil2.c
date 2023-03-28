/*
(c) Copyright 1994, 1995, Microline Software, Inc.  ALL RIGHTS RESERVED
  
THIS SOFTWARE IS FURNISHED UNDER A LICENSE AND MAY BE COPIED AND USED 
ONLY IN ACCORDANCE WITH THE TERMS OF THAT LICENSE AND WITH THE INCLUSION
OF THE ABOVE COPYRIGHT NOTICE.  THIS SOFTWARE AND DOCUMENTATION, AND ITS 
COPYRIGHTS ARE OWNED BY MICROLINE SOFTWARE AND ARE PROTECTED BY UNITED
STATES COPYRIGHT LAWS AND INTERNATIONAL TREATY PROVISIONS.
 
THE INFORMATION IN THIS SOFTWARE IS SUBJECT TO CHANGE WITHOUT NOTICE
AND SHOULD NOT BE CONSTRUED AS A COMMITMENT BY MICROLINE SOFTWARE.

THIS SOFTWARE AND REFERENCE MATERIALS ARE PROVIDED "AS IS" WITHOUT
WARRANTY AS TO THEIR PERFORMANCE, MERCHANTABILITY, FITNESS FOR ANY 
PARTICULAR PURPOSE, OR AGAINST INFRINGEMENT.  MICROLINE SOFTWARE
ASSUMES NO RESPONSIBILITY FOR THE USE OR INABILITY TO USE THIS 
SOFTWARE.

MICROLINE SOFTWARE SHALL NOT BE LIABLE FOR INDIRECT, SPECIAL OR
CONSEQUENTIAL DAMAGES RESULTING FROM THE USE OF THIS PRODUCT. SOME 
STATES DO NOT ALLOW THE EXCLUSION OR LIMITATION OF INCIDENTAL OR
CONSEQUENTIAL DAMAGES, SO THE ABOVE LIMITATIONS MIGHT NOT APPLY TO
YOU.

MICROLINE SOFTWARE SHALL HAVE NO LIABILITY OR RESPONSIBILITY FOR SOFTWARE
ALTERED, MODIFIED, OR CONVERTED BY YOU OR A THIRD PARTY, DAMAGES
RESULTING FROM ACCIDENT, ABUSE OR MISAPPLICATION, OR FOR PROBLEMS DUE
TO THE MALFUNCTION OF YOUR EQUIPMENT OR SOFTWARE NOT SUPPLIED BY
MICROLINE SOFTWARE.
  
U.S. GOVERNMENT RESTRICTED RIGHTS
This Software and documentation are provided with RESTRICTED RIGHTS.
Use, duplication or disclosure by the Government is subject to
restrictions as set forth in subparagraph (c)(1) of the Rights in
Technical Data and Computer Software Clause at DFARS 252.227-7013 or
subparagraphs (c)(1)(ii) and (2) of Commercial Computer Software -
Restricted Rights at 48 CFR 52.227-19, as applicable, supplier is
Microline Software, 41 Sutter St Suite 1374, San Francisco, CA 94104.
*/

#include <Xm/Xm.h>
#include <XmL/XmL.h>
#include <Xm/Form.h>
#include <Xm/DrawnB.h>
#include <stdio.h>

Widget form, drawnb;

void drawnbExpose();

main(argc, argv)
int argc;
char *argv[];
{
	XtAppContext context;
	Widget shell;

	shell = XtAppInitialize(&context, "Test", NULL, 0,
	    (Cardinal *)&argc, (String *)argv, NULL, NULL, 0);
	XtVaSetValues(shell,
	    XmNwidth, 200,
	    XmNheight, 300,
	    NULL);
	form = XtVaCreateManagedWidget(
	    "form", xmFormWidgetClass, shell,
	    NULL);
	drawnb = XtVaCreateManagedWidget(
	    "drawnb", xmDrawnButtonWidgetClass, form,
	    XmNleftAttachment, XmATTACH_FORM,
	    XmNrightAttachment, XmATTACH_FORM,
	    XmNtopAttachment, XmATTACH_FORM,
	    XmNbottomAttachment, XmATTACH_FORM,
	    NULL);
	XtAddCallback(drawnb, XmNexposeCallback, drawnbExpose, NULL);

	XtRealizeWidget(shell);
	XtAppMainLoop(context);
}

void drawnbExpose(w, clientData, callData)
Widget w;
XtPointer clientData, callData;
	{
	Display *dpy;
	Window win;
	XmString str;
	XmFontList fontlist;
	XColor scrncol, excol;
	XFontStruct *fontStruct;
	int width, height;
	GC gc;
	static int count;
	int i;
	static char *colors[] =
		{
		"blue",
		"green",
		"yellow",
		"white",
		"red",
		"grey"
		};

	if (!XtIsRealized(w))
		return;
	dpy = XtDisplay(w);
	win = XtWindow(w);
	XtVaGetValues(drawnb,
	    XmNfontList, &fontlist,
	    NULL);
	gc   = XCreateGC(dpy, win, 0, NULL);
	fontStruct = XLoadQueryFont(dpy, "fixed");
	if (!fontStruct)
		{
		fprintf(stderr, "cant load fixed font\n");
		return;
		}
	XSetFont(dpy, gc, fontStruct->fid);
	XAllocNamedColor(dpy, DefaultColormapOfScreen(XtScreen(drawnb)),
	    "blue", &scrncol, &excol);
	XSetForeground(dpy, gc, scrncol.pixel);

	str = XmStringCreateSimple("WWWW WWWWWWW WWWWWWWWW");
	width = XmStringWidth(fontlist, str);
	height = XmStringHeight(fontlist, str);
	XmStringFree(str);
	width += 20;
	height += 5;

	/* Text Right */
	str = XmStringCreateSimple("Right Aligned Beginning");
	XmLStringDrawDirection(dpy, win, fontlist, str, gc,
	    10, 5, width, XmALIGNMENT_BEGINNING,
	    XmSTRING_DIRECTION_L_TO_R, XmSTRING_RIGHT);
	XmStringFree(str);
	str = XmStringCreateSimple("Right Aligned Center");
	XmLStringDrawDirection(dpy, win, fontlist, str, gc,
	    10, height + 5, width, XmALIGNMENT_CENTER,
	    XmSTRING_DIRECTION_L_TO_R, XmSTRING_RIGHT);
	XmStringFree(str);
	str = XmStringCreateSimple("Right Aligned End");
	XmLStringDrawDirection(dpy, win, fontlist, str, gc,
	    10, height * 2 + 5, width, XmALIGNMENT_END,
	    XmSTRING_DIRECTION_L_TO_R, XmSTRING_RIGHT);
	XmStringFree(str);

	/* Draw offset baseline XmStringDraw comparison text */
	XAllocNamedColor(dpy, DefaultColormapOfScreen(XtScreen(drawnb)),
	    "white", &scrncol, &excol);
	XSetForeground(dpy, gc, scrncol.pixel);
	str = XmStringCreateSimple("Right Aligned Beginning");
	XmStringDraw(dpy, win, fontlist, str, gc,
	    11, 6, width, XmALIGNMENT_BEGINNING,
	    XmSTRING_DIRECTION_L_TO_R, 0);
	XmStringFree(str);
	str = XmStringCreateSimple("Right Aligned Center");
	XmStringDraw(dpy, win, fontlist, str, gc,
	    11, height + 6, width, XmALIGNMENT_CENTER,
	    XmSTRING_DIRECTION_L_TO_R, 0);
	XmStringFree(str);
	str = XmStringCreateSimple("Right Aligned End");
	XmStringDraw(dpy, win, fontlist, str, gc,
	    11, height * 2 + 6, width, XmALIGNMENT_END,
	    XmSTRING_DIRECTION_L_TO_R, 0);
	XmStringFree(str);

	/* Text Left */
	str = XmStringCreateSimple("Left Aligned Beginning");
	XmLStringDrawDirection(dpy, win, fontlist, str, gc,
	    10, height * 3 + 5, width, XmALIGNMENT_BEGINNING,
	    XmSTRING_DIRECTION_L_TO_R, XmSTRING_LEFT);
	XmStringFree(str);
	str = XmStringCreateSimple("Left Aligned Center");
	XmLStringDrawDirection(dpy, win, fontlist, str, gc,
	    10, height * 4 + 5, width, XmALIGNMENT_CENTER,
	    XmSTRING_DIRECTION_L_TO_R, XmSTRING_LEFT);
	XmStringFree(str);
	str = XmStringCreateSimple("Left Aligned End");
	XmLStringDrawDirection(dpy, win, fontlist, str, gc,
	    10, height * 5 + 5, width, XmALIGNMENT_END,
	    XmSTRING_DIRECTION_L_TO_R, XmSTRING_LEFT);
	XmStringFree(str);

	/* Text Up */
	str = XmStringCreateSimple("Up Aligned Beginning");
	XmLStringDrawDirection(dpy, win, fontlist, str, gc,
	    10, height * 6 + 5, width, XmALIGNMENT_BEGINNING,
	    XmSTRING_DIRECTION_L_TO_R, XmSTRING_UP);
	XmStringFree(str);
	str = XmStringCreateSimple("Up Aligned Center");
	XmLStringDrawDirection(dpy, win, fontlist, str, gc,
	    10 + height, height * 6 + 5, width, XmALIGNMENT_CENTER,
	    XmSTRING_DIRECTION_L_TO_R, XmSTRING_UP);
	XmStringFree(str);
	str = XmStringCreateSimple("Up Aligned End");
	XmLStringDrawDirection(dpy, win, fontlist, str, gc,
	    10 + height * 2, height * 6 + 5, width, XmALIGNMENT_END,
	    XmSTRING_DIRECTION_L_TO_R, XmSTRING_UP);
	XmStringFree(str);

	/* Text Down */
	str = XmStringCreateSimple("Down Aligned Beginning");
	XmLStringDrawDirection(dpy, win, fontlist, str, gc,
	    10 + height * 3, height * 6 + 5, width, XmALIGNMENT_BEGINNING,
	    XmSTRING_DIRECTION_L_TO_R, XmSTRING_DOWN);
	XmStringFree(str);
	str = XmStringCreateSimple("Down Aligned Center");
	XmLStringDrawDirection(dpy, win, fontlist, str, gc,
	    10 + height * 4, height * 6 + 5, width, XmALIGNMENT_CENTER,
	    XmSTRING_DIRECTION_L_TO_R, XmSTRING_DOWN);
	XmStringFree(str);
	str = XmStringCreateSimple("Down Aligned End");
	XmLStringDrawDirection(dpy, win, fontlist, str, gc,
	    10 + height * 5, height * 6 + 5, width, XmALIGNMENT_END,
	    XmSTRING_DIRECTION_L_TO_R, XmSTRING_DOWN);
	XmStringFree(str);

	count++;
	if (count > 2)
		for (i = 0; i < 100; i++)
			{
			XAllocNamedColor(dpy, DefaultColormapOfScreen(XtScreen(drawnb)),
	    		colors[rand() % 6], &scrncol, &excol);
			XSetForeground(dpy, gc, scrncol.pixel);
			str = XmStringCreateSimple("Random Text");
			XmLStringDrawDirection(dpy, win, fontlist, str, gc,
	    		rand() % 400 - 10, rand() % 400 - 10, rand() % 300,
				i % 3, XmSTRING_DIRECTION_L_TO_R, i % 4);
			XmStringFree(str);
			}

	XFreeGC(dpy, gc);
	XFreeFont(dpy, fontStruct);
	}
