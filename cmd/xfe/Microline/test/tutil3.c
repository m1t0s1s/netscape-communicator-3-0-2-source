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

main(argc, argv)
int argc;
char *argv[];
{
	XtAppContext context;
	Widget shell, form, b[4];
	XmString str;
	int i;
	static char *label[4] =
		{
		"DrawnButton Right",
		"DrawnButton Left",
		"DrawnButton Up",
		"DrawnButton Down"
		};
	static unsigned char dirs[4] =
		{
		XmDRAWNB_RIGHT,
		XmDRAWNB_LEFT,
		XmDRAWNB_UP,
		XmDRAWNB_DOWN
		};

	shell = XtAppInitialize(&context, "Test", NULL, 0,
	    (Cardinal *)&argc, (String *)argv, NULL, NULL, 0);
	form = XtVaCreateManagedWidget(
	    "form", xmFormWidgetClass, shell,
	    NULL);
	for (i = 0; i < 4; i++)
		{
		str = XmStringCreateSimple(label[i]);
		b[i] = XtVaCreateManagedWidget(
			"button", xmDrawnButtonWidgetClass, form,
			XmNlabelString, str,
			XmNleftAttachment, XmATTACH_FORM,
			NULL);
		if (!i)
			XtVaSetValues(b[i],
				XmNtopAttachment, XmATTACH_FORM,
				NULL);
		else
			XtVaSetValues(b[i],
				XmNtopAttachment, XmATTACH_WIDGET,
				XmNtopWidget, b[i - 1],
				NULL);
		XmStringFree(str);
		XmLDrawnButtonSetType(b[i], XmDRAWNB_STRING, dirs[i]);
		}
	XtRealizeWidget(shell);
	XtAppMainLoop(context);
}
