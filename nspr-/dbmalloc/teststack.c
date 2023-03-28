/*
 * (c) Copyright 1990, 1991, 1992 Conor P. Cahill (cpcahil@virtech.vti.com)
 *
 * This software may be distributed freely as long as the following conditions
 * are met:
 * 		* the distribution, or any derivative thereof, may not be
 *		  included as part of a commercial product
 *		* full source code is provided including this copyright
 *		* there is no charge for the software itself (there may be
 *		  a minimal charge for the copying or distribution effort)
 *		* this copyright notice is not modified or removed from any
 *		  source file
 */

#include "sysdefs.h"
#include <stdio.h>
#if ANSI_HEADERS
#include <stdlib.h>
#endif
#include <sys/types.h>
#include "malloc.h"

VOIDTYPE	sub1();
VOIDTYPE	sub2();
VOIDTYPE	sub3();

/*ARGSUSED*/
int
main(argc,argv)
	int			  argc;
	char			**argv[];
{

	char			* s;

	malloc_enter("main");

	s = malloc(10);

	sub1();
	sub2();
	sub3();
	
	malloc_leave("main");
	
	malloc_dump(1);

	return(0);
}

VOIDTYPE
sub1()
{
	char 	* s;
	malloc_enter("sub1");

	s = malloc(0);	

	sub2();

	sub3();
	
	sub2();

	s = malloc(10);

	malloc_leave("sub1");
}

VOIDTYPE
sub2()
{
	char 	* s;
	malloc_enter("sub2");

	s = malloc(0);	

	sub3();
	
	s = malloc(10);

	malloc_leave("sub2");
}

VOIDTYPE
sub3()
{
	char 	* s;
	malloc_enter("sub3");

	s = malloc(1);	

	strcpy(s,"1");

	malloc_leave("sub3");
}

