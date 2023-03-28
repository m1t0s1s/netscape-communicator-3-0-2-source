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
#include <Mrm/MrmPublic.h>
#include <XmL/Folder.h>
#include <XmL/Grid.h>
#include <XmL/Progress.h>
#include <stdio.h>

main(argc, argv)
int    argc;
String argv[];
{
	Display  	*display;
	XtAppContext app_context;
	Widget toplevel, grid, folder;
	MrmHierarchy hier;
	MrmCode clas;
	static char *files[] = {
		"uil1.uid"		};

	MrmInitialize ();
	XtToolkitInitialize();
	app_context = XtCreateApplicationContext();
	display = XtOpenDisplay(app_context, NULL, argv[0], "Uil1",
	    NULL, 0, &argc, argv);
	if (display == NULL) {
		fprintf(stderr, "%s:  Can't open display\n", argv[0]);
		exit(1);
	}

	toplevel = XtVaAppCreateShell(argv[0], NULL,
	    applicationShellWidgetClass, display,
	    XmNwidth, 400,
	    XmNheight, 300,
	    NULL);

	if (MrmOpenHierarchy (1, files, NULL, &hier) != MrmSUCCESS)
		printf ("can't open hierarchy\n");

	MrmRegisterClass(0, NULL, "XmLCreateFolder",
	    XmLCreateFolder, xmlFolderWidgetClass);
	MrmRegisterClass(0, NULL, "XmLCreateGrid",
	    XmLCreateGrid, xmlGridWidgetClass);
	MrmRegisterClass(0, NULL, "XmLCreateProgress",
	    XmLCreateProgress, xmlProgressWidgetClass);

	if (MrmFetchWidget(hier, "folder", toplevel, &folder, &clas) != MrmSUCCESS)
		printf("can't fetch folder\n");

	grid = XtNameToWidget(folder, "*grid");

	XmLGridAddColumns(grid, XmCONTENT, -1, 20);
	XtVaSetValues(grid,
	    XmNcolumn, 0,
	    XmNcolumnWidth, 20,
	    NULL);
	XmLGridAddRows(grid, XmCONTENT, -1, 50);

	XtRealizeWidget(toplevel);
	XtManageChild(folder);

	XmLFolderSetActiveTab(folder, 0, True);

	XtAppMainLoop(app_context);
	return (0);
}
