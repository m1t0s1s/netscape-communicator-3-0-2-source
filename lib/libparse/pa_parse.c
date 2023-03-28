/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t -*-
*/
#include "pa_parse.h"
#include <stdio.h>
#include "merrors.h"
#include "net.h"


extern int MK_OUT_OF_MEMORY;


#ifdef PROFILE
#pragma profile on
#endif

#ifdef XP_WIN16
#define	HOLD_BUF_UNIT		32000
#define	SIZE_LIMIT		32000
#else
#define	HOLD_BUF_UNIT		16384
#endif /* XP_WIN16 */

/*
 * Function to call with parsed tag elements.
 * It should be initialized by a call to PA_ParserInit*().
 */
static intn  (*PA_ParsedTag)(void *data_object, PA_Tag *tags, intn status) = NULL;


/*************************
 * The following is to speed up case conversion
 * to allow faster checking of caseless equal among strings.
 *************************/
#ifndef NON_ASCII_STRINGS
unsigned char lower_lookup[256]={
    0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,
    27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,
    51,52,53,54,55,56,57,58,59,60,61,62,63,64,
        97,98,99,100,101,102,103,104,105,106,107,108,109,
        110,111,112,113,114,115,116,117,118,119,120,121,122,
    91,92,93,94,95,96,97,98,99,100,101,102,103,104,105,106,107,108,109,110,
    111,112,113,114,115,116,117,118,119,120,121,122,123,124,125,126,127,128,
    129,130,131,132,133,134,135,136,137,138,139,140,141,142,143,144,145,146,
    147,148,149,150,151,152,153,154,155,156,157,158,159,160,161,162,163,164,
    165,166,167,168,169,170,171,172,173,174,175,176,177,178,179,180,181,182,
    183,184,185,186,187,188,189,190,191,192,193,194,195,196,197,198,199,200,
    201,202,203,204,205,206,207,208,209,210,211,212,213,214,215,216,217,218,
    219,220,221,222,223,224,225,226,227,228,229,230,231,232,233,234,235,236,
    237,238,239,240,241,242,243,244,245,246,247,248,249,250,251,252,253,254,
    255};
#endif /* not NON_ASCII_STRINGS */



/*************************************
 * Function: pa_caseless_equal
 *
 * Description: This function will compare two
 *      strings, similar to strcmp(), but ignoring the
 *      case of the letters A-Z.
 *
 * Params: Takes two \0 terminated strings.
 *
 * Returns: 1 if strings are equal, 0 if not.
 *************************************/
#if 0
static intn
pa_caseless_equal(char *string_1, char *string_2)
{
    /*
     * If either is NULL, they are not equal, even if both are NULL
     */
    if ((string_1 == NULL)||(string_2 == NULL))
    {
        return(0);
    }

    /*
     * While not at the end of the string, if they ever differ
     * they are not equal.
     */
    while ((*string_1 != '\0')&&(*string_2 != '\0'))
    {
        if (TOLOWER((unsigned char) *string_1) != TOLOWER((unsigned char) *string_2))
        {
            return(0);
        }
        string_1++;
        string_2++;
    }

    /*
     * One of the strings has ended, if they are both ended, then they
     * are equal, otherwise not.
     */
    if ((*string_1 == '\0')&&(*string_2 == '\0'))
    {
        return(1);
    }
    else
    {
        return(0);
    }
}
#endif


/*************************************
 * Function: pa_TagEqual
 *
 * Description: This function is a special purpose caseless compare
 *      to save me a few cycles of performance.
 *      Since we know the first string is a predefined TAG
 *      we are guaranteeing it will always be in lower case,
 *      thus we don't need to TOLOWER its characters as we
 *      compare them.
 *
 * Params: Takes two \0 terminated strings.  The first, being a predefined TAG
 *     is guaranteed to be all in lower case.
 *
 * Returns: 1 if strings are equal, 0 if not.
 *************************************/
intn
pa_TagEqual(char *tag, char *str)
{
    /*
     * If str is NULL, they are not equal, tag cannot be NULL.
     */
    if (str == NULL)
    {
        return(0);
    }

    /*
     * While not at the end of the string, if they ever differ
     * they are not equal.
     */
    while ((*tag != '\0')&&(*str != '\0'))
    {
        if ((int)(*tag) != TOLOWER((unsigned char) *str))
        {
            return(0);
        }
        tag++;
        str++;
    }

    /*
     * One of the strings has ended, if they are both ended, then they
     * are equal, otherwise not.
     */
    if ((*tag == '\0')&&(*str == '\0'))
    {
        return(1);
    }
    else
    {
        return(0);
    }
}


/*************************************
 * Function: PA_FreeTag
 *
 * Description: This function frees up all memory associated
 *      with a PA_Tag structure, including the structure
 *      itself.
 *
 * Params: Takes pointer to a PA_Tag structure.
 *
 * Returns: none.
 *************************************/
void
PA_FreeTag(PA_Tag *tag)
{
    /*
     * Nothing to do for already freed tags.
     */
    if (tag == NULL)
    {
        return;
    }

    /*
     * If we have data, free it.
     */
    if (tag->data != NULL)
    {
        PA_FREE(tag->data);
    }

    /*
     * Free the tag structure.
     */
    XP_DELETE(tag);
}


static int32 doc_id_gen = 0;	/* generator for document identifiers */


/*************************************
 * Function: pa_new_document
 *
 * Description: register a new document, create and initialize
 *      the pa_DocData structure for it.
 *
 * Params: Takes a unique document id, and the URL_Struct for this doc.
 *
 * Returns: a pointer to the new pa_DocData structure, already
 *      initialized, and with the doc_id filled in.
 *      Return NULL on failure.
 *************************************/
static pa_DocData *
pa_new_document(MWContext *window_id, PA_OutputFunction *output_func,
	URL_Struct *url_struct)
{
    pa_DocData *doc_data;

    /* Added by Lou:
     * This will interrupt anything else trying to go into
     * this same window, so that we have a clear path
     * to load this new document
     */
    NET_SilentInterruptWindow(window_id);

    doc_data = XP_NEW(pa_DocData);
    if (doc_data == NULL)
    {
        return(NULL);
    }

    /*
     * Allocate a static hold buffer.  This will
     * save on malloc calls in the long run.
     */
    doc_data->hold_buf = XP_ALLOC_BLOCK(HOLD_BUF_UNIT * sizeof(char));
    if (doc_data->hold_buf == NULL)
    {
        XP_DELETE(doc_data);
        return(NULL);
    }

    /*
     * Now that we can't fail, create the unique document ID.
     */
    doc_data->doc_id = ++doc_id_gen;
    doc_data->window_id = window_id;
    doc_data->output_tag = output_func;
    doc_data->hold = 0;
    doc_data->hold_size = HOLD_BUF_UNIT;
    doc_data->hold_len = 0;
    doc_data->brute_tag = P_UNKNOWN;
    doc_data->comment_bytes = 0;
    doc_data->lose_newline = FALSE;
    doc_data->layout_state = NULL;
    if (url_struct->address == NULL)
    {
	doc_data->url = NULL;
    }
    else
    {
	doc_data->url = XP_STRDUP(url_struct->address);
    }

    if ((url_struct->cache_file != NULL)||(url_struct->memory_copy != NULL))
    {
	doc_data->from_net = FALSE;
    }
    else
    {
	doc_data->from_net = TRUE;
    }

    /*
     * A NET_SUPER_RELOAD should always make everything reload, so no
     * matter what, act as if it all came new from the net.
     */
    if (url_struct->force_reload == NET_SUPER_RELOAD)
    {
	doc_data->from_net = TRUE;
    }

    doc_data->url_struct = url_struct;
#ifdef EDITOR
    doc_data->edit_buffer = NULL;
#endif

    return(doc_data);
}

/* the parser write stream is always ready to write
 */
unsigned int
pa_ParseWriteReady (void *data_object)
{
  return ((unsigned int) 2*1024);
}


/*************************************
 * Function: PA_BeginParseMDL
 *
 * Description: The outside world's main access to the parser.
 *      call this when you are going to start parsing
 *      a new document to set up the parsing stream.
 *      This function cannot be called successfully
 *      until PA_ParserInit() has been called.
 *
 * Params: Takes lots of document information that is all
 *     ignored right now, just used the window_id to create
 *     a unique document id.
 *
 * Returns: a pointer to a new NET_StreamClass structure, set up to
 *      give the caller a parsing stream into the parser.
 *      Returns NULL on error.
 *************************************/
NET_StreamClass *
PA_BeginParseMDL(FO_Present_Types format_out,
    void *init_data, URL_Struct *anchor, MWContext *window_id)
{
    NET_StreamClass *new_stream;
    PA_InitData *new_data;
    pa_DocData *doc_data;

    new_data = (PA_InitData *)init_data;

    /*
     * Create the new stream structure.
     */
    new_stream = XP_NEW(NET_StreamClass);
    if (new_stream == NULL)
    {
        return(NULL);
    }

    /*
     * If there was a Window-Target http header from the server,
     * we probably need to switch where this document goes.
     */
    if ((anchor->window_target != NULL)&&
	(*anchor->window_target != '\0')&&
	((format_out == FO_PRESENT)||(format_out == FO_CACHE_AND_PRESENT)))
    {
	if (NET_IsSafeForNewContext(anchor) != FALSE)
	{
		MWContext *new_context;

		/*
		 * Find the named window if it already exists.
		 */
		new_context = XP_FindNamedContextInList(window_id,
					anchor->window_target);
		/*
		 * If the named window didn't exist, create it.
		 */
		if (new_context == NULL)
		{
			/*
			 *	Don't pass in the URL_Struct if we're going to manually
			 *		get it to load ourselves, as MakeNewWindow will load
			 *		a URL passed in!
			 */
			new_context = FE_MakeNewWindow(window_id, NULL /* anchor */,
				anchor->window_target, anchor->window_chrome);
		}
		/*
		 * Else is the named window did exist, and we are it, we
		 * don't need to do anything.
		 */
		else if (new_context == window_id)
		{
			new_context = NULL;
		}

		/*
		 * Switch to loading this URL in the named window.
		 */
		if (new_context != NULL)
		{
			Net_GetUrlExitFunc *exit_func;

			exit_func = NULL;
			FE_SetWindowLoading(new_context, anchor, &exit_func);
			if (NET_SetNewContext(anchor, new_context, exit_func)
					== 0)
			{
				window_id = new_context;
			}
		}
	}
    }

    new_stream->name = NULL;
    new_stream->window_id = window_id;

    /*
     * Allocate the crucial data object that contains all the
     * per document parsing state information.
     */
    doc_data = pa_new_document(window_id, new_data->output_func, anchor);
    if (doc_data == NULL)
    {
        XP_DELETE(new_stream);
        return(NULL);
    }
    doc_data->format_out = format_out;
    doc_data->parser_stream = new_stream;
    doc_data->newline_count = 0;
    new_stream->data_object = (void *)doc_data;

    /*
     * Functions to call to use the parsing stream.
     */
    new_stream->complete = PA_MDLComplete;
    new_stream->abort = PA_MDLAbort;
    new_stream->put_block = (MKStreamWriteFunc)PA_ParseBlock;
    new_stream->is_write_ready = pa_ParseWriteReady;

    return(new_stream);
}


/*************************************
 * Function: PA_ParserInit
 *
 * Description: Very main interface to the parser library.
 *      It must be called before the parser can be used.
 *      Right now it just sets the one static global we use.
 *      This function can only be called once, all
 *      subsequent calls will fail.
 *
 * Params: Pass in a pointer to a PA_Functions struct, which tells the parser
 *     what functions to use for certain important functionality.
 *
 * Returns: A status code.  1 on success, -1 on failure.
 *************************************/
intn
PA_ParserInit(PA_Functions *funcs)
{
    /*
     * If this is not the first call, fail them.
     */
    if (PA_ParsedTag != NULL)
    {
        return(-1);
    }

    PA_ParsedTag = funcs->PA_ParsedTag;

    if (PA_ParsedTag == NULL)
    {
        return(-1);
    }
    else
    {
        return(1);
    }
}


/*************************************
 * Function: PA_ParseBlock
 *
 * Description: This is a very important entry point to the parser,
 *      but it will never be called directly.  It will be
 *      placed into the stream returned by PA_BeginParseMDL()
 *      and be called from there.
 *
 * Params: The data_object created and placed in the stream class
 *     in PA_BeginParseMDL().  A buffer of characters to be
 *     parsed, and the length of that buffer.  The buffer is NOT
 *     a \0 terminated string.
 *
 * Returns: a status code.  1 = success, -1 = failure.
 *************************************/
intn
PA_ParseBlock(void *data_object, const char *block, int block_len)
{
    pa_DocData *doc_data;
    PA_Tag *tag;
    intn ret;
    int32 len;
    char *buf;
    char *hold_buf;
    XP_Block buff;
#ifdef XP_WIN16
    int32 extra;
    char *extra_ptr;

    extra_ptr = NULL;
    extra = 0;
#endif /* XP_WIN16 */
    buf = (char *)block;
    len = (int32)block_len;

    /*
     * Parse this unique MDL document.  Get per-document state info.
     */
    doc_data = (pa_DocData *)data_object;

    /*
     * If we are holding buffered data for this document from a
     * previous parse attempt,
     * Glomb it onto the beginning in a new buffer.
     *
     * We hold data if we might have a partial MDL tag element.
     * We hold data if we might have a partial ampersand escape.
     * We hold data if we might have a two character newline to skip.
     */
    if (doc_data->hold)
    {
	if ((doc_data->hold_len + len) > doc_data->hold_size)
	{
	    /*
	     * Grow the hold buffer if itis not big enough to hold
	     * the combined buffers.
	     */
#ifdef XP_WIN16
	    /*
	     * On the 32K limit, our hold_buf is already max size
	     */
#else
	    buff = XP_REALLOC_BLOCK(doc_data->hold_buf,
		((doc_data->hold_size + HOLD_BUF_UNIT + len) * sizeof(char)));
	    if (buff == NULL)
	    {
                return(MK_OUT_OF_MEMORY);
	    }
	    doc_data->hold_buf = buff;
	    doc_data->hold_size += (HOLD_BUF_UNIT + len);
#endif /* XP_WIN16 */
	}

        /*
         * Lock down the hold buffer so we can do pointer magic
         * on it.
         */
        XP_LOCK_BLOCK(hold_buf, char *, (doc_data->hold_buf));

        /*
         * Append the new buffer to the old buffer
         * Make it look like the merged chunk is what
         * was passed to us originally.
         */
#ifdef XP_WIN16
	if ((doc_data->hold_len + len) > doc_data->hold_size)
	{
		extra = doc_data->hold_len + len - doc_data->hold_size;
		extra_ptr = (char *)(buf + (len - extra));

		XP_BCOPY(buf, (hold_buf + doc_data->hold_len), (len - extra));
		buf = hold_buf;
		len = doc_data->hold_len + len - extra;
		doc_data->hold_len = len;
	}
	else
	{
		XP_BCOPY(buf, (hold_buf + doc_data->hold_len), len);
		buf = hold_buf;
		len = len + doc_data->hold_len;
		doc_data->hold_len = len;
	}
#else
        XP_BCOPY(buf, (hold_buf + doc_data->hold_len), len);
        buf = hold_buf;
        len = len + doc_data->hold_len;
        doc_data->hold_len = len;
#endif /* XP_WIN16 */
    }
    else
    {
	/*
	 * We always want the hold buffer to be locked as we enter the
	 * following while loop
	 */
        doc_data->hold_len = 0;
        XP_LOCK_BLOCK(hold_buf, char *, (doc_data->hold_buf));
    }
    doc_data->hold = 0;

    /*
     * Loop until we get a partial something to hold,
     * or we have drained the buffer.
     */
    while ((!doc_data->hold)&&(len != 0))
    {
	char *tptr;
	char *tptr2;
	intn is_comment;

	/*
	 * The P_PLAIN_TEXT tag element is very special, and
	 * if we just got one, all other text is just dumped
	 * out of the parser as plain text.
	 */
	if (doc_data->brute_tag == P_PLAIN_TEXT)
	{
	    tag = pa_CreateTextTag(doc_data, buf, len);
	    if (tag == NULL)
	    {
		return(MK_OUT_OF_MEMORY);
	    }
	    ret = doc_data->output_tag(data_object, tag, PA_PARSED);
	    if (ret < 0)
	    {
		return(ret);
	    }
	    buf = NULL;
	    len = 0;
	    break;
	}

	/*
	 * In the case where we just removed a comment, or just opened
	 * a preformatting tag, we want to remove any newline that
	 * appears right after that tag element because we assume
	 * it was really "part of the element", I.E.  WE are guessing
	 * that the user WANTS us to remove it, a dangerous guess.
	 *
	 * Depending on what platform generated the file, a newline
	 * could be \n, or \r\n.  Since it can span 2 characters, we
	 * may actually have to hold until the next buffer to
	 * throw away the newline.
	 */
	if (doc_data->lose_newline != FALSE)
	{
	    if (*buf == '\n')
	    {
		buf++;
		len--;
		if (len == 0)
		{
		    buf = NULL;
		}
		doc_data->newline_count++;
		doc_data->comment_bytes++;
	    }
	    else if ((*buf == '\r')&&(len == 1))
	    {
		doc_data->hold = 1;
		/*
		 * Grow the hold buffer if it is not big enough to hold
		 * the rest of this buffer.
		 */
#ifdef XP_WIN16
	    /*
	     * On the 32K limit, our hold_buf is already max size
	     */
#else
		if (len > doc_data->hold_size)
		{
		    XP_UNLOCK_BLOCK((doc_data->hold_buf));
		    buff = XP_REALLOC_BLOCK(doc_data->hold_buf,
			((doc_data->hold_size + HOLD_BUF_UNIT + len) *
			sizeof(char)));
		    if (buff == NULL)
		    {
			return(MK_OUT_OF_MEMORY);
		    }
		    doc_data->hold_buf = buff;
		    XP_LOCK_BLOCK(hold_buf, char *,
			(doc_data->hold_buf));
		    doc_data->hold_size += (HOLD_BUF_UNIT + len);
		}
#endif /* XP_WIN16 */

		XP_BCOPY(buf, hold_buf, len);
		doc_data->hold_len = len;
		continue;
	    }
	    else if ((*buf == '\r')&&(*((char *)(buf + 1)) == '\n'))
	    {
		buf += 2;
		len -= 2;
		if (len == 0)
		{
		    buf = NULL;
		}
		doc_data->newline_count++;
		doc_data->comment_bytes += 2;
	    }
	    doc_data->lose_newline = FALSE;
	}

	/*
	 * Find the start of any MDL tags in this buffer.
	 * Returns NULL if there are none.
	 */
	tptr = pa_FindMDLTag(doc_data, buf, len, &is_comment);

	/*
	 * Some portion of the start of the buffer is text.
	 */
	if (tptr != buf)
	{
	    int32 text_len;
	    int32 new_len;

	    /*
	     * Find the length of the text.
	     */
	    if (tptr == NULL)
	    {
		text_len = len;
	    }
	    else
	    {
		text_len = (int32)(tptr - buf);
	    }

	    /*
	     * Expand any ampersand escapes.  We might need
	     * to hold a partial escape.  Ampersand escapes
	     * are NOT expanded if we are inside one of the
	     * following elements.
	     */
	    if ((doc_data->brute_tag == P_PLAIN_PIECE)||
		(doc_data->brute_tag == P_SERVER)||
		(doc_data->brute_tag == P_SCRIPT))
	    {
		tptr2 = NULL;
		new_len = text_len;
	    }
	    else
	    {
		/*
		 * If we have an MDL tag right after this, we can't have
		 * partial escapes because the tag is a guaranteed
		 * terminator.  Thus force expansion is set to true.
		 */
		if (tptr != NULL)
		{
			tptr2 = pa_ExpandEscapes(buf, text_len,
				&new_len, TRUE);
		}
		else
		{
			tptr2 = pa_ExpandEscapes(buf, text_len,
				&new_len, FALSE);
		}
	    }

	    /*
	     * Create and parse the text into a layout element.
	     */
	    tag = pa_CreateTextTag(doc_data, buf, new_len);
	    if (tag == NULL)
	    {
		return(MK_OUT_OF_MEMORY);
	    }
	    ret = doc_data->output_tag(data_object, tag, PA_PARSED);
	    if (ret < 0)
	    {
		return(ret);
	    }

	    /*
	     * Check if we are holding a partial ampersand escape.
	     */
	    if (tptr2 != NULL)
	    {
		text_len = (int32)(tptr2 - buf);
		tptr = tptr2;
		is_comment = COMMENT_MAYBE;
	    }

	    /*
	     * Move up pointers so beginning of tag now heads the buffer.
	     */
	    buf = tptr;
	    if (buf == NULL)
	    {
		len = 0;
	    }
	    else
	    {
		len = len - text_len;
	    }
	}

	/*
	 * If we got a maybe, we need to save this
	 * remnant for later.
	 */
	if ((is_comment == COMMENT_MAYBE)&&(buf != NULL))
	{
	    doc_data->hold = 1;
	    /*
	     * Grow the hold buffer if it is not big enough to hold
	     * the rest of this buffer.
	     */
#ifdef XP_WIN16
	    /*
	     * On the 32K limit, our hold_buf is already max size
	     */
#else
	    if (len > doc_data->hold_size)
	    {
		XP_UNLOCK_BLOCK((doc_data->hold_buf));
		buff = XP_REALLOC_BLOCK(doc_data->hold_buf,
		    ((doc_data->hold_size + HOLD_BUF_UNIT + len) *
		    sizeof(char)));
		if (buff == NULL)
		{
		    return(MK_OUT_OF_MEMORY);
		}
		doc_data->hold_buf = buff;
		XP_LOCK_BLOCK(hold_buf, char *,
			(doc_data->hold_buf));
		doc_data->hold_size += (HOLD_BUF_UNIT + len);
	    }
#endif /* XP_WIN16 */

	    XP_BCOPY(buf, hold_buf, len);
	    doc_data->hold_len = len;
	}
	/*
	 * else we either have the start of an MDL tag, or
	 * buf == NULL.  Find the end of the tag if we
	 * have it in the buffer, otherwise return NULL.
	 */
	else
	{
	    if (is_comment == COMMENT_YES)
	    {
		tptr = pa_FindMDLEndComment(doc_data, buf, len);
	    }
	    else
	    {
		tptr = pa_FindMDLEndTag(doc_data, buf, len);
	    }

	    /*
	     * Got the end of the MDL comment,
	     * discard the comment
	     */
	    if ((tptr != NULL)&&(is_comment == COMMENT_YES))
	    {
		int32 comment_len;

		comment_len = (int32)(tptr - buf) + 1;

		/*
		 * If we are inside one of the "special"
		 * tags that ignore all tags except their
		 * own endtags (e.g. P_TITLE, P_PLAIN_PIECE)
		 * we need to output this comment
		 * as normal text.
		 */
		if (doc_data->brute_tag != P_UNKNOWN)
		{
		    tag = pa_CreateTextTag(doc_data, buf, comment_len);
		    if (tag == NULL)
		    {
			return(MK_OUT_OF_MEMORY);
		    }
		    ret = doc_data->output_tag(data_object, tag, PA_PARSED);
		    if (ret < 0)
		    {
			return(ret);
		    }
		}
		else if (doc_data->window_id && EDT_IS_EDITOR(doc_data->window_id))
		{
		    /*
		     * The Editor wants to see comments.
		     */
		    tag = pa_CreateMDLTag(doc_data, buf, comment_len);
		    if (tag == NULL)
		    {
			return(MK_OUT_OF_MEMORY);
		    }
		    ret = doc_data->output_tag(data_object, tag, PA_PARSED);
		    if (ret < 0)
		    {
			return(ret);
		    }
		}
		else
		{
		    /*
		     * Apply lose_newline heuristic after
		     * discarding a comment.
		     */
		    doc_data->lose_newline = TRUE;
		    doc_data->comment_bytes += comment_len;
		}

		/*
		 * Move the buffer forward.
		 */
		len = len - comment_len;
		if (len == 0)
		{
		    buf = NULL;
		}
		else
		{
		    buf = tptr;
		    buf++;
		}
	    }
	    /*
	     * Else got the end of the MDL tag!
	     */
	    else if (tptr != NULL)
	    {
		int32 text_len;

		/*
		 * Create and format the tag(s)
		 */
		text_len = (int32)(tptr - buf) + 1;
		tag = pa_CreateMDLTag(doc_data, buf, text_len);

		/*
		 * If we are inside one of the "special"
		 * tags that ignore all tags except their
		 * own endtags (e.g. P_TITLE, P_PLAIN_PIECE)
		 * we check here to see if this is the
		 * proper end tag, if not, we turn it
		 * back into normal text.
		 */
		if (doc_data->brute_tag != P_UNKNOWN)
		{
		    if ((tag == NULL)||
			(tag->is_end == FALSE)||
			(doc_data->brute_tag != tag->type))
		    {
			PA_FreeTag(tag);
			/*
			 * Strip only the '<' which made us think this
			 * was an HTML tag.
			 */
			text_len = 1;
			tptr = buf;
			tag = pa_CreateTextTag(doc_data, buf, text_len);
			if (tag == NULL)
			{
			    return(MK_OUT_OF_MEMORY);
			}
		    }
		    else
		    {
			doc_data->brute_tag = P_UNKNOWN;
		    }
		}

		/*
		 * These tags are special in that, after opening one
		 * of them, all other tags are ignored until the matching
		 * closing tag.
		 */
		if ((tag != NULL)&&(tag->is_end == FALSE)&&
		    ((tag->type == P_TITLE)||
		     (tag->type == P_TEXTAREA)||
		     (tag->type == P_PLAIN_PIECE)||
		     (tag->type == P_PLAIN_TEXT)||
		     (tag->type == P_SERVER)||
		     (tag->type == P_SCRIPT)))
		{
		    doc_data->brute_tag = tag->type;
		}

		/*
		 * These tags are special in that, after opening one
		 * of them, the lose_newline heuristic is applied.
		 */
		if ((tag != NULL)&&(tag->is_end == FALSE)&&
		    ((tag->type == P_TITLE)||
		     (tag->type == P_TEXTAREA)||
		     (tag->type == P_PLAIN_PIECE)||
		     (tag->type == P_LISTING_TEXT)||
		     (tag->type == P_PREFORMAT)))
		{
		    doc_data->lose_newline = TRUE;
		}

		ret = doc_data->output_tag(data_object, tag, PA_PARSED);
		if (ret < 0)
		{
		    return(ret);
		}

		/*
		 * Move the buffer forward.
		 */
		len = len - text_len;
		if (len == 0)
		{
		    buf = NULL;
		}
		else
		{
		    buf = tptr;
		    buf++;
		}
	    }
	    /*
	     * We couldn't find the end of the MDL tag.
	     * Hold the start if we have one.
	     */
	    else if (buf != NULL)
	    {
		doc_data->hold = 1;
		/*
		 * Grow the hold buffer if it is not big enough to hold
		 * the rest of this buffer.
		 */
#ifdef XP_WIN16
	    /*
	     * On the 32K limit, our hold_buf is already max size
	     */
#else
		if (len > doc_data->hold_size)
		{
		    XP_UNLOCK_BLOCK((doc_data->hold_buf));
		    buff = XP_REALLOC_BLOCK(doc_data->hold_buf,
			((doc_data->hold_size + HOLD_BUF_UNIT + len) *
			sizeof(char)));
		    if (buff == NULL)
		    {
			return(MK_OUT_OF_MEMORY);
		    }
		    doc_data->hold_buf = buff;
		    XP_LOCK_BLOCK(hold_buf, char *,
			(doc_data->hold_buf));
		    doc_data->hold_size += (HOLD_BUF_UNIT + len);
		}
#endif /* XP_WIN16 */

		XP_BCOPY(buf, hold_buf, len);
		doc_data->hold_len = len;
	    }
	} /* end of else on COMMENT_MAYBE */
    } /* end of while */

    /*
     * Unlock the hold buffer, and clear it if we
     * aren't holding anything this time around.
     */
    if ((!doc_data->hold)&&(doc_data->hold_buf != NULL))
    {
        hold_buf = NULL;
        doc_data->hold_len = 0;
        XP_UNLOCK_BLOCK((doc_data->hold_buf));
    }
    else if (doc_data->hold_buf != NULL)
    {
        XP_UNLOCK_BLOCK((doc_data->hold_buf));
    }

#ifdef XP_WIN16
    if (extra_ptr != NULL)
    {
	if ((doc_data->hold_len + extra) > doc_data->hold_size)
	{
		char minibuf[1];
		intn ret;

		XP_LOCK_BLOCK(hold_buf, char *, (doc_data->hold_buf));
		minibuf[0] = *hold_buf;
                XP_BCOPY((char *)(hold_buf + 1), hold_buf, (doc_data->hold_len - 1));
		doc_data->hold_len--;
		XP_UNLOCK_BLOCK((doc_data->hold_buf));

		tag = pa_CreateTextTag(doc_data, minibuf, 1);
	        if (tag == NULL)
	        {
	            return(MK_OUT_OF_MEMORY);
	        }
		ret = doc_data->output_tag(data_object, tag, PA_PARSED);
	        if (ret < 0)
	        {
		    return(ret);
	        }

		ret = PA_ParseBlock(data_object, extra_ptr, extra);
		return(ret);
	}
	else
	{
		XP_LOCK_BLOCK(hold_buf, char *, (doc_data->hold_buf));
                XP_BCOPY(extra_ptr, (char *)(hold_buf + doc_data->hold_len), extra);
		doc_data->hold_len += extra;
		XP_UNLOCK_BLOCK((doc_data->hold_buf));
	}
    }
#endif /* XP_WIN16 */

    return(1);
}


#ifdef DEBUG_jwz
static void
ensure_all_tags_closed(pa_DocData *doc_data)
{
#if 1
  char random_close_tags[] = "<NSCP_CLOSE>";
#else /* 0 */
  char random_close_tags[] =
    "</TABLE></TABLE></TABLE></TABLE></TABLE></TABLE>"
    "</TABLE></TABLE></TABLE></TABLE></TABLE></TABLE>";
#endif /* 0 */
  if (doc_data && doc_data->url_struct)
    PA_ParseBlock (doc_data, random_close_tags, strlen(random_close_tags));
}
#endif /* DEBUG_jwz */


/*************************************
 * Function: PA_MDLComplete
 *
 * Description: This is a very important entry point to the parser,
 *      but it will never be called directly.  It will be
 *      placed into the stream returned by PA_BeginParseMDL()
 *      and be called from there.  It tells the parser that the
 *      passed document is done parsing, there is no new data.
 *
 * Params: The data_object created and placed in the stream class
 *     in PA_BeginParseMDL().  This contains document specific
 *     parse state information.
 *
 * Returns: nothing.
 *************************************/
void
PA_MDLComplete(void *data_object)
{
    pa_DocData *doc_data;
    PA_Tag *tag;

    /*
     * This MDL document is complete
     */
    doc_data = (pa_DocData *)data_object;

#ifdef DEBUG_jwz
    ensure_all_tags_closed(doc_data);
#endif /* DEBUG_jwz */

    /*
     * If we were holding some data we hadn't parsed yet, we need
     * to flush it through now.  Since we couldn't resolve whatever
     * we were waiting for, just push it through as plain text.
     */
    if (doc_data->hold)
    {
        char *tptr;

        /*
         * Lock down the hold buffer so we can do pointer magic
         * on it.
         */
        XP_LOCK_BLOCK(tptr, char *, (doc_data->hold_buf));

	/*
	 * Due to horrible NCSA Mosaic, there are many incorrectly
	 * commented documents out there, that have the <!-- comment
	 * start, but expect a different end such as --!> or just >
	 * If we have finished this document, and the hold buffer
	 * has a starting comment, this is probably what happened,
	 * So terminate the starting comment with the next > and
	 * parse on.
	 * Of necessity this may involve recursion for multiple comments
	 */
	if ((doc_data->hold_len > 4)&&(XP_STRNCMP(tptr, "<!--", 4) == 0))
	{
		char *nothing;
		NET_StreamClass *stream;

		/*
		 * Break the comment
		 */
		tptr[2] = 'C';
		XP_UNLOCK_BLOCK((doc_data->hold_buf));

		/*
		 * Reparse this, then call yourself.
		 */
		nothing = (char*) XP_ALLOC(1);
		if (nothing == NULL)
		{
			XP_FREE_BLOCK((doc_data->hold_buf));
			return;
		}
		nothing[0] = '\0';
		stream = doc_data->parser_stream;
		(void)PA_ParseBlock(data_object, nothing, 0);
		XP_FREE(nothing);
		stream->complete(stream->data_object);
		return;
	}
#ifndef LENIENT_END_TAG
	/*
	 * Also due to allowing '>' in quoted attributes, if they forget
	 * to close a quote, and there is no other in the entire document,
	 * we could have held the whole thing looking for the close quote,
	 * and never found the tag.  Skip this malformed tag by dropping
	 * its starting '<' and parsing on.
	 * Of necessity this may involve recursion for multiple errors.
	 */
	else if ((doc_data->hold_len > 3)&&(*tptr ==  '<'))
	{
		char *nothing;
		PA_Tag *tmp_tag;
		NET_StreamClass *stream;

		/*
		 * Push out the '<'
		 */
		nothing = (char*) XP_ALLOC(1);
		if (nothing == NULL)
		{
			XP_FREE_BLOCK((doc_data->hold_buf));
			return;
		}
		nothing[0] = '<';
		tmp_tag = pa_CreateTextTag(doc_data, nothing, 1);
		doc_data->output_tag(data_object, tmp_tag, PA_PARSED);

		/*
		 * Remove the '<' an move up the hold buffer.
		 */
                XP_BCOPY((char *)(tptr + 1), tptr, (doc_data->hold_len - 1));
		doc_data->hold_len--;
		XP_UNLOCK_BLOCK((doc_data->hold_buf));

		/*
		 * reparse the rest.
		 */
		nothing[0] = '\0';
		stream = doc_data->parser_stream;
		(void)PA_ParseBlock(data_object, nothing, 0);
		XP_FREE(nothing);
		stream->complete(stream->data_object);
		return;
	}
#endif /* LENIENT_END_TAG */

        tag = pa_CreateTextTag(doc_data, tptr, doc_data->hold_len);
	doc_data->output_tag(data_object, tag, PA_PARSED);

        XP_UNLOCK_BLOCK((doc_data->hold_buf));
        XP_FREE_BLOCK((doc_data->hold_buf));
        doc_data->hold_len = 0;
        doc_data->hold_buf = NULL;
        doc_data->hold = 0;
    }
    else if (doc_data->hold_buf != NULL)
    {
        XP_FREE_BLOCK((doc_data->hold_buf));
        doc_data->hold_buf = NULL;
        doc_data->hold_size = 0;
    }

    doc_data->output_tag(data_object, NULL, PA_COMPLETE);

    /*
     * free up all the data allocated when this stream was initiated.
     */
    if (doc_data->url != NULL)
    {
        XP_FREE(doc_data->url);
    }
    XP_DELETE(doc_data);
}


/*************************************
 * Function: PA_MDLAbort
 *
 * Description: This is a very important entry point to the parser,
 *      but it will never be called directly.  It will be
 *      placed into the stream returned by PA_BeginParseMDL()
 *      and be called from there.  It tells the parser that the
 *      passed document is aborted, there is no new data,
 *      throw out everything you have and stop parsing.
 *
 * Params: The data_object created and placed in the stream class
 *     in PA_BeginParseMDL().  This contains document specific
 *     parse state information.  Also passed a character message
 *     which is the reason for the abort.
 *
 * Returns: nothing.
 *************************************/
void
PA_MDLAbort(void *data_object, int status)
{
    pa_DocData *doc_data;

    /*
     * This MDL document is complete
     */
    doc_data = (pa_DocData *)data_object;

    /*
     * If we were holding some data we hadn't parsed yet, we need
     * to throw it out now.
     */
    if (doc_data->hold)
    {
        XP_UNLOCK_BLOCK((doc_data->hold_buf));
        XP_FREE_BLOCK((doc_data->hold_buf));
        doc_data->hold_len = 0;
        doc_data->hold_buf = NULL;
        doc_data->hold = 0;
    }
    else if (doc_data->hold_buf != NULL)
    {
        XP_FREE_BLOCK((doc_data->hold_buf));
        doc_data->hold_buf = NULL;
        doc_data->hold_size = 0;
    }

    doc_data->output_tag(data_object, NULL, PA_ABORT);

    /*
     * free up all the data allocated when this stream was initiated.
     */
    if (doc_data->url != NULL)
    {
        XP_FREE(doc_data->url);
    }
    XP_DELETE(doc_data);
}

#ifdef PROFILE
#pragma profile off
#endif

