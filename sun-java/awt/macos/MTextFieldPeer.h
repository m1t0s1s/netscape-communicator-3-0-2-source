 #pragma once

#include <LTextEdit.h>
#include "MComponentPeer.h"

class MTextFieldPeer : public LEditField {

	public:
		
		enum {
			class_ID 	= 'tfpr'
		};
		
		unicode				mEchoChar;

		MTextFieldPeer(const SPaneInfo &inPaneInfo);
		~MTextFieldPeer();
		
		void 				DrawSelf();
		virtual void		ClickSelf(const SMouseDownEvent &inMouseDown);

		virtual void		FindCommandStatus(
								CommandT	inCommand,
								Boolean		&outEnabled,
								Boolean		&outUsesMark,
								Char16		&outMark,
								Str255		outName);
		virtual Boolean		HandleKeyPress(const EventRecord &inKeyEvent);

		virtual void		BeTarget();
		virtual void		DontBeTarget();

		Boolean 			FocusDraw();

};

