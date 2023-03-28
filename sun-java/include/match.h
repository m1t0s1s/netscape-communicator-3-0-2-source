/*
 * @(#)match.h	1.4 95/01/31 James Gosling
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

/*
 * Definitions having to do with pattern matching
 */

#ifndef _MATCH_H_
#define _MATCH_H_

#ifndef _TREE_
#include <tree.h>
#endif

struct template {
    struct node *left, *right;
    struct node *replacement;
    struct template **backlink;
    struct template *sameset;
    struct template *next;
    int flags;
    char replaceafter;
};

struct ruleset {
    struct template *templates;
    struct ruleset *prev;
};

struct ruleset *thisruleset;

struct template *templates[N_OPS];

struct match_state {
    int mask;
    struct node *vars[10];
};

#endif /* !_MATCH_H_ */
