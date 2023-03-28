// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	CGrayBevelView.cp					  ©1996 Netscape. All rights reserved.
//
//		A beveled gray view that can draw raised, recessed or no bevel
//		for any of its subviews.
//
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

#ifdef PowerPlant_PCH
#include PowerPlant_PCH
#endif

#include <LArray.h>
#include <LArrayIterator.h>
#include <LDataStream.h>
#include <UDrawingState.h>
#include <UDrawingUtils.h>
#include <UResourceMgr.h>
#include <URegions.h>

#include "UStdBevels.h"
#include "UGraphicGizmos.h"
#include "CGrayBevelView.h"

#ifndef __PALETTES__
#include <Palettes.h>
#endif

// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//		¥ CreateGrayBevelViewStream
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

CGrayBevelView *CGrayBevelView::CreateGrayBevelViewStream(LStream *inStream)
{
	return (new CGrayBevelView(inStream));
}
	
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//		¥ CGrayBevelView
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

CGrayBevelView::CGrayBevelView(LStream *inStream)
	:	LView(inStream),
		mBevelList(sizeof(SSubBevel))
{
	*inStream >> mMainBevel;

	ResIDT theBevelTraitsID;
	*inStream >> theBevelTraitsID;
	UGraphicGizmos::LoadBevelTraits(theBevelTraitsID, mBevelColors);
	
	ResIDT theBevelResID;
	*inStream >> theBevelResID;
	StResource theBevelRes(ResType_BevelDescList, theBevelResID);

	{
		StHandleLocker theLock(theBevelRes);
		LDataStream theBevelStream(*theBevelRes.mResourceH, ::GetHandleSize(theBevelRes));
			
		SSubBevel theDesc;
		::SetRect(&theDesc.cachedFrame, 0, 0, 0, 0);			
		while (!theBevelStream.AtEnd())
			{
			theBevelStream >> theDesc.paneID;
			theBevelStream >> theDesc.bevelLevel;
			mBevelList.InsertItemsAt(1, arrayIndex_Last, &theDesc);
			}
	}
				
	mGrayRegion = ::NewRgn();
	ThrowIfNULL_(mGrayRegion);

	mSubMaskRegion = ::NewRgn();
	ThrowIfNULL_(mSubMaskRegion);
	
	mNeedsRecalc = true;
	SetRefreshAllWhenResized(false);
}

// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//		¥ ~CGrayBevelView
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

CGrayBevelView::~CGrayBevelView()
{
	if (mGrayRegion != NULL)
		::DisposeRgn(mGrayRegion);

	if (mSubMaskRegion != NULL)
		::DisposeRgn(mSubMaskRegion);
}

// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//		¥ FinishCreateSelf
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

void CGrayBevelView::FinishCreateSelf()
{
	CalcGrayRegion();
	CalcSubPaneMask(mSubMaskRegion);
}

// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	¥	ResizeFrameBy
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

void CGrayBevelView::ResizeFrameBy(
	Int16		inWidthDelta,
	Int16		inHeightDelta,
	Boolean		inRefresh)
{
	if (inRefresh)
		{
		// Force an update on the region that represents the main bevel area
		InvalMainFrame();
	
		// If this view is shriking at either axis, we need to invalidate the areas that
		// the bevels once occupied so that they will be fixed during the update event.
		if ((inWidthDelta < 0) || (inHeightDelta < 0))
			InvalSubFrames();

		// We need to flag the resize because we know we're going to redraw.  If we
		// don't, the gray area will get drawing in it's pre-resize shape.
		if (inRefresh)
			mNeedsRecalc = true;

		LView::ResizeFrameBy(inWidthDelta, inHeightDelta, inRefresh);
		CalcGrayRegion();
CalcSubPaneMask(mSubMaskRegion);


		// If this view is growing on either axis, we need to invalidate the areas that
		// the bevels will occupy after the resize has been processed so that a pending
		// update event will cause them to draw.
		if ((inWidthDelta > 0) || (inHeightDelta > 0))
			InvalSubFrames();
		}
	else
		{
		LView::ResizeFrameBy(inWidthDelta, inHeightDelta, inRefresh);
		CalcGrayRegion();

CalcSubPaneMask(mSubMaskRegion);
		}
}

// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//		¥ DrawSelf
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

void CGrayBevelView::Draw(RgnHandle inSuperDrawRgnH)
{
	LView::Draw(inSuperDrawRgnH);

#if 0	
	// Don't draw if invisible or unable to put in focus
	if (IsVisible() && FocusDraw())
		{
		// Area of this View to draw is the intersection of inSuperDrawRgnH
									//   with the Revealed area of this View
		RectRgn(mUpdateRgnH, &mRevealedRect);
		if (inSuperDrawRgnH != nil)
			{
			::SectRgn(inSuperDrawRgnH, mUpdateRgnH, mUpdateRgnH);
			}
			
		if (!EmptyRgn(mUpdateRgnH))
			{
			StRegion theIntersection;
			::SectRgn(mUpdateRgnH, mGrayRegion, theIntersection);

RGBColor	green = {0, 0xffff, 0};	RGBForeColor(&green);	PaintRgn(theIntersection);

			// Only redraw the gray area if the update region intersects it.			
			if (!::EmptyRgn(theIntersection))
				{

				Rect	frame;
				CalcLocalFrameRect(frame);
				if (ExecuteAttachments(msg_DrawOrPrint, &frame))
					{
					// A View is visually behind its SubPanes so it draws itself first,
					// then its SubPanes			
					DrawSelf();
					}
				}
				
			LListIterator iterator(mSubPanes, iterate_FromStart);
			LPane	*theSub;
			while (iterator.Next(&theSub))
				theSub->Draw(mUpdateRgnH);

			}
	
		// Emptying update region frees up memory if this region wasn't rectangular
		::SetEmptyRgn(mUpdateRgnH);	
		}
#endif
}

// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//		¥ DrawSelf
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

void CGrayBevelView::DrawSelf()
{
	if (mNeedsRecalc)
		CalcGrayRegion();
		
	StColorPenState theState;
	theState.Normalize();
			
	Rect theFrame;
	CalcLocalFrameRect(theFrame);

	::PmForeColor(mBevelColors.fillColor);
	::PaintRgn(mGrayRegion);
	UGraphicGizmos::BevelRect(theFrame, mMainBevel, mBevelColors.topBevelColor, mBevelColors.bottomBevelColor);

	DrawBeveledPanes();
}

// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//		¥ DrawRecessedSubPanes
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

void CGrayBevelView::DrawBeveledPanes(void)
{
	LArrayIterator iterator(mBevelList, LArrayIterator::from_Start);
	SSubBevel theBevelDesc;
	while (iterator.Next(&theBevelDesc))
		{
		LPane *theSub = FindPaneByID(theBevelDesc.paneID);
		if ((theSub != NULL) && (theSub->IsVisible()))
			{
			Rect subFrame = theBevelDesc.cachedFrame;
			Int16 theInsetLevel = theBevelDesc.bevelLevel;
			if (theInsetLevel == 0)
				{
				ApplyForeAndBackColors();
				::EraseRect(&subFrame);
				}
			else
				{
				if (theInsetLevel < 0)
					theInsetLevel = -theInsetLevel;
					
				::InsetRect(&subFrame, -(theInsetLevel), -(theInsetLevel));
					
				UGraphicGizmos::BevelRect(subFrame, theBevelDesc.bevelLevel,
											 mBevelColors.topBevelColor, mBevelColors.bottomBevelColor);
				}
			}
		}
}

// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//		¥ CalcGrayRegion
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

void CGrayBevelView::CalcGrayRegion(void)
{
	// If we can't focus, we cant establish a coordinate scheme and make
	// subpane position calculations.  So leave the recalc flag set.
	if (!FocusExposed(false))
		{
		mNeedsRecalc = true;
		return;
		}

	StRegion theSavedGrayRgn(mGrayRegion);
		
	// Get the current frame of the gray bevel view.
	// and convert the rectangle to a region
	Rect grayFrame;
	CalcLocalFrameRect(grayFrame);
	::RectRgn(mGrayRegion, &grayFrame);

	// Rip through the sub-panes to define the region that will
	// be gray when the gray bevel view is drawn.
	mBevelList.Lock();
	StRegion theUtilRgn;
	for (ArrayIndexT theIndex = arrayIndex_First; theIndex <= mBevelList.GetCount(); theIndex++)
		{
		SSubBevel* theBevelDesc = (SSubBevel*)mBevelList.GetItemPtr(theIndex);
		LPane *theSub = FindPaneByID(theBevelDesc->paneID);
		if ((theSub != NULL) && (theSub->IsVisible()))
			{
			CalcSubPaneFrame(theSub, theBevelDesc->cachedFrame);
			::RectRgn(theUtilRgn, &theBevelDesc->cachedFrame);
			::DiffRgn(mGrayRegion, theUtilRgn, mGrayRegion);		
			}
			
		// if the pane was not found we are assuming that we don't want to
		// clear the gray area underneath that pane.
		}
		
	mBevelList.Unlock();
	mNeedsRecalc = false;

	// Calculate the difference between the old an new gray
	// areas and invalidate the newly exposed stuff.	
//	::DiffRgn(theSavedGrayRgn, mGrayRegion, theSavedGrayRgn);
//	InvalPortRgn(theSavedGrayRgn);
}

// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	¥	CalcSubPaneFrame
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

void CGrayBevelView::CalcSubPaneFrame(LPane* inSub, Rect& outFrame)
{
	inSub->CalcPortFrameRect(outFrame);
	PortToLocalPoint(topLeft(outFrame));
	PortToLocalPoint(botRight(outFrame));
}

// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	¥	InvalMainFrame
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

void CGrayBevelView::InvalMainFrame(void)
{
	Rect theFrame;
	CalcLocalFrameRect(theFrame);

	StRegion theInvalidRgn(theFrame);
	::InsetRect(&theFrame, mMainBevel, mMainBevel);
	
	StRegion theInsetRgn(theFrame);
	::DiffRgn(theInvalidRgn, theInsetRgn, theInvalidRgn);

	InvalPortRgn(theInvalidRgn);
}

// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	¥	InvalSubFrames
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

void CGrayBevelView::InvalSubFrames(void)
{
	StRegion theInvalidRgn, theFrameRgn, theInsetRgn;
	::SetEmptyRgn(theInvalidRgn);
	
	LArrayIterator iterator(mBevelList, LArrayIterator::from_Start);
	SSubBevel theBevelDesc;
	while (iterator.Next(&theBevelDesc))
		{
		Rect theSubFrame = theBevelDesc.cachedFrame;
		::RectRgn(theFrameRgn, &theSubFrame);

		Int16 theInsetLevel = theBevelDesc.bevelLevel;

		if (theInsetLevel < 0)
			theInsetLevel = -theInsetLevel;
			
		::InsetRect(&theSubFrame, -(theInsetLevel), -(theInsetLevel));
		::RectRgn(theInsetRgn, &theSubFrame);
		
		::DiffRgn(theInsetRgn, theFrameRgn, theFrameRgn);


		::UnionRgn(theInvalidRgn, theFrameRgn, theInvalidRgn);
		}

	InvalPortRgn(theInvalidRgn);
}

// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	¥	CalcSubPaneMask
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

void CGrayBevelView::CalcSubPaneMask(RgnHandle ioRgn)
{
	::SetEmptyRgn(ioRgn);
	StRegion theUtilRgn;
	
	LList& theSubs = GetSubPanes();
	LListIterator theIter(theSubs, iterate_FromStart);

	LPane* thePane = NULL;	
	while(theIter.Next(&thePane))
		{
		Rect theFrame;
		CalcSubPaneFrame(thePane, theFrame);
		::RectRgn(theUtilRgn, &theFrame);
		::UnionRgn(theUtilRgn, ioRgn, ioRgn);
		}
}

// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	¥	CalcSubPaneMask
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

void CGrayBevelView::SubPanesChanged(void)
{
	StRegion theNewMask;
	CalcSubPaneMask(theNewMask);

	::DiffRgn(mSubMaskRegion, theNewMask, theNewMask);
	if (!::EmptyRgn(theNewMask))
		InvalPortRgn(theNewMask);

	CalcSubPaneMask(mSubMaskRegion);
}
