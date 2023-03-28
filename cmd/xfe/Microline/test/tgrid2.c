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

/* Grid selection tests */

#include <Xm/Xm.h>
#include <XmL/Grid.h>
#include <Xm/Form.h>
#include <Xm/PushB.h>

void cb();
void printSelected();

main(argc, argv)
int argc;
char *argv[];
{
	XtAppContext app;
	Widget shell, form, button, grid;
	int i, n;
	char buf[30];
	Widget left;
	unsigned char sp;

	shell =  XtAppInitialize(&app, "TGrid2", NULL, 0,
	    &argc, argv, NULL, NULL, 0);

	form = XtVaCreateManagedWidget("form",
	    xmFormWidgetClass, shell,
	    NULL);

	button = XtVaCreateManagedWidget("Show Selected",
	    xmPushButtonWidgetClass, form,
	    XmNtopAttachment, XmATTACH_FORM,
	    XmNleftAttachment, XmATTACH_FORM,
	    NULL);

	for (n = 0; n < 5; n++)
	{
		if (!n)
			sp = XmSELECT_NONE;
		else if (n == 1)
			sp = XmSELECT_SINGLE_ROW;
		else if (n == 2)
			sp = XmSELECT_BROWSE_ROW;
		else if (n == 3)
			sp = XmSELECT_MULTIPLE_ROW;
		else if (n == 4)
			sp = XmSELECT_CELL;
		sprintf(buf, "grid%d", n);
		grid = XtVaCreateManagedWidget(buf,
		    xmlGridWidgetClass, form,
			XmNallowDrop, True,
			XmNallowDragSelected, True,
		    XmNheight, 300, 
		    XmNhorizontalSizePolicy, XmVARIABLE,
		    XmNtopAttachment, XmATTACH_WIDGET,
		    XmNtopWidget, button,
		    XmNbottomAttachment, XmATTACH_FORM,
		    XmNshadowThickness, 1,
		    XmNselectionPolicy, sp,
		    NULL);
		if (!n)
			XtVaSetValues(grid,
				XmNleftAttachment, XmATTACH_FORM,
				NULL);
		else
			XtVaSetValues(grid,
		    	XmNleftAttachment, XmATTACH_WIDGET,
		    	XmNleftWidget, left,
		    	XmNleftOffset, 10,
				NULL);
		XtAddCallback(grid, XmNactivateCallback, cb, NULL);
		XtAddCallback(grid, XmNcellDropCallback, cb, NULL);
		XtAddCallback(grid, XmNcellFocusCallback, cb, NULL);
		XtAddCallback(grid, XmNcellPasteCallback, cb, NULL);
		XtAddCallback(grid, XmNdeselectCallback, cb, NULL);
		XtAddCallback(grid, XmNeditCallback, cb, NULL);
		XtAddCallback(grid, XmNresizeCallback, cb, NULL);
		XtAddCallback(grid, XmNscrollCallback, cb, NULL);
		XtAddCallback(grid, XmNselectCallback, cb, NULL);
		left = grid;

		XtAddCallback(button, XmNactivateCallback, printSelected,
			(XtPointer)grid);

		XtVaSetValues(grid,
		    XmNcellDefaults, True,
		    XmNcellTopBorderType, XmBORDER_NONE,
		    XmNcellBottomBorderType, XmBORDER_NONE,
		    XmNcellLeftBorderType, XmBORDER_NONE,
		    XmNcellRightBorderType, XmBORDER_NONE,
		    NULL);
		XmLGridAddColumns(grid, XmHEADING, -1, 1);
		XmLGridAddColumns(grid, XmCONTENT, -1, 5);
		XmLGridAddColumns(grid, XmFOOTER, -1, 1);
		XtVaSetValues(grid,
			XmNcolumnType, XmALL_TYPES,
			XmNcolumnWidth, 2,
			NULL);
		XtVaSetValues(grid,
			XmNcolumn, 0,
			XmNcolumnWidth, 6,
			NULL);
		XmLGridAddRows(grid, XmHEADING, -1, 1);
		XmLGridAddRows(grid, XmCONTENT, -1, 40);
		XmLGridAddRows(grid, XmFOOTER, -1, 1);


		for (i = 0; i < 40; i++)
		{
			sprintf(buf, "ROW %d", i);
			XmLGridSetStringsPos(grid, XmCONTENT, i, XmCONTENT, 0, buf);
		}

		XtVaSetValues(grid,
		    XmNcellType, XmTEXT_CELL,
		    NULL);
	}

	XtRealizeWidget(shell);
	XtAppMainLoop(app);
}

void cb(w, clientData, callData)
Widget w;
XtPointer clientData, callData;
{
	XmLGridCallbackStruct *cbs;

	cbs = (XmLGridCallbackStruct *)callData;
	printf ("%s ", XtName(w));
	switch (cbs->reason)
	{
	case XmCR_ACTIVATE:
		printf ("XmCR_ACTIVATE");
		break;
	case XmCR_CELL_DROP:
		printf ("XmCR_CELL_DROP");
		break;
	case XmCR_CELL_FOCUS_IN:
		printf ("XmCR_CELL_FOCUS_IN");
		break;
	case XmCR_CELL_FOCUS_OUT:
		printf ("XmCR_CELL_FOCUS_OUT");
		break;
	case XmCR_CELL_PASTE:
		printf ("XmCR_CELL_PASTE");
		break;
	case XmCR_DESELECT_CELL:
		printf ("XmCR_DESELECT_CELL");
		break;
	case XmCR_DESELECT_ROW:
		printf ("XmCR_DESELECT_ROW");
		break;
	case XmCR_DESELECT_COLUMN:
		printf ("XmCR_DESELECT_COLUMN");
		break;
	case XmCR_EDIT_BEGIN:
		printf ("XmCR_EDIT_BEGIN");
		break;
	case XmCR_EDIT_CANCEL:
		printf ("XmCR_EDIT_CANCE");
		break;
	case XmCR_EDIT_COMPLETE:
		printf ("XmCR_EDIT_COMPLETE");
		break;
	case XmCR_EDIT_INSERT:
		printf ("XmCR_EDIT_INSERT");
		break;
	case XmCR_RESIZE_ROW:
		printf ("XmCR_RESIZE_ROW");
		break;
	case XmCR_RESIZE_COLUMN:
		printf ("XmCR_RESIZE_COLUMN");
		break;
	case XmCR_SELECT_CELL:
		printf ("XmCR_SELECT_CELL");
		break;
	case XmCR_SELECT_ROW:
		printf ("XmCR_SELECT_ROW");
		break;
	case XmCR_SELECT_COLUMN:
		printf ("XmCR_SELECT_COLUMN");
		break;
	case XmCR_SCROLL_ROW:
		printf ("XmCR_SCROLL_ROW");
		break;
	case XmCR_SCROLL_COLUMN:
		printf ("XmCR_SCROLL_COLUMN");
		break;
	}
	printf (" row [%d/%d]", cbs->rowType, cbs->row);
	printf (" col [%d/%d]\n", cbs->columnType, cbs->column);
}

void printSelected(w, clientData, callData)
Widget w;
XtPointer clientData, callData;
{
	Widget grid;
	int i, count, *rowPos, *colPos;

	grid = (Widget)clientData;
	printf ("--- Current Selections for %s---\n", XtName(grid));
	count = XmLGridGetSelectedRowCount(grid);
	if (count)
	{
		rowPos = (int *)malloc(sizeof(int) * count);
		XmLGridGetSelectedRows(grid, rowPos, count);
		printf ("Selected Rows: ");
		for (i = 0; i < count; i++)
			printf ("%d ", rowPos[i]);
		printf ("\n");
		free((char *)rowPos);
	}
	count = XmLGridGetSelectedColumnCount(grid);
	if (count)
	{
		colPos = (int *)malloc(sizeof(int) * count);
		XmLGridGetSelectedColumns(grid, colPos, count);
		printf ("Selected Columns: ");
		for (i = 0; i < count; i++)
			printf ("%d ", colPos[i]);
		printf ("\n");
		free((char *)colPos);
	}

	count = XmLGridGetSelectedCellCount(grid);
	if (count)
	{
		colPos = (int *)malloc(sizeof(int) * count);
		rowPos = (int *)malloc(sizeof(int) * count);
		XmLGridGetSelectedCells(grid, rowPos, colPos, count);
		printf ("Selected Cells: ");
		for (i = 0; i < count; i++)
			printf ("[%d %d] ", rowPos[i], colPos[i]);
		printf ("\n");
		free((char *)colPos);
		free((char *)rowPos);
	}
}
