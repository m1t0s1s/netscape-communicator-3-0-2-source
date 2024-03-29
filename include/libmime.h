/* -*- Mode: C; tab-width: 4 -*-
   libmime.h --- external interface to libmime.a
   Copyright � 1997 Netscape Communications Corporation, all rights reserved.
   Created: Jamie Zawinski <jwz@netscape.com>, 15-May-96.
 */

#ifndef _LIBMIME_H_
#define _LIBMIME_H_

#include "xp.h"

#ifndef AKBAR
# define MIME_DRAFTS
#endif

/* Opaque object describing a block of message headers, and a couple of
   routines for extracting data from one.
 */
typedef struct MimeHeaders MimeHeaders;
typedef struct MimeDisplayOptions MimeDisplayOptions;
typedef struct MimeParseStateObject MimeParseStateObject;
#ifndef AKBAR
typedef struct MSG_AttachmentData MSG_AttachmentData;
#endif

XP_BEGIN_PROTOS

/* Given the name of a header, returns the contents of that header as
   a newly-allocated string (which the caller must free.)  If the header
   is not present, or has no contents, NULL is returned.

   If `strip_p' is TRUE, then the data returned will be the first token
   of the header; else it will be the full text of the header.  (This is
   useful for getting just "text/plain" from "text/plain; name=foo".)

   If `all_p' is FALSE, then the first header encountered is used, and
   any subsequent headers of the same name are ignored.  If TRUE, then
   all headers of the same name are appended together (this is useful
   for gathering up all CC headers into one, for example.)
 */
extern char *MimeHeaders_get(MimeHeaders *hdrs,
							 const char *header_name,
							 XP_Bool strip_p,
							 XP_Bool all_p);

/* Given a header of the form of the MIME "Content-" headers, extracts a
   named parameter from it, if it exists.  For example,
   MimeHeaders_get_parameter("text/plain; charset=us-ascii", "charset")
   would return "us-ascii".

   Returns NULL if there is no match, or if there is an allocation failure.
 */
extern char *MimeHeaders_get_parameter (const char *header_value,
										const char *parm_name);


/* Some defines for various standard header field names.
 */
#define HEADER_BCC							"BCC"
#define HEADER_CC							"CC"
#define HEADER_CONTENT_BASE					"Content-Base"
#define HEADER_CONTENT_DESCRIPTION			"Content-Description"
#define HEADER_CONTENT_DISPOSITION			"Content-Disposition"
#define HEADER_CONTENT_ENCODING				"Content-Encoding"
#define HEADER_CONTENT_LENGTH				"Content-Length"
#define HEADER_CONTENT_NAME					"Content-Name"
#define HEADER_CONTENT_TRANSFER_ENCODING	"Content-Transfer-Encoding"
#define HEADER_CONTENT_TYPE					"Content-Type"
#define HEADER_DATE							"Date"
#define HEADER_DISTRIBUTION					"Distribution"
#define HEADER_FCC							"FCC"
#define HEADER_FOLLOWUP_TO					"Followup-To"
#define HEADER_FROM							"From"
#define HEADER_LINES						"Lines"
#define HEADER_MESSAGE_ID					"Message-ID"
#define HEADER_SUPERSEDES					"Supersedes"
#define HEADER_MIME_VERSION					"MIME-Version"
#define HEADER_NEWSGROUPS					"Newsgroups"
#define HEADER_ORGANIZATION					"Organization"
#define HEADER_REFERENCES					"References"
#define HEADER_REPLY_TO						"Reply-To"
#define HEADER_RESENT_COMMENTS				"Resent-Comments"
#define HEADER_RESENT_DATE					"Resent-Date"
#define HEADER_RESENT_FROM					"Resent-From"
#define HEADER_RESENT_MESSAGE_ID			"Resent-Message-ID"
#define HEADER_RESENT_SENDER				"Resent-Sender"
#define HEADER_RESENT_TO					"Resent-To"
#define HEADER_RESENT_CC					"Resent-CC"
#define HEADER_SENDER						"Sender"
#define HEADER_SUBJECT						"Subject"
#define HEADER_TO							"To"
#define HEADER_X_MAILER						"X-Mailer"
#define HEADER_X_NEWSREADER					"X-Newsreader"
#define HEADER_X_POSTING_SOFTWARE			"X-Posting-Software"
#define HEADER_X_MOZILLA_STATUS	 			"X-Mozilla-Status"
#define HEADER_X_MOZILLA_NEWSHOST			"X-Mozilla-News-Host"
#define HEADER_X_MOZILLA_DRAFT_INFO			"X-Mozilla-Draft-Info"
#define HEADER_X_UIDL						"X-UIDL"

/* DEBUG_jwz */
#define HEADER_X_DIAGNOSTIC					"X-Diagnostic"
#define HEADER_X_COMMAND					"X-Command"
#define HEADER_X_PROCESSED					"X-Processed"

#define HEADER_XREF							"XREF"
#define HEADER_X_SUN_CHARSET				"X-Sun-Charset"
#define HEADER_X_SUN_CONTENT_LENGTH			"X-Sun-Content-Length"
#define HEADER_X_SUN_CONTENT_LINES			"X-Sun-Content-Lines"
#define HEADER_X_SUN_DATA_DESCRIPTION		"X-Sun-Data-Description"
#define HEADER_X_SUN_DATA_NAME				"X-Sun-Data-Name"
#define HEADER_X_SUN_DATA_TYPE				"X-Sun-Data-Type"
#define HEADER_X_SUN_ENCODING_INFO			"X-Sun-Encoding-Info"
#define HEADER_X_PRIORITY                   "X-Priority"

#define HEADER_PARM_BOUNDARY				"BOUNDARY"
#define HEADER_PARM_FILENAME				"FILENAME"
#define HEADER_PARM_NAME					"NAME"
#define HEADER_PARM_TYPE					"TYPE"


typedef enum {
  MimeHeadersAll,				/* Show all headers */
  MimeHeadersSome,				/* Show all "interesting" headers */
  MimeHeadersSomeNoRef,			/* Same, but suppress the `References' header
								   (for when we're printing this message.) */
  MimeHeadersMicro,				/* Show a one-line header summary */
  MimeHeadersMicroPlus,			/* Same, but show the full recipient list as
								   well (To, CC, etc.) */
  MimeHeadersCitation			/* A one-line summary geared toward use in a
								   reply citation ("So-and-so wrote:") */
} MimeHeadersState;


/* The signature for various callbacks in the MimeDisplayOptions structure.
 */
typedef char *(*MimeHTMLGeneratorFunction) (const char *data, void *closure,
											MimeHeaders *headers);

struct MimeDisplayOptions
{
  const char *url;			/* Base URL for the document.  This string should
							   be freed by the caller, after the parser
							   completes (possibly at the same time as the
							   MimeDisplayOptions itself.) */

#ifndef AKBAR
  MSG_Pane* pane;				/* The libmsg pane object that corresponds to
								   what we're showing.  This is used by very
								   little... */
#endif /* !AKBAR */

  MimeHeadersState headers;	/* How headers should be displayed. */
  XP_Bool fancy_headers_p;	/* Whether to do clever formatting of headers
							   using tables, instead of spaces. */

#ifndef AKBAR
  XP_Bool output_vcard_buttons_p;	/* Whether to output the buttons */
									/* on vcards. */
#endif /* !AKBAR */

  XP_Bool fancy_links_p;		/* Whether to insert fancy links, so you can
								   do things like click on an email address to
								   add it to your address book.  Something you
								   don't want to do while printing. */

  XP_Bool variable_width_plaintext_p;	/* Whether text/plain messages should
										   be in variable width, or fixed. */
  XP_Bool wrap_long_lines_p;	/* Whether to wrap long lines in text/plain
								   messages. */

  XP_Bool rot13_p;			/* Whether text/plain parts should be rotated
							   Set by "?rot13=true" */
  XP_Bool no_inline_p;		/* Whether inline display of attachments should
							   be suppressed.  Set by "?inline=false" */
  char *part_to_load;		/* The particular part of the multipart which
							   we are extracting.  Set by "?part=3.2.4" */

  XP_Bool write_html_p;		/* Whether the output should be HTML, or raw.
							   (Only used when compiled with MIME_TO_HTML.) */

  XP_Bool write_leaf_p;		/* Whether to write out the raw text of all
							   leaf nodes, flattened.  (Only used when
							   compiled with MIME_LEAF_TEXT.) */

  XP_Bool decrypt_p;		/* Whether all traces of encryption should be
							   eradicated -- this is only meaningful when
							   write_html_p is FALSE; we set this when
							   attaching a message for forwarding, since
							   forwarding someone else a message that wasn't
							   encrypted for them doesn't work.  We have to
							   decrypt it before sending it.  (Only used when
							   compiled with MIME_SECURITY.)
							 */

#ifndef AKBAR
  XP_Bool nice_html_only_p;		/* If TRUE, then we only should write html if
								   it's pretty HTML (stuff that we're willing
								   to get shipped out in mail messages).  If we
								   can't generate nice stuff for some part,
								   then don't say anything at all.  (Only used
								   when compiled with MIME_TO_HTML.) */

  XP_Bool dont_touch_citations_p; /* If TRUE, then we should leave citations
									 alone in plaintext parts.  If FALSE, then
									 go ahead and tweak the fonts according
									 to preferences.  (Only used when compiled
									 with MIME_TO_HTML.) */
#endif /* !AKBAR */

  char *default_charset;	/* If this is non-NULL, then it is the charset to
							   assume when no other one is specified via a
							   `charset' parameter.
							 */
  char *override_charset;	/* If this is non-NULL, then we will assume that
							   all data is in this charset, regardless of what
							   the `charset' parameter of that part says.
							   This overrides `default_charset' as well.
							   (This is to cope with the fact that, in the
							   real world, many messages are mislabelled with
							   the wrong charset.)
							 */

  /* =======================================================================
	 Stream-related callbacks; for these functions, the `closure' argument
	 is what is found in `options->stream_closure'.  (One possible exception
	 is for output_fn; see "output_closure" below.)
   */
  void *stream_closure;

  /* For setting up the display stream, so that the MIME parser can inform
	 the caller of the type of the data it will be getting. */
  int (*output_init_fn) (const char *type,
						 const char *charset,
						 const char *name,
						 const char *x_mac_type,
						 const char *x_mac_creator,
						 void *stream_closure);

  /* How the MIME parser feeds its output (HTML or raw) back to the caller. */
  int (*output_fn) (char *buf, int32 size, void *closure);

  /* Closure to pass to the above output_fn.  If NULL, then the
	 stream_closure is used. */
  void *output_closure;

  /* A callback used to reset the HTML parser to its default state -- this
	 function should close off any tags we've left open, reset the font
	 size and face, etc.  This may be called multiple times -- in particular,
	 it will be called at the end of each message part which might contain
	 human-generated (and thus arbitrarily buggy) markup.  The `abort_p'
	 argument specifies whether it's ok for data to be discarded.
	 (Only used when compiled with MIME_TO_HTML.)
   */
  int (*reset_html_state_fn) (void *stream_closure, XP_Bool abort_p);

  /* A hook for the caller to perform charset-conversion before HTML is
	 returned.  Each set of characters which originated in a mail message
	 (body or headers) will be run through this filter before being converted
	 into HTML.  (This should return bytes which may appear in an HTML file,
	 ie, we must be able to scan through the string to search for "<" and
	 turn it in to "&lt;", and so on.)

	 `input' is a non-NULL-terminated string of a single line from the message.
	 `input_length' is how long it is.
	 `input_charset' is a string representing the charset of this string (as
	   specified by MIME headers.)
	 `output_charset' is the charset to which conversion is desired.
	 `output_ret' is where a newly-malloced string is returned.  It may be
	   NULL if no translation is needed.
	 `output_size_ret' is how long the returned string is (it need not be
	   NULL-terminated.).
   */
  int (*charset_conversion_fn) (const char *input_line, int32 input_length,
								const char *input_charset,
								const char *output_charset,
								char **output_ret, int32 *output_size_ret,
								void *stream_closure);

  /* A hook for the caller to perform both charset-conversion and decoding of
	 MIME-2 header fields (using RFC-1522 encoding.)  Arguments and returned
	 values are as for `charset_conversion_fn'.
   */
  int (*rfc1522_conversion_fn) (const char *input_line, int32 input_length,
								const char *input_charset,
								const char *output_charset,
								char **output_ret, int32 *output_size_ret,
								void *stream_closure);

  /* A hook for the caller to translate a time string into a prettier or more
	 compact or localized form. */
  char *(*reformat_date_fn) (const char *old_date, void *stream_closure);

  /* A hook for the caller to turn a file name into a content-type. */
  char *(*file_type_fn) (const char *filename, void *stream_closure);

  /* A hook for the caller to turn a content-type into descriptive text. */
  char *(*type_description_fn) (const char *content_type,void *stream_closure);

  /* A hook for the caller to turn a content-type into an image icon. */
  char *(*type_icon_name_fn) (const char *content_type, void *stream_closure);

  /* A hook by which the user may be prompted for a password by the security
	 library.  (This is really of type `SECKEYGetPasswordKey'; see sec.h.) */
  void *(*passwd_prompt_fn)(void *arg1, void *arg2);
  void *passwd_prompt_fn_arg;

  /* =======================================================================
	 Various callbacks; for all of these functions, the `closure' argument
	 is what is found in `html_closure'.  (Only used when compiled with
	 MIME_TO_HTML.)
   */
  void *html_closure;

  /* For emitting some HTML before the start of the outermost message
	 (this is called before any HTML is written to layout.) */
  MimeHTMLGeneratorFunction generate_header_html_fn;

  /* For emitting some HTML after the outermost header block, but before
	 the body of the first message. */
  MimeHTMLGeneratorFunction generate_post_header_html_fn;

  /* For emitting some HTML at the very end (this is called after libmime
	 has written everything it's going to write.) */
  MimeHTMLGeneratorFunction generate_footer_html_fn;

  /* For turning a message ID into a loadable URL. */
  MimeHTMLGeneratorFunction generate_reference_url_fn;

  /* For turning a mail address into a mailto URL. */
  MimeHTMLGeneratorFunction generate_mailto_url_fn;

  /* For turning a newsgroup name into a news URL. */
  MimeHTMLGeneratorFunction generate_news_url_fn;

  /* =======================================================================
     Callbacks to handle the backend-specific inlined image display
	 (internal-external-reconnect junk.)  For `image_begin', the `closure'
	 argument is what is found in `stream_closure'; but for all of the
	 others, the `closure' argument is the data that `image_begin' returned.
   */

  /* Begins processing an embedded image; the URL and content_type are of the
	 image itself.  (Only used when compiled with MIME_TO_HTML.) */
  void *(*image_begin) (const char *image_url, const char *content_type,
						void *stream_closure);

  /* Stop processing an image. */
  void (*image_end) (void *image_closure, int status);

  /* Dump some raw image data down the stream. */
  int (*image_write_buffer) (char *buf, int32 size, void *image_closure);

  /* What HTML should be dumped out for this image. */
  char *(*make_image_html) (void *image_closure);


  /* =======================================================================
     Callbacks to handle flatening; this is somewhat different than the way
	 output_init_fn and output_fn work, so we have different callbacks here.
	 For `leaf_begin', the `closure' argument is what is found in
	 `leaf_closure'; but for `leaf_write' and `leaf_end', the closure
	 argument is the data that `leaf_begin' returned.  (Only used when
	 compiled with MIME_LEAF_TEXT.)
   */
  void *leaf_closure;

  /* A leaf node has been encountered... */
  void *(*leaf_begin) (const char *type,
					   MimeHeaders *leaf_headers,
					   MimeHeaders *container_headers,
					   void *leaf_closure);

  /* Here's its body (if it's textual)... */
  int (*leaf_write) (char *buf, int32 size, void *leaf_closure);

  /* Reached the end. */
  int (*leaf_end) (void *leaf_closure, XP_Bool abort_p);


  /* =======================================================================
	 Other random opaque state.
   */
  MimeParseStateObject *state;		/* Some state used by libmime internals;
									   initialize this to 0 and leave it alone.
									 */


#ifdef MIME_DRAFTS
  /* =======================================================================
	Mail Draft hooks -- 09-19-1996
   */
  XP_Bool decompose_file_p;            /* are we decomposing a mime msg 
									   into separate files */
  XP_Bool done_parsing_outer_headers;  /* are we done parsing the outer message
										  headers; this is really useful when
										  we have multiple Message/RFC822 
										  headers */
  XP_Bool is_multipart_msg;            /* are we decomposing a multipart
										  message */

  int decompose_init_count;            /* used for non multipart message only
										*/

  XP_Bool signed_p;					   /* to tell draft this is a signed
										  message */

  /* Callback to gather the outer most headers so we could use the 
	 information to initialize the addressing/subject/newsgroups fields
	 for the composition window. */
  int (*decompose_headers_info_fn) (void *closure, 
									MimeHeaders *headers);

  /* Callbacks to create temporary files for drafts attachments. */
  int (*decompose_file_init_fn) (void *stream_closure, 
								 MimeHeaders *headers );

  int (*decompose_file_output_fn) (char *buf, int32 size,
								   void *stream_closure);

  int (*decompose_file_close_fn) (void *stream_closure);
#endif /* MIME_DRAFTS */

};



/* Mozilla-specific interfaces
 */

/* Given a URL, this might return a better suggested name to save it as.

   When you have a URL, you can sometimes get a suggested name from
   URL_s->content_name, but if you're saving a URL to disk before the
   URL_Struct has been filled in by netlib, you don't have that yet.

   So if you're about to prompt for a file name *before* you call FE_GetURL
   with a format_out of FO_SAVE_AS, call this function first to see if it
   can offer you advice about what the suggested name for that URL should be.

   (This works by looking in a cache of recently-displayed MIME objects, and
   seeing if this URL matches.  If it does, the remembered content-name will
   be used.)
 */
extern char *MimeGuessURLContentName(MWContext *context, const char *url);


/* Determines whether the given context is currently showing a text/html
   message.  (Used by libmsg to determine if replys should bring up the
   text/html editor. */

#ifdef MIME_TO_HTML
extern XP_Bool MimeShowingTextHtml(MWContext* context);
#endif /* MIME_TO_HTML */



/* Yeech, hack...  Determine the URL to use to save just the HTML part of the
   currently-displayed message to disk.  If the current message doesn't have
   a text/html part, returns NULL.  Otherwise, the caller must free the
   returned string using XP_FREE(). */

#ifdef MIME_TO_HTML
extern char* MimeGetHtmlPartURL(MWContext* context);
#endif /* MIME_TO_HTML */


/* Return how many attachments are in the currently-displayed message. */
extern int MimeGetAttachmentCount(MWContext* context);

/* Returns what attachments are being viewed  in the currently-displayed
   message.  The returned data must be free'd using
   MimeFreeAttachmentList(). */
extern int MimeGetAttachmentList(MWContext* context,
								 MSG_AttachmentData** data);

extern void MimeFreeAttachmentList(MSG_AttachmentData* data);


/* Call this when destroying a context; this frees up some memory.
 */
extern void MimeDestroyContextData(MWContext *context);


/* After a message has been fully displayed (the document has finished
   loading) FEs call this with a Mail or News window to determine how
   the "security" toolbar button should be rendered.

   The first two values are whether it was an encrypted and/or signed
   message; the second two are whether it was *valid*.
 */
extern void MIME_GetMessageCryptoState(MWContext *context,
									   XP_Bool *signed_return,
									   XP_Bool *encrypted_return,
									   XP_Bool *signed_ok_return,
									   XP_Bool *encrypted_ok_return);

#ifndef AKBAR
/* Used only by libnet, this indicates that the user bonked on the "show me
   details about attachments" button. */

extern int MIME_DisplayAttachmentPane(MWContext* context);
#endif /* AKBAR */

XP_END_PROTOS

#endif /* _LIBMIME_H_ */
