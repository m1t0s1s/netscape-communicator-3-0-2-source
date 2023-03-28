/*
 * 
 * LICENSE AGREEMENT NON-COMMERCIAL USE (Random Seed Generation)
 * 
 * Netscape Communications Corporation ("Netscape") hereby grants you a
 * non-exclusive, non-transferable license to use the source code version
 * of the accompanying random seed generation software (the "Software")
 * subject to the following terms:
 * 
 * 1.  You may use, modify and/or incorporate the Software, in whole or in
 * part, into other software programs (a "Derivative Work") solely in
 * order to evaluate the Software's features, functions and capabilities.
 * 
 * 2.  You may not use the Software or Derivative Works for
 * revenue-generating purposes.  You may not:
 * 
 * 	(a)  license or distribute the Software or any Derivative Work
 * 	in any manner that generates license fees, royalties,
 * 	maintenance fees, upgrade fees or any other form of income.
 * 
 * 	(b)  use the Software or a Derivative Work to provide services
 * 	to others for which you are compensated in any manner.
 * 
 * 	(c)  distribute the Software or a Derivative Work without
 * 	written agreement from the end user to abide by the terms of
 * 	Sections 1 and 2 of this Agreement.
 * 
 * 3.  This license is granted free of charge.
 * 
 * 4.  Any modification of the Software or Derivative Work must
 * prominently state in the code and associated documentation that it has
 * been modified, the date modified and the identity of the person(s) who
 * made the modifications.
 * 
 * 5.  Licensee will promptly notify Netscape of any proposed or
 * implemented improvement in the Software and grants Netscape the
 * perpetual right to incorporate any such improvements into the Software
 * and into Netscape's products which incorporate the Software (which may
 * be distributed to third parties).  Licensee will also promptly notify
 * Netscape of any alleged deficiency in the Software.  If Netscape
 * requests, Licensee will provide Netscape with a copy of each Derivative
 * Work Licensee creates.
 * 
 * 6.  Any copy of the Software or Derivative Work shall include a copy of
 * this Agreement, Netscape's copyright notices and the disclaimer of
 * warranty and limitation of liability.
 * 
 * 7.  Title, ownership rights, and intellectual property rights in and to
 * the Software shall remain in Netscape and/or its suppliers.  You may
 * use the Software only as provided in this Agreement. Netscape shall
 * have no obligation to provide maintenance, support, upgrades or new
 * releases to you or any person to whom you distribute the Software or a
 * Derivative Work.
 * 
 * 8.  Netscape may use Licensee's name in publicity materials as a
 * licensee of the Software.
 * 
 * 9.  Disclaimer of Warranty.  THE SOFTWARE IS LICENSED "AS IS" WITHOUT
 * WARRANTY OF ANY KIND, INCLUDING WITHOUT LIMITATION,  PERFORMANCE,
 * MERCHANTABILITY OR FITNESS FOR ANY PARTICULAR PURPOSE.  THE ENTIRE RISK
 * AS TO THE RESULTS AND PERFORMANCE OF THE SOFTWARE IS ASSUMED BY YOU.
 * 
 * 10.  Limitation of Liability. UNDER NO CIRCUMSTANCES AND UNDER NO LEGAL
 * THEORY SHALL NETSCAPE OR ITS SUPPLIERS BE LIABLE TO YOU OR ANY OTHER
 * PERSON FOR ANY INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES
 * OF ANY CHARACTER INCLUDING WITHOUT LIMITATION ANY COMMERCIAL DAMAGES OR
 * LOSSES , EVEN IF NETSCAPE HAS BEEN INFORMED OF THE POSSIBILITY OF SUCH
 * DAMAGES.
 * 
 * 11.  You may not download or otherwise export or reexport the Software
 * or any underlying information or technology except in full compliance
 * with all United States and other applicable laws and regulations.
 * 
 * 10. Either party may terminate this Agreement immediately in the event
 * of default by the other party.  You may also terminate this Agreement
 * at any time by destroying the Software and all copies thereof.
 * 
 * 11.  Use, duplication or disclosure by the United States Government is
 * subject to restrictions set forth in subparagraphs (a) through (d) of
 * the Commercial Computer-Restricted Rights clause at FAR 52.227-19 when
 * applicable, or in subparagraph (c)(1)(ii) of the Rights in Technical
 * Data and Computer Program clause at DFARS 252.227-7013, and in similar
 * clauses in the NASA FAR Supplement. Contractor/manufacturer is Netscape
 * Communications Corporation, 501 East Middlefield Road, Mountain View,
 * CA 94043.
 * 
 * 12. This Agreement shall be governed by and construed under California
 * law as such law applies to agreements between California residents
 * entered into and to be performed entirely within California, except as
 * governed by Federal law.
 * 
 */
#include "xp_core.h"
#include "xp_file.h"
#include "secrng.h"
#ifdef XP_MAC
#include <Events.h>
#include <OSUtils.h>
#include <QDOffscreen.h>

/* Static prototypes */
static size_t CopyLowBits(void *dst, size_t dstlen, void *src, size_t srclen);
void FE_ReadScreen();

static size_t CopyLowBits(void *dst, size_t dstlen, void *src, size_t srclen)
{
    union endianness {
        int32 i;
        char c[4];
    } u;

    if (srclen <= dstlen) {
        memcpy(dst, src, srclen);
        return srclen;
    }
    u.i = 0x01020304;
    if (u.c[0] == 0x01) {
        /* big-endian case */
        memcpy(dst, (char*)src + (srclen - dstlen), dstlen);
    } else {
        /* little-endian case */
        memcpy(dst, src, dstlen);
    }
    return dstlen;
}

size_t RNG_GetNoise(void *buf, size_t maxbytes)
{
    uint32 c = TickCount();
 	return CopyLowBits(buf, maxbytes,  &c, sizeof(c));
}

void RNG_FileForRNG(char *filename)
{   
    unsigned char buffer[BUFSIZ];
    size_t bytes;
    XP_File file;
	unsigned long totalFileBytes = 0;

	if (filename == NULL)	/* For now, read in global history if filename is null */
		file = XP_FileOpen(NULL, xpGlobalHistory,XP_FILE_READ_BIN);
	else
		file = XP_FileOpen(NULL, xpURL,XP_FILE_READ_BIN);
    if (file != NULL) {
        for (;;) {
            bytes = XP_FileRead(buffer, sizeof(buffer), file);
            if (bytes == 0) break;
            RNG_RandomUpdate( buffer, bytes);
            totalFileBytes += bytes;
            if (totalFileBytes > 100*1024) break;	/* No more than 100 K */
        }
		XP_FileClose(file);
    }
    /*
     * Pass yet another snapshot of our highest resolution clock into
     * the hash function.
     */
    bytes = RNG_GetNoise(buffer, sizeof(buffer));
    RNG_RandomUpdate(buffer, bytes);
}

void RNG_SystemInfoForRNG()
{
/* Time */
	{
		unsigned long sec;
		size_t bytes;
		GetDateTime(&sec);	/* Current time since 1970 */
		RNG_RandomUpdate( &sec, sizeof(sec));
	    bytes = RNG_GetNoise(&sec, sizeof(sec));
	    RNG_RandomUpdate(&sec, bytes);
    }
/* User specific variables */
	{
		MachineLocation loc;
		ReadLocation(&loc);
		RNG_RandomUpdate( &loc, sizeof(loc));
	}
/* User name */
	{
		unsigned long userRef;
		Str32 userName;
		GetDefaultUser(&userRef, userName);
		RNG_RandomUpdate( &userRef, sizeof(userRef));
		RNG_RandomUpdate( userName, sizeof(userName));
	}
/* Mouse location */
	{
		Point mouseLoc;
		GetMouse(&mouseLoc);
		RNG_RandomUpdate( &mouseLoc, sizeof(mouseLoc));
	}
/* Keyboard time threshold */
	{
		SInt16 keyTresh = LMGetKeyThresh();
		RNG_RandomUpdate( &keyTresh, sizeof(keyTresh));
	}
/* Last key pressed */
	{
		SInt8 keyLast;
		keyLast = LMGetKbdLast();
		RNG_RandomUpdate( &keyLast, sizeof(keyLast));
	}
/* Volume */
	{
		UInt8 volume = LMGetSdVolume();
		RNG_RandomUpdate( &volume, sizeof(volume));
	}
/* Current directory */
	{
		SInt32 dir = LMGetCurDirStore();
		RNG_RandomUpdate( &dir, sizeof(dir));
	}
/* Process information about all the processes in the machine */
	{
		ProcessSerialNumber 	process;
		ProcessInfoRec pi;
	
		process.highLongOfPSN = process.lowLongOfPSN  = kNoProcess;
		
		while (GetNextProcess(&process) == noErr)
		{
			FSSpec fileSpec;
			pi.processInfoLength = sizeof(ProcessInfoRec);
			pi.processName = NULL;
			pi.processAppSpec = &fileSpec;
			GetProcessInformation(&process, &pi);
			RNG_RandomUpdate( &pi, sizeof(pi));
			RNG_RandomUpdate( &fileSpec, sizeof(fileSpec));
		}
	}
	
/* Heap */
	{
		THz zone = LMGetTheZone();
		RNG_RandomUpdate( &zone, sizeof(zone));
	}
	
/* Screen */
	{
		GDHandle h = LMGetMainDevice();		/* GDHandle is **GDevice */
		RNG_RandomUpdate( *h, sizeof(GDevice));
	}
/* Scrap size */
	{
		SInt32 scrapSize = LMGetScrapSize();
		RNG_RandomUpdate( &scrapSize, sizeof(scrapSize));
	}
/* Scrap count */
	{
		SInt16 scrapCount = LMGetScrapCount();
		RNG_RandomUpdate( &scrapCount, sizeof(scrapCount));
	}
/*  File stuff, last modified, etc. */
	{
		HParamBlockRec			pb;
		GetVolParmsInfoBuffer	volInfo;
		pb.ioParam.ioVRefNum = 0;
		pb.ioParam.ioNamePtr = nil;
		pb.ioParam.ioBuffer = (Ptr) &volInfo;
		pb.ioParam.ioReqCount = sizeof(volInfo);
		PBHGetVolParmsSync(&pb);
		RNG_RandomUpdate( &volInfo, sizeof(volInfo));
	}
/* Event queue */
	{
		EvQElPtr		eventQ;
		for (eventQ = (EvQElPtr) LMGetEventQueue()->qHead; 
				eventQ; 
				eventQ = (EvQElPtr)eventQ->qLink)
			RNG_RandomUpdate( &eventQ->evtQWhat, sizeof(EventRecord));
	}
	FE_ReadScreen();
	RNG_FileForRNG(NULL);
}

void FE_ReadScreen()
{
	UInt16				coords[4];
	PixMapHandle 		pmap;
	GDHandle			gh;	
	UInt16				screenHeight;
	UInt16				screenWidth;			/* just what they say */
	UInt32				bytesToRead;			/* number of bytes we're giving */
	UInt32				offset;					/* offset into the graphics buffer */
	UInt16				rowBytes;
	UInt32				rowsToRead;
	float				bytesPerPixel;			/* dependent on buffer depth */
	Ptr					p;						/* temporary */
	UInt16				x, y, w, h;

	gh = LMGetMainDevice();
	if ( !gh )
		return;
	pmap = (**gh).gdPMap;
	if ( !pmap )
		return;
		
	RNG_GenerateGlobalRandomBytes( coords, sizeof( coords ) );
	
	/* make x and y inside the screen rect */	
	screenHeight = (**pmap).bounds.bottom - (**pmap).bounds.top;
	screenWidth = (**pmap).bounds.right - (**pmap).bounds.left;
	x = coords[0] % screenWidth;
	y = coords[1] % screenHeight;
	w = ( coords[2] & 0x7F ) | 0x40;		/* Make sure that w is in the range 64..128 */
	h = ( coords[3] & 0x7F ) | 0x40;		/* same for h */

	bytesPerPixel = (**pmap).pixelSize / 8;
	rowBytes = (**pmap).rowBytes & 0x7FFF;

	/* starting address */
	offset = ( rowBytes * y ) + (UInt32)( (float)x * bytesPerPixel );
	
	/* don't read past the end of the pixmap's rowbytes */
	bytesToRead = MIN(	(UInt32)( w * bytesPerPixel ),
						(UInt32)( rowBytes - ( x * bytesPerPixel ) ) );

	/* don't read past the end of the graphics device pixmap */
	rowsToRead = MIN(	h, 
						( screenHeight - y ) );
	
	p = GetPixBaseAddr( pmap ) + offset;
	
	while ( rowsToRead-- )
	{
		RNG_RandomUpdate( p, bytesToRead );
		p += rowBytes;
	}
}
#endif
