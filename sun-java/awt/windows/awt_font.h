/*
 * @(#)awt_font.h	1.15 95/12/08 Patrick P. Chan
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

#ifndef _AWTFONT_H_
#define _AWTFONT_H_

#include "awtapp.h"
#include "afxcoll.h"

class AwtFont : public CFont
{
public:
    // Debug link used for list of ALL objects...
    DEFINE_AWT_LIST_LINK

	AwtFont();
	~AwtFont();
	DECLARE_DYNCREATE(AwtFont)

public:
	// These are the messages that can be sent to this class.
	enum {CREATE, DISPOSE};

// Operations
	// Returns the AwtFont object associated with the pFontJavaObject.
	// If none exists, create one.
	static AwtFont *GetFont(Hjava_awt_Font *pJavaObject);
	
	// Creates the specified font.  pzName names the font.  nStyle is a bit
	// vector that describes the style of the font.  nHeight is the point
	// size of the font.
	static AwtFont *Create(char *pzName, int nStyle, int nHeight);

	// Maps a platform-independent name to a platform-specific name.
	static char *PINameToPSName(char *pzPIName);

	// Called from AwtApp.
	static long HandleMsg(UINT nMsg, long p1 = NULL, long p2 = NULL, 
                              long p3 = NULL, long p4 = NULL, long p5 = NULL, 
                              long p6 = NULL, long p7 = NULL, long p8 = NULL);

	// Releases all resources and destroys the object.  After this call,
	// this object is no longer usable.
	void Dispose();

	static void Cleanup();

// Variables
public:
	// The ascent of this font.
	int m_nAscent;

private:
	static AwtFont *CreateNewFont(char *pzName, int nStyle, int nHeight);

    int m_nRefCount;
    static CMapStringToOb g_AwtFontCache;

private:
// Variables
	// The oak font object.
	//Hjava_awt_Font *m_pJavaObject;
};

#endif // _AWTFONT_H_
