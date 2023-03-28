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

/* Random Grid testing */

#include <Xm/Xm.h>
#include <XmL/Grid.h>
#include <Xm/PushB.h>
#include <Xm/Form.h>
#include <stdio.h>

XtAppContext app;
static int cbRowCount, cbColCount, cbCellCount;
static int ramCount;

void addCB(w, clientData, callData)
Widget w;
XtPointer clientData, callData;
	{
	XmLGridCallbackStruct *cbs;

	cbs = (XmLGridCallbackStruct *)callData;
	if (cbs->reason == XmCR_ADD_ROW)
		cbRowCount++;
	if (cbs->reason == XmCR_ADD_COLUMN)
		cbColCount++;
	if (cbs->reason == XmCR_ADD_CELL)
		cbCellCount++;
	}

void deleteCB(w, clientData, callData)
Widget w;
XtPointer clientData, callData;
	{
	XmLGridCallbackStruct *cbs;

	cbs = (XmLGridCallbackStruct *)callData;
	if (cbs->reason == XmCR_DELETE_ROW)
		cbRowCount--;
	else if (cbs->reason == XmCR_DELETE_COLUMN)
		cbColCount--;
	else if (cbs->reason == XmCR_DELETE_CELL)
		cbCellCount--;
	}

void cb(w, clientData, callData)
Widget w;
XtPointer clientData, callData;
	{
	Widget grid;
	static int rn = 3;

	grid = (Widget)clientData;
	printf ("row %d to height 0\n", rn);
	XtVaSetValues(grid,
		XmNrow, rn++,
		XmNrowHeight, 0,
		NULL);
	}

void ram(p, id)
XtPointer p;
XtIntervalId *id;
	{
	int op, pos, count, rowCount, colCount;
	Widget grid;
	char col[9];

	grid = (Widget)p;
	op = rand() % 20;
	printf ("%d.", op);
	fflush(stdout);
	XtVaGetValues(grid,
		XmNcolumns, &colCount,
		XmNrows, &rowCount,
		NULL);
	if (colCount != cbColCount || rowCount != cbRowCount ||
		cbCellCount != colCount * rowCount)
			printf ("COUNT ERROR\n");
	switch (op)
		{
		case 0:
			if (colCount < 100)
				XmLGridAddColumns(grid, XmCONTENT, rand() % 100, rand() % 20);
			break;
		case 1:
			if (colCount - count > 0)
				{
				count = rand() % colCount;
				pos = rand() % (colCount - count);
				XmLGridDeleteColumns(grid, XmCONTENT, pos, count);
				}
			break;
		case 2:
			if (rowCount < 100)
				XmLGridAddRows(grid, XmCONTENT, rand() % 100, rand() % 20);
			break;
		case 3:
			if (rowCount - count > 0)
				{
				count = rand() % rowCount;
				pos = rand() % (rowCount - count);
				XmLGridDeleteRows(grid, XmCONTENT, pos, count);
				}
			break;
		case 4:
			if (rowCount - count > 0)
				{
				count = rand() % rowCount;
				pos = rand() % (rowCount - count);
				XtVaSetValues(grid,
					XmNrowRangeStart, pos,
					XmNrowRangeEnd, pos + count,
					XmNrowHeight, rand() % 3,
					NULL);
				}
			break;
		case 5:
			if (colCount - count > 0)
				{
				count = rand() % colCount;
				pos = rand() % (colCount - count);
				XtVaSetValues(grid,
					XmNcolumnRangeStart, count,
					XmNcolumnRangeEnd, count + pos,
					XmNcolumnWidth, rand() % 3,
					NULL);
				}
			break;
		case 6:
			if (!(rand() % 5))
				{
				XmLGridDeleteAllRows(grid, XmCONTENT);
				XmLGridDeleteAllColumns(grid, XmCONTENT);
				}
			break;
		case 7:
			XtVaSetValues(grid,
				XmNtopFixedCount, rand() % 3,
				XmNbottomFixedCount, rand() % 3,
				XmNleftFixedCount, rand() % 3,
				XmNrightFixedCount, rand() % 3,
				NULL);
			break;
		case 8:
			if (rowCount > 0 && colCount > 0)
				XtVaSetValues(grid,
					XmNscrollRow, rand() % rowCount,
					XmNscrollColumn, rand() % colCount,
					NULL);
			break;
		case 9:
		case 10:
		case 11:
		case 12:
		case 13:
		case 14:
		case 15:
		case 16:
		case 17:
		case 18:
		case 19:
			sprintf(col, "#%2d%2d%2d", rand() % 90 + 10,
				rand() % 90 + 10, rand() % 90 + 10);
			if (rowCount > 0 && colCount > 0)
				{
				XtVaSetValues(grid,
					XmNrow, rand() % rowCount,
					XmNcolumn, rand() % colCount,
					XtVaTypedArg, XmNcellBackground,
					XmRString, col, 8,
					NULL);
				XtVaSetValues(grid,
					XmNrow, rand() % rowCount,
					XmNcolumn, rand() % colCount,
					XtVaTypedArg, XmNcellForeground,
					XmRString, col, 8,
					NULL);
				}
			break;
		}
	XSync(XtDisplay(grid), False);
	if (++ramCount < 500)
		XtAppAddTimeOut(app, 100L, ram, (XtPointer)grid);
	else
		{
		printf ("DONE\n");
		exit(0);
		}
	}

main(argc, argv)
int argc;
char *argv[];
	{
	Widget shell, form, grid, b;
	char s[30];
	int r, c;
	XmString str;
	static String resources[] =
		{
		"*background: #D0D0D0",
		"*foreground: #000000",
		NULL
		};

	shell =  XtAppInitialize(&app, "TGrid1", NULL, 0,
#ifdef X11R5
		(int *)&argc, (String *)argv, resources, NULL, 0);	
#else
		(Cardinal *)&argc, (String *)argv, resources, NULL, 0);	
#endif

	form = XtVaCreateWidget("form",
		xmFormWidgetClass, shell,
		XmNwidth, 300,
		XmNheight, 300,
		NULL);

	grid = XtVaCreateManagedWidget("grid",
		xmlGridWidgetClass, form,
		XmNheight, 300,
		XmNwidth, 300,
		XmNtopAttachment, XmATTACH_FORM,
		XmNleftAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_FORM,
		XmNbottomAttachment, XmATTACH_FORM,
		XmNbottomOffset, 20,
		NULL);
	XtRemoveAllCallbacks(grid, XmNaddCallback);
	XtAddCallback(grid, XmNaddCallback, addCB, 0);
	XtAddCallback(grid, XmNdeleteCallback, deleteCB, 0);
	XmLGridAddColumns(grid, XmCONTENT, 0, 10);
	XmLGridAddRows(grid, XmCONTENT, 0, 30);

	for (c = 0; c < 10; c++)
		for (r = 0; r < 30; r++)
			{
			sprintf(s,"%d / %d", c, r);
			str = XmStringCreateSimple(s);
			XtVaSetValues(grid,
				XmNrow, r,
				XmNcolumn, c,
				XmNcellString, str,
				NULL);
			XmStringFree(str);
			}

	b = XtVaCreateManagedWidget("hide row",
		xmPushButtonWidgetClass, form,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, grid,
		NULL);
	XtAddCallback(b, XmNactivateCallback, cb, (XtPointer)grid);

	XtManageChild(form);
	XtRealizeWidget(shell);

	srand(10);
	XtAppAddTimeOut(app, 100L, ram, (XtPointer)grid);

	XtAppMainLoop(app);
	}
