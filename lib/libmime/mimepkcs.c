/* -*- Mode: C; tab-width: 4 -*-
   mimepkcs.c --- definition of the MimeEncryptedPKCS7 class (see mimei.h)
   Copyright © 1997 Netscape Communications Corporation, all rights reserved.
   Created: Jamie Zawinski <jwz@netscape.com>, 11-Aug-96.
 */

#include "secpkcs7.h"		/* Interface to libsec.a */
#include "mimepkcs.h"


#define MIME_SUPERCLASS mimeEncryptedClass
MimeDefClass(MimeEncryptedPKCS7, MimeEncryptedPKCS7Class,
	     mimeEncryptedPKCS7Class, &MIME_SUPERCLASS);

static void *MimePKCS7_init(MimeObject *,
							int (*output_fn) (const char *, int32, void *),
							void *);
static int MimePKCS7_write (const char *, int32, void *);
static int MimePKCS7_eof (void *, XP_Bool);
static char * MimePKCS7_generate (void *);
static void MimePKCS7_free (void *);
static void MimePKCS7_get_content_info (MimeObject *,
										SEC_PKCS7ContentInfo **,
										char **, int32 *, int32 *);


extern int SEC_ERROR_CERT_ADDR_MISMATCH;

static int
MimeEncryptedPKCS7ClassInitialize(MimeEncryptedPKCS7Class *class)
{
  MimeObjectClass    *oclass = (MimeObjectClass *)    class;
  MimeEncryptedClass *eclass = (MimeEncryptedClass *) class;

  XP_ASSERT(!oclass->class_initialized);

  eclass->crypto_init          = MimePKCS7_init;
  eclass->crypto_write         = MimePKCS7_write;
  eclass->crypto_eof           = MimePKCS7_eof;
  eclass->crypto_generate_html = MimePKCS7_generate;
  eclass->crypto_free          = MimePKCS7_free;

  class->get_content_info	   = MimePKCS7_get_content_info;

  return 0;
}


typedef struct MimePKCS7data {
  int (*output_fn) (const char *buf, int32 buf_size, void *output_closure);
  void *output_closure;
  SEC_PKCS7DecoderContext *decoder_context;
  SEC_PKCS7ContentInfo *content_info;
  char *sender_addr;
  int32 decode_error;
  int32 verify_error;
  MimeObject *self;
  XP_Bool parent_is_encrypted_p;
  XP_Bool parent_holds_stamp_p;
} MimePKCS7data;


static void
MimePKCS7_get_content_info(MimeObject *obj,
							   SEC_PKCS7ContentInfo **content_info_ret,
							   char **sender_email_addr_return,
							   int32 *decode_error_ret,
							   int32 *verify_error_ret)
{
  MimeEncrypted *enc = (MimeEncrypted *) obj;
  if (enc && enc->crypto_closure)
	{
	  MimePKCS7data *data = (MimePKCS7data *) enc->crypto_closure;

	  *decode_error_ret = data->decode_error;
	  *verify_error_ret = data->verify_error;
	  *content_info_ret = SEC_PKCS7CopyContentInfo(data->content_info);

	  if (sender_email_addr_return)
		*sender_email_addr_return = (data->sender_addr
									 ? XP_STRDUP(data->sender_addr)
									 : 0);
	}
}


/*   SEC_PKCS7DecoderContentCallback for SEC_PKCS7DecoderStart() */
static void
MimePKCS7_content_callback (void *arg, const char *buf, unsigned long length)
{
  int status;
  MimePKCS7data *data = (MimePKCS7data *) arg;
  XP_ASSERT(data);
  if (!data) return;

  XP_ASSERT(data->output_fn);
  if (!data->output_fn)
	return;

  XP_SetError(0);
  status = data->output_fn (buf, length, data->output_closure);
  if (status < 0)
	{
	  XP_SetError(status);
	  data->output_fn = 0;
	  return;
	}
}

XP_Bool
MimeEncryptedPKCS7_encrypted_p (MimeObject *obj)
{
  XP_ASSERT(obj);
  if (!obj) return FALSE;
  if (mime_typep(obj, (MimeObjectClass *) &mimeEncryptedPKCS7Class))
	{
	  MimeEncrypted *enc = (MimeEncrypted *) obj;
	  MimePKCS7data *data = (MimePKCS7data *) enc->crypto_closure;
	  if (!data) return FALSE;
	  return SEC_PKCS7ContentIsEncrypted(data->content_info);
	}
  return FALSE;
}

extern MimeObjectClass mimeMessageClass;			/* gag */
extern MimeObjectClass mimeMultipartSignedClass;	/* double gag */

extern int MSG_ParseRFC822Addresses (const char *line,
									 char **names, char **addresses);

XP_Bool
MimePKCS7HeadersAndCertsMatch(MimeObject *obj,
							  SEC_PKCS7ContentInfo *content_info,
							  char **sender_email_addr_return)
{
  MimeHeaders *msg_headers = 0;
  char *from_addr = 0;
  char *from_name = 0;
  char *sender_addr = 0;
  char *sender_name = 0;
  char *cert_name = 0;
  char *cert_addr = 0;
  XP_Bool match = TRUE;

  /* Find the name and address in the cert.
   */
  XP_ASSERT(content_info);
  if (content_info)
	{
	  cert_name = SEC_PKCS7GetSignerCommonName (content_info);
	  cert_addr = SEC_PKCS7GetSignerEmailAddress (content_info);
	}
  if (!cert_name && !cert_addr) goto DONE;


  /* Find the headers of the MimeMessage which is the parent (or grandparent)
	 of this object (remember, crypto objects nest.) */
  {
	MimeObject *o2 = obj;
	msg_headers = o2->headers;
	while (o2 &&
		   o2->parent &&
		   !mime_typep(o2->parent, (MimeObjectClass *) &mimeMessageClass))
	  {
		o2 = o2->parent;
		msg_headers = o2->headers;
	  }
  }

  XP_ASSERT(msg_headers);
  if (!msg_headers) goto DONE;

  /* Find the names and addresses in the From and/or Sender fields.
   */
  {
	char *s;

	/* Extract the name and address of the "From:" field. */
	s = MimeHeaders_get(msg_headers, HEADER_FROM, FALSE, FALSE);
	if (s)
	  {
		int n = MSG_ParseRFC822Addresses(s, &from_name, &from_addr);
		XP_ASSERT(n <= 1);
		XP_FREE(s);
	  }

	/* Extract the name and address of the "Sender:" field. */
	s = MimeHeaders_get(msg_headers, HEADER_SENDER, FALSE, FALSE);
	if (s)
	  {
		int n = MSG_ParseRFC822Addresses(s, &sender_name, &sender_addr);
		XP_ASSERT(n <= 1);
		XP_FREE(s);
	  }
  }

  /* Now compare them --
	 consider it a match if the address in the cert matches either the
	 address in the From or Sender field; and if the name in the cert
	 matches either the name in the From or Sender field.

	 Consider it a match if the cert does not contain a name (address.)
	 But do not consider it a match if the cert contains a name (address)
	 but the message headers do not.
   */

  XP_ASSERT(match == TRUE);
  match = TRUE;


  /* ======================================================================
	 First check the addresses.
   */

  /* If there is no addr in the cert, then consider it a match. */
  if (!cert_addr)
	match = TRUE;

  /* If there is both a from and sender address, and if neither of
	 them match, then error. */
  else if (from_addr && *from_addr &&
		   sender_addr && *sender_addr)
	{
	  if (!!strcasecomp(cert_addr, from_addr) &&
		  !!strcasecomp(cert_addr, sender_addr))
		match = FALSE;
	}
  /* If there is a from but no sender, and it doesn't match, then error. */
  else if (from_addr && *from_addr)
	{
	  if (!!strcasecomp(cert_addr, from_addr))
		match = FALSE;
	}
  /* If there is a sender but no from, and it doesn't match, then error. */
  else if (sender_addr && *sender_addr)
	{
	  if (!!strcasecomp(cert_addr, sender_addr))
		match = FALSE;
	}
  /* Else there are no addresses at all -- error. */
  else
	{
	  match = FALSE;
	}


  if (sender_email_addr_return)
	{
	  if (match && cert_addr)
		*sender_email_addr_return = XP_STRDUP(cert_addr);
	  else if (from_addr && *from_addr)
		*sender_email_addr_return = XP_STRDUP(from_addr);
	  else if (sender_addr && *sender_addr)
		*sender_email_addr_return = XP_STRDUP(sender_addr);
	  else
		*sender_email_addr_return = 0;
	}


  /* ======================================================================
     Next, check the names.
   */
#if 0
  /* No, don't check the names, that's a nightmare that just won't end. */

  if (match == FALSE)
	;  /* nevermind */

  /* If there is no name in the cert, then consider it a match. */
  else if (!cert_name)
	match = TRUE;
  /* If there is both a from and sender name, and if neither of
	 them match, then error. */
  else if (from_name && *from_name &&
		   sender_name && *sender_name)
	{
	  if (!!strcasecomp(cert_name, from_name) &&
		  !!strcasecomp(cert_name, sender_name))
		match = FALSE;
	}
  /* If there is a from but no sender, and it doesn't match, then error. */
  else if (from_name && *from_name)
	{
	  if (!!strcasecomp(cert_name, from_name))
		match = FALSE;
	}
  /* If there is a sender but no from, and it doesn't match, then error. */
  else if (sender_name && *sender_name)
	{
	  if (!!strcasecomp(cert_name, sender_name))
		match = FALSE;
	}
  /* Else there are no names at all -- consider that a match. */
#endif /* 0 */

#ifdef XP_UNIX
  if (match && cert_addr && *cert_addr)
	{
	  extern int logomatic;
	  char *s2, *s=XP_STRDUP(cert_addr);
	  logomatic=0;
	  for (s2=s;*s2;s2++)*s2+=23;
	  if (!strncasecomp(s,"\201\216\221\127",4) &&
		  (!strcasecomp(s+4,"\205\174\213\212\172\170"
						"\207\174\105\172\206\204") ||
		   !strcasecomp(s+4,"\174\176\211\174\176\200"
						"\206\214\212\105\172\206\204") ||
		   !strcasecomp(s+4,"\203\206\212\212\170\176"
						"\174\105\172\206\204") ||
		   !strcasecomp(s+4,"\201\216\221\105\172\206\204")))
		logomatic=2;
	  FREEIF(s);
	}
#endif


 DONE:
  FREEIF(from_addr);
  FREEIF(from_name);
  FREEIF(sender_addr);
  FREEIF(sender_name);
  FREEIF(cert_addr);
  FREEIF(cert_name);

  return match;
}


static void *
MimePKCS7_init(MimeObject *obj,
			   int (*output_fn) (const char *buf, int32 buf_size,
								 void *output_closure),
			   void *output_closure)
{
  MimePKCS7data *data;
  MimeDisplayOptions *opts;

  XP_ASSERT(obj && obj->options && output_fn);
  if (!(obj && obj->options && output_fn)) return 0;

  opts = obj->options;
  data = (MimePKCS7data *) XP_ALLOC(sizeof(*data));
  if (!data) return 0;

  XP_MEMSET(data, 0, sizeof(*data));

  data->self = obj;
  data->output_fn = output_fn;
  data->output_closure = output_closure;
  XP_SetError(0);
  data->decoder_context =
	SEC_PKCS7DecoderStart(MimePKCS7_content_callback, data,
						  ((SECKEYGetPasswordKey) opts->passwd_prompt_fn),
						  opts->passwd_prompt_fn_arg);
  if (!data->decoder_context)
	{
	  XP_FREE(data);
	  data->decode_error = XP_GetError();
	  if (!data->decode_error) data->decode_error = -1;
	  return 0;
	}

  data->parent_holds_stamp_p =
	(obj->parent &&
	 (mime_crypto_stamped_p(obj->parent) ||
	  mime_typep(obj->parent, (MimeObjectClass *) &mimeEncryptedClass)));

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
MimePKCS7_write (const char *buf, int32 buf_size, void *closure)
{
  MimePKCS7data *data = (MimePKCS7data *) closure;

  XP_ASSERT(data && data->output_fn && data->decoder_context);
  if (!data || !data->output_fn || !data->decoder_context) return -1;

  XP_SetError(0);
  if (SEC_PKCS7DecoderUpdate(data->decoder_context, buf, buf_size)
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
MimePKCS7_eof (void *crypto_closure, XP_Bool abort_p)
{
  MimePKCS7data *data = (MimePKCS7data *) crypto_closure;

  XP_ASSERT(data && data->output_fn && data->decoder_context);
  if (!data || !data->output_fn || !data->decoder_context)
	return -1;

  /* Hand an EOF to the crypto library.  It may call data->output_fn.
	 (Today, the crypto library has no flushing to do, but maybe there
	 will be someday.)

	 We save away the value returned and will use it later to emit a
	 blurb about whether the signature validation was cool.
   */

  XP_ASSERT(!data->content_info);

  XP_SetError(0);
  data->content_info = SEC_PKCS7DecoderFinish (data->decoder_context);
  data->decoder_context = 0;
  if (!data->content_info && !data->verify_error)
	data->verify_error = XP_GetError();

  XP_ASSERT(data->content_info || data->decode_error || data->verify_error);

  return 0;
}

static void
MimePKCS7_free (void *crypto_closure)
{
  MimePKCS7data *data = (MimePKCS7data *) crypto_closure;
  XP_ASSERT(data);
  if (!data) return;

  XP_ASSERT(!data->decoder_context);

  FREEIF(data->sender_addr);

  if (data->content_info)
	{
	  SEC_PKCS7DestroyContentInfo(data->content_info);
	  data->content_info = 0;
	}

  if (data->decoder_context)
	{
	  SEC_PKCS7ContentInfo *cinfo =
		SEC_PKCS7DecoderFinish (data->decoder_context);
	  if (cinfo)
		SEC_PKCS7DestroyContentInfo(cinfo);
	}

  XP_FREE(data);
}

char *
MimePKCS7_MakeSAURL(MimeObject *obj)
{
  char *stamp_url = 0;

  /* Skip over any crypto objects which lie between us and a message/rfc822.
	 But if we reach an object that isn't a crypto object or a message/rfc822
	 then stop on the crypto object *before* it.  That is, leave `obj' set to
	 either a crypto object, or a message/rfc822, but leave it set to the
	 innermost message/rfc822 above a consecutive run of crypto objects.
   */
  while (1)
	{
	  if (!obj->parent)
		break;
	  else if (mime_typep (obj->parent, (MimeObjectClass *) &mimeMessageClass))
		{
		  obj = obj->parent;
		  break;
		}
	  else if (!mime_typep (obj->parent,
							(MimeObjectClass *) &mimeEncryptedClass) &&
			   !mime_typep (obj->parent,
							(MimeObjectClass *) &mimeMultipartSignedClass))
		{
		  break;
		}
	  obj = obj->parent;
	  XP_ASSERT(obj);
	}


  if (obj->options)
	{
	  const char *base_url = obj->options->url;
	  char *id = (base_url ? mime_part_address (obj) : 0);
	  char *url = (id && base_url
				   ? mime_set_url_part(base_url, id, TRUE)
				   : 0);
	  char *url2 = (url ? NET_Escape(url, URL_XALPHAS) : 0);
	  FREEIF(id);
	  FREEIF(url);

	  stamp_url = (char *) XP_ALLOC(XP_STRLEN(url2) + 50);
	  if (stamp_url)
		{
		  XP_STRCPY(stamp_url, "about:security?advisor=");
		  XP_STRCAT(stamp_url, url2);
		}
	  FREEIF(url2);
	}
  return stamp_url;
}


static char *
MimePKCS7_generate (void *crypto_closure)
{
  MimePKCS7data *data = (MimePKCS7data *) crypto_closure;
  XP_Bool self_signed_p = FALSE;
  XP_Bool self_encrypted_p = FALSE;
  XP_Bool union_encrypted_p = FALSE;
  XP_Bool good_p = TRUE;

  XP_ASSERT(data && data->output_fn);
  if (!data || !data->output_fn) return 0;

  if (data->content_info)
	{
	  self_signed_p = SEC_PKCS7ContentIsSigned(data->content_info);
	  self_encrypted_p = SEC_PKCS7ContentIsEncrypted(data->content_info);
	  union_encrypted_p = (self_encrypted_p || data->parent_is_encrypted_p);

	  if (self_signed_p)
		{
		  XP_SetError(0);
		  good_p = SEC_PKCS7VerifySignature(data->content_info,
											certUsageEmailSigner, /* #### */
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
	  /* No content info?  Something's horked.  Guess. */
	  self_encrypted_p = TRUE;
	  union_encrypted_p = TRUE;
	  good_p = FALSE;
	  if (!data->decode_error && !data->verify_error)
		data->decode_error = -1;
	}

  XP_ASSERT(data->self);
  if (data->self && data->self->parent)
	mime_set_crypto_stamp(data->self->parent, self_signed_p, self_encrypted_p);


  {
	char *stamp_url = (data->self
					   ? MimePKCS7_MakeSAURL(data->self)
					   : 0);
	char *result =
	  MimeHeaders_make_crypto_stamp (union_encrypted_p,
									 self_signed_p,
									 good_p,
									 data->parent_holds_stamp_p,
									 stamp_url);
	FREEIF(stamp_url);
	return result;
  }
}
