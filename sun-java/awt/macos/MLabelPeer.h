#pragma once

#include <LStdControl.h>
#include "MComponentPeer.h"

class MLabelPeer : public LCaption {

	public:
		
		enum {
			class_ID 	= 'btpr'
		};
		
		MLabelPeer(struct SPaneInfo &newScrollbarInfo, Str255 labelText);
		~MLabelPeer();
		
		Boolean 			FocusDraw();
		virtual void 		DrawSelf();
		
};

