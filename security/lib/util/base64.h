#ifndef _BASE64_H_
#define _BASE64_H_
/*
 * base64.h - public data structures and prototypes for base64
 * encoding/decoding
 *
 * $Id: base64.h,v 1.2 1996/10/14 10:04:24 jsw Exp $
 */
#include "seccomon.h"

typedef struct ATOBContextStr ATOBContext;
typedef struct BTOAContextStr BTOAContext;

SEC_BEGIN_PROTOS

/************************************************************************/
/*
** RFC1113 (base64) ascii to binary decoding
*/

/*
** Create a new ascii-to-binary (ATOB) context
*/
extern ATOBContext *ATOB_NewContext(void);

/*
** Destroy an ATOB context.
**	"cx" the context to destroy
*/
extern void ATOB_DestroyContext(ATOBContext *cx);

/*
** Reset an ATOB context, preparing it for a fresh round of decoding.
*/
extern SECStatus ATOB_Begin(ATOBContext *cx);

/*
** Update the ATOB decoder.
**	"cx" the context
**	"output" the output buffer to store the decoded data.
**	"outputLen" how much data is stored in "output". Set by the routine
**	   after some data is stored in output.
**	"maxOutputLen" the maximum amount of data that can ever be
**	   stored in "output"
**	"input" the ascii input data (in RFC1113 format)
**	"inputLen" the amount of input data
*/
extern SECStatus ATOB_Update(ATOBContext *cx, unsigned char *output,
			    unsigned int *outputLen, unsigned int maxOutputLen,
			    const unsigned char *input, unsigned int inputLen);

/*
** End the ATOB decoding process. This flushes any lingering binary data
** out to the output buffer.
**	"cx" the context
**	"output" the output buffer to store the decoded data.
**	"outputLen" how much data is stored in "output". Set by the routine
**	   after some data is stored in output.
**	"maxOutputLen" the maximum amount of data that can ever be
**	   stored in "output"
*/
extern SECStatus ATOB_End(ATOBContext *cx, unsigned char *output,
			 unsigned int *outputLen, unsigned int maxOutputLen);

/******************************************/
/*
** RFC1113 binary to ascii encoding (base64 encoding)
*/

/*
** Create a new binary-to-ascii (BTOA) context for encoding.
*/
extern BTOAContext *BTOA_NewContext(void);

/*
** Destroy a BTOA context.
**	"cx" the context to destroy
*/
extern void BTOA_DestroyContext(BTOAContext *cx);

/*
** Reset a BTOA context, preparing it for a fresh round of encoding.
*/
extern SECStatus BTOA_Begin(BTOAContext *cx);

/*
** Update the BTOA encoder.
**	"cx" the context
**	"output" the output buffer to store the RFC1113 encoded data.
**	"outputLen" how much data is stored in "output". Set by the routine
**	   after some data is stored in output.
**	"maxOutputLen" the maximum amount of data that can ever be
**	   stored in "output"
**	"input" the binary input data
**	"inputLen" the amount of input data
*/
extern SECStatus BTOA_Update(BTOAContext *cx, unsigned char *output,
			    unsigned int *outputLen, unsigned int maxOutputLen,
			    const unsigned char *input, unsigned int inputLen);

/*
** End the BTOA encoding process. This flushes any lingering encoded data
** out to the output buffer.
**	"cx" the context
**	"output" the output buffer to store the encoded data.
**	"outputLen" how much data is stored in "output". Set by the routine
**	   after some data is stored in output.
**	"maxOutputLen" the maximum amount of data that can ever be
**	   stored in "output"
*/
extern SECStatus BTOA_End(BTOAContext *cx, unsigned char *output,
			 unsigned int *outputLen, unsigned int maxOutputLen);

/*
** Return an estimate of how much ascii buffer space is necessary to
** RFC1113 encode given a specified amount of binary data.
*/
extern int BTOA_GetLength(int binaryLength);

/*
** Return an PORT_Alloc'd ascii string which is the base64 encoded
** version of the input string.
*/
extern char *BTOA_DataToAscii(const unsigned char *data, unsigned int len);

/*
** Return an PORT_Alloc'd string which is the base64 decoded version
** of the input string; set *lenp to the length of the returned data.
*/
extern unsigned char *ATOB_AsciiToData(const char *string, unsigned int *lenp);
 
/*
** Convert from ascii to binary encoding of an item.
*/
extern SECStatus ATOB_ConvertAsciiToItem(SECItem *binary_item, char *ascii);

/*
** Convert from binary encoding of an item to ascii.
*/
extern char *BTOA_ConvertItemToAscii(SECItem *binary_item);

SEC_END_PROTOS

#endif /* _BASE64_H_ */
