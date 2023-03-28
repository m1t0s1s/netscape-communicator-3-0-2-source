// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	UGraphicGizmos.cp					  ©1996 Netscape. All rights reserved.
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

#ifdef PowerPlant_PCH
#include PowerPlant_PCH
#endif

#include <UMemoryMgr.h>
#include <UDrawingState.h>
#include <UDrawingUtils.h>
#include "UGraphicGizmos.h"

#ifndef __PALETTES__
#include <Palettes.h>
#endif

// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	¥	LoadBevelTraits
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

void UGraphicGizmos::LoadBevelTraits(
	ResIDT 				inBevelTraitsID,
	SBevelColorDesc& 	outBevelDesc)
{
	Assert_(inBevelTraitsID != resID_Undefined);
	
	StResource theBevelResource('BvTr', inBevelTraitsID);
	::BlockMoveData(*(theBevelResource.mResourceH), &outBevelDesc, sizeof(SBevelColorDesc));
}

// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	¥	BevelRect
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

void UGraphicGizmos::BevelRect(
	const Rect 		&inRect,
	Int16 			inBevel,
	Int16 			inTopColor,
	Int16			inBottomColor)
{
	StColorPenState thePenSaver;
	thePenSaver.Normalize();

	Rect theBevelRect = inRect;
	theBevelRect.bottom--;
	theBevelRect.right--;

	StDeviceLoop theLoop(inRect);
	Int16 theDepth;
	while (theLoop.NextDepth(theDepth))
		{
		if (theDepth < 4)
			{
			// Draw a monochrome bevel
			QDGlobals* theQDPtr = UQDGlobals::GetQDGlobals();
			PatPtr theTopPat, theBottomPat;
	
			if (inBevel < 0)
				{
				inBevel = -inBevel;
				theTopPat = &theQDPtr->black;
				theBottomPat = &theQDPtr->white;
				}
			else
				{
				theTopPat = &theQDPtr->white;
				theBottomPat = &theQDPtr->black;
				}
	
			for (Int16 index = 1; index <= inBevel; index++)
				{
				::PenPat(theTopPat);
				::MoveTo(theBevelRect.right, theBevelRect.top);
				::LineTo(theBevelRect.left, theBevelRect.top);
				::LineTo(theBevelRect.left, theBevelRect.bottom);
				
				::PenPat(theBottomPat);
				::LineTo(theBevelRect.right, theBevelRect.bottom);
				::LineTo(theBevelRect.right, theBevelRect.top);
		
				::InsetRect(&theBevelRect, 1, 1);
				}
			}
		else
			{
			// Draw a color bevel
			Int16 theTopColor, theBottomColor;
	
			if (inBevel < 0)
				{
				inBevel = -inBevel;
				theTopColor = inBottomColor;
				theBottomColor = inTopColor;
				}
			else
				{
				theTopColor = inTopColor;
				theBottomColor = inBottomColor;
				}

			for (Int16 index = 1; index <= inBevel; index++)
				{
				::PmForeColor(theTopColor);
				::MoveTo(theBevelRect.right, theBevelRect.top);
				::LineTo(theBevelRect.left, theBevelRect.top);
				::LineTo(theBevelRect.left, theBevelRect.bottom);
				
				::PmForeColor(theBottomColor);
				::LineTo(theBevelRect.right, theBevelRect.bottom);
				::LineTo(theBevelRect.right, theBevelRect.top);
				
				::InsetRect(&theBevelRect, 1, 1);
				}
			}
		}
}

// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	¥	BevelPartialRect
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

void UGraphicGizmos::BevelPartialRect(
	const Rect&			inRect,
	Int16 				inBevel,
	Int16 				inTopColor,
	Int16				inBottomColor,
	const SBooleanRect&	inSides)
{
	StColorPenState thePenSaver;
	thePenSaver.Normalize();

	Rect theBevelRect = inRect;
	theBevelRect.bottom--;
	theBevelRect.right--;

// FIX ME!!! need to fix mono drawing
//	StDeviceLoop theLoop(inRect);
//	Int16 theDepth;
//	while (theLoop.NextDepth(theDepth))

	for (Int16 index = 1; index <= inBevel; index++)
		{
		::PmForeColor(inTopColor);
		if (inSides.left)
			{
			::MoveTo(theBevelRect.left, theBevelRect.top);
			::LineTo(theBevelRect.left, theBevelRect.bottom);
			}
			
		if (inSides.top)
			{
			::MoveTo(theBevelRect.right, theBevelRect.top);
			::LineTo(theBevelRect.left, theBevelRect.top);
			}

		::PmForeColor(inBottomColor);
		if (inSides.right)
			{
			::MoveTo(theBevelRect.right, theBevelRect.top);
			::LineTo(theBevelRect.right, theBevelRect.bottom);
			}
			
		if (inSides.bottom)
			{
			::MoveTo(theBevelRect.left, theBevelRect.bottom);
			::LineTo(theBevelRect.right, theBevelRect.bottom);
			}
		
		::InsetRect(&theBevelRect, 1, 1);
		}
}

// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	¥	PaintDisabledRect
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

void UGraphicGizmos::PaintDisabledRect(
	const Rect&		inRect,
	Uint16			inTint,
	Boolean			inMakeLighter)
{
	StColorPenState		theSavedState;
	theSavedState.Normalize();
	
	RGBColor theOpColor;
	theOpColor.red = theOpColor.blue = theOpColor.green = inTint;
	
	RGBColor theNewColor;
	if (inMakeLighter) 
		theNewColor.red = theNewColor.blue = theNewColor.green = 0xFFFF;
	else 
		theNewColor.red = theNewColor.blue = theNewColor.green = 0x0000;
	
	::OpColor(&theOpColor);
	::RGBForeColor(&theNewColor);
	::PenMode(blend);
	::PenPat(&qd.black);
	::PaintRect(&inRect);
}

// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	¥	CenterRectOnRect
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

void UGraphicGizmos::CenterRectOnRect(
	Rect			&inRect,
	const Rect		&inOnRect)
{
	Int16 theRectHeight = RectHeight(inRect);
	Int16 theRectWidth = RectWidth(inRect);
				
	inRect.top = inOnRect.top + ((RectHeight(inOnRect) - theRectHeight) / 2);
	inRect.bottom = inRect.top + theRectHeight;
	inRect.left = inOnRect.left + ((RectWidth(inOnRect) - theRectWidth) / 2);
	inRect.right = inRect.left + theRectWidth;
}

// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	¥	CenterStringInRect
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

void UGraphicGizmos::CenterStringInRect(
	const StringPtr inString,
	const Rect		&inRect)
{
	FontInfo theFontInfo;
	::GetFontInfo(&theFontInfo);
		
	Point thePoint;
	thePoint.h = inRect.left + ((RectWidth(inRect) - StringWidth(inString)) / 2);
		
	Int16 theFontHeight = theFontInfo.ascent + theFontInfo.descent + 1;
	thePoint.v = inRect.top + ((RectHeight(inRect) - theFontHeight) / 2) + theFontInfo.ascent;
	
	MoveTo(thePoint.h, thePoint.v);
	::DrawString(inString);
}

// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	¥	PlaceStringInRect
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

void UGraphicGizmos::PlaceStringInRect(
	const StringPtr inString,
	const Rect 		&inRect,
	Int16			inHorizJustType,
	Int16			inVertJustType)
{
	Point thePoint;
	FontInfo theFontInfo;
	::GetFontInfo(&theFontInfo);

	Int16 theFontHeight = theFontInfo.ascent + theFontInfo.descent + theFontInfo.leading;
	Int16 theStringWidth = ::StringWidth(inString);

	switch (inHorizJustType)
		{
		case teFlushRight:
			thePoint.h = inRect.right - theStringWidth;
			break;
			
		case teFlushLeft:
			thePoint.h = inRect.left;
			break;
				
		case teCenter:
		default:
			thePoint.h = inRect.left + ((RectWidth(inRect) - theStringWidth) / 2);
			break;
		}
		
	switch (inVertJustType)
		{
		case teFlushTop:
			thePoint.v = inRect.top + theFontHeight;
			break;
			
		case teFlushBottom:
			thePoint.v = inRect.bottom - theFontInfo.descent;
			break;
			
		case teCenter:
		default:
			thePoint.v = inRect.top + ((RectHeight(inRect) - theFontHeight) / 2) + theFontInfo.ascent + theFontInfo.leading;
			
			break;
		}
	
	::MoveTo(thePoint.h, thePoint.v);
	::DrawString(inString);
}

// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	¥	CalcWindowTingeColors
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

void UGraphicGizmos::CalcWindowTingeColors(WindowPtr inWindowPtr, RGBColor &outLightTinge, RGBColor &outDarkTinge)
{
	AuxWinHandle theAuxHandle;						// the default AuxWindow data
	::GetAuxWin(inWindowPtr, &theAuxHandle);

	if (theAuxHandle != NULL)
		{
		CTabHandle theColorTable = (**theAuxHandle).awCTable;

		// do we have a system 7 sized window color table?
		if ((**theColorTable).ctSize > wTitleBarColor)
			{
			// yes, now search for the tinge color, start at the end
			// because our tinge colors are usually last in the list.
			Boolean bLightFound = false;
			Boolean	bDarkFound = false;
			for (Int16 theIndex = (**theColorTable).ctSize; theIndex > 0; theIndex--)
				{
				if ((**theColorTable).ctTable[theIndex].value == wTingeLight)
					{
					outLightTinge = (**theColorTable).ctTable[theIndex].rgb;
					bLightFound = true;
					}
				else if ((**theColorTable).ctTable[theIndex].value == wTingeDark)
					{
					outDarkTinge = (**theColorTable).ctTable[theIndex].rgb;
					bDarkFound = true;
					}
				
				if (bLightFound && bDarkFound)
					break;	
				}
			}
		}
		
	// FIX ME!!! we'll need to set the colors if we didnt find them!	
}

// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	¥	FindInColorTable
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

RGBColor& UGraphicGizmos::FindInColorTable(CTabHandle inColorTable, Int16 inColorID)
{
	RGBColor theFoundColor = { 0, 0, 0 };
	
	for (Int16 theIndex = (**inColorTable).ctSize; theIndex > 0; theIndex--)
		{
		if ((**inColorTable).ctTable[theIndex].value == inColorID)
			{
			theFoundColor = (**inColorTable).ctTable[theIndex].rgb;	
			break;
			}
		}
	
	return theFoundColor;
}


RGBColor& UGraphicGizmos::MixColor(
	const RGBColor& inLightColor,
	const RGBColor&	inDarkColor,
	Int16 inShade)
{
	RGBColor theMixedColor;
	
	inShade = 0x0F - inShade;
		// This is necessary because we give shades between light and
		// dark (0% is light), but for colors, $0000 is black and $FFFF 
		// is dark.

	theMixedColor.red	= (Int32) (inLightColor.red   - inDarkColor.red)   * inShade / 15 + inDarkColor.red;
	theMixedColor.green = (Int32) (inLightColor.green - inDarkColor.green) * inShade / 15 + inDarkColor.green;
	theMixedColor.blue  = (Int32) (inLightColor.blue  - inDarkColor.blue)  * inShade / 15 + inDarkColor.blue;

	return theMixedColor;
}

// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	¥	Class CSicn
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

CSicn::CSicn()
{
	::SetRect(&mSicnRect, 0, 0, 16, 16);
}

CSicn::CSicn(ResIDT inSicnID, Int16 inIndex)
{
	::SetRect(&mSicnRect, 0, 0, 16, 16);
	LoadSicnData(inSicnID, inIndex);
}

void CSicn::LoadSicnData(ResIDT inSicnID, Int16 inIndex)
{
	StResource theSicnList('SICN', inSicnID);
	ThrowIf_((::GetHandleSize(theSicnList.mResourceH) / sizeof(SicnT)) <= inIndex);

	StHandleLocker theLocker(theSicnList);
	::BlockMoveData((Ptr)(*((SicnT *)(*theSicnList.mResourceH) + inIndex)), mSicnData, sizeof(SicnT));
}

void CSicn::Plot(Int16 inMode)
{
	CopyImage(mSicnRect, inMode);
}

void CSicn::Plot(const Rect &inRect, Int16 inMode)
{
	CopyImage(inRect, inMode);
}

void CSicn::CopyImage(const Rect &inRect, Int16 inMode)
{
	// Set up a BitMap for the sicn
	BitMap theSourceBits;
	theSourceBits.baseAddr = (Ptr)(mSicnData);
	theSourceBits.rowBytes = 2;
	::SetRect(&theSourceBits.bounds, 0, 0, 16, 16);

	// draw the small icon in the current grafport
	::CopyBits(&theSourceBits, &(UQDGlobals::GetCurrentPort()->portBits),
				 &theSourceBits.bounds, &inRect, inMode, nil);
}

