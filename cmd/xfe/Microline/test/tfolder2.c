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
#include <Xm/Label.h>
#include <Xm/Form.h>
#include <XmL/Folder.h>

main(argc, argv)
int argc;
char *argv[];
{
	XtAppContext app;
	Widget shell, form, oForm, folder;
	static char tabLabel[3][20] = {
		"Tab One", "Tab Two", "Tab Three" 		};
	static char formLabel[3][20] = {
		"Page One", "Page Two", "Page Three" 		};
	Widget fol[4];
	XmString str;
	int i, j;

	shell =  XtAppInitialize(&app, "TFolder2", NULL, 0,
	    &argc, argv, NULL, NULL, 0);

	oForm = XtVaCreateManagedWidget("folder",
	    xmFormWidgetClass, shell,
	    XtVaTypedArg, XmNbackground, XmRString, "#C0C0C0", 8,
	    NULL);
	for (j = 0; j < 4; j++)
	{
		fol[j] = XtVaCreateManagedWidget("folder",
		    xmlFolderWidgetClass, oForm,
		    XtVaTypedArg, XmNbackground, XmRString, "#C0C0C0", 8,
		    XtVaTypedArg, XmNforeground, XmRString, "black", 6,
		    XmNcornerStyle, XmCORNER_ARC,
		    XmNtabPlacement, j,
		    NULL);
		if (j == 0)
			XtVaSetValues(fol[j],
			    XmNtabPlacement, XmFOLDER_LEFT,
			    XmNleftAttachment, XmATTACH_POSITION,
			    XmNleftPosition, 5,
			    XmNrightAttachment, XmATTACH_POSITION,
			    XmNrightPosition, 45,
			    XmNtopAttachment, XmATTACH_POSITION,
			    XmNtopPosition, 5,
			    XmNbottomAttachment, XmATTACH_POSITION,
			    XmNbottomPosition, 45,
			    NULL);
		if (j == 1)
			XtVaSetValues(fol[j],
			    XmNtabPlacement, XmFOLDER_TOP,
			    XmNleftAttachment, XmATTACH_POSITION,
			    XmNleftPosition, 55,
			    XmNrightAttachment, XmATTACH_POSITION,
			    XmNrightPosition, 95,
			    XmNtopAttachment, XmATTACH_POSITION,
			    XmNtopPosition, 5,
			    XmNbottomAttachment, XmATTACH_POSITION,
			    XmNbottomPosition, 45,
			    NULL);
		if (j == 2)
			XtVaSetValues(fol[j],
			    XmNtabPlacement, XmFOLDER_RIGHT,
			    XmNleftAttachment, XmATTACH_POSITION,
			    XmNleftPosition, 5,
			    XmNrightAttachment, XmATTACH_POSITION,
			    XmNrightPosition, 45,
			    XmNtopAttachment, XmATTACH_POSITION,
			    XmNtopPosition, 55,
			    XmNbottomAttachment, XmATTACH_POSITION,
			    XmNbottomPosition, 95,
			    NULL);
		if (j == 3)
			XtVaSetValues(fol[j],
			    XmNtabPlacement, XmFOLDER_BOTTOM,
			    XmNleftAttachment, XmATTACH_POSITION,
			    XmNleftPosition, 55,
			    XmNrightAttachment, XmATTACH_POSITION,
			    XmNrightPosition, 95,
			    XmNtopAttachment, XmATTACH_POSITION,
			    XmNtopPosition, 55,
			    XmNbottomAttachment, XmATTACH_POSITION,
			    XmNbottomPosition, 95,
			    NULL);
		for (i = 0; i < 3; i++)
		{
			str = XmStringCreateSimple(tabLabel[i]);
			form = XmLFolderAddTabForm(fol[j], str);
			XmStringFree(str);
			XtVaCreateManagedWidget(formLabel[i],
			    xmLabelWidgetClass, form,
			    XmNtopAttachment, XmATTACH_FORM,
			    XmNbottomAttachment, XmATTACH_FORM,
			    XmNleftAttachment, XmATTACH_FORM,
			    XmNrightAttachment, XmATTACH_FORM,
			    XtVaTypedArg, XmNbackground, XmRString, "#C0C0C0", 8,
			    XtVaTypedArg, XmNforeground, XmRString, "black", 6,
			    NULL);
		}
		XmLFolderSetActiveTab(fol[j], 0, True);

	}

	XtRealizeWidget(shell);
	XtAppMainLoop(app);
}
