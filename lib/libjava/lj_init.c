/* -*- Mode: C; tab-width: 4; -*- */
/* From mozilla */
#include "lj.h"

/* From NSPR */
#include "prfile.h"
#include "prthread.h"
#include "prlink.h"
#include "prlog.h"
#include "prprf.h"
#include "prmon.h"
#include "sys_api.h"
#include "sysmacros_md.h"		/* for sysStat (because PR_Stat isn't defined on all platforms) */
#include "prgc.h"
#include "oobj.h"
#include "interpreter.h"

#ifdef XP_MAC
#include <ConditionalMacros.h>
#include "prmacos.h"
#include "nspr_md.h"
#include "prlink.h"

extern PRStaticLinkTable rt_nodl_tab[];
extern PRStaticLinkTable nn_nodl_tab[];
extern PRStaticLinkTable na_nodl_tab[];
extern PRStaticLinkTable sam_nodl_tab[];
extern PRStaticLinkTable njs_nodl_tab[];
#endif

#if defined(AIX) || defined(BSDI) || defined(HPUX) || defined(LINUX) || defined(SCO) || defined(UNIXWARE)
#include "prlink.h"
extern PRStaticLinkTable au_nodl_tab[];
extern PRStaticLinkTable awt_nodl_tab[];
extern PRStaticLinkTable na_nodl_tab[];
extern PRStaticLinkTable nn_nodl_tab[];
extern PRStaticLinkTable njs_nodl_tab[];
extern PRStaticLinkTable rt_nodl_tab[];
#endif

#if (defined(XP_PC) && !defined(_WIN32))
extern PRStaticLinkTable nn_nodl_tab[];
extern PRStaticLinkTable na_nodl_tab[];
extern PRStaticLinkTable njs_nodl_tab[];
#endif

/* Keep these library handles around for unloading. */
PRLibrary* javaLib;
PRLibrary* awtLib;
PRLibrary* mmLib;
PRLibrary* jpegLib;
PRLibrary* netLib;
PRLibrary* appletLib;
PRLibrary* jsoLib;

/* From sun-java */
#include "jriext.h"
#include "zip.h"

#define IMPLEMENT_netscape_applet_MozillaAppletContext
#ifndef XP_MAC
#include "netscape_applet_MozillaAppletContext.h"
#else
#include "n_applet_MozillaAppletContext.h"
#endif
#define IMPLEMENT_netscape_applet_EmbeddedAppletFrame
#ifndef XP_MAC
#include "netscape_applet_EmbeddedAppletFrame.h"
#else
#include "n_applet_EmbeddedAppletFrame.h"
#endif

PR_LOG_DEFINE(NSJAVA);

#include "libi18n.h"

#ifdef XP_MAC
#include <Files.h>
#endif

#include <string.h>

#ifdef DEBUG_warren	/* add your name here */
#undef  WANT_BUILD_NUMBER_CHECK
#else
#define WANT_BUILD_NUMBER_CHECK
#endif

#ifdef WANT_BUILD_NUMBER_CHECK
#ifdef XP_UNIX
#include "ljbuild.h"
#else
int build_number = 7777777L;
#endif
#endif /* WANT_BUILD_NUMBER_CHECK */

/*
** This is a hack on the PC for now...
** Reason:  DLLs and EXEs do NOT share the same local environment
**          so to get bypass this problem ALL environment manipulation
**          is done in the DLL..
*/
#ifdef XP_PC   /* XXX  need PR_GetEnv() and PR_SetEnv() */
#define strcasecmp(a,b)     strcmpi(a,b)

extern char * privateGetenv(const char *);
extern int    privatePutenv(char *);

#define getenv(x)	privateGetenv(x)
#define putenv(x)	privatePutenv(x)

#ifndef _WIN32
#define MAX_PATH    _MAX_PATH
int64 LL_MAXINT = { 0xffffffffL, 0x7fffffffL };
int64 LL_MININT = { 0x00000000L, 0x80000000L };
int64 LL_ZERO   = { 0x00000000L, 0x00000000L };

#ifdef DEBUG
/* 
** These variables are used by the Win16 Debug heap check routines
** (defined in winfe/netscape.cpp) which have been overridden to
** work with NSPR threads...
*/
unsigned long hcCount;
unsigned long hcMallocCount;
unsigned long hcFreeCount;
unsigned long hcLimit = 0xffffffff;
#endif /* DEBUG */
#endif /* !_WIN32 */
#endif /* XP_PC */

#define MaxHeapSize 0
#define MinHeapSize 0

static char *program_name;



#ifndef DEBUG_jwz	/* You're all a bunch of weenies. */
# undef ZIP_NAME	/* Remove this ifdef to Do the Right Thing. */
#endif



#ifndef ZIP_NAME
  /* This fallback value is only used if -DZIP_NAME was not passed in as
     a cc option by the Makefile. */
# define ZIP_NAME "java_301"
#endif

char *lj_zip_name = ZIP_NAME;

JRIRuntimeInstance* jrt;
JRIEnv* mozenv;
jglobal classMozillaAppletContext = NULL;
int lj_bad_version;
int lj_correct_version;
int lj_is_started;

/* In the Windows version this code lives in the DLL */
#ifndef XP_PC
#include <fcntl.h>
#include <sys/stat.h>

int OpenCode(char *fn, char *sfn, char *dir, struct stat * st)
{
    long fd = -1;

    if ((st != 0) && (fn != 0) &&
        ((fd = PR_OpenFile(fn, 0, 0644)) >= 0) &&
        (fstat(fd, st) < 0)) {
        close(fd);
        fd = -1;
    }
    return fd < 0 ? -2 : fd;
}
#endif  /* !XP_PC */

#ifdef XP_UNIX
/*
** XXX Storage for this is defined in the javai.a library; good thing
** that whomever links with this library also links with that one!
** AIEEEEE!
*/
#include <X11/Xlib.h>
#include <X11/Intrinsic.h>
extern int awt_init_xt;
extern int16 awt_LocaleCharSetID;
extern Colormap (*awt_GetDefaultColormap)(void);
extern Widget (*awt_MakeAppletSecurityChrome)(Widget parent, char *message);
extern unsigned int (*awt_GetMaxColors)(void);

/* Imports from the front end */
extern int16 fe_LocaleCharSetID;
extern Colormap XFE_GetDefaultColormap(void);
extern unsigned int XFE_GetMaxColors(void);
extern Widget FE_MakeAppletSecurityChrome(Widget parent, char* message);
#endif /* XP_UNIX */

void LJ_SetProgramName(const char *name)
{
    program_name = strdup(name);
}

/************************************************************************/

#ifdef XP_UNIX
#if 0
/*
** Given a path to a car file, see if the car file is there and if it is,
** if it is the right version.
*/
static PRBool IsValidCAR(char *filename, int *found)
{
    CARFile *cf;
    TOCEntry *te, *endte;
    int num_found;

    cf = CAR_Open(filename);
    if (!cf) {
		return PR_FALSE;
    }

    *found = *found + 1;

    /*
    ** Scan through the car file looking for an entry that begins with a
    ** numeric value (which is an illegal java class or package name).
    */
#ifdef WANT_BUILD_NUMBER_CHECK
    te = cf->toc;
    endte = te + cf->entries;
    lj_bad_version = 0;
    lj_correct_version = build_number;
    for (; te < endte; te++) {
		if (isdigit(te->name[0])) {
			char *s = strchr((char*) te->name, '.');
			if (s) {
				*s = 0;
				num_found = atoi((char*) te->name);
				*s = '.';
				if (num_found == build_number) {
					CAR_Destroy(cf);
					return PR_TRUE;
				}
				lj_bad_version = num_found;
				break;
			} else {
				/* Bad car file */
				lj_bad_version = 31415;
				break;
			}
		}
    }
    CAR_Destroy(cf);
    return PR_FALSE;
#else
    return PR_TRUE;
#endif /* WANT_BUILD_NUMBER_CHECK */
}
#endif

/*
** Given a path to a zip file, see if the zip file is there and if it is,
** if it is the right version.
*/
static PRBool IsValidZIP(char *filename, int *found)
{
    zip_t *zf;
#ifdef WANT_BUILD_NUMBER_CHECK
    direl_t *d, *ed;
    int num_found;
#endif
	struct stat buf;
	
	if (!((sysStat(filename, &buf) != -1) && S_ISREG(buf.st_mode)))
		return PR_FALSE;

	zf = zip_open(filename);
	if (!zf) {
		return PR_FALSE;
	}

    *found = *found + 1;

    /*
    ** Scan through the zip file looking for an entry that begins with a
    ** numeric value (which is an illegal java class or package name).
    */
#ifdef WANT_BUILD_NUMBER_CHECK
    d = zf->dir;
    ed = d + zf->nel;
    lj_bad_version = 0;
    lj_correct_version = build_number;
    for (; d < ed; d++) {
		if (isdigit(d->fn[0])) {
			char *s = strchr((char*) d->fn, '.');
			if (s) {
				*s = 0;
				num_found = atoi((char*) d->fn);
				*s = '.';
				if (num_found == build_number) {
					zip_close(zf);
					return PR_TRUE;
				}
				lj_bad_version = num_found;
				break;
			} else {
				/* Bad zip file */
				lj_bad_version = 31415;
				break;
			}
		}
    }
    zip_close(zf);
    return PR_FALSE;
#else
    return PR_TRUE;
#endif /* WANT_BUILD_NUMBER_CHECK */
}

static PRBool MozillaExists(char *dir)
{
    char *fn;
    int fd;

    fn = PR_smprintf("%s/netscape/applet/MozillaAppletContext.class", dir);
    if ((fd = open(fn, O_RDONLY)) < 0) {
		free(fn);
		return PR_FALSE;
    }
    close(fd);
    free(fn);
    return PR_TRUE;
}
#endif /* XP_UNIX */

static void
AddDirToClassPath(char* path)
{
	const char* s = CLASSPATH();
    XP_ASSERT(path);
    if (!path[0]) 
      return;

    if (!strcmp(".", path)) {  /* hack: only help the user if 
                                  they give a trivial entry on the path
                                  We could find *all* relative paths,... but 
                                  that would be painful. XXX Perhaps after 3.01 */
      char cwd[MAXPATHLEN];
      if (getcwd(cwd, MAXPATHLEN)) {
        path = cwd;
      }
    }

	if (!s) {
		s = strdup(path);
	} else {
		s = PR_smprintf("%s%c%s", s, PATH_SEPARATOR, path);
	}
	setCLASSPATH(s);
}

static void
AddToClassPath(char* path)
{
    XP_ASSERT(path);

    if (!strchr(path, PATH_SEPARATOR)) {
      AddDirToClassPath(path);
    } else {
      char *dup = strdup(path);
      char *start;
      char *end;
      if (!dup)
        return; /* out of memory */
      start = dup;
      while (NULL != (end = strchr(start, PATH_SEPARATOR))) {
        *end = '\0';
        AddDirToClassPath(start);
        start = end + 1;
      }
      AddDirToClassPath(start);
      free(dup);
    }
}

void
LJ_AddToClassPath(char* dirPath)
{
    struct PRDirEntryStr *ptr;
    PRDir* dir;

	/* Add this path to the classpath: */
	AddToClassPath(dirPath);

	/* Also add any zip files in this directory to the classpath: */
	dir = PR_OpenDirectory(dirPath);
	if (dir == NULL) return;
	while ((ptr = PR_ReadDirectory(dir, 0)) != NULL) {
		if (strcmp(ptr->d_name, ".") && strcmp(ptr->d_name, "..")) {
			struct stat buf;
			char* path = PR_smprintf("%s%c%s", dirPath, DIRECTORY_SEPARATOR, ptr->d_name);
			if ((sysStat(path, &buf) != -1) && S_ISREG(buf.st_mode)) {
				int len = strlen(path);
				if ((len > 4) && (strcasecmp(path+len-4, ".zip") == 0)) {		/* Is it a zip file? */
					AddToClassPath(path);
				}
			}
			free(path);
		}
	}
	PR_CloseDirectory(dir);
}

#ifdef XP_UNIX
/*
** Grovel around looking for .class files that we an run with. Look for
** car files too, and if we find one make sure the version is acceptable.
*/
int LJ_GrovelForClasses(void)
{
    char *dir;
    char *s;
    char *path = 0;
    int canRun = 0;
    int inPath = 0;
    int found = 0;
    static int already_done = 0;
    static int last_result = 0;

    if (already_done) {
        return last_result;
    }
    already_done = 1;

    /*
    ** Look in the classpath for either a valid car file or an
    ** implementation of MozillaAppletContext.class.
    */
    if ((s = (char*)CLASSPATH()) != NULL) {
	char *s0, *s2;
        s0 = s = strdup(s);
        while (s != NULL) {
            s2 = strchr(s, PATH_SEPARATOR);
            if (s2 != NULL) {
                *s2++ = 0;
            }
            if (IsValidZIP(s, &found) || MozillaExists(s)) {
                inPath = canRun = 1;
                break;
            }
            s = s2;
        }
        free(s0);
    }

    while (!canRun) {
        char *fn;

        /*
        ** Look nearby where the program was run for a car file
        */
        if (program_name) {
            char *dirname = strrchr(program_name, '/');
            if (dirname) {
                *dirname = 0;
                fn = PR_smprintf("%s/%s", program_name, lj_zip_name);
                if (IsValidZIP(fn, &found)) {
                    path = fn;
                    canRun = 1;
                    break;
                }
                free(fn);
                if (MozillaExists(program_name)) {
                    path = strdup(program_name);
                    canRun = 1;
                    break;
                }

                /* strip out one more directory and tack on "lib" */
                dirname = strrchr(program_name, '/');
                if (dirname) {
                    *dirname = 0;
                    dir = PR_smprintf("%s/lib", program_name);
                    fn = PR_smprintf("%s/%s", dir, lj_zip_name);
                    if (IsValidZIP(fn, &found)) {
                        path = fn;
                        free(dir);
                        canRun = 1;
                        break;
                    }
                    free(fn);
                    if (MozillaExists(dir)) {
                        path = dir;
                        canRun = 1;
                        break;
                    }
                    free(dir);
                }
            }
        }

        /*
        ** Try looking in /usr/local/netscape/java/classes
        */
        dir = "/usr/local/netscape/java/classes";
        fn = PR_smprintf("%s/%s", dir, lj_zip_name);
        if (IsValidZIP(fn, &found)) {
            path = fn;
            canRun = 1;
            break;
        }
        free(fn);
        if (MozillaExists(dir)) {
            path = strdup(dir);
            canRun = 1;
            break;
        }

        /*
        ** Try looking in /usr/local/lib/netscape/
        */
        dir = "/usr/local/lib/netscape";
        fn = PR_smprintf("%s/%s", dir, lj_zip_name);
        if (IsValidZIP(fn, &found)) {
            path = fn;
            canRun = 1;
            break;
        }
        free(fn);
        if (MozillaExists(dir)) {
            path = strdup(dir);
            canRun = 1;
            break;
        }

        /*
        ** Try looking in ~/.netscape/ for the ZIP file
        */
        if ((s = getenv("HOME")) != 0) {
            dir = PR_smprintf("%s/.netscape", s);
            fn = PR_smprintf("%s/%s", dir, lj_zip_name);
            if (IsValidZIP(fn, &found)) {
                path = fn;
                free(dir);
                canRun = 1;
                break;
            }
            free(fn);
            if (MozillaExists(dir)) {
                path = dir;
                canRun = 1;
                break;
            }
            free(dir);
        }
        break;
    }

    if (!canRun) {
        /* Losing! */
        if (found) {
            last_result = LJ_INIT_VERSION_WRONG;
            return LJ_INIT_VERSION_WRONG;
        } else {
            last_result = LJ_INIT_NO_CLASSES;
            return LJ_INIT_NO_CLASSES;
        }
    }
    if (!inPath) {
        /*
        ** Append the place that we found the files to the end of the
        ** classpath.
        */
		AddToClassPath(path);
        free(path);
    }
    last_result = LJ_INIT_OK;
    return LJ_INIT_OK;
}
#endif /* XP_UNIX */

#ifdef XP_PC
int LJ_GrovelForClasses(void)
{
    char *ev;
    int i;
    char exe_path[MAX_PATH];

    /* Find the directory where the EXE lives...*/
#if defined(_WIN32)
    GetModuleFileName( NULL, exe_path, MAX_PATH );
#else 
    GetModuleFileName( GetModuleHandle("NETSCAPE.EXE"), exe_path, MAX_PATH );	
#endif
    for( i = strlen(exe_path); i && exe_path[i] != '\\'; i-- );
    exe_path[i] = '\0';

    /*
    ** Unlike AddToClassPath which appends to the CLASSPATH, the java_xx
    ** file has to go at the beginning of the CLASSPATH to ensure that
    ** the proper classes will be found.
    */
    /* Do not leave a dangling ';' in the CLASSPATH */
    ev = CLASSPATH();
    if( ev ) {
        ev = PR_smprintf("%s\\java\\classes\\%s;%s\\java\\classes;%s", 
                         exe_path, lj_zip_name, exe_path, ev);
    } else {
        ev = PR_smprintf("%s\\java\\classes\\%s;%s\\java\\classes", 
                         exe_path, lj_zip_name, exe_path);
    }
	setCLASSPATH(ev);
    /* The ev buffer cannot be freed since the RTL does not copy it */
                    
    /* Do not leave a dangling ';' in the LD_LIBRARY_PATH */
    ev = getenv("LD_LIBRARY_PATH");
    if( ev ) {
        ev = PR_smprintf("LD_LIBRARY_PATH=%s\\java\\bin;%s", exe_path, ev );
    } else {
        ev = PR_smprintf("LD_LIBRARY_PATH=%s\\java\\bin", exe_path );
    }
    putenv(ev);
    /* The ev buffer cannot be freed since the RTL does not copy it */

    return LJ_INIT_OK;
}
#endif  /* XP_PC */

#ifdef XP_MAC

enum {
	kCannotInitializeJavaWrongClasses	= -5668,
	kCannotInitializeJavaNoZipFile		= -5669
};

extern OSErr ConvertUnixPathToFSSpec(const char *unixPath, FSSpec *fileSpec);

int LJ_GrovelForClasses(void)
{
    static  int last_result = 0;
    char		*classPath;
    char		*zipFilePath = NULL,
    			*currentPathItem,
    			*scanLocation;
    UInt32		classPathLength;
	UInt32		appVersionNumber = 0,
				zipVersionNumber = 0;
	Handle		versionResource;
	int			zipFileFound = 0,
				zipFileRightVersion = 0;
				
	if (last_result != 0)
		return last_result;
	
	/*	In order to find out if we have a valid zip file, get
	**	the version (2) resource out of the application's resource
	**	fork and the version (2) resource ouf of the .zip
	**	file.  The first four bytes are the complete version
	**	number and the sub-release.  For the zip file to be valid,
	**	it must be versioned at or after the version of the app.
	
	**
	**	First get the information from the application...
	*/
	
	versionResource = GetResource('vers', 2);
	if (versionResource != NULL) {
		DetachResource(versionResource);
		appVersionNumber = **((UInt32 **)versionResource);
		DisposeHandle(versionResource);
	}
	
	/*
	**	Then from the zip file.  This is done by first finding
	**	the first item in the CLASSPATH env variable that has
	**	a suffix of .zip, and then opening this file's resource
	**	fork and extracting the version (2) resource.
	*/
	
    classPath = strdup(CLASSPATH());
    
    if (classPath != NULL) {
    
        UInt32	ljZipLength = strlen(lj_zip_name);

		classPathLength = strlen(classPath);
		currentPathItem = classPath;
		scanLocation = classPath;
		
		while (classPathLength--) {
		
			UInt32	currentItemLength;
			
			if (*scanLocation == ':') {
				*scanLocation = '\0';

				currentItemLength = strlen(currentPathItem);
				if ((currentItemLength >= ljZipLength) &&
                    (!strcmp(currentPathItem + currentItemLength - ljZipLength,
                             lj_zip_name))) {
					zipFilePath = currentPathItem;
					break;
				}
				currentPathItem = scanLocation + 1;
			
			}
			
			scanLocation++;
		
		}

	}
	
	if (zipFilePath != NULL) {
	
		char		*macPath = NULL;
		short		refNum;
		short		volume;
		FSSpec		zipFileSpec;
	
		if (ConvertUnixPathToFSSpec(zipFilePath, &zipFileSpec) == noErr) {
		
			refNum = FSpOpenResFile(&zipFileSpec, fsRdWrPerm);
			
			if (refNum > 0) {
			
				zipFileFound = 1;
			
				versionResource = Get1Resource('vers', 2);
				if (versionResource != NULL) {
					DetachResource(versionResource);
					zipVersionNumber = **((UInt32 **)versionResource);
					DisposeHandle(versionResource);
				}	 
			
				CloseResFile(refNum);
			
			}
		
			free(macPath);
		
		}
	
	}

	free(classPath);

	/*	Check the versions against each other. */
	
	zipFileRightVersion = (zipVersionNumber >= appVersionNumber) && (appVersionNumber != 0);
	
	/*	Display the right message if we need to. */
	
	if (!zipFileRightVersion) {
		lj_init_failed = 1;
	    if (zipFileFound) {
	    	Alert(kCannotInitializeJavaWrongClasses, NULL);
   	     	last_result = LJ_INIT_VERSION_WRONG;
   	     	return LJ_INIT_VERSION_WRONG;
   	 	} else {
	    	Alert(kCannotInitializeJavaNoZipFile, NULL);
 	       	last_result = LJ_INIT_NO_CLASSES;
   	     	return LJ_INIT_NO_CLASSES;
   	 	}
	}

    return LJ_INIT_OK;

}
#endif /* XP_MAC */

/*******************************************************************************
 * Applet Trimming
 ******************************************************************************/

extern jglobal classMozillaAppletContext;

/*
** You must set noisyTrimming to false if you're going to force trimming
** during gc (the Mac use to, but now it doesn't -- see fredmem.cp). If
** you don't, the calls to println will cause allocation while you're in
** the gc which will get you into even bigger trouble.
*/
jbool noisyTrimming = JRITrue;

#ifdef XP_MAC
/*
** The Mac forces trimming every time by only allowing one applet (i.e.
** no applets in the history).
*/ 
jint trimThreshold = 1;
#else
jint trimThreshold = 10;
#endif

void
LJ_SetAppletTrimParams(jint threshold, jbool noisy)
{
	struct java_lang_Class* clazz;
	trimThreshold = threshold;
	noisyTrimming = noisy;
    if (lj_init_failed || !lj_java_enabled
        || classMozillaAppletContext == NULL) {	/* haven't even called init */
		/* save the values for later when we do start up */
        return;
	}
	clazz = JRI_GetGlobalRef(mozenv, classMozillaAppletContext);
	set_netscape_applet_MozillaAppletContext_trimThreshold(
		mozenv, clazz, threshold);
	set_netscape_applet_MozillaAppletContext_noisyTrimming(
		mozenv, clazz, noisy);
}

jint
LJ_GetTotalApplets()
{
	struct java_lang_Class* clazz;
    if (lj_init_failed || !lj_java_enabled
        || classMozillaAppletContext == NULL) {	/* haven't even called init */
		/* save the values for later when we do start up */
        return 0;
	}
	clazz = JRI_GetGlobalRef(mozenv, classMozillaAppletContext);
	return get_netscape_applet_MozillaAppletContext_totalApplets(
		mozenv, clazz);
}

jint
LJ_TrimApplets(jint count, jbool trimOnlyStoppedApplets)
{
	jint result;
    if (lj_init_failed || !lj_java_enabled
        || classMozillaAppletContext == NULL)	/* haven't even called init */
        return -1;
    PR_EnableAllocation(0);
	result = netscape_applet_MozillaAppletContext_trimApplets(
				mozenv, 
				JRI_GetGlobalRef(mozenv, classMozillaAppletContext),
				count, trimOnlyStoppedApplets);
    PR_EnableAllocation(1);
	if (JRI_ExceptionOccurred(mozenv)) {
		JRI_ExceptionDescribe(mozenv);
		JRI_ExceptionClear(mozenv);
	}
    return result;
}

/************************************************************************/

static void
lj_CleanUpOldDownloadedZips(void)
{
    XP_Dir dir;
	XP_DirEntryStruct* entry;
	char* dirPath = XP_TempDirName();
/*	XP_TRACE(("temp dir = %s\n", dirPath));*/
	dir = XP_OpenDir(dirPath, xpTemporary);
	if (dir == NULL) return;
	while ((entry = XP_ReadDir(dir)) != NULL) {
/*		XP_TRACE(("looking at %s\n", entry->d_name));*/
		if (strncmp(entry->d_name, DOWNLOADABLE_ZIPFILE_PREFIX,
					strlen(DOWNLOADABLE_ZIPFILE_PREFIX)) == 0) {
			char buf[MAXPATHLEN];
			int status;
#ifdef XP_MAC
			/* The Mac XP_ routines use ':' as directory separators (and puts 
			   on a trailing one) whereas the nspr ones use '/'. Go figure. */
			PR_snprintf(buf, MAXPATHLEN, "%s%s", dirPath, entry->d_name);
#else
			PR_snprintf(buf, MAXPATHLEN, "%s%c%s",
						dirPath, DIRECTORY_SEPARATOR, entry->d_name);
#endif
			status = XP_FileRemove(buf, xpTemporary);
/*			XP_TRACE(("removing %s => %d\n", buf, status));*/
		}
	}
	XP_CloseDir(dir);
}

/************************************************************************/

/*
** Start up java. This reflects the mozilla nspr thread into java. It
** also prepares the interpreter for execution, and does all of the misc
** initialization the java runtime needs.
*/
int LJ_StartupJava()
{
    static char started = 0;
    char *ev;
    int result;
    PRThread* thread;
    int oldPri;
	JRIEnv* env;
	jref clazz;
	JRIRuntimeInstance* jrt;
	JRIRuntimeInitargs initargs = {
		1,		/* majorVersion */
		0,		/* minorVersion */
		10*1024,/* initialHeapSize */
		0,		/* maxHeapSize */
		NULL,	/* collectionStartProc */
		NULL,	/* collectionEndProc */
#if 0
		/* XXX I have to take this out for now because we get a 
		   verifier error on the class InetAddress when we leave
		   it in:

VERIFIER ERROR java/net/InetAddress.getAllByName(Ljava/lang/String;)[Ljava/net/InetAddress;:
Bad access to protected data

		   This is because the following coercion doesn't pass the 
		   verifier (line 245):

    	      obj = ((Object[])obj).clone();

		*/
#ifdef DEBUG
		JRIVerifyAll	/* verifyMode */
#else
		JRIVerifyRemote	/* verifyMode */
#endif
#else	/* use this instead for now: */
		JRIVerifyRemote	/* verifyMode */
#endif	/* 0 */
	};

    if (started) {
        if (lj_init_failed) {
            return LJ_INIT_FAILED_LAST_TIME;
        }
        return LJ_INIT_OK;
    }
    started = 1;
    result = LJ_INIT_OK;

#ifdef XP_UNIX
    awt_init_xt = 0;
	awt_GetDefaultColormap = XFE_GetDefaultColormap;
	awt_LocaleCharSetID = fe_LocaleCharSetID;
	awt_MakeAppletSecurityChrome = FE_MakeAppletSecurityChrome;
	awt_GetMaxColors = XFE_GetMaxColors;
#ifdef DEBUG
	{
		ev = getenv("VERBOSE");
		if (ev && ev[0]) {
			extern int verbose;
			verbose = 1;
		}
		ev = getenv("VERBOSECL");
		if (ev && ev[0]) {
			extern int verbose_class_loading;
			verbose_class_loading = 1;
		}
	}
#endif
#endif

	lj_CleanUpOldDownloadedZips();

	/*
    ** Snapshot the CLASSPATH so that it doesn't get munged for any child
    ** processes we launch:
    */
    ev = getenv("CLASSPATH");
	if (ev) 
      AddToClassPath(ev);

    result = LJ_GrovelForClasses();
    if (result != LJ_INIT_OK) {
        return result;
    }

    thread = PR_CurrentThread();/* XXX */
    oldPri = PR_GetThreadPriority(thread);/* XXX */

	/*
	** Static libraries must be initialized before java is started.  Otherwise,
	** native methods required for java initialization may be unavailable :-(
	*/
#ifdef XP_PC
#if defined(_WIN32)
	javaLib =	PR_LoadLibrary("jrt32301.dll");
	jpegLib =	PR_LoadStaticLibrary("jpeg.dll",	NULL);
	netLib =	PR_LoadStaticLibrary("net.dll",		NULL);
#else
	javaLib =	PR_LoadLibrary("jrt1630.dll");
	jpegLib =	PR_LoadStaticLibrary("jpeg.dll",	NULL);
	netLib =	PR_LoadStaticLibrary("net.dll",		nn_nodl_tab);
	appletLib =	PR_LoadStaticLibrary("app.dll",		na_nodl_tab);
	jsoLib =	PR_LoadStaticLibrary("jso.dll",		njs_nodl_tab); 
#endif
#endif /* XP_PC */

#if defined(AIX) || defined(BSDI) || defined(HPUX) || defined(LINUX) || defined(SCO) || defined(UNIXWARE)
	javaLib =	PR_LoadStaticLibrary("javart.so",	rt_nodl_tab);
	awtLib =	PR_LoadStaticLibrary("libawt.so",	awt_nodl_tab);
	mmLib =		PR_LoadStaticLibrary("libmm3230.so",	au_nodl_tab);/* XXX for sun/audio */
	jpegLib =	PR_LoadStaticLibrary("libjpeg.so",	NULL);
	netLib =	PR_LoadStaticLibrary("libnet.so",	nn_nodl_tab);
	appletLib =	PR_LoadStaticLibrary("libapp.so",	na_nodl_tab);
	jsoLib =	PR_LoadStaticLibrary("libjso.so",	njs_nodl_tab);
#elif defined(XP_UNIX)
	javaLib =	PR_LoadStaticLibrary("javart.so",		NULL);
	awtLib =	PR_LoadStaticLibrary("libawt.so",		NULL);
	mmLib =		PR_LoadStaticLibrary("libmm3230.so",	NULL);/* XXX for sun/audio */
	jpegLib =	PR_LoadStaticLibrary("libjpeg.so",		NULL);
	netLib =	PR_LoadStaticLibrary("libnet.so",		NULL);
	appletLib =	PR_LoadStaticLibrary("libapp.so",		NULL);
#endif /* XP_UNIX */

#if defined(XP_MAC) && GENERATINGCFM
	javaLib =	PR_LoadStaticLibrary("javart",	rt_nodl_tab);
	awtLib =	PR_LoadStaticLibrary("awt",		NULL);
	mmLib =		PR_LoadStaticLibrary("mm3230",	NULL);	/* XXX for sun/audio */
	jpegLib =	PR_LoadStaticLibrary("jpeg",	NULL);
	netLib =	PR_LoadStaticLibrary("net",		nn_nodl_tab);
	appletLib =	PR_LoadStaticLibrary("app",		na_nodl_tab);
	jsoLib =	PR_LoadStaticLibrary("jso",		njs_nodl_tab);
#elif defined(XP_MAC) && !GENERATINGCFM
	javaLib =	PR_LoadStaticLibrary("javart",	rt_nodl_tab);
	awtLib =	PR_LoadStaticLibrary("awt",		sam_nodl_tab);
	/*		mmLib =		PR_LoadStaticLibrary("mm3230",	NULL); */
	/*		jpegLib =	PR_LoadStaticLibrary("jpeg",	NULL); */
	netLib =	PR_LoadStaticLibrary("net",		nn_nodl_tab);
	appletLib =	PR_LoadStaticLibrary("app",		na_nodl_tab);
	jsoLib =	PR_LoadStaticLibrary("jso",		njs_nodl_tab);
#endif	/* XP_MAC */

	jrt = JRI_NewRuntime(&initargs);
	if (jrt == NULL)
		result = LJ_INIT_NO_CLASSES;
	else {
		env = JRI_NewEnv(jrt, PR_CurrentThread());
		if (env == NULL)
			result = LJ_INIT_NO_CLASSES;
		else {
			mozenv = env;
			lj_InitThread();
#ifdef DEBUG
			JRI_SetIOMode(jrt, (JRIIOModeFlags)(JRIIOMode_AllowStdout | JRIIOMode_AllowSocket));
#else
			JRI_SetIOMode(jrt, JRIIOMode_AllowSocket);
#endif /* DEBUG */
			JRI_SetFSMode(jrt, JRIFSMode_None);

			/*
			** Initialize all the method IDs of the methods we're going
			** to call:
			*/
			use_netscape_applet_EmbeddedAppletFrame(env);
			clazz = use_netscape_applet_MozillaAppletContext(env);
			if (clazz == NULL) {
				result = LJ_INIT_NO_CLASSES;
			}
			else {
				classMozillaAppletContext = JRI_NewGlobalRef(env, clazz);
				LJ_InvokeMethod(methodID_netscape_applet_MozillaAppletContext_init, clazz);
			}
			LJ_SetAppletTrimParams(trimThreshold, noisyTrimming);
			if (JRI_ExceptionOccurred(env)) {
#ifdef DEBUG
				JRI_ExceptionDescribe(env);
#endif
				JRI_ExceptionClear(env);
				PR_LOG(NSJAVA, error, 
					   ("Can't find class %s\n", 
						classname_netscape_applet_MozillaAppletContext));
				result = LJ_INIT_NO_CLASSES;
			}
		}
	}

#ifdef XP_WIN
	/* try to load a JIT */
	if(jrt && env) {
		(void)JRI_FindClass(env, "java/lang/Compiler");
	}
#endif

    /* Because Java could change it on us: *//* XXX */
    PR_SetThreadPriority(thread, oldPri);/* XXX */

    if (result == LJ_INIT_OK) {
		lj_is_started = 1;
    }
    return result;
}

void LJ_ShutdownJava()
{
	lj_CleanUpOldDownloadedZips();
    if (lj_init_failed || !lj_java_enabled) return;
#ifndef XP_WIN
	PR_UnloadLibrary(jsoLib);
	PR_UnloadLibrary(appletLib);
	PR_UnloadLibrary(netLib);
	PR_UnloadLibrary(jpegLib);
	PR_UnloadLibrary(mmLib);
	PR_UnloadLibrary(awtLib);
	PR_UnloadLibrary(javaLib);
#endif
}

/************************************************************************/

jref
LJ_MakeArrayOfString(int size)
{
    JRIEnv* env = mozenv;
    jref classString;
    classString = JRI_FindClass(env, "java/lang/String");
    return JRI_NewObjectArray(env, size, classString, NULL);
}

jref
intl_makeJavaString(int16 encoding, char *str, int len)
{
    JRIEnv* env = mozenv;
    INTL_Unicode* q;
    int unicodelen, realunicodelen;
    
    /* call the origional makeJavaString for some ascii */ 
    if (encoding == 0)
        return JRI_NewStringUTF(env, str, len);
    
    unicodelen = INTL_TextToUnicodeLen(encoding, (unsigned char*)str, len);
    
    if ((q = XP_ALLOC(sizeof(INTL_Unicode) * unicodelen)) != NULL) {
        jref result;
        if (str) 
            INTL_StrToUnicode(encoding, (unsigned char*)str, q, unicodelen );
        realunicodelen = INTL_TextToUnicode(encoding, (unsigned char*)str, len, q, unicodelen );
	
        result = JRI_NewString(env, (const short*)q, realunicodelen);
        XP_FREE(q);
        return result;
    } else {
        XP_ASSERT(q);
        return JRI_NewStringUTF(env, str, len);
    }
}

/*
** Setup an argument to a java procedure. This converts name, value into
** "name=value" for the java code and then converts the C string to a
** java string.
*/
int LJ_SetStringArraySlot(jref hs, int slot, char *name, char *value,
			  int16 encoding)
{
    JRIEnv* env = mozenv;
    char sbuf[100];
    char *buf = sbuf;
    int l1, l2;
    jref str;

    l1 = strlen(name);
    if (value) {
        l2 = strlen(value);
        if (l1 + 1 + l2 >= sizeof(sbuf)) {
            buf = (char*) malloc(l1+1+l2+1);
            if (!buf) return 0;
        }
        sprintf(buf, "%s=%s", name, value);
    } else {
        if (l1 >= sizeof(sbuf)) {
            buf = (char*) malloc(l1+1);
            if (!buf) return 0;
        }
        sprintf(buf, "%s", name);
    }

    /* Store address of converted C string */
    str = intl_makeJavaString(encoding, buf, strlen(buf));
    JRI_SetObjectArrayElement(env, hs, slot, str);
    if (buf != sbuf) {
        free(buf);
    }
    return 1;
}

/******************************************************************************/

/*
** Invoke a public static method in the MozillaAppletContext class.  The
** MozillaAppletContext manages all of the embedded java applets.
*/
jref LJ_InvokeMethod(JRIMethodID method, ...)
{
    JRIEnv* env = mozenv;
    va_list args;
    PRThread* thread;
    int oldPri;
    jref ctxt;
    jref rval;

    if (lj_init_failed || !lj_java_enabled
        || classMozillaAppletContext == NULL)	/* haven't even called init */
        return NULL;

    thread = PR_CurrentThread();/* XXX */
    oldPri = PR_GetThreadPriority(thread);/* XXX */

#ifdef XP_PC
    /*    PreventAsyncGC(TRUE); */
#endif

    ctxt = JRI_GetGlobalRef(env, classMozillaAppletContext);

    va_start(args, method);
    rval = JRI_CallStaticMethodV(env, ctxt, method, args);
    va_end(args);
    
    if (JRI_ExceptionOccurred(env)) {
#ifdef DEBUG
		JRI_ExceptionDescribe(env);
#endif
        JRI_ExceptionClear(env);
        rval = NULL;
    }

    /* XXX Because Java could change it on us: */
    PR_SetThreadPriority(thread, oldPri);

#ifdef XP_PC    
    /*    PreventAsyncGC(TRUE); */
#endif

    return rval;
}

/************************************************************************/

/*
** This doesn't really do anything, but this does force the symbols to be
** bound in. On some systems a data reference is insufficient to drag in
** a library.
*/

#ifdef XP_PC
extern void _java_image_init(void);
#else
extern void _java_awt_init(void);
extern void _java_lang_init(void);
extern void _java_mmedia_init(void);
#endif   /* !XP_PC */
extern void _java_net_init(void);
extern void _java_applet_init(void);
extern void _java_javascript_init(void);

void LJ_FakeInit(void)
{
#ifdef XP_PC
    _java_image_init();
#else
    _java_awt_init();
    _java_lang_init();
    _java_mmedia_init();
#endif   /* !XP_PC */
    _java_net_init();
    _java_applet_init();
    _java_javascript_init();
}
