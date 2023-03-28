/* -*- Mode: C; tab-width: 4; -*- */
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>

#ifdef __MWERKS__
#include <Types.h>
#endif

#include "xp_trace.h"
#include "xp_mcom.h"

#if defined(__sun)
#include <nspr/sunos4.h>
#endif

#ifdef XP_UNIX
/* jwz: stderr can't be used as a top-level lvalue on RH 6.1 */
/* FILE *real_stderr = stderr;*/
FILE *real_stderr = 0;

#ifdef DEBUG
void XP_Trace (const char* message, ...)
{
	int len;
    char buffer [2000];
    va_list stack;

    va_start (stack, message);
    (void) vsprintf (buffer, message, stack);
    va_end (stack);

	len = XP_STRLEN(buffer); /* vsprintf does not return length */

    fwrite(buffer, 1, len, real_stderr);

	if (buffer[len-1] != '\n')
        fprintf (real_stderr, "\n");
}
#endif /* DEBUG */
#endif /* XP_UNIX */

/*-----------------------------------------------------------------------------
	Macintosh XP_Trace
	Always exists, doesn't do anything unless in DEBUG
-----------------------------------------------------------------------------*/


#ifdef XP_MAC

#ifdef DEBUG

extern OSErr	InitLibraryManager( size_t poolsize, int zoneType, int memType );
extern void		CleanupLibraryManager( void );
extern void 	Trace( const char *, ... );

Boolean ASLMInited = FALSE;

extern void InitializeASLM()
{
	if ( !ASLMInited )
	{
		OSErr err = InitLibraryManager( 0, 4, 1 );
		if ( err == noErr ) 
		{
			ASLMInited = TRUE;
			XP_TRACE(("\n\nHello world!\n\n"));
		}
	}
}

extern void CleanupASLM()
{
	if ( ASLMInited )
		CleanupLibraryManager();
	ASLMInited = FALSE;
}

extern void XP_Trace( const char *format, ... )
{
	static char buffer[ 10000 ];
	char*		current;
	
	va_list			stack;
	va_start( stack, format );
	
	if ( !ASLMInited )
		return;
		
	buffer[0] = vsprintf( buffer + 1, format, stack );
	va_end( stack );
	for ( current = buffer + 1; *current; current++ )
		if ( *current == 10 )
			*current = 13;
	Trace( "%s", buffer + 1 );
}

#else
// I am not really sure how to deal with this
extern void XP_Trace( const char *format, ... )
{ }

#endif /* DEBUG */

#endif /* XP_MAC */

