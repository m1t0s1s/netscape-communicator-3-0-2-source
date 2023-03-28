/* -*- Mode:C: tab-width: 4 -*- */
#ifndef _XP_Assert_
#define _XP_Assert_

#include "xp_trace.h"
/*include <stdlib.h>*/

/*-----------------------------------------------------------------------------
	abort
	
	For debug builds...
	XP_ABORT(X), unlike abort(), takes a text string argument. It will print
	it out and then call abort (to drop you into your debugger).
	
	For release builds...
	XP_ABORT will call abort(). Whether you #define NDEBUG or not is up
	to you.
-----------------------------------------------------------------------------*/
#define XP_ABORT(MESSAGE)	(XP_TRACE(MESSAGE),abort())

/*-----------------------------------------------------------------------------
	XP_ASSERT is just like "assert" but calls XP_ABORT if it fails the test.
	I need this (on a Mac) because "assert" and "abort" are braindead,
	whereas my XP_Abort function will invoke the debugger. It could
	possibly have been easier to just #define assert to be something decent.
-----------------------------------------------------------------------------*/

#if defined (XP_UNIX)
#include <assert.h>
#define XP_ASSERT(X)			assert(X) /* are we having fun yet? */

#elif defined (XP_WIN)
#ifdef DEBUG

/* LTNOTE: I got tired of seeing all asserts at FEGUI.CPP.  This should
 * Fix the problem.  I intentionally left out Win16 because strings are stuffed
 * into the datasegment we probably couldn't build.
*/
#ifdef WIN32
XP_BEGIN_PROTOS
void XP_AssertAtLine( char *pFileName, int iLine );
XP_END_PROTOS
#define XP_ASSERT(X)   ( ((X)!=0)? (void)0: XP_AssertAtLine(__FILE__,__LINE__))

#else  /* win16 */
#define XP_ASSERT(X)                ( ((X)!=0)? (void)0: XP_Assert((X) != 0)  )
XP_BEGIN_PROTOS
void XP_Assert(int);
XP_END_PROTOS
#endif

#else
#define XP_ASSERT(X)
#endif 

#elif defined (XP_MAC)

#ifdef DEBUG
#ifdef __cplusplus
extern "C" void MacFE_Signal(const char* s);
#else
void MacFE_Signal(const char* s);
#endif

#define XP_ASSERT(X)			 if (!(X)) MacFE_Signal(#X)
#else
#define XP_ASSERT(X)
#endif

#endif

/*-----------------------------------------------------------------------------
	assert variants
	
	XP_WARN_ASSERT if defined to nothing for release builds. This means
	that instead of
				#ifdef DEBUG
				assert (X);
				#endif
	you can just do
				XP_WARN_ASSERT(X);
	
	Of course when asserts fail that means something is going wrong and you
	*should* have normal code to deal with that.
	I frequently found myself writing code like this:
				#ifdef DEBUG
				assert (aPtr);
				#endif
				if (!aPtr)
					return error_something_has_gone_wrong;
	so I just combined them into a macro that can be used like this:
				if (XP_FAIL_ASSERT(aPtr))
					return; // or whatever else you do when things go wrong
	What this means is it will return if X *fails* the test. Essentially
	the XP_FAIL_ASSERT bit replaces the "!" in the if test.
	
	XP_OK_ASSERT is the opposite. If if you want to do something only if
	something is OK, then use it. For example:
				if (XP_OK_ASSERT(aPtr))
					aPtr->aField = 25;
	Use this if you are 99% sure that aPtr will be valid. If it ever is not,
	you'll drop into the debugger. For release builds, it turns into an
	if statement, so it's completely safe to execute.
-----------------------------------------------------------------------------*/
#ifdef DEBUG
#	define XP_WARN_ASSERT(X)	(  ((X)!=0)? (void)0: XP_ABORT((#X))  )
#	define XP_OK_ASSERT(X)		(((X)!=0)? 1: (XP_ABORT((#X)),0))
#	define XP_FAIL_ASSERT(X)	(((X)!=0)? 0: (XP_ABORT((#X)),1))
#else
#	define XP_WARN_ASSERT(X)	(void)((X)!=0)
#	define XP_OK_ASSERT(X)		(((X)!=0)? 1: 0)
#	define XP_FAIL_ASSERT(X)	(((X)!=0)? 0: 1)
#endif

#endif /* _XP_Assert_ */

