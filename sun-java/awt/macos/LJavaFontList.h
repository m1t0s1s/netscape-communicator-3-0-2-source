// =================================================================================
// Copyright Netscape Communication 1996-1997
// Author: Frank Tang <ftang@netscape.com>
// =================================================================================
#pragma once
#include "LFontList.h"
#include "LVFont.h"
extern "C" {
#include <Types.h>
#include <QuickDraw.h>
#include "java_awt_Font.h"
#include "sun_awt_macos_MacFontMetrics.h"
};
class LJavaFontList : public LFontList {
	private:
		short	fSize;
		Style	fStyle;		
	public:
		LJavaFontList();
		LJavaFontList(Classjava_awt_Font	*font);
		LJavaFontList(const LJavaFontList& infont);
		~LJavaFontList();
		
		void	SetFont(Classjava_awt_Font	*font);
		void	SetFontSize(short inSize) {fSize = inSize;};
		void	InitFontMetrics(Classsun_awt_macos_MacFontMetrics *macFontMetrics);		
		//	Used by friend
		void	SetTextFont(int16 encoding)	
						{::TextFont(getFont(encoding) );};
		void	SetTextSize()	
						{ ::TextSize(fSize);};
		void	SetTextStyle()
						{ ::TextFace(fStyle);};
		void	SetTextLatin1Font();
	private:
		//	First level setting private
		void	setFontName(Str255 fontName);
		void	setFontSize(Classjava_awt_Font	*infont) 
						{ fSize = infont->size; };
		void	setFontStyle(Classjava_awt_Font	*font);

		//	Drawing Handling
		void	getFontInfo(int16 encoding, FontInfo& info);
		
		// 	Font Metrics private
		void	fillOverallFontMetrics(int16 encoding, 
									Classsun_awt_macos_MacFontMetrics *macFontMetrics);		
		void	fillPerCharFontMetrics(Classsun_awt_macos_MacFontMetrics *macFontMetrics);		
};
