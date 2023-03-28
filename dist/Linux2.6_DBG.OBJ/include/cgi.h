#ifndef __cgi_h_
#define __cgi_h_

/*
** CGI assist library. Portability layer for writing correctly behaving
** CGI programs.
*/
#include "ds.h"

XP_BEGIN_PROTOS

/*
** Read in the input, generating a single long string out of it.  CGI
** programs normally get the value of various forms elements as input.
*/
extern char *CGI_GatherInput(FILE *in);

/*
** Given a null terminated string, compress it in place, converting
** "funny characters" into their ascii equivalent. Maps "+" to space and
** %xx to the binary version of xx, where xx is a pair of hex digits.
*/
extern void CGI_CompressString(char *s);

/*
** Convert a string into an argument vector. This seperates the incoming
** string into pieces, and calls CGI_CompressString to compress the
** pieces. This allocates memory for the return value only.
*/
extern char **CGI_ConvertStringToArgVec(char *string, int *argcp);

/*
** Look for the variable called "name" in the argv. Return a pointer to
** the value portion of the variable if found, zero otherwise.  this does
** not malloc memory.
*/
extern char *CGI_GetVariable(char *name, int argc, char **argv);

/* Return non-zero if the variable string is not empty */
#define CGI_IsEmpty(var) (!(var) || ((var)[0] == 0))

/*
** Return true if the server that started the cgi running is using
** security (https).
*/
extern DSBool CGI_IsSecureServer(void);

/*
** Concatenate strings to produce a single string.
*/
extern char *CGI_Cat(char *, ...);

/* Escape a string, cgi style */
char *CGI_Escape(char *in);

XP_END_PROTOS

#endif /* __cgi_h_ */
