#include "tiny.h"
#include "java_awt_Font.h"
#include "sun_awt_tiny_TinyFontMetrics.h"

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

XFontStruct *
awt_getFont(Font *font)
{
    Classjava_awt_Font	*f;
    Display		*display;
    char		fontSpec[1024];
    long		height;
    long		oheight;
    int			above = 0; /* tries above height */
    int			below = 0; /* tries below height */
    char		*foundry;
    char		*name;
    char		*encoding;
    char		*style;
    XFontStruct		*xfont;

    if (font == 0) {
	return 0;
    }
    display = awt_display;
    f = unhand(font);

    xfont = (XFontStruct *)unhand(font)->pData;
    if (xfont) {
	return xfont;
    }

    if (!FontName(f->family, &foundry, &name, &encoding)) {
	return 0;
    }

    style = Style(f->style);
    oheight = height = f->size;

    while (1) {
	jio_snprintf(fontSpec,sizeof(fontSpec),"-%s-%s-%s-*-*-%d-*-*-*-*-*-%s",
		     foundry,
		     name,
		     style,
		     height,
		     encoding);

	/*fprintf(stderr,"LoadFont: %s\n", fontSpec);*/
	xfont = XLoadQueryFont(display, fontSpec);
	/* XXX: sometimes XLoadQueryFont returns a bogus font structure */
	/* with negative ascent. */
	if (xfont == 0 || xfont->ascent < 0) {
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
			return 0;
		    }
		}
		height = oheight - below;
	    }
	    continue;
	} else {
	    unhand(font)->pData = (long)xfont;
	    return xfont;
	}
    }
}

void 
sun_awt_tiny_TinyFontMetrics_init(TinyFontMetrics *this)
{
    Classsun_awt_tiny_TinyFontMetrics	*fm;
    XFontStruct			*xfont;
    int32_t			*widths;
    int				ccount;
    int				i;

    AWT_LOCK();
    xfont = awt_getFont(unhand(this)->font);
    fm = (Classsun_awt_tiny_TinyFontMetrics *)unhand(this);

    if (xfont == 0) {
	SignalError(0, JAVAPKG "NullPointerException", 0);
	AWT_UNLOCK();
	return;
    }

    fm->ascent = xfont->ascent;
    fm->descent = xfont->descent;
    fm->leading = 1;
    fm->height = fm->ascent + fm->descent + fm->leading;
    fm->maxAscent = xfont->max_bounds.ascent;
    fm->maxDescent = xfont->max_bounds.descent;
    fm->maxHeight = fm->maxAscent + fm->maxDescent + fm->leading;
    fm->maxAdvance = xfont->max_bounds.width;

    fm->widths = (HArrayOfInt *)ArrayAlloc(T_INT, 256);
    if (fm->widths == NULL) {
	SignalError(0, JAVAPKG "OutOfMemoryError", 0);
	AWT_UNLOCK();
	return;
    }
    widths = unhand(fm->widths)->body;
    memset(widths, 0, 256 * sizeof(int32_t));
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
    AWT_UNLOCK();
}
