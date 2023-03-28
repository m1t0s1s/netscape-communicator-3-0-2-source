/**********************************************************************
 ad-obfuscate.c
 By Daniel Malmer
 16 Jan 1996

 Used to generate the Enterprise Kit default resources.
 Reads from the standard input.
 Prepends each line with quotes, prints out the obfuscated line, and
 adds a quote at the end.
 The effect is to create one long character string with the octal
 version of the input, with 42 added to each character.
**********************************************************************/

#include <stdio.h>
#include <string.h>

int
main()
{
    char buf[1024];
    char* ptr;

    while ( fgets(buf, sizeof(buf), stdin) != NULL ) {
        printf("\"");
        for ( ptr = &(buf[0]); *ptr != '\0'; ptr++ ) {
            printf("\\%o", (*ptr)+42);
        }
        printf("\"\n");
    }
}


