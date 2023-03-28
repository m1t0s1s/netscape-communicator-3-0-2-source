/*
 * @(#)font.cpp	1.1 95/08/03 Patrick P. Chan
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
#include "stdafx.h"
#include "awt.h"
#include "awt_font.h"
#include "prprf.h"		// for PR_snprintf(...)


IMPLEMENT_DYNCREATE(AwtFont, CFont)

#ifdef ENABLE_DEBUG_NEW
#define new DEBUG_NEW
#endif


AwtFont::AwtFont()
{
    // Add the new object to the list of all AWT objects (debug only)
    INITIALIZE_AWT_LIST_LINK("AwtFont");

	m_nAscent = -1;
}

AwtFont::~AwtFont()
{
    CAwtApp* pApp = (CAwtApp *)AfxGetApp();
    if (pApp->IsServerThread()) {
	DeleteObject();
    } else {
        pApp->SendAwtMsg(CAwtApp::AWT_FONT, DISPOSE, (long)this);
    }

    // Remove the object from the list of all AWT objects (debug only)
    REMOVE_AWT_LIST_LINK;
}

long AwtFont::HandleMsg(UINT nMsg, long p1, long p2, long p3, 
                        long p4, long p5, long p6, long p7, long p8) {
    switch (nMsg) {
      case CREATE:				
          return (long)Create((char*)p1, (int)p2, (int)p3);
      case DISPOSE:
          ((AwtFont *)p1)->Dispose();
          break;
      default:
          ASSERT(FALSE);
    }
    return 0;
}

AwtFont *AwtFont::GetFont(Hjava_awt_Font *pJavaObject) {
	AwtFont *pFont = (AwtFont *)unhand(pJavaObject)->pData;
	Classjava_awt_Font *pClass = unhand(pJavaObject);
	char stackBuffer[64];
	char *buffer = &stackBuffer[0];

	if (pFont != NULL) {
		return pFont;
	}
	int len = javaStringLength(pClass->family);

	if (len >= sizeof(stackBuffer)) {
		buffer = (char *)malloc(len + 1);
	}
	javaString2CString(pClass->family, buffer, len + 1);
	pFont = Create(PINameToPSName(buffer), pClass->style, pClass->size);

	if (len >= sizeof(stackBuffer)) {
		free(buffer);
	}
	unhand(pJavaObject)->pData = (long)pFont;
	return pFont;
}

CMapStringToPtr fontCache;

AwtFont *AwtFont::Create(char *pzName, int nStyle, int nHeight)
{
    CAwtApp* pApp = (CAwtApp *)AfxGetApp();
    if (!pApp->IsServerThread()) {
        return (AwtFont *)pApp->SendAwtMsg(CAwtApp::AWT_FONT, CREATE, 
                                           (long)pzName, 
                                           (long)nStyle, 
                                           (long)nHeight);
    }

    char longName[80];
    PR_snprintf(longName, sizeof(longName), "%s-%d-%d", pzName, nStyle, nHeight);
    void* p;
    if (fontCache.Lookup(longName, p)) {
        return (AwtFont*)p;
    }

    AwtFont *pFont = new AwtFont();
    int nLen = strlen(pzName);
    LOGFONT logFont;
	
    logFont.lfWidth = 0;
    logFont.lfEscapement = 0;
    logFont.lfOrientation = 0;
    logFont.lfUnderline = FALSE;
    logFont.lfStrikeOut = FALSE;
    logFont.lfCharSet = ANSI_CHARSET;
    logFont.lfOutPrecision = OUT_DEFAULT_PRECIS;
    logFont.lfClipPrecision = CLIP_DEFAULT_PRECIS;
    logFont.lfQuality = DEFAULT_QUALITY;
    logFont.lfPitchAndFamily = DEFAULT_PITCH;

    // Get face name
    if (nLen >= LF_FACESIZE) {
	nLen = LF_FACESIZE-1;
    }
    strncpy(&logFont.lfFaceName[0], pzName, nLen);
    logFont.lfFaceName[nLen] = 0;

    // Set style
    logFont.lfWeight = nStyle & java_awt_Font_BOLD ? FW_BOLD : FW_NORMAL;
    logFont.lfItalic = (nStyle & java_awt_Font_ITALIC) != 0;
    logFont.lfUnderline = 0;//(nStyle & java_awt_Font_UNDERLINE) != 0;

    // Get point size
    logFont.lfHeight = -nHeight;
#ifdef DEBUG
    char *pAwtEnv = getenv("AWT_FONT");
    if (pAwtEnv != NULL) {
        logFont.lfHeight = (int)((float)logFont.lfHeight * atof(pAwtEnv));// <<temp hack to make things bigger>>
    }
#endif
    VERIFY(pFont->CreateFontIndirect(&logFont)); 

    fontCache.SetAt(longName, pFont);
    return pFont;
}

char *AwtFont::PINameToPSName(char *pzPIName)
{
	if (!strcmp(pzPIName, "Helvetica")) {
		return "Arial";
	} else if (!strcmp(pzPIName, "TimesRoman")) {
		return "Times New Roman";
	} else if (!strcmp(pzPIName, "Courier")) {
		return "Courier New";
	} else if (!strcmp(pzPIName, "Dialog")) {
		return "MS Sans Serif";
	} else if (!strcmp(pzPIName, "DialogInput")) {
		return "MS Sans Serif";
	} else if (!strcmp(pzPIName, "ZapfDingbats")) {
		return "WingDings";
	}
	return "Arial";
}

void AwtFont::Dispose()
{
// Font.dispose() is never called (it's not a component), so leave it in
// the fontCache.
//    DeleteObject();
}

void AwtFont::Cleanup()
{
    POSITION pos = fontCache.GetStartPosition();
    while (pos != NULL) {
        CString key;
        void *p;
        fontCache.GetNextAssoc(pos, key, p);
        AwtFont* pFont = (AwtFont*)p;
        if (pFont != NULL) {
            delete pFont;
        }
    }
    fontCache.RemoveAll();
}
