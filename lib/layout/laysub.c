#include "xp.h"
#include "pa_parse.h"
#include "layout.h"

#ifdef TEST_16BIT
#define XP_WIN16
#endif /* TEST_16BIT */

#define	SUBDOC_DEF_ANCHOR_BORDER		1
#define	SUBDOC_DEF_VERTICAL_SPACE		5
#define	SUBDOC_DEF_HORIZONTAL_SPACE		5

#define	CELL_LINE_INC		10


void
lo_InheritParentState(lo_DocState *child_state, lo_DocState *parent_state)
{
	/*
	 * Instead of the default of a new doc assuming 100 lines,
	 * we will start assuming a cell is 10 lines.
	 * This save lots of memory in table processing.
	 */
	if ((child_state->is_a_subdoc == SUBDOC_CELL)||
		(child_state->is_a_subdoc == SUBDOC_CAPTION))
	{
		XP_Block line_array_block;
		LO_Element **line_array;

		line_array_block = XP_ALLOC_BLOCK(CELL_LINE_INC *
					sizeof(LO_Element *));
		if (line_array_block != NULL)
		{
			XP_FREE_BLOCK(child_state->line_array);
			child_state->line_array = line_array_block;
			XP_LOCK_BLOCK(line_array, LO_Element **,
				child_state->line_array);
			line_array[0] = NULL;
			XP_UNLOCK_BLOCK(child_state->line_array);
			child_state->line_array_size = CELL_LINE_INC;
#ifdef XP_WIN16
{
			XP_Block *larray_array;

			XP_LOCK_BLOCK(larray_array, XP_Block *,
				child_state->larray_array);
			larray_array[0] = child_state->line_array;
			XP_UNLOCK_BLOCK(child_state->larray_array);
}
#endif /* XP_WIN16 */
		}
	}

	if (((child_state->is_a_subdoc == SUBDOC_CELL)||
		(child_state->is_a_subdoc == SUBDOC_CAPTION))&&
	    ((parent_state->is_a_subdoc == SUBDOC_CELL)||
		(parent_state->is_a_subdoc == SUBDOC_CAPTION)))
	{
		child_state->subdoc_tags = parent_state->subdoc_tags_end;
		child_state->subdoc_tags_end = NULL;
	}

	child_state->text_fg.red = STATE_DEFAULT_FG_RED(parent_state);
	child_state->text_fg.green = STATE_DEFAULT_FG_GREEN(parent_state);
	child_state->text_fg.blue = STATE_DEFAULT_FG_BLUE(parent_state);

	child_state->text_bg.red = STATE_DEFAULT_BG_RED(parent_state);
	child_state->text_bg.green = STATE_DEFAULT_BG_GREEN(parent_state);
	child_state->text_bg.blue = STATE_DEFAULT_BG_BLUE(parent_state);
	lo_RecolorText(child_state);

	child_state->anchor_color.red =
		STATE_UNVISITED_ANCHOR_RED(parent_state);
	child_state->anchor_color.green =
		STATE_UNVISITED_ANCHOR_GREEN(parent_state);
	child_state->anchor_color.blue =
		STATE_UNVISITED_ANCHOR_BLUE(parent_state);

	child_state->visited_anchor_color.red =
		STATE_VISITED_ANCHOR_RED(parent_state);
	child_state->visited_anchor_color.green =
		STATE_VISITED_ANCHOR_GREEN(parent_state);
	child_state->visited_anchor_color.blue =
		STATE_VISITED_ANCHOR_BLUE(parent_state);

	child_state->active_anchor_color.red =
		STATE_SELECTED_ANCHOR_RED(parent_state);
	child_state->active_anchor_color.green =
		STATE_SELECTED_ANCHOR_GREEN(parent_state);
	child_state->active_anchor_color.blue =
		STATE_SELECTED_ANCHOR_BLUE(parent_state);
}


int32
lo_GetSubDocBaseline(LO_SubDocStruct *subdoc)
{
	LO_Element **line_array;
	LO_Element *eptr;
	lo_DocState *subdoc_state;

	subdoc_state = (lo_DocState *)subdoc->state;
	if (subdoc_state == NULL)
	{
		return(0);
	}

	/*
	 * Make eptr point to the start of the element chain
	 * for this subdoc.
	 */
#ifdef XP_WIN16
{
	XP_Block *larray_array;

	if (subdoc_state->larray_array == NULL)
	{
		return(0);
	}
	XP_LOCK_BLOCK(larray_array, XP_Block *, subdoc_state->larray_array);
	subdoc_state->line_array = larray_array[0];
	XP_UNLOCK_BLOCK(subdoc_state->larray_array);
}
#endif /* XP_WIN16 */
	if (subdoc_state->line_array == NULL)
	{
		return(0);
	}
	XP_LOCK_BLOCK(line_array, LO_Element **, subdoc_state->line_array);
	eptr = line_array[0];
	XP_UNLOCK_BLOCK(subdoc_state->line_array);

	while (eptr != NULL)
	{
		if (eptr->type == LO_LINEFEED)
		{
			break;
		}
		eptr = eptr->lo_any.next;
	}
	if (eptr == NULL)
	{
		return(0);
	}
	return(eptr->lo_linefeed.baseline);
}


void
lo_BeginSubDoc(MWContext *context, lo_DocState *state, PA_Tag *tag)
{
	LO_SubDocStruct *subdoc;
	lo_DocState *new_state;
	PA_Block buff;
	char *str;
	int32 val;
	int32 first_width, first_height;

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

	subdoc->vert_alignment = LO_ALIGN_TOP;
	subdoc->alignment = LO_ALIGN_LEFT;

	subdoc->border_width = SUBDOC_DEF_ANCHOR_BORDER;
	subdoc->border_vert_space = SUBDOC_DEF_VERTICAL_SPACE;
	subdoc->border_horiz_space = SUBDOC_DEF_HORIZONTAL_SPACE;

	subdoc->ele_attrmask = 0;

	subdoc->sel_start = -1;
	subdoc->sel_end = -1;


	/*
	 * Check for an align parameter
	 */
	subdoc->alignment = LO_ALIGN_LEFT;
	subdoc->ele_attrmask |= LO_ELE_FLOATING;
	buff = lo_FetchParamValue(context, tag, PARAM_ALIGN);
	if (buff != NULL)
	{
		Bool floating;

		floating = TRUE;
		PA_LOCK(str, char *, buff);
		subdoc->alignment = lo_EvalAlignParam(str, &floating);
		if (floating == FALSE)
		{
			subdoc->ele_attrmask &= (~(LO_ELE_FLOATING));
		}
		PA_UNLOCK(buff);
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
	}

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
			val = state->win_width * val / 100;
		}
		else
		{
			val = FEUNITS_X(val, context);
		}
		subdoc->width = val;
		if (subdoc->width < 1)
		{
			subdoc->width = 1;
		}
		PA_UNLOCK(buff);
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
			val = state->win_height * val / 100;
		}
		else
		{
			val = FEUNITS_Y(val, context);
		}
		subdoc->height = val;
		if (subdoc->height < 1)
		{
			subdoc->height = 1;
		}
		PA_UNLOCK(buff);
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
		subdoc->border_width = val;
		PA_UNLOCK(buff);
	}
	subdoc->border_width = FEUNITS_X(subdoc->border_width, context);

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
		subdoc->border_vert_space = val;
		PA_UNLOCK(buff);
	}
	subdoc->border_vert_space = FEUNITS_Y(subdoc->border_vert_space,
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
		subdoc->border_horiz_space = val;
		PA_UNLOCK(buff);
	}
	subdoc->border_horiz_space = FEUNITS_X(subdoc->border_horiz_space,
						context);

	if (subdoc->width == 0)
	{
		first_width = FEUNITS_X(5000, context);
	}
	else
	{
		first_width = subdoc->width;
	}

	if (subdoc->height == 0)
	{
		first_height = FEUNITS_Y(10000, context);
	}
	else
	{
		first_height = subdoc->height;
	}

	new_state = lo_NewLayout(context, first_width, first_height, 0, 0, NULL);
	if (new_state == NULL)
	{
		lo_FreeElement(context, (LO_Element *)subdoc, FALSE);
		return;
	}
	new_state->is_a_subdoc = SUBDOC_IS;
	lo_InheritParentState(new_state, state);

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

	state->sub_state = new_state;

	state->current_ele = (LO_Element *)subdoc;
}


void
lo_EndSubDoc(MWContext *context, lo_DocState *state, lo_DocState *old_state,
	LO_Element *element)
{
	LO_Element *eptr;
	LO_SubDocStruct *subdoc;
	PA_Block buff;
	char *str;
	Bool line_break;
	int32 vert_inc;
	int32 baseline_inc;
	int32 line_inc;
	int32 subdoc_height, subdoc_width;
	LO_TextStruct tmp_text;
	LO_TextInfo text_info;
	LO_Element **line_array;

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

	if (subdoc->width == 0)
	{
		subdoc->width = old_state->max_width +
			(2 * subdoc->border_width);
	}

	if (subdoc->height == 0)
	{
		subdoc->height = old_state->y + (2 * subdoc->border_width);
	}

	/*
	 * Mess with vertical alignment.
	 */
	vert_inc = subdoc->height - (old_state->y + (2 * subdoc->border_width));
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
		switch (eptr->type)
		{
		    case LO_LINEFEED:
			if ((eptr->lo_linefeed.x + eptr->lo_linefeed.x_offset +
				eptr->lo_linefeed.width) >
				(subdoc->width -1 - (2 * subdoc->border_width)))
			{
				eptr->lo_linefeed.width = subdoc->width - 1 -
					(2 * subdoc->border_width) -
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

				side_inc = (subdoc->width - 1 -
					(2 * subdoc->border_width)) -
					(c_eptr->lo_linefeed.x +
					c_eptr->lo_linefeed.x_offset);
				side_inc = side_inc / 2;
				if (side_inc > 0)
				{
					c_eptr->lo_linefeed.width -= side_inc;
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
		    default:
			break;
		}
		eptr->lo_any.y += vert_inc;
		eptr = eptr->lo_any.next;
	}

	subdoc_width = subdoc->width + (2 * subdoc->border_width) +
		(2 * subdoc->border_horiz_space);
	subdoc_height = subdoc->height + (2 * subdoc->border_width) +
		(2 * subdoc->border_vert_space);

	/*
	 * SEVERE FLOW BREAK!  This may be a floating subdoc,
	 * which means at this point we go do something completely
	 * different.
	 */
	if (subdoc->ele_attrmask & LO_ELE_FLOATING)
	{
		if (subdoc->alignment == LO_ALIGN_RIGHT)
		{
			subdoc->x = state->right_margin - subdoc_width;
			if (subdoc->x < 0)
			{
				subdoc->x = 0;
			}
		}
		else
		{
			subdoc->x = state->left_margin;
		}

		subdoc->y = -1;
		subdoc->x_offset += (int16)subdoc->border_horiz_space;
		subdoc->y_offset += (int32)subdoc->border_vert_space;

		old_state->base_x = subdoc->x + subdoc->x_offset +
			subdoc->border_width;
		if (subdoc->y < 0)
		{
			old_state->base_y = subdoc->y;
		}
		else
		{
			old_state->base_y = subdoc->y + subdoc->y_offset +
				subdoc->border_width;
		}

		lo_AddMarginStack(state, subdoc->x, subdoc->y,
			subdoc->width, subdoc->height,
			subdoc->border_width, 
			subdoc->border_vert_space, subdoc->border_horiz_space,
			(intn)subdoc->alignment);

		/*
		 * Insert this element into the float list.
		 */
		subdoc->next = state->float_list;
		state->float_list = (LO_Element *)subdoc;

		if (state->at_begin_line != FALSE)
		{
			lo_FindLineMargins(context, state);
			state->x = state->left_margin;

			if (state->display_blocked == FALSE)
			{
				subdoc->x += state->base_x;
				subdoc->y += state->base_y;
				lo_DisplaySubDoc(context, FE_VIEW, subdoc);
				subdoc->x -= state->base_x;
				subdoc->y -= state->base_y;
			}
		}
		else
		{
		}
		return;
	}

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
         * break on the subdoc if we have
         * a break.
         */
        if (line_break != FALSE)
        {
		/*
		 * We need to make the elements sequential, linefeed
		 * before image.
		 */
		state->top_state->element_id = subdoc->ele_id;

                lo_HardLineBreak(context, state, TRUE);
                subdoc->x = state->x;
                subdoc->y = state->y;
		subdoc->ele_id = NEXT_ELEMENT;
        }

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

	lo_AppendToLineList(context, state,
		(LO_Element *)subdoc, baseline_inc);

	state->baseline += (intn) baseline_inc;
	state->line_height += (intn) (baseline_inc + line_inc);

	/*
	 * Clean up state
	 */
	state->x = state->x + subdoc->x_offset +
		subdoc_width - subdoc->border_horiz_space;
	state->linefeed_state = 0;
	state->at_begin_line = FALSE;
	state->trailing_space = FALSE;
	state->cur_ele_type = LO_SUBDOC;
}

#ifdef TEST_16BIT
#undef XP_WIN16
#endif /* TEST_16BIT */

