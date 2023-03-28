/* -*- Mode: C; tab-width: 4 -*-
   msgbiff.c --- handle "check for new mail" stuff.
   Copyright © 1995 Netscape Communications Corporation, all rights reserved.
   Created: Terry Weissman <terry@netscape.com>, 2-Nov-95.
 */

#include "msg.h"

extern int MK_OUT_OF_MEMORY;


static int biffInterval = 600;	/* Seconds to wait between biffs. */

static char* biffStatFile = NULL; /* File to stat; if NULL, use pop3. */


typedef struct MSG_BiffFrame {
  void* timer;					/* Timer that goes off next time we ought to
								   check for new mail. */
} MSG_BiffFrame;


void
MSG_SetBiffInterval(int seconds)
{
  MWContext* context;
  if (biffInterval != seconds) {
	biffInterval = seconds;
	context = XP_FindContextOfType(NULL, MWContextBiff);
	if (context && context->biff_frame) {
	  if (biffInterval > 0) {
		MSG_BiffCheckNow(context);
	  } else if (context->biff_frame->timer) {
		FE_ClearTimeout(context->biff_frame->timer);
		context->biff_frame->timer = NULL;
	  }
	}
  }
}

#ifdef XP_UNIX
void
MSG_SetBiffStatFile(const char* filename)
{
  MWContext* context;
  if (filename == biffStatFile) return;
  if (filename && biffStatFile && XP_STRCMP(filename, biffStatFile) == 0) {
	return;
  }
  /* We're definitely checking some different place for mail than we used to.
     We'd best set the FE into "unknown" state until we get a chance to check
	 the new place. */
  FE_UpdateBiff(MSG_BIFF_Unknown);

  FREEIF(biffStatFile);
  if (filename) biffStatFile = XP_STRDUP(filename);
  else biffStatFile = NULL;

  context = XP_FindContextOfType(NULL, MWContextBiff);
  if (context && context->biff_frame && biffInterval > 0) {
	MSG_BiffCheckNow(context);
  }
}
#endif


int
MSG_BiffInit(MWContext* context)
{
  XP_ASSERT(context && context->type == MWContextBiff);
  if (!context || context->type != MWContextBiff) return -1;
  XP_ASSERT(context->biff_frame == NULL); /* Catch duplicate initialization. */
  if (context->biff_frame) return -1;
  context->biff_frame = XP_NEW_ZAP(MSG_BiffFrame);
  if (!context->biff_frame) return MK_OUT_OF_MEMORY;
  if (biffInterval > 0) MSG_BiffCheckNow(context);
  return 0;
}


int
MSG_BiffCleanupContext(MWContext* context)
{
  XP_ASSERT(context && context->type == MWContextBiff && context->biff_frame);
  if (!context || context->type != MWContextBiff || !context->biff_frame) {
	return -1;
  }
  if (context->biff_frame->timer) {
	FE_ClearTimeout(context->biff_frame->timer);
	context->biff_frame->timer = NULL;
  }
  XP_FREE(context->biff_frame);
  context->biff_frame = NULL;
  return 0;
}


static void
msg_biff_timer(void* closure)
{
  MWContext* context = (MWContext*) closure;
  if (context->biff_frame) {
	context->biff_frame->timer = NULL;
  }
  MSG_BiffCheckNow(context);
}


static void
msg_biff_done(URL_Struct* url_struct, int status, MWContext *context)
{
  /* This used to say: */
  /* if (status < 0) FE_UpdateBiff(MSG_BIFF_Unknown); */
  /* But that doesn't seem like such a good idea.  For example, if we try
	 to do this biff check while another context is talking to the pop3 server,
	 we can get here. */

  NET_FreeURLStruct(url_struct);
}


void
MSG_BiffCheckNow(MWContext* context)
{
  extern char *msg_pop3_host;
  char* url;
  URL_Struct* url_struct;

  XP_ASSERT(context && context->type == MWContextBiff && context->biff_frame);
  if (!context || context->type != MWContextBiff || !context->biff_frame) {
	return;
  }
  if (context->biff_frame->timer) {
	FE_ClearTimeout(context->biff_frame->timer);
	context->biff_frame->timer = NULL;
  }
  if (biffInterval > 0) {
	context->biff_frame->timer = FE_SetTimeout(msg_biff_timer, context,
											   1000L * biffInterval);
  }

  if (biffStatFile) {
#ifdef XP_UNIX
	XP_StatStruct biffst;
	if (!XP_Stat(biffStatFile, &biffst, xpMailFolder) && biffst.st_size > 0) {
	  FE_UpdateBiff(MSG_BIFF_NewMail);
	} else {
	  FE_UpdateBiff(MSG_BIFF_NoMail);
	}
#else
	XP_ASSERT(0);
#endif
  } else {
	url = PR_smprintf("pop3://%s/?check", msg_pop3_host);
	if (!url) return;
	url_struct = NET_CreateURLStruct(url, NET_SUPER_RELOAD);
	XP_FREE(url);
	if (!url_struct) return;
	url_struct->internal_url = TRUE;
	msg_InterruptContext(context, FALSE);
	NET_GetURL(url_struct, FO_PRESENT, context, msg_biff_done);
  }
}
