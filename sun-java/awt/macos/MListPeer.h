 #pragma once

#include <LStdControl.h>
#include "MComponentPeer.h"

class MListPeer : public LPane {

	public:
		
		LList				*mItemList;
		LList				*mSelectionList;
		Boolean				mCanMultiplySelect;
		UInt32				mScrollPosition;
		ControlHandle		mScrollControl;

		enum {
			class_ID 	= 'lipr'
		};
		
		MListPeer(struct SPaneInfo &newScrollbarInfo);
		~MListPeer();

		Boolean 			FocusDraw();
		
		void				HideSelf();
		void				ShowSelf();

		void				EnableSelf();
		void				DisableSelf();
		
		void				DrawSelf();
		void				ClickSelf(const SMouseDownEvent &inMouseDown);

		virtual void		PlaceInSuperImageAt(Int32 inHorizOffset,
								Int32 inVertOffset, Boolean inRefresh);
		virtual void		ResizeFrameBy(Int16 inWidthDelta, Int16 inHeightDelta,
									Boolean inRefresh);

		void				InvalidateContent();
		
		void				SelectItem(UInt32);
		void				DeselectItem(UInt32);
		void				DeselectAll(UInt32 exceptItem);
		
		Boolean				GetItemCell(UInt32 whichItem, Rect &itemRect);
		UInt32				GetItemsOnScreen();
		
		Boolean				IsItemSelected(UInt32);
		
		void				DoRelativeScroll(SInt16 fromLocation, SInt16 toLocation);
		
		void				AdjustScrollBar();

};

