/* -*- Mode: C; tab-width: 4; tabs-indent-mode: t -*-
 */
#include "xp.h"
#include "net.h"
#include "glhist.h"
#include "shist.h"
#include "merrors.h"
#include "il.h"
#include "pa_parse.h"
#include "layout.h"
#include "edt.h"
#include "libmocha.h"

#ifdef HTML_CERTIFICATE_SUPPORT
#include "cert.h"
#endif

extern int MK_OUT_OF_MEMORY;


#ifdef PROFILE
#pragma profile on
#endif

#ifdef TEST_16BIT
#define XP_WIN16
#endif /* TEST_16BIT */

#ifdef XP_WIN16
#define SIZE_LIMIT		32000
#endif /* XP_WIN16 */


#define DEF_LINE_HEIGHT		20
#define URL_LIST_INC		20
#define LINE_INC		100


typedef struct lo_StateList_struct {
	int32 doc_id;
	lo_TopState *state;
	struct lo_StateList_struct *next;
} lo_StateList;

static lo_StateList *StateList = NULL;

LO_Color lo_master_colors[] = {
	{LO_DEFAULT_BG_RED, LO_DEFAULT_BG_GREEN, LO_DEFAULT_BG_BLUE},
	{LO_DEFAULT_FG_RED, LO_DEFAULT_FG_GREEN, LO_DEFAULT_FG_BLUE},
	{LO_UNVISITED_ANCHOR_RED, LO_UNVISITED_ANCHOR_GREEN,
	 LO_UNVISITED_ANCHOR_BLUE},
	{LO_VISITED_ANCHOR_RED, LO_VISITED_ANCHOR_GREEN,
	 LO_VISITED_ANCHOR_BLUE},
	{LO_SELECTED_ANCHOR_RED, LO_SELECTED_ANCHOR_GREEN,
	 LO_SELECTED_ANCHOR_BLUE}
};
static char *Master_backdrop_url = NULL;
static Bool UserOverride = FALSE;

static void lo_process_deferred_image_info(void *closure);
void lo_DisplayLine(MWContext *context, lo_DocState *state, int32 line_num,
	int32 x, int32 y, uint32 w, uint32 h, Bool can_clip);
lo_SavedEmbedListData *	lo_NewDocumentEmbedListData(void);
lo_SavedGridData *		lo_NewDocumentGridData(void);


void
LO_SetDefaultBackdrop(char *url)
{
	if ((url != NULL)&&(*url != '\0'))
	{
		Master_backdrop_url = XP_STRDUP(url);
	}
	else
	{
		Master_backdrop_url = NULL;
	}
}


void
LO_SetUserOverride(Bool override)
{
	UserOverride = override;
}


void
LO_SetDefaultColor(intn type, uint8 red, uint8 green, uint8 blue)
{
	LO_Color *color;

	if (type < sizeof lo_master_colors / sizeof lo_master_colors[0])
	{
		color = &lo_master_colors[type];
		color->red = red;
		color->green = green;
		color->blue = blue;
	}
}

lo_TopState *
lo_NewTopState(MWContext *context, char *url)
{
	lo_TopState *top_state;
	char *name_target;
	LO_TextAttr **text_attr_hash;
	LO_AnchorData **anchor_array;
	int32 i;

	top_state = XP_NEW(lo_TopState);
	if (top_state == NULL)
	{
		return(NULL);
	}

#ifdef MEMORY_ARENAS
    if ( EDT_IS_EDITOR(context) ) {
        top_state->current_arena = NULL;
    }
    else
    {
	    lo_InitializeMemoryArena(top_state);
	    if (top_state->first_arena == NULL)
	    {
		    XP_DELETE(top_state);
		    return(NULL);
	    }
    }
#endif /* MEMORY_ARENAS */

	top_state->tags = NULL;
	top_state->tags_end = &top_state->tags;
	top_state->insecure_images = FALSE;
	top_state->out_of_memory = FALSE;
	top_state->force_reload = NET_DONT_RELOAD;
#ifdef MOCHA
	top_state->script_lineno = 0;
	top_state->in_script = SCRIPT_TYPE_NOT;
	top_state->resize_reload = FALSE;
#endif
	top_state->doc_state = NULL;

	if (url == NULL)
	{
		top_state->url = NULL;
		top_state->base_url = NULL;
	}
	else
	{
		top_state->url = XP_STRDUP(url);
		top_state->base_url = XP_STRDUP(url);
	}
	top_state->base_target = NULL;

	/*
	 * This is the special named anchor we are jumping to in this
	 * document, it changes the starting display position.
	 */
	name_target = NET_ParseURL(top_state->url, GET_HASH_PART);
	if ((name_target[0] != '#')||(name_target[1] == '\0'))
	{
		XP_FREE(name_target);
		top_state->name_target = NULL;
	}
	else
	{
		top_state->name_target = name_target;
	}

	top_state->element_id = 0;
	top_state->layout_blocking_element = NULL;
	top_state->backdrop_image = NULL;
	top_state->have_a_bg_url = FALSE;
	top_state->doc_specified_bg = FALSE;
	top_state->nothing_displayed = TRUE;
	top_state->in_head = TRUE;
	top_state->in_body = FALSE;
	top_state->have_title = FALSE;
	top_state->scrolling_doc = FALSE;
	top_state->is_grid = FALSE;
	top_state->in_nogrids = FALSE;
	top_state->in_noembed = FALSE;
	top_state->in_noscript = FALSE;
	top_state->in_applet = FALSE;
	top_state->unknown_head_tag = NULL;
	top_state->the_grid = NULL;
	top_state->old_grid = NULL;

    top_state->embed_list = NULL;
    top_state->applet_list = NULL;

	top_state->map_list = NULL;
	top_state->current_map = NULL;

#ifdef MOCHA
    top_state->image_list = top_state->last_image = NULL;
    top_state->image_count = 0;
#endif

	top_state->form_list = NULL;
	top_state->current_form_num = 0;
	top_state->in_form = FALSE;
	top_state->savedData.FormList = NULL;
	top_state->savedData.EmbedList = NULL;
	top_state->savedData.Grid = NULL;
#ifdef MOCHA
	top_state->savedData.OnLoad = NULL;
	top_state->savedData.OnUnload = NULL;
	top_state->savedData.OnFocus = NULL;
	top_state->savedData.OnBlur = NULL;
#endif
	top_state->embed_count = 0;

	top_state->total_bytes = 0;
	top_state->current_bytes = 0;
	top_state->layout_bytes = 0;
	top_state->script_bytes = 0;
	top_state->layout_percent = 0;

	top_state->text_attr_hash = XP_ALLOC_BLOCK(FONT_HASH_SIZE *
		sizeof(LO_TextAttr *));
	if (top_state->text_attr_hash == NULL)
	{
		XP_DELETE(top_state);
		return(NULL);
	}
	XP_LOCK_BLOCK(text_attr_hash, LO_TextAttr **,top_state->text_attr_hash);
	for (i=0; i<FONT_HASH_SIZE; i++)
	{
		text_attr_hash[i] = NULL;
	}
	XP_UNLOCK_BLOCK(top_state->text_attr_hash);

	top_state->font_face_array = NULL;
	top_state->font_face_array_len = 0;
	top_state->font_face_array_size = 0;

	top_state->url_list = XP_ALLOC_BLOCK(URL_LIST_INC * sizeof(LO_AnchorData *));
	if (top_state->url_list == NULL)
	{
		XP_FREE_BLOCK(top_state->text_attr_hash);
		XP_DELETE(top_state);
		return(NULL);
	}
	top_state->url_list_size = URL_LIST_INC;
	/* url_list is a Handle of (LO_AnchorData*) so the pointer is a (LO_AnchorData**) */
	XP_LOCK_BLOCK(anchor_array, LO_AnchorData **, top_state->url_list);
	for (i=0; i < URL_LIST_INC; i++)
	{
		anchor_array[i] = NULL;
	}
	XP_UNLOCK_BLOCK(top_state->url_list);
	top_state->url_list_len = 0;

#ifdef XP_WIN16
	{
		XP_Block *ulist_array;

		top_state->ulist_array = XP_ALLOC_BLOCK(sizeof(XP_Block));
		if (top_state->ulist_array == NULL)
		{
			XP_FREE_BLOCK(top_state->text_attr_hash);
			XP_FREE_BLOCK(top_state->url_list);
			XP_DELETE(top_state);
			return(NULL);
		}
		XP_LOCK_BLOCK(ulist_array, XP_Block *, top_state->ulist_array);
		ulist_array[0] = top_state->url_list;
		XP_UNLOCK_BLOCK(top_state->ulist_array);
		top_state->ulist_array_size = 1;
	}
#endif /* XP_WIN16 */

	top_state->recycle_list = NULL;
	top_state->trash = NULL;
#ifdef MOCHA
	top_state->doc_data = NULL;
	top_state->script_state = NULL;
	top_state->mocha_mark = FALSE;
	top_state->mocha_write_point = NULL;
	top_state->mocha_write_level = 0;
#ifdef MOCHA_CACHES_WRITES
	top_state->mocha_write_stream = NULL;
#endif
	top_state->mocha_loading_applets_count = 0;
	top_state->mocha_loading_embeds_count = 0;
	top_state->mocha_has_onload = FALSE;
#endif

#ifdef HTML_CERTIFICATE_SUPPORT
	top_state->cert_list = NULL;
#endif

	return(top_state);
}


/*************************************
 * Function: lo_NewLayout
 *
 * Description: This function creates and initializes a new
 *	layout state structure to be used for all state information
 *	about this document (or sub-document) during its lifetime.
 *
 * Params: Window context,
 *	the width and height of the
 *	window we are formatting to.
 *
 * Returns: A pointer to a lo_DocState structure.
 *	Returns a NULL on error (such as out of memory);
 *************************************/
lo_DocState *
lo_NewLayout(MWContext *context, int32 width, int32 height,
	int32 margin_width, int32 margin_height, lo_DocState* clone_state )
{
	LO_Element **line_array;
	lo_TopState *top_state;
	lo_DocState *state;
#ifdef XP_WIN16
	int32 i;
	PA_Block *sblock_array;
#endif /* XP_WIN16 */
	int32 doc_id;

	doc_id = XP_DOCID(context);
	top_state = lo_FetchTopState(doc_id);
	if (top_state == NULL)
	{
		return(NULL);
	}

	state = XP_NEW(lo_DocState);
	if (state == NULL)
	{
		top_state->out_of_memory = TRUE;
		return(NULL);
	}
	state->top_state = top_state;

	state->subdoc_tags = NULL;
	state->subdoc_tags_end = NULL;

	/*
	 * Set colors early so default text is correct.
	 */

	if( clone_state == NULL ){
		state->text_fg.red = lo_master_colors[LO_COLOR_FG].red;
		state->text_fg.green = lo_master_colors[LO_COLOR_FG].green;
		state->text_fg.blue = lo_master_colors[LO_COLOR_FG].blue;

		state->text_bg.red = lo_master_colors[LO_COLOR_BG].red;
		state->text_bg.green = lo_master_colors[LO_COLOR_BG].green;
		state->text_bg.blue = lo_master_colors[LO_COLOR_BG].blue;

		state->anchor_color.red = lo_master_colors[LO_COLOR_LINK].red;
		state->anchor_color.green = lo_master_colors[LO_COLOR_LINK].green;
		state->anchor_color.blue = lo_master_colors[LO_COLOR_LINK].blue;

		state->visited_anchor_color.red = lo_master_colors[LO_COLOR_VLINK].red;
		state->visited_anchor_color.green = lo_master_colors[LO_COLOR_VLINK].green;
		state->visited_anchor_color.blue = lo_master_colors[LO_COLOR_VLINK].blue;

		state->active_anchor_color.red = lo_master_colors[LO_COLOR_ALINK].red;
		state->active_anchor_color.green = lo_master_colors[LO_COLOR_ALINK].green;
		state->active_anchor_color.blue = lo_master_colors[LO_COLOR_ALINK].blue;
	}
	else
	{
		state->text_fg.red = clone_state->text_fg.red;
		state->text_fg.green = clone_state->text_fg.green;
		state->text_fg.blue = clone_state->text_fg.blue;

		state->text_bg.red = clone_state->text_bg.red;
		state->text_bg.green = clone_state->text_bg.green;
		state->text_bg.blue = clone_state->text_bg.blue;

		state->anchor_color.red = clone_state->anchor_color.red;
		state->anchor_color.green = clone_state->anchor_color.green;
		state->anchor_color.blue = clone_state->anchor_color.blue;

		state->visited_anchor_color.red = clone_state->visited_anchor_color.red;
		state->visited_anchor_color.green = clone_state->visited_anchor_color.green;
		state->visited_anchor_color.blue = clone_state->visited_anchor_color.blue;

		state->active_anchor_color.red = clone_state->active_anchor_color.red;
		state->active_anchor_color.green = clone_state->active_anchor_color.green;
		state->active_anchor_color.blue = clone_state->active_anchor_color.blue;
	}

	state->win_top = margin_height;
	state->win_bottom = margin_height;
	state->win_width = width;
	state->win_height = height;

	state->base_x = 0;
	state->base_y = 0;
	state->x = 0;
	state->y = 0;
	state->width = 0;
	state->line_num = 1;

	state->win_left = margin_width;
	state->win_right = margin_width;

	state->max_width = state->win_left + state->win_right;

	state->x = state->win_left;
	state->y = state->win_top;

	state->left_margin = state->win_left;
	state->right_margin = state->win_width - state->win_right;
	state->left_margin_stack = NULL;
	state->right_margin_stack = NULL;

	state->break_holder = state->x;
	state->min_width = 0;
	state->text_divert = P_UNKNOWN;
	state->linefeed_state = 2;
	state->delay_align = FALSE;
	state->breakable = TRUE;
	state->preformatted = PRE_TEXT_NO;

	state->in_paragraph = FALSE;

	state->display_blocked = FALSE;
	state->display_blocking_element_id = 0;
	state->display_blocking_element_y = 0;

	state->line_list = NULL;
	state->end_last_line = NULL;
	state->float_list = NULL;

	state->line_array = XP_ALLOC_BLOCK(LINE_INC * sizeof(LO_Element *));
	if (state->line_array == NULL)
	{
		top_state->out_of_memory = TRUE;
		XP_DELETE(state);
		return(NULL);
	}
	XP_LOCK_BLOCK(line_array, LO_Element **, state->line_array);
	line_array[0] = NULL;
	XP_UNLOCK_BLOCK(state->line_array);
	state->line_array_size = LINE_INC;

#ifdef XP_WIN16
	{
		XP_Block *larray_array;

		state->larray_array = XP_ALLOC_BLOCK(sizeof(XP_Block));
		if (state->larray_array == NULL)
		{
			top_state->out_of_memory = TRUE;
			XP_FREE_BLOCK(state->line_array);
			XP_DELETE(state);
			return(NULL);
		}
		XP_LOCK_BLOCK(larray_array, XP_Block *, state->larray_array);
		larray_array[0] = state->line_array;
		XP_UNLOCK_BLOCK(state->larray_array);
		state->larray_array_size = 1;
	}
#endif /* XP_WIN16 */

	state->name_list = NULL;

	state->base_font_size = DEFAULT_BASE_FONT_SIZE;
	state->font_stack = lo_DefaultFont(state, context);
	if (state->font_stack == NULL)
	{
		top_state->out_of_memory = TRUE;
		XP_FREE_BLOCK(state->line_array);
#ifdef XP_WIN16
		XP_FREE_BLOCK(state->larray_array);
#endif /* XP_WIN16 */
		XP_DELETE(state);
		return(NULL);
	}
	state->font_stack->text_attr->size = state->base_font_size;

	state->align_stack = NULL;

	state->list_stack = lo_DefaultList(state);
	if (state->list_stack == NULL)
	{
		top_state->out_of_memory = TRUE;
		XP_FREE_BLOCK(state->line_array);
#ifdef XP_WIN16
		XP_FREE_BLOCK(state->larray_array);
#endif /* XP_WIN16 */
		XP_DELETE(state->font_stack);
		XP_DELETE(state);
		return(NULL);
	}

	state->text_info.max_width = 0;
	state->text_info.ascent = 0;
	state->text_info.descent = 0;
	state->text_info.lbearing = 0;
	state->text_info.rbearing = 0;

	/*
	 * To initialize the default line height, we need to
	 * jump through hoops here to get the front end
	 * to tell us the default fonts line height.
	 */
	{
		LO_TextStruct tmp_text;
		PA_Block buff;
		char *str;

		memset (&tmp_text, 0, sizeof (tmp_text));
		buff = PA_ALLOC(1);
		if (buff == NULL)
		{
			top_state->out_of_memory = TRUE;
			XP_FREE_BLOCK(state->line_array);
#ifdef XP_WIN16
			XP_FREE_BLOCK(state->larray_array);
#endif /* XP_WIN16 */
			XP_DELETE(state->font_stack);
			XP_DELETE(state);
			return(NULL);
		}
		PA_LOCK(str, char *, buff);
		str[0] = ' ';
		PA_UNLOCK(buff);
		tmp_text.text = buff;
		tmp_text.text_len = 1;
		tmp_text.text_attr = state->font_stack->text_attr;
		FE_GetTextInfo(context, &tmp_text, &(state->text_info));
		PA_FREE(buff);

		state->default_line_height = state->text_info.ascent +
			state->text_info.descent;
	}
	if (state->default_line_height <= 0)
	{
		state->default_line_height = FEUNITS_Y(DEF_LINE_HEIGHT,context);
	}

	state->line_buf_size = 0;
	state->line_buf = PA_ALLOC(LINE_BUF_INC * sizeof(char));
	if (state->line_buf == NULL)
	{
		top_state->out_of_memory = TRUE;
		XP_FREE_BLOCK(state->line_array);
#ifdef XP_WIN16
		XP_FREE_BLOCK(state->larray_array);
#endif /* XP_WIN16 */
		XP_DELETE(state->font_stack);
		XP_DELETE(state);
		return(NULL);
	}
	state->line_buf_size = LINE_BUF_INC;
	state->line_buf_len = 0;

	state->baseline = 0;
	state->line_height = 0;
	state->break_pos = -1;
	state->break_width = 0;
	state->last_char_CR = FALSE;
	state->trailing_space = FALSE;
	state->at_begin_line = TRUE;

	state->old_break = NULL;
	state->old_break_pos = -1;
	state->old_break_width = 0;

	state->current_named_anchor = NULL;
	state->current_anchor = NULL;

	state->cur_ele_type = LO_NONE;
	state->current_ele = NULL;
	state->current_java = NULL;

	state->current_table = NULL;
	state->current_grid = NULL;
	state->current_multicol = NULL;

	/*
	 * These values are saved into the (sub)doc here
	 * so that if it gets Relayout() later, we know where
	 * to reset the from counts to.
	 */
	if (top_state->form_list != NULL)
	{
		lo_FormData *form_list;

		form_list = top_state->form_list;
		state->form_ele_cnt = form_list->form_ele_cnt;
		state->form_id = form_list->id;
	}
	else
	{
		state->form_ele_cnt = 0;
		state->form_id = 0;
	}
	state->start_in_form = top_state->in_form;
	if (top_state->savedData.FormList != NULL)
	{
		lo_SavedFormListData *all_form_ele;

		all_form_ele = top_state->savedData.FormList;
		state->form_data_index = all_form_ele->data_index;
	}
	else
	{
		state->form_data_index = 0;
	}
	
	/*
	 * Remember number of embeds we had before beginning this
	 * (sub)doc. This information is used in laytable.c so
	 * it can preserve embed indices across a relayout.
	 */
	state->embed_count_base = top_state->embed_count;

	/*
	 * Ditto for anchors.
	 */
	state->url_count_base = top_state->url_list_len;

	state->must_relayout_subdoc = FALSE;
	state->allow_percent_width = TRUE;
	state->allow_percent_height = TRUE;
	state->is_a_subdoc = SUBDOC_NOT;
	state->current_subdoc = 0;
	state->sub_documents = NULL;
	state->sub_state = NULL;

	state->current_cell = NULL;

	state->extending_start = FALSE;
	state->selection_start = NULL;
	state->selection_start_pos = 0;
	state->selection_end = NULL;
	state->selection_end_pos = 0;
	state->selection_new = NULL;
	state->selection_new_pos = 0;

#ifdef EDITOR
	state->edit_force_offset = FALSE;
	state->edit_current_element = 0;
	state->edit_current_offset = 0;
	state->edit_relayout_display_blocked = FALSE;
#endif
#ifdef MOCHA
	state->in_relayout = FALSE;
#endif
	return(state);
}


static void
lo_set_background_image(MWContext *context, lo_DocState *state, char *pUrl)
{
	PA_Block buff;
	PA_Tag tag;

	tag.type = P_BODY;
	tag.is_end = FALSE;
	tag.newline_count = 0;
	buff = PA_ALLOC(XP_STRLEN(pUrl) +
		XP_STRLEN(" background=\"\">") + 1);
	tag.data = NULL;
	tag.data_len = 0;
	tag.true_len = 0;
	if (buff != NULL)
	{
		char *str;

		PA_LOCK(str, char *, buff);
		XP_SPRINTF(str, " background=\"%s\">", pUrl);
		tag.data_len = XP_STRLEN(str);
		PA_UNLOCK(buff);
		tag.data = buff;
	}
	tag.lo_data = NULL;
	tag.next = NULL;

	lo_BodyBackground(context, state, &tag, TRUE);
}


static void
lo_load_user_backdrop(MWContext *context, lo_DocState *state)
{
	lo_set_background_image( context, state, Master_backdrop_url );
}

void LO_SetBackgroundImage( MWContext *context, char *pUrl )
{
	lo_TopState* top_state;
	lo_DocState *state;

	/*
	 * Get the unique document ID, and retreive this
	 * documents layout state.
	 */
	top_state = lo_FetchTopState(XP_DOCID(context));
	if ((top_state == NULL)||(top_state->doc_state == NULL))
	{
		return;
	}
	state = top_state->doc_state;

	/* 
	 * clear existing image.
	*/
	if( top_state->backdrop_image ){
		lo_FreeElement( context, top_state->backdrop_image, TRUE );
		top_state->backdrop_image = NULL;
		top_state->have_a_bg_url = FALSE;
	}
	top_state->doc_specified_bg = FALSE;

	if( pUrl )
	{
#ifdef EDITOR
		FE_ClearBackgroundImage( context );
		/* clear out the editor current element fields so that
		 * lo_NewElement doesn't try to access a bad element.
		 */
		state->edit_current_element = NULL;
		state->edit_current_offset = 0;
#endif
		lo_set_background_image( context, state, pUrl );
	}
	else
	{
#ifdef EDITOR
		/* LTNOTE: this should be implemented by the front ends
		 * and shouldn't be in an ifdef EDITOR 
		 */
		FE_ClearBackgroundImage( context );
#endif
	}
}


/*************************************
 * Function: lo_FlushLineList
 *
 * Description: This function adds a linefeed element to the end
 *	of the current line, and now that the line is done, it asks
 *	the front end to display the line.  This function is only
 * 	called by a hard linebreak.
 *
 * Params: Window context, document state, and flag for whether this is
 *	a breaking linefeed of not.  A breaking linefeed is one inserted
 *	just to break a text flow to the current window width.
 *
 * Returns: Nothing
 *************************************/
void
lo_FlushLineList(MWContext *context, lo_DocState *state, Bool breaking)
{
	LO_LinefeedStruct *linefeed;
	LO_Element **line_array;
#ifdef XP_WIN16
	int32 a_size;
	int32 a_indx;
	int32 a_line;
	XP_Block *larray_array;
#endif /* XP_WIN16 */


	if (state->top_state->nothing_displayed != FALSE)
	{
		/*
		 * If we are displaying elements we are
		 * no longer in the HEAD section of the HTML
		 * we are in the BODY section.
		 */
		state->top_state->in_head = FALSE;
		state->top_state->in_body = TRUE;

		if ((state->top_state->doc_specified_bg == FALSE)&&
		    (Master_backdrop_url != NULL)&&
		    (UserOverride == FALSE))
		{
			lo_load_user_backdrop(context, state);
		}
		state->top_state->nothing_displayed = FALSE;
	}

	/*
	 * There is a break at the end of this line.
	 * this may change min_width.
	 */
	{
		int32 new_break_holder;
		int32 min_width;

		new_break_holder = state->x;
		min_width = new_break_holder - state->break_holder + 1;
		if (min_width > state->min_width)
		{
			state->min_width = min_width;
		}
		state->break_holder = new_break_holder;
	}

	/*
	 * If in a division centering or right aligning this line
	 */
	if ((state->align_stack != NULL)&&(state->delay_align == FALSE))
	{
		int32 push_right;
		LO_Element *tptr;

		if (state->align_stack->alignment == LO_ALIGN_CENTER)
		{
			push_right = (state->right_margin - state->x) / 2;
		}
		else if (state->align_stack->alignment == LO_ALIGN_RIGHT)
		{
			push_right = state->right_margin - state->x;
		}
		else
		{
			push_right = 0;
		}

		if ((push_right > 0)&&(state->line_list != NULL))
		{
			tptr = state->line_list;

			while (tptr != NULL)
			{
				tptr->lo_any.x += push_right;
				if (tptr->type == LO_CELL)
				{
					lo_ShiftCell((LO_CellStruct *)tptr,
						push_right, 0);
				}
				tptr = tptr->lo_any.next;
			}
			state->x += push_right;
		}
	}

	/*
	 * Tack a linefeed element on to the end of the line list.
	 * Take advantage of this necessary pass thorugh the line's
	 * elements to set the line_height field in all
	 * elements, and to look and see if we have reached a
	 * possibly display blocking element.
	 */
	linefeed = lo_NewLinefeed(state,context);
	if (linefeed != NULL)
	{
		LO_Element *tptr;

		if (breaking != FALSE)
		{
			linefeed->ele_attrmask |= LO_ELE_BREAKING;
		}

		if ((state->align_stack != NULL)&&
		    (state->delay_align != FALSE)&&
		    (state->align_stack->alignment != LO_ALIGN_LEFT))
		{
			if (state->align_stack->alignment == LO_ALIGN_CENTER)
			{
				linefeed->ele_attrmask |= LO_ELE_DELAY_CENTER;
			}
			else if (state->align_stack->alignment == LO_ALIGN_RIGHT)
			{
				linefeed->ele_attrmask |= LO_ELE_DELAY_RIGHT;
			}
		}

		tptr = state->line_list;

		if (tptr == NULL)
		{
			state->line_list = (LO_Element *)linefeed;
			linefeed->prev = NULL;
		}
		else
		{
			while (tptr->lo_any.next != NULL)
			{
				/*
				 * If the display is blocked for an element
				 * we havn't reached yet, check to see if
				 * it is in this line, and if so, save its
				 * y position.
				 */
				if ((state->display_blocked != FALSE)&&
#ifdef EDITOR
				    (!state->edit_relayout_display_blocked)&&
#endif
				    (state->is_a_subdoc == SUBDOC_NOT)&&
				    (state->display_blocking_element_y == 0)&&
				    (state->display_blocking_element_id != -1)&&
				    (tptr->lo_any.ele_id >=
					state->display_blocking_element_id))
				{
					state->display_blocking_element_y =
						state->y;
					/*
					 * Special case, if the blocking element
					 * is on the first line, no blocking
					 * at all needs to happen.
					 */
					if (state->y == state->win_top)
					{
						state->display_blocked = FALSE;
						FE_SetDocPosition(context,
						    FE_VIEW, 0, state->base_y);
						lo_RefreshDocumentArea(context,
							state, 0, 0,
							state->win_width,
							state->win_height);
					}
				}
				tptr->lo_any.line_height = state->line_height;
				tptr = tptr->lo_any.next;
			}
			/*
			 * Same code as in the while loop, performed on the
			 * last element in the line.
			 */
			if ((state->display_blocked != FALSE)&&
#ifdef EDITOR
				(!state->edit_relayout_display_blocked)&&
#endif
				(state->is_a_subdoc == SUBDOC_NOT)&&
				(state->display_blocking_element_y == 0)&&
				(state->display_blocking_element_id != -1)&&
				(tptr->lo_any.ele_id >=
				state->display_blocking_element_id))
			{
				state->display_blocking_element_y = state->y;
				/*
				 * Special case, if the blocking element
				 * is on the first line, no blocking
				 * at all needs to happen.
				 */
				if (state->y == state->win_top)
				{
					state->display_blocked = FALSE;
					FE_SetDocPosition(context,
						FE_VIEW, 0, state->base_y);
					lo_RefreshDocumentArea(context, state,
						0, 0, state->win_width,
						state->win_height);
				}
			}
			tptr->lo_any.line_height = state->line_height;
			tptr->lo_any.next = (LO_Element *)linefeed;
			linefeed->prev = tptr;
		}
		state->x += linefeed->width;
	}
	else
	{
		return;		/* ALEKS OUT OF MEMORY RETURN */
	}

	/*
	 * Nothing to flush
	 */
	if (state->line_list == NULL)
	{
		return;
	}

	/*
	 * If necessary, grow the line array to hold more lines.
	 */
#ifdef XP_WIN16
	a_size = SIZE_LIMIT / sizeof(LO_Element *);
	a_indx = (state->line_num - 1) / a_size;
	a_line = state->line_num - (a_indx * a_size);

	XP_LOCK_BLOCK(larray_array, XP_Block *, state->larray_array);
	state->line_array = larray_array[a_indx];

	if (a_line == a_size)
	{
		state->line_array = XP_ALLOC_BLOCK(LINE_INC *
					sizeof(LO_Element *));
		if (state->line_array == NULL)
		{
			XP_UNLOCK_BLOCK(state->larray_array);
			state->top_state->out_of_memory = TRUE;
			return;
		}
		XP_LOCK_BLOCK(line_array, LO_Element **, state->line_array);
		line_array[0] = NULL;
		XP_UNLOCK_BLOCK(state->line_array);
		state->line_array_size = LINE_INC;

		state->larray_array_size++;
		XP_UNLOCK_BLOCK(state->larray_array);
		state->larray_array = XP_REALLOC_BLOCK(
			state->larray_array, (state->larray_array_size
			* sizeof(XP_Block)));
		if (state->larray_array == NULL)
		{
			state->top_state->out_of_memory = TRUE;
			return;
		}
		XP_LOCK_BLOCK(larray_array, XP_Block *, state->larray_array);
		larray_array[state->larray_array_size - 1] = state->line_array;
		state->line_array = larray_array[a_indx];
	}
	else if (a_line >= state->line_array_size)
	{
		state->line_array_size += LINE_INC;
		if (state->line_array_size > a_size)
		{
			state->line_array_size = (intn)a_size;
		}
		state->line_array = XP_REALLOC_BLOCK(state->line_array,
		    (state->line_array_size * sizeof(LO_Element *)));
		if (state->line_array == NULL)
		{
			XP_UNLOCK_BLOCK(state->larray_array);
			state->top_state->out_of_memory = TRUE;
			return;
		}
		larray_array[a_indx] = state->line_array;
	}

	/*
	 * Place this line of elements into the line array.
	 */
	XP_LOCK_BLOCK(line_array, LO_Element **, state->line_array);
	line_array[a_line - 1] = state->line_list;
	XP_UNLOCK_BLOCK(state->line_array);

	XP_UNLOCK_BLOCK(state->larray_array);
#else
	if (state->line_num > state->line_array_size)
	{
		int32 line_inc;

		if (state->line_array_size > (LINE_INC * 10))
		{
			line_inc = state->line_array_size / 10;
		}
		else
		{
			line_inc = LINE_INC;
		}

		state->line_array = XP_REALLOC_BLOCK(state->line_array,
			((state->line_array_size + line_inc) *
			sizeof(LO_Element *)));
		if (state->line_array == NULL)
		{
			state->top_state->out_of_memory = TRUE;
			return;
		}
		state->line_array_size += line_inc;
	}

	/*
	 * Place this line of elements into the line array.
	 */
	XP_LOCK_BLOCK(line_array, LO_Element **, state->line_array);
	line_array[state->line_num - 1] = state->line_list;
	XP_UNLOCK_BLOCK(state->line_array);
#endif /* XP_WIN16 */

	/*
	 * connect the complete doubly linked list between this line
	 * and the last one.
	 */
	if (state->end_last_line != NULL)
	{
		state->end_last_line->lo_any.next = state->line_list;
		state->line_list->lo_any.prev = state->end_last_line;
	}
	state->end_last_line = (LO_Element *)linefeed;

	state->line_list = NULL;
	state->line_num++;

	/*
	 * Have the front end display this line.
	 */
	lo_DisplayLine(context, state, (state->line_num - 2),
		0, 0, 0, 0, FALSE);

	/*
	 * We are starting a new line.  Clear any old break
	 * positions left over, clear the line_list, and increase
	 * the line number.
	 */
	state->old_break = NULL;
	state->old_break_pos = -1;
	state->old_break_width = 0;
	state->baseline = 0;
}


/*************************************
 * Function: lo_CleanTextWhitespace
 *
 * Description: Utility function to pass through a line an clean up
 * 	all its whitespace.   Compress multiple whitespace between
 *	element to a single space, and remove heading and
 *	trailing whitespace.
 *	Text is cleaned in place in the passed buffer.
 *
 * Params: Pointer to text to be cleand, and its length.
 *
 * Returns: Length of cleaned text.
 *************************************/
int32
lo_CleanTextWhitespace(char *text, int32 text_len)
{
	char *from_ptr;
	char *to_ptr;
	int32 len;
	int32 new_len;

	if (text == NULL)
	{
		return(0);
	}

	len = 0;
	new_len = 0;
	from_ptr = text;
	to_ptr = text;

	while (len < text_len)
	{
		/*
		 * Compress chunk of whitespace
		 */
		while ((len < text_len)&&(XP_IS_SPACE(*from_ptr)))
		{
			len++;
			from_ptr++;
		}
		if (len == text_len)
		{
			continue;
		}

		/*
		 * Skip past "good" text
		 */
		while ((len < text_len)&&(!XP_IS_SPACE(*from_ptr)))
		{
			*to_ptr++ = *from_ptr++;
			len++;
			new_len++;
		}

		/*
		 * Put in one space to represent the compressed spaces.
		 */
		if (len != text_len)
		{
			*to_ptr++ = ' ';
			new_len++;
		}
	}

	/*
	 * Remove the trailing space, and terminate string.
	 */
	if ((new_len > 0)&&(*(to_ptr - 1) == ' '))
	{
		to_ptr--;
		new_len--;
	}
	*to_ptr = '\0';

	return(new_len);
}


/*
 * Used to strip white space off of HREF parameter values to make
 * valid URLs out of them.  Remove whitespace from both ends, and
 * remove all non-space whitespace from the middle.
 */
int32
lo_StripTextWhitespace(char *text, int32 text_len)
{
	char *from_ptr;
	char *to_ptr;
	int32 len;
	int32 tail;

	if ((text == NULL)||(text_len < 1))
	{
		return(0);
	}

	len = 0;
	from_ptr = text;
	/*
	 * strip leading whitespace
	 */
	while ((len < text_len)&&(XP_IS_SPACE(*from_ptr)))
	{
		len++;
		from_ptr++;
	}

	if (len == text_len)
	{
		*text = '\0';
		return(0);
	}

	tail = 0;
	from_ptr = (char *)(text + text_len - 1);
	/*
	 * Remove any trailing space
	 */
	while (XP_IS_SPACE(*from_ptr))
	{
		from_ptr--;
		tail++;
	}

	/*
	 * terminate string
	 */
	from_ptr++;
	*from_ptr = '\0';

	/*
	 * remove all non-space whitespace from the middle of the string.
	 */
	from_ptr = (char *)(text + len);
	len = text_len - len - tail;
	to_ptr = text;
	while (*from_ptr != '\0')
	{
		if (XP_IS_SPACE(*from_ptr) && (*from_ptr != ' '))
		{
			from_ptr++;
			len--;
		}
		else
		{
			*to_ptr++ = *from_ptr++;
		}
	}
	*to_ptr = '\0';

	return(len);
}


LO_AnchorData *
lo_NewAnchor(lo_DocState *state, PA_Block href, PA_Block targ)
{
	LO_AnchorData *anchor_data;

	anchor_data = XP_NEW(LO_AnchorData);
	if (anchor_data)
	{
		anchor_data->anchor = href;
		anchor_data->target = targ;
		anchor_data->alt = NULL;
#ifdef MOCHA
		anchor_data->mocha_object = NULL;
#endif
	}
	else
	{
		state->top_state->out_of_memory = TRUE;
	}
	return anchor_data;
}


void
lo_DestroyAnchor(LO_AnchorData *anchor_data)
{
	if (anchor_data->anchor != NULL)
	{
		PA_FREE(anchor_data->anchor);
	}
	if (anchor_data->target != NULL)
	{
		PA_FREE(anchor_data->target);
	}
	XP_DELETE(anchor_data);
}


/*
 * Add this url's block to the list
 * of all allocated urls so we can free
 * it later.
 */
void
lo_AddToUrlList(MWContext *context, lo_DocState *state,
	LO_AnchorData *url_buff)
{
	lo_TopState *top_state;
	int32 i;
	LO_AnchorData **anchor_array;
	intn a_url;
#ifdef XP_WIN16
	intn a_size;
	intn a_indx;
	XP_Block *ulist_array;
#endif /* XP_WIN16 */

	/*
	 * Error checking
	 */
	if ((url_buff == NULL)||(state == NULL))
	{
		return;
	}

	top_state = state->top_state;

	/*
	 * We may need to grow the url_list.
	 */
#ifdef XP_WIN16
	a_size = SIZE_LIMIT / sizeof(XP_Block *);
	a_indx = top_state->url_list_len / a_size;
	a_url = top_state->url_list_len - (a_indx * a_size);

	XP_LOCK_BLOCK(ulist_array, XP_Block *, top_state->ulist_array);


	if ((a_url == 0)&&(a_indx > 0))
	{
		top_state->url_list = XP_ALLOC_BLOCK(URL_LIST_INC *
					sizeof(LO_AnchorData *));
		if (top_state->url_list == NULL)
		{
			XP_UNLOCK_BLOCK(top_state->ulist_array);
			top_state->out_of_memory = TRUE;
			return;
		}
		top_state->url_list_size = URL_LIST_INC;
		XP_LOCK_BLOCK(anchor_array, LO_AnchorData **,
			top_state->url_list);
		for (i=0; i < URL_LIST_INC; i++)
		{
			anchor_array[i] = NULL;
		}
		XP_UNLOCK_BLOCK(top_state->url_list);

		top_state->ulist_array_size++;
		XP_UNLOCK_BLOCK(top_state->ulist_array);
		top_state->ulist_array = XP_REALLOC_BLOCK(
			top_state->ulist_array, (top_state->ulist_array_size
			* sizeof(XP_Block)));
		if (top_state->ulist_array == NULL)
		{
			top_state->out_of_memory = TRUE;
			return;
		}
		XP_LOCK_BLOCK(ulist_array, XP_Block *, top_state->ulist_array);
		ulist_array[top_state->ulist_array_size - 1] = top_state->url_list;
		top_state->url_list = ulist_array[a_indx];
	}
	else if (a_url >= top_state->url_list_size)
	{
		int32 url_list_inc;

		if ((top_state->url_list_size / 10) > URL_LIST_INC)
		{
			url_list_inc = top_state->url_list_size / 10;
		}
		else
		{
			url_list_inc = URL_LIST_INC;
		}
		top_state->url_list_size += (intn)url_list_inc;
		if (top_state->url_list_size > a_size)
		{
			top_state->url_list_size = a_size;
		}

		top_state->url_list = ulist_array[a_indx];
		top_state->url_list = XP_REALLOC_BLOCK(top_state->url_list,
			(top_state->url_list_size * sizeof(LO_AnchorData *)));
		if (top_state->url_list == NULL)
		{
			XP_UNLOCK_BLOCK(top_state->ulist_array);
			top_state->out_of_memory = TRUE;
			return;
		}
		ulist_array[a_indx] = top_state->url_list;
		/*
		 * Clear the new entries
		 */
		XP_LOCK_BLOCK(anchor_array, LO_AnchorData **,
			top_state->url_list);
		for (i = top_state->url_list_len; i < top_state->url_list_size; i++)
		{
			anchor_array[i] = NULL;
		}
		XP_UNLOCK_BLOCK(top_state->url_list);
	}
	top_state->url_list = ulist_array[a_indx];
#else
	if (top_state->url_list_len ==
		top_state->url_list_size)
	{
		int32 url_list_inc;

		if ((top_state->url_list_size / 10) > URL_LIST_INC)
		{
			url_list_inc = top_state->url_list_size / 10;
		}
		else
		{
			url_list_inc = URL_LIST_INC;
		}
		top_state->url_list_size += url_list_inc;
		top_state->url_list = XP_REALLOC_BLOCK(top_state->url_list,
					(top_state->url_list_size *
					sizeof(LO_AnchorData *)));
		if (top_state->url_list == NULL)
		{
			top_state->out_of_memory = TRUE;
			return;
		}

		/*
		 * Clear the new entries
		 */
		XP_LOCK_BLOCK(anchor_array, LO_AnchorData **, top_state->url_list);
		for (i = top_state->url_list_len; i < top_state->url_list_size; i++)
		{
			anchor_array[i] = NULL;
		}
		XP_UNLOCK_BLOCK(top_state->url_list);
	}
	a_url = top_state->url_list_len;
#endif /* XP_WIN16 */

	XP_LOCK_BLOCK(anchor_array, LO_AnchorData **, top_state->url_list);
	if (anchor_array[a_url] != NULL)
	{
		lo_DestroyAnchor(anchor_array[a_url]);
	}
	anchor_array[a_url] = url_buff;
	top_state->url_list_len++;
	XP_UNLOCK_BLOCK(top_state->url_list);
}


void
lo_ProcessBodyTag(MWContext *context, lo_DocState *state, PA_Tag *tag)
{
	if (tag->is_end == FALSE)
	{
		if ((state->top_state->doc_specified_bg == FALSE)&&
		    (state->end_last_line == NULL)&&(UserOverride == FALSE))
		{
			lo_BodyBackground(context, state, tag, FALSE);
		}
#ifdef MOCHA
		if( !EDT_IS_EDITOR( context ) )
		{
			lo_ProcessContextEventHandlers(context, state, tag);
		}
#endif
	}
}


/*************************************
 * Function: lo_BlockTag
 *
 * Description: This function blocks the layout of the tag, and
 *	saves them in the context.
 *
 * Params: Window context and document state, and the tag to be blocked.
 *
 * Returns: Nothing
 *************************************/
static void
lo_BlockTag(MWContext *context, lo_DocState *state, PA_Tag *tag)
{
	PA_Tag ***tags_end_ptr, **tags_end;	/* triple-indirect, wow! see below */
	int32 doc_id;
	lo_TopState *top_state;
	lo_DocState *new_state;

	/*
	 * All blocked tags should be at the top level of state
	 */
	doc_id = XP_DOCID(context);
	top_state = lo_FetchTopState(doc_id);
	if ((top_state == NULL)||(top_state->doc_state == NULL))
	{
		return;
	}
	state = top_state->doc_state;

	new_state = lo_CurrentSubState(state);
	switch (tag->type)
	{
		case P_IMAGE:
		case P_NEW_IMAGE:
			if (tag->is_end == FALSE)
			{
				lo_BlockedImageLayout(context, new_state, tag);
			}
			break;
#ifdef MOCHA
		case P_SCRIPT:
			lo_BlockScriptTag(context, new_state, tag);
			break;
#endif
	}

	/*
	 * Unify the basis and induction cases of singly-linked list insert
	 * using a double-indirect pointer.  Instead of
	 *
	 *	if (head == NULL)
	 *		head = tail = elem;
	 *	else
	 *		tail = tail->next = elem;
	 *
	 * where head = tail = NULL is the empty state and elem->next = NULL,
	 * use a pointer to the pointer to the last element:
	 *
	 *	*tail_ptr = elem;
	 *	tail_ptr = &elem->next;
	 *
	 * where head = NULL, tail_ptr = &head is the empty state.
	 *
	 * The triple-indirection below is to avoid further splitting cases
	 * based on whether we're inserting at the mocha_write_point or at the
	 * end of the tags list.  The 'if (top_state->mocha_write_level)' test
	 * is unavoidable, but once the tags_end_ptr variable is set to point
	 * at the right double-indirect "tail_ptr" (per above sketch), there
	 * is shared code.
	 */
#ifdef MOCHA
	if (top_state->mocha_write_level)
	{
		tags_end_ptr = &top_state->mocha_write_point;
		if (*tags_end_ptr == NULL)
		{
			*tags_end_ptr = top_state->tags_end;
		}
		if (top_state->tags_end == top_state->mocha_write_point)
		{
			*tags_end_ptr = &tag->next;
			tags_end_ptr = &top_state->tags_end;
		}
	}
	else
#endif
	{
		tags_end_ptr = &top_state->tags_end;
	}
	tags_end = *tags_end_ptr;
	tag->next = *tags_end;
	*tags_end = tag;
	*tags_end_ptr = &tag->next;
}


lo_DocState *
lo_TopSubState(lo_TopState *top_state)
{
	lo_DocState *new_state;

	if ((top_state == NULL)||(top_state->doc_state == NULL))
	{
		return(NULL);
	}

	new_state = top_state->doc_state;
	while (new_state->sub_state != NULL)
	{
		new_state = new_state->sub_state;
	}

	return(new_state);
}


lo_DocState *
lo_CurrentSubState(lo_DocState *state)
{
	lo_DocState *new_state;

	if (state == NULL)
	{
		return(NULL);
	}

	new_state = state;
	while (new_state->sub_state != NULL)
	{
		new_state = new_state->sub_state;
	}

	return(new_state);
}


/*************************************
 * Function: lo_FetchTopState
 *
 * Description: This function uses the document ID to locate the
 *	lo_TopState structure for this document.
 *	Eventually this state will be kept somewhere in the
 *	Window context.
 *
 * Params: A unique document ID as derived from the window context.
 *
 * Returns: A pointer to the lo_TopState structure for this document.
 *	    NULL on error.
 *************************************/
lo_TopState *
lo_FetchTopState(int32 doc_id)
{
	lo_StateList *sptr;
	lo_TopState *state;

	sptr = StateList;
	while (sptr != NULL)
	{
		if (sptr->doc_id == doc_id)
		{
			break;
		}
		sptr = sptr->next;
	}
	if (sptr == NULL)
	{
		state = NULL;
	}
	else
	{
		state = sptr->state;
	}

	return(state);
}


static void
lo_RemoveTopState(int32 doc_id)
{
	lo_StateList *state;
	lo_StateList *sptr;

	state = StateList;
	sptr = StateList;
	while (sptr != NULL)
	{
		if (sptr->doc_id == doc_id)
		{
			break;
		}
		state = sptr;
		sptr = sptr->next;
	}
	if (sptr != NULL)
	{
		if (sptr == StateList)
		{
			StateList = StateList->next;
		}
		else
		{
			state->next = sptr->next;
		}
		XP_DELETE(sptr);
	}
}


Bool
lo_StoreTopState(int32 doc_id, lo_TopState *new_state)
{
	lo_StateList *sptr;

	sptr = StateList;
	while (sptr != NULL)
	{
		if (sptr->doc_id == doc_id)
		{
			break;
		}
		sptr = sptr->next;
	}

	if (sptr == NULL)
	{
		sptr = XP_NEW(lo_StateList);
		if (sptr == NULL)
		{
			return FALSE;
		}
		sptr->doc_id = doc_id;
		sptr->next = StateList;
		StateList = sptr;
	}
	sptr->state = new_state;
	return TRUE;
}


void
lo_SaveSubdocTags(MWContext *context, lo_DocState *state, PA_Tag *tag)
{
	PA_Tag *tag_ptr;

	tag->next = NULL;

	if (state->subdoc_tags_end == NULL)
	{
		if (state->subdoc_tags == NULL)
		{
			state->subdoc_tags = tag;
			state->subdoc_tags_end = tag;
		}
		else
		{
			state->subdoc_tags->next = tag;
			state->subdoc_tags = tag;
			state->subdoc_tags_end = tag;
		}
	}
	else
	{
		tag_ptr = state->subdoc_tags_end;
		tag_ptr->next = tag;
		state->subdoc_tags_end = tag;
	}
}


void
lo_FlushBlockage(MWContext *context, lo_DocState *state,
				 lo_DocState *main_doc_state)
{
	PA_Tag *tag_ptr;
	int32 doc_id;
	lo_TopState *top_state;
	lo_DocState *orig_state;
	lo_DocState *up_state;

	/*
	 * All blocked tags should be at the top level of state
	 */
	doc_id = XP_DOCID(context);
	top_state = lo_FetchTopState(doc_id);
	if ((top_state == NULL)||(top_state->doc_state == NULL))
	{
		return;
	}
	orig_state = top_state->doc_state;

	tag_ptr = top_state->tags;

	up_state = NULL;
	state = orig_state;
	while (state->sub_state != NULL)
	{
		up_state = state;
		state = state->sub_state;
	}

	top_state->layout_blocking_element = NULL;

	while ((tag_ptr != NULL)&&(top_state->layout_blocking_element == NULL))
	{
		PA_Tag *tag;
		lo_DocState *new_state;
		lo_DocState *new_up_state;
		lo_DocState *tmp_state;
		Bool may_save;

		tag = tag_ptr;
		tag_ptr = tag_ptr->next;
		tag->next = NULL;

		/*
		 * Always keep top_state->tags sane
		 */
		top_state->tags = tag_ptr;

		if ((state->is_a_subdoc == SUBDOC_CELL)||
			(state->is_a_subdoc == SUBDOC_CAPTION))
		{
			may_save = TRUE;
		}
		else
		{
			may_save = FALSE;
		}

		if (state->top_state->out_of_memory == FALSE)
		{
			lo_LayoutTag(context, state, tag);
#ifdef MOCHA
			if (tag->type == P_SCRIPT)
			{
				lo_UnblockScriptTag(state);
			}
#endif
		}
		new_up_state = NULL;
		new_state = orig_state;
		while (new_state->sub_state != NULL)
		{
			new_up_state = new_state;
			new_state = new_state->sub_state;
		}
		tmp_state = new_state;
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
				(tmp_state != state))
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
			else if (tmp_state == state)
			{
				lo_SaveSubdocTags(context, state, tag);
			}
			/*
			 * Else that tag started a new, nested subdoc.
			 * Add the starting tag to the parent.
			 */
			else if (tmp_state == state->sub_state)
			{
				lo_SaveSubdocTags(context, state, tag);
				/*
				 * Since we have extended the parent chain,
				 * we need to reset the child to the new
				 * parent end-chain.
				 */
				if ((tmp_state->is_a_subdoc == SUBDOC_CELL)||
				    (tmp_state->is_a_subdoc == SUBDOC_CAPTION))
				{
					tmp_state->subdoc_tags =
						state->subdoc_tags_end;
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
		else
		{
			PA_FreeTag(tag);
		}
		tag_ptr = top_state->tags;
		state = new_state;
		up_state = new_up_state;
	}

	if (tag_ptr != NULL)
	{
		top_state->tags = tag_ptr;
	}
	else
	{
		top_state->tags = NULL;
		top_state->tags_end = &top_state->tags;
#ifdef MOCHA
		top_state->mocha_write_point = NULL;
#endif
	}

	if ((top_state->layout_blocking_element == NULL)&&
		(top_state->tags == NULL)&&
		(top_state->layout_status == PA_COMPLETE))
	{
		/*
		 * makes sure we are at the bottom
		 * of everything in the document.
		 */
		state = lo_CurrentSubState(main_doc_state);
		lo_CloseOutLayout(context, state);
		lo_FinishLayout(context, state, LM_LOAD);
	}
}


/*************************************
 * Function: lo_DisplayLine
 *
 * Description: This function displays a single line
 *	of the document, by having the front end individually
 *	display each element in it.
 *
 * Params: Window context, docuemnt state, and an index into
 *	the line array.
 *
 * Returns: Nothing.
 *************************************/
void
lo_DisplayLine(MWContext *context, lo_DocState *state, int32 line_num,
	int32 x, int32 y, uint32 w, uint32 h, Bool can_clip)
{
	LO_Element *tptr;
	LO_Element *end_ptr;
	LO_Element **line_array;

#ifdef LOCAL_DEBUG
XP_TRACE(("lo_DisplayLine(%d)\n", line_num));
#endif
	if (state == NULL)
	{
		return;
	}

	if (state->display_blocked != FALSE)
	{
		return;
	}

#ifdef XP_WIN16
	{
		intn a_size;
		intn a_indx;
		intn a_line;
		XP_Block *larray_array;

		a_size = SIZE_LIMIT / sizeof(LO_Element *);
		a_indx = (intn)(line_num / a_size);
		a_line = (intn)(line_num - (a_indx * a_size));

		XP_LOCK_BLOCK(larray_array, XP_Block *, state->larray_array);
		state->line_array = larray_array[a_indx];
		XP_LOCK_BLOCK(line_array, LO_Element **, state->line_array);
		tptr = line_array[a_line];

		if (line_num >= (state->line_num - 2))
		{
			end_ptr = NULL;
		}
		else
		{
			if ((a_line + 1) == a_size)
			{
				XP_UNLOCK_BLOCK(state->line_array);
				state->line_array = larray_array[a_indx + 1];
				XP_LOCK_BLOCK(line_array, LO_Element **,
					state->line_array);
				end_ptr = line_array[0];
			}
			else
			{
				end_ptr = line_array[a_line + 1];
			}
		}
		XP_UNLOCK_BLOCK(state->line_array);
		XP_UNLOCK_BLOCK(state->larray_array);
	}
#else
	XP_LOCK_BLOCK(line_array, LO_Element **, state->line_array);
	tptr = line_array[line_num];

	if (line_num >= (state->line_num - 2))
	{
		end_ptr = NULL;
	}
	else
	{
		end_ptr = line_array[line_num + 1];
	}
	XP_UNLOCK_BLOCK(state->line_array);
#endif /* XP_WIN16 */

#ifdef LOCAL_DEBUG
	if (tptr == NULL)
	{
		XP_TRACE(("	line %d is NULL!\n", line_num));
	}
#endif
	while (tptr != NULL && tptr != end_ptr)
	{
		tptr->lo_any.x += state->base_x;
		tptr->lo_any.y += state->base_y;

		switch (tptr->type)
		{
			case LO_TEXT:
				if (tptr->lo_text.text != NULL)
				{
					lo_DisplayText(context, FE_VIEW,
						(LO_TextStruct *)tptr, FALSE);
				}
#ifdef LOCAL_DEBUG
else
{
XP_TRACE(("	tptr->text is NULL!\n"));
}
#endif
				break;
			case LO_LINEFEED:
				lo_DisplayLineFeed(context, FE_VIEW,
					(LO_LinefeedStruct *)tptr, FALSE);
				break;
			case LO_HRULE:
				lo_DisplayHR(context, FE_VIEW,
					(LO_HorizRuleStruct *)tptr);
				break;
			case LO_FORM_ELE:
				lo_DisplayFormElement(context, FE_VIEW,
					(LO_FormElementStruct *)tptr);
				break;
			case LO_BULLET:
				lo_DisplayBullet(context, FE_VIEW,
					(LO_BulletStruct *)tptr);
				break;
			case LO_IMAGE:
				if (can_clip != FALSE)
				{
					lo_ClipImage(context, FE_VIEW,
						(LO_ImageStruct *)tptr,
						(x + state->base_x),
						(y + state->base_y), w, h);
				}
				else
				{
					lo_DisplayImage(context, FE_VIEW,
						(LO_ImageStruct *)tptr);
				}
				break;
			case LO_TABLE:
				lo_DisplayTable(context, FE_VIEW,
					(LO_TableStruct *)tptr);
				break;
			case LO_EMBED:
				lo_DisplayEmbed(context, FE_VIEW,
					(LO_EmbedStruct *)tptr);
				break;
			case LO_JAVA:
				lo_DisplayJavaApp(context, FE_VIEW,
					(LO_JavaAppStruct *)tptr);
				break;
			case LO_EDGE:
				lo_DisplayEdge(context, FE_VIEW,
					(LO_EdgeStruct *)tptr);
				break;
			case LO_CELL:
				lo_DisplayCell(context, FE_VIEW,
					(LO_CellStruct *)tptr);
				lo_DisplayCellContents(context, FE_VIEW,
					(LO_CellStruct *)tptr,
					state->base_x, state->base_y);
				break;
			case LO_SUBDOC:
			    {
				LO_SubDocStruct *subdoc;
				int32 new_x, new_y;
				uint32 new_width, new_height;
				lo_DocState *sub_state;

				subdoc = (LO_SubDocStruct *)tptr;
				sub_state = (lo_DocState *)subdoc->state;

				if (sub_state == NULL)
				{
					break;
				}

				lo_DisplaySubDoc(context, FE_VIEW,
					subdoc);

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
XP_TRACE(("lo_DisplayLine(%d) skip element type = %d\n", line_num, tptr->type));
				break;
		}

		tptr->lo_any.x -= state->base_x;
		tptr->lo_any.y -= state->base_y;

		tptr = tptr->lo_any.next;
	}
}


/*************************************
 * Function: lo_PointToLine
 *
 * Description: This function calculates the line which contains
 *	a point. It's used for tracking mouse motion and button presses.
 *
 * Params: Window context, x, y position of point.
 *
 * Returns: The line that contains the point.
 *	Returns -1 on error.
 *************************************/
int32
lo_PointToLine (MWContext *context, lo_DocState *state, int32 x, int32 y)
{
	int32 line, start, middle, end;
	LO_Element **line_array;
#ifdef XP_WIN16
	int32 line_add;
#endif /* XP_WIN16 */

	line = -1;

	/*
	 * No document state yet.
	 */
	if (state == NULL)
	{
		return(line);
	}

	/*
	 * No laid out lines yet.
	 */
	if (state->line_num == 1)
	{
		return(line);
	}

#ifdef XP_WIN16
	{
		intn a_size;
		intn a_indx;
		intn a_line;
		XP_Block *larray_array;

		a_size = SIZE_LIMIT / sizeof(LO_Element *);
		a_indx = (intn)((state->line_num - 2) / a_size);
		a_line = (intn)((state->line_num - 2) - (a_indx * a_size));

		XP_LOCK_BLOCK(larray_array, XP_Block *, state->larray_array);
		while (a_indx > 0)
		{
			XP_LOCK_BLOCK(line_array, LO_Element **,
					larray_array[a_indx]);
			if (y < line_array[0]->lo_any.y)
			{
				XP_UNLOCK_BLOCK(larray_array[a_indx]);
				a_indx--;
			}
			else
			{
				XP_UNLOCK_BLOCK(larray_array[a_indx]);
				break;
			}
		}
		state->line_array = larray_array[a_indx];
		XP_UNLOCK_BLOCK(state->larray_array);

		/*
		 * We are about to do a binary search through the line
		 * array, lock it down.
		 */
		XP_LOCK_BLOCK(line_array, LO_Element **, state->line_array);
		line_add = a_indx * a_size;
		start = 0;
		end = ((a_indx + 1) * a_size) - 1;
		if (end > (state->line_num - 2))
		{
			end = state->line_num - 2;
		}
		end = end - line_add;
	}
#else
	/*
	 * We are about to do a binary search through the line
	 * array, lock it down.
	 */
	XP_LOCK_BLOCK(line_array, LO_Element **, state->line_array);
	start = 0;
	end = (intn) state->line_num - 2;
#endif /* XP_WIN16 */

	/*
	 * Special case if area is off the top.
	 */
	if (y <= 0)
	{
		line = 0;
	}
	/*
	 * Else another special case if area is off the bottom.
	 */
	else if (y >= line_array[end]->lo_any.y)
	{
		line = end;
	}
	/*
	 * Else a binary search through the document
	 * line list to find the line that contains the point.
	 */
	else
	{
		/*
		 * find the containing line.
		 */
		while ((end - start) > 0)
		{
			middle = (start + end + 1) / 2;
			if (y < line_array[middle]->lo_any.y)
			{
				end = middle - 1;
			}
			else
			{
				start = middle;
			}
		}
		line = start;
	}
	XP_UNLOCK_BLOCK(state->line_array);
#ifdef XP_WIN16
	line = line + line_add;
#endif /* XP_WIN16 */
	return(line);
}



/*************************************
 * Function: lo_RegionToLines
 *
 * Description: This function calculates the lines which intersect
 * a front end drawable region. It's used for drawing and in
 * calculating page breaks for printing.
 *
 * Params: Window context, x, y position of upper left corner,
 *	and width X height of the rectangle. Also pointers to
 * return values. dontCrop is true if we do not want to draw lines
 * which would be cropped (fall on the top/bottom region boundary).
 * This is for printing.
 *
 * Returns: Sets top and bottom to the lines numbers
 * to display. Sets them to -1 on error.
 *************************************/
void
lo_RegionToLines (MWContext *context, lo_DocState *state, int32 x, int32 y,
	uint32 width, uint32 height, Bool dontCrop, int32 * topLine, int32 * bottomLine)
{
	LO_Element **line_array;
	int32 top, bottom;
	int32 y2;
	int32	topCrop;
	int32	bottomCrop;
	
	/*
		Set drawable lines to -1 (assuming error by default)
	*/
	*topLine = -1;
	*bottomLine = -1;
	
	/*
	 * No document state yet.
	 */
	if (state == NULL)
	{
		return;
	}

	/*
	 * No laid out lines yet.
	 */
	if (state->line_num == 1)
	{
		return;
	}

	y2 = y + height;

	top = lo_PointToLine(context, state, x, y);
	bottom = lo_PointToLine(context, state, x, y2);

	if ( dontCrop )
	{
		XP_LOCK_BLOCK(line_array, LO_Element**, state->line_array );
		
		topCrop = line_array[ top ]->lo_any.y;
		bottomCrop = line_array[ bottom ]->lo_any.y + line_array[ bottom ]->lo_any.line_height;
		XP_UNLOCK_BLOCK( state->line_array );
		
		if ( y > topCrop )
		{
			top++;
			if ( top > ( state->line_num - 2 ) )
				top = -1;
		} 
		if ( y2 < bottomCrop )
			bottom--;
	}
	
	/*
	 * Sanity check
	 */
	if (bottom < top)
	{
		bottom = top;
	}

	*topLine = top;
	*bottomLine = bottom;
}

/*
	LO_CalcPrintArea
    
    Takes a region and adjusts it inward so that the top and bottom lines
    are not cropped. Results can be passed to LO_RefreshArea to print
    one page. To calculate the next page, use y+height as the next y.
*/
intn
LO_CalcPrintArea (MWContext *context, int32 x, int32 y,
				  uint32 width, uint32 height,
				  int32 *top, int32 *bottom)
{
	LO_Element **line_array;
	int32 doc_id;
	lo_TopState *top_state;
	lo_DocState *state;
	int32		lineTop;
	int32		lineBottom;
	
	*top = -1;
	*bottom = -1;
	/*
	 * Get the unique document ID, and retreive this
	 * documents layout state.
	 */
	doc_id = XP_DOCID(context);
	top_state = lo_FetchTopState(doc_id);
	if ((top_state == NULL)||(top_state->doc_state == NULL))
	{
		return(0);
	}
	state = top_state->doc_state;
	
	/*	
		Find which lines intersect the drawable region.
	*/
	lo_RegionToLines( context, state, x, y, width, height, TRUE, &lineTop, &lineBottom );
	
	if ( lineTop != -1 && lineBottom != -1 )
	{
		XP_LOCK_BLOCK(line_array, LO_Element **, state->line_array );
	
		*top = line_array[ lineTop ]->lo_any.y;
		*bottom = line_array[ lineBottom ]->lo_any.y + line_array[ lineBottom ]->lo_any.line_height - 1;
		
		XP_UNLOCK_BLOCK( state->line_array );
		return (1);
	}
	return (0);
}

void
lo_CloseOutLayout(MWContext *context, lo_DocState *state)
{
	if ((state->cur_ele_type == LO_TEXT)&&
		(state->line_buf_len != 0))
	{
		lo_FlushLineBuffer(context, state);
	}

	/*
	 * makes sure we are at the bottom
	 * of everything in the document.
	 *
	 * Only close forms if we aren't in a subdoc because
	 * forms can span subdocs.
	 */
	if ((state->is_a_subdoc == SUBDOC_NOT)&&
		(state->top_state->in_form != FALSE))
	{
		lo_EndForm(context, state);
	}
	lo_SoftLineBreak(context, state, FALSE, 1);
	lo_ClearToBothMargins(context, state);
}


void
LO_CloseAllTags(MWContext *context)
{
	int32 doc_id;
	lo_TopState *top_state;
	lo_DocState *state;
	lo_DocState *sub_state;

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
	state = top_state->doc_state;

	/*
	 * If we are in a table, get us out.
	 */
	sub_state = lo_CurrentSubState(state);
	while (sub_state != state)
	{
		lo_DocState *old_state;

		lo_CloseOutTable(context, sub_state);
		old_state = sub_state;
		sub_state = lo_CurrentSubState(state);
		/*
		 * If sub_state doesn't change we could be in a loop.
		 * break it here.
		 */
		if (sub_state == old_state)
		{
			break;
		}
	}
	state->sub_state = NULL;

	/*
	 * First do the normal closing.
	 */
	lo_CloseOutLayout(context, state);

	/*
	 * Reset a bunch of state variables.
	 */
	state->text_divert = P_UNKNOWN;
	state->delay_align = FALSE;
	state->breakable = TRUE;
	state->preformatted = PRE_TEXT_NO;
	state->in_paragraph = FALSE;
	/*
	 * clear the alignment stack.
	 */
	while (lo_PopAlignment(state) != NULL);
	/*
	 * clear the list stack.
	 */
	while (lo_PopList(state, NULL) != NULL);
	/*
	 * clear all open anchors.
	 */
	lo_PopAllAnchors(state);
	/*
	 * clear the font stack.
	 */
	while (state->font_stack->next != NULL)
	{
		(void)lo_PopFont(state, P_UNKNOWN);
	}
	state->base_font_size = DEFAULT_BASE_FONT_SIZE;
	state->font_stack->text_attr->size = state->base_font_size;
	state->current_anchor = NULL;
	state->current_ele = NULL;
	state->current_table = NULL;
	state->current_cell = NULL;
	state->current_java = NULL;
	state->current_multicol = NULL;
	state->must_relayout_subdoc = FALSE;
	state->allow_percent_width = TRUE;
	state->allow_percent_height = TRUE;
	state->is_a_subdoc = SUBDOC_NOT;
	state->current_subdoc = 0;

	/*
	 * Reset a few top_state variables.
	 */
	top_state->current_map = NULL;
	top_state->in_nogrids = FALSE;
	top_state->in_noembed = FALSE;
	top_state->in_noscript = FALSE;
	top_state->in_applet = FALSE;
}


static void
lo_free_layout_state_data(MWContext *context, lo_DocState *state)
{
	/*
	 * Short circuit out if the state was freed out from
	 * under us.
	 */
	if (state == NULL)
	{
		return;
	}

#ifdef SCARY_LEAK_FIX
	if (state->current_table != NULL)
	{
		lo_FreePartialTable(context, state, state->current_table);
		state->current_table = NULL;
	}
#endif /* SCARY_LEAK_FIX */

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
		LO_Element *eptr;
		LO_Element *element;

		eptr = state->line_list;
		while (eptr != NULL)
		{
			element = eptr;
			eptr = eptr->lo_any.next;
			lo_FreeElement(context, element, TRUE);
		}
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
}


/* Clean up the layout data structures. This used to be inside lo_FinishLayout,
 * but we needed to call it from the editor, without all of the UI code that
 * lo_FinishLayout also calls.
 */

void
lo_FreeLayoutData(MWContext *context, lo_DocState *state)
{
	int32 cnt;

	/*
	 * Clean out the lo_DocState
	 */
	lo_free_layout_state_data(context, state);

	/*
	 * Short circuit out if the state was freed out from
	 * under us.
	 */
	if (state == NULL)
	{
		return;
	}

	/*
	 * These are tags that were blocked instead of laid out.
	 * If there was an image in here, we already started the
	 * layout of it, we better free it now.
	 */
	if (state->top_state->tags != NULL)
	{
		PA_Tag *tptr;
		PA_Tag *tag;

		tptr = state->top_state->tags;
		while (tptr != NULL)
		{
			tag = tptr;
			tptr = tptr->next;
			if (((tag->type == P_IMAGE)||
			     (tag->type == P_NEW_IMAGE))&&
			    (tag->lo_data != NULL))
			{
				LO_Element *element;

				element = (LO_Element *)tag->lo_data;
				lo_FreeElement(context, element, TRUE);
				tag->lo_data = NULL;
			}
			PA_FreeTag(tag);
		}
		state->top_state->tags = NULL;
		state->top_state->tags_end = &state->top_state->tags;
#ifdef MOCHA
		state->top_state->mocha_write_point = NULL;
#endif
	}

	/*
	 * If we were blocked on and image when we left this page, we need to
	 * free up that partially loaded image structure.
	 */
	if (state->top_state->layout_blocking_element != NULL)
	{
		LO_Element *element;

		element = state->top_state->layout_blocking_element;
		lo_FreeElement(context, element, TRUE);
		state->top_state->layout_blocking_element = NULL;
	}

	/*
	 * now we're done laying out the entire document. we had saved all
	 * the layout structures from the previous document so we're throwing
	 * away unused ones.
	 */
	cnt = lo_FreeRecycleList(context, state->top_state->recycle_list);
#ifdef MEMORY_ARENAS
	if (state->top_state->current_arena != NULL)
	{
		cnt = lo_FreeMemoryArena(state->top_state->current_arena->next);
		state->top_state->current_arena->next = NULL;
	}
#endif /* MEMORY_ARENAS */
	state->top_state->recycle_list = NULL;
}


#ifdef MOCHA_CACHES_WRITES
static void
lo_close_mocha_write_stream(lo_TopState *top_state, int mocha_event)
{
	NET_StreamClass *stream = top_state->mocha_write_stream;

	if (stream != NULL)
	{
		top_state->mocha_write_stream = NULL;
		if (mocha_event == LM_LOAD)
		{
			stream->complete(stream->data_object);
		}
		else
		{
			stream->abort(stream->data_object,
						  top_state->layout_status);
		}
		XP_DELETE(stream);
	}
}
#endif /* MOCHA_CACHES_WRITES */


#ifdef SCARY_LEAK_FIX
void
lo_FinishLayout_OutOfMemory(MWContext *context, lo_DocState *state)
{
	lo_TopState *top_state;
	lo_DocState *main_state;
	lo_DocState *sub_state;

	/*
	 * Short circuit out if the state was freed out from
	 * under us.
	 */
	if (state == NULL)
	{
		return;
	}

	/*
	 * Check for dangling sub-states, and clean them up if they exist.
	 */
	top_state = state->top_state;
	main_state = top_state->doc_state;
	sub_state = lo_TopSubState(top_state);
	while ((sub_state != NULL)&&(sub_state != main_state))
	{
		state = main_state;
		while ((state != NULL)&&(state->sub_state != sub_state))
		{
			state = state->sub_state;
		}
		if (state != NULL)
		{
			lo_free_layout_state_data(context, sub_state);
			XP_DELETE(sub_state);
			state->sub_state = NULL;
		}
		sub_state = lo_TopSubState(top_state);
	}
	state = lo_TopSubState(top_state);

#ifdef MOCHA_CACHES_WRITES
	lo_close_mocha_write_stream(top_state, LM_ABORT);
#endif /* MOCHA_CACHES_WRITES */

	/*
	 * Short circuit out if the state was freed out from
	 * under us.
	 */
	if (state == NULL)
	{
		return;
	}

	lo_free_layout_state_data(context, state);
}
#endif /* SCARY_LEAK_FIX */


void
lo_FinishLayout(MWContext *context, lo_DocState *state, int mocha_event)
{
#ifdef SCARY_LEAK_FIX
	lo_TopState *top_state;
	lo_DocState *main_state;
	lo_DocState *sub_state;
#endif /* SCARY_LEAK_FIX */

#ifdef MOCHA
#ifdef MOCHA_CACHES_WRITES
	if ((state != NULL) &&
		(state->top_state != NULL))
	{
		lo_close_mocha_write_stream(state->top_state, mocha_event);
	}
#endif /* MOCHA_CACHES_WRITES */

	if ((state == NULL) ||
		(state->top_state == NULL) ||
		(state->top_state->resize_reload == FALSE))
	{
		LM_SendLoadEvent(context, mocha_event);
	}
#endif

	/*
	 * Short circuit out if the state was freed out from
	 * under us.
	 */
	if (state == NULL)
	{
		FE_SetProgressBarPercent(context, 100);
#ifdef OLD_MSGS
		FE_Progress(context, "Layout Phase Complete");
#endif /* OLD_MSGS */
		FE_FinishedLayout(context);
		return;
	}

#ifdef SCARY_LEAK_FIX
	/*
	 * Check for dangling sub-states, and clean them up if they exist.
	 */
	top_state = state->top_state;
	main_state = top_state->doc_state;
	sub_state = lo_TopSubState(top_state);
	while ((sub_state != NULL)&&(sub_state != main_state))
	{
		state = main_state;
		while ((state != NULL)&&(state->sub_state != sub_state))
		{
			state = state->sub_state;
		}
		if (state != NULL)
		{
			lo_free_layout_state_data(context, sub_state);
			XP_DELETE(sub_state);
			state->sub_state = NULL;
		}
		sub_state = lo_TopSubState(top_state);
	}
	state = lo_TopSubState(top_state);

	/*
	 * Short circuit out if the state was freed out from
	 * under us.
	 */
	if (state == NULL)
	{
		FE_SetProgressBarPercent(context, 100);
#ifdef OLD_MSGS
		FE_Progress(context, "Layout Phase Complete");
#endif /* OLD_MSGS */
		FE_FinishedLayout(context);
		return;
	}
#endif /* SCARY_LEAK_FIX */

    lo_FreeLayoutData(context, state);

	if (state->display_blocked != FALSE)
	{
		int32 y;

		if (state->top_state->name_target != NULL)
		{
			XP_FREE(state->top_state->name_target);
			state->top_state->name_target = NULL;
		}
		state->display_blocking_element_id = 0;
		state->display_blocked = FALSE;
		y = state->display_blocking_element_y;
		state->display_blocking_element_y = 0;
		state->y += state->win_bottom;
		FE_SetDocDimension(context, FE_VIEW, state->max_width,
			state->y);
		if (y != state->win_top)
		{
			y = state->y - state->win_height;
			if (y < 0)
			{
				y = 0;
			}
		}
		FE_SetDocPosition(context, FE_VIEW, 0, y);
		lo_RefreshDocumentArea(context, state, 0, y,
			state->win_width, state->win_height);
	}
	else
	{
		state->y += state->win_bottom;
		FE_SetDocDimension(context, FE_VIEW, state->max_width,
			state->y);
	}

	if(!state->top_state->is_binary)
		FE_SetProgressBarPercent(context, 100);

#ifdef OLD_MSGS
	FE_Progress(context, "Layout Phase Complete");
#endif /* OLD_MSGS */

	FE_FinishedLayout(context);
#ifdef EDITOR
	if (EDT_IS_EDITOR (context))
		EDT_FinishedLayout(context);
#endif
}

/*******************************************************************************
 * LO_ProcessTag helper routines
 ******************************************************************************/

void
lo_GetRecycleList(MWContext* context, int32 doc_id, pa_DocData* doc_data, 
				  LO_Element* *recycle_list, lo_arena* *old_arena)
{
	lo_TopState* old_top_state;
	lo_DocState* old_state;

	/*
	 * Free up any old document displayed in this
	 * window.
	 */
	old_top_state = lo_FetchTopState(XP_DOCID(context));
	if (old_top_state == NULL)
	{
		old_state = NULL;
	}
	else
	{
		old_state = old_top_state->doc_state;
	}
	if (old_state != NULL)
	{
#ifdef MEMORY_ARENAS
		*old_arena = old_top_state->first_arena;
#endif /* MEMORY_ARENAS */
		lo_SaveFormElementState(context, old_state, TRUE);
		/*
		 * If this document has no session history
		 * there is no place to save this data.
		 * So just free it now.
		 */
		if (SHIST_GetCurrent(&context->hist) == NULL)
		{
			lo_TopState *top_state = old_state->top_state;
			LO_FreeDocumentFormListData(context, (void *)top_state->savedData.FormList);
			top_state->savedData.FormList = NULL;
			LO_FreeDocumentEmbedListData(context, (void *)top_state->savedData.EmbedList);
			top_state->savedData.EmbedList = NULL;
			LO_FreeDocumentGridData(context, (void *)top_state->savedData.Grid);
			top_state->savedData.Grid = NULL;
		}
		*recycle_list =
			lo_InternalDiscardDocument(context, old_state, doc_data, TRUE);
	}

	/* Now that we've discarded the old document, set the new doc_id. */
	XP_SET_DOCID(context, doc_id);
}

intn
lo_ProcessTag_OutOfMemory(MWContext* context, LO_Element* recycle_list, lo_TopState* top_state)
{
	int32 cnt;
	
#ifdef SCARY_LEAK_FIX
	if ((top_state != NULL)&&(top_state->doc_state != NULL))
	{
		lo_FinishLayout_OutOfMemory(context, top_state->doc_state);
	}
#endif /* SCARY_LEAK_FIX */

	cnt = lo_FreeRecycleList(context, recycle_list);
#ifdef MEMORY_ARENAS
	if ((top_state != NULL)&&(top_state->first_arena != NULL))
	{
		cnt = lo_FreeMemoryArena(top_state->first_arena->next);
		top_state->first_arena->next = NULL;
	}
#endif /* MEMORY_ARENAS */
	return MK_OUT_OF_MEMORY;
}

#define RESTORE_SAVED_DATA(Type, var, doc_data, recycle_list, top_state) \
{ \
	var = (lo_Saved##Type##Data *) \
		(doc_data->url_struct->savedData.Type); \
	if (var == NULL) \
	{ \
		/* \
		 * A document we have never visited, insert a pointer \
		 * to a form list into its history, if it has one. \
		 */ \
		var = lo_NewDocument##Type##Data(); \
		if (SHIST_GetCurrent(&context->hist) != NULL) \
		{ \
			SHIST_SetCurrentDoc##Type##Data(context, (void *)var); \
		} \
		/* \
		 * Stuff it in the url_struct as well for those damn \
		 * continuous loading documents. \
		 */ \
		doc_data->url_struct->savedData.Type = (void *)var; \
	} \
	else if (doc_data->from_net != FALSE) \
	{ \
		lo_FreeDocument##Type##Data(context, var); \
	} \
	 \
	/* \
	 * If we don't have a form list, something very bad \
	 * has happened.  Like we are out of memory. \
	 */ \
	if (var == NULL) \
		return lo_ProcessTag_OutOfMemory(context, recycle_list, top_state); \
	 \
	top_state->savedData.Type = var; \
}

/****************************************************************************
 ****************************************************************************
 **
 **  Beginning of Public Utility Functions
 **
 ****************************************************************************
 ****************************************************************************/

/*************************************
 * Function: LO_ProcessTag
 *
 * Description: This function is the main layout entry point.
 *	New tags or tag lists are sent here along with the
 *	status as to where they came from.
 *
 * Params: A blank data object from the parser, right now it
 *	is parse document data.  The tag or list of tags to process.
 * 	and a status field as to why it came.
 *
 * Returns: 1 - Done ok, continuing.
 *	    0 - Done ok, stopping.
 *	   -1 - not done, error.
 *************************************/
intn
LO_ProcessTag(void *data_object, PA_Tag *tag, intn status)
{
	pa_DocData *doc_data;
	int32 doc_id;
	MWContext *context;
	lo_TopState *top_state;
	lo_DocState *state;
	lo_DocState *orig_state;
	lo_DocState *up_state;

	doc_data = (pa_DocData *)data_object;
	doc_id = doc_data->doc_id;
	top_state = (lo_TopState *)(doc_data->layout_state);
	if (top_state == NULL)
	{
		state = NULL;
	}
	else
	{
		state = top_state->doc_state;
	}
	context = doc_data->window_id;
	
	/*
	 * if we get called with abort/complete then ignore oom condition
	 * and clean up
	 */
	if ((state != NULL)&&(state->top_state != NULL)&&
		(state->top_state->out_of_memory != FALSE)&&(tag != NULL))
	{
#ifdef SCARY_LEAK_FIX
		lo_FinishLayout_OutOfMemory(context, state);
#endif /* SCARY_LEAK_FIX */
		return(MK_OUT_OF_MEMORY);
	}
	
	/*	
		Test to see if this is a new stream.
		If it is, kill the old one (NET_InterruptStream(url)),
		free its data, and continue.
	*/
	if (!state) { /* this is a new document being laid out */
		/* is there already a state for this context? */
		lo_TopState *top_state;
		lo_DocState *old_state;

		top_state = lo_FetchTopState(doc_id);
		if (top_state == NULL)
		{
			old_state = NULL;
		}
		else
		{
			old_state = top_state->doc_state;
		}

		if (old_state) {
			if (top_state->nurl) { /* is it still running? */
				/* Kill it with extreme prejudice. Take no prisoners. No mercy. */
/* ALEKS				NET_InterruptStream (top_state->nurl); */
				/* We've done a complete "cancel" now. We shouldn't need to do any more
				work; we'll find this "old state" again and recycle it */
			}
		}
	}
		
	/*
	 * We be done.
	 */
	if (tag == NULL)
	{
		if (status == PA_ABORT)
		{
#ifdef OLD_MSGS
			FE_Progress (context, "Document Load Aborted");
#endif /* OLD_MSGS */
            lo_process_deferred_image_info(NULL);
			if (state != NULL)
			{
				state->top_state->layout_status = status;
			}
			lo_FinishLayout(context, state, LM_ABORT);
		}
		else if (status == PA_COMPLETE)
		{
			orig_state = state;
			state = lo_CurrentSubState(state);

			/*
			 * IF the document is complete, we need to flush
			 * out any remaining unfinished elements.
			 */
			if (state != NULL)
			{
				state->top_state->total_bytes = state->top_state->current_bytes;

				if ((state->top_state->layout_blocking_element != NULL)
#ifdef MOCHA
					|| (state->top_state->script_state != NULL)
#endif
					)
				{
#ifdef OLD_MSGS
				    FE_Progress(context, "Layout Phase Complete but Blocked");
#endif /* OLD_MSGS */
				}
				else
				{
				    /*
				     * makes sure we are at the bottom
				     * of everything in the document.
				     */
				    lo_CloseOutLayout(context, state);

#ifdef OLD_MSGS
				    FE_Progress(context, "Layout Phase Complete");
#endif /* OLD_MSGS */
				    lo_FinishLayout(context, state, LM_LOAD);
				}
				orig_state->top_state->layout_status = status;
			}
			else
			{
				LO_Element *recycle_list = NULL;
				int32 cnt;
				int32 width, height;
				int32 margin_width, margin_height;
				lo_arena *old_arena = NULL;

				lo_GetRecycleList(context, doc_id, doc_data,
						  &recycle_list, &old_arena);
				margin_width = 0;
				margin_height = 0;
				FE_LayoutNewDocument(context,
						     doc_data->url_struct,
						     &width, &height,
						     &margin_width,
						     &margin_height);

				/* see first lo_FreeRecycleList comment. safe */
				cnt = lo_FreeRecycleList(context, recycle_list);
#ifdef MEMORY_ARENAS
				cnt = lo_FreeMemoryArena(old_arena);
#endif /* MEMORY_ARENAS */

				FE_SetDocPosition(context, FE_VIEW, 0, 0);
				FE_SetDocDimension(context, FE_VIEW, 1, 1);

#ifdef OLD_MSGS
				FE_Progress(context, "Layout Phase Complete");
#endif /* OLD_MSGS */
				lo_FinishLayout(context, state, LM_LOAD);
			}
		}

#ifdef SCARY_LEAK_FIX
		/*
		 * Since lo_FinishLayout make have caused state to be
		 * freed if it was a dangling sub_state, we need to
		 * reinitialize state here so it is safe to use it.
		 */
		state = lo_TopSubState(top_state);
#endif /* SCARY_LEAK_FIX */

		if (state != NULL)
		{
			/* done with netlib stream */
			state->top_state->nurl = NULL;
		}
#ifdef MOCHA
		if ((state != NULL) &&
			(state->top_state->doc_data != NULL))
		{
			/* done with parser stream */
			lo_ClearInputStream(context, state->top_state);
		}
#endif

		/*
		 * Make sure we are done with all images now.
		 */
		lo_NoMoreImages(context);

		return(0);
	}

	/*
	 * Check if this is a new document being laid out
	 */
	if (top_state == NULL)
	{
		int32 width, height;
		int32 margin_width, margin_height;
		lo_SavedFormListData *form_list;
		lo_SavedEmbedListData *embed_list;
		lo_SavedGridData *grid_data;
		LO_Element *recycle_list = NULL;
		int32 start_element;
		lo_arena *old_arena = NULL;

#ifdef LOCAL_DEBUG
XP_TRACE(("Initializing new doc %d\n", doc_id));
#endif /* DEBUG */

		lo_GetRecycleList(context, doc_id, doc_data, &recycle_list, &old_arena);	/* whh */
		width = 1;
		height = 1;
		margin_width = 0;
		margin_height = 0;
		/*
		 * Grid cells can set their own margin dimensions.
		 */
		if (context->is_grid_cell != FALSE)
		{
			lo_GetGridCellMargins(context,
				&margin_width, &margin_height);
		}
		FE_LayoutNewDocument(context, doc_data->url_struct,
			&width, &height, &margin_width, &margin_height);
		
		top_state = lo_NewTopState(context, doc_data->url);
		if (top_state == NULL)
		{
#ifdef MEMORY_ARENAS
			if (old_arena != NULL)
			{
				int32 cnt;

				cnt = lo_FreeMemoryArena(old_arena);
			}
#endif /* MEMORY_ARENAS */
			return lo_ProcessTag_OutOfMemory(context, recycle_list, top_state);
		}
#ifdef MEMORY_ARENAS
        if ( ! EDT_IS_EDITOR(context) ) {
		    top_state->first_arena->next = old_arena;
        }
#endif /* MEMORY_ARENAS */

		/* take a special hacks from URL_Struct */
		top_state->auto_scroll =(intn)doc_data->url_struct->auto_scroll;
		top_state->is_binary = doc_data->url_struct->is_binary;
		/*
		 * Security dialogs only for FO_PRESENT docs.
		 */
		if (CLEAR_CACHE_BIT(doc_data->format_out) == FO_PRESENT)
		{
			top_state->security_level =
				doc_data->url_struct->security_on;
		}
		else
		{
			top_state->security_level = 0;
		}
		top_state->force_reload = doc_data->url_struct->force_reload;
		top_state->nurl = doc_data->url_struct;
		top_state->total_bytes = doc_data->url_struct->content_length;
#ifdef MOCHA
		top_state->doc_data = doc_data;
		top_state->resize_reload = doc_data->url_struct->resize_reload;
#endif

		if (top_state->auto_scroll != 0)
		{
			top_state->scrolling_doc = TRUE;
			if (top_state->auto_scroll < 1)
			{
				top_state->auto_scroll = 1;
			}
			if (top_state->auto_scroll > 1000)
			{
				top_state->auto_scroll = 1000;
			}
		}

		/*
		 * give the parser a pointer to the layout data so we can
		 * get to it next time
		 */
		doc_data->layout_state = top_state;

		/*
		 * Add this new top state to the global state list.
		 */
		if (lo_StoreTopState(doc_id, top_state) == FALSE)
		{
			return lo_ProcessTag_OutOfMemory(context, recycle_list,
							 top_state);
		}

#ifdef EDITOR
		/* LTNOTE: maybe this should be passed into New_Layout.
		 * For now, it is not.
		 */
		top_state->edit_buffer = doc_data->edit_buffer;
#endif
		state = lo_NewLayout(context, width, height,
			margin_width, margin_height, NULL);
		if (state == NULL)
			return lo_ProcessTag_OutOfMemory(context, recycle_list, top_state);
		top_state->doc_state = state;
		
		/*
		 * Take care of forms.
		 * If we have been here before, we can pull the old form
		 * data out of the URL_Struct.  It may be invalid if the
		 * form was interrupted.
		 */
		RESTORE_SAVED_DATA(FormList, form_list, doc_data, recycle_list, top_state);
		if (!form_list->valid)
			lo_FreeDocumentFormListData(context, form_list);
		form_list->data_index = 0;

		/*
		 * Take care of embeds.
		 * If we have been here before, we can pull the old embed
		 * data out of the URL_Struct.
		 */
		RESTORE_SAVED_DATA(EmbedList, embed_list, doc_data, recycle_list, top_state);

		/*
		 * Take care of saved grid data.
		 * If we have been here before, we can pull the old
		 * grid data out of the URL_Struct.
		 */
		RESTORE_SAVED_DATA(Grid, grid_data, doc_data, recycle_list, top_state);

#ifdef MOCHA
		/* XXX just in case a grid had onLoad or onUnload handlers */
		if (!doc_data->url_struct->resize_reload)
		{
			lo_RestoreContextEventHandlers(context, state, tag,
										   &doc_data->url_struct->savedData);
		}
#endif


		top_state->recycle_list = recycle_list;

		if (state->top_state->name_target != NULL)
		{
			state->display_blocking_element_id = -1;
		}

		start_element = doc_data->url_struct->position_tag;
		if (start_element == 1)
		{
			start_element = 0;
		}

		if (start_element > 0)
		{
			state->display_blocking_element_id = start_element;
		}

		if ((state->display_blocking_element_id > state->top_state->element_id)||
			(state->display_blocking_element_id  == -1))
		{
			state->display_blocked = TRUE;
		}
		else
		{
			FE_SetDocPosition(context, FE_VIEW, 0, state->base_y);
		}

		if ((Master_backdrop_url != NULL)&&(UserOverride != FALSE))
		{
			lo_load_user_backdrop(context, state);
		}
	}

	if (state == NULL)
	{
		return(-1);
	}

	if (tag->data != NULL)
	{
		state->top_state->current_bytes += tag->data_len;
	}

	if ((state->top_state->is_grid == FALSE)&&
	    (state->top_state->nothing_displayed != FALSE)&&
	    ((tag->type == P_IMAGE)||(tag->type == P_NEW_IMAGE)||
	     (tag->type == P_EMBED)))
	{
		/*
		 * If we are displaying elements we are
		 * no longer in the HEAD section of the HTML
		 */
		state->top_state->in_head = FALSE;
		state->top_state->in_body = TRUE;

		if ((state->top_state->doc_specified_bg == FALSE)&&
		    (Master_backdrop_url != NULL)&&
		    (UserOverride == FALSE))
		{
			lo_load_user_backdrop(context, state);
		}
		state->top_state->nothing_displayed = FALSE;
	}

	/*
	 * Divert all tags to the current sub-document if there is one.
	 */
	up_state = NULL;
	orig_state = state;
	while (state->sub_state != NULL)
	{
		lo_DocState *new_state;

		up_state = state;
		new_state = state->sub_state;
		state = new_state;
	}

	orig_state->top_state->layout_status = status;

	/*
	 * The filter checks for all the NOFRAMES, NOEMBED, NOSCRIPT, and
	 * the APPLET ignore setup.
	 */
	if (lo_FilterTag(context, state, tag) == FALSE)
	{
		PA_FreeTag(tag);
	}
	else if (top_state->layout_blocking_element != NULL)
	{
		lo_BlockTag(context, state, tag);
	}
	else
	{
		lo_DocState *tmp_state;
		Bool may_save;

		if ((state->is_a_subdoc == SUBDOC_CELL)||
			(state->is_a_subdoc == SUBDOC_CAPTION))
		{
			may_save = TRUE;
		}
		else
		{
			may_save = FALSE;
		}

		lo_LayoutTag(context, state, tag);
		tmp_state = lo_CurrentSubState(orig_state);

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
				(tmp_state != state))
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
			else if (tmp_state == state)
			{
				lo_SaveSubdocTags(context, state, tag);
			}
			/*
			 * Else that tag started a new, nested subdoc.
			 * Add the starting tag to the parent.
			 */
			else if (tmp_state == state->sub_state)
			{
				lo_SaveSubdocTags(context, state, tag);
				/*
				 * Since we have extended the parent chain,
				 * we need to reset the child to the new
				 * parent end-chain.
				 */
				if ((tmp_state->is_a_subdoc == SUBDOC_CELL)||
				    (tmp_state->is_a_subdoc == SUBDOC_CAPTION))
				{
					tmp_state->subdoc_tags =
						state->subdoc_tags_end;
				}
			}
			/*
			 * This can never happen.
			 */
			else
			{
				PA_FreeTag(tag);
			}

			state = tmp_state;
		}
		else
		{
			PA_FreeTag(tag);
		}
	}

	if (state->top_state->out_of_memory != FALSE)
	{
#ifdef SCARY_LEAK_FIX
		lo_FinishLayout_OutOfMemory(context, state);
#endif /* SCARY_LEAK_FIX */
		return(MK_OUT_OF_MEMORY);
	}

	/*
	 * The parser (libparse/pa_parse.c) only calls us with these status
	 * codes when tag is null, and we handled that case far above (in fact,
	 * we will crash less far above on a null tag pointer, so we can't get
	 * here when aborting or completing).
	 */
	XP_ASSERT(status != PA_ABORT && status != PA_COMPLETE);

	return(1);
}

void
lo_RefreshDocumentArea(MWContext *context, lo_DocState *state, int32 x, int32 y,
	uint32 width, uint32 height)
{
	LO_Element *eptr;
	int32 i;
	int32 top, bottom;

	if (state->display_blocked != FALSE)
	{
		return;
	}
	
	/*	
	 * Find which lines intersect the drawable region.
	 */
	lo_RegionToLines (context, state, x, y, width, height, FALSE,
		&top, &bottom);

	/*
	 * Display all these lines, if there are any.
	 */
	if (bottom >= 0)
	{
		for (i=top; i <= bottom; i++)
		{
			lo_DisplayLine(context, state, i,
				x, y, width, height, TRUE);
		}
	}

	/*
	 * Check the floating element list.
	 */
	eptr = state->float_list;
	while (eptr != NULL)
	{
		if (((int32)(eptr->lo_any.y + eptr->lo_any.y_offset +
			eptr->lo_any.height) >= (int32)y)&&
			((int32)(eptr->lo_any.y + eptr->lo_any.y_offset) <=
			(int32)(y + height)))
		{
			if (((int32)(eptr->lo_any.x + eptr->lo_any.x_offset +
			    eptr->lo_any.width) >= (int32)x)&&
			    ((int32)(eptr->lo_any.x + eptr->lo_any.x_offset) <=
			    (int32)(x + width)))
			{
			    eptr->lo_any.x += state->base_x;
			    eptr->lo_any.y += state->base_y;

			    switch (eptr->type)
			    {
				case LO_IMAGE:
				    lo_ClipImage(context, FE_VIEW,
					(LO_ImageStruct *)eptr,
					(x + state->base_x),
					(y + state->base_y), width, height);
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
						(LO_CellStruct *)eptr,
						state->base_x, state->base_y);
					break;
				case LO_SUBDOC:
				    {
					LO_SubDocStruct *subdoc;
					int32 new_x, new_y;
					uint32 new_width, new_height;
					lo_DocState *sub_state;

					subdoc = (LO_SubDocStruct *)eptr;
					sub_state = (lo_DocState *)
						subdoc->state;

					if (sub_state == NULL)
					{
						break;
					}

					if (sub_state->base_y < 0)
					{
						sub_state->base_y = subdoc->y +
							subdoc->y_offset +
							subdoc->border_width;
						sub_state->display_blocked =
							FALSE;
					}

					lo_DisplaySubDoc(context, FE_VIEW,
						subdoc);

					new_x = x;
					if (new_x < subdoc->x)
					{
						new_x = subdoc->x;
					}
					new_y = y;
					if (new_y < subdoc->y)
					{
						new_y = subdoc->y;
					}
					new_width = x + width - new_x;
					if (new_width > ((uint32) subdoc->x_offset +
						(uint32) subdoc->width))
					{
						new_width = (subdoc->x_offset +
							subdoc->width);
					}
					new_height = y + height - new_y;
					if (new_height > ((uint32) subdoc->y_offset +
						(uint32) subdoc->height))
					{
						new_height = (subdoc->y_offset +
							subdoc->height);
					}
					new_x = new_x - subdoc->x;
					new_y = new_y - subdoc->y;
					sub_state->base_x = subdoc->x +
						subdoc->x_offset +
						subdoc->border_width;
					sub_state->base_y = subdoc->y +
						subdoc->y_offset +
						subdoc->border_width;
					lo_RefreshDocumentArea(context,
					    sub_state,
					    new_x, new_y,
					    new_width, new_height);
				    }
				    break;
				default:
				    break;
			    }
			    eptr->lo_any.x -= state->base_x;
			    eptr->lo_any.y -= state->base_y;
			}
		}
		eptr = eptr->lo_any.next;
	}
}


/*************************************
 * Function: LO_RefreshArea
 *
 * Description: This is a public function that allows the front end
 *	to ask that a particulat portion of the document be refreshed.
 *	The layout then finds the relevant elements, and calls
 *	the appropriate front end functions to display those
 *	elements.
 *
 * Params: Window context, x, y position of upper left corner,
 *	and width X height of the rectangle.
 *
 * Returns: Nothing
 *************************************/
void
LO_RefreshArea(MWContext *context, int32 x, int32 y,
	uint32 width, uint32 height)
{
	int32 doc_id;
	lo_TopState *top_state;
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
	state = top_state->doc_state;

	lo_RefreshDocumentArea(context, state, x, y, width, height);
}


/*************************************
 * Function: lo_GetLineEnds
 *
 * Description: This is a private function that returns the line ends for
 * a particular line.
 *
 * Params: Window context, line number.
 *
 * Returns: A pointer to the first and last elements in the line at that location,
 *	or NULLs if there is no matching line.
 *************************************/
void
lo_GetLineEnds(MWContext *context, lo_DocState *state,
	int32 line, LO_Element** retBegin, LO_Element** retEnd)
{
	LO_Element **line_array;

	*retBegin = NULL;
	*retEnd = NULL;

	/*
	 * No document state yet.
	 */
	if (state == NULL)
	{
		return;
	}

	if (line >= 0)
#ifdef XP_WIN16
	{
		intn a_size;
		intn a_indx;
		intn a_line;
		XP_Block *larray_array;

		a_size = SIZE_LIMIT / sizeof(LO_Element *);
		a_indx = (intn)(line / a_size);
		a_line = (intn)(line - (a_indx * a_size));

		XP_LOCK_BLOCK(larray_array, XP_Block *, state->larray_array);
		state->line_array = larray_array[a_indx];
		XP_LOCK_BLOCK(line_array, LO_Element **, state->line_array);
		*retBegin = line_array[a_line];

		if (line >= (state->line_num - 2))
		{
			*retEnd = NULL;
		}
		else
		{
			if ((a_line + 1) == a_size)
			{
				XP_UNLOCK_BLOCK(state->line_array);
				state->line_array = larray_array[a_indx + 1];
				XP_LOCK_BLOCK(line_array, LO_Element **,
					state->line_array);
				*retEnd = line_array[0];
			}
			else
			{
				*retEnd = line_array[a_line + 1];
			}
		}
		XP_UNLOCK_BLOCK(state->line_array);
		XP_UNLOCK_BLOCK(state->larray_array);
	}
#else
	{
		XP_LOCK_BLOCK(line_array, LO_Element **, state->line_array);
		*retBegin = line_array[line];
		if (line == (state->line_num - 2))
		{
			*retEnd = NULL;
		}
		else
		{
			*retEnd = line_array[line + 1];
		}
		XP_UNLOCK_BLOCK(state->line_array);
	}
#endif /* XP_WIN16 */
}

/*************************************
 * Function: lo_XYToDocumentElement
 *
 * Description: This is a public function that allows the front end
 *	to ask for the particular element residing at the passed
 *	(x, y) position in the window.  This is very important
 *	for resolving button clicks on anchors.
 *
 * Params: Window context, x, y position of the window location.
 *
 * Returns: A pointer to the element record at that location,
 *	or NULL if there is no matching element.
 *************************************/
LO_Element *
lo_XYToDocumentElement2(MWContext *context, lo_DocState *state,
	int32 x, int32 y, Bool check_float, Bool into_cells, Bool editMode,
	int32 *ret_x, int32 *ret_y)
{
	Bool in_table;
	int32 line;
	LO_Element *tptr;
	LO_Element *end_ptr;

	/*
	 * No document state yet.
	 */
	if (state == NULL)
	{
		return(NULL);
	}

	line = lo_PointToLine(context, state, x, y);

#ifdef LOCAL_DEBUG
XP_TRACE(("lo_PointToLine says line %d\n", line));
#endif /* LOCAL_DEBUG */

    lo_GetLineEnds(context, state, line, &tptr, &end_ptr);

	in_table = FALSE;
	while (tptr != end_ptr)
	{
		int32 y2;
        int32 x1;
		int32 t_width, t_height;

        x1 = tptr->lo_any.x + tptr->lo_any.x_offset;
 
		t_width = tptr->lo_any.width;
		/*
		 * Images need to account for border width
		 */
		if (tptr->type == LO_IMAGE)
		{
			t_width = t_width + (2 * tptr->lo_image.border_width);
            /*
             * If we're editing, images also account for their border space
             */
            if ( editMode )
            {
                t_width += 2 * tptr->lo_image.border_horiz_space;
                x1 -= tptr->lo_image.border_horiz_space;
            }
		}
		if (t_width <= 0)
		{
			t_width = 1;
		}

		t_height = tptr->lo_any.height;
		/*
		 * Images need to account for border width
		 */
		if (tptr->type == LO_IMAGE)
		{
			t_height = t_height + (2 * tptr->lo_image.border_width);

            /*
             * If we're editing, images also account for their border space
             */
            if ( editMode )
            {
                t_height += 2 * tptr->lo_image.border_vert_space;
            }
		}

		if (in_table == FALSE)
		{
			y2 = tptr->lo_any.y + tptr->lo_any.line_height;
		}
		else
		{
			y2 = tptr->lo_any.y + tptr->lo_any.y_offset +
				t_height;
		}
		if ((y >= tptr->lo_any.y)&&(y < y2)&&
			(x >= tptr->lo_any.x)&&
			(x < (x1 + t_width)))
		{
			/*
			 * Tables are containers.  Don't stop on them,
			 * look inside them.
			 */
			if (tptr->type != LO_TABLE)
			{
				break;
			}
			else
			{
				in_table = TRUE;
			}
		}
		tptr = tptr->lo_any.next;
	}
	if (tptr == end_ptr)
	{
		tptr = NULL;
	}

	/*
	 * Check the floating element list.
	 */
	if ((tptr == NULL)&&(check_float != FALSE))
	{
		LO_Element *eptr;

		eptr = state->float_list;
		while (eptr != NULL)
		{
			int32 t_width, t_height;

			t_width = eptr->lo_any.width;
			/*
			 * Images need to account for border width
			 */
			if (eptr->type == LO_IMAGE)
			{
				t_width = t_width +
					(2 * eptr->lo_image.border_width);
			}
			if (t_width <= 0)
			{
				t_width = 1;
			}

			t_height = eptr->lo_any.height;
			/*
			 * Images need to account for border width
			 */
			if (eptr->type == LO_IMAGE)
			{
				t_height = t_height +
					(2 * eptr->lo_image.border_width);
			}

			if ((y >= eptr->lo_any.y)&&
				(y < (eptr->lo_any.y + eptr->lo_any.y_offset +
					t_height)))
			{
				if ((x >= eptr->lo_any.x)&&
					(x < (eptr->lo_any.x +
						eptr->lo_any.x_offset +
						t_width)))
				{
					/*
					 * Tables are containers.
					 * Don't stop on them,
					 * look inside them.
					 */
					if (eptr->type != LO_TABLE)
					{
						break;
					}
				}
			}
			eptr = eptr->lo_any.next;
		}
		tptr = eptr;
	}

	if ((tptr != NULL)&&(tptr->type == LO_SUBDOC))
	{
		LO_SubDocStruct *subdoc;
		int32 new_x, new_y;
		int32 ret_new_x, ret_new_y;

		subdoc = (LO_SubDocStruct *)tptr;

		new_x = x - (subdoc->x + subdoc->x_offset +
			subdoc->border_width);
		new_y = y - (subdoc->y + subdoc->y_offset +
			subdoc->border_width);
		tptr = lo_XYToDocumentElement(context,
			(lo_DocState *)subdoc->state, new_x, new_y, check_float,
			into_cells, &ret_new_x, &ret_new_y);
		x = ret_new_x;
		y = ret_new_y;
	}
	else if ((tptr != NULL)&&(tptr->type == LO_CELL)&&(into_cells != FALSE))
	{
		tptr = lo_XYToCellElement(context, state,
			(LO_CellStruct *)tptr, x, y, TRUE, into_cells);
	}

	*ret_x = x;
	*ret_y = y;
	return(tptr);
}

LO_Element *
lo_XYToDocumentElement(MWContext *context, lo_DocState *state,
	int32 x, int32 y, Bool check_float, Bool into_cells,
	int32 *ret_x, int32 *ret_y)
{
    return lo_XYToDocumentElement2(context, state, x, y, check_float, into_cells, FALSE, ret_x, ret_y);
}

/*************************************
 * Function: LO_XYToElement
 *
 * Description: This is a public function that allows the front end
 *	to ask for the particular element residing at the passed
 *	(x, y) position in the window.  This is very important
 *	for resolving button clicks on anchors.
 *
 * Params: Window context, x, y position of the window location.
 *
 * Returns: A pointer to the element record at that location,
 *	or NULL if there is no matching element.
 *************************************/
LO_Element *
LO_XYToElement(MWContext *context, int32 x, int32 y)
{
	int32 doc_id;
	lo_TopState *top_state;
	lo_DocState *state;
	LO_Element *tptr;
	int32 ret_x, ret_y;

	/*
	 * Get the unique document ID, and retreive this
	 * documents layout state.
	 */
	doc_id = XP_DOCID(context);
	top_state = lo_FetchTopState(doc_id);
	if ((top_state == NULL)||(top_state->doc_state == NULL))
	{
		return(NULL);
	}
	state = top_state->doc_state;

	tptr = lo_XYToDocumentElement(context, state, x, y, TRUE, TRUE, 
		&ret_x, &ret_y);

	if ((tptr == NULL)&&(top_state->is_grid != FALSE))
	{
		tptr = lo_XYToGridEdge(context, state, x, y);
	}

	/*
	 * Make sure we do not return an invisible edge.
	 */
	if ((tptr != NULL)&&(tptr->type == LO_EDGE)&&
		(tptr->lo_edge.visible == FALSE))
	{
		tptr = NULL;
	}
	return(tptr);
}


void
LO_ClearBackdropBlock(MWContext *context, LO_ImageStruct *image, Bool fg_ok)
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
	main_doc_state = top_state->doc_state;
	state = lo_CurrentSubState(main_doc_state);

	/*
	 * Set the backdrop into main state so we know we got it.
	 */
	top_state->backdrop_image = (LO_Element *)image;

	/*
	 * We have been blocked on this backdrop for a while.
	 * We need to flush the blockage.
	 */
	if (top_state->layout_blocking_element == (LO_Element *)image)
	{
		if ((fg_ok != FALSE)||(image->anchor_href != NULL))
		{
			lo_BodyForeground(context, state, image);
		}

		lo_FlushBlockage(context, state, main_doc_state);
	}
	/*
	 * Else we have never been blocked on this backdrop, and
	 * we have been given it as soon as we asked for it.
	 */
	else
	{
		if ((fg_ok != FALSE)||(image->anchor_href != NULL))
		{
			lo_BodyForeground(context, state, image);
		}
	}
}


static void
lo_set_image_info(MWContext *context, int32 ele_id, int32 width, int32 height)
{
	int32 doc_id;
	lo_TopState *top_state;
	lo_DocState *main_doc_state;
	lo_DocState *state;
	LO_ImageStruct *image;

#ifdef EDITOR
	if ( ele_id == ED_IMAGE_LOAD_HACK_ID ){
		EDT_SetImageInfo( context, ele_id, width, height );
		return;
	}
#endif


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
	main_doc_state = top_state->doc_state;
	state = lo_CurrentSubState(main_doc_state);

	/*
	 * If the font stack has already been freed, we are too far gone
	 * to deal with delayed images coming in.  Ignore them.
	 */
	if (state->font_stack == NULL)
	{
		top_state->layout_blocking_element = NULL;
		return;
	}

	if ((top_state->layout_blocking_element != NULL)&&
		(top_state->layout_blocking_element->lo_any.ele_id == ele_id))
	{
		image = (LO_ImageStruct *)top_state->layout_blocking_element;

		if (image->image_attr->attrmask & LO_ATTR_BACKDROP)
		{
			return;
		}

		image->width = width;
		image->height = height;
		lo_FinishImage(context, state, image);
		lo_FlushBlockage(context, state, main_doc_state);
	}
	else if (top_state->tags != NULL)
	{
		PA_Tag *tag_ptr;

		tag_ptr = top_state->tags;
		while (tag_ptr != NULL)
		{
			if (((tag_ptr->type == P_IMAGE)||
				(tag_ptr->type == P_NEW_IMAGE))&&
				(tag_ptr->lo_data != NULL)&&
				(tag_ptr->is_end == FALSE))
			{
				image = (LO_ImageStruct *)tag_ptr->lo_data;
				if (image->ele_id == ele_id)
				{
					image->width = width;
					image->height = height;
					break;
				}
			}
			tag_ptr = tag_ptr->next;
		}
	}
}

typedef struct setImageInfoClosure_struct
{
    MWContext      *context;
    int32           ele_id;
    int32           width;
    int32           height;
    struct setImageInfoClosure_struct
                   *next;
} setImageInfoClosure;

/* List of images for which image size info is available,
   but for which the layout process is deferred */
static setImageInfoClosure *image_info_deferred_list;
static void *deferred_image_info_timeout = NULL;

/* Handle image dimension information that has been reported to
   layout since the last time we were called. */
static void
lo_process_deferred_image_info(void *closure)
{
/* DEBUG_jwz -- does this help?  no. */
    setImageInfoClosure *c = 0, *next_c = 0;

    for (c = image_info_deferred_list; c; c = next_c) {
        next_c = c->next;
        lo_set_image_info(c->context, c->ele_id, c->width, c->height);
        XP_FREE(c);
    }
    image_info_deferred_list = NULL;
    deferred_image_info_timeout = NULL;
}

/* Tell layout the dimensions of an image.  Actually, to avoid subtle
   bugs involving mutual recursion between the imagelib and the
   netlib, we don't process this information immediately, but rather
   thread it on a list to be processed later. */
void
LO_SetImageInfo(MWContext *context, int32 ele_id, int32 width, int32 height)
{
    setImageInfoClosure *c = XP_NEW(setImageInfoClosure);

    if (! c)
        return;
    c->context  = context;
    c->ele_id   = ele_id;
    c->width    = width;
    c->height   = height;
    c->next     = image_info_deferred_list;
    image_info_deferred_list = c;

    if (! deferred_image_info_timeout) {
        deferred_image_info_timeout = 
            FE_SetTimeout(lo_process_deferred_image_info, NULL, 0);
    }
}

/* XXX need to pass resize_reload through FE_FreeGridWindow */
static pa_DocData *lo_internal_doc_data;

LO_Element *
lo_InternalDiscardDocument(MWContext *context, lo_DocState *state,
						   pa_DocData *doc_data, Bool bWholeDoc)
{
	int32 doc_id;
	LO_Element *recycle_list;
	LO_Element **line_array;
	LO_TextAttr **text_attr_hash;
#ifdef MOCHA
	/* Look in doc_data->url_struct for the *new* document's reload flag. */
	Bool resize_reload = 
		((doc_data != NULL) &&
		 (doc_data->url_struct->resize_reload == TRUE));

	/*
	 * Order is crucial here: run the onunload handlers in a frames tree
	 * from the top down, but destroy frames bottom-up.  Look for the
	 * LM_ReleaseDocument call below, almost at the end of this function.
	 * This way, onunload handlers get to use their kids' properties, and
	 * kids get to use their parent's properties and functions.
	 */
	if (bWholeDoc)
	{
		if (state->top_state->doc_data != NULL)
		{
			/*
			 * Prevent recursion through lo_ClearInputStream/
			 * LM_ClearContextStream/LM_ClearDecoderStream/.../
			 * PA_MDLComplete/LO_ProcessTag.  This can happen
			 * when a window or frame cell is destroyed while
			 * its JavaScript document is open.  The FE calls
			 * LO_DiscardDocument, which calls us, and we must
			 * complete the open stream to layout.  But that
			 * inevitably causes layout to get its recycle list
			 * (see LO_ProcessTag in this file), and reenter
			 * this function.
			 */
			doc_id = XP_DOCID(context);
			lo_RemoveTopState(doc_id);

			/* XXX close stream now before freeing, even if resize_reload */
			lo_ClearInputStream(context, state->top_state);

			/*
			 * Now put top_state back on the StateList just in
			 * case something below calls lo_FetchTopState on
			 * the current context doc-id!  Yeesh.
			 */
			lo_StoreTopState(doc_id, state->top_state);
		}
		if (!resize_reload)
		{
			/* Send unload event before destroying anything. */
			LM_SendLoadEvent(context, LM_UNLOAD);
		}
	}
#endif /* MOCHA */

	if ( bWholeDoc && state->top_state->backdrop_image != NULL)
	{
		lo_RecycleElements(context, state,
			state->top_state->backdrop_image);
		state->top_state->backdrop_image = NULL;
	}

	if ( state->top_state->trash != NULL)
	{
		lo_RecycleElements(context, state,
			state->top_state->trash);
		state->top_state->trash = NULL;
	}

	if (state->float_list != NULL)
	{
		lo_RecycleElements(context, state, state->float_list);
		state->float_list = NULL;
	}

	if (state->line_array != NULL)
	{
		LO_Element *eptr;

#ifdef XP_WIN16
		{
			XP_Block *larray_array;
			intn cnt;

			XP_LOCK_BLOCK(larray_array, XP_Block *,
					state->larray_array);
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
        /* If this is a relayout delete, the elements are already owned by the document. So
         * don't recycle them.
         */
		if ( bWholeDoc && eptr != NULL)
		{
			lo_RecycleElements(context, state, eptr);
		}
		XP_UNLOCK_BLOCK(state->line_array);
		XP_FREE_BLOCK(state->line_array);
		state->line_array = NULL;
	}

	if ( bWholeDoc && state->top_state->text_attr_hash != NULL)
	{
		LO_TextAttr *attr;
		LO_TextAttr *attr_ptr;
		int i;

		XP_LOCK_BLOCK(text_attr_hash, LO_TextAttr **,
			state->top_state->text_attr_hash);
		for (i=0; i<FONT_HASH_SIZE; i++)
		{
			attr_ptr = text_attr_hash[i];
			while (attr_ptr != NULL)
			{
				attr = attr_ptr;
				attr_ptr = attr_ptr->next;
				XP_DELETE(attr);
			}
			text_attr_hash[i] = NULL;
		}
		XP_UNLOCK_BLOCK(state->top_state->text_attr_hash);
		XP_FREE_BLOCK(state->top_state->text_attr_hash);
		state->top_state->text_attr_hash = NULL;
	}

	if ( bWholeDoc && state->top_state->font_face_array != NULL)
	{
		int i;
		char **face_list;

		PA_LOCK(face_list, char **, state->top_state->font_face_array);
		for (i=0; i < state->top_state->font_face_array_len; i++)
		{
			XP_FREE(face_list[i]);
		}
		PA_UNLOCK(state->top_state->font_face_array);
		PA_FREE(state->top_state->font_face_array);
		state->top_state->font_face_array = NULL;
		state->top_state->font_face_array_len = 0;
		state->top_state->font_face_array_size = 0;
	}

	if ( bWholeDoc && state->top_state->map_list != NULL)
	{
		lo_MapRec *map_list;

		map_list = state->top_state->map_list;
		while (map_list != NULL)
		{
			map_list = lo_FreeMap(map_list);
		}
		state->top_state->map_list = NULL;
	}

	if ( bWholeDoc && state->top_state->url_list != NULL)
	{
		lo_TopState *top_state;
		int i;
		LO_AnchorData **anchor_array;

		top_state = state->top_state;
#ifdef XP_WIN16
		{
		XP_Block *ulist_array;
		int32 j;
		intn a_size, a_indx, a_url;

		a_size = SIZE_LIMIT / sizeof(XP_Block *);
		a_indx = top_state->url_list_len / a_size;
		a_url = top_state->url_list_len - (a_indx * a_size);

		XP_LOCK_BLOCK(ulist_array, XP_Block *, top_state->ulist_array);
		for (j=0; j < (top_state->ulist_array_size - 1); j++)
		{
			top_state->url_list = ulist_array[j];
			XP_LOCK_BLOCK(anchor_array, LO_AnchorData **,
				top_state->url_list);
			for (i=0; i < a_size; i++)
			{
				if (anchor_array[i] != NULL)
				{
					lo_DestroyAnchor(anchor_array[i]);
				}
			}
			XP_UNLOCK_BLOCK(top_state->url_list);
			XP_FREE_BLOCK(top_state->url_list);
		}
		top_state->url_list = ulist_array[top_state->ulist_array_size - 1];
		XP_LOCK_BLOCK(anchor_array, LO_AnchorData **,
			top_state->url_list);
		for (i=0; i < a_url; i++)
		{
			if (anchor_array[i] != NULL)
			{
				lo_DestroyAnchor(anchor_array[i]);
			}
		}
		XP_UNLOCK_BLOCK(top_state->url_list);
		XP_FREE_BLOCK(top_state->url_list);

		XP_UNLOCK_BLOCK(top_state->ulist_array);
		XP_FREE_BLOCK(top_state->ulist_array);
		}
#else
		XP_LOCK_BLOCK(anchor_array, LO_AnchorData **,
			top_state->url_list);
		for (i=0; i < top_state->url_list_len; i++)
		{
			if (anchor_array[i] != NULL)
			{
				lo_DestroyAnchor(anchor_array[i]);
			}
		}
		XP_UNLOCK_BLOCK(top_state->url_list);
		XP_FREE_BLOCK(top_state->url_list);
#endif /* XP_WIN16 */
		top_state->url_list = NULL;
		top_state->url_list_len = 0;
	}

	if (state->name_list != NULL)
	{
		lo_NameList *nptr;

		nptr = state->name_list;
		while (nptr != NULL)
		{
			lo_NameList *name_rec;

			name_rec = nptr;
			nptr = nptr->next;
			if (name_rec->name != NULL)
			{
				PA_FREE(name_rec->name);
			}
			XP_DELETE(name_rec);
		}
		state->name_list = NULL;
	}

	if ( bWholeDoc && state->top_state->form_list != NULL)
	{
		lo_FormData *fptr;

		fptr = state->top_state->form_list;
		while (fptr != NULL)
		{
			lo_FormData *form;

			form = fptr;
			fptr = fptr->next;
#ifdef MOCHA /* added by jwz */
			if (form->name != NULL)
			{
				PA_FREE(form->name);
			}
#endif /* MOCHA -- added by jwz */
			if (form->action != NULL)
			{
				PA_FREE(form->action);
			}
			if (form->form_elements != NULL)
			{
				PA_FREE(form->form_elements);
			}
			XP_DELETE(form);
		}
		state->top_state->form_list = NULL;
	}

	if ( bWholeDoc && state->top_state->the_grid != NULL)
	{
		pa_DocData *save;
		lo_GridRec *grid;
		lo_GridCellRec *cell_ptr;
		Bool incomplete;

		save = lo_internal_doc_data;
		lo_internal_doc_data = doc_data;
		grid = state->top_state->the_grid;
		cell_ptr = grid->cell_list;
		/*
		 * If this grid document finished loading, save
		 * its grid data to be restored when we return through
		 * history.
		 * We still have to free the FE grid windows.
		 */
		incomplete = (state->top_state->layout_status != PA_COMPLETE);
		if (incomplete == FALSE)
		{
			while (cell_ptr != NULL)
			{
				intn cell_status;
				char *old_url;

				cell_status =
					lo_GetCurrentGridCellStatus(cell_ptr);

				/*
				 * Only save cell history for COMPLETE cells.
				 */
				if (cell_status == PA_COMPLETE)
				{
				    /*
				     * free the original url for this cell.
				     */
				    old_url = cell_ptr->url;
				    if (cell_ptr->url != NULL)
				    {
						XP_FREE(cell_ptr->url);
				    }

				    /*
				     * Fetch the url for the current contents
				     * of the cell, and save that.
				     */
				    cell_ptr->url =
					lo_GetCurrentGridCellUrl(cell_ptr);

				    cell_ptr->history = NULL;
#ifdef NEW_FRAME_HIST
				    cell_ptr->hist_list = NULL;
#endif /* NEW_FRAME_HIST */
				    if (cell_ptr->context != NULL)
				    {
#ifdef NEW_FRAME_HIST
					cell_ptr->history =
					    cell_ptr->hist_array[
						cell_ptr->hist_indx - 1].hist;
					cell_ptr->hist_list = FE_FreeGridWindow(
						cell_ptr->context, TRUE);
#else
					cell_ptr->history = FE_FreeGridWindow(
						cell_ptr->context, TRUE);
#endif /* NEW_FRAME_HIST */
					cell_ptr->context = 0;		/* jwz */
				    }
				}
				else
				{
				    incomplete = TRUE;
				    cell_ptr->history = NULL;
#ifdef NEW_FRAME_HIST
				    cell_ptr->hist_list = NULL;
#endif /* NEW_FRAME_HIST */
				    if (cell_ptr->context != NULL)
				    {
					(void)FE_FreeGridWindow(
						cell_ptr->context, FALSE);
					cell_ptr->context = 0;		/* jwz */
				    }
				}
				cell_ptr->context = NULL;

				cell_ptr = cell_ptr->next;
			}
		}
		/*
		 * Check again in case one or more frames were incomplete.
		 */
		if (incomplete == FALSE)
		{
		  if (state->top_state && state->top_state->savedData.Grid) /* jwz */
			{
			state->top_state->savedData.Grid->the_grid =
				state->top_state->the_grid;
			state->top_state->savedData.Grid->main_width =
				grid->main_width;
			state->top_state->savedData.Grid->main_height =
				grid->main_height;
			}
		}
		/*
		 * Else free up everything now.
		 */
		else
		{
			cell_ptr = grid->cell_list;
			while (cell_ptr != NULL)
			{
				lo_GridCellRec *tmp_cell;

				cell_ptr->history = NULL;
				if (cell_ptr->context != NULL)
				{
#ifdef NEW_FRAME_HIST
					/*
					 * The context still owns its history
					 * list, so null the cell's pointer to
					 * be clear.  If this cell doesn't have
					 * a context, then it owns the history
					 * list and lo_FreeGridCellRec() will
					 * free it.
					 */
					cell_ptr->hist_list = NULL;
#endif /* NEW_FRAME_HIST */
					(void)FE_FreeGridWindow(
						cell_ptr->context, FALSE);
					cell_ptr->context = 0;		/* jwz */
				}
				tmp_cell = cell_ptr;
				cell_ptr = cell_ptr->next;
				lo_FreeGridCellRec(context, tmp_cell);
			}
			lo_FreeGridRec(grid);
		}
		state->top_state->the_grid = NULL;
		lo_internal_doc_data = save;
	}

	if ( bWholeDoc && state->top_state->url != NULL)
	{
		XP_FREE(state->top_state->url);
		state->top_state->url = NULL;
	}

	if ( bWholeDoc && state->top_state->base_url != NULL)
	{
		XP_FREE(state->top_state->base_url);
		state->top_state->base_url = NULL;
	}

	if ( bWholeDoc && state->top_state->base_target != NULL)
	{
		XP_FREE(state->top_state->base_target);
		state->top_state->base_target = NULL;
	}

	if ( bWholeDoc && state->top_state->name_target != NULL)
	{
		XP_FREE(state->top_state->name_target);
		state->top_state->name_target = NULL;
	}

#ifdef HTML_CERTIFICATE_SUPPORT
	if ( bWholeDoc && state->top_state->cert_list != NULL)
	{
		lo_Certificate *cptr;

		cptr = state->top_state->cert_list;
		while (cptr != NULL)
		{
			lo_Certificate *lo_cert;

			lo_cert = cptr;
			cptr = cptr->next;
			if (lo_cert->name != NULL)
			{
				PA_FREE(lo_cert->name);
			}
			if (lo_cert->cert != NULL)
			{
				CERT_DestroyCertificate(lo_cert->cert);
			}
			XP_DELETE(lo_cert);
		}
		state->top_state->cert_list = NULL;
	}
#endif /* HTML_CERTIFICATE_SUPPORT */

	if (bWholeDoc && state->top_state->unknown_head_tag != NULL)
	{
		XP_FREE(state->top_state->unknown_head_tag);
		state->top_state->unknown_head_tag = NULL;
	}

#ifdef MOCHA
	if (state->top_state->mocha_mark)
	{
		LM_ReleaseDocument(context, resize_reload);
		state->top_state->mocha_mark = FALSE;
	}

	/* XXX all this because grid docs are never re-parsed after first load. */
	if (!resize_reload &&
		((state->top_state->savedData.OnLoad != NULL)||
		 (state->top_state->savedData.OnUnload != NULL)||
		 (state->top_state->savedData.OnFocus != NULL)||
		 (state->top_state->savedData.OnBlur != NULL)))
	{
		History_entry *he = SHIST_GetCurrent(&context->hist);
		if (he)
		{
			he->savedData.OnLoad = state->top_state->savedData.OnLoad;
			he->savedData.OnUnload = state->top_state->savedData.OnUnload;
			he->savedData.OnFocus = state->top_state->savedData.OnFocus;
			he->savedData.OnBlur = state->top_state->savedData.OnBlur;
		}
		else
		{
			if (state->top_state->savedData.OnLoad != NULL)
				PA_FREE(state->top_state->savedData.OnLoad);
			if (state->top_state->savedData.OnUnload != NULL)
				PA_FREE(state->top_state->savedData.OnUnload);
			if (state->top_state->savedData.OnFocus != NULL)
				PA_FREE(state->top_state->savedData.OnFocus);
			if (state->top_state->savedData.OnBlur != NULL)
				PA_FREE(state->top_state->savedData.OnBlur);
		}
		state->top_state->savedData.OnLoad = NULL;
		state->top_state->savedData.OnUnload = NULL;
		state->top_state->savedData.OnFocus = NULL;
		state->top_state->savedData.OnBlur = NULL;
	}
#endif /* MOCHA */

	if ( bWholeDoc )
	{
        recycle_list = state->top_state->recycle_list;
	    XP_DELETE(state->top_state);
	    XP_DELETE(state);

	    /*
	     * Get the unique document ID, and remove this
	     * documents layout state.
	     */
	    doc_id = XP_DOCID(context);
	    lo_RemoveTopState(doc_id);

		/*
		 * Note that frame contexts are destroyed and recreated
		 * on reload, so we must defer this IL_StartPage call till
		 * XP_RemoveContextFromList to let libmocha deal with any
		 * anonymous images.
		 */
		if (!context->is_grid_cell)
		{
			IL_StartPage(context);
		}
    }
    else
    {
        XP_DELETE(state);
        recycle_list = NULL;
    }
	return(recycle_list);
}


void
LO_DiscardDocument(MWContext *context)
{
	int32 doc_id;
	lo_TopState *top_state;
	lo_DocState *state;
	LO_Element *recycle_list;
	int32 cnt;
#ifdef MEMORY_ARENAS
	lo_arena *old_arena;
#endif /* MEMORY_ARENAS */

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
	state = top_state->doc_state;
	
#ifdef MEMORY_ARENAS
    if ( ! EDT_IS_EDITOR(context) ) {
	    old_arena = top_state->first_arena;
    }
#endif /* MEMORY_ARENAS */
	lo_SaveFormElementState(context, state, TRUE);
	/*
	 * If this document has no session history
	 * this data is not saved there and will get leaked
	 * if we don't free it now.
	 */
	if (SHIST_GetCurrent(&context->hist) == NULL)
	{
		LO_FreeDocumentFormListData(context, (void *)top_state->savedData.FormList);
		top_state->savedData.FormList = NULL;
		LO_FreeDocumentEmbedListData(context, (void *)top_state->savedData.EmbedList);
		top_state->savedData.EmbedList = NULL;
		LO_FreeDocumentGridData(context, (void *)top_state->savedData.Grid);
		top_state->savedData.Grid = NULL;
	}
	recycle_list =
		lo_InternalDiscardDocument(context, state, lo_internal_doc_data, TRUE);
	/* see first lo_FreeRecycleList comment. safe */
	cnt = lo_FreeRecycleList(context, recycle_list);
#ifdef MEMORY_ARENAS
    if ( ! EDT_IS_EDITOR(context) ) {
	    cnt = lo_FreeMemoryArena(old_arena);
    }
#endif /* MEMORY_ARENAS */
}


lo_SavedFormListData *
lo_NewDocumentFormListData(void)
{
	lo_SavedFormListData *fptr;

	fptr = XP_NEW(lo_SavedFormListData);
	if (fptr == NULL)
	{
		return(NULL);
	}

	fptr->valid = FALSE;
	fptr->data_index = 0;
	fptr->data_count = 0;
	fptr->data_list = NULL;
	fptr->next = NULL;

	return(fptr);
}


void
LO_FreeDocumentFormListData(MWContext *context, void *data)
{
	lo_SavedFormListData *forms = (lo_SavedFormListData *)data;

	if (forms != NULL)
	{
		lo_FreeDocumentFormListData(context, forms);
		XP_DELETE(forms);
	}
}


lo_SavedGridData *
lo_NewDocumentGridData(void)
{
	lo_SavedGridData *sgptr;

	sgptr = XP_NEW(lo_SavedGridData);
	if (sgptr == NULL)
	{
		return(NULL);
	}

	sgptr->main_width = 0;
	sgptr->main_height = 0;
	sgptr->the_grid = NULL;

	return(sgptr);
}


void
LO_FreeDocumentGridData(MWContext *context, void *data)
{
	lo_SavedGridData *sgptr = (lo_SavedGridData *)data;

	if (sgptr != NULL)
	{
		lo_FreeDocumentGridData(context, sgptr);
		XP_DELETE(sgptr);
	}
}


lo_SavedEmbedListData *
lo_NewDocumentEmbedListData(void)
{
	lo_SavedEmbedListData *eptr;

	eptr = XP_NEW(lo_SavedEmbedListData);
	if (eptr == NULL)
	{
		return(NULL);
	}

	eptr->embed_count = 0;
	eptr->embed_data_list = NULL;

	return(eptr);
}


void
LO_FreeDocumentEmbedListData(MWContext *context, void *data)
{
	lo_SavedEmbedListData *eptr = (lo_SavedEmbedListData *)data;

	if (eptr != NULL)
	{
		lo_FreeDocumentEmbedListData(context, eptr);
		XP_DELETE(eptr);
	}
}

void
LO_HighlightAnchor(MWContext *context, LO_Element *element, Bool on)
{
	int32 doc_id;
	lo_TopState *top_state;
	lo_DocState *state;
	LO_Element *start, *end;
	LO_AnchorData *anchor;
	Bool done;

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
	state = top_state->doc_state;

	if (element == NULL)
	{
		return;
	}

	switch(element->type)
	{
		case LO_TEXT:
			anchor = element->lo_text.anchor_href;
			break;
		case LO_IMAGE:
			anchor = element->lo_image.anchor_href;
			break;
		case LO_TABLE:
			anchor = element->lo_table.anchor_href;
			break;
		case LO_SUBDOC:
			anchor = element->lo_subdoc.anchor_href;
			break;
		case LO_LINEFEED:
			anchor = element->lo_linefeed.anchor_href;
			break;
		default:
			anchor = NULL;
			break;
	}

	if (anchor == NULL)
	{
		return;
	}

	start = element;
	done = FALSE;
	while ((start->lo_any.prev != NULL)&&(done == FALSE))
	{
		switch(start->lo_any.prev->type)
		{
		    case LO_TEXT:
			if (start->lo_any.prev->lo_text.anchor_href !=
				anchor)
			{
				done = TRUE;
			}
			break;
		    case LO_IMAGE:
			if (start->lo_any.prev->lo_image.anchor_href !=
				anchor)
			{
				done = TRUE;
			}
			break;
		    case LO_TABLE:
			if (start->lo_any.prev->lo_table.anchor_href !=
				anchor)
			{
				done = TRUE;
			}
			break;
		    case LO_SUBDOC:
			if (start->lo_any.prev->lo_subdoc.anchor_href !=
				anchor)
			{
				done = TRUE;
			}
			break;
		    case LO_LINEFEED:
			if (start->lo_any.prev->lo_linefeed.anchor_href !=
				anchor)
			{
				done = TRUE;
			}
			break;
		    default:
			done = TRUE;
			break;
		}
		if (done == FALSE)
		{
			start = start->lo_any.prev;
		}
	}

	end = element;
	done = FALSE;
	while ((end->lo_any.next != NULL)&&(done == FALSE))
	{
		switch(end->lo_any.next->type)
		{
		    case LO_TEXT:
			if (end->lo_any.next->lo_text.anchor_href !=
				anchor)
			{
				done = TRUE;
			}
			break;
		    case LO_IMAGE:
			if (end->lo_any.next->lo_image.anchor_href !=
				anchor)
			{
				done = TRUE;
			}
			break;
		    case LO_TABLE:
			if (end->lo_any.next->lo_table.anchor_href !=
				anchor)
			{
				done = TRUE;
			}
			break;
		    case LO_SUBDOC:
			if (end->lo_any.next->lo_subdoc.anchor_href !=
				anchor)
			{
				done = TRUE;
			}
			break;
		    case LO_LINEFEED:
			if (end->lo_any.next->lo_linefeed.anchor_href !=
				anchor)
			{
				done = TRUE;
			}
			break;
		    default:
			done = TRUE;
			break;
		}
		if (done == FALSE)
		{
			end = end->lo_any.next;
		}
	}

	end = end->lo_any.next;
	while (start != end)
	{
		LO_Color save_fg;

		start->lo_any.x += state->base_x;
		start->lo_any.y += state->base_y;

		switch (start->type)
		{
			case LO_TEXT:
				if (start->lo_text.text != NULL)
				{
					save_fg.red =
					    start->lo_text.text_attr->fg.red;
					save_fg.green =
					    start->lo_text.text_attr->fg.green;
					save_fg.blue =
					    start->lo_text.text_attr->fg.blue;
				    if (on != FALSE)
				    {
					start->lo_text.text_attr->fg.red =
						STATE_SELECTED_ANCHOR_RED(state);
					start->lo_text.text_attr->fg.green =
						STATE_SELECTED_ANCHOR_GREEN(state);
					start->lo_text.text_attr->fg.blue =
						STATE_SELECTED_ANCHOR_BLUE(state);
				    }
					lo_DisplayText(context, FE_VIEW,
						(LO_TextStruct *)start, FALSE);
					start->lo_text.text_attr->fg.red =
						save_fg.red;
					start->lo_text.text_attr->fg.green =
						save_fg.green;
					start->lo_text.text_attr->fg.blue =
						save_fg.blue;
				}
				break;
			case LO_IMAGE:
				{
				Bool saveSelected;

				saveSelected = FALSE;
				if (start->lo_image.ele_attrmask &
					LO_ELE_SELECTED)
				{
					saveSelected = TRUE;
				}
				save_fg.red =
				    start->lo_image.text_attr->fg.red;
				save_fg.green =
				    start->lo_image.text_attr->fg.green;
				save_fg.blue =
				    start->lo_image.text_attr->fg.blue;
			    if (on != FALSE)
			    {
				start->lo_image.ele_attrmask |= LO_ELE_SELECTED;
				start->lo_image.text_attr->fg.red =
					STATE_SELECTED_ANCHOR_RED(state);
				start->lo_image.text_attr->fg.green =
					STATE_SELECTED_ANCHOR_GREEN(state);
				start->lo_image.text_attr->fg.blue =
					STATE_SELECTED_ANCHOR_BLUE(state);
			    }
				lo_DisplayImage(context, FE_VIEW,
					(LO_ImageStruct *)start);
				start->lo_image.text_attr->fg.red =
					save_fg.red;
				start->lo_image.text_attr->fg.green =
					save_fg.green;
				start->lo_image.text_attr->fg.blue =
					save_fg.blue;
				if (saveSelected == FALSE)
				{
					start->lo_image.ele_attrmask &=
						(~(LO_ELE_SELECTED));
				}
				else
				{
					start->lo_image.ele_attrmask |=
						LO_ELE_SELECTED;
				}
				}
				break;
			case LO_SUBDOC:
				save_fg.red =
				    start->lo_subdoc.text_attr->fg.red;
				save_fg.green =
				    start->lo_subdoc.text_attr->fg.green;
				save_fg.blue =
				    start->lo_subdoc.text_attr->fg.blue;
			    if (on != FALSE)
			    {
				start->lo_subdoc.text_attr->fg.red =
					STATE_SELECTED_ANCHOR_RED(state);
				start->lo_subdoc.text_attr->fg.green =
					STATE_SELECTED_ANCHOR_GREEN(state);
				start->lo_subdoc.text_attr->fg.blue =
					STATE_SELECTED_ANCHOR_BLUE(state);
			    }
				lo_DisplaySubDoc(context, FE_VIEW,
					(LO_SubDocStruct *)start);
				start->lo_subdoc.text_attr->fg.red =
					save_fg.red;
				start->lo_subdoc.text_attr->fg.green =
					save_fg.green;
				start->lo_subdoc.text_attr->fg.blue =
					save_fg.blue;
				break;
			case LO_TABLE:
			case LO_LINEFEED:
			case LO_HRULE:
			case LO_FORM_ELE:
			case LO_BULLET:
			default:
				break;
		}

		start->lo_any.x -= state->base_x;
		start->lo_any.y -= state->base_y;

		start = start->lo_any.next;
	}
}


LO_Element *
LO_XYToNearestElement(MWContext *context, int32 x, int32 y)
{
	int32 doc_id;
	lo_TopState *top_state;
	lo_DocState *state;
	LO_Element *eptr;
	LO_Element **line_array;
	int32 ret_x, ret_y;

	/*
	 * Get the unique document ID, and retreive this
	 * documents layout state.
	 */
	doc_id = XP_DOCID(context);
	top_state = lo_FetchTopState(doc_id);
	if ((top_state == NULL)||(top_state->doc_state == NULL))
	{
		return(NULL);
	}
	state = top_state->doc_state;

	if (x <= state->win_left)
	{
		x = state->win_left + 1;
	}

	if (y < state->win_top)
	{
		y = state->win_top + 1;
	}

	eptr = lo_XYToDocumentElement(context, state, x, y, TRUE, TRUE, 
		&ret_x, &ret_y);

	if (eptr == NULL)
	{
		int32 top, bottom;

		/*	
		 * Find the nearest line.
		 */
		lo_RegionToLines (context, state, x, y, 1, 1, FALSE,
			&top, &bottom);

		if (top >= 0)
#ifdef XP_WIN16
		{
			intn a_size;
			intn a_indx;
			intn a_line;
			XP_Block *larray_array;

			a_size = SIZE_LIMIT / sizeof(LO_Element *);
			a_indx = (intn)(top / a_size);
			a_line = (intn)(top - (a_indx * a_size));
			XP_LOCK_BLOCK(larray_array, XP_Block *,
					state->larray_array);
			state->line_array = larray_array[a_indx];
			XP_LOCK_BLOCK(line_array, LO_Element **,
					state->line_array);
			eptr = line_array[a_line];
			XP_UNLOCK_BLOCK(state->line_array);
			XP_UNLOCK_BLOCK(state->larray_array);
		}
#else
		{
			XP_LOCK_BLOCK(line_array, LO_Element **,
					state->line_array);
			eptr = line_array[top];
			XP_UNLOCK_BLOCK(state->line_array);
		}
#endif /* XP_WIN16 */

		/*
		 * The nearest line may be a table with cells.  In which case
		 * we really need to move into that table to find the nearest
		 * cell, and possibly nearest element in that cell.
		 */
		if ((eptr != NULL)&&(eptr->type == LO_TABLE)&&
		    (eptr->lo_any.next->type == LO_CELL))
		{
		    LO_Element *new_eptr;

		    new_eptr = eptr->lo_any.next;
		    /*
		     * Find the cell that overlaps in Y.  Move to the lower
		     * cell if between cells.
		     */
		    while ((new_eptr != NULL)&&(new_eptr->type == LO_CELL))
		    {
			int32 y2;

			y2 = new_eptr->lo_any.y + new_eptr->lo_any.y_offset +
				new_eptr->lo_any.height +
				(2 * new_eptr->lo_cell.border_width);
			if (y <= y2)
			{
				break;
			}
			new_eptr = new_eptr->lo_any.next;
		    }
		    /*
		     * If we walked through the table to a non-cell element
		     * and still didn't match, match to the last cell in
		     * the table.
		     */
		    if ((new_eptr != NULL)&&(new_eptr->type != LO_CELL))
		    {
			new_eptr = new_eptr->lo_any.prev;
		    }

		    /*
		     * If new_eptr is not NULL it is the cell we matched.
		     * Move us just into that cell and see if we can match
		     * an element inside it.
		     */
		    if ((new_eptr != NULL)&&(new_eptr->type == LO_CELL))
		    {
			LO_Element *cell_eptr;
			int32 new_x, new_y;

			new_y = y;
			if (new_y >= (new_eptr->lo_any.y +
					new_eptr->lo_any.y_offset +
					new_eptr->lo_cell.border_width +
					new_eptr->lo_any.height))
			{
				new_y = new_eptr->lo_any.y +
					new_eptr->lo_any.y_offset +
					new_eptr->lo_cell.border_width +
					new_eptr->lo_any.height - 1;
			}
			if (new_y < (new_eptr->lo_any.y +
					new_eptr->lo_any.y_offset +
					new_eptr->lo_cell.border_width))
			{
				new_y = new_eptr->lo_any.y +
					new_eptr->lo_any.y_offset +
					new_eptr->lo_cell.border_width;
			}
			new_x = x;
			if (new_x >= (new_eptr->lo_any.x +
					new_eptr->lo_any.x_offset +
					new_eptr->lo_cell.border_width +
					new_eptr->lo_any.width))
			{
				new_x = new_eptr->lo_any.x +
					new_eptr->lo_any.x_offset +
					new_eptr->lo_cell.border_width +
					new_eptr->lo_any.width - 1;
			}
			if (new_x < (new_eptr->lo_any.x +
					new_eptr->lo_any.x_offset +
					new_eptr->lo_cell.border_width))
			{
				new_x = new_eptr->lo_any.x +
					new_eptr->lo_any.x_offset +
					new_eptr->lo_cell.border_width;
			}
			cell_eptr = lo_XYToNearestCellElement(context, state,
				(LO_CellStruct *)new_eptr, new_x, new_y);
			if (cell_eptr != NULL)
			{
				new_eptr = cell_eptr;
			}
		    }
		    if (new_eptr != NULL)
		    {
			eptr = new_eptr;
		    }
		}
	}

	return(eptr);
}


void
LO_RefreshAnchors(MWContext *context)
{
	int32 doc_id;
	lo_TopState *top_state;
	lo_DocState *state;
	LO_Element *eptr;
	LO_Element **line_array;

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
	state = top_state->doc_state;

	/*
	 * line_array could be NULL when discarding the document, so
	 * check it and exit early to avoid dereferencing it below.
	 */
	if (state->line_array == NULL)
		return;

#ifdef XP_WIN16
	{
		XP_Block *larray_array;

		XP_LOCK_BLOCK(larray_array, XP_Block *, state->larray_array);
		state->line_array = larray_array[0];
		XP_UNLOCK_BLOCK(state->larray_array);
	}
#endif /* XP_WIN16 */

	XP_LOCK_BLOCK(line_array, LO_Element **, state->line_array);
	eptr = line_array[0];
	XP_UNLOCK_BLOCK(state->line_array);

	while (eptr != NULL)
	{
		switch (eptr->type)
		{
		    case LO_TEXT:
			if (eptr->lo_text.text_attr != NULL)
			{
			    LO_TextAttr tmp_attr;
			    LO_TextAttr *attr;
			    LO_TextAttr *new_attr;

			    attr = eptr->lo_text.text_attr;
			    if ((attr->attrmask & LO_ATTR_ANCHOR)&&
				(eptr->lo_text.anchor_href != NULL))
			    {
				char *url;

				lo_CopyTextAttr(attr, &tmp_attr);
				PA_LOCK(url, char *,
					eptr->lo_text.anchor_href->anchor);
				if (GH_CheckGlobalHistory(url) != -1)
				{
				    tmp_attr.fg.red =   STATE_VISITED_ANCHOR_RED(state);
				    tmp_attr.fg.green = STATE_VISITED_ANCHOR_GREEN(state);
				    tmp_attr.fg.blue =  STATE_VISITED_ANCHOR_BLUE(state);
				}
				else
				{
				    tmp_attr.fg.red =   STATE_UNVISITED_ANCHOR_RED(state);
				    tmp_attr.fg.green = STATE_UNVISITED_ANCHOR_GREEN(state);
				    tmp_attr.fg.blue =  STATE_UNVISITED_ANCHOR_BLUE(state);
				}
				PA_UNLOCK(eptr->lo_text.anchor_href->anchor);

				new_attr = lo_FetchTextAttr(state, &tmp_attr);
				if (new_attr != attr)
				{
				    eptr->lo_text.text_attr = new_attr;
				    eptr->lo_any.x += state->base_x;
				    eptr->lo_any.y += state->base_y;
				    lo_DisplayText(context, FE_VIEW,
					(LO_TextStruct *)eptr, FALSE);
				    eptr->lo_any.x -= state->base_x;
				    eptr->lo_any.y -= state->base_y;
				}
			    }
			}
			break;
		    case LO_IMAGE:
			if ((eptr->lo_image.text_attr != NULL)&&
			    (eptr->lo_image.image_attr != NULL))
			{
			    LO_TextAttr tmp_attr;
			    LO_TextAttr *attr;
			    LO_TextAttr *new_attr;
			    LO_ImageAttr *img_attr;

			    attr = eptr->lo_image.text_attr;
			    img_attr = eptr->lo_image.image_attr;
			    if ((img_attr->attrmask & LO_ATTR_ANCHOR)&&
				(eptr->lo_image.anchor_href != NULL))
			    {
				char *url;

				lo_CopyTextAttr(attr, &tmp_attr);
				PA_LOCK(url, char *,
					eptr->lo_image.anchor_href->anchor);
				if (GH_CheckGlobalHistory(url) != -1)
				{
				    tmp_attr.fg.red =   STATE_VISITED_ANCHOR_RED(state);
				    tmp_attr.fg.green = STATE_VISITED_ANCHOR_GREEN(state);
				    tmp_attr.fg.blue =  STATE_VISITED_ANCHOR_BLUE(state);
				}
				else
				{
				    tmp_attr.fg.red =   STATE_UNVISITED_ANCHOR_RED(state);
				    tmp_attr.fg.green = STATE_UNVISITED_ANCHOR_GREEN(state);
				    tmp_attr.fg.blue =  STATE_UNVISITED_ANCHOR_BLUE(state);
				}
				PA_UNLOCK(eptr->lo_image.anchor_href->anchor);

				new_attr = lo_FetchTextAttr(state, &tmp_attr);
				if (new_attr != attr)
				{
				    eptr->lo_image.text_attr = new_attr;
				    eptr->lo_any.x += state->base_x;
				    eptr->lo_any.y += state->base_y;
				    lo_DisplayImage(context, FE_VIEW,
					(LO_ImageStruct *)eptr);
				    eptr->lo_any.x -= state->base_x;
				    eptr->lo_any.y -= state->base_y;
				}
			    }
			}
			break;
		    case LO_TABLE:
		    case LO_SUBDOC:
		    case LO_LINEFEED:
		    case LO_HRULE:
		    case LO_FORM_ELE:
		    case LO_BULLET:
		    default:
			break;
		}
		eptr = eptr->lo_any.next;
	}
}


static void
lo_split_named_anchor(char *url, char **cmp_url, char **cmp_name)
{
	char tchar, *tptr, *tname;

	*cmp_url = NULL;
	*cmp_name = NULL;

	if (url == NULL)
	{
		return;
	}

	tptr = url;
	while (((tchar = *tptr) != '\0')&&(tchar != '#'))
	{
		tptr++;
	}
	if (tchar == '\0')
	{
		*cmp_url = (char *)XP_STRDUP(url);
		return;
	}

	*tptr = '\0';
	*cmp_url = (char *)XP_STRDUP(url);
	*tptr = tchar;
	tname = ++tptr;
	while (((tchar = *tptr) != '\0')&&(tchar != '?'))
	{
		tptr++;
	}
	*tptr = '\0';
	*cmp_name = (char *)XP_STRDUP(tname);
	*tptr = tchar;
	if (tchar == '?')
	{
		StrAllocCat(*cmp_url, tptr);
	}
}


/*
 * Is this anchor local to this document?
 */
Bool
lo_IsAnchorInDoc(lo_DocState *state, char *url)
{
	char *cmp_url, *cmp_name;
	char *doc_url, *doc_name;
	Bool local;

	local = FALSE;
	/*
	 * Extract the base from the name part.
	 */
	lo_split_named_anchor(url, &cmp_url, &cmp_name);
	if (cmp_name != NULL)
	{
		XP_FREE(cmp_name);
	}

	/*
	 * Extract the base from the name part.
	 */
	doc_url = NULL;
	doc_name = NULL;
	lo_split_named_anchor(state->top_state->url, &doc_url, &doc_name);
	if (doc_name != NULL)
	{
		XP_FREE(doc_name);
	}

	/*
	 * If the document itself has no base URL (probably an error)
	 * then the only way we are local is if the passed in url has no
	 * base either.
	 */
	if (doc_url == NULL)
	{
		if (cmp_url == NULL)
		{
			local = TRUE;
		}
	}
	/*
	 * Else if the passed in url has no base, and we have a bas, you must
	 * be local because you are relative to our base.
	 */
	else if (cmp_url == NULL)
	{
		local = TRUE;
	}
	/*
	 * Else if the base of the two urls is equal, you are local.
	 */
	else if (XP_STRCMP(cmp_url, doc_url) == 0)
	{
		local = TRUE;
	}

	if (cmp_url != NULL)
	{
		XP_FREE(cmp_url);
	}
	if (doc_url != NULL)
	{
		XP_FREE(doc_url);
	}
	return(local);
}


static LO_Element *
lo_cell_id_to_element(lo_DocState *state, int32 ele_id, LO_CellStruct *cell)
{
	LO_Element *eptr;

	if (cell == NULL)
	{
		return(NULL);
	}

	eptr = cell->cell_list;
	while (eptr != NULL)
	{
		if (eptr->lo_any.ele_id == ele_id)
		{
			break;
		}
		if (eptr->type == LO_CELL)
		{
			LO_Element *cell_eptr;

			cell_eptr = lo_cell_id_to_element(state, ele_id,
						(LO_CellStruct *)eptr);
			if (cell_eptr != NULL)
			{
				eptr = cell_eptr;
				break;
			}
		}
		if (eptr->lo_any.ele_id > ele_id)
		{
			break;
		}
		eptr = eptr->lo_any.next;
	}
	return(eptr);
}


/*
 * Find the x, y, coords of the element id passed in, and return them.
 * If can't find the exact element, return the closest (next greater)
 * element id's position.
 * We need to walk into cells.
 * On severe error return 0, 0.
 */
static void
lo_element_id_to_xy(lo_DocState *state, int32 ele_id, int32 *xpos, int32 *ypos)
{
	LO_Element *eptr;
	LO_Element **line_array;

	*xpos = 0;
	*ypos = 0;

	/*
	 * On error return the top
	 */
	if (state == NULL)
	{
		return;
	}

#ifdef XP_WIN16
	{
		XP_Block *larray_array;

		XP_LOCK_BLOCK(larray_array, XP_Block *, state->larray_array);
		state->line_array = larray_array[0];
		XP_UNLOCK_BLOCK(state->larray_array);
	}
#endif /* XP_WIN16 */

	XP_LOCK_BLOCK(line_array, LO_Element **, state->line_array);
	eptr = line_array[0];
	XP_UNLOCK_BLOCK(state->line_array);

	while (eptr != NULL)
	{
		/*
		 * Exact match?
		 */
		if (eptr->lo_any.ele_id == ele_id)
		{
			*xpos = eptr->lo_any.x;
			*ypos = eptr->lo_any.y;
			break;
		}

		/*
		 * Look for a match in this cell
		 */
		if (eptr->type == LO_CELL)
		{
			LO_Element *cell_eptr;

			cell_eptr = lo_cell_id_to_element(state, ele_id,
						(LO_CellStruct *)eptr);
			if (cell_eptr != NULL)
			{
				*xpos = cell_eptr->lo_any.x;
				*ypos = cell_eptr->lo_any.y;
				break;
			}
		}

		/*
		 * If we passed it, fake a match.
		 */
		if (eptr->lo_any.ele_id > ele_id)
		{
			*xpos = eptr->lo_any.x;
			*ypos = eptr->lo_any.y;
			break;
		}
		eptr = eptr->lo_any.next;
	}
}


/*
 * Check the current URL to see if it is the URL for this doc, and if
 * so return the position if the name referenced after the '#' in
 * the URL.  If there is no name or no match, return 0,0.  In comparing URLs
 * both NULL are considered equal
 */
Bool
LO_LocateNamedAnchor(MWContext *context, URL_Struct *url_struct,
			int32 *xpos, int32 *ypos)
{
	int32 doc_id;
	lo_TopState *top_state;
	lo_DocState *state;
	lo_NameList *nptr;
	char *url;
	char *cmp_url;
	char *cmp_name;

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
	state = top_state->doc_state;

	/*
	 * obvious error
	 */
	if ((url_struct == NULL)||(url_struct->address == NULL))
	{
		return(FALSE);
	}

	/*
	 * Extract the URL we are going to.
	 */
	url = url_struct->address;


	/*
	 * Split the passed url into the real url part
	 * and the name part.  The parts (if non-NULL) must
	 * be freed.
	 */
	cmp_url = NULL;
	cmp_name = NULL;
	lo_split_named_anchor(url, &cmp_url, &cmp_name);

	/*
	 * can't strcmp on NULL strings.
	 */
	if (state->top_state->url == NULL)
	{
		/*
		 * 2 NULLs are considered equal.
		 * If not equal fail here
		 */
		if (cmp_url != NULL)
		{
			if (cmp_name != NULL)
			{
				XP_FREE(cmp_name);
			}
			return(FALSE);
		}
	}
	/*
	 * Else lets compare the 2 URLs now
	 */
	else
	{
		/*
		 * The current URL might itself have a name part.  Lose it.
		 */
		char *this_url;
		char *this_name;
		int same_p;

		lo_split_named_anchor(state->top_state->url,
			&this_url, &this_name);

		/*
		 * If the 2 URLs are not equal fail out here
		 */
		same_p = ((this_url)&&(XP_STRCMP(cmp_url, this_url) == 0));

		if (this_url != NULL)
		{
			XP_FREE (this_url);
		}

		if (this_name)
		{
			XP_FREE (this_name);
		}

		if (!same_p)
		{
			XP_FREE(cmp_url);
			if (cmp_name != NULL)
			{
				XP_FREE(cmp_name);
			}
			return(FALSE);
		}
	}

	/*
	 * If we got here the URLs are considered equal.
	 * Free this one.
	 */
	if (cmp_url != NULL)
	{
		XP_FREE(cmp_url);
	}

	/*
	 * Do we have a saved element ID that is not the start
	 * of the document to return to because we are moving through history?
	 * If so, go there and return.
	 */
	if (url_struct->position_tag > 1)
	{
		lo_element_id_to_xy(state, url_struct->position_tag,
			xpos, ypos);
		return(TRUE);
	}

	/*
	 * Special case for going to a NULL or empty
	 * name to return the beginning of the
	 * document.
	 */
	if (cmp_name == NULL)
	{
		*xpos = 0;
		*ypos = 0;

		if (state->top_state->url != NULL)
		{
			XP_FREE(state->top_state->url);
		}
		state->top_state->url = XP_STRDUP(url);
		return(TRUE);
	}
	else if (*cmp_name == '\0')
	{
		*xpos = 0;
		*ypos = 0;
		XP_FREE(cmp_name);

		if (state->top_state->url != NULL)
		{
			XP_FREE(state->top_state->url);
		}
		state->top_state->url = XP_STRDUP(url);
		return(TRUE);
	}

	/*
	 * Actually search the name list for a matching name.
	 */
	nptr = state->name_list;
	while (nptr != NULL)
	{
		char *name;

		PA_LOCK(name, char *, nptr->name);
		/*
		 * If this is a match, return success
		 * here.
		 */
		if (XP_STRCMP(cmp_name, name) == 0)
		{
			PA_UNLOCK(nptr->name);
			if (nptr->element == NULL)
			{
				*xpos = 0;
				*ypos = 0;
			}
			else
			{
				*xpos = nptr->element->lo_any.x;
				*ypos = nptr->element->lo_any.y;
			}
			XP_FREE(cmp_name);

			if (state->top_state->url != NULL)
			{
				XP_FREE(state->top_state->url);
			}
			state->top_state->url = XP_STRDUP(url);
			return(TRUE);
		}
		PA_UNLOCK(nptr->name);
		nptr = nptr->next;
	}

	/*
	 * Failed to find the matchng name.
	 * If no match return the top of the doc.
	 */
	XP_FREE(cmp_name);
	*xpos = 0;
	*ypos = 0;

	if (state->top_state->url != NULL)
	{
		XP_FREE(state->top_state->url);
	}
	state->top_state->url = XP_STRDUP(url);
	return(TRUE);
}


Bool
LO_HasBGImage(MWContext *context)
{
	int32 doc_id;
	lo_TopState *top_state;

	/*
	 * Get the unique document ID, and retreive this
	 * documents layout state.
	 */
	doc_id = XP_DOCID(context);
	top_state = lo_FetchTopState(doc_id);
	if (top_state == NULL)
	{
		return(FALSE);
	}

#ifdef XP_WIN
	/*
	 * Evil hack alert.
	 * We don't want print context's under windows to use backdrops
	 *  currently as it causes bad display (black in the transparent
	 *  area due to drawing problems under windows).
	 */
	 if(context->type == MWContextPrint || context->type == MWContextMetaFile)
	 {
	 	return(FALSE);
	 }
#endif

	return(top_state->have_a_bg_url);
}


#ifdef TEST_16BIT
#undef XP_WIN16
#endif /* TEST_16BIT */

#ifdef PROFILE
#pragma profile off
#endif
