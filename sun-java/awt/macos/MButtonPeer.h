#pragma once

#include <LStdControl.h>
#include "MComponentPeer.h"

class MButtonPeer : public LStdButton {

	public:
		
		enum {
			class_ID 	= 'btpr'
		};
		
		MButtonPeer(struct SPaneInfo &newScrollbarInfo, Str255 labelText);
		~MButtonPeer();
			
		virtual void 		HotSpotResult(Int16 inHotSpot);
		Boolean 			FocusDraw();
#ifdef UNICODE_FONTLIST
	protected:
		virtual void		DrawSelf();
		virtual void		DrawTitle();
		virtual void		HotSpotAction(Int16 inHotSpot, Boolean inCurrInside, Boolean inPrevInside);
		virtual Boolean		TrackHotSpot(Int16 inHotSpot, Point inPoint);
#endif
};

enum {
	kButtonPeerIsStandard 	= 1,
	kButtonPeerIsRadio		= 2,
	kButtonPeerIsCheckbox 	= 3
};
