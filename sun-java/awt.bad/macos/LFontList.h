// =================================================================================
// Copyright Netscape Communication 1996-1997
// Author: Frank Tang <ftang@netscape.com>
// =================================================================================
#pragma once
#include <LList.h>
class 	LFontList{
	public:
		LFontList();
		LFontList(const LFontList& infont);
		~LFontList();
	private:
		LList*  fID;
		LList*	fFont;
	protected:
		void resetFonts();
		void addFont(int16 id, short font);
		short getFont(int16 id);
		LList* getIDList()	
			{ return fID; };
};
