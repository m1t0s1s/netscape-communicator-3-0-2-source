/* -*- Mode: C; tab-width: 4 -*-
   compose.c --- generation and delivery of MIME objects.
   Copyright © 1997 Netscape Communications Corporation, all rights reserved.
   Created: Jamie Zawinski <jwz@netscape.com>, 25-May-95.
 */

#include "msg.h"
#include "ntypes.h"
#include "structs.h"
#include "xlate.h"		/* Text and PostScript converters */
#include "merrors.h"
#include "gui.h"		/* for XP_AppCodeName */
#include "mime.h"
#include "mimeenc.h"
#include "mailsum.h"
#include "xp_time.h"	/* For XP_LocalZoneOffset() */
#include "libi18n.h"
#include "xpgetstr.h"
#include "prtime.h"
#include "prtypes.h"
#include "composec.h"
#include "secrng.h"	/* for RNG_GenerateGlobalRandomBytes() */

extern int MK_MSG_ASSEMBLING_MSG;
extern int MK_MSG_ASSEMB_DONE_MSG;
extern int MK_MSG_LOAD_ATTACHMNT;
extern int MK_MSG_LOAD_ATTACHMNTS;
extern int MK_MSG_DELIV_MAIL;
extern int MK_MSG_DELIV_MAIL_DONE;
extern int MK_MSG_DELIV_NEWS;
extern int MK_MSG_DELIV_NEWS_DONE;
extern int MK_MSG_QUEUEING;
extern int MK_MSG_WRITING_TO_FCC;
extern int MK_MSG_QUEUED;

#ifdef XP_MAC
#include "m_cvstrm.h"
#endif

extern int MK_MIME_ERROR_WRITING_FILE;
extern int MK_MIME_MULTIPART_BLURB;
extern int MK_MIME_NO_RECIPIENTS;
extern int MK_MIME_NO_SENDER;
extern int MK_MSG_COULDNT_OPEN_FCC_FILE;  
extern int MK_OUT_OF_MEMORY;
extern int MK_UNABLE_TO_OPEN_TMP_FILE;  


#define MK_ATTACHMENT_LOAD_FAILED -666

/* Asynchronous mailing of messages with attached URLs.

   - If there are any attachments, start their URLs going, and write each
     of them to a temp file.

   - While writing to their files, examine the data going by and decide
     what kind of encoding, if any, they need.  Also remember their content
     types.

   - Once that URLs has been saved to a temp file (or, if there were no
     attachments) generate a final temp file, of the actual message:

	 -  Generate a string of the headers.
	 -  Open the final temp file.
	 -  Write the headers.
     -  Examine the first part, and decide whether to encode it.
	 -  Write the first part to the file, possibly encoded.
	 -  Write the second and subsequent parts to the file, possibly encoded.
        (Open the first temp file and copy it to the final temp file, and so
		on, through an encoding filter.)

     - Delete the attachment temp file(s) as we finish with them.
	 - Close the final temp file.
	 - Open the news: url.
	 - Send the final temp file to NNTP.
	   If there's an error, run the callback with "failure" status.
	 - If mail succeeded, open the mailto: url.
	 - Send the final temp file to SMTP.
	   If there's an error, run the callback with "failure" status.
	 - Otherwise, run the callback with "success" status.
	 - Free everything, delete the final temp file.

  The theory behind the encoding logic:
  =====================================

  If the document is of type text/html, and the user has asked to attach it
  as source or postscript, it will be run through the appropriate converter
  (which will result in a document of type text/plain.)

  An attachment will be encoded if:

   - it is of a non-text type (in which case we will use base64); or
   - The "use QP" option has been selected and high-bit characters exist; or
   - any NULLs exist in the document; or
   - any line is longer than 900 bytes.

   - If we are encoding, and more than 10% of the document consists of 
     non-ASCII characters, then we always use base64 instead of QP.

  We eschew quoted-printable in favor of base64 for documents which are likely
  to always be binary (images, sound) because, on the off chance that a GIF
  file (for example) might contain primarily bytes in the ASCII range, using
  the quoted-printable representation might cause corruption due to the
  translation of CR or LF to CRLF.  So, when we don't know that the document
  has "lines", we don't use quoted-printable.
 */


/* It's better to send a message as news before sending it as mail, because
   the NNTP server is more likely to reject the article (for any number of
   reasons) than the SMTP server is. */
#undef MAIL_BEFORE_NEWS

/* Generating a message ID here is a good because it means that if a message
   is sent to both mail and news, it will have the same ID in both places. */
#define GENERATE_MESSAGE_ID


/* For maximal compatibility, it helps to emit both
      Content-Type: <type>; name="<original-file-name>"
   as well as
      Content-Disposition: inline; filename="<original-file-name>"

  The lossage here is, RFC1341 defined the "name" parameter to Content-Type,
  but then RFC1521 deprecated it in anticipation of RFC1806, which defines
  Content-Type and the "filename" parameter.  But, RFC1521 is "Standards Track"
  while RFC1806 is still "Experimental."  So, it's probably best to just
  implement both.
 */
#define EMIT_NAME_IN_CONTENT_TYPE


/* Whether the contents of the BCC header should be preserved in the FCC'ed
   copy of a message.  See comments below, in mime_do_fcc_1().
 */
#ifndef XP_MAC
#define SAVE_BCC_IN_FCC_FILE
#endif

/* When attaching an HTML document, one must indicate the original URL of
   that document, if the receiver is to have any chance of being able to
   retreive and display the inline images, or to click on any links in the
   HTML.

   The way we have done this in the past is by inserting a <BASE> tag as the
   first line of all HTML documents we attach.  (This is kind of bad in that
   we're actually modifying the document, and it really isn't our place to
   do that.)

   The sanctioned *new* way of doing this is to insert a Content-Base header
   field on the attachment.  This is (will be) a part of the forthcoming MHTML
   spec.

   If GENERATE_CONTENT_BASE, we generate a Content-Base header.

   If MANGLE_HTML_ATTACHMENTS_WITH_BASE_TAG is defined, we add a BASE tag to
   the bodies; we should do this for a while if only for backward compatibility
   with the installed base.
 */
#define GENERATE_CONTENT_BASE
#define MANGLE_HTML_ATTACHMENTS_WITH_BASE_TAG


static XP_Bool mime_use_quoted_printable_p = TRUE;

#ifdef XP_MAC

extern OSErr my_FSSpecFromPathname(char* src_filename, FSSpec* fspec);
static char* NET_GetLocalFileFromURL(char *url)
{
	char * finalPath;
	XP_ASSERT(strncasecomp(url, "file://", 7) == 0);
	finalPath = (char*)XP_ALLOC(strlen(url));
	if (finalPath == NULL)
		return NULL;
	strcpy(finalPath, url+6+1);
	return finalPath;
}

static char* NET_GetURLFromLocalFile(char *filename)
{
    /*  file:///<path>0 */
	char * finalPath = (char*)XP_ALLOC(strlen(filename) + 8 + 1);
	if (finalPath == NULL)
		return NULL;
	finalPath[0] = 0;
	strcat(finalPath, "file://");
	strcat(finalPath, filename);
	return finalPath;
}
#endif

void
MIME_ConformToStandard (XP_Bool conform_p)
{
  mime_use_quoted_printable_p = conform_p;
}


struct mime_delivery_state
{
  MWContext *context;		/* Context to use when loading the URLs */
  void *fe_data;			/* passed in and passed to callback */
  char *to;					/* Where to send the message once it's done */
  char *cc;
  char *bcc;
  char *fcc;
  char *newsgroups;
  char *followup_to;
  char *reply_to;
  char *news_url;			/* the news url prefix for posting */
  char* message_id;

  /* these are used only in generating the initial header block. */
  char *from;
  char *organization;
  char *subject;
  char *references;
  char *other_random_headers;

  XP_Bool dont_deliver_p;	/* If set, we just return the name of the file
							   we created, instead of actually delivering
							   this message. */

  XP_Bool queue_for_later_p;	/* If set, we write the message to the
								   Queue folder, instead of doing delivery.
								   We also write FCC and BCC fields there
								   instead of doing them now. */

  XP_Bool encrypt_p;			/* Whether the user has requested crypto. */
  XP_Bool sign_p;

  XP_Bool attachments_only_p;	/* If set, then we don't construct a complete
								   MIME message; instead, we just retrieve the
								   attachments from the network, store them in
								   tmp files, and return a list of
								   MSG_AttachedFile structs which describe
								   them. */

  XP_Bool pre_snarfed_attachments_p;	/* If true, then the attachments were
										   loaded by msg_DownloadAttachments()
										   and therefore we shouldn't delete
										   the tmp files (but should leave
										   that to the caller.) */

  XP_Bool digest_p;			/* Whether to be multipart/digest instead of
							   multipart/mixed. */

  XP_Bool be_synchronous_p;	/* If true, we will load one URL after another,
							   rather than starting all URLs going at once
							   and letting them load in parallel.  This is
							   more efficient if (for example) all URLs are
							   known to be coming from the same news server
							   or mailbox: loading them in parallel would
							   cause multiple connections to the news server
							   to be opened, or would cause much seek()ing.
							 */

  void *crypto_closure;		/* State used by composec.c */

  /* The first attachment, if any (typed in by the user.)
   */
  char *attachment1_type;
  char *attachment1_encoding;
  MimeEncoderData *attachment1_encoder_data;
  char *attachment1_body;
  uint32 attachment1_body_length;

  /* Subsequent attachments, if any.
   */
  uint32 attachment_count;
  uint32 attachment_pending_count;
  struct mime_attachment *attachments;
  int32 status; /* in case some attachments fail but not all */

  /* The caller's `exit' method. */
  void (*message_delivery_done_callback) (MWContext *context,
										  void * fe_data, int status,
										  const char * error_msg);

  /* The exit method used when downloading attachments only. */
  void (*attachments_done_callback) (MWContext *context,
									 void * fe_data, int status,
									 const char * error_msg,
									 struct MSG_AttachedFile *attachments);

  char *msg_file_name;				/* Our temporary file */
  XP_File msg_file;
/*  char *headers;					/ * The headers of the message */

#if 0
  /* Some state to control the thermometer during message delivery.
   */
  int32 msg_size;					/* Size of the final message. */
  int32 delivery_total_bytes;		/* How many bytes we will be delivering:
									   for example, if we're sending the
									   message to both mail and news, this
									   will be 2x the size of the message.
									 */
  int32 delivery_bytes;				/* How many bytes we have delivered so far.
									 */
#endif /* 0 */
};

struct mime_attachment
{
  char *url_string;
  URL_Struct *url;
  XP_Bool done;

  struct mime_delivery_state *state;

  char *type;						/* The real type, once we know it. */
  char *override_type;				/* The type we should assume it to be
									   or 0, if we should get it from the
									   URL_Struct (from the server) */
  char *override_encoding;			/* Goes along with override_type */

  char *desired_type;				/* The type it should be converted to. */
  char *description;				/* For Content-Description header */
  char *x_mac_type, *x_mac_creator; /* Some kinda dumb-ass mac shit. */
  char *real_name;					/* The name for the headers, if different
									   from the URL. */
  char *encoding;					/* The encoding, once we've decided. */
  XP_Bool already_encoded_p;		/* If we attach a document that is already
									   encoded, we just pass it through. */

  char *file_name;					/* The temp file to which we save it */
  XP_File file;

#ifdef XP_MAC
  char *ap_filename;				/* The temp file holds the appledouble
									   encoding of the file we want to post. */
#endif

  XP_Bool decrypted_p;		/* S/MIME -- when attaching a message that was
							   encrypted, it's necessary to decrypt it first
							   (since nobody but the original recipient can
							   read it -- if you forward it to someone in the
							   raw, it will be useless to them.)  This flag
							   indicates whether decryption occurred, so that
							   libmsg can issue appropriate warnings about
							   doing a cleartext forward of a message that was
							   originally encrypted.
							 */
  uint32 size;						/* Some state used while filtering it */
  uint32 unprintable_count;
  uint32 highbit_count;
  uint32 ctl_count;
  uint32 null_count;
  uint32 current_column;
  uint32 max_column;
  uint32 lines;

  MimeEncoderData *encoder_data;	/* Opaque state for base64/qp encoder. */

  XP_Bool graph_progress_started;

  PrintSetup print_setup;			/* Used by HTML->Text and HTML->PS */
};


static void mime_attachment_url_exit (URL_Struct *url, int status,
									  MWContext *context);
static void mime_text_attachment_url_exit (PrintSetup *p);
static int mime_gather_attachments (struct mime_delivery_state *state);
static int mime_sanity_check_fields (const char *from,
									 const char *reply_to,
									 const char *to,
									 const char *cc,
									 const char *bcc,
									 const char *fcc,
									 const char *newsgroups,
									 const char *followup_to,
									 const char *subject,
									 const char *references,
									 const char *organization,
									 const char *other_random_headers);
static char * mime_generate_headers (const char *from, const char *reply_to,
									 const char *to,
									 const char *cc, const char *bcc,
									 const char *fcc,
									 const char *newsgroups,
									 const char *followup_to,
									 const char *subject,
									 const char *references,
									 const char *organization,
									 const char* message_id,
									 const char *other_random_headers,
									 int csid);
static char * mime_generate_attachment_headers (const char *type,
												const char *encoding,
												const char *description,
												const char *x_mac_type,
												const char *x_mac_creator,
												const char *real_name,
												const char *base_url,
												XP_Bool digest_p,
												int16 mail_csid);
static void mime_free_message_state (struct mime_delivery_state *state);
#if 0
static XP_Bool mime_type_conversion_possible (const char *from_type,
											  const char *to_type);
#endif
static void mime_deliver_message (struct mime_delivery_state *state);
static void mime_fail (struct mime_delivery_state *state, int failure_code,
					   char *error_msg);

static int mime_encoder_output_fn (const char *, int32, void *);
static int mime_write_message_body (struct mime_delivery_state *,
									char *buf, int32 size);

#ifdef XP_UNIX
extern void XFE_InitializePrintSetup (PrintSetup *p);
#endif /* XP_UNIX */

extern char * NET_ExplainErrorDetails (int code, ...);


char *
mime_make_separator(const char *prefix)
{
  unsigned char rand_buf[13]; 
#ifdef HAVE_SECURITY /* added by jwz */
  RNG_GenerateGlobalRandomBytes((void *) rand_buf, 12);
#else
  {
	int i;
	for (i = 0; i < 12; i++)
	  rand_buf[i] = random();
  }
#endif /* !HAVE_SECURITY -- added by jwz */
  return PR_smprintf("------------%s"
					 "%02X%02X%02X%02X"
					 "%02X%02X%02X%02X"
					 "%02X%02X%02X%02X",
					 prefix,
					 rand_buf[0], rand_buf[1], rand_buf[2], rand_buf[3],
					 rand_buf[4], rand_buf[5], rand_buf[6], rand_buf[7],
					 rand_buf[8], rand_buf[9], rand_buf[10], rand_buf[11]);
}


static int32
mime_snarf_attachment (struct mime_attachment *ma)
{
  int32 status = 0;
  XP_ASSERT (! ma->done);

  ma->file_name = WH_TempName (xpFileToPost, "nsmail");
  if (! ma->file_name)
	return (MK_OUT_OF_MEMORY);

  ma->file = XP_FileOpen (ma->file_name, xpFileToPost, XP_FILE_WRITE_BIN);
  if (! ma->file)
	return MK_UNABLE_TO_OPEN_TMP_FILE; /* #### how do we pass file name? */

  ma->url->fe_data = ma;

  /* #### ma->type is still unknown at this point.
	 We *really* need to find a way to make the textfe not blow
	 up on documents that are not text/html!!
   */
#ifdef XP_MAC
  if (NET_IsLocalFileURL(ma->url->address) &&
	  (strncasecomp(ma->url->address, "mailbox:", 8) != 0))
  {
	/* convert the apple file to the appledouble first, 
	**	and then patch the address in the url. 
	*/
	char* src_filename = NET_GetLocalFileFromURL (ma->url->address);
	
	if(isMacFile(src_filename))
	{
		char	*separator, tmp[128];
		NET_StreamClass *ad_encode_stream;
		
		separator = mime_make_separator("ad");
		if (!separator)
		  return MK_OUT_OF_MEMORY;
						
		ma->ap_filename  = WH_TempName (xpFileToPost, "nsmail");
		
		ad_encode_stream = (NET_StreamClass *)		/* need a prototype */
			fe_MakeAppleDoubleEncodeStream (FO_CACHE_AND_MAIL_TO,
	                         NULL,
	                         ma->url,
	                         ma->state->context,
	                         src_filename,
	                         ma->ap_filename,
	                         separator);	/* the file to hold the encoding */
		
		if (ad_encode_stream == NULL)
		  {
			FREEIF(separator);
			return MK_OUT_OF_MEMORY;
		  }
			
		do
		{ 
			status = (*ad_encode_stream->put_block)
				(ad_encode_stream->data_object, NULL, 1024);
		
		} while (status == noErr);
		
		if (status >= 0)
			(*ad_encode_stream->complete)(ad_encode_stream->data_object);
		else
			(*ad_encode_stream->abort)(ad_encode_stream->data_object, status);
	
		FREE(ad_encode_stream);
	
		if (status < 0)
		  {
			FREEIF(separator);
			return status;
		  }
			
		FREE(ma->url->address);
		ma->url->address = NET_GetURLFromLocalFile(ma->ap_filename);
		/*
		**	and also patch the types.
		*/ 
		if (ma->type) 
			XP_FREE (ma->type);
			
		XP_SPRINTF(tmp, MULTIPART_APPLEDOUBLE "; boundary=\"%s\"",
				   separator);
		FREEIF(separator);

		ma->type = XP_STRDUP(tmp);
	}
	else
	{
		/* make sure the file type and create are set.	*/
  		char filetype[32];
  		FSSpec	fsSpec;
  		FInfo 	info;
		Bool 	useDefault;
		char	*macType, *macEncoding;
		
  		my_FSSpecFromPathname(src_filename, &fsSpec);	
		if (FSpGetFInfo (&fsSpec, &info) == noErr)
		{
	  		XP_SPRINTF(filetype, "%X", info.fdType);
	  		ma->x_mac_type    = XP_STRDUP(filetype);
	  		
	  		XP_SPRINTF(filetype, "%X", info.fdCreator);
	  		ma->x_mac_creator = XP_STRDUP(filetype);
	  		if (ma->type == NULL || 
	  			!strcasecomp (ma->type, TEXT_PLAIN))
	  		{

#define TEXT_TYPE	0x54455854
#define text_TYPE	0x74657874

	  			if (info.fdType != TEXT_TYPE && info.fdType != text_TYPE)
	  			{
	  				FE_FileType(ma->url->address, &useDefault,
								&macType, &macEncoding);

	  				FREEIF(ma->type);
	  				ma->type = macType;
	  			}
	  		}
	  	}
	  	
	  	/* don't bother to set the types if we failed in getting the file
		   infor. */		
	}
	FREEIF(src_filename);
  }
#endif /* XP_MAC */

  if (ma->desired_type &&
	  !strcasecomp (ma->desired_type, TEXT_PLAIN) /* #### &&
	  mime_type_conversion_possible (ma->type, ma->desired_type) */ )
	{
	  /* Conversion to plain text desired.
	   */
	  ma->print_setup.url = ma->url;
	  ma->print_setup.carg = ma;
	  ma->print_setup.completion = mime_text_attachment_url_exit;
	  ma->print_setup.filename = NULL;
	  ma->print_setup.out = ma->file;
	  ma->print_setup.eol = CRLF;
	  ma->print_setup.width = 76;
	  ma->url->savedData.FormList = 0;

	  /* 5-31-96 jefft -- bug# 21519
	   * We have to check for the valid return from XL_TranslateText.
	   * ma may already been free'd if url is malformed or for some other
	   * reason. 
	   */
#ifdef _USRDLL
	  if ( ! NDLLXL_TranslateText (ma->state->context, ma->url, &ma->print_setup) )
#else
	  if ( ! XL_TranslateText (ma->state->context, ma->url, &ma->print_setup) )
#endif
		return MK_ATTACHMENT_LOAD_FAILED;

	  if (ma->type) XP_FREE (ma->type);
	  ma->type = ma->desired_type;
	  ma->desired_type = 0;
	  if (ma->encoding) XP_FREE (ma->encoding);
	  ma->encoding = 0;
	}
#ifdef XP_UNIX
  else if (ma->desired_type &&
		   !strcasecomp (ma->desired_type, APPLICATION_POSTSCRIPT) /* #### &&
		   mime_type_conversion_possible (ma->type, ma->desired_type) */ )
	{
	  /* Conversion to postscript desired.
	   */
	  XFE_InitializePrintSetup (&ma->print_setup);
	  ma->print_setup.url = ma->url;
	  ma->print_setup.carg = ma;
	  ma->print_setup.completion = mime_text_attachment_url_exit;
	  ma->print_setup.filename = NULL;
	  ma->print_setup.out = ma->file;
	  ma->print_setup.eol = CRLF;
	  ma->url->savedData.FormList = 0;
	  XL_TranslatePostscript (ma->state->context, ma->url,
							  &ma->print_setup);
	  if (ma->type) XP_FREE (ma->type);
	  ma->type = ma->desired_type;
	  ma->desired_type = 0;
	  if (ma->encoding) XP_FREE (ma->encoding);
	  ma->encoding = 0;
	}
#endif /* XP_UNIX */
  else
	{
	  int get_url_status;
	  
	  /* In this case, ignore the status, as that will be handled by
		 the exit routine. */
		 
	  /* jwz && tj -> we're assuming that it is safe to return the result
	  of this call as our status result.
	  
	  A negative result means that the exit routine was run, either because
	  the operation completed quickly or failed.
	  
	  */
	   get_url_status = NET_GetURL (ma->url, FO_CACHE_AND_MAIL_TO,
				   ma->state->context, mime_attachment_url_exit);
				   
	   if (get_url_status < 0)
	   	 return MK_ATTACHMENT_LOAD_FAILED;
	   else
	   	 return 0;
	}
  return status;
}


static void
mime_attachment_url_exit (URL_Struct *url, int status, MWContext *context)
{
  struct mime_attachment *ma = (struct mime_attachment *) url->fe_data;
  char *error_msg = url->error_msg;
  url->error_msg = 0;
  url->fe_data = 0;

  XP_ASSERT(ma != NULL);
  XP_ASSERT(ma->state != NULL);
  XP_ASSERT(ma->state->context != NULL);
  XP_ASSERT(ma->url != NULL);
  
  if (ma->graph_progress_started)
	{
	  ma->graph_progress_started = FALSE;
	  FE_GraphProgressDestroy (ma->state->context, ma->url,
							   ma->url->content_length, ma->size);
	}

  if (status < 0)
	/* If any of the attachment URLs fail, kill them all. */
	NET_InterruptWindow (context);

  /* Close the file, but don't delete it (or the file name.) */
  XP_FileClose (ma->file);
  ma->file = 0;
  NET_FreeURLStruct (ma->url);
  /* I'm pretty sure ma->url == url */
  ma->url = 0;
  url = 0;

  if (status < 0 && ma->state->status >= 0)
	ma->state->status = status;

  ma->done = TRUE;

  XP_ASSERT (ma->state->attachment_pending_count > 0);
  ma->state->attachment_pending_count--;

  if (status >= 0 && ma->state->be_synchronous_p)
	{
	  /* Find the next attachment which has not yet been loaded,
		 if any, and start it going.
		 */
	  int i;
	  struct mime_attachment *next = 0;
	  for (i = 0; i < ma->state->attachment_count; i++)
		if (!ma->state->attachments[i].done)
		  {
			next = &ma->state->attachments[i];
			break;
		  }
	  if (next)
		{
		  int status = mime_snarf_attachment (next);
		  if (status < 0)
			{
			  mime_fail (ma->state, status, 0);
			  return;
			}
		}
	}

  if (ma->state->attachment_pending_count == 0)
	{
	  /* If this is the last attachment, then either complete the
		 delivery (if successful) or report the error by calling
		 the exit routine and terminating the delivery.
	   */
	  if (status < 0)
		{
		  mime_fail (ma->state, status, error_msg);
		  error_msg = 0;
		}
	  else
		{
		  mime_gather_attachments (ma->state);
		}
	}
  else
	{
	  /* If this is not the last attachment, but it got an error,
		 then report that error and continue (we won't actually
		 abort the delivery until all the other pending URLs have
		 caught up with the NET_InterruptWindow() we did up above.)
	   */
	  if (status < 0 && error_msg)
		FE_Alert (context, error_msg);
	}
  FREEIF (error_msg);
}


static void
mime_text_attachment_url_exit (PrintSetup *p)
{
  struct mime_attachment *ma = (struct mime_attachment *) p->carg;
  XP_ASSERT (p->url == ma->url);
  ma->url->fe_data = ma;  /* grr */
  mime_attachment_url_exit (p->url, p->status, ma->state->context);
}


PRIVATE unsigned int
mime_attachment_stream_write_ready (void *closure)
{
  return MAX_WRITE_READY;
}

PRIVATE int
mime_attachment_stream_write (void *closure, const char *block, int32 length)
{
  struct mime_attachment *ma = (struct mime_attachment *) closure;
  const unsigned char *s;
  const unsigned char *end;

  if (ma->state->status < 0)
	return ma->state->status;

  ma->size += length;

  if (!ma->graph_progress_started)
	{
	  ma->graph_progress_started = TRUE;
	  FE_GraphProgressInit (ma->state->context, ma->url,
							ma->url->content_length);
	}

  FE_GraphProgress (ma->state->context, ma->url,
					ma->size, length, ma->url->content_length);


  /* Copy out the content type and encoding if we haven't already.
   */
  if (!ma->type && ma->url->content_type)
	{
	  ma->type = XP_STRDUP (ma->url->content_type);

	  /* If the URL has an encoding, and it's not one of the "null" encodings,
		 then keep it. */
	  if (ma->url->content_encoding &&
		  strcasecomp (ma->url->content_encoding, ENCODING_7BIT) &&
		  strcasecomp (ma->url->content_encoding, ENCODING_8BIT) &&
		  strcasecomp (ma->url->content_encoding, ENCODING_BINARY))
		{
		  if (ma->encoding) XP_FREE (ma->encoding);
		  ma->encoding = XP_STRDUP (ma->url->content_encoding);
		  ma->already_encoded_p = TRUE;
		}

	  /* Make sure there's a string in the type field.
		 Note that UNKNOWN_CONTENT_TYPE and APPLICATION_OCTET_STREAM are
		 different; "octet-stream" means that this document was *specified*
		 as an anonymous binary type; "unknown" means that we will guess
		 whether it is text or binary based on its contents.
	   */
	  if (!ma->type || !*ma->type)
		StrAllocCopy (ma->type, UNKNOWN_CONTENT_TYPE);


#ifdef XP_WIN
	  /* This bogosity thanks to Garrett, our resident ZZ-Top fan.
		 WinFE tends to spew out bogus internal "zz-" types for things
		 it doesn't know, so map those to the "real" unknown type.
	   */
	  if (ma->type && !strncasecomp (ma->type, "zz-", 3))
		StrAllocCopy (ma->type, UNKNOWN_CONTENT_TYPE);
#endif /* XP_WIN */

	  /* This bogosity thanks to the server folks, and their co-contipators
		 at SGI.  There are some of these stupid "magnus" types in the default
		 mime.types file that SGI ships in /usr/local/lib/netscape/.  These
		 types are meaningless to the end user, and the server never returns
		 them, but they're getting attached to local .cgi files anyway.
		 Losers.  Nuke 'em.
	   */
	  if (ma->type && !strncasecomp (ma->type, "magnus-internal/", 16))
		StrAllocCopy (ma->type, UNKNOWN_CONTENT_TYPE);


	  /* #### COMPLETE HORRIFIC KLUDGE
		 It totally sucks that the URL_Struct shares the `encoding' slot
		 amongst the Content-Encoding and Content-Transfer-Encoding headers.
		 Content-Transfer-Encoding is required to be one of the standard
		 MIME encodings (x- types are explicitly discourgaged.)  But
		 Content-Encoding can be anything (it's HTTP, not MIME.)

		 So, to prevent binary compressed data from getting dumped into the
		 mail stream, we special case some things here.  If the encoding is
		 "x-compress" or "x-gzip", then that must have come from a
		 Content-Encoding header, So change the type to application/x-compress
		 and allow it to be encoded in base64.

		 But what if it's something we don't know?  In that case, we just
		 dump it into the mail.  For Content-Transfer-Encodings, like for
		 example, x-uuencode, that's appropriate.  But for Content-Encodings,
		 like for example, x-some-brand-new-binary-compression-algorithm,
		 that's wrong.
	   */
	  if (ma->encoding &&
		  (!strcasecomp (ma->encoding, ENCODING_COMPRESS) ||
		   !strcasecomp (ma->encoding, ENCODING_COMPRESS2)))
		{
		  StrAllocCopy (ma->type, APPLICATION_COMPRESS);
		  StrAllocCopy (ma->encoding, ENCODING_BINARY);
		  ma->already_encoded_p = FALSE;
		}
	  else if (ma->encoding &&
			   (!strcasecomp (ma->encoding, ENCODING_GZIP) ||
				!strcasecomp (ma->encoding, ENCODING_GZIP2)))
		{
		  StrAllocCopy (ma->type, APPLICATION_GZIP);
		  StrAllocCopy (ma->encoding, ENCODING_BINARY);
		  ma->already_encoded_p = FALSE;
		}

	  /* If the caller has passed in an overriding type for this URL,
		 then ignore what the netlib guessed it to be.  This is so that
		 we can hand it a file:/some/tmp/file and tell it that it's of
		 type message/rfc822 without having to depend on that tmp file
		 having some particular extension.
	   */
	  if (ma->override_type)
		{
		  StrAllocCopy (ma->type, ma->override_type);
		  if (ma->override_encoding)
			StrAllocCopy (ma->encoding, ma->override_encoding);
		}
	}

  /* Cumulatively examine the data that is passing through this stream.
   */
  s = (unsigned char *) block;
  end = s + length;
  for (; s < end; s++)
	{
	  if (*s > 126)
		{
		  ma->highbit_count++;
		  ma->unprintable_count++;
		}
	  else if (*s < ' ' && *s != '\t' && *s != CR && *s != LF)
		{
		  ma->unprintable_count++;
		  ma->ctl_count++;
		  if (*s == 0)
			ma->null_count++;
		}

	  if (*s == CR || *s == LF)
		{
		  if (s+1 < end && s[0] == CR && s[1] == LF)
			s++;
		  if (ma->max_column < ma->current_column)
			ma->max_column = ma->current_column;
		  ma->current_column = 0;
		  ma->lines++;
		}
	  else
		{
		  ma->current_column++;
		}
	}

  /* Write it to the file.
   */
  while (length > 0)
	{
	  int32 l;
	  l = XP_FileWrite (block, length, ma->file);
	  if (l < length)
		{
		  ma->state->status = MK_MIME_ERROR_WRITING_FILE;
		  return ma->state->status;
		}
	  block += l;
	  length -= l;
	}

  return 1;
}

PRIVATE void
mime_attachment_stream_complete (void *closure)
{
  /* Nothing to do here - the URL exit method does our cleanup. */
}

PRIVATE void
mime_attachment_stream_abort (void *closure, int status)
{
  struct mime_attachment *ma = (struct mime_attachment *) closure;

  if (ma->state->status >= 0)
	ma->state->status = status;

  /* Nothing else to do here - the URL exit method does our cleanup. */
}


static NET_StreamClass *
mime_make_attachment_stream (int format_out, void *closure,
							 URL_Struct *url, MWContext *context)
{
  NET_StreamClass *stream;
    
  TRACEMSG(("Setting up attachment stream. Have URL: %s\n", url->address));

  stream = XP_NEW (NET_StreamClass);
  if (stream == NULL) 
	return (NULL);

  XP_MEMSET (stream, 0, sizeof (NET_StreamClass));

  stream->name           = "attachment stream";
  stream->complete       = mime_attachment_stream_complete;
  stream->abort          = mime_attachment_stream_abort;
  stream->put_block      = mime_attachment_stream_write;
  stream->is_write_ready = mime_attachment_stream_write_ready;
  stream->data_object    = url->fe_data;
  stream->window_id      = context;

  TRACEMSG(("Returning stream from mime_make_attachment_stream"));

  return stream;
}


/* This is how libmime/mimemoz.c lets us know that the message was decrypted.
   Oh, the humanity. */
void
MSG_MimeNotifyCryptoAttachmentKludge(NET_StreamClass *stream)
{
  struct mime_attachment *ma;
  if (!stream || !stream->data_object)
	{
	  XP_ASSERT(0);
	  return;
	}

  ma = (struct mime_attachment *) stream->data_object;

  /* I'm nervous -- do some checks to make sure this void* really smells
	 like a mime_attachment struct. */
  if (!ma->state ||
	  !ma->state->context ||
	  ma->state->context != stream->window_id)
	{
	  XP_ASSERT(0);
	  return;
	}

  /* Well ok then. */
  ma->decrypted_p = TRUE;
}



void
MSG_RegisterConverters (void)
{
  NET_RegisterContentTypeConverter ("*", FO_MAIL_TO,
									NULL, mime_make_attachment_stream);
  NET_RegisterContentTypeConverter ("*", FO_CACHE_AND_MAIL_TO,
									NULL, mime_make_attachment_stream);

  /* FO_MAIL_MESSAGE_TO is treated the same as FO_MAIL_TO -- this format_out
	 just means that libmime has already gotten its hands on this document
	 (which happens to be of type message/rfc822 or message/news) and has
	 altered it in some way (for example, has decrypted it.) */
  NET_RegisterContentTypeConverter ("*", FO_MAIL_MESSAGE_TO,
									NULL, mime_make_attachment_stream);
  NET_RegisterContentTypeConverter ("*", FO_CACHE_AND_MAIL_MESSAGE_TO,
									NULL, mime_make_attachment_stream);

  /* Attachment of mail and news messages happens slightly differently:
	 Rather than FO_MAIL_TO going in to mime_make_attachment_stream, it
	 goes into MIME_MessageConverter, which will then open a later stream
	 with FO_MAIL_MESSAGE_TO -- which is how it eventually gets into
	 mime_make_attachment_stream, after having been frobbed by libmime.
   */
  NET_RegisterContentTypeConverter (MESSAGE_RFC822, FO_MAIL_TO,
									NULL, MIME_MessageConverter);
  NET_RegisterContentTypeConverter (MESSAGE_NEWS, FO_MAIL_TO,
									NULL, MIME_MessageConverter);
  NET_RegisterContentTypeConverter (MESSAGE_RFC822, FO_CACHE_AND_MAIL_TO,
									NULL, MIME_MessageConverter);
  NET_RegisterContentTypeConverter (MESSAGE_NEWS, FO_CACHE_AND_MAIL_TO,
									NULL, MIME_MessageConverter);


  /* This is for newsgrp.c */
  NET_RegisterContentTypeConverter ("*", FO_APPEND_TO_FOLDER, 0,
									msg_MakeAppendToFolderStream);
  NET_RegisterContentTypeConverter ("*", FO_CACHE_AND_APPEND_TO_FOLDER, 0,
									msg_MakeAppendToFolderStream);

  /* Decoders from mimehtml.c for message/rfc822 */
  NET_RegisterContentTypeConverter (MESSAGE_RFC822, FO_PRESENT,
									NULL, MIME_MessageConverter);
  NET_RegisterContentTypeConverter (MESSAGE_RFC822, FO_PRINT,
									NULL, MIME_MessageConverter);
  NET_RegisterContentTypeConverter (MESSAGE_RFC822, FO_QUOTE_MESSAGE,
									NULL, MIME_MessageConverter);
  NET_RegisterContentTypeConverter (MESSAGE_RFC822, FO_SAVE_AS,
									NULL, MIME_MessageConverter);
  NET_RegisterContentTypeConverter (MESSAGE_RFC822, FO_SAVE_AS_TEXT,
									NULL, MIME_MessageConverter);
  NET_RegisterContentTypeConverter (MESSAGE_RFC822, FO_INTERNAL_IMAGE,
									NULL, MIME_MessageConverter);
#ifdef XP_UNIX
  NET_RegisterContentTypeConverter (MESSAGE_RFC822, FO_SAVE_AS_POSTSCRIPT,
									NULL, MIME_MessageConverter);
#endif /* XP_UNIX */

  /* Decoders from mimehtml.c for message/news (same as message/rfc822) */
  NET_RegisterContentTypeConverter (MESSAGE_NEWS, FO_PRESENT,
									NULL, MIME_MessageConverter);
  NET_RegisterContentTypeConverter (MESSAGE_NEWS, FO_PRINT,
									NULL, MIME_MessageConverter);
  NET_RegisterContentTypeConverter (MESSAGE_NEWS, FO_QUOTE_MESSAGE,
									NULL, MIME_MessageConverter);
  NET_RegisterContentTypeConverter (MESSAGE_NEWS, FO_SAVE_AS,
									NULL, MIME_MessageConverter);
  NET_RegisterContentTypeConverter (MESSAGE_NEWS, FO_SAVE_AS_TEXT,
									NULL, MIME_MessageConverter);
  NET_RegisterContentTypeConverter (MESSAGE_NEWS, FO_INTERNAL_IMAGE,
									NULL, MIME_MessageConverter);
#ifdef XP_UNIX
  NET_RegisterContentTypeConverter (MESSAGE_NEWS, FO_SAVE_AS_POSTSCRIPT,
									NULL, MIME_MessageConverter);
#endif /* XP_UNIX */

  /* Decoders from mimehtml.c for text/richtext and text/enriched */
  NET_RegisterContentTypeConverter (TEXT_RICHTEXT, FO_PRESENT,
									NULL, MIME_RichtextConverter);
  NET_RegisterContentTypeConverter (TEXT_RICHTEXT, FO_PRINT,
									NULL, MIME_RichtextConverter);
  NET_RegisterContentTypeConverter (TEXT_ENRICHED, FO_PRESENT,
									NULL, MIME_EnrichedTextConverter);
  NET_RegisterContentTypeConverter (TEXT_ENRICHED, FO_PRINT,
									NULL, MIME_EnrichedTextConverter);
}


static XP_Bool
mime_7bit_data_p (const char *string, uint32 size)
{
  const unsigned char *s = (const unsigned char *) string;
  const unsigned char *end = s + size;
  if (s)
	for (; s < end; s++)
	  if (*s > 0x7F)
		return FALSE;
  return TRUE;
}


static int
mime_sanity_check_fields (const char *from,
						  const char *reply_to,
						  const char *to,
						  const char *cc,
						  const char *bcc,
						  const char *fcc,
						  const char *newsgroups,
						  const char *followup_to,
						  const char *subject,
						  const char *references,
						  const char *organization,
						  const char *other_random_headers)
{
  if (from) while (XP_IS_SPACE (*from)) from++;
  if (reply_to) while (XP_IS_SPACE (*reply_to)) reply_to++;
  if (to) while (XP_IS_SPACE (*to)) to++; 
  if (cc) while (XP_IS_SPACE (*cc)) cc++; 
  if (bcc) while (XP_IS_SPACE (*bcc)) bcc++; 
  if (fcc) while (XP_IS_SPACE (*fcc)) fcc++; 
  if (newsgroups) while (XP_IS_SPACE (*newsgroups)) newsgroups++;
  if (followup_to) while (XP_IS_SPACE (*followup_to)) followup_to++;

  /* #### sanity check other_random_headers for newline conventions */

  if (!from || !*from)
	return MK_MIME_NO_SENDER;
  else if ((!to || !*to) && (!newsgroups || !*newsgroups))
	return MK_MIME_NO_RECIPIENTS;
  else
	return 0;
}


/* Strips whitespace, and expands newlines into newline-tab for use in
   mail headers.  Returns a new string or 0 (if it would have been empty.)
   If addr_p is true, the addresses will be parsed and reemitted as
   rfc822 mailboxes.
 */
static char *
mime_fix_header_1 (const char *string,
				   XP_Bool addr_p, XP_Bool news_p)
{
  char *new_string;
  const char *in;
  char *out;
  int32 i, old_size, new_size;
  if (!string || !*string)
	return 0;

  if (addr_p)
	{
	  char *n = MSG_ReformatRFC822Addresses (string);
	  if (n) return n;
	}

  old_size = XP_STRLEN (string);
  new_size = old_size;
  for (i = 0; i < old_size; i++)
	if (string[i] == CR || string[i] == LF)
	  new_size += 2;

  new_string = (char *) XP_ALLOC (new_size + 1);
  if (! new_string)
	return 0;

  in  = string;
  out = new_string;

  /* strip leading whitespace. */
  while (XP_IS_SPACE (*in))
	in++;

  /* replace CR, LF, or CRLF with CRLF-TAB. */
  while (*in)
	{
	  if (*in == CR || *in == LF)
		{
		  if (*in == CR && in[1] == LF)
			in++;
		  in++;
		  *out++ = CR;
		  *out++ = LF;
		  *out++ = '\t';
		}
	  else if (news_p && *in == ',')
		{
		  *out++ = *in++;
		  /* skip over all whitespace after a comma. */
		  while (XP_IS_SPACE (*in))
			in++;
		}
	  else
		{
		  *out++ = *in++;
		}
	}
  *out = 0;

  /* strip trailing whitespace. */
  while (out > in && XP_IS_SPACE (out[-1]))
	*out-- = 0;

  /* If we ended up throwing it all away, use 0 instead of "". */
  if (!*new_string)
	{
	  XP_FREE (new_string);
	  new_string = 0;
	}

  return new_string;
}

static char *
mime_fix_header (const char *string)
{
  return mime_fix_header_1 (string, FALSE, FALSE);
}

static char *
mime_fix_addr_header (const char *string)
{
  return mime_fix_header_1 (string, TRUE, FALSE);
}

static char *
mime_fix_news_header (const char *string)
{
  return mime_fix_header_1 (string, FALSE, TRUE);
}

#if 0
static XP_Bool
mime_type_conversion_possible (const char *from_type, const char *to_type)
{
  if (! to_type)
	return TRUE;

  if (! from_type)
	return FALSE;

  if (!strcasecomp (from_type, to_type))
	/* Don't run text/plain through the text->html converter. */
	return FALSE;

  if ((!strcasecomp (from_type, INTERNAL_PARSER) ||
	   !strcasecomp (from_type, TEXT_HTML) ||
	   !strcasecomp (from_type, TEXT_MDL)) &&
	  !strcasecomp (to_type, TEXT_PLAIN))
	/* Don't run UNKNOWN_CONTENT_TYPE through the text->html converter
	   (treat it as text/plain already.) */
	return TRUE;

#ifdef XP_UNIX
  if ((!strcasecomp (from_type, INTERNAL_PARSER) ||
	   !strcasecomp (from_type, TEXT_PLAIN) ||
	   !strcasecomp (from_type, TEXT_HTML) ||
	   !strcasecomp (from_type, TEXT_MDL) ||
	   !strcasecomp (from_type, IMAGE_GIF) ||
	   !strcasecomp (from_type, IMAGE_JPG) ||
	   !strcasecomp (from_type, IMAGE_PJPG) ||
	   !strcasecomp (from_type, IMAGE_XBM) ||
	   !strcasecomp (from_type, IMAGE_XBM2) ||
	   !strcasecomp (from_type, IMAGE_XBM3) ||
	   /* always treat unknown content types as text/plain */
	   !strcasecomp (from_type, UNKNOWN_CONTENT_TYPE)
	   ) &&
	  !strcasecomp (to_type, APPLICATION_POSTSCRIPT))
	return TRUE;
#endif /* XP_UNIX */

  return FALSE;
}
#endif

static XP_Bool
mime_type_requires_b64_p (const char *type)
{
  if (!type || !strcasecomp (type, UNKNOWN_CONTENT_TYPE))
	/* Unknown types don't necessarily require encoding.  (Note that
	   "unknown" and "application/octet-stream" aren't the same.) */
	return FALSE;

  else if (!strncasecomp (type, "image/", 6) ||
		   !strncasecomp (type, "audio/", 6) ||
		   !strncasecomp (type, "video/", 6) ||
		   !strncasecomp (type, "application/", 12))
	{
	  /* The following types are application/ or image/ types that are actually
		 known to contain textual data (meaning line-based, not binary, where
		 CRLF conversion is desired rather than disasterous.)  So, if the type
		 is any of these, it does not *require* base64, and if we do need to
		 encode it for other reasons, we'll probably use quoted-printable.
		 But, if it's not one of these types, then we assume that any subtypes
		 of the non-"text/" types are binary data, where CRLF conversion would
		 corrupt it, so we use base64 right off the bat.

		 The reason it's desirable to ship these as text instead of just using
		 base64 all the time is mainly to preserve the readability of them for
		 non-MIME users: if I mail a /bin/sh script to someone, it might not
		 need to be encoded at all, so we should leave it readable if we can.

		 This list of types was derived from the comp.mail.mime FAQ, section
		 10.2.2, "List of known unregistered MIME types" on 2-Feb-96.
	   */
	  static const char *app_and_image_types_which_are_really_text[] = {
		"application/mac-binhex40",		/* APPLICATION_BINHEX */
		"application/pgp",				/* APPLICATION_PGP */
		"application/x-pgp-message",	/* APPLICATION_PGP2 */
		"application/postscript",		/* APPLICATION_POSTSCRIPT */
		"application/x-uuencode",		/* APPLICATION_UUENCODE */
		"application/x-uue",			/* APPLICATION_UUENCODE2 */
		"application/uue",				/* APPLICATION_UUENCODE4 */
		"application/uuencode",			/* APPLICATION_UUENCODE3 */
		"application/sgml",
		"application/x-csh",
		"application/x-javascript",
		"application/x-latex",
		"application/x-macbinhex40",
		"application/x-ns-proxy-autoconfig",
		"application/x-www-form-urlencoded",
		"application/x-perl",
		"application/x-sh",
		"application/x-shar",
		"application/x-tcl",
		"application/x-tex",
		"application/x-texinfo",
		"application/x-troff",
		"application/x-troff-man",
		"application/x-troff-me",
		"application/x-troff-ms",
		"application/x-troff-ms",
		"application/x-wais-source",
		"image/x-bitmap",
		"image/x-pbm",
		"image/x-pgm",
		"image/x-portable-anymap",
		"image/x-portable-bitmap",
		"image/x-portable-graymap",
		"image/x-portable-pixmap",		/* IMAGE_PPM */
		"image/x-ppm",
		"image/x-xbitmap",				/* IMAGE_XBM */
		"image/x-xbm",					/* IMAGE_XBM2 */
		"image/xbm",					/* IMAGE_XBM3 */
		"image/x-xpixmap",
		"image/x-xpm",
		0 };
	  const char **s;
	  for (s = app_and_image_types_which_are_really_text; *s; s++)
		if (!strcasecomp (type, *s))
		  return FALSE;

	  /* All others must be assumed to be binary formats, and need Base64. */
	  return TRUE;
	}

  else
	return FALSE;
}


/* Given a content-type and some info about the contents of the document,
   decide what encoding it should have. 
 */
static int
mime_pick_encoding (struct mime_attachment *ma, int16 mail_csid)
{
  if (ma->already_encoded_p)
	goto DONE;

  if (mime_type_requires_b64_p (ma->type))
	{
	  /* If the content-type is "image/" or something else known to be binary,
		 always use base64 (so that we don't get confused by newline
		 conversions.)
	   */
	  StrAllocCopy (ma->encoding, ENCODING_BASE64);
	}
  else
	{
	  /* Otherwise, we need to pick an encoding based on the contents of
		 the document.
	   */

	  XP_Bool encode_p;

	  if (ma->max_column > 900)
		encode_p = TRUE;
	  else if (mime_use_quoted_printable_p && ma->unprintable_count)
		encode_p = TRUE;

	  else if (ma->null_count)	/* If there are nulls, we must always encode,
								   because sendmail will blow up. */
		encode_p = TRUE;
#if 0
	  else if (ma->ctl_count)	/* Should we encode if random other control
								   characters are present?  Probably... */
		encode_p = TRUE;
#endif
	  else
		encode_p = FALSE;

	  /* MIME requires a special case that these types never be encoded.
		 It also doesn't really work to convert them to text, but that's
		 because the whole architecture here is such a piece of junk.
	   */
	  if (!strncasecomp (ma->type, "message", 7) ||
		  !strncasecomp (ma->type, "multipart", 9))
		{
		  encode_p = FALSE;
		  if (ma->desired_type && !strcasecomp (ma->desired_type, TEXT_PLAIN))
			{
			  XP_FREE (ma->desired_type);
			  ma->desired_type = 0;
			}
		}
	  /* if the Mail csid is STATFUL ( CS_JIS or CS_2022_KR)
		we should force it to use 7 bit */
#ifdef XP_WIN
	  if((mail_csid & STATEFUL) &&
         ((strcasecomp(ma->type, TEXT_HTML) == 0) ||     
          (strcasecomp(ma->type, TEXT_MDL) == 0) ||
          (strcasecomp(ma->type, TEXT_PLAIN) == 0) ||
          (strcasecomp(ma->type, TEXT_RICHTEXT) == 0) ||
          (strcasecomp(ma->type, TEXT_ENRICHED) == 0) ||
          (strcasecomp(ma->type, MESSAGE_RFC822) == 0) ||
          (strcasecomp(ma->type, MESSAGE_NEWS) == 0)))
      {
        StrAllocCopy (ma->encoding, ENCODING_7BIT);
      }
#else
	  if(mail_csid & STATEFUL)
		StrAllocCopy (ma->encoding, ENCODING_7BIT);
#endif
	  else if (encode_p &&
			   ma->size > 500 &&
			   ma->unprintable_count > (ma->size / 10))
		/* If the document contains more than 10% unprintable characters,
		   then that seems like a good candidate for base64 instead of
		   quoted-printable.
		 */
		StrAllocCopy (ma->encoding, ENCODING_BASE64);
	  else if (encode_p)
		StrAllocCopy (ma->encoding, ENCODING_QUOTED_PRINTABLE);
	  else if (ma->highbit_count > 0)
		StrAllocCopy (ma->encoding, ENCODING_8BIT);
	  else
		StrAllocCopy (ma->encoding, ENCODING_7BIT);
	}


  /* Now that we've picked an encoding, initialize the filter.
   */
  XP_ASSERT(!ma->encoder_data);
  if (!strcasecomp(ma->encoding, ENCODING_BASE64))
	{
	  ma->encoder_data = MimeB64EncoderInit(mime_encoder_output_fn, ma->state);
	  if (!ma->encoder_data) return MK_OUT_OF_MEMORY;
	}
  else if (!strcasecomp(ma->encoding, ENCODING_QUOTED_PRINTABLE))
	{
	  ma->encoder_data = MimeQPEncoderInit(mime_encoder_output_fn, ma->state);
	  if (!ma->encoder_data) return MK_OUT_OF_MEMORY;
	}
  else
	{
	  ma->encoder_data = 0;
	}


  /* Do some cleanup for documents with unknown content type.

	 There are two issues: how they look to MIME users, and how they look to
	 non-MIME users.

	 If the user attaches a "README" file, which has unknown type because it
	 has no extension, we still need to send it with no encoding, so that it
	 is readable to non-MIME users.

	 But if the user attaches some random binary file, then base64 encoding
	 will have been chosen for it (above), and in this case, it won't be
	 immediately readable by non-MIME users.  However, if we type it as
	 text/plain instead of application/octet-stream, it will show up inline
	 in a MIME viewer, which will probably be ugly, and may possibly have
	 bad charset things happen as well.

	 So, the heuristic we use is, if the type is unknown, then the type is
	 set to application/octet-stream for data which needs base64 (binary data)
	 and is set to text/plain for data which didn't need base64 (unencoded or
	 lightly encoded data.)
   */
 DONE:
  if (!ma->type || !*ma->type || !strcasecomp(ma->type, UNKNOWN_CONTENT_TYPE))
	{
	  if (ma->already_encoded_p)
		StrAllocCopy (ma->type, APPLICATION_OCTET_STREAM);
	  else if (ma->encoding && !strcasecomp(ma->encoding, ENCODING_BASE64))
		StrAllocCopy (ma->type, APPLICATION_OCTET_STREAM);
	  else
		StrAllocCopy (ma->type, TEXT_PLAIN);
	}
  return 0;
}


/* Some types should have a "charset=" parameter, and some shouldn't.
   This is what decides. */
static XP_Bool
mime_type_needs_charset (const char *type)
{
  /* Only text types should have charset. */
  if (!type || !*type)
	return FALSE;
  else if (!strncasecomp (type, "text", 4))
	return TRUE;
  else
	return FALSE;
}


static char *mime_mailto_stream_read_buffer = 0;
static char *mime_mailto_stream_write_buffer = 0;
#define MIME_BUFFER_SIZE 4096

static int32
mime_encode_block (struct mime_delivery_state *state,
				   MimeEncoderData *encoder_data,
				   const char *block, uint32 block_size)
{
  if (encoder_data)
	{
	  /* Feed it through the encoder. */
	  return MimeEncoderWrite(encoder_data, block, block_size);
	}
  else
	{
	  /* Merely translate all linebreaks to CRLF.
	   */
	  int status = 0;
	  const char *in = block;
	  const char *end = in + block_size;
	  char *buffer, *out;

	  /* Kludge to avoid having to allocate memory on the toy computers... */
	  if (! mime_mailto_stream_write_buffer)
		mime_mailto_stream_write_buffer = (char *) XP_ALLOC (MIME_BUFFER_SIZE);
	  buffer = mime_mailto_stream_write_buffer;
	  if (! buffer) return MK_OUT_OF_MEMORY;

	  XP_ASSERT (block != buffer);
	  out = buffer;

	  for (; in < end; in++)
		{
		  if (*in == CR || *in == LF)
			{
			  /* Write out the newline. */
			  *out++ = CR;
			  *out++ = LF;

			  status = mime_write_message_body(state, buffer, out - buffer);
			  if (status < 0) return status;
			  out = buffer;

			  /* If its CRLF, swallow two chars instead of one. */
			  if (in[0] == CR && in[1] == LF)
				in++;

			  out = buffer;
			}
		  else
			{
			  *out++ = *in;
			}
		}

	  /* Flush the last line. */
	  if (out > buffer)
		{
		  status = mime_write_message_body(state, buffer, out - buffer);
		  if (status < 0) return status;
		  out = buffer;
		}
	}

  return 0;
}


#define PUSH_STRING(S) \
 do { XP_STRCPY (buffer_tail, S); buffer_tail += XP_STRLEN (S); } while(0)
#define PUSH_NEWLINE() \
 do { *buffer_tail++ = CR; *buffer_tail++ = LF; *buffer_tail = '\0'; } while(0)


/* All of the desired attachments have been written to individual temp files,
   and we know what's in them.  Now we need to make a final temp file of the
   actual mail message, containing all of the other files after having been
   encoded as appropriate.
 */
static int
mime_gather_attachments (struct mime_delivery_state *state)
{
  int32 status, i;
  char *headers = 0;
  char *separator = 0;
  XP_File in_file = 0;
  XP_Bool multipart_p = FALSE;
  char *buffer = 0;
  char *buffer_tail = 0;
  char* error_msg = 0;
  int16 win_csid	 = INTL_DefaultWinCharSetID(state->context);
  int16 mail_csid 	 = INTL_DefaultMailCharSetID(win_csid);

  status = state->status;
  if (status < 0)
	goto FAIL;


  if (state->attachments_only_p)
	{
	  /* If we get here, we shouldn't have the "generating a message" cb. */
	  XP_ASSERT(!state->dont_deliver_p &&
				!state->message_delivery_done_callback);
	  if (state->attachments_done_callback)
		{
		  struct MSG_AttachedFile *attachments;

		  XP_ASSERT(state->attachment_count > 0);
		  if (state->attachment_count <= 0)
			{
			  state->attachments_done_callback (state->context,
												state->fe_data, 0, 0,
												0);
			  state->attachments_done_callback = 0;
			  mime_free_message_state (state);
			  state = 0;
			  goto FAIL;
			}

		  attachments = (struct MSG_AttachedFile *)
			XP_ALLOC((state->attachment_count + 1) * sizeof(*attachments));

		  if (!attachments)
			{
			  status = MK_OUT_OF_MEMORY;
			  goto FAIL;
			}
		  XP_MEMSET(attachments, 0, ((state->attachment_count + 1)
									 * sizeof(*attachments)));
		  for (i = 0; i < state->attachment_count; i++)
			{
			  struct mime_attachment *ma = &state->attachments[i];

#undef SNARF
#define SNARF(x,y) do { if((y) && *(y) && !(x)) { ((x) = (y)); ((y) = 0); }} \
				   while(0)
			  /* Rather than copying the strings and dealing with allocation
				 failures, we'll just "move" them into the other struct (this
				 should be ok since this file uses FREEIF when discarding
				 the mime_attachment objects.) */
			  SNARF(attachments[i].orig_url, ma->url_string);
			  SNARF(attachments[i].file_name, ma->file_name);
			  SNARF(attachments[i].type, ma->type);
			  SNARF(attachments[i].encoding, ma->encoding);
			  SNARF(attachments[i].description, ma->description);
			  SNARF(attachments[i].x_mac_type, ma->x_mac_type);
			  SNARF(attachments[i].x_mac_creator, ma->x_mac_creator);
#undef SNARF
			  attachments[i].decrypted_p = ma->decrypted_p;
			  attachments[i].size = ma->size;
			  attachments[i].unprintable_count = ma->unprintable_count;
			  attachments[i].highbit_count = ma->highbit_count;
			  attachments[i].ctl_count = ma->ctl_count;
			  attachments[i].null_count = ma->null_count;
			  attachments[i].max_line_length = ma->max_column;

			  /* Doesn't really matter, but let's not lie about encoding
				 in case it does someday. */
			  if (attachments[i].highbit_count > 0 &&
				  attachments[i].encoding &&
				  !strcasecomp(attachments[i].encoding, ENCODING_7BIT))
				StrAllocCopy (attachments[i].encoding, ENCODING_8BIT);
			}

		  /* The callback is expected to free the attachments list and all
			 the strings in it.   It's also expected to delete the files!
		   */
		  state->attachments_done_callback (state->context,
											state->fe_data, 0, 0,
											attachments);
		  state->attachments_done_callback = 0;
		  mime_free_message_state (state);
		  state = 0;
		}
	  goto FAIL;
	}


  /* If we get here, we're generating a message, so there shouldn't be an
	 attachments callback. */
  XP_ASSERT(!state->attachments_done_callback);


  /* Kludge to avoid having to allocate memory on the toy computers... */
  if (! mime_mailto_stream_write_buffer)
	mime_mailto_stream_write_buffer = (char *) XP_ALLOC (MIME_BUFFER_SIZE);
  buffer = mime_mailto_stream_write_buffer;
  if (! buffer) return MK_OUT_OF_MEMORY;

  buffer_tail = buffer;

  XP_ASSERT (state->attachment_pending_count == 0);

  FE_Progress(state->context, XP_GetString(MK_MSG_ASSEMBLING_MSG));


  /* First, open the message file.
   */
  state->msg_file_name = WH_TempName (xpFileToPost, "nsmail");
  if (! state->msg_file_name)
	{
	  status = MK_OUT_OF_MEMORY;
	  goto FAIL;
	}
  state->msg_file = XP_FileOpen (state->msg_file_name, xpFileToPost,
								 XP_FILE_WRITE_BIN);
  if (! state->msg_file)
	{
	  status = MK_UNABLE_TO_OPEN_TMP_FILE;
	  error_msg = PR_smprintf(XP_GetString(status), state->msg_file_name);
	  if (!error_msg) status = MK_OUT_OF_MEMORY;
	  goto FAIL;
	}

#ifdef GENERATE_MESSAGE_ID
  if (state->message_id == NULL) {
	state->message_id = msg_generate_message_id ();
  }
#endif /* GENERATE_MESSAGE_ID */


  /* Write out the message headers.
   */
  headers = mime_generate_headers (state->from, state->reply_to, state->to,
								   state->cc, state->bcc, state->fcc,
								   state->newsgroups, state->followup_to,
								   state->subject, state->references,
								   state->organization,
								   state->message_id,
								   state->other_random_headers,
								   state->context->win_csid);
  if (!headers)
	{
	  status = MK_OUT_OF_MEMORY;
	  goto FAIL;
	}
  if (XP_FileWrite(headers, XP_STRLEN (headers),
				   state->msg_file) < XP_STRLEN(headers))
	{
	  status = MK_MIME_ERROR_WRITING_FILE;
	  goto FAIL;
	}


#ifdef HAVE_SECURITY /* added by jwz */
  if (state->encrypt_p || state->sign_p)
	{
	  char *recipients = (char *)
		XP_ALLOC((state->to  ? XP_STRLEN(state->to)  : 0) +
				 (state->cc  ? XP_STRLEN(state->cc)  : 0) +
				 (state->bcc ? XP_STRLEN(state->bcc) : 0) +
				 (state->newsgroups ? XP_STRLEN(state->newsgroups) : 0) +
				 20);
	  if (!recipients)
		{
		  status = MK_OUT_OF_MEMORY;
		  goto FAIL;
		}
	  *recipients = 0;
# define FROB(X) \
	  if (state->X && *state->X) \
		{ \
		  if (*recipients) XP_STRCAT(recipients, ","); \
		  XP_STRCAT(recipients, state->X); \
		}
	  FROB(to)
	  FROB(cc)
	  FROB(bcc)
	  FROB(newsgroups)
# undef FROB
	  status = mime_begin_crypto_encapsulation (state->context,
												state->msg_file,
												&state->crypto_closure,
												state->encrypt_p,
												state->sign_p,
												recipients,
												FALSE);
	  XP_FREE(recipients);
	  if (status < 0) goto FAIL;
	}
#endif /* NO_SSL -- added by jwz */


  multipart_p = (state->attachment_count > 1 ||
				 (state->attachment_count == 1 &&
				  state->attachment1_body_length > 0));

  if (multipart_p)
	{
	  /* It's a multipart.  Write the content-type and first separator. */

	  const char *multipart_blurb = (state->crypto_closure
									 ? 0
									 : XP_GetString (MK_MIME_MULTIPART_BLURB));

	  separator = mime_make_separator("");
	  if (!separator)
		{
		  status = MK_OUT_OF_MEMORY;
		  goto FAIL;
		}

	  PUSH_STRING ("Content-Type: " MULTIPART_MIXED "; boundary=\"");
	  PUSH_STRING (separator);
	  PUSH_STRING ("\"" CRLF);
	  PUSH_NEWLINE ();				/* close header block */

	  if (multipart_blurb)
		{
		  PUSH_STRING (multipart_blurb);
		  PUSH_STRING (CRLF CRLF);
		}

	  PUSH_STRING ("--");
	  PUSH_STRING (separator);
	  PUSH_NEWLINE ();
	  status = mime_write_message_body (state, buffer, buffer_tail - buffer);
	  buffer_tail = buffer;
	  if (status < 0) goto FAIL;
	}

  /* Write out the first part (user-typed.)
   */
  if (state->attachment1_body_length > 0)
	{
	  unsigned char * intlbuff;
	  char *hdrs = 0;
	  
	  if ((mail_csid & STATEFUL) || /* CS_JIS or CS_2022_KR */
			  mime_7bit_data_p (state->attachment1_body,
							state->attachment1_body_length))	
		StrAllocCopy (state->attachment1_encoding, ENCODING_7BIT);
	  else if (mime_use_quoted_printable_p)
		StrAllocCopy (state->attachment1_encoding, ENCODING_QUOTED_PRINTABLE);
	  else
		StrAllocCopy (state->attachment1_encoding, ENCODING_8BIT);


	  /* Set up encoder for the first part (message body.)
	   */
	  XP_ASSERT(!state->attachment1_encoder_data);
	  if (!strcasecomp(state->attachment1_encoding, ENCODING_BASE64))
		{
		  state->attachment1_encoder_data =
			MimeB64EncoderInit(mime_encoder_output_fn, state);
		  if (!state->attachment1_encoder_data)
			{
			  status = MK_OUT_OF_MEMORY;
			  goto FAIL;
			}
		}
	  else if (!strcasecomp(state->attachment1_encoding,
							ENCODING_QUOTED_PRINTABLE))
		{
		  state->attachment1_encoder_data =
			MimeQPEncoderInit(mime_encoder_output_fn, state);
		  if (!state->attachment1_encoder_data)
			{
			  status = MK_OUT_OF_MEMORY;
			  goto FAIL;
			}
		}


	  hdrs = mime_generate_attachment_headers (state->attachment1_type,
											   state->attachment1_encoding,
											   0, 0, 0, 0, 0,
											   state->digest_p,
											   mail_csid);
	  if (!hdrs)
		{
		  status = MK_OUT_OF_MEMORY;
		  goto FAIL;
		}
	  status = mime_write_message_body (state, hdrs, XP_STRLEN(hdrs));
	  if (status < 0) goto FAIL;

	  FREEIF(hdrs);


	  /* for the message body. We need to convert it from the win_csid to the
	       mail csid */
	  intlbuff = INTL_ConvWinToMailCharCode (state->context, 
						(unsigned char *)state->attachment1_body, 
						state->attachment1_body_length);
						
	  if (intlbuff)
		{
		  /* if it is multibyte and return something. We need to free the origional one and
			assign the state->attachment1_body to the new one */
		  if (intlbuff != (unsigned char *)state->attachment1_body)
		  	{
		  	  XP_FREE (state->attachment1_body);
		  	  state->attachment1_body_length = XP_STRLEN((char*)intlbuff);
		  	}
		  state->attachment1_body = (char*)intlbuff;
		}
		
	  status = mime_encode_block (state,
								  state->attachment1_encoder_data,
								  state->attachment1_body,
								  state->attachment1_body_length);
	  if (status < 0)
		goto FAIL;
				
	  XP_FREE (state->attachment1_body);
	  state->attachment1_body = 0;

	  /* Close off encoder for the first part (message body.) */
	  if (state->attachment1_encoder_data)
		{
		  status = MimeEncoderDestroy(state->attachment1_encoder_data, FALSE);
		  state->attachment1_encoder_data = 0;
		  if (status < 0) goto FAIL;
		}


	  /* Write out multipart seperator */
	  if (multipart_p)
		{
		  buffer_tail = buffer;
		  /* one CRLF to terminate last line of first part;
			 a second CRLF to insert a blank line; and then
			 two dashes to start the separator. */
		  PUSH_STRING (CRLF CRLF "--");
		  PUSH_STRING (separator);
		  PUSH_NEWLINE ();
		  status = mime_write_message_body (state, buffer,
											buffer_tail - buffer);
		  buffer_tail = buffer;
		  if (status < 0) goto FAIL;
		}
	}

  /* Write out the subsequent parts.
   */
  if (state->attachment_count > 0)
	{
	  char *buffer;

	  /* Kludge to avoid having to allocate memory on the toy computers... */
	  if (! mime_mailto_stream_read_buffer)
		mime_mailto_stream_read_buffer = (char *) XP_ALLOC (MIME_BUFFER_SIZE);
	  buffer = mime_mailto_stream_read_buffer;
	  if (! buffer) return MK_OUT_OF_MEMORY;
	  buffer_tail = buffer;

	  for (i = 0; i < state->attachment_count; i++)
		{
		  struct mime_attachment *ma = &state->attachments[i];
		  char *hdrs = 0;

		  status = mime_pick_encoding (ma, mail_csid);
		  if (status < 0) goto FAIL;
		  		  
		  hdrs = mime_generate_attachment_headers (ma->type, ma->encoding,
												   ma->description,
												   ma->x_mac_type,
												   ma->x_mac_creator,
												   ma->real_name,
												   ma->url_string,
												   state->digest_p,
												   mail_csid);
		  if (!hdrs)
			{
			  status = MK_OUT_OF_MEMORY;
			  goto FAIL;
			}
		  status = mime_write_message_body (state, hdrs, XP_STRLEN(hdrs));
		  if (status < 0) goto FAIL;

		  FREEIF(hdrs);

		  in_file = XP_FileOpen (ma->file_name, xpFileToPost,
								 XP_FILE_READ_BIN);
		  if (! in_file)
			{
			  status = -1;
			  goto FAIL;
			}

		  ma->current_column = 0;

#ifdef MANGLE_HTML_ATTACHMENTS_WITH_BASE_TAG
		  if (ma->url_string &&
			  ma->type &&
			  !ma->already_encoded_p &&		/* can't add it in this case */
			  (!strcasecomp (ma->type, TEXT_HTML) ||
			   !strcasecomp (ma->type, TEXT_MDL)))
			{
			  char *base = (char *) XP_ALLOC (XP_STRLEN (ma->url_string) + 50);
			  if (!base)
				return MK_OUT_OF_MEMORY;
			  XP_STRCPY (base, "<BASE HREF=\"");
			  XP_STRCAT (base, ma->url_string);
			  XP_STRCAT (base, "\">" CRLF CRLF);
			  status = mime_encode_block (state, ma->encoder_data,
										  base, XP_STRLEN (base));
			  XP_FREE (base);
			}
#endif /* MANGLE_HTML_ATTACHMENTS_WITH_BASE_TAG */

		  if (ma->type &&
			  (!strcasecomp (ma->type, MESSAGE_RFC822) ||
			   !strcasecomp (ma->type, MESSAGE_NEWS)))
			{
			  /* We are attaching a message, so we should be careful to
				 strip out certain sensitive internal header fields.
			   */
			  XP_Bool in_headers = TRUE;
			  XP_Bool skipping = FALSE;
			  XP_ASSERT(MIME_BUFFER_SIZE > 1000); /* SMTP (RFC821) limit */

			  while (1)
				{
				  char *line = XP_FileReadLine(buffer, MIME_BUFFER_SIZE-3,
											   in_file);
				  if (!line) break;  /* EOF */

				  if (skipping)
					{
					  if (*line == ' ' || *line == '\t')
						continue;
					  else
						skipping = FALSE;
					}

				  if (in_headers &&
					  (!strncasecomp(line, "BCC:", 4) ||
					   !strncasecomp(line, "FCC:", 4) ||
					   !strncasecomp(line, CONTENT_LENGTH ":",
									 CONTENT_LENGTH_LEN + 1) ||
					   !strncasecomp(line, "Lines:", 6) ||
					   !strncasecomp(line, "Status:", 7) ||
					   !strncasecomp(line, X_MOZILLA_STATUS ":",
									 X_MOZILLA_STATUS_LEN+1) ||
					   !strncasecomp(line, X_MOZILLA_NEWSHOST ":",
									 X_MOZILLA_NEWSHOST_LEN+1) ||
					   !strncasecomp(line, X_UIDL ":", X_UIDL_LEN+1) ||
					   !strncasecomp(line, "X-VM-", 5))) /* hi Kyle */
					{
					  skipping = TRUE;
					  continue;
					}

				  status = mime_encode_block (state, ma->encoder_data,
											  line, XP_STRLEN(line));
				  if (status < 0) return status;

				  if (in_headers && (*line == CR || *line == LF))
					in_headers = FALSE;
				}
			}
		  else
			{
			  /* Normal attachment.  Do big reads.
			   */
			  XP_Bool firstBlock = TRUE;
			  /* only convert if we need to tag charset */
			  XP_Bool needIntlConversion = mime_type_needs_charset(ma->type);
			  CCCDataObject	*intlDocToMailConverter = NULL;
			  char* encoded_data = NULL;
			  uint32 encoded_data_len = 0;
		  	  while (1)
				{
			  	  status = XP_FileRead (buffer, MIME_BUFFER_SIZE, in_file);
			  	  if (status < 0)
			    	{
				  	  if(intlDocToMailConverter)
					 	  INTL_DestroyCharCodeConverter(intlDocToMailConverter);
				  	  goto FAIL;
					}
			  	  else if (status == 0)
					  break;

			  	  encoded_data = buffer;
			  	  encoded_data_len = status;
			  
			  	  /* if this is the first block, create the conversion object */
			  	  if(firstBlock)
					{
				  	  if(needIntlConversion)
						{
					  	  intlDocToMailConverter = 
						  	  INTL_CreateDocToMailConverter(
							  		  state->context, 
									  (!strcasecomp (ma->type, TEXT_HTML)) , 
									  (unsigned char*) buffer, 
									  status);
						}
				  	  firstBlock = FALSE;		/* No longer the first block */
					}
				
			  	  if(intlDocToMailConverter)
					{
				  	  encoded_data = (char*)INTL_CallCharCodeConverter(
												intlDocToMailConverter, 
											  	(unsigned char*)buffer,
											  	status);
				  		/* the return buffer is different from the */
						/* origional one. The size need to be change */
				  	  if(encoded_data && encoded_data != buffer)
						  encoded_data_len = XP_STRLEN(encoded_data);
					}
				
			  	  status = mime_encode_block (state,
										  	  (ma->already_encoded_p
										   	  /* If the file already has an
											 	   encoding, leave it alone. */
										 	    ? 0
										 	    : ma->encoder_data),
										  	  (encoded_data 
										 	    ? encoded_data 
										 	    : buffer), 
										  	  encoded_data_len);
										  
			  	  if(encoded_data && encoded_data != buffer)
				 	  XP_FREE(encoded_data);
					
			  	  if (status < 0)
					{ 
				  	  if(intlDocToMailConverter)
						  INTL_DestroyCharCodeConverter(intlDocToMailConverter);
				  	  goto FAIL;
					}
				}
		  	  if(intlDocToMailConverter)
					INTL_DestroyCharCodeConverter(intlDocToMailConverter);
			}
			
		  /* Shut down the encoder (and flush out any buffered data.)
		   */
		  if (ma->encoder_data)
			{
			  status = MimeEncoderDestroy(ma->encoder_data, FALSE);
			  ma->encoder_data = 0;
			  if (status < 0) goto FAIL;
			}

		  XP_FileClose (in_file);
		  in_file = 0;

		  /* At this point, we're done with the temp file for this part,
			 and can safely delete it. */
		  if (!state->pre_snarfed_attachments_p)
			XP_FileRemove (ma->file_name, xpFileToPost);
		  XP_FREE (ma->file_name);
		  ma->file_name = 0;

		  /* Write out multipart seperator */
		  if (multipart_p)
			{
			  buffer_tail = buffer;
			  PUSH_STRING (CRLF "--");
			  PUSH_STRING (separator);
			  if (i == state->attachment_count - 1)  /* final part */
				PUSH_STRING ("--");
			  PUSH_NEWLINE ();
			  status = mime_write_message_body (state, buffer,
												buffer_tail - buffer);
			  buffer_tail = buffer;
			  if (status < 0) goto FAIL;
			}
		}
	}


#ifdef HAVE_SECURITY /* added by jwz */
  /* Close down encryption stream */
  if (state && state->crypto_closure)
	{
	  status = mime_finish_crypto_encapsulation (state->crypto_closure, FALSE);
	  state->crypto_closure = 0;
	  if (status < 0) goto FAIL;
	}
#endif /* !HAVE_SECURITY -- added by jwz */

  if (state->msg_file)
	XP_FileClose (state->msg_file);
  state->msg_file = 0;

#if 0
  /* Find the size of the final, fully-assembled message. */
  {
	XP_StatStruct st;
	st.st_size = 0;
	XP_Stat (state->msg_file_name, &st, xpFileToPost);
	state->msg_size = st.st_size;
  }
#endif

  FE_Progress(state->context, XP_GetString(MK_MSG_ASSEMB_DONE_MSG));

  if (state->dont_deliver_p &&
	  state->message_delivery_done_callback)
	{
	  state->message_delivery_done_callback (state->context,
											 state->fe_data, 0,
											 XP_STRDUP (state->msg_file_name));
	  /* Need to ditch the file name here so that we don't delete the
		 file, since in this case, the FE needs the file itself. */
	  FREEIF (state->msg_file_name);
	  state->msg_file_name = 0;
	  state->message_delivery_done_callback = 0;
	  mime_free_message_state (state);
	  state = 0;
	}
  else
	{
	  mime_deliver_message (state);
	}

 FAIL:
  if (headers) XP_FREE (headers);
  if (separator) XP_FREE (separator);
  if (in_file)
	XP_FileClose (in_file);

#ifdef HAVE_SECURITY /* added by jwz */
  /* Close down encryption stream */
  if (state && state->crypto_closure)
	{
	  mime_finish_crypto_encapsulation (state->crypto_closure, TRUE);
	  state->crypto_closure = 0;
	}
#endif /* !HAVE_SECURITY -- added by jwz */

  if (state && status < 0)
	{
	  state->status = status;
	  mime_fail (state, status, error_msg);
	}
  /* If status is >= 0, then the the next event coming up is posting to
	 a "mailto:" or "news:" URL; the message_delivery_done_callback will
	 be called from the exit routine of that URL. */

  return status;
}


static int
mime_write_message_body (struct mime_delivery_state *state,
						 char *buf, int32 size)
{
#ifdef HAVE_SECURITY /* added by jwz */
  if (state->crypto_closure)
	{
	  return mime_crypto_write_block (state->crypto_closure, buf, size);
	}
  else
#endif /* !HAVE_SECURITY -- added by jwz */
	{
	  if (XP_FileWrite (buf, size, state->msg_file) < size)
		return MK_MIME_ERROR_WRITING_FILE;
	  else
		return 0;
	}
}


static int
mime_encoder_output_fn (const char *buf, int32 size, void *closure)
{
  struct mime_delivery_state *state = (struct mime_delivery_state *) closure;
  return mime_write_message_body (state, (char *) buf, size);
}


char *
msg_generate_message_id (void)
{
  time_t now = XP_TIME();
  uint32 salt = 0;
  const char *host = 0;
  const char *from = FE_UsersMailAddress ();

#ifdef HAVE_SECURITY /* added by jwz */
  RNG_GenerateGlobalRandomBytes((void *) &salt, sizeof(salt));
#else
  salt = random();
#endif /* !HAVE_SECURITY -- added by jwz */

  if (from && (host = XP_STRCHR (from, '@')))
	{
	  const char *s;
	  for (s = ++host; *s; s++)
		if (!XP_IS_ALPHA(*s) && !XP_IS_DIGIT(*s) &&
			*s != '-' && *s != '_' && *s != '.')
		  {
			host = 0;
			break;
		  }
	}

  if (! host)
	/* If we couldn't find a valid host name to use, we can't generate a
	   valid message ID, so bail, and let NNTP and SMTP generate them. */
	return 0;

  return PR_smprintf("<%lX.%lX@%s>",
					 (unsigned long) now, (unsigned long) salt, host);
}


static char *
mime_generate_headers (const char *from, const char *reply_to,
					   const char *to, const char *cc, const char *bcc,
					   const char *fcc, const char *newsgroups,
					   const char *followup_to, const char *subject,
					   const char *references,
					   const char *organization,
					   const char* message_id,
					   const char *other_random_headers,
					   int csid)
{
  int size = 0;
  char *buffer = 0, *buffer_tail = 0;

  /* Multiply by 3 here to make enough room for MimePartII conversion */
  if (from)                 size += 3 * XP_STRLEN (from);
  if (reply_to)             size += 3 * XP_STRLEN (reply_to);
  if (to)                   size += 3 * XP_STRLEN (to);
  if (cc)                   size += 3 * XP_STRLEN (cc);
  if (newsgroups)           size += 3 * XP_STRLEN (newsgroups);
  if (followup_to)          size += 3 * XP_STRLEN (followup_to);
  if (subject)              size += 3 * XP_STRLEN (subject);
  if (references)           size += 3 * XP_STRLEN (references);
  if (organization)         size += 3 * XP_STRLEN (organization);
  if (other_random_headers) size += 3 * XP_STRLEN (other_random_headers);
#ifdef GENERATE_MESSAGE_ID
  if (message_id)           size += XP_STRLEN (message_id);
#endif /* GENERATE_MESSAGE_ID */

  /* Add a bunch of slop for the static parts of the headers. */
  size += 2048;

  buffer = (char *) XP_ALLOC (size);
  if (!buffer)
	return 0; /* MK_OUT_OF_MEMORY */

  buffer_tail = buffer;

#ifdef GENERATE_MESSAGE_ID
  if (message_id)
	{
	  PUSH_STRING ("Message-ID: ");
	  PUSH_STRING (message_id);
	  PUSH_NEWLINE ();
	}
#endif /* GENERATE_MESSAGE_ID */

  {
	int gmtoffset = XP_LocalZoneOffset();
    PRTime now;
    PR_ExplodeTime( &now, PR_Now() );
    
  /* Use PR_FormatTimeUSEnglish() to format the date in US English format, then figure
     out what our local GMT offset is, and append it (since PR_FormatTimeUSEnglish()
     can't do that.) Generate four digit years as per RFC 1123 (superceding RFC 822.)
   */
	PR_FormatTimeUSEnglish( buffer_tail, 100, "Date: %a, %d %b %Y %H:%M:%S ", &now );
	buffer_tail += XP_STRLEN (buffer_tail);
	PR_snprintf(buffer_tail, buffer + size - buffer_tail,
				"%c%02d%02d" CRLF,
				(gmtoffset >= 0 ? '+' : '-'),
				((gmtoffset >= 0 ? gmtoffset : -gmtoffset) / 60),
				((gmtoffset >= 0 ? gmtoffset : -gmtoffset) % 60));
	buffer_tail += XP_STRLEN (buffer_tail);
  }

  if (from)
	{
	  char *convbuf;
  	  PUSH_STRING ("From: ");
	  convbuf = IntlEncodeMimePartIIStr((char *)from, csid,
										mime_use_quoted_printable_p);
	  if (convbuf)     /* MIME-PartII conversion */
	    {
	  	  PUSH_STRING (convbuf);
	  	  XP_FREE(convbuf);
	    }
	  else
	    PUSH_STRING (from);
      PUSH_NEWLINE ();
	}

  if (reply_to)
	{
	  char *convbuf;
	  PUSH_STRING ("Reply-To: ");
	  convbuf = IntlEncodeMimePartIIStr((char *)reply_to, csid,
										mime_use_quoted_printable_p);
	  if (convbuf)     /* MIME-PartII conversion */
	    {
	  	  PUSH_STRING (convbuf);
	  	  XP_FREE(convbuf);
	    }
	  else
	    PUSH_STRING (reply_to);
	  PUSH_NEWLINE ();
	}

  if (organization)
	{
	  char *convbuf;
	  PUSH_STRING ("Organization: ");
	  convbuf = IntlEncodeMimePartIIStr((char *)organization, csid,
										mime_use_quoted_printable_p);
	  if (convbuf)     /* MIME-PartII conversion */
	    {
	  	  PUSH_STRING (convbuf);
	  	  XP_FREE(convbuf);
	    }
	  else
	    PUSH_STRING (organization);
	  PUSH_NEWLINE ();
	}

  PUSH_STRING ("X-Mailer: ");
  PUSH_STRING (XP_AppCodeName);
  PUSH_STRING (" ");
  PUSH_STRING (XP_AppVersion);
  PUSH_NEWLINE ();

  PUSH_STRING ("MIME-Version: 1.0" CRLF);

  if (newsgroups)
	{
      /* turn whitespace into a comma list
       */
	  char *ptr, *ptr2;
	  char *n2;
	  char *convbuf;
	  
	  convbuf = IntlEncodeMimePartIIStr((char *)newsgroups, csid,
										mime_use_quoted_printable_p);
	  if (convbuf)
	  	n2 = XP_StripLine (convbuf);
	  else {
		ptr = XP_STRDUP(newsgroups);
		if (!ptr)
		  {
			FREEIF(buffer);
			return 0; /* MK_OUT_OF_MEMORY */
		  }
  	  	n2 = XP_StripLine(ptr);
		XP_ASSERT(n2 == ptr);	/* Otherwise, the XP_FREE below is
								   gonna choke badly. */
	  }

      for(ptr=n2; *ptr != '\0'; ptr++)
        {
          /* find first non white space */
          while(!XP_IS_SPACE(*ptr) && *ptr != ',' && *ptr != '\0')
              ptr++;

          if(*ptr == '\0')
              break;

          if(*ptr != ',')
              *ptr = ',';

          /* find next non white space */
          ptr2 = ptr+1;
          while(XP_IS_SPACE(*ptr2))
              ptr2++;

          if(ptr2 != ptr+1)
              XP_STRCPY(ptr+1, ptr2);
        }

	  PUSH_STRING ("Newsgroups: ");
	  PUSH_STRING (n2);
	  XP_FREE (n2);
	  PUSH_NEWLINE ();
	}

  /* #### shamelessly duplicated from above */
  if (followup_to)
	{
      /* turn whitespace into a comma list
       */
	  char *ptr, *ptr2;
	  char *n2;
	  char *convbuf;
	  
	  convbuf = IntlEncodeMimePartIIStr((char *)followup_to, csid,
										mime_use_quoted_printable_p);
	  if (convbuf)
	  	n2 = XP_StripLine (convbuf);
	  else {
		ptr = XP_STRDUP(followup_to);
		if (!ptr)
		  {
			FREEIF(buffer);
			return 0; /* MK_OUT_OF_MEMORY */
		  }
  	  	n2 = XP_StripLine (ptr);
		XP_ASSERT(n2 == ptr);	/* Otherwise, the XP_FREE below is
								   gonna choke badly. */
	  }

      for(ptr=n2; *ptr != '\0'; ptr++)
        {
          /* find first non white space */
          while(!XP_IS_SPACE(*ptr) && *ptr != ',' && *ptr != '\0')
              ptr++;

          if(*ptr == '\0')
              break;

          if(*ptr != ',')
              *ptr = ',';

          /* find next non white space */
          ptr2 = ptr+1;
          while(XP_IS_SPACE(*ptr2))
              ptr2++;

          if(ptr2 != ptr+1)
              XP_STRCPY(ptr+1, ptr2);
        }

	  PUSH_STRING ("Followup-To: ");
	  PUSH_STRING (n2);
	  XP_FREE (n2);
	  PUSH_NEWLINE ();
	}

  if (to)
	{
	  char *convbuf;
	  PUSH_STRING ("To: ");
	  convbuf = IntlEncodeMimePartIIStr((char *)to, csid,
										mime_use_quoted_printable_p);
	  if (convbuf)     /* MIME-PartII conversion */
	    {
	  	  PUSH_STRING (convbuf);
	  	  XP_FREE(convbuf);
	    }
	  else
	    PUSH_STRING (to);

	  PUSH_NEWLINE ();
	}
  if (cc)
	{
	  char *convbuf;
	  PUSH_STRING ("CC: ");
	  convbuf = IntlEncodeMimePartIIStr((char *)cc, csid,
										mime_use_quoted_printable_p);
	  if (convbuf)   /* MIME-PartII conversion */
	    {
	  	  PUSH_STRING (convbuf);
	  	  XP_FREE(convbuf);
	    }
	  else
	    PUSH_STRING (cc);
	  PUSH_NEWLINE ();
	}
  if (subject)
	{
	  char *convbuf;
	  PUSH_STRING ("Subject: ");
	  convbuf = IntlEncodeMimePartIIStr((char *)subject, csid,
										mime_use_quoted_printable_p);
	  if (convbuf)  /* MIME-PartII conversion */
	    {
	  	  PUSH_STRING (convbuf);
	  	  XP_FREE(convbuf);
	    }
	  else
	    PUSH_STRING (subject);
	  PUSH_NEWLINE ();
	}
  if (references)
	{
	  PUSH_STRING ("References: ");
	  PUSH_STRING (references);
	  PUSH_NEWLINE ();
	}

  if (other_random_headers)
	{
	  /* Assume they already have the right newlines and continuations
		 and so on. */
	  PUSH_STRING (other_random_headers);
	}

  if (buffer_tail > buffer + size - 1)
	abort ();

  /* realloc it smaller... */
  buffer = (char*) XP_REALLOC (buffer, buffer_tail - buffer + 1);

  return buffer;
}


/* Generate headers for a form post to a mailto: URL.
   This lets the URL specify additional headers, but is careful to
   ignore headers which would be dangerous.  It may modify the URL
   (because of CC) so a new URL to actually post to is returned.
 */
int
MIME_GenerateMailtoFormPostHeaders (const char *old_post_url,
									const char *referer,
									char **new_post_url_return,
									char **headers_return)
{
  char *from = 0, *to = 0, *cc = 0, *body = 0, *search = 0;
  char *extra_headers = 0;
  char *s;
  XP_Bool subject_p = FALSE;
  XP_Bool sign_p = FALSE, encrypt_p = FALSE;
  char *rest;
  int status = 0;
  static const char *forbidden_headers[] = {
	"Apparently-To",
	"BCC",
	"Content-Encoding",
	CONTENT_LENGTH,
	"Content-Transfer-Encoding",
	"Content-Type",
	"Date",
	"Distribution",
	"FCC",
	"Followup-To",
	"From",
	"Lines",
	"MIME-Version",
	"Message-ID",
	"Newsgroups",
	"Organization",
	"Reply-To",
	"Sender",
	X_MOZILLA_STATUS,
	X_MOZILLA_NEWSHOST,
	X_UIDL,
	"XRef",
	0 };

  from = MIME_MakeFromField ();
  if (!from) {
	status = MK_OUT_OF_MEMORY;
	goto FAIL;
  }

  to = NET_ParseURL (old_post_url, GET_PATH_PART);
  if (!to) {
	status = MK_OUT_OF_MEMORY;
	goto FAIL;
  }

  if (!*to)
	{
	  status = -1;
	  goto FAIL;
	}

  search = NET_ParseURL (old_post_url, GET_SEARCH_PART);

  rest = search;
  if (rest && *rest == '?')
	{
	  /* start past the '?' */
	  rest++;
	  rest = XP_STRTOK (rest, "&");
	  while (rest && *rest)
		{
		  char *token = rest;
		  char *value = 0;
		  char *eq;

		  rest = XP_STRTOK (0, "&");

		  eq = XP_STRCHR (token, '=');
		  if (eq)
			{
			  value = eq+1;
			  *eq = 0;
			}

		  if (!strcasecomp (token, "subject"))
			subject_p = TRUE;

		  if (value)
			/* Don't allow newlines or control characters in the value. */
			for (s = value; *s; s++)
			  if (*s < ' ' && *s != '\t')
				*s = ' ';

		  if (!strcasecomp (token, "to"))
			{
			  if (to && *to)
				{
				  StrAllocCat (to, ", ");
				  StrAllocCat (to, value);
				}
			  else
				{
				  StrAllocCopy (to, value);
				}
			}
		  else if (!strcasecomp (token, "cc"))
			{
			  if (cc && *cc)
				{
				  StrAllocCat (cc, ", ");
				  StrAllocCat (cc, value);
				}
			  else
				{
				  StrAllocCopy (cc, value);
				}
			}
		  else if (!strcasecomp (token, "body"))
			{
			  if (body && *body)
				{
				  StrAllocCat (body, "\n");
				  StrAllocCat (body, value);
				}
			  else
				{
				  StrAllocCopy (body, value);
				}
			}
		  else if (!strcasecomp (token, "encrypt") ||
				   !strcasecomp (token, "encrypted"))
			{
			  encrypt_p = (!strcasecomp(value, "true") ||
						   !strcasecomp(value, "yes"));
			}
		  else if (!strcasecomp (token, "sign") ||
				   !strcasecomp (token, "signed"))
			{
			  sign_p = (!strcasecomp(value, "true") ||
						!strcasecomp(value, "yes"));
			}
		  else
			{
			  const char **fh = forbidden_headers;
			  XP_Bool ok = TRUE;
			  while (*fh)
				if (!strcasecomp (token, *fh++))
				  {
					ok = FALSE;
					break;
				  }
			  if (ok)
				{
				  XP_Bool upper_p = FALSE;
				  char *s;
				  for (s = token; *s; s++)
					{
					  if (*s >= 'A' && *s <= 'Z')
						upper_p = TRUE;
					  else if (*s <= ' ' || *s >= '~' || *s == ':')
						goto NOT_OK;  /* bad character in header! */
					}
				  if (!upper_p && *token >= 'a' && *token <= 'z')
					*token -= ('a' - 'A');

				  StrAllocCat (extra_headers, token);
				  StrAllocCat (extra_headers, ": ");
				  if (value)
					StrAllocCat (extra_headers, value);
				  StrAllocCat (extra_headers, CRLF);
				NOT_OK: ;
				}
			}
		}
	}

  if (!subject_p)
	{
	  /* If the URL didn't provide a subject, we will. */
	  StrAllocCat (extra_headers, "Subject: Form posted from ");
	  XP_ASSERT (XP_AppCodeName);
	  StrAllocCat (extra_headers, XP_AppCodeName);
	  StrAllocCat (extra_headers, CRLF);
	}

  /* Note: the `encrypt', `sign', and `body' parameters are currently
	 ignored in mailto form submissions. */

  *new_post_url_return = 0;
  *headers_return = mime_generate_headers (from, 0, to, cc, 0, 0, 0, 0, 0, 0,
										   FE_UsersOrganization(), 0,
										   extra_headers, 0);
  if (*headers_return == 0)
	{
	  status = MK_OUT_OF_MEMORY;
	  goto FAIL;
	}

  StrAllocCat ((*new_post_url_return), "mailto:");
  if (to)
	StrAllocCat ((*new_post_url_return), to);
  if (to && cc)
	StrAllocCat ((*new_post_url_return), ",");
  if (cc)
	StrAllocCat ((*new_post_url_return), cc);

 FAIL:
  FREEIF (from);
  FREEIF (to);
  FREEIF (cc);
  FREEIF (body);
  FREEIF (search);
  FREEIF (extra_headers);
  return status;
}



static char *
mime_generate_attachment_headers (const char *type, const char *encoding,
								  const char *description,
								  const char *x_mac_type,
								  const char *x_mac_creator,
								  const char *real_name,
								  const char *base_url,
								  XP_Bool digest_p,
								  int16 mail_csid)
{
  char *buffer = (char *) XP_ALLOC (1024);
  char *buffer_tail = buffer;

  if (! buffer)
	return 0; /* MK_OUT_OF_MEMORY */

  XP_ASSERT (encoding);

  if (digest_p && type &&
	  (!strcasecomp (type, MESSAGE_RFC822) ||
	   !strcasecomp (type, MESSAGE_NEWS)))
	{
	  /* If we're in a multipart/digest, and this document is of type
		 message/rfc822, then it's appropriate to emit no headers.
	   */
	  PUSH_NEWLINE ();
	}
  else
	{
	  PUSH_STRING ("Content-Type: ");
	  PUSH_STRING (type);

	  if (mime_type_needs_charset (type))
		{
		  char charset[30];

		  /* push 7bit encoding out based on current default codeset */	
		  INTL_CharSetIDToName (mail_csid, charset);

		  /* If the characters are all 7bit, then it's better (and true) to
			 claim the charset to be US-ASCII rather than Latin1.  Should we
			 do this all the time, for all charsets?  I'm not sure.  But we
			 should definitely do it for Latin1. */
		  if (encoding &&
			  !strcasecomp (encoding, "7bit") &&
			  !strcasecomp (charset, "iso-8859-1"))
			XP_STRCPY (charset, "us-ascii");

		  PUSH_STRING ("; charset=");
		  PUSH_STRING (charset);
		}

	  if (x_mac_type && *x_mac_type)
		{
		  PUSH_STRING ("; x-mac-type=\"");
		  PUSH_STRING (x_mac_type);
		  PUSH_STRING ("\"");
		}
	  if (x_mac_creator && *x_mac_creator)
		{
		  PUSH_STRING ("; x-mac-creator=\"");
		  PUSH_STRING (x_mac_creator);
		  PUSH_STRING ("\"");
		}

#ifdef EMIT_NAME_IN_CONTENT_TYPE
	  if (real_name && *real_name)
		{
		  char *s;
		  PUSH_STRING ("; name=\"");
		  s = buffer_tail;
		  PUSH_STRING (real_name);
		  while (*s)
			{
			  /* This isn't quite right, but who cares.  If the file name
				 contains characters which would break the string quoting,
				 map them to SPC. */
			  if (*s == '\\' || *s == '"' || *s == CR || *s == LF)
				*s = ' ';
			  s++;
			}
		  PUSH_STRING ("\"");
		}
#endif /* EMIT_NAME_IN_CONTENT_TYPE */

	  PUSH_NEWLINE ();

	  PUSH_STRING ("Content-Transfer-Encoding: ");
	  PUSH_STRING (encoding);
	  PUSH_NEWLINE ();

	  if (description && *description)
		{
		  char *s = mime_fix_header (description);
		  if (s)
			{
			  PUSH_STRING ("Content-Description: ");
			  PUSH_STRING (s);
			  PUSH_NEWLINE ();
			  XP_FREE(s);
			}
		}

	  if (real_name && *real_name)
		{
		  char *s;
		  PUSH_STRING ("Content-Disposition: ");

		  /* If this document is an anonymous binary file, then always show it
			 as an attachment, never inline. */
		  if (!strcasecomp(type, APPLICATION_OCTET_STREAM))
			PUSH_STRING ("attachment");
		  else
			PUSH_STRING ("inline");

		  PUSH_STRING ("; filename=\"");
		  s = buffer_tail;
		  PUSH_STRING (real_name);
		  while (*s)
			{
			  /* This isn't quite right, but who cares.  If the file name
				 contains characters which would break the string quoting,
				 map them to SPC. */
			  if (*s == '\\' || *s == '"' || *s == CR || *s == LF)
				*s = ' ';
			  s++;
			}
		  PUSH_STRING ("\"" CRLF);
		}
	  else if (type &&
			   (!strcasecomp (type, MESSAGE_RFC822) ||
				!strcasecomp (type, MESSAGE_NEWS)))
		{
		  XP_ASSERT (!digest_p);
		  PUSH_STRING ("Content-Disposition: inline" CRLF);
		}

#ifdef GENERATE_CONTENT_BASE
	  /* If this is an HTML document, and we know the URL it originally
		 came from, write out a Content-Base header. */
	  if (type &&
		  (!strcasecomp (type, TEXT_HTML) ||
		   !strcasecomp (type, TEXT_MDL)) &&
		  base_url && *base_url)
		{
		  int32 col = 0;
		  const char *s = base_url;
		  const char *colon = XP_STRCHR (s, ':');

		  if (!colon) goto GIVE_UP_ON_CONTENT_BASE;  /* malformed URL? */

		  /* Don't emit a content-base that points to (or into) a news or
			 mail message. */
		  if (!strncasecomp (s, "news:", 5) ||
			  !strncasecomp (s, "snews:", 6) ||
			  !strncasecomp (s, "mailbox:", 8))
			goto GIVE_UP_ON_CONTENT_BASE;

		  PUSH_STRING ("Content-Base: \"");
		  while (*s != 0 && *s != '#')
			{
			  const char *ot = buffer_tail;

			  /* URLs must be wrapped at 40 characters or less. */
			  if (col >= 38)
				{
				  PUSH_STRING(CRLF "\t");
				  col = 0;
				}

			  if (*s == ' ')       PUSH_STRING("%20");
			  else if (*s == '\t') PUSH_STRING("%09");
			  else if (*s == '\n') PUSH_STRING("%0A");
			  else if (*s == '\r') PUSH_STRING("%0D");
			  else
				{
				  *buffer_tail++ = *s;
				  *buffer_tail = '\0';
				}
			  s++;
			  col += (buffer_tail - ot);
			}
		  PUSH_STRING ("\"" CRLF);

		GIVE_UP_ON_CONTENT_BASE:
		  ;
		}
#endif /* GENERATE_CONTENT_BASE */


	  PUSH_NEWLINE ();	/* blank line for end of headers */
	}

  /* realloc it smaller... */
  buffer = (char*) XP_REALLOC (buffer, buffer_tail - buffer + 1);

  return buffer;
}


static void
mime_fail (struct mime_delivery_state *state, int failure_code,
		   char *error_msg)
{
  if ( ! state ) return;

  if (state->message_delivery_done_callback)
	{
	  if (failure_code < 0 && !error_msg)
		error_msg = NET_ExplainErrorDetails(failure_code, 0, 0, 0, 0);
	  state->message_delivery_done_callback (state->context, state->fe_data,
											 failure_code, error_msg);
	  FREEIF(error_msg);		/* ### Is there a memory leak here?  Shouldn't
								   this free be outside the if? */
	}
 else if (state->attachments_done_callback)
   {
	  if (failure_code < 0 && !error_msg)
		error_msg = NET_ExplainErrorDetails(failure_code, 0, 0, 0, 0);
			
	/* mime_free_message_state will take care of cleaning up the 
	   attachment files and attachment structures */
	  state->attachments_done_callback (state->context,
										state->fe_data, failure_code,
										error_msg, 0);
											 
	  FREEIF(error_msg);		/* ### Is there a memory leak here?  Shouldn't
								   this free be outside the if? */   
   }
 
  state->message_delivery_done_callback = 0;
  state->attachments_done_callback = 0;
  
  mime_free_message_state (state);
}


/* Rip apart the URL and extract a reasonable value for the `real_name' slot.
 */
static void
msg_pick_real_name (struct mime_attachment *attachment)
{
  const char *s, *s2;
  char *s3;
  char *url;

  if (attachment->real_name)
	return;

  url = attachment->url_string;

  /* Perhaps the MIME parser knows a better name than the URL itself?
	 This can happen when one attaches a MIME part from one message
	 directly into another message. */
  attachment->real_name = MimeGuessURLContentName(attachment->state->context,
												  url);
  if (attachment->real_name)
	return;

  /* Otherwise, extract a name from the URL. */

  s = url;
  s2 = XP_STRCHR (s, ':');
  if (s2) s = s2 + 1;
  /* If we know the URL doesn't have a sensible file name in it,
	 don't bother emitting a content-disposition. */
  if (!strncasecomp (url, "news:", 5) ||
	  !strncasecomp (url, "snews:", 6) ||
	  !strncasecomp (url, "mailbox:", 8))
	return;

  /* Take the part of the file name after the last / or \ */
  s2 = XP_STRRCHR (s, '/');
  if (s2) s = s2+1;
  s2 = XP_STRRCHR (s, '\\');
  if (s2) s = s2+1;
  /* Copy it into the attachment struct. */
  StrAllocCopy (attachment->real_name, s);
  /* Now trim off any named anchors or search data. */
  s3 = XP_STRCHR (attachment->real_name, '?');
  if (s3) *s3 = 0;
  s3 = XP_STRCHR (attachment->real_name, '#');
  if (s3) *s3 = 0;

  /* Now lose the %XX crap. */
  NET_UnEscape (attachment->real_name);


  /* Now a special case for attaching uuencoded files...

	 If we attach a file "foo.txt.uu", we will send it out with
	 Content-Type: text/plain; Content-Transfer-Encoding: x-uuencode.
	 When saving such a file, a mail reader will generally decode it first
	 (thus removing the uuencoding.)  So, let's make life a little easier by
	 removing the indication of uuencoding from the file name itself.  (This
	 will presumably make the file name in the Content-Disposition header be
	 the same as the file name in the "begin" line of the uuencoded data.)

	 However, since there are mailers out there (including earlier versions of
	 Mozilla) that will use "foo.txt.uu" as the file name, we still need to
	 cope with that; the code which copes with that is in the MIME parser, in
	 libmime/mimei.c.
   */
  if (attachment->already_encoded_p &&
	  attachment->encoding)
	{
	  char *result = attachment->real_name;
	  int32 L = XP_STRLEN(result);
	  const char **exts = 0;

	  /* #### TOTAL FUCKING KLUDGE.
		 I'd like to ask the mime.types file, "what extensions correspond
		 to obj->encoding (which happens to be "x-uuencode") but doing that
		 in a non-sphagetti way would require brain surgery.  So, since
		 currently uuencode is the only content-transfer-encoding which we
		 understand which traditionally has an extension, we just special-
		 case it here!  Icepicks in my forehead!

		 Note that it's special-cased in a similar way in libmime/mimei.c.
	   */
	  if (!strcasecomp(attachment->encoding, ENCODING_UUENCODE) ||
		  !strcasecomp(attachment->encoding, ENCODING_UUENCODE2) ||
		  !strcasecomp(attachment->encoding, ENCODING_UUENCODE3) ||
		  !strcasecomp(attachment->encoding, ENCODING_UUENCODE4))
		{
		  static const char *uue_exts[] = { "uu", "uue", 0 };
		  exts = uue_exts;
		}

	  while (exts && *exts)
		{
		  const char *ext = *exts;
		  int32 L2 = XP_STRLEN(ext);
		  if (L > L2 + 1 &&							/* long enough */
			  result[L - L2 - 1] == '.' &&			/* '.' in right place*/
			  !strcasecomp(ext, result + (L - L2)))	/* ext matches */
			{
			  result[L - L2 - 1] = 0;		/* truncate at '.' and stop. */
			  break;
			}
		  exts++;
		}
	}

}


static int
msg_start_message_delivery_hack_attachments (struct mime_delivery_state *state,
					   const struct MSG_AttachmentData *attachments,
					   const struct MSG_AttachedFile *preloaded_attachments)
{
  if (preloaded_attachments) XP_ASSERT(!attachments);
  if (attachments) XP_ASSERT(!preloaded_attachments);

  if (preloaded_attachments && preloaded_attachments[0].orig_url)
	{
	  /* These are attachments which have already been downloaded to tmp files.
		 We merely need to point the internal attachment data at those tmp
		 files. */
	  int32 i;

	  state->pre_snarfed_attachments_p = TRUE;

	  state->attachment_count = 0;
	  while (preloaded_attachments[state->attachment_count].orig_url)
		state->attachment_count++;
	  state->attachments = (struct mime_attachment *)
		XP_ALLOC (sizeof (struct mime_attachment) * state->attachment_count);
	  if (! state->attachments)
		return MK_OUT_OF_MEMORY;
	  XP_MEMSET (state->attachments, 0,
				 sizeof (struct mime_attachment) * state->attachment_count);

	  for (i = 0; i < state->attachment_count; i++)
		{
		  state->attachments[i].state = state;
		  /* These attachments are already "snarfed". */
		  state->attachments[i].done = TRUE;
		  XP_ASSERT (preloaded_attachments[i].orig_url);
		  StrAllocCopy (state->attachments[i].url_string,
						preloaded_attachments[i].orig_url);
		  StrAllocCopy (state->attachments[i].type,
						preloaded_attachments[i].type);
		  StrAllocCopy (state->attachments[i].description,
						preloaded_attachments[i].description);
		  StrAllocCopy (state->attachments[i].x_mac_type,
						preloaded_attachments[i].x_mac_type);
		  StrAllocCopy (state->attachments[i].x_mac_creator,
						preloaded_attachments[i].x_mac_creator);
		  StrAllocCopy (state->attachments[i].encoding,
						preloaded_attachments[i].encoding);
		  StrAllocCopy (state->attachments[i].file_name,
						preloaded_attachments[i].file_name);

		  state->attachments[i].decrypted_p =
			preloaded_attachments[i].decrypted_p;

		  state->attachments[i].size = preloaded_attachments[i].size;
		  state->attachments[i].unprintable_count =
			preloaded_attachments[i].unprintable_count;
		  state->attachments[i].highbit_count =
			preloaded_attachments[i].highbit_count;
		  state->attachments[i].ctl_count = preloaded_attachments[i].ctl_count;
		  state->attachments[i].null_count =
			preloaded_attachments[i].null_count;
		  state->attachments[i].max_column =
			preloaded_attachments[i].max_line_length;

		  /* If the attachment has an encoding, and it's not one of
			 the "null" encodings, then keep it. */
		  if (state->attachments[i].encoding &&
			  strcasecomp (state->attachments[i].encoding, ENCODING_7BIT) &&
			  strcasecomp (state->attachments[i].encoding, ENCODING_8BIT) &&
			  strcasecomp (state->attachments[i].encoding, ENCODING_BINARY))
			state->attachments[i].already_encoded_p = TRUE;

		  msg_pick_real_name(&state->attachments[i]);
		}
	}
  else if (attachments && attachments[0].url)
	{
	  /* These are attachments which have already been downloaded to tmp files.
		 We merely need to point the internal attachment data at those tmp
		 files.  We will delete the tmp files as we attach them.
	   */
	  int32 i;
	  int mailbox_count = 0, news_count = 0;

	  state->attachment_count = 0;
	  while (attachments[state->attachment_count].url)
		state->attachment_count++;
	  state->attachments = (struct mime_attachment *)
		XP_ALLOC (sizeof (struct mime_attachment) * state->attachment_count);
	  if (! state->attachments)
		return MK_OUT_OF_MEMORY;
	  XP_MEMSET (state->attachments, 0,
				 sizeof (struct mime_attachment) * state->attachment_count);

	  for (i = 0; i < state->attachment_count; i++)
		{
		  state->attachments[i].state = state;
		  XP_ASSERT (attachments[i].url);
		  StrAllocCopy (state->attachments[i].url_string,
						attachments[i].url);
		  StrAllocCopy (state->attachments[i].override_type,
						attachments[i].real_type);
		  StrAllocCopy (state->attachments[i].override_encoding,
						attachments[i].real_encoding);
		  StrAllocCopy (state->attachments[i].desired_type,
						attachments[i].desired_type);
		  StrAllocCopy (state->attachments[i].description,
						attachments[i].description);
		  StrAllocCopy (state->attachments[i].x_mac_type,
						attachments[i].x_mac_type);
		  StrAllocCopy (state->attachments[i].x_mac_creator,
						attachments[i].x_mac_creator);
		  StrAllocCopy (state->attachments[i].encoding, "7bit");
		  state->attachments[i].url =
			NET_CreateURLStruct (state->attachments[i].url_string,
								 NET_DONT_RELOAD);

		  state->attachments[i].real_name = 0;

		  /* Count up attachments which are going to come from mail folders
			 and from NNTP servers. */
		  if (strncasecomp(state->attachments[i].url_string, "mailbox:",8))
			mailbox_count++;
		  else if (strncasecomp(state->attachments[i].url_string, "news:",5) ||
				   strncasecomp(state->attachments[i].url_string, "snews:",6))
			news_count++;

		  msg_pick_real_name(&state->attachments[i]);
		}

	  /* If there is more than one mailbox URL, or more than one NNTP url,
		 do the load in serial rather than parallel, for efficiency.
	   */
	  if (mailbox_count > 1 || news_count > 1)
		state->be_synchronous_p = TRUE;

	  state->attachment_pending_count = state->attachment_count;

	  /* Start the URL attachments loading (eventually, an exit routine will
		 call the done_callback). */

	  if (state->attachment_count == 1)
		FE_Progress(state->context, XP_GetString(MK_MSG_LOAD_ATTACHMNT));
	  else
		FE_Progress(state->context, XP_GetString(MK_MSG_LOAD_ATTACHMNTS));

	  for (i = 0; i < state->attachment_count; i++)
		{
		  /* This only returns a failure code if NET_GetURL was not called
			 (and thus no exit routine was or will be called.) */
		  int status = mime_snarf_attachment (&state->attachments [i]);
		  if (status < 0)
			return status;

		  if (state->be_synchronous_p)
			break;
		}
	}

  if (state->attachment_pending_count <= 0)
	/* No attachments - finish now (this will call the done_callback). */
	mime_gather_attachments (state);

  return 0;
}


void
msg_start_message_delivery(MWContext *context,
						  void      *fe_data,

						  const char *from,
						  const char *reply_to,
						  const char *to,
						  const char *cc,
						  const char *bcc,
						  const char *fcc,
						  const char *newsgroups,
						  const char *followup_to,
						  const char *organization,
						  const char *message_id,
						  const char *news_url,
						  const char *subject,
						  const char *references,
						  const char *other_random_headers,

						  XP_Bool digest_p,
						  XP_Bool dont_deliver_p,
						  XP_Bool queue_for_later_p,

						  XP_Bool encrypt_p,
						  XP_Bool sign_p,

						  const char *attachment1_type,
						  const char *attachment1_body,
						  uint32 attachment1_body_length,
						  const struct MSG_AttachmentData *attachments,
						  const struct MSG_AttachedFile *preloaded_attachments,
						  void (*message_delivery_done_callback)
						       (MWContext *context, 
								void *fe_data,
								int status,
								const char *error_message))
{
  struct mime_delivery_state *state = 0;
  int failure = 0;

  if (!attachment1_body || !*attachment1_body)
	attachment1_type = attachment1_body = 0;

  state = (struct mime_delivery_state *)
	XP_ALLOC (sizeof (struct mime_delivery_state));

  if (! state)
	{
	  failure = MK_OUT_OF_MEMORY;
	  goto FAIL;
	}

  XP_MEMSET (state, 0, sizeof(struct mime_delivery_state));

  state->context = context;
  state->fe_data = fe_data;
  state->message_delivery_done_callback = message_delivery_done_callback;

#ifdef GENERATE_MESSAGE_ID
  if (message_id) {
	state->message_id = XP_STRDUP(message_id);
	/* Don't bother checking for out of memory; if it fails, then we'll just
	   let the server generate the message-id, and suffer with the possibility
	   of duplicate messages.*/
  }
#endif

  /* Strip whitespace from beginning and end of body. */
  if (attachment1_body)
	{
	  const char *old = attachment1_body;
	  while (attachment1_body_length > 0 &&
			 XP_IS_SPACE (*attachment1_body))
		{
		  attachment1_body++;
		  attachment1_body_length--;
		}
	  /* but allow leading whitespace on the first line...  just strip out
		 leading blank lines and lines consisting only of whitespace. */
	  while (attachment1_body > old &&
			 (attachment1_body[-1] == ' ' ||
			  attachment1_body[-1] == '\t'))
		{
		  attachment1_body--;
		  attachment1_body_length++;
		}

	  while (attachment1_body_length > 0 &&
			 XP_IS_SPACE (attachment1_body [attachment1_body_length - 1]))
		{
		  attachment1_body_length--;
		}
	  if (attachment1_body_length <= 0)
		attachment1_body = 0;

	  if (attachment1_body)
		{
		  char *newb = (char *) XP_ALLOC (attachment1_body_length + 1);
		  if (! newb)
			{
			  failure = MK_OUT_OF_MEMORY;
			  goto FAIL;
			}
		  XP_MEMCPY (newb, attachment1_body, attachment1_body_length);
		  newb [attachment1_body_length] = 0;
		  state->attachment1_body = newb;
		  state->attachment1_body_length = attachment1_body_length;
		}
	}

  if (!news_url || !*news_url)
	news_url = "news:";
  
  StrAllocCopy (state->news_url, news_url);
  StrAllocCopy (state->attachment1_type, attachment1_type);
  StrAllocCopy (state->attachment1_encoding, "8bit");

  /* strip whitespace from and duplicate header fields. */
  state->from         = mime_fix_addr_header (from);
  state->reply_to     = mime_fix_addr_header (reply_to);
  state->to           = mime_fix_addr_header (to);
  state->cc           = mime_fix_addr_header (cc);
  state->fcc          = mime_fix_header (fcc);
  state->bcc          = mime_fix_addr_header (bcc);
  state->newsgroups   = mime_fix_news_header (newsgroups);
  state->followup_to  = mime_fix_news_header (followup_to);
  state->organization = mime_fix_header (organization);
  state->subject      = mime_fix_header (subject);
  state->references   = mime_fix_header (references);

  if (other_random_headers)
	state->other_random_headers = XP_STRDUP (other_random_headers);

  /* Check the fields for legitimacy, and run the callback if they're not
	 ok.  */
  failure = mime_sanity_check_fields (state->from, state->reply_to,
									  state->to, state->cc,
									  state->bcc, state->fcc,
									  state->newsgroups, state->followup_to,
									  state->subject, state->references,
									  state->organization,
									  state->other_random_headers);
  if (failure) goto FAIL;

  state->digest_p = digest_p;
  state->dont_deliver_p = dont_deliver_p;
  state->queue_for_later_p = queue_for_later_p;
  state->encrypt_p = encrypt_p;
  state->sign_p = sign_p;
  state->msg_file_name = WH_TempName (xpFileToPost, "nsmail");

  failure = msg_start_message_delivery_hack_attachments(state,
														attachments,
														preloaded_attachments);
  if (failure >= 0)
	return;

 FAIL:
  XP_ASSERT (failure);

  {
	char *err_msg = NET_ExplainErrorDetails (failure);
	message_delivery_done_callback (context, fe_data, failure, err_msg);
	if (err_msg) XP_FREE (err_msg);
	if (state) {
	  mime_free_message_state (state);
	  XP_FREE (state);
	}
  }
}



/* This is the main driving function of this module.  It generates a
   document of type message/rfc822, which contains the stuff provided.
   The first few arguments are the standard header fields that the
   generated document should have.

   `other_random_headers' is a string of additional headers that should
   be inserted beyond the standard ones.  If provided, it is just tacked
   on to the end of the header block, so it should have newlines at the
   end of each line, shouldn't have blank lines, multi-line headers
   should be properly continued, etc.

   `digest_p' says that most of the documents we are attaching are
   themselves messages, and so we should generate a multipart/digest
   container instead of multipart/mixed.  (It's a minor difference.)

   The full text of the first attachment is provided via `attachment1_type',
   `attachment1_body' and `attachment1_body_length'.  These may all be 0
   if all attachments are provided externally.

   Subsequent attachments are provided as URLs to load, described in the
   MSG_AttachmentData structures.

   If `dont_deliver_p' is false, then we actually deliver the message to the
   SMTP and/or NNTP server, and the message_delivery_done_callback will be
   invoked with the status.

   If `dont_deliver_p' is true, then we just generate the message, we don't
   actually deliver it, and the message_delivery_done_callback will be called
   with the name of the generated file.  The callback is responsible for both
   freeing the file name string, and deleting the file when it is done with
   it.  If an error occurred, then `status' will be negative and
   `error_message' may be an error message to display.  If status is non-
   negative, then `error_message' contains the file name (this is kind of
   a kludge...)
 */
void
MSG_StartMessageDelivery (MWContext *context,
						  void      *fe_data,

						  const char *from,
						  const char *reply_to,
						  const char *to,
						  const char *cc,
						  const char *bcc,
						  const char *fcc,
						  const char *newsgroups,
						  const char *followup_to,
						  const char *organization,
						  const char *message_id,
						  const char *news_url,
						  const char *subject,
						  const char *references,
						  const char *other_random_headers,

						  XP_Bool digest_p,
						  XP_Bool dont_deliver_p,
						  XP_Bool queue_for_later_p,

						  XP_Bool encrypt_p,
						  XP_Bool sign_p,

						  const char *attachment1_type,
						  const char *attachment1_body,
						  uint32 attachment1_body_length,
						  const struct MSG_AttachmentData *attachments,
						  void (*message_delivery_done_callback)
						       (MWContext *context, 
								void *fe_data,
								int status,
								const char *error_message))
{
  msg_start_message_delivery (context, fe_data, from, reply_to, to, cc, bcc,
							  fcc, newsgroups, followup_to, organization,
							  message_id, news_url, subject, references,
							  other_random_headers,
							  digest_p, dont_deliver_p, queue_for_later_p,
							  encrypt_p, sign_p,
							  attachment1_type, attachment1_body,
							  attachment1_body_length,
							  attachments, 0,
							  message_delivery_done_callback);
}


void
msg_StartMessageDeliveryWithAttachments (MWContext *context,
						  void      *fe_data,

						  const char *from,
						  const char *reply_to,
						  const char *to,
						  const char *cc,
						  const char *bcc,
						  const char *fcc,
						  const char *newsgroups,
						  const char *followup_to,
						  const char *organization,
						  const char *message_id,
						  const char *news_url,
						  const char *subject,
						  const char *references,
						  const char *other_random_headers,

						  XP_Bool digest_p,
						  XP_Bool dont_deliver_p,
						  XP_Bool queue_for_later_p,

						  XP_Bool encrypt_p,
						  XP_Bool sign_p,

						  const char *attachment1_type,
						  const char *attachment1_body,
						  uint32 attachment1_body_length,
						  const struct MSG_AttachedFile *attachments,
						  void (*message_delivery_done_callback)
						       (MWContext *context, 
								void *fe_data,
								int status,
								const char *error_message))
{
  msg_start_message_delivery (context, fe_data, from, reply_to, to, cc, bcc,
							  fcc, newsgroups, followup_to, organization,
							  message_id, news_url, subject, references,
							  other_random_headers,
							  digest_p, dont_deliver_p, queue_for_later_p,
							  encrypt_p, sign_p,
							  attachment1_type, attachment1_body,
							  attachment1_body_length,
							  0, attachments,
							  message_delivery_done_callback);
}


void
msg_DownloadAttachments (MWContext *context,
						 void *fe_data,
						 const struct MSG_AttachmentData *attachments,
						 void (*attachments_done_callback)
						      (MWContext *context, 
							   void *fe_data,
							   int status, const char *error_message,
							   struct MSG_AttachedFile *attachments))
{
  struct mime_delivery_state *state = 0;
  int failure = 0;

  XP_ASSERT(attachments && attachments[0].url);
  if (!attachments || !attachments[0].url)
	{
	  failure = -1;
	  goto FAIL;
	}
  state = (struct mime_delivery_state *)
	XP_ALLOC (sizeof (struct mime_delivery_state));

  if (! state)
	{
	  failure = MK_OUT_OF_MEMORY;
	  goto FAIL;
	}
  XP_MEMSET (state, 0, sizeof(struct mime_delivery_state));

  state->context = context;
  state->fe_data = fe_data;
  state->attachments_only_p = TRUE;
  state->attachments_done_callback = attachments_done_callback;

  failure = msg_start_message_delivery_hack_attachments(state, attachments, 0);
  if (failure >= 0)
	return;

 FAIL:
  XP_ASSERT (failure);

  {
	
	/* in this case, our NET_GetURL exit routine has already freed
	 the state */
	if (failure != MK_ATTACHMENT_LOAD_FAILED)
	{
		char *err_msg = NET_ExplainErrorDetails (failure);
		
		
		attachments_done_callback (context, fe_data, failure, err_msg, 0);
		if (state) {
		  mime_free_message_state (state);
		  XP_FREE (state);
		}
		
		if (err_msg) XP_FREE (err_msg);
	}
		
	
	
  }
}



void
MIME_BuildRFC822File (MWContext *context,
					  void      *fe_data,

					  const char *from,
					  const char *reply_to,
					  const char *to,
					  const char *cc,
					  const char *bcc,
					  const char *fcc,
					  const char *newsgroups,
					  const char *followup_to,
					  const char *organization,
					  const char *news_url,
					  const char *subject,
					  const char *references,
					  const char *other_random_headers,

					  const char *attachment1_type,
					  const char *attachment1_body,
					  uint32 attachment1_body_length,
					  const struct MSG_AttachmentData *attachments,
					  void (*message_file_written_callback)
					  (MWContext *context, 
					   void *fe_data,
					   int status,
					   const char *error_message))
{
  msg_start_message_delivery (context, fe_data,
							  from, reply_to, to, cc, bcc, fcc,
							  newsgroups, followup_to, organization,
							  0, /* message_id */
							  news_url, subject, references,
							  other_random_headers,
							  FALSE, /* digest_p */
							  TRUE,  /* dont_deliver_p */
							  FALSE, /* queue_for_later_p */
							  FALSE, /* encrypt_p */
							  FALSE, /* sign_p */
							  attachment1_type, attachment1_body,
							  attachment1_body_length,
							  attachments,
							  0, /* preloaded_attachments */
							  message_file_written_callback);
}


static void
mime_free_message_state (struct mime_delivery_state *state)
{
  if (state->to) 					XP_FREE (state->to);
  if (state->newsgroups)			XP_FREE (state->newsgroups);
  if (state->news_url)	 			XP_FREE (state->news_url);
  if (state->from)	 				XP_FREE (state->from);
  if (state->organization) 			XP_FREE (state->organization);
  if (state->message_id)			XP_FREE (state->message_id);
  if (state->subject) 				XP_FREE (state->subject);
  if (state->references) 			XP_FREE (state->references);
  if (state->attachment1_type)	 	XP_FREE (state->attachment1_type);
  if (state->attachment1_encoding)	XP_FREE (state->attachment1_encoding);
  if (state->attachment1_body)		XP_FREE (state->attachment1_body);

  if (state->attachment1_encoder_data)
	{
	  MimeEncoderDestroy(state->attachment1_encoder_data, TRUE);
	  state->attachment1_encoder_data = 0;
	}

/*  if (state->headers)				XP_FREE (state->headers); */

  if (state->msg_file)
	{
	  XP_FileClose (state->msg_file);
	  state->msg_file = 0;
	  XP_ASSERT (state->msg_file_name);
	}
  if (state->msg_file_name)
	{
	  XP_FileRemove (state->msg_file_name, xpFileToPost);
	  XP_FREE (state->msg_file_name);
	  state->msg_file_name = 0;
	}

  if (state->attachments)
	{
	  int i;
	  for (i = 0; i < state->attachment_count; i++)
		{
		  if (state->attachments [i].encoder_data)
			{
			  MimeEncoderDestroy(state->attachments [i].encoder_data,
								 TRUE);
			  state->attachments [i].encoder_data = 0;
			}

		  FREEIF (state->attachments [i].url_string);
		  if (state->attachments [i].url)
			NET_FreeURLStruct (state->attachments [i].url);
		  FREEIF (state->attachments [i].type);
		  FREEIF (state->attachments [i].override_type);
		  FREEIF (state->attachments [i].override_encoding);
		  FREEIF (state->attachments [i].desired_type);
		  FREEIF (state->attachments [i].description);
		  FREEIF (state->attachments [i].x_mac_type);
		  FREEIF (state->attachments [i].x_mac_creator);
		  FREEIF (state->attachments [i].real_name);
		  FREEIF (state->attachments [i].encoding);
		  if (state->attachments [i].file)
			XP_FileClose (state->attachments [i].file);
		  if (state->attachments [i].file_name)
			{
			  if (!state->pre_snarfed_attachments_p)
				XP_FileRemove (state->attachments [i].file_name, xpFileToPost);
			  XP_FREE (state->attachments [i].file_name);
			}
#ifdef XP_MAC
			/* remove the appledoubled intermediate file after we done all.
			 */
		  if (state->attachments [i].ap_filename)
			{
			  XP_FileRemove (state->attachments [i].ap_filename, xpFileToPost);
			  XP_FREE (state->attachments [i].ap_filename);
			}
#endif /* XP_MAC */
		}
	  XP_MEMSET (state->attachments, 0,
				 sizeof (*state->attachments) * state->attachment_count);
	  XP_FREE (state->attachments);
	}

  XP_MEMSET (state, 0, sizeof (*state));
}


static void mime_deliver_file_as_news (struct mime_delivery_state *state);
static void mime_deliver_file_as_mail (struct mime_delivery_state *state);
static XP_Bool mime_do_fcc (struct mime_delivery_state *state);
static void mime_queue_for_later (struct mime_delivery_state *state);

static void
mime_deliver_message (struct mime_delivery_state *state)
{
  XP_Bool mail_p = (state->to || state->cc || state->bcc);
  XP_Bool news_p = !!state->newsgroups;

  XP_ASSERT(mail_p || news_p);

#if 0
  /* Figure out how many bytes we're actually going to be writing, total.
   */
  state->delivery_bytes = 0;
  state->delivery_total_bytes = 0;

  if (state->fcc && *state->fcc)
	state->delivery_total_bytes += state->msg_size;

  if (state->queue_for_later_p)
	state->delivery_total_bytes += state->msg_size;
  else
	{
	  if (mail_p)
		state->delivery_total_bytes += state->msg_size;
	  if (news_p)
		state->delivery_total_bytes += state->msg_size;
	}
#endif /* 0 */

  if (state->queue_for_later_p)
	{
	  mime_queue_for_later (state);
	  return;
	}

  if (state->fcc)
	if (!mime_do_fcc (state))
	  return;

#ifdef XP_UNIX
  {
	int status = msg_DeliverMessageExternally(state->context,
											  state->msg_file_name);
	if (status != 0)
	  {
		if (status < 0)
		  mime_fail (state, status, 0);
		else
		  {
			/* The message has now been delivered successfully. */
			MWContext *context = state->context;
			if (state->message_delivery_done_callback)
			  state->message_delivery_done_callback (context,
													 state->fe_data, 0, NULL);
			state->message_delivery_done_callback = 0;

			mime_free_message_state (state);

			/* When attaching, even though the context has
			   active_url_count == 0, XFE_AllConnectionsComplete() **is**
			   called.  However, when not attaching, and not delivering right
			   away, we don't actually open any URLs, so we need to destroy
			   the window ourself.  Ugh!!
			 */
			if (state->attachment_count == 0)
			  MSG_MailCompositionAllConnectionsComplete (context);
		  }
		return;
	  }
  }
#endif /* XP_UNIX */


#ifdef MAIL_BEFORE_NEWS
  if (mail_p)
	mime_deliver_file_as_mail (state);   /* May call ...as_news() next. */
  else if (news_p)
	mime_deliver_file_as_news (state);
#else  /* !MAIL_BEFORE_NEWS */
  if (news_p)
	mime_deliver_file_as_news (state);   /* May call ...as_mail() next. */
  else if (mail_p)
	mime_deliver_file_as_mail (state);
#endif /* !MAIL_BEFORE_NEWS */
  else
	abort ();
}


#if 0
static void
mime_delivery_thermo (struct mime_delivery_state *state, int32 increment)
{
  int32 percent;
  state->delivery_bytes += increment;
  XP_ASSERT(state->delivery_total_bytes > 0);
  if (state->delivery_total_bytes <= 0) return;
  percent = 100 * (((double) state->delivery_bytes) /
				   ((double) state->delivery_total_bytes));
  FE_SetProgressBarPercent (state->context, percent);
#if 0
  FE_GraphProgress (state->context, 0,
					state->delivery_bytes, 0,
					state->delivery_total_bytes);
#endif
}
#endif /* 0 */

static void mime_deliver_as_mail_exit (URL_Struct *, int status, MWContext *);
static void mime_deliver_as_news_exit (URL_Struct *url, int status,
									   MWContext *);

static void
mime_deliver_file_as_mail (struct mime_delivery_state *state)
{
  char *buf, *buf2;
  URL_Struct *url;
  char *fn = XP_STRDUP(state->msg_file_name);

  if (!fn)
	{
	  mime_fail (state, MK_OUT_OF_MEMORY, 0);
	  XP_FREE (state);
	  return;
	}

  FE_Progress (state->context, XP_GetString(MK_MSG_DELIV_MAIL));

  buf = (char *) XP_ALLOC ((state->to  ? XP_STRLEN (state->to)  + 10 : 0) +
						   (state->cc  ? XP_STRLEN (state->cc)  + 10 : 0) +
						   (state->bcc ? XP_STRLEN (state->bcc) + 10 : 0) +
						   10);
  if (! buf)
	{
	  XP_FREE(fn);
	  mime_fail (state, MK_OUT_OF_MEMORY, 0);
	  XP_FREE (state);
	  return;
	}
  XP_STRCPY (buf, "mailto:");
  buf2 = buf + XP_STRLEN (buf);
  if (state->to)
	{
	  XP_STRCAT (buf2, state->to);
	}
  if (state->cc)
	{
	  if (*buf2) XP_STRCAT (buf2, ",");
	  XP_STRCAT (buf2, state->cc);
	}
  if (state->bcc)
	{
	  if (*buf2) XP_STRCAT (buf2, ",");
	  XP_STRCAT (buf2, state->bcc);
	}

  url = NET_CreateURLStruct (buf, NET_DONT_RELOAD);
  XP_FREE (buf);
  if (! url)
	{
	  XP_FREE(fn);
	  mime_fail (state, MK_OUT_OF_MEMORY, 0);
	  XP_FREE (state);
	  return;
	}

  /* put the filename of the message into the post data field and set a flag
	 in the URL struct to specify that it is a file
   */
  url->post_data = fn;
  url->post_data_size = XP_STRLEN(fn);
  url->post_data_is_file = TRUE;
  url->method = URL_POST_METHOD;
  url->fe_data = state;
  url->internal_url = TRUE;

  /* We can ignore the return value of NET_GetURL() because we have
	 handled the error in mime_deliver_as_mail_exit(). */
  NET_GetURL (url, FO_CACHE_AND_PRESENT, state->context,
			  mime_deliver_as_mail_exit);
}

static void
mime_deliver_file_as_news (struct mime_delivery_state *state)
{
  URL_Struct *url = NET_CreateURLStruct (state->news_url, NET_DONT_RELOAD);
  char *fn = XP_STRDUP(state->msg_file_name);

  if (!fn)
	{
	  mime_fail (state, MK_OUT_OF_MEMORY, 0);
	  XP_FREE (state);
	  return;
	}

  if (! url)
	{
	  XP_FREE(fn);
	  mime_fail (state, MK_OUT_OF_MEMORY, 0);
	  XP_FREE (state);
	  return;
	}

  FE_Progress (state->context, XP_GetString(MK_MSG_DELIV_NEWS));

	/* put the filename of the message into the post
	 * data field and set a flag in the URL struct
	 * to specify that it is a file
	 */
  url->post_data = fn;
  url->post_data_size = XP_STRLEN(fn);
  url->post_data_is_file = TRUE;
  url->method = URL_POST_METHOD;

  url->fe_data = state;
  url->internal_url = TRUE;

  /* We can ignore the return value of NET_GetURL() because we have
	 handled the error in mime_deliver_as_news_exit(). */
  NET_GetURL (url, FO_CACHE_AND_PRESENT, state->context,
			  mime_deliver_as_news_exit);
}

static void 
mime_deliver_as_mail_exit (URL_Struct *url, int status, MWContext *context)
{
  struct mime_delivery_state *state =
	(struct mime_delivery_state *) url->fe_data;
  char *error_msg = 0;

  url->fe_data = 0;
  if (status < 0 && url->error_msg)
    {
	  error_msg = url->error_msg;
	  url->error_msg = 0;
    }
  NET_FreeURLStruct (url);

  if (status < 0)
	{
	  mime_fail (state, status, error_msg);
	  XP_FREE (state);
	}
#ifdef MAIL_BEFORE_NEWS
  else if (state->newsgroups)
	{
	  /* If we're sending this mail message to news as well, start it now.
		 Completion and further errors will be handled there.
	   */
	  mime_deliver_file_as_news (state);
	}
#endif /* MAIL_BEFORE_NEWS */
  else
	{
	  /* The message has now been sent successfully! */
	  FE_Progress (state->context, XP_GetString(MK_MSG_DELIV_MAIL_DONE));
	  if (state->message_delivery_done_callback)
		state->message_delivery_done_callback (state->context,
											   state->fe_data, 0, NULL);
	  state->message_delivery_done_callback = 0;
	  mime_free_message_state (state);
	  XP_FREE (state);
	}
}

static void 
mime_deliver_as_news_exit (URL_Struct *url, int status, MWContext *context)
{
  struct mime_delivery_state *state =
	(struct mime_delivery_state *) url->fe_data;
  char *error_msg = 0;

  url->fe_data = 0;
  if (status < 0 && url->error_msg)
    {
	  error_msg = url->error_msg;
	  url->error_msg = 0;
    }
  NET_FreeURLStruct (url);

  if (status < 0)
	{
	  mime_fail (state, status, error_msg);
	  XP_FREE (state);
	}
#ifndef MAIL_BEFORE_NEWS
  else if (state->to || state->cc || state->bcc)
	{
	  /* If we're sending this news message to mail as well, start it now.
		 Completion and further errors will be handled there.
	   */
	  mime_deliver_file_as_mail (state);
	}
#endif /* !MAIL_BEFORE_NEWS */
  else
	{
	  /* The message has now been sent successfully! */
	  FE_Progress (state->context, XP_GetString(MK_MSG_DELIV_NEWS_DONE));
	  if (state->message_delivery_done_callback)
		state->message_delivery_done_callback (state->context,
											   state->fe_data, 0, NULL);
	  state->message_delivery_done_callback = 0;
	  mime_free_message_state (state);
	  XP_FREE (state);
	}
}


/* #### move this to a header file -- also used by newsgrp.c */
int32 msg_do_fcc_handle_line(char* line, uint32 length, void* closure);

static int
mime_do_fcc_1 (MWContext *context,
			   const char *input_file_name,  XP_FileType input_file_type,
			   const char *output_file_name, XP_FileType output_file_type,
			   XP_Bool queue_p,
			   const char *bcc_header,
			   const char *fcc_header,
			   const char *news_url)
{
  int status = 0;
  XP_File in = 0;
  XP_File out = 0;
  XP_Bool file_existed_p;
  XP_StatStruct st;
  char *ibuffer = 0;
  int ibuffer_size = 4096;
  char *obuffer = 0;
  int32 obuffer_size = 0, obuffer_fp = 0;
  int32 n;
  XP_Bool summary_valid = FALSE;
  XP_Bool mark_as_read = TRUE;
/*  XP_Bool mark_as_read = FALSE; */

  if (queue_p)
	FE_Progress (context, XP_GetString(MK_MSG_QUEUEING));
  else
	FE_Progress (context, XP_GetString(MK_MSG_WRITING_TO_FCC));


  ibuffer = (char *) XP_ALLOC (ibuffer_size);
  if (!ibuffer)
	{
	  status = MK_OUT_OF_MEMORY;
	  goto FAIL;
	}

  file_existed_p = !XP_Stat (output_file_name, &st, output_file_type);

  if (file_existed_p)
	{
	  summary_valid = msg_IsSummaryValid(output_file_name, &st);
	  if (!msg_ConfirmMailFile (context, output_file_name)) {
		return MK_INTERRUPTED; /* #### What a hack.  It turns out we already
								  were testing for this result code and
								  silently canceling the send if we ever got
								  it (because it meant that the user hit the
								  Stop button).  Returning it here has a
								  similar effect -- the user was asked to
								  confirm writing to the FCC folder, and he
								  hit the Cancel button, so we now quietly
								  do nothing. */
	  }
	}
  else
	{
	  /* We're creating a new file.  If this is a mail folder, and we
		 already have a mail context, tell it to create the new file and
		 display it. */
	  MWContext* mailctx = XP_FindContextOfType(NULL, MWContextMail);
	  if (mailctx) {
		const char* dir = FE_GetFolderDirectory(mailctx);
		if (dir && XP_STRNCMP(output_file_name, dir, XP_STRLEN(dir)) == 0) {
		  (void) msg_FindMailFolder(mailctx, output_file_name, TRUE);
		  if (!XP_Stat (output_file_name, &st, output_file_type)) {
			summary_valid = msg_IsSummaryValid(output_file_name, &st);
		  }
		}
	  }
	}


  out = XP_FileOpen (output_file_name, output_file_type, XP_FILE_APPEND_BIN);
  if (!out)
	{
	  /* #### include file name in error message! */
	  status = MK_MSG_COULDNT_OPEN_FCC_FILE;
	  goto FAIL;
	}

  in = XP_FileOpen (input_file_name, input_file_type, XP_FILE_READ_BIN);
  if (!in)
	{
	  status = -1; /* How did this happen? */
	  goto FAIL;
	}

  /* Write a BSD mailbox envelope line to the file.
	 If the file existed, preceed it by a linebreak: this file format wants a
	 *blank* line before all "From " lines except the first.  This assumes
	 that the previous message in the file was properly closed, that is, that
	 the last line in the file ended with a linebreak.
   */
  {
	const char *envelope = msg_GetDummyEnvelope();
	if (file_existed_p && st.st_size > 0) {
	  if (XP_FileWrite (LINEBREAK, LINEBREAK_LEN, out) < LINEBREAK_LEN) {
		status = MK_MIME_ERROR_WRITING_FILE;
		goto FAIL;
	  }
	}
	if (XP_FileWrite (envelope, XP_STRLEN (envelope),
					  out) < XP_STRLEN(envelope)) {
	  status = MK_MIME_ERROR_WRITING_FILE;
	  goto FAIL;
	}
  }

  /* Write out an X-Mozilla-Status header.

	 This is required for the queue file, so that we can overwrite it once
	 the messages have been delivered, and so that the MSG_FLAG_QUEUED bit
	 is set.

	 For FCC files, we don't necessarily need one, but we might as well put
	 one in so that it's marked as read already.
   */
  if (queue_p || mark_as_read)
	{
	  char buf[X_MOZILLA_STATUS_LEN + 10];
	  uint16 flags = 0;

	  mark_as_read = TRUE;
	  flags |= MSG_FLAG_READ;
	  if (queue_p)
		flags |= MSG_FLAG_QUEUED;
	  PR_snprintf(buf, sizeof(buf),
				  X_MOZILLA_STATUS ": %04x" LINEBREAK, flags);
	  if (XP_FileWrite (buf, XP_STRLEN(buf), out) < XP_STRLEN(buf)) {
 		status = MK_MIME_ERROR_WRITING_FILE;
		goto FAIL;
	  }
	}

  /* Write out the FCC and BCC headers.

	 When writing to the Queue file, we *must* write the FCC and BCC
	 headers, or else that information would be lost.  Because, when actually
	 delivering the message (with "deliver now") we do FCC/BCC right away;
	 but when queueing for later delivery, we do FCC/BCC at delivery-time.

	 The question remains of whether FCC and BCC should be written into normal
	 BCC folders (like the Sent Mail folder.)

	 For FCC, there seems no point to do that; it's not information that one
	 would want to refer back to.

	 For BCC, the question isn't as clear.  On the one hand, if I send someone
	 a BCC'ed copy of the message, and save a copy of it for myself (with FCC)
	 I might want to be able to look at that message later and see the list of
	 people to whom I had BCC'ed it.

	 On the other hand, the contents of the BCC header is sensitive
	 information, and should perhaps not be stored at all.

	 Thus the consultation of the #define SAVE_BCC_IN_FCC_FILE.

	 (Note that, if there is a BCC header present in a message in some random
	 folder, and that message is forwarded to someone, then the attachment
	 code will strip out the BCC header before forwarding it.)
   */
  if (fcc_header && *fcc_header && queue_p)
	{
	  int32 L = XP_STRLEN(fcc_header) + 20;
	  char *buf = (char *) XP_ALLOC (L);
	  if (!buf)
		{
		  status = MK_OUT_OF_MEMORY;
		  goto FAIL;
		}
	  PR_snprintf(buf, L-1, "FCC: %s" LINEBREAK, fcc_header);
	  L = XP_STRLEN(buf);
	  if (XP_FileWrite (buf, L, out) < L)
		{
		  status = MK_MIME_ERROR_WRITING_FILE;
		  goto FAIL;
		}
	}

  if (bcc_header && *bcc_header
#ifndef SAVE_BCC_IN_FCC_FILE
	  && queue_p
#endif
	  )
	{
	  int32 L = XP_STRLEN(bcc_header) + 20;
	  char *buf = (char *) XP_ALLOC (L);
	  if (!buf)
		{
		  status = MK_OUT_OF_MEMORY;
		  goto FAIL;
		}
	  PR_snprintf(buf, L-1, "BCC: %s" LINEBREAK, bcc_header);
	  L = XP_STRLEN(buf);
	  if (XP_FileWrite (buf, L, out) < L)
		{
		  status = MK_MIME_ERROR_WRITING_FILE;
		  goto FAIL;
		}
	}


  /* Write out the X-Mozilla-News-Host header.
	 This is done only when writing to the queue file, not the FCC file.
	 We need this to complement the "Newsgroups" header for the case of
	 queueing a message for a non-default news host.

	 Convert a URL like "snews://host:123/" to the form "host:123/secure"
	 or "news://user@host:222" to simply "host:222".
   */
  if (queue_p && news_url && *news_url)
	{
	  XP_Bool secure_p = (news_url[0] == 's' || news_url[0] == 'S');
	  char *orig_hap = NET_ParseURL (news_url, GET_HOST_PART);
	  char *host_and_port = orig_hap;
	  if (host_and_port)
		{
		  /* There may be authinfo at the front of the host - it could be of
			 the form "user:password@host:port", so take off everything before
			 the first at-sign.  We don't want to store authinfo in the queue
			 folder, I guess, but would want it to be re-prompted-for at
			 delivery-time.
		   */
		  char *at = XP_STRCHR (host_and_port, '@');
		  if (at) host_and_port = at + 1;
		}

	  if ((host_and_port && *host_and_port) || !secure_p)
		{
		  char *line = PR_smprintf(X_MOZILLA_NEWSHOST ": %s%s" LINEBREAK,
								   host_and_port ? host_and_port : "",
								   secure_p ? "/secure" : "");
		  int L;
		  FREEIF(orig_hap);
		  if (!line)
			{
			  status = MK_OUT_OF_MEMORY;
			  goto FAIL;
			}
		  L = XP_STRLEN(line);
		  if (XP_FileWrite (line, L, out) < L)
			{
			  status = MK_MIME_ERROR_WRITING_FILE;
			  goto FAIL;
			}
		}
	  FREEIF(orig_hap);
	}


  /* Read from the message file, and write to the FCC or Queue file.
	 There are two tricky parts: the first is thta the message file
	 uses CRLF, and the FCC file should use LINEBREAK.  The second
	 is that the message file may have lines beginning with "From "
	 but the FCC file must have those lines mangled.

	 It's unfortunate that we end up writing the FCC file a line
	 at a time, but it's the easiest way...
   */
  while (1)
	{
	  n = XP_FileRead (ibuffer, ibuffer_size, in);
	  if (n == 0)
		break;
	  if (n < 0) /* read failed (not eof) */
		{
		  status = n;
		  goto FAIL;
		}

	  n = msg_LineBuffer (ibuffer, n,
						  &obuffer, (uint32 *)&obuffer_size,
						  (uint32*)&obuffer_fp,
						  TRUE, msg_do_fcc_handle_line,
						  out);
	  if (n < 0) /* write failed */
		{
		  status = n;
		  goto FAIL;
		}
	}

  /* If there's still stuff in the buffer (meaning the last line had no
	 newline) push it out. */
  if (obuffer_fp > 0)
	msg_do_fcc_handle_line (obuffer, obuffer_fp, out);

  /* Terminate with a final newline. */
  if (XP_FileWrite (LINEBREAK, LINEBREAK_LEN, out) < LINEBREAK_LEN) {
	status = MK_MIME_ERROR_WRITING_FILE;
  }

 FAIL:

  if (ibuffer)
	XP_FREE (ibuffer);
  if (obuffer && obuffer != ibuffer)
	XP_FREE (obuffer);

  if (in)
	XP_FileClose (in);

  if (out)
	{
	  if (status >= 0)
		{
		  XP_FileClose (out);
		  if (summary_valid)
			msg_SetSummaryValid(output_file_name, 1, (mark_as_read ? 0 : 1));
		}
	  else if (! file_existed_p)
		{
		  XP_FileClose (out);
		  XP_FileRemove (output_file_name, output_file_type);
		}
	  else
		{
		  /* #### truncate file to `st.st_size' */
		  XP_FileClose (out);
		}
	}

  if (status < 0)
	{
	  /* Fail, and terminate. */

	  if (status == -1)
		status = MK_MIME_ERROR_WRITING_FILE; /* #### wrong error */

	  return status;
	}
  else
	{
	  /* Otherwise, continue on to _deliver_as_mail or _deliver_as_news
		 or mime_queue_for_later. */
	  return 0;
	}
}

int32
msg_do_fcc_handle_line(char* line, uint32 length, void* closure)
{
  XP_File out = (XP_File) closure;

#ifdef MANGLE_INTERNAL_ENVELOPE_LINES
  /* Note: it is correct to mangle all lines beginning with "From ",
	 not just those that look like parsable message delimiters.
	 Other software expects this. */
  if (length >= 5 && line[0] == 'F' && !XP_STRNCMP(line, "From ", 5))
	{
	  if (XP_FileWrite (">", 1, out) < 1) return MK_MIME_ERROR_WRITING_FILE;
	}
#endif /* MANGLE_INTERNAL_ENVELOPE_LINES */

  /* #### if XP_FileWrite is a performance problem, we can put in a
	 call to msg_ReBuffer() here... */
  if (XP_FileWrite (line, length, out) < length) {
	return MK_MIME_ERROR_WRITING_FILE;
  }
  return 0;
}


int
msg_DoFCC (MWContext *context,
		   const char *input_file,  XP_FileType input_file_type,
		   const char *output_file, XP_FileType output_file_type,
		   const char *bcc_header_value,
		   const char *fcc_header_value)
{
  XP_ASSERT(context &&
			input_file && *input_file &&
			output_file && *output_file);
  if (! (context &&
		 input_file && *input_file &&
		 output_file && *output_file))
	return -1;
  return mime_do_fcc_1 (context,
						input_file, input_file_type,
						output_file, output_file_type,
						FALSE,
						bcc_header_value,
						fcc_header_value,
						0);
}


/* Returns false if an error happened. */
static XP_Bool
mime_do_fcc (struct mime_delivery_state *state)
{
  if (!state->fcc || !*state->fcc)
	return TRUE;
  else
	{
	  int status = msg_DoFCC (state->context,
							  state->msg_file_name, xpFileToPost,
							  state->fcc, xpMailFolder,
							  state->bcc,
							  state->fcc);
	  if (status < 0)
		mime_fail (state, status, 0);
	  return (status >= 0);
	}
}


/* Queues the message for later delivery, and runs the completion/failure
   callback.
 */
static void
mime_queue_for_later (struct mime_delivery_state *state)
{
  char *name = 0;
  int status = 0;
  name = msg_QueueFolderName(state->context);
  if (!name)
	{
	  status = MK_OUT_OF_MEMORY;
	  goto FAIL;
	}
  msg_EnsureQueueFolderListed(state->context);
  status = mime_do_fcc_1 (state->context,
						  state->msg_file_name, xpFileToPost,
						  name, xpMailFolder,
						  TRUE, state->bcc, state->fcc,
						  (state->newsgroups && *state->newsgroups
						   ? state->news_url : 0));
  XP_FREE (name);
  if (status < 0)
	goto FAIL;

FAIL:

  if (status < 0)
	{
	  mime_fail (state, status, 0);
	}
  else
	{
	  MWContext *context = state->context;
	  FE_Progress(context, XP_GetString(MK_MSG_QUEUED));

	  /* The message has now been queued successfully. */
	  if (state->message_delivery_done_callback)
		state->message_delivery_done_callback (context,
											   state->fe_data, 0, NULL);
	  state->message_delivery_done_callback = 0;

	  mime_free_message_state (state);

	  /* When attaching, even though the context has active_url_count == 0,
		 XFE_AllConnectionsComplete() **is** called.  However, when not
		 attaching, and not delivering right away, we don't actually open
		 any URLs, so we need to destroy the window ourself.  Ugh!!
	   */
	  if (state->attachment_count == 0)
		MSG_MailCompositionAllConnectionsComplete (context);
	}
}


#ifdef _USRDLL

PUBLIC void
NET_RegisterDLLContentConverters()
{

  RealNET_RegisterContentTypeConverter ("*", FO_MAIL_TO,
										NULL, mime_make_attachment_stream);
  RealNET_RegisterContentTypeConverter ("*", FO_CACHE_AND_MAIL_TO,
										NULL, mime_make_attachment_stream);

  /* #### I don't understand this -- what is this function for?
	 Is this right?  I've cloned this stuff from MSG_RegisterConverters()
	 above.  --jwz */

  RealNET_RegisterContentTypeConverter ("*", FO_MAIL_MESSAGE_TO,
									NULL, mime_make_attachment_stream);
  RealNET_RegisterContentTypeConverter ("*", FO_CACHE_AND_MAIL_MESSAGE_TO,
									NULL, mime_make_attachment_stream);

  RealNET_RegisterContentTypeConverter (MESSAGE_RFC822, FO_MAIL_MESSAGE_TO,
									NULL, MIME_MessageConverter);
  RealNET_RegisterContentTypeConverter (MESSAGE_NEWS, FO_MAIL_MESSAGE_TO,
									NULL, MIME_MessageConverter);
  RealNET_RegisterContentTypeConverter (MESSAGE_RFC822,
									FO_CACHE_AND_MAIL_MESSAGE_TO,
									NULL, MIME_MessageConverter);
  RealNET_RegisterContentTypeConverter (MESSAGE_NEWS,
									FO_CACHE_AND_MAIL_MESSAGE_TO,
									NULL, MIME_MessageConverter);
}

#endif



