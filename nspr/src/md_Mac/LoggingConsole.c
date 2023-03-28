/************************************************************************/
/*	Project...:	Standard ANSI-C Library									*/
/*	Name......:	console.c												*/
/*	Purpose...:	Stubs for console.c										*/
/*  Copyright.: ©Copyright 1994 by metrowerks inc. All rights reserved. */
/************************************************************************/

#ifndef __CONSOLE__
#include <console.h>
#endif


#include <Errors.h>
#include <Files.h>
#include <Script.h>
#include <TextUtils.h>
#include <Types.h>

#include "MacErrorHandling.h"


/*
 *	The following four functions provide the UI for the console package.
 *	Users wishing to replace SIOUX with their own console package need
 *	only provide the four functions below in a library.
 */


extern long	gInTimerCallback;

// We keep a copy of the file pointer around in a static.
short 	gLogFileRefNum = 0;
FSSpec	gLogFileFSSpec;

#define kLocalLogBufferSize (1024 * 16)
unsigned char gLocalLogBuffer[kLocalLogBufferSize];
unsigned char *gLocalLogBufferMark = gLocalLogBuffer;

/*
 *	extern short InstallConsole(short fd);
 *
 *	Installs the Console package, this function will be called right
 *	before any read or write to one of the standard streams.
 *
 *	short fd:		The stream which we are reading/writing to/from.
 *	returns short:	0 no error occurred, anything else error.
 */

short InstallConsole(short fd)
{
#ifdef DEBUG
	OSErr		err;
	char		*fNameFromEnv;
	StringPtr	fName;
	
	if (gLogFileRefNum != 0)
		return 0;					// It’s already been opened.
		
	// Create the FSSpec we’ll use.
	// 1st get the name of the file
	fNameFromEnv = PR_GetEnv("CONSOLE_LOG_NAME");
	
	if (fNameFromEnv == NULL)
		fNameFromEnv = "Console Log\0";
	
	fName = (StringPtr)strdup(fNameFromEnv);
	require ((fName != NULL), exitFail);
	
	// convert to Pascal style name
	fName = c2pstr((char *)fName);
	
	// Create the File Spec
	err = FSMakeFSSpec(0, 0, fName, &gLogFileFSSpec);
	require (((err == noErr) || (err == fnfErr)), exitFailRelease);		// OK if it’s not there.
		
	// Try to create the file
	err = FSpCreate(&gLogFileFSSpec, 'CWIE', 'TEXT', smSystemScript);
	require (((err == noErr) || (err == dupFNErr)), exitFailRelease);	// OK if it already exists
	
	// Now open it.
	err = FSpOpenDF(&gLogFileFSSpec, fsRdWrPerm, &gLogFileRefNum);
	require ((err == noErr), exitFailRelease);
		
	// Truncate the file
	err = SetEOF(gLogFileRefNum, 0);
	require ((err == noErr), exitFailRelease);
	
	return 0;
	
	
exitFailRelease:
	free (fName);

exitFail:
	return -1;
#else
	#pragma unused(fd)
	return -1;
#endif
}

/*
 *	extern void RemoveConsole(void);
 *
 *	Removes the console package.  It is called after all other streams
 *	are closed and exit functions (installed by either atexit or _atexit)
 *	have been called.  Since there is no way to recover from an error,
 *	this function doesn't need to return any.
 */

void RemoveConsole(void)
{
	OSErr	err;
	
	err = FSClose(gLogFileRefNum);
}

/*
 *	extern long WriteCharsToConsole(char *buffer, long n);
 *
 *	Writes a stream of output to the Console window.  This function is
 *	called by write.
 *
 *	char *buffer:	Pointer to the buffer to be written.
 *	long n:			The length of the buffer to be written.
 *	returns short:	Actual number of characters written to the stream,
 *					-1 if an error occurred.
 */

long WriteCharsToConsole(char *buffer, long n)
{
	int 	i;
	long	spaceInBuffer;
	long	bytesWritten = n;
	long	amtToMove = 0;
	OSErr	err = noErr;

	// Convert LF (0x0A) to CR (0x0D)
	for (i = 0; i < n; i++) {
		if (buffer[i] == '\n')
			buffer[i] = '\r';
	}
	
	if ((gInTimerCallback) || (gLogFileRefNum == 0)) {
		DebugStr("\pYikes, in timer callback");
		
		// Do we have room in the buffer?
		spaceInBuffer = (unsigned long)&gLocalLogBuffer + kLocalLogBufferSize - (unsigned long)gLocalLogBufferMark;
		
		amtToMove = (n > spaceInBuffer) ? spaceInBuffer : n; 
		
		BlockMoveData(buffer, gLocalLogBufferMark, amtToMove);

		gLocalLogBufferMark += amtToMove;
		
		return amtToMove;
	}
	else {
		// Is there anything in the buffer that needs to be written?
		if (gLocalLogBufferMark != gLocalLogBuffer) {
			amtToMove = (unsigned long)gLocalLogBufferMark - (unsigned long)&gLocalLogBuffer;
			err = FSWrite(gLogFileRefNum, &bytesWritten, gLocalLogBuffer);
		}
		if (err == noErr)
			err = FSWrite(gLogFileRefNum, &bytesWritten, buffer);
#if 0
		if (err == noErr)
			err = FlushVol(NULL, gLogFileFSSpec.vRefNum);
#endif	
		return ((err == noErr) ? bytesWritten : -1);
	}
}

/*
 *	extern long ReadCharsFromConsole(char *buffer, long n);
 *
 *	Reads from the Console into a buffer.  This function is called by
 *	read.
 *
 *	char *buffer:	Pointer to the buffer which will recieve the input.
 *	long n:			The maximum amount of characters to be read (size of
 *					buffer).
 *	returns short:	Actual number of characters read from the stream,
 *					-1 if an error occurred.
 */

long ReadCharsFromConsole(char *buffer, long n)
{
#pragma unused (buffer, n)

	return 0;
}

/*
 *	extern char *__ttyname(long fildes);
 *
 *	Return the name of the current terminal (only valid terminals are
 *	the standard stream (ie stdin, stdout, stderr).
 *
 *	long fildes:	The stream to query.
 *
 *	returns char*:	A pointer to static global data which contains a C string
 *					or NULL if the stream is not valid.
 */

extern char *__ttyname(long fildes)
{
#pragma unused (fildes)
	/* all streams have the same name */
	static char *__devicename = "null device";

	if (fildes >= 0 && fildes <= 2)
		return (__devicename);

	return (0L);
}

