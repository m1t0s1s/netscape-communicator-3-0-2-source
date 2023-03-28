/*
 * @(#)profiler.c	1.17 95/11/29  
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
 *      code for collecting profiling statistics on Java code.
 */

#include <ctype.h>
#include <string.h>
#include <sys/types.h>
#include <stdlib.h>

#include "timeval.h"
#include "oobj.h"
#include "interpreter.h"
#include "sys_api.h"

typedef struct {
    struct methodblock *caller, *callee;
    int32_t count;
    int32_t time;
} javamon_entry;


#define JAVAMON_TABLE_SIZE   10001
static javamon_entry *javamon_table;
static int javamon_table_count;
int java_monitor = 0;

void javamon(int i)
{
    java_monitor = i;
    if (java_monitor) {
	javamon_table = (javamon_entry *)sysMalloc(JAVAMON_TABLE_SIZE * sizeof(javamon_entry));
	memset((char *)javamon_table, 0, JAVAMON_TABLE_SIZE * sizeof(javamon_entry));
	javamon_table_count = 0;
    }
}

void
java_mon(struct methodblock *caller, struct methodblock *callee, int32_t time)
{
    javamon_entry *tab = javamon_table;
    javamon_entry *p = &tab[((((unsigned long)caller)^((unsigned long)callee)) >> 2) % JAVAMON_TABLE_SIZE];

    while ((p->callee != 0) && ((p->caller != caller) || (p->callee != callee))) {
	if (p-- == tab) {
	    p = &tab[JAVAMON_TABLE_SIZE - 1];
	}
    }
    if (p->callee == 0) {
	if (++javamon_table_count == JAVAMON_TABLE_SIZE) {
	    fprintf(stderr, "profile table overflow");
	    sysExit(1);
	}
	p->caller = caller;
	p->callee = callee;
    }
    p->time += time;
    p->count++;
}

static void
java_mon_dump_to_file(FILE *fp)
{
    javamon_entry *tab = javamon_table;
    javamon_entry *p = &tab[JAVAMON_TABLE_SIZE];

    fprintf(fp, "# count callee caller time\n");
    while (p-- != tab) {
	if (p->callee != 0) {
	    fprintf(fp, "%d ", p->count);
	    if (p->callee == (struct methodblock *)-1) {
		fprintf(fp, "java/lang/System.gc()V ");
	    } else if ((strcmp(classname(fieldclass(&p->callee->fb)), "java/lang/System") != 0) ||
		       (strcmp(fieldname(&p->callee->fb), "gc") != 0)) {
		fprintf(fp, "%s.%s%s ", classname(fieldclass(&p->callee->fb)), 
			fieldname(&p->callee->fb),
			fieldsig(&p->callee->fb));
	    }
	    if (p->caller != 0) {
		fprintf(fp, "%s.%s%s ", classname(fieldclass(&p->caller->fb)), 
					fieldname(&p->caller->fb),
		    			fieldsig(&p->caller->fb));
	    } else {
		fprintf(fp, "?.? ");
	    }
	    fprintf(fp, "%d\n", p->time);
	}
    }
}

void
java_mon_dump()
{
    extern void prof_heap(FILE *fp);
    FILE *fp = fopen("java.prof", "w");

    if (fp == 0) {
	fprintf(stderr, "Can't write to java.prof\n");
	return;
    }

    java_mon_dump_to_file(fp);
    prof_heap(fp);
    fclose(fp);
}
