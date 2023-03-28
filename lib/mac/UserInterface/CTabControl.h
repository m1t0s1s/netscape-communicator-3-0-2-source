// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	CTabControl.h						  ©1996 Netscape. All rights reserved.
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

#pragma once

#include <LArray.h>
#include <LControl.h>
#include <LString.h>

#include "UStdBevels.h"


#ifdef powerc
#pragma options align=mac68k
#endif

typedef struct STabDescriptor {
	MessageT		valueMessage;
	Int16			width;			// -1 means auto adjust
	Str63			title;
} STabDescriptor;

#ifdef powerc
#pragma options align=reset
#endif

const ResType_TabDescList 		=	'TabL';
const MessageT msg_TabSwitched 	=	'TbSw';

class CTabInstance;

class CTabControl : public LControl
{
	public:
		enum { class_ID = 'TabC' };
		enum { eNorthTab = 0, eEastTab, eSouthTab, eWestTab };

		static CTabControl* CreateTabControlStream(LStream* inStream);
								CTabControl(LStream* inStream);
		virtual					~CTabControl();

			// ¥ Access

		virtual void			SetValue(Int32 inValue);
		virtual MessageT		GetMessageForValue(Int32 inValue);
		virtual Point			GetMinumumSize() { return mMinimumSize; }	// -1 = uninitialized

			// ¥ Drawing
		virtual void			Draw(RgnHandle inSuperDrawRgnH);
		virtual void			ResizeFrameBy(
										Int16 			inWidthDelta,
										Int16			inHeightDelta,
										Boolean			inRefresh);
										
		virtual void			DoLoadTabs(ResIDT inTabDescResID);
		
		
	protected:

			// ¥ Initialization & Configuration
		virtual void			FinishCreateSelf(void);
		virtual	void			Recalc(void);
		virtual	void			CalcTabMask(
										const Rect& 	inControlFrame,
										const Rect&		inTabFrame,
										RgnHandle		ioTabMask);
			// ¥ Drawing
		virtual void			DrawSelf(void);
		virtual	void			DrawOneTab(CTabInstance* inTab);
		virtual	void			DrawBevel(CTabInstance* inTab);
		virtual	void			DrawTitle(CTabInstance* inTab);
		virtual	void			DrawSides(void);
		
		virtual	void			ActivateSelf(void);
		virtual	void			DeactivateSelf(void);

			// ¥ Control Behaviour
		virtual Int16			FindHotSpot(Point inPoint);
		virtual Boolean			PointInHotSpot(Point inPoint, Int16 inHotSpot);

		virtual void			HotSpotAction(
										Int16			inHotSpot,
										Boolean 		inCurrInside,
										Boolean			inPrevInside);
									
		virtual void			HotSpotResult(Int16 inHotSpot);	
		virtual void			BroadcastValueMessage();

		Uint8					mOrientation;
		Int16					mCornerPixels;
		Int16					mBevelDepth;
		Int16					mSpacing;
		Int16					mLeadPixels;
		Int16					mRisePixels;
		ResIDT					mTabDescID;
		ResIDT					mTitleTraitsID;
		Point					mMinimumSize;	// The minimum size at which all tabs will be displayed

		SBevelColorDesc			mActiveColors;
		SBevelColorDesc			mOtherColors;
		SBooleanRect			mBevelSides;
		Rect					mSideClipFrame;
		
		LArray					mTabs;
		CTabInstance*			mCurrentTab;
		RgnHandle				mControlMask;	
		Boolean					mTrackInside;
};

// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	¥	
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

class CTabInstance
{
	friend class CTabControl;
	
	protected:
								CTabInstance();
								CTabInstance(const STabDescriptor& inDesc);
		virtual					~CTabInstance();

	private:
		MessageT				mMessage;
		RgnHandle 				mMask;
		Rect					mFrame;
		Rect					mShadeFrame;
		Int16					mWidth;
		TString<Str63> 			mTitle;	
};

