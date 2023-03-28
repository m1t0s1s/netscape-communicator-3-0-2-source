#include "pa_parse.h"
#include <stdio.h>
#include "pa_amp.h"

#include "libi18n.h"

#ifdef PROFILE
#pragma profile on
#endif


/*************************************
 * Function: pa_map_escape
 *
 * Description: This function maps the passed in amperstand
 * 		escape sequence to a single 8 bit char (ISO 8859-1).
 *
 * Params: Takes a buffer, and a length for that buffer. (buf is NOT
 *         a \0 terminated string).  Also takes a pointer to an escape length
 *	   which is home much of the original buffer was used.
 *	   And an out buffer, and it's outbuflen.
 *
 * Returns: On error, which means it was not passed
 *          a valid amperstand escape sequence, it stores the
 *	    char value '\0' in "out".  It also returns the length of
 *	    the part of the passed buffer that was replaced.
 *	    Also, the number of bytes written to the out buffer (outlen).
 *************************************/
static void
pa_map_escape(char *buf, int32 len, int32 *esc_len, unsigned char *out,
	uint16 outbuflen, uint16 *outlen, Bool force)
{
	char val;
	int32 cnt;

	*esc_len = 0;
	val = 0;

	/*
	 * Skip the amperstand character at the start.
	 */
	buf++;
	len--;

	/*
	 * Ampersand followed by number sign specifies the decimal
	 * value of the character to be placed here.
	 */
	if (*buf == '#')
	{
		INTL_NumericCharacterReference((unsigned char *) (buf + 1),
			(uint32) (len - 1), (uint32 *) esc_len, out, outbuflen,
			outlen, force);
		*esc_len += 1;
	}
	/*
	 * Else we have to look this escape up in
	 * the array of valid escapes.
	 */
	else
	{
		/*
		 * Since we are searching on the PA_AmpEscapes[].str string,
		 * and not on the escape entered, this lets us partial
		 * match if PA_AmpEscapes[].str matches a prefix of the escape.
		 * This means no one PA_AmpEscapes[].str should be a prefix
		 * of any other PA_AmpEscapes[].str.
		 */
		cnt = 0;
		while (PA_AmpEscapes[cnt].str != NULL)
		{
			if (XP_STRNCMP(buf, PA_AmpEscapes[cnt].str,
				PA_AmpEscapes[cnt].len) == 0)
			{
				val = PA_AmpEscapes[cnt].value;
				*esc_len = PA_AmpEscapes[cnt].len;
				break;
			}
			cnt++;
		}

		/*
		 * for invalid escapes, return '\0'
		 */
		if (PA_AmpEscapes[cnt].str == NULL)
		{
			val = '\0';
		}

		INTL_EntityReference((unsigned char) val, out, outbuflen,
			outlen);
	}
}


/*************************************
 * Function: pa_ExpandEscapes
 *
 * Description: Go through the passed in buffer of text, find and replace
 *		all amperstand escape sequences with their character
 * 		equivilants.  The buffer is modified in place,
 * 		each escape replacement shortens the contents of the
 * 		buffer.
 *
 * Params: Takes a buffer, and a length for that buffer. (buf is NOT
 *         a \0 terminated string).  Also takes a pointer to a new length.
 *         This is the new (possibly shorter) length of the buffer
 *         after the escapes have been replaced.  Finally takes a boolean
 *	   that specifies whether partial escapes are allowed or not.
 *
 * Returns: The new length of the string in the nlen parameter. Also
 * 	    returns a NULL character pointer on success.  
 *          If the buffer passed contained a possibly truncated
 *	    amperstand escape, that is considered a partial success.
 *	    Escapes are replace in all the buffer up to the truncated
 *	    escape, nlen is the length of the new sub-buffer, and
 * 	    the return code is a pointer to the begining of the truncated
 * 	    escape in the original buffer.
 *************************************/

char *
pa_ExpandEscapes(char *buf, int32 len, int32 *nlen, Bool force)
{
	int32 cnt;
	char *from_ptr;
	char *to_ptr;
	unsigned char out[10];
	uint16 outlen;
	unsigned char *p;

	*nlen = 0;

	/*
	 * A NULL buffer translates to a NULL buffer.
	 */
	if (buf == NULL)
	{
		return(NULL);
	}

	/*
	 * Look through the passed buf for the beginning of amperstand
	 * escapes.  They begin with an amperstand followed by a letter
	 * or a number sign.
	 */
	from_ptr = buf;
	to_ptr = buf;
	cnt = 0;
	while (cnt < len)
	{
		char *tptr2;
		int32 cnt2;
		int32 esc_len;

		if (*from_ptr == '&')
		{
			/*
			 * If this is the last character in the string
			 * we don't know if it is an amperstand escape yet
			 * or not.  We want to save it until the next buffer
			 * to make sure.  However, if force is true this
			 * can't be a partial escape, so asuume an error
			 * and cope as best you can.
			 */
			if ((cnt + 1) == len)
			{
				if (force != FALSE)
				{
					*to_ptr++ = *from_ptr++;
					cnt++;
					continue;
				}
				/*
				 * This might be a partial amperstand escape.
				 * we will have to wait until we have
				 * more data.  Break the while loop
				 * here.
				 */
				break;
			}
			/*
			 * else is the next character is a letter or
			 * a '#' , this should be some kind of amperstand
			 * escape.
			 */
			else if ((XP_IS_ALPHA(*(from_ptr + 1)))||
				(*(from_ptr + 1) == '#'))
			{
				/*
				 * This is an amperstand escape!  Drop
				 * through to the rest of the while loop.
				 */
			}
			/*
			 * else this character does not begin an
			 * amperstand escape increment pointer, and go
			 * back to the start of the while loop.
			 */
			else
			{
				*to_ptr++ = *from_ptr++;
				cnt++;
				continue;
			}
		}
		/*
		 * else this character does not begin an amperstand escape
		 * increment pointer, and go back to the start of the
		 * while loop.
		 */
		else
		{
			*to_ptr++ = *from_ptr++;
			cnt++;
			continue;
		}

		/*
		 * If we got here, it is because we think we have
		 * an amperstand escape.  Amperstand escapes are of arbitrary
		 * length, terminated by either a semi-colon, or white space.
		 *
		 * Find the end of the amperstand escape.
		 */
		tptr2 = from_ptr;
		cnt2 = cnt;
		while (cnt2 < len)
		{
			if ((*tptr2 == ';')||(XP_IS_SPACE(*tptr2)))
			{
				break;
			}
			tptr2++;
			cnt2++;
		}
		/*
		 * We may have failed to find the end because this is
		 * only a partial amperstand escape.  However, if force is
		 * true this can't be a partial escape, so asuume and error
		 * and cope as best you can.
		 */
		if ((cnt2 == len)&&(force == FALSE))
		{
			/*
			 * This might be a partial amperstand escape.
			 * we will have to wait until we have
			 * more data.  Break the while loop
			 * here.
			 */
			break;
		}

		esc_len = 0;
		pa_map_escape(from_ptr, (intn)(tptr2 - from_ptr + 1),
			&esc_len, out, sizeof(out), &outlen, force);
		if (esc_len > (tptr2 - from_ptr + 1))
		{
			/*
			 * This is a partial numeric char ref.
			 */
			break;
		}
		/*
		 * invalid escape sequences return the end of string
		 * character.  Treat this not as an amperstand escape, but as
		 * normal text.
		 */
		if (out[0] == '\0')
		{
			*to_ptr++ = *from_ptr++;
			cnt++;
			continue;
		}

		/*
		 * valid escape sequences are replaced in place, with the
		 * rest of the buffer copied forward.  For improper escape
		 * sequences that we think we can recover from, we may have
		 * just used part of the beginning of the escape sequence.
		 */
		p = out;
		while (outlen--)
		{
			*to_ptr++ = *p++;
		}
		tptr2 = (char *)(from_ptr + esc_len + 1);
		cnt2 = cnt + esc_len + 1;

		/*
		 * for semi-colon terminated escapes, eat the semi-colon.
		 * The test on cnt2 prevents up walking off the end of the
		 * buffer on a broken escape we are coping with.
		 */
		if ((cnt2 < len)&&(*tptr2 == ';'))
		{
			tptr2++;
			cnt2++;
		}
		from_ptr = tptr2; 
		cnt = cnt2;
	}

	/*
	 * If we broke the while loop early,
	 * it is because we may be holding a
	 * partial amperstand escape.
	 * return a pointer to that partial escape.
	 */
	if (cnt < len)
	{
		*nlen = (int32)(to_ptr - buf);
		return(from_ptr);
	}
	else
	{
		*nlen = (int32)(to_ptr - buf);
		return(NULL);
	}
}

#ifdef PROFILE
#pragma profile off
#endif

