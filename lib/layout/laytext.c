/* -*- Mode: C; tab-width: 4; tabs-indent-mode: t -*- */

#include "xp.h"
#include "pa_tags.h"
#include "layout.h"
#include "libi18n.h"
#include "edt.h"

#ifdef XP_MAC
	#ifdef XP_TRACE
		#undef XP_TRACE
	#endif
	#define XP_TRACE(X)
#else
#ifndef XP_TRACE
	#define XP_TRACE(X) fprintf X
#endif
#endif /* XP_MAC */

#ifdef TEST_16BIT
#define XP_WIN16
#endif /* TEST_16BIT */

#ifdef XP_WIN16
#define SIZE_LIMIT              32000
#endif /* XP_WIN16 */

#define TEXT_CHUNK_LIMIT	500


#define DEF_TAB_WIDTH		8
#define NON_BREAKING_SPACE	160

#ifdef PROFILE
#pragma profile on
#endif

static int32 lo_correct_text_element_width(LO_TextInfo *);


void
lo_SetDefaultFontAttr(lo_DocState *state, LO_TextAttr *tptr,
	MWContext *context)
{
	tptr->size = DEFAULT_BASE_FONT_SIZE;
	tptr->fontmask = 0;
	tptr->no_background = TRUE;
	tptr->attrmask = 0;
	tptr->fg.red =   STATE_DEFAULT_FG_RED(state);
	tptr->fg.green = STATE_DEFAULT_FG_GREEN(state);
	tptr->fg.blue =  STATE_DEFAULT_FG_BLUE(state);
	tptr->bg.red =   STATE_DEFAULT_BG_RED(state);
	tptr->bg.green = STATE_DEFAULT_BG_GREEN(state);
	tptr->bg.blue =  STATE_DEFAULT_BG_BLUE(state);
	tptr->charset = INTL_DefaultTextAttributeCharSetID(context);
	tptr->font_face = NULL;
	tptr->FE_Data = NULL;
}


/*************************************
 * Function: lo_DefaultFont
 *
 * Description: This function sets up the text attribute
 *      structure for the default text drawing font.
 *      This is the font that sits at the bottom of the font
 *	stack, and can never be popped off.
 *
 * Params: Document state
 *
 * Returns: A pointer to a lo_FontStack structure.
 *	Returns a NULL on error (such as out of memory);
 *************************************/
lo_FontStack *
lo_DefaultFont(lo_DocState *state, MWContext *context)
{
	LO_TextAttr tmp_attr;
	lo_FontStack *fptr;
	LO_TextAttr *tptr;

	/*
	 * Fill in default font information.
	 */
	lo_SetDefaultFontAttr(state, &tmp_attr, context);
	tptr = lo_FetchTextAttr(state, &tmp_attr);

	fptr = XP_NEW(lo_FontStack);
	if (fptr == NULL)
	{
		return(NULL);
	}
	fptr->tag_type = P_UNKNOWN;
	fptr->text_attr = tptr;
	fptr->next = NULL;

	return(fptr);
}


void
lo_PushAlignment(lo_DocState *state, PA_Tag *tag, int32 alignment)
{
	lo_AlignStack *aptr;

	aptr = XP_NEW(lo_AlignStack);
	if (aptr == NULL)
	{
		state->top_state->out_of_memory = TRUE;
		return;
	}

	aptr->type = tag->type;
	aptr->alignment = alignment;

	aptr->next = state->align_stack;
	state->align_stack = aptr;
}


lo_AlignStack *
lo_PopAlignment(lo_DocState *state)
{
	lo_AlignStack *aptr;

	aptr = state->align_stack;
	if (aptr != NULL)
	{
		state->align_stack = aptr->next;
		aptr->next = NULL;
	}
	return(aptr);
}


/*************************************
 * Function: lo_FreshText
 *
 * Description: This function clears out the information from the
 *	document state that refers to text in the midst of being
 *	laid out.  It is called to clear the layout state before
 *	Laying out new text potentially in a new font.
 *
 * Params: Document state structure.
 *
 * Returns: Nothing
 *************************************/
void
lo_FreshText(lo_DocState *state)
{
	state->text_info.max_width = 0;
	state->text_info.ascent = 0;
	state->text_info.descent = 0;
	state->text_info.lbearing = 0;
	state->text_info.rbearing = 0;
	state->line_buf_len = 0;
	state->break_pos = -1;
	state->break_width = 0;
	state->last_char_CR = FALSE;
#ifdef EDITOR
	if( !state->edit_force_offset )
	{
		state->edit_current_offset = 0;
	}
#endif
}


/*************************************
 * Function: lo_new_text_element
 *
 * Description: Create a new text element structure based
 *	on the current text information stored in the document
 *	state.
 *
 * Params: Document state
 *
 * Returns: A pointer to a LO_TextStruct structure.
 *	Returns a NULL on error (such as out of memory);
 *************************************/
static LO_TextStruct *
lo_new_text_element(MWContext *context, lo_DocState *state, ED_Element *edit_element,
		intn edit_offset )
{
	LO_TextStruct *text_ele = NULL;
#ifdef DEBUG
	assert (state);
#endif

	if (state == NULL)
	{
		return(NULL);
	}

	text_ele = (LO_TextStruct *)lo_NewElement(context, state, LO_TEXT, 
			edit_element, edit_offset);
	if (text_ele == NULL)
	{
#ifdef DEBUG
		assert (state->top_state->out_of_memory);
#endif
		return(NULL);
	}

	text_ele->type = LO_TEXT;
	text_ele->ele_id = NEXT_ELEMENT;
	text_ele->x = state->x;
	text_ele->x_offset = 0;
	text_ele->y = state->y;
	text_ele->y_offset = 0;
	text_ele->width = 0;
	text_ele->height = 0;
	text_ele->next = NULL;
	text_ele->prev = NULL;

	if (state->line_buf_len > 0)
	{
		text_ele->text = PA_ALLOC((state->line_buf_len + 1) *
			sizeof(char));
		if (text_ele->text != NULL)
		{
			char *text_buf;
			char *line;

			PA_LOCK(line, char *, state->line_buf);
			PA_LOCK(text_buf, char *, text_ele->text);
			XP_BCOPY(line, text_buf, state->line_buf_len);
			text_buf[state->line_buf_len] = '\0';
			PA_UNLOCK(text_ele->text);
			PA_UNLOCK(state->line_buf);
			text_ele->text_len = (int16)state->line_buf_len;
		}
		else
		{
			state->top_state->out_of_memory = TRUE;
			/*
			 * free text element && return; it's no different
			 * than if we had run out of memory allocating
			 * the text element
			 */
			lo_FreeElement(context, (LO_Element *)text_ele, FALSE);
			return(NULL);
		}
	}
	else
	{
		text_ele->text = NULL;
		text_ele->text_len = 0;
	}

	text_ele->anchor_href = state->current_anchor;

	if (state->font_stack == NULL)
	{
		text_ele->text_attr = NULL;
	}
	else
	{
		text_ele->text_attr = state->font_stack->text_attr;
	}

	text_ele->ele_attrmask = 0;
	if (state->breakable != FALSE)
	{
		text_ele->ele_attrmask |= LO_ELE_BREAKABLE;
	}

	text_ele->sel_start = -1;
	text_ele->sel_end = -1;

	return(text_ele);
}


/*
 * A bogus an probably too expensive function to fill in the
 * textinfo for whatever font is now on the font stack.
 */
static void
lo_fillin_text_info(MWContext *context, lo_DocState *state)
{
	LO_TextStruct tmp_text;
	PA_Block buff;
	char *str;

	memset (&tmp_text, 0, sizeof (tmp_text));
	buff = PA_ALLOC(1);
	if (buff == NULL)
	{
		return;
	}
	PA_LOCK(str, char *, buff);
	str[0] = ' ';
	PA_UNLOCK(buff);
	tmp_text.text = buff;
	tmp_text.text_len = 1;
	tmp_text.text_attr = state->font_stack->text_attr;
	FE_GetTextInfo(context, &tmp_text, &(state->text_info));
	PA_FREE(buff);
}


/*************************************
 * Function: lo_NewLinefeed
 *
 * Description: Create a new linefeed element structure based
 *	on the current information stored in the document
 *	state.
 *
 * Params: Document state
 *
 * Returns: A pointer to a LO_LinefeedStruct structure.
 *	Returns a NULL on error (such as out of memory);
 *************************************/
LO_LinefeedStruct *
lo_NewLinefeed(lo_DocState *state, MWContext * context)
{
	LO_LinefeedStruct *linefeed = NULL;

	if (state == NULL)
	{
		return(NULL);
	}

	linefeed = (LO_LinefeedStruct *)lo_NewElement(context, state, LO_LINEFEED, NULL, 0);
#ifdef DEBUG
	assert (state);
#endif
	if (linefeed == NULL)
	{
#ifdef DEBUG
		assert (state->top_state->out_of_memory);
#endif
		state->top_state->out_of_memory = TRUE;
		return(NULL);
	}

	linefeed->type = LO_LINEFEED;
	linefeed->ele_id = NEXT_ELEMENT;
	linefeed->x = state->x;
	linefeed->x_offset = 0;
	linefeed->y = state->y;
	linefeed->y_offset = 0;
	linefeed->width = state->right_margin - state->x;
	if (linefeed->width < 0)
	{
		linefeed->width = 0;
	}
	linefeed->height = state->line_height;
	/*
	 * if this linefeed has a zero height, make it the height
	 * of the current font.
	 */
	if (linefeed->height == 0)
	{
		linefeed->height = state->text_info.ascent +
			state->text_info.descent;
		if ((linefeed->height <= 0)&&(state->font_stack != NULL)&&
			(state->font_stack->text_attr != NULL))
		{
			lo_fillin_text_info(context, state);
			linefeed->height = state->text_info.ascent +
				state->text_info.descent;
		}
		/*
		 * This should never happen, but we have it
		 * covered just in case it does :-)
		 */
		if (linefeed->height <= 0)
		{
			linefeed->height = state->default_line_height;
		}
	}
	linefeed->line_height = linefeed->height;

	linefeed->FE_Data = NULL;
	linefeed->anchor_href = state->current_anchor;

	if (state->font_stack == NULL)
	{
		LO_TextAttr tmp_attr;
		LO_TextAttr *tptr;

		/*
		 * Fill in default font information.
		 */
		lo_SetDefaultFontAttr(state, &tmp_attr, context);
		tptr = lo_FetchTextAttr(state, &tmp_attr);
		linefeed->text_attr = tptr;
	}
	else
	{
		linefeed->text_attr = state->font_stack->text_attr;
	}

	linefeed->baseline = state->baseline;

	linefeed->ele_attrmask = 0;

	linefeed->sel_start = -1;
	linefeed->sel_end = -1;

	linefeed->next = NULL;
	linefeed->prev = NULL;
	linefeed->break_type = LO_LINEFEED_BREAK_SOFT;

	return(linefeed);
}

#if 0
   static void
lo_MarkHardBreak(MWContext *context, lo_DocState *state)
{
	LO_Element *pSearch, *pLinefeed;
	/*
	 * LTNOTE: some editor voodo here.  We just closed a paragraph or it was 
	 *  already closed.  Search backwards and mark the first linefeed as the
	 *  one that closed the 'end_paragraph'
	 *  linefeed.
     * (By "Paragraph" we mean any paragraph-like unit, including pargraphs,
     * headings, hard breaks, and list items.
	*/
	pSearch = state->end_last_line;
	pLinefeed = NULL;
	while( pSearch && pSearch->type == LO_LINEFEED )
	{
		pLinefeed = pSearch;
		pSearch = pSearch->lo_any.prev;
	}

	/* 
	 * if we found it.  Mark it as the end of a paragraph.
	*/
	if( pLinefeed )
	{
		pLinefeed->lo_linefeed.break_type = LO_LINEFEED_BREAK_PARAGRAPH;
	}
}
#endif

/*************************************
 * Function: lo_HardLineBreak
 *
 * Description: This function forces a linebreak at this time.
 *	Forcing the current text insetion point down one line
 *	and back to the left margin.
 *	It also maintains the linefeed state:
 *		0 - middle of a line.
 *		1 - at left margin below a line of text.
 *		2 - at left margin below a blank line.
 *
 * Params: Window context, Document state and a boolean
 *	That describes whether this is a breaking linefeed or not.
 *	(Breaking linefeed is one inserted just to break a formatted
 *	 line to the current document width.)
 *
 * Returns: Nothing
 *************************************/
void
lo_HardLineBreak(MWContext *context, lo_DocState *state, Bool breaking)
{
	int32 line_width;
	Bool scroll_at_bottom;

	scroll_at_bottom = FALSE;
	/*
	 * If this is an auto-scrolling doc, we will be scrolling
	 * later if we are at the bottom now.
	 */
	if ((state->is_a_subdoc == SUBDOC_NOT)&&
		(state->display_blocked == FALSE)&&
		(state->top_state->auto_scroll > 0))
	{
		int32 doc_x, doc_y;

		FE_GetDocPosition(context, FE_VIEW, &doc_x, &doc_y);
		if ((doc_y + state->win_height + state->win_bottom) >= state->y)
		{
			scroll_at_bottom = TRUE;
		}
	}

	/*
	 * This line is done, flush it into the line table and
	 * display it to the front end.
	 */
	lo_FlushLineList(context, state, breaking);
	if (state->top_state->out_of_memory != FALSE)
	{
		return;
	}

	/*
	 * if this linefeed has a zero height, make it the height
	 * of the current font.
	 */
	if (state->line_height == 0)
	{
		state->line_height = state->text_info.ascent +
			state->text_info.descent;
		if ((state->line_height <= 0)&&(state->font_stack != NULL)&&
			(state->font_stack->text_attr != NULL))
		{
			lo_fillin_text_info(context, state);
			state->line_height = state->text_info.ascent +
				state->text_info.descent;
		}
		/*
		 * This should never happen, but we have it
		 * covered just in case it does :-)
		 */
		if (state->line_height <= 0)
		{
			state->line_height = state->default_line_height;
		}
	}

	if (state->end_last_line != NULL)
	{
		line_width = state->end_last_line->lo_any.x + state->win_right;
	}
	else
	{
		line_width = state->x + state->win_right;
	}

	if (line_width > state->max_width)
	{
		state->max_width = line_width;
	}

	state->x = state->left_margin;
	state->y = state->y + state->line_height;
	state->width = 0;
	state->at_begin_line = TRUE;
	state->trailing_space = FALSE;
	state->line_height = 0;
	state->break_holder = state->x;

	state->linefeed_state++;
	if (state->linefeed_state > 2)
	{
		state->linefeed_state = 2;
	}

	if (state->is_a_subdoc == SUBDOC_NOT)
	{
		int32 percent;

		if (state->top_state->total_bytes < 1)
		{
			percent = -1;
		}
		else
		{
			percent = (100 * state->top_state->layout_bytes) /
				state->top_state->total_bytes;
			if (percent > 100)
			{
				percent = 100;
			}
		}
		if ((percent == 100)||(percent < 0)||
			(percent > (state->top_state->layout_percent + 1)))
		{
			if(!state->top_state->is_binary)
				FE_SetProgressBarPercent(context, percent);
			state->top_state->layout_percent = (intn)percent;
		}
	}

	/*
	 * Tell the front end how big the document is right now.
	 * Only do this for the top level document.
	 */
	if ((state->is_a_subdoc == SUBDOC_NOT)
        &&(state->display_blocked == FALSE)
#ifdef EDITOR
		&&(!state->edit_relayout_display_blocked)
#endif
        )
	{
		FE_SetDocDimension(context, FE_VIEW,
			state->max_width, state->y);
	}

	if ((state->display_blocked != FALSE)&&
#ifdef EDITOR
		(!state->edit_relayout_display_blocked)&&
#endif
	   (state->display_blocking_element_y > 0)&&
	   (state->y > (state->display_blocking_element_y + state->win_height)))
	{
		int32 y;

		state->display_blocked = FALSE;
		y = state->display_blocking_element_y;
		state->display_blocking_element_y = 0;
		FE_SetDocDimension(context, FE_VIEW,
			state->max_width, state->y);
		FE_SetDocPosition(context, FE_VIEW, 0, y);
		lo_RefreshDocumentArea(context, state, 0, y,
			state->win_width, state->win_height);
	}

	/*
	 * Reset the left and right margins
	 */
	lo_FindLineMargins(context, state);
	state->x = state->left_margin;

	if ((state->is_a_subdoc == SUBDOC_NOT)&&
	    (state->top_state->auto_scroll > 0)&&
	    ((state->line_num - 1) > state->top_state->auto_scroll))
	{
		Bool redraw;
		int32 old_y, dy, new_y;
		int32 doc_x, doc_y;

		old_y = state->y;
		FE_GetDocPosition(context, FE_VIEW, &doc_x, &doc_y);
		lo_ClipLines(context, state, 1);
		/*
		 * Calculate how much of the top was clipped
		 * off, and if any of that area was in the users
		 * window we need to redraw their window.
		 */
		redraw = FALSE;
		dy = old_y - state->y;
		if (dy >= doc_y)
		{
			redraw = TRUE;
		}
		new_y = doc_y - dy;
		if (new_y < 0)
		{
			new_y = 0;
		}

		FE_SetDocPosition(context, FE_VIEW, doc_x, new_y);
		FE_SetDocDimension(context, FE_VIEW,
			state->max_width, state->y);
		if (redraw != FALSE)
		{
			FE_ClearView(context, FE_VIEW );
			lo_RefreshDocumentArea(context, state, 0, new_y,
				state->win_width, state->win_height);
		}
	}

	if ((state->is_a_subdoc == SUBDOC_NOT)&&
		(state->display_blocked == FALSE)&&
		(state->top_state->auto_scroll > 0))
	{
		int32 doc_x, doc_y;

		FE_GetDocPosition(context, FE_VIEW, &doc_x, &doc_y);
		if (((doc_y + state->win_height) < state->y)&&
			(scroll_at_bottom != FALSE))
		{
			int32 y;

			y = state->y - state->win_height + state->win_bottom;
			if (y < 0)
			{
				y = 0;
			}
			else
			{
#if defined(XP_UNIX)
				FE_ScrollDocTo(context, FE_VIEW, 0, y);
#else
				FE_SetDocPosition(context, FE_VIEW, 0, y);
#endif /* XP_UNIX */
			}
		}
	}
#if 0
    if ( ! breaking ) {
        lo_MarkHardBreak(context, state);
    }
#endif
}


/*************************************
 * Function: lo_SoftLineBreak
 *
 * Description: This function compares the current linefeed
 *	state to the desired linefeed state, and if it is
 *	not already there, it forces linefeeds until we get there.
 *	Linefeed states:
 *		0 - middle of a line.
 *		1 - at left margin below a line of text.
 *		2 - at left margin below a blank line.
 *
 * Params: Window context, Document state, a boolean that
 *	describes if this is a breaking linefeed, and the desired
 *	linefeed state.
 *
 * Returns: Nothing
 *************************************/
void
lo_SoftLineBreak(MWContext *context, lo_DocState *state, Bool breaking, intn linefeed_state)
{
	/*
	 * Softlinefeeds are partially disabled if we are placing
	 * preformatted text.
	 */
	if (state->preformatted != PRE_TEXT_NO)
	{
		if ((breaking == FALSE)&&(state->linefeed_state < 1)&&
			(state->top_state->out_of_memory == FALSE))
		{
			lo_HardLineBreak(context, state, breaking);
		}
		return;
	}

	if (linefeed_state > 2)
	{
		linefeed_state = 2;
	}

	while ((state->linefeed_state < linefeed_state)&&
		(state->top_state->out_of_memory == FALSE))
	{
		lo_HardLineBreak(context, state, breaking);
	}
}


/*************************************
 * Function: lo_InsertWordBreak
 *
 * Description: This insert a word break into the laid out
 *	element chain for the current line.  A word break is
 *	basically an empty text element structure.  All
 *	word break elements should probably be removed
 *	from the layout chain once the line list for this
 *	line is flushed.
 *
 * Params: Window context and document state.
 *
 * Returns: Nothing
 *************************************/
void
lo_InsertWordBreak(MWContext *context, lo_DocState *state)
{
	LO_TextStruct *text_ele = NULL;
	char *text_buf;

	if (state == NULL)
	{
		return;
	}

	text_ele = (LO_TextStruct *)lo_NewElement(context, state, LO_TEXT, NULL, 0);
	if (text_ele == NULL)
	{
#ifdef DEBUG
		assert (state->top_state->out_of_memory);
#endif
		return;
	}

	text_ele->type = LO_TEXT;
	text_ele->ele_id = NEXT_ELEMENT;
	text_ele->x = state->x;
	text_ele->x_offset = 0;
	text_ele->y = state->y;
	text_ele->y_offset = 0;
	text_ele->width = 0;
	text_ele->height = 0;
	text_ele->next = NULL;
	text_ele->prev = NULL;

	text_ele->text = PA_ALLOC(sizeof(char));
	if (text_ele->text != NULL)
	{
		PA_LOCK(text_buf, char *, text_ele->text);
		text_buf[0] = '\0';
		PA_UNLOCK(text_ele->text);
	}
	else
	{
		state->top_state->out_of_memory = TRUE;
	}
	text_ele->text_len = 0;
	text_ele->anchor_href = state->current_anchor;

	if (state->font_stack == NULL)
	{
		text_ele->text_attr = NULL;
	}
	else
	{
		text_ele->text_attr = state->font_stack->text_attr;
	}

	text_ele->ele_attrmask = 0;
	if (state->breakable != FALSE)
	{
		text_ele->ele_attrmask |= LO_ELE_BREAKABLE;
	}

	text_ele->sel_start = -1;
	text_ele->sel_end = -1;

	lo_AppendToLineList(context, state, (LO_Element *)text_ele, 0);

	state->old_break = text_ele;
	state->old_break_pos = 0;
	state->old_break_width = 0;
}


#ifdef NOT_USED
/*************************************
 * Function: lo_calc_baseline_and_height
 *
 * Description: We have a disconnected tail half of a line list.
 *	It may have a baseline and line height the same or equal
 *	to what it used to be.  If less, find its new value,
 *	and return the difference between the old and the new.
 *	Also set the new baseline and line height into state.
 *
 * Params: Document state and the LO_Element list pointer.
 *
 * Returns: amount that all the y_offsets should be reduced by.
 *************************************/
static int32
lo_calc_baseline_and_height(MWContext *context, lo_DocState *state,
				LO_Element *eptr)
{
	int32 baseline, old_baseline;
	int32 correction;
	int32 new_height;
	LO_TextInfo text_info;

	baseline = 0;
	old_baseline = 0;
	correction = 0;
	new_height = 0;

	state->baseline = 0;
	state->line_height = 0;

	while (eptr != NULL)
	{
		switch (eptr->type)
		{
			case LO_TEXT:
				FE_GetTextInfo(context, (LO_TextStruct *)eptr,
					&text_info);
				if (text_info.ascent > baseline)
				{
					baseline = text_info.ascent;
				}
				if ((text_info.ascent + text_info.descent) >
					new_height)
				{
					new_height = text_info.ascent +
						text_info.descent;
				}
				old_baseline = eptr->lo_text.y_offset +
					text_info.ascent;
				break;
			case LO_FORM_ELE:
				if (eptr->lo_form.baseline > baseline)
				{
					baseline = eptr->lo_form.baseline;
				}
				if (eptr->lo_any.height > new_height)
				{
					new_height = eptr->lo_any.height;
				}
				old_baseline = eptr->lo_any.y_offset +
					eptr->lo_form.baseline;
				break;
			case LO_HRULE:
			case LO_BULLET:
			case LO_IMAGE:
				if (eptr->lo_any.height > new_height)
				{
					new_height = eptr->lo_any.height;
				}
				break;
			default:
				break;
		}
		eptr = eptr->lo_any.next;
	}

	if (baseline == 0)
	{
		baseline = new_height - 1;
	}

	state->line_height = (intn) new_height;
	state->baseline = (intn) baseline;
	if (old_baseline > baseline)
	{
		correction = old_baseline - baseline;
	}
	return(correction);
}
#endif /* NOT_USED */


static int32
lo_baseline_adjust(MWContext *context, lo_DocState *state, LO_Element *ele_list,
			int32 old_baseline, int32 old_line_height)
{
	LO_Element *eptr;
	int32 baseline;
	int32 new_height;
	int32 baseline_adjust;
	LO_TextInfo text_info;

	baseline = 0;
	new_height = 0;
	eptr = ele_list;
	while (eptr != NULL)
	{
		switch (eptr->type)
		{
			case LO_TEXT:
				FE_GetTextInfo(context, (LO_TextStruct *)eptr,
					&text_info);
				if (text_info.ascent > baseline)
				{
					baseline = text_info.ascent;
				}
				if ((text_info.ascent + text_info.descent) >
					new_height)
				{
					new_height = text_info.ascent +
						text_info.descent;
				}
				break;
			case LO_FORM_ELE:
				if (eptr->lo_form.baseline > baseline)
				{
					baseline = eptr->lo_form.baseline;
				}
				if (eptr->lo_any.height > new_height)
				{
					new_height = eptr->lo_any.height;
				}
				break;
			case LO_IMAGE:
				if ((old_baseline - eptr->lo_any.y_offset) >
					baseline)
				{
					baseline = old_baseline -
						eptr->lo_any.y_offset;
				}
				if ((eptr->lo_image.height +
					(2 * eptr->lo_image.border_width) +
					(2 * eptr->lo_image.border_vert_space))
					> new_height)
				{
					new_height = eptr->lo_image.height +
					 (2 * eptr->lo_image.border_width) +
					 (2 * eptr->lo_image.border_vert_space);
				}
				break;
			case LO_HRULE:
			case LO_BULLET:
				if ((old_baseline - eptr->lo_any.y_offset) >
					baseline)
				{
					baseline = old_baseline -
						eptr->lo_any.y_offset;
				}
				if (eptr->lo_any.height > new_height)
				{
					new_height = eptr->lo_any.height;
				}
				break;
			default:
				break;
		}
		eptr = eptr->lo_any.next;
	}
	baseline_adjust = old_baseline - baseline;
	return(baseline_adjust);
}


/*************************************
 * Function: lo_BreakOldElement
 *
 * Description: This function goes back to a previous
 *	element in the element chain, that is still on the
 *	same line, and breaks the line there, putting the
 *	remaining elements (if any) on the next line.
 *
 * Params: Window context and document state.
 *
 * Returns: Nothing
 *************************************/
void
lo_BreakOldElement(MWContext *context, lo_DocState *state)
{
	char *text_buf;
	char *break_ptr;
	char *word_ptr;
	char *new_buf;
	PA_Block new_block;
	intn word_len;
	int32 save_width;
	int32 old_baseline;
	int32 old_line_height;
	int32 adjust;
	LO_TextStruct *text_data;
	LO_TextStruct *new_text_data;
	LO_Element *tptr;
	int16 charset;
	int multi_byte;
#ifdef LOCAL_DEBUG
XP_TRACE(("lo_BreakOldElement, flush text.\n"));
#endif /* LOCAL_DEBUG */

	if (state == NULL)
	{
		return;
	}

	charset = state->font_stack->text_attr->charset;
	multi_byte = (INTL_CharSetType(charset) != SINGLEBYTE);

	/*
	 * Move to the element we will break
	 */
	text_data = state->old_break;

	/*
	 * If there is no text there to break
	 * it is an error.
	 */
	if ((text_data == NULL)||(text_data->text == NULL))
	{
		return;
	}

	/*
	 * Later operations will trash the width field.
	 * So save it now to restore later.
	 */
	save_width = state->width;

	PA_LOCK(text_buf, char *, text_data->text);
	/*
	 * If this buffer is just an empty string, then we are
	 * breaking at a previously inserted word break element.
	 * Knowing that, the breaking is easier, so it is special 
	 * cased here.
	 */
	if (*text_buf == '\0')
	{
		/*
		 * Back the state up to this element's
		 * location, break off the rest of the elements
		 * and save them for later.
		 * Flush this line, and insert a linebreak.
		 */
		state->x = text_data->x;
		state->y = text_data->y;
		tptr = text_data->next;
		text_data->next = NULL;
		PA_UNLOCK(text_data->text);
		state->width = text_data->width;
		state->x += state->width;
		lo_HardLineBreak(context, state, TRUE);

		/*
		 * The remaining elements go on the next line.
		 */
		state->line_list = tptr;
		if (tptr != NULL)
		{
			tptr->lo_any.prev = NULL;
		}

		/*
		 * If there are no elements to place on the next
		 * line, and we have new text buffered,
		 * remove any spaces at the start of that new
		 * text since new lines are not allowed to begin
		 * with whitespace.
		 */
		if ((tptr == NULL)&&(state->line_buf_len != 0))
		{
			char *cptr;
			int32 wlen;

			PA_LOCK(text_buf, char *, state->line_buf);
			cptr = text_buf;
			wlen = 0;
			while ((XP_IS_SPACE(*cptr))&&(*cptr != '\0'))
			{
				cptr++;
				wlen++;
			}
			if (wlen)
			{
				LO_TextStruct tmp_text;

				memset (&tmp_text, 0, sizeof (tmp_text));

				/*
				 * We removed space, move the string up
				 * and recalculate its current width.
				 */
				XP_BCOPY(cptr, text_buf,
					(state->line_buf_len - wlen + 1));
				state->line_buf_len -= (intn) wlen;
				PA_UNLOCK(state->line_buf);
				tmp_text.text = state->line_buf;
				tmp_text.text_len = (int16)state->line_buf_len;
				tmp_text.text_attr =
					state->font_stack->text_attr;
				FE_GetTextInfo(context, &tmp_text,
					&(state->text_info));

				/*
				 * Override the saved width since we did want
				 * to change the real width in this case.
				 */
				save_width = lo_correct_text_element_width(
					&(state->text_info));
			}
			else
			{
				PA_UNLOCK(state->line_buf);
			}
		}
	}
	/*
	 * We are breaking somewhere in the middle of an old
	 * text element, this will mean splitting it into 2 text
	 * elements, and putting a line break between them.
	 */
	else
	{
		LO_TextInfo text_info;
		int32 base_change;

		/*
		 * Locate our break location, and a pointer to
		 * the remaining word (with its preceeding space removed).
		 */
		break_ptr = (char *)(text_buf + state->old_break_pos);
		/*
		 * On multibyte, we often break on character which is 
		 * not space.
		 */ 
		if (multi_byte == FALSE || XP_IS_SPACE(*break_ptr))
		{
			*break_ptr = '\0';
			word_ptr = (char *)(break_ptr + 1);
		}
		else
		{
			word_ptr = (char *)break_ptr;
		}

		/*
		 * Copy the remaining word into its own block.
		 */
		word_len = XP_STRLEN(word_ptr);
		new_block = PA_ALLOC((word_len + 1));
		if (new_block == NULL)
		{
			state->top_state->out_of_memory = TRUE;
			return;
		}
		PA_LOCK(new_buf, char *, new_block);
		XP_BCOPY(word_ptr, new_buf, (word_len + 1));
		new_buf[word_len] = '\0';

		/*
		 * Back the state up to this element's
		 * location, break off the rest of the elements
		 * and save them for later.
		 * Flush this line, and insert a linebreak.
		 */
		state->x = text_data->x;
		state->y = text_data->y;
		tptr = text_data->next;
		text_data->next = NULL;
		if (word_ptr != break_ptr)
			text_data->text_len = text_data->text_len - word_len - 1;
		else
			text_data->text_len = text_data->text_len - word_len;
		FE_GetTextInfo(context, text_data, &text_info);
		state->width = lo_correct_text_element_width(&text_info);
		PA_UNLOCK(text_data->text);
		state->x += state->width;

		/*
		 * Make the split element know its new width.
		 */
		text_data->width = state->width;

		/*
		 * If the element that caused this break has a different
		 * baseline than the element we are breaking, we need to
		 * preserve that difference after the break.
		 */
		base_change = state->baseline -
			(text_data->y_offset + text_info.ascent);

		old_baseline = state->baseline;
		old_line_height = state->line_height;

		/*
		 * Reset element_id so they are still sequencial.
		 */
		state->top_state->element_id = text_data->ele_id + 1;

		/*
		 * If we are breaking an anchor, we need to make sure the
		 * linefeed gets its anchor href set properly.
		 */
		if (text_data->anchor_href != NULL)
		{
			LO_AnchorData *tmp_anchor;

			tmp_anchor = state->current_anchor;
			state->current_anchor = text_data->anchor_href;
			lo_HardLineBreak(context, state, TRUE);
			state->current_anchor = tmp_anchor;
		}
		else
		{
			lo_HardLineBreak(context, state, TRUE);
		}

		adjust = lo_baseline_adjust(context, state, tptr,
			old_baseline, old_line_height);
		state->baseline = old_baseline - adjust;
		state->line_height = (intn) old_line_height - adjust;

		/*
		 * If there was really no remaining word, free the
		 * unneeded buffer.
		 */
		if (word_len == 0)
		{
			LO_Element *eptr;

			PA_UNLOCK(new_block);
			PA_FREE(new_block);
			state->line_list = tptr;
			if (tptr != NULL)
			{
				tptr->lo_any.prev = NULL;
			}
			state->width = 0;

			eptr = tptr;
			while (eptr != NULL)
			{
				eptr->lo_any.ele_id = NEXT_ELEMENT;
				eptr->lo_any.y_offset -= adjust;
				eptr = eptr->lo_any.next;
			}
		}
		/*
		 * Else create a new text element for the remaining word.
		 * and stick it in the begining of the next line of
		 * text elements.
		 */
		else
		{
			LO_Element *eptr;
			int32 baseline_inc;
			LO_TextInfo text_info;
			baseline_inc = -1 * adjust;
			new_text_data = lo_new_text_element(context, state, 
					text_data->edit_element,
                    text_data->edit_offset+text_data->text_len+1 );
			if (new_text_data == NULL)
			{
				return;
			}
			if (new_text_data->text != NULL)
			{
				PA_FREE(new_text_data->text);
				new_text_data->text = NULL;
				new_text_data->text_len = 0;
			}
			new_text_data->anchor_href = text_data->anchor_href;
			new_text_data->text_attr = text_data->text_attr;
			new_text_data->x = state->x;
			new_text_data->y = state->y;
			
#ifdef LOCAL_DEBUG
XP_TRACE(("lo_BreakOldElement, left over word (%s)\n", new_buf));
#endif /* LOCAL_DEBUG */

			PA_UNLOCK(new_block);
			new_text_data->text = new_block;
			new_text_data->text_len = word_len;
			FE_GetTextInfo(context, new_text_data, &text_info);
			new_text_data->width =
				lo_correct_text_element_width(&text_info);

			/*
			 * Some fonts (particulatly italic ones with curly
			 * tails on letters like 'f') have a left bearing
			 * that extends back into the previous character.
			 * Since in this case the previous character is
			 * probably not in the same font, we move forward
			 * to avoid overlap.
			 */
			if (text_info.lbearing < 0)
			{
				new_text_data->x_offset =
					text_info.lbearing * -1;
			}

			/*
			 * The baseline of the text element just inserted in
			 * the line may be less than or greater than the
			 * baseline of the rest of the line due to font
			 * changes.  If the baseline is less, this is easy,
			 * we just increase y_offest to move the text down
			 * so the baselines line up.  For greater baselines,
			 * we can't move the text up to line up the baselines
			 * because we will overlay the previous line, so we
			 * have to move all rest of the elements in this line
			 * down.
			 *
			 * If the baseline is zero, we are the first element
			 * on the line, and we get to set the baseline.
			 */
			if (state->baseline == 0)
			{
				state->baseline = text_info.ascent;
			}
			else if (text_info.ascent < state->baseline)
			{
				new_text_data->y_offset = state->baseline -
						text_info.ascent;
			}
			else
			{
				baseline_inc = baseline_inc +
					(text_info.ascent - state->baseline);
				state->baseline =
					text_info.ascent;
			}

			/*
			 * Now that we have broken, and added the new
			 * element, we need to move it down to restore the
			 * baseline difference that previously existed.
			 */
			new_text_data->y_offset -= base_change;

			/*
			 * Calculate the height of this new
			 * text element.
			 */
			new_text_data->height = text_info.ascent +
				text_info.descent;
			state->x += new_text_data->width;

			/*
			 * Stick this new text element at the beginning
			 * of the remaining line elements
			 */
			state->line_list = (LO_Element *)new_text_data;
			new_text_data->prev = NULL;
			new_text_data->next = tptr;
			if (tptr != NULL)
			{
				tptr->lo_any.prev = (LO_Element *)new_text_data;
			}
			eptr = tptr;

			while (eptr != NULL)
			{
				eptr->lo_any.ele_id = NEXT_ELEMENT;
				eptr->lo_any.y_offset += baseline_inc;
				eptr = eptr->lo_any.next;
			}

			if ((new_text_data->y_offset + new_text_data->height) > 
				state->line_height)
			{
				state->line_height = (intn) new_text_data->y_offset +
					new_text_data->height;
			}

			state->at_begin_line = FALSE;
		}
	}

	/*
	 * If we are at the beginning of a line, and there is
	 * remaining text to place here, remove leading space
	 * which is not allowed at the start of lines.
	 * ERIC, make a test case for this, I suspect right now
	 * this code is never being executed and may contain an error.
	 */
	if ((state->at_begin_line != FALSE)&&(tptr != NULL)&&
		(tptr->type == LO_TEXT))
	{
		char *cptr;
		int32 wlen;
		LO_TextStruct *tmp_text;
		LO_TextInfo text_info;

		tmp_text = (LO_TextStruct *)tptr;

		PA_LOCK(text_buf, char *, tmp_text->text);
		cptr = text_buf;
		wlen = 0;
		while ((XP_IS_SPACE(*cptr))&&(*cptr != '\0'))
		{
			cptr++;
			wlen++;
		}
		if (wlen)
		{
			XP_BCOPY(cptr, text_buf,
				(tmp_text->text_len - wlen + 1));
			tmp_text->text_len -= (intn) wlen;

			PA_UNLOCK(tmp_text->text);
			FE_GetTextInfo(context, tmp_text, &text_info);
			tmp_text->width = lo_correct_text_element_width(
				&text_info);
		}
		else
		{
			PA_UNLOCK(tmp_text->text);
		}
	}

	/*
	 * Upgrade forward the x and y text positions in the document
	 * state.
	 */
	while (tptr != NULL)
	{
		tptr->lo_any.x = state->x;
		tptr->lo_any.y = state->y;
		state->x = state->x + tptr->lo_any.width;
		tptr = tptr->lo_any.next;
	}

	state->at_begin_line = FALSE;
	state->width = save_width;
}


/*************************************
 * Function: lo_correct_text_element_width
 *
 * Description: Calculate the correct width of this text element
 *	if it is a complete element surrounded by elements of potentially
 *	different fonts, so we have to take care not to truncate
 *	any slanted characters at either end of the element.
 *
 * Params: LO_TextInfo structure for this text element's text string.
 *
 * Returns: The width this element would have if it stood alone.
 *************************************/
static int32
lo_correct_text_element_width(LO_TextInfo *text_info)
{
	int32 x_offset;
	int32 width;

	width = text_info->max_width;
	x_offset = 0;

	/*
	 * For text that leans into the previous character.
	 */
	if (text_info->lbearing < 0)
	{
		x_offset = text_info->lbearing * -1;
		width += x_offset;
	}

	/*
	 * For text that leans right into the following characters.
	 */
	if (text_info->rbearing > text_info->max_width)
	{
		width += (text_info->rbearing - text_info->max_width);
	}

	return(width);
}


/*************************************
 * Function: lo_textinfo_copy
 *
 * Description: Simple structure copy, I'm certain there is a faster
 *	and more efficient way to do this.
 *
 * Params: src and dest LO_TextInfo pointers.
 *
 * Returns: Nothing
 *************************************/
#if 0
static void
lo_textinfo_copy(LO_TextInfo *src_tinfo, LO_TextInfo *dest_tinfo)
{
	dest_tinfo->max_width = src_tinfo->max_width;
	dest_tinfo->ascent = src_tinfo->ascent;
	dest_tinfo->descent = src_tinfo->descent;
	dest_tinfo->lbearing = src_tinfo->lbearing;
	dest_tinfo->rbearing = src_tinfo->rbearing;
}
#endif


int32
lo_characters_in_line(lo_DocState *state)
{
	int32 cnt;
	LO_Element *eptr;

	cnt = 0;
	eptr = state->line_list;
	while (eptr != NULL)
	{
		if (eptr->type == LO_TEXT)
		{
			cnt += eptr->lo_text.text_len;
		}
		eptr = eptr->lo_any.next;
	}
	return(cnt);
}


void
lo_PreformatedText(MWContext *context, lo_DocState *state, char *text)
{
	char *tptr;
	char *w_start;
	char *w_end;
	char *text_buf;
	char tchar1;
	Bool have_CR;
	Bool line_break;
	Bool white_space;
	LO_TextStruct text_data;
	char *tmp_buf;
	PA_Block tmp_block;
	int32 tab_count, ignore_cnt, line_length;
	int16 charset;
	int multi_byte;

	/*
	 * Initialize the structures to 0 (mark)
	 */
	memset (&text_data, 0, sizeof (LO_TextStruct));

	/*
	 * Error conditions
	 */
	if ((state == NULL)||(state->cur_ele_type != LO_TEXT)||(text == NULL))
	{
		return;
	}

	charset = state->font_stack->text_attr->charset;
	multi_byte = (INTL_CharSetType(charset) != SINGLEBYTE);

	/*
	 * Move through this text fragment, expand tabs, honor LF/CR.
	 */
	have_CR = state->last_char_CR;
	tptr = text;
	while ((*tptr != '\0')&&(state->top_state->out_of_memory == FALSE))
	{
		Bool has_nbsp;
		Bool in_word;
		Bool wrap_break;

		/*
		 * white_space is a tag to tell us if the current word
		 * ends in whitespace.
		 */
		white_space = FALSE;
		line_break = FALSE;
		has_nbsp = FALSE;

		/*
		 * Find the end of the line, counting tabs.
		 */
		tab_count = 0;
		ignore_cnt = 0;
		line_length = 0;
		w_start = tptr;

		/*
		 * If the last character processed was a CR, and the next
		 * char is a LF, ignore it.  Otherwise we know we
		 * can break the line on the first CR or LF found.
		 */
		if ((have_CR != FALSE)&&(*tptr == LF))
		{
			ignore_cnt++;
			tptr++;
		}
		have_CR = FALSE;

		in_word = FALSE;
		wrap_break = FALSE;
#ifdef XP_WIN16
		while ((*tptr != CR)&&(*tptr != LF)&&(*tptr != '\0')&&
#ifdef TEXT_CHUNK_LIMIT
		    ((line_length + (tab_count * DEF_TAB_WIDTH)) < TEXT_CHUNK_LIMIT)&&
#endif /* TEXT_CHUNK_LIMIT */
		    (((state->line_buf_len + line_length + (tab_count * DEF_TAB_WIDTH))) < SIZE_LIMIT))
#else
		while ((*tptr != CR)&&(*tptr != LF)&&(*tptr != '\0')
#ifdef TEXT_CHUNK_LIMIT
		    &&((line_length + (tab_count * DEF_TAB_WIDTH)) < TEXT_CHUNK_LIMIT)
#endif /* TEXT_CHUNK_LIMIT */
			)
#endif /* XP_WIN16 */
		{
			/*
			 * In the special wrapping preformatted text
			 * we need to chunk by word instead of by
			 * line.
			 */
			if (state->preformatted == PRE_TEXT_WRAP)
			{
				if ((in_word == FALSE)&&(!XP_IS_SPACE(*tptr)))
				{
					in_word = TRUE;
				}
				else if ((in_word != FALSE)&&
					(XP_IS_SPACE(*tptr)))
				{
					wrap_break = TRUE;
					break;
				}
			}

			if ((*tptr == FF)||(*tptr == VTAB))
			{
				/*
				 * Ignore the form feeds
				 * thrown in by some platforms.
				 * Ignore vertical tabs since we don't know
				 * what else to do with them.
				 */
				ignore_cnt++;
			}
			else if (*tptr == TAB)
			{
				tab_count++;
			}
			else if (!multi_byte && (unsigned char)*tptr == NON_BREAKING_SPACE)
			{
				/* *tptr = ' '; Replace this later */
				has_nbsp = TRUE;
				line_length++;
			}
			else
			{
				line_length++;
			}
			tptr++;
		}
		line_length = line_length + (DEF_TAB_WIDTH * tab_count);

#ifdef TEXT_CHUNK_LIMIT
		if ((state->line_buf_len + line_length) > TEXT_CHUNK_LIMIT)
		{
			lo_FlushLineBuffer(context, state);
			if (state->cur_ele_type != LO_TEXT)
			{
				lo_FreshText(state);
				state->cur_ele_type = LO_TEXT;
			}
		}
#endif /* TEXT_CHUNK_LIMIT */

#ifdef XP_WIN16
		if ((state->line_buf_len + line_length) >= SIZE_LIMIT)
		{
			line_break = TRUE;
		}
#endif /* XP_WIN16 */

		/*
		 * Terminate the word, saving the char we replaced
		 * with the terminator so it can be restored later.
		 */
		w_end = tptr;
		tchar1 = *w_end;
		*w_end = '\0';

		tmp_block = PA_ALLOC(line_length + 1);
		if (tmp_block == NULL)
		{
			*w_end = tchar1;
			state->top_state->out_of_memory = TRUE;
			break;
		}
		PA_LOCK(tmp_buf, char *, tmp_block);

		if ((tab_count)||(ignore_cnt))
		{
			char *cptr;
			char *text_ptr;
			int32 cnt;

			text_ptr = tmp_buf;
			cptr = w_start;
			cnt = lo_characters_in_line(state);
			cnt += state->line_buf_len;
			while (*cptr != '\0')
			{
				if ((*cptr == LF)||(*cptr == FF)||
					(*cptr == VTAB))
				{
					/*
					 * Ignore any linefeeds that must have
					 * been after CR, and form feeds.
					 * Ignore vertical tabs since we
					 * don't know what else to do with them.
					 */
					cptr++;
				}
				else if (*cptr == TAB)
				{
					int32 i, tab_pos;

					tab_pos = ((cnt / DEF_TAB_WIDTH) +
						1) * DEF_TAB_WIDTH;
					for (i=0; i<(tab_pos - cnt); i++)
					{
						*text_ptr++ = ' ';
					}
					cnt = tab_pos;
					cptr++;
				}
				else
				{
					*text_ptr++ = *cptr++;
					cnt++;
				}
			}
			*text_ptr = *cptr;
		}
		else
		{
			XP_BCOPY(w_start, tmp_buf, line_length + 1);
		}

		/*
		 * Now we catch those nasty non-breaking space special
		 * characters and make them spaces.
		 */
		if (has_nbsp != FALSE)
		{
			char *tmp_ptr;

			tmp_ptr = tmp_buf;
			while (*tmp_ptr != '\0')
			{
#ifndef XP_MAC
/*	Don't change NON_BREAKING_SPACE to SPACE in Mac See Bug # 6640 */

				if ((unsigned char)*tmp_ptr ==
					NON_BREAKING_SPACE)
				{
					*tmp_ptr = ' ';
				}
#endif
				tmp_ptr++;
			}
		}

		/* don't need this any more since we're converting
		 * elsewhere -- erik
		tmp_buf = FE_TranslateISOText(context, charset, tmp_buf);
		 */
		line_length = XP_STRLEN(tmp_buf);

		if ((line_length > 0)&&(XP_IS_SPACE(tmp_buf[line_length - 1])))
		{
			state->trailing_space = TRUE;
		}
#ifdef LOCAL_DEBUG
XP_TRACE(("Found Preformatted text (%s)\n", tmp_buf));
#endif /* LOCAL_DEBUG */
		PA_UNLOCK(tmp_block);

#if WHAT
		/*
		 * If this is an empty string, just throw it out
		 * and move on
		 */
		if (*w_start == '\0')
		{
			*w_end = tchar1;
#ifdef LOCAL_DEBUG
XP_TRACE(("Throwing out empty string!\n"));
#endif /* LOCAL_DEBUG */
			continue;
		}
#endif

		/*
		 * If we have extra text, Append it to the line buffer.
		 * It may be necessary to expand the line
		 * buffer.
		 */
		if (*w_start != '\0')
		{
			int32 old_len;
			Bool old_begin_line;

			old_len = state->line_buf_len;
			old_begin_line = state->at_begin_line;

			if ((state->line_buf_len + line_length + 1) >
				state->line_buf_size)
			{
				state->line_buf = PA_REALLOC(
					state->line_buf, (state->line_buf_size +
					line_length + LINE_BUF_INC));
				if (state->line_buf == NULL)
				{
					*w_end = tchar1;
					state->top_state->out_of_memory = TRUE;
					break;
				}
				state->line_buf_size += (line_length +
					LINE_BUF_INC);
			}
			PA_LOCK(text_buf, char *, state->line_buf);
			PA_LOCK(tmp_buf, char *, tmp_block);

			XP_BCOPY(tmp_buf,
			    (char *)(text_buf + state->line_buf_len),
			    (line_length + 1));
			state->line_buf_len += (intn) line_length;
			PA_UNLOCK(state->line_buf);
			PA_UNLOCK(tmp_block);

			/*
			 * Having added text, we cannot be at the start
			 * of a line
			 */
			state->cur_ele_type = LO_TEXT;
			state->at_begin_line = FALSE;

			/*
			 * Most common case is appending to the same line.
			 * assume that is what we are doing here.
			 */
			text_data.text = state->line_buf;
			text_data.text_len = (int16)state->line_buf_len;
			text_data.text_attr = state->font_stack->text_attr;
			FE_GetTextInfo(context, &text_data,
				&(state->text_info));
			state->width =
				lo_correct_text_element_width(
				&(state->text_info));

			/*
			 * If this is a special wrapping pre, and we would
			 * wrap here, break before this, and strip all
			 * following whitespace so there is none at
			 * the start of the next line.
			 * If we were at the beginning of the line before
			 * this, then obviously trying to wrap here will
			 * be pointless, and will in fact cause an
			 * infinite loop.
			 */
			if ((state->preformatted == PRE_TEXT_WRAP)&&
			    (old_begin_line == FALSE)&&
			    ((state->x + state->width) > state->right_margin))
			{
				PA_LOCK(text_buf, char *, state->line_buf);
				text_buf[old_len] = '\0';
				PA_UNLOCK(state->line_buf);
				state->line_buf_len = old_len;

				*w_end = tchar1;
				tptr = w_start;
				while ((XP_IS_SPACE(*tptr))&&(*tptr != '\0'))
				{
					tptr++;
				}
				line_break = TRUE;
				w_start = tptr;
				w_end = tptr;
				tchar1 = *w_end;
			}
		}

		if (tchar1 == LF)
		{
			line_break = TRUE;
		}
		else if (tchar1 == CR)
		{
			line_break = TRUE;
			have_CR = TRUE;
		}

		/*
		 * If we are breaking the line here, flush the
		 * line_buf, and then insert a linebreak.
		 */
		if (line_break != FALSE)
		{
#ifdef LOCAL_DEBUG
XP_TRACE(("LineBreak, flush text.\n"));
#endif /* LOCAL_DEBUG */

			/*
			 * Flush the line and insert the linebreak.
			 */
			PA_LOCK(text_buf, char *, state->line_buf);
			text_data.text = state->line_buf;
			text_data.text_len = (int16)state->line_buf_len;
			text_data.text_attr = state->font_stack->text_attr;
			FE_GetTextInfo(context, &text_data,&(state->text_info));
			PA_UNLOCK(state->line_buf);
			state->width = lo_correct_text_element_width(
				&(state->text_info));
			lo_FlushLineBuffer(context, state);
#ifdef EDITOR
	/* LTNOTE: do something here like: */
			/*state->edit_current_offset += (word_ptr - text_buf);*/
#endif
			if (state->top_state->out_of_memory != FALSE)
			{
				PA_FREE(tmp_block);
				return;
			}
			/*
			 * Put on a linefeed element.
			 * This line is finished and will be added
			 * to the line array.
			 */
			lo_HardLineBreak(context, state, TRUE);

			state->line_buf_len = 0;
			state->width = 0;

			/*
			 * having just broken the line, we have no break
			 * position.
			 */
			state->break_pos = -1;
			state->break_width = 0;
		}

		*w_end = tchar1;
		if ((*tptr == CR)||(*tptr == LF))
		{
			tptr++;
		}
		PA_FREE(tmp_block);
	}

	/*
	 * Because we just might get passed text broken between the
	 * CR and the LF, we need to save this state.
	 */
	if ((tptr > text)&&(*(tptr - 1) == CR))
	{
		state->last_char_CR = TRUE;
	}
}




/*************************************
 * Function: lo_FormatText
 *
 * Description: This function formats text by breaking it into lines
 *	at word boundries.  Word boundries are whitespace, or special
 *	word break tags.
 *
 * Params: Window context and document state., and the text to be formatted.
 *
 * Returns: Nothing
 *************************************/
void
lo_FormatText(MWContext *context, lo_DocState *state, char *text)
{
	char *tptr;
	char *w_start;
	char *w_end;
	char *text_buf;
	char tchar1;
	int32 word_len;
	Bool line_break;
	Bool word_break;
	Bool prev_word_breakable;
	Bool white_space;
	int16 charset;
	Bool multi_byte;
	LO_TextStruct text_data;

	/*
	 * Initialize the structures to 0 (mark)
	 */
	memset (&text_data, 0, sizeof (LO_TextStruct));

	/*
	 * Error conditions
	 */
	if ((state == NULL)||(state->cur_ele_type != LO_TEXT)||(text == NULL))
	{
		return;
	}

	charset = state->font_stack->text_attr->charset;
	if ((INTL_CharSetType(charset) == SINGLEBYTE) ||
		(INTL_CharSetType(charset) & CS_SPACE))
	{
		multi_byte = FALSE;
	}
	else
	{
		multi_byte = TRUE;
	}

	/*
	 * Move through this text fragment, breaking it up into
	 * words, and then grouping the words into lines.
	 */
	tptr = text;
	prev_word_breakable = FALSE;
	while ((*tptr != '\0')&&(state->top_state->out_of_memory == FALSE))
	{
		Bool has_nbsp;
#ifdef TEXT_CHUNK_LIMIT
		int32 w_char_cnt;
#endif /* TEXT_CHUNK_LIMIT */
#ifdef XP_WIN16
		int32 ccnt;
#endif /* XP_WIN16 */
		Bool mb_sp;  /* Allow space between multibyte */

		/*
		 * white_space is a tag to tell us if the currenct word
		 * contains nothing but whitespace.
		 * word_break tells us if there was whitespace
		 * before this word so we know we can break it.
		 */
		white_space = FALSE;
		word_break = FALSE;
		line_break = FALSE;
		has_nbsp = FALSE;
		mb_sp = FALSE;

		if (multi_byte == FALSE)
		{
			/*
			 * Find the start of the word, skipping whitespace.
			 */
			w_start = tptr;
			while ((XP_IS_SPACE(*tptr))&&(*tptr != '\0'))
			{
				tptr++;
			}

			/*
			 * if tptr has been moved at all, that means
			 * there was some whitespace to skip, which means
			 * we are allowed to put a linebreak before this
			 * word if we want to.
			 */
			if (tptr != w_start)
			{
				int32 new_break_holder;
				int32 min_width;

				w_start = tptr;
				word_break = TRUE;

				new_break_holder = state->x + state->width;
				min_width = new_break_holder - state->break_holder + 1;
				if (min_width > state->min_width)
				{
					state->min_width = min_width;
				}
				state->break_holder = new_break_holder;
			}

			/*
			 * Find the end of the word.
			 * Terminate the word, saving the char we replaced
			 * with the terminator so it can be restored later.
			 */
#ifdef TEXT_CHUNK_LIMIT
			w_char_cnt = 0;
#endif /* TEXT_CHUNK_LIMIT */
#ifdef XP_WIN16
			ccnt = state->line_buf_len;
			while ((!XP_IS_SPACE(*tptr))&&(*tptr != '\0')&&(ccnt < SIZE_LIMIT))
			{
				if ((unsigned char)*tptr == NON_BREAKING_SPACE)
				{
					has_nbsp = TRUE;
				}
				tptr++;
#ifdef TEXT_CHUNK_LIMIT
				w_char_cnt++;
#endif /* TEXT_CHUNK_LIMIT */
				ccnt++;
			}
			if (ccnt >= SIZE_LIMIT)
			{
				line_break = TRUE;
			}
#else
			while ((!XP_IS_SPACE(*tptr))&&(*tptr != '\0'))
			{
				if ((unsigned char)*tptr == NON_BREAKING_SPACE)
				{
					/* *tptr = ' '; Replace this later */
					has_nbsp = TRUE;
				}
				tptr++;
#ifdef TEXT_CHUNK_LIMIT
				w_char_cnt++;
#endif /* TEXT_CHUNK_LIMIT */
			}
#endif /* XP_WIN16 */
		}
		else
		{
			/*
			 * Find the start of the word, skipping whitespace.
			 */
			w_start = tptr;
			while ((XP_IS_SPACE(*tptr))&&(*tptr != '\0'))
			{
				tptr = INTL_NextChar(charset, tptr);
			}
			if (w_start != tptr)
				mb_sp = TRUE;

			/*
			 * if tptr has been moved at all, that means
			 * there was some whitespace to skip, which means
			 * we are allowed to put a linebreak before this
			 * word if we want to.
			 */
			/*
			 * If this char is a two-byte thing, we can break
			 * before it.
			 */
			if ((tptr != w_start)||((unsigned char)*tptr > 127))
			{
				int32 new_break_holder;
				int32 min_width;

				/* If it's multibyte character, it always be able to break */
				if (tptr == w_start)
					prev_word_breakable = TRUE;
				w_start = tptr;
				word_break = TRUE;

				new_break_holder = state->x + state->width;
				min_width = new_break_holder - state->break_holder + 1;
				if (min_width > state->min_width)
				{
					state->min_width = min_width;
				}
				state->break_holder = new_break_holder;
			}
			else if (prev_word_breakable)
			{
				int32 new_break_holder;
				int32 min_width;

				prev_word_breakable = FALSE;
				w_start = tptr;
				word_break = TRUE;

				new_break_holder = state->x + state->width;
				min_width = new_break_holder - state->break_holder + 1;
				if (min_width > state->min_width)
				{
					state->min_width = min_width;
				}
				state->break_holder = new_break_holder;
			}

			/*
			 * Find the end of the word.
			 * Terminate the word, saving the char we replaced
			 * with the terminator so it can be restored later.
			 */
#ifdef TEXT_CHUNK_LIMIT
			w_char_cnt = 0;
#endif /* TEXT_CHUNK_LIMIT */
#ifdef XP_WIN16
			ccnt = state->line_buf_len;
			while (((unsigned char)*tptr < 128)&&(!XP_IS_SPACE(*tptr))&&(*tptr != '\0')&&(ccnt < SIZE_LIMIT))
			{
				intn c_len;
				char *tptr2;

				tptr2 = INTL_NextChar(charset, tptr);
				c_len = (intn)(tptr2 - tptr);
				tptr = tptr2;
#ifdef TEXT_CHUNK_LIMIT
				w_char_cnt += c_len;
#endif /* TEXT_CHUNK_LIMIT */
				ccnt += c_len;
			}
			if (ccnt >= SIZE_LIMIT)
			{
				line_break = TRUE;
			}
#else
			while (((unsigned char)*tptr < 128)&&(!XP_IS_SPACE(*tptr))&&(*tptr != '\0'))
			{
				intn c_len;
				char *tptr2;

				tptr2 = INTL_NextChar(charset, tptr);
				c_len = (intn)(tptr2 - tptr);
				tptr = tptr2;
#ifdef TEXT_CHUNK_LIMIT
				w_char_cnt += c_len;
#endif /* TEXT_CHUNK_LIMIT */
			}
#endif /* XP_WIN16 */
		}  /* multi byte */

#ifdef TEXT_CHUNK_LIMIT
		if (w_char_cnt > TEXT_CHUNK_LIMIT)
		{
			tptr = (char *)(tptr - (w_char_cnt - TEXT_CHUNK_LIMIT));
			w_char_cnt = TEXT_CHUNK_LIMIT;
		}

		if ((state->line_buf_len + w_char_cnt) > TEXT_CHUNK_LIMIT)
		{
			lo_FlushLineBuffer(context, state);
			if (state->top_state->out_of_memory != FALSE)
			{
				return;
			}
			if (state->cur_ele_type != LO_TEXT)
			{
				lo_FreshText(state);
				state->cur_ele_type = LO_TEXT;
			}
		}
#endif /* TEXT_CHUNK_LIMIT */
		if (multi_byte != FALSE)
		{
			if ((w_start == tptr)&&((unsigned char)*tptr > 127))
			{
				tptr = INTL_NextChar(charset, tptr);
			}
		}
		w_end = tptr;
		tchar1 = *w_end;
		*w_end = '\0';

		/*
		 * If the "word" is just an empty string, this
		 * is just whitespace that we may wish to compress out.
		 */
		if (*w_start == '\0')
		{
			white_space = TRUE;
		}

		/*
		 * compress out whitespace if the last word added was also
		 * whitespace.
		 */
		if ((white_space != FALSE)&&(state->trailing_space != FALSE))
		{
			*w_end = tchar1;
#ifdef LOCAL_DEBUG
XP_TRACE(("Discarding(%s)\n", w_start));
#endif /* LOCAL_DEBUG */
			continue;
		}

		/*
		 * This places the preceeding space in front of
		 * separate words on a line.
		 * Unecessary if last item was trailng space.
		 *
		 * If there was a word break before this word, so we know it
		 * was supposed to be separate, and if we are not at the
		 * beginning of the line, and if the
		 * preceeding word is not already whitespace, then add
		 * a space before this word.
		 */
		if ((word_break != FALSE)&&
			(state->at_begin_line == FALSE)&&
			(state->trailing_space == FALSE))
		{
			/*
			 * Since word_break is true, we know
			 * we skipped some spaces previously
			 * so we know there is space to back up
			 * the word pointer inside the buffer.
			 */
		    if ((multi_byte == FALSE)||mb_sp)
		    {
				w_start--;
				*w_start = ' ';
		    }

			/*
			 * If we are formatting breakable text
			 * set break position to be just before this word.
			 * This is where we will break this line if the
			 * new word makes it too long.
			 */
			if (state->breakable != FALSE)
			{
				state->break_pos = state->line_buf_len;
				state->break_width = state->width;
			}
		}

#ifdef LOCAL_DEBUG
XP_TRACE(("Found Word (%s)\n", w_start));
#endif /* LOCAL_DEBUG */
		/*
		 * If this is an empty string, just throw it out
		 * and move on
		 */
		if (*w_start == '\0')
		{
			*w_end = tchar1;
#ifdef LOCAL_DEBUG
XP_TRACE(("Throwing out empty string!\n"));
#endif /* LOCAL_DEBUG */
			continue;
		}

		/*
		 * Now we catch those nasty non-breaking space special
		 * characters and make them spaces.  Yuck, so that 
		 * relayout in tables will still see the non-breaking
		 * spaces, we need to copy the buffer here.
		 */
		if (has_nbsp != FALSE)
		{
			char *tmp_ptr;
			char *to_ptr;
			char *tmp_buf;
			PA_Block tmp_block;

			tmp_block = PA_ALLOC(XP_STRLEN(w_start) + 1);
			if (tmp_block == NULL)
			{
				*w_end = tchar1;
				state->top_state->out_of_memory = TRUE;
				break;
			}
			PA_LOCK(tmp_buf, char *, tmp_block);

			tmp_ptr = w_start;
			to_ptr = tmp_buf;
			while (*tmp_ptr != '\0')
			{
				*to_ptr = *tmp_ptr;
#ifndef XP_MAC	
/*	Don't change NON_BREAKING_SPACE to SPACE in Mac See Bug # 6640 */
				if ((unsigned char)*to_ptr ==
					NON_BREAKING_SPACE)
				{
					*to_ptr = ' ';
				}
#endif
				to_ptr++;
				tmp_ptr++;
			}
			*w_end = tchar1;
			w_start = tmp_buf;
			w_end = to_ptr;
			*w_end = '\0';
		}

		/*
		 * Make this Front End specific text, and count
		 * the length of the word.
		 */
		/* don't need this any more since we're converting
		 * elsewhere -- erik
		w_start = FE_TranslateISOText(context, charset, w_start);
		 */
		word_len = XP_STRLEN(w_start);

		/*
		 * Append this word to the line buffer.
		 * It may be necessary to expand the line
		 * buffer.
		 */
		if ((state->line_buf_len + word_len + 1) >
			state->line_buf_size)
		{
			state->line_buf = PA_REALLOC( state->line_buf,
				(state->line_buf_size +
				word_len + LINE_BUF_INC));
			if (state->line_buf == NULL)
			{
				*w_end = tchar1;
				state->top_state->out_of_memory = TRUE;
				break;
			}
			state->line_buf_size += (word_len + LINE_BUF_INC);
		}
		PA_LOCK(text_buf, char *, state->line_buf);
		XP_BCOPY(w_start,
		    (char *)(text_buf + state->line_buf_len),
		    (word_len + 1));
		state->line_buf_len += word_len;
		PA_UNLOCK(state->line_buf);

		/*
		 * Having added a word, we cannot be at the start of a line
		 */
		state->cur_ele_type = LO_TEXT;
		state->at_begin_line = FALSE;

		/*
		 * Most common case is appending to the same line.
		 * assume that is what we are doing here.
		 */
		text_data.text = state->line_buf;
		text_data.text_len = (int16)state->line_buf_len;
		text_data.text_attr = state->font_stack->text_attr;
		FE_GetTextInfo(context, &text_data, &(state->text_info));
		state->width =
			lo_correct_text_element_width(&(state->text_info));

		/*
		 * Set line_break based on document window width
		 */
#ifdef XP_WIN16
		if (((state->x + state->width) > state->right_margin)||(line_break != FALSE))
#else
		if ((state->x + state->width) > state->right_margin)
#endif /* XP_WIN16 */
		{
			/*  
			 * INTL kinsoku line break, some of characters are not allowed to put 
			 * in the end of line or beginning of line
			 */
			if (multi_byte && (state->break_pos != -1))
			{
				int cur_wordtype, pre_wordtype, pre_break_pos;
				cur_wordtype = INTL_KinsokuClass(charset, (unsigned char *) w_start);

				PA_LOCK(text_buf, char *, state->line_buf);
				pre_break_pos = INTL_PrevCharIdx(charset, 
					(unsigned char *)text_buf, state->break_pos);
				pre_wordtype = INTL_KinsokuClass(charset, 
					(unsigned char *)(text_buf + pre_break_pos));

				if (pre_wordtype == PROHIBIT_END_OF_LINE ||
				    (cur_wordtype == PROHIBIT_BEGIN_OF_LINE && 
					 XP_IS_ALPHA(*(text_buf+pre_break_pos)) == FALSE))
					state->break_pos = pre_break_pos;

				PA_UNLOCK(state->line_buf);
			}
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
#ifdef XP_WIN16
			else if (ccnt >= SIZE_LIMIT)
			{
				state->break_pos = state->line_buf_len - 1;
			}
			else
			{
				line_break = FALSE;
			}
#else
			else
			{
				line_break = FALSE;
			}
#endif /* XP_WIN16 */
		}

		/*
		 * If we are breaking the line here, flush the
		 * line_buf, and then insert a linebreak.
		 */
		if (line_break != FALSE)
		{
			char *break_ptr;
			char *word_ptr;
			char *new_buf;
			PA_Block new_block;
#ifdef LOCAL_DEBUG
XP_TRACE(("LineBreak, flush text.\n"));
#endif /* LOCAL_DEBUG */

			/*
			 * Find the breaking point, and the pointer
			 * to the remaining word without its leading
			 * space.
			 */
			PA_LOCK(text_buf, char *, state->line_buf);
			break_ptr = (char *)(text_buf + state->break_pos);
/*			word_ptr = (char *)(break_ptr + 1); */
			word_ptr = break_ptr;

		    if ((multi_byte == FALSE)||mb_sp)
		    {
				word_ptr++;
		    }

			/*
			 * Copy the remaining word into its
			 * own buffer.
			 */
			word_len = XP_STRLEN(word_ptr);
			new_block = PA_ALLOC((word_len + 1) *
				sizeof(char));
			if (new_block == NULL)
			{
				PA_UNLOCK(state->line_buf);
				*w_end = tchar1;
				state->top_state->out_of_memory = TRUE;
				break;
			}
			PA_LOCK(new_buf, char *, new_block);
			XP_BCOPY(word_ptr, new_buf, (word_len + 1));

			*break_ptr = '\0';
			state->line_buf_len = state->line_buf_len -
				word_len;
			if ((multi_byte == FALSE)||(word_ptr != break_ptr))
		    {
				state->line_buf_len--;
		    }
			text_data.text = state->line_buf;
			text_data.text_len = (int16)state->line_buf_len;
			text_data.text_attr = state->font_stack->text_attr;
			FE_GetTextInfo(context, &text_data,&(state->text_info));
			PA_UNLOCK(state->line_buf);
			state->width = lo_correct_text_element_width(
				&(state->text_info));
			lo_FlushLineBuffer(context, state);
#ifdef EDITOR
			state->edit_current_offset += (word_ptr - text_buf);
#endif
			if (state->top_state->out_of_memory != FALSE)
			{
				PA_UNLOCK(new_block);
				PA_FREE(new_block);
				return;
			}

			/*
			 * Put on a linefeed element.
			 * This line is finished and will be added
			 * to the line array.
			 */
			lo_HardLineBreak(context, state, TRUE);

			/*
			 * If there was no remaining word, free up
			 * the unnecessary buffer, and empty out
			 * the line buffer.
			 */
			if (word_len == 0)
			{
				PA_UNLOCK(new_block);
				PA_FREE(new_block);
				state->line_buf_len = 0;
				state->width = 0;
			}
			else
			{
				PA_LOCK(text_buf, char *,state->line_buf);
				XP_BCOPY(new_buf, text_buf, (word_len + 1));
				PA_UNLOCK(state->line_buf);
				PA_UNLOCK(new_block);
				PA_FREE(new_block);
				state->line_buf_len = word_len;
				text_data.text = state->line_buf;
				text_data.text_len = (int16)state->line_buf_len;
				text_data.text_attr =
					state->font_stack->text_attr;
				FE_GetTextInfo(context, &text_data,
					&(state->text_info));
				state->width = lo_correct_text_element_width(
					&(state->text_info));

				/*
				 * Having added text, we are no longer at the
				 * start of the line.
				 */
				state->at_begin_line = FALSE;
				state->cur_ele_type = LO_TEXT;
			}


			/*
			 * having just broken the line, we have no break
			 * position.
			 */
			state->break_pos = -1;
			state->break_width = 0;
		}
		else
		{
			if (white_space != FALSE)
			{
				state->trailing_space = TRUE;
			}
			else
			{
				state->trailing_space = FALSE;
			}
		}

		*w_end = tchar1;
	}
	/*  
	 * if last char is multibyte, break position need to be set to end of string
	 */
	if (multi_byte != FALSE && *tptr == '\0' && prev_word_breakable != FALSE)
		state->break_pos = state->line_buf_len;
}


/*************************************
 * Function: lo_FlushLineBuffer
 *
 * Description: Flush out the current line buffer of text
 *	into a new text element, and add that element to
 *	the end of the line list of elements.
 *
 * Params: Window context and document state.
 *
 * Returns: Nothing
 *************************************/
void
lo_FlushLineBuffer(MWContext *context, lo_DocState *state)
{
	LO_TextStruct *text_data;
	int32 baseline_inc;
	int32 line_inc;

	baseline_inc = 0;
	line_inc = 0;
#ifdef DEBUG
	assert (state);
#endif

	/* 
	 * LTNOTE: probably should be grabbing state edit_element and offset from
	 * state.
	*/
    text_data = lo_new_text_element(context, state, NULL, 0);

	if (text_data == NULL)
	{
		state->top_state->out_of_memory = TRUE;
		return;
	}
	state->linefeed_state = 0;

	/*
	 * Some fonts (particulatly italic ones with curly tails
	 * on letters like 'f') have a left bearing that extends
	 * back into the previous character.  Since in this case the
	 * previous character is probably not in the same font, we
	 * move forward to avoid overlap.
	 *
	 * Those same funny fonts can extend past the last character,
	 * and we also have to catch that, and advance the following text
	 * to eliminate cutoff.
	 */
	if (state->text_info.lbearing < 0)
	{
		text_data->x_offset = state->text_info.lbearing * -1;
	}
	text_data->width = state->width;

	/*
	 * The baseline of the text element just added to the line may be
	 * less than or greater than the baseline of the rest of the line
	 * due to font changes.  If the baseline is less, this is easy,
	 * we just increase y_offest to move the text down so the baselines
	 * line up.  For greater baselines, we can't move the text up to
	 * line up the baselines because we will overlay the previous line,
	 * so we have to move all the previous elements in this line down.
	 *
	 * If the baseline is zero, we are the first element on the line,
	 * and we get to set the baseline.
	 */
	if (state->baseline == 0)
	{
		state->baseline = state->text_info.ascent;
		if (state->line_height < 
			(state->baseline + state->text_info.descent))
		{
			state->line_height = state->baseline +
				state->text_info.descent;
		}
	}
	else if (state->text_info.ascent < state->baseline)
	{
		text_data->y_offset = state->baseline -
					state->text_info.ascent;
		if ((text_data->y_offset + state->text_info.ascent +
			state->text_info.descent) > state->line_height)
		{
			line_inc = text_data->y_offset +
				state->text_info.ascent +
				state->text_info.descent -
				state->line_height;
		}
	}
	else
	{
		baseline_inc = state->text_info.ascent - state->baseline;
		if ((text_data->y_offset + state->text_info.ascent +
			state->text_info.descent - baseline_inc) >
				state->line_height)
		{
			line_inc = text_data->y_offset +
				state->text_info.ascent +
				state->text_info.descent -
				state->line_height - baseline_inc;
		}
	}

	lo_AppendToLineList(context, state,
		(LO_Element *)text_data, baseline_inc);

	state->baseline += (intn) baseline_inc;
	state->line_height += (intn) (baseline_inc + line_inc);
	text_data->height = state->text_info.ascent +
		state->text_info.descent;

	/*
	 * If the element we just flushed had a breakable word
	 * position in it, save that position in case we have
	 * to go back and break this element before we finish
	 * the line.
	 */
	if (state->break_pos != -1)
	{
		state->old_break = text_data;
		state->old_break_pos = state->break_pos;
		state->old_break_width = state->break_width;
	}

	state->line_buf_len = 0;
	state->x += state->width;
	state->width = 0;
	state->cur_ele_type = LO_NONE;
}


void
lo_RecolorText(lo_DocState *state)
{
	lo_FontStack *fptr;
	LO_TextAttr *attr;

	fptr = state->font_stack;
	while (fptr != NULL)
	{
		attr = fptr->text_attr;
		attr->fg.red =   STATE_DEFAULT_FG_RED(state);
		attr->fg.green = STATE_DEFAULT_FG_GREEN(state);
		attr->fg.blue =  STATE_DEFAULT_FG_BLUE(state);
		attr->bg.red =   STATE_DEFAULT_BG_RED(state);
		attr->bg.green = STATE_DEFAULT_BG_GREEN(state);
		attr->bg.blue =  STATE_DEFAULT_BG_BLUE(state);
		fptr = fptr->next;
	}
}


/*************************************
 * Function: lo_PushFont
 *
 * Description: Push the text attribute information for a new
 *	font onto the font stack.  Also save the type of the
 *	tag that caused the change.
 *
 * Params: Document state, tag type, and the text attribute
 *	structure for the new font.
 *
 * Returns: Nothing
 *************************************/
void
lo_PushFont(lo_DocState *state, intn tag_type, LO_TextAttr *attr)
{
	lo_FontStack *fptr;

	fptr = XP_NEW(lo_FontStack);
	if (fptr == NULL)
	{
		return;
	}
	fptr->tag_type = tag_type;
	fptr->text_attr = attr;
	fptr->next = state->font_stack;
	state->font_stack = fptr;;
}


/*************************************
 * Function: lo_PopFontStack
 *
 * Description: This function pops the next font
 *	off the font stack, and return the text attribute of the
 *	previous font.
 *	The last font on the font stack cannot be popped off.
 *
 * Params: Document state, and the tag type that caused the change.
 *
 * Returns: The LO_TextAttr structure of the font just passed.
 *************************************/
LO_TextAttr *
lo_PopFontStack(lo_DocState *state, intn tag_type)
{
	LO_TextAttr *attr;
	lo_FontStack *fptr;

	if (state->font_stack->next == NULL)
	{
#ifdef LOCAL_DEBUG
XP_TRACE(("Popped too many fonts!\n"));
#endif /* LOCAL_DEBUG */
		return(NULL);
	}

	fptr = state->font_stack;
	attr = fptr->text_attr;
	if (fptr->tag_type != tag_type)
	{
#ifdef LOCAL_DEBUG
XP_TRACE(("Warning:  Font popped by different TAG than pushed it %d != %d\n", fptr->tag_type, tag_type));
#endif /* LOCAL_DEBUG */
	}
	state->font_stack = fptr->next;
	XP_DELETE(fptr);

	return(attr);
}


LO_TextAttr *
lo_PopFont(lo_DocState *state, intn tag_type)
{
	LO_TextAttr *attr;
	lo_FontStack *fptr;

	/*
	 * This should never happen, but we are patching a
	 * more serious problem that causes us to be called
	 * here after the font stack has been freed.
	 */
	if ((state->font_stack == NULL)||(state->font_stack->next == NULL))
	{
#ifdef LOCAL_DEBUG
XP_TRACE(("Popped too many fonts!\n"));
#endif /* LOCAL_DEBUG */
		return(NULL);
	}

	fptr = state->font_stack;
	attr = NULL;

	if (fptr->tag_type != P_ANCHOR)
	{
		attr = fptr->text_attr;
		if (fptr->tag_type != tag_type)
		{
#ifdef LOCAL_DEBUG
XP_TRACE(("Warning:  Font popped by different TAG than pushed it %d != %d\n", fptr->tag_type, tag_type));
#endif /* LOCAL_DEBUG */
		}
		state->font_stack = fptr->next;
		XP_DELETE(fptr);
	}
	else
	{
		while ((fptr->next != NULL)&&(fptr->next->tag_type == P_ANCHOR))
		{
			fptr = fptr->next;
		}
		if (fptr->next->next != NULL)
		{
			lo_FontStack *f_tmp;

			f_tmp = fptr->next;
			fptr->next = fptr->next->next;
			attr = f_tmp->text_attr;
			XP_DELETE(f_tmp);
		}
	}

	return(attr);
}


void
lo_PopAllAnchors(lo_DocState *state)
{
	lo_FontStack *fptr;

	if (state->font_stack->next == NULL)
	{
#ifdef LOCAL_DEBUG
XP_TRACE(("Popped too many fonts!\n"));
#endif /* LOCAL_DEBUG */
		return;
	}

	/*
	 * Remove all anchors on top of the font stack
	 */
	fptr = state->font_stack;
	while ((fptr->tag_type == P_ANCHOR)&&(fptr->next != NULL))
	{
		lo_FontStack *f_tmp;

		f_tmp = fptr;
		fptr = fptr->next;
		XP_DELETE(f_tmp);
	}
	state->font_stack = fptr;

	/*
	 * Remove all anchors buried in the stack
	 */
	while (fptr->next != NULL)
	{
		/*
		 * Reset spurrious anchor color text entries
		 */
		if ((fptr->text_attr != NULL)&&
			(fptr->text_attr->attrmask & LO_ATTR_ANCHOR))
		{
			LO_TextAttr tmp_attr;

			lo_CopyTextAttr(fptr->text_attr, &tmp_attr);
			tmp_attr.attrmask =
				tmp_attr.attrmask & (~LO_ATTR_ANCHOR);
			tmp_attr.fg.red = STATE_DEFAULT_FG_RED(state);
			tmp_attr.fg.green = STATE_DEFAULT_FG_GREEN(state);
			tmp_attr.fg.blue = STATE_DEFAULT_FG_BLUE(state);
			tmp_attr.bg.red = STATE_DEFAULT_BG_RED(state);
			tmp_attr.bg.green = STATE_DEFAULT_BG_GREEN(state);
			tmp_attr.bg.blue = STATE_DEFAULT_BG_BLUE(state);
			fptr->text_attr = lo_FetchTextAttr(state, &tmp_attr);
		}

		if (fptr->next->tag_type == P_ANCHOR)
		{
			lo_FontStack *f_tmp;

			f_tmp = fptr->next;
			fptr->next = fptr->next->next;
			XP_DELETE(f_tmp);
		}
		else
		{
			fptr = fptr->next;
		}
	}
}


void
lo_PlaceBullet(MWContext *context, lo_DocState *state)
{
	LO_BulletStruct *bullet = NULL;
	LO_TextAttr tmp_attr;
	LO_TextStruct tmp_text;
	LO_TextInfo text_info;
	LO_TextAttr *tptr;
	PA_Block buff;
	char *str;
	int32 bullet_size;

	bullet = (LO_BulletStruct *)lo_NewElement(context, state, LO_BULLET, NULL, 0);
	if (bullet == NULL)
	{
#ifdef DEBUG
		assert (state->top_state->out_of_memory);
#endif
		return;
	}

	lo_SetDefaultFontAttr(state, &tmp_attr, context);
	tptr = lo_FetchTextAttr(state, &tmp_attr);

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
	tmp_text.text_attr = tptr;
	FE_GetTextInfo(context, &tmp_text, &text_info);
	PA_FREE(buff);

	bullet_size = (text_info.ascent + text_info.descent) / 2;

	bullet->type = LO_BULLET;
	bullet->ele_id = NEXT_ELEMENT;
	bullet->x = state->x - (2 * bullet_size);
	if (bullet->x < state->win_left)
	{
		bullet->x = state->win_left;
	}
	bullet->x_offset = 0;
	bullet->y = state->y;
	bullet->y_offset =
		(text_info.ascent + text_info.descent - bullet_size) / 2;
	bullet->width = bullet_size;
	bullet->height = bullet_size;
	bullet->next = NULL;
	bullet->prev = NULL;

	bullet->FE_Data = NULL;

	bullet->level = state->list_stack->level;
	bullet->bullet_type = state->list_stack->bullet_type;
	bullet->text_attr = tptr;

	bullet->ele_attrmask = 0;

	bullet->sel_start = -1;
	bullet->sel_end = -1;

	lo_AppendToLineList(context, state, (LO_Element *)bullet, 0);

	state->baseline = text_info.ascent;
	state->line_height = text_info.ascent + text_info.descent;

	/*
	 * Clean up state
	 */
/*
 * Supporting old mistakes made in some other browsers.
 * I will put the "correct code" here, but comment it out, since
 * some other browsers allowed headers inside lists, so we should to, sigh.
	state->linefeed_state = 0;
 */
	state->at_begin_line = TRUE;
	state->cur_ele_type = LO_BULLET;
	if (bullet->x == state->win_left)
	{
		state->x += (bullet->x_offset + (2 * bullet->width));
	}
}


void
lo_PlaceBulletStr(MWContext *context, lo_DocState *state)
{
	intn len;
	char str2[22];
	char *str;
	char *str3;
	PA_Block buff;
	LO_TextStruct *bullet_text = NULL;
	LO_TextInfo text_info;

	bullet_text = (LO_TextStruct *)lo_NewElement(context, state, LO_TEXT, NULL, 0);
	if (bullet_text == NULL)
	{
#ifdef DEBUG
		assert (state->top_state->out_of_memory);
#endif
		return;
	}

	if( EDT_IS_EDITOR( context )) 
	{
		switch( state->list_stack->bullet_type ){
		case BULLET_ALPHA_L:
			str = "A";
			break;
		case BULLET_ALPHA_S:
			str = "a";
			break;
		case BULLET_NUM_S_ROMAN:
			str = "x";
			break;
		case BULLET_NUM_L_ROMAN:
			str = "X";
			break;
		default:
			str = "#";
			break;
		}
		len = XP_STRLEN(str);
		buff = PA_ALLOC(len + 1);
		if (buff != NULL)
		{
			PA_LOCK(str3, char *, buff);
			XP_STRCPY(str3, str);
			PA_UNLOCK(buff);
		}
	}
	else {
		if (state->list_stack->bullet_type == BULLET_ALPHA_S)
		{
			buff = lo_ValueToAlpha(state->list_stack->value, FALSE, &len);
		}
		else if (state->list_stack->bullet_type == BULLET_ALPHA_L)
		{
			buff = lo_ValueToAlpha(state->list_stack->value, TRUE, &len);
		}
		else if (state->list_stack->bullet_type == BULLET_NUM_S_ROMAN)
		{
			buff = lo_ValueToRoman(state->list_stack->value, FALSE, &len);
		}
		else if (state->list_stack->bullet_type == BULLET_NUM_L_ROMAN)
		{
			buff = lo_ValueToRoman(state->list_stack->value, TRUE, &len);
		}
		else
		{
			XP_SPRINTF(str2, "%d.", (intn)state->list_stack->value);
			len = XP_STRLEN(str2);
			buff = PA_ALLOC(len + 1);
			if (buff != NULL)
			{
				PA_LOCK(str, char *, buff);
				XP_STRCPY(str, str2);
				PA_UNLOCK(buff);
			}
			else
			{
				state->top_state->out_of_memory = TRUE;
			}
		}
	}

	if (buff == NULL)
	{
		return;
	}

	bullet_text->text = buff;
	bullet_text->text_len = len;
	bullet_text->text_attr = state->font_stack->text_attr;
	FE_GetTextInfo(context, bullet_text, &text_info);
	bullet_text->width = lo_correct_text_element_width(&text_info);
	bullet_text->height = text_info.ascent + text_info.descent;

	bullet_text->type = LO_TEXT;
	bullet_text->ele_id = NEXT_ELEMENT;
	bullet_text->x = state->x - (bullet_text->height / 2) -
		bullet_text->width;
	if (bullet_text->x < state->win_left)
	{
		bullet_text->x = state->win_left;
	}
	bullet_text->x_offset = 0;
	bullet_text->y = state->y;
	bullet_text->y_offset = 0;

	bullet_text->anchor_href = state->current_anchor;

	bullet_text->ele_attrmask = 0;
	if (state->breakable != FALSE)
	{
		bullet_text->ele_attrmask |= LO_ELE_BREAKABLE;
	}

	bullet_text->sel_start = -1;
	bullet_text->sel_end = -1;

	bullet_text->next = NULL;
	bullet_text->prev = NULL;

	bullet_text->FE_Data = NULL;

	lo_AppendToLineList(context, state, (LO_Element *)bullet_text, 0);

	state->baseline = text_info.ascent;
	state->line_height = (intn) bullet_text->height;

	/*
	 * Clean up state
	 */
/*
 * Supporting old mistakes made in some other browsers.
 * I will put the "correct code" here, but comment it out, since
 * some other browsers allowed headers inside lists, so we should to, sigh.
	state->linefeed_state = 0;
	state->at_begin_line = FALSE;
 */
	state->at_begin_line = TRUE;
	state->cur_ele_type = LO_TEXT;
}


#ifdef TEST_16BIT
#undef XP_WIN16
#endif /* TEST_16BIT */

#ifdef PROFILE
#pragma profile off
#endif
