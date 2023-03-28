/*
 * locale.c
 * --------
 * Implement FE functions to support xp_locale and other locale stuff
 */

#include <xplocale.h>

#include "xfe.h"
#include "felocale.h"


int16 fe_LocaleCharSetID = CS_LATIN1;

char fe_LocaleCharSetName[128] = { 0 };

int (*fe_collation_func)(const char *, const char *) = strcoll;


void
fe_InitCollation(void)
{
	/*
	 * Check to see if strcoll() is broken
	 */
	if
	(
		((strcoll("A", "B") < 0) && (strcoll("a", "B") >= 0)) ||

		/*
		 * Unlikely, but just in case...
		 */
		((strcoll("A", "B") > 0) && (strcoll("a", "B") <= 0)) ||
		(strcoll("A", "B") == 0)
	)
	{
		/*
		 * strcoll() is broken, so we use our own routine
		 */
		fe_collation_func = strcasecomp;
	}
}


int
FE_StrColl(const char *s1, const char *s2)
{
	return (*fe_collation_func)(s1, s2);
}


size_t FE_StrfTime(MWContext *context, char *result, size_t maxsize,
	int format, const struct tm *timeptr)

{
	char	*fmt;

#ifdef OLD

	switch (format)
	{
	case XP_DATEFORMAT:
		fmt = "%x";
		break;
	case XP_TIMEFORMAT:
	default:
		fmt = "%X";
		break;
	}

#else /* OLD */

	switch (format)
	{
	case XP_TIME_FORMAT:
		/* fmt = "%X"; */
		fmt = "%H:%M";
		break;
	case XP_WEEKDAY_TIME_FORMAT:
		/* fmt = "%a %X"; */
		fmt = "%a %H:%M";
		break;
	case XP_DATE_TIME_FORMAT:
		/* fmt = "%x %X"; */
		fmt = "%x %H:%M";
		break;
	case XP_LONG_DATE_TIME_FORMAT:
		fmt = "%c";
		break;
	default:
		fmt = "%c";
		break;
	}

#endif /* OLD */

	return strftime(result, maxsize, fmt, timeptr);
}


char *
fe_GetNormalizedLocaleName(void)
{

#ifdef _HPUX_SOURCE

	int	len;
	char	*locale;

	locale = setlocale(LC_CTYPE, NULL);
	if (locale && *locale)
	{
		len = strlen(locale);
	}
	else
	{
		locale = "C";
		len = 1;
	}

	if
	(
		(!strncmp(locale, "/\x03:", 3)) &&
		(!strcmp(&locale[len - 2], ";/"))
	)
	{
		locale += 3;
		len -= 5;
	}

	locale = strdup(locale);
	if (locale)
	{
		locale[len] = 0;
	}

	return locale;

#else

	char	*locale;

	locale = setlocale(LC_CTYPE, NULL);
	if (locale && *locale)
	{
		return strdup(locale);
	}

	return strdup("C");

#endif

}


unsigned char *
fe_ConvertToLocaleEncoding(int16 charset, unsigned char *str)
{
	CCCDataObject	*obj;
	unsigned char	*ret;

	if ((charset == fe_LocaleCharSetID) || (!str) || (!*str))
	{
		return str;
	}

	obj = INTL_CreateCharCodeConverter();
	if (!obj)
	{
		return str;
	}
	if (INTL_GetCharCodeConverter(charset, fe_LocaleCharSetID, obj))
	{
		ret = INTL_CallCharCodeConverter(obj, str,
			strlen((char *) str));
		if (!ret)
		{
			ret = str;
		}
	}
	else
	{
		ret = str;
	}

	INTL_DestroyCharCodeConverter(obj);

	return ret;
}


XmString
fe_ConvertToXmString(int16 charset, unsigned char *str)
{
	unsigned char	*loc;
	XmString	xms;

	loc = fe_ConvertToLocaleEncoding(charset, str);
	if (loc)
	{
		xms = XmStringCreate((char *) loc, XmFONTLIST_DEFAULT_TAG);
		if (loc != str)
		{
			free(loc);
		}
	}
	else
	{
		xms = NULL;
	}

	return xms;
}


unsigned char *
fe_ConvertFromLocaleEncoding(int16 charset, unsigned char *str)
{
	CCCDataObject	*obj;
	unsigned char	*ret;

	if ((charset == fe_LocaleCharSetID) || (!str) || (!*str))
	{
		return str;
	}

	obj = INTL_CreateCharCodeConverter();
	if (!obj)
	{
		return str;
	}
	if (INTL_GetCharCodeConverter(fe_LocaleCharSetID, charset, obj))
	{
		ret = INTL_CallCharCodeConverter(obj, str,
			strlen((char *) str));
		if (!ret)
		{
			ret = str;
		}
	}
	else
	{
		ret = str;
	}

	INTL_DestroyCharCodeConverter(obj);

	return ret;
}


#if 0
/* fe_GetTextSelection() is a direct replacement for XmTextGetSelection --
 * use XtFree() to free the returned string.
 */
char *
fe_GetTextSelection(Widget widget)
{
	char	*loc;
	char	*str;

	XP_ASSERT(XmIsText(widget) || XmIsTextField(widget));

	loc = NULL;
	if (XmIsText(widget)) {
		loc = XmTextGetSelection(widget);
	}
	else {
		loc = XmTextFieldGetSelection(widget);
	}
	if (!loc)
	{
		return NULL;
	}
	str = (char *) fe_ConvertFromLocaleEncoding(
		INTL_DefaultWinCharSetID(NULL), (unsigned char *) loc);
	if (str == loc)
	{
		str = XtNewString(str);
		if (!str)
		{
			return NULL;
		}
	}
	XtFree(loc);

	return str;
}
#endif /* 0 */


/* fe_GetTextField() is a direct replacement for XmTextGetString --
 * use XtFree() to free the returned string.
 */
char *
fe_GetTextField(Widget widget)
{
	char	*loc;
	char	*str;

	XP_ASSERT(XmIsText(widget) || XmIsTextField(widget));

	loc = NULL;
	XtVaGetValues(widget, XmNvalue, &loc, 0);
	if (!loc)
	{
		return NULL;
	}
	str = (char *) fe_ConvertFromLocaleEncoding(
		INTL_DefaultWinCharSetID(NULL), (unsigned char *) loc);
	if (str == loc)
	{
		str = XtNewString(str);
		if (!str)
		{
			return NULL;
		}
	}
	XtFree(loc);

	return str;
}


void
fe_SetTextField(Widget widget, const char *str)
{
	unsigned char	*loc;

	XP_ASSERT(XmIsText(widget) || XmIsTextField(widget));

	if ((NULL==str) || ('\0'==*str)) {
		XtVaSetValues(widget, XmNvalue, str, 0);
		return;
	}

	loc = fe_ConvertToLocaleEncoding(INTL_DefaultWinCharSetID(NULL),
		(unsigned char *) str);

#ifdef DEBUG_jwz
        {
          /* On my Linux box, Motif freaks out if you try to insert a
             high-bit character into a text field.  So don't.
           */
          unsigned char *s = (unsigned char *) str;
          if (s)
            while (*s)
              {
                if (*s & 0x80) *s &= 0x7F;
                s++;
              }
        }
#endif /* DEBUG_jwz */

	XtVaSetValues(widget, XmNvalue, loc, 0);
	if (loc != ((unsigned char *) str))
	{
		XP_FREE(loc);
	}
}


void
fe_SetTextFieldAndCallBack(Widget widget, const char *str)
{
	unsigned char	*loc = NULL;

	XP_ASSERT(XmIsText(widget) || XmIsTextField(widget));

	if ((NULL != str) && ('\0' != *str)) {

		loc = fe_ConvertToLocaleEncoding(INTL_DefaultWinCharSetID(NULL),
										 (unsigned char *) str);
	}

#ifdef DEBUG_jwz
        {
          /* On my Linux box, Motif freaks out if you try to insert a
             high-bit character into a text field.  So don't.
           */
          unsigned char *s = (unsigned char *) str;
          if (s)
            while (*s)
              {
                if (*s & 0x80) *s &= 0x7F;
                s++;
              }
        }
#endif /* DEBUG_jwz */

	/*
	 * Warning: on SGI, XtVaSetValues() doesn't run the
	 * valueChangedCallback, but XmTextFieldSetString() does.
	 */
	if (XmIsText(widget))
		XmTextSetString(widget, (char *) str);
	else if (XmIsTextField(widget))
		XmTextFieldSetString(widget, (char *) str);

	if (loc != ((unsigned char *) str))
	{
		XP_FREE(loc);
	}
}


/*
 * We link statically on AIX to force it to pick up our thread-safe malloc.
 * But AIX has a bug where linking statically results in a broken mbstowcs
 * (and wcstombs) being linked in. So we re-implement these here, so that
 * these are used. See bug # 13574.
 */

#ifdef AIXV3

size_t
mbstowcs(wchar_t *pwcs, const char *s, size_t n)
{
	int	charlen;
	size_t	inlen;
	size_t	ret;
	wchar_t	wc;

	if (!s)
	{
		return 0;
	}

	ret = 0;
	inlen = strlen(s) + 1;

	while (1)
	{
		wc = 0;
		charlen = mbtowc(&wc, s, inlen);
		if (charlen < 0)
		{
			return -1;
		}
		else if (charlen == 0)
		{
			if (pwcs)
			{
				if (n > 0)
				{
					*pwcs = 0;
				}
			}
			break;
		}
		else
		{
			if (pwcs)
			{
				if (n > 0)
				{
					*pwcs++ = wc;
					ret++;
					n--;
					if (n == 0)
					{
						break;
					}
				}
				else
				{
					break;
				}
			}
			else
			{
				ret++;
			}
			inlen -= charlen;
			s += charlen;
		}
	}

	return ret;
}


size_t
wcstombs(char *s, const wchar_t *pwcs, size_t n)
{
	char	buf[MB_LEN_MAX];
	int	charlen;
	int	i;
	size_t	ret;

	if (!pwcs)
	{
		return 0;
	}

	ret = 0;

	while (1)
	{
		buf[0] = 0;
		charlen = wctomb(buf, *pwcs);
		if (charlen <= 0)
		{
			return -1;
		}
		else
		{
			if (s)
			{
				if (n >= charlen)
				{
					for (i = 0; i < charlen; i++)
					{
						*s++ = buf[i];
					}
					if (*pwcs)
					{
						ret += charlen;
					}
					else
					{
						break;
					}
					n -= charlen;
					if (n == 0)
					{
						break;
					}
				}
				else
				{
					break;
				}
			}
			else
			{
				if (*pwcs)
				{
					ret += charlen;
				}
				else
				{
					break;
				}
			}
			pwcs++;
		}
	}

	return ret;
}

#endif /* AIXV3 */
