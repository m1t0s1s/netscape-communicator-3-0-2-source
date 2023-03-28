//	===========================================================================
//	WTextMenuAttachments
//
//	Copyright 1996 ViviStar Consulting.
//	===========================================================================

#include	"WTextMenuAttachments.h"
#include 	<VText_Commands.h>
#include	<LStream.h>

#include 	<LTextView.h>
#include 	<LTextSelection.h>
#include 	<LStyleSet.h>
#include	<AEVTextSuite.h>
#include	<UFontsStyles.h>

#include	<LMenu.h>
#include	<LMenuBar.h>
#include	<LString.h>
#include	<UAEGizmos.h>
#include	<UAppleEventsMgr.h>
#include	<UModalDialogs.h>
#include	<LAEAction.h>

//	---------------------------------------------------------------------------

VTextMenuAttachment::VTextMenuAttachment(
	LTextView	*inView,
	Int16		inMenuID)
:	LAttachment(msg_AnyMessage)
{
	mMenuID = inMenuID;
	mTextView = NULL;
	mMenuH = NULL;
	mMenu = NULL;

	mTextView = inView;
	ThrowIfNULL_(mTextView);
	
}

VTextMenuAttachment::VTextMenuAttachment(LStream* inStream)
	:	LAttachment(inStream)
{
	mTextView = NULL;
	mMenuH = NULL;
	mMenu = NULL;

	mTextView = dynamic_cast<LTextView*>(LAttachable::GetDefaultAttachable());
	ThrowIfNULL_(mTextView);
	
	(*inStream) >> mMenuID;
}

void	VTextMenuAttachment::CheckMenu(void)
{
	if (!mMenu) {
		mMenu = LMenuBar::GetCurrentMenuBar()->FetchMenu(mMenuID);
		ThrowIfNULL_(mMenu);
	}
	
	if (!mMenuH) {
		mMenuH = GetMenuHandle(mMenuID);
		ThrowIfNULL_(mMenuH);
	}
}

void	VTextMenuAttachment::EnableEveryItem(Boolean inEnable) const
{
	Int16	n = ::CountMItems(mMenuH),
			i;
	
	for (i = 1; i <= n; i++) {
		if (inEnable)
			::EnableItem(mMenuH, i);
		else
			::DisableItem(mMenuH, i);
	}
}

void	VTextMenuAttachment::CheckEveryItem(Boolean inApplyCheck) const
{
	Int16	n = ::CountMItems(mMenuH),
			i;
	
	for (i = 1; i <= n; i++) {
		::CheckItem(mMenuH, i, inApplyCheck);
	}
}

void	VTextMenuAttachment::ExecuteSelf(
	MessageT		inMessage,
	void			*ioParam)
{
	Boolean	executeHost = true;
	
	CheckMenu();
	
	if (inMessage == msg_CommandStatus) {
		SCommandStatusP status = (SCommandStatusP)ioParam;

		executeHost = !FindCommandStatus(
										status->command,
										*status->enabled,
										*status->usesMark,
										*status->mark,
										status->name
		);
	} else {
		executeHost = !ObeyCommand(inMessage, ioParam);
	}
	
	SetExecuteHost(executeHost);
}

LAEAction *	VTextMenuAttachment::MakeModifyAction(Int32 inForValue)
{
	LTextSelection	*selection = mTextView->GetTextSelection();
	LTextEngine		*engine = mTextView->GetTextEngine();

	StAEDescriptor	doerEvent;
	{
		LAEStream		stream(kAEVTextSuite, kAEModify);
		stream.WriteKey(keyDirectObject);
		stream.WriteSpecifier(selection);
		WriteModifyParameters(stream, inForValue);
		stream.Close(&doerEvent.mDesc);
	}

	LAEAction	*action = new LAEAction();
	action->SetRedoAE(doerEvent);
	action->SetSelection(selection, typeNull, typeNull);
	selection->UndoAddOldData(action);
	action->SetRecalcAccumulator(engine);
	action->SetRecordOnlyFinalState(true);
	
	return action;
}

//	---------------------------------------------------------------------------
//	Font menu

VFontMenuAttachment::VFontMenuAttachment(
	LTextView	*inView,
	Int16		inMenuID)
:	VTextMenuAttachment(inView, inMenuID)
{
}

VFontMenuAttachment::VFontMenuAttachment(LStream* inStream)
:	VTextMenuAttachment(inStream)
{
}

void	VFontMenuAttachment::CheckMenu(void)
{
	if (!mMenuH) {
		inherited::CheckMenu();
		
		if (!::CountMItems(mMenuH))
			::AppendResMenu(mMenuH, 'FONT');
	}
}

Int32	VFontMenuAttachment::FontNumberToItem(Int16 inFontNumber) const
{
	Int16	n = ::CountMItems(mMenuH),
			i;
	
	for (i = 1; i <= n; i++) {
		static Str255	name;
		Int16			number;
		
		::GetMenuItemText(mMenuH, i, name);
		number = UFontsStyles::GetFontId(name);
		
		if (number == inFontNumber)
			break;
	}
	
	Assert_(i != n);
	
	return i;
}

Boolean VFontMenuAttachment::FindCommandStatus(
	CommandT			inCommand,
	Boolean				&outEnabled,
	Boolean				&outUsesMark,
	Char16				&outMark,
	Str255				outName)
{
	Boolean		handled = false;
	ResIDT		menuNo;
	Int16		itemNo;
	
	if (LCommander::IsSyntheticCommand(inCommand, menuNo, itemNo)) {
		if ((menuNo == mMenuID) && (itemNo == 0)) {
			LTextSelection	*selection = mTextView->GetTextSelection();
			LStyleSet		*set = selection ? selection->GetCurrentStyle() : NULL;

			ThrowIfNULL_(set);

			::EnableItem(mMenuH, 0);
			EnableEveryItem(true);
			CheckEveryItem(false);
			::CheckItem(mMenuH, FontNumberToItem(set->GetFontNumber()), true);
			outEnabled = true;
			handled = true;
		}
	} else if (inCommand == cmd_FontMenuItem) {
		outEnabled = true;
		handled = true;
	}
	
	return handled;
}

Boolean	VFontMenuAttachment::ObeyCommand(
	CommandT			inCommand,
	void				*ioParam)
{
	Boolean		handled = false;
	ResIDT		menuNo;
	Int16		itemNo;
	
	if (LCommander::IsSyntheticCommand(inCommand, menuNo, itemNo)) {
		if (menuNo == mMenuID) {
			LCommander::PostAnAction(
				MakeModifyAction(itemNo)
			);
			handled = true;
		}
	}
	
	return handled;
}

void	VFontMenuAttachment::WriteModifyParameters(
	LAEStream		&inStream,
	Int32			inForValue) const
{
	Str255	fontName;
	
	::GetMenuItemText(mMenuH, inForValue, fontName);
	if (fontName[0] == 0)
		Throw_(paramErr);

	inStream.WriteKey(keyAEModifyProperty);
	inStream.WriteEnumDesc(pFont);
	inStream.WriteKey(keyAEModifyToValue);
	inStream.WriteDesc(typeChar, fontName + 1, fontName[0]);
}

//	---------------------------------------------------------------------------
//	Size menu

VSizeMenuAttachment::VSizeMenuAttachment(
	LTextView	*inView,
	Int16		inMenuID)
:	VTextMenuAttachment(inView, inMenuID)
{
}

VSizeMenuAttachment::VSizeMenuAttachment(LStream* inStream)
:	VTextMenuAttachment(inStream)
{
}

Boolean VSizeMenuAttachment::FindCommandStatus(
	CommandT			inCommand,
	Boolean				&outEnabled,
	Boolean				&outUsesMark,
	Char16				&outMark,
	Str255				outName)
{
	ResIDT			menuNo;
	Int16			itemNo;
	Boolean			handled = false;
	static Int16	itemChecked = 0;
	
	if (LCommander::IsSyntheticCommand(inCommand, menuNo, itemNo)) {
		if (menuNo == mMenuID) {
			LTextSelection	*selection = mTextView->GetTextSelection();
			LStyleSet		*set = selection ? selection->GetCurrentStyle() : NULL;
			ThrowIfNULL_(set);
			
			Str255	str;
			long	aLong;
			::GetMenuItemText(mMenuH, itemNo, str);
			::StringToNum(str, &aLong);
			
			if (itemNo == 1)
				itemChecked = 0;
				
			outEnabled = true;
			outUsesMark = true;
			
			if (set->GetFontSize() == aLong) {
				outMark = checkMark;
				itemChecked = itemNo;
			} else {
				outMark = noMark;
			}
		
			Style	style = RealFont(set->GetFontNumber(), aLong)
								?	outline
								:	normal;
								
			::SetItemStyle(mMenuH, itemNo, style);
			
			handled = true;
		}
	} else {
		switch (inCommand) {
			case cmd_SizeMenuItem:
			case cmd_FontLarger:
			case cmd_FontSmaller:
				outEnabled = true;
				handled = true;
				break;

			case cmd_FontOther:
			{
				outEnabled = true;
				outUsesMark = true;
				if (!itemChecked) {
					outMark = checkMark;
				} else {
					outMark = noMark;
				}
				Int16	itemNo = mMenu->IndexFromCommand(cmd_FontOther);
				if (itemNo) {
					LTextSelection	*selection = mTextView->GetTextSelection();
					LStyleSet		*set = selection ? selection->GetCurrentStyle() : NULL;
					ThrowIfNULL_(set);
					
					LStr255	oldString,
							newString;
					::GetMenuItemText(mMenuH, itemNo, oldString);
					Uint8	oldLBracket = oldString.Find('('),
							oldRBracket = oldString.Find(')');
					
					Str32	numString;
					
					::NumToString(set->GetFontSize(), numString);
					newString.Assign(oldString, 1, oldLBracket);
					newString.Append(numString);
					newString.Append(&oldString[oldRBracket], oldString.Length() - oldRBracket +1);
					
					::SetMenuItemText(mMenuH, itemNo, newString);
				}
				handled = true;
				break;
			}
		}
	}
	
	return handled;
}

Boolean	VSizeMenuAttachment::ObeyCommand(
	CommandT			inCommand,
	void				*ioParam)
{
	Boolean		handled = false;
	ResIDT		menuNo;
	Int16		itemNo;
	
	if (LCommander::IsSyntheticCommand(inCommand, menuNo, itemNo)) {
		if (menuNo == mMenuID) {
			Str255	itemStr;
			long	aLong;
			::GetMenuItemText(mMenuH, itemNo, itemStr);
			StringToNum(itemStr, &aLong);

			LAEAction	*action = MakeModifyAction(aLong);
			LCommander::PostAnAction(action);
			handled = true;
		}
	} else {
		switch (inCommand) {
			case cmd_FontLarger:
			case cmd_FontSmaller:
			{
				Int32	encoding = (inCommand == cmd_FontSmaller) ? -1 : -2;
				LAEAction	*action = MakeModifyAction(encoding);
				LCommander::PostAnAction(action);
				handled = true;
				break;
			}

			case cmd_FontOther:
			{
				LTextSelection	*selection = mTextView->GetTextSelection();
				LStyleSet		*set = selection ? selection->GetCurrentStyle() : NULL;
				ThrowIfNULL_(set);
				Int32			presentSize = set->GetFontSize(),
								newSize = presentSize;
				
				if (
					UModalDialogs::AskForOneNumber(LCommander::GetTarget(), kVText_WIND_ID_FontSize, kVText_PaneID_FontSize, newSize) &&
					(newSize != presentSize)
				) {
					LAEAction	*action = MakeModifyAction(newSize);
					LCommander::PostAnAction(action);
				}
				handled = true;
				break;
			}
		}
	}
		
	return handled;
}

void	VSizeMenuAttachment::WriteModifyParameters(
	LAEStream		&inStream,
	Int32			inForValue) const
{
	Int16	value = inForValue;
	
	inStream.WriteKey(keyAEModifyProperty);
	inStream.WriteEnumDesc(pPointSize);
	
	if (value > 0) {
		inStream.WriteKey(keyAEModifyToValue);
	} else {
		inStream.WriteKey(keyAEModifyByValue);

		value = (inForValue == -1)
			?	-1
			:	1;
	}
	inStream.WriteDesc(typeShortInteger, &value, sizeof(value));
}

//	---------------------------------------------------------------------------
//	Style menu

VStyleMenuAttachment::VStyleMenuAttachment(
	LTextView	*inView,
	Int16		inMenuID)
:	VTextMenuAttachment(inView, inMenuID)
{
}

VStyleMenuAttachment::VStyleMenuAttachment(LStream* inStream)
:	VTextMenuAttachment(inStream)
{
}

Boolean VStyleMenuAttachment::FindCommandStatus(
	CommandT			inCommand,
	Boolean				&outEnabled,
	Boolean				&outUsesMark,
	Char16				&outMark,
	Str255				outName)
{
	Boolean		handled = false;
	
	switch (inCommand) {
		case cmd_Plain:
		case cmd_Bold:
		case cmd_Italic:
		case cmd_Underline:
		case cmd_Outline:
		case cmd_Shadow:
		case cmd_Condense:
		case cmd_Extend:
		{
			outEnabled = true;
			outUsesMark = true;
			outMark = noMark;
			{
				LTextSelection	*selection = mTextView->GetTextSelection();
				LStyleSet		*set = selection ? selection->GetCurrentStyle() : NULL;

				ThrowIfNULL_(set);

				if (inCommand == cmd_Plain) {
					outMark = (set->GetFontFace() == normal)
									?	checkMark
									:	noMark;
				} else {
					Int32	bit = inCommand - cmd_Bold;
					bit = 0x1L << bit;
					outMark = (bit & set->GetFontFace())
									?	checkMark
									:	noMark;
				}
			}
			
			::EnableItem(mMenuH, 0);
			
			handled = true;
			break;
		}
		
		case cmd_StyleMenuItem:
			outEnabled = true;
			handled = true;
			break;
	}
	
	return handled;
}

Boolean	VStyleMenuAttachment::ObeyCommand(
	CommandT			inCommand,
	void				*ioParam)
{
	Boolean		handled = false;
	
	switch (inCommand) {
		case cmd_Plain:
		case cmd_Bold:
		case cmd_Italic:
		case cmd_Underline:
		case cmd_Outline:
		case cmd_Shadow:
		case cmd_Condense:
		case cmd_Extend:
		{
			LAEAction	*action = MakeModifyAction(inCommand);
			LCommander::PostAnAction(action);
			handled = true;
			break;
		}
	}
	
	return handled;
}

void	VStyleMenuAttachment::WriteModifyParameters(
	LAEStream		&inStream,
	Int32			inForValue) const
{

	inStream.WriteKey(keyAEModifyProperty);
	inStream.WriteEnumDesc(pTextStyles);
	switch (inForValue) {
		case cmd_Plain:
			inStream.WriteKey(keyAEModifyToValue);
			UFontsStyles::WriteStyles(inStream, normal);
			break;

		case cmd_Bold:
		case cmd_Italic:
		case cmd_Underline:
		case cmd_Outline:
		case cmd_Shadow:
		case cmd_Condense:
		case cmd_Extend:
			inStream.WriteKey(keyAEModifyByValue);
			UFontsStyles::WriteStyles(inStream, 0x1L << (inForValue - cmd_Bold));
			break;
		
		default:
			Assert_(false);
			break;
	}
}


