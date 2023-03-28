// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	CSimpleDividedView.h				  ©1996 Netscape. All rights reserved.
//
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
#pragma once


enum
{
	kHorizontalResizeCursor	=	1120,
	kVerticalResizeCursor	=	1136
};


/*****************************************************************************
	class CSimpleDividedView
	Simplest divided view.
 
 ### Standard usage:
  In Constructor, create the CSimpleDividedView from a template,
  set the IDs and min size of subviews
  
  It should be in the same level of hierarchy as the views it manages.
  It does not do any drawing
  
  It broadcasts msg_SubviewChangedSize after a successful drag. Hook it up
  to your window manually (outside the RidL resource), because it is not a
  LControl subclass. Do this only if you need to do something in response to
  resize. For example, recalc the GrayBevelView.
  
  timm has a more sophisticated class, divview.cp, which takes car of resizing
 *****************************************************************************/

#include <LPane.h>
#include <LBroadcaster.h>

const MessageT msg_SubviewChangedSize = 32400;	// ioParam is CSimpleDividedView
 
class CSimpleDividedView : public LPane,
							public LBroadcaster
{
	public:
	
	// constructors

		enum { class_ID = 'SDIV' };
		static	CSimpleDividedView *CreateSimpleDividedView(LStream *inStream);
		
							CSimpleDividedView(LStream *inStream);

		virtual				~CSimpleDividedView();
	
		virtual void		FinishCreateSelf();
		
	// access
		
				void		SetMinSize(Int16 topLeft, Int16 botRight)
				{
					fTopLeftMinSize = topLeft;
					fBottomRightMinSize = botRight;
					RepositionView();
				}

	// event handling

		virtual void		AdaptToSuperFrameSize(Int32 inSurrWidthDelta,
										Int32 inSurrHeightDelta,
										Boolean inRefresh);
		virtual void		ClickSelf(const SMouseDownEvent	&inMouseDown);

		virtual void		AdjustCursorSelf(Point inPortPt, const EventRecord&	inMacEvent );


#ifdef DEBUG
		virtual void		DrawSelf();
#endif
	private:
	
		void				ReadjustConstraints();
		void				RepositionView();
		Boolean				GetViewRects(Rect& r1, Rect& r2);

		PaneIDT				fTopLeftViewID;
		PaneIDT				fBottomRightViewID;
		Int16				fTopLeftMinSize;
		Int16				fBottomRightMinSize;
		Int16				fIsVertical;	// Are views stacked vertically?. -1 means uninitialized, true and false
};
