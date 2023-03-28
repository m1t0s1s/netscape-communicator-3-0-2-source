//	===========================================================================
//	VTextView.cp
//	===========================================================================

#ifdef PowerPlant_PCH
#include PowerPlant_PCH
#endif

#include	"VTextView.h"
#include	<PP_Messages.h>
#include	<PP_Resources.h>
#include	<LTextSelection.h>
#include	<LTextElemAEOM.h>
#include	<LStyleSet.h>
#include	<UAEGizmos.h>
#include	<UFontsStyles.h>
#include	<UAppleEventsMgr.h>
#include	<AERegistry.h>
#include	<AEVTextSuite.h>
#include	<LAEAction.h>
#include	<UModalDialogs.h>
#include	<VTextHandler.h>
#include	<VText_Commands.h>
#include	<VTSMProxy.h>
#include	<LStream.h>

#pragma	warn_unusedarg off

VTextView::VTextView()
{
	mTSMProxy = NULL;
}

VTextView::VTextView(LStream *inStream)
:	LTextView(inStream)
{
	mTSMProxy = NULL;
}

VTextView::~VTextView()
{
	SetTSMProxy(NULL);
}

void	VTextView::FinishCreateSelf(void)
{
	inherited::FinishCreateSelf();
	
	SetTSMProxy(WTSMManager::MakeInlineTSMProxy(*this));
}

#include	<VTextEngine.h>
#include	<VOTextModel.h>
#include	<VStyleSet.h>
#include	<LRulerSet.h>
#include	<UMemoryMgr.h>

void	VTextView::BuildTextObjects(LModelObject *inSuperModel)
{
	ThrowIfNULL_(inSuperModel);
	
	MakeTextObjects();

	LinkTextObjects(inSuperModel);
	
	SetTSMProxy(WTSMManager::MakeInlineTSMProxy(*this));
}

void	VTextView::MakeTextObjects(void)
{
	if (!mEventHandler)
		SetEventHandler(new VTextHandler);

	if (!mTextSelection)
		SetSelection(new LTextSelection);

	if (!mTextModel)
		SetTextModel(new VOTextModel);

	if (!mTextEngine) {
		SetTextEngine(new VTextEngine());
		
		mTextEngine->SetAttributes(textEngineAttr_MultiStyle | textEngineAttr_Editable | textEngineAttr_Selectable);
		Rect	r = {10, 10, 10, 10};
		mTextEngine->SetTextMargins(r);
		
		mTextEngine->SetDefaultStyleSet(new VStyleSet());
		mTextEngine->SetDefaultRulerSet(new LRulerSet());
	}
}

void	VTextView::LinkTextObjects(LModelObject *inSuperModel)
{
	LTextEditHandler	*handler = dynamic_cast<LTextEditHandler *>(mEventHandler);
	Assert_(handler == mEventHandler);
	if (handler) {
		handler->SetCommander(this);
		handler->SetSelection(mSelection);
		handler->SetTextEngine(mTextEngine);
		handler->SetTextModel(mTextModel);
		handler->SetHandlerView(this);
	}

	LTextSelection	*selection = dynamic_cast<LTextSelection *>(mSelection);
	Assert_(selection == mSelection);
	if (selection) {
		selection->SetSuperModel(mTextModel);
		selection->SetTextEngine(mTextEngine);
		selection->SetTextModel(mTextModel);
	}

	VOTextModel	*model = dynamic_cast<VOTextModel *>(mTextModel);
	Assert_(model == mTextModel);
	if (model)	{
		model->SetSuperModel(inSuperModel);
		model->SetSelection(mSelection);
		model->SetTextEngine(mTextEngine);
		
		ThrowIfNULL_(mTextEngine);
		if (mTextEngine) {
			LStyleSet	*style = mTextEngine->GetDefaultStyleSet();
			ThrowIfNULL_(style);
			style->SetSuperModel(model);
			model->AddStyle(style);

			LRulerSet	*ruler = mTextEngine->GetDefaultRulerSet();
			ThrowIfNULL_(ruler);
			ruler->SetSuperModel(model);
			model->AddRuler(ruler);
		}
	}
	
	Assert_(mTextEngine);
	if (mTextEngine) {
		mTextEngine->SetHandlerView(this);
		mTextEngine->AddListener(this);
	}

	//?	required to get image set to frame as needed on new window
	FixImage();
}

void	VTextView::ScrollImageBy(
	Int32		inLeftDelta,
	Int32		inTopDelta,
	Boolean		inRefresh)
{
	LSelectHandlerView::ScrollImageBy(inLeftDelta, inTopDelta, inRefresh);
}

void	VTextView::SetTextEngine(LTextEngine *inTextEngine)
{
	inherited::SetTextEngine(inTextEngine);
	
	SetTSMProxy(WTSMManager::MakeInlineTSMProxy(*this));
}

void	VTextView::SetTSMProxy(VInlineTSMProxy *inProxy)
{
	delete mTSMProxy;
	mTSMProxy = NULL;
	mTSMProxy = inProxy;
	
	VTextHandler	*textHandler = dynamic_cast<VTextHandler *>(mEventHandler);
	if (textHandler)
		textHandler->SetTSMProxy(mTSMProxy);
}

VInlineTSMProxy *	VTextView::GetTSMProxy(void)
{
	return mTSMProxy;
}

void	VTextView::DrawSelfSelection(void)
{
	if (mTSMProxy)
		mTSMProxy->DrawHilite();
	
	inherited::DrawSelfSelection();
}


void VTextView::BeTarget(void)
{
	inherited::BeTarget();
	
	if (mTSMProxy != NULL)
		mTSMProxy->Activate();
}

void VTextView::DontBeTarget(void)
{
	if (mTSMProxy != NULL)
		mTSMProxy->Deactivate();

	inherited::DontBeTarget();
}


/*-
Boolean	VTextView::ObeyCommand(
	CommandT	inCommand,
	void*		ioParam)
{
	Boolean		handled = false;
	ResIDT		menuNo;
	Int16		itemNo;
	
	if (IsSyntheticCommand(inCommand, menuNo, itemNo)) {
		switch (menuNo) {
			case MENU_Font:
			case MENU_Size:
			case MENU_Style:
			{
				StAEDescriptor	doerEvent;
				LAEStream		stream(kAEVTextSuite, kAEModify);
				stream.WriteKey(keyDirectObject);
				stream.WriteSpecifier(mSelection);
				switch (menuNo) {

					case MENU_Font:
					{
						Str255	fontName;
						Int16	fontNo = UFontMenu::GetFontNumber(itemNo);
						GetFontName(fontNo, fontName);
						if (fontName[0] == 0)
							Throw_(paramErr);
						stream.WriteKey(keyAEModifyProperty);
						stream.WriteEnumDesc(pFont);
						stream.WriteKey(keyAEModifyToValue);
						stream.WriteDesc(typeChar, fontName + 1, fontName[0]);
						break;
					}

					case MENU_Size:
					{
						Int16	size = USizeMenu::GetFontSize(itemNo);
						stream.WriteKey(keyAEModifyProperty);
						stream.WriteEnumDesc(pPointSize);
						stream.WriteKey(keyAEModifyToValue);
						stream.WriteDesc(typeShortInteger, &size, sizeof(size));
						break;
					}
					
					case MENU_Style:
					{
						Assert_(false);	//	wont happen
						break;
					}
				}
				stream.Close(&doerEvent.mDesc);
				LAEAction	*action = new LAEAction();
				action->SetRedoAE(doerEvent);
				action->SetSelection(mSelection, typeNull, typeNull);
				mSelection->UndoAddOldData(action);
				action->SetRecalcAccumulator(mTextEngine);
				action->SetRecordOnlyFinalState(true);
				PostAction(action);
				handled = true;
				break;
			}
			
			default:
				handled = inherited::ObeyCommand(inCommand, ioParam);
				break;
		}
	} else {
		switch (inCommand) {

			case cmd_Plain:
			case cmd_Bold:
			case cmd_Italic:
			case cmd_Underline:
			case cmd_Outline:
			case cmd_Shadow:
			case cmd_Condense:
			case cmd_Extend:
			case cmd_FontLarger:
			case cmd_FontSmaller:
			{
				StAEDescriptor	doerEvent;
				LAEStream		stream(kAEVTextSuite, kAEModify);
				stream.WriteKey(keyDirectObject);
				stream.WriteSpecifier(mSelection);
				switch (inCommand) {

					case cmd_Plain:
						stream.WriteKey(keyAEModifyProperty);
						stream.WriteEnumDesc(pTextStyles);
						stream.WriteKey(keyAEModifyToValue);
						UFontsStyles::WriteStyles(stream, normal);
						break;

					case cmd_Bold:
					case cmd_Italic:
					case cmd_Underline:
					case cmd_Outline:
					case cmd_Shadow:
					case cmd_Condense:
					case cmd_Extend:
						stream.WriteKey(keyAEModifyProperty);
						stream.WriteEnumDesc(pTextStyles);
						stream.WriteKey(keyAEModifyByValue);
						UFontsStyles::WriteStyles(stream, 0x1L << (inCommand - cmd_Bold));
						break;
						
					case cmd_FontLarger:
					case cmd_FontSmaller:
					{
						Int16	value = (inCommand == cmd_FontSmaller)
											?	-1
											:	1;
						stream.WriteKey(keyAEModifyProperty);
						stream.WriteEnumDesc(pPointSize);
						stream.WriteKey(keyAEModifyByValue);
						stream.WriteDesc(typeShortInteger, &value, sizeof(value));
						break;
					}

				}
				stream.Close(&doerEvent.mDesc);
				LAEAction	*action = new LAEAction();
				action->SetRedoAE(doerEvent);
				action->SetSelection(mSelection, typeNull, typeNull);
				mSelection->UndoAddOldData(action);
				action->SetRecalcAccumulator(mTextEngine);
				action->SetRecordOnlyFinalState(true);
				PostAction(action);
				handled = true;
				break;
			}
			
			case cmd_FontOther:
			{
				LStyleSet		*style = NULL;
				if (mTextSelection)
					style = mTextSelection->GetCurrentStyle();
				ThrowIfNULL_(style);
				StAEDescriptor	value;
				style->GetAEProperty(pPointSize, UAppleEventsMgr::sAnyType, value);
				LAESubDesc		valueSD(value);
				Int32			presentSize = valueSD.ToInt32(),
								newSize = presentSize;
				
				if (
					UModalDialogs::AskForOneNumber(this, Wind_FontSizeDialog, pane_FontSize, newSize) &&
					(newSize != presentSize)
				) {
					StAEDescriptor	doerEvent;
					LAEStream		stream(kAEVTextSuite, kAEModify);
					stream.WriteKey(keyDirectObject);
					stream.WriteSpecifier(mSelection);
					stream.WriteKey(keyAEModifyProperty);
					stream.WriteEnumDesc(pPointSize);
					stream.WriteKey(keyAEModifyToValue);
					stream.WriteDesc(typeLongInteger, &newSize, sizeof(newSize));
					stream.Close(&doerEvent.mDesc);
					LAEAction	*action = new LAEAction();
					action->SetRedoAE(doerEvent);
					action->SetSelection(mSelection);
					mSelection->UndoAddOldData(action);
					action->SetRecalcAccumulator(mTextEngine);
					action->SetRecordOnlyFinalState(true);
					PostAction(action);
				}
				handled = true;
				break;
			}
	
			default:
				handled = inherited::ObeyCommand(inCommand, ioParam);
				break;
		}
	}
		
	return handled;
}

void	VTextView::FindCommandStatus(
	CommandT	inCommand,
	Boolean		&outEnabled,
	Boolean		&outUsesMark,
	Char16		&outMark,
	Str255		outName)
{
	ResIDT		menuNo;
	Int16		itemNo;
	Boolean		puntToStyle = false;
	
	if (IsSyntheticCommand(inCommand, menuNo, itemNo)) {
		switch (menuNo) {
			case MENU_Font:
				UFontMenu::EnableMenu();
				UFontMenu::EnableEveryItem();
				if (menuNo == 0) {
					UFontMenu::DisableEveryItem();
					break;
				}
				//	fall through
				
			case MENU_Size:
			case MENU_Style:
				puntToStyle = true;
				break;
		}
	}
	
	if (!puntToStyle) {
		switch (inCommand) {
			
			case cmd_FontMenuItem:
			case cmd_SizeMenuItem:
			case cmd_StyleMenuItem:
				outEnabled = true;
				break;

			case cmd_Plain:
			case cmd_Bold:
			case cmd_Italic:
			case cmd_Underline:
			case cmd_Outline:
			case cmd_Shadow:
			case cmd_Condense:
			case cmd_Extend:
			case cmd_FontLarger:
			case cmd_FontSmaller:
			case cmd_FontOther:
				puntToStyle = true;
				break;
		}
	}
	
	LStyleSet		*style = NULL;
	if (mTextSelection)
		style = mTextSelection->GetCurrentStyle();

	if (puntToStyle && style) {
		Boolean	found = style->FindCommandStatus(inCommand, outEnabled, outUsesMark, outMark, outName);
//?		Assert_(found);
	} else {
		switch (inCommand) {
			
			case cmd_FontMenuItem:
			case cmd_SizeMenuItem:
			case cmd_StyleMenuItem:
				outEnabled = true;
				break;

			default:
				inherited::FindCommandStatus(inCommand, outEnabled, outUsesMark, outMark, outName);
				break;
		}
	}
	
}
*/