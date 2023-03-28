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
#include <XmL/Folder.h>
#include <XmL/Grid.h>
#include <XmL/Progress.h>

#include <Xm/Xm.h>
#include <Xm/DialogS.h>
#include <Xm/DrawnB.h>
#include <Xm/Form.h>
#include <Xm/Frame.h>
#include <Xm/Label.h>
#include <Xm/PushB.h>
#include <Xm/RowColumn.h>
#include <Xm/Separator.h>
#include <Xm/Text.h>
#include <Xm/ToggleB.h>
#include <Xm/Protocols.h>

#include <X11/IntrinsicP.h>
#include <X11/cursorfont.h>

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

void CreateWidgetEdit();
Widget CreateOptionForm();
int ToggleButtonSet();
Widget CreateDescText();

void AddIntroTabForm();
void AddFolderTabForm();
void AddGridTabForm();
void AddProgressTabForm();

#define SMALLFONT "-*-helvetica-bold-r-*--12-*-*-*-*-*-*-1"
#define BIGFONT "-*-helvetica-bold-r-normal--14-*-*-*-*-*-*-1"

/*
   Main Window
*/

static int xErrorHandler();
static void exitCB();
static void editCB();
static void folderCB();

main(argc, argv)
int argc;
char *argv[];
{
	XtAppContext context;
	Widget w, shell, form, folder, footer;
	int i;
	XrmDatabase db;
	static String resources[] =
	{
		"*background: #D0D0D0",
		"*foreground: black",
		"*XmText.background: white",
		"*XmText.marginHeight: 2",
		"*XmText.fontList: -*-helvetica-bold-r-*--12-*-*-*-*-*-*-1",
		"*XmToggleButton.selectColor: blue",
		"*fontList: -*-helvetica-bold-r-*--12-*-*-*-*-*-*-1",
		"*folder.fontList: -*-helvetica-bold-o-*--12-*-*-*-*-*-*-1",
		"*XmPushButton.topShadowColor: white",
		"*XmPushButton.bottomShadowColor: #404040",
		"*XmPushButton.marginWidth: 10",
		"*XmPushButton.marginHeight: 4",
		"*XmSeparator.topShadowColor: white",
		"*XmSeparator.bottomShadowColor: black",
		"*XmPushButton.marginHeight: 4",
		"*descText.fontList: -*-times-medium-r-*--14-*-*-*-*-*-*-1",
		"*titleLabel.fontList: -*-helvetica-bold-o-*--24-*-*-*-*-*-*-1",
		"*titleLabel.foreground: #000060",
		"*WidgetEdit*nameLabel.background: white",
		"*WidgetEdit*classLabel.background: white",
		"*WidgetEdit*name.foreground: #000060",
		"*WidgetEdit*class.foreground: #000060",
		"*optionForm*background: white",
		"*XmLFolder.topShadowColor: white",
		"*XmLFolder.bottomShadowColor: #404040",
		"*XmLFolder.defaultabFontList: 10x20",
		"*XmLFolder.XmForm.background: #D0D0D0",
		"*XmLGrid.topShadowColor: #B0B0B0",
		"*XmLGrid.bottomShadowColor: #202020",
		"*XmLProgress.topShadowColor: #B0B0B0",
		"*XmLProgress.bottomShadowColor: #404040",
		"*footer*XmPushButton.highlightColor: white",
		"*popupForm*background: white",
		NULL
	};

	shell =  XtVaAppInitialize(&context, "Test", NULL, 0,
#ifdef X11R4
	    (Cardinal *)&argc, argv, resources, NULL);
#else
	(int *)&argc, argv, resources, NULL);
#endif

	/* Force override of the falback resources.  Normally, we would
	   have an app-default file or similar but since this is a demo,
	   we don't want the user to have to set anything up to run */
	db = XtDatabase(XtDisplay(shell));
	i = 0;
	while (resources[i])
	{
		XrmPutLineResource(&db, resources[i]);
		i++;
	}

	XtVaSetValues(
	    shell,
	    XmNtitle, "Microline Widget Library",
	    XmNheight, 400,
	    NULL);

	form = XtVaCreateWidget("form",
	    xmFormWidgetClass, shell,
		XmNmarginWidth, 5,
		XmNmarginHeight, 5,
	    NULL);

	/* Create the footer form holding the bottom row of buttons */
	footer = XtVaCreateManagedWidget("footer",
	    xmFormWidgetClass, form,
	    XmNleftAttachment, XmATTACH_FORM,
	    XmNrightAttachment, XmATTACH_FORM,
	    XmNbottomAttachment, XmATTACH_FORM,
	    NULL);
	XtVaCreateManagedWidget("Copyright (c) 1995 Microline Software",
	    xmLabelWidgetClass, footer,
	    XmNleftAttachment, XmATTACH_FORM,
	    XmNbottomAttachment, XmATTACH_FORM,
	    XmNbottomOffset, 3,
	    XtVaTypedArg, XmNfontList,
	    XmRString, "fixed", 6,
	    NULL);
	w = XtVaCreateManagedWidget("Exit",
	    xmPushButtonWidgetClass, footer,
	    XmNrightAttachment, XmATTACH_FORM,
	    XmNbottomAttachment, XmATTACH_FORM,
	    XmNwidth, 80,
	    NULL);
	XtAddCallback(w, XmNactivateCallback, exitCB, (XtPointer)shell);
	w = XtVaCreateManagedWidget("Editor",
	    xmPushButtonWidgetClass, footer,
	    XmNrightAttachment, XmATTACH_WIDGET,
	    XmNrightWidget, w,
	    XmNbottomAttachment, XmATTACH_FORM,
	    XmNwidth, 80,
	    NULL);
	XtAddCallback(w, XmNactivateCallback, editCB, NULL);
	XtManageChild(footer);

	CreateWidgetEdit(form);

	/* Create the folder and add the tabs and forms */
	folder = XtVaCreateManagedWidget("folder",
	    xmlFolderWidgetClass, form,
	    XmNshadowThickness, 2,
	    XmNtopAttachment, XmATTACH_FORM,
	    XmNleftAttachment, XmATTACH_FORM,
	    XmNrightAttachment, XmATTACH_FORM,
	    XmNbottomAttachment, XmATTACH_WIDGET,
	    XmNbottomWidget, footer,
		XmNbottomOffset, 3,
		XmNspacing, 1,
	    NULL);
	AddIntroTabForm(folder);
	AddFolderTabForm(folder);
	AddGridTabForm(folder);
	AddProgressTabForm(folder);
	XmLFolderSetActiveTab(folder, 0, False);
	XtAddCallback(folder, XmNactivateCallback, folderCB, NULL);

	XtManageChild(form);

	XSetErrorHandler(xErrorHandler);
	XtRealizeWidget(shell);
	XtAppMainLoop(context);
}

static int xErrorHandler(dpy, event)
Display *dpy;
XErrorEvent *event;
{
	char buf[512];

	/* Motif 1.1 can generate a BadDrawable error when changing
	   fonts/attributes on the text widget in a grid */
	XGetErrorText(dpy, event->error_code, buf, 512);
#ifdef NOTDEMO
	fprintf (stderr, "X Error: %s\n", buf);
#endif
	return 0;
}

static void exitCB(w, clientData, callData)
Widget w;
XtPointer clientData;
XtPointer callData;
{
	Widget shell;

	shell = (Widget)clientData;
	exit(0);
}

/*
   Widget Editor Functions
*/

static void _getResourceAsString();
static void _editCB();
static void _editCommandCB();
static void _editChooseCB();
static void _editShowCB();
static void _editCloseCB();
static void _wmCloseCB();

Widget _editDialog, _editClassL, _editNameL, _editGrid, _editCommandT;
Widget _editWidget;
Boolean _editShowInherited, _editDialogUp;
int _editRow, _editCol;

void CreateWidgetEdit(parent)
Widget parent;
{
	Widget form, w;
	Widget chooseB, closeB;
	Atom WM_DELETE_WINDOW;

	_editWidget = 0;
	_editShowInherited = 0;
	_editDialogUp = False;

	_editDialog = XtVaAppCreateShell("WidgetEdit", "WidgetEdit",
	    topLevelShellWidgetClass, XtDisplay(parent),
	    XtNtitle, "Widget Editor",
		XmNdeleteResponse, XmDO_NOTHING,
	    NULL);

	WM_DELETE_WINDOW = XmInternAtom(XtDisplay(parent),
		"WM_DELETE_WINDOW", False);
	XmAddWMProtocolCallback(_editDialog, WM_DELETE_WINDOW,
		_wmCloseCB, (XtPointer)_editDialog);

	form = XtVaCreateWidget("form",
	    xmFormWidgetClass, _editDialog,
	    XmNautoUnmanage, False,
	    XmNhorizontalSpacing, 4,
	    XmNverticalSpacing, 4,
	    NULL);

	/* Top Row */
	w = XtVaCreateManagedWidget("Name:",
	    xmLabelWidgetClass, form,
	    XmNleftAttachment, XmATTACH_FORM,
	    XmNtopAttachment, XmATTACH_FORM,
	    XmNtopOffset, 6,
	    XmNmarginWidth, 0,
	    NULL);
	_editNameL = XtVaCreateManagedWidget("name",
	    xmLabelWidgetClass, form,
	    XmNleftAttachment, XmATTACH_WIDGET,
	    XmNleftWidget, w,
	    XmNtopAttachment, XmATTACH_FORM,
	    XmNtopOffset, 6,
	    XmNmarginWidth, 0,
	    NULL);

	w = XtVaCreateManagedWidget("Class:",
	    xmLabelWidgetClass, form,
	    XmNleftAttachment, XmATTACH_WIDGET,
	    XmNleftWidget, _editNameL,
	    XmNleftOffset, 10,
	    XmNtopAttachment, XmATTACH_FORM,
	    XmNtopOffset, 6,
	    XmNmarginWidth, 0,
	    NULL);
	_editClassL = XtVaCreateManagedWidget("class",
	    xmLabelWidgetClass, form,
	    XmNleftAttachment, XmATTACH_WIDGET,
	    XmNleftWidget, w,
	    XmNtopAttachment, XmATTACH_FORM,
	    XmNtopOffset, 6,
	    XmNmarginWidth, 0,
	    NULL);

	w = XtVaCreateManagedWidget("Show All",
	    xmToggleButtonWidgetClass, form,
	    XmNtopAttachment, XmATTACH_FORM,
	    XmNrightAttachment, XmATTACH_FORM,
	    XmNspacing, 2,
	    NULL);
	XtAddCallback(w, XmNvalueChangedCallback, _editShowCB, 0);

	_editGrid = XtVaCreateManagedWidget("Grid",
	    xmlGridWidgetClass, form,
	    XmNhorizontalSizePolicy, XmVARIABLE,
	    XmNheight, 150,
	    XmNshadowThickness, 1,
	    XmNselectionPolicy, XmSELECT_NONE,
	    XmNallowColumnResize, True,
	    XmNtopAttachment, XmATTACH_WIDGET,
	    XmNtopWidget, w,
	    XmNleftAttachment, XmATTACH_FORM,
	    XmNrightAttachment, XmATTACH_FORM,
	    NULL);

	XtVaSetValues(_editGrid, XmNlayoutFrozen, True, NULL);
	XmLGridAddColumns(_editGrid, XmCONTENT, -1, 4);
	XtVaSetValues(_editGrid,
	    XmNcellDefaults, True,
	    XtVaTypedArg, XmNcellBackground,
	    XmRString, "Yellow", 7,
	    XmNcellType, XmLABEL_CELL,
	    NULL);
	XmLGridAddRows(_editGrid, XmHEADING, -1, 1);
	XmLGridSetStrings(_editGrid, "Class|Type|Resource|Value");
	XtVaSetValues(_editGrid, XmNcolumn, 0, XmNcolumnWidth, 12, NULL);
	XtVaSetValues(_editGrid, XmNcolumn, 1, XmNcolumnWidth, 18, NULL);
	XtVaSetValues(_editGrid, XmNcolumn, 2, XmNcolumnWidth, 22, NULL);
	XtVaSetValues(_editGrid, XmNcolumn, 3, XmNcolumnWidth, 15, NULL);
	XtVaSetValues(_editGrid, XmNlayoutFrozen, False, NULL);
	XtAddCallback(_editGrid, XmNeditCallback, _editCB, 0);

	w = XtVaCreateManagedWidget("Command",
	    xmLabelWidgetClass, form,
	    XmNbottomAttachment, XmATTACH_FORM,
	    XmNbottomOffset, 5,
	    XmNleftAttachment, XmATTACH_FORM,
	    NULL);
	_editCommandT = XtVaCreateManagedWidget("editCommandT",
	    xmTextWidgetClass, form,
	    XmNbottomAttachment, XmATTACH_FORM,
	    XmNleftAttachment, XmATTACH_WIDGET,
	    XmNleftWidget, w,
	    XmNcolumns, 30,
	    XmNmaxLength, 30,
	    NULL);
	XtAddCallback(_editCommandT, XmNactivateCallback, _editCommandCB, 0);

	/* Bottom Row */
	chooseB = XtVaCreateManagedWidget("Choose",
	    xmPushButtonWidgetClass, form,
	    XmNbottomAttachment, XmATTACH_FORM,
	    NULL);
	XtAddCallback(chooseB, XmNactivateCallback, _editChooseCB, 0);
	closeB = XtVaCreateManagedWidget("Close",
	    xmPushButtonWidgetClass, form,
	    XmNrightAttachment, XmATTACH_FORM,
	    XmNbottomAttachment, XmATTACH_FORM,
	    NULL);
	XtAddCallback(closeB, XmNactivateCallback, _editCloseCB, 0);
	XtVaSetValues(chooseB,
	    XmNrightAttachment, XmATTACH_WIDGET,
	    XmNrightWidget, closeB,
	    NULL);
	XtVaSetValues(_editGrid,
	    XmNbottomAttachment, XmATTACH_WIDGET,
	    XmNbottomWidget, closeB,
	    NULL);

	XtManageChild(form);
}

void SetEditWidget(w)
Widget w;
{
	WidgetClass wc, superWc;
	XtResourceList resList;
	char className[50], *resName, *resType;
	Cardinal resCount;
	int i, j, n, skip, done, superSize;
	char resVal[50], line[100];
	XmString str;
	char *s;
	static char *skipRes[] =
	{
		"row",
		"rowRangeStart",
		"rowRangeEnd",
		"rowPtr",
		"column",
		"columnRangeStart",
		"columnRangeEnd",
		"columnPtr",
		0
	};
	static char *skipType[] =
	{
		"Callback",
		0
	};

	_editWidget = w;
	if (_editWidget)
		s = XtName(_editWidget);
	else
		s = "none";
	if (s)
	{
		str = XmStringCreateSimple(s);
		XtVaSetValues(_editNameL, XmNlabelString, str, NULL);
		XmStringFree(str);
	}
	if (_editWidget)
	{
		wc = XtClass(_editWidget);
		s = wc->core_class.class_name;
	}
	else
		s = "none";
	if (s)
	{
		str = XmStringCreateSimple(s);
		XtVaSetValues(_editClassL, XmNlabelString, str, NULL);
		XmStringFree(str);
	}
	if (!_editWidget)
	{
		XmLGridDeleteAllRows(_editGrid, XmCONTENT);
		return;
	}
	n = 0;
	done = 0;
	XtVaSetValues(_editGrid, XmNlayoutFrozen, True, NULL);
	XmLGridDeleteAllRows(_editGrid, XmCONTENT);
	XtVaSetValues(_editGrid,
	    XmNcellDefaults, True,
	    XmNcellAlignment, XmALIGNMENT_LEFT,
	    XtVaTypedArg, XmNcellBackground,
	    XmRString, "white", 6,
	    NULL);
	while (!done)
	{
		if (!wc)
		{
			wc = XtClass(XtParent(_editWidget));
			XtGetConstraintResourceList(wc, &resList, &resCount);
			sprintf(className, "%s [C]", wc->core_class.class_name);
			superSize = 0;
			done = 1;
		}
		else
		{
			XtGetResourceList(wc, &resList, &resCount);
			sprintf(className, wc->core_class.class_name);
			superWc = wc->core_class.superclass;
			if (superWc)
				superSize = superWc->core_class.widget_size;
		}
		for (i = 0; i < resCount; i++)
		{
			resName = resList[i].resource_name;
			resType = resList[i].resource_type;
			if (resList[i].resource_offset < superSize)
				continue;
			skip = 0;
			for (j = 0; skipRes[j]; j++)
				if (!strcmp(skipRes[j], resName))
					skip = 1;
			for (j = 0; skipType[j]; j++)
				if (!strcmp(skipType[j], resType))
					skip = 1;
			if (skip)
				continue;
			_getResourceAsString(resName, resType, resVal);
			sprintf(line, "%s|%s|%s|%s", className, resType, resName, resVal);
			XmLGridAddRows(_editGrid, XmCONTENT, -1, 1);
			XmLGridSetStringsPos(_editGrid, XmCONTENT, n, XmCONTENT, 0, line);
			n++;
		}
		wc = wc->core_class.superclass;
		XtFree((char *)resList);
		if (_editShowInherited == False)
			done = 1;
	}
	XtVaSetValues(_editGrid,
	    XmNcolumn, 3,
	    XmNcellType, XmTEXT_CELL,
	    NULL);
	XtVaSetValues(_editGrid, XmNlayoutFrozen, False, NULL);
}

static void _getResourceAsString(res, type, str)
char *res;
char *type;
char *str;
{
	Dimension dimension;
	int i;

	if (!strcmp(type, XmRInt))
	{
		XtVaGetValues(_editWidget, res, &i, NULL);
		sprintf(str, "%d", i);
	}
	if (!strcmp(type, XmRDimension))
	{
		XtVaGetValues(_editWidget, res, &dimension, NULL);
		sprintf(str, "%d", (int)dimension);
	}
	else
		sprintf(str, "");
}

static void _editCB(w, clientData, callData)
Widget w;
XtPointer clientData;
XtPointer callData;
{
	XmLGridCallbackStruct *cbs;
	int resRow, rowType, row, rowEnd, colType, col, colEnd;
	XmLCGridRow *rowp;
	XmLCGridColumn *colp;
	XmString resStr, valStr;
	char *s, *resS, *valS;
	static char *emptyValS = "";

	cbs = (XmLGridCallbackStruct *)callData;
	if (cbs->reason != XmCR_EDIT_COMPLETE)
		return;
	resRow = cbs->row;

	/* get resource and value */
	rowp = XmLGridGetRow(_editGrid, XmCONTENT, resRow);
	colp = XmLGridGetColumn(_editGrid, XmCONTENT, 2);
	XtVaGetValues(_editGrid,
	    XmNrowPtr, rowp,
	    XmNcolumnPtr, colp,
	    XmNcellString, &resStr,
	    NULL);
	if (!resStr)
		resS = 0;
	else
		XmStringGetLtoR(resStr, XmSTRING_DEFAULT_CHARSET, &resS);
	colp = XmLGridGetColumn(_editGrid, XmCONTENT, 3);
	XtVaGetValues(_editGrid,
	    XmNrowPtr, rowp,
	    XmNcolumnPtr, colp,
	    XmNcellString, &valStr,
	    NULL);
	if (!valStr)
		valS = emptyValS;
	else
		XmStringGetLtoR(valStr, XmSTRING_DEFAULT_CHARSET, &valS);

	if (XmLIsGrid(_editWidget))
		XtVaSetValues(_editWidget,
		    XmNrow, _editRow,
		    XmNcolumn, _editCol,
		    XtVaTypedArg, resS,
		    XmRString, valS, strlen(valS) + 1,
		    NULL);
	else
		XtVaSetValues(_editWidget,
		    XtVaTypedArg, resS,
		    XmRString, valS, strlen(valS) + 1,
		    NULL);

	if (resS)
		XtFree(resS);
	if (resStr)
		XmStringFree(resStr);
	if (valS != emptyValS)
		XtFree(valS);
	if (valStr)
		XmStringFree(valStr);
}

static void _editCommandCB(w, clientData, callData)
Widget w;
XtPointer clientData;
XtPointer callData;
{
	char op[40], *cmd;
	int *rowPos, *colPos;
	int i, l1, l2, l3, l4, n, count;

	cmd = XmTextGetString(_editCommandT);
	if (_editWidget && XmLIsGrid(_editWidget))
	{
		n = sscanf(cmd, "%s %d %d %d %d", op, &l1, &l2, &l3, &l4);
		if (!strcmp(op, "addHRows") && n == 3)
			XmLGridAddRows(_editWidget, XmHEADING, l1, l2);
		else if (!strcmp(op, "addFRows") && n == 3)
			XmLGridAddRows(_editWidget, XmFOOTER, l1, l2);
		else if (!strcmp(op, "addRows") && n == 3)
			XmLGridAddRows(_editWidget, XmCONTENT, l1, l2);
		else if (!strcmp(op, "addHColumns") && n == 3)
			XmLGridAddColumns(_editWidget, XmHEADING, l1, l2);
		else if (!strcmp(op, "addFColumns") && n == 3)
			XmLGridAddColumns(_editWidget, XmFOOTER, l1, l2);
		else if (!strcmp(op, "addColumns") && n == 3)
			XmLGridAddColumns(_editWidget, XmCONTENT, l1, l2);
		else if (!strcmp(op, "deleteHRows") && n == 3)
			XmLGridDeleteRows(_editWidget, XmHEADING, l1, l2);
		else if (!strcmp(op, "deleteFRows") && n == 3)
			XmLGridDeleteRows(_editWidget, XmFOOTER, l1, l2);
		else if (!strcmp(op, "deleteRows") && n == 3)
			XmLGridDeleteRows(_editWidget, XmCONTENT, l1, l2);
		else if (!strcmp(op, "deleteHColumns") && n == 3)
			XmLGridDeleteColumns(_editWidget, XmHEADING, l1, l2);
		else if (!strcmp(op, "deleteFColumns") && n == 3)
			XmLGridDeleteColumns(_editWidget, XmFOOTER, l1, l2);
		else if (!strcmp(op, "deleteColumns") && n == 3)
			XmLGridDeleteColumns(_editWidget, XmCONTENT, l1, l2);
		else if (!strcmp(op, "copyPos") && n == 3)
			XmLGridCopyPos(_editWidget, CurrentTime, XmCONTENT,
				l1, XmCONTENT, l2, 3, 3);
		else if (!strcmp(op, "copySelected") && n == 1)
			XmLGridCopySelected(_editWidget, CurrentTime);
		else if (!strcmp(op, "paste") && n == 1)
			XmLGridPaste(_editWidget);
		else if (!strcmp(op, "pastePos") && n == 3)
			XmLGridPastePos(_editWidget, XmCONTENT, l1, XmCONTENT, l2);
		else if (!strcmp(op, "printSelected") && n == 1)
		{
			printf ("--- Current Selections ---\n");
			count = XmLGridGetSelectedRowCount(_editWidget);
			if (count)
			{
				rowPos = (int *)malloc(sizeof(int) * count);
				XmLGridGetSelectedRows(_editWidget, rowPos, count);
				printf ("Selected Rows: ");
				for (i = 0; i < count; i++)
					printf ("%d ", rowPos[i]);
				printf ("\n");
				free((char *)rowPos);
			}

			count = XmLGridGetSelectedColumnCount(_editWidget);
			if (count)
			{
				colPos = (int *)malloc(sizeof(int) * count);
				XmLGridGetSelectedColumns(_editWidget, colPos, count);
				printf ("Selected Columns: ");
				for (i = 0; i < count; i++)
					printf ("%d ", colPos[i]);
				printf ("\n");
				free((char *)colPos);
			}

			count = XmLGridGetSelectedCellCount(_editWidget);
			if (count)
			{
				colPos = (int *)malloc(sizeof(int) * count);
				rowPos = (int *)malloc(sizeof(int) * count);
				XmLGridGetSelectedCells(_editWidget, rowPos, colPos, count);
				printf ("Selected Cells: ");
				for (i = 0; i < count; i++)
					printf ("[%d %d] ", rowPos[i], colPos[i]);
				printf ("\n");
				free((char *)colPos);
				free((char *)rowPos);
			}
		}
		else if (!strcmp(op, "setCell") && n == 3)
		{
			printf ("WidgetEdit: set row %d col %d\n", l1, l2);
			_editRow = l1;
			_editCol = l2;
		}
		else if (!strcmp(op, "write") && n == 1)
			XmLGridWrite(_editWidget, stdout, XmFORMAT_DELIMITED, '|', True);
		else if (!strcmp(op, "writeXL") && n == 1)
			XmLGridWrite(_editWidget, stdout, XmFORMAT_XL, 0, True);
		else if (!strcmp(op, "writePAD") && n == 1)
			XmLGridWrite(_editWidget, stdout, XmFORMAT_PAD, 0, True);
		else if (!strcmp(op, "writePos") && n == 3)
			XmLGridWritePos(_editWidget, stdout, XmFORMAT_DELIMITED, '|',
			    True, XmCONTENT, 0, XmCONTENT, 0, l1, l2);
		else
			fprintf(stderr, "WidgetEdit: unknown command %s\n", cmd);
	}
	XtFree(cmd);
	XmTextSetString(_editCommandT, "");
}

static void _editChooseCB(w, clientData, callData)
Widget w;
XtPointer clientData;
XtPointer callData;
{
	Cursor cursor;
	Widget editW;

	cursor = XCreateFontCursor(XtDisplay(w), XC_hand2);
	editW = XmTrackingLocate(w, cursor, False);
	if (editW)
		SetEditWidget(editW);
	XFreeCursor(XtDisplay(w), cursor);
}

static void _editShowCB(w, clientData, callData)
Widget w;
XtPointer clientData;
XtPointer callData;
{
	XmToggleButtonCallbackStruct *cbs;

	cbs = (XmToggleButtonCallbackStruct *)callData;
	_editShowInherited = cbs->set;
	SetEditWidget(_editWidget);
}

static void _wmCloseCB(w, clientData, callData)
Widget w;
XtPointer clientData;
XtPointer callData;
{
	XtPopdown(XmLShellOfWidget((Widget)clientData));
	_editDialogUp = False;
}

static void _editCloseCB(w, clientData, callData)
Widget w;
XtPointer clientData;
XtPointer callData;
{
	XtPopdown(XmLShellOfWidget(w));
	_editDialogUp = False;
}

/*
   Callbacks which manipulate the Widget Editor
*/

static void folderCB(w, clientData, callData)
Widget w;
XtPointer clientData;
XtPointer callData;
{
	XmLFolderCallbackStruct *cbs;
	extern Widget _folder, _grid, _progress;

	cbs = (XmLFolderCallbackStruct *)callData;
	switch (cbs->pos)
	{
	case 1:
		_editWidget = _folder;
		break;
	case 2:
		_editWidget = _grid;
		break;
	case 3:
		_editWidget = _progress;
		break;
	default:
		_editWidget = 0;
		break;
	}
	if (_editDialogUp == True)
		SetEditWidget(_editWidget);
}

static void editCB(w, clientData, callData)
Widget w;
XtPointer clientData;
XtPointer callData;
{
	SetEditWidget(_editWidget);
	XtPopup(_editDialog, XtGrabNone);
	_editDialogUp = True;
}

/*
   Utility to create a form with toggle options.  The toggle
   labels must be statically defined.
*/

Widget CreateOptionForm(form, opts, cb)
Widget form;
char *opts[];
XtCallbackProc cb;
{
	Widget oForm, label, radio, toggle;
	Arg args[20];
	int i, j, len, maxLen;
	char *buf;
	Dimension width;
	XmString str;
	XmFontList fontList;

	oForm = XtVaCreateManagedWidget("optionForm",
	    xmFormWidgetClass, form,
	    XmNrightAttachment, XmATTACH_FORM,
	    XmNrightOffset, 5,
	    XmNtopAttachment, XmATTACH_FORM,
	    XmNtopOffset, 5,
	    XmNshadowType, XmSHADOW_IN,
	    XmNshadowThickness, 1,
	    NULL);
	/* find maximum label width */
	i = 0;
	maxLen = 0;
	while (opts[i])
	{
		len = strlen(opts[i]);
		if (len > maxLen)
			maxLen = len;
		i++;
		while (opts[i])
			i++;
		i++;
	}
	label = 0;
	radio = 0;
	i = 0;
	while (opts[i])
	{
		if (!i)
		{
			label = XtVaCreateManagedWidget(opts[i],
			    xmLabelWidgetClass, oForm,
			    XmNleftAttachment, XmATTACH_FORM,
			    XmNleftOffset, 4,
			    XmNtopAttachment, XmATTACH_FORM,
			    XmNtopOffset, 6,
			    XmNalignment, XmALIGNMENT_RIGHT,
			    NULL);
			maxLen += 2; /* simple accounting for borders, margins */
			XtVaGetValues(label, XmNfontList, &fontList, NULL);
			buf = (char *)malloc(maxLen + 1);
			for (j = 0; j < maxLen; j++)
				buf[j] = 'X';
			buf[maxLen] = 0;
			str = XmStringCreateSimple(buf);
			width = XmStringWidth(fontList, str);
			XmStringFree(str);
			free(buf);
			/* size first label to maximum label size */
			XtVaSetValues(label, XmNwidth, width, NULL);
			XtSetArg(args[0], XmNrightAttachment, XmATTACH_FORM);
			XtSetArg(args[1], XmNrightOffset, 4);
			XtSetArg(args[2], XmNtopAttachment, XmATTACH_FORM);
			XtSetArg(args[3], XmNtopOffset, 4);
			XtSetArg(args[4], XmNorientation, XmHORIZONTAL);
			XtSetArg(args[5], XmNmarginWidth, 0);
			XtSetArg(args[6], XmNmarginHeight, 0);
			XtSetArg(args[7], XmNleftAttachment, XmATTACH_WIDGET);
			XtSetArg(args[8], XmNleftWidget, label);
			radio = XmCreateRadioBox(oForm, "radio", args, 9);
		}
		else
		{
			label = XtVaCreateManagedWidget(opts[i],
			    xmLabelWidgetClass, oForm,
			    XmNleftAttachment, XmATTACH_FORM,
			    XmNleftOffset, 4,
			    XmNtopAttachment, XmATTACH_WIDGET,
			    XmNtopWidget, radio,
			    XmNtopOffset, 2,
			    XmNrightAttachment, XmATTACH_OPPOSITE_WIDGET,
			    XmNrightWidget, label,
			    XmNalignment, XmALIGNMENT_RIGHT,
			    NULL);
			XtSetArg(args[0], XmNrightAttachment, XmATTACH_FORM);
			XtSetArg(args[1], XmNrightOffset, 4);
			XtSetArg(args[2], XmNleftAttachment, XmATTACH_WIDGET);
			XtSetArg(args[3], XmNleftWidget, label);
			XtSetArg(args[4], XmNtopAttachment, XmATTACH_WIDGET);
			XtSetArg(args[5], XmNtopWidget, radio);
			XtSetArg(args[6], XmNorientation, XmHORIZONTAL);
			XtSetArg(args[7], XmNmarginWidth, 0);
			XtSetArg(args[8], XmNmarginHeight, 0);
			radio = XmCreateRadioBox(oForm, "radio", args, 9);
		}
		i++;
		while (opts[i])
		{
			if (strlen(opts[i]) < 4)
			{
				i++;
				continue;
			}
			toggle = XtVaCreateManagedWidget(&opts[i][3],
			    xmToggleButtonWidgetClass, radio,
			    XmNmarginWidth, 0,
			    XmNmarginHeight, 0,
			    NULL);
			if (opts[i][2] == '*')
			{
				XmToggleButtonSetState(toggle, 1, False);
				XtVaSetValues(toggle,
				    XmNuserData, (XtPointer)1,
				    NULL);
			}
			else
				XtVaSetValues(toggle,
				    XmNuserData, (XtPointer)0,
				    NULL);
			XtAddCallback(toggle, XmNvalueChangedCallback, cb,
			    (XtPointer)opts[i]);
			i++;
		}
		XtManageChild(radio);
		i++;
	}
	if (label)
		XtVaSetValues(label,
		    XmNbottomAttachment, XmATTACH_FORM,
		    XmNbottomOffset, 4,
		    NULL);
	if (radio)
		XtVaSetValues(radio,
		    XmNbottomAttachment, XmATTACH_FORM,
		    XmNbottomOffset, 4,
		    NULL);
	return oForm;
}

/*
   Utility to determine in a toggle button callback if
   the items state has been set and has changed from
   its previous value
*/

int ToggleButtonSet(w, clientData, callData)
Widget w;
XtPointer clientData;
XtPointer callData;
{
	XmToggleButtonCallbackStruct *cbs;
	XtPointer set;

	cbs = (XmToggleButtonCallbackStruct *)callData;
	/* userData holds old state - make sure value really changed */
	XtVaGetValues(w, XmNuserData, &set, NULL);
	if ((int)set == cbs->set)
		return 0; /* unchanged */
	XtVaSetValues(w, XmNuserData, (XtPointer)cbs->set, NULL);
	if (!cbs->set)
		return 0; /* deselected */
	return 1;
}

/*
   Utility to create a scrolled text containing a desctiption
*/

Widget CreateDescText(form, topWidget, desc)
Widget form;
Widget topWidget;
char *desc;
{
	Widget text;
	Arg args[20];

	XtSetArg(args[0], XmNleftAttachment, XmATTACH_FORM);
	XtSetArg(args[1], XmNleftOffset, 3);
	XtSetArg(args[2], XmNrightAttachment, XmATTACH_FORM);
	XtSetArg(args[3], XmNrightOffset, 3);
	XtSetArg(args[4], XmNtopAttachment, XmATTACH_WIDGET);
	XtSetArg(args[5], XmNbottomOffset, 3);
	XtSetArg(args[6], XmNwordWrap, True);
	XtSetArg(args[7], XmNscrollHorizontal, False);
	XtSetArg(args[8], XmNeditMode, XmMULTI_LINE_EDIT);
	XtSetArg(args[9], XmNeditable, False);
	XtSetArg(args[10], XmNcursorPositionVisible, False);
	XtSetArg(args[11], XmNbottomAttachment, XmATTACH_FORM);
	XtSetArg(args[12], XmNtopAttachment, XmATTACH_WIDGET);
	XtSetArg(args[13], XmNtopWidget, topWidget);
	XtSetArg(args[14], XmNtopOffset, 5);
	text = XmCreateScrolledText(form, "descText", args, 15);
	XmTextSetString(text, desc);
	XtManageChild(text);
#ifndef MOTIF11
	XmDropSiteUnregister(text);
#endif
	return text;
}

/*
   Intro Tab Form
*/

#define MLlogo_width 135
#define MLlogo_height 36
static char MLlogo_bits[] = {
 0x3f,0x00,0xdf,0x1f,0xf0,0x11,0xff,0x03,0xf0,0x03,0xfe,0x00,0x7f,0x1f,0xf8,
 0xfe,0xbf,0x7c,0x80,0x07,0x07,0x1c,0x1e,0x1c,0x0f,0x1c,0x0e,0x38,0x00,0x1c,
 0x3e,0x20,0x38,0xb8,0x7c,0x80,0x07,0x07,0x0e,0x1c,0x1c,0x1e,0x0e,0x1c,0x38,
 0x00,0x1c,0x7c,0x20,0x38,0xb0,0xf4,0x40,0x07,0x07,0x0f,0x18,0x1c,0x1e,0x0f,
 0x3c,0x38,0x00,0x1c,0xfc,0x20,0x38,0xa0,0xf4,0x40,0x07,0x07,0x07,0x10,0x1c,
 0x1e,0x07,0x38,0x38,0x00,0x1c,0xf4,0x21,0x38,0xa2,0xe4,0x21,0x07,0x87,0x07,
 0x00,0x1c,0x8e,0x07,0x78,0x38,0x00,0x1c,0xe4,0x23,0x38,0x82,0xe4,0x21,0x07,
 0x87,0x07,0x00,0x1c,0x8f,0x07,0x78,0x38,0x00,0x1c,0xc4,0x27,0x38,0x83,0xc4,
 0x13,0x07,0x87,0x07,0x00,0xfc,0x81,0x07,0x78,0x38,0x00,0x1c,0x84,0x2f,0xf8,
 0x83,0xc4,0x13,0x07,0x87,0x07,0x00,0xdc,0x83,0x07,0x78,0x38,0x00,0x1c,0x84,
 0x2f,0x38,0x83,0x84,0x0f,0x07,0x87,0x07,0x00,0x9c,0x87,0x07,0x78,0x38,0x00,
 0x1c,0x04,0x3f,0x38,0x82,0x84,0x0f,0x07,0x07,0x07,0x00,0x9c,0x0f,0x07,0x38,
 0x38,0x20,0x1c,0x04,0x3e,0x38,0xc2,0x84,0x07,0x07,0x07,0x0f,0x10,0x1c,0x0f,
 0x0f,0x3c,0x38,0x20,0x1c,0x04,0x3c,0x38,0xc0,0x04,0x07,0x07,0x07,0x0e,0x18,
 0x1c,0x1e,0x0e,0x1c,0x38,0x30,0x1c,0x04,0x38,0x38,0xe0,0x04,0x07,0x07,0x07,
 0x1c,0x04,0x1c,0x3e,0x1c,0x0e,0x38,0x38,0x1c,0x04,0x30,0x38,0xb8,0x1f,0xc2,
 0xdf,0x1f,0xf0,0x03,0x7f,0x7c,0xf0,0x03,0xfe,0x1f,0x7f,0x1f,0x20,0xfe,0xbf,
 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
 0x00,0x80,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
 0x00,0x00,0x00,0x80,0xfc,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
 0xff,0xff,0xff,0xff,0xff,0x9f,0xfc,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
 0xff,0xff,0xff,0xff,0xff,0xff,0xff,0x9f,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x80,0x00,0x00,0x00,0x00,0x00,
 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x80,0xe0,0x09,0xe0,
 0x07,0xfc,0x3f,0xfe,0x7f,0x7f,0x7f,0x3c,0x10,0x80,0xff,0x01,0xfe,0xbf,0x18,
 0x0e,0x38,0x1c,0x70,0x38,0xce,0x71,0x1e,0x3c,0x18,0x18,0x00,0x8e,0x07,0x38,
 0xb8,0x08,0x0c,0x1c,0x38,0x70,0x30,0xc6,0x61,0x1c,0x3c,0x08,0x38,0x00,0x0e,
 0x0f,0x38,0xb0,0x0c,0x08,0x1e,0x78,0x70,0x20,0xc2,0x41,0x3c,0x38,0x08,0x38,
 0x00,0x0e,0x0f,0x38,0xa0,0x1c,0x08,0x0e,0x70,0x70,0x24,0xc2,0x41,0x3c,0x7c,
 0x04,0x7c,0x00,0x0e,0x0f,0x38,0xa2,0xfc,0x00,0x0f,0xf0,0x70,0x04,0xc0,0x01,
 0x38,0x7c,0x04,0x7c,0x00,0x0e,0x07,0x38,0x82,0xf8,0x03,0x0f,0xf0,0x70,0x06,
 0xc0,0x01,0x78,0x72,0x04,0x72,0x00,0x8e,0x07,0x38,0x83,0xf0,0x07,0x0f,0xf0,
 0xf0,0x07,0xc0,0x01,0x78,0xf2,0x02,0xf2,0x00,0xfe,0x00,0xf8,0x83,0xe0,0x0f,
 0x0f,0xf0,0x70,0x06,0xc0,0x01,0xf0,0xe2,0x02,0xe2,0x00,0xee,0x01,0x38,0x83,
 0x00,0x1f,0x0f,0xf0,0x70,0x04,0xc0,0x01,0xf0,0xe1,0x02,0xe1,0x01,0xce,0x03,
 0x38,0x82,0x04,0x1c,0x0e,0x70,0x70,0x04,0xc0,0x01,0xe0,0xe1,0x01,0xff,0x01,
 0xce,0x07,0x38,0xc2,0x0c,0x1c,0x1e,0x78,0x70,0x00,0xc0,0x01,0xe0,0xc0,0x81,
 0xc0,0x03,0x8e,0x07,0x38,0xc0,0x0c,0x0c,0x1c,0x38,0x70,0x00,0xc0,0x01,0xe0,
 0xc0,0x81,0xc0,0x03,0x0e,0x0f,0x38,0xe0,0x1c,0x06,0x38,0x1c,0x70,0x00,0xc0,
 0x01,0xc0,0xc0,0xc0,0xc0,0x07,0x0e,0x1f,0x38,0xb8,0xe4,0x03,0xe0,0x07,0xfc,
 0x01,0xf0,0x0f,0x40,0x80,0xe0,0xe3,0x8f,0x3f,0x3e,0xfe,0xbf};

#define intro_width 15
#define intro_height 15
static unsigned char intro_bits[] = {
	0x00, 0x00, 0x00, 0x00, 0x80, 0x01, 0x80, 0x01, 0x00, 0x00, 0xc0, 0x01,
	0x80, 0x01, 0x80, 0x01, 0x80, 0x01, 0x80, 0x01, 0x80, 0x01, 0x80, 0x01,
	0xc0, 0x03, 0xc0, 0x03, 0x00, 0x00};

void AddIntroTabForm(folder)
Widget folder;
{
	Widget form, text, logoLabel, label, sep;
	XmString str;
	Display *dpy;
	Window root;
	int depth;
	Pixel black, bg;
	Pixmap pix;
	static char *desc =
	"The Microline Widget Library(TM) contains widgets and \
utilities for creating advanced user-interfaces with Motif \
1.1, 1.2, and 2.0.  The library includes:\n\n\
       - The Folder Widget, a tabbed folder interface\n\
       - The Grid Widget, an editable grid of cells containing \
text or images in rows and columns\n\
       - The Progress Widget, a bar displaying progress\n\
       - Utility Functions, including functions creating \
various DrawnButtons and other functions\n\n\
This demonstation program presents an overview of the library's \
basic components. You may choose the tabs along the top to \
obtain more information and view sample widgets.   If you understand \
how widget resources work, you may want to try clicking the Editor \
button to display and edit a widget's resources directly.  Note: \
the widget editor is blank for this introduction screen. \
You may exit this application at any time by clicking on the \
Exit button.\n\n\
If you have obtained the Microline Widget Library (TM) along with \
this demo, please be sure to consult your license agreement \
for legal information concerning use and distribution.\n\n\
Microline Widget Library is a trademark of Microline Software. \
All other trademarks are the property of their owners.\n\n\
Microline Software\n\
41 Sutter St, Suite 1374\n\
San Francisco, CA 94104\n\
info@mlsoft.com";

	str = XmStringCreateSimple("Introduction");
	form = XmLFolderAddBitmapTabForm(folder, str, (char *)intro_bits,
	    intro_width, intro_height);
	XmStringFree(str);

	str = XmStringCreateLtoR("Microline\nWidget Library Demo",
	    XmSTRING_DEFAULT_CHARSET);
	label = XtVaCreateManagedWidget("titleLabel",
	    xmLabelWidgetClass, form,
	    XmNlabelString, str,
		XmNalignment, XmALIGNMENT_BEGINNING,
	    XmNleftAttachment, XmATTACH_FORM,
	    XmNleftOffset, 5,
	    XmNtopAttachment, XmATTACH_FORM,
	    XmNtopOffset, 10,
	    NULL);
	XmStringFree(str);

	sep = XtVaCreateManagedWidget("titleSep",
	    xmSeparatorWidgetClass, form,
	    XmNleftAttachment, XmATTACH_FORM,
	    XmNleftOffset, 5,
	    XmNrightAttachment, XmATTACH_OPPOSITE_WIDGET,
	    XmNrightWidget, label,
	    XmNtopAttachment, XmATTACH_WIDGET,
	    XmNtopWidget, label,
	    XmNseparatorType, XmSHADOW_ETCHED_IN,
	    NULL);

	dpy = XtDisplay(form);
	root = DefaultRootWindow(dpy);
	depth = DefaultDepthOfScreen(XtScreen(form));
	black = BlackPixelOfScreen(XtScreen(form));
	XtVaGetValues(form,
	    XmNbackground, &bg,
	    NULL);
	pix = XCreatePixmapFromBitmapData(dpy, root, (char *)MLlogo_bits,
	    MLlogo_width, MLlogo_height, black, bg, depth);
	logoLabel = XtVaCreateManagedWidget("logoLabel",
	    xmLabelWidgetClass, form,
	    XmNrightAttachment, XmATTACH_FORM,
	    XmNrightOffset, 5,
		XmNtopAttachment, XmATTACH_FORM,
		XmNtopOffset, 24,
	    XmNlabelType, XmPIXMAP,
	    XmNlabelPixmap, pix,
	    XmNmarginWidth, 0,
	    XmNmarginHeight, 0,
	    NULL);

	text = CreateDescText(form, sep, desc);
	XtVaSetValues(XtParent(text),
	    XmNtopOffset, 5,
	    NULL);
}

/*
   Folder Tab Form
*/

#define folder_width 15
#define folder_height 15
static unsigned char folder_bits[] = {
	0x00, 0x00, 0x00, 0x00, 0x1e, 0x3f, 0xa1, 0x40, 0xa1, 0x40, 0xe1, 0x7f,
	0x01, 0x40, 0x01, 0x40, 0x01, 0x40, 0x01, 0x40, 0x01, 0x40, 0x01, 0x40,
	0x01, 0x40, 0xff, 0x7f, 0x00, 0x00};

static void FolderOptCB();
Widget _folder;
Widget _firstTab, _folderLabel;

void AddFolderTabForm(folder)
Widget folder;
{
	Widget form, sForm, oForm;
	XmString str;
	int i;
	static char *tabLabels[] =
	{
		"First Tab", "Second", "Third"
	};
	static char *opts[] =
	{
		"Corners:", "CL:Line", "CR*Round", "CS:Square", 0,
		"Corner Size:", "RS*Small", "RL:Large", 0,
		"Labels:", "TM:Multi-line", "TS*Single-line", 0,
		"Colors:", "SC*Classic", "SM:Modern", 0,
		"Placement:", "PT*Top", "PB:Bot", "PL:Left", "PR:Right", 0,
		0
	};
	static char *desc =
	"The Folder widget provides a folder containing tabs along \
the top, bottom, left or right and an area managed by the tabs in \
the center.  The folder itself is a Manager widget and each tab \
consists of a Primitive widget surrounded by tab decorations \
including an optional pixmap. Any non-Primitive widget children \
of the folder are placed in the center area.  The widgets \
contained in the tabs can be assigned a non-Primitive widget to \
display/manage when the tab is selected.\n\n\
\
Resources exist to set the style of the corners, Pixmaps \
(icons) to display in the tabs, various colors and sizes and \
spacing in and around the tabs.  Also, since the tabs themselves \
have Primitive widgets like DrawnButtons inside of them, they \
retain all display abilities of their respective Primitive \
widgets.  DrawnButton tabs, for example, can be displayed with \
different fonts, character sets, colors, alignments, etc.\n\n\
\
The Folder widget provides functions to add tabs and to set the \
active tab.  Tabs may be placed on the top, bottom, left or right \
of the Folder.  When tabs are placed on the left or right, the \
text inside the tabs can be drawn vertically as they would in \
a real folder. Commonly used tab types can be created in one \
function call. For example, a tab containing a Bitmap and  \
managing a Form widget can be created in a single function call. \
Tabs may also be added by simply adding a Primitive widget to \
the folder.  Likewise, tabs may be deleted by deleting the \
Primitive widgets contained inside them. \
Callbacks are provided which can notify an application when a \
tab is activated, allowing the application to either accept \
or reject the activation.\n\n\
\
Keyboard traversal of the tabs is supported by using the arrow \
keys to move from one tab to the next.  Tabs can be activated \
by either selection with the mouse or by activating their \
underlying Primitive widget.  For example, DrawnButton tabs \
can be activated by pressing the space when focus is in the \
tab.";

	str = XmStringCreateSimple("Folder Widget");
	form = XmLFolderAddBitmapTabForm(folder, str, (char *)folder_bits,
	    folder_width, folder_height);
	XmStringFree(str);

	oForm = CreateOptionForm(form, opts, FolderOptCB);
	_folder = XtVaCreateManagedWidget("sampleFolder",
	    xmlFolderWidgetClass, form,
	    XmNleftAttachment, XmATTACH_FORM,
	    XmNleftOffset, 5,
	    XmNtopAttachment, XmATTACH_FORM,
	    XmNtopOffset, 5,
	    XmNrightAttachment, XmATTACH_WIDGET,
	    XmNrightWidget, oForm,
	    XmNrightOffset, 5,
	    XmNbottomAttachment, XmATTACH_POSITION,
	    XmNbottomPosition, 55,
	    NULL);
	for (i = 0; i < 3; i++)
	{
		str = XmStringCreateSimple(tabLabels[i]);
		if (!i)
			_firstTab = XmLFolderAddTab(_folder, str);
		else
			XmLFolderAddTab(_folder, str);
		XmStringFree(str);
	}
	XmLFolderSetActiveTab(_folder, 0, False);
	sForm = XtVaCreateManagedWidget("sampleForm",
	    xmFormWidgetClass, _folder,
	    NULL);
	str = XmStringCreateLtoR("\nSample Folder\nWidget\n",
	    XmSTRING_DEFAULT_CHARSET);
	_folderLabel = XtVaCreateManagedWidget("sampleLabel",
	    xmLabelWidgetClass, sForm,
	    XmNlabelString, str,
	    XmNleftAttachment, XmATTACH_FORM,
	    XmNrightAttachment, XmATTACH_FORM,
	    XmNtopAttachment, XmATTACH_FORM,
	    XmNbottomAttachment, XmATTACH_FORM,
	    NULL);
	XmStringFree(str);

	CreateDescText(form, _folder, desc);
}

static void FolderOptCB(w, clientData, callData)
Widget w;
XtPointer clientData;
XtPointer callData;
{
	char *opt;
	XmString str;

	if (!ToggleButtonSet(w, clientData, callData))
		return;
	opt = (char *)clientData;

	/* Style */
	if (opt[0] == 'S' && opt[1] == 'C')
	{
		XtVaSetValues(_folderLabel,
		    XtVaTypedArg, XmNbackground, XmRString, "#D0D0D0", 8,
		    NULL);
		XtVaSetValues(_folder,
		    XmNcornerDimension, 3,
		    XtVaTypedArg, XmNbackground, XmRString, "#D0D0D0", 8,
		    XtVaTypedArg, XmNinactiveBackground, XmRString, "#D0D0D0", 8,
		    NULL);
	}
	if (opt[0] == 'S' && opt[1] == 'M')
	{
		XtVaSetValues(_folderLabel,
		    XtVaTypedArg, XmNbackground,
		    XmRString, "yellow", 7,
		    NULL);
		XtVaSetValues(_folder,
		    XmNcornerDimension, 7,
		    XtVaTypedArg, XmNbackground,
		    XmRString, "yellow", 7,
		    XtVaTypedArg, XmNinactiveBackground,
		    XmRString, "lightBlue", 10,
		    NULL);
	}

	/* Placement */
	if (opt[0] == 'P' && opt[1] == 'T')
		XtVaSetValues(_folder,
		    XmNtabPlacement, XmFOLDER_TOP,
		    NULL);
	if (opt[0] == 'P' && opt[1] == 'B')
		XtVaSetValues(_folder,
		    XmNtabPlacement, XmFOLDER_BOTTOM,
		    NULL);
	if (opt[0] == 'P' && opt[1] == 'L')
		XtVaSetValues(_folder,
		    XmNtabPlacement, XmFOLDER_LEFT,
		    NULL);
	if (opt[0] == 'P' && opt[1] == 'R')
		XtVaSetValues(_folder,
		    XmNtabPlacement, XmFOLDER_RIGHT,
		    NULL);

	/* Corners */
	if (opt[0] == 'C' && opt[1] == 'L')
		XtVaSetValues(_folder,
		    XmNcornerStyle, XmCORNER_LINE,
		    NULL);
	if (opt[0] == 'C' && opt[1] == 'R')
		XtVaSetValues(_folder,
		    XmNcornerStyle, XmCORNER_ARC,
		    NULL);
	if (opt[0] == 'C' && opt[1] == 'S')
		XtVaSetValues(_folder,
		    XmNcornerStyle, XmCORNER_NONE,
		    NULL);

	/* Corner Size */
	if (opt[0] == 'R' && opt[1] == 'S')
		XtVaSetValues(_folder,
		    XmNcornerDimension, 4,
		    NULL);
	if (opt[0] == 'R' && opt[1] == 'L')
		XtVaSetValues(_folder,
		    XmNcornerDimension, 9,
		    NULL);

	/* Text */
	if (opt[0] == 'T' && opt[1] == 'M')
	{
		str = XmStringCreateLtoR("First Tab\nis 2 Lines",
		    XmSTRING_DEFAULT_CHARSET);
		XtVaSetValues(_firstTab, XmNlabelString, str, NULL);
		XmStringFree(str);
	}
	if (opt[0] == 'T' && opt[1] == 'S')
	{
		str = XmStringCreateSimple("First Tab");
		XtVaSetValues(_firstTab, XmNlabelString, str, NULL);
		XmStringFree(str);
	}
}

/*
   Grid Tab Form
*/

#define grid_width 15
#define grid_height 15
static unsigned char grid_bits[] = {
	0x00, 0x00, 0xff, 0x7f, 0x21, 0x42, 0xad, 0x5a, 0x21, 0x42, 0xff, 0x7f,
	0x21, 0x42, 0xad, 0x5a, 0x21, 0x42, 0xff, 0x7f, 0x21, 0x42, 0xad, 0x5a,
	0x21, 0x42, 0xff, 0x7f, 0x00, 0x00};

static void GridOptCB();
static void GridSample1();
static void GridSample2();
static void GridSample3();
Widget _grid;

void AddGridTabForm(folder)
Widget folder;
{
	Widget form, oForm;
	XmString str;
	static char *opts[] =
	{
		"Sample", "ST*Table", "SL:List", "SS:Spread", 0,
		"Font:", "NS*Small", "NB:Big", 0,
		"Shadows:", "HN:None", "HA*All", 0,
		"Margins:", "MW:Wide", "MN*Narrow", 0,
		0
	};
	static char *desc =
	"The Grid Widget provides an editable grid of \
cells containing text or images in rows and columns. \
It includes a number of advanced features such as:\n\n\
   - text (XmString format), label or pixmap cells\n\
   - keyboard traversal using arrow keys, page keys, etc.\n\
   - add, delete, reorder, move and hiding of rows/columns\n\
   - heading, content and footer rows/columns\n\
   - top, bottom, left and/or right fixed rows/columns\n\
   - rows/column intelligently size to cell fonts/images\n\
   - user-adjustable row/column sizes\n\
   - cells may span rows/columns\n\
   - full control of cell borders and colors\n\
   - 9 cell alignment options (top-left, center, etc)\n\
   - uses reference-counted cell attributes to save memory\n\
   - SetValues on ranges of cells/rows/columns\n\
   - callbacks for select, activate, draw, edit, focus, etc.\n\
   - up to 2 billion rows/columns\n\
   - cut, paste, drag and drop of cell contents\n\
   - ASCII file import and export\n\n\
The Grid Widget is ideal for replacing single-column Motif \
scrolled lists with multi-column lists. Its also perfect for creating \
high-performance tables.  Tables can be created where the data \
displayed is not kept in the grid itself but retrieved from a \
database when it needs to be displayed.  This direct database \
access can save memory and time loading/storing the table. \
Click-sorting can be added to the table to allow a user to \
sort rows by clicking on a column heading.\n\n\
Cell attributes in the grid are reference counted.  If a number \
of cells in the grid have similar attributes (color, fonts, etc) \
they may all contain a pointer to the same attributes structure \
instead of creating an attributes structure for each individual \
cell.  This reference counting is transparent to a programmer \
creating a grid.\n\n\
Cells may have borders set on the top, bottom, left and right and \
have a number of color/alignment options.  Keyboard traversal is \
provided using the standard Motif key mappings of Page Up, \
Page Down, Home, etc.  Selection modes are provided to allow \
selection of a single row (browse mode or single mode), multiple \
rows and arbitrary regions of rows, columns and cells. \
Functions are provided to easily import data to and from text \
files.\n\n\
Internally the Grid is a Manager widget which contains two \
ScrollBar children and one Text widget child.";

	str = XmStringCreateSimple("Grid Widget");
	form = XmLFolderAddBitmapTabForm(folder, str, (char *)grid_bits,
	    grid_width, grid_height);
	XmStringFree(str);

	oForm = CreateOptionForm(form, opts, GridOptCB);

	_grid = XtVaCreateManagedWidget("grid",
	    xmlGridWidgetClass, form,
	    XmNleftAttachment, XmATTACH_FORM,
	    XmNleftOffset, 5,
	    XmNrightAttachment, XmATTACH_WIDGET,
	    XmNrightWidget, oForm,
	    XmNrightOffset, 5,
	    XmNtopAttachment, XmATTACH_FORM,
	    XmNtopOffset, 5,
	    XmNbottomAttachment, XmATTACH_POSITION,
	    XmNbottomPosition, 50,
	    XtVaTypedArg, XmNblankBackground,
	    XmRString, "white", 6,
	    NULL);
	XtVaSetValues(_grid,
	    XmNcellDefaults, True,
	    XtVaTypedArg, XmNcellBackground,
	    XmRString, "white", 6,
	    NULL);
	GridSample1();

	CreateDescText(form, _grid, desc);
}

static void GridOptCB(w, clientData, callData)
Widget w;
XtPointer clientData;
XtPointer callData;
{
	char *opt;

	if (!ToggleButtonSet(w, clientData, callData))
		return;
	opt = (char *)clientData;

	/* Sample */
	if (opt[0] == 'S' && opt[1] == 'T')
		GridSample1();
	if (opt[0] == 'S' && opt[1] == 'L')
		GridSample2();
	if (opt[0] == 'S' && opt[1] == 'S')
		GridSample3();

	/* Font */
	if (opt[0] == 'N' && opt[1] == 'S')
		XtVaSetValues(_grid,
		    XmNrowType, XmALL_TYPES,
		    XmNcolumnType, XmALL_TYPES,
		    XtVaTypedArg, XmNcellFontList,
		    XmRString, SMALLFONT, strlen(SMALLFONT) + 1,
		    NULL);
	if (opt[0] == 'N' && opt[1] == 'B')
		XtVaSetValues(_grid,
		    XmNrowType, XmALL_TYPES,
		    XmNcolumnType, XmALL_TYPES,
		    XtVaTypedArg, XmNcellFontList,
		    XmRString, BIGFONT, strlen(BIGFONT) + 1,
		    NULL);

	/* Shadows */
	if (opt[0] == 'H' && opt[1] == 'N')
		XtVaSetValues(_grid,
		    XmNshadowRegions, 0,
		    NULL);
	if (opt[0] == 'H' && opt[1] == 'A')
		XtVaSetValues(_grid,
		    XmNshadowRegions, 511,
		    NULL);

	/* Margins */
	if (opt[0] == 'M' && opt[1] == 'W')
		XtVaSetValues(_grid,
		    XmNscrollBarMargin, 4,
		    XmNtopFixedMargin, 3,
		    XmNleftFixedMargin, 3,
		    NULL);
	if (opt[0] == 'M' && opt[1] == 'N')
		XtVaSetValues(_grid,
		    XmNscrollBarMargin, 2,
		    XmNtopFixedMargin, 0,
		    XmNleftFixedMargin, 0,
		    NULL);

}

static char *gridSample1Data = 
"Name|Price||Chg|Close Dt|Location|Yld\n\
TZZ 3.0 Tana Zin|102.23||-1.2|1/1/94|Tomano, RE|5.6%\n\
BLC 4.5 Board Lvl C|103.23||+2.5|2/15/94|Rino, LO|6.8%\n\
DGB 5.4 Dig Gen B|101.53||+3.7|6/20/94|Carolana, CA|7.5%\n\
KLC 5.3 Kord Lim C|108.98||-2.5|2/15/94|Cannes, CA|8.7%\n\
LLO 3.4 Liw Lo O|107.24||+2.2|2/20/94|Tenise, MI|6.7%\n\
MMN 3.1 Mon Mor N|105.63||+1.4|2/15/94|Waterton, MN|6.3%\n\
PLP 4.3 Pol Lab P|102.12||-2.3|2/20/94|Holard, LM|7.6%\n\
SZN 4.5 Siml Zi N|101.11||+1.6|2/15/94|Tempest, CA|5.4%\n\
TTL 5.6 Towa Tin L|101.12||+2.7|3/15/94|Conila, OK|6.7%\n\
ULD 5.4 Upl La D|105.12||-2.2|4/15/94|Mandrill, HI|7.8%";

static Pixmap gridSample1PixUp = XmUNSPECIFIED_PIXMAP;
static Pixmap gridSample1PixDown = XmUNSPECIFIED_PIXMAP;

#define up_width 12
#define up_height 18
static char up_bits[] = {
	0x00, 0x00, 0x00, 0x00, 0x60, 0x00, 0xf0, 0x00, 0xf8, 0x01, 0xfc, 0x03,
	0xfe, 0x07, 0xfe, 0x07, 0xf6, 0x06, 0xf2, 0x04, 0xf0, 0x00, 0xf0, 0x00,
	0xf0, 0x00, 0xf0, 0x00, 0xf0, 0x00, 0xf0, 0x00, 0xf0, 0x00, 0xf0, 0x00};

#define down_width 12
#define down_height 18
static char down_bits[] = {
	0xf0, 0x00, 0xf0, 0x00, 0xf0, 0x00, 0xf0, 0x00, 0xf0, 0x00, 0xf0, 0x00,
	0xf0, 0x00, 0xf0, 0x00, 0xf2, 0x04, 0xf6, 0x06, 0xfe, 0x07, 0xfe, 0x07,
	0xfc, 0x03, 0xf8, 0x01, 0xf0, 0x00, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00};

static void GridSample1()
{
	Display *dpy;
	Window root;
	XColor col;
	int i, j, depth;
	Pixel fg, bg;
	char buf[20];
	XmString str;

	XtVaSetValues(_grid, XmNlayoutFrozen, True, NULL);

	/* We share the same grid with the other sample functions
	   so we need to delete all the rows and column possibly
	   created by the other sample functions and reset some
	   of the resources in the grid */
	XmLGridDeleteAllRows(_grid, XmHEADING);
	XmLGridDeleteAllRows(_grid, XmCONTENT);
	XmLGridDeleteAllColumns(_grid, XmHEADING);
	XmLGridDeleteAllColumns(_grid, XmCONTENT);

	XtVaSetValues(_grid,
	    XmNshadowThickness, 2,
	    XmNselectionPolicy, XmSELECT_MULTIPLE_ROW,
	    XmNrightFixedCount, 1,
	    XmNleftFixedCount, 1,
	    XmNallowColumnResize, True,
	    XmNallowRowResize, False,
		XmNallowDragSelected, False,
		XmNallowDrop, False,
	    NULL);

	/* add the columns and set their width */

	XmLGridAddColumns(_grid, XmCONTENT, -1, 7);

	XtVaSetValues(_grid, XmNcolumn, 0, XmNcolumnWidth, 19, NULL);
	XtVaSetValues(_grid, XmNcolumn, 3, XmNcolumnWidth, 5, NULL);
	XtVaSetValues(_grid, XmNcolumn, 4, XmNcolumnWidth, 9, NULL);
	XtVaSetValues(_grid, XmNcolumn, 5, XmNcolumnWidth, 13, NULL);
	XtVaSetValues(_grid, XmNcolumn, 6, XmNcolumnWidth, 5, NULL);

	/* add the heading row */

	XtVaSetValues(_grid,
	    XmNcellDefaults, True,
	    XmNcellAlignment, XmALIGNMENT_CENTER,
	    XmNcellType, XmLABEL_CELL,
	    XtVaTypedArg, XmNcellBackground,
	    XmRString, "Yellow", 7,
	    NULL);
	XmLGridAddRows(_grid, XmHEADING, -1, 1);

	/* add the content rows */

	XtVaSetValues(_grid,
	    XmNcellDefaults, True,
	    XmNcellAlignment, XmALIGNMENT_RIGHT,
	    XtVaTypedArg, XmNcellBackground,
	    XmRString, "white", 6,
	    NULL);
	XmLGridAddRows(_grid, XmCONTENT, -1, 10);

	XtVaSetValues(_grid,
	    XmNcolumn, 0,
	    XmNcellAlignment, XmALIGNMENT_LEFT,
	    NULL);
	XtVaSetValues(_grid,
	    XmNrowType, XmHEADING,
	    XmNcolumn, 1,
	    XmNcellAlignment, XmALIGNMENT_RIGHT,
	    NULL);

	/* set the data in the grid */

	XmLGridSetStrings(_grid, gridSample1Data);

	XtVaSetValues(_grid,
	    XmNrowType, XmALL_TYPES,
	    XmNcolumn, 1,
	    XmNcellRightBorderType, XmBORDER_NONE,
	    NULL);

	dpy = XtDisplay(_grid);
	root = DefaultRootWindow(dpy);
	depth = DefaultDepthOfScreen(XtScreen(_grid));

	bg = WhitePixelOfScreen(XtScreen(_grid));
	col.red = 0;
	col.green = 65535;
	col.blue = 0;
	col.flags = DoRed | DoGreen | DoBlue;
	XAllocColor(dpy, DefaultColormapOfScreen(XtScreen(_grid)), &col);
	fg = col.pixel;
	if (gridSample1PixUp == XmUNSPECIFIED_PIXMAP)
		gridSample1PixUp = XCreatePixmapFromBitmapData(dpy, root,
		    (char *)up_bits, up_width, up_height, fg, bg, depth);

	col.red = 65535;
	col.green = 0;
	col.blue = 0;
	col.flags = DoRed | DoGreen | DoBlue;
	XAllocColor(dpy, DefaultColormapOfScreen(XtScreen(_grid)), &col);
	fg = col.pixel;
	if (gridSample1PixDown == XmUNSPECIFIED_PIXMAP)
		gridSample1PixDown = XCreatePixmapFromBitmapData(dpy, root,
		    (char *)down_bits, down_width, down_height, fg, bg, depth);

	/* change column 2 to pixmap cells and set pixmaps */

	XtVaSetValues(_grid,
	    XmNrowType, XmALL_TYPES,
	    XmNcolumn, 2,
	    XmNcellType, XmPIXMAP_CELL,
	    XmNcellLeftBorderType, XmBORDER_NONE,
	    NULL);
	XtVaSetValues(_grid,
	    XmNcolumn, 2,
	    XmNcellPixmap, gridSample1PixUp,
	    NULL);
	XtVaSetValues(_grid,
	    XmNcolumn, 2,
	    XmNrowStep, 3,
	    XmNcellPixmap, gridSample1PixDown,
	    NULL);

	XtVaSetValues(_grid, XmNlayoutFrozen, False, NULL);
}

static char *gridSample2Data = 
"Europe|CD-ROM|$29|List\n\
Yugoslovia|Floppy|$39|Retail\n\
North America|Tape|$29|List\n\
South America|CD-ROM|$49|Retail\n\
Japan|Tape|$49|Retail\n\
Russia|Floppy|$49|Retail\n\
Poland|CD-ROM|$39|Retail\n\
Norway|CD-ROM|$29|List\n\
England|Tape|$49|List\n\
Jordan|CD-ROM|$39|Retail";

static void GridSample2()
{
	int i, j;
	char buf[20];
	XmString str;

	XtVaSetValues(_grid, XmNlayoutFrozen, True, NULL);

	XmLGridDeleteAllRows(_grid, XmHEADING);
	XmLGridDeleteAllRows(_grid, XmCONTENT);
	XmLGridDeleteAllColumns(_grid, XmHEADING);
	XmLGridDeleteAllColumns(_grid, XmCONTENT);

	XtVaSetValues(_grid,
	    XmNshadowThickness, 3,
		XmNallowDragSelected, False,
		XmNallowDrop, False,
	    XmNtopFixedCount, 0,
	    XmNleftFixedCount, 0,
	    XmNrightFixedCount, 0,
	    XmNselectionPolicy, XmSELECT_BROWSE_ROW,
	    XmNallowColumnResize, False,
	    XmNallowRowResize, False,
	    NULL);
	XtVaSetValues(_grid,
	    XmNcellDefaults, True,
	    XmNcellType, XmLABEL_CELL,
	    XmNcellAlignment, XmALIGNMENT_RIGHT,
	    NULL);

	XmLGridAddColumns(_grid, XmCONTENT, -1, 4);
	XtVaSetValues(_grid,
	    XmNcolumn, 0,
	    XmNcolumnWidth, 20,
	    NULL);
	XtVaSetValues(_grid,
	    XmNcolumn, 1,
	    XmNcolumnWidth, 9,
	    NULL);
	XmLGridAddRows(_grid, XmCONTENT, -1, 10);

	XtVaSetValues(_grid,
	    XmNcolumn, 0,
	    XmNcellAlignment, XmALIGNMENT_LEFT,
	    NULL);

	XtVaSetValues(_grid,
	    XmNrowStep, 2,
	    XtVaTypedArg, XmNcellBackground,
	    XmRString, "#E0E0E0", 8,
	    NULL);

	XtVaSetValues(_grid, XmNlayoutFrozen, False, NULL);

	XmLGridSetStrings(_grid, gridSample2Data);
	XmLGridSelectRow(_grid, 0);
}

static void GridSample3()
{
	char buf[4];
	int i;

	XtVaSetValues(_grid, XmNlayoutFrozen, True, NULL);

	XmLGridDeleteAllRows(_grid, XmHEADING);
	XmLGridDeleteAllRows(_grid, XmCONTENT);
	XmLGridDeleteAllColumns(_grid, XmHEADING);
	XmLGridDeleteAllColumns(_grid, XmCONTENT);

	XtVaSetValues(_grid,
	    XmNshadowThickness, 1,
		XmNallowDragSelected, True,
		XmNallowDrop, True,
	    XmNselectionPolicy, XmSELECT_CELL,
	    XmNtopFixedCount, 0,
	    XmNleftFixedCount, 0,
	    XmNrightFixedCount, 0,
	    XmNallowColumnResize, True,
	    XmNallowRowResize, True,
	    NULL);
	XtVaSetValues(_grid,
	    XmNcellDefaults, True,
	    XmNcellType, XmTEXT_CELL,
	    XmNcellAlignment, XmALIGNMENT_LEFT,
	    NULL);

	XmLGridAddColumns(_grid, XmHEADING, -1, 1);
	XmLGridAddColumns(_grid, XmCONTENT, -1, 26);
	XmLGridAddRows(_grid, XmHEADING, -1, 1);
	XmLGridAddRows(_grid, XmCONTENT, -1, 100);

	XtVaSetValues(_grid, XmNlayoutFrozen, False, NULL);

	for (i = 0; i < 26; i++)
	{
		sprintf(buf, "%c", 'A' + i);
		XmLGridSetStringsPos(_grid, XmHEADING, 0, XmCONTENT, i, buf);
	}
	for (i = 0; i < 100; i++)
	{
		sprintf(buf, "%d", i + 1);
		XmLGridSetStringsPos(_grid, XmCONTENT, i, XmHEADING, 0, buf);
	}
	XtVaSetValues(_grid,
		XmNcolumnType, XmHEADING,
		XmNcolumn, 0,
	    XmNcellAlignment, XmALIGNMENT_CENTER,
		NULL);
	XtVaSetValues(_grid,
		XmNrowType, XmHEADING,
		XmNrow, 0,
	    XmNcellAlignment, XmALIGNMENT_CENTER,
		NULL);
}

/*
   Progress Tab Form
*/

#define progress_width 15
#define progress_height 15
static unsigned char progress_bits[] = {
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0x7f,
	0x6b, 0x40, 0x55, 0x40, 0xeb, 0x41, 0x55, 0x40, 0xff, 0x7f, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

static void ProgressOptCB();
static void ProgressSweepCB();
Widget _progress;

void AddProgressTabForm(folder)
Widget folder;
{
	Widget form, oForm, w;
	XmString str;
	static char *opts[] =
	{
		"Color:", "CC*Classic", "CS:Stoplight", 0,
		"Style:", "SB*Bar", "SO:Boxes", 0,
		"Percentage:", "PO*On", "PF:Off", 0,
		"Show Time:", "TO:On", "TF*Off", 0,
		0
	};
	static char *desc =
	"The Progress widget can be used to chart the completion of \
a task.  It displays a progress bar and can display text \
with percentage completion and elapsed and estimated time \
to completion.\n\n\
\
Resources exist to set the style of the meter bar and the \
values used to determine how complete the task currently \
is.  The bar meter style allows showing of estimated time to \
completion and the percentage indicator. Estimated and \
elapsed times are calculated by the \
Progress widget itself using the time the progress was set to \
0 (the start time), the current percentage complete and the \
current time.\n\n\
\
The Progress widget contains all the functionality of the \
Primitive widget it inherits from, allowing the foreground and \
background colors to be set as well as the size of the shadow \
to display around the progress bar.";

	str = XmStringCreateSimple("Progress Widget");
	form = XmLFolderAddBitmapTabForm(folder, str, (char *)progress_bits,
	    progress_width, progress_height);
	XmStringFree(str);

	oForm = CreateOptionForm(form, opts, ProgressOptCB);

	_progress = XtVaCreateManagedWidget("progress",
	    xmlProgressWidgetClass, form,
	    XmNleftAttachment, XmATTACH_FORM,
	    XmNleftOffset, 5,
	    XmNrightAttachment, XmATTACH_WIDGET,
	    XmNrightWidget, oForm,
	    XmNrightOffset, 5,
		XmNshadowThickness, 2,
	    XmNtopAttachment, XmATTACH_FORM,
	    XmNtopOffset, 5,
		XmNheight, 25,
	    XmNwidth, 342,
	    XtVaTypedArg, XmNforeground,
	    XmRString, "blue", 5,
	    XtVaTypedArg, XmNbackground,
	    XmRString, "white", 6,
	    NULL);

	w = XtVaCreateManagedWidget("Quick Sweep",
	    xmPushButtonWidgetClass, form,
	    XmNleftAttachment, XmATTACH_FORM,
	    XmNleftOffset, 5,
	    XmNbottomAttachment, XmATTACH_POSITION,
	    XmNbottomPosition, 45,
	    NULL);
	XtAddCallback(w, XmNactivateCallback, ProgressSweepCB, (XtPointer)1);
	w = XtVaCreateManagedWidget("Slow Sweep",
	    xmPushButtonWidgetClass, form,
	    XmNleftAttachment, XmATTACH_WIDGET,
	    XmNleftWidget, w,
	    XmNleftOffset, 5,
	    XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
	    XmNbottomWidget, w,
	    NULL);
	XtAddCallback(w, XmNactivateCallback, ProgressSweepCB, (XtPointer)0);

	CreateDescText(form, w, desc);
}

static void ProgressOptCB(w, clientData, callData)
Widget w;
XtPointer clientData;
XtPointer callData;
{
	char *opt;

	if (!ToggleButtonSet(w, clientData, callData))
		return;
	opt = (char *)clientData;

	/* Percentage */
	if (opt[0] == 'P' && opt[1] == 'O')
		XtVaSetValues(_progress,
		    XmNshowPercentage, True,
		    NULL);
	if (opt[0] == 'P' && opt[1] == 'F')
		XtVaSetValues(_progress,
		    XmNshowPercentage, False,
		    NULL);

	/* Style */
	if (opt[0] == 'S' && opt[1] == 'B')
		XtVaSetValues(_progress,
		    XmNmeterStyle, XmMETER_BAR,
			XmNshadowThickness, 2,
		    NULL);
	if (opt[0] == 'S' && opt[1] == 'O')
		XtVaSetValues(_progress,
		    XmNmeterStyle, XmMETER_BOXES,
			XmNnumBoxes, 20,
			XmNshadowThickness, 1,
		    NULL);

	/* Show Time */
	if (opt[0] == 'T' && opt[1] == 'O')
		XtVaSetValues(_progress,
		    XmNshowTime, True,
		    NULL);
	if (opt[0] == 'T' && opt[1] == 'F')
		XtVaSetValues(_progress,
		    XmNshowTime, False,
		    NULL);

	/* Color */
	if (opt[0] == 'C' && opt[1] == 'C')
		XtVaSetValues(_progress,
		    XtVaTypedArg, XmNforeground,
		    XmRString, "blue", 5,
		    XtVaTypedArg, XmNbackground,
		    XmRString, "white", 6,
		    NULL);
	if (opt[0] == 'C' && opt[1] == 'S')
		XtVaSetValues(_progress,
		    XtVaTypedArg, XmNforeground,
		    XmRString, "green", 6,
		    XtVaTypedArg, XmNbackground,
		    XmRString, "red", 4,
		    NULL);
}

static void ProgressSweepCB(w, clientData, callData)
Widget w;
XtPointer clientData;
XtPointer callData;
{
	int i, quickSweep;

	quickSweep = (int)clientData;
	if (quickSweep)
		for (i = 0; i <= 100; i += 4)
			XtVaSetValues(_progress, XmNvalue, i, NULL);
	else
	{
		for (i = 0; i <= 100; i += 23)
		{
			XtVaSetValues(_progress, XmNvalue, i, NULL);
			sleep(1);
		}
		XtVaSetValues(_progress, XmNvalue, 100, NULL);
	}
}
