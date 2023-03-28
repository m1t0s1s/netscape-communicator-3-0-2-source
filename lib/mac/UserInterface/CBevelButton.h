// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	CBevelButton.h						  ©1996 Netscape. All rights reserved.					
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

#pragma once

#include <LAttachment.h>
#include <LControl.h>
#include <LString.h>

#include "UStdBevels.h"
#include "UGraphicGizmos.h"

#ifndef __MENUS__
#include <menus.h>
#endif

// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	¥¥¥
//	¥	Class CBevelButton
//	¥¥¥
//
//	The bevel button class is the base in which other types of beveled graphic
//	buttons are based.  Drawing is done offscreen in temp mem if a world can be
//	allocated.  A mask region is maintained so that the buttons background view
//	will show through if the corners are rounded.
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

class LStream;

class CBevelButton : public LControl
{
	public:
		enum	{ class_ID = 'BvBt' };	
		static 	CBevelButton* CreateBevelButtonStream(LStream *inStream);
		
							CBevelButton(LStream *inStream);
		virtual				~CBevelButton();
				
		Boolean				IsPushed(void) const;

		virtual StringPtr	GetDescriptor(Str255 outDescriptor) const;
		virtual void		SetDescriptor(ConstStr255Param inDescriptor);
		
		virtual void		SetGraphicID(ResIDT inResID);
		
	protected:
		virtual void		PrepareDrawButton(void);
		virtual void		FinalizeDrawButton(void);

		virtual	void 		Draw(RgnHandle inSuperDrawRgnH);
		virtual	void		DrawSelf(void);
		virtual	void		DrawSelfDisabled(void);		
		virtual void		DrawButtonContent(void);
		virtual	void		DrawButtonTitle(void);
		virtual	void		DrawButtonGraphic(void);
				
		virtual void		ActivateSelf(void);
		virtual void		DeactivateSelf(void);
		
		virtual void		EnableSelf(void);
		virtual void		DisableSelf(void);
		
		virtual void		HotSpotAction(
									Int16			inHotSpot,
									Boolean 		inCurrInside,
									Boolean			inPrevInside);
									
		virtual void		HotSpotResult(Int16 inHotSpot);

		Boolean				mPushed;
		Rect 				mCachedButtonFrame;
		RgnHandle			mButtonMask;

		Int16				mBevel;
		Int16				mOvalWidth;
		Int16				mOvalHeight;
		SBevelColorDesc		mUpColors;
		SBevelColorDesc		mDownColors;
	
		TString<Str255>		mTitle;
		ResIDT				mTitleTraitsID;	
		Point				mTitlePosition;
		
		ResType				mGraphicType;
		ResIDT				mGraphicID;
		Point				mGraphicPosition;
		CIconHandle			mGraphicHandle;
		
	private:
		void 				InitBevelButton(void);
};

inline Boolean CBevelButton::IsPushed(void) const
	{	return mPushed;		}
	

// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	¥¥¥
//	¥	Class CBevelButton
//	¥¥¥
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

class CDeluxeBevelButton : public CBevelButton
{
	public:
		enum	{ class_ID = 'DxBt' };	
		static 	CDeluxeBevelButton* CreateDeluxeBevelButtonStream(LStream *inStream);
		
							CDeluxeBevelButton(LStream *inStream);
	protected:

		virtual void		DrawButtonContent(void);
};

