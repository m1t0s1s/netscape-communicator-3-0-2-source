/* -*- Mode: C; tab-width: 4; -*- */
/*
 * @(#)appletStubs.c	1.2 95/06/14 Warren Harris
 *
 * Copyright (c) 1995 Netscape Communications Corporation. All Rights Reserved.
 *
 */

#include "java.h"
#include "xp.h"
#include "fe_proto.h"

#include "jri.h"
#include "java_lang_String.h"
#include "netscape_applet_Console.h"
#define IMPLEMENT_netscape_applet_MozillaAppletContext
#ifndef XP_MAC
#include "netscape_applet_MozillaAppletContext.h"
#else
#undef DEBUG /* because javah generates extern symbols that are too long */
#include "n_applet_MozillaAppletContext.h"
#endif

#include "prdump.h"
#include "prlog.h"
#include "prgc.h"

/******************************************************************************/

#if Quickfix
void LJ_FixURLHack(void *);
#endif

JRI_PUBLIC_API(void)
native_netscape_applet_MozillaAppletContext_pShowDocument(
    JRIEnv* env,
    struct netscape_applet_MozillaAppletContext* this,
    struct java_lang_String* url,
    struct java_lang_String* addr,
    struct java_lang_String* target)
{
    char* urlString;
    char* addrString;
    char* targetName;
    MWContext* cx;

    /* Don't grab frameMWContext until we've entered the monitor */
    PR_EnterMonitor(mozilla_event_queue->monitor);

    cx = (MWContext*)get_netscape_applet_MozillaAppletContext_frameMWContext(env, this);
    if (cx == NULL) {
		/*
		** This MozillaAppletContext has been detached from it's C
		** context (the MWContext). Don't do anything!
		*/
		goto done;
    }

    PR_ASSERT(cx->funcs != 0);
    PR_ASSERT(cx->funcs->Progress != 0);

    if (url == NULL || target == NULL) {
		/* silently fail */
		goto done;
    }

    urlString = (char*)JRI_GetStringUTFChars(env, url);
	urlString = strdup(urlString);	/* we're going to munge it */
    /*
    ** This hack is to fix up the fact that java's toExternalForm message
    ** mangles our about urls to look like this: about:/foo. Bogus
    ** characters in the string get squeezed out, and the rest of the
    ** string slides down.
    */
#if Quickfix
    LJ_FixURLHack(urlString);
#endif

    targetName = (char*)JRI_GetStringUTFChars(env, target);
    addrString = addr ? (char*)JRI_GetStringUTFChars(env, addr) : NULL;

    /* Turn leading percents into spaces because percents are reserved: */
    while (*targetName == '%') *targetName++ = ' ';
    LJ_PostShowDocument(cx, urlString, addrString, targetName);

    free(urlString);
    /* addrString and targetName are garbage collected */
  done:
    PR_ExitMonitor(mozilla_event_queue->monitor);
}

/******************************************************************************/

JRI_PUBLIC_API(void)
native_netscape_applet_MozillaAppletContext_pShowStatus(
    JRIEnv* env,
    struct netscape_applet_MozillaAppletContext* this,
    struct java_lang_String* status)
{
    const char* str;
    MWContext* cx;

    /* Don't grab frameMWContext until we've entered the monitor */
    PR_EnterMonitor(mozilla_event_queue->monitor);

    cx = (MWContext*)get_netscape_applet_MozillaAppletContext_frameMWContext(env, this);
    if (cx == NULL) {
		/*
		** This MozillaAppletContext has been detached from it's C
		** context (the MWContext). Don't do anything!
		*/
		goto done;
    }

    PR_ASSERT(cx->funcs != 0);
    PR_ASSERT(cx->funcs->Progress != 0);

    if (status == NULL)
		str = "";	/* null status means erase status */
    else
		str = JRI_GetStringUTFChars(env, status);

    /*
    ** Post an event to the mozilla thread rather than do the operation
    ** on our thread directly.
    */
    LJ_PostShowStatus(cx, str);
    
  done:
    PR_ExitMonitor(mozilla_event_queue->monitor);
}



/******************************************************************************/

JRI_PUBLIC_API(void)
native_netscape_applet_MozillaAppletContext_pMochaOnLoad(
    JRIEnv* env,
    struct netscape_applet_MozillaAppletContext* this,
	jint status)
{
    MWContext* cx;

    /* Don't grab frameMWContext until we've entered the monitor */
    PR_EnterMonitor(mozilla_event_queue->monitor);

    cx = (MWContext*)get_netscape_applet_MozillaAppletContext_frameMWContext(env, this);
    if (cx == NULL) {
		/*
		** This MozillaAppletContext has been detached from it's C
		** context (the MWContext). Don't do anything!
		*/
		goto done;
    }

    PR_ASSERT(cx->funcs != 0);
    PR_ASSERT(cx->funcs->Progress != 0);

    /*
    ** Post an event to the mozilla thread rather than do the operation
    ** on our thread directly.
    */
    LJ_PostMochaOnLoad(cx,status);
    
  done:
    PR_ExitMonitor(mozilla_event_queue->monitor);
}

/******************************************************************************/

JRI_PUBLIC_API(void)
native_netscape_applet_MozillaAppletContext_setConsoleState0(
    JRIEnv* env, 
	struct java_lang_Class* clazz,
    jint status)
{
    PR_EnterMonitor(mozilla_event_queue->monitor);
    LJ_PostSetConsoleState(status);
    PR_ExitMonitor(mozilla_event_queue->monitor);
}

/******************************************************************************/

/*** private static native setSystemIO (Ljava/io/InputStream;Ljava/io/PrintStream;Ljava/io/PrintStream;)V ***/
JRI_PUBLIC_API(void)
native_netscape_applet_Console_setSystemIO(JRIEnv                     *env, 
                                           struct java_lang_Class     *clazz, 
                                           struct java_io_InputStream *in, 
                                           struct java_io_PrintStream *out, 
                                           struct java_io_PrintStream *err)
{
    /* Forward call to native method in System.java */
    /* A prototype from #include "java_lang_System.h" is desirable...
       ...but a broken build which results from a mistake in a makefile
       that would be needed to get this #include is very bad.
       Bottom line: XXX need to fix when akbar is out the door */
    struct Hjava_lang_System;
    struct Hjava_io_InputStream;
    struct Hjava_io_PrintStream;
    extern void java_lang_System_setSystemIO(struct Hjava_lang_System*,
					     struct Hjava_io_InputStream *,
					     struct Hjava_io_PrintStream *,
					     struct Hjava_io_PrintStream *);
    
    java_lang_System_setSystemIO(0, 
    		(struct Hjava_io_InputStream *)in, 
    		(struct Hjava_io_PrintStream *)out, 
    		(struct Hjava_io_PrintStream *)err); 
}

JRI_PUBLIC_API(void)
native_netscape_applet_Console_dumpMemory(
    JRIEnv* env,
    struct java_lang_Class* clazz, jbool detailed)
{
    PR_DumpMemory(detailed);
}

JRI_PUBLIC_API(void)
native_netscape_applet_Console_dumpMemorySummary(
    JRIEnv* env,
    struct java_lang_Class* clazz)
{
    PR_DumpMemorySummary();
}

JRI_PUBLIC_API(void)
native_netscape_applet_Console_dumpApplicationHeaps(JRIEnv* env)
{
    PR_DumpApplicationHeaps();
}

JRI_PUBLIC_API(void)
native_netscape_applet_Console_dumpNSPRInfo(
    JRIEnv* env,
    struct java_lang_Class* clazz)
{
    PR_DumpStuffToFile();
}

#if defined(DEBUG) && defined(XP_WIN32) && defined(MSWINDBGMALLOC)
#include "xp_mem.h"
#define NUM_MEMSTATES	10
_CrtMemState memState[NUM_MEMSTATES];
long memStateCnt = 0;
#endif

JRI_PUBLIC_API(void)
native_netscape_applet_Console_checkpointMemory(
    JRIEnv* env,
    struct java_lang_Class* clazz)
{
#if defined(DEBUG) && defined(XP_WIN32) && defined(MSWINDBGMALLOC) 

	HFILE reportFile;
	OFSTRUCT foo;
	foo.cBytes = sizeof(OFSTRUCT);
	reportFile = OpenFile("mem.out", &foo, OF_CREATE|OF_WRITE);
	_CrtSetReportMode( _CRT_WARN, _CRTDBG_MODE_FILE );
	_CrtSetReportFile( _CRT_WARN, (_HFILE)reportFile );
	_CrtSetReportMode( _CRT_ERROR, _CRTDBG_MODE_FILE );
	_CrtSetReportFile( _CRT_ERROR, (_HFILE)reportFile );
	_CrtSetReportMode( _CRT_ASSERT, _CRTDBG_MODE_FILE );
	_CrtSetReportFile( _CRT_ASSERT, (_HFILE)reportFile );
	
	if (memStateCnt < NUM_MEMSTATES) {
		_CrtMemState* state = &memState[memStateCnt++];
		_CrtMemCheckpoint(state);
		if (memStateCnt > 1) {
			_CrtMemState diffs;
			_RPT0(_CRT_WARN, "--memory-statistics---------------------------------------\n");
			if (_CrtMemDifference(&diffs, state-1, state)) {
				_CrtMemDumpStatistics(&diffs);
			}
			_CrtMemDumpAllObjectsSince(state-1);
#if 0
			_CrtDumpMemoryLeaks();
#endif
			_CrtCheckMemory();
		}
	}
	else {
		_RPT0(_CRT_WARN, "Can't checkpoint anymore.");
	}

	_lclose(reportFile);
	_CrtSetReportMode( _CRT_WARN, _CRTDBG_MODE_DEBUG );
	_CrtSetReportMode( _CRT_ERROR, _CRTDBG_MODE_DEBUG );
	_CrtSetReportMode( _CRT_ASSERT, _CRTDBG_MODE_DEBUG );

#endif
}

/*******************************************************************************
 * AppletClassLoader Methods
 ******************************************************************************/

#ifndef XP_MAC
#include "netscape_applet_AppletClassLoader.h"
#else
#undef DEBUG /* because javah generates extern symbols that are too long */
#include "n_applet_AppletClassLoader.h"
#endif
#include "zip.h"
#include "oobj.h"
#include "interpreter.h"

/*
** For Windows and Mac we open and close the zip file each time to avoid having too
** many open file descriptors. We also do this because if we left the zip files
** opened, when java shuts down it doesn't finalize everything. Therefore they're
** still opened when we quit the browser, which prevents us from deleting the files.
*/
#if defined(XP_WIN) || defined(XP_MAC)
#define OPEN_ZIP_EVERY_TIME
#endif

/* See the ugly disgusting xp_file hack below: */
PUBLIC char * WH_TempName(XP_FileType type, const char * prefix);
PUBLIC char * WH_FileName (const char *name, XP_FileType type);

static zip_t*
openZip(JRIEnv* env, const char* zipName)
{
	zip_t* zip;
#ifdef XP_MAC
	/*
	** The Mac xp_file routines are all screwed up. XP_FileName
	** returns a colon-separated pathname, but zip_open wants a
	** unix-style name.
	*/
	OSErr ConvertMacPathToUnixPath(const char *macPath, char **unixPath);
	const char* unixPath = NULL;
	OSErr err = ConvertMacPathToUnixPath(zipName, &unixPath);
	if (err) {
		if (unixPath) XP_FREE(unixPath);
		return NULL;
	}
#else
	const char* unixPath = zipName;
#endif
	zip = zip_open(unixPath);
	if (zip == NULL) {
		JRI_ThrowNew(env, JRI_FindClass(env, "java/io/IOException"), 
					 "bad zip file format");
	}
#ifdef XP_MAC
	XP_FREE(unixPath);
#endif
	return zip;
}

/*
** We're cheating here and calling URLInputStream.read because we know
** that the stream handed to the openZipFile will always be a
** URLInputStream (because we wrote the URLStreamHandler and
** AppletClassLoader classes.
*/
struct Hnetscape_net_URLInputStream;
extern long
netscape_net_URLInputStream_read(struct Hnetscape_net_URLInputStream* this,
				 HArrayOfByte* bytes, long offset, long length);

JRI_PUBLIC_API(jint)
native_netscape_applet_AppletClassLoader_openZipFile(
	JRIEnv* env,
	struct netscape_applet_AppletClassLoader* self,
	struct java_io_InputStream* in)
{
	#define BUF_LEN		4096
	const char* tempName;
	XP_File file;
	char* zipName;
	jref byteArray;
	jbyte* buf;
	int n;
	
	tempName = WH_TempName(xpTemporary, DOWNLOADABLE_ZIPFILE_PREFIX);
	if (tempName == NULL) return 0;
	file = XP_FileOpen(tempName, xpTemporary, XP_FILE_WRITE_BIN);
	byteArray = JRI_NewByteArray(env, BUF_LEN, NULL);
	buf = JRI_GetByteArrayElements(env, byteArray);

	/* Write all the data from the input stream to the temp file: */
/*	while ((n = java_io_InputStream_read_2(env, in, byteArray, 0, BUF_LEN)) > 0) {
 */	while ((n = netscape_net_URLInputStream_read(
					(struct Hnetscape_net_URLInputStream*)in,
					(HArrayOfByte*)byteArray, 0, BUF_LEN)) > 0) {
		int status = XP_FileWrite(buf, n, file);
		if (status <= 0) {
			XP_FileClose(file);
			return 0;
		}
	}
	XP_FileClose(file);
	zipName = WH_FileName(tempName, xpTemporary);
	/*XP_Trace("openZipFile: temp=%s, path=%s\n", tempName, zipName);*/
	XP_FREE((void*)tempName);

#ifdef OPEN_ZIP_EVERY_TIME
	return (jint)zipName;
#else  /* !OPEN_ZIP_EVERY_TIME */
	/* Now open the temp file as a zip file: */
	{
		zip_t* zip = openZip(env, zipName);
		XP_FREE(zipName);
		return (jint)zip;
	}
#endif /* !OPEN_ZIP_EVERY_TIME */
}

JRI_PUBLIC_API(jbyteArray)
native_netscape_applet_AppletClassLoader_loadFromZipFile(
	JRIEnv* env,
	struct netscape_applet_AppletClassLoader* self,
	jint zipPtr, struct java_lang_String *classname) 
{
	zip_t* zip;
	const char* fn = JRI_GetStringUTFChars(env, classname);
	jbyteArray result;
    struct stat st;
	char* classData;

#ifdef OPEN_ZIP_EVERY_TIME
	/* Now open the temp file as a zip file: */
	zip = openZip(env, (const char*)zipPtr);
	if (zip == NULL) return NULL;
#else  /* !OPEN_ZIP_EVERY_TIME */
	zip = (zip_t*)zipPtr;
#endif /* !OPEN_ZIP_EVERY_TIME */

    if (!zip_stat(zip, fn, &st)) {
		return NULL;
    }
    if ((classData = malloc((size_t)st.st_size)) == 0) {
		return NULL;
    }
    if (!zip_get(zip, fn, classData, st.st_size)) {
		XP_FREE(classData);
		return NULL;
    }
/*
	result = (struct java_lang_Class*)JRI_DefineClass(env,
				 (struct java_lang_ClassLoader*)self, classData, st.st_size);
*/
	result = JRI_NewByteArray(env, st.st_size, classData);
	XP_FREE(classData);

#ifdef OPEN_ZIP_EVERY_TIME
	zip_close(zip);	
#endif /* OPEN_ZIP_EVERY_TIME */

	return result;
}

JRI_PUBLIC_API(void)
native_netscape_applet_AppletClassLoader_closeZipFile(
	JRIEnv* env,
	struct netscape_applet_AppletClassLoader* self,
	jint zipPtr)
{
#ifdef OPEN_ZIP_EVERY_TIME
	const char* zipName = (const char*)zipPtr;
#else /* !OPEN_ZIP_EVERY_TIME */
	zip_t* zip = (zip_t*)zipPtr;
	const char* zipName = XP_STRDUP(zip->fn);
	if (zip != NULL)
		zip_close(zip);	
#endif /* !OPEN_ZIP_EVERY_TIME */
	XP_FileRemove(zipName, xpTemporary);
	XP_FREE((void*)zipName);
}

/******************************************************************************/
