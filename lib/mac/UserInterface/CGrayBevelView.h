// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	CGrayBevelView.cp					  ©1996 Netscape. All rights reserved.
//
//		A beveled gray view that can draw raised, recessed, or no bevel
//		for any of its subviews.
//
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

#pragma once

#include <LView.h>
#include <LArray.h>
#include "UStdBevels.h"


const ResType ResType_BevelDescList = 'BevL';

typedef struct SSubBevel {
	PaneIDT		paneID;
	Int16		bevelLevel;
	Rect		cachedFrame;
} SSubBevel;


class CGrayBevelView : public LView
{
	public:
		enum { class_ID = 'GrBv' };
		static	CGrayBevelView *CreateGrayBevelViewStream(LStream *inStream);
		
							CGrayBevelView(LStream *inStream);
		virtual				~CGrayBevelView();
	
		virtual void		ResizeFrameBy(
									Int16 		inWidthDelta,
									Int16		inHeightDelta,
									Boolean 	inRefresh);

		virtual void		Draw(RgnHandle inSuperDrawRgnH);

		virtual	void		CalcGrayRegion(void);
		virtual	void		SubPanesChanged(void);
		
		const SBevelColorDesc& GetBevelColors(void) const;

	protected:
		virtual void		FinishCreateSelf(void);
		virtual	void		DrawSelf(void);
		virtual	void 		DrawBeveledPanes(void);
		
		virtual void		CalcSubPaneFrame(LPane* inSub, Rect& outFrame);

		virtual	void		InvalMainFrame(void);
		virtual void		InvalSubFrames(void);
		
		virtual	void		CalcSubPaneMask(RgnHandle ioRgn);
		
		Int16				mMainBevel;
		SBevelColorDesc		mBevelColors;
		
		LArray				mBevelList;
		RgnHandle			mGrayRegion;
		RgnHandle			mSubMaskRegion;
		Boolean				mNeedsRecalc;
};

inline const SBevelColorDesc& CGrayBevelView::GetBevelColors(void) const
	{	return mBevelColors;		}
	
