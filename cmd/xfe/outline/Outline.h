/* Outline.h: Public header file for the Outline widget. */
/* Copyright © 1994 Torgeir Veimo. */
/* See the README file for copyright details. */

#ifndef OUTLINE_H
#define OUTLINE_H

#ifdef __cplusplus 
extern "C" {
#endif

extern WidgetClass xmOutlineWidgetClass;
typedef struct _XmOutlineClassRec *XmOutlineWidgetClass;
typedef struct _XmOutlineRec *XmOutlineWidget;

#define XmNindentation		"indentation"
#define XmCIndentation		"Indentation"
#define XmNoutline		"outline"
#define XmCOutline		"Outline"

#ifdef _NO_PROTO
Widget XmCreateOutline();
#else
Widget XmCreateOutline(Widget, char *, ArgList, Cardinal);
#endif

#ifdef __cplusplus
}
#endif

#endif /* OUTLINE_H */
        
