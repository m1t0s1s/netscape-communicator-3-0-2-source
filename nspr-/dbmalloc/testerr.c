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

#define ALLOCSIZE	20
#define BIG_ALIGN	(4*1024)
#define SMALL_ALIGN	(1*1024)

/*ARGSUSED*/
int
main(argc,argv)
	int			  argc;
	char			**argv[];
{

	union dbmalloptarg	  m;
	unsigned long		  oldsize;
	char			* s;
	unsigned long		  size;
	char			* t;
	char			* u;


	/*
	 * make sure we have both chain checking and fill area enabled
	 */
	m.i = 1;
	dbmallopt(MALLOC_CKCHAIN,&m);
	m.i = 3;
	dbmallopt(MALLOC_FILLAREA,&m);

	/*
	 * test leak detection software
	 */
	fprintf(stderr,"-------------------------------------\n");
	fprintf(stderr,"Testing malloc_inuse()...");
	oldsize = malloc_inuse( (unsigned long *)0);
	s = malloc(ALLOCSIZE);
	size = malloc_inuse( (unsigned long *)0);
	if( size != (oldsize + ALLOCSIZE))
	{
		fprintf(stderr,"ERROR\n");
		fprintf(stderr,"\toldsize = %lu, size = %lu - should be %lu\n",
			oldsize, size, oldsize+ALLOCSIZE);
	}
	else
	{
		fprintf(stderr,"OK\n");
	}
	
	fprintf(stderr,"-------------------------------------\n");
	fprintf(stderr,"Testing malloc_mark()...");
	malloc_mark(s);
	size = malloc_inuse( (unsigned long *) 0);
	if( size != oldsize )
	{
		fprintf(stderr,"ERROR\n");
		fprintf(stderr,"\tsize = %lu, should be %lu\n",size,oldsize);
	}
	else
	{
		fprintf(stderr,"OK\n");
	}

	/*
	 * test new malloc_size function
	 */
	fprintf(stderr,"-------------------------------------\n");
	fprintf(stderr,"Testing malloc_size()...");
	size = malloc_size(s);
	if( size != ALLOCSIZE )
	{
		fprintf(stderr,"ERROR\n");
		fprintf(stderr,"\tsize = %lu, should be %d\n",size,ALLOCSIZE);
	}
	else
	{
		fprintf(stderr,"OK\n");
	}
			
		
	/*
	 * test memalign
	 */
	fprintf(stderr,"-------------------------------------\n");
	fprintf(stderr,"Testing memalign()...");
	s = memalign((SIZETYPE)BIG_ALIGN,(SIZETYPE)10);
	t = memalign((SIZETYPE)BIG_ALIGN,(SIZETYPE)20);
	if( (s == NULL) || ((((SIZETYPE)s)&(BIG_ALIGN-1)) != 0)
			|| ((((SIZETYPE)t)&(SMALL_ALIGN-1)) != 0) )
	{
		fprintf(stderr,"ERROR\n");
		fprintf(stderr,"\ts = 0x%lx(align=%d), t = 0x%lx(align=%d)\n",
			s, BIG_ALIGN, t, SMALL_ALIGN);
	}
	else
	{
		fprintf(stderr,"OK\n");
	}
	
	s = memalign((SIZETYPE)4096,(SIZETYPE)10);
	

	t = malloc(20);

	s = malloc(10);

	fprintf(stderr,"-------------------------------------\n");
	fprintf(stderr,"Error from strcpy() - out of bounds\n");
	fflush(stderr);
	strncpy(s," ",11);

	fprintf(stderr,"-------------------------------------\n");
	fprintf(stderr,"Error from memset() - out of bounds (beyond)\n");
	fflush(stderr);
	memset(t,' ',21);

	fprintf(stderr,"-------------------------------------\n");
	fprintf(stderr,"Error from free() - overrun\n");
	fflush(stderr);
	free(t);

	fprintf(stderr,"-------------------------------------\n");
	fprintf(stderr,"Error from free() - double free\n");
	fflush(stderr);
	free(t);

	fprintf(stderr,"-------------------------------------\n");
	fprintf(stderr,"NO error from bzero\n");
	fflush(stderr);
	bzero(s,10);

	fprintf(stderr,"-------------------------------------\n");
	fprintf(stderr,"Error from bzero() - out of bounds\n");
	fflush(stderr);
	bzero(s,11);

	fprintf(stderr,"-------------------------------------\n");
	fprintf(stderr,"Error from free() - overrun\n");
	fflush(stderr);
	free(s);

	u = malloc(1);

	fprintf(stderr,"-------------------------------------\n");
	fprintf(stderr,"Error from memset() - out of bounds (before)\n");
	fflush(stderr);
	memset(u-2,' ',3);

	fprintf(stderr,"-------------------------------------\n");
	fprintf(stderr,"Error from free() - underrun\n");
	fflush(stderr);
	free(u);

	s = malloc(10);
	t = malloc(500); 	/* do this to make sure memset doesnt core */

	fprintf(stderr,"-------------------------------------\n");
	fprintf(stderr,"Error from memset() - out of bounds\n");
	fflush(stderr);
	memset(s,'1',100);

	
	fprintf(stderr,"-------------------------------------\n");
	fprintf(stderr,"Error from malloc() - chain broken\n");
	fflush(stderr);
	t = malloc(10);
	
	return(0);
}

