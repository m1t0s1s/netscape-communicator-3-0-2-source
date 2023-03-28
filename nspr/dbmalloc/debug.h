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
/************************************************************************/
/*									*/
/* this include sets up some macro functions which can be used while	*/
/* debugging the program, and then left in the code, but turned of by	*/
/* just not defining "DEBUG".  This way your production version of 	*/
/* the program will not be filled with bunches of debugging junk	*/
/*									*/
/************************************************************************/
/*
 * $Id: debug.h,v 1.1 1996/06/18 03:29:14 warren Exp $
 */

#undef DEBUG	/* XXX hack -- I couldn't figure out how to undef DEBUG in the Makefile -whh */
#ifdef DEBUG

#if DEBUG == 1			/* if default level			*/
#undef DEBUG
#define DEBUG	100		/*   use level 100			*/
#endif

#include <stdio.h>

#define DEBUG0(val,str)\
				{\
				  if( DEBUG > val ) \
				    fprintf(stderr,"%s(%d): %s\n",\
						__FILE__,__LINE__,str);\
				}
#define DEBUG1(val,str,a1)\
			        {\
				  char _debugbuf[100];\
				  if( DEBUG > val )\
				   {\
				    sprintf(_debugbuf,str,a1);\
				    fprintf(stderr,"%s(%d): %s\n",\
						__FILE__,__LINE__,_debugbuf);\
				   }\
		       		}

#define DEBUG2(val,str,a1,a2)\
			        {\
				 char _debugbuf[100];\
				  if( DEBUG > val )\
				   {\
				    sprintf(_debugbuf,str,a1,a2);\
				    fprintf(stderr,"%s(%d): %s\n",\
						__FILE__,__LINE__,_debugbuf);\
				   }\
		       		}

#define DEBUG3(val,str,a1,a2,a3)\
			        {\
				  char _debugbuf[100];\
				  if( DEBUG > val )\
				   {\
				    sprintf(_debugbuf,str,a1,a2,a3);\
				    fprintf(stderr,"%s(%d): %s\n",\
						__FILE__,__LINE__,_debugbuf);\
				   }\
		       		}

#define DEBUG4(val,str,a1,a2,a3,a4)\
			         {\
				  char _debugbuf[100];\
				  if( DEBUG > val )\
				   {\
				    sprintf(_debugbuf,str,a1,a2,a3,a4);\
				    fprintf(stderr,"%s(%d): %s\n",\
						__FILE__,__LINE__,_debugbuf);\
				   }\
		       		}

#define DEBUG5(val,str,a1,a2,a3,a4,a5)\
			         {\
				  char _debugbuf[100];\
				  if( DEBUG > val )\
				   {\
				    sprintf(_debugbuf,str,a1,a2,a3,a4,a5);\
				    fprintf(stderr,"%s(%d): %s\n",\
						__FILE__,__LINE__,_debugbuf);\
				   }\
		       		}

#else

#define DEBUG0(val,s)
#define DEBUG1(val,s,a1)
#define DEBUG2(val,s,a1,a2)
#define DEBUG3(val,s,a1,a2,a3)
#define DEBUG4(val,s,a1,a2,a3,a4)
#define DEBUG5(val,s,a1,a2,a3,a4,a5)

#endif /* DEBUG */


/*
 * $Log: debug.h,v $
 * Revision 1.1  1996/06/18 03:29:14  warren
 * Added debug malloc package.
 *
 * Revision 1.5  1992/08/22  16:27:13  cpcahil
 * final changes for pl14
 *
 * Revision 1.4  1992/04/13  03:06:33  cpcahil
 * Added Stack support, marking of non-leaks, auto-config, auto-testing
 *
 * Revision 1.3  1991/11/25  14:41:51  cpcahil
 * Final changes in preparation for patch 4 release
 *
 * Revision 1.2  90/05/11  00:13:08  cpcahil
 * added copyright statment
 * 
 * Revision 1.1  90/02/23  07:09:01  cpcahil
 * Initial revision
 * 
 */
