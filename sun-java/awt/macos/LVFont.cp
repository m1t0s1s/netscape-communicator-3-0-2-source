// =================================================================================
// Copyright Netscape Communication 1996-1997
// Author: Frank Tang <ftang@netscape.com>
// =================================================================================
#include "LVFont.h"
#include "String_Utils.h"
#include "uprefd.h"
#include "LString.h"
#include "xp_trace.h"

enum {
	kUseFEProportionalFont = 1,
	kUseFEFixFont
};
LVFont::LVFont(Str255 javaFontName)
{
	if(EqualString(javaFontName, "\pTimesRoman", false, false)) {
		InitCommon("\pTimes", kUseFEProportionalFont);
	} else if(EqualString(javaFontName, "\pDialog", false, false))	{
		InitCommon("\pGeneva", kUseFEFixFont);
	} else if(EqualString(javaFontName, "\pHelvetica", false, false))	{
		InitCommon("\pHelvetica", smScriptAppFondSize);
	} else if(EqualString(javaFontName, "\pDialogInput", false, false))	{
		InitCommon("\pGeneva", smScriptSysFondSize);
	} else if(EqualString(javaFontName, "\pCourier", false, false))	{
		InitCommon("\pCourier", smScriptMonoFondSize);
	} else {
		InitCommon("\pTimes", kUseFEProportionalFont);
	}	
	// otherwise, use the default
}
LVFont::LVFont()
{
	LString::CopyPStr("\pTimes", fRomanfn, 255);
	fScriptSelector = kUseFEProportionalFont;
}
void LVFont::InitCommon(ConstStringPtr inRomanfn, short inScriptSelector)
{
	LString::CopyPStr(inRomanfn,fRomanfn, 255);
	fScriptSelector = inScriptSelector;
}
LVFont::~LVFont()
{
}
short LVFont::getFontNumber(Str255 fname)
{
	short family;
	::GetFNum(fname, &family);
	return family;
}


// This the object version
short LVFont::GetVF(int16 encoding)
{
	return this->getVF(encoding, fRomanfn, fScriptSelector);
}

// This is the static version
short LVFont::getVF(int16 encoding, Str255 romanfn, short selector)
{
	switch(encoding)
	{
		case CS_MAC_ROMAN:
			return LVFont::getFontNumber(romanfn);
		case CS_DINGBATS:
			return  LVFont::getFontNumber("\pDingbats");
		case CS_SYMBOL:
			return  LVFont::getFontNumber("\pSymbol");
		default:
			switch(selector)
			{
				case kUseFEProportionalFont:
					return	CPrefs::GetProportionalFont((unsigned short) encoding);
				case kUseFEFixFont:
					return	CPrefs::GetFixFont((unsigned short)encoding);
				default:
					return (::GetScriptVariable(CPrefs::CsidToScript(encoding), selector) >> 16);
			}
	}
}
short LVFont::GetVF(Classjava_awt_Font *font, int16 encoding)
{
	Str255	fontPString;
	javaString2CString(font->name, (char *)(fontPString + 1), sizeof(fontPString) - 1);
	fontPString[0] = strlen((char *)(fontPString + 1));
	return LVFont::GetVF(fontPString, encoding);
}
short LVFont::GetVF(Str255 javaFontName, int16 encoding)
{
	if(EqualString(javaFontName, "\pTimesRoman", false, false)) {
		return getVF(encoding, "\pTimes", kUseFEProportionalFont);
	} else if(EqualString(javaFontName, "\pDialog", false, false))	{
		return getVF(encoding,"\pGeneva", kUseFEFixFont);
	} else if(EqualString(javaFontName, "\pHelvetica", false, false))	{
		return getVF(encoding,"\pHelvetica", smScriptAppFondSize);
	} else if(EqualString(javaFontName, "\pDialogInput", false, false))	{
		return getVF(encoding,"\pGeneva", smScriptSysFondSize);
	} else if(EqualString(javaFontName, "\pCourier", false, false))	{
		return getVF(encoding,"\pCourier", smScriptMonoFondSize);
	} else
	return getVF(encoding,"\pTimes", kUseFEProportionalFont);
}



ResIDT	UEncoding::GetTextTextTraitsID(int16 encoding)
{
	return CPrefs::GetTextFieldTextResIDs(encoding);
}
ResIDT	UEncoding::GetButtonTextTraitsID(int16 encoding)
{
	return CPrefs::GetButtonFontTextResIDs(encoding);
}
short	UEncoding::GetScriptID(int16 encoding)
{
	return CPrefs::CsidToScript(encoding);
}
Boolean	UEncoding::IsMacRoman(int16 encoding)
{
	return (encoding == CS_MAC_ROMAN);
}
