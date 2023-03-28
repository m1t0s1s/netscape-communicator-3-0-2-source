/*
 * @(#)audio.c	1.14 95/09/02 David Connelly
 *
 * Copyright (c) 1995 Sun Microsystems, Inc. All Rights Reserved.
 *
 * Permission to use, copy, modify, and distribute this software
 * and its documentation for NON-COMMERCIAL purposes and without
 * fee is hereby granted provided that this copyright notice
 * appears in all copies. Please refer to the file "copyright.html"
 * for further important copyright and licensing information.
 *
 * SUN MAKES NO REPRESENTATIONS OR WARRANTIES ABOUT THE SUITABILITY OF
 * THE SOFTWARE, EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED
 * TO THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
 * PARTICULAR PURPOSE, OR NON-INFRINGEMENT. SUN SHALL NOT BE LIABLE FOR
 * ANY DAMAGES SUFFERED BY LICENSEE AS A RESULT OF USING, MODIFYING OR
 * DISTRIBUTING THIS SOFTWARE OR ITS DERIVATIVES.
 */

#include <io.h> 
#include <fcntl.h> 
#include <sys/types.h> 
#include <sys/stat.h> 
#include <stdio.h> 
#include <windows.h> 

#include "oobj.h"
#include "interpreter.h"

#include "sun_audio_AudioDevice.h"

#define	MAXBUF	    10		/* Maximum number of buffers */
#define	DATALEN     640		/* Size of wave data buffer */

struct buf {
    WAVEHDR hdr;		/* Wave data header */
    short data[DATALEN];	/* Actual wave data */
    struct buf *next;		/* Next buffer in free list */
};

HWAVEOUT waveHandle;		/* Handle of wave output device */
struct buf *free_list;		/* List of available buffers */
CRITICAL_SECTION free_list_lock;
int free_buffers;		/* Number of buffers in free list */
int total_buffers;		/* Total number of buffers allocated */
HANDLE free_buffer_event;	/* Automatic-reset event triggered whenever
				   a buffer is freed by the callback routine */

/* ulaw8tol16 - Convert 8-bit ulaw to 16-bit linear */

__inline int
ulaw8tol16(int datum)
{
    static const int exp_lut[8] =
	{ 0, 132, 396, 924, 1980, 4092, 8316, 16764 };
    int sign, exponent, mantissa;

    datum = ~datum;
    sign = datum >> 7;
    exponent = (datum >> 4) & 0x07;
    mantissa = datum & 0x0f;
    datum = exp_lut[exponent] + (mantissa << (exponent + 3));
    return (sign < 0 ? -datum : datum);
}

static void
printError(char *f, int r)
{
    char msg[80];

    if (waveOutGetErrorText(r, msg, sizeof(msg)) != 0) {
	sprintf(msg, "Unknown error");
    }
    fprintf(stderr, "Error: %s: %s\n", f, msg);
}
	
static void
audioWrite(char *data, long dataLen)
{
    struct buf *bp;
    int len, i, r;
 
    while (dataLen > 0) {
	if (free_list == NULL && total_buffers < MAXBUF) {
	    /* Expand available buffer space */
	    bp = (struct buf *)malloc(sizeof(struct buf));
	    total_buffers++;
	} else {
	    if (free_list == NULL) {
		/* We must wait for a free buffer */
		while (free_list == NULL) {
		    WaitForSingleObject(free_buffer_event, INFINITE);
		}
	    }
	    EnterCriticalSection(&free_list_lock);
	    bp = free_list;
	    free_list = free_list->next;
	    --free_buffers;
	    LeaveCriticalSection(&free_list_lock);
	    r = waveOutUnprepareHeader(waveHandle, &bp->hdr, sizeof(WAVEHDR));
	    if (r != 0) {
		printError("waveOutUnprepareHeader", r);
		return;
	    }
	}
	len = min(dataLen, DATALEN);
	bp->hdr.lpData = (char *)bp->data;
	bp->hdr.dwBufferLength = len * sizeof(short);
	bp->hdr.dwBytesRecorded = len * sizeof(short);
	bp->hdr.dwUser = (DWORD)bp;
	bp->hdr.dwFlags = 0;
	bp->hdr.dwLoops = 0;
	r = waveOutPrepareHeader(waveHandle, &bp->hdr, sizeof(WAVEHDR));
	if (r != 0) {
	    printError("waveOutPrepareHeader", r);
	    return;
	}
	for (i = 0; i < len; i++) {
	    bp->data[i] = ulaw8tol16(data[i]);
	}
	r = waveOutWrite(waveHandle, &bp->hdr, sizeof(WAVEHDR));
	if (r != 0) {
	    printError("waveOutWrite", r);
	    return;
	}
	data += len;
	dataLen -= len;
    }
}

static void CALLBACK 
waveCallBack(HWAVE waveHandle, UINT msg, DWORD inst, DWORD p1, DWORD p2)
{
    if (msg == MM_WOM_DONE) {
	struct buf *bp = (struct buf *)p1;
	EnterCriticalSection(&free_list_lock);
	bp->next = free_list;
	free_list = bp;
	free_buffers++;
	SetEvent(free_buffer_event);
	LeaveCriticalSection(&free_list_lock);
    }
}

long
sun_audio_AudioDevice_audioOpen(struct Hsun_audio_AudioDevice *aP)
{
///    PCMWAVEFORMAT waveFormat;
    WAVEFORMATEX waveFormat;
    int r;

    if (waveHandle != NULL) {
	return 1;	/* output device already open */
    }
#if 0 /// rjp
    waveFormat.wf.wFormatTag = WAVE_FORMAT_PCM;
    waveFormat.wf.nChannels = 1;
    waveFormat.wf.nSamplesPerSec = 8000;
    waveFormat.wf.nAvgBytesPerSec = 16000;
    waveFormat.wf.nBlockAlign = 2;
    waveFormat.wBitsPerSample = 16;
#else
    waveFormat.wFormatTag = WAVE_FORMAT_PCM;
    waveFormat.nChannels = 1;
    waveFormat.nSamplesPerSec = 8000;
    waveFormat.nAvgBytesPerSec = 16000;
    waveFormat.nBlockAlign = 2;
    waveFormat.wBitsPerSample = 16;
    waveFormat.cbSize= sizeof(waveFormat);
#endif

    r = waveOutOpen(&waveHandle, 
		    WAVE_MAPPER,
///		    (WAVEFORMAT *)&waveFormat,
		    &waveFormat,
		    (DWORD)waveCallBack,
		    0,
		    CALLBACK_FUNCTION);
    if (r != 0) {
	printError("waveOutOpen", r);
	return -1;
    }

    (void)waveOutReset(waveHandle);

    free_list = NULL;
    InitializeCriticalSection(&free_list_lock);
    free_buffers = 0;
    free_buffer_event = CreateEvent(NULL, FALSE, FALSE, NULL);
    total_buffers = 0;

    /*
     * HACK: If we immediately start writing valid audio data to the device
     * then the sound is choppy in the beginning. Writing a null packet to
     * to the device first seems to fix this problem although I have no idea
     * why - DAC
     */
    {
        char null[DATALEN];
	int i;
	for (i = 0; i < DATALEN; i++) {
	    null[i] = 127;
	}
	audioWrite(null, DATALEN);
    }

    return 1;
}

void
sun_audio_AudioDevice_audioClose(struct Hsun_audio_AudioDevice *aP)
{
    struct buf *bp, *next;
    int r;

    /* Force cancellation of any pending buffers */
    (void)waveOutReset(waveHandle);

    /* Wait until all pending buffers have been freed */
    while (free_buffers < total_buffers) {
	WaitForSingleObject(free_buffer_event, INFINITE);
    }

    /* Close the device */
    if ((r = waveOutClose(waveHandle)) != 0) {
	printError("waveOutClose", r);
	return;
    }

    /* Free allocated buffers */
    for (bp = free_list; bp != NULL; bp = next) {
	next = bp->next;
	(void)free(bp);
    }
	
    waveHandle = NULL;
}


void
sun_audio_AudioDevice_audioWrite(struct Hsun_audio_AudioDevice *Hthis,
				     HArrayOfByte *Hbuf,
				     long dataLen)
{
    if (Hbuf == NULL) {
	SignalError(0, JAVAPKG "NullPointerException", 0);
    } else if (dataLen > 0) {
	char *data = unhand(Hbuf)->body;
	if (obj_length(Hbuf) < dataLen) {
	    dataLen = obj_length(Hbuf);
	}
	audioWrite(data, dataLen);
    }
}
