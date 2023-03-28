/*
 * @(#)audio.c	1.8 95/08/22 Eugene Kuerner
 *
 * Copyright (c) 1994 Sun Microsystems, Inc. All Rights Reserved.
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


#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/errno.h>

#include "oobj.h"
#include "interpreter.h"

#include "sun_audio_AudioDevice.h"


static 	int	audioFd = -1;

long
sun_audio_AudioDevice_audioOpen(struct Hsun_audio_AudioDevice *aP)
{
    extern int errno;

    if (audioFd < 0) {
	errno = 0;
  	if ((audioFd = open("/dev/audio", O_WRONLY | O_NONBLOCK)) < 0) {
	    switch (errno) {
	      case ENODEV:
	      case EACCES:
		/* Dont bother, there is no audio device */
		return -1;
	    }
	    /* Try again later, probably busy */
	    return 0;
	}
    }
    return 1;
}


void
sun_audio_AudioDevice_audioClose(struct Hsun_audio_AudioDevice *aP)
{
    close(audioFd);
    audioFd = -1;
    return;
}


void
sun_audio_AudioDevice_audioWrite(struct Hsun_audio_AudioDevice *aP,
				       HArrayOfByte *buf,
				       long len)
{
    char *data;
    long dataLen;

    if (buf == 0) {
	SignalError(0, JAVAPKG "NullPointerException", 0);
	return;
    }
    if (len <= 0) {
	return;
    }

    data = unhand(buf)->body;
    dataLen = obj_length(buf);

    if (dataLen < len) {
	len = dataLen;
    }
    write(audioFd, data, len);
    return;
}
