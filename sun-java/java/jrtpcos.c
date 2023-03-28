/*
 * @(#)jrtpcos.c	1.2 95/06/14 Dennis Tabuena
 *
 * Copyright (c) 1996 Netscape Communications Corporation. All Rights Reserved.
 *
 */
#if defined(XP_PC) && !defined(_WIN32)
#include "windows.h"
#include "jri.h"
#include "jrtpcos.h"

static void (*clientOutput)(const char *s);

static void
LogString(const char *str)
{
	static FILE *logFile = NULL;
	static char *logName = NULL;

	if (logFile == NULL) {
		if (logName==NULL) {
			logName = getenv("LOGPATH");
			if (logName==NULL)
				logName= "c:\\java.log";
		}
		logFile = fopen(logName,"w");
	}

	if (logFile != NULL) {
		fwrite(str,1,strlen(str),logFile);
		fflush(logFile);
	}

	if (clientOutput != NULL)
		clientOutput(str);
}


void* JRI_CALLBACK
JRTPCOS_malloc (size_t size)
{
	return malloc(size);
}

void* JRI_CALLBACK
JRTPCOS_realloc(void *old, size_t newsize)
{
	return realloc(old, newsize);
}


void*JRI_CALLBACK 
JRTPCOS_calloc(size_t count, size_t size)
{
	return calloc(count, size);
}


void JRI_CALLBACK 
JRTPCOS_free(void *item)
{
	free(item);
}


int JRI_CALLBACK 
JRTPCOS_gethostname(char * name, int namelen)
{
	return gethostname(name, namelen);
}


struct hostent * JRI_CALLBACK 
JRTPCOS_gethostbyname(const char * name)
{
	return gethostbyname(name);
}


struct hostent * JRI_CALLBACK
JRTPCOS_gethostbyaddr(const char * addr, int len, int type)
{
	return gethostbyaddr( addr, len, type);
}

char* JRI_CALLBACK
JRTPCOS_getenv(const char *varname)
{
	return getenv(varname);
}

int JRI_CALLBACK
JRTPCOS_putenv(const char *varname)
{
	return putenv(varname);
}

int JRI_CALLBACK
JRTPCOS_auxOutput(const char *string)
{
	LogString(string);
	return 0;
}

static  CATCHBUF	JRTPCOS_exit_cb;
static	int			JRTPCOS_exit_code;

void JRI_CALLBACK
JRTPCOS_exit(int exitCode)
{
	JRTPCOS_exit_code = exitCode;
	Throw( JRTPCOS_exit_cb, -1 );
}

static struct PRMethodCallbackStr JRTPCOS_MethodCallbackStr =
{
	JRTPCOS_malloc,
	JRTPCOS_realloc,
	JRTPCOS_calloc,
	JRTPCOS_free,
	JRTPCOS_gethostname,
	JRTPCOS_gethostbyname,
	JRTPCOS_gethostbyaddr,
	JRTPCOS_getenv,
	JRTPCOS_putenv,
	JRTPCOS_auxOutput,
	JRTPCOS_exit
};

typedef void (FAR *PR_MDInitFn)( struct PRMethodCallbackStr *x);

/*
 * Bring down any existing copies of the library first, before loading
 */
HINSTANCE
LoadFreshLibrary(const char *libName)
{
	HINSTANCE	inst;
	int			useCount;
	
	
	inst = LoadLibrary(libName);
	if (inst == NULL)
		return inst; /* Failed, won't load at all */
	
	useCount = GetModuleUsage(inst); /* How many users? */
	if (useCount == 1)
		return inst;
		
	while (useCount > 0) {
		FreeLibrary(inst);
		if (--useCount > 0)
			useCount = GetModuleUsage(inst); /* How many users? */
	}
	
	return LoadLibrary(libName);
	
}

void
JRTPC_SetClientOutputCallback( void (*outputCallback)(const char *) )
{
	clientOutput = outputCallback;
}

HINSTANCE
JRTPC_Init(struct PRMethodCallbackStr *cbStruct)
{
	HINSTANCE	libInst;
	PR_MDInitFn pf;

	if (cbStruct == NULL)
		cbStruct = &JRTPCOS_MethodCallbackStr;
	libInst = LoadFreshLibrary("PR162100.DLL"); /* HACK */
	if (libInst != NULL) {
		pf = (PR_MDInitFn) GetProcAddress(libInst, "_PR_MDInit");
		if (pf != NULL)
			(* pf)(cbStruct);
	}

	return libInst;
}

typedef int (FAR *JRT_StartJavaFn)(int argc, char *(argv[]));

int
JRTPC_StartJava(int argc, char *(argv[]))
{
	HINSTANCE	libInst;
	JRT_StartJavaFn sjf;
	int			startJavaCode;

	startJavaCode = 0;
    if (Catch((int FAR*) JRTPCOS_exit_cb) == 0) {
		libInst = LoadFreshLibrary("JRT1621.DLL"); /* HACK */
		if (libInst != NULL) {
			sjf = (JRT_StartJavaFn) GetProcAddress(libInst, "_start_java");
			if (sjf != NULL)
				startJavaCode = (* sjf)(argc, argv);
		}
	} else 
		startJavaCode = JRTPCOS_exit_code;

	if (libInst != NULL) {
		FreeLibrary(libInst);
		FreeLibrary(libInst);
	}

	return startJavaCode;
}

#endif

