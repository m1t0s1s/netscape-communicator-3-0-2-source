/* -*- Mode: C; tab-width: 8 -*-
   images.c --- X front-end stuff dealing with images
   Copyright © 1995 Netscape Communications Corporation, all rights reserved.
   Created: Jamie Zawinski <jwz@netscape.com>, 10-Jul-94.
 */

/* A document has many LO_Images.
   Each LO_Image->FE_Data points to a `struct fe_Pixmap'.
   If it's an icon, that holds the icon number, which we draw specially.
   If it's a real image, it points to an IL_Image.
   Multiple LO_Images may point to the same IL_Image.

   There will be only one IL_Image per URL, when the same URL appears in
   multiple pages.

   Each IL_Image->platformData points to a pair of X Pixmaps.
   When we start reading an image, we allocate space for all of its bits,
   for the image library to write into.

   During the process of uploading the image to the server, we create an
   XImage structure, and point it at those existing bits; the XImage is
   then freed right away (adn we reuse the bits again for the next segment.)
   We only upload the bits to the pixmap through the first fe_Pixmap which
   got created (for the multiple-copies-of-the-same-image case).  That's what
   the bits_creator_p slot indicates.

   As soon as the image is done being loaded, we free the bits, since we
   won't need to ship them to the server ever again.  We later reuse the
   existing fe_Pixmap, and thus X Pixmap, for other instances of this same
   URL.

   The Pixmap and fe_Pixmap are freed from FE_FreeImageElement() at the
   beginning of the next document.
 */

#include "mozilla.h"
#include "xfe.h"
#include "nspr/prcpucfg.h"  /* for IS_LITTLE_ENDIAN / IS_BIG_ENDIAN */
#ifdef EDITOR
#include "xeditor.h"
#endif

/*
#ifdef DEBUG
#define Debug 1
#endif
*/

#define GRAYSCALE_WORKS

#define PARTIAL_IMAGE_BORDER_WIDTH 2

typedef struct fe_ImageData
{
    Pixmap bits_pixmap;
    Pixmap mask_pixmap;
} fe_ImageData;

/* We set this flag when we're running on a visual of a depth that we
   know the image library won't support at all. */
Boolean fe_ImagesCantWork = False;

struct fe_Pixmap *
fe_new_fe_Pixmap(void)
{
    struct fe_Pixmap *fep;
    fep = (struct fe_Pixmap *) malloc (sizeof (struct fe_Pixmap));
    if (! fep)
        return NULL;
    
    fep->type = fep_nothing;
    fep->bits_creator_p = False;
    fep->image_coords_valid = False;
    fep->image_display_begun = False;
    fep->status = ilStart;
    return fep;
}
    
void
XFE_GetImageInfo (MWContext *context, LO_ImageStruct *lo_image,
		  NET_ReloadMethod force_load_p)
{
  struct fe_Pixmap *fep = lo_image->FE_Data;

  if (fep && fep->type != fep_nothing)
    {
      /* We already have the image data. */
      if (IL_GetImage ((char *) lo_image->image_url, context, lo_image,
		       NET_CACHE_ONLY_RELOAD)
	  < 0)
	assert (0);

      return;
    }

  lo_image->image_data = NULL;	 /* we don't use this slot. */

  if (! fep)
      fep = fe_new_fe_Pixmap();
  lo_image->FE_Data = fep;

  if (fe_ImagesCantWork)
    XFE_ImageIcon (context, IL_IMAGE_DELAYED, lo_image);
  else
    {
      Boolean load_p;
      if (CONTEXT_DATA (context)->force_load_images == FORCE_LOAD_ALL_IMAGES)
	load_p = True;
      else if (CONTEXT_DATA (context)->autoload_images_p)
	load_p = True;
      else if (CONTEXT_DATA (context)->force_load_images == 0)
	load_p = False;
      else
	load_p = !strcmp (CONTEXT_DATA (context)->force_load_images,
			  (char *) lo_image->image_url);

      if (lo_image && lo_image->image_attr &&
	  (lo_image->image_attr->attrmask & LO_ATTR_BACKDROP))
	{
	  /* If this is a mono system, don't do backdrops. */
	  Cardinal depth = 0;
	  XtVaGetValues (CONTEXT_WIDGET (context), XtNdepth, &depth, 0);
	  if (depth <= 1 || fe_globalData.force_mono_p)
	    {
	      /* This will Do the Right Thing. */
	      XFE_ImageIcon (context, IL_IMAGE_DELAYED, lo_image);
	      return;
	    }

	  /* There's going to be a backdrop, though we don't necessarily
	     know anything about it yet.  Inform XFE_ImageSize() that it
	     should make masks, in case other images happen to start
	     loading before the backdrop does. */
	  if (! CONTEXT_DATA (context)->backdrop_pixmap)
	    CONTEXT_DATA (context)->backdrop_pixmap = (Pixmap) ~0;
	}

      IL_GetImage ((char *) lo_image->image_url, context, lo_image,
		   (load_p ? force_load_p : NET_CACHE_ONLY_RELOAD));
    }
}


/* Display image. This may be called before layout has positioned the
   image, so if LO_ATTR_DISPLAYED is not set then return because its too
   soon. This may also be called after layout has positioned the image,
   but before any image data has been decoded. In this was we draw a
   shadowed border around the place where the image will go (we draw this
   the same as we do later into the image pixmap). Otherwise, we have
   some of the image data so we draw it. In any case, if we can draw and
   the image has a border, we draw it
 */
static void
fe_DisplaySubImage (MWContext *context, int iLocation,
		    LO_ImageStruct *lo_image,
		    int32 i_x, int32 i_y,
		    int32 i_width, int32 i_height,
		    Boolean suppress_border_p)
{
  long x, y, w, h, bw;
  int sub_x, sub_y;
  struct fe_Pixmap *fep;
  IL_Image *il_image;
  Pixmap pixmap = 0;
  Pixmap mask = 0;
  Pixmap tmp_bg_pixmap = 0;

  Widget widget = CONTEXT_WIDGET (context);
  Display *dpy = XtDisplay (widget);

  if (! XtIsRealized (widget))
    return;

  XP_ASSERT(lo_image->image_attr);
  if (! lo_image->image_attr)
    return;

  fep = (struct fe_Pixmap *) lo_image->FE_Data;
  if (!fep ||
      fep->type == fep_nothing ||
      (lo_image->image_attr &&
       ((lo_image->image_attr->attrmask & LO_ATTR_DISPLAYED) == 0)))
    {
      /* Called too soon */
      return;
    }
  il_image = fep->data.il_image;

  XP_ASSERT(lo_image->border_width >= 0);
  if (lo_image->border_width < 0)
      return;

  bw = lo_image->border_width;

  x = lo_image->x + lo_image->x_offset - CONTEXT_DATA (context)->document_x;
  y = lo_image->y + lo_image->y_offset - CONTEXT_DATA (context)->document_y;
  w = lo_image->width  + bw + bw;
  h = lo_image->height + bw + bw;

  fep->image_display_begun = True;

  /* If the image isn't on the screen there's no work to do. */
  if ((x > 0 && x > CONTEXT_DATA (context)->scrolled_width) ||
      (y > 0 && y > CONTEXT_DATA (context)->scrolled_height) ||
      (x + w < 0) ||
      (y + h < 0))
    return;

  if (fep->type == fep_image &&
      il_image && il_image->platformData)
    {
      fe_ImageData *fe_image_data = (fe_ImageData *)il_image->platformData;
      pixmap = fe_image_data->bits_pixmap;
      mask   = fe_image_data->mask_pixmap;
    }

  if (fep->type == fep_icon)
    {
      fe_DrawIcon (context, lo_image, fep->data.icon_number);
    }
  else if (!pixmap && bw == 0)
    {
      /* Draw a blank box instead of the image since we don't have it yet...
	 If this is an image with a border, don't bother, since the border
	 itself will indicate the edges of the image.

	 Also, don't draw it if it's a transparent image, because it won't
	 get erased properly.
       */
      int ibw = PARTIAL_IMAGE_BORDER_WIDTH;
      if (w > (ibw+ibw) &&
	  h > (ibw+ibw) &&
	  !il_image->transparent)
	if (! suppress_border_p)
	  fe_DrawShadows (context,
			  XtWindow (CONTEXT_DATA (context)->drawing_area),
			  x, y, w, h, ibw, XmSHADOW_OUT);
    }
  else if (pixmap)
    {
      Visual *visual = 0;
      Screen *screen;
      int visual_depth;
      unsigned long flags;
      Widget drawing_area = CONTEXT_DATA (context)->drawing_area;
      XGCValues gcv;
      GC gc;

      XtVaGetValues (widget, XmNscreen, &screen, XmNvisual, &visual, 0);
      if (!visual) visual = fe_globalData.default_visual;
      visual_depth = fe_VisualDepth (dpy, visual);

      i_x = i_x - (lo_image->x + lo_image->x_offset) - bw;
      i_y = i_y - (lo_image->y + lo_image->y_offset) - bw;

      if (i_x < 0)
	{
	  i_width += i_x;
	  i_x = 0;
	}
      if ((i_x + i_width) > lo_image->width)
	{
	  i_width = (uint32)((int32)lo_image->width - i_x);
	}

      if (i_y < 0)
	{
	  i_height += i_y;
	  i_y = 0;
	}
      if ((i_y + i_height) >
          (il_image->transparent ? il_image->validHeight : lo_image->height))
	{
	  i_height = (uint32)((int32)lo_image->height - i_y);
	}

      if ((i_width <= 0) || (i_height <= 0))
          return;

      sub_x = x + bw + i_x;
      sub_y = y + bw + i_y;

      /* Ok, so the mask might be different now, if this image is coming
	 in incrementally - so we need to repaint the background before
	 throwing down the pixmap!  FUCK! */
      if (mask)
          {
              memset (&gcv, ~0, sizeof (gcv));
              
              /* Draw the backdrop pixmap only if the image has           */
              /* no_background.  If the image is in a table cell with a   */
              /* background, then lo_image->text_attr->no_background will */
              /* be FALSE.                                                */
              if (lo_image->text_attr->no_background &&
                  CONTEXT_DATA (context)->backdrop_pixmap &&
                  /* This can only happen if something went wrong while loading
                     the background pixmap, I think. */
                  CONTEXT_DATA (context)->backdrop_pixmap != (Pixmap) ~0)
                  {
                      gcv.fill_style = FillTiled;
                      gcv.tile = CONTEXT_DATA (context)->backdrop_pixmap;
                      gcv.ts_x_origin = -(CONTEXT_DATA (context)->document_x +
                                          sub_x);
                      gcv.ts_y_origin = -(CONTEXT_DATA (context)->document_y +
                                          sub_y);
                      flags = GCTile | GCFillStyle | GCTileStipXOrigin |
                          GCTileStipYOrigin;
                  }
              else /* Fill in background color. */
                  {
                      gcv.fill_style = FillSolid;
                      gcv.foreground = 
                          fe_GetPixel (context,
                                       lo_image->text_attr->bg.red,
                                       lo_image->text_attr->bg.green,
                                       lo_image->text_attr->bg.blue);
                      flags = GCFillStyle | GCForeground;
                  }
              
              gc = fe_GetGC (drawing_area, flags, &gcv);

              /* Draw the background into a temporary pixmap.  Later, we'll 
               * draw the image over the background and then BLiT the whole
               * thing on to the screen.  This avoids flashing while a 
               * transparent image is being drawn.
               */
              tmp_bg_pixmap = XCreatePixmap (dpy, XtWindow(drawing_area),
                                             i_width, i_height,
                                             visual_depth);
              XFillRectangle (dpy, tmp_bg_pixmap, gc, 0, 0, i_width,
                              i_height);
          }


      memset (&gcv, ~0, sizeof (gcv));
      gcv.function = GXcopy;
      flags = GCFunction;
      if (mask)
	{
#if 0
	  XGCValues gcv2;
	  GC gc2;
	  gcv2.function = GXcopy;
	  gcv2.foreground = fe_GetPixel (context, 0, 255, 0);
	  gc2 = XCreateGC (dpy, pixmap,
			   GCFunction|GCForeground, &gcv2);
	  XFillRectangle (dpy, pixmap, gc2,
			  0, 0, lo_image->width, lo_image->height);
	  XFreeGC (dpy, gc2);
#endif

	  gcv.clip_mask = mask;

          if (tmp_bg_pixmap)
            {
                gcv.clip_x_origin = (x + bw) - sub_x;
                gcv.clip_y_origin = (y + bw) - sub_y;
            }
          else
            {
                gcv.clip_x_origin = x + bw;
                gcv.clip_y_origin = y + bw;
            }
                  
	  flags |= (GCClipMask | GCClipXOrigin | GCClipYOrigin);
	}

      gc = fe_GetGC (drawing_area, flags, &gcv);

      if (mask)
	/* This sucks.  If you have a GC that has a pixmap in its ClipMask
	   slot, and then you change the bits in that pixmap, the server will
	   *sometimes* fail to notice that the bits of the mask have changed.
	   We need to explicitly put the same damn pixmap into the GC again
	   to get it to notice. */
	XSetClipMask (XtDisplay (drawing_area), gc, gcv.clip_mask);

      /* Draw the image over a temporary copy of the backdrop image.  Then
       * blast the temporary on to the screen.  This avoids annoying flashing
       * while the image is being drawn.
       */
      if (tmp_bg_pixmap)
        {
            XGCValues gcv2;
            GC gc2;
            gcv2.function = GXcopy;
            gc2 = XCreateGC (dpy, tmp_bg_pixmap, GCFunction, &gcv2);
            XCopyArea (dpy, pixmap, tmp_bg_pixmap, gc,
                       i_x, i_y, i_width, i_height, 0, 0);
            XCopyArea (dpy, tmp_bg_pixmap, XtWindow (drawing_area), gc2,
                       0, 0, i_width, i_height, sub_x, sub_y);
            XFreeGC (dpy, gc2);
            XFreePixmap (dpy, tmp_bg_pixmap);
        }
      else
        {
#ifdef EDITOR
	    /*
	     *    If the image has a transparency mask, and there is
	     *    no background, we have to make sure we draw over
	     *    any selection decoration.
	     */
	    if ((mask)
		&&
		(bw <= 0 || suppress_border_p)
		&&
		((lo_image->ele_attrmask & LO_ELE_SELECTED) == 0)) {
		XGCValues gcv;
		GC gc;
		memset(&gcv, ~0, sizeof (gcv));
		gcv.foreground = CONTEXT_DATA(context)->bg_pixel;
		gcv.line_width = 1;
		gcv.line_style = LineSolid;
		gc = fe_GetGC(CONTEXT_DATA(context)->widget, 
			      GCForeground|GCLineStyle|GCLineWidth,
			      &gcv);
		XDrawRectangle(XtDisplay(CONTEXT_DATA(context)->drawing_area),
			       XtWindow(CONTEXT_DATA(context)->drawing_area),
			       gc, x, y, w-gcv.line_width, h-gcv.line_width);
	    }
#endif /*EDITOR*/
            /* We always always always make pixmaps in the depth of the visual
               so we should always always always use XCopyArea, never XCopyPlane. */
            XCopyArea (dpy, pixmap, XtWindow (drawing_area), gc,
                       i_x, i_y, i_width, i_height, sub_x, sub_y);
            
        }
    }

  /* Draw border around image area if it has one. */
  if (bw > 0 && !suppress_border_p)
    {
      XGCValues gcv;
      GC gc;
      memset (&gcv, ~0, sizeof (gcv));
      gcv.foreground = fe_GetPixel (context,
				    lo_image->text_attr->fg.red,
				    lo_image->text_attr->fg.green,
				    lo_image->text_attr->fg.blue);
      gcv.line_width = bw;
      gc = fe_GetGC (CONTEXT_DATA (context)->widget, GCForeground|GCLineWidth,
		     &gcv);
      /* beware: XDrawRectangle centers the line-thickness on the coords. */
      XDrawRectangle (XtDisplay (CONTEXT_DATA (context)->drawing_area),
		      XtWindow (CONTEXT_DATA (context)->drawing_area),
		      gc, x + (bw / 2), y + (bw / 2),
		      w - bw, h - bw);
    }
#ifdef EDITOR
    /*
     *    Draw selection effects.
     */
  if (EDT_IS_EDITOR(context) &&
      (lo_image->ele_attrmask & LO_ELE_SELECTED) != 0) {
      XGCValues gcv;
      GC gc;
      memset(&gcv, ~0, sizeof (gcv));
      gcv.foreground = CONTEXT_DATA(context)->fg_pixel;
      gcv.background = CONTEXT_DATA(context)->select_bg_pixel;
      gcv.line_width = 1;
      gcv.line_style = LineDoubleDash;
      gc = fe_GetGC(CONTEXT_DATA(context)->widget, 
		    GCForeground|GCBackground|GCLineStyle|GCLineWidth,
		    &gcv);
      /* beware: XDrawRectangle centers the line-thickness on the coords. */
      XDrawRectangle(XtDisplay(CONTEXT_DATA(context)->drawing_area),
		     XtWindow(CONTEXT_DATA(context)->drawing_area),
		     gc, x, y, w-gcv.line_width, h-gcv.line_width);
  }
#endif /*EDITOR*/
}

void
XFE_DisplaySubImage (MWContext *context, int iLocation,
		     LO_ImageStruct *lo_image,
		     int32 i_x, int32 i_y,
		     uint32 i_width, uint32 i_height)
{
  struct fe_Pixmap *fep;
  int bw = lo_image->border_width;

  fep = (struct fe_Pixmap *) lo_image->FE_Data;
  if (!fep ||
      (lo_image->image_attr &&
       ((lo_image->image_attr->attrmask & LO_ATTR_DISPLAYED) == 0)))
    {
      /* Called too soon */
      return;
    }
  else if (fep->image_coords_valid == False)
    {
      fep->image_coords_valid = True;
    }

  /* Layout thinks the document is shorter than it really is by the size
     of the horizontal scrollbar (if there isn't one present now) and thus
     sometimes doesn't ask us to redraw enough of the image if that image
     is at the bottom of the screen.  Rather than teach layout about the
     real size of the document, which scares me because of its possible
     impact on scrolling, let's just add some slop to the height of the
     image every time FE_DisplaySubImage() is called.
   */
  i_height += (CONTEXT_DATA (context)->sb_h * 2);
  if (i_height > lo_image->height)
    i_height = lo_image->height;

  fe_DisplaySubImage (context, iLocation, lo_image,
		      i_x - bw, i_y - bw, i_width + bw + bw, i_height + bw + bw,
		      False);
}


/*
 * Layout has moved this image to a new location, and it may get
 * moved again before it is next displayed, so set valid to
 * false so its final new coords will be saved when next it is displayed.
 */
void
FE_ShiftImage (MWContext *context, LO_ImageStruct *lo_image)
{
  struct fe_Pixmap *fep;

  fep = (struct fe_Pixmap *) lo_image->FE_Data;
  if (fep)
    {
      fep->image_coords_valid = False;
    }
}


void
XFE_DisplayImage (MWContext *context, int iLocation, LO_ImageStruct *lo_image)
{
  struct fe_Pixmap *fep;
  int bw = lo_image->border_width;

  fep = (struct fe_Pixmap *) lo_image->FE_Data;
  if (! fep)
      lo_image->FE_Data = fep = fe_new_fe_Pixmap();

  if (!fep ||
      (lo_image->image_attr &&
       ((lo_image->image_attr->attrmask & LO_ATTR_DISPLAYED) == 0)))
    {
      /* Called too soon */
      return;
    }
  else if (fep->image_coords_valid == False)
    {
      fep->image_coords_valid = True;
    }

  fe_DisplaySubImage (context, iLocation, lo_image,
		      (lo_image->x + lo_image->x_offset) - bw,
		      (lo_image->y + lo_image->y_offset) - bw,
		      lo_image->width + bw + bw,
		      lo_image->height + bw + bw,
		      /* Always draw border, since this is probably the result
			 of an expose. */
		      False);
}


/* Delaying images. */

void
fe_LoadDelayedImage (MWContext *context, const char *url)
{
  if (! CONTEXT_DATA (context)->delayed_images_p)
    return;
  CONTEXT_DATA (context)->force_load_images =
    (url == FORCE_LOAD_ALL_IMAGES ? url : strdup (url));
  CONTEXT_DATA (context)->delayed_images_p = False;
  if (CONTEXT_DATA (context)->delayed_button)
    XtVaSetValues (CONTEXT_DATA (context)->delayed_button,
		   XmNsensitive, False, 0);
  if (CONTEXT_DATA (context)->delayed_menuitem)
    XtVaSetValues (CONTEXT_DATA (context)->delayed_menuitem,
		   XmNsensitive, False, 0);

  fe_ReLayout (context, 0);
}

void
fe_LoadDelayedImages (MWContext *context)
{
  fe_LoadDelayedImage (context, FORCE_LOAD_ALL_IMAGES);
}


/* Backdrops.
 */

static void
fe_set_backdrop (MWContext *context, LO_ImageStruct *lo_image)
{
  struct fe_Pixmap *fep =
    (struct fe_Pixmap *) (lo_image ? lo_image->FE_Data : 0);

  if (fep &&
      fep->type == fep_image &&
      fep->data.il_image->platformData)
    CONTEXT_DATA (context)->backdrop_pixmap =
      ((fe_ImageData *) fep->data.il_image->platformData)->bits_pixmap;
  else
    CONTEXT_DATA (context)->backdrop_pixmap = 0;

#if 0	/* #### this doesn't quite work. */

  /* If there is a background pixmap, we get better performance by setting
     the window's background to None (since we draw the background pixmap
     ourself, instead of letting the server do it for us.)

     But when the background is removed, we must restore the window's
     background to the real background color.

     #### This duplicates some work with XFE_SetBackgroundColor().
   */
  if (CONTEXT_DATA (context)->backdrop_pixmap)
    {
      XtVaSetValues (CONTEXT_DATA (context)->drawing_area,
		     XmNbackgroundPixmap, None, 0);
      XtVaSetValues (CONTEXT_DATA (context)->scrolled,
		     XmNbackgroundPixmap, None, 0);
    }
  else
    {
      Pixel bg = CONTEXT_DATA (context)->bg_pixel;
      XtVaSetValues (CONTEXT_DATA (context)->drawing_area,
		     XmNbackground, bg, 0);
      XtVaSetValues (CONTEXT_DATA (context)->scrolled,
		     XmNbackground, bg, 0);
    }

#endif /* 0 */

  /* The X protocol says that if a pixmap is used as both a
     destination of a drawing operation (i.e., the loading of the next
     small portion of the pixmap from the net) after it is used as a
     tile in a GC (i.e., filling the context backdrop with a tile) and
     then subsequently used again as a tile without any intervening
     XSetTile operation, the results of the next drawing operation
     using the GC with the tile are undefined.  So we get rid of any
     GC's that specify the tile pixmap from our cache every time we
     get more backdrop data.  For more details see Bug #5345. */
  fe_FlushGCCache (CONTEXT_WIDGET (context), 
                   GCTile | GCTileStipXOrigin | GCTileStipYOrigin);

  fe_ClearArea (context, 0, 0,
		CONTEXT_DATA (context)->scrolled_width,
		CONTEXT_DATA (context)->scrolled_height);
  fe_RefreshArea (context,
		  CONTEXT_DATA (context)->document_x,
		  CONTEXT_DATA (context)->document_y,
		  CONTEXT_DATA (context)->scrolled_width,
		  CONTEXT_DATA (context)->scrolled_height);
}

#ifdef EDITOR
void
FE_ClearBackgroundImage(MWContext* context)
{
    fe_set_backdrop(context, NULL);
}
#endif /*EDITOR*/

static int
fe_update_image_pixmap (MWContext *context, struct fe_Pixmap *fep,
                        LO_ImageStruct *lo_image, int start_row, int row_count)
{
  IL_Image *il_image = fep->data.il_image;
  unsigned char *bits = (unsigned char *) il_image->bits;
  unsigned char *mask = (unsigned char *) il_image->mask;
  Widget widget = CONTEXT_WIDGET (context);
  Display *dpy = XtDisplay (widget);
  Visual *visual = 0;
  Screen *screen;
  Window window;
  unsigned int visual_depth;
  unsigned int pixmap_depth;
  XImage *ximage;
  Pixmap pixmap, mask_map, tmp_pixmap;
  char *image_bits, *tmp_bits = NULL;
  XGCValues gcv;
  GC gc, tmp_gc;
  int i;
  Pixel *pixels = fe_ColormapMapping(context);
  fe_ImageData *fe_image_data = (fe_ImageData*)il_image->platformData;

  XtVaGetValues (widget, XmNscreen, &screen, XmNvisual, &visual, 0);
  if (!visual) visual = fe_globalData.default_visual;
  window = XtWindow (CONTEXT_DATA (context)->drawing_area);
  visual_depth = fe_VisualDepth (dpy, visual);
  pixmap_depth = fe_VisualPixmapDepth (dpy, visual);

  assert (il_image->depth >= 1);
  assert (visual_depth >= 1);
  assert (pixmap_depth >= 1);

#ifdef GRAYSCALE_WORKS
  if (visual->class == GrayScale && visual_depth > 1)
    pixels = fe_gray_map;
#endif /* GRAYSCALE_WORKS */

  if ((visual->class != StaticGray) &&
#ifndef GRAYSCALE_WORKS
      (visual->class != GrayScale) &&
#endif /* !GRAYSCALE_WORKS */
      (visual->class != TrueColor) &&
      (visual->class != DirectColor) &&
      (il_image->depth != 1))
  {
    /* Remap the pixels in the image from the indexes in `map' to indexes
       in the actual server colormap that we recently allocated.
       Only remap the pixels that just changed. */
    int j = 0;
    int nb = il_image->widthBytes * (start_row + row_count);
    int sb = il_image->widthBytes * start_row;

    tmp_bits = malloc(nb - sb);
    if (!tmp_bits)
      return FALSE;
    
    for (i = sb; i < nb; i++)
      tmp_bits [j++] = pixels [bits [i]];
    image_bits = tmp_bits;
  }
  else
    image_bits = bits + (il_image->widthBytes * start_row);

  /* The XImage is of the depth of the visual, or of depth 1, depending
     on what the image library returned.  Note that il_image->depth
     is the depth of the *data* which the image library created.  This
     is not necessarily the same as the depth of the visual, and the
     XImage must be created with the same depth as the visual, or 1.
   */
  ximage = XCreateImage (dpy, visual,
			 (il_image->depth == 1 ? 1 : visual_depth),
			 (il_image->depth == 1 ? XYPixmap : ZPixmap),
			 0,				   /* offset */
			 image_bits,
			 il_image->width,
			 row_count,
			 8,				   /* bitmap_pad */
			 il_image->widthBytes); /* bytes_per_line */
  
  if (il_image->depth == 1)
    {
      /* Image library always places pixels left-to-right MSB to LSB */
      ximage->bitmap_bit_order = MSBFirst;

      /* This definition doesn't depend on client byte ordering
         because the image library ensures that the bytes in
         bitmask data are arranged left to right on the screen,
         low to high address in memory. */
      ximage->byte_order = MSBFirst;
    }
  else 
    {
          
#if defined(IS_LITTLE_ENDIAN)
      ximage->byte_order = LSBFirst;
#elif defined (IS_BIG_ENDIAN)
      ximage->byte_order = MSBFirst;
#else
      ERROR! Endianness is unknown;
#endif
    }

  /* Create pixmap if it hasn't already been done */
  if (fe_image_data) {
    pixmap = fe_image_data->bits_pixmap;
    mask_map = fe_image_data->mask_pixmap;
  } else {
    pixmap = mask_map = 0;
  }

  if (!pixmap)
    {
      GC gc;
      XGCValues gcv;
      int bw = PARTIAL_IMAGE_BORDER_WIDTH;

      memset (&gcv, ~0, sizeof (gcv));

      if (! fe_image_data)
	il_image->platformData = fe_image_data =
          (fe_ImageData *) malloc (sizeof (fe_ImageData));

      /* `pixmap' is what we return - it is visual depth.
	 (But the data we use with XPutImage is possibly
	 different, as per XListPixmapFormats(), which
	 tells us the depth of Z-format XImage data.)
      */
      pixmap = XCreatePixmap (dpy, window, il_image->width, il_image->height,
			      visual_depth);
      if (mask)
	mask_map = XCreatePixmap (dpy, window,
				  il_image->width, il_image->height, 1);

      fe_image_data->bits_pixmap = pixmap;
      fe_image_data->mask_pixmap = mask_map;

#if 0
      /* Initialize the mask to `clear'. */
      if (mask_map)
	{
	  gcv.function = GXset;
	  gcv.foreground = 0;
	  gc = XCreateGC (dpy, mask_map, GCFunction|GCForeground, &gcv);
	  XFillRectangle (dpy, mask_map, gc,
			  0, 0, il_image->width, il_image->height);
	  XFreeGC (dpy, gc);
	}
#endif

      /* Clear the pixmap. */
      gcv.foreground = CONTEXT_DATA (context)->bg_pixel;
      gc = fe_GetGC (widget, GCForeground, &gcv);
      XFillRectangle (dpy, pixmap, gc,
		      0, 0, il_image->width, il_image->height);

      /* Draw the bevel into the pixmap so that partial image loads will
	 look turbo cool.  (This wouldn't work if the pixmap was less
	 deep than the visual.)

	 (Don't do it for backdrop or transparent images.)
       */
      if ((il_image->width > bw * 3) &&
	  (il_image->height > bw * 3) &&
	  ((start_row + row_count + 1) < il_image->height) &&
	  !il_image->transparent &&
	  (! (lo_image &&
	      lo_image->image_attr &&
	      (lo_image->image_attr->attrmask & LO_ATTR_BACKDROP))))
	{
	  fe_DrawShadows (context, pixmap, 0, 0,
			  il_image->width, il_image->height,
			  bw, XmSHADOW_OUT);
#if 0
	  /* No, don't do this or we get turds. */

	  /* Let the borders show through in the mask. */
	  if (mask_map)
	    {
	      gcv.function = GXset;
	      gcv.foreground = 1;
	      gcv.line_width = bw * 2;
	      gc = XCreateGC (dpy, mask_map, GCFunction|GCForeground, &gcv);
	      XDrawRectangle (dpy, mask_map, gc,
			      0, 0,
			      il_image->width - 1, il_image->height - 1);
	      XFreeGC (dpy, gc);
	    }
#endif
	}
    }

  memset (&gcv, ~0, sizeof (gcv));

  /* Push over the bits for the pixmap
   */
  if (il_image->depth == 1 && visual_depth != 1)
    {
      int py = start_row;

      /* If this is a bitmap, then the XImage is XYPixmap.  Make a 1 bit
	 pixmap, and copy the XImage into that.  Then copy that 1-bit Pixmap
	 into our N-bit pixmap.  This is less server traffic than expanding
	 from 1->N bits on the client side and sending that.

	 It might make more sense to keep 1-bit images as 1-bit data in the
	 server, as well, but then we couldn't draw the bevelled edges into
	 partially loaded XBMs, and some other things get more complicated.
       */
      gcv.function = GXcopy;

#if 0
      /* If the image library has given us a colormap, then use that as
	 the fg/bg of this image; otherwise, 1 means default foreground. */
      if (il_image->map)
	{
	  assert (il_image->colors == 2);
	  gcv.background = pixels [0];
	  gcv.foreground = pixels [1];
	}
      else
#endif
	{
	  gcv.background = CONTEXT_DATA (context)->bg_pixel;
	  gcv.foreground = CONTEXT_DATA (context)->fg_pixel;
	}
      gc = fe_GetGC (widget, GCFunction|GCForeground|GCBackground, &gcv);

      tmp_pixmap = XCreatePixmap (dpy, window,
				  il_image->width, row_count,
				  il_image->depth);
      tmp_gc = XCreateGC (dpy, tmp_pixmap, GCFunction, &gcv);
      XPutImage (dpy, tmp_pixmap, tmp_gc, ximage, 0, 0, 0, 0,
		 il_image->width, row_count);
      XCopyPlane (dpy, tmp_pixmap, pixmap, gc, 0, 0,
		  il_image->width, row_count, 0, py, 1);
      XFreeGC (dpy, tmp_gc);
      XFreePixmap (dpy, tmp_pixmap);
    }
  else if (il_image->depth != pixmap_depth)
    {
      fprintf (stderr,
	       "%s: internal error: image is %d deep, "
	       "need pixmap data of depth %d for visual of depth %d\n",
	       fe_progname, il_image->depth, pixmap_depth, visual_depth);
    }
  else
    {
      int py;

      /* If the depth is right, we can copy the XImage right into the
	 pixmap we're going to return. */

      if (il_image->depth != 1)
	{
	  /* For images of more than one plane, we're in a colormapped world,
	     and what the image library has given us in the cells of the data
	     array are indexes into the colormap.  So we should just copy those
	     to server land without argument. */
	  gcv.function = GXcopy;
	}
      else
	{	
	  /* If we're in 1-bit mode, the image library has returned 1 for black
	     and 0 for white. It is up to the server whether black and white
	     are 1/0 or 0/1.  (We're dealing with 1-bit images here, so those
	     are the only options.)

	     So, if the image library and the server have different ideas about
	     black and white, we need to invert the bits before giving them to
	     the server.  Fuck you, NCD.  Fuck you.
	   */
	  if (WhitePixelOfScreen (screen) == 1)
	    gcv.function = GXcopyInverted;
	  else
	    gcv.function = GXcopy;
	}

      gc = fe_GetGC (widget, GCFunction, &gcv);
      py = start_row;
      XPutImage (dpy, pixmap, gc, ximage, 0, 0, 0, py,
		 il_image->width, row_count);
    }
  ximage->data = 0;  /* It is not our responsibility to free `bits' */
  XDestroyImage (ximage);

  /* Push over the bits for the mask
   */
  if (il_image->depth == 1 && il_image->transparent && mask_map)
    /* When displaying a 1 bit image (an XBM, or any image in mono)
       the image library assumes we'll hack the masking ourselves.
       But since we create all pixmaps in the depth of the visual,
       we can't just use CopyPlane to put the fg bits in without
       trashing the bg bits - we actually need a mask, which means
       we need to create two pixmaps, one of depth 1 and one of the
       depth of the visual.  (We could optimize the case of a 1-bit
       visual, but who cares.)
       */
    mask = bits;

  if (mask && row_count)
    {
      int py;
      gcv.function = GXcopy;
      gc = XCreateGC (dpy, mask_map, GCFunction, &gcv);

      ximage = XCreateImage (dpy, visual, 1, XYPixmap,
			     0,				   /* offset */
			     mask + (il_image->maskWidthBytes *
				     start_row),
			     il_image->width,
			     row_count,
			     8,				   /* bitmap_pad */
			     il_image->maskWidthBytes); /* bytes_per_line */
      
      /* Image library always places pixels left-to-right MSB to LSB */
      ximage->bitmap_bit_order = MSBFirst;

      /* This definition doesn't depend on client byte ordering
         because the image library ensures that the bytes in
         bitmask data are arranged left to right on the screen,
         low to high address in memory. */
      ximage->byte_order = MSBFirst;

      py = start_row;
      XPutImage (dpy, mask_map, gc, ximage, 0, 0, 0, py,
		 il_image->width, row_count);
      XFreeGC (dpy, gc);

      ximage->data = 0;  /* It is not our responsibility to free `bits' */
      XDestroyImage (ximage);
    }

  if (tmp_bits)
    free(tmp_bits);

  return TRUE;
}

int
XFE_ImageSize (MWContext *context, IL_ImageStatus message,
	       IL_Image *il_image, void *data)
{
  LO_ImageStruct *lo_image = (LO_ImageStruct *) data;
  struct fe_Pixmap *fep = lo_image->FE_Data;

  if (! fep)
      fep = fe_new_fe_Pixmap();
  lo_image->FE_Data = fep;

  fep->type = fep_image;
  fep->data.il_image = il_image;

  if (!il_image->bits && (fep->status != ilComplete))
    {
      fep->type = fep_image;
      fep->bits_creator_p = True;

      /* Allocate storage to hold the image data */
      il_image->bits = calloc (il_image->widthBytes * il_image->height, 1);

      if (!il_image->bits)
	return -1;

      /* Allocate storage to hold the mask data, but only if this document
         has a backdrop, and the image is transparent.  (And never allocate
	 a mask if this image *is* a backdrop pixmap.)
       */
      if (!il_image->mask &&
	  il_image->transparent &&
       /* CONTEXT_DATA (context)->backdrop_pixmap && */
	  (! (lo_image->image_attr &&
	      (lo_image->image_attr->attrmask & LO_ATTR_BACKDROP)))
	  )
	{
	  int size = il_image->maskWidthBytes * il_image->height;
	  il_image->mask = calloc (size, 1);

	  if (!il_image->mask)
	    {
	      free (il_image->bits);
	      return -1;
	    }

	  memset (il_image->mask, ~0, size);
	}
    }

  /*
   * This image was aspect scaled by the image
   * library.  Set the sizes the image library
   * scaled it to into the lo_image struct.
   */
  lo_image->width = il_image->width;
  lo_image->height = il_image->height;

  /* Don't call LO_SetImageInfo() for backdrop images. */
  if (lo_image->image_attr &&
      (lo_image->image_attr->attrmask & LO_ATTR_BACKDROP))
    return 0;

  LO_SetImageInfo (context, lo_image->ele_id,
		   il_image->width, il_image->height);

  if (fep->status != ilComplete)
      fep->status = message;
  return 0;
}

void
XFE_ImageData (MWContext *context, IL_ImageStatus message, IL_Image *il_image,
               void *data, int start_row, int row_count, XP_Bool update_image)
{
  LO_ImageStruct *lo_image = (LO_ImageStruct *) data;
  struct fe_Pixmap *fep = lo_image->FE_Data;

  XP_ASSERT(fep);
  if (! fep)
      return;

  /* Need to set this in case this element was previously an icon.
     This can happen with multi-part MIME where some images can be
     properly decoded and others can't, and are replaced with error
     icons. */
  fep->type = fep_image;
  fep->data.il_image = lo_image->il_image;

  if (update_image && row_count)
     /* Update image pixmap with data from image library. */
     fe_update_image_pixmap (context, fep, lo_image, start_row, row_count);

  /* Display the data that we have so far */
  if (fep->image_coords_valid)
    {
      int bw = lo_image->border_width;

      fe_DisplaySubImage (context, 0, lo_image,
			  (lo_image->x + lo_image->x_offset) + bw,
			  ((lo_image->y + lo_image->y_offset) + bw
			   + start_row),
			  il_image->width,
			  row_count,
			  /* Don't bother drawing border unless this is the
			     first line (hack hack). */
			  (start_row != 0));
    }

  /* This is a backdrop image. */
  if (lo_image->image_attr &&
      (lo_image->image_attr->attrmask & LO_ATTR_BACKDROP))
    {
      /* Call this every time new data comes in, so that we show the user
	 the backdrop image as it loads (and so that it fills the whole
	 window.) */
#ifdef BLOCK_ON_BG_IMAGE
      /* for incremental display */
      fe_set_backdrop (context, lo_image);
#endif /* BLOCK_ON_BG_IMAGE */

      /* If we're done, tell layout to continue. */
      if (message == ilComplete)
        {
#ifndef BLOCK_ON_BG_IMAGE
          fe_set_backdrop (context, lo_image);
#endif /* BLOCK_ON_BG_IMAGE */
	  if (fep->type == fep_image)
	    LO_ClearBackdropBlock (context, lo_image, True);
	  else
	    LO_ClearBackdropBlock (context, lo_image, False);
        }
    }

  fep->status = message;

  /* If we're done with the bits, we can free them (just the bits, though:
     not the fe_Pixmap or the IL_Image, which may still be reused. */
  if (message == ilComplete)
    {
      if (il_image->bits)
	free (il_image->bits);
      il_image->bits = 0;

      if (il_image->mask)
	free (il_image->mask);
      il_image->mask = 0;
    }
}

void
XFE_FreeImageElement (MWContext *context, LO_ImageStruct *lo_image)
{
  struct fe_Pixmap *fep = lo_image->FE_Data;

  /* fep->type == fep_nothing when layout want to free and image that
   * has been requested, but the imagelib hasn't called FE_ImageSize() yet.
   */
  if (lo_image->il_image)
    {
      IL_FreeImage (context, lo_image->il_image, lo_image);
      lo_image->il_image = 0;
    }
  
  if (fep)
    free (fep);
  lo_image->FE_Data = 0;
}

void
FE_ImageDelete (IL_Image *il_image)
{
    Display *dpy = fe_display;
    fe_ImageData *fe_image_data = (fe_ImageData *)il_image->platformData;
    void *bits = il_image->bits;
    void *mask = il_image->mask;

    if (bits)
    {
	free (bits);
	il_image->bits = 0;
    }
    if (mask)
    {
	free (mask);
	il_image->mask = 0;
    }
    if (fe_image_data)
    {
	if (fe_image_data->bits_pixmap) XFreePixmap (dpy, fe_image_data->bits_pixmap);
	if (fe_image_data->mask_pixmap) XFreePixmap (dpy, fe_image_data->mask_pixmap);
	free (fe_image_data);
	il_image->platformData = 0;
    }
}


XP_Bool
XFE_ImageOnScreen (MWContext *context, LO_ImageStruct *lo_image)
{
  int x = lo_image->x + lo_image->x_offset - CONTEXT_DATA(context)->document_x;
  int y = lo_image->y + lo_image->y_offset - CONTEXT_DATA(context)->document_y;
  int bw = lo_image->border_width;
  int w = lo_image->width  + bw + bw;
  int h = lo_image->height + bw + bw;

  if ((x > 0 && x > CONTEXT_DATA (context)->scrolled_width) ||
      (y > 0 && y > CONTEXT_DATA (context)->scrolled_height) ||
      (x + w < 0) ||
      (y + h < 0))
    return False;
  else
    return True;
}
