/* -*- Mode: C; tab-width: 4 -*-
  mimempkc.c --- definition of the MimeMultipartSignedPKCS7 class (see mimei.h)
   Copyright © 1997 Netscape Communications Corporation, all rights reserved.
   Created: Jamie Zawinski <jwz@netscape.com>, 23-Sep-96.
 */

#include "secpkcs7.h"		/* Interface to libsec.a */
#include "mimempkc.h"

#define MIME_SUPERCLASS mimeMultipartSignedClass
MimeDefClass(MimeMultipartSignedPKCS7, MimeMultipartSignedPKCS7Class,
			 mimeMultipartSignedPKCS7Class, &MIME_SUPERCLASS);

static int MimeMultipartSignedPKCS7_initialize (MimeObject *);

static void *MimeMultPKCS7_init (MimeObject *);
static int MimeMultPKCS7_data_hash (char *, int32, void *);
static int MimeMultPKCS7_sig_hash  (char *, int32, void *);
static int MimeMultPKCS7_data_eof (void *, XP_Bool);
static int MimeMultPKCS7_sig_eof  (void *, XP_Bool);
static int MimeMultPKCS7_sig_init (void *, MimeObject *, MimeHeaders *);
static char * MimeMultPKCS7_generate (void *);
static void MimeMultPKCS7_free (void *);
static void MimeMultPKCS7_get_content_info (MimeObject *,
											SEC_PKCS7ContentInfo **,
											char **, int32 *, int32 *);

extern int SEC_ERROR_CERT_ADDR_MISMATCH;

static int
MimeMultipartSignedPKCS7ClassInitialize(MimeMultipartSignedPKCS7Class *class)
{
  MimeObjectClass          *oclass = (MimeObjectClass *)    class;
  MimeMultipartSignedClass *sclass = (MimeMultipartSignedClass *) class;

  oclass->initialize  = MimeMultipartSignedPKCS7_initialize;

  sclass->crypto_init           = MimeMultPKCS7_init;
  sclass->crypto_data_hash      = MimeMultPKCS7_data_hash;
  sclass->crypto_data_eof       = MimeMultPKCS7_data_eof;
  sclass->crypto_signature_init = MimeMultPKCS7_sig_init;
  sclass->crypto_signature_hash = MimeMultPKCS7_sig_hash;
  sclass->crypto_signature_eof  = MimeMultPKCS7_sig_eof;
  sclass->crypto_generate_html  = MimeMultPKCS7_generate;
  sclass->crypto_free           = MimeMultPKCS7_free;

  class->get_content_info	    = MimeMultPKCS7_get_content_info;

  XP_ASSERT(!oclass->class_initialized);
  return 0;
}

static int
MimeMultipartSignedPKCS7_initialize (MimeObject *object)
{
  return ((MimeObjectClass*)&MIME_SUPERCLASS)->initialize(object);
}


typedef struct MimeMultPKCS7data {
  HASH_HashType hash_type;
  void *data_hash_context;
  SEC_PKCS7DecoderContext *sig_decoder_context;
  SEC_PKCS7ContentInfo *content_info;
  char *sender_addr;
  int32 decode_error;
  int32 verify_error;
  SECItem item;
  MimeObject *self;
  XP_Bool parent_is_encrypted_p;
  XP_Bool parent_holds_stamp_p;
} MimeMultPKCS7data;


static void
MimeMultPKCS7_get_content_info(MimeObject *obj,
							   SEC_PKCS7ContentInfo **content_info_ret,
							   char **sender_email_addr_return,
							   int32 *decode_error_ret,
							   int32 *verify_error_ret)
{
  MimeMultipartSigned *msig = (MimeMultipartSigned *) obj;
  if (msig && msig->crypto_closure)
	{
	  MimeMultPKCS7data *data = (MimeMultPKCS7data *) msig->crypto_closure;

	  *decode_error_ret = data->decode_error;
	  *verify_error_ret = data->verify_error;
	  *content_info_ret = SEC_PKCS7CopyContentInfo(data->content_info);

	  if (sender_email_addr_return)
		*sender_email_addr_return = (data->sender_addr
									 ? XP_STRDUP(data->sender_addr)
									 : 0);
	}
}

/* #### MimeEncryptedPKCS7 and MimeMultipartSignedPKCS7 have a sleazy,
        incestuous, dysfunctional relationship. */
extern XP_Bool MimeEncryptedPKCS7_encrypted_p (MimeObject *obj);
extern XP_Bool MimePKCS7HeadersAndCertsMatch(MimeObject *obj,
											 SEC_PKCS7ContentInfo *,
											 char **);
extern char *MimePKCS7_MakeSAURL(MimeObject *obj);


static void *
MimeMultPKCS7_init (MimeObject *obj)
{
  MimeHeaders *hdrs = obj->headers;
  MimeMultPKCS7data *data = 0;
  char *ct, *micalg;
  HASH_HashType hash_type;

  ct = MimeHeaders_get (hdrs, HEADER_CONTENT_TYPE, FALSE, FALSE);
  if (!ct) return 0; /* #### bogus message?  out of memory? */
  micalg = MimeHeaders_get_parameter (ct, PARAM_MICALG);
  XP_FREE(ct);
  ct = 0;
  if (!micalg) return 0; /* #### bogus message?  out of memory? */

  if (!strcasecomp(micalg, PARAM_MICALG_MD5))
	hash_type = HASH_AlgMD5;
  else if (!strcasecomp(micalg, PARAM_MICALG_SHA1) ||
		   !strcasecomp(micalg, PARAM_MICALG_SHA1_2) ||
		   !strcasecomp(micalg, PARAM_MICALG_SHA1_3) ||
		   !strcasecomp(micalg, PARAM_MICALG_SHA1_4) ||
		   !strcasecomp(micalg, PARAM_MICALG_SHA1_5))
	hash_type = HASH_AlgSHA1;
  else if (!strcasecomp(micalg, PARAM_MICALG_MD2))
	hash_type = HASH_AlgMD2;
  else
	hash_type = HASH_AlgNULL;

  XP_FREE(micalg);
  micalg = 0;

  if (hash_type == HASH_AlgNULL) return 0; /* #### bogus message? */

  data = (MimeMultPKCS7data *) XP_ALLOC(sizeof(*data));
  if (!data) return 0;

  XP_MEMSET(data, 0, sizeof(*data));

  data->self = obj;
  data->hash_type = hash_type;

  XP_ASSERT(SECHashObjects[data->hash_type].create);
  if (!SECHashObjects[data->hash_type].create) return 0;

  XP_ASSERT(!data->data_hash_context);
  XP_ASSERT(!data->sig_decoder_context);

  data->data_hash_context = SECHashObjects[data->hash_type].create();

  XP_ASSERT(data->data_hash_context);
  if (!data->data_hash_context)
	{
	  XP_FREE(data);
	  return 0;
	}

  XP_ASSERT(SECHashObjects[data->hash_type].begin);
  if (!SECHashObjects[data->hash_type].begin) return 0;

  XP_SetError(0);
  SECHashObjects[data->hash_type].begin(data->data_hash_context);
  if (!data->decode_error)
	{
	  data->decode_error = XP_GetError();
	  if (data->decode_error)
		{
		  XP_FREE(data);
		  return 0;
		}
	}

  data->parent_holds_stamp_p =
	(obj->parent && mime_crypto_stamped_p(obj->parent));

  data->parent_is_encrypted_p =
	(obj->parent && MimeEncryptedPKCS7_encrypted_p (obj->parent));

  /* If the parent of this object is a crypto-blob, then it's the grandparent
	 who would have written out the headers and prepared for a stamp...
	 (This shit sucks.)
   */
  if (data->parent_is_encrypted_p &&
	  !data->parent_holds_stamp_p &&
	  obj->parent && obj->parent->parent)
	data->parent_holds_stamp_p =
	  mime_crypto_stamped_p (obj->parent->parent);

  return data;
}

static int
MimeMultPKCS7_data_hash (char *buf, int32 size, void *crypto_closure)
{
  MimeMultPKCS7data *data = (MimeMultPKCS7data *) crypto_closure;
  XP_ASSERT(data && data->data_hash_context);
  if (!data || !data->data_hash_context) return -1;

  XP_ASSERT(SECHashObjects[data->hash_type].update);
  if (!SECHashObjects[data->hash_type].update) return 0;

  XP_ASSERT(!data->sig_decoder_context);

  XP_SetError(0);
  SECHashObjects[data->hash_type].update(data->data_hash_context,
										 (unsigned char *) buf, size);
  if (!data->verify_error)
	data->verify_error = XP_GetError();

  return 0;
}

static int
MimeMultPKCS7_data_eof (void *crypto_closure, XP_Bool abort_p)
{
  MimeMultPKCS7data *data = (MimeMultPKCS7data *) crypto_closure;
  XP_ASSERT(data && data->data_hash_context);
  if (!data || !data->data_hash_context) return -1;

  XP_ASSERT(SECHashObjects[data->hash_type].end);
  if (!SECHashObjects[data->hash_type].end) return 0;

  XP_ASSERT(!data->sig_decoder_context);

  data->item.len = SECHashObjects[data->hash_type].length;
  data->item.data = (unsigned char *) XP_ALLOC(data->item.len);
  if (!data->item.data) return MK_OUT_OF_MEMORY;

  XP_SetError(0);
  SECHashObjects[data->hash_type].end(data->data_hash_context,
									  data->item.data,
									  &data->item.len,
									  data->item.len);
  if (!data->verify_error)
	data->verify_error = XP_GetError();

  SECHashObjects[data->hash_type].destroy(data->data_hash_context, TRUE);
  data->data_hash_context = 0;

  /* At this point, data->item.data contains a digest for the first part.
	 When we process the signature, the security library will compare this
	 digest to what's in the signature object. */

  return 0;
}


static int
MimeMultPKCS7_sig_init (void *crypto_closure,
						MimeObject *multipart_object,
						MimeHeaders *signature_hdrs)
{
  MimeMultPKCS7data *data = (MimeMultPKCS7data *) crypto_closure;
  MimeDisplayOptions *opts = multipart_object->options;
  char *ct;
  int status = 0;

  XP_ASSERT(!data->data_hash_context);
  XP_ASSERT(!data->sig_decoder_context);

  XP_ASSERT(signature_hdrs);
  if (!signature_hdrs) return -1;

  ct = MimeHeaders_get (signature_hdrs, HEADER_CONTENT_TYPE, TRUE, FALSE);

  /* Verify that the signature object is of the right type. */
  if (!ct || strcasecomp(ct, APPLICATION_XPKCS7_SIGNATURE))
	status = -1; /* #### error msg about bogus message */
  FREEIF(ct);
  if (status < 0) return status;

  XP_SetError(0);
  data->sig_decoder_context =
	SEC_PKCS7DecoderStart(0, 0, /* no content cb */
						  ((SECKEYGetPasswordKey) opts->passwd_prompt_fn),
						  opts->passwd_prompt_fn_arg);
  if (!data->sig_decoder_context)
	{
	  status = XP_GetError();
	  XP_ASSERT(status < 0);
	  if (status >= 0) status = -1;
	}
  return status;
}


static int
MimeMultPKCS7_sig_hash (char *buf, int32 size, void *crypto_closure)
{
  MimeMultPKCS7data *data = (MimeMultPKCS7data *) crypto_closure;

  XP_ASSERT(data && data->sig_decoder_context);
  if (!data || !data->sig_decoder_context) return -1;

  XP_ASSERT(!data->data_hash_context);

  XP_SetError(0);
  if (SEC_PKCS7DecoderUpdate(data->sig_decoder_context, buf, size)
	  != SECSuccess)
	{
	  if (!data->verify_error)
		data->verify_error = XP_GetError();
	  XP_ASSERT(data->verify_error < 0);
	  if (data->verify_error >= 0)
		data->verify_error = -1;
	}

  return 0;
}

static int
MimeMultPKCS7_sig_eof (void *crypto_closure, XP_Bool abort_p)
{
  MimeMultPKCS7data *data = (MimeMultPKCS7data *) crypto_closure;

  if (!data) return -1;

  XP_ASSERT(!data->data_hash_context);

  /* Hand an EOF to the crypto library.

	 We save away the value returned and will use it later to emit a
	 blurb about whether the signature validation was cool.
   */

  XP_ASSERT(!data->content_info);

  if (data->sig_decoder_context)
	{
	  XP_SetError(0);
	  data->content_info = SEC_PKCS7DecoderFinish (data->sig_decoder_context);
	  data->sig_decoder_context = 0;
	  if (!data->content_info && !data->verify_error)
		data->verify_error = XP_GetError();

	  XP_ASSERT(data->content_info ||
				data->verify_error || data->decode_error);
	}

  return 0;
}



static void
MimeMultPKCS7_free (void *crypto_closure)
{
  MimeMultPKCS7data *data = (MimeMultPKCS7data *) crypto_closure;
  XP_ASSERT(data);
  if (!data) return;

  FREEIF(data->sender_addr);

  if (data->data_hash_context)
	{
	  SECHashObjects[data->hash_type].destroy(data->data_hash_context, TRUE);
	  data->data_hash_context = 0;
	}

  if (data->sig_decoder_context)
	{
	  SEC_PKCS7ContentInfo *cinfo =
		SEC_PKCS7DecoderFinish (data->sig_decoder_context);
	  if (cinfo)
		SEC_PKCS7DestroyContentInfo(cinfo);
	}

  if (data->content_info)
	{
	  SEC_PKCS7DestroyContentInfo(data->content_info);
	  data->content_info = 0;
	}

  FREEIF(data->item.data);

  XP_FREE(data);
}


static char *
MimeMultPKCS7_generate (void *crypto_closure)
{
  MimeMultPKCS7data *data = (MimeMultPKCS7data *) crypto_closure;
  XP_Bool signed_p = TRUE;
  XP_Bool good_p = TRUE;
  XP_Bool encrypted_p;

  XP_ASSERT(data);
  if (!data) return 0;
  encrypted_p = data->parent_is_encrypted_p;

  if (data->content_info)
	{
	  XP_SetError(0);
	  good_p =
		SEC_PKCS7VerifyDetachedSignature(data->content_info,
										 certUsageEmailSigner,
										 &data->item,
										 data->hash_type,
										 TRUE);  /* #### keepcerts */
	  if (!good_p)
		{
		  if (!data->verify_error)
			data->verify_error = XP_GetError();
		  XP_ASSERT(data->verify_error < 0);
		  if (data->verify_error >= 0)
			data->verify_error = -1;
		}
	  else
		{
		  good_p = MimePKCS7HeadersAndCertsMatch(data->self,
												 data->content_info,
												 &data->sender_addr);
		  if (!good_p && !data->verify_error)
			data->verify_error = SEC_ERROR_CERT_ADDR_MISMATCH;
		}

	  if (SEC_PKCS7ContainsCertsOrCrls(data->content_info))
		{
		  /* #### call libsec telling it to import the certs */
		}

	  /* Don't free these yet -- keep them around for the lifetime of the
		 MIME object, so that we can get at the security info of sub-parts
		 of the currently-displayed message. */
#if 0
	  SEC_PKCS7DestroyContentInfo(data->content_info);
	  data->content_info = 0;
#endif /* 0 */
	}
  else
	{
	  /* No content_info at all -- since we're inside a multipart/signed,
		 that means that we've either gotten a message that was truncated
		 before the signature part, or we ran out of memory, or something
		 awful has happened.  Anyway, it sure ain't good_p.
	   */
	  good_p = FALSE;
	}


  XP_ASSERT(data->self);
  if (data->self && data->self->parent)
	mime_set_crypto_stamp(data->self->parent, signed_p, encrypted_p);


  {
	char *stamp_url = (data->self
					   ? MimePKCS7_MakeSAURL(data->self)
					   : 0);
	char *result =
	  MimeHeaders_make_crypto_stamp (encrypted_p, signed_p, good_p,
									 data->parent_holds_stamp_p,
									 stamp_url);
	FREEIF(stamp_url);
	return result;
  }
}
