// ΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡ
//	CGWorld.cp						  	  ©1996 Netscape. All rights reserved.
//
//	This is a replacement class for PowerPlant's LGWorld.  It allows for the
//	resizing and reorienting of the offscreen world, which LGWorld won't let
//	you do.  Because the CGWorld can be updated, the pixmap can be made
//	purgeable by passing in the pixPurge flag in the constructor.  LGWorld
//	would let you do this, however, if the pixels ever got purged you'd be
//	completely hosed.
//
//	This class has ben submitted to metrowerks for a potential replacement
//	of LGWorld.  In the event that metrowerks adds the extended functionality
//	that CGWorld offers, we can make this module obsolete.
// ΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡ

#ifdef PowerPlant_PCH
#include PowerPlant_PCH
#endif

#include "CGWorld.h"
#include "UGraphicGizmos.h"

// ΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡ
//	₯	CGWorld
// ΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡ

CGWorld::CGWorld(
	const Rect&		inBounds,
	Int16 			inPixelDepth,
	GWorldFlags 	inFlags,
	CTabHandle 		inCTableH,
	GDHandle 		inGDeviceH)
{
	::GetGWorld(&mSavePort, &mSaveDevice);

	mMacGWorld = nil;
	mBounds = inBounds;
	mDepth = inPixelDepth;

		// NewGWorld interprets the bounds in global coordinates
		// when specifying a zero pixel depth. It uses the maximum
		// depth of all screen devices intersected by the bounds.
	
	Rect	gwRect = inBounds;
	if (inPixelDepth == 0)
		{
		LocalToGlobal(&topLeft(gwRect));
		LocalToGlobal(&botRight(gwRect));
		}

	mGWorldStatus = ::NewGWorld(&mMacGWorld, inPixelDepth, &gwRect,
								inCTableH, inGDeviceH, inFlags);
						
	ThrowIfOSErr_(mGWorldStatus);
	ThrowIfNil_(mMacGWorld);
	
		// Set up coordinates and erase pixels in GWorld
	
	::SetGWorld(mMacGWorld, nil);
	::SetOrigin(mBounds.left, mBounds.top);
	::LockPixels(::GetGWorldPixMap(mMacGWorld));
	::EraseRect(&mBounds);
	::UnlockPixels(::GetGWorldPixMap(mMacGWorld));
	::SetGWorld(mSavePort, mSaveDevice);
}

// ΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡ
//	₯	~CGWorld
// ΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡ

CGWorld::~CGWorld()
{
	if (mMacGWorld != nil)
		::DisposeGWorld(mMacGWorld);
}

// ΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡ
//	₯	BeginDrawing
// ΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡ

Boolean CGWorld::BeginDrawing(void)
{
	Boolean bReadyToDraw = false;

		// The mac GWolrd will be inconsistent if a previous call to UpdateGWorld()
		// has failed.  We get one more shot a fixing that here before we draw.

	if (!IsConsistent())
		AdjustGWorld(mBounds);
	
	if (IsConsistent())
		{
		if (!::LockPixels(::GetGWorldPixMap(mMacGWorld)))
			{
			// This means we're consistentlty sized and oriented, but our pixMap
			// data has been purged.
			AdjustGWorld(mBounds);
			
			if (IsConsistent())
				bReadyToDraw = ::LockPixels(::GetGWorldPixMap(mMacGWorld));
			}
		else
			bReadyToDraw = true;
		}

	if (bReadyToDraw)
		{
		::GetGWorld(&mSavePort, &mSaveDevice);
		::SetGWorld(mMacGWorld, nil);
		}
		
	return bReadyToDraw;
}

// ΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡ
//	₯	EndDrawing
// ΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡ

void CGWorld::EndDrawing(void)
{
	Assert_(IsConsistent());			// You should have noticed that
										// BeginDrawing() returned false
										
	::UnlockPixels(::GetGWorldPixMap(mMacGWorld));
	::SetGWorld(mSavePort, mSaveDevice);
}

// ΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡ
//	₯	CopyImage
// ΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡ

void CGWorld::CopyImage(
	GrafPtr 	inDestPort,
	const Rect&	inDestRect,
	Int16 		inXferMode,
	RgnHandle 	inMaskRegion)
{
	::CopyBits(&((GrafPtr)mMacGWorld)->portBits,
				&inDestPort->portBits,
				&mBounds, &inDestRect, inXferMode, inMaskRegion);
}

// ΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡ
//	₯	SetBounds
// ΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡ

void CGWorld::SetBounds(const Rect &inBounds)
{
	if (::EqualRect(&inBounds, &mBounds))
		return;
	
	if ((RectWidth(inBounds) != RectWidth(mBounds)) || (RectHeight(inBounds) != RectHeight(mBounds)))
		UpdateTo(inBounds);
		
		// Note that even if we're not consistent, we still remember the bounds so that
		// the next time through BeginDrawing() we can attempt to realloc for the
		// size it's supposed to be.
		
	mBounds = inBounds;		
		
		// We've reallocted successfully.  Now lets get the origin set properly.
		
	if (IsConsistent())
		OrientTo(mBounds);
}

// ΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡ
//	₯	AdjustGWorld
// ΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡ

void CGWorld::AdjustGWorld(const Rect& inBounds)
{
	UpdateTo(inBounds);
	if (IsConsistent())
		OrientTo(inBounds);
}

// ΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡ
//	₯	UpdateTo
//
//	Internal method which is called when the offscreen world needs to change
//	it's dimensions.
// ΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡ

void CGWorld::UpdateTo(const Rect& inBounds)
{
	GWorldFlags theFlags = ::UpdateGWorld(&mMacGWorld, mDepth, &inBounds, NULL, NULL, 0);
													
	if (theFlags & gwFlagErr)
		mGWorldStatus = ::QDError();
}

// ΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡ
//	₯	OrientTo
//
//	Internal method in which the origin on the offscreen world needs to be
//	offset to match the on-screen representation.
// ΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡ

void CGWorld::OrientTo(const Rect& inBounds)
{
	::GetGWorld(&mSavePort, &mSaveDevice);
	::SetGWorld(mMacGWorld, nil);
	::SetOrigin(inBounds.left, inBounds.top);
	::SetGWorld(mSavePort, mSaveDevice);
}

// ΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡ
//	₯	
// ΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡ

CSharedWorld::CSharedWorld(
	const Rect& 	inFrame,
	Int16 			inDepth,
	GWorldFlags 	inFlags)
	:	CGWorld(inFrame, inDepth, inFlags)
{
}

