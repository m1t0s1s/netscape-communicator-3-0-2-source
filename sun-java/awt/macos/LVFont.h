// =================================================================================
// Copyright Netscape Communication 1996-1997
// Author: Frank Tang <ftang@netscape.com>
// =================================================================================
#pragma once
extern "C" {
#include <Types.h>
#include <QuickDraw.h>
#include "java_awt_Font.h"
#include "sun_awt_macos_MacFontMetrics.h"
};

class LVFont {
	private:
		Str255 	fRomanfn;
		short 	fScriptSelector;
	public:
		LVFont();
		LVFont(Str255 javaFontName);
		~LVFont();
		short			GetVF(int16 encoding);
		
		// static function for ComponmentPeer
		static short 	GetVF(Str255 javaFontName, int16 encoding);
		static short 	GetVF(Classjava_awt_Font *font, int16 encoding);
private:
		void 			InitCommon(ConstStringPtr romanfn, short selector);

		// static function
		static short	getVF(int16 encoding, Str255 romanfn, short selector);
		//	Convienent private
		static short	getFontNumber(Str255 fname);

};

class UEncoding {
	public:
		static ResIDT	GetTextTextTraitsID(int16 encoding);
		static ResIDT	GetButtonTextTraitsID(int16 encoding);
		static short	GetScriptID(int16 encoding);
		static Boolean	IsMacRoman(int16 encoding);
};
