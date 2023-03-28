#include "sec.h"

void main(int argc, char **argv)
{
    for (;;) {
	SECName *n;
	char name[1000];

	printf("Name: ");
	gets(name);
	n = SEC_AsciiToName(name);
	if (n) {
	    char *a = SEC_NameToAscii(n);
	    printf("Conversion ok\nName: %s\n",
		   a ? a : "oops - didn't convert back to ascii");
	} else {
	    printf("Conversion failed\n");
	}
    }
}
