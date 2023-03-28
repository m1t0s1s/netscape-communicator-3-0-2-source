/* NOT copyright by SoftQuad Inc. -- msb, 1988 */
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
#ifndef lint
static char *SQ_SccsId = "@(#)mtest3.c	1.2 88/08/25";
#endif
#include <stdio.h>
#include <sys/types.h>
/*
** looptest.c -- intensive allocator tester 
**
** Usage:  looptest
**
** History:
**	4-Feb-1987 rtech!daveb 
*/

#ifndef HAS_VADVISE
#ifdef random
#undef random
#endif
/*# define random	rand*/
#else
# include <sys/vadvise.h>
#endif

# include <math.h>
# include <stdio.h>
# include <signal.h>
# include <setjmp.h>

#include "malloc.h"

# define MAXITER	1000000		/* main loop iterations */
# define MAXOBJS	1000		/* objects in pool */
# define BIGOBJ		90000		/* max size of a big object */
# define TINYOBJ	80		/* max size of a small object */
# define BIGMOD		100		/* 1 in BIGMOD is a BIGOBJ */
# define STATMOD	10000		/* interation interval for status */

int
main( argc, argv )
int argc;
char **argv;
{
	register int	 **objs;	/* array of objects */
	register SIZETYPE *sizes;	/* array of object sizes */
	register SIZETYPE n;		/* iteration counter */
	register SIZETYPE i;		/* object index */
	register SIZETYPE size;		/* object size */
	register SIZETYPE r;		/* random number */

	SIZETYPE objmax;		/* max size this iteration */
	long cnt;			/* number of allocated objects */
	long nm = 0;			/* number of mallocs */
	long nre = 0;			/* number of reallocs */
	long startsize;			/* size at loop start */
	long endsize;			/* size at loop exit */
	long maxiter = 0;		/* real max # iterations */

	double * dblptr;		/* pointer for doubleword test */

	extern char end;		/* memory before heap */
	char *sbrk();
	long atol();
	union dbmalloptarg m;

#ifdef HAS_VADVISE
	/* your milage may vary... */
	vadvise( VA_ANOM );
#endif

	/*
	 * test that allocation can be used for doubleword manipulation
	 */
	dblptr = (double *) malloc(sizeof(double));
	if( (dblptr == 0) || ((*dblptr=10.0) != 10.0) )
	{
		fprintf(stderr,"Doubleword test failed\n");
		exit(1);
	}
	free( (DATATYPE *) dblptr);

	if (argc > 1)
		maxiter = atol (argv[1]);
	if (maxiter <= 0)
		maxiter = MAXITER;

	printf("MAXITER %ld MAXOBJS %d ", maxiter, MAXOBJS );
	printf("BIGOBJ %ld, TINYOBJ %d, nbig/ntiny 1/%d\n",
	BIGOBJ, TINYOBJ, BIGMOD );
	fflush( stdout );

	if( NULL == (objs = (int **)calloc( MAXOBJS, sizeof( *objs ) ) ) )
	{
		fprintf(stderr, "Can't allocate memory for objs array\n");
		exit(1);
	}

	if( NULL == ( sizes = (SIZETYPE *)calloc( MAXOBJS, sizeof( *sizes ) ) ))
	{
		fprintf(stderr, "Can't allocate memory for sizes array\n");
		exit(1);
	}

	/* as per recent discussion on net.lang.c, calloc does not 
	** necessarily fill in NULL pointers...
	*/
	for( i = 0; i < MAXOBJS; i++ )
		objs[ i ] = NULL;

	startsize = sbrk(0) - &end;
	printf( "Memory use at start: %ld bytes\n", startsize );
	fflush(stdout);

	printf("Starting the test...\n");
	fflush(stdout);
	for( n = 0; n < maxiter ; n++ )
	{
		if( !(n % STATMOD) )
		{
			printf("%ld iterations\n", n);
			fflush(stdout);
		}

		/* determine object of interest and its size */

		r = random();
		objmax = ( r % BIGMOD ) ? TINYOBJ : BIGOBJ;
		size = r % objmax;
		i = r % (MAXOBJS - 1);

		/*
		 * make sure size is at least one byte
		 */
		if( size == 0 )
		{
			size++;
		}
		/* either replace the object or get a new one */

		if( objs[ i ] == NULL )
		{
			if( (i % 5) == 0 )
			{
				objs[ i ] = (int *)memalign(1024, size );
			}
			else if( (i % 4) == 0 )
			{
				objs[ i ] = (int *)calloc(1, size );
			}
			else if( (i%3) == 0 )
			{
				objs[ i ] = (int *)XtCalloc(1, size );
			}
			else if( (i%2) == 0 )
			{
				objs[ i ] = (int *)XtMalloc( size );
			}
			else
			{
				objs[ i ] = (int *)malloc( size );
			}
			nm++;
		}
		else
		{
			/* don't keep bigger objects around */
			if( size > sizes[ i ] )
			{
				if( (i & 0x01) == 0 )
				{
					objs[ i ] = (int *)realloc(
					        (DATATYPE *) objs[ i ], size );
				}
				else
				{
					objs[ i ] = (int *)XtRealloc(
					        (DATATYPE *) objs[ i ], size );
				}
				nre++;
			}
			else
			{
				if( (i & 0x01) == 0 )
				{
					free( (DATATYPE *) objs[ i ] );
				}
				else
				{
					XtFree( (DATATYPE *) objs[ i ] );
				}
				objs[ i ] = (int *)malloc( size );
				nm++;
			}
		}

		sizes[ i ] = size;
		if( objs[ i ] == NULL )
		{
			printf("\nCouldn't allocate %ld byte object!\n", 
				size );
			break;
		}
	} /* for() */

	printf( "\n" );
	cnt = 0;
	for( i = 0; i < MAXOBJS; i++ )
		if( objs[ i ] )
			cnt++;

	printf( "Did %ld iterations, %d objects, %d mallocs, %d reallocs\n",
		n, cnt, nm, nre );
	printf( "Memory use at end: %ld bytes\n", sbrk(0) - &end );
	fflush( stdout );

	/* free all the objects */
	for( i = 0; i < MAXOBJS; i++ )
	{
		if( objs[ i ] != NULL )
		{
				if( (i & 0x01) == 0 )
				{
					free( (DATATYPE *) objs[ i ] );
				}
				else
				{
					XtFree( (DATATYPE *) objs[ i ] );
				}
		}
	}

	endsize = sbrk(0) - &end;
	printf( "Memory use after free: %ld bytes\n", endsize );
	fflush( stdout );

	if( startsize != endsize )
		printf("startsize %ld != endsize %d\n", startsize, endsize );

	free( (DATATYPE *) objs );
	free( (DATATYPE *) sizes );

	/*
	 * make sure detail mode is enabled
	 */	
	m.i = 1;
	dbmallopt(MALLOC_DETAIL,&m);

	malloc_dump(2);
	exit( 0 );
	/*NOTREACHED*/
}

/*
 * Fake X Windows stuff in order to get xmalloc stuff to link correctly.
 */
char * XtCXtToolkitError = "junk string";
int
XtErrorMsg()
{
	return(0);
}
