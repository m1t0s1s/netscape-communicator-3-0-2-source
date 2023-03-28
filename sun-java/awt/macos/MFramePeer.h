#pragma once

#include <LWindow.h>
#include <LPeriodical.h>
#include "MComponentPeer.h"

enum {
	kFramePeerTopBevelColor			= 0,
	kFramePeerMiddleBevelColor		= 3,
	kFramePeerBottomBevelColor		= 6,
	kWarningCicnResID				= 169
};

class MFramePeer : public LView, public LCommander {

	public:
	
		UInt32				mMenuBarHeight;

		enum {
			class_ID		 = 'Mfpr'
		};
	
		MFramePeer(LStream *inStream);
		~MFramePeer();		

		static MFramePeer 	*CreateMFramePeerStream(LStream *inStream);
		Boolean 			FocusDraw();

		virtual void		BeTarget();
		virtual void		DontBeTarget();
		virtual void		ClickSelf(const SMouseDownEvent	&inMouseDown);

		virtual void		Show();
		virtual void		Hide();
		
		virtual void		DrawSelf();

};

class MWindow : LWindow {

	public:
	
		enum {
			class_ID		 = 'Mwin'
		};
	
		MWindow(LStream *inStream);
		~MWindow();		

		static MWindow 		*CreateMWindowStream(LStream *inStream);

		virtual void		SetAEProperty(DescType inProperty,
								const AEDesc &inValue,
								AEDesc& outAEReply);
								
		virtual Boolean		ObeyCommand(
								CommandT			inCommand,
								void				*ioParam = nil);


		virtual void		FindCommandStatus(
								CommandT			inCommand,
								Boolean				&outEnabled,
								Boolean				&outUsesMark,
								Char16				&outMark,
								Str255				outName);

};

class MUnsecureFramePane : LPane {

	public:

		Str255				warningText;

		enum {
			class_ID				= 'uJpn',
			unsecureWarningHeight	= 15,
			kTotalWarningPixpats	= 3,
			kWarningPixPatBase		= 1280,
			kMediumWarningPixpat	= 0,
			kDarkWarningPixpat		= 1,
			kLightWarningPixpat		= 2
		};
		
		static CIconHandle		gUnsecureWarningCICN;
		
		MUnsecureFramePane(LStream *inStream);		
		static MUnsecureFramePane *CreateMUnsecureFramePaneStream(LStream *inStream);
		virtual void DrawSelf();
		
};

Boolean UsingCustomAWTFrameCursor(void);
