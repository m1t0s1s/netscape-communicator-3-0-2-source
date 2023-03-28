/* -*- Mode: C; tab-width: 4; -*- */
#include "xp.h"
#include "net.h"
#include "pa_parse.h"
#include "layout.h"
#include "java.h"

#define JAVA_DEF_DIM			50
#define JAVA_DEF_BORDER		0
#define JAVA_DEF_VERTICAL_SPACE	0
#define JAVA_DEF_HORIZONTAL_SPACE	0

void lo_FinishJavaApp(MWContext *, lo_DocState *, LO_JavaAppStruct *);

void
LO_ClearJavaAppBlock(MWContext *context, LO_JavaAppStruct *java_app)
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

	if (top_state->layout_blocking_element == (LO_Element *)java_app)
	{
		if (java_app->width == 0)
		{
			java_app->width = JAVA_DEF_DIM;
		}
		if (java_app->height == 0)
		{
			java_app->height = JAVA_DEF_DIM;
		}

		main_doc_state = top_state->doc_state;
		state = lo_CurrentSubState(main_doc_state);

		lo_FinishJavaApp(context, state, java_app);
		lo_FlushBlockage(context, state, main_doc_state);
	}
}


void
lo_FormatJavaApp(MWContext *context, lo_DocState *state, PA_Tag *tag)
{
	LO_JavaAppStruct *java_app;
	PA_Block buff;
	char *str;
	int32 val;
	int32 doc_width;
	Bool widthSpecified = FALSE;
	Bool heightSpecified = FALSE;

	/*
	 * Fill in the java applet structure with default data
	 */
	java_app = (LO_JavaAppStruct *)lo_NewElement(context, state, LO_JAVA, NULL, 0);
	if (java_app == NULL)
	{
		return;
	}

	java_app->type = LO_JAVA;
	java_app->ele_id = NEXT_ELEMENT;
	java_app->x = state->x;
	java_app->x_offset = 0;
	java_app->y = state->y;
	java_app->y_offset = 0;
	java_app->width = 0;
	java_app->height = 0;
	java_app->next = NULL;
	java_app->prev = NULL;
    java_app->nextApplet = NULL;
#ifdef MOCHA
    java_app->mocha_object = NULL;
#endif

	java_app->FE_Data = NULL;
	java_app->session_data = NULL;
	java_app->line_height = state->line_height;

	java_app->base_url = NULL;
	java_app->attr_code = NULL;
	java_app->attr_codebase = NULL;
	java_app->attr_archive = NULL;
	java_app->attr_name = NULL;

	java_app->param_cnt = 0;
	java_app->param_names = NULL;
	java_app->param_values = NULL;

	java_app->may_script = FALSE;

	java_app->alignment = LO_ALIGN_BASELINE;
	java_app->border_width = JAVA_DEF_BORDER;
	java_app->border_vert_space = JAVA_DEF_VERTICAL_SPACE;
	java_app->border_horiz_space = JAVA_DEF_HORIZONTAL_SPACE;
	java_app->ele_attrmask = 0;

	/*
	 * Assign a unique index for this object 
	 * and increment the master index.
	 */
	java_app->embed_index = state->top_state->embed_count++;

	/*
	 * Save away the base of the document
	 */
	buff = PA_ALLOC(XP_STRLEN(state->top_state->base_url) + 1);
	if (buff != NULL)
	{
		char *cp;
		PA_LOCK(cp, char*, buff);
		XP_STRCPY(cp, state->top_state->base_url);
		PA_UNLOCK(buff);
		java_app->base_url = buff;
	}
	else
	{
		state->top_state->out_of_memory = TRUE;
		return;
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
		java_app->alignment = lo_EvalAlignParam(str, &floating);
		if (floating != FALSE)
		{
			java_app->ele_attrmask |= LO_ELE_FLOATING;
		}
		PA_UNLOCK(buff);
		PA_FREE(buff);
	}

	/*
	 * Get the applet parameter.
	 */
	buff = lo_FetchParamValue(context, tag, PARAM_CODE);
	java_app->attr_code = buff;

	/*
	 * Check for the loaderbase parameter.
	 */
	buff = lo_FetchParamValue(context, tag, PARAM_CODEBASE);
	java_app->attr_codebase = buff;

	/*
	 * Check for the archive parameter.
	 */
	buff = lo_FetchParamValue(context, tag, PARAM_ARCHIVE);
	java_app->attr_archive = buff;

	/*
	 * Check for a mayscript attribute
	 */
	buff = lo_FetchParamValue(context, tag, PARAM_MAYSCRIPT);
	if (buff != NULL)
	{
		java_app->may_script = TRUE;
		PA_FREE(buff);
	}

	/*
	 * Get the name of this java applet.
	 */
	buff = lo_FetchParamValue(context, tag, PARAM_NAME);
	if (buff != NULL)
	{
		PA_LOCK(str, char *, buff);
		if (str != NULL)
		{
			int32 len;

			len = lo_StripTextWhitespace(str, XP_STRLEN(str));
		}
		PA_UNLOCK(buff);
	}
	java_app->attr_name = buff;

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
		java_app->width = val;
		PA_UNLOCK(buff);
		PA_FREE(buff);
		widthSpecified = TRUE;
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
		java_app->height = val;
		PA_UNLOCK(buff);
		PA_FREE(buff);
		heightSpecified = TRUE;
	}

	/* If they forgot to specify a width, make one up. */
	if (!widthSpecified) {
		val = 0;
		if (heightSpecified) {
			val = java_app->height;
		}
		else if (state->allow_percent_width) {
			val = state->win_width * 90 / 100;
		}
		if (val < 1) {
			val = 600;
		}
		java_app->width = val;
	}
	
	/* If they forgot to specify a height, make one up. */
	if (!heightSpecified) {
		val = 0;
		if (widthSpecified) {
			val = java_app->width;
		}
		else if (state->allow_percent_height) {
			val = state->win_height / 2;
		}
		if (val < 1) {
			val = 400;
		}
		java_app->height = val;
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
		java_app->border_width = val;
		PA_UNLOCK(buff);
		PA_FREE(buff);
	}
	java_app->border_width = FEUNITS_X(java_app->border_width, context);

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
		java_app->border_vert_space = val;
		PA_UNLOCK(buff);
		PA_FREE(buff);
	}
	java_app->border_vert_space = FEUNITS_Y(java_app->border_vert_space,
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
		java_app->border_horiz_space = val;
		PA_UNLOCK(buff);
		PA_FREE(buff);
	}
	java_app->border_horiz_space = FEUNITS_X(java_app->border_horiz_space,
						context);

	/*
	 * See if we have some saved embed/java_app session data to restore.
	 */
	if (state->top_state->savedData.EmbedList != NULL)
	{
		lo_SavedEmbedListData *embed_list;
		embed_list = state->top_state->savedData.EmbedList;

		/*
		 * If there is still valid data to restore available.
		 */
		if (java_app->embed_index < embed_list->embed_count)
		{
			lo_EmbedDataElement* embed_data_list;

			PA_LOCK(embed_data_list, lo_EmbedDataElement*,
				embed_list->embed_data_list);
			java_app->session_data =
				embed_data_list[java_app->embed_index].data;
			PA_UNLOCK(embed_list->embed_data_list);
		}
	}

	/* put the applet onto the applet list for later
	 * possible reflection */
	java_app->nextApplet = state->top_state->applet_list;
	state->top_state->applet_list = java_app;

	state->current_java = java_app;
	
	/* only wait on applets if onload flag */
	if (state->top_state->mocha_has_onload && !state->in_relayout)
	{
		state->top_state->mocha_loading_applets_count++;
	}
}

void
lo_JavaAppParam(MWContext *context, lo_DocState *state, PA_Tag *tag)
{
	LO_JavaAppStruct *java_app;
	PA_Block buff;
	char *str;
	char *param_name;
	char *param_value;

	java_app = state->current_java;
	param_name = NULL;
	param_value = NULL;

	/*
	 * Get the name of this java applet parameter.
	 */
	buff = lo_FetchParamValue(context, tag, PARAM_NAME);
	if (buff != NULL)
	{
		PA_LOCK(str, char *, buff);
		if (str != NULL)
		{
			int32 len;
			char *new_str;

			len = lo_StripTextWhitespace(str, XP_STRLEN(str));
			new_str = (char *)XP_ALLOC(len + 1);
			if (new_str != NULL)
			{
				XP_STRCPY(new_str, str);
			}
			param_name = new_str;
		}
		PA_UNLOCK(buff);
		PA_FREE(buff);
	}

	/*
	 * Get the value of this java applet parameter.
	 */
	buff = lo_FetchParamValue(context, tag, PARAM_VALUE);
	if (buff != NULL)
	{
		PA_LOCK(str, char *, buff);
		if (str != NULL)
		{
			int32 len;
			char *new_str;

			len = lo_StripTextWhitespace(str, XP_STRLEN(str));
			new_str = (char *)XP_ALLOC(len + 1);
			if (new_str != NULL)
			{
				XP_STRCPY(new_str, str);
			}
			param_value = new_str;
		}
		PA_UNLOCK(buff);
		PA_FREE(buff);
	}

	/*
	 * If the parameter has a name, add it to the name/value
	 * list.
	 */
	if (param_name != NULL)
	{
		char **name_list;
		char **value_list;

		/*
		 * If this is the first parameter, initialize the name/value
		 * list with it.
		 */
		if (java_app->param_cnt == 0)
		{
			name_list = (char **)XP_ALLOC(sizeof(char *));
			/*
			 * If we run out of memory here, free up what
			 * we can and return.
			 */
			if (name_list == NULL)
			{
				state->top_state->out_of_memory = TRUE;
				XP_FREE(param_name);
				if (param_value != NULL)
				{
					XP_FREE(param_value);
				}
				return;
			}

			value_list = (char **)XP_ALLOC(sizeof(char *));
			/*
			 * If we run out of memory here, free up what
			 * we can and return.
			 */
			if (value_list == NULL)
			{
				state->top_state->out_of_memory = TRUE;
				XP_FREE(name_list);
				XP_FREE(param_name);
				if (param_value != NULL)
				{
					XP_FREE(param_value);
				}
				return;
			}

			name_list[0] = param_name;
			value_list[0] = param_value;

			java_app->param_cnt = 1;
			java_app->param_names = name_list;
			java_app->param_values = value_list;
		}
		/*
		 * Else if we already have a name/value list we will
		 * grow it to add another.
		 */
		else
		{
			int32 cnt;

			cnt = java_app->param_cnt + 1;

			name_list = (char **)XP_REALLOC(
				java_app->param_names, (cnt * sizeof(char *)));
			/*
			 * If we run out of memory here, free up what
			 * we can and return.
			 */
			if (name_list == NULL)
			{
				state->top_state->out_of_memory = TRUE;
				XP_FREE(param_name);
				if (param_value != NULL)
				{
					XP_FREE(param_value);
				}
				return;
			}

			value_list = (char **)XP_REALLOC(
				java_app->param_values, (cnt * sizeof(char *)));
			/*
			 * If we run out of memory here, free up what
			 * we can and return.
			 * We reset param_names to the realloced memory
			 * location so we don't lose all the parameters we
			 * may already have.
			 */
			if (value_list == NULL)
			{
				state->top_state->out_of_memory = TRUE;
				java_app->param_names = name_list;
				XP_FREE(param_name);
				if (param_value != NULL)
				{
					XP_FREE(param_value);
				}
				return;
			}

			name_list[cnt - 1] = param_name;
			value_list[cnt - 1] = param_value;

			java_app->param_cnt = cnt;
			java_app->param_names = name_list;
			java_app->param_values = value_list;
		}
	}
	/*
	 * Else if no name, delete any value there might be here.
	 */
	else
	{
		if (param_value != NULL)
		{
			XP_FREE(param_value);
		}
	}
}


void
lo_FinishJavaApp(MWContext *context, lo_DocState *state,
	LO_JavaAppStruct *java_app)
{
	PA_Block buff;
	char *str;
	Bool line_break;
	int32 baseline_inc;
	int32 line_inc;
	int32 java_app_height, java_app_width;
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
	if (java_app->width == 0)
	{
		java_app->width = JAVA_DEF_DIM;
	}
	if (java_app->height == 0)
	{
		java_app->height = JAVA_DEF_DIM;
	}

	java_app_width = java_app->width + (2 * java_app->border_width) +
		(2 * java_app->border_horiz_space);
	java_app_height = java_app->height + (2 * java_app->border_width) +
		(2 * java_app->border_vert_space);

	/*
	 * SEVERE FLOW BREAK!  This may be a floating java_app,
	 * which means at this point we go do something completely
	 * different.
	 */
	if (java_app->ele_attrmask & LO_ELE_FLOATING)
	{
		if (java_app->alignment == LO_ALIGN_RIGHT)
		{
			java_app->x = state->right_margin - java_app_width;
			if (java_app->x < 0)
			{
				java_app->x = 0;
			}
		}
		else
		{
			java_app->x = state->left_margin;
		}

		java_app->y = -1;
		java_app->x_offset += java_app->border_horiz_space;
		java_app->y_offset += java_app->border_vert_space;

		lo_AddMarginStack(state, java_app->x, java_app->y,
			java_app->width, java_app->height,
			java_app->border_width,
			java_app->border_vert_space,
			java_app->border_horiz_space,
			java_app->alignment);

		/*
		 * Insert this element into the float list.
		 */
		java_app->next = state->float_list;
		state->float_list = (LO_Element *)java_app;

		/*
		 * Make sure the applet get created on
		 * the java side before we ever try to display
		 * (which the following if might do).
		 */
		if (java_app->session_data == NULL && !EDT_IS_EDITOR( context ))
		{
			/*
			 * This creates an applet in the java airspace,
			 * sends it the init message and sets up the
			 * session_data of the java_app structure.
			 */
			LJ_CreateApplet(java_app, context);
		}

		if (state->at_begin_line != FALSE)
		{
			lo_FindLineMargins(context, state);
			state->x = state->left_margin;
		}

		return;
	}

	/*
	 * Will this java_app make the line too wide.
	 */
	if ((state->x + java_app_width) > state->right_margin)
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
	 * break on the java_app if we have
	 * a break.
	 */
	if (line_break != FALSE)
	{
		/*
		 * We need to make the elements sequential, linefeed
		 * before image.
		 */
		state->top_state->element_id = java_app->ele_id;

		lo_HardLineBreak(context, state, TRUE);
		java_app->x = state->x;
		java_app->y = state->y;
		java_app->ele_id = NEXT_ELEMENT;
	}

	/*
	 * Figure out how to align this java_app.
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

	lo_CalcAlignOffsets(state, &text_info, java_app->alignment,
		java_app_width, java_app_height,
		&java_app->x_offset, &java_app->y_offset,
		&line_inc, &baseline_inc);

	java_app->x_offset += java_app->border_horiz_space;
	java_app->y_offset += java_app->border_vert_space;

	lo_AppendToLineList(context, state,
		(LO_Element *)java_app, baseline_inc);

	state->baseline += (intn) baseline_inc;
	state->line_height += (intn) (baseline_inc + line_inc);

	/*
	 * Clean up state
	 */
	state->x = state->x + java_app->x_offset +
		java_app_width - java_app->border_horiz_space;
	state->linefeed_state = 0;
	state->at_begin_line = FALSE;
	state->trailing_space = FALSE;
	state->cur_ele_type = LO_JAVA;

	if (java_app->session_data == NULL && !EDT_IS_EDITOR( context )) {
		/*
		** This creates an applet in the java airspace, sends it the init
		** message and sets up the session_data of the java_app structure:
		*/
		LJ_CreateApplet(java_app, context);
	}
}


void
lo_CloseJavaApp(MWContext *context, lo_DocState *state,
	LO_JavaAppStruct *java_app)
{
    /*
    ** Create the applet first (if there wasn't a saved one in the
    ** history).  That way we'll be able to remember reload method.
    */
	if (java_app->session_data == NULL && !EDT_IS_EDITOR( context )) {
		/*
		** This creates an applet in the java airspace, sends it the init
		** message and sets up the session_data of the java_app structure:
		*/
		LJ_CreateApplet(java_app, context);
	}

	/*
	 * Have the front end fetch this java applet, and tells us
	 * its dimentions if it knows them.
	 */
	FE_GetJavaAppSize(context, java_app, state->top_state->force_reload);

	/*
	 * We may have to block on this java applet until later
	 * when the front end can give us the width and height.
	 */
	if ((java_app->width == 0)||(java_app->height == 0))
	{
		state->top_state->layout_blocking_element =
			(LO_Element *)java_app;
	}
	else
	{
		lo_FinishJavaApp(context, state, java_app);
	}
	state->current_java = NULL;
}

