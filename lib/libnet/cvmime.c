/* -*- Mode: C; tab-width: 4 -*-
   cvmime.c --- implements filters for the standard MIME encodings.
   Copyright © 1996 Netscape Communications Corporation, all rights reserved.
   Created: Jamie Zawinski <jwz@mcom.com>, 30-Dec-94.
 */

/* Please leave outside of ifdef for window precompiled headers */
#include "mkutils.h"

#ifdef MOZILLA_CLIENT


#include "xp.h"
#include "mkstream.h"
#include "mkgeturl.h"
#include "cvextcon.h"
#include "mkformat.h"
#include "il.h"
#include "mime.h"
#include "cvactive.h"
#include "gui.h"
#include "msgcom.h"
#include "mkautocf.h"	/* Proxy auto-config */
#include "mkhelp.h"

#include "xlate.h"		/* Text and PostScript converters */
#include "libi18n.h"		/* For INTL_ConvCharCode()   */
#include "edt.h"
#include "secnav.h"
#include "cert.h"

#include "m_cvstrm.h"

#include "mimeenc.h"

#include "xpgetstr.h"
extern int XP_EDITOR_NON_HTML;


/* #### defined in libmsg/msgutils.c */
extern NET_StreamClass * 
msg_MakeRebufferingStream (NET_StreamClass *next_stream,
						   URL_Struct *url,
						   MWContext *context);


/* defined at the bottom of this file */
NET_StreamClass *
NET_PrintRawToDisk(int format_out, 
				   void *data_obj,
                   URL_Struct *url_struct, 
				   MWContext *context);


typedef struct MIME_DataObject {
  MimeDecoderData *decoder;		/* State used by the decoder */  
  NET_StreamClass *next_stream;	/* Where the output goes */
  XP_Bool partial_p;			/* Whether we should close that stream */
} MIME_DataObject;


static int
net_mime_decoder_cb (const char *buf, int32 size, void *closure)
{
  MIME_DataObject *obj = (MIME_DataObject *) closure;
  NET_StreamClass *stream = (obj ? obj->next_stream : 0);
  if (stream)
	return stream->put_block (stream->data_object, (char *) buf, size);
  else
	return 0;
}


static int
net_MimeEncodingConverterWrite (void *closure, CONST char* buffer,
								int32 length)
{
  MIME_DataObject *obj = (MIME_DataObject *) closure;
  return MimeDecoderWrite (obj->decoder, (char *) buffer, length);
}


PRIVATE unsigned int net_MimeEncodingConverterWriteReady (void *closure)
{
  /* #### I'm not sure what the right thing to do here is. --jwz */
#if 1
  return (MAX_WRITE_READY);
#else
  MIME_DataObject *data = (MIME_DataObject *) closure;
  if(data->next_stream)
    return ((*data->next_stream->is_write_ready)
			(data->next_stream->data_object));
  else
    return (MAX_WRITE_READY);
#endif
}

PRIVATE void net_MimeEncodingConverterComplete (void *closure)
{
  MIME_DataObject *data = (MIME_DataObject *) closure;

  if (data->decoder)
	{
	  MimeDecoderDestroy(data->decoder, FALSE);
	  data->decoder = 0;
	}

  /* complete the next stream */
  if (!data->partial_p && data->next_stream)
    {
      (*data->next_stream->complete) (data->next_stream->data_object);
      XP_FREE (data->next_stream);
    }
  XP_FREE (data);
}

PRIVATE void net_MimeEncodingConverterAbort (void *closure, int status)
{
  MIME_DataObject *data = (MIME_DataObject *) closure;

  if (data->decoder)
	{
	  MimeDecoderDestroy(data->decoder, TRUE);
	  data->decoder = 0;
	}

  /* abort the next stream */
  if (!data->partial_p && data->next_stream)
    {
      (*data->next_stream->abort) (data->next_stream->data_object, status);
      XP_FREE (data->next_stream);
    }
  XP_FREE (data);
}


PRIVATE NET_StreamClass * 
NET_MimeEncodingConverter_1 (int          format_out,
							 void        *data_obj,
							 URL_Struct  *URL_s,
							 MWContext   *window_id,
							 XP_Bool partial_p,
							 NET_StreamClass *next_stream)
{
   MIME_DataObject* obj;
   MimeDecoderData *(*fn) (int (*) (const char*, int32, void*), void*) = 0;

    NET_StreamClass* stream;
	char *type = (char *) data_obj;

    TRACEMSG(("Setting up encoding stream. Have URL: %s\n", URL_s->address));

    stream = XP_NEW(NET_StreamClass);
    if(stream == NULL) 
	  return(NULL);

	XP_MEMSET(stream, 0, sizeof(NET_StreamClass));

    obj = XP_NEW(MIME_DataObject);
    if (obj == NULL) 
	  return(NULL);
	memset(obj, 0, sizeof(MIME_DataObject));

	if (!strcasecomp (type, ENCODING_QUOTED_PRINTABLE))
	  fn = &MimeQPDecoderInit;
	else if (!strcasecomp (type, ENCODING_BASE64))
	  fn = &MimeB64DecoderInit;
	else if (!strcasecomp (type, ENCODING_UUENCODE) ||
			 !strcasecomp (type, ENCODING_UUENCODE2) ||
			 !strcasecomp (type, ENCODING_UUENCODE3) ||
			 !strcasecomp (type, ENCODING_UUENCODE4))
	  fn = &MimeUUDecoderInit;
	else
	  abort ();

	obj->decoder = fn (net_mime_decoder_cb, obj);
	if (!obj->decoder)
	  {
		XP_FREE(obj);
		return 0;
	  }

    stream->put_block      = net_MimeEncodingConverterWrite;

    stream->name           = "Mime Stream";
    stream->complete       = net_MimeEncodingConverterComplete;
    stream->abort          = net_MimeEncodingConverterAbort;
    stream->is_write_ready = net_MimeEncodingConverterWriteReady;
    stream->data_object    = obj;  /* document info object */
    stream->window_id      = window_id;

	if (partial_p)
	  {
		XP_ASSERT (next_stream);
		obj->next_stream = next_stream;
		TRACEMSG(("Using existing stream in NET_MimeEncodingConverter\n"));
	  }
	else
	  {
		XP_ASSERT (!next_stream);

		/* open next stream
		 */
		FREEIF (URL_s->content_encoding);
		obj->next_stream = NET_StreamBuilder (format_out, URL_s, window_id);

		if (!obj->next_stream)
		  return (NULL);

		/* When uudecoding, we tend to come up with tiny chunks of data
		   at a time.  Make a stream to put them back together, so that
		   we hand bigger pieces to the image library.
		 */
		{
		  NET_StreamClass *buffer =
			msg_MakeRebufferingStream (obj->next_stream, URL_s, window_id);
		  if (buffer)
			obj->next_stream = buffer;
		}

		TRACEMSG(("Returning stream from NET_MimeEncodingConverter\n"));
	  }

    return stream;
}


PRIVATE NET_StreamClass * 
NET_MimeEncodingConverter (int          format_out,
						   void        *data_obj,
						   URL_Struct  *URL_s,
						   MWContext   *window_id)
{
  return NET_MimeEncodingConverter_1 (format_out, data_obj, URL_s, window_id,
									  FALSE, 0);
}

NET_StreamClass * 
NET_MimeMakePartialEncodingConverterStream (int          format_out,
											void        *data_obj,
											URL_Struct  *URL_s,
											MWContext   *window_id,
											NET_StreamClass *next_stream)
{
  return NET_MimeEncodingConverter_1 (format_out, data_obj, URL_s, window_id,
									  TRUE, next_stream);
}

/* Registers the default things that should be registered cross-platform.
   Really this doesn't belong in this file.
 */


#ifdef NEW_DECODERS
extern void NET_RegisterAllEncodingConverters (char *format_in,
											   FO_Present_Types format_out);
#endif /* NEW_DECODERS */


#ifdef EDITOR
NET_StreamClass *
EDT_ErrorOut (int format_out,
			  void *data_obj,	
			  URL_Struct *URL_s,
			  MWContext  *window_id)
{

 	FE_Alert(window_id, XP_GetString(XP_EDITOR_NON_HTML));

	return(NULL);
}
#endif /* EDITOR */
		

PRIVATE void
net_RegisterDefaultDecoders (void)
{
  static PA_InitData parser_data;

#ifdef XP_UNIX
# ifndef NEW_DECODERS
  NET_ClearExternalViewerConverters ();
# endif /* !NEW_DECODERS */
#endif /* XP_UNIX */

  /* for the parser/layout functionality
   */
#ifdef EDITOR
  parser_data.output_func = EDT_ProcessTag;
#else
  parser_data.output_func = LO_ProcessTag;
#endif

  /* Convert the charsets of HTML into the canonical internal form.
   */
  NET_RegisterContentTypeConverter (TEXT_HTML, FO_PRESENT,
									NULL, INTL_ConvCharCode);

  /* send all HTML to the editor, everything else as error
   */
  NET_RegisterContentTypeConverter (TEXT_HTML, FO_EDIT,
  									NULL, INTL_ConvCharCode);

  /* send file listings to html converter */
  NET_RegisterContentTypeConverter (APPLICATION_HTTP_INDEX, FO_PRESENT,
  									NULL, NET_HTTPIndexFormatToHTMLConverter);

#ifdef EDITOR
  NET_RegisterContentTypeConverter ("*", FO_EDIT,
  									NULL, EDT_ErrorOut);
#endif /* EDITOR */

  NET_RegisterContentTypeConverter("*", FO_LOAD_HTML_HELP_MAP_FILE,
								   NULL, NET_HTMLHelpMapToURL);

	/* this should be windows and mac soon too... */
#ifdef XP_UNIX
	/* the view source converter */
  NET_RegisterContentTypeConverter ("*", FO_VIEW_SOURCE,
									TEXT_PLAIN, net_ColorHTMLStream);
  NET_RegisterContentTypeConverter (TEXT_HTML, FO_VIEW_SOURCE,
									NULL, INTL_ConvCharCode);
  NET_RegisterContentTypeConverter (MESSAGE_RFC822, FO_VIEW_SOURCE,
									NULL, INTL_ConvCharCode);
  NET_RegisterContentTypeConverter (MESSAGE_NEWS, FO_VIEW_SOURCE,
									NULL, INTL_ConvCharCode);
  NET_RegisterContentTypeConverter (INTERNAL_PARSER, FO_VIEW_SOURCE,
									TEXT_HTML, net_ColorHTMLStream);
  NET_RegisterContentTypeConverter (APPLICATION_HTTP_INDEX, FO_VIEW_SOURCE,
  									NULL, NET_HTTPIndexFormatToHTMLConverter);
#endif /* XP_UNIX */

  NET_RegisterContentTypeConverter (TEXT_HTML, FO_SAVE_AS_TEXT,
								    NULL, INTL_ConvCharCode);
  NET_RegisterContentTypeConverter (TEXT_HTML, FO_QUOTE_MESSAGE,
								    NULL, INTL_ConvCharCode);
  NET_RegisterContentTypeConverter (APPLICATION_HTTP_INDEX, FO_QUOTE_MESSAGE,
  									NULL, NET_HTTPIndexFormatToHTMLConverter);
  NET_RegisterContentTypeConverter (APPLICATION_HTTP_INDEX, FO_SAVE_AS_TEXT,
  									NULL, NET_HTTPIndexFormatToHTMLConverter);

#ifdef XP_UNIX
  NET_RegisterContentTypeConverter (TEXT_HTML, FO_SAVE_AS_POSTSCRIPT,
									NULL, INTL_ConvCharCode);
  NET_RegisterContentTypeConverter (APPLICATION_HTTP_INDEX,
 									FO_SAVE_AS_POSTSCRIPT,
  									NULL, NET_HTTPIndexFormatToHTMLConverter);
#endif /* XP_UNIX */

  /* And MDL too, sigh. */
  NET_RegisterContentTypeConverter (TEXT_MDL, FO_PRESENT,
									NULL, INTL_ConvCharCode);
  NET_RegisterContentTypeConverter (TEXT_MDL, FO_SAVE_AS_TEXT,
								    NULL, INTL_ConvCharCode);
  NET_RegisterContentTypeConverter (TEXT_MDL, FO_QUOTE_MESSAGE,
								    NULL, INTL_ConvCharCode);
#ifdef XP_UNIX
  NET_RegisterContentTypeConverter (TEXT_MDL, FO_SAVE_AS_POSTSCRIPT,
								    NULL, INTL_ConvCharCode);
#endif /* XP_UNIX */

  /* Convert the charsets of plain text into the canonical internal form.
   */
  NET_RegisterContentTypeConverter (TEXT_PLAIN, FO_PRESENT,
									NULL, NET_PlainTextConverter);
  NET_RegisterContentTypeConverter (TEXT_PLAIN, FO_QUOTE_MESSAGE,
								    NULL, NET_PlainTextConverter);
	/* don't register TEXT_PLAIN for FO_SAVE_AS_TEXT
     * since it is already text
	 */
#ifdef XP_UNIX
  NET_RegisterContentTypeConverter (TEXT_PLAIN, FO_SAVE_AS_POSTSCRIPT,
									NULL, NET_PlainTextConverter);
#endif /* XP_UNIX */

  /* always treat unknown content types as text/plain */
  NET_RegisterContentTypeConverter (UNKNOWN_CONTENT_TYPE, FO_PRESENT,
									NULL, NET_PlainTextConverter);
  NET_RegisterContentTypeConverter (UNKNOWN_CONTENT_TYPE, FO_QUOTE_MESSAGE,
								    NULL, NET_PlainTextConverter);

  /* let mail view forms sent via web browsers */
  NET_RegisterContentTypeConverter (APPLICATION_WWW_FORM_URLENCODED, 
									FO_PRESENT,
									NULL, 
									NET_PlainTextConverter);

#if defined(XP_MAC) || defined(XP_UNIX) || defined(XP_WIN)
  /* the new apple single/double and binhex decode.	20oct95 	*/
  NET_RegisterContentTypeConverter (APPLICATION_BINHEX, FO_PRESENT,
								    NULL, fe_MakeBinHexDecodeStream);
  NET_RegisterContentTypeConverter (MULTIPART_APPLEDOUBLE, FO_PRESENT,
								    NULL, fe_MakeAppleDoubleDecodeStream_1);
  NET_RegisterContentTypeConverter (MULTIPART_HEADER_SET, FO_PRESENT,
								    NULL, fe_MakeAppleDoubleDecodeStream_1);
  NET_RegisterContentTypeConverter (APPLICATION_APPLEFILE, FO_PRESENT,
								    NULL, fe_MakeAppleSingleDecodeStream_1);
#endif /* XP_MAC || XP_UNIX */

	/* don't register UNKNOWN_CONTENT_TYPE for FO_SAVE_AS_TEXT
     * since it is already text
	 */
#ifdef XP_UNIX
  NET_RegisterContentTypeConverter (UNKNOWN_CONTENT_TYPE,
									FO_SAVE_AS_POSTSCRIPT,
									NULL, NET_PlainTextConverter);
#endif /* XP_UNIX */


  /* Take the canonical internal form and do layout on it.
	 We do the same thing when the format_out is PRESENT or any of
	 the SAVE_AS types; the different behavior is gotten by the use
	 of a different context rather than a different stream.
   */
  NET_RegisterContentTypeConverter (INTERNAL_PARSER, FO_PRESENT,
									(void *) &parser_data, PA_BeginParseMDL);
  NET_RegisterContentTypeConverter (INTERNAL_PARSER, FO_SAVE_AS_TEXT,
									(void *) &parser_data, PA_BeginParseMDL);
  NET_RegisterContentTypeConverter (INTERNAL_PARSER, FO_QUOTE_MESSAGE,
									(void *) &parser_data, PA_BeginParseMDL);
#ifdef XP_UNIX
  NET_RegisterContentTypeConverter (INTERNAL_PARSER, FO_SAVE_AS_POSTSCRIPT,
									(void *) &parser_data, PA_BeginParseMDL);
#endif /* XP_UNIX */

  /* one for the editor */
  NET_RegisterContentTypeConverter (INTERNAL_PARSER, FO_EDIT,
									(void *) &parser_data, PA_BeginParseMDL);

  /* Note that we don't register a converter for "*" to FO_SAVE_AS,
	 because the FE needs to do that specially to set up an output file.
	 (The file I/O stuff for SAVE_AS_TEXT and SAVE_AS_POSTSCRIPT is dealt
	 with by libxlate.a, which could also handle SAVE_AS, but it's probably
	 not worth the effort.)
   */


  /* Do the same for the internally-handled image types when the format_out
	 is SAVE_AS_POSTSCRIPT, because the TEXT->PS code can handle that.  But
	 do not register converters to feed the image types to the HTML->TEXT
	 code, because that doesn't work.
   */
#ifdef XP_UNIX
  NET_RegisterContentTypeConverter (IMAGE_GIF, FO_SAVE_AS_POSTSCRIPT,
									NULL, IL_ViewStream);
  NET_RegisterContentTypeConverter (IMAGE_JPG, FO_SAVE_AS_POSTSCRIPT,
									NULL, IL_ViewStream);
  NET_RegisterContentTypeConverter (IMAGE_PJPG, FO_SAVE_AS_POSTSCRIPT,
									NULL, IL_ViewStream);
  NET_RegisterContentTypeConverter (IMAGE_XBM, FO_SAVE_AS_POSTSCRIPT,
									NULL, IL_ViewStream);
  NET_RegisterContentTypeConverter (IMAGE_XBM2, FO_SAVE_AS_POSTSCRIPT,
									NULL, IL_ViewStream);
  NET_RegisterContentTypeConverter (IMAGE_XBM3, FO_SAVE_AS_POSTSCRIPT,
									NULL, IL_ViewStream);
#endif /* XP_UNIX */

#if 0 /* no! */
  NET_RegisterContentTypeConverter (IMAGE_GIF, FO_SAVE_AS_TEXT,
									NULL, IL_ViewStream);
  NET_RegisterContentTypeConverter (IMAGE_JPG, FO_SAVE_AS_TEXT,
									NULL, IL_ViewStream);
  NET_RegisterContentTypeConverter (IMAGE_PJPG, FO_SAVE_AS_TEXT,
									NULL, IL_ViewStream);
  NET_RegisterContentTypeConverter (IMAGE_XBM, FO_SAVE_AS_TEXT,
									NULL, IL_ViewStream);
  NET_RegisterContentTypeConverter (IMAGE_XBM2, FO_SAVE_AS_TEXT,
									NULL, IL_ViewStream);
  NET_RegisterContentTypeConverter (IMAGE_XBM3, FO_SAVE_AS_TEXT,
									NULL, IL_ViewStream);
#endif

  /* Set things up so that the image library gets reconnected once the
	 internally-handled images have been started. */
  NET_RegisterContentTypeConverter (IMAGE_GIF, FO_INTERNAL_IMAGE,
									(void *) IL_GIF, IL_NewStream);
  NET_RegisterContentTypeConverter (IMAGE_JPG, FO_INTERNAL_IMAGE,
									(void *) IL_JPEG, IL_NewStream);
  NET_RegisterContentTypeConverter (IMAGE_PJPG, FO_INTERNAL_IMAGE,
									(void *) IL_JPEG, IL_NewStream);
  NET_RegisterContentTypeConverter (IMAGE_XBM, FO_INTERNAL_IMAGE,
									(void *) IL_XBM, IL_NewStream);
  NET_RegisterContentTypeConverter (IMAGE_XBM2, FO_INTERNAL_IMAGE,
									(void *) IL_XBM, IL_NewStream);
  NET_RegisterContentTypeConverter (IMAGE_XBM3, FO_INTERNAL_IMAGE,
									(void *) IL_XBM, IL_NewStream);
  NET_RegisterContentTypeConverter ("*", FO_INTERNAL_IMAGE,
									(void *) IL_UNKNOWN, IL_NewStream);

  NET_RegisterContentTypeConverter (IMAGE_GIF, FO_PRESENT,NULL, IL_ViewStream);
  NET_RegisterContentTypeConverter (IMAGE_JPG, FO_PRESENT,NULL, IL_ViewStream);
  NET_RegisterContentTypeConverter (IMAGE_PJPG,FO_PRESENT,NULL, IL_ViewStream);
  NET_RegisterContentTypeConverter (IMAGE_XBM, FO_PRESENT,NULL, IL_ViewStream);
  NET_RegisterContentTypeConverter (IMAGE_XBM2,FO_PRESENT,NULL, IL_ViewStream);
  NET_RegisterContentTypeConverter (IMAGE_XBM3,FO_PRESENT,NULL, IL_ViewStream);

  /* register default (non)decoders for the text printer
   */
  NET_RegisterContentTypeConverter ("*", FO_SAVE_AS_TEXT,
                                    NULL, NET_PrintRawToDisk);
  NET_RegisterContentTypeConverter ("*", FO_QUOTE_MESSAGE,
                                    NULL, NET_PrintRawToDisk);
#ifdef XP_UNIX
  NET_RegisterContentTypeConverter ("*", FO_SAVE_AS_POSTSCRIPT,
                                    NULL, NET_PrintRawToDisk);
#endif /* XP_UNIX */

  /* cache things */
  NET_RegisterContentTypeConverter ("*", FO_CACHE_AND_PRESENT,
									NULL, NET_CacheConverter);

  NET_RegisterContentTypeConverter ("*", FO_CACHE_ONLY,
									NULL, NET_CacheConverter);

  NET_RegisterContentTypeConverter ("*", FO_CACHE_AND_INTERNAL_IMAGE,
									NULL, NET_CacheConverter);

  NET_RegisterContentTypeConverter ("*", FO_CACHE_AND_SAVE_AS,
									NULL, NET_CacheConverter);
  NET_RegisterContentTypeConverter ("*", FO_CACHE_AND_SAVE_AS_TEXT,
									NULL, NET_CacheConverter);
#ifdef XP_UNIX
  NET_RegisterContentTypeConverter ("*", FO_CACHE_AND_SAVE_AS_POSTSCRIPT,
									NULL, NET_CacheConverter);
#endif /* XP_UNIX */
  NET_RegisterContentTypeConverter ("*", FO_CACHE_AND_QUOTE_MESSAGE,
									NULL, NET_CacheConverter);
  NET_RegisterContentTypeConverter ("*", FO_CACHE_AND_VIEW_SOURCE,
									NULL, NET_CacheConverter);
  NET_RegisterContentTypeConverter ("*", FO_CACHE_AND_MAIL_TO,
									NULL, NET_CacheConverter);
  NET_RegisterContentTypeConverter ("*", FO_CACHE_AND_EDIT,
  									NULL, NET_CacheConverter);
  NET_RegisterContentTypeConverter("*", FO_CACHE_AND_LOAD_HTML_HELP_MAP_FILE,
								   NULL, NET_CacheConverter);

#ifndef MCC_PROXY
  NET_RegisterContentTypeConverter(APPLICATION_NS_PROXY_AUTOCONFIG,
								   FO_CACHE_AND_PRESENT,
								   (void *)0, NET_ProxyAutoConfig);
  NET_RegisterContentTypeConverter(APPLICATION_NS_PROXY_AUTOCONFIG,
								   FO_PRESENT,
								   (void *)0, NET_ProxyAutoConfig);
#endif

#ifdef HAVE_SECURITY /* added by jwz */
  /* security related content type converters */
  NET_RegisterContentTypeConverter (APPLICATION_X509_CA_CERT, FO_PRESENT,
									(void *)SEC_CERT_CLASS_CA,
									SECNAV_CertificateStream);
  NET_RegisterContentTypeConverter (APPLICATION_X509_SERVER_CERT, FO_PRESENT,
									(void *)SEC_CERT_CLASS_SERVER,
									SECNAV_CertificateStream);
  NET_RegisterContentTypeConverter (APPLICATION_X509_USER_CERT, FO_PRESENT,
									(void *)SEC_CERT_CLASS_USER,
									SECNAV_CertificateStream);
  NET_RegisterContentTypeConverter (APPLICATION_NETSCAPE_REVOCATION,FO_PRESENT,
									(void *)0, SECNAV_RevocationStream);

  NET_RegisterContentTypeConverter (APPLICATION_X509_CRL,FO_PRESENT,
									(void *)SEC_CRL_TYPE, SECNAV_CrlStream);
#ifdef FORTEZZA
  NET_RegisterContentTypeConverter (APPLICATION_FORTEZZA_KRL,FO_PRESENT,
									(void *)SEC_KRL_TYPE, SECNAV_CrlStream);
  /* pre-encrypted fortezza converters */
  NET_RegisterContentTypeConverter (INTERNAL_PRE_ENCRYPTED, FO_PRESENT,
									(void *)0, SECNAV_MakePreencryptedStream);
  NET_RegisterContentTypeConverter (INTERNAL_PRE_ENCRYPTED, FO_INTERNAL_IMAGE,
									(void *)0, SECNAV_MakePreencryptedStream);
  NET_RegisterContentTypeConverter (INTERNAL_PRE_ENCRYPTED, FO_VIEW_SOURCE,
									(void *)0, SECNAV_MakePreencryptedStream);
  NET_RegisterContentTypeConverter (APPLICATION_PRE_ENCRYPTED, FO_PRESENT,
									(void *)0, SECNAV_MakePreencryptedStream);
  NET_RegisterContentTypeConverter (APPLICATION_PRE_ENCRYPTED, FO_INTERNAL_IMAGE,
									(void *)0, SECNAV_MakePreencryptedStream);
  NET_RegisterContentTypeConverter (APPLICATION_PRE_ENCRYPTED, FO_VIEW_SOURCE,
									(void *)0, SECNAV_MakePreencryptedStream);
  NET_RegisterContentTypeConverter (APPLICATION_PRE_ENCRYPTED, FO_CACHE_AND_PRESENT,
									(void *)0, SECNAV_MakePreencryptedStream);
  NET_RegisterContentTypeConverter (APPLICATION_PRE_ENCRYPTED, FO_CACHE_AND_INTERNAL_IMAGE,
									(void *)0, SECNAV_MakePreencryptedStream);
  NET_RegisterContentTypeConverter (APPLICATION_PRE_ENCRYPTED, FO_CACHE_AND_VIEW_SOURCE,
									(void *)0, SECNAV_MakePreencryptedStream);
  NET_RegisterContentTypeConverter (APPLICATION_PRE_ENCRYPTED, FO_SAVE_AS,
									(void *)0, SECNAV_MakePreencryptedStream);
  NET_RegisterContentTypeConverter (APPLICATION_PRE_ENCRYPTED, FO_CACHE_AND_SAVE_AS,
									(void *)0, SECNAV_MakePreencryptedStream);
  NET_RegisterContentTypeConverter (INTERNAL_PRE_ENCRYPTED, FO_SAVE_AS,
									(void *)0, SECNAV_MakePreencryptedStream);
  NET_RegisterContentTypeConverter (INTERNAL_PRE_ENCRYPTED, FO_CACHE_AND_SAVE_AS,
									(void *)0, SECNAV_MakePreencryptedStream);
#endif
#endif /* !HAVE_SECURITY -- added by jwz */
#ifdef JAVA
  {		/* stuff from ns/sun-java/netscape/net/netStubs.c */
	  extern void  NSN_RegisterJavaConverter(void);
	  NSN_RegisterJavaConverter();
  }
#endif /* JAVA */
}


#ifdef NEW_DECODERS
PRIVATE void
net_RegisterDefaultEncodingDecoders (void)
{
  /* Register the decompression content-encoding converters for those
	 types which are displayed internally. */
  NET_RegisterAllEncodingConverters (INTERNAL_PARSER, FO_PRESENT);
  NET_RegisterAllEncodingConverters (TEXT_HTML,       FO_PRESENT);
  NET_RegisterAllEncodingConverters (TEXT_MDL,        FO_PRESENT);
  NET_RegisterAllEncodingConverters (TEXT_PLAIN,      FO_PRESENT);
  NET_RegisterAllEncodingConverters (IMAGE_GIF,       FO_PRESENT);
  NET_RegisterAllEncodingConverters (IMAGE_JPG,       FO_PRESENT);
  NET_RegisterAllEncodingConverters (IMAGE_PJPG,      FO_PRESENT);
  NET_RegisterAllEncodingConverters (IMAGE_XBM,       FO_PRESENT);
  NET_RegisterAllEncodingConverters (IMAGE_XBM2,      FO_PRESENT);
  NET_RegisterAllEncodingConverters (IMAGE_XBM3,      FO_PRESENT);
  /* always treat unknown content types as text/plain */
  NET_RegisterAllEncodingConverters (UNKNOWN_CONTENT_TYPE, FO_PRESENT);

  /* when displaying anything in a news mime-multipart image
   * de compress it first
   */
  NET_RegisterAllEncodingConverters ("*",  FO_MULTIPART_IMAGE);

  /* When saving anything as Text or PostScript, it's necessary to
	 uncompress it first. */
  NET_RegisterAllEncodingConverters ("*",  FO_SAVE_AS_TEXT);
# ifdef XP_UNIX
  NET_RegisterAllEncodingConverters ("*",  FO_SAVE_AS_POSTSCRIPT);
# endif /* XP_UNIX */

  /* Whenever we save text or HTML to disk, we uncompress it first,
	 because the rules we decided upon is:

    - When Netscape saves text/plain or text/html documents to a
      user-specified file, they are always uncompressed first.

    - When Netscape saves any other kind of document to a user-specified
      file, it is always saved in its compressed state.

    - When Netscape saves a document to a temporary file before handing
      it off to an external viewer, it is always uncompressed first.
   */
  NET_RegisterAllEncodingConverters (TEXT_HTML,       FO_SAVE_AS);
  NET_RegisterAllEncodingConverters (TEXT_MDL,        FO_SAVE_AS);
  NET_RegisterAllEncodingConverters (TEXT_PLAIN,      FO_SAVE_AS);
  NET_RegisterAllEncodingConverters (INTERNAL_PARSER, FO_SAVE_AS);
  /* always treat unknown content types as text/plain */
  NET_RegisterAllEncodingConverters (UNKNOWN_CONTENT_TYPE, FO_SAVE_AS);
}
#endif /* NEW_DECODERS */


PUBLIC void
NET_RegisterMIMEDecoders (void)
{
  net_RegisterDefaultDecoders ();

  NET_RegisterEncodingConverter (ENCODING_QUOTED_PRINTABLE,
								 (void *) ENCODING_QUOTED_PRINTABLE,
								 NET_MimeEncodingConverter);
  NET_RegisterEncodingConverter (ENCODING_BASE64,
								 (void *) ENCODING_BASE64,
								 NET_MimeEncodingConverter);
  NET_RegisterEncodingConverter (ENCODING_UUENCODE,
								 (void *) ENCODING_UUENCODE,
								 NET_MimeEncodingConverter);
  NET_RegisterEncodingConverter (ENCODING_UUENCODE2,
								 (void *) ENCODING_UUENCODE2,
								 NET_MimeEncodingConverter);
  NET_RegisterEncodingConverter (ENCODING_UUENCODE3,
								 (void *) ENCODING_UUENCODE3,
								 NET_MimeEncodingConverter);
  NET_RegisterEncodingConverter (ENCODING_UUENCODE4,
								 (void *) ENCODING_UUENCODE4,
								 NET_MimeEncodingConverter);

  NET_RegisterContentTypeConverter ("*", FO_MULTIPART_IMAGE,
									(void *) 1, IL_ViewStream);

  /* Decoders for libmsg/compose.c */
  MSG_RegisterConverters ();

  NET_RegisterContentTypeConverter("multipart/x-mixed-replace", 
								 FO_CACHE_AND_PRESENT, 
                                 (void *) CVACTIVE_SIGNAL_AT_END_OF_MULTIPART, 
                                 CV_MakeMultipleDocumentStream);
  NET_RegisterContentTypeConverter("multipart/x-mixed-replace", 
								 FO_CACHE_AND_INTERNAL_IMAGE, 
                                 (void *) CVACTIVE_SIGNAL_AT_END_OF_MULTIPART, 
                                 CV_MakeMultipleDocumentStream);
  NET_RegisterContentTypeConverter("multipart/x-byteranges", 
								 FO_CACHE_AND_PRESENT, 
                                 (void *) CVACTIVE_SIGNAL_AT_END_OF_MULTIPART, 
                                 CV_MakeMultipleDocumentStream);
  NET_RegisterContentTypeConverter("multipart/mixed", 
						 		 FO_CACHE_AND_PRESENT, 
						 		 (void *) CVACTIVE_SIGNAL_AT_END_OF_MULTIPART, 
						 		 CV_MakeMultipleDocumentStream);

#ifdef NEW_DECODERS
  net_RegisterDefaultEncodingDecoders ();
#endif /* NEW_DECODERS */
}


#if 0
void
main ()
{
  unsigned char *s;
  int32 i;
#define TEST(S) \
  /*MIME_EncodeBase64String*/ \
  MIME_EncodeQuotedPrintableString \
   (S, strlen(S), &s, &i); \
  printf ("---------\n" \
		  "%s\n" \
		  "---------\n" \
		  "%s\n" \
		  "---------\n", \
		  S, s); \
  free(s)

  TEST("ThisThisThisThis\n"
	   "This is a line with a c\238ntr\001l character in it.\n"
	   "This line has whitespace at EOL.    \n"
	   "This is a long line.  All work and no play makes Jack a dull boy.  "
	   "All work and no play makes Jack a dull boy.  "
	   "All work and no play makes Jack a dull boy.  "
	   "All work and no play makes Jack a dull boy.  \n"
	   "\n"
	   "Here's an equal sign: =\n"
	   "and here's a return\r..."
	   "Now make it realloc:"
	   "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
	   "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
	   "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
	   "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
	   "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
	   "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
	   "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
	   "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
	   "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
	   "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
	   "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
	   "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
	   "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
	   "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
	   );
}
#endif

typedef struct _NET_PrintRawToDiskStruct {
    XP_File    fp;
    MWContext *context;
    int32      content_length;
    int32      bytes_read;
} NET_PrintRawToDiskStruct;

/* this converter uses the
 * (context->prSetup->filename) variable to
 * open the filename specified and save all data raw to
 * the file.
 */
int
net_PrintRawToDiskWrite (void *vobj, CONST char *str, int32 len)
{
  NET_PrintRawToDiskStruct *obj = (NET_PrintRawToDiskStruct *) vobj;
/*  int32 rv = fwrite ((char *) str, 1, len, obj->fp);*/
  int32 rv = XP_FileWrite(str, len, obj->fp);
  obj->bytes_read += len;

  if (obj->content_length > 0)
    FE_SetProgressBarPercent (obj->context,
                  (obj->bytes_read * 100) /
                  obj->content_length);

  if(rv == len)
    return(0);
  else
    return(-1);
}

PRIVATE unsigned int
net_PrintRawToDiskIsWriteReady (void *vobj)
{
  return(MAX_WRITE_READY);
}

PRIVATE void
net_PrintRawToDiskComplete (void *vobj)
{
/*  NET_PrintRawToDiskStruct *obj = (NET_PrintRawToDiskStruct *) vobj;*/

  /* XP_FileClose(obj->fp); DONT DO THIS the FE's do this */
}

PRIVATE void
net_PrintRawToDiskAbort (void *vobj, int status)
{
/*  NET_PrintRawToDiskStruct *obj = (NET_PrintRawToDiskStruct *) vobj; */

  /* XP_FileClose(obj->fp); DONT DO THIS the FE's do this */
}

/* this converter uses the
 * (context->prSetup->out) file pointer variable to
 * save all data raw to the file.
 */
PUBLIC NET_StreamClass *
NET_PrintRawToDisk(int format_out, 
				   void *data_obj,
                   URL_Struct *url_struct, 
				   MWContext *context)
{
  NET_PrintRawToDiskStruct *obj = XP_NEW(NET_PrintRawToDiskStruct);
  NET_StreamClass * stream;

  if(!obj)
	return(NULL);

  XP_MEMSET(obj, 0, sizeof(NET_PrintRawToDiskStruct));

  if(!context->prSetup || !context->prSetup->out)
	{
	  FREE(obj);
	  return NULL;
	}

#ifdef NOT /* file ptr should already be open in context->prSetup->out */
  if(!(obj->fp = XP_FileOpen(context->prSetup->filename, 
						xpTemporary, 
						XP_FILE_WRITE_BIN)))
	{
	  FREE(obj);
	  return NULL;
	}
#endif

  obj->fp = context->prSetup->out;

  obj->context = context;
  obj->content_length = url_struct->content_length;

  stream = (NET_StreamClass *) XP_NEW(NET_StreamClass);
  if (!stream) 
	{
	  FREE(obj);
	  return NULL;
	}

  XP_MEMSET(stream, 0, sizeof(NET_StreamClass));

  stream->name           = "PrintRawToDisk";
  stream->complete       = net_PrintRawToDiskComplete;
  stream->abort          = net_PrintRawToDiskAbort;
  stream->put_block      = net_PrintRawToDiskWrite;
  stream->is_write_ready = net_PrintRawToDiskIsWriteReady;
  stream->data_object    = obj;
  stream->window_id      = context;

  return(stream);
}

#endif /*  MOZILLA_CLIENT */
