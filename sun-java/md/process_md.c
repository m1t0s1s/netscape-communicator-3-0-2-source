/*
 * @(#)process_md.c	1.5 95/08/03  
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

#include <stdlib.h>
#include <fcntl.h>
#include "standardlib.h"
#include <sys/types.h>
 
#include "oobj.h"
#include "tree.h"
#include "interpreter.h"
#include "javaString.h"
#include "typecodes.h"
#include "sys_api.h"
#include "monitor.h"
#include "monitor_md.h"

#include "java_io_FileInputStream.h"
#include "java_io_FileDescriptor.h"
#include "java_lang_Process.h"
#include "java_lang_Runtime.h"
#include <errno.h>
 
#if defined(XP_UNIX) && defined(BREAKPTS)
#include <wait.h>
#include <signal.h>

#include <java_lang_UNIXProcess.h>
#include <java_lang_ProcessReaper.h>

extern PRMonitor *_pr_child_intr;

/*
 * This routine is called once from the Process Reaper thread and
 * never returns.  It's job is to catch sigchld events and pass them
 * on to class UNIXProcess.
 */
void
java_lang_ProcessReaper_waitForDeath(Hjava_lang_ProcessReaper *this) 
{
    siginfo_t info;
    char *classname = JAVAPKG "UNIXProcess";
    ClassClass *cb = FindClass(0, classname, TRUE);

    if (cb == NULL) {
	SignalError(0, JAVAPKG "NoClassDefFoundError", classname);
	return;
    }

    while (1) {
	PR_EnterMonitor(_pr_child_intr);
	while (waitid(P_ALL, 0, &info, WEXITED|WNOHANG) != 0 || info.si_pid == 0) {
	    PR_Wait(_pr_child_intr, LL_MAXINT);
	}
	fprintf(stderr, "pid %d status %d\n", info.si_pid, info.si_status);
	execute_java_static_method(EE(), cb, "deadChild", "(II)V",
				   info.si_pid, info.si_status);
	PR_ExitMonitor(_pr_child_intr);
    }
}


long
java_lang_UNIXProcess_waitForUNIXProcess(Hjava_lang_UNIXProcess *this)
{
    siginfo_t info;

    waitid(P_PID, unhand(this)->pid, &info, WEXITED);
    return 1;
}


/* XXX make this race proof! */

long
java_lang_UNIXProcess_fork(Hjava_lang_UNIXProcess *this) 
{
    int pid = 0;
    int fdin[2], fdout[2], fderr[2], fdsync[2];
    Classjava_lang_UNIXProcess *thisptr = unhand(this);
    Classjava_io_FileDescriptor *infdptr = unhand(thisptr->stdin_fd);
    Classjava_io_FileDescriptor *outfdptr = unhand(thisptr->stdout_fd);
    Classjava_io_FileDescriptor *errfdptr = unhand(thisptr->stderr_fd);
    Classjava_io_FileDescriptor *syncfdptr = unhand(thisptr->sync_fd);

    pipe(fdin);  /* stdin */
    pipe(fdout);  /* stdout */
    pipe(fderr);  /* stderr */
    pipe(fdsync);  /* fd for synchronization */

/*    sysThreadSingle(); */
    if ((pid = fork()) == 0) {
int foo = open("/tmp/child-log", O_RDWR|O_CREAT, 0644);
char bfoo[200];
if (foo < 0) abort();
	/* Child process */
	dup2(fdin[0], 0);
	dup2(fdout[1], 1);
	dup2(fderr[1], 2);

	close(fdin[0]);
	close(fdin[1]);
	close(fdout[0]);
	close(fdout[1]);
	close(fderr[0]);
	close(fderr[1]);
sprintf(bfoo, "here #1\n"); write(foo, bfoo, strlen(bfoo));
	{ int err;
	    char c;
	    err = sysRead(fdsync[0], &c, 1);
sprintf(bfoo, "fdsync returns %d (errno=%d)\n", err, errno); write(foo, bfoo, strlen(bfoo));
if (err < 0) abort();
	    fprintf(stderr, "err = %d!\n", err);
sprintf(bfoo, "here #2 =>%d\n", (int)c); write(foo, bfoo, strlen(bfoo));
	}
    } else {
	/* parent process */
/*	sysThreadMulti(); */
	infdptr->fd = fdin[1]+1;
	outfdptr->fd = fdout[0]+1;
	errfdptr->fd = fderr[0]+1;
	syncfdptr->fd = fdsync[1]+1;

	close(fdin[0]);
	close(fdout[1]);
	close(fderr[1]);
	close(fdsync[0]);
    }
    return pid;
}

void
java_lang_UNIXProcess_exec(struct Hjava_lang_UNIXProcess *this,
			   HArrayOfString *cmdarray,
			   HArrayOfString *envarray)
{
    char *cmdstr[200];	    /* no more than 198 arguments!! */
    int len;

    len = obj_length(cmdarray);
    if (len == 0 || len > 198) {
	SignalError(0, JAVAPKG "IllegalArgumentException", 0);
	return;
    }
#define BODYOF(h)   unhand(h)->body
    cmdstr[len] = 0;
    while (len--) {
	cmdstr[len] = allocCString(BODYOF(cmdarray)[len]);
    }
    fprintf(stderr, "Execing: %s\n", cmdstr[0]);
    execv(cmdstr[0], cmdstr);
    fprintf(stderr, "exec of \"%s\" failed with errno %d\n", cmdstr[0], errno);
    abort();
}

Hjava_lang_Process *
java_lang_Runtime_execInternal(Hjava_lang_Runtime *this, 
			       HArrayOfString *cmdarray,
			       HArrayOfString *envarray)
{
    Hjava_lang_Process *proc = 0;

    if (cmdarray == 0) {
	SignalError(0, JAVAPKG "NullPointerException", 0);
	return 0;
    }
	
    /*
	We need a way to know which subclass of Process we want to create.
    */

    proc = (Hjava_lang_Process *) execute_java_constructor(EE(),
				  JAVAPKG "UNIXProcess",
				  (ClassClass *) 0, 
				  "([Ljava/lang/String;[Ljava/lang/String;)", 
				  cmdarray, envarray);
    return proc;
}
 
void
java_lang_UNIXProcess_destroy(Hjava_lang_UNIXProcess *this)
{
    kill(unhand(this)->pid, SIGTERM);
}

#else

Hjava_lang_Process *
java_lang_Runtime_execInternal(Hjava_lang_Runtime *this, 
			       HArrayOfString *cmdarray,
			       HArrayOfString *envarray)
{
    if (cmdarray == 0) {
	SignalError(0, JAVAPKG "NullPointerException", 0);
	return 0;
    }
    SignalError(0, JAVAPKG "SecurityException", "exec is not allowed");
    return 0;
}

#endif /* XP_UNIX && BREAKPTS */
