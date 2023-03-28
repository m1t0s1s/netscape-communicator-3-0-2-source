/*
 * nullplugin.c
 *
 * Implementation of the null plugins for Unix.
 *
 * dp <dp@netscape.com>
 *
 */

#include <stdio.h>
#include <Xm/Xm.h>
#include <Xm/MessageB.h>
#include "npapi.h"
#include "nullplugin.h"

/* Global data */
MimeTypeElement *head = NULL;

static void
UnmanageChild_safe (Widget w)
{
  if (w) XtUnmanageChild (w);
}

static void
nullPlugin_destroy_cb(Widget widget, XtPointer closure, XtPointer call_data)
{
	XmAnyCallbackStruct *cb = (XmAnyCallbackStruct *) call_data;
	PluginInstance* This = (PluginInstance*) closure;
	This->dialog = NULL;
}

static void
nullPlugin_cb(Widget widget, XtPointer closure, XtPointer call_data)
{
	XmAnyCallbackStruct *cb = (XmAnyCallbackStruct *) call_data;
	PluginInstance* This = (PluginInstance*) closure;
	char *url;
	char *urlpage = PLUGINSPAGE_URL;

	switch (cb->reason) {
	    case XmCR_OK:
			XtUnmanageChild(This->dialog);
			if (This->pluginsPageUrl) urlpage = This->pluginsPageUrl;
			url = NPN_MemAlloc(strlen(urlpage)+
						1+	/* For ? */
						strlen(This->type)+
						1	/* For NULL */
				);
			if (url) {
				sprintf(url, "%s?%s", urlpage, This->type);
				NPN_GetURL(This->instance, url, "PluginRegistry");
				NPN_MemFree(url);
			}

			break;

	    case XmCR_CANCEL:
			XtUnmanageChild(This->dialog);
			break;
	}
}


void
showPluginDialog(Widget widget, XtPointer closure, XtPointer call_data)
{
	PluginInstance* This = (PluginInstance*) closure;
	Widget dialog;
	Arg av[20];
	int ac;
	XmString xmstr;
	char message[1024];

	if (!This) return;

	dialog = This->dialog;

	if (dialog) {
		/* The dialog is already available */
		XtManageChild(dialog);
		XMapRaised(XtDisplay(dialog), XtWindow(dialog));
		return;
	}

	/* Create the dialog */

	sprintf(message, MESSAGE, This->type);

	xmstr = XmStringCreateLtoR(message, XmSTRING_DEFAULT_CHARSET);

	ac = 0;
	XtSetArg (av[ac], XmNvisual, This->visual); ac++;
	XtSetArg (av[ac], XmNdepth, This->depth); ac++;
	XtSetArg (av[ac], XmNcolormap, This->colormap); ac++;
	XtSetArg (av[ac], XmNallowShellResize, TRUE); ac++;
	XtSetArg (av[ac], XmNdialogStyle, XmDIALOG_MODELESS); ac++;
	XtSetArg (av[ac], XmNdialogType, XmDIALOG_QUESTION); ac++;
	XtSetArg (av[ac], XmNdeleteResponse, XmUNMAP); ac++;
	XtSetArg (av[ac], XmNautoUnmanage, False); ac++;
	XtSetArg (av[ac], XmNmessageString, xmstr); ac++;
	dialog = XmCreateMessageDialog(This->button, "nullpluginDialog",
					av, ac);

	UnmanageChild_safe (XmMessageBoxGetChild (dialog,
						XmDIALOG_HELP_BUTTON));

	XtAddCallback(dialog, XmNokCallback, nullPlugin_cb, This);
	XtAddCallback(dialog, XmNcancelCallback, nullPlugin_cb, This);
	XtAddCallback(dialog, XmNdestroyCallback, nullPlugin_destroy_cb, This);

	XmStringFree(xmstr);

	This->dialog = dialog;
	XtManageChild(dialog);
}

/* MIMETypeList maintenance routines */

static Boolean
isEqual(NPMIMEType t1, NPMIMEType t2)
{
	return(strcmp(t1, t2) == 0);
}

static Boolean
isExist(MimeTypeElement **typelist, NPMIMEType type)
{
	MimeTypeElement *ele;

	if (typelist == NULL) return False;

	ele = *typelist;
	while (ele != NULL) {
		if (isEqual(ele->value, type))
			return True;
		ele = ele->next;
	}
	return False;
}

NPMIMEType
dupMimeType(NPMIMEType type)
{
	NPMIMEType mimetype = NPN_MemAlloc(strlen(type)+1);
	if (mimetype)
		strcpy(mimetype, type);
	return(mimetype);
}

int
addToList(MimeTypeElement **typelist, NPMIMEType type)
{
	MimeTypeElement *ele;

	if (!typelist) return(0);

	if (isExist(typelist, type)) return(0);

	ele = (MimeTypeElement *) NPN_MemAlloc(sizeof(MimeTypeElement));
	ele->value = dupMimeType(type);
	ele->next = *typelist;
	*typelist = ele;
	return(1);
}
