// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

#include "TextensionCommon.h"
#include <URegions.h>
#include <Windows.h>
#include <Scrap.h>
#include <stdlib.h>

//***************************************************************************************************

char 		gHasManyScripts;
Boolean g2Directions;
Boolean	g2Bytes;

Boolean gHasColor;

Boolean gTextensionImaging;

short gDefaultTabVal;

long gMinAppMemory;  //the amount guaranteed after any edit operation

//***************************************************************************************************


static Handle AllocateBlock(Size countBytes, char useTemp, OSErr* err)
{
	if (useTemp)
		return TempNewHandle(countBytes, err);
	else {
		Handle result = NewHandle(countBytes);
		*err = MemError();
		return result;
	}
}
//***************************************************************************************************

Handle AllocateTempHandle(Size countBytes, char lockIt, OSErr* err)
{
	register char useTemp = (TempFreeMem() >= FreeMem()) ? 1 : 0;
	
	register Handle result = AllocateBlock(countBytes, useTemp, err);
	
	if (!result) {
		useTemp ^= 1;
		result = AllocateBlock(countBytes, useTemp, err);
	}

	if (result && lockIt) {
		if (!useTemp)
			MoveHHi(result);
			
		HLock(result);
	}
	
	return result;
}
//***************************************************************************************************

long Clip(long val, long lowerBound, long upperBound)
{
	if (val <= lowerBound)
		return lowerBound;
		
	if (val >= upperBound)
		return upperBound;
	return val;
}
//***************************************************************************************************

void SwapShort(short* short1, short* short2)
{
	short temp = *short1;
	*short1 = *short2;
	*short2 = temp;
}
//***************************************************************************************************

void SwapLong(long* long1, long* long2)
{
	long temp = *long1;
	*long1 = *long2;
	*long2 = temp;
}
//***************************************************************************************************

Boolean IsColorPort()
{
	return (gHasColor) && ((((qd.thePort)->portBits).rowBytes & 0x8000) != 0);
}
//***************************************************************************************************

Boolean IsScript2Bytes(short theScript)
{
	return (GetScriptVariable(theScript, smScriptFlags) & (1 << smsfSingByte)) == 0;
}
//***************************************************************************************************

void MyKeyScript(short newScript)
/*the check for the curr key script is made since KeyScript is quite expensive : flushes the events
and causes subsequent HiliteMenu to redraw menu bar (HiliteMenu is called in the main event loop in MacApp!)*/
{
	if (GetScriptManagerVariable(smKeyScript) != newScript)
		KeyScript(newScript);
}
//***************************************************************************************************

short MonoFontToScript(short aFont)
{
	aFont -= 16384; //16384 is the first non-roman font id
	if (aFont >= 0)
		return (aFont >> 9) + 1;

	return 0;
}
//***************************************************************************************************


Boolean ClipFurther(Rect* moreClipRect, RgnHandle origClip)
{
	RgnHandle currClip = qd.thePort->clipRgn;
	
	if (IsRectRgn(currClip))
		{
		if (!SectRect(moreClipRect, &((*currClip)->rgnBBox), moreClipRect))
			return false;
		
		GetClip(origClip);
		ClipRect(moreClipRect);
		
		return true;
		}
	else
		{
		StRegion theTempRgn;
		GetClip(origClip);
		
		RectRgn(theTempRgn, moreClipRect);
		SectRgn(theTempRgn, currClip, currClip);
		
		if (EmptyRgn(currClip))
			{
			SetClip(origClip);
			return false;
			}
		else
			{
			*moreClipRect = (*currClip)->rgnBBox;
			return true;
			}
		}
}
//*************************************************************************************************

Boolean CalcClipRect(Rect* theRect)
{
	RgnHandle currClip = qd.thePort->clipRgn;
	
	if (IsRectRgn(currClip))
		{
		return SectRect(&((*(currClip))->rgnBBox), theRect, theRect);
		}
	else
		{
		StRegion theTempRgn;
		
		RectRgn(theTempRgn, theRect);
		SectRgn(currClip, theTempRgn, theTempRgn);
		
		Boolean result = SectRect(&((*(theTempRgn))->rgnBBox), theRect, theRect);
				
		return result;
		}
}
//*************************************************************************************************

void InvalSectRect(Rect* theRect, RgnHandle theClip /*= nil*/)
{
	if (!theClip)
		theClip = qd.thePort->clipRgn;
		
	if (SectRect(&((*theClip)->rgnBBox), theRect, theRect))
		InvalRect(theRect);
}
//*************************************************************************************************

short GetMaxPixDepth(const Rect& aRect)
{
	if (!IsColorPort())
		return 1;
	
	Rect globRect = aRect;
	LocalToGlobal(PointPtr(&globRect));
	LocalToGlobal(PointPtr(&globRect.bottom));
	
	GDHandle deepDevHdl = GetMaxDevice(&globRect);
	return (deepDevHdl) ? (*((*deepDevHdl)->gdPMap))->pixelSize : 1;
}
//************************************************************************************************

void SetCursorIBeam()
{
	unsigned char savedRomMap = LMGetROMMapInsert();
	LMSetROMMapInsert(1);
	SetCursor(*GetCursor(iBeamCursor));
	LMSetROMMapInsert(savedRomMap);
}
//************************************************************************************************

long LongRoundUp(long aNumber, short aModulus)
{
	ldiv_t divResult = ldiv(aNumber, aModulus);
	return (divResult.rem) ? divResult.quot+1 : divResult.quot;
}
//************************************************************************************************

short ShortRoundUp(short aNumber, short aModulus)
{
	div_t divResult = div(aNumber, aModulus);
	return (divResult.rem) ? divResult.quot+1 : divResult.quot;
}
//************************************************************************************************

short ScaleVal(short valToScale, short numer, short denom, Boolean roundUp /*= true*/)
{
	if (numer == denom)
		return valToScale;
	else {
		short times = valToScale * numer;
		return (roundUp) ? ShortRoundUp(times, denom) : times / denom;
	}
}
//************************************************************************************************

long LongScaleVal(long valToScale, short numer, short denom, Boolean roundUp /*= true*/)
{
	if (numer == denom)
		return valToScale;
	
	if (valToScale > 0x7FFF)
		return (valToScale / denom) * numer;
	else {
		long times = valToScale * numer;
		return (roundUp) ? LongRoundUp(times, denom) : times / denom;
	}
}
//************************************************************************************************

Fixed FixScaleVal(Fixed valToScale, short numer, short denom)
{
	return (numer == denom) ? valToScale : FixMul(valToScale, FixDiv(numer, denom));
}
//************************************************************************************************


#ifndef txtnRulers
Fixed GetDefaultTabWidth(Fixed tabPixStart, short numer /*= 1*/, short denom /*= 1*/)
{
	short scaledTabVal = ScaleVal(gDefaultTabVal, numer, denom);
	
	short value = (FixRound(tabPixStart)/scaledTabVal + 1) * scaledTabVal;

	return LongToFixed(value) - tabPixStart;
}
//************************************************************************************************
#endif


Boolean TLongRect::IsPointInside(const SPoint32& thePt) const
{
	return (thePt.h >= left) && (thePt.h <= right)
			&& (thePt.v >= top) && (thePt.v <= bottom);
}
//***************************************************************************************************

void TLongRect::Inset(long h, long v)
{
	left += h;
	right -= h;
	top += v;
	bottom -= v;
}
//***************************************************************************************************

void TLongRect::Offset(long h, long v)
{
	left += h;
	right += h;
	top += v;
	bottom += v;
}
//***************************************************************************************************


Boolean TLongRect::operator==(const TLongRect& aRect) const
{
	return (top == aRect.top) && (left == aRect.left)
				&& (right == aRect.right) && (bottom == aRect.bottom);
}
//***************************************************************************************************

Boolean TLongRect::Sect(const TLongRect& src, TLongRect* intersection) const
{

	intersection->top = MaxLong(top, src.top);
	intersection->left = MaxLong(left, src.left);
	intersection->bottom = MinLong(bottom, src.bottom);
	intersection->right = MinLong(right, src.right);
	
	if ((intersection->top >= intersection->bottom) || (intersection->left >= intersection->right)) {
		intersection->top = 0;
		intersection->left = 0;
		intersection->bottom = 0;
		intersection->right = 0;
		
		return false;
	}
	else
		return true;
}
//***************************************************************************************************

void TLongRect::Union(const TLongRect& src, TLongRect* theUnion) const
{
	theUnion->top = MinLong(top, src.top);
	theUnion->left = MinLong(left, src.left);
	theUnion->bottom = MaxLong(bottom, src.bottom);
	theUnion->right = MaxLong(right, src.right);
}
//***************************************************************************************************


Boolean TOffset::operator==(const TOffset& rhs) const
{
	return (offset == rhs.offset) && (trailEdge == rhs.trailEdge);
}	
//***************************************************************************************************


TOffsetRange::TOffsetRange(const TOffset& start, const TOffset& end)
{
	rangeStart = start;
	rangeEnd = end;
}
//***************************************************************************************************

TOffsetRange::TOffsetRange(long offsetStart, long offsetEnd, Boolean trailStart, Boolean trailEnd)
{
	rangeStart.Set(offsetStart, trailStart);
	rangeEnd.Set(offsetEnd, trailEnd);
}
//***************************************************************************************************

void TOffsetRange::Set(long offsetStart, long offsetEnd, Boolean trailStart, Boolean trailEnd)
{
	rangeStart.Set(offsetStart, trailStart);
	rangeEnd.Set(offsetEnd, trailEnd);
}
//***************************************************************************************************

Boolean TOffsetRange::operator==(const TOffsetRange& rhs) const
{
	return (rangeStart == rhs.rangeStart) && (rangeEnd == rhs.rangeEnd);
}
//***************************************************************************************************

void TOffsetRange::Offset(long value)
{
	rangeStart.offset += value;
	rangeEnd.offset += value;
}
//***************************************************************************************************

void TOffsetRange::CheckBounds()
{
	if (rangeStart.offset > rangeEnd.offset) {
		long temp = rangeStart.offset;
		rangeStart.offset = rangeEnd.offset;
		rangeEnd.offset = temp;
	}
}	



#ifdef powerc

pascal void AddToArrayElements(long value, char* array, long count, short elemSize)
{
	while (--count >= 0) {
		*(long*)(array) += value;
		array += elemSize;
	}
}
//***************************************************************************************************

char CompareBytes(register unsigned char* ptr1, register unsigned char* ptr2, long count)
{
	while (--count >= 0) {
		if (*ptr1++ != *ptr2++)
			return 0;
	}
	
	return 1;
}
//***************************************************************************************************

pascal short GetCtrlCharOffset(unsigned char* chars, short len, unsigned char* theChar)
//returns the offset of the char, -1 if not found
{
const unsigned char kMaxCtrlChar = 0x1F;

	register short offset = 0;
	register unsigned char maxCtrlChar = kMaxCtrlChar;
	
	while (--len >= 0) {
		register unsigned char ctrlChar = *chars++;
		
		if (ctrlChar <= maxCtrlChar) {
			*theChar = ctrlChar;
			return offset;
		}
		
		++offset;
	}
	
	return -1;
}
//***************************************************************************************************

pascal short SearchChar(unsigned char theChar, unsigned char* chars, short len)
//returns the offset of the char from "chars", -1 if not found
{
	register short offset = 0;
	
	while (--len >= 0) {
		if (*chars++ == theChar)
			return offset;
		
		++offset;
	}
	
	return -1;
}
//***************************************************************************************************

pascal short SearchCharBack(unsigned char theChar, unsigned char* chars, short lenBefore)
//same as SearchChar but search backward
{
	register short offset = 0;
	
	while (--lenBefore >= 0) {
		++offset;
		
		if (*(--chars) == theChar)
			return offset;
	}
	
	return -1;
}
//***************************************************************************************************
#endif //powerc




void CDisplayChanges::SetFormatRange(const TOffsetPair& newRange)
{
	fFormatRange.firstOffset = MinLong(fFormatRange.firstOffset, newRange.firstOffset);
	fFormatRange.lastOffset = MaxLong(fFormatRange.lastOffset, newRange.lastOffset);
}
//*************************************************************************************************

CDisplayChanges::CDisplayChanges()
//so, callers can use the same CDisplayChanges variable many times
{
	fKind = kNoChanges;
	fFormatRange.firstOffset = kMaxLongInt;
	fFormatRange.lastOffset = 0;
}
//*************************************************************************************************

void CDisplayChanges::GetFormatRange(TOffsetPair* formatRange) const
{*formatRange = fFormatRange;}

