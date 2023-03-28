#include "xp.h"
#include "pa_parse.h"
#include "layout.h"

#ifdef PROFILE
#pragma profile on
#endif

#ifdef TEST_16BIT
#define XP_WIN16
#endif /* TEST_16BIT */


void
lo_display_element_list(MWContext *context, int iLocation, LO_Element *eptr,
	int32 base_x, int32 base_y)
{
	while (eptr != NULL)
	{
		eptr->lo_any.x += base_x;
		eptr->lo_any.y += base_y;

		switch (eptr->type)
		{
			case LO_TEXT:
				if (eptr->lo_text.text != NULL)
				{
					lo_DisplayText(context, FE_VIEW,
						(LO_TextStruct *)eptr, FALSE);
				}
				break;
			case LO_LINEFEED:
				lo_DisplayLineFeed(context, FE_VIEW,
					(LO_LinefeedStruct *)eptr, FALSE);
				break;
			case LO_HRULE:
				lo_DisplayHR(context, FE_VIEW,
					(LO_HorizRuleStruct *)eptr);
				break;
			case LO_FORM_ELE:
				lo_DisplayFormElement(context, FE_VIEW,
					(LO_FormElementStruct *)eptr);
				break;
			case LO_BULLET:
				lo_DisplayBullet(context, FE_VIEW,
					(LO_BulletStruct *)eptr);
				break;
			case LO_IMAGE:
				lo_DisplayImage(context, FE_VIEW,
					(LO_ImageStruct *)eptr);
				break;
			case LO_TABLE:
				lo_DisplayTable(context, FE_VIEW,
					(LO_TableStruct *)eptr);
				break;
			case LO_EMBED:
				lo_DisplayEmbed(context, FE_VIEW,
					(LO_EmbedStruct *)eptr);
				break;
			case LO_JAVA:
				lo_DisplayJavaApp(context, FE_VIEW,
					(LO_JavaAppStruct *)eptr);
				break;
			case LO_EDGE:
				lo_DisplayEdge(context, FE_VIEW,
					(LO_EdgeStruct *)eptr);
				break;
			case LO_CELL:
				lo_DisplayCell(context, FE_VIEW,
					(LO_CellStruct *)eptr);
				lo_DisplayCellContents(context, FE_VIEW,
					(LO_CellStruct *)eptr, base_x, base_y);
				break;
			case LO_SUBDOC:
			    {
				LO_SubDocStruct *subdoc;
				int32 new_x, new_y;
				uint32 new_width, new_height;
				lo_DocState *sub_state;

				subdoc = (LO_SubDocStruct *)eptr;
				sub_state = (lo_DocState *)subdoc->state;

				if (sub_state == NULL)
				{
					break;
				}

				lo_DisplaySubDoc(context, FE_VIEW, subdoc);

				new_x = subdoc->x;
				new_y = subdoc->y;
				new_width = subdoc->x_offset + subdoc->width;
				new_height = subdoc->y_offset + subdoc->height;

				new_x = new_x - subdoc->x;
				new_y = new_y - subdoc->y;
				sub_state->base_x = subdoc->x +
					subdoc->x_offset + subdoc->border_width;
				sub_state->base_y = subdoc->y +
					subdoc->y_offset + subdoc->border_width;
				lo_RefreshDocumentArea(context, sub_state,
					new_x, new_y, new_width, new_height);
			    }
			    break;
			default:
				break;
		}

                eptr->lo_any.x -= base_x;
                eptr->lo_any.y -= base_y;

                eptr = eptr->lo_any.next;
        }
}


void
lo_DisplayCellContents(MWContext *context, int iLocation, LO_CellStruct *cell,
	int32 base_x, int32 base_y)
{
	LO_Element *eptr;

	eptr = cell->cell_list;
	lo_display_element_list(context, iLocation, eptr, base_x, base_y);
	eptr = cell->cell_float_list;
	lo_display_element_list(context, iLocation, eptr, base_x, base_y);
}


LO_Element *
lo_search_element_list(MWContext *context, LO_Element *eptr, int32 x, int32 y)
{
	while (eptr != NULL)
	{
		int32 width, height;

		width = eptr->lo_any.width;
		/*
		 * Images need to account for border width
		 */
		if (eptr->type == LO_IMAGE)
		{
			width = width + (2 * eptr->lo_image.border_width);
		}
		if (width <= 0)
		{
			width = 1;
		}

		height = eptr->lo_any.height;
		/*
		 * Images need to account for border width
		 */
		if (eptr->type == LO_IMAGE)
		{
			height = height + (2 * eptr->lo_image.border_width);
		}

		if ((y < (eptr->lo_any.y + eptr->lo_any.y_offset + height))&&
			(y >= eptr->lo_any.y))
		{
			if ((x < (eptr->lo_any.x + eptr->lo_any.x_offset +
				width))&&(x >= eptr->lo_any.x))
			{
				/*
				 * Don't stop on tables, they are containers
				 * we must look inside of.
				 */
				if (eptr->type != LO_TABLE)
				{
					break;
				}
			}
		}
		eptr = eptr->lo_any.next;
	}
	return(eptr);
}


LO_Element *
lo_XYToCellElement(MWContext *context, lo_DocState *state,
	LO_CellStruct *cell, int32 x, int32 y,
	Bool check_float, Bool into_cells)
{
	LO_Element *eptr;
	LO_Element *element;

	eptr = cell->cell_list;
	element = lo_search_element_list(context, eptr, x, y);

	if ((element == NULL)&&(check_float != FALSE))
	{
		eptr = cell->cell_float_list;
		element = lo_search_element_list(context, eptr, x, y);
	}

	if ((element != NULL)&&(element->type == LO_SUBDOC))
	{
		LO_SubDocStruct *subdoc;
		int32 new_x, new_y;
		int32 ret_new_x, ret_new_y;

		subdoc = (LO_SubDocStruct *)element;

		new_x = x - (subdoc->x + subdoc->x_offset +
			subdoc->border_width);
		new_y = y - (subdoc->y + subdoc->y_offset +
			subdoc->border_width);
		element = lo_XYToDocumentElement(context,
			(lo_DocState *)subdoc->state, new_x, new_y, check_float,
			into_cells, &ret_new_x, &ret_new_y);
		x = ret_new_x;
		y = ret_new_y;
	}
	else if ((element != NULL)&&(element->type == LO_CELL)&&
		 (into_cells != FALSE))
	{
		element = lo_XYToCellElement(context, state,
			(LO_CellStruct *)element, x, y,
			check_float, into_cells);
	}

	return(element);
}


LO_Element *
lo_XYToNearestCellElement(MWContext *context, lo_DocState *state,
	LO_CellStruct *cell, int32 x, int32 y)
{
	LO_Element *eptr;

	eptr = lo_XYToCellElement(context, state, cell, x, y, TRUE, TRUE);

	/*
	 * If no exact hit, find nearest in Y overlap.
	 */
	if (eptr == NULL)
	{
		eptr = cell->cell_list;
		while (eptr != NULL)
		{
			int32 y2;

			/*
			 * Skip tables, we just walk into them.
			 */
			if (eptr->type == LO_TABLE)
			{
				eptr = eptr->lo_any.next;
				continue;
			}

			y2 = eptr->lo_any.y + eptr->lo_any.y_offset +
				eptr->lo_any.height;
			/*
			 * Images and cells need to account for border width
			 */
			if (eptr->type == LO_IMAGE)
			{
				y2 = y2 + (2 * eptr->lo_image.border_width);
			}
			else if (eptr->type == LO_CELL)
			{
				y2 = y2 + (2 * eptr->lo_cell.border_width);
			}

			if (y <= y2)
			{
				break;
			}
			eptr = eptr->lo_any.next;
		}

		/*
		 * If nothing overlaps y, match the last element in the cell.
		 */
		if (eptr == NULL)
		{
			eptr = cell->cell_list_end;
		}

		/*
		 * If we matched on a cell, recurse into it.
		 */
		if ((eptr != NULL)&&(eptr->type == LO_CELL))
		{
			eptr = lo_XYToNearestCellElement(context, state,
				(LO_CellStruct *)eptr, x, y);
		}
	}
	return(eptr);
}


void
lo_ShiftCell(LO_CellStruct *cell, int32 dx, int32 dy)
{
	LO_Element *eptr;

	if (cell == NULL)
	{
		return;
	}

	eptr = cell->cell_list;
	while (eptr != NULL)
	{
		eptr->lo_any.x += dx;
		eptr->lo_any.y += dy;
		if (eptr->type == LO_CELL)
		{
			/*
			 * This would cause an infinite loop.
			 */
			if ((LO_CellStruct *)eptr == cell)
			{
				XP_ASSERT(FALSE);
				break;
			}
			lo_ShiftCell((LO_CellStruct *)eptr, dx, dy);
		}

		eptr = eptr->lo_any.next;
	}

	eptr = cell->cell_float_list;
	while (eptr != NULL)
	{
		eptr->lo_any.x += dx;
		eptr->lo_any.y += dy;
		if (eptr->type == LO_CELL)
		{
			lo_ShiftCell((LO_CellStruct *)eptr, dx, dy);
		}

		eptr = eptr->lo_any.next;
	}
}


static void
lo_recolor_backgrounds(lo_DocState *state, LO_CellStruct *cell,
	LO_Element *eptr)
{
	LO_TextAttr *old_attr;
	LO_TextAttr *attr;
	LO_TextAttr tmp_attr;

	if (cell->bg_color == NULL)
	{
		return;
	}

	attr = NULL;
	old_attr = NULL;
	if (eptr->type == LO_TEXT)
	{
		old_attr = eptr->lo_text.text_attr;
	}
	else if (eptr->type == LO_LINEFEED)
	{
		old_attr = eptr->lo_linefeed.text_attr;
	}
	else if (eptr->type == LO_IMAGE)
	{
		old_attr = eptr->lo_image.text_attr;
	}
	else if (eptr->type == LO_FORM_ELE)
	{
		old_attr = eptr->lo_form.text_attr;
	}

	if (old_attr != NULL)
	{
		lo_CopyTextAttr(old_attr, &tmp_attr);
		tmp_attr.bg.red = cell->bg_color->red;
		tmp_attr.bg.green = cell->bg_color->green;
		tmp_attr.bg.blue = cell->bg_color->blue;
		tmp_attr.no_background = FALSE;
		attr = lo_FetchTextAttr(state, &tmp_attr);
	}

	if (eptr->type == LO_TEXT)
	{
		eptr->lo_text.text_attr = attr;
	}
	else if (eptr->type == LO_LINEFEED)
	{
		eptr->lo_linefeed.text_attr = attr;
	}
	else if (eptr->type == LO_IMAGE)
	{
		eptr->lo_image.text_attr = attr;
	}
	else if (eptr->type == LO_FORM_ELE)
	{
		eptr->lo_form.text_attr = attr;
	}
}


/*
 * In addition to renumbering the cell, we now
 * also make sure that text in cell with a bg_color
 * have their text_attr->bg set correctly.
 */
void
lo_RenumberCell(lo_DocState *state, LO_CellStruct *cell)
{
	LO_Element *eptr;

	if (cell == NULL)
	{
		return;
	}

	eptr = cell->cell_list;
	while (eptr != NULL)
	{
		/* Keep element IDs in order */
		eptr->lo_any.ele_id = NEXT_ELEMENT;

		/*
		 * If the display is blocked for an element
		 * we havn't reached yet, check to see if
		 * this element is it, and if so, save its
		 * y position.
		 */
		if ((state->display_blocked != FALSE)&&
		    (state->is_a_subdoc == SUBDOC_NOT)&&
		    (state->display_blocking_element_y == 0)&&
		    (state->display_blocking_element_id != -1)&&
		    (eptr->lo_any.ele_id >=
			state->display_blocking_element_id))
		{
			state->display_blocking_element_y = eptr->lo_any.y;
		}

		/*
		 * If this is a cell with a BGCOLOR attribute, pass
		 * that color on to the bg of all text in this cell.
		 */
		if ((cell->bg_color != NULL)&&((eptr->type == LO_TEXT)||
		    (eptr->type == LO_LINEFEED)||(eptr->type == LO_IMAGE)||
		    (eptr->type == LO_FORM_ELE)))
		{
			lo_recolor_backgrounds(state, cell, eptr);
		}

		if (eptr->type == LO_CELL)
		{
			lo_RenumberCell(state, (LO_CellStruct *)eptr);
		}

		eptr = eptr->lo_any.next;
	}

	eptr = cell->cell_float_list;
	while (eptr != NULL)
	{
		/* Keep element IDs in order */
		eptr->lo_any.ele_id = NEXT_ELEMENT;

		/*
		 * If the display is blocked for an element
		 * we havn't reached yet, check to see if
		 * this element is it, and if so, save its
		 * y position.
		 */
		if ((state->display_blocked != FALSE)&&
		    (state->is_a_subdoc == SUBDOC_NOT)&&
		    (state->display_blocking_element_y == 0)&&
		    (state->display_blocking_element_id != -1)&&
		    (eptr->lo_any.ele_id >=
			state->display_blocking_element_id))
		{
			state->display_blocking_element_y = eptr->lo_any.y;
		}

		/*
		 * If this is a cell with a BGCOLOR attribute, pass
		 * that color on to the bg of all text in this cell.
		 */
		if ((cell->bg_color != NULL)&&((eptr->type == LO_TEXT)||
		    (eptr->type == LO_LINEFEED)||(eptr->type == LO_IMAGE)||
		    (eptr->type == LO_FORM_ELE)))
		{
			lo_recolor_backgrounds(state, cell, eptr);
		}

		if (eptr->type == LO_CELL)
		{
			lo_RenumberCell(state, (LO_CellStruct *)eptr);
		}

		eptr = eptr->lo_any.next;
	}
}


static void
lo_free_element_list(MWContext *context, LO_Element *elist)
{
	LO_Element *eptr;
	LO_Element *element;

	eptr = elist;
	while (eptr != NULL)
	{
		element = eptr;
		eptr = eptr->lo_any.next;
		lo_FreeElement(context, element, TRUE);
	}
}


static void
lo_free_subdoc_state(MWContext *context, lo_DocState *parent_state,
#ifdef SCARY_LEAK_FIX
			lo_DocState *state, Bool free_all)
#else
			lo_DocState *state)
#endif /* SCARY_LEAK_FIX */
{
	LO_Element **line_array;

	if (state == NULL)
	{
		return;
	}

	/*
	 * If this subdoc is not nested inside another subdoc,
	 * free any re-parse tags stored on this subdoc.
	 */
	if ((state->subdoc_tags != NULL)&&
		(parent_state->is_a_subdoc != SUBDOC_CELL)&&
		(parent_state->is_a_subdoc != SUBDOC_CAPTION))
	{
		PA_Tag *tptr;
		PA_Tag *tag;

		tptr = state->subdoc_tags;
		while ((tptr != state->subdoc_tags_end)&&(tptr != NULL))
		{
			tag = tptr;
			tptr = tptr->next;
			tag->next = NULL;
			PA_FreeTag(tag);
		}
		if (tptr != NULL)
		{
			tptr->next = NULL;
			PA_FreeTag(tptr);
		}
		state->subdoc_tags = NULL;
		state->subdoc_tags_end = NULL;
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
		lo_free_element_list(context, state->line_list);
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
		/*
		 * Don't free these, they are squished into the cell.
		 * Only free them if this is a free_all.
		 */
#ifdef SCARY_LEAK_FIX
		if (free_all != FALSE)
		{
			lo_free_element_list(context, state->float_list);
		}
#endif /* SCARY_LEAK_FIX */
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
			/*
			 * Don't free these, they are squished into the cell.
			 * Only free them if this is a free_all.
			 */
#ifdef SCARY_LEAK_FIX
			if (free_all != FALSE)
			{
				lo_free_element_list(context, eptr);
			}
#endif /* SCARY_LEAK_FIX */
		}
		XP_UNLOCK_BLOCK(state->line_array);
		XP_FREE_BLOCK(state->line_array);
	}

	XP_DELETE(state);
}


#ifdef SCARY_LEAK_FIX
void
lo_FreePartialSubDoc(MWContext *context, lo_DocState *state,
	LO_SubDocStruct *subdoc)
{
	lo_DocState *subdoc_state;

	if (subdoc == NULL)
	{
		return;
	}

	subdoc_state = (lo_DocState *)subdoc->state;
	if (subdoc_state != NULL)
	{
		lo_free_subdoc_state(context, state, subdoc_state, TRUE);
	}

	subdoc->state = NULL;
	lo_FreeElement(context, (LO_Element *)subdoc, TRUE);
}
#endif /* SCARY_LEAK_FIX */


LO_CellStruct *
lo_SquishSubDocToCell(MWContext *context, lo_DocState *state,
	LO_SubDocStruct *subdoc)
{
	LO_CellStruct *cell;
	lo_DocState *subdoc_state;

	if (subdoc == NULL)
	{
		return(NULL);
	}

	cell = (LO_CellStruct *)lo_NewElement(context, state, LO_CELL, NULL, 0);
	if (cell == NULL)
	{
		return(NULL);
	}

	cell->type = LO_CELL;

	cell->ele_id = NEXT_ELEMENT;
	cell->x = subdoc->x;
	cell->x_offset = subdoc->x_offset;
	cell->y = subdoc->y;
	cell->y_offset = subdoc->y_offset;
	cell->width = subdoc->width;
	cell->height = subdoc->height;
	cell->next = subdoc->next;
	cell->prev = subdoc->prev;
	cell->FE_Data = subdoc->FE_Data;

	cell->bg_color = subdoc->bg_color;
	/*
	 * Clear out the bg_color struct from the subdocs so it doesn't
	 * get freed twice.
	 */
	subdoc->bg_color = NULL;

	cell->border_width = subdoc->border_width;
	cell->border_vert_space = subdoc->border_vert_space;
	cell->border_horiz_space = subdoc->border_horiz_space;

	cell->ele_attrmask = 0;
	cell->ele_attrmask |= (subdoc->ele_attrmask & LO_ELE_SECURE);
	cell->ele_attrmask |= (subdoc->ele_attrmask & LO_ELE_SELECTED);

	cell->sel_start = subdoc->sel_start;
	cell->sel_end = subdoc->sel_end;

	cell->cell_list = NULL;
	cell->cell_list_end = NULL;
	cell->cell_float_list = NULL;
	subdoc_state = (lo_DocState *)subdoc->state;
	if (subdoc_state != NULL)
	{
		LO_Element **line_array;
		LO_Element *eptr;
		int32 base_x, base_y;

		/*
		 * Make eptr point to the start of the element chain
		 * for this subdoc.
		 */
#ifdef XP_WIN16
		{
			XP_Block *larray_array;

			XP_LOCK_BLOCK(larray_array, XP_Block *,
				subdoc_state->larray_array);
			subdoc_state->line_array = larray_array[0];
			XP_UNLOCK_BLOCK(subdoc_state->larray_array);
		}
#endif /* XP_WIN16 */
		XP_LOCK_BLOCK(line_array, LO_Element **,
			subdoc_state->line_array);
		eptr = line_array[0];
		XP_UNLOCK_BLOCK(subdoc_state->line_array);

		cell->cell_list = eptr;

		base_x = subdoc->x + subdoc->x_offset + subdoc->border_width;
		base_y = subdoc->y + subdoc->y_offset + subdoc->border_width;

		cell->cell_list_end = subdoc_state->end_last_line;
		if (cell->cell_list_end == NULL)
		{
			cell->cell_list_end = cell->cell_list;
		}

		cell->cell_float_list = subdoc_state->float_list;

		lo_ShiftCell(cell, base_x, base_y);
		lo_RenumberCell(state, cell);
		lo_AddNameList(state, subdoc_state);

#ifdef SCARY_LEAK_FIX
		lo_free_subdoc_state(context, state, subdoc_state, FALSE);
#else
		lo_free_subdoc_state(context, state, subdoc_state);
#endif /* SCARY_LEAK_FIX */
	}

	subdoc->state = NULL;
	lo_FreeElement(context, (LO_Element *)subdoc, TRUE);

	return(cell);
}


void
lo_BeginCell(MWContext *context, lo_DocState *state, PA_Tag *tag)
{
	LO_CellStruct *cell;

	cell = (LO_CellStruct *)lo_NewElement(context, state, LO_CELL, NULL, 0);
	if (cell == NULL)
	{
		return;
	}

	cell->type = LO_CELL;
	cell->ele_id = NEXT_ELEMENT;
	cell->x = state->x;
	cell->x_offset = 0;
	cell->y = state->y;
	cell->y_offset = 0;
	cell->width = 0;
	cell->height = 0;
	cell->next = NULL;
	cell->prev = NULL;

	cell->FE_Data = NULL;
	cell->cell_list = NULL;
	cell->cell_list_end = NULL;
	cell->cell_float_list = NULL;
	cell->bg_color = NULL;

	cell->border_width = 0;
	cell->border_vert_space = 0;
	cell->border_horiz_space = 0;

	cell->ele_attrmask = 0;

	cell->sel_start = -1;
	cell->sel_end = -1;

	state->current_cell = (LO_Element *)cell;
	lo_AppendToLineList(context, state, (LO_Element *)cell, 0);
}


void
lo_EndCell(MWContext *context, lo_DocState *state, LO_Element *element)
{
	LO_CellStruct *cell;
	LO_Element *eptr;
	int32 max_x, max_y;

	cell = (LO_CellStruct *)element;
	cell->cell_list = cell->next;
	eptr = cell->cell_list;
	if (eptr == NULL)
	{
		state->current_cell = NULL;
		return;
	}

	max_x = eptr->lo_any.x + eptr->lo_any.x_offset +
		eptr->lo_any.width;
	max_y = eptr->lo_any.y + eptr->lo_any.y_offset +
		eptr->lo_any.height;
	while (eptr->lo_any.next != NULL)
	{
		int32 val;

		eptr = eptr->lo_any.next;

		val = eptr->lo_any.x + eptr->lo_any.x_offset +
			eptr->lo_any.width;
		if (val > max_x)
		{
			max_x = val;
		}

		val = eptr->lo_any.y + eptr->lo_any.y_offset +
			eptr->lo_any.height;
		if (val > max_y)
		{
			max_y = val;
		}
	}
	cell->cell_list_end = eptr;
	cell->next = NULL;
	cell->cell_list->lo_any.prev = NULL;

	state->current_cell = NULL;
	state->x = cell->x;
	state->y = cell->y;
	cell->width = max_x - cell->x + 1;
	cell->height = max_y - cell->y + 1;
}

#ifdef TEST_16BIT
#undef XP_WIN16
#endif /* TEST_16BIT */

#ifdef PROFILE
#pragma profile off
#endif
