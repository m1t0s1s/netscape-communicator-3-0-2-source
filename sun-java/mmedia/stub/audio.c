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
    return 1;
}


void
sun_audio_AudioDevice_audioClose(struct Hsun_audio_AudioDevice *aP)
{
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

    /* DROP */
}
