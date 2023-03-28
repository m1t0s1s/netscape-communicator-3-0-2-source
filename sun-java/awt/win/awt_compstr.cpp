// =================================================================================
// Copyright Netscape Communication 1996-1997
// Author: Frank Tang <ftang@netscape.com>
// =================================================================================
#include "awt_compstr.h"
#include "awt_i18n.h"
#ifdef INTLUNICODE

CompStr::CompStr()
{
	m_IsMenuSeperator = FALSE;
	fCompStr = NULL;
}
CompStr::CompStr(CompStr& str)
{
	m_IsMenuSeperator = FALSE;
	if(str.fCompStr)
		fCompStr = libi18n::INTL_CompoundStrClone(str.fCompStr);
	else
		fCompStr = NULL;
}
CompStr::~CompStr()
{
	if(fCompStr)
		libi18n::INTL_CompoundStrDestroy(fCompStr);
	fCompStr = NULL;	
}
void CompStr::InitCommon(WCHAR* ustr, int32 len)
{
	fCompStr = libi18n::INTL_CompoundStrFromUnicode((INTL_Unicode* )ustr, len);
	m_IsMenuSeperator = ((len == 1) && (ustr[0] == 0x002d));
}
CompStr::CompStr(WCHAR* ustr)
{
	InitCommon(ustr, libi18n::INTL_UnicodeLen(ustr));
}
CompStr::CompStr(Hjava_lang_String *string)
{
	HArrayOfChar* array = (HArrayOfChar *)unhand( string )->value;
	InitCommon(unhand(array)->body + unhand( string )->offset, unhand( string )->count);
}

CompStrIterator::CompStrIterator(CompStr *inStr)
{
	fState = 0;
	fIterator = getIterator(inStr);
}
CompStrIterator::~CompStrIterator()
{
	// empty
}
INTL_CompoundStrIterator getIterator(CompStr* inStr)
{
	return (INTL_CompoundStrIterator) inStr->fCompStr;
}

BOOL CompStrIterator::Next(int16 *outencoding, unsigned char** outtext)
{
	if(fState == 0)
	{
		fState++;
		fIterator = libi18n::INTL_CompoundStrFirstStr(	fIterator, 
												(INTL_Encoding_ID *)outencoding,
												outtext);
	}
	else
	{
		fIterator = libi18n::INTL_CompoundStrNextStr(	fIterator, 
												(INTL_Encoding_ID *)outencoding,
												outtext);
	}
	return (fIterator != 0);
}

#endif // INTLUNICODE
