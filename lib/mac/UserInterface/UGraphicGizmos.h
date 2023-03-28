// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	UGraphicGizmos.h					  ©1996 Netscape. All rights reserved.
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

#pragma once

#include <PP_Prefix.h>
#include <LPane.h>

#include "UStdBevels.h"

#ifndef __TEXTEDIT__
#include <TextEdit.h>
#endif

#ifndef __ICONS__
#include <Icons.h>
#endif

enum {
	teFlushBottom = teFlushRight,
	teFlushTop = teFlushLeft
};

enum {
	wHiliteLight 	= 5,
	wHiliteDark,
	wTitleBarLight,
	wTitleBarDark,
	wDialogLight,
	wDialogDark,
	wTingeLight,
	wTingeDark
};

// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	¥	Class UGraphicGizmos
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

class UGraphicGizmos
{
	public:

		static void		LoadBevelTraits(
							ResIDT 				inBevelTraitsID,
							SBevelColorDesc& 	outBevelDesc);
							
		static void 	BevelRect(
								const Rect&		inRect,
								Int16 			inBevel,
								Int16 			inTopColor,
								Int16			inBottomColor);

		static void 	BevelPartialRect(
								const Rect &	inRect,
								Int16 			inBevel,
								Int16 			inTopColor,
								Int16			inBottomColor,
								const SBooleanRect&	inSides);

		static void 	PaintDisabledRect(
								const Rect&		inRect,
								Uint16			inTint = 0x8000,
								Boolean			inMakeLighter = true);

		static void 	CenterRectOnRect(
								Rect			&inRect,
								const Rect		&inOnRect);

		static void		CenterStringInRect(
								const StringPtr inString,
								const Rect		&inRect);
			
		static void 	PlaceStringInRect(
								const StringPtr inString,
								const Rect 		&inRect,
								Int16			inHorizJustType = teCenter,
								Int16			inVertJustType = teCenter);
		
		static void 	CalcWindowTingeColors(
								WindowPtr		inWindowPtr,
								RGBColor		&outLightTinge,
								RGBColor		&outDarkTinge);
								
		static RGBColor& FindInColorTable(
								CTabHandle 	inColorTable,
								Int16 			inColorID);
		
		static RGBColor& MixColor(
								const RGBColor& inLightColor,
								const RGBColor&	inDarkColor,
								Int16 inShade);

};

inline Int16 RectWidth(const Rect& inRect)
	{	return inRect.right - inRect.left;		}
inline Int16 RectHeight(const Rect& inRect)
	{	return inRect.bottom - inRect.top;		}


// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	¥	Class CSicn
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

typedef Int16 SicnT[16];

class CSicn
{
	public:
						CSicn();
						CSicn(ResIDT inSicnID, Int16 inIndex = 0);

		void			LoadSicnData(ResIDT inSicnID, Int16 inIndex = 0);

		void			Plot(Int16 inMode = srcCopy);
		void			Plot(const Rect &inRect, Int16 inMode = srcCopy);
				
		operator		Rect*();
		operator		Rect&();
		
	protected:
		void			CopyImage(const Rect &inRect, Int16 inMode);
		
		SicnT			mSicnData;
		Rect			mSicnRect;
};

inline CSicn::operator Rect*()
	{	return &mSicnRect;		}
inline CSicn::operator Rect&()
	{ 	return mSicnRect;  		}
	
