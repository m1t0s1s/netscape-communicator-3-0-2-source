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

#ifndef XmLH
#define XmLH

#include <Xm/Xm.h>

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

#define XmNdebugLevel "debugLevel"
#define XmCDebugLevel "DebugLevel"

#define XmNdayRows "dayRows"
#define XmCDayRows "DayRows"
#define XmNdayColumns "dayColumns"
#define XmCDayColumns "DayColumns"
#define XmNmonth "month"
#define XmCMonth "Month"
#define XmNyear "year"
#define XmCYear "Year"

/* Folder resources */

#define XmNactiveTab "activeTab"
#define XmCActiveTab "activeTab"
#define XmNblankBackground "blankBackground"
#define XmCBlankBackground "BlankBackground"
#define XmNcornerDimension "cornerDimension"
#define XmCCornerDimension "CornerDimension"
#define XmNcornerStyle "cornerStyle"
#define XmCCornerStyle "CornerStyle"
#define XmRCornerStyle "CornerStyle"
#define XmNinactiveBackground "inactiveBackground"
#define XmCInactiveBackground "InactiveBackground"
#define XmNinactiveForeground "inactiveForeground"
#define XmCInactiveForeground "InactiveForeground"
#define XmNpixmapMargin "pixmapMargin"
#define XmCPixmapMargin "PixmapMargin"
#define XmNrotateWhenLeftRight "rotateWhenLeftRight"
#define XmCRotateWhenLeftRight "RotateWhenLeftRight"
#define XmNtabCount "tabCount"
#define XmCTabCount "TabCount"
#define XmNtabPlacement "tabPlacement"
#define XmCTabPlacement "TabPlacement"
#define XmRTabPlacement "TabPlacement"
#define XmNtabTranslations "tabTranslations"
#define XmNtabWidgetList "tabWidgetList"

/* Folder Constraint resources */

#define XmNtabFreePixmaps "tabFreePixmaps"
#define XmCTabFreePixmaps "TabFreePixmaps"
#define XmNtabInactivePixmap "tabInactivePixmap"
#define XmCTabInactivePixmap "TabInactivePixmap"
#define XmNtabManagedWidget "tabManagedWidget"
#define XmCTabManagedWidget "TabManagedWidget"
#define XmNtabPixmap "tabPixmap"
#define XmCTabPixmap "TabPixmap"

/* Folder callbacks */

typedef struct
	{
	int reason;
	XEvent *event;
	int pos;
	int allowActivate;
	} XmLFolderCallbackStruct;

/* Folder defines */

#define XmCORNER_NONE 0
#define XmCORNER_LINE 1
#define XmCORNER_ARC  2

#define XmFOLDER_TOP    0
#define XmFOLDER_LEFT   1
#define XmFOLDER_BOTTOM 2
#define XmFOLDER_RIGHT  3

/* Grid resources */

#define XmNaddCallback "addCallback"
#define XmNallowColumnResize "allowColumnResize"
#define XmCAllowColumnResize "AllowColumnResize"
#define XmNallowDragSelected "allowDragSelected"
#define XmCAllowDragSelected "AllowDragSelected"
#define XmNallowDrop "allowDrop"
#define XmCAllowDrop "AllowDrop"
#define XmNallowRowResize "allowRowResize"
#define XmCAllowRowResize "AllowRowResize"
#ifndef XmNblankBackground
#define XmNblankBackground "blankBackground"
#endif
#ifndef XmCBlankBackground
#define XmCBlankBackground "BlankBackground"
#endif
#define XmNbottomFixedCount "bottomFixedCount"
#define XmCBottomFixedCount "BottomFixedCount"
#define XmNbottomFixedMargin "bottomFixedMargin"
#define XmCBottomFixedMargin "BottomFixedMargin"
#define XmNcellDefaults "cellDefaults"
#define XmCCellDefaults "CellDefaults"
#define XmNcellDrawCallback "cellDrawCallback"
#define XmNcellDropCallback "cellDropCallback"
#define XmNcellFocusCallback "cellFocusCallback"
#define XmNcellPasteCallback "cellPasteCallback"
#define XmNdeleteCallback "deleteCallback"
#define XmNdeselectCallback "deselectCallback"
#define XmNeditCallback "editCallback"
#define XmNeditTranslations "editTranslations"
#define XmNfooterColumnCount "footerColumnCount"
#define XmCFooterColumnCount "FooterColumnCount"
#define XmNfooterRowCount "footerRowCount"
#define XmCFooterRowCount "FooterRowCount"
#define XmCGridSelectionPolicy "GridSelectionPolicy"
#define XmRGridSelectionPolicy "GridSelectionPolicy"
#define XmRGridSizePolicy "GridSizePolicy"
#define XmNheadingColumnCount "headingColumnCount"
#define XmCHeadingColumnCount "HeadingColumnCount"
#define XmNheadingRowCount "headingRowCount"
#define XmCHeadingRowCount "HeadingRowCount"
#define XmNhiddenColumns "hiddenColumns"
#define XmCHiddenColumns "HiddenColumns"
#define XmNhiddenRows "hiddenRows"
#define XmCHiddenRows "HiddenRows"
#define XmNhorizontalSizePolicy "horizontalSizePolicy"
#define XmCHorizontalSizePolicy "HorizontalSizePolicy"
#define XmNhsbDisplayPolicy "hsbDisplayPolicy"
#define XmCHsbDisplayPolicy "HsbDisplayPolicy"
#define XmNlayoutFrozen "layoutFrozen"
#define XmCLayoutFrozen "LayoutFrozen"
#define XmNleftFixedCount "leftFixedCount"
#define XmCLeftFixedCount "LeftFixedCount"
#define XmNleftFixedMargin "leftFixedMargin"
#define XmCLeftFixedMargin "LeftFixedMargin"
#define XmNrightFixedCount "rightFixedCount"
#define XmCRightFixedCount "RightFixedCount"
#define XmNrightFixedMargin "rightFixedMargin"
#define XmCRightFixedMargin "RightFixedMargin"
#define XmNscrollBarMargin "scrollBarMargin"
#define XmCScrollBarMargin "ScrollBarMargin"
#define XmNscrollCallback "scrollCallback"
#define XmNscrollColumn "scrollColumn"
#define XmCScrollColumn "ScrollColumn"
#define XmNscrollRow "scrollRow"
#define XmCScrollRow "ScrollRow"
#define XmNselectCallback "selectCallback"
#define XmNselectForeground "selectForeground"
#define XmCSelectForeground "SelectForeground"
#define XmNselectBackground "selectBackground"
#define XmCSelectBackground "SelectBackground"
#define XmNshadowRegions "shadowRegions"
#define XmCShadowRegions "ShadowRegions"
#define XmNtextWidget "textWidget"
#define XmCTextWidget "TextWidget"
#define XmNtopFixedCount "topFixedCount"
#define XmCTopFixedCount "TopFixedCount"
#define XmNtopFixedMargin "topFixedMargin"
#define XmCTopFixedMargin "TopFixedMargin"
#define XmNtraverseTranslations "traverseTranslations"
#define XmNuseAverageFontWidth "useAverageFontWidth"
#define XmCUseAverageFontWidth "UseAverageFontWidth"
#define XmNverticalSizePolicy "verticalSizePolicy"
#define XmCVerticalSizePolicy "VerticalSizePolicy"
#define XmNvsbDisplayPolicy "vsbDisplayPolicy"
#define XmCVsbDisplayPolicy "VsbDisplayPolicy"

/* Grid Row/Column/Cell resources */

#define XmNrow "row"
#define XmCRow "row"
#define XmNrowHeight "rowHeight"
#define XmCRowHeight "RowHeight"
#define XmNrowPtr "rowPtr"
#define XmNrowRangeEnd "rowRangeEnd"
#define XmCRowRangeEnd "RowRangeEnd"
#define XmNrowRangeStart "rowRangeStart"
#define XmCRowRangeStart "RowRangeStart"
#define XmNrowSizePolicy "rowSizePolicy"
#define XmCRowSizePolicy "RowSizePolicy"
#define XmNrowStep "rowStep"
#define XmCRowStep "RowStep"
#define XmNrowType "rowType"
#define XmCRowType "RowType"
#define XmNrowUserData "rowUserData"

#define XmNcolumn "column"
#define XmCColumn "Column"
#define XmNcolumnPtr "columnPtr"
#define XmNcolumnRangeEnd "columnRangeEnd"
#define XmCColumnRangeEnd "ColumnRangeEnd"
#define XmNcolumnRangeStart "columnRangeStart"
#define XmCColumnRangeStart "ColumnRangeStart"
#define XmNcolumnSizePolicy "columnSizePolicy"
#define XmCColumnSizePolicy "ColumnSizePolicy"
#define XmNcolumnStep "columnStep"
#define XmCColumnStep "ColumnStep"
#define XmNcolumnTraversable "columnTraversable"
#define XmCColumnTraversable "ColumnTraversable"
#define XmNcolumnType "columnType"
#define XmCColumnType "ColumnType"
#define XmNcolumnWidth "columnWidth"
#define XmCColumnWidth "ColumnWidth"
#define XmNcolumnUserData "columnUserData"

#define XmNcellAlignment "cellAlignment"
#define XmCCellAlignment "CellAlignment"
#define XmRCellAlignment "CellAlignment"
#define XmNcellBackground "cellBackground"
#define XmCCellBackground "CellBackground"
#define XmRCellBorderType "CellBorderType"
#define XmNcellBottomBorderType "cellBottomBorderType"
#define XmCCellBottomBorderType "CellBottomBorderType"
#define XmNcellBottomBorderPixel "cellBottomBorderPixel"
#define XmCCellBottomBorderPixel "CellBottomBorderPixel"
#define XmNcellColumnSpan "cellColumnSpan"
#define XmCCellColumnSpan "CellColumnSpan"
#define XmNcellForeground "cellForeground"
#define XmCCellForeground "CellForeground"
#define XmNcellFontList "cellFontList"
#define XmCCellFontList "CellFontList"
#define XmNcellLeftBorderType "cellLeftBorderType"
#define XmCCellLeftBorderType "CellLeftBorderType"
#define XmNcellLeftBorderPixel "cellLeftBorderPixel"
#define XmCCellLeftBorderPixel "CellLeftBorderPixel"
#define XmNcellPixmap "cellPixmap"
#define XmCCellPixmap "CellPixmap"
#define XmNcellRightBorderType "cellRightBorderType"
#define XmCCellRightBorderType "CellRightBorderType"
#define XmNcellRightBorderPixel "cellRightBorderPixel"
#define XmCCellRightBorderPixel "CellRightBorderPixel"
#define XmNcellRowSpan "cellRowSpan"
#define XmCCellRowSpan "CellRowSpan"
#define XmNcellString "cellString"
#define XmNcellTopBorderType "cellTopBorderType"
#define XmCCellTopBorderType "CellTopBorderType"
#define XmNcellTopBorderPixel "cellTopBorderPixel"
#define XmCCellTopBorderPixel "CellTopBorderPixel"
#define XmNcellType "cellType"
#define XmCCellType "CellType"
#define XmRCellType "CellType"
#define XmNcellUserData "cellUserData"

/* Grid callbacks */

typedef struct _XmLGridDrawStruct
	{
	GC gc;
	XRectangle *cellRect;
	Pixel selectBackground, selectForeground;
	Boolean hasFocus, isSelected;
	XmStringDirection stringDirection;
	} XmLGridDrawStruct;

typedef struct _XmLGridCallbackStruct
	{
	int reason;
	XEvent *event;
	unsigned char rowType, columnType;
	int row, column;
	XRectangle *clipRect;
	XmLGridDrawStruct *drawInfo;
	void *object;
	} XmLGridCallbackStruct;

#define XmCR_ADD_ROW         900
#define XmCR_ADD_COLUMN      901
#define XmCR_ADD_CELL        902
#define XmCR_CELL_DRAW       903
#define XmCR_CELL_DROP       904
#define XmCR_CELL_FOCUS_IN   905
#define XmCR_CELL_FOCUS_OUT  906
#define XmCR_CELL_PASTE      907
#define XmCR_COLUMN_WIDTH    908
#define XmCR_DELETE_ROW      909
#define XmCR_DELETE_COLUMN   910
#define XmCR_DELETE_CELL     911
#define XmCR_EDIT_BEGIN      912
#define XmCR_EDIT_INSERT     913
#define XmCR_EDIT_CANCEL     914
#define XmCR_EDIT_COMPLETE   915
#define XmCR_FREE_VALUE      916
#define XmCR_RESIZE_ROW      917
#define XmCR_RESIZE_COLUMN   918
#define XmCR_ROW_HEIGHT      919
#define XmCR_SCROLL_ROW      920
#define XmCR_SCROLL_COLUMN   921
#define XmCR_SELECT_CELL     922
#define XmCR_SELECT_COLUMN   923
#define XmCR_SELECT_ROW      924
#define XmCR_DESELECT_CELL   925
#define XmCR_DESELECT_COLUMN 926
#define XmCR_DESELECT_ROW    927

/* Grid defines */

#define XmCONTENT 0
#define XmHEADING 1
#define XmFOOTER  2
#define XmALL_TYPES 3

#define XmLABEL_CELL   0
#define XmTEXT_CELL    1
#define XmPIXMAP_CELL  2

#define XmBORDER_NONE 1
#define XmBORDER_LINE 2

#define XmFORMAT_DELIMITED 1
#define XmFORMAT_XL        2
#define XmFORMAT_PAD       3
#define XmFORMAT_PASTE     4
#define XmFORMAT_DROP      5

#define XmSELECT_NONE         1
#define XmSELECT_SINGLE_ROW   2
#define XmSELECT_BROWSE_ROW   3
#define XmSELECT_MULTIPLE_ROW 4
#define XmSELECT_CELL         5

#define XmTRAVERSE_EXTEND_DOWN  20
#define XmTRAVERSE_EXTEND_LEFT  21
#define XmTRAVERSE_EXTEND_RIGHT 22
#define XmTRAVERSE_EXTEND_UP    23
#define XmTRAVERSE_PAGE_DOWN    24
#define XmTRAVERSE_PAGE_LEFT    25
#define XmTRAVERSE_PAGE_RIGHT   26
#define XmTRAVERSE_PAGE_UP      27
#define XmTRAVERSE_TO_BOTTOM    28
#define XmTRAVERSE_TO_TOP       29

#define XmALIGNMENT_LEFT         0
#ifndef XmALIGNMENT_CENTER
#define XmALIGNMENT_CENTER       1
#endif
#define XmALIGNMENT_RIGHT        2
#define XmALIGNMENT_TOP_LEFT     3
#define XmALIGNMENT_TOP          4
#define XmALIGNMENT_TOP_RIGHT    5
#define XmALIGNMENT_BOTTOM_LEFT  6
#define XmALIGNMENT_BOTTOM       7
#define XmALIGNMENT_BOTTOM_RIGHT 8

/* Progress resources */

#define XmNcompleteValue "completeValue"
#define XmCCompleteValue "CompleteValue"
#define XmNnumBoxes "numBoxes"
#define XmCNumBoxes "NumBoxes"
#define XmNmeterStyle "meterStyle"
#define XmCMeterStyle "MeterStyle"
#define XmRMeterStyle "MeterStyle"
#define XmNshowPercentage "showPercentage"
#define XmCShowPercentage "ShowPercentage"
#define XmNshowTime "showTime"
#define XmCShowTime "ShowTime"

/* Progress defines */

#define XmMETER_BAR 0
#define XmMETER_BOXES 1

/* Utility defines */

#define XmDRAWNB_ARROW        0
#define XmDRAWNB_ARROWLINE    1
#define XmDRAWNB_DOUBLEARROW  2
#define XmDRAWNB_SQUARE       3
#define XmDRAWNB_DOUBLEBAR    4
#define XmDRAWNB_STRING       5

#define XmDRAWNB_RIGHT 0
#define XmDRAWNB_LEFT  1
#define XmDRAWNB_UP    2
#define XmDRAWNB_DOWN  3

#define XmSTRING_RIGHT 0
#define XmSTRING_LEFT  1
#define XmSTRING_UP    2
#define XmSTRING_DOWN  3

enum { XmLRectInside, XmLRectOutside, XmLRectPartial };

/* Utility functions */

#if defined(_STDC_) || defined(__cplusplus) || defined(c_plusplus)

typedef int (*XmLSortCompareFunc)(void *userData, void *l, void *r);

int XmLDateDaysInMonth(int m, int y);
int XmLDateWeekDay(int m, int d, int y);
void XmLDrawnButtonSetType(Widget w, int drawnType, int drawnDir);
XmFontList XmLFontListCopyDefault(Widget widget);
void XmLFontListGetDimensions(XmFontList fontList, short *width,
	short *height, Boolean useAverageWidth);
int XmLMessageBox(Widget w, char *string, Boolean okOnly);
void XmLPixmapDraw(Widget w, Pixmap pixmap, int pixmapWidth, int pixmapHeight,
	unsigned char alignment, GC gc, XRectangle *rect, XRectangle *clipRect);
int XmLRectIntersect(XRectangle *r1, XRectangle *r2);
Widget XmLShellOfWidget(Widget w);
void XmLSort(void *base, int numItems, unsigned int itemSize,
	XmLSortCompareFunc, void *userData);
void XmLStringDraw(Widget w, XmString string, XmStringDirection stringDir,
	XmFontList fontList, unsigned char alignment, GC gc,
	XRectangle *rect, XRectangle *clipRect);
void XmLStringDrawDirection(Display *dpy, Window win, XmFontList fontlist,
	XmString string, GC gc, int x, int y, Dimension width,
	unsigned char alignment, unsigned char layout_direction,
	unsigned char drawing_direction);
void XmLWarning(Widget w, char *msg);

#else

typedef int (*XmLSortCompareFunc)();

int XmLDateDaysInMonth();
int XmLDateWeekDay();
void XmLDrawnButtonSetType();
XmFontList XmLFontListCopyDefault();
void XmLFontListGetDimensions();
int XmLMessageBox();
void XmLPixmapDraw();
int XmLRectIntersect();
Widget XmLShellOfWidget();
void XmLSort();
void XmLStringDraw();
void XmLStringDrawDirection();
void XmLWarning();

#endif

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif
