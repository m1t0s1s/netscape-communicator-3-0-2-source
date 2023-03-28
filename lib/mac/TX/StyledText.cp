// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

#include "StyledText.h"

#include <Windows.h>




CStyledText::CStyledText()
{
	Assert_(false);		// parameters required
}

//¥¥textPort may be nil

CStyledText::CStyledText(
	GrafPtr 		inPort,
	CRunObject* 	inProtoObj,
	TSize 			inSizeInfo)
{
	mRunRanges = NULL;
	mTextChars = NULL;
	mTextPort = inPort;
	fSavedObj = inProtoObj;
	fTextEnvLevel = 0;
	
	try
		{
		mRunRanges = new CRunsRanges;
		mRunRanges->IRunsRanges(inProtoObj->CreateNew(), inSizeInfo);

		mTextChars = new CArray;
		mTextChars->IArray(1, (inSizeInfo == kLargeSize) ? 50 : 10/*more elements count*/);
		}
	catch (ExceptionCode inErr)
		{
		delete mRunRanges;
		delete mTextChars;
		throw inErr;
		}
}


CStyledText::~CStyledText()
{
	mRunRanges->Free();
	delete mRunRanges;

	fSavedObj->Free();
	
	mTextChars->Free();
	delete mTextChars;
}

//	//////////
//	jah 950130...
CArray	*	CStyledText::GetArray(void)
{
	return mTextChars;
}

void	CStyledText::SetArray(CArray *inArray)
{
	if (mTextChars)
		mTextChars->Free();
	mTextChars = inArray;
}
//	\\\\\\\\\\


void CStyledText::SetTextPort(GrafPtr newPort)
{
	mTextPort = newPort;
}

GrafPtr CStyledText::GetTextPort() const
{
	return (mTextPort && !gTextensionImaging) ? mTextPort : qd.thePort;
}

void CStyledText::CharToWord(TOffset theChar, TOffsetRange* wordRange)
{
	long scriptOffset;
	long scriptLen;

	char wordScript = mRunRanges->GetScriptRange(theChar, &scriptOffset, &scriptLen);
	
	//FindWord needs shorts ==> clip
	long scriptRunEnd = scriptOffset+scriptLen;
	scriptOffset = MaxLong(scriptOffset, theChar-kMaxInt/2);
	scriptRunEnd = MinLong(scriptRunEnd, theChar+kMaxInt/2);
	
	TTextEnv savedTextEnv;
	this->SetTextEnv(&savedTextEnv);
	
	TextFont(ScriptToDefaultFont(wordScript));
	
	this->LockChars();
	
	long temp1 = scriptRunEnd-scriptOffset;
	long temp2 = theChar-scriptOffset;
	
	OffsetTable wordOffsets;
	
	FindWord(Ptr(this->GetCharPtr(scriptOffset)), short(temp1)
		, short(temp2), !theChar.trailEdge, nil/*breaksTable*/, wordOffsets);
	
	wordRange->Set(wordOffsets->offFirst + scriptOffset, wordOffsets->offSecond + scriptOffset
								, false, true);
	
	this->UnlockChars();
	
	this->RestoreTextEnv(savedTextEnv);
}

const CRunObject* CStyledText::GetNextRun(long offset, long* runLen) const
{
	CRunObject* runObj;
	mRunRanges->GetNextRun(offset, &runObj, runLen);
	return runObj;
}

short CStyledText::GetByteOrder(unsigned char* chars, short offset, short charScript)
//returns the same result as CharByte
{
	TTextEnv savedTextEnv;
	this->SetTextEnv(&savedTextEnv);
	
	TextFont(ScriptToDefaultFont(charScript)); //CharByte definitely needs a proper font

	short whichByte = CharByte(Ptr(chars), offset);
	
	this->RestoreTextEnv(savedTextEnv);
	
	return whichByte;
}

short CStyledText::AdvanceOffset(long offset, Boolean forward)
{
	if (forward)
		{
		if (offset >= mTextChars->CountElements())
			return 0;
		}
	else
		{
		--offset;
		if (offset < 0)
			return 0;
		}
	
	TOffset charOffset(offset, !forward/*trailEdge*/);
	
	long runIndex = mRunRanges->OffsetToRangeIndex(charOffset);
	if (mRunRanges->RangeIndexToObject(short(runIndex))->GetObjFlags() & kIndivisibleRun)
		return mRunRanges->GetRangeLen(runIndex);
	
	if (!g2Bytes)
		return 1;
	
	long scriptOffset;
	long scriptLen;
	
	char theScript = mRunRanges->GetScriptRange(charOffset, &scriptOffset, &scriptLen);
	scriptOffset = MaxLong(scriptOffset, offset-kMaxInt/2);

	long temp = offset-scriptOffset;
	
	this->LockChars();
	short whichByte = this->GetByteOrder(this->GetCharPtr(scriptOffset), short(temp), theScript);
	this->UnlockChars();
	
	short count;
	if (forward)
		count = (whichByte == smFirstByte) ? 2 : 1;
	else
		count = (whichByte == smLastByte) ? 2 : 1;
	
	return count;
}


/*Note on FindScriptRun:
In RL systems (AIS for instance) if sysJust=-1 FindScriptRun returns
runs in reverse order (starting from the end of the text) i.e it returns runs in their visible order
which Doesn't correspond (at all) to what is explained and expected by it.
Apparently it plays the role of a Mono GetFormatOrder.
The sysJust will be set to 0 before calling it.
Note also that the current port font should be set properly, for instance on AIS if the curr font is
roman the returned scripts will depend on the fontForce flag if it is off script 0 will be returned
for all the text (even for arabic runs), if it is on or the font is arabic ==>ok.
And then "system font" is a suitable font for this routine to work properly in a Multi style 
env.
*/

char CStyledText::CalcNextScriptRun(long offset, long len, long* scriptLen)
{
	if (!gHasManyScripts) {
		*scriptLen = len;
		return 0;
	}
	
	GrafPtr savedPort, anyPort;
	GetPort(&savedPort);
	GetWMgrPort(&anyPort);
	/*This routine should be independent from mTextPort, since it may be needed to call it when there 
		is no associated port (reading files in MacApp for instance)
	*/
	SetPort(anyPort);
	
	short savedFont = (qd.thePort)->txFont;
	TextFont(GetSysFont());
	
	short savedJust = ::GetSysDirection();
	::SetSysDirection(0);

	this->LockChars();
	unsigned char* textPtr = (unsigned char*) mTextChars->GetElementPtr(offset);
	
	if (len > kMaxInt)
		{len = kMaxInt;} //FindScriptRun is limited to short
	
	ScriptRunStatus runScript = FindScriptRun((Ptr)textPtr, len, scriptLen);
	
	#ifdef txtnDebug
	if ((*scriptLen <= 0) || (*scriptLen > len)) {SignalPStr_("\p Strange len from FindScriptRun!");}
	#endif
	
	//include CRs and tabs in the script run preceding it
	while (*scriptLen != len) {
		unsigned char theChar = *(textPtr + *scriptLen);
		
		//special case for Korean, FindScriptRun consider the 0x20 as roman run, which affects very badly the performance (each 2 words are seperated by a different run!) 
		if (((runScript.script == smKorean) && (theChar == kRomanSpaceCharCode)) || (IsCtrlChar(theChar)))
			{++(*scriptLen);}
		else
			{break;}
	}
	this->UnlockChars();
	
	TextFont(savedFont);
	SetPort(savedPort);
	::SetSysDirection(savedJust);
	
	return runScript.script;
}

OSErr CStyledText::RunToScriptRuns(long runOffset, long runLen, const CRunObject* defaultRun)
//-defaultRun may be nil or partially valid
{
	if ((!gHasManyScripts) || (runLen == 0)) {return noErr;}
	
	CRunObject* oldRunObj = (CRunObject*)(mRunRanges->OffsetToObject(runOffset));
	
	Assert_(oldRunObj);
	Assert_(oldRunObj->IsTextRun());
	
	char oldRunScript = oldRunObj->GetRunScript();

	CRunObject* scriptRunObj = (CRunObject*)(fSavedObj->CreateNew());
	if (!scriptRunObj) {return memFullErr;}
	scriptRunObj->Assign(oldRunObj);
	
	char defaultRunScript = (defaultRun) ? defaultRun->GetRunScript() : smUninterp;
	
	OSErr err = noErr;
	
	while ((!err) && (runLen)) {
		long scriptLen;
		char runScript = this->CalcNextScriptRun(runOffset, runLen, &scriptLen);
		
		if (runScript != oldRunScript) {
			scriptRunObj->SetDefaults(runScript+kUpdateOnlyScriptInfo);
			
			if (runScript == defaultRunScript)
				scriptRunObj->Update(defaultRun);
			
			err = mRunRanges->ReplaceRange(runOffset, scriptLen, scriptLen, scriptRunObj);
		}
		
		runLen -= scriptLen;
		runOffset += scriptLen;
	}
	
	scriptRunObj->Free();
	
	return err;
}

OSErr CStyledText::CalcScriptRuns(const CRunObject* defaultRun, char* majorDirection)
/*
-defaultRun may be nil
-returns in majorDirection the suggested direction for the text depending on the direction of the 
majority of the chars, returns kSysDirection if no chars*/
{
	long len = mTextChars->CountElements();
	
	mRunRanges->FreeData();
	
	if (!len) {
		*majorDirection = kSysDirection;
		return noErr;
	}
	
	CRunObject* scriptRunObj = (CRunObject*)(fSavedObj->CreateNew());
	if (!scriptRunObj) {return memFullErr;}
	
	short dirRunsDiff = 0; //<=0 ==> RL else LR
	long offset = 0;
	
	char defaultRunScript = (defaultRun) ? defaultRun->GetRunScript() : smUninterp;
	
	OSErr err = noErr;
	
	while ((!err) && (len)) {
		long scriptLen;
		char runScript = this->CalcNextScriptRun(offset, len, &scriptLen);
		scriptRunObj->SetDefaults(runScript + ((offset) ? kUpdateOnlyScriptInfo : 0));
		if (defaultRunScript == runScript)
			scriptRunObj->Update(defaultRun);
		//note that defaultRun may be partially valid
		
		if (scriptRunObj->GetRunDir() == kLR) {++dirRunsDiff;}
		else {--dirRunsDiff;}
		
		err = mRunRanges->ReplaceRange(offset, 0/*oldLen*/, scriptLen/*newLen*/, scriptRunObj);
		
		len -= scriptLen;
		offset += scriptLen;
	}
	
	scriptRunObj->Free();
	
	*majorDirection = (dirRunsDiff <= 0) ? kRL : kLR;

	return noErr;
}


void CStyledText::SetTextEnv(TTextEnv* savedTextEnv)
{
	if ((savedTextEnv->allowNesting) || (fTextEnvLevel++ == 0)) {
		GetPort(&savedTextEnv->savedPort);
		
		SetPort(this->GetTextPort());
		
		fSavedObj->SaveRunEnv();
		
		savedTextEnv->savedSysJust = ::GetSysDirection();
		
		if (gHasManyScripts) {
			savedTextEnv->savedFontForce = GetScriptManagerVariable(smFontForce);
			::SetScriptManagerVariable(smFontForce, 0);
		}
	}
}


void CStyledText::RestoreTextEnv(const TTextEnv& savedTextEnv)
{
	#ifdef txtnDebug
	if (savedTextEnv.allowNesting)
		Assert_(fTextEnvLevel >= 0);
	else
		Assert_(fTextEnvLevel > 0);
	#endif
	
	if ((savedTextEnv.allowNesting) || (--fTextEnvLevel == 0)) {
		fSavedObj->SetRunEnv();
		
		SetPort(savedTextEnv.savedPort);
		
		::SetSysDirection(savedTextEnv.savedSysJust);
		
		if (gHasManyScripts)
			::SetScriptManagerVariable(smFontForce, savedTextEnv.savedFontForce);
	}
}

