#include <stdio.h>

#define OFFSET	0105

int
main()
{
	int c;
	while((c=getchar()) != EOF)
		putchar(c+OFFSET);
	return 0;
}

