/* -*- Mode: C; tab-width: 4 -*-
 * layimage.c - Image layout and fetching/prefetching
 *
 * $Id: layimage.c,v 1.111 1996/07/31 21:11:09 brendan Exp $
 */

#include "xp.h"
#include "net.h"
#include "xp_rgb.h"
#include "pa_parse.h"
#include "layout.h"

#ifdef PROFILE
#pragma profile on
#endif

#define IMAGE_DEF_DIM			50
#define IMAGE_DEF_BORDER		0
#define IMAGE_DEF_ANCHOR_BORDER		2
#define IMAGE_DEF_VERTICAL_SPACE	0
#define IMAGE_DEF_HORIZONTAL_SPACE	0
#define IMAGE_DEF_FLOAT_HORIZONTAL_SPACE	3


/* Primitive image allocator also sets defaults for image structure */
static LO_ImageStruct *
lo_new_image_element(MWContext *context, lo_DocState *state,
                     void *edit_element, LO_TextAttr *tptr)
{
    LO_ImageStruct *image;
    image = (LO_ImageStruct *)lo_NewElement(context, state, LO_IMAGE, edit_element, 0);
    if (image == NULL)
        return NULL;

    /*
     * preserve the edit element and offset across the bzero.
     * The edit_element and edit_offset were set in lo_NewElement.
     */
    {
        void* save_edit_element = image->edit_element;
        int32 save_edit_offset = image->edit_offset;
        XP_BZERO(image, sizeof(*image));
        image->edit_element = save_edit_element;
        image->edit_offset = save_edit_offset;
    }

    image->type = LO_IMAGE;

    /*
     * Fill in default font information.
     */
    if (tptr == NULL)
    {
        LO_TextAttr tmp_attr;

        lo_SetDefaultFontAttr(state, &tmp_attr, context);
        tptr = lo_FetchTextAttr(state, &tmp_attr);
    }
    image->text_attr = tptr;
     
    image->FE_Data = NULL;
    image->image_data = NULL;

    image->image_attr = XP_NEW(LO_ImageAttr);
    if (image->image_attr == NULL)
    {	/* clean up all previously allocated objects */
        lo_FreeElement(context, (LO_Element *)image, FALSE);
        state->top_state->out_of_memory = TRUE;
        return 0;
    }

    image->image_attr->attrmask = 0;
    image->image_attr->form_id = -1;
    image->image_attr->alignment = LO_ALIGN_BASELINE;
    image->image_attr->usemap_name = NULL;
    image->image_attr->usemap_ptr = NULL;
    
    image->ele_attrmask = 0;

    image->sel_start = -1;
    image->sel_end = -1;

    return image; 
}

Bool
LO_ParseRGB(char *rgb, uint8 *red_ptr, uint8 *green_ptr, uint8 *blue_ptr)
{
	char *ptr;
	int32 i, j, len;
	int32 val, bval;
	int32 red_val, green_val, blue_val;
	intn bytes_per_val;

	*red_ptr = 0;
	*green_ptr = 0;
	*blue_ptr = 0;
	red_val = 0;
	green_val = 0;
	blue_val = 0;

	if (rgb == NULL)
	{
		return FALSE;
	}

	len = XP_STRLEN(rgb);
	if (len == 0)
	{
		return FALSE;
	}

	/*
	 * Strings not starting with a '#' are probably named colors.
	 * look them up in the xp lookup table.
	 */
	ptr = rgb;
	if (*ptr == '#')
	{
		ptr++;
		len--;
	}
	else
	{
		/*
		 * If we successfully look up a color name, return its RGB.
		 */
		if (XP_ColorNameToRGB(ptr, red_ptr, green_ptr, blue_ptr) == 0)
		{
			return TRUE;
		}
	}

	if (len == 0)
	{
		return FALSE;
	}

	bytes_per_val = (intn)((len + 2) / 3);
	if (bytes_per_val > 4)
	{
		bytes_per_val = 4;
	}

	for (j=0; j<3; j++)
	{
		val = 0;
		for (i=0; i<bytes_per_val; i++)
		{
			if (*ptr == '\0')
			{
				bval = 0;
			}
			else
			{
				bval = TOLOWER((unsigned char)*ptr);
				if ((bval >= '0')&&(bval <= '9'))
				{
					bval = bval - '0';
				}
				else if ((bval >= 'a')&&(bval <= 'f'))
				{
					bval = bval - 'a' + 10;
				}
				else
				{
					bval = 0;
				}
				ptr++;
			}
			val = (val << 4) + bval;
		}
		if (j == 0)
		{
			red_val = val;
		}
		else if (j == 1)
		{
			green_val = val;
		}
		else
		{
			blue_val = val;
		}
	}

	while ((red_val > 255)||(green_val > 255)||(blue_val > 255))
	{
		red_val = (red_val >> 4);
		green_val = (green_val >> 4);
		blue_val = (blue_val >> 4);
	}
	*red_ptr = (uint8)red_val;
	*green_ptr = (uint8)green_val;
	*blue_ptr = (uint8)blue_val;
	return TRUE;
}


void
lo_BodyForeground(MWContext *context, lo_DocState *state, LO_ImageStruct *image)
{
	PA_Tag tag_rec;
	PA_Tag *tag;
	PA_Block buff;
	char *str;
	uint8 red, green, blue;

	tag = &tag_rec;
	tag->type = P_BODY;
	tag->is_end = FALSE;
	tag->newline_count = 0;
	tag->data = image->alt;
	tag->data_len = image->alt_len;
	tag->true_len = 0;
	tag->lo_data = NULL;
	tag->next = NULL;

	/*
	 * Get the document text TEXT parameter.
	 */
	buff = lo_FetchParamValue(context, tag, PARAM_TEXT);
	if (buff != NULL)
	{
		PA_LOCK(str, char *, buff);
		LO_ParseRGB(str, &red, &green, &blue);
		PA_UNLOCK(buff);
		PA_FREE(buff);

		state->text_fg.red = red;
		state->text_fg.green = green;
		state->text_fg.blue = blue;
		lo_RecolorText(state);
	}

	/*
	 * Get the anchor text LINK parameter.
	 */
	buff = lo_FetchParamValue(context, tag, PARAM_LINK);
	if (buff != NULL)
	{
		PA_LOCK(str, char *, buff);
		LO_ParseRGB(str, &red, &green, &blue);
		PA_UNLOCK(buff);
		PA_FREE(buff);

		state->anchor_color.red = red;
		state->anchor_color.green = green;
		state->anchor_color.blue = blue;
	}

	/*
	 * Get the visited anchor text VLINK parameter.
	 */
	buff = lo_FetchParamValue(context, tag, PARAM_VLINK);
	if (buff != NULL)
	{
		PA_LOCK(str, char *, buff);
		LO_ParseRGB(str, &red, &green, &blue);
		PA_UNLOCK(buff);
		PA_FREE(buff);

		state->visited_anchor_color.red = red;
		state->visited_anchor_color.green = green;
		state->visited_anchor_color.blue = blue;
	}

	/*
	 * Get the active anchor text ALINK parameter.
	 */
	buff = lo_FetchParamValue(context, tag, PARAM_ALINK);
	if (buff != NULL)
	{
		PA_LOCK(str, char *, buff);
		LO_ParseRGB(str, &red, &green, &blue);
		PA_UNLOCK(buff);
		PA_FREE(buff);

		state->active_anchor_color.red = red;
		state->active_anchor_color.green = green;
		state->active_anchor_color.blue = blue;
	}

	if (image->alt != NULL)
	{
		PA_FREE(image->alt);
		image->alt = NULL;
		image->alt_len = 0;
	}

	/*
	 * Free up stuff we stuck in anchor_href
	 */
	if (image->anchor_href != NULL)
	{
		PA_FREE((PA_Block)(image->anchor_href));
		image->anchor_href = NULL;
	}
}


void
lo_BodyBackground(MWContext *context, lo_DocState *state, PA_Tag *tag,
	Bool from_user)
{
	LO_ImageStruct *image;
	PA_Block buff;
	char *str;
	uint8 red, green, blue;

	/*
	 * Fill in the backdrop image structure with default data
	 */
    image = lo_new_image_element(context, state, NULL, NULL);
    if (!image)
        return;
	image->ele_id = -1;

	/*
	 * Get the required src parameter, and make the resulting
	 * url and absolute url.
	 */
	buff = lo_FetchParamValue(context, tag, PARAM_BACKGROUND);
	if (buff != NULL)
	{
		PA_Block new_buff;
		char *new_str;

		PA_LOCK(str, char *, buff);
		if (str != NULL)
		{
			int32 len;

			len = lo_StripTextWhitespace(str, XP_STRLEN(str));
		}
		/*
		 * Do not allow BACKGROUND=""
		 */
		if ((str != NULL)&&(*str != '\0'))
		{
			new_str = NET_MakeAbsoluteURL(
					state->top_state->base_url, str);
		}
		else
		{
			new_str = NULL;
		}
		if (new_str == NULL)
		{
			new_buff = NULL;
		}
		else
		{
			char *url;

			new_buff = PA_ALLOC(XP_STRLEN(new_str) + 1);
			if (new_buff != NULL)
			{
				PA_LOCK(url, char *, new_buff);
				XP_STRCPY(url, new_str);
				PA_UNLOCK(new_buff);
			}
			else
			{
				state->top_state->out_of_memory = TRUE;
			}
			XP_FREE(new_str);
		}
		PA_UNLOCK(buff);
		PA_FREE(buff);
		buff = new_buff;
	}
	image->image_url = buff;

	/*
	 * Get the BGCOLOR parameter.  This is for a color background
	 * instead of an image backdrop.  BACKGROUND if it exists overrides
	 * the BGCOLOR specification.
	 */
	buff = lo_FetchParamValue(context, tag, PARAM_BGCOLOR);
	if (buff != NULL)
	{
		PA_LOCK(str, char *, buff);
		LO_ParseRGB(str, &red, &green, &blue);
		PA_UNLOCK(buff);
	}
	image->anchor_href = (LO_AnchorData *)buff;

	/*
	 * Copy this tag data into alt, so we can yank it out later
	 * to parse possible foreground colors if our backdrop image
	 * succeeds.
	 */
	image->alt = PA_ALLOC(tag->data_len + 1);
	if (image->alt != NULL)
	{
		char *tag_data;

		PA_LOCK(tag_data, char *, tag->data);
		PA_LOCK(str, char *, image->alt);
		XP_STRCPY(str, tag_data);
		PA_UNLOCK(image->alt);
		PA_UNLOCK(tag->data);
		image->alt_len = (int16)tag->data_len;
	}

	/*
	 * Make sure we don't load insecure images inside
	 * a secure document.
	 */
	if (state->top_state->security_level > 0)
	{
		PA_LOCK(str, char *, image->image_url);

		if (((from_user == FALSE)&&(str != NULL))||
			((from_user != FALSE)&&(str != NULL)&&
			 (XP_STRNCMP(str, "file:/", 6) != 0)))
		{
			if (NET_IsURLSecure(str) == FALSE)
			{
				PA_UNLOCK(image->image_url);
				PA_FREE(image->image_url);
				image->image_url = PA_ALLOC(
				    XP_STRLEN("internal-icon-insecure") + 1);
				PA_LOCK(str, char *, image->image_url);
				if (image->image_url != NULL)
				{
				    XP_STRCPY(str, "internal-icon-insecure");
				}
				else
				{
				    state->top_state->out_of_memory = TRUE;
				}
				if (state->top_state->insecure_images == FALSE)
				{
					state->top_state->insecure_images = TRUE;
					FE_SecurityDialog(context, SD_INSECURE_DOCS_WITHIN_SECURE_DOCS_NOT_SHOWN);
				}
			}
		}
		PA_UNLOCK(image->image_url);
	}

	image->image_attr->attrmask |= LO_ATTR_BACKDROP;
	if (from_user != FALSE)
	{
		image->image_attr->attrmask |= LO_ATTR_STICKY;
	}

	/*
	 * Have the front end fetch this backdrop image. It should
	 * know not to return or call me back until the whole image
	 * is loaded.
	 */
	if (image->anchor_href != NULL)
	{
		state->top_state->doc_specified_bg = TRUE;
		state->text_bg.red = red;
		state->text_bg.green = green;
		state->text_bg.blue = blue;
		lo_RecolorText(state);
		FE_SetBackgroundColor(context, red, green, blue);
	}

	if (image->image_url != NULL)
	{
		state->top_state->doc_specified_bg = TRUE;
		state->top_state->have_a_bg_url = TRUE;
		FE_GetImageInfo(context, image, state->top_state->force_reload);
		/*
		 * If the FE gave us the backdrop immediately,
		 * then top_state->backdrop_image will already be
		 * set, otherwise we block.  Don't block if we are
		 * already blocked on a real image.
		 */
		if ((state->top_state->backdrop_image != (LO_Element *)image)&&
			(state->top_state->layout_blocking_element == NULL))
		{
#ifdef BLOCK_ON_BG_IMAGE
			state->top_state->layout_blocking_element =
				(LO_Element *)image;
#else
			lo_BodyForeground(context, state, image);
#endif /* BLOCK_ON_BG_IMAGE */
		}
	}
	else
	{
		lo_BodyForeground(context, state, image);

		/*
		 * Free up the fake image element
		 */
		if (image->alt != NULL)
		{
			PA_FREE(image->alt);
		}
		image->alt = NULL;
		if (image->image_url != NULL)
		{
			PA_FREE(image->image_url);
		}
		image->image_url = NULL;
		if (image->lowres_image_url != NULL)
		{
			PA_FREE(image->lowres_image_url);
		}
		image->lowres_image_url = NULL;
		lo_FreeImageAttributes(image->image_attr);
		image->image_attr = NULL;

		/*
		 * Free up stuff we stuck in anchor_href
		 */
		if (image->anchor_href != NULL)
		{
			PA_FREE((PA_Block)(image->anchor_href));
		}
	}
}


void
lo_BlockedImageLayout(MWContext *context, lo_DocState *state, PA_Tag *tag)
{
	LO_ImageStruct *image;
	PA_Block buff;
	char *str;
	int32 val;
	int32 doc_width;

	/*
	 * Fill in the image structure with default data
	 */
    image = lo_new_image_element(context, state, tag->edit_element, NULL);
    if (!image)
        return;
	image->ele_id = NEXT_ELEMENT;

	image->border_width = -1;
	image->border_vert_space = IMAGE_DEF_VERTICAL_SPACE;
	image->border_horiz_space = IMAGE_DEF_HORIZONTAL_SPACE;

	/*
	 * Check for the ISMAP parameter
	 */
	buff = lo_FetchParamValue(context, tag, PARAM_ISMAP);
	if (buff != NULL)
	{
		image->image_attr->attrmask |= LO_ATTR_ISMAP;
		PA_FREE(buff);
	}

	/*
	 * Check for the USEMAP parameter.  The name is stored in the
	 * image attributes immediately, the map pointer will be
	 * stored in later.
	 */
	buff = lo_FetchParamValue(context, tag, PARAM_USEMAP);
	if (buff != NULL)
	{
		char *new_str;
		char *map_name;
		char *tptr;

		map_name = NULL;
		PA_LOCK(str, char *, buff);

		/*
		 * Make this an absolute URL
		 */
		if (str != NULL)
		{
			int32 len;

			len = lo_StripTextWhitespace(str, XP_STRLEN(str));
		}
		/*
		 * The IETF draft says evaluate #name relative to real,
		 * and not the base url.
		 */
		if ((str != NULL)&&(*str == '#'))
		{
			new_str = NET_MakeAbsoluteURL(
				state->top_state->url, str);
		}
		else
		{
			new_str = NET_MakeAbsoluteURL(
				state->top_state->base_url, str);
		}

		/*
		 * If we have a local URL, we can use the USEMAP.
		 */
		if ((new_str != NULL)&&
			(lo_IsAnchorInDoc(state, new_str) != FALSE))
		{
			tptr = strrchr(new_str, '#');
			if (tptr == NULL)
			{
				tptr = new_str;
			}
			else
			{
				tptr++;
			}
			map_name = XP_STRDUP(tptr);
			XP_FREE(new_str);
		}
		/*
		 * Else a non-local URL, right now we don't support this.
		 */
		else if (new_str != NULL)
		{
			XP_FREE(new_str);
		}
		PA_UNLOCK(buff);
		PA_FREE(buff);
		image->image_attr->usemap_name = map_name;
	}

	/*
	 * Check for an align parameter
	 */
	buff = lo_FetchParamValue(context, tag, PARAM_ALIGN);
	if (buff != NULL)
	{
		Bool floating;

		floating = FALSE;
		PA_LOCK(str, char *, buff);
		image->image_attr->alignment = lo_EvalAlignParam(str,
			&floating);
		if (floating != FALSE)
		{
			image->ele_attrmask |= LO_ELE_FLOATING;
		}
		PA_UNLOCK(buff);
		PA_FREE(buff);
	}

	/*
	 * Get the required src parameter, and make the resulting
	 * url and absolute url.
	 */
	buff = lo_FetchParamValue(context, tag, PARAM_SRC);
	if (buff != NULL)
	{
		PA_Block new_buff;
		char *new_str;

		new_buff = NULL;
		PA_LOCK(str, char *, buff);
		if (str != NULL)
		{
			int32 len;

			len = lo_StripTextWhitespace(str, XP_STRLEN(str));
		}
		new_str = NET_MakeAbsoluteURL(state->top_state->base_url, str);
		if (new_str != NULL)
		{
			char *url;

			new_buff = PA_ALLOC(XP_STRLEN(new_str) + 1);
			if (new_buff != NULL)
			{
				PA_LOCK(url, char *, new_buff);
				XP_STRCPY(url, new_str);
				PA_UNLOCK(new_buff);
			}
			XP_FREE(new_str);
		}
		PA_UNLOCK(buff);
		PA_FREE(buff);
		buff = new_buff;
		
		if (new_str == NULL || new_buff == NULL)
		{	/* clean up all previously allocated objects */
			lo_FreeElement(context, (LO_Element *)image, TRUE);
			state->top_state->out_of_memory = TRUE;
			return;
		}
	}
	image->image_url = buff;

	/*
	 * Get the optional lowsrc parameter, and make the resulting
	 * url and absolute url.
	 */
	buff = lo_FetchParamValue(context, tag, PARAM_LOWSRC);
	if (buff != NULL)
	{
	    PA_Block new_buff;
	    char *new_str;

	    new_buff = NULL;
	    PA_LOCK(str, char *, buff);
	    if (str != NULL)
	    {
		int32 len;

		len = lo_StripTextWhitespace(str, XP_STRLEN(str));
	    }
	    new_str = NET_MakeAbsoluteURL(state->top_state->base_url, str);
	    if (new_str != NULL)
	    {
		    char *url;

		    new_buff = PA_ALLOC(XP_STRLEN(new_str) + 1);
		    if (new_buff != NULL)
		    {
			    PA_LOCK(url, char *, new_buff);
			    XP_STRCPY(url, new_str);
			    PA_UNLOCK(new_buff);
		    }
		    XP_FREE(new_str);
	    }
	    PA_UNLOCK(buff);
	    PA_FREE(buff);
	    buff = new_buff;

	    if (new_str == NULL || new_buff == NULL)
	    {       /* clean up all previously allocated objects */
		    lo_FreeElement(context, (LO_Element *)image, TRUE);
		    state->top_state->out_of_memory = TRUE;
		    return;
	    }
	}
	image->lowres_image_url = buff;

	/*
	 * Get the alt parameter, and store the resulting
	 * text, and its length.
	 */
	buff = lo_FetchParamValue(context, tag, PARAM_ALT);
	if (buff != NULL)
	{
		PA_LOCK(str, char *, buff);
		image->alt_len = XP_STRLEN(str);
		image->alt_len = (int16)lo_StripTextNewlines(str,
						(int32)image->alt_len);
		PA_UNLOCK(buff);
	}
	image->alt = buff;

	doc_width = state->right_margin - state->left_margin;

	/*
	 * Get the width parameter, in absolute or percentage.
	 * If percentage, make it absolute.
	 */
	buff = lo_FetchParamValue(context, tag, PARAM_WIDTH);
	if (buff != NULL)
	{
		Bool is_percent;

		PA_LOCK(str, char *, buff);
		val = lo_ValueOrPercent(str, &is_percent);
		if (is_percent != FALSE)
		{
			if (state->allow_percent_width == FALSE)
			{
				val = 0;
			}
			else
			{
				val = doc_width * val / 100;
				if (val < 1)
				{
					val = 1;
				}
			}
		}
		else
		{
			val = FEUNITS_X(val, context);
			if (val < 1)
			{
				val = 1;
			}
		}
		image->width = val;
		PA_UNLOCK(buff);
		PA_FREE(buff);
	}

	/*
	 * Get the height parameter, in absolute or percentage.
	 * If percentage, make it absolute.
	 */
	buff = lo_FetchParamValue(context, tag, PARAM_HEIGHT);
	if (buff != NULL)
	{
		Bool is_percent;

		PA_LOCK(str, char *, buff);
		val = lo_ValueOrPercent(str, &is_percent);
		if (is_percent != FALSE)
		{
			if (state->allow_percent_height == FALSE)
			{
				val = 0;
			}
			else
			{
				val = state->win_height * val / 100;
				if (val < 1)
				{
					val = 1;
				}
			}
		}
		else
		{
			val = FEUNITS_Y(val, context);
			if (val < 1)
			{
				val = 1;
			}
		}
		image->height = val;
		PA_UNLOCK(buff);
		PA_FREE(buff);
	}

	/*
	 * Get the border parameter.
	 */
	buff = lo_FetchParamValue(context, tag, PARAM_BORDER);
	if (buff != NULL)
	{
		PA_LOCK(str, char *, buff);
		val = XP_ATOI(str);
		if (val < 0)
		{
			val = 0;
		}
		image->border_width = val;
		PA_UNLOCK(buff);
		PA_FREE(buff);
	}
	image->border_width = FEUNITS_X(image->border_width, context);

	/*
	 * Get the extra vertical space parameter.
	 */
	buff = lo_FetchParamValue(context, tag, PARAM_VSPACE);
	if (buff != NULL)
	{
		PA_LOCK(str, char *, buff);
		val = XP_ATOI(str);
		if (val < 0)
		{
			val = 0;
		}
		image->border_vert_space = val;
		PA_UNLOCK(buff);
		PA_FREE(buff);
	}
	image->border_vert_space = FEUNITS_Y(image->border_vert_space, context);

	/*
	 * Get the extra horizontal space parameter.
	 */
	if (image->ele_attrmask & LO_ELE_FLOATING)
	{
		image->border_horiz_space = IMAGE_DEF_FLOAT_HORIZONTAL_SPACE;
	}
	else
	{
		image->border_horiz_space = IMAGE_DEF_HORIZONTAL_SPACE;
	}
	buff = lo_FetchParamValue(context, tag, PARAM_HSPACE);
	if (buff != NULL)
	{
		PA_LOCK(str, char *, buff);
		val = XP_ATOI(str);
		if (val < 0)
		{
			val = 0;
		}
		image->border_horiz_space = val;
		PA_UNLOCK(buff);
		PA_FREE(buff);
	}
	image->border_horiz_space = FEUNITS_X(image->border_horiz_space,
						context);

	/*
	 * Make sure we don't load insecure images inside
	 * a secure document.
	 */
	if (state->top_state->security_level > 0)
	{
		if (image->image_url != NULL)
		{
			PA_LOCK(str, char *, image->image_url);
			if (NET_IsURLSecure(str) == FALSE)
			{
				PA_UNLOCK(image->image_url);
				PA_FREE(image->image_url);
				image->image_url = PA_ALLOC(
				    XP_STRLEN("internal-icon-insecure") + 1);
				if (image->image_url != NULL)
				{
				    PA_LOCK(str, char *, image->image_url);
				    XP_STRCPY(str, "internal-icon-insecure");
				    PA_UNLOCK(image->image_url);
				}
				else
				{
				    state->top_state->out_of_memory = TRUE;
				}
				if (state->top_state->insecure_images == FALSE)
				{
					state->top_state->insecure_images = TRUE;
					FE_SecurityDialog(context, SD_INSECURE_DOCS_WITHIN_SECURE_DOCS_NOT_SHOWN);
				}
			}
			else
			{
				PA_UNLOCK(image->image_url);
			}
		}
	}

#ifdef MOCHA
    /* Reflect the image into Javascript */
    lo_ReflectImage(context, state, tag, image, TRUE);
#endif

	/*
	 * Have the front end fetch this image, and tells us
	 * its dimentions if it knows them.
	 */
	FE_GetImageInfo(context, image, state->top_state->force_reload);

	tag->lo_data = (void *)image;
}


void
lo_PartialFormatImage(MWContext *context, lo_DocState *state, PA_Tag *tag)
{
	LO_ImageStruct *image;

	image = (LO_ImageStruct *)tag->lo_data;
	if (image == NULL)
	{
		return;
	}

#ifdef MOCHA
    /* Reflect the image into Javascript */
    lo_ReflectImage(context, state, tag, image, FALSE);
#endif

	tag->lo_data = NULL;

	/*
	 * Assign it a properly sequencial element id.
	 */
	image->ele_id = NEXT_ELEMENT;

	image->x = state->x;
	image->y = state->y;

	image->anchor_href = state->current_anchor;

	if (image->border_width < 0)
	{
		if ((image->anchor_href != NULL)||
		    (image->image_attr->usemap_name != NULL))
		{
			image->border_width = IMAGE_DEF_ANCHOR_BORDER;
		}
		else
		{
			image->border_width = IMAGE_DEF_BORDER;
		}
	
		image->border_width = FEUNITS_X(image->border_width, context);

	}

	if (state->font_stack == NULL)
	{
		LO_TextAttr tmp_attr;
		LO_TextAttr *tptr;

		/*
		 * Fill in default font information.
		 */
		lo_SetDefaultFontAttr(state, &tmp_attr, context);
		tptr = lo_FetchTextAttr(state, &tmp_attr);
		image->text_attr = tptr;
	}
	else
	{
		image->text_attr = state->font_stack->text_attr;
	}

	if ((image->text_attr != NULL)&&
		(image->text_attr->attrmask & LO_ATTR_ANCHOR))
	{
		image->image_attr->attrmask |= LO_ATTR_ANCHOR;
	}

	if (image->image_attr->usemap_name != NULL)
	{
		LO_TextAttr tmp_attr;

		lo_CopyTextAttr(image->text_attr, &tmp_attr);
		tmp_attr.fg.red =   STATE_UNVISITED_ANCHOR_RED(state);
		tmp_attr.fg.green = STATE_UNVISITED_ANCHOR_GREEN(state);
		tmp_attr.fg.blue =  STATE_UNVISITED_ANCHOR_BLUE(state);
		image->text_attr = lo_FetchTextAttr(state, &tmp_attr);
	}

	/*
	 * We may have to block on this image until later
	 * when the front end can give us the width and height.
	 */
	if ((image->width == 0)||(image->height == 0))
	{
		state->top_state->layout_blocking_element = (LO_Element *)image;
	}
	else
	{
		lo_FinishImage(context, state, image);
	}
}


/*************************************
 * Function: lo_FormatImage
 *
 * Description: This function does all the work to format
 *	and lay out an image on a line of elements.
 *	Creates the new image structure.  Fills it
 *	in based on the parameters in the image tag.
 *	Calls the front end to start fetching the image
 *	and get image dimentions if necessary.
 *	Places image on line, breaking line if necessary.
 *
 * Params: Window context, document state, and the image tag data.
 *
 * Returns: Nothing.
 *************************************/
void
lo_FormatImage(MWContext *context, lo_DocState *state, PA_Tag *tag)
{
	LO_ImageStruct *image;
    LO_TextAttr *tptr = NULL;
	PA_Block buff;
	char *str;
	int32 val;
	int32 doc_width;
	Bool is_a_form;

    if (state->font_stack)
        tptr = state->font_stack->text_attr;

	/*
	 * Fill in the image structure with default data
	 */
    image = lo_new_image_element(context, state, tag->edit_element, tptr);
    if (!image)
        return;

	image->ele_id = NEXT_ELEMENT;
	image->x = state->x;
	image->y = state->y;
	image->anchor_href = state->current_anchor;

	if (image->anchor_href != NULL)
	{
		image->border_width = IMAGE_DEF_ANCHOR_BORDER;
	}
	else
	{
		image->border_width = IMAGE_DEF_BORDER;
	}
	image->border_vert_space = IMAGE_DEF_VERTICAL_SPACE;
	image->border_horiz_space = IMAGE_DEF_HORIZONTAL_SPACE;

	if ((image->text_attr != NULL)&&
		(image->text_attr->attrmask & LO_ATTR_ANCHOR))
	{
		image->image_attr->attrmask |= LO_ATTR_ANCHOR;
	}

	if (tag->type == P_INPUT)
	{
		is_a_form = TRUE;
	}
	else
	{
		is_a_form = FALSE;
	}

	if (is_a_form != FALSE)
	{
		LO_TextAttr tmp_attr;

		image->image_attr->form_id = state->top_state->current_form_num;
		image->image_attr->attrmask |= LO_ATTR_ISFORM;
		image->border_width = IMAGE_DEF_ANCHOR_BORDER;
		lo_CopyTextAttr(image->text_attr, &tmp_attr);
		tmp_attr.fg.red =   STATE_UNVISITED_ANCHOR_RED(state);
		tmp_attr.fg.green = STATE_UNVISITED_ANCHOR_GREEN(state);
		tmp_attr.fg.blue =  STATE_UNVISITED_ANCHOR_BLUE(state);
		image->text_attr = lo_FetchTextAttr(state, &tmp_attr);
	}

	image->image_attr->usemap_name = NULL;
	image->image_attr->usemap_ptr = NULL;

	image->ele_attrmask = 0;

	image->sel_start = -1;
	image->sel_end = -1;

	/*
	 * Check for the ISMAP parameter
	 */
	buff = lo_FetchParamValue(context, tag, PARAM_ISMAP);
	if (buff != NULL)
	{
		image->image_attr->attrmask |= LO_ATTR_ISMAP;
		PA_FREE(buff);
	}

	/*
	 * Check for the USEMAP parameter.  The name is stored in the
	 * image attributes immediately, the map pointer will be
	 * stored in later.
	 */
	buff = lo_FetchParamValue(context, tag, PARAM_USEMAP);
	if (buff != NULL)
	{
		char *new_str;
		char *map_name;
		char *tptr;

		map_name = NULL;
		PA_LOCK(str, char *, buff);

		/*
		 * Make this an absolute URL
		 */
		if (str != NULL)
		{
			int32 len;

			len = lo_StripTextWhitespace(str, XP_STRLEN(str));
		}
		/*
		 * The IETF draft says evaluate #name relative to real,
		 * and not the base url.
		 */
		if ((str != NULL)&&(*str == '#'))
		{
			new_str = NET_MakeAbsoluteURL(
				state->top_state->url, str);
		}
		else
		{
			new_str = NET_MakeAbsoluteURL(
				state->top_state->base_url, str);
		}

		/*
		 * If we have a local URL, we can use the USEMAP.
		 */
		if ((new_str != NULL)&&
			(lo_IsAnchorInDoc(state, new_str) != FALSE))
		{
			tptr = strrchr(new_str, '#');
			if (tptr == NULL)
			{
				tptr = new_str;
			}
			else
			{
				tptr++;
			}
			map_name = XP_STRDUP(tptr);
			XP_FREE(new_str);
		}
		/*
		 * Else a non-local URL, right now we don't support this.
		 */
		else if (new_str != NULL)
		{
			XP_FREE(new_str);
		}

		PA_UNLOCK(buff);
		PA_FREE(buff);
		image->image_attr->usemap_name = map_name;
		if (image->image_attr->usemap_name != NULL)
		{
			LO_TextAttr tmp_attr;

			image->border_width = IMAGE_DEF_ANCHOR_BORDER;
			lo_CopyTextAttr(image->text_attr, &tmp_attr);
			tmp_attr.fg.red =   STATE_UNVISITED_ANCHOR_RED(state);
			tmp_attr.fg.green = STATE_UNVISITED_ANCHOR_GREEN(state);
			tmp_attr.fg.blue =  STATE_UNVISITED_ANCHOR_BLUE(state);
			image->text_attr = lo_FetchTextAttr(state, &tmp_attr);
		}
	}

	/*
	 * Check for an align parameter
	 */
	buff = lo_FetchParamValue(context, tag, PARAM_ALIGN);
	if (buff != NULL)
	{
		Bool floating;

		floating = FALSE;
		PA_LOCK(str, char *, buff);
		image->image_attr->alignment = lo_EvalAlignParam(str,
			&floating);
		if (floating != FALSE)
		{
			image->ele_attrmask |= LO_ELE_FLOATING;
		}
		PA_UNLOCK(buff);
		PA_FREE(buff);
	}

	/*
	 * Get the required src parameter, and make the resulting
	 * url and absolute url.
	 */
	buff = lo_FetchParamValue(context, tag, PARAM_SRC);
	if (buff != NULL)
	{
		PA_Block new_buff;
		char *new_str;

		PA_LOCK(str, char *, buff);
		if (str != NULL)
		{
			int32 len;

			len = lo_StripTextWhitespace(str, XP_STRLEN(str));
		}
		new_str = NET_MakeAbsoluteURL(state->top_state->base_url, str);
		if (new_str == NULL)
		{
			new_buff = NULL;
		}
		else
		{
			char *url;

			new_buff = PA_ALLOC(XP_STRLEN(new_str) + 1);
			if (new_buff != NULL)
			{
				PA_LOCK(url, char *, new_buff);
				XP_STRCPY(url, new_str);
				PA_UNLOCK(new_buff);
			}
			else
			{
				state->top_state->out_of_memory = TRUE;
			}

			/* Complete kludge: if this image has the magic SRC,
			 * then also parse an HREF from it (the IMG tag doesn't
			 * normally have an HREF parameter.)
			 *
			 * The front ends know that when the user clicks on an
			 * image with the magic SRC, that they may find the
			 * "real" URL in the HREF... Gag me.
			 */
			if (!XP_STRCMP(new_str, "internal-external-reconnect"))
			{
				PA_Block hbuf;

				hbuf = lo_FetchParamValue(context, tag, PARAM_HREF);
				if (hbuf != NULL)
				{
					image->anchor_href =
					    lo_NewAnchor(state, hbuf, NULL);
				}
			}

			XP_FREE(new_str);
		}
		PA_UNLOCK(buff);
		PA_FREE(buff);
		buff = new_buff;
	}
	image->image_url = buff;

	/*
	 * Get the optional lowsrc parameter, and make the resulting
	 * url and absolute url.
	 */
	buff = lo_FetchParamValue(context, tag, PARAM_LOWSRC);
	if (buff != NULL)
	{
	    PA_Block new_buff;
	    char *new_str;

	    PA_LOCK(str, char *, buff);
	    if (str != NULL)
	    {
		int32 len;

		len = lo_StripTextWhitespace(str, XP_STRLEN(str));
	    }
	    new_str = NET_MakeAbsoluteURL(state->top_state->base_url, str);
	    if (new_str == NULL)
	    {
		    new_buff = NULL;
	    }
	    else
	    {
		    char *url;

		    new_buff = PA_ALLOC(XP_STRLEN(new_str) + 1);
		    if (new_buff != NULL)
		    {
			    PA_LOCK(url, char *, new_buff);
			    XP_STRCPY(url, new_str);
			    PA_UNLOCK(new_buff);
		    }
		    else
		    {
			    state->top_state->out_of_memory = TRUE;
		    }
		    XP_FREE(new_str);
	    }
	    PA_UNLOCK(buff);
	    PA_FREE(buff);
	    buff = new_buff;
	}
	image->lowres_image_url = buff;

	/*
	 * Get the alt parameter, and store the resulting
	 * text, and its length.  Form the special image form
	 * element, store the name in alt.
	 */
	if (is_a_form != FALSE)
	{
		buff = lo_FetchParamValue(context, tag, PARAM_NAME);
	}
	else
	{
		buff = lo_FetchParamValue(context, tag, PARAM_ALT);
	}
	if (buff != NULL)
	{
		PA_LOCK(str, char *, buff);
		image->alt_len = XP_STRLEN(str);
		image->alt_len = (int16)lo_StripTextNewlines(str,
						(int32)image->alt_len);
		PA_UNLOCK(buff);
	}
	image->alt = buff;

	doc_width = state->right_margin - state->left_margin;

	/*
	 * Get the width parameter, in absolute or percentage.
	 * If percentage, make it absolute.
	 */
	buff = lo_FetchParamValue(context, tag, PARAM_WIDTH);
	if (buff != NULL)
	{
		Bool is_percent;

		PA_LOCK(str, char *, buff);
		val = lo_ValueOrPercent(str, &is_percent);
		if (is_percent != FALSE)
		{
			if (state->allow_percent_width == FALSE)
			{
				val = 0;
			}
			else
			{
				val = doc_width * val / 100;
				if (val < 1)
				{
					val = 1;
				}
			}
		}
		else
		{
			val = FEUNITS_X(val, context);
			if (val < 1)
			{
				val = 1;
			}
		}
		image->width = val;
		PA_UNLOCK(buff);
		PA_FREE(buff);
	}

	/*
	 * Get the height parameter, in absolute or percentage.
	 * If percentage, make it absolute.
	 */
	buff = lo_FetchParamValue(context, tag, PARAM_HEIGHT);
	if (buff != NULL)
	{
		Bool is_percent;

		PA_LOCK(str, char *, buff);
		val = lo_ValueOrPercent(str, &is_percent);
		if (is_percent != FALSE)
		{
			if (state->allow_percent_height == FALSE)
			{
				val = 0;
			}
			else
			{
				val = state->win_height * val / 100;
				if (val < 1)
				{
					val = 1;
				}
			}
		}
		else
		{
			val = FEUNITS_Y(val, context);
			if (val < 1)
			{
				val = 1;
			}
		}
		image->height = val;
		PA_UNLOCK(buff);
		PA_FREE(buff);
	}

	/*
	 * Get the border parameter.
	 */
	buff = lo_FetchParamValue(context, tag, PARAM_BORDER);
	if (buff != NULL)
	{
		PA_LOCK(str, char *, buff);
		val = XP_ATOI(str);
		if (val < 0)
		{
			val = 0;
		}
		image->border_width = val;
		PA_UNLOCK(buff);
		PA_FREE(buff);
	}
	image->border_width = FEUNITS_X(image->border_width, context);

	/*
	 * Get the extra vertical space parameter.
	 */
	buff = lo_FetchParamValue(context, tag, PARAM_VSPACE);
	if (buff != NULL)
	{
		PA_LOCK(str, char *, buff);
		val = XP_ATOI(str);
		if (val < 0)
		{
			val = 0;
		}
		image->border_vert_space = val;
		PA_UNLOCK(buff);
		PA_FREE(buff);
	}
	image->border_vert_space = FEUNITS_Y(image->border_vert_space, context);

	/*
	 * Get the extra horizontal space parameter.
	 */
	if (image->ele_attrmask & LO_ELE_FLOATING)
	{
		image->border_horiz_space = IMAGE_DEF_FLOAT_HORIZONTAL_SPACE;
	}
	else
	{
		image->border_horiz_space = IMAGE_DEF_HORIZONTAL_SPACE;
	}
	buff = lo_FetchParamValue(context, tag, PARAM_HSPACE);
	if (buff != NULL)
	{
		PA_LOCK(str, char *, buff);
		val = XP_ATOI(str);
		if (val < 0)
		{
			val = 0;
		}
		image->border_horiz_space = val;
		PA_UNLOCK(buff);
		PA_FREE(buff);
	}
	image->border_horiz_space = FEUNITS_X(image->border_horiz_space,
						context);

	/*
	 * Make sure we don't load insecure images inside
	 * a secure document.
	 */
	if (state->top_state->security_level > 0)
	{
		if (image->image_url != NULL)
		{
			PA_LOCK(str, char *, image->image_url);
			if (NET_IsURLSecure(str) == FALSE)
			{
				PA_UNLOCK(image->image_url);
				PA_FREE(image->image_url);
				image->image_url = PA_ALLOC(
				    XP_STRLEN("internal-icon-insecure") + 1);
				if (image->image_url != NULL)
				{
				    PA_LOCK(str, char *, image->image_url);
				    XP_STRCPY(str, "internal-icon-insecure");
				    PA_UNLOCK(image->image_url);
				}
				else
				{
				    state->top_state->out_of_memory = TRUE;
				}
				if (state->top_state->insecure_images == FALSE)
				{
					state->top_state->insecure_images = TRUE;
					FE_SecurityDialog(context, SD_INSECURE_DOCS_WITHIN_SECURE_DOCS_NOT_SHOWN);
				}
			}
			else
			{
				PA_UNLOCK(image->image_url);
			}
		}
	}

#ifdef MOCHA
    /* Reflect the image into Javascript */
    lo_ReflectImage(context, state, tag, image, FALSE);
#endif

	/*
	 * Have the front end fetch this image, and tells us
	 * its dimentions if it knows them.
	 */
	FE_GetImageInfo(context, image, state->top_state->force_reload);

	/*
	 * We may have to block on this image until later
	 * when the front end can give us the width and height.
	 */
	if ((image->width == 0)||(image->height == 0))
	{
		state->top_state->layout_blocking_element = (LO_Element *)image;
	}
	else
	{
		lo_FinishImage(context, state, image);
	}
}


void
lo_FinishImage(MWContext *context, lo_DocState *state, LO_ImageStruct *image)
{
	PA_Block buff;
	char *str;
	Bool line_break;
	int32 baseline_inc;
	int32 line_inc;
	int32 image_height, image_width;
	LO_TextStruct tmp_text;
	LO_TextInfo text_info;

	/*
	 * All this work is to get the text_info filled in for the current
	 * font in the font stack. Yuck, there must be a better way.
	 */
	memset (&tmp_text, 0, sizeof (tmp_text));
	buff = PA_ALLOC(1);
	if (buff == NULL)
	{
		state->top_state->out_of_memory = TRUE;
		return;
	}
	PA_LOCK(str, char *, buff);
	str[0] = ' ';
	PA_UNLOCK(buff);
	tmp_text.text = buff;
	tmp_text.text_len = 1;
	tmp_text.text_attr =
		state->font_stack->text_attr;
	FE_GetTextInfo(context, &tmp_text, &text_info);
	PA_FREE(buff);

	/*
	 * Eventually this will never happen since we always
	 * have dims here from either the image tag itself or the
	 * front end.
	 */
	if (image->width == 0)
	{
		if (image->image_data != NULL)
		{
			image->width = image->image_data->width;
		}
		else
		{
			image->width = IMAGE_DEF_DIM;
		}
	}
	if (image->height == 0)
	{
		if (image->image_data != NULL)
		{
			image->height = image->image_data->height;
		}
		else
		{
			image->height = IMAGE_DEF_DIM;
		}
	}

	image_width = image->width + (2 * image->border_width) +
		(2 * image->border_horiz_space);
	image_height = image->height + (2 * image->border_width) +
		(2 * image->border_vert_space);

	/*
	 * If this image has a named usemap, try to locate it
	 * in the map_list and stuff a pointer into the image
	 * structure.
	 *
	 * We no longer look this up now, because due to
	 * table cell relayouts, the usemap_ptr may change.
	 * This lookup will be triggered later, after everything
	 * is laid out by user events in the USEMAPed image.
	 */
#if 0
	if (image->image_attr->usemap_name != NULL)
	{
		lo_MapRec *map;

		map = lo_FindNamedMap(context, state,
				image->image_attr->usemap_name);
		image->image_attr->usemap_ptr = (void *)map;
	}
#endif

	/*
	 * SEVERE FLOW BREAK!  This may be a floating image,
	 * which means at this point we go do something completely
	 * different.
	 */
	if (image->ele_attrmask & LO_ELE_FLOATING)
	{
		if (image->image_attr->alignment == LO_ALIGN_RIGHT)
		{
			if (state->right_margin_stack == NULL)
			{
				image->x = state->right_margin - image_width;
			}
			else
			{
				image->x = state->right_margin_stack->margin -
					image_width;
			}
			if (image->x < 0)
			{
				image->x = 0;
			}
		}
		else
		{
			image->x = state->left_margin;
		}

		image->y = -1;
		image->x_offset += (int16)image->border_horiz_space;
		image->y_offset += (int32)image->border_vert_space;

		lo_AddMarginStack(state, image->x, image->y,
			image->width, image->height,
			image->border_width,
			image->border_vert_space, image->border_horiz_space,
			image->image_attr->alignment);

		/*
		 * Insert this element into the float list.
		 */
		image->next = state->float_list;
		state->float_list = (LO_Element *)image;

		if (state->at_begin_line != FALSE)
		{
			lo_FindLineMargins(context, state);
			state->x = state->left_margin;
		}

		return;
	}

	/*
	 * Will this image make the line too wide.
	 */
	if ((state->x + image_width) > state->right_margin)
	{
		line_break = TRUE;
	}
	else
	{
		line_break = FALSE;
	}

	/*
	 * We cannot break a line if we have no break positions.
	 * Usually happens with a single line of unbreakable text.
	 */
	/*
	 * This doesn't work right now, I don't know why, seems we
	 * have lost the last old_break_pos somehow.
	 */
#ifdef FIX_THIS
	if ((line_break != FALSE)&&(state->break_pos == -1))
	{
		/*
		 * It may be possible to break a previous
		 * text element on the same line.
		 */
		if (state->old_break_pos != -1)
		{
			lo_BreakOldElement(context, state);
			line_break = FALSE;
		}
		else
		{
			line_break = FALSE;
		}
	}
#endif /* FIX_THIS */

	/*
	 * if we are at the beginning of the line.  There is
	 * no point in breaking, we are just too wide.
	 * Also don't break in unwrapped preformatted text.
	 * Also can't break inside a NOBR section.
	 */
	if ((state->at_begin_line != FALSE)||
		(state->preformatted == PRE_TEXT_YES)||
		(state->breakable == FALSE))
	{
		line_break = FALSE;
	}

	/*
	 * break on the image if we have
	 * a break.
	 */
	if (line_break != FALSE)
	{
		/*
		 * We need to make the elements sequential, linefeed
		 * before image.
		 */
		state->top_state->element_id = image->ele_id;

		lo_HardLineBreak(context, state, TRUE);
		image->x = state->x;
		image->y = state->y;
		image->ele_id = NEXT_ELEMENT;
	}

	/*
	 * Figure out how to align this image.
	 * baseline_inc is how much to increase the baseline
	 * of previous element of this line.  line_inc is how
	 * much to increase the line height below the baseline.
	 */
	baseline_inc = 0;
	line_inc = 0;
	/*
	 * If we are at the beginning of a line, with no baseline,
	 * we first set baseline and line_height based on the current
	 * font, then place the image.
	 */
	if (state->baseline == 0)
	{
#ifdef ALIGN_IMAGE_WITH_FAKE_TEXT
		state->baseline = text_info.ascent;
		state->line_height = text_info.ascent + text_info.descent;
#else
		state->baseline = 0;
/* Why do we need this?
		if (state->line_height < 1)
		{
			state->line_height = 1;
		}
*/
#endif /* ALIGN_IMAGE_WITH_FAKE_TEXT */
	}

	lo_CalcAlignOffsets(state, &text_info, image->image_attr->alignment,
		image_width, image_height,
		&image->x_offset, &image->y_offset, &line_inc, &baseline_inc);

	image->x_offset += (int16)image->border_horiz_space;
	image->y_offset += (int32)image->border_vert_space;

	lo_AppendToLineList(context, state,
		(LO_Element *)image, baseline_inc);

	state->baseline += (intn) baseline_inc;
	state->line_height += (intn) (baseline_inc + line_inc);

	/*
	 * Clean up state
	 */
	state->x = state->x + image->x_offset +
		image_width - image->border_horiz_space;
	state->linefeed_state = 0;
	state->at_begin_line = FALSE;
	state->trailing_space = FALSE;
	state->cur_ele_type = LO_IMAGE;
}


Bool
LO_BlockedOnImage(MWContext *context, LO_ImageStruct *image)
{
	int32 doc_id;
	lo_TopState *top_state;

	/*
	 * Get the unique document ID, and retreive this
	 * documents layout state.
	 */
	doc_id = XP_DOCID(context);
	top_state = lo_FetchTopState(doc_id);
	if ((top_state == NULL)||(top_state->doc_state == NULL))
	{
		return(FALSE);
	}

	if ((top_state->layout_blocking_element != NULL)&&
	    (top_state->layout_blocking_element == (LO_Element *)image))
	{
		return(TRUE);
	}
	return(FALSE);
}


/*
 * Make sure the image library has been informed of all possible
 * image URLs that might be loaded by this document.
 */
void
lo_NoMoreImages(MWContext *context)
{
	PA_Tag *tag_ptr;
	int32 doc_id;
	lo_TopState *top_state;
	lo_DocState *state;

	/*
	 * All blocked tags should be at the top level of state
	 */
	doc_id = XP_DOCID(context);
	top_state = lo_FetchTopState(doc_id);
	if ((top_state == NULL)||(top_state->doc_state == NULL))
	{
		/*
		 * No matter what, we must be done with all images now
		 */
		IL_NoMoreImages(context);
		return;
	}
	state = top_state->doc_state;
	tag_ptr = top_state->tags;

	/*
	 * Go through all the remaining blocked tags looking
	 * for <INPUT TYPE=IMAGE> tags.
	 */
	while (tag_ptr != NULL)
	{
		if (tag_ptr->type == P_INPUT)
		{
			PA_Block buff;
			char *str;
			int32 type;

			type = FORM_TYPE_TEXT;
			buff = lo_FetchParamValue(context, tag_ptr, PARAM_TYPE);
			if (buff != NULL)
			{
				PA_LOCK(str, char *, buff);
				type = lo_ResolveInputType(str);
				PA_UNLOCK(buff);
				PA_FREE(buff);
			}

			/*
			 * Prefetch this image.
			 * The ImageStruct created will get stuck into
			 * tag_ptr->lo_data, and freed later in 
			 * lo_ProcessInputTag().
			 */
			if (type == FORM_TYPE_IMAGE)
			{
				lo_BlockedImageLayout(context, state, tag_ptr);
			}
		}
		tag_ptr = tag_ptr->next;
	}
	/*
	 * No matter what, we must be done with all images now
	 */
	IL_NoMoreImages(context);
}

#ifdef EDITOR
LO_ImageStruct* LO_NewImageElement( MWContext* context ){
	int32 doc_id;
	lo_TopState *top_state;
	lo_DocState *state;
	LO_ImageStruct *image;

	/*
	 * Get the unique document ID, and retreive this
	 * documents layout state.
	 */
	doc_id = XP_DOCID(context);
	top_state = lo_FetchTopState(doc_id);
	if ((top_state == NULL)||(top_state->doc_state == NULL))
	{
	    return 0;
	}
	state = top_state->doc_state;

    state->edit_current_element = 0;
	state->edit_current_offset = 0;

	/*
	 * Fill in the image structure with default data
	 */
    image = lo_new_image_element(context, state, NULL, NULL);
    if (!image)
        return NULL;

    image->border_width = -1;
    image->border_vert_space  = IMAGE_DEF_VERTICAL_SPACE;
    image->border_horiz_space = IMAGE_DEF_HORIZONTAL_SPACE;

	return image;
}
#endif        



#ifdef PROFILE
#pragma profile off
#endif
