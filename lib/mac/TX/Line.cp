// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

#include "Line.h"
#include "RunObject.h"
#include "RunsRanges.h"
#include "Paragraph.h"
#include "QDTextRun.h" //for kFontAttr


//***************************************************************************************************


const short kMaxLineRuns = 100;

CLine::CLine()
{
	fOrderedRuns = NULL;
	fUnorderedRuns = NULL;
}

#ifdef txtnRulers
void CLine::ILine(CStyledText* lineText, char direction, CRulersRanges* rulersRanges, TSize sizeInfo)
#else
void CLine::ILine(CStyledText* lineText, char direction, TSize sizeInfo)
#endif
{
	fStart = -1;
	fDirection = direction;
	
	fStyledText = lineText;
	fRunsRanges = lineText->GetRunsRanges();

	#ifdef txtnRulers
	fRulersRanges = rulersRanges;
	#endif

	// Failure at this point (fully constructed object) will be handled by the caller because the
	// destructor will be called.
	fOrderedRuns = new CArray;
	fOrderedRuns->IArray(sizeof(LineRunInfo), (sizeInfo == kLargeSize) ? 3 : 1);

	fUnorderedRuns = new CArray;
	fUnorderedRuns->IArray(sizeof(LineRunInfo), 3);
}
//***************************************************************************************************

void CLine::Free()
{
	fOrderedRuns->Free();
	delete fOrderedRuns;
	
	fUnorderedRuns->Free();
	delete fUnorderedRuns;
}
//***************************************************************************************************



void CLine::GetRunDisplayInfo(const LineRunInfo* runInfo, LineRunDisplayInfo* runDisplayInfo)
{
	runDisplayInfo->runChars = fStyledText->GetCharPtr(fStart + runInfo->runOffset);
	runDisplayInfo->runLen = runInfo->runLen;
	
	runDisplayInfo->runWidth = runInfo->runWidth;
	runDisplayInfo->runExtraWidth = runInfo->runExtraWidth;

	runDisplayInfo->lineDirection = fDirection;
	
//	runDisplayInfo->zoomNumer = fNumer;
//	runDisplayInfo->zoomDenom = fDenom;

Point foo = { 1, 1 };
	runDisplayInfo->zoomNumer = foo;
	runDisplayInfo->zoomDenom = foo;
}
//***************************************************************************************************

Fixed CLine::RunCharToPixel(const LineRunInfo* runInfo, short offset)
{
	if (!offset || (offset == runInfo->runLen)) {
		/*¥Note that it is important to calc run bounds like this and to not add the "short" result of CharToPixel
		to FixRound(runLeftEdge) since it may differ by 1 pix from the drawn runs (a run is drawn by "MoveTo" to FixRound of all preceding runs)*/
		
		Boolean startBound = (offset == 0);
		if (this->GetRunDirection(*runInfo) == kRL)
			startBound = !startBound;
	
		return (startBound) ? 0 : runInfo->runWidth + runInfo->runExtraWidth;
	}
	
	LineRunDisplayInfo runDisplayInfo;
	this->GetRunDisplayInfo(runInfo, &runDisplayInfo);

	CRunObject* runObj = runInfo->runObj;
	
	if (runInfo->runCtrlChar || (runObj->GetObjFlags() & kIndivisibleRun))
		return IndivisibleCharToPixel(runDisplayInfo, offset);
	else
		return runObj->CharToPixel(runDisplayInfo, offset);
}
//***************************************************************************************************

Fixed CLine::RunMeasure(const LineRunInfo* runInfo)
{
	LineRunDisplayInfo runDisplayInfo;
	this->GetRunDisplayInfo(runInfo, &runDisplayInfo);
	
	return runInfo->runObj->MeasureWidth(runDisplayInfo);
}
//***************************************************************************************************

void CLine::DoLineLayout(long lineStart, short lineLen, short maxLineWidth)
//lineLen == 0 ==> empty line (useful for empty texts, to calc the caret position)

{
	if (fStart == lineStart)
		return;
	
	fMaxWidth = LongToFixed(maxLineWidth);

	//SaveTextEnv, LockChars many called routines (DefineRunWidths, ..) rely on this
	TTextEnv savedTextEnv;
	fStyledText->SetTextEnv(&savedTextEnv);
	
	fStyledText->LockChars();

	fStart = lineStart;
	
	#ifdef txtnRulers
	fLineRuler = (CRulerObject*) fRulersRanges->OffsetToObject(lineStart);
	#endif

	this->DefineRuns(lineStart, lineLen, fUnorderedRuns); //fill gUnorderedRuns

	fLastRun = short(fUnorderedRuns->CountElements()) - 1; //-ive if no Runs
	
	fUnorderedRuns->LockArray(); //assumed in "CalcVisibleLength", "DefineRunWidths" and "OrderRuns"

	short blankRuns = 0;
	
	short visLength = (lineLen) ? this->CalcVisibleLength(fUnorderedRuns, lineLen, &blankRuns) : 0;
	
	fNoDraw = visLength == 0;
	
	#ifdef txtnRulers
	char lineJust;
	fLineRuler->GetAttributeValue(kJustAttr, &lineJust);

	if (lineJust == kForceFullJust) {
		if (visLength)
			lineJust = kFullJust;
		else
			lineJust = (fDirection == kLR) ? kLeftJust : kRightJust;
	}
	else {
		if (lineJust == kFullJust) {
			long lastChar = lineStart+lineLen-1;
			
			if (!visLength || (*fStyledText->GetCharPtr(lastChar) == kCRCharCode) || (lastChar == fStyledText->CountCharsBytes()-1))
				lineJust = (fDirection == kLR) ? kLeftJust : kRightJust;
		}
	}

	if (lineJust == kFullJust) {
		fVisLen = visLength;
		
		if (blankRuns) {
			fLastRun -= blankRuns;
			gUnorderedRuns->SetElementsCount(fLastRun+1);
		}
		
		//fLastRun >= 0 (since visLength > 0)
		LineRunInfo* lastRunInfo = (LineRunInfo*) gUnorderedRuns->GetElementPtr(fLastRun);
		lastRunInfo->runLen = visLength-lastRunInfo->runOffset;
	}
	else
	#endif
	fVisLen = lineLen;
	
	Boolean paragStart;
	
	#ifdef txtnRulers
	paragStart = (lineStart == 0) || (*fStyledText->GetCharPtr(lineStart-1) == kCRCharCode);

	fLeftEdge = fLineRuler->GetLineLeftBlanks(fDirection, paragStart, fNumer.h, fDenom.h);
	
	Fixed effectiveWidth = fMaxWidth
								- fLineRuler->GetLineLeftBlanks(fDirection, paragStart, fNumer.h, fDenom.h)
								- fLineRuler->GetLineRightBlanks(fDirection, paragStart, fNumer.h, fDenom.h);
	//"effectiveWidth" takes into account marges and indent
		
	#else
	paragStart = false; //DefineRunWidths will not use it in this case
	
	Fixed effectiveWidth = fMaxWidth;
	
	fLeftEdge = 0;
	#endif //txtnRulers
	
	Fixed blankWidth = effectiveWidth - this->DefineRunWidths(fUnorderedRuns, effectiveWidth, paragStart);
	/*blankWidth can be -ive if lineJust != kFullJust since in this case the whites are included, but
	this has no vis effect since if just==right or left the extra whites will be drawn outside the line
	pix space
	*/
	
	this->OrderRuns(fUnorderedRuns); //define fOrderedRuns

	
	#ifdef txtnRulers
	if (lineJust == kFullJust)
		this->DefineRunsExtraWidths(blankWidth);
	else {
		if (lineJust == kRightJust)
			fLeftEdge += blankWidth;
		else {
			if (lineJust == kCenterJust)
				fLeftEdge += FixDiv(blankWidth, LongToFixed(2));
		}
	}
	#else
	if (fDirection == kRL)
		fLeftEdge += blankWidth;
	#endif

	fUnorderedRuns->UnlockArray();
	
	fStyledText->UnlockChars();
	fStyledText->RestoreTextEnv(savedTextEnv);
}
//***************************************************************************************************

short CLine::CalcVisibleLength(CArray* theRuns, short lineLen, short* blankRuns)
//assume lineLen != 0, "theRuns" is locked and curr port font is saved
{
	Assert_(lineLen != 0);
	
	register LineRunInfo* firstRunInfo = (LineRunInfo*) theRuns->GetElementPtr(0);
	register LineRunInfo* runInfo = (LineRunInfo*) theRuns->GetLastElementPtr();
	
	::SetSysDirection(fDirection);
	//became necessary with CUBE-E where "VisibleLength" is dependent on sysJust
	
	register short visLen = lineLen;
	
	*blankRuns = 0;
	
	register short visRunLen;
	do  {
		short runFont;
		if (!runInfo->runObj->GetAttributeValue(kFontAttr, &runFont))
			break; //run doesn't have a font (pict, sound..)
		
		register short runLen = runInfo->runLen;
		
		TextFont(runFont);
		visRunLen = short(VisibleLength(Ptr(fStyledText->GetCharPtr(fStart+runInfo->runOffset)), runLen));
		
		visLen -= (runLen-visRunLen);
		
		if (!visRunLen)
			++(*blankRuns);
		
	} while (!visRunLen && (--runInfo >= firstRunInfo));
	
	return visLen;
}
//***************************************************************************************************

void CLine::InsertRun(long runStart, short runLen, CRunObject* runObj, CArray* unorderedRuns)
//runStart is an offset from the whole text
{
	short remainingLen = runLen;
	
	while (remainingLen) {
		long ctrlOffset = gParagCtrlChars.GetCurrCtrlOffset()-runStart; //offset from runStart, -ive if no curr

		LineRunInfo* runInfo = (LineRunInfo*) unorderedRuns->InsertElements(1, nil); //¥¥ neglect error (drawing, clicking...!)
		
		runInfo->runObj = runObj;
		long forceLongEval = runStart-fStart;
		runInfo->runOffset = short(forceLongEval);
		runInfo->runExtraWidth = 0;

		if (ctrlOffset == 0) {
			runInfo->runLen = 1;
			runInfo->runCtrlChar = gParagCtrlChars.GetCurrCtrlChar();
			gParagCtrlChars.Next();
		}
		else {
			if ((ctrlOffset > 0) && (remainingLen > ctrlOffset))
				runInfo->runLen = short(ctrlOffset);
			else
				runInfo->runLen = remainingLen;
				
			runInfo->runCtrlChar = 0;
		}
		
		runStart += runInfo->runLen;
		remainingLen -= runInfo->runLen;
	}
}
//***************************************************************************************************

void CLine::DefineRuns(long lineStart, short lineLen, CArray* unorderedRuns)
{
	unorderedRuns->SetElementsCount(0);
	
	gParagCtrlChars.Define(fStyledText->GetCharPtr(0), lineStart, lineStart+lineLen);
	
	while (lineLen > 0) {
		long runLen;
		CRunObject* runObj;
		fRunsRanges->GetNextRun(lineStart, &runObj, &runLen);
		if (runLen > lineLen)
			runLen = lineLen;
		
		this->InsertRun(lineStart, short(runLen), runObj, unorderedRuns);
		
		lineStart += runLen;
		lineLen -= short(runLen);
	}
}
//***************************************************************************************************

short CLine::GetRunDirection(const LineRunInfo& runInfo)
{
	if (runInfo.runCtrlChar)
		return  fDirection;
	else {
		short theDir = runInfo.runObj->GetRunDir();
		
		if (theDir == kLineDirection)
			theDir = fDirection;
			
		return theDir;
	}
}
//***************************************************************************************************


// #ifdef powerc
enum {
	uppRunDirProcInfo = kPascalStackBased
		 | RESULT_SIZE(SIZE_CODE(sizeof(Boolean)))
		 | STACK_ROUTINE_PARAMETER(1, SIZE_CODE(sizeof(short)))
		 | STACK_ROUTINE_PARAMETER(2, SIZE_CODE(sizeof(long)))
};

// #endif

struct RunDirProcData {
	LineRunInfo* runsInfo;
	CLine* theLine;
};
typedef RunDirProcData* RunDirProcDataPtr;

pascal Boolean RunDirProc(short runIndex, RunDirProcDataPtr dataLink);
pascal Boolean RunDirProc(short runIndex, RunDirProcDataPtr dataLink)
{
	short theDir = dataLink->theLine->GetRunDirection(*(dataLink->runsInfo + runIndex));
	return theDir == kRL;
}
//***************************************************************************************************

#ifndef powerc
void Push_D3()	= 0x2F03;		// MOVE.L	D3, -(A7)
void Pop_D3() 	= 0x261F; 	// MOVE.L	(A7)+, D3
#endif

void CLine::OrderRuns(CArray* unorderedRuns)
{
	short countRuns = fLastRun+1;
	
	if (countRuns <= 0)
		return; //fLastRun is -ive if no runs
	
	#ifdef txtnDebug
	if (countRuns > kMaxLineRuns)
		SignalPStr_("\p line having too much runs, will not be ordered");
	#endif
	
	fOrderedRuns->SetElementsCount(countRuns); //¥¥ neglect error (drawing, clicking...!)
	
	LineRunInfo* unOrderedRunPtr = (LineRunInfo*) unorderedRuns->GetElementPtr(0);
	
	// a line having more than 100 runs will not be ordered!
	if (!g2Directions || (countRuns == 1) || (countRuns > kMaxLineRuns)) {
		fOrderedRuns->SetElementsVal(0, Ptr(unOrderedRunPtr), countRuns);
		return;
	}
	//(countRuns == 1) is also made since GetFormatOrder does nothing if no of runs <= 1
	
	short orderedRunsIndices[kMaxLineRuns];
	
	RunDirProcData dirProcData;
	dirProcData.runsInfo = unOrderedRunPtr;
	dirProcData.theLine = this;

//	#ifdef powerc
	StyleRunDirectionUPP runDirRD = NewStyleRunDirectionProc(RunDirProc);
//	(uppRunDirProcInfo, &RunDirProc);
	
#ifndef	powerc
	Push_D3();
#endif
	GetFormatOrder(FormatOrderPtr(orderedRunsIndices), 0, fLastRun
								 , fDirection == kRL, runDirRD, Ptr(&dirProcData));
#ifndef	powerc
	Pop_D3();
#endif

/*	#else
	Push_D3(); //GetFormatOrder bug in 7.0: Destroy D3!
	GetFormatOrder(FormatOrderPtr(orderedRunsIndices), 0, fLastRun
								 , fDirection == kRL, StyleRunDirectionUPP(&RunDirProc), Ptr(&dirProcData));
	Pop_D3();
	#endif
*/	
	//Copy the runs in their visible order to fOrderedRuns
	LineRunInfo* orderedRunPtr = (LineRunInfo*) fOrderedRuns->GetElementPtr(0);
	short* orderedIndex = orderedRunsIndices;
	
	while (--countRuns >= 0)
		*orderedRunPtr++ = *(unOrderedRunPtr + *orderedIndex++);
}
//***************************************************************************************************

#ifdef txtnRulers
Fixed CLine::CalcAlignTabWidth(TPendingTab* pendingTab, Fixed tabTrailWidth, const LineRunInfo& runAfter)
{
	short alignOffset = SearchChar(pendingTab->theTab.alignChar
																, fStyledText->GetCharPtr(fStart+runAfter.runOffset)
																, runAfter.runLen);
																
	if (alignOffset < 0)
		return 0; //alignChar not found, still pending
	
	pendingTab->valid = false;
	Fixed tabWidth = pendingTab->tabWidth-tabTrailWidth;
	
	if (alignOffset) {
		LineRunInfo runToMeasure = runAfter;
		runToMeasure.runLen = alignOffset;
		tabWidth -= this->RunMeasure(&runToMeasure);
	}
	
	return (tabWidth >= 0) ? tabWidth : 0;
}
//***************************************************************************************************
#endif

Fixed CLine::DefineRunWidths(CArray* unorderedRuns, Fixed maxWidth, Boolean paragStart) // return sum of run widths
//works on the unordered runs (to calculate the tab widths as the formatter did)
{
	if (fLastRun  < 0) return 0;
	
	LineRunInfo* runInfo = (LineRunInfo*) unorderedRuns->GetElementPtr(0);
	
	Fixed sumWidth = 0;
	
	#ifdef txtnRulers
	TPendingTab pendingTab;

	pendingTab.valid = false;
	LineRunInfo* pendingTabRun = nil;
	Fixed tabTrailWidth = 0;
	
	Fixed blanks;
	if (fDirection == kLR)
		blanks = fLineRuler->GetLineLeftBlanks(fDirection, paragStart, fNumer.h, fDenom.h);
	else
		blanks = fLineRuler->GetLineRightBlanks(fDirection, paragStart, fNumer.h, fDenom.h);
	#endif
	
	short lastRun = fLastRun;
	
	for (short ctr = 0; ctr <= lastRun; ++ctr, ++runInfo) {
		if (runInfo->runCtrlChar) {
			if (runInfo->runCtrlChar != kTabCharCode) {
				runInfo->runWidth = 0;
				continue;
			}
			
			#ifdef txtnRulers
			if (pendingTab.valid) {
				pendingTabRun->runWidth = fLineRuler->CalcPendingTabWidth(pendingTab, tabTrailWidth, maxWidth-sumWidth);
				sumWidth += pendingTabRun->runWidth;
			}
			
			runInfo->runWidth = fLineRuler->GetTabWidth(sumWidth+blanks, fDirection, &pendingTab, fNumer.h, fDenom.h);
			
			if (pendingTab.valid) {
				pendingTabRun = runInfo;
				tabTrailWidth = 0;
				runInfo->runWidth = 0;
			}
			
			#else
//			runInfo->runWidth = GetDefaultTabWidth(sumWidth, fNumer.h, fDenom.h);
			runInfo->runWidth = GetDefaultTabWidth(sumWidth, 1, 1);
			#endif
		}
		else {
			runInfo->runObj->SetRunEnv();
			
			runInfo->runWidth = this->RunMeasure(runInfo);
			
			
			#ifdef txtnRulers
			if (pendingTab.valid) {
				if (pendingTab.theTab.kind == kAlignTab) {
					pendingTabRun->runWidth = this->CalcAlignTabWidth(&pendingTab, tabTrailWidth, *runInfo);
					sumWidth += pendingTabRun->runWidth;
				}
					
				tabTrailWidth += runInfo->runWidth;
			}
			#endif
		}
		sumWidth += runInfo->runWidth;
	}

	#ifdef txtnRulers
	if (pendingTab.valid) {//last tab is pending
		pendingTabRun->runWidth = fLineRuler->CalcPendingTabWidth(pendingTab, tabTrailWidth, maxWidth-sumWidth);
		sumWidth += pendingTabRun->runWidth;
	}
	#endif
	
	return sumWidth;
}
//***************************************************************************************************

#ifdef txtnRulers
Fixed CLine::CalcFullJustifPortions(Fixed* portionRunArr, short* cFullJustifRuns)
{
	fOrderedRuns->LockArray();
	
	short lastRun = fLastRun;
	LineRunInfo* runInfo = (LineRunInfo*) fOrderedRuns->GetElementPtr(lastRun);
	
	Fixed sumPortions = 0;
	*cFullJustifRuns = 0;
	
	for (short ctr = 0; ctr <= lastRun; ++ctr, --runInfo) {
		if (runInfo->runCtrlChar) {
			if (runInfo->runCtrlChar == kTabCharCode)
				break;
				
			*portionRunArr++ = 0;
		}
		else {
			CRunObject* run = runInfo->runObj;
			
			++(*cFullJustifRuns);
			
			run->SetRunEnv();

			LineRunDisplayInfo runDisplayInfo;
			this->GetRunDisplayInfo(runInfo, &runDisplayInfo);
			
			*portionRunArr = run->FullJustifPortion(runDisplayInfo);

			sumPortions += *portionRunArr++;
		}
	}
	
	fOrderedRuns->UnlockArray();
	
	return sumPortions;
}
//***************************************************************************************************
#endif //txtnRulers

#ifdef txtnRulers
void CLine::DefineRunsExtraWidths(Fixed theSlope)
{
	if ((theSlope == 0) || (fLastRun >= kMaxLineRuns) || (fLastRun < 0))
		return;
	// a line having more than 100 runs will not be full justified!
	
	if (fLastRun == 0) {
		((LineRunInfo*) fOrderedRuns->GetElementPtr(0))->runExtraWidth = theSlope;
		return;
	}
	
	Fixed portionRun[kMaxLineRuns];
	
	Fixed sumPortions;
	short cFullJustifRuns;
	sumPortions = this->CalcFullJustifPortions(portionRun, &cFullJustifRuns);
	if (sumPortions) { //can be 0 : a line with only numerics with no spaces or the tab constraints doesn't allow for any portion
		Fixed* portionRunPtr = portionRun;
		LineRunInfo* runInfo = (LineRunInfo*) fOrderedRuns->GetElementPtr(fLastRun);
		
		for (short ctr = 0; ctr < cFullJustifRuns; ++ctr, --runInfo)
			runInfo->runExtraWidth = FixMul(FixDiv(*portionRunPtr++, sumPortions), theSlope);
	}
}
//***************************************************************************************************
#endif //txtnRulers

void CLine::Draw(const Rect& lineRect, short lineAscent)
{
	if (fNoDraw)
		return;

	Fixed runLeftEdge = LongToFixed(lineRect.left) + fLeftEdge;
	
	LineRunInfo* runInfo = (LineRunInfo*) fOrderedRuns->LockArray();
	
	short vBase = lineRect.top + lineAscent;
	short lastRun = fLastRun;
	for (short runIndex=0; runIndex <= lastRun; ++runIndex, ++runInfo) {
		if (!runInfo->runCtrlChar) {
			CRunObject* run = runInfo->runObj;
			
			run->SetRunEnv();
		
			MoveTo(FixRound(runLeftEdge), vBase);


			LineRunDisplayInfo runDisplayInfo;
			this->GetRunDisplayInfo(runInfo, &runDisplayInfo);
			
			run->Draw(runDisplayInfo, runLeftEdge, lineRect, lineAscent);
		}
		runLeftEdge += runInfo->runWidth + runInfo->runExtraWidth;
	}
	
	fOrderedRuns->UnlockArray();
}
//***************************************************************************************************


void CLine::SetDirection(char newDirection)
{
	fDirection = newDirection;
	this->Invalid();
}
//***************************************************************************************************

short CLine::CharToRun(TOffset charOffset, Fixed* runLeftEdge/*= nil*/)
/*
¥charOffset.offset is wrt the whole text
¥charOffset.offset may be > Sum (run->runLen) for full justif lines since in this case run->runLen is
	the vis len (without spaces) and the caret may be in the invis spaces (back space at the end of a
	full justif line, for instance)
¥return the runIndex in the fOrderedRuns, -1 if no runs (empty line)
*/
{
	short lastRun = fLastRun;
	
	if (lastRun < 0)
		return -1;
	
	charOffset.offset -=  fStart;
	
	Fixed leftEdge = fLeftEdge;
	
	if (charOffset.trailEdge && (charOffset.offset))
		--charOffset.offset;
	charOffset.offset = MinLong(charOffset.offset, fVisLen-1);
	
	short runNo;
	LineRunInfo* runInfo = (LineRunInfo*) fOrderedRuns->GetElementPtr(0);
	for (runNo = 0; runNo < lastRun; ++runNo, ++runInfo) {
		if ((charOffset.offset >= runInfo->runOffset)
			&& (charOffset.offset < runInfo->runOffset+runInfo->runLen))
			break;
			
		leftEdge += runInfo->runWidth + runInfo->runExtraWidth;
	}
	
	if (runLeftEdge)
		*runLeftEdge = leftEdge;
		
	return runNo;
}
//***************************************************************************************************

short CLine::CharacterToPixel(TOffset charOffset, char caretDir /*= kRunDir*/)
/*
¥charOffset.offset is wrt the whole text
¥ charOffset.offset may be > Sum (run->runLen) (see comment at CharToRun)
*/
{
	Fixed runLeftEdge;
	short charRun = this->CharToRun(charOffset, &runLeftEdge);
	if (charRun < 0)
		return FixRound(fLeftEdge);
	
	fOrderedRuns->LockArray();
	
	LineRunInfo* runInfo = (LineRunInfo*) fOrderedRuns->GetElementPtr(charRun);

	charOffset.offset -=  (fStart+runInfo->runOffset);
	
	if (caretDir != kRunDir) {
		if ((charOffset.offset == runInfo->runLen)
				&& (this->GetRunDirection(*runInfo) != caretDir)
				&& (caretDir == fDirection))
			charOffset.offset = 0;
	}
	
	fStyledText->LockChars();

	TTextEnv savedTextEnv;
	fStyledText->SetTextEnv(&savedTextEnv);

	runInfo->runObj->SetRunEnv();
	
	short result = FixRound(runLeftEdge + this->RunCharToPixel(runInfo, short(charOffset.offset)));
	
	fStyledText->RestoreTextEnv(savedTextEnv);
	fStyledText->UnlockChars();
	
	fOrderedRuns->UnlockArray();
	
	return result;
}
//***************************************************************************************************

short CLine::CalcRunHilite(short startOff, short endOff, short runVisIndex
												, Fixed* startPix, Fixed* pixWidth, Fixed* runLeft, Boolean lineBounds)
/* runVisIndex is index in fOrderedRuns
startOff, endOff are offsets from the start of the line.
endOff > startOff.
startOff-endOff need not be completely in the run char range
runLeft is input-output
Assume fOrderedRuns, fTextChars are Locked, the Style Env is saved
startPix is returned wrt to the start of the line
returns count selected chars */
{
	LineRunInfo* runInfo = (LineRunInfo*) fOrderedRuns->GetElementPtr(runVisIndex);
	Fixed runWidth = runInfo->runWidth + runInfo->runExtraWidth;
	
	Fixed prevRunRight = *runLeft;
	*runLeft += runWidth;
	
	startOff -= runInfo->runOffset;
	endOff -= runInfo->runOffset;
	if ((endOff < 0) || (startOff > runInfo->runLen))
		return 0;
	
	if (startOff < 0)
		startOff = 0;
	if (endOff > runInfo->runLen)
		endOff = runInfo->runLen;
	if (endOff == startOff)
		return 0;
	
	runInfo->runObj->SetRunEnv();
	
	*startPix = prevRunRight;
	
	short runDir = this->GetRunDirection(*runInfo);
	
	if (startOff == 0) {
		if (runDir == kLR) {
			if ((runVisIndex == 0) && lineBounds)
				*startPix = 0;  //else it is already = prevRunRight
		}
		else {
			if ((runVisIndex == fLastRun) && lineBounds)
				*startPix = fMaxWidth;
			else
				*startPix += runWidth;
		}
	}
	else
		*startPix += this->RunCharToPixel(runInfo, startOff);
	
	Fixed endPix = prevRunRight;
	if (endOff == runInfo->runLen) {
		if (runDir == kLR) {
			if ((runVisIndex == fLastRun) && lineBounds)
				endPix = fMaxWidth;
			else
				endPix += runWidth;
		}
		else
			if ((runVisIndex == 0) && lineBounds)
				endPix = 0; //else endPix is already = prevRunRight
	}
	else
		endPix += this->RunCharToPixel(runInfo, endOff);
		
	if (endPix < *startPix)
		SwapLong(startPix, &endPix);
	
	*pixWidth = endPix - *startPix;
	
	return endOff-startOff;
}
//***************************************************************************************************


short CLine::PixelToRun(Fixed* hPix)
/*
¥return the runIndex in the fOrderedRuns
¥hPix is input/output, on exit it contains the remaining pix
*/
{
	*hPix -= fLeftEdge;
	if (*hPix < 0)
		*hPix = 0;
	
	LineRunInfo* runInfo = (LineRunInfo*) fOrderedRuns->GetElementPtr(0);
	
	Fixed runTotalWidth = 0;
	short lastRun = fLastRun;
	for (short runIndex=0; runIndex <= lastRun; ++runIndex, ++runInfo) {
		runTotalWidth = runInfo->runWidth + runInfo->runExtraWidth;
		if (*hPix > runTotalWidth)
			*hPix -= runTotalWidth;
		else
			return runIndex;
	}
	
	//The pix offset is beyond the last vis run	
	*hPix = runTotalWidth;
	return fLastRun;
}
//***************************************************************************************************

CRunObject* CLine::PixelToCharacter(Fixed hPixel, TOffsetRange* charRange)
/* offsets in charRange are wrt to the start of the Text (not start of the line)
-the fn result is the range CRunObject*, nil if empty line
*/
{
	if (fLastRun < 0) { //emptyLine
		charRange->Set(fStart, fStart, false, false);
		return nil;
	}

	short charRun = this->PixelToRun(&hPixel);
	
	fStyledText->LockChars();
	fOrderedRuns->LockArray();
	
	LineRunInfo* runInfo = (LineRunInfo*) fOrderedRuns->GetElementPtr(charRun);
	
	TTextEnv savedTextEnv;
	fStyledText->SetTextEnv(&savedTextEnv);
	
	CRunObject* runObj = runInfo->runObj;
	
	runObj->SetRunEnv();
	
	LineRunDisplayInfo runDisplayInfo;
	this->GetRunDisplayInfo(runInfo, &runDisplayInfo);
	
	if (runInfo->runCtrlChar || (runObj->GetObjFlags() & kIndivisibleRun)) {
		if (runInfo->runCtrlChar == kCRCharCode)
			charRange->Set(0, 0, false, false);
		//trailEdge was set to true (for clicking beyond a cr to follow preceding style, but should be false for double clicking on the cr to work
		else
			IndivisiblePixelToChar(runDisplayInfo, hPixel, charRange);
	}
	else
		runObj->PixelToChar(runDisplayInfo, hPixel, charRange);
	
	charRange->Offset(fStart + runInfo->runOffset);
	
	fStyledText->RestoreTextEnv(savedTextEnv);
	
	fStyledText->UnlockChars();
	fOrderedRuns->UnlockArray();
	
	return runInfo->runObj;
}
//***************************************************************************************************

Boolean CLine::VisibleToBackOrder(TOffset* charOffset, char reqDir)
/*
	define the back storage order for a run with a direction "reqDir" to be inserted at "charOffset"
	(inverse of "GetFormatOrder"?)
¥charOffset is offset from the start of the whole text (input/output)
*/
{
	short charRun = this->CharToRun(*charOffset);
	if (charRun < 0)
		return false;
	//charRun is the visible order of the run containing charOffset
	
	LineRunInfo* runInfo = (LineRunInfo*) fOrderedRuns->GetElementPtr(charRun);
	
	short runDir = this->GetRunDirection(*runInfo);
	
	if (runDir == reqDir)
		return false;
	
	long startOffset = charOffset->offset - fStart - runInfo->runOffset;
	Boolean runStart = (startOffset == 0);
	Boolean runEnd = (startOffset == runInfo->runLen);
	if (!runStart && !runEnd)
		return false;
	
	Boolean boundIsNextVis = (runEnd && (runDir == kLR)) || (runStart && (runDir == kRL));
	//!boundIsNextVis means ((runEnd && (runDir == kRL)) || (runStart && (runDir == kLR))) since either runStart or runEnd is true
	
	short nearRun = (boundIsNextVis) ? ++charRun : --charRun;
	if ((nearRun >= 0) && (nearRun <= fLastRun)) {
		LineRunInfo* nearRunInfo = (LineRunInfo*) fOrderedRuns->GetElementPtr(nearRun);
		if (this->GetRunDirection(*nearRunInfo) == runDir)
			return false;
		
		charOffset->offset = fStart+nearRunInfo->runOffset;
		if (boundIsNextVis && (runDir == kLR) || !boundIsNextVis && (runDir == kRL)) {
			charOffset->offset += nearRunInfo->runLen;
			charOffset->trailEdge = true;
		}
		else
			charOffset->trailEdge = false;
	}
	else {
		if (reqDir != fDirection)
			return false;
		
		charOffset->offset = fStart+runInfo->runOffset;
		if (runStart) {
			charOffset->offset += runInfo->runLen;
			charOffset->trailEdge = true;
		}
		else
			charOffset->trailEdge = false;
	}
	
	return true;
}
//***************************************************************************************************


void CLine::Invalid() {fStart = -1;}

Boolean	 CLine::IsValid(void) const
{
	return (fStart != -1);
}

