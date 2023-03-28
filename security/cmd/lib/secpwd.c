#include "secutil.h"

/*
 * NOTE:  The contents of this file are NOT used by the client.
 * (They are part of the security library as a whole, but they are
 * NOT USED BY THE CLIENT.)  Do not change things on behalf of the
 * client (like localizing strings), or add things that are only
 * for the client (put them elsewhere).  Clearly we need a better
 * way to organize things, but for now this will have to do.
 */


#ifdef XP_UNIX
#include <termios.h>
#endif

#ifdef _WINDOWS
#include <conio.h>
#define QUIET_FGETS quiet_fgets
static int quiet_fgets (char *buf, int length, FILE *input);
#else
#define QUIET_FGETS fgets
#endif

static void echoOff(int fd)
{
#ifdef XP_UNIX
    if (isatty(fd)) {
	struct termios tio;
	tcgetattr(fd, &tio);
	tio.c_lflag &= ~ECHO;
	tcsetattr(fd, TCSAFLUSH, &tio);
    }
#endif
}

static void echoOn(int fd)
{
#ifdef XP_UNIX
    if (isatty(fd)) {
	struct termios tio;
	tcgetattr(fd, &tio);
	tio.c_lflag |= ECHO;
	tcsetattr(fd, TCSAFLUSH, &tio);
    }
#endif
}

unsigned char *SEC_GetPassword(FILE *input, FILE *output, char *prompt,
			       PRBool (*ok)(unsigned char *))
{
    unsigned char phrase[200];
    int infd = fileno(input);
    int isTTY = isatty(infd);
    for (;;) {
	/* Prompt for password */
	if (isTTY) {
	    fprintf(output, "%s", prompt);
            fflush (output);
	    echoOff(infd);
	}

	QUIET_FGETS ((char*) phrase, sizeof(phrase), input);

	if (isTTY) {
	    fprintf(output, "\n");
	    echoOn(infd);
	}

	/* stomp on newline */
	phrase[PORT_Strlen((char*)phrase)-1] = 0;

	/* Validate password */
	if (!(*ok)(phrase)) {
	    /* Not weird enough */
	    if (!isTTY) return 0;
	    fprintf(output, "Password must be at least 8 characters long with one or more\n");
	    fprintf(output, "non-alphabetic characters\n");
	    continue;
	}
	return (unsigned char*) PORT_Strdup((char*)phrase);
    }
}



PRBool SEC_CheckPassword(unsigned char *cp)
{
    int len;
    unsigned char *end;

    len = PORT_Strlen((char*)cp);
    if (len < 8) {
	return PR_FALSE;
    }
    end = cp + len;
    while (cp < end) {
	unsigned char ch = *cp++;
	if (!((ch >= 'A') && (ch <= 'Z')) &&
	    !((ch >= 'a') && (ch <= 'z'))) {
	    /* pass phrase has at least one non alphabetic in it */
	    return PR_TRUE;
	}
    }
    return PR_FALSE;
}

PRBool SEC_BlindCheckPassword(unsigned char *cp)
{
    if (cp != NULL) {
	return PR_TRUE;
    }
    return PR_FALSE;
}

/* Get a password from the input terminal, without echoing */

#ifdef _WINDOWS
static int quiet_fgets (char *buf, int length, FILE *input)
  {
  int c;
  char *end = buf;

  fflush (input);
  memset (buf, 0, length);

  while (1)
    {
    c = getch();

    if (c == '\b')
      {
      if (end > buf)
        end--;
      }

    else if (--length > 0)
      *end++ = c;

    if (!c || c == '\n' || c == '\r')
      break;
    }

  return 0;
  }
#endif
