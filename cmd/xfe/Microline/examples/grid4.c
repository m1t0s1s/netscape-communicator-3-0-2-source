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
#include <XmL/Grid.h>

#define TITLEFONT "-*-helvetica-bold-r-normal--18-*-*-*-*-*-iso8859-1"
#define BOLDFONT "-*-helvetica-bold-r-normal--14-*-*-*-*-*-iso8859-1"
#define TEXTFONT "-*-helvetica-medium-r-normal--12-*-*-*-*-*-iso8859-1"

static char *data =
"|1995 Income Summary\n\
|Shampoo|Conditioner|Soap|Total\n\
Revenues:\n\
Sales|$ 1,600,000|$ 1,000,000|$  800,000|$ 3,400,000\n\
Less Discounts|(16,000)|(10,000)|(8,000)|(34,000)\n\
Less Return Allowance|(8,000)|(5,000)|(4,000)|(17,000)\n\
  Net Revenue|1,576,000|985,000|792,000|3,349,000\n\
\n\
Less Expenses:\n\
Cost of Goods Sold|(640,000)|(330,000)|(264,000)|(1,234,000)\n\
Salary Expense|(380,000)|(280,000)|(180,000)|(840,000)\n\
Marketing Expense|(157,600)|(98,500)|(79,200)|(335,300)\n\
Rent Expense|(36,000)|(36,000)|(36,000)|(108,000)\n\
Misc. Other Expense|(36,408)|(22,335)|(16,776)|(75,519)\n\
  Total Expenses|(1,250,008)|(766,835)|(575,976)|(2,592,819)\n\
\n\
Income Tax Expense|(130,397)|(87,266)|(86,410)|(304,072)\n\
Net Income|195,595|130,899|129,614|456,109";

main(argc, argv)
int argc;
char *argv[];
{
	XtAppContext app;
	Widget shell, grid;
	int i, r;

	shell =  XtAppInitialize(&app, "Grid4", NULL, 0,
	    &argc, argv, NULL, NULL, 0);

	grid = XtVaCreateManagedWidget("grid",
	    xmlGridWidgetClass, shell,
	    XmNheight, 300,
	    XmNwidth, 500,
	    XtVaTypedArg, XmNbackground, XmRString, "white", 6,
	    XtVaTypedArg, XmNforeground, XmRString, "black", 6,
	    NULL);

	XtVaSetValues(grid, XmNlayoutFrozen, True, NULL);

	XmLGridAddColumns(grid, XmHEADING, -1, 1);
	XmLGridAddColumns(grid, XmCONTENT, -1, 3);
	XmLGridAddColumns(grid, XmFOOTER, -1, 1);

	/* set column widths */
	XtVaSetValues(grid,
	    XmNcolumnType, XmHEADING,
	    XmNcolumnWidth, 24,
	    NULL);
	XtVaSetValues(grid,
	    XmNcolumnWidth, 11,
	    NULL);
	XtVaSetValues(grid,
	    XmNcolumnType, XmFOOTER,
	    XmNcolumnWidth, 11,
	    NULL);

	/* add 'Income Summary' heading row */
	XtVaSetValues(grid, 
	    XmNcellDefaults, True,
	    XmNcellLeftBorderType, XmBORDER_NONE,
	    XmNcellRightBorderType, XmBORDER_NONE,
	    XmNcellTopBorderType, XmBORDER_NONE,
	    XtVaTypedArg, XmNcellBottomBorderPixel, XmRString, "black", 6,
	    XmNcellAlignment, XmALIGNMENT_CENTER,
	    XtVaTypedArg, XmNcellForeground, XmRString, "NavyBlue", 9,
	    NULL);
	XmLGridAddRows(grid, XmHEADING, -1, 1);
	XtVaSetValues(grid,
	    XmNrowType, XmHEADING,
	    XmNrow, 0,
	    XmNcolumn, 0,
	    XmNcellColumnSpan, 2,
	    XtVaTypedArg, XmNcellFontList, XmRString, TITLEFONT,
	    strlen(TITLEFONT) + 1,
	    NULL);

	/* add 'Shampoo Conditioner Soap' heading row */
	XtVaSetValues(grid, 
	    XmNcellDefaults, True,
	    XtVaTypedArg, XmNcellFontList, XmRString, BOLDFONT,
	    strlen(BOLDFONT) + 1,
	    XtVaTypedArg, XmNcellBackground, XmRString, "white", 6,
	    XtVaTypedArg, XmNcellForeground, XmRString, "black", 6,
	    XmNcellBottomBorderType, XmBORDER_NONE,
	    NULL);
	XmLGridAddRows(grid, XmHEADING, -1, 1);

	/* add content and footer rows */
	XtVaSetValues(grid,
	    XmNcellDefaults, True,
	    XmNcellAlignment, XmALIGNMENT_RIGHT,
	    XtVaTypedArg, XmNcellFontList, XmRString, TEXTFONT,
	    strlen(TEXTFONT) + 1,
	    NULL);
	XmLGridAddRows(grid, XmCONTENT, -1, 15);
	XmLGridAddRows(grid, XmFOOTER, -1, 1);

	/* left justify left-most (heading) column */
	XtVaSetValues(grid,
	    XmNcolumn, 0,
	    XmNcolumnType, XmHEADING,
	    XmNrowType, XmALL_TYPES,
	    XmNcellAlignment, XmALIGNMENT_LEFT,
	    NULL);

	/* bold 'Revenues' cell */
	XtVaSetValues(grid,
	    XmNcolumnType, XmHEADING,
	    XmNcolumn, 0,
	    XmNrow, 0,
	    XtVaTypedArg, XmNcellFontList, XmRString, BOLDFONT,
	    strlen(BOLDFONT) + 1,
	    NULL);

	/* bold 'Less Expenses' cell */
	XtVaSetValues(grid,
	    XmNcolumnType, XmHEADING,
	    XmNcolumn, 0,
	    XmNrow, 6,
	    XtVaTypedArg, XmNcellFontList, XmRString, BOLDFONT,
	    strlen(BOLDFONT) + 1,
	    NULL);

	/* grey middle and footer content column */
	XtVaSetValues(grid,
	    XmNrowType, XmALL_TYPES,
	    XmNcolumnType, XmALL_TYPES,
	    XmNcolumnRangeStart, 2,
	    XmNcolumnRangeEnd, 4,
	    XmNcolumnStep, 2,
	    XtVaTypedArg, XmNcellBackground, XmRString, "#E8E8E8", 8,
	    NULL);

	/* set 'Income Summary' heading row yellow */
	XtVaSetValues(grid,
	    XmNcolumnType, XmALL_TYPES,
	    XmNrowType, XmHEADING,
	    XmNrow, 0,
	    XtVaTypedArg, XmNcellBackground, XmRString, "yellow", 7,
	    NULL);

	/* blue and bold 'Net Revenue' and 'Total Expenses' rows */
	XtVaSetValues(grid,
	    XmNrowRangeStart, 4,
	    XmNrowRangeEnd, 12,
	    XmNrowStep, 8,
	    XmNcolumnType, XmALL_TYPES,
	    XtVaTypedArg, XmNcellForeground, XmRString, "white", 6,
	    XtVaTypedArg, XmNcellBackground, XmRString, "NavyBlue", 9,
	    XtVaTypedArg, XmNcellFontList, XmRString, BOLDFONT,
	    strlen(BOLDFONT) + 1,
	    NULL);

	/* blue and bold 'Net Income' footer row */
	XtVaSetValues(grid,
	    XmNrow, 0,
	    XmNrowType, XmFOOTER,
	    XmNcolumnType, XmALL_TYPES,
	    XtVaTypedArg, XmNcellForeground, XmRString, "white", 6,
	    XtVaTypedArg, XmNcellBackground, XmRString, "NavyBlue", 9,
	    XtVaTypedArg, XmNcellFontList, XmRString, BOLDFONT,
	    strlen(BOLDFONT) + 1,
	    NULL);

	XtVaSetValues(grid, XmNlayoutFrozen, False, NULL);

	XmLGridSetStrings(grid, data);

	XtRealizeWidget(shell);
	XtAppMainLoop(app);
}
