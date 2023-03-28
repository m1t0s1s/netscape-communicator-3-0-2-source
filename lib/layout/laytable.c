/* -*- Mode: C; tab-width: 4; indent-tabs-mode: t -*-
 */
#include "xp.h"
#include "pa_parse.h"
#include "layout.h"
#include "np.h"


#ifdef TEST_16BIT
#define XP_WIN16
#endif /* TEST_16BIT */


#define	TABLE_BORDERS_ON		1
#define	TABLE_BORDERS_OFF		0
#define	TABLE_BORDERS_GONE		-1

#define	TABLE_DEF_INNER_CELL_PAD	1
#define	TABLE_DEF_INTER_CELL_PAD	2
#define	TABLE_DEF_CELL_BORDER		1

#define	TABLE_DEF_BORDER		0
#define	TABLE_DEF_VERTICAL_SPACE	0
#define	TABLE_DEF_HORIZONTAL_SPACE	0


typedef struct lo_cell_data_struct {
#ifdef XP_WIN16
	/*
	 * This struct must be a power of 2 if we are going to allocate an
	 * array of more than 128K
	 */
	unsigned width, height;
#else
	int32 width, height;
#endif
	lo_TableCell *cell;
} lo_cell_data;


void
lo_BeginCaptionSubDoc(MWContext *context, lo_DocState *state,
	lo_TableCaption *caption, PA_Tag *tag)
{
	LO_SubDocStruct *subdoc;
	lo_DocState *new_state;
	int32 first_width, first_height;
	Bool allow_percent_width;
	Bool allow_percent_height;

	/*
	 * Fill in the subdoc structure with default data
	 */
	subdoc = (LO_SubDocStruct *)lo_NewElement(context, state, LO_SUBDOC, NULL, 0);
	if (subdoc == NULL)
	{
		return;
	}

	subdoc->type = LO_SUBDOC;
	subdoc->ele_id = NEXT_ELEMENT;
	subdoc->x = state->x;
	subdoc->x_offset = 0;
	subdoc->y = state->y;
	subdoc->y_offset = 0;
	subdoc->width = 0;
	subdoc->height = 0;
	subdoc->next = NULL;
	subdoc->prev = NULL;

	subdoc->anchor_href = state->current_anchor;

	if (state->font_stack == NULL)
	{
		subdoc->text_attr = NULL;
	}
	else
	{
		subdoc->text_attr = state->font_stack->text_attr;
	}

	subdoc->FE_Data = NULL;
	subdoc->bg_color = NULL;
	subdoc->state = NULL;

	subdoc->vert_alignment = LO_ALIGN_CENTER;
	subdoc->horiz_alignment = caption->horiz_alignment;

	subdoc->alignment = LO_ALIGN_BOTTOM;

	subdoc->border_width = 0;
	subdoc->border_vert_space = 0;
	subdoc->border_horiz_space = 0;

	subdoc->ele_attrmask = 0;

	subdoc->sel_start = -1;
	subdoc->sel_end = -1;

	subdoc->border_width = FEUNITS_X(subdoc->border_width, context);

	if (subdoc->width == 0)
	{
		first_width = FEUNITS_X(5000, context);
		allow_percent_width = FALSE;
	}
	else
	{
		first_width = subdoc->width;
		allow_percent_width = TRUE;
	}

	if (subdoc->height == 0)
	{
		first_height = FEUNITS_Y(10000, context);
		allow_percent_height = FALSE;
	}
	else
	{
		first_height = subdoc->height;
		allow_percent_height = TRUE;
	}

	new_state = lo_NewLayout(context, first_width, first_height, 0, 0, NULL);
	if (new_state == NULL)
	{
		lo_FreeElement(context, (LO_Element *)subdoc, FALSE);
		return;
	}
	new_state->is_a_subdoc = SUBDOC_CAPTION;
	lo_InheritParentState(new_state, state);
	new_state->allow_percent_width = allow_percent_width;
	new_state->allow_percent_height = allow_percent_height;

	new_state->base_x = 0;
	new_state->base_y = 0;
	new_state->display_blocked = TRUE;
	if (subdoc->width == 0)
	{
		new_state->delay_align = TRUE;
	}

	new_state->win_left = 0;
	new_state->win_right = 0;
	new_state->x = new_state->win_left;
	new_state->y = 0;
	new_state->max_width = new_state->x + 1;
	new_state->left_margin = new_state->win_left;
	new_state->right_margin = new_state->win_width - new_state->win_right;
	new_state->list_stack->old_left_margin = new_state->left_margin;
	new_state->list_stack->old_right_margin = new_state->right_margin;

	/*
	 * Initialize the alignment stack for this state
	 * to match the caption alignment.
	 */
	if (subdoc->horiz_alignment != LO_ALIGN_LEFT)
	{
		lo_PushAlignment(new_state, tag, subdoc->horiz_alignment);
	}

	state->sub_state = new_state;

	state->current_ele = (LO_Element *)subdoc;
}



void
lo_BeginCellSubDoc(MWContext *context, lo_DocState *state,
	lo_TableRec *table, PA_Tag *tag,
	Bool is_a_header, intn draw_borders)
{
	LO_SubDocStruct *subdoc;
	lo_DocState *new_state;
	lo_TableRow *table_row;
	lo_TableCell *table_cell;
	PA_Block buff;
	char *str;
	int32 val;
	int32 first_width, first_height;
	Bool allow_percent_width;
	Bool allow_percent_height;
#ifdef LOCAL_DEBUG
 fprintf(stderr, "lo_BeginCellSubDoc called\n");
#endif /* LOCAL_DEBUG */

	table_row = table->row_ptr;
	table_cell = table_row->cell_ptr;

	/*
	 * Fill in the subdoc structure with default data
	 */
	subdoc = (LO_SubDocStruct *)lo_NewElement(context, state, LO_SUBDOC, NULL, 0);
	if (subdoc == NULL)
	{
		return;
	}

	subdoc->type = LO_SUBDOC;
	subdoc->ele_id = NEXT_ELEMENT;
	subdoc->x = state->x;
	subdoc->x_offset = 0;
	subdoc->y = state->y;
	subdoc->y_offset = 0;
	subdoc->width = 0;
	subdoc->height = 0;
	subdoc->next = NULL;
	subdoc->prev = NULL;

	subdoc->anchor_href = state->current_anchor;

	if (state->font_stack == NULL)
	{
		subdoc->text_attr = NULL;
	}
	else
	{
		subdoc->text_attr = state->font_stack->text_attr;
	}

	subdoc->FE_Data = NULL;
	subdoc->state = NULL;

	subdoc->vert_alignment = table_row->vert_alignment;
	if (subdoc->vert_alignment == LO_ALIGN_DEFAULT)
	{
		subdoc->vert_alignment = LO_ALIGN_CENTER;
	}
	subdoc->horiz_alignment = table_row->horiz_alignment;
	if (subdoc->horiz_alignment == LO_ALIGN_DEFAULT)
	{
		if (is_a_header == FALSE)
		{
			subdoc->horiz_alignment = LO_ALIGN_LEFT;
		}
		else
		{
			subdoc->horiz_alignment = LO_ALIGN_CENTER;
		}
	}
	subdoc->alignment = LO_ALIGN_BOTTOM;

	if (draw_borders == TABLE_BORDERS_OFF)
	{
		subdoc->border_width = 0;
		subdoc->border_vert_space = TABLE_DEF_CELL_BORDER;
		subdoc->border_horiz_space = TABLE_DEF_CELL_BORDER;
	}
	else if (draw_borders == TABLE_BORDERS_ON)
	{
		subdoc->border_width = TABLE_DEF_CELL_BORDER;
		subdoc->border_vert_space = 0;
		subdoc->border_horiz_space = 0;
	}
	else
	{
		subdoc->border_width = 0;
		subdoc->border_vert_space = 0;
		subdoc->border_horiz_space = 0;
	}

	subdoc->ele_attrmask = 0;

	subdoc->sel_start = -1;
	subdoc->sel_end = -1;

	subdoc->bg_color = NULL;

	/*
	 * May inherit a bg_color from your row.
	 */
	if (table_row->bg_color != NULL)
	{
		subdoc->bg_color = XP_NEW(LO_Color);
		if (subdoc->bg_color != NULL)
		{
			subdoc->bg_color->red = table_row->bg_color->red;
			subdoc->bg_color->green = table_row->bg_color->green;
			subdoc->bg_color->blue = table_row->bg_color->blue;
		}
	}
	/*
	 * Else may inherit a bg_color from your table.
	 */
	else if (table->bg_color != NULL)
	{
		subdoc->bg_color = XP_NEW(LO_Color);
		if (subdoc->bg_color != NULL)
		{
			subdoc->bg_color->red = table->bg_color->red;
			subdoc->bg_color->green = table->bg_color->green;
			subdoc->bg_color->blue = table->bg_color->blue;
		}
	}
	/*
	 * Else possibly inherit the background color attribute
	 * of a parent table cell.
	 */
	else if (state->is_a_subdoc == SUBDOC_CELL)
	{
		lo_DocState *up_state;

		/*
		 * Find the parent subdoc's state.
		 */
		up_state = state->top_state->doc_state;
		while ((up_state->sub_state != NULL)&&
			(up_state->sub_state != state))
		{
			up_state = up_state->sub_state;
		}
		if ((up_state->sub_state != NULL)&&
		    (up_state->current_ele != NULL)&&
		    (up_state->current_ele->type == LO_SUBDOC)&&
		    (up_state->current_ele->lo_subdoc.bg_color != NULL))
		{
			LO_Color *old_bg;

			old_bg = up_state->current_ele->lo_subdoc.bg_color;
			subdoc->bg_color = XP_NEW(LO_Color);
			if (subdoc->bg_color != NULL)
			{
				subdoc->bg_color->red = old_bg->red;
				subdoc->bg_color->green = old_bg->green;
				subdoc->bg_color->blue = old_bg->blue;
			}
		}
	}

	/*
	 * Check for a background color attribute
	 */
	buff = lo_FetchParamValue(context, tag, PARAM_BGCOLOR);
	if (buff != NULL)
	{
		uint8 red, green, blue;

		PA_LOCK(str, char *, buff);
		LO_ParseRGB(str, &red, &green, &blue);
		PA_UNLOCK(buff);
		PA_FREE(buff);
		subdoc->bg_color = XP_NEW(LO_Color);
		if (subdoc->bg_color != NULL)
		{
			subdoc->bg_color->red = red;
			subdoc->bg_color->green = green;
			subdoc->bg_color->blue = blue;
		}
	}

	/*
	 * Check for a vertical align parameter
	 */
	buff = lo_FetchParamValue(context, tag, PARAM_VALIGN);
	if (buff != NULL)
	{
		PA_LOCK(str, char *, buff);
		subdoc->vert_alignment = lo_EvalVAlignParam(str);
		PA_UNLOCK(buff);
		PA_FREE(buff);
	}

	/*
	 * Check for a horizontal align parameter
	 */
	buff = lo_FetchParamValue(context, tag, PARAM_ALIGN);
	if (buff != NULL)
	{
		PA_LOCK(str, char *, buff);
		subdoc->horiz_alignment = lo_EvalCellAlignParam(str);
		PA_UNLOCK(buff);
		PA_FREE(buff);
	}

	subdoc->border_width = FEUNITS_X(subdoc->border_width, context);
	subdoc->border_vert_space =
		FEUNITS_Y(subdoc->border_vert_space, context);
	subdoc->border_horiz_space =
		FEUNITS_X(subdoc->border_horiz_space, context);

	/*
	 * Allow individual setting of cell
	 * width and height.
	 */
	buff = lo_FetchParamValue(context, tag, PARAM_WIDTH);
	if (buff != NULL)
	{
		Bool is_percent;

		PA_LOCK(str, char *, buff);
		val = lo_ValueOrPercent(str, &is_percent);
		if (is_percent != FALSE)
		{
			/*
			 * Percentage width cells must be handled
			 * later after at least the whole row is
			 * processed.
			 */
			table_cell->percent_width = val;
			table_row->has_percent_width_cells = TRUE;
			table->has_percent_width_cells = TRUE;
			val = 0;
		}
		else
		{
			val = FEUNITS_X(val, context);
		}
		subdoc->width = val;
		if (subdoc->width < 0)
		{
			subdoc->width = 0;
		}
		PA_UNLOCK(buff);
		PA_FREE(buff);
	}
	buff = lo_FetchParamValue(context, tag, PARAM_HEIGHT);
	if (buff != NULL)
	{
		Bool is_percent;

		PA_LOCK(str, char *, buff);
		val = lo_ValueOrPercent(str, &is_percent);
		if (is_percent != FALSE)
		{
			/*
			 * Percentage width cells must be handled
			 * later after at least the whole row is
			 * processed.
			 */
			val = 0;
			table_cell->percent_height = val;
			table->has_percent_height_cells = TRUE;
		}
		else
		{
			val = FEUNITS_X(val, context);
		}
		subdoc->height = val;
		if (subdoc->height < 0)
		{
			subdoc->height = 0;
		}
		PA_UNLOCK(buff);
		PA_FREE(buff);
	}

	if (subdoc->width == 0)
	{
		first_width = FEUNITS_X(5000, context);
		allow_percent_width = FALSE;
	}
	else
	{
		first_width = subdoc->width;
		allow_percent_width = TRUE;
	}

	if (subdoc->height == 0)
	{
		first_height = FEUNITS_Y(10000, context);
		allow_percent_height = FALSE;
	}
	else
	{
		first_height = subdoc->height;
		allow_percent_height = TRUE;
	}

	new_state = lo_NewLayout(context, first_width, first_height, 0, 0, state);
	if (new_state == NULL)
	{
		lo_FreeElement(context, (LO_Element *)subdoc, FALSE);
		return;
	}
	new_state->is_a_subdoc = SUBDOC_CELL;
	lo_InheritParentState(new_state, state);
	new_state->allow_percent_width = allow_percent_width;
	new_state->allow_percent_height = allow_percent_height;

	new_state->base_x = 0;
	new_state->base_y = 0;
	new_state->display_blocked = TRUE;
	if (subdoc->width == 0)
	{
		new_state->delay_align = TRUE;
	}

	new_state->win_left = 0;
	new_state->win_right = 0;
	new_state->x = new_state->win_left;
	new_state->y = 0;
	new_state->max_width = new_state->x + 1;
	new_state->left_margin = new_state->win_left;
	new_state->right_margin = new_state->win_width - new_state->win_right;
	new_state->list_stack->old_left_margin = new_state->left_margin;
	new_state->list_stack->old_right_margin = new_state->right_margin;

	/*
	 * Initialize the alignment stack for this state
	 * to match the caption alignment.
	 */
	if (subdoc->horiz_alignment != LO_ALIGN_LEFT)
	{
		lo_PushAlignment(new_state, tag, subdoc->horiz_alignment);
	}

	state->sub_state = new_state;

	state->current_ele = (LO_Element *)subdoc;

	if (is_a_header != FALSE)
	{
		LO_TextAttr *old_attr;
		LO_TextAttr *attr;
		LO_TextAttr tmp_attr;

		old_attr = new_state->font_stack->text_attr;
		lo_CopyTextAttr(old_attr, &tmp_attr);
		tmp_attr.fontmask |= LO_FONT_BOLD;
		attr = lo_FetchTextAttr(new_state, &tmp_attr);

		lo_PushFont(new_state, tag->type, attr);
	}
}


Bool
lo_subdoc_has_elements(lo_DocState *sub_state)
{
	if (sub_state->end_last_line != NULL)
	{
		return(TRUE);
	}
	if (sub_state->float_list != NULL)
	{
		return(TRUE);
	}
	return(FALSE);
}


int32
lo_align_subdoc(MWContext *context, lo_DocState *state, lo_DocState *old_state,
	LO_SubDocStruct *subdoc, lo_TableRec *table, lo_table_span *row_max)
{
	int32 ele_cnt;
	LO_Element *eptr;
	int32 vert_inc;
	int32 inner_pad;
	LO_Element **line_array;

	inner_pad = FEUNITS_X(table->inner_cell_pad, context);

	ele_cnt = 0;

	/*
	 * Mess with vertical alignment.
	 */
	vert_inc = subdoc->height - (2 * subdoc->border_width) - 
		(2 * inner_pad) - old_state->y;
	if (vert_inc > 0)
	{
		if (subdoc->vert_alignment == LO_ALIGN_CENTER)
		{
			vert_inc = vert_inc / 2;
		}
		else if (subdoc->vert_alignment == LO_ALIGN_BOTTOM)
		{
			/* vert_inc unchanged */
		}
		else if ((subdoc->vert_alignment == LO_ALIGN_BASELINE)&&
			(row_max != NULL))
		{
			int32 baseline;
			int32 tmp_val;

			baseline = lo_GetSubDocBaseline(subdoc);
			tmp_val = row_max->min_dim - baseline;
			if (tmp_val > vert_inc)
			{
				tmp_val = vert_inc;
			}
			if (tmp_val > 0)
			{
				vert_inc = tmp_val;
			}
			else
			{
				vert_inc = 0;
			}
		}
		else
		{
			vert_inc = 0;
		}
	}
	else
	{
		vert_inc = 0;
	}

	/*
	 * Make eptr point to the start of the element chain
	 * for this subdoc.
	 */
#ifdef XP_WIN16
{
	XP_Block *larray_array;

	XP_LOCK_BLOCK(larray_array, XP_Block *, old_state->larray_array);
	old_state->line_array = larray_array[0];
	XP_UNLOCK_BLOCK(old_state->larray_array);
}
#endif /* XP_WIN16 */
	XP_LOCK_BLOCK(line_array, LO_Element **, old_state->line_array);
	eptr = line_array[0];
	XP_UNLOCK_BLOCK(old_state->line_array);

	while (eptr != NULL)
	{
		eptr->lo_any.x += inner_pad;

		switch (eptr->type)
		{
		    case LO_LINEFEED:
			if (eptr->lo_linefeed.ele_attrmask &
				LO_ELE_DELAY_ALIGNED)
			{
			    if (eptr->lo_linefeed.ele_attrmask &
				LO_ELE_DELAY_CENTER)
			    {
				int32 side_inc;
				int32 inside_width;
				LO_Element *c_eptr;

				c_eptr = eptr;
				inside_width = subdoc->width -
				    (2 * subdoc->border_width) -
				    (2 * subdoc->border_horiz_space) -
				    (2 * inner_pad);

				side_inc = inside_width -
					(c_eptr->lo_linefeed.x +
					c_eptr->lo_linefeed.x_offset -
					inner_pad);
				side_inc = side_inc / 2;
				if (side_inc > 0)
				{
					c_eptr->lo_linefeed.width -= side_inc;
					if (c_eptr->lo_linefeed.width < 0)
					{
						c_eptr->lo_linefeed.width = 0;
					}
					while ((c_eptr->lo_any.prev != NULL)&&
						(c_eptr->lo_any.prev->type !=
							LO_LINEFEED))
					{
						c_eptr->lo_any.x += side_inc;
						if (c_eptr->type == LO_CELL)
						{
						    lo_ShiftCell(
							(LO_CellStruct *)c_eptr,
							side_inc, 0);
						}
						c_eptr = c_eptr->lo_any.prev;
					}
					c_eptr->lo_any.x += side_inc;
					if (c_eptr->type == LO_CELL)
					{
					    lo_ShiftCell(
						(LO_CellStruct *)c_eptr,
						side_inc, 0);
					}
				}
			    }
			    else if (eptr->lo_linefeed.ele_attrmask &
				LO_ELE_DELAY_RIGHT)
			    {
				int32 side_inc;
				int32 inside_width;
				LO_Element *c_eptr;

				c_eptr = eptr;
				inside_width = subdoc->width -
				    (2 * subdoc->border_width) -
				    (2 * subdoc->border_horiz_space) -
				    (2 * inner_pad);

				side_inc = inside_width -
					(c_eptr->lo_linefeed.x +
					c_eptr->lo_linefeed.x_offset -
					inner_pad);
				if (side_inc > 0)
				{
					c_eptr->lo_linefeed.width -= side_inc;
					if (c_eptr->lo_linefeed.width < 0)
					{
						c_eptr->lo_linefeed.width = 0;
					}
					while ((c_eptr->lo_any.prev != NULL)&&
						(c_eptr->lo_any.prev->type !=
							LO_LINEFEED))
					{
						c_eptr->lo_any.x += side_inc;
						if (c_eptr->type == LO_CELL)
						{
						    lo_ShiftCell(
							(LO_CellStruct *)c_eptr,
							side_inc, 0);
						}
						c_eptr = c_eptr->lo_any.prev;
					}
					c_eptr->lo_any.x += side_inc;
					if (c_eptr->type == LO_CELL)
					{
					    lo_ShiftCell(
						(LO_CellStruct *)c_eptr,
						side_inc, 0);
					}
				}
			    }
			    eptr->lo_linefeed.ele_attrmask &=
					(~(LO_ELE_DELAY_ALIGNED));
			}
			break;
		    default:
			break;
		}
		eptr->lo_any.y += (vert_inc + inner_pad);
		if (eptr->type == LO_CELL)
		{
			lo_ShiftCell((LO_CellStruct *)eptr, inner_pad,
				(vert_inc + inner_pad));
		}
		eptr = eptr->lo_any.next;
		ele_cnt++;
	}

	eptr = old_state->float_list;
	while (eptr != NULL)
	{
		eptr->lo_any.x += inner_pad;
		eptr->lo_any.y += (vert_inc + inner_pad);
		if (eptr->type == LO_CELL)
		{
			lo_ShiftCell((LO_CellStruct *)eptr, inner_pad,
				(vert_inc + inner_pad));
		}
		eptr = eptr->lo_any.next;
		ele_cnt++;
	}

	return(ele_cnt);
}


LO_SubDocStruct *
lo_EndCellSubDoc(MWContext *context, lo_DocState *state, lo_DocState *old_state,
	LO_Element *element)
{
	LO_Element *eptr;
	LO_SubDocStruct *subdoc;
	PA_Block buff;
	char *str;
	Bool line_break;
	int32 baseline_inc;
	int32 line_inc;
	int32 subdoc_height, subdoc_width;
	int32 doc_height;
	LO_TextStruct tmp_text;
	LO_TextInfo text_info;
	LO_Element **line_array;
	Bool dynamic_width;
	Bool dynamic_height;
#ifdef LOCAL_DEBUG
 fprintf(stderr, "lo_EndCellSubDoc called\n");
#endif /* LOCAL_DEBUG */

	subdoc = (LO_SubDocStruct *)element;
	subdoc->state = (void *)old_state;

	/*
	 * All this work is to get the text_info filled in for the current
	 * font in the font stack. Yuck, there must be a better way.
	 */
	memset (&tmp_text, 0, sizeof (tmp_text));
	buff = PA_ALLOC(1);
	if (buff == NULL)
	{
		state->top_state->out_of_memory = TRUE;
		return(NULL);
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

	dynamic_width = FALSE;
	dynamic_height = FALSE;
	if (subdoc->width == 0)
	{
		dynamic_width = TRUE;
		/*
		 * Subdocs that were dynamically widthed, and which
		 * contained right aligned items, must be relaid out
		 * once their width is determined.
		 */
		eptr = old_state->float_list;
		while (eptr != NULL)
		{
			if ((eptr->type == LO_IMAGE)&&
				(eptr->lo_image.image_attr->alignment == LO_ALIGN_RIGHT))
			{
				old_state->must_relayout_subdoc = TRUE;
				break;
			}
			else if ((eptr->type == LO_SUBDOC)&&
				(eptr->lo_subdoc.alignment == LO_ALIGN_RIGHT))
			{
				old_state->must_relayout_subdoc = TRUE;
				break;
			}
			eptr = eptr->lo_any.next;
		}

		subdoc->width = old_state->max_width +
			(2 * subdoc->border_width);
	}

	/*
	 * Figure the height the subdoc laid out in.
	 * Use this if we were passed no height, or if it is greater
	 * than the passed height.
	 */
	doc_height = old_state->y + (2 * subdoc->border_width);
	if (subdoc->height == 0)
	{
		dynamic_height = TRUE;
		subdoc->height = doc_height;
	}
	else if (subdoc->height < doc_height)
	{
		subdoc->height = doc_height;
	}

	/*
	 * Make eptr point to the start of the element chain
	 * for this subdoc.
	 */
#ifdef XP_WIN16
{
	XP_Block *larray_array;

	XP_LOCK_BLOCK(larray_array, XP_Block *, old_state->larray_array);
	old_state->line_array = larray_array[0];
	XP_UNLOCK_BLOCK(old_state->larray_array);
}
#endif /* XP_WIN16 */
	XP_LOCK_BLOCK(line_array, LO_Element **, old_state->line_array);
	eptr = line_array[0];
	XP_UNLOCK_BLOCK(old_state->line_array);

	while (eptr != NULL)
	{
		switch (eptr->type)
		{
		    case LO_LINEFEED:
			if ((eptr->lo_linefeed.x + eptr->lo_linefeed.x_offset +
				eptr->lo_linefeed.width) >= (subdoc->width - 1 -
				(2 * subdoc->border_width)))
			{
				eptr->lo_linefeed.width = (subdoc->width - 1 -
					(2 * subdoc->border_width)) -
					(eptr->lo_linefeed.x +
					eptr->lo_linefeed.x_offset);
				if (eptr->lo_linefeed.width < 0)
				{
					eptr->lo_linefeed.width = 0;
				}
			}
#ifdef OLD_DELAY_CENTERED
			if (eptr->lo_linefeed.ele_attrmask &
				LO_ELE_DELAY_CENTERED)
			{
				int32 side_inc;
				LO_Element *c_eptr;

				c_eptr = eptr;

				side_inc = subdoc->width - 1 -
					(2 * subdoc->border_width) -
					(eptr->lo_linefeed.x +
					eptr->lo_linefeed.x_offset);
				side_inc = side_inc / 2;
				if (side_inc > 0)
				{
					c_eptr->lo_linefeed.width -= side_inc;
					if (c_eptr->lo_linefeed.width < 0)
					{
						c_eptr->lo_linefeed.width = 0;
					}
					while ((c_eptr->lo_any.prev != NULL)&&
						(c_eptr->lo_any.prev->type !=
							LO_LINEFEED))
					{
						c_eptr->lo_any.x += side_inc;
						if (c_eptr->type == LO_CELL)
						{
						    lo_ShiftCell(
							(LO_CellStruct *)c_eptr,
							side_inc, 0);
						}
						c_eptr = c_eptr->lo_any.prev;
					}
					c_eptr->lo_any.x += side_inc;
					if (c_eptr->type == LO_CELL)
					{
					    lo_ShiftCell(
						(LO_CellStruct *)c_eptr,
						side_inc, 0);
					}
				}
				eptr->lo_linefeed.ele_attrmask &=
					(~(LO_ELE_DELAY_CENTERED));
			}
#endif /* OLD_DELAY_CENTERED */
			break;
		    /*
		     * This is awful, but any dynamically dimenstioned
		     * subdoc that contains one of these elements
		     * must be relayed out to take care of window
		     * widht and height dependencies.
		     */
		    case LO_HRULE:
		    case LO_SUBDOC:
		    case LO_TABLE:
			old_state->must_relayout_subdoc = TRUE;
			break;
		    case LO_IMAGE:
/* WILL NEED THIS LATER FOR PERCENT WIDTH IMAGES
			if ((old_state->allow_percent_width == FALSE)||
				(old_state->allow_percent_height == FALSE))
			{
				old_state->must_relayout_subdoc = TRUE;
			}
*/
			break;
		    default:
			break;
		}
		eptr = eptr->lo_any.next;
	}

	subdoc_width = subdoc->width + (2 * subdoc->border_width) +
		(2 * subdoc->border_horiz_space);
	subdoc_height = subdoc->height + (2 * subdoc->border_width) +
		(2 * subdoc->border_vert_space);

	if (old_state->display_blocked != FALSE)
	{
		old_state->display_blocked = FALSE;
	}

	/*
	 * Will this subdoc make the line too wide.
	 */
	if ((state->x + subdoc_width) > state->right_margin)
	{
		line_break = TRUE;
	}
	else
	{
		line_break = FALSE;
	}

	/*
	 * if we are at the beginning of the line.  There is
	 * no point in breaking, we are just too wide.
	 * Also don't break in unwrappedpreformatted text.
	 * Also can't break inside a NOBR section.
	 */
	if ((state->at_begin_line != FALSE)||
		(state->preformatted == PRE_TEXT_YES)||
		(state->breakable == FALSE))
	{
		line_break = FALSE;
	}

	/*
         * break on the subdoc if we have
         * a break.
         */
/* DOn't break in cell
        if (line_break != FALSE)
        {
                lo_HardLineBreak(context, state, TRUE);
                subdoc->x = state->x;
                subdoc->y = state->y;
        }
*/

	/*
	 * Figure out how to align this subdoc.
	 * baseline_inc is how much to increase the baseline
	 * of previous element of this line.  line_inc is how
	 * much to increase the line height below the baseline.
	 */
	baseline_inc = 0;
	line_inc = 0;
	/*
	 * If we are at the beginning of a line, with no baseline,
	 * we first set baseline and line_height based on the current
	 * font, then place the subdoc.
	 */
	if (state->baseline == 0)
	{
		state->baseline = 0;
	}

	lo_CalcAlignOffsets(state, &text_info, (intn)subdoc->alignment,
		subdoc_width, subdoc_height,
		&subdoc->x_offset, &subdoc->y_offset, &line_inc, &baseline_inc);

	subdoc->x_offset += (int16)subdoc->border_horiz_space;
	subdoc->y_offset += (int32)subdoc->border_vert_space;

	old_state->base_x = subdoc->x + subdoc->x_offset +
		subdoc->border_width;
	old_state->base_y = subdoc->y + subdoc->y_offset +
		subdoc->border_width;

	/*
	 * Clean up state
	state->x = state->x + subdoc->x_offset +
		subdoc_width - subdoc->border_horiz_space;
	state->linefeed_state = 0;
	state->at_begin_line = FALSE;
	state->trailing_space = FALSE;
	state->cur_ele_type = LO_SUBDOC;
	 */

	return(subdoc);
}


void
lo_relayout_recycle(MWContext *context, lo_DocState *state, LO_Element *elist)
{
	while (elist != NULL)
	{
		LO_Element *eptr;

		eptr = elist;
		elist = elist->lo_any.next;
		eptr->lo_any.next = NULL;

		if (eptr->type == LO_IMAGE)
		{
			eptr->lo_any.next = state->top_state->trash;
			state->top_state->trash = eptr;
		}
		else if (eptr->type == LO_CELL)
		{
			if (eptr->lo_cell.cell_list != NULL)
			{
			    if (eptr->lo_cell.cell_list_end != NULL)
			    {
				eptr->lo_cell.cell_list_end->lo_any.next = NULL;
			    }
			    lo_relayout_recycle(context, state,
				eptr->lo_cell.cell_list);
			    eptr->lo_cell.cell_list = NULL;
			    eptr->lo_cell.cell_list_end = NULL;
			}
			if (eptr->lo_cell.cell_float_list != NULL)
			{
			    lo_relayout_recycle(context, state,
				eptr->lo_cell.cell_float_list);
			    eptr->lo_cell.cell_float_list = NULL;
			}
			lo_RecycleElements(context, state, eptr);
		}
		else
		{
			/*
			 * If this is an embed, tell it that it should
			 * recycle rather than delete itself.
			 */
			if (eptr->type == LO_EMBED)
				NPL_SameElement((LO_EmbedStruct*)eptr);

			lo_RecycleElements(context, state, eptr);
		}
	}
}


static void
lo_cleanup_state(MWContext *context, lo_DocState *state)
{
	LO_Element **line_array;

	if (state == NULL)
	{
		return;
	}

	if (state->left_margin_stack != NULL)
	{
		lo_MarginStack *mptr;
		lo_MarginStack *margin;

		mptr = state->left_margin_stack;
		while (mptr != NULL)
		{
			margin = mptr;
			mptr = mptr->next;
			XP_DELETE(margin);
		}
		state->left_margin_stack = NULL;
	}
	if (state->right_margin_stack != NULL)
	{
		lo_MarginStack *mptr;
		lo_MarginStack *margin;

		mptr = state->right_margin_stack;
		while (mptr != NULL)
		{
			margin = mptr;
			mptr = mptr->next;
			XP_DELETE(margin);
		}
		state->right_margin_stack = NULL;
	}

	if (state->line_list != NULL)
	{
		lo_relayout_recycle(context, state, state->line_list);
		state->line_list = NULL;
	}

	if (state->font_stack != NULL)
	{
		lo_FontStack *fstack;
		lo_FontStack *fptr;

		fptr = state->font_stack;
		while (fptr != NULL)
		{
			fstack = fptr;
			fptr = fptr->next;
			XP_DELETE(fstack);
		}
		state->font_stack = NULL;
	}

	if (state->align_stack != NULL)
	{
		lo_AlignStack *aptr;
		lo_AlignStack *align;

		aptr = state->align_stack;
		while (aptr != NULL)
		{
			align = aptr;
			aptr = aptr->next;
			XP_DELETE(align);
		}
		state->align_stack = NULL;
	}

	if (state->list_stack != NULL)
	{
		lo_ListStack *lptr;
		lo_ListStack *list;

		lptr = state->list_stack;
		while (lptr != NULL)
		{
			list = lptr;
			lptr = lptr->next;
			XP_DELETE(list);
		}
		state->list_stack = NULL;
	}

	if (state->line_buf != NULL)
	{
		PA_FREE(state->line_buf);
		state->line_buf = NULL;
	}

	if (state->float_list != NULL)
	{
		lo_relayout_recycle(context, state, state->float_list);
		state->float_list = NULL;
	}

	if (state->line_array != NULL)
	{
		LO_Element *eptr;

#ifdef XP_WIN16
{
		XP_Block *larray_array;
		intn cnt;

		XP_LOCK_BLOCK(larray_array, XP_Block *, state->larray_array);
		cnt = state->larray_array_size - 1;
		while (cnt > 0)
		{
			XP_FREE_BLOCK(larray_array[cnt]);
			cnt--;
		}
		state->line_array = larray_array[0];
		XP_UNLOCK_BLOCK(state->larray_array);
		XP_FREE_BLOCK(state->larray_array);
}
#endif /* XP_WIN16 */

		eptr = NULL;
		XP_LOCK_BLOCK(line_array, LO_Element **, state->line_array);
		eptr = line_array[0];
		if (eptr != NULL)
		{
			lo_relayout_recycle(context, state, eptr);
		}
		XP_UNLOCK_BLOCK(state->line_array);
		XP_FREE_BLOCK(state->line_array);
	}

	XP_DELETE(state);
}


static lo_FormData *
lo_merge_form_data(lo_FormData *new_form_list, lo_FormData *hold_form_list)
{
	intn i;
	LO_FormElementStruct **new_ele_list;
	LO_FormElementStruct **hold_ele_list;

	PA_LOCK(new_ele_list, LO_FormElementStruct **,
			new_form_list->form_elements);
	PA_LOCK(hold_ele_list, LO_FormElementStruct **,
			hold_form_list->form_elements);
	for (i=0; i<new_form_list->form_ele_cnt; i++)
	{
		if (hold_ele_list[i] != new_ele_list[i])
		{
#ifdef MOCHA
			if (new_ele_list[i]->mocha_object == NULL)
			{
				new_ele_list[i]->mocha_object
				    = hold_ele_list[i]->mocha_object;
			}
#endif
			hold_ele_list[i] = new_ele_list[i];
		}
	}
	PA_UNLOCK(hold_form_list->form_elements);
	PA_UNLOCK(new_form_list->form_elements);

#ifdef MOCHA
	/* XXX sometimes the second pass does not reflect a form object! */
	if (new_form_list->mocha_object != NULL)
	{
		hold_form_list->mocha_object = new_form_list->mocha_object;
	}
#endif
	return(hold_form_list);
}


/*
 * Serious table magic.  Given a table cell subdoc,
 * redo the layout to the new width from the stored tag list,
 * and create a whole new subdoc, which is returned.
 */
LO_SubDocStruct *
lo_RelayoutSubdoc(MWContext *context, lo_DocState *state,
	LO_SubDocStruct *subdoc, int32 width, Bool is_a_header)
{
	int32 first_height;
	lo_TopState *top_state;
	lo_DocState *old_state;
	lo_DocState *new_state;
	PA_Tag *tag_list;
	PA_Tag *tag_ptr;
	PA_Tag *tag_end_ptr;
	int32 doc_id;
	Bool allow_percent_width;
	Bool allow_percent_height;
	int32 hold_form_ele_cnt;
	int32 hold_form_data_index;
	intn hold_form_id;
	intn hold_current_form_num;
	Bool hold_in_form;
	lo_FormData *hold_form_list;
	int32 hold_embed_global_index;
	intn hold_url_list_len;
	NET_ReloadMethod save_force;

	/*
	 * Relayout is NOT allowed to block on images, so never
	 * have the forced reload flag set.
	 */
	save_force = state->top_state->force_reload;
	state->top_state->force_reload = NET_CACHE_ONLY_RELOAD;

#ifdef LOCAL_DEBUG
 fprintf(stderr, "lo_RelayoutSubdoc called\n");
#endif /* LOCAL_DEBUG */

	top_state = state->top_state;
	old_state = (lo_DocState *) subdoc->state;
	
	/*
	 * We are going to reuse this subdoc element instead
	 * of allocating a new one.
	 */
	subdoc->state = NULL;
	subdoc->width = width;
	subdoc->height = 0;

	/*
	 * If this subdoc contained form elements.  We want to reuse
	 * them without screwing up the order of later form elements in
	 * other subdocs.
	 *
	 * We assume that relaying out the doc will create the exact
	 * same number of form elements in the same order as the first time.
	 * so we reset global form counters to before this subdoc
	 * was ever laid out, saving the current counts to restore later.
	 */
	if (top_state->form_list != NULL)
	{
		lo_FormData *form_list;
		lo_FormData *tmp_fptr;

		hold_form_id = old_state->form_id;
		hold_current_form_num = top_state->current_form_num;
		form_list = top_state->form_list;
		hold_form_list = NULL;
		while ((form_list != NULL)&&
			(form_list->id > hold_form_id))
		{
			tmp_fptr = form_list;
			form_list = form_list->next;
			tmp_fptr->next = hold_form_list;
			hold_form_list = tmp_fptr;
		}
		top_state->form_list = form_list;
		top_state->current_form_num = hold_form_id;
		if (form_list != NULL)
		{
			hold_form_ele_cnt = form_list->form_ele_cnt;
			form_list->form_ele_cnt = old_state->form_ele_cnt;
		}
		else
		{
			hold_form_ele_cnt = 0;
		}
	}
	else
	{
		hold_form_id = 0;
		hold_form_ele_cnt = 0;
		hold_form_list = NULL;
		hold_current_form_num = 0;
	}

	/*
	 * We're going to be deleting and then recreating the contents 
	 * of this subdoc.  We want the newly-created embeds to have
	 * the same embed indices as their recycled counterparts, so
	 * we reset the master embed index in the top_state to be the
	 * value it held at the time the old subdoc was created. When
	 * we're finished creating the new subdoc, the master index
	 * should be back to its original value (but just in case we
	 * save that value in a local variable here).
	 */
	hold_embed_global_index = top_state->embed_count;
	top_state->embed_count = old_state->embed_count_base;

	/*
	 * Same goes for anchors.
	 */
	hold_url_list_len = top_state->url_list_len;
	top_state->url_list_len = old_state->url_count_base;

	/*
	 * Save whether we are in a form or not, and restore
	 * to what we were when the subdoc was started.
	 */
	hold_in_form = top_state->in_form;
	top_state->in_form = old_state->start_in_form;

	if (top_state->savedData.FormList != NULL)
	{
		lo_SavedFormListData *all_form_ele;

		all_form_ele = top_state->savedData.FormList;
		hold_form_data_index = all_form_ele->data_index;
		all_form_ele->data_index = old_state->form_data_index;
	}
	else
	{
		hold_form_data_index = 0;
	}

	/*
	 * Still variable height, but we know the width now.
	 */
	first_height = FEUNITS_Y(10000, context);
	allow_percent_height = FALSE;
	allow_percent_width = TRUE;

	/*
	 * Get a new sub-state to create the new subdoc in.
	 */
	new_state = lo_NewLayout(context, width, first_height, 0, 0, NULL);
	if (new_state == NULL)
	{
		state->top_state->force_reload = save_force;
		return(subdoc);
	}
	if (old_state->is_a_subdoc == SUBDOC_CAPTION)
	{
		new_state->is_a_subdoc = SUBDOC_CAPTION;
	}
	else
	{
		new_state->is_a_subdoc = SUBDOC_CELL;
	}
	lo_InheritParentState(new_state, state);
	new_state->allow_percent_width = allow_percent_width;
	new_state->allow_percent_height = allow_percent_height;
	/*
	 * In relaying out subdocs, we NEVER link back up subdoc tags
	 */
	new_state->subdoc_tags = NULL;
	new_state->subdoc_tags_end = NULL;

	/*
	 * Initialize other new state sundries.
	 */
	new_state->base_x = 0;
	new_state->base_y = 0;
	new_state->display_blocked = TRUE;
	if (subdoc->width == 0)
	{
		new_state->delay_align = TRUE;
	}

	new_state->win_left = 0;
	new_state->win_right = 0;
	new_state->x = new_state->win_left;
	new_state->y = 0;
	new_state->max_width = new_state->x + 1;
	new_state->left_margin = new_state->win_left;
	new_state->right_margin = new_state->win_width - new_state->win_right;
	new_state->list_stack->old_left_margin = new_state->left_margin;
	new_state->list_stack->old_right_margin = new_state->right_margin;

	/*
	 * Initialize the alignment stack for this state
	 * to match the caption alignment.
	 */
	if (subdoc->horiz_alignment != LO_ALIGN_LEFT)
	{
		PA_Tag tmp_tag;

		tmp_tag.type = P_TABLE_DATA;
		lo_PushAlignment(new_state, &tmp_tag, subdoc->horiz_alignment);
	}

	state->sub_state = new_state;

	state->current_ele = (LO_Element *)subdoc;

	if (is_a_header != FALSE)
	{
		LO_TextAttr *old_attr;
		LO_TextAttr *attr;
		LO_TextAttr tmp_attr;

		old_attr = new_state->font_stack->text_attr;
		lo_CopyTextAttr(old_attr, &tmp_attr);
		tmp_attr.fontmask |= LO_FONT_BOLD;
		attr = lo_FetchTextAttr(new_state, &tmp_attr);

		lo_PushFont(new_state, P_TABLE_HEADER, attr);
	}

	/*
	 * Move our current state down into the new sub-state.
	 * Fetch the tag list from the old subdoc's state in preparation
	 * for relayout.
	 */
	state = new_state;
	tag_ptr = old_state->subdoc_tags;
	tag_end_ptr = old_state->subdoc_tags_end;
	old_state->subdoc_tags = NULL;
	old_state->subdoc_tags_end = NULL;
	if (tag_end_ptr != NULL)
	{
		tag_end_ptr = tag_end_ptr->next;
	}

	/*
	 * Clean up memory used by old subdoc state.
	 */
	lo_cleanup_state(context, old_state);

	while (tag_ptr != tag_end_ptr)
	{
		PA_Tag *tag;
		lo_DocState *sub_state;
		lo_DocState *up_state;
		lo_DocState *tmp_state;
		Bool may_save;

		tag = tag_ptr;
		tag_ptr = tag_ptr->next;
		tag->next = NULL;
		tag_list = tag_ptr;

		up_state = NULL;
		sub_state = state;
		while (sub_state->sub_state != NULL)
		{
			up_state = sub_state;
			sub_state = sub_state->sub_state;
		}

		if ((sub_state->is_a_subdoc == SUBDOC_CELL)||
			(sub_state->is_a_subdoc == SUBDOC_CAPTION))
		{
			may_save = TRUE;
		}
		else
		{
			may_save = FALSE;
		}

#ifdef MOCHA
		sub_state->in_relayout = TRUE;
#endif
		lo_LayoutTag(context, sub_state, tag);
		tmp_state = lo_CurrentSubState(state);
#ifdef MOCHA
		if (tmp_state == sub_state)
		{
			sub_state->in_relayout = FALSE;
		}
#endif

		if (may_save != FALSE)
		{
			/*
			 * That tag popped us up one state level.  If this new
			 * state is still a subdoc, save the tag there.
			 */
			if (tmp_state == up_state)
			{
				if ((tmp_state->is_a_subdoc == SUBDOC_CELL)||
				    (tmp_state->is_a_subdoc == SUBDOC_CAPTION))
				{
				    lo_SaveSubdocTags(context, tmp_state, tag);
				}
				else
				{
				    PA_FreeTag(tag);
				}
			}
			/*
			 * Else that tag put us in a new subdoc on the same
			 * level.  It needs to be saved one level up,
			 * if the parent is also a subdoc.
			 */
			else if ((up_state != NULL)&&
				(tmp_state == up_state->sub_state)&&
				(tmp_state != sub_state))
			{
				if ((up_state->is_a_subdoc == SUBDOC_CELL)||
				     (up_state->is_a_subdoc == SUBDOC_CAPTION))
				{
				    lo_SaveSubdocTags(context, up_state, tag);
				}
				else
				{
				    PA_FreeTag(tag);
				}
			}
			/*
			 * Else we are still in the same subdoc
			 */
			else if (tmp_state == sub_state)
			{
				lo_SaveSubdocTags(context, sub_state, tag);
			}
			/*
			 * Else that tag started a new, nested subdoc.
			 * Add the starting tag to the parent.
			 */
			else if (tmp_state == sub_state->sub_state)
			{
				lo_SaveSubdocTags(context, sub_state, tag);
				/*
				 * Since we have extended the parent chain,
				 * we need to reset the child to the new
				 * parent end-chain.
				 */
				if ((tmp_state->is_a_subdoc == SUBDOC_CELL)||
				    (tmp_state->is_a_subdoc == SUBDOC_CAPTION))
				{
					tmp_state->subdoc_tags =
						sub_state->subdoc_tags_end;
				}
			}
			/*
			 * This can never happen.
			 */
			else
			{
				PA_FreeTag(tag);
			}
		}
		tag_ptr = tag_list;
	}

	/*
	 * Link the end of this subdoc's tag chain back to
	 * its parent.  The beginning should have never been
	 * unlinked from the parent.
	 */
	if (state->subdoc_tags_end != NULL)
	{
		state->subdoc_tags_end->next = tag_end_ptr;
	}

	/*
	 * makes sure we are at the bottom
	 * of everything in the document.
	 */
	lo_CloseOutLayout(context, state);

	old_state = state;
	/*
	 * Get the unique document ID, and retreive this
	 * documents layout state.
	 */
	doc_id = XP_DOCID(context);
	top_state = lo_FetchTopState(doc_id);
	state = top_state->doc_state;

	while ((state->sub_state != NULL)&&
		(state->sub_state != old_state))
	{
		state = state->sub_state;
	}

	/*
	 * Ok, now reset the global form element counters
	 * to their original values.
	 */
	if ((top_state->form_list != NULL)||(hold_form_list != NULL))
	{
		lo_FormData *form_list;
		lo_FormData *new_form_list;
		lo_FormData *tmp_fptr;

		/*
		 * Remove any new forms created in this subdocs to a
		 * separate list.
		 */
		form_list = top_state->form_list;
		new_form_list = NULL;
		while ((form_list != NULL)&&
			(form_list->id > hold_form_id))
		{
			tmp_fptr = form_list;
			form_list = form_list->next;
			tmp_fptr->next = new_form_list;
			new_form_list = tmp_fptr;
		}

		/*
		 * Restore the element count in the form we started in
		 * because the subdoc may have left it half finished.
		 */
		if (form_list != NULL)
		{
			form_list->form_ele_cnt = hold_form_ele_cnt;
		}

		/*
		 * Merge the newly created forms with the old ones
		 * since the new ones may have partially complete
		 * data.
		 */
		while ((new_form_list != NULL)&&(hold_form_list != NULL))
		{
			tmp_fptr = lo_merge_form_data(new_form_list,
					hold_form_list);
			new_form_list = new_form_list->next;
			hold_form_list = hold_form_list->next;
			tmp_fptr->next = form_list;
			form_list = tmp_fptr;
		}

		/*
		 * Add back on forms that are from later subdocs.
		 */
		while (hold_form_list != NULL)
		{
			tmp_fptr = hold_form_list;
			hold_form_list = hold_form_list->next;
			tmp_fptr->next = form_list;
			form_list = tmp_fptr;
		}

		top_state->form_list = form_list;
		top_state->current_form_num = hold_current_form_num;
	}

	/*
	 * Restore in form status to what is
	 * was before this relayout.
	 */
	top_state->in_form = hold_in_form;

	if (top_state->savedData.FormList != NULL)
	{
		lo_SavedFormListData *all_form_ele;

		all_form_ele = top_state->savedData.FormList;
		all_form_ele->data_index = hold_form_data_index;
	}

	/*
	 * The master embed index is typically now be back
	 * to its original value, but might not be if we
	 * only had to relayout some (not all) of the embeds
	 * in this subdoc.  So, make sure it's set back to
	 * its original value here.
	 */
	top_state->embed_count = hold_embed_global_index;

	/*
	 * Likewise for the anchor list.
	 */
	top_state->url_list_len = hold_url_list_len;

	subdoc = NULL;
	if ((state != old_state)&&
		(state->current_ele != NULL))
	{
		subdoc = lo_EndCellSubDoc(context, state,
				old_state, state->current_ele);
		state->sub_state = NULL;
		state->current_ele = NULL;
/*
		if ((state->is_a_subdoc == SUBDOC_CELL)||
			(state->is_a_subdoc == SUBDOC_CAPTION))
		{
			if (old_state->subdoc_tags_end != NULL)
			{
				state->subdoc_tags_end =
					old_state->subdoc_tags_end;
			}
		}
*/
	}

	old_state->must_relayout_subdoc = FALSE;

	state->top_state->force_reload = save_force;
	return(subdoc);
}


void
lo_BeginTableCell(MWContext *context, lo_DocState *state, lo_TableRec *table,
	PA_Tag *tag, Bool is_a_header)
{
	lo_TableRow *table_row;
	lo_TableCell *table_cell;
	PA_Block buff;
	char *str;
	int32 val;
#ifdef LOCAL_DEBUG
 fprintf(stderr, "lo_BeginTableCell called\n");
#endif /* LOCAL_DEBUG */

	table_row = table->row_ptr;

	table_cell = XP_NEW(lo_TableCell);
	if (table_cell == NULL)
	{
		return;
	}

	table_cell->cell_done = FALSE;
	table_cell->nowrap = FALSE;
	table_cell->is_a_header = is_a_header;
	table_cell->percent_width = 0;
	table_cell->percent_height = 0;
	table_cell->min_width = 0;
	table_cell->max_width = 0;
	table_cell->height = 0;
	table_cell->baseline = 0;
	table_cell->rowspan = 1;
	table_cell->colspan = 1;
	table_cell->cell_subdoc = NULL;
	table_cell->next = NULL;

	buff = lo_FetchParamValue(context, tag, PARAM_COLSPAN);
	if (buff != NULL)
	{
		PA_LOCK(str, char *, buff);
		val = XP_ATOI(str);
		if (val < 1)
		{
			val = 1;
		}
		table_cell->colspan = val;
		PA_UNLOCK(buff);
		PA_FREE(buff);
	}

	buff = lo_FetchParamValue(context, tag, PARAM_ROWSPAN);
	if (buff != NULL)
	{
		PA_LOCK(str, char *, buff);
		val = XP_ATOI(str);
		if (val < 1)
		{
			val = 1;
		}
		table_cell->rowspan = val;
		PA_UNLOCK(buff);
		PA_FREE(buff);
	}

	buff = lo_FetchParamValue(context, tag, PARAM_NOWRAP);
	if (buff != NULL)
	{
		table_cell->nowrap = TRUE;
		PA_FREE(buff);
	}

	if (table_row->cell_list == NULL)
	{
		table_row->cell_list = table_cell;
		table_row->cell_ptr = table_cell;
	}
	else
	{
		table_row->cell_ptr->next = table_cell;
		table_row->cell_ptr = table_cell;
	}

	lo_BeginCellSubDoc(context, state, table, tag,
		is_a_header, table->draw_borders);
}


void
lo_EndTableCell(MWContext *context, lo_DocState *state)
{
	LO_SubDocStruct *subdoc;
	lo_TableRec *table;
	lo_TableRow *table_row;
	lo_TableCell *table_cell;
	lo_table_span *span_rec;
	int32 inner_pad;
	int32 i;
	lo_TopState *top_state;
	lo_DocState *old_state;
	int32 doc_id;
#ifdef LOCAL_DEBUG
 fprintf(stderr, "lo_EndTableCell called\n");
#endif /* LOCAL_DEBUG */

	/*
	 * makes sure we are at the bottom
	 * of everything in the document.
	 */
	lo_CloseOutLayout(context, state);

	old_state = state;
	/*
	 * Get the unique document ID, and retreive this
	 * documents layout state.
	 */
	doc_id = XP_DOCID(context);
	top_state = lo_FetchTopState(doc_id);
	state = top_state->doc_state;

	while ((state->sub_state != NULL)&&
		(state->sub_state != old_state))
	{
		state = state->sub_state;
	}

	subdoc = NULL;
	if ((state != old_state)&&
		(state->current_ele != NULL))
	{
		subdoc = lo_EndCellSubDoc(context, state,
				old_state, state->current_ele);
		state->sub_state = NULL;
		state->current_ele = NULL;
		if ((state->is_a_subdoc == SUBDOC_CELL)||
			(state->is_a_subdoc == SUBDOC_CAPTION))
		{
			if (old_state->subdoc_tags_end != NULL)
			{
				state->subdoc_tags_end =
					old_state->subdoc_tags_end;
			}
		}
		else
		{
		}
	}

	table = state->current_table;
	table_row = table->row_ptr;
	table_cell = table_row->cell_ptr;

	inner_pad = FEUNITS_X(table->inner_cell_pad, context);

	if (subdoc != NULL)
	{
		int32 subdoc_width;
		int32 subdoc_height;

		subdoc_width = subdoc->width +
			(2 * subdoc->border_horiz_space) +
			(2 * inner_pad);
		subdoc_height = subdoc->height +
			(2 * subdoc->border_vert_space) +
			(2 * inner_pad);
		table_cell->max_width = subdoc_width;
		table_cell->min_width = old_state->min_width +
			(2 * subdoc->border_width) +
			(2 * subdoc->border_horiz_space) +
			(2 * inner_pad);
		if ((table_cell->nowrap != FALSE)||
			(table_cell->min_width > table_cell->max_width))
		{
			table_cell->min_width = table_cell->max_width;
		}
		table_cell->height = subdoc_height;
		table_cell->baseline = lo_GetSubDocBaseline(subdoc);
	}

	table_cell->cell_subdoc = subdoc;
	table_cell->cell_done = TRUE;

	i = 0;
	while (i < table_cell->colspan)
	{
		if ((table->width_spans == NULL)||
		    ((table->width_span_ptr != NULL)&&
		     (table->width_span_ptr->next == NULL)))
		{
			span_rec = XP_NEW(lo_table_span);
			if (span_rec == NULL)
			{
				return;
			}
			span_rec->dim = 1;
			span_rec->min_dim = 1;
			span_rec->span = 0;
			span_rec->next = NULL;

			if (table->width_spans == NULL)
			{
				table->width_spans = span_rec;
				table->width_span_ptr = span_rec;
			}
			else
			{
				table->width_span_ptr->next = span_rec;
				table->width_span_ptr = span_rec;
			}
		}
		else
		{
			if (table->width_span_ptr == NULL)
			{
				table->width_span_ptr = table->width_spans;
			}
			else
			{
				table->width_span_ptr =
					table->width_span_ptr->next;
			}
			span_rec = table->width_span_ptr;
		}

		if (span_rec->span > 0)
		{
			span_rec->span--;
			table_row->cells++;

			/*
			 * This only happens if a colspan on this
			 * row overlaps a rowspan from a previous row.
			 */
			if (i > 0)
			{
				int32 new_span;

				new_span = table_cell->rowspan - 1;
				if (new_span > span_rec->span)
				{
					span_rec->span = new_span;
				}
				i++;
				/*
				 * Don't count this cell twice.
				 */
				table_row->cells--;
			}
			continue;
		}
		else
		{
			span_rec->span = table_cell->rowspan - 1;
			i++;
			if (table_cell->colspan == 1)
			{
				if (table_cell->max_width > span_rec->dim)
				{
				    span_rec->dim = table_cell->max_width;
				}
				if (table_cell->min_width > span_rec->min_dim)
				{
				    span_rec->min_dim = table_cell->min_width;
				}
			}
		}
	}

	span_rec = table->height_span_ptr;
	if (table_cell->rowspan == 1)
	{
		int32 tmp_val;

		tmp_val = table_cell->baseline - span_rec->min_dim;
		if (tmp_val > 0)
		{
			span_rec->min_dim += tmp_val;
			if (table_row->vert_alignment == LO_ALIGN_BASELINE)
			{
				span_rec->dim += tmp_val;
			}
		}

		if (table_row->vert_alignment == LO_ALIGN_BASELINE)
		{
			tmp_val = (table_cell->height - table_cell->baseline) -
				(span_rec->dim - span_rec->min_dim);
			if (tmp_val > 0)
			{
				span_rec->dim += tmp_val;
			}
		}
		else
		{
			if (table_cell->height > span_rec->dim)
			{
				span_rec->dim = table_cell->height;
			}
		}
	}

	table_row->cells += table_cell->colspan;
}


void
lo_BeginTableCaption(MWContext *context, lo_DocState *state, lo_TableRec *table,
	PA_Tag *tag)
{
	lo_TableCaption *caption;
	PA_Block buff;
	char *str;

	caption = XP_NEW(lo_TableCaption);
	if (caption == NULL)
	{
		return;
	}

	caption->vert_alignment = LO_ALIGN_TOP;
	caption->horiz_alignment = LO_ALIGN_CENTER;
	caption->min_width = 0;
	caption->max_width = 0;
	caption->height = 0;
	caption->subdoc = NULL;

	/*
	 * Check for an align parameter
	 */
	buff = lo_FetchParamValue(context, tag, PARAM_ALIGN);
	if (buff != NULL)
	{
		PA_LOCK(str, char *, buff);
		if (pa_TagEqual("bottom", str))
		{
			caption->vert_alignment = LO_ALIGN_BOTTOM;
		}
		PA_UNLOCK(buff);
		PA_FREE(buff);
	}

	table->caption = caption;

	lo_BeginCaptionSubDoc(context, state, caption, tag);
}


void
lo_EndTableCaption(MWContext *context, lo_DocState *state)
{
	LO_SubDocStruct *subdoc;
	lo_TableRec *table;
	lo_TableCaption *caption;
	lo_TopState *top_state;
	lo_DocState *old_state;
	int32 doc_id;
	int32 inner_pad;

	/*
	 * makes sure we are at the bottom
	 * of everything in the document.
	 */
	lo_CloseOutLayout(context, state);

	old_state = state;
	/*
	 * Get the unique document ID, and retreive this
	 * documents layout state.
	 */
	doc_id = XP_DOCID(context);
	top_state = lo_FetchTopState(doc_id);
	state = top_state->doc_state;

	while ((state->sub_state != NULL)&&
		(state->sub_state != old_state))
	{
		state = state->sub_state;
	}

	subdoc = NULL;
	if ((state != old_state)&&
		(state->current_ele != NULL))
	{
		subdoc = lo_EndCellSubDoc(context, state,
				old_state, state->current_ele);
		state->sub_state = NULL;
		state->current_ele = NULL;
		if ((state->is_a_subdoc == SUBDOC_CELL)||
			(state->is_a_subdoc == SUBDOC_CAPTION))
		{
			if (old_state->subdoc_tags_end != NULL)
			{
				state->subdoc_tags_end =
					old_state->subdoc_tags_end;
			}
		}
	}

	table = state->current_table;
	caption = table->caption;

	inner_pad = FEUNITS_X(table->inner_cell_pad, context);

	if (subdoc != NULL)
	{
		int32 subdoc_width;
		int32 subdoc_height;

		subdoc_width = subdoc->width +
			(2 * subdoc->border_horiz_space) +
			(2 * inner_pad);
		subdoc_height = subdoc->height +
			(2 * subdoc->border_vert_space) + 
			(2 * inner_pad);
		caption->max_width = subdoc_width;
		caption->min_width = old_state->min_width +
			(2 * subdoc->border_width) +
			(2 * subdoc->border_horiz_space) +
			(2 * inner_pad);
		if (caption->min_width > caption->max_width)
		{
			caption->min_width = caption->max_width;
		}
		caption->height = subdoc_height;
	}

	caption->subdoc = subdoc;
}


void
lo_BeginTableRow(MWContext *context, lo_DocState *state, lo_TableRec *table,
	PA_Tag *tag)
{
	lo_TableRow *table_row;
	lo_table_span *span_rec;
	PA_Block buff;
	char *str;
#ifdef LOCAL_DEBUG
 fprintf(stderr, "lo_BeginTableRow called\n");
#endif /* LOCAL_DEBUG */

	table_row = XP_NEW(lo_TableRow);
	if (table_row == NULL)
	{
		return;
	}

	table_row->row_done = FALSE;
	table_row->has_percent_width_cells = FALSE;
	table_row->bg_color = NULL;
	table_row->cells = 0;
	table_row->vert_alignment = LO_ALIGN_DEFAULT;
	table_row->horiz_alignment = LO_ALIGN_DEFAULT;
	table_row->cell_list = NULL;
	table_row->cell_ptr = NULL;
	table_row->next = NULL;

	/*
	 * Check for a background color attribute
	 */
	buff = lo_FetchParamValue(context, tag, PARAM_BGCOLOR);
	if (buff != NULL)
	{
		uint8 red, green, blue;

		PA_LOCK(str, char *, buff);
		LO_ParseRGB(str, &red, &green, &blue);
		PA_UNLOCK(buff);
		PA_FREE(buff);
		table_row->bg_color = XP_NEW(LO_Color);
		if (table_row->bg_color != NULL)
		{
			table_row->bg_color->red = red;
			table_row->bg_color->green = green;
			table_row->bg_color->blue = blue;
		}
	}

	/*
	 * Check for a vertical align parameter
	 */
	buff = lo_FetchParamValue(context, tag, PARAM_VALIGN);
	if (buff != NULL)
	{
		PA_LOCK(str, char *, buff);
		table_row->vert_alignment = lo_EvalVAlignParam(str);
		PA_UNLOCK(buff);
		PA_FREE(buff);
	}

	/*
	 * Check for a horizontal align parameter
	 */
	buff = lo_FetchParamValue(context, tag, PARAM_ALIGN);
	if (buff != NULL)
	{
		PA_LOCK(str, char *, buff);
		table_row->horiz_alignment = lo_EvalCellAlignParam(str);
		PA_UNLOCK(buff);
		PA_FREE(buff);
	}

	if (table->row_list == NULL)
	{
		table->row_list = table_row;
		table->row_ptr = table_row;
	}
	else
	{
		table->row_ptr->next = table_row;
		table->row_ptr = table_row;
	}

	table->width_span_ptr = NULL;
	span_rec = XP_NEW(lo_table_span);
	if (span_rec == NULL)
	{
		return;
	}
	span_rec->dim = 1;
	/*
	 * Since min_dim on the heights is never used.
	 * I am appropriating it to do baseline aligning.  It will
	 * start as 0, and eventually be the amount of baseline needed
	 * to align all these cells in this row
	 * by their baselines.
	 */
	span_rec->min_dim = 0;
	span_rec->span = 0;
	span_rec->next = NULL;
	if (table->height_spans == NULL)
	{
		table->height_spans = span_rec;
		table->height_span_ptr = span_rec;
	}
	else
	{
		table->height_span_ptr->next = span_rec;
		table->height_span_ptr = span_rec;
	}
}


void
lo_EndTableRow(MWContext *context, lo_DocState *state, lo_TableRec *table)
{
	lo_TableRow *table_row;
#ifdef LOCAL_DEBUG
 fprintf(stderr, "lo_EndTableRow called\n");
#endif /* LOCAL_DEBUG */

	table_row = table->row_ptr;
	table_row->row_done = TRUE;
	table->rows++;

	while (table->width_span_ptr != NULL)
	{
		table->width_span_ptr = table->width_span_ptr->next;
		if (table->width_span_ptr != NULL)
		{
			table->width_span_ptr->span--;
			table_row->cells++;
		}
	}

	if (table_row->cells > table->cols)
	{
		table->cols = table_row->cells;
	}
}


void
lo_BeginTable(MWContext *context, lo_DocState *state, PA_Tag *tag)
{
	lo_TableRec *table;
	LO_TableStruct *table_ele;
	PA_Block buff;
	char *str;
	int32 val;
	Bool allow_percent_width;
	Bool allow_percent_height;
#ifdef LOCAL_DEBUG
 fprintf(stderr, "lo_BeginTable called\n");
#endif /* LOCAL_DEBUG */

	if (state == NULL)
	{
		return;
	}

	table_ele = (LO_TableStruct *)lo_NewElement(context, state, LO_TABLE, NULL, 0);
	if (table_ele == NULL)
	{
		state->top_state->out_of_memory = TRUE;
		return;
	}

	table_ele->type = LO_TABLE;
	table_ele->ele_id = NEXT_ELEMENT;
	table_ele->x = state->x;
	table_ele->x_offset = 0;
	table_ele->y = state->y;
	table_ele->y_offset = 0;
	table_ele->width = 0;
	table_ele->height = 0;
	table_ele->line_height = 0;

	table_ele->FE_Data = NULL;
	table_ele->anchor_href = state->current_anchor;

	table_ele->alignment = LO_ALIGN_CENTER;

	table_ele->border_width = TABLE_DEF_BORDER;
	table_ele->border_vert_space = TABLE_DEF_VERTICAL_SPACE;
	table_ele->border_horiz_space = TABLE_DEF_HORIZONTAL_SPACE;

	table_ele->ele_attrmask = 0;

	table_ele->sel_start = -1;
	table_ele->sel_end = -1;

	table_ele->next = NULL;
	table_ele->prev = NULL;

	/*
	 * Check for an align parameter
	 */
	buff = lo_FetchParamValue(context, tag, PARAM_ALIGN);
	if (buff != NULL)
	{
		Bool floating;

		floating = FALSE;
		PA_LOCK(str, char *, buff);
		table_ele->alignment = lo_EvalAlignParam(str, &floating);
		/*
		 * Only allow left and right (floating) and center.
		 */
		if (floating != FALSE)
		{
			table_ele->ele_attrmask |= LO_ELE_FLOATING;
		}
		else
		{
			table_ele->alignment = LO_ALIGN_CENTER;
		}
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
		if ((val == 0)&&(*str == '0'))
		{
			val = -1;
		}
		else if (val < 1)
		{
			val = 1;
		}
		table_ele->border_width = val;
		PA_UNLOCK(buff);
		PA_FREE(buff);
	}
	table_ele->border_width = FEUNITS_X(table_ele->border_width, context);

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
		table_ele->border_vert_space = val;
		PA_UNLOCK(buff);
		PA_FREE(buff);
	}
	table_ele->border_vert_space = FEUNITS_Y(table_ele->border_vert_space,
						context);

	/*
	 * Get the extra horizontal space parameter.
	 */
	buff = lo_FetchParamValue(context, tag, PARAM_HSPACE);
	if (buff != NULL)
	{
		PA_LOCK(str, char *, buff);
		val = XP_ATOI(str);
		if (val < 0)
		{
			val = 0;
		}
		table_ele->border_horiz_space = val;
		PA_UNLOCK(buff);
		PA_FREE(buff);
	}
	table_ele->border_horiz_space = FEUNITS_X(table_ele->border_horiz_space,
						context);


	table = XP_NEW(lo_TableRec);
	if (table == NULL)
	{
		return;
	}

	if (table_ele->border_width < 0)
	{
		table->draw_borders = TABLE_BORDERS_GONE;
		table_ele->border_width = 0;
	}
	else if (table_ele->border_width == 0)
	{
		table->draw_borders = TABLE_BORDERS_OFF;
	}
	else
	{
		table->draw_borders = TABLE_BORDERS_ON;
	}
	table->has_percent_width_cells = FALSE;
	table->has_percent_height_cells = FALSE;
	table->bg_color = NULL;
	table->rows = 0;
	table->cols = 0;
	table->width_spans = NULL;
	table->width_span_ptr = NULL;
	table->height_spans = NULL;
	table->height_span_ptr = NULL;
	table->caption = NULL;
	table->table_ele = table_ele;
	table->row_list = NULL;
	table->row_ptr = NULL;
	table->width = 0;
	table->height = 0;
	table->inner_cell_pad = TABLE_DEF_INNER_CELL_PAD;
	table->inter_cell_pad = TABLE_DEF_INTER_CELL_PAD;

	/*
	 * You can't do percentage widths if the parent's
	 * width is still undecided.
	 */
	allow_percent_width = TRUE;
	allow_percent_height = TRUE;
	if ((state->is_a_subdoc == SUBDOC_CELL)||
		(state->is_a_subdoc == SUBDOC_CAPTION))
	{
		lo_TopState *top_state;
		lo_DocState *new_state;
		int32 doc_id;

		/*
		 * Get the unique document ID, and retreive this
		 * documents layout state.
		 */
		doc_id = XP_DOCID(context);
		top_state = lo_FetchTopState(doc_id);
		new_state = top_state->doc_state;

		while ((new_state->sub_state != NULL)&&
			(new_state->sub_state != state))
		{
			new_state = new_state->sub_state;
		}

		if ((new_state->sub_state == state)&&
			(new_state->current_ele != NULL))
		{
			LO_SubDocStruct *subdoc;

			subdoc = (LO_SubDocStruct *)new_state->current_ele;
			if (subdoc->width == 0)
			{
				allow_percent_width = FALSE;
#ifdef LOCAL_DEBUG
 fprintf(stderr, "Percent width not allowed in this subdoc!\n");
#endif /* LOCAL_DEBUG */
			}
			else
			{
#ifdef LOCAL_DEBUG
 fprintf(stderr, "Percent width IS allowed in this subdoc!\n");
#endif /* LOCAL_DEBUG */
			}
			if (subdoc->height == 0)
			{
				allow_percent_height = FALSE;
			}
		}
	}

	/*
	 * Check for a background color attribute
	 */
	buff = lo_FetchParamValue(context, tag, PARAM_BGCOLOR);
	if (buff != NULL)
	{
		uint8 red, green, blue;

		PA_LOCK(str, char *, buff);
		LO_ParseRGB(str, &red, &green, &blue);
		PA_UNLOCK(buff);
		PA_FREE(buff);
		table->bg_color = XP_NEW(LO_Color);
		if (table->bg_color != NULL)
		{
			table->bg_color->red = red;
			table->bg_color->green = green;
			table->bg_color->blue = blue;
		}
	}

	/*
	 * Get the width and height parameters, in absolute or percentage.
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
			if (allow_percent_width == FALSE)
			{
				val = 0;
			}
			else
			{
				val = (state->win_width - state->win_left -
					state->win_right) * val / 100;
			}
		}
		else
		{
			val = FEUNITS_X(val, context);
		}
		table->width = val;
		if (table->width < 0)
		{
			table->width = 0;
		}
		PA_UNLOCK(buff);
		PA_FREE(buff);
	}
	buff = lo_FetchParamValue(context, tag, PARAM_HEIGHT);
	if (buff != NULL)
	{
		Bool is_percent;

		PA_LOCK(str, char *, buff);
		val = lo_ValueOrPercent(str, &is_percent);
		if (is_percent != FALSE)
		{
			if (allow_percent_height == FALSE)
			{
				val = 0;
			}
			else
			{
				val = (state->win_height - state->win_top -
					state->win_bottom) * val / 100;
			}
		}
		else
		{
			val = FEUNITS_X(val, context);
		}
		table->height = val;
		if (table->height < 0)
		{
			table->height = 0;
		}
		PA_UNLOCK(buff);
		PA_FREE(buff);
	}

	buff = lo_FetchParamValue(context, tag, PARAM_CELLPAD);
	if (buff != NULL)
	{
		PA_LOCK(str, char *, buff);
		val = XP_ATOI(str);
		if (val < 0)
		{
			val = 0;
		}
		table->inner_cell_pad = val;
		PA_UNLOCK(buff);
		PA_FREE(buff);
	}
	buff = lo_FetchParamValue(context, tag, PARAM_CELLSPACE);
	if (buff != NULL)
	{
		PA_LOCK(str, char *, buff);
		val = XP_ATOI(str);
		if (val < 0)
		{
			val = 0;
		}
		table->inter_cell_pad = val;
		PA_UNLOCK(buff);
		PA_FREE(buff);
	}

	state->current_table = table;
}

static void
lo_fill_cell_array(lo_TableRec *table, lo_cell_data XP_HUGE *cell_array,
	lo_TableCell *blank_cell, int32 cell_pad, Bool *relayout_pass)
{
	int32 x, y;
	int32 indx;
	lo_table_span *row_max;
	lo_table_span *col_max;
	lo_TableRow *row_ptr;
	lo_TableCell *cell_ptr;

	row_max = table->height_spans;
	row_ptr = table->row_list;
	for (y = 0; y < table->rows; y++)
	{
		x = 0;
		col_max = table->width_spans;
		cell_ptr = row_ptr->cell_list;
		while (cell_ptr != NULL)
		{
			lo_DocState *subdoc_state;

			/*
			 * Also on this pass check if any of the cells
			 * NEED to be relaid out later.
			 */
			subdoc_state =
				(lo_DocState *)cell_ptr->cell_subdoc->state;
			if (subdoc_state->must_relayout_subdoc != FALSE)
			{
				*relayout_pass = TRUE;
			}

			/*
			 * "fix" up badly specified row spans.
			 */
			if ((y + cell_ptr->rowspan) > table->rows)
			{
				cell_ptr->rowspan = table->rows - y;
				if (cell_ptr->rowspan == 1)
				{
					if (cell_ptr->height > row_max->dim)
					{
						row_max->dim = cell_ptr->height;
					}
				}
			}

			indx = (y * table->cols) + x;

			if (cell_array[indx].cell == blank_cell)
			{
				x++;
				col_max = col_max->next;
				continue;
			}
			cell_array[indx].cell = cell_ptr;
			cell_array[indx].width = cell_ptr->max_width;
			cell_array[indx].height = cell_ptr->height;

			if (cell_ptr->colspan > 1)
			{
				int32 i;
				int32 width, min_width;
				lo_table_span *max_ptr;

				max_ptr = col_max;

				width = max_ptr->dim;
				min_width = max_ptr->min_dim;
				for (i=1; i < cell_ptr->colspan; i++)
				{
					cell_array[indx + i].cell = blank_cell;
					max_ptr = max_ptr->next;
					width = width + cell_pad + max_ptr->dim;
					min_width = min_width + cell_pad +
						max_ptr->min_dim;
				}

				if (width < cell_ptr->max_width)
				{
					int32 add_width;
					int32 add;
					lo_table_span *add_ptr;

					add_ptr = col_max;
					add_width = cell_ptr->max_width -
						width;
					add = 0;
					while (add_ptr != max_ptr)
					{
						int32 newWidth;

						newWidth = add_width *
							add_ptr->dim / width;
						add_ptr->dim += newWidth;
						add += newWidth;
						add_ptr = add_ptr->next;
					}
					add_ptr->dim += (add_width - add);
				}

				if (min_width < cell_ptr->min_width)
				{
					int32 add_width;
					int32 add;
					lo_table_span *add_ptr;

					add_ptr = col_max;
					add_width = cell_ptr->min_width -
						min_width;
					add = 0;
					while (add_ptr != max_ptr)
					{
						int32 newWidth;

						newWidth = add_width *
							add_ptr->min_dim /
							min_width;
						/*
						 * We are not allowed to add
						 * enough to put min_dim > dim.
						 */
						if ((add_ptr->min_dim +
						     newWidth) > add_ptr->dim)
						{
						    newWidth = add_ptr->dim -
							add_ptr->min_dim;
						    /*
						     * don't let newWidth become
						     * negative.
						     */
						    if (newWidth < 0)
						    {
							newWidth = 0;
						    }
						}
						add_ptr->min_dim += newWidth;
						add += newWidth;
						add_ptr = add_ptr->next;
					}
					add_ptr->min_dim += (add_width - add);
				}
				col_max = max_ptr;
			}

			if (cell_ptr->rowspan > 1)
			{
				int32 i;
				int32 height;
				int32 tmp_val;
				lo_table_span *max_ptr;

				max_ptr = row_max;

				height = max_ptr->dim;
				for (i=1; i < cell_ptr->rowspan; i++)
				{
				    cell_array[indx + (i * table->cols)].cell =
					blank_cell;
				    if (cell_ptr->colspan > 1)
				    {
					int32 j;

					for (j=1; j < cell_ptr->colspan; j++)
					{
					    cell_array[indx +
						(i * table->cols) + j].cell =
						blank_cell;
					}
				    }
				    max_ptr = max_ptr->next;
				    height = height + cell_pad + max_ptr->dim;
				}

				tmp_val = cell_ptr->baseline - row_max->min_dim;
				if (tmp_val > 0)
				{
					row_max->min_dim += tmp_val;
					if (row_ptr->vert_alignment ==
						LO_ALIGN_BASELINE)
					{
						row_max->dim += tmp_val;
						height += tmp_val;
					}
				}

				if (height < cell_ptr->height)
				{
					int32 add_height;
					int32 add;
					lo_table_span *add_ptr;

					add_ptr = row_max;
					add_height = cell_ptr->height -
						height;
					add = 0;
					while (add_ptr != max_ptr)
					{
						int32 newHeight;

						newHeight = add_height *
							add_ptr->dim / height;
						add_ptr->dim += newHeight;
						add += newHeight;
						add_ptr = add_ptr->next;
					}
					add_ptr->dim += (add_height - add);
				}
			}

			x += cell_ptr->colspan;

			col_max = col_max->next;
			cell_ptr = cell_ptr->next;
			cell_array[indx].cell->next = NULL;
		}
		row_ptr = row_ptr->next;
		row_max = row_max->next;
	}
}

static void
lo_percent_width_cells(lo_TableRec *table, lo_cell_data XP_HUGE *cell_array,
	lo_TableCell *blank_cell, int32 cell_pad, int32 table_pad,
	int32 *table_width, int32 *min_table_width)
{
	int32 x, y;
	int32 indx;
	int32 new_table_width;
	int32 new_min_table_width;
	Bool need_pass_two;
	lo_table_span *row_max;
	lo_table_span *col_max;
	lo_TableRow *row_ptr;
	lo_TableCell *cell_ptr;

	new_table_width = 0;
	row_ptr = table->row_list;
	row_max = table->height_spans;
	for (y=0; y < table->rows; y++)
	{
	    if (row_ptr->has_percent_width_cells != FALSE)
	    {
		int32 reserve;
		int32 unknown, unknown_base;

		unknown = 0;
		unknown_base = 0;
		reserve = 100 - table->cols;
		col_max = table->width_spans;
		for (x=0; x < table->cols; x++)
		{
			indx = (y * table->cols) + x;
			cell_ptr = cell_array[indx].cell;
			if ((cell_ptr == blank_cell)||
				(cell_ptr == NULL))
			{
				col_max = col_max->next;
				continue;
			}
			if (cell_ptr->percent_width > 0)
			{
				int32 width;

				reserve += cell_ptr->colspan;
				if (cell_ptr->percent_width > reserve)
				{
				    cell_ptr->percent_width = reserve;
				    reserve = 0;
				}
				else
				{
				    reserve -= cell_ptr->percent_width;
				}
				width = cell_ptr->max_width * 100 /
					cell_ptr->percent_width;
				if (width > new_table_width)
				{
					new_table_width = width;
				}
			}
			else
			{
				unknown++;
				unknown_base += cell_ptr->max_width;
			}
			col_max = col_max->next;
		}
		if (unknown)
		{
		    col_max = table->width_spans;
		    for (x=0; x < table->cols; x++)
		    {
			indx = (y * table->cols) + x;
			cell_ptr = cell_array[indx].cell;
			if ((cell_ptr == blank_cell)||
				(cell_ptr == NULL))
			{
				col_max = col_max->next;
				continue;
			}
			if (cell_ptr->percent_width == 0)
			{
				int32 width;
				int32 percent;

				if (unknown == 1)
				{
				    percent = reserve;
				}
				else
				{
				    percent = reserve *
					cell_ptr->max_width /
					unknown_base;
				}
				reserve -= percent;
				if (reserve < 0)
				{
				    reserve = 0;
				}
				percent += cell_ptr->colspan;
				cell_ptr->percent_width = percent;

				width = cell_ptr->max_width * 100 /
					cell_ptr->percent_width;
				if (width > new_table_width)
				{
					new_table_width = width;
				}

				unknown--;
			}
			col_max = col_max->next;
		    }
		}
	    }
	    else
	    {
		int32 width;

		width = 0;
		col_max = table->width_spans;
		for (x=0; x < table->cols; x++)
		{
			indx = (y * table->cols) + x;
			cell_ptr = cell_array[indx].cell;
			if ((cell_ptr == blank_cell)||
				(cell_ptr == NULL))
			{
				col_max = col_max->next;
				continue;
			}
			width += cell_ptr->max_width;
			col_max = col_max->next;
		}
		if (width > new_table_width)
		{
			new_table_width = width;
		}
	    }
	    row_ptr = row_ptr->next;
	    row_max = row_max->next;
	}

	if (*table_width > new_table_width)
	{
		new_table_width = *table_width;
	}

	/*
	 * If we already know how wide this table must be
	 * Use that width when calculate percentage cell widths.
	 */
	if ((table->width > 0)&&(table->width >= *min_table_width))
	{
		new_table_width = table->width;
	}

	need_pass_two = FALSE;

	col_max = table->width_spans;
	for (x=0; x < table->cols; x++)
	{
		int32 current_max;

		current_max = col_max->dim;
		col_max->dim = 1;
		row_max = table->height_spans;
		for (y=0; y < table->rows; y++)
		{
			indx = (y * table->cols) + x;
			cell_ptr = cell_array[indx].cell;
			if ((cell_ptr == blank_cell)||
				(cell_ptr == NULL))
			{
				row_max = row_max->next;
				continue;
			}
			if ((cell_ptr->percent_width > 0)&&
				(cell_ptr->colspan == 1))
			{
				int32 p_width;

				p_width = new_table_width *
					cell_ptr->percent_width / 100;
				if (p_width < cell_ptr->min_width)
				{
					p_width = cell_ptr->min_width;
				}
				if (p_width > col_max->dim)
				{
					col_max->dim = p_width;
				}
			}
			else if (cell_ptr->colspan > 1)
			{
				need_pass_two = TRUE;
			}
			else
			{
				if (cell_ptr->max_width > col_max->dim)
				{
					col_max->dim =
						cell_ptr->max_width;
				}
			}
			row_max = row_max->next;
		}
		if (col_max->dim < col_max->min_dim)
		{
			col_max->dim = col_max->min_dim;
		}
		col_max = col_max->next;
	}
	/*
	 * Take care of spanning columns if any
	 */
	if (need_pass_two != FALSE)
	{
	    row_max = table->height_spans;
	    for (y=0; y < table->rows; y++)
	    {
		col_max = table->width_spans;
		for (x=0; x < table->cols; x++)
		{
			indx = (y * table->cols) + x;
			cell_ptr = cell_array[indx].cell;
			if ((cell_ptr == blank_cell)||
				(cell_ptr == NULL))
			{
				col_max = col_max->next;
				continue;
			}
			if (cell_ptr->colspan > 1)
			{
				int32 i;
				int32 width;
				lo_table_span *max_ptr;
				int32 p_width;
				int32 new_width;

				new_width = cell_ptr->max_width;
			    if (cell_ptr->percent_width > 0)
			    {
				p_width = new_table_width *
					cell_ptr->percent_width / 100;
				if (p_width >= cell_ptr->min_width)
				{
					new_width = p_width;
				}
			    }

				max_ptr = col_max;

				width = max_ptr->dim;
				for (i=1; i < cell_ptr->colspan; i++)
				{
					max_ptr = max_ptr->next;
					width = width + cell_pad +
						max_ptr->dim;
				}

				if (width < new_width)
				{
					int32 add_width;
					int32 add;
					lo_table_span *add_ptr;

					add_ptr = col_max;
					add_width = new_width - width;
					add = 0;
					while (add_ptr != max_ptr)
					{
						int32 newWidth;

						newWidth = add_width *
							add_ptr->dim /
							width;
						add_ptr->dim +=newWidth;
						add += newWidth;
						add_ptr = add_ptr->next;
					}
					add_ptr->dim += (add_width -
						add);
				}
			}
			col_max = col_max->next;
		}
		row_max = row_max->next;
	    }
	}

	new_table_width = 0;
	new_min_table_width = 0;
	col_max = table->width_spans;
	while (col_max != NULL)
	{
		new_table_width = new_table_width + cell_pad +
			col_max->dim;
		new_min_table_width = new_min_table_width + cell_pad +
			col_max->min_dim;
		col_max = col_max->next;
	}
	new_table_width += cell_pad;
	new_table_width += (2 * table->table_ele->border_width);
	new_min_table_width += cell_pad;
	new_min_table_width += (2 * table->table_ele->border_width);
	if (table->draw_borders == TABLE_BORDERS_OFF)
	{
		new_table_width += (2 * table_pad);
		new_min_table_width += (2 * table_pad);
	}

	*table_width = new_table_width;
	*min_table_width = new_min_table_width;
}

static void
lo_cell_relayout_pass(MWContext *context, lo_DocState *state,
	lo_TableRec *table, lo_cell_data XP_HUGE *cell_array, lo_TableCell *blank_cell,
	int32 cell_pad, int32 inner_pad, Bool *rowspan_pass)
{
	int32 x, y;
	int32 indx;
	lo_table_span *row_max;
	lo_table_span *col_max;
	lo_TableRow *row_ptr;
	lo_TableCell *cell_ptr;

	row_ptr = table->row_list;
	row_max = table->height_spans;
	for (y=0; y < table->rows; y++)
	{
		col_max = table->width_spans;
		for (x=0; x < table->cols; x++)
		{
			lo_DocState *subdoc_state;
			LO_SubDocStruct *subdoc;
			int32 width;
			Bool has_elements;

			indx = (y * table->cols) + x;
			cell_ptr = cell_array[indx].cell;
			if ((cell_ptr == blank_cell)||
				(cell_ptr == NULL))
			{
				col_max = col_max->next;
				continue;
			}
			subdoc = cell_ptr->cell_subdoc;
			subdoc_state = (lo_DocState *)subdoc->state;
			width = col_max->dim;
			if (cell_ptr->colspan > 1)
			{
				int32 i;
				lo_table_span *max_ptr;

				max_ptr = col_max;

				for (i=1; i < cell_ptr->colspan; i++)
				{
					max_ptr = max_ptr->next;
					width = width + cell_pad +
					    max_ptr->dim;
				}
			}
			/*
			 * Don't relayout documents that have no
			 * elements, it causes errors.
			 */
			has_elements = lo_subdoc_has_elements(subdoc_state);
			if ((width < cell_ptr->max_width)&&
				(has_elements == FALSE))
			{
				int32 inside_width;

				inside_width = width -
				    (2 * subdoc->border_width) -
				    (2 * subdoc->border_horiz_space) -
				    (2 * inner_pad);
				subdoc->width = inside_width;
			}
			else if (width < cell_ptr->max_width)
			{
				int32 inside_width;

				inside_width = width -
				    (2 * subdoc->border_width) -
				    (2 * subdoc->border_horiz_space) -
				    (2 * inner_pad);
				cell_ptr->cell_subdoc =
					lo_RelayoutSubdoc(context,
					    state, subdoc, inside_width,
					    cell_ptr->is_a_header);
				subdoc = cell_ptr->cell_subdoc;
				cell_ptr->baseline =
					lo_GetSubDocBaseline(subdoc);
				if (cell_ptr->rowspan == 1)
				{
				    int32 subdoc_height;
				    int32 tmp_val;

				    subdoc_height = subdoc->height +
					(2 * inner_pad) + 
					(2 * subdoc->border_vert_space);

				    tmp_val = cell_ptr->baseline -
					row_max->min_dim;
				    if (tmp_val > 0)
				    {
					row_max->min_dim += tmp_val;
					if (row_ptr->vert_alignment ==
						LO_ALIGN_BASELINE)
					{
						row_max->dim += tmp_val;
					}
				    }

				    if (row_ptr->vert_alignment ==
					LO_ALIGN_BASELINE)
				    {
					tmp_val = (subdoc_height -
					    cell_ptr->baseline) -
					    (row_max->dim -
					    row_max->min_dim);
					if (tmp_val > 0)
					{
					    row_max->dim += tmp_val;
					}
				    }
				    else
				    {
					if (subdoc_height >
						row_max->dim)
					{
					    row_max->dim =
						subdoc_height;
					}
				    }
				}
				else
				{
					*rowspan_pass = TRUE;
				}
			}
			else if ((subdoc_state->must_relayout_subdoc != FALSE)&&
				 (has_elements == FALSE))
			{
				int32 inside_width;

				inside_width = width -
				    (2 * subdoc->border_width) -
				    (2 * subdoc->border_horiz_space) -
				    (2 * inner_pad);
				subdoc->width = inside_width;
				subdoc_state->must_relayout_subdoc = FALSE;
			}
			else if (subdoc_state->must_relayout_subdoc !=
					FALSE)
			{
				int32 inside_width;

				inside_width = width -
				    (2 * subdoc->border_width) -
				    (2 * subdoc->border_horiz_space) -
				    (2 * inner_pad);
				cell_ptr->cell_subdoc =
					lo_RelayoutSubdoc(context,
					    state, subdoc, inside_width,
					    cell_ptr->is_a_header);
				subdoc = cell_ptr->cell_subdoc;
				cell_ptr->baseline =
					lo_GetSubDocBaseline(subdoc);
				if (cell_ptr->rowspan == 1)
				{
				    int32 subdoc_height;
				    int32 tmp_val;

				    subdoc_height = subdoc->height +
					(2 * inner_pad) +
					(2 * subdoc->border_vert_space);

				    tmp_val = cell_ptr->baseline -
					row_max->min_dim;
				    if (tmp_val > 0)
				    {
					row_max->min_dim += tmp_val;
					if (row_ptr->vert_alignment ==
						LO_ALIGN_BASELINE)
					{
						row_max->dim += tmp_val;
					}
				    }

				    if (row_ptr->vert_alignment ==
					LO_ALIGN_BASELINE)
				    {
					tmp_val = (subdoc_height -
					    cell_ptr->baseline) -
					    (row_max->dim -
					    row_max->min_dim);
					if (tmp_val > 0)
					{
					    row_max->dim += tmp_val;
					}
				    }
				    else
				    {
					if (subdoc_height >
						row_max->dim)
					{
					    row_max->dim =
						subdoc_height;
					}
				    }
				}
				else
				{
					*rowspan_pass = TRUE;
				}
			}
			col_max = col_max->next;
		}
		row_max = row_max->next;
		row_ptr = row_ptr->next;
	}
}


static void
lo_cell_rowspan_pass(lo_TableRec *table, lo_cell_data XP_HUGE *cell_array,
	lo_TableCell *blank_cell, int32 cell_pad, int32 inner_pad)
{
	int32 x, y;
	int32 indx;
	lo_table_span *row_max;
	lo_table_span *col_max;
	lo_TableRow *row_ptr;
	lo_TableCell *cell_ptr;

	row_max = table->height_spans;
	row_ptr = table->row_list;
	for (y=0; y < table->rows; y++)
	{
		col_max = table->width_spans;
		for (x=0; x < table->cols; x++)
		{
			LO_SubDocStruct *subdoc;

			indx = (y * table->cols) + x;
			cell_ptr = cell_array[indx].cell;
			if ((cell_ptr == blank_cell)||
				(cell_ptr == NULL))
			{
				col_max = col_max->next;
				continue;
			}
			subdoc = cell_ptr->cell_subdoc;

			if (cell_ptr->rowspan > 1)
			{
				int32 i;
				int32 height;
				int32 tmp_val;
				int32 subdoc_height;
				lo_table_span *max_ptr;

				max_ptr = row_max;

				height = max_ptr->dim;
				for (i=1; i < cell_ptr->rowspan; i++)
				{
				    max_ptr = max_ptr->next;
				    height = height + cell_pad +
					max_ptr->dim;
				}
				tmp_val = cell_ptr->baseline -
				    row_max->min_dim;
				if (tmp_val > 0)
				{
				    row_max->min_dim += tmp_val;
				    if (row_ptr->vert_alignment ==
					LO_ALIGN_BASELINE)
				    {
					row_max->dim += tmp_val;
					height += tmp_val;
				    }
				}

				subdoc_height = subdoc->height +
					(2 * inner_pad) +
					(2 * subdoc->border_vert_space);

				if (height < subdoc_height)
				{
					int32 add_height;
					int32 add;
					lo_table_span *add_ptr;

					add_ptr = row_max;
					add_height = subdoc_height -
						height;
					add = 0;
					while (add_ptr != max_ptr)
					{
						int32 newHeight;

						newHeight = add_height *
							add_ptr->dim /
							height;
						add_ptr->dim +=
							newHeight;
						add += newHeight;
						add_ptr = add_ptr->next;
					}
					add_ptr->dim +=
						(add_height - add);
				}
			}
			col_max = col_max->next;
		}
		row_max = row_max->next;
		row_ptr = row_ptr->next;
	}
}


static void
#ifdef SCARY_LEAK_FIX
lo_free_row_record(lo_TableRow *row, Bool partial)
#else
lo_free_row_record(lo_TableRow *row)
#endif /* SCARY_LEAK_FIX */
{
#ifdef SCARY_LEAK_FIX
	lo_TableCell *cell_ptr;
	lo_TableCell *cell;
#endif /* SCARY_LEAK_FIX */

	if (row->cell_list != NULL)
	{
		/*
		 * These are already freed on a completed table, and
		 * need to be freed on a partial table.
		 */
#ifdef SCARY_LEAK_FIX
		if (partial != FALSE)
		{
			cell_ptr = row->cell_list;
			while (cell_ptr != NULL)
			{
				cell = cell_ptr;
				cell_ptr = cell_ptr->next;
				XP_DELETE(cell);
			}
		}
#endif /* SCARY_LEAK_FIX */
		row->cell_list = NULL;
		row->cell_ptr = NULL;
	}

	if (row->bg_color != NULL)
	{
		XP_DELETE(row->bg_color);
	}

	XP_DELETE(row);
}


static void
#ifdef SCARY_LEAK_FIX
lo_free_table_record(MWContext *context, lo_TableRec *table, Bool partial)
#else
lo_free_table_record(MWContext *context, lo_TableRec *table)
#endif /* SCARY_LEAK_FIX */
{
	if (table->row_list != NULL)
	{
		lo_TableRow *row_ptr;
		lo_TableRow *row;

		row_ptr = table->row_list;
		while (row_ptr != NULL)
		{
			row = row_ptr;
			row_ptr = row_ptr->next;
#ifdef SCARY_LEAK_FIX
			lo_free_row_record(row, partial);
#else
			lo_free_row_record(row);
#endif /* SCARY_LEAK_FIX */
		}
		table->row_list = NULL;
		table->row_ptr = NULL;
	}

	if (table->caption != NULL)
	{
		XP_DELETE(table->caption);
		table->caption = NULL;
	}

	if (table->width_spans != NULL)
	{
		lo_table_span *span_ptr;
		lo_table_span *span;

		span_ptr = table->width_spans;
		while (span_ptr != NULL)
		{
			span = span_ptr;
			span_ptr = span_ptr->next;
			XP_DELETE(span);
		}
		table->width_spans = NULL;
		table->width_span_ptr = NULL;
	}

	if (table->height_spans != NULL)
	{
		lo_table_span *span_ptr;
		lo_table_span *span;

		span_ptr = table->height_spans;
		while (span_ptr != NULL)
		{
			span = span_ptr;
			span_ptr = span_ptr->next;
			XP_DELETE(span);
		}
		table->height_spans = NULL;
		table->height_span_ptr = NULL;
	}

	if (table->bg_color != NULL)
	{
		XP_DELETE(table->bg_color);
	}

	XP_DELETE(table);
}


#ifdef SCARY_LEAK_FIX
void
lo_FreePartialTable(MWContext *context, lo_DocState *state, lo_TableRec *table)
{
	lo_TableRow *row_ptr;
	lo_TableCell *cell_ptr;
	LO_SubDocStruct *subdoc;

	if (table == NULL)
	{
		return;
	}

	row_ptr = table->row_list;
	while (row_ptr != NULL)
	{
		cell_ptr = row_ptr->cell_list;
		while (cell_ptr != NULL)
		{
			subdoc = cell_ptr->cell_subdoc;
			lo_FreePartialSubDoc(context, state, subdoc);
			cell_ptr->cell_subdoc = NULL;
			cell_ptr = cell_ptr->next;
		}
		row_ptr = row_ptr->next;
	}

	if (table->caption != NULL)
	{
		subdoc = table->caption->subdoc;
		lo_FreePartialSubDoc(context, state, subdoc);
		table->caption->subdoc = NULL;
	}

	lo_free_table_record(context, table, TRUE);
}
#endif /* SCARY_LEAK_FIX */


void
lo_EndTable(MWContext *context, lo_DocState *state, lo_TableRec *table)
{
	int32 save_doc_min_width;
	int32 x, y;
	int32 cell_x, cell_y;
	int32 indx;
	int32 cell_cnt;
	int32 ele_cnt;
	int32 table_width, min_table_width;
	int32 table_height;
	int32 width_limit;
	Bool relayout_pass;
	Bool rowspan_pass;
	Bool cut_to_window_width;
	lo_TableCell blank_cell;
	lo_cell_data XP_HUGE *cell_array;
	XP_Block cell_array_buff;
	lo_table_span *row_max;
	lo_table_span *col_max;
	lo_TableCell *cell_ptr;
	int32 cell_pad;
	int32 inner_pad;
	int32 table_pad;
	Bool floating;
	int32 save_state_x, save_state_y;
	LO_Element *save_line_list;
#ifdef LOCAL_DEBUG
 fprintf(stderr, "lo_EndTable called\n");
#endif /* LOCAL_DEBUG */

	cell_pad = FEUNITS_X(table->inter_cell_pad, context);
	inner_pad = FEUNITS_X(table->inner_cell_pad, context);
	table_pad = FEUNITS_X(TABLE_DEF_CELL_BORDER, context);
	relayout_pass = FALSE;
	cut_to_window_width = TRUE;
	floating = FALSE;

	/*
	 * So gcc won't complain
	 */
	save_state_x = 0;
	save_state_y = 0;
	save_line_list = NULL;
/*
	state->current_table = NULL;
*/

	cell_cnt = table->rows * table->cols;
	/*
	 * Empty tables are completely ignored!
	 */
	if (cell_cnt <= 0)
	{
		/*
		 * Clear table state, and free up this structure.
		 */
		state->current_table = NULL;
		if (table != NULL)
		{
#ifdef SCARY_LEAK_FIX
			lo_free_table_record(context, table, FALSE);
#else
			lo_free_table_record(context, table);
#endif /* SCARY_LEAK_FIX */
		}
		return;
	}
#ifdef XP_WIN16
	/*
	 * It had better be the case that the size of the struct is a
	 * power of 2
	 */
	XP_ASSERT(sizeof(lo_cell_data) == 8);
	cell_array = (lo_cell_data XP_HUGE *)_halloc(cell_cnt, sizeof(lo_cell_data));

	/*
	 * There isn't a runtime routine that can initialize a huge array
	 */
	if (cell_cnt * sizeof(lo_cell_data) <= 0xFFFFL)
		memset(cell_array, 0, (cell_cnt * sizeof(lo_cell_data)));
	else {
		BYTE XP_HUGE *tmp = (BYTE XP_HUGE *)cell_array;

		for (indx = 0; indx < cell_cnt * sizeof(lo_cell_data); indx++)
			tmp[indx] = 0;
	}
#else
	cell_array_buff = XP_ALLOC_BLOCK(cell_cnt * sizeof(lo_cell_data));
	XP_LOCK_BLOCK(cell_array, lo_cell_data *, cell_array_buff);
	memset(cell_array, 0, (cell_cnt * sizeof(lo_cell_data)));
#endif

	lo_fill_cell_array(table, cell_array, &blank_cell, cell_pad,
		&relayout_pass);

	table_width = 0;
	min_table_width = 0;
	col_max = table->width_spans;
	while (col_max != NULL)
	{
		table_width = table_width + cell_pad + col_max->dim;
		min_table_width = min_table_width + cell_pad + col_max->min_dim;
		col_max = col_max->next;
	}
	table_width += cell_pad;
	table_width += (2 * table->table_ele->border_width);
	min_table_width += cell_pad;
	min_table_width += (2 * table->table_ele->border_width);
	if (table->draw_borders == TABLE_BORDERS_OFF)
	{
		table_width += (2 * table_pad);
		min_table_width += (2 * table_pad);
	}

	/*
	 * Take care of cells with percentage widths
	 */
	if (table->has_percent_width_cells != FALSE)
	{
		lo_percent_width_cells(table, cell_array, &blank_cell,
			cell_pad, table_pad, &table_width, &min_table_width);
		relayout_pass = TRUE;
	}

	/*
	 * Use state->left_margin here instead of state->win_left to take
	 * indent from lists into account.
	 */
	width_limit = (state->win_width - state->left_margin -state->win_right);
	if (table->width > 0)
	{
		width_limit = table->width;
	}
	/*
	 * Else, if we are a nested table in an infinite width
	 * layout space, we don't want to cut to "window width"
	 */
	else if (state->allow_percent_width == FALSE)
	{
		cut_to_window_width = FALSE;
	}

	if (min_table_width >= width_limit)
	{
		lo_table_span *sub_ptr;

		relayout_pass = TRUE;
		sub_ptr = table->width_spans;
		while (sub_ptr != NULL)
		{
			sub_ptr->dim = sub_ptr->min_dim;
			sub_ptr = sub_ptr->next;
		}
	}
	else if ((table_width > width_limit)&&(cut_to_window_width != FALSE))
	{
		intn pass;
		Bool expand_failed;
		int32 add_width;
		int32 div_width;
		int32 div_width_next;
		int32 add;
		int32 total_to_add;
		lo_table_span *add_ptr;

		relayout_pass = TRUE;

		expand_failed = TRUE;
		pass = 0;
		total_to_add = width_limit - min_table_width;
		div_width = table_width;
		while ((total_to_add > 0)&&(pass < 10)&&(expand_failed !=FALSE))
		{
			int32 extra;
			int32 min_extra;

			expand_failed = FALSE;
			add_ptr = table->width_spans;
			add_width = total_to_add;
			total_to_add = 0;
			div_width_next = (table->cols + 1) * cell_pad;
			add = 0;
			while (add_ptr->next != NULL)
			{
				if ((pass > 0)&&
					(add_ptr->min_dim == add_ptr->dim))
				{
					add_ptr = add_ptr->next;
					continue;
				}
				extra = add_width * add_ptr->dim /
					div_width;
				if ((add_ptr->min_dim + extra) > add_ptr->dim)
				{
					expand_failed = TRUE;
					min_extra = add_ptr->dim -
						add_ptr->min_dim;
					extra = min_extra;
				}

				add_ptr->min_dim += extra;
				if (add_ptr->min_dim < add_ptr->dim)
				{
					div_width_next += add_ptr->dim;
				}
				add += extra;
				add_ptr = add_ptr->next;
			}
			if ((pass == 0)||(add_ptr->min_dim < add_ptr->dim))
			{
				if (expand_failed == FALSE)
				{
					extra = add_width - add;
				}
				else
				{
					extra = add_width * add_ptr->dim /
						div_width;
				}
				if ((add_ptr->min_dim + extra) > add_ptr->dim)
				{
					expand_failed = TRUE;
					min_extra = add_ptr->dim -
						add_ptr->min_dim;
					extra = min_extra;
				}
				add_ptr->min_dim += extra;
				if (add_ptr->min_dim < add_ptr->dim)
				{
					div_width_next += add_ptr->dim;
				}
				add += extra;
			}
			total_to_add = add_width - add;
			div_width = div_width_next;
			pass++;
		}
		if (total_to_add > 0)
		{
			add = 0;
			add_ptr = table->width_spans;
			while ((add_ptr->next != NULL)&&(total_to_add > 0))
			{
				if ((add_ptr->min_dim + total_to_add) >
					add_ptr->dim)
				{
					add = add_ptr->dim - add_ptr->min_dim;
					total_to_add = total_to_add - add;
					add_ptr->min_dim += add;
				}
				else
				{
					add_ptr->min_dim += total_to_add;
					total_to_add = 0;
				}
				add_ptr = add_ptr->next;
			}
			add_ptr->min_dim += total_to_add;
		}

		add_ptr = table->width_spans;
		while (add_ptr != NULL)
		{
			add_ptr->dim = add_ptr->min_dim;
			add_ptr = add_ptr->next;
		}
	}

	/*
	 * If the user specified a wider width than the optimal
	 * width.  Make it so.
	 */
	if ((table->width > 0)&&(width_limit > table_width))
	{
		int32 add_width;
		int32 add;
		int32 newWidth;
		lo_table_span *add_ptr;

		add_ptr = table->width_spans;
		add_width = width_limit - table_width;
		add = 0;
		while ((add_ptr != NULL)&&(add_ptr->next != NULL))
		{

			newWidth = add_width * add_ptr->dim / table_width;
			add_ptr->dim += newWidth;
			add += newWidth;
			add_ptr = add_ptr->next;
		}
		add_ptr->dim += (add_width - add);
	}

	rowspan_pass = FALSE;
	if (relayout_pass != FALSE)
	{
		lo_cell_relayout_pass(context, state, table, cell_array,
			&blank_cell, cell_pad, inner_pad, &rowspan_pass);
	}

	if (rowspan_pass != FALSE)
	{
		lo_cell_rowspan_pass(table, cell_array, &blank_cell,
			cell_pad, inner_pad);
	}

	table_width = 0;
	min_table_width = 0;
	col_max = table->width_spans;
	while (col_max != NULL)
	{
		table_width = table_width + cell_pad + col_max->dim;
		min_table_width = min_table_width + cell_pad + col_max->min_dim;
		col_max = col_max->next;
	}
	table_width += cell_pad;
	table_width += (2 * table->table_ele->border_width);
	min_table_width += cell_pad;
	min_table_width += (2 * table->table_ele->border_width);

	table_height = 0;
	row_max = table->height_spans;
	while (row_max != NULL)
	{
		table_height = table_height + cell_pad + row_max->dim;
		row_max = row_max->next;
	}
	table_height += cell_pad;
	table_height += (2 * table->table_ele->border_width);
	if (table->draw_borders == TABLE_BORDERS_OFF)
	{
		table_width += (2 * table_pad);
		min_table_width += (2 * table_pad);
		table_height += (2 * table_pad);
	}


	/*
	 * If the user specified a taller table than the optimal
	 * height.  Make it so.
	 */
	if ((table->height > 0)&&(table->height > table_height))
	{
		int32 add_height;
		int32 add;
		lo_table_span *add_ptr;
		int32 newHeight;

		add_ptr = table->height_spans;
		add_height = table->height - table_height;
		add = 0;
		while((add_ptr != NULL) && (add_ptr->next != NULL))
		{

			newHeight = add_height * add_ptr->dim / table_height;
			add_ptr->dim += newHeight;
			add += newHeight;
			add_ptr = add_ptr->next;
		}
		add_ptr->dim += (add_height - add);

		table_height = table->height;
	}

	table->table_ele->width = table_width;
	table->table_ele->height = table_height;

	/*
	 * Here, if we are floating we don't do the linebreak.
	 * We do save some state info and then "fake" a linebreak
	 * so we can use common code to place the rest of the table.
	 */
	if (table->table_ele->ele_attrmask & LO_ELE_FLOATING)
	{
		save_line_list = state->line_list;
		save_state_x = state->x;
		save_state_y = state->y;
		floating = TRUE;
		state->x = state->left_margin;
		state->y = state->y + state->line_height;
		state->line_list = NULL;
	}
	else
	{
		lo_SoftLineBreak(context, state, FALSE, 1);
	}

	table->table_ele->x = state->x;
	table->table_ele->y = state->y;
	/* Keep the element IDs in order */
	table->table_ele->ele_id = NEXT_ELEMENT;
	lo_AppendToLineList(context, state,
		(LO_Element *)table->table_ele, 0);
	state->x += (cell_pad + table->table_ele->border_width);
	state->y += (cell_pad + table->table_ele->border_width);
	if (table->draw_borders == TABLE_BORDERS_OFF)
	{
		state->x += table_pad;
		state->y += table_pad;
	}

	if (table->caption != NULL)
	{
		int32 new_width;
		LO_SubDocStruct *subdoc;
		Bool has_elements;

		subdoc = table->caption->subdoc;

		col_max = table->width_spans;
		new_width = col_max->dim;
		while (col_max->next != NULL)
		{
			col_max = col_max->next;
			new_width = new_width + cell_pad + col_max->dim;
		}
		/*
		 * Don't relayout documents that have no
		 * elements, it causes errors.
		 */
		has_elements = lo_subdoc_has_elements(subdoc->state);
		if ((new_width < table->caption->max_width)&&
			(has_elements == FALSE))
		{
			int32 inside_width;

			inside_width = new_width -
			    (2 * subdoc->border_width) -
			    (2 * subdoc->border_horiz_space) -
			    (2 * inner_pad);
			subdoc->width = inside_width;
		}
		else if (new_width < table->caption->max_width)
		{
			int32 inside_width;

			inside_width = new_width -
			    (2 * subdoc->border_width) -
			    (2 * subdoc->border_horiz_space) -
			    (2 * inner_pad);
			table->caption->subdoc = lo_RelayoutSubdoc(context,
				state, subdoc, inside_width, FALSE);
			subdoc = table->caption->subdoc;

			table->caption->height = subdoc->height +
				(2 * subdoc->border_vert_space) +
				(2 * inner_pad);
			table->caption->max_width = new_width;
		}
		else
		{
			table->caption->max_width = new_width;
		}
	}

	state->current_table = NULL;
	cell_x = state->x;
	cell_y = state->y;

	if ((table->caption != NULL)&&
		(table->caption->vert_alignment == LO_ALIGN_TOP))
	{
		LO_SubDocStruct *subdoc;

		cell_x = state->x;
		subdoc = table->caption->subdoc;
		subdoc->x = cell_x;
		subdoc->y = table->table_ele->y;
		subdoc->x_offset = (int16)subdoc->border_horiz_space;
		subdoc->y_offset = (int32)subdoc->border_vert_space;
		subdoc->width = table->caption->max_width -
			(2 * subdoc->border_horiz_space);
		subdoc->height = table->caption->height -
			(2 * subdoc->border_vert_space);
		ele_cnt = lo_align_subdoc(context, state,
			(lo_DocState *)subdoc->state, subdoc, table, NULL);
		if (ele_cnt > 0)
		{
			LO_CellStruct *cell_ele;

			cell_ele = lo_SquishSubDocToCell(context, state,
						subdoc);
			if (cell_ele == NULL)
			{
				lo_AppendToLineList(context, state,
					(LO_Element *)subdoc, 0);
			}
			else
			{
				lo_AppendToLineList(context, state,
					(LO_Element *)cell_ele, 0);
			}
			cell_x = state->x;
			cell_y = cell_y + table->caption->height + cell_pad;

			table->table_ele->y_offset = cell_y -
				table->table_ele->y;
			cell_y += (cell_pad + table->table_ele->border_width);
			if (table->draw_borders == TABLE_BORDERS_OFF)
			{
				cell_y += table_pad;
			}
		}
		else
		{
			LO_CellStruct *cell_ele;

			/*
			 * Free up the useless subdoc
			 */
			cell_ele = lo_SquishSubDocToCell(context, state,
						subdoc);
			if (cell_ele != NULL)
			{
				lo_FreeElement(context,
					(LO_Element *)cell_ele, TRUE);
			}
		}
	}

	row_max = table->height_spans;
	for (y=0; y < table->rows; y++)
	{
		cell_x = state->x;
		col_max = table->width_spans;
		for (x=0; x < table->cols; x++)
		{
			LO_SubDocStruct *subdoc;

			indx = (y * table->cols) + x;
			cell_ptr = cell_array[indx].cell;
			if ((cell_ptr == &blank_cell)||(cell_ptr == NULL))
			{
				cell_x = cell_x + col_max->dim + cell_pad;
				col_max = col_max->next;
				continue;
			}
			subdoc = cell_ptr->cell_subdoc;
			subdoc->x = cell_x;
			subdoc->y = cell_y;
			subdoc->x_offset = (int16)subdoc->border_horiz_space;
			subdoc->y_offset = (int32)subdoc->border_vert_space;
			subdoc->width = col_max->dim;
			if (cell_ptr->colspan > 1)
			{
				int32 i;
				lo_table_span *max_ptr;

				max_ptr = col_max;

				for (i=1; i < cell_ptr->colspan; i++)
				{
					max_ptr = max_ptr->next;
					subdoc->width = subdoc->width +
						cell_pad + max_ptr->dim;
				}
			}
			subdoc->width = subdoc->width -
				(2 * subdoc->border_horiz_space);
			subdoc->height = row_max->dim;
			if (cell_ptr->rowspan > 1)
			{
				int32 i;
				lo_table_span *max_ptr;

				max_ptr = row_max;

				for (i=1; i < cell_ptr->rowspan; i++)
				{
					max_ptr = max_ptr->next;
					subdoc->height = subdoc->height +
						cell_pad + max_ptr->dim;
				}
			}
			subdoc->height = subdoc->height -
				(2 * subdoc->border_vert_space);
			ele_cnt = lo_align_subdoc(context, state,
				(lo_DocState *)subdoc->state, subdoc,
				table, row_max);
			if (ele_cnt > 0)
			{
				LO_CellStruct *cell_ele;

				cell_ele = lo_SquishSubDocToCell(context, state,
							subdoc);
				if (cell_ele == NULL)
				{
					lo_AppendToLineList(context, state,
						(LO_Element *)subdoc, 0);
				}
				else
				{
					lo_AppendToLineList(context, state,
						(LO_Element *)cell_ele, 0);
				}
			}
			else
			{
				LO_CellStruct *cell_ele;

				/*
				 * Free up the useless subdoc
				 */
				cell_ele = lo_SquishSubDocToCell(context, state,
							subdoc);
				if (cell_ele != NULL)
				{
					lo_FreeElement(context,
						(LO_Element *)cell_ele, TRUE);
				}
			}
			cell_x = cell_x + col_max->dim + cell_pad;
			col_max = col_max->next;
		}
		cell_y = cell_y + row_max->dim + cell_pad;
		row_max = row_max->next;
	}
	cell_x = cell_x + table->table_ele->border_width;
	cell_y = cell_y + table->table_ele->border_width;
	if (table->draw_borders == TABLE_BORDERS_OFF)
	{
		cell_x += table_pad;
		cell_y += table_pad;
	}

	if ((table->caption != NULL)&&
		(table->caption->vert_alignment != LO_ALIGN_TOP))
	{
		LO_SubDocStruct *subdoc;

		subdoc = table->caption->subdoc;
		subdoc->x = state->x;
		subdoc->y = cell_y;
		subdoc->x_offset = (int16)subdoc->border_horiz_space;
		subdoc->y_offset = (int32)subdoc->border_vert_space;
		subdoc->width = table->caption->max_width -
			(2 * subdoc->border_horiz_space);
		subdoc->height = table->caption->height -
			(2 * subdoc->border_vert_space);
		ele_cnt = lo_align_subdoc(context, state,
			(lo_DocState *)subdoc->state, subdoc, table, NULL);
		if (ele_cnt > 0)
		{
			LO_CellStruct *cell_ele;

			cell_ele = lo_SquishSubDocToCell(context, state,
						subdoc);
			if (cell_ele == NULL)
			{
				lo_AppendToLineList(context, state,
					(LO_Element *)subdoc, 0);
			}
			else
			{
				lo_AppendToLineList(context, state,
					(LO_Element *)cell_ele, 0);
			}
			cell_y = cell_y + table->caption->height + cell_pad;
		}
		else
		{
			LO_CellStruct *cell_ele;

			/*
			 * Free up the useless subdoc
			 */
			cell_ele = lo_SquishSubDocToCell(context, state,
						subdoc);
			if (cell_ele != NULL)
			{
				lo_FreeElement(context,
					(LO_Element *)cell_ele, TRUE);
			}
		}
	}

	/*
	 * The usual stuff for a table.
	 */
	if (floating == FALSE)
	{
		state->x = cell_x;
		state->baseline = 0;
		state->line_height = cell_y - state->y;
		state->linefeed_state = 0;
		state->at_begin_line = FALSE;
		state->trailing_space = FALSE;
		state->cur_ele_type = LO_SUBDOC;

		table->table_ele->line_height = state->line_height;

		save_doc_min_width = state->min_width;
		lo_HardLineBreak(context, state, FALSE);
		if (min_table_width > save_doc_min_width)
		{
			save_doc_min_width = min_table_width;
		}
		state->min_width = save_doc_min_width;
	}
	/*
	 * Else this is a floating table, rip it out of the "faked"
	 * line list, stuff itin the margin, and restore our state
	 * to where we left off.
	 */
	else
	{
		int32 push_right;
		int32 line_height;
                LO_Element *tptr;
                LO_Element *last;

		line_height = cell_y - state->y;
		push_right = 0;
		if (table->table_ele->alignment == LO_ALIGN_RIGHT)
		{
			push_right = state->right_margin - cell_x;
		}

		table->table_ele->x_offset +=
			(int16)table->table_ele->border_horiz_space;
		table->table_ele->y_offset +=
			(int32)table->table_ele->border_vert_space;

		last = NULL;
		tptr = state->line_list;
		while (tptr != NULL)
		{
			tptr->lo_any.x += push_right;
			if (tptr->type == LO_CELL)
			{
				tptr->lo_any.x +=
					table->table_ele->border_horiz_space;
				tptr->lo_any.y +=
					table->table_ele->border_vert_space;
				lo_ShiftCell((LO_CellStruct *)tptr,
					(push_right +
					table->table_ele->border_horiz_space),
					table->table_ele->border_vert_space);
			}
			last = tptr;
			tptr->lo_any.line_height = line_height;
			tptr = tptr->lo_any.next;
		}

		/*
		 * Stuff the whole line list into the float list, and
		 * restore the line list to its pre-table state.
		 */
		if (state->line_list != NULL)
		{
			last->lo_any.next = state->float_list;
			state->float_list = state->line_list;
			state->line_list = NULL;
		}
		state->line_list = save_line_list;

		table->table_ele->line_height = line_height;
		table->table_ele->expected_y = table->table_ele->y;
		table->table_ele->y = -1;

		lo_AddMarginStack(state,
			table->table_ele->x, table->table_ele->y,
                        table->table_ele->width, table->table_ele->line_height,
                        table->table_ele->border_width,
                        table->table_ele->border_vert_space,
			table->table_ele->border_horiz_space,
                        (intn)table->table_ele->alignment);

		/*
		 * Restore state to pre-table values.
		 */
		state->x = save_state_x;
		state->y = save_state_y;

		/*
		 * All the doc_min_width stuff makes state->min_width
		 * be correct for tables nested inside tables.
		 */
		save_doc_min_width = state->min_width;

		/*
		 * Standard float stuff, if we happen to be at the start
		 * of a line, place the table now.
		 */
		if (state->at_begin_line != FALSE)
		{
			lo_FindLineMargins(context, state);
			state->x = state->left_margin;
		}

		if (min_table_width > save_doc_min_width)
		{
			save_doc_min_width = min_table_width;
		}
		state->min_width = save_doc_min_width;
	}

	for (y=0; y < table->rows; y++)
	{
		for (x=0; x < table->cols; x++)
		{
			indx = (y * table->cols) + x;
			cell_ptr = cell_array[indx].cell;
			if ((cell_ptr != &blank_cell)&&(cell_ptr != NULL))
			{
				XP_DELETE(cell_ptr);
			}
		}
	}

#ifdef XP_WIN16
	_hfree(cell_array);
#else
	XP_UNLOCK_BLOCK(cell_array_buff);
	XP_FREE_BLOCK(cell_array_buff);
#endif

#ifdef SCARY_LEAK_FIX
	lo_free_table_record(context, table, FALSE);
#else
	lo_free_table_record(context, table);
#endif /* SCARY_LEAK_FIX */
}

#ifdef TEST_16BIT
#undef XP_WIN16
#endif /* TEST_16BIT */

