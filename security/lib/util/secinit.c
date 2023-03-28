#include "xp.h"
#include "sechash.h"
#include "cert.h"
#include "key.h"
#include "secrng.h"
#include "ssl.h"
#include "nspr.h"
#ifdef XP_MAC
#include "sslimpl.h"
#else
#include "../ssl/sslimpl.h"
#endif
#include "pk11func.h"

#ifdef FORTEZZA
/* Sigh for FortezzaGlobalinit() */
#include "fortezza.h"
#endif

static void *
null_hash_new_context(void)
{
    return NULL;
}

static void *
null_hash_clone_context(void *v)
{
    PORT_Assert(v == NULL);
    return NULL;
}

static void
null_hash_begin(void *v)
{
}

static void
null_hash_update(void *v, const unsigned char *input, unsigned int length)
{
}

static void
null_hash_end(void *v, unsigned char *output, unsigned int *outLen,
	      unsigned int maxOut)
{
    *outLen = 0;
}

static void
null_hash_destroy_context(void *v, PRBool b)
{
    PORT_Assert(v == NULL);
}


static void *
md2_NewContext(void) {
	return (void *) PK11_CreateDigestContext(SEC_OID_MD2);
}

static void *
md5_NewContext(void) {
	return (void *) PK11_CreateDigestContext(SEC_OID_MD5);
}

static void *
sha1_NewContext(void) {
	return (void *) PK11_CreateDigestContext(SEC_OID_SHA1);
}

SECHashObject SECHashObjects[] = {
  { 0,
    (void * (*)(void)) null_hash_new_context,
    (void * (*)(void *)) null_hash_clone_context,
    (void (*)(void *, PRBool)) null_hash_destroy_context,
    (void (*)(void *)) null_hash_begin,
    (void (*)(void *, const unsigned char *, unsigned int)) null_hash_update,
    (void (*)(void *, unsigned char *, unsigned int *,
	      unsigned int)) null_hash_end
  },
  { MD2_LENGTH,
    (void * (*)(void)) md2_NewContext,
    (void * (*)(void *)) PK11_CloneContext,
    (void (*)(void *, PRBool)) PK11_DestroyContext,
    (void (*)(void *)) PK11_DigestBegin,
    (void (*)(void *, const unsigned char *, unsigned int)) PK11_DigestOp,
    (void (*)(void *, unsigned char *, unsigned int *, unsigned int)) 
							PK11_DigestFinal
  },
  { MD5_LENGTH,
    (void * (*)(void)) md5_NewContext,
    (void * (*)(void *)) PK11_CloneContext,
    (void (*)(void *, PRBool)) PK11_DestroyContext,
    (void (*)(void *)) PK11_DigestBegin,
    (void (*)(void *, const unsigned char *, unsigned int)) PK11_DigestOp,
    (void (*)(void *, unsigned char *, unsigned int *, unsigned int)) 
							PK11_DigestFinal
  },
  { SHA1_LENGTH,
    (void * (*)(void)) sha1_NewContext,
    (void * (*)(void *)) PK11_CloneContext,
    (void (*)(void *, PRBool)) PK11_DestroyContext,
    (void (*)(void *)) PK11_DigestBegin,
    (void (*)(void *, const unsigned char *, unsigned int)) PK11_DigestOp,
    (void (*)(void *, unsigned char *, unsigned int *, unsigned int)) 
							PK11_DigestFinal
  },
};


SECHashObject SECRawHashObjects[] = {
  { 0,
    (void * (*)(void)) null_hash_new_context,
    (void * (*)(void *)) null_hash_clone_context,
    (void (*)(void *, PRBool)) null_hash_destroy_context,
    (void (*)(void *)) null_hash_begin,
    (void (*)(void *, const unsigned char *, unsigned int)) null_hash_update,
    (void (*)(void *, unsigned char *, unsigned int *,
	      unsigned int)) null_hash_end
  },
  { MD2_LENGTH,
    (void * (*)(void)) MD2_NewContext,
    (void * (*)(void *)) MD2_CloneContext,
    (void (*)(void *, PRBool)) MD2_DestroyContext,
    (void (*)(void *)) MD2_Begin,
    (void (*)(void *, const unsigned char *, unsigned int)) MD2_Update,
    (void (*)(void *, unsigned char *, unsigned int *, unsigned int)) MD2_End
  },
  { MD5_LENGTH,
    (void * (*)(void)) MD5_NewContext,
    (void * (*)(void *)) MD5_CloneContext,
    (void (*)(void *, PRBool)) MD5_DestroyContext,
    (void (*)(void *)) MD5_Begin,
    (void (*)(void *, const unsigned char *, unsigned int)) MD5_Update,
    (void (*)(void *, unsigned char *, unsigned int *, unsigned int)) MD5_End
  },
  { SHA1_LENGTH,
    (void * (*)(void)) SHA1_NewContext,
    (void * (*)(void *)) SHA1_CloneContext,
    (void (*)(void *, PRBool)) SHA1_DestroyContext,
    (void (*)(void *)) SHA1_Begin,
    (void (*)(void *, const unsigned char *, unsigned int)) SHA1_Update,
    (void (*)(void *, unsigned char *, unsigned int *, unsigned int)) SHA1_End
  },
};


static int sec_inited = 0;
void SEC_Init(void)
{
    /* PR_Init() must be called before SEC_Init() */
#if !defined(SERVER_BUILD)
    PORT_Assert(PR_Initialized() == PR_TRUE);
#endif
    if (sec_inited)
	return;

    /* RNG_RNGInit(); */
    SSL_InitHashLock();
    SSL3_Init();
#ifdef FORTEZZA
    FortezzaGlobalInit();
#endif

    sec_inited = 1;
}
