#include "xlate_i.h"
#include "isotab.c"
#include "locale.h"

#define RED_PART 0.299		/* Constants for converting RGB to greyscale */
#define GREEN_PART 0.587
#define BLUE_PART 0.114

#define XL_SET_NUMERIC_LOCALE() char* cur_locale = setlocale(LC_NUMERIC, "C")
#define XL_RESTORE_NUMERIC_LOCALE() setlocale(LC_NUMERIC, cur_locale)

/*
** These two functions swap values around in order to deal with page
** rotation.
*/

void xl_initialize_translation(MWContext *cx, PrintSetup* pi)
{
  PrintSetup *dup = XP_NEW(PrintSetup);
  *dup = *pi;
  cx->prSetup = dup;
  dup->width = POINT_TO_PAGE(dup->width);
  dup->height = POINT_TO_PAGE(dup->height);
  dup->top = POINT_TO_PAGE(dup->top);
  dup->left = POINT_TO_PAGE(dup->left);
  dup->bottom = POINT_TO_PAGE(dup->bottom);
  dup->right = POINT_TO_PAGE(dup->right);
  if (pi->landscape)
  {
    dup->height = POINT_TO_PAGE(pi->width);
    dup->width = POINT_TO_PAGE(pi->height);
    /* XXX Should I swap the margins too ??? */
  }	
}

void xl_finalize_translation(MWContext *cx)
{
    XP_DELETE(cx->prSetup);
    cx->prSetup = NULL;
}

void xl_begin_document(MWContext *cx)
{
    int i;
    XP_File f;

    f = cx->prSetup->out;
    XP_FilePrintf(f, "%%!PS-Adobe-3.0\n");
#ifdef DEBUG_jwz
    /* jwz: was excluding top and bottom margins */
    {
      int left = cx->prSetup->left;
      int right = cx->prSetup->right;
      int top = cx->prSetup->top;
      int bot = cx->prSetup->bottom;

      /* if there is a page header, leave room for it in the bounding box. */
      /* #### hardcodes assumption that only the inner half of the margin
         #### area is actually used.  See xl_annotate_page(). */
      if (cx->prSetup->header && *cx->prSetup->header)
        top = top / 2;
      /* if there is a page footer, leave room for it in the bounding box. */
      if (cx->prSetup->footer && *cx->prSetup->footer)
        bot = bot / 2;

      /* BoundingBox takes llx lly urx ury */
      XP_FilePrintf(f, "%%%%BoundingBox: %d %d %d %d\n",
                    PAGE_TO_POINT_I(left),
                    PAGE_TO_POINT_I(bot),
                    PAGE_TO_POINT_I(cx->prSetup->width - right),
                    PAGE_TO_POINT_I(cx->prSetup->height - top));
    }
#else  /* !DEBUG_jwz */
    XP_FilePrintf(f, "%%%%BoundingBox: %d %d %d %d\n",
        PAGE_TO_POINT_I(cx->prSetup->left),
	PAGE_TO_POINT_I(cx->prSetup->bottom),
	PAGE_TO_POINT_I(cx->prSetup->width-cx->prSetup->right),
	PAGE_TO_POINT_I(cx->prSetup->height-cx->prSetup->top));
#endif /* !DEBUG_jwz */
    XP_FilePrintf(f, "%%%%Creator: Mozilla (NetScape) HTML->PS\n");
    XP_FilePrintf(f, "%%%%DocumentData: Clean7Bit\n");
    XP_FilePrintf(f, "%%%%Orientation: %s\n",
        (cx->prSetup->width < cx->prSetup->height) ? "Portrait" : "Landscape");
    XP_FilePrintf(f, "%%%%Pages: %d\n", (int) cx->prInfo->n_pages);
    if (cx->prSetup->reverse)
	XP_FilePrintf(f, "%%%%PageOrder: Descend\n");
    else
	XP_FilePrintf(f, "%%%%PageOrder: Ascend\n");
    XP_FilePrintf(f, "%%%%Title: %s\n", cx->prInfo->doc_title);
#ifdef NOTYET
    XP_FilePrintf(f, "%%%%For: %n", user_name_stuff);
#endif
    XP_FilePrintf(f, "%%%%EndComments\n");
    
    XP_FilePrintf(f, "%%%%BeginProlog\n");
    XP_FilePrintf(f, "[");
    for (i = 0; i < 256; i++)
      {
	if (*isotab[i] == '\0')
	  XP_FilePrintf(f, " /.notdef");
	else
	  XP_FilePrintf(f, " /%s", isotab[i]);
	if (( i % 10) == 9)
	  XP_FilePrintf(f, "\n");
      }
    XP_FilePrintf(f, "] /isolatin1encoding exch def\n");
    XP_FilePrintf(f, "/c { matrix currentmatrix currentpoint translate\n");
    XP_FilePrintf(f, "     3 1 roll scale newpath 0 0 1 0 360 arc setmatrix } bind def\n");
    for (i = 0; i < N_FONTS; i++)
	XP_FilePrintf(f, 
	    "/F%d\n"
	    "    /%s findfont\n"
	    "    dup length dict begin\n"
	    "	{1 index /FID ne {def} {pop pop} ifelse} forall\n"
	    "	/Encoding isolatin1encoding def\n"
	    "    currentdict end\n"
	    "definefont pop\n"
	    "/f%d { /F%d findfont exch scalefont setfont } bind def\n",
		i, PSFE_MaskToFI[i]->name, i, i);
    if (cx->prSetup->otherFontName)
      XP_FilePrintf(f, "/of { /%s findfont exch scalefont "
		     "setfont } bind def\n", cx->prSetup->otherFontName);
    XP_FilePrintf(f, "/rhc {\n");
    XP_FilePrintf(f, "    {\n");
    XP_FilePrintf(f, "        currentfile read {\n");
    XP_FilePrintf(f, "	    dup 97 ge\n");
    XP_FilePrintf(f, "		{ 87 sub true exit }\n");
    XP_FilePrintf(f, "		{ dup 48 ge { 48 sub true exit } { pop } ifelse }\n");
    XP_FilePrintf(f, "	    ifelse\n");
    XP_FilePrintf(f, "	} {\n");
    XP_FilePrintf(f, "	    false\n");
    XP_FilePrintf(f, "	    exit\n");
    XP_FilePrintf(f, "	} ifelse\n");
    XP_FilePrintf(f, "    } loop\n");
    XP_FilePrintf(f, "} bind def\n");
    XP_FilePrintf(f, "\n");
    XP_FilePrintf(f, "/cvgray { %% xtra_char npix cvgray - (string npix long)\n");
    XP_FilePrintf(f, "    dup string\n");
    XP_FilePrintf(f, "    0\n");
    XP_FilePrintf(f, "    {\n");
    XP_FilePrintf(f, "	rhc { cvr 4.784 mul } { exit } ifelse\n");
    XP_FilePrintf(f, "	rhc { cvr 9.392 mul } { exit } ifelse\n");
    XP_FilePrintf(f, "	rhc { cvr 1.824 mul } { exit } ifelse\n");
    XP_FilePrintf(f, "	add add cvi 3 copy put pop\n");
    XP_FilePrintf(f, "	1 add\n");
    XP_FilePrintf(f, "	dup 3 index ge { exit } if\n");
    XP_FilePrintf(f, "    } loop\n");
    XP_FilePrintf(f, "    pop\n");
    XP_FilePrintf(f, "    3 -1 roll 0 ne { rhc { pop } if } if\n");
    XP_FilePrintf(f, "    exch pop\n");
    XP_FilePrintf(f, "} bind def\n");
    XP_FilePrintf(f, "\n");
    XP_FilePrintf(f, "/smartimage12rgb { %% w h b [matrix] smartimage12rgb -\n");
    XP_FilePrintf(f, "    /colorimage where {\n");
    XP_FilePrintf(f, "	pop\n");
    XP_FilePrintf(f, "	{ currentfile rowdata readhexstring pop }\n");
    XP_FilePrintf(f, "	false 3\n");
    XP_FilePrintf(f, "	colorimage\n");
    XP_FilePrintf(f, "    } {\n");
    XP_FilePrintf(f, "	exch pop 8 exch\n");
    XP_FilePrintf(f, "	3 index 12 mul 8 mod 0 ne { 1 } { 0 } ifelse\n");
    XP_FilePrintf(f, "	4 index\n");
    XP_FilePrintf(f, "	6 2 roll\n");
    XP_FilePrintf(f, "	{ 2 copy cvgray }\n");
    XP_FilePrintf(f, "	image\n");
    XP_FilePrintf(f, "	pop pop\n");
    XP_FilePrintf(f, "    } ifelse\n");
    XP_FilePrintf(f, "} def\n");
    XP_FilePrintf(f,"/cshow { dup stringwidth pop 2 div neg 0 rmoveto show } bind def\n");  
     XP_FilePrintf(f,"/rshow { dup stringwidth pop neg 0 rmoveto show } bind def\n");
   XP_FilePrintf(f, "%%%%EndProlog\n");
}

void xl_begin_page(MWContext *cx, int pn)
{
  XP_File f;

  f = cx->prSetup->out;
  pn++;
  XP_FilePrintf(f, "%%%%Page: %d %d\n", pn, pn);
  XP_FilePrintf(f, "%%%%BeginPageSetup\n/pagelevel save def\n");
  if (cx->prSetup->landscape)
    XP_FilePrintf(f, "%d 0 translate 90 rotate\n",
		  PAGE_TO_POINT_I(cx->prSetup->height));
  XP_FilePrintf(f, "%d 0 translate\n", PAGE_TO_POINT_I(cx->prSetup->left));
  XP_FilePrintf(f, "%%%%EndPageSetup\n");
  xl_annotate_page(cx, cx->prSetup->header, 0, -1, pn);

#ifdef DEBUG_jwz
  /* gsave around whole page, so we can undo this clip path to draw the
     bottom margin. */
  XP_FilePrintf(f, "gsave\n");
#endif /* DEBUG_jwz */

  XP_FilePrintf(f, "newpath 0 %d moveto ", PAGE_TO_POINT_I(cx->prSetup->bottom));
  XP_FilePrintf(f, "%d 0 rlineto 0 %d rlineto -%d 0 rlineto ",
			PAGE_TO_POINT_I(cx->prInfo->page_width),
			PAGE_TO_POINT_I(cx->prInfo->page_height),
			PAGE_TO_POINT_I(cx->prInfo->page_width));
  XP_FilePrintf(f, " closepath clip newpath\n");
}

void xl_end_page(MWContext *cx, int pn)
{
#ifdef DEBUG_jwz
  /* undo the clip path so that we can draw the bottom margin. */
  XP_FilePrintf(cx->prSetup->out, "grestore\n");
#endif /* DEBUG_jwz */

  xl_annotate_page(cx, cx->prSetup->footer,
		   cx->prSetup->height-cx->prSetup->bottom-cx->prSetup->top,
		   1, pn);
  XP_FilePrintf(cx->prSetup->out, "pagelevel restore\nshowpage\n");
}

void xl_end_document(MWContext *cx)
{
    XP_FilePrintf(cx->prSetup->out, "%%%%EOF\n");
}

void xl_moveto(MWContext* cx, int x, int y)
{
  XL_SET_NUMERIC_LOCALE();
  y -= cx->prInfo->page_topy;
  /*
  ** Agh! Y inversion HELL again !!
  */
  y = (cx->prInfo->page_height - y - 1) + cx->prSetup->bottom;

  XP_FilePrintf(cx->prSetup->out, "%g %g moveto\n",
		PAGE_TO_POINT_F(x), PAGE_TO_POINT_F(y));
  XL_RESTORE_NUMERIC_LOCALE();
}

void xl_translate(MWContext* cx, int x, int y)
{
    XL_SET_NUMERIC_LOCALE();
    y -= cx->prInfo->page_topy;
    /*
    ** Agh! Y inversion HELL again !!
    */
    y = (cx->prInfo->page_height - y - 1) + cx->prSetup->bottom;

    XP_FilePrintf(cx->prSetup->out, "%g %g translate\n", PAGE_TO_POINT_F(x), PAGE_TO_POINT_F(y));
    XL_RESTORE_NUMERIC_LOCALE();
}

void xl_show(MWContext *cx, char* txt, int len, char *align)
{
    XP_File f;

    f = cx->prSetup->out;
    XP_FilePrintf(f, "(");
    while (len-- > 0) {
	switch (*txt) {
	    case '(':
	    case ')':
	    case '\\':
		XP_FilePrintf(f, "\\%c", *txt);
		break;
	    default:
		if (*txt < ' ' || (*txt & 0x80))
		  XP_FilePrintf(f, "\\%o", *txt & 0xff);
		else
		  XP_FilePrintf(f, "%c", *txt);
		break;
	}
	txt++;
    }
    XP_FilePrintf(f, ") %sshow\n", align);
}

void xl_circle(MWContext* cx, int w, int h)
{
  XL_SET_NUMERIC_LOCALE();
  XP_FilePrintf(cx->prSetup->out, "%g %g c ", PAGE_TO_POINT_F(w)/2.0, PAGE_TO_POINT_F(h)/2.0);
  XL_RESTORE_NUMERIC_LOCALE();
}

void xl_box(MWContext* cx, int w, int h)
{
  XL_SET_NUMERIC_LOCALE();
  XP_FilePrintf(cx->prSetup->out, "%g 0 rlineto 0 %g rlineto %g 0 rlineto closepath ",
      PAGE_TO_POINT_F(w), -PAGE_TO_POINT_F(h), -PAGE_TO_POINT_F(w));
  XL_RESTORE_NUMERIC_LOCALE();
}

MODULE_PRIVATE void
xl_draw_border(MWContext *cx, int x, int y, int w, int h, int thick)
{
  XL_SET_NUMERIC_LOCALE();
  XP_FilePrintf(cx->prSetup->out, "gsave %g setlinewidth\n ",
		  PAGE_TO_POINT_F(thick));
  xl_moveto(cx, x, y);
  xl_box(cx, w, h);
  xl_stroke(cx);
  XP_FilePrintf(cx->prSetup->out, "grestore\n");
  XL_RESTORE_NUMERIC_LOCALE();
}

void xl_stroke(MWContext *cx)
{
    XP_FilePrintf(cx->prSetup->out, " stroke \n");
}

void xl_fill(MWContext *cx)
{
    XP_FilePrintf(cx->prSetup->out, " fill \n");
}

/*
** This fucntion works, but is starting to show it's age, as the list
** of known problems grows:
**
** +  The images are completely uncompressed, which tends to generate
**    huge output files.  The university prototype browser uses RLE
**    compression on the images, which causes them to print slowly,
**    but is a huge win on some class of images.
** +  12 bit files can be constructed which print on any printer, but
**    not 24 bit files.
** +  The 12 bit code is careful not to create a string-per-row, unless
**    it is going through the compatibility conversion routine.
** +  The 1-bit code is completely unused and untested.
** +  It should squish the image if squishing is currently in effect.
*/

void xl_colorimage(MWContext *cx, int x, int y, int w, int h, void* fe_data)
{
    int row, col, pps;
    int n;
    IL_Image *il = (IL_Image*) fe_data;
    int16 *p12;
    int32 *p24;
    unsigned char *p8;
    int cbits;
    int rowdata, rowextra, row_ends_within_byte;
    XP_File f;

    XL_SET_NUMERIC_LOCALE();
    f = cx->prSetup->out;
    pps = 1;
    row_ends_within_byte = 0;

    /* Imagelib row data is aligned to 32-bit boundaries */
    if (il->depth == 1) {
	rowdata = (il->width + 7)/8;
        rowextra = il->maskWidthBytes - rowdata;
	cbits = 1;
	pps = 8;
    } else if (il->depth == 16) {
	if (cx->prSetup->color) {
	    rowdata   = (il->width*12)/8;
	    row_ends_within_byte = (il->width*12)%8 ? 1 : 0;
            cbits = 4;
	} else {
	    cbits = 8;
	    rowdata = il->width;
	}
        rowextra = il->widthBytes - il->width * 2;
    } else { /* depth assumed to be 32 */
	rowdata = 3 * il->width;
        rowextra = 0;
	cbits = 8;
    }

    assert(il->depth == 16 || il->depth == 32 || il->depth == 1);
    XP_FilePrintf(f, "gsave\n");
    XP_FilePrintf(f, "/rowdata %d string def\n",
                  rowdata + row_ends_within_byte);
    xl_translate(cx, x, y + h);
    XP_FilePrintf(f, "%g %g scale\n", PAGE_TO_POINT_F(w), PAGE_TO_POINT_F(h));
    XP_FilePrintf(f, "%d %d ", il->width, il->height);
    XP_FilePrintf(f, "%d ", cbits);
    XP_FilePrintf(f, "[%d 0 0 %d 0 %d]\n", il->width, -il->height, il->height);
    if (cx->prSetup->color && il->depth == 16)
      XP_FilePrintf(f, " smartimage12rgb\n");
    else if (il->depth == 32) {
      XP_FilePrintf(f, " { currentfile rowdata readhexstring pop }\n");
      XP_FilePrintf(f, " false 3 colorimage\n");
    } else {
	XP_FilePrintf(f, " { currentfile rowdata readhexstring pop }\n");
	XP_FilePrintf(f, " image\n");
    }

    n = 0;
    p8 = (unsigned char*) il->bits;
    p12 = (int16*) il->bits;
    p24 = (int32*) il->bits;
    for (row = 0; row < il->height; row++) {
	for (col = 0; col < il->width; col += pps) {
	    switch ( il->depth )
	    {
		case 16:
		    if (cx->prSetup->color) {
			if (n > 76) {
			    XP_FilePrintf(f, "\n");
			    n = 0;
			}
			XP_FilePrintf(f, "%03x", 0xfff & *p12++);
			n += 3;
		    } else {
			unsigned char value;
			value = (*p12 & 0x00f)/15.0 * GREEN_PART * 255.0
			      + ((((*p12 & 0x0f0)>>4)/15.0) * BLUE_PART * 255.0)
			      + ((((*p12 & 0xf00)>>8)/15.0) * RED_PART * 255.0);
			p12++;
			if (n > 76) {
			    XP_FilePrintf(f, "\n");
			    n = 0;
			}
			XP_FilePrintf(f, "%02X", value);
			n += 2;
		    }
		    break;
		case 32:
		    if (n > 71) {
			XP_FilePrintf(f, "\n");
			n = 0;
		    }
		    XP_FilePrintf(f, "%06x", (int) (0xffffff & *p24++));
		    n += 6;
		    break;
		case 1:
		  if (n > 78) {
		    XP_FilePrintf(f, "\n");
		    n = 0;
		  }
                  XP_FilePrintf(f, "%02x", 0xff ^ *p8++);
                  n += 2;
                  break;
	    }
	}
	if (row_ends_within_byte) {
	    XP_FilePrintf(f, "0");
	    n += 1;
	}
        p8 += rowextra;
        p12 = (int16*)((char*)p12 + rowextra);
    }
    XP_FilePrintf(f, "\ngrestore\n");
    XL_RESTORE_NUMERIC_LOCALE();
}

MODULE_PRIVATE void
xl_begin_squished_text(MWContext *cx, float scale)
{
    XL_SET_NUMERIC_LOCALE();
    XP_FilePrintf(cx->prSetup->out, "gsave %g 1 scale\n", scale);
    XL_RESTORE_NUMERIC_LOCALE();
}

MODULE_PRIVATE void
xl_end_squished_text(MWContext *cx)
{
    XP_FilePrintf(cx->prSetup->out, "grestore\n");
}
