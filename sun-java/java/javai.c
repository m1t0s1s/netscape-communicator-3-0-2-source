/*
 * @(#)javai.c	1.75 95/08/11
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

/*-
 *      interpret an java object file
 */


#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <ctype.h>

#include "javaThreads.h"
#include "errno.h"
#include "tree.h"
#include "oobj.h"
#include "interpreter.h"
#include "exceptions.h"
#include "javaString.h"
#include "log.h"
#include "installpath.h"
#include "bool.h"
#include "jio.h"

#ifndef NATIVE
#include "iomgr.h"
#include <fcntl.h>
#ifndef XP_PC
#include <sys/wait.h>
#include <unistd.h>
#include <sys/signal.h>
#endif /* ! XP_PC */
#include <signal.h>
#else /* NATIVE  */
#include "monitor.h"
#endif /* NATIVE */

#include "prglobal.h"

#include "prlink.h"
#if defined(AIX) || defined(BSDI) || defined(HPUX) || defined(LINUX) || defined(SCO) || defined(UNIXWARE) || (defined(XP_PC) && !defined(_WIN32))
extern PRStaticLinkTable rt_nodl_tab[];
#endif

#ifdef MONITOR
#include <mon.h>
#endif

#ifndef NATIVE
#include <java_lang_Thread.h>
#endif /* NATIVE */

int RELEASE =
#include "../../build/build_number"
;

extern int verbose_class_loading;

/* Force load in the language level natives */
extern void _java_lang_init(void);

extern Hjava_lang_Thread *InitializeClassThread(ExecEnv *ee, char **);
extern void InitializeAlloc(long max, long min);
#ifndef NATIVE
extern void InitializeSbrk(void);
#endif /* NATIVE */

extern void WaitToDie(void);

static void usage(void);

#ifdef MONITOR
static void MonitorTeardown(void) {
    monitor( 0, 0, 0, 0, 0 );
}

static void MonitorSetup(void) {
    extern int etext(void);
    extern int _start(void);
    int (* low)() = &_start;
    int (* high)() = & etext;
    WORD * stuff;
    int textwords = ((int)high - (int)low) / 4;
    int nbytes = sizeof(struct hdr) + 10*sizeof( struct cnt )
				    + textwords*sizeof(WORD);
    stuff = (WORD*)malloc( nbytes );
    monitor( low, high, stuff, nbytes/sizeof(WORD), 10 );
    sysAtexit( MonitorTeardown );
}
#endif

/* from properties_md.c */
extern char **user_props;
static int nprops = 0;
static int max_props = 0;

/* add a user property */
static void add_user_prop(char *def) 
{
    char *p, *str;

    /* scan to the '=' */
    for (p = def ; *p && *p != '=' ; p++);

    /* grow the array (if needed) */
    if (nprops + 2 > max_props) {
	if (user_props == 0) {
	    user_props = (char **)malloc(16 * sizeof(char *));
	    max_props = 16;
	} else {
	    char **new_props = (char **)malloc(2 * max_props * sizeof(char *));
	    memcpy(new_props, user_props, nprops * sizeof(char *));
	    free(user_props);
	    user_props = new_props;
	    max_props *= 2;
	}
    }

    /* malloc the key */
    str = (char *)malloc((p - def) + 1);
    strncpy(str, def, p - def);
    str[p - def] = '\0';
    user_props[nprops++] = str;

    /* malloc the value */
    if (*p == '=') {
	p++;
    }
    str = (char *)malloc(strlen(p) + 1);
    strcpy(str, p);
    user_props[nprops++] = str;
}

/*
 * have to put the hidden copy of the command line argument list where GC
 * will find it
 */

HArrayOfString *
commandLineArguments(int argc, char **argv, struct execenv *ee)
{
    HArrayOfString *ret = (HArrayOfString *) ArrayAlloc(T_CLASS, argc);
    if (ret == 0) {
OutOfMemory:
	SignalError(ee, JAVAPKG "OutOfMemoryError", 0);
	return 0;
    }
    /* Give the array a "type" */
    unhand((HArrayOfObject *)ret)->body[argc] = 
                        (JHandle *)FindClass(ee, JAVAPKG "String", TRUE);
    while (--argc >= 0) {
	char *s = argv[argc];
	if ((unhand(ret)->body[argc] = makeJavaString(s, strlen(s))) == 0)
	    goto OutOfMemory;
    }
    return (HArrayOfString *) ret;
}

long
atomi(register char *p)
{
    long v = 0;
    long mult = 1;
    long c;
    while ((c = *p++))
	switch (c) {
	case 'M':
	case 'm':
	    mult = 1024 * 1024;
	    break;
	case 'K':
	case 'k':
	    mult = 1024;
	    break;
	default:
	    if (c < '0' || c > '9')
		return -1;
	    v = v * 10 + c - '0';
	    break;
	}
    return v * mult;
}

/*
 * The reason we do not use this routine anymore is that we explicitly
 * leave stdin, stdout, and stderr, unblocked i.e we do not mark this 
 * file descriptors nonblocking and hence we do not have to undo i.e
 * mark them blocking before we exit anymore.  Look at iomgr.c for how
 * these fds are left unmarked and more details
 */
#ifndef NATIVE
void
makeTTYsane()
{
    nonblock_io(0, IO_BLOCK);
    nonblock_io(1, IO_BLOCK);
    nonblock_io(2, IO_BLOCK);
}
#endif /* NATIVE */

extern void InitializeAsyncMonitors(void);

#include <path_md.h>

/*
 * Default values for minimum and maximum amounts of memory to devote
 * to the Java heap.  The current abstraction, which may not hold up
 * forever, is that the initial size of the heap is the minimum heap.
 *
 * The maximum memory to map: As we're mmapping the memory, it looks
 * like we should ask swapctl(2) how much swap we have, and thus what
 * the maximum we've got is.
 *
 * These values can be overridden on the java command line.
 */
#ifdef USE_MALLOC
#define MINHEAPSIZE ((int) ( 3 * (1024*1024)))
#define MAXHEAPSIZE MINHEAPSIZE
#else
#define MINHEAPSIZE ((int) ( 1 * (1024*1024)))
#define MAXHEAPSIZE ((int) (16 * (1024*1024)))
#endif /* USE_MALLOC */

/*
 * The default thread stack size.  This is machine-dependent, because it
 * is based on a platform's C stack usage.
 */
#define PROCSTACKSIZE (128 * 1024)     /* Default size of a thread C stack */

int awt_init_xt = 1;

#if defined(STATIC_JAVA)
#include "prlink.h"

extern void _java_awt_init(void);
extern void _java_mmedia_init(void);

#if defined(AIX) || defined(BSDI) || defined(HPUX) || defined(LINUX) || defined(SCO) || defined(UNIXWARE)
extern PRStaticLinkTable au_nodl_tab[];
extern PRStaticLinkTable awt_nodl_tab[];
extern PRStaticLinkTable rt_nodl_tab[];
#endif

#endif /* STATIC_JAVA */

#if defined(XP_PC) && !defined(_WIN32)

#include "prlink.h"
extern PRStaticLinkTable rt_nodl_tab[];

#endif  /* XP_PC && !_WIN32 */

#if defined(XP_UNIX) && defined(BREAKPTS)
extern void PR_InitInterrupts(void);
#endif

#if defined(XP_PC) && !defined(_WIN32)
/*
** On 16-bit windows, the Floating Point subsystem (WINEM87.DLL) 
** must be explicitly initialized... 
** Unfortunately, the necessary functions do not appear in any
** header files...
*/
void _acrtused2() {};
extern LPVOID   pascal  _FPInit();
extern void     pascal  _FPTerm(LPVOID);
#endif

#define INITBUFSIZE 1024
#define INITNARGS 32

/*
** Start an execution environment based on the arguments
** provided in argv
** ( called by start_java or set_argfile_opts, which is called by start_java )
*/
static int start_exec_env(int argc, char** argv) 
{
    int MinHeapSize = MINHEAPSIZE;
    int MaxHeapSize = MAXHEAPSIZE;
	long T;
    Hjava_lang_Thread *self;
    int on_exit(void (*)(), void *);
	struct methodblock *mb = 0;
	extern int noasyncgc;
	extern int tracegc;
	long debugport = 0L;
    ExecEnv ee;
	char * errmsg;

    char *sourcefile = 0;
    char **args = argv;
    char **dargv = argv;

    verifyclasses = VERIFY_REMOTE; /* we've decided this is the default */
    while (--argc > 0)
	if ((++argv)[0][0] == '-' && sourcefile == 0)
#ifdef TRACING
	    if (strcmp(*argv, "-t") == 0)
		trace++;
	    else if (strcmp(*argv, "-tm") == 0)
		tracem++;
	    else
#endif
#ifdef MONITOR
	    if (strcmp(*argv, "-m") == 0) {
    		MonitorSetup();
	    }
	    else
#endif	    
            if (strcmp(*argv, "-v") == 0 || strcmp(*argv, "-verbose") == 0)
		verbose++;
	    else if (strcmp(*argv, "-debug") == 0)
		debugagent++;
	    else if (strncmp(*argv, "-debugport", 10) == 0 &&
		     (T = atomi(&argv[0][10])) >= 0)
		debugport = T;
	    else if (strcmp(*argv, "-noasyncgc") == 0)
		noasyncgc++;
	    else if (strcmp(*argv, "-verbosegc") == 0)
		verbosegc++;
	    else if (strcmp(*argv, "-verbosecl") == 0)
		verbose_class_loading++;
	    else if (strcmp(*argv, "-verify") == 0)
		verifyclasses = VERIFY_ALL;
            else if (strcmp(*argv, "-verifyremote") == 0)
		verifyclasses = VERIFY_REMOTE;
            else if (strcmp(*argv, "-noverify") == 0)
		verifyclasses = VERIFY_NONE;
	    else if (strcmp(*argv, "-tracegc") == 0)
		tracegc++;
	    else if (strcmp(*argv, "-prof") == 0) {
		extern void javamon(int i);
		sysAtexit(java_mon_dump);
		javamon(1);
	    } else if (strcmp(*argv, "-cs") == 0 || 
		     strcmp(*argv, "-checksource") == 0)
		SkipSourceChecks = 0;
	    else if (strncmp(*argv, "-ss", 3) == 0 &&
		     (T = atomi(&argv[0][3])) > 1000)
		ProcStackSize = T;
	    else if (strncmp(*argv, "-oss", 3) == 0 &&
		     (T = atomi(&argv[0][3])) > 1000)
		JavaStackSize = T;
	    else if (strncmp(*argv, "-ms", 3) == 0 &&
		     (T = atomi(&argv[0][3])) > 1000)
		MinHeapSize = T;
	    else if (strncmp(*argv, "-mx", 3) == 0 &&
		     (T = atomi(&argv[0][3])) > 1000)
		MaxHeapSize = T;
            else if (strncmp(*argv, "-D", 2) == 0)
		add_user_prop(*argv + 2);
#ifdef LOGGING
	    else if (strncmp(*argv, "-l", 2) == 0 &&
		     (T = atomi(&argv[0][2])) >= 0)
		logging_level = T;
#endif
	    else if (strncmp(*argv, "-classpath", 10) == 0) {
		if (argc > 1) {
		    char *buf = (char *)malloc(strlen(argv[1]) + 32);
		    sprintf(buf, "CLASSPATH=%s", argv[1]);
		    putenv(buf);
		    argc--; argv++;
		} else {
		    fprintf(stderr,
			    "-classpath requires class path specification\n");
		    usage();
		    return 1;
                } 
	    } else if (strncmp(*argv, "-addpath", 10) == 0) {
	    	char *cpcurrent = getenv("CLASSPATH");
	    	if (cpcurrent == NULL)
			  cpcurrent = "";
	    		
			if (argc > 1) {
			  char *buf = (char *)malloc(strlen(cpcurrent)+strlen(argv[1])+32);
			  sprintf(buf, "CLASSPATH=%s;%s", getenv("CLASSPATH"), argv[1]);
			  putenv(buf);
			  argc--; argv++;
			} else {
			  fprintf(stderr,
					  "+classpath requires class path specification\n");
			  usage();
			  return 1;
			}
	    } else if (strcmp(*argv, "-h") == 0 || 
                       strcmp(*argv, "-help") == 0 ||
		       strncmp(*argv, "?", 1) == 0 || 
                       strncmp(*argv, "-?", 2) == 0) {
		usage();
		return 0;
            } else if (strcmp(*argv, "-version") == 0) {
                fprintf(stderr, "%s version %d\n", progname, RELEASE);
                return 0;
	    } else {
		fprintf(stderr, "%s: illegal argument\n", *argv);
		usage();
		return 1;
	    }
	else if (sourcefile)
	    *dargv++ = *argv;
	else {
	    sourcefile = *argv;
	}
    *dargv = 0;

	/* the rest of the args belong to the java program */
    InitializeExecEnv(&ee, 0);  /* Call before InitializeMem() */
    InitializeMem();		/* Could throw exception on open() */
#ifdef USE_MALLOC
    MinHeapSize = MaxHeapSize;
#endif /* USE_MALLOC */
    InitializeAlloc(MaxHeapSize, MinHeapSize);
    InitializeInterpreter();	/* Call before InitializeClassThread() */

    self = InitializeClassThread(&ee, &errmsg);
    if (self == 0) {
	fprintf(stderr, "Unable to initialize threads: %s\n", errmsg);
	sysExit(1);
    }
    setThreadName(self,MakeString("main",4));

    if (sourcefile == 0) {
	usage();
	return 1;
    } if (strchr(sourcefile, '/')) {
	fprintf(stderr, "Invalid class name: %s\n", sourcefile);
	usage();
	return 1;
    } else {
	ClassClass *cb;
	int i, c;
	char classname[200];

	/* Allow the use of '.' in class names (convert to /) */
	for (i = 0; i < sizeof classname - 1 && (c = sourcefile[i]) != 0; i++) {
	    classname[i] = c;
	    if (c == '.') sourcefile[i] = '/';
	    if (c == '/') classname[i] = '.';
	}
	classname[i] = 0;

	cb = FindClass(&ee, sourcefile, TRUE);
	if (cb == 0) {
	    fprintf(stderr, "Can't find class %s\n", classname);
	    return 1;
	}
	cb->flags |= CCF_Sticky;

	if (debugagent) {
	    ClassClass *agentcb = FindClass(&ee, "sun/tools/debug/Agent", 
					    TRUE);
	    if (agentcb == 0) {
		fprintf(stderr, "Can't find class sun.tools.debug.Agent\n");
		return 1;
	    }
	    execute_java_static_method(0, agentcb, "boot", "(I)V", debugport);
	}

	/* Finish initialization of the main thread */
	InitializeMainThread();

	mb = cbMethods(cb);
	for (i = cb->methods_count; --i >= 0; mb++) {
	    if ((strcmp(fieldname(&mb->fb), "main") == 0) &&
		(strncmp(fieldsig(&mb->fb), "([Ljava/lang/String;)", 20) == 0)){
		
		if (fieldsig(&mb->fb)[21] == SIGNATURE_VOID)
		    break;
		/* If we matched all the way to the return type, and its 
		 * return type is not SIGNATURE_VOID, then we know there is
		 * no method match because the compiler would complain about
		 * duplicate method declaration. */
		fprintf(stderr, "In class %s: main must return void\n",
			classname);
		return 1;
	    }
	}
	if (i < 0) {
	    fprintf(stderr, "In class %s: void main(String argv[]) is not defined\n",
		    classname);
	    return 1;
	}
	
	if ((mb->fb.access & (ACC_STATIC|ACC_PUBLIC))
	    != (ACC_STATIC|ACC_PUBLIC)) {
	    fprintf(stderr, "In class %s: main must be public and static\n", classname);
	    return 1;
	}

#if !defined(XP_PC) || defined(_WIN32)
        do_execute_java_method(&ee, cb, 0, 0, mb, TRUE,
                               (JHandle *)commandLineArguments(dargv - args, args, &ee));
#else
        /*
        ** For 16-bit windows initialize the floating-point system before executing
        ** the java method.  Restore the previous FP settings after the call...
        */
        {
            LPVOID oldHandler;

            oldHandler = _FPInit(); 
            do_execute_java_method(&ee, cb, 0, 0, mb, TRUE,
                                   (JHandle *)commandLineArguments(dargv - args, args, &ee));
            _FPTerm(oldHandler);
	}
#endif

	if (exceptionOccurred(&ee)) {
	    if (ee.thread && THREAD(ee.thread)->group) {
		void *t = (void *)ee.exception.exc;
		exceptionClear(&ee);
		execute_java_dynamic_method(&ee, (void *)THREAD(ee.thread)->group, 
		    "uncaughtException", "(Ljava/lang/Thread;Ljava/lang/Throwable;)V", ee.thread, t);
	    }
	}
    }

    /*
     * The primordial thread is not allowed to exit until all user
     * threads have exited.  Note that the primordial thread's
     * resources (e.g. its Java stack) have not been reclaimed at this
     * point, even though it's got no chance of running again.  The
     * thread is still on the active thread queue and will be scanned
     * on GC, so has to be left intact.  This should be revisited.
     */
    WaitToDie();

#if defined(XP_PC) /* jag, hack hack :) */
    if( WSACleanup() ) {
        fprintf(stderr, "error shutting down winsock\n");
        return 1;
    }
#endif

    return 0;

}

/*
** Reads command line arguments from one or more files.
** This is a std workaround for the limited command line
** length on windows
*/
static int
set_argfile_opts(int nfiles, char** files) {
  char* buf;
  int bufsize = INITBUFSIZE;
  int numchars = 0;
  int begin = 0;
  char** argv;
  int argvsize = INITNARGS;
  int argc = 0;
  int c;
  FILE *responseFile;
  int i, j = 0;
  int errcode;
  void * reallocd; /* tmp ptr to memory that has been realloc'd */

  if( !(buf = (char*) malloc( sizeof(char) * bufsize)) ){
	fprintf(stderr,"Out of memory.\n");
	return 1;
  }

  if( !(argv = (char**) malloc( sizeof(char*) * argvsize)) ){
	free( buf );
	fprintf(stderr, "Out of memory.\n");
	return 1;
  }

  *argv = progname; /* crude but quick: keep argv[0] the same for parsing */
  argc++;

  while( nfiles-- ) {
	if( (responseFile = fopen( files[j++],"r")) == NULL ) {
	  fprintf(stderr, "unable to open argument file %s", argv[1] );
	  errcode = 1;
	  goto Fail_Cleanup;
	  return 1;
	}

	/* read the entire file into memory */
	numchars = 0;
	while( (c = getc(responseFile) ) != EOF ) {
	  buf[numchars++] = (char) c;
	  if( numchars >= bufsize ) {
		bufsize *= 2;
		if( !(reallocd = realloc( buf, sizeof(char) * bufsize)) ) {
		  fprintf(stderr,"Out of memory.\n");
		  errcode = 1;
		  goto Fail_Cleanup;
		} else {
		  buf = (char*) reallocd;
		}
	  }
	}

	/* break into tokens */
	begin = 0;
	for( i = 0; i < bufsize; i++) {
	  if( isspace(buf[i]) ) {
		buf[i] = (char) NULL;
		argv[argc++] = strdup(buf + begin);
		while( i < (bufsize-1) && isspace(buf[i+1]))
		  buf[++i] = (char) NULL;
		begin = i + 1;
		if( argc >= argvsize ) {
		  argvsize *= 2;
		  if( !(reallocd = realloc( argv, sizeof(char*) * argvsize)) ) {
			fprintf(stderr,"Out of memory.\n");
			errcode = 1;
			goto Fail_Cleanup;
		  } else {
			argv = (char**) reallocd;
		  }
		}
	  }
	}

	if( fclose( responseFile ) ) {
	  fprintf(stderr,"error closing response file %s\n", files[j-1]);
	  errcode = 1;
	  goto Fail_Cleanup;
	}
  }

  if( !(reallocd = realloc( argv,  sizeof(char*) * argc)) ) {
	/* this should never happen, really */
	fprintf(stderr,"Out of memory.\n");
	errcode = 1;
	goto Fail_Cleanup;
  } else {
	argv = (char**) reallocd;
  }

  free(buf); buf = NULL; 
  errcode = start_exec_env( argc, argv );
Fail_Cleanup:
  if ( buf ) { /* we got here due to error in this proc, before start_exec_env */
	free( buf );
  }
  if( errcode ) { /* free stuff that the GC would have gotten */
	for( i = 1; i < argc; i++ ) { /* argv[0] points to the real argv! */
	  free( argv[i] );
	}
  }
  free( argv ); /* this isn't really argv -- see above */
  return errcode;
}


JAVA_PUBLIC_API(int)
start_java(int argc, char **argv)
{
	int ret = 0;

#if defined(XP_PC) /* jag, hack hack :) */
    WSADATA wsaData;
    WORD wVersionRequested = MAKEWORD(1,1);

    if( WSAStartup( wVersionRequested, &wsaData ) ) {
        fprintf(stderr, "winsock failed to initialize\n");
        return 1;
    }
#endif

    PR_Init("javai", 5, 0, 0);

#ifdef XP_UNIX
    PR_StartEvents(0);
#ifdef BREAKPTS
    PR_InitInterrupts();
#endif
#endif /* XP_UNIX */

    set_java_io_mode(JIO_UNRESTRICTED);
    set_java_fs_mode(JFS_UNRESTRICTED);
    set_java_rt_mode(JRT_UNRESTRICTED);
    intrInit();

    /* Initialize the monitor registry */
    monitorRegistryInit();
    /* Create the monitor cache */
    monitorCacheInit();

#ifndef NATIVE
    /* Initialize the special case for sbrk on Solaris (see synch.c) */
    InitializeSbrk();
    /* Initialize the async io */
    InitializeAsyncIO();
#endif 

#ifdef XP_PC
#if defined(_WIN32)
	PR_LoadLibrary("jrt3230.dll");
	PR_LoadStaticLibrary("jpeg.dll",	NULL);
	PR_LoadStaticLibrary("net.dll",		NULL);
#else
	PR_LoadLibrary("jrt1630.dll");
	PR_LoadStaticLibrary("jpeg.dll",	NULL);
	PR_LoadStaticLibrary("net.dll",		nn_nodl_tab);
	PR_LoadStaticLibrary("app.dll",		na_nodl_tab);
	PR_LoadStaticLibrary("jso.dll",		njs_nodl_tab); 
#endif
#endif /* XP_PC */

#if defined(STATIC_JAVA)
#if defined(AIX) || defined(BSDI) || defined(HPUX) || defined(LINUX) || defined(SCO) || defined(UNIXWARE)
    PR_LoadStaticLibrary("javart.so",	rt_nodl_tab);
    PR_LoadStaticLibrary("libawt.so",	awt_nodl_tab);
#else
    PR_LoadStaticLibrary("javart.so",	NULL);
    PR_LoadStaticLibrary("libawt.so",	NULL);
#endif
    PR_LoadStaticLibrary("libmm3230.so",au_nodl_tab);/* XXX for sun/audio */
    PR_LoadStaticLibrary("libjpeg.so",	NULL);
    PR_LoadStaticLibrary("libnet.so",	NULL);
#else /* STATIC_JAVA */
    PR_LoadStaticLibrary("libjpeg.so",	NULL);
    PR_LoadStaticLibrary("libnet.so",	NULL);
#endif

#ifdef DEBUG
/*    on_exit(PrintStats, 0); */
#endif
    {   /* assume -cs if executed as "java"
	   rather than "java" [DOES THIS MAKE SENSE ANY MORE??? */
	char *p = &argv[0][strlen(argv[0]) - 1];
	if (p[0] == 'g' && p[-1] == '_')
	    p -= 2;
	if (p[0] != 'k')
	    SkipSourceChecks = 1;
    }

    /* Store the base of the primordial thread's stack */
    mainstktop = (stackp_t) &argc;

    if ((progname = strrchr(argv[0], LOCAL_DIR_SEPARATOR)) != 0) {
	progname++;
    } else {
	progname = argv[0];
    }

	if( argc >= 2 && strcmp(argv[1],"-argfile") == 0 ) {
	  if( argc < 3 ) {
		fprintf(stderr,
			    "-argfile requires argfile specification\n");
		usage();
		return 1;
	  }
	  ret = set_argfile_opts( argc - 2, argv + 2 );
	} else { 
	  ret = start_exec_env( argc, argv );
	}

	return ret;
}

void
Execute(char **command, char *alternate)
{
    long pid, wpid;
    long lapcnt;
    int status = -1;
    if (verbose) {
	printf("[Executing");
	for (lapcnt = 0; command[lapcnt]; lapcnt++)
	    printf(" %s", command[lapcnt]);
	printf("]\n");
    }
    lapcnt = 0;
#ifdef XP_PC	/* rjp */
    sysAssert(0);
#else
    while ((pid = fork()) < 0) {
	if (lapcnt == 0)
	    write(2, "[ Running out of system memory, waiting...", 42);
	lapcnt++;
	sleep(5);
    }
    if (pid) {
	if (lapcnt)
	    write(2, " got it ]\n", 10);
	while ((wpid = wait(&status)) == -1 || wpid != pid);
	if (status != 0) {
	    fprintf(stderr, "%s: failed (%X)\n", command[0], status);
	    sysExit(1);
	}
    } else {
	lapcnt = 0;
	while (1) {
	    execvp(command[0], command);
	    if (alternate)
		execvp(alternate, command);
	    if (errno != ENOMEM) {
		perror(command[0]);
		sysExit(1);
	    }
	    if (lapcnt == 0)
		write(2, "Waiting for system memory...\n", 29);
	    sleep(20);
	}
    }
#endif
    if (verbose)
	printf("[Finished %s]\n", command[0]);
}

static void usage()
{
    fprintf(stderr, "Usage: %s [-options] class\n", progname);
    fprintf(stderr, "\n");
    fprintf(stderr, "where options include:\n");
    fprintf(stderr, "    -help             print out this message\n");
    fprintf(stderr, "    -version          print out the build version\n");
#ifndef trace
    fprintf(stderr, "    -t                turn on instruction tracing\n");
    fprintf(stderr, "    -tm               turn on method tracing\n");
#endif
#ifdef MONITOR
    fprintf(stderr, "    -m                turn on monitoring\n");
#endif
    fprintf(stderr, "    -v -verbose       turn on verbose mode\n");
    fprintf(stderr, "    -debug            enable JAVA debugging\n");
    fprintf(stderr, "    -noasyncgc        don't allow asynchronous gc's\n");
    fprintf(stderr, "    -verbosegc        print a message when GCs occur\n");
    fprintf(stderr, "    -cs -checksource  check if source is newer when loading classes\n");
    fprintf(stderr, "    -ss<number>       set the C stack size of a process\n");
    fprintf(stderr, "    -oss<number>      set the JAVA stack size of a process\n");
    fprintf(stderr, "    -ms<number>       set the initial Java heap size\n");
    fprintf(stderr, "    -mx<number>       set the maximum Java heap size\n");
    fprintf(stderr, "    -D<name>=<value>  set a system property\n");
#ifdef LOGGING
    fprintf(stderr, "    -l<number>        set the logging level\n");
#endif
    fprintf(stderr, "    -classpath <directories separated by colons>\n");
    fprintf(stderr, "                      list directories in which to look for classes\n");
    fprintf(stderr, "    -prof             output profiling data to ./java.prof\n");
    fprintf(stderr, "    -verify           verify all classes when read in\n");
    fprintf(stderr, "    -verifyremote     verify classes read in over "
                                              "the network [default]\n");
    fprintf(stderr, "    -noverify         do not verify any class\n");
}

void fakeref(void)
{
    _java_lang_init();
#if defined(STATIC_JAVA)
    _java_awt_init();
    _java_mmedia_init();
#endif
}
