/* -*- Mode: C; tab-width: 8 -*-
   printprogress.c --- I don't know what this does
   Copyright © 1995 Netscape Communications Corporation, all rights reserved.
 */

#include "mozilla.h"
#include "xfe.h"
#include <Xm/Label.h>
#include "GBar.h"

struct progress_widget_info {
    Widget printWin;
    Widget PrintStateLabel;		/* Printing: */
    Widget PrintDocumentField;		/* URL */
    Widget bar;
    Widget PrintStatusLabel;		/* text info */	
    Boolean cancelled;
};

/*
you can set state var in your ctxt (?) regarding existence of
this widget.  If exists, pop up with XtManageWidget() and nuke
with XtUnmanageWidget().  

if you don't want, you need to XtDestroyWidget(?).

Look At XtAddCallback
*/

void cancel_print_cb() {}
void dismiss_pp_cb() {}


struct progress_widget_info *
create_printWin( parent )
Widget parent;
{
    Widget children[3];      /* Children to manage */
    Arg al[64];           /* Arg List */
    register int ac = 0;      /* Arg Count */
    XmString xmstrings[15];    /* temporary storage for XmStrings */
    Widget widget5;
    Widget widget7;
    Widget printForm, PrintBtnForm, PrintCancelBtn;
    Widget PrintCloseBtn, PrintBodyForm, PrintProgress;
    struct progress_widget_info *pwi;

    pwi = (struct progress_widget_info *) calloc(1, sizeof *pwi);
    XtSetArg(al[ac], XmNtitle, "Print Progress"); ac++;
    XtSetArg(al[ac], XmNallowShellResize, FALSE); ac++;
    pwi->printWin = XmCreateDialogShell ( parent, "printWin", al, ac );
    ac = 0;
    XtSetArg(al[ac], XmNautoUnmanage, FALSE); ac++;
    printForm = XmCreateForm ( pwi->printWin, "printForm", al, ac );
    ac = 0;
    PrintBtnForm = XmCreateForm ( printForm, "PrintBtnForm", al, ac );
    xmstrings[0] = XmStringCreateLtoR("Cancel Printing", (XmStringCharSet)XmFONTLIST_DEFAULT_TAG);
    XtSetArg(al[ac], XmNlabelString, xmstrings[0]); ac++;
    PrintCancelBtn = XmCreatePushButton ( PrintBtnForm, "PrintCancelBtn", al, ac );
    XtAddCallback(PrintCancelBtn, XmNactivateCallback, cancel_print_cb, pwi);
    ac = 0;
    XmStringFree ( xmstrings [ 0 ] );
    xmstrings[0] = XmStringCreateLtoR("Close", (XmStringCharSet)XmFONTLIST_DEFAULT_TAG);
    XtSetArg(al[ac], XmNlabelString, xmstrings[0]); ac++;
    PrintCloseBtn = XmCreatePushButton ( PrintBtnForm, "PrintCloseBtn", al, ac );
    XtAddCallback(PrintCloseBtn, XmNactivateCallback, dismiss_pp_cb, pwi);
    ac = 0;
    XmStringFree ( xmstrings [ 0 ] );
    widget5 = XmCreateSeparator ( printForm, "widget5", al, ac );
    PrintBodyForm = XmCreateForm ( printForm, "PrintBodyForm", al, ac );
    widget7 = XmCreateForm ( PrintBodyForm, "widget7", al, ac );
    xmstrings[0] = XmStringCreateLtoR("Printing:", (XmStringCharSet)XmFONTLIST_DEFAULT_TAG);
    XtSetArg(al[ac], XmNlabelString, xmstrings[0]); ac++;
    pwi->PrintStateLabel = XmCreateLabel ( widget7, "PrintStateLabel", al, ac );
    ac = 0;
    XmStringFree ( xmstrings [ 0 ] );
    XtSetArg(al[ac], XmNcolumns, 50); ac++;
    XtSetArg(al[ac], XmNeditable, FALSE); ac++;
    pwi->PrintDocumentField = fe_CreateTextField ( widget7, "PrintDocumentField", al, ac );
    ac = 0;
    XmTextFieldSetString ( pwi->PrintDocumentField, "http://home.netscape.com/" );
    XtSetArg(al[ac], XmNheight, 20); ac++;
    XtSetArg(al[ac], XmNshadowType, XmSHADOW_OUT); ac++;
    PrintProgress = XmCreateFrame ( PrintBodyForm, "PrintProgress", al, ac );
    ac = 0;
    xmstrings[0] = XmStringCreateLtoR("Ongoing status goes here...", (XmStringCharSet)XmFONTLIST_DEFAULT_TAG);
    XtSetArg(al[ac], XmNlabelString, xmstrings[0]); ac++;
    pwi->PrintStatusLabel = XmCreateLabel ( PrintBodyForm, "PrintStatusLabel", al, ac );

    pwi->bar = XtVaCreateManagedWidget("thermometer", gbarWidgetClass, PrintProgress, 0);
    ac = 0;
    XmStringFree ( xmstrings [ 0 ] );

    XtSetArg(al[ac], XmNtopAttachment, XmATTACH_WIDGET); ac++;
    XtSetArg(al[ac], XmNtopOffset, 10); ac++;
    XtSetArg(al[ac], XmNtopWidget, widget5); ac++;
    XtSetArg(al[ac], XmNbottomAttachment, XmATTACH_FORM); ac++;
    XtSetArg(al[ac], XmNbottomOffset, 10); ac++;
    XtSetArg(al[ac], XmNleftAttachment, XmATTACH_FORM); ac++;
    XtSetArg(al[ac], XmNleftOffset, 10); ac++;
    XtSetArg(al[ac], XmNrightAttachment, XmATTACH_FORM); ac++;
    XtSetArg(al[ac], XmNrightOffset, 10); ac++;
    XtSetValues ( PrintBtnForm,al, ac );
    ac = 0;

    XtSetArg(al[ac], XmNtopAttachment, XmATTACH_WIDGET); ac++;
    XtSetArg(al[ac], XmNtopOffset, 10); ac++;
    XtSetArg(al[ac], XmNtopWidget, PrintBodyForm); ac++;
    XtSetArg(al[ac], XmNbottomAttachment, XmATTACH_NONE); ac++;
    XtSetArg(al[ac], XmNleftAttachment, XmATTACH_FORM); ac++;
    XtSetArg(al[ac], XmNleftOffset, 0); ac++;
    XtSetArg(al[ac], XmNrightAttachment, XmATTACH_FORM); ac++;
    XtSetArg(al[ac], XmNrightOffset, 0); ac++;
    XtSetValues ( widget5,al, ac );
    ac = 0;

    XtSetArg(al[ac], XmNtopAttachment, XmATTACH_FORM); ac++;
    XtSetArg(al[ac], XmNtopOffset, 10); ac++;
    XtSetArg(al[ac], XmNbottomAttachment, XmATTACH_NONE); ac++;
    XtSetArg(al[ac], XmNleftAttachment, XmATTACH_FORM); ac++;
    XtSetArg(al[ac], XmNleftOffset, 10); ac++;
    XtSetArg(al[ac], XmNrightAttachment, XmATTACH_FORM); ac++;
    XtSetArg(al[ac], XmNrightOffset, 10); ac++;
    XtSetValues ( PrintBodyForm,al, ac );
    ac = 0;

    XtSetArg(al[ac], XmNtopAttachment, XmATTACH_FORM); ac++;
    XtSetArg(al[ac], XmNleftAttachment, XmATTACH_FORM); ac++;
    XtSetArg(al[ac], XmNrightAttachment, XmATTACH_POSITION); ac++;
    XtSetArg(al[ac], XmNrightPosition, 49); ac++;
    XtSetValues ( PrintCancelBtn,al, ac );
    ac = 0;

    XtSetArg(al[ac], XmNtopAttachment, XmATTACH_FORM); ac++;
    XtSetArg(al[ac], XmNtopOffset, 0); ac++;
    XtSetArg(al[ac], XmNbottomAttachment, XmATTACH_NONE); ac++;
    XtSetArg(al[ac], XmNleftAttachment, XmATTACH_POSITION); ac++;
    XtSetArg(al[ac], XmNleftPosition, 51); ac++;
    XtSetArg(al[ac], XmNrightAttachment, XmATTACH_FORM); ac++;
    XtSetArg(al[ac], XmNrightOffset, 0); ac++;
    XtSetValues ( PrintCloseBtn,al, ac );
    ac = 0;
    children[ac++] = PrintCancelBtn;
    children[ac++] = PrintCloseBtn;
    XtManageChildren(children, ac);
    ac = 0;

    XtSetArg(al[ac], XmNtopAttachment, XmATTACH_FORM); ac++;
    XtSetArg(al[ac], XmNtopOffset, 0); ac++;
    XtSetArg(al[ac], XmNleftAttachment, XmATTACH_FORM); ac++;
    XtSetArg(al[ac], XmNrightAttachment, XmATTACH_FORM); ac++;
    XtSetArg(al[ac], XmNrightOffset, 0); ac++;
    XtSetValues ( widget7,al, ac );
    ac = 0;

    XtSetArg(al[ac], XmNtopAttachment, XmATTACH_WIDGET); ac++;
    XtSetArg(al[ac], XmNtopOffset, 15); ac++;
    XtSetArg(al[ac], XmNtopWidget, widget7); ac++;
    XtSetArg(al[ac], XmNbottomAttachment, XmATTACH_NONE); ac++;
    XtSetArg(al[ac], XmNleftAttachment, XmATTACH_FORM); ac++;
    XtSetArg(al[ac], XmNleftOffset, 0); ac++;
    XtSetArg(al[ac], XmNrightAttachment, XmATTACH_FORM); ac++;
    XtSetArg(al[ac], XmNrightOffset, 0); ac++;
    XtSetValues ( PrintProgress,al, ac );
    ac = 0;

    XtSetArg(al[ac], XmNtopAttachment, XmATTACH_WIDGET); ac++;
    XtSetArg(al[ac], XmNtopOffset, 15); ac++;
    XtSetArg(al[ac], XmNtopWidget, PrintProgress); ac++;
    XtSetArg(al[ac], XmNbottomAttachment, XmATTACH_FORM); ac++;
    XtSetArg(al[ac], XmNbottomOffset, 15); ac++;
    XtSetArg(al[ac], XmNleftAttachment, XmATTACH_FORM); ac++;
    XtSetArg(al[ac], XmNleftOffset, 0); ac++;
    XtSetArg(al[ac], XmNrightAttachment, XmATTACH_FORM); ac++;
    XtSetArg(al[ac], XmNrightOffset, 0); ac++;
    XtSetValues ( pwi->PrintStatusLabel,al, ac );
    ac = 0;

    XtSetArg(al[ac], XmNtopAttachment, XmATTACH_FORM); ac++;
    XtSetArg(al[ac], XmNbottomAttachment, XmATTACH_FORM); ac++;
    XtSetArg(al[ac], XmNbottomOffset, 0); ac++;
    XtSetArg(al[ac], XmNleftAttachment, XmATTACH_FORM); ac++;
    XtSetValues ( pwi->PrintStateLabel,al, ac );
    ac = 0;

    XtSetArg(al[ac], XmNtopAttachment, XmATTACH_FORM); ac++;
    XtSetArg(al[ac], XmNtopOffset, 0); ac++;
    XtSetArg(al[ac], XmNbottomAttachment, XmATTACH_FORM); ac++;
    XtSetArg(al[ac], XmNbottomOffset, 0); ac++;
    XtSetArg(al[ac], XmNleftAttachment, XmATTACH_WIDGET); ac++;
    XtSetArg(al[ac], XmNleftOffset, 0); ac++;
    XtSetArg(al[ac], XmNleftWidget, pwi->PrintStateLabel); ac++;
    XtSetArg(al[ac], XmNrightAttachment, XmATTACH_FORM); ac++;
    XtSetArg(al[ac], XmNrightOffset, 0); ac++;
    XtSetValues ( pwi->PrintDocumentField,al, ac );
    ac = 0;
    children[ac++] = pwi->PrintStateLabel;
    children[ac++] = pwi->PrintDocumentField;
    XtManageChildren(children, ac);
    ac = 0;
    children[ac++] = widget7;
    children[ac++] = PrintProgress;
    children[ac++] = pwi->PrintStatusLabel;
    XtManageChildren(children, ac);
    ac = 0;
    children[ac++] = PrintBtnForm;
    children[ac++] = widget5;
    children[ac++] = PrintBodyForm;
    XtManageChildren(children, ac);
    ac = 0;

    return pwi;
}
