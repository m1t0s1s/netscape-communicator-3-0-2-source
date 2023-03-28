#include "xp.h"
#include "pa_parse.h"
#include "layout.h"

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


#define	DEFAULT_HR_THICKNESS	2

void
lo_HorizontalRule(MWContext *context, lo_DocState *state, PA_Tag *tag)
{
	LO_HorizRuleStruct *hrule;
	PA_Block buff;
	char *str;
	int32 val;
	int32 doc_width;

	hrule = (LO_HorizRuleStruct *)lo_NewElement(context, state, LO_HRULE, NULL, 0);
	if (hrule == NULL)
	{
		return;
	}

	hrule->type = LO_HRULE;
	hrule->ele_id = NEXT_ELEMENT;
	hrule->x = state->x;
	hrule->x_offset = 0;
	hrule->y = state->y;
	hrule->y_offset = 0;
	hrule->width = state->right_margin - state->left_margin;
	hrule->height = 0;
	hrule->next = NULL;
	hrule->prev = NULL;

	hrule->end_x = 0;
	hrule->end_y = 0;

	hrule->FE_Data = NULL;
	hrule->alignment = LO_ALIGN_CENTER;
	hrule->thickness = DEFAULT_HR_THICKNESS;

	hrule->ele_attrmask = LO_ELE_SHADED;

	hrule->sel_start = -1;
	hrule->sel_end = -1;

	buff = lo_FetchParamValue(context, tag, PARAM_ALIGN);
	if (buff != NULL)
	{
		PA_LOCK(str, char *, buff);
		if (pa_TagEqual("left", str))
		{
			hrule->alignment = LO_ALIGN_LEFT;
		}
		else if (pa_TagEqual("right", str))
		{
			hrule->alignment = LO_ALIGN_RIGHT;
		}
		PA_UNLOCK(buff);
		PA_FREE(buff);
	}

	buff = lo_FetchParamValue(context, tag, PARAM_SIZE);
	if (buff != NULL)
	{
		PA_LOCK(str, char *, buff);
		val = XP_ATOI(str);
		if (val < 1)
		{
			val = 1;
		}
		else if (val > 100)
		{
			val = 100;
		}
		hrule->thickness = (intn) val;
		PA_UNLOCK(buff);
		PA_FREE(buff);
	}
	state->line_height = FEUNITS_Y(hrule->thickness, context) +
		FEUNITS_Y(2, context);
	/*
	 * horizontal rules are always at least as tall
	 * as the default line height.
	 */
	if (state->line_height < state->default_line_height)
	{
		state->line_height = state->default_line_height;
	}
	hrule->height = FEUNITS_Y(hrule->thickness, context);

	doc_width = state->right_margin - state->left_margin;

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
				val = 1;
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
		hrule->width = val;
		PA_UNLOCK(buff);
		PA_FREE(buff);
	}
	else if (state->allow_percent_width == FALSE)
	{
		hrule->width = 1;
	}

	buff = lo_FetchParamValue(context, tag, PARAM_NOSHADE);
	if (buff != NULL)
	{
		hrule->ele_attrmask &= (~(LO_ELE_SHADED));
		PA_FREE(buff);
	}

	hrule->y_offset = (state->line_height - hrule->thickness) / 2;

	/*
	 * Alignment of HRULE now done before we ever get here.
	if (state->allow_percent_width != FALSE)
	{
		if (hrule->alignment == LO_ALIGN_CENTER)
		{
			hrule->x_offset = (doc_width - hrule->width) / 2;
		}
		else if (hrule->alignment == LO_ALIGN_RIGHT)
		{
			hrule->x_offset = doc_width - hrule->width;
		}
	}
	if (hrule->x_offset < 0)
	{
		hrule->x_offset = state->left_margin;
	}
	 */


	lo_AppendToLineList(context, state, (LO_Element *)hrule, 0);

	state->x = state->x + hrule->x_offset + hrule->width;
	state->linefeed_state = 0;
	state->at_begin_line = FALSE;
	state->cur_ele_type = LO_HRULE;
}

#ifdef PROFILE
#pragma profile off
#endif
