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
#include <Xm/Form.h>
#include <Xm/PushB.h>
#include <Xm/Label.h>
#include <XmL/Folder.h>

void tabActivate();
void removeTab();

#define bit1_width 19
#define bit1_height 19
static char bit1_bits[] = {
	0xc0, 0x1f, 0x00, 0xf0, 0x7f, 0x00, 0x78, 0xf0, 0x00, 0x9c, 0xcf, 0x01,
	0xee, 0xbf, 0x03, 0x76, 0x70, 0x03, 0x37, 0x67, 0x07, 0x9b, 0xcf, 0x06,
	0xdb, 0xdd, 0x06, 0xdb, 0xd8, 0x06, 0xdb, 0xdd, 0x06, 0x9b, 0xcf, 0x06,
	0x37, 0x67, 0x07, 0x76, 0x70, 0x03, 0xee, 0xbf, 0x03, 0x9c, 0xcf, 0x01,
	0x78, 0xf0, 0x00, 0xf0, 0x7f, 0x00, 0xc0, 0x1f, 0x00};

#define bit2_width 19
#define bit2_height 19
static char bit2_bits[] = {
	0xc0, 0x1f, 0x00, 0xf0, 0x7f, 0x00, 0x78, 0xf0, 0x00, 0x1c, 0xc0, 0x01,
	0x0e, 0x80, 0x03, 0xc6, 0x18, 0x03, 0x27, 0x25, 0x07, 0x23, 0x25, 0x06,
	0xc3, 0x18, 0x06, 0x03, 0x00, 0x06, 0x03, 0x00, 0x06, 0x33, 0x60, 0x06,
	0x67, 0x30, 0x07, 0xc6, 0x1f, 0x03, 0x8e, 0x8f, 0x03, 0x1c, 0xc0, 0x01,
	0x78, 0xf0, 0x00, 0xf0, 0x7f, 0x00, 0xc0, 0x1f, 0x00};

Widget folder, label;

main(argc, argv)
int argc;
char *argv[];
{
	XtAppContext app;
	XmString str;
	int i;
	char buf[10];
	Widget shell, form, removeButton;

	shell =  XtAppInitialize(&app, "Folder2", NULL, 0,
	    &argc, argv, NULL, NULL, 0);

	folder = XtVaCreateManagedWidget("folder",
	    xmlFolderWidgetClass, shell,
	    XmNcornerStyle, XmCORNER_LINE,
	    XmNcornerDimension, 4,
	    XmNspacing, 2,
	    XtVaTypedArg, XmNbackground, XmRString, "#C0C0C0", 8,
	    XtVaTypedArg, XmNforeground, XmRString, "black", 6,
	    XtVaTypedArg, XmNinactiveBackground, XmRString, "#A8A8A8", 8,
	    XtVaTypedArg, XmNinactiveForeground, XmRString, "black", 6,
	    NULL);
	XtAddCallback(folder, XmNactivateCallback, tabActivate, NULL);

	form = XtVaCreateManagedWidget("form",
	    xmFormWidgetClass, folder,
	    XtVaTypedArg, XmNbackground, XmRString, "#C0C0C0", 8,
	    NULL);

	for (i = 0; i < 4; i++)
	{
		sprintf(buf, "Tab %d", i + 1);
		str = XmStringCreateSimple(buf);
		if (i == 1 || i == 3)
			XmLFolderAddBitmapTab(folder, str, (char *)bit1_bits,
			    bit1_width, bit1_height);
		else
			XmLFolderAddBitmapTab(folder, str, (char *)bit2_bits,
			    bit2_width, bit2_height);
		XmStringFree(str);
	}

	label = XtVaCreateManagedWidget("label",
	    xmLabelWidgetClass, form,
	    XmNtopAttachment, XmATTACH_FORM,
	    XmNleftAttachment, XmATTACH_FORM,
	    XmNrightAttachment, XmATTACH_FORM,
	    XmNmarginWidth, 80,
	    XmNmarginHeight, 60,
	    XtVaTypedArg, XmNbackground, XmRString, "#C0C0C0", 8,
	    NULL);

	removeButton = XtVaCreateManagedWidget("Remove Tab",
	    xmPushButtonWidgetClass, form,
	    XmNtopAttachment, XmATTACH_WIDGET,
	    XmNtopWidget, label,
	    XmNleftAttachment, XmATTACH_FORM,
	    NULL);
	XtAddCallback(removeButton, XmNactivateCallback, removeTab, NULL);

	XmLFolderSetActiveTab(folder, 0, True);

	XtRealizeWidget(shell);
	XtAppMainLoop(app);
}

void tabActivate(w, clientData, callData)
Widget w;
XtPointer clientData;
XtPointer callData;
{
	XmLFolderCallbackStruct *cbs;
	XmString str;
	char buf[30];

	cbs = (XmLFolderCallbackStruct *)callData;
	sprintf(buf, "Page Number %d", cbs->pos + 1);
	str = XmStringCreateSimple(buf);
	XtVaSetValues(label,
	    XmNlabelString, str,
	    NULL);
	XmStringFree(str);
}

void removeTab(w, clientData, callData)
Widget w;
XtPointer clientData;
XtPointer callData;
{
	int i;
	WidgetList tabs;

	XtVaGetValues(folder,
	    XmNtabCount, &i,
	    XmNtabWidgetList, &tabs,
	    NULL);
	if (!i)
		return;
	XtDestroyWidget(tabs[i - 1]);
}
