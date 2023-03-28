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
#include <Xm/Text.h>
#include <XmL/Grid.h>

/* DATABASE PROTOTYPE FUNCTIONS */

int dbTableNumRows = 14;
int dbTableNumColumns = 5;

typedef enum {
	ID, Desc, Price, Qty, UnitPrice, Buyer }
DbTableColumnID;

typedef struct
	{
	DbTableColumnID id;
	char label[15];
	int width;
	unsigned char cellAlignment;
	Boolean cellType;
} DbTableColumn;

DbTableColumn dbTableColumns[] =
{
	{ Desc,      "Description", 16, XmALIGNMENT_LEFT,  XmTEXT_CELL  },
	{ Price,     "Price",        9, XmALIGNMENT_RIGHT, XmTEXT_CELL  },
	{ Qty,       "Qty",          5, XmALIGNMENT_RIGHT, XmTEXT_CELL  },
	{ UnitPrice, "Unit Prc",     9, XmALIGNMENT_RIGHT, XmLABEL_CELL },
	{ Buyer,     "Buyer",       15, XmALIGNMENT_LEFT,  XmTEXT_CELL  },
};

typedef struct
	{
	char key[10];
	char desc[20];
	float price;
	int qty;
	char buyer[20];
} DbTableRow;

DbTableRow dbTableRows[] =
{
	{ "key01", "Staples",        1.32, 100, "Tim Pick"   },
	{ "key02", "Notebooks",      1.11,   4, "Mary Miner" },
	{ "key03", "3-Ring Binders", 2.59,   2, "Mary Miner" },
	{ "key04", "Pads",           1.23,   3, "Tim Pick"   },
	{ "key05", "Scissors",       4.41,   1, "Mary Miner" },
	{ "key06", "Pens",            .29,   4, "Mary Miner" },
	{ "key07", "Pencils",         .10,   5, "Tim Pick"   },
	{ "key08", "Markers",         .95,   3, "Mary Miner" },
	{ "key09", "Fax Paper",      3.89, 100, "Bob Coal"   },
	{ "key10", "3.5\" Disks",   15.23,  30, "Tim Pick"   },
	{ "key11", "8mm Tape",      32.22,   2, "Bob Coal"   },
	{ "key12", "Toner",         35.69,   1, "Tim Pick"   },
	{ "key13", "Paper Cups",     4.25,   3, "Bob Coal"   },
	{ "key14", "Paper Clips",    2.09,   3, "Tim Pick"   },
};

DbTableRow *dbFindRow(rowKey)
char *rowKey;
{
	int i;

	for (i = 0; i < dbTableNumRows; i++)
		if (!strcmp(rowKey, dbTableRows[i].key))
			return &dbTableRows[i];
	return 0;
}

int dbCompareRowKeys(userData, l, r)
void *userData;
void *l;
void *r;
{
	DbTableRow *dbRow1, *dbRow2;
	float u1, u2;

	dbRow1 = dbFindRow(*(char **)l);
	dbRow2 = dbFindRow(*(char **)r);
	switch ((int)userData)
	{
	case Desc:
		return strcmp(dbRow1->desc, dbRow2->desc);
	case Price:
		u1 = dbRow1->price - dbRow2->price;
		if (u1 < 0)
			return -1;
		else if (u1 == 0)
			return 0;
		return 1;
	case Qty:
		return dbRow1->qty - dbRow2->qty;
	case UnitPrice:
		u1 = dbRow1->price / (float)dbRow1->qty;
		u2 = dbRow2->price / (float)dbRow2->qty;
		if (u1 < u2)
			return -1;
		else if (u1 == u2)
			return 0;
		else
			return 1;
	case Buyer:
		return strcmp(dbRow1->buyer, dbRow2->buyer);
	}
	return (int)(dbRow1 - dbRow2);
}

char **dbGetRowKeysSorted(sortColumnID)
int sortColumnID;
{
	char **keys;
	int i;

	keys = (char **)malloc(sizeof(char *) * dbTableNumRows);
	for (i = 0; i < dbTableNumRows; i++)
		keys[i] = dbTableRows[i].key;
	XmLSort(keys, dbTableNumRows, sizeof(char *),
	    dbCompareRowKeys, (void *)sortColumnID);
	return keys;
}

/* GRID FUNCTIONS */

void setRowKeysInGridSorted(grid, sortColumnID)
Widget grid;
int sortColumnID;
{
	char **keys;
	int i;

	keys = dbGetRowKeysSorted(sortColumnID);
	/* place a pointer to each row key in each rows userData */
	for (i = 0; i < dbTableNumRows; i++)
		XtVaSetValues(grid,
		    XmNrow, i,
		    XmNrowUserData, (XtPointer)keys[i],
		    NULL);
	free((char *)keys);
}

void cellSelect(w, clientData, callData)
Widget w;
XtPointer clientData;
XtPointer callData;
{
	XmLGridCallbackStruct *cbs;
	XmLCGridColumn *column;
	XtPointer columnUserData;

	cbs = (XmLGridCallbackStruct *)callData;

	if (cbs->rowType != XmHEADING)
		return;

	/* cancel any edits in progress */
	XmLGridEditCancel(w);

	column = XmLGridGetColumn(w, cbs->columnType, cbs->column);
	XtVaGetValues(w,
	    XmNcolumnPtr, column,
	    XmNcolumnUserData, &columnUserData,
	    NULL);
	setRowKeysInGridSorted(w, (DbTableColumnID)columnUserData);
	XmLGridRedrawAll(w);
}

void cellDraw(w, clientData, callData)
Widget w;
XtPointer clientData;
XtPointer callData;
{
	XmLGridCallbackStruct *cbs;
	XmLGridDrawStruct *ds;
	XmLCGridRow *row;
	XmLCGridColumn *column;
	XtPointer rowUserData, columnUserData;
	DbTableRow *dbRow;
	unsigned char alignment;
	Pixel fg;
	XmFontList fontList;
	XmString str;
	char buf[50];

	cbs = (XmLGridCallbackStruct *)callData;
	if (cbs->rowType != XmCONTENT)
		return;

	ds = cbs->drawInfo;

	/* retrieve userData from the cells row */
	row = XmLGridGetRow(w, cbs->rowType, cbs->row);
	XtVaGetValues(w,
	    XmNrowPtr, row,
	    XmNrowUserData, &rowUserData,
	    NULL);

	/* retrieve userData from cells column */
	column = XmLGridGetColumn(w, cbs->columnType, cbs->column);
	XtVaGetValues(w,
	    XmNcolumnPtr, column,
	    XmNcolumnUserData, &columnUserData,
	    NULL);

	/* retrieve the cells value from the database */
	dbRow = dbFindRow((char *)rowUserData);
	switch ((DbTableColumnID)columnUserData)
	{
	case Desc:
		sprintf(buf, "%s", dbRow->desc);
		break;
	case Price:
		sprintf(buf, "$%4.2f", dbRow->price);
		break;
	case Qty:
		sprintf(buf, "%d", dbRow->qty);
		break;
	case UnitPrice:
		sprintf(buf, "$%4.2f", dbRow->price / (float)dbRow->qty);
		break;
	case Buyer:
		sprintf(buf, "%s", dbRow->buyer);
		break;
	}

	/* retrieve drawing info from the cell */
	XtVaGetValues(w,
	    XmNrowPtr, row,
	    XmNcolumnPtr, column,
	    XmNcellForeground, &fg,
	    XmNcellAlignment, &alignment,
	    XmNcellFontList, &fontList,
	    NULL);

	/* draw the string */
	str = XmStringCreateSimple(buf);
	if (ds->isSelected == True)
		XSetForeground(XtDisplay(w), ds->gc, ds->selectForeground);
	else
		XSetForeground(XtDisplay(w), ds->gc, fg);
	XmLStringDraw(w, str, ds->stringDirection, fontList, alignment, ds->gc,
	    ds->cellRect, cbs->clipRect);
	XmStringFree(str);
}

void cellEdit(w, clientData, callData)
Widget w;
XtPointer clientData;
XtPointer callData;
{
	XmLGridCallbackStruct *cbs;
	XmLCGridRow *row;
	XmLCGridColumn *column;
	XtPointer rowUserData, columnUserData;
	DbTableRow *dbRow;
	Widget text;
	float f;
	int i;
	char *value;
	Boolean redrawRow;

	cbs = (XmLGridCallbackStruct *)callData;

	/* for a production version, this function should also
	   handle XmCR_EDIT_INSERT by retrieving the current value
	   from the database and performing an XmTextSetString on
	   the text widget in the grid with that value.  This allows
	   a user to hit insert or F2 to modify an existing cell value */

	if (cbs->reason != XmCR_EDIT_COMPLETE)
		return;

	/* get the value the user just typed in */
	XtVaGetValues(w,
	    XmNtextWidget, &text,
	    NULL);
	value = XmTextGetString(text);
	if (!value)
		return;

	/* retrieve userData from the cells row */
	row = XmLGridGetRow(w, cbs->rowType, cbs->row);
	XtVaGetValues(w,
	    XmNrowPtr, row,
	    XmNrowUserData, &rowUserData,
	    NULL);

	/* retrieve userData from cells column */
	column = XmLGridGetColumn(w, cbs->columnType, cbs->column);
	XtVaGetValues(w,
	    XmNcolumnPtr, column,
	    XmNcolumnUserData, &columnUserData,
	    NULL);

	/* set new value in the database */
	redrawRow = False;
	dbRow = dbFindRow((char *)rowUserData);
	switch ((DbTableColumnID)columnUserData)
	{
	case Desc:
		if (strlen(value) < 20)
			strcpy(dbRow->desc, value);
		break;
	case Price:
		if (sscanf(value, "%f", &f) == 1)
		{
			dbRow->price = f;
			redrawRow = True;
		}
		break;
	case Qty:
		if (sscanf(value, "%d", &i) == 1)
		{
			dbRow->qty = i;
			redrawRow = True;
		}
		break;
	case Buyer:
		if (strlen(value) < 20)
			strcpy(dbRow->buyer, value);
		break;
	}

	/* redraw the row if we need to redisplay unit price */
	if (redrawRow == True)
		XmLGridRedrawRow(w, cbs->rowType, cbs->row);

	/* set the cellString to NULL - its is set to the value the
	   user typed in the text widget at this point */
	XtVaSetValues(w,
	    XmNrow, cbs->row,
	    XmNcolumn, cbs->column,
	    XmNcellString, NULL,
	    NULL);

	XtFree(value);
}

main(argc, argv)
int argc;
char *argv[];
{
	XtAppContext app;
	Widget shell, grid;
	XmString str;
	int i;

	shell =  XtAppInitialize(&app, "Grid5", NULL, 0,
	    &argc, argv, NULL, NULL, 0);

	grid = XtVaCreateManagedWidget("grid",
	    xmlGridWidgetClass, shell,
	    XmNheight, 200,
	    XmNhorizontalSizePolicy, XmVARIABLE,
	    XmNselectionPolicy, XmSELECT_NONE,
	    NULL);
	XtAddCallback(grid, XmNcellDrawCallback, cellDraw, NULL);
	XtAddCallback(grid, XmNeditCallback, cellEdit, NULL);
	XtAddCallback(grid, XmNselectCallback, cellSelect, NULL);

	/* add heading row with yellow cell background */
	XmLGridAddColumns(grid, XmCONTENT, -1, dbTableNumColumns);
	XtVaSetValues(grid,
	    XmNcellDefaults, True,
	    XtVaTypedArg, XmNcellBackground, XmRString, "yellow", 7,
	    XtVaTypedArg, XmNcellForeground, XmRString, "black", 6,
	    NULL);
	XmLGridAddRows(grid, XmHEADING, -1, 1);

	/* add content rows with white cell background */
	XtVaSetValues(grid,
	    XmNcellDefaults, True,
	    XtVaTypedArg, XmNcellBackground, XmRString, "white", 6,
	    NULL);
	XmLGridAddRows(grid, XmCONTENT, -1, dbTableNumRows);

	for (i = 0; i < dbTableNumColumns; i++)
	{
		/* create the column heading label */
		str = XmStringCreateSimple(dbTableColumns[i].label);
		XtVaSetValues(grid,
		    XmNrowType, XmHEADING,
		    XmNrow, 0,
		    XmNcolumn, i,
		    XmNcellString, str,
		    NULL);
		XmStringFree(str);

		/* set the width and the id on the column */
		XtVaSetValues(grid,
		    XmNcolumn, i,
		    XmNcolumnUserData, (XtPointer)dbTableColumns[i].id,
		    XmNcolumnWidth, dbTableColumns[i].width,
		    NULL);

		/* set the type and alignment of the content cells in the column */
		XtVaSetValues(grid,
		    XmNcolumn, i,
		    XmNcellAlignment, dbTableColumns[i].cellAlignment,
		    XmNcellType, dbTableColumns[i].cellType,
		    NULL);
	}

	/* set the row keys in the rows */
	setRowKeysInGridSorted(grid, Desc);

	XtRealizeWidget(shell);
	XtAppMainLoop(app);
}
