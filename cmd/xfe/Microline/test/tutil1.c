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

#include <XmL/XmL.h>
#include <Xm/Xm.h>
#include <Xm/PushB.h>

void act();

Widget shell, pushB;

main(argc, argv)
int argc;
char *argv[];
{
	XtAppContext app;

	shell = XtAppInitialize(&app, "Folder1", NULL, 0,
	    &argc, argv, NULL, NULL, 0);

	pushB = XtVaCreateManagedWidget("Test",
		xmPushButtonWidgetClass, shell,
		NULL);
	XtAddCallback(pushB, XmNactivateCallback, act, NULL);

	XtRealizeWidget(shell);
	XtAppMainLoop(app);
}

int compare(userData, l, r)
void *userData;
void *l, *r;
{
	char *name1, *name2;

	name1 = *((char **)l);
	name2 = *((char **)r);
	return strcmp(name1, name2);
}

void act(w, clientData, callData)
Widget w;
XtPointer clientData, callData;
	{
	XmFontList fontList;
	XRectangle r1, r2;
	static char names[5][10] = { "Mary", "Terry", "Louie", "Carry", "Barry" };
	char *namesp[5];
	short width, height;
	Widget sw;
	int i;

	sw = XmLShellOfWidget(pushB);
	printf ("SHELL NAME %s\n", XtName(sw));

	i = XmLMessageBox(pushB, "Testing", True);
	printf ("%d chosen\n", i);
	i = XmLMessageBox(pushB, "Testing", False);
	printf ("%d chosen\n", i);

	XmLWarning(pushB, "WARNING MESSAGE");

	fontList = XmLFontListCopyDefault(pushB);
	XmLFontListGetDimensions(fontList, &width, &height, True);
	printf ("FONT LIST WIDTH %d HEIGHT %d\n", (int)width, (int)height);
	XmFontListFree(fontList);

	i = XmLDateDaysInMonth(2, 1995);
	printf ("Days in month for Feb 1995 is %d\n", i);

	i = XmLDateWeekDay(2, 1, 1995);
	printf ("Weekday of Feb 1, 1995 is %d\n", i);

	r1.x = 4;
	r1.y = 4;
	r1.width = 10;
	r1.height = 10;
	r2 = r1;
	r2.x = 20;
	i = XmLRectIntersect(&r1, &r2);
	if (i == XmLRectInside)
		printf ("r1 outside of r2\n");
	else if (i == XmLRectOutside)
		printf ("r1 outside of r2\n");
	else
		printf ("r1 partially inside r2\n");

	for (i = 0; i < 5; i++)
		namesp[i] = names[i];
	XmLSort(namesp, 5, sizeof(char *), compare, NULL);
	for (i = 0; i < 5; i++)
		printf ("%s\n", namesp[i]);
	}
