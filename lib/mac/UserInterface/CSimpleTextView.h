// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	CSimpleTextView.h					  ©1996 Netscape. All rights reserved.
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

#pragma once

#include	<VTextView.h>

// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	Wrapping Modes
//
//	Text can be wrapped in one of the following three ways:
//
//		wrapMode_WrapToFrame - Wrap to view frame.  This is similar to common
//			TE style wrapping.  
//
//		wrapMode_FixedPage - Don't wrap.  This would be like the metrowerks
//			code editor in which the page size is arbitrarily large in width
//			but no wrapping will occur when text flows outside the page bounds.
//
//		wrapMode_FixedPage - Text is wrapped to the width of the page.  This
//			would be like a word processor.
//
//		wrapMode_WrapWidestUnbreakable - The page size is determined by
//			the view frame size or the width of the widest unbreakable run.
//			This behaviour would be most like an HTML browser.  Text can
//			always be broken, but things like graphic images can not.
//			With text only, this behaves like the normal wrap to frame mode.
//
//	IMPORTANT NOTE:  if you are providing multiple views on a text engine,
//		and those views have different wrapping modes, the formatter will
//		reformat the text to the mode specified by the view EVERY TIME THE
//		VIEW IS FOCUSED.  This is probably not something you'd want to have
//		happening in your app.
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

enum {
	wrapMode_WrapToFrame = 1,
	wrapMode_FixedPage,
	wrapMode_FixedPageWrap,
	wrapMode_WrapWidestUnbreakable
};

struct SSimpleTextParms {
	Uint16				attributes;
	Uint16				wrappingMode;
	ResIDT				traitsID;
	ResIDT				textID;
	Int32				pageWidth;
	Int32				pageHeight;
	Rect				margins;
};

class CSimpleTextView : public VTextView
{
		friend class LTextEditHandler;	//	this will be going away!

			// ¥ Construction/Destruction
	private:
							CSimpleTextView();	//	Must use parameters
	public:
		enum { class_ID = 'Stxt' };
		static CSimpleTextView*	CreateSimpleTextViewStream(LStream* inStream);
							CSimpleTextView(LStream* inStream);
		virtual				~CSimpleTextView();
		
		virtual void		BuildTextObjects(LModelObject *inSuperModel);
		
			// ¥ Event Handler Overrides
		virtual void			NoteOverNewThing(LManipulator* inThing);

			// ¥ Initialization
		virtual	void 			SetInitialTraits(ResIDT inTextTraitsID);
		virtual	void			SetInitialText(ResIDT inTextID);
			
	protected:
		virtual	void		FinishCreateSelf(void);

		SSimpleTextParms*		mTextParms;
};

