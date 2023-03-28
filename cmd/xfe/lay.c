/* -*- Mode: C; tab-width: 8 -*-
   layout.c --- UI routines called by the layout module.
   Copyright © 1994 Netscape Communications Corp., all rights reserved.
   Created: Jamie Zawinski <jwz@netscape.com>, 23-Jun-94.
 */

#include "mozilla.h"
#include "xfe.h"
#include "fonts.h"
#include "felocale.h"
#include "msgcom.h"
#include "mozjava.h"
#include "np.h"

#include <Xm/SashP.h> /* for grid edges */
#include <Xm/DrawP.h> /* #### for _XmDrawShadows() */

#ifdef MOCHA
#include "layout.h"
#include <libmocha.h>
#endif /* MOCHA */

#include <libi18n.h>
/* for XP_GetString() */
#include <xpgetstr.h>
extern int XFE_COMPOSE;
extern int XFE_NO_SUBJECT;
extern int XFE_MAIL_TITLE_FMT, XFE_NEWS_TITLE_FMT, XFE_TITLE_FMT;
extern int XFE_EDITOR_TITLE_FMT;

static void fe_add_blinker (MWContext *context, LO_TextStruct *text);
static void fe_cancel_blinkers (MWContext *context);
void XFE_ClearView (MWContext *context, int which);

void fe_ClearAreaWithColor (MWContext *context,
		       int x, int y, unsigned int w, unsigned int h,
		       unsigned long color);

/* State for the highlighted item (this should eventually be done by
   layout I think).  This is kind of a kludge, but it only assumes that: 
   there is only one mouse; and that the events and translations are 
   being dispatched correctly... */
static LO_Element *last_armed_xref = 0;
static Boolean last_armed_xref_highlighted_p = False;
MWContext *last_documented_xref_context = 0;
static LO_Element *last_documented_xref = 0;
static LO_AnchorData *last_documented_anchor_data = 0;

/* This is in the set of function pointers, but there is no
   definition for it. */
MWContext*
XFE_CreateNewDocWindow(MWContext * calling_context,URL_Struct * URL)
{
    abort();
    return NULL;
}


#ifdef DEBUG_jwz
/* convert non-Latin1 characters to something close to readable...
 */
static unsigned char xfe_latin1_table[256] = {
# define LOSER '?'
  /* \000 */  LOSER,
  /* \001 */  LOSER,
  /* \002 */  LOSER,
  /* \003 */  LOSER,
  /* \004 */  LOSER,
  /* \005 */  LOSER,
  /* \006 */  LOSER,
  /* \007 */  LOSER,
  /* \010 */  LOSER,
  /* \011 */  '\r',
  /* \012 */  '\n',
  /* \013 */  LOSER,
  /* \014 */  LOSER,
  /* \015 */  LOSER,
  /* \016 */  LOSER,
  /* \017 */  LOSER,
  /* \020 */  LOSER,
  /* \021 */  LOSER,
  /* \022 */  LOSER,
  /* \023 */  LOSER,
  /* \024 */  LOSER,
  /* \025 */  LOSER,
  /* \026 */  LOSER,
  /* \027 */  LOSER,
  /* \030 */  LOSER,
  /* \031 */  LOSER,
  /* \032 */  LOSER,
  /* \033 */  LOSER,
  /* \034 */  LOSER,
  /* \035 */  LOSER,
  /* \036 */  LOSER,
  /* \037 */  LOSER,
  /* \040 */  ' ',	/* start of printable ASCII */
  /* \041 */  '!',
  /* \042 */  '"',
  /* \043 */  '#',
  /* \044 */  '$',
  /* \045 */  '%',
  /* \046 */  '&',
  /* \047 */  '\'',
  /* \050 */  '(',
  /* \051 */  ')',
  /* \052 */  '*',
  /* \053 */  '+',
  /* \054 */  ',',
  /* \055 */  '-',
  /* \056 */  '.',
  /* \057 */  '/',
  /* \060 */  '0',
  /* \061 */  '1',
  /* \062 */  '2',
  /* \063 */  '3',
  /* \064 */  '4',
  /* \065 */  '5',
  /* \066 */  '6',
  /* \067 */  '7',
  /* \070 */  '8',
  /* \071 */  '9',
  /* \072 */  ':',
  /* \073 */  ';',
  /* \074 */  '<',
  /* \075 */  '=',
  /* \076 */  '>',
  /* \077 */  '?',
  /* \100 */  '@',
  /* \101 */  'A',
  /* \102 */  'B',
  /* \103 */  'C',
  /* \104 */  'D',
  /* \105 */  'E',
  /* \106 */  'F',
  /* \107 */  'G',
  /* \110 */  'H',
  /* \111 */  'I',
  /* \112 */  'J',
  /* \113 */  'K',
  /* \114 */  'L',
  /* \115 */  'M',
  /* \116 */  'N',
  /* \117 */  'O',
  /* \120 */  'P',
  /* \121 */  'Q',
  /* \122 */  'R',
  /* \123 */  'S',
  /* \124 */  'T',
  /* \125 */  'U',
  /* \126 */  'V',
  /* \127 */  'W',
  /* \130 */  'X',
  /* \131 */  'Y',
  /* \132 */  'Z',
  /* \133 */  '[',
  /* \134 */  '\\',
  /* \135 */  ']',
  /* \136 */  '^',
  /* \137 */  '_',
  /* \140 */  '`',
  /* \141 */  'a',
  /* \142 */  'b',
  /* \143 */  'c',
  /* \144 */  'd',
  /* \145 */  'e',
  /* \146 */  'f',
  /* \147 */  'g',
  /* \150 */  'h',
  /* \151 */  'i',
  /* \152 */  'j',
  /* \153 */  'k',
  /* \154 */  'l',
  /* \155 */  'm',
  /* \156 */  'n',
  /* \157 */  'o',
  /* \160 */  'p',
  /* \161 */  'q',
  /* \162 */  'r',
  /* \163 */  's',
  /* \164 */  't',
  /* \165 */  'u',
  /* \166 */  'v',
  /* \167 */  'w',
  /* \170 */  'x',
  /* \171 */  'y',
  /* \172 */  'z',
  /* \173 */  '{',
  /* \174 */  '|',
  /* \175 */  '}',
  /* \176 */  '~',	/* end of printable ASCII */

  /* \177 */  LOSER,
  /* \200 */  LOSER,
  /* \201 */  LOSER,
  /* \202 */  '\'',	/* Windows: low-9 quote */
  /* \203 */  'f',	/* Windows: f with hook */
  /* \204 */  '"',	/* Windows: low-9 double quote */
  /* \205 */  '-',	/* Windows: ellipses */
  /* \206 */  LOSER,	/* Windows: dagger */
  /* \207 */  LOSER,	/* Windows: double dagger */
  /* \210 */  '^',	/* Windows: circumflex accent */
  /* \211 */  LOSER,	/* Windows: per mille sign */
  /* \212 */  '§',	/* Windows: S with caron */
  /* \213 */  '<',	/* Windows: left-pointing angle quote */
  /* \214 */  'E',	/* Windows: OE ligature */
  /* \215 */  LOSER,
  /* \216 */  LOSER,
  /* \217 */  LOSER,
  /* \220 */  LOSER,
  /* \221 */  '`',	/* Windows: left single quote */
  /* \222 */  '\'',	/* Windows: right single quote */
  /* \223 */  '"',	/* Windows: left double quote */
  /* \224 */  '"',	/* Windows: right double quote */
  /* \225 */  '*',	/* Windows: bullet */
  /* \226 */  '-',	/* Windows: en dash */
  /* \227 */  '-',	/* Windows: em dash */
  /* \230 */  '~',	/* Windows: small tilde */
  /* \231 */  LOSER,	/* Windows: trademark */
  /* \232 */  LOSER,	/* Windows: s with caron */
  /* \233 */  '>',	/* Windows: right-pointing angle quote */
  /* \234 */  LOSER,	/* Windows: oe ligature */
  /* \235 */  LOSER,
  /* \236 */  LOSER,
  /* \237 */  'Y',	/* Windows: Y diaeresis */

  /* \240 */  ' ',	/* start of printable non-ASCII Latin1 */
  /* \241 */  '¡',
  /* \242 */  '¢',
  /* \243 */  '£',
  /* \244 */  '¤',
  /* \245 */  '¥',
  /* \246 */  '¦',
  /* \247 */  '§',
  /* \250 */  '¨',
  /* \251 */  '©',
  /* \252 */  'ª',
  /* \253 */  '«',
  /* \254 */  '¬',
  /* \255 */  '­',
  /* \256 */  '®',
  /* \257 */  '¯',
  /* \260 */  '°',
  /* \261 */  '±',
  /* \262 */  '²',
  /* \263 */  '³',
  /* \264 */  '´',
  /* \265 */  'µ',
  /* \266 */  '¶',
  /* \267 */  '·',
  /* \270 */  '¸',
  /* \271 */  '¹',
  /* \272 */  'º',
  /* \273 */  '»',
  /* \274 */  '¼',
  /* \275 */  '½',
  /* \276 */  '¾',
  /* \277 */  '¿',
  /* \300 */  'À',
  /* \301 */  'Á',
  /* \302 */  'Â',
  /* \303 */  'Ã',
  /* \304 */  'Ä',
  /* \305 */  'Å',
  /* \306 */  'Æ',
  /* \307 */  'Ç',
  /* \310 */  'È',
  /* \311 */  'É',
  /* \312 */  'Ê',
  /* \313 */  'Ë',
  /* \314 */  'Ì',
  /* \315 */  'Í',
  /* \316 */  'Î',
  /* \317 */  'Ï',
  /* \320 */  'Ð',
  /* \321 */  'Ñ',
  /* \322 */  'Ò',
  /* \323 */  'Ó',
  /* \324 */  'Ô',
  /* \325 */  'Õ',
  /* \326 */  'Ö',
  /* \327 */  '×',
  /* \330 */  'Ø',
  /* \331 */  'Ù',
  /* \332 */  'Ú',
  /* \333 */  'Û',
  /* \334 */  'Ü',
  /* \335 */  'Ý',
  /* \336 */  'Þ',
  /* \337 */  'ß',
  /* \340 */  'à',
  /* \341 */  'á',
  /* \342 */  'â',
  /* \343 */  'ã',
  /* \344 */  'ä',
  /* \345 */  'å',
  /* \346 */  'æ',
  /* \347 */  'ç',
  /* \350 */  'è',
  /* \351 */  'é',
  /* \352 */  'ê',
  /* \353 */  'ë',
  /* \354 */  'ì',
  /* \355 */  'í',
  /* \356 */  'î',
  /* \357 */  'ï',
  /* \360 */  'ð',
  /* \361 */  'ñ',
  /* \362 */  'ò',
  /* \363 */  'ó',
  /* \364 */  'ô',
  /* \365 */  'õ',
  /* \366 */  'ö',
  /* \367 */  '÷',
  /* \370 */  'ø',
  /* \371 */  'ù',
  /* \372 */  'ú',
  /* \373 */  'û',
  /* \374 */  'ü',
  /* \375 */  'ý',
  /* \376 */  'þ',
  /* \377 */  'ÿ',	/* end of printable non-ASCII Latin1 */
# undef LOSER
};
#endif /* DEBUG_jwz */



/* Translate the string from ISO-8859/1 to something that the window
   system can use (for X, this is nearly a no-op.)
 */
char *
XFE_TranslateISOText (MWContext *context, int charset, char *ISO_Text)
{
  unsigned char *s;

  /* charsets such as Shift-JIS contain 0240's that are valid */
  if (INTL_CharSetType(charset) != SINGLEBYTE)
    return ISO_Text;

#ifdef DEBUG_jwz
  if (ISO_Text)
    for (s = (unsigned char *) ISO_Text; *s; s++)
      *s = xfe_latin1_table[(int) *s];

#else /* !DEBUG_jwz */

  /* When &nbsp; is encountered, display a normal space character instead.
     This is necessary because the MIT fonts are messed up, and have a
     zero-width character for nobreakspace, so we need to print it as a
     normal space instead. */
  if (ISO_Text)
    for (s = (unsigned char *) ISO_Text; *s; s++)
      if (*s == 0240) *s = ' ';
#endif /* !DEBUG_jwz */

  return ISO_Text;
}

struct fe_gc_data
{
  unsigned long flags;
  XGCValues gcv;
  GC gc;
};

/* The GC cache is shared among all windows, since it doesn't hog
   any scarce resources (like colormap entries.) */
static struct fe_gc_data fe_gc_cache [30] = { { 0, }, };
static int fe_gc_cache_fp;
static int fe_gc_cache_wrapped_p = 0;

/* Dispose of entries matching the given flags, compressing the GC cache */
void
fe_FlushGCCache (Widget widget, unsigned long flags)
{
  int i, new_fp;

  Display *dpy = XtDisplay (widget);
  int maxi = (fe_gc_cache_wrapped_p ? countof (fe_gc_cache) : fe_gc_cache_fp);
  new_fp = 0;
  for (i = 0; i < maxi; i++)
    {
      if (fe_gc_cache [i].flags & flags)
        {
          XFreeGC (dpy, fe_gc_cache [i].gc);
          memset (&fe_gc_cache [i], 0,  sizeof (fe_gc_cache [i]));
        }
      else
        fe_gc_cache[new_fp++] = fe_gc_cache[i];
    }
  if (new_fp == countof (fe_gc_cache))
    {
      fe_gc_cache_wrapped_p = 1;
      fe_gc_cache_fp = 0;
    }
  else
    {
      fe_gc_cache_wrapped_p = 0;
      fe_gc_cache_fp = new_fp;
    }
}

GC
fe_GetGCfromDW(Display* dpy, Window win, unsigned long flags, XGCValues *gcv)
{
  int i;
  for (i = 0;
       i < (fe_gc_cache_wrapped_p ? countof (fe_gc_cache) : fe_gc_cache_fp);
       i++)
    {
      if (flags == fe_gc_cache [i].flags &&
	  !memcmp (gcv, &fe_gc_cache [i].gcv, sizeof (*gcv)))
	return fe_gc_cache [i].gc;
    }

  {
    GC gc;
    int this_slot = fe_gc_cache_fp;
    int clear_p = fe_gc_cache_wrapped_p;

    fe_gc_cache_fp++;
    if (fe_gc_cache_fp >= countof (fe_gc_cache))
      {
	fe_gc_cache_fp = 0;
	fe_gc_cache_wrapped_p = 1;
      }

    if (clear_p)
      {
	XFreeGC (dpy, fe_gc_cache [this_slot].gc);
      }

    gc = XCreateGC (dpy, win, flags, gcv);

    fe_gc_cache [this_slot].flags = flags;
    fe_gc_cache [this_slot].gcv = *gcv;
    fe_gc_cache [this_slot].gc = gc;

    return gc;
  }
}

GC
fe_GetGC(Widget widget, unsigned long flags, XGCValues *gcv)
{
    Display *dpy = XtDisplay (widget);
    Window win = XtWindow (widget);

    return fe_GetGCfromDW(dpy, win, flags, gcv);
}

static GC
fe_get_text_gc (MWContext *context, LO_TextAttr *text, fe_Font *font_ret,
		Boolean *selected_p, Boolean blunk)
{
  unsigned long flags;
  XGCValues gcv;
  Widget widget = CONTEXT_WIDGET (context);
  fe_Font font = fe_LoadFontFromFace (context, text, &text->charset, text->font_face,
			      text->size, text->fontmask);
  Pixel fg, bg;
  bg = fe_GetPixel(context,
                   text->bg.red, text->bg.green, text->bg.blue);
  fg = fe_GetPixel(context,
                   text->fg.red, text->fg.green, text->fg.blue);

/*  if (text->attrmask & LO_ATTR_ANCHOR)
    fg = CONTEXT_DATA (context)->xref_pixel;*/

  if (selected_p && *selected_p)
    {
      Pixel nfg = CONTEXT_DATA (context)->select_fg_pixel;
      Pixel nbg = CONTEXT_DATA (context)->select_bg_pixel;
      Pixel swap;

      /* First, undo the hack that layout has already swapped fg/bg on us. */
      swap = fg; fg = bg; bg = swap;

#if 0
      /* Preserve the original link color if possible:
	 If the default foreground and highlight foreground were specified
	 as the same color, then just use the foreground from the text object.
	 Likewise for the background.
       */
      if (nfg == CONTEXT_DATA (context)->fg_pixel)
	nfg = fg;
      else if (nfg == CONTEXT_DATA (context)->bg_pixel)
	nfg = bg;

      if (nbg == CONTEXT_DATA (context)->bg_pixel)
	nbg = bg;
      else if (nbg == CONTEXT_DATA (context)->fg_pixel)
	nbg = fg;
#endif

      fg = nfg;
      bg = nbg;
    }

  if (blunk)
    fg = bg;

  if (! font) return NULL;

  memset (&gcv, ~0, sizeof (gcv));

  flags = 0;
  FE_SET_GC_FONT(text->charset, &gcv, font, &flags);
  gcv.foreground = fg;
  gcv.background = bg;
  flags |= (GCForeground | GCBackground);

  if (font_ret) *font_ret = font;

  return fe_GetGC (widget, flags, &gcv);
}

/* Given text and attributes, returns the size of those characters.
 */
int
XFE_GetTextInfo (MWContext *context,
		LO_TextStruct *text,
		LO_TextInfo *text_info)
{
  fe_Font font;
  char *str = (char *) text->text;
  int length = text->text_len;
  int remaining = length;

#ifdef DEBUG_jwz
  {
    char o = str[length];
    /* Don't know why this isn't getting called on normal document text...
       This side-effects the string to make random Windows characters
       readable.  This function *should* be sufficiently early...
     */
    str[length] = 0;
    str = XFE_TranslateISOText (context, text->text_attr->charset, str);
    str[length] = o;
  }
#endif /* DEBUG_jwz */

  font = fe_LoadFontFromFace (context, text->text_attr,
			      &text->text_attr->charset,
			      text->text_attr->font_face,
			      text->text_attr->size,
			      text->text_attr->fontmask);
  /* X is such a winner, it uses 16 bit quantities to represent all pixel
     widths.  This is really swell, because it means that if you've got
     a large font, you can't correctly compute the size of strings which
     are only a few thousand characters long.  So, when the string is more
     than N characters long, we divide up our calls to XTextExtents to
     keep the size down so that the library doesn't run out of fingers
     and toes.
   */
#define SUCKY_X_MAX_LENGTH 600

  text_info->ascent = 14;
  text_info->descent = 3;
  text_info->max_width = 0;
  text_info->lbearing = 0;
  text_info->rbearing = 0;
  if (! font) return 0;

  do
    {
      int L = (remaining > SUCKY_X_MAX_LENGTH ? SUCKY_X_MAX_LENGTH :
	       remaining);
      int ascent, descent;
      XCharStruct overall;
      FE_TEXT_EXTENTS (text->text_attr->charset, font, str, L,
		    &ascent, &descent, &overall);
      /* ascent and descent are per the font, not per this text. */
      text_info->ascent = ascent;
      text_info->descent = descent;

      text_info->max_width += overall.width;

#define FOO(x,y) if (y > x) x = y
      FOO (text_info->lbearing,   overall.lbearing);
      FOO (text_info->rbearing,   overall.rbearing);
      /*
       * If font metrics were set right, overall.descent should never exceed
       * descent, but since there are broken fonts in the world.
       */
      FOO (text_info->descent,   overall.descent);
#undef FOO

      str += L;
      remaining -= L;
    }
  while (remaining > 0);

  /* What is the return value expected to be?
     layout/layout.c doesn't seem to use it. */
  return 0;
}


/* Draws a rectangle with dropshadows on the drawing_area window.
   shadow_width is the border thickness (they go inside the rect).
   shadow_style is XmSHADOW_IN or XmSHADOW_OUT.
 */
void
fe_DrawSelectedShadows(MWContext *context, Drawable drawable,
		int x, int y, int width, int height,
		int shadow_width, int shadow_style, Boolean selected)
{
  Widget widget = CONTEXT_WIDGET (context);
  Display *dpy = XtDisplay (widget);
  XGCValues gcv1, gcv2;
  GC gc1, gc2;
  unsigned long flags;
  memset (&gcv1, ~0, sizeof (gcv1));
  memset (&gcv2, ~0, sizeof (gcv2));

  if (CONTEXT_DATA (context)->backdrop_pixmap ||
      CONTEXT_DATA (context)->bg_pixel !=
      CONTEXT_DATA (context)->default_bg_pixel)
    {
      static Pixmap gray50 = 0;
      if (! gray50)
	{
#	  define gray50_width  8
#	  define gray50_height 2
	  static char gray50_bits[] = { 0x55, 0xAA };
	  gray50 =
	    XCreateBitmapFromData (XtDisplay (widget),
				   RootWindowOfScreen (XtScreen (widget)),
				   gray50_bits, gray50_width, gray50_height);
	}
      flags = (GCForeground | GCStipple | GCFillStyle |
	       GCTileStipXOrigin | GCTileStipYOrigin);

      gcv1.fill_style = FillStippled;
      gcv1.stipple = gray50;
      gcv1.ts_x_origin = -CONTEXT_DATA (context)->document_x;
      gcv1.ts_y_origin = -CONTEXT_DATA (context)->document_y;
      gcv1.foreground = fe_GetPixel (context, 0xFF, 0xFF, 0xFF);
      gcv2 = gcv1;
      gcv2.foreground = fe_GetPixel (context, 0x00, 0x00, 0x00);
    }
  else
    {
      flags = GCForeground;
      gcv1.foreground = CONTEXT_DATA (context)->top_shadow_pixel;
      gcv2.foreground = CONTEXT_DATA (context)->bottom_shadow_pixel;
    }

#ifdef EDITOR
  if (selected) {
      gcv1.background = CONTEXT_DATA(context)->select_bg_pixel;
      gcv1.line_style = LineDoubleDash;

      gcv2.background = CONTEXT_DATA(context)->select_bg_pixel;
      gcv2.line_style = LineDoubleDash;

      flags |= GCLineStyle|GCBackground;
  }
#endif /*EDITOR*/

  gc1 = fe_GetGC (widget, flags, &gcv1);
  gc2 = fe_GetGC (widget, flags, &gcv2);

  _XmDrawShadows (dpy, drawable, gc1, gc2, x, y, width, height,
		  shadow_width, shadow_style);
}

void
fe_DrawShadows (MWContext *context, Drawable drawable,
		int x, int y, int width, int height,
		int shadow_width, int shadow_style)
{
    fe_DrawSelectedShadows(context, drawable,
			      x, y, width, height,
			      shadow_width, shadow_style, FALSE);

}


/* Put some text on the screen at the given location with the given
   attributes.  (What is iLocation?  It is ignored.)
 */
static void
fe_display_text (MWContext *context, int iLocation, LO_TextStruct *text,
		 int32 start, int32 end, Boolean blunk)
{
  Widget shell = CONTEXT_WIDGET (context);
  Widget drawing_area = CONTEXT_DATA (context)->drawing_area;
  Display *dpy = XtDisplay (shell);
  Window win = XtWindow (drawing_area);
  fe_Font font;
  int ascent, descent;
  GC gc;
  long x, y;
  long x_offset = 0;
  Boolean selected_p = False;


  if ((text->ele_attrmask & LO_ELE_SELECTED) &&
	(start >= text->sel_start) && (end-1 <= text->sel_end))
    selected_p = True;

  gc = fe_get_text_gc (context, text->text_attr, &font, &selected_p, blunk);
  if (!gc)
  {
	return;
  }

  FE_FONT_EXTENTS(text->text_attr->charset, font, &ascent, &descent);

  x = (text->x + text->x_offset
	    - CONTEXT_DATA (context)->document_x);
  y = (text->y + text->y_offset + ascent
	    - CONTEXT_DATA (context)->document_y);

  if (text->text_len == 0)
    return;

  if (! XtIsRealized (drawing_area))
    return;

  if ((x > 0 && x > CONTEXT_DATA (context)->scrolled_width) ||
      (y > 0 && y > (CONTEXT_DATA (context)->scrolled_height +
	text->y_offset + ascent)) ||
      (x + text->width < 0) ||
      (y + text->line_height < 0))
    return;

  if (start < 0)
	start = 0;
  if (end > text->text_len)
	end = text->text_len;

  if (end - start > SUCKY_X_MAX_LENGTH)
    /* that's a fine way to make X blow up *real* good! */
    end = start + SUCKY_X_MAX_LENGTH;

  /* #### Oh, this doesn't even work, because that Bina bastard is
     passing us massively negative (> 16 bit) starting pixel positions.
   */

  if (start > 0)
    {
      XCharStruct overall;
      int ascent, descent;
      if (! font) abort ();
      FE_TEXT_EXTENTS (text->text_attr->charset, font, (char *) text->text,
			start, &ascent, &descent, &overall);
      x_offset = overall.width;
      x += x_offset;
    }

  /* If there is a backdrop pixmap, first draw a rectangle of that pixmap.
     Note that in many cases, the backdrop is already there, since it will
     have been drawn by our handling of exposure events or scrolling (we
     generally draw the backdrop before calling LO_RefreshArea().)  However,
     in some cases it needs to be redrawn again anyway - as when there had
     been a selection but no longer is one.  I don't know how to tell those
     two cases apart, so we're gonna be drawing that damn backdrop all the
     time...

     Do all the backdrop pixmap thing only if there is a backdrop and there
     is no_background for the text. If this text is in a table cell with
     background, then text_attr->no-background will be set to False.	- dp
   */
  if (blunk ||
      (text->text_attr->no_background &&
       CONTEXT_DATA (context)->backdrop_pixmap &&
       /* This can only happen if something went wrong while loading
	  the background pixmap, I think. */
       CONTEXT_DATA (context)->backdrop_pixmap != (Pixmap) ~0))
    {
      GC gc2;
      long width;

      if (CONTEXT_DATA (context)->backdrop_pixmap &&
	  /* This can only happen if something went wrong while loading
	     the background pixmap, I think. */
	  CONTEXT_DATA (context)->backdrop_pixmap != (Pixmap) ~0)
	{
	  XGCValues gcv;
	  memset (&gcv, ~0, sizeof (gcv));
	  gcv.fill_style = FillTiled;
	  gcv.tile = CONTEXT_DATA (context)->backdrop_pixmap;
	  gcv.ts_x_origin = -CONTEXT_DATA (context)->document_x;
	  gcv.ts_y_origin = -CONTEXT_DATA (context)->document_y;
	  gc2 = fe_GetGC (drawing_area,
			  (GCTile | GCFillStyle |
			   GCTileStipXOrigin | GCTileStipYOrigin),
			  &gcv);
	}
      else
	{
	  gc2 = gc;
	}

      if (end == text->text_len)
	{
	  width = text->width - x_offset;
	}
      else
	{
	  XCharStruct overall;
	  int ascent, descent;
	  if (! font) abort ();
	  FE_TEXT_EXTENTS (text->text_attr->charset, font,
		((char *) text->text) + start, end - start, &ascent, &descent,
		&overall);
	  width = overall.width;
	}

      XFillRectangle (dpy, win, gc2,
		      x, y - ascent,
		      width, ascent + descent);
    }

  if (blunk)
    ;	/* No text to draw. */
  else if (!selected_p && (CONTEXT_DATA (context)->backdrop_pixmap &&
	/* This can only happen if something went wrong while loading
	   the background pixmap, I think. */
	         CONTEXT_DATA (context)->backdrop_pixmap != (Pixmap) ~0)
	   && text->text_attr->no_background)
    FE_DRAW_STRING (text->text_attr->charset, dpy, win, font, gc, x, y,
	((char *) text->text) + start, end - start);
  else
    FE_DRAW_IMAGE_STRING (text->text_attr->charset, dpy, win, font, gc, x, y,
	((char *) text->text)+start, end - start);

  if (text->text_attr->attrmask & LO_ATTR_UNDERLINE ||
      text->text_attr->attrmask & LO_ATTR_STRIKEOUT ||
      (text->text_attr->attrmask & LO_ATTR_ANCHOR &&
       fe_globalPrefs.underline_links_p)
#if 0
      || (text->height < text->line_height)
#endif
    )
    {
      int upos;
      unsigned int uthick;
      int ul_width;

      if (start == 0 && end == text->text_len)
	{
	  ul_width = text->width;
	}
      else
	{
	  XCharStruct overall;
	  int ascent, descent;
	  if (! font) abort ();
	  FE_TEXT_EXTENTS (text->text_attr->charset, font,
		(char *) text->text+start, end-start, &ascent, &descent,
		&overall);
	  ul_width = overall.width;
	}

#if 0
      if (text->height < text->line_height)
	{
	  /* If the text is shorter than the line, then XDrawImageString()
	     won't fill in the whole background - so we need to do that by
	     hand. */
	  GC gc;
	  XGCValues gcv;
	  memset (&gcv, ~0, sizeof (gcv));
	  gcv.foreground = (text->selected
			    ? CONTEXT_DATA (context)->highlight_bg_pixel
			    : fe_GetPixel (context,
					   text->text_attr->bg.red,
					   text->text_attr->bg.green,
					   text->text_attr->bg.blue));
	  gc = fe_GetGC (CONTEXT_WIDGET (context), GCForeground, &gcv);
	  XFillRectangle (XtDisplay (CONTEXT_WIDGET (context)),
			  XtWindow (CONTEXT_DATA (context)->drawing_area),
			  gc,
			  x, y - ascent - 1,
			  ul_width,
			  text->line_height - text->height);
	}
#endif

      if (text->text_attr->attrmask & LO_ATTR_UNDERLINE ||
	  (text->text_attr->attrmask & LO_ATTR_ANCHOR &&
	   fe_globalPrefs.underline_links_p))
	{
	  int lineDescent;
	  upos = fe_GetUnderlinePosition(text->text_attr->charset);
	  lineDescent = text->line_height - text->y_offset - ascent - 1;
	  if (upos > lineDescent)
	    upos = lineDescent;
	  XDrawLine (dpy, win, gc, x, y + upos, x + ul_width, y + upos);
	}

      if (text->text_attr->attrmask & LO_ATTR_STRIKEOUT)
	{
	  upos = fe_GetStrikePosition(text->text_attr->charset, font);
	  uthick = (ascent / 8);
	  if (uthick <= 1)
	    XDrawLine (dpy, win, gc, x, y + upos, x + ul_width, y + upos);
	  else
	    XFillRectangle (dpy, win, gc, x, y + upos, ul_width, uthick);
	}
    }

  if (text->text_attr->attrmask & LO_ATTR_BLINK &&
      CONTEXT_DATA (context)->blinking_enabled_p)
    fe_add_blinker (context, text);
}

void
XFE_DisplayText (MWContext *context, int iLocation, LO_TextStruct *text,
	XP_Bool need_bg)
{
  fe_display_text (context, iLocation, text, 0, text->text_len, False);
}

void
XFE_DisplaySubtext (MWContext *context, int iLocation,
		   LO_TextStruct *text, int32 start_pos, int32 end_pos,
		   XP_Bool need_bg)
{
  fe_display_text (context, iLocation, text, start_pos, end_pos + 1, False);
}

#ifdef EDITOR

/*
 *    End of line indicator.
 */
typedef struct fe_bitmap_info
{
    unsigned char* bits;
    Dimension      width;
    Dimension      height;
    Pixmap         pixmap;
} fe_bitmap_info;

#define line_feed_width 7
#define line_feed_height 10
static unsigned char line_feed_bits[] = { /* lifted from Lucid Emacs */
    0x00, 0xbc, 0xfc, 0xe0, 0xe0, 0x72, 0x3e, 0x1e, 0x1e, 0x3e};
#define page_mark_width 8
#define page_mark_height 15
static char page_mark_bits[] = { /* from RobinS */
 0xfe,0x4f,0x4f,0x4f,0x4f,0x4e,0x48,0x48,0x48,0x48,0x48,0x48,0x48,0x48,0x48};

static fe_bitmap_info fe_line_feed_bitmap =
{ line_feed_bits, line_feed_width, line_feed_height };
static fe_bitmap_info fe_page_mark_bitmap =
{ page_mark_bits, page_mark_width, page_mark_height };

static Display* fe_line_feed_display;

static Pixmap
fe_make_line_feed_pixmap(Display* display, int type,
			 Dimension* r_width, Dimension* r_height)
{
    fe_bitmap_info* info;

    if (type == LO_LINEFEED_BREAK_PARAGRAPH)
	info = &fe_page_mark_bitmap;
    else
	info = &fe_line_feed_bitmap;

    if (fe_line_feed_display != display && fe_line_feed_display != 0) {
	if (info->pixmap != 0)
	    XFreePixmap(fe_line_feed_display, info->pixmap);
    }

    if (info->pixmap == 0) {

	info->pixmap = XCreateBitmapFromData(display,
					     DefaultRootWindow(display),
					     info->bits,
					     info->width,
					     info->height);
 	
	fe_line_feed_display = display;
    }

    *r_width = info->width;
    *r_height = info->height;
    
    return info->pixmap;
}

#endif /*EDITOR*/

/* Display a glyph representing a linefeed at the given location with the
   given attributes.  This looks just like a " " character.  (What is
   iLocation?  It is ignored.)
 */
void
XFE_DisplayLineFeed (MWContext *context,
		    int iLocation, LO_LinefeedStruct *line_feed, XP_Bool need_bg)
{
  GC gc;
  XGCValues gcv;
  unsigned long flags;
  LO_TextAttr *text = line_feed->text_attr;

  if ((line_feed->x > (CONTEXT_DATA (context)->document_x +
		       CONTEXT_DATA (context)->scrolled_width)) ||
      (line_feed->y > (CONTEXT_DATA (context)->document_y +
		       CONTEXT_DATA (context)->scrolled_height)) ||
      ((line_feed->y + line_feed->line_height)
       < CONTEXT_DATA (context)->document_y) ||
      ((line_feed->x + line_feed->x_offset + line_feed->width) <
       CONTEXT_DATA (context)->document_x))
    return;

  if (line_feed->width < 1)
    return;

  memset (&gcv, ~0, sizeof (gcv));

  gcv.foreground = ((line_feed->ele_attrmask & LO_ELE_SELECTED)
		    ? CONTEXT_DATA (context)->select_bg_pixel
		    : fe_GetPixel (context,
				   text->bg.red, text->bg.green,
				   text->bg.blue));
  flags = GCForeground;

  if (line_feed->text_attr->no_background &&
      CONTEXT_DATA (context)->backdrop_pixmap &&
      /* This can only happen if something went wrong while loading
	 the background pixmap, I think. */
      CONTEXT_DATA (context)->backdrop_pixmap != (Pixmap) ~0)
    {
      gcv.fill_style = FillTiled;
      gcv.tile = CONTEXT_DATA (context)->backdrop_pixmap;
      gcv.ts_x_origin = -CONTEXT_DATA (context)->document_x;
      gcv.ts_y_origin = -CONTEXT_DATA (context)->document_y;
      flags |= (GCTile | GCFillStyle | GCTileStipXOrigin | GCTileStipYOrigin);
    }

  gc = fe_GetGC (CONTEXT_DATA (context)->drawing_area, flags, &gcv);
  XFillRectangle (XtDisplay (CONTEXT_WIDGET (context)),
		  XtWindow (CONTEXT_DATA (context)->drawing_area),
		  gc,
		  line_feed->x + line_feed->x_offset
		  - CONTEXT_DATA (context)->document_x,
		  line_feed->y + line_feed->y_offset
		  - CONTEXT_DATA (context)->document_y,
		  line_feed->width, line_feed->height);
#ifdef EDITOR
  if (EDT_IS_EDITOR(context)
      &&
      EDT_DISPLAY_PARAGRAPH_MARKS(context)
      &&
      (line_feed->break_type != LO_LINEFEED_BREAK_SOFT)
      &&
      (!line_feed->prev || line_feed->prev->lo_any.edit_offset >= 0)) {

      LO_Color* fg_color;
      Display*  display = XtDisplay(CONTEXT_WIDGET(context));
      Window    window = XtWindow(CONTEXT_DATA(context)->drawing_area);
      Position  target_x;
      Position  target_y;
      Dimension width;
      Dimension height;
      Pixmap    bitmap;

      bitmap = fe_make_line_feed_pixmap(display,
					line_feed->break_type,
					&width,
					&height);
      
      target_x = line_feed->x + line_feed->x_offset
	         - CONTEXT_DATA(context)->document_x;

      target_y = line_feed->y + line_feed->y_offset
		 - CONTEXT_DATA(context)->document_y;

      memset (&gcv, ~0, sizeof (gcv));

      if ((line_feed->ele_attrmask & LO_ELE_SELECTED) != 0)
	  fg_color = &text->bg; /* layout delivers inverted colors */
      else
	  fg_color = &text->fg;

      gcv.foreground = fe_GetPixel(context,
				   fg_color->red,
				   fg_color->green,
				   fg_color->blue);
      gcv.graphics_exposures = False;
      gcv.clip_mask = bitmap;
      gcv.clip_x_origin = target_x;
      gcv.clip_y_origin = target_y;

      flags = GCClipMask|GCForeground|GCClipXOrigin| \
	      GCClipYOrigin|GCGraphicsExposures;

      gc = fe_GetGC(CONTEXT_DATA(context)->drawing_area, flags, &gcv);

      if (height > line_feed->height)
	  height = line_feed->height;

      XCopyPlane(display, bitmap, window,
		 gc,
		 0, 0, width, height,
		 target_x, target_y,
		 1L);
  }
#endif /*EDITOR*/
}

/* Display a horizontal line at the given location.
 */
void
XFE_DisplayHR (MWContext *context, int iLocation, LO_HorizRuleStruct *hr)
{
  int shadow_width = 1;			/* #### customizable? */
  int shadow_style = XmSHADOW_IN;	/* #### customizable? */
  int thickness = hr->thickness;
  long x = hr->x + hr->x_offset - CONTEXT_DATA (context)->document_x;
  long y = hr->y + hr->y_offset - CONTEXT_DATA (context)->document_y;
  int w = hr->width;
  Window win = XtWindow (CONTEXT_DATA (context)->drawing_area);

  if ((x > 0 && x > CONTEXT_DATA (context)->scrolled_width) ||
      (y > 0 && y > CONTEXT_DATA (context)->scrolled_height) ||
      (x + hr->width < 0) ||
      (y + hr->line_height < 0))
    return;

  thickness -= (shadow_width * 2);
  if (thickness < 0) thickness = 0;

#ifdef EDITOR
  /*
   *    Don't draw the editor's end-of-document hrule unless we're
   *    displaying paragraph marks.
   */
  if (hr->edit_offset < 0 && !EDT_DISPLAY_PARAGRAPH_MARKS(context)) {
      return;
  }
#endif /* EDITOR */

  if (hr->ele_attrmask & LO_ELE_SHADED)
    {
#ifdef EDITOR
	fe_DrawSelectedShadows(context, win,
			       x, y, w, thickness + (shadow_width * 2),
			       shadow_width, shadow_style,
			       ((hr->ele_attrmask & LO_ELE_SELECTED) != 0));
#else
	fe_DrawShadows (context, win, x, y, w, thickness + (shadow_width * 2),
	      shadow_width, shadow_style);
#endif /* EDITOR */
    }
  else
    {
      Display *dpy = XtDisplay (CONTEXT_DATA (context)->drawing_area);
      GC gc;
      XGCValues gcv;

      memset (&gcv, ~0, sizeof(gcv));
      gcv.foreground = CONTEXT_DATA(context)->fg_pixel;
      gc = fe_GetGC(CONTEXT_WIDGET (context), GCForeground, &gcv);
      XFillRectangle(dpy, win, gc, x, y, w, hr->height);
#ifdef EDITOR
      if ((hr->ele_attrmask & LO_ELE_SELECTED) != 0) {
	  gcv.background = CONTEXT_DATA(context)->select_bg_pixel;
	  gcv.line_width = 1;
	  gcv.line_style = LineDoubleDash;
	  gc = fe_GetGC(CONTEXT_WIDGET(context),
			GCForeground|GCBackground|GCLineWidth|GCLineStyle,
			&gcv);
	  XDrawRectangle(dpy, win, gc, x, y, w-1, hr->height-1);
      }
#endif /* EDITOR */
    }
}

void
XFE_DisplayBullet (MWContext *context, int iLocation, LO_BulletStruct *bullet)
{
  Widget widget = CONTEXT_WIDGET (context);
  Window window = XtWindow (CONTEXT_DATA (context)->drawing_area);
  Display *dpy = XtDisplay (widget);
  long x = bullet->x + bullet->x_offset - CONTEXT_DATA (context)->document_x;
  long y = bullet->y + bullet->y_offset - CONTEXT_DATA (context)->document_y;
  int w = bullet->width;
  int h = bullet->height;
  Boolean hollow_p = (bullet->bullet_type != BULLET_BASIC);
  GC gc;

  if ((x > 0 && x > CONTEXT_DATA (context)->scrolled_width) ||
      (y > 0 && y > CONTEXT_DATA (context)->scrolled_height) ||
      (x + w < 0) ||
      (y + h < 0))
    return;

  gc = fe_get_text_gc (context, bullet->text_attr, 0, 0, False);
  if (!gc)
  {
	return;
  }
  switch (bullet->bullet_type)
    {
    case BULLET_BASIC:
    case BULLET_ROUND:
      /* Subtract 1 to compensate for the behavior of XDrawArc(). */
      w -= 1;
      h -= 1;
      /* Now round up to an even number so that the circles look nice. */
      if (! (w & 1)) w++;
      if (! (h & 1)) h++;
      if (hollow_p)
	XDrawArc (dpy, window, gc, x, y, w, h, 0, 360*64);
      else
	XFillArc (dpy, window, gc, x, y, w, h, 0, 360*64);
      break;
    case BULLET_SQUARE:
      if (hollow_p)
	XDrawRectangle (dpy, window, gc, x, y, w, h);
      else
	XFillRectangle (dpy, window, gc, x, y, w, h);
      break;
    default:
      abort ();
    }
}

void
XFE_DisplaySubDoc (MWContext *context, int loc, LO_SubDocStruct *sd)
{
  int shadow_style = XmSHADOW_IN;	/* #### customizable? */
  long x = sd->x + sd->x_offset - CONTEXT_DATA (context)->document_x;
  long y = sd->y + sd->y_offset - CONTEXT_DATA (context)->document_y;
  Window win = XtWindow (CONTEXT_DATA (context)->drawing_area);

  if ((x > 0 && x > CONTEXT_DATA (context)->scrolled_width) ||
      (y > 0 && y > CONTEXT_DATA (context)->scrolled_height) ||
      (x + sd->width < 0) ||
      (y + sd->line_height< 0))
    return;

  fe_DrawShadows (context, win, x, y, sd->width, sd->height,
		  sd->border_width, shadow_style);
}

void
XFE_DisplayCell (MWContext *context, int loc, LO_CellStruct *cell)
{
  int shadow_style = XmSHADOW_IN;	/* #### customizable? */
  long x = cell->x + cell->x_offset - CONTEXT_DATA (context)->document_x;
  long y = cell->y + cell->y_offset - CONTEXT_DATA (context)->document_y;
  Window win = XtWindow (CONTEXT_DATA (context)->drawing_area);

  if ((x > 0 && x > CONTEXT_DATA (context)->scrolled_width) ||
      (y > 0 && y > CONTEXT_DATA (context)->scrolled_height) ||
      (x + cell->width < 0) ||
      (y + cell->line_height< 0))
    return;

  if (cell->bg_color) {
      Pixel color = fe_GetPixel (context,
				cell->bg_color->red,
				cell->bg_color->green,
				cell->bg_color->blue);
      fe_ClearAreaWithColor(context, x, y, cell->width, cell->height, color);
  }

  fe_DrawShadows (context, win, x, y, cell->width, cell->height,
		  cell->border_width, shadow_style);
}

void
XFE_DisplayTable (MWContext *context, int loc, LO_TableStruct *ts)
{
  int shadow_style = XmSHADOW_OUT;	/* #### customizable? */
  long x = ts->x + ts->x_offset - CONTEXT_DATA (context)->document_x;
  long y = ts->y + ts->y_offset - CONTEXT_DATA (context)->document_y;
  Window win = XtWindow (CONTEXT_DATA (context)->drawing_area);

  if ((x > 0 && x > CONTEXT_DATA (context)->scrolled_width) ||
      (y > 0 && y > CONTEXT_DATA (context)->scrolled_height) ||
      (x + ts->width < 0) ||
      (y + ts->line_height< 0))
    return;

  fe_DrawShadows (context, win, x, y, ts->width, ts->height,
		  ts->border_width, shadow_style);
}

typedef struct _SashInfo {
  Widget sash;
  Widget separator;
  LO_EdgeStruct *edge;
  MWContext *context;
  time_t last;
} SashInfo;

static void
fe_sash_cb (Widget widget, XtPointer closure, XtPointer call_data)
{
  SashInfo *sashinfo = (SashInfo *) closure;
  MWContext *context;
  LO_EdgeStruct *edge;
  SashCallData sash_data = (SashCallData) call_data;

  static EventMask activity = 0;
  static GC trackgc = 0;
  static int lastx = 0;
  static int lasty = 0;
  static int lastw = 0;
  static int lasth = 0;

  TRACEMSG (("fe_sash_cb\n"));

  if (!sashinfo) return;
  context = sashinfo->context;
  edge = sashinfo->edge;

  switch (sash_data->event->xany.type) {
  case ButtonPress: {
      XGCValues values;
      unsigned long valuemask;

      if (activity) return;
      activity = ButtonPressMask;

      if (!trackgc) {
	valuemask = GCForeground | GCSubwindowMode | GCFunction;
	values.foreground = CONTEXT_DATA (context)->default_bg_pixel;
	values.subwindow_mode = IncludeInferiors;
	values.function = GXinvert;
	trackgc = XCreateGC (XtDisplay (widget),
			     XtWindow (CONTEXT_WIDGET (context)),
			     valuemask, &values);
      }
    }
    break;
  case ButtonRelease:
    if (activity & PointerMotionMask) {
      static time_t last = 0;
      time_t now;

      /* Clean up the last line drawn */
      XDrawLine (XtDisplay (widget),
		   XtWindow (CONTEXT_DATA (context)->drawing_area),
		   trackgc, lastx, lasty, lastw, lasth);

      activity = 0; /* make sure we clear this for next time */

      if (trackgc) XFreeGC (XtDisplay (widget), trackgc);
      trackgc = 0;

      /* What's the scrolling policy for this context? */
      if (!edge->movable)
	return;

      /* Don't thrash */
      now = time ((time_t) 0);
      if (now > last)
        LO_MoveGridEdge (context, edge, lastx, lasty);
      last = now;

      lastx = lasty = lastw = lasth = 0;
    }
    break;
  case MotionNotify: {
      Display *dpy = XtDisplay (widget);
      Window kid;
      int da_x, da_y;

      if (!(activity & ButtonPressMask)) return;

      /* What's the scrolling policy for this context? */
      if (!edge->movable)
	return;

      /* Now that we know we're going to do something */
      activity |= PointerMotionMask;

      XTranslateCoordinates(dpy,
                            XtWindow (CONTEXT_DATA (context)->drawing_area),
                            DefaultRootWindow (dpy),
                            0, 0, &da_x, &da_y, &kid);

      if (lastw && lasth)
        XDrawLine (XtDisplay (widget),
		   XtWindow (CONTEXT_DATA (context)->drawing_area),
		   trackgc, lastx, lasty, lastw, lasth);

      if (edge->is_vertical)
        {
          int cx = sash_data->event->xmotion.x_root;

	  lastx = cx - da_x;
	  lasty = edge->y + edge->y_offset;

	  if (lastx < edge->left_top_bound + 20)
	    lastx = edge->left_top_bound + 20;

	  if (lastx > edge->right_bottom_bound - 20)
	    lastx = edge->right_bottom_bound - 20;

	  lastw = lastx;
	  lasth = lasty + edge->height - 1;
        }
      else
        {
          int cy = sash_data->event->xmotion.y_root;

	  lastx = edge->x + edge->x_offset;
	  lasty = cy - da_y;

	  if (lasty < edge->left_top_bound + 20)
	    lasty = edge->left_top_bound + 20;

	  if (lasty > edge->right_bottom_bound - 20)
	    lasty = edge->right_bottom_bound - 20;

	  lastw = lastx + edge->width - 1;
	  lasth = lasty;
        }

      XDrawLine (XtDisplay (widget),
		 XtWindow (CONTEXT_DATA (context)->drawing_area),
		 trackgc, lastx, lasty, lastw, lasth);
    }
    break;
  default:
    break;
  }
}

static void
fe_sash_destroy_cb (Widget w, XtPointer closure, XtPointer cb)
{
  SashInfo *sashinfo = (SashInfo *) closure;
  sashinfo->sash = NULL;
}

void
XFE_DisplayEdge (MWContext *context, int loc, LO_EdgeStruct *edge)
{
  long x = edge->x + edge->x_offset - CONTEXT_DATA (context)->document_x;
  long y = edge->y + edge->y_offset - CONTEXT_DATA (context)->document_y;
  Widget drawing_area = CONTEXT_DATA (context)->drawing_area;
  Widget sash;
  static XtCallbackRec sashCallback[] = { {fe_sash_cb, 0}, {0, 0} };
  SashInfo *sashinfo;
  Arg av [50];
  int ac;

  if ((x > 0 && x > CONTEXT_DATA (context)->scrolled_width) ||
      (y > 0 && y > CONTEXT_DATA (context)->scrolled_height) ||
      (x + edge->width < 0) ||
      (y + edge->height< 0))
    return;

  /* Set up the args for the sash.
   * Careful! This is the only place we initialize av.
   */
  ac = 0;
  XtSetArg (av[ac], XmNx, x); ac++;
  XtSetArg (av[ac], XmNy, y); ac++;
  XtSetArg (av[ac], XmNwidth, edge->width); ac++;
  XtSetArg (av[ac], XmNheight, edge->height); ac++;
  if (edge->bg_color) {
    Pixel color = fe_GetPixel (context,
			       edge->bg_color->red,
			       edge->bg_color->green,
			       edge->bg_color->blue);
    XtSetArg (av[ac], XmNbackground, color); ac++;
  }

  if (edge->FE_Data) {
    time_t now = time((time_t) 0);

    sashinfo = (SashInfo *) edge->FE_Data;
    if (now <= sashinfo->last) return;
    sashinfo->last = now;

    if (sashinfo->sash) {
      XtSetValues (sashinfo->sash, av, ac);
      return;
    }
      
    edge->FE_Data = NULL;
    XP_FREE (sashinfo);
  }

  /* Otherwise, create and display a new one */

  sashinfo = (SashInfo *) XP_ALLOC (sizeof (SashInfo));
  if (!sashinfo) return;

  sashCallback[0].closure = (XtPointer) sashinfo;

  /* av and ac were initialized above */
  XtSetArg (av[ac], XmNcallback, (XtArgVal) sashCallback); ac++;
  sash = XtCreateWidget("sash", xmSashWidgetClass, drawing_area, av, ac);
  if (!edge->movable)
    XtVaSetValues (sash, XmNsensitive, False, 0);

  XtAddCallback (sash, XtNdestroyCallback, fe_sash_destroy_cb,
		 (XtPointer) sashinfo);
  XtManageChild (sash);

  sashinfo->sash = sash;
  sashinfo->edge = edge;
  sashinfo->context = context;
  sashinfo->last = time((time_t) 0);

  edge->FE_Data = (void *) sashinfo;
}


void
XFE_SetBackgroundColor (MWContext *context, uint8 red, uint8 green,
			uint8 blue)
{
  Pixel bg = fe_GetPixel (context, red, green, blue);
  IL_IRGB rgb;

  CONTEXT_DATA (context)->bg_red   = rgb.red   = red;
  CONTEXT_DATA (context)->bg_green = rgb.green = green;
  CONTEXT_DATA (context)->bg_blue  = rgb.blue  = blue;

  rgb.attr = 0;
  rgb.index = bg;
  IL_SetTransparent (context, &rgb);

  CONTEXT_DATA (context)->bg_pixel = bg;

  /*
   *    Change the window color, even if there is a backdrop.
   *    The backdrop may be transparent, in which case the
   *    window color shows through. Call fe_ClearArea() to do
   *    the repaint because it knows about backdrops.
   */
#ifdef OLD_WAY
  XtVaSetValues (CONTEXT_DATA (context)->drawing_area,
		 XmNbackground, bg, 0);
#endif /* OLD_WAY */
  XSetWindowBackground (XtDisplay(CONTEXT_DATA (context)->drawing_area),
			XtWindow(CONTEXT_DATA (context)->drawing_area), bg);
  CONTEXT_DATA (context)->drawing_area->core.background_pixel = bg;

  fe_ClearArea (context, 0, 0,
		CONTEXT_DATA (context)->scrolled_width,
		CONTEXT_DATA (context)->scrolled_height);

#if 0
  /* No, don't do this - the scrolled window's background should be the
     widget background color, not the document.  See comments in scroll.c. */
  XtVaSetValues (CONTEXT_DATA (context)->scrolled,
		 XmNbackground, bg, 0);
#endif
#ifdef LEDGES
  XtVaSetValues (CONTEXT_DATA (context)->top_ledge,
		 XmNbackground, bg, 0);
  XtVaSetValues (CONTEXT_DATA (context)->bottom_ledge,
		 XmNbackground, bg, 0);
  XtVaSetValues (XtParent (CONTEXT_DATA (context)->top_ledge),
		 XmNbackground, bg, 0);
  XtVaSetValues (XtParent (CONTEXT_DATA (context)->bottom_ledge),
		 XmNbackground, bg, 0);
#endif
}


#ifdef GRIDS
void
FE_GetEdgeMinSize(MWContext *context, int32 *size_p)
{
	*size_p = 5;
}


void
FE_GetFullWindowSize(MWContext *context, int32 *width, int32 *height)
{
  Dimension w = 0, h = 0;
  Dimension extra;
  Widget widget = XtParent(CONTEXT_DATA (context)->drawing_area);

  XtUnmanageChild (CONTEXT_DATA (context)->hscroll);
  XtUnmanageChild (CONTEXT_DATA (context)->vscroll);

  if (context->is_grid_cell)
    {
      Position sx, sy;
      XtVaGetValues (CONTEXT_DATA (context)->scrolled,
	XmNwidth, &w, XmNheight, &h, 0);

      XtVaGetValues (CONTEXT_DATA (context)->vscroll,
	XmNx, &sx,
	XmNwidth, &extra, 0);
      if ((long) sx < (long) w)
	{
          *width = (int32)w;
	}
      else
	{
          *width = (int32)(w + extra);
	}
      XtVaGetValues (CONTEXT_DATA (context)->hscroll,
	XmNy, &sy,
	XmNheight, &extra, 0);
      if ((long) sy < (long) h)
	{
          *height = (int32)h;
	}
      else
	{
          *height = (int32)(h + extra);
	}

      XtVaSetValues (CONTEXT_DATA (context)->scrolled,
	XmNwidth, (Dimension)*width, XmNheight, (Dimension)*height, 0);
    }
  else
    {
      XtVaGetValues (widget, XmNwidth, &w, XmNheight, &h, 0);
      *width = (int32)w;
      *height = (int32)h;

      XtVaSetValues (CONTEXT_DATA (context)->drawing_area,
	XmNwidth, (Dimension)*width, XmNheight, (Dimension)*height, 0);
    }
}
#else
void
FE_GetFullWindowSize(MWContext *context, int32 *width, int32 *height)
{
  Dimension w = 0, h = 0;

  XtVaGetValues (CONTEXT_DATA (context)->drawing_area,
	XmNwidth, &w, XmNheight, &h, 0);
  *width = (int32)w;
  *height = (int32)h;
}
#endif /* GRIDS */


void
fe_GetMargin(MWContext *context, int32 *marginw_ptr, int32 *marginh_ptr)
{
  int32 w, h;
  if (context->is_grid_cell) {
    w = FEUNITS_X(7, context);
    h = FEUNITS_X(4, context);
  } else if (context->type == MWContextMail ||
	     context->type == MWContextNews) {
    w = FEUNITS_X(20, context);
    h = 0; /* No top margin for mail and news windows */
  } else {
    w = FEUNITS_X(20, context);
    h = FEUNITS_X(20, context);
  }

  if (marginw_ptr) *marginw_ptr = w;
  if (marginh_ptr) *marginh_ptr = h;
}


void
XFE_LayoutNewDocument (MWContext *context, URL_Struct *url,
		      int32 *iWidth, int32 *iHeight,
		       int32 *mWidth, int32 *mHeight)
{
  Dimension w = 0, h = 0;
  XColor color;
  int32 fe_mWidth, fe_mHeight;
#ifdef GRIDS
  Boolean grid_cell_p = context->is_grid_cell;
#endif /* GRIDS */

  fe_FreeTransientColors(context);
  
  color.pixel = CONTEXT_DATA (context)->default_bg_pixel;
  fe_QueryColor (context, &color);

  /* The pixmap itself is freed when its IL_Image is destroyed. */
  CONTEXT_DATA (context)->backdrop_pixmap = 0;

  /* Set background after making the backdrop_pixmap 0 as SetBackground
   * will ignore a background setting request if backdrop_pixmap is
   * available.
   */
  XFE_SetBackgroundColor (context,
			  color.red >> 8,
			  color.green >> 8,
			  color.blue >> 8);

  if (grid_cell_p) {
    if (!CONTEXT_DATA (context)->drawing_area) return;

    XtVaGetValues (CONTEXT_DATA (context)->drawing_area,
               XmNwidth, &w, XmNheight, &h, 0);
  } else {
    if (!CONTEXT_DATA (context)->scrolled) return;

    XtVaGetValues (CONTEXT_DATA (context)->scrolled,
		 XmNwidth, &w, XmNheight, &h, 0);
  }
  if (!w || !h) abort ();

  /* Subtract out the size of the scrollbars - they might not end up being
     present, but tell layout that all it has to work with is the area that
     would be available if there *were* scrollbars. */
  w -= CONTEXT_DATA (context)->sb_w;
  h -= CONTEXT_DATA (context)->sb_h;

  w -= 2;	/* No, this isn't a hack.  What makes you think that? */

  *iWidth = w;
  *iHeight = h;

  fe_GetMargin(context, &fe_mWidth, &fe_mHeight);

  /*
   * If layout already knows margin width, let it pass unless it
   * is just too big.
   */
  if (*mWidth != 0)
    {
      if (*mWidth > ((w / 2) - 1))
        *mWidth = ((w / 2) - 1);
    }
  else
    {
      *mWidth = fe_mWidth;
    }

  /*
   * If layout already knows margin height, let it pass unless it
   * is just too big.
   */
  if (*mHeight != 0)
    {
      if (*mHeight > ((h / 2) - 1))
        *mHeight = ((h / 2) - 1);
    }
  else
    {
      *mHeight = fe_mHeight;
    }

  /* Get rid of the old title; don't install "Unknown" until we've gotten
     to the end of the document without XFE_SetDocTitle() having been called.
   */
  if (context->title)
    free (context->title);
  context->title = 0;

#ifdef GRIDS
  if (!grid_cell_p)
#endif /* GRIDS */
    fe_SetURLString (context, url);

  if (url->address && !(sploosh && !strcmp (url->address, sploosh)))
    {
      if (sploosh)
	{
	  free (sploosh);
	  sploosh = 0;
	}
#ifdef GRIDS
      if (!grid_cell_p)
#endif /* GRIDS */
        fe_AddHistory (context, url, 0);
#ifdef GRIDS
      else
        SHIST_AddDocument (context, SHIST_CreateHistoryEntry (url, ""));
#endif /* GRIDS */
    }

  /* Make sure we clear the string from the previous document */
  if (context->defaultStatus) {
    XP_FREE (context->defaultStatus);
    context->defaultStatus = 0;
  }

  /* #### temporary, until we support printing GIFs that don't have HTML
     wrapped around them. */
#if 0
  if (CONTEXT_DATA (context)->print_menuitem)
    XtVaSetValues (CONTEXT_DATA (context)->print_menuitem,
		   XmNsensitive, !url->is_binary, 0);
  if (CONTEXT_DATA (context)->print_button)
    XtVaSetValues (CONTEXT_DATA (context)->print_button,
		   XmNsensitive, !url->is_binary, 0);
#endif

#ifdef LEDGES
  XFE_ClearView (context, FE_TLEDGE);
  XFE_ClearView (context, FE_BLEDGE);
#endif
  XFE_ClearView (context, FE_VIEW);
  fe_SetDocPosition (context, 0, 0);

  fe_cancel_blinkers (context);
  fe_FindReset (context);
#ifdef HAVE_SECURITY
  fe_UpdateSecurityBar (fe_GridToTopContext(context));
#endif
#ifdef GRIDS
  if (!grid_cell_p)
#endif /* GRIDS */
    fe_UpdateDocInfoDialog (context);
}


void
fe_FormatDocTitle (const char *title, const char *url, char *output, int size)
{
  if (title && !strcasecmp (title, "Untitled"))
    title = 0;  /* Losers!!! */

  if (title)
    PR_snprintf (output, size, "%.200s", title);
  else if (!url || !*url || strcmp(url, "file:///Untitled") == 0)
    {
      strcpy (output, "Untitled");
    }
  else
    {
      const char *s = (const char *) strrchr (url, '/');
      if (s)
	s++;
      else
	s = url;
      PR_snprintf (output, size, "%.200s (Untitled)", s);
    }
}


void
XFE_SetDocTitle (MWContext *context, char *title)
{
  char buf [1024];
  char buf2 [1024];
  Widget shell = CONTEXT_WIDGET (context);
  XTextProperty text_prop;
  struct fe_MWContext_cons *cntx = fe_all_MWContexts;
  char *fmt;

  if (context->type == MWContextSaveToDisk || !shell)
    return;

  /* For some context types like MWContextDialog, the shell is the parent
   * of the CONTEXT_WIDGET. In general, traverse back and get the shell.
   */
  while(!XtIsWMShell(shell) && (XtParent(shell)!=0))
    shell = XtParent(shell);

  /* We don't need to set the title for grid cells;
   * the backend sets the toplevel's title for us.
   */
  if (context->is_grid_cell)
    return;

  if (context->type == MWContextMessageComposition)
    {
      /* I18N watch */
      if (context->title) free (context->title);
      context->title = (title ? strdup (title) : 0);
      if (!title)
	title = XP_GetString( XFE_NO_SUBJECT );
      PR_snprintf (buf, sizeof(buf), "%.200s", title);
    }
  else
    {
      History_entry *he = SHIST_GetCurrent (&context->hist);
      char *url = (he && he->address ? he->address : 0);

      SHIST_SetTitleOfCurrentDoc (&context->hist, title);

      /* Just redraw the history window to show the new title. */
      fe_AddHistory (context, 0, 0);
      fe_UpdateDocInfoDialog (context);

      if (context->title) free (context->title);
      context->title = (title ? strdup (title) : 0);

      fe_FormatDocTitle (title, url, buf, 1024);
    }

  switch (context->type) {
    case MWContextMail:
	fmt = XP_GetString(XFE_MAIL_TITLE_FMT);
	break;
    case MWContextNews:
	fmt = XP_GetString(XFE_NEWS_TITLE_FMT);
	break;
    case MWContextMessageComposition:
        fmt = XP_GetString(XFE_COMPOSE);
	break;
    case MWContextEditor:
	fmt = XP_GetString(XFE_EDITOR_TITLE_FMT);
	break;
    case MWContextBrowser: 	/* FALL THROUGH */
    default:
	fmt = XP_GetString(XFE_TITLE_FMT);
  }
  PR_snprintf (buf2, sizeof (buf2), fmt, buf);

  /* We get X errors if we try to set the window title to a string with
     high-bit characters in it by doing this:

     XtVaSetValues (shell, XtNtitle, buf2, 0);
     XtVaSetValues (shell, XtNiconName, buf2, 0);

     So do it this way:
  */
  if (context->win_csid == CS_LATIN1)
    {
      text_prop.value = (unsigned char *) buf2;
      text_prop.encoding = XA_STRING;
      text_prop.format = 8;
      text_prop.nitems = strlen(buf2);
    }
  else
    {
      char *loc;
      int status;

      loc = (char *) fe_ConvertToLocaleEncoding(context->win_csid,
                                                (unsigned char *) buf2);
      status = XmbTextListToTextProperty(XtDisplay(shell), &loc, 1,
                                         XStdICCTextStyle, &text_prop);
      if (loc != buf2)
        {
          XP_FREE(loc);
        }
      if (status != Success)
        {
          text_prop.value = (unsigned char *) buf2;
          text_prop.encoding = XA_STRING;
          text_prop.format = 8;
          text_prop.nitems = strlen(buf2);
        }
    }

  XSetWMName(XtDisplay(shell), XtWindow(shell), &text_prop);

    /* Only set the icon title on browser windows - not mail, news or
       download. */
  if (context->type == MWContextBrowser)
    XSetWMIconName(XtDisplay(shell), XtWindow(shell), &text_prop);

  /* Windows menu needs regeneration whenever title is changed */
  for (; cntx; cntx = cntx->next)
    CONTEXT_DATA(cntx->context)->windows_menu_up_to_date_p = False;
}


void
fe_DestroyLayoutData (MWContext *context)
{
  LO_DiscardDocument (context);
  fe_cancel_blinkers (context);
  free (CONTEXT_DATA (context)->color_data);
}

void
XFE_FinishedLayout (MWContext *context)
{
  /* Since our processing of XFE_SetDocDimension() may have been lazy,
     do it for real this time. */
  CONTEXT_DATA (context)->doc_size_last_update_time = 0;
  XFE_SetDocDimension (context, -1,
		      CONTEXT_DATA (context)->document_width,
		      CONTEXT_DATA (context)->document_height);
  if (CONTEXT_DATA (context)->force_load_images &&
      CONTEXT_DATA (context)->force_load_images != FORCE_LOAD_ALL_IMAGES)
    free ((char *) CONTEXT_DATA (context)->force_load_images);
  CONTEXT_DATA (context)->force_load_images = 0;
#ifdef HAVE_SECURITY
  fe_UpdateSecurityBar (fe_GridToTopContext(context));
#endif
  IL_EndPage (context);
}

void
fe_ClearAreaWithColor (MWContext *context,
		       int x, int y, unsigned int w, unsigned int h,
		       unsigned long color)
{
  Widget widget = CONTEXT_DATA (context)->drawing_area;
  Display *dpy = XtDisplay (widget);

  GC gc;
  XGCValues gcv;
  memset (&gcv, ~0, sizeof (gcv));
  gcv.foreground = color;
  gc = fe_GetGC (CONTEXT_DATA (context)->drawing_area,
		     (GCForeground),
		     &gcv);
  XFillRectangle (dpy, XtWindow (widget), gc, x, y, w, h);
}

void
fe_ClearAreaWithExposures(MWContext *context,
	      int x, int y, unsigned int w, unsigned int h, Boolean exposures)
{
  Widget widget = CONTEXT_DATA (context)->drawing_area;
  Display *dpy = XtDisplay (widget);
  Window window = XtWindow(widget);
  if (CONTEXT_DATA (context)->backdrop_pixmap &&
      /* This can only happen if something went wrong while loading
	 the background pixmap, I think. */
      CONTEXT_DATA (context)->backdrop_pixmap != (Pixmap) ~0)
    {
      GC gc;
      XGCValues gcv;
      memset (&gcv, ~0, sizeof (gcv));
      gcv.fill_style = FillTiled;
      gcv.tile = CONTEXT_DATA (context)->backdrop_pixmap;
      gcv.ts_x_origin = -CONTEXT_DATA (context)->document_x;
      gcv.ts_y_origin = -CONTEXT_DATA (context)->document_y;
      gc = fe_GetGC (CONTEXT_DATA (context)->drawing_area,
		     (GCTile | GCFillStyle |
		      GCTileStipXOrigin | GCTileStipYOrigin),
		     &gcv);
      XFillRectangle (dpy, window, gc, x, y, w, h);

      if (exposures) {

	  XExposeEvent event;

	  /* generate an expose */
	  event.type = Expose;
	  event.serial = 0;
	  event.send_event = True;
	  event.display = dpy;
	  event.window = window;
	  event.x = x;
	  event.y = y;
	  event.width = w;
	  event.height = h;
	  event.count = 0;
	  XSendEvent(dpy, window, False, ExposureMask, (XEvent*)&event);
      }
    }
  else
    {
      XClearArea (dpy, window, x, y, w, h, exposures);
    }
}

void
fe_ClearArea (MWContext *context,
	      int x, int y, unsigned int w, unsigned int h)
{
    fe_ClearAreaWithExposures(context, x, y, w, h, False);
}

void
XFE_ClearView (MWContext *context, int which)
{
  if (!XtIsManaged (CONTEXT_WIDGET (context)))
    return;

  /* Clear out the data for the mouse-highlighted item.
     #### What if one ledge is being cleared but not all, and the
     highlighted item is in the other?
   */
  last_armed_xref = 0;
  last_armed_xref_highlighted_p = False;
  last_documented_xref_context = 0;
  last_documented_xref = 0;
  last_documented_anchor_data = 0;
  fe_SetCursor (context, False);

  switch (which)
    {
#ifdef LEDGES
    case FE_TLEDGE:
      XClearWindow (XtDisplay (CONTEXT_WIDGET (context)),
		    XtWindow (CONTEXT_DATA (context)->top_ledge));
      break;
    case FE_BLEDGE:
      XClearWindow (XtDisplay (CONTEXT_WIDGET (context)),
		    XtWindow (CONTEXT_DATA (context)->bottom_ledge));
      break;
#endif
    case FE_VIEW:
      fe_ClearArea (context, 0, 0,
		    /* Some random big number (but if it's too big,
		       like most-possible-short, it will make some
		       MIT R4 servers mallog excessively, sigh.) */
		    CONTEXT_WIDGET (context)->core.width * 2,
		    CONTEXT_WIDGET (context)->core.height * 2);
      break;
    default:
      abort ();
    }
}

void
XFE_BeginPreSection (MWContext *context)
{
}

void
XFE_EndPreSection (MWContext *context)
{
}

void
XFE_FreeEdgeElement (MWContext *context, LO_EdgeStruct *edge)
{
  SashInfo *sashinfo = edge->FE_Data;

  if (!sashinfo) return;

  if (sashinfo->sash) {
    XtRemoveCallback (sashinfo->sash, XtNdestroyCallback,
		      fe_sash_destroy_cb, (XtPointer) sashinfo);
    XtDestroyWidget (sashinfo->sash);
    sashinfo->sash = NULL;
  }

  edge->FE_Data = NULL;
  XP_FREE (sashinfo);
}

static void
fe_destroyEmbed(MWContext *context, NPEmbeddedApp *app)
{
  NPWindow *nWin;
  Widget embed_widget;
      
#ifdef X_PLUGINS   /* added by jwz */
  if (app) {
      nWin = app->wdata;
      embed_widget = (Widget)app->fe_data;
      if (embed_widget) XtDestroyWidget(embed_widget);
      if (nWin) {
	  if (nWin->ws_info) free(nWin->ws_info);
	  free(nWin);
      }
  }
#endif /* X_PLUGINS -- added by jwz */

  /* Reset fullpage plugin */
  CONTEXT_DATA(context)->is_fullpage_plugin = 0;
}

void
XFE_FreeEmbedElement (MWContext *context, LO_EmbedStruct *embed_struct)
{
    NPEmbeddedApp *eApp = (NPEmbeddedApp *)embed_struct->FE_Data;

    fe_destroyEmbed(context, eApp);
    NPL_EmbedDelete(context, embed_struct);
    embed_struct->FE_Data = 0;
}

void
XFE_DisplayEmbed (MWContext *context,
		  int iLocation, LO_EmbedStruct *embed_struct)
{
    NPEmbeddedApp *eApp;
    int32 xs, ys;

    if (!embed_struct) return;
    eApp = (NPEmbeddedApp *)embed_struct->FE_Data;
    if (!eApp) return;

    /* If this is a full page plugin, then plugin needs to be notified of
       the new size as relayout never happens for this when we resize.
       Our resize handler marks this context as a fullpage plugin. */
	
    if (CONTEXT_DATA(context)->is_fullpage_plugin) {
	NPWindow *nWin = (NPWindow *)eApp->wdata;

	FE_GetFullWindowSize(context, &xs, &ys);
	
#if 0
	int32 mWidth, mHeight;
	/* Normally the right thing to do is to subtract the margin width.
	 * But we wont do this and give the plugin the full html area.
	 * Remember, layout still thinks the we offset the fullpage plugin
	 * by the margin offset.
	 */
	fe_GetMargin(context, &mWidth, &mHeight);
	xs -= mWidth;
	ys -= mHeight;
#else /* 0 */
	/* In following suit with our hack of no margins for fullpage plugins
	 * we force the plugin to (0,0) position.
	 */
	XtVaSetValues((Widget)eApp->fe_data, XmNx, (Position)0,
		      XmNy, (Position)0, 0);
#endif /* 0 */

	if (nWin->width != xs || nWin->height != ys) {
	    nWin->width = xs;
	    nWin->height = ys;
	    (void)NPL_EmbedSize(eApp);
	}
    }
    else {
	/* Layout might have changed the location of the embed since we
	 * created the embed in XFE_GetEmbedSize() So change the position
	 * of the plugin. Do this only if we are not a fullpage plugin as
	 * fullpage plugins are always at (0,0).
	 */
	xs = embed_struct->x + embed_struct->x_offset -
	  CONTEXT_DATA (context)->document_x;
	ys = embed_struct->y + embed_struct->y_offset -
	  CONTEXT_DATA (context)->document_y;
	XtVaSetValues((Widget)eApp->fe_data, XmNx, (Position)xs,
		      XmNy, (Position)ys, 0);
    }

    /* Manage the embed window. XFE_GetEmbedSize() only creates the it. */
    if (!XtIsManaged((Widget)eApp->fe_data))
	XtManageChild((Widget)eApp->fe_data);
	
}

void
XFE_GetEmbedSize (MWContext *context, LO_EmbedStruct *embed_struct,
		  NET_ReloadMethod force_reload)
{
    NPEmbeddedApp *eApp = (NPEmbeddedApp *)embed_struct->FE_Data;
#ifdef MOCHA
    int32 doc_id;
    lo_TopState *top_state;

    /* here we need only decrement the number of embeds expected to load */
    doc_id = XP_DOCID(context);
    top_state = lo_FetchTopState(doc_id);
#endif /* MOCHA */

    if(!eApp)
    {
	Widget parent = CONTEXT_DATA (context)->drawing_area;
	Widget embed;
	Pixel bg;
	Window win;
	Arg av [20];
	int ac = 0;
	int xp, yp;
	int32 xs, ys;

	/* attempt to make a plugin */
#ifdef UNIX_EMBED
	if(!(eApp = NPL_EmbedCreate(context, embed_struct)))
#else
	if(1)  /* disable unix plugin's */
#endif
	{
	    /* hmm, that failed which is unusual */
	    embed_struct->width = embed_struct->height=1;
	    return;
	}
	eApp->type = NP_Plugin;

	if (embed_struct->extra_attrmask & LO_ELE_HIDDEN) {
	    /* Hidden plugin. Dont create window for it. */
	    eApp->fe_data = 0;
	    eApp->wdata = 0;
	    embed_struct->width = embed_struct->height=0;
	    (void)NPL_EmbedSize(eApp);
	    return;
	}

	/* now we have a live embed and its the first time 
	   so we need to prepare a window for it */
	xp = embed_struct->x;
	yp = embed_struct->y;
	xs = embed_struct->width;
	ys = embed_struct->height;
	
	/* Determine if this is a fullpage plugin */
	if(xs==1 && ys==1 &&
	    (embed_struct->attribute_cnt > 0) &&
	    (!strcmp(embed_struct->attribute_list[0], "src")) &&
	    (!strcmp(embed_struct->value_list[0], "internal-external-plugin")))
	{
	    /* This is a full page plugin */
	    int32 mWidth, mHeight;

	    FE_GetFullWindowSize(context, &xs, &ys);
	    fe_GetMargin(context, &mWidth, &mHeight);
	    xs -= mWidth;
	    ys -= mHeight;
	    
	    CONTEXT_DATA(context)->is_fullpage_plugin = 1;
	    xp = yp = 0;
	}

	XtVaGetValues (parent, XmNbackground, &bg, 0);
	ac = 0;
	/* 	XtSetArg (av [ac], XmNborderWidth, 1); ac++; */
	XtSetArg (av [ac], XmNx, (Position)xp); ac++;
	XtSetArg (av [ac], XmNy, (Position)yp); ac++;
	XtSetArg (av [ac], XmNwidth, (Dimension)xs); ac++;
	XtSetArg (av [ac], XmNheight, (Dimension)ys); ac++;
#ifdef X_PLUGINS
	XtSetArg (av [ac], XmNmarginWidth, 0); ac++;
	XtSetArg (av [ac], XmNmarginHeight, 0); ac++;
#endif
	XtSetArg (av [ac], XmNbackground, bg); ac++;
	embed = XmCreateDrawingArea (parent, "netscapeEmbed",
					av, ac);
	XtRealizeWidget (embed); /* create window, but don't map */
	win = XtWindow(embed);

	if (fe_globalData.fe_guffaw_scroll == 1)
	{
	    XSetWindowAttributes attr;
	    unsigned long valuemask;
	    valuemask = CWBitGravity | CWWinGravity;
	    attr.win_gravity = StaticGravity;
	    attr.bit_gravity = StaticGravity;
	    XChangeWindowAttributes (XtDisplay (embed), XtWindow (embed),
				     valuemask, &attr);
	}
	/* XtManageChild (embed); */

	/* make a plugin wininfo */
        {
	    NPWindow *nWin = (NPWindow *)malloc(sizeof(NPWindow));
	    if(nWin)
	    {
#ifdef X_PLUGINS
		NPSetWindowCallbackStruct *fe_data;
#endif /* X_PLUGINS */

		nWin->window = (void *)win;
		nWin->x = xp;
		nWin->y = yp;
		nWin->width = xs;
		nWin->height = ys;
		
#ifdef X_PLUGINS
		fe_data = (NPSetWindowCallbackStruct *)
				malloc(sizeof(NPSetWindowCallbackStruct));

		if(fe_data)
		{
		    Visual *v = 0;
		    Colormap cmap = 0;
		    Cardinal depth = 0;

		    XtVaGetValues(CONTEXT_WIDGET(context), XtNvisual, &v,
			XtNcolormap, &cmap, XtNdepth, &depth, 0);

	            fe_data->type = NP_SETWINDOW;
	            fe_data->display = (void *) XtDisplay(embed);
	            fe_data->visual = v;
	            fe_data->colormap = cmap;
	            fe_data->depth = depth;
		    nWin->ws_info = (void *) fe_data;
	        }
#endif /* X_PLUGINS */
	    }
	    eApp->wdata = nWin;
	    eApp->fe_data = (void *)embed;
	}
	embed_struct->FE_Data = (void *)eApp;

	if (NPL_EmbedStart(context, embed_struct, eApp) != NPERR_NO_ERROR) {
	    /* Spoil sport! */
	    embed_struct->FE_Data = NULL;
	    fe_destroyEmbed(context, eApp);
	    return;
	}
    }

    /* always inform plugins of size changes */
    (void)NPL_EmbedSize(eApp);
}

/************************************************************************/

#define FIX_NULL(x) ((x) ? (x) : "")

void
XFE_HideJavaAppElement(MWContext *context, void* session_data)
{
#ifdef JAVA
    LJAppletData *ad;
    Widget contextWidget;

    ad = (LJAppletData*)session_data;
    if (ad == NULL) return;
    
    contextWidget = ad->window;

    /*
    ** If we've got a context widget (window) and we're running we need
    ** to stop the applet and destroy the window.
    */
    if (contextWidget != NULL && ad->running) {
	Widget awtWindow = ad->awtWindow;

	PR_LOG(NSJAVA, warn,
	       ("stopping doc=%d, ad=%d, win=%d (%s)",
		ad->documentURL, ad, contextWidget, FIX_NULL(ad->documentURL)));
	LJ_Applet_stop(ad);

	XtUnmapWidget(contextWidget);

	if (awtWindow != NULL) {
	    /*
	    ** Reparent the awtWindow onto the root window if it was on a
	    ** grid cell because the grid will get destroyed.
	    */
	    XtUnmapWidget(awtWindow);
	    XReparentWindow(XtDisplay(awtWindow),
			    XtWindow(awtWindow),
			    RootWindowOfScreen(XtScreen(awtWindow)), 0, 0);

	    /*
	    ** Also, we need to sync the awt X connection Otherwise the
	    ** requests we just made to it will arrive after the mozilla
	    ** requests to destroy the widget hierarchy (there are two X
	    ** connections so a sync is necessary instead of just a
	    ** flush).
	    */
	    XSync(XtDisplay(awtWindow), 0);
	}
	 
	/*
	** If the context we're on is a grid cell, it's going away. But
	** we need a valid context in order to dispatch the
	** FE_FreeJavaAppElement from LJ_DeleteSessionData if we get
	** destroyed. Grab the first non-grid-cell context and use
	** it. We'll set it back to a new grid context in
	** FE_DisplayJavaApp if the context is reconstructed.
	*/
	if (context->is_grid_cell) {
	    MWContext* ctxt = context;
	    while (ctxt->is_grid_cell)
		ctxt = ctxt->grid_parent;
	    ad->context = ctxt;
	}
    
	/*
	** Destroy the window and set the pointer to null because it will
	** need to get recreated.
	*/
	XtDestroyWidget(contextWidget);
	ad->window = NULL;
	    
	ad->running = FALSE;
    }
#endif /* JAVA */
}

void
XFE_FreeJavaAppElement(MWContext *context, LJAppletData *appletData)
{
#ifdef JAVA
    Widget contextWidget;
    
    if (appletData == NULL) return;

    /*
    ** We need to free the applet before we destroy the widget, so save a
    ** copy of window before LJ_Applet_destroy destroys LJAppletData.
    */
    contextWidget = appletData->window;

    PR_LOG(NSJAVA, warn,
	   ( "deleting doc=%d, appletData=%d, win=%d (%s)",
	    appletData->documentURL, appletData, contextWidget, FIX_NULL(appletData->documentURL)));

    LJ_Applet_destroy(appletData);

    if (contextWidget != NULL) {
	XtDestroyWidget(contextWidget);
	appletData->window = NULL;
    }
#endif /* JAVA */
}

void
XFE_DisplayJavaApp(MWContext *context,
		   int iLocation, LO_JavaAppStruct *java_struct)
{
#ifdef JAVA
    Widget parent;
    Widget contextWidget;
    Pixel bg;
    Arg av [20];
    int ac = 0;
    int xp, yp;
    int32 xs, ys;
    LJAppletData* ad = (LJAppletData*)java_struct->session_data;

    if (!ad) {
	/* XXX Need broken icon for failure cases */
	java_struct->width = java_struct->height = 1;
	return;
    }

    /* now we have a live applet and its the first time 
       so we need to prepare a window for it */
    xp = java_struct->x + java_struct->x_offset
	- CONTEXT_DATA(context)->document_x;
    yp = java_struct->y + java_struct->y_offset
	- CONTEXT_DATA(context)->document_y;
    xs = java_struct->width;
    ys = java_struct->height;
    
    parent = CONTEXT_DATA(context)->drawing_area;

    /* Create the contextWidget if it doesn't exist: */
    if (ad->window == NULL) {
	/* starting applet for the first time */

	/*
	** First time in for this applet; create motif widget for it
	*/
	XtVaGetValues(parent, XmNbackground, &bg, 0);
	ac = 0;
	XtSetArg(av [ac], XmNborderWidth, 0); ac++;
	XtSetArg(av [ac], XmNx, (Position)xp); ac++;
	XtSetArg(av [ac], XmNy, (Position)yp); ac++;
	XtSetArg(av [ac], XmNwidth, (Dimension)xs); ac++;
	XtSetArg(av [ac], XmNheight, (Dimension)ys); ac++;
	XtSetArg(av [ac], XmNbackground, bg); ac++;
#ifdef DEBUG
	XtSetArg(av [ac], XmNtitle, ad->documentURL); ac++;
#endif /* DEBUG */
	contextWidget = XmCreateDrawingArea(parent,
					    (char *)java_struct->attr_name,/* XXX */
					    av, ac);
	ad->running = FALSE;
	XtSetMappedWhenManaged(contextWidget, FALSE);
	XtRealizeWidget(contextWidget); /* create window, but don't map */

	if (fe_globalData.fe_guffaw_scroll == 1)
	{
	    XSetWindowAttributes attr;
	    unsigned long valuemask;
	    valuemask = CWBitGravity | CWWinGravity;
	    attr.win_gravity = StaticGravity;
	    attr.bit_gravity = StaticGravity;
	    XChangeWindowAttributes(XtDisplay(contextWidget),
				    XtWindow(contextWidget),
				    valuemask, &attr);
	}
	XtManageChild(contextWidget);
	XSync(XtDisplay(contextWidget), 0);

	/* squirrel away the Widget */
	ad->window = (void*) contextWidget;
    }

    /*
    ** Create the applet if it doesn't exist. It might already exist if
    ** this is a grid window being reconstructed.
    */
    if (ad->awtWindow == NULL) {
	PR_LOG(NSJAVA, warn,
	       ("creating doc=%d, ad=%d, win=%d (%s)",
		ad->documentURL, ad, ad->window, FIX_NULL(ad->documentURL)));
	LJ_Applet_init(java_struct);
    }

    if (!ad->running) {
	PR_LOG(NSJAVA, warn,
	       ("starting doc=%d, ad=%d, win=%d (%s)",
		ad->documentURL, ad, ad->window, FIX_NULL(ad->documentURL)));
	if (ad->awtWindow != NULL) {
	    /*
	    ** Reparent the awtWindow onto the current window and remap
	    ** it because we may have reparented it to the root in
	    ** XFE_HideJavaAppElement to prevent it from getting
	    ** destroyed.
	    */
	    XReparentWindow(XtDisplay(ad->awtWindow),
			    XtWindow(ad->awtWindow),
			    XtWindow(contextWidget), 0, 0);
	    XtMapWidget(ad->awtWindow);

	    /*
	    ** We need to sync the awt X connection, otherwise, the
	    ** requests we just made to it will arrive after the mozilla
	    ** requests (there are two X connections so a sync is necessary
	    ** instead of just a flush).
	    */
	    XSync(XtDisplay(ad->awtWindow), 0);
	}

	XtMapWidget(ad->window);

	/*
	** We must stuff the current context into the LJAppletData
	** structure before we restart because the old one may have been
	** on a grid cell that was destroyed.
	*/
	ad->context = context;

	LJ_Applet_start(ad);
	ad->running = TRUE;
    }
    else {
	XMoveWindow(XtDisplay(ad->window), XtWindow(ad->window), xp, yp);
    }
#endif /* JAVA */
}

void
XFE_GetJavaAppSize (MWContext *context, LO_JavaAppStruct *java_struct,
		    NET_ReloadMethod force_reload)
{
#ifdef JAVA
    LJAppletData* ad = (LJAppletData*)java_struct->session_data;
    if (ad != NULL)
	ad->reloadMethod = force_reload;
#else
    /* XXX need no java supported icon */
    java_struct->width = 1;
    java_struct->height = 1;
#endif
}


void
fe_ReLayout (MWContext *context, NET_ReloadMethod force_reload)
{
  LO_Element *e = LO_XYToNearestElement (context,
					 CONTEXT_DATA (context)->document_x,
					 CONTEXT_DATA (context)->document_y);
  History_entry *he = SHIST_GetCurrent (&context->hist);
  URL_Struct *url;
  /* We must store the position into the History_entry before making
     a URL_Struct from it. */
  if (e && he)
    SHIST_SetPositionOfCurrentDoc (&context->hist, e->lo_any.ele_id);

  if (he)
    url = (force_reload == NET_RESIZE_RELOAD)
	? SHIST_CreateWysiwygURLStruct (context, he)
	: SHIST_CreateURLStructFromHistoryEntry (context, he);
  else if (sploosh)
    url = NET_CreateURLStruct (sploosh, FALSE);
  else
    url = 0;

  if (url)
    {
      if (force_reload != NET_DONT_RELOAD)
	url->force_reload = force_reload;

      /* warn plugins that the page relayout is not disasterous so that
	 it can fake caching their instances */
      if (force_reload == NET_RESIZE_RELOAD || force_reload == NET_DONT_RELOAD)
	NPL_SamePage (context);

      fe_GetURL (context, url, FALSE);
    }
}


/* Blinking.  Oh the humanity. */

static int blink_state = 0;
static int blink_time = 250;

struct fe_blinker
{
  LO_TextStruct *text;
  struct fe_blinker *next;
  int real_x, real_y;	/* in subimages, x and y change. */
};

static void
fe_blink_timer (XtPointer closure, XtIntervalId *id)
{
  MWContext *context = (MWContext *) closure;
  struct fe_blinker *b;
  for (b = CONTEXT_DATA (context)->blinkers; b; b = b->next)
    {
      int oldx = b->text->x;
      int oldy = b->text->y;
      int oldxo = b->text->x_offset;
      int oldyo = b->text->y_offset;
      b->text->x = b->real_x;
      b->text->y = b->real_y;
      b->text->x_offset = 0;
      b->text->y_offset = 0;
      fe_display_text (context, 0, b->text, 0, b->text->text_len,
		       (blink_state == 3));
      b->text->x = oldx;
      b->text->y = oldy;
      b->text->x_offset = oldxo;
      b->text->y_offset = oldyo;
    }
  blink_state = (blink_state + 1) % 4;
  CONTEXT_DATA (context)->blink_timer_id =
    XtAppAddTimeOut (fe_XtAppContext, blink_time, fe_blink_timer, context);
}


static void
fe_add_blinker (MWContext *context, LO_TextStruct *text)
{
  struct fe_blinker *b;

  for (b = CONTEXT_DATA (context)->blinkers; b; b = b->next)
    if (b->text == text)
      return;

  b = (struct fe_blinker *) malloc (sizeof (struct fe_blinker));
  b->next = CONTEXT_DATA (context)->blinkers;
  b->text = text;
  b->real_x = text->x + text->x_offset;
  b->real_y = text->y + text->y_offset;
  CONTEXT_DATA (context)->blinkers = b;
  if (! CONTEXT_DATA (context)->blink_timer_id)
    CONTEXT_DATA (context)->blink_timer_id =
      XtAppAddTimeOut (fe_XtAppContext, blink_time, fe_blink_timer, context);
}

static void
fe_cancel_blinkers (MWContext *context)
{
  struct fe_blinker *b, *n;
  b = CONTEXT_DATA (context)->blinkers;
  while (b)
    {
      n = b->next;
      free (b);
      b = n;
    }
  CONTEXT_DATA (context)->blinkers = 0;
  if (CONTEXT_DATA (context)->blink_timer_id)
    {
      XtRemoveTimeOut (CONTEXT_DATA (context)->blink_timer_id);
      CONTEXT_DATA (context)->blink_timer_id = 0;
    }
  blink_state = 0;
}


/* Following links */

/* Returns the URL string of the LO_Element, if it has one.
   Returns "" for LO_ATTR_ISFORM, which are a total kludge...
 */
static char *
fe_url_of_xref (MWContext *context, LO_Element *xref, long x, long y)
{
  struct fe_Pixmap *fep;

  switch (xref->type)
    {
    case LO_TEXT:
      if (xref->lo_text.anchor_href)
	{
          return (char *) xref->lo_text.anchor_href->anchor;
	}
      else
	{
          return (char *) NULL;
	}

    case LO_IMAGE:
      fep = (struct fe_Pixmap *) xref->lo_image.FE_Data;
      if (fep &&
          fep->type == fep_icon &&
          fep->data.icon_number == IL_IMAGE_DELAYED)
        {
          long width, height;

          fe_IconSize(IL_IMAGE_DELAYED, &width, &height);
          if (xref->lo_image.alt &&
              xref->lo_image.alt_len &&
              (x > xref->lo_image.x + xref->lo_image.x_offset + 1 + 4 +
                  width))
            {
	      if (xref->lo_image.anchor_href)
		{
		  return (char *) xref->lo_image.anchor_href->anchor;
		}
	      else
		{
		  return (char *) NULL;
		}
            }
          else
            {
              return (char *) xref->lo_image.image_url;
            }
        }
      else if (xref->lo_image.image_attr->attrmask & LO_ATTR_ISFORM)
        {
	  return "";
        }
      /*
       * This would be a client-side usemap image.
       */
      else if (xref->lo_image.image_attr->usemap_name != NULL)
        {
          LO_AnchorData *anchor_href;
	  long ix = xref->lo_image.x + xref->lo_image.x_offset;
	  long iy = xref->lo_image.y + xref->lo_image.y_offset;
	  long mx = x - ix - xref->lo_image.border_width;
	  long my = y - iy - xref->lo_image.border_width;

          anchor_href = LO_MapXYToAreaAnchor(context, (LO_ImageStruct *)xref,
							mx, my);
          if (anchor_href)
	    {
              return (char *) anchor_href->anchor;
	    }
          else
	    {
              return (char *) NULL;
	    }
        }
      else
        {
          if (xref->lo_image.anchor_href)
	    {
              return (char *) xref->lo_image.anchor_href->anchor;
	    }
          else
	    {
              return (char *) NULL;
	    }
        }

    default:
      return 0;
    }
}

void
fe_EventLOCoords (MWContext *context, XEvent *event,
		  unsigned long *x, unsigned long *y)
{
  *x = 0;
  *y = 0;

  if (event->xany.type == ButtonPress ||
      event->xany.type == ButtonRelease)
    {
      *x = event->xbutton.x;
      *y = event->xbutton.y;
    }
  else if (event->xany.type == MotionNotify)
    {
      *x = event->xmotion.x;
      *y = event->xmotion.y;
    }
  else
    abort ();

  *x += CONTEXT_DATA (context)->document_x;
  *y += CONTEXT_DATA (context)->document_y;
}

/* Returns the LO_Element under the mouse, if it is an anchor. */
static LO_Element *
fe_anchor_of_action (MWContext *context, XEvent *event)
{
  LO_Element *le;
  unsigned long x, y;
  fe_EventLOCoords (context, event, &x, &y);
  le = LO_XYToElement (context, x, y);
  if (le && !fe_url_of_xref (context, le, x, y) && (le->type != LO_EDGE))
    le = 0;
  return le;
}


void
fe_SetCursor (MWContext *context, Boolean over_link_p)
{
  Cursor c;

  if (CONTEXT_DATA (context)->save_next_mode_p)
    {
      if (over_link_p)
	c = CONTEXT_DATA (context)->save_next_link_cursor;
      else
	c = CONTEXT_DATA (context)->save_next_nonlink_cursor;
    }
  else if (CONTEXT_DATA (context)->clicking_blocked ||
	   CONTEXT_DATA (context)->synchronous_url_dialog)
    {
      c = CONTEXT_DATA (context)->busy_cursor;
    }
  else
    {
      if (over_link_p)
	c = CONTEXT_DATA (context)->link_cursor;
      else
	c = None;
    }
  if (CONTEXT_DATA (context)->drawing_area) {
    XDefineCursor (XtDisplay (CONTEXT_DATA (context)->drawing_area),
		   XtWindow (CONTEXT_DATA (context)->drawing_area),
		   c);
  }
}


void
fe_NeutralizeFocus (MWContext *context)
{
  MWContext *top_context = fe_GridToTopContext(context);

  /* Move focus out of "Location:" and back to top-level.
     Unfortunately, just focusing on the top-level doesn't do it - that
     assigns focus to its first child, which may be the URL text field,
     which is exactly what we're trying to avoid.  So, first try assigning
     focus to a few other buttons if they exist.  Only try to assign focus
     to the top level as a last resort.
   */
  if (CONTEXT_DATA (top_context)->logo &&
      XmProcessTraversal(CONTEXT_DATA (top_context)->logo, XmTRAVERSE_CURRENT))
    ;
  else if (CONTEXT_DATA (top_context)->thermo &&
	   XmProcessTraversal (CONTEXT_DATA (top_context)->thermo,
			       XmTRAVERSE_CURRENT))
    ;
  else
    XmProcessTraversal (CONTEXT_WIDGET (top_context), XmTRAVERSE_CURRENT);
}


static int click_x = -1, click_y = -1;	/* gag */
static Boolean moving = False;
static XtIntervalId auto_scroll_timer = 0;
static int fe_auto_scroll_x = 0;
static int fe_auto_scroll_y = 0;

static void
fe_auto_scroll_timer (XtPointer closure, XtIntervalId *id)
{
  MWContext *context = closure;
  int scale = 50; /* #### */
  int msecs = 10; /* #### */
  long new_x = (CONTEXT_DATA (context)->document_x +
		(scale * fe_auto_scroll_x));
  long new_y = (CONTEXT_DATA (context)->document_y +
		(scale * fe_auto_scroll_y));

  LO_ExtendSelection (context, new_x, new_y);
  fe_ScrollTo (context, (new_x > 0 ? new_x : 0), (new_y > 0 ? new_y : 0));

  auto_scroll_timer =
    XtAppAddTimeOut (fe_XtAppContext, msecs, fe_auto_scroll_timer, closure);
}

MWContext*
fe_GridToTopContext (MWContext *context)
{
  MWContext *top = context;

  XP_ASSERT (context);
  if (!context) return NULL;

  while (top && top->is_grid_cell)
    {
      top = top->grid_parent;
    }
  return top;
}

/* Invoked via a translation on <Btn1Down> and <Btn2Down>.
 */
static void
fe_arm_link_action (Widget widget, XEvent *event, String *av, Cardinal *ac)
{
  MWContext *context = fe_MotionWidgetToMWContext (widget);
  LO_Element *xref;
  unsigned long x, y;
  Time time;
  XP_ASSERT (context);
  if (!context) return;

  fe_UserActivity (context);

  fe_NeutralizeFocus (context);

  if (CONTEXT_DATA (context)->clicking_blocked ||
      CONTEXT_DATA (context)->synchronous_url_dialog)
    {
      XBell (XtDisplay (widget), 0);
      return;
    }

  xref = fe_anchor_of_action (context, event);

  time = (event && (event->type == KeyPress ||
		    event->type == KeyRelease)
	       ? event->xkey.time :
	       event && (event->type == ButtonPress ||
			 event->type == ButtonRelease)
	       ? event->xbutton.time :
	       XtLastTimestampProcessed (XtDisplay(CONTEXT_WIDGET (context))));

  fe_EventLOCoords (context, event, &x, &y);
  fe_DisownSelection (context, time, False);
  LO_StartSelection (context, x, y);

  click_x = x;
  click_y = y;
  moving = False;

#ifdef DEBUG
  if (last_armed_xref)
    fprintf (stderr,
	     "%s: ArmLink() invoked twice without intervening DisarmLink()?\n",
	     fe_progname);
#endif

  last_armed_xref = xref;
  if (xref)
    {
      LO_HighlightAnchor (context, last_armed_xref, True);
      last_armed_xref_highlighted_p = True;
    }
  else
    {
      last_armed_xref_highlighted_p = False;
    }

  if (CONTEXT_DATA (context)->save_next_mode_p)
    {
      if (! xref)
	{
	  XBell (XtDisplay (widget), 0);
	  CONTEXT_DATA (context)->save_next_mode_p = False;
	  fe_SetCursor (context, False);
	  XFE_Progress (context,
			fe_globalData.click_to_save_cancelled_message);
	}
    }
}

/* Invoked via a translation on <Btn1Up>
 */
static void
fe_disarm_link_action (Widget widget, XEvent *event, String *av, Cardinal *ac)
{
  MWContext *context = fe_MotionWidgetToMWContext (widget);
  Time time;

  if (!context) return;

  if (auto_scroll_timer)
    {
      XtRemoveTimeOut (auto_scroll_timer);
      auto_scroll_timer = 0;
    }

  fe_UserActivity (context);
  time = (event && (event->type == KeyPress ||
		    event->type == KeyRelease)
	       ? event->xkey.time :
	       event && (event->type == ButtonPress ||
			 event->type == ButtonRelease)
	       ? event->xbutton.time :
	       XtLastTimestampProcessed (XtDisplay(CONTEXT_WIDGET (context))));
  LO_EndSelection (context);
  fe_OwnSelection (context, time, False);

  if (last_armed_xref)
    {
      LO_HighlightAnchor (context, last_armed_xref, False);
    }

  last_armed_xref = 0;
  last_armed_xref_highlighted_p = False;
}

/* Invoked via a translation on <Btn1Motion>
 */
static void
fe_disarm_link_if_moved_action (Widget widget, XEvent *event,
				String *av, Cardinal *ac)
{
  MWContext *context = fe_MotionWidgetToMWContext (widget);
  LO_Element *xref = fe_anchor_of_action (context, event);
  Boolean same_xref;
  unsigned long x, y;
  XP_ASSERT (context);
  if (!context) return;
  fe_EventLOCoords (context, event, &x, &y);
  same_xref = (last_armed_xref && xref &&
	       fe_url_of_xref (context, last_armed_xref, x, y) ==
	       fe_url_of_xref (context, xref, x, y));

  if (!moving &&
      (x > click_x + CONTEXT_DATA (context)->hysteresis ||
       x < click_x - CONTEXT_DATA (context)->hysteresis ||
       y > click_y + CONTEXT_DATA (context)->hysteresis ||
       y < click_y - CONTEXT_DATA (context)->hysteresis))
    moving = True;

  if (moving &&
      !CONTEXT_DATA (context)->clicking_blocked &&
      !CONTEXT_DATA (context)->synchronous_url_dialog)
    {
      int x_region, y_region;

      if (event->xmotion.x < 0)
	x_region = -1;
      else if (event->xmotion.x > CONTEXT_DATA (context)->scrolled_width)
	x_region = 1;
      else
	x_region = 0;

      if (event->xmotion.y < 0)
	y_region = -1;
      else if (event->xmotion.y > CONTEXT_DATA (context)->scrolled_height)
	y_region = 1;
      else
	y_region = 0;

      if (last_armed_xref && last_armed_xref_highlighted_p)
	{
	  LO_HighlightAnchor (context, last_armed_xref, False);
	  last_armed_xref = 0;
	  last_armed_xref_highlighted_p = False;
	  fe_SetCursor (context, False);
	}
      LO_ExtendSelection (context, x, y);

      fe_auto_scroll_x = x_region;
      fe_auto_scroll_y = y_region;

      if ((x_region != 0 || y_region != 0) && !auto_scroll_timer)
	{
	  /* turn on the timer */
	  fe_auto_scroll_timer (context, 0);
	}
      else if ((x_region == 0 && y_region == 0) && auto_scroll_timer)
	{
	  /* cancel the timer */
	  XtRemoveTimeOut (auto_scroll_timer);
	  auto_scroll_timer = 0;
	}
    }

  if (!last_armed_xref)
    return;

  if (!same_xref && last_armed_xref_highlighted_p)
    {
      LO_HighlightAnchor (context, last_armed_xref, False);
      last_armed_xref_highlighted_p = False;
    }
  else if (same_xref && !last_armed_xref_highlighted_p)
    {
      LO_HighlightAnchor (context, last_armed_xref, True);
      last_armed_xref_highlighted_p = True;
    }
}

#ifdef DEBUG_jwz
extern void fe_other_browser (MWContext *context, URL_Struct *url);
#endif /* DEBUG_jwz */


Boolean fe_HandleHREF (MWContext *context, LO_Element *xref,
		    Boolean save_p, Boolean other_p,
		    XEvent *event, String *av, Cardinal *ac)
{
  MWContext *top = NULL;
  LO_AnchorData *anchor_data = NULL;
  Boolean link_selected_p = False;
  Boolean image_delayed_p = False;

    {
      URL_Struct *url;
      LO_FormSubmitData *data = NULL;

#ifdef MOCHA
      /* The user clicked; tell libmocha */
      CALLING_MOCHA(context);
      if (!LM_SendOnClick (context, xref))
	return False;
      if (IS_FORM_DESTROYED(context)) return False;
#endif /* MOCHA */

      if (xref->type == LO_IMAGE)
	{
	  long cx = event->xbutton.x + CONTEXT_DATA (context)->document_x;
	  long cy = event->xbutton.y + CONTEXT_DATA (context)->document_y;
	  long ix = xref->lo_image.x + xref->lo_image.x_offset;
	  long iy = xref->lo_image.y + xref->lo_image.y_offset;
	  long x = cx - ix - xref->lo_image.border_width;
	  long y = cy - iy - xref->lo_image.border_width;
	  struct fe_Pixmap *fep = (struct fe_Pixmap *) xref->lo_image.FE_Data;

          if (fep &&
		   fep->type == fep_icon &&
		   fep->data.icon_number == IL_IMAGE_DELAYED)
            {
              long width, height;

              fe_IconSize(IL_IMAGE_DELAYED, &width, &height);
              if (xref->lo_image.alt &&
                  xref->lo_image.alt_len &&
                  (event->xbutton.x > xref->lo_image.x +
                      xref->lo_image.x_offset + 1 + 4 + width))
                {
		  char *anchor = NULL;

	          if (xref->lo_image.anchor_href)
		    {
		      anchor = (char *) xref->lo_image.anchor_href->anchor;
		      anchor_data = xref->lo_image.anchor_href;
		    }
                  url = NET_CreateURLStruct (anchor, FALSE);
                }
              else
                {
                  image_delayed_p = True;
                  url = NET_CreateURLStruct ((char *) xref->lo_image.image_url,
                                         FALSE);
                }
            }
	  else if (xref->lo_image.image_attr->attrmask & LO_ATTR_ISFORM)
	    /* If this is a form image, submit it... */
	    {
	      char *action = NULL;
	      XP_Bool is_your_mocha_the_shitz_Lord_Whorfin = True;

#ifdef MOCHA
	      CALLING_MOCHA(context);
	      if (!LM_SendOnSubmit (context, (LO_Element *) &xref->lo_image))
		is_your_mocha_the_shitz_Lord_Whorfin = False;
	      if (IS_FORM_DESTROYED(context)) return (False); /* XXX */
#endif /* MOCHA */
	      if (is_your_mocha_the_shitz_Lord_Whorfin)
		{
		  data = LO_SubmitImageForm (context, &xref->lo_image, x, y);
		  if (data == NULL)
		    return True;	/* XXX ignored anyway? what is right? */

		  action = (char *) data->action;
		  url = NET_CreateURLStruct (action, FALSE);
		  NET_AddLOSubmitDataToURLStruct (data, url);
	        }
	    }
	  else if (xref->lo_image.image_attr->usemap_name != NULL)
	    /* If this is a usemap image, map the x,y to a url */
	    {
	      char *anchor = NULL;
	      LO_AnchorData *anchor_href;

              anchor_href = LO_MapXYToAreaAnchor(context,
					(LO_ImageStruct *)xref, x, y);
#ifdef MOCHA
	      /* The user clicked; tell libmocha 
	       * We need to repeat the process because previous
	       * LM_SendOnClick does not do proper operation for
	       * Anchor.
	       */
	      CALLING_MOCHA(context);
	      if (!LM_SendOnClickToAnchor(context, anchor_href))
		  return False;
	      if (IS_FORM_DESTROYED(context)) return (False);
#endif /* MOCHA */
	      if (anchor_href)
	        {
	          anchor = (char *) anchor_href->anchor;
		  anchor_data = anchor_href;
	        }
	      url = NET_CreateURLStruct (anchor, FALSE);
	    }
	  else if (xref->lo_image.image_attr->attrmask & LO_ATTR_ISMAP)
	    /* If this is an image map, append ?x?y to the URL. */
	    {
	      char *anchor = NULL;

	      if (xref->lo_image.anchor_href)
	        {
	          anchor = (char *) xref->lo_image.anchor_href->anchor;
		  anchor_data = xref->lo_image.anchor_href;
	        }
	      url = NET_CreateURLStruct (anchor, FALSE);
	      NET_AddCoordinatesToURLStruct (url, ((x < 0) ? 0 : x),
						  ((y < 0) ? 0 : y));
	    }
	  else
	    {
	      char *anchor = NULL;

	      if (xref->lo_image.anchor_href)
	        {
	          anchor = (char *) xref->lo_image.anchor_href->anchor;
		  anchor_data = xref->lo_image.anchor_href;
	        }
	      url = NET_CreateURLStruct (anchor, FALSE);
	    }
	}
      else if (xref->type == LO_TEXT)
	{
	  char *anchor = NULL;

	  if (xref->lo_text.anchor_href)
	    {
	      anchor = (char *) xref->lo_text.anchor_href->anchor;
	      anchor_data = xref->lo_text.anchor_href;
	    }
	  url = NET_CreateURLStruct (anchor, FALSE);
	}
      else if (xref->type == LO_EDGE)
	{
	  /* Nothing to do here - should we ever get here? ### */
	  ;
	}
      else
	{
	  abort ();
	}

      /* Add the referer to the URL. */
      {
	History_entry *he = SHIST_GetCurrent (&context->hist);
	if (url->referer) {
	  free (url->referer);
	  url->referer = 0;
	}
	if (he && he->address)
	  url->referer = strdup (he->address);
	else
	  url->referer = 0;
      }

#ifdef DEBUG_jwz
      if (save_p)
	{
	  /* If we're saving to disk, then always use this context for the
	     prompt dialog -- don't ever select or create another window.
	  */
	  other_p = False;
	}
      else
#endif /* DEBUG_jwz */

      if (MSG_NewWindowProhibited (context, url->address))
	{
	  XP_ASSERT (!MSG_NewWindowRequired (context, url->address));
	  other_p = False;
	}
      else if (MSG_NewWindowRequired (context, url->address))
	{
	  MWContext *new_context = 0;
	  XP_ASSERT (!MSG_NewWindowProhibited (context, url->address));

	  /* If the user has clicked left (the "open in this window" gesture)
	     on a link in a window which is not able to display that kind of
	     URL (like, clicking on an HTTP link in a mail message) then we
	     find an existing context of an appropriate type (in this case,
	     a browser window) to display it in.  If there is no window of
	     the appropriate type, of if they had used the `new window'
	     gesture, then we create a new context of the apropriate type.
	   */
	  if (other_p)
	    new_context = 0;
	  else if (MSG_RequiresMailWindow (url->address))
	    new_context = XP_FindContextOfType (context, MWContextMail);
	  else if (MSG_RequiresNewsWindow (url->address))
	    new_context = XP_FindContextOfType (context, MWContextNews);
	  else if (MSG_RequiresBrowserWindow (url->address))
	    {
	      new_context = XP_FindContextOfType (context, MWContextBrowser);
	      while (new_context && new_context->is_grid_cell)
		new_context = new_context->grid_parent;
	    }

	  if (!new_context)
	    other_p = True;
	  else
	    {
	      if (context != new_context)
		/* If we have picked an existing context that isn't this
		   one in which to display this document, make sure that
		   context is uniconified and raised first. */
		XMapRaised(XtDisplay(CONTEXT_WIDGET(new_context)),
			   XtWindow(CONTEXT_WIDGET(new_context)));
	      context = new_context;
	    }
	}

      /* Regardless of how we got here, we need to make sure and
       * and use the toplevel context if our current one is a grid
       * cell. Grid cell's don't have chrome, and our new window
       * should.
       */
      top = context;
      while (top && top->is_grid_cell)
	top = top->grid_parent;

      if (save_p)
	{
	  fe_SaveURL (context, url);
	}
      /*
       * definitely get here from middle-click, are there other ways?
       */
      else if (other_p)
	{
#ifdef DEBUG_jwz
          XP_Bool mail_attachment_p =
            (!strncmp (url->address, "mailbox:", 8) ||
             !strncmp (url->address, "mailto:",  7));

          if (! mail_attachment_p)
            {
              /* When opening a link in a new window, always use the
                 other browser, */

              fe_other_browser (context, url);

            }
          else
            {
#endif  /* DEBUG_jwz */

	  /* Need to clear it right away, or it doesn't get cleared because
	     we blast last_armed_xref from fe_ClearArea...  Sigh. */
	  fe_disarm_link_action (CONTEXT_DATA (context)->drawing_area, event, av, ac);

	  /*
	   * When we middle-click for a new window we need
	   * to ignore all window targets.  It is easy to ignore
	   * the target on the anchor here, but we also need to
	   * ignore other targets that might be set later.  We do
	   * this by setting window_target in the URL struct, but
	   * not setting a window name in the context.
	   */
	  url->window_target = strdup ("");

	  /*
	   * We no longer want to follow anchor targets from middle-clicks.
	   */
#if 0
	  /*
	   * If this link was targetted to a name window we need to either
	   * open it in that window (if it exists) or create a new window
	   * to open this link in (and assign the name to).
	   */
	  if ((anchor_data)&&(anchor_data->target))
	    {
		MWContext *target_context = XP_FindNamedContextInList(context,
						(char *)anchor_data->target);
		url->window_target = strdup ((char *)anchor_data->target);
		/*
		 * We found the named window, open this link there.
		 */
		if (target_context)
		  {
		    fe_GetURL (target_context, url, FALSE);
		  }
		/*
		 * No such named window, create one and open the link there.
		 */
		else
		  {
		    fe_MakeWindow (XtParent(CONTEXT_WIDGET (top)), top,
				   url, (char *)anchor_data->target,
				   MWContextBrowser, FALSE);
		  }
	    }
	  /*
	   * Else no target, just follow the link in a new window.
	   */
	  else
	    {
	      fe_MakeWindow (XtParent (CONTEXT_WIDGET (top)), top,
			     url, NULL, MWContextBrowser, FALSE);
	    }
#else
	  fe_MakeWindow (XtParent (CONTEXT_WIDGET (top)), top,
			     url, NULL, MWContextBrowser, FALSE);
#endif

#ifdef DEBUG_jwz
            }
#endif /* DEBUG_jwz */
	}
      else if (image_delayed_p)
        {
          fe_LoadDelayedImage (context, url->address);
          NET_FreeURLStruct (url);
        }
      /*
       * Else a normal click on a link.
       * Follow that link in this window.
       */
      else
	{
	  /*
	   * If this link was targetted to a name window we need to either
	   * open it in that window (if it exists) or create a new window
	   * to open this link in (and assign the name to).
	   *
	   * Ignore targets for ComposeWindow urls.
	   */
	  if (!MSG_RequiresComposeWindow(url->address) &&
		((anchor_data)&&(anchor_data->target)))
	    {
		MWContext *target_context = XP_FindNamedContextInList(context,
						(char *)anchor_data->target);
		/*
		 * If we copy the real target it, it will get processed
		 * again at parse time.  This is bad, because magic names
		 * like _parent return different values each time.
		 * So if we put the magic empty string here, it prevents
		 * us being overridden later, while not causing reprocessing.
		 */
		url->window_target = strdup ("");
		/*
		 * We found the named window, open this link there.
		 */
		if (target_context)
		  {
		    fe_GetURL (target_context, url, FALSE);
		  }
		/*
		 * No such named window, create one and open the link there.
		 */
		else
		  {
		    fe_MakeWindow (XtParent (CONTEXT_WIDGET (top)), top,
				   url, (char *)anchor_data->target,
				   MWContextBrowser, FALSE);
		  }
	    }
	  /*
	   * Else no target, just follow the link in this window.
	   */
	  else
	    {
	      fe_GetURL (context, url, FALSE);
	    }
	}

      if (data)
	LO_FreeSubmitData (data);

      link_selected_p = True;
    }
    return link_selected_p;
}

/* Invoked via a translation on <Btn1Up>
 */
static void
fe_activate_link_action (Widget widget, XEvent *event,
			 String *av, Cardinal *ac)
{
  MWContext *context = fe_MotionWidgetToMWContext (widget);
  LO_Element *xref = fe_anchor_of_action (context, event);
  Boolean other_p = False;
  Boolean save_p = False;
  Boolean link_selected_p = False;

  XP_ASSERT (context);
  if (!context) return;

  fe_NeutralizeFocus (context);

  if (auto_scroll_timer)
    {
      XtRemoveTimeOut (auto_scroll_timer);
      auto_scroll_timer = 0;
    }

  fe_UserActivity (context);
  if (*ac > 2)
    fprintf (stderr, "%s: too many args (%d) to ActivateLinkAction()\n",
	     fe_progname, *ac);
  else if (*ac == 1 && !strcmp ("new-window", av[0]))
    other_p = True;
  else if (*ac == 1 && !strcmp ("save-only", av[0]))
    save_p = True;
  else if (*ac > 0)
    fprintf (stderr, "%s: unknown parameter (%s) to ActivateLinkAction()\n",
	     fe_progname, av[0]);

  if (CONTEXT_DATA (context)->save_next_mode_p)
    {
      save_p = True;
      CONTEXT_DATA (context)->save_next_mode_p = False;
    }

  /* Turn off the selection cursor.  It'll be updated again at next motion. */
  fe_SetCursor (context, False);

  if (LO_HaveSelection (context))
    /* If a selection was made, don't follow the link. */
    ;

  else if (CONTEXT_DATA (context)->clicking_blocked ||
	   CONTEXT_DATA (context)->synchronous_url_dialog)
    ;

  else if (xref && xref == last_armed_xref)
    link_selected_p = fe_HandleHREF (context, xref, save_p, other_p,
					event, av, ac);

/* DONT ACCESS context AFTER A GetURL. fe_HandleHREF could do fe_GetURL. */

}

/* Invoked via a translation on <Motion>
 */
void
fe_describe_link_action (Widget widget, XEvent *event,
			 String *av, Cardinal *ac)
{
  MWContext *context = fe_MotionWidgetToMWContext (widget);
  MWContext *top = fe_GridToTopContext (context);
  LO_Element *xref = fe_anchor_of_action (context, event);
  LO_AnchorData *anchor_data = NULL;
  unsigned long x, y;
  long ix, iy, mx, my;
  fe_EventLOCoords (context, event, &x, &y);

  XP_ASSERT (context);
  if (!context) return;

  if (xref == NULL || xref != last_documented_xref  ||
      (last_documented_xref && (last_documented_xref->type == LO_IMAGE)&&
       (last_documented_xref->lo_image.image_attr->usemap_name != NULL)))
    {
      char *url = (xref ? fe_url_of_xref (context, xref, x, y) : 0);
#ifdef MOCHA
      anchor_data = NULL;
      if (xref) {
	if (last_documented_xref != xref && xref->type == LO_TEXT)
	  anchor_data = xref->lo_text.anchor_href;
	else if (xref->type == LO_IMAGE)
	  if (xref->lo_image.image_attr->usemap_name != NULL) {
	    /* Image map */
	    ix = xref->lo_image.x + xref->lo_image.x_offset;
	    iy = xref->lo_image.y + xref->lo_image.y_offset;
	    mx = x - ix - xref->lo_image.border_width;
	    my = y - iy - xref->lo_image.border_width;
	    anchor_data =
	      LO_MapXYToAreaAnchor(context, (LO_ImageStruct *)xref, mx, my);
	  }
	  else if (last_documented_xref != xref)
	    anchor_data = xref->lo_image.anchor_href;
      }
      
      /* send mouse out mocha event only if we have left a link.
       * conditions are :
       *  i) left a link to go to a non-link
       * ii) left a link to go to another link
       * iii) Moving around inside an image
       * Note: Mouse Out must happen before mouse over.
       */
      if (last_documented_anchor_data && last_documented_xref)
	if (last_documented_anchor_data != anchor_data) {
	  LM_SendMouseOutOfAnchor(last_documented_xref_context,
				  last_documented_anchor_data);
	}
#endif /* MOCHA */

      if (CONTEXT_DATA (context)->active_url_count == 0) {
	/* If there are transfers in progress, don't document the URL under
	   the mouse, since that message would interfere with the transfer
	   messages.  Do change the cursor, however. */
#ifdef MOCHA
	XP_Bool used = False;
	if (anchor_data)
	  if (anchor_data != last_documented_anchor_data) {
	    CALLING_MOCHA(context);
	    used = LM_SendMouseOverAnchor (context, anchor_data);
	    if (IS_FORM_DESTROYED(context)) return;
	  }
	  else
	    /* Dont update url too as we haven't moved to a new AREA */
	    used = True;
	if (!used)
#endif /* MOCHA */
	  fe_MidTruncatedProgress (context, (xref ? url : ""));
      }

      last_documented_xref_context = context;
      last_documented_xref = xref;
      last_documented_anchor_data = anchor_data;

      fe_SetCursor (top, !!xref);
    }
}

/* Invoked via a translation on <Btn3Down>
 */
void
fe_extend_selection_action (Widget widget, XEvent *event,
			    String *av, Cardinal *ac)
{
  MWContext *context = fe_MotionWidgetToMWContext (widget);
  Time time;
  unsigned long x, y;

  XP_ASSERT (context);
  if (!context) return;

  time = (event && (event->type == KeyPress ||
		    event->type == KeyRelease)
	  ? event->xkey.time :
	  event && (event->type == ButtonPress ||
		    event->type == ButtonRelease)
	  ? event->xbutton.time :
	  XtLastTimestampProcessed (XtDisplay(CONTEXT_WIDGET (context))));

  if (auto_scroll_timer)
    {
      XtRemoveTimeOut (auto_scroll_timer);
      auto_scroll_timer = 0;
    }

  fe_UserActivity (context);

  fe_NeutralizeFocus (context);

  fe_EventLOCoords (context, event, &x, &y);
  LO_ExtendSelection (context, x, y);
  fe_OwnSelection (context, time, False);

  /* Making a selection turns off "Save Next" mode. */
  if (CONTEXT_DATA (context)->save_next_mode_p)
    {
      XBell (XtDisplay (widget), 0);
      CONTEXT_DATA (context)->save_next_mode_p = False;
      fe_SetCursor (context, False);
      XFE_Progress (context, fe_globalData.click_to_save_cancelled_message);
    }
}



static XtActionsRec fe_mouse_actions [] =
{
  { "ArmLink",		 fe_arm_link_action },
  { "DisarmLink",	 fe_disarm_link_action },
  { "ActivateLink",	 fe_activate_link_action },
  { "DisarmLinkIfMoved", fe_disarm_link_if_moved_action },
  { "ExtendSelection",	 fe_extend_selection_action },
  { "DescribeLink",	 fe_describe_link_action }
};

void
fe_InitMouseActions ()
{
  XtAppAddActions (fe_XtAppContext, fe_mouse_actions,
		   countof (fe_mouse_actions));
}

ContextFuncs *
fe_BuildDisplayFunctionTable(void)
{
    ContextFuncs * funcs = (ContextFuncs *) malloc(sizeof(ContextFuncs));

	if(!funcs)
		return(NULL);

#define MAKE_FE_FUNCS_PREFIX(func) XFE##_##func
#define MAKE_FE_FUNCS_ASSIGN funcs->
#include "mk_cx_fn.h"

    return(funcs);
}

#ifdef NEW_FRAME_HIST
void
FE_LoadGridCellFromHistory(MWContext *context, void *hist,
			   NET_ReloadMethod force_reload)
{
#ifdef GRIDS
  History_entry *he = (History_entry *)hist;
  URL_Struct *url;

  if (! he) return;
  url = SHIST_CreateURLStructFromHistoryEntry (context, he);
  url->force_reload = force_reload;
  fe_GetURL (context, url, FALSE);
#endif /* GRIDS */
}
#endif /* NEW_FRAME_HIST */

void *
FE_FreeGridWindow(MWContext *context, XP_Bool save_history)
{
#ifdef GRIDS
  LO_Element *e;
  History_entry *he;
#ifdef NEW_FRAME_HIST
  XP_List *hist_list;

  hist_list = NULL;
#endif /* NEW_FRAME_HIST */
  he = NULL;
  if ((context)&&(context->is_grid_cell))
    {
      /* remove focus from this grid */
      CONTEXT_DATA (context)->focus_grid = False;

      /*
       * If we are going to save the history of this grid cell
       * we need to stuff the last scroll position into the
       * history structure, and then remove that structure
       * from its linked list so it won't be freed when the
       * context is destroyed.
       */
      if (save_history)
        {
          e = LO_XYToNearestElement (context,
			     CONTEXT_DATA (context)->document_x,
			     CONTEXT_DATA (context)->document_y);
          he = SHIST_GetCurrent (&context->hist);
          if (e)
            SHIST_SetPositionOfCurrentDoc (&context->hist, e->lo_any.ele_id);
#ifdef NEW_FRAME_HIST
	  LO_DiscardDocument(context);
	  hist_list = context->hist.list_ptr;
	  context->hist.list_ptr = NULL;
#else
          if (he)
	    XP_ListRemoveObject(context->hist.list_ptr, (void *)he);
#endif /* NEW_FRAME_HIST */
        }

      fe_DestroyContext(context);
    }
#ifdef NEW_FRAME_HIST
  return(hist_list);
#else
  return(he);
#endif /* NEW_FRAME_HIST */
#endif /* GRIDS */
}

