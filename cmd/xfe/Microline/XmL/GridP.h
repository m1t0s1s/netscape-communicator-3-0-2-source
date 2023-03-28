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

#ifndef XmLGridPH
#define XmLGridPH

#include <Xm/XmP.h>
#include <stdlib.h>
#ifndef MOTIF11
#include <Xm/ManagerP.h>
#include <Xm/DrawP.h>
#include <Xm/DragC.h>
#include <Xm/DropTrans.h>
#endif

#include "Grid.h"

enum { DrawAll, DrawHScroll, DrawVScroll, DrawRow, DrawCol, DrawCell };
enum { SelectRow, SelectCol, SelectCell };
enum { CursorNormal, CursorHResize, CursorVResize };
enum { InNormal, InSelect, InResize, InMove };
enum { DragLeft = 1, DragRight = 2, DragUp = 4, DragDown = 8 };

typedef struct
	{
	int x, y, width, height;
	int row, col, nrow, ncol;
	} GridReg;

typedef struct
	{
	int row, col;
	} GridDropLoc;

#if defined(_STDC_) || defined(__cplusplus) || defined(c_plusplus)
typedef int (*XmLArrayCompareFunc)(void *, void **, void **);
#else
typedef int (*XmLArrayCompareFunc)();
#endif

typedef struct
	{
	int _pos;
	} XmLArrayItem;

typedef struct _XmLArrayRec
	{
	int _count, _size;
	void **_items;
	} XmLArray;

typedef struct
	{
	unsigned char alignment;
	Pixel background;
	Pixel bottomBorderPixel;
	char bottomBorderType;
	int columnSpan;
	short fontHeight;
	XmFontList fontList;
	short fontWidth;
	Pixel foreground;
	Pixel leftBorderPixel;
	char leftBorderType;
	int refCount;
	Pixel rightBorderPixel;
	char rightBorderType;
	int rowSpan;
	Pixel topBorderPixel;
	char topBorderType;
	unsigned char type;
	void *userData;
	} XmLGridCellRefValues;

typedef struct
	{
	Pixmap pixmap;
	Dimension width, height;
	} XmLGridCellPixmap;

#define XmLGridCellSelectedFlag     (1 << 0)
#define XmLGridCellValueSetFlag     (1 << 1)
#define XmLGridCellInRowSpanFlag    (1 << 2)
#define XmLGridCellInColumnSpanFlag (1 << 3)

struct _XmLGridCellRec
	{
	int _pos;		/* lined up with XmLArrayItem */
	XmLGridCellRefValues *_refValues;
	unsigned char _flags;
	void *_value;
	};

typedef struct
	{
	Dimension height;
	unsigned char sizePolicy;
	Boolean selected;
	XtPointer userData;
	} XmLGridRowValues;

struct _XmLGridRowRec
	{
	int _pos;		/* lined up with XmLArrayItem */
	XmLGridRowValues _values;
	Dimension _heightInPixels;
	unsigned int _heightInPixelsValid:1;
	Widget _grid;
	int _visPos;
	XmLArray *_cellArray;
	};

typedef struct
	{
	Dimension width;
	unsigned char sizePolicy;
	Boolean selected;
	XtPointer userData;
	} XmLGridColumnValues;

struct _XmLGridColumnRec
	{
	int _pos;		/* lined up with XmLArrayItem */
	Widget _grid;
	Dimension _widthInPixels;
	unsigned int _widthInPixelsValid:1;
	XmLGridColumnValues _values;
	int _visPos;
	};

typedef struct _XmLGridPart
	{
	/* resource values */
	int leftFixedCount, rightFixedCount;
	int headingRowCount, footerRowCount;
	int topFixedCount, bottomFixedCount;
	int headingColCount, footerColCount;
	Dimension leftFixedMargin, rightFixedMargin;
	Dimension topFixedMargin, bottomFixedMargin;
	Dimension scrollBarMargin;
	unsigned char selectionPolicy;
	Boolean layoutFrozen;
	char debugLevel;
	unsigned char vsPolicy, hsPolicy;
	unsigned char hsbDisplayPolicy, vsbDisplayPolicy;
	int rowCount, colCount;
	int hiddenRowCount, hiddenColCount;
	int shadowRegions;
	Widget hsb, vsb, text;
	XmFontList fontList;
	Pixel blankBg, selectBg, selectFg;
	Pixel defaultCellBg, defaultCellFg;
	XtTranslations editTrans, traverseTrans;
	Boolean allowRowResize, allowColResize;
	Boolean allowDrag, allowDrop;
	Boolean useAvgWidth;
	int scrollRow, scrollCol, cScrollRow, cScrollCol;
	XtCallbackList addCallback, deleteCallback;
	XtCallbackList cellDrawCallback, cellFocusCallback;
	XtCallbackList cellDropCallback, cellPasteCallback;
	XtCallbackList activateCallback, editCallback;
	XtCallbackList selectCallback, deselectCallback;
	XtCallbackList resizeCallback, scrollCallback;

	/* private variables */
	GC gc;
	Cursor hResizeCursor, vResizeCursor;
	XFontStruct *fallbackFont;
	char ignoreModifyVerify;
	char focusIn, inEdit, inMode;
	char cursorDefined, textHidden, resizeIsVert;
	char needsHorizLayout, needsVertLayout;
	char recalcHorizVisPos, recalcVertVisPos;
	char dragTimerSet;
	XtIntervalId dragTimerId;
	int resizeRow, resizeCol, resizeLineXY;
	char *extendSel;
	int extendRow, extendCol, extendToRow, extendToCol;
	int lastSelectRow, lastSelectCol;
	Time lastSelectTime;
	int focusRow, focusCol;
	XmLArray *rowArray;
	XmLArray *colArray;
	GridReg reg[9];
	GridDropLoc dropLoc;

	/* used by Set and GetSubValues */
	XmLGridCellRefValues cellValues, *defCellValues;
	XmString cellString;
	Pixmap cellPixmap;
	Dimension cellPixmapWidth, cellPixmapHeight;
	XmLGridRowValues rowValues;
	XmLGridColumnValues colValues;
	Boolean cellDefaults;
	unsigned char rowType, colType;
	int rowStep, colStep;
	int cellRow, cellCol;
	int cellColRangeStart, cellColRangeEnd;
	int cellRowRangeStart, cellRowRangeEnd;
	} XmLGridPart;

typedef struct _XmLGridRec
	{
	CorePart core;
	CompositePart composite;
	ConstraintPart constraint;
	XmManagerPart manager;
	XmLGridPart grid;
	} XmLGridRec;

typedef struct _XmLGridClassPart
	{
	int unused;
	} XmLGridClassPart;

typedef struct _XmLGridClassRec
	{
	CoreClassPart core_class;
	CompositeClassPart composite_class;
	ConstraintClassPart constraint_class;
	XmManagerClassPart manager_class;
	XmLGridClassPart grid_class;
	} XmLGridClassRec;

extern XmLGridClassRec xmlGridClassRec;

typedef struct _XmLGridConstraintPart
	{
	int unused;
	} XmLGridConstraintPart;

typedef struct _XmLGridConstraintRec
	{
	XmManagerConstraintPart manager;
	XmLGridConstraintPart grid;
	} XmLGridConstraintRec, *XmLGridConstraintPtr;

#endif 
