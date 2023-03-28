/*
 * @(#)awt_Font.c	1.17 95/11/27 Sami Shaio
 *
 * Copyright (c) 1994 Sun Microsystems, Inc. All Rights Reserved.
 *
 * Permission to use, copy, modify, and distribute this software
 * and its documentation for NON-COMMERCIAL purposes and without
 * fee is hereby granted provided that this copyright notice
 * appears in all copies. Please refer to the file "copyright.html"
 * for further important copyright and licensing information.
 *
 * SUN MAKES NO REPRESENTATIONS OR WARRANTIES ABOUT THE SUITABILITY OF
 * THE SOFTWARE, EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED
 * TO THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
 * PARTICULAR PURPOSE, OR NON-INFRINGEMENT. SUN SHALL NOT BE LIABLE FOR
 * ANY DAMAGES SUFFERED BY LICENSEE AS A RESULT OF USING, MODIFYING OR
 * DISTRIBUTING THIS SOFTWARE OR ITS DERIVATIVES.
 */
#include "awt_p.h"
#include "interpreter.h"
#include <string.h>
#include "java_awt_Component.h"
#include "java_awt_Font.h"
#include "java_awt_FontMetrics.h"
#include "sun_awt_motif_MToolkit.h"
#include "sun_awt_motif_X11FontMetrics.h"

#ifdef  NETSCAPE
#define UNICODE_FONT_LIST
#endif 


#ifdef UNICODE_FONT_LIST
#define USE_FTANG_FONTLIST_HACK
#endif

#ifdef USE_FTANG_FONTLIST_HACK
#include "ustring.h"
XmFontList ftang_fontlist_hack(Display *display, XFontStruct *iso1xfont, 
		long height);
#endif

/*
 * Hardwired list of mappings for generic font names "Helvetica",
 * "TimesRoman", "Courier", "Dialog", and "DialogInput".
 */
static char 	*defaultfontname = "fixed";
static char 	*defaultfoundry = "misc";
static char	*anyfoundry = "*";
static char	*anystyle = "*-*";
static char	*isolatin1 = "iso8859-1";

static int
FontName(struct Hjava_lang_String *name,
	 char **foundry, char **facename, char **encoding)
{
    char *cname;

    if (name == 0) {
	return 0;
    }
    cname = makeCString(name);

    if (strcmp(cname,"Helvetica")==0) {
	*foundry = "adobe";
	*facename = "helvetica";
	*encoding = isolatin1;
    } else if (strcmp(cname,"TimesRoman") == 0) {
	*foundry = "adobe";
	*facename = "times";
	*encoding = isolatin1;
    } else if (strcmp(cname,"Courier") == 0) {
	*foundry = "adobe";
	*facename = "courier";
	*encoding = isolatin1;
    } else if (strcmp(cname,"Dialog") == 0) {
	*foundry = "b&h";
	*facename = "lucida";
	*encoding = isolatin1;
    } else if (strcmp(cname,"DialogInput") == 0) {
	*foundry = "b&h";
	*facename = "lucidatypewriter";
	*encoding = isolatin1;
    } else if (strcmp(cname,"ZapfDingbats") == 0) {
	*foundry = "itc";
	*facename = "zapfdingbats";
	*encoding = "*-*";
    } else {
#ifdef DEBUG
	fprintf(stderr, "Unknown font: %s\n", cname);
#endif
	*foundry = defaultfoundry;
	*facename = defaultfontname;
	*encoding = isolatin1;
    }

    return 1;
}

    
static char *
Style(long s)
{
    switch (s) {
      case java_awt_Font_ITALIC:
	return "medium-i";
      case java_awt_Font_BOLD:
	return "bold-r";
      case java_awt_Font_BOLD+java_awt_Font_ITALIC:
	return "bold-i";
      case java_awt_Font_PLAIN:
      default:
	return "medium-r";
    }
}
	
static void
font_dispose(struct FontData *fdata)
{
    if (fdata == NULL)
        return;

    if (fdata->refcount > 1) {
        fdata->refcount--;
        return;
    }

    /*fprintf(stderr, "Freeing the font %x\n", (long)fdata);*/
    XFreeFont(awt_display, fdata->xfont);
#ifdef UNICODE_FONT_LIST
    XmFontListFree(fdata->xmfontlist);
#endif
    sysFree((void*)fdata);
}

struct FontData *
awt_GetFontData(struct Hjava_awt_Font *font, char **errmsg)
{
    Classjava_awt_Font	*f;
    Display		*display;
    struct FontData	*fdata;
    char		fontSpec[1024];
    char		fontKeySpec[1024];
    long		height;
    long		oheight;
    int			above = 0; /* tries above height */
    int			below = 0; /* tries below height */
    char		*foundry;
    char		*name;
    char		*encoding;
    char		*style;
    XFontStruct		*xfont;
    static struct FontData *savedFontData=NULL;
    static char	 	savedFontKey[1024];

    if (font == 0) {
	if (errmsg) {
	    *errmsg = JAVAPKG "NullPointerException";
	}
	return (struct FontData *) NULL;
    }
    display = XDISPLAY;
    f = unhand(font);

    fdata = PDATA(FontData,font);
    if (fdata != NULL && fdata->xfont != NULL) {
	return fdata;
    }

    if (!FontName(f->family, &foundry, &name, &encoding)) {
	if (errmsg) {
	    *errmsg = JAVAPKG "NullPointerException";
	}
	return (struct FontData *) NULL;
    }

    style = Style(f->style);
    oheight = height = f->size;

    /* Look for the font in the cache, if it is already in the cache, then */
    /* return it, after incrementing the count. */
    if (savedFontData != NULL) {
        jio_snprintf(fontKeySpec, sizeof(fontKeySpec), "%s-%s-%s-%d-%s",
                     foundry, name, style, height, encoding);
        if (strncmp(savedFontKey, fontKeySpec, sizeof(fontKeySpec)) == 0) {
            fdata = savedFontData;
            if (fdata->xfont != NULL) {
                fdata->refcount++;
                SET_PDATA(font, fdata);
                return fdata;
            }
        }

        font_dispose(savedFontData);
        strcpy(savedFontKey, "");
        savedFontData = NULL;
    }

    while (1) {
	jio_snprintf(fontSpec, sizeof(fontSpec), "-%s-%s-%s-*-*-%d-*-*-*-*-*-%s",
		     foundry,
		     name,
		     style,
		     height,
		     encoding);

	/*fprintf(stderr,"LoadFont: %s\n", fontSpec);*/
	xfont = XLoadQueryFont(display, fontSpec);
	/* XXX: sometimes XLoadQueryFont returns a bogus font structure */
	/* with negative ascent. */
	if (xfont == (Font) NULL || xfont->ascent < 0) {
	    if (xfont) {
		XFreeFont(display, xfont);
	    }
	    if (foundry != anyfoundry) {
		/* Try any other foundry before messing with the sizes */
		foundry = anyfoundry;
		continue;
	    }
	    /* We couldn't find the font. We'll try to find an */
	    /* alternate by searching for heights above and below our */
	    /* preferred height. We try for 4 heights above and below. */
	    /* If we still can't find a font we repeat the algorithm */
	    /* using misc-fixed as the font. If we then fail, then we */
	    /* give up and signal an error. */
	    if (above == below) {
		above++;
		height = oheight + above;
	    } else {
		below++;
		if (below > 4) {
		    if (name != defaultfontname || style != anystyle) {
			name = defaultfontname;
			foundry = defaultfoundry;
			height = oheight;
			style = anystyle;
			encoding = isolatin1;
			above = below = 0;
			continue;
		    } else {
			if (errmsg) {
			    *errmsg = "java/io/" "FileNotFoundException";
			}
			return (struct FontData *) NULL;
		    }
		}
		height = oheight - below;
	    }
	    continue;
	} else {
	    fdata = ZALLOC(FontData);

	    if (fdata == NULL) {
		if (errmsg) {
		    *errmsg = JAVAPKG "OutOfMemoryError";
		}
	    } else {
#ifdef UNICODE_FONT_LIST
#ifdef USE_FTANG_FONTLIST_HACK
           /*fdata->xmfontlist = ftang_fontlist_hack(display, xfont, oheight);*/
           fdata->xmfontlist = makeXmFontList(font, xfont);
#else
		fdata->xmfontlist = NULL;
#endif
#endif
		fdata->xfont = xfont;
		SET_PDATA(font, fdata);

		jio_snprintf(savedFontKey, sizeof(savedFontKey), 
			     "%s-%s-%s-%d-%s",
			     foundry, name, style, height, encoding);
		savedFontData = fdata;
		/* refcount is equal to 2, because one object is saved with */
                /* font object and another for the saved data */
		fdata->refcount = 2;
	    }
	    return fdata;
	}
    }
}

#ifdef UNICODE_FONT_LIST
#define max_(a,b)	(((a) > (b)) ? (a) : (b))
static 
Boolean	FillFontMetricsFromXmFontList(XmFontList fontlist ,
                                      Classsun_awt_motif_X11FontMetrics	*fm)
{
   XmFontContext context;
   XmFontListEntry entry;
	
   if(XmFontListInitFontContext(&context, fontlist) == False)
      return False;

   fm->leading = 1;
   fm->ascent = fm->descent = fm->height = 0;
   fm->maxAscent = fm->maxDescent = fm->maxHeight = fm->maxAdvance = 0;

   while((entry = XmFontListNextEntry(context)) != NULL) 
   {
      XtPointer xp;
      XmFontType fonttype;
      xp = XmFontListEntryGetFont(entry, &fonttype);
      if(fonttype == XmFONT_IS_FONT) 
      {		
         XFontStruct *xfont = xp;
         char* tag = XmFontListEntryGetTag(entry);         
         
         fm->ascent = max_(xfont->ascent, fm->ascent);
         fm->descent = max_(xfont->descent, fm->descent);
         fm->maxAscent = max_(xfont->max_bounds.ascent, fm->maxAscent);
         fm->maxDescent = max_(xfont->max_bounds.descent, fm->maxDescent);
         fm->maxAdvance = max_(xfont->max_bounds.width, fm->maxAdvance);		
         fm->height = max_(fm->height, (xfont->ascent + xfont->descent + 1));
         fm->maxHeight = max_(fm->maxHeight,
                              (xfont->max_bounds.ascent + xfont->max_bounds.descent + 1));
								

         if(strcmp(tag, "iso-8859-1") == 0)
         {
            int				ccount;
            int				i;
            int32_t			*widths;
			    
            widths = unhand(fm->widths)->body;
            widths += xfont->min_char_or_byte2;
            ccount = xfont->max_char_or_byte2 - xfont->min_char_or_byte2;
            if (xfont->per_char) {
               for (i = 0; i <= ccount; i++) {
                  *widths++ = (long) xfont->per_char[i].width;
               }
            } else {
               for (i = 0; i <= ccount; i++) {
                  *widths++ = (long) xfont->max_bounds.width;
               }
            }
         }
         XtFree(tag);
      }
#if 0
      XmFontListEntryFree(&entry); /* ftang- We probably should not free it */
#endif
   }
   XmFontListFreeFontContext(context);
   return True;
}

void
sun_awt_motif_X11FontMetrics_init(struct Hsun_awt_motif_X11FontMetrics *this)
{
    Classsun_awt_motif_X11FontMetrics	*fm;
    Classjava_awt_Font		*f;
    struct FontData		*fdata;
    int32_t			*widths;
    int				ccount;
    int				i;
    char			*err;

    fm = (Classsun_awt_motif_X11FontMetrics *)unhand(this);
    if (fm == NULL || fm->font == NULL) {
	SignalError(0, JAVAPKG "NullPointerException", 0);
	return;
    }
    AWT_LOCK();
    fdata = awt_GetFontData(fm->font, &err);
    if (fdata == NULL) {
	SignalError(0, err, 0);
	AWT_UNLOCK();
	return;
    }
    f = unhand(fm->font);

    fm->ascent = fdata->xfont->ascent;
    fm->descent = fdata->xfont->descent;
    fm->leading = 1;
    fm->height = fm->ascent + fm->descent + fm->leading;
    fm->maxAscent = fdata->xfont->max_bounds.ascent;
    fm->maxDescent = fdata->xfont->max_bounds.descent;
    fm->maxHeight = fm->maxAscent + fm->maxDescent + fm->leading;
    fm->maxAdvance = fdata->xfont->max_bounds.width;
    fm->widths = (HArrayOfInt *)ArrayAlloc(T_INT, 256);

    if (fm->widths == NULL) {
	SignalError(0, JAVAPKG "OutOfMemoryError", 0);
	AWT_UNLOCK();
	return;
    }
    widths = unhand(fm->widths)->body;
    memset(widths, 0, 256 * sizeof(int32_t));

    widths += fdata->xfont->min_char_or_byte2;
    ccount = fdata->xfont->max_char_or_byte2 - fdata->xfont->min_char_or_byte2;
    if (fdata->xfont->per_char) {
	for (i = 0; i <= ccount; i++) {
	    *widths++ = (long) fdata->xfont->per_char[i].width;
	}
    } else {
	for (i = 0; i <= ccount; i++) {
	    *widths++ = (long) fdata->xfont->max_bounds.width;
	}
    }

    AWT_UNLOCK();
}
#else
void
sun_awt_motif_X11FontMetrics_init(struct Hsun_awt_motif_X11FontMetrics *this)
{
    Classsun_awt_motif_X11FontMetrics	*fm;
    Classjava_awt_Font		*f;
    struct FontData		*fdata;
    int32_t			*widths;
    int				ccount;
    int				i;
    char			*err;

    fm = (Classsun_awt_motif_X11FontMetrics *)unhand(this);
    if (fm == NULL || fm->font == NULL) {
	SignalError(0, JAVAPKG "NullPointerException", 0);
	return;
    }
    AWT_LOCK();
    fdata = awt_GetFontData(fm->font, &err);
    if (fdata == NULL) {
	SignalError(0, err, 0);
	AWT_UNLOCK();
	return;
    }
    f = unhand(fm->font);

    fm->ascent = fdata->xfont->ascent;
    fm->descent = fdata->xfont->descent;
    fm->leading = 1;
    fm->height = fm->ascent + fm->descent + fm->leading;
    fm->maxAscent = fdata->xfont->max_bounds.ascent;
    fm->maxDescent = fdata->xfont->max_bounds.descent;
    fm->maxHeight = fm->maxAscent + fm->maxDescent + fm->leading;
    fm->maxAdvance = fdata->xfont->max_bounds.width;

    fm->widths = (HArrayOfInt *)ArrayAlloc(T_INT, 256);
    if (fm->widths == NULL) {
	SignalError(0, JAVAPKG "OutOfMemoryError", 0);
	AWT_UNLOCK();
	return;
    }
    widths = unhand(fm->widths)->body;
    memset(widths, 0, 256 * sizeof(int32_t));
    widths += fdata->xfont->min_char_or_byte2;
    ccount = fdata->xfont->max_char_or_byte2 - fdata->xfont->min_char_or_byte2;
    if (fdata->xfont->per_char) {
	for (i = 0; i <= ccount; i++) {
	    *widths++ = (long) fdata->xfont->per_char[i].width;
	}
    } else {
	for (i = 0; i <= ccount; i++) {
	    *widths++ = (long) fdata->xfont->max_bounds.width;
	}
    }
    AWT_UNLOCK();
}
#endif /* UNICODE_FONT_LIST */


void
java_awt_Font_dispose(struct Hjava_awt_Font *this)
{
    struct FontData	*fdata;

    if (this == 0) {
	SignalError(0, JAVAPKG "NullPointerException", 0);
	return;
    }
    AWT_LOCK();
    /*fprintf(stderr, "Freeing the font from java dispose\n");*/
    fdata = PDATA(FontData,this);
    if (fdata != 0) {
        SET_PDATA(this,0);
        font_dispose(fdata);
    }
    AWT_UNLOCK();
}

long
sun_awt_motif_X11FontMetrics_stringWidth(struct Hsun_awt_motif_X11FontMetrics *this,
					 struct Hjava_lang_String *str)
{
    HArrayOfChar	*data;
    unicode		*s;
    long		w;
    int			ch;
    int			cnt;
    int32_t		*widths;
    int			widlen;
    Classsun_awt_motif_X11FontMetrics	*fm;

    if (str == 0 || (data = unhand(str)->value) == 0) {
	SignalError(0, JAVAPKG "NullPointerException", 0);
	return 0;
    }
    s = unhand(data)->body + unhand(str)->offset;
    cnt = unhand(str)->count;
    fm = (Classsun_awt_motif_X11FontMetrics *)unhand(this);
    if (fm->widths) {
	w = 0;
	widths = unhand(fm->widths)->body;
	widlen = obj_length(fm->widths);
	while (--cnt >= 0) {
	    ch = *s++;
	    if (ch < widlen) {
		w += widths[ch];
	    } else {
		w += fm->maxAdvance;
	    }
	}
    } else {
	w = fm->maxAdvance * cnt;
    }

    return w;
}

/*
 * Return a bounding rectangle for the given string.  The height of the rectangle
 * is the maximum font height of the fonts actually used to display the string,
 * plus the leading.  Its not clear why leading is added but we do it to maintain
 * compatability with the getHeight() routine.  Note that the maximum font height
 * used is not necessarily the same as the height of the tallest character
 * in the string.
 */
struct Hjava_awt_Dimension *
sun_awt_motif_X11FontMetrics_stringExtent(struct Hsun_awt_motif_X11FontMetrics* this,
                                          struct Hjava_lang_String* string)
{
   Classsun_awt_motif_X11FontMetrics* fontMetrics;
   struct Hjava_awt_Dimension*        dimension;
   struct FontData*		      fdata;
   char*			      err;
   Dimension                          width, height;
   XmString                           xString;
   
   fontMetrics = (Classsun_awt_motif_X11FontMetrics *)unhand(this);
   if( fontMetrics == NULL || fontMetrics->font == NULL )
   {
      SignalError(0, JAVAPKG "NullPointerException", 0);
      return 0;
   }
   
   AWT_LOCK();
   fdata = awt_GetFontData(fontMetrics->font, &err);
   if( fdata == NULL )
   {
      SignalError(0, err, 0);
      AWT_UNLOCK();
      return 0;
   }

   xString = makeXmString( string );
   XmStringExtent( fdata->xmfontlist, xString, &width, &height );

   height += fontMetrics->leading;
   
   dimension
      = (struct Hjava_awt_Dimension *)execute_java_constructor(EE(),"java/awt/Dimension",
                                                               0, "(II)", width, height);
   AWT_UNLOCK();

   return dimension;
}

long
sun_awt_motif_X11FontMetrics_charsWidth(struct Hsun_awt_motif_X11FontMetrics *this,
					HArrayOfChar *str, long off, long len)
{
    long		w;
    unicode		*pstr;
    int			ch;
    int32_t		*widths;
    int			widlen;
    Classsun_awt_motif_X11FontMetrics	*fm;

    if (str == 0) {
	SignalError(0, JAVAPKG "NullPointerException", 0);
	return 0;
    }
    fm = (Classsun_awt_motif_X11FontMetrics *)unhand(this);
    if ((len < 0) || (off < 0) || (len + off > obj_length(str))) {
	SignalError(0, JAVAPKG "ArrayIndexOutOfBoundsException", 0);
	return 0;
    }
    if (fm->widths) {
	w = 0;
	widths = unhand(fm->widths)->body;
	widlen = obj_length(fm->widths);
	for (pstr = unhand(str)->body + off ; len ; len--) {
	    ch = *pstr++;
	    if (ch < widlen) {
		w += widths[ch];
	    } else {
		w += fm->maxAdvance;
	    }
	}
    } else {
	w = fm->maxAdvance * len;
    }

    return w;
}

long
sun_awt_motif_X11FontMetrics_bytesWidth(struct Hsun_awt_motif_X11FontMetrics *this,
					HArrayOfByte *str, long off, long len)
{
    long		w;
    unsigned char	*pstr;
    int			ch;
    int32_t		*widths;
    int			widlen;
    Classsun_awt_motif_X11FontMetrics	*fm;

    if (str == 0) {
	SignalError(0, JAVAPKG "NullPointerException", 0);
	return 0;
    }
    fm = (Classsun_awt_motif_X11FontMetrics *)unhand(this);
    if ((len < 0) || (off < 0) || (len + off > obj_length(str))) {
	SignalError(0, JAVAPKG "ArrayIndexOutOfBoundsException", 0);
	return 0;
    }
    if (fm->widths) {
	w = 0;
	widths = unhand(fm->widths)->body;
	widlen = obj_length(fm->widths);
	for (pstr = (unsigned char *)unhand(str)->body + off ; len ; len--) {
	    ch = *pstr++;
	    if (ch < widlen) {
		w += widths[ch];
	    } else {
		w += fm->maxAdvance;
	    }
	}
    } else {
	w = fm->maxAdvance * len;
    }

    return w;
}

