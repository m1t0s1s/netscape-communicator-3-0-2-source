#pragma once

#include <LStdControl.h>
#include "MComponentPeer.h"

class MCheckboxPeer : public LStdCheckBox {

#ifdef UNICODE_FONTLIST
	LStr255 mText;
#endif
	public:
	
		enum {
			class_ID = 'chpr'
		};
	
		MCheckboxPeer(struct SPaneInfo &newScrollbarInfo, SInt32 value, Str255 labelText);
		~MCheckboxPeer();

		virtual void 		HotSpotResult(Int16 inHotSpot);
		Boolean 			FocusDraw();

#ifdef UNICODE_FONTLIST
		virtual void		DrawTitle();
		virtual StringPtr	GetDescriptor(Str255 outDescriptor) const;
		virtual void		SetDescriptor(ConstStringPtr inDescriptor);
#endif

		virtual void		DrawSelf();
		virtual void		ClickSelf(const SMouseDownEvent &inMouseDown);

};

class MRadioButtonPeer : public LStdRadioButton {

#ifdef UNICODE_FONTLIST
	LStr255 mText;
#endif

	public:
	
		enum {
			class_ID = 'rbpr'
		};

		MRadioButtonPeer(struct SPaneInfo &newScrollbarInfo, SInt32 value, Str255 labelText);		
		~MRadioButtonPeer();

		virtual void 		HotSpotResult(Int16 inHotSpot);
		Boolean 			FocusDraw();

#ifdef UNICODE_FONTLIST
		virtual void		DrawTitle();
		virtual StringPtr	GetDescriptor(Str255 outDescriptor) const;
		virtual void		SetDescriptor(ConstStringPtr inDescriptor);
#endif
		virtual void		DrawSelf();
		virtual void		ClickSelf(const SMouseDownEvent &inMouseDown);

};