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
#include "secrng.h"
#include "xp_core.h"
#ifdef XP_WIN
#include <windows.h>
#include <time.h>
#include <io.h>
#include <sys/types.h>
#include <sys/stat.h>

#ifndef _WIN32
#define VTD_Device_ID   5
#define OP_OVERRIDE     _asm _emit 0x66
#include <dos.h>
#endif

static BOOL
CurrentClockTickTime(LPDWORD lpdwHigh, LPDWORD lpdwLow)
{
#ifdef _WIN32
    LARGE_INTEGER   liCount;

    if (!QueryPerformanceCounter(&liCount))
        return FALSE;

    *lpdwHigh = liCount.u.HighPart;
    *lpdwLow = liCount.u.LowPart;
    return TRUE;

#else
    BOOL    bRetVal;
    FARPROC lpAPI;
    WORD    w1, w2, w3, w4;
    
    // Get direct access to the VTD and query the current clock tick time   
    _asm {
        xor   di, di
        mov   es, di
        mov   ax, 1684h
        mov   bx, VTD_Device_ID
        int   2fh
        mov   ax, es
        or    ax, di
        jz    EnumerateFailed

        ; VTD API is available. First store the API address (the address actually
        ; contains an instruction that causes a fault, the fault handler then
        ; makes the ring transition and calls the API in the VxD)
        mov   word ptr lpAPI, di
        mov   word ptr lpAPI+2, es
        mov   ax, 100h      ; API function to VTD_Get_Real_Time
        call  dword ptr lpAPI

        ; Result is in EDX:EAX which we will get 16-bits at a time
        mov   w2, dx
        OP_OVERRIDE
        shr   dx,10h        ; really "shr edx, 16"
        mov   w1, dx
        
        mov   w4, ax
        OP_OVERRIDE
        shr   ax,10h        ; really "shr eax, 16"
        mov   w3, ax
        
        mov   bRetVal, 1    ; return TRUE
        jmp   EnumerateExit

      EnumerateFailed:
        mov   bRetVal, 0    ; return FALSE

      EnumerateExit:
    }

    *lpdwHigh = MAKELONG(w2, w1);
    *lpdwLow = MAKELONG(w4, w3);
    
    return bRetVal;
#endif
}

size_t RNG_GetNoise(void *buf, size_t maxbuf)
{
	DWORD	dwHigh, dwLow, dwVal;
	int		n = 0;
	int		nBytes;
    time_t 	sTime;

	if (maxbuf <= 0)
		return 0;

	CurrentClockTickTime(&dwHigh, &dwLow);

	// get the maximally changing bits first
	nBytes = sizeof(dwLow) > maxbuf ? maxbuf : sizeof(dwLow);
	memcpy((char *)buf, &dwLow, nBytes);
	n += nBytes;
	maxbuf -= nBytes;

	if (maxbuf <= 0)
		return n;

	nBytes = sizeof(dwHigh) > maxbuf ? maxbuf : sizeof(dwHigh);
	memcpy(((char *)buf) + n, &dwHigh, nBytes);
	n += nBytes;
	maxbuf -= nBytes;

	if (maxbuf <= 0)
		return n;

	// get the number of milliseconds that have elapsed since Windows started
	dwVal = GetTickCount();

	nBytes = sizeof(dwVal) > maxbuf ? maxbuf : sizeof(dwVal);
	memcpy(((char *)buf) + n, &dwVal, nBytes);
	n += nBytes;
	maxbuf -= nBytes;

	if (maxbuf <= 0)
		return n;

	// get the time in seconds since midnight Jan 1, 1970
    time(&sTime);
	nBytes = sizeof(sTime) > maxbuf ? maxbuf : sizeof(sTime);
	memcpy(((char *)buf) + n, &sTime, nBytes);
	n += nBytes;

	return n;
}

static BOOL
EnumSystemFiles(void (*func)(char *))
{
	int					iStatus;
	char				szSysDir[_MAX_PATH];
	char				szFileName[_MAX_PATH];
#ifdef _WIN32
    struct _finddata_t 	fdData;
    long 				lFindHandle;
#else
    struct _find_t 	fdData;
#endif

    if (!GetSystemDirectory(szSysDir, sizeof(szSysDir)))
		return FALSE;

    // tack *.* on the end so we actually look for files. this will
	// not overflow
	strcpy(szFileName, szSysDir);
	strcat(szFileName, "\\*.*");

#ifdef _WIN32
    lFindHandle = _findfirst(szFileName, &fdData);
    if (lFindHandle == -1)
        return FALSE;
#else
	if (_dos_findfirst(szFileName, _A_NORMAL | _A_RDONLY | _A_ARCH | _A_SUBDIR, &fdData) != 0)
		return FALSE;
#endif

    do {
		// pass the full pathname to the callback
        sprintf(szFileName, "%s\\%s", szSysDir, fdData.name);
		(*func)(szFileName);

#ifdef _WIN32
        iStatus = _findnext(lFindHandle, &fdData);
#else
        iStatus = _dos_findnext(&fdData);
#endif
    } while (iStatus == 0);

#ifdef _WIN32
	_findclose(lFindHandle);
#endif

	return TRUE;
}

static DWORD	dwNumFiles, dwReadEvery;

static void
CountFiles(char *file)
{
	dwNumFiles++;	
}

static void
ReadFiles(char *file)
{
	if ((dwNumFiles % dwReadEvery) == 0)
		RNG_FileForRNG(file);

	dwNumFiles++;
}

static void
ReadSystemFiles()
{
	// first count the number of files
	dwNumFiles = 0;
	if (!EnumSystemFiles(CountFiles))
		return;

	RNG_RandomUpdate(&dwNumFiles, sizeof(dwNumFiles));

	// now read 10 files
	if (dwNumFiles == 0)
		return;

	dwReadEvery = dwNumFiles / 10;
	if (dwReadEvery == 0)
		dwReadEvery = 1;  // less than 10 files

	dwNumFiles = 0;
	EnumSystemFiles(ReadFiles);
}

void RNG_SystemInfoForRNG(void)
{
    DWORD  			dwVal;
    POINT  			ptVal;
    HWND   			hWnd;
    char   			buffer[256];
	int				nBytes;
#ifdef _WIN32
    MEMORYSTATUS 	sMem;                  
    UUID 			sUuid;
    DWORD 			dwSerialNum;
    DWORD 			dwComponentLen;
    DWORD 			dwSysFlags;
	char			volName[128];
    DWORD 			dwSectors, dwBytes, dwFreeClusters, dwNumClusters;
    HANDLE 			hVal;
#else                        
	int				iVal;
	HTASK			hTask;
	WORD			wDS, wCS;
    LPSTR 			lpszEnv;
#endif

	nBytes = RNG_GetNoise(buffer, 20);  // get up to 20 bytes
	RNG_RandomUpdate(buffer, nBytes);

    GetCursorPos(&ptVal);
    RNG_RandomUpdate(&ptVal, sizeof(ptVal));

#ifdef _WIN32
    sMem.dwLength = sizeof(sMem);
    GlobalMemoryStatus(&sMem);                // assorted memory stats
    RNG_RandomUpdate(&sMem, sizeof(sMem));

    dwVal = GetLogicalDrives();
    RNG_RandomUpdate(&dwVal, sizeof(dwVal));  // bitfields in bits 0-25

#else
    dwVal = GetFreeSpace(0);
    RNG_RandomUpdate(&dwVal, sizeof(dwVal));

	_asm	mov wDS, ds;
	_asm	mov wCS, cs;
    RNG_RandomUpdate(&wDS, sizeof(wDS));
    RNG_RandomUpdate(&wCS, sizeof(wCS));
#endif

    dwVal = GetQueueStatus(QS_ALLINPUT);      // high and low significant
    RNG_RandomUpdate(&dwVal, sizeof(dwVal));

    hWnd = GetClipboardOwner();               // 2 or 4 bytes
    RNG_RandomUpdate((void *)&hWnd, sizeof(hWnd));

#ifdef _WIN32
    dwVal = sizeof(buffer);
    if (GetComputerName(buffer, &dwVal))
        RNG_RandomUpdate(buffer, dwVal);

    UuidCreate(&sUuid);                       // this will fail on machines with no ethernet
    RNG_RandomUpdate(&sUuid, sizeof(sUuid));  // boards. shove the bits in regardless

    hVal = GetCurrentProcess();               // 4 byte handle of current task
    RNG_RandomUpdate(&hVal, sizeof(hVal));

    dwVal = GetCurrentProcessId();            // process ID (4 bytes)
    RNG_RandomUpdate(&dwVal, sizeof(dwVal));

	volName[0] = '\0';
	buffer[0] = '\0';
    GetVolumeInformation(NULL,
                         volName,
                         sizeof(volName),
                         &dwSerialNum,
                         &dwComponentLen,
                         &dwSysFlags,
                         buffer,
                         sizeof(buffer));

    RNG_RandomUpdate(volName, strlen(volName));
    RNG_RandomUpdate(&dwSerialNum, sizeof(dwSerialNum));
    RNG_RandomUpdate(&dwComponentLen, sizeof(dwComponentLen));
    RNG_RandomUpdate(&dwSysFlags, sizeof(dwSysFlags));
    RNG_RandomUpdate(buffer, strlen(buffer));

    if (GetDiskFreeSpace(NULL, &dwSectors, &dwBytes, &dwFreeClusters, &dwNumClusters)) {
        RNG_RandomUpdate(&dwSectors, sizeof(dwSectors));
        RNG_RandomUpdate(&dwBytes, sizeof(dwBytes));
        RNG_RandomUpdate(&dwFreeClusters, sizeof(dwFreeClusters));
        RNG_RandomUpdate(&dwNumClusters, sizeof(dwNumClusters));
    }

#else
    hTask = GetCurrentTask();
    RNG_RandomUpdate((void *)&hTask, sizeof(hTask));

    iVal = GetNumTasks();
    RNG_RandomUpdate(&iVal, sizeof(iVal));      // number of running tasks

    lpszEnv = GetDOSEnvironment();
	while (*lpszEnv != '\0') {
        RNG_RandomUpdate(lpszEnv, strlen(lpszEnv));

		lpszEnv += strlen(lpszEnv) + 1;
	}
#endif

    // now let's do some files
	ReadSystemFiles();

	nBytes = RNG_GetNoise(buffer, 20);  // get up to 20 bytes
	RNG_RandomUpdate(buffer, nBytes);
}

void RNG_FileForRNG(char *filename)
{
    struct stat 	stat_buf;
    unsigned char 	buffer[1024];
    FILE*			file;
	int				nBytes;
    static DWORD 	totalFileBytes = 0;
    
    if (stat((char *)filename, &stat_buf) < 0)
		return;

    RNG_RandomUpdate((unsigned char*)&stat_buf, sizeof(stat_buf));
    
    file = fopen((char *)filename, "r");
    if (file != NULL) {
		for (;;) {
			size_t	bytes = fread(buffer, 1, sizeof(buffer), file);

			if (bytes == 0)
				break;

			RNG_RandomUpdate(buffer, bytes);
			totalFileBytes += bytes;
			if (totalFileBytes > 250000)
				break;
		}

		fclose(file);
    }

	nBytes = RNG_GetNoise(buffer, 20);  // get up to 20 bytes
	RNG_RandomUpdate(buffer, nBytes);
}

#endif
