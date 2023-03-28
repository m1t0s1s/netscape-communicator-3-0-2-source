// =================================================================================
// Copyright Netscape Communication 1996-1997
// Author: Frank Tang <ftang@netscape.com>
// =================================================================================
#ifndef AWT_COMPSTR_H
#define AWT_COMPSTR_H
#include "awt_defs.h"
#include "libi18n.h"
extern "C" {
#include "java_lang_String.h"
};

class CompStr {
		friend INTL_CompoundStrIterator getIterator(CompStr* inStr);	
	private:
		INTL_CompoundStr* 			fCompStr;
		BOOL						m_IsMenuSeperator;
	public:
		CompStr();
		CompStr(CompStr &in);
		CompStr(WCHAR* ustr);
		CompStr(Hjava_lang_String *string);
		~CompStr();
		virtual BOOL IsMenuSeparator()	{ return m_IsMenuSeperator; }
	private:
		void InitCommon(WCHAR* ustr, int32 len);	
};

class CompStrIterator {
	private:
		int fState;
		INTL_CompoundStrIterator 	fIterator;
	public:
		CompStrIterator(CompStr* str);
		~CompStrIterator();
		BOOL Next(int16 *outencoding, unsigned char** outtext);
};
#endif // AWT_COMPSTR_H
