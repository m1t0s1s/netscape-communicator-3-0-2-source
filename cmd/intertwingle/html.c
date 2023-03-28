/* -*- Mode: C; tab-width: 4 -*-
   html.c --- extract more-or-less plain-text from HTML
   Copyright © 1997 Netscape Communications Corporation, all rights reserved.
   Created: Jamie Zawinski <jwz@netscape.com>, 28-Jul-97
 */

#include "config.h"

extern int MK_OUT_OF_MEMORY;

/* from libmsg/msgutils.c (via libmime/mimestub.c) */
extern int msg_GrowBuffer (uint32 desired_size, uint32 element_size,
			   uint32 quantum, char **buffer, uint32 *size);

/* from libnet/mkutils.c (via libmime/mimestub.c) */
extern char * NET_MakeAbsoluteURL(char * absolute_url, char * relative_url);

/* from libnet/mkutils.c (via libmime/mimestub.c) */
extern int NET_URL_Type (const char *url);


typedef struct {
  const char *string;
  unsigned char latin1_char;
  unsigned char length;
} entity;

static entity entities[] = {
  {"lt", '<', 2},
  {"LT", '<', 2},
  {"gt", '>', 2},
  {"GT", '>', 2},
  {"amp", '&', 3},
  {"AMP", '&', 3},
  {"quot", '\"', 4},
  {"QUOT", '\"', 4},
  {"nbsp", ' ', 4},	/* \240 */
  {"reg", '\256', 3},
  {"REG", '\256', 3},
  {"copy", '\251', 4},
  {"COPY", '\251', 4},

  {"iexcl", '\241', 5},
  {"cent", '\242', 4},
  {"pound", '\243', 5},
  {"curren", '\244', 6},
  {"yen", '\245', 3},
  {"brvbar", '\246', 6},
  {"sect", '\247', 4},
  
  {"uml", '\250', 3},
  {"ordf", '\252', 4},
  {"laquo", '\253', 5},
  {"not", '\254', 3},
  {"shy", '\255', 3},
  {"macr", '\257', 4},

  {"deg", '\260', 3},
  {"plusmn", '\261', 6},
  {"sup2", '\262', 4},
  {"sup3", '\263', 4},
  {"acute", '\264', 5},
  {"micro", '\265', 5},
  {"para", '\266', 4},
  {"middot", '\267', 6},

  {"cedil", '\270', 5},
  {"sup1", '\271', 4},
  {"ordm", '\272', 4},
  {"raquo", '\273', 5},
  {"frac14", '\274', 6},
  {"frac12", '\275', 6},
  {"frac34", '\276', 6},
  {"iquest", '\277', 6},

  {"Agrave", '\300', 6},
  {"Aacute", '\301', 6},
  {"Acirc", '\302', 5},
  {"Atilde", '\303', 6},
  {"Auml", '\304', 4},
  {"Aring", '\305', 5},
  {"AElig", '\306', 5},
  {"Ccedil", '\307', 6},

  {"Egrave", '\310', 6},
  {"Eacute", '\311', 6},
  {"Ecirc", '\312', 5},
  {"Euml", '\313', 4},
  {"Igrave", '\314', 6},
  {"Iacute", '\315', 6},
  {"Icirc", '\316', 5},
  {"Iuml", '\317', 4},

  {"ETH", '\320', 3},
  {"Ntilde", '\321', 6},
  {"Ograve", '\322', 6},
  {"Oacute", '\323', 6},
  {"Ocirc", '\324', 5},
  {"Otilde", '\325', 6},
  {"Ouml", '\326', 4},
  {"times", '\327', 5},

  {"Oslash", '\330', 6},
  {"Ugrave", '\331', 6},
  {"Uacute", '\332', 6},
  {"Ucirc", '\333', 5},
  {"Uuml", '\334', 4},
  {"Yacute", '\335', 6},
  {"THORN", '\336', 5},
  {"szlig", '\337', 5},

  {"agrave", '\340', 6},
  {"aacute", '\341', 6},
  {"acirc", '\342', 5},
  {"atilde", '\343', 6},
  {"auml", '\344', 4},
  {"aring", '\345', 5},
  {"aelig", '\346', 5},
  {"ccedil", '\347', 6},

  {"egrave", '\350', 6},
  {"eacute", '\351', 6},
  {"ecirc", '\352', 5},
  {"euml", '\353', 4},
  {"igrave", '\354', 6},
  {"iacute", '\355', 6},
  {"icirc", '\356', 5},
  {"iuml", '\357', 4},

  {"eth", '\360', 3},
  {"ntilde", '\361', 6},
  {"ograve", '\362', 6},
  {"oacute", '\363', 6},
  {"ocirc", '\364', 5},
  {"otilde", '\365', 6},
  {"ouml", '\366', 4},
  {"divide", '\367', 6},

  {"oslash", '\370', 6},
  {"ugrave", '\371', 6},
  {"uacute", '\372', 6},
  {"ucirc", '\373', 5},
  {"uuml", '\374', 4},
  {"yacute", '\375', 6},
  {"thorn", '\376', 5},
  {"yuml", '\377', 4},
};


static char
get_entity (const char *string, int length)
{
  int i;
  char c = *string;
  for (i = 0; i < (sizeof(entities)/sizeof(*entities)); i++)
    if (length == entities[i].length &&
		c == entities[i].string[0] &&
		!strncmp(string, entities[i].string, length))
      return entities[i].latin1_char;
  return 0;
}


static int
expand_url (const char *url, int32 size, const char *base,
	    char *output)
{
  if (!base)
    {
      memcpy (output, url, size);
      output[size] = 0;
      return size;
    }
  else
    {
      char *u = (char *) malloc(size+1);
      char *s;
      if (!u) return MK_OUT_OF_MEMORY;
      memcpy(u, url, size);
      u[size] = 0;
      s = NET_MakeAbsoluteURL((char *) base, u);
      free (u);
      if (!s) return MK_OUT_OF_MEMORY;
      strcpy (output, s);
	  free (s);
      return strlen (output);
    }
}


/* Strips HTML tags out of the given buffer, modifying it in place (since
   the resultant text will be the same size or smaller.)

   Any HREFs in the text will be returned to `urls_return' -- this
   will be a double-null-terminated list of URLs of the form
   "url-A\000url-b\000url-C\000\000".

   If `base' is provided, it is the default base for relative URLs in the
   document; any <BASE> tag in the document will override this.

   Returns negative on failure (but `buf' may already have been destroyed.)
 */
int
html_to_plaintext (char *buf, int32 *sizeP,
				   char **baseP,
				   char **urls_return)
{
  int status = 0;
  int32 size = *sizeP;
  const char *end = buf + size;
  char *in, *out;

  char *def_base = (baseP ? *baseP : 0);
  char *base = 0;
  char *urls = 0;
  int32 urls_size = 0;
  int32 urls_fp = 0;

  if (size < 3) return 0;

  /* First pass: strip out tags and collect URLs.
   */
  out = buf;
  for (in = buf; in < end; in++)
    {
      if (*in != '<')
		*out++ = *in;
      else
		{
		  char *tend;
		  XP_Bool base_p = FALSE;
		  XP_Bool href_p = FALSE;

		  /* Find the tag name... */
		  in++;
		  /* Skip over whitespace after <. */
		  while (in < end && isspace(*in)) in++;

		  /* Skip over token-constitutents to find end of tag. */
		  tend = in;
		  while (tend < end &&
				 !isspace(*tend) &&
				 *tend != '<' && *tend != '>' &&
				 *tend != '"' && *tend != '\'' &&
				 *tend != '&' && *tend != ';' &&
				 *tend != '=')
			tend++;

		  if (tend-in == 1 && (*in == 'a' || *in == 'A'))
			href_p = TRUE;
		  else if (tend-in == 4 && !strncasecmp(in, "BASE", 4))
			base_p = TRUE;
		  else if (tend-in >= 3 && !strncasecmp(in, "!--", 3))
			{
			  in = tend;
			  while (in < end)
				{
				  if (in[0] == '-' && in[1] == '-' && in[2] == '>')
					{
					  in += 2;
					  break;
					}
				  else
					in++;
				}
			  continue;
			}
		  in = tend;

		  /* Swallow the tag arguments... */
		  while (in < end)
			{
			  char *value_start = 0;
			  char *value_end = 0;
			  Bool got_it = FALSE;

			  /* Skip over whitespace before parameter. */
			  while (in < end && isspace(*in)) in++;

			  if (in < end && *in == '/') in++;

			  /* Skip over token-constitutents to find end of a parameter. */
			  tend = in;
			  while (tend < end &&
					 !isspace(*tend) &&
					 *tend != '<' && *tend != '>' &&
					 *tend != '"' && *tend != '\'' &&
					 *tend != '&' && *tend != ';' &&
					 *tend != '=')
				tend++;

			  if ((base_p || href_p) &&
				  (tend-in == 4 && !strncasecmp(in, "HREF", 4)))
				got_it = TRUE;

			  /* Skip over whitespace after parameter. */
			  in = tend;
			  while (in < end && isspace(*in)) in++;

			  if (*in == '>')
				break;
			  else if (*in != '=')
				continue;
			  else
				{
				  /* Skip over whitespace after equal-sign. */
				  in++;
				  while (in < end && isspace(*in)) in++;
				}

			  if (*in == '>')
				break;
			  else if (*in == '"')
				{
				  in++;
				  value_start = in;
				  while (in < end && *in != '"') in++;
				  value_end = in;
				  if (in < end) in++;
				}
			  else if (*in == '\'')
				{
				  in++;
				  value_start = in;
				  while (in < end && *in != '\'') in++;
				  value_end = in;
				  if (in < end) in++;
				}
			  else
				{
				  value_start = in;
				  while (in < end && *in != '>' && !isspace(*in)) in++;
				  value_end = in;
				  if (!isspace(*in))
					value_end++;
				  if (*in == '>') value_end--;
				}

			  if (got_it)
				{
				  if (base_p)
					{
					  if (base) free (base);
					  base = (char *) malloc (value_end - value_start + 1);
					  if (!base)
						{
						  status = MK_OUT_OF_MEMORY;
						  goto FAIL;
						}
					  memcpy(base, value_start, value_end - value_start);
					  base[value_end-value_start] = 0;
					}
				  else /* if (href_p) */
					{
					  const char *b = (base ? base :
									   (def_base ? def_base : ""));
					  int32 dsize = (urls_fp + strlen(b) +
									 (value_end - value_start) + 10);
					  if (urls_size < dsize)
						{
						  status = msg_GrowBuffer (dsize, sizeof(char), 512,
												   &urls, &urls_size);
						  if (status < 0) goto FAIL;
						}
					  status = expand_url (value_start, value_end-value_start,
										   b, urls+urls_fp);
					  if (status < 0) goto FAIL;
					  urls_fp += status+1;
					  urls[urls_fp] = 0;
					}
				}
			}

		  /* Convert each tag to a single space. */
		  if (out > buf && out[-1] != ' ')
			*out++ = ' ';
		}
    }

  size = out-buf;
  end = buf+size;


  /* Second pass: strip out ampersand entities.
   */
  out = buf;
  for (in = buf; in < end; in++)
    {
      if (*in != '&')
		*out++ = *in;
      else
		{
		  char *eend = in + 1;
		  while (*eend &&
				 *eend != ';' &&
				 *eend != '&' &&
				 !isspace(*eend) &&
				 eend < in + 10)
			eend++;
		  if (*eend == ';' || *eend == '&' || isspace(*eend))
			{
			  char e = get_entity(in+1, eend-(in+1));
			  if (e)
				*out++ = e;
			  if (*eend == ';')
				eend++;
			  in = eend-1;
			}
		}
    }

 FAIL:
  if (status > 0 && baseP)
	*baseP = base;
  else if (base)
	free (base);

  if (status >= 0 && urls_return)
    *urls_return = urls;
  else if (urls)
    free (urls);

  *sizeP = (out - buf);

  return status;
}


/* Strips out overstriking/underlining (for example "_^Hf_^Ho_^Ho" => "foo".)
 */
int
strip_overstriking (char *buf, int32 *sizeP)
{
  int32 size = *sizeP;
  const char *end = buf + size;
  char *in, *out;

  out = buf;
  for (in = buf; in < end; in++)
	if (*in != '\010')
	  *out++ = *in;
	else if (out > buf)
	  {
		out--;
		size--;
	  }
  *sizeP = size;
  return 0;
}


/* Searches for URLs in text/plain documents; returns the same format as
   html_to_plaintext() does.

   Liberally cribbed from libnet/mkutils.c: NET_ScanForURLs().
 */
int
search_for_urls (const char *buf, int32 size,
				 char **result,
				 int32 *result_size,
				 int32 *result_fp)
{
  const char *end = buf + size;
  const char *cp;

  for (cp = buf; cp < end; cp++)
	{
	  /* if NET_URL_Type returns true then it is most likely a URL
		 But only match protocol names if at the very beginning of
		 the string, or if the preceeding character was not alphanumeric;
		 this lets us match inside "---HTTP://XXX" but not inside of
		 things like "NotHTTP://xxx"
	   */
	  int type = 0;
	  if (!isspace(*cp) &&
		  (cp == buf || (!isalpha(cp[-1]) && !isdigit(cp[-1]))) &&
		  (type = NET_URL_Type(cp)) != 0)
		{
		  const char *cp2;
		  for (cp2 = cp; cp2 < end; cp2++)
			{
			  /* These characters always mark the end of the URL. */
			  if (isspace(*cp2) ||
				  *cp2 == '<' || *cp2 == '>' ||
				  *cp2 == '`' || *cp2 == ')' ||
				  *cp2 == '\'' || *cp2 == '"' ||
				  *cp2 == ']' || *cp2 == '}')
				break;
			}

		  /* Check for certain punctuation characters on the end, and strip
			 them off. */
		  while (cp2 > cp && 
				 (cp2[-1] == '.' || cp2[-1] == ',' || cp2[-1] == '!' ||
				  cp2[-1] == ';' || cp2[-1] == '-' || cp2[-1] == '?' ||
				  cp2[-1] == '#'))
			cp2--;

		  /* if the url is less than 7 characters then we screwed up
		   * and got a "news:" url or something which is worthless
		   * to us.  Exclude the A tag in this case.
		   *
		   * Also exclude any URL that ends in a colon; those tend
		   * to be internal and magic and uninteresting.
		   *
		   * And also exclude the builtin icons, whose URLs look
		   * like "internal-gopher-binary".
		   */
		  if (cp2-cp < 7 ||
			  (cp2 > cp && cp2[-1] == ':') ||
			  !strncmp(cp, "internal-", 9))
			{
			  continue;
			}
		  else
			{
			  int32 dsize = ((*result_fp) + (cp2 - cp) + 10);
			  if (*result_size < dsize)
				{
				  int status = msg_GrowBuffer (dsize, sizeof(char), 512,
											   result, result_size);
				  if (status < 0)
					{
					  if (*result) free (*result);
					  *result = 0;
					  return MK_OUT_OF_MEMORY;
					}
				}

			  memcpy ((*result) + (*result_fp), cp, cp2-cp);
			  (*result_fp) += cp2-cp;
			  (*result)[(*result_fp)++] = 0;
			  (*result)[(*result_fp)] = 0;
			}

		  cp = cp2-1;  /* go to next word */
		}
	}

  return 0;
}


/* Searches through plaintext for anything that looks like an email address
   or a message ID.
 */
int
search_for_addrs_or_ids (const char *buf, int32 size,
						 char **result,
						 int32 *result_size,
						 int32 *result_fp)
{
  const char *end = buf + size;
  const char *cp;

  for (cp = buf; cp < end; cp++)
	if (*cp == '@')
	  {
		/* We've found an at-sign.  See if the thing around it looks like
		   it might be a message ID or email address. */

		const char *astart = 0, *aend = 0;
		const char *s;

		/* Skip forward to the end of this token. */
		for (aend = cp+1; aend < end; aend++)
		  if (isspace(*aend) ||
			  *aend == '<' || *aend == '>' ||
			  *aend == '[' || *aend == ']' ||
			  *aend == '{' || *aend == '}' ||
			  *aend == '(' || *aend == ')' ||
			  *aend == ',' || *aend == ';' ||
			  *aend == '!' || *aend == '?' ||
			  *aend == '`' || *aend == '\'' ||
			  *aend == '\"' || *aend == '@' ||
			  *aend == ':' || *aend == '=')
			break;

		/* Strip off trailing punctuation. */
		while ((aend-1) > cp && !isalnum(aend[-1]))
		  aend--;

		/* If there are any characters in the token that aren't domain-name-
		   contituents, give up on this one. */
		{
		  XP_Bool got_dot = FALSE;

		  for (s = cp+1; s < aend; s++)
			if (*s == '.')
			  got_dot = TRUE;
			else if (!isalnum(*s) && *s != '_' && *s != '-')
			  break;

		  if (!got_dot || s != aend)
			{
			  cp = aend;
			  continue;
			}
		}


		/* Ok, what's after the @ could possibly be a host name.
		   Now let's look at what's before the @ and see if it
		   could be a user-id or msg-id.
		 */
		/* Skip forward to the end of this token. */
		for (astart = cp-1; astart >= buf; astart--)
		  if (isspace(*astart) ||
			  *astart == '<' || *astart == '>' ||
			  *astart == '[' || *astart == ']' ||
			  *astart == '{' || *astart == '}' ||
			  *astart == '(' || *astart == ')' ||
			  *astart == ',' || *astart == ';' ||
			  *astart == '!' || *astart == '?' ||
			  *astart == '`' || *astart == '\'' ||
			  *astart == '\"' || *astart == '@' ||
			  *astart == ':' || *astart == '=')
			break;

		/* Strip off leading punctuation. */
		while (astart < cp && !isalnum(*astart))
		  astart++;

#if 0
		/* If there are any characters in the token that aren't user-name-
		   contituents, give up on this one. */
		for (s = astart; s < cp; s++)
		  if (!isalnum(*s) && *s != '.' && *s != '_' && *s != '-')
			break;

		if (s != cp)
		  {
			cp = aend;
			continue;
		  }
#endif

		if (astart < cp-2 && aend > cp+3)
		  {
			/* At this point, we've found one.  Save it. */

			  int32 dsize = ((*result_fp) + (aend - astart) + 10);
			  if (*result_size < dsize)
				{
				  int status = msg_GrowBuffer (dsize, sizeof(char), 512,
											   result, result_size);
				  if (status < 0)
					{
					  if (*result) free (*result);
					  *result = 0;
					  return MK_OUT_OF_MEMORY;
					}
				}

			  memcpy ((*result) + (*result_fp), astart, aend-astart);
			  (*result_fp) += aend - astart;
			  (*result)[(*result_fp)++] = 0;
			  (*result)[(*result_fp)] = 0;
		  }

		cp = aend;
	  }

  return 0;
}
