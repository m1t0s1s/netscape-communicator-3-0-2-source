#ifdef BREAKPTS
/*
 * @(#)agentthread.c	1.8 96/02/08 Thomas Ball
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
 *	Solaris-specific routines to support the debugging agent.  
 */

#include <oobj.h>
#include <javaThreads.h>
#include <threads_md.h>
#include <opcodes.h>

#include "sun_tools_debug_Agent.h"
#include "sun_tools_debug_AgentConstants.h"

#ifdef DEBUG
extern int dprintf(char *fmt, ...);

extern int be_verbose;

void setDebugState(void)
{
    static int fInitialized = 0;

    if (!fInitialized) {
#ifdef XP_WIN
	if (getenv("ENTER_DBX") != NULL) {
            DebugBreak();
        }
#else /* !XP_WIN */
	/* Setup our debugging environment. */
	if (getenv("DEBUG") != NULL) {
	    (void)freopen("/dev/console", "w", stderr);
	    be_verbose++;
	}

	/* Load dbx and wait.  The developer must set fContinueAgent 
	 * to continue. */
	if (getenv("ENTER_DBX") != NULL) {
	    char cmd[80];
	    static int fContinueAgent = 0;

	    dprintf("Starting debugger ... \n");
	    sprintf(cmd, "debugger - %d &", getpid());
	    system(cmd);
	    dprintf("Set fContinueAgent to non-zero to continue.\n");
	    while (fContinueAgent == 0) {
		sleep(1);
	    }
	}
#endif /* !XP_WIN */
	fInitialized = 1;
    }
}
#else
void setDebugState(void) {}
#endif

long
sun_tools_debug_Agent_getThreadStatus(Hsun_tools_debug_Agent *this, 
				       Hjava_lang_Thread *h)
{
    ExecEnv* thread_ee;

#ifdef DEBUG
    setDebugState();
#endif

    if (h == 0)
	return sun_tools_debug_AgentConstants_THR_STATUS_UNKNOWN;

    if (SYSTHREAD(h) == 0)
	return sun_tools_debug_AgentConstants_THR_STATUS_ZOMBIE;

    switch (SYSTHREAD(h)->state) {
      case _PR_UNBORN:
      case _PR_ZOMBIE:
	return sun_tools_debug_AgentConstants_THR_STATUS_ZOMBIE;

      case _PR_RUNNABLE:
      case _PR_RUNNING:
	return sun_tools_debug_AgentConstants_THR_STATUS_RUNNING;

      case _PR_SUSPENDED:
	thread_ee = (ExecEnv *)unhand(h)->eetop;
	if ((thread_ee != 0) && (thread_ee->current_frame != 0) &&
            (thread_ee->current_frame->lastpc != 0) &&
	    (*thread_ee->current_frame->lastpc == opc_breakpoint)) {
	    return sun_tools_debug_AgentConstants_THR_STATUS_BREAK;
	}

	return sun_tools_debug_AgentConstants_THR_STATUS_SUSPENDED;

      case _PR_SLEEPING:
	return sun_tools_debug_AgentConstants_THR_STATUS_SLEEPING;

      case _PR_MON_WAIT:
	if (SYSTHREAD(h)->flags & _PR_SUSPENDING)
	    return sun_tools_debug_AgentConstants_THR_STATUS_SUSPENDED;

	return sun_tools_debug_AgentConstants_THR_STATUS_MONWAIT;

      case _PR_COND_WAIT:
	if (SYSTHREAD(h)->flags & _PR_SUSPENDING)
	    return sun_tools_debug_AgentConstants_THR_STATUS_SUSPENDED;

	return sun_tools_debug_AgentConstants_THR_STATUS_CONDWAIT;

      default:
	return sun_tools_debug_AgentConstants_THR_STATUS_UNKNOWN;
    }
}

#endif /* BREAKPTS */
