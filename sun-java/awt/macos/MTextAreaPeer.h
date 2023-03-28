 #pragma once

#include <LTextEdit.h>
#include "MComponentPeer.h"
#include "sun_awt_macos_MTextAreaPeer.h"

class MTextAreaPeer : public LView {

	public:
		
		enum {
			class_ID 	= 'tapr'
		};
		
		MTextAreaPeer(LStream *inStream);
		~MTextAreaPeer();	

		static LView* CreateMTextAreaPeerStream(LStream *inStream);

		Boolean 			FocusDraw();
		void				DrawSelf();

};

class MTextEditPeer : public LTextEdit {

	public:
		
		enum {
			class_ID 	= 'tepr'
		};

		MTextEditPeer(LStream *inStream);
		~MTextEditPeer();

		static LView* CreateMTextEditPeerStream(LStream *inStream);

		Boolean 			FocusDraw();
		void				DrawSelf();

		virtual void		BeTarget();
		virtual void		DontBeTarget();
		void				SetEditable(Boolean editable);

};