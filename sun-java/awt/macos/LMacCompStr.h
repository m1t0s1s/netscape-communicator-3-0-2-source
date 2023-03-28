// =================================================================================
// Copyright Netscape Communication 1996-1997
// Author: Frank Tang <ftang@netscape.com>
// =================================================================================
#pragma once
#include "LCompStr.h"
#include "LJavaFontList.h"
class LMacCompStr : public LCompStr {
	public:
		LMacCompStr();
		LMacCompStr(unicode* ustr, int32 len);
		LMacCompStr(ConstStr255Param unicodePStr);
		LMacCompStr(struct Hjava_lang_String *stringObject);
		LMacCompStr(HArrayOfChar *text, long offset, long length);
		~LMacCompStr();
		void Draw(LJavaFontList* font, Rect& rect);
		void DrawLeft(LJavaFontList* font, Rect& rect);
		void DrawJustified(LJavaFontList* font, Rect& rect, Int16	justification);
		void Draw(LJavaFontList* font, short x, short y);
		long DrawWidth(LJavaFontList* font, short x, short y);
		long Width(LJavaFontList* font);
		long Height(LJavaFontList* font);
		long Baseline(LJavaFontList* font, Rect& rect);
	private:
};
