/*
 * @(#)awt_fontmetrics.cpp	1.14 95/11/30 Patrick P. Chan
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
#include "awt_fontmetrics.h"

/*
 * DAC: The following is a workaround for a compiler bug which caused
 * incorrect code to be generated in LoadMetrics when using the inlined
 * version of memset. This pragma forces the compiler to use a function
 * call instead.
 */
#pragma function(memset)

#ifdef ENABLE_DEBUG_NEW
#define new DEBUG_NEW
#endif


// We need a DC in order to determine the width of strings.
// The WServer.oak class synchronizes access to this global
// variable.
static HDC ghDC = NULL;
static AwtFont *gpCurrentFont = NULL;

AwtFontMetrics::AwtFontMetrics()
{
    // Add the new object to the list of all AWT objects (debug only)
    INITIALIZE_AWT_LIST_LINK("AwtFontMetrics");
}

AwtFontMetrics::~AwtFontMetrics()
{
	if (ghDC != NULL) {
		// Make sure the font is not currently selected in ghDC.
		VERIFY(::SelectObject(ghDC, ::GetStockObject(SYSTEM_FONT)) != NULL);
		gpCurrentFont = NULL;
	}

    // Remove the object from the list of all AWT objects (debug only)
    REMOVE_AWT_LIST_LINK;
}

AwtFont *AwtFontMetrics::GetFont(Hsun_awt_win32_Win32FontMetrics *pJavaObject)
{
	Classsun_awt_win32_Win32FontMetrics *pClass = unhand(pJavaObject);
	return (AwtFont *)(pClass->font)->obj->pData;
}

void AwtFontMetrics::SetFontAscent(AwtFont *pFont) {
	TEXTMETRIC metrics;

	if (ghDC == NULL) {
		ghDC = ::CreateCompatibleDC(NULL);
	}
	::SelectObject(ghDC, (HGDIOBJ)(pFont->m_hObject));
	VERIFY(::GetTextMetrics(ghDC, &metrics));
 	gpCurrentFont = pFont;
	pFont->m_nAscent = metrics.tmAscent;
}

void AwtFontMetrics::LoadMetrics(Hsun_awt_win32_Win32FontMetrics *pJavaObject)
{
	Classsun_awt_win32_Win32FontMetrics *pClass = unhand(pJavaObject);
	AwtFont *pFont = AwtFont::GetFont(pClass->font);
	TEXTMETRIC metrics;
	char buffer[2];
 	SIZE size;


	pClass->widths = (HArrayOfInt *)ArrayAlloc(T_INT, 256);
	long *pWidths = unhand(pClass->widths)->body;
	if (pClass->widths == NULL) {
		SignalError(0, JAVAPKG "OutOfMemoryError", 0);
		return;
	}

	if (ghDC == NULL) {
		ghDC = ::CreateCompatibleDC(NULL);
	}
	::SelectObject(ghDC, (HGDIOBJ)(pFont->m_hObject));
	VERIFY(::GetTextMetrics(ghDC, &metrics));
 	gpCurrentFont = pFont;

	pClass->ascent = pFont->m_nAscent = metrics.tmAscent;
	pClass->descent = metrics.tmDescent;
	pClass->leading = metrics.tmExternalLeading;
	pClass->height = metrics.tmAscent + metrics.tmDescent + pClass->leading;
	pClass->maxAscent = pClass->ascent;
	pClass->maxDescent = pClass->descent;
	pClass->maxHeight = pClass->maxAscent + pClass->maxDescent + pClass->leading;
	pClass->maxAdvance = metrics.tmMaxCharWidth;

	memset(pWidths, 0, 256 * sizeof(long));	
	buffer[1] = 0;
	pWidths += metrics.tmFirstChar;
	for (int i=metrics.tmFirstChar; i<=metrics.tmLastChar; ++i) {
		buffer[0] = (char)i;
		VERIFY(::GetTextExtentPoint32(ghDC, &buffer[0], 1, &size));
		*pWidths++ = size.cx;
	}
}

long AwtFontMetrics::StringWidth(Hsun_awt_win32_Win32FontMetrics *pJavaObject, char *pzString)
{
	AwtFont *pFont = GetFont(pJavaObject);
	CSize size;

	if (ghDC == NULL) {
		ghDC = ::CreateCompatibleDC(NULL);
	}
	if (gpCurrentFont != pFont) {
		gpCurrentFont = pFont;
		VERIFY(::SelectObject(ghDC, (HGDIOBJ)(pFont->m_hObject)) != NULL);
	}
	VERIFY(::GetTextExtentPoint32(ghDC, pzString, strlen(pzString), &size));
	return size.cx;
}

CSize AwtFontMetrics::StringWidth(CFont *pFont, char *pzString)
{
	CSize size;
	HGDIOBJ oldFont;
	
	if (ghDC == NULL) {
		ghDC = ::CreateCompatibleDC(NULL);
	}
	if (pFont == NULL) {
		oldFont = ::SelectObject(ghDC, ::GetStockObject(SYSTEM_FONT));
	} else {
		oldFont = ::SelectObject(ghDC, (HGDIOBJ)pFont->m_hObject);
	}
	VERIFY(::GetTextExtentPoint32(ghDC, pzString, strlen(pzString), &size));
	VERIFY(::SelectObject(ghDC, oldFont));
	return size;
}

CSize AwtFontMetrics::TextSize(CFont *pFont, int nColumns, int nRows)
{
	CSize size;
	HGDIOBJ oldFont;
	
	if (ghDC == NULL) {
		ghDC = ::CreateCompatibleDC(NULL);
	}
	if (pFont == NULL) {
		oldFont = ::SelectObject(ghDC, ::GetStockObject(SYSTEM_FONT));
	} else {
		oldFont = ::SelectObject(ghDC, (HGDIOBJ)pFont->m_hObject);
	}
	VERIFY(::GetTextExtentPoint(ghDC, 
		"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ", 52, &size));
	VERIFY(::SelectObject(ghDC, oldFont));
	size.cx = size.cx * nColumns / 52;
	size.cy = size.cy * nRows;
	return size;
}
