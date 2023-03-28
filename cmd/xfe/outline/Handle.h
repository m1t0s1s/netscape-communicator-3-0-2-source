/* Handle.h: Public header file for the Outline Handle widget. */
/* Copyright © 1994 Torgeir Veimo. */
/* See the README file for copyright details. */

#ifndef HANDLE_H
#define HANDLE_H

#ifdef __cplusplus 
extern "C" {
#endif

extern WidgetClass xmHandleWidgetClass;
typedef struct _XmHandleClassRec *XmHandleWidgetClass;
typedef struct _XmHandleRec *XmHandleWidget;

#define XmNsubWidget		"subWidget"
#define XmCSubWidget		"SubWidget"

#ifdef _NO_PROTO
Widget XmCreateHandle();
#else
Widget XmCreateHandle(Widget, char *, ArgList, Cardinal);
#endif

#ifdef __cplusplus
}
#endif

#endif /* HANDLE_H */
        
