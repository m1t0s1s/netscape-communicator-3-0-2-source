//	===========================================================================
//	VOTextModel.cp
//
//	Copyright 1996 ViviStar Consulting.
//	===========================================================================

#ifdef PowerPlant_PCH
#include PowerPlant_PCH
#endif

#include	"VOTextModel.h"
#include	<VStyleSet.h>
#include	<UFontsStyles.h>
#include	<LRulerSet.h>
#include	<AETextSuite.h>
#include	<UAppleEventsMgr.h>
#include	<LDataTube.h>
#include	<LStream.h>
#include	<VTextElemAEOM.h>
#include	<VTextView.h>
#include	<VTextHandler.h>
#include	<LTextSelection.h>
#include	<UMemoryMgr.h>
#include	<AEVTextSuite.h>
#include	<algo.h>

#pragma	warn_unusedarg off

//	===========================================================================
//	Maintenance

VOTextModel::VOTextModel()
{
	mStyleListBehavior = setBehavior_Ignore;
mStyleListBehavior = setBehavior_FreeForm;	//-
	mRulerListBehavior = setBehavior_Ignore;
}

/*-
VOTextModel::VOTextModel(LTextEngine *inTextEngine)
:	LTextModel(inTextEngine)
{
	mStyleListBehavior = setBehavior_Ignore;
mStyleListBehavior = setBehavior_FreeForm;	//-
	mRulerListBehavior = setBehavior_Ignore;
	
	mImageWidth = 0;
	if (mTextEngine) {
		SDimension32	size;
		mTextEngine->GetImageSize(&size);
		mImageWidth = size.width;
	}
	
	if (mTextEngine) {
		if (mTextEngine->GetDefaultStyleSet())
			AddStyle(mTextEngine->GetDefaultStyleSet());
		if (mTextEngine->GetDefaultRulerSet())
			AddRuler(mTextEngine->GetDefaultRulerSet());
	}
}
*/

VOTextModel::~VOTextModel()
{
	for_each(mStyleList.begin(), mStyleList.end(), replaceSharableF<LStyleSet*>(NULL, this));
	for_each(mRulerList.begin(), mRulerList.end(), replaceSharableF<LRulerSet*>(NULL, this));
}

#ifndef	PP_No_ObjectStreaming

void	VOTextModel::WriteObject(LStream &inStream) const
{
	//	no simple data
	
	LTextModel::WriteObject(inStream);
}

void	VOTextModel::ReadObject(LStream &inStream)
{
	//	no simple data

	LTextModel::ReadObject(inStream);
}

#endif

void	VOTextModel::SetTextEngine(LTextEngine *inTextEngine)
{
	if (mTextEngine != inTextEngine) {
		inherited::SetTextEngine(inTextEngine);
		
		if (mTextEngine) {
			if (mTextEngine->GetDefaultStyleSet())
				AddStyle(mTextEngine->GetDefaultStyleSet());
			if (mTextEngine->GetDefaultRulerSet())
				AddRuler(mTextEngine->GetDefaultRulerSet());
		}
	}
}

//	===========================================================================
//	Implementation

LTextElemAEOM *	VOTextModel::MakeNewElem(
	const TextRangeT	&inRange,
	DescType			inKind,
	const LModelObject	*inSuperModel) const
{
	LTextElemAEOM	*rval = NULL;
	
	if (inSuperModel) {
	
		rval = MakeNewElemSelf(inRange, inKind, inSuperModel);

	} else {
		
		ThrowIfNULL_(mTextEngine);
		
		//	Base off a paragraph based off the head model
		TextRangeT	range = inRange,
					totRange = mTextEngine->GetTotalRange(),
					paraRange;
		
		range.WeakCrop(totRange);
		
		if (
				range.IsBefore(totRange) ||
				range.IsAfter(totRange) ||
				!totRange.Length()
		) {
			rval = MakeNewElem(range, inKind, mTextLink);
		} else {
			paraRange = range;
			mTextEngine->FindPart(LTextEngine::sTextAll, &paraRange, cParagraph);

Assert_(paraRange.Length());

			//	Filter out rangeCode_Before's at the end of paragraph delimiters
			Boolean	filter = false;
			if (
					(range.Insertion() == rangeCode_Before) &&
					(range.End() == paraRange.End())
			) {
				Int32	offset = paraRange.End();
				offset -= mTextEngine->PrevCharSize(offset);
				if (mTextEngine->ThisChar(offset) == 0x0d)
					filter = true;
			}

			if (filter) {
				range.SetInsertion(rangeCode_After);
				
				if (!range.Length() && (range.Start() == totRange.End())) {
					rval = MakeNewElem(range, inKind, mTextLink);
				} else {
					//	find the real paragraph to base it off of
					paraRange = range;
					mTextEngine->FindPart(LTextEngine::sTextAll, &paraRange, cParagraph);
				}
			} else if ((inKind == cParagraph) && (range == paraRange)) {
				rval = MakeNewElem(range, inKind, mTextLink);
			}
		}
		
		if (!rval) {
			LTextElemAEOM	*paraElem = MakeNewElem(paraRange, cParagraph, mTextLink);
			try {
				rval = MakeNewElem(range, inKind, paraElem);
			} catch (ExceptionCode inErr) {
				delete paraElem;
				throw(inErr);
			} catch (...) {
				delete paraElem;
				throw;
			}
		}
	}

	Assert_(rval && rval->GetSuperRange().WeakContains(rval->GetRange()));

	return rval;
}

LTextElemAEOM *	VOTextModel::MakeNewElemSelf(
	const TextRangeT	&inRange,
	DescType			inKind,
	const LModelObject	*inSuperModel
) const
{
	return new VTextElemAEOM(inRange, inKind, (LModelObject *)inSuperModel, (LTextModel *)this, mTextEngine);
}

LModelObject*	VOTextModel::HandleCreateElementEvent(
	DescType			inElemClass,
	DescType			inInsertPosition,
	LModelObject		*inTargetObject,
	const AppleEvent	&inAppleEvent,

	AppleEvent			&outAEReply)
{
	LModelObject	*rval = NULL;

	switch(inElemClass) {
	
		case cStyleSet:
		case cRulerSet:
		{
			LStyleSet		*newStyle = NULL;
			LRulerSet		*newRuler = NULL;
			LSharableModel	*candidate;
			if (inElemClass == cStyleSet) {
				newStyle = new VStyleSet();
				candidate = newStyle;
			} else if (inElemClass == cRulerSet) {
				newRuler = new LRulerSet();
				candidate = newRuler;
			} else {
				Throw_(paramErr);
			}
			StSharer		lock(candidate);	//	purge candidate incase of failure

			LAESubDesc	ae(inAppleEvent),
						initialProperties = ae.KeyedItem(keyAEPropData),
						property;
			Int32		i,
						count = initialProperties.CountItems();
			DescType	key;
			
			for (i = 1; i <= count; i++) {
				StAEDescriptor	propDesc,
								bogusDesc;

				property = initialProperties.NthItem(i, &key);
				property.ToDesc(&propDesc.mDesc);
				candidate->SetAEProperty(key, propDesc, bogusDesc);
			}
			
			if (newStyle)
				AddStyle(newStyle);
			if (newRuler)
				AddRuler(newRuler);
			rval = candidate;
			break;
		}

		default:
			rval = LTextModel::HandleCreateElementEvent(inElemClass, inInsertPosition,
						inTargetObject, inAppleEvent, outAEReply);
			break;
	}
	return rval;
}

void	VOTextModel::AddFlavorTypesTo(
	const TextRangeT	&inRange,
	LDataTube			*inTube) const
{
	TextRangeT	range = inRange;
	range.Crop(mTextEngine->GetTotalRange());
	
	if (inRange.Length() > 0) {
//?		if rulersheet behavior && !onlyReqdFlavors
			inTube->AddFlavor(typeVRulerSets);
			inTube->AddFlavor(typeVRulerSetRuns);

//?		if stylesheet behavior && !onlyReqdFlavors
			inTube->AddFlavor(typeVStyleSets);
			inTube->AddFlavor(typeVStyleSetRuns);

//?		if free form style behavior && !onlyReqdFlavors
			inTube->AddFlavor(typeScrapStyles);
	}
	
	inherited::AddFlavorTypesTo(range, inTube);
}

Boolean	VOTextModel::SendFlavorTo(
	FlavorType			inFlavor,
	const TextRangeT	&inRange,
	LDataTube			*inTube) const
{
	TextRangeT	range = inRange;
	Boolean		rval = false;
	
	range.Crop(mTextEngine->GetTotalRange());
	
	switch (inFlavor) {

		case typeVStyleSets:
		case typeVStyleSetRuns:
		//	!!!!!	parallel code in typeVRulerSet... below	!!!!!
		{
			vector<LStyleSet*>				uniqueSets;
			vector<LStyleSet*>::iterator	ip;
			Int32							count = mTextEngine->CountStyleSets(range),
											i;
			LStyleSet						*set;
			LStream							&stream = inTube->SetFlavorStreamOpen(inFlavor);
			
			for (i = 0; i < count; i++) {
				set = mTextEngine->GetStyleSet(range, i + 1);
				
				ip = find(uniqueSets.begin(), uniqueSets.end(), set);
				if (ip == uniqueSets.end())
					uniqueSets.push_back(set);
			}
			
			if (inFlavor == typeVStyleSets) {
				for (ip = uniqueSets.begin(); ip != uniqueSets.end(); ip++)
					(*ip)->WriteFlavorRecord(typeVStyleSets, stream);
			} else if (inFlavor == typeVStyleSetRuns) {
				TextRangeT	setRange = range;
				Int32		index;
				
				setRange.Front();
				while (setRange.Start() < range.End()) {
					set = mTextEngine->GetStyleSet(setRange);
					mTextEngine->GetStyleSetRange(&setRange);
					setRange.Crop(range);
					index = find(uniqueSets.begin(), uniqueSets.end(), set) - uniqueSets.begin();
			
					Assert_(setRange.Length());
					stream << setRange.Length();
					stream << index;
					setRange.After();
				}
			}
			
			inTube->SetFlavorStreamClose();
			break;
		}

		case typeVRulerSets:
		case typeVRulerSetRuns:
		//	!!!!!	parallel code in typeVStyleSet... above	!!!!!
		{
			vector<LRulerSet*>				uniqueSets;
			vector<LRulerSet*>::iterator	ip;
			Int32							count = mTextEngine->CountRulerSets(range),
											i;
			LRulerSet						*set;
			LStream							&stream = inTube->SetFlavorStreamOpen(inFlavor);
			
			for (i = 0; i < count; i++) {
				set = mTextEngine->GetRulerSet(range, i + 1);
				
				ip = find(uniqueSets.begin(), uniqueSets.end(), set);
				if (ip == uniqueSets.end())
					uniqueSets.push_back(set);
			}
			
			if (inFlavor == typeVRulerSets) {
				for (ip = uniqueSets.begin(); ip != uniqueSets.end(); ip++)
					(*ip)->WriteFlavorRecord(typeVRulerSets, stream);
			} else if (inFlavor == typeVRulerSetRuns) {
				TextRangeT	setRange = range;
				Int32		index;
				
				setRange.Front();
				while (setRange.Start() < range.End()) {
					set = mTextEngine->GetRulerSet(setRange);
					mTextEngine->GetRulerSetRange(&setRange);
					setRange.Crop(range);
					index = find(uniqueSets.begin(), uniqueSets.end(), set) - uniqueSets.begin();
			
					Assert_(setRange.Length());
					stream << setRange.Length();
					stream << index;
					setRange.After();
				}
			}
			
			inTube->SetFlavorStreamClose();
			break;
		}

		case typeScrapStyles:
		{
			if (range.Length()) {
				TextRangeT			styleRange = range;
				LStream				&stream = inTube->SetFlavorStreamOpen(inFlavor);
				
				Int32	count = mTextEngine->CountStyleSets(range);
				ThrowIf_(count > max_Int16);
				short	count2 = count;
				
				stream << count2;	//	style record count
				
				styleRange.Front();
				while (styleRange.Start() < range.End()) {
					LStyleSet	*style = mTextEngine->GetStyleSet(styleRange);

					stream << (long)(styleRange.Start() - range.Start());	//	offset
					style->WriteFlavorRecord(typeScrapStyles, stream);	//	font info

					mTextEngine->GetStyleSetRange(&styleRange);
					styleRange.After();
				}

				inTube->SetFlavorStreamClose();
				rval = true;
			}
			break;
		}
		
		default:
			rval = inherited::SendFlavorTo(inFlavor, range, inTube);
			break;
	}

	return rval;
}

FlavorType	VOTextModel::PickFlavorFrom(
	const TextRangeT	&inRange,
	const LDataTube		*inTube) const
{
	Assert_(inTube);
	
	FlavorType	rval = typeNull,
				flavor;
	Int32		n = inTube->CountItems();
	
	do {
		if (inTube->FlavorExists(typeChar)) {
			rval = typeChar;
			break;
		}
		
		for (Int32 i = 1; i <= n; i++) {
			LDataTube	subTube = inTube->NthItem(i);
			flavor = PickFlavorFrom(inRange, &subTube);
			
			if (flavor == typeNull) {
				rval = flavor;
				break;
			}
			
			if (rval == typeNull) {
				rval = flavor;
			} else if (rval != flavor) {
				rval = typeWildCard;
			}
		}
//?+
if (rval != typeNull) rval = typeWildCard;

	} while (false);

	return rval;
}

/*
FlavorType	VOTextModel::PickFlavorFrom(
	const TextRangeT	&inRange,
	const LDataTube		*inTube)
{
	Assert_(inTube);
	
	FlavorType	rval = typeNull,
				flavor;
	Int32		n = inTube->CountItems();
	
	do {
		if (inTube->FlavorExists(typeWildCard)) {
			rval = PickFlavorSelf(inTube);
			if (rval == typeNull)
				break;
		}
		
		for (i = 1; i <= n; i++) {
			LDataTube subTube = inTube->NthItem(i);
			flavor = PickFlavorFrom(inRange, &subTube)
			
			if (flavor == typeNull) {
				rval = flavor;
				break;
			}
			
			if (rval == typeNull) {
				rval = flavor;
			} else if (rval != flavor) {
				rval = typeWildCard;
			}
		}
	} while (false);
}

FlavorType	VOTextModel::PickFlavorFromSelf(...)
{
	if (inTube->FlavorExists(typeChar))
		return typeChar;
	else
		return typeNull;
}
*/		

void	VOTextModel::ReceiveDataFrom(
	const TextRangeT	&inRange,
	LDataTube			*inTube)
{
	FlavorType	textFlavor = typeNull;
	TextRangeT	preRange = inRange,
				postRange,
				oldTotRange = mTextEngine->GetTotalRange();

	preRange.Crop(oldTotRange);

	if (inTube->FlavorExists(typeIntlText)) {
		ScriptCode		script;
		LangCode		language;

		Int32			size = inTube->GetFlavorSize(typeIntlText) - (sizeof(script) + sizeof(language));
		if (size > 0) {
			LStream			&stream = inTube->GetFlavorStreamOpen(typeIntlText);
			StPointerBlock	data(size);
			stream >> script;
			stream >> language;
			stream.ReadBlock(data.mPtr, size);
			inTube->GetFlavorStreamClose();

			mTextEngine->TextReplaceByPtr(preRange, data.mPtr, size);
			TextRangeT	newRange(preRange.Start(), preRange.Start() + size);
			LStyleSet	*set = StyleSetForScript(newRange, script);
			ThrowIfNULL_(set);
			mTextEngine->SetStyleSet(newRange, set);
		} else {
			mTextEngine->TextDelete(preRange);
		}
		textFlavor = typeIntlText;
	} else if (inTube->FlavorExists(typeChar)) {
		inherited::ReceiveDataFrom(inRange, inTube);
		textFlavor = typeChar;
	} else {
		Int32	n = inTube->CountItems();
		for (Int32 i = 1; i <= n; i++) {
			LDataTube	subTube = inTube->NthItem(i);
			ReceiveDataFrom(inRange, &subTube);
		}
	}
	
	if (textFlavor != typeNull) {

		//	Text has been entered.  Now for the styles...
		postRange = preRange;
		postRange.SetLength(preRange.Length() + (mTextEngine->GetTotalRange().Length() - oldTotRange.Length()), rangeCode_After);

		if (postRange.Length()) {
			Assert_(mTextEngine->GetTotalRange().Contains(postRange));
			try {
			
				FlavorType	styleFlavor = typeNull,
							rulerFlavor = typeNull;
				
				//	stylesets
				switch (GetStyleListBehavior()) {

					case setBehavior_Sheet:
						if (inTube->FlavorExists(typeVStyleSets) && inTube->FlavorExists(typeVStyleSetRuns))
							styleFlavor = typeVStyleSets;
						break;

					case setBehavior_FreeForm:
					case setBehavior_FreeSheet:
						if (inTube->FlavorExists(typeVStyleSets) && inTube->FlavorExists(typeVStyleSetRuns))
							styleFlavor = typeVStyleSets;
						else if (inTube->FlavorExists(typeScrapStyles))
							styleFlavor = typeScrapStyles;
						break;
					
					default:
						break;
				}
				if (styleFlavor != typeNull)
					ApplyStylesFrom(styleFlavor, postRange, inTube);
				
				//	rulersets
				switch (GetRulerListBehavior()) {

					case setBehavior_Sheet:
					case setBehavior_FreeForm:
					case setBehavior_FreeSheet:
						if (inTube->FlavorExists(typeVRulerSets) && inTube->FlavorExists(typeVRulerSetRuns))
							styleFlavor = typeVRulerSets;
						break;
					
					default:
						break;
				}
				if (rulerFlavor != typeNull)
					ApplyRulersFrom(rulerFlavor, postRange, inTube);
				
			} catch (ExceptionCode inErr) {
				//	The text characters were already imported so don't throw out
				Assert_(false);
			} catch (...) {
				//	The text characters were already imported so don't throw out
				Assert_(false);
			}
		}
	}
}

//	----------------------------------------------------------------------

void	VOTextModel::Modify(
	const TextRangeT	&inRange,
	DescType			inProperty,
	DescType			inOperator,
	const LAESubDesc	&inValue)
{
	StAEDescriptor	value;
	if (inOperator == keyAEModifyToValue)
		inValue.ToDesc(&value.mDesc);
		
	switch (GetStyleListBehavior()) {
	
		case setBehavior_Sheet:
			Throw_(errAEEventNotHandled);
			break;
			
		case setBehavior_FreeSheet:
			Throw_(paramErr);	//	not implemented yet
			break;
			
		case setBehavior_FreeForm:
		{
			Int32			i,
							count = mTextEngine->CountStyleSets(inRange);
			LStyleSet		*set;
			VTextHandler	*handler = dynamic_cast<VTextHandler *>(mTextEngine->GetHandlerView()->GetEventHandler());
			Boolean			doPendingStyle = mSelection && (mSelection->ListCount() == 1) && !mSelection->IsSubstantive();
			
			vector<LStyleSet*>	uniqueSets;
			{
				if (doPendingStyle) {
					ThrowIfNULL_(mTextSelection);
					set = mTextSelection->GetCurrentStyle();
					uniqueSets.push_back(set);
				} else {
					for (i = 1; i <= count; i++) {
						set = mTextEngine->GetStyleSet(inRange, i);
						if (find(uniqueSets.begin(), uniqueSets.end(), set) == uniqueSets.end())
							uniqueSets.push_back(set);
					}
				}
			}
			
			vector<LStyleSet*>	modifiedSets;
			try {
				for (i = 0; i < uniqueSets.size(); i++) {
					LStyleSet	*testSet = new VStyleSet(uniqueSets[i]);
					StSharer	lock(testSet);
					if (inOperator == keyAEModifyToValue) {
						StAEDescriptor	bogusReply;
						testSet->SetAEProperty(inProperty, value, bogusReply);
					} else if (inOperator == keyAEModifyByValue) {
						testSet->ModifyProperty(inProperty, inValue);
					} else {
						Throw_(errAEEventNotHandled);
					}
					vector<LStyleSet*>::iterator	ip = find_if(mStyleList.begin(), mStyleList.end(), setEquivF<LStyleSet>(*testSet));
					
					if (ip != mStyleList.end()) {
						testSet = *ip;
					} else {
						AddStyle(testSet);
					}
					modifiedSets.push_back(testSet);
					testSet->AddUser(&modifiedSets);
				}
				
				TextRangeT	scanRange = inRange,
							range = inRange;
				range.Crop(mTextEngine->GetTotalRange());
				scanRange = range;
				if (doPendingStyle) {
					Assert_(modifiedSets.size() == 1);
					Assert_(!scanRange.Length());
					if (handler) {
						if (mTextSelection)
							mTextSelection->SetPendingStyle(modifiedSets[0]);
						handler->FollowStyleScript();
					}
				} else {
					scanRange.Front();
					for (i = 1; i <= count; i++) {
						set = mTextEngine->GetStyleSet(scanRange);
						Int32	uniqueIndex = find(uniqueSets.begin(), uniqueSets.end(), set) - uniqueSets.begin();
						mTextEngine->GetStyleSetRange(&scanRange);
						scanRange.Crop(range);
						mTextEngine->SetStyleSet(scanRange, modifiedSets[uniqueIndex]);
						scanRange.After();
					}
					if (handler) {
						handler->FollowStyleScript();
					}
				}
					
				for_each(modifiedSets.begin(), modifiedSets.end(), replaceSharableF<LStyleSet*>(NULL, &modifiedSets));
			} catch (ExceptionCode inErr) {
				for_each(modifiedSets.begin(), modifiedSets.end(), replaceSharableF<LStyleSet*>(NULL, &modifiedSets));
				Throw_(inErr);
			} catch (...) {
				for_each(modifiedSets.begin(), modifiedSets.end(), replaceSharableF<LStyleSet*>(NULL, &modifiedSets));
				throw;
			}
			break;
		}
			
		default:
			break;
	}
}	

//	----------------------------------------------------------------------

void	VOTextModel::ListenToMessage(MessageT inMessage, void *ioParam)
{
	switch (inMessage) {

		case msg_ModelWidthChanged:
		{
			try {
				//	Adjust the rulers with no parent (the others will
				//	get affected indirectly)
				vector<LRulerSet*>::iterator
							ib = mRulerList.begin(),
							ie = mRulerList.end(),
							ip;
				
				for (ip = ib; ip != ie; ip++) {
					if (!(*ip)->GetParent()) {
						(*ip)->Changed();
					}
				}
				
			} catch (...) {
				Assert_(false);
			}
			//	fall through
		}
		
		case msg_CumulativeRangeChanged:
			inherited::ListenToMessage(inMessage, ioParam);
			PurgeStyles();
			PurgeRulers();
			break;
			
		default:
			inherited::ListenToMessage(inMessage, ioParam);
			break;
	}
}

LStyleSet *	VOTextModel::StyleSetForScript(
	const TextRangeT	&inRange,
	ScriptCode			inScript)
{
/*
	Searches backwards from inRange for first script matching styleset
*/
	LStyleSet	*rval = NULL,
				*set = NULL;
	
	if (mTextEngine) {
		TextRangeT	totRange = mTextEngine->GetTotalRange(),
					range;
		
		//	backward
		range = inRange;
		range.Crop(totRange);
		range.Front();
		while (!rval) {
			set = mTextEngine->GetStyleSet(range);
			mTextEngine->GetStyleSetRange(&range);
			
			if (set->GetScript() == inScript) {
				rval = set;
				break;
			}
			range.Before();
			if (range.Start() == totRange.Start())
				break;
		}
		
		//	forward
		range = inRange;
		range.Crop(totRange);
		range.Back();
		while (!rval) {
			set = mTextEngine->GetStyleSet(range);
			mTextEngine->GetStyleSetRange(&range);
			
			if (set->GetScript() == inScript) {
				rval = set;
				break;
			}
			range.After();
			if (range.Start() == totRange.End())
				break;
		}
		
		if (!rval) {
			set = mTextEngine->GetStyleSet(inRange);

			Assert_(set->GetScript() != inScript);

			Int16 			fontNo = GetScriptVariable(inScript, smScriptAppFond);
			Str255			fontName;
			UFontsStyles::GetFontName(fontNo, fontName);
			StAEDescriptor	font(typeChar, fontName + 1, fontName[0]),
							bogusReply;

			rval = new VStyleSet(set);
			StSharer	lock(rval);
			rval->SetAEProperty(pFont, font, bogusReply);
			AddStyle(rval);
		}
	}
	
	return rval;
}

//	----------------------------------------------------------------------
//	Styles

EVTextSetBehavior	VOTextModel::GetStyleListBehavior(void) const
{
	return mStyleListBehavior;
}

void	VOTextModel::SetStyleListBehavior(EVTextSetBehavior inBehavior)
{
	mStyleListBehavior = inBehavior;
}

void	VOTextModel::AddStyle(const LStyleSet *inStyle)
{
	LStyleSet	*style = (LStyleSet *)inStyle;	//	following funcs can be considered const
	
	if (mStyleList.end() == find(mStyleList.begin(), mStyleList.end(), style)) {
		Assert_(!style->GetSuperModel() || (style->GetSuperModel() == this));
		
		style->AddUser(this);
		style->SetSuperModel(this);
		mStyleList.push_back(style);
	} else {
		//	don't add it twice!
	}
}

void	VOTextModel::RemoveStyle(const LStyleSet *inStyle)
{
	LStyleSet						*style = (LStyleSet *)inStyle;	//	following funcs can be considered const
	vector<LStyleSet*>::iterator	ip = find(mStyleList.begin(), mStyleList.end(), style);

	if (ip != mStyleList.end()) {
		if (style->GetUseCount() > 1)
			Throw_(paramErr);	//	can't remove from list if something else still uses it

		
		mStyleList.erase(ip);
		style->RemoveUser(this);
	} else {
		//	it is already absent from style list
	}
}

void	VOTextModel::StyleChanged(const LStyleSet *inStyle)
{
	mTextEngine->StyleSetChanged((LStyleSet*)inStyle);
}

void	VOTextModel::PurgeStyles(void)
{
	switch (mStyleListBehavior) {

		case setBehavior_FreeForm:
		case setBehavior_FreeSheet:
		{
			vector<LStyleSet*>::iterator	ip = mStyleList.begin();
			
			while (ip != mStyleList.end()) {
				if ((*ip)->GetUseCount() == 1) {
					(*ip)->RemoveUser(this);
					mStyleList.erase(ip);
				} else {
					ip++;
				}
			}
		}
	}
}

void	VOTextModel::ApplyStylesFrom(
	FlavorType 			inFlavor,
	const TextRangeT	&inRange,
	LDataTube			*inTube)
{
	if (!inRange.Length())
		return;
	
	Assert_(mTextEngine->GetTotalRange().WeakContains(inRange));
		
	switch (inFlavor) {
		case typeScrapStyles:
		{
			if (!(GetStyleListBehavior() & setBehavior_FreeForm))
				Throw_(paramErr);
			
			vector<LStyleSet*>::iterator
								ip;
			TextRangeT			scanRange = inRange;
			LStyleSet			*testStyle = NULL;
			StSharer			lock(testStyle);	//	avoid leaking in exceptional circumstances
			const LStyleSet		*style;
			Int32				offset;
			Int16				count;
			LStream				&stream = inTube->GetFlavorStreamOpen(typeScrapStyles);
			
			stream >> count;	//	ignored
			stream >> offset;

			scanRange.Front();
			while (
				(stream.GetMarker() < stream.GetLength()) &&
				(scanRange.Start() < inRange.End())
			) {
				if (!testStyle) {
					testStyle = new VStyleSet();
					lock.SetSharable(testStyle);
				}
				
				testStyle->ReadFlavorRecord(typeScrapStyles, stream);

				//	find equivalent style
				style = NULL;
				ip = find_if(mStyleList.begin(), mStyleList.end(), setEquivF<LStyleSet>(*testStyle));
				if (ip != mStyleList.end())
					style = *ip;

				if (!style) {
					AddStyle(testStyle);
					style = testStyle;
					testStyle = NULL;
					lock.SetSharable(testStyle);
				}

				if (stream.GetMarker() < stream.GetLength())
					stream >> offset;
				else
					offset = inRange.Length();
				scanRange.SetEnd(inRange.Start() + offset);
				
				scanRange.Crop(inRange);
				if (scanRange.Length())
					mTextEngine->SetStyleSet(scanRange, style);
				scanRange.After();
			}
			Assert_(scanRange.End() == inRange.End());

			inTube->GetFlavorStreamClose();
			break;
		}
		
		case typeVStyleSets:
		{
			vector<LStyleSet*>				lookup;
			vector<LStyleSet*>::iterator	ip = mStyleList.end();
			LStyleSet						*style = NULL;
			
			//	load lookup table
			{
				LStyleSet	*testStyle = NULL;
				StSharer	lock(testStyle);
				LStream		&stream = inTube->GetFlavorStreamOpen(typeVStyleSets);
				
				while (stream.GetMarker() < stream.GetLength()) {
					style = NULL;
					
					if (!testStyle) {
						testStyle = new VStyleSet();
						lock.SetSharable(testStyle);
					}
					
					testStyle->ReadFlavorRecord(typeVStyleSets, stream);
					
					switch (GetStyleListBehavior()) {
						
						case setBehavior_Sheet:
							ip = find_if(mStyleList.begin(), mStyleList.end(), nameEquivF<LStyleSet>(*testStyle));
							if (ip != mStyleList.end())
								style = *ip;
							break;
							
						case setBehavior_FreeForm:
							ip = find_if(mStyleList.begin(), mStyleList.end(), setEquivF<LStyleSet>(*testStyle));
							if (ip != mStyleList.end())
								style = *ip;

							if (!style) {
								AddStyle(testStyle);
								style = testStyle;
								testStyle = NULL;
								lock.SetSharable(testStyle);
							}
							break;
								
						case setBehavior_FreeSheet:
						{
							nameEquivF<LStyleSet>	f(*testStyle);
							
							ip = find_if(mStyleList.begin(), mStyleList.end(), f);
//							ip = find_if(mStyleList.begin(), mStyleList.end(), nameEquivF<LStyleSet>(*testStyle));
							if (ip != mStyleList.end())
								style = *ip;

							if (!style) {
								AddStyle(testStyle);
								style = testStyle;
								testStyle = NULL;
								lock.SetSharable(testStyle);
							} else if (*style != *testStyle) {
/*+								testStyle->SetParent(style);
								fix test style name
								AddStyle(testStyle);
								style = testStyle;
								testStyle = NULL;
								lock.SetSharable(testStyle);
*/
style = NULL;
							}
							break;
						}
					}
					Assert_(style);
					lookup.push_back(style);
				}
				
				inTube->GetFlavorStreamClose();
			}
			
			//	apply set runs
			{
//-.				StRecalculator	recalc(mTextEngine);
				TextRangeT		scanRange = inRange;
				LStream			&stream = inTube->GetFlavorStreamOpen(typeVStyleSetRuns);
				Int32			length,
								index;
				
				scanRange.Front();
				
				while (
					(stream.GetMarker() < stream.GetLength()) &&
					(scanRange.Start() < inRange.End())
				) {
					stream >> length;
					stream >> index;
					
					ThrowIf_(length <= 0);
					ThrowIf_(index >= lookup.size());
					
					scanRange.SetLength(length);
					scanRange.Crop(inRange);
					style = lookup[index];
					if (style && scanRange.Length())
						mTextEngine->SetStyleSet(scanRange, lookup[index]);
					scanRange.After();
				}
				Assert_(scanRange.End() == inRange.End());

				inTube->GetFlavorStreamClose();
			}
			break;
		}

		default:
			Assert_(false);
			break;
	}
}

//	----------------------------------------------------------------------
//	Rulers

EVTextSetBehavior	VOTextModel::GetRulerListBehavior(void) const
{
	return mRulerListBehavior;
}

void	VOTextModel::SetRulerListBehavior(EVTextSetBehavior inBehavior)
{
	mRulerListBehavior = inBehavior;
}

void	VOTextModel::AddRuler(const LRulerSet *inRuler)
{
	LRulerSet	*ruler = (LRulerSet *)inRuler;	//	following funcs can be considered const
	
	if (mRulerList.end() == find(mRulerList.begin(), mRulerList.end(), ruler)) {
		Assert_(!ruler->GetSuperModel() || (ruler->GetSuperModel() == this));
		
		ruler->AddUser(this);
		ruler->SetSuperModel(this);
		mRulerList.push_back(ruler);
	} else {
		//	don't add it twice!
	}
}

void	VOTextModel::RemoveRuler(const LRulerSet *inRuler)
{
	LRulerSet						*ruler = (LRulerSet *)inRuler;	//	following funcs can be considered const
	vector<LRulerSet*>::iterator	ip = find(mRulerList.begin(), mRulerList.end(), ruler);

	if (ip != mRulerList.end()) {
		if (ruler->GetUseCount() > 1)
			Throw_(paramErr);	//	can't remove from list if something else still uses it

		
		mRulerList.erase(ip);
		ruler->RemoveUser(this);
	} else {
		//	it is already absent from ruler list
	}
}

void	VOTextModel::RulerChanged(const LRulerSet *inRuler)
{
	if (mTextEngine)
		mTextEngine->RulerSetChanged((LRulerSet*)inRuler);
	
	vector<LRulerSet*>::iterator
				ib = mRulerList.begin(),
				ie = mRulerList.end(),
				ip;
	
	for (ip = ib; ip != ie; ip++) {
		if ((*ip)->GetParent() == inRuler) {
			(*ip)->Changed();
		}
	}
}

void	VOTextModel::PurgeRulers(void)
{
	switch (mRulerListBehavior) {

		case setBehavior_FreeForm:
		case setBehavior_FreeSheet:
		{
			vector<LRulerSet*>::iterator	ip = mRulerList.begin();
			
			while (ip != mRulerList.end()) {
				if ((*ip)->GetUseCount() == 1) {
					(*ip)->RemoveUser(this);
					mRulerList.erase(ip);
				} else {
					ip++;
				}
			}
		}
	}
}

void	VOTextModel::ApplyRulersFrom(
	FlavorType 			inFlavor,
	const TextRangeT	&inRange,
	LDataTube			*inTube)
{
	if (!inRange.Length())
		return;
	
	Assert_(mTextEngine->GetTotalRange().WeakContains(inRange));

#if	0		
	switch (inFlavor) {
		case typeScrapRulers:
		{
			if (!(GetRulerListBehavior() & setBehavior_FreeForm))
				Throw_(paramErr);
			
			TextRangeT			scanRange = inRange;
			LRulerSet			*testRuler = NULL;
			StSharer			lock(testRuler);	//	avoid leaking in exceptional circumstances
			const LRulerSet		*ruler;
			Int32				offset;
			Int16				count;
			LStream				&stream = inTube->GetFlavorStreamOpen(typeScrapRulers);
			
			stream >> count;	//	ignored
			stream >> offset;

			scanRange.Front();
			while (
				(stream.GetMarker() < stream.GetLength()) &&
				(scanRange.Start() < inRange.End())
			) {
				if (!testRuler) {
					testRuler = new VRulerSet();
					lock.SetSharable(testRuler);
				}
				
				testRuler->ReadFlavorRecord(typeScrapRulers, stream);
				ruler = FindEquivalentRuler(testRuler);
				if (!ruler) {
					AddRuler(testRuler);
					ruler = testRuler;
					testRuler = NULL;
				}

				if (stream.GetMarker() < stream.GetLength())
					stream >> offset;
				else
					offset = inRange.Length();
				scanRange.SetEnd(inRange.Start() + offset);
				
				scanRange.Crop(inRange);
				if (scanRange.Length())
					mTextEngine->SetRulerSet(scanRange, ruler);
				scanRange.After();
			}
			Assert_(scanRange.End() == inRange.End());

			inTube->GetFlavorStreamClose();
			break;
		}
		
		case typeVRulerSets:
		{
			vector<LRulerSet*>	lookup;
			LRulerSet			*ruler = NULL;
			
			//	load lookup table
			{
				LRulerSet	*testRuler = NULL;
				StSharer	lock(testRuler);
				LStream		&stream = inTube->GetFlavorStreamOpen(typeVRulerSets);
				
				while (stream.GetMarker() < stream.GetLength()) {
					ruler = NULL;
					
					if (!testRuler) {
						testRuler = new VRulerSet();
						lock.SetSharable(testRuler);
					}
					
					testRuler->ReadFlavorRecord(typeVRulerSets, stream);
					
#if	0
					switch (GetRulerListBehavior()) {

							ruler = *find_if(mRulerList.begin(), mRulerList.end(), namematch???);
							break;
							
						case setBehavior_FreeForm:
							ruler = *find_if(mRulerList.begin(), mRulerList.end(), attrmatch???);
							if (!ruler) {
								AddRuler(testRuler);
								ruler = testRuler;
								testRuler = NULL;
							}
							break;
								
						case setBehavior_FreeSheet:
							ruler = *find_if(mRulerList.begin(), mRulerList.end(), namematch???);
							if (!ruler) {
								AddRuler(testRuler);
								ruler = testRuler;
								testRuler = NULL;
							} else if (*ruler != *testRuler) {
/*+								testRuler->SetParent(ruler);
								fix test ruler name
								AddRuler(testRuler);
								ruler = testRuler;
								testRuler = NULL;
*/
ruler = NULL;
							}
							break;
					}
#endif

					lookup.push_back(ruler);
				}
				
				inTube->GetFlavorStreamClose();
			}
			
			//	apply set runs
			{
				TextRangeT	scanRange = inRange;
				LStream		&stream = inTube->GetFlavorStreamOpen(typeVRulerSetRuns);
				Int32		length,
							index;
				
				scanRange.Front();
				
				while (
					(stream.GetMarker() < stream.GetLength()) &&
					(scanRange.Start() < inRange.End())
				) {
					stream >> length;
					stream >> index;
					
					ThrowIf_(length <= 0);
					ThrowIf_(index >= lookup.size());
					
					scanRange.SetLength(length);
					scanRange.Crop(inRange);
					mTextEngine->SetRulerSet(scanRange, lookup[index]);
					scanRange.After();
				}
				Assert_(scanRange.End() == inRange.End());

				inTube->GetFlavorStreamClose();
			}
			break;
		}

		default:
			Assert_(false);
			break;
	}
#endif
}


