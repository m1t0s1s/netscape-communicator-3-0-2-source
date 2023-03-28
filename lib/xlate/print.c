/*
** This file is mis-named.  It is the main body of code for
** dealing with PostScript translation.  It was originally
** written by Michael Toy, but now that you have read this
** comment, all bugs in this code will be automatically
** assigned to you.
*/

#if 1 /* kill 'em all */
# undef X_PLUGINS
#endif /* 1 -- kill 'em all */


#include "xlate_i.h"
#ifdef X_PLUGINS
#include "np.h"
#endif /* X_PLUGINS */
#include <libi18n.h>

PRIVATE void
default_completion(PrintSetup *p)
{
  XP_FileClose(p->out);
  p->out = NULL;
}

typedef struct {
    int32	start;
    int32	end;
    float	scale;
} InterestingPRE;

static int small_sizes[7] = { 6, 8, 10, 12, 14, 18, 24 };
static int medium_sizes[7] = { 8, 10, 12, 14, 18, 24, 36 };
static int large_sizes[7] = { 10, 12, 14, 18, 24, 36, 48 };
static int huge_sizes[7] = { 12, 14, 18, 24, 36, 48, 72 };

MODULE_PRIVATE int
scale_factor(MWContext *cx, int size, int fontmask)
{
  int bigness;

  size--;
  if (size >= 7)
    size = 6;
  if (size < 0)
    size = 0;
  if (cx->prSetup->sizes)
    return cx->prSetup->sizes[size];
  bigness = cx->prSetup->bigger;
  if (fontmask & LO_FONT_FIXED)
    bigness--;
  switch (bigness)
    {
    case -2: return small_sizes[size];
    case -1: return small_sizes[size];
    case 0: return medium_sizes[size];
    case 1: return large_sizes[size];
    case 2: return huge_sizes[size];
    }
  return 12;
}

MODULE_PRIVATE void
PSFE_SetCallNetlibAllTheTime(MWContext *cx)
{
  if (cx->prInfo->scnatt)
    (*cx->prInfo->scnatt)(NULL);
}

MODULE_PRIVATE void
PSFE_ClearCallNetlibAllTheTime(MWContext *cx)
{
  if (cx->prInfo->ccnatt)
    (*cx->prInfo->ccnatt)(NULL);
}

PUBLIC void
XL_InitializePrintSetup(PrintSetup *p)
{
    XP_BZERO(p, sizeof *p);
    p->right = p->left = .75 * 72;
    p->bottom = p->top = 72;

    p->width = 72 * 8.5;
    p->height = 72 * 11;

    p->reverse = FALSE;
    p->color = TRUE;
    p->landscape = FALSE;
    p->n_up = 1;
    p->bigger = 0;
    p->completion = default_completion;
    p->dpi = 96.0;
    p->underline = TRUE;
    p->scale_pre = TRUE;
    p->scale_images = TRUE;
    p->rules = 1.0;
}

/*
** Default completion routine for process started with NET_GetURL.
** Just save the status.  We will be called again when all connections
** associated wiht this context chut down, and there is where the
** status variable is used.
*/
PRIVATE void
ps_alldone(URL_Struct *URL_s, int status, MWContext *window_id)
{
    window_id->prSetup->status = status;
}

/*
** Called when everything is done and the whole doc has been fetched
** successfully. It draws the document one page at a time, in the order
** specified by the user.
**
** XXX Problems
**	If items fetched properly the first time, but not the second,
**	that fact is never ever passed back to the user.  This might
**	occur if, for example the doc and all it's images were bigger than
**	the caches, and the host machine went down between the first
**	and second passes.
*/
PRIVATE void xl_generate_postscript(MWContext *cx)
{
    int pn;

    /*
    ** This actually makes a second pass through the document, finding
    ** the appropriate page breaks.
    */
    XP_LayoutForPrint(cx, cx->prInfo->doc_height);

    /*
    ** Pass 3.  For each page, generate the matching postscript.
    */
    cx->prInfo->phase = XL_DRAW_PHASE;
    xl_begin_document(cx);
    for (pn = 0; pn < cx->prInfo->n_pages; pn++) {
	int page;
 	if (cx->prSetup->reverse)
	    page = cx->prInfo->n_pages - pn - 1;
	else
	    page = pn;
	xl_begin_page(cx, page);
	XP_DrawForPrint(cx, page);
	xl_end_page(cx,page);
    }
    xl_end_document(cx);
}
 
/*
** XL_TranslatePostscript:
** 	This is the main routine for postscript translation.
**
**	context - Front end context containing a function pointer table funcs,
**		   used to learn how to bonk the netlib when appropriate.
**	url - This is the URL that is to be printed.
**	pi - This is a structure describing all the options for controlling
**	     the translation process.
**
** XXX This routine should check for errors (like malloc failing) and have
** a return value which indicates whether or not a translation was actually
** going to happen.
**
** The completion routine in "pi" is called when the translation is complete.
** The value passed to the completion routine is the status code passed by
** netlib when the docuement and it's associated files are all fetched.
*/

PUBLIC void
XL_TranslatePostscript(MWContext *context, URL_Struct *url_struct, PrintSetup *pi)
{
    int nfd;
    MWContext* print_context = XP_NEW_ZAP(MWContext);
    IL_IRGB tp;
    ContextFuncs *cx_funcs = context->funcs;

    XP_BZERO(print_context, sizeof (MWContext));

    print_context->type = MWContextPostScript;

    /* inherit character set */
    print_context->doc_csid = INTL_DefaultDocCharSetID(context);
    print_context->win_csid = INTL_DocToWinCharSetID(print_context->doc_csid);

    print_context->funcs = XP_NEW(ContextFuncs);
#define MAKE_FE_FUNCS_PREFIX(f) PSFE_##f
#define MAKE_FE_FUNCS_ASSIGN print_context->funcs->
#include "mk_cx_fn.h"

    /*
    ** Coordinate space is 720 units to the inch.  Space we are converting
    ** from is "screen" coordinates, which we will assume for the sake of
    ** argument to be 72 dpi.  This is just a misguided attempt to account
    ** for fractional character placement to give more pleasing text layout.
    */
    print_context->convertPixX = INCH_TO_PAGE(1) / pi->dpi;
    print_context->convertPixY = INCH_TO_PAGE(1) / pi->dpi;
    xl_initialize_translation(print_context, pi);
    pi = print_context->prSetup;
    XP_InitializePrintInfo(print_context);
    /* Bail out if we cannot get memory for print info */
    if (print_context->prInfo == NULL)
	return;
    print_context->prInfo->phase = XL_LOADING_PHASE;
    print_context->prInfo->page_width = (pi->width - pi->left - pi->right);
    print_context->prInfo->page_height = (pi->height - pi->top - pi->bottom);
    print_context->prInfo->page_break = print_context->prInfo->page_height;
    print_context->prInfo->doc_title = XP_STRDUP(url_struct->address);
#ifdef LATER
    print_context->prInfo->interesting = NULL;
    print_context->prInfo->in_pre = FALSE;
#endif
    if (cx_funcs) {
	print_context->prInfo->scnatt = cx_funcs->SetCallNetlibAllTheTime;
	print_context->prInfo->ccnatt = cx_funcs->ClearCallNetlibAllTheTime;
    }
    
    tp.red = tp.green = tp.blue = 0xff;
    if (pi->color && pi->deep_color)
	IL_EnableTrueColor (print_context, 32, 16, 8, 0, 8, 8, 8, &tp, FALSE);
    else
	IL_EnableTrueColor (print_context, 16, 8, 4, 0, 4, 4, 4, &tp, FALSE);
    IL_DisableLowSrc (print_context);
    print_context->prSetup->url = url_struct;
    print_context->prSetup->status = -1;
    url_struct->position_tag = 0;

	/* strip off any named anchor information so that we don't
	 * truncate the document when quoting or saving
	 */
	if(url_struct->address)
		XP_STRTOK(url_struct->address, "#");

    nfd = NET_GetURL (url_struct,
	      FO_SAVE_AS_POSTSCRIPT,
	      print_context,
	      ps_alldone);
}

#ifndef max
#define max(a,b) ((a)>(b)?(a):(b))
#endif
#ifndef min
#define min(a,b) ((a)<(b)?(a):(b))
#endif

static void
measure_non_latin1_text(MWContext *cx, PS_FontInfo *f, unsigned char *cp,
	int start, int last, float sf, LO_TextInfo *text_info)
{
	int	charSize;
	int	height;
	int	i;
	int	left;
	int	right;
	int	square;
	int	width;
	int	x;
	int	y;

	width = height = left = right = x = y = 0;

	square = cx->prSetup->otherFontWidth;

	i = start;
	while (i <= last)
	{
		if ((*cp) & 0x80)
		{
			while ((i <= last) && ((*cp) & 0x80))
			{
				if (INTL_IsHalfWidth(cx, cp))
				{
					width = x + square/2;
					right = max(right, x + square*45/100);
					x += square/2;
				}
				else
				{
					width = x + square;
					right = max(right, x + square*9/10);
					x += square;
				}
				height = max(height, y + square*7/10);
				left = min(left, x + square/25);
				charSize = INTL_CharLen(cx->win_csid, cp);
				i += charSize;
				cp += charSize;
			}
		}
		else
		{
			while ((i <= last) && (!((*cp) & 0x80)))
			{
				PS_CharInfo *temp;
	
				temp = f->chars + *cp;
				width = max(width, x + temp->wx);
				height = max(height, y + temp->charBBox.ury);
				left = min(left, x + temp->charBBox.llx);
				right = max(right, x + temp->charBBox.urx);
				x += temp->wx;
				y += temp->wy;
				i++;
				cp++;
			}
		}
	}
	/*
	text_info->max_height = height * sf;
	*/
	text_info->max_width = width * sf;
	text_info->lbearing = left * sf;
	text_info->rbearing = right * sf;
}

PRIVATE void
ps_measure_text(MWContext *cx, LO_TextStruct *text,
		LO_TextInfo *text_info, int start, int last)
{
    PS_FontInfo *f;
    float sf;
    int x, y, left, right, height, width;
    int i;
    unsigned char *cp;

    assert(text->text_attr->fontmask >= 0 && text->text_attr->fontmask < N_FONTS);
    f = PSFE_MaskToFI[text->text_attr->fontmask];
    assert(f != NULL);
    /*
    ** Font info is scale by 1000, I want everything to be in points*10,
    ** so the scale factor is 1000/10 =>100
    */
    sf = scale_factor(cx, text->text_attr->size, text->text_attr->fontmask) / 100.0;

    text_info->ascent = (f->fontBBox.ury * sf);
    text_info->descent = -(f->fontBBox.lly * sf);

    width = height = left = right = x = y = 0.0;
    cp = ((unsigned char*) text->text)+start;
    if (cx->prSetup->otherFontName) {
	measure_non_latin1_text(cx, f, cp, start, last, sf, text_info);
	return;
    }
    for (i = start; i <= last; cp++, i++) {
	PS_CharInfo *temp;

	temp = f->chars + *cp;
	width = max(width, x + temp->wx);
	height = max(height, y + temp->charBBox.ury);
	left = min(left, x + temp->charBBox.llx);
	right = max(right, x + temp->charBBox.urx);
	x += temp->wx;
	y += temp->wy;
    }
    text_info->max_width = width * sf;
    text_info->lbearing = left * sf;
    text_info->rbearing = right * sf;
}

static char *months[12] =	{
  "January", "February", "March", "April", "May", "June", "July", "August",
  "September", "October", "November", "December"
};

/*
** Here's a lovely hack.  This routine draws the header or footer for a page.
*/

void
xl_annotate_page(MWContext *cx, char *template, int y, int delta_dir, int pn)
{
  float sf;
  int as, dc, ty;
  char left[300], middle[300], right[300], *bp, *fence;

#ifdef DEBUG_jwz
  int top_ink_margin = cx->prSetup->top / 2;
  int bottom_ink_margin = cx->prSetup->bottom / 2;
#endif /* !DEBUG_jwz */


  if (template == NULL)
    return;

  sf = scale_factor(cx, 1, LO_FONT_NORMAL) / 10.0;
#ifdef DEBUG_jwz
  sf /= 10.0;  /* this was WRONG!  It's 100x, not 10x! */
#endif /* !DEBUG_jwz */

  as = PSFE_MaskToFI[LO_FONT_NORMAL]->fontBBox.ury * sf;
  dc = -PSFE_MaskToFI[LO_FONT_NORMAL]->fontBBox.lly * sf;

#ifdef DEBUG_jwz
  y = cx->prInfo->page_topy;		/* move to this page's top origin */

  if (delta_dir == -1)			/* if at top, move back over tmargin */
    y -= cx->prSetup->top - top_ink_margin;
  else					/* else move over body and bmargin */
    y += cx->prInfo->page_height + cx->prSetup->bottom - bottom_ink_margin;

  ty = y;
  ty -= (as + dc) * delta_dir;		/* position bottom of font */

  y = ty - ((as + dc) * delta_dir);	/* reposition y for <HR> */
  if (delta_dir == -1) y -= as;

#else  /* !DEBUG_jwz */
  y += cx->prInfo->page_break;

  ty = y;
  if (delta_dir == 1)
  {
    y += (dc * delta_dir);
    ty = y + as + dc;
  } else {
    y -= (as + dc);
    ty = y - (dc + dc);
  }
#endif /* !DEBUG_jwz */

  bp = left;
  fence = left + sizeof left - 1;  
  *left = *middle = *right = '\0';
  while (*template != '\0')
  {
    int useit;
    int my_pn;
    
    useit = 1;
    
    my_pn = pn;
    if (*template == '%')
    {
      useit = 0;
      switch (*++template)
      {
      default:
	useit = 1;
	break;
      case '\0':
	break;
      case 'u': case 'U':
      {
	char *up;
	up = cx->prSetup->url->address;
#ifdef DEBUG_jwz
        if (!strncasecmp (up, "file:", 5))
          {
            char *s = strrchr (up, '/');
            if (!s) s = strrchr (up, '\\');
            if (!s) s = strrchr (up, ':');
            if (s) up = s+1;
          }
        else if (!strncasecmp (up, "mailbox:", 8))
          up = "";

#endif /* DEBUG_jwz */
	while (*up != '\0' && bp < fence)
	  *bp++ = *up++;
	break;
      }
      case 't': case 'T':
      {
	char *tp;
	tp = cx->prInfo->doc_title;
#ifdef DEBUG_jwz
        if (!tp || !*tp || !strcmp (tp, cx->prSetup->url->address))
          tp = "Untitled";
        else if (!strncasecmp (cx->prSetup->url->address, "mailbox:", 8))
          tp = "";
#endif /* DEBUG_jwz */
	while (*tp != '\0' && bp < fence)
	  *bp++ = *tp++;
	break;
      }
      case 'm': case 'M':
	*bp = '\0';
	bp = middle;
	fence = middle + sizeof middle - 1;
	break;
      case 'r': case 'R':
	*bp = '\0';
	bp = right;
	fence = right + sizeof right - 1;
	break;
      case 's': case 'S':
	xl_moveto(cx, 0, y);
	xl_box(cx, cx->prInfo->page_width, POINT_TO_PAGE(1));
	xl_fill(cx);
	break;
      case 'n': case 'N':
	my_pn = cx->prInfo->n_pages - 1;
      case 'p': case 'P':
      {
  
	char bf[20], *pp;
	sprintf(bf, "%d", my_pn+1);
	pp = bf;
	while (*pp && bp < fence)
	  *bp++ = *pp++;
	break;
      }	

      case 'd':
      case 'D':
      {
	time_t now;
	struct tm *tp;
	char bf[50], *dp;
	
	now = time(NULL);
	tp = localtime(&now);
#ifdef DEBUG_jwz	    /* I like a different date format than mtoy... */
	sprintf(bf, "%d-%c%c%c-%04d",
                tp->tm_mday,
		months[tp->tm_mon][0],
                months[tp->tm_mon][1],
                months[tp->tm_mon][2],
                (1900 + tp->tm_year));
#else  /* !DEBUG_jwz */
	sprintf(bf, "%s %d, %d",
		months[tp->tm_mon], tp->tm_mday, tp->tm_year+1900);
#endif /* !DEBUG_jwz */
	dp = bf;
	while (*dp && bp < fence)
	  *bp++ = *dp++;
      }
      }
      if (useit == 0 && *template != '\0')
	template++;
    }
    if (useit && bp < fence)
      *bp++ = *template++;
  }
  *bp = '\0';
  
  XP_FilePrintf(cx->prSetup->out, "%d f0 ", scale_factor(cx, 1, LO_FONT_NORMAL));
  if (*left != '\0')
  {
    xl_moveto(cx, 0, ty);
    xl_show(cx, left, strlen(left), "");
  }
  if (*middle != '\0') 
  {
    xl_moveto(cx, cx->prInfo->page_width / 2, ty);
    xl_show(cx, middle, strlen(middle), "c");
  }
  if (*right != '\0')
  {
    xl_moveto(cx, cx->prInfo->page_width, ty);
    xl_show(cx, right, strlen(right), "r");
  }
}


MODULE_PRIVATE int
PSFE_GetTextInfo(MWContext *cx, LO_TextStruct *text, LO_TextInfo *text_info)
{
    ps_measure_text(cx, text, text_info, 0, text->text_len-1);
    return 0;
}

MODULE_PRIVATE void
PSFE_LayoutNewDocument(MWContext *cx, URL_Struct *url, int32 *w, int32 *h, int32 *mw, int32* mh)
{
    *w = cx->prInfo->page_width;
    *h = cx->prInfo->page_height;
    *mw = *mh = 0;
}

MODULE_PRIVATE void
PSFE_FinishedLayout(MWContext *cx)
{
  IL_EndPage(cx);
}

static void
display_non_latin1_text(MWContext *cx, unsigned char *str, int len, int sf,
	int fontmask)
{
	int		charSize;
	int		convert;
	XP_File		f;
	unsigned char	*freeThis;
	CCCDataObject	*obj;
	unsigned char	*out;
	int		outlen;
	unsigned char	*start;

	obj = INTL_CreateCharCodeConverter();
	if (!obj)
	{
		return;
	}
	if (cx->win_csid != cx->prSetup->otherFontCharSetID)
	{
		convert = INTL_GetCharCodeConverter(cx->win_csid,
			cx->prSetup->otherFontCharSetID, obj);
	}
	else
	{
		convert = 0;
	}

	f = cx->prSetup->out;
	freeThis = NULL;

	while (len > 0)
	{
		if ((*str) & 0x80)
		{
			start = str;
			while ((len > 0) && ((*str) & 0x80))
			{
				charSize = INTL_CharLen(cx->win_csid, str);
				len -= charSize;
				str += charSize;
			}
			XP_FilePrintf(f, "%d of\n", sf);
			if (convert)
			{
				out = INTL_CallCharCodeConverter(obj, start,
					str - start);
				outlen = strlen(out);
				freeThis = out;
			}
			else
			{
				out = start;
				outlen = str - start;
			}
			XP_FilePrintf(f, "<");
			while (outlen > 0)
			{
				XP_FilePrintf(f, "%02x", *out);
				outlen--;
				out++;
			}
			XP_FilePrintf(f, "> show\n");
			if (freeThis)
			{
				free(freeThis);
				freeThis = NULL;
			}
		}
		else
		{
			start = str;
			while ((len > 0) && (!((*str) & 0x80)))
			{
				len--;
				str++;
			}
			XP_FilePrintf(f, "%d f%d\n", sf, fontmask);
			xl_show(cx, (char *) start, str - start, "");
		}
	}

	INTL_DestroyCharCodeConverter(obj);
}


MODULE_PRIVATE void
PSFE_DisplaySubtext(MWContext *cx, int iLocation, LO_TextStruct *text,
		    int32 start_pos, int32 end_pos, XP_Bool notused)
{
  int x, y;
  LO_TextInfo ti;
  int sf;

  x = text->x + text->x_offset;
  y = text->y + text->y_offset;
  ps_measure_text(cx, text, &ti, 0, start_pos-1);
  y += ti.ascent;		/* Move to baseline */
  x += ti.max_width;		/* Skip over un-displayed text */
  if (!XP_CheckElementSpan(cx, (LO_Any*) text))
    return;

  sf = scale_factor(cx, text->text_attr->size, text->text_attr->fontmask);
  xl_moveto(cx, x, y);
  if (cx->prSetup->otherFontName) {
    display_non_latin1_text(cx, ((unsigned char *) (text->text)) + start_pos,
			    end_pos - start_pos + 1, sf,
			    (int) text->text_attr->fontmask);
    return;
  }
  XP_FilePrintf(cx->prSetup->out, "%d f%d\n", sf,
		(int) text->text_attr->fontmask);
  xl_show(cx, ((char*) text->text) + start_pos, end_pos - start_pos + 1,"");
}

MODULE_PRIVATE void
PSFE_DisplayText(MWContext *cx, int iLocation, LO_TextStruct *text, XP_Bool b)
{
    (cx->funcs->DisplaySubtext)(cx, iLocation, text, 0, text->text_len-1, b);
}

/*
** Called at the end of each "Line" which could be a tiny line-section
** inside of a table cell or subdoc.
*/
MODULE_PRIVATE void
PSFE_DisplayLineFeed(MWContext *cx, int iLocation, LO_LinefeedStruct *line_feed,
		     XP_Bool b)
{
#ifdef NOT_THIS_TIME
    if (cx->prInfo->layout_phase) {
	if (cx->prInfo->in_pre) {
	    if (cx->prInfo->pre_start == -1)
		cx->prInfo->pre_start = line_feed->y;
	    cx->prInfo->pre_end = line_feed->y+line_feed->line_height;
	}
	emit_y(cx, line_feed->y, line_feed->y+line_feed->line_height);
    } else if (cx->prInfo->interesting != NULL) {
	InterestingPRE *p;
	p = (InterestingPRE *) XP_ListPeekTopObject(cx->prInfo->interesting);
	if (p == NULL) {
	    XP_DELETE(cx->prInfo->interesting);
	    cx->prInfo->interesting = NULL;
	} else if (line_feed->y + line_feed->line_height >= p->end) {
	    (void) XP_ListRemoveTopObject(cx->prInfo->interesting);
	    XP_DELETE(p);
	}
    }
#endif
}

MODULE_PRIVATE void
PSFE_DisplayHR(MWContext *cx, int iLocation , LO_HorizRuleStruct *HR)
{
  int delta;
  int top, bottom;

  if (!XP_CheckElementSpan(cx, (LO_Any*) HR))
    return;
  delta = (HR->height - HR->height*cx->prSetup->rules) / 2;
  top = HR->y + HR->y_offset + delta;
  bottom = top + HR->height*cx->prSetup->rules;


  xl_moveto(cx, HR->x+HR->x_offset, top);
  xl_box(cx, HR->width, HR->height*cx->prSetup->rules);
  xl_fill(cx);
}

MODULE_PRIVATE void
PSFE_DisplayBullet(MWContext *cx, int iLocation, LO_BullettStruct *bullet)
{
  int top;

  if (!XP_CheckElementSpan(cx, (LO_Any*) bullet))
    return;
  top = bullet->y + bullet->y_offset;

  switch (bullet->bullet_type) {
    case BULLET_ROUND:
      xl_moveto(cx,
		 bullet->x+bullet->x_offset+bullet->height/2,
		 top+bullet->height/2);
      xl_circle(cx, bullet->width, bullet->height);
      xl_stroke(cx);
      break;
    case BULLET_BASIC:
      xl_moveto(cx,
		 bullet->x+bullet->x_offset+bullet->height/2,
		 top+bullet->height/2);
      xl_circle(cx, bullet->width, bullet->height);
      xl_fill(cx);
      break;
    case BULLET_SQUARE:
      xl_moveto(cx, bullet->x+bullet->x_offset, top);
      xl_box(cx, bullet->width, bullet->height);
      xl_fill(cx);
      break;
#ifdef DEBUG
      default:
	  XP_Trace("Strange bullet type %d", bullet->bullet_type);
#endif
  }
}

PUBLIC void PSFE_AllConnectionsComplete(MWContext *cx) 
{
      /*
      ** All done, time to let everything go.
      */
  if (cx->prSetup->status >= 0)
    xl_generate_postscript(cx);
  (*cx->prSetup->completion)(cx->prSetup);
  LO_DiscardDocument(cx);
  IL_DeleteImages(cx);
  xl_finalize_translation(cx);
  XP_FREE(cx->prInfo->doc_title);
  XP_CleanupPrintInfo(cx);
  XP_DELETE(cx->funcs);
  XP_DELETE(cx);
}

MODULE_PRIVATE void
PSFE_DisplayFormElement(MWContext *cx, int loc,LO_FormElementStruct *element)
{
}

MODULE_PRIVATE void
PSFE_SetDocDimension(MWContext *cx, int iloc, int32 iWidth, int32 iLength)
{
  cx->prInfo->doc_width = iWidth;
  cx->prInfo->doc_height = iLength;
}

MODULE_PRIVATE void
PSFE_SetDocTitle(MWContext * cx, char * title)
{
    XP_FREE(cx->prInfo->doc_title);
    cx->prInfo->doc_title = XP_STRDUP(title);
}

MODULE_PRIVATE char *
PSFE_TranslateISOText(MWContext *cx, int charset, char *ISO_Text)
{
    return ISO_Text;
}

MODULE_PRIVATE void
PSFE_BeginPreSection(MWContext* cx)
{
#ifdef LATER
    cx->prInfo->scale = 1.0;
    cx->prInfo->in_pre = TRUE;
    cx->prInfo->pre_start = -1;
#endif
}

MODULE_PRIVATE void
PSFE_EndPreSection(MWContext* cx)
{
#ifdef LATER
    if (cx->prInfo->layout_phase && cx->prInfo->scale != 1.0) {
	InterestingPRE *p = XP_NEW(InterestingPRE);
	p->start = cx->prInfo->pre_start;
	p->end = cx->prInfo->pre_end;
	p->scale = max(cx->prInfo->scale, 0.25);
	if (cx->prInfo->interesting == NULL)
	    cx->prInfo->interesting = XP_ListNew();
	XP_ListAddObjectToEnd(cx->prInfo->interesting, p);
    }
    cx->prInfo->in_pre = FALSE;
#endif
}

void PSFE_DisplayTable(MWContext *cx, int iLoc, LO_TableStruct *table)
{
  if (table->border_width != 0 && XP_CheckElementSpan(cx, (LO_Any*) table)) {
    int y = table->y+table->y_offset;
    xl_draw_border(cx, table->x+table->x_offset, y,
		    table->width, table->height, table->border_width);
  }
}

void PSFE_DisplayCell(MWContext *cx, int iLoc,LO_CellStruct *cell)
{
  if (cell->border_width != 0 && XP_CheckElementSpan(cx, (LO_Any*) cell)) {
    int y = cell->y+cell->y_offset;
    xl_draw_border(cx, cell->x+cell->x_offset, y,
		    cell->width, cell->height, cell->border_width);
  }
}

MODULE_PRIVATE char *
PSFE_PromptPassword(MWContext *cx, const char *msg)
{
  if (cx->prSetup->cx != NULL)
    return FE_PromptPassword(cx->prSetup->cx, msg);

  return NULL;
}


/***************************
 * Plugin printing support *
 ***************************/
#ifndef X_PLUGINS
/* Plugins are disabled */
void PSFE_GetEmbedSize(MWContext *context, LO_EmbedStruct *embed_struct, NET_ReloadMethod force_reload) {}
void PSFE_FreeEmbedElement(MWContext *context, LO_EmbedStruct *embed_struct) {}
void PSFE_DisplayEmbed(MWContext *context, int iLocation, LO_EmbedStruct *embed_struct) {}
#else
void
PSFE_FreeEmbedElement (MWContext *context, LO_EmbedStruct *embed_struct)
{
    NPL_EmbedDelete(context, embed_struct);
    embed_struct->FE_Data = 0;
}

void
PSFE_DisplayEmbed (MWContext *context,
		  int iLocation, LO_EmbedStruct *embed_struct)
{
    NPPrintCallbackStruct npPrintInfo;
    NPEmbeddedApp *eApp;
    NPPrint npprint;

    if (!embed_struct) return;
    eApp = (NPEmbeddedApp *)embed_struct->FE_Data;
    if (!eApp) return;

    npprint.mode = NP_EMBED;
    npprint.print.embedPrint.platformPrint = NULL;
    npprint.print.embedPrint.window.window = NULL;
    npprint.print.embedPrint.window.x =
      embed_struct->x + embed_struct->x_offset;
    npprint.print.embedPrint.window.y =
      embed_struct->y + embed_struct->y_offset;
    npprint.print.embedPrint.window.width = embed_struct->width;
    npprint.print.embedPrint.window.height = embed_struct->height;
    
    npPrintInfo.type = NP_PRINT;
    npPrintInfo.fp = context->prSetup->out;
    npprint.print.embedPrint.platformPrint = (void *) &npPrintInfo;

    NPL_Print(eApp, &npprint);	
}

void
PSFE_GetEmbedSize (MWContext *context, LO_EmbedStruct *embed_struct,
		  NET_ReloadMethod force_reload)
{
  NPEmbeddedApp *eApp = (NPEmbeddedApp *)embed_struct->FE_Data;
  
  if(eApp) return;

  /* attempt to make a plugin */
  if(!(eApp = NPL_EmbedCreate(context, embed_struct)) ||
     (embed_struct->extra_attrmask & LO_ELE_HIDDEN)) {
    return;
  }

  /* Determine if this is a fullpage plugin */
  if ((embed_struct->attribute_cnt > 0) &&
      (!strcmp(embed_struct->attribute_list[0], "src")) &&
      (!strcmp(embed_struct->value_list[0], "internal-external-plugin")))
    {
      embed_struct->width = context->prInfo->page_width;
      embed_struct->height = context->prInfo->page_height;
    }

  embed_struct->FE_Data = (void *)eApp;

  if (NPL_EmbedStart(context, embed_struct, eApp) != NPERR_NO_ERROR) {
    /* Spoil sport! */
    embed_struct->FE_Data = NULL;
    return;
  }
}
#endif /* !X_PLUGINS */
