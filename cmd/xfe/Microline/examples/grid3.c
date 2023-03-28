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
#include <Xm/PushB.h>
#include <Xm/Form.h>
#include <XmL/Grid.h>

void cellSelect();
void showSelected();

#define HEADINGFONT "-*-helvetica-bold-o-normal--12-*-*-*-*-*-iso8859-1"
#define CONTENTFONT "-*-helvetica-medium-r-normal--12-*-*-*-*-*-iso8859-1"

#define check_width 9
#define check_height 9
static char check_bits[] = {
	0x00, 0x01, 0x80, 0x01, 0xc0, 0x00, 0x61, 0x00, 0x37, 0x00, 0x3e, 0x00,
	0x1c, 0x00, 0x08, 0x00, 0x00, 0x00};

#define nocheck_width 9
#define nocheck_height 9
static char nocheck_bits[] = {
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

static char *data[20] = 
{
	"3|Bob Thompson|bobt@teledyne.com",
	"12|Darl Simon|ds@atg.org",
	"14|Jacq Frontier|jf@terrax.com",
	"19|Patty Lee|patlee@isis.com",
	"22|Michal Barnes|mickeyb@softorb.com",
	"23|Dave Schultz|daves@timing.com",
	"23|Eric Stanley|ericst@aldor.com",
	"29|Tim Winters|timw@terra.com",
	"31|Agin Tomitu|agt@umn.edu",
	"33|Betty Tinner|bett@ost.edu",
	"37|Tom Smith|tsmith@netwld.com",
	"38|Rick Wild|raw@mlsoft.com",
	"41|Al Joyce|aj@ulm.edu",
	"41|Tim Burtan|timb@autoc.com",
	"41|George Marlin|gjm@eyeln.com",
	"41|Bill Boxer|billb@idesk.com",
	"41|Maria Montez|marm@ohio.edu",
	"41|Yin Fang|aj@utxs.edu",
	"41|Suzy Saps|ss@umg.edu",
	"41|Jerry Rodgers|jr@lyra.com",
};

Pixmap nocheckPix, checkPix;
Widget grid;

main(argc, argv)
int argc;
char *argv[];
{
	XtAppContext app;
	Widget shell, form, button;
	Pixel blackPixel, whitePixel;
	Pixmap pix;
	int i;

	shell =  XtAppInitialize(&app, "Grid3", NULL, 0,
	    &argc, argv, NULL, NULL, 0);

	blackPixel = BlackPixelOfScreen(XtScreen(shell));
	whitePixel = WhitePixelOfScreen(XtScreen(shell));
	checkPix = XCreatePixmapFromBitmapData(XtDisplay(shell),
	    DefaultRootWindow(XtDisplay(shell)),
	    check_bits, check_width, check_height,
	    blackPixel, whitePixel, 
	    DefaultDepthOfScreen(XtScreen(shell)));
	nocheckPix = XCreatePixmapFromBitmapData(XtDisplay(shell),
	    DefaultRootWindow(XtDisplay(shell)),
	    nocheck_bits, nocheck_width, nocheck_height,
	    blackPixel, whitePixel, 
	    DefaultDepthOfScreen(XtScreen(shell)));

	form = XtVaCreateManagedWidget("form",
	    xmFormWidgetClass, shell,
	    NULL);

	button = XtVaCreateManagedWidget("Show Selected",
	    xmPushButtonWidgetClass, form,
	    XmNbottomAttachment, XmATTACH_FORM,
	    XmNleftAttachment, XmATTACH_FORM,
	    NULL);

	grid = XtVaCreateManagedWidget("grid",
	    xmlGridWidgetClass, form,
	    XmNhorizontalSizePolicy, XmVARIABLE,
	    XmNheight, 300,
	    XtVaTypedArg, XmNbackground, XmRString, "white", 6,
	    XtVaTypedArg, XmNforeground, XmRString, "black", 6,
	    XmNtopAttachment, XmATTACH_FORM,
	    XmNleftAttachment, XmATTACH_FORM,
	    XmNrightAttachment, XmATTACH_FORM,
	    XmNbottomAttachment, XmATTACH_WIDGET,
	    XmNbottomWidget, button,
	    XmNselectionPolicy, XmSELECT_NONE,
	    NULL);
	XtAddCallback(grid, XmNselectCallback, cellSelect, NULL);

	XtAddCallback(button, XmNactivateCallback, showSelected, NULL);

	XtVaSetValues(grid, XmNlayoutFrozen, True, NULL);

	XmLGridAddColumns(grid, XmCONTENT, -1, 4);

	XtVaSetValues(grid, 
	    XmNcellDefaults, True,
	    XtVaTypedArg, XmNcellFontList, XmRString, HEADINGFONT,
	    strlen(HEADINGFONT) + 1,
	    XtVaTypedArg, XmNcellBackground, XmRString, "blue", 5,
	    XtVaTypedArg, XmNcellForeground, XmRString, "white", 6,
	    XmNcellAlignment, XmALIGNMENT_LEFT,
	    NULL);
	XmLGridAddRows(grid, XmHEADING, -1, 1);

	XtVaSetValues(grid,
	    XmNcellDefaults, True,
	    XtVaTypedArg, XmNcellFontList, XmRString, CONTENTFONT,
	    strlen(CONTENTFONT) + 1,
	    XtVaTypedArg, XmNcellBackground, XmRString, "white", 6,
	    XtVaTypedArg, XmNcellForeground, XmRString, "black", 6,
	    XmNcellLeftBorderType, XmNONE,
	    XmNcellRightBorderType, XmNONE,
	    NULL);
	XmLGridAddRows(grid, XmCONTENT, -1, 20);

	XtVaSetValues(grid,
	    XmNcolumn, 0,
	    XmNcolumnWidth, 4,
	    XmNcellType, XmPIXMAP_CELL,
	    XmNcellAlignment, XmALIGNMENT_CENTER,
	    NULL);
	XtVaSetValues(grid,
	    XmNcolumn, 1,
	    XmNcolumnWidth, 4,
	    XmNcellAlignment, XmALIGNMENT_RIGHT,
	    NULL);
	XtVaSetValues(grid,
	    XmNcolumn, 2,
	    XmNcolumnWidth, 18,
	    NULL);
	XtVaSetValues(grid,
	    XmNcolumn, 3,
	    XmNcolumnWidth, 20,
	    NULL);

	XtVaSetValues(grid, XmNlayoutFrozen, False, NULL);

	XmLGridSetStrings(grid, "OD|Qty|Name|EMail Addr");

	for (i = 0; i < 20; i++)
	{
		XmLGridSetStringsPos(grid, XmCONTENT, i, XmCONTENT, 1, data[i]);
		if (i == 2 || i == 4 || i == 5 || i == 8 || i == 13)
			pix = checkPix;
		else
			pix = nocheckPix;
		XtVaSetValues(grid,
		    XmNcolumn, 0,
		    XmNrow, i,
		    XmNcellPixmap, pix,
		    NULL);
	}

	XtRealizeWidget(shell);
	XtAppMainLoop(app);
}

void showSelected(w, clientData, callData)
Widget w;
XtPointer clientData;
XtPointer callData;
{
	XmLCGridRow *row;
	XmLCGridColumn *column;
	Pixmap pix;
	int i, n;

	printf ("Selected Rows: ");
	XtVaGetValues(grid,
	    XmNrows, &n,
	    NULL);
	for (i = 0; i < n; i++)
	{
		row = XmLGridGetRow(grid, XmCONTENT, i);
		column = XmLGridGetColumn(grid, XmCONTENT, 0);
		XtVaGetValues(grid,
		    XmNrowPtr, row,
		    XmNcolumnPtr, column,
		    XmNcellPixmap, &pix,
		    NULL);
		if (pix == checkPix)
			printf ("%d ", i);
	}
	printf ("\n");
}

void cellSelect(w, clientData, callData)
Widget w;
XtPointer clientData;
XtPointer callData;
{
	XmLGridCallbackStruct *cbs;
	XmLCGridRow *row;
	XmLCGridColumn *column;
	Pixmap pix;

	cbs = (XmLGridCallbackStruct *)callData;
	if (cbs->reason != XmCR_SELECT_CELL)
		return;
	if (cbs->rowType != XmCONTENT)
		return;

	row = XmLGridGetRow(w, cbs->rowType, cbs->row);
	column = XmLGridGetColumn(w, XmCONTENT, 0);
	XtVaGetValues(w,
	    XmNrowPtr, row,
	    XmNcolumnPtr, column,
	    XmNcellPixmap, &pix,
	    NULL);
	if (pix == nocheckPix)
		pix = checkPix;
	else
		pix = nocheckPix;
	XtVaSetValues(w,
	    XmNrow, cbs->row,
	    XmNcolumn, 0,
	    XmNcellPixmap, pix,
	    NULL);
}
