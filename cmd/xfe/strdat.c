static void output(int, char *);

#define RESOURCE_STR
#define RESOURCE_STR_X

#include <allxpstr.h>
#include "xfe_err.h"


static void
output(int i, char *s)
{
	unsigned char	c;

	(void) printf("*strings.%d:", i);

	while ((c = *s++))
	{
		if (c == '\n')
		{
			(void) printf("\\n");
		}
		else if (c == '\\')
		{
			(void) printf("\\\\");
		}
		else
		{
			(void) printf("%c", c);
		}
	}

	(void) printf("\n\n");
}


int
main(int argc, char *argv[])
{
	mcom_include_merrors_i_strings();
	mcom_include_secerr_i_strings();
	mcom_include_sslerr_i_strings();
	mcom_include_xp_error_i_strings();
	mcom_include_xp_msg_i_strings();
	mcom_cmd_xfe_xfe_err_h_strings();
}
