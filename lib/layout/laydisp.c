#include "xp.h"
#include "layout.h"
#include "libi18n.h"

#ifdef PROFILE
#pragma profile on
#endif

#ifdef XP_MAC
# ifdef XP_TRACE
#  undef XP_TRACE
# endif
# define XP_TRACE(X)
#else
#ifndef XP_TRACE
# define XP_TRACE(X) fprintf X
#endif
#endif /* XP_MAC */


static void
lo_NormalizeStartAndEndOfSelection(LO_TextStruct *text, int32 *start,
	int32 *end)
{
	int32 p1, p2;
	char *string;
	int n;
	int16 charset;

	if (text->ele_attrmask & LO_ELE_SELECTED)
	{
		if (text->sel_start < 0)
		{
			p1 = 0;
		}
		else
		{
			p1 = text->sel_start;
		}

		if (text->sel_end < 0)
		{
			p2 = text->text_len - 1;
		}
		else
		{
			p2 = text->sel_end;
		}

		PA_LOCK(string, char *, text->text);

		/*
		 * find beginning of first character
		 */
		charset = text->text_attr->charset;
		switch (n = INTL_NthByteOfChar(charset, string, (int)(p1+1)))
		{
		case 0:
		case 1:
			break;
		default:
			p1 -= (n - 1);
			if (p1 < 0)
			{
				p1 = 0;
			}
			break;
		}
		if (text->sel_start >= 0)
		{
			text->sel_start = p1;
		}

		/*
		 * find end of last character
		 */
		switch (n = INTL_NthByteOfChar(charset, string, (int)(p2+1)))
		{
		case 0:
			break;
		default:
			p2 -= (n - 1);
			if (p2 < 0)
			{
				p2 = 0;
			}
			/* fall through */
		case 1:
			p2 += INTL_IsLeadByte(charset, string[p2]);
			if (p2 > text->text_len - 1)
			{
				p2 = text->text_len - 1;
			}
			break;
		}
		if (text->sel_end >= 0)
		{
			text->sel_end = p2;
		}

		*start = p1;
		*end = p2;

		PA_UNLOCK(text->text);
	}
}


void
lo_DisplaySubtext(MWContext *context, int iLocation, LO_TextStruct *text,
	int32 start_pos, int32 end_pos, Bool need_bg)
{
	lo_NormalizeStartAndEndOfSelection(text, &start_pos, &end_pos);
	FE_DisplaySubtext(context, iLocation, text, start_pos, end_pos,
		(XP_Bool)need_bg);
}


void
lo_DisplayText(MWContext *context, int iLocation,
	LO_TextStruct *text, Bool need_bg)
{
	int32 p1, p2;
	LO_TextAttr tmp_attr;
	LO_TextAttr *hold_attr;

	if (text->ele_attrmask & LO_ELE_SELECTED)
	{
		lo_NormalizeStartAndEndOfSelection(text, &p1, &p2);

		if (p1 > 0)
		{
			FE_DisplaySubtext(context, iLocation, text,
				0, (p1 - 1), (XP_Bool)need_bg);
		}

		lo_CopyTextAttr(text->text_attr, &tmp_attr);
		tmp_attr.fg.red = text->text_attr->bg.red;
		tmp_attr.fg.green = text->text_attr->bg.green;
		tmp_attr.fg.blue = text->text_attr->bg.blue;
		tmp_attr.bg.red = text->text_attr->fg.red;
		tmp_attr.bg.green = text->text_attr->fg.green;
		tmp_attr.bg.blue = text->text_attr->fg.blue;

        /* lo_CopyTextAttr doesn't copy the FE_Data. In
         * this case, however, we need to copy the FE_Data
         * because otherwise the front end will
         * synthesize a new copy. This assumes that
         * the FE_Data doesn't care about font color.
         */
        tmp_attr.FE_Data = text->text_attr->FE_Data;

		hold_attr = text->text_attr;
		text->text_attr = &tmp_attr;
		FE_DisplaySubtext(context, iLocation, text, p1, p2,
			(XP_Bool)need_bg);
		text->text_attr = hold_attr;

		if (p2 < (text->text_len - 1))
		{
			FE_DisplaySubtext(context, iLocation, text,
				(p2 + 1), (text->text_len - 1),
				(XP_Bool)need_bg);
		}
	}
	else
	{
		FE_DisplayText(context, iLocation, text, (XP_Bool)need_bg);
	}
}


void
lo_DisplayEmbed(MWContext *context, int iLocation, LO_EmbedStruct *embed)
{
	/*
	 * Don't ever display hidden embeds
	 */
	if ((embed->extra_attrmask & LO_ELE_HIDDEN) == 0)
	{
		FE_DisplayEmbed(context, iLocation, embed);
	}
}


void
lo_DisplayJavaApp(MWContext *context, int iLocation, LO_JavaAppStruct *java_app)
{
	FE_DisplayJavaApp(context, iLocation, java_app);
}


void
lo_DisplayImage(MWContext *context, int iLocation, LO_ImageStruct *image)
{
	image->image_attr->attrmask |= LO_ATTR_DISPLAYED;
	FE_DisplayImage(context, iLocation, image);
}


void
lo_DisplaySubImage(MWContext *context, int iLocation, LO_ImageStruct *image,
	int32 x, int32 y, uint32 width, uint32 height)
{
	image->image_attr->attrmask |= LO_ATTR_DISPLAYED;
	FE_DisplaySubImage(context, iLocation, image, x, y, width, height);
}


void
lo_ClipImage(MWContext *context, int iLocation, LO_ImageStruct *image,
	int32 x, int32 y, uint32 width, uint32 height)
{
	int32 sub_x, sub_y;
	uint32 sub_w, sub_h;

	/*
	 * If the two don't overlap, do nothing.
	 */
	if (((int32)(image->x + image->x_offset + image->width +
	             image->border_width) < x)||
	    ((int32)(x + width) < (int32)(image->x + image->x_offset))||
	    ((int32)(image->y + image->y_offset + image->height +
		      image->border_width) < y)||
	    ((int32)(y + height) < (int32)(image->y + image->y_offset)))
	{
		return;
	}

	if ((image->x + image->x_offset) >= x)
	{
	    sub_x = image->x + image->x_offset;
	}
	else
	{
	    sub_x = x;
	}

	if ((int32)(image->x + image->x_offset + image->width + image->border_width)
		<= (int32)(x + width))
	{
	    sub_w = image->x + image->x_offset + image->width +
		image->border_width - sub_x;
	}
	else
	{
	    sub_w = x + width - sub_x;
	}

	if ((image->y + image->y_offset) >= y)
	{
	    sub_y = image->y + image->y_offset;
	}
	else
	{
	    sub_y = y;
	}

	if ((int32)(image->y + image->y_offset + image->height + image->border_width)
		<= (int32)(y + height))
	{
	    sub_h = image->y + image->y_offset + image->height +
		image->border_width - sub_y;
	}
	else
	{
	    sub_h = y + height - sub_y;
	}

	if (((int32)sub_w >= (int32)(image->width - 2))&&
	    ((int32)sub_h >= (int32)(image->height - 2)))
	{
	    lo_DisplayImage(context, FE_VIEW, (LO_ImageStruct *)image);
	}
	else
	{
	    lo_DisplaySubImage(context, FE_VIEW, (LO_ImageStruct *)image,
		sub_x, sub_y, sub_w, sub_h);
	}
}


void
lo_DisplayEdge(MWContext *context, int iLocation, LO_EdgeStruct *edge)
{
	if (edge->visible != FALSE)
	{
		FE_DisplayEdge(context, iLocation, edge);
	}
}


void
lo_DisplayTable(MWContext *context, int iLocation, LO_TableStruct *table)
{
	FE_DisplayTable(context, iLocation, table);
}


void
lo_DisplaySubDoc(MWContext *context, int iLocation, LO_SubDocStruct *subdoc)
{
	FE_DisplaySubDoc(context, iLocation, subdoc);
}


void
lo_DisplayCell(MWContext *context, int iLocation, LO_CellStruct *cell)
{
	FE_DisplayCell(context, iLocation, cell);
}


void
lo_DisplayLineFeed(MWContext *context, int iLocation,
	LO_LinefeedStruct *lfeed, Bool need_bg)
{
	LO_TextAttr tmp_attr;
	LO_TextAttr *hold_attr;

	if (lfeed->ele_attrmask & LO_ELE_SELECTED)
	{
		lo_CopyTextAttr(lfeed->text_attr, &tmp_attr);
		tmp_attr.fg.red = lfeed->text_attr->bg.red;
		tmp_attr.fg.green = lfeed->text_attr->bg.green;
		tmp_attr.fg.blue = lfeed->text_attr->bg.blue;
		tmp_attr.bg.red = lfeed->text_attr->fg.red;
		tmp_attr.bg.green = lfeed->text_attr->fg.green;
		tmp_attr.bg.blue = lfeed->text_attr->fg.blue;
		hold_attr = lfeed->text_attr;
		lfeed->text_attr = &tmp_attr;
		FE_DisplayLineFeed(context, iLocation, lfeed, (XP_Bool)need_bg);
		lfeed->text_attr = hold_attr;
	}
	else
	{
		FE_DisplayLineFeed(context, iLocation, lfeed, (XP_Bool)need_bg);
	}
}


void
lo_DisplayHR(MWContext *context, int iLocation, LO_HorizRuleStruct *hrule)
{
	FE_DisplayHR(context, iLocation, hrule);
}


void
lo_DisplayBullet(MWContext *context, int iLocation, LO_BulletStruct *bullet)
{
	FE_DisplayBullet(context, iLocation, bullet);
}


void
lo_DisplayFormElement(MWContext *context, int iLocation,
	LO_FormElementStruct *form_element)
{
	FE_DisplayFormElement(context, iLocation, form_element);
}

#ifdef PROFILE
#pragma profile off
#endif
