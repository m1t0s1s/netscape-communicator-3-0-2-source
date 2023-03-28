#include "xp.h"
#include "pa_parse.h"
#include "layout.h"

#ifdef PROFILE
#pragma profile on
#endif

lo_ListStack *
lo_DefaultList(lo_DocState *state)
{
	lo_ListStack *lptr;

	lptr = XP_NEW(lo_ListStack);
	if (lptr == NULL)
	{
		return(NULL);
	}

	lptr->type = P_UNKNOWN;
	lptr->level = 0;
	lptr->compact = FALSE;
	lptr->bullet_type = BULLET_BASIC;
	lptr->old_left_margin = state->win_left;
	lptr->old_right_margin = state->win_width - state->win_right;
	lptr->next = NULL;

	return(lptr);
}


void
lo_PushList(lo_DocState *state, PA_Tag *tag)
{
	lo_ListStack *lptr;
	intn bullet_type;
	int32 val;
	Bool no_level;

	val = 1;
	no_level = FALSE;
	switch (tag->type)
	{
		/*
		 * Blockquotes and multicolumns pretend to be the current
		 * list type, unless the current list is nothing.
		 */
		case P_MULTICOLUMN:
		case P_BLOCKQUOTE:
			bullet_type = state->list_stack->bullet_type;
			no_level = TRUE;
			if (state->list_stack->type != P_UNKNOWN)
			{
				tag->type = state->list_stack->type;
			}
			val = state->list_stack->value;
			break;
		case P_NUM_LIST:
			bullet_type = BULLET_NUM;
			break;
		case P_UNUM_LIST:
		case P_MENU:
		case P_DIRECTORY:
			bullet_type = BULLET_BASIC;
			break;
		default:
			bullet_type = BULLET_NONE;
			break;
	}

	lptr = XP_NEW(lo_ListStack);
	if (lptr == NULL)
	{
		state->top_state->out_of_memory = TRUE;
		return;
	}

	lptr->type = tag->type;
	if (no_level != FALSE)
	{
		lptr->level = state->list_stack->level;
	}
	else
	{
		lptr->level = state->list_stack->level + 1;
	}
	lptr->compact = FALSE;
	lptr->value = val;
	lptr->bullet_type = bullet_type;
	lptr->old_left_margin = state->left_margin;
	lptr->old_right_margin = state->right_margin;

	lptr->next = state->list_stack;
	state->list_stack = lptr;
}


lo_ListStack *
lo_PopList(lo_DocState *state, PA_Tag *tag)
{
	lo_ListStack *lptr;

	lptr = state->list_stack;
	if ((lptr->type == P_UNKNOWN)||(lptr->next == NULL))
	{
		return(NULL);
	}

	state->list_stack = lptr->next;
	lptr->next = NULL;
	return(lptr);
}

#ifdef PROFILE
#pragma profile off
#endif
