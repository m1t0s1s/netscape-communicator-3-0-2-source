// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

#include "Textension.h"
#include "TextensionCommon.h"
#include "RunObject.h"
#include "QDTextRun.h"
#include "RunsRanges.h"
#include "StyledText.h"
#include "Formatter.h"
#include "Line.h"
#include "Paragraph.h"


#include "CPageGeometry.h"

//#include "TextensionIOSuite.h"

//***************************************************************************************************


CRegisteredObjects gRegisteredRuns;

#ifdef txtnRulers
CRegisteredObjects gRegisteredRulers;
#endif

unsigned char gGraphicsRunAssocChar = '?';
//¥¥should NOT be in the range of control chars, otherwise the formatter is confused

//***************************************************************************************************


short gNStyledLineBreakBug; //used in NStyledLineBreak.a

#ifdef txtnDebug
static char gInitialized = 0;
#endif

//***************************************************************************************************


CRunObject* CTextension::fDefaultRun = nil;


void CRegisteredObjects::Free()
{
	register CAttrObject** objPtr = &fTable[0];
	register CAttrObject** maxPtr = &fTable[fCount];
	
	while (objPtr < maxPtr) {(*objPtr++)->Free();}
}
//************************************************************************************************

void CRegisteredObjects::Add(CAttrObject* theObject)
{
	#ifdef txtnDebug
	if (fCount == kMaxRegisteredObjects) {SignalPStr_("\p no more room to register!!");}
	#endif
	
	fTable[fCount++] = theObject;
}
//************************************************************************************************

CAttrObject* CRegisteredObjects::Search(ClassId objClassId)
{
	register CAttrObject** objPtr = &fTable[0];
	register CAttrObject** maxPtr = &fTable[fCount];
	
	while (objPtr < maxPtr) {
		CAttrObject* theObj = *objPtr;
		if (theObj->GetClassId() == objClassId)
			return theObj;
		
		++objPtr;
	}
	
	return nil;
}
//************************************************************************************************

CAttrObject* CRegisteredObjects::GetIndObject(short index) const
{
	return fTable[index];
}
//************************************************************************************************


//CTextensionScrap* CTextension::fLocalScrap = nil;
//***************************************************************************************************



//***************************************************************************************************

OSErr CTextension::TextensionStart(void) //static
{
	#ifdef txtnDebug
	gInitialized = 1;
	#endif
	
	gMinAppMemory = 32*1024;
	
	SysEnvRec sysEnv;
	SysEnvirons(curSysEnvVers, &sysEnv);
	
	Assert_(sysEnv.systemVersion >= 0x0700);
	
	gHasColor = sysEnv.hasColorQD;
	
	gDefaultTabVal = 30;
	
	OSErr err = InitObjects();
	
	if (err)
		return err;
		
	short countScripts = 0;
	g2Directions = false;
	g2Bytes = false;
	
	register short scriptNo = smUninterp;
	while (--scriptNo >= 0)
		{
		if (GetScriptVariable(scriptNo, smScriptEnabled))
			{
			++countScripts;
			
			if (GetScriptVariable(scriptNo, smScriptRight))
				g2Directions = true;

			if (IsScript2Bytes(scriptNo))
				g2Bytes = true;
			}
		}
	
	gHasManyScripts = countScripts > 1;
	
	if ((sysEnv.systemVersion <= 0x0701) && ((GetScriptVariable(smHebrew, smScriptEnabled)) || (GetScriptVariable(smArabic, smScriptEnabled))))
		{gNStyledLineBreakBug = 1;}
	else {gNStyledLineBreakBug = 0;}
	/*¥¥ gNStyledLineBreakBug is used in NStyledLineBreak: in AIS and HIS 7.0.1 there was a bug which makes NPixel2Char crashes if the 
	pixRemain VAR param is passed as non nil value. This affects the performance of NStyledLineBreak since in this case it has to call
	StdTxMeas unnecessairly. NStyledLineBreak will check for this var, if != 0 it will pass nil as pixRemain*/

	gTextensionImaging = false;
	
	gRegisteredRuns.IRegisteredObjects();
	
	#ifdef txtnRulers
	gRegisteredRulers.IRegisteredObjects();
	#endif
	
	return err;
}
//***************************************************************************************************




void CTextension::RegisterRun(CRunObject* theObject) //static
{
	Assert_(theObject);
	
	gRegisteredRuns.Add(theObject);
}
//***************************************************************************************************

#ifdef txtnRulers
void CTextension::RegisterRuler(CRulerObject* theObject) //static
{
	Assert_(theObject);

	gRegisteredRulers.Add(theObject);
}
//***************************************************************************************************
#endif

CRunObject* CTextension::GetNewRunObject() //static
{
// FIX ME!!! figure out when this gets called since we nuked the scrap
//	Assert_(false);
	Assert_(gRegisteredRuns.GetCountObjects());
	
	return (CRunObject*)(CTextension::GetDefaultObject(kRunsIOSuiteType));
}
//***************************************************************************************************

#ifdef txtnRulers
CRulerObject* CTextension::GetNewRulerObject() //static
{
	Assert_(gRegisteredRulers.GetCountObjects());
	
	return (CRulerObject*)(CTextensionScrap::GetDefaultObject(kRulersIOSuiteType));
}
//***************************************************************************************************
#endif

void CTextension::TextensionTerminate() //static
{
	if (fDefaultRun)
		fDefaultRun->Free();
	
	gRegisteredRuns.Free();
	
	#ifdef txtnRulers
	gRegisteredRulers.Free();
	#endif
		
	EndObjects();
}
//***************************************************************************************************

CTextension::CTextension()
	:	CStyledText(NULL, GetNewRunObject(), kLargeSize)
{
	mDrawVisLevel = 0;
	mGeometry = NULL;
	mRenderer = NULL;
	mLineHeights = NULL;
	mFormatter = NULL;

	#ifdef txtnRulers
	mRulerRanges = NULL;
	#endif
	
	mPendingRun = NULL;
}

//***************************************************************************************************

void CTextension::ITextension(GrafPtr textPort, TSize sizeInfo)
{
	try
		{
		mPendingRun = GetNewRunObject();

		mLineHeights = new CLineHeights;
		mGeometry = new CPageGeometry(mLineHeights);
		mFormatter = new CFormatter(this, mGeometry, mLineHeights);
		mRenderer = new CTextRenderer(mGeometry, this, mFormatter, mLineHeights);

		#ifdef txtnRulers
			fRulersRanges = new CRulersRanges;
			fRulersRanges->IRulersRanges(fTextChars, this->GetNewRulerObject());
		#endif

		#ifdef txtnDebug
			if (!gInitialized)
				{
				SignalPStr_("\p ¥TextensionStart has not been called");
				throw (ExceptionCode)paramErr;
				}
			
			if (!gRegisteredRuns.GetCountObjects())
				{
				SignalPStr_("\p ¥RegisterRun has not been called");
				throw (ExceptionCode)paramErr;
				}
		
			#ifdef txtnRulers
				if (!gRegisteredRulers.GetCountObjects())
					{
					SignalPStr_("\p ¥RegisterRuler has not been called");
					throw (ExceptionCode)paramErr;
					}
			#endif
		#endif
			
		if (fDefaultRun)
			{
			mPendingRun->SetDefaults(GetScriptManagerVariable(smSysScript));
			mPendingRun->Update(fDefaultRun, kForceScriptModif);
			}
		}
	catch (ExceptionCode inErr)
		{
		Free();
		throw inErr;
		}
}





//***************************************************************************************************

void CTextension::Free()
{
	mPendingRun->Free();
	
	delete mLineHeights;
	delete mGeometry;
	delete mFormatter;
	delete mRenderer;
	
	#ifdef txtnRulers
	fRulersRanges->Free();
	delete fRulersRanges;
	#endif
	

//	CStyledText::Free();
}
//***************************************************************************************************


short CTextension::DisplayChanged(const CDisplayChanges& displayChanges)
{
	register short changesAction = displayChanges.GetChangesKind();
	
	if (changesAction == kNoChanges)
		return kNoChanges;
	
	if (changesAction & kNeedHReformat)
		{
		this->Format();
		return kRedrawAll + kCheckSize;
		}
	
	mRenderer->Invalidate();
	
	if (changesAction & kNeedVReformat)
		{
//		mLineHeights->Format();
		return kRedrawAll + kCheckSize;
		}
	
//	if (changesAction & kCheckSize) //marges change for instance
//		mRenderer->CheckScroll();
	
	if (changesAction & kNeedPartialReformat)
		{
		TOffsetPair formatRange;
		displayChanges.GetFormatRange(&formatRange);
		
		TEditInfo editInfo;
		editInfo.checkUpdateRgn = true;
		//in linked frames caller may invalid some frames before calling (after moving, resizing, adding, deleting frames)
		
		this->BeginEdit(&editInfo);
		
		changesAction -= kNeedPartialReformat; 
		
		mFormatter->Format(mFormatter->GetLineStart(formatRange.firstOffset)
											, mFormatter->GetLineEnd(formatRange.lastOffset)
											, &formatRange.firstOffset, &formatRange.lastOffset);
		
		this->EndEdit(editInfo, formatRange.firstOffset, formatRange.lastOffset);
		
		}
	
	return changesAction;
}
//************************************************************************************************

OSErr CTextension::Format(Boolean temporary/*=false*/, long firstChar /*=-1*/, long count /*=-1*/)
{
	OSErr err;
	if (firstChar < 0)
		{
		err = mFormatter->Format();
		
		if (!err)
			{
//			if (!temporary)
//				mRenderer->CheckScroll(false/*doUpdate*/);
		
			mRenderer->Invalidate();
			}
		}
	else
		{
		TEditInfo editInfo;
		this->BeginEdit(&editInfo);
	
		long firstFormatLine, lastFormatLine;
		err = mFormatter->Format(firstChar, firstChar+count, &firstFormatLine, &lastFormatLine);
	
		this->EndEdit(editInfo, firstFormatLine, lastFormatLine);
		}
	
	return err;
}
//***************************************************************************************************


void CTextension::Draw(
	LView* 				inViewContext,
	const Rect& 		updateRect,
	TDrawFlags 			drawFlags)
{
	if (mDrawVisLevel == 0)
		mRenderer->Draw(inViewContext, updateRect, drawFlags);
}

void CTextension::DisableDrawing(void)
{
	if (--mDrawVisLevel < 0)
		mDrawVisLevel = 0;
}

void CTextension::EnableDrawing(void)
{
	++mDrawVisLevel;
}


void CTextension::SetDirection(char newDirection, CDisplayChanges* displayChanges)
{
	mRenderer->SetDirection(newDirection);
	
	#ifdef txtnRulers
	if (!this->CountCharsBytes())
		fRulersRanges->SetDefaultRuler((newDirection == kLR) ? kLeftJust : kRightJust);
	#endif
	
	#ifdef txtnRulers
	mFormatter->SetDirection(newDirection);
	
	displayChanges->SetChangesKind(kNeedHReformat);
	#else
	displayChanges->SetChangesKind(kRedrawAll);
	#endif
}
//***************************************************************************************************


void CTextension::SetCRFlag(Boolean newFlag, CDisplayChanges* displayChanges)
{
	mFormatter->SetCRFlag(newFlag);

	displayChanges->SetChangesKind(kNeedHReformat);
}

//***************************************************************************************************

void CTextension::BeginEdit(TEditInfo* editInfo)
{
	mRenderer->PurgeLineCache();
}

//***************************************************************************************************

void CTextension::EndEdit(const TEditInfo& editInfo, long firstFormatLine, long lastFormatLine
											, TOffset* selOffset/*=nil*/, Boolean updateKeyScript/*=true*/)
{
	#ifdef txtnDebug
	long count = this->CountCharsBytes();
	
	mFormatter->CheckCoherence(count);
	mRunRanges->CheckCoherence(count);
	
	#ifdef txtnRulers
	fRulersRanges->CheckCoherence(count);
	#endif
	#endif
	
//	mRenderer->EndEdit(editInfo, firstFormatLine, lastFormatLine, selOffset);
	

	if (selOffset && updateKeyScript)
		this->UpdateKeyboardScript();
}
//***************************************************************************************************


//void CTextension::PointToWord(Point thePt, TOffsetRange* wordRange
//																		, Boolean* outsideFrame, Boolean* outsideLine)
//{
//	this->PointToChar(thePt, wordRange, outsideFrame, outsideLine); //nil if empty line
//	if ((wordRange->Start() < 0) || (!wordRange->IsEmpty()))
//		return;
//	//empty line or double click in a non text run
//	
//	this->CharToWord(wordRange->rangeStart, wordRange);
//}
//***************************************************************************************************

long CTextension::CharToLine(TOffset charOffset, TOffsetRange* lineRange /*= nil*/) const
{
	long lineNo = mFormatter->OffsetToLine(charOffset);
	if (lineRange)
		mFormatter->GetLineRange(lineNo, lineRange);
	
	return lineNo;
}
//***************************************************************************************************

void CTextension::CharToParagraph(long offset, TOffsetRange* paragRange)
{
	unsigned char* charsPtr = this->GetCharPtr(offset);
	
	paragRange->Set(offset - GetParagStartOffset(charsPtr, offset)
									, offset + GetParagEndOffset(charsPtr, mTextChars->CountElements()-offset)
									, false, true);
}

//***************************************************************************************************

//void CTextension::PointToParagraph(Point thePt, TOffsetRange* paragRange
//																	, Boolean* outsideFrame, Boolean* outsideLine)
//{
//	this->PointToChar(thePt, paragRange, outsideFrame, outsideLine);
//	if (paragRange->Start() >= 0)
//		this->CharToParagraph(paragRange->Start(), paragRange);
//}
//***************************************************************************************************


CRunObject* CTextension::IsRangeGraphicsObj(const TOffsetRange& charRange)
{
	CRunObject* theRunObject = NULL;

	if (charRange.Len() == 1)
		{ //only for optimization (to not call CharToRun unless there is a doubt)
		CRunObject* selObject = (CRunObject*) mRunRanges->OffsetToObject(charRange.Start()); //we pass endOfRange as false, should be independent of trailEdge
		
		if (!selObject->IsTextRun())
			theRunObject = selObject;
		}
	
	return theRunObject;
}
//***************************************************************************************************

CRunObject* CTextension::UpdatePendingRun()
//should be called before using mPendingRun since mPendingRun may be invalidated by many members.
{
	register short keyBoardScript = (gHasManyScripts) ? short(GetScriptManagerVariable(smKeyScript)) : 0;
	
	register CRunObject* pending = mPendingRun;
	
	register const CRunObject* nearObj;
	
	TOffsetRange selRange;
	selRange.rangeStart.offset = -1;
	
// FIX ME!!! did we break anything be removing this???
//	if (pending->IsInvalid()) {
//		this->GetSelectionRange(&selRange);
//		
//		nearObj = this->CharToTextRun(selRange.rangeStart);
//		
//		if (nearObj)
//			pending->Assign(nearObj);
//		else {
//			pending->SetDefaults(keyBoardScript);
//
//			if (fDefaultRun && (fDefaultRun->GetRunScript() == keyBoardScript))
//				pending->Update(fDefaultRun);
//
//			return pending; //skip the check on keyboard script
//		}
//	}
	
//	if (pending->GetRunScript() != keyBoardScript) {
//		if (selRange.rangeStart.offset < 0)
//			this->GetSelectionRange(&selRange);
//		
//		nearObj = mRunRanges->CharToScriptRun(selRange.rangeStart, keyBoardScript);
//		
//		if (nearObj) {
//			short theFont, theSize;
//			nearObj->GetAttributeValue(kFontAttr, &theFont);
//			nearObj->GetAttributeValue(kFontSizeAttr, &theSize);
//			
//			pending->SetAttributeValue(kFontAttr, &theFont);
//			pending->SetAttributeValue(kFontSizeAttr, &theSize);
//		}
//		else {
//			pending->SetDefaults(keyBoardScript+kUpdateOnlyScriptInfo);
//			
//			if (fDefaultRun && (fDefaultRun->GetRunScript() == keyBoardScript))
//				pending->Update(fDefaultRun);
//		}
//	}
	
	return pending;
}
//***************************************************************************************************

void CTextension::UpdateKeyboardScript()
{
// FIX ME!!! selection removed, now what???
//	if (gHasManyScripts) {
//		TOffsetRange selRange;
//		this->GetSelectionRange(&selRange);
//	
//		const CRunObject* selStartObj = this->CharToTextRun(selRange.rangeStart);
//		
//		if (selStartObj)
//			selStartObj->SetKeyScript();
//	}
}
//***************************************************************************************************


OSErr CTextension::ReplaceRange(
	long 				rangeStart,
	long 				rangeEnd,
	const TReplaceParams& params,
	Boolean 			invisible,
	TOffsetPair*		ioLineFormatPair)
{
	long firstFormatLine;
	long lastFormatLine;
	TOffset newSel;

	long rangeLen = rangeEnd-rangeStart;
	
	TEditInfo editInfo;
	if (!invisible)
		{
		editInfo.checkUpdateRgn = rangeLen != 0;
		//"BeginEdit" test updateRgn after "SetSelStat" <==> checkUpdateRgn is true (added for movies with shown controller)
		this->BeginEdit(&editInfo);
		}
	
	OSErr err = noErr;
	
	#ifdef txtnRulers
	CAttrObject* rulerObj;
	long extraChars = fRulersRanges->GetReplaceExtraChars(rangeStart, rangeEnd, &rulerObj);
	//¥GetReplaceExtraChars should be called before changing "fTextChars" which is probable in "fIOSuite->Get"
	#else
	long extraChars = 0;
	#endif
	
	Boolean updateKeyScript = true;
	
	long newCharsCount = params.fTextDesc.fTextSize;
	Boolean calcScriptRuns = params.fCalcScriptRuns;
	
	TIOFlags ioSuiteTypes = kNoIOFlags;
	
	#ifdef txtnRulers
	if ((ioSuiteTypes == kNoIOFlags) || (ioSuiteTypes == kIOText+kIORuns) || (ioSuiteTypes == kIOText))
	#else
	if ((ioSuiteTypes == kNoIOFlags) || (ioSuiteTypes == kIOText))
	#endif
	{
		#ifdef txtnRulers
		fRulersRanges->ReplaceRange(rangeStart, rangeLen+extraChars, newCharsCount+extraChars, rulerObj);
		#endif
		
		if (ioSuiteTypes != kIOText+kIORuns) {
			if (params.fNewRuns)
				mRunRanges->ReplaceRange(rangeStart, rangeLen, params.fNewRuns, params.fPlugNewRuns);
			else {
				Boolean takeObjCopy;
				
				CRunObject* replaceObj = params.fReplaceObj;
				
				if (!replaceObj) {
					//¥UpdatePendingRun is made before Replacing text chars since UpdatePendingRun needs a coherent Text and Objects tables
					replaceObj = mRunRanges->CharToTextRun(rangeStart);
					if (!replaceObj) //all runs are non text or no chars ==> let UpdatePendingRun do it
						replaceObj = this->UpdatePendingRun();
					
					takeObjCopy = true;
				}
				else
					takeObjCopy = params.fTakeObjCopy;
				
				mRunRanges->ReplaceRange(rangeStart, rangeLen, newCharsCount, replaceObj, takeObjCopy);
			}
		}
		
		if (ioSuiteTypes == kNoIOFlags) {
			err = mTextChars->ReplaceElements(rangeLen, newCharsCount, rangeStart, params.fTextDesc.fTextPtr);
			
// FIX ME!!! need to rearrange this logic with try/catch
//			nrequire(err, ReplaceChars);
			
//			if (params.fTextDesc.fTextStream) {//fTextDesc.fTextPtr was nil, ReplaceElements just reserved room for "newCharsCount"
//				this->LockChars();
//				params.fTextDesc.fTextStream->ReadBytes(this->GetCharPtr(rangeStart), newCharsCount);
//				this->UnlockChars();
//			}
		}
		
		if (calcScriptRuns)
			this->RunToScriptRuns(rangeStart, newCharsCount, fDefaultRun);
		else
			updateKeyScript = false;
	}
	
	err = mFormatter->ReplaceRange(rangeStart, rangeLen+extraChars, newCharsCount+extraChars
																, &firstFormatLine, &lastFormatLine);
	
	newSel.Set(rangeStart+newCharsCount, params.fTrailEdge);
	if (!invisible)
		this->EndEdit(editInfo, firstFormatLine, lastFormatLine, &newSel, updateKeyScript);
	
	if (ioLineFormatPair != NULL)
		{
		ioLineFormatPair->firstOffset = firstFormatLine;
		ioLineFormatPair->lastOffset = lastFormatLine;
		}
	
//	if (params.fIOSuite)
//		this->Compact();
	
	if (params.fInvalPendingRun)
		mPendingRun->Invalid();
	
	return err;

ReplaceChars:
IOSuiteInput:
	if (!invisible) {
		editInfo.editSuccess = false;
		this->EndEdit(editInfo, 0/*not used*/, 0/*not used*/);
	}
	
	return err;
}
//***************************************************************************************************




long CTextension::ClearKeyDown(unsigned char key, TOffsetRange* rangeToClear)
//rangeToClear->rangeStart is in/output (both "offset" and "trailEdge")
{
	long lenToClear = rangeToClear->Len();
	
	if (!lenToClear) {
		if (key == kBackSpaceCharCode) {
			lenToClear = this->AdvanceOffset(rangeToClear->Start(), false/*forward*/);
			
			if (!lenToClear)
				return 0;
			
			rangeToClear->rangeStart.offset -= lenToClear;
		}
		else
			return 0;
	}
	
	CRunObject* runToDelete;
	long remainLen;
	mRunRanges->GetNextRun(rangeToClear->rangeStart.offset, &runToDelete, &remainLen);
	
	rangeToClear->rangeStart.trailEdge = (lenToClear >= remainLen);
	/*
	the important here is to prevent the caret jumps at direction boundaries: if the deleted chars are 
	at the end of the run ==> trailEdge should be true so that the caret remains at the end the same run.
	Similarly if the chars were at the start of the run ==> trailEdge should be false.
	In all other cases trailEdge doesn't matter since this routine updates the pending run.
	*/
	
	runToDelete->SetKeyScript(); //will do nothing if runToDelete is not a text obj
	
	if (runToDelete->GetClassId() == mPendingRun->GetClassId())
		mPendingRun->Assign(runToDelete);
	/*we do Assign only if run types are equal since UpdatePendingRun assumes mPendingRun has all
	attributes on or all off to decide whether to update it or not (it calls "mPendingRun->IsInvalid").
	assigning 2 different class ids may make mPendingRun to become partially invalid
	*/
	
	return lenToClear;
}
//***************************************************************************************************




void CTextension::Compact()
{
	mTextChars->Compact();
	mFormatter->Compact();
	mRunRanges->Compact();
	#ifdef txtnRulers
	fRulersRanges->Compact();
	#endif
	mRenderer->Compact();
}

//***************************************************************************************************

void CTextension::Activate(Boolean activ, Boolean turnSelOn /*=true*/)
{
	if (activ)
		{
		register CRunObject* pendingRun = mPendingRun;
		
		/*
		update the keyboard script to the pending font if any, this is especially important if one wants
		to implement preferences font, or an empty text having default font (data bases?).
		*/
		if (pendingRun->IsInvalid())
			this->UpdateKeyboardScript();
		else
			pendingRun->SetKeyScript();
		}
	else
		{
		Compact();
		mRenderer->Compact();
		}
}
//***************************************************************************************************



/*
returns a run object which has a valid attribute (a) <===> attribute (a) is continuous over the current selection range
A continuous run attribute over a char range is an attribute which has the same value in all runs in that range.
A valid attribute is an attribute that has been set by calling the CAttrObject member "SetAttributeValue".
You can check if an attribute is valid or not by calling the CAttrObject members "IsAttributeON" or "GetAttributeValue".

"GetContinuousRun" is normally used to set up the menu items related to runs (font, size, face....).

Notes:
	1- You should not modify or free the run object returned by this function.
	2- if the current selection range is empty, the returned object is the pending run object.
		(the pending run object is the object that will be used in subsequent typed chars).
	
the following examples show how you can use "GetContinuousRun":

ex1: set up the font menu
	const CRunObject* continuousRun =  textension->GetContinuousRun();
	short continuousFont;
	if (continuousRun->GetAttributeValue(kFontAttr, &continuousFont)) {
		//check the font menu item corresponding to "continuousFont"
	}
	else {
		//remove the current check mark, if any, from the font menu
	}

ex2: enable the "play sound" menu item <==> the current selected run is a sound run
	const CRunObject* continuousRun =  textension->GetContinuousRun();
	if (continuousRun->GetClassId() == kSoundRunClassId) {
		//enable the play sound menu item
	}
	else {
		//disable the play sound menu item
	}
*/
	
const CRunObject* CTextension::GetContinuousRun(const TOffsetRange& inRange)
{
	CRunObject* theRunObject = NULL;
	
	long selChars = inRange.Len();
	if (selChars > 0)
		{
		CRunObject* theRangeObject = IsRangeGraphicsObj(inRange);
		if (theRangeObject)
			theRunObject = theRangeObject;
		else
			theRunObject = (CRunObject*)(mRunRanges->GetContinuousObj(inRange.Start(), selChars));
		}
	else
		theRunObject = UpdatePendingRun();


	return theRunObject;
}
//***************************************************************************************************

#ifdef txtnRulers
const CRulerObject* CTextension::GetNextRuler(long offset, long* rulerLen) const
{
	CRulerObject* rulerObj;
	fRulersRanges->GetNextRuler(offset, &rulerObj, rulerLen);
	return rulerObj;
}
//***************************************************************************************************
#endif

/*"UpdateRangeRuns" (and UpdateFormatter which is called from it) are not in the InOut seg since
they may be called with each keydown during 2bytes InLine */

OSErr CTextension::UpdateFormatter(long updateFlags, const TOffsetRange& range
																	, long* firstFormatLine, long* lastFormatLine)
{
	long rangeStart = range.Start();
	long rangeEnd = range.End();
	
	if (updateFlags & (kReformat+kVReformat))
		return mFormatter->Format(rangeStart, rangeEnd, firstFormatLine, lastFormatLine);
	else {
		*firstFormatLine = mFormatter->OffsetToLine(rangeStart);
		
		/*the check "rangeStart == rangeEnd" is not only for optimization: if  *firstFormatLine is the 
		last empty line, *lastFormatLine will be calculated as lastLine-1 
		*/
		if (rangeStart == rangeEnd)
			*lastFormatLine = *firstFormatLine;
		else {
			TOffset temp(rangeEnd, true/*endOfRange*/);
			*lastFormatLine = mFormatter->OffsetToLine(temp);
		}
		
		return noErr;
	}
}
//***************************************************************************************************

OSErr CTextension::UpdateRangeRuns(const TOffsetRange& theRange, const AttrObjModifier& modifier
																	, Boolean invisible/*=false*/)
{
	long rangeLen = theRange.Len();

	AttrObjModifier actModifier = modifier; //eventually we'll need to modify the fMessage
	
	if (!rangeLen) {
		this->UpdatePendingRun();
		
		actModifier.fMessage |= kForceScriptModif;
		
		mPendingRun->Update(actModifier, mPendingRun/*continuous*/);
		
		mPendingRun->SetKeyScript();
		/*
		update the keyboard script to the new font if any, this is especially important since
		UpdatePendingRun looks for the keyboard script to decide whether to "default" the pending style
		to the new script, then if a roman font is chosen while the keyboard is non roman,
		UpdatePendingRun (if called later) will change it to the non roman one!, which is required 
		if the user changes the keyboard script.
		*/
		
//		fSelection->Invalid(true/*turnSelOff*/);

		return noErr;
	}
	
	mPendingRun->Invalid();
	
	EventRecord theEvent;
	EventAvail(everyEvent, &theEvent);
	if ((theEvent.modifiers & controlKey) != 0)
		actModifier.fMessage |= kForceScriptModif;
	
	TEditInfo editInfo;
	
	OSErr err = noErr;
	
	if (!invisible)
		this->BeginEdit(&editInfo);
	
	long updateFlags = mRunRanges->UpdateRangeObjects(theRange.Start(), rangeLen, actModifier);
	
	if (!invisible) {
		long firstFormatLine, lastFormatLine;
		err = this->UpdateFormatter(updateFlags, theRange, &firstFormatLine, &lastFormatLine);

		this->EndEdit(editInfo, firstFormatLine, lastFormatLine);
	}
	
	return err;
}
//***************************************************************************************************

#ifdef txtnRulers
OSErr CTextension::UpdateRangeRulers(const TOffsetRange& theRange, const AttrObjModifier& modifier
																		, Boolean invisible/*=false*/)
{
	TOffsetRange effRange = theRange;
	
	fRulersRanges->CharRangeToParagRange(&effRange.rangeStart, &effRange.rangeEnd);
	
	TEditInfo editInfo;
	if (!invisible)
		this->BeginEdit(&editInfo);
	
	long updateFlags = fRulersRanges->UpdateRangeObjects(effRange.Start(), effRange.Len(), modifier);
	
	OSErr err = noErr;
	
	if (!invisible) {
		long firstFormatLine, lastFormatLine;
		err = this->UpdateFormatter(updateFlags, effRange, &firstFormatLine, &lastFormatLine);
	
		this->EndEdit(editInfo, firstFormatLine, lastFormatLine);
	}
	
	return err;
}
//***************************************************************************************************
#endif

CAttrObject* CTextension::GetDefaultObject(unsigned long ioSuiteType) //static
//¥assume that the first registered objects is the default one
{
	#ifdef txtnRulers
	CRegisteredObjects* registered = (ioSuiteType == kRunsIOSuiteType) ? &gRegisteredRuns: &gRegisteredRulers;
	
	return registered->GetIndObject(0)->CreateNew();
	#else
	return gRegisteredRuns.GetIndObject(0)->CreateNew();
	#endif
}



TReplaceParams::TReplaceParams()
{
	//fTextDesc's default constructor will initialize to nil
	
//	fIOSuite = nil;
	
	fReplaceObj = nil;
	
	fNewRuns = nil;
	
	fTrailEdge = true;
	
	fInvalPendingRun = true;
}
//***************************************************************************************************

TReplaceParams::TReplaceParams(const TTextDescriptor& textDesc)
{
	fTextDesc = textDesc;
	
	fCalcScriptRuns = true;
	
//	fIOSuite = nil;
	
	fReplaceObj = nil;
	fTakeObjCopy = false;
	
	fNewRuns = nil;
	
	fTrailEdge = true;
	
	fInvalPendingRun = true;
}
//***************************************************************************************************

TReplaceParams::TReplaceParams(const TTextDescriptor& textDesc, CObjectsRanges* runs, Boolean plugNewRuns)
{
	fTextDesc = textDesc;
	
	fCalcScriptRuns = false;
	
//	fIOSuite = nil;
	
	fReplaceObj = nil;
	fTakeObjCopy = false; //not used
	
	fNewRuns = runs;
	fPlugNewRuns = plugNewRuns;
	
	fTrailEdge = true;
	
	fInvalPendingRun = true;
}
//***************************************************************************************************

TReplaceParams::TReplaceParams(const TTextDescriptor& textDesc
															, CRunObject* replaceObj, Boolean takeObjCopy)
{
	fTextDesc = textDesc;
	fCalcScriptRuns = false; //don't change it : see CTSMTextension::FixActiveInputArea 
	
//	fIOSuite = nil;
	
	fReplaceObj = replaceObj;
	fTakeObjCopy = takeObjCopy;
	
	fNewRuns = nil;
	
	fTrailEdge = true;
	
	fInvalPendingRun = true;
}
//***************************************************************************************************


TReplaceParams::TReplaceParams(CRunObject* runObj)
//non text run obj (pict, sound, ..)
{
	Assert_(!runObj->IsTextRun());
	//fTextDesc's default constructor will initialize to nil
	
	fTextDesc.fTextPtr = &gGraphicsRunAssocChar;
	fTextDesc.fTextSize = sizeof(gGraphicsRunAssocChar);
	
	fCalcScriptRuns = false;
	
//	fIOSuite = nil;
	
	fReplaceObj = runObj;
	fTakeObjCopy = false;
	
	fNewRuns = nil;
	
	fTrailEdge = true;
	
	fInvalPendingRun = true;
}

