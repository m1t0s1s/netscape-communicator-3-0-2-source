/*
(c) Copyright 1994, 1995, Microline Software, Inc.  ALL RIGHTS RESERVED
  
THIS PROGRAM BELONGS TO MICROLINE SOFTWARE.  IT IS CONSIDERED A TRADE
SECRET AND IS NOT TO BE DIVULGED OR USED BY PARTIES WHO HAVE NOT
RECEIVED WRITTEN AUTHORIZATION FROM THE OWNER.

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

#include "GridP.h"

#include <Xm/ScrollBar.h>
#include <Xm/Text.h>
#include <Xm/CutPaste.h>
#include <X11/StringDefs.h>
#include <X11/cursorfont.h>
#include <stdio.h>
#include <stdlib.h>

#if defined(_STDC_) || defined(__cplusplus) || defined(c_plusplus)

/* Create and Destroy */
static void ClassInitialize();
static void Initialize(Widget req, Widget newW, ArgList args, Cardinal *nargs);
static void Destroy(Widget w);

/* Geometry, Drawing, Entry and Picking */
static void Realize(Widget w, XtValueMask *valueMask,
	XSetWindowAttributes *attr);
static void Redisplay(Widget w, XExposeEvent *event, Region region);
static void DrawResizeLine(XmLGridWidget g, int xy, int erase);
static void DrawXORRect(XmLGridWidget g, int xy, int size,
	int isVert, int erase);
static void DefineCursor(XmLGridWidget g, char defineCursor);
static void DrawArea(XmLGridWidget g, int type, int row, int col);
static void ExtendSelectRange(XmLGridWidget g, int *type,
	int *fr, int *lr, int *fc, int *lc);
static void ExtendSelect(XmLGridWidget g, Boolean set, int row, int col);
static void SelectTypeArea(XmLGridWidget g, int type, int row, int col,
	Boolean select, Boolean notify);
static int GetSelectedArea(XmLGridWidget g, int type, int *rowPos,
	int *colPos, int count);
static void ChangeManaged(Widget w);
static void Resize(Widget w);
static void PlaceScrollbars(XmLGridWidget w);
static void VertLayout(XmLGridWidget g, int resizeIfNeeded);
static void HorizLayout(XmLGridWidget g, int resizeIfNeeded);
static void RecalcVisPos(XmLGridWidget g, int isVert);
static int PosToVisPos(XmLGridWidget g, int pos, int isVert);
static int VisPosToPos(XmLGridWidget g, int visPos, int isVert);
static unsigned char ColPosToType(XmLGridWidget g, int pos);
static int ColPosToTypePos(XmLGridWidget g, unsigned char type, int pos);
static unsigned char RowPosToType(XmLGridWidget g, int pos);
static int RowPosToTypePos(XmLGridWidget g, unsigned char type, int pos);
static int ColTypePosToPos(XmLGridWidget g, unsigned char type,
	int pos, int allowNeg);
static int RowTypePosToPos(XmLGridWidget g, unsigned char type,
	int pos, int allowNeg);
int ScrollRowBottomPos(XmLGridWidget g, int row);
int ScrollColRightPos(XmLGridWidget g, int col);
static int PosIsResize(XmLGridWidget g, int x, int y,
	int *row, int *col, int *isVert);
static int XYToRowCol(XmLGridWidget g, int x, int y, int *row, int *col);
static int RowColToXY(XmLGridWidget g, int row, int col, XRectangle *rect);
static int RowColFirstSpan(XmLGridWidget g, int row, int col,
	int *spanRow, int *spanCol, XRectangle *rect, Boolean lookLeft,
	Boolean lookUp);
static void RowColSpanRect(XmLGridWidget g, int row, int col, XRectangle *rect);
XmLGridCell GetCell(XmLGridWidget g, int row, int col);
int GetColWidth(XmLGridWidget g, int col);
int GetRowHeight(XmLGridWidget g, int row);
static int ColIsVisible(XmLGridWidget g, int col);
static int RowIsVisible(XmLGridWidget g, int row);
static XtGeometryResult GeometryManager(Widget w, XtWidgetGeometry *request,
	XtWidgetGeometry *);
static void ScrollCB(Widget w, XtPointer, XtPointer);
static int FindNextFocus(XmLGridWidget g, int row, int col,
	int rowDir, int colDir, int *nextRow, int *nextCol);
static int SetFocus(XmLGridWidget g, int row, int col, int rowDir, int colDir);
static void ChangeFocus(XmLGridWidget g, int row, int col);
static void MakeColVisible(XmLGridWidget g, int col);
static void MakeRowVisible(XmLGridWidget g, int row);
static void TextAction(XmLGridWidget g, int action);

/* Getting and Setting Values */
static void GetSubValues(Widget w, ArgList args, Cardinal *nargs);
static void SetSubValues(Widget w, ArgList args, Cardinal *nargs);
static Boolean SetValues(Widget curW, Widget, Widget newW, 
	ArgList args, Cardinal *nargs);
static void CopyFontList(XmLGridWidget g);
static Boolean CvtStringToSizePolicy(Display *dpy, XrmValuePtr args,
	Cardinal *numArgs, XrmValuePtr fromVal, XrmValuePtr toVal,
	XtPointer *data);
static Boolean CvtStringToSelectionPolicy(Display *dpy, XrmValuePtr args,
	Cardinal *numArgs, XrmValuePtr fromVal, XrmValuePtr toVal,
	XtPointer *data);
static Boolean CvtStringToCellAlignment(Display *dpy, XrmValuePtr args,
	Cardinal *numArgs, XrmValuePtr fromVal, XrmValuePtr toVal,
	XtPointer *data);
static Boolean CvtStringToCellType(Display *dpy, XrmValuePtr args,
	Cardinal *numArgs, XrmValuePtr fromVal, XrmValuePtr toVal,
	XtPointer *data);
static Boolean CvtStringToCellBorderType(Display *dpy, XrmValuePtr args,
	Cardinal *numArgs, XrmValuePtr fromVal, XrmValuePtr toVal,
	XtPointer *data);

/* Getting and Setting Row Values */
static void RowValueStringToMask(char *s, long *mask);
static void GetRowValue(XmLGridRow row, XtArgVal value, long mask);
static int SetRowValues(XmLGridWidget g, XmLGridRow row, long mask);

/* Getting and Setting Column Values */
static void ColumnValueStringToMask(char *s, long *mask);
static void GetColumnValue(XmLGridColumn col, XtArgVal value, long mask);
static int SetColumnValues(XmLGridWidget g, XmLGridColumn col, long mask);

/* Getting and Setting Cell Values */
static void CellValueStringToMask(char *s, long *mask);
static void GetCellValue(XmLGridCell cell, XtArgVal value, long mask);
static XmLGridCellRefValues *CellRefValuesCreate(XmLGridWidget g,
	XmLGridCellRefValues *copy);
static void SetCellValuesPreprocess(XmLGridWidget g, long mask);
static int SetCellHasRefValues(long mask);
static int SetCellValuesNeedsResize(XmLGridWidget g, XmLGridRow row,
	XmLGridColumn col, XmLGridCell cell, long mask);
static void SetCellValues(XmLGridWidget g, XmLGridCell cell, long mask);
static void SetCellRefValues(XmLGridWidget g, XmLGridCellRefValues *values,
	long mask);
static int SetCellRefValuesCompare(void *, void **, void **);
static void SetCellRefValuesPreprocess(XmLGridWidget g, int row, int col,
	XmLGridCell cell, long mask);

/* Read, Write, Copy, Paste */
static int Read(XmLGridWidget g, int format, char delimiter,
	int row, int col, char *data);
static void Write(XmLGridWidget g, FILE *file, int format, char delimiter,
	Boolean skipHidden, int row, int col, int nrow, int ncol);
char *CopyDataCreate(XmLGridWidget g, int selected, int row, int col,
	int nrow, int ncol);
Boolean Copy(XmLGridWidget g, Time time, int selected,
	int row, int col, int nrow, int ncol);
Boolean Paste(XmLGridWidget g, int row, int col);

/* Utility */
static void GetCoreBackground(Widget w, int, XrmValue *value);
static void GetManagerForeground(Widget w, int, XrmValue *value);
static void ClipRectToReg(XmLGridWidget g, XRectangle *rect, GridReg *reg);
static char *FileToString(FILE *file);

/* Actions, Callbacks and Handlers */
static void ButtonMotion(Widget w, XEvent *event, String *, Cardinal *);
static void DragTimer(XtPointer, XtIntervalId *);
static void CursorMotion(Widget w, XEvent *event, String *, Cardinal *);
static void Edit(Widget w, XEvent *event, String *, Cardinal *);
static void EditCancel(Widget w, XEvent *event, String *, Cardinal *);
static void EditComplete(Widget w, XEvent *event, String *, Cardinal *);
static void DragStart(Widget w, XEvent *event, String *, Cardinal *);
static Boolean DragConvert(Widget w, Atom *selection, Atom *target,
	Atom *type, XtPointer *value, unsigned long *length, int *format);
static void DragFinish(Widget w, XtPointer clientData, XtPointer callData);
static void DropRegister(XmLGridWidget g, Boolean set);
static void DropStart(Widget w, XtPointer clientData, XtPointer callData);
static void DropTransfer(Widget w, XtPointer clientData, Atom *selType,
	Atom *type, XtPointer value, unsigned long *length, int *format);
static void Select(Widget w, XEvent *event, String *, Cardinal *);
static void TextActivate(Widget w, XtPointer clientData, XtPointer callData);
static void TextFocus(Widget w, XtPointer clientData, XtPointer callData);
static void TextMapped(Widget w, XtPointer closure, XEvent *event,
	Boolean *ctd);
static void TextModifyVerify(Widget w, XtPointer clientData,
	XtPointer callData);
static void Traverse(Widget w, XEvent *event, String *, Cardinal *);

/* XmLArray */

XmLArray *XmLArrayNew();
void XmLArrayFree(XmLArray *array);
void XmLArrayAdd(XmLArray *array, int pos, int count);
int XmLArrayDel(XmLArray *array, int pos, int count);
int XmLArraySet(XmLArray *array, int pos, void *item);
void *XmLArrayGet(XmLArray *array, int pos);
int XmLArrayGetCount(XmLArray *array);
int XmLArrayMove(XmLArray *array, int newPos, int pos, int count);
int XmLArrayReorder(XmLArray *array, int *newPositions, int pos, int count);
int XmLArraySort(XmLArray *array, XmLArrayCompareFunc compare, void *userData,
	int pos, int count);

/* XmLGridRow */

XmLGridRow XmLGridRowNew(Widget grid);
void XmLGridRowFree(XmLGridRow row);
XmLArray *XmLGridRowCells(XmLGridRow row);
int XmLGridRowGetPos(XmLGridRow row);
XmLGridRowValues *XmLGridRowGetValues(XmLGridRow row);
int XmLGridRowGetVisPos(XmLGridRow row);
Boolean XmLGridRowIsHidden(XmLGridRow row);
Boolean XmLGridRowIsSelected(XmLGridRow row);
void XmLGridRowSetSelected(XmLGridRow row, Boolean selected);
void XmLGridRowSetVisPos(XmLGridRow row, int visPos);
int XmLGridRowGetHeightInPixels(XmLGridRow row);
void XmLGridRowHeightChanged(XmLGridRow row);

/* XmLGridColumn */

XmLGridColumn XmLGridColumnNew(Widget grid);
void XmLGridColumnFree(XmLGridColumn column);
XmLGridColumnValues *XmLGridColumnGetValues(XmLGridColumn column);
int XmLGridColumnGetPos(XmLGridColumn column);
int XmLGridColumnGetVisPos(XmLGridColumn column);
Boolean XmLGridColumnIsHidden(XmLGridColumn column);
Boolean XmLGridColumnIsSelected(XmLGridColumn column);
void XmLGridColumnSetSelected(XmLGridColumn column, Boolean selected);
void XmLGridColumnSetVisPos(XmLGridColumn column, int visPos);
int XmLGridColumnGetWidthInPixels(XmLGridColumn column);
void XmLGridColumnWidthChanged(XmLGridColumn column);

/* XmLGridCell */

XmLGridCell XmLGridCellNew();
void XmLGridCellFree(XmLGridCell cell);
int XmLGridCellAction(XmLGridCell cell, Widget w, XmLGridCallbackStruct *cbs);
XmLGridCellRefValues *XmLGridCellGetRefValues(XmLGridCell cell);
void XmLGridCellSetRefValues(XmLGridCell cell, XmLGridCellRefValues *values);
void XmLGridCellDerefValues(XmLGridCellRefValues *values);
Boolean XmLGridCellInRowSpan(XmLGridCell cell);
Boolean XmLGridCellInColumnSpan(XmLGridCell cell);
Boolean XmLGridCellIsSelected(XmLGridCell cell);
Boolean XmLGridCellIsValueSet(XmLGridCell cell);
void XmLGridCellSetValueSet(XmLGridCell cell, Boolean set);
void XmLGridCellSetInRowSpan(XmLGridCell cell, Boolean set);
void XmLGridCellSetInColumnSpan(XmLGridCell cell, Boolean set);
void XmLGridCellSetSelected(XmLGridCell cell, Boolean selected);
void XmLGridCellSetString(XmLGridCell cell, XmString string, Boolean copy);
XmString XmLGridCellGetString(XmLGridCell cell);
void XmLGridCellSetPixmap(XmLGridCell cell, Pixmap pixmap,
	Dimension width, Dimension height);
XmLGridCellPixmap *XmLGridCellGetPixmap(XmLGridCell cell);
void XmLGridCellDrawBackground(XmLGridCell cell, Widget w,
	XRectangle *clipRect, XmLGridDrawStruct *ds);
void XmLGridCellDrawBorders(XmLGridCell cell, Widget w,
	XRectangle *clipRect, XmLGridDrawStruct *ds);
void XmLGridCellDrawValue(XmLGridCell cell, Widget w,
	XRectangle *clipRect, XmLGridDrawStruct *ds);
void XmLGridCellClearTextString(XmLGridCell cell, Widget w);
int XmLGridCellBeginTextEdit(XmLGridCell cell, Widget w,
	XRectangle *clipRect);
void XmLGridCellCompleteTextEdit(XmLGridCell cell, Widget w);
void XmLGridCellInsertText(XmLGridCell cell, Widget w);
int XmLGridCellGetHeight(XmLGridCell cell, XmLGridRow row);
int XmLGridCellGetWidth(XmLGridCell cell, XmLGridColumn col);
void XmLGridCellFreeValue(XmLGridCell cell);

#else

static void ClassInitialize();
static void Initialize();
static void Destroy();

static void Realize();
static void Redisplay();
static void DrawResizeLine();
static void DrawXORRect();
static void DefineCursor();
static void DrawArea();
static void ExtendSelectRange();
static void ExtendSelect();
static void SelectTypeArea();
static int GetSelectedArea();
static void ChangeManaged();
static void Resize();
static void PlaceScrollbars();
static void VertLayout();
static void HorizLayout();
static void RecalcVisPos();
static int PosToVisPos();
static int VisPosToPos();
static unsigned char ColPosToType();
static int ColPosToTypePos();
static unsigned char RowPosToType();
static int RowPosToTypePos();
static int ColTypePosToPos();
static int RowTypePosToPos();
int ScrollRowBottomPos();
int ScrollColRightPos();
static int PosIsResize();
static int XYToRowCol();
static int RowColToXY();
static int RowColFirstSpan();
static void RowColSpanRect();
XmLGridCell GetCell();
int GetColWidth();
int GetRowHeight();
static int ColIsVisible();
static int RowIsVisible();
static XtGeometryResult GeometryManager();
static void ScrollCB();
static int FindNextFocus();
static int SetFocus();
static void ChangeFocus();
static void MakeColVisible();
static void MakeRowVisible();
static void TextAction();

static void GetSubValues();
static void SetSubValues();
static Boolean SetValues();
static void CopyFontList();
static Boolean CvtStringToSizePolicy();
static Boolean CvtStringToSelectionPolicy();
static Boolean CvtStringToCellAlignment();
static Boolean CvtStringToCellType();
static Boolean CvtStringToCellBorderType();

static void RowValueStringToMask();
static void GetRowValue();
static int SetRowValues();

static void ColumnValueStringToMask();
static void GetColumnValue();
static int SetColumnValues();

static void CellValueStringToMask();
static void GetCellValue();
static XmLGridCellRefValues *CellRefValuesCreate();
static void SetCellValuesPreprocess();
static int SetCellHasRefValues();
static int SetCellValuesNeedsResize();
static void SetCellValues();
static void SetCellRefValues();
static int SetCellRefValuesCompare();
static void SetCellRefValuesPreprocess();

static int Read();
static void Write();
char *CopyDataCreate();
Boolean Copy();
Boolean Paste();

static void GetCoreBackground();
static void GetManagerForeground();
static void ClipRectToReg();
static char *FileToString();

static void ButtonMotion();
static void DragTimer();
static void CursorMotion();
static void Edit();
static void EditCancel();
static void EditComplete();
static void DragStart();
static Boolean DragConvert();
static void DragFinish();
static void DropRegister();
static void DropStart();
static void DropTransfer();
static void Select();
static void TextActivate();
static void TextFocus();
static void TextMapped();
static void TextModifyVerify();
static void Traverse();

XmLArray *XmLArrayNew();
void XmLArrayFree();
void XmLArrayAdd();
int XmLArrayDel();
int XmLArraySet();
void *XmLArrayGet();
int XmLArrayGetCount();
int XmLArrayMove();
int XmLArrayReorder();
int XmLArraySort();

XmLGridRow XmLGridRowNew();
void XmLGridRowFree();
XmLArray *XmLGridRowCells();
int XmLGridRowGetPos();
XmLGridRowValues *XmLGridRowGetValues();
int XmLGridRowGetVisPos();
Boolean XmLGridRowIsHidden();
Boolean XmLGridRowIsSelected();
void XmLGridRowSetSelected();
void XmLGridRowSetVisPos();
int XmLGridRowGetHeightInPixels();
void XmLGridRowHeightChanged();

XmLGridColumn XmLGridColumnNew();
void XmLGridColumnFree();
XmLGridColumnValues *XmLGridColumnGetValues();
int XmLGridColumnGetPos();
int XmLGridColumnGetVisPos();
Boolean XmLGridColumnIsHidden();
Boolean XmLGridColumnIsSelected();
void XmLGridColumnSetSelected();
void XmLGridColumnSetVisPos();
int XmLGridColumnGetWidthInPixels();
void XmLGridColumnWidthChanged();

XmLGridCell XmLGridCellNew();
void XmLGridCellFree();
int XmLGridCellAction();
XmLGridCellRefValues *XmLGridCellGetRefValues();
void XmLGridCellSetRefValues();
void XmLGridCellDerefValues();
Boolean XmLGridCellInRowSpan();
Boolean XmLGridCellInColumnSpan();
Boolean XmLGridCellIsSelected();
Boolean XmLGridCellIsValueSet();
void XmLGridCellSetValueSet();
void XmLGridCellSetInRowSpan();
void XmLGridCellSetInColumnSpan();
void XmLGridCellSetSelected();
void XmLGridCellSetString();
XmString XmLGridCellGetString();
void XmLGridCellSetPixmap();
XmLGridCellPixmap *XmLGridCellGetPixmap();
void XmLGridCellDrawBackground();
void XmLGridCellDrawBorders();
void XmLGridCellDrawValue();
void XmLGridCellClearTextString();
int XmLGridCellBeginTextEdit();
void XmLGridCellCompleteTextEdit();
void XmLGridCellInsertText();
int XmLGridCellGetHeight();
int XmLGridCellGetWidth();
void XmLGridCellFreeValue();

#endif

static XtActionsRec actions[] =
	{
	{ "XmLGridEditComplete", EditComplete },
	{ "XmLGridButtonMotion", ButtonMotion },
	{ "XmLGridCursorMotion", CursorMotion },
	{ "XmLGridEditCancel",   EditCancel   },
	{ "XmLGridEdit",         Edit         },
	{ "XmLGridSelect",       Select       },
	{ "XmLGridDragStart",    DragStart    },
	{ "XmLGridTraverse",     Traverse     },
	};

#define TEXT_HIDE           1
#define TEXT_SHOW           2
#define TEXT_EDIT           3
#define TEXT_EDIT_CANCEL    4
#define TEXT_EDIT_COMPLETE  5
#define TEXT_EDIT_INSERT    6

/* Cursors */

#define horizp_width 19
#define horizp_height 13
static unsigned char horizp_bits[] = {
 0x00, 0x00, 0x00, 0xff, 0x07, 0x00, 0xff, 0x07, 0x00, 0x00, 0x06, 0x00,
 0x00, 0x06, 0x00, 0x20, 0x46, 0x00, 0x30, 0xc6, 0x00, 0x38, 0xc6, 0x01,
 0xfc, 0xff, 0x03, 0x38, 0xc6, 0x01, 0x30, 0xc6, 0x00, 0x20, 0x46, 0x00,
 0x00, 0x06, 0x00 };

#define horizm_width 19
#define horizm_height 13
static unsigned char horizm_bits[] = {
 0xff, 0x0f, 0x00, 0xff, 0x0f, 0x00, 0xff, 0x0f, 0x00, 0xff, 0x0f, 0x00,
 0x60, 0x6f, 0x00, 0x70, 0xef, 0x00, 0x78, 0xef, 0x01, 0xfc, 0xff, 0x03,
 0xfe, 0xff, 0x07, 0xfc, 0xff, 0x03, 0x78, 0xef, 0x01, 0x70, 0xef, 0x00,
 0x60, 0x6f, 0x00 };

#define vertp_width 13
#define vertp_height 19
static unsigned char vertp_bits[] = {
   0x06, 0x00, 0x06, 0x00, 0x06, 0x01, 0x86, 0x03, 0xc6, 0x07, 0xe6, 0x0f,
   0x06, 0x01, 0x06, 0x01, 0x06, 0x01, 0xfe, 0x1f, 0xfe, 0x1f, 0x00, 0x01,
   0x00, 0x01, 0x00, 0x01, 0xe0, 0x0f, 0xc0, 0x07, 0x80, 0x03, 0x00, 0x01,
   0x00, 0x00};

#define vertm_width 13
#define vertm_height 19
static unsigned char vertm_bits[] = {
   0x0f, 0x00, 0x0f, 0x01, 0x8f, 0x03, 0xcf, 0x07, 0xef, 0x0f, 0xff, 0x1f,
   0xff, 0x1f, 0x8f, 0x03, 0xff, 0x1f, 0xff, 0x1f, 0xff, 0x1f, 0xff, 0x1f,
   0x80, 0x03, 0xf0, 0x1f, 0xf0, 0x1f, 0xe0, 0x0f, 0xc0, 0x07, 0x80, 0x03,
   0x00, 0x01};

/* Grid Translations */

static char translations[] =
"<Btn1Motion>:           XmLGridButtonMotion()\n\
<MotionNotify>:          XmLGridCursorMotion()\n\
~Ctrl ~Shift <Btn1Down>: XmLGridSelect(BEGIN)\n\
~Ctrl Shift <Btn1Down>:  XmLGridSelect(EXTEND)\n\
Ctrl ~Shift <Btn1Down>:  XmLGridSelect(TOGGLE)\n\
<Btn1Up>:                XmLGridSelect(END)\n\
<Btn2Down>:              XmLGridDragStart()\n\
<EnterWindow>:           ManagerEnter()\n\
<LeaveWindow>:           ManagerLeave()\n\
<FocusOut>:              ManagerFocusOut()\n\
<FocusIn>:               ManagerFocusIn()";

/* Text Translations */

static char traverseTranslations[] =
"~Ctrl ~Shift <Key>osfUp:         XmLGridTraverse(UP)\n\
~Ctrl Shift <Key>osfUp:           XmLGridTraverse(EXTEND_UP)\n\
Ctrl ~Shift <Key>osfUp:           XmLGridTraverse(PAGE_UP)\n\
~Ctrl ~Shift <Key>osfDown:        XmLGridTraverse(DOWN)\n\
~Ctrl Shift <Key>osfDown:         XmLGridTraverse(EXTEND_DOWN)\n\
Ctrl ~Shift <Key>osfDown:         XmLGridTraverse(PAGE_DOWN)\n\
~Ctrl ~Shift <Key>osfLeft:        XmLGridTraverse(LEFT)\n\
~Ctrl Shift <Key>osfLeft:         XmLGridTraverse(EXTEND_LEFT)\n\
Ctrl ~Shift <Key>osfLeft:         XmLGridTraverse(PAGE_LEFT)\n\
~Ctrl ~Shift <Key>osfRight:       XmLGridTraverse(RIGHT)\n\
~Ctrl Shift <Key>osfRight:        XmLGridTraverse(EXTEND_RIGHT)\n\
Ctrl ~Shift <Key>osfRight:        XmLGridTraverse(PAGE_RIGHT)\n\
~Ctrl ~Shift <Key>osfPageUp:      XmLGridTraverse(PAGE_UP)\n\
~Ctrl Shift <Key>osfPageUp:       XmLGridTraverse(EXTEND_PAGE_UP)\n\
Ctrl ~Shift <Key>osfPageUp:       XmLGridTraverse(PAGE_LEFT)\n\
Ctrl Shift <Key>osfPageUp:        XmLGridTraverse(EXTEND_PAGE_LEFT)\n\
~Ctrl Shift <Key>osfPageDown:     XmLGridTraverse(EXTEND_PAGE_DOWN)\n\
Ctrl ~Shift <Key>osfPageDown:     XmLGridTraverse(PAGE_RIGHT)\n\
~Ctrl ~Shift <Key>osfPageDown:    XmLGridTraverse(PAGE_DOWN)\n\
Ctrl Shift <Key>osfPageDown:      XmLGridTraverse(EXTEND_PAGE_RIGHT)\n\
~Ctrl ~Shift <Key>Tab:            XmLGridTraverse(RIGHT)\n\
~Ctrl Shift <Key>Tab:             XmLGridTraverse(LEFT)\n\
~Ctrl ~Shift <Key>Home:           XmLGridTraverse(TO_TOP)\n\
~Ctrl ~Shift <Key>osfBeginLine:   XmLGridTraverse(TO_TOP)\n\
Ctrl ~Shift <Key>Home:            XmLGridTraverse(TO_TOP_LEFT)\n\
~Ctrl ~Shift <Key>End:            XmLGridTraverse(TO_BOTTOM)\n\
~Ctrl ~Shift <Key>osfEndLine:     XmLGridTraverse(TO_BOTTOM)\n\
Ctrl ~Shift <Key>End:             XmLGridTraverse(TO_BOTTOM_RIGHT)\n\
<Key>osfInsert:                   XmLGridEdit()\n\
<Key>F2:                          XmLGridEdit()\n\
~Ctrl ~Shift <KeyDown>space:      XmLGridSelect(BEGIN)\n\
~Ctrl Shift <KeyDown>space:       XmLGridSelect(EXTEND)\n\
Ctrl ~Shift <KeyDown>space:       XmLGridSelect(TOGGLE)\n\
<KeyUp>space:                     XmLGridSelect(END)";

/* You cannot put multiple actions for any translation
   where one translation changes the translation table
   XmLGridComplete() and XmLGridCancel() do this and these
   actions cannot be combined with others */
static char editTranslations[] =
"~Ctrl ~Shift <Key>osfDown:     XmLGridEditComplete(DOWN)\n\
~Ctrl Shift <Key>Tab:	        XmLGridEditComplete(LEFT)\n\
~Ctrl ~Shift <Key>Tab:	        XmLGridEditComplete(RIGHT)\n\
~Ctrl ~Shift <Key>osfUp:        XmLGridEditComplete(UP)\n\
<Key>osfCancel:                 XmLGridEditCancel()";

static XtResource resources[] =
	{
		/* Grid Resources */
		{
		XmNactivateCallback, XmCCallback,
		XmRCallback, sizeof(XtCallbackList),
		XtOffset(XmLGridWidget, grid.activateCallback),
		XmRImmediate, (XtPointer)0,
		},
		{
		XmNaddCallback, XmCCallback,
		XmRCallback, sizeof(XtCallbackList),
		XtOffset(XmLGridWidget, grid.addCallback),
		XmRImmediate, (XtPointer)0,
		},
		{
		XmNallowColumnResize, XmCAllowColumnResize,
		XmRBoolean, sizeof(Boolean),
		XtOffset(XmLGridWidget, grid.allowColResize),
		XmRImmediate, (XtPointer)False,
		},
		{
		XmNallowDragSelected, XmCAllowDragSelected,
		XmRBoolean, sizeof(Boolean),
		XtOffset(XmLGridWidget, grid.allowDrag),
		XmRImmediate, (XtPointer)False,
		},
		{
		XmNallowDrop, XmCAllowDrop,
		XmRBoolean, sizeof(Boolean),
		XtOffset(XmLGridWidget, grid.allowDrop),
		XmRImmediate, (XtPointer)False,
		},
		{
		XmNallowRowResize, XmCAllowRowResize,
		XmRBoolean, sizeof(Boolean),
		XtOffset(XmLGridWidget, grid.allowRowResize),
		XmRImmediate, (XtPointer)False,
		},
		{
		XmNblankBackground, XmCBlankBackground,
		XmRPixel, sizeof(Pixel),
		XtOffset(XmLGridWidget, grid.blankBg),
#ifdef MOTIF11
		XmRCallProc, (XtPointer)GetCoreBackground,
#else
		XmRCallProc, GetCoreBackground,
#endif
		},
		{
		XmNbottomFixedCount, XmCBottomFixedCount,
		XmRInt, sizeof(int),
		XtOffset(XmLGridWidget, grid.bottomFixedCount),
		XmRImmediate, (XtPointer)0,
		},
		{
		XmNbottomFixedMargin, XmCBottomFixedMargin,
		XmRDimension, sizeof(Dimension),
		XtOffset(XmLGridWidget, grid.bottomFixedMargin),
		XmRImmediate, (XtPointer)0,
		},
		{
		XmNcellDefaults, XmCCellDefaults,
		XmRBoolean, sizeof(Boolean),
		XtOffset(XmLGridWidget, grid.cellDefaults),
		XmRImmediate, (XtPointer)False,
		},
		{
		XmNcellDrawCallback, XmCCallback,
		XmRCallback, sizeof(XtCallbackList),
		XtOffset(XmLGridWidget, grid.cellDrawCallback),
		XmRImmediate, (XtPointer)0,
		},
		{
		XmNcellDropCallback, XmCCallback,
		XmRCallback, sizeof(XtCallbackList),
		XtOffset(XmLGridWidget, grid.cellDropCallback),
		XmRImmediate, (XtPointer)0,
		},
		{
		XmNcellFocusCallback, XmCCallback,
		XmRCallback, sizeof(XtCallbackList),
		XtOffset(XmLGridWidget, grid.cellFocusCallback),
		XmRImmediate, (XtPointer)0,
		},
		{
		XmNcellPasteCallback, XmCCallback,
		XmRCallback, sizeof(XtCallbackList),
		XtOffset(XmLGridWidget, grid.cellPasteCallback),
		XmRImmediate, (XtPointer)0,
		},
		{
		XmNcolumns, XmCColumns,
		XmRInt, sizeof(int),
		XtOffset(XmLGridWidget, grid.colCount),
		XmRImmediate, (XtPointer)0,
		},
		{
		XmNdeleteCallback, XmCCallback,
		XmRCallback, sizeof(XtCallbackList),
		XtOffset(XmLGridWidget, grid.deleteCallback),
		XmRImmediate, (XtPointer)0,
		},
		{
		XmNdeselectCallback, XmCCallback,
		XmRCallback, sizeof(XtCallbackList),
		XtOffset(XmLGridWidget, grid.deselectCallback),
		XmRImmediate, (XtPointer)0,
		},
		{
		XmNeditCallback, XmCCallback,
		XmRCallback, sizeof(XtCallbackList),
		XtOffset(XmLGridWidget, grid.editCallback),
		XmRImmediate, (XtPointer)0,
		},
		{
		XmNfontList, XmCFontList,
		XmRFontList, sizeof(XmFontList),
		XtOffset(XmLGridWidget, grid.fontList),
		XmRImmediate, (XtPointer)0
		},
		{
		XmNdebugLevel, XmCDebugLevel,
		XmRChar, sizeof(char),
		XtOffset(XmLGridWidget, grid.debugLevel),
		XmRImmediate, (XtPointer)0,
		},
		{
		XmNeditTranslations, XmCTranslations,
		XmRTranslationTable, sizeof(XtTranslations),
		XtOffset(XmLGridWidget, grid.editTrans),
		XmRString, (XtPointer)editTranslations,
		},
		{
		XmNfooterColumnCount, XmCFooterColumnCount,
		XmRInt, sizeof(int),
		XtOffset(XmLGridWidget, grid.footerColCount),
		XmRImmediate, (XtPointer)0,
		},
		{
		XmNfooterRowCount, XmCFooterRowCount,
		XmRInt, sizeof(int),
		XtOffset(XmLGridWidget, grid.footerRowCount),
		XmRImmediate, (XtPointer)0,
		},
		{
		XmNheadingColumnCount, XmCHeadingColumnCount,
		XmRInt, sizeof(int),
		XtOffset(XmLGridWidget, grid.headingColCount),
		XmRImmediate, (XtPointer)0,
		},
		{
		XmNheadingRowCount, XmCHeadingRowCount,
		XmRInt, sizeof(int),
		XtOffset(XmLGridWidget, grid.headingRowCount),
		XmRImmediate, (XtPointer)0,
		},
		{
		XmNhiddenColumns, XmCHiddenColumns,
		XmRInt, sizeof(int),
		XtOffset(XmLGridWidget, grid.hiddenColCount),
		XmRImmediate, (XtPointer)0,
		},
		{
		XmNhiddenRows, XmCHiddenRows,
		XmRInt, sizeof(int),
		XtOffset(XmLGridWidget, grid.hiddenRowCount),
		XmRImmediate, (XtPointer)0,
		},
		{
		XmNhorizontalScrollBar, XmCHorizontalScrollBar,
		XmRWidget, sizeof(Widget),
		XtOffset(XmLGridWidget, grid.hsb),
		XmRImmediate, (XtPointer)0,
		},
		{
		XmNhorizontalSizePolicy, XmCHorizontalSizePolicy,
		XmRGridSizePolicy, sizeof(unsigned char),
		XtOffset(XmLGridWidget, grid.hsPolicy),
		XmRImmediate, (XtPointer)XmCONSTANT,
		},
		{
		XmNhsbDisplayPolicy, XmCHsbDisplayPolicy,
		XmRScrollBarDisplayPolicy, sizeof(unsigned char),
		XtOffset(XmLGridWidget, grid.hsbDisplayPolicy),
		XmRImmediate, (XtPointer)XmAS_NEEDED,
		},
		{
		XmNlayoutFrozen, XmCLayoutFrozen,
		XmRBoolean, sizeof(Boolean),
		XtOffset(XmLGridWidget, grid.layoutFrozen),
		XmRImmediate, (XtPointer)False,
		},
		{
		XmNleftFixedCount, XmCLeftFixedCount,
		XmRInt, sizeof(int),
		XtOffset(XmLGridWidget, grid.leftFixedCount),
		XmRImmediate, (XtPointer)0,
		},
		{
		XmNleftFixedMargin, XmCLeftFixedMargin,
		XmRDimension, sizeof(Dimension),
		XtOffset(XmLGridWidget, grid.leftFixedMargin),
		XmRImmediate, (XtPointer)0,
		},
		{
		XmNrightFixedCount, XmCRightFixedCount,
		XmRInt, sizeof(int),
		XtOffset(XmLGridWidget, grid.rightFixedCount),
		XmRImmediate, (XtPointer)0,
		},
		{
		XmNrightFixedMargin, XmCRightFixedMargin,
		XmRDimension, sizeof(Dimension),
		XtOffset(XmLGridWidget, grid.rightFixedMargin),
		XmRImmediate, (XtPointer)0,
		},
		{
		XmNresizeCallback, XmCCallback,
		XmRCallback, sizeof(XtCallbackList),
		XtOffset(XmLGridWidget, grid.resizeCallback),
		XmRImmediate, (XtPointer)0,
		},
		{
		XmNrows, XmCRows,
		XmRInt, sizeof(int),
		XtOffset(XmLGridWidget, grid.rowCount),
		XmRImmediate, (XtPointer)0,
		},
		{
		XmNscrollBarMargin, XmCScrollBarMargin,
		XmRDimension, sizeof(Dimension),
		XtOffset(XmLGridWidget, grid.scrollBarMargin),
		XmRImmediate, (XtPointer)2,
		},
		{
		XmNscrollCallback, XmCCallback,
		XmRCallback, sizeof(XtCallbackList),
		XtOffset(XmLGridWidget, grid.scrollCallback),
		XmRImmediate, (XtPointer)0,
		},
		{
		XmNscrollColumn, XmCScrollColumn,
		XmRInt, sizeof(int),
		XtOffset(XmLGridWidget, grid.cScrollCol),
		XmRImmediate, (XtPointer)0,
		},
		{
		XmNscrollRow, XmCScrollRow,
		XmRInt, sizeof(int),
		XtOffset(XmLGridWidget, grid.cScrollRow),
		XmRImmediate, (XtPointer)0,
		},
		{
		XmNselectCallback, XmCCallback,
		XmRCallback, sizeof(XtCallbackList),
		XtOffset(XmLGridWidget, grid.selectCallback),
		XmRImmediate, (XtPointer)0,
		},
		{
		XmNselectionPolicy, XmCGridSelectionPolicy,
		XmRGridSelectionPolicy, sizeof(unsigned char),
		XtOffset(XmLGridWidget, grid.selectionPolicy),
		XmRImmediate, (XtPointer)XmSELECT_BROWSE_ROW,
		},
		{
		XmNselectBackground, XmCSelectBackground,
		XmRPixel, sizeof(Pixel),
		XtOffset(XmLGridWidget, grid.selectBg),
#ifdef MOTIF11
		XmRCallProc, (XtPointer)GetManagerForeground,
#else
		XmRCallProc, GetManagerForeground,
#endif
		},
		{
		XmNselectForeground, XmCSelectForeground,
		XmRPixel, sizeof(Pixel),
		XtOffset(XmLGridWidget, grid.selectFg),
#ifdef MOTIF11
		XmRCallProc, (XtPointer)GetCoreBackground,
#else
		XmRCallProc, GetCoreBackground,
#endif
		},
		{
		XmNshadowRegions, XmCShadowRegions,
		XmRInt, sizeof(int),
		XtOffset(XmLGridWidget, grid.shadowRegions),
		XmRImmediate, (XtPointer)511,
		},
		{
		XmNtextWidget, XmCTextWidget,
		XmRWidget, sizeof(Widget),
		XtOffset(XmLGridWidget, grid.text),
		XmRImmediate, (XtPointer)0,
		},
		{
		XmNtraverseTranslations, XmCTranslations,
		XmRTranslationTable, sizeof(XtTranslations),
		XtOffset(XmLGridWidget, grid.traverseTrans),
		XmRString, (XtPointer)traverseTranslations,
		},
		{
		XmNtopFixedCount, XmCTopFixedCount,
		XmRInt, sizeof(int),
		XtOffset(XmLGridWidget, grid.topFixedCount),
		XmRImmediate, (XtPointer)0,
		},
		{
		XmNtopFixedMargin, XmCTopFixedMargin,
		XmRDimension, sizeof(Dimension),
		XtOffset(XmLGridWidget, grid.topFixedMargin),
		XmRImmediate, (XtPointer)0,
		},
		{
		XmNuseAverageFontWidth, XmCUseAverageFontWidth,
		XmRBoolean, sizeof(Boolean),
		XtOffset(XmLGridWidget, grid.useAvgWidth),
		XmRImmediate, (XtPointer)True,
		},
		{
		XmNverticalScrollBar, XmCVerticalScrollBar,
		XmRWidget, sizeof(Widget),
		XtOffset(XmLGridWidget, grid.vsb),
		XmRImmediate, (XtPointer)0,
		},
		{
		XmNverticalSizePolicy, XmCVerticalSizePolicy,
		XmRGridSizePolicy, sizeof(unsigned char),
		XtOffset(XmLGridWidget, grid.vsPolicy),
		XmRImmediate, (XtPointer)XmCONSTANT,
		},
		{
		XmNvsbDisplayPolicy, XmCVsbDisplayPolicy,
		XmRScrollBarDisplayPolicy, sizeof(unsigned char),
		XtOffset(XmLGridWidget, grid.vsbDisplayPolicy),
		XmRImmediate, (XtPointer)XmAS_NEEDED,
		},
		/* Row Resources */
		{
		XmNrow, XmCRow,
		XmRInt, sizeof(int),
		XtOffset(XmLGridWidget, grid.cellRow),
		XmRImmediate, (XtPointer)-1,
		},
		{
		XmNrowUserData, XmCUserData,
		XmRPointer, sizeof(XtPointer),
		XtOffset(XmLGridWidget, grid.rowValues.userData),
		XmRImmediate, (XtPointer)0,
		},
		{
		XmNrowHeight, XmCRowHeight,
		XmRDimension, sizeof(Dimension),
		XtOffset(XmLGridWidget, grid.rowValues.height),
		XmRImmediate, (XtPointer)0,
		},
		{
		XmNrowRangeEnd, XmCRowRangeEnd,
		XmRInt, sizeof(int),
		XtOffset(XmLGridWidget, grid.cellRowRangeEnd),
		XmRImmediate, (XtPointer)-1,
		},
		{
		XmNrowRangeStart, XmCRowRangeStart,
		XmRInt, sizeof(int),
		XtOffset(XmLGridWidget, grid.cellRowRangeStart),
		XmRImmediate, (XtPointer)-1,
		},
		{
		XmNrowSizePolicy, XmCRowSizePolicy,
		XmRGridSizePolicy, sizeof(unsigned char),
		XtOffset(XmLGridWidget, grid.rowValues.sizePolicy),
		XmRImmediate, (XtPointer)0,
		},
		{
		XmNrowStep, XmCRowStep,
		XmRInt, sizeof(int),
		XtOffset(XmLGridWidget, grid.rowStep),
		XmRImmediate, (XtPointer)1,
		},
		{
		XmNrowType, XmCRowType,
		XmRUnsignedChar, sizeof(unsigned char),
		XtOffset(XmLGridWidget, grid.rowType),
		XmRImmediate, (XtPointer)XmCONTENT,
		},
		/* Column Resources */
		{
		XmNcolumn, XmCColumn,
		XmRInt, sizeof(int),
		XtOffset(XmLGridWidget, grid.cellCol),
		XmRImmediate, (XtPointer)-1,
		},
		{
		XmNcolumnUserData, XmCUserData,
		XmRPointer, sizeof(XtPointer),
		XtOffset(XmLGridWidget, grid.colValues.userData),
		XmRImmediate, (XtPointer)0,
		},
		{
		XmNcolumnWidth, XmCColumnWidth,
		XmRDimension, sizeof(Dimension),
		XtOffset(XmLGridWidget, grid.colValues.width),
		XmRImmediate, (XtPointer)0,
		},
		{
		XmNcolumnRangeEnd, XmCColumnRangeEnd,
		XmRInt, sizeof(int),
		XtOffset(XmLGridWidget, grid.cellColRangeEnd),
		XmRImmediate, (XtPointer)-1,
		},
		{
		XmNcolumnRangeStart, XmCColumnRangeStart,
		XmRInt, sizeof(int),
		XtOffset(XmLGridWidget, grid.cellColRangeStart),
		XmRImmediate, (XtPointer)-1,
		},
		{
		XmNcolumnSizePolicy, XmCColumnSizePolicy,
		XmRGridSizePolicy, sizeof(unsigned char),
		XtOffset(XmLGridWidget, grid.colValues.sizePolicy),
		XmRImmediate, (XtPointer)0,
		},
		{
		XmNcolumnStep, XmCColumnStep,
		XmRInt, sizeof(int),
		XtOffset(XmLGridWidget, grid.colStep),
		XmRImmediate, (XtPointer)1,
		},
		{
		XmNcolumnType, XmCColumnType,
		XmRUnsignedChar, sizeof(unsigned char),
		XtOffset(XmLGridWidget, grid.colType),
		XmRImmediate, (XtPointer)XmCONTENT,
		},
		/* Cell Resources */
		{
		XmNcellAlignment, XmCCellAlignment,
		XmRCellAlignment, sizeof(unsigned char),
		XtOffset(XmLGridWidget, grid.cellValues.alignment),
		XmRImmediate, (XtPointer)0,
		},
		{
		XmNcellBackground, XmCCellBackground,
		XmRPixel, sizeof(Pixel),
		XtOffset(XmLGridWidget, grid.cellValues.background),
		XmRImmediate, (XtPointer)0,
		},
		{
		XmNcellBottomBorderPixel, XmCCellBottomBorderPixel,
		XmRPixel, sizeof(Pixel),
		XtOffset(XmLGridWidget, grid.cellValues.bottomBorderPixel),
		XmRImmediate, (XtPointer)0,
		},
		{
		XmNcellBottomBorderType, XmCCellBottomBorderType,
		XmRCellBorderType, sizeof(unsigned char),
		XtOffset(XmLGridWidget, grid.cellValues.bottomBorderType),
		XmRImmediate, (XtPointer)0,
		},
		{
		XmNcellColumnSpan, XmCCellColumnSpan,
		XmRInt, sizeof(int),
		XtOffset(XmLGridWidget, grid.cellValues.columnSpan),
		XmRImmediate, (XtPointer)0,
		},
		{
		XmNcellUserData, XmCUserData,
		XmRPointer, sizeof(XtPointer),
		XtOffset(XmLGridWidget, grid.cellValues.userData),
		XmRImmediate, (XtPointer)0,
		},
		{
		XmNcellFontList, XmCCellFontList,
		XmRFontList, sizeof(XmFontList),
		XtOffset(XmLGridWidget, grid.cellValues.fontList),
		XmRImmediate, (XtPointer)0,
		},
		{
		XmNcellForeground, XmCCellForeground,
		XmRPixel, sizeof(Pixel),
		XtOffset(XmLGridWidget, grid.cellValues.foreground),
		XmRImmediate, (XtPointer)0,
		},
		{
		XmNcellLeftBorderPixel, XmCCellLeftBorderPixel,
		XmRPixel, sizeof(Pixel),
		XtOffset(XmLGridWidget, grid.cellValues.leftBorderPixel),
		XmRImmediate, (XtPointer)0,
		},
		{
		XmNcellLeftBorderType, XmCCellLeftBorderType,
		XmRCellBorderType, sizeof(unsigned char),
		XtOffset(XmLGridWidget, grid.cellValues.leftBorderType),
		XmRImmediate, (XtPointer)0,
		},
		{
		XmNcellPixmap, XmCCellPixmap,
		XmRPixmap, sizeof(Pixmap),
		XtOffset(XmLGridWidget, grid.cellPixmap),
		XmRImmediate, (XtPointer)XmUNSPECIFIED_PIXMAP,
		},
		{
		XmNcellRightBorderPixel, XmCCellRightBorderPixel,
		XmRPixel, sizeof(Pixel),
		XtOffset(XmLGridWidget, grid.cellValues.rightBorderPixel),
		XmRImmediate, (XtPointer)0,
		},
		{
		XmNcellRightBorderType, XmCCellRightBorderType,
		XmRCellBorderType, sizeof(unsigned char),
		XtOffset(XmLGridWidget, grid.cellValues.rightBorderType),
		XmRImmediate, (XtPointer)0,
		},
		{
		XmNcellRowSpan, XmCCellRowSpan,
		XmRInt, sizeof(int),
		XtOffset(XmLGridWidget, grid.cellValues.rowSpan),
		XmRImmediate, (XtPointer)0,
		},
		{
		XmNcellString, XmCXmString,
		XmRXmString, sizeof(XmString),
		XtOffset(XmLGridWidget, grid.cellString),
		XmRImmediate, (XtPointer)0,
		},
		{
		XmNcellTopBorderPixel, XmCCellTopBorderPixel,
		XmRPixel, sizeof(Pixel),
		XtOffset(XmLGridWidget, grid.cellValues.topBorderPixel),
		XmRImmediate, (XtPointer)0,
		},
		{
		XmNcellTopBorderType, XmCCellTopBorderType,
		XmRCellBorderType, sizeof(unsigned char),
		XtOffset(XmLGridWidget, grid.cellValues.topBorderType),
		XmRImmediate, (XtPointer)0,
		},
		{
		XmNcellType, XmCCellType,
		XmRCellType, sizeof(unsigned char),
		XtOffset(XmLGridWidget, grid.cellValues.type),
		XmRImmediate, (XtPointer)0,
		},
		/* Overridden inherited resources */
		{
		XmNshadowThickness, XmCShadowThickness,
		XmRHorizontalDimension, sizeof(Dimension),
		XtOffset(XmLGridWidget, manager.shadow_thickness),
		XmRImmediate, (XtPointer)2,
		},
    };

XmLGridClassRec xmlGridClassRec =
    {
        { /* core_class */
        (WidgetClass)&xmManagerClassRec,          /* superclass         */
        "XmLGrid",                                /* class_name         */
        sizeof(XmLGridRec),                       /* widget_size        */
        ClassInitialize,                          /* class_init         */
        0,                                        /* class_part_init    */
        FALSE,                                    /* class_inited       */
        (XtInitProc)Initialize,                   /* initialize         */
        0,                                        /* initialize_hook    */
        (XtRealizeProc)Realize,                   /* realize            */
        (XtActionList)actions,                    /* actions            */
        (Cardinal)XtNumber(actions),              /* num_actions        */
        resources,                                /* resources          */
        XtNumber(resources),                      /* num_resources      */
        NULLQUARK,                                /* xrm_class          */
        TRUE,                                     /* compress_motion    */
        XtExposeCompressMaximal,                  /* compress_exposure  */
        TRUE,                                     /* compress_enterleave*/
        TRUE,                                     /* visible_interest   */
        (XtWidgetProc)Destroy,                    /* destroy            */
        (XtWidgetProc)Resize,                     /* resize             */
        (XtExposeProc)Redisplay,                  /* expose             */
        (XtSetValuesFunc)SetValues,               /* set_values         */
        0,                                        /* set_values_hook    */
        XtInheritSetValuesAlmost,                 /* set_values_almost  */
        (XtArgsProc)GetSubValues,                 /* get_values_hook    */
        0,                                        /* accept_focus       */
        XtVersion,                                /* version            */
        0,                                        /* callback_private   */
        translations,                             /* tm_table           */
        0,                                        /* query_geometry     */
        0,                                        /* display_accelerator*/
        0,                                        /* extension          */
        },
        { /* composite_class */
        (XtGeometryHandler)GeometryManager,       /* geometry_manager   */
        (XtWidgetProc)ChangeManaged,              /* change_managed     */
        XtInheritInsertChild,                     /* insert_child       */
        XtInheritDeleteChild,                     /* delete_child       */
        0,                                        /* extension          */
        },
        { /* constraint_class */
        0,	                                      /* subresources       */
        0,                                        /* subresource_count  */
        sizeof(XmLGridConstraintRec),             /* constraint_size    */
        0,                                        /* initialize         */
        0,                                        /* destroy            */
        0,                                        /* set_values         */
        0,                                        /* extension          */
        },
        { /* manager_class */
        XtInheritTranslations,                    /* translations          */
        0,                                        /* syn resources         */
        0,                                        /* num syn_resources     */
        0,                                        /* get_cont_resources    */
        0,                                        /* num_get_cont_resources*/
        XmInheritParentProcess,                   /* parent_process        */
        0,                                        /* extension             */
        },
        { /* grid_class */
        0,                                        /* unused                */
        }
    };

WidgetClass xmlGridWidgetClass = (WidgetClass)&xmlGridClassRec;

/*
   Create and Destroy
*/

static void ClassInitialize()
	{
	XtSetTypeConverter(XmRString, XmRGridSizePolicy,
		CvtStringToSizePolicy, 0, 0, XtCacheNone, 0);
	XtSetTypeConverter(XmRString, XmRGridSelectionPolicy,
		CvtStringToSelectionPolicy, 0, 0, XtCacheNone, 0);
	XtSetTypeConverter(XmRString, XmRCellAlignment,
		CvtStringToCellAlignment, 0, 0, XtCacheNone, 0);
	XtSetTypeConverter(XmRString, XmRCellType,
		CvtStringToCellType, 0, 0, XtCacheNone, 0);
	XtSetTypeConverter(XmRString, XmRCellBorderType,
		CvtStringToCellBorderType, 0, 0, XtCacheNone, 0);
	/* long must be > 2 bytes for cell mask to work */
	if (sizeof(long) < 3)
		fprintf(stderr, "xmlGridClass: fatal error: long < 3 bytes\n");
	}

static void Initialize(reqW, newW, args, narg)
Widget reqW, newW;
ArgList args;
Cardinal *narg;
	{
	XmLGridWidget g, request;
	Display *dpy;
	Window root;
	Widget shell;
	Pixmap pix, pixMask;
	XColor fg, bg;
	XtGCMask mask;
	XGCValues values;
	GridReg *reg;
	int i;
	unsigned char kfp;

	g = (XmLGridWidget)newW;
	dpy = XtDisplay((Widget)g);
	request = (XmLGridWidget)reqW;
	root = RootWindowOfScreen(XtScreen(newW));

	shell = XmLShellOfWidget(newW);
	if (shell && XmIsVendorShell(shell)) 
		{
		XtVaGetValues(shell,
			XmNkeyboardFocusPolicy, &kfp,
			NULL);
		if (kfp == XmPOINTER)
			XmLWarning(newW, "keyboardFocusPolicy of XmPOINTER not supported");
		}

	g->grid.rowArray = XmLArrayNew();
	g->grid.colArray = XmLArrayNew();

	if (g->core.width <= 0) 
		g->core.width = 100;
	if (g->core.height <= 0) 
		g->core.height = 100;

	CopyFontList(g);

	g->grid.text = XtVaCreateManagedWidget(
		"text", xmTextWidgetClass, (Widget)g,
		XmNmarginHeight, 0,
		XmNmarginWidth, 3,
		XmNshadowThickness, 0,
		XmNhighlightThickness, 0,
		XmNx, 0,
		XmNy, 0,
		XmNwidth, 40,
		XmNheight, 40,
		NULL);
	XtOverrideTranslations(g->grid.text, g->grid.traverseTrans);
	XtAddEventHandler(g->grid.text, StructureNotifyMask,
		True, (XtEventHandler)TextMapped, (XtPointer)0);
	XtAddCallback(g->grid.text, XmNactivateCallback, TextActivate, 0);
	XtAddCallback(g->grid.text, XmNfocusCallback, TextFocus, 0);
	XtAddCallback(g->grid.text, XmNlosingFocusCallback, TextFocus, 0);
	XtAddCallback(g->grid.text, XmNmodifyVerifyCallback, TextModifyVerify, 0);

	g->grid.hsb = XtVaCreateWidget(
		"hsb", xmScrollBarWidgetClass, (Widget)g,
		XmNincrement, 1,
		XmNorientation, XmHORIZONTAL,
		XmNtraversalOn, False,
		NULL);
	XtAddCallback(g->grid.hsb, XmNdragCallback, ScrollCB, 0);
	XtAddCallback(g->grid.hsb, XmNvalueChangedCallback, ScrollCB, 0);
	g->grid.vsb = XtVaCreateWidget(
		"vsb", xmScrollBarWidgetClass, (Widget)g,
		XmNorientation, XmVERTICAL,
		XmNincrement, 1,
		XmNtraversalOn, False,
		NULL);
	XtAddCallback(g->grid.vsb, XmNdragCallback, ScrollCB, 0);
	XtAddCallback(g->grid.vsb, XmNvalueChangedCallback, ScrollCB, 0);
	PlaceScrollbars(g);

	/* Cursors */
	fg.red = ~0;
	fg.green = ~0;
	fg.blue = ~0;
	fg.pixel = WhitePixelOfScreen(XtScreen((Widget)g));
	fg.flags = DoRed | DoGreen | DoBlue;
	bg.red = 0;
	bg.green = 0;
	bg.blue = 0;
	bg.pixel = BlackPixelOfScreen(XtScreen((Widget)g));
	bg.flags = DoRed | DoGreen | DoBlue;
    pix = XCreatePixmapFromBitmapData(dpy, DefaultRootWindow(dpy),
		(char *)horizp_bits, horizp_width, horizp_height, 0, 1, 1);
    pixMask = XCreatePixmapFromBitmapData(dpy, DefaultRootWindow(dpy),
		(char *)horizm_bits, horizm_width, horizm_height, 1, 0, 1);
	g->grid.hResizeCursor = XCreatePixmapCursor(dpy, pix, pixMask,
		&fg, &bg, 9, 9);
	XFreePixmap(dpy, pix);
	XFreePixmap(dpy, pixMask);
    pix = XCreatePixmapFromBitmapData(dpy, DefaultRootWindow(dpy),
		(char *)vertp_bits, vertp_width, vertp_height, 0, 1, 1);
    pixMask = XCreatePixmapFromBitmapData(dpy, DefaultRootWindow(dpy),
		(char *)vertm_bits, vertm_width, vertm_height, 1, 0, 1);
	g->grid.vResizeCursor = XCreatePixmapCursor(dpy, pix, pixMask,
		&fg, &bg, 9, 9);
	XFreePixmap(dpy, pix);
	XFreePixmap(dpy, pixMask);
	g->grid.cursorDefined = CursorNormal;

#if 0
/* XXX
 * Calling XCreateGC here is bad because:
 * 1) this widget's parent may not be realized when we get
 *    here so we won't have a window at all.
 * 2) this widget doesn't necessarily have the same visual
 *    as our parent, so using the root will break whenever
 *    they differ.
 */
	g->grid.fallbackFont = XLoadQueryFont(dpy, "fixed");
	values.foreground = g->manager.foreground;
	values.font = g->grid.fallbackFont->fid;
	mask = GCForeground | GCFont;
	g->grid.gc = XCreateGC(dpy, root, mask, &values);
#endif /* 0 */

	g->grid.focusIn = 0;
	g->grid.focusRow = -1;
	g->grid.focusCol = -1;
	g->grid.scrollCol = 0;
	g->grid.scrollRow = 0;
	g->grid.textHidden = 1;
	g->grid.inMode = InNormal;
	g->grid.inEdit = 0;
	g->grid.needsHorizLayout = 0;
	g->grid.needsVertLayout = 0;
	g->grid.recalcHorizVisPos = 0;
	g->grid.recalcVertVisPos = 0;
	g->grid.defCellValues = CellRefValuesCreate(g, 0);
	g->grid.defCellValues->refCount = 1;
	g->grid.ignoreModifyVerify = 0;
	g->grid.extendSel = 0;
	g->grid.extendRow = -1;
	g->grid.extendCol = -1;
	g->grid.extendToRow = -1;
	g->grid.extendToCol = -1;
	g->grid.lastSelectRow = -1;
	g->grid.lastSelectCol = -1;
	g->grid.lastSelectTime = 0;
	g->grid.dragTimerSet = 0;

	g->grid.rowType = XmCONTENT;
	g->grid.colType = XmCONTENT;
	g->grid.cellRow = -1;
	g->grid.cellRowRangeStart = -1;
	g->grid.cellRowRangeEnd = -1;
	g->grid.cellCol = -1;
	g->grid.cellColRangeStart = -1;
	g->grid.cellColRangeEnd = -1;

	reg = g->grid.reg;
    for (i = 0; i < 9; i++)
			{
			reg[i].x = 0;
			reg[i].y = 0;
			reg[i].width = 0;
			reg[i].height = 0;
			reg[i].row = 0;
			reg[i].col = 0;
			reg[i].nrow = 0;
			reg[i].ncol = 0;
			}

	DropRegister(g, g->grid.allowDrop);
	}

static void Destroy(w)
Widget w;
	{
	XmLGridWidget g;
	Display *dpy;
	int i, count;

	g = (XmLGridWidget)w;
	dpy = XtDisplay(w);
	if (g->grid.dragTimerSet)
		XtRemoveTimeOut(g->grid.dragTimerId);
	DefineCursor(g, CursorNormal);
	XFreeCursor(dpy, g->grid.hResizeCursor);
	XFreeCursor(dpy, g->grid.vResizeCursor);
	XFreeGC(dpy, g->grid.gc);
	XFreeFont(dpy, g->grid.fallbackFont);
	XmFontListFree(g->grid.fontList);
	XmLGridCellDerefValues(g->grid.defCellValues);
	ExtendSelect(g, False, -1, -1);
	count = XmLArrayGetCount(g->grid.rowArray);
	for (i = 0; i < count; i++)
		XmLGridRowFree((XmLGridRow)XmLArrayGet(g->grid.rowArray, i));
	XmLArrayFree(g->grid.rowArray);
	count = XmLArrayGetCount(g->grid.colArray);
	for (i = 0; i < count; i++)
		XmLGridColumnFree((XmLGridColumn)XmLArrayGet(g->grid.colArray, i));
	XmLArrayFree(g->grid.colArray);
	}

/*
   Geometry, Drawing, Entry and Picking
*/

static void Realize(w, valueMask, attr)
Widget w;
XtValueMask *valueMask;
XSetWindowAttributes *attr;
	{
	XmLGridWidget g;
	WidgetClass superClass;
	XtRealizeProc realize;

	g = (XmLGridWidget)w;

	superClass = xmlGridWidgetClass->core_class.superclass;
	realize = superClass->core_class.realize;
	(*realize)(w, valueMask, attr);

	/* XXX create the gc here, since we now have a window */
	    {
	    Display *dpy = XtDisplay (w);
	    XtGCMask mask;
	    XGCValues values;

	    g->grid.fallbackFont = XLoadQueryFont(dpy, "fixed");
	    values.foreground = g->manager.foreground;
	    values.font = g->grid.fallbackFont->fid;
	    mask = GCForeground | GCFont;
	    g->grid.gc = XCreateGC(dpy, XtWindow(w), mask, &values);
	    }
	}

static void Redisplay(w, event, region)
Widget w;
XExposeEvent *event;
Region region;
	{
	XmLGridWidget g;
	Display *dpy;
	Window win;
	XmLGridCell cell;
	XmLGridRow row;
	XmLGridColumn col;
	XmLGridCellRefValues *cellValues;
	XmLGridDrawStruct ds;
	XmLGridCallbackStruct cbs;
	GridReg *reg;
	XRectangle eRect, rRect, clipRect, rect[6];
	int i, n, st, c, r, sc, sr, width, height, hasDrawCB;
	Boolean spanUp, spanLeft;

	g = (XmLGridWidget)w;
	if (!XtIsRealized((Widget)g))
		return;
	if (!g->core.visible)
		return;
	if (g->grid.layoutFrozen == True)
		XmLWarning(w, "Redisplay() - layout frozen is True during redraw");
	dpy = XtDisplay(g);
	win = XtWindow(g);
	st = g->manager.shadow_thickness;
	reg = g->grid.reg;
	if (event)
		{
		eRect.x = event->x;
		eRect.y = event->y;
		eRect.width = event->width;
		eRect.height = event->height;
		if (g->grid.debugLevel)
			fprintf(stderr, "XmLGrid: Redisplay x %d y %d w %d h %d\n",
				event->x, event->y, event->width, event->height);
		}
	else
		{
		eRect.x = 0;
		eRect.y = 0;
		eRect.width = g->core.width;
		eRect.height = g->core.height;
		}
	if (!eRect.width || !eRect.height)
		return;
	/* Hide any XORed graphics */
	DrawResizeLine(g, 0, 1);
	hasDrawCB = 0;
	if (XtHasCallbacks(w, XmNcellDrawCallback) == XtCallbackHasSome)
			hasDrawCB = 1;
	for (i = 0; i < 9; i++)
		{
		if (g->grid.debugLevel)
			fprintf(stderr, "XmLGrid: Redisplay region %d phase 0\n", i);
		if (!reg[i].width || !reg[i].height)
			continue;
		rRect.x = reg[i].x;
		rRect.y = reg[i].y;
		rRect.width = reg[i].width;
		rRect.height = reg[i].height;
		if (XmLRectIntersect(&eRect, &rRect) == XmLRectOutside)
			continue;
		if (g->grid.debugLevel)
			fprintf(stderr, "XmLGrid: Redisplay region %d phase 1\n", i);
		rRect.x += st;
		rRect.width -= st * 2;
		rRect.y += st;
		rRect.height -= st * 2;
		if (XmLRectIntersect(&eRect, &rRect) != XmLRectInside &&
			g->manager.shadow_thickness)
			{
			if (g->grid.shadowRegions & (1 << i))
#ifdef MOTIF11
				_XmDrawShadow(dpy, win,
					g->manager.bottom_shadow_GC,
					g->manager.top_shadow_GC,
					g->manager.shadow_thickness,
					reg[i].x, reg[i].y,
					reg[i].width, reg[i].height);
#else
				_XmDrawShadows(dpy, win,
					g->manager.top_shadow_GC,
					g->manager.bottom_shadow_GC,
					reg[i].x, reg[i].y,
					reg[i].width, reg[i].height,
					g->manager.shadow_thickness,
					XmSHADOW_IN);
#endif
			else
#ifdef MOTIF11
				_XmEraseShadow(dpy, win,
					g->manager.shadow_thickness,
					reg[i].x, reg[i].y,
					reg[i].width, reg[i].height);
#else
				_XmClearBorder(dpy, win,
					reg[i].x, reg[i].y,
					reg[i].width, reg[i].height,
					g->manager.shadow_thickness);
#endif
			}
		rRect.x += st;
		height = 0;
		if (g->grid.debugLevel)
			fprintf(stderr, "XmLGrid: Redisplay region %d phase 2\n", i);
		for (r = reg[i].row; r < reg[i].row + reg[i].nrow; r++)
			{
			width = 0;
			for (c = reg[i].col; c < reg[i].col + reg[i].ncol; c++)
				{
				rRect.x = reg[i].x + st + width;
				rRect.y = reg[i].y + st + height;
				rRect.width = GetColWidth(g, c);
				rRect.height = GetRowHeight(g, r);
				width += rRect.width;
				cell = GetCell(g, r, c);
				if (!cell)
					continue;
				cellValues = XmLGridCellGetRefValues(cell);
				if (!cellValues)
					continue;

				spanUp = False;
				spanLeft = False;
				if (XmLGridCellInRowSpan(cell))
					{
					if (r == reg[i].row)
						{
						spanUp = True;
						if (c == reg[i].col)
							spanLeft = True;
						}
					else
						continue;
					}
				if (XmLGridCellInColumnSpan(cell))
					{
					if (c == reg[i].col)
						spanLeft = True;
					else
						continue;
					}
				sr = r;
				sc = c;
				if (spanUp == True || spanLeft == True ||
					cellValues->rowSpan || cellValues->columnSpan)
					{
					if (RowColFirstSpan(g, r, c, &sr, &sc, &rRect,
						spanLeft, spanUp) == -1)
						continue;
					RowColSpanRect(g, sr, sc, &rRect);
					}

				clipRect = rRect;
				ClipRectToReg(g, &clipRect, &reg[i]);

				if (event && XRectInRegion(region, clipRect.x, clipRect.y,
						clipRect.width, clipRect.height) == RectangleOut)
					continue;

				cell = GetCell(g, sr, sc);
				if (!cell)
					continue;
				cbs.reason = XmCR_CELL_DRAW;
				cbs.event = (XEvent *)event;
				cbs.rowType = RowPosToType(g, sr);
				cbs.row = RowPosToTypePos(g, cbs.rowType, sr);
				cbs.columnType = ColPosToType(g, sc);
				cbs.column = ColPosToTypePos(g, cbs.columnType, sc);
				cbs.clipRect = &clipRect;
				cbs.drawInfo = &ds;
				ds.gc = g->grid.gc;
				ds.cellRect = &rRect;
				ds.selectBackground = g->grid.selectBg;
				ds.selectForeground = g->grid.selectFg;
				if (g->grid.focusIn && g->grid.focusRow == sr &&
					g->grid.focusCol == sc)
					ds.hasFocus = True;
				else
					ds.hasFocus = False;
				row = (XmLGridRow)XmLArrayGet(g->grid.rowArray, sr);
				col = (XmLGridColumn)XmLArrayGet(g->grid.colArray, sc);
				if (XmLGridRowIsSelected(row) == True ||
					XmLGridColumnIsSelected(col) == True ||
					XmLGridCellIsSelected(cell) == True)
					ds.isSelected = True;
				else
					ds.isSelected = False;
				if (g->grid.selectionPolicy == XmSELECT_CELL && ds.hasFocus)
					ds.isSelected = False;
				ds.stringDirection = g->manager.string_direction;
				XmLGridCellAction(cell, (Widget)g, &cbs);
				if (hasDrawCB)
					XtCallCallbackList(w, g->grid.cellDrawCallback,
						(XtPointer)&cbs);
				}
			height += GetRowHeight(g, r);
			}
		}
	if (g->grid.debugLevel)
		fprintf(stderr, "XmLGrid: Redisplay non-cell areas\n");
	n = 0;
	if (reg[0].width && g->grid.leftFixedMargin)
		{
		rect[n].x = reg[0].width;
		rect[n].y = 0;
		rect[n].width = g->grid.leftFixedMargin;
		rect[n].height = g->core.height;
		n++;
		}
	if (reg[2].width && g->grid.rightFixedMargin)
		{
		width = 0;
		if (reg[0].ncol)
			width += reg[0].width + g->grid.leftFixedMargin;
		if (reg[1].ncol)
			width += reg[1].width;
		rect[n].x = width;
		rect[n].y = 0;
		rect[n].width = g->grid.rightFixedMargin;
		rect[n].height = g->core.height;
		n++;
		}
	if (reg[0].height && g->grid.topFixedMargin)
		{
		rect[n].x = 0;
		rect[n].y = reg[0].height;
		rect[n].width = g->core.width;
		rect[n].height = g->grid.topFixedMargin;
		n++;
		}
	if (reg[6].height && g->grid.bottomFixedMargin)
		{
		rect[n].x = 0;
		height = 0;
		if (reg[0].nrow)
			height += reg[0].height + g->grid.topFixedMargin;
		if (reg[3].nrow)
			height += reg[3].height;
		rect[n].y = height;
		rect[n].width = g->core.width;
		rect[n].height = g->grid.bottomFixedMargin;
		n++;
		}
	width = reg[1].width;
	if (reg[0].ncol)
		width += reg[0].width + g->grid.leftFixedMargin;
	if (reg[2].ncol)
		width += g->grid.rightFixedMargin + reg[2].width;
	if (width < (int)g->core.width)
		{
		rect[n].x = width;
		rect[n].y = 0;
		rect[n].width = g->core.width - width;
		rect[n].height = g->core.height;
		n++;
		}
	height = reg[3].height;
	if (reg[0].nrow)
		height += reg[0].height + g->grid.topFixedMargin;
	if (reg[6].nrow)
		height += g->grid.bottomFixedMargin + reg[6].height;
	if (height < (int)g->core.height)
		{
		rect[n].x = 0;
		rect[n].y = height;
		rect[n].width = g->core.width;
		rect[n].height = g->core.height - height;
		n++;
		}
	for (i = 0; i < n; i++)
		{
		if (XmLRectIntersect(&eRect, &rect[i]) == XmLRectOutside)
			continue;
		XClearArea(dpy, win, rect[i].x, rect[i].y, rect[i].width,
			rect[i].height, False);
		}
	n = 0;
	if (reg[1].width)
		{
		width = 0;
		for (c = reg[1].col; c < reg[1].col + reg[1].ncol; c++)
			width += GetColWidth(g, c);
		for (i = 1; i < 9; i += 3)
			if (reg[i].height && width < reg[i].width - st * 2)
				{
				rect[n].x = reg[i].x + st + width;
				rect[n].y = reg[i].y + st;
				rect[n].width = reg[i].x + reg[i].width -
					rect[n].x - st;
				rect[n].height = reg[i].height - st * 2;
				n++;
				}
		}
	if (reg[3].height)
		{
		height = 0;
		for (r = reg[3].row; r < reg[3].row + reg[3].nrow; r++)
			height += GetRowHeight(g, r);
		for (i = 3; i < 6; i++)
			if (reg[i].width && height < reg[i].height - st * 2)
				{
				rect[n].x = reg[i].x + st;
				rect[n].y = reg[i].y + st + height;
				rect[n].width = reg[i].width - st * 2;
				rect[n].height = reg[i].y + reg[i].height -
					rect[n].y - st;
				n++;
				}
		}
	XSetForeground(dpy, g->grid.gc, g->grid.blankBg);
	for (i = 0; i < n; i++)
		{
		if (XmLRectIntersect(&eRect, &rect[i]) == XmLRectOutside)
			continue;
		XFillRectangle(dpy, win, g->grid.gc, rect[i].x, rect[i].y,
			rect[i].width, rect[i].height);
		}
	/* Show any XORed graphics */
	DrawResizeLine(g, 0, 1);
	}

static void DrawResizeLine(g, xy, erase)
XmLGridWidget g;
int xy;
int erase;
	{
	if (g->grid.inMode != InResize)
		return;
	DrawXORRect(g, xy, 2, g->grid.resizeIsVert, erase);
	}

static void DrawXORRect(g, xy, size, isVert, erase)
XmLGridWidget g;
int xy;
int size;
int isVert;
int erase;
	{
	Display *dpy;
	Window win;
	GC gc;
	Pixel black, white;
	int oldXY, maxX, maxY;

	/* erase is (0 == none) (1 == hide/show) (2 == permenent erase) */
	dpy = XtDisplay(g);
	win = XtWindow(g);
	gc = g->grid.gc;
	XSetFunction(dpy, gc, GXinvert);
	black = BlackPixelOfScreen(XtScreen((Widget)g));
	white = WhitePixelOfScreen(XtScreen((Widget)g));
	XSetForeground(dpy, gc, black ^ white);
	maxX = g->core.width;
	maxY = g->core.height;
	if (XtIsManaged (g->grid.vsb))
	  maxX -= (g->grid.vsb->core.width + g->grid.scrollBarMargin);
	if (XtIsManaged (g->grid.hsb))
	  maxY -= (g->grid.hsb->core.height + g->grid.scrollBarMargin);
	oldXY = g->grid.resizeLineXY;
	if (isVert)
		{
		if (oldXY != -1)
			XFillRectangle(dpy, win, gc, 0, oldXY, maxX, size);
		}
	else
		{
		if (oldXY != -1)
			XFillRectangle(dpy, win, gc, oldXY, 0, size, maxY);
		}
	if (!erase)
		{
		if (isVert)
			{
			if (xy > maxY)
				xy = maxY - 2;
			if (xy < 0)
				xy = 0;
			XFillRectangle(dpy, win, gc, 0, xy, maxX, size);
			}
		else
			{
			if (xy > maxX)
				xy = maxX - 2;
			if (xy < 0)
				xy = 0;
			XFillRectangle(dpy, win, gc, xy, 0, size, maxY);
			}
		g->grid.resizeLineXY = xy;
		}
	else if (erase == 2)
		g->grid.resizeLineXY = -1;
	XSetFunction(dpy, gc, GXcopy);
	}

static void DefineCursor(g, defineCursor)
XmLGridWidget g;
char defineCursor;
	{
	Display *dpy;
	Window win;

	if (!XtIsRealized((Widget)g))
		return;
	dpy = XtDisplay(g);
	win = XtWindow(g);
	if (defineCursor != g->grid.cursorDefined)
		XUndefineCursor(dpy, win);
	if (defineCursor == CursorVResize)
		XDefineCursor(dpy, win, g->grid.vResizeCursor);
	else if (defineCursor == CursorHResize)
		XDefineCursor(dpy, win, g->grid.hResizeCursor);
	g->grid.cursorDefined = defineCursor;
	}

static void DrawArea(g, type, row, col)
XmLGridWidget g;
int type;
int row;
int col;
	{
	GridReg *reg;
	Display *dpy;
	Window win;
	XExposeEvent event;
	XRectangle rect[3];
	int i, j, n;
	Dimension width, height, st;

	if (g->grid.layoutFrozen == True)
		return;
	if (!XtIsRealized((Widget)g))
		return;
	if (!g->core.visible)
		return;
	dpy = XtDisplay(g);
	win = XtWindow(g);
	reg = g->grid.reg;
	st = g->manager.shadow_thickness;
	if (g->grid.debugLevel)
		fprintf(stderr, "XmLGrid: DrawArea %d %d %d\n", type, row, col);

	n = 0;
	switch (type)
	{
	case DrawAll:
		{
		rect[n].x = 0;
		rect[n].y = 0;
		rect[n].width = g->core.width;
		rect[n].height = g->core.height;
		n++;
		break;
		}
	case DrawHScroll:
		{
		for (i = 1; i < 9; i += 3)
			{
			if (!reg[i].width || !reg[i].height)
				continue;
			rect[n].x = reg[i].x + st;
			rect[n].y = reg[i].y + st;
			rect[n].width = reg[i].width - st * 2;
			rect[n].height = reg[i].height - st * 2;
			n++;
			}
		break;
		}
	case DrawVScroll:
		{
		for (i = 3; i < 6; i++)
			{
			if (!reg[i].width || !reg[i].height)
				continue;
			rect[n].x = reg[i].x + st;
			rect[n].y = reg[i].y + st;
			rect[n].width = reg[i].width - st * 2;
			rect[n].height = reg[i].height - st * 2;
			n++;
			}
		break;
		}
	case DrawRow:
		{
		for (i = 0; i < 9; i++)
			{
			if (!reg[i].width || !reg[i].height)
				continue;
			if (!(row >= reg[i].row &&
				row < reg[i].row + reg[i].nrow))
				continue;
			height = 0;
			for (j = reg[i].row; j < row; j++)
				height += GetRowHeight(g, j);
			rect[n].x = reg[i].x + st;
			rect[n].y = reg[i].y + st + height;
			rect[n].width = reg[i].width - st * 2;
			rect[n].height = GetRowHeight(g, row);
			ClipRectToReg(g, &rect[n], &reg[i]);
			n++;
			}
		break;
		}
	case DrawCol:
		{
		for (i = 0; i < 9; i++)
			{
			if (!reg[i].width || !reg[i].height)
				continue;
			if (!(col >= reg[i].col &&
				col < reg[i].col + reg[i].ncol))
				continue;
			width = 0;
			for (j = reg[i].col; j < col; j++)
				width += GetColWidth(g, j);
			rect[n].x = reg[i].x + st + width;
			rect[n].y = reg[i].y + st;
			rect[n].width = GetColWidth(g, col);
			rect[n].height = reg[i].height - st * 2;
			ClipRectToReg(g, &rect[n], &reg[i]);
			n++;
			}
		break;
		}
	case DrawCell:
		{
		if (!RowColToXY(g, row, col, &rect[n]))
			n++;
		break;
		}
	}
	for (i = 0; i < n; i++)
		{
		if (!rect[i].width || !rect[i].height)
			continue;
		event.type = Expose;
		event.window = win;
		event.display = dpy;
		event.x = rect[i].x;
		event.y = rect[i].y;
		event.width = rect[i].width;
		event.height = rect[i].height;
		event.send_event = True;
		event.count = 0;
		XSendEvent(dpy, win, False, ExposureMask, (XEvent *)&event);
		if (g->grid.debugLevel)
			fprintf(stderr, "XmLGrid: DrawArea expose x %d y %d w %d h %d\n",
				event.x, event.y, event.width, event.height);
		}
	}

static void ExtendSelectRange(g, type, fr, lr, fc, lc)
XmLGridWidget g;
int *type;
int *fr, *lr, *fc, *lc;
	{
	int r, c;

	if ((g->grid.selectionPolicy == XmSELECT_MULTIPLE_ROW) ||
		(ColPosToType(g, g->grid.extendCol) != XmCONTENT))
		*type = SelectRow;
	else if (RowPosToType(g, g->grid.extendRow) != XmCONTENT)
		*type = SelectCol;
	else
		*type = SelectCell;

	r = g->grid.extendToRow;
	if (r < g->grid.headingRowCount)
		r = g->grid.headingRowCount;
	if (r >= g->grid.headingRowCount + g->grid.rowCount)
		r = g->grid.headingRowCount + g->grid.rowCount - 1;
	if (*type == SelectCol)
		{
		*fr = 0;
		*lr = 1;
		}
	else if (g->grid.extendRow < r)
		{
		*fr = g->grid.extendRow;
		*lr = r;
		}
	else
		{
		*fr = r;
		*lr = g->grid.extendRow;
		}
	c = g->grid.extendToCol;
	if (c < g->grid.headingColCount)
		c = g->grid.headingColCount;
	if (c >= g->grid.headingColCount + g->grid.colCount)
		c = g->grid.headingColCount + g->grid.colCount - 1;
	if (*type == SelectRow)
		{
		*fc = 0;
		*lc = 1;
		}
	else if (g->grid.extendCol < c)
		{
		*fc = g->grid.extendCol;
		*lc = c;
		}
	else
		{
		*fc = c;
		*lc = g->grid.extendCol;
		}
	}

static void ExtendSelect(g, set, row, col)
XmLGridWidget g;
Boolean set;
int row;
int col;
	{
	int type;
	int r, fr, lr;
	int c, fc, lc;

	if (row == -1 || col == -1)
		{
		g->grid.extendRow = -1;
		g->grid.extendCol = -1;
		g->grid.extendToRow = -1;
		g->grid.extendToCol = -1;
		if (g->grid.extendSel)
			free((char *)g->grid.extendSel);
		return;
		}
	if (RowPosToType(g, row) == XmFOOTER || ColPosToType(g, col) == XmFOOTER)
		return;
	if ((g->grid.extendToRow == row && g->grid.extendToCol == col) ||
		(g->grid.extendRow == -1 && row == g->grid.focusRow &&
		g->grid.extendCol == -1 && col == g->grid.focusCol))
		return;
	if (g->grid.extendRow != -1 && g->grid.extendCol != -1)
		{
		/* clear previous extend */
		ExtendSelectRange(g, &type, &fr, &lr, &fc, &lc);
		for (r = fr; r <= lr; r += 1)
			for (c = fc; c <= lc; c += 1)
				SelectTypeArea(g, type, RowPosToTypePos(g, XmCONTENT, r),
					ColPosToTypePos(g, XmCONTENT, c), False, True);
		}
	else
		{
		g->grid.extendRow = g->grid.focusRow;
		g->grid.extendCol = g->grid.focusCol;
		}
	if (set == True)
		{
		g->grid.extendRow = row;
		g->grid.extendCol = col;
		}
	if (g->grid.extendRow < 0 || g->grid.extendCol < 0)
		return;
	g->grid.extendToRow = row;
	g->grid.extendToCol = col;

	/* set new extend */
	ExtendSelectRange(g, &type, &fr, &lr, &fc, &lc);
	for (r = fr; r <= lr; r += 1)
		for (c = fc; c <= lc; c += 1)
			SelectTypeArea(g, type, RowPosToTypePos(g, XmCONTENT, r),
				ColPosToTypePos(g, XmCONTENT, c), True, True);
	}

static void SelectTypeArea(g, type, row, col, select, notify)
XmLGridWidget g;
int type;
int row, col;
Boolean select;
Boolean notify;
	{
	Widget w;
	XmLGridRow rowp;
	XmLGridColumn colp;
	XmLGridCell cellp;
	int r, fr, lr, hrc;
	int c, fc, lc, hcc;
	int badPos, hasCB;
	XmLGridCallbackStruct cbs;

	w = (Widget)g;
	hrc = g->grid.headingRowCount;
	hcc = g->grid.headingColCount;
	cbs.rowType = XmCONTENT;
	cbs.columnType = XmCONTENT;
	hasCB = 0;
	if (select == True)
		{
		if (type == SelectRow)
			cbs.reason = XmCR_SELECT_ROW;
		else if (type == SelectCol)
			cbs.reason = XmCR_SELECT_COLUMN;
		else if (type == SelectCell)
			cbs.reason = XmCR_SELECT_CELL;
		if (XtHasCallbacks(w, XmNselectCallback) == XtCallbackHasSome)
			hasCB = 1;
		}
	else
		{
		if (type == SelectRow)
			cbs.reason = XmCR_DESELECT_ROW;
		else if (type == SelectCol)
			cbs.reason = XmCR_DESELECT_COLUMN;
		else if (type == SelectCell)
			cbs.reason = XmCR_DESELECT_CELL;
		if (XtHasCallbacks(w, XmNdeselectCallback) == XtCallbackHasSome)
			hasCB = 1;
		}
	if (row != -1)
		{
		fr = hrc + row;
		lr = fr + 1;
		}
	else
		{
		fr = hrc;
		lr = XmLArrayGetCount(g->grid.rowArray) - g->grid.footerRowCount;
		}
	if (col != -1)
		{
		fc = hcc + col;
		lc = fc + 1;
		}
	else
		{
		fc = hcc;
		lc = XmLArrayGetCount(g->grid.colArray) - g->grid.footerColCount;
		}
	badPos = 0;
	for (r = fr; r < lr; r++)
		for (c = fc; c < lc; c++)
			{
			if (type == SelectRow)
				{
				rowp = (XmLGridRow)XmLArrayGet(g->grid.rowArray, r);
				if (!rowp)
					{
					badPos = 1;
					continue;
					}
				if (XmLGridRowIsSelected(rowp) == select)
					continue;
				if (select == True &&
					(g->grid.selectionPolicy == XmSELECT_BROWSE_ROW ||
					g->grid.selectionPolicy == XmSELECT_SINGLE_ROW))
					SelectTypeArea(g, SelectRow, -1, 0, False, notify);
				XmLGridRowSetSelected(rowp, select);
				if (RowIsVisible(g, r))
					DrawArea(g, DrawRow, r, 0);
				if (notify && hasCB)
					{
					cbs.row = r - hrc;
					if (select == True)
						XtCallCallbackList(w, g->grid.selectCallback,
							(XtPointer)&cbs);
					else
						XtCallCallbackList(w, g->grid.deselectCallback,
							(XtPointer)&cbs);
					}
				}
			else if (type == SelectCol)
				{
				colp = (XmLGridColumn)XmLArrayGet(g->grid.colArray, c);
				if (!colp)
					{
					badPos = 1;
					continue;
					}
				if (XmLGridColumnIsSelected(colp) == select)
					continue;
				XmLGridColumnSetSelected(colp, select);
				if (ColIsVisible(g, c))
					DrawArea(g, DrawCol, 0, c);
				if (notify && hasCB)
					{
					cbs.column = c - hcc;
					if (select == True)
						XtCallCallbackList(w, g->grid.selectCallback,
							(XtPointer)&cbs);
					else
						XtCallCallbackList(w, g->grid.deselectCallback,
							(XtPointer)&cbs);
					}
				}
			else if (type == SelectCell)
				{
				cellp = GetCell(g, r, c);
				if (!cellp)
					{
					badPos = 1;
					continue;
					}
				if (XmLGridCellIsSelected(cellp) == select)
					continue;
				XmLGridCellSetSelected(cellp, select);
				DrawArea(g, DrawCell, r, c);
				if (notify && hasCB)
					{
					cbs.column = c - hcc;
					cbs.row = r - hrc;
					if (select == True)
						XtCallCallbackList(w, g->grid.selectCallback,
							(XtPointer)&cbs);
					else
						XtCallCallbackList(w, g->grid.deselectCallback,
							(XtPointer)&cbs);
					}
				}
			}
	if (badPos)
		XmLWarning((Widget)g, "SelectTypeArea() - bad position");
	}

static int GetSelectedArea(g, type, rowPos, colPos, count)
XmLGridWidget g;
int type;
int *rowPos, *colPos;
int count;
	{
	XmLGridRow row;
	XmLGridColumn col;
	XmLGridCell cell;
	int r, fr, lr;
	int c, fc, lc;
	int n;

	if (type == SelectCol)
		{
		fr = 0;
		lr = 1;
		}
	else
		{
		fr = g->grid.headingRowCount;
		lr = XmLArrayGetCount(g->grid.rowArray) - g->grid.footerRowCount;
		}
	if (type == SelectRow)
		{
		fc = 0;
		lc = 1;
		}
	else
		{
		fc = g->grid.headingColCount;
		lc = XmLArrayGetCount(g->grid.colArray) - g->grid.footerColCount;
		}
	n = 0;
	for (r = fr; r < lr; r++)
		for (c = fc; c < lc; c++)
			{
			if (type == SelectRow)
				{
				row = (XmLGridRow)XmLArrayGet(g->grid.rowArray, r);
				if (row && XmLGridRowIsSelected(row) == True)
					{
					if (rowPos && n < count)
						rowPos[n] = r - fr;
					n++;
					}
				}
			else if (type == SelectCol)
				{
				col = (XmLGridColumn)XmLArrayGet(g->grid.colArray, c);
				if (col && XmLGridColumnIsSelected(col) == True)
					{
					if (colPos && n < count)
						colPos[n] = c - fc;
					n++;
					}
				}
			else if (type == SelectCell)
				{
				cell = GetCell(g, r, c);
				if (cell && XmLGridCellIsSelected(cell) == True)
					{
					if (rowPos && colPos && n < count)
						{
						rowPos[n] = r - fr;
						colPos[n] = c - fc;
						}
					n++;
					}
				}
			}
	return n;
	}

static void ChangeManaged(w)
Widget w;
	{
	_XmNavigChangeManaged(w);
	}

static void Resize(w)
Widget w;
	{
	XmLGridWidget g;

	g = (XmLGridWidget)w;
	VertLayout(g, 0);
	HorizLayout(g, 0);
	PlaceScrollbars(g);
	DrawArea(g, DrawAll, 0, 0);
	}

static void PlaceScrollbars(g)
XmLGridWidget g;
	{
	int x, y;
	int width, height;
	Widget vsb, hsb;

	vsb = g->grid.vsb;
	hsb = g->grid.hsb;
	width = g->core.width;
	if (g->grid.vsPolicy == XmCONSTANT)
		width -= vsb->core.width;
	if (width <= 0)
		width = 1;
	y = g->core.height - hsb->core.height;
	if (hsb->core.x != 0 || hsb->core.y != y || hsb->core.width != width)
		XtConfigureWidget(hsb, 0, y, width, hsb->core.height, 0);

	height = g->core.height;
	if (g->grid.hsPolicy == XmCONSTANT)
		height -= hsb->core.height;
	if (height <= 0)
		width = 1;
	x = g->core.width - vsb->core.width;
	if (vsb->core.x != x || vsb->core.y != 0 || vsb->core.height != height)
		XtConfigureWidget(vsb, x, 0, vsb->core.width, height, 0);
	}

static void VertLayout(g, resizeIfNeeded)
XmLGridWidget g;
int resizeIfNeeded;
	{
	GridReg *reg;
	int i, j, st2, height, rowCount;
	int topNRow, topHeight;
	int midRow, midY, midNRow, midHeight;
	int botRow, botY, botNRow, botHeight;
	int scrollChanged, needsSB, needsResize, cRow;
	int maxRow, maxPos, maxHeight, newHeight, sliderSize;
	XtWidgetGeometry req;
	XmLGridCallbackStruct cbs;

	if (g->grid.layoutFrozen == True)
		{
		g->grid.needsVertLayout = 1;
		return;
		}
	scrollChanged = 0;
	needsResize = 0;
	needsSB = 0;
	rowCount = XmLArrayGetCount(g->grid.rowArray);
	reg = g->grid.reg;
	st2 = g->manager.shadow_thickness * 2;
	TextAction(g, TEXT_HIDE);

	topHeight = 0;
	topNRow = g->grid.topFixedCount;
	if (topNRow > g->grid.rowCount + g->grid.headingRowCount)
		topNRow = g->grid.rowCount + g->grid.headingRowCount;
	if (topNRow)
		{
		topHeight += st2;
		for (i = 0; i < topNRow; i++)
			topHeight += GetRowHeight(g, i);
		}
	botHeight = 0;
	botNRow = g->grid.bottomFixedCount;
	if (topNRow + botNRow > rowCount)
		botNRow = rowCount - topNRow;
	botRow = rowCount - botNRow;
	if (botNRow)
		{
		botHeight += st2;
		for (i = botRow; i < rowCount; i++)
			botHeight += GetRowHeight(g, i);
		}
	height = 0;
	if (topNRow)
		height += topHeight + g->grid.topFixedMargin;
	midY = height;
	if (botNRow)
		height += botHeight + g->grid.bottomFixedMargin;
	if (g->grid.hsPolicy == XmCONSTANT)
		{
		height += g->grid.hsb->core.height; 
		height += g->grid.scrollBarMargin;
		}
	maxHeight = g->core.height - height;
	if (g->grid.vsPolicy != XmCONSTANT)
		{
		if (rowCount == topNRow + botNRow)
			midHeight = 0;
		else
			midHeight = st2;
		for (i = topNRow; i < rowCount - botNRow; i++)
			midHeight += GetRowHeight(g, i);
		midRow = topNRow;
		midNRow = rowCount - topNRow - botNRow;
		needsResize = 1;
		newHeight = midHeight + height;
		if (g->grid.debugLevel)
			fprintf(stderr, "XmLGrid: VertLayout VARIABLE height\n");
		}
	else
		{
		if (maxHeight < st2)
			maxHeight = 0;
		height = st2;
		j = rowCount - botNRow - 1;
		for (i = j; i >= topNRow; i--)
			{
			height += GetRowHeight(g, i);
			if (height > maxHeight)
				break;
			}
		i++;
		if (i > j)
			i = j;
		maxRow = i;
		if (maxRow < topNRow)
			maxRow = topNRow;
		if (g->grid.debugLevel)
			fprintf(stderr, "Grid: VertLayout max scroll row %d\n", i);
		if (maxRow == topNRow)
			{
			if (g->grid.scrollRow != topNRow)
				{
				scrollChanged = 1;
				g->grid.scrollRow = topNRow;
				}
			midRow = topNRow;
			midHeight = maxHeight;
			midNRow = rowCount - topNRow - botNRow;
			if (g->grid.debugLevel)
				fprintf(stderr, "XmLGrid: VertLayout everything fits\n");
			}
		else
			{
			if (g->grid.scrollRow < topNRow)
				{
				scrollChanged = 1;
				g->grid.scrollRow = topNRow;
				if (g->grid.debugLevel)
					fprintf(stderr, "XmLGrid: VertLayout scrolled < topRow\n");
				}
			if (g->grid.scrollRow > maxRow)
				{
				if (g->grid.debugLevel)
					fprintf(stderr, "XmLGrid: VertLayout scrolled > maxRow\n");
				scrollChanged = 1;
				g->grid.scrollRow = maxRow;
				}
			height = st2;
			midNRow = 0;
			for (i = g->grid.scrollRow; i < rowCount - botNRow; i++)
				{
				midNRow++;
				height += GetRowHeight(g, i);
				if (height >= maxHeight)
					break;
				}
			needsSB = 1;
			midRow = g->grid.scrollRow;
			midHeight = maxHeight;
			}
		}
	botY = midY + midHeight;
	if (botNRow)
		botY += g->grid.bottomFixedMargin;
	for (i = 0; i < 3; i++)
		{
		reg[i].y = 0;
		reg[i].height = topHeight;
		reg[i].row = 0;
		reg[i].nrow = topNRow;
		}
	for (i = 3; i < 6; i++)
		{
		reg[i].y = midY;
		reg[i].height = midHeight;
		reg[i].row = midRow;
		reg[i].nrow = midNRow;
		}
	for (i = 6; i < 9; i++)
		{
		reg[i].y = botY;
		reg[i].height = botHeight;
		reg[i].row = botRow;
		reg[i].nrow = botNRow;
		}
	if (g->grid.debugLevel)
			{
			fprintf(stderr, "XmLGrid: VertLayout T y %d h %d r %d nr %d\n",
				reg[0].y, reg[0].height, reg[0].row, reg[0].nrow);
			fprintf(stderr, "XmLGrid: VertLayout M y %d h %d r %d nr %d\n",
				reg[3].y, reg[3].height, reg[3].row, reg[3].nrow);
			fprintf(stderr, "XmLGrid: VertLayout B y %d h %d r %d nr %d\n",
				reg[6].y, reg[6].height, reg[6].row, reg[6].nrow);
			}
	if (needsSB)
		{
		if (!XtIsManaged(g->grid.vsb))
			XtManageChild(g->grid.vsb);
		if (g->grid.debugLevel)
			fprintf(stderr, "XmLGrid: VertLayout set sb values\n");
		maxPos = PosToVisPos(g, maxRow, 1);
		sliderSize = PosToVisPos(g, rowCount - botNRow - 1, 1) - maxPos + 1;
		XtVaSetValues(g->grid.vsb,
			XmNminimum, PosToVisPos(g, topNRow, 1),
			XmNmaximum, maxPos + sliderSize,
			XmNsliderSize, sliderSize,
			XmNpageIncrement, sliderSize,
			XmNvalue, PosToVisPos(g, g->grid.scrollRow, 1),
			NULL);
		}
	else if (g->grid.vsPolicy == XmCONSTANT &&
		g->grid.vsbDisplayPolicy == XmSTATIC)
		{
		if (!XtIsManaged(g->grid.vsb))
			XtManageChild(g->grid.vsb);
		if (g->grid.debugLevel)
			fprintf(stderr, "XmLGrid: VertLayout vsb not needed but static\n");
		XtVaSetValues(g->grid.vsb,
			XmNminimum, 0,
			XmNmaximum, 1,
			XmNsliderSize, 1,
			XmNpageIncrement, 1,
			XmNvalue, 0,
			NULL);
		}
	else
		{
		if (XtIsManaged(g->grid.vsb))
			XtUnmanageChild(g->grid.vsb);
		}
	if (needsResize && resizeIfNeeded && newHeight != g->core.height)
		{
		if (newHeight < 1)
			newHeight = 1;
		req.request_mode = CWHeight;
		req.height = newHeight;
		if (g->grid.debugLevel)
			fprintf(stderr, "XmLGrid: VertLayout Req h %d\n", (int)newHeight);
		XtMakeGeometryRequest((Widget)g, &req, NULL);
		PlaceScrollbars(g);
		}
	if (scrollChanged)
		DrawArea(g, DrawVScroll, 0, 0);
	TextAction(g, TEXT_SHOW);
	cRow = g->grid.scrollRow - g->grid.headingRowCount;
	if (cRow != g->grid.cScrollRow)
		{
		g->grid.cScrollRow = cRow;
		cbs.reason = XmCR_SCROLL_ROW;
		cbs.rowType = XmCONTENT;
		cbs.row = cRow;
		XtCallCallbackList((Widget)g, g->grid.scrollCallback, (XtPointer)&cbs);
		}
	}

static void HorizLayout(g, resizeIfNeeded)
XmLGridWidget g;
int resizeIfNeeded;
	{
	GridReg *reg;
	int i, j, st2, width, colCount;
	int leftNCol, leftWidth;
	int midCol, midX, midNCol, midWidth;
	int rightCol, rightX, rightNCol, rightWidth;
	int scrollChanged, needsSB, needsResize, cCol;
	int maxCol, maxPos, maxWidth, newWidth, sliderSize;
	XtWidgetGeometry req;
	XmLGridCallbackStruct cbs;

	if (g->grid.layoutFrozen == True)
		{
		g->grid.needsHorizLayout = 1;
		return;
		}
	scrollChanged = 0;
	needsResize = 0;
	needsSB = 0;
	colCount = XmLArrayGetCount(g->grid.colArray);
	reg = g->grid.reg;
	st2 = g->manager.shadow_thickness * 2;
	TextAction(g, TEXT_HIDE);

	leftWidth = 0;
	leftNCol = g->grid.leftFixedCount;
	if (leftNCol > g->grid.colCount + g->grid.headingColCount)
		leftNCol = g->grid.colCount + g->grid.headingColCount;
	if (leftNCol)
		{
		leftWidth += st2;
		for (i = 0; i < leftNCol; i++)
			leftWidth += GetColWidth(g, i);
		}
	rightWidth = 0;
	rightNCol = g->grid.rightFixedCount;
	if (rightNCol + leftNCol > colCount)
		rightNCol = colCount - leftNCol;
	rightCol = colCount - rightNCol;
	if (rightNCol)
		{
		rightWidth += st2;
		for (i = rightCol; i < colCount; i++)
			rightWidth += GetColWidth(g, i);
		}
	width = 0;
	if (leftNCol)
		width += leftWidth + g->grid.leftFixedMargin;
	midX = width;
	if (rightNCol)
		width += rightWidth + g->grid.rightFixedMargin;
	if (g->grid.vsPolicy == XmCONSTANT)
		{
		width += g->grid.vsb->core.width; 
		width += g->grid.scrollBarMargin;
		}
	maxWidth = g->core.width - width;
	if (g->grid.hsPolicy != XmCONSTANT)
		{
		if (colCount == leftNCol + rightNCol)
			midWidth = 0;
		else
			midWidth = st2;
		for (i = leftNCol; i < colCount - rightNCol; i++)
			midWidth += GetColWidth(g, i);
		midCol = leftNCol;
		midNCol = colCount - leftNCol - rightNCol;
		needsResize = 1;
		newWidth = midWidth + width;
		if (g->grid.debugLevel)
			fprintf(stderr, "XmLGrid: HorizLayout VARIABLE width\n");
		}
	else
		{
		if (maxWidth < st2)
			maxWidth = 0;
		width = st2;
		j = colCount - rightNCol - 1;
		for (i = j; i >= leftNCol; i--)
			{
			width += GetColWidth(g, i);
			if (width > maxWidth)
				break;
			}
		i++;
		if (i > j)
			i = j;
		maxCol = i;
		if (maxCol < leftNCol)
			maxCol = leftNCol;
		if (g->grid.debugLevel)
			fprintf(stderr, "XmLGrid: HorizLayout max scroll col %d\n", i);
		if (maxCol == leftNCol)
			{
			if (g->grid.scrollCol != leftNCol)
				{
				scrollChanged = 1;
				g->grid.scrollCol = leftNCol;
				}
			midCol = leftNCol;
			midWidth = maxWidth;
			midNCol = colCount - leftNCol - rightNCol;
			if (g->grid.debugLevel)
				fprintf(stderr, "XmLGrid: HorizLayout everything fits\n");
			}
		else
			{
			if (g->grid.scrollCol < leftNCol)
				{
				scrollChanged = 1;
				g->grid.scrollCol = leftNCol;
				if (g->grid.debugLevel)
					fprintf(stderr, "XmLGrid: HorizLayout scroll < leftCol\n");
				}
			if (g->grid.scrollCol > maxCol)
				{
				if (g->grid.debugLevel)
					fprintf(stderr, "XmLGrid: HorizLayout scroll > maxCol\n");
				scrollChanged = 1;
				g->grid.scrollCol = maxCol;
				}
			width = st2;
			midNCol = 0;
			for (i = g->grid.scrollCol; i < colCount - rightNCol; i++)
				{
				midNCol++;
				width += GetColWidth(g, i);
				if (width >= maxWidth)
					break;
				}
			needsSB = 1;
			midCol = g->grid.scrollCol;
			midWidth = maxWidth;
			}
		}
	rightX = midX + midWidth;
	if (rightNCol)
		rightX += g->grid.rightFixedMargin;
	for (i = 0; i < 9; i += 3)
		{
		reg[i].x = 0;
		reg[i].width = leftWidth;
		reg[i].col = 0;
		reg[i].ncol = leftNCol;
		}
	for (i = 1; i < 9; i += 3)
		{
		reg[i].x = midX;
		reg[i].width = midWidth;
		reg[i].col = midCol;
		reg[i].ncol = midNCol;
		}
	for (i = 2; i < 9; i += 3)
		{
		reg[i].x = rightX;
		reg[i].width = rightWidth;
		reg[i].col = rightCol;
		reg[i].ncol = rightNCol;
		}
	if (g->grid.debugLevel)
		{
		fprintf(stderr, "XmLGrid: HorizLayout TOP x %d w %d c %d nc %d\n",
			reg[0].x, reg[0].width, reg[0].col, reg[0].ncol);
		fprintf(stderr, "XmLGrid: HorizLayout MID x %d w %d c %d nc %d\n",
			reg[1].x, reg[1].width, reg[1].col, reg[1].ncol);
		fprintf(stderr, "XmLGrid: HorizLayout BOT x %d w %d c %d nc %d\n",
			reg[2].x, reg[2].width, reg[2].col, reg[2].ncol);
		}
	if (needsSB)
		{
		if (g->grid.debugLevel)
			fprintf(stderr, "XmLGrid: HorizLayout set sb values\n");
		if (!XtIsManaged(g->grid.hsb))
			XtManageChild(g->grid.hsb);
		maxPos = PosToVisPos(g, maxCol, 0);
		sliderSize = PosToVisPos(g, colCount - rightNCol - 1, 0) - maxPos + 1;
		XtVaSetValues(g->grid.hsb,
			XmNminimum, PosToVisPos(g, leftNCol, 0),
			XmNmaximum, maxPos + sliderSize,
			XmNsliderSize, sliderSize,
			XmNpageIncrement, sliderSize,
			XmNvalue, PosToVisPos(g, g->grid.scrollCol, 0),
			NULL);
		}
	else if (g->grid.hsPolicy == XmCONSTANT &&
		g->grid.hsbDisplayPolicy == XmSTATIC)
		{
		if (!XtIsManaged(g->grid.hsb))
			XtManageChild(g->grid.hsb);
		if (g->grid.debugLevel)
			fprintf(stderr, "XmLGrid: HorizLayout hsb not needed - static\n");
		XtVaSetValues(g->grid.hsb,
			XmNminimum, 0,
			XmNmaximum, 1,
			XmNsliderSize, 1,
			XmNpageIncrement, 1,
			XmNvalue, 0,
			NULL);
		}
	else
		{
		if (XtIsManaged(g->grid.hsb))
			XtUnmanageChild(g->grid.hsb);
		}
	if (needsResize && resizeIfNeeded && newWidth != g->core.width)
		{
		if (newWidth < 1)
			newWidth = 1;
		req.request_mode = CWWidth;
		req.width = newWidth;
		if (g->grid.debugLevel)
			fprintf(stderr, "XmLGrid: HorizLayout Req w %d\n", (int)newWidth);
		XtMakeGeometryRequest((Widget)g, &req, NULL);
		PlaceScrollbars(g);
		}
	if (scrollChanged)
		DrawArea(g, DrawHScroll, 0, 0);
	TextAction(g, TEXT_SHOW);
	cCol = g->grid.scrollCol - g->grid.headingColCount;
	if (cCol != g->grid.cScrollCol)
		{
		g->grid.cScrollCol = cCol;
		cbs.reason = XmCR_SCROLL_COLUMN;
		cbs.columnType = XmCONTENT;
		cbs.column = cCol;
		XtCallCallbackList((Widget)g, g->grid.scrollCallback, (XtPointer)&cbs);
		}
	}

static void RecalcVisPos(g, isVert)
XmLGridWidget g;
int isVert;
	{
	XmLGridRow row;
	XmLGridColumn col;
	int i, count, visPos;

	if (g->grid.layoutFrozen == True)
			XmLWarning((Widget)g, "RecalcVisPos() - layout is frozen");
	visPos = 0;
	if (isVert)
		{
		if (!g->grid.recalcVertVisPos)
			return;
		count = XmLArrayGetCount(g->grid.rowArray);
		for (i = 0; i < count; i++)
			{
			row = (XmLGridRow)XmLArrayGet(g->grid.rowArray, i);
			XmLGridRowSetVisPos(row, visPos);
			if (!XmLGridRowIsHidden(row))
				visPos++;
			}
		g->grid.recalcVertVisPos = 0;
		}
	else
		{
		if (!g->grid.recalcHorizVisPos)
			return;
		count = XmLArrayGetCount(g->grid.colArray);
		for (i = 0; i < count; i++)
			{
			col = (XmLGridColumn)XmLArrayGet(g->grid.colArray, i);
			XmLGridColumnSetVisPos(col, visPos);
			if (!XmLGridColumnIsHidden(col))
				visPos++;
			}
		g->grid.recalcHorizVisPos = 0;
		}
	}

static int PosToVisPos(g, pos, isVert)
XmLGridWidget g;
int pos;
int isVert;
	{
	int visPos;
	XmLGridRow row;
	XmLGridColumn col;

	if (isVert)
		{
		if (!g->grid.hiddenRowCount)
			visPos = pos;
		else
			{
			RecalcVisPos(g, isVert);
			row = (XmLGridRow)XmLArrayGet(g->grid.rowArray, pos);
			if (row)
				visPos = XmLGridRowGetVisPos(row);
			else
				visPos = -1;
			}
		}
	else
		{
		if (!g->grid.hiddenColCount)
			visPos = pos;
		else
			{
			RecalcVisPos(g, isVert);
			col = (XmLGridColumn)XmLArrayGet(g->grid.colArray, pos);
			if (col)
				visPos = XmLGridColumnGetVisPos(col);
			else
				visPos = -1;
			}
		}
	if (visPos == -1)
		XmLWarning((Widget)g, "PosToVisPos() - invalid pos");
	return visPos;
	}

static int VisPosToPos(g, visPos, isVert)
XmLGridWidget g;
int visPos;
int isVert;
	{
	XmLGridRow row;
	XmLGridColumn col;
	int i1, i2, vp1, vp2, ic, mid, val, count;

	if (isVert)
		{
		if (!g->grid.hiddenRowCount)
			return visPos;
		count = XmLArrayGetCount(g->grid.rowArray);
		if (!count)
			{
			XmLWarning((Widget)g, "VisPosToPos() - called when no rows exist");
			return -1;
			}
		RecalcVisPos(g, isVert);
		row = (XmLGridRow)XmLArrayGet(g->grid.rowArray, 0);
		vp1 = XmLGridRowGetVisPos(row);
		row = (XmLGridRow)XmLArrayGet(g->grid.rowArray, count - 1);
		vp2 = XmLGridRowGetVisPos(row);
		if (visPos < vp1 || visPos > vp2)
			{
			XmLWarning((Widget)g, "VisPosToPos() - invalid Vert visPos");
			return -1;
			}
		}
	else
		{
		if (!g->grid.hiddenColCount)
			return visPos;
		count = XmLArrayGetCount(g->grid.colArray);
		if (!count)
			{
			XmLWarning((Widget)g, "VisPosToPos() - called when no cols exist");
			return -1;
			}
		RecalcVisPos(g, isVert);
		col = (XmLGridColumn)XmLArrayGet(g->grid.colArray, 0);
		vp1 = XmLGridColumnGetVisPos(col);
		col = (XmLGridColumn)XmLArrayGet(g->grid.colArray, count - 1);
		vp2 = XmLGridColumnGetVisPos(col);
		if (visPos < vp1 || visPos > vp2)
			{
			XmLWarning((Widget)g, "VisPosToPos() - invalid Horiz visPos");
			return -1;
			}
		}
	i1 = 0;
	i2 = count;
	ic = 0;
	while (1)
		{
		mid = i1 + (i2 - i1) / 2;
		if (isVert)
			{
			row = (XmLGridRow)XmLArrayGet(g->grid.rowArray, mid);
			val = XmLGridRowGetVisPos(row);
			}
		else
			{
			col = (XmLGridColumn)XmLArrayGet(g->grid.colArray, mid);
			val = XmLGridColumnGetVisPos(col);
			}
		if (visPos > val)
			i1 = mid;
		else if (visPos < val)
			i2 = mid;
		else
			break;
		if (++ic > 1000)
				return -1; /* inf loop */
		}
	return mid;
	}

static unsigned char ColPosToType(g, pos)
XmLGridWidget g;
int pos;
	{
	unsigned char type;

	if (pos < g->grid.headingColCount)
		type = XmHEADING;
	else if (pos < g->grid.headingColCount + g->grid.colCount)
		type = XmCONTENT;
	else
		type = XmFOOTER;
	return type;
	}

static int ColPosToTypePos(g, type, pos)
XmLGridWidget g;
unsigned char type;
int pos;
	{
	int c;

	if (type == XmHEADING)
		c = pos;
	else if (type == XmCONTENT)
		c = pos - g->grid.headingColCount;
	else
		c = pos - g->grid.headingColCount - g->grid.colCount;
	return c;
	}

static unsigned char RowPosToType(g, pos)
XmLGridWidget g;
int pos;
	{
	unsigned char type;

	if (pos < g->grid.headingRowCount)
		type = XmHEADING;
	else if (pos < g->grid.headingRowCount + g->grid.rowCount)
		type = XmCONTENT;
	else
		type = XmFOOTER;
	return type;
	}

static int RowPosToTypePos(g, type, pos)
XmLGridWidget g;
unsigned char type;
int pos;
	{
	int r;

	if (type == XmHEADING)
		r = pos;
	else if (type == XmCONTENT)
		r = pos - g->grid.headingRowCount;
	else
		r = pos - g->grid.headingRowCount - g->grid.rowCount;
	return r;
	}

static int ColTypePosToPos(g, type, pos, allowNeg)
XmLGridWidget g;
unsigned char type;
int pos;
int allowNeg;
	{
	if (pos < 0)
		{
		if (!allowNeg)
			return -1;
		if (type == XmHEADING)
			pos = g->grid.headingColCount;
		else if (type == XmFOOTER || type == XmALL_TYPES)
			pos = g->grid.headingColCount + g->grid.colCount +
				g->grid.footerColCount;
		else /* XmCONTENT */
			pos = g->grid.headingColCount + g->grid.colCount;
		}
	else
		{
		if (type == XmALL_TYPES)
			;
		else if (type == XmHEADING)
			{
			if (pos >= g->grid.headingColCount)
				return -1;
			}
		else if (type == XmFOOTER)
			{
			if (pos >= g->grid.footerColCount)
				return -1;
			pos += g->grid.headingColCount + g->grid.colCount;
			}
		else /* XmCONTENT */
			{
			if (pos >= g->grid.colCount)
				return -1;
			pos += g->grid.headingColCount;
			}
		}
	return pos;
	}

static int RowTypePosToPos(g, type, pos, allowNeg)
XmLGridWidget g;
unsigned char type;
int pos;
int allowNeg;
	{
	if (pos < 0)
		{
		if (!allowNeg)
			return -1;
		if (type == XmHEADING)
			pos = g->grid.headingRowCount;
		else if (type == XmFOOTER || type == XmALL_TYPES)
			pos = g->grid.headingRowCount + g->grid.rowCount +
					g->grid.footerRowCount;
		else /* XmCONTENT */
			pos = g->grid.headingRowCount + g->grid.rowCount;
		}
	else
		{
		if (type == XmALL_TYPES)
			;
		else if (type == XmHEADING)
			{
			if (pos >= g->grid.headingRowCount)
				return -1;
			}
		else if (type == XmFOOTER)
			{
			if (pos >= g->grid.footerRowCount)
				return -1;
			pos += g->grid.headingRowCount + g->grid.rowCount;
			}
		else /* XmCONTENT */
			{
			if (pos >= g->grid.rowCount)
				return -1;
			pos += g->grid.headingRowCount;
			}
		}
	return pos;
	}

int ScrollRowBottomPos(g, row)
XmLGridWidget g;
int row;
	{
	int r, h, height;

	if (g->grid.vsPolicy == XmVARIABLE)
		return -1;
	height = g->grid.reg[4].height;
	h = 0;
	r = row;
	while (r >= 0)
		{
		h += GetRowHeight(g, r);
		if (h > height)
			break;
		r--;
		}
	if (r != row)
		r++;
	return r;
	}

int ScrollColRightPos(g, col)
XmLGridWidget g;
int col;
	{
	int c, w, width;

	if (g->grid.hsPolicy == XmVARIABLE)
		return -1;
	width = g->grid.reg[4].width;
	w = 0;
	c = col;
	while (c >= 0)
		{
		w += GetColWidth(g, c);
		if (w >= width)
			break;
		c--;
		}
	if (c != col)
		c++;
	return c;
	}

static int PosIsResize(g, x, y, row, col, isVert)
XmLGridWidget g;
int x, y;
int *row, *col;
int *isVert;
	{
	XRectangle rect;
	int i, nx, ny, margin;

	/* check for bottom resize */
	for (i = 0; i < 2; i++)
		{
		if (g->grid.allowRowResize != True)
			continue;
		nx = x;
		ny = y;
		if (i)
			ny = y - 4;
		if (XYToRowCol(g, nx, ny, row, col) == -1)
			continue;
		if (RowColToXY(g, *row, *col, &rect) == -1)
			continue;
		if (GetRowHeight(g, *row) != rect.height)
			continue;
		if (ColPosToType(g, *col) != XmHEADING)
			continue;
		margin = ny - (rect.y + rect.height);
		if (margin > -5 && margin < 5)
			{
			*isVert = 1;
			return 1;
			}
		}
	/* check for right resize */
	for (i = 0; i < 2; i++)
		{
		if (g->grid.allowColResize != True)
			continue;
		nx = x;
		ny = y;
		if (i)
			nx = x - 4;
		if (XYToRowCol(g, nx, ny, row, col) == -1)
			continue;
		if (RowColToXY(g, *row, *col, &rect) == -1)
			continue;
		if (GetColWidth(g, *col) != rect.width)
			continue;
		if (RowPosToType(g, *row) != XmHEADING)
			continue;
		margin = nx - (rect.x + rect.width);
		if (margin > -5 && margin < 5)
			{
			*isVert = 0;
			return 1;
			}
		}
	return 0;
	}

int XmLGridPosIsResize(g, x, y)
Widget g;
int x, y;
{
  int row, col;
  int isVert;
  return PosIsResize((XmLGridWidget) g, x, y, &row, &col, &isVert);
}


static int XYToRowCol(g, x, y, row, col)
XmLGridWidget g;
int x, y;
int *row, *col;
	{
	XRectangle xyRect, rect;
	GridReg *reg;
	int i, r, c;
	int width, height;
	int st;

	reg = g->grid.reg;
	st = g->manager.shadow_thickness;
	xyRect.x = x;
	xyRect.y = y;
	xyRect.width = 1;
	xyRect.height = 1;
	for (i = 0; i < 9; i++)
		{
		if (!reg[i].width || !reg[i].height)
			continue;
		rect.x = reg[i].x + st;
		rect.y = reg[i].y + st;
		rect.width = reg[i].width - st * 2;
		rect.height = reg[i].height - st * 2;
		if (XmLRectIntersect(&xyRect, &rect) == XmLRectOutside)
			continue;
		height = 0;
		for (r = reg[i].row; r < reg[i].row + reg[i].nrow; r++)
			{
			width = 0;
			for (c = reg[i].col; c < reg[i].col + reg[i].ncol; c++)
				{
				rect.x = reg[i].x + st + width;
				rect.y = reg[i].y + st + height;
				rect.width = GetColWidth(g, c);
				rect.height = GetRowHeight(g, r);
				ClipRectToReg(g, &rect, &reg[i]);
				if (XmLRectIntersect(&xyRect, &rect) != XmLRectOutside)
					{
					if (!RowColFirstSpan(g, r, c, row, col, 0, True, True))
						return 0; /* SUCCESS */
					else
						return -1;
					}
				width += GetColWidth(g, c);
				}
			height += GetRowHeight(g, r);
			}
		}
	return -1;
	}

static int RowColToXY(g, row, col, rect)
XmLGridWidget g;
int row, col;
XRectangle *rect;
	{
	XmLGridCell cell;
	XmLGridCellRefValues *values;
	GridReg *reg;
	Dimension st;
	int i, r, c, off;

	reg = g->grid.reg;
	st = g->manager.shadow_thickness;
	cell = GetCell(g, row, col);
	if (!cell)
		return -1;
	values = XmLGridCellGetRefValues(cell);
	if (!values) return -1;
	for (i = 0; i < 9; i++)
		{
		if (!reg[i].width || !reg[i].height)
			continue;
		if (!(col >= reg[i].col &&
			col < reg[i].col + reg[i].ncol &&
			row >= reg[i].row &&
			row < reg[i].row + reg[i].nrow))
			continue;
		off = 0;
		for (r = reg[i].row; r < row; r++)
			off += GetRowHeight(g, r);
		rect->y = reg[i].y + st + off;
		rect->height = GetRowHeight(g, row);
		if (values->rowSpan)
			for (r = row + 1; r <= row + values->rowSpan; r++)
				rect->height += GetRowHeight(g, r);
		off = 0;
		for (c = reg[i].col; c < col; c++)
			off += GetColWidth(g, c);
		rect->x = reg[i].x + st + off;
		rect->width = GetColWidth(g, col);
		if (values->columnSpan)
			for (c = col + 1; c <= col + values->columnSpan; c++)
				rect->width += GetColWidth(g, c);
		ClipRectToReg(g, rect, &reg[i]);
		return 0;
		}
	return -1;
	}

static int RowColFirstSpan(g, row, col, firstRow, firstCol, rect,
	lookLeft, lookUp)
XmLGridWidget g;
int row, col;
int *firstRow, *firstCol;
XRectangle *rect;
Boolean lookLeft, lookUp;
	{
	XmLGridCell cell;
	int done;

	done = 0;
	while (!done)
		{
		cell = GetCell(g, row, col);
		if (!cell)
			return -1;
		if (XmLGridCellInRowSpan(cell) == True)
			{
			if (lookUp == True)
				{
				row--;
				if (rect)
					rect->y -= GetRowHeight(g, row);
				}
			else
				row = -1;
			if (row < 0)
				done = 1;
			}
		else if (XmLGridCellInColumnSpan(cell) == True)
			{
			if (lookLeft == True)
				{
				col--;
				if (rect)
					rect->x -= GetColWidth(g, col);
				}
			else
				col = -1;
			if (col < 0)
				done = 1;
			}
		else
			done = 1;
		}
	if (row < 0 || col < 0)
		return -1;
	*firstRow = row;
	*firstCol = col;
	return 0;
	}

static void RowColSpanRect(g, row, col, rect)
XmLGridWidget g;
int row, col;
XRectangle *rect;
	{
	XmLGridCell cell;
	XmLGridCellRefValues *cellValues;
	int r, c;

	cell = GetCell(g, row, col);
	cellValues = XmLGridCellGetRefValues(cell);
	rect->width = 0;
	rect->height = 0;
	for (r = row; r <= row + cellValues->rowSpan; r++)
		rect->height += GetRowHeight(g, r);
	for (c = col; c <= col + cellValues->columnSpan; c++)
		rect->width += GetColWidth(g, c);
	}

XmLGridCell GetCell(g, row, col)
XmLGridWidget g;
int row, col;
	{
	XmLGridRow rowp;

	rowp = (XmLGridRow)XmLArrayGet(g->grid.rowArray, row);
	if (!rowp)
		return 0;
	return (XmLGridCell)XmLArrayGet(XmLGridRowCells(rowp), col);
	}

int GetColWidth(g, col)
XmLGridWidget g;
int col;
	{
	XmLGridColumn colp;

	colp = (XmLGridColumn)XmLArrayGet(g->grid.colArray, col);
	if (!colp)
		return 0;
	return XmLGridColumnGetWidthInPixels(colp);
	}

int GetRowHeight(g, row)
XmLGridWidget g;
int row;
	{
	XmLGridRow rowp;

	rowp = (XmLGridRow)XmLArrayGet(g->grid.rowArray, row);
	if (!rowp)
		return 0;
	return XmLGridRowGetHeightInPixels(rowp);
	}

static int ColIsVisible(g, col)
XmLGridWidget g;
int col;
	{
	XmLGridColumn c;
	int i, c1, c2;

	c = (XmLGridColumn)XmLArrayGet(g->grid.colArray, col);
	if (!c)
		{
		XmLWarning((Widget)g, "ColumnIsVisible() - invalid column");
		return -1;
		}
	if (XmLGridColumnIsHidden(c) == True)
		return 0;
	if (g->grid.hsPolicy != XmCONSTANT)
		return 1;
	for (i = 0; i < 3; i ++)
		{
		c1 = g->grid.reg[i].col;
		c2 = c1 + g->grid.reg[i].ncol;
		if (col >= c1 && col < c2)
			return 1;
		}
	return 0;
	}

static int RowIsVisible(g, row)
XmLGridWidget g;
int row;
	{
	XmLGridRow r;
	int i, r1, r2;

	r = (XmLGridRow)XmLArrayGet(g->grid.rowArray, row);
	if (!r)
		{
		XmLWarning((Widget)g, "RowIsVisible() - invalid row");
		return -1;
		}
	if (XmLGridRowIsHidden(r) == True)
		return 0;
	if (g->grid.vsPolicy != XmCONSTANT)
		return 1;
	for (i = 0; i < 9; i += 3)
		{
		r1 = g->grid.reg[i].row;
		r2 = r1 + g->grid.reg[i].nrow;
		if (row >= r1 && row < r2)
			return 1;
		}
	return 0;
	}

static XtGeometryResult GeometryManager(w, request, allow)
Widget w;
XtWidgetGeometry *request, *allow;
	{
	if (request->request_mode & CWWidth)
		w->core.width = request->width;
	if (request->request_mode & CWHeight)
		w->core.height = request->height;
	if (request->request_mode & CWX)
		w->core.x = request->x;
	if (request->request_mode & CWY)
		w->core.y = request->y;
	if (request->request_mode & CWBorderWidth)
		w->core.border_width = request->border_width;
	return XtGeometryYes;
	}

static void ScrollCB(w, clientData, callData)
Widget w;
XtPointer clientData, callData;
	{
	XmLGridWidget g;
	XmScrollBarCallbackStruct *cbs;
	unsigned char orientation;
	int visPos;

	g = (XmLGridWidget)(XtParent(w));
	cbs = (XmScrollBarCallbackStruct *)callData;
	visPos = cbs->value;
	XtVaGetValues(w, XmNorientation, &orientation, NULL);
	if (orientation == XmVERTICAL)
		{
		if (visPos == PosToVisPos(g, g->grid.scrollRow, 1))
			return;
		g->grid.scrollRow = VisPosToPos(g, visPos, 1);
		VertLayout(g, 0);
		DrawArea(g, DrawVScroll, 0, 0);
		}
	else
		{
		if (visPos == PosToVisPos(g, g->grid.scrollCol, 0))
			return;
		g->grid.scrollCol = VisPosToPos(g, visPos, 0);
		HorizLayout(g, 0);
		DrawArea(g, DrawHScroll, 0, 0);
		}
	}

static int FindNextFocus(g, row, col, rowDir, colDir, nextRow, nextCol)
XmLGridWidget g;
int row, col;
int rowDir, colDir;
int *nextRow, *nextCol;
	{
	int traverse;
	int hrc, hcc, rc, cc;
	XmLGridColumn colp;
	XmLGridRow rowp;
	XmLGridCell cell;

	hcc = g->grid.headingColCount;
	cc = g->grid.colCount;
	hrc = g->grid.headingRowCount;
	rc = g->grid.rowCount;
	if (!rc || !cc)
		return -1;
	if (col < hcc)
		col = hcc;
	else if (col >= hcc + cc)
		col = hcc + cc - 1;
	if (row < hrc)
		row = hrc;
	else if (row >= hrc + rc)
		row = hrc + rc - 1;
	traverse = 0;
	while (1)
		{
		if (row < hrc || row >= hrc + rc)
			break;
		if (col < hcc || col >= hcc + cc)
			break;
		rowp = (XmLGridRow)XmLArrayGet(g->grid.rowArray, row);
		colp = (XmLGridColumn)XmLArrayGet(g->grid.colArray, col);
		cell = GetCell(g, row, col);
		if (cell &&
			RowPosToType(g, row) == XmCONTENT &&
			ColPosToType(g, col) == XmCONTENT &&
			XmLGridCellInRowSpan(cell) == False &&
			XmLGridCellInColumnSpan(cell) == False &&
			XmLGridRowIsHidden(rowp) == False &&
			XmLGridColumnIsHidden(colp) == False)
			{
			traverse = 1;
			break;
			}
		if (!rowDir && !colDir)
			break;
		row += rowDir;
		col += colDir;
		}
	if (!traverse)
		return -1;
	*nextRow = row;
	*nextCol = col;
	return 0;
	}

static int SetFocus(g, row, col, rowDir, colDir)
XmLGridWidget g;
int row, col;
int rowDir, colDir;
	{
	if (FindNextFocus(g, row, col, rowDir, colDir, &row, &col) == -1)
		return -1;
	ChangeFocus(g, row, col);
	return 0;
	}

static void ChangeFocus(g, row, col)
XmLGridWidget g;
int row, col;
	{
	XmLGridCell cell;
	XmLGridCallbackStruct cbs;
	int focusRow, focusCol;

	focusRow = g->grid.focusRow;
	focusCol = g->grid.focusCol;
	g->grid.focusRow = row;
	g->grid.focusCol = col;
	if (focusRow != -1 && focusCol != -1)
		{
		cell = GetCell(g, focusRow, focusCol);
		if (cell)
			{
			DrawArea(g, DrawCell, focusRow, focusCol);
			cbs.reason = XmCR_CELL_FOCUS_OUT;
			cbs.columnType = ColPosToType(g, focusCol);
			cbs.column = ColPosToTypePos(g, cbs.columnType, focusCol);
			cbs.rowType = RowPosToType(g, focusRow);
			cbs.row = RowPosToTypePos(g, cbs.rowType, focusRow);
			XmLGridCellAction(cell, (Widget)g, &cbs);
			XtCallCallbackList((Widget)g, g->grid.cellFocusCallback,
				(XtPointer)&cbs);
			}
		}
	if (row != -1 && col != -1)
		{
		cell = GetCell(g, row, col);
		if (cell)
			{
			DrawArea(g, DrawCell, row, col);
			cbs.reason = XmCR_CELL_FOCUS_IN;
			cbs.columnType = ColPosToType(g, col);
			cbs.column = ColPosToTypePos(g, cbs.columnType, col);
			cbs.rowType = RowPosToType(g, row);
			cbs.row = RowPosToTypePos(g, cbs.rowType, row);
			XmLGridCellAction(cell, (Widget)g, &cbs);
			XtCallCallbackList((Widget)g, g->grid.cellFocusCallback,
				(XtPointer)&cbs);
			}
		else
			{
			if (!XmLArrayGet(g->grid.rowArray, row))
				g->grid.focusRow = -1;
			if (!XmLArrayGet(g->grid.colArray, col))
				g->grid.focusCol = -1;
			}
		}
	}
	
static void MakeColVisible(g, col)
XmLGridWidget g;
int col;
	{
	int st, width, scrollWidth, scrollCol, prevScrollCol;

	if (g->grid.hsPolicy != XmCONSTANT)
		return;
	if (col < g->grid.leftFixedCount ||
		col >= XmLArrayGetCount(g->grid.colArray) - g->grid.rightFixedCount)
		return;
	st = g->manager.shadow_thickness;
	scrollCol = col;
	if (col > g->grid.scrollCol)
		{
		scrollWidth = g->grid.reg[4].width - st * 2;
		width = 0;
		while (1)
			{
			width += GetColWidth(g, scrollCol);
			if (width > scrollWidth)
				break;
			if (scrollCol < g->grid.leftFixedCount)
				break;
			scrollCol--;
			}
		scrollCol++;
		if (scrollCol > col)
			scrollCol = col;
		/* only scroll if needed */
		if (scrollCol < g->grid.scrollCol)
			scrollCol = g->grid.scrollCol;
		}
	if (scrollCol == g->grid.scrollCol)
		return;
	prevScrollCol = g->grid.scrollCol;
	g->grid.scrollCol = scrollCol;
	HorizLayout(g, 0);
	if (g->grid.scrollCol != prevScrollCol)
		DrawArea(g, DrawHScroll, 0, 0);
	}

static void MakeRowVisible(g, row)
XmLGridWidget g;
int row;
	{
	int st, height, scrollHeight, scrollRow, prevScrollRow;

	if (g->grid.vsPolicy != XmCONSTANT)
		return;
	if (row < g->grid.topFixedCount ||
		row >= XmLArrayGetCount(g->grid.rowArray) - g->grid.bottomFixedCount)
		return;
	st = g->manager.shadow_thickness;
	scrollRow = row;
	if (row > g->grid.scrollRow)
		{
		scrollHeight = g->grid.reg[4].height - st * 2;
		height = 0;
		while (1)
			{
			height += GetRowHeight(g, scrollRow);
			if (height > scrollHeight)
				break;
			if (scrollRow < g->grid.topFixedCount)
				break;
			scrollRow--;
			}
		scrollRow++;
		if (scrollRow > row)
			scrollRow = row;
		/* only scroll if needed */
		if (scrollRow < g->grid.scrollRow)
			scrollRow = g->grid.scrollRow;
		}
	if (scrollRow == g->grid.scrollRow)
		return;
	prevScrollRow = g->grid.scrollRow;
	g->grid.scrollRow = scrollRow;
	VertLayout(g, 0);
	if (g->grid.scrollRow != prevScrollRow)
		DrawArea(g, DrawVScroll, 0, 0);
	}

static void TextAction(g, action)
XmLGridWidget g;
int action;
	{
	int row, col, ret, isVisible;
	XRectangle rect;
	XtTranslations trans;
	WidgetClass wc;
	XmLGridCell cellp;
	XmLGridCallbackStruct cbs;

	if (!XtIsRealized(g->grid.text))
		return;
	row = g->grid.focusRow;
	col = g->grid.focusCol;
	if (row == -1 || col == -1)
		return;
	isVisible = 0;
	if (RowColToXY(g, row, col, &rect) != -1)
		isVisible = 1;
	cellp = GetCell(g, row, col);
	if (!cellp)
		return;

	switch (action)
		{
		case TEXT_EDIT:
		case TEXT_EDIT_INSERT:
			{
			if (g->grid.inEdit || !isVisible)	
				return;
			g->grid.inEdit = 1;
			if (action == TEXT_EDIT)
				cbs.reason = XmCR_EDIT_BEGIN;
			else
				cbs.reason = XmCR_EDIT_INSERT;
			cbs.rowType = XmCONTENT;
			cbs.row = RowPosToTypePos(g, XmCONTENT, row);
			cbs.columnType = XmCONTENT;
			cbs.column = ColPosToTypePos(g, XmCONTENT, col);
			cbs.clipRect = &rect;
			ret = XmLGridCellAction(cellp, (Widget)g, &cbs);
			if (ret)
				{
				XtConfigureWidget(g->grid.text, rect.x, rect.y,
						rect.width, rect.height, 0);
				wc = g->grid.text->core.widget_class;
				trans = (XtTranslations)wc->core_class.tm_table;
				XtVaSetValues(g->grid.text, XmNtranslations, trans, NULL);
				XtOverrideTranslations(g->grid.text, g->grid.editTrans);
				XtMapWidget(g->grid.text);
				g->grid.textHidden = 0;
				XtCallCallbackList((Widget)g, g->grid.editCallback,
					(XtPointer)&cbs);
				}
			else
				g->grid.inEdit = 0;
			break;
			}
		case TEXT_EDIT_CANCEL:
		case TEXT_EDIT_COMPLETE:
			{
			if (!g->grid.inEdit)
				return;
			if (action == TEXT_EDIT_COMPLETE)
				cbs.reason = XmCR_EDIT_COMPLETE;
			else
				cbs.reason = XmCR_EDIT_CANCEL;
			cbs.rowType = XmCONTENT;
			cbs.row = RowPosToTypePos(g, XmCONTENT, row);
			cbs.columnType = XmCONTENT;
			cbs.column = ColPosToTypePos(g, XmCONTENT, col);
			XmLGridCellAction(cellp, (Widget)g, &cbs);
			wc = g->grid.text->core.widget_class;
			trans = (XtTranslations)wc->core_class.tm_table;
			XtVaSetValues(g->grid.text, XmNtranslations, trans, NULL);
			XtOverrideTranslations(g->grid.text, g->grid.traverseTrans);
			XtCallCallbackList((Widget)g, g->grid.editCallback,
				(XtPointer)&cbs);
			XmTextSetString(g->grid.text, "");
			XmTextSetInsertionPosition(g->grid.text, 0);
			g->grid.inEdit = 0;
			XtUnmapWidget(g->grid.text);
			g->grid.textHidden = 1;
			XtConfigureWidget(g->grid.text, 0, 0, 30, 10, 0);
			break;
			}
		case TEXT_HIDE:
			{
			if (g->grid.textHidden || !isVisible)
				return;
			XtUnmapWidget(g->grid.text);
			g->grid.textHidden = 1;
			break;
			}
		case TEXT_SHOW:
			{
			if (!g->grid.textHidden || !g->grid.inEdit || !isVisible)
				return;
			if (rect.width == 0 || rect.height == 0)
				TextAction(g, TEXT_EDIT_CANCEL);
			else
				{
				XtConfigureWidget(g->grid.text, rect.x, rect.y,
					rect.width, rect.height, 0);
				XtMapWidget(g->grid.text);
				g->grid.textHidden = 0;
				}
			break;
			}
		}
	}

/*
   Getting and Setting Values
*/

static void GetSubValues(w, args, nargs)
Widget w;
ArgList args;
Cardinal *nargs;
	{
	XmLGridWidget g;
	int i, c, n;
	long mask, totalMask;
	XmLGridRow row;
	XmLGridColumn col;
	XmLGridCell cell;

	g = (XmLGridWidget)w;
	totalMask = 0;
	row = 0;
	col = 0;
	n = 0;
	for (i = 0; i < *nargs; i++)
		{
		if (!args[i].name)
			continue;
		if (!strncmp(args[i].name, "row", 3))
			{
			n++;
			if (!args[i].name[3])
				{
				XmLWarning(w, "GetValues() - can't use XmNrow");
				continue;
				}
			if (!strncmp(&args[i].name[3], "Range", 5))
				{
				XmLWarning(w, "GetValues() - can't use XmNrowRange");
				continue;
				}
			if (!strcmp(args[i].name, XmNrowPtr))
				{
				row = (XmLGridRow)args[i].value;
				continue;
				}
			if (!row)
				continue;
			mask = 0;
			RowValueStringToMask(args[i].name, &mask);
			totalMask |= mask;
			GetRowValue(row, args[i].value, mask);
			}
		else if (!strncmp(args[i].name, "column", 6))
			{
			n++;
			if (!args[i].name[6])
				{
				XmLWarning(w, "GetValues() - can't use XmNcolumn");
				continue;
				}
			if (!strcmp(args[i].name, XmNcolumnPtr))
				{
				col = (XmLGridColumn)args[i].value;
				continue;
				}
			if (!col)
				continue;
			mask = 0;
			ColumnValueStringToMask(args[i].name, &mask);
			totalMask |= mask;
			GetColumnValue(col, args[i].value, mask);
			}
		else if (!strncmp(args[i].name, "cell", 4))
			{
			n++;
			if (!row || !col)
				continue;
			c = XmLGridColumnGetPos(col);
			cell = (XmLGridCell)XmLArrayGet(XmLGridRowCells(row), c);
			mask = 0;
			CellValueStringToMask(args[i].name, &mask);
			totalMask |= mask;
			GetCellValue(cell, args[i].value, mask);
			}
		}
	if (totalMask && n != *nargs)
		XmLWarning(w, "GetValues() - resource(s) not valid for row/column");
	}

static void SetSubValues(w, args, nargs)
Widget w;
ArgList args;
Cardinal *nargs;
	{
	XmLGridWidget g;
	XmLGridRow row;
	XmLGridColumn col;
	XmLGridCell cell;
	XmLGridCellRefValues *values, *newValues, *prevValues;
	XmLArray *cellArray;
	int r, r1, r2, rowsValid;
	int c, c1, c2, colsValid;
	unsigned char rowType, colType;
	long rowMask, colMask, cellMask;
	int i, n, nRefValues;
	int needsHorizResize, needsVertResize;

	g = (XmLGridWidget)w;
	needsHorizResize = 0;
	needsVertResize = 0;
	rowMask = 0;
	colMask = 0;
	cellMask = 0;
	n = 0;
	for (i = 0; i < *nargs; i++)
		{
		if (!args[i].name)
			continue;
		if (!strncmp(args[i].name, "row", 3))
			{
			n++;
			if (!strcmp(args[i].name, XmNrowPtr))
				{
				XmLWarning(w, "SetValues() - can't use XmNrowPtr");
				continue;
				}
			RowValueStringToMask(args[i].name, &rowMask);
			}
		else if (!strncmp(args[i].name, "column", 6))
			{
			n++;
			if (!strcmp(args[i].name, XmNcolumnPtr))
				{
				XmLWarning(w, "SetValues() - can't use XmNcolumnPtr");
				continue;
				}
			ColumnValueStringToMask(args[i].name, &colMask);
			}
		else if (!strncmp(args[i].name, "cell", 4))
			{
			n++;
			CellValueStringToMask(args[i].name, &cellMask);
			}
		}
	if (rowMask || colMask || cellMask)
		{
		if (n != *nargs)
			XmLWarning(w, "SetValues() - invalid resource(s) for row/column");
		rowType = g->grid.rowType;
		rowsValid = 1;
		if (g->grid.cellRowRangeStart != -1 && g->grid.cellRowRangeEnd != -1)
			{
			r1 = RowTypePosToPos(g, rowType, g->grid.cellRowRangeStart, 0);
			r2 = RowTypePosToPos(g, rowType, g->grid.cellRowRangeEnd, 0);
			if (r1 < 0 || r2 < 0 || r1 > r2)
				rowsValid = 0;
			r2++;
			}
		else if (g->grid.cellRow != -1)
			{
			r1 = RowTypePosToPos(g, rowType, g->grid.cellRow, 0);
			if (r1 == -1)
				rowsValid = 0;
			r2 = r1 + 1;
			}
		else
			{
			r1 = RowTypePosToPos(g, rowType, 0, 0);
			if (r1 == -1)
				r1 = 0;
			r2 = RowTypePosToPos(g, rowType, -1, 1);
			}
		if (!rowsValid)
			{
			XmLWarning(w, "SetValues() - invalid row(s) specified");
			r1 = 0;
			r2 = 0;
			}
		colType = g->grid.colType;
		colsValid = 1;
		if (g->grid.cellColRangeStart != -1 && g->grid.cellColRangeEnd != -1)
			{
			c1 = ColTypePosToPos(g, colType, g->grid.cellColRangeStart, 0);
			c2 = ColTypePosToPos(g, colType, g->grid.cellColRangeEnd, 0);
			if (c1 < 0 || c2 < 0 || c1 > c2)
				colsValid = 0;
			c2++;
			}
		else if (g->grid.cellCol != -1)
			{
			c1 = ColTypePosToPos(g, colType, g->grid.cellCol, 0);
			if (c1 == -1)
				colsValid = 0;
			c2 = c1 + 1;
			}
		else
			{
			c1 = ColTypePosToPos(g, colType, 0, 0);
			if (c1 == -1)
				c1 = 0;
			c2 = ColTypePosToPos(g, colType, -1, 1);
			}
		if (!colsValid)
			{
			XmLWarning(w, "SetValues() - invalid column(s) specified");
			c1 = 0;
			c2 = 0;
			}
		if (g->grid.debugLevel)
			{
			fprintf(stderr, "XmLGrid: SetValues for rows %d to %d", r1, r2);
			fprintf(stderr, "XmLGrid: & columns %d to %d", c1, c2);
			}
		if (rowMask)
			for (r = r1; r < r2; r += g->grid.rowStep)
				{
				row = (XmLGridRow)XmLArrayGet(g->grid.rowArray, r);
				if (!row)
					continue;
				if (SetRowValues(g, row, rowMask))
					needsVertResize = 1;
				DrawArea(g, DrawRow, r, 0);
				}
		if (colMask)
			for (c = c1; c < c2; c += g->grid.colStep)
				{
				col = (XmLGridColumn)XmLArrayGet(g->grid.colArray, c);
				if (!col)
					continue;
				if (SetColumnValues(g, col, colMask))
					needsHorizResize = 1;
				DrawArea(g, DrawCol, 0, c);
				}
		if (cellMask && g->grid.cellDefaults == True)
			{
			SetCellValuesPreprocess(g, cellMask);
			newValues = CellRefValuesCreate(g, g->grid.defCellValues);
			newValues->refCount = 1;
			SetCellRefValues(g, newValues, cellMask);
			if (newValues->rowSpan || newValues->columnSpan)
				{
				newValues->rowSpan = 0;
				newValues->columnSpan = 0;
				XmLWarning((Widget)g,
					"SetValues() - cannot set default cell spans");
				}
			XmLGridCellDerefValues(g->grid.defCellValues);
			g->grid.defCellValues = newValues;
			}
		if (cellMask && g->grid.cellDefaults != True)
			{
			SetCellValuesPreprocess(g, cellMask);
			if (SetCellHasRefValues(cellMask))
				{
				cellArray = XmLArrayNew();
				XmLArrayAdd(cellArray, -1, (r2 - r1) * (c2 - c1));
				nRefValues = 0;
				for (r = r1; r < r2; r += g->grid.rowStep)
					for (c = c1; c < c2; c += g->grid.colStep)
						{
						cell = GetCell(g, r, c);
						if (!cell)
							continue;
						SetCellRefValuesPreprocess(g, r, c, cell, cellMask);
						XmLArraySet(cellArray, nRefValues, cell);
						nRefValues++;
						}
				XmLArraySort(cellArray, SetCellRefValuesCompare,
					(void *)&cellMask, 0, nRefValues);
				prevValues = 0;
				for (i = 0; i < nRefValues; i++)
					{
					cell = (XmLGridCell)XmLArrayGet(cellArray, i);
					values = XmLGridCellGetRefValues(cell);
					if (values != prevValues)
						{
						newValues = CellRefValuesCreate(g, values);
						SetCellRefValues(g, newValues, cellMask);
						}
					XmLGridCellSetRefValues(cell, newValues);
					prevValues = values;
					}
				XmLArrayFree(cellArray);
				}
			for (r = r1; r < r2; r += g->grid.rowStep)
				for (c = c1; c < c2; c += g->grid.colStep)
					{
					row = (XmLGridRow)XmLArrayGet(g->grid.rowArray, r);
					col = (XmLGridColumn)XmLArrayGet(g->grid.colArray, c);
					cell = GetCell(g, r, c);
					if (!row || !col || !cell)
						continue;
					if (SetCellValuesNeedsResize(g, row, col, cell, cellMask))
						{
						needsHorizResize = 1;
						needsVertResize = 1;
						}
					SetCellValues(g, cell, cellMask);
					if (!needsHorizResize && !needsVertResize)
						DrawArea(g, DrawCell, r, c);
					}
			}
		}
	if (needsHorizResize)
		HorizLayout(g, 1);
	if (needsVertResize)
		VertLayout(g, 1);
	if (needsHorizResize || needsVertResize)
		DrawArea(g, DrawAll, 0, 0);
	g->grid.rowType = XmCONTENT;
	g->grid.colType = XmCONTENT;
	g->grid.rowStep = 1;
	g->grid.colStep = 1;
	g->grid.cellRow = -1;
	g->grid.cellRowRangeStart = -1;
	g->grid.cellRowRangeEnd = -1;
	g->grid.cellCol = -1;
	g->grid.cellColRangeStart = -1;
	g->grid.cellColRangeEnd = -1;
	g->grid.cellDefaults = False;
	}

static Boolean SetValues(curW, reqW, newW, args, nargs)
Widget curW, reqW, newW;
ArgList args;
Cardinal *nargs;
	{
	XmLGridWidget g;
	XmLGridWidget cur;
	int needsVertLayout, needsHorizLayout;
	int needsRedraw, needsPlaceSB;
	XmLGridCellRefValues *cellValues;
 
	g = (XmLGridWidget)newW;
	cur = (XmLGridWidget)curW;
	needsRedraw = 0;
	needsVertLayout = 0;
	needsHorizLayout = 0;
	needsPlaceSB = 0;

#define NE(value) (g->value != cur->value)
	if (NE(grid.cScrollCol))
		{
		g->grid.scrollCol = g->grid.cScrollCol + g->grid.headingColCount;
		needsHorizLayout = 1;
		needsRedraw = 1;
		}
	if (NE(grid.cScrollRow))
		{
		g->grid.scrollRow = g->grid.cScrollRow + g->grid.headingRowCount;
		needsVertLayout = 1;
		needsRedraw = 1;
		}
	if (NE(grid.bottomFixedCount) ||
		NE(grid.bottomFixedMargin) ||
		NE(grid.topFixedCount) ||
		NE(grid.topFixedMargin) ||
		NE(grid.vsbDisplayPolicy) ||
		NE(grid.vsPolicy))
		{
		needsVertLayout = 1;
		needsRedraw = 1;
		}
	if (NE(grid.leftFixedCount) ||
		NE(grid.leftFixedMargin) ||
		NE(grid.rightFixedCount) ||
		NE(grid.rightFixedMargin) ||
		NE(grid.hsbDisplayPolicy) ||
		NE(grid.hsPolicy))
		{
		needsHorizLayout = 1;
		needsRedraw = 1;
		}
	if (NE(grid.layoutFrozen))
		{
		if (g->grid.layoutFrozen == False)
			{
			if (g->grid.needsVertLayout == True)
				{
				needsVertLayout = 1;
				g->grid.needsVertLayout = False;
				}
			if (g->grid.needsHorizLayout == True)
				{
				needsHorizLayout = 1;
				g->grid.needsHorizLayout = False;
				}
			needsRedraw = 1;
			}
		}
	if (NE(grid.scrollBarMargin) ||
		NE(manager.shadow_thickness) ||
		NE(core.border_width))
		{
		needsVertLayout = 1;
		needsHorizLayout = 1;
		needsRedraw = 1;
		}
	if (NE(grid.vsPolicy) ||
		NE(grid.hsPolicy) ||
		NE(grid.scrollBarMargin))
		needsPlaceSB = 1;
	if (NE(manager.bottom_shadow_color) ||
		NE(manager.bottom_shadow_pixmap) ||
		NE(manager.top_shadow_color) ||
		NE(manager.top_shadow_pixmap) ||
		NE(grid.shadowRegions) ||
		NE(grid.blankBg) ||
		NE(grid.selectBg) ||
		NE(grid.selectFg) ||
		NE(core.border_pixel) ||
		NE(core.border_pixmap) ||
		NE(manager.string_direction) ||
		NE(manager.traversal_on))
		needsRedraw = 1;
	if (NE(grid.fontList))
		{
		XmFontListFree(cur->grid.fontList);
		CopyFontList(g);
		cellValues = CellRefValuesCreate(g, g->grid.defCellValues);
		cellValues->refCount = 1;
		XmFontListFree(cellValues->fontList);
		cellValues->fontList = XmFontListCopy(g->grid.fontList);
		XmLFontListGetDimensions(cellValues->fontList,
			&cellValues->fontWidth, &cellValues->fontHeight,
			g->grid.useAvgWidth);
		XmLGridCellDerefValues(g->grid.defCellValues);
		g->grid.defCellValues = cellValues;
		}
	if (NE(grid.allowDrop))
		DropRegister(g, g->grid.allowDrop); 
	if (g->grid.rowStep < 1)
		{
		XmLWarning(newW, "SetValues() - rowStep cannot be < 1");
		g->grid.rowStep = 1;
		}
	if (g->grid.colStep < 1)
		{
		XmLWarning(newW, "SetValues() - colStep cannot be < 1");
		g->grid.colStep = 1;
		}
	if (g->grid.topFixedCount < g->grid.headingRowCount)
		{
		XmLWarning(newW,
			"SetValues() - cannot set topFixedCount < headingRowCount");
		g->grid.topFixedCount = cur->grid.topFixedCount;
		}
	if (g->grid.bottomFixedCount < g->grid.footerRowCount)
		{
		XmLWarning(newW,
			"SetValues() - cannot set bottomFixedCount < footerRowCount");
		g->grid.bottomFixedCount = cur->grid.bottomFixedCount;
		}
	if (g->grid.leftFixedCount < g->grid.headingColCount)
		{
		XmLWarning(newW,
			"SetValues() - cannot set leftFixedCount < headingColumnCount");
		g->grid.leftFixedCount = cur->grid.leftFixedCount;
		}
	if (g->grid.rightFixedCount < g->grid.footerColCount)
		{
		XmLWarning(newW,
			"SetValues() - cannot set rightFixedCount < footerColumnCount");
		g->grid.rightFixedCount = cur->grid.rightFixedCount;
		}
	if (g->grid.hsb != cur->grid.hsb ||
		g->grid.vsb != cur->grid.vsb ||
		g->grid.text != cur->grid.text)
		{
		XmLWarning(newW, "SetValues() - child Widgets are read-only");
		g->grid.hsb = cur->grid.hsb;
		g->grid.vsb = cur->grid.vsb;
		g->grid.text = cur->grid.text;
		}
	if (g->grid.hiddenColCount != cur->grid.hiddenColCount ||
		g->grid.hiddenRowCount != cur->grid.hiddenRowCount)
		{
		XmLWarning(newW, "SetValues() - cannot set hidden rows or columns");
		g->grid.hiddenColCount = cur->grid.hiddenColCount;
		g->grid.hiddenRowCount = cur->grid.hiddenRowCount;
		}
	if (g->grid.colCount != cur->grid.colCount ||
		g->grid.rowCount != cur->grid.rowCount)
		{
		XmLWarning(newW, "SetValues() - cannot set number of rows or columns");
		g->grid.colCount = cur->grid.colCount;
		g->grid.rowCount = cur->grid.rowCount;
		}
	if (g->grid.headingColCount != cur->grid.headingColCount ||
		g->grid.headingRowCount != cur->grid.headingRowCount ||
		g->grid.footerColCount != cur->grid.footerColCount ||
		g->grid.footerRowCount != cur->grid.footerRowCount)
		{
		XmLWarning(newW, "SetValues() - cannot set heading/footer counts");
		g->grid.headingColCount = cur->grid.headingColCount;
		g->grid.headingRowCount = cur->grid.headingRowCount;
		g->grid.footerColCount = cur->grid.footerColCount;
		g->grid.footerRowCount = cur->grid.footerRowCount;
		}
#undef NE

	SetSubValues(newW, args, nargs);

	if (needsVertLayout)
		VertLayout(g, 1);
	if (needsHorizLayout)
		HorizLayout(g, 1);
	if (needsPlaceSB)
		PlaceScrollbars(g);
	if (needsRedraw)
		DrawArea(g, DrawAll, 0, 0);
	return (False);
	}

static void CopyFontList(g)
XmLGridWidget g;
	{
	if (!g->grid.fontList)
		g->grid.fontList = XmLFontListCopyDefault((Widget)g);
	else
		g->grid.fontList = XmFontListCopy(g->grid.fontList);
	if (!g->grid.fontList)
		XmLWarning((Widget)g, "- fatal error - font list NULL");
	}

static Boolean CvtStringToSizePolicy(dpy, args, narg, fromVal, toVal, data)
Display *dpy;
XrmValuePtr args;
Cardinal *narg;
XrmValuePtr fromVal, toVal;
XtPointer *data;
	{
	char *from;
	int i, num, valid;
	static struct policiesStruct
		{ char *name; unsigned char val; } policies[] =
		{
		{ "CONSTANT", XmCONSTANT },
		{ "VARIABLE", XmVARIABLE },
		};

	valid = 0;
	from = (char *)fromVal->addr;
	num = sizeof(policies) / sizeof(struct policiesStruct);
	for (i = 0; i < num; i++)
		if (!strcmp(from, policies[i].name))
			{
			valid = 1;
			break;
			}
	if (!valid)
		{
		XtDisplayStringConversionWarning(dpy, from, "XmRGridSizePolicy");
		toVal->size = 0;
		toVal->addr = 0;
		return False;
		}
	toVal->size = sizeof(unsigned char);
	toVal->addr = (caddr_t)&policies[i].val;
	return True;
	}

static Boolean CvtStringToSelectionPolicy(dpy, args, narg,
	fromVal, toVal, data)
Display *dpy;
XrmValuePtr args;
Cardinal *narg;
XrmValuePtr fromVal, toVal;
XtPointer *data;
	{
	char *from;
	int i, num, valid;
	static struct policiesStruct
		{ char *name; unsigned char val; } policies[] =
		{
		{ "SELECT_NONE", XmSELECT_NONE },
		{ "SELECT_SINGLE_ROW", XmSELECT_SINGLE_ROW },
		{ "SELECT_BROWSE_ROW", XmSELECT_BROWSE_ROW },
		{ "SELECT_MULTIPLE_ROW", XmSELECT_MULTIPLE_ROW },
		{ "SELECT_CELL", XmSELECT_CELL },
		};

	valid = 0;
	from = (char *)fromVal->addr;
	num = sizeof(policies) / sizeof(struct policiesStruct);
	for (i = 0; i < num; i++)
		if (!strcmp(from, policies[i].name))
			{
			valid = 1;
			break;
			}
	if (!valid)
		{
		XtDisplayStringConversionWarning(dpy, from, "XmRGridSelectionPolicy");
		toVal->size = 0;
		toVal->addr = 0;
		return False;
		}
	toVal->size = sizeof(unsigned char);
	toVal->addr = (caddr_t)&policies[i].val;
	return True;
	}

static Boolean CvtStringToCellAlignment(dpy, args, narg, fromVal, toVal, data)
Display *dpy;
XrmValuePtr args;
Cardinal *narg;
XrmValuePtr fromVal, toVal;
XtPointer *data;
	{
	char *from;
	int i, num, valid;
	static struct policiesStruct
		{ char *name; unsigned char val; } policies[] =
		{
		{ "ALIGNMENT_LEFT", XmALIGNMENT_LEFT },
		{ "ALIGNMENT_CENTER", XmALIGNMENT_CENTER },
		{ "ALIGNMENT_RIGHT", XmALIGNMENT_RIGHT },
		{ "ALIGNMENT_TOP_LEFT", XmALIGNMENT_TOP_LEFT },
		{ "ALIGNMENT_TOP", XmALIGNMENT_TOP },
		{ "ALIGNMENT_TOP_RIGHT", XmALIGNMENT_TOP_RIGHT },
		{ "ALIGNMENT_BOTTOM_LEFT", XmALIGNMENT_BOTTOM_LEFT },
		{ "ALIGNMENT_BOTTOM", XmALIGNMENT_BOTTOM },
		{ "ALIGNMENT_BOTTOM_RIGHT", XmALIGNMENT_BOTTOM_RIGHT },
		};

	valid = 0;
	from = (char *)fromVal->addr;
	num = sizeof(policies) / sizeof(struct policiesStruct);
	for (i = 0; i < num; i++)
		if (!strcmp(from, policies[i].name))
			{
			valid = 1;
			break;
			}
	if (!valid)
		{
		XtDisplayStringConversionWarning(dpy, from, "XmRGridCellAlignment");
		toVal->size = 0;
		toVal->addr = 0;
		return False;
		}
	toVal->size = sizeof(unsigned char);
	toVal->addr = (caddr_t)&policies[i].val;
	return True;
	}

static Boolean CvtStringToCellBorderType(dpy, args, narg, fromVal, toVal, data)
Display *dpy;
XrmValuePtr args;
Cardinal *narg;
XrmValuePtr fromVal, toVal;
XtPointer *data;
	{
	char *from;
	int i, num, valid;
	static struct policiesStruct
		{ char *name; unsigned char val; } policies[] =
		{
		{ "BORDER_NONE", XmBORDER_NONE },
		{ "BORDER_LINE", XmBORDER_LINE },
		};

	valid = 0;
	from = (char *)fromVal->addr;
	num = sizeof(policies) / sizeof(struct policiesStruct);
	for (i = 0; i < num; i++)
		if (!strcmp(from, policies[i].name))
			{
			valid = 1;
			break;
			}
	if (!valid)
		{
		XtDisplayStringConversionWarning(dpy, from, "XmRGridCellBorderType");
		toVal->size = 0;
		toVal->addr = 0;
		return False;
		}
	toVal->size = sizeof(unsigned char);
	toVal->addr = (caddr_t)&policies[i].val;
	return True;
	}

static Boolean CvtStringToCellType(dpy, args, narg, fromVal, toVal, data)
Display *dpy;
XrmValuePtr args;
Cardinal *narg;
XrmValuePtr fromVal, toVal;
XtPointer *data;
	{
	char *from;
	int i, num, valid;
	static struct policiesStruct
		{ char *name; unsigned char val; } policies[] =
		{
		{ "TEXT_CELL", XmTEXT_CELL },
		{ "LABEL_CELL", XmLABEL_CELL },
		{ "PIXMAP_CELL", XmPIXMAP_CELL },
		};

	valid = 0;
	from = (char *)fromVal->addr;
	num = sizeof(policies) / sizeof(struct policiesStruct);
	for (i = 0; i < num; i++)
		if (!strcmp(from, policies[i].name))
			{
			valid = 1;
			break;
			}
	if (!valid)
		{
		XtDisplayStringConversionWarning(dpy, from, "XmRGridCellType");
		toVal->size = 0;
		toVal->addr = 0;
		return False;
		}
	toVal->size = sizeof(unsigned char);
	toVal->addr = (caddr_t)&policies[i].val;
	return True;
	}

/*
   Getting and Setting Row Values
*/

#define XmLGridRowHeight             (1L<<0)
#define XmLGridRowSizePolicy         (1L<<1)
#define XmLGridRowUserData           (1L<<2)

static void RowValueStringToMask(s, mask)
char *s;
long *mask;
	{
	static XrmQuark qHeight, qSizePolicy, qUserData;
	static int quarksValid = 0;
	XrmQuark q;

	if (!quarksValid)
		{
		qHeight = XrmStringToQuark(XmNrowHeight);
		qSizePolicy = XrmStringToQuark(XmNrowSizePolicy);
		qUserData = XrmStringToQuark(XmNrowUserData);
		quarksValid = 1;
		}
	q = XrmStringToQuark(s);
	if (q == qHeight)
			*mask |= XmLGridRowHeight;
	else if (q == qSizePolicy)
			*mask |= XmLGridRowSizePolicy;
	else if (q == qUserData)
			*mask |= XmLGridRowUserData;
	}

static void GetRowValue(row, value, mask)
XmLGridRow row;
XtArgVal value;
long mask;
	{
	XmLGridRowValues *values;

	values = XmLGridRowGetValues(row);
	switch (mask)
		{
		case XmLGridRowHeight:
			*((Dimension *)value) = values->height; 
			break;
		case XmLGridRowSizePolicy:
			*((unsigned char *)value) = values->sizePolicy; 
			break;
		case XmLGridRowUserData:
			*((XtPointer *)value) = (XtPointer)values->userData;
		}
	}

static int SetRowValues(g, row, mask)
XmLGridWidget g;
XmLGridRow row;
long mask;
	{
	XmLGridRowValues *newValues, *values;
	int needsResize, visible;
	Boolean newIsHidden;

	newValues = &g->grid.rowValues;
	values = XmLGridRowGetValues(row);
	needsResize = 0;
	if (mask & XmLGridRowHeight || mask & XmLGridRowSizePolicy)
		{
		visible = RowIsVisible(g, XmLGridRowGetPos(row));
		XmLGridRowHeightChanged(row);
		}
	if (mask & XmLGridRowHeight)
		{
		if (newValues->height > 0)
			newIsHidden = False;
		else
			newIsHidden = True;
		if (XmLGridRowIsHidden(row) != newIsHidden)
			{
			if (newIsHidden == True)
				g->grid.hiddenRowCount++;
			else
				g->grid.hiddenRowCount--;
			g->grid.recalcVertVisPos = 1;
			needsResize = 1;
			}
		if (visible)
			needsResize = 1;
		values->height = newValues->height;
		}
	if (mask & XmLGridRowSizePolicy)
		{
		values->sizePolicy = newValues->sizePolicy;
		if (visible)
			needsResize = 1;
		}
	if (mask & XmLGridRowUserData)
		values->userData = newValues->userData;
	return needsResize;
	}

/*
   Getting and Setting Column Values
*/

#define XmLGridColumnWidth           (1L<<0)
#define XmLGridColumnSizePolicy      (1L<<1)
#define XmLGridColumnUserData        (1L<<2)

static void ColumnValueStringToMask(s, mask)
char *s;
long *mask;
	{
	static XrmQuark qWidth, qSizePolicy, qUserData;
	static int quarksValid = 0;
	XrmQuark q;

	if (!quarksValid)
		{
		qWidth = XrmStringToQuark(XmNcolumnWidth);
		qSizePolicy = XrmStringToQuark(XmNcolumnSizePolicy);
		qUserData = XrmStringToQuark(XmNcolumnUserData);
		quarksValid = 1;
		}
	q = XrmStringToQuark(s);
	if (q == qWidth)
			*mask |= XmLGridColumnWidth;
	else if (q == qSizePolicy)
			*mask |= XmLGridColumnSizePolicy;
	else if (q == qUserData)
			*mask |= XmLGridColumnUserData;
	}

static void GetColumnValue(col, value, mask)
XmLGridColumn col;
XtArgVal value;
long mask;
	{
	XmLGridColumnValues *values;

	values = XmLGridColumnGetValues(col);
	switch (mask)
		{
		case XmLGridColumnWidth:
			*((Dimension *)value) = values->width; 
			break;
		case XmLGridColumnSizePolicy:
			*((unsigned char *)value) = values->sizePolicy; 
			break;
		case XmLGridColumnUserData:
			*((XtPointer *)value) = values->userData;
		}
	}

static int SetColumnValues(g, col, mask)
XmLGridWidget g;
XmLGridColumn col;
long mask;
	{
	XmLGridColumnValues *newValues, *values;
	int needsResize, visible;
	Boolean newIsHidden;

	newValues = &g->grid.colValues;
	values = XmLGridColumnGetValues(col);
	needsResize = 0;
	if (mask & XmLGridColumnWidth || mask & XmLGridColumnSizePolicy)
		{
		visible = ColIsVisible(g, XmLGridColumnGetPos(col));
		XmLGridColumnWidthChanged(col);
		}
	if (mask & XmLGridColumnWidth)
		{
		if (newValues->width > 0)
			newIsHidden = False;
		else
			newIsHidden = True;
		if (XmLGridColumnIsHidden(col) != newIsHidden)
			{
			if (newIsHidden == True)
				g->grid.hiddenColCount++;
			else
				g->grid.hiddenRowCount--;
			g->grid.recalcHorizVisPos = 1;
			needsResize = 1;
			}
		if (visible)
			needsResize = 1;
		values->width = newValues->width;
		}
	if (mask & XmLGridColumnSizePolicy)
		{
		values->sizePolicy = newValues->sizePolicy;
		if (visible)
			needsResize = 1;
		}
	if (mask & XmLGridColumnUserData)
		values->userData = newValues->userData;
	return needsResize;
	}

/*
   Getting and Setting Cell Values
*/

#define XmLGridCellAlignment         (1L<<0)
#define XmLGridCellBackground        (1L<<1)
#define XmLGridCellBottomBorderPixel (1L<<2)
#define XmLGridCellBottomBorderType  (1L<<3)
#define XmLGridCellColumnSpan        (1L<<4)
#define XmLGridCellFontList          (1L<<5)
#define XmLGridCellForeground        (1L<<6)
#define XmLGridCellLeftBorderPixel   (1L<<7)
#define XmLGridCellLeftBorderType    (1L<<8)
#define XmLGridCellPixmapM           (1L<<9)
#define XmLGridCellRightBorderPixel  (1L<<10)
#define XmLGridCellRightBorderType   (1L<<11)
#define XmLGridCellRowSpan           (1L<<12)
#define XmLGridCellString            (1L<<13)
#define XmLGridCellTopBorderPixel    (1L<<14)
#define XmLGridCellTopBorderType     (1L<<15)
#define XmLGridCellType              (1L<<16)
#define XmLGridCellUserData          (1L<<17)

static void CellValueStringToMask(s, mask)
char *s;
long *mask;
	{
	static XrmQuark qAlignment, qBackground;
	static XrmQuark qBottomBorderPixel, qBottomBorderType;
	static XrmQuark qColumnSpan, qFontList, qForeground;
	static XrmQuark qLeftBorderPixel, qLeftBorderType;
	static XrmQuark qPixmap;
	static XrmQuark qRightBorderPixel, qRightBorderType;
	static XrmQuark qRowSpan, qString;
	static XrmQuark qTopBorderPixel, qTopBorderType;
	static XrmQuark qType, qUserData;
	static int quarksValid = 0;
	XrmQuark q;

	if (!quarksValid)
		{
		qAlignment = XrmStringToQuark(XmNcellAlignment);
		qBackground = XrmStringToQuark(XmNcellBackground);
		qBottomBorderPixel = XrmStringToQuark(XmNcellBottomBorderPixel);
		qBottomBorderType = XrmStringToQuark(XmNcellBottomBorderType);
		qColumnSpan = XrmStringToQuark(XmNcellColumnSpan);
		qFontList = XrmStringToQuark(XmNcellFontList);
		qForeground = XrmStringToQuark(XmNcellForeground);
		qLeftBorderPixel = XrmStringToQuark(XmNcellLeftBorderPixel);
		qLeftBorderType = XrmStringToQuark(XmNcellLeftBorderType);
		qPixmap = XrmStringToQuark(XmNcellPixmap);
		qRightBorderPixel = XrmStringToQuark(XmNcellRightBorderPixel);
		qRightBorderType = XrmStringToQuark(XmNcellRightBorderType);
		qRowSpan = XrmStringToQuark(XmNcellRowSpan);
		qString = XrmStringToQuark(XmNcellString);
		qTopBorderPixel = XrmStringToQuark(XmNcellTopBorderPixel);
		qTopBorderType = XrmStringToQuark(XmNcellTopBorderType);
		qType = XrmStringToQuark(XmNcellType);
		qUserData = XrmStringToQuark(XmNcellUserData);
		quarksValid = 1;
		}
	q = XrmStringToQuark(s);
	if (q == qAlignment)
			*mask |= XmLGridCellAlignment;
	else if (q == qBackground)
			*mask |= XmLGridCellBackground;
	else if (q == qBottomBorderPixel)
			*mask |= XmLGridCellBottomBorderPixel;
	else if (q == qBottomBorderType)
			*mask |= XmLGridCellBottomBorderType;
	else if (q == qColumnSpan)
			*mask |= XmLGridCellColumnSpan;
	else if (q == qFontList)
			*mask |= XmLGridCellFontList;
	else if (q == qForeground)
			*mask |= XmLGridCellForeground;
	else if (q == qLeftBorderPixel)
			*mask |= XmLGridCellLeftBorderPixel;
	else if (q == qLeftBorderType)
			*mask |= XmLGridCellLeftBorderType;
	else if (q == qPixmap)
			*mask |= XmLGridCellPixmapM;
	else if (q == qRightBorderPixel)
			*mask |= XmLGridCellRightBorderPixel;
	else if (q == qRightBorderType)
			*mask |= XmLGridCellRightBorderType;
	else if (q == qRowSpan)
			*mask |= XmLGridCellRowSpan;
	else if (q == qString)
			*mask |= XmLGridCellString;
	else if (q == qTopBorderPixel)
			*mask |= XmLGridCellTopBorderPixel;
	else if (q == qTopBorderType)
			*mask |= XmLGridCellTopBorderType;
	else if (q == qType)
			*mask |= XmLGridCellType;
	else if (q == qUserData)
			*mask |= XmLGridCellUserData;
	}

static void GetCellValue(cell, value, mask)
XmLGridCell cell;
XtArgVal value;
long mask;
	{
	XmLGridCellRefValues *values;
	XmLGridCellPixmap *pix;
	XmString str;

	values = XmLGridCellGetRefValues(cell);
	switch (mask)
		{
		case XmLGridCellAlignment:
			*((unsigned char *)value) = values->alignment; 
			break;
		case XmLGridCellBackground:
			*((Pixel *)value) = values->background; 
			break;
		case XmLGridCellBottomBorderPixel:
			*((Pixel *)value) = values->bottomBorderPixel;
			break;
		case XmLGridCellBottomBorderType:
			*((unsigned char *)value) = values->bottomBorderType;
			break;
		case XmLGridCellColumnSpan:
			*((int *)value) = values->columnSpan;
			break;
		case XmLGridCellFontList:
			*((XmFontList *)value) = values->fontList; 
			break;
		case XmLGridCellForeground:
			*((Pixel *)value) = values->foreground; 
			break;
		case XmLGridCellLeftBorderPixel:
			*((Pixel *)value) = values->leftBorderPixel;
			break;
		case XmLGridCellLeftBorderType:
			*((unsigned char *)value) = values->leftBorderType;
			break;
		case XmLGridCellPixmapM:
			pix = XmLGridCellGetPixmap(cell);
			if (pix)
				*((Pixmap *)value) = pix->pixmap;
			else
				*((Pixmap *)value) = (Pixmap)XmUNSPECIFIED_PIXMAP;
			break;
		case XmLGridCellRightBorderPixel:
			*((Pixel *)value) = values->rightBorderPixel;
			break;
		case XmLGridCellRightBorderType:
			*((unsigned char *)value) = values->rightBorderType;
			break;
		case XmLGridCellRowSpan:
			*((int *)value) = values->rowSpan;
			break;
		case XmLGridCellString:
			str = XmLGridCellGetString(cell);
			if (str)
				*((XmString *)value) = XmStringCopy(str);
			else
				*((XmString *)value) = 0;
			break;
		case XmLGridCellTopBorderPixel:
			*((Pixel *)value) = values->topBorderPixel;
			break;
		case XmLGridCellTopBorderType:
			*((unsigned char *)value) = values->topBorderType;
			break;
		case XmLGridCellType:
			*((unsigned char *)value) = values->type;
			break;
		case XmLGridCellUserData:
			*((XtPointer *)value) = (XtPointer)values->userData;
			break;
		}
	}

static XmLGridCellRefValues *CellRefValuesCreate(g, copy)
XmLGridWidget g;
XmLGridCellRefValues *copy;
	{
	short width, height;
	XmLGridCellRefValues *values;

	values = (XmLGridCellRefValues *)malloc(sizeof(XmLGridCellRefValues));
	if (!copy)
		{
		/* default values */
		XmLFontListGetDimensions(g->grid.fontList, &width, &height,
			g->grid.useAvgWidth);
		values->alignment = XmALIGNMENT_CENTER;
		values->background = g->core.background_pixel;
		values->bottomBorderPixel = g->manager.bottom_shadow_color;
		values->bottomBorderType = XmBORDER_LINE;
		values->columnSpan = 0;
		values->fontHeight = height;
		values->fontList = XmFontListCopy(g->grid.fontList);
		values->fontWidth = width;
		values->foreground = g->manager.foreground;
		values->leftBorderPixel = g->manager.top_shadow_color;
		values->leftBorderType = XmBORDER_LINE;
		values->refCount = 0;
		values->rightBorderPixel = g->manager.bottom_shadow_color;
		values->rightBorderType = XmBORDER_LINE;
		values->rowSpan = 0;
		values->topBorderPixel = g->manager.top_shadow_color;
		values->topBorderType = XmBORDER_LINE;
		values->type = XmLABEL_CELL;
		values->userData = 0;
		}
	else
		{
		/* copy values */
		values->alignment = copy->alignment;
		values->background = copy->background;
		values->bottomBorderPixel = copy->bottomBorderPixel;
		values->bottomBorderType = copy->bottomBorderType;
		values->columnSpan = copy->columnSpan;
		values->fontHeight = copy->fontHeight;
		values->fontList = XmFontListCopy(copy->fontList);
		values->fontWidth = copy->fontWidth;
		values->foreground = copy->foreground;
		values->leftBorderPixel = copy->leftBorderPixel;
		values->leftBorderType = copy->leftBorderType;
		values->refCount = 0;
		values->rightBorderPixel = copy->rightBorderPixel;
		values->rightBorderType = copy->rightBorderType;
		values->rowSpan = copy->rowSpan;
		values->topBorderPixel = copy->topBorderPixel;
		values->topBorderType = copy->topBorderType;
		values->type = copy->type;
		values->userData = copy->userData;
		}
	return values;
	}

static void SetCellValuesPreprocess(g, mask)
XmLGridWidget g;
long mask;
	{
	XmLGridCellRefValues *newValues;
	int x, y;
	short width, height;
	Display *dpy;
	Window pixRoot;
	unsigned int pixWidth, pixHeight;
	unsigned int pixBW, pixDepth;

	/* calculate font width and height if set */
	newValues = &g->grid.cellValues;
	if (mask & XmLGridCellFontList)
		{
		XmLFontListGetDimensions(newValues->fontList, &width, &height,
			g->grid.useAvgWidth);
		newValues->fontWidth = width;
		newValues->fontHeight = height;
		}
	if ((mask & XmLGridCellPixmapM) &&
		(g->grid.cellPixmap != XmUNSPECIFIED_PIXMAP))
		{
		dpy = XtDisplay(g);
		XGetGeometry(dpy, g->grid.cellPixmap, &pixRoot,
			&x, &y, &pixWidth, &pixHeight, &pixBW, &pixDepth);
		g->grid.cellPixmapWidth = (Dimension)pixWidth;
		g->grid.cellPixmapHeight = (Dimension)pixHeight;
		}
	}

static int SetCellHasRefValues(mask)
long mask;
	{
	long unrefMask;

	/* return 1 if mask contains any reference counted values */
	unrefMask = XmLGridCellPixmapM | XmLGridCellString;
	mask = mask | unrefMask;
	mask = mask ^ unrefMask;
	if (!mask)
		return 0;
	return 1;
	}

static int SetCellValuesNeedsResize(g, row, col, cell, mask)
XmLGridWidget g;
XmLGridRow row;
XmLGridColumn col;
XmLGridCell cell;
long mask;
	{
	XmLGridCellPixmap *cellPix;
	XmLGridCellRefValues *refValues;
	int pixResize, needsResize, rowVisible, colVisible;

	needsResize = 0;
	pixResize = 0;
	if (mask & XmLGridCellPixmapM)
		{
		pixResize = 1;
		refValues = XmLGridCellGetRefValues(cell);
		if (refValues->type == XmPIXMAP_CELL && !(mask & XmLGridCellType))
			{
			cellPix = XmLGridCellGetPixmap(cell);
			if (cellPix && cellPix->pixmap != XmUNSPECIFIED_PIXMAP &&
				g->grid.cellPixmap != XmUNSPECIFIED_PIXMAP)
				{
				if (cellPix->width == g->grid.cellPixmapWidth &&
					cellPix->height == g->grid.cellPixmapHeight)
					pixResize = 0;
				}
			}
		}
	if (mask & XmLGridCellType || mask & XmLGridCellFontList || pixResize ||
		mask & XmLGridCellRowSpan || mask & XmLGridCellColumnSpan)
		{
		XmLGridRowHeightChanged(row);
		XmLGridColumnWidthChanged(col);
		rowVisible = RowIsVisible(g, XmLGridRowGetPos(row));
		colVisible = ColIsVisible(g, XmLGridColumnGetPos(col));
		if (rowVisible | colVisible)
			needsResize = 1;
		}
	return needsResize;
	}

static void SetCellValues(g, cell, mask)
XmLGridWidget g;
XmLGridCell cell;
long mask;
	{
	/* set non-reference counted cell values */
	if (mask & XmLGridCellPixmapM)
		XmLGridCellSetPixmap(cell, g->grid.cellPixmap, g->grid.cellPixmapWidth,
			g->grid.cellPixmapHeight);
	if (mask & XmLGridCellString)
		XmLGridCellSetString(cell, g->grid.cellString, True);
	}

static void SetCellRefValues(g, values, mask)
XmLGridWidget g;
XmLGridCellRefValues *values;
long mask;
	{
	XmLGridCellRefValues *newValues;

	/* set reference counted cell values */
	newValues = &g->grid.cellValues;
	if (mask & XmLGridCellAlignment)
		values->alignment = newValues->alignment;
	if (mask & XmLGridCellBackground)
		values->background = newValues->background;
	if (mask & XmLGridCellBottomBorderPixel)
		values->bottomBorderPixel = newValues->bottomBorderPixel;
	if (mask & XmLGridCellBottomBorderType)
		values->bottomBorderType = newValues->bottomBorderType;
	if (mask & XmLGridCellColumnSpan)
		values->columnSpan = newValues->columnSpan;
	if (mask & XmLGridCellFontList)
		{
		XmFontListFree(values->fontList);
		values->fontList = XmFontListCopy(newValues->fontList);
		values->fontWidth = newValues->fontWidth;
		values->fontHeight = newValues->fontHeight;
		}
	if (mask & XmLGridCellForeground)
		values->foreground = newValues->foreground;
	if (mask & XmLGridCellLeftBorderPixel)
		values->leftBorderPixel = newValues->leftBorderPixel;
	if (mask & XmLGridCellLeftBorderType)
		values->leftBorderType = newValues->leftBorderType;
	if (mask & XmLGridCellRightBorderPixel)
		values->rightBorderPixel = newValues->rightBorderPixel;
	if (mask & XmLGridCellRightBorderType)
		values->rightBorderType = newValues->rightBorderType;
	if (mask & XmLGridCellRowSpan)
		values->rowSpan = newValues->rowSpan;
	if (mask & XmLGridCellTopBorderPixel)
		values->topBorderPixel = newValues->topBorderPixel;
	if (mask & XmLGridCellTopBorderType)
		values->topBorderType = newValues->topBorderType;
	if (mask & XmLGridCellType)
		values->type = newValues->type;
	if (mask & XmLGridCellUserData)
		values->userData = newValues->userData;
	}

static int SetCellRefValuesCompare(userData, item1, item2)
void *userData;
void **item1, **item2;
	{
	XmLGridCell cell1, cell2;
	XmLGridCellRefValues *values1, *values2;
	long mask;

	mask = *((long *)userData);
	cell1 = (XmLGridCell)*item1;
	cell2 = (XmLGridCell)*item2;
	values1 = XmLGridCellGetRefValues(cell1);
	values2 = XmLGridCellGetRefValues(cell2);
	if (values1 == values2)
		return 0;
	if (!(mask & XmLGridCellAlignment))
		{
		if (values1->alignment < values2->alignment)
			return -1;
		if (values1->alignment > values2->alignment)
			return 1;
		}
	if (!(mask & XmLGridCellBackground))
		{
		if (values1->background < values2->background)
			return -1;
		if (values1->background > values2->background)
			return 1;
		}
	if (!(mask & XmLGridCellBottomBorderPixel))
		{
		if (values1->bottomBorderPixel < values2->bottomBorderPixel)
			return -1;
		if (values1->bottomBorderPixel > values2->bottomBorderPixel)
			return 1;
		}
	if (!(mask & XmLGridCellBottomBorderType))
		{
		if (values1->bottomBorderType < values2->bottomBorderType)
			return -1;
		if (values1->bottomBorderType > values2->bottomBorderType)
			return 1;
		}
	if (!(mask & XmLGridCellColumnSpan))
		{
		if (values1->columnSpan < values2->columnSpan)
			return -1;
		if (values1->columnSpan > values2->columnSpan)
			return 1;
		}
	if (!(mask & XmLGridCellFontList))
		{
		if (values1->fontList < values2->fontList)
			return -1;
		if (values1->fontList > values2->fontList)
			return 1;
		}
	if (!(mask & XmLGridCellForeground))
		{
		if (values1->foreground < values2->foreground)
			return -1;
		if (values1->foreground > values2->foreground)
			return 1;
		}
	if (!(mask & XmLGridCellLeftBorderPixel))
		{
		if (values1->leftBorderPixel < values2->leftBorderPixel)
			return -1;
		if (values1->leftBorderPixel > values2->leftBorderPixel)
			return 1;
		}
	if (!(mask & XmLGridCellLeftBorderType))
		{
		if (values1->leftBorderType < values2->leftBorderType)
			return -1;
		if (values1->leftBorderType > values2->leftBorderType)
			return 1;
		}
	if (!(mask & XmLGridCellRightBorderPixel))
		{
		if (values1->rightBorderPixel < values2->rightBorderPixel)
			return -1;
		if (values1->rightBorderPixel > values2->rightBorderPixel)
			return 1;
		}
	if (!(mask & XmLGridCellRightBorderType))
		{
		if (values1->rightBorderType < values2->rightBorderType)
			return -1;
		if (values1->rightBorderType > values2->rightBorderType)
			return 1;
		}
	if (!(mask & XmLGridCellRowSpan))
		{
		if (values1->rowSpan < values2->rowSpan)
			return -1;
		if (values1->rowSpan > values2->rowSpan)
			return 1;
		}
	if (!(mask & XmLGridCellTopBorderPixel))
		{
		if (values1->topBorderPixel < values2->topBorderPixel)
			return -1;
		if (values1->topBorderPixel > values2->topBorderPixel)
			return 1;
		}
	if (!(mask & XmLGridCellTopBorderType))
		{
		if (values1->topBorderType < values2->topBorderType)
			return -1;
		if (values1->topBorderType > values2->topBorderType)
			return 1;
		}
	if (!(mask & XmLGridCellType))
		{
		if (values1->type < values2->type)
			return -1;
		if (values1->type > values2->type)
			return 1;
		}
	if (!(mask & XmLGridCellUserData))
		{
		if (values1->userData < values2->userData)
			return -1;
		if (values1->userData > values2->userData)
			return 1;
		}
	/* If the two cell values are equal, we merge them
	   into one record here.  This speeds up the sort
  	   and will allow the merge to compare just the values
	   pointers to test equality. This will not merge
	   everything that could be merged. */
	if (values1 < values2)
		XmLGridCellSetRefValues(cell1, values2);
	else
		XmLGridCellSetRefValues(cell2, values1);
	return 0;
	}

static void SetCellRefValuesPreprocess(g, row, col, cell, mask)
XmLGridWidget g;
int row, col;
XmLGridCell cell;
long mask;
	{
	int r, c, rowSpan, colSpan;
	XmLGridCell spanCell;
	XmLGridCellRefValues *oldValues, *newValues;
	unsigned char oldType, newType;
	XmLGridCallbackStruct cbs;

	if (mask & XmLGridCellType)
		{
		oldType = XmLGridCellGetRefValues(cell)->type;
		newType = g->grid.cellValues.type;
		if ((oldType == newType) ||
			(oldType == XmLABEL_CELL && newType == XmTEXT_CELL) ||
			(oldType == XmTEXT_CELL && newType == XmLABEL_CELL))
			;
		else
			{
			cbs.reason = XmCR_FREE_VALUE;
			XmLGridCellAction(cell, 0, &cbs);
			}
		}
	if (mask & XmLGridCellRowSpan || mask & XmLGridCellColumnSpan)
		{
		/* expose old cell area in case the span area shrinks */
		DrawArea(g, DrawCell, row, col);
		oldValues = XmLGridCellGetRefValues(cell);
		newValues = &g->grid.cellValues;
		if (mask & XmLGridCellRowSpan)
			{
			if (newValues->rowSpan < 0)
				{
				XmLWarning((Widget)g,
					"SetValues() - row span cannot be < 0");
				newValues->rowSpan = 0;
				}
			rowSpan = newValues->rowSpan;
			}
		else
			rowSpan = oldValues->rowSpan;
		if (mask & XmLGridCellColumnSpan)
			{
			if (newValues->columnSpan < 0)
				{
				XmLWarning((Widget)g,
					"SetValues() - column span cannot be < 0");
				newValues->columnSpan = 0;
				}
			colSpan = newValues->columnSpan;
			}
		else
			colSpan = oldValues->columnSpan;
		/* clear old span */
		for (c = col; c <= col + oldValues->columnSpan; c++)
			for (r = row; r <= row + oldValues->rowSpan; r++)
				{
				/* skip the cell itself */
				if (c == col && r == row)
					continue;
				spanCell = GetCell(g, r, c);
				if (!spanCell)
					continue;
				XmLGridCellSetInRowSpan(spanCell, False);
				XmLGridCellSetInColumnSpan(spanCell, False);
				}
		/* set new span */
		for (c = col; c <= col + colSpan; c++)
			for (r = row; r <= row + rowSpan; r++)
				{
				/* skip the cell itself */
				if (c == col && r == row)
					continue;
				spanCell = GetCell(g, r, c);
				if (!spanCell)
					continue;
				if (r == row)
					XmLGridCellSetInColumnSpan(spanCell, True);
				else
					XmLGridCellSetInRowSpan(spanCell, True);
				}
		}
	}

/*
   Read, Write, Copy, Paste
*/

static int Read(g, format, delimiter, row, col, data)
XmLGridWidget g;
int format;
char delimiter;
int row, col;
char *data;
	{
	char *c1, *c2, buf[256], *bufp;
	int r, c, i, len, n, allowSet, done;
	XmString str;
	XmLGridCell cell;
	XmLGridCellRefValues *cellValues;
	XmLGridCallbackStruct cbs;

	if (format == XmFORMAT_PAD)
		{
		XmLWarning((Widget)g, "Read() - FORMAT_PAD not supported");
		return 0;
		}
	if (format == XmFORMAT_XL ||
		format == XmFORMAT_DROP ||
		format == XmFORMAT_PASTE)
		delimiter = '\t';
	c1 = data;
	c2 = data;
	r = row;
	c = col;
	done = 0;
	n = 0;
	while (!done)
		{
		if (!(*c2) || *c2 == delimiter || *c2 == '\n')
			{
			len = c2 - c1;
			if (len < 256)
				bufp = buf;
			else
				bufp = (char *)malloc(len + 1);
			if (format == XmFORMAT_XL)
				{
				/* strip leading and trailing double-quotes */
				if (len && c1[0] == '"')
					{
					c1++;
					len--;
					}
				if (len && c1[len - 1] == '"')
					len--;
				}
			for (i = 0; i < len; i++)
				bufp[i] = *c1++;
			bufp[i] = 0;
#ifdef MOTIF11
			str = XmStringCreate(bufp, XmSTRING_DEFAULT_CHARSET);
#else
			str = XmStringCreateLocalized(bufp);
#endif
			if (bufp != buf)
				free((char *)bufp);
			cell = GetCell(g, r, c);
			allowSet = 1;
			if (cell && (format == XmFORMAT_PASTE || format == XmFORMAT_DROP))
				{
				cellValues = XmLGridCellGetRefValues(cell);
				if (cellValues->type != XmTEXT_CELL ||
					RowPosToType(g, r) != XmCONTENT ||
					ColPosToType(g, c) != XmCONTENT)
					allowSet = 0;
				}
			if (cell && allowSet)
				{
				XmLGridCellSetString(cell, str, False);
				DrawArea(g, DrawCell, r, c);
				cbs.columnType = ColPosToType(g, c);
				cbs.column = ColPosToTypePos(g, cbs.columnType, c);
				cbs.rowType = RowPosToType(g, r);
				cbs.row = RowPosToTypePos(g, cbs.rowType, r);
				if (format == XmFORMAT_PASTE)
					{
					cbs.reason = XmCR_CELL_PASTE;
					XtCallCallbackList((Widget)g, g->grid.cellPasteCallback,
						(XtPointer)&cbs);
					}
				else if (format == XmFORMAT_DROP)
					{
					cbs.reason = XmCR_CELL_DROP;
					XtCallCallbackList((Widget)g, g->grid.cellDropCallback,
						(XtPointer)&cbs);
					}
				n++;
				}
			else
				XmStringFree(str);
			}
		if (!(*c2))
			done = 1;
		else if (*c2 == delimiter)
			{
			c++;
			c1 = c2 + 1;
			}
		else if (*c2 == '\n')
			{
			r++;
			c = col;
			c1 = c2 + 1;
			}
		c2++;
		}
	return n;
	}

static void Write(g, file, format, delimiter, skipHidden, row, col, nrow, ncol)
XmLGridWidget g;
FILE *file;
int format;
char delimiter;
Boolean skipHidden;
int row, col;
int nrow, ncol;
	{
	int r, c, i, last;
	char *cs;
	Boolean set;
	XmString str;
	XmLGridColumnValues *colValues;
	XmLGridColumn colp;
	XmLGridRow rowp;
	XmLGridCell cell;

	for (r = row; r < row + nrow; r++)
		{
		rowp = (XmLGridRow)XmLArrayGet(g->grid.rowArray, r);
		if (!rowp)
			continue;
		if (skipHidden == True && XmLGridRowIsHidden(rowp) == True)
			continue;
		for (c = col; c < col + ncol; c++)
			{
			colp = (XmLGridColumn)XmLArrayGet(g->grid.colArray, c);
			if (!colp)
				continue;
			if (skipHidden == True && XmLGridColumnIsHidden(colp) == True)
				continue;
			cell = GetCell(g, r, c);
			if (!cell)
				continue;
			str = XmLGridCellGetString(cell);
			set = False;
			if (str)
				set = XmStringGetLtoR(str, XmSTRING_DEFAULT_CHARSET, &cs);
			if (set == False)
				cs = "";
			fprintf(file, "%s", cs);

			last = 0;
			if (c == col + ncol - 1)
				last = 1;
			if (!last && format == XmFORMAT_DELIMITED)
				fprintf(file, "%c", delimiter);
			else if (!last && format == XmFORMAT_XL)
				fprintf(file, "\t");
			else if (format == XmFORMAT_PAD)
				{
				colValues = XmLGridColumnGetValues(colp);
				if (colValues->sizePolicy == XmVARIABLE)
					for (i = 0; i < colValues->width - strlen(cs); i++)
						fprintf(file, " ");
				}

			if (set == True)
				XtFree(cs);
			}
		fprintf(file, "\n");
		}
	}

char *CopyDataCreate(g, selected, row, col, nrow, ncol)
XmLGridWidget g;
int selected;
int row, col;
int nrow, ncol;
	{
	XmLGridColumn colp;
	XmLGridRow rowp;
	XmLGridCell cell;
	char *buf, *cs;
	XmString str;
	Boolean set;
	int r, c, wroteStr, bufsize, buflen, len;

	if (selected)
		{
		row = 0;
		nrow = XmLArrayGetCount(g->grid.rowArray);
		col = 0;
		ncol = XmLArrayGetCount(g->grid.colArray);
		}
	bufsize = 1024;
	buflen = 0;
	buf = (char *)malloc(bufsize);

	for (r = row; r < row + nrow; r++)
		{
		wroteStr = 0;
		rowp = (XmLGridRow)XmLArrayGet(g->grid.rowArray, r);
		if (!rowp)
			continue;
		for (c = col; c < col + ncol; c++)
			{
			colp = (XmLGridColumn)XmLArrayGet(g->grid.colArray, c);
			if (!colp)
				continue;
			cell = GetCell(g, r, c);
			if (!cell)
				continue;
			if (selected &&
				XmLGridRowIsSelected(rowp) == False &&
				XmLGridColumnIsSelected(colp) == False &&
				XmLGridCellIsSelected(cell) == False)
				continue;
			str = XmLGridCellGetString(cell);
			set = False;
			if (str)
				set = XmStringGetLtoR(str, XmSTRING_DEFAULT_CHARSET, &cs);
			if (set == False)
				cs = "";

			if (wroteStr)
				buf[buflen++] = '\t';
			len = strlen(cs);
			/* allocate if string plus tab plus new-line plus 0 if too large */
			while (len + buflen + 3 > bufsize)
				{
				bufsize *= 2;
				buf = (char *)realloc(buf, bufsize);
				}
			strcpy(&buf[buflen], cs);
			buflen += len;
			if (set == True)
				XtFree(cs);
			wroteStr = 1;
			}
		if (wroteStr)
			buf[buflen++] = '\n';
		}
	if (!buflen)
		{
		free((char *)buf);
		return 0;
		}
	buf[buflen - 1] = 0;
	return buf;
	}

Boolean Copy(g, time, selected, row, col, nrow, ncol)
XmLGridWidget g;
Time time;
int selected;
int row, col;
int nrow, ncol;
	{
	int i;
	long itemID;
	Display *dpy;
	Window win;
	XmString clipStr;
	char *buf;
#ifdef MOTIF11
	int dataID;
#else
	long ldataID;
#endif

	if (!XtIsRealized((Widget)g))
		{
		XmLWarning((Widget)g, "Copy() - widget not realized");
		return False;
		}
	dpy = XtDisplay((Widget)g);
	win = XtWindow((Widget)g);
	buf = CopyDataCreate(g, selected, row, col, nrow, ncol);
	if (!buf)
		return False;
#ifdef MOTIF11
	clipStr = XmStringCreate("Grid Copy", XmSTRING_DEFAULT_CHARSET);
#else
	clipStr = XmStringCreateLocalized("Grid Copy");
#endif
	for (i = 0; i < 10000; i++)
		if (XmClipboardStartCopy(dpy, win, clipStr, time, NULL,
			NULL, &itemID) == ClipboardSuccess)
			break;
	XmStringFree(clipStr);
	if (i == 10000)
		{
		XmLWarning((Widget)g, "Copy() - start clipboard copy failed");
		return False;
		}
	for (i = 0; i < 10000; i++)
#ifdef MOTIF11
		if (XmClipboardCopy(dpy, win, itemID, "STRING", buf,
			(long)strlen(buf), 0, &dataID) == ClipboardSuccess)
#else
		if (XmClipboardCopy(dpy, win, itemID, "STRING", buf,
			(long)strlen(buf), 0, &ldataID) == ClipboardSuccess)
#endif
			break;
	free((char *)buf);
	if (i == 10000)
		{
		XmLWarning((Widget)g, "Copy() - clipboard copy transfer failed");
		return False;
		}
	for (i = 0; i < 10000; i++)
		if (XmClipboardEndCopy(dpy, win, itemID) == ClipboardSuccess)
			break;
	if (i == 10000)
		{
		XmLWarning((Widget)g, "Copy() - end clipboard copy failed");
		return False;
		}
	return True;
	}

Boolean Paste(g, row, col)
XmLGridWidget g;
int row, col;
	{
	Display *dpy;
	Window win;
	int i, res, done;
	unsigned long len, reclen;
	char *buf;

	if (!XtIsRealized((Widget)g))
		{
		XmLWarning((Widget)g, "Paste() - widget not realized");
		return False;
		}
	dpy = XtDisplay((Widget)g);
	win = XtWindow((Widget)g);
	for (i = 0; i < 10000; i++)
		if (XmClipboardInquireLength(dpy, win, "STRING", &len) ==
			ClipboardSuccess)
			break;
	if (i == 10000)
		{
		XmLWarning((Widget)g, "Paste() - cant retrieve clipboard length");
		return False;
		}
	buf = (char *)malloc((int)len + 1);
	done = 0;
	while (!done)
		{
		res = XmClipboardRetrieve(dpy, win, "STRING", buf, len + 1,
			&reclen, NULL);
		switch (res)
			{
			case ClipboardSuccess:
				done = 2;
				break;
			case ClipboardTruncate:
			case ClipboardNoData:
				done = 1;
				break;
			case ClipboardLocked:
				break;
			}
		}
	if (done != 2 || reclen != len)
		{
		free((char *)buf);
		XmLWarning((Widget)g, "Paste() - retrieve from clipboard failed");
		return False;
		}
	buf[len] = 0;
	Read(g, XmFORMAT_PASTE, 0, row, col, buf);
	free((char *)buf);
	return True;
	}

/*
   Utility
*/

static void GetCoreBackground(w, offset, value)
Widget w;
int offset;
XrmValue *value;
	{
	value->addr = (caddr_t)&w->core.background_pixel;
	}

static void GetManagerForeground(w, offset, value)
Widget w;
int offset;
XrmValue *value;
	{
	XmLGridWidget g;

	g = (XmLGridWidget)w;
	value->addr = (caddr_t)&g->manager.foreground;
	}

static void ClipRectToReg(g, rect, reg)
XmLGridWidget g;
XRectangle *rect;
GridReg *reg;
	{
	int i, st;
	XRectangle regRect;

	st = g->manager.shadow_thickness;
	if (!reg->width || !reg->height)
		i = XmLRectOutside;
	else 
		{
		regRect.x = reg->x + st;
		regRect.y = reg->y + st;
		regRect.width = reg->width - st * 2;
		regRect.height = reg->height - st * 2;
		i = XmLRectIntersect(rect, &regRect);
		}
	if (i == XmLRectInside)
		return;
	if (i == XmLRectOutside)
		{
		rect->width = 0;
		rect->height = 0;
		return;
		}
	if (rect->y + (int)rect->height - 1 >= reg->y + (int)reg->height - st)
		rect->height = reg->y + reg->height - rect->y - st;
	if (rect->x + (int)rect->width - 1 >= reg->x + (int)reg->width - st)
		rect->width = reg->x + reg->width - rect->x - st;
	if (rect->y < reg->y + st)
		{
		rect->height -= (reg->y + st) - rect->y;
		rect->y = reg->y + st;
		}
	if (rect->x < reg->x + st)
		{
		rect->width -= (reg->x + st) - rect->x;
		rect->x = reg->x + st;
		}
	}

static char *FileToString(file)
FILE *file;
	{
	long len, n;
	char *s;

	if (!file)
		return 0;
	fseek(file, 0L, 2);
	len = ftell(file);
	s = (char *)malloc((int)len + 1);
	if (!s)
		return 0;
	s[len] = 0;
	fseek(file, 0L, 0);
	n = fread(s, 1, (int)len, file);
	if (n != len)
		{
		free((char *)s);
		return 0;
		}
	return s;
	}

/*
   Actions, Callbacks and Handlers
*/

static void ButtonMotion(w, event, params, nparam)
Widget w;
XEvent *event;
String *params;
Cardinal *nparam;
	{
	XmLGridWidget g;
	XMotionEvent *me;
	char dragTimerSet;
	int row, col, x, y;

	if (event->type != MotionNotify)
		return;
	g = (XmLGridWidget)w;
	me = (XMotionEvent *)event;
	if (g->grid.inMode == InResize)
		{
		if (g->grid.resizeIsVert)
			DrawResizeLine(g, me->y, 0);
		else
			DrawResizeLine(g, me->x, 0);
		}

	/* drag scrolling */
	dragTimerSet = 0;
	if (g->grid.inMode == InSelect)
		{
		if (g->grid.vsPolicy == XmCONSTANT)
			{
			y = g->grid.reg[4].y;
			if (g->grid.selectionPolicy == XmSELECT_CELL &&
				g->grid.extendRow != -1 &&
				g->grid.extendCol != -1 &&
				RowPosToType(g, g->grid.extendRow) == XmHEADING)
				;
			else if (me->y < y)
				dragTimerSet |= DragUp;
			y += g->grid.reg[4].height;
			if (me->y > y)
				dragTimerSet |= DragDown;
			}
		if (g->grid.hsPolicy == XmCONSTANT)
			{
			x = g->grid.reg[4].x;
			if (g->grid.selectionPolicy == XmSELECT_CELL &&
				g->grid.extendCol != -1 &&
				g->grid.extendRow != -1 &&
				ColPosToType(g, g->grid.extendCol) == XmHEADING)
				;
			else if (me->x < x)
				dragTimerSet |= DragLeft;
			x += g->grid.reg[4].width;
			if (me->x > x)
				dragTimerSet |= DragRight;
			}
		}
	if (!g->grid.dragTimerSet && dragTimerSet)
		g->grid.dragTimerId = XtAppAddTimeOut(XtWidgetToApplicationContext(w),
			80, DragTimer, (caddr_t)g);
	else if (g->grid.dragTimerSet && !dragTimerSet)
		XtRemoveTimeOut(g->grid.dragTimerId);
	g->grid.dragTimerSet = dragTimerSet;

	/* Extend Selection */
	if (g->grid.inMode == InSelect && XYToRowCol(g, me->x, me->y,
		&row, &col) != -1)
		{
		if (g->grid.selectionPolicy == XmSELECT_MULTIPLE_ROW &&
			RowPosToType(g, row) == XmCONTENT)
			ExtendSelect(g, False, row, col);
		else if (g->grid.selectionPolicy == XmSELECT_CELL)
			ExtendSelect(g, False, row, col);
		}

	if (g->grid.inMode == InSelect &&
		g->grid.selectionPolicy == XmSELECT_BROWSE_ROW &&
		XYToRowCol(g, me->x, me->y, &row, &col) != -1)
		{
		if (RowPosToType(g, row) == XmCONTENT)
			{
			if (!SetFocus(g, row, col, 0, 1))
				SelectTypeArea(g, SelectRow, RowPosToTypePos(g, XmCONTENT,
					g->grid.focusRow), 0, True, True);
			}
		}
	}

static void DragTimer(clientData, intervalId)
XtPointer clientData;
XtIntervalId *intervalId;
	{
	Widget w;
	XmLGridWidget g;
	XRectangle rect;
	XmLGridRow rowp;
	XmLGridColumn colp;
	int r, c, min, max, inc, pi, ss, value, newValue;
	int extRow, extCol;

	g = (XmLGridWidget)clientData;
	w = (Widget)g;
	extRow = -1;
	extCol = -1;
	if (g->grid.vsPolicy == XmCONSTANT && ((g->grid.dragTimerSet & DragUp) ||
		(g->grid.dragTimerSet & DragDown)))
		{
		XtVaGetValues(g->grid.vsb,
			XmNminimum, &min,
			XmNmaximum, &max,
			XmNvalue, &value,
			XmNsliderSize, &ss,
			XmNincrement, &inc,
			XmNpageIncrement, &pi,
			NULL);
		newValue = value;
		if (g->grid.dragTimerSet & DragUp)
			newValue--;
		else
			newValue++;
		if (newValue != value && newValue >= min && newValue <= (max - ss))
			{
			XmScrollBarSetValues(g->grid.vsb, newValue, ss, inc, pi, True);
			r = g->grid.reg[4].row;
			if (g->grid.dragTimerSet & DragDown)
				r += g->grid.reg[4].nrow - 1;
			/* simple check to make sure row selected is totally visible */
			if (g->grid.reg[4].nrow)
				{
				rowp = (XmLGridRow)XmLArrayGet(g->grid.rowArray, r);
				if (rowp && !RowColToXY(g, r, 0, &rect))
					{
					if (XmLGridRowGetHeightInPixels(rowp) != rect.height)
						r--;
					}
				}
			if (g->grid.selectionPolicy == XmSELECT_BROWSE_ROW)
				{
				if (!SetFocus(g, r, g->grid.focusCol, -1, 1))
					SelectTypeArea(g, SelectRow, RowPosToTypePos(g,
						XmCONTENT, g->grid.focusRow), 0, True, True);
				}
			else if (g->grid.selectionPolicy == XmSELECT_MULTIPLE_ROW)
				ExtendSelect(g, False, r, g->grid.focusCol);
			else if (g->grid.selectionPolicy == XmSELECT_CELL)
				{
				extRow = r;
				extCol = g->grid.extendToCol;
				}
			}
		}
	if (g->grid.hsPolicy == XmCONSTANT && ((g->grid.dragTimerSet & DragLeft) ||
		(g->grid.dragTimerSet & DragRight)))
		{
		XtVaGetValues(g->grid.hsb,
			XmNminimum, &min,
			XmNmaximum, &max,
			XmNvalue, &value,
			XmNsliderSize, &ss,
			XmNincrement, &inc,
			XmNpageIncrement, &pi,
			NULL);
		newValue = value;
		if (g->grid.dragTimerSet & DragLeft)
			newValue--;
		else
			newValue++;
		if (newValue != value && newValue >= min && newValue <= (max - ss))
			{
			XmScrollBarSetValues(g->grid.hsb, newValue, ss, inc, pi, True);
			c = g->grid.reg[4].col;
			if (g->grid.dragTimerSet & DragRight)
				c += g->grid.reg[4].ncol - 1;
			if (g->grid.selectionPolicy == XmSELECT_CELL)
				{
				/* simple check to make sure col selected is totally visible */
				if (g->grid.reg[4].ncol)
					{
					colp = (XmLGridColumn)XmLArrayGet(g->grid.colArray, c);
					if (colp && !RowColToXY(g, c, 0, &rect))
						{
						if (XmLGridColumnGetWidthInPixels(colp) !=
							rect.width)
							c--;
						}
					}
				if (extRow == -1)
					extRow = g->grid.extendToRow;
				extCol = c;
				}
			}
		}
	if (extRow != -1 && extCol != -1)
		ExtendSelect(g, False, extRow, extCol);
	g->grid.dragTimerId = XtAppAddTimeOut(XtWidgetToApplicationContext(w),
		80, DragTimer, (caddr_t)g);
	}

static void CursorMotion(w, event, params, nparam)
Widget w;
XEvent *event;
String *params;
Cardinal *nparam;
	{
	XmLGridWidget g;
	XMotionEvent *me;
	int isVert, row, col;
	char defineCursor;

	if (event->type != MotionNotify)
		return;
	g = (XmLGridWidget)w;
	me = (XMotionEvent *)event;
	defineCursor = CursorNormal;
	if (PosIsResize(g, me->x, me->y, &row, &col, &isVert))
		{
		if (isVert)
			defineCursor = CursorVResize;
		else
			defineCursor = CursorHResize;
		}
	DefineCursor(g, defineCursor);
	}

static void Edit(w, event, params, nparam)
Widget w;
XEvent *event;
String *params;
Cardinal *nparam;
	{
	XmLGridWidget g;

	g = (XmLGridWidget)XtParent(w);
	TextAction(g, TEXT_EDIT_INSERT);
	}

static void EditCancel(w, event, params, nparam)
Widget w;
XEvent *event;
String *params;
Cardinal *nparam;
	{
	XmLGridWidget g;

	g = (XmLGridWidget)XtParent(w);
	TextAction(g, TEXT_EDIT_CANCEL);
	}

static void EditComplete(w, event, params, nparam)
Widget w;
XEvent *event;
String *params;
Cardinal *nparam;
	{
	XmLGridWidget g;

	g = (XmLGridWidget)XtParent(w);
	TextAction(g, TEXT_EDIT_COMPLETE);
	if (*nparam == 1)
		Traverse(w, event, params, nparam);
	}

#ifndef MOTIF11
#if defined(_STDC_) || defined(__cplusplus) || defined(c_plusplus)
extern Widget _XmGetTextualDragIcon(Widget);
#else
extern Widget _XmGetTextualDragIcon();
#endif
#endif

static void DragStart(w, event, params, nparam)
Widget w;
XEvent *event;
String *params;
Cardinal *nparam;
	{
#ifndef MOTIF11
	XmLGridWidget g;
	Widget dragIcon;
	Atom exportTargets[1];
	Arg args[10];
	char *data;
	XButtonEvent *be;
	int row, col;
	XmLGridColumn colp;
	XmLGridRow rowp;
	XmLGridCell cell;
	static XtCallbackRec dragFinish[2] =
		{ { DragFinish, NULL }, { NULL, NULL } };

	g = (XmLGridWidget)w;
	be = (XButtonEvent *)event;
	if (!g->grid.allowDrag || !be)
		return;
	if (XYToRowCol(g, be->x, be->y, &row, &col) == -1)
		return;
	rowp = (XmLGridRow)XmLArrayGet(g->grid.rowArray, row);
	colp = (XmLGridColumn)XmLArrayGet(g->grid.colArray, col);
	cell = GetCell(g, row, col);
	if (!rowp || !colp || !cell)
		return;
	if (XmLGridRowIsSelected(rowp) == False &&
		XmLGridColumnIsSelected(colp) == False &&
		XmLGridCellIsSelected(cell) == False)
		return;
	data = CopyDataCreate(g, 1, 0, 0, 0, 0);
	if (!data)
		return;
	dragIcon = _XmGetTextualDragIcon((Widget)w);
	exportTargets[0] = XA_STRING;
	dragFinish[0].closure = (XtPointer)data;
	XtSetArg(args[0], XmNconvertProc, DragConvert);
	XtSetArg(args[1], XmNexportTargets, exportTargets);
	XtSetArg(args[2], XmNnumExportTargets, 1);
	XtSetArg(args[3], XmNdragOperations, XmDROP_COPY);
	XtSetArg(args[4], XmNsourceCursorIcon, dragIcon);
	XtSetArg(args[4], XmNclientData, (XtPointer)data);
	XtSetArg(args[5], XmNdragDropFinishCallback, dragFinish);
	XmDragStart(w, event, args, 6);
#endif
	}

static Boolean DragConvert(w, selection, target, type, value, length, format)
Widget w;
Atom *selection, *target, *type;
XtPointer *value;
unsigned long *length;
int *format;
	{
#ifdef MOTIF11
	return FALSE;
#else
	Atom targetsA, timestampA, multipleA, *exportTargets;
	int n;
	char *data, *dataCopy;

	if (!XmIsDragContext(w))
		return FALSE;
	targetsA = XInternAtom(XtDisplay(w), "TARGETS", FALSE);
	timestampA = XInternAtom(XtDisplay(w), "TIMESTAMP", FALSE);
	multipleA = XInternAtom(XtDisplay(w), "MULTIPLE", FALSE);
	if (*target == targetsA)
		{
		n = 4;
		exportTargets = (Atom *)XtMalloc(sizeof(Atom) * n);
		exportTargets[0] = XA_STRING;
		exportTargets[1] = targetsA;
		exportTargets[2] = multipleA;
		exportTargets[3] = timestampA;
		*type = XA_ATOM;
		*value = (XtPointer)exportTargets;
		*format = 32;
		*length = (n * sizeof(Atom)) >> 2;
		return TRUE;
		}
	else if (*target == XA_STRING)
		{
		XtVaGetValues(w, XmNclientData, &data, NULL);
  		*type = XA_STRING;
		dataCopy = XtMalloc(strlen(data));
		strncpy(dataCopy, data, strlen(data));
  		*value = (XtPointer)dataCopy;
  		*length = strlen(data);
  		*format = 8;
  		return TRUE;
		}
	return FALSE;
#endif
	}

static void DragFinish(w, clientData, callData)
Widget w;
XtPointer clientData, callData;
	{
	free ((char *)clientData);
	}

static void DropRegister(g, set)
XmLGridWidget g;
Boolean set;
	{
#ifndef MOTIF11
	Atom importTargets[1];
	Arg args[4];

	if (set == True)
		{
		importTargets[0] = XA_STRING;
		XtSetArg(args[0], XmNdropSiteOperations, XmDROP_COPY);
		XtSetArg(args[1], XmNimportTargets, importTargets);
		XtSetArg(args[2], XmNnumImportTargets, 1);
		XtSetArg(args[3], XmNdropProc, DropStart);
		XmDropSiteRegister((Widget)g, args, 4);
		}
	else
		XmDropSiteUnregister((Widget)g);
#endif
	}

static void DropStart(w, clientData, callData)
Widget w;
XtPointer clientData, callData;
	{
#ifndef MOTIF11
	XmLGridWidget g;
	XmDropProcCallbackStruct *cbs;
	XmDropTransferEntryRec te[2];
	static XPoint pos;
	Atom *exportTargets;
	Arg args[10];
	int row, col, i, n, valid;

	g = (XmLGridWidget)w;
	cbs = (XmDropProcCallbackStruct *)callData;
	if (g->grid.allowDrop == False || cbs->dropAction == XmDROP_HELP)
		{
		cbs->dropSiteStatus = XmINVALID_DROP_SITE;
		return;
		}
	valid = 0;
	if (XYToRowCol(g, cbs->x, cbs->y, &row, &col) != -1 &&
		cbs->dropAction == XmDROP && cbs->operation == XmDROP_COPY)
		{
		XtVaGetValues(cbs->dragContext,
			XmNexportTargets, &exportTargets,
			XmNnumExportTargets, &n,
			NULL);
		for (i = 0; i < n; i++)
			if (exportTargets[i] == XA_STRING)
				valid = 1;
		}
	if (!valid)
		{
		cbs->operation = (long)XmDROP_NOOP;
		cbs->dropSiteStatus = XmINVALID_DROP_SITE;
		XtSetArg(args[0], XmNtransferStatus, XmTRANSFER_FAILURE);
		XtSetArg(args[1], XmNnumDropTransfers, 0);
		XmDropTransferStart(cbs->dragContext, args, 2);
		return;
		}
	g->grid.dropLoc.row = row;
	g->grid.dropLoc.col = col;
	cbs->operation = (long)XmDROP_COPY;
	te[0].target = XA_STRING;
	te[0].client_data = (XtPointer)g;
	XtSetArg(args[0], XmNdropTransfers, te);
	XtSetArg(args[1], XmNnumDropTransfers, 1);
	XtSetArg(args[2], XmNtransferProc, DropTransfer);
	XmDropTransferStart(cbs->dragContext, args, 3);
#endif
	}

static void DropTransfer(w, clientData, selType, type, value, length, format)
Widget w;
XtPointer clientData;
Atom *selType, *type;
XtPointer value;
unsigned long *length;
int *format;
	{
#ifndef MOTIF11
	XmLGridWidget g;
	char *buf;
	int len;

	if (!value)
		return;
	g = (XmLGridWidget)clientData;
	len = (int)*length;
	if (len < 0)
		return;
	buf = (char *)malloc(len + 1);
	strncpy(buf, (char *)value, len);
	XtFree((char *)value);
	buf[len] = 0;
	Read(g, XmFORMAT_DROP, 0, g->grid.dropLoc.row, g->grid.dropLoc.col, buf);
	free((char *)buf);
#endif
	}

static void Select(w, event, params, nparam)
Widget w;
XEvent *event;
String *params;
Cardinal *nparam;
	{
	XmLGridWidget g;
	Display *dpy;
	Window win;
	static XrmQuark qACTIVATE, qBEGIN, qEXTEND, qEND;
	static XrmQuark qTOGGLE;
	static int quarksValid = 0;
	XrmQuark q;
	int isVert;
	int row, col, clickTime, resizeRow, resizeCol;
	XButtonEvent *be;
	XRectangle rect;
	XmLGridRow rowp;
	XmLGridCallbackStruct cbs;
	Boolean flag;

	if (*nparam != 1)
		return;

	memset (&cbs, 0, sizeof(cbs));
	cbs.event = event;

	if (XmLIsGrid(w))
		g = (XmLGridWidget)w;
	else
		g = (XmLGridWidget)XtParent(w);
	dpy = XtDisplay(g);
	win = XtWindow(g);
	if (!quarksValid)
		{
		qACTIVATE = XrmStringToQuark("ACTIVATE");
		qBEGIN = XrmStringToQuark("BEGIN");
		qEXTEND = XrmStringToQuark("EXTEND");
		qEND = XrmStringToQuark("END");
		qTOGGLE = XrmStringToQuark("TOGGLE");
		}
	q = XrmStringToQuark(params[0]);
	be = 0;
	if (event->type == KeyPress || event->type == KeyRelease)
		{
		row = g->grid.focusRow;
		col = g->grid.focusCol;
		}
	else /* Button */
		{
		be = (XButtonEvent *)event;
		if (XYToRowCol(g, be->x, be->y, &row, &col) == -1)
			{
			row = -1;
			col = -1;
			}
		}
	/* double click activate check */
	if (q == qBEGIN && be)
		{
		clickTime = XtGetMultiClickTime(dpy);
		if (row != -1 && col != -1 &&
			row == g->grid.lastSelectRow && col == g->grid.lastSelectCol &&
			(be->time - g->grid.lastSelectTime) < clickTime)
			q = qACTIVATE;
		g->grid.lastSelectRow = row;
		g->grid.lastSelectCol = col;
		g->grid.lastSelectTime = be->time;
		}
	else if (q == qBEGIN)
		g->grid.lastSelectTime = 0;

	if (q == qBEGIN && be && PosIsResize(g, be->x, be->y,
		&resizeRow, &resizeCol, &isVert))
		{
		g->grid.resizeIsVert = isVert;
		g->grid.inMode = InResize;
		g->grid.resizeLineXY = -1; 
		g->grid.resizeRow = resizeRow;
		g->grid.resizeCol = resizeCol;
		if (isVert)
			{
			DrawResizeLine(g, be->y, 0);
			DefineCursor(g, CursorVResize);
			}
		else
			{
			DrawResizeLine(g, be->x, 0);
			DefineCursor(g, CursorHResize);
			}
		}
	else if (q == qBEGIN || q == qEXTEND || q == qTOGGLE)
		{
		if (g->grid.inMode != InNormal)
			return;
		if (row == -1 || col == -1)
			return;
		if (RowPosToType(g, row) == XmCONTENT &&
			ColPosToType(g, col) == XmCONTENT)
			{
			TextAction(g, TEXT_EDIT_COMPLETE);
			if (q != qEXTEND)
				{
				SetFocus(g, row, col, 0, 1);
				ExtendSelect(g, False, -1, -1);
				}
			XmProcessTraversal(g->grid.text, XmTRAVERSE_CURRENT);
			}
		if (g->grid.selectionPolicy == XmSELECT_MULTIPLE_ROW &&
			RowPosToType(g, row) == XmCONTENT)
			{
			flag = True;
			rowp = (XmLGridRow)XmLArrayGet(g->grid.rowArray, row);
			if (q == qBEGIN && rowp && XmLGridRowIsSelected(rowp) == True)
				flag = False;
			if (q == qTOGGLE && rowp && XmLGridRowIsSelected(rowp) == True)
				flag = False;
			if (q == qBEGIN)
				SelectTypeArea(g, SelectRow, -1, 0, False, True);
			if (be && q == qEXTEND)
				ExtendSelect(g, False, row, col);
			else
				SelectTypeArea(g, SelectRow, RowPosToTypePos(g, XmCONTENT,
					row), 0, flag, True);
			}
		if (g->grid.selectionPolicy == XmSELECT_CELL)
			{
			if (q == qBEGIN)
				{
				SelectTypeArea(g, SelectCell, -1, -1, False, True);
				SelectTypeArea(g, SelectRow, -1, 0, False, True);
				SelectTypeArea(g, SelectCol, 0, -1, False, True);
				}
			else if (q == qTOGGLE)
				ExtendSelect(g, False, -1, -1);
			if (RowPosToType(g, row) == XmFOOTER ||
				ColPosToType(g, col) == XmFOOTER)
				ExtendSelect(g, False, -1, -1);
			if (be && q == qEXTEND)
				ExtendSelect(g, False, row, col);
			else if (RowPosToType(g, row) == XmCONTENT &&
				ColPosToType(g, col) == XmCONTENT)
				SelectTypeArea(g, SelectCell, RowPosToTypePos(g, XmCONTENT,
					row), ColPosToTypePos(g, XmCONTENT, col), True, True);
			else if (RowPosToType(g, row) == XmHEADING &&
				ColPosToType(g, col) == XmCONTENT)
				ExtendSelect(g, True, row, col);
			else if (ColPosToType(g, col) == XmHEADING &&
				RowPosToType(g, row) == XmCONTENT)
				ExtendSelect(g, True, row, col);
			}
		if (g->grid.selectionPolicy == XmSELECT_SINGLE_ROW &&
			RowPosToType(g, row) == XmCONTENT)
			{
			flag = True;
			rowp = (XmLGridRow)XmLArrayGet(g->grid.rowArray, row);
			if (rowp && XmLGridRowIsSelected(rowp) == True)
				flag = False;
			SelectTypeArea(g, SelectRow, RowPosToTypePos(g, XmCONTENT, row),
				0, flag, True);
			}
		if (g->grid.selectionPolicy == XmSELECT_BROWSE_ROW &&
			RowPosToType(g, row) == XmCONTENT)
			SelectTypeArea(g, SelectRow, RowPosToTypePos(g, XmCONTENT, row),
				0, True, True);
		if (g->grid.selectionPolicy == XmSELECT_NONE ||
			(g->grid.selectionPolicy == XmSELECT_BROWSE_ROW &&
			RowPosToType(g, row) != XmCONTENT) ||
			(g->grid.selectionPolicy == XmSELECT_SINGLE_ROW &&
			RowPosToType(g, row) != XmCONTENT) )
			{
			cbs.reason = XmCR_SELECT_CELL;
			cbs.columnType = ColPosToType(g, col);
			cbs.column = ColPosToTypePos(g, cbs.columnType, col);
			cbs.rowType = RowPosToType(g, row);
			cbs.row = RowPosToTypePos(g, cbs.rowType, row);
			XtCallCallbackList((Widget)g, g->grid.selectCallback,
				(XtPointer)&cbs);
			}
		g->grid.inMode = InSelect;
		}
	else if (q == qEND && g->grid.inMode == InResize)
		{
		int r, c, width, height;
		r = g->grid.resizeRow;
		c = g->grid.resizeCol;
		g->grid.resizeRow = -1;
		g->grid.resizeCol = -1;
		if (!RowColToXY(g, r, c, &rect))
			{
			if (g->grid.resizeIsVert)
				{
				cbs.rowType = RowPosToType(g, r);
				cbs.row = RowPosToTypePos(g, cbs.rowType, r);
				height = 0;
				if (g->grid.resizeLineXY > rect.y)
					height = g->grid.resizeLineXY - rect.y;
				XtVaSetValues((Widget)g,
					XmNrowType, cbs.rowType,
					XmNrow, cbs.row,
					XmNrowHeight, height,
					XmNrowSizePolicy, XmCONSTANT,
					NULL);
				cbs.reason = XmCR_RESIZE_ROW;
				}
			else
				{
				cbs.columnType = ColPosToType(g, c);
				cbs.column = ColPosToTypePos(g, cbs.columnType, c);
				width = 0;
				if (g->grid.resizeLineXY > rect.x)
					width = g->grid.resizeLineXY - rect.x;
				XtVaSetValues((Widget)g,
					XmNcolumnType, cbs.columnType,
					XmNcolumn, cbs.column,
					XmNcolumnWidth, width,
					XmNcolumnSizePolicy, XmCONSTANT,
					NULL);
				cbs.reason = XmCR_RESIZE_COLUMN;
				}
			XtCallCallbackList((Widget)g, g->grid.resizeCallback,
				(XtPointer)&cbs);
			}
		DrawResizeLine(g, 0, 2);
		DefineCursor(g, CursorNormal);
		g->grid.inMode = InNormal;
		}
	else if (q == qEND)
		{
		g->grid.inMode = InNormal;
		if (g->grid.dragTimerSet)
			XtRemoveTimeOut(g->grid.dragTimerId);
		g->grid.dragTimerSet = 0;
		}
	if (q == qACTIVATE)
		{
		cbs.reason = XmCR_ACTIVATE;
		cbs.columnType = ColPosToType(g, col);
		cbs.column = ColPosToTypePos(g, cbs.columnType, col);
		cbs.rowType = RowPosToType(g, row);
		cbs.row = RowPosToTypePos(g, cbs.rowType, row);
		XtCallCallbackList((Widget)g, g->grid.activateCallback,
			(XtPointer)&cbs);
		}
	}

static void Traverse(w, event, params, nparam)
Widget w;
XEvent *event;
String *params;
Cardinal *nparam;
	{
	XmLGridWidget g;
	static XrmQuark qDOWN, qEXTEND_DOWN, qEXTEND_LEFT;
	static XrmQuark qEXTEND_PAGE_DOWN, qEXTEND_PAGE_LEFT;
	static XrmQuark qEXTEND_PAGE_RIGHT, qEXTEND_PAGE_UP;
	static XrmQuark qEXTEND_RIGHT, qEXTEND_UP, qLEFT, qPAGE_DOWN;
	static XrmQuark qPAGE_LEFT, qPAGE_RIGHT, qPAGE_UP, qRIGHT;
	static XrmQuark qTO_BOTTOM, qTO_BOTTOM_RIGHT, qTO_LEFT;
	static XrmQuark qTO_RIGHT, qTO_TOP, qTO_TOP_LEFT, qUP;
	static int quarksValid = 0;
	int extend, focusRow, focusCol, rowDir, colDir;
	int rowLoc, colLoc, prevRowLoc, prevColLoc;
	int scrollRow, scrollCol, prevScrollRow, prevScrollCol;
	XrmQuark q;

	g = (XmLGridWidget)XtParent(w);
	if (*nparam != 1)
		return;
	if (!quarksValid)
		{
		qDOWN = XrmStringToQuark("DOWN");
		qEXTEND_DOWN = XrmStringToQuark("EXTEND_DOWN");
		qEXTEND_LEFT = XrmStringToQuark("EXTEND_LEFT");
		qEXTEND_PAGE_DOWN = XrmStringToQuark("EXTEND_PAGE_DOWN");
		qEXTEND_PAGE_LEFT = XrmStringToQuark("EXTEND_PAGE_LEFT");
		qEXTEND_PAGE_RIGHT = XrmStringToQuark("EXTEND_PAGE_RIGHT");
		qEXTEND_PAGE_UP = XrmStringToQuark("EXTEND_PAGE_UP");
		qEXTEND_RIGHT = XrmStringToQuark("EXTEND_RIGHT");
		qEXTEND_UP = XrmStringToQuark("EXTEND_UP");
		qLEFT = XrmStringToQuark("LEFT");
		qPAGE_DOWN = XrmStringToQuark("PAGE_DOWN");
		qPAGE_LEFT = XrmStringToQuark("PAGE_LEFT");
		qPAGE_RIGHT = XrmStringToQuark("PAGE_RIGHT");
		qPAGE_UP = XrmStringToQuark("PAGE_UP");
		qRIGHT = XrmStringToQuark("RIGHT");
		qTO_BOTTOM = XrmStringToQuark("TO_BOTTOM");
		qTO_BOTTOM_RIGHT = XrmStringToQuark("TO_BOTTOM_RIGHT");
		qTO_LEFT = XrmStringToQuark("TO_LEFT");
		qTO_RIGHT = XrmStringToQuark("TO_RIGHT");
		qTO_TOP = XrmStringToQuark("TO_TOP");
		qTO_TOP_LEFT = XrmStringToQuark("TO_TOP_LEFT");
		qUP = XrmStringToQuark("UP");
		quarksValid = 1;
		}
	q = XrmStringToQuark(params[0]);
	extend = 0;
	/* map the extends to their counterparts and set extend flag */
	if (q == qEXTEND_DOWN)
		{
		q = qDOWN;
		extend = 1;
		}
	else if (q == qEXTEND_LEFT)
		{
		q = qLEFT;
		extend = 1;
		}
	else if (q == qEXTEND_PAGE_DOWN)
		{
		q = qPAGE_DOWN;
		extend = 1;
		}
	else if (q == qEXTEND_PAGE_LEFT)
		{
		q = qPAGE_LEFT;
		extend = 1;
		}
	else if (q == qEXTEND_PAGE_RIGHT)
		{
		q = qPAGE_RIGHT;
		extend = 1;
		}
	else if (q == qEXTEND_PAGE_UP)
		{
		q = qPAGE_UP;
		extend = 1;
		}
	else if (q == qEXTEND_RIGHT)
		{
		q = qRIGHT;
		extend = 1;
		}
	else if (q == qEXTEND_UP)
		{
		q = qUP;
		extend = 1;
		}
	if (extend && g->grid.selectionPolicy != XmSELECT_MULTIPLE_ROW &&
		g->grid.selectionPolicy != XmSELECT_CELL)
		return;
	if (extend && g->grid.extendRow != -1 && g->grid.extendCol != -1)
		{
		focusRow = g->grid.extendToRow;
		focusCol = g->grid.extendToCol;
		}
	else
		{
		focusRow = g->grid.focusRow;
		focusCol = g->grid.focusCol;
		}
	rowDir = 0;
	colDir = 0;
	if (focusRow < g->grid.topFixedCount)
		prevRowLoc = 0;
	else if (focusRow >= XmLArrayGetCount(g->grid.rowArray) -
		g->grid.bottomFixedCount)
		prevRowLoc = 2;
	else
		prevRowLoc = 1;
	if (focusCol < g->grid.leftFixedCount)
		prevColLoc = 0;
	else if (focusCol >= XmLArrayGetCount(g->grid.colArray) -
		g->grid.rightFixedCount)
		prevColLoc = 2;
	else
		prevColLoc = 1;
	/* calculate new focus row, col and walk direction */
	if (q == qDOWN)
		{
		focusRow++;
		rowDir = 1;
		}
	else if (q == qLEFT)
		{
		focusCol--;
		colDir = -1;
		}
	else if (q == qPAGE_DOWN)
		{
		if (prevRowLoc == 1)
			focusRow = g->grid.reg[4].row + g->grid.reg[4].nrow - 1;
		if (focusRow == g->grid.focusRow)
			focusRow += 4;
		rowDir = 1;
		}
	else if (q == qPAGE_LEFT)
		{
		if (prevColLoc == 1)
			focusCol = g->grid.reg[4].col - 1;
		if (focusCol == g->grid.focusCol)
			focusCol -= 4;
		colDir = -1;
		}
	else if (q == qPAGE_RIGHT)
		{
		if (prevColLoc == 1)
			focusCol = g->grid.reg[4].col + g->grid.reg[4].ncol - 1;
		if (focusCol == g->grid.focusCol)
			focusCol += 4;
		colDir = 1;
		}
	else if (q == qPAGE_UP)
		{
		if (prevRowLoc == 1)
			focusRow = g->grid.reg[4].row - 1;
		if (focusRow == g->grid.focusRow)
			focusRow -= 4;
		rowDir = -1;
		}
	else if (q == qRIGHT)
		{
		focusCol++;
		colDir = 1;
		}
	else if (q == qTO_BOTTOM)
		{
		focusRow = XmLArrayGetCount(g->grid.rowArray) - 1;
		rowDir = -1;
		}
	else if (q == qTO_BOTTOM_RIGHT)
		{
		focusCol = XmLArrayGetCount(g->grid.colArray) - 1;
		focusRow = XmLArrayGetCount(g->grid.rowArray) - 1;
		rowDir = -1;
		colDir = -1;
		}
	else if (q == qTO_LEFT)
		{
		focusCol = 0;
		colDir = 1;
		}
	else if (q == qTO_RIGHT)
		{
		focusCol = XmLArrayGetCount(g->grid.colArray) - 1;
		colDir = -1;
		}
	else if (q == qTO_TOP)
		{
		focusRow = 0;
		rowDir = 1;
		}
	else if (q == qTO_TOP_LEFT)
		{
		focusCol = 0;
		focusRow = 0;
		rowDir = 1;
		colDir = 1;
		}
	else if (q == qUP)
		{
		focusRow -= 1;
		rowDir = -1;
		}
	if (!rowDir && !colDir)
		return;
	if (extend)
		{
		if (FindNextFocus(g, focusRow, focusCol, rowDir, colDir,
			&focusRow, &focusCol) == -1)
			return;
		ExtendSelect(g, False, focusRow, focusCol);
		}
	else
		{
		if (SetFocus(g, focusRow, focusCol, rowDir, colDir) == -1)
			return;
		ExtendSelect(g, False, -1, -1);
		focusRow = g->grid.focusRow;
		focusCol = g->grid.focusCol;
		if (g->grid.selectionPolicy == XmSELECT_CELL)
			{
			SelectTypeArea(g, SelectCell, -1, -1, False, True);
			SelectTypeArea(g, SelectRow, -1, 0, False, True);
			SelectTypeArea(g, SelectCol, 0, -1, False, True);
			}
		else if (g->grid.selectionPolicy == XmSELECT_BROWSE_ROW)
			SelectTypeArea(g, SelectRow, RowPosToTypePos(g, XmCONTENT,
				focusRow), 0, True, True);
		}
	scrollRow = -1;
	scrollCol = -1;
	if (q == qPAGE_UP)
		scrollRow = ScrollRowBottomPos(g, focusRow);
	else if (q == qPAGE_DOWN)
		scrollRow = focusRow;
	else if (q == qPAGE_LEFT)
		scrollCol = ScrollColRightPos(g, focusCol);
	else if (q == qPAGE_RIGHT)
		scrollCol = focusCol;
	else
		{
		if (focusRow < g->grid.topFixedCount)
			rowLoc = 0;
		else if (focusRow >= XmLArrayGetCount(g->grid.rowArray) -
			g->grid.bottomFixedCount)
			rowLoc = 2;
		else
			rowLoc = 1;
		if (prevRowLoc != 0 && rowLoc == 0)
			scrollRow = g->grid.reg[0].nrow;
		else if (prevRowLoc != 2 && rowLoc == 2)
			scrollRow = g->grid.reg[6].row - 1;

		if (focusCol < g->grid.leftFixedCount)
			colLoc = 0;
		else if (focusCol >= XmLArrayGetCount(g->grid.colArray) -
			g->grid.rightFixedCount)
			colLoc = 2;
		else
			colLoc = 1;
		if (prevColLoc != 0 && colLoc == 0)
			scrollCol = g->grid.reg[0].ncol;
		else if (prevColLoc != 2 && colLoc == 2)
			scrollCol = g->grid.reg[2].col - 1;
		}
	if (g->grid.vsPolicy == XmVARIABLE)
		;
	else if (scrollRow != -1)
		{
		prevScrollRow = g->grid.scrollRow;
		g->grid.scrollRow = scrollRow;
		VertLayout(g, 0);
		if (g->grid.scrollRow != prevScrollRow)
			DrawArea(g, DrawVScroll, 0, 0);
		}
	else
		MakeRowVisible(g, focusRow);
	if (g->grid.hsPolicy == XmVARIABLE)
		;
	else if (scrollCol != -1)
		{
		prevScrollCol = g->grid.scrollCol;
		g->grid.scrollCol = scrollCol;
		HorizLayout(g, 0);
		if (g->grid.scrollCol != prevScrollCol)
			DrawArea(g, DrawHScroll, 0, 0);
		}
	else
		MakeColVisible(g, focusCol);
	}

static void TextActivate(w, clientData, callData)
Widget w;
XtPointer clientData, callData;
	{
	XmLGridWidget g;
	XmAnyCallbackStruct *cbs;
	String params[1];
	Cardinal nparam;

	cbs = (XmAnyCallbackStruct *)callData;
	g = (XmLGridWidget)XtParent(w);
	if (g->grid.inEdit)
		{
		nparam = 0;
		EditComplete(w, cbs->event, params, &nparam);
		}
	else
		{
		params[0] = "ACTIVATE";
		nparam = 1;
		Select(XtParent(w), cbs->event, params, &nparam);
		}
	}

static void TextFocus(w, clientData, callData)
Widget w;
XtPointer clientData, callData;
	{
	XmLGridWidget g;
	XmAnyCallbackStruct *cbs;
	int focusRow, focusCol;

	cbs = (XmAnyCallbackStruct *)callData;
	g = (XmLGridWidget)XtParent(w);
	if (cbs->reason == XmCR_FOCUS)
		g->grid.focusIn = 1;
	else
		g->grid.focusIn = 0;
	focusRow = g->grid.focusRow;
	focusCol = g->grid.focusCol;
	if (focusRow != -1 && focusCol != -1)
		DrawArea(g, DrawCell, focusRow, focusCol);
	}

static void TextMapped(w, clientData, event, con)
Widget w;
XtPointer clientData;
XEvent *event;
Boolean *con;
	{
	XmLGridWidget g;

	g = (XmLGridWidget)(XtParent(w));
	if (event->type != MapNotify)
		return;
	if (g->grid.textHidden)
		XtUnmapWidget(g->grid.text);
	}

static void TextModifyVerify(w, clientData, callData)
Widget w;
XtPointer clientData, callData;
	{
	XmLGridWidget g;
	XmTextVerifyCallbackStruct *cbs;

	g = (XmLGridWidget)XtParent(w);
	cbs = (XmTextVerifyCallbackStruct *)callData;
	if (!cbs->event || g->grid.ignoreModifyVerify || g->grid.inEdit)
		return;
	TextAction(g, TEXT_EDIT);
	if (!g->grid.inEdit)
		cbs->doit = False;
	}

/*
   XmLArray
*/

XmLArray *XmLArrayNew()
	{
	XmLArray *array;

	array = (XmLArray *)malloc(sizeof(struct _XmLArrayRec));
	array->_count = 0;
	array->_size = 0;
	array->_items = 0;
	return array;
	}

void XmLArrayFree(array)
XmLArray *array;
	{
	if (array->_items)
		free((char *)array->_items);
	free((char *)array);
	}

void XmLArrayAdd(array, pos, count)
XmLArray *array;
int pos;
int count;
	{
	int i;
	void **items;

	if (count < 1)
		return;
	if (pos < 0 || pos > array->_count)
		pos = array->_count;
	if (array->_count + count >= array->_size)
		{
		array->_size = array->_count + count;
		items = (void **)malloc(sizeof(void *) * array->_size);
		if (array->_items)
			{
			for (i = 0; i < array->_count; i++)
				items[i] = array->_items[i];
			free((char *)array->_items);
			}
		array->_items = items;
		}
	for (i = array->_count + count - 1; i >= pos + count; i--)
		{
		array->_items[i] = array->_items[i - count];
		((XmLArrayItem *)array->_items[i])->_pos = i;
		}
	for (i = pos; i < pos + count; i++)
		array->_items[i] = 0;
	array->_count += count;
	}

int XmLArrayDel(array, pos, count)
XmLArray *array;
int pos;
int count;
	{
	int i;

	if (pos < 0 || pos + count > array->_count)
		return -1;
	for (i = pos; i < array->_count - count; i++)
		{
		array->_items[i] = array->_items[i + count];
		((XmLArrayItem *)array->_items[i])->_pos = i;
		}
	array->_count -= count;
	if (!array->_count)
		{
		if (array->_items)
			free((char *)array->_items);
		array->_items = 0;
		array->_size = 0;
		}
	return 0;
	}

int XmLArraySet(array, pos, item)
XmLArray *array;
int pos;
void *item;
	{
	if (pos < 0 || pos >= array->_count)
		return -1;
	if (array->_items[pos])
		fprintf(stderr, "XmLArraySet: warning: overwriting pointer\n");
	array->_items[pos] = item;
	((XmLArrayItem *)array->_items[pos])->_pos = pos;
	return 0;
	}

void *XmLArrayGet(array, pos)
XmLArray *array;
int pos;
	{
	if (pos < 0 || pos >= array->_count)
		return 0;
	return array->_items[pos];
	}

int XmLArrayGetCount(array)
XmLArray *array;
	{
	return array->_count;
	}

int XmLArrayMove(array, newPos, pos, count)
XmLArray *array;
int newPos;
int pos;
int count;
	{
	void **items;
	int i;

	if (count <= 0)
		return -1;
	if (newPos < 0 || newPos + count > array->_count)
		return -1;
	if (pos < 0 || pos + count > array->_count)
		return -1;
	if (pos == newPos)
		return 0;
	/* copy items to move */
	items = (void **)malloc(sizeof(void *) * count);
	for (i = 0; i < count; i++)
		items[i] = array->_items[pos + i];
	/* move real items around */
	if (newPos < pos)
		for (i = pos + count - 1; i >= newPos + count; i--)
			{
			array->_items[i] = array->_items[i - count];
			((XmLArrayItem *)array->_items[i])->_pos = i;
			}
	else
		for (i = pos; i < newPos; i++)
			{
			array->_items[i] = array->_items[i + count];
			((XmLArrayItem *)array->_items[i])->_pos = i;
			}
	/* move items copy back */
	for (i = 0; i < count; i++)
		{
		array->_items[newPos + i] = items[i];
		((XmLArrayItem *)array->_items[newPos + i])->_pos = newPos + i;
		}
	free((char *)items);
	return 0;
	}

int XmLArrayReorder(array, newPositions, pos, count)
XmLArray *array;
int *newPositions;
int pos;
int count;
	{
	int i;
	void **items;

	if (count <= 0)
		return -1;
	if (pos < 0 || pos + count > array->_count)
		return -1;
	for (i = 0; i < count; i++)
		{
		if (newPositions[i] < pos || newPositions[i] >= pos + count)
			return -1;
		}
	items = (void **)malloc(sizeof(void *) * count);
	for (i = 0; i < count; i++)
		items[i] = array->_items[newPositions[i]];
	for (i = 0; i < count; i++)
		{
		array->_items[pos + i] = items[i];
		((XmLArrayItem *)array->_items[pos + i])->_pos = pos + i;
		}
	free((char *)items);
	return 0;
	}

int XmLArraySort(array, compare, userData, pos, count)
XmLArray *array;
XmLArrayCompareFunc compare;
void *userData;
int pos;
int count;
	{
	int i;

	if (pos < 0 || pos + count > array->_count)
		return -1;
	XmLSort(&array->_items[pos], count, sizeof(void *),
		(XmLSortCompareFunc)compare, userData);
	for (i = pos; i < pos + count; i++)
		((XmLArrayItem *)array->_items[i])->_pos = i;
	return 0;
	}

/*
   XmLGridRow
*/

XmLGridRow XmLGridRowNew(grid)
Widget grid;
	{
	XmLGridRow row;

	row = (XmLGridRow)malloc(sizeof(struct _XmLGridRowRec));
	row->_grid = grid;
	row->_heightInPixels = 0;
	row->_heightInPixelsValid = 0;
	row->_values.height = 1;
	row->_values.selected = False;
	row->_values.sizePolicy = XmVARIABLE;
	row->_values.userData = 0;
	row->_cellArray = XmLArrayNew();
	return row;
	}

void XmLGridRowFree(row)
XmLGridRow row;
	{
	int i, count;

	count = XmLArrayGetCount(row->_cellArray);
	for (i = 0; i < count; i++)
		XmLGridCellFree((XmLGridCell)XmLArrayGet(row->_cellArray, i));
	XmLArrayFree(row->_cellArray);
	free ((char *)row);
	}

XmLArray *XmLGridRowCells(row)
XmLGridRow row;
	{
	return row->_cellArray;
	}

int XmLGridRowGetPos(row)
XmLGridRow row;
	{
	return row->_pos;
	}

XmLGridRowValues *XmLGridRowGetValues(row)
XmLGridRow row;
	{
	return &row->_values;
	}

int XmLGridRowGetVisPos(row)
XmLGridRow row;
	{
	return row->_visPos;
	}

Boolean XmLGridRowIsHidden(row)
XmLGridRow row;
	{
	if (!row->_values.height)
		return True;
	return False;
	}

Boolean XmLGridRowIsSelected(row)
XmLGridRow row;
	{
	return row->_values.selected;
	}

void XmLGridRowSetSelected(row, selected)
XmLGridRow row;
Boolean selected;
	{
	row->_values.selected = selected;
	}

void XmLGridRowSetVisPos(row, visPos)
XmLGridRow row;
int visPos;
	{
	row->_visPos = visPos;
	}

int XmLGridRowGetHeightInPixels(row)
XmLGridRow row;
	{
	int i, count;
	Dimension height, maxHeight;
	XmLGridCell cell;
	XmLGridCallbackStruct cbs;

	if (row->_values.sizePolicy == XmCONSTANT)
		return row->_values.height;
	if (!row->_values.height)
		return 0;
	if (!row->_heightInPixelsValid)
		{
		maxHeight = 0;
		count = XmLArrayGetCount(row->_cellArray);
		for (i = 0; i < count; i++)
			{
			cell = (XmLGridCell)XmLArrayGet(row->_cellArray, i);
			cbs.reason = XmCR_ROW_HEIGHT;
			cbs.object = (void *)row;
			height = XmLGridCellAction(cell, row->_grid, &cbs);
			if (height > maxHeight)
				maxHeight = height;
			}
		row->_heightInPixels = maxHeight;
		row->_heightInPixelsValid = 1;
		}
	return row->_heightInPixels;
	}

void XmLGridRowHeightChanged(row)
XmLGridRow row;
	{
	row->_heightInPixelsValid = 0;
	}

/*
   XmLGridColumn
*/

XmLGridColumn XmLGridColumnNew(grid)
Widget grid;
	{
	XmLGridColumn column;

	column = (XmLGridColumn)malloc(sizeof(struct _XmLGridColumnRec));
	column->_grid = grid;
	column->_widthInPixels = 0;
	column->_widthInPixelsValid = 0;
	column->_values.selected = False;
	column->_values.sizePolicy = XmVARIABLE;
	column->_values.width = 8;
	column->_values.userData = 0;
	return column;
	}

void XmLGridColumnFree(column)
XmLGridColumn column;
	{
	free((char *)column);
	}

XmLGridColumnValues *XmLGridColumnGetValues(column)
XmLGridColumn column;
	{
	return &column->_values;
	}

int XmLGridColumnGetPos(column)
XmLGridColumn column;
	{
	return column->_pos;
	}

int XmLGridColumnGetVisPos(column)
XmLGridColumn column;
	{
	return column->_visPos;
	}

Boolean XmLGridColumnIsHidden(column)
XmLGridColumn column;
	{
	if (!column->_values.width)
		return True;
	return False;
	}

Boolean XmLGridColumnIsSelected(column)
XmLGridColumn column;
	{
	return column->_values.selected;
	}

void XmLGridColumnSetSelected(column, selected)
XmLGridColumn column;
Boolean selected;
	{
	column->_values.selected = selected;
	}

void XmLGridColumnSetVisPos(column, visPos)
XmLGridColumn column;
int visPos;
	{
	column->_visPos = visPos;
	}

int XmLGridColumnGetWidthInPixels(column)
XmLGridColumn column;
	{
	int i, count;
	Dimension width, maxWidth;
	XmLGridCell cell;
	XmLGridWidget g;
	XmLGridCallbackStruct cbs;

	if (column->_values.sizePolicy == XmCONSTANT)
		return column->_values.width;
	if (!column->_values.width)
		return 0;
	if (!column->_widthInPixelsValid)
		{
		maxWidth = 0;
		g = (XmLGridWidget)column->_grid;
		count = XmLArrayGetCount(g->grid.rowArray);
		for (i = 0; i < count; i++)
			{
			cell = GetCell(g, i, column->_pos);
			cbs.reason = XmCR_COLUMN_WIDTH;
			cbs.object = (void *)column;
			width = XmLGridCellAction(cell, column->_grid, &cbs);
			if (width > maxWidth)
				maxWidth = width;
			}
		column->_widthInPixels = maxWidth;
		column->_widthInPixelsValid = 1;
		}
	return column->_widthInPixels;
	}

void XmLGridColumnWidthChanged(column)
XmLGridColumn column;
	{
	column->_widthInPixelsValid = 0;
	}

/*
   XmLGridCell
*/


XmLGridCell XmLGridCellNew()
	{
	XmLGridCell c;

	c = (XmLGridCell)malloc(sizeof(struct _XmLGridCellRec));
	c->_refValues = 0;
	c->_flags = 0;
	return c;
	}

void XmLGridCellFree(cell)
XmLGridCell cell;
	{
	XmLGridCallbackStruct cbs;

	cbs.reason = XmCR_FREE_VALUE;
	XmLGridCellAction(cell, 0, &cbs);
	XmLGridCellDerefValues(cell->_refValues);
	free((char *)cell);
	}

int XmLGridCellAction(cell, w, cbs)
XmLGridCell cell;
Widget w;
XmLGridCallbackStruct *cbs;
	{
	int ret;

	ret = 0;
	switch (cbs->reason)
		{
		case XmCR_CELL_DRAW:
			XmLGridCellDrawBackground(cell, w, cbs->clipRect, cbs->drawInfo);
			XmLGridCellDrawValue(cell, w, cbs->clipRect, cbs->drawInfo);
			XmLGridCellDrawBorders(cell, w, cbs->clipRect, cbs->drawInfo);
			break;
		case XmCR_CELL_FOCUS_IN:
			break;
		case XmCR_CELL_FOCUS_OUT:
			break;
		case XmCR_COLUMN_WIDTH:
			ret = XmLGridCellGetWidth(cell, (XmLGridColumn)cbs->object);
			break;
		case XmCR_EDIT_BEGIN:
			ret = XmLGridCellBeginTextEdit(cell, w, cbs->clipRect);
			break;
		case XmCR_EDIT_CANCEL:
			break;
		case XmCR_EDIT_COMPLETE:
			XmLGridCellCompleteTextEdit(cell, w);
			break;
		case XmCR_EDIT_INSERT:
			ret = XmLGridCellBeginTextEdit(cell, w, cbs->clipRect);
			if (ret)
				XmLGridCellInsertText(cell, w);
			break;
		case XmCR_FREE_VALUE:
			XmLGridCellFreeValue(cell);
			break;
		case XmCR_ROW_HEIGHT:
			ret = XmLGridCellGetHeight(cell, (XmLGridRow)cbs->object);
			break;
		}
	return ret;
	}

XmLGridCellRefValues *XmLGridCellGetRefValues(cell)
XmLGridCell cell;
	{
	return cell->_refValues;
	}

void XmLGridCellSetRefValues(cell, values)
XmLGridCell cell;
XmLGridCellRefValues *values;
	{
	values->refCount++;
	XmLGridCellDerefValues(cell->_refValues);
	cell->_refValues = values;
	}

void XmLGridCellDerefValues(values)
XmLGridCellRefValues *values;
	{
	if (!values)
		return;
	values->refCount--;
	if (!values->refCount)
		{
		XmFontListFree(values->fontList);
		free((char *)values);
		}
	}

Boolean XmLGridCellInRowSpan(cell)
XmLGridCell cell;
	{
	if (cell->_flags & XmLGridCellInRowSpanFlag)
		return True;
	return False;
	}

Boolean XmLGridCellInColumnSpan(cell)
XmLGridCell cell;
	{
	if (cell->_flags & XmLGridCellInColumnSpanFlag)
		return True;
	return False;
	}

Boolean XmLGridCellIsSelected(cell)
XmLGridCell cell;
	{
	if (cell->_flags & XmLGridCellSelectedFlag)
		return True;
	return False;
	}

Boolean XmLGridCellIsValueSet(cell)
XmLGridCell cell;
	{
	if (cell->_flags & XmLGridCellValueSetFlag)
		return True;
	return False;
	}

void XmLGridCellSetValueSet(cell, set)
XmLGridCell cell;
Boolean set;
	{
	cell->_flags |= XmLGridCellValueSetFlag;
	if (set != True)
		cell->_flags ^= XmLGridCellValueSetFlag;
	}

void XmLGridCellSetInRowSpan(cell, set)
XmLGridCell cell;
Boolean set;
	{
	cell->_flags |= XmLGridCellInRowSpanFlag;
	if (set != True)
		cell->_flags ^= XmLGridCellInRowSpanFlag;
	}

void XmLGridCellSetInColumnSpan(cell, set)
XmLGridCell cell;
Boolean set;
	{
	cell->_flags |= XmLGridCellInColumnSpanFlag;
	if (set != True)
		cell->_flags ^= XmLGridCellInColumnSpanFlag;
	}

void XmLGridCellSetSelected(cell, selected)
XmLGridCell cell;
Boolean selected;
	{
	cell->_flags |= XmLGridCellSelectedFlag;
	if (selected != True)
		cell->_flags ^= XmLGridCellSelectedFlag;
	}

void XmLGridCellSetString(cell, string, copy)
XmLGridCell cell;
XmString string;
Boolean copy;
	{
	if (cell->_refValues->type != XmTEXT_CELL &&
		cell->_refValues->type != XmLABEL_CELL)
		{
		fprintf(stderr, "not a text cell type\n");
		return; 
		}
	XmLGridCellFreeValue(cell);
	if (string)
		{
		if (copy == True)
			cell->_value = (void *)XmStringCopy(string);
		else
			cell->_value = (void *)string;
		XmLGridCellSetValueSet(cell, True);
		}
	else
		XmLGridCellSetValueSet(cell, False);
	}

XmString XmLGridCellGetString(cell)
XmLGridCell cell;
	{
	if (cell->_refValues->type != XmTEXT_CELL &&
		cell->_refValues->type != XmLABEL_CELL)
		{
		fprintf(stderr, "not a text/label cell\n");
		return 0;
		}
	if (XmLGridCellIsValueSet(cell) == False)
		return 0;
	return (XmString)cell->_value;
	}

void XmLGridCellSetPixmap(cell, pixmap, width, height)
XmLGridCell cell;
Pixmap pixmap;
Dimension width, height;
	{
	XmLGridCellPixmap *pix;

	if (cell->_refValues->type != XmPIXMAP_CELL)
		{
		fprintf(stderr, "not a pixmap cell type\n");
		return; 
		}
	if (XmLGridCellIsValueSet(cell) == False)
		{
		pix = (XmLGridCellPixmap *)malloc(sizeof(XmLGridCellPixmap));
		pix->width = 0;
		pix->height = 0;
		cell->_value = (void *)pix;
		XmLGridCellSetValueSet(cell, True);
		}
	else
		pix = (XmLGridCellPixmap *)cell->_value;
	pix->pixmap = pixmap;
	pix->height = height;
	pix->width = width;
	}

XmLGridCellPixmap *XmLGridCellGetPixmap(cell)
XmLGridCell cell;
	{
	if (cell->_refValues->type != XmPIXMAP_CELL)
		{
		fprintf(stderr, "not a pixmap cell type\n");
		return 0;
		}
	if (XmLGridCellIsValueSet(cell) == False)
		return 0;
	return (XmLGridCellPixmap *)cell->_value;
	}

void XmLGridCellDrawBackground(cell, w, clipRect, ds)
XmLGridCell cell;
Widget w;
XRectangle *clipRect;
XmLGridDrawStruct *ds;
	{
	Display *dpy;
	Window win;
	Pixel fg;

	dpy = XtDisplay(w);
	win = XtWindow(w);
	if (ds->isSelected == True)
		fg = ds->selectBackground;
	else
		fg = cell->_refValues->background;
	XSetForeground(dpy, ds->gc, fg);
	XFillRectangle(dpy, win, ds->gc, clipRect->x, clipRect->y,
		clipRect->width, clipRect->height);
	}

void XmLGridCellDrawBorders(cell, w, clipRect, ds)
XmLGridCell cell;
Widget w;
XRectangle *clipRect;
XmLGridDrawStruct *ds;
	{
	Display *dpy;
	Window win;
	GC gc;
	int x1, x2, y1, y2;
	int drawLeft, drawRight, drawBot, drawTop;
	XRectangle *cellRect;
	Pixel black, white;

	dpy = XtDisplay(w);
	win = XtWindow(w);
	gc = ds->gc;
	cellRect = ds->cellRect;
	x1 = clipRect->x;
	x2 = clipRect->x + clipRect->width - 1;
	y1 = clipRect->y;
	y2 = clipRect->y + clipRect->height - 1;
	drawLeft = 1;
	drawRight = 1;
	drawBot = 1;
	drawTop = 1;
	if (cellRect->x != clipRect->x)	
		drawLeft = 0;
	if (cellRect->x + cellRect->width != clipRect->x + clipRect->width)
		drawRight = 0;
	if (cellRect->y != clipRect->y)
		drawTop = 0;
	if (cellRect->y + cellRect->height != clipRect->y + clipRect->height)
		drawBot = 0;

	if (ds->hasFocus == True)
		{
		black = BlackPixelOfScreen(XtScreen(w));
		white = WhitePixelOfScreen(XtScreen(w));
		XSetFunction(dpy, gc, GXxor);
		XSetForeground(dpy, gc, black ^ white);
		if (drawTop)
			XDrawLine(dpy, win, gc, x1, y1, x2, y1);
		if (drawLeft)
			XDrawLine(dpy, win, gc, x1, y1 + 2, x1, y2);
		if (drawRight)
			XDrawLine(dpy, win, gc, x2, y1 + 2, x2, y2);
		if (drawBot)
			{
			if (drawRight)
				XDrawLine(dpy, win, gc, x1 + 2, y2, x2 - 2, y2);
			else
				XDrawLine(dpy, win, gc, x1 + 2, y2, x2, y2);
			}
		if (clipRect->width > 1 && clipRect->height > 1)
			{
			if (drawTop)
				XDrawLine(dpy, win, gc, x1, y1 + 1, x2, y1 + 1);
			if (drawLeft)
				XDrawLine(dpy, win, gc, x1 + 1, y1 + 2, x1 + 1, y2);
			if (drawRight)
				XDrawLine(dpy, win, gc, x2 - 1, y1 + 2, x2 - 1, y2);
			if (drawBot)
				{
				if (drawRight)
					XDrawLine(dpy, win, gc, x1 + 2, y2 - 1, x2 - 2, y2 - 1);
				else
					XDrawLine(dpy, win, gc, x1 + 2, y2 - 1, x2, y2 - 1);
				}	
			}
		XSetFunction(dpy, gc, GXcopy);
		return;
		}

	if (cell->_refValues->rightBorderType == XmBORDER_LINE && drawRight)
		{
		XSetForeground(dpy, gc, cell->_refValues->rightBorderPixel);
		XDrawLine(dpy, win, gc, x2, y1, x2, y2);
		}
	if (cell->_refValues->bottomBorderType == XmBORDER_LINE && drawBot)
		{
		XSetForeground(dpy, gc, cell->_refValues->bottomBorderPixel);
		XDrawLine(dpy, win, gc, x1, y2, x2, y2);
		}	
	if (cell->_refValues->topBorderType == XmBORDER_LINE && drawTop)
		{
		XSetForeground(dpy, gc, cell->_refValues->topBorderPixel);
		XDrawLine(dpy, win, gc, x1, y1, x2, y1);
		}
	if (cell->_refValues->leftBorderType == XmBORDER_LINE && drawLeft)
		{
		XSetForeground(dpy, gc, cell->_refValues->leftBorderPixel);
		XDrawLine(dpy, win, gc, x1, y1, x1, y2);
		}
	}

void XmLGridCellDrawValue(cell, w, clipRect, ds)
XmLGridCell cell;
Widget w;
XRectangle *clipRect;
XmLGridDrawStruct *ds;
	{
	int type;
	Pixel fg;
	Display *dpy;
	XmLGridCellPixmap *pix;

	if (XmLGridCellIsValueSet(cell) == False)
		return;
	type = cell->_refValues->type;
	if (type == XmTEXT_CELL || type == XmLABEL_CELL)
		{
		dpy = XtDisplay(w);
		if (ds->isSelected == True)
			fg = ds->selectForeground;
		else
			fg = cell->_refValues->foreground;
		XSetForeground(dpy, ds->gc, fg);
		XmLStringDraw(w, (XmString)cell->_value, ds->stringDirection,
			cell->_refValues->fontList, cell->_refValues->alignment, ds->gc,
			ds->cellRect, clipRect);
		}	
	else if (type == XmPIXMAP_CELL)
		{
		pix = (XmLGridCellPixmap *)cell->_value;
		XmLPixmapDraw(w, pix->pixmap, pix->width, pix->height,
			cell->_refValues->alignment, ds->gc, ds->cellRect, clipRect);
		}
	}

int XmLGridCellBeginTextEdit(cell, w, clipRect)
XmLGridCell cell;
Widget w;
XRectangle *clipRect;
	{
	Widget text;

	if (cell->_refValues->type != XmTEXT_CELL)
		return 0;
	XtVaGetValues(w,
		XmNtextWidget, &text,
		NULL);
	XtVaSetValues(text,
		XmNfontList, cell->_refValues->fontList,
		NULL);
	return 1;
	}

void XmLGridCellCompleteTextEdit(cell, w)
XmLGridCell cell;
Widget w;
	{
	Widget text;
	char *s;
	int i, len;

	if (cell->_refValues->type != XmTEXT_CELL)
		return;
	XtVaGetValues(w,
		XmNtextWidget, &text,
		NULL);
	s = XmTextGetString(text);
	len = strlen(s);
	for (i = 0; i < len - 1; i++)
		if (s[i] == '\\' && s[i + 1] == 'n')
			{
			s[i] = '\n';
			strcpy(&s[i + 1], &s[i + 2]);
			}
	if (XmLGridCellIsValueSet(cell) == True)
		XmStringFree((XmString)cell->_value);
	if (strlen(s))
		{
#ifdef MOTIF11
		cell->_value = (void *)XmStringCreateLtoR(s, XmSTRING_DEFAULT_CHARSET);
#else
		cell->_value = (void *)XmStringCreateLocalized(s);
#endif
		XmLGridCellSetValueSet(cell, True);
		}
	else
		XmLGridCellSetValueSet(cell, False);
	XtFree(s);
	}

void XmLGridCellInsertText(cell, w)
XmLGridCell cell;
Widget w;
	{
	char *s;
	Widget text;

	if (cell->_refValues->type != XmTEXT_CELL)
		return;
	XtVaGetValues(w,
		XmNtextWidget, &text,
		NULL);
	if (XmLGridCellIsValueSet(cell) == True)
		{
		XmStringGetLtoR((XmString)cell->_value, XmSTRING_DEFAULT_CHARSET, &s);
		XmTextSetString(text, s);
		XtFree(s);
		}
	else
		XmTextSetString(text, "");
	XmTextSetInsertionPosition(text, XmTextGetLastPosition(text));
	XmProcessTraversal(text, XmTRAVERSE_CURRENT);
	}

int XmLGridCellGetHeight(cell, row)
XmLGridCell cell;
XmLGridRow row;
	{
	XmLGridCellPixmap *pix;
	XmLGridRowValues *rowValues;
	int height;

	height = -1;
	if (cell->_refValues->type == XmPIXMAP_CELL)
		{
		if (XmLGridCellIsValueSet(cell) == True)
			{
			pix = (XmLGridCellPixmap *)cell->_value;
			if (pix->height)
				height = pix->height + 4;
			}
		}
	else
		{
		rowValues = XmLGridRowGetValues(row);
		height = rowValues->height * cell->_refValues->fontHeight + 4;
		}
	if (height < 0 || cell->_refValues->rowSpan)
		height = 4;
	return height;
	}

int XmLGridCellGetWidth(cell, col)
XmLGridCell cell;
XmLGridColumn col;
	{
	XmLGridCellPixmap *pix;
	XmLGridColumnValues *colValues;
	int width;

	width = -1;
	colValues = XmLGridColumnGetValues(col);
	if (cell->_refValues->type == XmPIXMAP_CELL)
		{
		if (XmLGridCellIsValueSet(cell) == True)
			{
			pix = (XmLGridCellPixmap *)cell->_value;
			if (pix->width)
				width = pix->width + 4;
			}
		}
	else
		width = colValues->width * cell->_refValues->fontWidth + 4;
	if (width < 0 || cell->_refValues->columnSpan)
		width = 4;
	return width;
	}

void XmLGridCellFreeValue(cell)
XmLGridCell cell;
	{
	int type;

	if (XmLGridCellIsValueSet(cell) == False)
		return;
	type = cell->_refValues->type;
	if (type == XmTEXT_CELL || type == XmLABEL_CELL)
		XmStringFree((XmString)cell->_value);
	else if (type == XmPIXMAP_CELL)
		free((char *)cell->_value);
	XmLGridCellSetValueSet(cell, False);
	}

/*
   Public Functions
*/

Widget XmLCreateGrid(parent, name, arglist, argcount)
Widget parent;
char *name;
ArgList arglist;
Cardinal argcount;
	{
	return XtCreateWidget(name, xmlGridWidgetClass, parent,
		arglist, argcount);
	}

void XmLGridAddColumns(w, type, position, count)
Widget w;
unsigned char type;
int position;
int count;
	{
	XmLGridWidget g;
	XmLGridColumn col;
	XmLGridRow row;
	XmLGridCell cell;
	XmLGridCallbackStruct cbs;
	int i, j, rowCount, hasAddCB, redraw;

	if (!XmLIsGrid(w))
		{
		XmLWarning(w, "AddColumns() - widget not a XmLGrid");
		return;
		}
	g = (XmLGridWidget)w;
	if (count <= 0)
		return;
	redraw = 0;
	position = ColTypePosToPos(g, type, position, 1);
	if (position < 0)
		position = ColTypePosToPos(g, type, -1, 1);
	/* adjust count */
	if (type == XmHEADING)
		{
		g->grid.headingColCount += count;
		g->grid.leftFixedCount += count;
		redraw = 1;
		}
	else if (type == XmFOOTER)
		{
		g->grid.footerColCount += count;
		g->grid.rightFixedCount += count;
		redraw = 1;
		}
	else /* XmCONTENT */
		g->grid.colCount += count;

	hasAddCB = 0;
	if (XtHasCallbacks(w, XmNaddCallback) == XtCallbackHasSome)
		hasAddCB = 1;
	/* add columns */
	XmLArrayAdd(g->grid.colArray, position, count);
	for (i = 0; i < count; i++)
		{
		col = 0;
		if (hasAddCB)
			{
			cbs.reason = XmCR_ADD_COLUMN;
			cbs.columnType = type;
			cbs.object = 0;
			XtCallCallbackList(w, g->grid.addCallback, (XtPointer)&cbs);
			col = (XmLGridColumn)cbs.object;
			}
		if (!col)
			col = XmLGridColumnNew(w);
		XmLArraySet(g->grid.colArray, position + i, col);
		}
	/* add cells */
	rowCount = XmLArrayGetCount(g->grid.rowArray);
	for (j = 0; j < rowCount; j++)
		{
		row = (XmLGridRow)XmLArrayGet(g->grid.rowArray, j);
		XmLArrayAdd(XmLGridRowCells(row), position, count);
		for (i = position; i < position + count; i++)
			{
			cell = 0;
			if (hasAddCB)
				{
				cbs.reason = XmCR_ADD_CELL;
				cbs.rowType = RowPosToType(g, j);
				cbs.columnType = type;
				cbs.object = 0;
				XtCallCallbackList(w, g->grid.addCallback, (XtPointer)&cbs);
				cell = (XmLGridCell)cbs.object;
				}
			if (!cell)
				cell = XmLGridCellNew();
			XmLGridCellSetRefValues(cell, g->grid.defCellValues);
			XmLArraySet(XmLGridRowCells(row), i, cell);
			}
		XmLGridRowHeightChanged(row);
		}
	for (i = position; i < position + count; i++)
		{
		col = (XmLGridColumn)XmLArrayGet(g->grid.colArray, i);
		XmLGridColumnWidthChanged(col);
		}
	/* sanity check */
	if (XmLArrayGetCount(g->grid.colArray) != g->grid.headingColCount +
		g->grid.colCount + g->grid.footerColCount)
			XmLWarning(w, "AddColumns() - count sanity check failed");

	/* maintain scroll and focus position */
	if (g->grid.hsPolicy == XmCONSTANT)
		{
		if (type == XmCONTENT && g->grid.colCount == count)
			g->grid.scrollCol = 0;
		else if (position <= g->grid.scrollCol)
			g->grid.scrollCol += count;
		}
	if (position <= g->grid.focusCol)
		g->grid.focusCol += count;
	g->grid.recalcHorizVisPos = 1;
	HorizLayout(g, 1);
	VertLayout(g, 1);
	if (g->grid.focusCol == -1 && type == XmCONTENT)
		{
		if (g->grid.focusRow != -1)
			SetFocus(g, g->grid.focusRow, position, 0, 0);
		else
			g->grid.focusCol = position;
		}	
	for (i = position; i < position + count; i++)
		redraw |= ColIsVisible(g, i);
	if (redraw)
		DrawArea(g, DrawAll, 0, 0);
	}

void XmLGridAddRows(w, type, position, count)
Widget w;
unsigned char type;
int position;
int count;
	{
	XmLGridWidget g;
	XmLGridRow row;
	XmLGridColumn col;
	XmLGridCell cell;
	XmLGridCallbackStruct cbs;
	int i, j, hasAddCB, redraw, colCount;

	if (!XmLIsGrid(w))
		{
		XmLWarning(w, "AddRows() - widget not a XmLGrid");
		return;
		}
	g = (XmLGridWidget)w;
	if (count <= 0)
		return;
	redraw = 0;
	position = RowTypePosToPos(g, type, position, 1);
	if (position < 0)
		position = RowTypePosToPos(g, type, -1, 1);
	/* adjust count */
	if (type == XmHEADING)
		{
		g->grid.headingRowCount += count;
		g->grid.topFixedCount += count;
		redraw = 1;
		}
	else if (type == XmFOOTER)
		{
		g->grid.footerRowCount += count;
		g->grid.bottomFixedCount += count;
		redraw = 1;
		}
	else /* XmCONTENT */
		g->grid.rowCount += count;

	/* add rows and cells */
	XmLArrayAdd(g->grid.rowArray, position, count);
	colCount = XmLArrayGetCount(g->grid.colArray);
	hasAddCB = 0;
	if (XtHasCallbacks(w, XmNaddCallback) == XtCallbackHasSome)
		hasAddCB = 1;
	for (i = 0; i < count; i++)
		{
		row = 0;
		if (hasAddCB)
			{
			cbs.reason = XmCR_ADD_ROW;
			cbs.rowType = type;
			cbs.object = 0;
			XtCallCallbackList(w, g->grid.addCallback, (XtPointer)&cbs);
			row = (XmLGridRow)cbs.object;
			}
		if (!row)
			row = XmLGridRowNew(w);
		XmLArraySet(g->grid.rowArray, position + i, row);
		XmLArrayAdd(XmLGridRowCells(row), 0, colCount);
		for (j = 0; j < colCount; j++)
			{
			cell = 0;
			if (hasAddCB)
				{
				cbs.reason = XmCR_ADD_CELL;
				cbs.rowType = type;
				cbs.columnType = ColPosToType(g, j);
				cbs.object = 0;
				XtCallCallbackList(w, g->grid.addCallback, (XtPointer)&cbs);
				cell = (XmLGridCell)cbs.object;
				}
			if (!cell)
				cell = XmLGridCellNew();
			XmLGridCellSetRefValues(cell, g->grid.defCellValues);
			XmLArraySet(XmLGridRowCells(row), j, cell);
			}
		XmLGridRowHeightChanged(row);
		}
	for (j = 0; j < colCount; j++)
		{
		col = (XmLGridColumn)XmLArrayGet(g->grid.colArray, j);
		XmLGridColumnWidthChanged(col);
		}
	/* sanity check */
	if (XmLArrayGetCount(g->grid.rowArray) != g->grid.headingRowCount +
		g->grid.rowCount + g->grid.footerRowCount)
			XmLWarning(w, "AddRows() - count sanity check failed");

	/* maintain scroll and focus position */
	if (g->grid.vsPolicy == XmCONSTANT)
		{
		if (type == XmCONTENT && g->grid.rowCount == count)
			g->grid.scrollRow = 0;
		else if (position <= g->grid.scrollRow)
			g->grid.scrollRow += count;
		}
	if (position <= g->grid.focusRow)
		g->grid.focusRow += count;
	g->grid.recalcVertVisPos = 1;
	HorizLayout(g, 1);
	VertLayout(g, 1);
	if (g->grid.focusRow == -1 && type == XmCONTENT)
		{
		if (g->grid.focusCol != -1)
			SetFocus(g, position, g->grid.focusCol, 0, 0);
		else
			g->grid.focusRow = position;
		}
	for (i = position; i < position + count; i++)
		redraw |= RowIsVisible(g, i);
	if (redraw)
		DrawArea(g, DrawAll, 0, 0);
	}

void XmLGridDeleteAllColumns(w, type)
Widget w;
unsigned char type;
	{
	XmLGridWidget g;
	int n;

	if (!XmLIsGrid(w))
		{
		XmLWarning(w, "DeleteAllColumns() - widget not a XmLGrid");
		return;
		}
	g = (XmLGridWidget)w;
	if (type == XmHEADING)
		n = g->grid.headingColCount;
	else if (type == XmCONTENT)
		n = g->grid.colCount;
	else if (type == XmFOOTER)
		n = g->grid.footerColCount;
	else
		{
		XmLWarning(w, "DeleteAllColumns() - invalid type");
		return;
		}
	if (!n)
		return;
	XmLGridDeleteColumns(w, type, 0, n);
	}

void XmLGridDeleteAllRows(w, type)
Widget w;
unsigned char type;
	{
	XmLGridWidget g;
	int n;

	if (!XmLIsGrid(w))
		{
		XmLWarning(w, "DeleteAllRows() - widget not a XmLGrid");
		return;
		}
	g = (XmLGridWidget)w;
	if (type == XmHEADING)
		n = g->grid.headingRowCount;
	else if (type == XmCONTENT)
		n = g->grid.rowCount;
	else if (type == XmFOOTER)
		n = g->grid.footerRowCount;
	else
		{
		XmLWarning(w, "DeleteAllRows() - invalid type");
		return;
		}
	if (!n)
		return;
	XmLGridDeleteRows(w, type, 0, n);
	}

void XmLGridDeselectAllRows(w, notify)
Widget w;
Boolean notify;
	{
	XmLGridWidget g;

	if (!XmLIsGrid(w))
		{
		XmLWarning(w, "DeselectAllRows() - widget not a XmLGrid");
		return;
		}
	g = (XmLGridWidget)w;
	SelectTypeArea(g, SelectRow, -1, 0, False, notify);
	}

void XmLGridDeselectAllColumns(w, notify)
Widget w;
Boolean notify;
	{
	XmLGridWidget g;

	if (!XmLIsGrid(w))
		{
		XmLWarning(w, "DeselectAllRows() - widget not a XmLGrid");
		return;
		}
	g = (XmLGridWidget)w;
	SelectTypeArea(g, SelectCol, 0, -1, False, notify);
	}

void XmLGridDeselectAllCells(w, notify)
Widget w;
Boolean notify;
	{
	XmLGridWidget g;

	if (!XmLIsGrid(w))
		{
		XmLWarning(w, "DeselectAllCells() - widget not a XmLGrid");
		return;
		}
	g = (XmLGridWidget)w;
	SelectTypeArea(g, SelectCell, -1, -1, False, notify);
	}

void XmLGridDeselectCell(w, row, column, notify)
Widget w;
int row, column;
Boolean notify;
	{
	XmLGridWidget g;

	if (!XmLIsGrid(w))
		{
		XmLWarning(w, "DeselectCell() - widget not a XmLGrid");
		return;
		}
	g = (XmLGridWidget)w;
	SelectTypeArea(g, SelectCell, row, column, False, notify);
	}

void XmLGridDeselectColumn(w, column, notify)
Widget w;
int column;
Boolean notify;
	{
	XmLGridWidget g;

	if (!XmLIsGrid(w))
		{
		XmLWarning(w, "DeselectColumn() - widget not a XmLGrid");
		return;
		}
	g = (XmLGridWidget)w;
	SelectTypeArea(g, SelectCol, 0, column, False, notify);
	}

void XmLGridDeselectRow(w, row, notify)
Widget w;
int row;
Boolean notify;
	{
	XmLGridWidget g;

	if (!XmLIsGrid(w))
		{
		XmLWarning(w, "DeselectRow() - widget not a XmLGrid");
		return;
		}
	g = (XmLGridWidget)w;
	SelectTypeArea(g, SelectRow, row, 0, False, notify);
	}

void XmLGridDeleteColumns(w, type, position, count)
Widget w;
unsigned char type;
int position;
int count;
	{
	XmLGridWidget g;
	XmLGridRow row;
	XmLGridColumn col;
	XmLGridCallbackStruct cbs;
	int changeFocus, i, j, rowCount, lastPos, redraw;

	if (!XmLIsGrid(w))
		{
		XmLWarning(w, "DeleteColumns() - widget not a XmLGrid");
		return;
		}
	g = (XmLGridWidget)w;
	if (count <= 0)
		return;
	lastPos = ColTypePosToPos(g, type, position + count - 1, 0);
	position = ColTypePosToPos(g, type, position, 0);
	if (position < 0 || lastPos < 0)
		{
		XmLWarning(w, "DeleteColumns() - invalid position");
		return;
		}
	changeFocus = 0;
	if (position <= g->grid.focusCol && lastPos >= g->grid.focusCol)
		{
		/* deleting focus col */
		TextAction(g, TEXT_EDIT_CANCEL);
		ChangeFocus(g, g->grid.focusRow, -1);
		changeFocus = 1;
		}
	redraw = 0;

	/* adjust count */
	if (type == XmHEADING)
		{
		g->grid.headingColCount -= count;
		g->grid.leftFixedCount -= count;
		redraw = 1;
		}
	else if (type == XmFOOTER)
		{
		g->grid.footerColCount -= count;
		g->grid.rightFixedCount -= count;
		redraw = 1;
		}
	else /* XmCONTENT */
		g->grid.colCount -= count;

	for (i = position; i < position + count; i++)
		{
		col = (XmLGridColumn)XmLArrayGet(g->grid.colArray, i);
		if (XmLGridColumnIsHidden(col) == True)
			g->grid.hiddenColCount--;
		redraw |= ColIsVisible(g, i);
		}
	if (XtHasCallbacks(w, XmNdeleteCallback) == XtCallbackHasSome)
		for (i = position; i < position + count; i++)
			{
			rowCount = XmLArrayGetCount(g->grid.rowArray);
			for (j = 0; j < rowCount; j++)
				{
				row = (XmLGridRow)XmLArrayGet(g->grid.rowArray, j);
				cbs.reason = XmCR_DELETE_CELL;
				cbs.rowType = RowPosToType(g, j);
				cbs.columnType = type;
				cbs.object = XmLArrayGet(XmLGridRowCells(row), i);
				XtCallCallbackList(w, g->grid.deleteCallback, (XtPointer)&cbs);
				}
			cbs.reason = XmCR_DELETE_COLUMN;
			cbs.columnType = type;
			cbs.object = XmLArrayGet(g->grid.colArray, i);
			XtCallCallbackList(w, g->grid.deleteCallback, (XtPointer)&cbs);
			}
	/* delete columns */
	for (i = position; i < position + count; i++)
		{
		col = (XmLGridColumn)XmLArrayGet(g->grid.colArray, i);
		XmLGridColumnFree(col);
		}
	XmLArrayDel(g->grid.colArray, position, count);
	/* delete cells */
	rowCount = XmLArrayGetCount(g->grid.rowArray);
	for (i = 0; i < rowCount; i++)
		{
		row = (XmLGridRow)XmLArrayGet(g->grid.rowArray, i);
		for (j = position; j < position + count; j++)
			XmLGridCellFree((XmLGridCell)XmLArrayGet(
				XmLGridRowCells(row), j));
		XmLArrayDel(XmLGridRowCells(row), position, count);
		XmLGridRowHeightChanged(row);
		}
	/* sanity check */
	if (XmLArrayGetCount(g->grid.colArray) != g->grid.headingColCount +
		g->grid.colCount + g->grid.footerColCount)
			XmLWarning(w, "DeleteColumns() - count sanity check failed");

	/* maintain scroll and focus position */
	if (g->grid.hsPolicy == XmCONSTANT)
		{
		if (lastPos < g->grid.scrollCol)
			g->grid.scrollCol -= count;
		else if (position <= g->grid.scrollCol)
			g->grid.scrollCol = position;
		}
	if (lastPos < g->grid.focusCol)
		g->grid.focusCol -= count;
	g->grid.recalcHorizVisPos = 1;
	HorizLayout(g, 1);
	VertLayout(g, 1);
	if (changeFocus)
		{
		SetFocus(g, g->grid.focusRow, position, 0, 1);
		if (g->grid.focusCol == -1)
			SetFocus(g, g->grid.focusRow, position, 0, -1);
		}
	if (redraw)
		DrawArea(g, DrawAll, 0, 0);
	}

void XmLGridDeleteRows(w, type, position, count)
Widget w;
unsigned char type;
int position;
int count;
	{
	XmLGridWidget g;
	XmLGridRow row;
	XmLGridColumn col;
	XmLGridCallbackStruct cbs;
	int changeFocus, i, j, colCount, lastPos, redraw;

	if (!XmLIsGrid(w))
		{
		XmLWarning(w, "DeleteRows() - widget not a XmLGrid");
		return;
		}
	g = (XmLGridWidget)w;
	if (count <= 0)
		return;
	lastPos = RowTypePosToPos(g, type, position + count - 1, 0);
	position = RowTypePosToPos(g, type, position, 0);
	if (position < 0 || lastPos < 0)
		{
		XmLWarning(w, "DeleteRows() - invalid position");
		return;
		}
	changeFocus = 0;
	if (position <= g->grid.focusRow && lastPos >= g->grid.focusRow)
		{
		/* deleting focus row */
		TextAction(g, TEXT_EDIT_CANCEL);
		ChangeFocus(g, -1, g->grid.focusCol);
		changeFocus = 1;
		}
	redraw = 0;

	/* adjust count */
	if (type == XmHEADING)
		{
		g->grid.headingRowCount -= count;
		g->grid.topFixedCount -= count;
		redraw = 1;
		}
	else if (type == XmFOOTER)
		{
		g->grid.footerRowCount -= count;
		g->grid.bottomFixedCount -= count;
		redraw = 1;
		}
	else /* XmCONTENT */
		g->grid.rowCount -= count;

	for (i = position; i < position + count; i++)
		{
		row = (XmLGridRow)XmLArrayGet(g->grid.rowArray, i);
		if (XmLGridRowIsHidden(row) == True)
			g->grid.hiddenRowCount--;
		redraw |= RowIsVisible(g, i);
		}
	if (XtHasCallbacks(w, XmNdeleteCallback) == XtCallbackHasSome)
		for (i = position; i < position + count; i++)
			{
			row = (XmLGridRow)XmLArrayGet(g->grid.rowArray, i);
			colCount = XmLArrayGetCount(g->grid.colArray);
			for (j = 0; j < colCount; j++)
				{
				cbs.reason = XmCR_DELETE_CELL;
				cbs.rowType = type;
				cbs.columnType = ColPosToType(g, j);
				cbs.object = XmLArrayGet(XmLGridRowCells(row), j);
				XtCallCallbackList(w, g->grid.deleteCallback, (XtPointer)&cbs);
				}
			cbs.reason = XmCR_DELETE_ROW;
			cbs.rowType = type;
			cbs.object = (void *)row;
			XtCallCallbackList(w, g->grid.deleteCallback, (XtPointer)&cbs);
			}
	/* delete rows and cells */
	for (i = position; i < position + count; i++)
		{
		row = (XmLGridRow)XmLArrayGet(g->grid.rowArray, i);
		XmLGridRowFree(row);
		}
	XmLArrayDel(g->grid.rowArray, position, count);
	colCount = XmLArrayGetCount(g->grid.colArray);
	for (i = 0; i < colCount; i++)
		{
		col = (XmLGridColumn)XmLArrayGet(g->grid.colArray, i);
		XmLGridColumnWidthChanged(col);
		}
	/* sanity check */
	if (XmLArrayGetCount(g->grid.rowArray) != g->grid.headingRowCount +
		g->grid.rowCount + g->grid.footerRowCount)
			XmLWarning(w, "DeleteRows() - count sanity check failed");

	/* maintain scroll and focus position */
	if (g->grid.vsPolicy == XmCONSTANT)
		{
		if (lastPos < g->grid.scrollRow)
			g->grid.scrollRow -= count;
		else if (position <= g->grid.scrollRow)
			g->grid.scrollRow = position;
		}
	if (lastPos < g->grid.focusRow)
		g->grid.focusRow -= count;
	g->grid.recalcVertVisPos = 1;
	HorizLayout(g, 1);
	VertLayout(g, 1);
	if (changeFocus)
		{
		SetFocus(g, position, g->grid.focusCol, 1, 0);
		if (g->grid.focusRow == -1)
			SetFocus(g, position, g->grid.focusCol, -1, 0);
		}
	if (redraw)
		DrawArea(g, DrawAll, 0, 0);
	}

void XmLGridGetFocus(w, row, column, focusIn)
Widget w;
int *row, *column;
Boolean *focusIn;
	{
	XmLGridWidget g;
	unsigned char rowType, colType;

	if (!XmLIsGrid(w))
		{
		XmLWarning(w, "GetFocus() - widget not a XmLGrid");
		return;
		}
	g = (XmLGridWidget)w;
	if (g->grid.focusIn)
		*focusIn = True;
	else
		*focusIn = False;
	if (g->grid.focusRow < 0 || g->grid.focusCol < 0)
		{
		*row = -1;
		*column = -1;
		return;
		}
	rowType = RowPosToType(g, g->grid.focusRow);
	colType = ColPosToType(g, g->grid.focusCol);
	if (rowType != XmCONTENT || colType != XmCONTENT)
		{
		*row = -1;
		*column = -1;
		XmLWarning(w, "GetFocus() - focus row/column invalid\n");
		return;
		}
	*row = RowPosToTypePos(g, XmCONTENT, g->grid.focusRow);
	*column = ColPosToTypePos(g, XmCONTENT, g->grid.focusCol);
	}

XmLGridRow XmLGridGetRow(w, rowType, row)
Widget w;
unsigned char rowType;
int row;
	{
	XmLGridWidget g;
	XmLGridRow r;
	int pos;

	if (!XmLIsGrid(w))
		{
		XmLWarning(w, "GetRow() - widget not a XmLGrid");
		return 0;
		}
	g = (XmLGridWidget)w;
	pos = RowTypePosToPos(g, rowType, row, 0);
	r = (XmLGridRow)XmLArrayGet(g->grid.rowArray, pos);
	if (!r)
		XmLWarning(w, "GetRow() - invalid row");
	return r;
	}

int XmLGridEditBegin(w, insert, row, column)
Widget w;
Boolean insert;
int row, column;
	{
	XmLGridWidget g;
	int r, c;
	XRectangle rect;

	if (!XmLIsGrid(w))
		{
		XmLWarning(w, "EditBegin() - widget not a XmLGrid");
		return -1;
		}
	g = (XmLGridWidget)w;
	if (column < 0 || column >= g->grid.colCount) 
		{
		XmLWarning(w, "EditBegin() - invalid column");
		return -1;
		}
	if (row < 0 || row >= g->grid.rowCount) 
		{
		XmLWarning(w, "EditBegin() - invalid row");
		return -1;
		}
	r = RowTypePosToPos(g, XmCONTENT, row, 0);
	c = ColTypePosToPos(g, XmCONTENT, column, 0);
	if (RowColToXY(g, r, c, &rect) == -1)
		{
		XmLWarning(w, "EditBegin() - cell must be visible to begin edit");
		return -1;
		}
	if (SetFocus(g, r, c, 0, 0) == -1)
		{
		XmLWarning(w, "EditBegin() - cant set focus to cell");
		return -1;
		}
	if (insert == False)
		TextAction(g, TEXT_EDIT);
	else 
		TextAction(g, TEXT_EDIT_INSERT);
	return 0;
	}

void XmLGridEditCancel(w)
Widget w;
	{
	XmLGridWidget g;

	if (!XmLIsGrid(w))
		{
		XmLWarning(w, "EditCancel() - widget not a XmLGrid");
		return;
		}
	g = (XmLGridWidget)w;
	TextAction(g, TEXT_EDIT_CANCEL);
	}

void XmLGridEditComplete(w)
Widget w;
	{
	XmLGridWidget g;

	if (!XmLIsGrid(w))
		{
		XmLWarning(w, "EditComplete() - widget not a XmLGrid");
		return;
		}
	g = (XmLGridWidget)w;
	TextAction(g, TEXT_EDIT_COMPLETE);
	}

XmLGridCell XmLGridGetCell(w, rowType, row, columnType, column)
Widget w;
unsigned char rowType;
int row;
unsigned char columnType;
int column;
	{
	XmLGridWidget g;
	XmLGridCell cellp;
	int r, c;

	if (!XmLIsGrid(w))
		{
		XmLWarning(w, "GetCell() - widget not a XmLGrid");
		return 0;
		}
	g = (XmLGridWidget)w;
	r = RowTypePosToPos(g, rowType, row, 0);
	c = ColTypePosToPos(g, columnType, column, 0);
	cellp = GetCell(g, r, c);
	if (!cellp)
		{
		XmLWarning(w, "GetCell() - invalid position");
		return 0;
		}
	return cellp;
	}

XmLGridColumn XmLGridGetColumn(w, columnType, column)
Widget w;
unsigned char columnType;
int column;
	{
	XmLGridWidget g;
	XmLGridColumn c;
	int pos;

	if (!XmLIsGrid(w))
		{
		XmLWarning(w, "GetColumn() - widget not a XmLGrid");
		return 0;
		}
	g = (XmLGridWidget)w;
	pos = ColTypePosToPos(g, columnType, column, 0);
	c = (XmLGridColumn)XmLArrayGet(g->grid.colArray, pos);
	if (!c)
		XmLWarning(w, "GetColumn() - invalid column");
	return c;
	}

int XmLGridGetSelectedCellCount(w)
Widget w;
	{
	XmLGridWidget g;

	if (!XmLIsGrid(w))
		{
		XmLWarning(w, "GetSelectedCellCount() - widget not a XmLGrid");
		return -1;
		}
	g = (XmLGridWidget)w;
	return GetSelectedArea(g, SelectCell, 0, 0, 0);
	}

int XmLGridGetSelectedCells(w, rowPositions, columnPositions, count)
Widget w;
int *rowPositions, *columnPositions;
int count;
	{
	XmLGridWidget g;
	int n;

	if (!XmLIsGrid(w))
		{
		XmLWarning(w, "GetSelectedCells() - widget not a XmLGrid");
		return -1;
		}
	g = (XmLGridWidget)w;
	n = GetSelectedArea(g, SelectCell, rowPositions, columnPositions, count);
	if (count != n)
		{
		XmLWarning(w, "GetSelectedCells() - count is incorrect");
		return -1;
		}
	return 0;
	}

int XmLGridGetSelectedColumnCount(w)
Widget w;
	{
	XmLGridWidget g;

	if (!XmLIsGrid(w))
		{
		XmLWarning(w, "GetSelectedColumnCount() - widget not a XmLGrid");
		return -1;
		}
	g = (XmLGridWidget)w;
	return GetSelectedArea(g, SelectCol, 0, 0, 0);
	}

int XmLGridGetSelectedColumns(w, positions, count)
Widget w;
int *positions;
int count;
	{
	XmLGridWidget g;
	int n;

	if (!XmLIsGrid(w))
		{
		XmLWarning(w, "GetSelectedColumns() - widget not a XmLGrid");
		return -1;
		}
	g = (XmLGridWidget)w;
	n = GetSelectedArea(g, SelectCol, 0, positions, count);
	if (count != n)
		{
		XmLWarning(w, "GetSelectedColumns() - count is incorrect");
		return -1;
		}
	return 0;
	}

int XmLGridGetSelectedRow(w)
Widget w;
	{
	XmLGridWidget g;
	int n, pos;

	if (!XmLIsGrid(w))
		{
		XmLWarning(w, "GetSelectedRowCount() - widget not a XmLGrid");
		return -1;
		}
	g = (XmLGridWidget)w;
	n = GetSelectedArea(g, SelectRow, &pos, 0, 1);
	if (n != 1)
		return -1;
	return pos;
	}

int XmLGridGetSelectedRowCount(w)
Widget w;
	{
	XmLGridWidget g;

	if (!XmLIsGrid(w))
		{
		XmLWarning(w, "GetSelectedRowCount() - widget not a XmLGrid");
		return -1;
		}
	g = (XmLGridWidget)w;
	return GetSelectedArea(g, SelectRow, 0, 0, 0);
	}

int XmLGridGetSelectedRows(w, positions, count)
Widget w;
int *positions;
int count;
	{
	XmLGridWidget g;
	int n;

	if (!XmLIsGrid(w))
		{
		XmLWarning(w, "GetSelectedRows() - widget not a XmLGrid");
		return -1;
		}
	g = (XmLGridWidget)w;
	n = GetSelectedArea(g, SelectRow, positions, 0, count);
	if (count != n)
		{
		XmLWarning(w, "GetSelectedRows() - count is incorrect");
		return -1;
		}
	return 0;
	}

Boolean XmLGridColumnIsVisible(w, col)
Widget w;
int col;
	{
	XmLGridWidget g;

	if (!XmLIsGrid(w))
		{
		XmLWarning(w, "ColumnIsVisible() - widget not a XmLGrid");
		return -1;
		}
	g = (XmLGridWidget)w;
	if (!ColIsVisible(g, g->grid.headingColCount + col))
		return False;
	return True;
	}

void XmLGridMoveColumns(w, newPosition, position, count)
Widget w;
int newPosition;
int position;
int count;
	{
	XmLGridWidget g;
	XmLGridRow row;
	int i, np, p, rowCount;

	if (!XmLIsGrid(w))
		{
		XmLWarning(w, "MoveColumns() - widget not a XmLGrid");
		return;
		}
	g = (XmLGridWidget)w;
	np = ColTypePosToPos(g, XmCONTENT, newPosition, 0);
	p = ColTypePosToPos(g, XmCONTENT, position, 0);
	if (XmLArrayMove(g->grid.colArray, np, p, count) == -1)
		{
		XmLWarning(w, "MoveColumns() - invalid position");
		return;
		}
	rowCount = XmLArrayGetCount(g->grid.rowArray);
	for (i = 0; i < rowCount; i++)
		{
		row = XmLArrayGet(g->grid.rowArray, i);
		XmLArrayMove(XmLGridRowCells(row), np, p, count);
		}
	g->grid.recalcHorizVisPos = 1;
	HorizLayout(g, 1);
	DrawArea(g, DrawAll, 0, 0);
	}

void XmLGridMoveRows(w, newPosition, position, count)
Widget w;
int newPosition;
int position;
int count;
	{
	XmLGridWidget g;
	int np, p;

	if (!XmLIsGrid(w))
		{
		XmLWarning(w, "MoveRows() - widget not a XmLGrid");
		return;
		}
	g = (XmLGridWidget)w;
	np = RowTypePosToPos(g, XmCONTENT, newPosition, 0);
	p = RowTypePosToPos(g, XmCONTENT, position, 0);
	if (XmLArrayMove(g->grid.rowArray, np, p, count) == -1)
		{
		XmLWarning(w, "MoveRows() - invalid position/count");
		return;
		}
	g->grid.recalcVertVisPos = 1;
	VertLayout(g, 1);
	DrawArea(g, DrawAll, 0, 0);
	}

void XmLGridReorderColumns(w, newPositions, position, count)
Widget w;
int *newPositions;
int position;
int count;
	{
	XmLGridWidget g;
	XmLGridRow row;
	int i, *np, p, rowCount, status;

	if (!XmLIsGrid(w))
		{
		XmLWarning(w, "ReorderColumns() - widget not a XmLGrid");
		return;
		}
	if (count <= 0)
		return;
	g = (XmLGridWidget)w;
	p = ColTypePosToPos(g, XmCONTENT, position, 0);
	np = (int *)malloc(sizeof(int) * count);
	for (i = 0; i < count; i++)
		np[i] = ColTypePosToPos(g, XmCONTENT, newPositions[i], 0);
	status = XmLArrayReorder(g->grid.colArray, np, p, count);
	if (status < 0)
		{
		free((char *)np);
		XmLWarning(w, "ReorderColumns() - invalid position/count");
		return;
		}
	rowCount = XmLArrayGetCount(g->grid.rowArray);
	for (i = 0; i < rowCount; i++)
		{
		row = XmLArrayGet(g->grid.rowArray, i);
		XmLArrayReorder(XmLGridRowCells(row), np, p, count);
		}
	free((char *)np);
	g->grid.recalcHorizVisPos = 1;
	HorizLayout(g, 1);
	DrawArea(g, DrawAll, 0, 0);
	}

void XmLGridReorderRows(w, newPositions, position, count)
Widget w;
int *newPositions;
int position;
int count;
	{
	XmLGridWidget g;
	int i, *np, p, status;

	if (!XmLIsGrid(w))
		{
		XmLWarning(w, "ReorderRows() - widget not a XmLGrid");
		return;
		}
	if (count <= 0)
		return;
	g = (XmLGridWidget)w;
	p = RowTypePosToPos(g, XmCONTENT, position, 0);
	np = (int *)malloc(sizeof(int) * count);
	for (i = 0; i < count; i++)
		np[i] = RowTypePosToPos(g, XmCONTENT, newPositions[i], 0);
	status = XmLArrayReorder(g->grid.rowArray, np, p, count);
	free((char *)np);
	if (status == -1)
		{
		XmLWarning(w, "ReorderRows() - invalid position/count");
		return;
		}
	g->grid.recalcVertVisPos = 1;
	VertLayout(g, 1);
	DrawArea(g, DrawAll, 0, 0);
	}

int XmLGridRowColumnToXY(w, rowType, row, columnType, column, rect)
Widget w;
unsigned char rowType;
int row;
unsigned char columnType;
int column;
XRectangle *rect;
	{
	XmLGridWidget g;
	int r, c;

	if (!XmLIsGrid(w))
		{
		XmLWarning(w, "RowColumnToXY() - widget not a XmLGrid");
		return -1;
		}
	g = (XmLGridWidget)w;
	r = RowTypePosToPos(g, rowType, row, 0);
	c = ColTypePosToPos(g, columnType, column, 0);
	if (r < 0 || r >= XmLArrayGetCount(g->grid.rowArray) ||
		c < 0 || c >= XmLArrayGetCount(g->grid.colArray))
		{
  /*		XmLWarning(w, "RowColToXY() - invalid position"); */
		return -1;
		}
	return RowColToXY(g, r, c, rect);
	}

Boolean XmLGridRowIsVisible(w, row)
Widget w;
int row;
	{
	XmLGridWidget g;

	if (!XmLIsGrid(w))
		{
		XmLWarning(w, "RowIsVisible() - widget not a XmLGrid");
		return -1;
		}
	g = (XmLGridWidget)w;
	if (!RowIsVisible(g, g->grid.headingRowCount + row))
		return False;
	return True;
	}

void XmLGridSelectAllRows(w, notify)
Widget w;
Boolean notify;
	{
	XmLGridWidget g;

	if (!XmLIsGrid(w))
		{
		XmLWarning(w, "SelectAllRows() - widget not a XmLGrid");
		return;
		}
	g = (XmLGridWidget)w;
	SelectTypeArea(g, SelectRow, -1, 0, True, notify);
	}

void XmLGridSelectAllColumns(w, notify)
Widget w;
Boolean notify;
	{
	XmLGridWidget g;

	if (!XmLIsGrid(w))
		{
		XmLWarning(w, "SelectAllRows() - widget not a XmLGrid");
		return;
		}
	g = (XmLGridWidget)w;
	SelectTypeArea(g, SelectCol, 0, -1, True, notify);
	}

void XmLGridSelectAllCells(w, notify)
Widget w;
Boolean notify;
	{
	XmLGridWidget g;

	if (!XmLIsGrid(w))
		{
		XmLWarning(w, "SelectAllCells() - widget not a XmLGrid");
		return;
		}
	g = (XmLGridWidget)w;
		SelectTypeArea(g, SelectCell, -1, -1, True, notify);
	}

void XmLGridSelectCell(w, row, column, notify)
Widget w;
int row, column;
Boolean notify;
	{
	XmLGridWidget g;

	if (!XmLIsGrid(w))
		{
		XmLWarning(w, "SelectCell() - widget not a XmLGrid");
		return;
		}
	g = (XmLGridWidget)w;
	SelectTypeArea(g, SelectCell, row, column, True, notify);
	}

void XmLGridSelectColumn(w, column, notify)
Widget w;
int column;
Boolean notify;
	{
	XmLGridWidget g;

	if (!XmLIsGrid(w))
		{
		XmLWarning(w, "SelectColumn() - widget not a XmLGrid");
		return;
		}
	g = (XmLGridWidget)w;
	SelectTypeArea(g, SelectCol, 0, column, True, notify);
	}

void XmLGridSelectRow(w, row, notify)
Widget w;
int row;
Boolean notify;
	{
	XmLGridWidget g;

	if (!XmLIsGrid(w))
		{
		XmLWarning(w, "SelectRow() - widget not a XmLGrid");
		return;
		}
	g = (XmLGridWidget)w;
	SelectTypeArea(g, SelectRow, row, 0, True, notify);
	}

int XmLGridSetFocus(w, row, column)
Widget w;
int row, column;
	{
	XmLGridWidget g;
	int r, c;

	if (!XmLIsGrid(w))
		{
		XmLWarning(w, "SetFocus() - widget not a XmLGrid");
		return -1;
		}
	g = (XmLGridWidget)w;
	if (row < 0 || row >= g->grid.rowCount)
		{
		XmLWarning(w, "SetFocus() - invalid row");
		return -1;
		}
	if (column < 0 || column >= g->grid.colCount)
		{
		XmLWarning(w, "SetFocus() - invalid column");
		return -1;
		}
	r = RowTypePosToPos(g, XmCONTENT, row, 0);
	c = ColTypePosToPos(g, XmCONTENT, column, 0);
	return SetFocus(g, r, c, 0, 0);
	}

int XmLGridXYToRowColumn(w, x, y, rowType, row, columnType, column)
Widget w;
int x, y;
unsigned char *rowType;
int *row;
unsigned char *columnType;
int *column;
	{
	XmLGridWidget g;
	int r, c;

	if (!XmLIsGrid(w))
		{
		XmLWarning(w, "XYToRowColumn() - widget not a XmLGrid");
		return -1;
		}
	g = (XmLGridWidget)w;
	if (XYToRowCol(g, x, y, &r, &c) == -1)
		return -1;
	*rowType = RowPosToType(g, r);
	*row = RowPosToTypePos(g, *rowType, r);
	*columnType = ColPosToType(g, c);
	*column = ColPosToTypePos(g, *columnType, c);
	return 0;
	}

void XmLGridRedrawAll(w)
Widget w;
	{
	XmLGridWidget g;

	if (!XmLIsGrid(w))
		{
		XmLWarning(w, "RedrawAll() - widget not a XmLGrid");
		return;
		}
	g = (XmLGridWidget)w;
	DrawArea(g, DrawAll, 0, 0);
	}

void XmLGridRedrawCell(w, rowType, row, columnType, column)
Widget w;
unsigned char rowType;
int row;
unsigned char columnType;
int column;
	{
	XmLGridWidget g;
	int r, c;

	if (!XmLIsGrid(w))
		{
		XmLWarning(w, "RedrawCell() - widget not a XmLGrid");
		return;
		}
	g = (XmLGridWidget)w;
	r = RowTypePosToPos(g, rowType, row, 0);
	c = ColTypePosToPos(g, columnType, column, 0);
	DrawArea(g, DrawCell, r, c);
	}

void XmLGridRedrawColumn(w, type, column)
Widget w;
unsigned char type;
int column;
	{
	XmLGridWidget g;
	int c;

	if (!XmLIsGrid(w))
		{
		XmLWarning(w, "RedrawColumn() - widget not a XmLGrid");
		return;
		}
	g = (XmLGridWidget)w;
	c = ColTypePosToPos(g, type, column, 0);
	DrawArea(g, DrawCol, 0, c);
	}

void XmLGridRedrawRow(w, type, row)
Widget w;
unsigned char type;
int row;
	{
	XmLGridWidget g;
	int r;

	if (!XmLIsGrid(w))
		{
		XmLWarning(w, "RedrawRow() - widget not a XmLGrid");
		return;
		}
	g = (XmLGridWidget)w;
	r = RowTypePosToPos(g, type, row, 0);
	DrawArea(g, DrawRow, r, 0);
	}

int XmLGridRead(w, file, format, delimiter)
Widget w;
FILE *file;
int format;
char delimiter;
	{
	XmLGridWidget g;
	char *data;
	int n;

	if (!XmLIsGrid(w))
		{
		XmLWarning(w, "Read() - widget not a XmLGrid");
		return 0;
		}
	if (!file)
		{
		XmLWarning(w, "Read() - file is NULL");
		return 0;
		}
	g = (XmLGridWidget)w;
	data = FileToString(file);
	if (!data)
		{
		XmLWarning(w, "Read() - error loading file");
		return 0;
		}
	n = Read(g, format, delimiter, 0, 0, data);
	free((char *)data);
	return n;
	}

int XmLGridReadPos(w, file, format, delimiter, rowType, row,
	columnType, column)
Widget w;
FILE *file;
int format;
char delimiter;
unsigned char rowType;
int row;
unsigned char columnType;
int column;
	{
	XmLGridWidget g;
	char *data;
	int r, c, n;

	if (!XmLIsGrid(w))
		{
		XmLWarning(w, "ReadPos() - widget not a XmLGrid");
		return 0;
		}
	if (!file)
		{
		XmLWarning(w, "ReadPos() - file is NULL");
		return 0;
		}
	g = (XmLGridWidget)w;
	data = FileToString(file);
	if (!data)
		{
		XmLWarning(w, "ReadPos() - error loading file");
		return 0;
		}
	r = RowTypePosToPos(g, rowType, row, 0);
	c = ColTypePosToPos(g, columnType, column, 0);
	n = Read(g, format, delimiter, r, c, data);
	free((char *)data);
	return n;
	}

int XmLGridSetStrings(w, data)
Widget w;
char *data;
	{
	XmLGridWidget g;

	if (!XmLIsGrid(w))
		{
		XmLWarning(w, "SetStrings() - widget not a XmLGrid");
		return 0;
		}
	g = (XmLGridWidget)w;
	return Read(g, XmFORMAT_DELIMITED, '|', 0, 0, data);
	}

int XmLGridSetStringsPos(w, rowType, row, columnType, column, data)
Widget w;
unsigned char rowType;
int row;
unsigned char columnType;
int column;
char *data;
	{
	XmLGridWidget g;
	int r, c;

	if (!XmLIsGrid(w))
		{
		XmLWarning(w, "SetStringsPos() - widget not a XmLGrid");
		return 0;
		}
	g = (XmLGridWidget)w;
	r = RowTypePosToPos(g, rowType, row, 0);
	c = ColTypePosToPos(g, columnType, column, 0);
	return Read(g, XmFORMAT_DELIMITED, '|', r, c, data);
	}

int XmLGridWrite(w, file, format, delimiter, skipHidden)
Widget w;
FILE *file;
int format;
char delimiter;
Boolean skipHidden;
	{
	XmLGridWidget g;
	int nrow, ncol;

	if (!XmLIsGrid(w))
		{
		XmLWarning(w, "Write() - widget not a XmLGrid");
		return 0;
		}
	g = (XmLGridWidget)w;
	nrow = XmLArrayGetCount(g->grid.rowArray);
	ncol = XmLArrayGetCount(g->grid.colArray);
	Write(g, file, format, delimiter, skipHidden, 0, 0, nrow, ncol);
	return 0;
	}

int XmLGridWritePos(w, file, format, delimiter, skipHidden,
	rowType, row, columnType, column, nrow, ncolumn)
Widget w;
FILE *file;
int format;
char delimiter;
Boolean skipHidden;
unsigned char rowType;
int row;
unsigned char columnType;
int column;
int nrow, ncolumn;
	{
	XmLGridWidget g;
	int r, c;

	if (!XmLIsGrid(w))
		{
		XmLWarning(w, "WritePos() - widget not a XmLGrid");
		return 0;
		}
	g = (XmLGridWidget)w;
	r = RowTypePosToPos(g, rowType, row, 0);
	c = ColTypePosToPos(g, columnType, column, 0);
	Write(g, file, format, delimiter, skipHidden, r, c, nrow, ncolumn);
	return 0;
	}

Boolean XmLGridCopyPos(w, time, rowType, row, columnType, column,
	nrow, ncolumn)
Widget w;
Time time;
unsigned char rowType;
int row;
unsigned char columnType;
int column;
int nrow, ncolumn;
	{
	XmLGridWidget g;
	int r, c;

	if (!XmLIsGrid(w))
		{
		XmLWarning(w, "CopyPos() - widget not a XmLGrid");
		return False;
		}
	g = (XmLGridWidget)w;
	r = RowTypePosToPos(g, rowType, row, 0);
	c = ColTypePosToPos(g, columnType, column, 0);
	return Copy(g, time, 0, r, c, nrow, ncolumn);
	}

Boolean XmLGridCopySelected(w, time)
Widget w;
Time time;
	{
	XmLGridWidget g;

	if (!XmLIsGrid(w))
		{
		XmLWarning(w, "CopySelected() - widget not a XmLGrid");
		return False;
		}
	g = (XmLGridWidget)w;
	return Copy(g, time, 1, 0, 0, 0, 0);
	}

Boolean XmLGridPaste(w)
Widget w;
	{
	XmLGridWidget g;
	int r, c;

	if (!XmLIsGrid(w))
		{
		XmLWarning(w, "Paste() - widget not a XmLGrid");
		return False;
		}
	g = (XmLGridWidget)w;
	r = g->grid.focusRow;
	c = g->grid.focusCol;
	if (r < 0 || c < 0)
		{
		XmLWarning(w, "Paste() - no cell has focus");
		return False;
		}
	return Paste(g, r, c);
	}

Boolean XmLGridPastePos(w, rowType, row, columnType, column)
Widget w;
unsigned char rowType;
int row;
unsigned char columnType;
int column;
	{
	XmLGridWidget g;
	int r, c;

	if (!XmLIsGrid(w))
		{
		XmLWarning(w, "PastePos() - widget not a XmLGrid");
		return False;
		}
	g = (XmLGridWidget)w;
	r = RowTypePosToPos(g, rowType, row, 0);
	c = ColTypePosToPos(g, columnType, column, 0);
	return Paste(g, r, c);
	}
