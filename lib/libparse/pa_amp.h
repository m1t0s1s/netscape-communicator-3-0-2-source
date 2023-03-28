#include "xp.h"

typedef struct PA_AmpEsc_struct {
        char *str;
        char value;
        intn len;
} PA_AmpEsc;

#ifndef XP_MAC
static PA_AmpEsc PA_AmpEscapes[] = {
	{"lt", '<', 2},
	{"LT", '<', 2},
	{"gt", '>', 2},
	{"GT", '>', 2},
	{"amp", '&', 3},
	{"AMP", '&', 3},
	{"quot", '\"', 4},
	{"QUOT", '\"', 4},
	{"nbsp", '\240', 4},
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

	{NULL, '\0', 0},
};
#else /* ! XP_MAC */
							/* Entities encoded in MacRoman.  */
static PA_AmpEsc PA_AmpEscapes[] = {
	{"lt", '<', 2},
	{"LT", '<', 2},
	{"gt", '>', 2},
	{"GT", '>', 2},
	{"amp", '&', 3},
	{"AMP", '&', 3},
	{"quot", '\"', 4},
	{"QUOT", '\"', 4},
	{"nbsp", '\312', 4},
	{"reg", '\250', 3},
	{"REG", '\250', 3},
	{"copy", '\251', 4},
	{"COPY", '\251', 4},

	{"iexcl", '\301', 5},
	{"cent", '\242', 4},
	{"pound", '\243', 5},
	{"curren", '\333', 6},
	{"yen", '\264', 3},

	/*
	 * Navigator Gold currently inverts this table in such a way that
	 * ASCII characters (less than 128) get converted to the names
	 * listed here.  For things like ampersand (&amp;) this is the
	 * right thing to do, but for this one (brvbar), it isn't since
	 * both broken vertical bar and vertical bar are mapped to the same
	 * character by the Latin-1 to Mac Roman table.
	 *
	 * Punt for now. This needs to be fixed later. -- erik
	 */
	/* {"brvbar", '\174', 6}, */

	{"sect", '\244', 4},

	{"uml", '\254', 3},
	{"ordf", '\273', 4},
	{"laquo", '\307', 5},
	{"not", '\302', 3},
	{"shy", '\320', 3},
	{"macr", '\370', 4},

	{"deg", '\241', 3},
	{"plusmn", '\261', 6},
	/* {"sup2", '\62', 4}, see comment above */
	/* {"sup3", '\63', 4}, see comment above */
	{"acute", '\253', 5},
	{"micro", '\265', 5},
	{"para", '\246', 4},
	{"middot", '\341', 6},

	{"cedil", '\374', 5},
	/* {"sup1", '\61', 4}, see comment above */
	{"ordm", '\274', 4},
	{"raquo", '\310', 5},
	{"frac14", '\271', 6},
	{"frac12", '\270', 6},
	{"frac34", '\262', 6},
	{"iquest", '\300', 6},

	{"Agrave", '\313', 6},
	{"Aacute", '\347', 6},
	{"Acirc", '\345', 5},
	{"Atilde", '\314', 6},
	{"Auml", '\200', 4},
	{"Aring", '\201', 5},
	{"AElig", '\256', 5},
	{"Ccedil", '\202', 6},

	{"Egrave", '\351', 6},
	{"Eacute", '\203', 6},
	{"Ecirc", '\346', 5},
	{"Euml", '\350', 4},
	{"Igrave", '\355', 6},
	{"Iacute", '\352', 6},
	{"Icirc", '\353', 5},
	{"Iuml", '\354', 4},

	{"ETH", '\334', 3},			/* Icelandic MacRoman: ETH ('D' w/horiz bar) */
	{"Ntilde", '\204', 6},
	{"Ograve", '\361', 6},
	{"Oacute", '\356', 6},
	{"Ocirc", '\357', 5},
	{"Otilde", '\315', 6},
	{"Ouml", '\205', 4},
	/* {"times", '\170', 5}, see comment above */

	{"Oslash", '\257', 6},
	{"Ugrave", '\364', 6},
	{"Uacute", '\362', 6},
	{"Ucirc", '\363', 5},
	{"Uuml", '\206', 4},
	{"Yacute", '\240', 6},		/* Icelandic MacRoman: Yacute */
	{"THORN", '\336', 5},		/* Icelandic MacRoman: THORN (kinda like 'P') */
	{"szlig", '\247', 5},

	{"agrave", '\210', 6},
	{"aacute", '\207', 6},
	{"acirc", '\211', 5},
	{"atilde", '\213', 6},
	{"auml", '\212', 4},
	{"aring", '\214', 5},
	{"aelig", '\276', 5},
	{"ccedil", '\215', 6},

	{"egrave", '\217', 6},
	{"eacute", '\216', 6},
	{"ecirc", '\220', 5},
	{"euml", '\221', 4},
	{"igrave", '\223', 6},
	{"iacute", '\222', 6},
	{"icirc", '\224', 5},
	{"iuml", '\225', 4},

	{"eth", '\335', 3},		/* Icelandic MacRoman: eth ('d' w/horiz bar) */
	{"ntilde", '\226', 6},
	{"ograve", '\230', 6},
	{"oacute", '\227', 6},
	{"ocirc", '\231', 5},
	{"otilde", '\233', 6},
	{"ouml", '\232', 4},
	{"divide", '\326', 6},

	{"oslash", '\277', 6},
	{"ugrave", '\235', 6},
	{"uacute", '\234', 6},
	{"ucirc", '\236', 5},
	{"uuml", '\237', 4},
	{"yacute", '\340', 6},		/* Icelandic MacRoman: yacute */
	{"thorn", '\337', 5},		/* Icelandic MacRoman: thorn (kinda like 'p') */
	{"yuml", '\330', 4},

	{NULL, '\0', 0},
};
#endif /* ! XP_MAC */

