// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	CSimpleTextView.cp					  ©1996 Netscape. All rights reserved.
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

#include "CSimpleTextView.h"

#include <LTextModel.h>
#include <LTextSelection.h>
#include <LTextEditHandler.h>
#include <LTextElemAEOM.h>
#include <VStyleSet.h>
#include <VOTextModel.h>
#include <StClasses.h>
#include <UCursor.h>

//#include "Textension.h"

#pragma global_optimizer off

// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
// ¥¥¥ 	Construction/Destruction
#pragma mark --- Construction/Destruction ---


// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	¥	
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

CSimpleTextView* CSimpleTextView::CreateSimpleTextViewStream(LStream* inStream)
{
	return new CSimpleTextView(inStream);
}

// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	¥	
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

CSimpleTextView::CSimpleTextView()
{
	Assert_(false);	//	Must use parameters
}

// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	¥	
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

CSimpleTextView::CSimpleTextView(LStream *inStream)
	:	VTextView(inStream)
{
	mTextParms = NULL;
	
	try
		{
		// This is the first run at a new text scheme.
		// The streaming parameters will probably change
		mTextParms = new SSimpleTextParms;
		inStream->ReadData(&mTextParms->attributes, sizeof(Uint16));
		inStream->ReadData(&mTextParms->wrappingMode, sizeof(Uint16));
		inStream->ReadData(&mTextParms->traitsID, sizeof(ResIDT));
		inStream->ReadData(&mTextParms->textID, sizeof(ResIDT));
		inStream->ReadData(&mTextParms->pageWidth, sizeof(Int32));
		inStream->ReadData(&mTextParms->pageHeight, sizeof(Int32));
		inStream->ReadData(&mTextParms->margins, sizeof(Rect));
		}
	catch (ExceptionCode inErr)
		{
		// this is either a streaming error or we couldn't get the memory for
		// a text parm structure, in which case you're totally screwed.
		delete mTextParms;
		throw inErr;
		}
}

// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	¥	
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

CSimpleTextView::~CSimpleTextView()
{
}

// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	¥	
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ


// Wrap fix page size.
//  turn word wrap off
//  set image size

// Wrap to frame

void CSimpleTextView::FinishCreateSelf(void)
{
	Assert_(mTextParms != NULL);
	VTextView::FinishCreateSelf();

	switch (mTextParms->wrappingMode)
		{
		case wrapMode_WrapToFrame:
			{
			mTextParms->attributes |= textAttr_WordWrap;
			}
			break;
		
		case wrapMode_FixedPage:
			{
			Assert_(false);	// not yet implemented
			}
			break;
		
		case wrapMode_FixedPageWrap:
			{
			mTextParms->attributes &= ~textAttr_WordWrap;
			ResizeImageTo(mTextParms->pageWidth, mTextParms->pageHeight, false);
			}
			break;
		}

}

// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	¥	
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

void CSimpleTextView::BuildTextObjects(LModelObject *inSuperModel)
{
	Assert_(mTextParms != NULL);
	VTextView::BuildTextObjects(inSuperModel);

	StRecalculator	change(mTextEngine);

	mTextEngine->SetAttributes(mTextParms->attributes);

	SetInitialTraits(mTextParms->traitsID);
	if (mTextParms->textID != 0)
		SetInitialText(mTextParms->textID);
		
	mTextEngine->SetAttributes(mTextParms->attributes);	//	restore editable/selectable bits

	if (!(mTextParms->attributes & textAttr_Selectable))
		SetSelection(NULL);
		
	mTextEngine->SetTextMargins(mTextParms->margins);
	
	VOTextModel* theModel = dynamic_cast<VOTextModel*>(GetTextModel());
	theModel->SetStyleListBehavior(setBehavior_Ignore);
	
	delete mTextParms;
	mTextParms = NULL;
}

// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	¥	
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

void CSimpleTextView::NoteOverNewThing(LManipulator *inThing)
{
	if (inThing)
		{
		switch(inThing->ItemType())
			{
			case kManipulator:
				UCursor::Reset();	//	?
				break;

			case kSelection:
				if (mEventHandler && ((LDataDragEventHandler *)mEventHandler)->GetStartsDataDrags())
					{
					CursHandle theCursH = ::GetCursor(131);
					if (theCursH != nil)
						::SetCursor(*theCursH);
					break;
					}
				else
					{
					//	fall through
					}

			case kSelectableItem:
				UCursor::Tick(cu_IBeam);	//	?
				break;

			}
		}
	else
		{
		UCursor::Reset();
		}
}

// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
// ¥¥¥	Initialization
#pragma mark --- Initialization ---

// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	¥	
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

void CSimpleTextView::SetInitialTraits(ResIDT inTextTraitsID)
{
	TextTraitsRecord	traits;
	TextTraitsH			traitsH = UTextTraits::LoadTextTraits(inTextTraitsID);

	if (traitsH)
		traits = **traitsH;
	else
		UTextTraits::LoadSystemTraits(traits);

	LStyleSet* defaultStyle = mTextEngine->GetDefaultStyleSet();
	defaultStyle->SetTextTraits(traits);
		
	SPoint32 theScrollUnit;
	theScrollUnit.v = defaultStyle->GetHeight();
	theScrollUnit.h = theScrollUnit.v;
	SetScrollUnit(theScrollUnit);
}

// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	¥	
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

void CSimpleTextView::SetInitialText(ResIDT inTextID)
{
	StResource	textRsrc('TEXT', inTextID, false);
	if (textRsrc.mResourceH != nil)
		{
		mTextEngine->TextReplaceByHandle(LTextEngine::sTextAll, textRsrc.mResourceH);
		((LTextSelection *)mSelection)->SetSelectionRange(LTextEngine::sTextStart);
		}
}




