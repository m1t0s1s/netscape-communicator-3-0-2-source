// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
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
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

#pragma once

#include <LSharable.h>

#ifndef __QDOFFSCREEN__
#include <QDOffscreen.h>
#endif

class CGWorld
{
	public:
						CGWorld(
							const Rect&		inBounds,
							Int16 			inPixelDepth = 0,
							GWorldFlags 	inFlags = 0,
							CTabHandle 		inCTableH = nil,
							GDHandle 		inGDeviceH = nil);
							
						~CGWorld();
						
		Boolean			BeginDrawing(void);
		void			EndDrawing(void);
		
		void			CopyImage(
								GrafPtr 	inDestPort,
								const Rect&	inDestRect,
								Int16 		inXferMode = srcCopy,
								RgnHandle 	inMaskRegion = nil);

		GWorldPtr		GetMacGWorld(void);

		void			SetBounds(const Rect &inBounds);
		const Rect&		GetBounds(void) const;

		Boolean			IsConsistent(void) const;
		
		void			AdjustGWorld(const Rect& inToBounds);

	protected:
		
		void			UpdateTo(const Rect& inBounds);
		void			OrientTo(const Rect& inBounds);

		GWorldPtr		mMacGWorld;
		QDErr			mGWorldStatus;
		Rect			mBounds;
		Int16			mDepth;
		
		CGrafPtr		mSavePort;
		GDHandle		mSaveDevice;
};

inline const Rect& CGWorld::GetBounds(void) const
	{	return mBounds;									}
inline Boolean CGWorld::IsConsistent(void) const
	{	return (mGWorldStatus == noErr);				}
inline GWorldPtr CGWorld::GetMacGWorld(void)
	{	return mMacGWorld;								}
	

// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	¥	
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

class CSharedWorld : public CGWorld, public LSharable
{
	public:
						CSharedWorld(
								const Rect& 	inFrame,
								Int16 			inDepth,
								GWorldFlags 	inFlags);
};

