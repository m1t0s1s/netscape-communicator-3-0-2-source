// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

#include "TextensionCommon.h"
#include "Array.h"
#include "RunObject.h"
#include "RunsRanges.h"



//***************************************************************************************************


CRunsRanges::CRunsRanges()
{
}
//***************************************************************************************************

void CRunsRanges::IRunsRanges(CAttrObject* protoObj, TSize sizeInfo /*= kLargeSize*/)
{
	this->IObjectsRanges(protoObj, (sizeInfo == kLargeSize) ? 10 : 1/*more elements count*/);
}
//***************************************************************************************************


char CRunsRanges::GetScriptRange(TOffset charOffset, long *startOffset, long	*countChars) const
{
	long totalChars = this->GetLastRangeEnd();
	
	if ((!gHasManyScripts) || (!totalChars)) {
		*startOffset = 0;
		*countChars = totalChars;
		return 0;
	}
	
	#ifdef txtnDebug
	if ((charOffset.offset > totalChars) || (charOffset.offset < 0)) {SignalPStr_("\pBad offset in GetScriptRange");}
	#endif
	
	short runIndex = short(this->OffsetToRangeIndex(charOffset));
	
	CRunObject* runObj = (CRunObject*)(this->RangeIndexToObject(runIndex));
	#ifdef txtnDebug
		if (!runObj->IsTextRun()) {SignalPStr_("\pGetScriptRange at a non text object");}
	#endif
	
	register char theScript = runObj->GetRunScript();
	
	*countChars = 0;
	
	short tempPosIndex = runIndex;
	do {
		*startOffset = this->GetRangeStart(tempPosIndex);
		*countChars += this->GetRangeLen(tempPosIndex);
	}
	while ((--tempPosIndex >= 0) && (this->IsSameScript(tempPosIndex, theScript)));
	
	short lastRunIndex = short(this->CountRanges());
	while ((++runIndex < lastRunIndex) && (this->IsSameScript(runIndex, theScript)))
		{*countChars += this->GetRangeLen(runIndex);}
		
	return theScript;
}
//***************************************************************************************************

CRunObject* CRunsRanges::IsSameScript(short runIndex, char theScript) const
{
	register CRunObject* theRun = (CRunObject*)(this->RangeIndexToObject(runIndex));
	//no need to check if the run is a text run (GetRunScript returns smUnimplemented in this case)
	return (theRun->GetRunScript() == theScript) ? theRun : nil;
}
//***************************************************************************************************

CRunObject* CRunsRanges::IsTextRun(short runIndex) const
{
	register CRunObject* theRun = (CRunObject*)(this->RangeIndexToObject(runIndex));
	return (theRun->IsTextRun()) ? theRun : nil;
}
//***************************************************************************************************


CRunObject* CRunsRanges::CharToTextRun(TOffset charOffset) const
{
	register short runIndex = short(this->OffsetToRangeIndex(charOffset));
	if (runIndex < 0) {return nil;}
	
	register CRunObject* theRun = this->SearchTextRunBackward(runIndex);
	
	if (!theRun) {theRun = this->SearchTextRunForward(++runIndex);}
	
	return theRun;
}
//***************************************************************************************************


CRunObject* CRunsRanges::SearchTextRunBackward(short runIndex) const
//returns nil if runIndex < 0
{
	while (runIndex >= 0) {
		register CRunObject* theRun = this->IsTextRun(runIndex);
		
		if (theRun) {return theRun;}
		
		--runIndex;
	}
	
	return nil;
}
//***************************************************************************************************

CRunObject* CRunsRanges::SearchTextRunForward(short runIndex) const
//returns nil if runIndex >= countRuns
{
	register short countRuns = short(this->CountRanges());
	
	while (runIndex < countRuns) {
		register CRunObject* theRun = this->IsTextRun(runIndex);
		
		if (theRun) {return theRun;}
		
		++runIndex;
	}
	
	return nil;
}
//***************************************************************************************************


CRunObject* CRunsRanges::CharToScriptRun(TOffset charOffset, char theScript) const
{
	register short runIndex = short(this->OffsetToRangeIndex(charOffset));
	if (runIndex < 0) {return nil;}
	
	if (!gHasManyScripts) {return (CRunObject*)(this->RangeIndexToObject(runIndex));}
	
	register CRunObject* theRun = this->SearchScriptBackward(runIndex, theScript);
	
	if (!theRun) {theRun = this->SearchScriptForward(runIndex+1, theScript);}
	
	return theRun;
}
//***************************************************************************************************



CRunObject* CRunsRanges::SearchScriptBackward(short runIndex, char theScript) const
//returns nil if runIndex < 0
{
	while (runIndex >= 0) {
		CRunObject* theRun = this->IsSameScript(runIndex, theScript);
		
		if (theRun) {return theRun;}
		
		--runIndex;
	}
	
	return nil;
}
//***************************************************************************************************

CRunObject* CRunsRanges::SearchScriptForward(short runIndex, char theScript) const
//returns nil if runIndex >= countRuns
{
	short countRuns = short(this->CountRanges());
	
	while (runIndex < countRuns) {
		CRunObject* theRun = this->IsSameScript(runIndex, theScript);
		
		if (theRun) {return theRun;}
		
		++runIndex;
	}
	
	return nil;
}
//***************************************************************************************************
