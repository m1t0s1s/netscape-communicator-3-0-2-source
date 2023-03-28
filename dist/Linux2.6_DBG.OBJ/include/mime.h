/* -*- Mode: C; tab-width: 4 -*- */

#ifndef MIME_H
#define MIME_H

#include "ntypes.h"

XP_BEGIN_PROTOS

extern PUBLIC const char *FE_UsersOrganization(void);


/* Returns the appropriate contents of a From: field of a mail message
   originating from the current user.  This calls FE_UsersFullName()
   and FE_UsersMailAddress() and correctly munges the values, using
   MSG_MakeFullAddress()

   A new string is returned, which you must free when you're done with it.
   */
extern PUBLIC char *MIME_MakeFromField (void);

extern PUBLIC void MISC_ValidateSignature (MWContext *context,
										   const char *sig);

/* This does a very rough sanity-check on the return email address specified
   in preferences, and pops up a dialog and returns negative if there is
   something obviously wrong.  MSG_ComposeMessage() calls this, and won't
   let messages be composed until the problem is corrected.  The FEs should
   call this after preferences have been edited as well.

   The address should be just the email address, not including the real name.
 */
extern PUBLIC int MISC_ValidateReturnAddress (MWContext *context,
											  const char *addr);


/* Convert a block of text to the MIME quoted-printable encoding.
   Returns a new string and its size, or NULL if it couldn't allocate.
 */
extern PUBLIC void MIME_EncodeQuotedPrintableString(const unsigned char *input,
													uint32 input_size,
													unsigned char **output,
													uint32 *output_size);

/* Convert a block of text to the MIME base64 encoding.
   Returns a new string and its size, or NULL if it couldn't allocate.
 */
extern PUBLIC void MIME_EncodeBase64String(const unsigned char *input,
										   uint32 input_size,
										   unsigned char **output,
										   uint32 *output_size);

/* build a mailto: url address given a to field
 *
 * returns a malloc'd string
 */
extern char *
MIME_BuildMailtoURLAddress(const char * to);

/* build a news: url address to post to, given a partial news-post
 * URL and the newsgroups line
 *
 * returns a malloc'd string
 */
extern char *
MIME_BuildNewspostURLAddress(const char *partial_newspost_url,
                             const char *newsgroups);

typedef void
(*message_delivery_done_callback_func) (MWContext  *context, 
										void       *fe_data,
										int         status,
									    const char       *error_message);

/* Routines to generate MIME messages, and then (optionally) deliver them
   to mail and news servers.  See libmsg/compose.c for full descriptions of
   arguments.
 */
typedef struct MSG_AttachmentData
{
  const char *url;			/* The URL to attach.
							   This should be 0 to signify "end of list".
							 */

  const char *desired_type;	/* The type to which this document should be
							   converted.  Legal values are NULL, TEXT_PLAIN
							   and APPLICATION_POSTSCRIPT (which are macros
							   defined in net.h); other values are ignored.
							 */

  const char *real_type;	/* The type of the URL if known, otherwise NULL.
							   For example, if you were attaching a temp file
							   which was known to contain HTML data, you would
							   pass in TEXT_HTML as the real_type, to override
							   whatever type the name of the tmp file might
							   otherwise indicate.
							 */
  const char *real_encoding; /* Goes along with real_type */

  const char *real_name;	/* The original name of this document, which will
							   eventually show up in the Content-Disposition
							   header.  For example, if you had copied a
							   document to a tmp file, this would be the
							   original, human-readable name of the document.
							 */

  const char *description;	/* If you put a string here, it will show up as
							   the Content-Description header.  This can be
							   any explanatory text; it's not a file name.
							 */

  const char *x_mac_type, *x_mac_creator;
							/* Random bogus pseudo-standard losing Mac-specific
							   data that should show up as optional parameters
							   to the content-type header.  If you don't know,
							   you don't want to...
							 */
} MSG_AttachmentData;


extern void
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
						  message_delivery_done_callback_func callback);

extern void
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
					  message_delivery_done_callback_func callback);


/* If the argument is true, we use quoted-printable encoding on mail and
   news messages which have 8bit characters in them.  Otherwise, we assume
   that the mail and news transport will not strip the eighth bit (a bad
   assumption.) */
extern void MIME_ConformToStandard (XP_Bool conform_p);


/* Given a string which contains a list of RFC822 addresses, parses it into
   their component names and mailboxes.

   The returned value is the number of addresses, or a negative error code;
   the names and addresses are returned into the provided pointers as
   consecutive null-terminated strings.  It is up to the caller to free them.
   Note that some of the strings may be zero-length.

   Either of the provided pointers may be NULL if the caller is not interested
   in those components.
 */
extern int MSG_ParseRFC822Addresses (const char *line,
									 char **names, char **addresses);


/* Given a string which contains a list of RFC822 addresses, returns a
   comma-seperated list of just the `mailbox' portions.
 */
extern char *MSG_ExtractRFC822AddressMailboxes (const char *line);


/* Given a string which contains a list of RFC822 addresses, returns a
   comma-seperated list of just the `user name' portions.  If any of
   the addresses doesn't have a name, then the mailbox is used instead.
 */
extern char *MSG_ExtractRFC822AddressNames (const char *line);

/* Like MSG_ExtractRFC822AddressNames(), but only returns the first name
   in the list, if there is more than one. 
 */
extern char *MSG_ExtractRFC822AddressName (const char *line);

/* Given a string which contains a list of RFC822 addresses, returns a new
   string with the same data, but inserts missing commas, parses and reformats
   it, and wraps long lines with newline-tab.
 */
extern char * MSG_ReformatRFC822Addresses (const char *line);

/* Returns a copy of ADDRS which may have had some addresses removed.
   Addresses are removed if they are already in either ADDRS or OTHER_ADDRS.
   (If OTHER_ADDRS contain addresses which are not in ADDRS, they are not
   added.  That argument is for passing in addresses that were already
   mentioned in other header fields.)

   Addresses are considered to be the same if they contain the same mailbox
   part (case-insensitive.)  Real names and other comments are not compared.
 */
extern char *MSG_RemoveDuplicateAddresses (const char *addrs,
										   const char *other_addrs);


/* Given an e-mail address and a person's name, cons them together into a
   single string of the form "name <address>", doing all the necessary quoting.
   A new string is returned, which you must free when you're done with it.
 */
extern char *MSG_MakeFullAddress (const char* name, const char* addr);


/* Generate headers for a form post to a mailto: URL.
   This lets the URL specify additional headers, but is careful to
   ignore headers which would be dangerous.  It may modify the URL
   (because of CC) so a new URL to actually post to is returned.
 */
extern int MIME_GenerateMailtoFormPostHeaders (const char *old_post_url,
											   const char *referer,
											   char **new_post_url_return,
											   char **headers_return);


extern NET_StreamClass * 
NET_MimeMakePartialEncodingConverterStream (int          format_out,
											void        *data_obj,
											URL_Struct  *URL_s,
											MWContext   *window_id,
											NET_StreamClass *next_stream);

XP_END_PROTOS

#endif /* mime.h */
