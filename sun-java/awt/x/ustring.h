/*************************************************************************************
 *
 * Copyright (c) 1996 Netscape Communications Corp.  All rights reserved.
 *
 * Description:
 *  This file declares a set of functions used to read & display Unicode under
 *  Motif.  These functions attempt to support the full Unicode range - not just
 *  the ASCII range.
 *
 * Authors:
 *  Brian Beck <bcbeck@netscape.com>    Feb 22, 1996     created this file.
 *
 *************************************************************************************/

#ifndef _USTRING_H_
#define _USTRING_H_

#include "awt_p.h"

/* Create an XmString from a Java string. */ 
XmString  makeXmString( Hjava_lang_String* javaStringHandle );


/* Create an XmString from a buffer of Unicode characters. */
XmString  makeXmStringFromChars( unicode* uniChars, long length );


/* Create a C string from a Java string.  The C string will use the encoding
 * appropriate to the user's current locale.  
 */
char*  makeLocalizedCString( Hjava_lang_String* javaStringHandle );


/* Create a Java string from a C string.  The given C string uses an encoding
 * appropriate to the user's current locale.
 */
Hjava_lang_String*  makeJavaStringFromLocalizedCString( char* cString );


/* Create an XmFontList from a Java font object. */
XmFontList makeXmFontList( struct Hjava_awt_Font* javaFontHandle , XFontStruct *xfont );

#endif /* _USTRING_H_ */
