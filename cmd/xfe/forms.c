/* -*- Mode: C; tab-width: 8 -*-
   forms.c --- creating and displaying forms widgets
   Copyright © 1995 Netscape Communications Corporation, all rights reserved.
   Created: Jamie Zawinski <jwz@netscape.com>, 21-Jul-94.
 */

#include "mozilla.h"
#include "xfe.h"
#include "fonts.h"
#include "felocale.h"
#include <X11/IntrinsicP.h> /* for XtMoveWidget() */

#ifdef MOCHA
#include <libmocha.h>
#endif /* MOCHA */
#include <libi18n.h>


/* for XP_GetString() */
#include <xpgetstr.h>
extern int XFE_ERROR_OPENING_FILE;

/* Kludge around conflicts between Motif and xp_core.h... */
#undef Bool
#define Bool char

/* #define FORMS_ARE_FONTIFIED */
/* #define FORMS_ARE_COLORED */


/* Kludge kludge kludge, should be in resources... */
#define fe_FORM_LEFT_MARGIN   0
#define fe_FORM_RIGHT_MARGIN  0
#define fe_FORM_TOP_MARGIN    4
#define fe_FORM_BOTTOM_MARGIN 4

/* Hack to overcome mocha destroying document underneath us */
int32 _doc_id_before_calling_mocha = 0;

static void fe_form_file_browse_cb (Widget, XtPointer, XtPointer);
static void fe_activate_submit_cb (Widget, XtPointer, XtPointer);
static void fe_submit_form_cb (Widget, XtPointer, XtPointer);
static void fe_button_form_cb (Widget, XtPointer, XtPointer);
static void fe_reset_form_cb (Widget, XtPointer, XtPointer);
static void fe_radio_form_cb (Widget, XtPointer, XtPointer);
static void fe_check_form_cb (Widget, XtPointer, XtPointer);
static void fe_popup_form_cb (Widget, XtPointer, XtPointer);
static void fe_list_form_cb (Widget, XtPointer, XtPointer);
static void fe_got_focus_cb (Widget, XtPointer, XtPointer);
static void fe_lost_focus_cb (Widget, XtPointer, XtPointer);


struct fe_form_data
{
  MWContext *context;
  LO_FormElementStruct *form;
  Widget widget;
  Widget file_widget;			/* Text Widget for FORM_TYPE_FILE */
  Widget popup_widget;
  int nkids;
  Widget *popup_kids;
  char *selected_p;
};


static void
fe_font_list_metrics(MWContext *context, LO_FormElementStruct *form,
	XmFontList font_list, int *ascentp, int *descentp)
{
	fe_Font		font;
	LO_TextAttr	*text;

	text = form->text_attr;
	font = fe_LoadFontFromFace(context,text, &text->charset, text->font_face,
				   text->size, text->fontmask);
	if (!font)
	{
		*ascentp = 14;
		*descentp = 3;
		return;
	}
	FE_FONT_EXTENTS(text->charset, font, ascentp, descentp);
}


/* Motif likes to bitch and moan when there are binary characters in
   text strings.  Fuck 'em.
 */
void
fe_forms_clean_text (MWContext *context, int charset, char *text, Boolean newlines_too_p)
{
  unsigned char *t = (unsigned char *) text;
  for (; *t; t++)
    {
      unsigned char c = *t;
      if ((newlines_too_p && (c == '\n' || c == '\r')) ||
	  (c < ' ' && c != '\t' && c != '\n' && c != '\r') ||
	  (c == 0x7f) ||
	  ((INTL_CharSetType(charset) == SINGLEBYTE) &&
	   (0x7f <= c) && (c <= 0xa0)))
	*t = ' ';
    }
}


/*
 * This silly button event handler makes sure a widgets coords matches its
 * window coords whenever a button is pressed in it.
 * This is needed to get option menus to pop up after we have
 * scrolled their rowColumns by moving the windows instead of the
 * widgets.
 */
static void
option_press (Widget widget, XtPointer closure, XEvent *event)
{
  XWindowAttributes attr;

  XGetWindowAttributes(XtDisplay(widget), XtWindow(widget), &attr);
  XtVaSetValues (widget, XmNx, (Position)attr.x, XmNy, (Position)attr.y, 0);
}

/*
 * A list key event handler.
 * This will search for a key in the XmList and shift the location cursor
 * to a match.
 */
static void
fe_list_find_eh(Widget widget, XtPointer closure, XEvent* event,
		Boolean* continue_to_dispatch)
{
  MWContext *context = (MWContext *) closure;
  Boolean usedEvent_p = False;

  Modifiers mods;
  KeySym k;
  char *s;
  int i;
  int index;				/* index is 1 based */
  XmString *items;
  int nitems;

#define MILLI_500_SECS	500
#define _LIST_MAX_CHARS	100
  static char input[_LIST_MAX_CHARS] = {0};
  static Time prev_time;
  static Widget w = 0;
  static Boolean is_prev_found = False;

  if (!context || !event) return;

  fe_UserActivity (context);

  if (event->type == KeyPress) {
    if (!(k = XtGetActionKeysym (event, &mods))) return;
    if (!(s = XKeysymToString (k))) return;
    if (!s || !*s || s[1]) return;		/* no or more than 1 char */
    if (!isprint(*s)) return;			/* unprintable */

    if ((widget == w) && (event->xkey.time - prev_time < MILLI_500_SECS)) {
	/* Continue search */
	if (is_prev_found) strcat(input, s);
	else return;
    }
    else {
	/* Fresh search */
	is_prev_found = False;
	strcpy(input, s);
    }
    w = widget;
    prev_time = event->xkey.time;
    index = XmListGetKbdItemPos(widget);
    if (!index) return;				/* List is empty */

    /* Wrap search for input[] in List */
    i = index;
    i = (i == nitems) ? 1 : i+1;		/* Increment i */
    XtVaGetValues(widget, XmNitems, &items, XmNitemCount, &nitems, 0);
    while (i != index) {
	if (!XmStringGetLtoR(items[i-1], XmSTRING_DEFAULT_CHARSET, &s))
	    continue;
	if (!strncmp(s, input, strlen(input))) {
	    /* Found */
	    XP_FREE(s);
	    XmListSetKbdItemPos(widget, i);
	    is_prev_found = True;
	    break;
	}
	XP_FREE(s);
	i = (i == nitems) ? 1 : i+1;	/* Increment i */
    }
    usedEvent_p = True;
  }

  if (usedEvent_p) *continue_to_dispatch = False;
}


static void
fe_get_compatible_colors (MWContext *context, Pixel *fgP, Pixel *bgP,
                          int fgr, int fgg, int fgb,
                          int bgr, int bgg, int bgb)
{
  int fgi = (int) ((fgr * 0.299) + (fgg * 0.587) + (fgb * 0.114));
  int bgi = (int) ((bgr * 0.299) + (bgg * 0.587) + (bgb * 0.114));
  int distance = bgi - fgi;

  if (distance > -80 && distance < 130)
    {
      if (bgi < 146)
        {
# ifdef DEBUG
          fprintf (stderr,
                   "bad form colors (#%02x%02x%02x = %d; %02x%02x%02x = %d);"
                   " setting fg to white.\n",
                   fgr, fgg, fgb, fgi,
                   bgr, bgg, bgb, bgi);
# endif /* DEBUG */
          fgr = fgg = fgb = 255;
        }
      else
        {
# ifdef DEBUG
          fprintf (stderr,
                   "bad form colors (#%02x%02x%02x = %d; %02x%02x%02x = %d);"
                   " setting fg to black.\n",
                   fgr, fgg, fgb, fgi,
                   bgr, bgg, bgb, bgi);
# endif /* DEBUG */
          fgr = fgg = fgb = 0;
        }
    }

  *fgP = fe_GetPixel (context, fgr, fgg, fgb);
  *bgP = fe_GetPixel (context, bgr, bgg, bgb);
}



void
XFE_GetFormElementInfo (MWContext *context, LO_FormElementStruct *form)
{
  Widget parent = CONTEXT_DATA (context)->drawing_area;
  Widget widget;
  XmFontList font_list;
  struct fe_form_data *fed;
  Boolean reinitializing;
  int16 charset;

  charset = form->text_attr->charset;

  switch (form->element_data->type)
    {
    case FORM_TYPE_FILE:
    case FORM_TYPE_TEXT:
    case FORM_TYPE_PASSWORD:
      fed = form->element_data->ele_text.FE_Data;
      break;
    case FORM_TYPE_SUBMIT:
    case FORM_TYPE_RESET:
    case FORM_TYPE_BUTTON:
    case FORM_TYPE_READONLY:
      fed = form->element_data->ele_minimal.FE_Data;
      break;
    case FORM_TYPE_RADIO:
    case FORM_TYPE_CHECKBOX:
      fed = form->element_data->ele_toggle.FE_Data;
      break;
    case FORM_TYPE_SELECT_ONE:
    case FORM_TYPE_SELECT_MULT:
      fed = form->element_data->ele_select.FE_Data;
      break;
    case FORM_TYPE_TEXTAREA:
      fed = form->element_data->ele_textarea.FE_Data;
      break;
    case FORM_TYPE_HIDDEN:
    case FORM_TYPE_JOT:
    case FORM_TYPE_ISINDEX:
    case FORM_TYPE_IMAGE:
    case FORM_TYPE_KEYGEN:
      return;
    default:
      assert (0);
      return;
    }

  if (fed)
    {
      reinitializing = !fed->widget;
    }
  else
    {
      fed = (struct fe_form_data *) calloc (sizeof (struct fe_form_data), 1);
      /* We could either be
	 changing the input element via FE_ChangeInputElement()
	 or
	 creating it for the first time
      */
      if (CONTEXT_DATA(context)->mocha_changing_input_element)
	reinitializing = True;
      else
	reinitializing = False;
    }

  if (! fed->widget)
    {
      /* Create the widget. */
      Arg av [50];
      int ac = 0;
      Pixel fg, bg;
      LO_TextAttr *text = form->text_attr;
      /* Use the prevailing font to decide how big to make the thing. */
      int mask = text->fontmask;

      switch (form->element_data->type)
	{
	case FORM_TYPE_FILE:
	case FORM_TYPE_TEXT:
	case FORM_TYPE_PASSWORD:
	case FORM_TYPE_TEXTAREA:
	case FORM_TYPE_READONLY:
	  /* Make sure text fields are non-bold, non-italic, fixed. */
	  mask &= (~ (LO_FONT_BOLD|LO_FONT_ITALIC));
	  mask |= LO_FONT_FIXED;
	  break;
	default:
	  break;
	}

      {
		XmFontListEntry	flentry;
		XtPointer	fontOrFontSet;
		XmFontType	type;

		fontOrFontSet =
		  fe_GetFontOrFontSetFromFace(context, NULL, text->font_face,
					      text->size, mask, &type);
		if (!fontOrFontSet)
		{
			return;
		}
		flentry = XmFontListEntryCreate(XmFONTLIST_DEFAULT_TAG, type,
			fontOrFontSet);
		if (!flentry)
		{
			return;
		}
		font_list = XmFontListAppendEntry(NULL, flentry);
		if (!font_list)
		{
			return;
		}
		XmFontListEntryFree(&flentry);
      }

#ifdef DEBUG_jwz
      fe_get_compatible_colors (context, &fg, &bg,
                                text->fg.red, text->fg.green, text->fg.blue,
                                text->bg.red, text->bg.green, text->bg.blue);
#else  /* !DEBUG_jwz */
      fg = fe_GetPixel (context, text->fg.red, text->fg.green, text->fg.blue);
      bg = fe_GetPixel (context, text->bg.red, text->bg.green, text->bg.blue);
#endif /* !DEBUG_jwz */

      /* Note: you can't use gadgets here, or events don't get dispatched. */

      switch (form->element_data->type)
	{
	case FORM_TYPE_FILE:
	  {
	    Widget tmp_widget;
	    lo_FormElementTextData *data = &form->element_data->ele_text;
	    int columns = ((fe_LocaleCharSetID & MULTIBYTE) ?
		(data->size + 1) / 2 : data->size);
	    char *tmp_text = 0;
	    char *text = (reinitializing
			  ? (char *) data->current_text
			  : (char *) data->default_text);
	    unsigned char *loc;
	    if (! text) text = (char *) data->default_text;
	    if (! text) text = "";
	    if (data->max_size > 0 && (int) XP_STRLEN (text) >= data->max_size) {
	      tmp_text = XP_STRDUP (text);
	      tmp_text [data->max_size] = '\0';
	      text = tmp_text;
	    }
	    /* XXX maybe we shouldn't do this */
	    fe_forms_clean_text (context, charset, text, True);

	    /* Create a RowColumn widget to handle both the text and its
		associated browseButton */
	    ac = 0;
	    XtSetArg (av [ac], XmNpacking, XmPACK_TIGHT); ac++;
	    XtSetArg (av [ac], XmNorientation, XmHORIZONTAL); ac++;
	    XtSetArg (av [ac], XmNnumColumns, 2); ac++;
	    XtSetArg (av [ac], XmNspacing, 5); ac++;
	    XtSetArg (av [ac], XmNmarginWidth, 0); ac++;
	    XtSetArg (av [ac], XmNmarginHeight, 0); ac++;
	    widget = XmCreateRowColumn(parent, "formRowCol", av, ac);

	    if (columns == 0) columns = 20;
	    ac = 0;
	    XtSetArg (av [ac], XmNfontList, font_list); ac++;
	    XtSetArg (av [ac], XmNcolumns, columns); ac++;
#ifdef FORMS_ARE_COLORED
	    XtSetArg (av [ac], XmNforeground, fg); ac++;
	    XtSetArg (av [ac], XmNbackground, bg); ac++;
#endif
	    if (data->max_size > 0)
	      XtSetArg (av [ac], XmNmaxLength, data->max_size), ac++;
	    fed->file_widget = fe_CreateTextField (widget, "textForm", av, ac);
#ifdef MOCHA
            XtAddCallback(fed->file_widget, XmNlosingFocusCallback,
                                        fe_lost_focus_cb, fed);
            XtAddCallback(fed->file_widget, XmNfocusCallback,
                                        fe_got_focus_cb, fed);
#endif /* MOCHA */

	    XtManageChild(fed->file_widget);

	    ac = 0;
	    XtSetArg (av [ac], XmNfontList, font_list); ac++;
	    XtSetArg (av [ac], XmNforeground, fg); ac++;
	    XtSetArg (av [ac], XmNbackground, bg); ac++;
	    tmp_widget = XmCreatePushButton (widget, "formFileBrowseButton",
							av, ac);
	    XtAddCallback(tmp_widget, XmNactivateCallback,
					fe_form_file_browse_cb, fed);
	    XtManageChild(tmp_widget);

	    data->FE_Data = fed;

	    /* This is changed later for self-submitting fields. */
	    if (fe_globalData.nonterminal_text_translations)
	      XtOverrideTranslations (fed->file_widget, fe_globalData.
				      nonterminal_text_translations);

	    /* Must be after passwd setup. */
	    XtVaSetValues (fed->file_widget, XmNcursorPosition, 0, 0);
	    loc = fe_ConvertToLocaleEncoding (charset, text);
	    /*
	     * I don't know whether it is SGI or some version of Motif
	     * that is fucked, but doing a XtVaSetValues() here will
	     * execute the valueChangedCallback but ignore its changes.
	     * XmTextFieldSetString() works as expected.
	     */
	    XmTextFieldSetString (fed->file_widget, loc);
	    if (((char *) loc) != text) {
	      XP_FREE (loc);
	    }
	    if (tmp_text) XP_FREE (tmp_text);
	    break;
	  }
	case FORM_TYPE_TEXT:
	case FORM_TYPE_PASSWORD:
	  {
	    lo_FormElementTextData *data = &form->element_data->ele_text;
	    int columns = ((fe_LocaleCharSetID & MULTIBYTE) ?
		(data->size + 1) / 2 : data->size);
	    char *tmp_text = 0;
	    char *text = (reinitializing
			  ? (char *) data->current_text
			  : (char *) data->default_text);
	    unsigned char *loc;
	    if (! text) text = (char *) data->default_text;
	    if (! text) text = "";
	    if (data->max_size > 0 && (int) XP_STRLEN (text) >= data->max_size) {
	      tmp_text = XP_STRDUP (text);
	      tmp_text [data->max_size] = '\0';
	      text = tmp_text;
	    }
	    fe_forms_clean_text (context, charset, text, True);
	    if (columns == 0) columns = 20;
	    ac = 0;
	    /* Ok, let's try using the fixed font for text areas only. */
/*#ifdef FORMS_ARE_FONTIFIED*/
	    XtSetArg (av [ac], XmNfontList, font_list); ac++;
/*#endif*/
	    XtSetArg (av [ac], XmNcolumns, columns); ac++;
#ifdef FORMS_ARE_COLORED
	    XtSetArg (av [ac], XmNforeground, fg); ac++;
	    XtSetArg (av [ac], XmNbackground, bg); ac++;
#endif
	    if (data->max_size > 0)
	      XtSetArg (av [ac], XmNmaxLength, data->max_size), ac++;
	    widget = fe_CreateTextField (parent, "textForm", av, ac);
#ifdef MOCHA
            XtAddCallback(widget, XmNlosingFocusCallback,
                                        fe_lost_focus_cb, fed);
	    XtAddCallback(widget, XmNfocusCallback, fe_got_focus_cb, fed);
#endif /* MOCHA */
	    data->FE_Data = fed;

	    /* This is changed later for self-submitting fields. */
	    if (fe_globalData.nonterminal_text_translations)
	      XtOverrideTranslations (widget, fe_globalData.
				      nonterminal_text_translations);

	    if (form->element_data->type == FORM_TYPE_PASSWORD)
	      fe_SetupPasswdText (widget, (data->max_size > 0
					   ? data->max_size : 1024));
	    /* Must be after passwd setup. */
	    XtVaSetValues (widget, XmNcursorPosition, 0, 0);
	    loc = fe_ConvertToLocaleEncoding (charset, text);
	    /*
	     * I don't know whether it is SGI or some version of Motif
	     * that is fucked, but doing a XtVaSetValues() here will
	     * execute the valueChangedCallback but ignore its changes.
	     * XmTextFieldSetString() works as expected.
	     */
	    XmTextFieldSetString (widget, loc);
	    if (((char *) loc) != text) {
	      XP_FREE (loc);
	    }
	    if (tmp_text) XP_FREE (tmp_text);
	    break;
	  }
	case FORM_TYPE_SUBMIT:
	case FORM_TYPE_RESET:
	case FORM_TYPE_BUTTON:
	  {
	    lo_FormElementMinimalData *data = &form->element_data->ele_minimal;
	    XmString xmstring;
	    char *name = (form->element_data->type == FORM_TYPE_SUBMIT
			  ? "formSubmitButton"
			  : form->element_data->type == FORM_TYPE_RESET
			  ? "formResetButton"
			  : "formButton");
	    xmstring = fe_ConvertToXmString (charset,
						(unsigned char *) data->value);
	    ac = 0;
	    if (xmstring)
	      XtSetArg (av [ac], XmNlabelString, xmstring); ac++;
/*#ifdef FORMS_ARE_FONTIFIED*/
	    XtSetArg (av [ac], XmNfontList, font_list); ac++;
/*#endif*/
	    /* Ok, buttons are always text-colored, because OptionMenu buttons
	       must be, and making buttons be a different color looks
	       inconsistent.
	     */
/*#ifdef FORMS_ARE_COLORED*/
	    XtSetArg (av [ac], XmNforeground, fg); ac++;
	    XtSetArg (av [ac], XmNbackground, bg); ac++;
/*#endif*/
	    if (form->element_data->type == FORM_TYPE_SUBMIT)
	      {
		XtSetArg (av [ac], XmNshowAsDefault, 1), ac++;
	      }
	    widget = XmCreatePushButton (parent, name, av, ac);
	    data->FE_Data = fed;
	    if (xmstring)
	      XmStringFree (xmstring);
	    switch (form->element_data->type)
	      {
	      case FORM_TYPE_SUBMIT:
		XtAddCallback (widget, XmNactivateCallback, fe_submit_form_cb,
			       fed);
		break;
	      case FORM_TYPE_RESET:
		XtAddCallback (widget, XmNactivateCallback, fe_reset_form_cb,
			       fed);
		break;
	      case FORM_TYPE_BUTTON:
		XtAddCallback (widget, XmNactivateCallback, fe_button_form_cb,
			       fed);
		break;
	      }
	    break;
	  }
	case FORM_TYPE_RADIO:
	case FORM_TYPE_CHECKBOX:
	  {
	    lo_FormElementToggleData *data = &form->element_data->ele_toggle;
	    XmString xmstring;
	    int margin = 2; /* leave some slack... */
	    int size;
	    int descent;

	    fe_font_list_metrics (context, form, font_list, &size, &descent);
	    size += (margin * 2);

	    ac = 0;
	    xmstring = XmStringCreate (" ", XmFONTLIST_DEFAULT_TAG);
	    /* These always use our font list, for proper sizing. */
	    XtSetArg (av [ac], XmNfontList, font_list); ac++;
	    XtSetArg (av [ac], XmNwidth,  size); ac++;
	    XtSetArg (av [ac], XmNheight, size); ac++;
	    XtSetArg (av [ac], XmNresize, False); ac++;
	    XtSetArg (av [ac], XmNspacing, 2); ac++;
	    XtSetArg (av [ac], XmNlabelString, xmstring); ac++;
	    XtSetArg (av [ac], XmNvisibleWhenOff, True); ac++;
	    XtSetArg (av [ac], XmNset, (reinitializing
					? data->toggled
					: data->default_toggle)); ac++;
	    XtSetArg (av [ac], XmNindicatorType,
		      (form->element_data->type == FORM_TYPE_RADIO
		       ? XmONE_OF_MANY : XmN_OF_MANY)); ac++;
	    /* These must always be the prevailing color of the text,
	       or they look silly. */
	    XtSetArg (av [ac], XmNforeground, fg); ac++;
	    XtSetArg (av [ac], XmNbackground, bg); ac++;
	    widget = XmCreateToggleButton (parent, "toggleForm", av, ac);
	    if (form->element_data->type == FORM_TYPE_RADIO)
	      XtAddCallback (widget, XmNvalueChangedCallback,
			     fe_radio_form_cb, fed);
	    else if (form->element_data->type == FORM_TYPE_CHECKBOX)
	      XtAddCallback (widget, XmNvalueChangedCallback,
			     fe_check_form_cb, fed);
	    data->FE_Data = fed;
	    XmStringFree (xmstring);
	    break;
	  }
	case FORM_TYPE_SELECT_ONE:
	  {
	    lo_FormElementSelectData *data = &form->element_data->ele_select;
	    Widget option_button, popup_menu;
	    Widget *kids;
	    char *selected_p;
	    int i, s;
	    Visual *v = 0;
	    Colormap cmap = 0;
	    Cardinal depth = 0;

	    Widget cur_parent, *cur_kids;
	    int height = 0, hoffset = 0, scrheight, cur_i = 0;

	    fe_getVisualOfContext (context, &v, &cmap, &depth);

	    ac = 0;
	    XtSetArg (av[ac], XmNvisual, v); ac++;
	    XtSetArg (av[ac], XmNcolormap, cmap); ac++;
	    XtSetArg (av[ac], XmNdepth, depth); ac++;
#ifdef FORMS_ARE_FONTIFIED
	    XtSetArg (av [ac], XmNfontList, font_list); ac++;
#endif
	    /* Always same color as OptionMenu (see below.) */
	    XtSetArg (av [ac], XmNforeground, fg); ac++;
	    XtSetArg (av [ac], XmNbackground, bg); ac++;
	    popup_menu = XmCreatePulldownMenu (parent, "menu", av, ac);
	    kids = (Widget *) malloc (data->option_cnt * sizeof (Widget));
	    cur_kids = (Widget *) malloc (data->option_cnt * sizeof (Widget));
	    selected_p = (char *) calloc (data->option_cnt, sizeof (char));

	    s = -1;
	    cur_parent = popup_menu;
	    scrheight = HeightOfScreen(XtScreen(popup_menu));
	    hoffset = 0;
	    cur_i = height = 0;
	    for (i = 0; i < data->option_cnt; i++, cur_i++)
	      {
		lo_FormElementOptionData *d2 =
		  & (((lo_FormElementOptionData *) data->options) [i]);
		XmString xmstring;

		xmstring = fe_ConvertToXmString (charset, (d2->text_value ?
			(char *) d2->text_value : "---\?\?\?---"));
		selected_p [i] = (reinitializing
				  ? !!d2->selected
				  : !!d2->def_selected);
		if (height + hoffset > scrheight) {
		  /* start cascading the option menu */
		  Widget new_popup_menu;
		  ac = 0;
		  XtSetArg (av[ac], XmNvisual, v); ac++;
		  XtSetArg (av[ac], XmNcolormap, cmap); ac++;
		  XtSetArg (av[ac], XmNdepth, depth); ac++;
#ifdef FORMS_ARE_FONTIFIED
		  XtSetArg (av [ac], XmNfontList, font_list); ac++;
#endif
		  /* Always same color as OptionMenu (see below.) */
		  XtSetArg (av [ac], XmNforeground, fg); ac++;
		  XtSetArg (av [ac], XmNbackground, bg); ac++;
		  new_popup_menu = XmCreatePulldownMenu (parent, "menu",
							 av, ac);
		  ac = 0;
		  XtSetArg (av [ac], XmNforeground, fg); ac++;
		  XtSetArg (av [ac], XmNbackground, bg); ac++;
		  XtSetArg (av [ac], XmNsubMenuId, new_popup_menu); ac++;
		  cur_kids[cur_i] =
		    XmCreateCascadeButtonGadget(cur_parent, "moreButton",
						av, ac);
		  cur_i++;
		  XtManageChildren(cur_kids, cur_i);

		  cur_parent = new_popup_menu;
		  cur_i = height = 0;
		}
		ac = 0;
		XtSetArg (av [ac], XmNfontList, font_list); ac++;
		XtSetArg (av [ac], XmNforeground, fg); ac++;
		XtSetArg (av [ac], XmNbackground, bg); ac++;
		XtSetArg (av [ac], XmNlabelString, xmstring); ac++;
		cur_kids[cur_i] = kids [i] =
		  XmCreatePushButtonGadget (cur_parent, "button", av, ac);
		XtAddCallback (kids [i], XmNactivateCallback,
			       fe_popup_form_cb, fed);
		if (selected_p [i])
		  {
		    s = i;
		    XtVaSetValues (popup_menu, XmNmenuHistory, kids [i], 0);
		  }
		XmStringFree (xmstring);
		height += kids[i]->core.height;
		/* hoffset decides when cascading of optionmenu starts. We
		 * set it to 3 pushbutton heights. This should atleast be 1
		 * cascadebuttongadget high to accomodate the more... cascade
		 */
		if (!hoffset) hoffset = kids[i]->core.height * 3;
	      }
	    XtManageChildren (cur_kids, cur_i);

	    ac = 0;
#ifdef FORMS_ARE_FONTIFIED
	    XtSetArg (av [ac], XmNfontList, font_list); ac++;
#endif
	    /* These must always be the prevailing color of the text,
	       or they look silly. */
	    XtSetArg (av [ac], XmNforeground, fg); ac++;
	    XtSetArg (av [ac], XmNbackground, bg); ac++;
	    XtSetArg (av [ac], XmNsubMenuId, popup_menu); ac++;
	    option_button = fe_CreateOptionMenu (parent, "button", av, ac);

	    /*
	     * I don't know why, but if we don't also set menuHistory
	     * on this widget the HP screws up.
	     */
	    if (s != -1)
	      {
		XtVaSetValues (option_button, XmNmenuHistory, kids [s], 0);
	      }

	    /*
	     * Because Motif is scum, it checks all button presses in the
	     * rowColumn to make sure they are inside the rowColumn
	     * before popping up the menu.  Unfortunately, they compare
	     * Widget coordinates to window coordinates.  This is bad
	     * because of the way we scroll windows not widgets.
	     * Thus we add an event handler here to catch all button presses
	     * and set the widget coords to the window coords at that
	     * moment so Motif will pop up our menu.
	     */
	    XtAddEventHandler(option_button, ButtonPressMask, FALSE,
			      (XtEventHandler)option_press, NULL);
	    XP_FREE(cur_kids);

	    fed->popup_widget = popup_menu;
	    fed->nkids = data->option_cnt;
	    fed->popup_kids = kids;
	    fed->selected_p = selected_p;
	    data->FE_Data = fed;
	    widget = option_button;
	    break;
	  }
	case FORM_TYPE_SELECT_MULT:
	  {
	    lo_FormElementSelectData *data = &form->element_data->ele_select;
	    int nitems = data->option_cnt;
	    int vlines = (data->size > 0 ? data->size : nitems);
	    XmString *items;
	    char *selected_p;

	    int i;

	    if (nitems > 0) {
	    	items = (XmString *) malloc (sizeof (XmString) * nitems);
	    	selected_p = (char *) calloc (nitems, sizeof (char));
	    }

	    for (i = 0; i < nitems; i++)
	      {
		lo_FormElementOptionData *d2 =
		  & (((lo_FormElementOptionData *) data->options) [i]);
		items [i] = fe_ConvertToXmString (charset,
					    (d2->text_value
					     ? ((*((char *) d2->text_value))
						? ((char *) d2->text_value)
						: " ")
					     : "---\?\?\?---"));
		selected_p [i] = (reinitializing
				  ? !!d2->selected
				  : !!d2->def_selected);
	      }
	    ac = 0;
/* #ifdef FORMS_ARE_FONTIFIED */
	    XtSetArg (av [ac], XmNfontList, font_list); ac++;
/* #endif */
#ifdef FORMS_ARE_COLORED
	    XtSetArg (av [ac], XmNforeground, fg); ac++;
	    XtSetArg (av [ac], XmNbackground, bg); ac++;
#endif
	    if (nitems) {
		XtSetArg (av [ac], XmNitems, items); ac++;
		XtSetArg (av [ac], XmNitemCount, nitems); ac++;
	    }
	    XtSetArg (av [ac], XmNvisibleItemCount, vlines); ac++;
	    XtSetArg (av [ac], XmNspacing, 0); ac++;
	    XtSetArg (av [ac], XmNlistSizePolicy, XmRESIZE_IF_POSSIBLE); ac++;
	    XtSetArg (av [ac], XmNselectionPolicy, (data->multiple
						    ? XmMULTIPLE_SELECT
						    : XmSINGLE_SELECT)); ac++;
	    XtSetArg(av[ac], XmNx, -15000); ac++;
	    widget = XmCreateScrolledList (parent, "list", av, ac);
	    /*
	     * We need this Unmanage because otherwise XtIsManaged on the
	     * parent fails later.  It seems XmCreateScrolledList() creates
	     * an unmanaged list with a managed scrolled window parent.
	     * How stupid.
	     */
	    XtUnmanageChild (XtParent (widget));
	    XtManageChild (widget);

	    for (i = 0; i < nitems; i++)
	      if (selected_p[i]) XmListSelectPos(widget, i+1, False);

	    for (i = 0; i < nitems; i++)
	      XmStringFree (items [i]);
	    if (nitems) {
		free (items);
	    }
	    
	    XtAddCallback (widget, XmNdefaultActionCallback,
			   fe_list_form_cb, fed);
	    XtAddCallback (widget, XmNsingleSelectionCallback,
			   fe_list_form_cb, fed);
	    XtAddCallback (widget, XmNmultipleSelectionCallback,
			   fe_list_form_cb, fed);
	    XtAddCallback (widget, XmNextendedSelectionCallback,
			   fe_list_form_cb, fed);
	    XtInsertEventHandler(widget, KeyPressMask, False,
		       fe_list_find_eh, context, XtListHead);

	    fed->nkids = nitems;
	    fed->popup_kids = 0;
	    fed->selected_p = selected_p;
	    data->FE_Data = fed;
	    break;
	  }
	case FORM_TYPE_TEXTAREA:
	  {
	    lo_FormElementTextareaData *data =
	      &form->element_data->ele_textarea;
	    char *text = (reinitializing
			  ? (char *) data->current_text
			  : (char *) data->default_text);
	    unsigned char *loc;
	    if (!text) text = (char *) data->default_text;
	    if (!text) text = "";
	    fe_forms_clean_text (context, charset, text, False);
	    loc = fe_ConvertToLocaleEncoding (charset, text);
	    ac = 0;

	    XtSetArg (av[ac], XmNscrollingPolicy, XmAUTOMATIC); ac++;
	    XtSetArg (av[ac], XmNvisualPolicy, XmCONSTANT); ac++;
	    XtSetArg (av[ac], XmNscrollBarDisplayPolicy, XmSTATIC); ac++;

	    XtSetArg (av[ac], XmNvalue, loc); ac++;
	    XtSetArg (av[ac], XmNcursorPosition, 0); ac++;
	    XtSetArg (av[ac], XmNcolumns, ((fe_LocaleCharSetID & MULTIBYTE) ?
			(data->cols + 1) / 2 : data->cols)); ac++;
	    XtSetArg (av[ac], XmNrows, data->rows); ac++;
	    /*
	     * Gotta love Motif.  No matter what wordWrap is set to, if
	     * there is a horizontal scrollbar present it won't wrap.
	     * Of course the documentation mentions this nowhere.
	     * "Use the source Luke!"
	     */
	    if (data->auto_wrap != TEXTAREA_WRAP_OFF)
	      {
	    XtSetArg (av[ac], XmNscrollHorizontal, FALSE); ac++;
	    XtSetArg (av[ac], XmNwordWrap, TRUE); ac++;
	      }
	    XtSetArg (av[ac], XmNeditMode, XmMULTI_LINE_EDIT); ac++;
	    /* Ok, let's try using the fixed font for text areas only. */
/*#ifdef FORMS_ARE_FONTIFIED*/
	    XtSetArg (av[ac], XmNfontList, font_list); ac++;
/*#endif*/
#ifdef FORMS_ARE_COLORED
	    XtSetArg (av [ac], XmNforeground, fg); ac++;
	    XtSetArg (av [ac], XmNbackground, bg); ac++;
#endif
	    XtSetArg(av[ac], XmNx, -15000); ac++;
	    widget = XmCreateScrolledText (parent, "formText", av, ac);
	    if (((char *) loc) != text)
	      {
		XP_FREE (loc);
	      }
#ifdef MOCHA
            XtAddCallback(widget, XmNlosingFocusCallback,
                                        fe_lost_focus_cb, fed);
	    XtAddCallback(widget, XmNfocusCallback, fe_got_focus_cb, fed);
#endif /* MOCHA */

	    /* The scroller must have the prevailing color of the text,
	       or it looks funny.*/
	    XtVaSetValues (XtParent (widget),
			   XmNforeground, fg, XmNbackground, bg, 0);

	    XtUnmanageChild (XtParent (widget));
	    XtManageChild (widget);

	    data->FE_Data = fed;
	    break;
	  }
	case FORM_TYPE_READONLY:
	  {
	    lo_FormElementMinimalData *data = &form->element_data->ele_minimal;
	    char *text = (char *) data->value;
	    int size = XP_STRLEN (text);
	    int columns = ((fe_LocaleCharSetID & MULTIBYTE) ? (size + 1) / 2 :
		size);
	    unsigned char *loc;

	    if (! text) text = "";
	    fe_forms_clean_text (context, charset, text, True);
	    if (columns == 0) columns = 1;
	    ac = 0;
	    /* Ok, let's try using the fixed font for text areas only. */
/*#ifdef FORMS_ARE_FONTIFIED*/
	    XtSetArg (av [ac], XmNfontList, font_list); ac++;
/*#endif*/
	    XtSetArg (av [ac], XmNcolumns, columns); ac++;
	    /* The field must have the prevailing color of the text,
	       or it looks confusing. */
	    XtSetArg (av [ac], XmNforeground, fg); ac++;
	    XtSetArg (av [ac], XmNbackground, bg); ac++;
	    XtSetArg (av [ac], XmNeditable, False); ac++;
	    XtSetArg (av [ac], XmNcursorPositionVisible, False); ac++;
	    widget = XmCreateTextField (parent, "textForm", av, ac);
#ifdef MOCHA
            XtAddCallback(widget, XmNlosingFocusCallback,
                                        fe_lost_focus_cb, fed);
	    XtAddCallback(widget, XmNfocusCallback, fe_got_focus_cb, fed);
#endif /* MOCHA */
	    data->FE_Data = fed;

	    /* This is changed later for self-submitting fields. */
	    if (fe_globalData.nonterminal_text_translations)
	      XtOverrideTranslations (widget, fe_globalData.
				      nonterminal_text_translations);

	    XtVaSetValues (widget, XmNcursorPosition, 0, 0);
	    loc = fe_ConvertToLocaleEncoding (charset, text);
	    /*
	     * I don't know whether it is SGI or some version of Motif
	     * that is fucked, but doing a XtVaSetValues() here will
	     * execute the valueChangedCallback but ignore its changes.
	     * XmTextFieldSetString() works as expected.
	     */
	    XmTextFieldSetString (widget, loc);
	    if (((char *) loc) != text)
	      {
		XP_FREE (loc);
	      }
	    break;
	  }
	case FORM_TYPE_HIDDEN:
	case FORM_TYPE_JOT:
	case FORM_TYPE_ISINDEX:
	case FORM_TYPE_IMAGE:
	case FORM_TYPE_KEYGEN:
	default:
	  assert (0);
	  return;
	}
      fed->widget = widget;
      fed->context = context;
    }

  font_list = 0;
  widget = fed->widget;

  if (form->element_data->type == FORM_TYPE_SELECT_ONE)
    XtVaGetValues (XmOptionButtonGadget (widget), XmNfontList, &font_list, 0);
  else
    XtVaGetValues (widget, XmNfontList, &font_list, 0);

  fed->form = form;	/* might be different this time around. */

  {
    Widget widget_to_ream = widget;
    Dimension width = 0;
    Dimension height = 0;
    Dimension bw = 0;
    Dimension st = 0;
    Dimension ht = 0;
    Dimension margin_height = 0;
    Dimension bottom = 0;
    int descent;
    int ascent;

    if (form->element_data->type == FORM_TYPE_SELECT_MULT ||
	form->element_data->type == FORM_TYPE_TEXTAREA)
      widget_to_ream = XtParent (widget);	/* fuck! */

    fe_HackTranslations (context, widget_to_ream);

    fe_font_list_metrics (context, form, font_list, &ascent, &descent);
    XtRealizeWidget (widget_to_ream); /* create window, but don't map */

    if (fe_globalData.fe_guffaw_scroll == 1)
    {
      XSetWindowAttributes attr;
      unsigned long valuemask;

      valuemask = CWBitGravity | CWWinGravity;
      attr.win_gravity = StaticGravity;
      attr.bit_gravity = StaticGravity;
      XChangeWindowAttributes (XtDisplay (widget), XtWindow (widget_to_ream),
			       valuemask, &attr);
    }

    XtVaGetValues (widget_to_ream,
		   XmNwidth,		  &width,
		   XmNheight,		  &height,
		   XmNborderWidth,	  &bw,
		   XmNshadowThickness,	  &st,
		   XmNhighlightThickness, &ht,
		   XmNmarginHeight,	  &margin_height,
		   XmNmarginBottom,	  &bottom,
		   0);

    if (form->element_data->type == FORM_TYPE_SELECT_ONE) /* you suck! */
      {
	Dimension bw2 = 0;
	Dimension st2 = 0;
	Dimension ht2 = 0;
	Dimension mh2 = 0;
	Dimension bt2 = 0;
	XtVaGetValues (XmOptionButtonGadget (widget),
		       XmNborderWidth,		&bw2,
		       XmNshadowThickness,	&st2,
		       XmNhighlightThickness,	&ht2,
		       XmNmarginHeight,		&mh2,
		       XmNmarginBottom,		&bt2,
		       0);
	bw += bw2;
	st += st2;
	ht += ht2;
	margin_height += mh2;
	bottom += bt2;
      }

    width  += fe_FORM_LEFT_MARGIN + fe_FORM_RIGHT_MARGIN;
    height += fe_FORM_TOP_MARGIN + fe_FORM_BOTTOM_MARGIN;

    /* The WxH doesn't take into account the border width; so add that in
       for layout's benefit.  We subtract it again later. */
    width  += (bw * 2);
    height += (bw * 2);
    form->width  = width;
    form->height = height;

    /* Yeah.  Uh huh. */
    if (form->element_data->type == FORM_TYPE_RADIO ||
	form->element_data->type == FORM_TYPE_CHECKBOX)
      {
	bottom = 0;
	descent = bw;
      }

    form->baseline = height - (bottom + margin_height + st + bw + ht + descent
			       + fe_FORM_BOTTOM_MARGIN);
  }
}


void
XFE_FreeFormElement (MWContext *context, LO_FormElementData *data)
{
  struct fe_form_data *fed = 0;
  switch (data->type)
    {
    case FORM_TYPE_FILE:
    case FORM_TYPE_TEXT:
    case FORM_TYPE_PASSWORD:
      fed = data->ele_text.FE_Data;
      data->ele_text.FE_Data = 0;
      break;
    case FORM_TYPE_SUBMIT:
    case FORM_TYPE_RESET:
    case FORM_TYPE_BUTTON:
    case FORM_TYPE_READONLY:
      fed = data->ele_minimal.FE_Data;
      data->ele_minimal.FE_Data = 0;
      break;
    case FORM_TYPE_RADIO:
    case FORM_TYPE_CHECKBOX:
      fed = data->ele_toggle.FE_Data;
      data->ele_toggle.FE_Data = 0;
      break;
    case FORM_TYPE_SELECT_ONE:
    case FORM_TYPE_SELECT_MULT:
      fed = data->ele_select.FE_Data;
      data->ele_select.FE_Data = 0;
      break;
    case FORM_TYPE_TEXTAREA:
      fed = data->ele_textarea.FE_Data;
      data->ele_textarea.FE_Data = 0;
      break;
    case FORM_TYPE_HIDDEN:
    case FORM_TYPE_JOT:
    case FORM_TYPE_ISINDEX:
    case FORM_TYPE_IMAGE:
    case FORM_TYPE_KEYGEN:
      break;
    default:
      assert (0);
      return;
    }
  if (!fed)
    return;
  if (fed->widget)
    XtDestroyWidget (fed->widget);
  if (fed->popup_widget)
    XtDestroyWidget (fed->popup_widget);
  if (fed->popup_kids)
    free (fed->popup_kids);
  free (fed);
}


void
XFE_FormTextIsSubmit (MWContext *context, LO_FormElementStruct *form)
{
  struct fe_form_data *fed = 0;
  char *cb_name = 0;
  switch (form->element_data->type)
    {
    case FORM_TYPE_FILE:
    case FORM_TYPE_TEXT:
    case FORM_TYPE_PASSWORD:
      {
	lo_FormElementTextData *data = &form->element_data->ele_text;
	fed = data->FE_Data;
	cb_name = XmNactivateCallback;
	break;
      }
    case FORM_TYPE_TEXTAREA:
      {
	lo_FormElementTextareaData *data = &form->element_data->ele_textarea;
	fed = data->FE_Data;
	cb_name = XmNactivateCallback;
	break;
      }
    default:
      break;
    }
  if (! fed) return;
  XtRemoveCallback (fed->widget, cb_name, fe_activate_submit_cb, fed);
  XtAddCallback (fed->widget, cb_name, fe_activate_submit_cb, fed);
  if (fe_globalData.terminal_text_translations)
    XtOverrideTranslations (fed->widget,
			    fe_globalData.terminal_text_translations);
}


void
XFE_DisplayFormElement (MWContext *context, int iLocation,
		       LO_FormElementStruct *form)
{
  struct fe_form_data *fed = 0;
  lo_FormElementTextData *text_data = 0;
  Widget widget, widget_to_manage, widget_to_ream;
  Arg av [20];
  int ac = 0;

  if ((form->x > (CONTEXT_DATA (context)->document_x +
		  CONTEXT_DATA (context)->scrolled_width)) ||
      (form->y > (CONTEXT_DATA (context)->document_y +
		  CONTEXT_DATA (context)->scrolled_height)) ||
      ((form->y + form->line_height) < CONTEXT_DATA (context)->document_y) ||
      ((form->x + form->x_offset + form->width) <
       CONTEXT_DATA (context)->document_x))
    {
      XFE_GetFormElementValue (context, form, FALSE);
      return;
    }

  if (form->element_data == NULL)
    return;
  switch (form->element_data->type)
    {
    case FORM_TYPE_FILE:
    case FORM_TYPE_TEXT:
    case FORM_TYPE_PASSWORD:
      fed = form->element_data->ele_text.FE_Data;
      text_data = &form->element_data->ele_text;
      break;
    case FORM_TYPE_SUBMIT:
    case FORM_TYPE_RESET:
    case FORM_TYPE_BUTTON:
    case FORM_TYPE_READONLY:
      fed = form->element_data->ele_minimal.FE_Data;
      break;
    case FORM_TYPE_RADIO:
    case FORM_TYPE_CHECKBOX:
      fed = form->element_data->ele_toggle.FE_Data;
      break;
    case FORM_TYPE_SELECT_ONE:
    case FORM_TYPE_SELECT_MULT:
      fed = form->element_data->ele_select.FE_Data;
      break;
    case FORM_TYPE_TEXTAREA:
      fed = form->element_data->ele_textarea.FE_Data;
      break;
    default:
      assert (0);
      return;
    }
  assert (fed);
  if (! fed) return;
  if (!fed->widget) return;

  widget = fed->widget;
  widget_to_manage = widget;
  widget_to_ream = widget;

  if (form->element_data->type == FORM_TYPE_SELECT_MULT ||
      form->element_data->type == FORM_TYPE_TEXTAREA)
    {
      /* There's an XmScrolledWindow around this fucker...
	 Gee, it's not like Motif makes you JUMP THROUGH HOOPS
	 and MAKE ALL KINDS OF SPECIAL CASES or anything. */
      widget_to_manage = widget;
      widget_to_ream = XtParent (widget);
    }

  if (!XtIsManaged (widget_to_manage) ||
      !XtIsManaged (widget_to_ream))
    {
      Dimension bw = 0;
      Dimension width = form->width;
      Dimension height = form->height;
      Position x = (form->x + form->x_offset
		    - CONTEXT_DATA (context)->document_x);
      Position y = (form->y + form->y_offset
		    - CONTEXT_DATA (context)->document_y);
      ac = 0;

    if (form->element_data->type == FORM_TYPE_SELECT_ONE)
      {
	Widget label  = XmOptionLabelGadget (widget_to_ream);
	Widget button = XmOptionButtonGadget (widget_to_ream);

	/* On SGI, this label won't go the fuck away - 4 pixels wide.
	   So unmanage it. */
	if (label)
	  XtUnmanageChild (label);
	
	/* On SGI, the button doesn't pick up the colors that we gave to
	   it and to the menu - so force the issue again. */
	if (button)
	  {
	    Pixel fg = 0, bg = 0;
	    XtVaGetValues (widget_to_ream,
			   XmNforeground, &fg, XmNbackground, &bg, 0);
	    XtVaSetValues (button,
			   XmNforeground, fg, XmNbackground, bg, 0);
	  }
      }

      XtVaGetValues (widget_to_ream, XmNborderWidth, &bw, 0);
      /* Since I told layout that width was width+bw+bw (for its sizing and
	 positioning purposes), subtract it out again when setting the size of
	 the text area.  X thinks the borders are outside the WxH rectangle,
	 not inside, but thinks that the borders are below and to the right of
	 the X,Y origin, instead of up and to the left as you would expect
	 given the WxH behavior!!
       */
      width -= (bw * 2);
      height -= (bw * 2);
      /* x += bw; */
      /* y += bw;*/

      x += fe_FORM_LEFT_MARGIN;
      y += fe_FORM_TOP_MARGIN;
      width  -= fe_FORM_LEFT_MARGIN + fe_FORM_RIGHT_MARGIN;
      height -= fe_FORM_TOP_MARGIN + fe_FORM_BOTTOM_MARGIN;

      /*
       * Move to 0 before moving to where we want so it ends up
       * where we want.
       */
      ac = 0;
      XtSetArg (av [ac], XmNx, 0); ac++;
      XtSetArg (av [ac], XmNy, 0); ac++;
      XtSetValues (widget_to_ream, av, ac);

      ac = 0;
#if 0
      XtSetArg (av [ac], XmNx, (Position)x); ac++;
      XtSetArg (av [ac], XmNy, (Position)y); ac++;
#endif
      if (text_data && text_data->max_size > 0)
	XtSetArg (av [ac], XmNmaxLength, text_data->max_size), ac++;
      if (ac)
	XtSetValues (widget_to_manage, av, ac);

      ac = 0;
      XtSetArg (av [ac], XmNx, (Position)x); ac++;
      XtSetArg (av [ac], XmNy, (Position)y); ac++;
      XtSetArg (av [ac], XmNwidth, (Dimension)width); ac++;
      XtSetArg (av [ac], XmNheight, (Dimension)height); ac++;
      XtSetValues (widget_to_ream, av, ac);

      XtManageChild (widget_to_ream);
/*
      XtManageChild (widget_to_manage);
*/
    }
  XFE_GetFormElementValue (context, form, FALSE);
}

void
XFE_GetFormElementValue (MWContext *context, LO_FormElementStruct *form,
			XP_Bool delete_p)
{
  struct fe_form_data *fed = 0;

  if (form->element_data == NULL)
    return;
  switch (form->element_data->type)
    {
    case FORM_TYPE_FILE:
    case FORM_TYPE_TEXT:
    case FORM_TYPE_PASSWORD:
      {
	lo_FormElementTextData *data = &form->element_data->ele_text;
	char *text = 0;
	fed = data->FE_Data;
	if (form->element_data->type == FORM_TYPE_TEXT)
	  XtVaGetValues (fed->widget, XmNvalue, &text, 0);
	else if (form->element_data->type == FORM_TYPE_PASSWORD)
	  text = fe_GetPasswdText (fed->widget);
	else /* FORM_TYPE_FILE */
	  XtVaGetValues (fed->file_widget, XmNvalue, &text, 0);
	if (data->current_text && data->current_text != data->default_text)
	  free (data->current_text);
	data->current_text = (XP_Block)
		fe_ConvertFromLocaleEncoding (context->win_csid,
			(unsigned char *) text);
	if (((char *) data->current_text) != text) {
	  free (text);
	}
	break;
      }
    case FORM_TYPE_SUBMIT:
    case FORM_TYPE_RESET:
    case FORM_TYPE_BUTTON:
    case FORM_TYPE_READONLY:
      {
	fed = form->element_data->ele_minimal.FE_Data;
	break;
      }
    case FORM_TYPE_RADIO:
    case FORM_TYPE_CHECKBOX:
      {
	lo_FormElementToggleData *data = &form->element_data->ele_toggle;
	Boolean set = False;
	fed = data->FE_Data;
	XtVaGetValues (fed->widget, XmNset, &set, 0);
	data->toggled = set;
	break;
      }
    case FORM_TYPE_SELECT_ONE:
    case FORM_TYPE_SELECT_MULT:
      {
	lo_FormElementSelectData *data = &form->element_data->ele_select;
	lo_FormElementOptionData *d2 =
	  (lo_FormElementOptionData *) data->options;
	int i;
	fed = data->FE_Data;
	for (i = 0; i < fed->nkids; i++)
	  d2 [i].selected = fed->selected_p [i];
	break;
      }
    case FORM_TYPE_TEXTAREA:
      {
	lo_FormElementTextareaData *data = &form->element_data->ele_textarea;
	char *text = 0;
	fed = data->FE_Data;
	XtVaGetValues (fed->widget, XmNvalue, &text, 0);
	if (! text) break;
	if (data->auto_wrap == TEXTAREA_WRAP_HARD) {
	  char *tmp = XP_WordWrap(fe_LocaleCharSetID, text, data->cols, 0);
	  if (text) XtFree(text);
	  if (!tmp) break;
	  text = tmp;
	}
	if (data->current_text && data->current_text != data->default_text)
	  free (data->current_text);
	data->current_text = (XP_Block)
		fe_ConvertFromLocaleEncoding (context->win_csid,
			(unsigned char *) text);
	if (((char *) data->current_text) != text) {
	  free (text);
	}
	break;
      }
    case FORM_TYPE_HIDDEN:
    case FORM_TYPE_JOT:
    case FORM_TYPE_ISINDEX:
    case FORM_TYPE_IMAGE:
    case FORM_TYPE_KEYGEN:
      {
	fed = 0;
	break;
      }
    default:
      assert (0);
      return;
    }

  if (delete_p && fed)
    {
      XtDestroyWidget (fed->widget);
      fed->widget = 0;
      if (fed->popup_widget)
	{
	  XtDestroyWidget (fed->popup_widget);
	  fed->popup_widget = 0;
	}
    }
}

void
XFE_ResetFormElement (MWContext *context, LO_FormElementStruct *form)
{
  int16 charset;

  if (form->element_data == NULL)
    return;
  charset = form->text_attr->charset;

  switch (form->element_data->type)
    {
    case FORM_TYPE_FILE:
    case FORM_TYPE_TEXT:
    case FORM_TYPE_PASSWORD:
      {
	lo_FormElementTextData *data = &form->element_data->ele_text;
	struct fe_form_data *fed = data->FE_Data;
	char *text = (data->default_text ? (char *) data->default_text : "");
	char *tmp_text = 0;
	Widget w = fed->widget;
	unsigned char *loc;
	if (data->max_size > 0 && (int) XP_STRLEN (text) >= data->max_size) {
	  tmp_text = XP_STRDUP (text);
	  tmp_text [data->max_size] = '\0';
	  text = tmp_text;
	}
	fe_forms_clean_text (context, charset, text, True);
	if (form->element_data->type == FORM_TYPE_FILE)
	    w = fed->file_widget;
	XtVaSetValues (w, XmNcursorPosition, 0, 0);
	loc = fe_ConvertToLocaleEncoding (charset, text);
	/*
	 * I don't know whether it is SGI or some version of Motif
	 * that is fucked, but doing a XtVaSetValues() here will
	 * execute the valueChangedCallback but ignore its changes.
	 * XmTextFieldSetString() works as expected.
	 */
	XmTextFieldSetString (w, loc);
	if (((char *) loc) != text)
	  {
	    XP_FREE (loc);
	  }
	if (tmp_text) XP_FREE (tmp_text);
	break;
      }
    case FORM_TYPE_TEXTAREA:
      {
	lo_FormElementTextareaData *data = &form->element_data->ele_textarea;
	struct fe_form_data *fed = data->FE_Data;
	char *text = (data->default_text ? (char *) data->default_text : "");
	unsigned char *loc;
	fe_forms_clean_text (context, charset, text, False);
	XtVaSetValues (fed->widget, XmNcursorPosition, 0, 0);
	loc = fe_ConvertToLocaleEncoding (charset, text);
	XtVaSetValues (fed->widget, XmNvalue, loc, 0);
	if (((char *) loc) != text)
	  {
	    XP_FREE (loc);
	  }
	break;
      }
    case FORM_TYPE_RADIO:
    case FORM_TYPE_CHECKBOX:
      {
	lo_FormElementToggleData *data = &form->element_data->ele_toggle;
	struct fe_form_data *fed = data->FE_Data;
	XtVaSetValues (fed->widget, XmNset, data->default_toggle, 0);
	break;
      }
    case FORM_TYPE_SELECT_ONE:
      {
	lo_FormElementSelectData *data = &form->element_data->ele_select;
	lo_FormElementOptionData *d2 =
	  (lo_FormElementOptionData *) data->options;
	struct fe_form_data *fed = data->FE_Data;
	int i, s = 0;
	for (i = 0; i < fed->nkids; i++)
	  {
	    d2[i].selected = d2[i].def_selected;
	    fed->selected_p [i] = d2[i].selected;
	    if (d2[i].selected) s = i;
	  }
	XtVaSetValues (fed->popup_widget,
		       XmNmenuHistory, fed->popup_kids [s], 0);
	XtVaSetValues (fed->widget,
		       XmNmenuHistory, fed->popup_kids [s], 0);
	break;
      }
    case FORM_TYPE_SELECT_MULT:
      {
	lo_FormElementSelectData *data = &form->element_data->ele_select;
	lo_FormElementOptionData *d2 = 
	  (lo_FormElementOptionData *) data->options;
	struct fe_form_data *fed = data->FE_Data;
	int nitems = data->option_cnt;
	int i;
	for (i = 0; i < nitems; i++)
	  {
	    d2[i].selected = d2[i].def_selected;
	    if (d2[i].selected) {
	      /* Highlight the item at pos i+1 if it is not already */
	      if (!fed->selected_p[i])
		XmListSelectPos(fed->widget, i+1, False);
	    }
	    else
	      XmListDeselectPos(fed->widget, i+1);
	    fed->selected_p [i] = d2[i].selected;
	  }
	break;
      }
    default:
      assert (0);
      return;
    }
}

void
XFE_SetFormElementToggle (MWContext *context, LO_FormElementStruct *form,
			 XP_Bool state)
{
  if (form->element_data == NULL)
    return;
  switch (form->element_data->type)
    {
    case FORM_TYPE_RADIO:
    case FORM_TYPE_CHECKBOX:
      {
	lo_FormElementToggleData *data = &form->element_data->ele_toggle;
	struct fe_form_data *fed = data->FE_Data;
	XtVaSetValues (fed->widget, XmNset, state, 0);
	break;
      }
    default:
      assert (0);
      return;
    }
}


static void
fe_got_focus_cb (Widget widget, XtPointer closure, XtPointer call_data)
{
#ifdef MOCHA
  struct fe_form_data *fed = (struct fe_form_data *) closure;

  TRACEMSG(("fe_got_focus_c:\n"));

  CALLING_MOCHA(fed->context);
  LM_SendOnFocus (fed->context, (LO_Element *) fed->form);
  if (IS_FORM_DESTROYED(fed->context)) return;
#endif /* MOCHA */
}

static void
fe_lost_focus_cb (Widget widget, XtPointer closure, XtPointer call_data)
{
#ifdef MOCHA
  struct fe_form_data *fed = (struct fe_form_data *) closure;
  lo_FormElementTextData *data;
  char *text;
  int tellmocha = False;

  data = (lo_FormElementTextData *) &fed->form->element_data->ele_text;

  XtVaGetValues (widget, XmNvalue, &text, 0);

  if (data->type == FORM_TYPE_READONLY) return;

  if (!data->current_text) {
    data->current_text = (PA_Block) XP_STRDUP (text);
    tellmocha = True;
  } else if (XP_STRCMP ((char *) data->current_text, text)) {
    XP_FREE (data->current_text);
    data->current_text = (PA_Block) XP_STRDUP (text);
    tellmocha = True;
  }

  if (text) XP_FREE (text);

  TRACEMSG(("fe_lost_focus_cb: %s\n", data->current_text));

  if (tellmocha) {
    CALLING_MOCHA(fed->context);
    LM_SendOnChange (fed->context, (LO_Element *) fed->form);
    if (IS_FORM_DESTROYED(fed->context)) return;
  }

  CALLING_MOCHA(fed->context);
  LM_SendOnBlur (fed->context, (LO_Element *) fed->form);
  if (IS_FORM_DESTROYED(fed->context)) return;
#endif /* MOCHA */
}

static void
fe_form_file_browse_cb (Widget widget, XtPointer closure, XtPointer call_data)
{
  struct fe_form_data *fed = (struct fe_form_data *) closure;
  char *filename;
  XmString xm_title = 0;
  char *title = 0;

  XtVaGetValues (widget, XmNlabelString, &xm_title, 0);
  if (xm_title)
    XmStringGetLtoR (xm_title, XmFONTLIST_DEFAULT_TAG, &title);
  XmStringFree(xm_title);

  filename = fe_ReadFileName( fed->context, title, 0, FALSE, 0);
  XP_FREE(title);

  if (filename) {
    XP_StatStruct st;
    char buf [2048];
    if (stat(filename, &st) < 0) {
	  /* Error: Cant stat */
	  PR_snprintf (buf, sizeof (buf),
			XP_GetString(XFE_ERROR_OPENING_FILE), filename);
	  FE_Alert(fed->context, buf);
    }
    else {
	  /* The file was found. Stick the name into the text field. */
	  XmTextFieldSetString (fed->file_widget, filename);
    }
    XP_FREE(filename);
  }
}

/* This happens for self submiting forms. */
static void
fe_activate_submit_cb (Widget widget, XtPointer closure, XtPointer call_data)
{
  struct fe_form_data *fed = (struct fe_form_data *) closure;

  /* Move focus out of the current focus widget. This will make any OnChange
   * calls to mocha.
   */
  fe_NeutralizeFocus(fed->context);

  fe_submit_form_cb(widget, closure, call_data);
}

static void
fe_submit_form_cb (Widget widget, XtPointer closure, XtPointer call_data)
{
  struct fe_form_data *fed = (struct fe_form_data *) closure;
  LO_FormSubmitData *data;
  URL_Struct *url;

#ifdef MOCHA

  /* Lord Whorfin says: send me a click, dammit! */
  CALLING_MOCHA(fed->context);
  if (!LM_SendOnClick (fed->context, (LO_Element *) fed->form))
    return;
  if (IS_FORM_DESTROYED(fed->context)) return;
#endif /* MOCHA */

  /* Load the element value before telling mocha about the submit.
   * This needs to be done if we came here not because of the user
   * clicking on the submit button, like hitting RETURN on a text field.
   */
  XFE_GetFormElementValue(fed->context, fed->form, False);

#ifdef MOCHA
  /* We are about to submit, so tell libmocha */
  CALLING_MOCHA(fed->context);
  if (!LM_SendOnSubmit (fed->context, (LO_Element *) fed->form))
    return;
  if (IS_FORM_DESTROYED(fed->context)) return;
#endif /* MOCHA */

  data = LO_SubmitForm (fed->context, fed->form);
  if (! data) return;
  url = NET_CreateURLStruct ((char *) data->action, FALSE);

  /* Add the referer to the URL. */
  {
    History_entry *he = SHIST_GetCurrent (&fed->context->hist);
    if (url->referer)
      free (url->referer);
    if (he && he->address)
      url->referer = strdup (he->address);
    else
      url->referer = 0;
  }

  NET_AddLOSubmitDataToURLStruct (data, url);

  fe_GetURL (fed->context, url, FALSE);
  LO_FreeSubmitData (data);
}


static void
fe_button_form_cb (Widget widget, XtPointer closure, XtPointer call_data)
{
  struct fe_form_data *fed = (struct fe_form_data *) closure;

#ifdef MOCHA
  /* We are about to submit, so tell libmocha */
  CALLING_MOCHA(fed->context);
  if (!LM_SendOnClick (fed->context, (LO_Element *) fed->form))
    return;
  if (IS_FORM_DESTROYED(fed->context)) return;
#endif /* MOCHA */

  XFE_GetFormElementValue (fed->context, fed->form, False);
}

static void
fe_reset_form_cb (Widget widget, XtPointer closure, XtPointer call_data)
{
  struct fe_form_data *fed = (struct fe_form_data *) closure;

#ifdef MOCHA
  CALLING_MOCHA(fed->context);
  if (!LM_SendOnClick (fed->context, (LO_Element *) fed->form))
    return;
  if (IS_FORM_DESTROYED(fed->context)) return;
#endif /* MOCHA */

  LO_ResetForm (fed->context, fed->form);
}


static void
fe_radio_form_cb (Widget widget, XtPointer closure, XtPointer call_data)
{
  struct fe_form_data *fed = (struct fe_form_data *) closure;
  XmToggleButtonCallbackStruct *cb = (XmToggleButtonCallbackStruct *)call_data;
  LO_FormElementStruct *save;

  if (cb->set)
    save = LO_FormRadioSet (fed->context, fed->form);
  else
    /* Don't allow the user to ever toggle a button off - exactly one
       must be selected at all times. */
    XtVaSetValues (widget, XmNset, True, 0);

  XFE_GetFormElementValue (fed->context, fed->form, False);

#ifdef MOCHA
  CALLING_MOCHA(fed->context);
  if (!LM_SendOnClick (fed->context, (LO_Element *) fed->form))
    {
      if (IS_FORM_DESTROYED(fed->context)) return;
      if (cb->set && save != fed->form)
	{
	  XFE_SetFormElementToggle (fed->context, save, TRUE);
	  LO_FormRadioSet (fed->context, save);
	}

      XFE_GetFormElementValue (fed->context, fed->form, False);
    }
  else
    if (IS_FORM_DESTROYED(fed->context)) return;
#endif /* MOCHA */
}

static void
fe_check_form_cb (Widget widget, XtPointer closure, XtPointer call_data)
{
  struct fe_form_data *fed = (struct fe_form_data *) closure;
#ifdef MOCHA
  lo_FormElementToggleData *data;
  Bool save;

  data = &fed->form->element_data->ele_toggle;
  save = data->toggled;
#endif

  XFE_GetFormElementValue (fed->context, fed->form, False);

#ifdef MOCHA
  /* We are about to submit, so tell libmocha */
  CALLING_MOCHA(fed->context);
  if (!LM_SendOnClick (fed->context, (LO_Element *) fed->form))
    {
      Boolean restore = save;

      if (IS_FORM_DESTROYED(fed->context)) return;

      XtVaSetValues (widget, XmNset, restore, 0);
      data->toggled = restore;
    }
  else
    if (IS_FORM_DESTROYED(fed->context)) return;

#endif /* MOCHA */
}

static void
fe_popup_form_cb (Widget widget, XtPointer closure, XtPointer call_data)
{
  struct fe_form_data *fed = (struct fe_form_data *) closure;
  lo_FormElementSelectData *data;
  lo_FormElementOptionData *d2;
  int i;

  data = &fed->form->element_data->ele_select;
  d2 = (lo_FormElementOptionData *) data->options;

  for (i = 0; i < fed->nkids; i++)
    {
      fed->selected_p [i] = (fed->popup_kids [i] == widget);
#ifdef MOCHA
      d2 [i].selected = fed->selected_p [i];
#endif /* MOCHA */
    }
#ifdef MOCHA
  CALLING_MOCHA(fed->context);
  LM_SendOnChange (fed->context, (LO_Element *) fed->form);
  if (IS_FORM_DESTROYED(fed->context)) return;
#endif /* MOCHA */

  /* There could have been many cascades. Unpost all of them */
  XtUnmanageChild(fed->popup_widget);
}


static void
fe_list_form_cb (Widget widget, XtPointer closure, XtPointer call_data)
{
  struct fe_form_data *fed = (struct fe_form_data *) closure;
  XmListCallbackStruct *cb = (XmListCallbackStruct *) call_data;
  lo_FormElementSelectData *data;
  lo_FormElementOptionData *d2;
  int i, j;

  data = &fed->form->element_data->ele_select;
  d2 = (lo_FormElementOptionData *) data->options;

  switch (cb->reason)
    {
    case XmCR_SINGLE_SELECT:
    case XmCR_BROWSE_SELECT:
      for (i = 0; i < fed->nkids; i++)
	{
	  /* Note that the fuckin' item_position starts at 1, not 0!!!! */
	  fed->selected_p [i] = (cb->selected_item_count &&
			       i == (cb->item_position - 1));
#ifdef MOCHA
          d2 [i].selected = fed->selected_p [i];
#endif /* MOCHA */
        }
      break;
    case XmCR_MULTIPLE_SELECT:
    case XmCR_EXTENDED_SELECT:
      for (i = 0; i < fed->nkids; i++)
	{
	  fed->selected_p [i] = 0;
#ifdef MOCHA
	  d2 [i].selected = 0;
#endif /* MOCHA */
	  for (j = 0; j < cb->selected_item_count; j++)
	    if (i == (cb->selected_item_positions [j] - 1))
	      {
	        fed->selected_p [i] = 1;
#ifdef MOCHA
		d2 [i].selected = fed->selected_p [i];
#endif /* MOCHA */
	      }	
	}
      break;
    case XmCR_DEFAULT_ACTION:
      break;
    default:
      assert (0);
      return;
    }
#ifdef MOCHA
  CALLING_MOCHA(fed->context);
  LM_SendOnChange (fed->context, (LO_Element *) fed->form);
  if (IS_FORM_DESTROYED(fed->context)) return;
#endif /* MOCHA */
}


void
fe_ScrollForms (MWContext *context, int x_off, int y_off)
{
  Widget *kids;
  Cardinal nkids = 0;

  if (x_off == 0 && y_off == 0)
    return;

  XtVaGetValues (CONTEXT_DATA (context)->drawing_area,
		 XmNchildren, &kids, XmNnumChildren, &nkids,
		 0);
  while (nkids--)
    XtMoveWidget (kids [nkids],
		  kids [nkids]->core.x + x_off,
		  kids [nkids]->core.y + y_off);
}


void
fe_GravityCorrectForms (MWContext *context, int x_off, int y_off)
{
  Widget *kids;
  Cardinal nkids = 0;

  if (x_off == 0 && y_off == 0)
    return;

  XtVaGetValues (CONTEXT_DATA (context)->drawing_area,
                 XmNchildren, &kids, XmNnumChildren, &nkids,
                 0);

  while (nkids--)
  {
    if (XtIsManaged (kids[nkids]))
    {
        kids[nkids]->core.x = kids[nkids]->core.x + x_off;
        kids[nkids]->core.y = kids[nkids]->core.y + y_off;
    }
  }
}


void
fe_SetFormsGravity (MWContext *context, int gravity)
{
  Widget *kids;
  Widget area;
  Cardinal nkids = 0;
  XSetWindowAttributes attr;
  unsigned long valuemask;
 
  XtVaGetValues (CONTEXT_DATA (context)->drawing_area,
                 XmNchildren, &kids, XmNnumChildren, &nkids,
                 0);
 
  valuemask = CWBitGravity | CWWinGravity;
  attr.win_gravity = gravity;
  attr.bit_gravity = gravity;
  area = CONTEXT_DATA (context)->drawing_area;
  XChangeWindowAttributes (XtDisplay(area), XtWindow(area), valuemask, &attr);
  while (nkids--)
  {
    if (XtIsManaged (kids[nkids]))
      XChangeWindowAttributes (XtDisplay(kids[nkids]), XtWindow(kids[nkids]),
                valuemask, &attr);
  }
}


/* -------------------------------------------------------------------------
 * Input focus and event controls, for Mocha.
 */

#ifdef MOCHA

/* For now mocha backend wont use these functions. So they are defined here */
static void FE_LowerWindow(MWContext *context);

/*
 * Force input to be focused on an element in the given window.
 */
void
FE_FocusInputElement(MWContext *context, LO_Element *element)
{
  LO_FormElementStruct *form = (LO_FormElementStruct *) element;
  struct fe_form_data *fed;
  
  TRACEMSG(("XFE_FocusInputElement: element->type = %d\n"));

  if (!context) return;

  if (element) {
    switch (form->element_data->type)
      {
      case FORM_TYPE_PASSWORD:
      case FORM_TYPE_TEXTAREA:
      case FORM_TYPE_TEXT:
	fed = form->element_data->ele_text.FE_Data;
	XmProcessTraversal (fed->widget, XmTRAVERSE_CURRENT);
	break;
      case FORM_TYPE_FILE:
	fed = form->element_data->ele_text.FE_Data;
	XmProcessTraversal (fed->file_widget, XmTRAVERSE_CURRENT);
	break;
      default:
	break;
      }
  }
  else
    {
      if (context->is_grid_cell)
	fe_SetGridFocus (context);
      XmProcessTraversal (CONTEXT_WIDGET(context), XmTRAVERSE_CURRENT);
    }

  if (!element) {
    FE_RaiseWindow(context);
    CALLING_MOCHA(context);
    LM_SendOnFocus(context, NULL);
    if (IS_FORM_DESTROYED(context)) return;
  }
}

/*
 * Force input to be defocused from an element in the given window.
 * It's ok if the input didn't have input focus.
 */
void
FE_BlurInputElement(MWContext *context, LO_Element *element)
{
  /* Just blow away focus */    
  TRACEMSG(("XFE_BlurInputElement: element->type = %d\n"));

  if (!context) return;

  fe_NeutralizeFocus (context);

  if (!element) {
    FE_LowerWindow(context);
    CALLING_MOCHA(context);
    LM_SendOnBlur(context, NULL);
    if (IS_FORM_DESTROYED(context)) return;
  }
}
#endif /* MOCHA -- added by jwz */

/*
 * Raise the window to the top of the view order
 */
void
FE_RaiseWindow(MWContext *context)
{
  XtRealizeWidget (CONTEXT_WIDGET (context));
  XtManageChild (CONTEXT_WIDGET (context));
  /* XXX Uniconify the window if it was iconified */
  XRaiseWindow(XtDisplay(CONTEXT_WIDGET(context)),
	       XtWindow(CONTEXT_WIDGET(context)));
}

#ifdef MOCHA /* added by jwz */
/*
 * Lower the window to the bottom of the view order
 */
void
FE_LowerWindow(MWContext *context)
{
  if (!XtIsRealized(CONTEXT_WIDGET(context)))
      return;

  XLowerWindow(XtDisplay(CONTEXT_WIDGET(context)),
	       XtWindow(CONTEXT_WIDGET(context)));
}

/*
 * Selecting input in a field, highlighting it and preparing for change.
 */
void
FE_SelectInputElement(MWContext *context, LO_Element *element)
{
  LO_FormElementStruct *form = (LO_FormElementStruct *) element;
  struct fe_form_data *fed = form->element_data->ele_text.FE_Data;
   
  TRACEMSG(("XFE_SelectInputElement: element->type = %d\n"));
   switch (form->element_data->type)
     {
       case FORM_TYPE_TEXTAREA:
       case FORM_TYPE_TEXT:
	 XmTextSetSelection (fed->widget, 0,
			     XmTextGetMaxLength (fed->widget), CurrentTime);
	 break;
       default:
	 break;
     }
}

/*
 * Tell the FE that something in element changed, and to redraw it in a way
 * that reflects the change.  The FE is responsible for telling layout about
 * radiobutton state.
 */
void
FE_ChangeInputElement(MWContext *context, LO_Element *element)
{
  LO_FormElementStruct *form = (LO_FormElementStruct *) element;
  struct fe_form_data *fed = form->element_data->ele_text.FE_Data;
  int16 charset;
  XP_Bool checked;

  charset = form->text_attr->charset;

  TRACEMSG(("XFE_ChangeInputElement: element->type = %d\n"));
  switch (form->element_data->type)
    {
      case FORM_TYPE_TEXT:
      case FORM_TYPE_TEXTAREA: {
	lo_FormElementTextData *data = &form->element_data->ele_text;
	char *text = (char *) data->current_text;
	unsigned char *loc;

	if (!text) text = (char *) data->default_text;
	loc = fe_ConvertToLocaleEncoding (charset, text);
	XtVaSetValues (fed->widget, XmNvalue, text, 0);
	if (((char *) loc) != text)
	  {
	    XP_FREE (loc);
	  }
	break;
      }
      case FORM_TYPE_RADIO:
	checked = form->element_data->ele_toggle.toggled;
	LO_FormRadioSet (context, form);
	/* SPECIAL: If this was the only radio button in the radio group,
	   LO_FormRadioSet() will turn it back ON again if Mocha tried to
	   turn it OFF. We want to let mocha be able to turn ON/OFF
	   radio buttons. So we are going to override that and let mocha
	   have its way.
	*/
	XFE_SetFormElementToggle(context, form, checked);
	break;
      case FORM_TYPE_CHECKBOX: {
	lo_FormElementToggleData *data = &form->element_data->ele_toggle;
	XtVaSetValues (fed->widget, XmNset, data->toggled, 0);
	break;
      }
      case FORM_TYPE_RESET:
	break;
      case FORM_TYPE_SELECT_ONE:
	/* Fall through */
      case FORM_TYPE_SELECT_MULT:
	XFE_FreeFormElement(context, form->element_data);
	CONTEXT_DATA(context)->mocha_changing_input_element = True;
	XFE_GetFormElementInfo(context, form);
	CONTEXT_DATA(context)->mocha_changing_input_element = False;
	break;
      default:
	break;
    }
}

/*
** Tell the FE that a form is being submitted without a UI gesture indicating
** that fact, i.e., in a Mocha-automated fashion ("document.myform.submit()").
** The FE is responsible for emulating whatever happens when the user hits the
** submit button, or auto-submits by typing Enter in a single-field form.
*/
void
FE_SubmitInputElement(MWContext *context, LO_Element *element)
{
  LO_FormSubmitData *submit;
  URL_Struct        *url;
  History_entry *he = NULL;

  TRACEMSG(("XFE_SubmitInputElement: element->type = %d\n"));
  if (!context || !element)
    return;

  submit = LO_SubmitForm (context, (LO_FormElementStruct *) element);
  if (!submit)
    return;

  /* Create the URL to load */
  url = NET_CreateURLStruct((char *)submit->action, NET_DONT_RELOAD);

  /* ... add the form data */
  NET_AddLOSubmitDataToURLStruct(submit, url);

  /* referrer field if there is one */
  he = SHIST_GetCurrent (&context->hist);
  if (he || he->address)
    url->referer = XP_STRDUP (he->address);
  else
    url->referer = NULL;

  fe_GetURL (context, url, FALSE);
  LO_FreeSubmitData (submit);
}

/*
 * Emulate a button or HREF anchor click for element.
 */
void
FE_ClickInputElement(MWContext *context, LO_Element *xref)
{
  LO_FormElementStruct *form = (LO_FormElementStruct *) xref;
  struct fe_form_data *fed = form->element_data->ele_text.FE_Data;

  switch (xref->type) {
  case LO_FORM_ELE:
    {
      XmPushButtonCallbackStruct cb;
      cb.reason = XmCR_ACTIVATE;
      cb.event = 0;
      cb.click_count = 1;
      XtCallCallbacks (fed->widget, XmNactivateCallback, &cb);
    }
    break;
  case LO_IMAGE:
  case LO_TEXT:
    {
      XEvent event;

      event.type = ButtonPress;
      event.xbutton.x = xref->lo_image.x;
      event.xbutton.y = xref->lo_image.y;
      event.xbutton.time = XtLastTimestampProcessed (XtDisplay(CONTEXT_WIDGET (context)));
      (void) fe_HandleHREF (context, xref, False, False, &event, NULL, 0);
    }
    break;
  }
}

#endif /* MOCHA */

