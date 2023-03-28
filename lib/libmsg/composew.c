/* -*- Mode: C; tab-width: 4 -*-
   composew.c --- implementation of the message composition window
   Copyright © 1998 Netscape Communications Corporation, all rights reserved.
   Created: Jamie Zawinski <jwz@netscape.com>, 14-Jul-95.
 */

#include "msg.h"
#include "mailsum.h"
#include "mime.h"
#include "shist.h"
#include "xlate.h"
#include "bkmks.h"
#include "libi18n.h"
#include "xpgetstr.h"
#include "prefapi.h"

extern int MK_MSG_MSG_COMPOSITION;

extern int MK_COMMUNICATIONS_ERROR;
extern int MK_OUT_OF_MEMORY;

extern int MK_MSG_EMPTY_MESSAGE;
extern int MK_MSG_DOUBLE_INCLUDE;

extern int MK_MSG_WHY_QUEUE_SPECIAL;
extern int MK_MSG_NOT_AS_SENT_FOLDER;

extern int MK_MSG_FORWARDING_ENCRYPTED_WARNING;


/* This is inside an MWContext of type MWContextMessageComposition */
typedef struct MSG_CompositionFrame
{
  MSG_REPLY_TYPE reply_type;		/* The kind of message composition in
									   progress (reply, forward, etc.) */

  char *newspost_url;				/* If this is a news post, this is the
									   URL we should use when delivering. */

  MSG_HEADER_SET visible_headers;	/* The list of headers currently visible
									   in the UI. */

  XP_Bool markup_p;					/* Whether we should generate messages
									   whose first part is text/html rather
									   than text/plain. */

  MSG_AttachmentData *attach_data;	/* null-terminated list of the URLs and
									   desired types currently scheduled for
									   attachment.
									 */
  MSG_AttachedFile *attached_files;	/* The attachments which have already been
									   downloaded, and some info about them.
									 */

  char *references;					/* Uneditable "References" header field. */
  char *default_url;				/* Default URL for attaching, etc. */

  char *from;			/* The current values of the text fields currently */
  char *reply_to;		/* visible in the UI, as reported by the FE via    */
  char *to;				/* MSG_SetHeaderContents().                        */
  char *cc;				
  char *bcc;
  char *fcc;
  char *newsgroups;
  char *followup_to;
  char *organization;
  char *message_id;
  char *subject;
  char *other_random_headers;

  XP_Bool encrypt_p;
  XP_Bool sign_p;

  int security_delivery_error;

  /* Stuff used while quoting a message. */
  PrintSetup p;
  MWContext *textContext;
  char* quoteurl;
  URL_Struct *dummyurl;
  Net_GetUrlExitFunc *exit_quoting;
  
  XP_Bool delivery_in_progress;    /* True while mail is being sent. */
  XP_Bool attachment_in_progress;  /* True while attachments being saved. */

  XP_Bool body_edited_p;
  XP_Bool cited_p;
  
  XP_Bool duplicate_post_p;		/* Whether we seem to be trying for a second
								   time to post the same message.  (If this
								   is true, then we know to ignore
								   435 errors from the newsserver.) */

  /* This sucks sucks sucks sucks sucks sucks sucks. */
  int status;

} MSG_ComposeFrame;


/* Through preferences, the user can set the default values for certain
   headers (and can set them independently for mail and news.) */
struct msg_default_headers
{
  char *reply_to;
  char *bcc;
  char *fcc;
  XP_Bool bcc_self;
  char* bcc_storage;			/* Temporary place to store results of last
								   calculation of full bcc field. */
};

static struct msg_default_headers msg_default_mail_headers = { 0 };
static struct msg_default_headers msg_default_news_headers = { 0 };

static void msg_set_composition_window_title (MWContext *context);
static void msg_download_attachments (MWContext *context);
static void msg_free_attachment_list(MWContext *,
									 struct MSG_AttachmentData *list);

XP_Bool msg_auto_quote_reply = TRUE;

void
MSG_SetAutoQuoteReply(XP_Bool value)
{
  msg_auto_quote_reply = value;
}


static const char*
msg_figure_bcc(MWContext* context, struct msg_default_headers* headers)
{
  MWContext* addressbook_context;
  char* tmp;
  FREEIF(headers->bcc_storage);
  if (!headers->bcc_self) {
	headers->bcc_storage = XP_STRDUP(headers->bcc ? headers->bcc : "");
  } else if (!headers->bcc || !*headers->bcc) {
	headers->bcc_storage = XP_STRDUP(FE_UsersMailAddress());
  } else {
	headers->bcc_storage = PR_smprintf("%s, %s", FE_UsersMailAddress(),
									   headers->bcc);
  }
  if (headers->bcc_storage) {
	addressbook_context = FE_GetAddressBook(context, FALSE);
	if (addressbook_context) {
	  tmp = BM_ExpandHeaderString(addressbook_context, headers->bcc_storage,
								  FALSE);
	  if (tmp) {
		XP_FREE(headers->bcc_storage);
		headers->bcc_storage = tmp;
	  }
	}
  }
  return headers->bcc_storage;
}


static const char* 
msg_check_for_losing_fcc(MWContext* context, const char* fcc)
{
  if (context && fcc && *fcc) {
	char *q = msg_QueueFolderName(context);
	if (q && *q && !XP_FILENAMECMP (q, fcc)) {
	  char *buf;

	  /* We cannot allow them to use the queued mail folder
		 as their fcc folder, too.  Tell them why. */

	  buf = PR_smprintf("%s%s", XP_GetString(MK_MSG_WHY_QUEUE_SPECIAL),
						XP_GetString(MK_MSG_NOT_AS_SENT_FOLDER));
	  if (buf) {
		FE_Alert(context, buf);
		XP_FREE(buf);
	  }

	  /* Now ignore the FCC file they passed in. */
	  fcc = 0;
	}
  }
  return fcc;
}


static MWContext*
msg_compose_message(MWContext *old_context,
					const char *from,
					const char *reply_to,
					const char *to,
					const char *cc,
					const char *bcc,
					const char *fcc,
					const char *newsgroups,
					const char *followup_to,
					const char *organization,
					const char *subject,
					const char *references,
					const char *other_random_headers,
					const char *attachment,
					const char *newspost_url,
					const char *body,
					XP_Bool encrypt_p,
					XP_Bool sign_p)
{
  MWContext *context;
  MSG_ComposeFrame *frame;
  const char *real_addr = FE_UsersMailAddress ();
  char *real_return_address;
  char* attachment_string;
  const char* sig;
  XP_Bool forward_quoted;
  XP_Bool compose_as_news_message = FALSE;

	/* The theory here is, if the encrypt_p or sign_p options were passed in
	   to this function, then there's a good reason to try and do that (for
	   example, one should default to encrypting replies to encrypted
	   messages.)

	   However, if those arguments are false, then that doesn't mean *don't*
	   encrypt, it means don't *necessarily* encrypt -- so in that case, we
	   should encrypt if the user has said that they want to do that by
	   default.
	 */
	if (!newsgroups || !*newsgroups)	/* it's a mail message */
	  {
#ifdef HAVE_SECURITY /* added by jwz */
		if (! encrypt_p) encrypt_p = MSG_GetMailEncryptionPreference();
		if (! sign_p)    sign_p    = MSG_GetMailSigningPreference();
#endif /* !HAVE_SECURITY -- added by jwz */
	  }
	else								/* it's a news message */
	  {
		encrypt_p = FALSE;
#ifdef HAVE_SECURITY /* added by jwz */
		if (! sign_p) sign_p = MSG_GetNewsSigningPreference();
#endif /* !HAVE_SECURITY -- added by jwz */
	  }


  forward_quoted = FALSE;

  /* hack for forward quoted.  Checks the attachment field for a cookie string
     indicating that this is a forward quoted operation.  If a cookie is found,
     the attachment string is slid back down over the cookie.  This will put
	 the original string back in tact. */

  if (attachment)
    {  
      if (!XP_STRNCMP(attachment, MSG_FORWARD_COOKIE,
					  strlen(MSG_FORWARD_COOKIE)))
      {
        char * src;
        src = (char*)attachment + XP_STRLEN(MSG_FORWARD_COOKIE);
        forward_quoted = TRUE;      /* set forward with quote flag */
        XP_STRCPY(attachment, src);
      }
    }

  if (MISC_ValidateReturnAddress (old_context, real_addr) < 0)
	return NULL;

  real_return_address = MIME_MakeFromField ();
  context = FE_MakeMessageCompositionContext (old_context);

  if (!context) return NULL;
  XP_ASSERT (context->type == MWContextMessageComposition);
  XP_ASSERT (XP_FindContextOfType (0, MWContextMessageComposition));
  XP_ASSERT (!context->msg_cframe);
  context->msg_cframe = frame = (MSG_ComposeFrame *)
	XP_CALLOC (sizeof (MSG_ComposeFrame), 1);
  if (!frame) return NULL;

  frame->status = -1;

  if (!to) to = "";
  if (!from) from = "";
  if (!reply_to) reply_to = "";
  if (!to) to = "";
  if (!cc) cc = "";
  if (!bcc) bcc = "";
  if (!fcc) fcc = "";
  if (!newsgroups) newsgroups = "";
  else compose_as_news_message = TRUE;
  if (!followup_to) followup_to = "";
  if (!organization) organization = "";
  if (!subject) subject = "";
  if (!references) references = "";
  if (!other_random_headers) other_random_headers = "";
  if (!attachment) attachment = "";
  if (!newspost_url) newspost_url = "";

  frame->encrypt_p = !!encrypt_p;
  frame->sign_p = !!sign_p;

  frame->references   = XP_STRDUP (references);
  frame->newspost_url = XP_STRDUP (newspost_url);

  if (old_context && !XP_STRCMP(attachment, "(digest)")) /* #### gag */
	{
	  int count = 0;
	  char** list = msg_GetSelectedMessageURLs (old_context);
	  if (list)
		{
		  MSG_AttachmentData *alist;
		  while (list[count]) count++;
		  alist = (struct MSG_AttachmentData *)
			XP_ALLOC((count + 1) * sizeof(MSG_AttachmentData));
		  if (alist)
			{
			  XP_MEMSET(alist, 0, (count + 1) * sizeof(*alist));
			  for (count--; count >= 0; count--)
				alist[count].url = list[count];
			  MSG_SetAttachmentList(context, alist);
			  XP_FREE(alist);
			}
		  XP_FREE(list);
		}
	}
  else if (*attachment)
	{
	  struct MSG_AttachmentData alist[2];
	  XP_MEMSET(alist, 0, sizeof(alist));
	  alist[0].url = attachment;
	  MSG_SetAttachmentList(context, alist);
	}

  if (*attachment)
	{
	  if (*attachment != '(')
		frame->default_url = XP_STRDUP (attachment);
	}
  else if (old_context)
	{
	  History_entry *h = SHIST_GetCurrent (&old_context->hist);
	  if (h && h->address)
		frame->default_url = XP_STRDUP (h->address);
	}

  if (!*from)
	from = real_return_address;

  /* Guess what kind of reply this is based on the headers we passed in.
   */
  if (*attachment) 
    {
      /* if an attachment exists and the forward_quoted flag is set, this
         is a forward quoted operation. */
      if (forward_quoted)
        {
          frame->reply_type = MSG_ForwardMessageQuoted;
          /* clear out the attachment list for forward quoted messages. */
          MSG_SetAttachmentList(context,NULL);
        }
      else
	    frame->reply_type = MSG_ForwardMessage;
    }
  else if (*references && *newsgroups && (*to || *cc))
	frame->reply_type = MSG_PostAndMailReply;
  else if (*references && *newsgroups)
	frame->reply_type = MSG_PostReply;
  else if (*references && *cc)
	frame->reply_type = MSG_ReplyToAll;
  else if (*references && *to)
	frame->reply_type = MSG_ReplyToSender;
  else if (*newsgroups || compose_as_news_message)
	frame->reply_type = MSG_PostNew;
  else
	frame->reply_type = MSG_MailNew;

  if (!*organization)
	organization = FE_UsersOrganization();

  if (frame->reply_type == MSG_PostNew || *newsgroups)
	{
	  if (!*reply_to) reply_to = msg_default_news_headers.reply_to;
	  if (!*fcc) fcc = msg_default_news_headers.fcc;
	  if (!*bcc) bcc = msg_figure_bcc(context, &msg_default_news_headers);
	}
  else
	{
	  if (!*reply_to) reply_to = msg_default_mail_headers.reply_to;
#ifdef DEBUG_jwz
	  if (!*fcc)
		{
		  /* By default, FCC to the selected mail folder. */
		  if (old_context &&
			  old_context->type == MWContextMail &&
			  old_context->msgframe &&
			  old_context->msgframe->opened_folder &&
			  old_context->msgframe->opened_folder->data.mail.pathname)
			fcc = old_context->msgframe->opened_folder->data.mail.pathname;
		  else
			fcc = msg_default_mail_headers.fcc;
		}
#else
	  if (!*fcc) fcc = msg_default_mail_headers.fcc;
#endif
	  if (!*bcc) bcc = msg_figure_bcc(context, &msg_default_mail_headers);
	}
  fcc = msg_check_for_losing_fcc(context, fcc);
  if (!bcc) bcc = "";
  if (!fcc) fcc = "";
  if (!reply_to) reply_to = "";


  /* If we're forwarding a message, and that message was encrypted, then
	 the default should be to encrypt the "outer" message as well.
	 #### This only works if we're forwarding a single message.
	 (It also only works if that message is currently displayed in
	 old_context, but I think that it must be for us to have gotten here.)
   */
  if (!frame->encrypt_p &&
	  frame->reply_type == MSG_ForwardMessage)
	{
	  XP_Bool signed_p = FALSE, encrypted_p = FALSE;
	  MIME_GetMessageCryptoState(old_context, &signed_p, &encrypted_p, 0, 0);
	  if (encrypted_p)
		frame->encrypt_p = TRUE;
	}


  attachment_string = MSG_GetAttachmentString(context);

  FE_InitializeMailCompositionContext (context,
									   from, reply_to, to, cc, bcc, fcc,
									   newsgroups, followup_to, subject,
									   attachment_string);

  FREEIF(attachment_string);
  FREEIF (context->msg_cframe->organization);
  if (organization)
    context->msg_cframe->organization = XP_STRDUP(organization);
  else
    context->msg_cframe->organization = XP_STRDUP("");

  sig = FE_UsersSignature ();
  if (sig) {
	FE_InsertMessageCompositionText(context, sig, TRUE);
	/* If the sig doesn't begin with "--" followed by whitespace or a
	   newline, insert "-- \n" (the pseudo-standard sig delimiter.) */
	if (sig[0] != '-' || sig[1] != '-' ||
		(sig[2] != ' ' && sig[2] != CR && sig[2] != LF)) {
	  FE_InsertMessageCompositionText(context, "-- " LINEBREAK, TRUE);
	}
	FE_InsertMessageCompositionText(context, LINEBREAK, TRUE);
  }

  MSG_SetHeaderContents (context, MSG_FROM_HEADER_MASK, from);
  FREEIF (real_return_address);

  context->msg_cframe->body_edited_p = FALSE;

  if (body && *body)
	{
	  FE_InsertMessageCompositionText(context, LINEBREAK, TRUE);
	  FE_InsertMessageCompositionText(context, body, TRUE);
	  context->msg_cframe->body_edited_p = TRUE;
	}

  /* The FE should have called MSG_SetHeaderContents() above. */
  XP_ASSERT (frame->from && frame->to && frame->reply_to && frame->cc &&
			 frame->bcc && frame->fcc && frame->newsgroups &&
			 frame->followup_to && frame->subject);

  msg_ConfigureCompositionHeaders (context, FALSE);

  msg_set_composition_window_title (context);
  FE_RaiseMailCompositionWindow (context);

  /* check to see if this is either a forward quoted or auto quote operation.
   */
  if (frame->reply_type == MSG_ForwardMessageQuoted || msg_auto_quote_reply) {
	switch (frame->reply_type) {
    case MSG_ForwardMessageQuoted:
	case MSG_PostAndMailReply:
	case MSG_PostReply:
	case MSG_ReplyToAll:
	case MSG_ReplyToSender:
	  MSG_Command(context, MSG_QuoteMessage);
	  break;
	default:
	  break;
	}
  }

  /* Need this for crypto button to show up if not auto-quoting. */
  msg_UpdateToolbar(context);

  return context;
}


#ifdef DEBUG_jwz
const char *
MSG_CompositionWindowNewsgroups(MWContext *context)
{
  if (context &&
	  context->type == MWContextMessageComposition &&
	  context->msg_cframe &&
	  context->msg_cframe->newsgroups &&
	  *context->msg_cframe->newsgroups)
	return context->msg_cframe->newsgroups;
  else
	return 0;
}
#endif


MWContext*
MSG_ComposeMessage (MWContext *old_context,
					const char *from,
					const char *reply_to,
					const char *to,
					const char *cc,
					const char *bcc,
					const char *fcc,
					const char *newsgroups,
					const char *followup_to,
					const char *organization,
					const char *subject,
					const char *references,
					const char *other_random_headers,
					const char *attachment,
					const char *newspost_url,
					const char *body,
					XP_Bool encrypt_p,
					XP_Bool sign_p
					)
{
  return msg_compose_message(old_context, from, reply_to, to, cc, bcc, fcc,
							 newsgroups, followup_to, organization, subject,
							 references, other_random_headers, attachment,
							 body, newspost_url, encrypt_p, sign_p);
}


MWContext*
MSG_MailDocument (MWContext *old_context)
{
  const char *attachment = 0;
  MWContext* context;

#if 1 /* Do this if we want to attach the target of Mail Document by default */
  History_entry *h = (old_context ? SHIST_GetCurrent (&old_context->hist) : 0);
  if (h && h->address && *h->address)
	attachment = h->address;
#endif

  context = msg_compose_message(old_context, 0, 0, 0, 0, 0, 0, 0, 0, 0,
								old_context ? old_context->title : 0,
								0, 0, attachment, 0, 0, FALSE, FALSE);
  if (context &&
	  context->msg_cframe->default_url &&
	  *context->msg_cframe->default_url)
	{
	  FE_InsertMessageCompositionText(context, LINEBREAK, TRUE);
	  FE_InsertMessageCompositionText(context,
									  context->msg_cframe->default_url,
									  TRUE);
	  context->msg_cframe->body_edited_p = FALSE;
	}
  return context;
}


MWContext*
MSG_Mail (MWContext *old_context)
{
  return
	msg_compose_message(old_context, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,0,0,
						FALSE, FALSE);
}

const char*
MSG_GetAssociatedURL(MWContext* context)
{
  XP_ASSERT(context->type == MWContextMessageComposition);
  if (context->type == MWContextMessageComposition && context->msg_cframe) {
	return context->msg_cframe->default_url;
  }
  return NULL;
}

int
msg_SetCompositionFormattingStyle (MWContext *context, XP_Bool markup_p)
{
  XP_ASSERT (context->type == MWContextMessageComposition);
  if (context->type != MWContextMessageComposition) return -1;
  XP_ASSERT (context->msg_cframe);
  if (!context->msg_cframe) return -1;
  if (context->msg_cframe->markup_p == markup_p) return 0;
  context->msg_cframe->markup_p = markup_p;
  /* #### redisplay the FE stuff or something */
  return 0;
}

int
msg_SetCompositionEncrypted (MWContext *context, XP_Bool encrypt_p)
{
  XP_ASSERT (context->type == MWContextMessageComposition);
  if (context->type != MWContextMessageComposition) return -1;
  XP_ASSERT (context->msg_cframe);
  if (!context->msg_cframe) return -1;
  if (context->msg_cframe->encrypt_p == encrypt_p)
	return 0;
  context->msg_cframe->encrypt_p = encrypt_p;
  /* #### redisplay the FE stuff or something */
  return 0;
}

int
msg_SetCompositionSigned (MWContext *context, XP_Bool sign_p)
{
  XP_ASSERT (context->type == MWContextMessageComposition);
  if (context->type != MWContextMessageComposition) return -1;
  XP_ASSERT (context->msg_cframe);
  if (!context->msg_cframe) return -1;
  if (context->msg_cframe->sign_p == sign_p)
	return 0;
  context->msg_cframe->sign_p = sign_p;
  /* #### redisplay the FE stuff or something */
  return 0;
}

int
msg_ToggleCompositionEncrypted (MWContext *context)
{
  XP_ASSERT (context->type == MWContextMessageComposition);
  if (context->type != MWContextMessageComposition) return -1;
  XP_ASSERT (context->msg_cframe);
  if (!context->msg_cframe) return -1;
  return msg_SetCompositionEncrypted (context, !context->msg_cframe->encrypt_p);
}

int
msg_ToggleCompositionSigned (MWContext *context)
{
  XP_ASSERT (context->type == MWContextMessageComposition);
  if (context->type != MWContextMessageComposition) return -1;
  XP_ASSERT (context->msg_cframe);
  if (!context->msg_cframe) return -1;
  return msg_SetCompositionSigned (context, !context->msg_cframe->sign_p);
}

XP_Bool
msg_CompositionSendingEncrypted (MWContext *context)
{
  XP_ASSERT (context->type == MWContextMessageComposition);
  if (context->type != MWContextMessageComposition) return FALSE;
/*  XP_ASSERT (context->msg_cframe);*/
  if (!context->msg_cframe) return FALSE;
  return context->msg_cframe->encrypt_p;
}

XP_Bool
msg_CompositionSendingSigned (MWContext *context)
{
  XP_ASSERT (context->type == MWContextMessageComposition);
  if (context->type != MWContextMessageComposition) return FALSE;
/*  XP_ASSERT (context->msg_cframe);*/
  if (!context->msg_cframe) return FALSE;
  return context->msg_cframe->sign_p;
}



#define ALL_HEADERS (MSG_FROM_HEADER_MASK |			\
					 MSG_REPLY_TO_HEADER_MASK |		\
					 MSG_TO_HEADER_MASK |			\
					 MSG_CC_HEADER_MASK |			\
					 MSG_BCC_HEADER_MASK |			\
					 MSG_FCC_HEADER_MASK |			\
					 MSG_NEWSGROUPS_HEADER_MASK |	\
					 MSG_FOLLOWUP_TO_HEADER_MASK |	\
					 MSG_SUBJECT_HEADER_MASK |		\
					 MSG_ATTACHMENTS_HEADER_MASK)


int
msg_ConfigureCompositionHeaders (MWContext *context, XP_Bool show_all_p)
{
  uint32 desired_mask = 0;
  XP_ASSERT (context->type == MWContextMessageComposition);
  if (context->type != MWContextMessageComposition) return -1;
  XP_ASSERT (context->msg_cframe);
  if (!context->msg_cframe) return -1;

  if (show_all_p)
	{
	  /* Show every possible header field, whether empty or not.
	   */
	  desired_mask = ALL_HEADERS;
	}
  else
	{
	  /* The user has requested display of "interesting" header fields.
		 The logic here is a bit complicated, in the interest of DWIMity.
	   */

	  /* Cc, Subject, and Attachments are always interesting.
	   */
	  desired_mask |= (MSG_CC_HEADER_MASK |
					   MSG_SUBJECT_HEADER_MASK |
					   MSG_ATTACHMENTS_HEADER_MASK);

	  /* To is interesting if:
		 - it is non-empty, or
		 - this composition window was brought up with a "mail sending"
		   command (Mail New, Reply-*, Forward, or Post and Mail).
	   */
	  if ((context->msg_cframe->to && *context->msg_cframe->to) ||
		  context->msg_cframe->reply_type == MSG_MailNew ||
		  context->msg_cframe->reply_type == MSG_ReplyToSender ||
		  context->msg_cframe->reply_type == MSG_ReplyToAll ||
		  context->msg_cframe->reply_type == MSG_PostAndMailReply ||
		  context->msg_cframe->reply_type == MSG_ForwardMessage ||
		  context->msg_cframe->reply_type == MSG_ForwardMessageQuoted)
		desired_mask |= MSG_TO_HEADER_MASK;

#ifdef DEBUG_jwz
	  if (context->msg_cframe->reply_type == MSG_MailNew ||
		  context->msg_cframe->reply_type == MSG_ForwardMessage ||
		  context->msg_cframe->reply_type == MSG_ForwardMessageQuoted)
		desired_mask |= MSG_BCC_HEADER_MASK;
#endif

#ifdef DEBUG_jwz
	  /* hairball multi-account kludge */
		desired_mask |= MSG_FROM_HEADER_MASK;
#endif


	  /* CC is interesting if:
		 - it is non-empty, or
		 - this composition window was brought up as a reply to another
		   mail message.  (Should mail-and-post do this too?)
	   */
	  if ((context->msg_cframe->cc && *context->msg_cframe->cc) ||
		  context->msg_cframe->reply_type == MSG_ReplyToSender ||
		  context->msg_cframe->reply_type == MSG_ReplyToAll)
		desired_mask |= MSG_CC_HEADER_MASK;

	  /* Reply-To and BCC are interesting if:
		 - they are non-empty, AND
		 - they are different from both of the default values
		   (meaning the user has edited them this session.)
		*/
	  if (context->msg_cframe->reply_to &&
		  *context->msg_cframe->reply_to &&
		  ((msg_default_mail_headers.reply_to &&
			*msg_default_mail_headers.reply_to)
		   ? !!XP_STRCMP (context->msg_cframe->reply_to,
						  msg_default_mail_headers.reply_to)
		   : TRUE) &&
		  ((msg_default_news_headers.reply_to &&
			*msg_default_news_headers.reply_to)
		   ? !!XP_STRCMP (context->msg_cframe->reply_to,
						  msg_default_news_headers.reply_to)
		   : TRUE))
		desired_mask |= MSG_REPLY_TO_HEADER_MASK;

	  /* (see above.) */
	  if (context->msg_cframe->bcc &&
		  *context->msg_cframe->bcc &&
		  ((msg_default_mail_headers.bcc_storage &&
			*msg_default_mail_headers.bcc_storage)
		   ? !!XP_STRCMP (context->msg_cframe->bcc,
						  msg_default_mail_headers.bcc_storage)
		   : TRUE) &&
		  ((msg_default_news_headers.bcc_storage &&
			*msg_default_news_headers.bcc_storage)
		   ? !!XP_STRCMP (context->msg_cframe->bcc,
						  msg_default_news_headers.bcc_storage)
		   : TRUE))
		desired_mask |= MSG_BCC_HEADER_MASK;

	  /* FCC is never interesting.
	   */

	  /* Newsgroups is interesting if:
		 - it is non-empty, or
		 - this composition window was brought up with a "news posting"
		   command (Post New, Post Reply, or Post and Mail).
	   */
	  if ((context->msg_cframe->newsgroups &&
		   *context->msg_cframe->newsgroups) ||
		  context->msg_cframe->reply_type == MSG_PostNew ||
		  context->msg_cframe->reply_type == MSG_PostReply ||
		  context->msg_cframe->reply_type == MSG_PostAndMailReply)
		desired_mask |= MSG_NEWSGROUPS_HEADER_MASK;

	  /* Followup-To is interesting if:
		 - it is non-empty, AND
		 - it differs from the Newsgroups field.
	   */
	  if (context->msg_cframe->followup_to &&
		  *context->msg_cframe->followup_to &&
		  (context->msg_cframe->newsgroups
		   ? XP_STRCMP (context->msg_cframe->followup_to,
						context->msg_cframe->newsgroups)
		   : TRUE))
		desired_mask |= MSG_FOLLOWUP_TO_HEADER_MASK;
	}

  if (context->msg_cframe->visible_headers == desired_mask)
	return 0;
  context->msg_cframe->visible_headers = desired_mask;
  FE_MsgShowHeaders (context, desired_mask);
  return 0;
}

int
msg_ToggleCompositionHeader(MWContext* context, uint32 header)
{
  XP_ASSERT (context && context->msg_cframe);
  if (!context || !context->msg_cframe)
	return -1;
  if (context->msg_cframe->visible_headers & header) {
	context->msg_cframe->visible_headers &= ~header;
  } else {
	context->msg_cframe->visible_headers |= header;
  }
  FE_MsgShowHeaders(context, context->msg_cframe->visible_headers);
  return 0;
}

XP_Bool 
msg_ShowingAllCompositionHeaders(MWContext* context)
{
  return (context->msg_cframe != NULL &&
		  context->msg_cframe->visible_headers == ALL_HEADERS);
}

XP_Bool 
msg_ShowingCompositionHeader(MWContext* context, uint32 mask)
{
  if (context->type != MWContextMessageComposition) return FALSE;
  if (!context->msg_cframe) return FALSE; /* Can happen if we're in the
											 middle of creating the context.*/
  return (context->msg_cframe->visible_headers & mask) == mask;
}


static void
msg_get_url_done(PrintSetup* p)
{
  MWContext* context = (MWContext*) p->carg;
  MSG_ComposeFrame* f = context->msg_cframe;
  XP_File file;
  if (!f) return;
  FREEIF(f->quoteurl);
  f->textContext = NULL;  /* since this is called as a result of TXFE_AllConnectionsComplete, 
							 we know this context is going away by natural means */
  XP_FileClose(p->out);
  
  /* Open hateful temporary file as input  */
  file = XP_FileOpen (p->filename, xpTemporary, XP_FILE_READ);
  if (file) {
	char* buf;
	buf = (char*)XP_ALLOC(512 + 1);
	if (buf) {
	  XP_Bool body_edited_p = f->body_edited_p;
	  int l;
      CCCDataObject *conv;
      int doConv;

      /*
       * We aren't actually converting character encodings here.
       * (Note that both the "from" and "to" are the win_csid.)
       * This makes it call a special routine that makes sure we
       * deal with whole multibyte characters instead of partial
       * ones that happen to lie on a 512-byte boundary. -- erik
       */
      conv = INTL_CreateCharCodeConverter();
      if (conv) {
        doConv = INTL_GetCharCodeConverter(context->win_csid,
                                           context->win_csid, conv);
      } else {
        doConv = 0;
      }

	  while (0 < (l = XP_FileRead(buf, 512, file))) {
        char *newBuf;
		buf[l] = '\0';
        if (doConv) {
          newBuf = (char *) INTL_CallCharCodeConverter(conv,
                                                       (unsigned char *) buf,
                                                       (int32) l);
          if (!newBuf) {
            newBuf = buf;
          }
        } else {
          newBuf = buf;
        }
		FE_InsertMessageCompositionText(context, newBuf, FALSE);
        if (newBuf != buf) {
          XP_FREE(newBuf);
        }
	  }
	  XP_FREE(buf);
      if (conv) {
        INTL_DestroyCharCodeConverter(conv);
      }
	  f->body_edited_p = body_edited_p;
	}
	XP_FileClose(file);
  }
  f->cited_p = TRUE;
  XP_FileRemove(p->filename, xpTemporary);
  FREEIF(p->filename);
  if (f->exit_quoting) {
	(*f->exit_quoting)(f->dummyurl, 0, context);
	f->exit_quoting = NULL;
	f->dummyurl = NULL;

	/* ### Vile, gross, awful hack that manages to get the cursor back to
	   normal. */
	NET_SilentInterruptWindow(context);
  }

  /* Re-enable the UI. */
  FE_UpdateToolbar (context);
}

  

int
msg_QuoteMessage(MWContext* context)
{
  int status = 0;
  MSG_ComposeFrame* f = context->msg_cframe;
  char* ptr;
  if (!f || !f->default_url) return 0; /* Nothing to quote. */
  if (f->quoteurl) return 0;	/* Currently already quoting! */

  f->quoteurl = XP_STRDUP(f->default_url);
  if (!f->quoteurl) return MK_OUT_OF_MEMORY;

  /* remove any position information from the url
   */
  ptr = XP_STRCHR(f->quoteurl, '#');
  if (ptr) *ptr = '\0';

  XL_InitializeTextSetup(&(f->p));
  f->p.out = NULL;
  f->p.prefix = "> ";
  f->p.width = 70;	/* The default window is 72 wide; subtract 2 for "> ". */
  f->p.carg = context;
  f->p.url = NET_CreateURLStruct(context->msg_cframe->default_url,
								 NET_DONT_RELOAD);
  if (!f->p.url) {
	status = MK_OUT_OF_MEMORY;
	goto FAIL;
  }
  f->p.url->position_tag = 0;
  f->p.completion = msg_get_url_done;
  f->p.filename = WH_TempName(xpTemporary, "ns");
  if (!f->p.filename) {
	status = MK_OUT_OF_MEMORY;
	goto FAIL;
  }
  f->p.out = XP_FileOpen(f->p.filename, xpTemporary, XP_FILE_WRITE);
  if (!f->p.out) {
	status = -1;				/* ### Need the right error code! */
	goto FAIL;
  }
  f->p.cx = context;
  f->exit_quoting = NULL;
  f->dummyurl = NET_CreateURLStruct("about:", NET_DONT_RELOAD);
  if (f->dummyurl) {
	FE_SetWindowLoading(context, f->dummyurl, &f->exit_quoting);
	XP_ASSERT(f->exit_quoting != NULL);
  }

  /* Disable the UI while the quoted text is being loaded/formatted. */
  FE_UpdateToolbar (context);

  /* Start the URL loading... (msg_get_url_done gets called later.) */
  f->textContext = XL_TranslateText(context, f->p.url, &(f->p));

  return 0;
FAIL:
  FREEIF(f->p.filename);
  FREEIF(f->quoteurl);
  if (f->p.out) {
	XP_FileClose(f->p.out);
	f->p.out = NULL;
  }
  if (f->p.url) {
	NET_FreeURLStruct(f->p.url);
	f->p.url = NULL;
  }
  return status;
}


void 
MSG_SetAttachmentList(MWContext* context, struct MSG_AttachmentData* list)
{
  MSG_ComposeFrame *f = context->msg_cframe;
  int count = 0;
  MSG_AttachmentData *tmp;
  MSG_AttachmentData *tmp2;
  XP_ASSERT (context->type == MWContextMessageComposition);
  if (context->type != MWContextMessageComposition) return;
  XP_ASSERT (f);
  if (!f) return;

  MSG_ClearMessageID(context); /* Since the attachment list has changed, the
								  message has changed, so make sure we're using
								  a fresh message-id when we try to send it. */

  msg_free_attachment_list(context, f->attach_data);
  f->attach_data = NULL;

  for (tmp = list; tmp && tmp->url; tmp++) count++;

  if (count > 0)
	{
	  f->attach_data = XP_ALLOC((count + 1) * sizeof(MSG_AttachmentData));
	  if (!f->attach_data)
		{
		  FE_Alert(context, XP_GetString(MK_OUT_OF_MEMORY));
		  return;
		}

	  XP_MEMSET(f->attach_data, 0, (count + 1) * sizeof(MSG_AttachmentData));
	}

  if (count > 0)
	for (tmp = list, tmp2 = f->attach_data; tmp->url; tmp++, tmp2++)
	{
	  tmp2->url = XP_STRDUP(tmp->url);
	  if (tmp->desired_type) tmp2->desired_type = XP_STRDUP(tmp->desired_type);
	  if (tmp->real_type)    tmp2->real_type = XP_STRDUP(tmp->real_type);
	  if (tmp->real_encoding)tmp2->real_encoding=XP_STRDUP(tmp->real_encoding);
	  if (tmp->real_name)    tmp2->real_name = XP_STRDUP(tmp->real_name);
	  if (tmp->description)  tmp2->description = XP_STRDUP(tmp->description);
	  if (tmp->x_mac_type)   tmp2->x_mac_type = XP_STRDUP(tmp->x_mac_type);
	  if (tmp->x_mac_creator)tmp2->x_mac_creator=XP_STRDUP(tmp->x_mac_creator);
	}

  msg_download_attachments (context);
}


const struct MSG_AttachmentData *
MSG_GetAttachmentList(MWContext* context)
{
  MSG_ComposeFrame* f = context->msg_cframe;
  XP_ASSERT (context->type == MWContextMessageComposition);
  if (context->type != MWContextMessageComposition) return NULL;
  XP_ASSERT (f);
  if (!f) return NULL;

  if (f->attach_data && f->attach_data[0].url != NULL) return f->attach_data;
  return NULL;
}


static void
msg_free_attachment_list(MWContext* context, struct MSG_AttachmentData *list)
{
  MSG_AttachmentData* tmp;
  if (!list) return;
  for (tmp = list ; tmp->url ; tmp++)
	{
	  XP_FREE((char*) tmp->url);
	  if (tmp->desired_type) XP_FREE((char*) tmp->desired_type);
	  if (tmp->real_type) XP_FREE((char*) tmp->real_type);
	  if (tmp->real_encoding) XP_FREE((char*) tmp->real_encoding);
	  if (tmp->real_name) XP_FREE((char*) tmp->real_name);
	  if (tmp->description) XP_FREE((char*) tmp->description);
	  if (tmp->x_mac_type) XP_FREE((char*) tmp->x_mac_type);
	  if (tmp->x_mac_creator) XP_FREE((char*) tmp->x_mac_creator);
	}
  XP_FREE(list);
}


static void
msg_delete_attached_files (struct MSG_AttachedFile *attachments)
{
  struct MSG_AttachedFile *tmp;
  if (!attachments) return;
  for (tmp = attachments; tmp->orig_url; tmp++)
	{
	  FREEIF(tmp->orig_url);
	  FREEIF(tmp->type);
	  FREEIF(tmp->encoding);
	  FREEIF(tmp->description);
	  FREEIF(tmp->x_mac_type);
	  FREEIF(tmp->x_mac_creator);
	  if (tmp->file_name)
		{
		  XP_FileRemove(tmp->file_name, xpFileToPost);
		  XP_FREE(tmp->file_name);
		}
	}
  XP_FREE(attachments);
}


static void msg_download_attachments_done (MWContext *context, 
										   void *fe_data,
										   int status, const char *error_msg,
										   struct MSG_AttachedFile *);

/* Whether the given saved-attachment-file thing is a match for the given
   URL (in source and type-conversion.)
 */
static XP_Bool
msg_attachments_match (MSG_AttachmentData *attachment,
					   MSG_AttachedFile *file)
{
  const char *dt;
  XP_ASSERT(attachment && file);
  if (!attachment || !file) return FALSE;
  XP_ASSERT(attachment->url && file->orig_url);
  if (!attachment->url || !file->orig_url) return FALSE;

  XP_ASSERT(file->type);
  if (!file->type) return FALSE;
  XP_ASSERT(file->file_name);
  if (XP_STRCMP(attachment->url, file->orig_url))
	return FALSE;

  /* If the attachment has a conversion type specified (and it's not the
	 "no conversion" type) then this is only a match if the saved document
	 ended up with that type as well.
   */
  dt = ((attachment->desired_type && *attachment->desired_type)
		? attachment->desired_type
		: 0);
  if (dt && !strcasecomp(dt, TEXT_HTML))
	dt = 0;

  /* dt only has a value if it's "not `As Is', ie, text/plain or app/ps. */
  if (dt && XP_STRCMP(dt, file->type))
	return FALSE;

  return TRUE;
}


static void
msg_download_attachments (MWContext *context)
{
  MSG_ComposeFrame *f = context->msg_cframe;
  int attachment_count = 0;
  int new_download_count = 0;
  int download_overlap_count = 0;
  MSG_AttachmentData *tmp;
  MSG_AttachmentData *downloads = 0;
  MSG_AttachedFile *tmp2;
  XP_ASSERT (context->type == MWContextMessageComposition);
  if (context->type != MWContextMessageComposition) return;
  XP_ASSERT (f);
  if (!f) return;

  XP_ASSERT(!context->msg_cframe->delivery_in_progress);

  if (f->attach_data)
	for (tmp = f->attach_data; tmp->url; tmp++)
	  attachment_count++;

  /* First, go through the list of desired attachments, and the list of
	 currently-saved attachments, and delete the files (and data) of the
	 ones which were attached/saved but are no longer.
   */
  tmp2 = f->attached_files;
  while (tmp2 && tmp2->orig_url)
	{
	  XP_Bool match = FALSE;
	  for (tmp = f->attach_data; tmp && tmp->url; tmp++)
		{
		  if (msg_attachments_match(tmp, tmp2))
			{
			  match = TRUE;
			  break;
			}
		}
	  if (match)
		{
		  tmp2++;
		  download_overlap_count++;
		}
	  else
		{
		  /* Delete the file, free the strings, and pull the other entries
			 forward to cover this one. */
		  int i = 0;

		  if (tmp2->file_name)
			{
			  XP_FileRemove(tmp2->file_name, xpFileToPost);
			  XP_FREE(tmp2->file_name);
			}
		  FREEIF(tmp2->orig_url);
		  FREEIF(tmp2->type);
		  FREEIF(tmp2->encoding);
		  FREEIF(tmp2->description);
		  FREEIF(tmp2->x_mac_type);
		  FREEIF(tmp2->x_mac_creator);
		  
		  while (tmp2[i+1].orig_url)
			tmp2[i] = tmp2[++i];
		  tmp2[i].orig_url = 0;
		}
	}

  /* Now download any new files that are in the list.
   */
  if (download_overlap_count != attachment_count)
	{
	  MSG_AttachmentData *dfp;
	  new_download_count = attachment_count - download_overlap_count;
	  downloads = (MSG_AttachmentData *)
		XP_ALLOC(sizeof(MSG_AttachmentData) * (new_download_count + 1));
	  if (!downloads)
		{
		  FE_Alert(context, XP_GetString(MK_OUT_OF_MEMORY));
		  return;
		}
	  XP_MEMSET(downloads, 0, sizeof(*downloads) * (new_download_count + 1));

	  dfp = downloads;
	  for (tmp = f->attach_data; tmp && tmp->url; tmp++)
		{
		  XP_Bool match = FALSE;
		  if (f->attached_files)
			for (tmp2 = f->attached_files; tmp2->orig_url; tmp2++)
			  {
				if (msg_attachments_match(tmp, tmp2))
				  {
					match = TRUE;
					break;
				  }
			  }
		  if (!match)
			{
			  *dfp = *tmp;
			  dfp++;
			}
		}
	  if (!downloads[0].url)
		return;
	  XP_ASSERT(!context->msg_cframe->delivery_in_progress);
	  XP_ASSERT(!context->msg_cframe->attachment_in_progress);
	  context->msg_cframe->attachment_in_progress = TRUE;
	  FE_UpdateToolbar (context);
	  msg_DownloadAttachments (context, 0, downloads,
							   msg_download_attachments_done);
	  XP_FREE(downloads);
	}
}

static void 
msg_download_attachments_done (MWContext *context, 
							   void *fe_data,
							   int status, const char *error_message,
							   struct MSG_AttachedFile *attachments)
{
  MSG_ComposeFrame *f = context->msg_cframe;
  int old_count = 0;
  int new_count = 0;
  struct MSG_AttachedFile *tmp;
  MSG_AttachedFile *newd;


  XP_ASSERT (context->type == MWContextMessageComposition);
  if (context->type != MWContextMessageComposition) goto FAIL;
  XP_ASSERT (f);
  if (!f) goto FAIL;

  XP_ASSERT(!context->msg_cframe->delivery_in_progress);
  if (context->msg_cframe->attachment_in_progress)
	{
	  context->msg_cframe->attachment_in_progress = FALSE;
	  FE_UpdateToolbar (context);
	}

  if (status < 0)
	goto FAIL;

  if (!f->attach_data) goto FAIL;


  if (f->attached_files)
	for (tmp = f->attached_files; tmp->orig_url; tmp++)
	  old_count++;
  if (attachments)
	for (tmp = attachments; tmp->orig_url; tmp++)
	  new_count++;

  if (old_count + new_count == 0)
	goto FAIL;
  newd = (MSG_AttachedFile *)
	XP_REALLOC(f->attached_files,
			   ((old_count + new_count + 1)
				* sizeof(MSG_AttachedFile)));

  if (!newd)
	{
	  status = MK_OUT_OF_MEMORY;
	  error_message = XP_GetString(status);
	  goto FAIL;
	}
  f->attached_files = newd;
  XP_MEMCPY(newd + old_count,
			attachments,
			sizeof(MSG_AttachedFile) * (new_count + 1));

  return;

 FAIL:
  XP_ASSERT(status < 0);
  if (error_message)
	{
	  FE_Alert(context, error_message);
	}
  else if (status != MK_INTERRUPTED)
	{
	  char *errmsg;
	  errmsg = PR_smprintf(XP_GetString(MK_COMMUNICATIONS_ERROR), status);
	  if (errmsg)
		{
		  FE_Alert(context, errmsg);
		  XP_FREE(errmsg);
		}
	}

  /* Since we weren't able to store it, ditch the files and the strings
	 describing them. */
  msg_delete_attached_files(attachments);
}


/* How many implementations of this are there now?  4? */
static void
msg_mid_truncate_string (const char *input, char *output, int max_length)
{
  int L = XP_STRLEN (input);
  if (L <= max_length)
    {
      XP_MEMCPY (output, input, L+1);
    }
  else
    {
      int mid = (max_length - 3) / 2;
      char *tmp = 0;
      if (input == output)
		{
		  tmp = output;
		  output = (char *) XP_ALLOC (max_length + 1);
		  *tmp = 0;
		  if (!output) return;
		}
      XP_MEMCPY (output, input, mid);
      XP_MEMCPY (output + mid, "...", 3);
      XP_MEMCPY (output + mid + 3, input + L - mid, mid + 1);

      if (tmp)
		{
		  XP_MEMCPY (tmp, output, max_length + 1);
		  XP_FREE (output);
		}
    }
}


char *
MSG_GetAttachmentString(MWContext* context)
{
  /* #### bug 8688 */
  char *final_result;
  MSG_AttachmentData *tmp;
  MSG_ComposeFrame* f = context->msg_cframe;
  int count;
  int chars_per_attachment;
  int default_field_width = 63; /* 72 - some space for the word "Attachment" */

  XP_ASSERT (context->type == MWContextMessageComposition);
  if (context->type != MWContextMessageComposition) return NULL;
  XP_ASSERT (f);
  if (!f) return NULL;

  count = 0;
  for (tmp = f->attach_data; tmp && tmp->url ; tmp++) count++;
  if (count <= 0) return 0;

  chars_per_attachment = (default_field_width - (count * 2)) / count;
  if (chars_per_attachment < 15)
	chars_per_attachment = 15;

  final_result = (char *) XP_ALLOC(count * (chars_per_attachment + 3) + 60);
  if (!final_result) return 0;
  *final_result = 0;
  sprintf (final_result, "(%d) ", count);

  for (tmp = f->attach_data ; tmp && tmp->url ; tmp++)
	{
	  const char *url = tmp->real_name ? tmp->real_name : tmp->url;
	  const char *ptr = XP_STRCHR(url, ':');
	  char *result = 0;

	  if (!ptr)
		{
		  /* No colon?  Must be a file name. */
		  ptr = url;
		  goto DO_FILE;
		}

	  if (!XP_STRNCMP(url, "news:", 5) ||
		  !XP_STRNCMP(url, "snews:", 6) ||
		  !XP_STRNCMP(url, "mailbox:", 8))
		{
		  MWContext *c = XP_FindContextOfType (0,
											   (XP_STRNCMP(url, "mailbox:", 8)
												? MWContextNews
												: MWContextMail));
		  if (c &&
			  c->msgframe &&
			  c->msgframe->msgs &&
			  c->msgframe->msgs->msgs &&
			  c->msgframe->msgs->message_id_table)
			{
			  /* OK!  This is a news/mail message, and there is a news/mail
				 context around.  So go look for it in the currently-displayed
				 thread list.
			   */
			  struct MSG_ThreadEntry *msg;
			  char *s = XP_STRDUP(ptr + 1);
			  char *id = s;
			  if (!s) goto DONE;
			  if (!XP_STRNCMP(url, "mailbox:", 8))
				{
				  char *s2;
				  s2 = XP_STRSTR(id, "?id=");
				  if (s2) id = s2;
				  else id = XP_STRSTR(id, "&id=");
				  if (!id) goto DONE;
				  id += 4;
				  s2 = XP_STRCHR(id, '&');
				  if (s2) *s2 = 0;
				  NET_UnEscape (id);
				}
			  else
				{
				  char *s2 = XP_STRRCHR(id, '/');
				  if (s2) id = s2+1;
				  s2 = XP_STRCHR(id, '?');
				  if (s2) *s2 = 0;
				  NET_UnEscape (id);
				}

			  msg = (MSG_ThreadEntry *)
				XP_Gethash (c->msgframe->msgs->message_id_table, id, 0);
			  XP_FREE(s);
			  if (!msg)
				goto DO_FOLDER;

			  /* Found an entry in the thread list!  So now we know the
				 subject.  Display that. */
			  s = c->msgframe->msgs->string_table[msg->subject];
			  if (s && *s)
				{
				  char *conv_subject;
  				  conv_subject = IntlDecodeMimePartIIStr(s, context->win_csid, FALSE);
  				  if (conv_subject == NULL)
         		    conv_subject = (char *) s;
				  result = (char *) XP_ALLOC (XP_STRLEN(conv_subject) + 10);
				  if (!result) goto DONE;
				  XP_STRCPY(result, "\"");
				  if (msg->flags & MSG_FLAG_HAS_RE)
					XP_STRCAT(result, "Re: ");
				  XP_STRCAT(result, conv_subject);
				  XP_STRCAT(result, "\"");
				  if (conv_subject != s)
				    XP_FREE(conv_subject);
				  goto DONE;
				}
			}

		DO_FOLDER:

#if 0
		  /* Ok, we couldn't find this message in the thread list, maybe
			 because a different folder is now selected, or because the
			 mail/news window has been deleted.  So, display a folder name
			 instead.
		   */
		  if (!XP_STRNCMP(url, "mailbox:", 8))
			{
			}
#endif

		  goto DONE;
		}

	  /* Ok, so it must be something vaguely file-name-like.
		 Look for a slash.
	   */
	DO_FILE:
	  {
		char *ptr2 = XP_STRDUP(ptr);
		char *s;
		if (!ptr2) goto DONE;
		if ((s = XP_STRCHR(ptr2, '?'))) *s = 0;
		if ((s = XP_STRCHR(ptr2, '#'))) *s = 0;
		s = XP_STRRCHR(ptr2, '/');
		if(!s)
		  {
			XP_FREE(ptr2);
			goto DONE;
		  }
		s++;
		if (!*s || !strcasecomp(s,"index.html") || !strcasecomp(s,"index.htm"))
		  {
			/* This had a useless file name; take the last directory name. */
			char *s2 = s-1;
			if (*s2 == '/') s2--;
			while (s2 > ptr2 && *s2 != '/')
			  s2--;
			if (*s2 == ':' || *s2 == '/')
			  s2++;
			result = (char *) XP_ALLOC (s - s2 + 1);
			XP_MEMCPY (result, s2, s - s2);
			result[s - s2] = 0;
		  }
		else
		  {
			/* The file name is ok; use it. */
			result = XP_STRDUP (s);
		  }
		NET_UnEscape (result);
		XP_FREE(ptr2);
		goto DONE;
	  }

	DONE:
	  if (tmp != f->attach_data)
		{
		  XP_STRCAT(final_result, "; ");
		}

	  if (!result)
		{
		  if (!XP_STRNCMP(url, "news:", 5) ||
			  !XP_STRNCMP(url, "snews:", 6) ||
			  !XP_STRNCMP(url, "mailbox:", 8))
			result = XP_STRDUP("<message>");
		  else
			result = XP_STRDUP(url);
		  if (!result)
			break;
		}

	  msg_mid_truncate_string(result,
							  final_result + XP_STRLEN(final_result),
							  chars_per_attachment);
	  XP_FREE(result);
	}

  return final_result;
}


char*
MSG_UpdateHeaderContents(MWContext* context, MSG_HEADER_SET which_header,
						 const char* value)
{
  MWContext* addressbook_context;
  switch (which_header) {
	case MSG_TO_HEADER_MASK:
	case MSG_CC_HEADER_MASK:
	case MSG_BCC_HEADER_MASK:
    case MSG_REPLY_TO_HEADER_MASK:
	  addressbook_context = FE_GetAddressBook(context, FALSE);
	  if (addressbook_context) {
		return BM_ExpandHeaderString(addressbook_context, value, FALSE);
	  }
  default:
	return NULL;
  }
}

void
MSG_SetHeaderContents (MWContext *context, MSG_HEADER_SET which_header,
					   const char *value)
{
  char **header;
  XP_ASSERT (context->type == MWContextMessageComposition);
  if (context->type != MWContextMessageComposition) return;
  XP_ASSERT (context->msg_cframe);
  if (!context->msg_cframe) return;

  if (which_header == MSG_ATTACHMENTS_HEADER_MASK) return;

  switch (which_header)
	{
	case MSG_FROM_HEADER_MASK:
	  header = &context->msg_cframe->from; break;
	case MSG_REPLY_TO_HEADER_MASK:
	  header = &context->msg_cframe->reply_to; break;
	case MSG_TO_HEADER_MASK:
	  header = &context->msg_cframe->to; break;
	case MSG_CC_HEADER_MASK:
	  header = &context->msg_cframe->cc; break;
	case MSG_BCC_HEADER_MASK:
	  header = &context->msg_cframe->bcc; break;
	case MSG_FCC_HEADER_MASK:
	  header = &context->msg_cframe->fcc; break;
	case MSG_NEWSGROUPS_HEADER_MASK:
	  header = &context->msg_cframe->newsgroups; break;
	case MSG_FOLLOWUP_TO_HEADER_MASK:
	  header = &context->msg_cframe->followup_to; break;
	case MSG_SUBJECT_HEADER_MASK:
	  header = &context->msg_cframe->subject; break;
	default:
	  XP_ASSERT (0);
	  return;
	}
  if (value == NULL) value = "";
  if (*header == NULL || XP_STRCMP(*header, value) != 0) {
	FREEIF (*header);
	*header = XP_STRDUP (value ? value : "");
	if ((which_header & (MSG_TO_HEADER_MASK |
						 MSG_CC_HEADER_MASK |
						 MSG_BCC_HEADER_MASK)) == 0) {
	  /* A non-addressing header has changed, so the message has changed, so
         make sure that it has a fresh message-id when we send it. */
	  MSG_ClearMessageID(context);
	}
  }
}

static void
msg_set_composition_window_title (MWContext *context)
{
  char *s;
  XP_ASSERT(context->type == MWContextMessageComposition);
  if (context->type != MWContextMessageComposition) return;

  if (context->msg_cframe->subject && *context->msg_cframe->subject)
	s = context->msg_cframe->subject;
  else if (context->msg_cframe->to && *context->msg_cframe->to)
	s = context->msg_cframe->to;
  else if (context->msg_cframe->newsgroups && *context->msg_cframe->newsgroups)
	s = context->msg_cframe->newsgroups;
  else
	s =XP_GetString(MK_MSG_MSG_COMPOSITION);

  FE_SetDocTitle (context, s);
}



void
MSG_MessageBodyEdited(MWContext* context)
{
  XP_ASSERT(context && context->msg_cframe);
  if (context && context->msg_cframe) {
	context->msg_cframe->body_edited_p = TRUE;
	MSG_ClearMessageID(context); /* The message body has changed, so the
									message has changed, so make sure that it
									has a fresh message-id when we send it. */
  }
}


static XP_Bool
msg_sanity_check_message (MWContext *context)
{
  XP_Bool result = TRUE;
  XP_ASSERT (context->type == MWContextMessageComposition);
  if (context->type != MWContextMessageComposition) return FALSE;
  XP_ASSERT (context->msg_cframe);
  if (!context->msg_cframe) return FALSE;

  if (context->msg_cframe->cited_p &&
	  !context->msg_cframe->body_edited_p &&
	  context->msg_cframe->attach_data &&
	  context->msg_cframe->attach_data[0].url &&
	  context->msg_cframe->default_url &&
	  !XP_STRCMP (context->msg_cframe->attach_data[0].url,
				  context->msg_cframe->default_url)) {
	/* They have quoted a document and not edited it, and also attached the
	   same document.  Whine about it. */
	result = FE_Confirm(context, XP_GetString(MK_MSG_DOUBLE_INCLUDE));
	if (!result) return result;
  }

#if 0
  if ((context->msg_cframe->cited_p &&
	   !context->msg_cframe->body_edited_p)
	  /* They have quoted a document and have not edited it before sending.
		 This means that the entire document is sitting there as a citation;
		 offer to convert it to an attachment instead, since that preserves
		 more information without increasing the size of the message. */

	  || (context->msg_cframe->cited_p &&
		  context->msg_cframe->attach_data &&
		  context->msg_cframe->attach_data[1] &&
		  context->msg_cframe->attach_data[0]->url &&
		  context->msg_cframe->default_url &&
		  !XP_STRCMP (context->msg_cframe->attach_data[0]->url,
					  context->msg_cframe->default_url))
	  /* They have quoted a document and have *also* attached it, so it's
		 in there twice.  Offer to throw one away. */
	  )
	{
	  int answer = FE_BogusQuotationQuery (context, FALSE);
	  switch (answer)
		{
		case 0:
		  return FALSE;
		  break;
		case 1:
		  /* protect me from myself */
		  
		  break;
		case 2:
		  /* let me be a loser */
		  return TRUE;
		  break;
		default:
		  XP_ASSERT (0);
		  return FALSE;
		  break;
		}
	}
#endif

  if (!context->msg_cframe->body_edited_p &&
	  (!context->msg_cframe->attach_data ||
	   !context->msg_cframe->attach_data[0].url))
	{
	  /* This message has no attachments, and the body has not been edited.
		 Ask the user if that's really what they want, to prevent them from
		 accidentally sending an empty message.
	   */
	  result = FE_Confirm (context, XP_GetString(MK_MSG_EMPTY_MESSAGE));
	  if (!result) return result;
	}


  /* If they didn't type a subject, pop up a dialog asking them to provide
	 one.  If they don't, or take the default, we tolerate that. */
  {
	char *subject = context->msg_cframe->subject;
	if (subject)
	  while (XP_IS_SPACE (*subject))
		subject++;
	if (!subject || !*subject)
	  {
		subject = FE_PromptMessageSubject (context);
		if (!subject)
		  return FALSE;  /* user hit cancel */
		/* Else, user provided a subject; perhaps the default, or perhaps
		   an empty string. */
		FREEIF (context->msg_cframe->subject);
		context->msg_cframe->subject = subject;
	  }
  }


  /* If this message is being sent non-encrypted, then check to see if any of
	 the attached documents are messages that were decrypted at attach-time
	 (that is, msg-1 was encrypted for person-A; person-A creates msg-2, in
	 which he forwards a decrypted-version of msg-1 to person-B.)  In this
	 case, the safe thing is for msg-2 to itself be encrypted (for person-B);
	 anything else is a reduction in the overall level of security.  Which
	 might be ok, but the user needs to be warned of it.
   */
  if (!context->msg_cframe->encrypt_p &&
	  context->msg_cframe->attached_files &&
	  context->msg_cframe->attached_files[0].orig_url)
	{
	  XP_Bool blarg = TRUE;
	  PREF_GetBoolPref("mail.warn_forward_encrypted", &blarg);
	  if (blarg)
		{
		  int i;
		  XP_Bool any_decrypted_p = FALSE;
		  for (i = 0;
			   context->msg_cframe->attached_files[i].orig_url;
			   i++)
			{
			  if (context->msg_cframe->attached_files[i].decrypted_p)
				{
				  any_decrypted_p = TRUE;
				  break;
				}
			}

		  if (any_decrypted_p)
			{
			  result = FE_Confirm (context,
						   XP_GetString(MK_MSG_FORWARDING_ENCRYPTED_WARNING));
			  if (!result) return result;
			}
		}
	}

  return TRUE;
}


static void
msg_destroy_composition_context (MWContext *context)
{
  if (context->msg_cframe)
	{
	  /* delete tmp files, and free strings. */
	  msg_delete_attached_files (context->msg_cframe->attached_files);

	  FREEIF (context->msg_cframe->newspost_url);
	  FREEIF (context->msg_cframe->references);
	  FREEIF (context->msg_cframe->default_url);
	  FREEIF (context->msg_cframe->from);
	  FREEIF (context->msg_cframe->to);
	  FREEIF (context->msg_cframe->reply_to);
	  FREEIF (context->msg_cframe->cc);
	  FREEIF (context->msg_cframe->bcc);
	  FREEIF (context->msg_cframe->fcc);
	  FREEIF (context->msg_cframe->newsgroups);
	  FREEIF (context->msg_cframe->followup_to);
	  FREEIF (context->msg_cframe->subject);
	  FREEIF (context->msg_cframe->message_id);
	  msg_free_attachment_list(context, context->msg_cframe->attach_data);
	  XP_FREE (context->msg_cframe);
	  context->msg_cframe = 0;
	}
  FE_DestroyMailCompositionContext (context);
}

static void
msg_delivery_done_cb (MWContext *context, void *fe_data,
					  int status, const char *error_message)
{
  XP_ASSERT (context->type == MWContextMessageComposition);
  if (context->type != MWContextMessageComposition) return;
  XP_ASSERT (context->msg_cframe);
  if (!context->msg_cframe) return;
  context->msg_cframe->status = status;

  XP_ASSERT(!context->msg_cframe->attachment_in_progress);
  if (context->msg_cframe->delivery_in_progress)
	{
	  context->msg_cframe->delivery_in_progress = FALSE;
	  FE_UpdateToolbar (context);
	}

  if (status < 0)
    {
	  if (error_message)
		{
		  FE_Alert(context, error_message);
		}
	  else if (status != MK_INTERRUPTED)
		{
		  char *errmsg;
		  errmsg = PR_smprintf(XP_GetString(MK_COMMUNICATIONS_ERROR), status);
		  if (errmsg)
			{
			  FE_Alert(context, errmsg);
			  XP_FREE(errmsg);
			}
		}
    }
  else
	{
	  /* #### mark the parent message as replied */
	}
}

void
MSG_MailCompositionAllConnectionsComplete (MWContext *context)
{
  XP_ASSERT (context->type == MWContextMessageComposition);
  if (context->type != MWContextMessageComposition) return;
  XP_ASSERT (context->msg_cframe);
  if (!context->msg_cframe) return;

  /* This may be redundant, I'm not sure... */
  if (context->msg_cframe->delivery_in_progress)
	{
	  context->msg_cframe->delivery_in_progress = FALSE;
	  FE_UpdateToolbar (context);
	}
  if (context->msg_cframe->attachment_in_progress)
	{
	  context->msg_cframe->attachment_in_progress = FALSE;
	  FE_UpdateToolbar (context);
	}

  if (context->msg_cframe->status >= 0)
	{
	  msg_destroy_composition_context (context);
	}
}


void
MSG_CleanupMessageCompositionContext(MWContext *context)
{
  XP_ASSERT (context->type == MWContextMessageComposition);
  if (context->type != MWContextMessageComposition) return;
  if (!context->msg_cframe) return;
  msg_InterruptContext (context, FALSE);
  if (context->msg_cframe->textContext != NULL)
  {
	msg_InterruptContext(context->msg_cframe->textContext, TRUE);
  }
  msg_destroy_composition_context (context);
}


static void
msg_check_expansion(MWContext* context, char** header)
{
  MWContext* addressbook_context = FE_GetAddressBook(context, FALSE);
  char* newvalue;
  if (addressbook_context) {
	newvalue = BM_ExpandHeaderString(addressbook_context, *header, TRUE);
	if (newvalue) {
	  XP_FREE(*header);
	  *header = newvalue;
	}
  }
}


XP_Bool
msg_DeliveryInProgress (MWContext *context)
{
  XP_ASSERT(context && context->type == MWContextMessageComposition);
  if (!context || !context->type == MWContextMessageComposition ||
	  !context->msg_cframe)
	return FALSE;
  /* Disable the UI if delivery, attachment loading, or quoting is
	 in progress. */
  return (context->msg_cframe->delivery_in_progress ||
		  context->msg_cframe->attachment_in_progress ||
		  (context->msg_cframe->quoteurl != 0));
}


int
msg_DeliverMessage (MWContext *context, XP_Bool queue_p)
{
  char *body = 0;
  uint32 body_length = 0;
  MSG_FontCode *font_changes = 0;
  int attachment_count = 0;
  int status = 0;
  XP_Bool digest_p = FALSE;

  XP_ASSERT (context->type == MWContextMessageComposition);
  if (context->type != MWContextMessageComposition) return -1;
  XP_ASSERT (context->msg_cframe);
  if (!context->msg_cframe) return -1;

  msg_check_expansion(context, &context->msg_cframe->to);
  msg_check_expansion(context, &context->msg_cframe->cc);
  msg_check_expansion(context, &context->msg_cframe->bcc);

  if (!msg_sanity_check_message (context)) {
	/* Sanity check has rejected this message.  Note that it has also taken
	   care of all dialogs with the user telling him about the problems,
	   so we don't want to return an error code; we just want to return now
	   and do nothing. */
	return 0;
  }

  status = FE_GetMessageBody (context, &body, &body_length, &font_changes);
  if (status < 0) return status;

  for (attachment_count = 0;
	   context->msg_cframe->attach_data &&
		 context->msg_cframe->attach_data[attachment_count].url;
	   attachment_count++)
	;

#if 0

  /* Turns out that having a text/plain part inside a multipart/digest
	 is discouraged.  Instead, we should be doing
	   multipart/mixed
	     text/plain
		 multipart/digest
		   msg1
		   msg2
	 but that's more work, so not now...
   */

  if (context->msg_cframe->attach_data &&
	  context->msg_cframe->attach_data[0].url &&
	  context->msg_cframe->attach_data[1].url) {
	MSG_AttachmentData* s;
	digest_p = TRUE;
	for (s = context->msg_cframe->attach_data ; s->url ; s++) {
	  /* When there are attachments, start out assuming it is a digest, and
		 then decide that it is not if any of the attached URLs are not mail or
		 news messages. */
	  if (XP_STRNCMP(s->url, "news:", 5) != 0 &&
		  XP_STRNCMP(s->url, "snews:", 6) != 0 &&
		  XP_STRNCMP(s->url, "mailbox:", 8) != 0) {
		digest_p = FALSE;
	  }
	}
  }
#endif /* 0 */

  XP_ASSERT(!context->msg_cframe->attachment_in_progress);
  XP_ASSERT(!context->msg_cframe->delivery_in_progress);
  context->msg_cframe->delivery_in_progress = TRUE;
  FE_UpdateToolbar (context);

  if (context->msg_cframe->message_id == NULL) {
	context->msg_cframe->message_id = msg_generate_message_id();
	context->msg_cframe->duplicate_post_p = FALSE;
  } else {
	context->msg_cframe->duplicate_post_p = TRUE;
  }

  /* Reset this, so that old errors don't stick around. */
  context->msg_cframe->security_delivery_error = 0;

  msg_StartMessageDeliveryWithAttachments (context, NULL,
							context->msg_cframe->from,
							context->msg_cframe->reply_to,
							context->msg_cframe->to,
							context->msg_cframe->cc,
							context->msg_cframe->bcc,
							context->msg_cframe->fcc,
							context->msg_cframe->newsgroups,
							context->msg_cframe->followup_to,
#ifdef DEBUG_jwz
                            /* check for the organization late (before send,
                               instead of just after window creation) because
                               the hairball multi-account kludge changes it.
                             */
							FE_UsersOrganization(),
#else
							context->msg_cframe->organization,
#endif
						    context->msg_cframe->message_id,
							context->msg_cframe->newspost_url,
							context->msg_cframe->subject,
							context->msg_cframe->references,
							context->msg_cframe->other_random_headers,
							digest_p, FALSE, queue_p,
							context->msg_cframe->encrypt_p,
							context->msg_cframe->sign_p,
							(context->msg_cframe->markup_p
							 ? TEXT_HTML : TEXT_PLAIN),
							body, body_length,
							context->msg_cframe->attached_files,
							msg_delivery_done_cb);
  FE_DoneWithMessageBody(context, body, body_length);
  return 0;
}


void
MSG_SetDefaultHeaderContents (MWContext *context, MSG_HEADER_SET header,
							  const char *default_value, XP_Bool news_p)
{
  char **header1, **header2;
  switch (header)
	{
	case MSG_REPLY_TO_HEADER_MASK:
	  header1 = &msg_default_mail_headers.reply_to;
	  header2 = &msg_default_news_headers.reply_to;
	  break;
	case MSG_BCC_HEADER_MASK:
	  header1 = !news_p ? &msg_default_mail_headers.bcc : 0;
	  header2 =  news_p ? &msg_default_news_headers.bcc : 0;
	  break;
	case MSG_FCC_HEADER_MASK:
	  header1 = !news_p ? &msg_default_mail_headers.fcc : 0;
	  header2 =  news_p ? &msg_default_news_headers.fcc : 0;

	  default_value = msg_check_for_losing_fcc(context, default_value);
	  break;
	default:
	  XP_ASSERT (0);
	  return;
	}
  if (header1) FREEIF (*header1);
  if (header2) FREEIF (*header2);
  if (header1 && default_value) *header1 = XP_STRDUP (default_value);
  if (header2 && default_value) *header2 = XP_STRDUP (default_value);
}

const char*
msg_GetDefaultFcc(XP_Bool news_p)
{
  return news_p ? msg_default_news_headers.fcc : msg_default_mail_headers.fcc;
}


void
MSG_SetDefaultBccSelf(MWContext* context, XP_Bool includeself, XP_Bool news_p)
{
  if (news_p) msg_default_news_headers.bcc_self = includeself;
  else msg_default_mail_headers.bcc_self = includeself;
}


XP_Bool
MSG_IsDuplicatePost(MWContext* context) {
  if (context->msg_cframe) return context->msg_cframe->duplicate_post_p;
  else return FALSE;
}

void 
MSG_ClearMessageID(MWContext* context)
{
  if (context->msg_cframe) {
	FREEIF(context->msg_cframe->message_id);
  }
}

const char*
MSG_GetMessageID(MWContext* context)
{
  if (context->msg_cframe) {
	return context->msg_cframe->message_id;
  }
  return NULL;
}


void
msg_SetCompositionSecurityError(MWContext *context, int status)
{
  if (status >= 0) return;
  XP_ASSERT(context &&
			context->type == MWContextMessageComposition &&
			context->msg_cframe);
  if (!context || !context->msg_cframe) return;

  XP_ASSERT(context->msg_cframe->delivery_in_progress);

  if (context->msg_cframe->security_delivery_error >= 0)
	context->msg_cframe->security_delivery_error = status;
}


/* Given a message composition window, this returns the current set of
   recipients and selected crypto options.  If a delivery has already
   been attempted and failed for crypto-related reasons, then the
   `delivery_error_ret' will be the specific error code that the crypto
   library returned (otherwise, it will be 0.)  All returned strings
   should be freed by the caller.
 */
void
MimeGetCompositionSecurityInfo(MWContext *context,
							   char **sender_ret,
							   char **mail_recipients_ret,
							   char **news_recipients_ret,
							   XP_Bool *encrypt_p_ret,
							   XP_Bool *sign_p_ret,
							   int *delivery_error_ret)
{
  XP_ASSERT(context &&
			context->type == MWContextMessageComposition &&
			context->msg_cframe);

  *sender_ret = 0;
  *mail_recipients_ret = 0;
  *news_recipients_ret = 0;
  *encrypt_p_ret = FALSE;
  *sign_p_ret = FALSE;
  *delivery_error_ret = 0;

  if (!context || !context->msg_cframe) return;

  *sender_ret = XP_STRDUP(context->msg_cframe->from);

  *mail_recipients_ret =
	(char *) XP_ALLOC((context->msg_cframe->to
					   ? XP_STRLEN(context->msg_cframe->to) : 0) +
					  (context->msg_cframe->cc
					   ? XP_STRLEN(context->msg_cframe->cc) : 0) +
					  (context->msg_cframe->bcc
					   ? XP_STRLEN(context->msg_cframe->bcc) : 0) +
					  10);
  if (!*mail_recipients_ret) return;

  **mail_recipients_ret = 0;
  if (context->msg_cframe->to && *context->msg_cframe->to)
	XP_STRCPY(*mail_recipients_ret, context->msg_cframe->to);

  if (context->msg_cframe->cc && *context->msg_cframe->cc)
	{
	  if (**mail_recipients_ret)
		XP_STRCAT(*mail_recipients_ret, ", ");
	  XP_STRCAT(*mail_recipients_ret, context->msg_cframe->cc);
	}

  if (context->msg_cframe->bcc && *context->msg_cframe->bcc)
	{
	  if (**mail_recipients_ret)
		XP_STRCAT(*mail_recipients_ret, ", ");
	  XP_STRCAT(*mail_recipients_ret, context->msg_cframe->bcc);
	}

  *news_recipients_ret = XP_STRDUP(context->msg_cframe->newsgroups ?
								   context->msg_cframe->newsgroups : "");
  *encrypt_p_ret = context->msg_cframe->encrypt_p;
  *sign_p_ret = context->msg_cframe->sign_p;
  *delivery_error_ret = context->msg_cframe->security_delivery_error;
}
