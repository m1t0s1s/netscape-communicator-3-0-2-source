
# line 2 "gram.y"
/*
 * Copyright (c) 1993 Michael A. Cooper
 * Copyright (c) 1993 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef lint
static char RCSid[] = 
"$Id: gram.c,v 1.1 1996/01/31 23:32:25 dkarlton Exp $";

static	char *sccsid = "@(#)gram.y	5.2 (Berkeley) 85/06/21";

static char copyright[] =
"@(#) Copyright (c) 1983 Regents of the University of California.\n\
 All rights reserved.\n";
#endif /* not lint */

/*
 * Tell defs.h not to include y.tab.h
 */
#ifndef yacc
#define yacc
#endif

#include "defs.h"

static struct namelist *addnl(), *subnl(), *andnl();
struct	cmd *cmds = NULL;
struct	cmd *last_cmd;
struct	namelist *last_n;
struct	subcmd *last_sc;
int	parendepth = 0;


# line 78 "gram.y"
typedef union
#ifdef __cplusplus
	YYSTYPE
#endif
 {
	opt_t 			optval;
	char 		       *string;
	struct subcmd 	       *subcmd;
	struct namelist        *namel;
} YYSTYPE;
# define ARROW 1
# define COLON 2
# define DCOLON 3
# define NAME 4
# define STRING 5
# define INSTALL 6
# define NOTIFY 7
# define EXCEPT 8
# define PATTERN 9
# define SPECIAL 10
# define CMDSPECIAL 11
# define OPTION 12

#include <malloc.h>
#include <memory.h>
#include <unistd.h>
#include <values.h>

#ifdef __cplusplus

#ifndef yyerror
	void yyerror(const char *);
#endif
#ifndef yylex
	extern "C" int yylex(void);
#endif
	int yyparse(void);

#endif
#define yyclearin yychar = -1
#define yyerrok yyerrflag = 0
extern int yychar;
extern int yyerrflag;
YYSTYPE yylval;
YYSTYPE yyval;
typedef int yytabelem;
#ifndef YYMAXDEPTH
#define YYMAXDEPTH 150
#endif
#if YYMAXDEPTH > 0
int yy_yys[YYMAXDEPTH], *yys = yy_yys;
YYSTYPE yy_yyv[YYMAXDEPTH], *yyv = yy_yyv;
#else	/* user does initial allocation */
int *yys;
YYSTYPE *yyv;
#endif
static int yymaxdepth = YYMAXDEPTH;
# define YYERRCODE 256

# line 230 "gram.y"


int	yylineno = 1;
extern	FILE *fin;

yylex()
{
	static char yytext[INMAX];
	register int c;
	register char *cp1, *cp2;
	static char quotechars[] = "[]{}*?$";
	
again:
	switch (c = getc(fin)) {
	case EOF:  /* end of file */
		return(0);

	case '#':  /* start of comment */
		while ((c = getc(fin)) != EOF && c != '\n')
			;
		if (c == EOF)
			return(0);
	case '\n':
		yylineno++;
	case ' ':
	case '\t':  /* skip blanks */
		goto again;

	case '=':  /* EQUAL */
	case ';':  /* SM */
	case '+': 
	case '&': 
		return(c);

	case '(':  /* LP */
		++parendepth;
		return(c);

	case ')':  /* RP */
		--parendepth;
		return(c);

	case '-':  /* -> */
		if ((c = getc(fin)) == '>')
			return(ARROW);
		(void) ungetc(c, fin);
		c = '-';
		break;

	case '"':  /* STRING */
		cp1 = yytext;
		cp2 = &yytext[INMAX - 1];
		for (;;) {
			if (cp1 >= cp2) {
				yyerror("command string too long\n");
				break;
			}
			c = getc(fin);
			if (c == EOF || c == '"')
				break;
			if (c == '\\') {
				if ((c = getc(fin)) == EOF) {
					*cp1++ = '\\';
					break;
				}
			}
			if (c == '\n') {
				yylineno++;
				c = ' '; /* can't send '\n' */
			}
			*cp1++ = c;
		}
		if (c != '"')
			yyerror("missing closing '\"'\n");
		*cp1 = '\0';
		yylval.string = makestr(yytext);
		return(STRING);

	case ':':  /* : or :: */
		if ((c = getc(fin)) == ':')
			return(DCOLON);
		(void) ungetc(c, fin);
		return(COLON);
	}
	cp1 = yytext;
	cp2 = &yytext[INMAX - 1];
	for (;;) {
		if (cp1 >= cp2) {
			yyerror("input line too long\n");
			break;
		}
		if (c == '\\') {
			if ((c = getc(fin)) != EOF) {
				if (any(c, quotechars))
					*cp1++ = QUOTECHAR;
			} else {
				*cp1++ = '\\';
				break;
			}
		}
		*cp1++ = c;
		c = getc(fin);
		if (c == EOF || any(c, " \"'\t()=;:\n")) {
			(void) ungetc(c, fin);
			break;
		}
	}
	*cp1 = '\0';
	if (yytext[0] == '-' && yytext[1] == CNULL) 
		return '-';
	if (yytext[0] == '-' && parendepth <= 0) {
		opt_t opt = 0;
		static char ebuf[BUFSIZ];

		switch (yytext[1]) {
		case 'o':
			if (parsedistopts(&yytext[2], &opt, TRUE)) {
				(void) sprintf(ebuf, 
					       "Bad distfile options \"%s\".", 
					       &yytext[2]);
				yyerror(ebuf);
			}
			break;

			/*
			 * These options are obsoleted by -o.
			 */
		case 'b':	opt = DO_COMPARE;		break;
		case 'R':	opt = DO_REMOVE;		break;
		case 'v':	opt = DO_VERIFY;		break;
		case 'w':	opt = DO_WHOLE;			break;
		case 'y':	opt = DO_YOUNGER;		break;
		case 'h':	opt = DO_FOLLOW;		break;
		case 'i':	opt = DO_IGNLNKS;		break;
		case 'q':	opt = DO_QUIET;			break;
		case 'x':	opt = DO_NOEXEC;		break;
		case 'N':	opt = DO_CHKNFS;		break;
		case 'O':	opt = DO_CHKREADONLY;		break;
		case 's':	opt = DO_SAVETARGETS;		break;
		case 'r':	opt = DO_NODESCEND;		break;

		default:
			(void) sprintf(ebuf, "Unknown option \"%s\".", yytext);
			yyerror(ebuf);
		}

		yylval.optval = opt;
		return(OPTION);
	}
	if (!strcmp(yytext, "install"))
		c = INSTALL;
	else if (!strcmp(yytext, "notify"))
		c = NOTIFY;
	else if (!strcmp(yytext, "except"))
		c = EXCEPT;
	else if (!strcmp(yytext, "except_pat"))
		c = PATTERN;
	else if (!strcmp(yytext, "special"))
		c = SPECIAL;
	else if (!strcmp(yytext, "cmdspecial"))
		c = CMDSPECIAL;
	else {
		yylval.string = makestr(yytext);
		return(NAME);
	}
	yylval.subcmd = makesubcmd(c);
	return(c);
}

/*
 * XXX We should use strchr(), but most versions can't handle
 * some of the characters we use.
 */
extern int any(c, str)
	register int c;
	register char *str;
{
	while (*str)
		if (c == *str++)
			return(1);
	return(0);
}

/*
 * Insert or append ARROW command to list of hosts to be updated.
 */
insert(label, files, hosts, subcmds)
	char *label;
	struct namelist *files, *hosts;
	struct subcmd *subcmds;
{
	register struct cmd *c, *prev, *nc;
	register struct namelist *h, *lasth;

	debugmsg(DM_CALL, "insert(%s, %x, %x, %x) start, files = %s", 
		 label == NULL ? "(null)" : label,
		 files, hosts, subcmds, getnlstr(files));

	files = expand(files, E_VARS|E_SHELL);
	hosts = expand(hosts, E_ALL);
	for (h = hosts; h != NULL; lasth = h, h = h->n_next, 
	     free((char *)lasth)) {
		/*
		 * Search command list for an update to the same host.
		 */
		for (prev = NULL, c = cmds; c!=NULL; prev = c, c = c->c_next) {
			if (strcmp(c->c_name, h->n_name) == 0) {
				do {
					prev = c;
					c = c->c_next;
				} while (c != NULL &&
					strcmp(c->c_name, h->n_name) == 0);
				break;
			}
		}
		/*
		 * Insert new command to update host.
		 */
		nc = ALLOC(cmd);
		nc->c_type = ARROW;
		nc->c_name = h->n_name;
		nc->c_label = label;
		nc->c_files = files;
		nc->c_cmds = subcmds;
		nc->c_flags = 0;
		nc->c_next = c;
		if (prev == NULL)
			cmds = nc;
		else
			prev->c_next = nc;
		/* update last_cmd if appending nc to cmds */
		if (c == NULL)
			last_cmd = nc;
	}
}

/*
 * Append DCOLON command to the end of the command list since these are always
 * executed in the order they appear in the distfile.
 */
append(label, files, stamp, subcmds)
	char *label;
	struct namelist *files;
	char *stamp;
	struct subcmd *subcmds;
{
	register struct cmd *c;

	c = ALLOC(cmd);
	c->c_type = DCOLON;
	c->c_name = stamp;
	c->c_label = label;
	c->c_files = expand(files, E_ALL);
	c->c_cmds = subcmds;
	c->c_next = NULL;
	if (cmds == NULL)
		cmds = last_cmd = c;
	else {
		last_cmd->c_next = c;
		last_cmd = c;
	}
}

/*
 * Error printing routine in parser.
 */
yyerror(s)
	char *s;
{
	error("Error in distfile: line %d: %s", yylineno, s);
}

/*
 * Return a copy of the string.
 */
char *
makestr(str)
	char *str;
{
	char *cp;

	cp = strdup(str);
	if (cp == NULL)
		fatalerr("ran out of memory");

	return(cp);
}

/*
 * Allocate a namelist structure.
 */
struct namelist *
makenl(name)
	char *name;
{
	register struct namelist *nl;

	debugmsg(DM_CALL, "makenl(%s)", name == NULL ? "null" : name);

	nl = ALLOC(namelist);
	nl->n_name = name;
	nl->n_next = NULL;

	return(nl);
}


/*
 * Is the name p in the namelist nl?
 */
static int
innl(nl, p)
	struct namelist *nl;
	char *p;
{
	for ( ; nl; nl = nl->n_next)
		if (!strcmp(p, nl->n_name))
			return(1);
	return(0);
}

/*
 * Join two namelists.
 */
static struct namelist *
addnl(n1, n2)
	struct namelist *n1, *n2;
{
	struct namelist *nl, *prev;

	n1 = expand(n1, E_VARS);
	n2 = expand(n2, E_VARS);
	for (prev = NULL, nl = NULL; n1; n1 = n1->n_next, prev = nl) {
		nl = makenl(n1->n_name);
		nl->n_next = prev;
	}
	for (; n2; n2 = n2->n_next)
		if (!innl(nl, n2->n_name)) {
			nl = makenl(n2->n_name);
			nl->n_next = prev;
			prev = nl;
		}
	return(prev);
}

/*
 * Copy n1 except for elements that are in n2.
 */
static struct namelist *
subnl(n1, n2)
	struct namelist *n1, *n2;
{
	struct namelist *nl, *prev;

	n1 = expand(n1, E_VARS);
	n2 = expand(n2, E_VARS);
	for (prev = NULL; n1; n1 = n1->n_next)
		if (!innl(n2, n1->n_name)) {
			nl = makenl(n1->n_name);
			nl->n_next = prev;
			prev = nl;
		}
	return(prev);
}

/*
 * Copy all items of n1 that are also in n2.
 */
static struct namelist *
andnl(n1, n2)
	struct namelist *n1, *n2;
{
	struct namelist *nl, *prev;

	n1 = expand(n1, E_VARS);
	n2 = expand(n2, E_VARS);
	for (prev = NULL; n1; n1 = n1->n_next)
		if (innl(n2, n1->n_name)) {
			nl = makenl(n1->n_name);
			nl->n_next = prev;
			prev = nl;
		}
	return(prev);
}

/*
 * Make a sub command for lists of variables, commands, etc.
 */
extern struct subcmd *
makesubcmd(type)
	int type;
{
	register struct subcmd *sc;

	sc = ALLOC(subcmd);
	sc->sc_type = type;
	sc->sc_args = NULL;
	sc->sc_next = NULL;
	sc->sc_name = NULL;

	return(sc);
}
yytabelem yyexca[] ={
-1, 1,
	0, -1,
	-2, 0,
	};
# define YYNPROD 29
# define YYLAST 253
yytabelem yyact[]={

     3,     9,    57,    56,    55,    52,    51,    50,    14,    45,
    46,    28,     4,    13,    17,    12,    25,    31,    17,    16,
    18,    19,    49,    54,    53,     6,    33,    34,    35,    36,
    37,    38,    29,    26,    20,    27,     7,    30,    21,    22,
    23,     2,    39,    40,     1,    42,    43,    44,    47,    15,
     7,    48,    32,    24,     7,    10,    41,    11,     0,     0,
     8,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     5 };
yytabelem yypact[]={

-10000000,    -4,-10000000,    -1,    54,-10000000,   -30,-10000000,    14,    14,
    14,    30,    14,    14,    14,    12,-10000000,-10000000,    32,-10000000,
-10000000,-10000000,-10000000,-10000000,-10000000,-10000000,    14,    13,    20,    20,
-10000000,-10000000,-10000000,-10000000,    14,    14,    14,    14,    14,    20,
    20,    10,   -52,   -53,   -54,    19,-10000000,    18,   -55,-10000000,
-10000000,-10000000,-10000000,   -56,   -57,-10000000,-10000000,-10000000 };
yytabelem yypgo[]={

     0,    56,    11,    52,    10,    49,     9,    25,    44,    41 };
yytabelem yyr1[]={

     0,     8,     8,     9,     9,     9,     9,     9,     9,     4,
     4,     4,     4,     7,     7,     5,     5,     2,     2,     3,
     3,     3,     3,     3,     3,     1,     1,     6,     6 };
yytabelem yyr2[]={

     0,     0,     4,     7,     9,    13,     9,    13,     2,     3,
     7,     7,     7,     3,     7,     1,     5,     1,     5,     9,
     7,     7,     7,     9,     9,     1,     5,     1,     3 };
yytabelem yychk[]={

-10000000,    -8,    -9,     4,    -4,   256,    -7,    40,    61,     2,
     1,     3,    45,    43,    38,    -5,    -4,     4,    -4,    -4,
     4,    -7,    -7,    -7,    41,     4,     1,     3,    -2,    -2,
    -4,     4,    -3,     6,     7,     8,     9,    10,    11,    -2,
    -2,    -1,    -4,    -4,    -4,    -6,    -4,    -6,    -6,    12,
    59,    59,    59,     5,     5,    59,    59,    59 };
yytabelem yydef[]={

     1,    -2,     2,    13,     0,     8,     9,    15,     0,     0,
     0,     0,     0,     0,     0,     0,     3,    13,     0,    17,
    17,    10,    11,    12,    14,    16,     0,     0,     4,     6,
    17,    17,    18,    25,     0,     0,     0,    27,    27,     5,
     7,    27,     0,     0,     0,     0,    28,     0,     0,    26,
    20,    21,    22,     0,     0,    19,    23,    24 };
typedef struct
#ifdef __cplusplus
	yytoktype
#endif
{ char *t_name; int t_val; } yytoktype;
#ifndef YYDEBUG
#	define YYDEBUG	0	/* don't allow debugging */
#endif

#if YYDEBUG

yytoktype yytoks[] =
{
	"ARROW",	1,
	"COLON",	2,
	"DCOLON",	3,
	"NAME",	4,
	"STRING",	5,
	"INSTALL",	6,
	"NOTIFY",	7,
	"EXCEPT",	8,
	"PATTERN",	9,
	"SPECIAL",	10,
	"CMDSPECIAL",	11,
	"OPTION",	12,
	"-unknown-",	-1	/* ends search */
};

char * yyreds[] =
{
	"-no such reduction-",
	"file : /* empty */",
	"file : file command",
	"command : NAME '=' namelist",
	"command : namelist ARROW namelist cmdlist",
	"command : NAME COLON namelist ARROW namelist cmdlist",
	"command : namelist DCOLON NAME cmdlist",
	"command : NAME COLON namelist DCOLON NAME cmdlist",
	"command : error",
	"namelist : nlist",
	"namelist : nlist '-' nlist",
	"namelist : nlist '+' nlist",
	"namelist : nlist '&' nlist",
	"nlist : NAME",
	"nlist : '(' names ')'",
	"names : /* empty */",
	"names : names NAME",
	"cmdlist : /* empty */",
	"cmdlist : cmdlist cmd",
	"cmd : INSTALL options opt_namelist ';'",
	"cmd : NOTIFY namelist ';'",
	"cmd : EXCEPT namelist ';'",
	"cmd : PATTERN namelist ';'",
	"cmd : SPECIAL opt_namelist STRING ';'",
	"cmd : CMDSPECIAL opt_namelist STRING ';'",
	"options : /* empty */",
	"options : options OPTION",
	"opt_namelist : /* empty */",
	"opt_namelist : namelist",
};
#endif /* YYDEBUG */
/* 
 *	Copyright 1987 Silicon Graphics, Inc. - All Rights Reserved
 */

/* #ident	"@(#)yacc:yaccpar	1.10" */
#ident	"$Revision: 1.1 $"

/*
** Skeleton parser driver for yacc output
*/

/*
** yacc user known macros and defines
*/
#define YYERROR		goto yyerrlab
#define YYACCEPT	return(0)
#define YYABORT		return(1)
#ifdef __cplusplus
#define YYBACKUP( newtoken, newvalue )\
{\
	if ( yychar >= 0 || ( yyr2[ yytmp ] >> 1 ) != 1 )\
	{\
		yyerror( gettxt("uxlibc:78", "syntax error - cannot backup") );\
		goto yyerrlab;\
	}\
	yychar = newtoken;\
	yystate = *yyps;\
	yylval = newvalue;\
	goto yynewstate;\
}
#else
#define YYBACKUP( newtoken, newvalue )\
{\
	if ( yychar >= 0 || ( yyr2[ yytmp ] >> 1 ) != 1 )\
	{\
		yyerror( gettxt("uxlibc:78", "Syntax error - cannot backup") );\
		goto yyerrlab;\
	}\
	yychar = newtoken;\
	yystate = *yyps;\
	yylval = newvalue;\
	goto yynewstate;\
}
#endif
#define YYRECOVERING()	(!!yyerrflag)
#define YYNEW(type)	malloc(sizeof(type) * yynewmax)
#define YYCOPY(to, from, type) \
	(type *) memcpy(to, (char *) from, yynewmax * sizeof(type))
#define YYENLARGE( from, type) \
	(type *) realloc((char *) from, yynewmax * sizeof(type))
#ifndef YYDEBUG
#	define YYDEBUG	1	/* make debugging available */
#endif

/*
** user known globals
*/
int yydebug;			/* set to 1 to get debugging */

/*
** driver internal defines
*/
#define YYFLAG		(-10000000)

/*
** global variables used by the parser
*/
YYSTYPE *yypv;			/* top of value stack */
int *yyps;			/* top of state stack */

int yystate;			/* current state */
int yytmp;			/* extra var (lasts between blocks) */

int yynerrs;			/* number of errors */
int yyerrflag;			/* error recovery flag */
int yychar;			/* current input token number */



/*
** yyparse - return 0 if worked, 1 if syntax error not recovered from
*/
#if defined(__STDC__) || defined(__cplusplus)
int yyparse(void)
#else
int yyparse()
#endif
{
	register YYSTYPE *yypvt;	/* top of value stack for $vars */

	/*
	** Initialize externals - yyparse may be called more than once
	*/
	yypv = &yyv[-1];
	yyps = &yys[-1];
	yystate = 0;
	yytmp = 0;
	yynerrs = 0;
	yyerrflag = 0;
	yychar = -1;

#if YYMAXDEPTH <= 0
	if (yymaxdepth <= 0)
	{
		if ((yymaxdepth = YYEXPAND(0)) <= 0)
		{
#ifdef __cplusplus
			yyerror(gettxt("uxlibc:79", "yacc initialization error"));
#else
			yyerror(gettxt("uxlibc:79", "Yacc initialization error"));
#endif
			YYABORT;
		}
	}
#endif

	goto yystack;
	{
		register YYSTYPE *yy_pv;	/* top of value stack */
		register int *yy_ps;		/* top of state stack */
		register int yy_state;		/* current state */
		register int  yy_n;		/* internal state number info */

		/*
		** get globals into registers.
		** branch to here only if YYBACKUP was called.
		*/
	yynewstate:
		yy_pv = yypv;
		yy_ps = yyps;
		yy_state = yystate;
		goto yy_newstate;

		/*
		** get globals into registers.
		** either we just started, or we just finished a reduction
		*/
	yystack:
		yy_pv = yypv;
		yy_ps = yyps;
		yy_state = yystate;

		/*
		** top of for (;;) loop while no reductions done
		*/
	yy_stack:
		/*
		** put a state and value onto the stacks
		*/
#if YYDEBUG
		/*
		** if debugging, look up token value in list of value vs.
		** name pairs.  0 and negative (-1) are special values.
		** Note: linear search is used since time is not a real
		** consideration while debugging.
		*/
		if ( yydebug )
		{
			register int yy_i;

			printf( "State %d, token ", yy_state );
			if ( yychar == 0 )
				printf( "end-of-file\n" );
			else if ( yychar < 0 )
				printf( "-none-\n" );
			else
			{
				for ( yy_i = 0; yytoks[yy_i].t_val >= 0;
					yy_i++ )
				{
					if ( yytoks[yy_i].t_val == yychar )
						break;
				}
				printf( "%s\n", yytoks[yy_i].t_name );
			}
		}
#endif /* YYDEBUG */
		if ( ++yy_ps >= &yys[ yymaxdepth ] )	/* room on stack? */
		{
			int yynewmax, yys_off;

			/* The following pointer-differences are safe, since
			 * yypvt, yy_pv, and yypv all are a multiple of
			 * sizeof(YYSTYPE) bytes from yyv.
			 */
			int yypvt_off = yypvt - yyv;
			int yy_pv_off = yy_pv - yyv;
			int yypv_off = yypv - yyv;

			int *yys_base = yys;
#ifdef YYEXPAND
			yynewmax = YYEXPAND(yymaxdepth);
#else
			yynewmax = 2 * yymaxdepth;	/* double table size */
			if (yymaxdepth == YYMAXDEPTH)	/* first time growth */
			{
				void *newyys = YYNEW(int);
				void *newyyv = YYNEW(YYSTYPE);
				if (newyys != 0 && newyyv != 0)
				{
					yys = YYCOPY(newyys, yys, int);
					yyv = YYCOPY(newyyv, yyv, YYSTYPE);
				}
				else
					yynewmax = 0;	/* failed */
			}
			else				/* not first time */
			{
				yys = YYENLARGE(yys, int);
				yyv = YYENLARGE(yyv, YYSTYPE);
				if (yys == 0 || yyv == 0)
					yynewmax = 0;	/* failed */
			}
#endif
			if (yynewmax <= yymaxdepth)	/* tables not expanded */
			{
#ifdef __cplusplus
				yyerror( gettxt("uxlibc:80", "yacc stack overflow") );
#else
				yyerror( gettxt("uxlibc:80", "Yacc stack overflow") );
#endif
				YYABORT;
			}
			yymaxdepth = yynewmax;

			/* reset pointers into yys */
			yys_off = yys - yys_base;
			yy_ps = yy_ps + yys_off;
			yyps = yyps + yys_off;

			/* reset pointers into yyv */
			yypvt = yyv + yypvt_off;
			yy_pv = yyv + yy_pv_off;
			yypv = yyv + yypv_off;
		}
		*yy_ps = yy_state;
		*++yy_pv = yyval;

		/*
		** we have a new state - find out what to do
		*/
	yy_newstate:
		if ( ( yy_n = yypact[ yy_state ] ) <= YYFLAG )
			goto yydefault;		/* simple state */
#if YYDEBUG
		/*
		** if debugging, need to mark whether new token grabbed
		*/
		yytmp = yychar < 0;
#endif
		if ( ( yychar < 0 ) && ( ( yychar = yylex() ) < 0 ) )
			yychar = 0;		/* reached EOF */
#if YYDEBUG
		if ( yydebug && yytmp )
		{
			register int yy_i;

			printf( "Received token " );
			if ( yychar == 0 )
				printf( "end-of-file\n" );
			else if ( yychar < 0 )
				printf( "-none-\n" );
			else
			{
				for ( yy_i = 0; yytoks[yy_i].t_val >= 0;
					yy_i++ )
				{
					if ( yytoks[yy_i].t_val == yychar )
						break;
				}
				printf( "%s\n", yytoks[yy_i].t_name );
			}
		}
#endif /* YYDEBUG */
		if ( ( ( yy_n += yychar ) < 0 ) || ( yy_n >= YYLAST ) )
			goto yydefault;
		if ( yychk[ yy_n = yyact[ yy_n ] ] == yychar )	/*valid shift*/
		{
			yychar = -1;
			yyval = yylval;
			yy_state = yy_n;
			if ( yyerrflag > 0 )
				yyerrflag--;
			goto yy_stack;
		}

	yydefault:
		if ( ( yy_n = yydef[ yy_state ] ) == -2 )
		{
#if YYDEBUG
			yytmp = yychar < 0;
#endif
			if ( ( yychar < 0 ) && ( ( yychar = yylex() ) < 0 ) )
				yychar = 0;		/* reached EOF */
#if YYDEBUG
			if ( yydebug && yytmp )
			{
				register int yy_i;

				printf( "Received token " );
				if ( yychar == 0 )
					printf( "end-of-file\n" );
				else if ( yychar < 0 )
					printf( "-none-\n" );
				else
				{
					for ( yy_i = 0;
						yytoks[yy_i].t_val >= 0;
						yy_i++ )
					{
						if ( yytoks[yy_i].t_val
							== yychar )
						{
							break;
						}
					}
					printf( "%s\n", yytoks[yy_i].t_name );
				}
			}
#endif /* YYDEBUG */
			/*
			** look through exception table
			*/
			{
				register int *yyxi = yyexca;

				while ( ( *yyxi != -1 ) ||
					( yyxi[1] != yy_state ) )
				{
					yyxi += 2;
				}
				while ( ( *(yyxi += 2) >= 0 ) &&
					( *yyxi != yychar ) )
					;
				if ( ( yy_n = yyxi[1] ) < 0 )
					YYACCEPT;
			}
		}

		/*
		** check for syntax error
		*/
		if ( yy_n == 0 )	/* have an error */
		{
			/* no worry about speed here! */
			switch ( yyerrflag )
			{
			case 0:		/* new error */
#ifdef __cplusplus
				yyerror( gettxt("uxlibc:81", "syntax error") );
#else
				yyerror( gettxt("uxlibc:81", "Syntax error") );
#endif
				goto skip_init;
			yyerrlab:
				/*
				** get globals into registers.
				** we have a user generated syntax type error
				*/
				yy_pv = yypv;
				yy_ps = yyps;
				yy_state = yystate;
				yynerrs++;
				/* FALLTHRU */
			skip_init:
			case 1:
			case 2:		/* incompletely recovered error */
					/* try again... */
				yyerrflag = 3;
				/*
				** find state where "error" is a legal
				** shift action
				*/
				while ( yy_ps >= yys )
				{
					yy_n = yypact[ *yy_ps ] + YYERRCODE;
					if ( yy_n >= 0 && yy_n < YYLAST &&
						yychk[yyact[yy_n]] == YYERRCODE)					{
						/*
						** simulate shift of "error"
						*/
						yy_state = yyact[ yy_n ];
						goto yy_stack;
					}
					/*
					** current state has no shift on
					** "error", pop stack
					*/
#if YYDEBUG
#	define _POP_ "Error recovery pops state %d, uncovers state %d\n"
					if ( yydebug )
						printf( _POP_, *yy_ps,
							yy_ps[-1] );
#	undef _POP_
#endif
					yy_ps--;
					yy_pv--;
				}
				/*
				** there is no state on stack with "error" as
				** a valid shift.  give up.
				*/
				YYABORT;
			case 3:		/* no shift yet; eat a token */
#if YYDEBUG
				/*
				** if debugging, look up token in list of
				** pairs.  0 and negative shouldn't occur,
				** but since timing doesn't matter when
				** debugging, it doesn't hurt to leave the
				** tests here.
				*/
				if ( yydebug )
				{
					register int yy_i;

					printf( "Error recovery discards " );
					if ( yychar == 0 )
						printf( "token end-of-file\n" );
					else if ( yychar < 0 )
						printf( "token -none-\n" );
					else
					{
						for ( yy_i = 0;
							yytoks[yy_i].t_val >= 0;
							yy_i++ )
						{
							if ( yytoks[yy_i].t_val
								== yychar )
							{
								break;
							}
						}
						printf( "token %s\n",
							yytoks[yy_i].t_name );
					}
				}
#endif /* YYDEBUG */
				if ( yychar == 0 )	/* reached EOF. quit */
					YYABORT;
				yychar = -1;
				goto yy_newstate;
			}
		}/* end if ( yy_n == 0 ) */
		/*
		** reduction by production yy_n
		** put stack tops, etc. so things right after switch
		*/
#if YYDEBUG
		/*
		** if debugging, print the string that is the user's
		** specification of the reduction which is just about
		** to be done.
		*/
		if ( yydebug )
			printf( "Reduce by (%d) \"%s\"\n",
				yy_n, yyreds[ yy_n ] );
#endif
		yytmp = yy_n;			/* value to switch over */
		yypvt = yy_pv;			/* $vars top of value stack */
		/*
		** Look in goto table for next state
		** Sorry about using yy_state here as temporary
		** register variable, but why not, if it works...
		** If yyr2[ yy_n ] doesn't have the low order bit
		** set, then there is no action to be done for
		** this reduction.  So, no saving & unsaving of
		** registers done.  The only difference between the
		** code just after the if and the body of the if is
		** the goto yy_stack in the body.  This way the test
		** can be made before the choice of what to do is needed.
		*/
		{
			/* length of production doubled with extra bit */
			register int yy_len = yyr2[ yy_n ];

			if ( !( yy_len & 01 ) )
			{
				yy_len >>= 1;
				yyval = ( yy_pv -= yy_len )[1];	/* $$ = $1 */
				yy_state = yypgo[ yy_n = yyr1[ yy_n ] ] +
					*( yy_ps -= yy_len ) + 1;
				if ( yy_state >= YYLAST ||
					yychk[ yy_state =
					yyact[ yy_state ] ] != -yy_n )
				{
					yy_state = yyact[ yypgo[ yy_n ] ];
				}
				goto yy_stack;
			}
			yy_len >>= 1;
			yyval = ( yy_pv -= yy_len )[1];	/* $$ = $1 */
			yy_state = yypgo[ yy_n = yyr1[ yy_n ] ] +
				*( yy_ps -= yy_len ) + 1;
			if ( yy_state >= YYLAST ||
				yychk[ yy_state = yyact[ yy_state ] ] != -yy_n )
			{
				yy_state = yyact[ yypgo[ yy_n ] ];
			}
		}
					/* save until reenter driver code */
		yystate = yy_state;
		yyps = yy_ps;
		yypv = yy_pv;
	}
	/*
	** code supplied by user is placed in this switch
	*/
	switch( yytmp )
	{
		
case 3:
# line 96 "gram.y"
 {
			(void) lookup(yypvt[-2].string, INSERT, yypvt[-0].namel);
		} break;
case 4:
# line 99 "gram.y"
 {
			insert((char *)NULL, yypvt[-3].namel, yypvt[-1].namel, yypvt[-0].subcmd);
		} break;
case 5:
# line 102 "gram.y"
 {
			insert(yypvt[-5].string, yypvt[-3].namel, yypvt[-1].namel, yypvt[-0].subcmd);
		} break;
case 6:
# line 105 "gram.y"
 {
			append((char *)NULL, yypvt[-3].namel, yypvt[-1].string, yypvt[-0].subcmd);
		} break;
case 7:
# line 108 "gram.y"
 {
			append(yypvt[-5].string, yypvt[-3].namel, yypvt[-1].string, yypvt[-0].subcmd);
		} break;
case 9:
# line 114 "gram.y"
{ 
			yyval.namel = yypvt[-0].namel; 
		} break;
case 10:
# line 117 "gram.y"
{ 
			yyval.namel = subnl(yypvt[-2].namel, yypvt[-0].namel); 
		} break;
case 11:
# line 120 "gram.y"
{ 
			yyval.namel = addnl(yypvt[-2].namel, yypvt[-0].namel); 
		} break;
case 12:
# line 123 "gram.y"
{ 
			yyval.namel = andnl(yypvt[-2].namel, yypvt[-0].namel); 
		} break;
case 13:
# line 128 "gram.y"
 {
			yyval.namel = makenl(yypvt[-0].string);
		} break;
case 14:
# line 131 "gram.y"
 {
			yyval.namel = yypvt[-1].namel;
		} break;
case 15:
# line 136 "gram.y"
{
			yyval.namel = last_n = NULL;
		} break;
case 16:
# line 139 "gram.y"
 {
			if (last_n == NULL)
				yyval.namel = last_n = makenl(yypvt[-0].string);
			else {
				last_n->n_next = makenl(yypvt[-0].string);
				last_n = last_n->n_next;
				yyval.namel = yypvt[-1].namel;
			}
		} break;
case 17:
# line 150 "gram.y"
{
			yyval.subcmd = last_sc = NULL;
		} break;
case 18:
# line 153 "gram.y"
 {
			if (last_sc == NULL)
				yyval.subcmd = last_sc = yypvt[-0].subcmd;
			else {
				last_sc->sc_next = yypvt[-0].subcmd;
				last_sc = yypvt[-0].subcmd;
				yyval.subcmd = yypvt[-1].subcmd;
			}
		} break;
case 19:
# line 164 "gram.y"
 {
			register struct namelist *nl;

			yypvt[-3].subcmd->sc_options = yypvt[-2].optval | options;
			if (yypvt[-1].namel != NULL) {
				nl = expand(yypvt[-1].namel, E_VARS);
				if (nl) {
					if (nl->n_next != NULL)
					    yyerror("only one name allowed\n");
					yypvt[-3].subcmd->sc_name = nl->n_name;
					free(nl);
				} else
					yypvt[-3].subcmd->sc_name = NULL;
			}
			yyval.subcmd = yypvt[-3].subcmd;
		} break;
case 20:
# line 180 "gram.y"
 {
			if (yypvt[-1].namel != NULL)
				yypvt[-2].subcmd->sc_args = expand(yypvt[-1].namel, E_VARS);
			yyval.subcmd = yypvt[-2].subcmd;
		} break;
case 21:
# line 185 "gram.y"
 {
			if (yypvt[-1].namel != NULL)
				yypvt[-2].subcmd->sc_args = expand(yypvt[-1].namel, E_ALL);
			yyval.subcmd = yypvt[-2].subcmd;
		} break;
case 22:
# line 190 "gram.y"
 {
			struct namelist *nl;
			char *cp, *re_comp();

			for (nl = yypvt[-1].namel; nl != NULL; nl = nl->n_next)
				if ((cp = re_comp(nl->n_name)) != NULL)
					yyerror(cp);
			yypvt[-2].subcmd->sc_args = expand(yypvt[-1].namel, E_VARS);
			yyval.subcmd = yypvt[-2].subcmd;
		} break;
case 23:
# line 200 "gram.y"
 {
			if (yypvt[-2].namel != NULL)
				yypvt[-3].subcmd->sc_args = expand(yypvt[-2].namel, E_ALL);
			yypvt[-3].subcmd->sc_name = yypvt[-1].string;
			yyval.subcmd = yypvt[-3].subcmd;
		} break;
case 24:
# line 206 "gram.y"
 {
			if (yypvt[-2].namel != NULL)
				yypvt[-3].subcmd->sc_args = expand(yypvt[-2].namel, E_ALL);
			yypvt[-3].subcmd->sc_name = yypvt[-1].string;
			yyval.subcmd = yypvt[-3].subcmd;
		} break;
case 25:
# line 214 "gram.y"
 {
			yyval.optval = 0;
		} break;
case 26:
# line 217 "gram.y"
 {
			yyval.optval |= yypvt[-0].optval;
		} break;
case 27:
# line 222 "gram.y"
 {
			yyval.namel = NULL;
		} break;
case 28:
# line 225 "gram.y"
 {
			yyval.namel = yypvt[-0].namel;
		} break;
	}
	goto yystack;		/* reset registers in driver code */
}
