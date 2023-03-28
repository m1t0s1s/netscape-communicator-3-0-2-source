//	===========================================================================
//	VSimpleTextView.cp
//	===========================================================================

#ifdef PowerPlant_PCH
#include PowerPlant_PCH
#endif

#include	"VSimpleTextView.h"
#include	<VTextEngine.h>
#include	<VOTextModel.h>
#include	<LTextSelection.h>
#include	<VTextHandler.h>
#include	<VStyleSet.h>
#include	<LRulerSet.h>
#include	<UMemoryMgr.h>
#include	<LStream.h>
#include	<PP_Messages.h>

#pragma	warn_unusedarg off

VSimpleTextView::VSimpleTextView()
{
	pMarginsRect.top = 0;
	pMarginsRect.left = 0;
	pMarginsRect.right = 0;
	pMarginsRect.bottom = 0;
	pTraitsID = 0;
	pStyleSetID = 0;
	pRulerSetID = 0;
	pInitialTextID = 0;
	pImageWidth = 0;
	pAttributes = 0;
	mTabSelectsAll = false;
}

VSimpleTextView::VSimpleTextView(LStream *inStream)
:	VTextView(inStream)
{
	pMarginsRect.top = 0;
	pMarginsRect.left = 0;
	pMarginsRect.right = 0;
	pMarginsRect.bottom = 0;
	pTraitsID = 0;
	pStyleSetID = 0;
	pRulerSetID = 0;
	pInitialTextID = 0;
	pImageWidth = 0;
	pAttributes = 0;
	mTabSelectsAll = false;

	*inStream >> pMarginsRect;
	*inStream >> pTraitsID;
	*inStream >> pStyleSetID;
	*inStream >> pRulerSetID;
	*inStream >> pInitialTextID;
	*inStream >> pImageWidth;

	Boolean	hasIt;
	*inStream >> hasIt;
	if (hasIt)
		pAttributes |= textEngineAttr_MultiStyle;
	*inStream >> hasIt;
	if (hasIt)
		pAttributes |= textEngineAttr_MultiRuler;
	*inStream >> hasIt;
	if (hasIt)
		pAttributes |= textEngineAttr_Editable;
	*inStream >> hasIt;
	if (hasIt)
		pAttributes |= textEngineAttr_Selectable;
	
	*inStream >> mTabSelectsAll;
}

VSimpleTextView::~VSimpleTextView()
{
}

void	VSimpleTextView::FinishCreateSelf(void)
{
	inherited::FinishCreateSelf();
}

void	VSimpleTextView::MakeTextObjects(void)
{
	//	handler
	if (!mEventHandler)
		SetEventHandler(new VTextHandler);

	//	selection
	if (!mTextSelection && (pAttributes & textEngineAttr_Selectable))
		SetSelection(new LTextSelection);

	//	model
	if (!mTextModel) {
		VOTextModel	*model = new VOTextModel();
		SetTextModel(model);
		model->SetStyleListBehavior(
				(pAttributes & textEngineAttr_MultiStyle)
					?	setBehavior_FreeForm
					:	setBehavior_Ignore);
		model->SetRulerListBehavior(
				(pAttributes & textEngineAttr_MultiRuler)
					?	setBehavior_Sheet
					:	setBehavior_Ignore);
	}

	//	engine
	if (!mTextEngine) {
		SetTextEngine(new VTextEngine());
		
		//	default style set
		LStyleSet	*style = new VStyleSet();
		if (pStyleSetID) {
		} else if (pTraitsID) {
			TextTraitsH		traits = UTextTraits::LoadTextTraits(pTraitsID);
			StHandleLocker	lock((Handle)traits);

			style->SetTextTraits(**traits);
		}
		mTextEngine->SetDefaultStyleSet(style);
		
		//	default ruler set
		LRulerSet	*ruler = new LRulerSet();
		if (pRulerSetID) {
		}
		mTextEngine->SetDefaultRulerSet(ruler);
		
		//	text
		if (pInitialTextID) {
			StResource		r(typeChar, pInitialTextID);
			StHandleLocker	lock(r);
			Int32			size = ::GetHandleSize(r);
			
			mTextEngine->TextReplaceByPtr(LTextEngine::sTextAll, *((Handle)r), size);
		}
		
		//	margins
		mTextEngine->SetTextMargins(pMarginsRect);

		//	width		
		if (pImageWidth) {
			mTextEngine->SetImageWidth(pImageWidth);
		}

		//	attributes
		mTextEngine->SetAttributes(pAttributes);
	}
}

Boolean	VSimpleTextView::ObeyCommand(
	CommandT	inCommand,
	void*		ioParam)
{
	if ((inCommand == msg_TabSelect) && mTabSelectsAll)
		inCommand = cmd_SelectAll;
		
	return inherited::ObeyCommand(inCommand, ioParam);
}
