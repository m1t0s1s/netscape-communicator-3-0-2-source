//	===========================================================================
//	URegions.cp				©1994 Metrowerks Inc. All rights reserved.
//	===========================================================================

#ifdef PowerPlant_PCH
#include PowerPlant_PCH
#endif

#include	<URegions.h>
#include	<PP_Messages.h>
#include	<UDrawingState.h>
#include	<UDrawingUtils.h>

#ifndef		__ERRORS__
#include	<Errors.h>
#endif

//	===========================================================================
//	StRegion
//	===========================================================================
//
//	Class for a temporary region that exists only during the execution of a
//	block of C++ code.
//
//	When the StRegion variable goes out of scope (exceptions included), the
//	associated region will be disposed.

StRegion::StRegion()
{
	mRgn = URegions::sRgnPool.Get();
	ThrowIfNULL_(mRgn);
}

StRegion::StRegion(const Rect &inRect)
{
	mRgn = URegions::sRgnPool.Get();
	ThrowIfNULL_(mRgn);
	
	RectRgn(mRgn, &((Rect &)inRect));
	ThrowIfOSErr_(QDError());
}

StRegion::StRegion(const RgnHandle inRegion)
/*
	This constructor was semantically changed on 950304...
	
	This constructor now takes a copy of the passed region!  This is
	more useful than the previous policy of simply claiming the
	passed region.  Unfortunately, it is incompatible with 
	previous code!
	
	Previous code that used the previous RgnHandle constructor semantic 
	must be rewritten...
*/
{
	mRgn = URegions::sRgnPool.Get();
	ThrowIfNULL_(mRgn);
	
	if (inRegion) {
		CopyRgn(inRegion, mRgn);
		ThrowIfOSErr_(QDError());
	}
}

StRegion::StRegion(const StRegion &inStRegion)
{
	mRgn = URegions::sRgnPool.Get();
	ThrowIfNULL_(mRgn);
	
	CopyRgn(inStRegion.mRgn, mRgn);
	ThrowIfOSErr_(QDError());
}

StRegion::~StRegion()
{
	if (mRgn)
		URegions::sRgnPool.Return(mRgn);
	mRgn = NULL;
}

Boolean	StRegion::operator==(const StRegion &inRhs) const
{
	if (this == &inRhs)
		return true;
	
	return EqualRgn(mRgn, inRhs.mRgn);
}

StRegion &	StRegion::operator=(const StRegion &inRhs)
{
	if (this != &inRhs) {
		ThrowIfNULL_(mRgn);
		CopyRgn(inRhs.mRgn, mRgn);
	}
	
	return *this;
}

//	===========================================================================
//	StRegionBuilder
//	===========================================================================
//
//	Class to "open" a region for the scope of a given block.
//
//	When the StRegionBuilder variable goes out of scope (exceptions included),
//	the region will be closed.


StRegionBuilder::StRegionBuilder(RgnHandle ioRgn)
{
	mRgn = NULL;

	ThrowIfNULL_(ioRgn);
		
	if (UQDGlobals::GetQDGlobals()->thePort->rgnSave != NULL)
		Throw_(paramErr);	//	Can't nest open regions

	OpenRgn();
	ThrowIfOSErr_(QDError());

	mRgn = ioRgn;
}

StRegionBuilder::~StRegionBuilder()
{
	if (mRgn) {
		CloseRgn(mRgn);
		ThrowIfOSErr_(QDError());
	}
}

//	===========================================================================
//	StVisRgn
//	===========================================================================
//
//	Class providing a stack scoped vis region change.
//
//	When the StVisRgn variable goes out of scope (exceptions included), the
//	original port visRgn will be restored.
//
//	The (inPort) constructor will make the visRgn empty.

StVisRgn::StVisRgn(GrafPtr inPort, const Rect &inRect)
{
	ThrowIfNULL_(inPort);
	
	mPort = inPort;
	mSavedVisRgn = mPort->visRgn;
	RectRgn(mTempRgn.mRgn, &inRect);
	mPort->visRgn = mTempRgn.mRgn;

	mOldTopLeft = topLeft((**(mPort->visRgn)).rgnBBox);
}

StVisRgn::StVisRgn(GrafPtr inPort)
{
	ThrowIfNULL_(inPort);
	
	mPort = inPort;
	mSavedVisRgn = mPort->visRgn;
	mPort->visRgn = mTempRgn.mRgn;
}

StVisRgn::~StVisRgn()
{
		// Origin might have moved since constructor saved the 
		// visRgn. The distance to the current topLeft indicates
		// the amount that the origin  shifted.  We shift the
		// saved visRgn by this amount to keep it in synch with
		// the new origin position.
	
	Point	offset = topLeft((**(mPort->visRgn)).rgnBBox);
	
	offset.h -= mOldTopLeft.h;
	offset.v -= mOldTopLeft.v;
	
	URegions::Translate(mSavedVisRgn, offset);
	mPort->visRgn = mSavedVisRgn;
}

//	===========================================================================
//	LRegionPool
//	===========================================================================
//
//	Pool class to speed up StRegions.  "Free" regions are kept in an internal
//	list with region allocations & returns be sent through the free region
//	list.  Get and Return are optimized for St operation (ie. they favor
//	the end of the list).

#include	<LList.h>

LRegionPool::LRegionPool()
:	LList(sizeof(RgnHandle))
{
}

LRegionPool::~LRegionPool()
{
	RgnHandle	rgn = NULL;
	
	for (Int32 i = GetCount(); i > 0; i--) {
		FetchItemAt(i, &rgn);
		DisposeRgn(rgn);
		RemoveItemsAt(1, i);
	}
	Assert_(GetCount() == 0);
}

//	Get
//
//	Get an existing regions from the list (much faster than
//	allocating a new one) or calls NewRgn if no free region exists.

//
//	To have pooled free regions disposed of attach to an LGrowZone broadcaster
//	with:
//
//		LGrowZone::GetGrowZone()->AddListerner(URegions::sRgnPool);
//

//	For poor, non-pooled performance turn this conditional off.
#if	1
	
	RgnHandle	LRegionPool::Get(void)
	{
		RgnHandle	rval = NULL;
		Int32		count = GetCount();
		
		if (count == 0) {
			rval = NewRgn();
		} else {
			FetchItemAt(count, &rval);
			RemoveItemsAt(1, count);
		}
		
		return rval;
	}
	
	void	LRegionPool::Return(RgnHandle inRegion)
	{
		Try_ {
			InsertItemsAt(1, max_Int32, &inRegion);
		} Catch_(inErr) {
			DisposeRgn(inRegion);
			if (inErr != memFullErr)
				Throw_(inErr);
		} EndCatch_;
	}

#else

	RgnHandle	LRegionPool::Get(void)
	{
		RgnHandle	rval = NewRgn();
		
		ThrowIfNULL_(rval);
		
		return rval;
	}
	
	void	LRegionPool::Return(RgnHandle inRegion)
	{
		DisposeRgn(inRegion);
	}
	
#endif
	
void	LRegionPool::ListenToMessage(MessageT inMessage, void *ioParam)
{
	switch (inMessage) {
		case msg_GrowZone:
		{
			Int32		requested = *((Int32 *)ioParam);
			Int32		freedBytes = 0;
			RgnHandle	rgn = NULL;
			
			for (Int32 i = GetCount(); i > 0; i--) {
				if (freedBytes > requested)
					break;
				FetchItemAt(i, &rgn);
				freedBytes += GetHandleSize((Handle)rgn);
				DisposeRgn(rgn);
			}
			
			*((Int32 *)ioParam) = freedBytes;
			break;
		}
	}
}

//	===========================================================================
//	URegion
//	===========================================================================
//
//	Exception aware utility functions for Regions.

LRegionPool URegions::sRgnPool;

void	URegions::MakeEmpty(RgnHandle ioRegion)
{
	Rect	r = {0, 0, 0, 0};
	
	RectRgn(ioRegion, &r);
	ThrowIfOSErr_(QDError());
}

void	URegions::DrawAt(RgnHandle inRegion, Point inWhere, ConstPatternParam inPat)
{
	ThrowIfNULL_(inRegion);
	
	Point	negWhere;
	negWhere.h = -inWhere.h;
	negWhere.v = -inWhere.v;
	
	Translate(inRegion, inWhere);
	Try_{
		FillRgn(inRegion, inPat);
		ThrowIfOSErr_(QDError());
		Translate(inRegion, negWhere);
	} Catch_(inErr) {
		Translate(inRegion, negWhere);
	} EndCatch_;
}

void	URegions::FrameAt(RgnHandle inRegion, Point inWhere)
{
	ThrowIfNULL_(inRegion);
	
	Point	negWhere;
	negWhere.h = -inWhere.h;
	negWhere.v = -inWhere.v;
	
	Translate(inRegion, inWhere);
	Try_{
		FrameRgn(inRegion);
		ThrowIfOSErr_(QDError());
		Translate(inRegion, negWhere);
	} Catch_(inErr) {
		Translate(inRegion, negWhere);
	} EndCatch_;
}

void	URegions::Hilite(RgnHandle inRegion)
{
	StColorPenState	savePen;

	PenPat(&UQDGlobals::GetQDGlobals()->black);
	PenMode(srcXor);
	UDrawingUtils::SetHiliteModeOn();
	InvertRgn(inRegion);
}

void	URegions::Translate(RgnHandle ioRegion, Point inVector)
{
	ThrowIfNULL_(ioRegion);
	
	OffsetRgn(ioRegion, inVector.h, inVector.v);
}

void	URegions::MakeRegionOutline(RgnHandle inRegion, RgnHandle *ioRgn)
{
	OSErr		err = noErr;
	Boolean		createdRgn = false;
	
	ThrowIfNULL_(ioRgn);
	ThrowIfNULL_(inRegion);

	if (inRegion == *ioRgn) {
		StRegion	tempRgn(inRegion);
		
		InsetRgn(tempRgn, 1, 1);
								ThrowIfOSErr_(QDError());
		DiffRgn(inRegion, tempRgn, *ioRgn);	//	of course, inRegion & *ioRgn are the same!
								ThrowIfOSErr_(QDError());
	} else {
		if (*ioRgn == NULL) {
			*ioRgn = NewRgn();
			createdRgn = true;
		}
		ThrowIfNULL_(*ioRgn);

		Try_ {
			//	Get outline
			CopyRgn(inRegion, *ioRgn);
									ThrowIfOSErr_(QDError());
			InsetRgn(*ioRgn, 1, 1);
									ThrowIfOSErr_(QDError());
			DiffRgn(inRegion, *ioRgn, *ioRgn);
									ThrowIfOSErr_(QDError());
			
		} Catch_(inErr) {
		
			if (createdRgn && *ioRgn) {
				DisposeRgn(*ioRgn);
				*ioRgn = NULL;
			}
	
			Throw_(inErr);
		} EndCatch_;
	}	
}
