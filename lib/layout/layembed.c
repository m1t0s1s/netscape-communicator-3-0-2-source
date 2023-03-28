/* -*- Mode: C; tab-width: 4; -*- */
#include "xp.h"
#include "net.h"
#include "pa_parse.h"
#include "layout.h"
#include "np.h"


#ifdef XP_WIN16
#define SIZE_LIMIT              32000
#endif /* XP_WIN16 */


#define EMBED_DEF_DIM			50
#define EMBED_DEF_BORDER		0
#define EMBED_DEF_VERTICAL_SPACE	0
#define EMBED_DEF_HORIZONTAL_SPACE	0


void lo_FinishEmbed(MWContext *, lo_DocState *, LO_EmbedStruct *);

void
lo_AddEmbedData(MWContext *context, void *session_data, lo_FreeProc freeProc, int32 indx)
{
	int32 doc_id;
	lo_TopState *top_state;
	lo_DocState *state;
	lo_SavedEmbedListData *embed_list;
	lo_EmbedDataElement* embed_data_list;
	int32 old_count;

	/*
	 * Get the state, so we can access the savedEmbedListData
	 * list for this document.
	 */
	doc_id = XP_DOCID(context);
	top_state = lo_FetchTopState(doc_id);
	if ((top_state != NULL) && (top_state->doc_state != NULL))
		state = top_state->doc_state;
	else
		return;

	embed_list = state->top_state->savedData.EmbedList;
	if (embed_list == NULL)
		return;

	/*
	 * We may have to grow the list to fit this new data.
	 */
	if (indx >= embed_list->embed_count)
	{
		PA_Block old_embed_data_list; /* really a (lo_EmbedDataElement **) */

#ifdef XP_WIN16
		/*
		 * On 16 bit windows arrays can just get too big.
		 */
		if (((indx + 1) * sizeof(lo_EmbedDataElement)) > SIZE_LIMIT)
			return;
#endif /* XP_WIN16 */

		old_count = embed_list->embed_count;
		embed_list->embed_count = indx + 1;
		old_embed_data_list = NULL;
		
		if (old_count == 0)		/* Allocate new array */
		{
			embed_list->embed_data_list = PA_ALLOC(
				embed_list->embed_count * sizeof(lo_EmbedDataElement));
		}
		else					/* Enlarge existing array */
		{
			old_embed_data_list = embed_list->embed_data_list;
			embed_list->embed_data_list = PA_REALLOC(
				embed_list->embed_data_list,
				(embed_list->embed_count * sizeof(lo_EmbedDataElement)));
		}
		
		/*
		 * If we ran out of memory to grow the array.
		 */
		if (embed_list->embed_data_list == NULL)
		{
			embed_list->embed_data_list = old_embed_data_list;
			embed_list->embed_count = old_count;
			state->top_state->out_of_memory = TRUE;
			return;
		}
		
		/*
		 * We enlarged the array, so make sure
		 * the new slots are zero-filled.
		 */
		PA_LOCK(embed_data_list, lo_EmbedDataElement*, embed_list->embed_data_list);
		while (old_count < embed_list->embed_count)
		{
			embed_data_list[old_count].data = NULL;
			embed_data_list[old_count].freeProc = NULL;
			old_count++;
		}
		PA_UNLOCK(embed_list->embed_data_list);
	}

	/*
	 * Store the new data into the list.
	 */
	PA_LOCK(embed_data_list, lo_EmbedDataElement*, embed_list->embed_data_list);
	embed_data_list[indx].data = session_data;
	embed_data_list[indx].freeProc = freeProc;
	PA_UNLOCK(embed_list->embed_data_list);
}


void
LO_ClearEmbedBlock(MWContext *context, LO_EmbedStruct *embed)
{
	int32 doc_id;
	lo_TopState *top_state;
	lo_DocState *main_doc_state;
	lo_DocState *state;

	/*
	 * Get the unique document ID, and retreive this
	 * documents layout state.
	 */
	doc_id = XP_DOCID(context);
	top_state = lo_FetchTopState(doc_id);
	if ((top_state == NULL)||(top_state->doc_state == NULL))
	{
		return;
	}

	if (top_state->layout_blocking_element == (LO_Element *)embed)
	{
		if (embed->width == 0)
		{
			embed->width = EMBED_DEF_DIM;
		}
		if (embed->height == 0)
		{
			embed->height = EMBED_DEF_DIM;
		}

		main_doc_state = top_state->doc_state;
		state = lo_CurrentSubState(main_doc_state);

		lo_FinishEmbed(context, state, embed);
		lo_FlushBlockage(context, state, main_doc_state);
	}
}


void
lo_FormatEmbed(MWContext *context, lo_DocState *state, PA_Tag *tag)
{
	LO_EmbedStruct *embed;
	PA_Block buff;
	char *str;
	int32 val;
	int32 doc_width;
	XP_Bool widthSpecified = FALSE;
	XP_Bool heightSpecified = FALSE;

	/*
	 * Fill in the embed structure with default data
	 */
	embed = (LO_EmbedStruct *)lo_NewElement(context, state, LO_EMBED, NULL, 0);
	if (embed == NULL)
	{
		return;
	}

	embed->type = LO_EMBED;
	embed->ele_id = NEXT_ELEMENT;
	embed->x = state->x;
	embed->x_offset = 0;
	embed->y = state->y;
	embed->y_offset = 0;
	embed->width = 0;
	embed->height = 0;
	embed->next = NULL;
	embed->prev = NULL;
    embed->nextEmbed= NULL;
#ifdef MOCHA
    embed->mocha_object = NULL;
#endif

	embed->FE_Data = NULL;
	embed->session_data = NULL;
	embed->attribute_cnt = 0;
	embed->attribute_list = NULL;
	embed->value_list = NULL;
	embed->line_height = state->line_height;
	embed->embed_src = NULL;
	embed->alignment = LO_ALIGN_BASELINE;
	embed->border_width = EMBED_DEF_BORDER;
	embed->border_vert_space = EMBED_DEF_VERTICAL_SPACE;
	embed->border_horiz_space = EMBED_DEF_HORIZONTAL_SPACE;
	embed->extra_attrmask = 0;
	embed->ele_attrmask = 0;

	embed->attribute_cnt = PA_FetchAllNameValues(tag,
		&(embed->attribute_list), &(embed->value_list));
	/*
	 * Convert any javascript in the values
	 */
	lo_ConvertAllValues(context, embed->value_list, embed->attribute_cnt,
						tag->newline_count);

#ifdef MOCHA /* added by jwz */
	/* only wait on embeds if onload flag */
	if (state->top_state->mocha_has_onload && !state->in_relayout)
	{
		state->top_state->mocha_loading_embeds_count++;
	}
#endif /* MOCHA -- added by jwz */

	/*
	 * Assign a unique index for this object 
	 * and increment the master index.
	 */
	embed->embed_index = state->top_state->embed_count++;

	/*
	 * Check for the hidden attribute value.
	 * HIDDEN embeds have no visible part.
	 */
	buff = lo_FetchParamValue(context, tag, PARAM_HIDDEN);
	if (buff != NULL)
	{
		Bool hidden;

		hidden = TRUE;
		PA_LOCK(str, char *, buff);
		if (pa_TagEqual("no", str))
		{
			hidden = FALSE;
		}
		else if (pa_TagEqual("false", str))
		{
			hidden = FALSE;
		}
		else if (pa_TagEqual("off", str))
		{
			hidden = FALSE;
		}
		PA_UNLOCK(buff);
		PA_FREE(buff);

		if (hidden != FALSE)
		{
			embed->extra_attrmask |= LO_ELE_HIDDEN;
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
		embed->alignment = lo_EvalAlignParam(str, &floating);
		if (floating != FALSE)
		{
			embed->ele_attrmask |= LO_ELE_FLOATING;
		}
		PA_UNLOCK(buff);
		PA_FREE(buff);
	}

	/*
	 * Get the required src parameter.
	 */
	buff = lo_FetchParamValue(context, tag, PARAM_SRC);
	if (buff != NULL)
	{
		PA_Block new_buff;
		char *new_str;
		int32 len;

		len = 1;
		PA_LOCK(str, char *, buff);
		if (str != NULL)
		{

			len = lo_StripTextWhitespace(str, XP_STRLEN(str));
		}
		if (str != NULL)
		{
			new_str = NET_MakeAbsoluteURL(
				state->top_state->base_url, str);
		}
		else
		{
			new_str = NULL;
		}

		if ((new_str == NULL)||(len == 0))
		{
			new_buff = NULL;
		}
		else
		{
			char *enbed_src;

			new_buff = PA_ALLOC(XP_STRLEN(new_str) + 1);
			if (new_buff != NULL)
			{
				PA_LOCK(enbed_src, char *, new_buff);
				XP_STRCPY(enbed_src, new_str);
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
	embed->embed_src = buff;

	doc_width = state->right_margin - state->left_margin;

	/*
	 * Get the width parameter, in absolute or percentage.
	 * If percentage, make it absolute.  If the height is
	 * zero, make the embed hidden so the FE knows that the
	 * size really is zero (zero size normally means that
	 * we're blocking).
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
					embed->extra_attrmask |= LO_ELE_HIDDEN;
					val = 0;
				}
			}
		}
		else
		{
			val = FEUNITS_X(val, context);
			if (val < 1)
			{
				embed->extra_attrmask |= LO_ELE_HIDDEN;
				val = 0;
			}
		}
		embed->width = val;
		PA_UNLOCK(buff);
		PA_FREE(buff);
		widthSpecified = TRUE;
	}

	/*
	 * Get the height parameter, in absolute or percentage.
	 * If percentage, make it absolute.  If the height is
	 * zero, make the embed hidden so the FE knows that the
	 * size really is zero (zero size normally means that
	 * we're blocking).
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
					embed->extra_attrmask |= LO_ELE_HIDDEN;
					val = 0;
				}
			}
		}
		else
		{
			val = FEUNITS_Y(val, context);
			if (val < 1)
			{
				embed->extra_attrmask |= LO_ELE_HIDDEN;
				val = 0;
			}
		}
		embed->height = val;
		PA_UNLOCK(buff);
		PA_FREE(buff);
		heightSpecified = TRUE;
	}

#ifndef XP_WIN
	/*
	 * If they forgot to specify a width or height, make one up.
	 * Don't do this for Windows, because they want to block
	 * layout for unsized OLE objects.  (Eventually all FEs will
	 * want to support this behavior when there's a plug-in API
	 * to specify the desired size of the object.)
	 */
	if (!widthSpecified)
	{
		if (heightSpecified)
			embed->width = embed->height;
		else
			embed->width = EMBED_DEF_DIM;
	}
	
	if (!heightSpecified)
	{
		if (widthSpecified)
			embed->height = embed->width;
		else
			embed->height = EMBED_DEF_DIM;
	}
#endif /* ifndef XP_WIN */

/*
 * Ignore the BORDER attribute for embeds; none of the FEs
 * actually support it.  See also bugs #23663 and #22906.
 */
#if 0
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
		embed->border_width = val;
		PA_UNLOCK(buff);
		PA_FREE(buff);
	}
	embed->border_width = FEUNITS_X(embed->border_width, context);
#endif /* if 0 */

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
		embed->border_vert_space = val;
		PA_UNLOCK(buff);
		PA_FREE(buff);
	}
	embed->border_vert_space = FEUNITS_Y(embed->border_vert_space, context);

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
		embed->border_horiz_space = val;
		PA_UNLOCK(buff);
		PA_FREE(buff);
	}
	embed->border_horiz_space = FEUNITS_X(embed->border_horiz_space,
						context);

	/*
	 * See if we have some saved embed session data to restore.
	 */
	if (state->top_state->savedData.EmbedList != NULL)
	{
		lo_SavedEmbedListData *embed_list;
		embed_list = state->top_state->savedData.EmbedList;

		/*
		 * If there is still valid data to restore available.
		 */
		if (embed->embed_index < embed_list->embed_count)
		{
			lo_EmbedDataElement* embed_data_list;

			PA_LOCK(embed_data_list, lo_EmbedDataElement*,
				embed_list->embed_data_list);
			embed->session_data =
				embed_data_list[embed->embed_index].data;
			PA_UNLOCK(embed_list->embed_data_list);
		}
	}

	/*
	 * Have the front end fetch this embed data, and tell us
	 * the embed's dimensions if it knows them.
	 */
	FE_GetEmbedSize(context, embed, state->top_state->force_reload);

	/*
	 * Hidden embeds always have 0 width and height
	 */
	if (embed->extra_attrmask & LO_ELE_HIDDEN)
	{
		embed->width = 0;
		embed->height = 0;
	}

	/*
	 * We may have to block on this image until later
	 * when the front end can give us the width and height.
	 * Never block on hidden embeds.
	 */
	if (((embed->width == 0)||(embed->height == 0))&&
	    ((embed->extra_attrmask & LO_ELE_HIDDEN) == 0))
	{
		state->top_state->layout_blocking_element = (LO_Element *)embed;
	}
	else
	{
		lo_FinishEmbed(context, state, embed);
	}

    /* put the embed onto the embed list for later
     * possible reflection */
    embed->nextEmbed = state->top_state->embed_list;
    state->top_state->embed_list = embed;
}


void
lo_FinishEmbed(MWContext *context, lo_DocState *state, LO_EmbedStruct *embed)
{
	lo_TopState *top_state;
	PA_Block buff;
	char *str;
	Bool line_break;
	int32 baseline_inc;
	int32 line_inc;
	int32 embed_height, embed_width;
	LO_TextStruct tmp_text;
	LO_TextInfo text_info;

	top_state = state->top_state;

	/*
	 * All this work is to get the text_info filled in for the current
	 * font in the font stack. Yuck, there must be a better way.
	 */
	memset (&tmp_text, 0, sizeof (tmp_text));
	buff = PA_ALLOC(1);
	if (buff == NULL)
	{
		top_state->out_of_memory = TRUE;
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
	 * Hidden embeds are supposed to have 0 width and height.
	 */
	if ((embed->extra_attrmask & LO_ELE_HIDDEN) == 0)
	{
		if (embed->width == 0)
		{
			embed->width = EMBED_DEF_DIM;
		}
		if (embed->height == 0)
		{
			embed->height = EMBED_DEF_DIM;
		}
	}

	embed_width = embed->width + (2 * embed->border_width) +
		(2 * embed->border_horiz_space);
	embed_height = embed->height + (2 * embed->border_width) +
		(2 * embed->border_vert_space);

	/*
	 * SEVERE FLOW BREAK!  This may be a floating embed,
	 * which means at this point we go do something completely
	 * different.
	 */
	if (embed->ele_attrmask & LO_ELE_FLOATING)
	{
		if (embed->alignment == LO_ALIGN_RIGHT)
		{
			embed->x = state->right_margin - embed_width;
			if (embed->x < 0)
			{
				embed->x = 0;
			}
		}
		else
		{
			embed->x = state->left_margin;
		}

		embed->y = -1;
		embed->x_offset += (int16)embed->border_horiz_space;
		embed->y_offset += (int32)embed->border_vert_space;

		lo_AddMarginStack(state, embed->x, embed->y,
			embed->width, embed->height,
			embed->border_width,
			embed->border_vert_space, embed->border_horiz_space,
			(intn)embed->alignment);

		/*
		 * Insert this element into the float list.
		 */
		embed->next = state->float_list;
		state->float_list = (LO_Element *)embed;

		if (state->at_begin_line != FALSE)
		{
			lo_FindLineMargins(context, state);
			state->x = state->left_margin;
		}

	}
	else
	{
		/*
		 * Will this embed make the line too wide.
		 */
		if ((state->x + embed_width) > state->right_margin)
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
		 * break on the embed if we have
		 * a break.
		 */
		if (line_break != FALSE)
		{
			/*
			 * We need to make the elements sequential, linefeed
			 * before image.
			 */
			top_state->element_id = embed->ele_id;

			lo_HardLineBreak(context, state, TRUE);
			embed->x = state->x;
			embed->y = state->y;
			embed->ele_id = NEXT_ELEMENT;
		}

		/*
		 * Figure out how to align this embed.
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
			state->baseline = 0;
		}

		lo_CalcAlignOffsets(state, &text_info, (intn)embed->alignment,
			embed_width, embed_height,
			&embed->x_offset, &embed->y_offset, &line_inc, &baseline_inc);

		embed->x_offset += (int16)embed->border_horiz_space;
		embed->y_offset += (int32)embed->border_vert_space;

		lo_AppendToLineList(context, state,
			(LO_Element *)embed, baseline_inc);

		state->baseline += (intn) baseline_inc;
		state->line_height += (intn) (baseline_inc + line_inc);

		/*
		 * Clean up state
		 */
		state->x = state->x + embed->x_offset +
			embed_width - embed->border_horiz_space;
		state->linefeed_state = 0;
		state->at_begin_line = FALSE;
		state->trailing_space = FALSE;
		state->cur_ele_type = LO_IMAGE;
	}
}


void
LO_CopySavedEmbedData(MWContext *context, SHIST_SavedData *saved_data)
{
	int32 doc_id;
	lo_TopState *top_state;
	lo_DocState *state;
	lo_SavedEmbedListData *embed_list, *new_embed_list;
	lo_EmbedDataElement *embed_data_list, *new_embed_data_list;
	PA_Block old_embed_data_list = NULL;
	int32 old_count = 0;
	int32 index;

	/*
	 * Get the state, so we can access the savedEmbedListData
	 * list for this document.
	 */
	doc_id = XP_DOCID(context);
	top_state = lo_FetchTopState(doc_id);
	if ((top_state != NULL)&&(top_state->doc_state != NULL))
		state = top_state->doc_state;
	else
		return;

	embed_list = state->top_state->savedData.EmbedList;
	if (embed_list == NULL || embed_list->embed_data_list == NULL)
		return;
		
	
	PA_LOCK(embed_data_list, lo_EmbedDataElement*, embed_list->embed_data_list);

	if (saved_data->EmbedList == NULL)
		saved_data->EmbedList = XP_NEW_ZAP(lo_SavedEmbedListData);
	new_embed_list = saved_data->EmbedList;
	
	if (new_embed_list->embed_data_list == NULL)
	{
		/* Allocate new array */
		new_embed_list->embed_data_list = PA_ALLOC(
			embed_list->embed_count * sizeof(lo_EmbedDataElement));
	}
	else if (new_embed_list->embed_count < embed_list->embed_count)
	{	
		/* Enlarge existing array */
		old_embed_data_list = new_embed_list->embed_data_list;
		old_count = new_embed_list->embed_count;
		new_embed_list->embed_data_list = PA_REALLOC(
			new_embed_list->embed_data_list,
			(embed_list->embed_count * sizeof(lo_EmbedDataElement)));
	}

	/*
	 * If we ran out of memory to grow the array.
	 */
	if (new_embed_list->embed_data_list == NULL)
	{
		new_embed_list->embed_data_list = old_embed_data_list;
		new_embed_list->embed_count = old_count;
		state->top_state->out_of_memory = TRUE;
		return;
	}
	
	/*
	 * Copy all the elements from the old array to the new one.
	 */
	PA_LOCK(new_embed_data_list, lo_EmbedDataElement*, new_embed_list->embed_data_list);
	for (index = 0; index < embed_list->embed_count; index++)
		new_embed_data_list[index] = embed_data_list[index];
	new_embed_list->embed_count = embed_list->embed_count;
	PA_UNLOCK(new_embed_list->embed_data_list);
	
	PA_UNLOCK(embed_list->embed_data_list);
}

void
LO_AddEmbedData(MWContext *context, LO_EmbedStruct* embed, void *session_data)
{
	lo_AddEmbedData(context, session_data, NPL_DeleteSessionData, embed->embed_index);
}

