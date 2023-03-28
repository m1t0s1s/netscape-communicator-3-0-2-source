// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	CProgressBar.h						  ©1996 Netscape. All rights reserved.
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

#pragma once

#include <LPane.h>
#include <URange32.h>

class CProgressBar : public LPane
{
	public:
		enum { class_ID = 'PgBr' };
		static CProgressBar* CreateProgressBarStream(LStream* inStream);
								CProgressBar(LStream* inStream);
		virtual					~CProgressBar();

		virtual	void			SetValue(Int32 inValue);
		virtual	Int32			GetValue(void) const;
		virtual	void			SetValueRange(const Range32T& inRange);
		
	protected:
		virtual void			ActivateSelf(void);
		virtual void			DeactivateSelf(void);
	
		virtual	void			FinishCreateSelf(void);
		virtual	void			DrawSelf(void);

		virtual	void			DrawIndefiniteBar(Rect& inBounds);

		virtual	void			DrawPercentageBar(
									Rect& 				inBounds,
									const RGBColor& 	inBarColor,
									const RGBColor& 	inBodyColor);

		Int32 					mValue;
		Range32T				mValueRange;
		PixPatHandle			mPolePattern;
		Int32					mPatOffset;
		Int32					mPatWidth;
};
