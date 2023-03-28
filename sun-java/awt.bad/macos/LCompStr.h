// =================================================================================
// Copyright Netscape Communication 1996-1997
// Author: Frank Tang <ftang@netscape.com>
// =================================================================================
#pragma once
extern "C" {
#include "libi18n.h"
#include "oobj.h"
};

class LCompStr {
		friend INTL_CompoundStrIterator getIterator(LCompStr* inStr);	
	private:
		INTL_CompoundStr* 			fCompStr;
	public:
		LCompStr();
		LCompStr(ConstStr255Param unicodePStr);
		LCompStr(unicode* ustr, int32 len);
		LCompStr(struct Hjava_lang_String *stringObject);
		LCompStr(HArrayOfChar *text, long offset, long length);
		~LCompStr();
	private:
		void InitCommon(unicode* ustr, int32 len);	
};

class LCompStrIterator {
	private:
		int fState;
		INTL_CompoundStrIterator 	fIterator;
	public:
		LCompStrIterator(LCompStr* str);
		~LCompStrIterator();
		Boolean Next(int16 *outencoding, unsigned char** outtext);
};
