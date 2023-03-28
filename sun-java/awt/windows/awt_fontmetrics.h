/*
 * @(#)awt_fontmetrics.h	1.12 95/11/29 Patrick P. Chan
 *
 * Copyright (c) 1994 Sun Microsystems, Inc. All Rights Reserved.
 *
 * Permission to use, copy, modify, and distribute this software
 * and its documentation for NON-COMMERCIAL purposes and without
 * fee is hereby granted provided that this copyright notice
 * appears in all copies. Please refer to the file "copyright.html"
 * for further important copyright and licensing information.
 *
 * SUN MAKES NO REPRESENTATIONS OR WARRANTIES ABOUT THE SUITABILITY OF
 * THE SOFTWARE, EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED
 * TO THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
 * PARTICULAR PURPOSE, OR NON-INFRINGEMENT. SUN SHALL NOT BE LIABLE FOR
 * ANY DAMAGES SUFFERED BY LICENSEE AS A RESULT OF USING, MODIFYING OR
 * DISTRIBUTING THIS SOFTWARE OR ITS DERIVATIVES.
 */
#include "awt_window.h"

#ifndef _AwtFontMetrics_H_
#define _AwtFontMetrics_H_

class AwtFontMetrics
{
public:
    // Debug link used for list of ALL objects...
    DEFINE_AWT_LIST_LINK

	AwtFontMetrics();
	~AwtFontMetrics();

public:
// Operations
	// Loads the metrics of the associated font.
	// See Font.GetFont for purpose of pWS.  (Also, client should provide
	// Font java object instead of getting it from the FontMetrics instance variable.)
	static void LoadMetrics(Hsun_awt_win32_Win32FontMetrics *pJavaObject);

	static long StringWidth(Hsun_awt_win32_Win32FontMetrics *pJavaObject, char *pzString);

	// Returns the AwtFont associated with this metrics.
	static AwtFont *GetFont(Hsun_awt_win32_Win32FontMetrics *pJavaObject);

	// Sets the ascent of the font.  This member should be called if pFont->m_nAscent < 0.
	static void SetFontAscent(AwtFont *pFont);

	//long CharWidth(char ch);
	//long StringWidth(char *pzString);

	// Determines the average dimension of the character in the
	// specified font 'pFont' and multiplies it by the specified number of
	// rows and columns.  
	// 'pFont' can be a temporary object.
	static CSize AwtFontMetrics::TextSize(CFont *pFont, int nColumns, int nRows);

	// If 'pFont' is NULL, the SYSTEM_FONT is used to compute the size.
	// 'pFont' can be a temporary object.
	static CSize StringWidth(CFont *pFont, char *pzString);
};

#endif // _AwtFontMetrics_H_
