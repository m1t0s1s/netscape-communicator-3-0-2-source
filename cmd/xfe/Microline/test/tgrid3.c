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

/* Grid General Resource/Function Test */

#include <Xm/Xm.h>
#include <Xm/Form.h>
#include <XmL/Grid.h>

void gridSelect();

Widget grid, text;

main(argc, argv)
int argc;
char *argv[];
{
	Widget shell;
	XtAppContext app;
	int hc, c, fc, hr, r, fr, hidr, hidc;
	int srow[10], scol[10];
	XmLCGridColumn *colp;
	XmLCGridRow *rowp;
	Boolean focusIn;
	XRectangle rect;
	unsigned char rt, ct;

	shell = XtAppInitialize(&app, "TGrid3", NULL, 0,
	    &argc, argv, NULL, NULL, 0);
	XtVaSetValues(shell,
		XmNwidth, 500,
		XmNheight, 250,
		NULL);

	grid = XtVaCreateManagedWidget("grid",
	    xmlGridWidgetClass, shell,
	    XmNselectionPolicy, XmSELECT_SINGLE_ROW,
	    NULL);

	XtRealizeWidget(shell);

	XtVaSetValues(grid,
		XmNlayoutFrozen, True,
		NULL);
	XmLGridAddColumns(grid, XmHEADING, -1, 4);
	XmLGridAddColumns(grid, XmCONTENT, -1, 10);
	XmLGridAddColumns(grid, XmFOOTER, -1, 2);
	XmLGridAddRows(grid, XmHEADING, -1, 4);
	XmLGridAddRows(grid, XmCONTENT, -1, 10);
	XmLGridAddRows(grid, XmFOOTER, -1, 2);
	XtVaSetValues(grid,
		XmNlayoutFrozen, False,
		NULL);

	XtVaSetValues(grid,
		XmNrowType, XmHEADING,
		XmNrowRangeStart, 1,
		XmNrowRangeEnd, 3,
		XmNrowStep, 2,
		XmNrowHeight, 0,
		NULL);

	XtVaSetValues(grid,
		XmNcolumnType, XmHEADING,
		XmNcolumnRangeStart, 1,
		XmNcolumnRangeEnd, 3,
		XmNcolumnStep, 2,
		XmNcolumnWidth, 0,
		NULL);

	XtVaSetValues(grid,
		XmNallowColumnResize, True,
		XmNallowRowResize, True,
		XtVaTypedArg, XmNblankBackground, XmRString, "green", 6,
		XtVaTypedArg, XmNfontList, XmRString, "5x7", 6,
		XmNbottomFixedCount, 3,
		XmNbottomFixedMargin, 5,
		XmNleftFixedCount, 4,
		XmNleftFixedMargin, 5, 
		XmNrightFixedCount, 3,
		XmNrightFixedMargin, 5,
		XmNtopFixedCount, 5,
		XmNtopFixedMargin, 5,
		XmNscrollBarMargin, 5, 
		XmNscrollColumn, 4, 
		XmNscrollRow, 4, 
		XtVaTypedArg, XmNselectBackground, XmRString, "red", 4,
		XtVaTypedArg, XmNselectForeground, XmRString, "blue", 5,
		XmNshadowRegions, 7,
		XmNuseAverageFontWidth, False,
		NULL);

	XtVaGetValues(grid,
		XmNheadingColumnCount, &hc,
		XmNcolumns, &c,
		XmNfooterColumnCount, &fc,
		XmNheadingRowCount, &hr,
		XmNrows, &r,
		XmNfooterRowCount, &fr,
		XmNhiddenRows, &hidr,
		XmNhiddenColumns, &hidc,
		XmNtextWidget, &text,
		NULL);

	printf ("HEADING ROWS %d ROWS %d FOOTER ROWS %d\n", hr, r, fr);
	printf ("HEADING COLUMNS %d COLUMNS %d FOOTER COLUMNS %d\n", hc, c, fc);
	printf ("HIDDEN ROWS %d HIDDEN COLUMNS %d\n", hidr, hidc);

	XtVaSetValues(grid,
		XmNrowType, XmHEADING,
		XmNrow, 0,
		XmNrowHeight, 5,
		XmNrowSizePolicy, XmCONSTANT,
		NULL);

	XtVaSetValues(grid,
		XmNcolumnType, XmHEADING,
		XmNcolumn, 0,
		XmNcolumnWidth, 5,
		XmNcolumnSizePolicy, XmCONSTANT,
		NULL);

	XmLGridColumnIsVisible(grid, 2);
	XmLGridDeleteAllColumns(grid, XmHEADING);
	XmLGridDeleteAllRows(grid, XmFOOTER);
	XmLGridAddColumns(grid, XmHEADING, -1, 4);
	XmLGridAddRows(grid, XmFOOTER, -1, 4);
	XmLGridDeleteColumns(grid, XmHEADING, 2, 1);
	XmLGridDeleteRows(grid, XmFOOTER, 2, 1);
	XmLGridGetCell(grid, XmHEADING, 0, XmHEADING, 0);
	XmLGridGetFocus(grid, &r, &c, &focusIn);
	colp = XmLGridGetColumn(grid, XmHEADING, 0);
	rowp = XmLGridGetRow(grid, XmHEADING, 0);

	c = XmLGridGetSelectedCellCount(grid);
	XmLGridGetSelectedCells(grid, srow, scol, c);
	c = XmLGridGetSelectedColumnCount(grid);
	XmLGridGetSelectedColumns(grid, scol, c);
	XmLGridGetSelectedRow(grid);
	r = XmLGridGetSelectedRowCount(grid);
	XmLGridGetSelectedRows(grid, srow, r);
	XmLGridMoveColumns(grid, 1, 2, 2);
	XmLGridMoveRows(grid, 1, 2, 2);
	XmLGridRedrawAll(grid);
	XmLGridRedrawCell(grid, XmHEADING, 3, XmHEADING, 2);
	XmLGridRedrawColumn(grid, XmHEADING, 3);
	XmLGridRedrawRow(grid, XmHEADING, 3);
	scol[0] = 1;
	scol[1] = 0;
	XmLGridReorderColumns(grid, scol, 0, 2);
	srow[0] = 1;
	srow[1] = 0;
	XmLGridReorderRows(grid, srow, 0, 2);
	XmLGridRowColumnToXY(grid, XmHEADING, 3, XmHEADING, 1, &rect);
	XmLGridRowIsVisible(grid, 3);
	XmLGridSelectAllCells(grid);
	XmLGridSelectAllColumns(grid);
	XmLGridSelectAllRows(grid);
	XmLGridSelectCell(grid, 3, 4, True);
	XmLGridSelectColumn(grid, 3, True);
	XmLGridSelectRow(grid, 3, True);
	XmLGridDeselectCell(grid, 3, 2, True);
	XmLGridDeselectColumn(grid, 3, True);
	XmLGridDeselectRow(grid, 1, True);
	XmLGridDeselectAllCells(grid);
	XmLGridDeselectAllColumns(grid);
	XmLGridDeselectAllRows(grid);
	XmLGridSetStrings(grid, "STR1\nSTR2|STR3");
	XmLGridSetStringsPos(grid, XmCONTENT, 0, XmCONTENT, 0, "Cell 0|Cell 1");
	XmLGridXYToRowColumn(grid, 20, 20, &rt, &r, &ct, &c);
	XtVaSetValues(grid,
		XmNcolumn, 1,
		XmNcellType, XmTEXT_CELL,
		NULL);
	XtVaSetValues(grid,
		XmNrow, 1,
		XmNcolumn, 1,
		XtVaTypedArg, XmNcellBackground, XmRString, "yellow", 7,
		XtVaTypedArg, XmNcellForeground, XmRString, "pink", 5,
		XmNcellBottomBorderPixel, XmBORDER_NONE,
		XmNcellColumnSpan, 2,
		XmNcellRowSpan, 2,
		XmNcellType, XmTEXT_CELL,
		XmNcellAlignment, XmALIGNMENT_TOP_RIGHT,
		NULL);
	XtAddCallback(grid, XmNselectCallback, gridSelect, NULL);

	XtAppMainLoop(app);
}

void gridSelect(w, clientData, callData)
Widget w;
XtPointer clientData;
XtPointer callData;
{
	XmLGridEditBegin(grid, False, 1, 1);
	XmTextSetString(text, "Testing");
	XmLGridEditComplete(grid);
	XmLGridEditCancel(grid);
	XmLGridSetFocus(grid, 1, 6);
}
