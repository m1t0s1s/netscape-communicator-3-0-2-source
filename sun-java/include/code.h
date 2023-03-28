/*
 * @(#)code.h	1.6 95/01/31  
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
 * Definitions having to do with code generation
 */

#ifndef _CODE_H_
#define _CODE_H_

#include "oobj.h"

extern unsigned char *codebuf;
extern int         pc;
extern char       *binfilename;
extern int         reachable;
extern char        MustBranch[256];
struct typeinfo;
char       *MakeSignature(struct typeinfo *, char *);
void DecodeFile(char *);
void PrintCodeSequence(struct methodblock *mb);
void PrintClassSequence(ClassClass *);

extern struct methodblock **procblocks;

#define label() pc
#define ComesHere(fixup) GoesThere(fixup, pc)

/* field flags which share the type code byte in field bytecodes */
#include "typecodes.h"

#define	FIELD_STATIC	0x40
#define	FIELD_GET	0x00
#define	FIELD_PUT	0x80

#if N_TYPEMASK & (FIELD_STATIC | FIELD_PUT)
#error "field flags and type codes overlap"
#endif

#endif	/* !_CODE_H_ */
