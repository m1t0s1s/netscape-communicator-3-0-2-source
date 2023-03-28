#include "prtypes.h"
#define RESOURCE_STR
#include <allxpstr.h>


#ifndef MOZILLA_CLIENT
/*
 * This routine picks up a builtin (compiled in) numbered string.
 * (I.e. error messages, etc.)
 *
 * If you are getting an unresolved symbol XP_GetString and you don't
 * want to get strings from resources (which is how the Win, Mac and X
 * versions of the Navigator do it), then you can define your own
 * wrapper function like this:
 */
char *
XP_GetString(int16 i)
{
	extern char * XP_GetBuiltinString(int16 i);

	return XP_GetBuiltinString(i);
}
#endif /* ! MOZILLA_CLIENT */

char *
XP_GetBuiltinString(int16 i)
{
	static char	buf[128];
	char		*ret;

	i += RES_OFFSET;

	if
	(
		((ret = mcom_include_merrors_i_strings (i))) ||
		((ret = mcom_include_secerr_i_strings  (i))) ||
		((ret = mcom_include_sslerr_i_strings  (i))) ||
		((ret = mcom_include_xp_error_i_strings(i))) ||
		((ret = mcom_include_xp_msg_i_strings  (i)))
	)
	{
		return ret;
	}

	(void) sprintf(buf, "XP_GetBuiltinString: %d not found", i);

	return buf;
}
