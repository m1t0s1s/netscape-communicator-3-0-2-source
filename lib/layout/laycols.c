#include "xp.h"
#include "pa_parse.h"
#include "layout.h"


#define MULTICOL_MIN_WIDTH	10
#define MULTICOL_GUTTER_WIDTH	10


void
lo_BeginMulticolumn(MWContext *context, lo_DocState *state, PA_Tag *tag)
{
	lo_MultiCol *multicol;
	PA_Block buff;
	char *str;
	int32 val;
	int32 doc_width;
	int32 width;

	multicol = XP_NEW(lo_MultiCol);
	if (multicol == NULL)
	{
		state->top_state->out_of_memory = TRUE;
		return;
	}

	lo_SoftLineBreak(context, state, FALSE, 2);

	multicol->end_last_line = state->end_last_line;
	multicol->start_ele = state->top_state->element_id;
	multicol->start_line = state->line_num;
	multicol->end_line = multicol->start_line;
	multicol->start_x = state->x;
	multicol->start_y = state->y;
	multicol->end_y = multicol->start_y;
	multicol->cols = 1;
	multicol->gutter = MULTICOL_GUTTER_WIDTH;

	/*
         * Get the cols parameter.
         */
        buff = lo_FetchParamValue(context, tag, PARAM_COLS);
        if (buff != NULL)
        {
                PA_LOCK(str, char *, buff);
		multicol->cols = XP_ATOI(str);
                PA_UNLOCK(buff);
		PA_FREE(buff);
	}

	if (multicol->cols <= 1)
	{
		XP_DELETE(multicol);
		return;
	}

	/*
         * Get the gutter parameter.
         */
        buff = lo_FetchParamValue(context, tag, PARAM_GUTTER);
        if (buff != NULL)
        {
                PA_LOCK(str, char *, buff);
		multicol->gutter = XP_ATOI(str);
		if (multicol->gutter < 1)
		{
			multicol->gutter = 1;
		}
                PA_UNLOCK(buff);
		PA_FREE(buff);
	}
	multicol->gutter = FEUNITS_X(multicol->gutter, context);

	doc_width = state->right_margin - state->left_margin;
	width = doc_width;

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
		width = val;
		PA_UNLOCK(buff);
		PA_FREE(buff);
	}

	width = width - ((multicol->cols - 1) * multicol->gutter);
	multicol->col_width = width / multicol->cols;
	if (multicol->col_width < MULTICOL_MIN_WIDTH)
	{
		multicol->col_width = MULTICOL_MIN_WIDTH;
	}

	multicol->orig_margin = state->right_margin;
	lo_PushList(state, tag);
	state->right_margin = state->left_margin + multicol->col_width;
	state->x = state->left_margin;
	state->list_stack->old_left_margin = state->left_margin;
	state->list_stack->old_right_margin = state->right_margin;

	multicol->orig_min_width = state->min_width;
	state->min_width = 0;
	/*
	 * Don't display anything while processing the multicolumn text.
	 */
	multicol->orig_display_blocking_element_y = state->display_blocking_element_y;
	state->display_blocking_element_y = -1;
	multicol->orig_display_blocked = state->display_blocked;
	state->display_blocked = TRUE;

	multicol->next = state->current_multicol;
	state->current_multicol = multicol;
}


static LO_Element *
lo_capture_floating_elements(MWContext *context, lo_DocState *state,
		int32 ele1, int32 ele2, int32 top_y, int32 bottom_y,
		int32 *max_y)
{
	LO_Element *prev_eptr;
	LO_Element *eptr;
	LO_Element *float_list;

	*max_y = bottom_y;
	float_list = NULL;
	prev_eptr = NULL;
	eptr = state->float_list;
	while (eptr != NULL)
	{
		LO_Element *element;

		if (((eptr->lo_any.y >= top_y)&&
		     (eptr->lo_any.y < bottom_y))||
		    ((eptr->lo_any.ele_id >= ele1)&&
		     (eptr->lo_any.ele_id <= ele2)))
		{
			int32 my_y;

			element = eptr;
			if (prev_eptr != NULL)
			{
				prev_eptr->lo_any.next = eptr->lo_any.next;
			}
			else if (eptr == state->float_list)
			{
				state->float_list = eptr->lo_any.next;
			}
			eptr = eptr->lo_any.next;

			element->lo_any.next = float_list;
			float_list = element;

			my_y = element->lo_any.y +
				element->lo_any.y_offset +
				element->lo_any.height;
			if (element->type == LO_IMAGE)
			{
				/*
				 * Images need to account for border width
				 */
				my_y = my_y +
					(2 * element->lo_image.border_width);
			}
			if (my_y > *max_y)
			{
				*max_y = my_y;
			}
		}
		else
		{
			prev_eptr = eptr;
			eptr = eptr->lo_any.next;
		}
	}

	return(float_list);
}


LO_CellStruct *
lo_CaptureLines(MWContext *context, lo_DocState *state, int32 col_width,
		int32 line1, int32 line2, int32 dx, int32 dy)
{
	int32 height, max_y;
	int32 ele1, ele2;
	LO_Element *eptr;
	LO_Element *last_eptr;
	LO_Element *past_eptr;
	LO_CellStruct *cell;

	eptr = lo_FirstElementOfLine(context, state, line1);
	if (eptr == NULL)
	{
		return(NULL);
	}

	last_eptr = NULL;
	past_eptr = lo_FirstElementOfLine(context, state, (line2 + 1));
	if (past_eptr != NULL)
	{
		last_eptr = past_eptr->lo_any.prev;
	}

	if (last_eptr == NULL)
	{
		last_eptr = lo_FirstElementOfLine(context, state, line2);
		if (last_eptr == NULL)
		{
			last_eptr = eptr;
		}
		while (last_eptr->lo_any.next != NULL)
		{
			last_eptr = last_eptr->lo_any.next;
		}
	}

	if (past_eptr != NULL)
	{
		height = past_eptr->lo_any.y - eptr->lo_any.y;
	}
	else
	{
		height = last_eptr->lo_any.y + last_eptr->lo_any.line_height -
				eptr->lo_any.y;
	}

	cell = (LO_CellStruct *)lo_NewElement(context, state, LO_CELL, NULL, 0);
	if (cell == NULL)
	{
		state->top_state->out_of_memory = TRUE;
		return(NULL);
	}

	cell->type = LO_CELL;
	cell->ele_id = NEXT_ELEMENT;
	cell->x = eptr->lo_any.x + dx;
	cell->x_offset = 0;
	cell->y = eptr->lo_any.y + dy;
	cell->y_offset = 0;
	cell->width = col_width;
	cell->height = height;
	cell->next = NULL;
	cell->prev = NULL;
	cell->FE_Data = NULL;
	cell->cell_float_list = NULL;
	cell->bg_color = NULL;
	cell->border_width = 0;
	cell->border_vert_space = 0;
	cell->border_horiz_space = 0;
	cell->ele_attrmask = 0;
	cell->sel_start = -1;
	cell->sel_end = -1;
	cell->cell_list = eptr;
	cell->cell_list_end = last_eptr;
#ifdef LAYERS
        cell->cell_layer = NULL;
        cell->backdrop_image = NULL;
#endif /* LAYERS */

	if (eptr->lo_any.prev != NULL)
	{
		eptr->lo_any.prev->lo_any.next = NULL;
		eptr->lo_any.prev = NULL;
	}
	if (last_eptr->lo_any.next != NULL)
	{
		last_eptr->lo_any.next->lo_any.prev = NULL;
		last_eptr->lo_any.next = NULL;
	}

	ele1 = eptr->lo_any.ele_id;
	ele2 = last_eptr->lo_any.ele_id;

	cell->cell_float_list = lo_capture_floating_elements(context, state,
		ele1, ele2, eptr->lo_any.y, (eptr->lo_any.y + height), &max_y);
	if ((max_y - eptr->lo_any.y) > cell->height)
	{
		cell->height = max_y - eptr->lo_any.y;
	}

	lo_ShiftCell(cell, dx, dy);

	return(cell);
}


static int32
lo_find_breaking_line(MWContext *context, lo_DocState *state,
			int32 start_line, int32 start_x, int32 end_x)
{
	LO_Element *eptr;
	int32 line;

	line = start_line + 1;
	eptr = lo_FirstElementOfLine(context, state, line);
	while (eptr != NULL)
	{
		if (eptr->lo_any.x == start_x)
		{
			LO_Element *tmp_eptr;

			tmp_eptr = eptr;
			while ((tmp_eptr != NULL)&&
				(tmp_eptr->type != LO_LINEFEED))
			{
				tmp_eptr = tmp_eptr->lo_any.next;
			}
			if ((tmp_eptr != NULL)&&
			    ((tmp_eptr->lo_any.x + tmp_eptr->lo_any.x_offset +
			      tmp_eptr->lo_any.width) >= end_x))
			{
				return(line - 1);
			}
		}
		line++;
		eptr = lo_FirstElementOfLine(context, state, line);
	}
	return(line - 1);
}


void
lo_EndMulticolumn(MWContext *context, lo_DocState *state, PA_Tag *tag,
			lo_MultiCol *multicol)
{
	int32 i;
	lo_ListStack *lptr;
	int32 height;
	int32 col_height;
	int32 x, y;
	int32 line1, line2;
	int32 save_doc_min_width;
	int32 min_multi_width;
	LO_Element *cell_list;
	LO_Element *cell_list_end;
	LO_Element *cell_ele;
	LO_TableStruct *table_ele;

	cell_ele = NULL;
	cell_list = NULL;
	cell_list_end = NULL;

	lo_SoftLineBreak(context, state, FALSE, 1);

	multicol->end_line = state->line_num;
	multicol->end_y = state->y;

	/*
	 * Break to clear all left and right margins.
	 */
	lo_ClearToBothMargins(context, state);

	/*
	 * Reset the margins properly in case
	 * we are inside a list.
	 */
	lo_FindLineMargins(context, state);
	state->x = state->left_margin;

	height = multicol->end_y - multicol->start_y;
	col_height = height / multicol->cols;
	if (col_height < 1)
	{
		col_height = 1;
	}

	x = state->x;
	y = multicol->start_y;
	line1 = multicol->start_line - 1;
	for (i=0; i<multicol->cols; i++)
	{
		LO_CellStruct *cell;
		LO_Element *eptr;
		int32 line;
		int32 dx, dy;
		int32 move_height;

		eptr = lo_FirstElementOfLine(context, state, line1);
		if (eptr == NULL)
		{
			break;
		}
		y = eptr->lo_any.y + col_height;

		line = lo_PointToLine(context, state, x, y);
		eptr = lo_FirstElementOfLine(context, state, line);
		if (eptr->lo_any.x > multicol->start_x)
		{
			line = lo_find_breaking_line(context, state, line,
				multicol->start_x,
				(multicol->start_x + multicol->col_width));
		}

		line2 = line;
		if (line2 > (multicol->end_line - 2))
		{
			line2 = (multicol->end_line - 2);
		}
		cell = NULL;
		if (i > 0)
		{
			eptr = lo_FirstElementOfLine(context, state, line1);
			if (eptr != NULL)
			{
				dx = i * (multicol->col_width +
						multicol->gutter);
				dy = multicol->start_y - eptr->lo_any.y;
				cell = lo_CaptureLines(context, state,
					multicol->col_width,
					line1, line2, dx, dy);
				if (cell == NULL)
				{
					move_height = 0;
				}
				else
				{
					move_height = cell->height - 1;
				}
			}
			else
			{
				move_height = 0;
			}
		}
		else
		{
			cell = lo_CaptureLines(context, state,
				multicol->col_width,
				line1, line2, 0, 0);
			if (cell == NULL)
			{
				move_height = 0;
			}
			else
			{
				move_height = cell->height - 1;
			}
		}
		line1 = line2 + 1;
		if (move_height > col_height)
		{
			col_height = move_height;
		}

		if (cell != NULL)
		{
			if (cell_list_end == NULL)
			{
				cell_list = (LO_Element *)cell;
				cell_list_end = cell_list;
			}
			else
			{
				cell_list_end->lo_any.next = (LO_Element *)cell;
				cell_list_end = cell_list_end->lo_any.next;
			}
		}
	}

	lptr = lo_PopList(state, tag);
	if (lptr != NULL)
	{
		XP_DELETE(lptr);
	}
	state->left_margin = state->list_stack->old_left_margin;
	state->right_margin = state->list_stack->old_right_margin;

	state->top_state->element_id = multicol->start_ele;
	lo_SetLineArrayEntry(state, NULL, (multicol->start_line - 1));
	state->line_num = multicol->start_line;
	state->end_last_line = multicol->end_last_line;;
	if (state->end_last_line != NULL)
	{
		state->end_last_line->lo_any.next = NULL;
	}
	state->line_list = NULL;
	state->y = multicol->start_y;

	state->display_blocked = multicol->orig_display_blocked;
	state->display_blocking_element_y = multicol->orig_display_blocking_element_y;

	table_ele = (LO_TableStruct *)lo_NewElement(context, state, LO_TABLE,
					NULL, 0);
	if (table_ele == NULL)
	{
		state->top_state->out_of_memory = TRUE;
		return;
	}

	table_ele->type = LO_TABLE;
	table_ele->ele_id = NEXT_ELEMENT;
	table_ele->x = x;
	table_ele->x_offset = 0;
	table_ele->y = multicol->start_y;
	table_ele->y_offset = 0;
	table_ele->width = (multicol->cols * multicol->col_width) +
				((multicol->cols - 1) * multicol->gutter);
	table_ele->height = col_height + 1;
	table_ele->line_height = col_height + 1;
	table_ele->FE_Data = NULL;
	table_ele->anchor_href = NULL;
	table_ele->alignment = LO_ALIGN_LEFT;
	table_ele->border_width = 0;
	table_ele->border_vert_space = 0;
	table_ele->border_horiz_space = 0;
	table_ele->ele_attrmask = 0;
	table_ele->sel_start = -1;
	table_ele->sel_end = -1;
	table_ele->next = NULL;
	table_ele->prev = NULL;
	lo_AppendToLineList(context, state, (LO_Element *)table_ele, 0);

	while (cell_list != NULL)
	{
		cell_ele = cell_list;
		cell_list = cell_list->lo_any.next;
		cell_ele->lo_any.next = NULL;
		cell_ele->lo_any.ele_id = NEXT_ELEMENT;
		lo_RenumberCell(state, (LO_CellStruct *)cell_ele);
		lo_AppendToLineList(context, state, cell_ele, 0);
	}

	if (cell_ele != NULL)
	{
		state->x = cell_ele->lo_any.x + multicol->col_width;
	}
	else
	{
		state->x = table_ele->x + table_ele->width;
	}
	state->baseline = 0;
	state->line_height = col_height + 1;
	state->linefeed_state = 0;
	state->at_begin_line = FALSE;
	state->trailing_space = FALSE;
	state->cur_ele_type = LO_SUBDOC;


	min_multi_width = (state->min_width * multicol->cols) +
				((multicol->cols - 1) * multicol->gutter);
	state->min_width = multicol->orig_min_width;
	save_doc_min_width = state->min_width;
	lo_SoftLineBreak(context, state, FALSE, 2);
	if (min_multi_width > save_doc_min_width)
	{
		save_doc_min_width = min_multi_width;
	}
	state->min_width = save_doc_min_width;

	state->current_multicol = multicol->next;
	XP_DELETE(multicol);
}

