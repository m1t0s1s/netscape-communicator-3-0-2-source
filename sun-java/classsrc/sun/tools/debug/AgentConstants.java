/*
 * @(#)AgentConstants.java	1.22 96/01/31
 *
 * Copyright (c) 1995 Sun Microsystems, Inc. All Rights Reserved.
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

package sun.tools.debug;
import sun.tools.java.Constants;

interface AgentConstants extends Constants {
    int AGENT_VERSION		= 3;

    int TP_THREADGROUP		= 15;
    int TP_CLASS     		= 16;
    int TP_OBJECT		= 17;
    int TP_STRING		= 18;
    int TP_THREAD		= 19;

    int CMD_GET_CLASS_INFO	= 20;
    int CMD_GET_THREAD_NAME	= 21;
    int CMD_GET_CLASS_BY_NAME	= 22;
    int CMD_MARK_OBJECTS	= 23;
    int CMD_FREE_OBJECTS	= 24;
    int CMD_LIST_THREADS	= 25;
    int CMD_RUN			= 26;
    int CMD_SUSPEND_THREAD	= 27;
    int CMD_RESUME_THREAD	= 28;
    int CMD_GET_THREAD_STATUS   = 31;
    int CMD_GET_STACK_TRACE	= 32;
    int CMD_WRITE_BYTES		= 33;
    int CMD_SYSTEM		= 34;
    int CMD_GET_FIELDS		= 35;
    int CMD_GET_METHODS		= 36;
    int CMD_GET_VALUES		= 37;
    int CMD_GET_SLOT_SIGNATURE 	= 38;
    int CMD_GET_SLOT_VALUE 	= 39;
    int CMD_GET_ELEMENTS	= 40;
    int CMD_SET_BRKPT_LINE	= 41;
    int CMD_SET_BRKPT_METHOD	= 42;
    int CMD_CLEAR_BRKPT		= 43;
    int CMD_CLEAR_BRKPT_LINE	= 44;
    int CMD_CLEAR_BRKPT_METHOD	= 45;
    int CMD_BRKPT_NOTIFY	= 46;
    int CMD_LIST_THREADGROUPS	= 47;
    int CMD_GET_THREADGROUP_INFO= 48;
    int CMD_LIST_CLASSES	= 49;
    int CMD_GET_STACK_VALUE	= 50;
    int CMD_SET_VERBOSE		= 51;
    int CMD_EXCEPTION_NOTIFY	= 52;
    int CMD_CATCH_EXCEPTION_CLS = 53;
    int CMD_IGNORE_EXCEPTION_CLS= 54;
    int CMD_GET_CATCH_LIST	= 55;
    int CMD_STOP_THREAD		= 56;
    int CMD_STOP_THREAD_GROUP	= 57;
    int CMD_QUIT		= 58;
    int CMD_GET_SOURCE_FILE	= 59;
    int CMD_OBJECT_TO_STRING	= 60;
    int CMD_GET_SOURCEPATH	= 61;
    int CMD_SET_SOURCEPATH	= 62;
    int CMD_STEP_THREAD		= 63;
    int CMD_NEXT_STEP_THREAD	= 64;
    int CMD_LIST_BREAKPOINTS	= 65;
    int CMD_THREADDEATH_NOTIFY	= 66;
    int CMD_QUIT_NOTIFY		= 67;
    int CMD_SET_SLOT_VALUE	= 68;
    int CMD_SET_STACK_VALUE	= 69;

    int CMD_EXCEPTION		= -2;	// Exception occurred during cmd

    /* Thread statuses */
    int THR_STATUS_UNKNOWN	= -1;
    int THR_STATUS_ZOMBIE	= 0;
    int THR_STATUS_RUNNING	= 1;
    int THR_STATUS_SLEEPING	= 2;
    int THR_STATUS_MONWAIT	= 3;
    int THR_STATUS_CONDWAIT	= 4;
    int THR_STATUS_SUSPENDED	= 5;
    int THR_STATUS_BREAK	= 6;

    /* System commands */
    int SYS_MEMORY_TOTAL	= 1;
    int SYS_MEMORY_FREE		= 2;
    int SYS_TRACE_METHOD_CALLS	= 3;
    int SYS_TRACE_INSTRUCTIONS	= 4;

    /* Breakpoint types */
    int BKPT_NORMAL		= 1;
    int BKPT_ONESHOT		= 2;
    int BKPT_CONDITIONAL	= 3;

    /* Valid password characters:
     * [0-9][a-z] minus zero, one, O and L to reduce mistakes.
     */
    String PSWDCHARS = "23456789abcdefghijkmnpqrstuvwxyz";
}
