#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/errno.h>
#include "prlog.h"
#include "prmem.h"
#include <errno.h>

#include "oobj.h"
#include "interpreter.h"

#include "sun_audio_AudioDevice.h"

#include <audio.h>

#define JAVAIO "java/io/"

ALport _mmedia_alport;
ALconfig _mmedia_alconfig;
short *_mmedia_ulawbuf;

#define ULAW_BUFFER_SIZE	1024	/* samples, not bytes */

/*
** Table which converts ulaw input data into its signed short equivalent
*/
static short ulaw_table[256] = {
    -32124, -31100, -30076, -29052, -28028, -27004, -25980, -24956,
    -23932, -22908, -21884, -20860, -19836, -18812, -17788, -16764,
    -15996, -15484, -14972, -14460, -13948, -13436, -12924, -12412,
    -11900, -11388, -10876, -10364,  -9852,  -9340,  -8828,  -8316,
     -7932,  -7676,  -7420,  -7164,  -6908,  -6652,  -6396,  -6140,
     -5884,  -5628,  -5372,  -5116,  -4860,  -4604,  -4348,  -4092,
     -3900,  -3772,  -3644,  -3516,  -3388,  -3260,  -3132,  -3004,
     -2876,  -2748,  -2620,  -2492,  -2364,  -2236,  -2108,  -1980,
     -1884,  -1820,  -1756,  -1692,  -1628,  -1564,  -1500,  -1436,
     -1372,  -1308,  -1244,  -1180,  -1116,  -1052,   -988,   -924,
      -876,   -844,   -812,   -780,   -748,   -716,   -684,   -652,
      -620,   -588,   -556,   -524,   -492,   -460,   -428,   -396,
      -372,   -356,   -340,   -324,   -308,   -292,   -276,   -260,
      -244,   -228,   -212,   -196,   -180,   -164,   -148,   -132,
      -120,   -112,   -104,    -96,    -88,    -80,    -72,    -64,
       -56,    -48,    -40,    -32,    -24,    -16,     -8,      0,
     32124,  31100,  30076,  29052,  28028,  27004,  25980,  24956,
     23932,  22908,  21884,  20860,  19836,  18812,  17788,  16764,
     15996,  15484,  14972,  14460,  13948,  13436,  12924,  12412,
     11900,  11388,  10876,  10364,   9852,   9340,   8828,   8316,
      7932,   7676,   7420,   7164,   6908,   6652,   6396,   6140,
      5884,   5628,   5372,   5116,   4860,   4604,   4348,   4092,
      3900,   3772,   3644,   3516,   3388,   3260,   3132,   3004,
      2876,   2748,   2620,   2492,   2364,   2236,   2108,   1980,
      1884,   1820,   1756,   1692,   1628,   1564,   1500,   1436,
      1372,   1308,   1244,   1180,   1116,   1052,    988,    924,
       876,    844,    812,    780,    748,    716,    684,    652,
       620,    588,    556,    524,    492,    460,    428,    396,
       372,    356,    340,    324,    308,    292,    276,    260,
       244,    228,    212,    196,    180,    164,    148,    132,
       120,    112,    104,     96,     88,     80,     72,     64,
        56,     48,     40,     32,     24,     16,      8,      0
};

long
sun_audio_AudioDevice_audioOpen(struct Hsun_audio_AudioDevice *aP)
{
    long p[2];
    int rv;

    PR_LOG(IO, out, ("audio open"));

    /* Now open a port */
    if (!_mmedia_alport) {
	/*
	** Make audio configuration for 16 bit twos complement mono
	** samples
	*/
	_mmedia_alconfig = ALnewconfig();
	if (!_mmedia_alconfig) {
	    SignalError(0, JAVAIO "IOException", 0);
	    return -1;
	}
	ALsetwidth(_mmedia_alconfig, AL_SAMPLE_16);
	ALsetchannels(_mmedia_alconfig, 1);

	_mmedia_alport = ALopenport("java audio", "w", _mmedia_alconfig);
	if (!_mmedia_alport) {
	    PR_LOG(IO, out, ("audio open port fails: %d", oserror()));
	    SignalError(0, JAVAIO "IOException", 0);
	    return -1;
	}
	_mmedia_ulawbuf = (short*) malloc(sizeof(short) * ULAW_BUFFER_SIZE);
    }

    /*
    ** Configure for 8000hz just like sun. Do this every time we go to
    ** play audio in case some other application messed it up
    */
    p[0] = AL_OUTPUT_RATE;
    p[1] = AL_RATE_8000;
    ALsetparams(AL_DEFAULT_DEVICE, &p[0], 2);

    return 1;
}

void
sun_audio_AudioDevice_audioClose(struct Hsun_audio_AudioDevice *aP)
{
    /* Don't close the port, keep it open */
}

void
sun_audio_AudioDevice_audioWrite(struct Hsun_audio_AudioDevice *aP,
				 HArrayOfByte *buf, long len)
{
    unsigned char *data;
    long dataLen, n;
    short *ut;

    if (buf == 0) {
	SignalError(0, JAVAPKG "NullPointerException", 0);
	return;
    }
    if (len <= 0) {
	return;
    }

    data = (unsigned char*) unhand(buf)->body;
    dataLen = obj_length(buf);

    if (dataLen < len) {
	len = dataLen;
    }

    ut = ulaw_table;
    while (len) {
	short *sp;
	long amount = len;
	if (amount > ULAW_BUFFER_SIZE) {
	    amount = ULAW_BUFFER_SIZE;
	}

	/*
	** Convert ulaw data into signed audio data. Assume that incoming
	** data is 32 bit aligned (which it will be because it's java
	** data allocated out of the GC heap and the GC heap guarantees
	** 32 bit alignment...
	*/
	sp = _mmedia_ulawbuf;
	n = amount;
	while (n >= 16) {
	    unsigned long l1, l2, l3, l4;

	    l1 = *(long*)data;
	    l2 = *(long*)(data + 4);		/* prefetch */
	    l3 = *(long*)(data + 8);
	    l4 = *(long*)(data + 12);
	    data += 16;

	    sp[0] = ut[l1 >> 24];
	    sp[1] = ut[(l1 >> 16) & 0xff];
	    sp[2] = ut[(l1 >> 8) & 0xff];
	    sp[3] = ut[l1 & 0xff];

	    sp[4] = ut[l2 >> 24];
	    sp[5] = ut[(l2 >> 16) & 0xff];
	    sp[6] = ut[(l2 >> 8) & 0xff];
	    sp[7] = ut[l2 & 0xff];

	    sp[8] = ut[l3 >> 24];
	    sp[9] = ut[(l3 >> 16) & 0xff];
	    sp[10] = ut[(l3 >> 8) & 0xff];
	    sp[11] = ut[l3 & 0xff];

	    sp[12] = ut[l4 >> 24];
	    sp[13] = ut[(l4 >> 16) & 0xff];
	    sp[14] = ut[(l4 >> 8) & 0xff];
	    sp[15] = ut[l4 & 0xff];
	    sp += 16;
	    n -= 16;
	}
	while (n >= 4) {
	    unsigned long l1;

	    l1 = *(long*)data;
	    data += 4;

	    sp[0] = ut[l1 >> 24];
	    sp[1] = ut[(l1 >> 16) & 0xff];
	    sp[2] = ut[(l1 >> 8) & 0xff];
	    sp[3] = ut[l1 & 0xff];
	    sp += 4;
	    n -= 4;
	}
	while (--n >= 0) {
	    *sp++ = ulaw_table[*data++];
	}

	if (ALwritesamps(_mmedia_alport, _mmedia_ulawbuf, amount) < 0) {
	    PR_LOG(IO, out, ("audio write error: %d", oserror()));
	    SignalError(0, JAVAIO "IOException", 0);
	    break;
	}
	len -= amount;
    }
}
