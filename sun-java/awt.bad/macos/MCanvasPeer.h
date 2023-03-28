#pragma once

#include <LView.h>
#include <LAttachment.h>
#include "MComponentPeer.h"

#include "sun_awt_macos_MCanvasPeer.h"

class MCanvasPeer : public LView, public LCommander {

	public:
	
		enum {
			class_ID = 'cvpr'
		};
	
		MCanvasPeer(struct SPaneInfo &newCanvasInfo, struct SViewInfo &newCanvasViewInfo, LCommander *inSuper);
		~MCanvasPeer();
		
		virtual void		BeTarget();
		virtual void		DontBeTarget();

		virtual Boolean 	FocusDraw();
		virtual void		DrawSelf();
		
		static LPane *GetFocusedPane() { return sInFocusView; }		

};