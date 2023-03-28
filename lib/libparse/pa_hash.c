/*
** This is a generated file, do not edit it.  If you need to make changes,
** edit the file pa_hash.template and re-build pa_hash.c on a UNIX machine.
** This whole hacky pile of poop was done by Michael Toy.
*/

#include "pa_parse.h"
#define TOTAL_KEYWORDS 91
#define MIN_WORD_LENGTH 1
#define MAX_WORD_LENGTH 11
#define MIN_HASH_VALUE 1
#define MAX_HASH_VALUE 345
/* maximum key range = 345, duplicates = 0 */

#define MYLOWER(x) TOLOWER(((x) & 0x7f))

/*************************************
 * Function: pa_tokenize_tag
 *
 * Description: This function maps the passed in string
 * 		to one of the valid tag element tokens, or to
 *		the UNKNOWN token.
 *
 * Params: Takes a \0 terminated string.
 *
 * Returns: a 32 bit token to describe this tag element.  On error,
 * 	    which means it was not passed an unknown tag element string,
 * 	    it returns the token P_UNKNOWN.
 *
 * Performance Notes:
 * Profiling on mac revealed this routine as a big (5%) time sink.
 * This function was stolen from pa_mdl.c and merged with the perfect
 * hashing code and the tag comparison code so it would be flatter (fewer
 * function calls) since those are still expensive on 68K and x86 machines.
 *************************************/

intn
pa_tokenize_tag(char *str)
{
  static unsigned short asso_values[] =
    {
     346, 346, 346, 346, 346, 346, 346, 346, 346, 346,
     346, 346, 346, 346, 346, 346, 346, 346, 346, 346,
     346, 346, 346, 346, 346, 346, 346, 346, 346, 346,
     346, 346, 346, 346, 346, 346, 346, 346, 346, 346,
     346, 346, 346, 346, 346, 346, 346, 346, 346,   5,
      10,  45,  20,  25,  30, 346, 346, 346, 346, 346,
     346, 346, 346, 346, 346, 346, 346, 346, 346, 346,
     346, 346, 346, 346, 346, 346, 346, 346, 346, 346,
     346, 346, 346, 346, 346, 346, 346, 346, 346, 346,
     346, 346, 346, 346, 346, 346, 346,   0,  10,  25,
      65,   0,  15,  20,  92, 120, 346,  35,  40,  35,
       0,  45,   5, 346,   0,  40, 110,  47,  15,  10,
      25,   0, 346, 346, 346, 346, 346, 346,
    };
  static unsigned char lengthtable[] =
    {
      0,  1,  0,  0,  4,  0,  0,  0,  3,  0,  0,  1,  2,  8,
      0,  0,  0,  0,  3,  0,  5,  1,  0,  3,  0,  0,  0,  0,
      0,  0,  0,  6,  0,  0,  0,  0, 11,  7,  0,  0,  0,  6,
      0,  0,  0,  5,  6,  0,  3,  0,  0,  6,  0,  0,  4,  0,
      0,  0,  0,  4,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
      0,  0,  2,  3,  9, 10,  0,  0,  0,  0,  0,  1,  0,  0,
      4,  0,  4,  0,  0,  0,  0,  0,  0,  0,  2,  1,  0,  0,
      0,  4,  3,  4,  0,  0,  2, 10,  0,  0,  8,  4,  3,  0,
      2,  0,  2,  5,  0,  7,  0,  0,  5,  0,  0,  8,  4,  5,
      6,  2,  6,  2,  0,  0,  0,  8,  2,  0,  0,  0,  0,  4,
      0,  0,  0,  8,  2,  0,  0,  2,  0,  4,  0,  0,  0,  0,
      2,  0,  6,  0,  0,  0,  5,  4,  0,  0,  9,  0,  6,  0,
      8,  0,  8,  0,  0,  3,  4,  0,  6,  7,  3,  0,  0,  6,
      0,  0,  2,  0,  0,  0,  3,  0,  0,  0,  0,  0,  0,  0,
      6,  2,  3,  4,  0,  0,  0,  8,  0,  0,  0,  0,  0,  0,
      5,  0,  0,  0,  0,  0,  0,  0,  3,  0,  0,  0,  0,  0,
      0,  0,  0,  7,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
      0,  0,  5,  1,  2,  0,  0,  0,  0,  0,  0,  0,  0,  0,
      0,  0,  0,  0,  0,  0,  0,  4,  0,  0,  0,  0,  0,  0,
      0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
      0,  4,  2,  0,  0,  0,  0,  2,  0,  0,  0,  0,  0,  0,
      0,  0,  2,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
      0,  0,  0,  0,  7,  0,  0,  0,  0,  0,  0,  0,  0,  0,
      0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  2,  0,  0,  0,
      0,  0,  0,  0,  0,  0,  0,  0,  0,  5,
    };
  static struct pa_TagTable wordlist[] =
    {
      {"",}, 
      {"a",  P_ANCHOR},
      {"",}, {"",}, 
      {"area",  P_AREA},
      {"",}, {"",}, {"",}, 
      {"pre",  P_PREFORMAT},
      {"",}, {"",}, 
      {"p",  P_PARAGRAPH},
      {"br",  P_LINEBREAK},
      {"payorder",  P_PAYORDER},
      {"",}, {"",}, {"",}, {"",}, 
      {"var",  P_VARIABLE},
      {"",}, 
      {"frame",  P_GRID_CELL},
      {"b",  P_BOLD},
      {"",}, 
      {"wbr",  P_WORDBREAK},
      {"",}, {"",}, {"",}, {"",}, {"",}, {"",}, {"",}, 
      {"center",  P_CENTER},
      {"",}, {"",}, {"",}, {"",}, 
      {"certificate",  P_CERTIFICATE},
      {"caption",  P_CAPTION},
      {"",}, {"",}, {"",}, 
      {"keygen",  P_KEYGEN},
      {"",}, {"",}, {"",}, 
      {"param",  P_PARAM},
      {"server",  P_SERVER},
      {"",}, 
      {"map",  P_MAP},
      {"",}, {"",}, 
      {"spacer",  P_SPACER},
      {"",}, {"",}, 
      {"base",  P_BASE},
      {"",}, {"",}, {"",}, {"",}, 
      {"nobr",  P_NOBREAK},
      {"",}, {"",}, {"",}, {"",}, {"",}, {"",}, {"",}, {"",}, {"",}, 
      {"",}, {"",}, {"",}, 
      {"em",  P_EMPHASIZED},
      {"xmp",  P_PLAIN_PIECE},
      {"nscp_open",  P_NSCP_OPEN},
      {"nscp_close",  P_NSCP_CLOSE},
      {"",}, {"",}, {"",}, {"",}, {"",}, 
      {"s",  P_STRIKE},
      {"",}, {"",}, 
      {"samp",  P_SAMPLE},
      {"",}, 
      {"menu",  P_MENU},
      {"",}, {"",}, {"",}, {"",}, {"",}, {"",}, {"",}, 
      {"hr",  P_HRULE},
      {"u",  P_UNDERLINE},
      {"",}, {"",}, {"",}, 
      {"form",  P_FORM},
      {"sup",  P_SUPER},
      {"hype",  P_HYPE},
      {"",}, {"",}, 
      {"h1",  P_HEADER_1},
      {"blockquote",  P_BLOCKQUOTE},
      {"",}, {"",}, 
      {"noframes",  P_NOGRIDS},
      {"cell",  P_CELL},
      {"sub",  P_SUB},
      {"",}, 
      {"tr",  P_TABLE_ROW},
      {"",}, 
      {"h2",  P_HEADER_2},
      {"embed",  P_EMBED},
      {"",}, 
      {"noembed",  P_NOEMBED},
      {"",}, {"",}, 
      {"small",  P_SMALL},
      {"",}, {"",}, 
      {"colormap",  P_COLORMAP},
      {"body",  P_BODY},
      {"table",  P_TABLE},
      {"applet",  P_JAVA_APPLET},
      {"ol",  P_NUM_LIST},
      {"subdoc",  P_SUBDOC},
      {"ul",  P_UNUM_LIST},
      {"",}, {"",}, {"",}, 
      {"frameset",  P_GRID},
      {"h4",  P_HEADER_4},
      {"",}, {"",}, {"",}, {"",}, 
      {"code",  P_CODE},
      {"",}, {"",}, {"",}, 
      {"textarea",  P_TEXTAREA},
      {"h5",  P_HEADER_5},
      {"",}, {"",}, 
      {"dl",  P_DESC_LIST},
      {"",}, 
      {"meta",  P_META},
      {"",}, {"",}, {"",}, {"",}, 
      {"h6",  P_HEADER_6},
      {"",}, 
      {"strike",  P_STRIKEOUT},
      {"",}, {"",}, {"",}, 
      {"image",  P_NEW_IMAGE},
      {"head",  P_HEAD},
      {"",}, {"",}, 
      {"plaintext",  P_PLAIN_TEXT},
      {"",}, 
      {"option",  P_OPTION},
      {"",}, 
      {"basefont",  P_BASEFONT},
      {"",}, 
      {"multicol",  P_MULTICOLUMN},
      {"",}, {"",}, 
      {"big",  P_BIG},
      {"font",  P_FONT},
      {"",}, 
      {"strong",  P_STRONG},
      {"address",  P_ADDRESS},
      {"kbd",  P_KEYBOARD},
      {"",}, {"",}, 
      {"script",  P_SCRIPT},
      {"",}, {"",}, 
      {"h3",  P_HEADER_3},
      {"",}, {"",}, {"",}, 
      {"dir",  P_DIRECTORY},
      {"",}, {"",}, {"",}, {"",}, {"",}, {"",}, {"",}, 
      {"select",  P_SELECT},
      {"dd",  P_DESC_TEXT},
      {"img",  P_IMAGE},
      {"link",  P_LINK},
      {"",}, {"",}, {"",}, 
      {"noscript",  P_NOSCRIPT},
      {"",}, {"",}, {"",}, {"",}, {"",}, {"",}, 
      {"blink",  P_BLINK},
      {"",}, {"",}, {"",}, {"",}, {"",}, {"",}, {"",}, 
      {"div",  P_DIVISION},
      {"",}, {"",}, {"",}, {"",}, {"",}, {"",}, {"",}, {"",}, 
      {"listing",  P_LISTING_TEXT},
      {"",}, {"",}, {"",}, {"",}, {"",}, {"",}, {"",}, {"",}, {"",}, 
      {"",}, {"",}, {"",}, 
      {"input",  P_INPUT},
      {"i",  P_ITALIC},
      {"td",  P_TABLE_DATA},
      {"",}, {"",}, {"",}, {"",}, {"",}, {"",}, {"",}, {"",}, {"",}, 
      {"",}, {"",}, {"",}, {"",}, {"",}, {"",}, {"",}, 
      {"cite",  P_CITATION},
      {"",}, {"",}, {"",}, {"",}, {"",}, {"",}, {"",}, {"",}, {"",}, 
      {"",}, {"",}, {"",}, {"",}, {"",}, {"",}, {"",}, {"",}, {"",}, 
      {"",}, {"",}, {"",}, 
      {"html",  P_HTML},
      {"li",  P_LIST_ITEM},
      {"",}, {"",}, {"",}, {"",}, 
      {"dt",  P_DESC_TITLE},
      {"",}, {"",}, {"",}, {"",}, {"",}, {"",}, {"",}, {"",}, 
      {"th",  P_TABLE_HEADER},
      {"",}, {"",}, {"",}, {"",}, {"",}, {"",}, {"",}, {"",}, {"",}, 
      {"",}, {"",}, {"",}, {"",}, {"",}, {"",}, 
      {"isindex",  P_INDEX},
      {"",}, {"",}, {"",}, {"",}, {"",}, {"",}, {"",}, {"",}, {"",}, 
      {"",}, {"",}, {"",}, {"",}, {"",}, {"",}, {"",}, {"",}, {"",}, 
      {"",}, {"tt",  P_FIXED},
      {"",}, {"",}, {"",}, {"",}, {"",}, {"",}, {"",}, {"",}, {"",}, 
      {"",}, {"",}, {"",}, 
      {"title",  P_TITLE},
    };

  if (str != NULL)
  {
    int len = strlen(str);
    if (len <= MAX_WORD_LENGTH && len >= MIN_WORD_LENGTH)
    {
  register int hval = len;

  switch (hval)
    {
      default:
      case 3:
        hval += asso_values[MYLOWER(str[2])];
      case 2:
        hval += asso_values[MYLOWER(str[1])];
      case 1:
        hval += asso_values[MYLOWER(str[0])];
        break;
    }
  hval += asso_values[MYLOWER(str[len - 1])];
      if (hval <= MAX_HASH_VALUE && hval >= MIN_HASH_VALUE)
      {
	if (len == lengthtable[hval])
	{
	  register char *tag = wordlist[hval].name;
	  /*
	  ** The following code was stolen from pa_TagEqual,
	  ** again to make this function flatter.
	  */

	  /*
	  ** While not at the end of the string, if they ever differ
	  ** they are not equal.  We know "tag" is already lower case.
	  */
	  while ((*tag != '\0')&&(*str != '\0'))
	  {
	    if (*tag != (char) TOLOWER(*str))
	      return(P_UNKNOWN);
	    tag++;
	    str++;
	  }
	  /*
	  ** One of the strings has ended, if they are both ended, then they
	  ** are equal, otherwise not.
	  */
	  if ((*tag == '\0')&&(*str == '\0'))
	    return wordlist[hval].id;
	}
      }
    }
  }

#ifdef DEBUG_jwz
  /* Gosh, I think I can live with this efficiency hit in the case of
     unknown tags. */
  if (str[0] == 's' && !strcmp(str, "style"))
    /* Ignore text inside of <STYLE> by making it a synonym of <TITLE>
       (since subsequent TITLE tags are ignored.)
       I can't find another tag that will discard the body text:
       NOFRAMES seems likely, but it only works inside a FRAMESET.
       And NOSCRIPT is activated if JS is turned off...
       <SCRIPT LANGUAGE=NONE> works, but I don't know how to insert
       a parameter in the tag (and it defaults to JavaScript.)
    */
    return P_TITLE;
#endif /* DEBUG_jwz */

  return(P_UNKNOWN);
}
