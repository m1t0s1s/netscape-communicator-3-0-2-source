/* -*- Mode: C; tab-width: 8 -*-
 * Security Status dialog for Mozilla
 *
 * Copyright © 1995 Netscape Communications Corporation, all rights reserved.
 *
 * $Id: statdlg.h,v 1.1 1997/04/28 16:36:33 jsw Exp $
 */

#ifndef _STATDLG_H_
#define _STATDLG_H_
typedef struct _SECStatusDialog SECStatusDialog;

typedef void (*SECNAVStatusCancelCallback)(void *arg);

struct _SECStatusDialog {
    void *window;
    void *parent_window;
    PRBool dialogUp;
    XPDialogState *state;
};

SEC_BEGIN_PROTOS

void SECNAV_CloseStatusDialog(SECStatusDialog *dlg);
SECStatusDialog *SECNAV_MakeStatusDialog(char *initialstr, void *window,
					 SECNAVStatusCancelCallback cb,
					 void *cbarg);

SEC_END_PROTOS

#endif /* _STATDLG_H_ */
