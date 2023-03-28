/* -*- Mode: C; tab-width: 4 -*- 
 *
 *  editordialogs.c --- Editor-specific dialogs.
 *  Should only be built for the editor.
 *  Copyright © 1996 Netscape Communications Corporation, all rights reserved.
 *  Created: David Williams <djw@netscape.com>, Mar-12-1996
 *
 *  RCSID: "$Id: editordialogs.c,v 1.78.8.1 1996/09/13 02:53:58 briano Exp $"
 */
#include "mozilla.h"
#include "xfe.h"
#include <X11/keysym.h>   /* for editor key translation */
#include <XmL/Folder.h>   /* tab folder stuff */
#include <Xm/Label.h> 
#include <Xm/DrawnB.h> 
#include <Xm/XmP.h>       /* required for _XmFontListGetDefaultFont() */
#include <xpgetstr.h>     /* for XP_GetString() */

#include "edttypes.h"
#include "edt.h"

#include "menu.h"

#include "xeditor.h"

/* no security btn on prefs
#define _SECURITY_BTN_ON_PREFS
*/

/*
 *    I don't like this "fix" for MAXPATHLEN, but I cannot log onto
 *    a SCO machine to find the right way to do this...djw
 */
#ifdef SCO_SV
#include "mcom_db.h"	/* for MAXPATHLEN */
#endif

void fe_browse_file_of_text (MWContext *context, Widget text_field,
			     Boolean dirp);

extern int XFE_ERROR_SAVING_OPTIONS;
extern int XFE_VALUES_OUT_OF_RANGE;
extern int XFE_VALUE_OUT_OF_RANGE;
extern int XFE_EDITOR_TABLE_ROW_RANGE;
extern int XFE_EDITOR_TABLE_COLUMN_RANGE;
extern int XFE_EDITOR_TABLE_BORDER_RANGE;
extern int XFE_EDITOR_TABLE_SPACING_RANGE;
extern int XFE_EDITOR_TABLE_PADDING_RANGE;
extern int XFE_EDITOR_TABLE_WIDTH_RANGE;
extern int XFE_EDITOR_TABLE_HEIGHT_RANGE;
extern int XFE_EDITOR_TABLE_IMAGE_WIDTH_RANGE;
extern int XFE_EDITOR_TABLE_IMAGE_HEIGHT_RANGE;
extern int XFE_EDITOR_TABLE_IMAGE_SPACE_RANGE;
extern int XFE_ENTER_NEW_VALUE;
extern int XFE_ENTER_NEW_VALUES;
extern int XFE_EDITOR_LINK_TEXT_LABEL_NEW;
extern int XFE_EDITOR_LINK_TEXT_LABEL_IMAGE;
extern int XFE_EDITOR_LINK_TEXT_LABEL_TEXT;
extern int XFE_EDITOR_LINK_TARGET_LABEL_NO_TARGETS;
extern int XFE_EDITOR_LINK_TARGET_LABEL_SPECIFIED;
extern int XFE_EDITOR_LINK_TARGET_LABEL_CURRENT;
extern int XFE_EDITOR_WARNING_REMOVE_LINK;
extern int XFE_UNKNOWN;
extern int XFE_EDITOR_TAG_UNOPENED;
extern int XFE_EDITOR_TAG_UNCLOSED;
extern int XFE_EDITOR_TAG_UNTERMINATED_STRING;
extern int XFE_EDITOR_TAG_PREMATURE_CLOSE;
extern int XFE_EDITOR_TAG_TAGNAME_EXPECTED;
extern int XFE_EDITOR_TAG_UNKNOWN;
extern int XFE_EDITOR_TAG_OK;
extern int XFE_EDITOR_AUTOSAVE_PERIOD_RANGE;
extern int XFE_EDITOR_PUBLISH_LOCATION_INVALID;
extern int XFE_EDITOR_IMAGE_IS_REMOTE;
extern int XFE_CANNOT_READ_FILE;


#define TABLE_MAX_ROWS 100
#define TABLE_MIN_ROWS 1
#define TABLE_MAX_COLUMNS 100
#define TABLE_MIN_COLUMNS 1
#define TABLE_MAX_BORDER 10000
#define TABLE_MIN_BORDER 0
#define TABLE_MAX_SPACING 10000
#define TABLE_MIN_SPACING 0
#define TABLE_MAX_PADDING 10000
#define TABLE_MIN_PADDING 0
#define TABLE_MAX_PIXEL_WIDTH  10000
#define TABLE_MIN_PIXEL_WIDTH  1
#define TABLE_MAX_PERCENT_WIDTH 100
#define TABLE_MIN_PERCENT_WIDTH 1
#define TABLE_MAX_PIXEL_HEIGHT  10000
#define TABLE_MIN_PIXEL_HEIGHT  1
#define TABLE_MAX_PERCENT_HEIGHT 100
#define TABLE_MIN_PERCENT_HEIGHT 1
#define TABLE_MAX_CELL_NROWS 100
#define TABLE_MIN_CELL_NROWS 1
#define TABLE_MAX_CELL_NCOLUMNS 100
#define TABLE_MIN_CELL_NCOLUMNS 1

#define IMAGE_MIN_WIDTH  1
#define IMAGE_MAX_WIDTH  10000
#define IMAGE_MIN_HEIGHT 1
#define IMAGE_MAX_HEIGHT 10000
#define IMAGE_MIN_HSPACE 0
#define IMAGE_MAX_HSPACE 10000
#define IMAGE_MIN_VSPACE 0
#define IMAGE_MAX_VSPACE 10000
#define IMAGE_MIN_BORDER 0
#define IMAGE_MAX_BORDER 10000

#define HRULE_MIN_HEIGHT 1
#define HRULE_MAX_HEIGHT 10000
#define HRULE_MAX_PIXEL_WIDTH  10000
#define HRULE_MIN_PIXEL_WIDTH  1
#define HRULE_MAX_PERCENT_WIDTH 100
#define HRULE_MIN_PERCENT_WIDTH 1

#define AUTOSAVE_MIN_PERIOD 0
#define AUTOSAVE_MAX_PERIOD 600

#define RANGE_CHECK(o, a, b) ((o) < (a) || (o) > (b))

#define XFE_INVALID_TABLE_NROWS     1
#define XFE_INVALID_TABLE_NCOLUMNS  2
#define XFE_INVALID_TABLE_BORDER    3
#define XFE_INVALID_TABLE_SPACING   4
#define XFE_INVALID_TABLE_PADDING   5
#define XFE_INVALID_TABLE_WIDTH     6
#define XFE_INVALID_TABLE_HEIGHT    7
#define XFE_INVALID_CELL_NROWS      8
#define XFE_INVALID_CELL_NCOLUMNS   9
#define XFE_INVALID_CELL_WIDTH      10
#define XFE_INVALID_CELL_HEIGHT     11

#define XFE_INVALID_IMAGE_WIDTH     12
#define XFE_INVALID_IMAGE_HEIGHT    13
#define XFE_INVALID_IMAGE_HSPACE    14
#define XFE_INVALID_IMAGE_VSPACE    15
#define XFE_INVALID_IMAGE_BORDER    16

#define XFE_INVALID_HRULE_WIDTH     17
#define XFE_INVALID_HRULE_HEIGHT    18

static void
fe_error_dialog(MWContext* context, Widget parent, char* s)
{
	while (!XtIsWMShell(parent) && (XtParent(parent)!=0))
		parent = XtParent(parent);

	fe_dialog(parent, "error", s, FALSE, 0, FALSE, FALSE, 0);
}

static void
fe_message_dialog(MWContext* context, Widget parent, char* s)
{
	Widget mainw;

	while (!XtIsWMShell(parent) && (XtParent(parent)!=0))
		parent = XtParent(parent);

	/* yuck */
	mainw = CONTEXT_WIDGET(context);
	CONTEXT_WIDGET(context) = parent;

	fe_Message(context, s);

	CONTEXT_WIDGET(context) = mainw;
}

static void
fe_editor_range_error_dialog(MWContext* context, Widget parent,
							 unsigned* errors, unsigned nerrors)
{
	unsigned i;
	int      id;
	char     be_lazy[8192];

	if (nerrors > 1)
		id = XFE_VALUES_OUT_OF_RANGE; /* Some values are out of range: */
	else if (nerrors == 1)
		id = XFE_VALUE_OUT_OF_RANGE; /* The following value is out of range: */
	else
		return;

	strcpy(be_lazy, XP_GetString(id));
	strcat(be_lazy, "\n\n");

	for (i = 0; i < nerrors; i++) {
		switch (errors[i]) {
		case XFE_INVALID_TABLE_NROWS:
		case XFE_INVALID_CELL_NROWS:
			id = XFE_EDITOR_TABLE_ROW_RANGE;
			/* You can have between 1 and 100 rows. */
			break;
		case XFE_INVALID_TABLE_NCOLUMNS:
		case XFE_INVALID_CELL_NCOLUMNS:
			id = XFE_EDITOR_TABLE_COLUMN_RANGE;
			/* You can have between 1 and 100 columns. */
			break;
		case XFE_INVALID_TABLE_BORDER:
			id = XFE_EDITOR_TABLE_BORDER_RANGE;
			/* For border width, you can have 0 to 10000 pixels. */
			break;
		case XFE_INVALID_TABLE_SPACING:
			id = XFE_EDITOR_TABLE_SPACING_RANGE;
			/* For cell spacing, you can have 0 to 10000 pixels. */
			break;
		case XFE_INVALID_TABLE_PADDING:
			id = XFE_EDITOR_TABLE_PADDING_RANGE;
			/* For cell padding, you can have 0 to 10000 pixels. */
			break;
		case XFE_INVALID_TABLE_WIDTH:
		case XFE_INVALID_CELL_WIDTH:
		case XFE_INVALID_HRULE_WIDTH:
			id = XFE_EDITOR_TABLE_WIDTH_RANGE;
			/* For width, you can have between 1 and 10000 pixels, */
			/* or between 1 and 100%.                              */
			break;
		case XFE_INVALID_TABLE_HEIGHT:
		case XFE_INVALID_CELL_HEIGHT:
			id = XFE_EDITOR_TABLE_HEIGHT_RANGE;
			/* For height, you can have between 1 and 10000 pixels, */
			/* or between 1 and 100%.                               */
			break;
		case XFE_INVALID_IMAGE_WIDTH:
			id = XFE_EDITOR_TABLE_IMAGE_WIDTH_RANGE;
			/* For width, you can have between 1 and 10000 pixels. */
			break;
		case XFE_INVALID_IMAGE_HEIGHT:
		case XFE_INVALID_HRULE_HEIGHT:
			id = XFE_EDITOR_TABLE_IMAGE_HEIGHT_RANGE;
			/* For height, you can have between 1 and 10000 pixels. */
			break;
		case XFE_INVALID_IMAGE_HSPACE:
		case XFE_INVALID_IMAGE_VSPACE:
		case XFE_INVALID_IMAGE_BORDER:
			id = XFE_EDITOR_TABLE_IMAGE_SPACE_RANGE;
			/* For space, you can have between 1 and 10000 pixels. */
			break;
		}

		strcat(be_lazy, XP_GetString(id));
		strcat(be_lazy, "\n");
	}

	if (nerrors > 1) {
		id = XFE_ENTER_NEW_VALUES;
		/* Please enter new values and try again. */
    } else if (nerrors == 1) {
		id = XFE_ENTER_NEW_VALUE;
		/* Please enter a new value and try again. */
	}

	strcat(be_lazy, "\n");
	strcat(be_lazy, XP_GetString(id));

	fe_error_dialog(context, parent, be_lazy);
}

static Widget
fe_CreateTabForm(Widget parent, char* name, Arg* args, Cardinal n)
{
	Widget form;
	Widget tab;
	Pixel  bg;
	char*    string;
	XmString xm_string;

	XtVaGetValues(parent, XmNbackground, &bg, 0);
	form = XtVaCreateWidget(name,
							xmFormWidgetClass, parent,
							XmNbackground, bg,
							NULL);

	string = fe_ResourceString(form, "tabLabelString", XmCXmString);
	if (!string)
		string = name;
	xm_string = XmStringCreateSimple(string);
	tab = XmLFolderAddTab(parent, xm_string);
	XtVaSetValues(tab, XmNtabManagedWidget, form, NULL);

	return form;
}

void
fe_WidgetSetSensitive(Widget widget, Boolean sensitive)
{
    XtVaSetValues(widget, XmNsensitive, sensitive, 0);
}

#define SWATCH_SIZE 60

Widget
fe_CreateSwatch(Widget parent, char* name, Arg* p_args, Cardinal p_n)
{
    return XmCreateDrawnButton(parent, name, p_args, p_n);
}

void
fe_SwatchSetSensitive(Widget widget, Boolean sensitive)
{
    XtVaSetValues(widget, XmNsensitive, sensitive, 0);
}

static LO_Color swatch_black;

void
fe_SwatchSetColor(Widget widget, LO_Color* color)
{
	MWContext* context = fe_WidgetToMWContext(widget);
	Pixel      pixel;

	if (!color)
		color = &swatch_black; /* default color */

	pixel = fe_GetPixel(context,
						color->red,
						color->green,
						color->blue);

	XtVaSetValues(widget, XmNbackground, pixel, 0);
}

Widget
fe_CreatePasswordField(Widget parent, char* name, Arg* args, Cardinal n)
{
	Widget widget;
	int    max_length;

	widget = fe_CreateTextField(parent, name, args, n);

	XtVaGetValues(widget, XmNmaxLength, &max_length, 0);

	fe_SetupPasswdText(widget, max_length);

	return widget;
}

void
fe_TextFieldSetString(Widget widget, char* value, Boolean notify)
{
	XtCallbackRec buf[32]; /* hope that's enough! */
	XtCallbackList callbacks;
	int i;
	if (notify == FALSE) {
		XtVaGetValues(widget, XmNvalueChangedCallback, &callbacks, 0);
		for (i = 0; callbacks[i].callback != NULL; i++) {
			buf[i] = callbacks[i];
		}
		buf[i].callback = NULL;
		buf[i].closure = NULL;
		
		XtRemoveAllCallbacks(widget, XmNvalueChangedCallback);
	}
	XmTextFieldSetString(widget, value);
	if (notify == FALSE) {
		XtAddCallbacks(widget, XmNvalueChangedCallback, buf);
	}
}

void
fe_TextFieldSetEditable(MWContext* context, Widget widget, Boolean editable)
{
	Pixel top_shadow_pixel;
	Pixel bottom_shadow_pixel;
	Pixel bg_pixel;
	Arg   args[16];
	Cardinal n;

	/*
	 *    The TextField is so losing, it doesn't set the
	 *    background color to indicate the field is not-editable.
	 *    The Gods of Motif style say thou shalt bestow unto thine
	 *    non-editable TextField thine color of select color-ness.
	 *
	 *    Hmmm this looks butt ugly, maybe have to use a label instead.
	 */
	if (editable) {
		top_shadow_pixel = CONTEXT_DATA(context)->top_shadow_pixel;
		bottom_shadow_pixel = CONTEXT_DATA(context)->bottom_shadow_pixel;
		bg_pixel = CONTEXT_DATA(context)->text_bg_pixel;
	} else {
#if 1
		n = 0;
		XtSetArg(args[n], XmNbackground, &bg_pixel); n++;
		XtGetValues(XtParent(widget), args, n);
#else
		bg_pixel = CONTEXT_DATA(context)->default_bg_pixel;
#endif
		top_shadow_pixel = CONTEXT_DATA(context)->top_shadow_pixel;
		bottom_shadow_pixel = CONTEXT_DATA(context)->bottom_shadow_pixel;
	}

#if 0
	XtSetArg(args[n], XmNnavigationType, XmNONE); n++;
#endif

	n = 0;
	XtSetArg(args[n], XmNeditable, editable); n++;
	XtSetArg(args[n], XmNcursorPositionVisible, editable); n++;
	XtSetArg(args[n], XmNtraversalOn, editable); n++;
	XtSetArg(args[n], XmNbackground, bg_pixel); n++;
	XtSetArg(args[n], XmNtopShadowColor, top_shadow_pixel); n++;
	XtSetArg(args[n], XmNbottomShadowColor, bottom_shadow_pixel); n++;
	XtSetValues(widget, args, n);
}

Widget
fe_CreateFrame(Widget parent, char* name,  Arg* p_args, Cardinal p_n)
{
    Widget   frame;
    Widget   title;
    char     namebuf[256];
    Arg      args[16];
    Cardinal n;

    for (n = 0; n < p_n; n++) {
	XtSetArg(args[n], p_args[n].name, p_args[n].value);
    }

    strcpy(namebuf, name);
    strcat(namebuf, "Frame");

    XtSetArg(args[n], XmNshadowType, XmSHADOW_ETCHED_IN); n++;
    frame = XmCreateFrame(parent, namebuf, args, n);
    XtManageChild(frame);

    strcpy(namebuf, name);
    strcat(namebuf, "Title");

    n = 0;
    XtSetArg(args[n], XmNlabelType, XmSTRING); n++;
    XtSetArg(args[n], XmNchildType, XmFRAME_TITLE_CHILD); n++;
    XtSetArg(args[n], XmNchildHorizontalAlignment, XmALIGNMENT_BEGINNING); n++;
    XtSetArg(args[n], XmNchildVerticalAlignment, XmALIGNMENT_CENTER); n++;
    title = XmCreateLabelGadget(frame, namebuf, args, n);
    XtManageChild(title);

    return frame;
}

Widget
fe_CreateFramedForm(Widget parent, char* name, Arg* p_args, Cardinal p_n)
{
    Widget   frame;
    Widget   form;
    Arg      args[16];
    Cardinal n;

    n = 0;
    frame = fe_CreateFrame(parent, name, args, n);
    XtManageChild(frame);

    for (n = 0; n < p_n; n++) {
	XtSetArg(args[n], p_args[n].name, p_args[n].value);
    }

    form = XmCreateForm(frame, name, args, n);

    return form;
}

Widget
fe_CreateFramedRowColumn(Widget parent, char* name, Arg* p_args, Cardinal p_n)
{
    Widget   frame;
    Widget   rowcol;
    Arg      args[16];
    Cardinal n;

    n = 0;
    frame = fe_CreateFrame(parent, name, args, n);
    XtManageChild(frame);

    for (n = 0; n < p_n; n++) {
	XtSetArg(args[n], p_args[n].name, p_args[n].value);
    }
    rowcol = XmCreateRowColumn(frame, name, args, n);

    return rowcol;
}

static Widget
fe_CreateToggleButtonGadget(Widget parent, char* name,
			    Arg* p_args, Cardinal p_nargs)
{
    Arg      args[16];
    Cardinal n = 0;
    Widget   widget;

    for (n = 0; n < p_nargs; n++) {
	XtSetArg(args[n], p_args[n].name, p_args[n].value); n++;
    }

    XtSetArg(args[n], XmNindicatorType, XmN_OF_MANY); n++;
    widget = XmCreateToggleButtonGadget(parent, name, args, n);
    return widget;
}


static Widget
fe_CreateRadioButtonGadget(Widget parent, char* name,
			    Arg* p_args, Cardinal p_nargs)
{
    Arg      args[16];
    Cardinal n = 0;
    Widget   widget;

    for (n = 0; n < p_nargs; n++) {
	XtSetArg(args[n], p_args[n].name, p_args[n].value); n++;
    }

    XtSetArg(args[n], XmNindicatorType, XmONE_OF_MANY); n++;
    widget = XmCreateToggleButtonGadget(parent, name, args, n);
    return widget;
}

static Widget
fe_CreatePulldownMenu(Widget parent, char* name, Arg* p_argv, Cardinal p_argc)
{
  unsigned i;
  Widget   popup_menu;
  Arg      argv[8];
  Cardinal argc;
  Visual*  v = 0;
  Colormap cmap = 0;
  Cardinal depth = 0;
  Widget   widget;

  XtCallbackRec* button_callback_rec = NULL;

  for (i = 0; i < p_argc; i++) {
      if (p_argv[i].name == XmNactivateCallback)
	  button_callback_rec = (XtCallbackRec*)p_argv[i].value;
  }

  for (widget = parent; !XtIsWMShell(widget); widget = XtParent(widget))
      ;

  XtVaGetValues(widget, XtNvisual, &v, XtNcolormap, &cmap,
	XtNdepth, &depth, 0);

  argc = 0;
  XtSetArg (argv[argc], XmNvisual, v); argc++;
  XtSetArg (argv[argc], XmNdepth, depth); argc++;
  XtSetArg (argv[argc], XmNcolormap, cmap); argc++;

  popup_menu = XmCreatePulldownMenu(parent, name, argv, argc);

  return popup_menu;
}

static Widget
fe_CreateOptionMenuNoLabel(Widget widget, char* name, Arg* args, Cardinal n)
{
	Widget menu = fe_CreateOptionMenu(widget, name, args, n);
	fe_UnmanageChild_safe(XmOptionLabelGadget(menu));
	return menu;
}

typedef struct fe_DialogBuilderCreator {
    char*   name;
    Widget (*func)(Widget parent, char* name, ArgList args, Cardinal argcount);
    Boolean recurse;
} fe_DialogBuilderCreator;

static char XFE_DB_PUSHBUTTONGADGET[] = "XmPushButtonGadget";
static char XFE_DB_LABELGADGET[] = "XmLabelGadget";
static char XFE_DB_FRAME[] = "XmFrame";
static char XFE_DB_FORM[] = "XmForm";
static char XFE_DB_TEXTFIELD[] = "XmTextField";
static char XFE_DB_OPTIONMENU[] = "XmOptionMenu";
static char XFE_DB_PULLDOWNMENU[] = "fe_PulldownMenu";
static char XFE_DB_ROWCOL[] = "XmRowColumn";
static char XFE_DB_FRAMEDFORM[] = "fe_FramedForm";
static char XFE_DB_TOGGLEGADGET[] = "fe_ToggleButtonGadget";
static char XFE_DB_RADIOGADGET[] = "fe_RadioButtonGadget";
static char XFE_DB_FRAMEDROWCOL[] = "fe_FramedRowCol";

static fe_DialogBuilderCreator fe_DialogBuilderDefaultCreators[] = {
    { XFE_DB_PUSHBUTTONGADGET, XmCreatePushButtonGadget, FALSE },
    { XFE_DB_LABELGADGET, XmCreateLabelGadget, FALSE },
    { XFE_DB_TEXTFIELD, XmCreateTextField, FALSE },
    { XFE_DB_TOGGLEGADGET, fe_CreateToggleButtonGadget, FALSE },
    { XFE_DB_RADIOGADGET, fe_CreateRadioButtonGadget, FALSE },

    { XFE_DB_FRAME, XmCreateFrame, TRUE },
    { XFE_DB_FORM, XmCreateForm, TRUE },
    { XFE_DB_PULLDOWNMENU, fe_CreatePulldownMenu, TRUE },
    { XFE_DB_OPTIONMENU, fe_CreateOptionMenuNoLabel, FALSE },
    { XFE_DB_ROWCOL, XmCreateRowColumn, TRUE },
    { XFE_DB_FRAMEDFORM, fe_CreateFramedForm, TRUE },
    { XFE_DB_FRAMEDROWCOL, fe_CreateFramedRowColumn, TRUE },
    { 0 }
};

static char XFE_DB_END[] = "end";
static char XFE_DB_PUSH[] = "push";
static char XFE_DB_POP[] = "pop";
static char XFE_DB_WIDGETCALLED[] = "widgetcalled";
static char XFE_DB_WIDEST[] = "widest";
static char XFE_DB_TALLEST[] = "tallest";
static char XFE_DB_SETVAL[] = "setval";
static char XFE_DB_SETDATA[] = "setdata";
static char XFE_DB_SETVALCALLED[] = "setvalcalled";
static char XFE_DB_CALLBACK[] = "callback";

typedef struct fe_DialogBuilderArg {
    char*     name;
    XtPointer value;
} fe_DialogBuilderArg;


static fe_DialogBuilderCreator*
find_creator(fe_DialogBuilderCreator* creators, char* name)
{

    int i;
    
    for (i = 0; creators[i].name; i++) {
	if (creators[i].name == name || strcmp(creators[i].name, name) == 0)
	    return &creators[i];
    }
    return NULL;
}

Widget
db_do_work(
	   MWContext* context,
	   Widget parent,
	   fe_DialogBuilderCreator* creators,
	   fe_DialogBuilderArg** instructions_a,
	   void* data)
{
#define MACHINE_NARGS     16
#define MACHINE_NSTACK    16
#define MACHINE_NCALLBACK 16
#define MACHINE_NWIDGETS  32
    Arg           args[MACHINE_NARGS];
    XtPointer     stack[MACHINE_NSTACK];
    XtCallbackRec callbacks[MACHINE_NCALLBACK];
    Cardinal      stackposn;
#define POP()   (stack[--stackposn])
#define PUSH(x) (stack[stackposn++] = (XtPointer)(x))
    Cardinal      nargs;
    Cardinal      ncallbacks;
    Widget        children[MACHINE_NWIDGETS];
    Cardinal      nchildren;
    char*         name;
    XtPointer     value;
    Dimension     width;
    Dimension     max_width;
    Dimension     height;
    Dimension     max_height;
    Widget widget = NULL;
    Widget max_widget;
    int i;
    fe_DialogBuilderCreator* creator;

    nargs = 0;
    stackposn = 0;
    ncallbacks = 0;
    nchildren = 0;

    for (; (*instructions_a)->name != XFE_DB_END; (*instructions_a)++) {
	name = (*instructions_a)->name;
	value = (*instructions_a)->value;

	if (value == (XtPointer)XFE_DB_POP) /* pop is only ever a value */
	    value = POP();

	if (name == XFE_DB_PUSH) {
	    /* push is only ever a name */
	    PUSH(value);
	} else if (name == XFE_DB_WIDGETCALLED) {
	    for (i = 0; i < nchildren; i++) {
		if (strcmp(XtName(children[i]), (char*)value) == 0) {
		    PUSH(children[i]);
		    break;
		}
	    }
	    XP_ASSERT(i < nchildren);
	} else if (name == XFE_DB_WIDEST) {
	    max_width = 0;
	    max_widget = 0;
	    for (i = 0; i < (unsigned)value; i++) {
		widget = (Widget)POP();
		XtVaGetValues(widget, XtNwidth, &width, 0);
		if (width > max_width) {
		    max_width = width;
		    max_widget = widget;
		}
	    }
	    PUSH(max_widget);
	} else if (name == XFE_DB_TALLEST) {
	    max_height = 0;
	    max_widget = 0;
	    for (i = 0; i < (unsigned)value; i++) {
		widget = (Widget)POP();
		XtVaGetValues(widget, XtNheight, &height, 0);
		if (height > max_height) {
		    max_height = height;
		    max_widget = widget;
		}
	    }
	    PUSH(max_widget);
	} else if (name == XFE_DB_SETVAL) {
	    widget = (Widget)POP();
	    XtSetValues(widget, args, nargs);
	    nargs = 0;
	} else if (name == XFE_DB_SETVALCALLED) {
	    for (i = 0; i < nchildren; i++) {
		if (strcmp(XtName(children[i]), (char*)value) == 0) {
		    XtSetValues(children[i], args, nargs);
		    nargs = 0;
		    break;
		}
	    }
	    XP_ASSERT(i < nchildren);
	} else if (name == XFE_DB_SETDATA) {
	    Widget* foo = (Widget*)(((char*) data) + (int) (value));
	    *foo = (Widget)POP();
	} else if (name == XFE_DB_CALLBACK) {
	    callbacks[ncallbacks].callback = (XtCallbackProc)POP();
	    callbacks[ncallbacks].closure = (XtPointer)POP();
	    PUSH(&callbacks[ncallbacks++]);

	    /*
	     *    See if it's a creator.
	     */
	} else if ((creator = find_creator(creators, name))) {

	    widget = (*creator->func)(
				     parent,
				     (char*)value,
				     args,
				     nargs
				     );
	    /*
	     *    Hack, hack, hack for menus which get managed by cascades.
	     */
	    if (creator->name != XFE_DB_PULLDOWNMENU)
			XtManageChild(widget);
	    children[nchildren++] = widget;
	    nargs = 0;
	    /* if (XtIsSubclass(widget, compositeWidgetClass)) { */
	    if (creator->recurse) {
		(*instructions_a)++;
		db_do_work(
			   context,
			   widget,
			   creators,
			   instructions_a,
			   data);
		/* call ourselves */
	    }
	} else { 
	    /*
	     *    Must be an argument.
	     */
	    args[nargs].name = name;
	    args[nargs].value = (XtArgVal)value;
	    nargs++;
	}
    }

#if 0
    XtManageChildren(children, nchildren);
#endif

    return children[0];
}

/*
 *    Stuff for dialogs.
 */
typedef struct fe_HorizontalRulePropertiesDialogData {
	MWContext* context;
    Widget height;
    Widget width;
    Widget width_menu;
    Widget width_units;
    Widget width_pixels;
    Widget width_percent;
    Widget align_left;
    Widget align_center;
    Widget align_right;
    Widget three_d_shading;
} fe_HorizontalRulePropertiesDialogData;

/*
 *    DB program to build Horizontal Rule properties Dialog.
 */
static fe_DialogBuilderArg fe_HorizontalRulePropertiesDialogProgram[] = {

#define OFFSET(x) XtOffset(fe_HorizontalRulePropertiesDialogData *, x)

    { XmNorientation, (XtPointer)XmVERTICAL },
    { XFE_DB_ROWCOL, (XtPointer)"rowcol" },

        { XFE_DB_FRAMEDFORM, "dimensions" },
            { XFE_DB_LABELGADGET, "heightLabel" },
            { XFE_DB_TEXTFIELD, "heightText" },
            { XFE_DB_LABELGADGET, "pixels" },
            { XFE_DB_LABELGADGET, "widthLabel" },
            { XFE_DB_TEXTFIELD, "widthText" },
            { XFE_DB_PULLDOWNMENU, "widthUnitsPulldown" },
                { XFE_DB_PUSHBUTTONGADGET, "percent" },
                { XFE_DB_PUSHBUTTONGADGET, "pixels" },

                { XFE_DB_WIDGETCALLED, "percent" },
             	{ XFE_DB_SETDATA, (XtPointer)OFFSET(width_percent) },
                { XFE_DB_WIDGETCALLED, "pixels" },
             	{ XFE_DB_SETDATA, (XtPointer)OFFSET(width_pixels) },

                { XFE_DB_END, "widthUnitsPulldown" },
            { XFE_DB_WIDGETCALLED, "widthUnitsPulldown" },
            { XmNsubMenuId, XFE_DB_POP },
            { XFE_DB_OPTIONMENU, "widthUnits" },

            /* now do computed attachments, oh boy, we need a compiler */
            { XmNtopAttachment, (XtPointer)XmATTACH_FORM },
            { XmNleftAttachment, (XtPointer)XmATTACH_FORM },
            { XFE_DB_SETVALCALLED, "heightLabel" }, /* set heightLabel */

            { XmNtopAttachment, (XtPointer)XmATTACH_FORM },
            { XmNleftAttachment, (XtPointer)XmATTACH_WIDGET },
            { XFE_DB_WIDGETCALLED, "heightLabel" },
            { XFE_DB_WIDGETCALLED, "widthLabel" },
            { XFE_DB_WIDEST, (XtPointer)2 },
            { XmNleftWidget, XFE_DB_POP },
            { XFE_DB_SETVALCALLED, "heightText" }, /* set heightText */

            { XmNtopAttachment, (XtPointer)XmATTACH_FORM },
            { XmNleftAttachment, (XtPointer)XmATTACH_WIDGET },
            { XFE_DB_WIDGETCALLED, "heightText" },
            { XmNleftWidget, XFE_DB_POP },
            { XFE_DB_SETVALCALLED, "pixels" },     /* set pixel */

            { XmNtopAttachment, (XtPointer)XmATTACH_WIDGET },
            { XFE_DB_WIDGETCALLED, "heightText" },
            { XmNtopWidget, XFE_DB_POP },
            { XmNleftAttachment, (XtPointer)XmATTACH_FORM },
            { XFE_DB_SETVALCALLED, "widthLabel" }, /* set widthLabel */

            { XmNtopAttachment, (XtPointer)XmATTACH_WIDGET },
            { XFE_DB_WIDGETCALLED, "heightText" },
            { XmNtopWidget, XFE_DB_POP },
            { XmNleftAttachment, (XtPointer)XmATTACH_WIDGET },
            { XFE_DB_WIDGETCALLED, "heightLabel" },
            { XFE_DB_WIDGETCALLED, "widthLabel" },
            { XFE_DB_WIDEST, (XtPointer)2 },
            { XmNleftWidget, XFE_DB_POP },
            { XFE_DB_SETVALCALLED, "widthText" }, /* set widthText */

            { XmNtopAttachment, (XtPointer)XmATTACH_WIDGET },
            { XFE_DB_WIDGETCALLED, "heightText" },
            { XmNtopWidget, XFE_DB_POP },
            { XmNleftAttachment, (XtPointer)XmATTACH_WIDGET },
            { XFE_DB_WIDGETCALLED, "widthText" },
            { XmNleftWidget, XFE_DB_POP },
#if 0
            { XmNrightAttachment, (XtPointer)XmATTACH_FORM },
#endif
            { XFE_DB_SETVALCALLED, "widthUnits" }, /* set widthUnits */

         	/* do offsets */
            { XFE_DB_WIDGETCALLED, "widthText" },
         	{ XFE_DB_SETDATA, (XtPointer)OFFSET(width) },
            { XFE_DB_WIDGETCALLED, "heightText" },
         	{ XFE_DB_SETDATA, (XtPointer)OFFSET(height) },
            { XFE_DB_WIDGETCALLED, "widthUnitsPulldown" },
         	{ XFE_DB_SETDATA, (XtPointer)OFFSET(width_menu) },
            { XFE_DB_WIDGETCALLED, "widthUnits" },
         	{ XFE_DB_SETDATA, (XtPointer)OFFSET(width_units) },

            { XFE_DB_END, "dimensions" },

        { XmNorientation, (XtPointer)XmHORIZONTAL },
        { XmNradioBehavior, (XtPointer)TRUE },
        { XFE_DB_FRAMEDROWCOL, "align" },
            { XFE_DB_RADIOGADGET, "left" },
            { XFE_DB_RADIOGADGET, "center" },
            { XFE_DB_RADIOGADGET, "right" },

            { XFE_DB_WIDGETCALLED, "left" },
         	{ XFE_DB_SETDATA, (XtPointer)OFFSET(align_left) },
            { XFE_DB_WIDGETCALLED, "center" },
         	{ XFE_DB_SETDATA, (XtPointer)OFFSET(align_center) },
            { XFE_DB_WIDGETCALLED, "right" },
         	{ XFE_DB_SETDATA, (XtPointer)OFFSET(align_right) },

            { XFE_DB_END, "align" },
        { XFE_DB_TOGGLEGADGET, "threeDShading" },

	    { XFE_DB_WIDGETCALLED, "threeDShading" },
	    { XFE_DB_SETDATA, (XtPointer)OFFSET(three_d_shading) },

        { XFE_DB_END, "rowcol" },

     { XFE_DB_END, "program" }

#undef OFFSET

};

Widget
fe_CreatePromptDialog(
		      MWContext *context, char* name,
		      Boolean ok, Boolean cancel, Boolean apply, Boolean separator, Boolean modal)
{
  Widget mainw = CONTEXT_WIDGET(context);
  Widget dialog;
  Visual *v = 0;
  Colormap cmap = 0;
  Cardinal depth = 0;
  Arg av [20];
  int ac;

  XtVaGetValues (mainw, XtNvisual, &v, XtNcolormap, &cmap,
                 XtNdepth, &depth, 0);
  ac = 0;
  XtSetArg (av[ac], XmNvisual, v); ac++;
  XtSetArg (av[ac], XmNdepth, depth); ac++;
  XtSetArg (av[ac], XmNcolormap, cmap); ac++;
  XtSetArg (av[ac], XmNallowShellResize, TRUE); ac++;
  XtSetArg (av[ac], XmNtransientFor, mainw); ac++;
  if (modal)
  	XtSetArg (av[ac], XmNdialogStyle, XmDIALOG_PRIMARY_APPLICATION_MODAL); ac++;
  XtSetArg (av[ac], XmNdialogType, XmDIALOG_QUESTION); ac++;
  XtSetArg (av[ac], XmNdeleteResponse, XmDESTROY); ac++;
  XtSetArg (av[ac], XmNautoUnmanage, False); ac++;
  XtSetArg (av[ac], XmNnoResize, True); ac++;

  dialog = XmCreatePromptDialog (mainw, name, av, ac);

  if (!separator)
	  fe_UnmanageChild_safe(XmSelectionBoxGetChild(dialog,
												   XmDIALOG_SEPARATOR));

  fe_UnmanageChild_safe (XmSelectionBoxGetChild (dialog, XmDIALOG_TEXT));
  fe_UnmanageChild_safe (XmSelectionBoxGetChild (dialog,
                                                XmDIALOG_SELECTION_LABEL));
  if (!ok)
      fe_UnmanageChild_safe (XmSelectionBoxGetChild (dialog, XmDIALOG_OK_BUTTON));
  if (!cancel)
      fe_UnmanageChild_safe (XmSelectionBoxGetChild (dialog,
						     XmDIALOG_CANCEL_BUTTON));
#ifdef NO_HELP
  fe_UnmanageChild_safe (XmSelectionBoxGetChild (dialog, XmDIALOG_HELP_BUTTON));
#endif

  if (apply)
      XtManageChild (XmSelectionBoxGetChild (dialog, XmDIALOG_APPLY_BUTTON));

  return dialog;
}

static int
fe_get_numeric_text_field(Widget widget)
{
	char* p = XmTextFieldGetString(widget);
	char* q;
	if (p) {
		while (isspace(*p))
			p++;
		if (*p == '\0') return -1;
		for (q = p; *q != '\0'; q++) {
			if (!(isdigit(*q) || isspace(*q)))
				return -1; /* force error */
		}
		return atoi(p);
	} else {
		return -1;
	}
}

static void
fe_editor_hrule_properties_common(MWContext* context,
								  EDT_HorizRuleData* data,
							  fe_HorizontalRulePropertiesDialogData* w_data)
{
    Widget widget;
    
    /* height */
    data->size = fe_get_numeric_text_field(w_data->height);

    /* width */
    XtVaGetValues(w_data->width_menu, XmNmenuHistory, &widget, 0);

    if (widget == w_data->width_pixels)
		data->bWidthPercent = FALSE;
    else
		data->bWidthPercent = TRUE;
    data->iWidth = fe_get_numeric_text_field(w_data->width);

    /* align */
    if (XmToggleButtonGadgetGetState(w_data->align_right) == TRUE)
		data->align = ED_ALIGN_RIGHT;
    else  if (XmToggleButtonGadgetGetState(w_data->align_left) == TRUE)
		data->align = ED_ALIGN_LEFT;
    else
		data->align = ED_ALIGN_CENTER;

    /* shading */
    if (XmToggleButtonGadgetGetState(w_data->three_d_shading) == TRUE)
		data->bNoShade = FALSE;
    else
		data->bNoShade = TRUE;

    data->pExtra = 0;
}

static Boolean
fe_editor_hrule_properties_validate(MWContext* context,
							  fe_HorizontalRulePropertiesDialogData* w_data)
{
	EDT_HorizRuleData  data;
	EDT_HorizRuleData* h = &data;
	unsigned       errors[16];
	unsigned       nerrors = 0;

	fe_editor_hrule_properties_common(context, &data, w_data);
	
	if (RANGE_CHECK(h->size, HRULE_MIN_HEIGHT, HRULE_MAX_HEIGHT))
		errors[nerrors++] = XFE_INVALID_HRULE_HEIGHT;

	if (h->bWidthPercent == TRUE) {
		if (RANGE_CHECK(h->iWidth,
						HRULE_MIN_PERCENT_WIDTH, HRULE_MAX_PERCENT_WIDTH))
			errors[nerrors++] = XFE_INVALID_HRULE_WIDTH;
	} else {
		if (RANGE_CHECK(h->iWidth,
						HRULE_MIN_PIXEL_WIDTH, HRULE_MAX_PIXEL_WIDTH))
			errors[nerrors++] = XFE_INVALID_HRULE_WIDTH;
	}

	if (nerrors > 0) {
		fe_editor_range_error_dialog(context, w_data->width,
									 errors, nerrors);
		return FALSE;
	}

	return TRUE;
}

static void
fe_HorizontalRulePropertiesDialogSet(MWContext* context,
								 fe_HorizontalRulePropertiesDialogData* w_data)
{
    EDT_HorizRuleData e_data;

	EDT_BeginBatchChanges(context);
	fe_editor_hrule_properties_common(context, &e_data, w_data);

    e_data.pExtra = 0;
    
    fe_EditorHorizontalRulePropertiesSet(context, &e_data);
	EDT_EndBatchChanges(context);
}

static void
fe_table_percent_label_set(Widget widget, Boolean nested);

static void
fe_HorizontalRulePropertiesDialogDataGet(
			    MWContext* context,
			    fe_HorizontalRulePropertiesDialogData* w_data
)
{
    EDT_HorizRuleData e_data;
    char buf[16];
    Boolean left;
    Boolean center;
    Boolean right;
    Boolean shading;
	Widget widget;
	Boolean is_nested;

    fe_EditorHorizontalRulePropertiesGet(context, &e_data);
    
    /* height */
    sprintf(buf, "%d", e_data.size);
    XmTextFieldSetString(w_data->height, buf);

    /* width */
    sprintf(buf, "%d", e_data.iWidth);
    XmTextFieldSetString(w_data->width, buf);

    if (e_data.bWidthPercent)
		widget = w_data->width_percent;
	else
		widget = w_data->width_pixels;

	is_nested = EDT_IsInsertPointInTable(context);
	fe_table_percent_label_set(w_data->width_percent, is_nested);

	XtVaSetValues(w_data->width_units, XmNmenuHistory, widget, 0);

    /* align */
    if (e_data.align == ED_ALIGN_RIGHT) {
		left = FALSE;
		center = FALSE;
		right = TRUE;
    } else if (e_data.align == ED_ALIGN_LEFT) {
		left = TRUE;
		center = FALSE;
		right = FALSE;
    } else {
		left = FALSE;
		center = TRUE;
		right = FALSE;
    }
    XmToggleButtonGadgetSetState(w_data->align_right, right, FALSE);
    XmToggleButtonGadgetSetState(w_data->align_left, left, FALSE);
    XmToggleButtonGadgetSetState(w_data->align_center, center, FALSE);

    if (e_data.bNoShade)
		shading = FALSE;
    else
		shading = TRUE;
    XmToggleButtonGadgetSetState(w_data->three_d_shading, shading, FALSE);
}

Widget
fe_EditorHorizontalRulePropertiesCreate(
			      MWContext* context,
			      fe_HorizontalRulePropertiesDialogData* data
			      )
{
  fe_DialogBuilderArg* code;
  Widget dialog;

  /*
   *     Make shell.
   */
  dialog = fe_CreatePromptDialog(context, "horizontalLineProperties",
			       TRUE, TRUE, FALSE, TRUE, TRUE);

  /*
   *   Make the rowcol, because we want to manage it.
   */
  code = fe_HorizontalRulePropertiesDialogProgram;

  db_do_work(context, dialog,
		      fe_DialogBuilderDefaultCreators,
		      &code,
		      data);

  return dialog;
}

static void
fe_hrule_destroy_cb (Widget widget, XtPointer closure, XtPointer call_data)
{
	int* done = (int*)closure;

	*done = XFE_DIALOG_DESTROY_BUTTON;
}

static void
fe_hrule_ok_cb (Widget widget, XtPointer closure, XtPointer call_data)
{
	int* done = (int*)closure;

	*done = XmDIALOG_OK_BUTTON;
}

static void
fe_hrule_apply_cb (Widget widget, XtPointer closure, XtPointer call_data)
{
	int* done = (int*)closure;

	*done = XmDIALOG_APPLY_BUTTON;
}

static void
fe_hrule_cancel_cb (Widget widget, XtPointer closure, XtPointer call_data)
{
	int* done = (int*)closure;

	*done = XmDIALOG_CANCEL_BUTTON;
}

void
fe_EditorHorizontalRulePropertiesDialogDo(MWContext* context)
{
    fe_HorizontalRulePropertiesDialogData widgets;
	int done;
    Widget dialog;

#if 0
    if (!fe_EditorHorizontalRulePropertiesCanDo(context)) {
		XBell(XtDisplay(CONTEXT_WIDGET(context)), 0);
		return;
	}
#endif

	dialog = fe_EditorHorizontalRulePropertiesCreate(context, &widgets);

	/*
	 *   Add a bunch of callbacks to the buttons.
	 */
	XtAddCallback(dialog, XmNokCallback, fe_hrule_ok_cb, &done);
	XtAddCallback(dialog, XmNcancelCallback, fe_hrule_cancel_cb, &done);
	XtAddCallback(dialog, XmNdestroyCallback, fe_hrule_destroy_cb, &done);

    /*
     *    Load values.
     */
    fe_HorizontalRulePropertiesDialogDataGet(context, &widgets);
    
    /*
     *    Popup.
     */
    XtManageChild(dialog);

    /*
     *    Wait.
     */
	fe_NukeBackingStore(dialog); /* what does this do? */
	done = XmDIALOG_NONE;
	while (done == XmDIALOG_NONE) {
		fe_EventLoop();

		/*
		 *    Unload data.
		 */
		if (done == XmDIALOG_OK_BUTTON
			&&
			!fe_editor_hrule_properties_validate(context, &widgets)) {
			done = XmDIALOG_NONE;
		}
	}

	if (done == XmDIALOG_OK_BUTTON)
		fe_HorizontalRulePropertiesDialogSet(context, &widgets);

	if (done != XFE_DIALOG_DESTROY_BUTTON)
		XtDestroyWidget(dialog);
}

static void
fe_color_picker_button_help_cb(Widget widget,
							   XtPointer closure, XtPointer call_data)
{
	MWContext* context = (MWContext*)closure;
	char* name = XtName(widget);

	XFE_Progress(context, name);
}

#define DEFAULT_NCOLUMNS     8 /* this is what windows has */
#define DEFAULT_NROWS        6
#define DEFAULT_NCUSTOM_ROWS 2
#define DEFAULT_BUTTONSIZE   22

static char*
fe_color_picker_standard_palette[(DEFAULT_NROWS * DEFAULT_NCOLUMNS) + 1] = {
    "white",
    "oldlace",
    "beige",
    "cornsilk",
    "blanchedalmond",
    "palegoldenrod",
    "yellow",
    "gold",
    "honeydew",
    "greenyellow",
    "lawngreen",
    "lime",
    "green",
    "seagreen",
    "darkkhaki",
    "olive",
    "olivedrab",
    "lightcyan",
    "turquoise",
    "aqua",
    "cyan",
    "mediumturquoise",
    "cornflowerblue",
    "teal",
    "darkcyan",
    "lightskyblue",
    "blue",
    "navy",
    "midnightblue",
    "thistle",
    "lightpink",
    "deeppink",
    "fuchsia",
    "magenta",
    "darkviolet",
    "purple",
    "navajowhite",
    "peru",
    "coral",
    "salmon",
    "sienna",
    "firebrick",
    "red",
    "crimson",
    "saddlebrown",
    "maroon",
    "gray",
    "black",
	NULL
};

static void
fe_make_buttons(MWContext* context, Widget parent,
				char** palette, Dimension size,
				XtCallbackRec* callback)
{
	unsigned n;
	char* name;
	uint8 red;
	uint8 green;
	uint8 blue;
	Pixel pixel;
	Arg args[16];
	Cardinal nchildren;
	Widget children[256];
	unsigned long color_word;
	Pixel top_shadow_pixel;
	Pixel bottom_shadow_pixel;

	XmGetColors( /* use standard borders to save colors */
				XtScreen(parent),
				fe_cmap(context),
				CONTEXT_DATA(context)->default_bg_pixel,
				NULL,
				&top_shadow_pixel,
				&bottom_shadow_pixel,
				NULL);

	for (nchildren = 0; (name = palette[nchildren]); nchildren++) {

		if (!LO_ParseRGB(name, &red, &green, &blue)) {
			red = blue = green = 0; /* black */
			name = "loser";
		}

		/*
		 *    Make a pixel. I'm hoping Jamie's color stuff will
		 *    know when to release this...djw
		 */
		pixel = fe_GetPixel(context, red, green, blue);

		/*
		 *    Save the color in userdata.
		 */
		color_word = (red << 16) + (green << 8) + blue;

		/*
		 *    Make a button.
		 */
		n = 0;
		XtSetArg(args[n], XmNuserData, (XtPointer)color_word); n++;
		XtSetArg(args[n], XmNwidth, size); n++;
		XtSetArg(args[n], XmNheight, size); n++;
		XtSetArg(args[n], XmNmarginWidth, 0); n++;
		XtSetArg(args[n], XmNmarginHeight, 0); n++;
		XtSetArg(args[n], XmNbackground, pixel); n++;
		XtSetArg(args[n], XmNtopShadowColor, top_shadow_pixel); n++;
		XtSetArg(args[n], XmNbottomShadowColor, bottom_shadow_pixel); n++;
		XtSetArg(args[n], XmNpushButtonEnabled, TRUE); n++;
		children[nchildren] = XmCreateDrawnButton(parent, name, args, n);

		if (callback) {
			XtAddCallback(children[nchildren], XmNactivateCallback,
						  callback->callback, callback->closure);
		}
		XtAddCallback(children[nchildren], XmNarmCallback,
					  fe_color_picker_button_help_cb, (XtPointer)context);
	}
	XtManageChildren(children, nchildren);
}

static void
fe_destroy_cleanup_cb(Widget widget, XtPointer closure, XtPointer cb)
{
    if (closure)
        XtFree(closure);
}

typedef struct
{
	int       reason;
	XEvent*   event;
	int       click_count;
	LO_Color* color;
	Boolean   is_custom;
	char*     name;
	Pixel     pixel;       /* not sure this is a good idea */
} fe_ColorPickerButtonCallbackStruct;

char fe_ColorPickerActivateButtonName[] = "colorButtonActivateCallback";
char fe_ColorPickerButtonSize[] =         "colorButtonSize";
char fe_ColorPickerPalette[] =            "colorPalette";

#define XFE_NcolorPickerButtonActivateCallback fe_ColorPickerActivateButtonName

#define BUTTON_PARENT_NAME "standard"

#include <ntypes.h> /* for LO_Color_struct */

static struct LO_Color_struct fe_color_picker_fake_cb_lo_color;

static void
fe_color_picker_fake_cb(Widget widget, XtPointer closure, XtPointer foo_data)
{
	fe_ColorPickerButtonCallbackStruct my_data;
	XmDrawnButtonCallbackStruct* db_data = 
		(XmDrawnButtonCallbackStruct*)foo_data;
	XtCallbackRec*  real_cb = (XtCallbackRec*)closure;
	unsigned long   color_word = (unsigned long)fe_GetUserData(widget);
	Pixel           bg;

	fe_color_picker_fake_cb_lo_color.red = (color_word >> 16) & 0xff;
	fe_color_picker_fake_cb_lo_color.green = (color_word >> 8) & 0xff;
	fe_color_picker_fake_cb_lo_color.blue = color_word & 0xff;

	XtVaGetValues(widget, XmNbackground, &bg, 0);

	my_data.reason = db_data->reason;
	my_data.event = db_data->event;
	my_data.click_count = db_data->click_count;
	my_data.color = &fe_color_picker_fake_cb_lo_color;

	if (strcmp(XtName(XtParent(widget)), BUTTON_PARENT_NAME) == 0)
		my_data.is_custom = FALSE;
	else
		my_data.is_custom = TRUE;

	my_data.name = XtName(widget);
	my_data.pixel = bg;

	(*real_cb->callback)(widget, real_cb->closure, &my_data);
}

Widget
fe_CreateColorPickerDialog(Widget parent, char* name, Arg* p_args, Cardinal p_n)
{
	MWContext* context = (MWContext *)fe_WidgetToMWContext(parent);
	Widget dialog;
	Widget main_rc;
	Widget standard_frame;
	Widget standard_rc;
#if 0
	Widget custom_frame;
	Widget custom_group_rc;
	Widget custom_rc;
	Widget custom_add;
#endif
	Arg args[16];
	Cardinal n;
	unsigned nrows = DEFAULT_NROWS;
	Dimension button_size = DEFAULT_BUTTONSIZE;
	char** palette = fe_color_picker_standard_palette;
	XtCallbackRec* callback = NULL;
	XtCallbackRec* callers_callback;
	XtCallbackRec  my_callback;

	dialog = fe_CreatePromptDialog(context, name, TRUE, TRUE, FALSE, TRUE, TRUE);

	/*
	 *    Suck out args, for now we ignore most stuff.
	 */
#define STREQ(a,b) ((a)==(b)||strcmp((a),(b))==0)
	for (n = 0; n < p_n; n++) {
		if (STREQ(p_args[n].name, fe_ColorPickerActivateButtonName))
			callback = (XtCallbackRec*)p_args[n].value;
		else if (STREQ(p_args[n].name, fe_ColorPickerButtonSize))
			button_size = (Dimension)p_args[n].value;
#if 0
		else if (STREQ(p_args[n].name, fe_ColorPickerPalette))
			palette = (char**)p_args[n].value;
#endif
	}
#undef STREQ

	if (callback) {
		/* dup callers, as Xt will be hanging onto our fake callback */
		callers_callback = XtNew(XtCallbackRec);
		memcpy(callers_callback, callback, sizeof(XtCallbackRec));

		XtAddCallback(dialog, XmNdestroyCallback, /* Victor */
					  fe_destroy_cleanup_cb,(XtPointer)callers_callback);

		my_callback.callback = fe_color_picker_fake_cb;
		my_callback.closure = callers_callback;

		callback = &my_callback;
	}

	n = 0;
	XtSetArg(args[n], XmNorientation, XmVERTICAL); n++;
	main_rc = XmCreateRowColumn(dialog, "colorPicker", args, n);
	
	/*
	 *    Standard colors: frame with N color buttons.
	 */
	n = 0;
	standard_frame = fe_CreateFrame(main_rc, "standardColors", args, n);
	XtManageChild(standard_frame);

	n = 0;
	XtSetArg(args[n], XmNorientation, XmHORIZONTAL); n++;
	XtSetArg(args[n], XmNnumColumns, nrows); n++;
	XtSetArg(args[n], XmNpacking, XmPACK_COLUMN); n++;
	standard_rc = XmCreateRowColumn(standard_frame, BUTTON_PARENT_NAME, args, n);
	XtManageChild(standard_rc);

	/*
	 *    Make pretty buttons.
	 */
	fe_make_buttons(context, standard_rc, palette, button_size, callback);
	
	/*
	 *    Custom colors: frame with N color buttons and "Add custom color" PB.
	 */
	/*FIXME*/ /* not yet */

	/*
	 *    Return.
	 */
	XtManageChild(main_rc);
	return dialog;
}

#if 0 /* in xfe.h */
/*
 *   Dependency mechanism package.
 */
typedef unsigned fe_Dependency;

#define FE_MAKE_DEPENDENCY(x) ((fe_Dependency)(x))

typedef struct fe_DependentList {
	struct fe_DependentList* next;

	Widget        widget;   /* that's me */
	XtCallbackRec callback;
	fe_Dependency mask;     /* call me back if match this mask */
} fe_DependentList;
#endif

void
fe_DependentListAddDependent(fe_DependentList** list_a,
					  Widget widget, fe_Dependency mask,
					  XtCallbackProc proc, XtPointer closure)
{
	fe_DependentList* list = XtNew(fe_DependentList);

	list->next = *list_a;
	list->widget = widget;
	list->mask = mask;
	list->callback.callback = proc;
	list->callback.closure = closure;

	*list_a = list;
}
		
void
fe_DependentListDestroy(fe_DependentList* list)
{
	fe_DependentList* next;

    for (; list; list = next) {
		next = list->next;
		XtFree((XtPointer)list);
	}
}

typedef struct fe_DependentListCallbackStruct
{
    int     reason;
    XEvent  *event;
	fe_Dependency mask;
	Widget  caller;
	XtPointer callers_data;
} fe_DependentListCallbackStruct;

void
fe_DependentListCallDependents(Widget caller,
							   fe_DependentList* list,
							   fe_Dependency mask,
							   XtPointer callers_data)
{
	fe_DependentListCallbackStruct call_data;

	call_data.reason = 0;
	call_data.event = 0;
	call_data.mask = mask;
	call_data.caller = caller;
	call_data.callers_data = callers_data;

    for (; list; list = list->next) {
		if ((list->mask & mask) != 0)
			(*list->callback.callback)(list->widget, list->callback.closure,
									   &call_data);
	}
}

void
fe_MakeDependent(Widget widget, fe_Dependency mask, 
				 XtCallbackProc proc, XtPointer closure)
{
	MWContext* context = (MWContext *)fe_WidgetToMWContext(widget);

	fe_DependentListAddDependent(
								 &(CONTEXT_DATA(context)->dependents),
								 widget,
								 mask,
								 proc,
								 closure);
}
	
void
fe_CallDependents(MWContext* context, fe_Dependency mask)
{
	fe_DependentListCallDependents(NULL, CONTEXT_DATA(context)->dependents,
								   mask, NULL);
}

struct fe_EditorParagraphPropertiesWidgets;
struct fe_EditorCharacterPropertiesWidgets;
struct fe_EditorLinkPropertiesWidgets;
struct fe_EditorImagePropertiesWidgets;

typedef struct fe_EditorPropertiesWidgets {
	MWContext*                                  context;
	fe_DependentList*                           dependents;
	struct fe_EditorCharacterPropertiesWidgets* character;
	struct fe_EditorLinkPropertiesWidgets*      link;
	struct fe_EditorParagraphPropertiesWidgets* paragraph;
	struct fe_EditorImagePropertiesWidgets*     image;
	fe_Dependency                               changed;
} fe_EditorPropertiesWidgets;

typedef struct fe_EditorParagraphPropertiesWidgets {
	fe_EditorPropertiesWidgets* properties;
	Widget style_menu;
	Widget additional_menu;
	Widget list_style_menu;
	Widget bullet_style_menu;
	Widget start_text;

	Widget  numbering_pulldown;
	Widget  bullet_pulldown;
	TagType paragraph_style;
	TagType additional_style;
	TagType list_style;
	ED_ListType bullet_style;
	ED_Alignment align;
	unsigned start;
} fe_EditorParagraphPropertiesWidgets;

typedef enum fe_JavaScriptModes
{
	JAVASCRIPT_NONE = 0,
	JAVASCRIPT_SERVER,
	JAVASCRIPT_CLIENT
} fe_JavaScriptModes;

typedef struct fe_EditorCharacterPropertiesWidgets {
	fe_EditorPropertiesWidgets* properties;
	Widget form;
	Widget color_default;
	Widget color_custom;
	Widget color_swatch;
	/* something else to hold custom color I guess */

	Widget bold;
	Widget italic;
	Widget fixed;
	Widget strike_thru;
	Widget super_script;
	Widget sub_script;
	Widget blink;
	
	ED_FontSize font_size;
	ED_TextFormat text_attributes;
	ED_TextFormat changed_mask;
	LO_Color      color;
	Boolean is_custom_color;
	fe_JavaScriptModes js_mode;
} fe_EditorCharacterPropertiesWidgets;

typedef char* EDT_LtabbList_t;

typedef struct fe_EditorLinkPropertiesWidgets {
	fe_EditorPropertiesWidgets* properties;
	Widget form;
	Widget displayed_label;
	Widget displayed_text;
	Widget link_text;
	/* others I don't understand */
	Widget target_list;
	EDT_LtabbList_t target_list_data;
	Widget target_current_doc;
	Widget target_selected_file;
	Widget target_label;
	char* selected_filename;
	char* url;
	char* text;
} fe_EditorLinkPropertiesWidgets;

typedef struct fe_EditorImagePropertiesWidgets {
	fe_EditorPropertiesWidgets* properties;
	EDT_ImageData               image_data;
	Widget main_image;
	Widget alt_image;
	Widget alt_text;
	Widget image_height;
	Widget image_width;
	Widget margin_height;
	Widget margin_width;
	Widget margin_solid;
	Boolean  do_copy;        /* means copy the file to the local dir */
	Boolean  existing_image; /* means there was an image when we loaded */
	Boolean  new_image;      /* means a new image gets loaded */
} fe_EditorImagePropertiesWidgets;

#define PROP_CHAR_BOLD    (0x1<<0)
#define PROP_CHAR_ITALIC  (0x1<<1)
#define PROP_CHAR_FIXED   (0x1<<2)
#define PROP_CHAR_SUPER   (0x1<<3)
#define PROP_CHAR_SUB     (0x1<<4)
#define PROP_CHAR_STRIKE  (0x1<<5)
#define PROP_CHAR_BLINK   (0x1<<6)
#define PROP_CHAR_COLOR   (0x1<<7)
#define PROP_CHAR_SIZE    (0x1<<8)

#define PROP_CHAR_SERVER    (0x1<<10)
#define PROP_CHAR_CLIENT    (0x1<<11)
#define PROP_CHAR_UNDERLINE (0x1<<12)

#define PROP_PARA_STYLE   (0x1<<13)
#define PROP_PARA_LIST    (0x1<<14)
#define PROP_PARA_BULLET  (0x1<<15)
#define PROP_PARA_ALIGN   (0x1<<16)
#define PROP_LINK_TEXT    (0x1<<17)
#define PROP_LINK_HREF    (0x1<<18)
#define PROP_IMAGE_MAIN_IMAGE (0x1<<19)
#define PROP_IMAGE_ALT_IMAGE  (0x1<<20)
#define PROP_IMAGE_ALT_TEXT   (0x1<<21)
#define PROP_IMAGE_ALIGN      (0x1<<22)
#define PROP_IMAGE_HEIGHT     (0x1<<23)
#define PROP_IMAGE_WIDTH      (0x1<<24)
#define PROP_IMAGE_MARGIN_HEIGHT (0x1<<25)
#define PROP_IMAGE_MARGIN_WIDTH  (0x1<<26)
#define PROP_IMAGE_MARGIN_BORDER (0x1<<27)
#define PROP_IMAGE_COPY       (0x1<<28)
#define PROP_IMAGE_IMAP       (0x1<<29)
#define PROP_LINK_LIST    (0x1<<30)

#define PROP_CHAR_STYLE   \
(PROP_CHAR_BOLD|PROP_CHAR_ITALIC|PROP_CHAR_UNDERLINE|PROP_CHAR_FIXED| \
 PROP_CHAR_STRIKE|PROP_CHAR_SUPER|PROP_CHAR_SUB|PROP_CHAR_BLINK)
#define PROP_CHAR_JAVASCRIPT (PROP_CHAR_CLIENT|PROP_CHAR_SERVER)
#define PROP_CHAR_ALL     \
(PROP_CHAR_STYLE|PROP_CHAR_COLOR|PROP_CHAR_SIZE|PROP_CHAR_JAVASCRIPT)
#define PROP_PARA_ALL     \
(PROP_PARA_STYLE|PROP_PARA_LIST|PROP_PARA_BULLET|PROP_PARA_ALIGN)
#define PROP_LINK_ALL     \
(PROP_LINK_TEXT|PROP_LINK_HREF|PROP_LINK_LIST)
#define PROP_IMAGE_DIMENSIONS (PROP_IMAGE_HEIGHT|PROP_IMAGE_WIDTH)
#define PROP_IMAGE_SPACE      \
(PROP_IMAGE_MARGIN_HEIGHT|PROP_IMAGE_MARGIN_WIDTH|PROP_IMAGE_MARGIN_BORDER)
#define PROP_IMAGE_ALL    \
(PROP_IMAGE_MAIN_IMAGE|PROP_IMAGE_ALT_IMAGE|PROP_IMAGE_ALT_TEXT|  \
 PROP_IMAGE_ALIGN|PROP_IMAGE_DIMENSIONS|PROP_IMAGE_SPACE|         \
 PROP_IMAGE_COPY|PROP_IMAGE_IMAP)

static void
fe_update_dependents(Widget caller,
					 fe_EditorPropertiesWidgets* p_data, fe_Dependency mask)
{
	p_data->changed |= mask;
	fe_DependentListCallDependents(caller, p_data->dependents, mask, NULL);
}

static void
fe_register_dependent(fe_EditorPropertiesWidgets* p_data, Widget widget,
					  fe_Dependency mask, XtCallbackProc proc, XtPointer closure)
{
	fe_DependentListAddDependent(&p_data->dependents,
								 widget,
								 mask,
								 proc,
								 closure);
}

typedef struct fe_style_data {
	char* name;
	unsigned data;
} fe_style_data;

static struct fe_style_data fe_paragraph_style[] = {
    { "normal",          P_PARAGRAPH  },
    { "headingOne",      P_HEADER_1   },
    { "headingTwo",      P_HEADER_2   },
    { "headingThree",    P_HEADER_3   },
    { "headingFour",     P_HEADER_4   },
    { "headingFive",     P_HEADER_5   },
    { "headingSix",      P_HEADER_6   },
    { "address",         P_ADDRESS    },
    { "formatted",       P_PREFORMAT  },
    { "listItem",        P_LIST_ITEM  },
    { "descriptionItem", P_DESC_TITLE },
    { "descriptionText", P_DESC_TEXT  },
    { 0 }
};

static struct fe_style_data fe_additional_style[] = {
    { "default",         P_PARAGRAPH  },
    { "list",            P_LIST_ITEM  },
    { "blockQuote",      P_BLOCKQUOTE },
    { 0 }
};

static struct fe_style_data fe_list_style[] = {
    { "unnumbered",      P_UNUM_LIST  },
    { "numbered",        P_NUM_LIST   },
    { "directory",       P_DIRECTORY  },
    { "menu",            P_MENU       },
    { "description",     P_DESC_LIST  },
    { 0 }
};

static struct fe_style_data fe_bullet_style[] = {
    { "automatic",       ED_LIST_TYPE_DEFAULT },
    { "solidCircle",     ED_LIST_TYPE_DISC    },
    { "openSquare",      ED_LIST_TYPE_SQUARE  },
    { "openCircle",      ED_LIST_TYPE_CIRCLE  },
    { 0 }
};

static struct fe_style_data fe_numbering_style[] = {
    { "automatic",       ED_LIST_TYPE_DEFAULT        },
    { "digital",         ED_LIST_TYPE_DIGIT          },
    { "upperRoman",      ED_LIST_TYPE_BIG_ROMAN      },
    { "lowerRoman",      ED_LIST_TYPE_SMALL_ROMAN    },
    { "upperAlpha",      ED_LIST_TYPE_BIG_LETTERS    },
    { "lowerAlpha",      ED_LIST_TYPE_SMALL_LETTERS  },
    { 0 }
};

static struct fe_style_data fe_align_style[] = {
    { "left",       ED_ALIGN_LEFT   },
    { "center",     ED_ALIGN_ABSCENTER },
    { "right",      ED_ALIGN_RIGHT  },
    { 0 }
};

static unsigned
fe_convert_value_to_index(fe_style_data* style_data, unsigned value)
{
	unsigned i;

	for (i = 0; style_data[i].name; i++) {
		if (style_data[i].data == value)
			return i;
	}
	/* maybe should assert */
	return 0;
}

static void
fe_paragraph_style_menu_update_cb(Widget widget, XtPointer closure,
								  XtPointer call_data)
{
	fe_EditorParagraphPropertiesWidgets* w_data = (fe_EditorParagraphPropertiesWidgets*)closure;

	TagType  type = w_data->paragraph_style;
	unsigned index = fe_convert_value_to_index(fe_paragraph_style, type);

	fe_OptionMenuSetHistory(widget, index);
}

static void
fe_paragraph_style_cb(Widget widget, XtPointer closure, XtPointer call_data)
{
	fe_EditorParagraphPropertiesWidgets* w_data
		= (fe_EditorParagraphPropertiesWidgets*)closure;
	unsigned foo = (unsigned)fe_GetUserData(widget);
	TagType  type = (TagType)foo;

	w_data->paragraph_style = type;
	if (type == P_LIST_ITEM) {
		w_data->additional_style = P_LIST_ITEM;
	} else if (w_data->additional_style == P_LIST_ITEM) {
		w_data->additional_style = P_PARAGRAPH; /* default */
	}

	fe_update_dependents(widget, w_data->properties, PROP_PARA_STYLE|PROP_PARA_LIST);
}

static void
fe_additional_style_menu_update_cb(Widget widget, XtPointer closure,
								  XtPointer call_data)
{
	fe_EditorParagraphPropertiesWidgets* w_data = (fe_EditorParagraphPropertiesWidgets*)closure;

	TagType  type = w_data->additional_style;
	unsigned index = fe_convert_value_to_index(fe_additional_style, type);

	fe_OptionMenuSetHistory(widget, index);
}

static void
fe_additional_style_cb(Widget widget, XtPointer closure, XtPointer call_data)
{
	fe_EditorParagraphPropertiesWidgets* w_data
		= (fe_EditorParagraphPropertiesWidgets*)closure;
	unsigned foo = (unsigned)fe_GetUserData(widget);
	TagType  type = (TagType)foo;

	w_data->additional_style = type;
	if (type == P_LIST_ITEM)
		w_data->paragraph_style = P_LIST_ITEM;

	fe_update_dependents(widget, w_data->properties, PROP_PARA_STYLE|PROP_PARA_LIST);
}

static void
fe_list_style_menu_update_cb(Widget widget, XtPointer closure,
								  XtPointer call_data)
{
	fe_EditorParagraphPropertiesWidgets* w_data = (fe_EditorParagraphPropertiesWidgets*)closure;

	TagType  type = w_data->additional_style;
	Boolean  enabled = FALSE;
	unsigned index;

	if (type == P_LIST_ITEM) {
		type = w_data->list_style;
		index = fe_convert_value_to_index(fe_list_style, type);
		enabled = TRUE;

		fe_OptionMenuSetHistory(widget, index);
	}

	XtVaSetValues(XmOptionButtonGadget(widget), XmNsensitive, enabled, 0);
}

static void
fe_list_style_cb(Widget widget, XtPointer closure, XtPointer call_data)
{
	fe_EditorParagraphPropertiesWidgets* w_data
		= (fe_EditorParagraphPropertiesWidgets*)closure;
	unsigned foo = (unsigned)fe_GetUserData(widget);
	TagType  type = (TagType)foo;

	w_data->list_style = type;
	
	fe_update_dependents(widget, w_data->properties, PROP_PARA_LIST|PROP_PARA_BULLET);
}

static void
fe_bullet_style_menu_update_cb(Widget widget, XtPointer closure,
								  XtPointer call_data)
{
	fe_EditorParagraphPropertiesWidgets* w_data = (fe_EditorParagraphPropertiesWidgets*)closure;

	TagType  type = w_data->additional_style;
	Boolean  enabled = FALSE;
	unsigned index;
	Widget   cascade = XmOptionButtonGadget(widget);
	Widget   pulldown;

	if (type == P_LIST_ITEM) {
		type = w_data->list_style;
		if (type == P_NUM_LIST) {
			enabled = TRUE;
			index = fe_convert_value_to_index(fe_numbering_style,
											  w_data->bullet_style);
			pulldown = w_data->numbering_pulldown;
		} else if (type == P_UNUM_LIST) {
			enabled = TRUE;
			index = fe_convert_value_to_index(fe_bullet_style,
											  w_data->bullet_style);
			pulldown = w_data->bullet_pulldown;
		}

		if (enabled) {
			XtVaSetValues(cascade, XmNsubMenuId, pulldown, 0);
			fe_OptionMenuSetHistory(widget, index);
		}
	}

	XtVaSetValues(cascade, XmNsensitive, enabled, 0);
}

static void
fe_bullet_style_cb(Widget widget, XtPointer closure, XtPointer call_data)
{
	fe_EditorParagraphPropertiesWidgets* w_data = (fe_EditorParagraphPropertiesWidgets*)closure;

	ED_ListType	type = (ED_ListType)fe_GetUserData(widget);

	w_data->bullet_style = type;

	fe_update_dependents(widget, w_data->properties, PROP_PARA_BULLET);
}

static Widget
fe_create_style_pulldown(Widget parent, char* name, fe_style_data* style_data,
					 Arg* p_args, Cardinal p_n)
{
	Cardinal nchildren;
	Widget   children[16];
	Widget   pulldown;
	Arg      args[8];
	Cardinal n;
	Cardinal m;
	XtCallbackRec* button_callback_rec = 0;
	XtCallbackRec* update_callback_rec = 0;

	for (n = m = 0; n < p_n; n++) {
		if (p_args[n].name == XmNactivateCallback)
			button_callback_rec = (XtCallbackRec*)p_args[n].value;
		else if (p_args[n].name == XmNvalueChangedCallback)
			update_callback_rec = (XtCallbackRec*)p_args[n].value;
		else {
			XtSetArg(args[m], p_args[n].name, p_args[n].value); m++;
		}
	}

	n = m;
	pulldown = fe_CreatePulldownMenu(parent, name, args, n);

	for (nchildren = 0; style_data[nchildren].name; nchildren++) {

		n = 0;
		XtSetArg(args[n], XmNuserData, style_data[nchildren].data); n++;
		children[nchildren] = XmCreatePushButtonGadget(
													   pulldown,
													   style_data[nchildren].name,
													   args,
													   n
													   );
		if (button_callback_rec) {
			XtAddCallback(children[nchildren],
						  XmNactivateCallback,
						  button_callback_rec->callback,
						  (XtPointer)button_callback_rec->closure);
		}
	}
	XtManageChildren(children, nchildren);
	
	return pulldown;
}

static Widget
fe_create_style_menu(Widget parent, char* name, fe_style_data* style_data,
					 Arg* p_args, Cardinal p_n)
{
	Widget   pulldown;
	Widget   option;
	Arg      args[8];
	Arg      rc_args[8];
	Cardinal n;
	Cardinal my_n;
	Cardinal rc_n;

	for (my_n = rc_n = n = 0; n < p_n; n++) {
		if (
			p_args[n].name == XmNactivateCallback
			||
			p_args[n].name == XmNvalueChangedCallback
		) {
			XtSetArg(rc_args[rc_n], p_args[n].name, p_args[n].value); rc_n++;
		} else {
			XtSetArg(args[my_n], p_args[n].name, p_args[n].value); my_n++;
		}
	}

	pulldown = fe_create_style_pulldown(parent, name, style_data, rc_args, rc_n);

	XtSetArg(args[my_n], XmNsubMenuId, pulldown); my_n++;
	option = fe_CreateOptionMenu(parent,	name, args, my_n);
	fe_UnmanageChild_safe(XmOptionLabelGadget(option));

	return option;
}

static void
fe_paragraph_align_cb(Widget widget, XtPointer closure, XtPointer call_data)
{
	fe_EditorParagraphPropertiesWidgets* w_data =
		(fe_EditorParagraphPropertiesWidgets*)closure;
	XmToggleButtonCallbackStruct* info =
		(XmToggleButtonCallbackStruct*)call_data;

	/*
	 *    We get a callback on both the unset of the old toggle,
	 *    and the set of the new, so ignore the former, it's not
	 *    interesting.
	 */
	if (info->set) {

		w_data->align = (ED_Alignment)fe_GetUserData(widget);	

		fe_update_dependents(widget, w_data->properties, PROP_PARA_ALIGN);
	}
}

static void
fe_paragraph_align_update_cb(Widget widget,
							 XtPointer closure, XtPointer call_data)
{
	fe_EditorParagraphPropertiesWidgets* w_data =
		(fe_EditorParagraphPropertiesWidgets*)closure;
	ED_Alignment align = (ED_Alignment)fe_GetUserData(widget);
	ED_Alignment w_align = w_data->align;

	if (w_align == ED_ALIGN_DEFAULT)
	    w_align = ED_ALIGN_LEFT;

	if (w_align == ED_ALIGN_CENTER) /* just in case the BE changes */
	    w_align = ED_ALIGN_ABSCENTER;

    XmToggleButtonGadgetSetState(widget, (w_align == align), FALSE);
}

static void
fe_set_text_field(Widget widget, char* value,
				  XtCallbackProc cb, XtPointer closure)
{
	if (cb)
		XtRemoveCallback(widget, XmNvalueChangedCallback,
						 cb, closure);

	if (!value)
		value = "";

	XmTextFieldSetString(widget, value);
	
	if (cb)
		XtAddCallback(widget, XmNvalueChangedCallback,
					  cb, closure);
}

static void
fe_set_numeric_text_field(Widget widget, unsigned value)
{
	char buf[32];

	sprintf(buf, "%d", value);

	fe_TextFieldSetString(widget, buf, FALSE);
}

static void
fe_paragraph_start_cb(Widget widget, XtPointer closure, XtPointer call_data)
{
	fe_EditorParagraphPropertiesWidgets* w_data =
		(fe_EditorParagraphPropertiesWidgets*)closure;

	fe_update_dependents(widget, w_data->properties, PROP_PARA_BULLET);
}

#if 0
static unsigned
fe_aztoui(char* s, Boolean upper)
{
    char* p;
	unsigned rv = 0;
	unsigned high;
	unsigned low;
	if (upper) {
	  low = 'A';
	  high = 'Z';
	} else {
	  low = 'a';
	  high = 'z';
	}
	for (p = s; *p >= low && *p <= high; p++) {
	    rv *= 26;
		rv += (*p - low) + 1;
	}
	return rv;
} 

static int
fe_uitoaz(char* buf, unsigned value, Boolean upper)
{
    char* p;
	unsigned base = 26;
	unsigned low;
	if (upper) {
	  low = 'A';
	} else {
	  low = 'a';
	}

	if (value == 0) {
		buf[0] = '\0';
		return 0;
	}
	value--;

	for (p = buf; base < value; p++)
	    base = base * 26;

	p[1] = '\0';


	for (; p >= buf; p--) {
	    *p = (value % 26) + low;
		value /= 26;
	}
	return strlen(buf);
}
#endif

static void
fe_paragraph_start_update_cb(Widget widget,
							 XtPointer closure, XtPointer call_data)
{
	fe_EditorParagraphPropertiesWidgets* w_data =
		(fe_EditorParagraphPropertiesWidgets*)closure;
	fe_DependentListCallbackStruct* info = 
		(fe_DependentListCallbackStruct*)call_data;
	char buf[256];
	Boolean enabled;

	if (info->caller == widget)
		return; /* don't call ourselves */

	if (w_data->paragraph_style == P_LIST_ITEM
		&&
		w_data->list_style==P_NUM_LIST) {
#if 0
		if (
			w_data->bullet_style == ED_LIST_TYPE_DIGIT
			||
			w_data->bullet_style == ED_LIST_TYPE_BIG_ROMAN
			||
			w_data->bullet_style == ED_LIST_TYPE_SMALL_ROMAN
		) {
			sprintf(buf, "%d", w_data->start);
		} else if (w_data->bullet_style == ED_LIST_TYPE_BIG_LETTERS) {
		    fe_uitoaz(buf, w_data->start, TRUE);
		} else  if (w_data->bullet_style == ED_LIST_TYPE_SMALL_LETTERS) {
		    fe_uitoaz(buf, w_data->start, FALSE);
		}
#else
		if (w_data->start < 1)
			w_data->start = 1;
		sprintf(buf, "%d", w_data->start);
#endif
		enabled = TRUE;
	} else {
		buf[0] = '\0';
		enabled = FALSE;
	}

	fe_TextFieldSetEditable(w_data->properties->context, widget, enabled);
	fe_set_text_field(widget, buf, fe_paragraph_start_cb, (XtPointer)w_data);
}

static void
fe_EditorParagraphPropertiesDialogDataGet(
								  MWContext* context,
								  fe_EditorParagraphPropertiesWidgets* w_data)
{
	EDT_ListData list_data;

	fe_EditorParagraphPropertiesGetAll(context,
									   &w_data->paragraph_style,
									   &list_data,
									   &w_data->align);
	
	switch (list_data.iTagType) {
	case P_BLOCKQUOTE: /* block quotes */
		w_data->additional_style = P_BLOCKQUOTE;
		w_data->list_style = P_UNUM_LIST;
		w_data->bullet_style = ED_LIST_TYPE_DEFAULT;
		break;

	case P_NUM_LIST: /* lists */
		w_data->start = list_data.iStart;
		if (w_data->start < 1)
			w_data->start = 1;
		/*FALLTHRU*/
	case P_UNUM_LIST:
	case P_DIRECTORY:
	case P_MENU:
	case P_DESC_LIST:
		w_data->additional_style = P_LIST_ITEM;
		w_data->list_style = list_data.iTagType;
		w_data->bullet_style = list_data.eType;
		break;

	default:
		w_data->additional_style = P_PARAGRAPH;
		w_data->list_style = P_UNUM_LIST;
		w_data->bullet_style = ED_LIST_TYPE_DEFAULT;
		break;
	}

	/*
	 *    Update the display.
	 */
	fe_update_dependents(NULL, w_data->properties, PROP_PARA_ALL);
}

static void
fe_EditorParagraphPropertiesDialogSet(
								 MWContext* context,
								 fe_EditorParagraphPropertiesWidgets* w_data)
{
	EDT_ListData list_data;
	char* p = NULL;

	memset(&list_data, 0, sizeof(EDT_ListData));

	if (w_data->paragraph_style == P_LIST_ITEM) { /* do list stuff */
		list_data.iTagType = w_data->list_style;
		list_data.bCompact = 0; /* ??? */
		list_data.eType = w_data->bullet_style;
		p = XmTextFieldGetString(w_data->start_text);
		list_data.iStart = atoi(p);
		if (p)
			XtFree(p);

		fe_EditorParagraphPropertiesSetAll(context,
										   w_data->paragraph_style,
										   &list_data,
										   w_data->align);
		
	} else if (w_data->additional_style == P_BLOCKQUOTE) { /* do quote */
		
		list_data.iTagType = P_BLOCKQUOTE;
		list_data.bCompact = 0; /* ??? */
		list_data.eType = ED_LIST_TYPE_DEFAULT;

		fe_EditorParagraphPropertiesSetAll(context,
										   w_data->paragraph_style,
										   &list_data,
										   w_data->align);
	} else {
		fe_EditorParagraphPropertiesSetAll(context,
										   w_data->paragraph_style,
										   NULL,
										   w_data->align);
	}
	

}

static void
fe_make_paragraph_page(
					   MWContext *context,
					   Widget parent,
					   fe_EditorParagraphPropertiesWidgets* w_data)
{
	Arg args[16];
	Cardinal n;
	Widget form;
	Widget paragraph_label;
	Widget paragraph_option;
	Widget additional_label;
	Widget additional_option;
	Widget list_frame;
	Widget list_form;
	Widget list_style_label;
	Widget list_style_option;
	Widget bullet_style_option;
	Widget starting_text;
	Widget starting_label;
	Widget align_frame;
	Widget align_rc;
	Widget children[16];
	Cardinal nchildren;
	XtCallbackRec callback;

#if 0
	n = 0;
	form = XmCreateForm(parent, "paragraphProperties", args, n);
	XtManageChild(form);
#else
	form = parent;
#endif

	/*
	 *    Paragraph Style.
	 */
	n = 0;
	XtSetArg(args[n], XmNtopAttachment, XmATTACH_FORM); n++;
	XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM); n++;
	paragraph_label = XmCreateLabelGadget(form, "styleLabel", args, n);
	XtManageChild(paragraph_label);

	n = 0;
	callback.callback = fe_paragraph_style_cb;
	callback.closure = (XtPointer)w_data;
	XtSetArg(args[n], XmNactivateCallback, &callback); n++;
	XtSetArg(args[n], XmNtopAttachment, XmATTACH_WIDGET); n++;
	XtSetArg(args[n], XmNtopWidget, paragraph_label); n++;
	XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM); n++;
	paragraph_option = fe_create_style_menu(form, "style", fe_paragraph_style,
											args, n);
	XtManageChild(paragraph_option);

	fe_register_dependent(w_data->properties, paragraph_option,
						  FE_MAKE_DEPENDENCY(PROP_PARA_STYLE|PROP_PARA_LIST),
						  fe_paragraph_style_menu_update_cb, (XtPointer)w_data);

	/*
	 *    Additional Style.
	 */
	n = 0;
	XtSetArg(args[n], XmNtopAttachment, XmATTACH_FORM); n++;
	XtSetArg(args[n], XmNleftAttachment, XmATTACH_WIDGET); n++;
	XtSetArg(args[n], XmNleftWidget, paragraph_option); n++;
	additional_label = XmCreateLabelGadget(form, "additionalLabel", args, n);
	XtManageChild(additional_label);

	n = 0;
	callback.callback = fe_additional_style_cb;
	callback.closure = (XtPointer)w_data;
	XtSetArg(args[n], XmNactivateCallback, &callback); n++;
	XtSetArg(args[n], XmNtopAttachment, XmATTACH_WIDGET); n++;
	XtSetArg(args[n], XmNtopWidget, paragraph_label); n++;
	XtSetArg(args[n], XmNleftAttachment, XmATTACH_WIDGET); n++;
	XtSetArg(args[n], XmNleftWidget, paragraph_option); n++;
	additional_option = fe_create_style_menu(form, "additional", fe_additional_style,
											args, n);
	XtManageChild(additional_option);

	fe_register_dependent(w_data->properties, additional_option,
						  FE_MAKE_DEPENDENCY(PROP_PARA_STYLE|PROP_PARA_LIST),
						  fe_additional_style_menu_update_cb, (XtPointer)w_data);
	/*
	 *    List frame and friends. THIS IS SO BORING!!!!!!!!!!!!!
	 */
	n = 0;
	XtSetArg(args[n], XmNtopAttachment, XmATTACH_WIDGET); n++;
	XtSetArg(args[n], XmNtopWidget, paragraph_option); n++;
	XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM); n++;
	list_frame = fe_CreateFrame(form, "list", args, n);
	XtManageChild(list_frame);
	
	n = 0;
	list_form = XmCreateForm(list_frame, "list", args, n);
	XtManageChild(list_form);

	n = 0;
	XtSetArg(args[n], XmNtopAttachment, XmATTACH_FORM); n++;
	XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM); n++;
	list_style_label = XmCreateLabelGadget(list_form, "listLabel", args, n);
	XtManageChild(list_style_label);

	n = 0;
	callback.callback = fe_list_style_cb;
	callback.closure = (XtPointer)w_data;
	XtSetArg(args[n], XmNactivateCallback, &callback); n++;
	XtSetArg(args[n], XmNtopAttachment, XmATTACH_WIDGET); n++;
	XtSetArg(args[n], XmNtopWidget, list_style_label); n++;
	XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM); n++;
	list_style_option = fe_create_style_menu(list_form, "listStyle", fe_list_style,
											args, n);
	XtManageChild(list_style_option);

	fe_register_dependent(w_data->properties, list_style_option,
						  FE_MAKE_DEPENDENCY(PROP_PARA_LIST),
						  fe_list_style_menu_update_cb, (XtPointer)w_data);

	n = 0;
	callback.callback = fe_bullet_style_cb;
	callback.closure = (XtPointer)w_data;
	XtSetArg(args[n], XmNactivateCallback, &callback); n++;
	w_data->numbering_pulldown = fe_create_style_pulldown(list_form,
														  "bulletStyle",
														  fe_numbering_style,
														  args, n);
	w_data->bullet_pulldown = fe_create_style_pulldown(list_form,
													   "bulletStyle",
													   fe_bullet_style,
													   args, n);

	n = 0;
	XtSetArg(args[n], XmNtopAttachment, XmATTACH_WIDGET); n++;
	XtSetArg(args[n], XmNtopWidget, list_style_label); n++;
	XtSetArg(args[n], XmNleftAttachment, XmATTACH_WIDGET); n++;
	XtSetArg(args[n], XmNleftWidget, list_style_option); n++;
	XtSetArg(args[n], XmNsubMenuId, w_data->bullet_pulldown); n++;
	bullet_style_option = fe_CreateOptionMenu(list_form, "bulletStyle", args, n);
	fe_UnmanageChild_safe(XmOptionLabelGadget(bullet_style_option));
	XtManageChild(bullet_style_option);

	fe_register_dependent(w_data->properties, bullet_style_option,
						  FE_MAKE_DEPENDENCY(PROP_PARA_BULLET|PROP_PARA_LIST),
						  fe_bullet_style_menu_update_cb, (XtPointer)w_data);

	n = 0;
	XtSetArg(args[n], XmNtopAttachment, XmATTACH_WIDGET); n++;
	XtSetArg(args[n], XmNtopWidget, bullet_style_option); n++;
	XtSetArg(args[n], XmNrightAttachment, XmATTACH_OPPOSITE_WIDGET); n++;
	XtSetArg(args[n], XmNrightWidget, bullet_style_option); n++;
	w_data->start_text = starting_text = 
		XmCreateTextField(list_form, "startText", args, n);
	XtManageChild(starting_text);

	XtAddCallback(starting_text, XmNvalueChangedCallback,
				  fe_paragraph_start_cb, (XtPointer)w_data);
	fe_register_dependent(w_data->properties, starting_text,
						  FE_MAKE_DEPENDENCY(PROP_PARA_BULLET|PROP_PARA_LIST),
						  fe_paragraph_start_update_cb, (XtPointer)w_data);

	n = 0;
	XtSetArg(args[n], XmNtopAttachment, XmATTACH_WIDGET); n++;
	XtSetArg(args[n], XmNtopWidget, bullet_style_option); n++;
	XtSetArg(args[n], XmNrightAttachment, XmATTACH_WIDGET); n++;
	XtSetArg(args[n], XmNrightWidget, starting_text); n++;
	starting_label = XmCreateLabelGadget(list_form, "startLabel", args, n);
	XtManageChild(starting_label);

	/*
	 *    Align.
	 */
	n = 0;
	XtSetArg(args[n], XmNtopAttachment, XmATTACH_WIDGET); n++;
	XtSetArg(args[n], XmNtopWidget, list_frame); n++;
	XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM); n++;
	align_frame = fe_CreateFrame(form, "align", args, n);
	XtManageChild(align_frame);

	n = 0;
	XtSetArg(args[n], XmNorientation, XmHORIZONTAL); n++;
	XtSetArg(args[n], XmNradioBehavior, TRUE); n++;
	align_rc = XmCreateRowColumn(align_frame, "align", args, n);
	XtManageChild(align_rc);

	for (nchildren = 0; fe_align_style[nchildren].name; nchildren++) {
		n = 0;
		XtSetArg(args[n], XmNindicatorType, XmONE_OF_MANY); n++;
		XtSetArg(args[n], XmNuserData, fe_align_style[nchildren].data); n++;
		children[nchildren] = XmCreateToggleButtonGadget(
											 align_rc,
											 fe_align_style[nchildren].name,
											 args, n);

		XtAddCallback(children[nchildren], XmNvalueChangedCallback,
					  fe_paragraph_align_cb, (XtPointer)w_data);
		fe_register_dependent(w_data->properties,
							 children[nchildren], 
							 FE_MAKE_DEPENDENCY(PROP_PARA_ALIGN),
							 fe_paragraph_align_update_cb, (XtPointer)w_data);
	}

	XtManageChildren(children, nchildren);

#if 0
	/* Humour for Beta */
	/* Apparently our testers have no humour - oh well */
	n = 0;
	XtSetArg(args[n], XmNtopAttachment, XmATTACH_WIDGET); n++;
	XtSetArg(args[n], XmNtopWidget, align_frame); n++;
	XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM); n++;
	align_rc = XmCreateLabelGadget(form, "spaceAvailable", args, n);
	XtManageChild(align_rc);
#endif
}

static void
fe_link_text_changed_cb(Widget widget, XtPointer closure, XtPointer call_data)
{
	fe_EditorLinkPropertiesWidgets* w_data =
		(fe_EditorLinkPropertiesWidgets*)closure;
	
	fe_update_dependents(widget, w_data->properties, PROP_LINK_TEXT);
}

static char*
fe_image_get_name(fe_EditorImagePropertiesWidgets* w_data)
{
	char* p;

	if (w_data->main_image)
		p = XmTextFieldGetString(w_data->main_image);
	
	if (!p && w_data->alt_image)
		p = XmTextFieldGetString(w_data->alt_image);

	if (!p && w_data->alt_text)
		p = XmTextFieldGetString(w_data->alt_text);

	return p;
}

static void
fe_link_text_update_cb(Widget widget, XtPointer closure, XtPointer call_data)
{
	fe_EditorLinkPropertiesWidgets* w_data =
		(fe_EditorLinkPropertiesWidgets*)closure;
	fe_DependentListCallbackStruct* info = 
		(fe_DependentListCallbackStruct*)call_data;
	Boolean enabled = FALSE;
	char*   text;
	char* free_text = NULL;
	char* label_string;
	int   id;
	XmString xm_string;

	if (info->caller == widget)
		return; /* don't call ourselves */

	/*
	 *    If we have an image, use the image name as the current
	 *    displayed text.
	 */
	if (w_data->properties->image != NULL) {
		free_text = text = fe_image_get_name(w_data->properties->image);
		enabled = FALSE;
		id = XFE_EDITOR_LINK_TEXT_LABEL_IMAGE;
		/* Linked image: */
	} else if (w_data->text) {
		text = w_data->text;
		enabled = FALSE;
		id = XFE_EDITOR_LINK_TEXT_LABEL_TEXT;
		/* Linked text: */
	} else {
		text = "";
		enabled = TRUE;
		id = XFE_EDITOR_LINK_TEXT_LABEL_NEW;
		/* Enter the text of the link: */
	}

	fe_set_text_field(widget, text, fe_link_text_changed_cb,(XtPointer)w_data);
	fe_TextFieldSetEditable(w_data->properties->context, widget, enabled);
	
	/*
	 *    While we are here set the label also.
	 */
	label_string = XP_GetString(id);
	xm_string = XmStringCreateLocalized(label_string);
	XtVaSetValues(w_data->displayed_label, XmNlabelString, xm_string, 0);
	XmStringFree(xm_string);

	if (free_text)
		XtFree(free_text);
}

static void
fe_link_href_changed_cb(Widget widget, XtPointer closure, XtPointer call_data)
{
	
	fe_EditorLinkPropertiesWidgets* w_data =
		(fe_EditorLinkPropertiesWidgets*)closure;

	fe_update_dependents(widget, w_data->properties, PROP_LINK_HREF);
}

static void
fe_link_href_update_cb(Widget widget, XtPointer closure, XtPointer call_data)
{
	fe_EditorLinkPropertiesWidgets* w_data =
		(fe_EditorLinkPropertiesWidgets*)closure;
	fe_DependentListCallbackStruct* info = 
		(fe_DependentListCallbackStruct*)call_data;
	char*   text;

	if (info->caller == widget)
		return; /* don't call ourselves */

	/*
	 *    If we have an image, use the image name as the current
	 *    displayed text.
	 */
	if (w_data->url) {
		text = w_data->url;
	} else {
		text = "";
	}

	fe_set_text_field(widget, text, fe_link_href_changed_cb,(XtPointer)w_data);
	w_data->url = NULL;
}

static char*
fe_basename(char* target, char* source)
{
	char* p;
			
	p = strrchr(source, '/'); /* find last slash */

	if (p) { /* should be */
		strcpy(target, p+1);
	} else {
		strcpy(target, source);
	}
	return target;
}

static char*
fe_dirname(char* target, char* source)
{
	char* p;
			
	p = strrchr(source, '/'); /* find last slash */

	if (p) { /* should be */
		if (p == source) {
			strcpy(target, "/");
		} else {
			memcpy(target, source, p - source);
			target[p - source] = '\0';
		}
	} else {
		strcpy(target, ".");
	}
	return target;
}

static Boolean
fe_file_has_same_directory(MWContext* context, char* pathname)
{
	char my_dirname[MAXPATHLEN];
	char your_dirname[MAXPATHLEN];

	fe_EditorMakeName(context, your_dirname, sizeof(your_dirname));
	fe_dirname(my_dirname, your_dirname);
	fe_dirname(your_dirname, pathname);

	return (strcmp(my_dirname, your_dirname) == 0);
}

static void
fe_link_properties_list_update(fe_EditorLinkPropertiesWidgets*, Boolean);

static void
fe_editor_browse_file_of_text(MWContext* context, Widget text_field)
{
    char* before_save;
	char* before_changed = NULL;
	char* after;
	char* p;

	/*
	 *    Strip #target from the browse default filename.
	 */
	before_save = XmTextFieldGetString(text_field);

	/*
	 *    If the text field is a http: form URL, then zero it,
	 *    because we are not browsing for a file.
	 */
	if (before_save != NULL) {
		fe_StringTrim(before_save);

		if ((p = strchr(before_save, ':')) != NULL) {
			if (NET_IsLocalFileURL(before_save)) {
				before_changed = XtNewString(&p[1]);
			} else {
				before_changed = XtNewString("");
			}
		} else if ((p = strchr(before_save, '#')) != NULL) {
			*p = '\0';
			before_changed = XtNewString(before_save);
			*p = '#';
		}
		if (before_changed != NULL)
			fe_TextFieldSetString(text_field, before_changed, FALSE);
	}
	
	fe_browse_file_of_text(context, text_field, FALSE);

	after = XmTextFieldGetString(text_field);
	fe_StringTrim(after);

	/*
	 *    If we zapped it, and there was no change by the user,
	 *    put it back the way it was.
	 */
	if (before_changed != NULL && strcmp(after, before_changed) == 0) {
		fe_TextFieldSetString(text_field, before_save, FALSE);
	}
	/*
	 *    Or, if the main file, and the link file are in the same
	 *    directory, simplfy the name.
	 */
	else if (fe_file_has_same_directory(context, after)) {
		char  basename[MAXPATHLEN];
		fe_basename(basename, after);
		fe_TextFieldSetString(text_field, basename, FALSE);
	}

	if (before_save)
		XtFree(before_save);
	if (before_changed)
		XtFree(before_changed);
	if (after)
		XtFree(after);
}

static void
fe_link_href_browse_cb(Widget widget, XtPointer closure, XtPointer call_data)
{
	fe_EditorLinkPropertiesWidgets* w_data =
		(fe_EditorLinkPropertiesWidgets*)closure;
	MWContext* context = w_data->properties->context;
	Widget text_field = w_data->link_text;

	fe_editor_browse_file_of_text(context, text_field);

	fe_update_dependents(widget, w_data->properties, PROP_LINK_LIST);

	/*
	 *    Set the toggle to selected file, and update.
	 */
	XmToggleButtonGadgetSetState(w_data->target_current_doc, FALSE, FALSE);
	XmToggleButtonGadgetSetState(w_data->target_selected_file, TRUE, FALSE);

	fe_link_properties_list_update(w_data, FALSE);
}

static void
fe_browse_to_text_field_cb(Widget widget,
						   XtPointer closure, XtPointer call_data)
{
    MWContext* context = (MWContext*)closure;
	Widget     text_field = (Widget)fe_GetUserData(widget);

	fe_browse_file_of_text(context, text_field, FALSE);
}

static void
fe_link_tab_remove_link(fe_EditorLinkPropertiesWidgets* w_data)
{
	w_data->url = NULL;

	fe_update_dependents(NULL, w_data->properties, PROP_LINK_HREF);
}

static void
fe_link_href_remove_cb(Widget widget, XtPointer closure, XtPointer call_data)
{
	fe_EditorLinkPropertiesWidgets* w_data =
		(fe_EditorLinkPropertiesWidgets*)closure;

	fe_link_tab_remove_link(w_data);
}

static Boolean
fe_link_tab_has_link(fe_EditorLinkPropertiesWidgets* w_data)
{
	char*   link_text;
	Boolean rv = FALSE;

	if (w_data != NULL) {
		/* 
		 *    get the link text, to see if there any.
		 */
		link_text = XmTextFieldGetString(w_data->link_text);

		if (link_text != NULL) {
			if (link_text[0] != '\0') /* has a link */
				rv = TRUE;
			XtFree(link_text);
		}
	}
	
	return rv;
}

static void
fe_link_href_remove_update_cb(Widget widget,
							  XtPointer closure, XtPointer call_data)
{
	fe_EditorLinkPropertiesWidgets* w_data =
		(fe_EditorLinkPropertiesWidgets*)closure;
	Boolean sensitive = fe_link_tab_has_link(w_data);

	fe_WidgetSetSensitive(widget, sensitive);
}

#if 0
static void
fe_ListAddItems(Widget widget, char** items, unsigned nitems)
{
	unsigned i;
	XmString xm_string;

	XmListDeleteAllItems(widget);

	for (i = 0; i < nitems; i++) {
		xm_string = XmStringCreateLocalized(items[i]);
		XmListAddItem(widget, xm_string, i + 1); /* MOTIFism */
		XmStringFree(xm_string);
	}
}
#endif

static void
fe_ListSetFromLtabbList(Widget widget, EDT_LtabbList_t ltabb_list)
{
	char*    p;
	XmString xm_string;
	unsigned len;
	unsigned i;

	XmListDeleteAllItems(widget);

	if (ltabb_list) {
		for (p = ltabb_list, i = 0; (len = XP_STRLEN(p)) > 0; i++) {
			xm_string = XmStringCreateLocalized(p);
			XmListAddItem(widget, xm_string, i + 1); /* MOTIFism */
			XmStringFree(xm_string);
			p += len + 1;
		}
	}
}

static char*
fe_LtabbListGetItem(EDT_LtabbList_t ltabb_list, unsigned index)
{
	unsigned i;
	char* p;
	unsigned len;

	if (ltabb_list) {
		for (p = ltabb_list, i = 0; (len = XP_STRLEN(p)) > 0; i++) {
			if (i == index)
				return p;
			p += len + 1;
		}
	}
	return NULL;
}

static char*
fe_file_name_int_my_directory(MWContext* context,
							  char* my_dirname, char* basename)
{
	char your_dirname[MAXPATHLEN];

	if (basename[0] == '/' && basename[0] == '.')
		return basename;

	fe_EditorMakeName(context, your_dirname, sizeof(your_dirname));
	fe_dirname(my_dirname, your_dirname);
	strcat(my_dirname, "/");
	strcat(my_dirname, basename);

	return my_dirname;
}

static void
fe_link_properties_list_cb(Widget widget, XtPointer closure,
									XtPointer foo)
{
    fe_EditorLinkPropertiesWidgets* w_data = 
	  (fe_EditorLinkPropertiesWidgets*)closure;
	XmListCallbackStruct* cb_data = (XmListCallbackStruct*)foo;
	char  basename[MAXPATHLEN];
	char  buf[MAXPATHLEN];
	char* pathname;
	char* p;
	char* target_name;
	Boolean current_doc;

	current_doc = XmToggleButtonGadgetGetState(w_data->target_current_doc);
	
	if (cb_data->reason != XmCR_SINGLE_SELECT)
  	    return;

	if (cb_data->item_position > 0) {
		target_name = fe_LtabbListGetItem(w_data->target_list_data,
										  cb_data->item_position - 1);
	} else {
		target_name = NULL;
	}

	if (current_doc) {
		pathname = NULL;
	} else {
		pathname = XmTextFieldGetString(w_data->link_text);

		if ((p = strchr(pathname, '#')) != NULL) {
			memcpy(basename, pathname, p - pathname);
			basename[p - pathname] = '\0';
		} else {
			strcpy(basename, pathname);
		}

		XtFree(pathname);
		pathname = basename;

		if (pathname[0] == '\0')
			pathname = NULL;
	}
	
	if (pathname != NULL && target_name != NULL) {
		strcpy(buf, pathname);
		strcat(buf, "#");
		strcat(buf, target_name);
	} else if (target_name != NULL) {
		strcpy(buf, "#");
		strcat(buf, target_name);
	} else {
		buf[0] = '\0';
	}

	fe_TextFieldSetString(w_data->link_text, buf, FALSE);
	w_data->properties->changed |= PROP_LINK_HREF;
}

static void
fe_link_properties_list_update(fe_EditorLinkPropertiesWidgets* w_data,
								  Boolean current_file)
{
	MWContext* context = w_data->properties->context;
	char* link_text;
	EDT_LtabbList_t list_data = NULL;
	char* fuck;
	XmString xm_string;
	char buf[MAXPATHLEN];
	int id;

	if (w_data->target_list_data)
		XP_FREE(w_data->target_list_data);
	
	if (current_file) {
		list_data = EDT_GetAllDocumentTargets(context);
	} else {
		link_text = XmTextFieldGetString(w_data->link_text);
		fuck = fe_file_name_int_my_directory(context, buf, link_text);
		if (fuck && fuck[0] != '\0') {
			list_data =	EDT_GetAllDocumentTargetsInFile(context, fuck);
		}
		XtFree(link_text);
	}

	if (list_data && XP_STRLEN(list_data) == 0) {
		XP_FREE(list_data);
		list_data = NULL;
	}

	if (list_data && current_file)
		id = XFE_EDITOR_LINK_TARGET_LABEL_CURRENT;
     	/* Link to a named target in the current document (optional.) */
	else if (list_data && !current_file)
		id = XFE_EDITOR_LINK_TARGET_LABEL_SPECIFIED;
	    /* Link to a named target in the specified document (optional.) */
	else
		id = XFE_EDITOR_LINK_TARGET_LABEL_NO_TARGETS;
	    /* No targets in the selected document */

	fuck = XP_GetString(id);
	xm_string = XmStringCreateLocalized(fuck);
	XtVaSetValues(w_data->target_label, XmNlabelString, xm_string, 0);
	XmStringFree(xm_string);
	
	w_data->target_list_data = list_data;

	fe_ListSetFromLtabbList(w_data->target_list, w_data->target_list_data);
	XmToggleButtonGadgetSetState(w_data->target_current_doc,
								 current_file, FALSE);
	XmToggleButtonGadgetSetState(w_data->target_selected_file,
								 !current_file, FALSE);
}

static void
fe_link_properties_list_update_cb(Widget widget, XtPointer closure,
									XtPointer cb_data)
{
	fe_EditorLinkPropertiesWidgets* w_data = 
		(fe_EditorLinkPropertiesWidgets*)closure;
	Boolean set = XmToggleButtonGadgetGetState(w_data->target_current_doc);

	fe_link_properties_list_update(w_data, set);
}

static void
fe_link_properties_target_toggle_cb(Widget widget, XtPointer closure,
									XtPointer cb_data)
{
	fe_EditorLinkPropertiesWidgets* w_data = 
		(fe_EditorLinkPropertiesWidgets*)closure;
	Boolean current_file = (widget == w_data->target_current_doc);
	Boolean set = XmToggleButtonGadgetGetState(widget);
	
	fe_link_properties_list_update(w_data, (set == current_file));
}

/*
 *   Load the form context.
 */
static void
fe_editor_link_properties_dialog_data_init(
									 MWContext* context,
									 fe_EditorLinkPropertiesWidgets* w_data)
{
	char* text;
	char* href;
	char* p;
	Boolean text_exists;

	/*
	 *    If we are on something a link, we display the text that is
	 *    selected, or nothing. Text is read only. If we are not on
	 *    a link, text field is editable.
	 *
	 *    If we are on a link, we display that link, this field
	 *    is always editable.
	 */
	text_exists = fe_EditorHrefGet(context, &text, &href);

	/* if (!text_exists)	
		return;
	 */

	/*
	 *   Replace CR/LF with spaces to avoid ugly break in static display
	 */
	if (text) {
		for (p = text; *p; p++) {
			if (*p == '\n' || *p == '\r')
				*p = ' ';
		}
		w_data->text = text;
	} else {
		w_data->text = NULL;
	}

	if (href) {
		w_data->url = href;
		if (w_data->text == NULL)
			w_data->text = "";
	} else {
		w_data->url = NULL;
	}

	fe_update_dependents(NULL, w_data->properties, PROP_LINK_ALL);

	/*
	 *    Don't know what these are yet.
	 */
	w_data->selected_filename = NULL;
	fe_link_properties_list_update(w_data, TRUE);

	if (text)
		XtFree(text);
	if (href)
		XtFree(href);
}

static void
fe_editor_link_properties_dialog_set(
									 MWContext* context,
									 fe_EditorLinkPropertiesWidgets* w_data)
{
	char* displayed_text;
	char* link_text;

	displayed_text = XmTextFieldGetString(w_data->displayed_text);
	link_text = XmTextFieldGetString(w_data->link_text);
	
	/*
	 *    If the text field was editable, then we are creating a new
	 *    link. I don't really like carrying state around in the
	 *    values of the widgets, but this should work ok...djw.
	 */
	if (XmTextFieldGetEditable(w_data->displayed_text)) {
		if (link_text && (link_text[0] != '\0')) /* don't anything if no link! */
			fe_EditorHrefInsert(context, displayed_text, link_text);
	} else { /* modify exiting link/text */
		if (link_text && link_text[0] == '\0')
			fe_EditorHrefSetUrl(context, NULL);
		else
			fe_EditorHrefSetUrl(context, link_text);
	}
	
	if (displayed_text)
		XtFree(displayed_text);
	if (link_text)
		XtFree(link_text);
}

static void
fe_make_link_page(
				  MWContext* context,
				  Widget     parent,
				  fe_EditorLinkPropertiesWidgets* w_data
				  )
{
  Arg av [50];
  int ac;
  Widget form;
  Widget linkSourceLabel, sourceFrame, sourceForm;
  Widget linkToLabel, linkToFrame, linkToForm;
  Widget sourceTextLabel;
  Widget linkedTextLabel;
  Widget linkToTarget;
  Widget linkToBrowseFile;
  Widget linkToRemoveLink;
  Widget linkPageTextField;
  Widget sourceTextField;
  Widget target_list;
  Widget SelectedFileToggle;
  Widget currentDocToggle;
  Widget linkToShowTargets;
  Widget kids [100];
  Widget target_rc;
  Widget list_parent;
  Pixel  parent_bg;

  int i;

  XtVaGetValues(parent, XmNbackground, &parent_bg, 0);

#if 0
  ac = 0;
  XtSetArg (av [ac], XmNtopAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNbottomAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNleftAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNrightAttachment, XmATTACH_FORM); ac++;
  form = XmCreateForm(parent, "linkProperties", av, ac);
  XtManageChild (form);
#else
  form = parent;
#endif
  w_data->form = form;
    
  /**********************************************************************/
  /*  Create the Link source frame and form 28FEB96RCJ			*/
  /**********************************************************************/
  ac = 0;
  XtSetArg (av [ac], XmNleftAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNrightAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNtopAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNbottomAttachment, XmATTACH_NONE); ac++;
  sourceFrame = XmCreateFrame (form, "linkSourceFrame", av, ac);
  ac = 0;
  XtSetArg (av [ac], XmNleftAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNrightAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNtopAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNbottomAttachment, XmATTACH_NONE); ac++;
  sourceForm = XmCreateForm (sourceFrame, "linkSourceForm", av, ac);

  /**********************************************************************/
  /*  Create the Link To frame and form 28FEB96RCJ			*/
  /**********************************************************************/
  ac = 0;
  XtSetArg (av [ac], XmNleftAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNrightAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNtopAttachment, XmATTACH_WIDGET); ac++;
  XtSetArg (av [ac], XmNtopWidget, sourceFrame); ac++;
#if 0
  XtSetArg (av [ac], XmNbottomAttachment, XmATTACH_FORM); ac++;
#endif
  linkToFrame = XmCreateFrame (form, "linkToFrame", av, ac);

  ac = 0;
  XtSetArg (av [ac], XmNleftAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNrightAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNtopAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNbottomAttachment, XmATTACH_FORM); ac++;
  linkToForm = XmCreateForm (linkToFrame, "linkToForm", av, ac);

  /**********************************************************************/
  /*  Create the titles for the frames 28FEB96RCJ			*/
  /**********************************************************************/
  ac = 0;
  XtSetArg (av [ac], XmNchildType, XmFRAME_TITLE_CHILD); ac++;
  linkSourceLabel = XmCreateLabelGadget (sourceFrame, "linkSourceTitle", av, ac);
  linkToLabel = XmCreateLabelGadget (linkToFrame, "linkToTitle", av, ac);

  /**********************************************************************/
  /*  Create and manage the contents of the source form and frame	*/
  /**********************************************************************/
  ac = 0;
  i = 0;
  XtSetArg (av [ac], XmNtopAttachment,   XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNleftAttachment,   XmATTACH_FORM); ac++;
  kids [i++] = sourceTextLabel = XmCreateLabelGadget (sourceForm, 
                                                      "linkSourceLabel",
						      av, ac);
  w_data->displayed_label = sourceTextLabel;

  ac = 0;
  XtSetArg (av [ac], XmNtopAttachment,   XmATTACH_WIDGET); ac++;
  XtSetArg (av [ac], XmNtopWidget,       sourceTextLabel); ac++;
  XtSetArg (av [ac], XmNleftAttachment,  XmATTACH_FORM);   ac++;
  XtSetArg (av [ac], XmNrightAttachment, XmATTACH_FORM);   ac++;
  kids [i++] = sourceTextField = fe_CreateTextField (sourceForm, 
                                                     "linkSourceText", 
                                                     av, ac);
  w_data->displayed_text = sourceTextField;

  XtAddCallback(sourceTextField, XmNvalueChangedCallback,
				fe_link_text_changed_cb, (XtPointer)w_data);
  fe_register_dependent(w_data->properties, sourceTextField,
						FE_MAKE_DEPENDENCY(PROP_LINK_TEXT),
						fe_link_text_update_cb, (XtPointer)w_data);

  XtManageChildren (kids, i);
  XtManageChild (linkSourceLabel);
  XtManageChild (sourceForm);

  /**********************************************************************/
  /*  Create and manage the contents of the Link to form and frame	*/
  /**********************************************************************/
  ac = 0;
  i = 0;
  XtSetArg (av [ac], XmNleftAttachment,   XmATTACH_FORM);   ac++;
  XtSetArg (av [ac], XmNtopAttachment,    XmATTACH_FORM);   ac++;
  kids [i++] = linkedTextLabel = XmCreateLabelGadget (linkToForm, 
                                                    "linkToLabel",
						    av, ac);
  ac = 0;
  XtSetArg (av [ac], XmNleftAttachment,   XmATTACH_WIDGET); ac++;
  XtSetArg (av [ac], XmNleftWidget,       linkedTextLabel); ac++;
  XtSetArg (av [ac], XmNtopAttachment,    XmATTACH_FORM);   ac++;
  kids [i++] = linkToBrowseFile = XmCreatePushButtonGadget (linkToForm,
                                               "browseFile",
                                               av, ac);
  XtAddCallback(linkToBrowseFile, XmNactivateCallback,
				fe_link_href_browse_cb, (XtPointer)w_data);

  ac = 0;
  XtSetArg (av [ac], XmNleftAttachment,   XmATTACH_WIDGET);  ac++;
  XtSetArg (av [ac], XmNleftWidget,       linkToBrowseFile); ac++;
  XtSetArg (av [ac], XmNtopAttachment,    XmATTACH_FORM);    ac++;
  kids [i++] = linkToRemoveLink = XmCreatePushButtonGadget (linkToForm,
                                               "removeLink",
                                               av, ac);
  XtAddCallback(linkToRemoveLink, XmNactivateCallback,
				fe_link_href_remove_cb, (XtPointer)w_data);
  fe_register_dependent(w_data->properties, linkToRemoveLink,
						FE_MAKE_DEPENDENCY(PROP_LINK_HREF),
						fe_link_href_remove_update_cb, (XtPointer)w_data);
  
  ac = 0;
  XtSetArg (av [ac], XmNtopAttachment,   XmATTACH_WIDGET); ac++;
  XtSetArg (av [ac], XmNtopWidget,       linkedTextLabel); ac++;
  XtSetArg (av [ac], XmNtopOffset,       10);              ac++;
  XtSetArg (av [ac], XmNleftAttachment,  XmATTACH_FORM);   ac++;
  XtSetArg (av [ac], XmNrightAttachment, XmATTACH_FORM);   ac++;
  kids [i++] = linkPageTextField = fe_CreateTextField (linkToForm, 
                                                       "linkPageTextField", 
                                                       av, ac);
  w_data->link_text = linkPageTextField;
  XtAddCallback(linkPageTextField, XmNvalueChangedCallback,
				fe_link_href_changed_cb, (XtPointer)w_data);
  fe_register_dependent(w_data->properties, linkPageTextField,
						FE_MAKE_DEPENDENCY(PROP_LINK_HREF),
						fe_link_href_update_cb, (XtPointer)w_data);

  ac = 0;
  XtSetArg (av [ac], XmNtopAttachment,   XmATTACH_WIDGET);   ac++;
  XtSetArg (av [ac], XmNtopWidget,       linkPageTextField); ac++;
  XtSetArg (av [ac], XmNleftAttachment,  XmATTACH_FORM);     ac++;
  XtSetArg (av [ac], XmNalignment, XmALIGNMENT_BEGINNING);   ac++;
  kids [i++] = linkToTarget = XmCreateLabelGadget (linkToForm,
                                                   "linkTarget",
                                                   av, ac);
  w_data->target_label = linkToTarget;

  /*
   *    RowColumn to hold the buttons to right of target text.
   */
  ac = 0;
  XtSetArg (av [ac], XmNorientation, XmVERTICAL);   ac++;
  XtSetArg (av [ac], XmNtopAttachment,   XmATTACH_WIDGET);   ac++;
  XtSetArg (av [ac], XmNtopWidget,       linkToTarget);      ac++;
  XtSetArg (av [ac], XmNrightAttachment,  XmATTACH_FORM);     ac++;
  XtSetArg (av [ac], XmNpacking,  XmPACK_TIGHT);     ac++;
  kids[i++] = target_rc = XmCreateRowColumn(linkToForm,
											"targetRowColumn", av, ac);

  ac = 0;
  XtSetArg (av [ac], XmNtopAttachment,   XmATTACH_WIDGET);   ac++;
  XtSetArg (av [ac], XmNtopWidget,       linkToTarget);      ac++;
  XtSetArg (av [ac], XmNleftAttachment,  XmATTACH_FORM);     ac++;
  XtSetArg (av [ac], XmNrightAttachment,  XmATTACH_WIDGET);     ac++;
  XtSetArg (av [ac], XmNrightWidget,  target_rc);     ac++;
#if 0
  XtSetArg (av [ac], XmNbottomAttachment, XmATTACH_FORM);    ac++;
#endif
  XtSetArg(av[ac], XmNvisibleItemCount, 6); ac++;
  XtSetArg(av[ac], XmNselectionPolicy, XmSINGLE_SELECT); ac++;
  XtSetArg(av[ac], XmNbackground, parent_bg); ac++;
  target_list = XmCreateScrolledList(linkToForm, "targetList",
											 av, ac);
  XtManageChild(target_list);
  kids [i++] = list_parent = XtParent(target_list);

  XtAddCallback(target_list, XmNsingleSelectionCallback,
				fe_link_properties_list_cb, (XtPointer)w_data);
  fe_register_dependent(w_data->properties, target_list,
						FE_MAKE_DEPENDENCY(PROP_LINK_LIST),
						fe_link_properties_list_update_cb, (XtPointer)w_data);

  w_data->target_list = target_list;

  XtManageChildren (kids, i); /* children of form */
  XtManageChild (linkToLabel);
  XtManageChild (linkToForm);

  i = 0; /* no kids */
  ac = 0;
  XtSetArg(av[ac], XmNalignment, XmALIGNMENT_BEGINNING); ac++;
  kids [i++] = linkToShowTargets = XmCreateLabelGadget(target_rc,
													   "showTargets",
													   av, ac);

  ac = 0;
  XtSetArg (av [ac], XmNalignment, XmALIGNMENT_BEGINNING);     ac++;
  XtSetArg(av[ac], XmNset, TRUE); ac++;
  XtSetArg(av[ac], XmNindicatorType, XmONE_OF_MANY); ac++;
  kids [i++] = currentDocToggle =
    XmCreateToggleButtonGadget(target_rc, "currentDocument", av, ac);

  w_data->target_current_doc = currentDocToggle;

  ac = 0;
  XtSetArg (av [ac], XmNalignment, XmALIGNMENT_BEGINNING);     ac++;
  XtSetArg(av[ac], XmNset, FALSE); ac++;
  XtSetArg(av[ac], XmNindicatorType, XmONE_OF_MANY); ac++;
  kids [i++] = SelectedFileToggle =
    XmCreateToggleButtonGadget (target_rc, "selectedFile", av, ac);

  w_data->target_selected_file = SelectedFileToggle;
  XtManageChildren (kids, i); /* children of rc */

  XtAddCallback(currentDocToggle, XmNvalueChangedCallback,
			  fe_link_properties_target_toggle_cb, (XtPointer)w_data);
  XtAddCallback(SelectedFileToggle, XmNvalueChangedCallback,
			  fe_link_properties_target_toggle_cb, (XtPointer)w_data);

  /**********************************************************************/
  /*  Manage all of the frame comprising the Link dialog box		*/
  /**********************************************************************/

  i = 0;
  kids [i++] = sourceFrame;
  kids [i++] = linkToFrame;
  XtManageChildren (kids, i);

} /* end fe_make_link_page */

static struct fe_style_data fe_style_types[] = {
    { "bold",   TF_BOLD },
    { "italic", TF_ITALIC }, 
    { "underline", TF_UNDERLINE },
    { "fixed",  TF_FIXED },
    { "strikethrough", TF_STRIKEOUT },
    { "superscript",  TF_SUPER },
    { "subscript",    TF_SUB   },
    { "blink",  TF_BLINK },
	{ 0 }
};

static struct fe_style_data fe_font_size_types[] = {
    { "minusTwo",  ED_FONTSIZE_MINUS_TWO },
    { "minusOne",  ED_FONTSIZE_MINUS_ONE },
    { "plusZero",  ED_FONTSIZE_ZERO      },
    { "plusOne",   ED_FONTSIZE_PLUS_ONE  },
    { "plusTwo",   ED_FONTSIZE_PLUS_TWO  },
    { "plusThree", ED_FONTSIZE_PLUS_THREE },
    { "plusFour",  ED_FONTSIZE_PLUS_FOUR },
    { NULL }
};

static void
fe_font_size_update_cb(Widget widget, XtPointer closure, XtPointer call_data)
{
	fe_EditorCharacterPropertiesWidgets* w_data
		= (fe_EditorCharacterPropertiesWidgets*)closure;

	if (w_data->js_mode == JAVASCRIPT_NONE) {
		fe_OptionMenuSetHistory(widget, w_data->font_size - 1);
		fe_WidgetSetSensitive(XmOptionButtonGadget(widget), TRUE);
	} else {
		fe_OptionMenuSetHistory(widget, ED_FONTSIZE_ZERO - 1);
		fe_WidgetSetSensitive(XmOptionButtonGadget(widget), FALSE);
	}
	/*FIXME*/ /* fix sensitivity */
}

static void
fe_font_size_cb(Widget widget, XtPointer closure, XtPointer call_data)
{
	fe_EditorCharacterPropertiesWidgets* w_data = (fe_EditorCharacterPropertiesWidgets*)closure;

	ED_FontSize size = (ED_FontSize)fe_GetUserData(widget);

	w_data->font_size = size;
	w_data->changed_mask |= TF_FONT_SIZE;

	fe_update_dependents(widget, w_data->properties, PROP_CHAR_SIZE);
}

static void
fe_style_update_cb(Widget widget, XtPointer closure, XtPointer call_data)
{
	fe_EditorCharacterPropertiesWidgets* w_data
		= (fe_EditorCharacterPropertiesWidgets*)closure;
	ED_TextFormat type;
	Boolean set;
	Boolean enabled;
	unsigned foo;

	if (w_data->js_mode == JAVASCRIPT_NONE) {

		foo = (unsigned)fe_GetUserData(widget);
		type = foo;

		if ((type &	w_data->text_attributes) != 0)
			set = TRUE;
		else
			set = FALSE;

		enabled = TRUE;
	} else {
		set = FALSE;
		enabled = FALSE;
	}

	XmToggleButtonGadgetSetState(widget, set, FALSE);
	fe_WidgetSetSensitive(widget, enabled);
}
						 
static void
fe_style_cb(Widget widget, XtPointer closure, XtPointer call_data)
{
	fe_EditorCharacterPropertiesWidgets* w_data =
		(fe_EditorCharacterPropertiesWidgets*)closure;
	XmToggleButtonCallbackStruct* info =
		(XmToggleButtonCallbackStruct*)call_data;
	ED_TextFormat type;
	unsigned foo;

	foo = (unsigned)fe_GetUserData(widget);
	type = foo;
	
	if (info->set) {
		w_data->text_attributes |= type;

		/*
		 *    Super and Sub should be mutually exclusive.
		 */
		if (type == TF_SUPER)
			w_data->text_attributes &= ~TF_SUB;
		else if (type == TF_SUB)
			w_data->text_attributes &= ~TF_SUPER;
	} else {
		w_data->text_attributes &= ~type;
	}

	w_data->changed_mask |= type;

	fe_update_dependents(widget, w_data->properties, PROP_CHAR_STYLE);
}

static void
fe_clear_style_cb(Widget widget, XtPointer closure, XtPointer call_data)
{
	fe_EditorCharacterPropertiesWidgets*
		w_data = (fe_EditorCharacterPropertiesWidgets*)closure;

	w_data->text_attributes &= ~TF_ALL_MASK;
	w_data->changed_mask |= TF_ALL_MASK;

	fe_update_dependents(widget, w_data->properties, PROP_CHAR_STYLE);
}

static void
fe_clearall_settings_cb(Widget widget, XtPointer closure, XtPointer call_data)
{
	fe_EditorCharacterPropertiesWidgets* w_data
		= (fe_EditorCharacterPropertiesWidgets*)closure;
	MWContext* context;
	char* link_text;

	if (w_data->properties->link != NULL) {
	    /* 
		 *    get the link text, to see if there any.
		 */
	    link_text = XmTextFieldGetString(w_data->properties->link->link_text);

		if (link_text != NULL && link_text[0] != '\0') { /* has a link */
		    context = w_data->properties->context;
			/* Do you want to remove the link? */
		    if (XFE_Confirm(context,
							XP_GetString(XFE_EDITOR_WARNING_REMOVE_LINK)))
				fe_link_tab_remove_link(w_data->properties->link);
		}

		if (link_text != NULL)
		    XtFree(link_text);
	}

	w_data->js_mode = JAVASCRIPT_NONE;
	w_data->text_attributes &= ~TF_ALL_MASK;
	w_data->is_custom_color = FALSE;
	w_data->font_size = ED_FONTSIZE_ZERO;
	w_data->changed_mask |= TF_FONT_COLOR|TF_FONT_SIZE|TF_ALL_MASK|\
		                    PROP_CHAR_CLIENT|PROP_CHAR_SERVER;

	fe_update_dependents(widget, w_data->properties, PROP_CHAR_ALL);
}

typedef struct {
	LO_Color  color;
	Boolean   is_custom;
	Boolean   hit;
} fe_fe_EditorSetColorsInfo;

static void
fe_editor_set_colors_button_cb(Widget widget, XtPointer closure, XtPointer data)
{
	fe_ColorPickerButtonCallbackStruct* cp_data = 
		(fe_ColorPickerButtonCallbackStruct*)data;
	fe_fe_EditorSetColorsInfo* info = (fe_fe_EditorSetColorsInfo*)closure;

	info->color = *cp_data->color;
	info->is_custom = cp_data->is_custom;
	info->hit = TRUE;
}

static int
fe_color_do_work(MWContext* context, LO_Color* color_rv)
{
	Widget mainw = CONTEXT_WIDGET(context);
	Widget dialog;
	fe_fe_EditorSetColorsInfo info;
	XtCallbackRec callback;
	Arg args[8];
	Cardinal n;
	int done;

	LO_ParseRGB(fe_color_picker_standard_palette[0],
				&info.color.red, &info.color.green, &info.color.blue);
	info.hit = FALSE;
	callback.closure = &info;
	callback.callback = fe_editor_set_colors_button_cb;

	n = 0;
	XtSetArg(args[n], XFE_NcolorPickerButtonActivateCallback, &callback); n++;
	dialog = fe_CreateColorPickerDialog(mainw, "setColors", args, n);
	
	/*
	 *   Add a bunch of callbacks to the buttons.
	 */
	XtAddCallback(dialog, XmNokCallback, fe_hrule_ok_cb, &done);
	XtAddCallback(dialog, XmNcancelCallback, fe_hrule_cancel_cb, &done);
	XtAddCallback(dialog, XmNdestroyCallback, fe_hrule_destroy_cb, &done);

    /*
     *    Popup.
     */
    XtManageChild(dialog);

	/*
     *    Wait.
     */
	fe_NukeBackingStore(dialog); /* what does this do? */

	done = XmDIALOG_NONE;
	while (done == XmDIALOG_NONE) {
		fe_EventLoop();
	}

	if (done != XFE_DIALOG_DESTROY_BUTTON)
		XtDestroyWidget(dialog);

	if (done == XmDIALOG_OK_BUTTON) {
		*color_rv = info.color;
		return done;
	}
    return XmDIALOG_NONE;
}

static void
fe_color_swatch_update_cb(Widget widget, XtPointer closure,
						  XtPointer call_data)
{
	fe_EditorCharacterPropertiesWidgets* w_data
		= (fe_EditorCharacterPropertiesWidgets*)closure;
	LO_Color pooh;
	LO_Color* color;
	Boolean sensitive = FALSE;

	/*FIXME*/
	/*
	 *    I'm not sure how to generate these colors.
	 */
	if (w_data->js_mode == JAVASCRIPT_CLIENT) {
		pooh.red = 255;
		pooh.blue = 0;
		pooh.green = 0;
		color = &pooh;
	} else if (w_data->js_mode == JAVASCRIPT_SERVER) {
		pooh.red = 0;
		pooh.blue = 255;
		pooh.green = 0;
		color = &pooh;
	} else { /* JAVASCRIPT_NONE */

		if (w_data->is_custom_color) {
			color = &w_data->color;
			sensitive = TRUE;
		} else {
			MWContext* context = (MWContext *)fe_WidgetToMWContext(widget);

			fe_EditorDocumentGetColors(context,
									   NULL, /* bg image */
									   NULL, /* bg color */
									   &pooh,/* normal color */
									   NULL, /* link color */
									   NULL, /* active color */
									   NULL); /* followed color */
			color = &pooh;
		}
	} 

	fe_SwatchSetColor(widget, color);
	fe_SwatchSetSensitive(widget, sensitive);
}

Boolean
fe_ColorPicker(MWContext* context, LO_Color* in_out)
{
	LO_Color color = *in_out;

	if (fe_color_do_work(context, &color) == XmDIALOG_OK_BUTTON) {
		*in_out = color;
		return TRUE;
	}
	return FALSE;
}

static void
fe_editor_props_color_picker_cb(Widget widget,
								XtPointer closure, XtPointer call_data)
{
	fe_EditorCharacterPropertiesWidgets* w_data
		= (fe_EditorCharacterPropertiesWidgets*)closure;
	LO_Color color;

	if (fe_ColorPicker(w_data->properties->context, &color)) {
		w_data->color = color;
		w_data->is_custom_color = TRUE;
		w_data->changed_mask |= TF_FONT_COLOR;
	}
	fe_update_dependents(widget, w_data->properties, PROP_CHAR_COLOR);
}

static void
fe_color_default_update_cb(Widget widget, XtPointer closure,
						   XtPointer call_data)
{
	fe_EditorCharacterPropertiesWidgets* w_data
		= (fe_EditorCharacterPropertiesWidgets*)closure;
	Boolean sensitive = (w_data->js_mode == JAVASCRIPT_NONE);
	Boolean set = !sensitive || (w_data->is_custom_color == FALSE);

	XmToggleButtonSetState(widget, set, FALSE);
	fe_WidgetSetSensitive(widget, sensitive);
}

static void
fe_color_default_cb(Widget widget, XtPointer closure, XtPointer call_data)
{
	fe_EditorCharacterPropertiesWidgets* w_data =
		(fe_EditorCharacterPropertiesWidgets*)closure;
	XmToggleButtonCallbackStruct* info =
		(XmToggleButtonCallbackStruct*)call_data;

	if (info->set) {
		w_data->changed_mask |= TF_FONT_COLOR;
		w_data->is_custom_color = FALSE;
		fe_update_dependents(widget, w_data->properties, PROP_CHAR_COLOR);
	}
}

static void
fe_color_custom_update_cb(Widget widget, XtPointer closure,
						  XtPointer call_data)
{
	fe_EditorCharacterPropertiesWidgets* w_data
		= (fe_EditorCharacterPropertiesWidgets*)closure;
	Boolean sensitive = (w_data->js_mode == JAVASCRIPT_NONE);
	Boolean set = sensitive && (w_data->is_custom_color == TRUE);

	XmToggleButtonSetState(widget, set, FALSE);
	fe_WidgetSetSensitive(widget, sensitive);
}

static void
fe_choose_color_update_cb(Widget widget, XtPointer closure,
						  XtPointer call_data)
{
	fe_EditorCharacterPropertiesWidgets* w_data
		= (fe_EditorCharacterPropertiesWidgets*)closure;
	Boolean sensitive = (w_data->js_mode == JAVASCRIPT_NONE) &&
                        (w_data->is_custom_color == TRUE);

	fe_WidgetSetSensitive(widget, sensitive);
}

static void
fe_clear_styles_update_cb(Widget widget, XtPointer closure,
						  XtPointer call_data)
{
	fe_EditorCharacterPropertiesWidgets* w_data
		= (fe_EditorCharacterPropertiesWidgets*)closure;
	Boolean sensitive = (w_data->js_mode == JAVASCRIPT_NONE);

	fe_WidgetSetSensitive(widget, sensitive);
}

static void
fe_color_custom_cb(Widget widget, XtPointer closure, XtPointer call_data)
{
	fe_EditorCharacterPropertiesWidgets* w_data =
		(fe_EditorCharacterPropertiesWidgets*)closure;
	XmToggleButtonCallbackStruct* info =
		(XmToggleButtonCallbackStruct*)call_data;

	if (info->set) {
		w_data->changed_mask |= TF_FONT_COLOR;
		w_data->is_custom_color = TRUE;
		fe_update_dependents(widget, w_data->properties, PROP_CHAR_COLOR);
	}
}

static void
fe_javascript_cb(Widget widget, XtPointer closure, XtPointer call_data)
{
	fe_EditorCharacterPropertiesWidgets* w_data =
		(fe_EditorCharacterPropertiesWidgets*)closure;
	XmToggleButtonCallbackStruct* info =
		(XmToggleButtonCallbackStruct*)call_data;
	Boolean is_server = (fe_GetUserData(widget) != NULL);

	if (info->set) {
		if (is_server) {
			w_data->js_mode = JAVASCRIPT_SERVER;
		} else {
			w_data->js_mode = JAVASCRIPT_CLIENT;
		}

		/*
		 *    Javascript text doesn't have links, clear it..
		 */
		if (w_data->properties->link != NULL)
			fe_link_tab_remove_link(w_data->properties->link);
		
	} else {
		w_data->js_mode = JAVASCRIPT_NONE;
	}
	w_data->changed_mask |= PROP_CHAR_CLIENT|PROP_CHAR_SERVER;

	fe_update_dependents(widget, w_data->properties, PROP_CHAR_ALL);
}

static void
fe_javascript_update_cb(Widget widget, XtPointer closure, XtPointer call_data)
{
	fe_EditorCharacterPropertiesWidgets* w_data =
		(fe_EditorCharacterPropertiesWidgets*)closure;
	Boolean is_server = (fe_GetUserData(widget) != NULL);
	Boolean set = FALSE;

	if (is_server && (w_data->js_mode == JAVASCRIPT_SERVER))
		set = TRUE;
	else if (!is_server && (w_data->js_mode == JAVASCRIPT_CLIENT))
		set = TRUE;
	
	XmToggleButtonGadgetSetState(widget, set, FALSE);
}

static void
fe_EditorCharacterPropertiesDialogDataGet(
								 MWContext* context,
								 fe_EditorCharacterPropertiesWidgets* w_data)
{
	w_data->text_attributes = fe_EditorCharacterPropertiesGet(context);
	w_data->font_size = fe_EditorFontSizeGet(context);
	w_data->changed_mask = 0;

	if ((w_data->text_attributes & TF_SERVER))
		w_data->js_mode = JAVASCRIPT_SERVER;
	else if ((w_data->text_attributes & TF_SCRIPT))
		w_data->js_mode = JAVASCRIPT_CLIENT;
	else
		w_data->js_mode = JAVASCRIPT_NONE;
	w_data->text_attributes &= ~(TF_SERVER|TF_SCRIPT);

	if (fe_EditorColorGet(context, &w_data->color))
		w_data->is_custom_color = TRUE;
	else
		w_data->is_custom_color = FALSE;

	fe_update_dependents(NULL, w_data->properties, PROP_CHAR_ALL);
}

static void
fe_EditorCharacterPropertiesDialogSet(
								 MWContext* context,
								 fe_EditorCharacterPropertiesWidgets* w_data)
{
	LO_Color* color;

	if ((w_data->js_mode == JAVASCRIPT_SERVER)) {
		fe_EditorCharacterPropertiesSet(context, TF_SERVER);
	} else if ((w_data->js_mode == JAVASCRIPT_CLIENT)) {
		fe_EditorCharacterPropertiesSet(context, TF_SCRIPT);
	} else {

		/* clear javascript */
		if (w_data->changed_mask & (PROP_CHAR_CLIENT|PROP_CHAR_SERVER)) {
			fe_EditorCharacterPropertiesSet(context, (TF_SERVER|TF_SCRIPT));
			w_data->changed_mask = ~0;
		}

		if ((w_data->changed_mask & TF_ALL_MASK)!= 0) {
			fe_EditorCharacterPropertiesSet(context, 
									(w_data->text_attributes & TF_ALL_MASK));
		}

		if ((w_data->changed_mask & TF_FONT_SIZE)!= 0)
			fe_EditorFontSizeSet(context, w_data->font_size);

		if ((w_data->changed_mask & TF_FONT_COLOR)!= 0) {
			
			if (w_data->is_custom_color)
				color = &w_data->color;
			else
				color = NULL;

			fe_EditorColorSet(context, color);
		}
	}
}

static void
fe_make_character_page(
					   MWContext *context,
					   Widget parent,
					   fe_EditorCharacterPropertiesWidgets* w_data)
{
	Arg    args[32];
	Cardinal n;
	Widget form;
	Widget color_frame;
	Widget color_form;
	Widget color_label;
	Widget color_swatch;
	Widget color_default;
	Widget color_custom;
	Widget color_choose;
	Widget color_text;

	Widget size_frame;
	Widget size_form;
	Widget size_menu;
	Widget size_text;
	Widget size_cascade;

	Widget style_frame;
	Widget style_outer_rc;
	Widget style_inner_rc;
	Widget style_clear;

	Widget java_frame;
	Widget java_rc;
	
	Widget clear_all;

	Dimension width;
	Dimension height;

	Widget children[16];
	Cardinal nchildren;

#if 0
	n = 0;
	form = XmCreateForm(parent, "characterProperties", args, n);
	XtManageChild(form);
#else
	form = parent;
#endif
	w_data->form = form;

	/*
	 *    Color group.
	 */
	n = 0;
	XtSetArg(args[n], XmNtopAttachment, XmATTACH_FORM); n++;
	XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM); n++;
	color_frame = fe_CreateFrame(form, "color", args, n);
	XtManageChild(color_frame);

	n = 0;
	color_form = XmCreateForm(color_frame, "color", args, n);
	XtManageChild(color_form);

	nchildren = 0;
	n = 0;
	XtSetArg(args[n], XmNtopAttachment, XmATTACH_FORM); n++;
	XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM); n++;
	children[nchildren++] = color_label = XmCreateLabelGadget(color_form, "colorLabel", args, n);
	
	n = 0;
	XtSetArg(args[n], XmNtopAttachment, XmATTACH_FORM); n++;
	XtSetArg(args[n], XmNleftAttachment, XmATTACH_WIDGET); n++;
	XtSetArg(args[n], XmNleftWidget, color_label); n++;
	XtSetArg(args[n], XmNshadowThickness, 2); n++; /* make it look like label */
	children[nchildren++] = color_swatch = fe_CreateSwatch(color_form, "colorSwatch", args, n);
	w_data->color_swatch = color_swatch;

	fe_register_dependent(w_data->properties, color_swatch,
								 FE_MAKE_DEPENDENCY(PROP_CHAR_COLOR),
								 fe_color_swatch_update_cb, (XtPointer)w_data);
	XtAddCallback(color_swatch, XmNactivateCallback,
				  fe_editor_props_color_picker_cb, (XtPointer)w_data);

	n = 0;
	XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM); n++;
	XtSetArg(args[n], XmNtopAttachment, XmATTACH_WIDGET); n++;
	XtSetArg(args[n], XmNtopWidget, color_label); n++;
	XtSetArg(args[n], XmNindicatorType, XmONE_OF_MANY); n++;
	XtSetArg(args[n], XmNset, TRUE); n++;
	children[nchildren++] = color_default = XmCreateToggleButtonGadget(color_form, "default", args, n);

	XtAddCallback(color_default, XmNvalueChangedCallback,
								 fe_color_default_cb, (XtPointer)w_data);

	fe_register_dependent(w_data->properties, color_default,
								 FE_MAKE_DEPENDENCY(PROP_CHAR_COLOR),
								 fe_color_default_update_cb, (XtPointer)w_data);
	
	n = 0;
	XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM); n++;
	XtSetArg(args[n], XmNtopAttachment, XmATTACH_WIDGET); n++;
	XtSetArg(args[n], XmNtopWidget, color_default); n++;
	XtSetArg(args[n], XmNindicatorType, XmONE_OF_MANY); n++;
	XtSetArg(args[n], XmNset, FALSE); n++;
	children[nchildren++] = color_custom = XmCreateToggleButtonGadget(color_form, "custom", args, n);
	
	XtAddCallback(color_custom, XmNvalueChangedCallback,
								 fe_color_custom_cb, (XtPointer)w_data);

	fe_register_dependent(w_data->properties, color_custom,
								 FE_MAKE_DEPENDENCY(PROP_CHAR_COLOR),
								 fe_color_custom_update_cb, (XtPointer)w_data);

	n = 0;
	XtSetArg(args[n], XmNtopAttachment, XmATTACH_WIDGET); n++;
	XtSetArg(args[n], XmNtopWidget, color_default); n++;
	XtSetArg(args[n], XmNleftAttachment, XmATTACH_WIDGET); n++;
	XtSetArg(args[n], XmNleftWidget, color_custom); n++;
	children[nchildren++] = color_choose = XmCreatePushButtonGadget(color_form, "chooseColor", args, n);
	 
	XtAddCallback(color_choose, XmNactivateCallback,
				  fe_editor_props_color_picker_cb, (XtPointer)w_data);
	fe_register_dependent(w_data->properties, color_choose,
						  FE_MAKE_DEPENDENCY(PROP_CHAR_COLOR),
						  fe_choose_color_update_cb, (XtPointer)w_data);

	n = 0;
	XtSetArg(args[n], XmNtopAttachment, XmATTACH_WIDGET); n++;
	XtSetArg(args[n], XmNtopWidget, color_custom); n++;
	XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM); n++;
	XtSetArg(args[n], XmNalignment, XmALIGNMENT_BEGINNING); n++;
	children[nchildren++] = color_text = XmCreateLabelGadget(color_form, "colorText", args, n);

	XtManageChildren(children, nchildren); /* children of color form */

	/*
	 *    Set the swatch to be same height as label, and width as
	 *    choose button.
	 */
	XtVaGetValues(color_label, XmNheight, &height, 0);
	XtVaGetValues(color_choose, XmNwidth, &width, 0);
	n = 0;
	XtSetArg(args[n], XmNwidth, width); n++;
	XtSetArg(args[n], XmNheight, height); n++;
	XtSetValues(color_swatch, args, n);

	/*
	 *    Size group.
	 */
	n = 0;
	XtSetArg(args[n], XmNtopAttachment, XmATTACH_FORM); n++;
	XtSetArg(args[n], XmNleftAttachment, XmATTACH_WIDGET); n++;
	XtSetArg(args[n], XmNleftWidget, color_frame); n++;
	XtSetArg(args[n], XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET); n++;
	XtSetArg(args[n], XmNbottomWidget, color_frame); n++;
	XtSetArg(args[n], XmNrightAttachment, XmATTACH_FORM); n++;
	size_frame = fe_CreateFrame(form, "size", args, n);
	XtManageChild(size_frame);

	n = 0;
	size_form = XmCreateForm(size_frame, "size", args, n);
	XtManageChild(size_form);
	
	nchildren = 0;
	n = 0;
	size_menu = fe_CreatePulldownMenu(size_form,
									  "fontSize",
									  args, n);

	nchildren = 0;
	for (nchildren = 0; fe_font_size_types[nchildren].name; nchildren++) {
		n = 0;
		XtSetArg(args[n], XmNuserData, fe_font_size_types[nchildren].data); n++;
		children[nchildren] = XmCreatePushButtonGadget(
												   size_menu,
												   fe_font_size_types[nchildren].name,
												   args,
												   n
												   );
		XtAddCallback(children[nchildren], XmNactivateCallback,
					  fe_font_size_cb, (XtPointer)w_data);
															   
	}
	XtManageChildren(children, nchildren); /* size buttons */
	
	n = 0;
	XtSetArg(args[n], XmNsubMenuId, size_menu); n++;
	XtSetArg(args[n], XmNtopAttachment, XmATTACH_FORM); n++;
	XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM); n++;
	size_cascade = fe_CreateOptionMenu(
									  size_form,
									  "fontSizeOptionMenu",
									  args, n);
	fe_UnmanageChild_safe(XmOptionLabelGadget(size_cascade));
	fe_register_dependent(w_data->properties, size_cascade,
					 FE_MAKE_DEPENDENCY(PROP_CHAR_SIZE),
					 fe_font_size_update_cb, (XtPointer)w_data);

	XtManageChild(size_cascade);

	n = 0;
	XtSetArg(args[n], XmNtopAttachment, XmATTACH_WIDGET); n++;
	XtSetArg(args[n], XmNtopWidget, size_cascade); n++;
	XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM); n++;
	XtSetArg(args[n], XmNalignment, XmALIGNMENT_BEGINNING); n++;
	size_text = XmCreateLabelGadget(size_form, "sizeText",
									args, n);
	XtManageChild(size_text);

	/*
	 *    Style group.
	 */
	n = 0;
	XtSetArg(args[n], XmNtopAttachment, XmATTACH_WIDGET); n++;
	XtSetArg(args[n], XmNtopWidget, color_frame); n++;
	XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM); n++;
	XtSetArg(args[n], XmNrightAttachment, XmATTACH_OPPOSITE_WIDGET); n++;
	XtSetArg(args[n], XmNrightWidget, color_frame); n++;
	XtSetArg(args[n], XmNbottomAttachment, XmATTACH_FORM); n++;
	style_frame = fe_CreateFrame(form, "style", args, n);
	XtManageChild(style_frame);

	n = 0;
	style_outer_rc = XmCreateForm(style_frame, "style", args, n);
	XtManageChild(style_outer_rc);

	n = 0;
	XtSetArg(args[n], XmNorientation, XmVERTICAL); n++;
	XtSetArg(args[n], XmNnumColumns, 2); n++;
	XtSetArg(args[n], XmNpacking, XmPACK_COLUMN); n++;
	XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM); n++;
	XtSetArg(args[n], XmNtopAttachment, XmATTACH_FORM); n++;
	style_inner_rc = XmCreateRowColumn(style_outer_rc, "charProperties", args, n);
	XtManageChild(style_inner_rc);

	for (nchildren = 0; fe_style_types[nchildren].name; nchildren++) {
		n = 0;
		XtSetArg(args[n], XmNindicatorType, XmN_OF_MANY); n++;
		XtSetArg(args[n], XmNuserData, fe_style_types[nchildren].data); n++;
		children[nchildren] = XmCreateToggleButtonGadget(
											 style_inner_rc,
											 fe_style_types[nchildren].name,
											 args,
											 n);
															   
		XtAddCallback(children[nchildren], XmNvalueChangedCallback,
					  fe_style_cb, (XtPointer)w_data);

		fe_register_dependent(w_data->properties, children[nchildren],
						  FE_MAKE_DEPENDENCY(fe_style_types[nchildren].data),
						  fe_style_update_cb, (XtPointer)w_data);
	}
	XtManageChildren(children, nchildren); /* style toggles */

	n = 0;
	XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM); n++;
	XtSetArg(args[n], XmNrightAttachment, XmATTACH_FORM); n++;
	XtSetArg(args[n], XmNtopAttachment, XmATTACH_WIDGET); n++;
	XtSetArg(args[n], XmNtopWidget, style_inner_rc); n++;
	style_clear = XmCreatePushButtonGadget(style_outer_rc, "clearStyles",
										   args, n);
	XtAddCallback(style_clear, XmNactivateCallback,
				  fe_clear_style_cb, (XtPointer)w_data);
	fe_register_dependent(w_data->properties, style_clear,
						  FE_MAKE_DEPENDENCY(PROP_CHAR_JAVASCRIPT),
						  fe_clear_styles_update_cb, (XtPointer)w_data);

	XtManageChild(style_clear);

	/*
	 *    Java group.
	 */
	n = 0;
	XtSetArg(args[n], XmNtopAttachment, XmATTACH_WIDGET); n++;
	XtSetArg(args[n], XmNtopWidget, color_frame); n++;
	XtSetArg(args[n], XmNleftAttachment, XmATTACH_WIDGET); n++;
	XtSetArg(args[n], XmNleftWidget, color_frame); n++;
	XtSetArg(args[n], XmNrightAttachment, XmATTACH_FORM); n++;
	java_frame = fe_CreateFrame(form, "java", args, n);
	XtManageChild(java_frame);

	n = 0;
	XtSetArg(args[n], XmNorientation, XmVERTICAL); n++;
	java_rc = XmCreateRowColumn(java_frame, "java", args, n);
	XtManageChild(java_rc);

	nchildren = 0;
	n = 0;
	XtSetArg(args[n], XmNindicatorType, XmN_OF_MANY); n++;
	XtSetArg(args[n], XmNuserData, FALSE); n++;
	children[nchildren] = XmCreateToggleButtonGadget(java_rc, "client",
													 args, n);
	
	XtAddCallback(children[nchildren], XmNvalueChangedCallback,
				  fe_javascript_cb, (XtPointer)w_data);
	fe_register_dependent(w_data->properties, children[nchildren],
						  FE_MAKE_DEPENDENCY(PROP_CHAR_JAVASCRIPT), /* lazy */
						  fe_javascript_update_cb, (XtPointer)w_data);
	nchildren++;

	n = 0;
	XtSetArg(args[n], XmNindicatorType, XmN_OF_MANY); n++;
	XtSetArg(args[n], XmNuserData, TRUE); n++;
	children[nchildren] = XmCreateToggleButtonGadget(java_rc, "server",
													   args, n);
	XtAddCallback(children[nchildren], XmNvalueChangedCallback,
				  fe_javascript_cb, (XtPointer)w_data);
	fe_register_dependent(w_data->properties, children[nchildren],
						  FE_MAKE_DEPENDENCY(PROP_CHAR_JAVASCRIPT), /* lazy */
						  fe_javascript_update_cb, (XtPointer)w_data);
	nchildren++;

	XtManageChildren(children, nchildren); /* java toggles */

	n = 0;
	XtSetArg(args[n], XmNtopAttachment, XmATTACH_WIDGET); n++;
	XtSetArg(args[n], XmNtopWidget, java_frame); n++;
	XtSetArg(args[n], XmNleftAttachment, XmATTACH_OPPOSITE_WIDGET); n++;
	XtSetArg(args[n], XmNleftWidget, java_frame); n++;
	XtSetArg(args[n], XmNrightAttachment, XmATTACH_OPPOSITE_WIDGET); n++;
	XtSetArg(args[n], XmNrightWidget, java_frame); n++;
	clear_all = XmCreatePushButtonGadget(form, "clearAll", args, n);
	XtManageChild(clear_all);

	XtAddCallback(clear_all, XmNactivateCallback,
				  fe_clearall_settings_cb, (XtPointer)w_data);
}

static void
fe_image_main_image_cb(Widget widget, XtPointer closure, XtPointer call_data)
{
	fe_EditorImagePropertiesWidgets* w_data =
		(fe_EditorImagePropertiesWidgets*)closure;

	/*
	 *    Update changed tag, but don't call dependents (that's us).
	 *    Well we have to, so that we update the copy button.
	 *    No, we don't we'll set the tag here, but we'll only
	 *    call update_dependents from the browse cb. That way
	 *    we won't be *busy*. No, we have to, oh too bad!
	 */
	w_data->new_image = TRUE;
	fe_update_dependents(widget, w_data->properties, 
						 PROP_IMAGE_MAIN_IMAGE|PROP_IMAGE_COPY|PROP_LINK_TEXT);
}

static void
fe_image_main_image_update_cb(Widget widget,
							  XtPointer closure, XtPointer call_data)
{
	fe_EditorImagePropertiesWidgets* w_data =
		(fe_EditorImagePropertiesWidgets*)closure;
	fe_DependentListCallbackStruct* info = 
		(fe_DependentListCallbackStruct*)call_data;

	if (info->caller == widget)
		return; /* don't call ourselves */
	
	fe_set_text_field(widget, w_data->image_data.pSrc,
					  fe_image_main_image_cb, w_data);
}

static void
fe_image_main_image_browse_cb(Widget widget,
							  XtPointer closure, XtPointer call_data)
{
	fe_EditorImagePropertiesWidgets* w_data =
		(fe_EditorImagePropertiesWidgets*)closure;

	fe_browse_file_of_text(w_data->properties->context,
						   w_data->main_image,
						   FALSE);
}

static void
fe_image_alt_image_cb(Widget widget, XtPointer closure, XtPointer call_data)
{
	fe_EditorImagePropertiesWidgets* w_data =
		(fe_EditorImagePropertiesWidgets*)closure;

	/*
	 *    See discussion for main above.
	 */
	w_data->new_image = TRUE;
	fe_update_dependents(widget, w_data->properties, 
						 PROP_IMAGE_ALT_IMAGE|PROP_IMAGE_COPY|PROP_LINK_TEXT);
}

static void
fe_image_alt_image_update_cb(Widget widget,
							  XtPointer closure, XtPointer call_data)
{
	fe_EditorImagePropertiesWidgets* w_data =
		(fe_EditorImagePropertiesWidgets*)closure;
	fe_DependentListCallbackStruct* info = 
		(fe_DependentListCallbackStruct*)call_data;

	if (info->caller == widget)
		return; /* don't call ourselves */
	
	fe_set_text_field(w_data->alt_image, w_data->image_data.pLowSrc,
					  fe_image_alt_image_cb, w_data);
}

static void
fe_image_alt_image_browse_cb(Widget widget,
							  XtPointer closure, XtPointer call_data)
{
	fe_EditorImagePropertiesWidgets* w_data =
		(fe_EditorImagePropertiesWidgets*)closure;

	fe_browse_file_of_text(w_data->properties->context,
						   w_data->alt_image,
						   FALSE);
}

static void
fe_image_alt_text_cb(Widget widget, XtPointer closure, XtPointer call_data)
{
	fe_EditorImagePropertiesWidgets* w_data =
		(fe_EditorImagePropertiesWidgets*)closure;

	fe_update_dependents(widget, w_data->properties,
						 PROP_IMAGE_ALT_TEXT|PROP_IMAGE_COPY|PROP_LINK_TEXT);
}

static void
fe_image_alt_text_update_cb(Widget widget,
							  XtPointer closure, XtPointer call_data)
{
	fe_EditorImagePropertiesWidgets* w_data =
		(fe_EditorImagePropertiesWidgets*)closure;
	fe_DependentListCallbackStruct* info = 
		(fe_DependentListCallbackStruct*)call_data;

	if (info->caller == widget)
		return; /* don't call ourselves */
	
	fe_set_text_field(w_data->alt_text, w_data->image_data.pAlt,
					  fe_image_alt_text_cb, w_data);
}

static void
fe_image_align_cb(Widget widget, XtPointer closure, XtPointer call_data)
{
	fe_EditorImagePropertiesWidgets* w_data =
		(fe_EditorImagePropertiesWidgets*)closure;
	XmToggleButtonCallbackStruct* info =
		(XmToggleButtonCallbackStruct*)call_data;

	/*
	 *    We get a callback on both the unset of the old toggle,
	 *    and the set of the new, so ignore the former, it's not
	 *    interesting.
	 */
	if (info->set) {

		w_data->image_data.align = (ED_Alignment)fe_GetUserData(widget);	

		fe_update_dependents(widget, w_data->properties, PROP_IMAGE_ALIGN);
	}
}

static void
fe_image_align_update_cb(Widget widget,
						 XtPointer closure, XtPointer call_data)
{
	fe_EditorImagePropertiesWidgets* w_data =
		(fe_EditorImagePropertiesWidgets*)closure;
	ED_Alignment align = (ED_Alignment)fe_GetUserData(widget);	

	XmToggleButtonGadgetSetState(widget, 
								 (w_data->image_data.align == align),
								 FALSE);
}

static void
fe_image_dimensions_cb(Widget widget, XtPointer closure, XtPointer call_data)
{
	fe_EditorImagePropertiesWidgets* w_data =
		(fe_EditorImagePropertiesWidgets*)closure;

	w_data->properties->changed |= PROP_IMAGE_DIMENSIONS;
}

static void
fe_image_dimensions_height_update_cb(Widget widget,
									 XtPointer closure, XtPointer call_data)
{
	fe_EditorImagePropertiesWidgets* w_data =
		(fe_EditorImagePropertiesWidgets*)closure;
	fe_DependentListCallbackStruct* info = 
		(fe_DependentListCallbackStruct*)call_data;
	Boolean enabled = (w_data->existing_image);

	if (info->caller == widget)
		return; /* don't call ourselves */
	
	fe_set_numeric_text_field(widget, w_data->image_data.iHeight);
	fe_TextFieldSetEditable(w_data->properties->context, widget, enabled);
}

static void
fe_image_dimensions_width_update_cb(Widget widget,
									 XtPointer closure, XtPointer call_data)
{
	fe_EditorImagePropertiesWidgets* w_data =
		(fe_EditorImagePropertiesWidgets*)closure;
	fe_DependentListCallbackStruct* info = 
		(fe_DependentListCallbackStruct*)call_data;
	Boolean enabled = (w_data->existing_image);

	if (info->caller == widget)
		return; /* don't call ourselves */
	
	fe_set_numeric_text_field(widget, w_data->image_data.iWidth);
	fe_TextFieldSetEditable(w_data->properties->context, widget, enabled);
}

static void
fe_image_margin_cb(Widget widget, XtPointer closure, XtPointer call_data)
{
	fe_EditorImagePropertiesWidgets* w_data =
		(fe_EditorImagePropertiesWidgets*)closure;

	w_data->properties->changed |= PROP_IMAGE_SPACE;
}

static void
fe_image_margin_height_update_cb(Widget widget,
									 XtPointer closure, XtPointer call_data)
{
	fe_EditorImagePropertiesWidgets* w_data =
		(fe_EditorImagePropertiesWidgets*)closure;
	fe_DependentListCallbackStruct* info = 
		(fe_DependentListCallbackStruct*)call_data;

	if (info->caller == widget)
		return; /* don't call ourselves */
	
	fe_set_numeric_text_field(widget, w_data->image_data.iVSpace);
}

static void
fe_image_margin_width_update_cb(Widget widget,
									 XtPointer closure, XtPointer call_data)
{
	fe_EditorImagePropertiesWidgets* w_data =
		(fe_EditorImagePropertiesWidgets*)closure;
	fe_DependentListCallbackStruct* info = 
		(fe_DependentListCallbackStruct*)call_data;

	if (info->caller == widget)
		return; /* don't call ourselves */

	fe_set_numeric_text_field(widget,
							  w_data->image_data.iHSpace);
}

static Boolean
fe_has_link(fe_EditorPropertiesWidgets* properties)
{
	if (properties->link != NULL)
		return fe_link_tab_has_link(properties->link);
	else
		return FALSE;
}

static void
fe_image_margin_border_update_cb(Widget widget,
								 XtPointer closure, XtPointer call_data)
{
	fe_EditorImagePropertiesWidgets* w_data =
		(fe_EditorImagePropertiesWidgets*)closure;
	fe_DependentListCallbackStruct* info = 
		(fe_DependentListCallbackStruct*)call_data;
	unsigned value;

	if (info->caller == widget)
		return; /* don't call ourselves */

	if (w_data->image_data.iBorder == -1) { /* back end does this */
		if (fe_has_link(w_data->properties))
			value = 2;
		else
			value = 0;
	} else {
		value = w_data->image_data.iBorder;
	}

	fe_set_numeric_text_field(widget, value);
}

static void
fe_image_original_size_cb(Widget widget,
						  XtPointer closure, XtPointer call_data)
{
	fe_EditorImagePropertiesWidgets* w_data =
		(fe_EditorImagePropertiesWidgets*)closure;

	w_data->image_data.iWidth = w_data->image_data.iOriginalWidth;
	w_data->image_data.iHeight = w_data->image_data.iOriginalHeight;
	fe_update_dependents(widget, w_data->properties, PROP_IMAGE_DIMENSIONS);
}

static void
fe_image_original_size_update_cb(Widget widget,
						  XtPointer closure, XtPointer call_data)
{
	fe_EditorImagePropertiesWidgets* w_data =
		(fe_EditorImagePropertiesWidgets*)closure;
	Boolean enabled =  (w_data->existing_image) && !(w_data->new_image);
		
	XtVaSetValues(widget, XmNsensitive, enabled, 0);
}

static void
fe_image_copy_cb(Widget widget, XtPointer closure, XtPointer call_data)
{
	fe_EditorImagePropertiesWidgets* w_data =
		(fe_EditorImagePropertiesWidgets*)closure;
	XmToggleButtonCallbackStruct* info =
		(XmToggleButtonCallbackStruct*)call_data;

	w_data->do_copy = info->set;
	fe_update_dependents(widget, w_data->properties, PROP_IMAGE_COPY);
}

static void
fe_image_copy_update_cb(Widget widget, XtPointer closure, XtPointer call_data)
{
	fe_EditorImagePropertiesWidgets* w_data =
		(fe_EditorImagePropertiesWidgets*)closure;
	Arg args[8];
	Cardinal n;

	n = 0;
#if 1
	XtSetArg(args[n], XmNsensitive, TRUE); n++;
#else
	XtSetArg(args[n], XmNsensitive, (w_data->new_image)); n++;
#endif
	XtSetArg(args[n], XmNset, (w_data->do_copy)); n++;
	XtSetValues(widget, args, n);
#if 0
	XmToggleButtonSetState(widget, (w_data->do_copy), FALSE);
#endif
}

static void
fe_image_remove_imap_cb(Widget widget, XtPointer closure, XtPointer call_data)
{
	fe_EditorImagePropertiesWidgets* w_data =
		(fe_EditorImagePropertiesWidgets*)closure;

	w_data->image_data.bIsMap = FALSE;
	fe_update_dependents(widget, w_data->properties, PROP_IMAGE_IMAP);
}

static void
fe_image_remove_imap_update_cb(Widget widget,
							   XtPointer closure, XtPointer call_data)
{
	fe_EditorImagePropertiesWidgets* w_data =
		(fe_EditorImagePropertiesWidgets*)closure;
	Arg args[8];
	Cardinal n;

	n = 0;
	XtSetArg(args[n], XmNsensitive, (w_data->image_data.bIsMap != FALSE)); n++;
	XtSetValues(widget, args, n);
}

static void
fe_image_edit_image_cb(Widget widget, XtPointer closure, XtPointer call_data)
{
	fe_EditorImagePropertiesWidgets* w_data =
		(fe_EditorImagePropertiesWidgets*)closure;
	MWContext* context = w_data->properties->context;
    History_entry* hist_ent = SHIST_GetCurrent(&(context->hist));
	char* image;
	char* url;
	char* local_name;
	char buf[512];

    if (hist_ent == NULL || hist_ent->address == NULL)
		return; /* should not happen */

	image = XmTextFieldGetString(w_data->main_image);

	if (image == NULL || image[0] == '\0')
		return; /* also shouldn't happen */

	if (NET_IsHTTP_URL(image)) { /* remote url */
		fe_error_dialog(context, widget,
						XP_GetString(XFE_EDITOR_IMAGE_IS_REMOTE));
        return;
    }

	url = NET_MakeAbsoluteURL(hist_ent->address, image); /* alloc */
    if (XP_ConvertUrlToLocalFile(url, &local_name)) {
		fe_EditorEditImage(context, local_name);
	} else {
		sprintf(buf, XP_GetString(XFE_CANNOT_READ_FILE), image);

		fe_error_dialog(context, widget, buf);
	}

	if (image)
		XtFree(image);
	if (url)
		XtFree(url);
	if (local_name)
		XtFree(local_name);
}

static void
fe_image_edit_image_update_cb(Widget widget,
							   XtPointer closure, XtPointer call_data)
{
	fe_EditorImagePropertiesWidgets* w_data =
		(fe_EditorImagePropertiesWidgets*)closure;
	Boolean sensitive = XmTextFieldGetLastPosition(w_data->main_image) != 0;

	XtVaSetValues(widget, XmNsensitive, sensitive, 0);
}

static void
fe_EditorImagePropertiesDialogDataGet(MWContext* context,
								  fe_EditorImagePropertiesWidgets* w_data)
{
	EDT_ImageData* old = NULL;

	if (EDT_GetCurrentElementType(context) == ED_ELEMENT_IMAGE) {
		old = EDT_GetImageData(context);
		memcpy(&w_data->image_data, old, sizeof(EDT_ImageData));
		w_data->existing_image = TRUE;
		/*
		 *    Apparently, these are only convenience values for
		 *    use transiently by the dialog. The backend DOES NOT
		 *    save these values.
		 */
		w_data->image_data.iOriginalWidth = w_data->image_data.iWidth;
		w_data->image_data.iOriginalHeight = w_data->image_data.iHeight;
	} else {
		memset(&w_data->image_data, 0, sizeof(EDT_ImageData));
		w_data->image_data.align = ED_ALIGN_BASELINE;
	}
	w_data->new_image = FALSE;
	fe_EditorPreferencesGetLinksAndImages(context, NULL, &w_data->do_copy);

	fe_update_dependents(NULL, w_data->properties, PROP_IMAGE_ALL);
	/* yes, that's all sally */

	if (old)
		EDT_FreeImageData(old);
}

static void
fe_copy_string_over(char** tgt, Widget widget)
{
	char* p = XmTextFieldGetString(widget);
	
	if (*tgt)
		XP_FREE(*tgt);
	if (p && *p != '\0')
		*tgt = XP_STRDUP(p);
	else
		*tgt = NULL;
	if (p)
		XtFree(p);
}


static void
fe_editor_image_properties_common_set(MWContext* context,
											 EDT_ImageData* data,
								  fe_EditorImagePropertiesWidgets* w_data)
{
	data->iWidth = fe_get_numeric_text_field(w_data->image_width);
	data->iHeight = fe_get_numeric_text_field(w_data->image_height);
	data->iHSpace = fe_get_numeric_text_field(w_data->margin_width);
	data->iVSpace = fe_get_numeric_text_field(w_data->margin_height);
	data->iBorder = fe_get_numeric_text_field(w_data->margin_solid);
}

static Boolean
fe_editor_image_properties_validate(MWContext* context,
								  fe_EditorImagePropertiesWidgets* w_data)
{
	EDT_ImageData  image_data;
	EDT_ImageData* i = &image_data;
	unsigned       errors[16];
	unsigned       nerrors = 0;
	
	fe_editor_image_properties_common_set(context, &image_data, w_data);

	if (w_data->existing_image) {
		if (RANGE_CHECK(i->iWidth, IMAGE_MIN_WIDTH, IMAGE_MAX_WIDTH))
			errors[nerrors++] = XFE_INVALID_IMAGE_WIDTH;
		if (RANGE_CHECK(i->iHeight, IMAGE_MIN_HEIGHT, IMAGE_MAX_HEIGHT))
			errors[nerrors++] = XFE_INVALID_IMAGE_HEIGHT;
	}

	if (RANGE_CHECK(i->iHSpace, IMAGE_MIN_HSPACE, IMAGE_MAX_HSPACE))
		errors[nerrors++] = XFE_INVALID_IMAGE_HSPACE;
	if (RANGE_CHECK(i->iVSpace, IMAGE_MIN_VSPACE, IMAGE_MAX_VSPACE))
		errors[nerrors++] = XFE_INVALID_IMAGE_VSPACE;
	if (RANGE_CHECK(i->iBorder, IMAGE_MIN_BORDER, IMAGE_MAX_BORDER))
		errors[nerrors++] = XFE_INVALID_IMAGE_BORDER;

	if (nerrors > 0) {
		fe_editor_range_error_dialog(context, w_data->image_width,
									 errors, nerrors);
		return FALSE;
	}

	return TRUE;
}

static void
fe_editor_image_properties_set(MWContext* context,
								  fe_EditorImagePropertiesWidgets* w_data)
{
	EDT_ImageData* old;

	if (w_data->existing_image)
		old = EDT_GetImageData(context);
	else
		old = EDT_NewImageData();

	if ((w_data->properties->changed & PROP_IMAGE_MAIN_IMAGE) != 0)
		fe_copy_string_over(&old->pSrc, w_data->main_image);
	
	if ((w_data->properties->changed & PROP_IMAGE_ALT_IMAGE) != 0)
		fe_copy_string_over(&old->pLowSrc, w_data->alt_image);

	if ((w_data->properties->changed & PROP_IMAGE_ALT_TEXT) != 0)
		fe_copy_string_over(&old->pAlt, w_data->alt_text);
	
	fe_editor_image_properties_common_set(context, old, w_data);

	old->align = w_data->image_data.align;
	old->bIsMap = w_data->image_data.bIsMap;

	/*
	 *    We don't handle the link data here, let the link sheet
	 *    do that.
	 */
	if (old->pSrc != NULL) {
		if (w_data->existing_image)
			EDT_SetImageData(context, old, w_data->do_copy);
		else
			EDT_InsertImage(context, old, w_data->do_copy);
	}

	EDT_FreeImageData(old);

	/*
	 *    Reload.
	 */
	fe_EditorImagePropertiesDialogDataGet(context, w_data);
}

static struct fe_style_data fe_image_button_names[] = {
 	{ "alignTop",       ED_ALIGN_TOP       },
	{ "alignAbsCenter", ED_ALIGN_ABSCENTER },
	{ "alignCenter",    ED_ALIGN_CENTER    },
	{ "alignBaseline",  ED_ALIGN_BASELINE  },
	{ "alignAbsBottom", ED_ALIGN_ABSBOTTOM },
	{ "alignLeft",      ED_ALIGN_LEFT      },
	{ "alignRight",     ED_ALIGN_RIGHT     },
	{ 0 }

#if 0
	{ "alignAbsTop",    ED_ALIGN_ABSTOP    },
	{ "alignBottom",    ED_ALIGN_BOTTOM    },
#endif
};

static void
fe_make_image_icons(MWContext *context, Widget parent,
					fe_EditorImagePropertiesWidgets* w_data)
{
	Pixmap   p;
	Arg      args[16];
	Cardinal n;
	Cardinal i;
	Widget   children[16];
	char*    name;

	for (i = 0; fe_image_button_names[i].name; i++) {
	  
		p  = fe_ToolbarPixmap(context, IL_ALIGN1_RAISED+(2*i), False, False);

		name = fe_image_button_names[i].name;

		n = 0;
		XtSetArg(args[n], XmNlabelType, XmPIXMAP); n++;
		XtSetArg(args[n], XmNlabelPixmap, p); n++;
		XtSetArg(args[n], XmNuserData, fe_image_button_names[i].data); n++;
		XtSetArg(args[n], XmNindicatorOn, FALSE); n++; /* Windows-esk */
		children[i] = XmCreateToggleButtonGadget(parent, name, args, n);

		XtAddCallback(children[i], XmNvalueChangedCallback,
					  fe_image_align_cb, (XtPointer)w_data);
		fe_register_dependent(w_data->properties, children[i],
							  FE_MAKE_DEPENDENCY(PROP_IMAGE_ALIGN),
							  fe_image_align_update_cb, (XtPointer)w_data);
    }
	
	XtManageChildren(children, i);
}
								  

static void
fe_make_image_page(MWContext *context, Widget parent,
				   fe_EditorImagePropertiesWidgets* w_data)
{
  Arg av [50];
  int ac;
  Widget form;
  Widget kids [100];
  Widget ImageFileBrowse;
  Widget ImageBrowse,RemoveImageMap,EditImage,OriginalSize;
  Widget ImageFileTextField,WidthLabel,HeightLabel;
  Widget ImageFileFrame,ImageFileForm;
  Widget ImageSpaceFrame,ImageSpaceForm;
  Widget LeftRightLabel,TopBottomLabel,SolidBorderLabel;
  Widget LeftPixels,TopPixels,SolidPixels,heightPixels,widthPixels;
  Widget AlignFrame,AlignForm,AlignLabel;
  Widget AltRepsLabel,AltImageTextField,AltTextTextField;
  Widget DimensionsFrame,DimensionsForm;
  Widget HeightTextField,WidthTextField;
  Widget LeftTextField,TopTextField,SolidTextField;
  Widget CopyImageToggle,ImageFileLabel,ImageTextLabel;
  Widget align_rc;
  Dimension wide;
  int i;

#if 0
  ac = 0;
  XtSetArg (av [ac], XmNtopAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNbottomAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNleftAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNrightAttachment, XmATTACH_FORM); ac++;
  form = XmCreateForm(parent, "imageProperties", av, ac);
#else
  form = parent;
#endif

  /**********************************************************************/
  /*  Define the Image file name Frame and its form 4MAR96RCJ		*/
  /**********************************************************************/
  ac = 0;
  XtSetArg (av [ac], XmNleftAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNrightAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNtopAttachment, XmATTACH_FORM); ac++;
  ImageFileFrame = fe_CreateFrame(form, "imageFile", av, ac);

  ac = 0;
  XtSetArg (av [ac], XmNleftAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNrightAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNtopAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNbottomAttachment, XmATTACH_FORM); ac++;
  ImageFileForm = XmCreateForm (ImageFileFrame, "form", av, ac);
 
  ac = 0;
  i  = 0;
  XtSetArg (av [ac], XmNtopAttachment,   XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNrightAttachment, XmATTACH_FORM); ac++;
  kids [i++] = ImageFileBrowse = XmCreatePushButtonGadget (ImageFileForm,
                                              "browse", 
                                              av, ac);
  XtAddCallback(ImageFileBrowse, XmNactivateCallback,
				fe_image_main_image_browse_cb, (XtPointer)w_data);

  ac = 0;
  XtSetArg (av [ac], XmNtopAttachment,   XmATTACH_FORM);   ac++;
  XtSetArg (av [ac], XmNleftAttachment,  XmATTACH_FORM);   ac++;
  XtSetArg (av [ac], XmNrightAttachment, XmATTACH_WIDGET); ac++;
  XtSetArg (av [ac], XmNrightWidget,     ImageFileBrowse); ac++;
  kids [i++] = ImageFileTextField = fe_CreateTextField (ImageFileForm,
                                                     "imageFile",
                                                     av, ac);
  w_data->main_image = ImageFileTextField;
  XtAddCallback(ImageFileTextField, XmNvalueChangedCallback,
				fe_image_main_image_cb, (XtPointer)w_data);

  ac = 0;
  XtSetArg (av [ac], XmNalignment, XmALIGNMENT_BEGINNING); ac++;
  XtSetArg (av [ac], XmNtopAttachment,  XmATTACH_WIDGET);  ac++;
  XtSetArg (av [ac], XmNtopWidget,      ImageFileBrowse);  ac++;
  XtSetArg (av [ac], XmNleftAttachment, XmATTACH_FORM);    ac++;
  kids [i++] = AltRepsLabel = XmCreateLabelGadget (ImageFileForm, 
                                                   "alternativeInfoLabel",
						   av, ac);
  ac = 0;
  XtSetArg (av [ac], XmNalignment, XmALIGNMENT_BEGINNING); ac++;
  XtSetArg (av [ac], XmNtopAttachment,  XmATTACH_WIDGET);  ac++;
  XtSetArg (av [ac], XmNtopWidget,      AltRepsLabel);     ac++;
  XtSetArg (av [ac], XmNleftAttachment, XmATTACH_FORM);    ac++;
  kids [i++] = ImageFileLabel = XmCreateLabelGadget (ImageFileForm, 
                                                   "alternativeImageLabel",
						   av, ac);
  ac = 0;
  XtSetArg (av [ac], XmNtopAttachment,   XmATTACH_WIDGET); ac++;
  XtSetArg (av [ac], XmNtopWidget,       AltRepsLabel);    ac++;
  XtSetArg (av [ac], XmNrightAttachment, XmATTACH_FORM);   ac++;
  kids [i++] = ImageBrowse = XmCreatePushButtonGadget (ImageFileForm,
                                                       "browse",
                                                       av, ac);
  XtAddCallback(ImageBrowse, XmNactivateCallback,
				fe_image_alt_image_browse_cb, (XtPointer)w_data);

  ac = 0;
  XtSetArg (av [ac], XmNtopAttachment,   XmATTACH_WIDGET); ac++;
  XtSetArg (av [ac], XmNtopWidget,       AltRepsLabel);    ac++;
  XtSetArg (av [ac], XmNleftAttachment,  XmATTACH_WIDGET); ac++;
  XtSetArg (av [ac], XmNleftWidget,      ImageFileLabel);  ac++;
  XtSetArg (av [ac], XmNrightAttachment, XmATTACH_WIDGET); ac++;
  XtSetArg (av [ac], XmNrightWidget,     ImageBrowse);     ac++;
  kids [i++] = AltImageTextField = fe_CreateTextField (ImageFileForm,
                                                     "alternativeImage",
                                                     av, ac);
  w_data->alt_image = AltImageTextField;
  XtAddCallback(AltImageTextField, XmNvalueChangedCallback,
				fe_image_alt_image_cb, (XtPointer)w_data);
  fe_register_dependent(w_data->properties, AltImageTextField,
						FE_MAKE_DEPENDENCY(PROP_IMAGE_ALT_IMAGE),
						fe_image_alt_image_update_cb, (XtPointer)w_data);


  ac = 0;
  XtSetArg (av [ac], XmNalignment, XmALIGNMENT_BEGINNING); ac++;
  XtSetArg (av [ac], XmNtopAttachment,  XmATTACH_WIDGET);  ac++;
  XtSetArg (av [ac], XmNtopWidget,      ImageFileLabel);   ac++;
  XtSetArg (av [ac], XmNleftAttachment, XmATTACH_FORM);    ac++;
  kids [i++] = ImageTextLabel = XmCreateLabelGadget (ImageFileForm, 
                                                   "alternativeTextLabel",
						   av, ac);
  ac = 0;
  XtSetArg (av [ac], XmNtopAttachment,   XmATTACH_WIDGET);   ac++;
  XtSetArg (av [ac], XmNtopWidget,       AltImageTextField); ac++;
  XtSetArg (av [ac], XmNleftAttachment,  XmATTACH_WIDGET);   ac++;
  XtSetArg (av [ac], XmNleftWidget,      ImageFileLabel);    ac++;
  XtSetArg (av [ac], XmNrightAttachment, XmATTACH_FORM);     ac++;
  kids [i++] = AltTextTextField = fe_CreateTextField (ImageFileForm,
													  "alternativeText",
                                                     av, ac);
  w_data->alt_text = AltTextTextField;

  XtAddCallback(AltTextTextField, XmNvalueChangedCallback,
				fe_image_alt_text_cb, (XtPointer)w_data);
  fe_register_dependent(w_data->properties, AltTextTextField,
						FE_MAKE_DEPENDENCY(PROP_IMAGE_ALT_TEXT),
						fe_image_alt_text_update_cb, (XtPointer)w_data);

  XtManageChildren (kids, i);
  /**********************************************************************/
  /*  Define the Alignment Frame and its form	4MAR96RCJ		*/
  /**********************************************************************/
  ac = 0;
  XtSetArg (av [ac], XmNleftAttachment, XmATTACH_FORM);   ac++;
  XtSetArg (av [ac], XmNrightAttachment, XmATTACH_FORM);  ac++;
  XtSetArg (av [ac], XmNtopAttachment, XmATTACH_WIDGET);  ac++;
  XtSetArg (av [ac], XmNtopWidget, ImageFileFrame);       ac++;
  AlignFrame = fe_CreateFrame(form, "alignment", av, ac);

  ac = 0;
  AlignForm = XmCreateForm(AlignFrame, "alignmentForm", av, ac);

  ac = 0;
  XtSetArg(av[ac], XmNorientation, XmHORIZONTAL); ac++;
  XtSetArg(av[ac], XmNradioBehavior, TRUE); ac++;
  XtSetArg (av [ac], XmNleftAttachment, XmATTACH_FORM);   ac++;
  XtSetArg (av [ac], XmNrightAttachment, XmATTACH_FORM);  ac++;
  XtSetArg (av [ac], XmNtopAttachment, XmATTACH_FORM);  ac++;
  align_rc = XmCreateRowColumn(AlignForm, "alignmentRowColumn", av, ac);
  XtManageChild(align_rc);

  fe_make_image_icons(context, align_rc, w_data);

  ac = 0;
  XtSetArg (av [ac], XmNalignment,         XmALIGNMENT_END); ac++;
  XtSetArg (av [ac], XmNrightAttachment,   XmATTACH_FORM);   ac++;
  XtSetArg (av [ac], XmNtopAttachment, XmATTACH_WIDGET); ac++;
  XtSetArg (av [ac], XmNtopWidget,     align_rc);      ac++;
#if 0
  XtSetArg (av [ac], XmNbottomAttachment, XmATTACH_FORM); ac++;
#endif
  AlignLabel = XmCreateLabelGadget(AlignForm, "alignmentInfoLabel", av, ac);
  XtManageChild(AlignLabel);

  /**********************************************************************/
  /*  Define the Buttons at bottom of Image Properties Tab Panel	*/
  /**********************************************************************/
  ac = 0;
  i  = 0;
  XtSetArg (av [ac], XmNbottomAttachment, XmATTACH_FORM);   ac++;
  XtSetArg (av [ac], XmNrightAttachment,  XmATTACH_FORM);   ac++;
  XtSetArg (av [ac], XmNsensitive,        False);           ac++;
  kids [i++] = EditImage = XmCreatePushButtonGadget (form,
                                                     "editImage",
                                                     av, ac);
  XtAddCallback(EditImage, XmNactivateCallback,
				fe_image_edit_image_cb, (XtPointer)w_data);
  fe_register_dependent(w_data->properties, EditImage,
						FE_MAKE_DEPENDENCY(PROP_IMAGE_MAIN_IMAGE),
						fe_image_edit_image_update_cb, (XtPointer)w_data);

  ac = 0;
  XtSetArg (av [ac], XmNbottomAttachment, XmATTACH_FORM);   ac++;
  XtSetArg (av [ac], XmNrightAttachment,  XmATTACH_WIDGET); ac++;
  XtSetArg (av [ac], XmNrightWidget,      EditImage);       ac++;
  XtSetArg (av [ac], XmNsensitive,        False);           ac++;
  kids [i++] = RemoveImageMap = XmCreatePushButtonGadget (form,
                                                     "removeImageMap",
                                                     av, ac);
  XtAddCallback(RemoveImageMap, XmNactivateCallback,
				fe_image_remove_imap_cb, (XtPointer)w_data);
  fe_register_dependent(w_data->properties, RemoveImageMap,
						FE_MAKE_DEPENDENCY(PROP_IMAGE_IMAP),
						fe_image_remove_imap_update_cb, (XtPointer)w_data);

  ac = 0;
  XtSetArg (av [ac], XmNbottomAttachment, XmATTACH_FORM);   ac++;
  XtSetArg (av [ac], XmNleftAttachment,   XmATTACH_FORM);   ac++;
  kids [i++] = CopyImageToggle = XmCreateToggleButtonGadget (form,
                                                       "copyImage",
                                                       av, ac);

  XtAddCallback(CopyImageToggle, XmNvalueChangedCallback,
				fe_image_copy_cb, (XtPointer)w_data);

  fe_register_dependent(w_data->properties,
						CopyImageToggle, 
						FE_MAKE_DEPENDENCY(PROP_IMAGE_COPY),
						fe_image_copy_update_cb, (XtPointer)w_data);

  XtManageChildren (kids, i);

  /**********************************************************************/
  /*  Define the Dimensions Frame and its form	4MAR96RCJ		*/
  /**********************************************************************/
  ac = 0;
  XtSetArg (av [ac], XmNleftAttachment,   XmATTACH_FORM);   ac++;
  XtSetArg (av [ac], XmNtopAttachment,    XmATTACH_WIDGET); ac++;
  XtSetArg (av [ac], XmNtopWidget,        AlignFrame);      ac++;
  XtSetArg (av [ac], XmNbottomAttachment, XmATTACH_WIDGET); ac++;
  XtSetArg (av [ac], XmNbottomWidget,     RemoveImageMap);  ac++;
  DimensionsFrame = fe_CreateFrame(form, "dimensions", av, ac);

  ac = 0;
  XtSetArg (av [ac], XmNleftAttachment,   XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNrightAttachment,  XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNtopAttachment,    XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNbottomAttachment, XmATTACH_FORM); ac++;
  DimensionsForm = XmCreateForm (DimensionsFrame, 
                                 "dimensionsForm", av, ac);
  i  = 0;

  ac = 0;
  XtSetArg (av [ac], XmNleftAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNtopAttachment,  XmATTACH_FORM); ac++;
  kids [i++] = HeightLabel = XmCreateLabelGadget (DimensionsForm, 
                                                 "heightLabel",
                                                 av, ac);

  XtVaGetValues(HeightLabel,XmNwidth,&wide,NULL);

  ac = 0;
  XtSetArg (av [ac], XmNrightAttachment, XmATTACH_FORM);  ac++;
  XtSetArg (av [ac], XmNtopAttachment,  XmATTACH_FORM);   ac++;
  kids [i++] = heightPixels = XmCreateLabelGadget (DimensionsForm, /* 17MAR96RCJ */
                                                   "pixels",
                                                   av, ac);
  ac = 0;
  XtSetArg (av [ac], XmNtopAttachment,   XmATTACH_FORM);   ac++;
  XtSetArg (av [ac], XmNleftAttachment,  XmATTACH_WIDGET); ac++;
  XtSetArg (av [ac], XmNleftWidget,      HeightLabel);     ac++;
  XtSetArg (av [ac], XmNrightAttachment, XmATTACH_WIDGET); ac++;
  XtSetArg (av [ac], XmNrightWidget,     heightPixels);    ac++;
  XtSetArg (av [ac], XmNcolumns,     3);    ac++;
  kids [i++] = HeightTextField = fe_CreateTextField (DimensionsForm,
                                                     "imageHeight",
                                                     av, ac);
  w_data->image_height = HeightTextField;

  XtAddCallback(HeightTextField, XmNvalueChangedCallback,
				fe_image_dimensions_cb, (XtPointer)w_data);
  fe_register_dependent(w_data->properties, HeightTextField,
						FE_MAKE_DEPENDENCY(PROP_IMAGE_HEIGHT),
						fe_image_dimensions_height_update_cb, (XtPointer)w_data);

  ac = 0;
  XtSetArg (av [ac], XmNleftAttachment, XmATTACH_FORM);   ac++;
  XtSetArg (av [ac], XmNtopAttachment,  XmATTACH_WIDGET); ac++;
  XtSetArg (av [ac], XmNtopWidget,      HeightTextField); ac++;
  XtSetArg (av [ac], XmNwidth,          wide);            ac++;
  kids [i++] = WidthLabel = XmCreateLabelGadget (DimensionsForm, 
                                                 "widthLabel",
                                                 av, ac);
  ac = 0;
  XtSetArg (av [ac], XmNrightAttachment, XmATTACH_FORM);   ac++;
  XtSetArg (av [ac], XmNtopAttachment,   XmATTACH_WIDGET); ac++;
  XtSetArg (av [ac], XmNtopWidget,       HeightTextField); ac++;
  kids [i++] = widthPixels = XmCreateLabelGadget (DimensionsForm,
                                                   "pixels",
                                                   av, ac);
  ac = 0;
  XtSetArg (av [ac], XmNtopAttachment,   XmATTACH_WIDGET); ac++;
  XtSetArg (av [ac], XmNtopWidget,       HeightTextField); ac++;
  XtSetArg (av [ac], XmNleftAttachment,  XmATTACH_WIDGET); ac++;
  XtSetArg (av [ac], XmNleftWidget,      WidthLabel);      ac++;
  XtSetArg (av [ac], XmNrightAttachment, XmATTACH_WIDGET); ac++;
  XtSetArg (av [ac], XmNrightWidget,     widthPixels);    ac++;
  XtSetArg (av [ac], XmNcolumns,     3);    ac++;
  kids [i++] = WidthTextField = fe_CreateTextField (DimensionsForm,
                                                    "imageWidth",
                                                    av, ac);
  w_data->image_width = WidthTextField;

  XtAddCallback(WidthTextField, XmNvalueChangedCallback,
				fe_image_dimensions_cb, (XtPointer)w_data);
  fe_register_dependent(w_data->properties, WidthTextField,
						FE_MAKE_DEPENDENCY(PROP_IMAGE_WIDTH),
						fe_image_dimensions_width_update_cb, (XtPointer)w_data);

  ac = 0;
  XtSetArg (av [ac], XmNtopAttachment,    XmATTACH_WIDGET); ac++;
  XtSetArg (av [ac], XmNtopWidget,        WidthTextField);  ac++;
  XtSetArg (av [ac], XmNtopOffset,        5);               ac++;
  XtSetArg (av [ac], XmNleftAttachment,   XmATTACH_FORM);   ac++;
  kids [i++] = OriginalSize = XmCreatePushButtonGadget (DimensionsForm,
                                                        "originalSize",
                                                        av, ac);
  XtAddCallback(OriginalSize, XmNactivateCallback,
				fe_image_original_size_cb, (XtPointer)w_data);
  fe_register_dependent(w_data->properties, OriginalSize,
						FE_MAKE_DEPENDENCY(PROP_IMAGE_COPY),
						fe_image_original_size_update_cb, (XtPointer)w_data);

  XtManageChildren (kids, i);
  /**********************************************************************/
  /*  Create Space Around Image Frame, Form, Contents 6MAR96RCJ		*/
  /**********************************************************************/
  ac = 0;
  i  = 0;
  XtSetArg (av [ac], XmNleftAttachment,   XmATTACH_WIDGET); ac++;
  XtSetArg (av [ac], XmNleftWidget,       DimensionsFrame); ac++;
  XtSetArg (av [ac], XmNrightAttachment,  XmATTACH_FORM);   ac++;
  XtSetArg (av [ac], XmNtopAttachment,    XmATTACH_WIDGET); ac++;
  XtSetArg (av [ac], XmNtopWidget,        AlignFrame);      ac++;
  XtSetArg (av [ac], XmNbottomAttachment, XmATTACH_WIDGET); ac++;
  XtSetArg (av [ac], XmNbottomWidget,     RemoveImageMap);  ac++;
  ImageSpaceFrame = fe_CreateFrame(form, "imageSpace", av, ac);

  ac = 0;
  XtSetArg (av [ac], XmNleftAttachment,   XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNrightAttachment,  XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNtopAttachment,    XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNbottomAttachment, XmATTACH_FORM); ac++;
  ImageSpaceForm = XmCreateForm (ImageSpaceFrame, 
                                 "imageSpaceForm", av, ac);

  ac = 0;
  XtSetArg (av [ac], XmNleftAttachment, XmATTACH_FORM);    ac++;
  XtSetArg (av [ac], XmNtopAttachment,  XmATTACH_FORM);    ac++;
  XtSetArg (av [ac], XmNalignment, XmALIGNMENT_BEGINNING); ac++;
  kids [i++] = LeftRightLabel = XmCreateLabelGadget (ImageSpaceForm, 
                                                 "leftRightLabel",
                                                 av, ac);
  ac = 0;
  XtSetArg (av [ac], XmNleftAttachment, XmATTACH_FORM);   ac++;
  kids [i++] = TopBottomLabel = XmCreateLabelGadget (ImageSpaceForm, 
                                                 "topBottomLabel",
                                                 av, ac);

  XtVaGetValues(TopBottomLabel, XmNwidth,&wide,NULL);
  XtVaSetValues(LeftRightLabel, XmNwidth, wide,NULL);

  ac = 0;
  XtSetArg (av [ac], XmNrightAttachment, XmATTACH_FORM);  ac++;
  XtSetArg (av [ac], XmNtopAttachment,   XmATTACH_FORM);  ac++;
  kids [i++] = LeftPixels = XmCreateLabelGadget (ImageSpaceForm, 
                                                 "pixels",
                                                 av, ac);
  ac = 0;
  XtSetArg (av [ac], XmNtopAttachment,   XmATTACH_FORM);   ac++;
  XtSetArg (av [ac], XmNleftAttachment,  XmATTACH_WIDGET); ac++;
  XtSetArg (av [ac], XmNleftWidget,      LeftRightLabel);  ac++;
  XtSetArg (av [ac], XmNrightAttachment, XmATTACH_WIDGET); ac++;
  XtSetArg (av [ac], XmNrightWidget,     LeftPixels);   ac++;
  XtSetArg (av [ac], XmNcolumns,     3);    ac++;
  kids [i++] = LeftTextField = fe_CreateTextField (ImageSpaceForm,
                                                   "spaceWidth",
                                                   av, ac);
  w_data->margin_width = LeftTextField;

  XtAddCallback(LeftTextField, XmNvalueChangedCallback,
				fe_image_margin_cb, (XtPointer)w_data);
  fe_register_dependent(w_data->properties, LeftTextField,
						FE_MAKE_DEPENDENCY(PROP_IMAGE_MARGIN_WIDTH),
						fe_image_margin_width_update_cb, (XtPointer)w_data);

  XtVaSetValues(TopBottomLabel,
                XmNtopAttachment,  XmATTACH_WIDGET,
                XmNtopWidget,      LeftTextField,
                NULL);
  ac = 0;
  XtSetArg (av [ac], XmNtopAttachment,   XmATTACH_WIDGET); ac++;
  XtSetArg (av [ac], XmNtopWidget,       LeftTextField);   ac++;
  XtSetArg (av [ac], XmNleftAttachment,  XmATTACH_WIDGET); ac++;
  XtSetArg (av [ac], XmNleftWidget,      TopBottomLabel);  ac++;
  XtSetArg (av [ac], XmNrightAttachment, XmATTACH_WIDGET); ac++;
  XtSetArg (av [ac], XmNrightWidget,     LeftPixels);      ac++;
  XtSetArg (av [ac], XmNcolumns,     3);    ac++;
  kids [i++] = TopTextField = fe_CreateTextField (ImageSpaceForm,
                                                  "spaceHeight",
                                                  av, ac);
  w_data->margin_height = TopTextField;
  XtAddCallback(TopTextField, XmNvalueChangedCallback,
				fe_image_margin_cb, (XtPointer)w_data);
  fe_register_dependent(w_data->properties, TopTextField,
						FE_MAKE_DEPENDENCY(PROP_IMAGE_MARGIN_HEIGHT),
						fe_image_margin_height_update_cb, (XtPointer)w_data);

  ac = 0;
  XtSetArg (av [ac], XmNleftAttachment, XmATTACH_FORM);   ac++;
  XtSetArg (av [ac], XmNtopAttachment,  XmATTACH_WIDGET); ac++;
  XtSetArg (av [ac], XmNtopWidget,      TopTextField);    ac++;
  kids [i++] = SolidBorderLabel = XmCreateLabelGadget (ImageSpaceForm, 
                                                 "solidBorderLabel",
                                                 av, ac);
  ac = 0;
  XtSetArg (av [ac], XmNrightAttachment, XmATTACH_FORM);   ac++;
  XtSetArg (av [ac], XmNtopAttachment,   XmATTACH_WIDGET); ac++;
  XtSetArg (av [ac], XmNtopWidget,       LeftTextField);   ac++;
  kids [i++] = TopPixels = XmCreateLabelGadget (ImageSpaceForm, 
                                                 "pixels",
                                                 av, ac);
  ac = 0;
  XtSetArg (av [ac], XmNrightAttachment, XmATTACH_FORM);   ac++;
  XtSetArg (av [ac], XmNtopAttachment,   XmATTACH_WIDGET); ac++;
  XtSetArg (av [ac], XmNtopWidget,       TopTextField);    ac++;
  kids [i++] = SolidPixels = XmCreateLabelGadget (ImageSpaceForm, 
                                                 "pixels",
                                                 av, ac);
  ac = 0;
  XtSetArg (av [ac], XmNtopAttachment,   XmATTACH_WIDGET); ac++;
  XtSetArg (av [ac], XmNtopWidget,       TopTextField);    ac++;
  XtSetArg (av [ac], XmNleftAttachment,  XmATTACH_WIDGET); ac++;
  XtSetArg (av [ac], XmNleftWidget,      TopBottomLabel);  ac++;
  XtSetArg (av [ac], XmNrightAttachment, XmATTACH_WIDGET); ac++;
  XtSetArg (av [ac], XmNrightWidget,     LeftPixels);      ac++;
  XtSetArg (av [ac], XmNcolumns,     3);    ac++;
  kids [i++] = SolidTextField = fe_CreateTextField (ImageSpaceForm,
                                                    "spaceBorder",
                                                    av, ac);
  w_data->margin_solid = SolidTextField;
  XtAddCallback(SolidTextField, XmNvalueChangedCallback,
				fe_image_margin_cb, (XtPointer)w_data);
  /*
   *    Note border is dependent on link href, as it effects the
   *    default value.
   */
  fe_register_dependent(w_data->properties, SolidTextField,
				FE_MAKE_DEPENDENCY(PROP_LINK_HREF|PROP_IMAGE_MARGIN_BORDER),
				fe_image_margin_border_update_cb, (XtPointer)w_data);

  XtManageChildren (kids, i);

  /*
   *    Add this last, as other update methoda are dependent on it
   *    having run during init.
   */
  fe_register_dependent(w_data->properties, ImageFileTextField,
						FE_MAKE_DEPENDENCY(PROP_IMAGE_MAIN_IMAGE),
						fe_image_main_image_update_cb, (XtPointer)w_data);

  /**********************************************************************/
  /*  Display form by managing all of the Manager widgets 6MAR96RCJ	*/
  /**********************************************************************/
  XtManageChild(ImageSpaceForm);
  XtManageChild(ImageSpaceFrame);
  XtManageChild(ImageFileForm);
  XtManageChild(ImageFileFrame);
  XtManageChild(AlignForm);
  XtManageChild(AlignFrame);
  XtManageChild(DimensionsForm);
  XtManageChild(DimensionsFrame);
  XtManageChild (form);
} /* end fe_make_image_page */

Widget
fe_EditorPropertiesDialogCreate(
								MWContext *context, 
								fe_EditorPropertiesWidgets* p_data,
								Boolean is_image
)
{
	Widget   dialog;
	Widget   form;
	Widget   tab_form;
	char*    name = (is_image)?	"imagePropertiesDialog":
		                        "textPropertiesDialog";
	
	/*
	 *    Make prompt with ok, apply, cancel, no separator.
	 */
	dialog = fe_CreatePromptDialog(context, name,
								   TRUE, TRUE, TRUE, FALSE, TRUE);

	form = XtVaCreateManagedWidget(
								   "folder",
								   xmlFolderWidgetClass, dialog,
								   XmNshadowThickness, 2,
								   XmNtopAttachment, XmATTACH_FORM,
								   XmNleftAttachment, XmATTACH_FORM,
								   XmNrightAttachment, XmATTACH_FORM,
								   XmNbottomAttachment, XmATTACH_FORM,
#ifdef ALLOW_TAB_ROTATE
								   XmNtabPlacement, XmFOLDER_LEFT,
								   XmNrotateWhenLeftRight, FALSE,
#endif /* ALLOW_TAB_ROTATE */
								   XmNbottomOffset, 3,
								   XmNspacing, 1,
								   NULL);

	if (is_image) {
		tab_form = fe_CreateTabForm(form, "imageProperties", NULL, 0);
		fe_make_image_page(context, tab_form, p_data->image);
	} else {
		tab_form = fe_CreateTabForm(form, "characterProperties", NULL, 0);
		fe_make_character_page(context, tab_form, p_data->character);
	}

	tab_form = fe_CreateTabForm(form, "linkProperties", NULL, 0);
	fe_make_link_page(context, tab_form, p_data->link);

	tab_form = fe_CreateTabForm(form, "paragraphProperties", NULL, 0);
	fe_make_paragraph_page(context, tab_form, p_data->paragraph);

	XtManageChild(dialog);

	return form;
}

void
fe_EditorPropertiesDialogDo(MWContext* context,
								fe_EditorPropertiesDialogType tab_type)
{
	fe_EditorPropertiesWidgets          properties;
	fe_EditorParagraphPropertiesWidgets paragraph;
	fe_EditorLinkPropertiesWidgets      link;
	fe_EditorCharacterPropertiesWidgets character;
	fe_EditorImagePropertiesWidgets     image;
	int done;
    Widget dialog;
    Widget form;
	Widget apply_button;
	Boolean apply_sensitized;
	unsigned tab_number;
	Boolean is_image;

	is_image = fe_EditorPropertiesDialogCanDo(context,
											  XFE_EDITOR_PROPERTIES_IMAGE);

	/*
	 *    Pick the tab.
	 */
	switch (tab_type) {
	case XFE_EDITOR_PROPERTIES_TABLE:
		fe_EditorTablePropertiesDialogDo(context, 
						 XFE_EDITOR_PROPERTIES_TABLE);
		return;
	case XFE_EDITOR_PROPERTIES_IMAGE:
		if (!is_image)
			return;
		/*FALLTHRU*/
	case XFE_EDITOR_PROPERTIES_IMAGE_INSERT:
		is_image = TRUE;
		tab_number = 0;
		break;
	case XFE_EDITOR_PROPERTIES_CHARACTER:
		is_image = FALSE;
		tab_number = 0;
		break;
	case XFE_EDITOR_PROPERTIES_LINK_INSERT:
	case XFE_EDITOR_PROPERTIES_LINK:     
		tab_number = 1;
		break;
	case XFE_EDITOR_PROPERTIES_PARAGRAPH:
		tab_number = 2;
		break;
	case XFE_EDITOR_PROPERTIES_HRULE:
		fe_EditorHorizontalRulePropertiesDialogDo(context);
		return;
	default:
		return;
	}

	memset(&properties, 0, sizeof(fe_EditorPropertiesWidgets));
	memset(&paragraph, 0, sizeof(fe_EditorParagraphPropertiesWidgets));
	memset(&link, 0, sizeof(fe_EditorLinkPropertiesWidgets));
	memset(&character, 0, sizeof(fe_EditorCharacterPropertiesWidgets));
	memset(&image, 0, sizeof(fe_EditorImagePropertiesWidgets));

	/*
	 *    I'll show you mine if you show me yours.
	 */
	link.properties = &properties;
	paragraph.properties = &properties;
	image.properties = &properties;
	character.properties = &properties;

	properties.link = &link;
	properties.paragraph = &paragraph;
	if (is_image)
		properties.image = &image;
	else 
		properties.character = &character;

	properties.context = context;
	form = fe_EditorPropertiesDialogCreate(context, &properties, is_image);
	dialog = XtParent(form);

	/*
	 *   Add a bunch of callbacks to the buttons.
	 */
	XtAddCallback(dialog, XmNokCallback, fe_hrule_ok_cb, &done);
	XtAddCallback(dialog, XmNapplyCallback, fe_hrule_apply_cb, &done);
	XtAddCallback(dialog, XmNcancelCallback, fe_hrule_cancel_cb, &done);
	XtAddCallback(dialog, XmNdestroyCallback, fe_hrule_destroy_cb, &done);

    /*
     *    Load values.
     */
	if (is_image)
		fe_EditorImagePropertiesDialogDataGet(context, &image);
	else
		fe_EditorCharacterPropertiesDialogDataGet(context, &character);
	fe_editor_link_properties_dialog_data_init(context, &link);
	fe_EditorParagraphPropertiesDialogDataGet(context, &paragraph);

	/*
	 *    We toggle the sensitivity of the apply button on/off
	 *    depending if there are changes to apply. It would be
	 *    nice to use the depdency meahcnism, but it might get
	 *    very busy.
	 */
	apply_button = XmSelectionBoxGetChild(dialog, XmDIALOG_APPLY_BUTTON);
	XtVaSetValues(apply_button, XmNsensitive, FALSE, 0);
	apply_sensitized = FALSE;
	properties.changed = 0;

    /*
     *    Popup.
     */
    XtManageChild(form);

	XmLFolderSetActiveTab(form, tab_number, True);

	/*
     *    Wait.
     */
	fe_NukeBackingStore(dialog); /* what does this do? */

	done = XmDIALOG_NONE;
	EDT_BeginBatchChanges(context);
	while (done == XmDIALOG_NONE) {

		Boolean new_image_override = FALSE;

		fe_EventLoop();
		
		if (done == XFE_DIALOG_DESTROY_BUTTON||done == XmDIALOG_CANCEL_BUTTON)
			break;

		/*
		 *    This is a horrible crock to get around the fact
		 *    that imageinsert() moves the context away from
		 *    the image. Better solutions???
		 */
		new_image_override = (properties.image && image.new_image);
		
		if (new_image_override) {
			if (apply_sensitized == TRUE) {
				XtVaSetValues(apply_button, XmNsensitive, FALSE, 0);
				apply_sensitized = TRUE;
			}
		} else {
			if (apply_sensitized == FALSE && properties.changed != 0) {
				XtVaSetValues(apply_button, XmNsensitive, TRUE, 0);
				apply_sensitized = TRUE;
			}
		}

		if (done == XmDIALOG_APPLY_BUTTON || done == XmDIALOG_OK_BUTTON) {
			/* apply */

			if ((properties.changed & PROP_IMAGE_ALL) != 0
				&&
				!fe_editor_image_properties_validate(context, &image)) {
				done = XmDIALOG_NONE;
				continue;
			}

			if (properties.changed != 0) {
				if ((properties.changed & PROP_CHAR_ALL) != 0) 
					fe_EditorCharacterPropertiesDialogSet(context, &character);
				if ((properties.changed & PROP_IMAGE_ALL) != 0)
					fe_editor_image_properties_set(context, &image);
				if ((properties.changed & PROP_LINK_ALL) != 0)
					fe_editor_link_properties_dialog_set(context, &link);
				if ((properties.changed & PROP_PARA_ALL) != 0) 
					fe_EditorParagraphPropertiesDialogSet(context, &paragraph);
			}

			if (done == XmDIALOG_APPLY_BUTTON) {
				properties.changed = 0;
				XtVaSetValues(apply_button, XmNsensitive, FALSE, 0);
				apply_sensitized = FALSE;
				done = XmDIALOG_NONE; /* keep looping */
			}
		}
	}
	EDT_EndBatchChanges(context);

    /*
     *    Unload data.
     */
	fe_DependentListDestroy(properties.dependents);

	if (done != XFE_DIALOG_DESTROY_BUTTON)
		XtDestroyWidget(dialog);
}

void
fe_EditorSetColorsDialogDo(MWContext* context)
{
	LO_Color color;
	
	fe_EditorColorGet(context, &color);

	if (fe_ColorPicker(context, &color))
		fe_EditorColorSet(context, &color);
}


static Widget
fe_GetBiggestWidget(Boolean horizontal, Widget* widgets, Cardinal n)
{
	Dimension max_width = 0;
	Dimension width;
	Widget max_widget = NULL;
	Widget widget;
	Cardinal i;
	
	if (!widgets)
		return NULL;

	for (i = 0; i < n; i++) {

		widget = widgets[i];

		if (horizontal) {
			XtVaGetValues(widget, XmNwidth, &width, 0);
		} else {
			XtVaGetValues(widget, XmNheight, &width, 0);
		}
		if (width > max_width) {
			max_width = width;
			max_widget = widget;
		}
	}
	
	return max_widget;
}

Widget
fe_CreateCombo(Widget parent, char* name, Arg* p_args, Cardinal p_n)
{
  Widget pulldown;
  Arg args[8];
  Cardinal n;
  Widget button;
  char buf[64];

  sprintf(buf, "%sMenu", name);

  n = 0;
  pulldown = fe_CreatePulldownMenu(parent, buf, args, n);

  n = 0;
  button = XmCreatePushButtonGadget(pulldown, "emptyList", args, n);
  XtManageChild(button);

  n = 0;
  XtSetArg(args[n], XmNsubMenuId, pulldown); n++;

  return fe_CreateOptionMenuNoLabel(parent, name, args, n);
}

struct fe_EditorDocumentPropertiesStruct;

typedef struct fe_EditorDocumentAppearancePropertiesStruct
{
    fe_DependentList* dependents;
    MWContext* context;

    EDT_PageData page_data;
    LO_Color colors[5];         /* customized by user */
    LO_Color default_colors[5]; /* cache of defaults */
    Boolean  use_custom;
    Boolean  use_image;
    char*    image_path;
    Widget   image_text;
	unsigned changed;
    Boolean  is_editor_preferences;

} fe_EditorDocumentAppearancePropertiesStruct;

typedef struct fe_EditorDocumentGeneralPropertiesStruct
{
	struct fe_EditorDocumentPropertiesStruct* properties;
	
	Widget location;
	Widget title;
	Widget author;
	Widget description;
#ifdef EDITOR_SHOW_CREATE_DATE
	Widget created;
	Widget updated;
#endif /*EDITOR_SHOW_CREATE_DATE*/
	Widget keywords;
	Widget classification;

	unsigned changed;
	
} fe_EditorDocumentGeneralPropertiesStruct;

#define DOCUMENT_NORMAL_TEXT_COLOR   (0)
#define DOCUMENT_LINK_TEXT_COLOR     (1)
#define DOCUMENT_ACTIVE_TEXT_COLOR   (2)
#define DOCUMENT_FOLLOWED_TEXT_COLOR (3)
#define DOCUMENT_BACKGROUND_COLOR    (4)

#define DOCUMENT_NORMAL_TEXT_COLOR_MASK   (0x1<<(DOCUMENT_NORMAL_TEXT_COLOR))
#define DOCUMENT_LINK_TEXT_COLOR_MASK     (0x1<<(DOCUMENT_LINK_TEXT_COLOR))
#define DOCUMENT_ACTIVE_TEXT_COLOR_MASK   (0x1<<(DOCUMENT_ACTIVE_TEXT_COLOR))
#define DOCUMENT_FOLLOWED_TEXT_COLOR_MASK (0x1<<(DOCUMENT_FOLLOWED_TEXT_COLOR))
#define DOCUMENT_BACKGROUND_COLOR_MASK    (0x1<<(DOCUMENT_BACKGROUND_COLOR))
#define DOCUMENT_BACKGROUND_IMAGE_MASK    (0x1<<5)

#define DOCUMENT_USE_CUSTOM_MASK          (0x1<<6)
#define DOCUMENT_USE_IMAGE_MASK           (0x1<<7)

#define DOCUMENT_ALL_APPEARANCE                           \
(DOCUMENT_BACKGROUND_COLOR_MASK|DOCUMENT_LINK_TEXT_COLOR_MASK|      \
 DOCUMENT_NORMAL_TEXT_COLOR_MASK|DOCUMENT_FOLLOWED_TEXT_COLOR_MASK| \
 DOCUMENT_ACTIVE_TEXT_COLOR_MASK|DOCUMENT_BACKGROUND_IMAGE_MASK|    \
 DOCUMENT_USE_CUSTOM_MASK|DOCUMENT_USE_IMAGE_MASK)

#if 0
static LO_Color**
fe_document_appearance_color_configure(
			   fe_EditorDocumentAppearancePropertiesStruct* w_data,
			   ED_EColor  c_type,
			   LO_Color** def_color_r)
{
	LO_Color** result_addr;

	*def_color_r = &w_data->colors[c_type];

	switch (c_type) {
	case DOCUMENT_BACKGROUND_COLOR:
	  result_addr = &w_data->page_data.pColorBackground;
	  break;
	case DOCUMENT_LINK_TEXT_COLOR:
	  result_addr = &w_data->page_data.pColorLink;
	  break;
	case DOCUMENT_NORMAL_TEXT_COLOR:
	  result_addr = &w_data->page_data.pColorText;
	  break;
	case DOCUMENT_FOLLOWED_TEXT_COLOR:
	  result_addr = &w_data->page_data.pColorFollowedLink;
	  break;
	default:
	  result_addr = &w_data->page_data.pColorActiveLink;
	  break;
	}

	return result_addr;
}
#endif

#if 0

static void
fe_PreviewPanelSetColors(Widget widget,
						 MWContext* context,
						 unsigned mask,
						 Pixmap bg_pixmap,
						 LO_Color* colors)
{
    LO_Color* color;
	Pixel pixel;
	Pixel bg_pixel;
	WidgetList children;
	Cardinal num_children;
	Arg args[2];
	Cardinal n;
	int color_n;
	Boolean set_bg = FALSE;
	Boolean set_fg;

	XtVaGetValues(widget,
				  XmNchildren, &children,
				  XmNnumChildren, &num_children,
				  0);

	if ((mask & DOCUMENT_USE_CUSTOM_MASK) != 0)
	    mask = ~0; /* do everything */

	if ((mask & DOCUMENT_BACKGROUND_COLOR_MASK) != 0) {
	    color = &colors[DOCUMENT_BACKGROUND_COLOR];

		bg_pixel = fe_GetPixel(context,
							color->red,
							color->green,
							color->blue);

		XtVaSetValues(widget, XmNbackground, bg_pixel, 0);
		set_bg = TRUE;
	}

	for (color_n = 0; color_n < 4; color_n++) {
  	    n = 0;
		set_fg = FALSE;
	    if ((mask & (1<<color_n)) != 0) {
		    color = &colors[color_n];
			pixel = fe_GetPixel(context,color->red,color->green,color->blue);
			XtSetArg(args[n], XmNforeground, pixel); n++;
			set_fg = TRUE;
		}
		if (set_bg || set_fg) {
		    if (set_bg)
			    XtSetArg(args[n], XmNbackground, bg_pixel); n++;
		    XtSetValues(children[color_n], args, n);
		}
	}
}

static char* fe_PreviewPanelCreate_names[] =
{ "normal", "link", "active", "followed", 0 };

static Widget
fe_PreviewPanelCreate(Widget parent, char* name, Arg* p_args, Cardinal p_n)
{
	Widget rc;
	Widget children[4];
	Cardinal nchildren;
	Arg args[8];
	Cardinal n;
	
	XtSetArg(p_args[p_n], XmNorientation, XmVERTICAL); n++;
	rc = XmCreateRowColumn(parent, name, p_args, p_n);

	for (nchildren = 0; nchildren < 4; nchildren++) {
	  name = fe_PreviewPanelCreate_names[nchildren];
	  n = 0;
	  children[nchildren] = XmCreateLabel(rc, name, args, n);
	}

	XtManageChildren(children, nchildren);

	return rc;
}

#else

typedef struct fe_PreviewPanelStruct
{
    Pixmap     bg_pixmap;
    Pixel      bg_pixel;
    Pixel      string_pixels[4];
    XmString   strings[4];
    XmFontList fontList;
    Dimension  margin_height;
    Dimension  margin_width;
    Dimension  height;
    Dimension  width;
	Dimension  shadow_thickness;
	Dimension  highlight_thickness;
	Dimension  spacing;
} fe_PreviewPanelStruct;

GC
fe_GetGCfromDW(Display* dpy, Window win, unsigned long flags, XGCValues *gcv);

static void
fe_preview_draw_string(Display* display, Drawable window,
					   Pixel bg, Pixel fg,
					   XmFontList fontList, XmString string,
					   Position x, Position y,
					   Dimension width,	Dimension height,
					   Boolean underline)
{
	XGCValues    gc_values;
	XtGCMask     gc_mask;
    GC           gc;
	XRectangle   clip_rect;
	XFontStruct* font = NULL;

	memset (&gc_values, ~0, sizeof (gc_values));
	gc_values.foreground = fg;
	gc_values.background = bg;

	_XmFontListGetDefaultFont(fontList, &font); /* must include XmP.h */

	gc_mask = GCForeground|GCBackground;

	if (font != NULL) {
		gc_values.font = font->fid;
		gc_mask |= GCFont;
	}

	gc = fe_GetGCfromDW(display, window, gc_mask, &gc_values);

	clip_rect.x = x;
	clip_rect.y = y;
	clip_rect.width = width;
	clip_rect.height = height;

	if (underline) {
	  XmStringDrawUnderline(display, window, fontList,
							string,
							gc,
							x, y, width,
							XmALIGNMENT_BEGINNING, 0, /* layout direction */
							&clip_rect, /*clip_rectangle*/
							string);
	} else {
	  XmStringDraw(display, window, fontList,
				   string,
				   gc,
				   x, y, width,
				   XmALIGNMENT_BEGINNING, 0, /* layout direction */
				   &clip_rect /*clip_rectangle*/);
	}
}
					   
static void
fe_preview_panel_paint(Display* display, Drawable window, 
					   fe_PreviewPanelStruct* stuff)
{
	Position  x;
	Position  y;
	Dimension height;
	Dimension width;
	Dimension height_delta;
	Dimension margin_height;
	XmString  string;
	Pixel     fg_pixel;
	int i;

	/* use background color instead */
	XClearArea(display, window, 0, 0, 0, 0, FALSE); /* no expose */

	for (height = stuff->height; (height % 4) != 0; height++)
		;
	height_delta = height/4;

	margin_height = stuff->highlight_thickness +
		            stuff->shadow_thickness +
		            stuff->margin_height;
	x = stuff->highlight_thickness +
		stuff->shadow_thickness +
		stuff->margin_width;
	y = margin_height;

	for (i = DOCUMENT_NORMAL_TEXT_COLOR;
		 i <= DOCUMENT_FOLLOWED_TEXT_COLOR;
		 i++) {

	    fg_pixel = stuff->string_pixels[i];
		string = stuff->strings[i];

		XmStringExtent(stuff->fontList, string, &width, &height);

		fe_preview_draw_string(display, window,
							   stuff->bg_pixel, fg_pixel,
							   stuff->fontList, string,
							   x, y, width, height,
							   (i != DOCUMENT_NORMAL_TEXT_COLOR));
		y += height + (2*margin_height) + stuff->spacing; /* note: spacing */
	}
}

static void
fe_preview_panel_expose_cb(Widget widget, XtPointer closure, XtPointer cb)
{
    XmDrawingAreaCallbackStruct* cb_data = (XmDrawingAreaCallbackStruct*)cb;
	fe_PreviewPanelStruct* stuff = 
	  (fe_PreviewPanelStruct*)fe_GetUserData(widget);

	fe_preview_panel_paint(XtDisplay(widget), cb_data->window, stuff);
}

static void
fe_PreviewPanelSetColors(Widget widget,
						 MWContext* context,
						 unsigned mask,
						 Pixmap bg_pixmap,
						 LO_Color* colors)
{
	fe_PreviewPanelStruct* stuff = 
	  (fe_PreviewPanelStruct*)fe_GetUserData(widget);
    LO_Color* color;
	Pixel pixel;
	int color_n;
	Boolean set_bg = FALSE;
	Boolean set_fg;

	set_fg = FALSE;

	if ((mask & DOCUMENT_BACKGROUND_COLOR_MASK) != 0) {
	    color = &colors[DOCUMENT_BACKGROUND_COLOR];

		pixel = fe_GetPixel(context,
							color->red,
							color->green,
							color->blue);
		if (pixel != stuff->bg_pixel) {
			stuff->bg_pixel = pixel;
			/* this will generate an expose event -> cb */
			XtVaSetValues(widget, XmNbackground, stuff->bg_pixel, 0);
			set_bg = TRUE;
		}
	}

	for (color_n = 0; color_n < 4; color_n++) {
	    if ((mask & (1<<color_n)) != 0) {
		    color = &colors[color_n];
			pixel = fe_GetPixel(context,color->red,color->green,color->blue);
			stuff->string_pixels[color_n] = pixel;
			set_fg = TRUE;
		}
	}

	if (set_fg && !set_bg) /* if we set bg, we'll get an expose anyway */
	  fe_preview_panel_paint(XtDisplay(widget), XtWindow(widget), stuff);
}

static char* fe_PreviewPanelCreate_names[] =
{ "normal", "link", "active", "followed", "background", 0 };

static XtResource fe_PreviewPanelResources [] =
{
  {
	"normalLabelString", XmCXmString, XmRXmString, sizeof(XmString),
    XtOffset(fe_PreviewPanelStruct*, strings[DOCUMENT_NORMAL_TEXT_COLOR]),
	XmRImmediate,  (XtPointer)NULL
  },
  {
	"linkLabelString", XmCXmString, XmRXmString, sizeof(XmString),
    XtOffset(fe_PreviewPanelStruct*, strings[DOCUMENT_LINK_TEXT_COLOR]),
	XmRImmediate,  (XtPointer)NULL
  },
  {
	"activeLabelString", XmCXmString, XmRXmString, sizeof(XmString),
    XtOffset(fe_PreviewPanelStruct*, strings[DOCUMENT_ACTIVE_TEXT_COLOR]),
	XmRImmediate,  (XtPointer)NULL
  },
  {
	"followedLabelString", XmCXmString, XmRXmString, sizeof(XmString),
    XtOffset(fe_PreviewPanelStruct*, strings[DOCUMENT_FOLLOWED_TEXT_COLOR]),
	XmRImmediate,  (XtPointer)NULL
  },
  {
	XmNfontList, XmCFontList, XmRFontList, sizeof(XmFontList),
    XtOffset(fe_PreviewPanelStruct*, fontList),
	XmRImmediate, (XtPointer)NULL
  },
  {
	XmNmarginHeight, XmCMarginHeight, XmRVerticalDimension, sizeof(Dimension),
    XtOffset(fe_PreviewPanelStruct*, margin_height),
	XmRImmediate, (XtPointer)2
  },
  {
    XmNmarginWidth, XmCMarginWidth, XmRHorizontalDimension, sizeof(Dimension),
    XtOffset(fe_PreviewPanelStruct*, margin_width),
	XmRImmediate, (XtPointer)2
  },
  {
     XmNhighlightThickness, XmCHighlightThickness, XmRHorizontalDimension,
     sizeof (Dimension),
     XtOffset(fe_PreviewPanelStruct*, highlight_thickness),
     XmRImmediate, (XtPointer) 0 /* makes up for frame around us */
  },
  {
	 XmNshadowThickness, XmCShadowThickness, XmRHorizontalDimension,
     sizeof (Dimension), 
     XtOffset(fe_PreviewPanelStruct*, shadow_thickness),
     XmRImmediate, (XtPointer) 0
  },
  {
	 XmNspacing, XmCSpacing, XmRVerticalDimension,
     sizeof (Dimension), 
     XtOffset(fe_PreviewPanelStruct*, spacing),
     XmRImmediate, (XtPointer)4
  }
};

static Widget
fe_PreviewPanelCreate(Widget parent, char* name, Arg* p_args, Cardinal p_n)
{
	Widget drawing_area;
	XmString xm_string;
	char* resource_name;
	Pixel bg;
	XtResource copy_resources[XtNumber(fe_PreviewPanelResources)];
	Dimension height;
	Dimension width;
	Dimension total_height;
	Dimension max_width;
	fe_PreviewPanelStruct* stuff = XtNew(fe_PreviewPanelStruct);
	int i;
	
	XtSetArg(p_args[p_n], XmNuserData, stuff); p_n++;
	drawing_area = XmCreateDrawingArea(parent, name, p_args, p_n);

	XtVaGetValues(drawing_area,
				  XmNbackground, &bg,
				  XmNheight, &stuff->height,
				  XmNwidth, &stuff->width,
				  0);

	stuff->bg_pixel = bg;

	/*
	 *    Get resources.
	 */
	memcpy(copy_resources, fe_PreviewPanelResources,
		   sizeof(fe_PreviewPanelResources));
	XtGetSubresources(drawing_area, (XtPointer)stuff,
					  name, "NsPreviewPanel",
					  copy_resources,
					  XtNumber(fe_PreviewPanelResources),
					  NULL, 0);

	max_width = 0;
	total_height = 0;

	for (i = 0; i < 4; i++) {

	  if (stuff->strings[i] == NULL) { /* not set in resources */
		  resource_name = fe_PreviewPanelResources[i].resource_name;
		  xm_string = XmStringCreateLocalized(resource_name);
		  stuff->strings[i] = xm_string;
	  }

	  XmStringExtent(stuff->fontList, stuff->strings[i], &width, &height);
	  if (width > max_width)
		  max_width = width;
	  total_height += height;

	  stuff->string_pixels[i] = bg;
	}

	max_width += 2 * (stuff->highlight_thickness +
					  stuff->shadow_thickness + stuff->margin_width);
	if (stuff->width < max_width) {
		stuff->width = max_width;
		XtVaSetValues(drawing_area, XmNwidth, stuff->width, 0);
	}

	total_height += (2 * stuff->highlight_thickness) +
		            (8 * (stuff->shadow_thickness + stuff->margin_height)) +
		            (3 * stuff->spacing);
	if (stuff->height < total_height) {
		stuff->height = total_height;
		XtVaSetValues(drawing_area, XmNheight, stuff->height, 0);
	}

	XtAddCallback(drawing_area, XmNexposeCallback, fe_preview_panel_expose_cb,
				  (XtPointer)stuff);
	XtAddCallback(drawing_area, XmNdestroyCallback, fe_destroy_cleanup_cb,
				  (XtPointer)stuff);

	return drawing_area;
}
#endif

static void
fe_document_appearance_preview_update_cb(Widget widget, XtPointer closure,
								XtPointer cb_data)
{
    fe_EditorDocumentAppearancePropertiesStruct* w_data
	  = (fe_EditorDocumentAppearancePropertiesStruct*)closure;
	fe_DependentListCallbackStruct* cb = 
	  (fe_DependentListCallbackStruct*)cb_data;
    fe_Dependency mask = cb->mask;
	LO_Color* colors;

	if (w_data->use_custom)
	    colors = w_data->colors;
	else
	    colors = w_data->default_colors;

	if ((mask & DOCUMENT_USE_CUSTOM_MASK) != 0)
	    mask = ~0; /* do all colors */

	fe_PreviewPanelSetColors(widget,
							 w_data->context,
							 mask,
							 0, /*Pixmap bg_pixmap*/
							 colors);
}

static void
fe_document_appearance_swatch_update_cb(Widget widget, XtPointer closure,
								XtPointer cb_data)
{
	ED_EColor c_type = (ED_EColor)fe_GetUserData(widget);
    fe_EditorDocumentAppearancePropertiesStruct* w_data
	  = (fe_EditorDocumentAppearancePropertiesStruct*)closure;
	LO_Color* default_color;
	LO_Color* colors;
	Boolean sensitive;

	if (w_data->use_custom)
	    colors = w_data->colors;
	else
	    colors = w_data->default_colors;

	default_color = &colors[c_type];

	sensitive = w_data->use_custom;

	fe_SwatchSetColor(widget, default_color);
	fe_SwatchSetSensitive(widget, sensitive);
}

static void
fe_document_appearance_color_picker_do(
					   MWContext* context,
					   Widget widget,
					   fe_EditorDocumentAppearancePropertiesStruct* w_data,
					   unsigned which)
{
	LO_Color default_color;
	LO_Color* colors;
	fe_Dependency mask;

	if (w_data->use_custom)
	    colors = w_data->colors;
	else
	    colors = w_data->default_colors;

	default_color = colors[which];

	mask = (1 << which);

	if (fe_ColorPicker(w_data->context, &default_color)) {
		w_data->colors[which] = default_color;
		w_data->changed |= mask;
	    fe_DependentListCallDependents(widget,
									   w_data->dependents,
									   mask,
									   (XtPointer)w_data);
	}
}

static void
fe_document_appearance_color_cb(Widget widget, XtPointer closure,
								XtPointer cb_data)
{
    fe_Dependency mask;
	ED_EColor c_type = (ED_EColor)fe_GetUserData(widget);
    fe_EditorDocumentAppearancePropertiesStruct* w_data
	  = (fe_EditorDocumentAppearancePropertiesStruct*)closure;
	LO_Color default_color;
	LO_Color* colors;

	if (w_data->use_custom)
	    colors = w_data->colors;
	else
	    colors = w_data->default_colors;

	default_color = colors[c_type];

	mask = (1 << ((unsigned)c_type));

	if (fe_ColorPicker(w_data->context, &default_color)) {
		w_data->colors[c_type] = default_color;
		w_data->changed |= mask;
	    fe_DependentListCallDependents(widget,
									   w_data->dependents,
									   mask,
									   closure);
	}
}

static void
fe_preview_panel_click_cb(Widget widget, XtPointer closure, XtPointer cb)
{
    XmDrawingAreaCallbackStruct* cb_data = (XmDrawingAreaCallbackStruct*)cb;
    fe_EditorDocumentAppearancePropertiesStruct* w_data
	  = (fe_EditorDocumentAppearancePropertiesStruct*)closure;
	Dimension height;
	Dimension height_delta;
	int i;

	if (!w_data->use_custom) /* hack, hack, hack */
		return;

	if (cb_data->event->type == ButtonRelease) {

		unsigned y = cb_data->event->xbutton.y;

		XtVaGetValues(widget, XmNheight, &height, 0);

		for (; (height % 4) != 0; height++)
			;
		height_delta = height/4;

		for (i = 0 ; i < 3; i++) {
			if (y < ((i+1)*height_delta))
				break;
		}
		fe_document_appearance_color_picker_do(w_data->context,
											   widget, w_data, i);
	}
}


static void
fe_document_appearance_use_custom_update_cb(Widget widget, XtPointer closure,
									 XtPointer cb_data)
{
    fe_EditorDocumentAppearancePropertiesStruct* w_data
	  = (fe_EditorDocumentAppearancePropertiesStruct*)closure;
    Boolean is_custom_button = (fe_GetUserData(widget) != NULL);
	Boolean set = (is_custom_button == w_data->use_custom);

	XmToggleButtonGadgetSetState(widget, set, FALSE);
}

static void
fe_document_appearance_use_custom_cb(Widget widget, XtPointer closure,
									 XtPointer cb_data)
{
    fe_EditorDocumentAppearancePropertiesStruct* w_data
	  = (fe_EditorDocumentAppearancePropertiesStruct*)closure;
    Boolean is_custom_button = (fe_GetUserData(widget) != NULL);
	Boolean set = XmToggleButtonGadgetGetState(widget);

	w_data->use_custom = (set == is_custom_button);
	w_data->changed |= DOCUMENT_USE_CUSTOM_MASK;

	fe_DependentListCallDependents(widget,
								   w_data->dependents,
								   DOCUMENT_USE_CUSTOM_MASK,
								   closure);
}

static void
fe_document_appearance_use_image_update_cb(Widget widget, XtPointer closure,
									 XtPointer cb_data)
{
    fe_EditorDocumentAppearancePropertiesStruct* w_data
	  = (fe_EditorDocumentAppearancePropertiesStruct*)closure;
	
	XmToggleButtonGadgetSetState(widget, w_data->use_image, FALSE);
}

static void
fe_document_appearance_use_image_cb(Widget widget, XtPointer closure,
									 XtPointer cb_data)
{
    fe_EditorDocumentAppearancePropertiesStruct* w_data
	  = (fe_EditorDocumentAppearancePropertiesStruct*)closure;
	Boolean set = XmToggleButtonGadgetGetState(widget);

	w_data->use_image = set;
	w_data->changed |= DOCUMENT_USE_IMAGE_MASK;

	fe_DependentListCallDependents(widget,
								   w_data->dependents,
								   DOCUMENT_USE_IMAGE_MASK,
								   closure);
}

static void
fe_document_appearance_sensitized_update_cb(Widget widget, XtPointer closure,
									 XtPointer cb_data)
{
    fe_EditorDocumentAppearancePropertiesStruct* w_data
	  = (fe_EditorDocumentAppearancePropertiesStruct*)closure;
	Boolean sensitive;
	
	sensitive = w_data->use_custom;

	XtVaSetValues(widget, XmNsensitive, sensitive, 0);
}

static void
fe_document_appearance_image_text_cb(Widget widget, XtPointer closure,
									 XtPointer cb_data)
{
    fe_EditorDocumentAppearancePropertiesStruct* w_data
	  = (fe_EditorDocumentAppearancePropertiesStruct*)closure;

	w_data->changed |= DOCUMENT_BACKGROUND_IMAGE_MASK;
}

static void
fe_document_appearance_image_text_update_cb(Widget widget, XtPointer closure,
									 XtPointer cb_data)
{
    fe_EditorDocumentAppearancePropertiesStruct* w_data
	  = (fe_EditorDocumentAppearancePropertiesStruct*)closure;
	fe_DependentListCallbackStruct* info = 
		(fe_DependentListCallbackStruct*)cb_data;
	Boolean sensitive = w_data->use_image;

	if (info->caller == widget)
		return; /* don't call ourselves */
	
	if ((info->mask & DOCUMENT_BACKGROUND_IMAGE_MASK) != 0)
	  fe_set_text_field(widget, w_data->image_path,
						fe_document_appearance_image_text_cb, w_data);

	fe_TextFieldSetEditable(w_data->context, widget, sensitive);
}

static void
fe_document_appearance_image_text_browse_cb(Widget widget, XtPointer closure,
									 XtPointer cb_data)
{
    fe_EditorDocumentAppearancePropertiesStruct* w_data
	  = (fe_EditorDocumentAppearancePropertiesStruct*)closure;
	Widget image_text = (Widget)fe_GetUserData(widget);

	fe_editor_browse_file_of_text(w_data->context, image_text);
}

static void
fe_document_appearance_use_image_button_update_cb(Widget widget,
												   XtPointer closure,
												   XtPointer cb_data)
{
    fe_EditorDocumentAppearancePropertiesStruct* w_data
	  = (fe_EditorDocumentAppearancePropertiesStruct*)closure;
	Boolean sensitive = w_data->use_image;

	XtVaSetValues(widget, XmNsensitive, sensitive, 0);
}

static void
fe_document_appearance_set(
						  MWContext* context,
						  fe_EditorDocumentAppearancePropertiesStruct* w_data)
{
    LO_Color* bg_color;
    LO_Color* normal_color;
    LO_Color* link_color;
    LO_Color* active_color;
    LO_Color* followed_color;
	char*     bg_image;
	char*     p;
	char      buf[MAXPATHLEN];
	char      docname[MAXPATHLEN];

    if (w_data->use_custom) {
		bg_color = &w_data->colors[DOCUMENT_BACKGROUND_COLOR];
		normal_color = &w_data->colors[DOCUMENT_NORMAL_TEXT_COLOR];
		link_color = &w_data->colors[DOCUMENT_LINK_TEXT_COLOR];
		active_color = &w_data->colors[DOCUMENT_ACTIVE_TEXT_COLOR];
		followed_color = &w_data->colors[DOCUMENT_FOLLOWED_TEXT_COLOR];
	} else {
	    bg_color = NULL;
		normal_color = link_color = active_color = followed_color = NULL;
	}

	if (w_data->use_image) {
		bg_image = XmTextFieldGetString(w_data->image_text);

		/*
		 *    Make sure bg image is fully qualified URL.
		 */
		if (bg_image != NULL) {
		    fe_StringTrim(bg_image);

			if ((p = strchr(bg_image, ':')) == NULL) {
			    if (bg_image[0] != '/' && !w_data->is_editor_preferences) {
				    strcpy(buf, "file:");
					fe_EditorMakeName(context, docname, sizeof(buf) - 5);
					fe_dirname(&buf[5], docname);
					strcat(buf, "/");
					strcat(buf, bg_image);
				} else {
				    sprintf(buf, "file:%s", bg_image);
				}
				XtFree(bg_image);
				bg_image = XtNewString(buf);
			}
		}
	} else {
		bg_image = NULL;
	}

	if (w_data->is_editor_preferences)
	    fe_EditorPreferencesSetColors(context,
									  bg_image,
									  bg_color,
									  normal_color,
									  link_color,
									  active_color,
									  followed_color);
	else
	    fe_EditorDocumentSetColors(context,
								   bg_image,
								   bg_color,
								   normal_color,
								   link_color,
								   active_color,
								   followed_color);

	if (bg_image)
	    XtFree(bg_image);
}

static void
fe_document_appearance_init(
						  MWContext* context,
						  fe_EditorDocumentAppearancePropertiesStruct* w_data)
{
    Boolean use_custom;
	char    bg_image[MAXPATHLEN];

	w_data->context = context;

    fe_EditorDefaultGetColors(
					  &w_data->default_colors[DOCUMENT_BACKGROUND_COLOR],
					  &w_data->default_colors[DOCUMENT_NORMAL_TEXT_COLOR],
					  &w_data->default_colors[DOCUMENT_LINK_TEXT_COLOR],
					  &w_data->default_colors[DOCUMENT_ACTIVE_TEXT_COLOR],
					  &w_data->default_colors[DOCUMENT_FOLLOWED_TEXT_COLOR]);

	if (w_data->is_editor_preferences)
  	    use_custom = fe_EditorPreferencesGetColors(context,
						bg_image,
						&w_data->colors[DOCUMENT_BACKGROUND_COLOR],
						&w_data->colors[DOCUMENT_NORMAL_TEXT_COLOR],
						&w_data->colors[DOCUMENT_LINK_TEXT_COLOR],
						&w_data->colors[DOCUMENT_ACTIVE_TEXT_COLOR],
						&w_data->colors[DOCUMENT_FOLLOWED_TEXT_COLOR]);
	else
	    use_custom = fe_EditorDocumentGetColors(context,
						bg_image,
						&w_data->colors[DOCUMENT_BACKGROUND_COLOR],
						&w_data->colors[DOCUMENT_NORMAL_TEXT_COLOR],
						&w_data->colors[DOCUMENT_LINK_TEXT_COLOR],
						&w_data->colors[DOCUMENT_ACTIVE_TEXT_COLOR],
						&w_data->colors[DOCUMENT_FOLLOWED_TEXT_COLOR]);

	w_data->use_custom = use_custom;

	if (bg_image[0] != '\0') {
	  w_data->use_image = TRUE;
	  w_data->image_path = bg_image;
	} else {
	  w_data->use_image = FALSE;
	  w_data->image_path = NULL;
	}

	fe_DependentListCallDependents(NULL,
								   w_data->dependents,
								   ~0,
								   (XtPointer)w_data);
    w_data->image_path = NULL; /* held in TextField */
}

static Dimension
fe_get_offset_from_widest(Widget* children, Cardinal nchildren)
{
	Dimension width;
	Dimension margin_width;

	Widget fat_guy = fe_GetBiggestWidget(TRUE, children, nchildren);
	XtVaGetValues(fat_guy, XmNwidth, &width, 0);
	XtVaGetValues(XtParent(fat_guy), XmNmarginWidth, &margin_width, 0);

	return width + margin_width;
}

static Widget
fe_document_appearance_create(
						  MWContext* context,
						  Widget parent,
						  fe_EditorDocumentAppearancePropertiesStruct* w_data)
{
	Arg args[16];
	Cardinal n;
	Widget main_rc;

	Widget colors_frame;
	Widget colors_rc;

	Widget strategy_rc;
	Widget strategy_custom;
	Widget strategy_browser;

#ifdef DOCUMENT_COLOR_SCHEMES
	Widget schemes_frame;
	Widget schemes_form;
	Widget schemes_combo;
	Widget schemes_save;
	Widget schemes_remove;
#endif

	Widget custom_form;
	Widget custom_frame;
	Widget custom_preview_rc;
	Widget background_info;
	Widget fat_guy;
	Widget top_guy;
	Widget bottom_guy;
	Widget children[6];
	Widget swatches[6];
	Cardinal nchildren;
	Cardinal nswatch;
	Dimension width;
	int i;

	Widget background_frame;
	Widget background_form;
	Widget background_image_toggle;
	Widget background_image_text;
	Widget background_image_button;

	Widget info_label;

	n = 0;
	XtSetArg(args[n], XmNentryAlignment, XmALIGNMENT_CENTER); n++;
	XtSetArg(args[n], XmNorientation, XmVERTICAL); n++;
	main_rc = XmCreateRowColumn(parent, "appearance", args, n);
	XtManageChild(main_rc);

	/*
	 *    Custom colors.
	 */
	n = 0;
	colors_frame = fe_CreateFrame(main_rc, "documentColors", args, n);
	XtManageChild(colors_frame);

	n = 0;
	XtSetArg(args[n], XmNentryAlignment, XmALIGNMENT_CENTER); n++;
	XtSetArg(args[n], XmNorientation, XmVERTICAL); n++;
	colors_rc = XmCreateRowColumn(colors_frame, "colors", args, n);
	XtManageChild(colors_rc);

	/*
	 *    Top row.
	 */
	n = 0;
	XtSetArg(args[n], XmNorientation, XmHORIZONTAL); n++;
	XtSetArg(args[n], XmNpacking, XmPACK_TIGHT); n++;
	XtSetArg(args[n], XmNentryVerticalAlignment, XmALIGNMENT_BASELINE_TOP); n++;
	strategy_rc = XmCreateRowColumn(colors_rc, "strategy", args, n);
	XtManageChild(strategy_rc);

	n = 0;
	XtSetArg(args[n], XmNuserData, TRUE); n++;
	XtSetArg(args[n], XmNindicatorType, XmONE_OF_MANY); n++;
	strategy_custom = XmCreateToggleButtonGadget(strategy_rc,
												 "custom", args, n);
	XtManageChild(strategy_custom);

	XtAddCallback(strategy_custom, XmNvalueChangedCallback,
				  fe_document_appearance_use_custom_cb, (XtPointer)w_data);

	fe_DependentListAddDependent(&w_data->dependents,
								 strategy_custom,
								 DOCUMENT_USE_CUSTOM_MASK,
								 fe_document_appearance_use_custom_update_cb,
								 (XtPointer)w_data);

	n = 0;
	XtSetArg(args[n], XmNuserData, FALSE); n++;
	XtSetArg(args[n], XmNindicatorType, XmONE_OF_MANY); n++;
	strategy_browser = XmCreateToggleButtonGadget(strategy_rc,
												 "browser", args, n);
	XtManageChild(strategy_browser);
	
	XtAddCallback(strategy_browser, XmNvalueChangedCallback,
				  fe_document_appearance_use_custom_cb, (XtPointer)w_data);

	fe_DependentListAddDependent(&w_data->dependents,
								 strategy_browser,
								 DOCUMENT_USE_CUSTOM_MASK,
								 fe_document_appearance_use_custom_update_cb,
								 (XtPointer)w_data);

#ifdef DOCUMENT_COLOR_SCHEMES
	/*
	 *    Color Schemes.
	 */
	n = 0;
	schemes_frame = fe_CreateFrame(colors_rc, "schemes", args, n);
	XtManageChild(schemes_frame);

	n = 0;
	schemes_form = XmCreateForm(schemes_frame, "schemes", args, n);
	XtManageChild(schemes_form);

	n = 0;
	XtSetArg(args[n], XmNtopAttachment, XmATTACH_FORM); n++;
	XtSetArg(args[n], XmNrightAttachment, XmATTACH_FORM); n++;
	XtSetArg(args[n], XmNsensitive, FALSE); n++;
	schemes_remove = XmCreatePushButtonGadget(schemes_form, "remove", args, n);
	XtManageChild(schemes_remove);

	n = 0;
	XtSetArg(args[n], XmNtopAttachment, XmATTACH_FORM); n++;
	XtSetArg(args[n], XmNrightAttachment, XmATTACH_WIDGET); n++;
	XtSetArg(args[n], XmNrightWidget, schemes_remove); n++;
	XtSetArg(args[n], XmNsensitive, FALSE); n++;
	schemes_save = XmCreatePushButtonGadget(schemes_form, "save", args, n);
	XtManageChild(schemes_save);

	n = 0;
	XtSetArg(args[n], XmNtopAttachment, XmATTACH_FORM); n++;
	XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM); n++;
	XtSetArg(args[n], XmNrightAttachment, XmATTACH_WIDGET); n++;
	XtSetArg(args[n], XmNrightWidget, schemes_save); n++;
	XtSetArg(args[n], XmNsensitive, FALSE); n++;
	schemes_combo = fe_CreateCombo(schemes_form, "combo", args, n);
	XtManageChild(schemes_combo);
#else
#if 0
	/*
	 *    make something to break up the top row from the colro buttons.
	 */
	n = 0;
	custom_form = XmCreateSeparatorGadget(colors_rc, "separator", args, n);
	XtManageChild(custom_form);
#endif
#endif

	n = 0;
	custom_form = XmCreateForm(colors_rc, "custom", args, n);
	XtManageChild(custom_form);

	for (nchildren = DOCUMENT_NORMAL_TEXT_COLOR;
		 nchildren <= DOCUMENT_BACKGROUND_COLOR;
		 nchildren++) {
	    Widget button;
		char*  name = fe_PreviewPanelCreate_names[nchildren];
		unsigned flags;

		n = 0;
		XtSetArg(args[n], XmNuserData, nchildren); n++;
		button = XmCreatePushButtonGadget(custom_form, name, args, n);
		XtAddCallback(button, XmNactivateCallback,
					  fe_document_appearance_color_cb, (XtPointer)w_data);
		flags = DOCUMENT_USE_CUSTOM_MASK;
		if (nchildren == DOCUMENT_BACKGROUND_COLOR)
			flags |= DOCUMENT_USE_IMAGE_MASK;
		fe_DependentListAddDependent(&w_data->dependents,
								 button,
								 flags,
								 fe_document_appearance_sensitized_update_cb,
								 (XtPointer)w_data);
		children[nchildren] = button;
	}
	XtManageChildren(children, nchildren);

	fat_guy = fe_GetBiggestWidget(TRUE, children, nchildren);
	XtVaGetValues(fat_guy, XmNwidth, &width, 0);

	/* swatches */
	for (nswatch = DOCUMENT_NORMAL_TEXT_COLOR;
		 nswatch <= DOCUMENT_BACKGROUND_COLOR;
		 nswatch++) {
	    Widget foo;
		char   name[32];
		unsigned flags;

		sprintf(name, "%sSwatch", fe_PreviewPanelCreate_names[nswatch]);
		n = 0;
		XtSetArg(args[n], XmNuserData, nswatch); n++;
		XtSetArg(args[n], XmNwidth, SWATCH_SIZE); n++;
		foo = fe_CreateSwatch(custom_form, name, args, n);
		XtAddCallback(foo, XmNactivateCallback,
					  fe_document_appearance_color_cb, (XtPointer)w_data);

		flags = DOCUMENT_USE_CUSTOM_MASK|(1<<nswatch);
		if (nswatch == DOCUMENT_BACKGROUND_COLOR)
			flags |= DOCUMENT_USE_IMAGE_MASK;
		fe_DependentListAddDependent(&w_data->dependents,
									 foo,
									 flags,
									 fe_document_appearance_swatch_update_cb,
									 (XtPointer)w_data);
		swatches[nswatch] = foo;
	}
	XtManageChildren(swatches, nswatch);

	/*
	 *    Do attachments.
	 */
	for (top_guy = NULL, i = 0; i < nchildren; i++) {

		n = 0;
		if (top_guy) {
			XtSetArg(args[n], XmNtopAttachment, XmATTACH_WIDGET); n++;
			XtSetArg(args[n], XmNtopWidget, top_guy); n++;
		} else {
			XtSetArg(args[n], XmNtopAttachment, XmATTACH_FORM); n++;
		}
		XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM); n++;

		top_guy = children[i];

		if (top_guy != fat_guy) {
		    /*
			 *    Have to do this to avoid circular dependency in
			 *    losing XmForm. People were paid money for writing
			 *    that piece of shit? Real money? No way!
			 */
		    XtSetArg(args[n], XmNwidth, width); n++;
#if 0
		    XtSetArg(args[n], XmNrightAttachment,
				   XmATTACH_OPPOSITE_WIDGET); n++;
		    XtSetArg(args[n],XmNrightWidget, fat_guy); n++;
#endif
		}
		XtSetValues(top_guy, args, n);

		/* Do swatch. */
		n = 0;
		XtSetArg(args[n], XmNleftAttachment, XmATTACH_WIDGET); n++;
		XtSetArg(args[n], XmNleftWidget, top_guy); n++;
		XtSetArg(args[n], XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET); n++;
		XtSetArg(args[n], XmNtopWidget, top_guy); n++;
		XtSetArg(args[n], XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET); n++;
		XtSetArg(args[n], XmNbottomWidget, top_guy); n++;
		XtSetValues(swatches[i], args, n);
	}

	top_guy = swatches[DOCUMENT_NORMAL_TEXT_COLOR];
	bottom_guy = swatches[DOCUMENT_FOLLOWED_TEXT_COLOR];

	n = 0;
	XtSetArg(args[n], XmNleftAttachment, XmATTACH_WIDGET); n++;
	XtSetArg(args[n], XmNleftWidget, top_guy); n++;
	XtSetArg(args[n], XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET); n++;
	XtSetArg(args[n], XmNtopWidget, top_guy); n++;
	XtSetArg(args[n], XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET); n++;
	XtSetArg(args[n], XmNbottomWidget, bottom_guy); n++;
	XtSetArg(args[n], XmNrightAttachment, XmATTACH_FORM); n++;
#if 1
	custom_frame = /*fe_*/XmCreateFrame(custom_form, "previewFrame", args, n);
	XtManageChild(custom_frame);
	n = 0; /* I'm tired of fucking with resources to get this right */
	XtSetArg(args[n], XmNmarginWidth, 0); n++;
	XtSetArg(args[n], XmNmarginHeight, 0); n++;
	XtSetValues(custom_frame, args, n);
#else
	n = 0;
	XtSetArg(args[n], XmNorientation, XmVERTICAL); n++;
	XtSetArg(args[n], XmNborderWidth, 1); n++;
	XtSetArg(args[n], XmNborderColor,
			 CONTEXT_DATA(w_data->context)->default_fg_pixel); n++;
#endif
	custom_preview_rc = fe_PreviewPanelCreate(custom_frame, "preview",
											  args, n);
	XtManageChild(custom_preview_rc);

	XtAddCallback(custom_preview_rc, XmNinputCallback,
				  fe_preview_panel_click_cb, (XtPointer)w_data);

	fe_DependentListAddDependent(&w_data->dependents,
						 custom_preview_rc,
						 (DOCUMENT_USE_CUSTOM_MASK|DOCUMENT_ALL_APPEARANCE),
						 fe_document_appearance_preview_update_cb,
						 (XtPointer)w_data);
#if 1
	fe_DependentListAddDependent(&w_data->dependents,
								 custom_preview_rc,
								 DOCUMENT_USE_CUSTOM_MASK,
								 fe_document_appearance_sensitized_update_cb,
								 (XtPointer)w_data);
#endif
	top_guy = swatches[DOCUMENT_BACKGROUND_COLOR];

	n = 0;
	XtSetArg(args[n], XmNleftAttachment, XmATTACH_WIDGET); n++;
	XtSetArg(args[n], XmNleftWidget, top_guy); n++;
	XtSetArg(args[n], XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET); n++;
	XtSetArg(args[n], XmNtopWidget, top_guy); n++;
	XtSetArg(args[n], XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET); n++;
	XtSetArg(args[n], XmNbottomWidget, top_guy); n++;
	XtSetArg(args[n], XmNrightAttachment, XmATTACH_FORM); n++;
	XtSetArg(args[n], XmNalignment, XmALIGNMENT_BEGINNING); n++;
	background_info = XmCreateLabelGadget(custom_form, "backgroundInfo", args, n);
	XtManageChild(background_info);

	/*
	 *    Background.
	 */
	n = 0;
	background_frame = fe_CreateFrame(main_rc, "backgroundImage", args, n);
	XtManageChild(background_frame);

	n = 0;
	background_form = XmCreateForm(background_frame, "backgroundImage", args, n);
	XtManageChild(background_form);

	nchildren = 0;
	n = 0;
	XtSetArg(args[n], XmNindicatorType, XmN_OF_MANY); n++;
	background_image_toggle = XmCreateToggleButtonGadget(background_form,
														"useImage", args, n);
	children[nchildren++] = background_image_toggle;

	XtAddCallback(background_image_toggle, XmNvalueChangedCallback,
				  fe_document_appearance_use_image_cb, (XtPointer)w_data);
	fe_DependentListAddDependent(&w_data->dependents,
						 background_image_toggle,
						 (DOCUMENT_USE_IMAGE_MASK|DOCUMENT_USE_CUSTOM_MASK),
						 fe_document_appearance_use_image_update_cb,
						 (XtPointer)w_data);

	n = 0;
	background_image_text = fe_CreateTextField(background_form,
											  "imageText", args, n);
	XtAddCallback(background_image_text, XmNvalueChangedCallback,
				  fe_document_appearance_image_text_cb,
				  (XtPointer)w_data);
	fe_DependentListAddDependent(&w_data->dependents,
								 background_image_text,
								 (DOCUMENT_BACKGROUND_IMAGE_MASK|
								  DOCUMENT_USE_IMAGE_MASK|
								  DOCUMENT_USE_CUSTOM_MASK),
								 fe_document_appearance_image_text_update_cb,
								 (XtPointer)w_data);
	w_data->image_text = background_image_text;
	children[nchildren++] = background_image_text;

	n = 0;
	XtSetArg(args[n], XmNuserData, background_image_text); n++;
	background_image_button = XmCreatePushButtonGadget(background_form,
													   "browseImageFile", args, n);
	XtAddCallback(background_image_button, XmNactivateCallback,
				  fe_document_appearance_image_text_browse_cb,
				  (XtPointer)w_data);
	fe_DependentListAddDependent(&w_data->dependents,
						 background_image_button,
						 (DOCUMENT_USE_IMAGE_MASK|DOCUMENT_USE_CUSTOM_MASK),
						 fe_document_appearance_use_image_button_update_cb,
						 (XtPointer)w_data);
	children[nchildren++] = background_image_button;

	/*
	 *    Do background image group attachments.
	 */
	n = 0;
	XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM); n++;
	XtSetValues(background_image_toggle, args, n);

	n = 0;
	XtSetArg(args[n], XmNleftAttachment, XmATTACH_WIDGET); n++;
	XtSetArg(args[n], XmNleftWidget, background_image_toggle); n++;
	XtSetArg(args[n], XmNrightAttachment, XmATTACH_WIDGET); n++;
	XtSetArg(args[n], XmNrightWidget, background_image_button); n++;
	XtSetValues(background_image_text, args, n);

	n = 0;
	XtSetArg(args[n], XmNrightAttachment, XmATTACH_FORM); n++;
	XtSetValues(background_image_button, args, n);

	XtManageChildren(children, nchildren);

	/*
	 *    Bottom line info text.
	 */
	n = 0;
	info_label = XmCreateLabelGadget(main_rc, "infoLabel", args, n);
	XtManageChild(info_label);

	return main_rc;
}

#define DOCUMENT_GENERAL_TITLE           (0x1<<0)
#define DOCUMENT_GENERAL_AUTHOR          (0x1<<1)
#define DOCUMENT_GENERAL_DESCRIPTION     (0x1<<2)
#define DOCUMENT_GENERAL_KEYWORDS        (0x1<<3)
#define DOCUMENT_GENERAL_CLASSIFICATION  (0x1<<4)

static void
fe_document_general_changed_cb(Widget widget,
							   XtPointer closure, XtPointer call_data)
{
	fe_EditorDocumentGeneralPropertiesStruct* w_data
		= (fe_EditorDocumentGeneralPropertiesStruct*)closure;
    unsigned mask = (unsigned)fe_GetUserData(widget);

	w_data->changed |= mask;
}

/*
 *	Purpose:	Condense a URL to a given length.
 *              Lifted the algorithm from winfe/popup.cpp/WFE_CondenseURL()
 */
char*
FE_CondenseURL(char* target, char* source, unsigned target_len)
{
	unsigned source_len = strlen(source);
	unsigned first_half_len;
	unsigned second_half_len;

	if (source_len < target_len) {
		strcpy(target, source);
	} else {
		first_half_len = (target_len - 3)/2;
		second_half_len = target_len - first_half_len - 4; /* -1 for NUL */

		strncpy(target, source, first_half_len);
		strcpy(&target[first_half_len], "...");
		strcat(target, &source[source_len - second_half_len]);
	}
	return target;
}

static void
fe_document_general_init(MWContext* context,
						 fe_EditorDocumentGeneralPropertiesStruct* w_data)
{
	XmString xm_string;
	char buf[40]; /* will specifiy size of maximum location text length */
	char* value;

	/* set defaults */ /* <unknown> */
	xm_string = XmStringCreateLocalized(XP_GetString(XFE_UNKNOWN));
#ifdef EDITOR_SHOW_CREATE_DATE
	XtVaSetValues(w_data->created, XmNlabelString, xm_string, 0);
	XtVaSetValues(w_data->updated, XmNlabelString, xm_string, 0);
#endif /*EDITOR_SHOW_CREATE_DATE*/
	XtVaSetValues(w_data->location, XmNlabelString, xm_string, 0);
	XmStringFree(xm_string);

	/* get the location */
	fe_EditorMakeName(context, buf, sizeof(buf));
	xm_string = XmStringCreateLocalized(buf);
	XtVaSetValues(w_data->location, XmNlabelString, xm_string, 0);
	XmStringFree(xm_string);

	/* get the title */
	if ((value = fe_EditorDocumentGetTitle(context)) != NULL)
		XmTextFieldSetString(w_data->title, value);

	/* get the author */
	if ((value = fe_EditorDocumentGetMetaData(context, "Author")) != NULL)
		XmTextFieldSetString(w_data->author, value);

	/* get the description */
	if ((value = fe_EditorDocumentGetMetaData(context, "Description")))
		XmTextSetString(w_data->description, value);

	/* get the keywords */
	if ((value = fe_EditorDocumentGetMetaData(context, "Keywords")))
		XmTextSetString(w_data->keywords, value);

	/* get the classification */
	if ((value = fe_EditorDocumentGetMetaData(context, "Classification")))
		XmTextSetString(w_data->classification, value);

#ifdef EDITOR_SHOW_CREATE_DATE
	/* get the created */
	if ((value = fe_EditorDocumentGetMetaData(context, "Created"))) {
		xm_string = XmStringCreateLocalized(value);
		XtVaSetValues(w_data->created, XmNlabelString, xm_string, 0);
		XmStringFree(xm_string);
	}

	/* get the updated */
	if ((value = fe_EditorDocumentGetMetaData(context, "Last-Modified"))) {
		xm_string = XmStringCreateLocalized(value);
		XtVaSetValues(w_data->updated, XmNlabelString, xm_string, 0);
		XmStringFree(xm_string);
	}
#endif /*EDITOR_SHOW_CREATE_DATE*/

	w_data->changed = 0;
}

static void
fe_document_general_set(MWContext* context,
						fe_EditorDocumentGeneralPropertiesStruct* w_data)
{
	char* value;

	/* set the title */
	if ((w_data->changed & DOCUMENT_GENERAL_TITLE) != 0) {
		value = XmTextFieldGetString(w_data->title);
		fe_EditorDocumentSetTitle(context, value);
		XtFree(value);
	}

	/* author */
	if ((w_data->changed & DOCUMENT_GENERAL_AUTHOR) != 0) {
		value = XmTextFieldGetString(w_data->author);
		fe_EditorDocumentSetMetaData(context, "Author", value);
		XtFree(value);
	}
	
	/* description */
	if ((w_data->changed & DOCUMENT_GENERAL_DESCRIPTION) != 0) {
		value = XmTextGetString(w_data->description);
		fe_EditorDocumentSetMetaData(context, "Description", value);
		XtFree(value);
	}
	
	/* keywords */
	if ((w_data->changed & DOCUMENT_GENERAL_KEYWORDS) != 0) {
		value = XmTextGetString(w_data->keywords);
		fe_EditorDocumentSetMetaData(context, "Keywords", value);
		XtFree(value);
	}
	
	/* classification */
	if ((w_data->changed & DOCUMENT_GENERAL_CLASSIFICATION) != 0) {
		value = XmTextGetString(w_data->classification);
		fe_EditorDocumentSetMetaData(context, "Classification", value);
		XtFree(value);
	}
	
	w_data->changed = 0;
}

static Widget
fe_document_general_create(
						  MWContext* context,
						  Widget parent,
						  fe_EditorDocumentGeneralPropertiesStruct* w_data)
{
	Widget form;
	Widget location_label;
	Widget location_text;
	Widget title_label;
	Widget title_text;
	Widget author_label;
	Widget author_text;
	Widget description_label;
	Widget description_text;
#ifdef EDITOR_SHOW_CREATE_DATE
	Widget created_label;
	Widget created_text;
	Widget updated_label;
	Widget updated_text;
	Widget fat_guy;
#endif /*EDITOR_SHOW_CREATE_DATE*/
	Widget other_frame;
	Widget other_form;
	Widget info_label;
	Widget keywords_label;
	Widget keywords_text;
	Widget classification_label;
	Widget classification_text;
	Widget children[16];
	Cardinal nchildren;
	Dimension left_offset;
	Arg    args[8];
	Cardinal n;
	Cardinal i;

	n = 0;
	form = XmCreateForm(parent, "general", args, n);

	nchildren = 0;
	n = 0;
	location_label = XmCreateLabelGadget(form, "locationLabel", args, n);
	children[nchildren++] = location_label;
	
	n = 0;
	title_label = XmCreateLabelGadget(form, "titleLabel", args, n);
	children[nchildren++] = title_label;
	
	n = 0;
	author_label = XmCreateLabelGadget(form, "authorLabel", args, n);
	children[nchildren++] = author_label;
	
	n = 0;
	description_label = XmCreateLabelGadget(form, "descriptionLabel", args, n);
	children[nchildren++] = description_label;

	left_offset = fe_get_offset_from_widest(children, nchildren);

	n = 0;
	XtSetArg(args[n], XmNtopAttachment, XmATTACH_FORM); n++;
	XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM); n++;
	XtSetArg(args[n], XmNleftOffset, left_offset); n++;
	location_text = XmCreateLabelGadget(form, "locationText", args, n);
	children[nchildren++] = location_text;
	
	n = 0;
	XtSetArg(args[n], XmNtopAttachment, XmATTACH_WIDGET); n++;
	XtSetArg(args[n], XmNtopWidget, children[nchildren-1]); n++;
	XtSetArg(args[n], XmNleftAttachment, XmATTACH_OPPOSITE_WIDGET); n++;
	XtSetArg(args[n], XmNleftWidget, location_text); n++;
	XtSetArg(args[n], XmNrightAttachment, XmATTACH_FORM); n++;
	XtSetArg(args[n], XmNuserData, DOCUMENT_GENERAL_TITLE); n++;
	title_text = XmCreateTextField(form, "titleText", args, n);
	children[nchildren++] = title_text;

	XtAddCallback(title_text, XmNvalueChangedCallback,
				  fe_document_general_changed_cb, (XtPointer)w_data);
	
	n = 0;
	XtSetArg(args[n], XmNtopAttachment, XmATTACH_WIDGET); n++;
	XtSetArg(args[n], XmNtopWidget, children[nchildren-1]); n++;
	XtSetArg(args[n], XmNleftAttachment, XmATTACH_OPPOSITE_WIDGET); n++;
	XtSetArg(args[n], XmNleftWidget, location_text); n++;
	XtSetArg(args[n], XmNrightAttachment, XmATTACH_FORM); n++;
	XtSetArg(args[n], XmNuserData, DOCUMENT_GENERAL_AUTHOR); n++;
	author_text = XmCreateTextField(form, "authorText", args, n);
	children[nchildren++] = author_text;
	
	XtAddCallback(author_text, XmNvalueChangedCallback,
				  fe_document_general_changed_cb, (XtPointer)w_data);
	
	n = 0;
	XtSetArg(args[n], XmNeditMode, XmMULTI_LINE_EDIT); n++;
	XtSetArg(args[n], XmNrows, 2); n++;
	XtSetArg(args[n], XmNtopAttachment, XmATTACH_WIDGET); n++;
	XtSetArg(args[n], XmNtopWidget, children[nchildren-1]); n++;
	XtSetArg(args[n], XmNleftAttachment, XmATTACH_OPPOSITE_WIDGET); n++;
	XtSetArg(args[n], XmNleftWidget, location_text); n++;
	XtSetArg(args[n], XmNrightAttachment, XmATTACH_FORM); n++;
	XtSetArg(args[n], XmNuserData, DOCUMENT_GENERAL_DESCRIPTION); n++;
	description_text = XmCreateText(form, "descriptionText", args, n);
	children[nchildren++] = description_text;

	XtAddCallback(description_text, XmNvalueChangedCallback,
				  fe_document_general_changed_cb, (XtPointer)w_data);
	
	/* go back and do label attachments */
	for (i = 0; i < 4; i++) {
		n = 0;
		if (i == 0) {
			XtSetArg(args[n], XmNtopAttachment, XmATTACH_FORM); n++;
		} else {
			XtSetArg(args[n], XmNtopAttachment, XmATTACH_WIDGET); n++;
			XtSetArg(args[n], XmNtopWidget, children[i+3]); n++;
		}
		XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM); n++;
		XtSetValues(children[i], args, n);
	}

	XtManageChildren(children, nchildren);

	nchildren = 0;
#ifdef EDITOR_SHOW_CREATE_DATE
	n = 0;
	XtSetArg(args[n], XmNtopAttachment, XmATTACH_WIDGET); n++;
	XtSetArg(args[n], XmNtopWidget, description_text); n++;
	XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM); n++;
	created_label = XmCreateLabelGadget(form, "createdLabel", args, n);
	children[nchildren++] = created_label;

	n = 0;
	XtSetArg(args[n], XmNtopAttachment, XmATTACH_WIDGET); n++;
	XtSetArg(args[n], XmNtopWidget, created_label); n++;
	XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM); n++;
	updated_label = XmCreateLabelGadget(form, "updatedLabel", args, n);
	children[nchildren++] = updated_label;

	fat_guy = fe_GetBiggestWidget(TRUE, children, nchildren);

	n = 0;
	XtSetArg(args[n], XmNtopAttachment, XmATTACH_WIDGET); n++;
	XtSetArg(args[n], XmNtopWidget, description_text); n++;
	XtSetArg(args[n], XmNleftAttachment, XmATTACH_WIDGET); n++;
	XtSetArg(args[n], XmNleftWidget, fat_guy); n++;
	created_text = XmCreateLabelGadget(form, "createdText", args, n);
	children[nchildren++] = created_text;

	n = 0;
	XtSetArg(args[n], XmNtopAttachment, XmATTACH_WIDGET); n++;
	XtSetArg(args[n], XmNtopWidget, created_text); n++;
	XtSetArg(args[n], XmNleftAttachment, XmATTACH_WIDGET); n++;
	XtSetArg(args[n], XmNleftWidget, fat_guy); n++;
	updated_text = XmCreateLabelGadget(form, "updatedText", args, n);
	children[nchildren++] = updated_text;
#endif /*EDITOR_SHOW_CREATE_DATE*/

	n = 0;
	XtSetArg(args[n], XmNtopAttachment, XmATTACH_WIDGET); n++;
#ifdef EDITOR_SHOW_CREATE_DATE
	XtSetArg(args[n], XmNtopWidget, updated_text); n++;
#else
	XtSetArg(args[n], XmNtopWidget, description_text); n++;
#endif /*EDITOR_SHOW_CREATE_DATE*/
	XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM); n++;
	XtSetArg(args[n], XmNrightAttachment, XmATTACH_FORM); n++;
	XtSetArg(args[n], XmNbottomAttachment, XmATTACH_FORM); n++;
	other_frame = fe_CreateFrame(form, "otherAttributes", args, n);
	children[nchildren++] = other_frame;

	XtManageChildren(children, nchildren);
#if 1

	n = 0;
	other_form = XmCreateForm(other_frame, "form", args, n);
	XtManageChild(other_form);

	nchildren = 0;
	n = 0;
	keywords_label = XmCreateLabelGadget(other_form, "keywordsLabel", args, n);
	children[nchildren++] = keywords_label;

	n = 0;
	classification_label = XmCreateLabelGadget(other_form,
											   "classificationLabel",
											   args, n);
	children[nchildren++] = classification_label;

	left_offset = fe_get_offset_from_widest(children, nchildren);

	n = 0;
	XtSetArg(args[n], XmNtopAttachment, XmATTACH_FORM); n++;
	XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM); n++;
	XtSetArg(args[n], XmNleftOffset, left_offset); n++;
	info_label = XmCreateLabelGadget(other_form, "infoLabel", args, n);
	children[nchildren++] = info_label;

	n = 0;
	XtSetArg(args[n], XmNeditMode, XmMULTI_LINE_EDIT); n++;
	XtSetArg(args[n], XmNrows, 2); n++;
	XtSetArg(args[n], XmNtopAttachment, XmATTACH_WIDGET); n++;
	XtSetArg(args[n], XmNtopWidget, info_label); n++;
	XtSetArg(args[n], XmNleftAttachment, XmATTACH_OPPOSITE_WIDGET); n++;
	XtSetArg(args[n], XmNleftWidget, info_label); n++;
	XtSetArg(args[n], XmNrightAttachment, XmATTACH_FORM); n++;
	XtSetArg(args[n], XmNuserData, DOCUMENT_GENERAL_KEYWORDS); n++;
	keywords_text = XmCreateText(other_form, "keywordsText", args, n);
	children[nchildren++] = keywords_text;

	XtAddCallback(keywords_text, XmNvalueChangedCallback,
				  fe_document_general_changed_cb, (XtPointer)w_data);
	
	n = 0;
	XtSetArg(args[n], XmNeditMode, XmMULTI_LINE_EDIT); n++;
	XtSetArg(args[n], XmNrows, 2); n++;
	XtSetArg(args[n], XmNtopAttachment, XmATTACH_WIDGET); n++;
	XtSetArg(args[n], XmNtopWidget, keywords_text); n++;
	XtSetArg(args[n], XmNleftAttachment, XmATTACH_OPPOSITE_WIDGET); n++;
	XtSetArg(args[n], XmNleftWidget, info_label); n++;
	XtSetArg(args[n], XmNrightAttachment, XmATTACH_FORM); n++;
	XtSetArg(args[n], XmNuserData, DOCUMENT_GENERAL_CLASSIFICATION); n++;
	classification_text = XmCreateText(other_form, "classificationText",
									   args, n);
	children[nchildren++] = classification_text;

	XtAddCallback(classification_text, XmNvalueChangedCallback,
				  fe_document_general_changed_cb, (XtPointer)w_data);
	
	/* go back set attachments for labels */
	n = 0;
	XtSetArg(args[n], XmNtopAttachment, XmATTACH_WIDGET); n++;
	XtSetArg(args[n], XmNtopWidget, info_label); n++;
	XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM); n++;
	XtSetValues(keywords_label, args, n);
	
	n = 0;
	XtSetArg(args[n], XmNtopAttachment, XmATTACH_WIDGET); n++;
	XtSetArg(args[n], XmNtopWidget, keywords_text); n++;
	XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM); n++;
	XtSetValues(classification_label, args, n);

	XtManageChildren(children, nchildren);
#endif
	XtManageChild(form);

	w_data->location = location_text;
	w_data->title = title_text;
	w_data->author = author_text;
	w_data->description = description_text;
#ifdef EDITOR_SHOW_CREATE_DATE
	w_data->created = created_text;
	w_data->updated = updated_text;
#endif /*EDITOR_SHOW_CREATE_DATE*/
	w_data->keywords = keywords_text;
	w_data->classification = classification_text;

	return form;
}

typedef enum
{
  XFE_USERMETA_VIEW,
  XFE_USERMETA_MODIFY,
  XFE_USERMETA_NEW
} fe_AdvancedUserState;

typedef struct fe_NameValueItemList
{
    fe_NameValueItem* items;
    unsigned          nitems;    /* logical number of items */
    unsigned          size; /* size of vector */
	Widget            widget;
	Boolean           is_dirty;
    int               selected_item;
	unsigned          mask;
} fe_NameValueItemList;

#define NOTHING_SELECTED (-1)

typedef struct fe_EditorDocumentAdvancedPropertiesStruct
{
    MWContext* context;
	fe_NameValueItemList  system_list;
	fe_NameValueItemList  user_list;
	fe_NameValueItemList* active_list;
    Boolean  item_is_new;
    Boolean  item_is_dirty;
    Widget   name_text;
    Widget   value_text;
    unsigned changed;
    fe_DependentList* dependents;
} fe_EditorDocumentAdvancedPropertiesStruct;

#define DOCUMENT_ADVANCED_HTTP_LIST (0x1<<0)
#define DOCUMENT_ADVANCED_META_LIST (0x1<<1)
#define DOCUMENT_ADVANCED_ITEM      (0x1<<2)
#define DOCUMENT_ADVANCED_SELECTION (0x1<<3)

static void
fe_document_advanced_tell_me_eh(Widget widget, XtPointer closure,
								XEvent* event, Boolean* keep_going)
{
    fe_EditorDocumentAdvancedPropertiesStruct* w_data = 
	  (fe_EditorDocumentAdvancedPropertiesStruct*)closure;
	fe_NameValueItemList* my_list;

	*keep_going = TRUE;

	XmProcessTraversal(widget, XmTRAVERSE_CURRENT);

	/*
	 *    We may get called after the callback (next), so..
	 */
	if (w_data->active_list != NULL && w_data->active_list->widget == widget)
		return;

	if (w_data->user_list.widget == widget)
		my_list = &w_data->user_list;
	else
		my_list = &w_data->system_list;
		
	w_data->active_list = my_list;
	my_list->selected_item = NOTHING_SELECTED;
	my_list->is_dirty = FALSE;
	if (my_list->nitems == 0)
		w_data->item_is_new = TRUE;
	else
		w_data->item_is_new = FALSE;
	w_data->item_is_dirty = FALSE;

	fe_DependentListCallDependents(widget,
								   w_data->dependents,
								   DOCUMENT_ADVANCED_SELECTION,
								   closure);
}

static void
fe_document_advanced_list_cb(Widget widget, XtPointer closure,
									XtPointer foo)
{
    fe_EditorDocumentAdvancedPropertiesStruct* w_data = 
	  (fe_EditorDocumentAdvancedPropertiesStruct*)closure;
	XmListCallbackStruct* cb_data = (XmListCallbackStruct*)foo;
	fe_NameValueItemList* my_list;
	
	if (cb_data->reason != XmCR_SINGLE_SELECT)
  	    return;

	if (w_data->user_list.widget == widget)
		my_list = &w_data->user_list;
	else
		my_list = &w_data->system_list;

	/*
	 *    For some strange and unknown reason this callback
	 *    adds one to the list position it provides. Thanks
	 *    for putting it in the docs dicks.
	 */
	if (cb_data->item_position > 0 && cb_data->selected_item_count > 0) {
		my_list->selected_item = cb_data->item_position - 1;
	} else {
		my_list->selected_item = NOTHING_SELECTED;
	}

	if (my_list->nitems == 0)
		w_data->item_is_new = TRUE;
	else
		w_data->item_is_new = FALSE;

	w_data->active_list = my_list;
	my_list->is_dirty = FALSE;
	w_data->item_is_dirty = FALSE;

	fe_DependentListCallDependents(widget,
								   w_data->dependents,
								   DOCUMENT_ADVANCED_SELECTION,
								   closure);
}

static void
fe_copy_name_value_to_xm_list(Widget widget, fe_NameValueItem* list)
{
	unsigned i;
	char buf[512];
	XmString xm_string;

	XmListDeleteAllItems(widget);

	for (i = 0; list[i].name != NULL; i++) {
		strcpy(buf, list[i].name);
		strcat(buf, "=");
		strcat(buf, list[i].value);

		xm_string = XmStringCreateLocalized(buf);
		XmListAddItem(widget, xm_string, i + 1); /* MOTIFism */
		XmStringFree(xm_string);
	}
}

static void
fe_document_advanced_list_update_cb(Widget widget, XtPointer closure,
									XtPointer cb_data)
{
    fe_EditorDocumentAdvancedPropertiesStruct* w_data = 
	  (fe_EditorDocumentAdvancedPropertiesStruct*)closure;
	fe_NameValueItemList* my_list;
	Pixel    parent_bg;
	Pixel    select_bg;

	XtVaGetValues(XtParent(widget), XmNbackground, &parent_bg, 0);

	XmGetColors(XtScreen(widget), fe_cmap(w_data->context),
				parent_bg, NULL, NULL, NULL, &select_bg);

	if (w_data->user_list.widget == widget)
		my_list = &w_data->user_list;
	else
		my_list = &w_data->system_list;

	if (my_list->is_dirty) {
		fe_copy_name_value_to_xm_list(widget, my_list->items);
		my_list->is_dirty = FALSE;
	}

	if (w_data->active_list == my_list &&
		my_list->selected_item != NOTHING_SELECTED) {
	    XmListSelectPos(widget, my_list->selected_item + 1, FALSE);
	} else {
		XmListDeselectAllItems(widget);
	}

	if (w_data->active_list == my_list)
		XtVaSetValues(widget, XmNbackground, select_bg, 0);
	else
		XtVaSetValues(widget, XmNbackground, parent_bg, 0);
}

static void
fe_document_advanced_value_cb(Widget widget, XtPointer closure,
									XtPointer cb_data)
{
    fe_EditorDocumentAdvancedPropertiesStruct* w_data = 
	  (fe_EditorDocumentAdvancedPropertiesStruct*)closure;

	if (w_data->item_is_dirty == FALSE) {
		w_data->item_is_dirty = TRUE;

		fe_DependentListCallDependents(widget,
									   w_data->dependents,
									   DOCUMENT_ADVANCED_ITEM,
									   closure);
	}
}

static void
fe_document_advanced_name_update_cb(Widget widget, XtPointer closure,
									XtPointer cb_data)
{
    fe_EditorDocumentAdvancedPropertiesStruct* w_data = 
	  (fe_EditorDocumentAdvancedPropertiesStruct*)closure;
	fe_DependentListCallbackStruct* info = 
		(fe_DependentListCallbackStruct*)cb_data;
	char* name;
	
	if (info->caller == widget)
	  return;
	
	if (w_data->item_is_new) {
		fe_TextFieldSetString(widget, "", FALSE);
	    fe_TextFieldSetEditable(w_data->context, widget, TRUE);
		XtVaSetValues(XtParent(widget), XmNinitialFocus, widget, 0);
	} else {
		fe_NameValueItemList* active_list = w_data->active_list;
  	    if (active_list != NULL &&
			active_list->selected_item != NOTHING_SELECTED) {
		    name = active_list->items[active_list->selected_item].name;
			
			fe_TextFieldSetString(widget, name, FALSE);
		} else {
			fe_TextFieldSetString(widget, "", FALSE);
		}
	    fe_TextFieldSetEditable(w_data->context, widget, FALSE);
	}
}

static void
fe_document_advanced_value_update_cb(Widget widget, XtPointer closure,
									XtPointer cb_data)
{
    fe_EditorDocumentAdvancedPropertiesStruct* w_data = 
	  (fe_EditorDocumentAdvancedPropertiesStruct*)closure;
	fe_DependentListCallbackStruct* info = 
		(fe_DependentListCallbackStruct*)cb_data;
	char* value;
	
	if (info->caller == widget)
	  return;
	
	if (w_data->item_is_new) {
	    fe_TextFieldSetString(widget, "", FALSE);
	    fe_TextFieldSetEditable(w_data->context, widget, TRUE);
	} else {
		fe_NameValueItemList* active_list = w_data->active_list;
  	    if (active_list != NULL &&
			active_list->selected_item != NOTHING_SELECTED) {
		    value = active_list->items[active_list->selected_item].value;
			
			if (value == NULL)
			  value = "";

		    fe_TextFieldSetString(widget, value, FALSE);
			fe_TextFieldSetEditable(w_data->context, widget, TRUE);
#if 0
			XmProcessTraversal(widget, XmTRAVERSE_CURRENT);
#endif
		}	else { /* nothing selected */
		    fe_TextFieldSetString(widget, "", FALSE);
			fe_TextFieldSetEditable(w_data->context, widget, FALSE);
		}
	}
}

static void
fe_document_advanced_new_cb(Widget widget, XtPointer closure,
									XtPointer cb_data)
{
    fe_EditorDocumentAdvancedPropertiesStruct* w_data = 
	  (fe_EditorDocumentAdvancedPropertiesStruct*)closure;
	w_data->item_is_new = TRUE;
	w_data->item_is_dirty = FALSE;

	fe_DependentListCallDependents(widget,
								   w_data->dependents,
								   DOCUMENT_ADVANCED_SELECTION,
								   closure);
}

#if 0
static void
fe_document_advanced_new_update_cb(Widget widget, XtPointer closure,
									XtPointer cb_data)
{
    fe_EditorDocumentAdvancedPropertiesStruct* w_data = 
	  (fe_EditorDocumentAdvancedPropertiesStruct*)closure;
	Boolean sensitive = (w_data->item_is_new == FALSE);
	
	XtVaSetValues(widget, XmNsensitive, sensitive, 0);
}
#endif

static void
list_set_item(fe_NameValueItemList* active_list, unsigned index,
				char* name, char* value)
{
    unsigned size;

	if (active_list == NULL) /* this should be error */
		return;

    if (index == active_list->nitems)
	    active_list->nitems++;

    if (active_list->nitems > active_list->size) {

		active_list->size = active_list->nitems;
	    size = sizeof(fe_NameValueItem) * (active_list->size+1);
	    if (active_list->size == 0)
		    active_list->items = (fe_NameValueItem*)XtMalloc(size);
		else
		    active_list->items = (fe_NameValueItem*)XtRealloc((void*)active_list->items,
													 size);

		active_list->items[active_list->nitems].name = NULL;
		active_list->items[active_list->nitems].value = NULL;
	}

	if (active_list->items[index].name != NULL)
	    XtFree(active_list->items[index].name);
	if (active_list->items[index].value != NULL)
	    XtFree(active_list->items[index].value);

	active_list->items[index].name = XtNewString(name);
	if (value)
	    active_list->items[index].value = XtNewString(value);
	else
	    active_list->items[index].value = NULL;
	active_list->is_dirty = TRUE;
}

static void
list_delete_item(fe_NameValueItemList* active_list, unsigned index)
{

	if (active_list == NULL) /* this should be error */
		return;

    if (active_list->items[index].name)
	    XtFree(active_list->items[index].name);
    if (active_list->items[index].value)
	    XtFree(active_list->items[index].value);
	
	active_list->nitems--;
	if (index < active_list->nitems) {
	    memcpy(&active_list->items[index], &active_list->items[index+1],
			   (sizeof(fe_NameValueItem) * (active_list->nitems - index)));
	}
	active_list->items[active_list->nitems].name = NULL;
	active_list->items[active_list->nitems].value = NULL;
	active_list->is_dirty = TRUE;
}

static void
fe_document_advanced_delete_cb(Widget widget, XtPointer closure,
									XtPointer cb_data)
{
    fe_EditorDocumentAdvancedPropertiesStruct* w_data = 
	  (fe_EditorDocumentAdvancedPropertiesStruct*)closure;
	fe_NameValueItemList* active_list = w_data->active_list;

	if (active_list == NULL || active_list->selected_item == NOTHING_SELECTED)
	    return;

	list_delete_item(active_list, active_list->selected_item);
	if (active_list->selected_item > 0)
	    active_list->selected_item--;
	
	w_data->item_is_dirty = w_data->item_is_new = FALSE;
	  
	fe_DependentListCallDependents(widget,
								   w_data->dependents,
								   w_data->active_list->mask,
								   closure);
	w_data->changed |= w_data->active_list->mask;
}

static void
fe_document_advanced_delete_update_cb(Widget widget, XtPointer closure,
									XtPointer cb_data)
{
    fe_EditorDocumentAdvancedPropertiesStruct* w_data = 
	  (fe_EditorDocumentAdvancedPropertiesStruct*)closure;
	Boolean sensitive =
		(w_data->item_is_new == FALSE) &&
		(w_data->active_list != NULL) &&
		(w_data->active_list->selected_item != NOTHING_SELECTED);
	
	XtVaSetValues(widget, XmNsensitive, sensitive, 0);
}

static void
fe_document_advanced_set_cb(Widget widget, XtPointer closure,
									XtPointer cb_data)
{
    fe_EditorDocumentAdvancedPropertiesStruct* w_data = 
	  (fe_EditorDocumentAdvancedPropertiesStruct*)closure;
	char* name;
	char* value;

	if (w_data->active_list == NULL)
	    return;

	name = XmTextFieldGetString(w_data->name_text);
	value = XmTextFieldGetString(w_data->value_text);

	if (w_data->item_is_new)
		w_data->active_list->selected_item = w_data->active_list->nitems;

	list_set_item(w_data->active_list, w_data->active_list->selected_item,
				  name, value);

	XtFree(name);
	XtFree(value);

	w_data->item_is_new = FALSE;
	w_data->item_is_dirty = FALSE;

	fe_DependentListCallDependents(widget,
								   w_data->dependents,
								   w_data->active_list->mask,
								   closure);
	w_data->changed |= w_data->active_list->mask;
}

static void
fe_document_advanced_set_update_cb(Widget widget, XtPointer closure,
									XtPointer cb_data)
{
    fe_EditorDocumentAdvancedPropertiesStruct* w_data = 
	  (fe_EditorDocumentAdvancedPropertiesStruct*)closure;
	Boolean sensitive = (w_data->item_is_dirty);
	
	XtVaSetValues(widget, XmNsensitive, sensitive, 0);
}

static void
fe_document_advanced_init(MWContext* context,
						  fe_EditorDocumentAdvancedPropertiesStruct* w_data)
{
	int i;
	fe_NameValueItem* list;

	list = fe_EditorDocumentGetHttpEquivMetaDataList(context);
	w_data->system_list.items = list;
	for (i = 0; list[i].name != NULL; i++)
		;
	w_data->system_list.nitems = i;
	w_data->system_list.size = i;
	w_data->system_list.is_dirty = TRUE;
	w_data->system_list.selected_item = NOTHING_SELECTED;
	w_data->system_list.mask = DOCUMENT_ADVANCED_HTTP_LIST;
	
	list = fe_EditorDocumentGetAdvancedMetaDataList(context);
	w_data->user_list.items = list;
	for (i = 0; list[i].name != NULL; i++)
		;
	w_data->user_list.nitems = i;
	w_data->user_list.size = i;
	w_data->user_list.is_dirty = TRUE;
	w_data->user_list.selected_item = NOTHING_SELECTED;
	w_data->user_list.mask = DOCUMENT_ADVANCED_META_LIST;

	fe_DependentListCallDependents(NULL,
								   w_data->dependents,
								   ~0,
								   (XtPointer)w_data);
}

static void
fe_document_advanced_set(MWContext* context,
						  fe_EditorDocumentAdvancedPropertiesStruct* w_data)
{
	if ((w_data->changed & DOCUMENT_ADVANCED_HTTP_LIST) != 0) {
		fe_EditorDocumentSetHttpEquivMetaDataList(context,
												  w_data->system_list.items);
	}

	if ((w_data->changed & DOCUMENT_ADVANCED_META_LIST) != 0) {
		fe_EditorDocumentSetAdvancedMetaDataList(context,
												 w_data->user_list.items);
	}
}

static Widget
fe_document_advanced_create(
						  MWContext* context,
						  Widget parent,
						  fe_EditorDocumentAdvancedPropertiesStruct* w_data)
{
	Widget form;
	Widget system_label;
	Widget system_list;
	Widget user_label;
	Widget user_list;
	Widget name_label;
	Widget name_text;
	Widget name_new;
	Widget name_set;
	Widget value_label;
	Widget value_text;
	Widget name_delete;
	Widget list_parent;
	Widget fat_guy;
	Dimension left_offset;
	Arg args[16];
	Cardinal n;
	Widget children[12];
	Cardinal nchildren;
	Dimension width;

#if 1
	form = parent;
#else
	n = 0;
	form = XmCreateForm(parent, "advanced", args, n);
	XtManageChild(form);
#endif

	nchildren = 0;
	n = 0;
	XtSetArg(args[n], XmNtopAttachment, XmATTACH_FORM); n++;
	XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM); n++;
	XtSetArg(args[n], XmNalignment, XmALIGNMENT_BEGINNING); n++;
	system_label = XmCreateLabelGadget(form, "systemLabel", args, n);
	children[nchildren++] = system_label;

#define SCROLLED_LIST_SIZE 5
	n = 0;
	XtSetArg(args[n], XmNtopAttachment, XmATTACH_WIDGET); n++;
	XtSetArg(args[n], XmNtopWidget, system_label); n++;
	XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM); n++;
	XtSetArg(args[n], XmNrightAttachment, XmATTACH_FORM); n++;
	XtSetArg(args[n], XmNvisibleItemCount, SCROLLED_LIST_SIZE); n++;
	XtSetArg(args[n], XmNselectionPolicy, XmSINGLE_SELECT); n++;
	XtSetArg(args[n], XmNlistSizePolicy, XmCONSTANT); n++;
	system_list = XmCreateScrolledList(form, "systemList", args, n);
	XtManageChild(system_list);
	children[nchildren++] = list_parent = XtParent(system_list);
	
	n = 0;
	XtSetArg(args[n], XmNtopAttachment, XmATTACH_WIDGET); n++;
	XtSetArg(args[n], XmNtopWidget, list_parent); n++;
	XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM); n++;
	XtSetArg(args[n], XmNalignment, XmALIGNMENT_BEGINNING); n++;
	user_label = XmCreateLabelGadget(form, "userLabel", args, n);
	children[nchildren++] = user_label;

	n = 0;
	XtSetArg(args[n], XmNtopAttachment, XmATTACH_WIDGET); n++;
	XtSetArg(args[n], XmNtopWidget, user_label); n++;
	XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM); n++;
	XtSetArg(args[n], XmNrightAttachment, XmATTACH_FORM); n++;
	XtSetArg(args[n], XmNvisibleItemCount, SCROLLED_LIST_SIZE); n++;
	XtSetArg(args[n], XmNselectionPolicy, XmSINGLE_SELECT); n++;
	XtSetArg(args[n], XmNlistSizePolicy, XmCONSTANT); n++;
	user_list = XmCreateScrolledList(form, "userList", args, n);
	XtManageChild(user_list);
	children[nchildren++] = list_parent = XtParent(user_list);

	n = 0;
	XtSetArg(args[n], XmNtopAttachment, XmATTACH_WIDGET); n++;
	XtSetArg(args[n], XmNtopWidget, list_parent); n++;
	XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM); n++;
	XtSetArg(args[n], XmNalignment, XmALIGNMENT_BEGINNING); n++;
	name_label = XmCreateLabelGadget(form, "nameLabel", args, n);
	children[nchildren++] = name_label;

	n = 0;
	value_label = XmCreateLabelGadget(form, "valueLabel", args, n);
	children[nchildren++] = value_label;

	left_offset = fe_get_offset_from_widest(&children[nchildren-2], 2);

	n = 0;
	XtSetArg(args[n], XmNtopAttachment, XmATTACH_WIDGET); n++;
	XtSetArg(args[n], XmNtopWidget, list_parent); n++;
	XtSetArg(args[n], XmNrightAttachment, XmATTACH_FORM); n++;
	name_new = XmCreatePushButtonGadget(form, "new", args, n);
	children[nchildren++] = name_new;

	n = 0;
	XtSetArg(args[n], XmNtopAttachment, XmATTACH_WIDGET); n++;
	XtSetArg(args[n], XmNtopWidget, list_parent); n++;
	XtSetArg(args[n], XmNrightAttachment, XmATTACH_WIDGET); n++;
	XtSetArg(args[n], XmNrightWidget, name_new); n++;
	name_delete = XmCreatePushButtonGadget(form, "delete", args, n);
	children[nchildren++] = name_delete;

	n = 0;
	XtSetArg(args[n], XmNtopAttachment, XmATTACH_WIDGET); n++;
	XtSetArg(args[n], XmNtopWidget, list_parent); n++;
	XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM); n++;
	XtSetArg(args[n], XmNleftOffset, left_offset); n++;
	XtSetArg(args[n], XmNrightAttachment, XmATTACH_WIDGET); n++;
	XtSetArg(args[n], XmNrightWidget, name_delete); n++;
	name_text = fe_CreateTextField(form, "nameText", args, n);
	children[nchildren++] = name_text;

	n = 0;
	XtSetArg(args[n], XmNtopAttachment, XmATTACH_WIDGET); n++;
	XtSetArg(args[n], XmNtopWidget, name_text); n++;
	XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM); n++;
	XtSetArg(args[n], XmNalignment, XmALIGNMENT_BEGINNING); n++;
	XtSetValues(value_label, args, n);

	n = 0;
	XtSetArg(args[n], XmNtopAttachment, XmATTACH_WIDGET); n++;
	XtSetArg(args[n], XmNtopWidget, name_text); n++;
	XtSetArg(args[n], XmNrightAttachment, XmATTACH_FORM); n++;
	name_set = XmCreatePushButtonGadget(form, "set", args, n);
	children[nchildren++] = name_set;

	n = 0;
	XtSetArg(args[n], XmNtopAttachment, XmATTACH_WIDGET); n++;
	XtSetArg(args[n], XmNtopWidget, name_text); n++;
	XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM); n++;
	XtSetArg(args[n], XmNleftOffset, left_offset); n++;
	XtSetArg(args[n], XmNrightAttachment, XmATTACH_WIDGET); n++;
	XtSetArg(args[n], XmNrightWidget, name_set); n++;
	value_text = fe_CreateTextField(form, "valueText", args, n);
	children[nchildren++] = value_text;

	XtManageChildren(children, nchildren);

	nchildren = 0;
	children[nchildren++] = name_new;
	children[nchildren++] = name_delete;
	children[nchildren++] = name_set;
	fat_guy = fe_GetBiggestWidget(TRUE, children, nchildren);

	XtVaGetValues(fat_guy, XmNwidth, &width, 0);
	for (n = 0; n < nchildren; n++) {
		XtVaSetValues(children[n], XmNwidth, width, 0);
	}

	XtAddCallback(system_list, XmNsingleSelectionCallback,
				  fe_document_advanced_list_cb, (XtPointer)w_data);
	fe_DependentListAddDependent(&w_data->dependents,
								 system_list,
								 (DOCUMENT_ADVANCED_SELECTION|DOCUMENT_ADVANCED_HTTP_LIST),
								 fe_document_advanced_list_update_cb,
								 (XtPointer)w_data);
	XtInsertEventHandler(system_list, ButtonPressMask, False, 
						 fe_document_advanced_tell_me_eh, (XtPointer)w_data,
						 XtListTail);

	XtAddCallback(user_list, XmNsingleSelectionCallback,
				  fe_document_advanced_list_cb, (XtPointer)w_data);
	fe_DependentListAddDependent(&w_data->dependents,
								 user_list,
								 (DOCUMENT_ADVANCED_SELECTION|DOCUMENT_ADVANCED_META_LIST),
								 fe_document_advanced_list_update_cb,
								 (XtPointer)w_data);
	XtInsertEventHandler(user_list, ButtonPressMask, False, 
						 fe_document_advanced_tell_me_eh, (XtPointer)w_data,
						 XtListTail);

	XtAddCallback(name_text, XmNvalueChangedCallback,
				  fe_document_advanced_value_cb, (XtPointer)w_data);
	fe_DependentListAddDependent(&w_data->dependents,
						 name_text,
						 (DOCUMENT_ADVANCED_HTTP_LIST|DOCUMENT_ADVANCED_META_LIST|DOCUMENT_ADVANCED_SELECTION),
						 fe_document_advanced_name_update_cb,
						 (XtPointer)w_data);

	XtAddCallback(value_text, XmNvalueChangedCallback,
				  fe_document_advanced_value_cb, (XtPointer)w_data);
	fe_DependentListAddDependent(&w_data->dependents,
						 value_text,
						 (DOCUMENT_ADVANCED_HTTP_LIST|DOCUMENT_ADVANCED_META_LIST|DOCUMENT_ADVANCED_SELECTION),
						 fe_document_advanced_value_update_cb,
						 (XtPointer)w_data);

	XtAddCallback(name_new, XmNactivateCallback,
				  fe_document_advanced_new_cb,(XtPointer)w_data);
#if 0
	fe_DependentListAddDependent(&w_data->dependents,
						 name_new,
						 (DOCUMENT_ADVANCED_HTTP_LIST|DOCUMENT_ADVANCED_META_LIST|DOCUMENT_ADVANCED_SELECTION),
						 fe_document_advanced_new_update_cb,
						 (XtPointer)w_data);
#endif

	XtAddCallback(name_delete, XmNactivateCallback,
				  fe_document_advanced_delete_cb,(XtPointer)w_data);
	fe_DependentListAddDependent(&w_data->dependents,
						 name_delete,
						 (DOCUMENT_ADVANCED_HTTP_LIST|DOCUMENT_ADVANCED_META_LIST|DOCUMENT_ADVANCED_SELECTION),
						 fe_document_advanced_delete_update_cb,
						 (XtPointer)w_data);

	XtAddCallback(name_set, XmNactivateCallback,
				  fe_document_advanced_set_cb,(XtPointer)w_data);
	fe_DependentListAddDependent(&w_data->dependents,
						 name_set,
						 (DOCUMENT_ADVANCED_HTTP_LIST|DOCUMENT_ADVANCED_META_LIST|DOCUMENT_ADVANCED_ITEM
						  |DOCUMENT_ADVANCED_SELECTION),
						 fe_document_advanced_set_update_cb,
						 (XtPointer)w_data);
	w_data->name_text = name_text;
	w_data->value_text = value_text;
	w_data->system_list.widget = system_list;
	w_data->user_list.widget = user_list;
	w_data->context = context;

	return form;
}

typedef struct fe_EditorDocumentPropertiesStruct
{
  MWContext* context;
  fe_EditorDocumentAppearancePropertiesStruct* appearance;
  fe_EditorDocumentGeneralPropertiesStruct*    general;
  fe_EditorDocumentAdvancedPropertiesStruct*   advanced;
} fe_EditorDocumentPropertiesStruct;

static Widget
fe_EditorDocumentPropertiesCreate(MWContext* context,
								  fe_EditorDocumentPropertiesStruct* p_data)
{
	Widget   dialog;
	Widget   form;
	Widget   tab_form;
	char*    name = "documentPropertiesDialog";
	Arg      args[8];
	Cardinal n;
	
	/*
	 *    Make prompt with ok, apply, cancel, no separator.
	 */
	dialog = fe_CreatePromptDialog(context, name,
								   TRUE, TRUE, TRUE, FALSE, TRUE);

	form = XtVaCreateManagedWidget(
								   "folder",
								   xmlFolderWidgetClass, dialog,
								   XmNshadowThickness, 2,
								   XmNtopAttachment, XmATTACH_FORM,
								   XmNleftAttachment, XmATTACH_FORM,
								   XmNrightAttachment, XmATTACH_FORM,
								   XmNbottomAttachment, XmATTACH_FORM,
#ifdef ALLOW_TAB_ROTATE
								   XmNtabPlacement, XmFOLDER_LEFT,
								   XmNrotateWhenLeftRight, FALSE,
#endif /* ALLOW_TAB_ROTATE */
								   XmNbottomOffset, 3,
								   XmNspacing, 1,
								   NULL);

	n = 0;
	tab_form = fe_CreateTabForm(form, "appearanceProperties", args, n);
	fe_document_appearance_create(context, tab_form, p_data->appearance);
	
	tab_form = fe_CreateTabForm(form, "generalProperties", args, n);
	fe_document_general_create(context, tab_form, p_data->general);

	tab_form = fe_CreateTabForm(form, "advanced", args, n);
	fe_document_advanced_create(context, tab_form, p_data->advanced);

	XtManageChild(dialog);

	return form;
}

void
fe_EditorDocumentPropertiesDialogDo(MWContext* context, 
							  fe_EditorDocumentPropertiesDialogType tab_type)
{
	fe_EditorDocumentPropertiesStruct properties;
	fe_EditorDocumentAppearancePropertiesStruct appearance;
	fe_EditorDocumentGeneralPropertiesStruct general;
	fe_EditorDocumentAdvancedPropertiesStruct advanced;
	int done;
    Widget dialog;
    Widget form;
	Widget apply_button;
	Boolean apply_sensitized;
	unsigned tab_number;

	/*
	 *    Pick the tab.
	 */
	tab_number = tab_type - 1;

	memset(&properties, 0, sizeof(fe_EditorDocumentPropertiesStruct));
	memset(&appearance, 0,
		   sizeof(fe_EditorDocumentAppearancePropertiesStruct));
	memset(&general, 0, sizeof(fe_EditorDocumentGeneralPropertiesStruct));
	memset(&advanced, 0, sizeof(fe_EditorDocumentAdvancedPropertiesStruct));

	/*
	 *    I'll show you mine if you show me yours.
	 */
#if 0
	appearance.properties = &properties;
	general.properties = &properties;
	advanced.properties = &properties;
#endif
	
	properties.appearance = &appearance;
	properties.general = &general;
	properties.advanced = &advanced;

	properties.context = context;
	form = fe_EditorDocumentPropertiesCreate(context, &properties);
	dialog = XtParent(form);

	fe_document_appearance_init(context, &appearance);
	fe_document_general_init(context, &general);
	fe_document_advanced_init(context, &advanced);

	/*
	 *    Initialize.
	 */
#if 0
	fe_DependentListCallDependents(NULL, properties.dependents,
								   ~0, (XtPointer)&properties);
#endif

	/*
	 *   Add a bunch of callbacks to the buttons.
	 */
	XtAddCallback(dialog, XmNokCallback, fe_hrule_ok_cb, &done);
	XtAddCallback(dialog, XmNapplyCallback, fe_hrule_apply_cb, &done);
	XtAddCallback(dialog, XmNcancelCallback, fe_hrule_cancel_cb, &done);
	XtAddCallback(dialog, XmNdestroyCallback, fe_hrule_destroy_cb, &done);

	/*
	 *    We toggle the sensitivity of the apply button on/off
	 *    depending if there are changes to apply. It would be
	 *    nice to use the depdency meahcnism, but it might get
	 *    very busy.
	 */
	apply_button = XmSelectionBoxGetChild(dialog, XmDIALOG_APPLY_BUTTON);
	XtVaSetValues(apply_button, XmNsensitive, FALSE, 0);
	apply_sensitized = FALSE;

    /*
     *    Popup.
     */
    XtManageChild(form);

	XmLFolderSetActiveTab(form, tab_number, True);

	/*
     *    Wait.
     */
	fe_NukeBackingStore(dialog); /* what does this do? */

	done = XmDIALOG_NONE;
	while (done == XmDIALOG_NONE) {

		fe_EventLoop();

		if (done == XFE_DIALOG_DESTROY_BUTTON||done == XmDIALOG_CANCEL_BUTTON)
			break;

#define SOMETHING_CHANGED() \
(appearance.changed != 0 || general.changed != 0 || advanced.changed != 0)

		if (SOMETHING_CHANGED() && apply_sensitized == FALSE) {
			XtVaSetValues(apply_button, XmNsensitive, TRUE, 0);
			apply_sensitized = TRUE;
		}

		if (done == XmDIALOG_APPLY_BUTTON || done == XmDIALOG_OK_BUTTON) {
			/* apply */

			if (SOMETHING_CHANGED()) {

				EDT_BeginBatchChanges(context);

				if (appearance.changed != 0) {
				    fe_document_appearance_set(context, &appearance);
					appearance.changed = 0;
				}
				if (general.changed != 0) {
				    fe_document_general_set(context, &general);
					general.changed = 0;
				}
				if (advanced.changed != 0) {
				    fe_document_advanced_set(context, &advanced);
					advanced.changed = 0;
				}

				EDT_EndBatchChanges(context);
			}

			if (done == XmDIALOG_APPLY_BUTTON) {
				done = XmDIALOG_NONE; /* keep looping */

				XtVaSetValues(apply_button, XmNsensitive, FALSE, 0);
				apply_sensitized = FALSE;
			}
		}
	}
#undef SOMETHING_CHANGED

    /*
     *    Unload data.
     */
	fe_DependentListDestroy(appearance.dependents);

	if (done != XFE_DIALOG_DESTROY_BUTTON)
		XtDestroyWidget(dialog);
}

typedef struct fe_EditorGeneralPreferencesStruct
{
	MWContext* context;

    Widget author;
    Widget html_editor;
    Widget image_editor;
    Widget template;
	Widget autosave_toggle;
	Widget autosave_text;

    unsigned changed;
    
} fe_EditorGeneralPreferencesStruct;

#define EDITOR_GENERAL_AUTHOR          (0x1<<0)
#define EDITOR_GENERAL_HTML_EDITOR     (0x1<<1)
#define EDITOR_GENERAL_IMAGE_EDITOR    (0x1<<2)
#define EDITOR_GENERAL_TEMPLATE        (0x1<<3)
#define EDITOR_GENERAL_AUTOSAVE        (0x1<<4)

static void
fe_general_preferences_restore_template_cb(Widget widget,
							   XtPointer closure, XtPointer call_data)
{
    char* value;
	fe_EditorGeneralPreferencesStruct* w_data
		= (fe_EditorGeneralPreferencesStruct*)closure;

	/* get the template */
	if ((value = fe_EditorDefaultGetTemplate()) == NULL)
	    value = "";

	XmTextFieldSetString(w_data->template, value);

	w_data->changed |= EDITOR_GENERAL_TEMPLATE;
}

static void
fe_general_preferences_changed_cb(Widget widget,
							   XtPointer closure, XtPointer call_data)
{
	fe_EditorGeneralPreferencesStruct* w_data
		= (fe_EditorGeneralPreferencesStruct*)closure;
    unsigned mask = (unsigned)fe_GetUserData(widget);

	w_data->changed |= mask;
}

static void
fe_general_preferences_autosave_toggle_cb(Widget widget,
							   XtPointer closure, XtPointer call_data)
{
	fe_EditorGeneralPreferencesStruct* w_data
		= (fe_EditorGeneralPreferencesStruct*)closure;
	XmToggleButtonCallbackStruct* cb
		= (XmToggleButtonCallbackStruct*)call_data;

	fe_TextFieldSetEditable(w_data->context, w_data->autosave_text, cb->set);
	w_data->changed |= EDITOR_GENERAL_AUTOSAVE;
}

static void
fe_general_preferences_init(MWContext* context,
						 fe_EditorGeneralPreferencesStruct* w_data)
{
	char* value;
	char* value2;
	Boolean as_enable;
	unsigned as_time;

	/* get the author */
	if ((value = fe_EditorPreferencesGetAuthor(context)) != NULL)
		XmTextFieldSetString(w_data->author, value);

	/* get the editors */
	fe_EditorPreferencesGetEditors(context, &value, &value2);
	if (value != NULL)
		XmTextFieldSetString(w_data->html_editor, value);

	if (value2 != NULL)
		XmTextFieldSetString(w_data->image_editor, value2);

	/* get the template */
	if ((value = fe_EditorPreferencesGetTemplate(context)))
		XmTextFieldSetString(w_data->template, value);

	/* get the autosave state */
	fe_EditorPreferencesGetAutoSave(context, &as_enable, &as_time);
	if (!as_enable)
		as_time = 10;
	
	fe_set_numeric_text_field(w_data->autosave_text, as_time);
	fe_TextFieldSetEditable(context, w_data->autosave_text, as_enable);
	XmToggleButtonGadgetSetState(w_data->autosave_toggle, as_enable, FALSE);

	w_data->context = context;
	w_data->changed = 0;
}

static Boolean
fe_general_preferences_validate(MWContext* context,
						fe_EditorGeneralPreferencesStruct* w_data)
{
	Boolean as_enable;
	int as_time;

	/* autosave */
	if ((w_data->changed & EDITOR_GENERAL_AUTOSAVE) != 0) {
		as_time = fe_get_numeric_text_field(w_data->autosave_text);
		as_enable = XmToggleButtonGadgetGetState(w_data->autosave_toggle);
		
		if (as_time == 0)
			as_enable = FALSE;
		
		if (as_enable) {
			if (RANGE_CHECK(as_time,AUTOSAVE_MIN_PERIOD,AUTOSAVE_MAX_PERIOD)) {
				char* msg = XP_GetString(XFE_EDITOR_AUTOSAVE_PERIOD_RANGE);
				fe_error_dialog(context, w_data->autosave_text, msg);
				return FALSE;
			}
		}
	}
	return TRUE;
}

static void
fe_general_preferences_set(MWContext* context,
						fe_EditorGeneralPreferencesStruct* w_data)
{
	char* value;
	Boolean as_enable;
	unsigned as_time;

	/* author */
	if ((w_data->changed & EDITOR_GENERAL_AUTHOR) != 0) {
		value = XmTextFieldGetString(w_data->author);
		fe_EditorPreferencesSetAuthor(context, value);
		XtFree(value);
	}
	
	/* editors */
	value = NULL;
	if ((w_data->changed & EDITOR_GENERAL_HTML_EDITOR) != 0) {
		value = XmTextGetString(w_data->html_editor);
		fe_EditorPreferencesSetEditors(context, value, NULL);
		XtFree(value);
	}

	if ((w_data->changed & EDITOR_GENERAL_IMAGE_EDITOR) != 0) {
		value = XmTextGetString(w_data->image_editor);
		fe_EditorPreferencesSetEditors(context, NULL, value);
		XtFree(value);
	}

	/* template */
	if ((w_data->changed & EDITOR_GENERAL_TEMPLATE) != 0) {
		value = XmTextGetString(w_data->template);
		fe_EditorPreferencesSetTemplate(context, value);
		XtFree(value);
	}

	/* autosave */
	if ((w_data->changed & EDITOR_GENERAL_AUTOSAVE) != 0) {
		as_time = fe_get_numeric_text_field(w_data->autosave_text);
		as_enable = XmToggleButtonGadgetGetState(w_data->autosave_toggle);
		fe_EditorPreferencesSetAutoSave(context, as_enable, as_time);
	}

	w_data->changed = 0;
}

static Widget
fe_general_preferences_create(MWContext* context, Widget parent,
							 fe_EditorGeneralPreferencesStruct* w_data)
{
    Widget main_rc;
	Widget author_frame;
	Widget author_text;
	Widget external_frame;
	Widget external_form;
	Widget html_label;
	Widget html_text;
	Widget html_browse;
	Widget image_label;
	Widget image_text;
	Widget image_browse;
	Widget template_frame;
	Widget template_form;
	Widget template_label;
	Widget template_text;
	Widget template_info_label;
	Widget template_restore;
	Widget autosave_frame;
	Widget autosave_form;
	Widget autosave_toggle;
	Widget autosave_text;
	Widget autosave_label;
	Widget children[8];
	Cardinal nchildren;
	Dimension left_offset;
	Dimension right_offset;
	Arg    args[8];
	Cardinal n;
	Cardinal i;

	n = 0;
	XtSetArg(args[n], XmNorientation, XmVERTICAL); n++;
	main_rc = XmCreateRowColumn(parent, "general", args, n);
	XtManageChild(main_rc);

	n = 0;
	author_frame = fe_CreateFrame(main_rc, "author", args, n);
	XtManageChild(author_frame);

	n = 0;
	XtSetArg(args[n], XmNuserData, EDITOR_GENERAL_AUTHOR); n++;
	author_text = XmCreateTextField(author_frame, "authorText", args, n);
	XtManageChild(author_text);
	w_data->author = author_text;

	XtAddCallback(author_text, XmNvalueChangedCallback,
				  fe_general_preferences_changed_cb, (XtPointer)w_data);

	n = 0;
	external_frame = fe_CreateFrame(main_rc, "external", args, n);
	XtManageChild(external_frame);

	n = 0;
	external_form = XmCreateForm(external_frame, "external", args, n);
	XtManageChild(external_form);

	nchildren = 0;
	n = 0;
	html_label = XmCreateLabelGadget(external_form, "htmlLabel", args, n);
	children[nchildren++] = html_label;

	n = 0;
	image_label = XmCreateLabelGadget(external_form, "imageLabel", args, n);
	children[nchildren++] = image_label;

	left_offset = fe_get_offset_from_widest(children, nchildren);

	n = 0;
	html_browse = XmCreatePushButtonGadget(external_form, "browse", args, n);
	children[nchildren++] = html_browse;

	n = 0;
	image_browse = XmCreatePushButtonGadget(external_form, "browse", args, n);
	children[nchildren++] = image_browse;

	right_offset = fe_get_offset_from_widest(&children[2], 2);

	n = 0;
	XtSetArg(args[n], XmNtopAttachment, XmATTACH_FORM); n++;
	XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM); n++;
	XtSetArg(args[n], XmNleftOffset, left_offset); n++;
	XtSetArg(args[n], XmNrightAttachment, XmATTACH_FORM); n++;
	XtSetArg(args[n], XmNrightOffset, right_offset); n++;
	XtSetArg(args[n], XmNuserData, EDITOR_GENERAL_HTML_EDITOR); n++;
	html_text = XmCreateTextField(external_form, "htmlText", args, n);
	children[nchildren++] = html_text;
	w_data->html_editor = html_text;

	XtAddCallback(html_text, XmNvalueChangedCallback,
				  fe_general_preferences_changed_cb, (XtPointer)w_data);

	n = 0;
	XtSetArg(args[n], XmNtopAttachment, XmATTACH_WIDGET); n++;
	XtSetArg(args[n], XmNtopWidget, html_text); n++;
	XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM); n++;
	XtSetArg(args[n], XmNleftOffset, left_offset); n++;
	XtSetArg(args[n], XmNrightAttachment, XmATTACH_FORM); n++;
	XtSetArg(args[n], XmNrightOffset, right_offset); n++;
	XtSetArg(args[n], XmNuserData, EDITOR_GENERAL_IMAGE_EDITOR); n++;
	image_text = XmCreateTextField(external_form, "imageText", args, n);
	children[nchildren++] = image_text;
	w_data->image_editor = image_text;

	XtAddCallback(image_text, XmNvalueChangedCallback,
				  fe_general_preferences_changed_cb, (XtPointer)w_data);

	/*
	 *    Go back for browse callbacks
	 */
	XtVaSetValues(image_browse, XmNuserData, image_text, 0);
	XtAddCallback(image_browse, XmNactivateCallback,
				  fe_browse_to_text_field_cb, (XtPointer)context);

	XtVaSetValues(html_browse, XmNuserData, html_text, 0);
	XtAddCallback(html_browse, XmNactivateCallback,
				  fe_browse_to_text_field_cb, (XtPointer)context);


	/*
	 *    Go back and attach the labels and browse buttons.
	 */
	for (i = 0; i < 4; i++) {
	  n = 0;
	  if ((i & 0x1) == 0) { /* even, therefore topmost of pair */
		XtSetArg(args[n], XmNtopAttachment, XmATTACH_FORM); n++;
	  } else {
		XtSetArg(args[n], XmNtopAttachment, XmATTACH_WIDGET); n++;
		XtSetArg(args[n], XmNtopWidget, html_text); n++;
	  }

	  if (i < 2) { /* label, attach to left */
		XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM); n++;
	  } else { /* button, attach to right */
		XtSetArg(args[n], XmNrightAttachment, XmATTACH_FORM); n++;
	  }
	  XtSetValues(children[i], args, n);
	}

	XtManageChildren(children, nchildren);

	n = 0;
	template_frame = fe_CreateFrame(main_rc, "template", args, n);
	XtManageChild(template_frame);

	n = 0;
	template_form = XmCreateForm(template_frame, "template", args, n);
	XtManageChild(template_form);

	nchildren = 0;
	n = 0;
	XtSetArg(args[n], XmNtopAttachment, XmATTACH_FORM); n++;
	XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM); n++;
	template_label = XmCreateLabelGadget(template_form, "locationLabel",
										 args, n);
	children[nchildren++] = template_label;

	n = 0;
	XtSetArg(args[n], XmNtopAttachment, XmATTACH_FORM); n++;
	XtSetArg(args[n], XmNleftAttachment, XmATTACH_WIDGET); n++;
	XtSetArg(args[n], XmNrightAttachment, XmATTACH_FORM); n++;
	XtSetArg(args[n], XmNleftWidget, template_label); n++;
	XtSetArg(args[n], XmNuserData, EDITOR_GENERAL_TEMPLATE); n++;
	template_text = XmCreateTextField(template_form, "templateText", args, n);
	children[nchildren++] = template_text;
	w_data->template = template_text;

	XtAddCallback(template_text, XmNvalueChangedCallback,
				  fe_general_preferences_changed_cb, (XtPointer)w_data);

	n = 0;
	XtSetArg(args[n], XmNtopAttachment, XmATTACH_WIDGET); n++;
	XtSetArg(args[n], XmNtopWidget, template_text); n++;
	XtSetArg(args[n], XmNrightAttachment, XmATTACH_FORM); n++;
	template_restore = XmCreatePushButtonGadget(template_form,
												"restoreDefault", args, n);
	children[nchildren++] = template_restore;

	n = 0;
	XtSetArg(args[n], XmNtopAttachment, XmATTACH_WIDGET); n++;
	XtSetArg(args[n], XmNtopWidget, template_text); n++;
	XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM); n++;
	XtSetArg(args[n], XmNrightAttachment, XmATTACH_WIDGET); n++;
	XtSetArg(args[n], XmNrightWidget, template_restore); n++;
	XtSetArg(args[n], XmNalignment, XmALIGNMENT_BEGINNING); n++;
	template_info_label = XmCreateLabelGadget(template_form, "templateInfo",
										 args, n);
	children[nchildren++] = template_info_label;

	XtAddCallback(template_restore, XmNactivateCallback,
				  fe_general_preferences_restore_template_cb,
				  (XtPointer)w_data);

	XtManageChildren(children, nchildren);

	/*
	 *    Auto Save.
	 */
	n = 0;
	autosave_frame = fe_CreateFrame(main_rc, "autosave", args, n);
	XtManageChild(autosave_frame);

	n = 0;
	autosave_form = XmCreateForm(autosave_frame, "autosave", args, n);
	XtManageChild(autosave_form);

	nchildren = 0;
	n = 0;
	XtSetArg(args[n], XmNtopAttachment, XmATTACH_FORM); n++;
	XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM); n++;
	XtSetArg(args[n], XmNindicatorType, XmN_OF_MANY); n++;
	autosave_toggle = XmCreateToggleButtonGadget(autosave_form, 
												 "autosaveEnable", args, n);
	children[nchildren++] = autosave_toggle;

	XtAddCallback(autosave_toggle, XmNvalueChangedCallback,
				  fe_general_preferences_autosave_toggle_cb,
				  (XtPointer)w_data);
	w_data->autosave_toggle = autosave_toggle;

	n = 0;
	XtSetArg(args[n], XmNtopAttachment, XmATTACH_FORM); n++;
	XtSetArg(args[n], XmNleftAttachment, XmATTACH_WIDGET); n++;
	XtSetArg(args[n], XmNleftWidget, autosave_toggle); n++;
	XtSetArg(args[n], XmNcolumns, 4); n++;
	XtSetArg(args[n], XmNuserData, EDITOR_GENERAL_AUTOSAVE); n++;
	autosave_text = XmCreateTextField(autosave_form, "autosaveText", args, n);
	children[nchildren++] = autosave_text;

	XtAddCallback(autosave_text, XmNvalueChangedCallback,
				  fe_general_preferences_changed_cb,
				  (XtPointer)w_data);
	w_data->autosave_text = autosave_text;

	n = 0;
	XtSetArg(args[n], XmNtopAttachment, XmATTACH_FORM); n++;
	XtSetArg(args[n], XmNleftAttachment, XmATTACH_WIDGET); n++;
	XtSetArg(args[n], XmNleftWidget, autosave_text); n++;
	autosave_label = XmCreateLabelGadget(autosave_form,	"minutes", args, n);
	children[nchildren++] = autosave_label;

	XtManageChildren(children, nchildren);
	
	return main_rc;
}

typedef struct fe_EditorPublishPreferencesStruct
{
    Widget maintain_links;
    Widget keep_images;
    Widget publish_text;
    Widget browse_text;
    Widget username_text;
    Widget password_text;
    Widget save_password;
    unsigned changed;
} fe_EditorPublishPreferencesStruct;

#define EDITOR_PUBLISH_LINKS           (0x1<<0)
#define EDITOR_PUBLISH_IMAGES          (0x1<<1)
#define EDITOR_PUBLISH_PUBLISH         (0x1<<2)
#define EDITOR_PUBLISH_BROWSE          (0x1<<3)
#define EDITOR_PUBLISH_USERNAME        (0x1<<4)
#define EDITOR_PUBLISH_PASSWORD        (0x1<<5)
#define EDITOR_PUBLISH_PASSWORD_SAVE   (0x1<<6)

static void
fe_publish_page_changed(Widget widget, XtPointer closure, XtPointer cb)
{
	fe_EditorPublishPreferencesStruct* w_data = 
		(fe_EditorPublishPreferencesStruct*)closure;

	w_data->changed |= (unsigned)fe_GetUserData(widget);
}

static void
fe_publish_password_changed(Widget widget, XtPointer closure, XtPointer cb)
{
	fe_EditorPublishPreferencesStruct* w_data = 
		(fe_EditorPublishPreferencesStruct*)closure;

	w_data->changed |= EDITOR_PUBLISH_PASSWORD;
}

static void
fe_publish_preferences_set(MWContext* context,
						   fe_EditorPublishPreferencesStruct* w_data)
{
    char* location = NULL;
    char* browse_location = NULL;
	char* username = NULL;
	char* password = NULL;
	Boolean new_links;
	Boolean new_images;
	Boolean old_links;
	Boolean old_images;

	new_links = XmToggleButtonGetState(w_data->maintain_links);
	new_images = XmToggleButtonGetState(w_data->keep_images);

	fe_EditorPreferencesGetLinksAndImages(context, &old_links, &old_images);

	if (new_links != old_links || new_images != old_images) {
		fe_EditorPreferencesSetLinksAndImages(context, new_links, new_images);
	}

#ifdef _SECURITY_BTN_ON_PREFS
 
	/* don't need save password in prefs anymore - benjie */
	new_save_password = XmToggleButtonGetState(w_data->save_password); 

	old_save_password = fe_EditorPreferencesGetPublishLocation(context, 
															   NULL, NULL, 
															   NULL);
#endif

#define PUBLISH_MASK (EDITOR_PUBLISH_PUBLISH|  \
					  EDITOR_PUBLISH_BROWSE| \
					  EDITOR_PUBLISH_USERNAME| \
					  EDITOR_PUBLISH_PASSWORD|EDITOR_PUBLISH_PASSWORD_SAVE)

#ifdef _SECURITY_BTN_ON_PREFS		
	if (new_save_password != old_save_password || 
		(w_data->changed & PUBLISH_MASK) != 0) { 
#else
	if ((w_data->changed & PUBLISH_MASK) != 0) { 
#endif
		location = XmTextFieldGetString(w_data->publish_text);
		browse_location = XmTextFieldGetString(w_data->browse_text);
		username = XmTextFieldGetString(w_data->username_text);
		password = fe_GetPasswdText(w_data->password_text);
		
		fe_EditorPreferencesSetPublishLocation(context,
											   location,
											   username,
											   password);
/*
											   new_save_password? password: 0);
*/
		fe_EditorPreferencesSetBrowseLocation(context, browse_location);
	}
#undef PUBLISH_MASK
	if (browse_location) {
	    XtFree(browse_location);
	}
	if (location) {
	    XtFree(location);
	}
	if (username) {
	    XtFree(username);
	}
	if (password) {
	    memset(password, 0, strlen(password));
		XtFree(password);
	}
}


static void
fe_publish_preferences_init(MWContext* context,
							fe_EditorPublishPreferencesStruct* w_data)
{
    char* location;
    char* browse_location;
	char* username;
	char* password;
	Boolean links;
	Boolean images;
	Boolean save_password;

	fe_EditorPreferencesGetLinksAndImages(context, &links, &images);

	save_password = fe_EditorPreferencesGetPublishLocation(context,
														   &location,
														   &username,
														   &password);

	browse_location = fe_EditorPreferencesGetBrowseLocation(context);

	XmToggleButtonSetState(w_data->maintain_links, links, FALSE);
	XmToggleButtonSetState(w_data->keep_images, images, FALSE);
	/*XmToggleButtonSetState(w_data->save_password, save_password, FALSE);*/

	if (location) {
	    fe_TextFieldSetString(w_data->publish_text, location, FALSE);

		if (username)
		    fe_TextFieldSetString(w_data->username_text, username, FALSE);
		else
		    fe_TextFieldSetString(w_data->username_text, "", FALSE);

		if (password)
		    fe_TextFieldSetString(w_data->password_text, password, FALSE);
		else
		    fe_TextFieldSetString(w_data->password_text, "", FALSE);

	}

	if (browse_location)
		fe_TextFieldSetString(w_data->browse_text, browse_location, FALSE);
	else
		fe_TextFieldSetString(w_data->browse_text, "", FALSE);

	if (browse_location) {
	    XtFree(browse_location);
	}
	if (location) {
	    XtFree(location);
	}
	if (username) {
	    XtFree(username);
	}
	if (password) {
	    memset(password, 0, strlen(password));
		XtFree(password);
	}
}

static Widget
fe_publish_preferences_create(MWContext* context, Widget parent,
							  fe_EditorPublishPreferencesStruct* w_data)
{
    Widget main_rc;

	Widget links_frame;
	Widget links_main_rc;
	Widget links_main_info;
	Widget links_sub_rc;
	Widget links_toggle;
	Widget links_info;
	Widget images_toggle;
	Widget images_info;
	Widget links_main_tip;

	Widget publish_frame;
	Widget publish_form;
	Widget publish_label;
	Widget publish_text;
	Widget browse_label;
	Widget browse_text;
	Widget username_label;
	Widget username_text;
	Widget password_label;
	Widget password_text;
	Widget children[16];
	Cardinal nchildren;
	Dimension left_offset;
	Arg    args[16];
	Cardinal n;

	n = 0;
	XtSetArg(args[n], XmNorientation, XmVERTICAL); n++;
	main_rc = XmCreateRowColumn(parent, "publish", args, n);
	XtManageChild(main_rc);

	n = 0;
	links_frame = fe_CreateFrame(main_rc, "linksAndImages", args, n);
	XtManageChild(links_frame);

	n = 0;
	XtSetArg(args[n], XmNorientation, XmVERTICAL); n++;
	links_main_rc = XmCreateRowColumn(links_frame, "linksAndImages", args, n);
	XtManageChild(links_main_rc);

	nchildren = 0;
	n = 0;
	XtSetArg(args[n], XmNalignment, XmALIGNMENT_BEGINNING); n++;
	links_main_info = XmCreateLabelGadget(links_main_rc, "linksAndImagesLabel",
										  args, n);
	children[nchildren++] = links_main_info;

	n = 0;
	XtSetArg(args[n], XmNorientation, XmVERTICAL); n++;
	XtSetArg(args[n], XmNisAligned, TRUE); n++;
	XtSetArg(args[n], XmNentryAlignment, XmALIGNMENT_BEGINNING); n++;
	links_sub_rc = XmCreateRowColumn(links_main_rc, "linksAndImagesToggles",
									 args, n);
	children[nchildren++] = links_sub_rc;

	n = 0;
	XtSetArg(args[n], XmNalignment, XmALIGNMENT_BEGINNING); n++;
	links_main_tip = XmCreateLabelGadget(links_main_rc, "linksAndImagesTip",
										  args, n);
	children[nchildren++] = links_main_tip;

	XtManageChildren(children, nchildren);

	nchildren = 0;
	n = 0;
	links_toggle = XmCreateToggleButtonGadget(links_sub_rc, "linksToggle",
										  args, n);
	children[nchildren++] = links_toggle;

	n = 0;
	XtSetArg(args[n], XmNalignment, XmALIGNMENT_BEGINNING); n++;
	links_info = XmCreateLabelGadget(links_sub_rc, "linksInfo", args, n);
	children[nchildren++] = links_info;

	n = 0;
	images_toggle = XmCreateToggleButtonGadget(links_sub_rc, "imagesToggle",
										  args, n);
	children[nchildren++] = images_toggle;

	n = 0;
	XtSetArg(args[n], XmNalignment, XmALIGNMENT_BEGINNING); n++;
	images_info = XmCreateLabelGadget(links_sub_rc, "imagesInfo", args, n);
	children[nchildren++] = images_info;

	XtManageChildren(children, nchildren);

	n = 0;
	publish_frame = fe_CreateFrame(main_rc, "publish", args, n);
	XtManageChild(publish_frame);

	n = 0;
	publish_form = XmCreateForm(publish_frame, "publish", args, n);
	XtManageChild(publish_form);

	nchildren = 0;
	n = 0;
	XtSetArg(args[n], XmNtopAttachment, XmATTACH_FORM); n++;
	XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM); n++;
	publish_label = XmCreateLabelGadget(publish_form, "publishLabel", args, n);
	children[nchildren++] = publish_label;

	n = 0;
	browse_label = XmCreateLabelGadget(publish_form, "browseLabel", args, n);
	children[nchildren++] = browse_label;

	left_offset = fe_get_offset_from_widest(children, 2);

	n = 0;
	XtSetArg(args[n], XmNtopAttachment, XmATTACH_FORM); n++;
	XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM); n++;
	XtSetArg(args[n], XmNrightAttachment, XmATTACH_FORM); n++;
	XtSetArg(args[n], XmNleftOffset, left_offset); n++;
	XtSetArg(args[n], XmNuserData, EDITOR_PUBLISH_PUBLISH); n++;
	publish_text = fe_CreateTextField(publish_form, "publishText", args, n);
	children[nchildren++] = publish_text;
	
	n = 0;
	XtSetArg(args[n], XmNtopAttachment, XmATTACH_WIDGET); n++;
	XtSetArg(args[n], XmNtopWidget, publish_text); n++;
	XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM); n++;
	XtSetArg(args[n], XmNrightAttachment, XmATTACH_FORM); n++;
	XtSetArg(args[n], XmNleftOffset, left_offset); n++;
	XtSetArg(args[n], XmNuserData, EDITOR_PUBLISH_BROWSE); n++;
	browse_text = fe_CreateTextField(publish_form, "browseText", args, n);
	children[nchildren++] = browse_text;

	/*
	 *    Go back for browse label attachments.
	 */
	n = 0;
	XtSetArg(args[n], XmNtopAttachment, XmATTACH_WIDGET); n++;
	XtSetArg(args[n], XmNtopWidget, publish_text); n++;
	XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM); n++;
	XtSetValues(browse_label, args, n);

	n = 0;
	XtSetArg(args[n], XmNtopAttachment, XmATTACH_WIDGET); n++;
	XtSetArg(args[n], XmNtopWidget, browse_text); n++;
	XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM); n++;
	username_label = XmCreateLabelGadget(publish_form, "usernameLabel",
										 args, n);
	children[nchildren++] = username_label;

	n = 0;
	password_label = XmCreateLabelGadget(publish_form, "passwordLabel",
										 args, n);
	children[nchildren++] = password_label;

	left_offset = fe_get_offset_from_widest(&children[4], 2);

	n = 0;
	XtSetArg(args[n], XmNtopAttachment, XmATTACH_WIDGET); n++;
	XtSetArg(args[n], XmNtopWidget, browse_text); n++;
	XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM); n++;
	XtSetArg(args[n], XmNleftOffset, left_offset); n++;
	XtSetArg(args[n], XmNuserData, EDITOR_PUBLISH_USERNAME); n++;
	username_text = fe_CreateTextField(publish_form, "usernameText", args, n);
	children[nchildren++] = username_text;
	
	n = 0;
	XtSetArg(args[n], XmNtopAttachment, XmATTACH_WIDGET); n++;
	XtSetArg(args[n], XmNtopWidget, username_text); n++;
	XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM); n++;
	XtSetArg(args[n], XmNleftOffset, left_offset); n++;
	XtSetArg(args[n], XmNmaxLength, 1024); n++;
	password_text = fe_CreatePasswordField(publish_form, "passwordText",
										   args, n);
	children[nchildren++] = password_text;

	n = 0;
	XtSetArg(args[n], XmNtopAttachment, XmATTACH_WIDGET); n++;
	XtSetArg(args[n], XmNtopWidget, username_text); n++;
	XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM); n++;
	XtSetValues(password_label, args, n);

	/* we don't need this anymore - benjie */
	/* according to the bugsplat */
#ifdef _SECURITY_BTN_ON_PREFS
	n = 0;
	XtSetArg(args[n], XmNtopAttachment, XmATTACH_WIDGET); n++;
	XtSetArg(args[n], XmNtopWidget, username_text); n++;
	XtSetArg(args[n], XmNleftAttachment, XmATTACH_WIDGET); n++;
	XtSetArg(args[n], XmNleftWidget, password_text); n++;
	password_save = XmCreateToggleButtonGadget(publish_form, "savePassword",
										  args, n);
	children[nchildren++] = password_save;
#endif

	XtManageChildren(children, nchildren);

    w_data->maintain_links = links_toggle;
    w_data->keep_images = images_toggle;
    w_data->publish_text = publish_text;
    w_data->browse_text = browse_text;
    w_data->username_text = username_text;
    w_data->password_text = password_text;
#ifdef _SECURITY_BTN_ON_PREFS
    w_data->save_password = password_save; 
#endif

	XtVaSetValues(links_toggle, XmNuserData, EDITOR_PUBLISH_LINKS, 0);
	XtAddCallback(links_toggle, XmNvalueChangedCallback,
				  fe_publish_page_changed, (XtPointer)w_data);
	XtVaSetValues(images_toggle, XmNuserData, EDITOR_PUBLISH_IMAGES, 0);
	XtAddCallback(images_toggle, XmNvalueChangedCallback,
				  fe_publish_page_changed, (XtPointer)w_data);
	XtVaSetValues(publish_text, XmNuserData, EDITOR_PUBLISH_PUBLISH, 0);
	XtAddCallback(publish_text, XmNvalueChangedCallback,
				  fe_publish_page_changed, (XtPointer)w_data);
	XtVaSetValues(browse_text, XmNuserData, EDITOR_PUBLISH_BROWSE, 0);
	XtAddCallback(browse_text, XmNvalueChangedCallback,
				  fe_publish_page_changed, (XtPointer)w_data);
	XtVaSetValues(username_text, XmNuserData, EDITOR_PUBLISH_USERNAME, 0);
	XtAddCallback(username_text, XmNvalueChangedCallback,
				  fe_publish_page_changed, (XtPointer)w_data);

	XtAddCallback(password_text, XmNvalueChangedCallback,
				  fe_publish_password_changed, (XtPointer)w_data);
#ifdef _SECURITY_BTN_ON_PREFS
	XtVaSetValues(password_save, XmNuserData, EDITOR_PUBLISH_PASSWORD_SAVE, 0);
	XtAddCallback(password_save, XmNvalueChangedCallback,
				  fe_publish_page_changed, (XtPointer)w_data);
#endif

	return main_rc;
}

typedef struct fe_EditorPreferencesStruct
{
  fe_EditorGeneralPreferencesStruct*           general;
  fe_EditorDocumentAppearancePropertiesStruct* appearance;
  fe_EditorPublishPreferencesStruct*           publish;
} fe_EditorPreferencesStruct;


static Widget
fe_editor_preferences_dialog_create(MWContext* context,
								  fe_EditorPreferencesStruct* p_data)
{
	Widget   dialog;
	Widget   form;
	Widget   tab_form;
	char*    name = "editorPreferencesDialog";
	
	/*
	 *    Make prompt with ok, apply, cancel, no separator.
	 */
	dialog = fe_CreatePromptDialog(context, name,
								   TRUE, TRUE, TRUE, FALSE, TRUE);

	form = XtVaCreateManagedWidget(
								   "folder",
								   xmlFolderWidgetClass, dialog,
								   XmNshadowThickness, 2,
								   XmNtopAttachment, XmATTACH_FORM,
								   XmNleftAttachment, XmATTACH_FORM,
								   XmNrightAttachment, XmATTACH_FORM,
								   XmNbottomAttachment, XmATTACH_FORM,
#ifdef ALLOW_TAB_ROTATE
								   XmNtabPlacement, XmFOLDER_LEFT,
								   XmNrotateWhenLeftRight, FALSE,
#endif /* ALLOW_TAB_ROTATE */
								   XmNbottomOffset, 3,
								   XmNspacing, 1,
								   NULL);

	tab_form = fe_CreateTabForm(form, "appearanceProperties", NULL, 0);
	fe_document_appearance_create(context, tab_form, p_data->appearance);
	
	tab_form = fe_CreateTabForm(form, "generalPreferences", NULL, 0);
	fe_general_preferences_create(context, tab_form, p_data->general);

	tab_form = fe_CreateTabForm(form, "publishPreferences", NULL, 0);
	fe_publish_preferences_create(context, tab_form, p_data->publish);

	XtManageChild(dialog);

	return form;
}

void
fe_EditorPreferencesDialogDo(MWContext* context, unsigned tab_type)
{
	fe_EditorPreferencesStruct properties;
	fe_EditorDocumentAppearancePropertiesStruct appearance;
	fe_EditorGeneralPreferencesStruct general;
	fe_EditorPublishPreferencesStruct publish;
	int done;
    Widget dialog;
    Widget form;
	Widget apply_button;
	Boolean apply_sensitized;
	unsigned tab_number;

	/*
	 *    Pick the tab.
	 */
	tab_number = tab_type - 1;

	memset(&properties, 0, sizeof(fe_EditorPreferencesStruct));
	memset(&appearance, 0,
		   sizeof(fe_EditorDocumentAppearancePropertiesStruct));
	memset(&general, 0, sizeof(fe_EditorGeneralPreferencesStruct));
	memset(&publish, 0, sizeof(fe_EditorPublishPreferencesStruct));

	properties.appearance = &appearance;
	properties.general = &general;
	properties.publish = &publish;

	form = fe_editor_preferences_dialog_create(context, &properties);
	dialog = XtParent(form);

	appearance.is_editor_preferences = TRUE;
	fe_document_appearance_init(context, &appearance);
	fe_general_preferences_init(context, &general);
	fe_publish_preferences_init(context, &publish);

	/*
	 *   Add a bunch of callbacks to the buttons.
	 */
	XtAddCallback(dialog, XmNokCallback, fe_hrule_ok_cb, &done);
	XtAddCallback(dialog, XmNapplyCallback, fe_hrule_apply_cb, &done);
	XtAddCallback(dialog, XmNcancelCallback, fe_hrule_cancel_cb, &done);
	XtAddCallback(dialog, XmNdestroyCallback, fe_hrule_destroy_cb, &done);

	/*
	 *    We toggle the sensitivity of the apply button on/off
	 *    depending if there are changes to apply. It would be
	 *    nice to use the depdency meahcnism, but it might get
	 *    very busy.
	 */
	apply_button = XmSelectionBoxGetChild(dialog, XmDIALOG_APPLY_BUTTON);
	XtVaSetValues(apply_button, XmNsensitive, FALSE, 0);
	apply_sensitized = FALSE;

    /*
     *    Popup.
     */
    XtManageChild(form);

	XmLFolderSetActiveTab(form, tab_number, True);

	/*
     *    Wait.
     */
	fe_NukeBackingStore(dialog); /* what does this do? */

	done = XmDIALOG_NONE;
	while (done == XmDIALOG_NONE) {

		fe_EventLoop();

		if (done == XFE_DIALOG_DESTROY_BUTTON||done == XmDIALOG_CANCEL_BUTTON)
			break;

#define SOMETHING_CHANGED() \
(appearance.changed != 0 || general.changed != 0 || publish.changed != 0)

		if (SOMETHING_CHANGED() && apply_sensitized == FALSE) {
			XtVaSetValues(apply_button, XmNsensitive, TRUE, 0);
			apply_sensitized = TRUE;
		}

		if (done == XmDIALOG_APPLY_BUTTON || done == XmDIALOG_OK_BUTTON) {
			/* apply */

			if (SOMETHING_CHANGED()) {

				if (!fe_general_preferences_validate(context, &general)) {
					done = XmDIALOG_NONE;
					continue; /* you are not going anywhere buddy */
				}
				
				EDT_BeginBatchChanges(context);

				if (appearance.changed != 0) {
				    fe_document_appearance_set(context, &appearance);
					appearance.changed = 0;
				}
				if (general.changed != 0) {
				    fe_general_preferences_set(context, &general);
					general.changed = 0;
				}
				if (publish.changed != 0) {
				    fe_publish_preferences_set(context, &publish);
					publish.changed = 0;
				}

				EDT_EndBatchChanges(context);

				/*
				 *    Save options.
				 */
				if (!XFE_SavePrefs((char *)fe_globalData.user_prefs_file,
								   &fe_globalPrefs)) {
					fe_perror(context, XP_GetString(XFE_ERROR_SAVING_OPTIONS));
				} else {
					appearance.changed = 0;
					general.changed = 0;
					publish.changed = 0;
				}
			}

			if (done == XmDIALOG_APPLY_BUTTON) {
				done = XmDIALOG_NONE; /* keep looping */

				XtVaSetValues(apply_button, XmNsensitive, FALSE, 0);
				apply_sensitized = FALSE;
			}
		}
	}
#undef SOMETHING_CHANGED

    /*
     *    Unload data.
     */
	fe_DependentListDestroy(appearance.dependents);

	if (done != XFE_DIALOG_DESTROY_BUTTON)
		XtDestroyWidget(dialog);
}

typedef struct fe_EditorTargetPropertiesStruct
{
	Widget   text;
	Boolean  inserting;
} fe_EditorTargetPropertiesStruct;

static char*
cleanup_selection(char* target, char* source, unsigned max_size)
{
	char* p;
	char* q;
	char* end;

	for (p = source; isspace(*p); p++) /* skip beginning whitespace */
		;

	end = &p[max_size-1];
	q = target;

	while (p < end) {
		/*
		 *    Stop if we detect an unprintable, or newline.
		 */
		if (!isprint(*p) || *p == '\n' || *p == '\r')
			break;

		if (isspace(*p))
			*q++ = ' ', p++;
		else
			*q++ = *p++;
	}
	/* strip trailing whitespace */
	while (q > target && isspace(q[-1]))
		q--;
	 
	*q = '\0';

	return target;
}

static void
fe_target_properties_init(MWContext* context,
						  fe_EditorTargetPropertiesStruct* w_data)
{
	char* value;
	char buf[64];

	w_data->inserting = FALSE;
	
	if (EDT_GetCurrentElementType(context) == ED_ELEMENT_TARGET) {
		value = EDT_GetTargetData(context);
    } else {
		w_data->inserting = TRUE;

		/*
		 *    Use current selected text as suggested target name...
		 */
        if ((value = (char*)LO_GetSelectionText(context))) {
			cleanup_selection(buf, value, sizeof(buf));
			XP_FREE(value);
			value = buf;
		} else {
			value = "";
		}
	}

	/*
	 *    Zap text field.
	 */
	fe_TextFieldSetString(w_data->text, value, FALSE);
}

static char*
cleanup_string(char* target, char* source, unsigned max_size)
{
	char* p;
	char* q;
	char* end;

	if (max_size == 0)
		return NULL;

	for (p = source; isspace(*p); p++) /* skip beginning whitespace */
		;

	end = &p[max_size-1];
	q = target;

	if (strcmp(p,"")==0) return NULL;

	while (p < end) {
		/*
		 *    Stop if we detect an unprintable, or newline.
		 */
		if (*p == '\"')
			p++;
		else
			*q++ = *p++;
	}

	/* strip trailing whitespace */
	while (q > target && isspace(q[-1]))
		q--;
	 
	*q = '\0';

	return target;
}

static void
fe_target_properties_set(MWContext* context,
						 fe_EditorTargetPropertiesStruct* w_data)
{

	char* xm_value;
	char* value;
	char* target_list;
	char buf[512];
	
	xm_value = XmTextFieldGetString(w_data->text);

	target_list = EDT_GetAllDocumentTargets(context);
	/*FIXME*/ /* look at this thing */

	value = cleanup_string(buf, xm_value, sizeof(buf));
	if (value == NULL) {
		XtFree(xm_value);
		return;
	}
		
    EDT_BeginBatchChanges(context);
	if (value[0] == '#')
		value++;
	if (w_data->inserting)
		EDT_InsertTarget(context, value);
	else
		EDT_SetTargetData(context, value);
    EDT_EndBatchChanges(context);
	
	XtFree(xm_value);
}

static Widget
fe_target_properties_create(MWContext* context, Widget form,
								   fe_EditorTargetPropertiesStruct* w_data)
{
	Widget main_rc;
	Widget label;
	Widget text;
	Arg args[8];
	Cardinal n;

	n = 0;
	XtSetArg(args[n], XmNorientation, XmVERTICAL); n++;
	main_rc = XmCreateRowColumn(form, "targetRC", args, n);
	XtManageChild(main_rc);

	n = 0;
	XtSetArg(args[n], XmNalignment, XmALIGNMENT_BEGINNING); n++;
	label = XmCreateLabelGadget(main_rc, "targetLabel", args, n);
	XtManageChild(label);

	n = 0;
	XtSetArg(args[n], XmNalignment, XmALIGNMENT_BEGINNING); n++;
	XtSetArg(args[n], XmNcolumns, 40); n++; /* room to move dude! */
	text = fe_CreateTextField(main_rc, "targetText", args, n);
	XtManageChild(text);

	w_data->text = text;

	return main_rc;
}

void
fe_EditorTargetPropertiesDialogDo(MWContext* context)
{
	Widget dialog;
	Widget form;
	fe_EditorTargetPropertiesStruct data;
	int done;

	/*
	 *    Make prompt with ok, no apply, cancel, separator.
	 */
	form = fe_CreatePromptDialog(context, "targetPropertiesDialog",
								 TRUE, TRUE, FALSE, TRUE, TRUE);
	dialog = XtParent(form);

	fe_target_properties_create(context, form, &data);
	fe_target_properties_init(context, &data);

	/*
	 *   Add a bunch of callbacks to the buttons.
	 */
	XtAddCallback(form, XmNokCallback, fe_hrule_ok_cb, &done);
	XtAddCallback(form, XmNcancelCallback, fe_hrule_cancel_cb, &done);
	XtAddCallback(form, XmNdestroyCallback, fe_hrule_destroy_cb, &done);

    /*
     *    Popup.
     */
    XtManageChild(form);

	/*
     *    Wait.
     */
	fe_NukeBackingStore(dialog); /* what does this do? */

	done = XmDIALOG_NONE;
	while (done == XmDIALOG_NONE)
		fe_EventLoop();
	
	if (done == XmDIALOG_OK_BUTTON)
		fe_target_properties_set(context, &data);

	if (done != XFE_DIALOG_DESTROY_BUTTON)
		XtDestroyWidget(dialog);
}

typedef struct fe_EditorHtmlPropertiesStruct
{
	Widget   text;
	unsigned changed;
	Boolean inserting;
} fe_EditorHtmlPropertiesStruct;

static void
fe_html_properties_init(MWContext* context,
						  fe_EditorHtmlPropertiesStruct* w_data)
{
	char* value;
	char buf[64];

	w_data->inserting = FALSE;
	
	if (EDT_GetCurrentElementType(context) == ED_ELEMENT_UNKNOWN_TAG) {
		value = EDT_GetUnknownTagData(context);
    } else {
		w_data->inserting = TRUE;

		/*
		 *    Use current selected text as suggested target name...
		 */
        if ((value = (char*)LO_GetSelectionText(context))) {
			cleanup_selection(buf, value, sizeof(buf));
			XP_FREE(value);
			value = buf;
		} else {
			value = "";
		}
	}

	/*
	 *    Zap text field.
	 */
	XmTextSetString(w_data->text, value);
	w_data->changed = 0;
}

static void
fe_html_properties_set(MWContext* context,
					   fe_EditorHtmlPropertiesStruct* w_data)
{
	char* xm_value;
	
	xm_value = XmTextGetString(w_data->text);

    EDT_BeginBatchChanges(context);
	if (EDT_GetCurrentElementType(context) == ED_ELEMENT_UNKNOWN_TAG)
		EDT_SetUnknownTagData(context, xm_value);
	else
		EDT_InsertUnknownTag(context, xm_value);
    EDT_EndBatchChanges(context);

	XtFree(xm_value);
}

Boolean
fe_html_properties_verify(MWContext* context,
						 fe_EditorHtmlPropertiesStruct* w_data)
{
	char* xm_value;
	int id;
	ED_TagValidateResult e;
	Widget parent;
	
	xm_value = XmTextGetString(w_data->text);
	
	e = EDT_ValidateTag(xm_value, FALSE );

    switch (e) {
	case ED_TAG_OK:
		break;
	case ED_TAG_UNOPENED:
		id = XFE_EDITOR_TAG_UNOPENED;
		/* Unopened Tag: '<' was expected */
		break;
	case ED_TAG_UNCLOSED:
		id = XFE_EDITOR_TAG_UNCLOSED;
		/* Unopened Tag:  '>' was expected */
		break;
	case ED_TAG_UNTERMINATED_STRING:
		id = XFE_EDITOR_TAG_UNTERMINATED_STRING;
		/* Unterminated String in tag: closing quote expected */
		break;
	case ED_TAG_PREMATURE_CLOSE:
		id = XFE_EDITOR_TAG_PREMATURE_CLOSE;
		/* Premature close of tag */
		break;
	case ED_TAG_TAGNAME_EXPECTED:
		id = XFE_EDITOR_TAG_TAGNAME_EXPECTED;
		/* Tagname was expected */
		break;
	default:
		id = XFE_EDITOR_TAG_UNKNOWN;
		/* Unknown tag error */
		break;
    }

	XtFree(xm_value);

	parent = w_data->text;
		
	if (e == ED_TAG_OK) {
		return TRUE;
	} else {
		fe_error_dialog(context, w_data->text, XP_GetString(id));
		return FALSE;
	}
}

static void
fe_html_text_changed_cb(Widget widget, XtPointer closure, XtPointer cb)
{
	 fe_EditorHtmlPropertiesStruct* w_data =
		 (fe_EditorHtmlPropertiesStruct*)closure;
	 w_data->changed = 0x1;
}

static Widget
fe_html_properties_create(MWContext* context, Widget form,
								   fe_EditorHtmlPropertiesStruct* w_data)
{
	Widget main_rc;
	Widget label;
	Widget text;
	Arg args[8];
	Cardinal n;

	n = 0;
	XtSetArg(args[n], XmNorientation, XmVERTICAL); n++;
	main_rc = XmCreateRowColumn(form, "htmlRC", args, n);
	XtManageChild(main_rc);

	n = 0;
	XtSetArg(args[n], XmNalignment, XmALIGNMENT_BEGINNING); n++;
	label = XmCreateLabelGadget(main_rc, "htmlPropertiesInfo", args, n);
	XtManageChild(label);

	n = 0;
	XtSetArg(args[n], XmNeditMode, XmMULTI_LINE_EDIT); n++;
	XtSetArg(args[n], XmNcolumns, 70); n++;
	XtSetArg(args[n], XmNrows, 8); n++;
	text = XmCreateScrolledText(main_rc, "htmlText", args, n);
	fe_HackTextTranslations(text);
	XtAddCallback(text, XmNvalueChangedCallback, fe_html_text_changed_cb,
				  (XtPointer)w_data);
	XtManageChild(text);

	w_data->text = text;

	return main_rc;
}

void
fe_EditorHtmlPropertiesDialogDo(MWContext* context)
{
	Widget dialog;
	Widget form;
	Widget ok_button;
	fe_EditorHtmlPropertiesStruct data;
	int    done;

	/*
	 *    Make prompt with ok, no apply, cancel, separator.
	 */
	form = fe_CreatePromptDialog(context, "htmlPropertiesDialog",
								 TRUE, TRUE, TRUE, TRUE, TRUE);
	dialog = XtParent(form);

	fe_html_properties_create(context, form, &data);
	fe_html_properties_init(context, &data);

	/*
	 *   Add a bunch of callbacks to the buttons.
	 */
	XtAddCallback(form, XmNokCallback, fe_hrule_ok_cb, &done);
	XtAddCallback(form, XmNapplyCallback, fe_hrule_apply_cb, &done);
	XtAddCallback(form, XmNcancelCallback, fe_hrule_cancel_cb, &done);
	XtAddCallback(form, XmNdestroyCallback, fe_hrule_destroy_cb, &done);

	ok_button = XmSelectionBoxGetChild(form, XmDIALOG_OK_BUTTON);

    /*
     *    Popup.
     */
    XtManageChild(form);

	/*
     *    Wait.
     */
	fe_NukeBackingStore(dialog); /* what does this do? */

	done = XmDIALOG_NONE;
	while (done == XmDIALOG_NONE) {
		fe_EventLoop();

		if (done == XmDIALOG_APPLY_BUTTON || done == XmDIALOG_OK_BUTTON) {
			if (fe_html_properties_verify(context, &data) == FALSE) {
				done = XmDIALOG_NONE;
			} else if (done == XmDIALOG_APPLY_BUTTON) { /* but verified */
				/* Tag seems ok */
				fe_message_dialog(context,
								  dialog,
								  XP_GetString(XFE_EDITOR_TAG_OK));
				done = XmDIALOG_NONE;
			}
		}
	}
	
	if (done == XmDIALOG_OK_BUTTON && data.changed != 0)
		fe_html_properties_set(context, &data);

	if (done != XFE_DIALOG_DESTROY_BUTTON)
		XtDestroyWidget(dialog);
}

char* fe_SimpleTableAlignment[3] = {
	"left",
	"center",
	"right"
};

char* fe_SimpleOptionAboveBelow[2] = {
	"above",
	"below"
};

char* fe_SimpleOptionPixelPercent[2] = {
	"pixels",
	"percent"
};

char* fe_SimpleOptionHorizontalAlignment[4] = {
	"default",
	"left",
	"center",
	"right"
};

char* fe_SimpleOptionVerticalAlignment[5] = {
	"default",
	"top",
	"center",
	"bottom",
	"baselines"
};

Widget
fe_CreateSimpleOptionMenu(Widget parent, char* name, Arg* p_args, Cardinal p_n)
{
	Widget pulldown;
	Widget button;
	Widget option_menu;
	Widget history_widget;
	char namebuf[64];
	Arg args[32];
	Cardinal n;
	Cardinal i;
	char** button_names = NULL;
	unsigned button_count = 0;
	unsigned button_set = 0;
	char* button_name;

	strcpy(namebuf, name);
	strcat(namebuf, "Menu");

	n = 0;
	pulldown = fe_CreatePulldownMenu(parent, namebuf, args, n);

	n = 0;
	for (i = 0; i < p_n; i++) {
		if (p_args[i].name == XmNbuttons)
			button_names = (char**)p_args[i].value;
		else if (p_args[i].name == XmNbuttonCount)
			button_count = (unsigned)p_args[i].value;
		else if (p_args[i].name == XmNbuttonSet)
			button_set = (unsigned)p_args[i].value;
		else
			args[n++] = p_args[i];
	}

	if (button_names == fe_SimpleOptionAboveBelow)
		button_count = XtNumber(fe_SimpleOptionAboveBelow);
	else if (button_names == fe_SimpleOptionPixelPercent)
		button_count = XtNumber(fe_SimpleOptionPixelPercent);
	else if (button_names == fe_SimpleOptionHorizontalAlignment)
		button_count = XtNumber(fe_SimpleOptionHorizontalAlignment);
	else if (button_names == fe_SimpleOptionVerticalAlignment)
		button_count = XtNumber(fe_SimpleOptionVerticalAlignment);

	for (i = 0; i < button_count; i++) {
		if (button_names) {
			button_name = button_names[i];
		} else {
			sprintf(namebuf, "button%d", i);
			button_name = namebuf;
		}
		button = XmCreatePushButtonGadget(pulldown, button_name, NULL, 0);
		XtManageChild(button);
		if (i == button_set)
			history_widget = button;
	}

	XtSetArg(args[n], XmNsubMenuId, pulldown); n++;
	option_menu = fe_CreateOptionMenu(parent, name, args, n);
	fe_UnmanageChild_safe(XmOptionLabelGadget(option_menu));

	if (history_widget)
		XtVaSetValues(option_menu, XmNmenuHistory, history_widget, 0);

	return option_menu;
}

static Widget
fe_CreateSimpleRadioGroup(Widget parent, char* name, Arg* p_args, Cardinal p_n)
{
	Widget button;
	Widget option_menu;
	char namebuf[64];
	Arg args[32];
	Cardinal n;
	Cardinal i;
	char** button_names = NULL;
	unsigned button_count = 0;
	unsigned button_set = 0;
	char* button_name;

	strcpy(namebuf, name);
	strcat(namebuf, "Radio");

	n = 0;
	for (i = 0; i < p_n; i++) {
		if (p_args[i].name == XmNbuttons)
			button_names = (char**)p_args[i].value;
		else if (p_args[i].name == XmNbuttonCount)
			button_count = (unsigned)p_args[i].value;
		else if (p_args[i].name == XmNbuttonSet)
			button_set = (unsigned)p_args[i].value;
		else
			args[n++] = p_args[i];
	}

	XtSetArg(args[n], XmNradioBehavior, TRUE); n++;
	option_menu = XmCreateRowColumn(parent, name, args, n);

	if (button_names == fe_SimpleOptionAboveBelow)
		button_count = XtNumber(fe_SimpleOptionAboveBelow);
	else if (button_names == fe_SimpleOptionPixelPercent)
		button_count = XtNumber(fe_SimpleOptionPixelPercent);
	else if (button_names == fe_SimpleOptionHorizontalAlignment)
		button_count = XtNumber(fe_SimpleOptionHorizontalAlignment);
	else if (button_names == fe_SimpleOptionVerticalAlignment)
		button_count = XtNumber(fe_SimpleOptionVerticalAlignment);
	else if (button_names == fe_SimpleTableAlignment)
		button_count = XtNumber(fe_SimpleTableAlignment);

	for (i = 0; i < button_count; i++) {
		if (button_names) {
			button_name = button_names[i];
		} else {
			sprintf(namebuf, "button%d", i);
			button_name = namebuf;
		}
		n = 0;
		XtSetArg(args[n], XmNset, (i == button_set)); n++;
		XtSetArg(args[n], XmNindicatorType, XmONE_OF_MANY); n++;
		button = XmCreateToggleButtonGadget(option_menu, button_name, args, n);
		XtManageChild(button);
	}

	return option_menu;
}

void
fe_SimpleRadioGroupSetWhich(Widget widget, unsigned which)
{
	Widget* children;
	Cardinal num_children;
	Cardinal i;

	XtVaGetValues(widget,
				  XmNchildren, &children,
				  XmNnumChildren, &num_children, 0);

	if (which < num_children) {

		for (i = 0; i < num_children; i++) {
			XmToggleButtonGadgetSetState(children[i], (i == which), FALSE);
		}
	}
}

void
fe_SimpleRadioGroupSetSensitive(Widget widget, Boolean sensitive)
{
	Widget* children;
	Cardinal num_children;
	Cardinal i;

	XtVaGetValues(widget,
				  XmNchildren, &children,
				  XmNnumChildren, &num_children, 0);

	for (i = 0; i < num_children; i++) {
		XtVaSetValues(children[i], XmNsensitive, sensitive, 0);
	}
}

int
fe_SimpleRadioGroupGetWhich(Widget widget)
{
	Widget* children;
	Cardinal num_children;
	Cardinal i;

	XtVaGetValues(widget,
				  XmNchildren, &children,
				  XmNnumChildren, &num_children, 0);

	for (i = 0; i < num_children; i++) {
		if (XmToggleButtonGadgetGetState(children[i]) == TRUE)
			return i;
	}
	return -1; /* ?? */
}

Widget
fe_SimpleRadioGroupGetChild(Widget widget, unsigned n)
{
	Widget*  children;
	Cardinal num_children;

	XtVaGetValues(widget,
				  XmNchildren, &children,
				  XmNnumChildren, &num_children, 0);

	if (n < num_children)
		return children[n];
	else
		return NULL;
}


static unsigned
fe_ED_Alignment_to_index(ED_Alignment type)
{
    switch (type) {
	case ED_ALIGN_LEFT:
    case ED_ALIGN_ABSTOP:
	case ED_ALIGN_TOP:       return 1;
	case ED_ALIGN_CENTER:
	case ED_ALIGN_ABSCENTER: return 2;
	case ED_ALIGN_RIGHT:
	case ED_ALIGN_BOTTOM:
    case ED_ALIGN_ABSBOTTOM: return 3;
    case ED_ALIGN_BASELINE:  return 4;
	case ED_ALIGN_DEFAULT:
	default:                 return 0;
  }
}

typedef struct fe_EditorTablesTableStruct
{
	Widget number_rows_text;
	Widget number_columns_text;
	Widget line_width_text;
	Widget spacing_text;
	Widget padding_text;
	Widget width_toggle;
	Widget width_text;
	Widget width_units;
	Widget height_toggle;
	Widget height_text;
	Widget height_units;
	Widget color_toggle;
	Widget color_swatch;
	Widget choose_color;
	LO_Color color_value;
	Widget caption_toggle;
	Widget caption_type;
	Widget alignBox;
    Boolean inserting;
} fe_EditorTablesTableStruct; 

#if 0
static void
check_children(Widget* children, Cardinal nchildren)
{
	Widget widget;
	Widget parent;
	int i;

	for (i = 0; i < nchildren; i++) {
		widget = children[i];
		parent = XtParent(widget);
		fprintf(real_stderr, "parent(%s) = %s, ", XtName(widget), 
				XtName(parent));
	}
	fprintf(real_stderr, "\n");
	
}
#endif

static void
fe_ya_picker_cb(Widget widget, XtPointer closure, XtPointer cb)
{
	LO_Color* color = (LO_Color*)closure;
	MWContext* context = fe_WidgetToMWContext(widget);

	if (fe_ColorPicker(context, color)) {
		fe_SwatchSetColor(widget, color);
	}
}

static void
fe_table_choose_color_cb(Widget widget, XtPointer closure, XtPointer cb)
{
	MWContext* context = fe_WidgetToMWContext(widget);
	fe_EditorTablesTableStruct* w_data = (fe_EditorTablesTableStruct*)closure;

	if (fe_ColorPicker(context, &w_data->color_value)) {
		fe_SwatchSetColor(w_data->color_swatch, &w_data->color_value);
	}
}

static void
fe_table_tbr_set(MWContext* context, Widget toggle, Widget text, Widget radio,
				 Boolean enabled, unsigned numeric, Boolean second_one)
{
	XmToggleButtonGadgetSetState(toggle, enabled, FALSE);
	if (text != NULL) {
	    char buf[32];

		sprintf(buf, "%d", numeric);

		fe_TextFieldSetString(text, buf, FALSE);
		fe_TextFieldSetEditable(context, text, enabled);
	}
	fe_SimpleRadioGroupSetWhich(radio, second_one);
	fe_SimpleRadioGroupSetSensitive(radio, enabled);
}

static void
fe_table_percent_label_set(Widget widget, Boolean nested)
{
	char*    string;
	char*    name;
	XmString xm_string;
    XtResource resource;

	if (nested)
		name = "percentOfCell";
	else
		name = "percentOfWindow";

	resource.resource_name = XmNlabelString;
    resource.resource_class = XmCXmString;
    resource.resource_type = XmRString;
    resource.resource_size = sizeof(String);
    resource.resource_offset = 0;
    resource.default_type = XtRImmediate;
    resource.default_addr = 0;

    XtGetSubresources(widget, (XtPointer)&string, name,
					  XtClassName(widget), &resource, 1, NULL, 0);
    
	if (!string)
		string = name;
	
	xm_string = XmStringCreateLocalized(string);

	XtVaSetValues(widget, XmNlabelString, xm_string, 0);
	
	XmStringFree(xm_string);
}

static void
fe_editor_set_swatch_stuff(Widget widget, LO_Color* color, Boolean has_color)
{
	MWContext* context;
	LO_Color default_color;

	if (has_color) {
		fe_SwatchSetColor(widget, color);
	} else {
		context = fe_WidgetToMWContext(widget);
		fe_EditorDocumentGetColors(context,
								   NULL,
								   &default_color,
								   NULL,
								   NULL,
								   NULL,
								   NULL);
		fe_SwatchSetColor(widget, &default_color);
	}
	fe_SwatchSetSensitive(widget, has_color);
}


static void
fe_table_toggle_cb(Widget widget, XtPointer closure, XtPointer cb_data)
{
	fe_EditorTablesTableStruct* w_data = (fe_EditorTablesTableStruct*)closure;
	Boolean enabled = XmToggleButtonGetState(widget);
	MWContext* context = fe_WidgetToMWContext(widget);
	Widget  text;
	Widget  radio;

	if (widget == w_data->width_toggle) {
		text = w_data->width_text;
		radio = w_data->width_units;
	} else if (widget == w_data->height_toggle) {
		text = w_data->height_text;
		radio = w_data->height_units;
	} else if (widget == w_data->color_toggle) {
		text = w_data->width_text;
		radio = w_data->width_units;
		fe_editor_set_swatch_stuff(w_data->color_swatch, &w_data->color_value, enabled);
		fe_WidgetSetSensitive(w_data->choose_color, enabled);
		return;
	} else if (widget == w_data->caption_toggle) {
		fe_SimpleRadioGroupSetSensitive(w_data->caption_type, enabled);
		return;
	}

	fe_TextFieldSetEditable(context, text, enabled);
	fe_SimpleRadioGroupSetSensitive(radio, enabled);
}

static Widget
fe_tables_table_properties_create(MWContext* context, Widget parent,
								  fe_EditorTablesTableStruct* w_data)
{
	Widget frame;
	Widget form;
	Widget line_width_label;
	Widget line_width_text;
	Widget line_width_units;
	Widget spacing_label;
	Widget spacing_text;
	Widget spacing_units;
	Widget padding_label;
	Widget padding_text;
	Widget padding_units;
	Widget width_toggle;
	Widget width_text;
	Widget width_units;
	Widget height_toggle;
	Widget height_text;
	Widget height_units;
	Widget color_toggle;
	Widget color_swatch;
	Widget choose_color;
	Widget caption_toggle;
	Widget caption_type;
	Widget alignframe;
	Widget alignBox;
	Arg args[16];
	Cardinal n;
	Cardinal nchildren;
	Widget children[20];
	Cardinal i;
	Dimension left_offset;
	Dimension height;

	n = 0;
	XtSetArg(args[n], XmNtopAttachment, XmATTACH_FORM); n++;
	XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM); n++;
	frame = fe_CreateFrame(parent, "attributes", args, n);
    XtManageChild(frame);

	n = 0;
	form = XmCreateForm(frame, "attributes", args, n);
	XtManageChild(form);

	nchildren = 0;
	n = 0;
	line_width_label = XmCreateLabelGadget(form, "borderLineWidthLabel",
										   args, n);
	children[nchildren++] = line_width_label;

	n = 0;
	spacing_label = XmCreateLabelGadget(form, "cellSpacingLabel", args, n);
	children[nchildren++] = spacing_label;

	n = 0;
	padding_label = XmCreateLabelGadget(form, "cellPaddingLabel", args, n);
	children[nchildren++] = padding_label;

	n = 0;
	width_toggle = XmCreateToggleButtonGadget(form, "tableWidthToggle",
											  args, n);
	children[nchildren++] = width_toggle;
	XtAddCallback(width_toggle, XmNvalueChangedCallback, 
				  fe_table_toggle_cb, (XtPointer)w_data);

	n = 0;
	height_toggle = XmCreateToggleButtonGadget(form, "tableHeightToggle",
											   args, n);
	children[nchildren++] = height_toggle;
	XtAddCallback(height_toggle, XmNvalueChangedCallback, 
				  fe_table_toggle_cb, (XtPointer)w_data);

	n = 0;
	color_toggle = XmCreateToggleButtonGadget(form, "tableColorToggle",
											  args, n);
	children[nchildren++] = color_toggle;
	XtAddCallback(color_toggle, XmNvalueChangedCallback, 
				  fe_table_toggle_cb, (XtPointer)w_data);

	n = 0;
	caption_toggle = XmCreateToggleButtonGadget(form, "captionToggle",
												args, n);
	children[nchildren++] = caption_toggle;
	XtAddCallback(caption_toggle, XmNvalueChangedCallback, 
				  fe_table_toggle_cb, (XtPointer)w_data);

	left_offset = fe_get_offset_from_widest(children, nchildren);

	/*
	 *    Make all the text dudes + swatch + caption units
	 */
	n = 0;
	XtSetArg(args[n], XmNtopAttachment, XmATTACH_FORM); n++;
	XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM); n++;
	XtSetArg(args[n], XmNleftOffset, left_offset); n++;
	XtSetArg(args[n], XmNcolumns, 4); n++;
	line_width_text = fe_CreateTextField(form, "borderLineWidthText",
										  args, n);
	children[nchildren++] = line_width_text;
	
	n = 0;
	spacing_text = fe_CreateTextField(form, "cellSpacingText", args, n);
	children[nchildren++] = spacing_text;
	
	n = 0;
	padding_text = fe_CreateTextField(form, "cellPaddingText", args, n);
	children[nchildren++] = padding_text;
	
	n = 0;
	width_text = fe_CreateTextField(form, "tableWidthText", args, n);
	children[nchildren++] = width_text;
	
	n = 0;
	height_text = fe_CreateTextField(form, "tableHeightText", args, n);
	children[nchildren++] = height_text;

	XtVaGetValues(line_width_text, XmNheight, &height, 0);
	
	n = 0;
	XtSetArg(args[n], XmNheight, height); n++;
	color_swatch = fe_CreateSwatch(form, "colorSwatch", args, n);
	children[nchildren++] = color_swatch;

	XtAddCallback(color_swatch, XmNactivateCallback, 
				  fe_ya_picker_cb, (XtPointer)&w_data->color_value);

	n = 0;
	XtSetArg(args[n], XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET); n++;
	XtSetArg(args[n], XmNtopWidget, caption_toggle); n++;
	XtSetArg(args[n], XmNbottomAttachment, XmATTACH_FORM); n++;
	XtSetArg(args[n], XmNleftAttachment, XmATTACH_OPPOSITE_WIDGET); n++;
	XtSetArg(args[n], XmNleftWidget, line_width_text); n++;
	XtSetArg(args[n], XmNbuttons, fe_SimpleOptionAboveBelow); n++;
#ifdef TABLES_USE_OPTION_MENU	
	caption_type = fe_CreateSimpleOptionMenu(form, "captionType", args, n);
#else
	XtSetArg(args[n], XmNpacking, XmPACK_TIGHT); n++;
	XtSetArg(args[n], XmNorientation, XmHORIZONTAL); n++;
	caption_type = fe_CreateSimpleRadioGroup(form, "captionType", args, n);
#endif
	children[nchildren++] = caption_type;

	/*
	 *    Go back and do first column attachments.
	 */
	for (i = 0; i < 7; i++) {
		n = 0;
		if (i == 0) {
			XtSetArg(args[n], XmNtopAttachment, XmATTACH_FORM); n++;	
		} else {
			XtSetArg(args[n], XmNtopAttachment, XmATTACH_WIDGET); n++;
			XtSetArg(args[n], XmNtopWidget, children[i+6]); n++;
		}
		XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM); n++;
		XtSetValues(children[i], args, n);
	}

	for (i = 7 + 1; i < 7 + 1 + 5; i++) {
		n = 0;
		XtSetArg(args[n], XmNtopAttachment, XmATTACH_WIDGET); n++;
		XtSetArg(args[n], XmNtopWidget, children[i-1]); n++;
		XtSetArg(args[n], XmNleftAttachment, XmATTACH_OPPOSITE_WIDGET); n++;
		XtSetArg(args[n], XmNleftWidget, line_width_text); n++;

		XtSetArg(args[n], XmNrightAttachment, XmATTACH_OPPOSITE_WIDGET); n++;
		XtSetArg(args[n], XmNrightWidget, line_width_text); n++;
		XtSetValues(children[i], args, n);
	}

	n = 0;
	line_width_units = XmCreateLabelGadget(form, "borderLineWidthUnits",
										   args, n);
	children[nchildren++] = line_width_units;

	n = 0;
	spacing_units = XmCreateLabelGadget(form, "cellSpacingUnits", args, n);
	children[nchildren++] = spacing_units;

	n = 0;
	padding_units = XmCreateLabelGadget(form, "cellPaddingUnits", args, n);
	children[nchildren++] = padding_units;

	n = 0;
	XtSetArg(args[n], XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET); n++;
	XtSetArg(args[n], XmNtopWidget, width_text); n++;
	XtSetArg(args[n], XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET); n++;
	XtSetArg(args[n], XmNbottomWidget, width_text); n++;
	XtSetArg(args[n], XmNleftAttachment, XmATTACH_WIDGET); n++;
	XtSetArg(args[n], XmNleftWidget, line_width_text); n++;
	XtSetArg(args[n], XmNbuttons, fe_SimpleOptionPixelPercent); n++;
#ifdef TABLES_USE_OPTION_MENU	
	width_units = fe_CreateSimpleOptionMenu(form, "tableWidthUnits", args, n);
#else
	XtSetArg(args[n], XmNpacking, XmPACK_TIGHT); n++;
	XtSetArg(args[n], XmNorientation, XmHORIZONTAL); n++;
	width_units = fe_CreateSimpleRadioGroup(form, "tableWidthUnits", args, n);
#endif
	children[nchildren++] = width_units;

	n = 0;
	XtSetArg(args[n], XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET); n++;
	XtSetArg(args[n], XmNtopWidget, height_text); n++;
	XtSetArg(args[n], XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET); n++;
	XtSetArg(args[n], XmNbottomWidget, height_text); n++;
	XtSetArg(args[n], XmNleftAttachment, XmATTACH_WIDGET); n++;
	XtSetArg(args[n], XmNleftWidget, line_width_text); n++;
	XtSetArg(args[n], XmNbuttons, fe_SimpleOptionPixelPercent); n++;
#ifdef TABLES_USE_OPTION_MENU	
	height_units = fe_CreateSimpleOptionMenu(form, "tableHeightUnits",args, n);
#else
	XtSetArg(args[n], XmNpacking, XmPACK_TIGHT); n++;
	XtSetArg(args[n], XmNorientation, XmHORIZONTAL); n++;
	height_units = fe_CreateSimpleRadioGroup(form, "tableHeightUnits",args, n);
#endif
	children[nchildren++] = height_units;

	n = 0;
	XtSetArg(args[n], XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET); n++;
	XtSetArg(args[n], XmNtopWidget, color_swatch); n++;
	XtSetArg(args[n], XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET); n++;
	XtSetArg(args[n], XmNbottomWidget, color_swatch); n++;
	XtSetArg(args[n], XmNleftAttachment, XmATTACH_WIDGET); n++;
	XtSetArg(args[n], XmNleftWidget, line_width_text); n++;
	choose_color = XmCreatePushButtonGadget(form, "chooseColor",args, n);
	children[nchildren++] = choose_color;

	XtAddCallback(choose_color, XmNactivateCallback, 
				  fe_table_choose_color_cb, (XtPointer)w_data);

	for (i = 14; i < 14 + 3; i++) {
		n = 0;
		if (i == 14) {
			XtSetArg(args[n], XmNtopAttachment, XmATTACH_FORM); n++;
		} else {
			XtSetArg(args[n], XmNtopAttachment, XmATTACH_WIDGET); n++;
			XtSetArg(args[n], XmNtopWidget, children[i-8]); n++;
		}
		XtSetArg(args[n], XmNleftAttachment, XmATTACH_WIDGET); n++;
		XtSetArg(args[n], XmNleftWidget, line_width_text); n++;
		XtSetValues(children[i], args, n);
	}

#if 0
	check_children(children, nchildren);
#endif
	XtManageChildren(children, nchildren);

	n = 0;
	XtSetArg(args[n], XmNtopAttachment, XmATTACH_WIDGET); n++;
	XtSetArg(args[n], XmNtopWidget, frame); n++;
	XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM); n++;
	XtSetArg(args[n], XmNrightAttachment, XmATTACH_FORM); n++;
	alignframe = fe_CreateFrame(parent, "tableAlignment", args, n);
    XtManageChild(alignframe);

	n = 0;
	XtSetArg(args[n], XmNbuttons, fe_SimpleTableAlignment); n++;
	XtSetArg(args[n], XmNorientation, XmHORIZONTAL); n++;
	alignBox = fe_CreateSimpleRadioGroup(alignframe, "tableAlignmentBox", 
										 args, n);
	XtManageChild(alignBox);
	fe_SimpleRadioGroupSetWhich(alignBox, 0);

	w_data->line_width_text = line_width_text;
	w_data->spacing_text = spacing_text;
	w_data->padding_text = padding_text;
	w_data->width_toggle = width_toggle;
	w_data->width_text = width_text;
	w_data->width_units = width_units;
	w_data->height_toggle = height_toggle;
	w_data->height_text = height_text;
	w_data->height_units = height_units;
	w_data->color_toggle = color_toggle;
	w_data->caption_toggle = caption_toggle;
	w_data->caption_type = caption_type;
	w_data->color_swatch = color_swatch;
	w_data->choose_color = choose_color;
	w_data->alignBox = alignBox;

	return frame;
}

static Widget
fe_tables_table_create_create(MWContext* context, Widget parent,
							  fe_EditorTablesTableStruct* w_data)
{
	Widget form;
	Widget rows_label;
	Widget rows_text;
	Widget columns_label;
	Widget columns_text;
	Widget attributes;
	Arg args[8];
	Cardinal n;
	Cardinal nchildren = 0;
	Widget children[6];

	n = 0;
	form = XmCreateForm(parent, "tablesCreate", args, n);
	XtManageChild(form);

	n = 0;
	XtSetArg(args[n], XmNtopAttachment, XmATTACH_FORM); n++;
	XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM); n++;
	rows_label = XmCreateLabelGadget(form, "tableRowsLabel", args, n);
	children[nchildren++] = rows_label;

	n = 0;
	XtSetArg(args[n], XmNtopAttachment, XmATTACH_FORM); n++;
	XtSetArg(args[n], XmNleftAttachment, XmATTACH_WIDGET); n++;
	XtSetArg(args[n], XmNleftWidget, children[nchildren-1]); n++;
	XtSetArg(args[n], XmNcolumns, 4); n++;
	rows_text = fe_CreateTextField(form, "tableRowsText", args, n);
	children[nchildren++] = rows_text;

	n = 0;
	XtSetArg(args[n], XmNtopAttachment, XmATTACH_FORM); n++;
	XtSetArg(args[n], XmNleftAttachment, XmATTACH_WIDGET); n++;
	XtSetArg(args[n], XmNleftWidget, children[nchildren-1]); n++;
	columns_label = XmCreateLabelGadget(form, "tableColumnsLabel", args, n);
	children[nchildren++] = columns_label;

	n = 0;
	XtSetArg(args[n], XmNtopAttachment, XmATTACH_FORM); n++;
	XtSetArg(args[n], XmNleftAttachment, XmATTACH_WIDGET); n++;
	XtSetArg(args[n], XmNleftWidget, children[nchildren-1]); n++;
	XtSetArg(args[n], XmNcolumns, 4); n++;
	columns_text = fe_CreateTextField(form, "tableColumnsText", args, n);
	children[nchildren++] = columns_text;

	attributes = fe_tables_table_properties_create(context, form, w_data);
	children[nchildren++] = attributes;

	n = 0;
	XtSetArg(args[n], XmNtopAttachment, XmATTACH_WIDGET); n++;
	XtSetArg(args[n], XmNtopWidget, rows_text); n++;
	XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM); n++;
	XtSetValues(attributes, args, n);

#if 0
	check_children(children, nchildren);
#endif
	XtManageChildren(children, nchildren);

	w_data->number_rows_text = rows_text;
	w_data->number_columns_text = columns_text;

	return form;
}

static void
fe_table_properties_common_init(MWContext* context,
								EDT_AllTableData* table_data,
								fe_EditorTablesTableStruct* w_data)
{
	Boolean has_color;
	Boolean is_nested;
	Widget  widget;

	if (w_data->number_rows_text != NULL) /* inserting */
		is_nested = EDT_IsInsertPointInTable(context);
	else
		is_nested = EDT_IsInsertPointInNestedTable(context);

	/*
	 *    This should be done with a converter!
	 */
	fe_set_numeric_text_field(w_data->line_width_text,
							  table_data->td.iBorderWidth);
	fe_set_numeric_text_field(w_data->spacing_text,
							  table_data->td.iCellSpacing);
	fe_set_numeric_text_field(w_data->padding_text,
							  table_data->td.iCellPadding);

	fe_table_tbr_set(context, w_data->width_toggle,
					 w_data->width_text, w_data->width_units,
					 table_data->td.bWidthDefined,
					 table_data->td.iWidth,
					 (table_data->td.bWidthPercent));

	widget = fe_SimpleRadioGroupGetChild(w_data->width_units, 1);
	fe_table_percent_label_set(widget, is_nested);
	
	fe_table_tbr_set(context, w_data->height_toggle,
					 w_data->height_text, w_data->height_units,
					 table_data->td.bHeightDefined,
					 table_data->td.iHeight,
					 (table_data->td.bHeightPercent));
	widget = fe_SimpleRadioGroupGetChild(w_data->height_units, 1);
	fe_table_percent_label_set(widget, is_nested);
	
	XmToggleButtonGadgetSetState(w_data->color_toggle,
								 (table_data->td.pColorBackground != NULL),
								 FALSE);

	has_color= (table_data->td.pColorBackground != NULL);
	if (has_color) {
		w_data->color_value = *table_data->td.pColorBackground;
    } else {
		fe_EditorDocumentGetColors(context,
								   NULL,
								   &w_data->color_value,
								   NULL,
								   NULL,
								   NULL,
								   NULL);
	}

	fe_SwatchSetColor(w_data->color_swatch, &w_data->color_value);
	fe_SwatchSetSensitive(w_data->color_swatch, has_color);
	fe_WidgetSetSensitive(w_data->choose_color, has_color);

	fe_table_tbr_set(context, w_data->caption_toggle,
					 NULL, w_data->caption_type,
					 table_data->has_caption,
					 0,
					 (table_data->cd.align != ED_ALIGN_ABSTOP));

	/* we don't have default for alignment yet, so we need to subtract one
	 * from index 
  	 */
	fe_SimpleRadioGroupSetWhich(w_data->alignBox,
								fe_ED_Alignment_to_index(table_data->td.align)-1);
}


static void
fe_tables_table_properties_init(MWContext* context,
								fe_EditorTablesTableStruct* w_data)
{
	EDT_AllTableData table_data;

	if (!fe_EditorTableGetData(context, &table_data))
		return;

	w_data->number_rows_text = NULL;
	w_data->number_columns_text = NULL;
	fe_table_properties_common_init(context, &table_data, w_data);
}

static void
fe_table_properties_common_set(MWContext* context,
							   EDT_AllTableData* table_data,
							   fe_EditorTablesTableStruct* w_data)
{
	Boolean use_color;
	int alignment;

	table_data->td.iBorderWidth = 
		fe_get_numeric_text_field(w_data->line_width_text);
	table_data->td.iCellSpacing = 
		fe_get_numeric_text_field(w_data->spacing_text);
	table_data->td.iCellPadding = 
		fe_get_numeric_text_field(w_data->padding_text);

	table_data->td.bWidthDefined =
		XmToggleButtonGetState(w_data->width_toggle);
	table_data->td.iWidth =
		fe_get_numeric_text_field(w_data->width_text);
	table_data->td.bWidthPercent =
		(fe_SimpleRadioGroupGetWhich(w_data->width_units) == 1);

	table_data->td.bHeightDefined =
		XmToggleButtonGetState(w_data->height_toggle);
	table_data->td.iHeight =
		fe_get_numeric_text_field(w_data->height_text);
	table_data->td.bHeightPercent =
		(fe_SimpleRadioGroupGetWhich(w_data->height_units) == 1);

	alignment = fe_SimpleRadioGroupGetWhich(w_data->alignBox);

	switch(alignment) {
		case 0:
			table_data->td.align = ED_ALIGN_LEFT;
			break;
		case 1:
			table_data->td.align = ED_ALIGN_ABSCENTER;
			break;
		case 2:
			table_data->td.align = ED_ALIGN_RIGHT;
			break;
		default:
			XP_ASSERT(0);
			break;
	}

	/* COLOR */
	use_color = XmToggleButtonGetState(w_data->color_toggle);
	if (use_color)
		table_data->td.pColorBackground = &w_data->color_value;
	else
		table_data->td.pColorBackground = NULL;

	table_data->has_caption = XmToggleButtonGetState(w_data->caption_toggle);

	if (fe_SimpleRadioGroupGetWhich(w_data->caption_type) == 1)
		table_data->cd.align = ED_ALIGN_ABSBOTTOM;
	else
		table_data->cd.align = ED_ALIGN_ABSTOP;
}

static Boolean
fe_tables_table_properties_validate(MWContext* context,
									fe_EditorTablesTableStruct* w_data)
{
	EDT_AllTableData table_data;
	EDT_TableData*   t = &table_data.td;
	unsigned         errors[16];
	unsigned         nerrors = 0;
	
	fe_table_properties_common_set(context, &table_data, w_data);

	if (w_data->number_rows_text != NULL) {
		t->iRows = fe_get_numeric_text_field(w_data->number_rows_text);
		t->iColumns = 
			fe_get_numeric_text_field(w_data->number_columns_text);
	 
		if (RANGE_CHECK(t->iRows, TABLE_MIN_ROWS, TABLE_MAX_ROWS))
			errors[nerrors++] = XFE_INVALID_TABLE_NROWS;
		if (RANGE_CHECK(t->iColumns, TABLE_MIN_COLUMNS, TABLE_MAX_COLUMNS))
			errors[nerrors++] = XFE_INVALID_TABLE_NCOLUMNS;
	}

	if (RANGE_CHECK(t->iBorderWidth, TABLE_MIN_BORDER, TABLE_MAX_BORDER))
		errors[nerrors++] = XFE_INVALID_TABLE_BORDER;
		
	if (RANGE_CHECK(t->iCellSpacing, TABLE_MIN_SPACING, TABLE_MAX_SPACING))
		errors[nerrors++] = XFE_INVALID_TABLE_SPACING;
		
	if (RANGE_CHECK(t->iCellPadding, TABLE_MIN_PADDING, TABLE_MAX_PADDING))
		errors[nerrors++] = XFE_INVALID_TABLE_PADDING;

	if (t->bWidthDefined) {
		if (t->bWidthPercent) {
			if (RANGE_CHECK(t->iWidth,
						TABLE_MIN_PERCENT_WIDTH, TABLE_MAX_PERCENT_WIDTH))
				errors[nerrors++] = XFE_INVALID_TABLE_WIDTH;
		} else {
			if (RANGE_CHECK(t->iWidth,
							TABLE_MIN_PIXEL_WIDTH, TABLE_MAX_PIXEL_WIDTH))
				errors[nerrors++] = XFE_INVALID_TABLE_WIDTH;
		}
	}
		
	if (t->bHeightDefined) {
		if (t->bHeightPercent) {
			if (RANGE_CHECK(t->iHeight,
						TABLE_MIN_PERCENT_HEIGHT, TABLE_MAX_PERCENT_HEIGHT))
				errors[nerrors++] = XFE_INVALID_TABLE_HEIGHT;
		} else {
			if (RANGE_CHECK(t->iHeight,
							TABLE_MIN_PIXEL_HEIGHT, TABLE_MAX_PIXEL_HEIGHT))
				errors[nerrors++] = XFE_INVALID_TABLE_HEIGHT;
		}
	}

	if (nerrors > 0) {
		fe_editor_range_error_dialog(context, w_data->spacing_text,
									 errors, nerrors);
		return FALSE;
	}

	return TRUE;
}

static void
fe_tables_table_properties_set(MWContext* context,
							   fe_EditorTablesTableStruct* w_data)
{
	EDT_AllTableData table_data;

	if (!fe_EditorTableGetData(context, &table_data))
		return;

	fe_table_properties_common_set(context, &table_data, w_data);

	fe_EditorTableSetData(context, &table_data);
}

static void
fe_tables_table_create_init(MWContext* context,
							fe_EditorTablesTableStruct* w_data)
{
	EDT_AllTableData table_data;

	memset(&table_data, 0, sizeof(EDT_AllTableData));

	table_data.td.iBorderWidth = 1;
	table_data.td.iRows = 2;
	table_data.td.iColumns = 2;
	table_data.td.bWidthPercent = TRUE;
	table_data.td.iWidth = 100;
	table_data.td.bHeightPercent = TRUE;
	table_data.td.iHeight = 100;
	table_data.td.align = ED_ALIGN_LEFT;
	table_data.cd.align = ED_ALIGN_ABSTOP;

	fe_set_numeric_text_field(w_data->number_rows_text,
							  table_data.td.iRows);
	fe_set_numeric_text_field(w_data->number_columns_text,
							  table_data.td.iColumns);

	w_data->inserting = TRUE; /* reset after first insert */
	fe_table_properties_common_init(context, &table_data, w_data);
}

static void
fe_tables_table_create_set(MWContext* context,
							  fe_EditorTablesTableStruct* w_data)
{
	EDT_AllTableData table_data;

	memset(&table_data, 0, sizeof(EDT_AllTableData));

	table_data.td.iRows = 
		fe_get_numeric_text_field(w_data->number_rows_text);
	table_data.td.iColumns = 
		fe_get_numeric_text_field(w_data->number_columns_text);
	 
	fe_table_properties_common_set(context, &table_data, w_data);

	if (w_data->inserting) {
	    fe_EditorTableInsert(context, &table_data);
		w_data->inserting = FALSE; /* can only insert once */
	} else {
	    fe_EditorTableSetData(context, &table_data);
	}
}

static void
fe_cancel_button_set_cancel(Widget widget, Boolean can_cancel)
{
    char* name;
	char* string;
	XmString xm_string;

	if (can_cancel)
	    name = "cancelLabelString";
	else
	    name = "closeLabelString";

	string = fe_ResourceString(widget, name, XmCXmString);
	if (!string)
	  string = name;
	xm_string = XmStringCreateLocalized(string);

	XtVaSetValues(widget, XmNlabelString, xm_string, 0);

	XmStringFree(xm_string);
}

void
fe_EditorTableCreateDialogDo(MWContext* context)
{
	Widget dialog;
	Widget form;
	Widget apply_button;
	Widget cancel_button;
	fe_EditorTablesTableStruct data;
	int done;

	/*
	 *    Make prompt with ok, cancel, apply, separator.
	 */
	form = fe_CreatePromptDialog(context, "tableCreateDialog",
								 TRUE, TRUE, TRUE, TRUE, TRUE);
	dialog = XtParent(form);

	fe_tables_table_create_create(context, form, &data);
	fe_tables_table_create_init(context, &data);

	/*
	 *   Add a bunch of callbacks to the buttons.
	 */
	XtAddCallback(form, XmNokCallback, fe_hrule_ok_cb, &done);
	XtAddCallback(form, XmNapplyCallback, fe_hrule_apply_cb, &done);
	XtAddCallback(form, XmNcancelCallback, fe_hrule_cancel_cb, &done);
	XtAddCallback(form, XmNdestroyCallback, fe_hrule_destroy_cb, &done);

	apply_button = XmSelectionBoxGetChild(form, XmDIALOG_APPLY_BUTTON);
	XtVaSetValues(apply_button, XmNsensitive, FALSE, 0);
	cancel_button = XmSelectionBoxGetChild(form, XmDIALOG_CANCEL_BUTTON);
	fe_cancel_button_set_cancel(cancel_button, TRUE);

    /*
     *    Popup.
     */
    XtManageChild(form);

	/*
     *    Wait.
     */
	fe_NukeBackingStore(dialog); /* what does this do? */

	done = XmDIALOG_NONE;
	while (done == XmDIALOG_NONE) {
		fe_EventLoop();

		if (done == XmDIALOG_OK_BUTTON || done == XmDIALOG_APPLY_BUTTON) {
			if (fe_tables_table_properties_validate(context, &data)) {
				fe_tables_table_create_set(context, &data);
				
				if (done == XmDIALOG_APPLY_BUTTON) {
				    fe_cancel_button_set_cancel(cancel_button, FALSE);
				    done = XmDIALOG_NONE;
				}
			} else {
				done = XmDIALOG_NONE;
			}
		}
	}

	if (done != XFE_DIALOG_DESTROY_BUTTON)
		XtDestroyWidget(dialog);
}

typedef struct fe_EditorTablesRowStruct
{
	Widget horizontal_alignment;
	Widget vertical_alignment;
	Widget color_swatch;
	Widget color_toggle;
	Widget choose_color;
	LO_Color color_value;
} fe_EditorTablesRowStruct;

static ED_Alignment
fe_index_to_ED_Alignment(unsigned index, Boolean horizontal)
{
  switch (index) {
  case 1: return (horizontal)? ED_ALIGN_LEFT: ED_ALIGN_ABSTOP;
  case 2: return ED_ALIGN_ABSCENTER;
  case 3: return (horizontal)? ED_ALIGN_RIGHT: ED_ALIGN_ABSBOTTOM;
  case 4: if (!horizontal) return ED_ALIGN_BASELINE; /*FALLTHRU*/
  case 0:
  default: return ED_ALIGN_DEFAULT;
  }
}

static void
fe_tables_row_properties_init(MWContext* context,
					 fe_EditorTablesRowStruct* w_data)
{
    EDT_TableRowData row_data;
	Boolean enabled = TRUE;
	Boolean has_color;

	if (!fe_EditorTableRowGetData(context, &row_data)) {
	    row_data.align = ED_ALIGN_DEFAULT;
	    row_data.valign = ED_ALIGN_DEFAULT;
	    row_data.pColorBackground = NULL;
		enabled = FALSE;
	}

	fe_SimpleRadioGroupSetWhich(w_data->horizontal_alignment,
								fe_ED_Alignment_to_index(row_data.align));
	fe_SimpleRadioGroupSetSensitive(w_data->horizontal_alignment, enabled);

	fe_SimpleRadioGroupSetWhich(w_data->vertical_alignment,
								fe_ED_Alignment_to_index(row_data.valign));
	fe_SimpleRadioGroupSetSensitive(w_data->vertical_alignment, enabled);

	has_color = (row_data.pColorBackground != NULL);
	if (has_color) {
		w_data->color_value = *row_data.pColorBackground;
    } else {
		fe_EditorDocumentGetColors(context,
								   NULL,
								   &w_data->color_value,
								   NULL,
								   NULL,
								   NULL,
								   NULL);
	}
	XmToggleButtonGadgetSetState(w_data->color_toggle, has_color, FALSE);
	XtVaSetValues(w_data->color_toggle, XmNsensitive, enabled, 0);
	
	fe_SwatchSetColor(w_data->color_swatch, &w_data->color_value);
	fe_SwatchSetSensitive(w_data->color_swatch, has_color);
	fe_WidgetSetSensitive(w_data->choose_color, has_color);
}

static void
fe_tables_row_properties_set(MWContext* context,
					 fe_EditorTablesRowStruct* w_data)
{
    EDT_TableRowData row_data;
	unsigned index;

	index = fe_SimpleRadioGroupGetWhich(w_data->horizontal_alignment);
	row_data.align = fe_index_to_ED_Alignment(index, TRUE);

	index = fe_SimpleRadioGroupGetWhich(w_data->vertical_alignment);
	row_data.valign = fe_index_to_ED_Alignment(index, FALSE);

	if (XmToggleButtonGadgetGetState(w_data->color_toggle)) {
	    row_data.pColorBackground = &w_data->color_value;
	} else {
	    row_data.pColorBackground = NULL;
	}

    fe_EditorTableRowSetData(context, &row_data);
}

static Widget
fe_create_table_text_alignment_group(Widget parent, char* name,
									 Arg* p_args, Cardinal p_n,
									 Widget* h, Widget* v)
{
	Widget frame;
	Widget rc;
	Widget sub_rc;
	Widget horizontal_label;
	Widget horizontal_alignment;
	Widget vertical_label;
	Widget vertical_alignment;
	Arg args[16];
	Cardinal n;
	Widget children[8];
	Cardinal nchildren;

	frame = fe_CreateFrame(parent, name, p_args, p_n);
	XtManageChild(frame);

	n = 0;
	XtSetArg(args[n], XmNorientation, XmHORIZONTAL); n++;
	rc = XmCreateRowColumn(frame, "textAlignment", args, n);
	XtManageChild(rc);

	n = 0;
	XtSetArg(args[n], XmNorientation, XmVERTICAL); n++;
	sub_rc = XmCreateRowColumn(rc, "horizontal", args, n);
	XtManageChild(sub_rc);

	nchildren = 0;
	n = 0;
	horizontal_label = XmCreateLabelGadget(sub_rc, "horizontalLabel", args, n);
	children[nchildren++] = horizontal_label;

	n = 0;
	XtSetArg(args[n], XmNentryAlignment, XmALIGNMENT_BEGINNING); n++;
	XtSetArg(args[n], XmNpacking, XmPACK_COLUMN); n++;
	XtSetArg(args[n], XmNorientation, XmVERTICAL); n++;
	XtSetArg(args[n], XmNbuttons, fe_SimpleOptionHorizontalAlignment); n++;
	horizontal_alignment = fe_CreateSimpleRadioGroup(sub_rc,
											 "horizontalAlignment", args, n);
	children[nchildren++] = horizontal_alignment;

	XtManageChildren(children, nchildren);

	n = 0;
	XtSetArg(args[n], XmNorientation, XmVERTICAL); n++;
	sub_rc = XmCreateRowColumn(rc, "vertical", args, n);
	XtManageChild(sub_rc);

	nchildren = 0;
	n = 0;
	vertical_label = XmCreateLabelGadget(sub_rc, "verticalLabel", args, n);
	children[nchildren++] = vertical_label;

	n = 0;
	XtSetArg(args[n], XmNentryAlignment, XmALIGNMENT_BEGINNING); n++;
	XtSetArg(args[n], XmNpacking, XmPACK_COLUMN); n++;
	XtSetArg(args[n], XmNorientation, XmVERTICAL); n++;
	XtSetArg(args[n], XmNbuttons, fe_SimpleOptionVerticalAlignment); n++;
	vertical_alignment = fe_CreateSimpleRadioGroup(sub_rc,
											 "verticalAlignment", args, n);
	children[nchildren++] = vertical_alignment;

	XtManageChildren(children, nchildren);

	*v = vertical_alignment;
	*h = horizontal_alignment;

	return frame;
}

static void
fe_row_color_toggle_cb(Widget widget, XtPointer closure, XtPointer call_data)
{
	fe_EditorTablesRowStruct* w_data = (fe_EditorTablesRowStruct*)closure;
	Boolean sensitive = XmToggleButtonGadgetGetState(widget);

	fe_editor_set_swatch_stuff(w_data->color_swatch, &w_data->color_value, sensitive);
	fe_WidgetSetSensitive(w_data->choose_color, sensitive);
}

static void
fe_row_choose_color_cb(Widget widget, XtPointer closure, XtPointer cb)
{
	MWContext* context = fe_WidgetToMWContext(widget);
	fe_EditorTablesRowStruct* w_data = (fe_EditorTablesRowStruct*)closure;

	if (fe_ColorPicker(context, &w_data->color_value)) {
		fe_SwatchSetColor(w_data->color_swatch, &w_data->color_value);
	}
}

static Widget
fe_tables_row_properties_create(MWContext* context, Widget parent,
					 fe_EditorTablesRowStruct* w_data)
{
	Widget outside_form;
	Widget frame;
	Widget horizontal_alignment;
	Widget vertical_alignment;
	Widget color_toggle;
	Widget color_swatch;	
	Widget choose_color;	
	Arg args[16];
	Cardinal n;
	Widget children[8];
	Cardinal nchildren;

	outside_form = parent;

	nchildren = 0;
	n = 0;
	XtSetArg(args[n], XmNtopAttachment, XmATTACH_FORM); n++;
	XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM); n++;
	frame = fe_create_table_text_alignment_group(outside_form,
												 "textAlignment", args, n,
												 &horizontal_alignment,
												 &vertical_alignment);
	children[nchildren++] = frame;

	n = 0;
	XtSetArg(args[n], XmNtopAttachment, XmATTACH_WIDGET); n++;
	XtSetArg(args[n], XmNtopWidget, frame); n++;
	XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM); n++;
	color_toggle = XmCreateToggleButtonGadget(outside_form, "rowColorToggle",
											  args, n);
	children[nchildren++] = color_toggle;

	XtAddCallback(color_toggle, XmNvalueChangedCallback, 
				  fe_row_color_toggle_cb, (XtPointer)w_data);

	n = 0;
	XtSetArg(args[n], XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET); n++;
	XtSetArg(args[n], XmNtopWidget, color_toggle); n++;
	XtSetArg(args[n], XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET); n++;
	XtSetArg(args[n], XmNbottomWidget, color_toggle); n++;
	XtSetArg(args[n], XmNleftAttachment, XmATTACH_WIDGET); n++;
	XtSetArg(args[n], XmNleftWidget, color_toggle); n++;
	XtSetArg(args[n], XmNwidth, SWATCH_SIZE); n++;
	color_swatch = fe_CreateSwatch(outside_form, "colorSwatch", args, n);
	children[nchildren++] = color_swatch;

	XtAddCallback(color_swatch, XmNactivateCallback, 
				  fe_ya_picker_cb, (XtPointer)&w_data->color_value);

	n = 0;
	XtSetArg(args[n], XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET); n++;
	XtSetArg(args[n], XmNtopWidget, color_toggle); n++;
	XtSetArg(args[n], XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET); n++;
	XtSetArg(args[n], XmNbottomWidget, color_toggle); n++;
	XtSetArg(args[n], XmNleftAttachment, XmATTACH_WIDGET); n++;
	XtSetArg(args[n], XmNleftWidget, color_swatch); n++;
	choose_color = XmCreatePushButtonGadget(outside_form, "chooseColor", args, n);
	children[nchildren++] = choose_color;

	XtAddCallback(choose_color, XmNactivateCallback, 
				  fe_row_choose_color_cb, (XtPointer)w_data);

	XtManageChildren(children, nchildren);

	w_data->horizontal_alignment = horizontal_alignment;
	w_data->vertical_alignment = vertical_alignment;
	w_data->color_toggle = color_toggle;
	w_data->color_swatch = color_swatch;
	w_data->choose_color = choose_color;

	return outside_form;
}

typedef struct fe_EditorTablesCellStruct
{
	Widget number_rows_text;
	Widget number_columns_text;
	Widget line_width_text;
	Widget horizontal_alignment;
	Widget vertical_alignment;
	Widget header_style;
	Widget wrap_text;
	Widget width_toggle;
	Widget width_text;
	Widget width_units;
	Widget height_toggle;
	Widget height_text;
	Widget height_units;
	Widget color_swatch;
	Widget color_toggle;
	Widget choose_color;
	LO_Color color_value;
} fe_EditorTablesCellStruct;

#if 1
static void
fe_cell_toggle_cb(Widget widget, XtPointer closure, XtPointer cb_data)
{
	fe_EditorTablesCellStruct* w_data = (fe_EditorTablesCellStruct*)closure;
	Boolean enabled = XmToggleButtonGetState(widget);
	MWContext* context = fe_WidgetToMWContext(widget);
	Widget  text;
	Widget  radio;

	if (widget == w_data->width_toggle) {
		text = w_data->width_text;
		radio = w_data->width_units;
	} else if (widget == w_data->height_toggle) {
		text = w_data->height_text;
		radio = w_data->height_units;
	} else if (widget == w_data->color_toggle) {
		text = w_data->width_text;
		radio = w_data->width_units;
		fe_editor_set_swatch_stuff(w_data->color_swatch, &w_data->color_value, enabled);
		fe_WidgetSetSensitive(w_data->choose_color, enabled);
		return;
	}

	fe_TextFieldSetEditable(context, text, enabled);
	fe_SimpleRadioGroupSetSensitive(radio, enabled);
}
#else
static void
fe_cell_toggle_cb(Widget widget, XtPointer closure, XtPointer cb_data)
{
    Widget  first = (Widget)closure;
	Boolean enabled = XmToggleButtonGetState(widget);
	MWContext* context = fe_WidgetToMWContext(widget);
	Widget*  children;
	Cardinal num_children;
	Cardinal i;
	Widget*  group = NULL;

	XtVaGetValues(XtParent(widget),
				  XmNchildren, &children,
				  XmNnumChildren, &num_children, 0);

	for (i = 0; i < num_children; i++) {
	    if (children[i] == first) {
		    group = &children[i];
			break;
		}
	}

	if (!group)
  	    return;

	for (i = 0; i < 3; i++) {
	    if (group[i] == widget)
		    break;
	}

	if (i == 0 || i == 1) { /* width & height */
	    fe_TextFieldSetEditable(context, group[i+3], enabled);
		fe_SimpleRadioGroupSetSensitive(group[i+6], enabled);
	} else if (i == 2) { /* color */
	    XtVaSetValues(group[i+3], XmNsensitive, enabled, 0);
	}
}

#endif

static void
fe_cell_choose_color_cb(Widget widget, XtPointer closure, XtPointer cb)
{
	MWContext* context = fe_WidgetToMWContext(widget);
	fe_EditorTablesCellStruct* w_data = (fe_EditorTablesCellStruct*)closure;

	if (fe_ColorPicker(context, &w_data->color_value)) {
		fe_SwatchSetColor(w_data->color_swatch, &w_data->color_value);
	}
}

static Widget
fe_tables_cell_properties_create(MWContext* context, Widget parent,
					 fe_EditorTablesCellStruct* w_data)
{
	Widget form;
	Widget rows_label;
	Widget rows_text;
	Widget columns_label;
	Widget columns_text;
	Widget columns_units;
	Widget horizontal_alignment;
	Widget vertical_alignment;
	Widget align_frame;
	Widget other_frame;
	Widget rc;
	Widget header_style;
	Widget wrap_text;
	Widget width_toggle;
	Widget width_text;
	Widget width_units;
	Widget height_toggle;
	Widget height_text;
	Widget height_units;
	Widget color_toggle;
	Widget color_swatch;
	Widget choose_color;
	Arg args[16];
	Cardinal n;
	Cardinal nchildren = 0;
	Widget children[20];
	Dimension left_offset;
	Dimension height;
	Cardinal i;

	form = parent;

	n = 0;
	XtSetArg(args[n], XmNtopAttachment, XmATTACH_FORM); n++;
	XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM); n++;
	rows_label = XmCreateLabelGadget(form, "cellRowsLabel", args, n);
	children[nchildren++] = rows_label;

	n = 0;
	XtSetArg(args[n], XmNtopAttachment, XmATTACH_FORM); n++;
	XtSetArg(args[n], XmNleftAttachment, XmATTACH_WIDGET); n++;
	XtSetArg(args[n], XmNleftWidget, children[nchildren-1]); n++;
	XtSetArg(args[n], XmNcolumns, 4); n++;
	rows_text = fe_CreateTextField(form, "cellRowsText", args, n);
	children[nchildren++] = rows_text;

	n = 0;
	XtSetArg(args[n], XmNtopAttachment, XmATTACH_FORM); n++;
	XtSetArg(args[n], XmNleftAttachment, XmATTACH_WIDGET); n++;
	XtSetArg(args[n], XmNleftWidget, children[nchildren-1]); n++;
	columns_label = XmCreateLabelGadget(form, "cellColumnsLabel", args, n);
	children[nchildren++] = columns_label;

	n = 0;
	XtSetArg(args[n], XmNtopAttachment, XmATTACH_FORM); n++;
	XtSetArg(args[n], XmNleftAttachment, XmATTACH_WIDGET); n++;
	XtSetArg(args[n], XmNleftWidget, children[nchildren-1]); n++;
	XtSetArg(args[n], XmNcolumns, 4); n++;
	columns_text = fe_CreateTextField(form, "cellColumnsText", args, n);
	children[nchildren++] = columns_text;

	n = 0;
	XtSetArg(args[n], XmNtopAttachment, XmATTACH_FORM); n++;
	XtSetArg(args[n], XmNleftAttachment, XmATTACH_WIDGET); n++;
	XtSetArg(args[n], XmNleftWidget, children[nchildren-1]); n++;
	columns_units = XmCreateLabelGadget(form, "cellColumnsUnits", args, n);
	children[nchildren++] = columns_units;

	n = 0;
	XtSetArg(args[n], XmNtopAttachment, XmATTACH_WIDGET); n++;
	XtSetArg(args[n], XmNtopWidget, rows_text); n++;
	XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM); n++;
	align_frame = fe_create_table_text_alignment_group(form, "textAlignment",
													  args, n,
													  &horizontal_alignment,
													  &vertical_alignment);
	children[nchildren++] = align_frame;

	n = 0;
	XtSetArg(args[n], XmNtopAttachment, XmATTACH_WIDGET); n++;
	XtSetArg(args[n], XmNtopWidget, rows_text); n++;
	XtSetArg(args[n], XmNleftAttachment, XmATTACH_WIDGET); n++;
	XtSetArg(args[n], XmNleftWidget, align_frame); n++;
	XtSetArg(args[n], XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET); n++;
	XtSetArg(args[n], XmNbottomWidget, align_frame); n++;
	other_frame = fe_CreateFrame(form, "textOther", args, n);
	children[nchildren++] = other_frame;

	n = 0;
	XtSetArg(args[n], XmNorientation, XmVERTICAL); n++;
	rc = XmCreateRowColumn(other_frame, "textOther", args, n);
	XtManageChild(rc);

	n = 0;
	header_style = XmCreateToggleButtonGadget(rc, "headerStyle", args, n);
	XtManageChild(header_style);

	n = 0;
	wrap_text = XmCreateToggleButtonGadget(rc, "wrapText", args, n);
	XtManageChild(wrap_text);

	XtManageChildren(children, nchildren); nchildren = 0;

	n = 0;
	width_toggle = XmCreateToggleButtonGadget(form, "cellWidthToggle",
											  args, n);
	children[nchildren++] = width_toggle;

	n = 0;
	height_toggle = XmCreateToggleButtonGadget(form, "cellHeightToggle",
											  args, n);
	children[nchildren++] = height_toggle;

	n = 0;
	color_toggle = XmCreateToggleButtonGadget(form, "cellColorToggle",
											  args, n);
	children[nchildren++] = color_toggle;

	left_offset = fe_get_offset_from_widest(children, nchildren);

	n = 0;
	XtSetArg(args[n], XmNtopAttachment, XmATTACH_WIDGET); n++;
	XtSetArg(args[n], XmNtopWidget, align_frame); n++;
	XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM); n++;
	XtSetArg(args[n], XmNleftOffset, left_offset); n++;
	XtSetArg(args[n], XmNcolumns, 4); n++;
	width_text = fe_CreateTextField(form, "cellWidthText", args, n);
	children[nchildren++] = width_text;

	n = 0;
	XtSetArg(args[n], XmNtopAttachment, XmATTACH_WIDGET); n++;
	XtSetArg(args[n], XmNtopWidget, width_text); n++;
	XtSetArg(args[n], XmNleftAttachment, XmATTACH_OPPOSITE_WIDGET); n++;
	XtSetArg(args[n], XmNleftWidget, width_text); n++;
	XtSetArg(args[n], XmNrightAttachment, XmATTACH_OPPOSITE_WIDGET); n++;
	XtSetArg(args[n], XmNrightWidget, width_text); n++;
	height_text = fe_CreateTextField(form, "cellHeightText", args, n);
	children[nchildren++] = height_text;

	/* make color swatch match heght of text dudes */
	XtVaGetValues(height_text, XmNheight, &height, 0);

	n = 0;
	XtSetArg(args[n], XmNtopAttachment, XmATTACH_WIDGET); n++;
	XtSetArg(args[n], XmNtopWidget, height_text); n++;
	XtSetArg(args[n], XmNbottomAttachment, XmATTACH_FORM); n++;
	XtSetArg(args[n], XmNleftAttachment, XmATTACH_OPPOSITE_WIDGET); n++;
	XtSetArg(args[n], XmNleftWidget, width_text); n++;
	XtSetArg(args[n], XmNrightAttachment, XmATTACH_OPPOSITE_WIDGET); n++;
	XtSetArg(args[n], XmNrightWidget, width_text); n++;
	XtSetArg(args[n], XmNheight, height); n++;
	color_swatch = fe_CreateSwatch(form, "cellColorSwatch", args, n);
	children[nchildren++] = color_swatch;

	XtAddCallback(color_swatch, XmNactivateCallback, 
				  fe_ya_picker_cb, (XtPointer)&w_data->color_value);

	for (i = 0; i < 3; i++) {
		n = 0;
		XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM); n++;
		XtSetArg(args[n], XmNtopAttachment, XmATTACH_WIDGET); n++;
		if (i == 0) {
			XtSetArg(args[n], XmNtopWidget, align_frame); n++;
		} else {
			XtSetArg(args[n], XmNtopWidget, children[i+2]); n++;
		}
		XtSetValues(children[i], args, n);

		XtAddCallback(children[i], XmNvalueChangedCallback, 
					  fe_cell_toggle_cb, (XtPointer)w_data);
	}

	n = 0;
	XtSetArg(args[n], XmNtopAttachment, XmATTACH_WIDGET); n++;
	XtSetArg(args[n], XmNtopWidget, align_frame); n++;
	XtSetArg(args[n], XmNleftAttachment, XmATTACH_WIDGET); n++;
	XtSetArg(args[n], XmNleftWidget, width_text); n++;
	XtSetArg(args[n], XmNentryAlignment, XmALIGNMENT_BEGINNING); n++;
	XtSetArg(args[n], XmNpacking, XmPACK_TIGHT); n++;
	XtSetArg(args[n], XmNorientation, XmHORIZONTAL); n++;
	XtSetArg(args[n], XmNbuttons, fe_SimpleOptionPixelPercent); n++;
	width_units = fe_CreateSimpleRadioGroup(form, "widthUnits", args, n);
	children[nchildren++] = width_units;

	n = 0;
	XtSetArg(args[n], XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET); n++;
	XtSetArg(args[n], XmNtopWidget, height_text); n++;
	XtSetArg(args[n], XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET); n++;
	XtSetArg(args[n], XmNbottomWidget, height_text); n++;
	XtSetArg(args[n], XmNleftAttachment, XmATTACH_WIDGET); n++;
	XtSetArg(args[n], XmNleftWidget, width_text); n++;
	XtSetArg(args[n], XmNentryAlignment, XmALIGNMENT_BEGINNING); n++;
	XtSetArg(args[n], XmNpacking, XmPACK_TIGHT); n++;
	XtSetArg(args[n], XmNorientation, XmHORIZONTAL); n++;
	XtSetArg(args[n], XmNbuttons, fe_SimpleOptionPixelPercent); n++;
	height_units = fe_CreateSimpleRadioGroup(form, "heightUnits", args, n);
	children[nchildren++] = height_units;

	n = 0;
#if 1
	XtSetArg(args[n], XmNtopAttachment, XmATTACH_WIDGET); n++;
	XtSetArg(args[n], XmNtopWidget, height_text); n++;
	XtSetArg(args[n], XmNbottomAttachment, XmATTACH_FORM); n++;
#else
	XtSetArg(args[n], XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET); n++;
	XtSetArg(args[n], XmNtopWidget, color_swatch); n++;
	XtSetArg(args[n], XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET); n++;
	XtSetArg(args[n], XmNbottomWidget, color_swatch); n++;
#endif
	XtSetArg(args[n], XmNleftAttachment, XmATTACH_WIDGET); n++;
	XtSetArg(args[n], XmNleftWidget, width_text); n++;
	choose_color = XmCreatePushButtonGadget(form, "chooseColor", args, n);
	children[nchildren++] = choose_color;

	XtAddCallback(choose_color, XmNactivateCallback, 
				  fe_cell_choose_color_cb, (XtPointer)w_data);
	
	XtManageChildren(children, nchildren);

	w_data->number_rows_text = rows_text;
	w_data->number_columns_text = columns_text;
	w_data->horizontal_alignment = horizontal_alignment;
	w_data->vertical_alignment = vertical_alignment;
	w_data->header_style = header_style;
	w_data->wrap_text = wrap_text;
	w_data->width_toggle = width_toggle;
	w_data->width_text = width_text;
	w_data->width_units = width_units;
	w_data->height_toggle = height_toggle;
	w_data->height_text = height_text;
	w_data->height_units = height_units;
	w_data->color_toggle = color_toggle;
	w_data->color_swatch = color_swatch;
	w_data->choose_color = choose_color;

	return form;
}

static void
fe_tables_cell_properties_init(MWContext* context,
					 fe_EditorTablesCellStruct* w_data)
{
	Boolean is_nested = EDT_IsInsertPointInNestedTable(context);
    EDT_TableCellData cell_data;
	Boolean enabled = TRUE;
	Boolean has_color;
	Widget widget;

	if (!fe_EditorTableCellGetData(context, &cell_data)) {
	    memset(&cell_data, 0, sizeof(EDT_TableCellData));
		cell_data.iColSpan = 1;
		cell_data.iRowSpan = 1;
	    cell_data.align = ED_ALIGN_DEFAULT;
	    cell_data.valign = ED_ALIGN_DEFAULT;
	    cell_data.pColorBackground = NULL;
		enabled = FALSE;
	}

	fe_set_numeric_text_field(w_data->number_rows_text,
							  cell_data.iRowSpan);
	fe_TextFieldSetEditable(context, w_data->number_rows_text, enabled);
	fe_set_numeric_text_field(w_data->number_columns_text,
							  cell_data.iColSpan);
	fe_TextFieldSetEditable(context, w_data->number_columns_text, enabled);

	fe_SimpleRadioGroupSetWhich(w_data->horizontal_alignment,
								fe_ED_Alignment_to_index(cell_data.align));
	fe_SimpleRadioGroupSetSensitive(w_data->horizontal_alignment, enabled);

	fe_SimpleRadioGroupSetWhich(w_data->vertical_alignment,
								fe_ED_Alignment_to_index(cell_data.valign));
	fe_SimpleRadioGroupSetSensitive(w_data->vertical_alignment, enabled);

	XmToggleButtonGadgetSetState(w_data->header_style, cell_data.bHeader,
								 FALSE);
	XtVaSetValues(w_data->header_style, XmNsensitive, enabled, 0);
	XmToggleButtonGadgetSetState(w_data->wrap_text, cell_data.bNoWrap, FALSE);
	XtVaSetValues(w_data->wrap_text, XmNsensitive, enabled, 0);

	fe_table_tbr_set(context, w_data->width_toggle,
					 w_data->width_text, w_data->width_units,
					 cell_data.bWidthDefined,
					 cell_data.iWidth,
					 (cell_data.bWidthPercent));
	XtVaSetValues(w_data->width_toggle, XmNsensitive, enabled, 0);

	widget = fe_SimpleRadioGroupGetChild(w_data->width_units, 1);
	fe_table_percent_label_set(widget, is_nested);

	fe_table_tbr_set(context, w_data->height_toggle,
					 w_data->height_text, w_data->height_units,
					 cell_data.bHeightDefined,
					 cell_data.iHeight,
					 (cell_data.bHeightPercent));
	XtVaSetValues(w_data->height_toggle, XmNsensitive, enabled, 0);

	widget = fe_SimpleRadioGroupGetChild(w_data->height_units, 1);
	fe_table_percent_label_set(widget, is_nested);
	
	has_color = (cell_data.pColorBackground != NULL);
	if (has_color) {
		w_data->color_value = *cell_data.pColorBackground;
    } else {
		fe_EditorDocumentGetColors(context,
								   NULL,
								   &w_data->color_value,
								   NULL,
								   NULL,
								   NULL,
								   NULL);
	}
	XmToggleButtonGadgetSetState(w_data->color_toggle, has_color, FALSE);
	XtVaSetValues(w_data->color_toggle, XmNsensitive, enabled, 0);
	
	fe_SwatchSetColor(w_data->color_swatch, &w_data->color_value);
	fe_SwatchSetSensitive(w_data->color_swatch, has_color);
	fe_WidgetSetSensitive(w_data->choose_color, has_color);
}

static void
fe_tables_cell_properties_set_validate_common(MWContext* context,
										  EDT_TableCellData* cell_data,
										  fe_EditorTablesCellStruct* w_data)
{
	unsigned index;

	cell_data->iRowSpan = 
		fe_get_numeric_text_field(w_data->number_rows_text);
	cell_data->iColSpan = 
		fe_get_numeric_text_field(w_data->number_columns_text);
	 
	index = fe_SimpleRadioGroupGetWhich(w_data->horizontal_alignment);
	cell_data->align = fe_index_to_ED_Alignment(index, TRUE);

	index = fe_SimpleRadioGroupGetWhich(w_data->vertical_alignment);
	cell_data->valign = fe_index_to_ED_Alignment(index, FALSE);

	cell_data->bHeader = XmToggleButtonGadgetGetState(w_data->header_style);
	cell_data->bNoWrap = XmToggleButtonGadgetGetState(w_data->wrap_text);

	cell_data->bWidthDefined = XmToggleButtonGetState(w_data->width_toggle);
	cell_data->iWidth = fe_get_numeric_text_field(w_data->width_text);
	cell_data->bWidthPercent =
	  (fe_SimpleRadioGroupGetWhich(w_data->width_units) == 1);

	cell_data->bHeightDefined =
		XmToggleButtonGetState(w_data->height_toggle);
	cell_data->iHeight =
		fe_get_numeric_text_field(w_data->height_text);
	cell_data->bHeightPercent =
		(fe_SimpleRadioGroupGetWhich(w_data->height_units) == 1);

	if (XmToggleButtonGadgetGetState(w_data->color_toggle)) {
	    cell_data->pColorBackground = &w_data->color_value;
	} else {
	    cell_data->pColorBackground = NULL;
	}
}

static Boolean
fe_tables_cell_properties_validate(MWContext* context,
								   fe_EditorTablesCellStruct* w_data)
{
	EDT_TableCellData cell_data;
	EDT_TableCellData* c = &cell_data;
	unsigned         errors[16];
	unsigned         nerrors = 0;
	
	fe_tables_cell_properties_set_validate_common(context,
												  &cell_data,
												  w_data);

	if (RANGE_CHECK(c->iRowSpan, TABLE_MIN_ROWS, TABLE_MAX_ROWS))
		errors[nerrors++] = XFE_INVALID_CELL_NROWS;
	if (RANGE_CHECK(c->iColSpan, TABLE_MIN_COLUMNS, TABLE_MAX_COLUMNS))
		errors[nerrors++] = XFE_INVALID_CELL_NCOLUMNS;

	if (c->bWidthDefined) {
		if (c->bWidthPercent) {
			if (RANGE_CHECK(c->iWidth,
						TABLE_MIN_PERCENT_WIDTH, TABLE_MAX_PERCENT_WIDTH))
				errors[nerrors++] = XFE_INVALID_CELL_WIDTH;
		} else {
			if (RANGE_CHECK(c->iWidth,
							TABLE_MIN_PIXEL_WIDTH, TABLE_MAX_PIXEL_WIDTH))
				errors[nerrors++] = XFE_INVALID_CELL_WIDTH;
		}
	}
		
	if (c->bHeightDefined) {
		if (c->bHeightPercent) {
			if (RANGE_CHECK(c->iHeight,
						TABLE_MIN_PERCENT_HEIGHT, TABLE_MAX_PERCENT_HEIGHT))
				errors[nerrors++] = XFE_INVALID_CELL_HEIGHT;
		} else {
			if (RANGE_CHECK(c->iHeight,
							TABLE_MIN_PIXEL_HEIGHT, TABLE_MAX_PIXEL_HEIGHT))
				errors[nerrors++] = XFE_INVALID_CELL_HEIGHT;
		}
	}

	if (nerrors > 0) {
		fe_editor_range_error_dialog(context, w_data->number_rows_text,
									 errors, nerrors);
		return FALSE;
	}

	return TRUE;
}

static void
fe_tables_cell_properties_set(MWContext* context,
							  fe_EditorTablesCellStruct* w_data)
{
    EDT_TableCellData cell_data;

	fe_tables_cell_properties_set_validate_common(context,
												  &cell_data,
												  w_data);

    fe_EditorTableCellSetData(context, &cell_data);
}

static Widget
fe_CreateFolder(Widget parent, char* name, Arg* args, Cardinal n)
{
	Widget folder;

	folder = XtVaCreateWidget(name, xmlFolderWidgetClass, parent,
							  XmNshadowThickness, 2,
							  XmNtopAttachment, XmATTACH_FORM,
							  XmNleftAttachment, XmATTACH_FORM,
							  XmNrightAttachment, XmATTACH_FORM,
							  XmNbottomAttachment, XmATTACH_FORM,
#ifdef ALLOW_TAB_ROTATE
							  XmNtabPlacement, XmFOLDER_LEFT,
							  XmNrotateWhenLeftRight, FALSE,
#endif /* ALLOW_TAB_ROTATE */
							  XmNbottomOffset, 3,
							  XmNspacing, 1,
							  NULL);

	return folder;
}

void
fe_EditorTablePropertiesDialogDo(MWContext* context,
								 fe_EditorPropertiesDialogType type)
{
	Widget dialog;
	Widget folder;
	Widget tab_form;
	int done;
	fe_EditorTablesTableStruct table;
	fe_EditorTablesRowStruct   row;
	fe_EditorTablesCellStruct  cell;
	Arg args[4];
	Cardinal n;
	unsigned tab_number;

	if (type == XFE_EDITOR_PROPERTIES_TABLE_ROW)
		tab_number = 1;
	else if (type == XFE_EDITOR_PROPERTIES_TABLE_CELL)
		tab_number = 2;
	else
		tab_number = 0;

	/*
	 *    Make prompt with ok, cancel, no apply, separator.
	 */
	dialog = fe_CreatePromptDialog(context, "tablePropertiesDialog",
								 TRUE, TRUE, TRUE, FALSE, TRUE);

	n = 0;
	folder = fe_CreateFolder(dialog, "folder", args, n);
    XtManageChild(folder);

	n = 0;
	tab_form = fe_CreateTabForm(folder, "table", args, n);
    XtManageChild(tab_form);
	fe_tables_table_properties_create(context, tab_form, &table);
	fe_tables_table_properties_init(context, &table);
	
	n = 0;
	tab_form = fe_CreateTabForm(folder, "row", args, n);
    XtManageChild(tab_form);
	fe_tables_row_properties_create(context, tab_form, &row);
	fe_tables_row_properties_init(context, &row);

	n = 0;
	tab_form = fe_CreateTabForm(folder, "cell", args, n);
    XtManageChild(tab_form);
	fe_tables_cell_properties_create(context, tab_form, &cell);
	fe_tables_cell_properties_init(context, &cell);

	XmLFolderSetActiveTab(folder, tab_number, True);

	/*
	 *   Add a bunch of callbacks to the buttons.
	 */
	XtAddCallback(dialog, XmNokCallback, fe_hrule_ok_cb, &done);
	XtAddCallback(dialog, XmNapplyCallback, fe_hrule_apply_cb, &done);
	XtAddCallback(dialog, XmNcancelCallback, fe_hrule_cancel_cb, &done);
	XtAddCallback(dialog, XmNdestroyCallback, fe_hrule_destroy_cb, &done);

    /*
     *    Popup.
     */
    XtManageChild(dialog);

	/*
     *    Wait.
     */
	fe_NukeBackingStore(dialog); /* what does this do? */

	done = XmDIALOG_NONE;
	EDT_BeginBatchChanges(context);
	while (done == XmDIALOG_NONE) {
		fe_EventLoop();

		if (done == XmDIALOG_OK_BUTTON || done == XmDIALOG_APPLY_BUTTON) {
			/*
			 *    We can only handle one lot of errors at a time.
			 *    This a little dumb, but it shouldn't happen often.
			 */
			if (fe_tables_table_properties_validate(context, &table)) {
				fe_tables_table_properties_set(context, &table);
			} else {
				done = XmDIALOG_NONE;
				continue;
			}

			if (fe_tables_cell_properties_validate(context, &cell)) {
				fe_tables_cell_properties_set(context, &cell);
			} else {
				done = XmDIALOG_NONE;
				continue;
			}

			fe_tables_row_properties_set(context, &row);

			if (done == XmDIALOG_APPLY_BUTTON)
				done = XmDIALOG_NONE;
		}
	}
	EDT_EndBatchChanges(context);

	if (done != XFE_DIALOG_DESTROY_BUTTON)
		XtDestroyWidget(dialog);
}

typedef struct fe_PublishDialogStruct
{
    MWContext* context;
    Widget local_publish_info;
    Widget local_include_images;
    Widget local_include_files;
    Widget local_list;
    Widget local_select_all;
    Widget local_select_none;
    Widget publish_location_text;
    Widget publish_username_text;
    Widget publish_password_text;
    Widget publish_save_password;
    Widget publish_use_default;
    char*  directory;
	char*  basename;
    char** files;
} fe_PublishDialogStruct;

static void
free_string_array(char** vector)
{
    unsigned n;
	if (vector) {
	    for (n = 0; vector[n] != NULL; n++)
		    XP_FREE(vector[n]);
		XP_FREE(vector);
	}
}

static void
fe_publish_set_include_all_files(fe_PublishDialogStruct* w_data)
{
    MWContext* context = w_data->context;
    char** all_files;
	unsigned n = 0;
	XmString xm_string;
	unsigned len = strlen(w_data->directory);
	char* s;
	char* tmp;

	free_string_array(w_data->files);

	all_files = NET_AssembleAllFilesInDirectory(context, w_data->directory);

	XmListDeleteAllItems(w_data->local_list);

	for (n = 0; all_files && all_files[n] != NULL; n++) {
		s = all_files[n];

		if (strncmp(s, w_data->directory, len) == 0 && s[len] == '/') {
		    tmp = XP_STRDUP(&s[len+1]);
			XP_FREE(s);
			all_files[n] = tmp;
			s = tmp;
		}

		xm_string = XmStringCreateLocalized(s);
	    XmListAddItem(w_data->local_list, xm_string, n+1);
		XmStringFree(xm_string);
	}
	if (n > 0)
		w_data->files = all_files;
	else
		w_data->files = NULL;

	XmToggleButtonGadgetSetState(w_data->local_include_images, FALSE, FALSE);
	XmToggleButtonGadgetSetState(w_data->local_include_files, TRUE, FALSE);
	XtVaSetValues(w_data->local_select_all, XmNsensitive, TRUE, 0);
	XtVaSetValues(w_data->local_select_none, XmNsensitive, TRUE, 0);
}

static void
fe_publish_set_include_image_files(fe_PublishDialogStruct* w_data)
{
    MWContext* context = w_data->context;
    char* image_files = EDT_GetAllDocumentFiles(context);
	unsigned len;
	char* s;
	unsigned n = 0;
	XmString xm_string;
	char** all_files;

	free_string_array(w_data->files);

	XmListDeleteAllItems(w_data->local_list);

	if (image_files) {
	    n = 0;
	    for (s = image_files; (len = XP_STRLEN(s)) != 0; s += len + 1)
		  n++;

		if (n)
		    all_files = XP_ALLOC(sizeof(char*) * (n+1));
		else
		    all_files= NULL;

		n = 0;
	    for (s = image_files; (len = XP_STRLEN(s)) != 0; s += len + 1) {
		    xm_string = XmStringCreateLocalized(s);
			XmListAddItem(w_data->local_list, xm_string, n+1);
			XmStringFree(xm_string);
			all_files[n++] = XP_STRDUP(s);
		}
		all_files[n] = NULL;

	    XP_FREE(image_files);

		w_data->files = all_files;

	} else {
		w_data->files = NULL;
	}

	/*
	 *    Gray out if there are no images in doc.
	 */
	XtVaSetValues(w_data->local_include_images, XmNsensitive, (n > 0), 0);

	XmToggleButtonGadgetSetState(w_data->local_include_images, TRUE, FALSE);
	XmToggleButtonGadgetSetState(w_data->local_include_files, FALSE, FALSE);
	XtVaSetValues(w_data->local_select_all, XmNsensitive, TRUE, 0);
	XtVaSetValues(w_data->local_select_none, XmNsensitive, TRUE, 0);
}

static void
fe_publish_include_select_all(fe_PublishDialogStruct* w_data)
{
	unsigned n;
	unsigned nitems;

	XtVaGetValues(w_data->local_list, XmNitemCount, &nitems, 0);

	for (n = 0; n < nitems; n++)
	    XmListSelectPos(w_data->local_list, n+1, FALSE);
}

static void
fe_publish_include_image_files_cb(Widget widget, XtPointer closure,
								  XtPointer cb_data)
{
    fe_PublishDialogStruct* w_data = (fe_PublishDialogStruct*)closure;
	fe_publish_set_include_image_files(w_data);
	fe_publish_include_select_all(w_data);
}

static void
fe_publish_include_all_files_cb(Widget widget, XtPointer closure,
								  XtPointer cb_data)
{
    fe_PublishDialogStruct* w_data = (fe_PublishDialogStruct*)closure;
	fe_publish_set_include_all_files(w_data);
	fe_publish_include_select_all(w_data);
}

static void
fe_publish_include_select_none_cb(Widget widget, XtPointer closure,
								  XtPointer cb_data)
{
    fe_PublishDialogStruct* w_data = (fe_PublishDialogStruct*)closure;

	XmListDeselectAllItems(w_data->local_list);
}

static void
fe_publish_include_select_all_cb(Widget widget, XtPointer closure,
								  XtPointer cb_data)
{
    fe_PublishDialogStruct* w_data = (fe_PublishDialogStruct*)closure;
	fe_publish_include_select_all(w_data);
}

static void
fe_publish_use_default_cb(Widget widget, XtPointer closure,
								  XtPointer cb_data)
{
    fe_PublishDialogStruct* w_data = (fe_PublishDialogStruct*)closure;
	char* location;
	char* username;
	char* password;

	fe_EditorPreferencesGetPublishLocation(w_data->context,
										   &location,
										   &username,
										   &password);

	if (location) {
	    fe_TextFieldSetString(w_data->publish_location_text, location, FALSE);
		XP_FREE(location);

		if (username) {
		    fe_TextFieldSetString(w_data->publish_username_text, username, FALSE);
			XP_FREE(username);
		} else {
		    fe_TextFieldSetString(w_data->publish_username_text, "", FALSE);
		}

		if (password) {
		    fe_TextFieldSetString(w_data->publish_password_text, password,
								  FALSE);
			memset(password, 0, strlen(password));
			XP_FREE(password);
			XtVaSetValues(w_data->publish_save_password, XmNset, TRUE, 0);
		} else {
		    fe_TextFieldSetString(w_data->publish_password_text, "", FALSE);
			XtVaSetValues(w_data->publish_save_password, XmNset, FALSE, 0);
		}
	}
}

static void
fe_publish_dialog_init(MWContext* context, fe_PublishDialogStruct* w_data)
{
	XmString xm_string;
	History_entry* hist_ent;
	char* local_filename;
	char  buf[MAXPATHLEN];
	char* location;
	char* username;
	char* password;
	char* p;
	Boolean has_default;
	Boolean save_password;

	w_data->directory = NULL;

	/* get the location */
	hist_ent = SHIST_GetCurrent(&context->hist);
	if(hist_ent && hist_ent->address) {

	    XP_ConvertUrlToLocalFile(hist_ent->address, &local_filename);

		if (local_filename) {

		    FE_CondenseURL(buf, local_filename, 40);

			xm_string = XmStringCreateLtoR
					(buf,XmFONTLIST_DEFAULT_TAG);
			XtVaSetValues(w_data->local_publish_info,
						  XmNlabelString, xm_string, 0);
			XmStringFree(xm_string);

			p = strrchr(local_filename, '/'); /* find last slash */
			if (p) { /* should be */
				w_data->basename = XtNewString(&p[1]);
				*p = '\0'; /* we are about to free it anyway */
				w_data->directory = XtNewString(local_filename);
			} else { /* actually this shouldn't happen */
				w_data->basename = XtNewString(local_filename);
				w_data->directory = XtNewString(".");
			}

			XP_FREE(local_filename);
		}
    }

	w_data->files = NULL;
	fe_publish_set_include_image_files(w_data);
	fe_publish_include_select_all(w_data);

	fe_EditorPreferencesGetPublishLocation(context,
										   &location,
										   NULL,
										   NULL);

	if (location) {
		has_default = TRUE;
		XP_FREE(location);
	} else {
		has_default = FALSE;
	}

	/*
	 *    Setup lower half of dialog.
	 */
	save_password = fe_EditorDefaultGetLastPublishLocation(context,
														   &location,
														   &username,
														   &password);

	if ((location == NULL || location[0] == '\0') && has_default) {
		save_password = fe_EditorPreferencesGetPublishLocation(context,
															   &location,
															   &username,
															   &password);
	}
	
	if (location) {
	    fe_TextFieldSetString(w_data->publish_location_text, location, FALSE);
		XP_FREE(location);
	}
	
	if (username) {
	    fe_TextFieldSetString(w_data->publish_username_text, username, FALSE);
		XP_FREE(username);
	}
	
	if (password) {
	    fe_TextFieldSetString(w_data->publish_password_text, password, FALSE);
		memset(password, 0, strlen(password));
		XP_FREE(password);
	}
	
	XtVaSetValues(w_data->publish_use_default, XmNsensitive, has_default, 0);
	XtVaSetValues(w_data->publish_save_password, XmNset, save_password, 0);
	
	/*FIXME*/
	/*
	 *    Do the choices list.
	 */
}

Boolean
fe_EditorPublishFiles(MWContext* context,
					  char*      target_directory,
					  char**     source_files,
					  char*      username,
					  char*      password)
{
	char* full_location = NULL;
	Boolean rv;

	rv = NET_MakeUploadURL(&full_location, target_directory,
						   username, password);

	if (rv && full_location != NULL) {
		NET_PublishFiles(context, source_files, full_location);

#if 0
		XP_FREE(full_location);
#endif

		return TRUE;
	} else {
		return FALSE;
	}
}

static Boolean
fe_publish_dialog_validate(MWContext* context, fe_PublishDialogStruct* w_data)
{
	char* target_directory;
	Boolean rv  = TRUE;

	target_directory = XmTextFieldGetString(w_data->publish_location_text);

	fe_StringTrim(target_directory);

	if (strncasecmp(target_directory, "ftp://", 6) != 0 &&
		strncasecmp(target_directory, "http://", 7) != 0) {

		fe_error_dialog(context, w_data->publish_location_text,
						XP_GetString(XFE_EDITOR_PUBLISH_LOCATION_INVALID));
		rv = FALSE;
	}

	XtFree(target_directory);
	return rv;
}

static void
fe_publish_dialog_set(MWContext* context, fe_PublishDialogStruct* w_data)
{
    int* items;
	int  nitems;
	int n;
	int item;
	char* target_directory;
	char* username;
	char* password;
	char** source_files;
	char* file;
	unsigned dirname_len;
	unsigned basename_len;
	Boolean  save_password;

	XmListGetSelectedPos(w_data->local_list, &items, &nitems);

	source_files = (char**)XP_ALLOC(sizeof(char*) * (nitems + 2));

	source_files[0] = (char*)XP_ALLOC(strlen(w_data->directory) + 
									  strlen(w_data->basename) + 2);

	strcpy(source_files[0], w_data->directory);
	strcat(source_files[0], "/");
	strcat(source_files[0], w_data->basename);

	dirname_len = strlen(w_data->directory);

	for (n = 0; n < nitems; n++) {
		item = items[n] - 1;
		file = w_data->files[item];

		if (file[0] == '/') {
			basename_len = strlen(file);
			source_files[n+1] = (char*)XP_ALLOC(basename_len+1);

			strcpy(source_files[n+1], file);
		} else {
			basename_len = strlen(file);
			source_files[n+1] = (char*)XP_ALLOC(dirname_len+basename_len+2);

			strcpy(source_files[n+1], w_data->directory);
			strcat(source_files[n+1], "/");
			strcat(source_files[n+1], file);
		}
	}
	source_files[n+1] = NULL;

	if (nitems > 0 && items != NULL) /* sometimes get non-zero items */
		XtFree((void*)items);

	target_directory = XmTextFieldGetString(w_data->publish_location_text);
	username = XmTextFieldGetString(w_data->publish_username_text);
	password = fe_GetPasswdText(w_data->publish_password_text);

	fe_EditorPublishFiles(context, target_directory,
						  source_files, username, password);

	/*
	 *    Save the location for next time.
	 */
	XtVaGetValues(w_data->publish_save_password, XmNset, &save_password, 0);

	fe_EditorDefaultSetLastPublishLocation(context,
										   target_directory,
										   username,
										   save_password? password: NULL);
	
#if 0
	free_string_array(source_files);
#endif

	XtFree(target_directory);
	XtFree(username);
	memset(password, 0, strlen(password));
	XtFree(password);
}

static void
fe_publish_dialog_delete(MWContext* context, fe_PublishDialogStruct* w_data)
{
    if (w_data->directory)
	    XtFree(w_data->directory);
    if (w_data->basename)
	    XtFree(w_data->basename);
	free_string_array(w_data->files);
}

static void
fe_publish_dialog_create(MWContext* context, Widget parent,
						 fe_PublishDialogStruct* w_data)
{
    Widget form;
	Widget local_frame;
	Widget local_form;
	Widget local_publish_label;
	Widget local_publish_info;
	Widget local_include_label;
	Widget local_include_radio;
	Widget local_include_files;
	Widget local_include_images;
	Widget local_select_none;
	Widget local_select_all;
	Widget local_list;
	Widget list_parent;

	Widget publish_frame;
	Widget publish_form;
	Widget publish_label;
	Widget publish_drop;
	Widget publish_username_label;
	Widget publish_username_text;
	Widget publish_use_default;
	Widget publish_password_label;
	Widget publish_password_text;
	Widget publish_password_save;

	Widget children[16];
	Cardinal nchildren;
	Cardinal n;
	Arg args[16];
	Dimension left_offset;
	Pixel parent_bg;
	Pixel select_bg;
	Dimension width;
	Widget fat_guy;

	n = 0;
	form = XmCreateForm(parent, "publish", args, n);
	XtManageChild(form);

	n = 0;
	XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM); n++;
	XtSetArg(args[n], XmNrightAttachment, XmATTACH_FORM); n++;
	XtSetArg(args[n], XmNtopAttachment, XmATTACH_FORM); n++;
	local_frame = fe_CreateFrame(form, "localFiles", args, n);
	XtManageChild(local_frame);

	n = 0;
	local_form = XmCreateForm(local_frame, "localFiles", args, n);
	XtManageChild(local_form);

	nchildren = 0;
	n = 0;
	XtSetArg(args[n], XmNalignment, XmALIGNMENT_BEGINNING); n++;
	local_publish_label = XmCreateLabelGadget(local_form, "publishLabel",
											  args, n);
	children[nchildren++] = local_publish_label;

	n = 0;
	XtSetArg(args[n], XmNalignment, XmALIGNMENT_BEGINNING); n++;
	local_include_label = XmCreateLabelGadget(local_form, "includeLabel",
											  args, n);
	children[nchildren++] = local_include_label;

	n = 0;
	local_select_none = XmCreatePushButtonGadget(local_form, "selectNone",
											  args, n);
	children[nchildren++] = local_select_none;

	n = 0;
	local_select_all = XmCreatePushButtonGadget(local_form, "selectAll",
											  args, n);
	children[nchildren++] = local_select_all;

	left_offset = fe_get_offset_from_widest(children, nchildren);

	fat_guy = fe_GetBiggestWidget(TRUE, &children[nchildren-2], 2);
	XtVaGetValues(fat_guy, XmNwidth, &width, 0);

	n = 0;
	XtSetArg(args[n], XmNalignment, XmALIGNMENT_BEGINNING); n++;
	local_publish_info = XmCreateLabelGadget(local_form, "publishInfo",
											 args, n);
	children[nchildren++] = local_publish_info;

	n = 0;
	XtSetArg(args[n], XmNradioBehavior, TRUE); n++;
	XtSetArg(args[n], XmNorientation, XmHORIZONTAL); n++;
	local_include_radio = XmCreateRowColumn(local_form, "includeRadio",
											args, n);
	children[nchildren++] = local_include_radio;

	n = 0;
	XtSetArg(args[n], XmNbackground, &parent_bg); n++;
	XtGetValues(local_form, args, n);
	
	XmGetColors(XtScreen(parent), fe_cmap(context),
				parent_bg, NULL, NULL, NULL, &select_bg);

	n = 0;
	XtSetArg(args[n], XmNvisibleItemCount, 5); n++;
	XtSetArg(args[n], XmNselectionPolicy, XmMULTIPLE_SELECT); n++;
	XtSetArg(args[n], XmNbackground, select_bg); n++;
	local_list = XmCreateScrolledList(local_form, "includeList",
											  args, n);
	children[nchildren++] = list_parent = XtParent(local_list);

	/*
	 *    Now do attachments.
	 */
	n = 0;
	XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM); n++;
	XtSetArg(args[n], XmNtopAttachment, XmATTACH_FORM); n++;
	XtSetValues(local_publish_label, args, n);

	n = 0;
	XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM); n++;
	XtSetArg(args[n], XmNtopAttachment, XmATTACH_WIDGET); n++;
	XtSetArg(args[n], XmNtopWidget, local_publish_info); n++;
	XtSetValues(local_include_label, args, n);

#if 0
	n = 0;
	XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM); n++;
	XtSetArg(args[n], XmNtopAttachment, XmATTACH_WIDGET); n++;
	XtSetArg(args[n], XmNtopWidget, local_include_label); n++;
	XtSetArg(args[n], XmNwidth, width); n++;
	XtSetValues(local_select_none, args, n);
	
	n = 0;
	XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM); n++;
	XtSetArg(args[n], XmNtopAttachment, XmATTACH_WIDGET); n++;
	XtSetArg(args[n], XmNtopWidget, local_select_none); n++;
	XtSetArg(args[n], XmNwidth, width); n++;
	XtSetValues(local_select_all, args, n);

#else
	n = 0;
	XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM); n++;
	XtSetArg(args[n], XmNbottomAttachment, XmATTACH_WIDGET); n++;
	XtSetArg(args[n], XmNbottomWidget, local_select_all); n++;
	XtSetArg(args[n], XmNwidth, width); n++;
	XtSetValues(local_select_none, args, n);
	
	n = 0;
	XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM); n++;
#if 0
	XtSetArg(args[n], XmNbottomAttachment, XmATTACH_FORM); n++;
#else
	XtSetArg(args[n], XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET); n++;
	XtSetArg(args[n], XmNbottomWidget, list_parent); n++;
#endif
	XtSetArg(args[n], XmNwidth, width); n++;
	XtSetValues(local_select_all, args, n);
#endif

	n = 0;
	XtSetArg(args[n], XmNtopAttachment, XmATTACH_FORM); n++;
	XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM); n++;
	XtSetArg(args[n], XmNleftOffset, left_offset); n++;
	XtSetArg(args[n], XmNrightAttachment, XmATTACH_FORM); n++;
	XtSetValues(local_publish_info, args, n);
	
	n = 0;
	XtSetArg(args[n], XmNtopAttachment, XmATTACH_WIDGET); n++;
	XtSetArg(args[n], XmNtopWidget, local_publish_info); n++;
	XtSetArg(args[n], XmNleftAttachment, XmATTACH_OPPOSITE_WIDGET); n++;
	XtSetArg(args[n], XmNleftWidget, local_publish_info); n++;
	XtSetArg(args[n], XmNrightAttachment, XmATTACH_FORM); n++;
	XtSetValues(local_include_radio, args, n);
	
	n = 0;
	XtSetArg(args[n], XmNtopAttachment, XmATTACH_WIDGET); n++;
	XtSetArg(args[n], XmNtopWidget, local_include_radio); n++;
	XtSetArg(args[n], XmNleftAttachment, XmATTACH_OPPOSITE_WIDGET); n++;
	XtSetArg(args[n], XmNleftWidget, local_publish_info); n++;
	XtSetArg(args[n], XmNrightAttachment, XmATTACH_FORM); n++;
#if 0
	XtSetArg(args[n], XmNbottomAttachment, XmATTACH_FORM); n++;
#endif
	XtSetValues(list_parent, args, n);
	
	XtManageChildren(children, nchildren);

	nchildren = 0;
	n = 0;
	XtSetArg(args[n], XmNindicatorType, XmONE_OF_MANY); n++;
	local_include_images = XmCreateToggleButtonGadget(local_include_radio,
													  "includeImages",
													  args, n);
	children[nchildren++] = local_include_images;

	n = 0;
	XtSetArg(args[n], XmNindicatorType, XmONE_OF_MANY); n++;
	local_include_files = XmCreateToggleButtonGadget(local_include_radio,
													  "includeFiles",
													  args, n);
	children[nchildren++] = local_include_files;

	XtManageChildren(children, nchildren);

	XtManageChild(local_list);

	XtAddCallback(local_select_none, XmNactivateCallback,
				  fe_publish_include_select_none_cb, (XtPointer)w_data);

	XtAddCallback(local_select_all, XmNactivateCallback,
				  fe_publish_include_select_all_cb, (XtPointer)w_data);

	XtAddCallback(local_include_images, XmNvalueChangedCallback,
				  fe_publish_include_image_files_cb, (XtPointer)w_data);

	XtAddCallback(local_include_files, XmNvalueChangedCallback,
				  fe_publish_include_all_files_cb, (XtPointer)w_data);

	/*
	 *    Publish group.
	 */
	n = 0;
	XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM); n++;
	XtSetArg(args[n], XmNrightAttachment, XmATTACH_FORM); n++;
	XtSetArg(args[n], XmNtopAttachment, XmATTACH_WIDGET); n++;
	XtSetArg(args[n], XmNtopWidget, local_frame); n++;
	publish_frame = fe_CreateFrame(form, "publishLocation", args, n);
	XtManageChild(publish_frame);

	n = 0;
	publish_form = XmCreateForm(publish_frame, "publishLocation", args, n);
	XtManageChild(publish_form);

	nchildren = 0;
	n = 0;
	XtSetArg(args[n], XmNalignment, XmALIGNMENT_BEGINNING); n++;
	publish_label = XmCreateLabelGadget(publish_form, "publishLabel", args, n);
	children[nchildren++] = publish_label;

	n = 0;
	publish_drop = fe_CreateTextField(publish_form, "publishDrop", args, n);
	children[nchildren++] = publish_drop;

	n = 0;
	XtSetArg(args[n], XmNalignment, XmALIGNMENT_BEGINNING); n++;
	publish_username_label = XmCreateLabelGadget(publish_form, "usernameLabel",
												 args, n);
	children[nchildren++] = publish_username_label;

	n = 0;
	XtSetArg(args[n], XmNalignment, XmALIGNMENT_BEGINNING); n++;
	publish_password_label = XmCreateLabelGadget(publish_form, "passwordLabel",
												 args, n);
	children[nchildren++] = publish_password_label;

	left_offset = fe_get_offset_from_widest(&children[nchildren-2], 2);

	n = 0;
	publish_username_text = fe_CreateTextField(publish_form, "usernameText",
											   args, n);
	children[nchildren++] = publish_username_text;

	n = 0;
	publish_use_default = XmCreatePushButtonGadget(publish_form,
												   "useDefault",
												   args, n);
	children[nchildren++] = publish_use_default;

	XtAddCallback(publish_use_default, XmNactivateCallback,
				  fe_publish_use_default_cb, (XtPointer)w_data);

	n = 0;
	XtSetArg(args[n], XmNmaxLength, 1024); n++;
	publish_password_text = fe_CreatePasswordField(publish_form,
												   "passwordText",
												   args, n);
	children[nchildren++] = publish_password_text;

	n = 0;
	publish_password_save = XmCreateToggleButtonGadget(publish_form,
													   "savePassword",
													   args, n);
	children[nchildren++] = publish_password_save;
	
	/*
	 *    Attachments.
	 */
	n = 0;
	XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM); n++;
	XtSetArg(args[n], XmNtopAttachment, XmATTACH_FORM); n++;
	XtSetValues(publish_label, args, n);
	
	n = 0;
	XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM); n++;
	XtSetArg(args[n], XmNrightAttachment, XmATTACH_FORM); n++;
	XtSetArg(args[n], XmNtopAttachment, XmATTACH_WIDGET); n++;
	XtSetArg(args[n], XmNtopWidget, publish_label); n++;
	XtSetValues(publish_drop, args, n);
	
	n = 0;
	XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM); n++;
	XtSetArg(args[n], XmNtopAttachment, XmATTACH_WIDGET); n++;
	XtSetArg(args[n], XmNtopWidget, publish_drop); n++;
	XtSetValues(publish_username_label, args, n);
	
	n = 0;
	XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM); n++;
	XtSetArg(args[n], XmNtopAttachment, XmATTACH_WIDGET); n++;
	XtSetArg(args[n], XmNtopWidget, publish_username_text); n++;
	XtSetValues(publish_password_label, args, n);
	
	n = 0;
	XtSetArg(args[n], XmNtopAttachment, XmATTACH_WIDGET); n++;
	XtSetArg(args[n], XmNtopWidget, publish_drop); n++;
	XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM); n++;
	XtSetArg(args[n], XmNleftOffset, left_offset); n++;
	XtSetValues(publish_username_text, args, n);
	
	n = 0;
	XtSetArg(args[n], XmNtopAttachment, XmATTACH_WIDGET); n++;
	XtSetArg(args[n], XmNtopWidget, publish_username_text); n++;
	XtSetArg(args[n], XmNleftAttachment, XmATTACH_OPPOSITE_WIDGET); n++;
	XtSetArg(args[n], XmNleftWidget, publish_username_text); n++;
	XtSetArg(args[n], XmNrightAttachment, XmATTACH_OPPOSITE_WIDGET); n++;
	XtSetArg(args[n], XmNrightWidget, publish_username_text); n++;
	XtSetValues(publish_password_text, args, n);
	
	n = 0;
	XtSetArg(args[n], XmNleftAttachment, XmATTACH_WIDGET); n++;
	XtSetArg(args[n], XmNleftWidget, publish_username_text); n++;
	XtSetArg(args[n], XmNtopAttachment, XmATTACH_WIDGET); n++;
	XtSetArg(args[n], XmNtopWidget, publish_drop); n++;
	XtSetValues(publish_use_default, args, n);
	
	n = 0;
	XtSetArg(args[n], XmNleftAttachment, XmATTACH_OPPOSITE_WIDGET); n++;
	XtSetArg(args[n], XmNleftWidget, publish_use_default); n++;
	XtSetArg(args[n], XmNtopAttachment, XmATTACH_WIDGET); n++;
	XtSetArg(args[n], XmNtopWidget, publish_username_text); n++;
	XtSetValues(publish_password_save, args, n);
	
	XtManageChildren(children, nchildren);

	w_data->local_publish_info = local_publish_info;
    w_data->local_include_images = local_include_images;
    w_data->local_include_files = local_include_files;
    w_data->local_list = local_list;
    w_data->local_select_all = local_select_all;
    w_data->local_select_none = local_select_none;
    w_data->publish_location_text = publish_drop;
    w_data->publish_username_text = publish_username_text;
    w_data->publish_password_text = publish_password_text;
    w_data->publish_save_password = publish_password_save;
	w_data->publish_use_default = publish_use_default;
	w_data->context = context;
	w_data->directory = NULL;
	w_data->files = NULL;
}

Boolean
fe_EditorPublishDialogDo(MWContext* context)
{
    fe_PublishDialogStruct publish;
	Widget  dialog;
	int     done;
	Boolean rv;

	/*
	 *    Make prompt with ok, cancel, no apply, separator.
	 */
	dialog = fe_CreatePromptDialog(context, "publishFilesDialog",
								   TRUE, TRUE, FALSE, FALSE, TRUE);

	fe_publish_dialog_create(context, dialog, &publish);
	fe_publish_dialog_init(context, &publish);

	/*
	 *   Add a bunch of callbacks to the buttons.
	 */
	XtAddCallback(dialog, XmNokCallback, fe_hrule_ok_cb, &done);
	XtAddCallback(dialog, XmNcancelCallback, fe_hrule_cancel_cb, &done);
	XtAddCallback(dialog, XmNdestroyCallback, fe_hrule_destroy_cb, &done);

    /*
     *    Popup.
     */
    XtManageChild(dialog);

	/*
     *    Wait.
     */
	fe_NukeBackingStore(dialog); /* what does this do? */

	done = XmDIALOG_NONE;
	while (done == XmDIALOG_NONE) {
		fe_EventLoop();

		if (done == XmDIALOG_OK_BUTTON) {
			if (!fe_publish_dialog_validate(context, &publish)) {
				done = XmDIALOG_NONE;
			}
		}
	}

	/* something is wrong here - benjie */
	/* if you put in a wrong domain, but with http:// and ftp:// 
	 * the following call will fail, and then we loose btn callback 
	 * action and hangs, the problem traces to the exit routine
	 * we send in to NET_GetURL. For the editor, the exit routine,
	 * after poping up the error message, calls fe_EditorGetURLExitRoutine,
	 * which calls fe_EventLoop, and the code loops in there, but don't
  	 * pick up click actions on the buttons in this dialog
     */

	/* ok, so the following line temporarilly fix the above problem 
	 * by poping down the modal dialog.
	 * this fixes 26903. - benjie
     */
	XtUnmanageChild(dialog); 

	if (done == XmDIALOG_OK_BUTTON) {
		fe_publish_dialog_set(context, &publish);
		rv = TRUE;
 	} else {
	    rv = FALSE;
	}

	fe_publish_dialog_delete(context, &publish);

	if (done != XFE_DIALOG_DESTROY_BUTTON) 
		XtDestroyWidget(dialog); 

    return rv;
}

int
fe_YesNoCancelDialog(MWContext* context, char* name, char* message)
{
	Widget dialog;
	Widget mainw = CONTEXT_WIDGET(context);
	Widget yes_button;
	Widget no_button;
	Arg    args[8];
	Cardinal n;
	Visual*  v;
	Colormap cmap;
	Cardinal depth;
	int done;
	XmString xm_message;

	XtVaGetValues(mainw, XtNvisual, &v, XtNcolormap, &cmap,
				  XtNdepth, &depth, 0);

	xm_message = XmStringCreateLtoR(message,XmFONTLIST_DEFAULT_TAG);
	n = 0;
	XtSetArg(args[n], XmNvisual, v); n++;
	XtSetArg(args[n], XmNdepth, depth); n++;
	XtSetArg(args[n], XmNcolormap, cmap); n++;
	XtSetArg(args[n], XmNtransientFor, mainw); n++;
  	XtSetArg(args[n], XmNdialogStyle, XmDIALOG_PRIMARY_APPLICATION_MODAL); n++;
	XtSetArg(args[n], XmNdefaultButtonType, XmDIALOG_OK_BUTTON); n++;
	XtSetArg(args[n], XmNmessageString, xm_message); n++;
	dialog = XmCreateMessageDialog(mainw, name, args, n);
	XmStringFree(xm_message);

	/* Doesn't work as a create argument :-) */
	n = 0;
	XtSetArg(args[n], XmNdialogType, XmDIALOG_QUESTION); n++;
	XtSetValues(dialog, args, n);

#ifdef NO_HELP
	fe_UnmanageChild_safe(XmMessageBoxGetChild(dialog, XmDIALOG_HELP_BUTTON));
#endif

	n = 0;
	yes_button = XmCreatePushButtonGadget(dialog, "yes", args, n);
	XtManageChild(yes_button);

	n = 0;
	no_button = XmCreatePushButtonGadget(dialog, "no", args, n);
	XtManageChild(no_button);

	fe_UnmanageChild_safe(XmMessageBoxGetChild(dialog, XmDIALOG_OK_BUTTON));

	XtAddCallback(dialog, XmNcancelCallback, fe_hrule_cancel_cb, &done);
	XtAddCallback(dialog, XmNdestroyCallback, fe_hrule_destroy_cb, &done);
	XtAddCallback(yes_button, XmNactivateCallback, fe_hrule_ok_cb, &done);
	XtAddCallback(no_button, XmNactivateCallback, fe_hrule_apply_cb, &done);

	XtManageChild(dialog);

	done = XmDIALOG_NONE;
	while (done == XmDIALOG_NONE) {
		fe_EventLoop();
	}
	return done;
}

static char *last_open_url_text = 0;

/* Callback used by the clear button to nuke the contents of the text field. */
static void
fe_clear_text_cb (Widget widget, XtPointer closure, XtPointer call_data)
{
  Widget text = (Widget) closure;
  XtVaSetValues (text, XmNvalue, "", 0);
  /* Focus on the text widget after this, since otherwise you have to
     click again. */
  XmProcessTraversal (text, XmTRAVERSE_CURRENT);
}

struct fe_confirm_data {
  MWContext *context;
  Widget widget;
  Widget text;
  Widget in_editor;
  Widget in_browser;
};

static void
fe_open_url_cb (Widget widget, XtPointer closure, XtPointer call_data)
{
	struct fe_confirm_data *data = (struct fe_confirm_data *) closure;
	char*  text = NULL;
	MWContext* context;
	MWContextType ge_type = MWContextAny;

	if (call_data == NULL ||    /* when it's a destroy callback */
		widget->core.being_destroyed != FALSE || /* on it's way */
		data == NULL)                         /* data from hell */
		return;
	
	context = data->context;

	if (widget == data->in_editor || widget == data->in_browser) {

		text = XmTextFieldGetString(data->text);

		if (! text)
			return;
		
		cleanup_selection(text, text, strlen(text)+1);

		if (*text != '\0') {

			if (last_open_url_text)
				XtFree(last_open_url_text);

			last_open_url_text = text;
		
			if (widget == data->in_editor)
				ge_type = MWContextEditor;
			else if (widget == data->in_browser) 
				ge_type = MWContextBrowser;
		}
    }

	XtUnmanageChild(data->widget);
	XtDestroyWidget(data->widget);
	free(data);
	
	if (ge_type == MWContextEditor) {
		fe_EditorGetURL(context, text);
	} else if (ge_type == MWContextBrowser) {
		fe_BrowserGetURL(context, text);
	}
}

void
fe_EditorOpenURLDialog(MWContext* context)
{
	struct fe_confirm_data *data;
	Widget dialog;
	Widget label;
	Widget text;
	Widget in_browser;
	Widget in_editor;
	Widget form;
	Arg    args[20];
	int    n;
	char*  string;

	dialog = fe_CreatePromptDialog(context, "openURLDialog",
								   FALSE, TRUE, TRUE, TRUE, TRUE);

	n = 0;
	form = XmCreateForm(dialog, "form", args, n);
	XtManageChild(form);

	n = 0;
	XtSetArg(args [n], XmNleftAttachment, XmATTACH_FORM); n++;
	XtSetArg(args [n], XmNtopAttachment, XmATTACH_FORM); n++;
	XtSetArg(args [n], XmNalignment, XmALIGNMENT_BEGINNING); n++;
	label = XmCreateLabelGadget(form, "openURLLabel", args, n);
	XtManageChild(label);

	string = last_open_url_text? last_open_url_text : "";

	n = 0;
	XtSetArg(args [n], XmNleftAttachment, XmATTACH_FORM); n++;
	XtSetArg(args [n], XmNtopAttachment, XmATTACH_WIDGET); n++;
	XtSetArg(args [n], XmNtopWidget, label); n++;
	XtSetArg(args [n], XmNrightAttachment, XmATTACH_FORM); n++;
	XtSetArg(args [n], XmNvalue, string); n++;
    XtSetArg(args [n], XmNeditable, True); n++;
	XtSetArg(args [n], XmNcursorPositionVisible, True); n++;
	text = fe_CreateTextField(form, "openURLText", args, n);
	XtManageChild(text);

	n = 0;
	XtSetArg(args [n], XmNindicatorType, XmONE_OF_MANY); n++;
	in_editor = XmCreatePushButtonGadget(dialog, "openInEditor", args, n);
	XtManageChild(in_editor);

	n = 0;
	XtSetArg(args [n], XmNindicatorType, XmONE_OF_MANY); n++;
	in_browser = XmCreatePushButtonGadget(dialog, "openInBrowser", args, n);
	XtManageChild(in_browser);

	data = (struct fe_confirm_data *)calloc(sizeof(struct fe_confirm_data), 1);
	data->context = context;
	data->widget = dialog;
	data->text = text;
	data->in_editor = in_editor;
	data->in_browser = in_browser;

	if (context->type == MWContextEditor)
		XtVaSetValues(dialog, XmNdefaultButton, in_editor, 0);
	else
		XtVaSetValues(dialog, XmNdefaultButton, in_browser, 0);

	XtAddCallback(in_editor, XmNactivateCallback, fe_open_url_cb, data);
	XtAddCallback(in_browser, XmNactivateCallback, fe_open_url_cb, data);
	XtAddCallback(dialog, XmNcancelCallback, fe_open_url_cb, data);
	XtAddCallback(dialog, XmNdestroyCallback, fe_open_url_cb, data);
	XtAddCallback(dialog, XmNapplyCallback, fe_clear_text_cb, text);
	
	fe_NukeBackingStore (dialog);
	XtManageChild (dialog);
}

Boolean
fe_HintDialog(MWContext* context, char* message)
{
	Widget dialog;
	Widget mainw = CONTEXT_WIDGET(context);
	Arg    args[8];
	Cardinal n;
	Visual*  v;
	Colormap cmap;
	Cardinal depth;
	int done;
	Widget toggle_button;
	Widget toggle_row;
	XmString xm_message;
	Boolean  return_value;

	XtVaGetValues(mainw, XtNvisual, &v, XtNcolormap, &cmap,
				  XtNdepth, &depth, 0);

	xm_message = XmStringCreateLocalized(message);

	n = 0;
	XtSetArg(args[n], XmNvisual, v); n++;
	XtSetArg(args[n], XmNdepth, depth); n++;
	XtSetArg(args[n], XmNcolormap, cmap); n++;
	XtSetArg(args[n], XmNtransientFor, mainw); n++;
	XtSetArg(args[n], XmNdefaultButtonType, XmDIALOG_OK_BUTTON); n++;
	XtSetArg(args[n], XmNmessageString, xm_message); n++;
	dialog = XmCreateInformationDialog(mainw, "hintDialog", args, n);
	XmStringFree(xm_message);

	/*
	 *    This is sooooooo lame. Dispite what the manual says, an
	 *    additonal toggle button child is not placed above the push
	 *    buttons, but rather along aside the. Stick the toggle in
	 *    a row, just to get the thing to do what we want.
	 */
	n = 0;
	toggle_row = XmCreateRowColumn(dialog, "dontDisplayAgainRow", args, n);
	XtManageChild(toggle_row);
	n = 0;
	XtSetArg(args [n], XmNindicatorType, XmN_OF_MANY); n++;
	toggle_button = XmCreateToggleButtonGadget(toggle_row, "dontDisplayAgain",
											   args, n);
	XtManageChild(toggle_button);

	fe_UnmanageChild_safe(XmMessageBoxGetChild(dialog,XmDIALOG_CANCEL_BUTTON));
#ifdef NO_HELP
	fe_UnmanageChild_safe(XmMessageBoxGetChild(dialog, XmDIALOG_HELP_BUTTON));
#endif

	XtAddCallback(dialog, XmNokCallback, fe_hrule_ok_cb, &done);
	XtAddCallback(dialog, XmNdestroyCallback, fe_hrule_destroy_cb, &done);
	
	XtManageChild(dialog);

	done = XmDIALOG_NONE;
	while (done == XmDIALOG_NONE) {
		fe_EventLoop();
	}

	if (done != XFE_DIALOG_DESTROY_BUTTON) {
		return_value = XmToggleButtonGetState(toggle_button);
		XtDestroyWidget(dialog);
		return return_value;
	}

	return FALSE;
}

