

#include <stdio.h>
#include <fcntl.h>
#include <types.h>
#include <errno.h>

#include "oobj.h"
#include "interpreter.h"

#include <Sound.h>
#include <Resources.h>
#include <Gestalt.h>
#include <ConditionalMacros.h>

// Declare this here since including fp.h conflicts with math.h.
// We want to use math.h everywhere else in the project.
extern void ldtox80 ( const long double *x, extended80 *x80 );


//
// Constants
//
static const UInt32 kJavaSoundSampleRate		= 8000;
static const Fixed	kJavaSoundSampleRateFixed 	= (kJavaSoundSampleRate << 16);	
static const UInt32	kJavaSoundBufferSize		= 800L;


//
// JavaSoundBuffer: Our sample-data buffer.
//
// We allocate fixed-size buffers so we can reuse them easily.
// Buffers are only deallocated when the audio channel is closed.
//
// Windows uses a buffer free-list, but we cannot do so because we 
// are notified that a sound sample is finished at interrupt time,
// and we have no synchronization primitives.  
//
// So all allocated buffers are kept in a single list.  They are
// marked as "inUse".  When a new buffer is needed, the list is
// scanned for a buffer not in use.  If one is not available, 
// a new one is allocated.
//
typedef struct JavaSoundBuffer JavaSoundBuffer;
typedef struct JavaSoundBuffer
{
	JavaSoundBuffer	*next;
	UInt32			inUse;
	ExtSoundHeader	header;								// header includes the sample data at the end (.sampleArea)
	char			samples[kJavaSoundBufferSize-1];	// -1 because ExtSoundHeader has 1 character at the end
};



//
// Gestalt fun
//
inline static void		CheckSoundCapabilities(void);
inline static Boolean	CanPlaySixteenBit(void);


// 
// Conversion routines
//
static inline short	ULAW_ToMacLinear(int datum);
static Boolean		ConvertBufferToLinear(char *ulawSample,	short *linearSample, UInt32 ulawLen);

//
// Buffer setup/maintenance
//
inline static JavaSoundBuffer *	GetJavaSoundBuffer(void);
inline static void				ReleaseJavaSoundBuffer(JavaSoundBuffer *buffer);
static void						SetupJavaSoundHeader(ExtSoundHeader *header);
static void						FreeUnusedJavaSoundBuffers(void);


//
// Callback to let us know a sound command is finished
//
static pascal void 	JavaSoundCallback(SndChannelPtr chan, SndCommand *cmd);
#if GENERATINGCFM
static RoutineDescriptor gJavaSoundCallbackRD = BUILD_ROUTINE_DESCRIPTOR(uppSndCallBackProcInfo, JavaSoundCallback);
#endif


//
// mmm... globals
//
static JavaSoundBuffer		*gJavaSoundBufferList 			= NULL;		// allocated buffers
static SndChannel			*gJavaSoundChannel 				= NULL;		// our one channel - LAME
static UInt32				gJavaSoundChannelRefCount 		= 0;		// # of open calls for the one lame channel
static SInt32				gSoundCapabilities				= -1;		// -1 unknown, 0 no 16-bit, 1 16-bit ok

static UInt32				gSilenceCount					= 0;

#ifdef DEBUG
static UInt32				gMaxJavaSoundBufferCount		= 0;
static UInt32				gJavaSoundBufferCount			= 0;
#endif


//
// Determine whether we can play 16-bit audio...
//
inline static void
CheckSoundCapabilities(void)
{
	OSErr	err;
	long	soundBits;
	
	err = Gestalt(gestaltSoundAttr, &soundBits);
	
	if (err == 0 && (soundBits & (1 << gestalt16BitAudioSupport)) != 0)
		gSoundCapabilities = 1;
	else
		gSoundCapabilities = 0;
}

inline static Boolean	
CanPlaySixteenBit(void)	
{ 
	if (gSoundCapabilities == -1)
		CheckSoundCapabilities();
		
	return (gSoundCapabilities == 1);
}



//
//	sun_audio_AudioDevice_audioOpen
//
//	Opens a Mac sound channel, if one has not already been opened.
//	If one is already open, this bumps a ref count for it.
//
long
sun_audio_AudioDevice_audioOpen(struct Hsun_audio_AudioDevice *ap)
{		
#pragma unused (ap)

	OSErr	err;
	long	result = -1;
		
	if ( ! CanPlaySixteenBit() )
		return 1;
			
	if (gJavaSoundChannel != NULL)
	{
		gJavaSoundChannelRefCount++;
		return 1;
	}
	
		
	
	gJavaSoundChannel = (SndChannel *) malloc(sizeof(SndChannel));
	if (gJavaSoundChannel == NULL) return -1;
	memset(gJavaSoundChannel, 0, sizeof(SndChannel));
	
	//
	// std queue length is 128, but playing sampled sound
	// really only requires one command at a time...
	//
	gJavaSoundChannel->qLength = 32;
	
	err	= SndNewChannel(	&gJavaSoundChannel,						// channel to initialize
							sampledSynth,							// will play sampled sound
							initStereo, 							// ??, sure...
#if GENERATINGCFM
							&gJavaSoundCallbackRD					// no callback
#else
							JavaSoundCallback
#endif
							);

	if (err == noErr)
	{
		result = 1;
		gJavaSoundChannelRefCount++;
	}
	else
	{
		if (gJavaSoundChannelRefCount == 0)
		{
			free(gJavaSoundChannel);
			gJavaSoundChannel = NULL;
		}
	}
	
	return result;
}



//
// JavaSoundCallback
//
// Called when a sound sample is finished. 
// We mark the buffer as unused.
//
static pascal void 
JavaSoundCallback(SndChannelPtr chan, SndCommand *cmd)
{
#pragma unused (chan)

	if (cmd != 0 && cmd->param2 != 0)
	{
		JavaSoundBuffer	*buffer = (JavaSoundBuffer *) cmd->param2;
		buffer->inUse = 0;
	}
}



//
// sun_audio_AudioDevice_audioClose
//
// Useless.  Apparently never called.
//
void
sun_audio_AudioDevice_audioClose(struct Hsun_audio_AudioDevice *aP)
{
#pragma unused (aP)

	if ( ! CanPlaySixteenBit() )
		return;

	if (gJavaSoundChannel != NULL && (--gJavaSoundChannelRefCount) == 0)
	{
		JavaSoundBuffer	*	buf;
	
		//
		// Kill the sound channel
		//
		SndDisposeChannel(gJavaSoundChannel, true);
		free(gJavaSoundChannel);
		gJavaSoundChannel = NULL;
		
		
		//
		// Free all allocated sample buffers
		// 
		for (buf = gJavaSoundBufferList; buf != NULL;)
		{
			JavaSoundBuffer	*next = buf->next;
		
			free(buf);
			buf = next;
		}
		
	#ifdef DEBUG
		gJavaSoundBufferCount = 0;
	#endif
	
		gJavaSoundBufferList = NULL;
	}
}


//
// sun_audio_AudioDevice_audioWrite
//
void
sun_audio_AudioDevice_audioWrite(	struct Hsun_audio_AudioDevice	*	aP,
				       				HArrayOfByte 					*	buf,
				       				long 								len	)
{
#pragma unused (aP)

	OSErr				err;
	char 			*	data;			// ULAW data buffer
	JavaSoundBuffer	*	linearBuffer;	// sound header + data
	ExtSoundHeader	* 	header;			// ptr to header in linearBuffer
	SndCommand			command;		// sound command to play from buffer
		
	if ( ! CanPlaySixteenBit() )
		return;
		
	// Handle goofy cases
    if (buf == 0) 
    {
		SignalError(0, JAVAPKG "NullPointerException", 0);
		return;
    }
    if (len <= 0) 
    {	
		return;
    }
    
 	
 	//
 	// Get the ULAW data buffer, and allocate
 	// the linear data buffer.
 	//
 	
    data = unhand(buf)->body; 
    if (obj_length(buf) < len)
    	len = obj_length(buf);
	
	len = len << 1;	// 8 bit -> 16 bit
	
	
	
	//
	// Loop 'til we've played all the data
	//
	
	while (len > 0)
	{
		long	linearBlockLen;	// 16 bit length	
		long	ulawBlockLen;	// 8 
		
		linearBlockLen = MIN(len, kJavaSoundBufferSize);
		ulawBlockLen   = linearBlockLen >> 1;
		
		// Get an available buffer or allocate a new one
		linearBuffer = GetJavaSoundBuffer();
		if (linearBuffer == NULL)	return;		
	
		// Convert the 8-bit ULAW to 16-bit linear
		header = &(linearBuffer->header);
		if ( ! ConvertBufferToLinear(data, (short *) &(header->sampleArea[0]), ulawBlockLen))
		{
			// SILENCE !
			ReleaseJavaSoundBuffer(linearBuffer);
			
			// ¥¥¥ÊFIX ME HACK !!! - 
			// 
			// this is the only reasonable chance we get
			// to free our allocated buffers
			//
			gSilenceCount++;
			
			if (gSilenceCount > 20)
			{
				FreeUnusedJavaSoundBuffers();
				gSilenceCount = 0;
			}
			return;
		}
		
		gSilenceCount = 0;
		
		// The rest of the header is set up by GetJavaSoundBuffer()
		header->numFrames = ulawBlockLen;	// # bytes, not samples
		
		// Play this puppy.
		command.cmd 	= bufferCmd;
		command.param1	= 0;
		command.param2	= (long) header;		
		err = SndDoCommand(	gJavaSoundChannel, 		// our sound channel
							&command, 				// command to play sample from buffer
							true	);				// wait for space in command queue, if necessary 
		
		
		// Send a callback command so that
		// our callback func will run when the
		// sound sample is finished.
		// 
		// This lets us mark the sample buffer
		// as unused.
		//
		if (err == noErr)
		{
			command.cmd		= callBackCmd;
			command.param1	= 0;
			command.param2	= (long) linearBuffer;
			
			SndDoCommand(	gJavaSoundChannel,
							&command,
							true	);
		}
		else
		{
			ReleaseJavaSoundBuffer(linearBuffer);	// BAIL !
			return;
		}
			
		// moving on....
		len		-= linearBlockLen;	
		data	+= ulawBlockLen;
	}

}

//
// GetJavaSoundBuffer
//
// Returns an unused sound buffer, either from the buffer
// list, or newly allocated and added to the list.
//
inline static JavaSoundBuffer *
GetJavaSoundBuffer(void)
{
	JavaSoundBuffer	*curBuffer;

	//
	// It would be really nice to have a free-list for the buffers,
	// but since we're notified that a buffer is free at interrupt time,
	// we can't go manipulating the list at that point (unless we were to
	// turn interrupts off)
	//	
	
	for (curBuffer = gJavaSoundBufferList; curBuffer != NULL; curBuffer = curBuffer->next)
	{
		if (curBuffer->inUse == 0)
		{
			curBuffer->inUse = 1;
			return curBuffer;
		}
	}

	
	// Must allocate a new buffer
	//
	// Set up the sound header, mark the buffer
	// as used, and add it to the buffer list.
	//
	curBuffer = malloc(sizeof(JavaSoundBuffer));
	if (curBuffer != NULL)
	{
		SetupJavaSoundHeader(&(curBuffer->header));
		curBuffer->inUse 	= 1;
		curBuffer->next		= gJavaSoundBufferList;
		gJavaSoundBufferList= curBuffer;

#ifdef DEBUG
		gJavaSoundBufferCount++;
		
		if (gJavaSoundBufferCount > gMaxJavaSoundBufferCount)
			gMaxJavaSoundBufferCount = gJavaSoundBufferCount;
#endif		
	}
	
	return curBuffer;
}



//
// ReleaseJavaSoundBuffer
//
// Mark a buffer as unused
// 
inline static void
ReleaseJavaSoundBuffer(JavaSoundBuffer *buffer)
{
	buffer->inUse = 0;
}

//
// FreeUnusedJavaSoundBuffers
//
// Free all unused buffers
//
static void
FreeUnusedJavaSoundBuffers(void)
{
	JavaSoundBuffer	*curBuffer, *prevBuffer = NULL;
	
	for (curBuffer = gJavaSoundBufferList; curBuffer != NULL;)
	{
		JavaSoundBuffer	*nextBuffer = curBuffer->next;
		
		// If the buffer isn't being used, get rid of it
		if (curBuffer->inUse == 0)
		{
			if (prevBuffer != NULL)
				prevBuffer->next = curBuffer->next;
			else
				gJavaSoundBufferList = curBuffer->next;
							
			free(curBuffer);	
		
		#ifdef DEBUG
			gJavaSoundBufferCount--;
		#endif
			
		}
		else
			// don't bump prevBuffer if we just freed curBuffer
			prevBuffer = curBuffer;
			
		curBuffer = nextBuffer;
	}
	
}


//
// SetupJavaSoundHeader
//
// Setup the static values of the sound header
//
static void
SetupJavaSoundHeader(ExtSoundHeader	*	header)
						
{
	long double		aiffRate;
	
	memset(header, 0, sizeof(*header));
//	header->samplePtr 		= NULL;							// sample will be embedded at end of header
	header->numChannels		= 1;
	header->sampleRate		= kJavaSoundSampleRateFixed;		
//	header->loopStart		= 0;					
//	header->loopEnd			= 0;					
	header->encode			= extSH;
	header->baseFrequency	= 127;	
//	header->numFrames		= 0;
//	header->markerChunk		= NULL;
//	header->instrumentChunks= NULL;
//	header->AESRecording	= NULL;
	header->sampleSize		= 16;
	
	aiffRate = 8000.0;
#if GENERATINGPOWERPC
	ldtox80(&aiffRate, &(header->AIFFSampleRate));
#endif
}


//
// ConvertBufferToLinear
//
// Convert a buffer of ulaw data to linear data.
// The given length is the length in bytes of the ulaw data.
//
// Returns false if the sample is silence.
//
static Boolean
ConvertBufferToLinear(	char	*	ulawSample,
						short	*	linearSample,
						UInt32		ulawLen		)
{
	int		i;
	Boolean	silent = true;
	
	for (i=0; i<ulawLen; i++, linearSample++, ulawSample++)
	{
		*linearSample = ULAW_ToMacLinear(*ulawSample);
		silent = (silent && *ulawSample == 0x7F);
	}
	
	return !silent;
}			



//
// ULAW_ToMacLinear
//
// Convert sample from 8-bit ULAW encoding to 16-bit linear
//
static inline short
ULAW_ToMacLinear(int datum)
{
    int sign, exponent, mantissa;

	static const int exp_lut[8] =
	{ 0, 132, 396, 924, 1980, 4092, 8316, 16764 };
   
    datum		= ~datum;
    sign 		= datum >> 7;
    exponent 	= (datum >> 4) & 0x07;
    mantissa 	= datum & 0x0f;
    datum 		= exp_lut[exponent] + (mantissa << (exponent + 3));

    return (sign != 0 ? -datum : datum);
}


