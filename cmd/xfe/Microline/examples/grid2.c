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
#include <Xm/Text.h>
#include <Xm/Label.h>
#include <Xm/PushB.h>
#include <XmL/Grid.h>

Widget label, grid, text;

void cellFocus();
void textFocus();
void copy();
void paste();

main(argc, argv)
int argc;
char *argv[];
{
	XtAppContext app;
	Widget shell, form, copyB, pasteB, gridText;
	XmTextSource source;
	char buf[4];
	int i;

	shell =  XtAppInitialize(&app, "Grid2", NULL, 0,
	    &argc, argv, NULL, NULL, 0);

	form = XtVaCreateManagedWidget("form",
	    xmFormWidgetClass, shell,
	    NULL);

	label = XtVaCreateManagedWidget("label",
	    xmLabelWidgetClass, form,
	    XmNtopAttachment, XmATTACH_FORM,
	    XmNmarginHeight, 4,
	    XmNleftAttachment, XmATTACH_FORM,
	    NULL);

	pasteB = XtVaCreateManagedWidget("Paste To Focus",
	    xmPushButtonWidgetClass, form,
	    XmNrightAttachment, XmATTACH_FORM,
	    XmNmarginHeight, 0,
	    NULL);
	XtAddCallback(pasteB, XmNactivateCallback, paste, NULL);

	copyB = XtVaCreateManagedWidget("Copy Selected",
	    xmPushButtonWidgetClass, form,
	    XmNrightAttachment, XmATTACH_WIDGET,
	    XmNrightWidget, pasteB,
	    XmNmarginHeight, 0,
	    NULL);
	XtAddCallback(copyB, XmNactivateCallback, copy, NULL);

	text = XtVaCreateManagedWidget("text",
	    xmTextWidgetClass, form,
	    XmNtopAttachment, XmATTACH_FORM,
	    XmNleftAttachment, XmATTACH_WIDGET,
	    XmNleftWidget, label,
	    XmNrightAttachment, XmATTACH_WIDGET,
	    XmNrightWidget, copyB,
	    XmNcolumns, 40,
	    XmNmarginHeight, 0,
	    NULL);
#ifndef MOTIF11
	XmDropSiteUnregister(text);
#endif

	grid = XtVaCreateManagedWidget("grid",
	    xmlGridWidgetClass, form,
	    XmNheight, 300, 
	    XmNwidth, 500,
	    XmNallowDragSelected, True,
	    XmNallowDrop, True,
	    XmNallowRowResize, True,
	    XmNallowColumnResize, True,
	    XmNtopAttachment, XmATTACH_WIDGET,
	    XmNtopWidget, text,
	    XmNbottomAttachment, XmATTACH_FORM,
	    XmNleftAttachment, XmATTACH_FORM,
	    XmNrightAttachment, XmATTACH_FORM,
	    XmNshadowThickness, 1,
	    XmNselectionPolicy, XmSELECT_CELL,
	    NULL);
	XtAddCallback(grid, XmNcellFocusCallback, cellFocus, NULL);

	XtVaGetValues(grid,
	    XmNtextWidget, &gridText,
	    NULL);
	source = XmTextGetSource(gridText);
	XmTextSetSource(text, source, 0, 0);

	XtVaSetValues(grid,
	    XmNcellDefaults, True,
	    XmNcellType, XmTEXT_CELL,
	    NULL);
	XmLGridAddColumns(grid, XmHEADING, -1, 1);
	XmLGridAddColumns(grid, XmCONTENT, -1, 26);
	XmLGridAddRows(grid, XmHEADING, -1, 1);
	XmLGridAddRows(grid, XmCONTENT, -1, 100);

	for (i = 0; i < 26; i++)
	{
		sprintf(buf, "%c", 'A' + i);
		XmLGridSetStringsPos(grid, XmHEADING, 0, XmCONTENT, i, buf);
	}
	for (i = 0; i < 100; i++)
	{
		sprintf(buf, "%d", i + 1);
		XmLGridSetStringsPos(grid, XmCONTENT, i, XmHEADING, 0, buf);
	}

	XtRealizeWidget(shell);
	XtAppMainLoop(app);
}

void cellFocus(w, clientData, callData)
Widget w;
XtPointer clientData;
XtPointer callData;
{
	XmLGridCallbackStruct *cbs;
	XmLCGridRow *row;
	XmLCGridColumn *column;
	Widget sharedText;
	XmString str;
	char buf[10], *c;

	cbs = (XmLGridCallbackStruct *)callData;
	if (cbs->reason != XmCR_CELL_FOCUS_IN)
		return;

	/* update label to reflect new position */
	sprintf(buf, "(%c %d)", 'A' + cbs->column, cbs->row + 1);
	str = XmStringCreateSimple(buf);
	XtVaSetValues(label,
	    XmNlabelString, str,
	    NULL);
	XmStringFree(str);

	/* set text to the string contained in the new focus cell */
	row = XmLGridGetRow(w, cbs->rowType, cbs->row);
	column = XmLGridGetColumn(w, cbs->columnType, cbs->column);
	XtVaGetValues(w,
	    XmNrowPtr, row,
	    XmNcolumnPtr, column,
	    XmNcellString, &str,
	    NULL);
	c = 0;
	if (str)
		XmStringGetLtoR(str, XmSTRING_DEFAULT_CHARSET, &c);
	if (c)
	{
		XmTextSetString(text, c);
		XtFree(c);
		XmTextSetSelection(text, 0, XmTextGetLastPosition(text),
		    CurrentTime);
	}
	else
		XmTextSetString(text, "");
	if (str)
		XmStringFree(str);
}

void copy(w, clientData, callData)
Widget w;
XtPointer clientData;
XtPointer callData;
{
	XmLGridCopySelected(grid, CurrentTime);
}

void paste(w, clientData, callData)
Widget w;
XtPointer clientData;
XtPointer callData;
{
	/* Pastes starting at the current focus cell            */
	/* An alternative method of pasting is to make the user */
	/* select an area to paste into.  In this case we could */
	/* get the selected area and use XmLGridPastePos()      */
	/* to paste into that area                              */
	XmLGridPaste(grid);
}
