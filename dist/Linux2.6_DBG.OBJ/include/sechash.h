#ifndef _HASH_H_
#define _HASH_H_
/*
 * hash.h - public data structures and prototypes for the hashing library
 *
 * $Id: sechash.h,v 1.2 1996/11/26 16:51:38 repka Exp $
 */

#include "seccomon.h"
#include "hasht.h"

SEC_BEGIN_PROTOS

/******************************************/
/*
** MD5 secure hash function
*/

/*
** Hash a null terminated string "src" into "dest" using MD5
*/
extern SECStatus MD5_Hash(unsigned char *dest, const char *src);

/*
** Hash a non-null terminated string "src" into "dest" using MD5
*/
extern SECStatus MD5_HashBuf(unsigned char *dest, const unsigned char *src,
			     uint32 src_length);

/*
** Create a new MD5 context
*/
extern MD5Context *MD5_NewContext(void);

/*
** Clone a copy of the MD5 state. This is useful when you need to keep
** a running hash of something and and need to extract what would be
** the end values part way through.
*/
extern MD5Context *MD5_CloneContext(MD5Context *cx);

/*
** Destroy an MD5 secure hash context.
**	"cx" the context
**	"freeit" if PR_TRUE then free the object as well as its sub-objects
*/
extern void MD5_DestroyContext(MD5Context *cx, PRBool freeit);

/*
** Reset an MD5 context, preparing it for a fresh round of hashing
*/
extern void MD5_Begin(MD5Context *cx);

/*
** Update the MD5 hash function with more data.
**	"cx" the context
**	"input" the data to hash
**	"inputLen" the amount of data to hash
*/
extern void MD5_Update(MD5Context *cx,
		       const unsigned char *input, unsigned int inputLen);

/*
** Finish the MD5 hash function. Produce the digested results in "digest"
**	"cx" the context
**	"digest" where the 16 bytes of digest data are stored
**	"digestLen" where the digest length (16) is stored
**	"maxDigestLen" the maximum amount of data that can ever be
**	   stored in "digest"
*/
extern void MD5_End(MD5Context *cx, unsigned char *digest,
		    unsigned int *digestLen, unsigned int maxDigestLen);


/******************************************/
/*
** MD2 secure hash function
*/

/*
** Hash a null terminated string "src" into "dest" using MD2
*/
extern SECStatus MD2_Hash(unsigned char *dest, const char *src);

/*
** Create a new MD2 context
*/
extern MD2Context *MD2_NewContext(void);

/*
** Clone a copy of the MD2 state.  This is useful when you need to keep
** a running hash of something and and need to extract what would be
** the end values part way through.
*/
extern MD2Context *MD2_CloneContext(MD2Context *cx);

/*
** Destroy an MD2 secure hash context.
**	"cx" the context
**	"freeit" if PR_TRUE then free the object as well as its sub-objects
*/
extern void MD2_DestroyContext(MD2Context *cx, PRBool freeit);

/*
** Reset an MD2 context, preparing it for a fresh round of hashing
*/
extern void MD2_Begin(MD2Context *cx);

/*
** Update the MD2 hash function with more data.
**	"cx" the context
**	"input" the data to hash
**	"inputLen" the amount of data to hash
*/
extern void MD2_Update(MD2Context *cx,
		       const unsigned char *input, unsigned int inputLen);

/*
** Finish the MD2 hash function. Produce the digested results in "digest"
**	"cx" the context
**	"digest" where the 16 bytes of digest data are stored
**	"digestLen" where the digest length (16) is stored
**	"maxDigestLen" the maximum amount of data that can ever be
**	   stored in "digest"
*/
extern void MD2_End(MD2Context *cx, unsigned char *digest,
		    unsigned int *digestLen, unsigned int maxDigestLen);

/******************************************/
/*
** SHA-1 secure hash function
*/

/*
** Hash a null terminated string "src" into "dest" using SHA-1
*/
extern SECStatus SHA1_Hash(unsigned char *dest, const char *src);

/*
** Hash a non-null terminated string "src" into "dest" using SHA-1
*/
extern SECStatus SHA1_HashBuf(unsigned char *dest, const unsigned char *src,
			      uint32 src_length);

/*
** Create a new SHA-1 context
*/
extern SHA1Context *SHA1_NewContext(void);

/*
** Clone a copy of the SHA-1 state. This is useful when you need to keep
** a running hash of something and and need to extract what would be
** the end values part way through.
*/
extern SHA1Context *SHA1_CloneContext(SHA1Context *cx);

/*
** Destroy a SHA-1 secure hash context.
**	"cx" the context
**	"freeit" if PR_TRUE then free the object as well as its sub-objects
*/
extern void SHA1_DestroyContext(SHA1Context *cx, PRBool freeit);

/*
** Reset a SHA-1 context, preparing it for a fresh round of hashing
*/
extern void SHA1_Begin(SHA1Context *cx);

/*
** Update the SHA-1 hash function with more data.
**	"cx" the context
**	"input" the data to hash
**	"inputLen" the amount of data to hash
*/
extern void SHA1_Update(SHA1Context *cx, const unsigned char *input,
			unsigned int inputLen);

/*
** Finish the SHA-1 hash function. Produce the digested results in "digest"
**	"cx" the context
**	"digest" where the 16 bytes of digest data are stored
**	"digestLen" where the digest length (20) is stored
**	"maxDigestLen" the maximum amount of data that can ever be
**	   stored in "digest"
*/
extern void SHA1_End(SHA1Context *cx, unsigned char *digest,
		     unsigned int *digestLen, unsigned int maxDigestLen);

SEC_END_PROTOS

#endif /* _HASH_H_ */
