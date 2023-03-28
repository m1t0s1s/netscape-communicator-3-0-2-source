/*
 * @(#)runtime_stubs.c	1.11 95/08/27  
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
 * This file has a number of stub routines that are compiled into
 * programs that make calls to the thread library, but do not
 * actually do any multi threading.
 */

#include <sys_api.h>

int
sysMonitorEnter(sys_mon_t *m)
{
    return SYS_ERR;
}

int
sysMonitorExit(sys_mon_t *m)
{
    return SYS_ERR;
}

int
sysMonitorInit(sys_mon_t *m, bool_t in_cache)
{
    return SYS_ERR;
}

void
monitorEnter(unsigned int key)
{
}

void
monitorExit(unsigned int key)
{
}

int
monitorRegister(sys_mon_t *mid, char *name)
{
    return SYS_ERR;
}


