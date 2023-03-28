// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

#include "Formatter.h"
#include "TextensionCommon.h"
#include "RunObject.h"
#include "Paragraph.h"
#include "CPageGeometry.h"



const short kDefaultLineHeight = 12;
const short kMaxLineLen = 200;




CFormatter::CFormatter()
{
}


CFormatter::CFormatter(
	CStyledText* 		inStyledText,
	CPageGeometry* 		inGeometry,
	CLineHeights* 		inLineHeights)
{

	mStyledText = inStyledText;
	mGeometry = inGeometry;
	mLineHeights = inLineHeights;
	mLineEnds = NULL;
	
	mRunsRanges = mStyledText->GetRunsRanges();

	//	jah 950130
	fLineHeight = 0;
	fLineAscent = 0;
	fFormatLevel = 0;
	fCR = false;

	#ifdef txtnRulers
	fParagraphRuler = NULL;
	#endif
	
	fLastLineIndex = -1;
	
	#ifdef txtnRulers
	fDirection = GetSysJust();
	#endif

	try
		{
		mLineEnds = new CRanges;
		mLineEnds->IRanges(sizeof(long), 100 /* more */);
		}
	catch (ExceptionCode inErr)
		{
		delete mLineEnds;
		throw inErr;
		}

	#ifdef txtnRulers
	fRulersRanges = rulersRanges;
	#endif
	
	AppendEmptyLine();
}


CFormatter::~CFormatter()
{
	mLineEnds->Free();
	delete mLineEnds;
}

//***************************************************************************************************



void CFormatter::FreeData(Boolean emptyLine /*=true*/)
//made in a resident seg since its called when FormatAll which may be called during the 1st inline hole
{
	mLineEnds->FreeData();
	fLastLineIndex = -1;
	if (emptyLine) {this->AppendEmptyLine();}
}
//***************************************************************************************************

void CFormatter::Compact()
{
	mLineEnds->Compact();
	mLineHeights->Compact();
}
//***************************************************************************************************

OSErr CFormatter::ReserveLines(long extraLines)
{
	return mLineEnds->Reserve(extraLines);
}
//***************************************************************************************************


void CFormatter::GetLineRange(long lineNo, TOffsetRange* range) const
{
	TOffsetPair offsets;
	mLineEnds->GetRangeBounds(lineNo, &offsets);
	range->Set(offsets.firstOffset, offsets.lastOffset, false, true);
}
//*************************************************************************************************

OSErr CFormatter::InsertLine(TLineInfo* newLineInfo, TFormattingInfo* formatInfo)
//formatInfo->lineIndex < 0 ==> insert at the end
{
	if (mLineEnds->InsertElements(1, &(newLineInfo->lineEnd), formatInfo->lineIndex) == nil) {
		DebugMessage("\p no mem to insert a line!");
		return memFullErr;
	}
	
	#ifdef txtnRulers
	if (fParagraphRuler)
		fParagraphRuler->AdjustLineHeight(&(newLineInfo->lineHeightInfo));
	#endif
	
	mLineHeights->InsertLine(newLineInfo->lineHeightInfo, formatInfo->lineIndex);
	
	++fLastLineIndex;
		
	return noErr;
}
//***************************************************************************************************

void CFormatter::SetLineInfo(TLineInfo* newLineInfo, TFormattingInfo* formatInfo)
{
	formatInfo->nextLineStart = mLineEnds->GetRangeEnd(formatInfo->lineIndex); //oldLineEnd
	
	mLineEnds->SetRangeEnd(formatInfo->lineIndex, newLineInfo->lineEnd);
	
	#ifdef txtnRulers
	if (fParagraphRuler) {fParagraphRuler->AdjustLineHeight(&(newLineInfo->lineHeightInfo));}
	#endif
	
	mLineHeights->SetLineHeightInfo(newLineInfo->lineHeightInfo, formatInfo->lineIndex);
	 
	 
	if (formatInfo->nextLineStart != newLineInfo->lineEnd) {
		long pastRemove = this->RemoveFormattedLines(formatInfo->lineIndex);
		if (pastRemove >= 0)
			{formatInfo->nextLineStart = pastRemove;}
	}
	
}
//***************************************************************************************************

void CFormatter::RemoveLines(long firstLine, long count)
{
	if (count > 0) {
		mLineEnds->RemoveElements(firstLine, count);
		
		mLineHeights->RemoveLines(count, firstLine);
		
		fLastLineIndex -= count;
	}
}
//***************************************************************************************************

long CFormatter::RemoveFormattedLines(long lastFormattedLine)
//returns the line end of the last deleted line, < 0 if none
{
	long lastFormattedChar;
	lastFormattedChar = mLineEnds->GetRangeEnd(lastFormattedLine);
	
	long deleteCount = 0;
	long lastLineIndex = fLastLineIndex;
	
	long lastLineEnd;
	long lineToCheck = lastFormattedLine;
	while (++lineToCheck <= lastLineIndex) {
		long lineEnd = mLineEnds->GetRangeEnd(lineToCheck);
		if (lineEnd > lastFormattedChar) {break;}
		++deleteCount;
		lastLineEnd = lineEnd;
	}
	
	if (deleteCount) {
		this->RemoveLines(++lastFormattedLine, deleteCount);
		
		return lastLineEnd;
	}
	else {return -1;}
}
//***************************************************************************************************

Boolean CFormatter::IsLineFeed(long offset) const
{
	return *mStyledText->GetCharPtr(offset) == kCRCharCode;
}
//***************************************************************************************************

void CFormatter::CalcCharsHeight(long charsOffset, short charsCount, TLineHeightInfo* lineHeightInfo) const
//Assume the port obj env are saved
{
	short maxAscent = 0;
	short maxDescent = 0;
	short maxLeading = 0;
	
	while (charsCount) {
		long 	runLen;
		CRunObject* runObj;
		mRunsRanges->GetNextRun(charsOffset, &runObj, &runLen);
		if (runLen > charsCount)
			runLen = charsCount;
		
		short runAscent, runDescent, runLeading;
		
		runObj->SetRunEnv();
		
		runObj->GetHeightInfo(&runAscent, &runDescent, &runLeading);
		if (runAscent > maxAscent)
			maxAscent = runAscent;
			
		if (runDescent > maxDescent)
			maxDescent = runDescent;
			
		if (runLeading > maxLeading)
			maxLeading = runLeading;

		charsCount -= short(runLen);
		charsOffset += runLen;
	}
	
	lineHeightInfo->lineHeight = maxAscent+maxDescent+maxLeading;
	lineHeightInfo->lineAscent = maxAscent;
}
//***************************************************************************************************

short CFormatter::BreakVisibleChars(long lineStart, long runOffset, short runLen, Fixed* widthAvail, CRunObject* runObj)
// widthAvail is I/O
//Assume the port is set to the obj env and the chars are locked
//runOffset is measured from the start of the whole text
{
	unsigned char* scriptRunChars;
	long scriptRunLen;
	Boolean forceBreak;
																			
	if (runOffset == lineStart) {
			scriptRunChars = mStyledText->GetCharPtr(lineStart);
			scriptRunLen = runLen;
			runOffset = 0;
			forceBreak = true;
		}
		else {
			long	scriptRunOffset;
			if (runObj->IsTextRun()) {//GetScriptRange doesn't search for scripts if one script is installed !
				mRunsRanges->GetScriptRange(runOffset, &scriptRunOffset, &scriptRunLen);
				if (lineStart > scriptRunOffset)
					scriptRunOffset = lineStart;
			}
			else
				scriptRunOffset = runOffset;
			
			long run_ScriptOffset = runOffset-scriptRunOffset;
			
			scriptRunChars = mStyledText->GetCharPtr(scriptRunOffset);
			scriptRunLen = runLen + run_ScriptOffset;
			runOffset = run_ScriptOffset;
			forceBreak = (scriptRunOffset == lineStart);
		}
	
	long breakLen;
	StyledLineBreakCode breakResult = runObj->LineBreak(scriptRunChars, scriptRunLen, runOffset
																									, widthAvail, forceBreak, &breakLen);
	
	if (breakResult != smBreakOverflow)
		*widthAvail = 0;
	
	return (short)breakLen;
}
//***************************************************************************************************

short CFormatter::BreakCtrlChar(long lineOffset, long ctrlCharOffset, Fixed* widthAvail)
//returns the brokenLen
{
	if (gParagCtrlChars.GetCurrCtrlChar() != kTabCharCode)
		return 1; //width 0
	
	if (*widthAvail <= 0)
		return 0;
	
	#ifdef txtnRulers
	if (fPendingTab.valid)
		*widthAvail -= fParagraphRuler->CalcPendingTabWidth(fPendingTab, fTabMungedWidth, *widthAvail);
	
	Fixed tabWidth = fParagraphRuler->GetTabWidth(fLineFormattingWidth - *widthAvail + fLineStartBlanks
																								, fDirection, &fPendingTab);
	#else
	Fixed tabWidth = GetDefaultTabWidth(fLineFormattingWidth - *widthAvail);
	#endif
	
	
	#ifdef txtnRulers
	if (fPendingTab.valid) {
		if (tabWidth > *widthAvail) {
			*widthAvail = 0;
			return 0;
		}
		else
			fTabMungedWidth = 0;
	}
	else
	#endif
	{
		*widthAvail -= tabWidth;
		
		if (*widthAvail < 0)
			return (lineOffset == ctrlCharOffset) ? 1 : 0; //if this is the first char in a line force break if necessary
	}
	
	return 1;
}
//***************************************************************************************************

#ifdef txtnRulers
short CFormatter::BreakAlignTabChars(long lineStart, long runStart, short runLen, Fixed* widthAvail
																		, CRunObject* runObj)
{
	short alignOffset = SearchChar(fPendingTab.theTab.alignChar, mStyledText->GetCharPtr(runStart), runLen);
	if (alignOffset < 0) {return 0;}
	
	short brokenLen;
	
	if (alignOffset == 0) {brokenLen = 0;}
	else {
		Fixed widthAvailBefore = *widthAvail;
		brokenLen = this->BreakVisibleChars(lineStart, runStart, alignOffset, widthAvail, runObj);
																				
		fTabMungedWidth += widthAvailBefore-*widthAvail;
	}
	
	*widthAvail -= fParagraphRuler->CalcPendingTabWidth(fPendingTab, fTabMungedWidth, *widthAvail);
	
	fPendingTab.valid = false;
	
	return brokenLen;
}
//***************************************************************************************************
#endif

short CFormatter::BreakRun(long lineStart, long runStart, short runLen, Fixed* widthAvail
														, CRunObject* runObj)
{
	short remainingLen = runLen;
	
	while (remainingLen) {//we don't check on "*widthAvail <=0" to give "BreakCtrlChar" to include the controls with zero width in the same line
		short brokenLen;
		
		long ctrlOffset = gParagCtrlChars.GetCurrCtrlOffset()-runStart; //offset from runStart, -ive if no curr
		
		if (ctrlOffset == 0) {
			brokenLen = this->BreakCtrlChar(lineStart, runStart, widthAvail);
			
			if (brokenLen)
				gParagCtrlChars.Next();
			else {
				if (*widthAvail <= 0)
					break;
			}
		}
		else {
			Fixed widthAvailBefore = *widthAvail;
			if (widthAvailBefore <= 0)
				break;
			
			short lenToBreak;
			if ((ctrlOffset > 0) && (remainingLen > ctrlOffset))
				lenToBreak = short(ctrlOffset);
			else
				lenToBreak = remainingLen;
			
			#ifdef txtnRulers
			if ((fPendingTab.valid) && (fPendingTab.theTab.kind == kAlignTab)) {
				brokenLen = this->BreakAlignTabChars(lineStart, runStart, lenToBreak, widthAvail, runObj);
				
				lenToBreak -= brokenLen;
				
				if ((lenToBreak > 0) && (*widthAvail > 0))
					{brokenLen += this->BreakVisibleChars(lineStart, runStart+brokenLen, lenToBreak, widthAvail, runObj);}
			}
			else
			#endif
			{brokenLen = this->BreakVisibleChars(lineStart, runStart, lenToBreak, widthAvail, runObj);}
			
			#ifdef txtnRulers
			if (fPendingTab.valid)
				fTabMungedWidth += widthAvailBefore-*widthAvail;
			#endif
		}
		
		runStart += brokenLen;
		remainingLen -= brokenLen;
	}
	
	#ifdef txtnDebug
	if ((runLen == remainingLen) && (lineStart == runStart))
		SignalPStr_("\p infinite loop!");
	#endif
	
	return runLen-remainingLen;
}
//***************************************************************************************************

//	//////////
//	jah 950130
void	CFormatter::GetFixedLineHeight(short *inLineHeight, short *inLineAscent) const
{
	if (inLineHeight)
		*inLineHeight = fLineHeight;

	if (inLineAscent)
		*inLineAscent = fLineAscent;
}

OSErr	CFormatter::SetFixedLineHeight(short inLineHeight, short inLineAscent)
/*
	You should do a global reformat after this call.
*/
{
	OSErr	err = noErr;
	
	fLineHeight = inLineHeight;
	fLineAscent = inLineAscent;

	return err;
}
//	\\\\\\\\\\

void CFormatter::BreakLine(long startBreak, short formattingWidth, TLineInfo* lineInfo)
//Assume the port obj env are saved, and the chars are locked
{
	if (formattingWidth <= 0) //e.g no frames
		formattingWidth = kMaxInt; 
	
	long currParagEnd = gParagCtrlChars.GetParagEnd();
	long oldParagraphEnd = currParagEnd;
	if (startBreak >= currParagEnd) {
		currParagEnd = gParagCtrlChars.Define(mStyledText->GetCharPtr(0), startBreak, mStyledText->CountCharsBytes());
		
		#ifdef txtnRulers
		fParagraphRuler = (CRulerObject*) fRulersRanges->OffsetToObject(startBreak);
		#endif
	}
	
	if (fCR) {
		lineInfo->lineEnd = currParagEnd;

		//	jah 950323
		if (fLineHeight) {
			lineInfo->lineHeightInfo.lineHeight = fLineHeight;
			lineInfo->lineHeightInfo.lineAscent = fLineAscent;
			return;
		}
	
		long temp = currParagEnd-oldParagraphEnd;
		
		this->CalcCharsHeight(oldParagraphEnd, short(temp), &lineInfo->lineHeightInfo);
		return;
	}
	
	#ifdef txtnRulers
	Boolean paragStart = (startBreak == 0) || this->IsLineFeed(startBreak-1);
	
	Fixed leftBlanks = fParagraphRuler->GetLineLeftBlanks(fDirection, paragStart);
	Fixed rightBlanks = fParagraphRuler->GetLineRightBlanks(fDirection, paragStart);
	fLineFormattingWidth = LongToFixed(formattingWidth) - leftBlanks - rightBlanks;
	
	fLineStartBlanks = (fDirection == kLR) ? leftBlanks : rightBlanks; //needed to calc tab widths
		
	#else
	fLineFormattingWidth = LongToFixed(formattingWidth);
	#endif
	
	Fixed widthAvail = fLineFormattingWidth;

	long lineStart = startBreak;
	short lineLen = 0;
	
	short maxAscent = 0;
	short maxDescent = 0;
	short maxLeading = 0;
	
	long runLen;
	CRunObject* runObj;
	
	#ifdef txtnRulers
	fPendingTab.valid = false;
	#endif
	
	while ((widthAvail > 0) && (mRunsRanges->GetNextRun(startBreak, &runObj, &runLen) >= 0)) {
		runObj->SetRunEnv();
		
		long breakMaxLen = MinLong(currParagEnd - startBreak, runLen);
		if (breakMaxLen > kMaxLineLen) {breakMaxLen = kMaxLineLen;} //¥¥this limitation on line len seems to have no effect on the roman system, but a BIG effect on AIS when calling StyledLineBreak
		
		short brokenLen = this->BreakRun(lineStart, startBreak, short(breakMaxLen), &widthAvail, runObj);
		if (brokenLen == 0)	 {break;} //or continue since in this case widthAvail <= 0
		
		lineLen += brokenLen; //brokenLen may be -ive
		
		if (brokenLen < 0) { // widthAvail == 0
			this->CalcCharsHeight(lineStart, lineLen, &lineInfo->lineHeightInfo);
			maxAscent = -1;
			break;
		}
		
		short runAscent, runDescent, runLeading;
		runObj->GetHeightInfo(&runAscent, &runDescent, &runLeading);
		if (runAscent > maxAscent)
			maxAscent = runAscent;
		if (runDescent > maxDescent)
			maxDescent = runDescent;
		if (runLeading > maxLeading)
			maxLeading = runLeading;
		
		startBreak += brokenLen;
	}	
	
	if (maxAscent >= 0) {//else lineInfo->lineHeight and lineInfo->lineAscent are already calculated
		lineInfo->lineHeightInfo.lineHeight = maxAscent+maxDescent+maxLeading;
		lineInfo->lineHeightInfo.lineAscent = maxAscent;
	}
	
	lineInfo->lineEnd = lineStart + lineLen;
}
//***************************************************************************************************

Boolean CFormatter::AppendEmptyLine()
//if the text is empty or the last line ends with a CR an empty line is appended.
{
	TLineInfo lineInfo;
	lineInfo.lineEnd = -1;
	lineInfo.lineHeightInfo.lineHeight = -1;
	
	if (fLastLineIndex < 0)
		lineInfo.lineEnd = 0; //inserts empty line with kDefaultLineHeight
	else {
		if (!mLineEnds->IsRangeEmpty(fLastLineIndex)) {
			lineInfo.lineEnd = mLineEnds->GetRangeEnd(fLastLineIndex);
			
			if (!this->IsLineFeed(lineInfo.lineEnd - 1))
				lineInfo.lineEnd = -1;
			else {
				CRunObject* lastTextRun = mRunsRanges->CharToTextRun(mStyledText->CountCharsBytes());
				if (lastTextRun) {
					short descent, leading;
lastTextRun->SetRunEnv();
					lastTextRun->GetHeightInfo(&lineInfo.lineHeightInfo.lineAscent, &descent, &leading);
					lineInfo.lineHeightInfo.lineHeight = lineInfo.lineHeightInfo.lineAscent + descent + leading;
				}
				//else inserts empty line with kDefaultLineHeight
			}
		}
	}
	
	if (lineInfo.lineEnd >= 0) {
		TFormattingInfo formatInfo;
		formatInfo.lineIndex = -1;
		
		if (lineInfo.lineHeightInfo.lineHeight < 0) {
			lineInfo.lineHeightInfo.lineHeight = kDefaultLineHeight;
			lineInfo.lineHeightInfo.lineAscent = kDefaultLineHeight;
		}
		
		return this->InsertLine(&lineInfo, &formatInfo) == noErr;
	}
	else
		return false;
}
//***************************************************************************************************


OSErr CFormatter::FormatAll()
{
	mLineHeights->RemoveLines(this->CountLines(), 0);
	
	this->FreeData(false/*append empty line*/);
	
	TLineInfo lineInfo;
	lineInfo.lineEnd = 0;
	
	long countChars = mStyledText->CountCharsBytes();
	
	TFormattingInfo formatInfo;
	formatInfo.lineIndex = 0;

	#ifndef txtnRulers //all lines have the same format width
	short lineFormatWidth = mGeometry->GetLineFormatWidth(0);
	#endif
	
	while (lineInfo.lineEnd != countChars) {
		#ifdef txtnRulers
		this->BreakLine(lineInfo.lineEnd, fFrames->GetLineFormatWidth(formatInfo.lineIndex), &lineInfo);
		#else
		this->BreakLine(lineInfo.lineEnd, lineFormatWidth, &lineInfo);
		#endif
		
		OSErr err = this->InsertLine(&lineInfo, &formatInfo);
		if (err) {
			if (fLastLineIndex >= 0)
				{mLineEnds->SetRangeEnd(fLastLineIndex, countChars);}
			return err;
		}
		
		++formatInfo.lineIndex;
	}
	
	this->AppendEmptyLine();
	
	this->Compact();
	
	return noErr;
}
//***************************************************************************************************

OSErr CFormatter::Format(long startChar/*=0*/, long endChar/*=-1*/
											, long* firstLine /*=nil*/, long* lastLine /*=nil*/)
{
	long dummy;
	if (!firstLine) {
		firstLine = &dummy;
		lastLine = &dummy;
	}
	
	if (fFormatLevel < 0) {
		*firstLine = *lastLine = -1;
		return noErr;
	}
	
	long countChars = mStyledText->CountCharsBytes();
	
	if (endChar < 0) {endChar = countChars;}
	
	gParagCtrlChars.Invalid();

	#ifdef txtnRulers
	fParagraphRuler = nil;
	#endif

	TTextEnv savedTextEnv;
	mStyledText->SetTextEnv(&savedTextEnv);
	
	mStyledText->LockChars();

	OSErr err;
	
	if ((startChar == 0) && (endChar == countChars)) {
		err = this->FormatAll();
		*firstLine = 0;
		*lastLine = fLastLineIndex;
	}
	else {err = this->FormatRange(startChar, endChar, firstLine, lastLine);}
	
	mStyledText->UnlockChars();
	mStyledText->RestoreTextEnv(savedTextEnv);

	return err;
}
//***************************************************************************************************

OSErr CFormatter::FormatRange(long rangeStart, long rangeEnd, long* firstLine, long* lastLine)
//firstLine is the first formatted line and lastLine is the last formatted one.
{
	long totalChars = mStyledText->CountCharsBytes();

	Assert_(rangeStart <= rangeEnd);
	
	TFormattingInfo formatInfo;
	formatInfo.lastFormatChar = rangeEnd;

	formatInfo.lineIndex = mLineEnds->OffsetToRangeIndex(rangeStart);

	if ((rangeStart >= totalChars) && (mLineEnds->IsRangeEmpty(formatInfo.lineIndex))) {//rangeStart can be equal to totalChars if for instance backspace at the last char
	//added when a bug is noticed when undoing pending ruler changes
		*firstLine = *lastLine = fLastLineIndex;
		return noErr;
	}
	
	/*start updating lines from the line preceding the line containing rangeStart, since some chars may
	"up" to the preceding line, However this can not happenif the preceding line ends with a CR */
	Boolean isPrevLine;
	if ((formatInfo.lineIndex)
		&& (!this->IsLineFeed(mLineEnds->GetRangeEnd(formatInfo.lineIndex-1)-1))) {
		--formatInfo.lineIndex;
		isPrevLine = true;
	}
	else {isPrevLine = false;}
	
	*firstLine = -1;
	
	formatInfo.nextLineStart = mLineEnds->GetRangeStart(formatInfo.lineIndex);
	
	Boolean endFormat = false;
	TLineInfo currLineInfo;
	currLineInfo.lineEnd = formatInfo.nextLineStart;
	
	#ifndef txtnRulers //all lines have the same format width
	short lineFormatWidth = mGeometry->GetLineFormatWidth(formatInfo.lineIndex);
	#endif
	
	--formatInfo.lineIndex;
	OSErr err = noErr;
	
	do {
		++formatInfo.lineIndex;
		
		//in the next "BreakLine" call, the first currLineInfo.lineEnd is actually the last formatted line end
		#ifdef txtnRulers
		this->BreakLine(currLineInfo.lineEnd, fFrames->GetLineFormatWidth(formatInfo.lineIndex)
									, &currLineInfo);
		#else
		this->BreakLine(currLineInfo.lineEnd, lineFormatWidth, &currLineInfo);
		#endif
		
		//¥fLastLineIndex is used in the loop (instead of a var intialized before the loop) since it changes as InsertLine or remove is called
		if ((formatInfo.lineIndex > fLastLineIndex) || (currLineInfo.lineEnd <= formatInfo.nextLineStart)) {
			err = this->InsertLine(&currLineInfo, &formatInfo);
			if (err) {
				mLineEnds->SetRangeEnd(fLastLineIndex, totalChars); //effective only if "formatInfo.lineIndex > fLastLineIndex"
				break;
			}
			//we can break any time in the format process. the effect is that some line ends will not be calculated correctly, better than crashes!
		}
		else {
			Boolean prev_Unchanged = (isPrevLine) && (currLineInfo.lineEnd == mLineEnds->GetRangeEnd(formatInfo.lineIndex));
			
			if (!prev_Unchanged) {this->SetLineInfo(&currLineInfo, &formatInfo);}
			
			if (*firstLine < 0) {
				*firstLine = formatInfo.lineIndex;
				if (prev_Unchanged) {++(*firstLine);}
			}
			else {if (formatInfo.lineIndex < *firstLine) {*firstLine = formatInfo.lineIndex;}}
		}
		
		endFormat = ((currLineInfo.lineEnd == totalChars)
								|| (currLineInfo.lineEnd == formatInfo.nextLineStart)
									&& (currLineInfo.lineEnd >= formatInfo.lastFormatChar)
									&& (!isPrevLine));
									
		isPrevLine = false;
		
		/*
		the condition "!isPrevLine" in "endFormat" has been added when KeyDown is implemented. In this 
		case FormatRange may be called with "rangeStart" == "rangeEnd", the condition is added for the case of
		"rangeStart" == "rangeEnd" == a lineBoundary
		*/
	} while (!endFormat);
	
	Boolean atEnd = (formatInfo.lineIndex == fLastLineIndex); //made before, since AppendEmptyLine may change fLastLineIndex
	if ((this->AppendEmptyLine()) && (atEnd)) {++formatInfo.lineIndex;}
	
	*lastLine = formatInfo.lineIndex;
	
	return err;
}
//***************************************************************************************************

OSErr CFormatter::ReplaceRange(long offset, long oldLen, long newLen
														, long* firstLine, long* lastLine)
/*
	-if oldLen == 0 ==> acts as Insert, if newLen == 0 ==> acts a delete
	-firstLine and  lastLine have the same meaning as in "Format"
*/
{
	long diff = newLen - oldLen;
	long endFormat = offset+newLen;

	if (oldLen) { //check on "oldLen" is made to optimize for KeyDown case
		if ((endFormat == mStyledText->CountCharsBytes()) && (mLineEnds->IsRangeEmpty(fLastLineIndex))) {
			this->RemoveLines(fLastLineIndex, 1);
		}
	}
	
	OSErr err;
	
	if (diff > 0) {//including keydown
		/*
		we'll insert the new chars in the line at "offset". if "offset" is on a line boundary
		(line1, line2) we'll insert in line2 (except if "offset" == last char or "offset" == 0
		, where we'll insert in line1)
		The "Format" routine will adjust the line ends correctly. But, if our assumption in the case of
		line boundaries was wrong (means that all new chars went to line1), "lastLine" will be equal to 
		line2 which is wrong and will make line2 to be redrawn.
		To adjust "lastLine" we have compare the line end of line1 before and after the "Format" op
		(keeping in mind the "offset == last char" or offset == 0 case).
		*/
		
		long lineNo = mLineEnds->OffsetToRangeIndex(offset);
		
		long lineStart = this->GetLineStart(lineNo); //line end of lineNo-1
		if (lineStart != offset) {lineStart = -1;} //include the case of "offset" == last char, case of "offset" == 0 will work automatically
		
		mLineEnds->OffsetRanges(diff, lineNo);

		err = this->Format(offset, endFormat, firstLine, lastLine);
		
		if ((lineStart > 0) && (this->GetLineStart(lineNo) == lineStart+diff))
			{*lastLine = *firstLine;}
		
		return err;
	}
	
	TSectRanges sectedLines;
	if (mLineEnds->SectRanges(offset, oldLen, &sectedLines) > 1) {
		if (sectedLines.leadCount) {mLineEnds->SetRangeEnd(sectedLines.leadIndex, offset);} //else will be adjusted by OffsetRanges
		
		if (!sectedLines.trailCount) {++sectedLines.trailIndex;}
	}
	
	mLineEnds->OffsetRanges(diff, sectedLines.trailIndex);
	
	
	if (sectedLines.insideCount) {
		this->RemoveLines(sectedLines.insideIndex, sectedLines.insideCount);
	}
	
	
	err = this->Format(offset, endFormat, firstLine, lastLine);
	*firstLine = MinLong(*firstLine, sectedLines.leadIndex);
	
	return err;
}
//***************************************************************************************************

/*
void CFormatter::RecalcLinesHeight(long firstLine, long lastLine)
{
	TTextEnv savedTextEnv;
	mStyledText->SetTextEnv(&savedTextEnv); //for CalcCharsHeight
	
	long lineStart = mLineEnds->GetRangeStart(firstLine);
	
	for (long lineNo = firstLine; lineNo <= lastLine; ++lineNo) {
		long lineLen = mLineEnds->GetRangeLen(lineNo);
		
		TLineInfo lineInfo;
		this->CalcCharsHeight(lineStart, short(lineLen), &lineInfo.lineHeightInfo);
		
		#ifdef txtnRulers
		fParagraphRuler = (CRulerObject*)(fRulersRanges->OffsetToObject(lineStart));
		//¥¥bad, fParagraphRuler is set here because it is used by SetLineInfo
		#endif
		
		lineStart += lineLen;
		
		lineInfo.lineEnd = lineStart;
		
		this->SetLineInfo(lineNo, &lineInfo);
	}
	
	mStyledText->RestoreTextEnv(savedTextEnv);
}
*/
//***************************************************************************************************


#ifdef txtnDebug
void CFormatter::CheckCoherence(long count)
{
	if (fLastLineIndex != mLineEnds->CountElements()-1)
		{SignalPStr_("\p incoherent count lines");}
	
	long lastRangeEnd = mLineEnds->GetRangeEnd(fLastLineIndex);
		
	if (mLineEnds->IsRangeEmpty(fLastLineIndex)) {
		long lineEnd = mLineEnds->GetRangeEnd(fLastLineIndex);
		if ((lineEnd) && (!this->IsLineFeed(lineEnd-1)))
			{SignalPStr_("\p¥¥Unnecessary empty line at the end");}
	}
}
//***************************************************************************************************
#endif
