//	===========================================================================
//	URegions.h						©1994 Metrowerks Inc. All rights reserved.
//	===========================================================================

#pragma	once

#include	<PP_Prefix.h>
#include	<LListener.h>

class	StRegion {
public:
	inline operator		RgnHandle() {return mRgn;}
	
						StRegion();
						StRegion(const Rect &inRect);
						StRegion(const RgnHandle inRegion);
						StRegion(const StRegion &inStRegion);
						~StRegion();
						
	StRegion &			operator=(const StRegion &inRhs);
	Boolean				operator==(const StRegion &inRhs) const;
		
	RgnHandle			mRgn;
};


class	StRegionBuilder {
public:
	inline operator		RgnHandle() {return mRgn;}
	
						StRegionBuilder(RgnHandle ioRgn);
						~StRegionBuilder();
	RgnHandle			mRgn;
};


class	StVisRgn {
public:
	StVisRgn(GrafPtr inPort, const Rect &inRect);
	StVisRgn(GrafPtr inPort);
	~StVisRgn();
	
	StRegion	mTempRgn;
	GrafPtr		mPort;
	RgnHandle	mSavedVisRgn;
	Point		mOldTopLeft;
};

class	LRegionPool
			: public	LList
			, public	LListener
{
public:
						LRegionPool();
						~LRegionPool();
						
	RgnHandle			Get(void);
	void				Return(RgnHandle inHandle);
	
	virtual void		ListenToMessage(MessageT inMessage, void *ioParam);
};
	
class	URegions {
public:
	static void	MakeEmpty(RgnHandle ioRegion);
	static void	MakeRegionOutline(RgnHandle inRegion, RgnHandle *ioRegion);
	static void	DrawAt(RgnHandle inRegion, Point inWhere, ConstPatternParam inPat);
	static void	FrameAt(RgnHandle inRegion, Point inWhere);
	static void	Hilite(RgnHandle inRegion);
	static void	Translate(RgnHandle ioRegion, Point inVector);
	
	static	LRegionPool	sRgnPool;
};
