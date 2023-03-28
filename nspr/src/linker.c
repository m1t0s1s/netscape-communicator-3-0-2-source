#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "prmem.h"
#include "prmon.h"
#include "prlog.h"
#include "prlink.h"
#include "prprf.h"
#ifdef SW_THREADS
#include "swkern.h"
#else
#include "hwkern.h"
#endif

#ifdef XP_MAC
#include <ConditionalMacros.h>
#include <CodeFragments.h>
#include <TextUtils.h>
#include <Types.h>
#include <Strings.h>
#include "prmacos.h"
#endif

#ifdef XP_UNIX
#ifdef USE_DLFCN
#include <dlfcn.h>
#elif defined(USE_HPSHL)
#include <dl.h>
#endif


#undef HAVE_DLL		/* jwz -- kludge */




/* Define this on systems which don't have it (AIX) */
#ifndef RTLD_LAZY
#define RTLD_LAZY 1
#endif

#endif /* XP_UNIX */

#ifdef XP_PC
typedef PRStaticLinkTable *NODL_PROC(void);
#endif


struct PRLibraryStr {
    const char*			name;
    PRLibrary*			next;
    int				refCount;
#ifdef XP_PC
    HINSTANCE			dlh;
#endif
#ifdef XP_MAC
    CFragConnectionID		dlh;
#endif

#ifdef XP_UNIX
#if defined(USE_HPSHL)
    shl_t			dlh;
#else
    void*			dlh;
#endif 
#endif 
    const PRStaticLinkTable*	staticTable;
};

PR_LOG_DEFINE(Linker);

static PRLibrary *pr_loadmap;
static PRLibrary *pr_exe_loadmap;
static PRMonitor *pr_linker_lock;
static char* pr_current_lib_path = NULL;
static char needInit = 1;

/************************************************************************/

#if ( !defined(USE_DLFCN) && !defined(HAVE_STRERROR) ) || defined(HPUX) || defined(BSDI) || defined(LINUX)
static char* errStrBuf = NULL;
#define ERR_STR_BUF_LENGTH	20
static char*
errno_string()
{
    if (errStrBuf == NULL) {
	errStrBuf = malloc(ERR_STR_BUF_LENGTH);
	if (errStrBuf == NULL) return NULL;
    }
    PR_snprintf(errStrBuf, ERR_STR_BUF_LENGTH, "error %d", errno);
    return errStrBuf;
}
#endif

#ifdef HAVE_STRERROR
#define STRERROR(err)	strerror(err)
#else
#define STRERROR(err)	errno_string()
#endif

static char*
pr_DLLError(void)
{
    /*
    ** We have to be in the pr_linker_lock in this routine because
    ** pr_DLLError is not thread-safe.
    */
    PR_ASSERT(PR_InMonitor(pr_linker_lock));

#ifdef HAVE_DLL
#   ifdef USE_DLFCN
        return dlerror();
#   elif defined(USE_HPSHL)
	return STRERROR(errno);
#   else
	return STRERROR(errno);
#   endif
#elif (defined(XP_MAC) && !GENERATINGCFM)
    return NULL;
#else
    return STRERROR(errno);
#endif /* HAVE_DLL */
}

static void
InitLinker(void)
{
    PRLibrary *lm;
#if defined(XP_UNIX)
    void *h;
#endif

    if (!pr_linker_lock) {
        pr_linker_lock = PR_NewNamedMonitor(1, "linker-lock");
    } else
        PR_EnterMonitor(pr_linker_lock);

    if (!pr_loadmap) {
#if defined(XP_PC)
        lm = (PRLibrary*) calloc(1, sizeof(PRLibrary));
        lm->name = "Executable";

        /* 
        ** In WIN32, GetProcAddress(...) expects a module handle in order to
        ** get exported symbols from the executable...
        **
        ** However, in WIN16 this is accomplished by passing NULL to 
        ** GetProcAddress(...)
        */
#if defined(_WIN32)
        lm->dlh = GetModuleHandle(NULL);
#else
        lm->dlh = (HINSTANCE)NULL;
#endif /* ! _WIN32 */

	lm->refCount    = 1;
	lm->staticTable = NULL;
	pr_exe_loadmap  = lm;
        pr_loadmap      = lm;

#elif defined(XP_MAC)
	#pragma unused lm
	#pragma unused h
	
	//  DebugStr((StringPtr)"\pwrite me");
	//  FIX ME

#elif defined(XP_UNIX)
#ifdef HAVE_DLL
#ifdef USE_DLFCN
	h = dlopen(0, RTLD_LAZY);
	if (!h) {
	    fprintf(stderr, "failed to initialize shared libraries [%s]\n",
		    pr_DLLError());
	    abort();/* XXX */
	}
#elif defined(USE_HPSHL)
#if 0	/* XXXrobm HP man pages don't say you can send a NULL path */
        h = shl_load(NULL, BIND_DEFERRED, NULL);
#else
	h = NULL;
#endif
	/* don't abort with this NULL */
#else
#error no dll strategy
#endif /* USE_DLFCN */
	lm = (PRLibrary*) calloc(1, sizeof(PRLibrary));
	lm->name = "a.out";
	lm->refCount = 1;
	lm->dlh = h;
	lm->staticTable = NULL;
	pr_exe_loadmap = lm;
	pr_loadmap = lm;
#endif /* HAVE_DLL */
#endif /* XP_UNIX */
    }
    needInit = 0;
    PR_LOG(Linker, debug, ("Loaded library %s (init)", lm->name));
    PR_ExitMonitor(pr_linker_lock);
}

void _PR_ShutdownLinker(void)
{
    if (needInit)
        return;

    PR_EnterMonitor(pr_linker_lock);

    while (pr_loadmap) {
        PR_UnloadLibrary(pr_loadmap);
    }
    
    PR_ExitMonitor(pr_linker_lock);

    PR_DestroyMonitor(pr_linker_lock);
    pr_linker_lock = NULL;
}

/******************************************************************************/

PR_PUBLIC_API(void)
PR_SetLibraryPath(const char *path)
{
    pr_current_lib_path = (char*)path;
}

/*
** Return the library path for finding shared libraries.
*/
PR_PUBLIC_API(const char *)
PR_GetLibraryPath(void)
{
    char *ev;

    if (pr_current_lib_path != NULL)
	return pr_current_lib_path;

    /* else initialize pr_current_lib_path: */

#ifdef XP_PC
    ev = getenv("LD_LIBRARY_PATH");
    if (!ev) {
	ev = ".;\\lib";
    }
#endif

#ifdef XP_MAC
    ev = "";
#endif

#ifdef XP_UNIX
#if defined USE_DLFCN
    {
	char *home;
	char *local;
	char *p;
	int len;

	ev = getenv("LD_LIBRARY_PATH");
	if (!ev) {
	    ev = "/usr/lib:/lib";
	}
	home = getenv("HOME");

	/*
	** Augment the path automatically by adding in ~/.netscape and
	** /usr/local/netscape
	*/
	len = strlen(ev) + 1;		/* +1 for the null */
	if (home && home[0]) {
	    len += strlen(home) + 1;	/* +1 for the colon */
	}

	local = ":/usr/local/netscape/lib/" PR_LINKER_ARCH;
	len += strlen(local);		/* already got the : */
	p = (char*) malloc(len+50);
	strcpy(p, ev);
	if (home) {
	    strcat(p, ":");
	    strcat(p, home);
	}
	strcat(p, local);
	ev = p;
	PR_LOG(IO, warn, ("linker path '%s'", ev));
    }
#else
    /* AFAIK there isn't a library path with the HP SHL interface --Rob */
    ev = "";
#endif
#endif

    pr_current_lib_path = ev;
    return ev;
}

/*
** Build library name from path, lib and extensions
*/
PR_PUBLIC_API(char*)
PR_GetLibName(const char *path, const char *lib)
{
    char *fullname;

#ifdef XP_PC
    fullname = PR_smprintf("%s\\%s.dll", path, lib);
#endif
#ifdef XP_MAC
    fullname = PR_smprintf("%s%s", path, lib);
#endif
#ifdef XP_UNIX
    fullname = PR_smprintf("%s/lib%s.so", path, lib);
#endif
    return fullname;
}

static PRLibrary*
pr_UnlockedFindLibrary(const char *name)
{
    PRLibrary* lm = pr_loadmap;
    const char* np = strrchr(name, DIRECTORY_SEPARATOR);
    np = np ? np + 1 : name;
    while (lm) {
	const char* cp = strrchr(lm->name, DIRECTORY_SEPARATOR);
	cp = cp ? cp + 1 : lm->name;
#ifdef XP_PC
        /* Windows DLL names are case insensitive... */
	if (strcmpi(np, cp) == 0) {
#else
	if (strcmp(np, cp)  == 0) {
#endif
	    /* found */
	    lm->refCount++;
	    PR_LOG(Linker, debug,
		   ("%s incr => %d (find lib)",
		    lm->name, lm->refCount));
	    return lm;
	}
	lm = lm->next;
    }
    return NULL;
}

/*
** Dynamically load a library. Only load libraries once, so scan the load
** map first.
*/
PR_PUBLIC_API(PRLibrary*)
PR_LoadLibrary(const char *name)
{
    PRLibrary *lm;
    PRLibrary* result;

    /* Initialize the linker if necessary */
    if (needInit) InitLinker();

    /* See if library is already loaded */
    PR_EnterMonitor(pr_linker_lock);

    result = pr_UnlockedFindLibrary(name);
    if (result != NULL) goto unlock;

    lm = (PRLibrary*) calloc(1, sizeof(PRLibrary));
    if (lm == NULL) goto unlock;
    lm->staticTable = NULL;

#ifdef XP_PC
    {
        NODL_PROC *pfn;
	HINSTANCE h;

	h = LoadLibrary(name);
	if (h < (HINSTANCE)HINSTANCE_ERROR) {
	    free(lm);
	    goto unlock;
	}
	lm->name = strdup(name);
	lm->dlh  = h;
	lm->next = pr_loadmap;
	pr_loadmap = lm;

        /*
        ** Try to load a table of "static functions" provided by the DLL
        */

        pfn = (NODL_PROC *)GetProcAddress(h, "NODL_TABLE");
        if (pfn != NULL) {
            lm->staticTable = (*pfn)();
        }
    }
#endif

#if defined(XP_MAC) && GENERATINGCFM
    {
	OSErr				err;
	Ptr					main;
	CFragConnectionID	connectionID;
	Str255				errName;
	Str255				pName;
	char				cName[64];
		
	/*
	 * Algorithm: The "name" passed in could be either a shared
	 * library name that we should look for in the normal library
	 * search paths, or a full path name to a specific library on
	 * disk.  Since the full path will always contain a ":"
	 * (shortest possible path is "Volume:File"), and since a
	 * library name can not contain a ":", we can test for the
	 * presence of a ":" to see which type of library we should load.
	 */
	if (strchr(name, MAC_PATH_SEPARATOR) == NULL)
	{
		/*
		 * The name did not contain a ":", so it must be a
		 * library name.  Convert the name to a Pascal string
		 * and try to find the library.
		 */
		OSType archType;
#ifdef GENERATINGPOWERPC
		archType = kPowerPCCFragArch;
#else
		archType = kMotorola68KCFragArch;
#endif

		strcpy((char *)&pName, name);
		c2pstr((char *)&pName);
			
		/* Try to find the specified library */
		err = GetSharedLibrary(pName, archType, kLoadCFrag, &connectionID,
				       &main, errName);

		if (err != noErr)
			goto unlock;
		
		strcpy(cName, name);
	}
	else	
	{
		/*
		 * The name did contain a ":", so it must be a full path name.
		 * Now we have to do a lot of work to convert the path name to
		 * an FSSpec (silly, since we were probably just called from the
		 * MacFE plug-in code that already knew the FSSpec and converted
		 * it to a full path just to pass to us).  First we copy out the
		 * volume name (the text leading up to the first ":"); then we
		 * separate the file name (the text following the last ":") from
		 * rest of the path.  After converting the strings to Pascal
		 * format we can call GetCatInfo to get the parent directory ID
		 * of the file, and then (finally) make an FSSpec and call
		 * GetDiskFragment.
		 */
		char* cMacPath = NULL;
		char* cFileName = NULL;
		char* position = NULL;
		CInfoPBRec pb;
		FSSpec fileSpec;
		uint32 index;

		/* Copy the name: we'll change it */
		cMacPath = strdup(name);	
		if (cMacPath == NULL)
			goto unlock;
			
		/* First, get the vRefNum */
		position = strchr(cMacPath, MAC_PATH_SEPARATOR);
		if ((position == cMacPath) || (position == NULL))
			fileSpec.vRefNum = 0;		/* Use application relative searching */
		else
		{
			char cVolName[32];
			memset(cVolName, 0, sizeof(cVolName));
			strncpy(cVolName, cMacPath, position-cMacPath);
			fileSpec.vRefNum = GetVolumeRefNumFromName(cVolName);
		}

		/* Next, break the path and file name apart */
		index = 0;
		while (cMacPath[index] != 0)
			index++;
		while (cMacPath[index] != MAC_PATH_SEPARATOR && index > 0)
			index--;
		if (index == 0 || index == strlen(cMacPath))
		{
			free(cMacPath);
			goto unlock;
		}
		cMacPath[index] = 0;
		cFileName = &(cMacPath[index + 1]);
		
		/* Convert the path and name into Pascal strings */
		strcpy((char*) &pName, cMacPath);
		c2pstr((char*) &pName);
		strcpy((char*) &fileSpec.name, cFileName);
		c2pstr((char*) &fileSpec.name);
		strcpy(cName, cFileName);
		free(cMacPath);
		cMacPath = NULL;
		
		/* Now we can look up the path on the volume */
		pb.dirInfo.ioNamePtr = pName;
		pb.dirInfo.ioVRefNum = fileSpec.vRefNum;
		pb.dirInfo.ioDrDirID = 0;
		pb.dirInfo.ioFDirIndex = 0;
		err = PBGetCatInfoSync(&pb);
		if (err != noErr)
			goto unlock;
		fileSpec.parID = pb.dirInfo.ioDrDirID;

		/* Finally, try to load the library */
		err = GetDiskFragment(&fileSpec, 0, kCFragGoesToEOF, fileSpec.name, 
						kLoadCFrag, &connectionID, &main, errName);

		if (err != noErr)
		    goto unlock;
	}
	
	lm->name = strdup(cName);
	lm->dlh = connectionID;
	lm->next = pr_loadmap;
	pr_loadmap = lm;
    }
#elif defined(XP_MAC) && !GENERATINGCFM
    {
	
    }
#endif

#ifdef XP_UNIX
#ifdef HAVE_DLL
    {
#ifdef USE_DLFCN
	void *h;
	h = dlopen(name, RTLD_LAZY);
#else  /* USE_HPSHL */
        shl_t h = shl_load(name, BIND_DEFERRED, NULL);
#endif
	if (!h) {
	    free(lm);
	    goto unlock;
	}
	lm->name = strdup(name);
	lm->dlh = h;
	lm->next = pr_loadmap;
	pr_loadmap = lm;
#if 0
	fprintf(stderr, "nspr: loaded library %s\n", name);
#endif
    }
#endif /* HAVE_DLL */
#endif /* XP_UNIX */

    lm->refCount = 1;
    result = lm;	/* success */
    PR_ASSERT(lm->refCount == 1);
    PR_LOG(Linker, debug, ("Loaded library %s (load lib)", lm->name));
  unlock:
    if (result == NULL)
	_pr_current_thread->errstr = pr_DLLError();
    PR_ExitMonitor(pr_linker_lock);
    return result;
}


PR_PUBLIC_API(PRLibrary*)
PR_FindLibrary(const char *name)
{
    PRLibrary* result;

    /* Initialize the linker if necessary */
    if (needInit) InitLinker();

    PR_EnterMonitor(pr_linker_lock);
    result = pr_UnlockedFindLibrary(name);
    PR_ExitMonitor(pr_linker_lock);
    return result;
}

/*
** Unload a shared library which was loaded via PR_LoadLibrary
*/
PR_PUBLIC_API(int)
PR_UnloadLibrary(PRLibrary *lib)
{
    int result = 0;

    if (lib == 0) return 0;

    /* Initialize the linker if necessary */
    if (needInit) InitLinker();

    PR_EnterMonitor(pr_linker_lock);
    if (--lib->refCount > 0) {
	PR_LOG(Linker, debug,
	       ("%s decr => %d",
		lib->name, lib->refCount));
	goto done;
    }
#ifdef XP_UNIX
#ifdef HAVE_DLL
#ifdef USE_DLFCN
    result = dlclose(lib->dlh);
#elif defined(USE_HPSHL)
    result = shl_unload(lib->dlh);
#endif
#endif /* HAVE_DLL */
#endif /* XP_UNIX */
#ifdef XP_PC
    if (lib->dlh) {
        FreeLibrary((HINSTANCE)(lib->dlh));
        lib->dlh = (HINSTANCE)NULL;
    }
#endif  /* XP_PC */

#if defined(XP_MAC) && GENERATINGCFM
    /* Close the connection */
	CloseConnection(&(lib->dlh));
#endif

    /* unlink from library search list */
    if (pr_loadmap == lib)
	pr_loadmap = pr_loadmap->next;
    else if (pr_loadmap != NULL) {
	PRLibrary* prev = pr_loadmap;
	PRLibrary* next = pr_loadmap->next;
	while (next != NULL) {
	    if (next == lib) {
		prev->next = next->next;
		goto freeLib;
	    }
	    prev = next;
	    next = next->next;
	}
	/* fail (but don't wipe out an error from dlclose/shl_unload) */
	if (result == 0) {
	    result = -1;
	}
    }
  freeLib:
    PR_LOG(Linker, debug, ("Unloaded library %s", lib->name));
    free(lib);
  done:
    if (result == -1)
	_pr_current_thread->errstr = pr_DLLError();
    PR_ExitMonitor(pr_linker_lock);
    return result;
}

static FARPROC
pr_FindSymbolInLib(PRLibrary *lm, const char *name)
{
    FARPROC f=0;

    if (lm->staticTable != NULL) {
	const PRStaticLinkTable* tp;
	for (tp = lm->staticTable; tp->name; tp++) {
	    if (strcmp(name, tp->name) == 0) {
		return (FARPROC) tp->fp;
	    }
	}
        /* 
        ** If the symbol was not found in the static table then check if
        ** the symbol was exported in the DLL... Win16 only!!
        */
#if !defined(XP_PC) || defined(_WIN32)
        return (FARPROC)NULL;
#endif
    }
    
#ifdef XP_PC
    f = GetProcAddress(lm->dlh, name);
#endif  /* XP_PC */

#ifdef XP_MAC
    {
	OSErr		err;
	Ptr			symAddr;
	CFragSymbolClass	symClass;
	Str255		pName;
			
	strcpy((char *)&pName, name);
	c2pstr((char *)&pName);
			
	err = FindSymbol(lm->dlh, pName, &symAddr, &symClass);
	f = (err == noErr ? (FARPROC)symAddr : NULL);
    }
#endif /* XP_MAC */

#ifdef XP_UNIX
#ifdef HAVE_DLL
#ifdef USE_DLFCN
    f = dlsym(lm->dlh, name);
#elif defined(USE_HPSHL)
    if (shl_findsym(&lm->dlh, name, TYPE_PROCEDURE, &f) == -1)
	f = NULL;
#endif
#endif /* HAVE_DLL */
#endif /* XP_UNIX */

    if (f == NULL)
	_pr_current_thread->errstr = pr_DLLError();
    return f;
}

/*
** Called by class loader to resolve missing native's
*/
PR_PUBLIC_API(FARPROC)
PR_FindSymbol(const char *raw_name, PRLibrary *lib)
{
    FARPROC f = NULL;
    const char *name;

    if (needInit) InitLinker();

    /*
    ** Mangle the raw symbol name in any way that is platform specific.
    */
#if defined(SUNOS4)
    /* Need a leading _ */
    name = PR_smprintf("_%s", raw_name);
#elif defined(AIX)
    /*
    ** AIX with the normal linker put's a "." in front of the symbol
    ** name.  When use "svcc" and "svld" then the "." disappears. Go
    ** figure.
    */
    name = raw_name;
#else
    name = raw_name;
#endif

    PR_EnterMonitor(pr_linker_lock);
    PR_ASSERT(lib != NULL);
    f = pr_FindSymbolInLib(lib, name);

#if defined(SUNOS4)
    free((void*)name);
#endif

    PR_ExitMonitor(pr_linker_lock);
    return f;
}

PR_PUBLIC_API(FARPROC)
PR_FindSymbolAndLibrary(const char *raw_name, PRLibrary* *lib)
{
    FARPROC f = NULL;
    const char *name;
    PRLibrary* lm;

    if (needInit) InitLinker();

    /*
    ** Mangle the raw symbol name in any way that is platform specific.
    */
#if defined(SUNOS4)
    /* Need a leading _ */
    name = PR_smprintf("_%s", raw_name);
#elif defined(AIX)
    /*
    ** AIX with the normal linker put's a "." in front of the symbol
    ** name.  When use "svcc" and "svld" then the "." disappears. Go
    ** figure.
    */
    name = raw_name;
#else
    name = raw_name;
#endif

    PR_EnterMonitor(pr_linker_lock);

    /* search all libraries */
    for (lm = pr_loadmap; lm != NULL; lm = lm->next) {
	f = pr_FindSymbolInLib(lm, name);
	if (f != NULL) {
	    *lib = lm;
	    lm->refCount++;
	    PR_LOG(Linker, debug,
		   ("%s incr => %d (for %s)",
		    lm->name, lm->refCount, name));
	    break;
	}
    }

#if defined(SUNOS4)
    free((void*)name);
#endif

    PR_ExitMonitor(pr_linker_lock);
    return f;
}

/*
** Add a static library to the list of loaded libraries. If LoadLibrary
** is called with the name then we will pretend it was already loaded
*/
PR_PUBLIC_API(PRLibrary*)
PR_LoadStaticLibrary(const char *name, const PRStaticLinkTable *slt)
{
    PRLibrary *lm;
    PRLibrary* result = NULL;

    /* Initialize the linker if necessary */
    if (needInit) InitLinker();

    /* See if library is already loaded */
    PR_EnterMonitor(pr_linker_lock);

    /* If the lbrary is already loaded, then add the static table information... */
    result = pr_UnlockedFindLibrary(name);
    if (result != NULL) {
        PR_ASSERT( (result->staticTable == NULL) || (result->staticTable == slt) );
        result->staticTable = slt;
        goto unlock;
    }

    /* Add library to list...Mark it static */
    lm = (PRLibrary*) calloc(1, sizeof(PRLibrary));
    if (lm == NULL) goto unlock;

    lm->name        = name;
    lm->refCount    = 1;
    lm->dlh         = pr_exe_loadmap ? pr_exe_loadmap->dlh : 0;
    lm->staticTable = slt;
    lm->next        = pr_loadmap;
    pr_loadmap      = lm;

    result = lm;	/* success */
    PR_ASSERT(lm->refCount == 1);
  unlock:
    PR_LOG(Linker, debug, ("Loaded library %s (static lib)", lm->name));
    PR_ExitMonitor(pr_linker_lock);
    return result;
}

