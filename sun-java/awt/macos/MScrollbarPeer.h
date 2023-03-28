#pragma once

#include <LStdControl.h>
#include "MComponentPeer.h"

class MScrollbarPeer : public LStdControl {

	public:
		
		Boolean				valueChanged;
		
		MScrollbarPeer(struct SPaneInfo &newScrollbarInfo, UInt32 value, UInt32 min, UInt32 max);
		~MScrollbarPeer();	

		virtual Boolean 	FocusDraw();

		virtual Boolean 	TrackHotSpot(Int16 inHotSpot, Point inPoint);
		virtual void 		HotSpotResult(Int16 inHotSpot);

};
