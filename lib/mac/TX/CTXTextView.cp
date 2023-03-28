// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	CTXTextView.cp						  ©1995 Netscape. All rights reserved.
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

#include "CTXTextView.h"

#include "CTXTextEngine.h"
#include "CTXStyleSet.h"

#include <LTextModel.h>
#include <LTextSelection.h>
#include <LTextEditHandler.h>
#include <LTextElemAEOM.h>

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

CTXTextView* CTXTextView::CreateTXTextViewStream(LStream* inStream)
{
	return new CTXTextView(inStream);
}

// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	¥	
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

CTXTextView::CTXTextView()
{
	Assert_(false);	//	Must use parameters
}

// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	¥	
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

CTXTextView::CTXTextView(LStream *inStream)
	:	LSelectHandlerView(inStream)
{
	mTextModel = NULL;
	mTextEngine = NULL;
	mHasActiveTextView = false;
	mOldSelectionIsCaret = false;
	mCaretOn = false;
	mCaretTime = TickCount();
	mTextParms = NULL;
	
	mScaleNumer.h = 1;
	mScaleNumer.v = 1;
	mScaleDenom.h = 1;
	mScaleDenom.v = 1;

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

CTXTextView::~CTXTextView()
{
	if (mTextEngine && (mTextEngine->GetHandlerView() == this))
		mTextEngine->SetHandlerView(NULL);	//	The HandlerView is going away
	
	SetTextEngine(NULL);
	SetTextModel(NULL);
}

// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	¥	
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

void CTXTextView::FinishCreateSelf(void)
{
	//	¥	Make objects that haven't been made by subclasses...
	if (mTextEngine == NULL)
		ReplaceSharable_(mTextEngine, new CTXTextEngine, this);

	if (mTextModel == NULL)
		SetTextModel(new LTextModel(mTextEngine));

	if (mSelection == NULL)
		SetSelection(new LTextSelection(mTextEngine));

	if (mEventHandler == NULL)
		{
		FocusDraw();	//	set current qd port for handler...
		SetEventHandler(new LTextEditHandler);
		}

	Assert_(mTextParms != NULL);

	//	text attributes
	CTXTextEngine* theTextEngine = (CTXTextEngine*)mTextEngine;

	// Text style.  Important to do first because it sets the default style
	// set of the text engine
	SetInitialTraits(mTextParms->traitsID);

	theTextEngine->SetAttributes(mTextParms->attributes);
	theTextEngine->SetWrapMode(mTextParms->wrappingMode);

	LSelectHandlerView::FinishCreateSelf();

	//	¥	Build links
	mTextEngine->SetHandlerView(this);
	mTextEngine->AddListener(this);
	mTextModel->SetSelection(mSelection);
	
	((LTextSelection *)mSelection)->SetTextModel(mTextModel);
	((LTextSelection *)mSelection)->SetSuperModel(mTextModel);
	((LTextEditHandler *)mEventHandler)->SetCommander(this);
	((LTextEditHandler *)mEventHandler)->SetSelection(mSelection);
	((LTextEditHandler *)mEventHandler)->SetTextEngine(mTextEngine);
	((LTextEditHandler *)mEventHandler)->SetHandlerView(this);
	((LTextEditHandler *)mEventHandler)->SetTextModel(mTextModel);

	//	text image size
	if (mTextParms->wrappingMode == wrapMode_WrapToFrame)
		{
		Rect			r;
		SDimension32	size;
		
		if (!CalcLocalFrameRect(r))
			Throw_(paramErr);
	
		size.width = r.right - r.left;
		size.height = 0;

		theTextEngine->SetPageFrameSize(size);
		}
	else
		{
		SDimension32 thePageDims;
		thePageDims.width = mTextParms->pageWidth;
		thePageDims.height = mTextParms->pageHeight;
		theTextEngine->SetPageFrameSize(thePageDims);
		}

	if (mTextParms->textID != 0)
		SetInitialText(mTextParms->textID);
		
	mTextEngine->SetAttributes(mTextParms->attributes);	//	restore editable/selectable bits

	if (!(mTextParms->attributes & textAttr_Selectable))
		SetTextSelection(NULL);
		
	mTextEngine->SetTextMargins(mTextParms->margins);
	
	delete mTextParms;
	mTextParms = NULL;
}


// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
// ¥¥¥ 	Text Object Access
#pragma mark --- Text Object Access ---


// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	¥	
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

LTextEngine* CTXTextView::GetTextEngine(void)
{
	return mTextEngine;
}

// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	¥	
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

void CTXTextView::SetTextEngine(LTextEngine *inTextEngine)
{
	if (mTextEngine != inTextEngine)
		{

		if (mTextEngine)
			mTextEngine->RemoveListener(this);
	
		ReplaceSharable_(mTextEngine, inTextEngine, this);
		
		if (mTextEngine)
			mTextEngine->AddListener(this);
	
		FixImage();
		}
}

// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	¥	
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

LTextModel* CTXTextView::GetTextModel(void)
{
	return mTextModel;
}

// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	¥	
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

void CTXTextView::SetTextModel(LTextModel *inTextModel)
{
	ReplaceSharable_(mTextModel, inTextModel, this);
}

// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	¥	
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

LTextSelection*	CTXTextView::GetTextSelection(void)
{
	if (mSelection)
		return (LTextSelection *) mSelection;
	else
		return NULL;
}

// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	¥	
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

void CTXTextView::SetTextSelection(LTextSelection *inSelection)
{
	SetSelection(inSelection);
}


// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
// ¥¥¥	Event Handler Overrides
#pragma mark --- Event Handler Overrides ---


// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	¥	
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

LSelectableItem* CTXTextView::OverItem(Point inWhere)
{
	LSelectableItem	*rval = NULL;
	
	if (mTextEngine && mTextModel && mEventHandler)
		{
		FocusDraw();
		
		//	Find corresponding text offset
		TextRangeT		range;
		Boolean			leadingEdge;
		SPoint32		where;
		LocalToImagePoint(inWhere, where);
		mTextEngine->Image2Range(where, &leadingEdge, &range);
		if (leadingEdge)
			range.Front();
		else
			range.Back();
		range.Crop(mTextModel->GetRange());
		
		//	Determine part type to find
		DescType		findType = cChar;
		LTextElemAEOM	*oldElem = NULL;
		rval = inherited::OverItem(inWhere);
		if (rval)									//	inWhere is guaranteed to hit rval
			{
//			member(rval, LTextElemAEOM);
			oldElem = (LTextElemAEOM *)rval;
			findType = oldElem->GetModelKind();	//	in case count > 4
			}
			
		switch (mEventHandler->GetClickCount())
			{
			case 0:
			case 1:
				findType = cChar;
				break;
			case 2:
				findType = cWord;
				break;
			case 3:
				findType = cLine;
				break;
			case 4:
				findType = cParagraph;
				break;
			}
		
		//	Return old equivalent elem or make a new elem
		TextRangeT	foundRange = range;
		DescType	partFound = cChar;
		if (findType != cChar)
			{
			DescType type = mTextEngine->FindPart(LTextEngine::sTextAll, &foundRange, findType);
			if (type != typeNull)
				partFound = type;
			}
		if (!oldElem || (oldElem->GetRange() != foundRange))
			rval = mTextModel->MakeNewElem(foundRange, partFound);
		}
		
	return rval;
}

// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	¥	
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

LSelectableItem* CTXTextView::OverReceiver(Point inWhere)
{
	LSelectableItem	*rval = NULL;
	
	do {
		if (mEventHandler)
			rval = mEventHandler->OverReceiverSelf(inWhere);
		
		if (rval)
			break;
		
		if (!mTextEngine || !mTextModel || !mEventHandler)
			break;
			
		FocusDraw();
	
		//	Find corresponding text offset
		TextRangeT		range;
		Boolean			leadingEdge;
		SPoint32		where;
		LocalToImagePoint(inWhere, where);
		mTextEngine->Image2Range(where, &leadingEdge, &range);
		if (leadingEdge)
			range.Front();
		else
			range.Back();
		range.Crop(mTextModel->GetRange());
		rval = mTextModel->MakeNewElem(range, cChar);
	} while (false);
	
	return rval;
}

// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	¥	
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

void CTXTextView::NoteOverNewThing(LManipulator *inThing)
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
// ¥¥¥	Commader Overrides
#pragma mark --- Commander Overrides ---


// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	¥	
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

Boolean	CTXTextView::ObeyCommand(
	CommandT	inCommand,
	void*		ioParam)
{
	Boolean				cmdHandled = true;
	
	switch (inCommand)
		{
		case cmd_SelectAll:
			{
			if (mSelection)
				{
				LTextElemAEOM 	*newSelect = NULL;
				if (mTextModel)
					{
					newSelect = mTextModel->MakeNewElem(LTextEngine::sTextAll, cChar);
					}
				mSelection->SelectSimple(newSelect);
				}
			cmdHandled = true;
			break;
			}

		default:
			cmdHandled = LSelectHandlerView::ObeyCommand(inCommand, ioParam);
			break;
		}
	
	return cmdHandled;
}

// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	¥	
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

void CTXTextView::FindCommandStatus(
	CommandT	inCommand,
	Boolean		&outEnabled,
	Boolean		&outUsesMark,
	Char16		&outMark,
	Str255		outName)
{
	switch (inCommand)
		{
case 1000:
		case cmd_SelectAll:
			outUsesMark = false;
			if (mSelection)
				outEnabled = true;
			break;
			
		default:
			LSelectHandlerView::FindCommandStatus(inCommand, outEnabled, outUsesMark, outMark, outName);
			switch (inCommand)
				{
				case cmd_Clear:
				case cmd_Cut:
				case cmd_Paste:
					if (!mTextEngine || !(mTextEngine->GetAttributes() & textAttr_Editable))
						outEnabled = false;
					break;
				}
			break;
		}
}

// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	¥	
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

void CTXTextView::BeTarget(void)
{
	Assert_(!mHasActiveTextView);
	
	FocusDraw();
	LSelectHandlerView::BeTarget();
	mHasActiveTextView = true;
	CaretOn();
	
	if (mTextModel)
		LModelObject::DoAESwitchTellTarget(mTextModel);
}

// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	¥	
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

void CTXTextView::DontBeTarget(void)
{
	Assert_(mHasActiveTextView);

	FocusDraw();
	mHasActiveTextView = false;
	CaretOff();
	LSelectHandlerView::DontBeTarget();	
}


// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
// ¥¥¥	Other PowerPlant Linkage
#pragma mark --- Other PowerPlant Linkage ---


// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	¥	
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

void CTXTextView::ListenToMessage(MessageT inMessage, void* ioParam)
{
	LHandlerView	*handlerView = NULL;
	
	if ((inMessage == msg_BroadcasterDied) && (ioParam == mTextEngine))
		{
		mTextEngine = NULL;
		return;
		}

	if (mTextEngine)
		handlerView = mTextEngine->GetHandlerView();

	Try_
		{	//	If the handler view gets changed (through a FocusDraw)... boom

		switch (inMessage)
			{
	
			case msg_ModelScopeChanged:
				{
				FocusDraw();
				FixImage();
				break;
				}
	
			case msg_ModelAboutToChange:
				{
				FocusDraw();
				CaretOff();
				mOldSelectionChanged = false;
				break;
				}
				
			case msg_ModelChanged:
				{
				FocusDraw();
				if (mOldSelectionChanged)
					{
					Assert_(mTextEngine && (mTextEngine->GetNestingLevel() == 1));
					if (mOldSelectionIsSubstantive != (mSelection && mSelection->IsSubstantive()))
						SetUpdateCommandStatus(true);
			
					FocusDraw();
					MutateSelection(IsOnDuty());
					CheckSelectionScroll();
					}
				CaretOn();
				break;
				}
	
			case msg_InvalidateImage:
				{
				FocusDraw();
				Refresh();
				break;
				}
			
			case msg_UpdateImage:
				{
				if (!mTextEngine)
					return;

				ThrowIfNULL_(ioParam);
				TextUpdateRecordT *update = (TextUpdateRecordT *)ioParam;
				UpdateTextImage(update);
				break;
				}
			
			//	for multiview...
			case msg_SelectionAboutToChange:
				{
				mOldSelectionIsSubstantive = mSelection && mSelection->IsSubstantive();
				
				FocusDraw();
				if (mTextEngine && (mTextEngine->GetNestingLevel() == 0))
					{
					CaretOff();
					inherited::ListenToMessage(inMessage, ioParam);
					}
				break;
				}

			case msg_SelectionChanged:
				{
				FocusDraw();
				if (mTextEngine && (mTextEngine->GetNestingLevel() == 0))
					{
					CheckSelectionScroll();

					//	inherited::ListenToMessage(inMessage, ioParam);
					//
					//	Not!!!
					//
					//	The inherited method would always call SetUpdateCommandStatus(true)
					//	which would result in UpdateMenus being called for every typing
					//	event.  This would slow down typing drastically.  Instead,
					//	SetUpdateCommandStatus is called only when mSelection->IsSubstantive
					//	changes.
						{
						if (mOldSelectionIsSubstantive != (mSelection && mSelection->IsSubstantive()))
							SetUpdateCommandStatus(true);
				
						FocusDraw();
						MutateSelection(IsOnDuty());
						}
					CaretOn();
					}
				else
					{
					mOldSelectionChanged = true;
					}
				break;
				}

			default:
				{
				inherited::ListenToMessage(inMessage, ioParam);
				break;
				}
			}

		if (mTextEngine)
			mTextEngine->SetHandlerView(handlerView);
			
		OutOfFocus(this);

		if (handlerView)
			handlerView->FocusDraw();
		} 
	Catch_(inErr)
		{
		if (mTextEngine)
			mTextEngine->SetHandlerView(handlerView);
			
		OutOfFocus(this);

		if (handlerView)
			handlerView->FocusDraw();

		Throw_(inErr);
		}
	EndCatch_;
}

// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	¥	
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

void CTXTextView::SpendTime(const EventRecord &inMacEvent)
{
	if (!mTextEngine)
		return;

	LHandlerView* handlerView = mTextEngine->GetHandlerView();

	Try_
		{	//	If the handler view gets changed (through a FocusDraw)... boom

		LSelectHandlerView::SpendTime(inMacEvent);
		
		if (IsOnDuty())
			{
			FocusDraw();
			CaretMaintenance();
			}

		mTextEngine->SetHandlerView(handlerView);
		OutOfFocus(this);
		}
	Catch_(inErr)
		{
		mTextEngine->SetHandlerView(handlerView);
		OutOfFocus(this);
		Throw_(inErr);
		}
	EndCatch_;
}


// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
// ¥¥¥	View and Image
#pragma mark --- View and Image ---


// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	¥	
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

Boolean CTXTextView::FocusDraw(void)
{
	Boolean			rval;
	
	rval = LSelectHandlerView::FocusDraw();
	
	if (mTextEngine == NULL)
		return rval;
	
	mTextEngine->SetHandlerView(this);
	mTextEngine->SetPort(UQDGlobals::GetCurrentPort());

	return rval;
}

// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	¥	
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

void CTXTextView::DrawSelf(void)
{
	if (mTextEngine)
		{
		SPoint32		location;
		SDimension32	size;
		
		GetViewArea(&location, &size);		
		mTextEngine->DrawArea(location, size);
		}
	else
		{
		Rect	r;
	
		if (CalcLocalFrameRect(r))
			EraseRect(&r);	
		}
	
	LSelectHandlerView::DrawSelf();
}

// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	¥	
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

void CTXTextView::DrawSelfSelection(void)
{
	if (mOldSelectionIsCaret)
		{
		if (mCaretOn)
			CaretUpdate();
		}
	else
		{
		inherited::DrawSelfSelection();
		}
}

// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	¥	
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
// FIX ME!!! we nned to add support for the wrap bindings.

void CTXTextView::ResizeFrameBy(
	Int16		inWidthDelta,
	Int16		inHeightDelta,
	Boolean		inRefresh)
{
	LSelectHandlerView::ResizeFrameBy(inWidthDelta, inHeightDelta, inRefresh);

	CTXTextEngine* theEngine = (CTXTextEngine*)GetTextEngine();
	if (theEngine != NULL)
		{

		Boolean bNeedsImageFix = true;
		switch (theEngine->GetWrapMode())
			{
			case wrapMode_WrapToFrame:
				{
				
				Rect theFrame;
				CalcLocalFrameRect(theFrame);
				
				SDimension32 theNewSize;
				theNewSize.width = theFrame.right - theFrame.left;
				theNewSize.height = 0; // means auto adjust in text engine			
				
				// FIX ME!!! eventually we'll want to call some method
				// that sets the PAGE size.  Our text engine's SetImageSize()
				// method actually sets the size of the page.  It also
				// takes care of managing the appropriate changes in farmat/display.
					
//				theGeometry->SetPageFrameDimensions(size);
				mTextEngine->SetImageSize(theNewSize);
				}
				break;
	
			case wrapMode_WrapWidestUnbreakable:
				{
				// get the wiest unbreakable run
				// set the page text width to that width
				Assert_(false);		// Not Yet Implemented
				}
				break;

			case wrapMode_FixedPage:
			case wrapMode_FixedPageWrap:
				bNeedsImageFix = false;	// dont need to change the page size
				break;		
			}
		
		if (bNeedsImageFix)
			FixImage();
		}
}

// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	¥	
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
// FIX ME!!! it appears that we wont need to worry about scrolling the image
// wrt the text engine since we removed all scrolling from it.

void	CTXTextView::ScrollImageBy(
	Int32		inLeftDelta,
	Int32		inTopDelta,
	Boolean		inRefresh)
{
	LSelectHandlerView::ScrollImageBy(inLeftDelta, inTopDelta, inRefresh);

	// FIX ME!!! in  multi view scenarios the MutateSelection() call is
	// different from the LSelectHandlerView implementation.
	//
	// Here's what would change from that of LSelectHandlerView
	//	if (inRefresh && (inLeftDelta || inTopDelta)) {
	//		MutateSelection(IsOnDuty() || mHasActiveTextView);
	//	}
}

// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	¥	
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

void CTXTextView::GetPresentHiliteRgn(
	Boolean		inAsActive,
	RgnHandle	ioRegion,
	Int16		inOriginH,
	Int16		inOriginV)
{
	if (mSelection)
		{
		if (mSelection->IsSubstantive())
			{
			inherited::GetPresentHiliteRgn(inAsActive, ioRegion, inOriginH, inOriginV);
			}
		else
			{
			Point	origin;
			origin.h = inOriginH;
			origin.v = inOriginV;
			mSelection->MakeRegion(origin, &ioRegion);	//	don't make an outline
														//	because that would just be
														//	strange looking.
	
			//	Clip to visible area...
			Rect		r;
			if (CalcLocalFrameRect(r))
				{
				StRegion	tempRgn(r);
	
				SectRgn(ioRegion, tempRgn, ioRegion);
				ThrowIfOSErr_(QDError());
				}
			}
		}
	else
		{
		URegions::MakeEmpty(ioRegion);
		}
	
	mOldSelectionIsCaret = SelectionIsCaret(inAsActive);
}

// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	¥	
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

void CTXTextView::SubtractSelfErasingAreas(RgnHandle inSourceRgn)
{
	StRegion	myRgn;
	Rect		r;
	
	if (CalcPortFrameRect(r))
		{
		::RectRgn(myRgn, &r);
		::DiffRgn(inSourceRgn, myRgn, inSourceRgn);
		}

//	NO!	This pane's entire region was already subtracted above!
//
//	inherited::SubtractSelfErasingAreas(inSourceRgn);
//
//	NO!
}

// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	¥	
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
// FIX ME!!! the text engine does not care about the view rect anymore

void CTXTextView::SavePlace(
	LStream		*outPlace)
{
//	SPoint32		location = {0, 0};
//	SDimension32	size = {0, 0};
//
//	if (mTextEngine)
//		mTextEngine->GetViewRect(&location, &size);

	LView::SavePlace(outPlace);
	
//	outPlace->WriteData(&location, sizeof(location));
//	outPlace->WriteData(&size, sizeof(size));
}

// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	¥	
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

void CTXTextView::RestorePlace(
	LStream		*inPlace)
{
//	SPoint32		location;
//	SDimension32	size;

	LView::RestorePlace(inPlace);

//	inPlace->ReadData(&location, sizeof(location));
//	inPlace->ReadData(&size, sizeof(size));

//	if (mTextEngine)
//		mTextEngine->SetViewRect(location, size);
}

// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	¥	
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
// This should only be called when the page dimensions change!!!

void CTXTextView::FixImage(void)
{
	if (mTextEngine != NULL)
		{
		// FIX ME!!! eventually we'll want to call some method
		// that sets the PAGE size.  Our text engine's SetImageSize()
		// method actually sets the size of the page.  It also
		// takes care of managing the appropriate changes in farmat/display.
		SDimension32 theSize;
		mTextEngine->GetImageSize(&theSize); // actaully gets the page size

		SDimension32 theOldSize;
		GetImageSize(theOldSize);
		ResizeImageBy(theSize.width - theOldSize.width, theSize.height - theOldSize.height, false);
		}
}


// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
// ¥¥¥	Printing
#pragma mark --- Printing ---


// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	¥	
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

void CTXTextView::SetPrintRange(const TextRangeT &inRange)
{
	mPrintRange = inRange;
	if (mTextEngine)
		mPrintRange.Crop(mTextEngine->GetTotalRange());
}

// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	¥	
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

const TextRangeT& CTXTextView::GetPrintRange(void) const
{
	return mPrintRange;
}

// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	¥	
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

Boolean	CTXTextView::ScrollToPanel(const PanelSpec &inPanel)
{
	Boolean		panelInImage = false;
	
	if (mTextEngine) {
		Int32		panelTop = mTextEngine->GetRangeTop(mPrintRange),
					panelBottom,
					panelLeft;
		Int32		vertPanel;
		
		vertPanel = 0;
		
		do {
			vertPanel++;
		} while ((vertPanel < inPanel.vertIndex) && NextPanelTop(&panelTop));
		
		if (vertPanel == inPanel.vertIndex) {
			SDimension16	frameSize;
			GetFrameSize(frameSize);
			
			panelBottom = panelTop;
			NextPanelTop(&panelBottom);
			panelLeft = frameSize.width * (inPanel.horizIndex -1);
			
			OutOfFocus(this);
			FocusDraw();
			ScrollImageTo(panelLeft, panelTop, false);
			Rect	clipRect;
			if (CalcLocalFrameRect(clipRect)) {
				mRevealedRect.bottom = mRevealedRect.top + panelBottom - panelTop;
				clipRect.bottom = clipRect.top + panelBottom - panelTop;
				::ClipRect(&clipRect);
			}
			panelInImage = true;
		}
	}
	
	return panelInImage;
}

// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	¥	
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

void CTXTextView::CountPanels(
	Uint32&		outHorizPanels,
	Uint32&		outVertPanels)
{
	SDimension32	imageSize;
	SDimension16	frameSize;

	GetImageSize(imageSize);
	GetFrameSize(frameSize);
	
	outHorizPanels = 1;
	if (mTextEngine && !(mTextEngine->GetAttributes() & textAttr_WordWrap))
		{
		outHorizPanels = 1;
		}
	else if (frameSize.width > 0  &&  imageSize.width > 0)
		{
		outHorizPanels = ((imageSize.width - 1) / frameSize.width) + 1;
		}
	
	outVertPanels = 1;

	if (mTextEngine)
		{
		//	Find the real number of vertical panels
		Int32		panelTop = mTextEngine->GetRangeTop(mPrintRange);
		while (NextPanelTop(&panelTop))
			outVertPanels++;
		}
}

// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	¥	
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

Boolean	CTXTextView::NextPanelTop(Int32 *ioPanelTop) const
/*
	ioPanelTop is assumed correct on entry
*/
{
	Int32	oldTop = *ioPanelTop;
	Boolean	anotherExists = false;
	
	do {
		if (!mTextEngine)
			break;
		
		//	trivial reject if ioPanelTop is already past mPrintRange bottom
		Range32T	printV(mTextEngine->GetRangeTop(mPrintRange));
		printV.SetLength(mTextEngine->GetRangeHeight(mPrintRange));
		if (*ioPanelTop >= printV.End())
			break;
		
		//	get frame lower left image point
		SDimension16	frameSize;
		GetFrameSize(frameSize);
		SPoint32	llPt;
		llPt.h = 0;
		llPt.v = *ioPanelTop + frameSize.height;
		
		//	get vertical range of last partially(?) visible line
		TextRangeT	llRange;
		Boolean		leadingEdge;
		mTextEngine->Image2Range(llPt, &leadingEdge, &llRange);
		Range32T	llV(mTextEngine->GetRangeTop(llRange));
		llV.SetLength(mTextEngine->GetRangeHeight(llRange));

		*ioPanelTop = URange32::Min(printV.End(), llV.Start());
		
		if (*ioPanelTop <= oldTop)
			{
			//	Then a line is so tall it couldn't fit in a single panel so go aghead and split it
			*ioPanelTop = oldTop + frameSize.height;
			}
		
		if (*ioPanelTop >= printV.End())
			break;
			
		anotherExists = true;

		if (oldTop >= *ioPanelTop)
			{
			Assert_(false);
			anotherExists = false;
			}
	} while (false);
	
	return anotherExists;
}


// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
// ¥¥¥	Initialization
#pragma mark --- Initialization ---

// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	¥	
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

void CTXTextView::SetInitialTraits(ResIDT inTextTraitsID)
{
	TextTraitsRecord	traits;
	TextTraitsH			traitsH = UTextTraits::LoadTextTraits(inTextTraitsID);

	if (traitsH)
		traits = **traitsH;
	else
		UTextTraits::LoadSystemTraits(traits);

	CTXStyleSet	*defaultStyle = new CTXStyleSet;

	Assert_(mTextModel);
	defaultStyle->SetSuperModel(mTextModel);
	defaultStyle->SetTextTraits(traits);
	mTextEngine->SetDefaultStyleSet(defaultStyle);
	
	UTextTraits::SetPortTextTraits(&traits);
	FontInfo theInfo;
	::GetFontInfo(&theInfo);
	
	SPoint32 theScrollUnit;
	theScrollUnit.v = theInfo.ascent + theInfo.descent + theInfo.leading;
	theScrollUnit.h = theScrollUnit.v;
	SetScrollUnit(theScrollUnit);
}

// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	¥	
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

void CTXTextView::SetInitialText(ResIDT inTextID)
{
	StResource	textRsrc('TEXT', inTextID, false);
	if (textRsrc.mResourceH != nil)
		{
		mTextEngine->TextReplaceByHandle(LTextEngine::sTextAll, textRsrc.mResourceH);
		((LTextSelection *)mSelection)->SetSelectionRange(LTextEngine::sTextStart);
		}
}


// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
// ¥¥¥	Selection Maintenance
#pragma mark --- Selection Maintenance ---


// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	¥	
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

void CTXTextView::MutateSelection(Boolean inAsActive)
{
	Boolean		oldIsCaret = mOldSelectionIsCaret;
	StRegion	oldRegion(mOldSelectionRgn);
	
	GetPresentHiliteRgn(inAsActive, mOldSelectionRgn);
	mOldSelectionInvalid = false;
	
	MutateSelectionSelf(oldIsCaret, oldRegion, mOldSelectionIsCaret, mOldSelectionRgn);
}

// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	¥	
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

void CTXTextView::MutateSelectionSelf(
	Boolean			inOldIsCaret,
	const RgnHandle	inOldRegion,
	Boolean			inNewIsCaret,
	const RgnHandle	inNewRegion) const
{
	if (!inOldIsCaret && !inNewIsCaret)
		{
		StRegion	tempRgn(inOldRegion);
		
		XorRgn(tempRgn, inNewRegion, tempRgn);
		ThrowIfOSErr_(QDError());
		
		URegions::Hilite(tempRgn);
		}
	else if (!inOldIsCaret && inNewIsCaret)
		{
	
		URegions::Hilite(inOldRegion);
		
		}
	else if (inOldIsCaret && !inNewIsCaret)
		{
		
		URegions::Hilite(inNewRegion);

		}
	else
		{
		//	nada
		}
}

// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	¥	
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

void CTXTextView::CaretUpdate(void)
{
	if (mOldSelectionIsCaret)
		{
		if (IsActiveView())
			{
			StColorPenState	savePen;
		
			PenPat(&UQDGlobals::GetQDGlobals()->black);
			PenMode(srcXor);
			InvertRgn(mOldSelectionRgn.mRgn);
			}
		}
}

// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	¥	
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

void CTXTextView::CaretOn(void)
{
	if (mOldSelectionIsCaret)
		{
		if (!mCaretOn)
			CaretUpdate();
		
		mCaretOn = IsActiveView();
		mCaretTime = TickCount();
		}
}

// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	¥	
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

void CTXTextView::CaretOff(void)
{
	if (mOldSelectionIsCaret)
		{
		if (mCaretOn)
			CaretUpdate();
	
		mCaretOn = false;
		mCaretTime = TickCount();
		}
}

// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	¥	
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

void CTXTextView::CaretMaintenance(void)
{
	if (mOldSelectionIsCaret)
		{
		if ((TickCount() - mCaretTime) > GetCaretTime())
			{
			if (mCaretOn)
				CaretOff();
			else
				CaretOn();
			}
		}
}

// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	¥	
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
// FIX ME!!! we could handle this here instead of passing it to the text engine
// we may also nee to override LTextEngine::GetRangeScrollVector() to accout
// for scale

void CTXTextView::CheckSelectionScroll(void)
{
	if ((mOnDuty == triState_On) || (mOnDuty == triState_Latent))
		{
		if (mSelection && mTextEngine)
			{
			TextRangeT	range = ((LTextSelection*)mSelection)->GetSelectionRange();
			mTextEngine->ScrollToRange(range);
			}
		}
}

// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	¥	
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

Boolean CTXTextView::SelectionIsCaret(Boolean inIsActive) const
{
	return (mSelection
					&& 	inIsActive
					&&	!mSelection->IsSubstantive()
					&&	(mSelection->ListCount() == 1));
}


// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
// ¥¥¥	Internal View Maintenance
#pragma mark --- Internal View Maintenance ---

// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	¥	
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

void CTXTextView::UpdateTextImage(TextUpdateRecordT* inUpdate)
{
	FocusDraw();

	Range32T frameInterval;			//	in image y-pixels
	frameInterval = TextRangeT(mFrameLocation.v - mImageLocation.v);
	frameInterval.SetLength(mFrameSize.height);				

	if (!frameInterval.Intersects(inUpdate->gap))
		{
	
		//	¥	Then lets not so trivially handle this...
		if (inUpdate->gap.Start() > frameInterval.Start())
			{
			//	change is completely below this frame
			}
		else if (inUpdate->scrollAmount < 0)
			{
			Range32T	zappedBit(inUpdate->scrollStart, inUpdate->scrollStart - inUpdate->scrollAmount);
			
			if (zappedBit.Contains(frameInterval))
				{
				inherited::ScrollImageBy(0, mImageLocation.v - mFrameLocation.v + inUpdate->gap.Start(), false);
				FocusDraw();
				DrawSelf();
				}
			else if (frameInterval.Contains(Range32T(inUpdate->scrollStart - inUpdate->scrollAmount)))
				{
				inherited::ScrollImageBy(0, mImageLocation.v - mFrameLocation.v + inUpdate->gap.End(), false);
				FocusDraw();
				DrawSelf();
				}
			else
				{
				//	view is below changes.
				inherited::ScrollImageBy(0, inUpdate->scrollAmount, false);
				}
			} 
		else
			{
			//	view is below changes.
			inherited::ScrollImageBy(0, inUpdate->scrollAmount, false);
			}
		MutateSelection(IsActiveView());
		
		}
	else
		{

		//	in local coords...
		Rect		rFrame,
					rGap,
					rAbove,		//	region in frame above gap
					rScroll;	//	scrolled rect pre-scroll location

								//	rGap in 32bit! local coords
		Int32		rgTop = inUpdate->gap.Start() + mPortOrigin.v + mImageLocation.v,
					rgLeft = 0 + mFrameLocation.h + mPortOrigin.h,
					rgRight = rgLeft + mFrameSize.width, 
					rgBottom = inUpdate->gap.End() + mPortOrigin.v + mImageLocation.v;

		if (CalcLocalFrameRect(rFrame))
			{
			rGap.top = URange32::Max(rFrame.top, rgTop);
			rGap.left = URange32::Max(rFrame.left, rgLeft);
			rGap.right = URange32::Min(rFrame.right, rgRight);
			rGap.bottom = URange32::Min(rFrame.bottom, rgBottom);
			}
		else
			{
			Assert_(false);	//	huh?
			}
			
		rAbove = rFrame;
		rAbove.bottom = rGap.top;
		
		Boolean		oldSelectionIsCaret = mOldSelectionIsCaret;
		StRegion	rOld(mOldSelectionRgn.mRgn),
					rScreen,
					rTemp,
					rMask;
		StRegion	rgn;
		
		GetPresentHiliteRgn(IsActiveView(), mOldSelectionRgn);
	
		//	¥	Scroll
		if (inUpdate->scrollAmount && frameInterval.Contains(Range32T(inUpdate->scrollStart)))
			{
			Rect	r;
	
			RectRgn(rMask, &rAbove);
			SectRgn(rOld, rMask, rScreen);

			r.top = inUpdate->scrollStart + mPortOrigin.v + mImageLocation.v;
			r.left = mFrameLocation.h + mPortOrigin.h;
			r.right = r.left + mFrameSize.width;
			r.bottom = mFrameLocation.v + mFrameSize.height + mPortOrigin.v;

			if (inUpdate->scrollAmount < (r.bottom - r.top))
				{
				ScrollRect(&r, 0, inUpdate->scrollAmount, rgn);
				
				//	old selection...
				rScroll = r;
				if (inUpdate->scrollAmount > 0)
					rScroll.bottom -= inUpdate->scrollAmount;
				else
					rScroll.top -= inUpdate->scrollAmount;
				RectRgn(rMask, &rScroll);
				SectRgn(rOld, rMask, rTemp);
				OffsetRgn(rTemp, 0, inUpdate->scrollAmount);
				UnionRgn(rScreen, rTemp, rScreen);
				}
			if (inUpdate->scrollAmount < 0)
				{
				//	(offscreening this section would be a waste)
//				RGBColor	green = {0, 0xffff, 0};	RGBForeColor(&green);	PaintRgn(rgn);
				StClipRgnState	clipMe(rgn);
				DrawSelf();

				//	old selection...
				SectRgn(mOldSelectionRgn, rgn, rTemp);
				UnionRgn(rScreen, rTemp, rScreen);
				}
			}
		else
			{
			CopyRgn(rOld, rScreen);
			}
	
		//	¥	Gap
		// convert the revealed rect from port to local so that we can check
		// to see if it intersects the gap rect
		Rect rReveal;			
		GetRevealedRect(rReveal);
		PortToLocalPoint(topLeft(rReveal));
		PortToLocalPoint(botRight(rReveal));

		if (!EmptyRect(&rGap) && IsVisible() && ::SectRect(&rGap, &rReveal, &rGap))
			{
			RectRgn(rgn, &rGap);
//			RGBColor	green = {0, 0xffff, 0};	RGBForeColor(&green);	PaintRgn(rgn);
			Boolean	didit = false;

			try
				{
				StOffscreenGWorld	smoothit(rGap, 0, useTempMem);
				mTextEngine->SetPort(UQDGlobals::GetCurrentPort());
				DrawSelf();
				didit = true;
				}
			catch (ExceptionCode inErr)
				{
				// something went wrong in the allocation of the offscreen
				// drawing world, draw on screen
				}

			mTextEngine->SetPort(UQDGlobals::GetCurrentPort());

			if (!didit)
				{
				StClipRgnState		clipMe(rgn);
				DrawSelf();
				}

			//	knock out gap area and update selection
			DiffRgn(rScreen, rgn, rScreen);
			DiffRgn(mOldSelectionRgn, rgn, rTemp);
			MutateSelectionSelf(oldSelectionIsCaret, rScreen, mOldSelectionIsCaret, rTemp);
			}
		else
			{
			MutateSelectionSelf(oldSelectionIsCaret, rScreen, mOldSelectionIsCaret, mOldSelectionRgn);
			}
		}
}



// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	¥	
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

void CTXTextView::GetViewArea(SPoint32 *outOrigin, SDimension32 *outSize)
{
	Rect			r;

	if (!CalcLocalFrameRect(r))
		Throw_(paramErr);

	GetScrollPosition(*outOrigin);
	outSize->width = r.right - r.left;
	outSize->height = URange32::Min(r.bottom - r.top, mImageSize.height - outOrigin->v);
}



#pragma global_optimizer reset

