//##############################################################################
//##############################################################################
//
//	File:		LMacFontMetrics.cp
//	Author:		Dan Clifford
//	
//	Copyright © 1995-1996, Netscape Communications Corporation
//
//##############################################################################
//##############################################################################

extern "C" {

#include <stdlib.h>
#include "native.h"
#include "typedefs_md.h"
#include "java_awt_Color.h"
#include "java_awt_Font.h"
#include "java_awt_Component.h"
#include "java_awt_FontMetrics.h"
#include "sun_awt_macos_MComponentPeer.h"
#include "sun_awt_macos_MacFontMetrics.h"
#include "interpreter.h"
#include "exceptions.h"

};

#include <Quickdraw.h>
#include <QuickdrawText.h>
#include <Fonts.h>
#include <TextUtils.h>
#include "String_Utils.h"
#include "UDrawingState.h"

#include "LMacFontMetrics.h"
#include "LMacGraphics.h"

#ifdef UNICODE_FONTLIST
#include "LMacCompStr.h"
#endif

extern void sun_awt_macos_MacFontMetrics_init(struct Hsun_awt_macos_MacFontMetrics *pMacFontMetricsJavaObject)
{
	Classsun_awt_macos_MacFontMetrics 		*macFontMetrics = unhand(pMacFontMetricsJavaObject);
	SInt32									*widthArray;
#ifdef UNICODE_FONTLIST
	LJavaFontList							unicodeFontList;
#else
	short									fontNum;
	short									fontSize;
	Style									fontStyle;
	FontInfo								fontInfo;
#endif
	//	Save off our port and port information

	CGrafPtr								wMgrCPort;
	GetCWMgrPort(&wMgrCPort);
	StPortOriginState 						savedPortInfo((GrafPtr)wMgrCPort);
	StTextState								savedFontInfo;
#ifdef UNICODE_FONTLIST
	//	Allocate space for the width array

	macFontMetrics->widths = (HArrayOfInt *)ArrayAlloc(T_INT, 256);

	if (macFontMetrics->widths == NULL) {
		SignalError(0, JAVAPKG "OutOfMemoryError", 0);
	}

	else {
		unicodeFontList.SetFont(unhand(macFontMetrics->font));
		unicodeFontList.InitFontMetrics(macFontMetrics);
	}
#else	
	//	Get the name of the font, and convert it into 
	//	a pascal string.

	ConvertFontToNumSizeAndStyle(macFontMetrics->font, &fontNum, &fontSize, &fontStyle);
	
	//	Call Quickdraw to get our metrics.
	
	::TextFont(fontNum);
	::TextSize(fontSize);
	::TextFace(fontStyle);
	::GetFontInfo(&fontInfo);

	macFontMetrics->ascent = fontInfo.ascent;
  	macFontMetrics->descent = fontInfo.descent;
    macFontMetrics->leading = fontInfo.leading;
    macFontMetrics->height = fontInfo.ascent + fontInfo.descent;
    macFontMetrics->maxAscent = fontInfo.ascent;
    macFontMetrics->maxDescent = fontInfo.descent;
    macFontMetrics->maxHeight = fontInfo.ascent + fontInfo.descent;
    macFontMetrics->maxAdvance = fontInfo.widMax;

	//	Allocate space for the width array

	macFontMetrics->widths = (HArrayOfInt *)ArrayAlloc(T_INT, 256);

	if (macFontMetrics->widths == NULL) {
		SignalError(0, JAVAPKG "OutOfMemoryError", 0);
	}

	else {
	
		//	Fill the array with the font info.
		
		widthArray = unhand(macFontMetrics->widths)->body;
	
		for (UInt32 count = 0; count < 256; count++) 
			widthArray[count] = ::CharWidth(count);
	
	}
#endif
}

extern long sun_awt_macos_MacFontMetrics_stringWidth(struct Hsun_awt_macos_MacFontMetrics *pMacFontMetricsJavaObject, struct Hjava_lang_String *stringObject)
{
	Classsun_awt_macos_MacFontMetrics 		*macFontMetrics = unhand(pMacFontMetricsJavaObject);
#ifdef UNICODE_FONTLIST
	LJavaFontList							unicodeFontList;
#else
	short									fontNum;
	short									fontSize;
	Style									fontStyle;
	char									*newCString;
#endif
	short									textWidth;

	//	Save off our port and port information

	CGrafPtr								wMgrCPort;
	GetCWMgrPort(&wMgrCPort);
	StPortOriginState 						savedPortInfo((GrafPtr)wMgrCPort);
	StTextState								savedFontInfo;

#ifdef UNICODE_FONTLIST
	unicodeFontList.SetFont(unhand(macFontMetrics->font));
	LMacCompStr str(stringObject);
	textWidth = str.Width(&unicodeFontList);
	unicodeFontList.SetTextLatin1Font();
	
#else
	//	Get the name of the font, and convert it into 
	//	a pascal string.

	ConvertFontToNumSizeAndStyle(macFontMetrics->font, &fontNum, &fontSize, &fontStyle);
	
	//	Call Quickdraw to get our metrics.
	
	::TextFont(fontNum);
	::TextSize(fontSize);
	::TextFace(fontStyle);

	newCString = allocCString(stringObject);
	textWidth = ::TextWidth((unsigned char *)newCString, 0, javaStringLength(stringObject));
	free(newCString);
#endif	
	return textWidth;
	
}

extern long sun_awt_macos_MacFontMetrics_charsWidth(struct Hsun_awt_macos_MacFontMetrics *pMacFontMetricsJavaObject, HArrayOfChar *characters, long start, long length)
{
	Classsun_awt_macos_MacFontMetrics 		*macFontMetrics = unhand(pMacFontMetricsJavaObject);
#ifdef UNICODE_FONTLIST
	LJavaFontList							unicodeFontList;
#else
	unicode									*unicodeArray = unhand(characters)->body + start;
	short									fontNum;
	short									fontSize;
	Style									fontStyle;
	char									*newCString;
#endif
	long									textWidth;

	//	Save off our port and port information

	CGrafPtr								wMgrCPort;
	GetCWMgrPort(&wMgrCPort);
	StPortOriginState 						savedPortInfo((GrafPtr)wMgrCPort);
	StTextState								savedFontInfo;

#ifdef UNICODE_FONTLIST
	unicodeFontList.SetFont(unhand(macFontMetrics->font));
	LMacCompStr str(characters,start,length);
	textWidth = str.Width(&unicodeFontList);
	unicodeFontList.SetTextLatin1Font();
#else
	//	Get the name of the font, and convert it into 
	//	a pascal string.

	ConvertFontToNumSizeAndStyle(macFontMetrics->font, &fontNum, &fontSize, &fontStyle);
	
	//	Call Quickdraw to get our metrics.
	
	::TextFont(fontNum);
	::TextSize(fontSize);
	::TextFace(fontStyle);

	newCString = (char *)malloc(length);
	if (newCString == NULL) return 0;
	
	long	lengthCopy = length;
	char	*currentCharacter = newCString;
	
	while (lengthCopy--)
		*currentCharacter++ = *unicodeArray++;
	
	textWidth = ::TextWidth((unsigned char *)newCString, 0, length);
	free(newCString);
#endif	
	return textWidth;


}

extern long sun_awt_macos_MacFontMetrics_bytesWidth(struct Hsun_awt_macos_MacFontMetrics *pMacFontMetricsJavaObject, HArrayOfByte *bytes, long start, long length)
{

	Classsun_awt_macos_MacFontMetrics 		*macFontMetrics = unhand(pMacFontMetricsJavaObject);
	char									*charArray = unhand(bytes)->body + start;
	short									fontNum;
	short									fontSize;
	Style									fontStyle;
	long									textWidth;

	//	Save off our port and port information

	CGrafPtr								wMgrCPort;
	GetCWMgrPort(&wMgrCPort);
	StPortOriginState 						savedPortInfo((GrafPtr)wMgrCPort);
	StTextState								savedFontInfo;

	//	Get the name of the font, and convert it into 
	//	a pascal string.

	ConvertFontToNumSizeAndStyle(macFontMetrics->font, &fontNum, &fontSize, &fontStyle);
	
	//	Call Quickdraw to get our metrics.
	
	::TextFont(fontNum);
	::TextSize(fontSize);
	::TextFace(fontStyle);

	textWidth = ::TextWidth((unsigned char *)charArray, 0, length);
	
	return textWidth;
	

}
