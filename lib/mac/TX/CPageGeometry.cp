// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	CPageGeometry.cp					  ©1995 Netscape. All rights reserved.
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

#include "CPageGeometry.h"
#include "CTextRenderer.h"


// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	¥	
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

CPageGeometry::CPageGeometry()
{
}

// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	¥	
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

CPageGeometry::CPageGeometry(CLineHeights* inLineHeights)
{
	SetRect(&mMargins, 0, 0, 0, 0);
	mFrameBoundsSize.height = 0;
	mFrameBoundsSize.width = 0;
	
	mLineHeights = inLineHeights;
}

// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	¥	
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

CPageGeometry::~CPageGeometry()
{
}

// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	¥	
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
// Page Coordinates will always be normalized

void CPageGeometry::GetPageFrameBounds(TLongRect& outBounds) const
{
	SDimension32 theDims;
	GetPageFrameDimensions(theDims);
	outBounds.top = 0;
	outBounds.left = 0;
	outBounds.bottom = theDims.height;
	outBounds.right = theDims.width;
}

// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	¥	
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

void CPageGeometry::GetActualPageFrameBounds(TLongRect& outBounds) const
{
	outBounds.left = 0;
	outBounds.top = 0;
	outBounds.right = GetTotalWidth();
	outBounds.bottom = GetTotalHeight();	
}

// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	¥	
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

void CPageGeometry::GetActualPageFrameDimensions(SDimension32& outDims) const
{
	outDims.width = GetTotalWidth();
	outDims.height = GetTotalHeight();
}

// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	¥	
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

void CPageGeometry::GetPageFrameDimensions(SDimension32& outDims) const
{
	outDims.width = mFrameBoundsSize.width;
	outDims.height = mFrameBoundsSize.height;
}

// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	¥	
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

void CPageGeometry::SetPageFrameDimensions(
	const SDimension32& inNewDims,						// inNewSize is not scaled
	CDisplayChanges* 	ioChanges)
{
	SDimension32 theRealDims = inNewDims;
	if (inNewDims.height == 0)
		{
		theRealDims.width = inNewDims.width;
		theRealDims.height = max_Int32;
		}

	if ((mFrameBoundsSize.width == theRealDims.width) && (mFrameBoundsSize.height == theRealDims.height))
		{
		if (ioChanges)
			ioChanges->SetChangesKind(kNoChanges);
		}
	else
		{
		if (ioChanges)
			ioChanges->SetChangesKind((mFrameBoundsSize.width != theRealDims.width) ? kNeedHReformat : kNeedVReformat);
	
		mFrameBoundsSize = theRealDims;
		}

}

// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	¥	
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
// page coordinates

void CPageGeometry::GetPageTextBounds(TLongRect& outBounds) const
{
	SDimension32 theDims;
	GetPageFrameDimensions(theDims);

	outBounds.top = mMargins.top;
	outBounds.left = mMargins.left;
	outBounds.bottom = theDims.height - mMargins.bottom;
	outBounds.right = theDims.width - mMargins.right;
}

// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	¥	
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

void CPageGeometry::GetActualPageTextBounds(TLongRect& outBounds) const
{
	GetActualPageFrameBounds(outBounds);
	
	outBounds.top += mMargins.top;
	outBounds.left += mMargins.left;
	outBounds.bottom -= mMargins.bottom;
	outBounds.right -= mMargins.right;
}

// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	¥	
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

void CPageGeometry::GetActualPageTextDimensions(SDimension32& outDims) const
{
	GetActualPageFrameDimensions(outDims);

	outDims.width -= (mMargins.top + mMargins.bottom);
	outDims.height -= (mMargins.left + mMargins.bottom);
}

// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	¥	
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

void CPageGeometry::GetPageTextDimensions(SDimension32& outDims) const
{
	outDims.height = mFrameBoundsSize.height - (mMargins.top + mMargins.bottom);
	outDims.width = mFrameBoundsSize.width - (mMargins.left + mMargins.right);
}

// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	¥	
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
										
void CPageGeometry::SetPageTextDimensions(
	const SDimension32& inNewDims,						// inNewSize is not scaled
	CDisplayChanges* 	ioChanges)
{
	// FIX ME!!! we need to decide what to do here.  We have two choices.
	//	1. we could let the page frame size stay fixed and eat the size change with
	//		changes to the margins.
	//	2. we could force the page frame size to change keeping the same margins.
	
	Assert_(false);
}

// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	¥	
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

void CPageGeometry::GetPageMargins(Rect& outMargins) const
{
	outMargins = mMargins;
}

// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	¥
//
//	Setting the margins will not change the frame size.
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

void CPageGeometry::SetPageMargins(const Rect& inNewMargins, CDisplayChanges* ioChanges)
{
	Int16 theResult;
	if (::EqualRect(&inNewMargins, &mMargins))
		theResult = kNoChanges;
	else
		{
		mMargins = inNewMargins;
		theResult = kRedrawAll + kCheckSize;
		}
	
	if (ioChanges)
		ioChanges->SetChangesKind(theResult);
}

// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	¥	
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

Int32 CPageGeometry::GetTotalHeight(void) const
{
	Rect margins;
	GetPageMargins(margins);
	
	Int32 theTotalHeight = mLineHeights->GetTotalLinesHeight() + (margins.top + margins.bottom);
	
	return theTotalHeight;
}

// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	¥	
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

Int32 CPageGeometry::GetTotalWidth(void) const
{
	SDimension32 theDims;
	GetPageFrameDimensions(theDims);

	return theDims.width;
}

// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	¥	
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

Int32 CPageGeometry::GetLineFormatWidth(Int32 inLineNumber) const
{
	SDimension32 theDims;
	GetPageTextDimensions(theDims);

	return theDims.width;
}

// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	¥	
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

Int32 CPageGeometry::GetLineMaxWidth(Int32 inLineNumber) const
{
	SDimension32 theDims;
	GetPageTextDimensions(theDims);

	return theDims.width;
}



// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	¥	
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

void CPageGeometry::LocalToPageRect(
	LView*				inViewContext,
	const Rect& 		inLocalRect,
	TLongRect&			outPageRect) const
{
	SPoint32 thePageTopLeft, thePageBotRight;
	LocalToPagePoint(inViewContext, topLeft(inLocalRect), thePageTopLeft);
	LocalToPagePoint(inViewContext, botRight(inLocalRect), thePageBotRight);
	
	outPageRect.top = thePageTopLeft.v;
	outPageRect.left = thePageTopLeft.h;
	outPageRect.bottom = thePageBotRight.v;
	outPageRect.right = thePageBotRight.h;
}

// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	¥	
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

void CPageGeometry::PageToLocalRect(
	LView*				inViewContext,
	const TLongRect& 	inPageRect,
	Rect&				outLocalRect) const
{
	SPoint32 thePageTopLeft, thePageBotRight;
	thePageTopLeft.h = inPageRect.left;
	thePageTopLeft.v = inPageRect.top;
	thePageBotRight.h = inPageRect.right;
	thePageBotRight.v = inPageRect.bottom;

	PageToLocalPoint(inViewContext, thePageTopLeft, topLeft(outLocalRect));
	PageToLocalPoint(inViewContext, thePageBotRight, botRight(outLocalRect));
}

// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	¥	
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

void CPageGeometry::LocalToPagePoint(
	LView*				inViewContext,
	const Point& 		inLocalPoint,
	SPoint32&			outPagePoint) const
{
	// First convert local to image
	SPoint32 theImagePoint;
	inViewContext->LocalToImagePoint(inLocalPoint, theImagePoint);

	outPagePoint.h = theImagePoint.h;
	outPagePoint.v = theImagePoint.v;

	// FIX ME!!! scale based on view zoom
}

// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	¥	
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

void CPageGeometry::PageToLocalPoint(
	LView*				inViewContext,
	const SPoint32& 	inPagePoint,
	Point&				outLocalPoint) const
{
	SPoint32 thePageCopy = inPagePoint;

	// FIX ME!!! scale based on view zoom
	
	// convert from image to local coordinates
	inViewContext->ImageToLocalPoint(thePageCopy, outLocalPoint);
}


// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	¥	
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
		
Int16 CPageGeometry::HorizPageToLocal(
	LView* 				inViewContext,
	Int32				inPageHPixel) const
{
	// FIX ME!!! scale based on view zoom
	
	SPoint32 theImagePoint;
	theImagePoint.h = inPageHPixel;
	theImagePoint.v = 0;
	
	Point theLocalPoint;
	inViewContext->ImageToLocalPoint(theImagePoint, theLocalPoint);
	
	return theLocalPoint.h;
}

// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	¥	
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
		
Int16 CPageGeometry::VertPageToLocal(
	LView* 				inViewContext,
	Int32				inPageVPixel) const
{
	// FIX ME!!! scale based on view zoom
	
	SPoint32 theImagePoint;
	theImagePoint.h = 0;
	theImagePoint.v = inPageVPixel;
	
	Point theLocalPoint;
	inViewContext->ImageToLocalPoint(theImagePoint, theLocalPoint);
	
	return theLocalPoint.v;
}

// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	¥	
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

void CPageGeometry::CharToPagePoint(
	LView*				inViewContext,
	Int32				inOffset,
	SPoint32&			outPagePoint) const
{
//	long lineNo = mFormatter->OffsetToLine(theChar);
//	SPoint32 linePoint;
//	fFrames->LineToPoint(lineNo, &linePoint);
//	
//	
//	*top = linePoint.v;
//	
//	if (trailEdge) {
//		DoLineLayout(lineNo);
//		*trailEdge = linePoint.h + fLine->CharacterToPixel(theChar);
//	}
//	
//	if (height)
//		*height = short(mLineHeights->GetLineRangeHeight(lineNo, lineNo));
}

// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	¥	
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

void CPageGeometry::CharToLocalPoint(	
	LView*				inViewContext,
	Int32				inOffset,
	Point&				outLocalPoint) const
{
//	SPoint32 thePagePoint;
//	CharToPoint(theChar, thePagePoint, height);
//
//	Point theLocalPoint;
//	mGeometry->PageToLocalPoint(inViewContext, thePagePoint, theLocalPoint);
//
//	return theLocalPoint;
}

// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	¥	
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

void CPageGeometry::LineToPagePoint(
	LView*				inViewContext,
	Int32				inLineNumber,
	SPoint32&			outPagePoint) const
{
	TLongRect theTextBounds;
	GetPageTextBounds(theTextBounds);
		
	TOffsetPair theFrameLines;
	mLineHeights->GetTotalLineRange(theFrameLines);
		
	outPagePoint.h = theTextBounds.left;
		
	if (inLineNumber > theFrameLines.firstOffset)
			theTextBounds.top += mLineHeights->GetLineRangeHeight(theFrameLines.firstOffset, inLineNumber - 1);
			
	outPagePoint.v = theTextBounds.top;
}

// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	¥	
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

void CPageGeometry::LineToLocalPoint(
	LView*				inViewContext,
	Int32				inLineNumber,
	Point&				outLocalPoint) const
{
	SPoint32 thePagePoint;
	LineToPagePoint(inViewContext, inLineNumber, thePagePoint);
	PageToLocalPoint(inViewContext, thePagePoint, outLocalPoint);
}

// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	¥	
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

Int32 CPageGeometry::PagePointToLineNumber(
	LView*				inViewContext,
	const SPoint32&		inPagePoint,
	Boolean&			outOutsideLine) const
{
	outOutsideLine = false;
	
	TLongRect theTextBounds;
	GetPageTextBounds(theTextBounds);
	
	Int32 thePixel = inPagePoint.v - theTextBounds.top;
	if (thePixel < 0)
		thePixel = 0;
	
	Int32 theLineNumber = -1;
	
	TOffsetPair thePageLines;
	if (mLineHeights->GetTotalLineRange(thePageLines))
		{
		theLineNumber = mLineHeights->PixelToLine(&thePixel, thePageLines.firstOffset);
		if (theLineNumber > thePageLines.lastOffset)
			{
			outOutsideLine = true;
			theLineNumber = thePageLines.lastOffset;
			}
		}
	
	return theLineNumber;
}

// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	¥	
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

Int32 CPageGeometry::LocalPointToLineNumber(
	LView*				inViewContext,
	const Point&		inLocalPoint,
	Boolean&			outOutsideLine) const
{
	SPoint32 thePagePoint;
	LocalToPagePoint(inViewContext, inLocalPoint, thePagePoint);

	return PagePointToLineNumber(inViewContext, thePagePoint, outOutsideLine);
}



