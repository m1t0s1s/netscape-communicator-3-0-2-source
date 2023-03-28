/*
** All the wonderful code for dealing with images
*/
#include "xlate_i.h"

MODULE_PRIVATE void
PSFE_DisplaySubImage(MWContext *cx, int iLoc, LO_ImageStruct *image_struct,
		     int32 x, int32 y, uint32 width, uint32 height)
{
    PSFE_DisplayImage(cx, iLoc, image_struct);
}

MODULE_PRIVATE void
PSFE_DisplayImage(MWContext *cx, int iLocation, LO_ImageStruct *img)
{
  int x, y;

  if (img->image_attr->attrmask & LO_ATTR_BACKDROP)
    return;

  x = img->x+img->x_offset;
  y = img->y+img->y_offset;

  if (!XP_CheckElementSpan(cx, (LO_Any*) img))
    return;

  if (img->FE_Data)
    xl_colorimage(cx, x, y, img->width, img->height, img->FE_Data);
  else {
    /*
    ** XXX Something's wrong, probably an icon for which we don't
    ** have bits.
    */
#ifndef DEBUG_jwz /* I'd rather it draw nothing */
    xl_moveto(cx, x, y);
    xl_box(cx, img->width, img->height);
    xl_stroke(cx);
#endif /* DEBUG_jwz */
  }
}

MODULE_PRIVATE void
PSFE_FreeImageElement(MWContext *cx, LO_ImageStruct *image_struct)
{
  IL_Image *il;
  il = (IL_Image*) image_struct->FE_Data;
  if(il)
	  (void)IL_FreeImage (cx, il, image_struct);
  image_struct->FE_Data = 0;
}

MODULE_PRIVATE void
PSFE_GetImageInfo(MWContext *cx, LO_ImageStruct *image, NET_ReloadMethod reload)
{
  if (image->image_attr->attrmask & LO_ATTR_BACKDROP)
    LO_ClearBackdropBlock (cx, image, FALSE);
  else if (IL_GetImage ((char *) image->image_url, cx, image, NET_NORMAL_RELOAD) < 0) {
    /*
    ** XXX Don't understand why this is here, somehow need to insert
    ** the broken image icon here.
    */
    image->width = INCH_TO_PAGE(.5);
    image->height = INCH_TO_PAGE(.5);
  }
}

int
PSFE_ImageSize (MWContext *cx, IL_ImageStatus message, IL_Image *il_image, void *data)
{
  LO_ImageStruct *lo_image = (LO_ImageStruct *) data;

  {
    /*
    ** If these are zero, then there were no size tags for the
    ** image, so we pick a size.
    */
    lo_image->width = FEUNITS_X(il_image->width, cx);
    lo_image->height = FEUNITS_Y(il_image->height, cx);
  }
    
  /*
  ** If the image runs off the right edge of the page, we shrink it down
  ** so that it fits.
  */
  if (cx->prSetup->scale_images &&
      lo_image->width+lo_image->x+lo_image->x_offset > cx->prInfo->page_width)
  {
    float avail, scale;
    avail = cx->prInfo->page_width - (lo_image->x+lo_image->x_offset);
    scale = avail / (float) lo_image->width;
    lo_image->width *= scale;
    lo_image->height *= scale;
  }
    
  /*
  ** Now that the width is right, we need to make sure that there
  ** is room on a page for the image.
  */
  if (cx->prSetup->scale_images &&
      lo_image->height > cx->prInfo->page_height)
  {
    float scale;
    scale = cx->prInfo->page_height / (float) lo_image->height;
    lo_image->width *= scale;
    lo_image->height *= scale;
  }

  /*
  ** XXX ebinameister sez that this is evil, the correct way to set the size
  ** is to have GetImageInfo return the size.  Some hack should go here to
  ** not call this routine if GetImageInfo is higher up on the call chain.
  */
  LO_SetImageInfo (cx, lo_image->ele_id, lo_image->width, lo_image->height);
  if (!il_image->bits)
    il_image->bits = malloc (il_image->widthBytes * il_image->height);
  lo_image->FE_Data = il_image;
  return 0;
}

void
PSFE_ImageData (MWContext *cx, IL_ImageStatus msg, IL_Image *img, void *data,
                int start_row, int row_count, XP_Bool update_image)
{
}

XP_Bool
PSFE_ImageOnScreen (MWContext *cx, LO_ImageStruct *lo_image)
{
	return 1;
}

void
PSFE_ImageDelete (MWContext *cx, IL_Image *il)
{
    if (il->bits)
		free(il->bits);
    il->bits = 0;
}

/*
** XXX This should get image data for the icons so printing them does
** "the right thing"
*/
MODULE_PRIVATE void
PSFE_ImageIcon(MWContext *cx, int iconNumber, void* data)
{
  LO_ImageStruct *lo_image = (LO_ImageStruct *) data;
  lo_image->width = INCH_TO_PAGE(.5);
  lo_image->height = INCH_TO_PAGE(.5);
  lo_image->FE_Data = NULL;
  LO_SetImageInfo (cx, lo_image->ele_id, lo_image->width, lo_image->height);
}
