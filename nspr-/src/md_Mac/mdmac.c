#include <Types.h>
#include <Timer.h>
#include <Files.h>
#include <Errors.h>
#include <Folders.h>
#include <Events.h>
#include <Processes.h>
#include <TextUtils.h>
#include <MixedMode.h>

#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <stat.h>
#include <stdarg.h>
#include <unix.h>

#include "prthread.h"
#include "mdint.h"
#include "swkern.h"
#include "mdmacmem.h"
#include "prfile.h"
#include "prlog.h"
#include "prprf.h"
#include "MacErrorHandling.h"
#include "prmacos.h"
#include "fastmem.h"
#include "macfeprefs.h"

#include <LowMem.h>

enum {
	kDefalutMacintoshTimeSlice			= 25,
	kTimeSlicesPerWNELoop				= 5,
	kGarbageCollectionEmergencyFundSize = 16 * 1024,
	kGarbageCollectionMemTightFundSize	= 48 * 1024
};

enum {
	// Why aren’t these defined in Files.h?
	kFileLockedBit						= 0x01,
	kRsrcForkOpenBit					= 0x04,
	kDataForkOpenBit					= 0x08,
	kIsDirectoryBit						= 0x10,
	kEitherForkOpenBit					= 0x80
};

enum {
	uppExitToShellProcInfo 				= kPascalStackBased,
	uppStackSpaceProcInfo				= kRegisterBased 
										  | RESULT_SIZE(SIZE_CODE(sizeof(long)))
										  | REGISTER_RESULT_LOCATION(kRegisterD0)
		 								  | REGISTER_ROUTINE_PARAMETER(1, kRegisterD1, SIZE_CODE(sizeof(UInt16)))
};


#define UNIMPLEMENTED_ROUTINE			\
	debugstr("Not Implemented Yet");	\
	return 0;

// This should be in LowMem.h
#define LMSetStackLowPoint(value)		\
	*((UInt32 *)(0x110)) = (value)

static long 	gInTimerCriticalActive = 0;

//
// Local routines
//
pascal void TimerCallback(TMTaskPtr);
void ActivateTimer(void);
void DeactivateTimer(void);
OSErr GetFullPath(short , long , char **, int *);
void MacintoshInitializeTime(void);
void TruncateFileName(char *macPath);
OSErr ConvertUnixPathToMacPath(const char *, char **);
OSErr ConvertMacPathToUnixPath(const char *macPath, char **unixPath);
OSErr CreateMacPathFromUnixPath(const char *, char **);
void PStrFromCStr(const char *, Str255);
void CStrFromPStr(ConstStr255Param, char **);
int strcmpcore(const char *, const char *, int);
OSErr GetFullPath(short vRefNum, long dirID, char **fullPath, int *strSize);
void _MD_GetRegisters(prword_t *to);
pascal void BumpMainThreadPriorityForEvent(EventKind eventNum);
short GetVolumeRefNumFromName(const char *);

long gClockTimerExpired = false;

PRThread *gPrimaryThread = NULL;

pascal void ExitToShellPatch(void);

UniversalProcPtr	gExitToShellPatchCallThru = NULL;
#if GENERATINGCFM
RoutineDescriptor 	gExitToShellPatchRD = BUILD_ROUTINE_DESCRIPTOR(uppExitToShellProcInfo, &ExitToShellPatch);
#else
#define gExitToShellPatchRD ExitToShellPatch
#endif

UniversalProcPtr	gStackSpacePatchCallThru = NULL;
#if GENERATINGCFM
pascal long StackSpacePatch(UInt16);
RoutineDescriptor 	StackSpacePatchRD = BUILD_ROUTINE_DESCRIPTOR(uppStackSpaceProcInfo, &StackSpacePatch);
#else
pascal long StackSpacePatch();
asm pascal long StackSpacePatchGlue();
#endif



//##############################################################################
//##############################################################################
#pragma mark -
#pragma mark CREATING MACINTOSH THREAD STACKS

#if defined(powerc) || defined(__powerc)
#define kDefaultMacintoshStackSize 	58 * 1024	
#define kIdleThreadStackSize		8 * 1024
#define kFinalizerStackSize 		16 * 1024
#else
#define kDefaultMacintoshStackSize 	58 * 1024
#define kIdleThreadStackSize		8 * 1024
#define kFinalizerStackSize 		16 * 1024
#endif

int _MD_NewStack(PRThread *thread, size_t stackSize)
{
	PRThreadStack*	stackInfo;
	
	if (!strcmp(thread->name, "finalizer")) {
		stackSize = kFinalizerStackSize;
	}
	
	else if (!strcmp(thread->name, "idle")) {
		stackSize = kIdleThreadStackSize;
	}

	if (stackSize == 0)
		stackSize = kDefaultMacintoshStackSize;

	stackInfo = (PRThreadStack*) malloc(sizeof(PRThreadStack));
	if (stackInfo == NULL) {
#if DEBUG
		DebugStr("\p_MD_NewStack failed.");
#endif
		return 0;
	}

	//	Take the actual memory for the stack out of our
	//	Figment heap.

	stackInfo->allocSize = stackSize;
	stackInfo->allocBase = (char *)NewPtr(stackSize);

	if (stackInfo->allocBase == NULL) {
#if DEBUG
		DebugStr("\p_MD_NewStack failed.");
#endif
		free(stackInfo);
		return 0;
	}

	stackInfo->stackBottom = stackInfo->allocBase;
	stackInfo->stackTop = stackInfo->stackBottom + stackSize;
	stackInfo->stackSize = stackSize;
	
#if DEBUG
	//	Put a cookie at the top of the stack so that we can find 
	//	it from Macsbug.
	
	memset(stackInfo->allocBase, 0xDC, stackSize);
	
	((UInt32 *)stackInfo->stackTop)[-1] = 0xBEEFCAFE;
	((UInt32 *)stackInfo->stackTop)[-3] = (UInt32)(thread);
	((UInt32 *)stackInfo->stackBottom)[0] = 0xCAFEBEEF;
#endif
	
	thread->stack = stackInfo;
	
	// Turn off the snack stiffer.  The NSPR stacks are allocated in the 
	// application's heap; this throws the stack sniffer for a tizzy.
	// Note that the sniffer does not run on machines running the thread manager.
	// Yes, we will blast the low-mem every time a new stack is created.  We can afford
	// a couple extra cycles.
	LMSetStackLowPoint(0);
	
	return 1;
}

void _MD_FreeStack(PRThread *thread)
{
#if DEBUG
	//	Clear out our cookies. 
	
	memset(thread->stack->allocBase, 0xEF, thread->stack->stackSize);
	
	((UInt32 *)thread->stack->stackTop)[-1] = 0;
	((UInt32 *)thread->stack->stackTop)[-3] = 0;
	((UInt32 *)thread->stack->stackBottom)[0] = 0;
#endif

	if (thread->stack != NULL) {
		if (thread->stack->allocBase != NULL)
			DisposePtr((Ptr)(thread->stack->allocBase));
		free(thread->stack);	
	}
}

#if !GENERATINGCFM

asm long pascal StackSpacePatchCallThruGlue(long theAddress)
{
	move.l	4(sp), a0
	jsr		(a0)
	move.l	(sp)+, a0
	add		#0x8, sp 
	move.l	d0, -(sp)
	jmp		(a0)
}

asm pascal long StackSpacePatchGlue()
{

	//	Check out LocalA5.  If it is zero, then
	//	it is our first time through, and we should
	//	store away our A5.  If not, then we are
	//	a real time manager callback, so we should
	//	store away A5, set up our local a5, jsr
	//	to our callback, and then restore the
	//	previous A5.
	
	lea			LocalA5, a0
	move.l		(a0), d0
	cmpi.l		#0, d0
	bne			DoStackSpace
	
	move.l		a5, (a0)
	rts

DoStackSpace:
	
	//	Save A5, restore our local A5
	
	move.l		a5, -(sp)
	move.l		d0, a5 
	
	//	Jump to our C routine
	
	clr.l		-(sp)
	jsr 		StackSpacePatch
	move.l		(sp)+, d0
	
	//	Restore the previous A5
	
	move.l		(sp)+, a5

	rts

LocalA5:

	dc.l		0
	
}

#endif

#if GENERATINGCFM
pascal long StackSpacePatch(UInt16 trapNo)
#else
pascal long StackSpacePatch()
#endif
{
	char		tos;
	PRThread	*thisThread;
	
	thisThread = PR_CurrentThread();
	
	//	If we are the primary thread, then call through to the
	//	good ol' fashion stack space implementation.  Otherwise,
	//	compute it by hand.
	if ((thisThread == gPrimaryThread) || 	
		(&tos < thisThread->stack->stackBottom) || 
		(&tos > thisThread->stack->stackTop)) {
#if GENERATINGCFM
		return CallOSTrapUniversalProc(gStackSpacePatchCallThru, uppStackSpaceProcInfo, trapNo);
#else
		return StackSpacePatchCallThruGlue((long)gStackSpacePatchCallThru);
#endif
	}
	else {
		return &tos - thisThread->stack->stackBottom;
	}
}


//##############################################################################
//##############################################################################
#pragma mark -
#pragma mark TIME MANAGER-BASED CLOCK

TMTask		gTimeManagerTaskElem;
Boolean		gTimerHasFired = false;
Boolean 	gShouldReprime = false;
long		gInTimerCallback = false;
long		gTimeSlicesOnNonPrimaryThread = 0;

pascal void TimerCallback(TMTaskPtr tmTaskPtr)
{
	gInTimerCallback = true;
	
	if (gInTimerCriticalActive == 0) {

	#pragma unused (tmTaskPtr)
		gClockTimerExpired = true;
		
		//	And tell nspr that a clock interrupt occured.
		
		_PR_ClockInterruptHandler();
		
		//	This is a good opportunity to make sure that the main
		//	mozilla thread actually gets some time.  If interrupts
		//	are on, then we know it is safe to check if the main
		//	thread is being starved.  If moz has not been scheduled
		//	for a long time, then then temporarily bump the fe priority 
		//	up so that it gets to run at least one. 
		
		gTimeSlicesOnNonPrimaryThread++;

		if (!_pr_intsOff) {

			_pr_intsOff = 1;
	 
			if (gTimeSlicesOnNonPrimaryThread >= 20) {
				_PR_SetThreadPriority(gPrimaryThread, 31);
				gTimeSlicesOnNonPrimaryThread = 0;
			}
			
			if (gPrimaryThread == PR_CurrentThread())
				gTimeSlicesOnNonPrimaryThread = 0;
		
			_pr_intsOff = 0;

		}

		//	Reset the clock timer so that we fire again.
		
		ResetTimer();

	}
	
	gInTimerCallback = false;
}

#if GENERATINGCFM

RoutineDescriptor 	gTimerCallbackRD = BUILD_ROUTINE_DESCRIPTOR(uppTimerProcInfo, &TimerCallback);

#else

asm void gTimerCallbackRD(void)
{
	//	Check out LocalA5.  If it is zero, then
	//	it is our first time through, and we should
	//	store away our A5.  If not, then we are
	//	a real time manager callback, so we should
	//	store away A5, set up our local a5, jsr
	//	to our callback, and then restore the
	//	previous A5.
	
	lea			LocalA5, a0
	move.l		(a0), d0
	cmpi.l		#0, d0
	bne			TimerProc
	
	move.l		a5, (a0)
	rts

TimerProc:
	
	//	Save A5, restore our local A5
	
	move.l		a5, -(sp)
	move.l		d0, a5 
	
	//	Jump to our C routine
	
	move.l 		a1, -(sp)
	jsr 		TimerCallback
	
	//	Restore the previous A5
	
	move.l		(sp)+, a5

	rts

LocalA5:

	dc.l		0
	
}

#endif

void ActivateTimer(void)
{
	gInTimerCriticalActive++;

	//	NOTE:  	Software interrupts must be off at this time
	//			for any of this cruft to work!

	gTimeManagerTaskElem.qLink = 0;

	//	Make sure that our time manager task is ready to go.
	InsTime((QElemPtr)&gTimeManagerTaskElem);
	
	PrimeTime((QElemPtr)&gTimeManagerTaskElem, 5); // Default clock tick is 5 ms

	gInTimerCriticalActive--;

}

void DeactivateTimer(void)
{
	gInTimerCriticalActive++;
	
	RmvTime((QElemPtr)&gTimeManagerTaskElem);
	gTimeManagerTaskElem.qLink = 0;
	
	gInTimerCriticalActive--;
}

void ResetTimer(void)
{
	gInTimerCriticalActive++;
	DeactivateTimer();
	ActivateTimer();
	gInTimerCriticalActive--;
}


void _MD_StartClock(void)
{

	//	If we are not generating CFM-happy code, then make sure that 
	//	we call our callback wrapper once so that we can
	//	save away our A5.

#if !GENERATINGCFM
	gTimerCallbackRD();
#endif

	//	Fill in the Time Manager queue element
	
	gTimeManagerTaskElem.tmAddr = (TimerUPP)&gTimerCallbackRD;
	gTimeManagerTaskElem.tmWakeUp = 0;
	gTimeManagerTaskElem.tmReserved = 0;

	TimerCallback(NULL);
}


void _PR_Pause(void)
{	
	PR_Yield();
}


//##############################################################################
//##############################################################################
#pragma mark -
#pragma mark TIME

UnsignedWide			dstLocalBaseMicroseconds;
unsigned long			gJanuaryFirst1970Seconds;

void MacintoshInitializeTime(void)
{
	UnsignedWide			upTime;
	unsigned long			currentLocalTimeSeconds,
							startupTimeSeconds;
	uint64					startupTimeMicroSeconds;
	uint32					upTimeSeconds;	
	uint64					oneMillion, upTimeSecondsLong, microSecondsToSeconds;
	DateTimeRec				firstSecondOfUnixTime;
	
	//	Figure out in local time what time the machine 
	//	started up.  This information can be added to 
	//	upTime to figure out the current local time 
	//	as well as GMT.

	Microseconds(&upTime);
	
	GetDateTime(&currentLocalTimeSeconds);
	
	LL_I2L(microSecondsToSeconds, PR_USEC_PER_SEC);
	LL_DIV(upTimeSecondsLong,  *((uint64 *)&upTime), microSecondsToSeconds);
	LL_L2I(upTimeSeconds, upTimeSecondsLong);
	
	startupTimeSeconds = currentLocalTimeSeconds - upTimeSeconds;
	
	//	Make sure that we normalize the macintosh base seconds
	//	to the unix base of January 1, 1970.
	
	firstSecondOfUnixTime.year = 1970;
	firstSecondOfUnixTime.month = 1;
	firstSecondOfUnixTime.day = 1;
	firstSecondOfUnixTime.hour = 0;
	firstSecondOfUnixTime.minute = 0;
	firstSecondOfUnixTime.second = 0;
	firstSecondOfUnixTime.dayOfWeek = 0;
	
	DateToSeconds(&firstSecondOfUnixTime, &gJanuaryFirst1970Seconds);
	
	startupTimeSeconds -= gJanuaryFirst1970Seconds;
	
	//	Now convert the startup time into a wide so that we
	//	can figure out GMT and DST.
	
	LL_I2L(startupTimeMicroSeconds, startupTimeSeconds);
	LL_I2L(oneMillion, PR_USEC_PER_SEC);
	LL_MUL(dstLocalBaseMicroseconds, oneMillion, startupTimeMicroSeconds);
}



//##############################################################################
//##############################################################################
#pragma mark -
#pragma mark STDCLIB

void abort(void)
{
	exit(-1);
}

void *memcpy(void *to, const void *from, size_t size)
{
	if (size != 0) {
#if DEBUG
		if ((UInt32)to < 0x1000)
			DebugStr("\pmemcpy has illegal to argument");
		if ((UInt32)from < 0x1000)
			DebugStr("\pmemcpy has illegal from argument");
#endif
		BlockMoveData(from, to, size);
	}
	return to;
}


//##############################################################################
//##############################################################################
#pragma mark -
#pragma mark MISCELLANEOUS

void debugstr(const char *debuggerMsg)
{
	Str255		pStr;
	
	PStrFromCStr(debuggerMsg, pStr);
	DebugStr(pStr);
}


char *strdup(const char *source)
{
	char 	*newAllocation;
	size_t	stringLength;

#ifdef DEBUG
	PR_ASSERT(source);
#endif
	
	stringLength = strlen(source) + 1;
	
	newAllocation = (char *)malloc(stringLength);
	if (newAllocation == NULL)
		return NULL;
	BlockMoveData(source, newAllocation, stringLength);
	return newAllocation;
}

prword_t *_MD_HomeGCRegisters(PRThread *t, int isCurrent, int *np)
{
    if (isCurrent) {
	(void) setjmp(t->context.registers);
    }
    *np = sizeof(t->context) / sizeof(prword_t);
    return (prword_t*) (t->context.registers);
}

void _MD_GetRegisters(prword_t *to)
{
  (void) setjmp((void*) to);
}

typedef struct GCBlockHeader GCBlockHeader;

enum {
	kGCBlockNormalType				= 1,
	kGCBlockTempMemoryType,
	kGCBlockEmergencyFundType
};

struct GCBlockHeader {
	UInt32		blockType;
	Handle		blockHandle;
};

void *_MD_GrowGCHeap(size_t *sizep)
{
	static void		*gEmergencyFund = NULL;
	void			*heapPtr = NULL;
	size_t			sizeToAllocate = *sizep;
	Handle			newTempMemoryHandle;
	OSErr			resultCode;
	
	if (gEmergencyFund == NULL) {
		gEmergencyFund = NewPtr(kGarbageCollectionEmergencyFundSize + sizeof(GCBlockHeader));
		if (gEmergencyFund != NULL)	{
			((GCBlockHeader *)gEmergencyFund)->blockType = kGCBlockEmergencyFundType;
		}
	}
	
	//	I know this is evil, but try out of the temp 
	//	memory heap before the app.  This helps us run 
	//	better on systems with more memory.  It also
	//	makes us look less greedy.
	
	newTempMemoryHandle = TempNewHandle(sizeToAllocate + sizeof(GCBlockHeader), &resultCode);
	if (newTempMemoryHandle != NULL) {
		GCBlockHeader	*blockHeader = (GCBlockHeader *)(*newTempMemoryHandle);
		blockHeader->blockType = kGCBlockTempMemoryType;
		blockHeader->blockHandle = newTempMemoryHandle;
		MoveHHi(newTempMemoryHandle);
		HLock(newTempMemoryHandle);
		heapPtr = (void *)*newTempMemoryHandle;
	}
	
	else {	
		heapPtr = NewPtr(sizeToAllocate);
	}
	
	//	If we can't get a full segment, try a smaller one
	
	if (heapPtr == NULL) {
		heapPtr = NewPtr(kGarbageCollectionMemTightFundSize + sizeof(GCBlockHeader));
		if (heapPtr != NULL) {
			((GCBlockHeader *)heapPtr)->blockType = kGCBlockNormalType;
		}
		*sizep = kGarbageCollectionMemTightFundSize;
	}
	
	//	If even this fails, then use our emergency 
	//	(preallocated) fund.
	
	if (heapPtr == NULL) {
		heapPtr = gEmergencyFund;
		if (heapPtr == NULL)
			*sizep = 0;
		else	
			*sizep = kGarbageCollectionEmergencyFundSize;
	}
	
	if (heapPtr == NULL) {
		return NULL;
	} else
		return (void *)((char *)heapPtr + sizeof(sizeof(GCBlockHeader))); 

}

void _MD_FreeGCSegment(void *base, int32 len)
{
	GCBlockHeader	*blockHeader = (GCBlockHeader *)((char *)base - sizeof(GCBlockHeader));
	
	switch (blockHeader->blockType) {
	
		case kGCBlockNormalType:
			DisposePtr((Ptr)blockHeader);
			break;
		
		case kGCBlockTempMemoryType:
			DisposeHandle(blockHeader->blockHandle);
			break;

		case kGCBlockEmergencyFundType:
			DisposePtr((Ptr)blockHeader);
			break;
	
	}

}


void _MD_InitOS(int when)
{
	gPrimaryThread = PR_CurrentThread();

	if (when == _MD_INIT_AT_START) {
	
		Handle		environmentVariables;

		MacintoshInitializeMemory();
		MacintoshInitializeTime();
		
		//	Install resource-controlled environment variables.
		
		environmentVariables = GetResource('Envi', 128);
		if (environmentVariables != NULL) {
		
			Size 	resourceSize;
			char	*currentPutEnvString = (char *)*environmentVariables,
					*currentScanChar = currentPutEnvString;
					
			resourceSize = GetHandleSize(environmentVariables);			
			DetachResource(environmentVariables);
			HLock(environmentVariables);
			
			while (resourceSize--) {
			
				if ((*currentScanChar == '\n') || (*currentScanChar == '\r')) {
					*currentScanChar = 0;
					putenv(currentPutEnvString);
					currentPutEnvString = currentScanChar + 1;
				}
			
				currentScanChar++;
			
			}
			
			DisposeHandle(environmentVariables);

		}

#if GENERATINGCFM
		gStackSpacePatchCallThru = GetOSTrapAddress(0x0065);
		SetOSTrapAddress((UniversalProcPtr)&StackSpacePatchRD, 0x0065);
		{
			long foo;
			foo = StackSpace();
		}
#else
		gStackSpacePatchCallThru = GetOSTrapAddress(0x0065);
		SetOSTrapAddress((UniversalProcPtr)&StackSpacePatchGlue, 0x0065);
		StackSpace();
#endif

		//	THIS IS VERY IMPORTANT.  Install our ExitToShell patch.  
		//	This allows us to deactivate our Time Mananger task even
		//	if we are not totally gracefully exited.  If this is not
		//	done then we will randomly crash at later times when the
		//	task is called after the app heap is gone.
		
		gExitToShellPatchCallThru = GetToolboxTrapAddress(0x01F4);
		SetToolboxTrapAddress((UniversalProcPtr)&gExitToShellPatchRD, 0x01F4);

	}
	
}

PRThread *PR_GetPrimaryThread()
{
	return gPrimaryThread;
}

void _PR_InitIO(void)
{

}

void PR_InitMemory(void) {
	//	Needed for Mac browsers without Java.  We don’t want them calling PR_INIT, since it
	//	brings in all of the thread support.  But we do need to allow them to initialize
	//	the NSPR memory package.
	//	This should go away when all clients of the NSPR want threads AND memory.
	MacintoshInitializeMemory();
}

//##############################################################################
//##############################################################################
#pragma mark -
#pragma mark TERMINATION

typedef pascal void (* ExitToShellProc)(void);  

//	THIS IS *** VERY *** IMPORTANT... our ExitToShell patch.
//	This allows us to deactivate our Time Mananger task even
//	if we are not totally gracefully exited.  If this is not
//	done then we will randomly crash at later times when the
//	task is called after the app heap is gone.

pascal void ExitToShellPatch(void)
{
	gInTimerCriticalActive++;
	DeactivateTimer();
	
#if DEBUG_MAC_MEMORY || STATS_MAC_MEMORY
	DumpAllocHeap();
#endif
	
#if GENERATINGCFM
	CallUniversalProc(gExitToShellPatchCallThru, uppExitToShellProcInfo);
#else 
	{
		ExitToShellProc	*exitProc = (ExitToShellProc *)&gExitToShellPatchCallThru;
		(*exitProc)();
	}
#endif

}


//##############################################################################
//##############################################################################
#pragma mark -
#pragma mark STDC STYLE IO


OSErr GetFullPath(short vRefNum, long dirID, char **fullPath, int *strSize)
{
	Str255			pascalDirName;
	char			cDirName[256];
	char			*tmpPath = NULL;						// needed since sprintf isn’t safe
	CInfoPBRec		myPB;
	OSErr			err = noErr;
	
	
	// get the full path of the temp folder.
	*strSize = 256;
	*fullPath = NULL;
	*fullPath = malloc(*strSize);	// How big should this thing be?
	require_action (*fullPath != NULL, errorExit, err = memFullErr;);
		
	tmpPath = malloc(*strSize);
	require_action (tmpPath != NULL, errorExit, err = memFullErr;);

	strcpy(*fullPath, "");				// Clear C result
	strcpy(tmpPath, "");
	pascalDirName[0] = 0;				// Clear Pascal intermediate string
	
	myPB.dirInfo.ioNamePtr = &pascalDirName[0];
	myPB.dirInfo.ioVRefNum = vRefNum;
	myPB.dirInfo.ioDrParID = dirID;
	myPB.dirInfo.ioFDirIndex = -1;				// Getting info about
	
	do {
		myPB.dirInfo.ioDrDirID = myPB.dirInfo.ioDrParID;

		err = PBGetCatInfoSync(&myPB);
		require(err == noErr, errorExit);
			
		// Move the name into C domain
		memcpy(&cDirName, &pascalDirName, 256);
		p2cstr((unsigned char *)&cDirName);							// Changes in place!
		
		if ((strlen(cDirName) + strlen(*fullPath)) > *strSize) {
			// We need to grow the string, do it in 256 byte chunks
			(*strSize) += 256;										
			*fullPath = realloc(*fullPath, *strSize);
			require_action (*fullPath != NULL, errorExit, err = memFullErr;);

			tmpPath = realloc(tmpPath, *strSize);
			require_action (tmpPath != NULL, errorExit, err = memFullErr;);
		}
		sprintf(tmpPath, "%s:%s", cDirName, *fullPath);
		strcpy(*fullPath, tmpPath);
	} while (myPB.dirInfo.ioDrDirID != fsRtDirID);
	
	free(tmpPath);
	
	return noErr;
	
	
errorExit:
	free(*fullPath);
	free(tmpPath);
	
	return err;

}



OSErr CreateMacPathFromUnixPath(const char *unixPath, char **macPath)
{
	// Given a Unix style path with '/' directory separators, this allocates 
	// a path with Mac style directory separators in the path.
	//
	// It does not do any special directory translation; use ConvertUnixPathToMacPath
	// for that.
	
	char		*src;
	char		*tgt;
	OSErr		err = noErr;

	*macPath = malloc(strlen(unixPath) * 2);	// Will be enough extra space.
	require_action (*macPath != NULL, exit, err = memFullErr;);

	strcpy(*macPath, "");						// Clear the Mac path
	
	(const char *)src = unixPath;
	tgt = *macPath;
	
	if (strchr(src, DIRECTORY_SEPARATOR) == src)				// If we’re dealing with an absolute
		src++;													// path, skip the separator
	else
		*(tgt++) = MAC_PATH_SEPARATOR;	
		
	if (strstr(src, UNIX_THIS_DIRECTORY_STR) == src)			// If it starts with /
		src += 2;												// skip it.
		
	while (*src) 
	{				// deal with the rest of the path
		if (strstr(src, UNIX_PARENT_DIRECTORY_STR) == src) {	// Going up?
			*(tgt++) = MAC_PATH_SEPARATOR;						// simply add an extra colon.
			src +=3;
		}
		else if (*src == DIRECTORY_SEPARATOR) {					// Change the separator
			*(tgt++) = MAC_PATH_SEPARATOR;
			src++;
		}
		else
			*(tgt++) = *(src++);
	}
	
	*tgt = NULL;							// make sure it’s null terminated.

exit:
	return err;
}

OSErr ConvertUnixPathToMacPath(const char *unixPath, char **macPath)
{	
		OSErr		err = noErr;
		

	//	******** HACK ALERT ********
	//
	//	Java really wants long file names (>31 chars).  We truncate file names 
	//	greater than 31 characters long.  Truncation is from the middle.
	//
	//	Convert UNIX style path names (with . and / separators) into a Macintosh
	//	style path (with :).
	//
	// There are also a couple of special paths that need to be dealt with
	// by translating them to the appropriate Mac special folders.  These include:
	//
	//			/usr/tmp/file  =>  {TempFolder}file
	//
	// The file conversions we need to do are as follows:
	//
	//			file			=>		file
	//			dir/file		=>		:dir:file
	//			./file			=>		file
	//			../file			=>		::file
	//			../dir/file		=>		::dir:file
	//			/file			=>		::BootDrive:file
	//			/dir/file		=>		::BootDrive:dir:file
	
	
	
	if (*unixPath != DIRECTORY_SEPARATOR) {				// Not root relative, just convert it.
		err = CreateMacPathFromUnixPath(unixPath, macPath);
	}
	
	else {
		// We’re root-relative.  This is either a special Unix directory, or a 
		// full path (which we’ll support on the Mac since they might be generated).
		// This is not condoning the use of full-paths on the Macintosh for file 
		// specification.
		
		short		foundVRefNum;
		long		foundDirID;
		int			pathBufferSize;
		char		*temp;
		char		isNetscapeDir = false;

		// Are we dealing with the temp folder?
		if ((strncmp(unixPath, "/usr/tmp", strlen("/usr/tmp")) == 0) || 
			((strncmp(unixPath, "/tmp", strlen("/tmp")) == 0))) {
			unixPath = strrchr(unixPath, DIRECTORY_SEPARATOR);						// Find last back-slash
			unixPath++;																// Skip the slash
			err = FindFolder(kOnSystemDisk, kTemporaryFolderType, kCreateFolder,	// Create if needed
								&foundVRefNum, &foundDirID);
		}
		
		else if (strncmp(unixPath, "/bin", strlen("/bin")) == 0) {
			dprintf("Unable to translate Unix file path %s to Mac path\n", unixPath);
			err = -1;
			goto Exit_ConvertUnixPathToMacPath;
		}
		
		else if (strncmp(unixPath, "/dev", strlen("/dev")) == 0) {
			dprintf("Unable to translate Unix file path %s to Mac path\n", unixPath);
			err = -1;
			goto Exit_ConvertUnixPathToMacPath;
		}
	
		else if (strncmp(unixPath, "/etc", strlen("/etc")) == 0) {
			dprintf("Unable to translate Unix file path %s to Mac path\n", unixPath);
			err = -1;
			goto Exit_ConvertUnixPathToMacPath;
		}
	
		else if (strncmp(unixPath, "/usr", strlen("/usr")) == 0) {
		
			int		usrNetscapePathLen;
			
			usrNetscapePathLen = strlen("/usr/local/netscape/");
		
			if (strncmp(unixPath, "/usr/local/netscape/", usrNetscapePathLen) == 0) {
				unixPath += usrNetscapePathLen;				
				err = FindPreferencesFolder(&foundVRefNum, &foundDirID);
				isNetscapeDir = true;
			}
			
			else {
				dprintf("Unable to translate Unix file path %s to Mac path\n", unixPath);
				err = -1;
				goto Exit_ConvertUnixPathToMacPath;
			}

		}
		
		else {
			// This is a root relative directory, we’ll just convert the whole thing.
			err = CreateMacPathFromUnixPath(unixPath, macPath);
			goto Exit_ConvertUnixPathToMacPath;
		}
	
		// We’re dealing with a special folder
		if (err == noErr)
			// Get the path to the root-relative directory
			err = GetFullPath(foundVRefNum, foundDirID, macPath, &pathBufferSize);		// mallocs macPath
		
		// Insert bad O.J. joke here
		if (err == noErr){
			
			// copy over the remaining file name, converting
			if (pathBufferSize < (strlen(*macPath) + strlen(unixPath))) {
				// need to grow string
				*macPath = realloc(*macPath, (strlen(*macPath) + strlen(unixPath) + 
					(isNetscapeDir ? strlen("Netscape ƒ:") : 0)));
				err = (*macPath == NULL ? memFullErr : noErr);
			}
			
			if (isNetscapeDir)
				strcat(*macPath, "Netscape ƒ:");
		
			if (err == noErr)
				strcat(*macPath, unixPath);
			
			//	Make sure that all of the /’s are :’s in the final pathname
				
			for (temp = *macPath + strlen(*macPath) - strlen(unixPath); *temp != '\0'; temp++) {
				if (*temp == DIRECTORY_SEPARATOR)
					*temp = MAC_PATH_SEPARATOR;
			}

		}
	}
	
	
Exit_ConvertUnixPathToMacPath:

	if (err == noErr) {
		// Fix file names which are too long
		// *** HACK TO BE REMOVED BEFORE SHIPPING ***
		char	*fileName;
		
		fileName = strrchr(*macPath, MAC_PATH_SEPARATOR);		// Find last colon
		if (fileName == NULL)									// No colon, 
			fileName = *macPath;								// so use file name
		else													// otherwise
			fileName++;											// skip the colon.
		MapFullToPartialMacFile(fileName);
	}
	return err;
}


// Hey! Before you delete this "hack" you should look at how it's being
// used by sun-java/netscape/applet/appletStubs.c.
OSErr ConvertMacPathToUnixPath(const char *macPath, char **unixPath) 
{
	// *** HACK ***
	// Get minimal version working
	
	char		*unixPathPtr;
	
	*unixPath = malloc(strlen(macPath) + 2);	// Add one for the front slash, one for null
	if (*unixPath == NULL)
		return (memFullErr);
		
	unixPathPtr = *unixPath;
	
	*unixPathPtr++ = DIRECTORY_SEPARATOR;
	
	do {
		// Translate all colons to slashes
		if (*macPath == MAC_PATH_SEPARATOR)
			*unixPathPtr = DIRECTORY_SEPARATOR;
		else
			*unixPathPtr = *macPath;

		unixPathPtr++;
		macPath++;
	} while (*macPath != NULL);
	
	// Terminate the string
	*unixPathPtr = '\0';
	
	return (noErr);
}

OSErr ConvertUnixPathToFSSpec(const char *unixPath, FSSpec *fileSpec)
{
	char			*macPath,
					*macPathCopy,
					*scanChar;
	CInfoPBRec		pb;
	Str255			pascalMacPath;
	short			volume;
	short			fileNameLength;
	OSErr			convertError;
	
	convertError = ConvertUnixPathToMacPath(unixPath, &macPath);
	if (convertError != noErr)
		return convertError;
		
	macPathCopy = strdup(macPath);
	if (macPathCopy == NULL) {
		free(macPath);
		return memFullErr;
	}
	
	scanChar = macPathCopy;
	while ((*scanChar != ':') && (*scanChar != '\0'))
		scanChar++;
	*scanChar = '\0';
		
	volume = GetVolumeRefNumFromName(macPathCopy);
	
	free(macPathCopy);

	// Get info about the object.
	PStrFromCStr(macPath, pascalMacPath);
	
	pb.dirInfo.ioNamePtr = pascalMacPath;
	pb.dirInfo.ioVRefNum = volume;
	pb.dirInfo.ioDrDirID = 0;
	pb.dirInfo.ioFDirIndex = 0;
	
	if (PBGetCatInfoSync(&pb) != noErr) {
		free(macPath);
		return paramErr;
	}
	
	scanChar = macPath + strlen(macPath);
	while ((*scanChar != ':') && (scanChar != macPath))
		scanChar--;
	
	if (scanChar == macPath)
		return paramErr;
	
	scanChar++;
		
	fileNameLength = strlen(scanChar);
	if (fileNameLength > 255)
		fileNameLength = 255;
		
	BlockMove(scanChar, fileSpec->name + 1, fileNameLength);
	fileSpec->name[0] = fileNameLength;
	fileSpec->parID = pb.dirInfo.ioDrParID;
	fileSpec->vRefNum = volume;
	
	free(macPath);

	return noErr;

}

int PR_OpenFile(const char *path, int oflag, int mode)
{
	// Macintosh doesn’t really have mode bits, just drop them
	#pragma unused (mode)

    int 	result;
    char	*macPath = NULL;
    OSErr	err = noErr;
        
    
    err = ConvertUnixPathToMacPath(path, &macPath);
    	    
    // Use the standard C library open, using the newPath if we created one
    if (err == noErr) {
		result = open(((macPath == NULL) ? path : (const char *)macPath), oflag);
	    result = (result < 0 ? errno : result);
	}
	else
		result = err;
    
	if (macPath != NULL)
    	free(macPath);
    	
    return result;
}

int PR_Stat(const char *path, struct stat *buf)
{
	OSErr	err = noErr;
	char	*macFileName = NULL;
	int		result;	
	
    err = ConvertUnixPathToMacPath(path, &macFileName);
	
	if (err == noErr)
		result = stat(macFileName, buf);
	else {
		result = -1;
	}
		
	free(macFileName);
	
	return result;
}

FILE *_OS_FOPEN(const char *filename, const char *mode) 
{
	OSErr	err = noErr;
	char	*macFileName = NULL;
	FILE	*result;
	
	
    err = ConvertUnixPathToMacPath(filename, &macFileName);
	
	if (err == noErr)
		result = fopen(macFileName, mode);
	else {
		result = NULL;
		errno = err;
	}
		
	free(macFileName);
	
	return result;
}



int PR_Mkdir(char *unixPath, int mode)
{
	HFileParam		fpb;
	Str255			pMacPath = "\p";
	char			*cMacPath = NULL;
	OSErr			err = -1;

	#pragma unused (mode)					// Mode is ignored on the Mac
	if (unixPath) {
    	err = ConvertUnixPathToMacPath(unixPath, &cMacPath);

    	if (err == noErr) {
	    	PStrFromCStr(cMacPath, pMacPath);
			fpb.ioNamePtr = pMacPath;
			fpb.ioVRefNum = 0;
			fpb.ioDirID = 0L;
			err = PBDirCreateSync((HParmBlkPtr)&fpb);
		}
	}

	free(cMacPath);
	
	if (err != noErr)
		errno = err;
	return (err == noErr ? 0 : -1);
}



int PR_AccessFile(char *path, int amode)
{
	//
	// Emulate the Unix access routine
	//
	
	CInfoPBRec		pb;
	OSErr			err;
	char			*cMacPath = NULL;
	Str255			pacalMacPath;
	int				result = noErr;
	struct stat		info;
	
	
	// Convert to a Mac style path
	err = ConvertUnixPathToMacPath(path, &cMacPath);
	require_action (err == noErr, finish, {result = -1; errno = ENOENT;});
	
	err = stat(cMacPath, &info);
	require_action (err == noErr, finish, {result = -1; errno = ENOENT;});
	
	// If all we’re doing is checking for the existence of the file, we’re out of here.
	// On the Mac, if a file exists, you can read from it.
	// This doesn’t handle remote AppleShare volumes.  Does it need to?
	if ((amode == PR_AF_EXISTS) || (amode == PR_AF_READ_OK)) {
		result = 0;
		goto finish;
	}
	
	PStrFromCStr(cMacPath, pacalMacPath);
	
	pb.hFileInfo.ioNamePtr = pacalMacPath;
	pb.hFileInfo.ioVRefNum = info.st_dev;
	pb.hFileInfo.ioDirID = 0;
	pb.hFileInfo.ioFDirIndex = 0;
	
	err = PBGetCatInfoSync(&pb);
	require_action (err == noErr, finish, {result = -1; errno = ENOENT;});
	
	// Check out all the access permissions.
	
	if (amode == PR_AF_WRITE_OK) {
		// Make sure the file isn’t open by someone else.  Is this correct?
		if (pb.hFileInfo.ioFlAttrib & kEitherForkOpenBit) {
			errno = EACCES;
			result = -1;
		}
	}
	
	finish:
		free(cMacPath);
		return result;
}



long PR_Seek(int fd, long off, int whence) {
	
	return lseek(fd, off, whence);
}



//##############################################################################
//##############################################################################
#pragma mark -
#pragma mark DIRECTORY OPERATIONS

struct PRDirStr {
	char		*currentFileName;
	short		ioVRefNum;
	long		ioDirID;
	short		ioFDirIndex;
};
typedef struct PRDirStr PRDirStr;



short GetVolumeRefNumFromName(const char *cTgtVolName)
{
	Str32				pVolName;
	char				*cVolName = NULL;
	OSErr				err;
	HParamBlockRec		hPB;
	short				refNum = 0;
	
	hPB.volumeParam.ioVolIndex = 0;
	hPB.volumeParam.ioNamePtr = pVolName;
	do {
		hPB.volumeParam.ioVolIndex++;
		err = PBHGetVInfoSync(&hPB);
		CStrFromPStr(pVolName, &cVolName);
		if (strcmp(cTgtVolName, cVolName) == 0) {
			refNum =  hPB.volumeParam.ioVRefNum;
			break;
		}
	} while (err == noErr);
	
	return refNum;
}



PRDir *PR_OpenDirectory(char *cUnixPath)
{
	// Emulate the Unix opendir() routine.

	PRDirStr		*dir = NULL;
	char			*cMacPath = NULL;
	char			*position = NULL;
	char			cVolName[32];
	CInfoPBRec		pb;
	Str255			pascalMacPath;
	
	
	// Allocate the directory structure
	dir = malloc(sizeof(PRDir));
	require (dir != NULL, exitRelease);
		
	// Get the Macintosh path
	require((ConvertUnixPathToMacPath(cUnixPath, &cMacPath) == noErr), exitRelease);
	
	// Get the vRefNum
	position = strchr(cMacPath, MAC_PATH_SEPARATOR);
	if ((position == cMacPath) || (position == NULL))
		dir->ioVRefNum = 0;										// Use application relative searching
	else {
		memset(cVolName, 0, sizeof(cVolName));
		strncpy(cVolName, cMacPath, position-cMacPath);
		dir->ioVRefNum = GetVolumeRefNumFromName(cVolName);
	}

	// Get info about the object.
	PStrFromCStr(cMacPath, pascalMacPath);
	
	pb.dirInfo.ioNamePtr = pascalMacPath;
	pb.dirInfo.ioVRefNum = dir->ioVRefNum;
	pb.dirInfo.ioDrDirID = 0;
	pb.dirInfo.ioFDirIndex = 0;
	require(PBGetCatInfoSync(&pb) == noErr, exitRelease);
	
	// Are we dealing with a directory?
	require(((pb.dirInfo.ioFlAttrib & ioDirFlg) == 0), exitRelease);
	
	// This is a directory, store away the pertinent information
	dir->ioDirID = pb.dirInfo.ioDrDirID;
	dir->currentFileName = NULL;
	dir->ioFDirIndex = 1;						// We post increment.  I.e. index is always the
												// nth. item we should get on the next call.
	goto exit;
	
	exitRelease:
		free(dir);
		dir = NULL;
		
	exit:
		free(cMacPath);
		return dir;
}


PRDirEntry *PR_ReadDirectory(PRDir *dir, int flags)
{
	// Emulate the Unix readdir() routine.
	#pragma unused (flags)			// Mac doesn’t have the concept of . & ..
	
	CInfoPBRec					pb;
	char						*tempStr;
	char						*mappedFile;
	Str255						pascalName = "\p";
	
	if (dir == NULL)
		return NULL;

	// Release the last name read.
	free(dir->currentFileName);
	dir->currentFileName = NULL;
		
	// We’ve got all the info we need, just get info about this guy.
	pb.hFileInfo.ioNamePtr = pascalName;
	pb.hFileInfo.ioVRefNum = dir->ioVRefNum;
	pb.hFileInfo.ioFDirIndex = dir->ioFDirIndex;
	pb.hFileInfo.ioDirID = dir->ioDirID;
	if (PBGetCatInfoSync(&pb) != noErr)
		return NULL;
	
	// Convert the Pascal string to a C string.
	CStrFromPStr(pascalName, &tempStr);
	
	// Map the file if we need to
	mappedFile = MapPartialToFullMacFile(tempStr);
	if (mappedFile == NULL)
		dir->currentFileName = tempStr;
	else {
		dir->currentFileName = mappedFile;
		free(tempStr);
	}
	
	dir->ioFDirIndex++;
	
	return (PRDirEntry *)dir;
}



void PR_CloseDirectory(PRDir *dir)
{
	// Emulate the Unix closedir() routine
	free(dir->currentFileName);
	free(dir);
}


//##############################################################################
//##############################################################################
#pragma mark -
#pragma mark LINK/UNLINK OPERATIONS

int PR_Unlink(const char *path) {UNIMPLEMENTED_ROUTINE}



//##############################################################################
//##############################################################################
#pragma mark -
#pragma mark DLL OPERATIONS


char * PR_GetDLLSearchPath(void)
{
	// Return the colon separated list of paths containing the CFM search path.
	
	//  *** HACK *** 
	//	For now, just return the application’s directory.
	
	OSErr					err;
	ProcessInfoRec			processInfo;
	FSSpec					processFSSpec;
	char					*fullPath = NULL;
	char					*unixFullPath = NULL;
	int						sizeOfFullPath = 0;
	ProcessSerialNumber		thisAppsPSN = {0, kCurrentProcess};
	
	processInfo.processAppSpec = &processFSSpec;
	processInfo.processName = NULL;
	processInfo.processInfoLength = sizeof(ProcessInfoRec);
	
	err = GetProcessInformation(&thisAppsPSN, &processInfo);
	require (err == noErr, exit);
		
	err = GetFullPath(processFSSpec.vRefNum, processFSSpec.parID, &fullPath, &sizeOfFullPath);
	require (err == noErr, exit);
	
	err = ConvertMacPathToUnixPath(fullPath, &unixFullPath);
	free(fullPath);	
	
exit:
	return (err == noErr ? unixFullPath : NULL);
}



//##############################################################################
//##############################################################################
#pragma mark -
#pragma mark STRING OPERATIONS

#if 0
OSErr PStrFromCStr(const char *cString, Str255 pString)
{
	unsigned int	len; 

	len = strlen(cString);
	if (len > 255)
		return (-1);

	pString[0] = len;
	BlockMoveData(cString, &pString[1], len);

	return (0);
}
#endif

//	PStrFromCStr converts the source C string to a destination
//	pascal string as it copies. The dest string will
//	be truncated to fit into an Str255 if necessary.
//  If the C String pointer is NULL, the pascal string's length is set to zero
//
void PStrFromCStr(const char* src, Str255 dst)
{
	short 	length  = 0;
	
	// handle case of overlapping strings
	if ( (void*)src == (void*)dst )
	{
		unsigned char*		curdst = &dst[1];
		unsigned char		thisChar;
				
		thisChar = *(const unsigned char*)src++;
		while ( thisChar != '\0' ) 
		{
			unsigned char	nextChar;
			
			// use nextChar so we don't overwrite what we are about to read
			nextChar = *(const unsigned char*)src++;
			*curdst++ = thisChar;
			thisChar = nextChar;
			
			if ( ++length >= 255 )
				break;
		}
	}
	else if ( src != NULL )
	{
		unsigned char*		curdst = &dst[1];
		short 				overflow = 255;		// count down so test it loop is faster
		register char		temp;
	
		// Can't do the K&R C thing of “while (*s++ = *t++)” because it will copy trailing zero
		// which might overrun pascal buffer.  Instead we use a temp variable.
		while ( (temp = *src++) != 0 ) 
		{
			*(char*)curdst++ = temp;
				
			if ( --overflow <= 0 )
				break;
		}
		length = 255 - overflow;
	}
	dst[0] = length;
}


void CStrFromPStr(ConstStr255Param pString, char **cString)
{
	// Allocates a cString and copies a Pascal string into it.
	unsigned int	len;
	
	len = pString[0];
	*cString = malloc(len+1);
	
	if (*cString != NULL) {
		strncpy(*cString, (char *)&pString[1], len);
		(*cString)[len] = NULL;
	}
}


size_t strlen(const char *source)
{
	size_t currentLength = 0;
	
	if (source == NULL)
		return currentLength;
			
	while (*source++ != '\0')
		currentLength++;
		
	return currentLength;
}

int strcmpcore(const char *str1, const char *str2, int caseSensitive)
{
	char 	currentChar1, currentChar2;

	while (1) {
	
		currentChar1 = *str1;
		currentChar2 = *str2;
		
		if (!caseSensitive) {
			
			if ((currentChar1 >= 'a') && (currentChar1 <= 'z'))
				currentChar1 += ('A' - 'a');
		
			if ((currentChar2 >= 'a') && (currentChar2 <= 'z'))
				currentChar2 += ('A' - 'a');
		
		}
	
		if (currentChar1 == '\0')
			break;
	
		if (currentChar1 != currentChar2)
			return currentChar1 - currentChar2;
			
		str1++;
		str2++;
	
	}
	
	return currentChar1 - currentChar2;
}

int strcmp(const char *str1, const char *str2)
{
	return strcmpcore(str1, str2, true);
}

int strcasecmp(const char *str1, const char *str2)
{
	return strcmpcore(str1, str2, false);
}

#if GENERATING68K
asm void *memset(void *target, int pattern, size_t length)										// Legal asm qualifier
{
				MOVEA.L		4(SP),A0			// target -> A0
				MOVE.W		10(SP),D0			// pattern -> D0, length -> D1
				MOVE.L		12(SP),D1
				CMPI.L		#30,D1
				BLT			end
				
				// Fill D0 with the pattern
				MOVEQ		#0,D2				// Clear D2, we’ll use it as scratch
				MOVE.B		D0,D2				// Fill the bottom byte
				LSL.W		#8,D0				//  
				OR.W		D0,D2
				MOVE.L		D2,D0
				SWAP		D2
				OR.L		D2,D0
				
				// Are we odd aligned?
				MOVE.L		A0,D2				// Copy target address into scratch
				LSR.B		#1,D2				// Sets C bit
				BCC.S		checkAlign2			// If even, check for 16-byte alignment
				MOVE.B		D0,(A0)+			// Take care of odd byte
				SUBQ.L		#1,D1				// Update length
				
				// Are we odd 16-byte word alligned?
checkAlign2:	LSR.B		#1,D2				// Still set from last check
				BCC			totallyAligned
				MOVE.W		D0,(A0)+
				SUBQ.L		#2,D1
			
totallyAligned:	MOVE.L		D1,D2
				LSR.L		#4,D2
				SUBQ.L		#1,D2
copyHunk:		MOVE.L		D0,(A0)+
				MOVE.L		D0,(A0)+
				MOVE.L		D0,(A0)+
				MOVE.L		D0,(A0)+
				SUBQ.L		#1,D2
				BCC			copyHunk
				ANDI.W		#15,D1				// Check done?
				BRA			end
dribble:		MOVE.B		D0,(A0)+
end:			DBF			D1,dribble
				MOVE.L		4(SP),D0			// Return the target
				RTS								
}
#endif

void dprintf(const char *format, ...)
{
    va_list ap;
	char	*buffer;
	
	va_start(ap, format);
	buffer = PR_vsmprintf(format, ap);
	va_end(ap);
	
	debugstr(buffer);
	free(buffer);
}

void
exit(int result)
{
#pragma unused (result)

	if (gPrimaryThread != PR_CurrentThread())
		PR_Exit();
	else
		ExitToShell();
}


//##############################################################################
//##############################################################################
#pragma mark -
#pragma mark MISCELLANEOUS


//
//		***** HACK  FIX THESE ****
//
extern long _MD_GetOSName(char *buf, long count)
{
	long	len;
	
	len = PR_snprintf(buf, count, "Mac OS");
	
	return 0;
}

extern long _MD_GetOSVersion(char *buf, long count)
{
	long	len;
	
	len = PR_snprintf(buf, count, "7.5");

	return 0;
}

extern long _MD_GetArchitecture(char *buf, long count)
{
	long	len;
	
	len = PR_snprintf(buf, count, "PowerPC");

	return 0;
}
