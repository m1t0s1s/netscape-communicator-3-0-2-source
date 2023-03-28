#include <stdio.h>

#define PUTCHAR(c) if (whole_line || (!colon_seen)) { putchar(c); }

int
main(int argc, char *argv[])
{
	int	c;
	int	colon_seen;
	int	whole_line;

	if (argc == 1)
	{
		whole_line = 1;
	}
	else
	{
		whole_line = 0;
	}

	colon_seen = 0;
	while (c != EOF)
	{
		switch (c)
		{
		case ':':
			PUTCHAR(c);
			colon_seen = 1;
			break;
		case '\\':
			c = getchar();
			if (c != '\n')
			{
				PUTCHAR('\\');
				if (c != EOF)
				{
					PUTCHAR(c);
				}
			}
			break;
		case '\n':
			colon_seen = 0;
			PUTCHAR(c);
			break;
		default:
			PUTCHAR(c);
			break;
		}
		c = getchar();
	}
	return 0;
}
