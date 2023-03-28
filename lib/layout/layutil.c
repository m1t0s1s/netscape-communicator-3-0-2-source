/* -*- Mode: C; tab-width: 4; tabs-indent-mode: t -*- */

#include "xp.h"
#include "pa_parse.h"
#include "layout.h"
#ifdef MOCHA
#include "libmocha.h"
#endif /* MOCHA */
#ifdef EDITOR
#include "edt.h"
#endif
#ifdef PROFILE
#pragma profile on
#endif

#ifdef XP_WIN16
#define SIZE_LIMIT              32000
#endif /* XP_WIN16 */

#define	FONT_FACE_INC	10
#define	FONT_FACE_MAX	1000


void
LO_InvalidateFontData(MWContext *context)
{
	int32 i, doc_id;
	lo_TopState *top_state;
	lo_DocState *state;
	LO_TextAttr **text_attr_hash;
	LO_TextAttr *attr_ptr;

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

	if (state->top_state->text_attr_hash == NULL)
	{
		return;
	}

	XP_LOCK_BLOCK(text_attr_hash, LO_TextAttr **,
		state->top_state->text_attr_hash);
	for (i=0; i < FONT_HASH_SIZE; i++)
	{
		attr_ptr = text_attr_hash[i];
		while (attr_ptr != NULL)
		{
			attr_ptr->FE_Data = NULL;
			attr_ptr = attr_ptr->next;
		}
	}
}


static char *
lo_find_face_in_array(char **face_list, intn len, char *face)
{
	intn i;
	char *the_face;

	if (face == NULL)
	{
		return(NULL);
	}

	the_face = NULL;
	for (i=0; i<len; i++)
	{
		if ((face_list[i] != NULL)&&
			(XP_STRCMP(face, face_list[i]) == 0))
		{
			the_face = face_list[i];
			break;
		}
	}

	return(the_face);
}


char *
lo_FetchFontFace(MWContext *context, lo_DocState *state, char *face)
{
	char **face_list;
	char *new_face;

	/*
	 * If this is our first addition, allocate the array.
	 */
	if (state->top_state->font_face_array == NULL)
	{
		PA_Block buff;

		buff = PA_ALLOC(FONT_FACE_INC * sizeof(char *));
		if (buff == NULL)
		{
			state->top_state->out_of_memory = TRUE;
			return(NULL);
		}
		state->top_state->font_face_array = buff;
		state->top_state->font_face_array_size = FONT_FACE_INC;
		state->top_state->font_face_array_len = 0;
	}
	/*
	 * Else if the list is full, grow the array.
	 */
	else if (state->top_state->font_face_array_len >=
			state->top_state->font_face_array_size)
	{
		PA_Block buff;
		intn new_size;

		if (state->top_state->font_face_array_size >= FONT_FACE_MAX)
		{
			return(NULL);
		}

		new_size = state->top_state->font_face_array_size +
				FONT_FACE_INC;
		if (new_size > FONT_FACE_MAX)
		{
			new_size = FONT_FACE_MAX;
		}
		buff = XP_REALLOC_BLOCK(state->top_state->font_face_array,
			(new_size * sizeof(char *)));
		if (buff == NULL)
		{
			state->top_state->out_of_memory = TRUE;
			return(NULL);
		}
		state->top_state->font_face_array = buff;
		state->top_state->font_face_array_size = new_size;
	}

	PA_LOCK(face_list, char **, state->top_state->font_face_array);
	new_face = lo_find_face_in_array(face_list,
		state->top_state->font_face_array_len, face);
	if (new_face == NULL)
	{
		char *str;

		str = XP_STRDUP(face);
		if (str == NULL)
		{
			PA_UNLOCK(state->top_state->font_face_array);
			state->top_state->out_of_memory = TRUE;
			return(NULL);
		}
		face_list[state->top_state->font_face_array_len] = str;
		state->top_state->font_face_array_len++;
		new_face = str;
	}
	PA_UNLOCK(state->top_state->font_face_array);
	return(new_face);
}


Bool
lo_IsValidTarget(char *target)
{
	if (target == NULL)
	{
		return(FALSE);
	}
	if ((XP_IS_ALPHA(target[0]) != FALSE)||
	    (XP_IS_DIGIT(target[0]) != FALSE)||
	    (target[0] == '_'))
	{
		return(TRUE);
	}
	return(FALSE);
}


int32
lo_StripTextNewlines(char *text, int32 text_len)
{
	char *from_ptr;
	char *to_ptr;
	int32 len;

	if ((text == NULL)||(text_len < 1))
	{
		return(0);
	}

	/*
	 * remove all non-space whitespace from the middle of the string.
	 */
	from_ptr = text;
	len = text_len;
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


PA_Block
lo_FEToNetLinebreaks(PA_Block text_block)
{
	PA_Block new_block;
	char *new_text;
	char *text;
	char *from_ptr;
	char *to_ptr;
	char break_char;
	int32 len, new_len;
	int32 linebreak;

	if (text_block == NULL)
	{
		return(NULL);
	}
	PA_LOCK(text, char *, text_block);

	XP_BCOPY(LINEBREAK, (char *)(&break_char), 1);
	len = 0;
	linebreak = 0;
	from_ptr = text;
	while (*from_ptr != '\0')
	{
		if (*from_ptr == break_char)
		{
			if (strncmp(from_ptr, LINEBREAK, LINEBREAK_LEN) == 0)
			{
				linebreak++;
				len += LINEBREAK_LEN;
				from_ptr = (char *)(from_ptr + LINEBREAK_LEN);
			}
			else
			{
				len++;
				from_ptr++;
			}
		}
		else
		{
			len++;
			from_ptr++;
		}
	}

	new_len = len - (LINEBREAK_LEN * linebreak) + (2 * linebreak);
	new_block = PA_ALLOC(new_len + 1);
	if (new_block == NULL)
	{
		PA_UNLOCK(text_block);
		return(NULL);
	}
	PA_LOCK(new_text, char *, new_block);
	from_ptr = text;
	to_ptr = new_text;
	while (*from_ptr != '\0')
	{
		if (*from_ptr == break_char)
		{
			if (strncmp(from_ptr, LINEBREAK, LINEBREAK_LEN) == 0)
			{
				from_ptr = (char *)(from_ptr + LINEBREAK_LEN);
				*to_ptr++ = CR;
				*to_ptr++ = LF;
			}
			else
			{
				*to_ptr++ = *from_ptr++;
			}
		}
		else
		{
			*to_ptr++ = *from_ptr++;
		}
	}
	*to_ptr = '\0';

	new_text[new_len] = '\0';
	PA_UNLOCK(new_block);
	PA_UNLOCK(text_block);
	return(new_block);
}


PA_Block
lo_ConvertToFELinebreaks(char *text, int32 text_len, int32 *new_len_ptr)
{
	PA_Block new_block;
	char *new_text;
	char *from_ptr;
	char *to_ptr;
	char break_char;
	int32 len, new_len;
	int32 skip, linebreak;

	if (text == NULL)
	{
		return(NULL);
	}

	break_char = '\0';
	len = 0;
	skip = 0;
	linebreak = 0;
	from_ptr = text;
	to_ptr = text;
	while (len < text_len)
	{
		if ((*from_ptr == CR)||(*from_ptr == LF))
		{
			if ((break_char != '\0')&&(break_char != *from_ptr))
			{
				skip++;
				break_char = '\0';
			}
			else
			{
				skip++;
				linebreak++;
				break_char = *from_ptr;
			}
		}
		from_ptr++;
		len++;
	}

	new_len = len - skip + (LINEBREAK_LEN * linebreak);
	new_block = PA_ALLOC(new_len + 1);
	if (new_block == NULL)
	{
		return(NULL);
	}
	PA_LOCK(new_text, char *, new_block);
	break_char = '\0';
	len = 0;
	from_ptr = text;
	to_ptr = new_text;
	while (len < text_len)
	{
		if ((*from_ptr == CR)||(*from_ptr == LF))
		{
			if ((break_char != '\0')&&(break_char != *from_ptr))
			{
				break_char = '\0';
			}
			else
			{
				XP_BCOPY(LINEBREAK, to_ptr, LINEBREAK_LEN);
				to_ptr = (char *)(to_ptr + LINEBREAK_LEN);
				break_char = *from_ptr;
			}
			from_ptr++;
			len++;
		}
		else
		{
			*to_ptr++ = *from_ptr++;
			len++;
		}
	}

	new_text[new_len] = '\0';
	*new_len_ptr = new_len;
	PA_UNLOCK(new_block);
	return(new_block);
}


intn
lo_EvalVAlignParam(char *str)
{
	intn alignment;

	alignment = LO_ALIGN_TOP;
	if (pa_TagEqual("top", str))
	{
		alignment = LO_ALIGN_TOP;
	}
	else if (pa_TagEqual("bottom", str))
	{
		alignment = LO_ALIGN_BOTTOM;
	}
	else if ((pa_TagEqual("middle", str))||
		(pa_TagEqual("center", str)))
	{
		alignment = LO_ALIGN_CENTER;
	}
	else if (pa_TagEqual("baseline", str))
	{
		alignment = LO_ALIGN_BASELINE;
	}
	return(alignment);
}


intn
lo_EvalDivisionAlignParam(char *str)
{
	intn alignment;

	alignment = LO_ALIGN_LEFT;
	if (pa_TagEqual("left", str))
	{
		alignment = LO_ALIGN_LEFT;
	}
	else if (pa_TagEqual("right", str))
	{
		alignment = LO_ALIGN_RIGHT;
	}
	else if (pa_TagEqual("center", str))
	{
		alignment = LO_ALIGN_CENTER;
	}
	return(alignment);
}


intn
lo_EvalCellAlignParam(char *str)
{
	intn alignment;

	alignment = LO_ALIGN_LEFT;
	if (pa_TagEqual("left", str))
	{
		alignment = LO_ALIGN_LEFT;
	}
	else if (pa_TagEqual("right", str))
	{
		alignment = LO_ALIGN_RIGHT;
	}
	else if ((pa_TagEqual("middle", str))||
		(pa_TagEqual("center", str)))
	{
		alignment = LO_ALIGN_CENTER;
	}
	return(alignment);
}

#ifdef EDITOR
/* this is a replacement routine for the non-editor version and should eventually
 *	replace the non-edit version.  It uses an array of values. instead of an 
 *	unrolled loop.  
*/

/*
 * Indexed by LO_ALIGN_ETC
*/
char* lo_alignStrings[] = {
    "abscenter",		/* LO_ALIGN_CENTER */
    "left",				/* LO_ALIGN_LEFT */
    "right",			/* LO_ALIGN_RIGHT */
    "texttop",			/* LO_ALIGN_TOP */
    "absbottom",		/* LO_ALIGN_BOTTOM */
    "baseline",			/* LO_ALIGN_BASELINE */
    "center",			/* LO_ALIGN_NCSA_CENTER */
    "bottom",			/* LO_ALIGN_NCSA_BOTTOM */
    "top",				/* LO_ALIGN_NCSA */
	0
};

intn
lo_EvalAlignParam(char *str, Bool *floating)
{
	intn alignment;

	*floating = FALSE;
	alignment = LO_ALIGN_BASELINE;
	if (pa_TagEqual("middle", str))
	{
		alignment = LO_ALIGN_NCSA_CENTER;
	}
	else if (pa_TagEqual("absmiddle", str))
	{
		alignment = LO_ALIGN_CENTER;
	}
	else 
	{
		intn i = 0;
		Bool bFound = FALSE;

		while( lo_alignStrings[i] != NULL  && bFound == FALSE )
		{
			if( pa_TagEqual( lo_alignStrings[i], str) )
			{
				bFound = TRUE;
				alignment = i;
			}
			i++;
		}	 
	}
	if( alignment == LO_ALIGN_LEFT || alignment == LO_ALIGN_RIGHT)
	{
		*floating = TRUE;
	}
	return alignment;
} 

#else

intn
lo_EvalAlignParam(char *str, Bool *floating)
{
	intn alignment;

	*floating = FALSE;
	alignment = LO_ALIGN_BASELINE;
	if (pa_TagEqual("top", str))
	{
		alignment = LO_ALIGN_NCSA_TOP;
	}
	else if (pa_TagEqual("texttop", str))
	{
		alignment = LO_ALIGN_TOP;
	}
	else if (pa_TagEqual("bottom", str))
	{
		alignment = LO_ALIGN_NCSA_BOTTOM;
	}
	else if (pa_TagEqual("absbottom", str))
	{
		alignment = LO_ALIGN_BOTTOM;
	}
	else if (pa_TagEqual("baseline", str))
	{
		alignment = LO_ALIGN_BASELINE;
	}
	else if ((pa_TagEqual("middle", str))||
		(pa_TagEqual("center", str)))
	{
		alignment = LO_ALIGN_NCSA_CENTER;
	}
	else if ((pa_TagEqual("absmiddle", str))||
		(pa_TagEqual("abscenter", str)))
	{
		alignment = LO_ALIGN_CENTER;
	}
	else if (pa_TagEqual("left", str))
	{
		alignment = LO_ALIGN_LEFT;
		*floating = TRUE;
	}
	else if (pa_TagEqual("right", str))
	{
		alignment = LO_ALIGN_RIGHT;
		*floating = TRUE;
	}
	return(alignment);
}

#endif

void
lo_CalcAlignOffsets(lo_DocState *state, LO_TextInfo *text_info, intn alignment,
	int32 width, int32 height, int16 *x_offset, int32 *y_offset,
	int32 *line_inc, int32 *baseline_inc)
{
	*line_inc = 0;
	*baseline_inc = 0;
	switch (alignment)
	{
		case LO_ALIGN_CENTER:
			*x_offset = 0;
			/*
			 * Center after current text.
			 */
			/*
			 * If the image is shorter than the height
			 * of this line, just increase y_offset
			 * to get it centered.
			 */
			if (height < state->line_height)
			{
				*y_offset += ((state->line_height - height)
						/ 2);
			}
			/*
			 * Else for images taller than the line
			 * we will need to move all their
			 * baselines down, plus increase the rest
			 * of their lineheights to reflect the
			 * big centered image.
			 */
			else
			{
				*y_offset = 0;
				*baseline_inc = (height - state->line_height)
						/ 2;
				*line_inc = height - state->line_height -
					*baseline_inc;
			}
			break;
		case LO_ALIGN_NCSA_CENTER:
			*x_offset = 0;
			/*
			 * Center on baseline of current text.
			 */
			/*
			 * If the image centered on the baseline will not
			 * extend below or above the current line,
			 * just increase y_offset to get it centered.
			 */
			if (((height / 2) <= state->baseline)&&
				((height / 2) <= (state->line_height - 
					state->baseline)))
			{
				*y_offset = state->baseline -
					(height / 2);
			}
			/*
			 * Else the image extends over the bottom, but
			 * not off the top.
			 */
			else if ((height / 2) <= state->baseline)
			{
				*y_offset = state->baseline -
					(height / 2);
				*line_inc = *y_offset + height -
					state->line_height;
			}
			/*
			 * Else for images taller than the line
			 * we will need to move all their
			 * baselines down, plus and maybe increase the rest
			 * of their lineheights to reflect the
			 * big centered image.
			 */
			else
			{
				*y_offset = 0;
				*baseline_inc = (height / 2) - state->baseline;
				if ((state->baseline + (height / 2)) >
					state->line_height)
				{
					*line_inc = state->baseline +
						(height / 2) -
						state->line_height;
				}
			}
			break;
		case LO_ALIGN_TOP:
			*x_offset = 0;
			/*
			 * Move the top of the image to the top of the
			 * last text placed.  On negative offsets
			 * (such as at the start of a line) just
			 * start the image at the top.
			 */
			*y_offset = state->baseline - text_info->ascent;
			if (*y_offset < 0)
			{
				*y_offset = 0;
			}

			/*
			 * Top aligned images can easily hang down and make
			 * all the rest of the line taller.  Set line_inc
			 * to properly increase the height of all previous
			 * elements.
			 */
			if ((*y_offset + height) > state->line_height)
			{
				*line_inc = *y_offset + height -
					state->line_height;
			}
			break;
		case LO_ALIGN_NCSA_TOP:
			*x_offset = 0;
			/*
			 * Move the top of the image to the top of the
			 * line.
			 */
			*y_offset = 0;

			/*
			 * Top aligned images can easily hang down and make
			 * all the rest of the line taller.  Set line_inc
			 * to properly increase the height of all previous
			 * elements.
			 */
			if ((*y_offset + height) > state->line_height)
			{
				*line_inc = *y_offset + height -
					state->line_height;
			}
			break;
		case LO_ALIGN_BOTTOM:
			*x_offset = 0;
			/*
			 * Like centered images, images shorter than
			 * the line just get move with y_offset.
			 */
			if (height < state->line_height)
			{
				*y_offset = state->line_height - height;
			}
			/*
			 * Else since they are bottom aligned.
			 * they move everyone's baselines and can't
			 * change the height below the baseline.
			 */
			else
			{
				*y_offset = 0;
				*baseline_inc = height - state->line_height;
			}
			break;
		case LO_ALIGN_NCSA_BOTTOM:
		case LO_ALIGN_BASELINE:
		default:
			*x_offset = 0;
			/*
			 * Like centered images, images shorter than
			 * the baseline just get move with y_offset.
			 */
			if (height < state->baseline)
			{
				*y_offset = state->baseline - height;
			}
			/*
			 * Else since they are baseline aligned.
			 * they move everyone's baselines and can't
			 * change the height below the baseline.
			 */
			else
			{
				*y_offset = 0;
				*baseline_inc = height - state->baseline;
			}
			break;
	}
}


int32
lo_ValueOrPercent(char *str, Bool *is_percent)
{
	int32 val;
	char *tptr;

	*is_percent = FALSE;

	if (str == NULL)
	{
		return(0);
	}

	tptr = str;
	while (*tptr != '\0')
	{
		if (*tptr == '%')
		{
			*is_percent = TRUE;
			*tptr = '\0';
			break;
		}
		tptr++;
	}
	val = XP_ATOI(str);

	return(val);
}


void
lo_CopyTextAttr(LO_TextAttr *old_attr, LO_TextAttr *new_attr)
{
	if ((old_attr == NULL)||(new_attr == NULL))
	{
		return;
	}

	new_attr->size = old_attr->size;
	new_attr->fontmask = old_attr->fontmask;

	new_attr->fg.red = old_attr->fg.red;
	new_attr->fg.green = old_attr->fg.green;
	new_attr->fg.blue = old_attr->fg.blue;

	new_attr->bg.red = old_attr->bg.red;
	new_attr->bg.green = old_attr->bg.green;
	new_attr->bg.blue = old_attr->bg.blue;

	new_attr->no_background = old_attr->no_background;
	new_attr->attrmask = old_attr->attrmask;
	new_attr->font_face = old_attr->font_face;
	/*
	 * FE_Data is not copied, it belongs to the FE,
	 * And will need to be generated by the FE the first time this
	 * new TextAttr is used.
	 */
	new_attr->FE_Data = NULL;

	new_attr->charset = old_attr->charset;
}


LO_TextAttr *
lo_FetchTextAttr(lo_DocState *state, LO_TextAttr *old_attr)
{
	LO_TextAttr **text_attr_hash;
	LO_TextAttr *attr_ptr;
	int32 hash_index;

	if (old_attr == NULL)
	{
		return(NULL);
	}

	hash_index = (old_attr->fontmask & old_attr->attrmask) % FONT_HASH_SIZE;

	XP_LOCK_BLOCK(text_attr_hash, LO_TextAttr **,
		state->top_state->text_attr_hash);
	attr_ptr = text_attr_hash[hash_index];

	while (attr_ptr != NULL)
	{
		if ((attr_ptr->size == old_attr->size)&&
			(attr_ptr->fontmask == old_attr->fontmask)&&
			(attr_ptr->attrmask == old_attr->attrmask)&&
			(attr_ptr->fg.red == old_attr->fg.red)&&
			(attr_ptr->fg.green == old_attr->fg.green)&&
			(attr_ptr->fg.blue == old_attr->fg.blue)&&
			(attr_ptr->bg.red == old_attr->bg.red)&&
			(attr_ptr->bg.green == old_attr->bg.green)&&
			(attr_ptr->bg.blue == old_attr->bg.blue)&&
			(attr_ptr->font_face == old_attr->font_face)&&
			(attr_ptr->charset == old_attr->charset))
		{
			break;
		}
		attr_ptr = attr_ptr->next;
	}
	if (attr_ptr == NULL)
	{
		LO_TextAttr *new_attr;

		new_attr = XP_NEW(LO_TextAttr);
		if (new_attr != NULL)
		{
			lo_CopyTextAttr(old_attr, new_attr);
			new_attr->next = text_attr_hash[hash_index];
			text_attr_hash[hash_index] = new_attr;
		}
		else
		{
			state->top_state->out_of_memory = TRUE;
		}
		attr_ptr = new_attr;
	}

	XP_UNLOCK_BLOCK(state->top_state->text_attr_hash);
	return(attr_ptr);
}


void
lo_AddMarginStack(lo_DocState *state, int32 x, int32 y,
		int32 width, int32 height,
		int32 border_width,
		int32 border_vert_space, int32 border_horiz_space,
		intn alignment)
{
	lo_MarginStack *mptr;

	mptr = XP_NEW(lo_MarginStack);
	if (mptr == NULL)
	{
		state->top_state->out_of_memory = TRUE;
		return;
	}

	mptr->y_min = y;
	if (y >= 0)
	{
		mptr->y_max = y + height + (2 * border_width) +
			(2 * border_vert_space);
	}
	else
	{
		mptr->y_max = height + (2 * border_width) +
			(2 * border_vert_space);
	}

	if (alignment == LO_ALIGN_RIGHT)
	{
		if (state->right_margin_stack == NULL)
		{
			mptr->margin = state->right_margin - width -
				(2 * border_width) - (2 * border_horiz_space);
		}
		else
		{
			mptr->margin = state->right_margin_stack->margin -
				width - (2 * border_width) -
				(2 * border_horiz_space);
		}
		mptr->next = state->right_margin_stack;
		state->right_margin_stack = mptr;
	}
	else
	{
		mptr->margin = state->left_margin + width +
			(2 * border_width) + (2 * border_horiz_space);
		mptr->next = state->left_margin_stack;
		state->left_margin_stack = mptr;
	}
}


/*
 * When clipping lines we have essentially scrolled the document up.
 * Move the margin blocks up too.
 */
void
lo_ShiftMarginsUp(MWContext *context, lo_DocState *state, int32 dy)
{
	lo_MarginStack *mptr;
	int32 y;

	mptr = state->left_margin_stack;
	while (mptr != NULL)
	{
		y = mptr->y_min - dy;
		if (y < 0)
		{
			y = 0;
		}
		mptr->y_min = y;

		y = mptr->y_max - dy;
		if (y < 0)
		{
			y = 0;
		}
		mptr->y_max = y;
		mptr = mptr->next;
	}

	mptr = state->right_margin_stack;
	while (mptr != NULL)
	{
		y = mptr->y_min - dy;
		if (y < 0)
		{
			y = 0;
		}
		mptr->y_min = y;

		y = mptr->y_max - dy;
		if (y < 0)
		{
			y = 0;
		}
		mptr->y_max = y;
		mptr = mptr->next;
	}
}


void
lo_ClearToLeftMargin(MWContext *context, lo_DocState *state)
{
	lo_MarginStack *mptr;
	int32 y;

	mptr = state->left_margin_stack;
	if (mptr == NULL)
	{
		return;
	}
	y = state->y;

	if (mptr->y_max > y)
	{
		y = mptr->y_max;
	}
	while (mptr->next != NULL)
	{
		lo_MarginStack *margin;

		if (mptr->y_max > y)
		{
			y = mptr->y_max;
		}
		margin = mptr;
		mptr = mptr->next;
		XP_DELETE(margin);
	}
	XP_DELETE(mptr);
	state->y = y;
	state->left_margin_stack = NULL;
	state->left_margin = state->win_left;
	state->x = state->left_margin;

	/*
	 * Find where we hit the right margin, popping old
	 * obsolete elements as we go.
	 */
	mptr = state->right_margin_stack;
	while ((mptr != NULL)&&(state->y > mptr->y_max))
	{
		lo_MarginStack *margin;

		margin = mptr;
		mptr = mptr->next;
		XP_DELETE(margin);
	}
	if (mptr != NULL)
	{
		state->right_margin_stack = mptr;
		state->right_margin = mptr->margin;
	}
	else
	{
		state->right_margin_stack = NULL;
		state->right_margin = (state->list_stack != NULL)
							? state->list_stack->old_right_margin
							: 0;
	}
}


void
lo_ClearToRightMargin(MWContext *context, lo_DocState *state)
{
	lo_MarginStack *mptr;
	int32 y;

	mptr = state->right_margin_stack;
	if (mptr == NULL)
	{
		return;
	}
	y = state->y;

	if (mptr->y_max > y)
	{
		y = mptr->y_max;
	}
	while (mptr->next != NULL)
	{
		lo_MarginStack *margin;

		if (mptr->y_max > y)
		{
			y = mptr->y_max;
		}
		margin = mptr;
		mptr = mptr->next;
		XP_DELETE(margin);
	}
	XP_DELETE(mptr);
	state->y = y;
	state->right_margin_stack = NULL;
	state->right_margin = state->win_width - state->win_right;

	/*
	 * Find where we hit the left margin, popping old
	 * obsolete elements as we go.
	 */
	mptr = state->left_margin_stack;
	while ((mptr != NULL)&&(state->y > mptr->y_max))
	{
		lo_MarginStack *margin;

		margin = mptr;
		mptr = mptr->next;
		XP_DELETE(margin);
	}
	if (mptr != NULL)
	{
		state->left_margin_stack = mptr;
		state->left_margin = mptr->margin;
	}
	else
	{
		state->left_margin_stack = NULL;
		state->left_margin = (state->list_stack != NULL)
						   ? state->list_stack->old_left_margin
						   : 0;
	}
}


void
lo_ClearToBothMargins(MWContext *context, lo_DocState *state)
{
	lo_MarginStack *mptr;
	int32 y;

	y = state->y;

	mptr = state->right_margin_stack;
	if (mptr != NULL)
	{
		if (mptr->y_max > y)
		{
			y = mptr->y_max;
		}
		while (mptr->next != NULL)
		{
			lo_MarginStack *margin;

			if (mptr->y_max > y)
			{
				y = mptr->y_max;
			}
			margin = mptr;
			mptr = mptr->next;
			XP_DELETE(margin);
		}
		XP_DELETE(mptr);
		state->y = y;
		state->right_margin_stack = NULL;

		state->right_margin = state->win_width - state->win_right;
	}

	mptr = state->left_margin_stack;
	if (mptr != NULL)
	{
		if (mptr->y_max > y)
		{
			y = mptr->y_max;
		}
		while (mptr->next != NULL)
		{
			lo_MarginStack *margin;

			if (mptr->y_max > y)
			{
				y = mptr->y_max;
			}
			margin = mptr;
			mptr = mptr->next;
			XP_DELETE(margin);
		}
		XP_DELETE(mptr);
		if (y > state->y)
		{
			state->y = y;
		}
		state->left_margin_stack = NULL;

		state->left_margin = state->win_left;
		state->x = state->left_margin;
	}
}


void
lo_FindLineMargins(MWContext *context, lo_DocState *state)
{
	lo_MarginStack *mptr;
	LO_Element *eptr;

	eptr = state->float_list;
	while ((eptr != NULL)&&(eptr->lo_any.y < 0))
	{
		int32 expose_y;
		int32 expose_height;

		/*
		 * Float the y to our current position.
		 */
		eptr->lo_any.y = state->y;

		/*
		 * These get coppied into special variables because they
		 * may get changed by a table in the floating list.
		 */
		expose_y = eptr->lo_any.y;
		expose_height = eptr->lo_any.height;

		/*
		 * Special case:  If we have just placed a TABLE element,
		 * we need to properly offset all the CELLs inside it.
		 */
		if (eptr->type == LO_TABLE)
		{
			int32 y2;
			int32 change_y;
			LO_Element *tptr;

			/*
			 * We need to know the bottom and top extents of
			 * the table to know big an area to refresh.
			 */
			y2 = expose_y + eptr->lo_any.y_offset + expose_height;

			/*
			 * This is how far to move the cells based on
			 * where the table thought it was placed.
			 */
			change_y = state->y - eptr->lo_table.expected_y;

			/*
			 * set tptr to the start of the cell list.
			 */
			tptr = eptr->lo_any.next;
			while ((tptr != NULL)&&(tptr->type == LO_CELL))
			{
				/*
				 * Only do this work if the table has moved.
				 */
				if (change_y != 0)
				{
					tptr->lo_any.y += change_y;
					lo_ShiftCell((LO_CellStruct *)tptr,
						0, change_y);
				}

				/*
				 * Find upper and lower bounds of the
				 * cells.
				 */
				if (tptr->lo_any.y < expose_y)
				{
					expose_y = tptr->lo_any.y;
				}
				if ((tptr->lo_any.y + tptr->lo_any.y_offset +
					tptr->lo_any.height) > y2)
				{
					y2 = tptr->lo_any.y +
						tptr->lo_any.y_offset +
						tptr->lo_any.height;
				}

				tptr = tptr->lo_any.next;
			}

			/*
			 * Adjust exposed area height for lower bound.
			 * Upper bound already possibly moved.
			 */
			if ((y2 - expose_y - eptr->lo_any.y_offset) >
				expose_height)
			{
				expose_height = y2 - expose_y -
					eptr->lo_any.y_offset;
			}
		}

		/*
		 * We may have been blocking display on this floating image.
		 * If so deal with it here.
		 */
		if ((state->display_blocked != FALSE)&&
		    (state->is_a_subdoc == SUBDOC_NOT)&&
		    (state->display_blocking_element_y == 0)&&
		    (state->display_blocking_element_id != -1)&&
		    (eptr->lo_any.ele_id >=
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
				lo_RefreshDocumentArea(context, state, 0, 0,
					state->win_width, state->win_height);
			}
		}

		lo_RefreshDocumentArea(context, state,
			eptr->lo_any.x + eptr->lo_any.x_offset,
			expose_y + eptr->lo_any.y_offset,
			eptr->lo_any.width, expose_height);

		/*
		 * In case these floating elements are the only thing in the
		 * doc (or subdoc) we need to set the state min and max widths
		 * when we place them.
		 */
		if (eptr->type == LO_TABLE)
		{
			int32 width;

			width = eptr->lo_table.width +
				(2 * eptr->lo_table.border_width) +
				(2 * eptr->lo_table.border_horiz_space);

			if (width > state->min_width)
			{
				state->min_width = width;
			}
			if (width > state->max_width)
			{
				state->max_width = width;
			}
		}
		else if (eptr->type == LO_IMAGE)
		{
			int32 width;

			width = eptr->lo_image.width +
				(2 * eptr->lo_image.border_width) +
				(2 * eptr->lo_image.border_horiz_space);

			if (width > state->min_width)
			{
				state->min_width = width;
			}
			if (width > state->max_width)
			{
				state->max_width = width;
			}
		}

		eptr = eptr->lo_any.next;
	}

	/*
	 * Find floating elements just added to the left margin stack
	 * from the last line, and set their absolute
	 * coordinates.
	 */
	mptr = state->left_margin_stack;
	while ((mptr != NULL)&&(mptr->y_min < 0))
	{
		mptr->y_min = state->y;
		mptr->y_max = mptr->y_min + mptr->y_max;
		mptr = mptr->next;
	}

	/*
	 * Find where we hit the left margin, popping old
	 * obsolete elements as we go.
	 */
	mptr = state->left_margin_stack;
	while ((mptr != NULL)&&(state->y > mptr->y_max))
	{
		lo_MarginStack *margin;

		margin = mptr;
		mptr = mptr->next;
		XP_DELETE(margin);
	}
	if (mptr != NULL)
	{
		state->left_margin_stack = mptr;
		state->left_margin = mptr->margin;
		/*
		 * For a list wrapped around a floating image we may
		 * need to be indented from the image edge.
		 */
		if (state->list_stack->old_left_margin > mptr->margin)
		{
			state->left_margin = state->list_stack->old_left_margin;
		}
	}
	else
	{
		state->left_margin_stack = NULL;
		state->left_margin = (state->list_stack != NULL)
						   ? state->list_stack->old_left_margin
						   : 0;
	}

	/*
	 * Find floating elements just added to the right margin stack
	 * from the last line, and set their absolute
	 * coordinates.
	 */
	mptr = state->right_margin_stack;
	while ((mptr != NULL)&&(mptr->y_min < 0))
	{
		mptr->y_min = state->y;
		mptr->y_max = mptr->y_min + mptr->y_max;
		mptr = mptr->next;
	}

	/*
	 * Find where we hit the right margin, popping old
	 * obsolete elements as we go.
	 */
	mptr = state->right_margin_stack;
	while ((mptr != NULL)&&(state->y > mptr->y_max))
	{
		lo_MarginStack *margin;

		margin = mptr;
		mptr = mptr->next;
		XP_DELETE(margin);
	}
	if (mptr != NULL)
	{
		state->right_margin_stack = mptr;
		state->right_margin = mptr->margin;
	}
	else
	{
		state->right_margin_stack = NULL;
		state->right_margin = (state->list_stack != NULL)
							? state->list_stack->old_right_margin
							: 0;
	}
}

void
lo_RecycleElements(MWContext *context, lo_DocState *state, LO_Element *elements)
{
	LO_Element *eptr;

	if (elements == NULL)
	{
		return;
	}

	eptr = elements;
	while(eptr->lo_any.next != NULL)
	{
		lo_ScrapeElement(context, eptr);
		eptr = eptr->lo_any.next;
	}
	lo_ScrapeElement(context, eptr);
#ifdef MEMORY_ARENAS
    if ( EDT_IS_EDITOR(context) ) {
	    eptr->lo_any.next = state->top_state->recycle_list;
	    state->top_state->recycle_list = elements;
    }
#else
	eptr->lo_any.next = state->top_state->recycle_list;
	state->top_state->recycle_list = elements;
#endif /* MEMORY_ARENAS */
}


LO_Element *
lo_NewElement(MWContext *context, lo_DocState *state, intn type, 
		ED_Element *edit_element, intn edit_offset)
{
	LO_Element *eptr;
	int32 size;

#ifdef MEMORY_ARENAS
	switch(type)
	{
		case LO_TEXT:
			size = sizeof(LO_TextStruct);
			break;
		case LO_LINEFEED:
			size = sizeof(LO_LinefeedStruct);
			break;
		case LO_HRULE:
			size = sizeof(LO_HorizRuleStruct);
			break;
		case LO_IMAGE:
			size = sizeof(LO_ImageStruct);
			break;
		case LO_BULLET:
			size = sizeof(LO_BulletStruct);
			break;
		case LO_FORM_ELE:
			size = sizeof(LO_FormElementStruct);
			break;
		case LO_SUBDOC:
			size = sizeof(LO_SubDocStruct);
			break;
		case LO_TABLE:
			size = sizeof(LO_TableStruct);
			break;
		case LO_CELL:
			size = sizeof(LO_CellStruct);
			break;
		case LO_EMBED:
			size = sizeof(LO_EmbedStruct);
			break;
		case LO_JAVA:
			size = sizeof(LO_JavaAppStruct);
			break;
		case LO_EDGE:
			size = sizeof(LO_EdgeStruct);
			break;
		default:
			size = sizeof(LO_Any);
			break;
	}
    if ( ! EDT_IS_EDITOR(context) ) {
    	eptr = (LO_Element *)lo_MemoryArenaAllocate(state->top_state, size);
    }
    else {
	    if (state->top_state->recycle_list != NULL)
	    {
		    eptr = state->top_state->recycle_list;
		    state->top_state->recycle_list = eptr->lo_any.next;
	    }
	    else
	    {
            eptr = XP_NEW(LO_Element);
        }
    }
#else

	/* sorry about all the asserts, in my debug version somehow state
	 * becomes nil after a malloc failure. */
#ifdef DEBUG
	assert (state);
#endif
	
	if (state->top_state->recycle_list != NULL)
	{
		eptr = state->top_state->recycle_list;
		state->top_state->recycle_list = eptr->lo_any.next;
	}
	else
	{
		eptr = XP_NEW(LO_Element);
#ifdef DEBUG
		assert (state);
#endif
	} 
#endif /* MEMORY_ARENAS */
	
	if (eptr == NULL)
	{
#ifdef DEBUG
		assert (state);
#endif
		state->top_state->out_of_memory = TRUE;
	}
	else
	{
		eptr->lo_any.next = NULL;
		eptr->lo_any.prev = NULL;
#ifdef EDITOR
		eptr->lo_any.edit_element = NULL;
		eptr->lo_any.edit_offset = NULL;

		if( EDT_IS_EDITOR(context)
			&& lo_EditableElement( type ))
		{
			if( edit_element == 0 ){
				edit_element = state->edit_current_element;
				edit_offset = state->edit_current_offset;
			}
			EDT_SetLayoutElement( edit_element, 
									edit_offset,
									type,
									eptr );
		}
		state->edit_force_offset = FALSE;
#endif
	}

	return(eptr);
}


Bool
lo_SetNamedAnchor(lo_DocState *state, PA_Block name, LO_Element *element)
{
	LO_Element *eptr;
	lo_NameList *name_rec;

	/*
	 * No named anchors are allowed within a
	 * hacked scrolling content layout document.
	 */
	if (state->top_state->scrolling_doc != FALSE)
	{
		if (name != NULL)
		{
			PA_FREE(name);
		}
		return FALSE;
	}

	if (element == NULL)
	{
		if (state->line_list == NULL)
		{
			eptr = state->end_last_line;
		}
		else
		{
			eptr = state->line_list;
			while (eptr->lo_any.next != NULL)
			{
				eptr = eptr->lo_any.next;
			}
		}
	}
	else
	{
		eptr = element;
	}

	name_rec = XP_NEW(lo_NameList);
	if (name_rec == NULL)
	{
		state->top_state->out_of_memory = TRUE;
		PA_FREE(name);
		return FALSE;
	}

	if (eptr == NULL)
	{
		name_rec->x = 0;
		name_rec->y = 0;
	}
	else
	{
		name_rec->x = eptr->lo_any.x;
		name_rec->y = eptr->lo_any.y;
	}
	name_rec->element = eptr;
	name_rec->name = name;

	name_rec->next = state->name_list;
	state->name_list = name_rec;
	return TRUE;
}


void
lo_AddNameList(lo_DocState *state, lo_DocState *old_state)
{
	lo_NameList *name_list;
	lo_NameList *nptr;

	name_list = old_state->name_list;
	while (name_list != NULL)
	{
		/*
		 * Extract the current record from the list.
		 */
		nptr = name_list;
		name_list = name_list->next;

		/*
		 * The element almost certainly has been moved
		 * from subdoc to main cell, correct its position.
		 */
		if (nptr->element != NULL)
		{
			nptr->x = nptr->element->lo_any.x;
			nptr->y = nptr->element->lo_any.y;
		}

		/*
		 * Check is this name is one we are waiting on.
		 */
		if ((state->display_blocking_element_id == -1)&&
		    (nptr->name != NULL))
		{
			char *name;

			PA_LOCK(name, char *, nptr->name);
			if (XP_STRCMP(name,
			    (char *)(state->top_state->name_target + 1)) == 0)
			{
				XP_FREE(state->top_state->name_target);
				state->top_state->name_target = NULL;
				if (nptr->element != NULL)
				{
					state->display_blocking_element_id =
						nptr->element->lo_any.ele_id;
				}
				else
				{
					state->display_blocking_element_id =
						state->top_state->element_id;
				}
				/*
				 * If the display is blocked for an element
				 * we havn't reached yet, check to see if
				 * it is in this line, and if so, save its
				 * y position.
				 */
				if ((state->display_blocked != FALSE)&&
				    (state->is_a_subdoc == SUBDOC_NOT)&&
				    (state->display_blocking_element_y == 0)&&
				    (state->display_blocking_element_id != -1))
				{
					state->display_blocking_element_y = nptr->y;
				}
			}
			PA_UNLOCK(nptr->name);
		}

		/*
		 * Stick it into the main name list.
		 */
		nptr->next = state->name_list;
		state->name_list = nptr;
	}
	old_state->name_list = name_list;
}


void
lo_AppendToLineList(MWContext *context, lo_DocState *state,
	LO_Element *element, int32 baseline_inc)
{
	LO_Element *eptr;

	if (state->current_named_anchor != NULL)
	{
		lo_SetNamedAnchor(state, state->current_named_anchor, element);
	}
	state->current_named_anchor = NULL;

	element->lo_any.next = NULL;
	element->lo_any.prev = NULL;
	eptr = state->line_list;
	if (eptr == NULL)
	{
		state->line_list = element;
	}
	else
	{
		while (eptr->lo_any.next != NULL)
		{
			eptr->lo_any.y_offset += baseline_inc;
			eptr = eptr->lo_any.next;
		}
		eptr->lo_any.y_offset += baseline_inc;
		eptr->lo_any.next = element;
		element->lo_any.prev = eptr;
	}
}


void
lo_SetLineArrayEntry(lo_DocState *state, LO_Element *eptr, int32 line)
{
	LO_Element **line_array;

#ifdef XP_WIN16
        intn a_size;
        intn a_indx;
        intn a_line;
        XP_Block *larray_array;
#endif /* XP_WIN16 */

	if (line >= (state->line_num - 1))
	{
		return;
	}

#ifdef XP_WIN16
        a_size = SIZE_LIMIT / sizeof(LO_Element *);
        a_indx = (intn)(line / a_size);
        a_line = (intn)(line - (a_indx * a_size));

        XP_LOCK_BLOCK(larray_array, XP_Block *, state->larray_array);
        state->line_array = larray_array[a_indx];
        XP_LOCK_BLOCK(line_array, LO_Element **, state->line_array);
	line_array[a_line] = eptr;

        XP_UNLOCK_BLOCK(state->line_array);
        XP_UNLOCK_BLOCK(state->larray_array);
#else
        XP_LOCK_BLOCK(line_array, LO_Element **, state->line_array);
	line_array[line] = eptr;
        XP_UNLOCK_BLOCK(state->line_array);
#endif /* XP_WIN16 */
}


/*
 * Return the width of the window for this context in chars in the default
 * fixed width font.  Return -1 on error.
 */
int16
LO_WindowWidthInFixedChars(MWContext *context)
{
	int32 doc_id;
	lo_TopState *top_state;
	lo_DocState *state;
	LO_TextInfo tmp_info;
	LO_TextAttr tmp_attr;
	LO_TextStruct tmp_text;
	PA_Block buff;
	char *str;
	int32 width;
	int16 char_cnt;

	/*
	 * Get the unique document ID, and retreive this
	 * documents layout state.
	 */
	doc_id = XP_DOCID(context);
	top_state = lo_FetchTopState(doc_id);
	if ((top_state == NULL)||(top_state->doc_state == NULL))
	{
		return((int16)-1);
	}
	state = top_state->doc_state;

	/*
	 * Create a fake text buffer to measure an 'M'.
	 */
	memset (&tmp_text, 0, sizeof (tmp_text));
	buff = PA_ALLOC(1);
	if (buff == NULL)
	{
		return((int16)-1);
	}
	PA_LOCK(str, char *, buff);
	str[0] = 'M';
	PA_UNLOCK(buff);
	tmp_text.text = buff;
	tmp_text.text_len = 1;

	/*
	 * Fill in default fixed font information.
	 */
	lo_SetDefaultFontAttr(state, &tmp_attr, context);
	tmp_attr.fontmask |= LO_FONT_FIXED;
	tmp_text.text_attr = &tmp_attr;

	/*
	 * Get the width of that 'M'
	 */
	FE_GetTextInfo(context, &tmp_text, &tmp_info);
	PA_FREE(buff);

	/*
	 * Error if bad character width
	 */
	if (tmp_info.max_width <= 0)
	{
		return((int16)-1);
	}

	width = state->win_width - state->win_left - state->win_right;
	char_cnt = (int16)(width / tmp_info.max_width);
	return(char_cnt);
}


LO_Element *
lo_FirstElementOfLine(MWContext *context, lo_DocState *state, int32 line)
{
	LO_Element *eptr;
	LO_Element **line_array;

	if ((line < 0)||(line > (state->line_num - 2)))
	{
		return(NULL);
	}

#ifdef XP_WIN16
	{
		intn a_size;
		intn a_indx;
		intn a_line;
		XP_Block *larray_array;

		a_size = SIZE_LIMIT / sizeof(LO_Element *);
		a_indx = (intn)(line / a_size);
		a_line = (intn)(line - (a_indx * a_size));
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
		eptr = line_array[line];
		XP_UNLOCK_BLOCK(state->line_array);
	}
#endif /* XP_WIN16 */
	return(eptr);
}


#ifdef MOCHA

static Bool
lo_find_mocha_expr(char *str, char **expr_start, char **expr_end)
{
	char *tptr;
	char *start;
	char *end;

	if (str == NULL)
	{
		return(FALSE);
	}

	start = NULL;
	end = NULL;
	tptr = str;
	while (*tptr != '\0')
	{
		if ((*tptr == '&')&&(*(char *)(tptr + 1) == '{'))
		{
			start = tptr;
			break;
		}
		tptr++;
	}

	if (start == NULL)
	{
		return(FALSE);
	}

	tptr = (char *)(start + 2);
	while (*tptr != '\0')
	{
		if ((*tptr == '}')&&(*(char *)(tptr + 1) == ';'))
		{
			end = tptr;
			break;
		}
		tptr++;
	}

	if (end == NULL)
	{
		return(FALSE);
	}

	end = (char *)(end + 1);
	*expr_start = start;
	*expr_end = end;
	return(TRUE);
}


static char *
lo_add_string(char *base, char *add)
{
	char *new;
	int32 base_len, add_len;

	if (add == NULL)
	{
		return(base);
	}

	if (base == NULL)
	{
		new = XP_STRDUP(add);
		return(new);
	}

	base_len = XP_STRLEN(base);
	add_len = XP_STRLEN(add);
	new = (char *)XP_ALLOC(base_len + add_len + 1);
	if (new == NULL)
	{
		return(base);
	}

	XP_STRCPY(new, base);
	XP_STRCAT(new, add);
	XP_FREE(base);
	return(new);
}


static char *
lo_eval_javascript_entities(MWContext *context, char *orig_value,
							uint newline_count)
{
	char *tptr;
	char *str;
	char *new_str;
	int32 len;
	char tchar;
	Bool has_mocha;
	char *start, *end;

	if (orig_value == NULL)
	{
		return(NULL);
	}

	str = orig_value;
	len = XP_STRLEN(str);

	new_str = NULL;
	tptr = str;
	has_mocha = lo_find_mocha_expr(tptr, &start, &end);
	while (has_mocha != FALSE)
	{
		char *expr_start, *expr_end;

		if (start > tptr)
		{
			tchar = *start;
			*start = '\0';
			new_str = lo_add_string(new_str, tptr);
			*start = tchar;
		}

		expr_start = (char *)(start + 2);
		expr_end = (char *)(end - 1);
		if (expr_end > expr_start)
		{
			char *eval_str;

			tchar = *expr_end;
			*expr_end = '\0';
			eval_str = LM_EvaluateAttribute(context, expr_start,
											newline_count + 1);
			*expr_end = tchar;
			new_str = lo_add_string(new_str, eval_str);
			if (eval_str != NULL)
			{
				XP_FREE(eval_str);
			}
		}

		tptr = (char *)(end + 1);
		has_mocha = lo_find_mocha_expr(tptr, &start, &end);
	}
	if (tptr != str)
	{
		new_str = lo_add_string(new_str, tptr);
	}

	if (new_str == NULL)
	{
		new_str = str;
	}

	return(new_str);
}

#endif /* MOCHA */


void
lo_ConvertAllValues(MWContext *context, char **value_list, int32 list_len,
					uint newline_count)
{
#ifdef MOCHA
	int32 i;

	if (LM_GetMochaEnabled() == FALSE)
	{
		return;
	}

	for (i=0; i < list_len; i++)
	{
		char *str;
		char *new_str;

		str = value_list[i];
		if (str != NULL)
		{
			new_str = lo_eval_javascript_entities(context, str, newline_count);
			if (new_str != str)
			{
				XP_FREE(str);
				value_list[i] = new_str;
			}
		}
	}
#endif /* MOCHA */
}


PA_Block
lo_FetchParamValue(MWContext *context, PA_Tag *tag, char *param_name)
{
	PA_Block buff;
	char *str;

	buff = PA_FetchParamValue(tag, param_name);
#ifdef MOCHA /* added by jwz */
	if (LM_GetMochaEnabled() == FALSE)
	{
		return(buff);
	}
	if (buff != NULL)
	{
		char *new_str;

		PA_LOCK(str, char *, buff);

		new_str = lo_eval_javascript_entities(context, str, tag->newline_count);

		if (new_str != NULL)
		{
			int32 new_len;
			PA_Block new_buff;

			new_len = XP_STRLEN(new_str);
			new_buff = PA_ALLOC(new_len + 1);
			if (new_buff != NULL)
			{
				char *tmp_str;

				PA_LOCK(tmp_str, char *, new_buff);
				XP_STRCPY(tmp_str, new_str);
				PA_UNLOCK(new_buff);
				/*
				 * If the original buffer was
				 * copied, free the copy.
				 */
				if (new_str != str)
				{
					XP_FREE(new_str);
				}
				PA_UNLOCK(buff);
				PA_FREE(buff);
				buff = new_buff;
				PA_LOCK(str, char *, buff);
			}
		}
		PA_UNLOCK(buff);
	}
#endif /* MOCHA */

	return(buff);
}


#ifdef PROFILE
#pragma profile off
#endif
