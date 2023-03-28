/* -*- Mode: C; tab-width: 8 -*-
 *
 * Support for encoding/decoding of ASN.1 using BER/DER (Basic/Distinguished
 * Encoding Rules).  The routines are found in and used extensively by the
 * security library, but exported for other use.
 *
 * Copyright © 1996 Netscape Communications Corporation, all rights reserved.
 *
 * $Id: secasn1.h,v 1.4.2.2 1997/05/12 02:31:05 repka Exp $
 */

#ifndef _SECASN1_H_
#define _SECASN1_H_

#include "prarena.h"

#include "seccomon.h"
#include "secasn1t.h"


/************************************************************************/
SEC_BEGIN_PROTOS

/*
** Decoding.
*/

/* XXX describe it Lisa! */
extern SEC_ASN1DecoderContext *SEC_ASN1DecoderStart(PRArenaPool *pool,
						    void *dest,
						    const SEC_ASN1Template *t);

/* XXX describe it Lisa! */
/* XXX char or unsigned char? */
extern SECStatus SEC_ASN1DecoderUpdate(SEC_ASN1DecoderContext *cx,
				       const char *buf,
				       unsigned long len);

/* XXX describe it Lisa! */
extern SECStatus SEC_ASN1DecoderFinish(SEC_ASN1DecoderContext *cx);

/* XXX describe it Lisa! */
extern void SEC_ASN1DecoderSetFilterProc(SEC_ASN1DecoderContext *cx,
					 SEC_ASN1WriteProc fn,
					 void *arg, PRBool no_store);

/* XXX describe it Lisa! */
extern void SEC_ASN1DecoderClearFilterProc(SEC_ASN1DecoderContext *cx);

/* XXX describe it Lisa! */
extern void SEC_ASN1DecoderSetNotifyProc(SEC_ASN1DecoderContext *cx,
					 SEC_ASN1NotifyProc fn,
					 void *arg);

/* XXX describe it Lisa! */
extern void SEC_ASN1DecoderClearNotifyProc(SEC_ASN1DecoderContext *cx);

/* XXX describe it Lisa! */
extern SECStatus SEC_ASN1Decode(PRArenaPool *pool, void *dest,
				const SEC_ASN1Template *t,
				const char *buf, long len);

/* XXX describe it Lisa! */
extern SECStatus SEC_ASN1DecodeItem(PRArenaPool *pool, void *dest,
				    const SEC_ASN1Template *t,
				    SECItem *item);
/*
** Encoding.
*/

/* XXX describe it Lisa! */
extern SEC_ASN1EncoderContext *SEC_ASN1EncoderStart(void *src,
						    const SEC_ASN1Template *t,
						    SEC_ASN1WriteProc fn,
						    void *output_arg);

/* XXX describe it Lisa! */
/* XXX char or unsigned char? */
extern SECStatus SEC_ASN1EncoderUpdate(SEC_ASN1EncoderContext *cx,
				       const char *buf,
				       unsigned long len);

/* XXX describe it Lisa! */
extern void SEC_ASN1EncoderFinish(SEC_ASN1EncoderContext *cx);

/* XXX describe it Lisa! */
extern void SEC_ASN1EncoderSetNotifyProc(SEC_ASN1EncoderContext *cx,
					 SEC_ASN1NotifyProc fn,
					 void *arg);

/* XXX describe it Lisa! */
extern void SEC_ASN1EncoderClearNotifyProc(SEC_ASN1EncoderContext *cx);

/* XXX describe it Lisa! */
extern void SEC_ASN1EncoderSetStreaming(SEC_ASN1EncoderContext *cx);

/* XXX describe it Lisa! */
extern void SEC_ASN1EncoderClearStreaming(SEC_ASN1EncoderContext *cx);

/* XXX describe it Lisa! */
extern void SEC_ASN1EncoderSetTakeFromBuf(SEC_ASN1EncoderContext *cx);

/* XXX describe it Lisa! */
extern void SEC_ASN1EncoderClearTakeFromBuf(SEC_ASN1EncoderContext *cx);

/* XXX describe it Lisa! */
extern SECStatus SEC_ASN1Encode(void *src, const SEC_ASN1Template *t,
				SEC_ASN1WriteProc output_proc,
				void *output_arg);

/* XXX describe it Lisa! */
extern SECItem * SEC_ASN1EncodeItem(PRArenaPool *pool, SECItem *dest,
				    void *src, const SEC_ASN1Template *t);

/* XXX describe it Lisa! */
extern SECItem * SEC_ASN1EncodeInteger(PRArenaPool *pool,
				       SECItem *dest, long value);

/* XXX describe it Lisa! */
extern SECItem * SEC_ASN1EncodeUnsignedInteger(PRArenaPool *pool,
					       SECItem *dest,
					       unsigned long value);

extern SECItem * SEC_ASN1EncodeEnumerated(PRArenaPool *pool,
				         SECItem *dest, long value);

/*
** Utilities.
*/

/*
 * We have a length that needs to be encoded; how many bytes will the
 * encoding take?
 */
extern int SEC_ASN1LengthLength (unsigned long len);

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
extern const SEC_ASN1Template *
SEC_ASN1GetSubtemplate (const SEC_ASN1Template *template, void *thing,
			PRBool encoding);

SEC_END_PROTOS
/************************************************************************/

/*
 * Generic Templates
 * One for each of the simple types, plus a special one for ANY, plus:
 *	- a pointer to each one of those
 *	- a set of each one of those
 */

extern const SEC_ASN1Template SEC_AnyTemplate[];
extern const SEC_ASN1Template SEC_BitStringTemplate[];
extern const SEC_ASN1Template SEC_BooleanTemplate[];
extern const SEC_ASN1Template SEC_IA5StringTemplate[];
extern const SEC_ASN1Template SEC_IntegerTemplate[];
extern const SEC_ASN1Template SEC_EnumeratedTemplate[];
extern const SEC_ASN1Template SEC_NullTemplate[];
extern const SEC_ASN1Template SEC_ObjectIDTemplate[];
extern const SEC_ASN1Template SEC_OctetStringTemplate[];
extern const SEC_ASN1Template SEC_PrintableStringTemplate[];
extern const SEC_ASN1Template SEC_T61StringTemplate[];
extern const SEC_ASN1Template SEC_UTCTimeTemplate[];
extern const SEC_ASN1Template SEC_GeneralizedTimeTemplate[];

extern const SEC_ASN1Template SEC_PointerToAnyTemplate[];
extern const SEC_ASN1Template SEC_PointerToBitStringTemplate[];
extern const SEC_ASN1Template SEC_PointerToBooleanTemplate[];
extern const SEC_ASN1Template SEC_PointerToIA5StringTemplate[];
extern const SEC_ASN1Template SEC_PointerToIntegerTemplate[];
extern const SEC_ASN1Template SEC_PointerToNullTemplate[];
extern const SEC_ASN1Template SEC_PointerToObjectIDTemplate[];
extern const SEC_ASN1Template SEC_PointerToOctetStringTemplate[];
extern const SEC_ASN1Template SEC_PointerToPrintableStringTemplate[];
extern const SEC_ASN1Template SEC_PointerToT61StringTemplate[];
extern const SEC_ASN1Template SEC_PointerToUTCTimeTemplate[];

extern const SEC_ASN1Template SEC_SetOfAnyTemplate[];
extern const SEC_ASN1Template SEC_SetOfBitStringTemplate[];
extern const SEC_ASN1Template SEC_SetOfBooleanTemplate[];
extern const SEC_ASN1Template SEC_SetOfIA5StringTemplate[];
extern const SEC_ASN1Template SEC_SetOfIntegerTemplate[];
extern const SEC_ASN1Template SEC_SetOfNullTemplate[];
extern const SEC_ASN1Template SEC_SetOfObjectIDTemplate[];
extern const SEC_ASN1Template SEC_SetOfOctetStringTemplate[];
extern const SEC_ASN1Template SEC_SetOfPrintableStringTemplate[];
extern const SEC_ASN1Template SEC_SetOfT61StringTemplate[];
extern const SEC_ASN1Template SEC_SetOfUTCTimeTemplate[];
extern const SEC_ASN1Template SEC_BMPStringTemplate[];

/*
 * Template for skipping a subitem; this only makes sense when decoding.
 */
extern const SEC_ASN1Template SEC_SkipTemplate[];


#endif /* _SECASN1_H_ */
