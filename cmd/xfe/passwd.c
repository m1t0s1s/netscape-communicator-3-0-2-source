/* -*- Mode: C; tab-width: 8 -*-
   passwd.c --- reading passwords with Motif text fields.
   Copyright © 1994 Netscape Communications Corporation, all rights reserved.
   Created: Jamie Zawinski <jwz@netscape.com>, 21-Jul-94.
 */

#include "mozilla.h"
#include "xfe.h"

static void 
passwd_modify_cb (Widget text, XtPointer client_data, XtPointer call_data)
{
  XmTextVerifyCallbackStruct *vcb = (XmTextVerifyCallbackStruct *) call_data;
  char *plaintext = (char *) client_data;
  int deletion_length = vcb->endPos - vcb->startPos;
  int insertion_length = vcb->text->length;
  int L = strlen (plaintext);
  int i;
  
  if (vcb->reason != XmCR_MODIFYING_TEXT_VALUE)
    return;

  /* If a deletion occurred, clone it. */
  if (deletion_length > 0)
    {
      for (i = 0; i < (L + 1 - deletion_length); i++)
	{
	  plaintext [vcb->startPos+i] = plaintext[vcb->endPos+i];
	  if (! plaintext [vcb->startPos+i])
	    /* If we copied a null, we're done. */
	    break;
	}
      L -= deletion_length;
    }

  /* If an insertion occurred, open up space for it. */
  if (insertion_length > 0)
    {
      for (i = 0; i <= (L - vcb->startPos); i++)
	plaintext [L + insertion_length - i] = plaintext [L - i];
      L += insertion_length;

      /* Now fill in the opened gap. */
      for (i = 0; i < insertion_length; i++)
	plaintext [vcb->startPos + i] = vcb->text->ptr [i];
    }

  /* Now modify the text to insert stars. */
  for (i = 0; i < insertion_length; i++)
    vcb->text->ptr [i] = '*';
}

static void
passwd_destroy_cb (Widget text_field, XtPointer closure, XtPointer call_data)
{
  char *plaintext = 0;
  int i;
  XtVaGetValues (text_field, XmNuserData, &plaintext, 0);
  if (!plaintext) return;
  XtVaSetValues (text_field, XmNuserData, 0, 0);
  i = strlen (plaintext);
  while (i--) plaintext [i] = 0; /* paranoia about core files */
  free (plaintext);
}

void
fe_SetupPasswdText (Widget text_field, int max_length)
{
  char *plaintext = 0;
  if (max_length <= 0) abort ();
  XtVaGetValues (text_field, XmNuserData, &plaintext, 0);
  if (plaintext) return;    /* already initialized? */
  plaintext = (char *) calloc (max_length * 2, 1);
  XtAddCallback (text_field, XmNmodifyVerifyCallback, passwd_modify_cb,
		 plaintext);
  XtAddCallback (text_field, XtNdestroyCallback, passwd_destroy_cb, 0);
  XtVaSetValues (text_field,
		 XmNuserData, plaintext,
		 XmNmaxLength, max_length,
		 0);

  /*
   * make sure the international input method does not come up for this
   * widget, since we want to hide the password
   */
  XmImUnregister (text_field);
}

char *
fe_GetPasswdText (Widget text_field)
{
  char *plaintext = 0;
  XtVaGetValues (text_field, XmNuserData, &plaintext, 0);
  /* Return a copy to be analagous with GetValues of XmNvalue.
     The internal copy will be freed when the widget is destroyed. */
  return strdup (plaintext ? plaintext : "");
}
