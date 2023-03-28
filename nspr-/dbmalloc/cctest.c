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
#if __STDC__ || __cplusplus
# define __stdcargs(s) s
#else
# define __stdcargs(s) ()
#endif

#ifdef USE_STDLIB
#include <stdlib.h>
#endif
#ifdef USE_UNISTD
#include <unistd.h>
#endif
#ifdef USE_MALLOC
#include <malloc.h>
#endif
#ifdef USE_MEMORY_H
#include <memory.h>
#endif
#ifdef USE_STRING_H
#include <string.h>
#endif
#ifdef USE_SYSENT
#include <sysent.h>
#endif


/*
 * $Id: cctest.c,v 1.1 1996/06/18 03:29:12 warren Exp $
 */
/*
 * This file is not a real source file for the malloc library.  The
 * configuration utility uses this file to test the various compiler
 * settings that can be used by the library.
 */

#ifdef VOIDTEST
	/*
	 * testing to see if the void datatype is used by this system
	 */
	void *
	function()
	{
		static void * t;

		return(t);
	}
#endif

#ifdef EXITTEST

/*
 * determine the return type of exit
 */
#if __cplusplus
	extern "C" {
#endif
		EXITTYPE exit __stdcargs((int));
#if __cplusplus
	}
#endif
#if __cplusplus || __STDC__
#include <stdio.h>
main(int argc, char *argv[])
#else
main(argc,argv)
	int	  argc;
	char	* argv[];
#endif
{

	/*
	 * this bogus stuff is here simply to get c++ to shut-up about
 	 * unreferenced parameters.
	 */
	if( argv[argc] == "hello" )
	{
		printf("hello\n");
	}
	return(0);
}
#endif /* EXITTEST */

#ifdef SETENVTEST
/*
 * determine if setenv is supported
 */
#if __cplusplus || __STDC__
#include <stdio.h>
main(int argc, char *argv[])
#else
main(argc,argv)
	int	  argc;
	char	* argv[];
#endif
{
#ifdef setenv
#undef setenv
#endif

	setenv("TESTSYM","YES",1);

	/*
	 * this bogus stuff is here simply to get c++ to shut-up about
 	 * unreferenced parameters.
	 */
	if( argv[argc] == "hello" )
	{
		printf("hello\n");
	}
	return(0);
}
#endif /* SETENVTEST */

#ifdef MALLOCHTEST
#include <malloc.h>
#endif
#ifdef ANSIHEADERTEST
#include <stdlib.h>
#endif
#ifdef POSIXHEADERTEST
#include <unistd.h>
#endif

#if defined(MALLOCHTEST) || defined(ANSIHEADERTEST) || defined(POSIXHEADERTEST)
/*
 * determine if certain headers are available
 */
#if __cplusplus || __STDC__
main(int argc, char *argv[])
#else
main(argc,argv)
	int	  argc;
	char	* argv[];
#endif
{
	/*
	 * this bogus stuff is here simply to get c++ to shut-up about
 	 * unreferenced parameters.
	 */
	if( argv[argc] == "hello" )
	{
		printf("hello\n");
	}
	return(0);
}
#endif /* MALLOCHTEST || ANSIHEADERTEST || POSIXHEADERTEST */

#ifdef ASM_UNDTEST
/*
 * test requirement for underscores in external symbols
 */
#if __cplusplus || __STDC__
#include <stdio.h>
main(int argc, char *argv[])
#else
main(argc,argv)
	int	  argc;
	char	* argv[];
#endif
{
	int	  myroutine();

#if i386
	printf("OK\n", myroutine());
#else
	printf("NOT OK\n");
#endif

}

#ifdef i386
	asm("	.globl _myroutine");
	asm("_myroutine:");
	asm("	xor %eax");
	asm("   ret");
#endif


#endif /* ASM_UNDTEST */


#ifdef ASM_REPTEST
/*
 * test requirement for underscores in external symbols
 */
#if __cplusplus || __STDC__
#include <stdio.h>
main(int argc, char *argv[])
#else
main(argc,argv)
	int	  argc;
	char	* argv[];
#endif
{
	int	  myroutine();

#if i386
	printf("OK\n", myroutine());
#else
	printf("NOT OK\n");
#endif

}

#ifdef i386
#ifdef USE_UNDERSCORE
	asm("	.globl _myroutine");
	asm("_myroutine:");
#else
	asm("	.globl myroutine");
	asm("myroutine:");
#endif
	asm("	xor %ecx");
	asm("	repe");
	asm("   ret");
#endif


#endif /* ASM_REPTEST */

#ifdef CONSTTEST
	/*
	 * testing to see if the void datatype is used by this system
	 */
	const char *
	function()
	{
		static const char t[] = "hello";

		return(t);
	}
#endif

#ifdef MALLOC_COMPILETEST

#if __cplusplus
DATATYPE * malloc( SIZETYPE size)
#else
DATATYPE *
malloc( size)
	SIZETYPE 	size;
#endif
{
	if( size > 0 )
	{
		return(0);
	}
	
	return(0);
}

#endif /* MALLOC_COMPILETEST */

#ifdef FREE_COMPILETEST

#if __cplusplus
FREETYPE free( DATATYPE *data)
#else
FREETYPE
free(data)
	DATATYPE *data;
#endif
{
	if( ! data )
	{
		printf("foo\n");
	}
}

#endif /* FREE_COMPILETEST */

#ifdef MEM_COMPILETEST

MEMDATA *memcpy __stdcargs((MEMDATA *ptr1, CONST MEMDATA *ptr2, register MEMSIZE len));

#if __cplusplus
MEMDATA * memccpy(
	MEMDATA		* ptr1,
	CONST MEMDATA	* ptr2,
	int		  ch,
	MEMSIZE		  len )
#else
MEMDATA *
memccpy(ptr1,ptr2,ch,len)
	MEMDATA		* ptr1;
	CONST MEMDATA	* ptr2;
	int		  ch;
	MEMSIZE		  len;
#endif
{
	/*
	 * just make use of all the passed arguments so that we don't get bogus
	 * warning messages about unused arguments.  What we do doesn't have
	 * to make sense since we aren't going to run this.
	 */
	if( (ptr1 == ptr2) && (ch != len) )
	{
		return(ptr1);
	}
	return(memcpy(ptr1,ptr2,len));
}

#endif /* MEM_COMPILETEST */

#ifdef STR_COMPILETEST

#include <string.h>
#if __cplusplus
STRSIZE strlen( CONST char * str1 )
#else
STRSIZE
strlen(str1)
	CONST char * str1;
#endif
{
	if( str1[0] != '\0')
	{
		return(1);
	}
	return(0);
}

#endif /* STR_COMPILETEST */

#ifdef WRT_COMPILETEST
#if __cplusplus
int write(int fd, CONST char * buf, WRTSIZE size)
#else
int
write(fd,buf,size)
	 int		  fd;
	 CONST char	* buf;
	 WRTSIZE	  size;
#endif
{
	if( buf[fd] == (CONST char) size)
	{
		return(1);
	}
	return(0);
}

#endif /* WRT_COMPILETEST */


#ifdef PRE_DEFINES

/*
 * this is used by the Configure script to get the compiler pre-defined symbol
 * for this
 */
#if __cplusplus
#include <stdio.h>
main(int argc, char *argv[])
#else
main(argc,argv)
	int	  argc;
	char	* argv[];
#endif
{
	int	  varcnt = 0;

#if __GNUC__
	if (__GNUC__ > 1)
		printf("(__GNUC__ == %d)", __GNUC__);
	else
		printf("__GNUC__");
	varcnt++;
#endif
#if __STDC__ 
	if( varcnt > 0 )
	{
		printf(" && ");
	}
	printf("__STDC__");
	varcnt++;
#endif
#if __HIGHC__ 
	if( varcnt > 0 )
	{
		printf(" && ");
	}
	printf("__HIGHC__");
	varcnt++;
#endif
#if __C89__ 
	if( varcnt > 0 )
	{
		printf(" && ");
	}
	printf("__C89__");
	varcnt++;
#endif
#if __cplusplus
	if( varcnt > 0 )
	{
		printf(" && ");
	}
	/*
	 * this bogus stuff is here simply to get c++ to shut-up about
 	 * unreferenced parameters.
	 */
	if( argv[argc] == "hello" )
	{
		printf("hello\n");
	}
	printf("__cplusplus");
	varcnt++;
#endif

	/*
	 * if we found no pre-defines, print out the word none, so we can tell the
	 * difference between compilation failures and no pre-defs.
	 */
	if( varcnt == 0 )
	{
		printf("none");
		varcnt++;
	}

	if( varcnt > 0 )
	{
		printf("\n");
	}
	return(0);
}

#endif /* PRE_DEFINES */
	

#ifdef SIGIOTTEST
#include <sys/types.h>
#include <signal.h>
	int
	function()
	{
		int signal = SIGIOT;
		return(signal);
	}
#endif
#ifdef SIGABRTTEST
#include <sys/types.h>
#include <signal.h>
	int
	function()
	{
		int signal = SIGABRT;
		return(signal);
	}
#endif

#ifdef CHANGESTR

#include <stdio.h>

#define FILEBUFSIZE	(50*1024)

char iobuffer[FILEBUFSIZE];

main(argc,argv)
	int		  argc;
	char		* argv[];
{
	unsigned int	  cnt;
	FILE		* fp;
	unsigned int	  i;
	int		  len;
	char		* src;
	char		* tgt;

	if( argc != 4 )
	{
		fprintf(stderr,"Usage: changestr file oldstr newstr\n");
		exit(1);
	}
	src = argv[2];
	tgt = argv[3];

	/*
	 * get length of strings (note that we don't ensure that both strings
	 * are the same length and
	 */
	len = strlen(src);
	i   = strlen(tgt);
	if( i > len )
	{
		fprintf(stderr,"Error: second string cannot be longer %s\n",
				"than first string");
		exit(2);
	}

	fp = fopen(argv[1],"r+");

	if( fp == NULL )
	{
		fprintf(stderr,"Can't open %s\n",argv[1]);
		exit(3);
	}

	/*
	 * read the entire file in (note that if the file is bigger
	 * than the specified blocksize, we won't be able to
	 * process it.
	 */

	cnt = fread(iobuffer,1,FILEBUFSIZE,fp);
	if( cnt <= 0 )
	{
		fprintf(stderr,"Read error when reading %s\n",argv[1]);
		exit(4);
	}

	for(i=0; i < (cnt-len); i++)
	{
		if( memcmp(iobuffer+i,src,len) == 0 )
		{
			memcpy(iobuffer+i,tgt,len);
			i += len-1;
		}
	}

	if( fseek(fp,0L,0) != 0 )
	{
		fprintf(stderr,"Failed to seek to correct location\n");
		exit(5);
	}

	if( fwrite(iobuffer,1,cnt,fp) != cnt )
	{
		fprintf(stderr,"Failed to write new data\n");
		exit(6);
	}

	fclose(fp);
	
	exit(0);
} 


#endif /* CHNAGESTR */


#ifdef TESTDATAMC

#include <stdio.h>

main(argc,argv)
	int		  argc;
	char		* argv[];
{
	char		  buffer[30];

	buffer[0] = '\0';
	buffer[1] = '\0';
	buffer[2] = '\0';
	buffer[3] = '\0';
	buffer[4] = '\0';
	buffer[5] = '\0';
	buffer[6] = '\0';
	buffer[7] = '\0';
	buffer[8] = '\0';

	DataMC(buffer, "   y",5);
	DataMC(buffer+4, "yy",3);

	DataMC(buffer+1, buffer,7);
	DataMC(buffer,   "x",1);

	printf("%s\n",buffer);

	return(0);
}

/*
 * we need to have our own memcpy here in order to find out if there will be
 * some problem where the kludged version of memcpy (now should be named 
 * DataMC) at least one system (SGI) has gotten into an infinite loop
 * when the modified DataMC ended up calling the library's memcpy
 */
memcpy()
{
	write(1,"Infinite loop\n",14);
	exit(1);
}

#endif /* TESTDATAMC */

#ifdef TESTDATAMS
#include <stdio.h>

main(argc,argv)
	int		  argc;
	char		* argv[];
{
	char		  buffer[30];

	buffer[0] = '\0';
	buffer[1] = '\0';
	buffer[2] = '\0';
	buffer[3] = '\0';
	buffer[4] = '\0';
	buffer[5] = '\0';
	buffer[6] = '\0';
	buffer[7] = '\0';
	buffer[8] = '\0';
	buffer[9] = '\0';
	buffer[10] = '\0';

	DataMS(buffer,  'x',1);
	DataMS(buffer+1,' ',3);
	DataMS(buffer+4,'y',3);

	printf("%s\n",buffer);
}

memset()
{
	write(1,"Infinite loop\n",14);
	exit(1);
}

#endif /* TESTDATAMS */

#ifdef COMPARETEST 

#include <stdio.h>
#include <string.h>

#if __cplusplus
#include <stdlib.h>
main(int argc, char *argv[])
#else
main(argc,argv)
	int	  argc;
	char	* argv[];
#endif
{
	char		  buffer[10];
	char		  buf2[10];
	int		  result;

	buffer[0] = 'a';
	buffer[1] = '\0';
	buf2[0]   = 0xff;
	buf2[1] = '\0';

	/*
	 * just to get c++ and some ANSI C compilers to shutup.  argc will
	 * be more than 1 when running this test.
	 */
	if( argc > 10 )
	{
		result = strcmp(argv[0],"junkstr");
	}
	else
	{
		result = COMPARETEST (buffer,buf2,1);
	}

#ifdef TESTCHAR
	result = -result;
#endif

	if( result < 0 )
	{
		exit(0);
	}
	else
	{
		exit(1);
	}

}
	

#endif /* COMPARETEST */
