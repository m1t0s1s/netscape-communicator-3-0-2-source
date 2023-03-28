/* -*- Mode: C; tab-width: 4; -*-
 */
#include "xp.h"
#include "pa_parse.h"
#include "layout.h"
#include "glhist.h"
#include "il.h"
#include "java.h"
#include "libi18n.h"
#include "edt.h"

#ifdef MOCHA
#include "libmocha.h"
#endif

#ifdef XP_WIN16
#define SIZE_LIMIT 32000
#endif

#define	LIST_MARGIN_INC		(FEUNITS_X(40, context))

#define MIN_FONT_SIZE		1
#define MAX_FONT_SIZE		7

#define	HYPE_ANCHOR		"about:hype"
#define	HYPE_TAG_BECOMES	"SRC=internal-gopher-sound BORDER=0>"


/*************************************
 * Function: LO_ChangeFontSize
 *
 * Description: Utility function to change a size field based on a
 *	string which is either a new absolute value, or a relative
 *	change prefaced with + or -.  Also sanifies the result
 *	to a valid font size.
 *
 * Params: Original size, and change string.
 *
 * Returns: New size.
 *************************************/
intn
LO_ChangeFontSize(intn size, char *size_str)
{
	intn new_size;

	if ((size_str == NULL)||(*size_str == '\0'))
	{
		return(size);
	}

	if (*size_str == '+')
	{
		new_size = size + XP_ATOI((size_str + 1));
	}
	else if (*size_str == '-')
	{
		new_size = size - XP_ATOI((size_str + 1));
	}
	else
	{
		new_size = XP_ATOI(size_str);
	}

	if (new_size < MIN_FONT_SIZE)
	{
		new_size = MIN_FONT_SIZE;
	}
	if (new_size > MAX_FONT_SIZE)
	{
		new_size = MAX_FONT_SIZE;
	}

	return(new_size);
}


/*
 * Filter out tags we should ignore based on current state.
 * Return FALSE for tags we should ignore, and TRUE for
 * tags we should keep.
 */
Bool
lo_FilterTag(MWContext *context, lo_DocState *state, PA_Tag *tag)
{
	lo_TopState *top_state;

	top_state = state->top_state;
	if (top_state == NULL)
	{
		return(FALSE);
	}

	/*
	 * If we are in a GRID, all non-grid tags are discarded
	 * If we are in a NOGRIDS, all tags except its end are discarded.
	 */
	if (top_state->is_grid != FALSE)
	{
		if (tag->type == P_NOGRIDS)
		{
			if (tag->is_end == FALSE)
			{
				top_state->in_nogrids = TRUE;
			}
			else
			{
				top_state->in_nogrids = FALSE;
			}
			return(FALSE);
		}
		else if (top_state->in_nogrids != FALSE)
		{
			return(FALSE);
		}
		else if ((tag->type != P_GRID)&&
			 (tag->type != P_GRID_CELL))
		{
			return(FALSE);
		}
		return(TRUE);
	}

#ifdef JAVA
	/*
	 * If the user has disabled Java, act like we don't understand
	 * the APPLET tag.
	 */
	if (LJ_GetJavaEnabled() != FALSE)
	{
		/*
		 * APPLETs can appear anywhere.  Check for them and
		 * maintain proper in_applet state.
		 */
		if (tag->type == P_JAVA_APPLET)
		{
			if (tag->is_end == FALSE)
			{
				top_state->in_applet = TRUE;
				IL_UseDefaultColormapThisPage(context);
			}
			else
			{
				top_state->in_applet = FALSE;
			}
			return(TRUE);
		}

		/*
		 * Inside applet ignore everything except param and
		 * closing applets.
		 */
		if (top_state->in_applet != FALSE)
		{
			if (tag->type != P_PARAM)
			{
				return(FALSE);
			}
			return(TRUE);
		}
	}
#endif /* JAVA */

#ifdef MOCHA
	if (LM_GetMochaEnabled() != FALSE)
	{
		/*
		 * NOSCRIPT can appear anywhere.  Check for them and
		 * maintain proper in noscript state.
		 */
		if (tag->type == P_NOSCRIPT)
		{
			top_state->in_noscript = !tag->is_end;
			return(FALSE);
		}

		/*
		 * Inside NOSCRIPT ignore everything except the closing tag.
		 */
		if (top_state->in_noscript != FALSE)
		{
			return(FALSE);
		}
	}
#endif /* MOCHA */

/* UNIX can't do embed yet, so display the NOEMBED stuff */
#if !defined(XP_UNIX) || defined(UNIX_EMBED)
	/*
	 * NOEMBED can appear anywhere.  Check for them and maintain proper
	 * in noembed state.
	 */
	if (tag->type == P_NOEMBED)
	{
		if (tag->is_end == FALSE)
		{
			top_state->in_noembed = TRUE;
		}
		else
		{
			top_state->in_noembed = FALSE;
		}
		return(FALSE);
	}

	/*
	 * Inside NOEMBED ignore everything except the closing tag.
	 */
	if (top_state->in_noembed != FALSE)
	{
		return(FALSE);
	}
#endif /* !defined(XP_UNIX) || defined(UNIX_EMBED) */

	/*
	 * If we have an embed that passed the filter, we must use the
	 * default colormap.
	 */
	if (tag->type == P_EMBED)
	{
		IL_UseDefaultColormapThisPage(context);
	}

	return(TRUE);
}

static void
lo_CloseParagraph(MWContext *context, lo_DocState *state)
{
	/*
	 * If we are want to close a paragraph we are out of
	 * the HEAD section of the HTML and into the BODY
	 *
	 * With the exception of TITLE which can be in head, but tries to
	 * close paragraphs in case it was erroneously placed within body.
	 * This is taken care of in lo_process_title_tag()
	 */
	state->top_state->in_head = FALSE;
	state->top_state->in_body = TRUE;

	if (state->in_paragraph != FALSE)
	{
		if ((state->align_stack != NULL)&&
		    (state->align_stack->type == P_PARAGRAPH))
		{
			lo_AlignStack *aptr;

			lo_SoftLineBreak(context, state, FALSE, 1);

			aptr = lo_PopAlignment(state);
			if (aptr != NULL)
			{
				XP_DELETE(aptr);
			}
		}
	}
}


static void
lo_OpenHeader(MWContext *context, lo_DocState *state, PA_Tag *tag)
{
	PA_Block buff;
	char *str;

	buff = lo_FetchParamValue(context, tag, PARAM_ALIGN);
	if (buff != NULL)
	{
		int32 alignment;

		PA_LOCK(str, char *, buff);
		alignment = (int32)lo_EvalDivisionAlignParam(str);
		PA_UNLOCK(buff);
		PA_FREE(buff);

		lo_PushAlignment(state, tag, alignment);
	}
}


static void
lo_CloseHeader(MWContext *context, lo_DocState *state)
{
	Bool aligned_header;

	/*
	 * Check the top of the alignment stack to see if we are closing
	 * a header that explicitly set an alignment.
	 */
	aligned_header = FALSE;
	if (state->align_stack != NULL)
	{
		intn type;

		type = state->align_stack->type;
		if ((type == P_HEADER_1)||(type == P_HEADER_2)||
		    (type == P_HEADER_3)||(type == P_HEADER_4)||
		    (type == P_HEADER_5)||(type == P_HEADER_6))
		{
			aligned_header = TRUE;
		}
	}

	if (aligned_header != FALSE)
	{
		lo_AlignStack *aptr;

		lo_SoftLineBreak(context, state, FALSE, 1);

		aptr = lo_PopAlignment(state);
		if (aptr != NULL)
		{
			XP_DELETE(aptr);
		}
	}
	else
	{
		lo_SoftLineBreak(context, state, FALSE, 1);
	}
	/*
	 * Now that we are on the line after the header, we
	 * don't want the next blank line to be the height of the probably
	 * large header, so reset it to the default here.
	 */
	state->line_height = state->default_line_height;
}


static void
lo_process_text_tag(MWContext *context, lo_DocState *state, PA_Tag *tag)
{
	char *tptr;
	char *tptr2;
	int16 charset;
	INTL_FontEncodingList *fe;
	INTL_FontEncodingList *origFE;
	void (*formatFunc)(MWContext *, lo_DocState *, char *);

	/*
	 * This is normal text, formatted or preformatted.
	 */
	if ((state->text_divert == P_UNKNOWN)||(state->text_divert == P_TEXT))
	{
		/*
		 * If you don't have a current text element
		 * to add this text to, make one here.
		 */
		if (state->cur_ele_type != LO_TEXT)
		{
			lo_FreshText(state);
			state->cur_ele_type = LO_TEXT;
		}

		PA_LOCK (tptr, char *, tag->data);

		/*
		 * If we are processing non-head-data text, and it is
		 * not just white space, then we are out of
		 * the HEAD section of the HTML.
		 *
		 * If the text is inside an unknown element in the HEAD
		 * we can assume this is content we should ignore.
		 */
		if (state->top_state->in_head != FALSE)
		{
			char *cptr;
			Bool real_text;

			real_text = FALSE;
			cptr = tptr;
			while (*cptr != '\0')
			{
				if (!XP_IS_SPACE(*cptr))
				{
					real_text = TRUE;
					break;
				}
				cptr++;
			}
			if (real_text != FALSE)
			{
				if (state->top_state->unknown_head_tag != NULL)
				{
#ifdef DEBUG
XP_TRACE(("Toss out content in unknown HEAD (%s)\n", tptr));
#endif /* DEBUG */
					return;
				}
				/*
				 * If the content isnot ignore, we have left
				 * the HEAD and are in the BODY.
				 */
				state->top_state->in_head = FALSE;
				state->top_state->in_body = TRUE;
			}
		}

		if (state->preformatted != PRE_TEXT_NO)
		{
			formatFunc = lo_PreformatedText;
		}
		else
		{
			formatFunc = lo_FormatText;
		}
		charset = state->font_stack->text_attr->charset;
		origFE = INTL_InternalToFontEncoding(charset, (unsigned char *) tptr);
		fe = origFE;
		while (fe)
		{
			state->font_stack->text_attr->charset = fe->charset;
			(*formatFunc)(context, state, (char *) fe->text);
			fe = fe->next;
		}
		state->font_stack->text_attr->charset = charset;
		if (origFE)
		{
			INTL_FreeFontEncodingList(origFE);
		}
		PA_UNLOCK(tag->data);
	}
	/*
	 * These tags just capture the text flow as is.
	 */
	else if ((state->text_divert == P_TITLE)||
		(state->text_divert == P_TEXTAREA)||
		(state->text_divert == P_OPTION)||
		(state->text_divert == P_CERTIFICATE)
#ifdef MOCHA /* added by jwz */
			 || (state->text_divert == P_SCRIPT)
#endif
			 )
	{
		int32 copy_len;

		if ((state->line_buf_len + tag->data_len + 1) >
			state->line_buf_size)
		{
			int32 new_size;

			new_size = state->line_buf_size +
				tag->data_len + LINE_BUF_INC;
#ifdef XP_WIN16
			if (new_size > SIZE_LIMIT)
			{
				new_size = SIZE_LIMIT;
			}
#endif /* XP_WIN16 */
			state->line_buf = PA_REALLOC(state->line_buf, new_size);
			if (state->line_buf == NULL)
			{
				state->top_state->out_of_memory = TRUE;
				return;
			}
			state->line_buf_size = new_size;
		}

		copy_len = tag->data_len;
#ifdef XP_WIN16
		if ((state->line_buf_len + copy_len) >
			(state->line_buf_size - 1))
		{
			copy_len = state->line_buf_size - 1 -
				state->line_buf_len;
		}
#endif /* XP_WIN16 */

		PA_LOCK(tptr, char *, tag->data);
		PA_LOCK(tptr2, char *, state->line_buf);
		XP_BCOPY(tptr, (char *)(tptr2 + state->line_buf_len),
			(copy_len + 1));
		state->line_buf_len += copy_len;
#ifdef XP_WIN16
		tptr2[state->line_buf_len] = '\0';
#endif /* XP_WIN16 */
		PA_UNLOCK(state->line_buf);
		PA_UNLOCK(tag->data);
	}
	else /* Just ignore this text, it shouldn't be here */
	{
	}
}


static void
lo_process_title_tag(MWContext *context, lo_DocState *state, PA_Tag *tag)
{
	Bool tmp_in_head;
	Bool tmp_in_body;

	/*
	 * Don't let closing a paragraph before a title change the
	 * in_head or in_body state.
	 */
	tmp_in_head = state->top_state->in_head;
	tmp_in_body = state->top_state->in_body;
	if (state->in_paragraph != FALSE)
	{
		lo_CloseParagraph(context, state);
	}
	state->top_state->in_head = tmp_in_head;
	state->top_state->in_body = tmp_in_body;

	if (tag->is_end == FALSE)
	{
		/*
		 * Flush the line buffer so we can
		 * start storing the title text there.
		 */
		if ((state->cur_ele_type == LO_TEXT)&&
			(state->line_buf_len != 0))
		{
			lo_FlushLineBuffer(context, state);
		}

		state->line_buf_len = 0;
		state->text_divert = tag->type;
	}
	else
	{
	    /*
	     * Only if we captured some title text
	     */
	    if (state->line_buf_len != 0)
	    {
		int32 len;
		char *tptr2;

		PA_LOCK(tptr2, char *, state->line_buf);
		/*
		 * Title text is cleaned up before passing
		 * to the front-end.
		 */
		len = lo_CleanTextWhitespace(tptr2, state->line_buf_len);
		/*
		 * Subdocs can't have titles.
		 */
		if ((state->is_a_subdoc == SUBDOC_NOT)&&
			(state->top_state->have_title == FALSE))
		{
			int16 charset;

			charset = state->font_stack->text_attr->charset;
			tptr2 = FE_TranslateISOText(context, charset, tptr2);
#ifdef MAXTITLESIZE
			if ((XP_STRLEN(tptr2) > MAXTITLESIZE)&&
			    (MAXTITLESIZE >= 0))
			{
				tptr2[MAXTITLESIZE] = '\0';
			}
#endif
			FE_SetDocTitle(context, tptr2);
			state->top_state->have_title = TRUE;
		}
		PA_UNLOCK(state->line_buf);
	    }
	    state->line_buf_len = 0;
	    state->text_divert = P_UNKNOWN;
	}
}


static void
lo_process_anchor_tag(MWContext *context, lo_DocState *state, PA_Tag *tag)
{
	LO_TextAttr tmp_attr;

	/*
	 * Opening a new anchor
	 */
	if (tag->is_end == FALSE)
	{
		LO_TextAttr *old_attr;
		LO_TextAttr *attr;
		char *url;
		char *full_url;
		PA_Block buff;
		Bool is_new;
		PA_Block anchor;

		/*
		 * Get the NAME param for the name list
		 */
		buff = lo_FetchParamValue(context, tag, PARAM_NAME);
		if (buff != NULL)
		{
		    char *name;

		    /*
		     * If we are looking to start at a name,
		     * fetch the NAME and compare.
		     */
		    if (state->display_blocking_element_id == -1)
		    {
			PA_LOCK(name, char *, buff);
			if (XP_STRCMP(name,
			    (char *)(state->top_state->name_target + 1))
				== 0)
			{
			    XP_FREE(state->top_state->name_target);
			    state->top_state->name_target = NULL;
			    state->display_blocking_element_id =
				state->top_state->element_id;
			}
			PA_UNLOCK(buff);
		    }

		    if (state->current_named_anchor != NULL)
		    {
			if (lo_SetNamedAnchor(state,
			    state->current_named_anchor, NULL))
			{
#ifdef MOCHA
			    lo_ReflectNamedAnchor(context, state, tag,
						  state->name_list);
#endif
			}
		    }
		    state->current_named_anchor = buff;
		}

		is_new = TRUE;
		state->current_anchor = NULL;
		/*
		 * Fetch the HREF, and turn it into an
		 * absolute URL.
		 */
		anchor = lo_FetchParamValue(context, tag, PARAM_HREF);
		if (anchor != NULL)
		{
		    char *target;
		    PA_Block targ_buff;

		    PA_LOCK(url, char *, anchor);
		    if (url != NULL)
		    {
			int32 len;

			len = lo_StripTextWhitespace(url, XP_STRLEN(url));
		    }
		    url = NET_MakeAbsoluteURL(state->top_state->base_url, url);
		    if (url == NULL)
		    {
			buff = NULL;
		    }
		    else
		    {
			buff = PA_ALLOC(XP_STRLEN(url)+1);
			if (buff != NULL)
			{
			    PA_LOCK(full_url, char *, buff);
			    XP_STRCPY(full_url, url);
			    PA_UNLOCK(buff);

			    if (GH_CheckGlobalHistory(url) != -1)
			    {
				is_new = FALSE;
			    }
			}
			else
			{
#ifdef DEBUG
				assert (state);
#endif
				state->top_state->out_of_memory = TRUE;
			}
			
			XP_FREE(url);
		    }
		    PA_UNLOCK(anchor);
		    PA_FREE(anchor);
		    anchor = buff;

		    if (anchor != NULL)
		    {
			targ_buff = lo_FetchParamValue(context, tag, PARAM_TARGET);
			if (targ_buff != NULL)
			{
			    int32 len;

			    PA_LOCK(target, char *, targ_buff);
			    len = lo_StripTextWhitespace(target,
					XP_STRLEN(target));
			    if ((*target == '\0')||
				    (lo_IsValidTarget(target) == FALSE))
			    {
				PA_UNLOCK(targ_buff);
				PA_FREE(targ_buff);
				targ_buff = NULL;
			    }
			    else
			    {
				PA_UNLOCK(targ_buff);
			    }
			}

			/*
			 * If there was no target use the default one.
			 * (default provided by BASE tag)
			 */
			if ((targ_buff == NULL)&&
			    (state->top_state->base_target != NULL))
			{
				targ_buff = PA_ALLOC(XP_STRLEN(
					state->top_state->base_target) + 1);
				if (targ_buff != NULL)
				{
					char *targ;

					PA_LOCK(targ, char *, targ_buff);
					XP_STRCPY(targ,
						state->top_state->base_target);
					PA_UNLOCK(targ_buff);
				}
				else
				{
					state->top_state->out_of_memory = TRUE;
				}
			}

			state->current_anchor =
				lo_NewAnchor(state, anchor, targ_buff);
			if (state->current_anchor == NULL)
			{
			    PA_FREE(anchor);
			    if (targ_buff != NULL)
			    {
				PA_FREE(targ_buff);
			    }
			}
		    }

		    /*
		     * Add this url's block to the list
		     * of all allocated urls so we can free
		     * it later.
		     */
		    lo_AddToUrlList(context, state, state->current_anchor);
		    if (state->top_state->out_of_memory != FALSE)
		    {
			return;
		    }
#ifdef MOCHA
		    lo_ReflectLink(context, state, tag, state->current_anchor,
				   state->top_state->url_list_len - 1);
#endif
		}

		/*
		 * Change the anchor attribute only for
		 * anchors with hrefs.
		 */
		if (state->current_anchor != NULL)
		{
			old_attr = state->font_stack->text_attr;
			lo_CopyTextAttr(old_attr, &tmp_attr);

			tmp_attr.attrmask = (old_attr->attrmask|
				LO_ATTR_ANCHOR);
			if (is_new != FALSE)
			{
				tmp_attr.fg.red =   STATE_UNVISITED_ANCHOR_RED(state);
				tmp_attr.fg.green = STATE_UNVISITED_ANCHOR_GREEN(state);
				tmp_attr.fg.blue =  STATE_UNVISITED_ANCHOR_BLUE(state);
			}
			else
			{
				tmp_attr.fg.red =   STATE_VISITED_ANCHOR_RED(state);
				tmp_attr.fg.green = STATE_VISITED_ANCHOR_GREEN(state);
				tmp_attr.fg.blue =  STATE_VISITED_ANCHOR_BLUE(state);
			}
			attr = lo_FetchTextAttr(state, &tmp_attr);
			lo_PushFont(state, tag->type, attr);
		}
	}
	/*
	 * Closing an anchor.  Anchors can't be nested, so close
	 * ALL anchors.
	 */
	else
	{
		lo_PopAllAnchors(state);
		state->current_anchor = NULL;
	}
}


static void
lo_process_caption_tag(MWContext *context, lo_DocState *state, PA_Tag *tag)
{
	if ((state->current_table != NULL)&&
	    (state->current_table->caption == NULL))
	{
	    lo_TableRec *table;

	    table = state->current_table;
	    if (tag->is_end == FALSE)
	    {
		/*
		 * If there is an unterminated row open,
		 * terminate it now.
		 */
		if ((table->row_ptr != NULL)&&
			(table->row_ptr->row_done == FALSE))
		{
			lo_EndTableRow(context, state, table);
		}
		lo_BeginTableCaption(context, state, table,tag);
	    }
	    else
	    {
		/*
		 * This never happens because the
		 * close caption feeds into the subdoc.
		 */
	    }
	}
	else if ((state->current_table == NULL)&&
		(state->is_a_subdoc == SUBDOC_CAPTION))
	{
	    if (tag->is_end == FALSE)
	    {
	    }
	    else
	    {
		lo_EndTableCaption(context, state);
	    }
	}
	else if ((state->current_table == NULL)&&
		(state->is_a_subdoc == SUBDOC_CELL))
	{
	    if (tag->is_end == FALSE)
	    {
		lo_TopState *top_state;
		lo_DocState *new_state;
		int32 doc_id;

		/*
		 * This can only happen if the close cell
		 * was omitted, AND the close row
		 * after it was omitted.
		 */
		lo_EndTableCell(context, state);

		/*
		 * restore state to the new lowest level.
		 */
		doc_id = XP_DOCID(context);
		top_state = lo_FetchTopState(doc_id);
		new_state = lo_TopSubState(top_state);

		/*
		 * Now act just like a new caption has started.
		 */
		if (new_state->current_table != NULL)
		{
		    lo_TableRec *table;

		    table = new_state->current_table;
		    /*
		     * If there is an unterminated row open,
		     * terminate it now.
		     */
		    if ((table->row_ptr != NULL)&&
			(table->row_ptr->row_done == FALSE))
		    {
			lo_EndTableRow(context,new_state,table);
		    }
		    lo_BeginTableCaption(context, new_state,
					table, tag);
		}
	    }
	}
}

static void
lo_process_table_cell_tag(MWContext *context, lo_DocState *state, PA_Tag *tag)
{
	if ((state->current_table != NULL)&&
	    (state->current_table->row_ptr != NULL)&&
	    (state->current_table->row_ptr->row_done == FALSE))
	{
	    lo_TableRec *table;
	    Bool is_a_header;

	    if (tag->type == P_TABLE_HEADER)
	    {
		is_a_header = TRUE;
	    }
	    else
	    {
		is_a_header = FALSE;
	    }

	    table = state->current_table;
	    if (tag->is_end == FALSE)
	    {
		lo_BeginTableCell(context, state, table, tag, is_a_header);
	    }
	    else
	    {
		/*
		 * This never happens because the
		 * close cell feeds into the subdoc.
		 */
	    }
	}
	else if ((state->current_table != NULL)&&
	    ((state->current_table->row_ptr == NULL)||
	    (state->current_table->row_ptr->row_done != FALSE)))
	{
	    lo_TableRec *table;
	    Bool is_a_header;

	    if (tag->is_end == FALSE)
	    {
		table = state->current_table;
		/*
		 * This is a starting data cell, with no open
		 * row.  Open up the row for them.
		 */
		lo_BeginTableRow(context, state, table, tag);

		if (tag->type == P_TABLE_HEADER)
		{
		    is_a_header = TRUE;
		}
		else
		{
		    is_a_header = FALSE;
		}

		lo_BeginTableCell(context, state, table, tag, is_a_header);
	    }
	}
	else if ((state->current_table == NULL)&&
		(state->is_a_subdoc == SUBDOC_CELL))
	{
	    if (tag->is_end == FALSE)
	    {
		lo_TopState *top_state;
		lo_DocState *new_state;
		int32 doc_id;

		/*
		 * This can only happen if the close cell
		 * was omitted.
		 */
		lo_EndTableCell(context, state);

		/*
		 * restore state to the new lowest level.
		 */
		doc_id = XP_DOCID(context);
		top_state = lo_FetchTopState(doc_id);
		new_state = lo_TopSubState(top_state);

		/*
		 * Now act just like a new cell has started
		 * as above.
		 */
		if ((new_state->current_table != NULL)&&
		    (new_state->current_table->row_ptr != NULL)&&
		    (new_state->current_table->row_ptr->row_done == FALSE))
		{
		    lo_TableRec *table;
		    Bool is_a_header;

		    if (tag->type == P_TABLE_HEADER)
		    {
			is_a_header = TRUE;
		    }
		    else
		    {
			is_a_header = FALSE;
		    }

		    table = new_state->current_table;
		    lo_BeginTableCell(context, new_state, table, tag,
				is_a_header);
		}
	    }
	    else
	    {
		lo_EndTableCell(context, state);
	    }
	}
}


static void
lo_process_table_row_tag(MWContext *context, lo_DocState *state, PA_Tag *tag)
{
	if (state->current_table != NULL)
	{
	    lo_TableRec *table;

	    table = state->current_table;
	    if (tag->is_end == FALSE)
	    {
		/*
		 * If there is an unterminated row open,
		 * terminate it now.
		 */
		if ((table->row_ptr != NULL)&&
			(table->row_ptr->row_done == FALSE))
		{
			lo_EndTableRow(context, state, table);
		}
		lo_BeginTableRow(context, state, table, tag);
	    }
	    else
	    {
		if ((table->row_ptr != NULL)&&
			(table->row_ptr->row_done == FALSE))
		{
			lo_EndTableRow(context, state, table);
		}
	    }
	}
	else if ((state->current_table == NULL)&&
		(state->is_a_subdoc == SUBDOC_CELL))
	{
	    if (tag->is_end == FALSE)
	    {
		lo_TopState *top_state;
		lo_DocState *new_state;
		int32 doc_id;

		/*
		 * This can only happen if the close cell
		 * was omitted, AND the close row
		 * after it was omitted.
		 */
		lo_EndTableCell(context, state);

		/*
		 * restore state to the new lowest level.
		 */
		doc_id = XP_DOCID(context);
		top_state = lo_FetchTopState(doc_id);
		new_state = lo_TopSubState(top_state);

		/*
		 * Now act just like a new row has started.
		 */
		if (new_state->current_table != NULL)
		{
		    lo_TableRec *table;

		    table = new_state->current_table;
		    /*
		     * If there is an unterminated row open,
		     * terminate it now.
		     */
		    if ((table->row_ptr != NULL)&&
			(table->row_ptr->row_done == FALSE))
		    {
			lo_EndTableRow(context, new_state, table);
		    }
		    lo_BeginTableRow(context, new_state, table, tag);
		}
	    }
	    else
	    {
		/*
		 * This can only happen if the close cell
		 * was omitted.
		 */
		lo_EndTableCell(context, state);

		/*
		 * We should really go on to repeat all the code
		 * here to process the end row tag, but since
		 * we know our layout can recover fine without
		 * it, we don't bother.
		 */
	    }
	}
	else if ((state->current_table == NULL)&&
		(state->is_a_subdoc == SUBDOC_CAPTION))
	{
	    if (tag->is_end == FALSE)
	    {
		lo_TopState *top_state;
		lo_DocState *new_state;
		int32 doc_id;

		/*
		 * This can only happen if the close caption
		 * was omitted.
		 */
		lo_EndTableCaption(context, state);

		/*
		 * restore state to the new lowest level.
		 */
		doc_id = XP_DOCID(context);
		top_state = lo_FetchTopState(doc_id);
		new_state = lo_TopSubState(top_state);

		/*
		 * Now act just like a new row has started.
		 */
		if (new_state->current_table != NULL)
		{
		    lo_TableRec *table;

		    table = new_state->current_table;
		    /*
		     * If there is an unterminated row open,
		     * terminate it now.
		     */
		    if ((table->row_ptr != NULL)&&
			(table->row_ptr->row_done == FALSE))
		    {
			lo_EndTableRow(context, new_state, table);
		    }
		    lo_BeginTableRow(context, new_state, table, tag);
		}
	    }
	}
}


static void
lo_close_table(MWContext *context, lo_DocState *state)
{
	if (state->current_table != NULL)
	{
		lo_TableRec *table;

		table = state->current_table;
		/*
		 * If there is an unterminated row open,
		 * terminate it now.
		 */
		if ((table->row_ptr != NULL)&&
		    (table->row_ptr->row_done == FALSE))
		{
			lo_EndTableRow(context, state, table);
		}
		lo_EndTable(context, state, state->current_table);
	}
	else if ((state->current_table == NULL)&&
		(state->is_a_subdoc == SUBDOC_CELL))
	{
		lo_TopState *top_state;
		lo_DocState *new_state;
		int32 doc_id;

		/*
		 * This can only happen if the close cell
		 * was omitted, AND the close row
		 * after it was omitted.
		 */
		lo_EndTableCell(context, state);

		/*
		 * restore state to the new lowest level.
		 */
		doc_id = XP_DOCID(context);
		top_state = lo_FetchTopState(doc_id);
		new_state = lo_TopSubState(top_state);

		/*
		 * Now act like a close table with
		 * an omitted close row.
		 */
		if (new_state->current_table != NULL)
		{
			lo_TableRec *table;

			table = new_state->current_table;
			/*
			 * If there is an unterminated row open,
			 * terminate it now.
			 */
			if ((table->row_ptr != NULL)&&
			    (table->row_ptr->row_done == FALSE))
			{
				lo_EndTableRow(context, new_state, table);
			}
			lo_EndTable(context, new_state,
				new_state->current_table);
		}
	}
	else if ((state->current_table == NULL)&&
		(state->is_a_subdoc == SUBDOC_CAPTION))
	{
		lo_TopState *top_state;
		lo_DocState *new_state;
		int32 doc_id;

		/*
		 * This can only happen if the close
		 * caption was omitted.
		 */
		lo_EndTableCaption(context, state);

		/*
		 * restore state to the new lowest level.
		 */
		doc_id = XP_DOCID(context);
		top_state = lo_FetchTopState(doc_id);
		new_state = lo_TopSubState(top_state);

		/*
		 * Now act like a close table.
		 */
		if (new_state->current_table != NULL)
		{
			lo_TableRec *table;

			table = new_state->current_table;
			/*
			 * If there is an unterminated row open,
			 * terminate it now.
			 */
			if ((table->row_ptr != NULL)&&
			    (table->row_ptr->row_done == FALSE))
			{
				lo_EndTableRow(context, new_state, table);
			}
			lo_EndTable(context, new_state,
				new_state->current_table);
		}
	}
}


static void
lo_process_table_tag(MWContext *context, lo_DocState *state, PA_Tag *tag)
{
	if (state->in_paragraph != FALSE)
	{
		lo_CloseParagraph(context, state);
	}
	if (tag->is_end == FALSE)
	{
		lo_BeginTable(context, state, tag);
	}
	else
	{
		lo_close_table(context, state);
	}
}


void
lo_CloseOutTable(MWContext *context, lo_DocState *state)
{
	lo_close_table(context, state);
}


#ifdef HTML_CERTIFICATE_SUPPORT

static void
lo_process_certificate_tag(MWContext *context, lo_DocState *state, PA_Tag *tag)
{
	Bool tmp_in_head;
	lo_Certificate *lo_cert;

	/*
	 * Do not let closing a paragraph before a cert change the
	 * in_head state.
	 */
	tmp_in_head = state->top_state->in_head;
	if (state->in_paragraph != FALSE)
	{
		lo_CloseParagraph(context, state);
	}
	state->top_state->in_head = tmp_in_head;

	if (tag->is_end == FALSE)
	{
		/*
		 * Flush the line buffer so we can start
		 * storing the certificate data there.
		 */
		if ((state->cur_ele_type == LO_TEXT)&&
		    (state->line_buf_len != 0))
		{
			lo_FlushLineBuffer(context, state);
		}

		state->line_buf_len = 0;

		lo_cert = XP_NEW(lo_Certificate);
		if (lo_cert == NULL)
		{
			state->top_state->out_of_memory = TRUE;
			return;
		}

		lo_cert->cert = NULL;
		lo_cert->name = lo_FetchParamValue(context, tag, PARAM_NAME);

		lo_cert->next = state->top_state->cert_list;
		state->top_state->cert_list = lo_cert;

		state->text_divert = tag->type;
		return;
	}

	/*
	 * Get this out of the way so we can just return if we hit
	 * errors below.
	 */
	state->text_divert = P_UNKNOWN;

	/*
	 * Only if we captured a certificate
	 */
	if (state->line_buf_len != 0)
	{
		char *tptr;

		state->line_buf_len = 0;

		lo_cert = state->top_state->cert_list;
		if (lo_cert == NULL)
			return;

		PA_LOCK(tptr, char *, state->line_buf);
		lo_cert->cert = XP_STRDUP(tptr);
		PA_UNLOCK(state->line_buf);

		if (lo_cert->cert == NULL)
		{
			state->top_state->cert_list = lo_cert->next;
			PA_FREE(lo_cert->name);
			XP_FREE(lo_cert);
		}
	}
}

#endif /* HTML_CERTIFICATE_SUPPORT */


/*************************************
 * Function: lo_LayoutTag
 *
 * Description: This function begins the process of laying
 *	out a single MDL tag.
 *
 * Params: Window context and document state, and the tag to be laid out.
 *
 * Returns: Nothing
 *************************************/
void
lo_LayoutTag(MWContext *context, lo_DocState *state, PA_Tag *tag)
{
	char *tptr;
	char *tptr2;
	LO_TextAttr tmp_attr;

	XP_ASSERT(state);

#ifdef LOCAL_DEBUG
XP_TRACE(("lo_LayoutTag(%d)\n", tag->type));
#endif /* LOCAL_DEBUG */

	if (tag->data != NULL)
	{
		state->top_state->layout_bytes += tag->true_len;
	}

#ifdef MOCHA_CACHES_WRITES
	if (state->in_relayout == FALSE)
	{
		NET_StreamClass *stream = state->top_state->mocha_write_stream;

		if ((stream != NULL) &&
			(state->top_state->in_script == SCRIPT_TYPE_NOT))
		{
			switch (tag->type)
			{
				case P_TEXT:
					if (tag->data != NULL)
					{
						stream->put_block(stream->data_object,
										  (char *)tag->data,
										  tag->data_len);
					}
					break;

				case P_UNKNOWN:
					if (tag->data != NULL)
					{
						char *tag_str = PR_smprintf("<%s%s",
													tag->is_end ? "/" : "",
													tag->data);

						if (tag_str == NULL)
						{
							state->top_state->out_of_memory = TRUE;
						}
						else
						{
							stream->put_block(stream->data_object, tag_str,
											  XP_STRLEN(tag_str));
							XP_FREE(tag_str);
						}
					}
					break;

#ifdef MOCHA /* added by jwz */
				case P_SCRIPT:
					break;
#endif /* MOCHA -- added by jwz */

				default:
					{
						const char *tag_name;
						char *tag_str;

						tag_name = pa_PrintTagToken(tag->type);
						if ((tag->data != NULL) &&
							(*(char *)tag->data != '>'))
						{
							tag_str = PR_smprintf("<%s%s%s",
												  tag->is_end ? "/" : "",
												  tag_name, tag->data);
						}
						else
						{
							tag_str = PR_smprintf("<%s%s>",
												  tag->is_end ? "/" : "",
												  tag_name);
						}
						if (tag_str == NULL)
						{
							state->top_state->out_of_memory = TRUE;
						}
						else
						{
							stream->put_block(stream->data_object, tag_str,
											  XP_STRLEN(tag_str));
							XP_FREE(tag_str);
						}
					}
					break;
			}
		}
	}
#endif /* MOCHA_CACHES_WRITES */

	/*
	 * If we get a non-text tag, and we have some previous text
	 * elements we have been merging into state, flush those
	 * before processing the new tag.
	 * Exceptions: P_TITLE does special stuff with title text.
	 * Exceptions: P_TEXTAREA does special stuff with text.
	 * Exceptions: P_OPTION does special stuff with text.
	 * Exceptions: P_SELECT does special stuff with option text on close.
	 * Exceptions: P_SCRIPT does special stuff with Mocha script text.
	 * Exceptions: P_CERTIFICATE does special stuff with text.
	 * Exceptions: P_BASEFONT may not change text flow.
	 * Exceptions: P_BASE only affects URLs not text flow.
	 */
	if ((tag->type != P_TEXT)&&(tag->type != P_TITLE)&&
		(tag->type != P_BASEFONT)&&
		(tag->type != P_TEXTAREA)&&
		(tag->type != P_OPTION)&&
		(tag->type != P_SELECT)&&
#ifdef MOCHA /* added by jwz */
		(tag->type != P_SCRIPT)&&
#endif
		(tag->type != P_CERTIFICATE)&&
		(state->cur_ele_type == LO_TEXT)&&(state->line_buf_len != 0))
	{
		lo_FlushLineBuffer(context, state);
		if (state->top_state->out_of_memory)
		{
			return;
		}
	}

#ifdef EDITOR
	/*  
	 * The editor needs all text elements to be individual text elements.
	 * Force the line buffer to be flushed so each individual text item
	 * remains such.
	 */
	if (EDT_IS_EDITOR(context)&& 
		(state->cur_ele_type == LO_TEXT)&&(state->line_buf_len != 0))
	{
		lo_FlushLineBuffer(context, state);
		if (state->top_state->out_of_memory)
		{
			return;
		}
	}

	/* LTNOTE:
	 *	This is a kludge.  We should allow all Layout elements to have
	 *	pointers to edit elements.  As should also be setting the
	 *	edit_element at the point we actually parse the tag.
	 */

	if( !state->edit_force_offset )
	{
		state->edit_current_offset = 0;
	}

	if( tag->type == P_TEXT 
			|| tag->type == P_IMAGE 
			|| tag->type == P_HRULE
			|| tag->type == P_LINEBREAK
			)
	{
		state->edit_current_element = tag->edit_element;
		if( tag->type != P_TEXT )
		{
			state->edit_current_offset = 0;
		}
	}
	else 
	{
		state->edit_current_element = 0;
	}
#endif

	switch(tag->type)
	{
		case P_TEXT:
			lo_process_text_tag(context, state, tag);
			break;

		/*
		 * Title text is special, it is not displayed, just
		 * captured and sent to the front end.
		 */
		case P_TITLE:
			lo_process_title_tag(context, state, tag);
			break;

		/*
		 * Certificate text is special, it is not displayed,
		 * but decoded and saved for later reference.
		 */
		case P_CERTIFICATE:
#ifdef HTML_CERTIFICATE_SUPPORT
			lo_process_certificate_tag(context, state, tag);
#endif
			break;

		/*
		 * All these tags just flip the fixed width font
		 * bit, and push a new font on the font stack.
		 */
		case P_FIXED:
		case P_CODE:
		case P_SAMPLE:
		case P_KEYBOARD:
			if (tag->is_end == FALSE)
			{
				LO_TextAttr *old_attr;
				LO_TextAttr *attr;

				old_attr = state->font_stack->text_attr;
				lo_CopyTextAttr(old_attr, &tmp_attr);
				tmp_attr.fontmask |= LO_FONT_FIXED;
				attr = lo_FetchTextAttr(state, &tmp_attr);

				lo_PushFont(state, tag->type, attr);
			}
			else
			{
				LO_TextAttr *attr;

				attr = lo_PopFont(state, tag->type);
			}

			break;

#ifdef EDITOR
		process_server:
		case P_SERVER:
			if (tag->is_end == FALSE)
			{
				LO_TextAttr *old_attr;
				LO_TextAttr *attr;

				state->preformatted = PRE_TEXT_YES;
				FE_BeginPreSection(context);

				old_attr = state->font_stack->text_attr;
				lo_CopyTextAttr(old_attr, &tmp_attr);
				tmp_attr.fontmask |= LO_FONT_FIXED;
				if( tag->type == P_SCRIPT ){
					tmp_attr.fg.green = 0;
				    tmp_attr.fg.red = 0xff;
				    tmp_attr.fg.blue = 0;
				}
				else {
					tmp_attr.fg.green = 0;
				    tmp_attr.fg.red = 0;
				    tmp_attr.fg.blue = 0xff;
				}
				tmp_attr.size = 3;
				attr = lo_FetchTextAttr(state, &tmp_attr);

				lo_PushFont(state, tag->type, attr);
			}
			else
			{
				LO_TextAttr *attr;

				attr = lo_PopFont(state, tag->type);
				state->preformatted = PRE_TEXT_NO;
				FE_EndPreSection(context);
			}

			break;
#endif


		/*
		 * All these tags just flip the bold font
		 * bit, and push a new font on the font stack.
		 */
		case P_BOLD:
		case P_STRONG:
			if (tag->is_end == FALSE)
			{
				LO_TextAttr *old_attr;
				LO_TextAttr *attr;

				old_attr = state->font_stack->text_attr;
				lo_CopyTextAttr(old_attr, &tmp_attr);
				tmp_attr.fontmask |= LO_FONT_BOLD;
				attr = lo_FetchTextAttr(state, &tmp_attr);

				lo_PushFont(state, tag->type, attr);
			}
			else
			{
				LO_TextAttr *attr;

				attr = lo_PopFont(state, tag->type);
			}
			break;

		/*
		 * All these tags just flip the italic font
		 * bit, and push a new font on the font stack.
		 */
		case P_ITALIC:
		case P_EMPHASIZED:
		case P_VARIABLE:
		case P_CITATION:
			if (tag->is_end == FALSE)
			{
				LO_TextAttr *old_attr;
				LO_TextAttr *attr;

				old_attr = state->font_stack->text_attr;
				lo_CopyTextAttr(old_attr, &tmp_attr);
				tmp_attr.fontmask |= LO_FONT_ITALIC;
				attr = lo_FetchTextAttr(state, &tmp_attr);

				lo_PushFont(state, tag->type, attr);
			}
			else
			{
				LO_TextAttr *attr;

				attr = lo_PopFont(state, tag->type);
			}
			break;

		/*
		 * All headers force whitespace, then set the bold
		 * font bit and unset the fixed and/or italic if set.
		 * In addition, they set the size attribute of the
		 * new font they will push, like <font size=num>
		 * Sizes are:
		 * 	H1 -> 6
		 * 	H2 -> 5
		 * 	H3 -> 4
		 * 	H4 -> 3
		 * 	H5 -> 2
		 * 	H6 -> 1
		 */
		case P_HEADER_1:
			if (state->in_paragraph != FALSE)
			{
				lo_CloseParagraph(context, state);
			}
			if (tag->is_end == FALSE)
			{
				LO_TextAttr *old_attr;
				LO_TextAttr *attr;

				lo_SoftLineBreak(context, state, FALSE, 2);
				lo_OpenHeader(context, state, tag);

				old_attr = state->font_stack->text_attr;
				lo_CopyTextAttr(old_attr, &tmp_attr);
				tmp_attr.fontmask |= LO_FONT_BOLD;
				tmp_attr.fontmask &= (~(LO_FONT_FIXED|LO_FONT_ITALIC));
				tmp_attr.size = 6;
				attr = lo_FetchTextAttr(state, &tmp_attr);

				lo_PushFont(state, tag->type, attr);
			}
			else
			{
				LO_TextAttr *attr;

				attr = lo_PopFont(state, tag->type);

				lo_CloseHeader(context, state);
				lo_SoftLineBreak(context, state, FALSE, 2);
			}
			break;
		/*
		 * See the comment for P_HEADER_1.
		 */
		case P_HEADER_2:
			if (state->in_paragraph != FALSE)
			{
				lo_CloseParagraph(context, state);
			}
			if (tag->is_end == FALSE)
			{
				LO_TextAttr *old_attr;
				LO_TextAttr *attr;

				lo_SoftLineBreak(context, state, FALSE, 2);
				lo_OpenHeader(context, state, tag);

				old_attr = state->font_stack->text_attr;
				lo_CopyTextAttr(old_attr, &tmp_attr);
				tmp_attr.fontmask |= LO_FONT_BOLD;
				tmp_attr.fontmask &= (~(LO_FONT_FIXED|LO_FONT_ITALIC));
				tmp_attr.size = 5;
				attr = lo_FetchTextAttr(state, &tmp_attr);

				lo_PushFont(state, tag->type, attr);
			}
			else
			{
				LO_TextAttr *attr;

				attr = lo_PopFont(state, tag->type);

				lo_CloseHeader(context, state);
				lo_SoftLineBreak(context, state, FALSE, 2);
			}
			break;
		/*
		 * See the comment for P_HEADER_1.
		 */
		case P_HEADER_3:
			if (state->in_paragraph != FALSE)
			{
				lo_CloseParagraph(context, state);
			}
			if (tag->is_end == FALSE)
			{
				LO_TextAttr *old_attr;
				LO_TextAttr *attr;

				lo_SoftLineBreak(context, state, FALSE, 2);
				lo_OpenHeader(context, state, tag);

				old_attr = state->font_stack->text_attr;
				lo_CopyTextAttr(old_attr, &tmp_attr);
				tmp_attr.fontmask |= LO_FONT_BOLD;
				tmp_attr.fontmask &= (~(LO_FONT_FIXED|LO_FONT_ITALIC));
				tmp_attr.size = 4;
				attr = lo_FetchTextAttr(state, &tmp_attr);

				lo_PushFont(state, tag->type, attr);
			}
			else
			{
				LO_TextAttr *attr;

				attr = lo_PopFont(state, tag->type);

				lo_CloseHeader(context, state);
				lo_SoftLineBreak(context, state, FALSE, 2);
			}
			break;
		/*
		 * See the comment for P_HEADER_1.
		 */
		case P_HEADER_4:
			if (state->in_paragraph != FALSE)
			{
				lo_CloseParagraph(context, state);
			}
			if (tag->is_end == FALSE)
			{
				LO_TextAttr *old_attr;
				LO_TextAttr *attr;

				lo_SoftLineBreak(context, state, FALSE, 2);
				lo_OpenHeader(context, state, tag);

				old_attr = state->font_stack->text_attr;
				lo_CopyTextAttr(old_attr, &tmp_attr);
				tmp_attr.fontmask |= LO_FONT_BOLD;
				tmp_attr.fontmask &= (~(LO_FONT_FIXED|LO_FONT_ITALIC));
				tmp_attr.size = 3;
				attr = lo_FetchTextAttr(state, &tmp_attr);

				lo_PushFont(state, tag->type, attr);
			}
			else
			{
				LO_TextAttr *attr;

				attr = lo_PopFont(state, tag->type);

				lo_CloseHeader(context, state);
				lo_SoftLineBreak(context, state, FALSE, 2);
			}
			break;
		/*
		 * See the comment for P_HEADER_1.
		 */
		case P_HEADER_5:
			if (state->in_paragraph != FALSE)
			{
				lo_CloseParagraph(context, state);
			}
			if (tag->is_end == FALSE)
			{
				LO_TextAttr *old_attr;
				LO_TextAttr *attr;

				lo_SoftLineBreak(context, state, FALSE, 2);
				lo_OpenHeader(context, state, tag);

				old_attr = state->font_stack->text_attr;
				lo_CopyTextAttr(old_attr, &tmp_attr);
				tmp_attr.fontmask |= LO_FONT_BOLD;
				tmp_attr.fontmask &= (~(LO_FONT_FIXED|LO_FONT_ITALIC));
				tmp_attr.size = 2;
				attr = lo_FetchTextAttr(state, &tmp_attr);

				lo_PushFont(state, tag->type, attr);
			}
			else
			{
				LO_TextAttr *attr;

				attr = lo_PopFont(state, tag->type);

				lo_CloseHeader(context, state);
				lo_SoftLineBreak(context, state, FALSE, 2);
			}
			break;
		/*
		 * See the comment for P_HEADER_1.
		 */
		case P_HEADER_6:
			if (state->in_paragraph != FALSE)
			{
				lo_CloseParagraph(context, state);
			}
			if (tag->is_end == FALSE)
			{
				LO_TextAttr *old_attr;
				LO_TextAttr *attr;

				lo_SoftLineBreak(context, state, FALSE, 2);
				lo_OpenHeader(context, state, tag);

				old_attr = state->font_stack->text_attr;
				lo_CopyTextAttr(old_attr, &tmp_attr);
				tmp_attr.fontmask |= LO_FONT_BOLD;
				tmp_attr.fontmask &= (~(LO_FONT_FIXED|LO_FONT_ITALIC));
				tmp_attr.size = 1;
				attr = lo_FetchTextAttr(state, &tmp_attr);

				lo_PushFont(state, tag->type, attr);
			}
			else
			{
				LO_TextAttr *attr;

				attr = lo_PopFont(state, tag->type);

				lo_CloseHeader(context, state);
				lo_SoftLineBreak(context, state, FALSE, 2);
			}
			break;

		/*
		 * Description titles are just normal,
		 * unless value > 1 and then they are outdented
		 * the width of one list indention.
		 */
		case P_DESC_TITLE:
			if (state->in_paragraph != FALSE)
			{
				lo_CloseParagraph(context, state);
			}
			if (tag->is_end == FALSE)
			{
				/*
				 * Undent to the left for a title if necessary.
				 */
				if ((state->list_stack->level != 0)&&
				    (state->list_stack->type == P_DESC_LIST)&&
				    (state->list_stack->value > 1))
				{
					state->list_stack->old_left_margin -=
						LIST_MARGIN_INC;
					state->list_stack->value = 1;
				}

				if (state->linefeed_state >= 1)
				{
					lo_FindLineMargins(context, state);
				}
				else
				{
					lo_SoftLineBreak(context,state,FALSE,1);
				}
				state->x = state->left_margin;
			}
			break;

		/*
		 * Description text, is just normal text, but since there
		 * is no ending <dd> tag, we need to put the
		 * linebreak from the previous line in here.
		 * Compact lists may not linebreak since they try to
		 * put description text on the same line as title
		 * text if there is room.
		 * If value == 1 then we need to indent for the description
		 * text, if it is > 1 then we are already indented.
		 */
		case P_DESC_TEXT:
			if (state->in_paragraph != FALSE)
			{
				lo_CloseParagraph(context, state);
			}
			if (tag->is_end == FALSE)
			{
				/*
				 * Indent from the title if necessary.
				 */
				if ((state->list_stack->type == P_DESC_LIST)&&
					(state->list_stack->value == 1))
				{
					state->list_stack->old_left_margin +=
						LIST_MARGIN_INC;
					state->list_stack->value = 2;
				}

				if (state->list_stack->compact == FALSE)
				{
					if (state->linefeed_state >= 1)
					{
					    lo_FindLineMargins(context, state);
					}
					else
					{
					    lo_SoftLineBreak(context, state,
							FALSE, 1);
					}
				}
				else
				{
					if (state->x >=
					    state->list_stack->old_left_margin)
					{
						if (state->linefeed_state >= 1)
						{
						    lo_FindLineMargins(context,
							state);
						}
						else
						{
						    lo_SoftLineBreak(context,
							state, FALSE, 1);
						}
					}
					else
					{
						lo_FindLineMargins(context,
							state);
						state->x = state->left_margin;
					}
				}
				state->x = state->left_margin;

				/*
				 * Bug compatibility for <dd> outside a <dl>
				 */
				if (state->list_stack->type != P_DESC_LIST)
				{
					state->x += LIST_MARGIN_INC;
				}
			}
			break;
		/*
		 * A new list level, but don't move the
		 * left margin in until you need to.
		 * val is overloaded to tell us if we are
		 * not indented (val=1) or indented (val > 1).
		 * Push a new element on the list stack.
		 */
		case P_DESC_LIST:
			if (state->in_paragraph != FALSE)
			{
				lo_CloseParagraph(context, state);
			}
			if (tag->is_end == FALSE)
			{
				int32 type;
				int32 val;
				Bool compact;
				PA_Block buff;

				type = BULLET_BASIC;
				val = 1;

				buff = lo_FetchParamValue(context, tag, PARAM_COMPACT);
				if (buff != NULL)
				{
					PA_FREE(buff);
					compact = TRUE;
				}
				else
				{
					compact = FALSE;
				}

				if (state->list_stack->level == 0)
				{
					lo_SoftLineBreak(context, state,
						FALSE, 2);
				}
				else
				{
					lo_SoftLineBreak(context, state,
						FALSE, 1);
				}
/* THIS BREAKS FLOATING IMAGES IN LISTS
				state->list_stack->old_left_margin =
					state->left_margin;
				state->list_stack->old_right_margin =
					state->right_margin;
*/

				/*
				 * handle nested description lists
				 * with the right amount of indenting
				 */
				if ((state->list_stack->type == P_DESC_LIST)&&
					(state->list_stack->value == 1))
				{
					state->left_margin += LIST_MARGIN_INC;
				}

				lo_PushList(state, tag);

				state->x = state->left_margin;
				state->list_stack->old_left_margin =
					state->left_margin;
				state->list_stack->bullet_type = (intn) type;
				state->list_stack->value = val;
				state->list_stack->compact = compact;
			}
			else
			{
				lo_ListStack *lptr;

				lptr = lo_PopList(state, tag);
				if (lptr != NULL)
				{
					XP_DELETE(lptr);
				}
				state->left_margin =
					state->list_stack->old_left_margin;
				state->right_margin =
					state->list_stack->old_right_margin;

				if (state->list_stack->level == 0)
				{
					if (state->linefeed_state >= 2)
					{
						lo_FindLineMargins(context, state);
					}
					else
					{
						lo_SoftLineBreak(context, state, FALSE,2);
					}
				}
				else
				{
					if (state->linefeed_state >= 1)
					{
						lo_FindLineMargins(context, state);
					}
					else
					{
						lo_SoftLineBreak(context, state, FALSE,1);
					}
				}
				state->x = state->left_margin;
			}
			break;
		/*
		 * Move the left margin in another indent step.
		 * Push a new element on the list stack.
		 */
		case P_NUM_LIST:
		case P_UNUM_LIST:
		case P_MENU:
		case P_DIRECTORY:
			if (state->in_paragraph != FALSE)
			{
				lo_CloseParagraph(context, state);
			}
			if (tag->is_end == FALSE)
			{
				int32 type;
				int32 val;
				int32 old_right_margin;
				Bool compact;
				PA_Block buff;

				type = BULLET_BASIC;
				buff = lo_FetchParamValue(context, tag, PARAM_TYPE);
				if ((buff != NULL)&&(tag->type == P_NUM_LIST))
				{
					char *type_str;

					PA_LOCK(type_str, char *, buff);
					if (*type_str == 'A')
					{
						type = BULLET_ALPHA_L;
					}
					else if (*type_str == 'a')
					{
						type = BULLET_ALPHA_S;
					}
					else if (*type_str == 'I')
					{
						type = BULLET_NUM_L_ROMAN;
					}
					else if (*type_str == 'i')
					{
						type = BULLET_NUM_S_ROMAN;
					}
					else
					{
						type = BULLET_NUM;
					}
					PA_UNLOCK(buff);
				}
				else if ((buff != NULL)&&
					(tag->type != P_DESC_LIST))
				{
					char *type_str;

					PA_LOCK(type_str, char *, buff);
					if (pa_TagEqual("round", type_str))
					{
						type = BULLET_ROUND;
					}
					else if (pa_TagEqual("circle",type_str))
					{
						type = BULLET_ROUND;
					}
					else if (pa_TagEqual("square",type_str))
					{
						type = BULLET_SQUARE;
					}
					else
					{
						type = BULLET_BASIC;
					}
					PA_UNLOCK(buff);
				}
				else if (tag->type == P_NUM_LIST)
				{
					type = BULLET_NUM;
				}
				else if (tag->type != P_DESC_LIST)
				{
					intn lev;

					lev = state->list_stack->level;
					if (lev < 1)
					{
						type = BULLET_BASIC;
					}
					else if (lev == 1)
					{
						type = BULLET_ROUND;
					}
					else if (lev > 1)
					{
						type = BULLET_SQUARE;
					}
				}
				
				if (buff != NULL) 
				{
					PA_FREE(buff);
				}
				
				buff = lo_FetchParamValue(context, tag, PARAM_START);
				if (buff != NULL)
				{
					char *val_str;

					PA_LOCK(val_str, char *, buff);
					val = XP_ATOI(val_str);
					if (val < 1)
					{
						val = 1;
					}
					PA_UNLOCK(buff);
					PA_FREE(buff);
				}
				else
				{
					val = 1;
				}

				buff = lo_FetchParamValue(context, tag, PARAM_COMPACT);
				if (buff != NULL)
				{
					PA_FREE(buff);
					compact = TRUE;
				}
				else
				{
					compact = FALSE;
				}

				if (state->list_stack->level == 0)
				{
					lo_SoftLineBreak(context, state,
						FALSE, 2);
				}
				else
				{
					lo_SoftLineBreak(context, state,
						FALSE, 1);
				}
/* THIS BREAKS FLOATING IMAGES IN LISTS
				state->list_stack->old_left_margin =
					state->left_margin;
				state->list_stack->old_right_margin =
					state->right_margin;
*/
				/*
				 * For lists, we leave the right
				 * margin unchanged.
				 */
				old_right_margin =
					state->list_stack->old_right_margin;
				lo_PushList(state, tag);
				state->left_margin += LIST_MARGIN_INC;
				state->x = state->left_margin;
				state->list_stack->old_left_margin =
					state->left_margin;
				/*
				 * Set right margin to be last list's right,
				 * irregardless of current margin stack.
				 */
				state->list_stack->old_right_margin =
					old_right_margin;
				state->list_stack->bullet_type = (intn) type;
				state->list_stack->value = val;
				state->list_stack->compact = compact;
			}
			else
			{
				lo_ListStack *lptr;

				/*
				 * Reset fake linefeed state used to
				 * fake out headers in lists.
				 */
				if (state->at_begin_line == FALSE)
				{
					state->linefeed_state = 0;
				}

				lptr = lo_PopList(state, tag);
				if (lptr != NULL)
				{
					XP_DELETE(lptr);
				}
				state->left_margin =
					state->list_stack->old_left_margin;
				state->right_margin =
					state->list_stack->old_right_margin;
				/*
				 * Reset the margins properly in case
				 * there is a right-aligned image here.
				 */
				lo_FindLineMargins(context, state);
				if (state->list_stack->level == 0)
				{
					if (state->linefeed_state >= 2)
					{
						lo_FindLineMargins(context, state);
					}
					else
					{
						lo_SoftLineBreak(context, state, FALSE,2);
					}
				}
				else
				{
					if (state->linefeed_state >= 1)
					{
						lo_FindLineMargins(context, state);
					}
					else
					{
						lo_SoftLineBreak(context, state, FALSE,1);
					}
				}
				state->x = state->left_margin;
			}
			break;

		/*
		 * Complex because we have to place one of many types
		 * of bullet here, or one of several types of numbers.
		 */
		case P_LIST_ITEM:
			if (state->in_paragraph != FALSE)
			{
				lo_CloseParagraph(context, state);
			}
			if (tag->is_end == FALSE)
			{
				int type;
				int bullet_type;
				PA_Block buff;

				/*
				 * Reset fake linefeed state used to
				 * fake out headers in lists.
				 */
				if (state->at_begin_line == FALSE)
				{
					state->linefeed_state = 0;
				}

				lo_SoftLineBreak(context, state, FALSE, 1);
				/*
				 * Artificially setting state to 2 here
				 * allows us to put headers on list items
				 * even if they aren't double spaced.
				 */
				state->linefeed_state = 2;

				type = state->list_stack->bullet_type;
				buff = lo_FetchParamValue(context, tag, PARAM_TYPE);
				if ((buff != NULL)&&
					(state->list_stack->type == P_NUM_LIST))
				{
					char *type_str;

					PA_LOCK(type_str, char *, buff);
					if (*type_str == 'A')
					{
						type = BULLET_ALPHA_L;
					}
					else if (*type_str == 'a')
					{
						type = BULLET_ALPHA_S;
					}
					else if (*type_str == 'I')
					{
						type = BULLET_NUM_L_ROMAN;
					}
					else if (*type_str == 'i')
					{
						type = BULLET_NUM_S_ROMAN;
					}
					else
					{
						type = BULLET_NUM;
					}
					PA_UNLOCK(buff);
				}
				else if ((buff != NULL)&&
					(state->list_stack->type !=P_DESC_LIST))
				{
					char *type_str;

					PA_LOCK(type_str, char *, buff);
					if (pa_TagEqual("round", type_str))
					{
						type = BULLET_ROUND;
					}
					else if (pa_TagEqual("circle",type_str))
					{
						type = BULLET_ROUND;
					}
					else if (pa_TagEqual("square",type_str))
					{
						type = BULLET_SQUARE;
					}
					else
					{
						type = BULLET_BASIC;
					}
					PA_UNLOCK(buff);
				}
				state->list_stack->bullet_type = type;

				if (buff != NULL) 
				{
					PA_FREE(buff);
				}

				buff = lo_FetchParamValue(context, tag, PARAM_VALUE);
				if (buff != NULL)
				{
					int32 val;
					char *val_str;

					PA_LOCK(val_str, char *, buff);
					val = XP_ATOI(val_str);
					if (val < 1)
					{
						val = 1;
					}
					PA_UNLOCK(buff);
					state->list_stack->value = val;

					PA_FREE(buff);
				}

				bullet_type = state->list_stack->bullet_type;
				if ((bullet_type == BULLET_NUM)||
				    (bullet_type == BULLET_ALPHA_L)||
				    (bullet_type == BULLET_ALPHA_S)||
				    (bullet_type == BULLET_NUM_L_ROMAN)||
				    (bullet_type == BULLET_NUM_S_ROMAN))
				{
					lo_PlaceBulletStr(context, state);
					state->list_stack->value++;
				}
				else
				{
					lo_PlaceBullet(context, state);
				}

				/*
				 * Make at_begin_line be accurate
				 * so we can detect the header
				 * linefeed state deception later.
				 */
				state->at_begin_line = FALSE;

				/*
				 * After much soul-searching (and brow-beating
				 * by Jamie, I've agreed that really whitespace
				 * should be compressed out at the start of a
				 * list item.  They can always add non-breaking
				 * spaces if they want them.
				 * Setting trailing space true means it won't
				 * let the users add whitespace because it
				 * thinks there already is some.
				 */
				state->trailing_space = TRUE;
			}
			break;

		/*
		 * Another font attribute, another font on the font stack
		 */
		case P_BLINK:
			if (tag->is_end == FALSE)
			{
				LO_TextAttr *old_attr;
				LO_TextAttr *attr;

				old_attr = state->font_stack->text_attr;
				lo_CopyTextAttr(old_attr, &tmp_attr);
				tmp_attr.attrmask |= LO_ATTR_BLINK;
				attr = lo_FetchTextAttr(state, &tmp_attr);

				lo_PushFont(state, tag->type, attr);
			}
			else
			{
				LO_TextAttr *attr;

				attr = lo_PopFont(state, tag->type);
			}
			break;

		/*
		 * Another font attribute, another font on the font stack
		 * Because of spec mutation, two tags do the same thing.
		 * <s> and <strike> are the same.
		 */
		case P_STRIKEOUT:
		case P_STRIKE:
			if (tag->is_end == FALSE)
			{
				LO_TextAttr *old_attr;
				LO_TextAttr *attr;

				old_attr = state->font_stack->text_attr;
				lo_CopyTextAttr(old_attr, &tmp_attr);
				tmp_attr.attrmask |= LO_ATTR_STRIKEOUT;
				attr = lo_FetchTextAttr(state, &tmp_attr);

				lo_PushFont(state, tag->type, attr);
			}
			else
			{
				LO_TextAttr *attr;

				attr = lo_PopFont(state, tag->type);
			}
			break;

		/*
		 * Another font attribute, another font on the font stack
		 */
		case P_UNDERLINE:
			if (tag->is_end == FALSE)
			{
				LO_TextAttr *old_attr;
				LO_TextAttr *attr;

				old_attr = state->font_stack->text_attr;
				lo_CopyTextAttr(old_attr, &tmp_attr);
				tmp_attr.attrmask |= LO_ATTR_UNDERLINE;
				attr = lo_FetchTextAttr(state, &tmp_attr);

				lo_PushFont(state, tag->type, attr);
			}
			else
			{
				LO_TextAttr *attr;

				attr = lo_PopFont(state, tag->type);
			}
			break;

		/*
		 * Center all following lines.  Forces a single
		 * line break to start and end the new centered lines.
		 */
		case P_CENTER:
			lo_SoftLineBreak(context, state, FALSE, 1);
			if (tag->is_end == FALSE)
			{
				lo_PushAlignment(state, tag, LO_ALIGN_CENTER);
			}
			else
			{
				lo_AlignStack *aptr;

				aptr = lo_PopAlignment(state);
				if (aptr != NULL)
				{
					XP_DELETE(aptr);
				}
			}
			break;

		/*
		 * For plaintext there is no endtag, so this is
		 * really the last tag will we be parsing if this
		 * is a plaintext tag.
		 *
		 * Difference between these three tags are
		 * all based on what things the parser allows to
		 * be tags while inside them.
		 */
		case P_PREFORMAT:
		case P_PLAIN_PIECE:
		case P_PLAIN_TEXT:
			if (state->in_paragraph != FALSE)
			{
				lo_CloseParagraph(context, state);
			}
			if (tag->is_end == FALSE)
			{
				LO_TextAttr *old_attr;
				LO_TextAttr *attr;
				PA_Block buff;

				lo_SoftLineBreak(context, state, FALSE, 2);

				state->preformatted = PRE_TEXT_YES;
				FE_BeginPreSection(context);

				/*
				 * Special WRAP attribute of the PRE tag
				 * to make wrapped pre sections.
				 */
				if (tag->type == P_PREFORMAT)
				{
					buff = lo_FetchParamValue(context, tag,
							PARAM_WRAP);
					if (buff != NULL)
					{
						state->preformatted =
							PRE_TEXT_WRAP;
						PA_FREE(buff);
					}
				}

				old_attr = state->font_stack->text_attr;
				lo_CopyTextAttr(old_attr, &tmp_attr);

				/*
				 * Allow <PRE VARIABLE> to create a variable
				 * width font preformatted section.  This
				 * is only to be used internally for 
				 * mail and news in Netscape 2.0 and greater.
				 */
				buff = lo_FetchParamValue(context, tag, PARAM_VARIABLE);
				if ((tag->type == P_PREFORMAT)&&(buff != NULL))
				{
					tmp_attr.fontmask &= (~(LO_FONT_FIXED));
				}
				else
				{
					tmp_attr.fontmask |= LO_FONT_FIXED;
				}
				/*
				 * If VARIABLE attribute was there, free
				 * the value buffer.
				 */
				if (buff != NULL)
				{
					PA_FREE(buff);
				}

				attr = lo_FetchTextAttr(state, &tmp_attr);

				lo_PushFont(state, tag->type, attr);
			}
			else
			{
				LO_TextAttr *attr;

				attr = lo_PopFont(state, tag->type);

				state->preformatted = PRE_TEXT_NO;
				lo_SoftLineBreak(context, state, FALSE, 2);
				FE_EndPreSection(context);
			}
			break;

		/*
		 * Historic text type.  Paragraph break, font size change
		 * and font set to fixed.  Was supposed to support
		 * lineprinter listings if you can believe that!
		 */
		case P_LISTING_TEXT:
			if (state->in_paragraph != FALSE)
			{
				lo_CloseParagraph(context, state);
			}
			if (tag->is_end == FALSE)
			{
				LO_TextAttr *old_attr;
				LO_TextAttr *attr;

				lo_SoftLineBreak(context, state, FALSE, 2);

				state->preformatted = PRE_TEXT_YES;
				FE_BeginPreSection(context);

				old_attr = state->font_stack->text_attr;
				lo_CopyTextAttr(old_attr, &tmp_attr);
				tmp_attr.fontmask |= LO_FONT_FIXED;
				tmp_attr.size = DEFAULT_BASE_FONT_SIZE - 1;
				attr = lo_FetchTextAttr(state, &tmp_attr);

				lo_PushFont(state, tag->type, attr);
			}
			else
			{
				LO_TextAttr *attr;

				attr = lo_PopFont(state, tag->type);

				state->preformatted = PRE_TEXT_NO;
				lo_SoftLineBreak(context, state, FALSE, 2);
				FE_EndPreSection(context);
			}
			break;

		/*
		 * Another historic tag.  Break a line, and switch to italic.
		 */
		case P_ADDRESS:
			if (state->in_paragraph != FALSE)
			{
				lo_CloseParagraph(context, state);
			}
			if (tag->is_end == FALSE)
			{
				LO_TextAttr *old_attr;
				LO_TextAttr *attr;

				lo_SoftLineBreak(context, state, FALSE, 1);

				old_attr = state->font_stack->text_attr;
				lo_CopyTextAttr(old_attr, &tmp_attr);
				tmp_attr.fontmask |= LO_FONT_ITALIC;
				attr = lo_FetchTextAttr(state, &tmp_attr);

				lo_PushFont(state, tag->type, attr);
			}
			else
			{
				LO_TextAttr *attr;

				attr = lo_PopFont(state, tag->type);

				lo_SoftLineBreak(context, state, FALSE, 1);
			}
			break;

		/*
		 * Subscript.  Move down so the middle of the ascent
		 * of the new text aligns with the baseline of the old.
		 */
		case P_SUB:
			if (state->font_stack != NULL)
			{
				PA_Block buff;
				char *str;
				LO_TextStruct tmp_text;
				LO_TextInfo text_info;

				if (tag->is_end == FALSE)
				{
					LO_TextAttr *old_attr;
					LO_TextAttr *attr;
					intn new_size;

					old_attr = state->font_stack->text_attr;
					lo_CopyTextAttr(old_attr, &tmp_attr);
					new_size = LO_ChangeFontSize(tmp_attr.size,
								"-1");
					tmp_attr.size = new_size;
					attr = lo_FetchTextAttr(state,
						&tmp_attr);

					lo_PushFont(state, tag->type, attr);
				}

				/*
				 * All this work is to get the text_info
				 * filled in for the current font in the font
				 * stack. Yuck, there must be a better way.
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

				if (tag->is_end == FALSE)
				{
					state->baseline +=
						(text_info.ascent / 2);
				}
				else
				{
					LO_TextAttr *attr;

					state->baseline -=
						(text_info.ascent / 2);

					attr = lo_PopFont(state, tag->type);
				}
			}
			break;

		/*
		 * Superscript.  Move up so the baseline of the new text
		 * is raised above the baseline of the old by 1/2 the
		 * ascent of the new text.
		 */
		case P_SUPER:
			if (state->font_stack != NULL)
			{
				PA_Block buff;
				char *str;
				LO_TextStruct tmp_text;
				LO_TextInfo text_info;

				if (tag->is_end == FALSE)
				{
					LO_TextAttr *old_attr;
					LO_TextAttr *attr;
					intn new_size;

					old_attr = state->font_stack->text_attr;
					lo_CopyTextAttr(old_attr, &tmp_attr);
					new_size = LO_ChangeFontSize(tmp_attr.size,
								"-1");
					tmp_attr.size = new_size;
					attr = lo_FetchTextAttr(state,
						&tmp_attr);

					lo_PushFont(state, tag->type, attr);
				}

				/*
				 * All this work is to get the text_info
				 * filled in for the current font in the font
				 * stack. Yuck, there must be a better way.
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

				if (tag->is_end == FALSE)
				{
					state->baseline -=
						(text_info.ascent / 2);
				}
				else
				{
					LO_TextAttr *attr;

					state->baseline +=
						(text_info.ascent / 2);

					attr = lo_PopFont(state, tag->type);
				}
			}
			break;

		/*
		 * Change the base font size that relative font size changes
		 * are figured from.
		 */
		case P_BASEFONT:
			{
				LO_TextAttr *old_attr;
				LO_TextAttr *attr;
				PA_Block buff;
				intn new_size;
				char *size_str;
				lo_FontStack *fptr;

				buff = lo_FetchParamValue(context, tag, PARAM_SIZE);
				if (buff != NULL)
				{
					PA_LOCK(size_str, char *, buff);
					new_size = LO_ChangeFontSize(
						state->base_font_size,
						size_str);
					PA_UNLOCK(buff);
					PA_FREE(buff);
				}
				else
				{
					new_size = DEFAULT_BASE_FONT_SIZE;
				}
				state->base_font_size = new_size;

				fptr = state->font_stack;
				/*
				 * If the current font is the basefont
				 * we are changing the current font size.
				 */
				if (fptr->tag_type == P_UNKNOWN)
				{
					/*
					 * Flush all current text before
					 * changing the font size.
					 */
					if ((state->cur_ele_type == LO_TEXT)&&
						(state->line_buf_len != 0))
					{
						lo_FlushLineBuffer(context,
							state);
					}
				}

				/*
				 * Traverse the font stack to the base font
				 * entry.
				 */
				while (fptr->tag_type != P_UNKNOWN)
				{
					fptr = fptr->next;
				}

				/*
				 * Copy the old base entry, change its
				 * size, and stuff the new one in its place
				 */
				old_attr = fptr->text_attr;
				lo_CopyTextAttr(old_attr, &tmp_attr);
				tmp_attr.size = state->base_font_size;
				attr = lo_FetchTextAttr(state, &tmp_attr);
				if (attr != NULL)
				{
					fptr->text_attr = attr;
				}
			}
			break;


		/*
		 * Change font, currently only lets you change size.
		 * Thanks to Microsoft we now let you change color.
		 * Thanks again to Microsoft for the evil of FACE.
		 */
		case P_FONT:
			if (tag->is_end == FALSE)
			{
				LO_TextAttr *old_attr;
				LO_TextAttr *attr;
				PA_Block buff;
				intn new_size;
				char *size_str;
				char *str;
				char *new_face;
				Bool has_face;
				Bool has_size;
				Bool new_colors;
				uint8 red, green, blue;

				new_size = state->base_font_size;
				has_size = FALSE;
				has_face = FALSE;
				buff = lo_FetchParamValue(context, tag, PARAM_SIZE);
				if (buff != NULL)
				{
					PA_LOCK(size_str, char *, buff);
					new_size = LO_ChangeFontSize(
						state->base_font_size,
						size_str);
					PA_UNLOCK(buff);
					PA_FREE(buff);
					has_size = TRUE;
				}

				/*
				 * For backwards compatibility, a font
				 * tag with NO attributes will reset the
				 * font size to the base size.
				 */
				if (PA_TagHasParams(tag) == FALSE)
				{
					new_size = state->base_font_size;
					has_size = TRUE;
				}

				new_colors = FALSE;
				buff = lo_FetchParamValue(context, tag, PARAM_COLOR);
				if (buff != NULL)
				{
				    PA_LOCK(str, char *, buff);
				    LO_ParseRGB(str, &red, &green, &blue);
				    PA_UNLOCK(buff);
				    PA_FREE(buff);
				    new_colors = TRUE;
				}

				buff = lo_FetchParamValue(context, tag,
						PARAM_FACE);
				new_face = NULL;
				if (buff != NULL)
				{
				    PA_LOCK(str, char *, buff);
				    new_face = lo_FetchFontFace(context, state,
							str);
				    PA_UNLOCK(buff);
				    PA_FREE(buff);
				    has_face = TRUE;
				}

				old_attr = state->font_stack->text_attr;
				lo_CopyTextAttr(old_attr, &tmp_attr);

				if (has_size != FALSE)
				{
					tmp_attr.size = new_size;
				}

				if (new_colors != FALSE)
				{
				    tmp_attr.fg.red = red;
				    tmp_attr.fg.green = green;
				    tmp_attr.fg.blue = blue;
				}

				if (has_face != FALSE)
				{
				    tmp_attr.font_face = new_face;
				}

				attr = lo_FetchTextAttr(state, &tmp_attr);

				lo_PushFont(state, tag->type, attr);
			}
			else
			{
				LO_TextAttr *attr;

				attr = lo_PopFont(state, tag->type);
			}
			break;

		/*
		 * Change font to bigger than current font.
		 */
		case P_BIG:
			if (tag->is_end == FALSE)
			{
				LO_TextAttr *old_attr;
				LO_TextAttr *attr;
				intn new_size;

				old_attr = state->font_stack->text_attr;
				lo_CopyTextAttr(old_attr, &tmp_attr);
				new_size = LO_ChangeFontSize(tmp_attr.size, "+1");
				tmp_attr.size = new_size;
				attr = lo_FetchTextAttr(state, &tmp_attr);

				lo_PushFont(state, tag->type, attr);
			}
			else
			{
				LO_TextAttr *attr;

				attr = lo_PopFont(state, tag->type);
			}
			break;

		/*
		 * Change font to smaller than current font.
		 */
		case P_SMALL:
			if (tag->is_end == FALSE)
			{
				LO_TextAttr *old_attr;
				LO_TextAttr *attr;
				intn new_size;

				old_attr = state->font_stack->text_attr;
				lo_CopyTextAttr(old_attr, &tmp_attr);
				new_size = LO_ChangeFontSize(tmp_attr.size, "-1");
				tmp_attr.size = new_size;
				attr = lo_FetchTextAttr(state, &tmp_attr);

				lo_PushFont(state, tag->type, attr);
			}
			else
			{
				LO_TextAttr *attr;

				attr = lo_PopFont(state, tag->type);
			}
			break;

		/*
		 * Change the absolute URL that describes where this
		 * docuemtn came from for all following
		 * relative URLs.
		 */
		case P_BASE:
			{
				PA_Block buff;
				char *url;
				char *target;

				buff = lo_FetchParamValue(context, tag, PARAM_HREF);
				if (buff != NULL)
				{
					PA_LOCK(url, char *, buff);
					if (url != NULL)
					{
					    int32 len;

					    len = lo_StripTextWhitespace(
							url, XP_STRLEN(url));
					}
					if (state->top_state->base_url != NULL)
					{
					    XP_FREE(state->top_state->base_url);
					    state->top_state->base_url = NULL;
					}
					state->top_state->base_url =
							XP_STRDUP(url);
					PA_UNLOCK(buff);
					PA_FREE(buff);
				}

				buff = lo_FetchParamValue(context, tag, PARAM_TARGET);
				if (buff != NULL)
				{
					PA_LOCK(target, char *, buff);
					if (target != NULL)
					{
					    int32 len;

					    len = lo_StripTextWhitespace(
						target, XP_STRLEN(target));
					}
				        if (lo_IsValidTarget(target) == FALSE)
					{
					    PA_UNLOCK(buff);
					    PA_FREE(buff);
					    target = NULL;
					}
					if (state->top_state->base_target !=
						NULL)
					{
					    XP_FREE(
						state->top_state->base_target);
					    state->top_state->base_target =NULL;
					}
					if (target != NULL)
					{
					    state->top_state->base_target =
							XP_STRDUP(target);
					}
					PA_UNLOCK(buff);
					PA_FREE(buff);
				}
			}
			break;

		/*
		 * Begin an anchor.  Change font colors if
		 * there is an HREF here.  Make the HREF absolute
		 * based on the base url.  Check name in case we
		 * are jumping to a named anchor inside this document.
		 */
		case P_ANCHOR:
			lo_process_anchor_tag(context, state, tag);
			if (state->top_state->out_of_memory != FALSE)
			{
				return;
			}
			break;

		/*
		 * Insert a wordbreak.  Allows layout to break a line
		 * at this location in case it needs to.
		 */
		case P_WORDBREAK:
			lo_InsertWordBreak(context, state);
			break;

		/*
		 * All text inside these tags must be considered one
		 * word, and not broken (unless there is a word break
		 * in there).
		 */
		case P_NOBREAK:
			if (tag->is_end == FALSE)
			{
				state->breakable = FALSE;
			}
			else
			{
				state->breakable = TRUE;
			}
			break;

		/*
		 * spacer element to move stuff around
		 */
		case P_SPACER:
			if (tag->is_end == FALSE)
			{
				lo_ProcessSpacerTag(context, state, tag);
			}
			break;

		case P_MULTICOLUMN:
			if (tag->is_end == FALSE)
			{
				lo_BeginMulticolumn(context, state, tag);
			}
			else
			{
				if (state->current_multicol != NULL)
				{
					lo_EndMulticolumn(context, state, tag,
						state->current_multicol);
				}
			}
			break;

		/*
		 * Break the current line.
		 * Optionally break furthur down to get past floating
		 * elements in the margins.
		 */
		case P_LINEBREAK:
			/*
			 * <br> tag diabled in preformatted text
			 */
#ifdef EDITOR
			if (state->preformatted != PRE_TEXT_NO && !EDT_IS_EDITOR(context))
#else
			if (state->preformatted != PRE_TEXT_NO)
#endif
			{
				lo_SoftLineBreak(context, state, FALSE, 1);
				break;
			}
			lo_HardLineBreak(context, state, FALSE);
#ifdef EDITOR
			/* editor hack.  Make breaks always do something. */
			if ( EDT_IS_EDITOR(context) ) {
				state->linefeed_state = 0;
            }
#endif

			{
			    PA_Block buff;

			    buff = lo_FetchParamValue(context, tag, PARAM_CLEAR);
			    if (buff != NULL)
			    {
				char *clear_str;

				PA_LOCK(clear_str, char *, buff);
				if (pa_TagEqual("left", clear_str))
				{
					lo_ClearToLeftMargin(context, state);
				}
				else if (pa_TagEqual("right", clear_str))
				{
					lo_ClearToRightMargin(context, state);
				}
				else if (pa_TagEqual("all", clear_str))
				{
					lo_ClearToBothMargins(context, state);
				}
				else if (pa_TagEqual("both", clear_str))
				{
					lo_ClearToBothMargins(context, state);
				}
				PA_UNLOCK(buff);
				PA_FREE(buff);
				/*
				 * Reset the margins properly in case
				 * we are inside a list.
				 */
				lo_FindLineMargins(context, state);
				state->x = state->left_margin;
			    }
			}
			break;

		/*
		 * The head tag.  The tag is supposed to be a container
		 * for the head of the HTML document.
		 * We use it to try and detect when we should
		 * suppress the content of unknown head tags.
		 */
		case P_HEAD:
			/*
			 * The end HEAD tags defines the end of
			 * the HEAD section of the HTML
			 */
			if (tag->is_end != FALSE)
			{
				state->top_state->in_head = FALSE;
			}
			break;

		/*
		 * The body tag.  The tag is supposed to be a container
		 * for the body of the HTML document.
		 * Right now we are just using it to specify
		 * the document qualities and colors.
		 */
		case P_BODY:
			/*
			 * The start of BODY definitely ends
			 * the HEAD section of the HTML and starts the BODY
			 */
			state->top_state->in_head = FALSE;
			state->top_state->in_body = TRUE;

			lo_ProcessBodyTag(context, state, tag);
			break;

		/*
		 * The colormap tag.  This is used to specify the colormap
		 * to be used by all the images on this page.
		 */
		case P_COLORMAP:
			if ((tag->is_end == FALSE)&&
				(state->end_last_line == NULL))
			{
				PA_Block buff;
				char *str;
				char *image_url;

				image_url = NULL;
				buff = lo_FetchParamValue(context, tag, PARAM_SRC);
				if (buff != NULL)
				{
					PA_LOCK(str, char *, buff);
					if (str != NULL)
					{
						int32 len;

						len = lo_StripTextWhitespace(
							str, XP_STRLEN(str));
					}
					image_url = NET_MakeAbsoluteURL(
						state->top_state->base_url,str);
					PA_UNLOCK(buff);
					PA_FREE(buff);
				}
				(void)IL_ColormapTag(image_url, context);
				if (image_url != NULL)
				{
					XP_FREE(image_url);
				}
			}
			break;

		/*
		 * The meta tag.  This tag can only appear in the
		 * head of the document.
		 * It overrides HTTP headers sent by the
		 * web server that served this document.
		 * If URLStruct somehow became NULL, META must
		 * be ignored.
		 */
		case P_META:
			if ((tag->is_end == FALSE)&&
				(state->end_last_line == NULL)&&
				(state->top_state->nurl != NULL))
			{
				PA_Block buff;
				PA_Block buff2;
				PA_Block buff3;
				char *name;
				char *value;
				char *tptr;

				buff = lo_FetchParamValue(context, tag,
					PARAM_HTTP_EQUIV);
				if (buff != NULL)
				{
				    /*
				     * Lou needs a colon on the end of
				     * the name.
				     */
				    PA_LOCK(tptr, char *, buff);
				    buff3 = PA_ALLOC(XP_STRLEN(tptr) + 2);
				    if (buff3 != NULL)
				    {
					PA_LOCK(name, char *, buff3);
					XP_STRCPY(name, tptr);
					XP_STRCAT(name, ":");
					PA_UNLOCK(buff);
					PA_FREE(buff);

					buff2 = lo_FetchParamValue(context, tag,
						PARAM_CONTENT);
					if (buff2 != NULL)
					{
						Bool ok;

						PA_LOCK(value, char *, buff2);
						ok = NET_ParseMimeHeader(
							context,
							state->top_state->nurl,
							name, value);
						PA_UNLOCK(buff2);
						PA_FREE(buff2);

/* DEBUG_jwz */
#if 0  /* jwz: never ever do that fucking relayout!
		  it's useless to me, slow, and occasionally corrupts memory.
	    */

						if (context->relayout == METACHARSET_REQUESTRELAYOUT)
						{
							context->relayout = METACHARSET_FORCERELAYOUT;
							INTL_Relayout(context);
						}
#endif

					}
					PA_UNLOCK(buff3);
					PA_FREE(buff3);
				    }
				    else
				    {
					PA_UNLOCK(buff);
					PA_FREE(buff);
				    }
				}
			}
			break;

		/*
		 * The hype tag is just for fun.
		 * It only effects the UNIX version
		 * which can affor to have a sound file
		 * compiled into the binary.
		 */
		case P_HYPE:
#if defined(XP_UNIX) || defined(XP_MAC)
			if (tag->is_end == FALSE)
			{
			    PA_Tag tmp_tag;
			    PA_Block buff;
			    PA_Block abuff;
			    LO_AnchorData *hold_current_anchor;
			    LO_AnchorData *hype_anchor;
			    char *str;

			    /*
			     * Try to allocate the hype anchor as if we
			     * were inside:
			     * <A HREF="HYPE_ANCHOR">
			     * Save the real current anchor to restore later
			     */
			    hype_anchor = NULL;
			    abuff = PA_ALLOC(XP_STRLEN(HYPE_ANCHOR) + 1);
			    if (abuff != NULL)
			    {
				hype_anchor = lo_NewAnchor(state, NULL, NULL);
				if (hype_anchor == NULL)
				{
				    PA_FREE(abuff);
				    abuff = NULL;
				}
				else
				{
				    PA_LOCK(str, char *, abuff);
				    XP_STRCPY(str, HYPE_ANCHOR);
				    PA_UNLOCK(abuff);

				    hype_anchor->anchor = abuff;
				    hype_anchor->target = NULL;

				    /*
				     * Add this url's block to the list
				     * of all allocated urls so we can free
				     * it later.
				     */
				    lo_AddToUrlList(context, state,
					hype_anchor);
				    if (state->top_state->out_of_memory !=FALSE)
				    {
					PA_FREE(abuff);
					XP_DELETE(hype_anchor);
					return;
				    }
				}
			    }
			    hold_current_anchor = state->current_anchor;
			    state->current_anchor = hype_anchor;

			    /*
			     * If we have the memory, create a fake image
			     * tag to replace the <HYPE> tag and process it.
			     */
			    buff = PA_ALLOC(XP_STRLEN(HYPE_TAG_BECOMES)+1);
			    if (buff != NULL)
			    {

				PA_LOCK(str, char *, buff);
				XP_STRCPY(str, HYPE_TAG_BECOMES);
				PA_UNLOCK(buff);

				tmp_tag.type = P_IMAGE;
				tmp_tag.is_end = FALSE;
				tmp_tag.newline_count = tag->newline_count;
				tmp_tag.data = buff;
				tmp_tag.data_len = XP_STRLEN(HYPE_TAG_BECOMES);
				tmp_tag.true_len = 0;
				tmp_tag.lo_data = NULL;
				tmp_tag.next = NULL;
				lo_FormatImage(context, state, &tmp_tag);
				PA_FREE(buff);
			    }

			    /*
			     * Restore the proper current_anchor.
			     */
			    state->current_anchor = hold_current_anchor;
			}
#endif /* XP_UNIX */
			break;

		/*
		 * Insert a horizontal rule element.
		 * They always take up an entire line.
		 */
		case P_HRULE:
			if (state->in_paragraph != FALSE)
			{
				lo_CloseParagraph(context, state);
			}

			/*
			 * HR now does alignment the same as P, H?, and DIV.
			 * Ignore </hr>.
			 */
			if (tag->is_end == FALSE)
			{
				PA_Block buff;
				char *str;
				int32 alignment;
				lo_AlignStack *aptr;

				lo_SoftLineBreak(context, state, FALSE, 1);

				/*
				 * IF no ALIGN and no container, horizontal
				 * rules default to center alignments.
				 */
				alignment = LO_ALIGN_CENTER;
				buff = lo_FetchParamValue(context, tag, PARAM_ALIGN);
				if (buff != NULL)
				{
					PA_LOCK(str, char *, buff);
					alignment =
					  (int32)lo_EvalDivisionAlignParam(str);
					PA_UNLOCK(buff);
					PA_FREE(buff);
				}
				/*
				 * If there is no specific ALIGN,
				 * default to the container's alignment.
				 */
				else if (state->align_stack != NULL)
				{
					alignment =
						state->align_stack->alignment;
				}
				lo_PushAlignment(state, tag, alignment);

				lo_HorizontalRule(context, state, tag);
				lo_HardLineBreak(context, state, FALSE);

				aptr = lo_PopAlignment(state);
				if (aptr != NULL)
				{
					XP_DELETE(aptr);
				}
			}
			break;

		/*
		 * Experimental!  May be used to implement TABLES
		 */
		case P_CELL:
			if (state->in_paragraph != FALSE)
			{
				lo_CloseParagraph(context, state);
			}
			if (tag->is_end == FALSE)
			{
				lo_SoftLineBreak(context, state, FALSE, 1);
				lo_BeginCell(context, state, tag);
			}
			else
			{
				if (state->current_cell != NULL)
				{
					lo_SoftLineBreak(context, state,
						FALSE, 1);
					lo_EndCell(context, state,
						state->current_cell);
				}
			}
			break;


		/*
		 * The start of a caption in a table
		 */
		case P_CAPTION:
			lo_process_caption_tag(context, state, tag);
			break;

		/*
		 * The start of a header or data cell
		 * in a row in a table
		 */
		case P_TABLE_HEADER:
		case P_TABLE_DATA:
			lo_process_table_cell_tag(context, state, tag);
			break;

		/*
		 * The start of a row in a table
		 */
		case P_TABLE_ROW:
			lo_process_table_row_tag(context, state, tag);
			break;

		/*
		 * The start of a table
		 */
		case P_TABLE:
			lo_process_table_tag(context, state, tag);
			break;

		/*
		 * Experimental!  Everything in these tags is a complete
		 * separate sub document, it should be formatted, and the
		 * resulting entire document placed just like an image.
		 */
		case P_SUBDOC:
#ifdef NETSCAPE1_5
			if (state->in_paragraph != FALSE)
			{
				lo_CloseParagraph(context, state);
			}
			if (tag->is_end == FALSE)
			{
				lo_BeginSubDoc(context, state, tag);
			}
			else
			{
				lo_TopState *top_state;
				lo_DocState *old_state;
				int32 doc_id;

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

				if ((state != old_state)&&
					(state->current_ele != NULL))
				{
					lo_EndSubDoc(context, state,
						old_state, state->current_ele);
					state->sub_state = NULL;
				}
			}
#endif /* NETSCAPE1_5 */
			break;

		/*
		 * Begin a grid.  MUST be the very first thing inside
		 * this HTML document.  After a grid is started, all non-grid
		 * tags will be ignored!
		 * Also, right now the text and postscript FEs can't
		 * handle grids, so we block them.
		 */
		case P_GRID:
			if ((context->type != MWContextText)&&
			    (context->type != MWContextPostScript)&&
				(EDT_IS_EDITOR(context) == FALSE)&&
			    (state->top_state->nothing_displayed != FALSE)&&
			    (state->top_state->in_body == FALSE)&&
			    (state->end_last_line == NULL)&&
			    (state->line_list == NULL)&&
			    (state->float_list == NULL)&&
			    (state->top_state->the_grid == NULL))
			{
			    if (tag->is_end == FALSE)
			    {
				if (state->current_grid == NULL)
				{
					state->top_state->is_grid = TRUE;
					/*
					 * We may be restoring a grid from
					 * history.  If so we recreate it
					 * from saved data, and ignore all
					 * the rest of the tags in this
					 * document.  This happens because
					 * we will set the_grid and not
					 * set current_grid, so all
					 * grid tags are ignored, and since
					 * we have set is_grid, all non-grid
					 * tags will be ignored.
					 *
					 * Only recreate if main window
					 * size has not changed.
					 */
					if (state->top_state->savedData.Grid->the_grid != NULL)
					{
					    lo_SavedGridData *savedGridData;
					    int32 width, height;

					    width = state->win_width;
					    height = state->win_height;
					    FE_GetFullWindowSize(context,
						&width, &height);

					    savedGridData =
					      state->top_state->savedData.Grid;
					    if ((savedGridData->main_width ==
							width)&&
					        (savedGridData->main_height ==
							height))
					    {
						lo_RecreateGrid(context, state,
						    savedGridData->the_grid);
						savedGridData->the_grid =NULL;
					    }
					    else
					    {
						/*
						 * If window size has changed
						 * on the grid we are restoring
						 * we process normally, but
						 * hang on to the old data so
						 * we can restore the grid cell
						 * contents once new sizes are
						 * calculated.
						 */
						state->top_state->old_grid =
						    savedGridData->the_grid;
						savedGridData->the_grid =NULL;

						lo_BeginGrid(context, state,
								tag);
					    }
					}
					else
					{
					    lo_BeginGrid(context, state, tag);
					}
				}
				else
				{
					lo_BeginSubgrid(context, state, tag);
				}
			    }
			    else if ((tag->is_end != FALSE)&&
					(state->current_grid != NULL))
			    {
				lo_GridRec *grid;

				grid = state->current_grid;
				if (grid->subgrid == NULL)
				{
					state->current_grid = NULL;
					lo_EndGrid(context, state, grid);
				}
				else
				{
					lo_EndSubgrid(context, state, grid);
				}
			    }
			}
			break;

		/*
		 * A grid cell can only happen inside a grid
		 */
		case P_GRID_CELL:
			if ((state->top_state->nothing_displayed != FALSE)&&
			    (state->top_state->in_body == FALSE)&&
			    (state->end_last_line == NULL)&&
			    (state->line_list == NULL)&&
			    (state->float_list == NULL)&&
			    (state->current_grid != NULL))
			{
				if (tag->is_end == FALSE)
				{
					lo_BeginGridCell(context, state, tag);
				}
			}
			break;

		/*
		 * Begin a map.  Maps define area in client-side
		 * image maps.
		 */
		case P_MAP:
			if ((tag->is_end == FALSE)&&
			    (state->top_state->current_map == NULL))
			{
				lo_BeginMap(context, state, tag);
			}
			else if ((tag->is_end != FALSE)&&
				 (state->top_state->current_map != NULL))
			{
				lo_MapRec *map;

				map = state->top_state->current_map;
				state->top_state->current_map = NULL;
				lo_EndMap(context, state, map);
			}
			break;

		/*
		 * An area definition, can only happen inside a MAP
		 * definition for a client-side image map.
		 */
		case P_AREA:
			if ((state->top_state->current_map != NULL)&&
				(tag->is_end == FALSE))
			{
				lo_BeginMapArea(context, state, tag);
			}
			break;

		/*
		 * Shortcut and old historic tag to auto-place
		 * a simple single item query form.
		 */
		case P_INDEX:
			/*
			 * No forms in the scrolling document
			 * hack
			 */
			if (state->top_state->scrolling_doc != FALSE)
			{
				break;
			}

			if ((tag->is_end == FALSE)&&
				(state->top_state->in_form == FALSE))
			{
				if (state->in_paragraph != FALSE)
				{
					lo_CloseParagraph(context, state);
				}
				lo_ProcessIsIndexTag(context, state, tag);
			}
			break;

		/*
		 * Begin a form.
		 * Form cannot be nested!
		 */
		case P_FORM:
			/*
			 * No forms in the scrolling document
			 * hack
			 */
			if (state->top_state->scrolling_doc != FALSE)
			{
				break;
			}

			if (state->in_paragraph != FALSE)
			{
				lo_CloseParagraph(context, state);
			}
			if (tag->is_end == FALSE)
			{
				/*
				 * Sorry, no nested forms
				 */
				if (state->top_state->in_form == FALSE)
				{
					lo_SoftLineBreak(context, state,
						FALSE, 2);
					lo_BeginForm(context, state, tag);
				}
			}
			else if (state->top_state->in_form != FALSE)
			{
				lo_EndForm(context, state);
				lo_SoftLineBreak(context, state, FALSE, 2);
			}
			break;

		/*
		 * Deceptive simple form element tag.  Can only be inside a
		 * form.  Really multiplies into one of many possible
		 * input elements.
		 */
		case P_INPUT:
			/*
			 * Input tags only inside an open form.
			 */
			if ((state->top_state->in_form != FALSE)&&
				(tag->is_end == FALSE))
			{
				if (state->in_paragraph != FALSE)
				{
					lo_CloseParagraph(context, state);
				}
				lo_ProcessInputTag(context, state, tag);
			}
			break;

		/*
		 * A multi-line text input form element.  It encloses text
		 * that will be its default text.
		 */
		case P_TEXTAREA:
			/*
			 * Textarea tags only inside an open form.
			 */
			if (state->top_state->in_form != FALSE)
			{
			    if (state->in_paragraph != FALSE)
			    {
				lo_CloseParagraph(context, state);
			    }
			    if (tag->is_end == FALSE)
			    {
				if ((state->cur_ele_type == LO_TEXT)&&
					(state->line_buf_len != 0))
				{
					lo_FlushLineBuffer(context, state);
				}

				state->line_buf_len = 0;
				state->text_divert = tag->type;
				lo_BeginTextareaTag(context, state, tag);
			    }
			    else if (state->text_divert == tag->type)
			    {
				PA_Block buff;
				int32 len;
				int32 new_len;

				if (state->line_buf_len != 0)
				{
				    PA_LOCK(tptr2, char *,
					state->line_buf);
				    /*
				     * Don't do this, they were already
				     * expanded by the parser.
				    (void)pa_ExpandEscapes(tptr2,
					state->line_buf_len, &len, TRUE);
				     */
				    len = state->line_buf_len;
				    buff = lo_ConvertToFELinebreaks(tptr2, len,
						&new_len);
				    PA_UNLOCK(state->line_buf);
				}
				else
				{
				    buff = NULL;
				    new_len = 0;
				}
				lo_EndTextareaTag(context, state, buff);
				state->line_buf_len = 0;
				state->text_divert = P_UNKNOWN;
			    }
			}
			break;

		/*
		 * Option tag can only occur inside a select inside a form
		 */
		case P_OPTION:
			/*
			 * Select tags only inside an open select
			 * in an open form.
			 */
			if ((state->top_state->in_form != FALSE)&&
				(tag->is_end == FALSE)&&
				(state->current_ele != NULL)&&
				(state->current_ele->type == LO_FORM_ELE)&&
                                ((state->current_ele->lo_form.element_data->type
                                        == FORM_TYPE_SELECT_ONE)||
                                 (state->current_ele->lo_form.element_data->type
                                        == FORM_TYPE_SELECT_MULT)))
			{
			    if (state->in_paragraph != FALSE)
			    {
				lo_CloseParagraph(context, state);
			    }
			    if (state->text_divert == P_OPTION)
			    {
				PA_Block buff;
				int32 len;

				if (state->line_buf_len != 0)
				{
				    PA_LOCK(tptr2, char *,
					state->line_buf);
				    /*  
				     * Don't do this, they were already
				     * expanded by the parser.
				    (void)pa_ExpandEscapes(tptr2,
					state->line_buf_len, &len, TRUE);
				     */
				    len = state->line_buf_len;
				    len = lo_CleanTextWhitespace(tptr2, len);
				    buff = PA_ALLOC(len + 1);
				    if (buff != NULL)
				    {
					PA_LOCK(tptr, char *, buff);
					XP_BCOPY(tptr2, tptr, len);
					tptr[len] = '\0';
					PA_UNLOCK(buff);
				    }
				    PA_UNLOCK(state->line_buf);
				}
				else
				{
				    buff = NULL;
				    len = 0;
				}
				lo_EndOptionTag(context, state, buff);
			    }
			    state->line_buf_len = 0;
			    lo_BeginOptionTag(context, state, tag);
			    state->text_divert = tag->type;
			}
			break;

		/*
		 * Select tag, either an option menu or a scrolled
		 * list inside a form.  Lets you chose N of Many.
		 */
		case P_SELECT:
			/*
			 * Select tags only inside an open form.
			 */
			if (state->top_state->in_form != FALSE)
			{
			    /*
			     * Because SELECT doesn't automatically flush,
			     * we must do it here if this is the start of a
			     * select.
			     */
			    if ((tag->is_end == FALSE)&&
				(state->current_ele == NULL)&&
				(state->cur_ele_type == LO_TEXT)&&
				(state->line_buf_len != 0))
			    {
				lo_FlushLineBuffer(context, state);
			    }

			    if (state->in_paragraph != FALSE)
			    {
				lo_CloseParagraph(context, state);
			    }
			    if ((tag->is_end == FALSE)&&
				(state->current_ele == NULL))
			    {
				/*
				 * Don't allow text between beginning of select
				 * and beginning of first option.
				 */
			        state->text_divert = tag->type;

				lo_BeginSelectTag(context, state, tag);
			    }
			    else if ((state->current_ele != NULL)&&
				(state->current_ele->type == LO_FORM_ELE)&&
				((state->current_ele->lo_form.element_data->type
					== FORM_TYPE_SELECT_ONE)||
				 (state->current_ele->lo_form.element_data->type
					== FORM_TYPE_SELECT_MULT)))
			    {
			        if (state->text_divert == P_OPTION)
			        {
				    PA_Block buff;
				    int32 len;

				    if (state->line_buf_len != 0)
				    {
				        PA_LOCK(tptr2, char *,
					    state->line_buf);
					/*  
					 * Don't do this, they were already
					 * expanded by the parser.
				        (void)pa_ExpandEscapes(tptr2,
					    state->line_buf_len, &len, TRUE);
					 */
					len = state->line_buf_len;
				        len = lo_CleanTextWhitespace(tptr2,len);
				        buff = PA_ALLOC(len + 1);
				        if (buff != NULL)
				        {
					    PA_LOCK(tptr, char *, buff);
					    XP_BCOPY(tptr2, tptr, len);
					    tptr[len] = '\0';
					    PA_UNLOCK(buff);
				        }
				        PA_UNLOCK(state->line_buf);
				    }
				    else
				    {
				        buff = NULL;
				        len = 0;
				    }
				    lo_EndOptionTag(context, state, buff);
			        }
			        state->line_buf_len = 0;
			        state->text_divert = P_UNKNOWN;

				lo_EndSelectTag(context, state);
			    }
			}
			break;

		/*
		 * Place an embedded object.
		 * Just reserve an area of layout for that object
		 * to use as it wishes.
		 */
		case P_EMBED:
			if (tag->is_end == FALSE)
			{
				/*
				 * If we have started loading an EMBED we are
				 * out if the HEAD section of the HTML
				 * and into the BODY
				 */
				state->top_state->in_head = FALSE;
				state->top_state->in_body = TRUE;
				lo_FormatEmbed(context, state, tag);
			}
			break;

		/*
		 * Generate a public/private key pair.  The user sees
		 * a selection of key sizes to choose from; an ascii
		 * representation of the public key becomes the form
		 * field value.
		 */
		case P_KEYGEN:
			/*
			 * Keygen tags only inside an open form.
			 */
			if ((state->top_state->in_form != FALSE)&&
			    (tag->is_end == FALSE))
			{
				if (state->in_paragraph != FALSE)
				{
					lo_CloseParagraph(context, state);
				}
				lo_ProcessKeygenTag(context, state, tag);
			}
			break;

		/*
		 * Process Mocha script source in a <SCRIPT LANGUAGE="Mocha">
		 * ...</SCRIPT> container.  Drop all unknown language SCRIPT
		 * contents on the floor.  Process SRC=<url> attributes for
		 * known languages.
		 */
		case P_SCRIPT:
#ifdef EDITOR
			if( EDT_IS_EDITOR( context ) )
			{
				goto process_server;
			}
#endif

#ifdef MOCHA
			lo_ProcessScriptTag(context, state, tag);
			break;
#else
			goto unknown;
#endif

		/*
		 * This is the NEW HTML-WG approved embedded JAVA
		 * application.
		 */
#ifdef JAVA
		case P_JAVA_APPLET:
		    /*
		     * If the user has disabled Java, act like it we don't
		     * understand the APPLET tag.
		     */
		    if (LJ_GetJavaEnabled() != FALSE)
		    {
			if (tag->is_end == FALSE)
			{
			    /*
			     * If we open a new java applet without closing
			     * an old one, force a close of the old one
			     * first.
			     * Now open the new java applet.
			     */
			    if (state->current_java != NULL)
			    {
				lo_CloseJavaApp(context, state,
						state->current_java);
				state->current_java = NULL;

				lo_FormatJavaApp(context, state, tag);
			    }
			    /*
			     * Else this is a new java applet, open it.
			     */
			    else
			    {
				/*
				 * If we have started loading an APPLET we are
				 * out if the HEAD section of the HTML
				 * and into the BODY
				 */
				state->top_state->in_head = FALSE;
				state->top_state->in_body = TRUE;
				
				lo_FormatJavaApp(context, state, tag);
			    }
			}
			else
			{
			    /*
			     * Only close a java app if it exists.
			     */
			    if (state->current_java != NULL)
			    {
				lo_CloseJavaApp(context, state,
						state->current_java);
				state->current_java = NULL;
			    }
			}
		    }
			break;

		/*
		 * This is the NEW HTML-WG approved parameter tag
		 * for embedded JAVA applications.
		 */
		case P_PARAM:
			if ((tag->is_end == FALSE)&&
			    (state->current_java != NULL))
			{
				lo_JavaAppParam(context, state, tag);
			}
			break;
#endif

		/*
		 * Place an image.  Special because if layout of all
		 * other tags was blocked, image tags are still
		 * partially processed at that time.
		 */
		case P_IMAGE:
		case P_NEW_IMAGE:
			if (tag->is_end == FALSE)
			{
				/*
				 * If we have started loading an IMAGE we are
				 * out if the HEAD section of the HTML
				 * and into the BODY
				 */
				state->top_state->in_head = FALSE;
				state->top_state->in_body = TRUE;

				if (tag->lo_data != NULL)
				{
					lo_PartialFormatImage(context, state,tag);
				}
				else
				{
					lo_FormatImage(context, state, tag);
				}
			}
			break;

		/*
		 * Paragraph break, a single line of whitespace.
		 */
		case P_PARAGRAPH:
			if (tag->is_end == FALSE)
			{
				PA_Block buff;
				char *str;

				if (state->in_paragraph != FALSE)
				{
					lo_CloseParagraph(context, state);
				}

				lo_SoftLineBreak(context, state, FALSE, 2);

				buff = lo_FetchParamValue(context, tag, PARAM_ALIGN);
				if (buff != NULL)
				{
					int32 alignment;

					PA_LOCK(str, char *, buff);
					alignment =
					  (int32)lo_EvalDivisionAlignParam(str);
					PA_UNLOCK(buff);
					PA_FREE(buff);

					lo_PushAlignment(state, tag, alignment);
				}
				state->in_paragraph = TRUE;
			}
			else
			{
				lo_SoftLineBreak(context, state, FALSE, 2);
				if (state->in_paragraph != FALSE)
				{
					lo_CloseParagraph(context, state);
				}
			}
			break;

		/*
		 * Division.  General alignment text block
		 */
		case P_DIVISION:
			lo_SoftLineBreak(context, state, FALSE, 1);
			if (tag->is_end == FALSE)
			{
				PA_Block buff;
				char *str;
				int32 alignment;

				alignment = LO_ALIGN_LEFT;
				buff = lo_FetchParamValue(context, tag, PARAM_ALIGN);
				if (buff != NULL)
				{
					PA_LOCK(str, char *, buff);
					alignment =
					  (int32)lo_EvalDivisionAlignParam(str);
					PA_UNLOCK(buff);
					PA_FREE(buff);
				}
				lo_PushAlignment(state, tag, alignment);
			}
			else
			{
				lo_AlignStack *aptr;

				aptr = lo_PopAlignment(state);
				if (aptr != NULL)
				{
					XP_DELETE(aptr);
				}
			}
			break;

		/*
		 * Just move the left and right margins in for this paragraph
		 * of text.
		 */
		case P_BLOCKQUOTE:
			if (state->in_paragraph != FALSE)
			{
				lo_CloseParagraph(context, state);
			}
			if (tag->is_end == FALSE)
			{
				lo_SoftLineBreak(context, state, FALSE, 2);

/* THIS BREAKS FLOATING IMAGES IN BLOCKQUOTES
				state->list_stack->old_left_margin =
					state->left_margin;
				state->list_stack->old_right_margin =
					state->right_margin;
*/
				lo_PushList(state, tag);
				state->left_margin += LIST_MARGIN_INC;
				state->right_margin -= LIST_MARGIN_INC;
				state->x = state->left_margin;
				state->list_stack->old_left_margin =
					state->left_margin;
				state->list_stack->old_right_margin =
					state->right_margin;
			}
			else
			{
				lo_ListStack *lptr;

				lptr = lo_PopList(state, tag);
				if (lptr != NULL)
				{
					XP_DELETE(lptr);
				}
				state->left_margin =
					state->list_stack->old_left_margin;
				state->right_margin =
					state->list_stack->old_right_margin;

				/*
				 * Reset the margins properly in case
				 * there is a right-aligned image here.
				 */
				lo_FindLineMargins(context, state);
				if (state->linefeed_state >= 2)
				{
					lo_FindLineMargins(context, state);
				}
				else
				{
					lo_SoftLineBreak(context, state, FALSE, 2);
				}

				state->x = state->left_margin;
			}
			break;

		/*
		 * An internal tag to allow libmocha/lm_doc.c's
		 * doc_open_stream to get the new document's top
		 * state struct early.
		 */
		case P_NSCP_OPEN:
			break;

		/*
		 * An internal tag to allow us to merge two
		 * possibly incomplete HTML docs as neatly
		 * as possible.
		 */
		case P_NSCP_CLOSE:
			if (tag->is_end == FALSE)
			{
				LO_CloseAllTags(context);
			}
			break;

		/*
		 * Normally unknown tags are just ignored.
		 * But if the unknown tag is in the head of a document, we
		 * assume it is a tag that can contain content, and ignore
		 * that content.
		 * WARNING:  In special cases with omitted </head> tags,
		 * this can result in the loss of body data!
		 */
		case P_UNKNOWN:
#ifndef MOCHA
		unknown:
#endif
			if (state->top_state->in_head != FALSE)
			{
			    char *tdata;
			    char *tptr;
			    char tchar;

			    PA_LOCK(tdata, char *, tag->data);

			    /*
			     * Throw out non-tags like <!doctype>
			     */
			    if ((tag->data_len < 1)||(tdata[0] == '!'))
			    {
				PA_UNLOCK(tag->data);
				break;
			    }

			    /*
			     * Remove the unknown tag name from any attributes
			     * it might have.
			     */
			    tptr = tdata;
			    while ((!XP_IS_SPACE(*tptr))&&(*tptr != '>'))
			    {
				tptr++;
			    }
			    tchar = *tptr;
			    *tptr = '\0';

			    /*
			     * Save the unknown tag name to match to its
			     * end tag.
			     */
			    if (tag->is_end == FALSE)
			    {
				if (state->top_state->unknown_head_tag == NULL)
				{
				    state->top_state->unknown_head_tag =
					XP_STRDUP(tdata);
				}
			    }
			    /*
			     * Else if we already have a saved start and this
			     * is the matching end.  Clear the knowledge
			     * that we are in an unknown head tag.
			     */
			    else
			    {
				if ((state->top_state->unknown_head_tag != NULL)
				    &&(XP_STRCASECMP(state->top_state->unknown_head_tag, tdata) == 0))
				{
				    XP_FREE(state->top_state->unknown_head_tag);
				    state->top_state->unknown_head_tag = NULL;
				}
			    }

			    *tptr = tchar;
			    PA_UNLOCK(tag->data);
			}
			break;

		default:
			break;
	}

#ifdef EDITOR
	/*  
	 * The editor needs all text elements to be individual text elements.
	 * Force the line buffer to be flushed so each individual text item
	 * remains such.
	 */
	if (EDT_IS_EDITOR(context) &&
	    (state->cur_ele_type == LO_TEXT)&&(state->line_buf_len != 0))
	{
		lo_FlushLineBuffer(context, state);
	}
#endif

}

