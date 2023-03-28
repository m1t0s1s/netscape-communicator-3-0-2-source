/* -*- Mode: C; tab-width: 8 -*-
 *
 * Utility routines to complement the ASN.1 encoding and decoding functions.
 *
 * Copyright © 1996 Netscape Communications Corporation, all rights reserved.
 *
 * $Id: secasn1u.c,v 1.4 1996/11/26 16:52:19 repka Exp $
 */

#include "secasn1.h"


/*
 * We have a length that needs to be encoded; how many bytes will the
 * encoding take?
 *
 * The rules are that 0 - 0x7f takes one byte (the length itself is the
 * entire encoding); everything else takes one plus the number of bytes
 * in the length.
 */
int
SEC_ASN1LengthLength (unsigned long len)
{
    int lenlen = 1;

    if (len > 0x7f) {
	do {
	    lenlen++;
	    len >>= 8;
	} while (len);
    }

    return lenlen;
}


/*
 * XXX Move over (and rewrite as appropriate) the rest of the
 * stuff in dersubr.c!
 */


/*
 * Find the appropriate subtemplate for the given template.
 * This may involve calling a "chooser" function, or it may just
 * be right there.  In either case, it is expected to *have* a
 * subtemplate; this is asserted in debug builds (in non-debug
 * builds, NULL will be returned).
 *
 * "thing" is a pointer to the structure being encoded/decoded
 * "encoding", when true, means that we are in the process of encoding
 *	(as opposed to in the process of decoding)
 */
const SEC_ASN1Template *
SEC_ASN1GetSubtemplate (const SEC_ASN1Template *template, void *thing,
			PRBool encoding)
{
    const SEC_ASN1Template *subt;

    PORT_Assert (template->sub != NULL);
    if (template->kind & SEC_ASN1_DYNAMIC) {
	SEC_ChooseASN1TemplateFunc chooser, *chooserp;

	chooserp = (SEC_ChooseASN1TemplateFunc *) template->sub;
	if (chooserp == NULL || *chooserp == NULL)
	    return NULL;
	chooser = *chooserp;
	if (thing != NULL)
	    thing = (char *)thing - template->offset;
	subt = (* chooser)(thing, encoding);
    } else {
	subt = template->sub;
    }

    return subt;
}
