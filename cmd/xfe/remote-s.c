/* -*- Mode:C; tab-width: 8 -*-
   remote.c --- remote control of netscape.
   Copyright © 1996 Netscape Communications Corporation, all rights reserved.
   Created: Jamie Zawinski <jwz@netscape.com>, 24-Dec-94.
 */

#include "mozilla.h"
#include "xfe.h"

#define progname fe_progname
#define mozilla_remote_init_atoms fe_init_atoms
#define mozilla_remote_commands fe_RemoteCommands
static const char *expected_mozilla_version = fe_version;
#include "remote.c"


/* server side */

static void fe_property_change_action (Widget widget, XEvent *event,
				       String *av, Cardinal *ac);

static char *fe_server_handle_command (Display *dpy, Window window,
				       XEvent *event, char *command);

static XtTranslations fe_prop_translations;

void
fe_InitRemoteServer (Display *dpy)
{
  static Boolean done = False;
  static XtActionsRec actions [] =
    { { "HandleMozillaCommand", fe_property_change_action } };

  if (done) return;
  done = True;
  fe_init_atoms (dpy);

  XtAppAddActions (fe_XtAppContext, actions, countof (actions));

  fe_prop_translations =
    XtParseTranslationTable ("<PropertyNotify>: HandleMozillaCommand()");
}

void
fe_InitRemoteServerWindow (MWContext *context)
{
  Widget widget = CONTEXT_WIDGET (context);
  Display *dpy = XtDisplay (widget);
  Window window = XtWindow (widget);
  XWindowAttributes attrs;
  unsigned char *data = (unsigned char *) fe_version;

  XtOverrideTranslations (widget, fe_prop_translations);

#ifndef DEBUG_jwz
  /* don't let ns3 handle -remote requests at all. */
  XChangeProperty (dpy, window, XA_MOZILLA_VERSION, XA_STRING,
		   8, PropModeReplace, data, strlen (data));
#endif /* DEBUG_jwz */

  XGetWindowAttributes (dpy, window, &attrs);
  if (! (attrs.your_event_mask & PropertyChangeMask))
    XSelectInput (dpy, window, attrs.your_event_mask | PropertyChangeMask);
}


static void
fe_server_handle_property_change (Display *dpy, Window window, XEvent *event)
{
  if (event->xany.type == PropertyNotify &&
      event->xproperty.state == PropertyNewValue &&
      event->xproperty.window == window &&
      event->xproperty.atom == XA_MOZILLA_COMMAND)
    {
      int result;
      Atom actual_type;
      int actual_format;
      unsigned long nitems, bytes_after;
      unsigned char *data = 0;

      result = XGetWindowProperty (dpy, window, XA_MOZILLA_COMMAND,
				   0, (65536 / sizeof (long)),
				   True, /* atomic delete after */
				   XA_STRING,
				   &actual_type, &actual_format,
				   &nitems, &bytes_after,
				   &data);
      if (result != Success)
	{
	  fprintf (stderr, "%s: unable to read " MOZILLA_COMMAND_PROP
		   " property\n",
		   fe_progname);
	  return;
	}
      else if (!data || !*data)
	{
	  fprintf (stderr, "%s: invalid data on " MOZILLA_COMMAND_PROP
		   " of window 0x%x.\n",
		   fe_progname, (unsigned int) window);
	  return;
	}
      else
	{
	  char *response =
	    fe_server_handle_command (dpy, window, event, (char *) data);
	  if (! response) abort ();
	  XChangeProperty (dpy, window, XA_MOZILLA_RESPONSE, XA_STRING,
			   8, PropModeReplace, response, strlen (response));
	  free (response);
	}

      if (data)
	XFree (data);
    }
#ifdef DEBUG_PROPS
  else if (event->xany.type == PropertyNotify &&
	   event->xproperty.window == window &&
	   event->xproperty.state == PropertyDelete &&
	   event->xproperty.atom == XA_MOZILLA_RESPONSE)
    {
      fprintf (stderr, "%s: (client has accepted the response on 0x%x.)\n",
	       fe_progname, (unsigned int) window);
    }
  else if (event->xany.type == PropertyNotify &&
	   event->xproperty.window == window &&
	   event->xproperty.atom == XA_MOZILLA_LOCK)
    {
      fprintf (stderr, "%s: (client has %s a lock on 0x%x.)\n",
	       fe_progname,
	       (event->xproperty.state == PropertyNewValue
		? "obtained" : "freed"),
	       (unsigned int) window);
    }
#endif /* DEBUG_PROPS */

}

static void
fe_property_change_action (Widget widget, XEvent *event,
			   String *av, Cardinal *ac)
{
  MWContext *context = fe_WidgetToMWContext (widget);
  if (! context) return;
  fe_server_handle_property_change (XtDisplay (CONTEXT_WIDGET (context)),
				    XtWindow (CONTEXT_WIDGET (context)),
				    event);
}

static char *
fe_server_handle_command (Display *dpy, Window window, XEvent *event,
			  char *command)
{
  char *name = 0;
  char **av = 0;
  int ac = 0;
  int avsize = 0;
  Boolean raise_p = True;
  int i;
  char *buf;
  char *buf2;
  int32 buf2_size;
  char *head, *tail;
  XtActionProc action = 0;
  Widget widget = XtWindowToWidget (dpy, window);
  MWContext *context = fe_WidgetToMWContext (widget);
  XP_Bool mail_or_news_required = FALSE;
  MWContextType required_type = (MWContextType) (~0);
  XP_Bool make_context_if_necessary = FALSE;
  XP_Bool done_once = FALSE;

  XP_ASSERT(context);

  buf = fe_StringTrim (strdup (command));
  buf2_size = strlen (buf) + 200;
  buf2 = (char *) malloc (buf2_size);

  head = buf;
  tail = buf;

  if (! widget)
    {
      PR_snprintf (buf2, buf2_size,
	    "509 internal error: unable to translate"
	    "window 0x%x to a widget",
	    (unsigned int) window);
      free (buf);
      return buf2;
    }

  while (1)
    if (*tail == '(' || isspace (*tail) || !*tail)
      {
	*tail = 0;
	tail++;
	name = fe_StringTrim (head);
	break;
      }
    else
      tail++;

  if (!name || !*name)
    {
      PR_snprintf (buf2, buf2_size, "500 unparsable command: %s", command);
      free (buf);
      return buf2;
    }

  for (i = 0; i < fe_CommandActionsSize; i++)
    if (!strcasecmp (name, fe_CommandActions [i].string))
      {
	name = fe_CommandActions [i].string;
	action = fe_CommandActions [i].proc;
	break;
      }

  if (!action)
    for (i = 0; i < fe_MailNewsActionsSize; i++)
      if (!strcasecmp (name, fe_MailNewsActions [i].string))
	{
	  name = fe_MailNewsActions [i].string;
	  action = fe_MailNewsActions [i].proc;

	  if (strstr(name, "Mail") || strstr(name, "Folder"))
	    required_type = MWContextMail;
	  else if (strstr(name, "News") || strstr(name, "Group") ||
		   strstr(name, "Host"))
	    required_type = MWContextNews;
	  else
	    {
	      required_type = (MWContextType) (~0);
	      mail_or_news_required = TRUE;
	    }

	  break;
	}

  if (!action)
    {
      PR_snprintf (buf2, buf2_size, "501 unrecognised command: %s", name);
      free (buf);
      return buf2;
    }

 AGAIN:

  /* If the required context-type doesn't match, find one that does.
   */
  if ((mail_or_news_required &&
       context->type != MWContextMail &&
       context->type != MWContextNews) ||
      (required_type != (MWContextType) (~0) &&
       required_type != context->type))
    {
      MWContext *oc = context;
      MWContextType type = (mail_or_news_required
			    ? MWContextMail
			    : required_type);
      context = XP_FindContextOfType (context, type);

      if (!context && make_context_if_necessary)
	context = fe_MakeWindow (XtParent(widget), oc, 0, NULL, type, FALSE);

      widget = context ? CONTEXT_WIDGET(context) : 0;
      window = widget ? XtWindow(widget) : 0;
      if (!window)
	{
	  PR_snprintf (buf2, buf2_size, "502 no appropriate window for %s",
		       (av ? av[0] : name));
	  free (buf);
	  if (av) free (av);
	  return buf2;
	}
    }

  if (!av)
    {
      avsize = 20;
      av = (char **) calloc (avsize, sizeof (char *));

      while (*tail == '(' || isspace (*tail))
	tail++;

      head = tail;
      while (1)
	{
	  if (*tail == ')' || *tail == ',' || *tail == 0)
	    {
	      char delim = *tail;
	      if (ac >= (avsize - 2))
		{
		  avsize += 20;
		  av = (char **) realloc (av, avsize * sizeof (char *));
		}
	      *tail = 0;

	      av [ac++] = fe_StringTrim (head);

	      if (delim != ',' && !*av[ac-1])
		ac--;
	      else if (!strcasecomp (av [ac-1], "noraise"))
		{
		  raise_p = False;
		  ac--;
		}
	      else if (!strcasecomp (av [ac-1], "raise"))
		{
		  raise_p = True;
		  ac--;
		}
#ifdef DEBUG_jwz
              /* If the argument "new-window" will be passed to the actual
                 command, then don't bother raising this window (but do
                 leave that arg in the list.) */
	      else if (!strcasecomp (av[ac-1], "new-window") ||
                       !strcasecomp (av[ac-1], "new_window") ||
                       !strcasecomp (av[ac-1], "newWindow") ||
                       !strcasecomp (av[ac-1], "new"))
		{
		  raise_p = False;
		}
#endif /* DEBUG_jwz */


	      head = tail+1;
	      if (delim != ',')
		break;
	    }
	  tail++;
	}

      av [ac++] = "remote";
    }


  /* If this is GetURL or something like it, make sure the context we pick
     matches the URL.
   */
  if (!done_once &&
      required_type == (MWContextType) (~0) &&
      strstr(name, "URL"))
    {
      const char *url = av[0];
      mail_or_news_required = FALSE;
      required_type = (MWContextType) (~0);
      if (MSG_RequiresMailWindow (url))
	required_type = MWContextMail;
      else if (MSG_RequiresNewsWindow (url))
	required_type = MWContextNews;
      else if (MSG_RequiresBrowserWindow (url))
	required_type = MWContextBrowser;
      /* Nothing to do for MSG_RequiresComposeWindow compose. */

      if (required_type != (MWContextType) (~0))
	{
	  done_once = TRUE;
	  make_context_if_necessary = TRUE;
	  goto AGAIN;
	}
    }


  if (raise_p)
    XMapRaised (dpy, window);

  {
    Cardinal ac2 = ac; /* why is this passed as a pointer??? */
    (*action) (widget, event, av, &ac2);
  }

  PR_snprintf (buf2, buf2_size, "200 executed command: %s(", name);
  for (i = 0; i < ac-1; i++)
    {
      strcat (buf2, av [i]);
      if (i < ac-2)
	strcat (buf2, ", ");
    }
  strcat (buf2, ")");

  free (av);

  free (buf);
  return buf2;
}
