// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

#pragma once

#include "TextensionCommon.h"
#include "Array.h"
#include "RunObject.h"
#include "ObjectsRanges.h"



class	CRunsRanges	:	public CObjectsRanges
{
public:
	CRunsRanges();
	
	void IRunsRanges(CAttrObject* protoObj, TSize sizeInfo = kLargeSize);
	
	inline short GetNextRun(long offset, CRunObject** theRun, long* runLen) const
		{return this->GetNextObjectRange(offset, (CAttrObject**)theRun, runLen);}
	// returns the run index, -1 if no next

	char	GetScriptRange(TOffset charOffset, long *startOffset, long	*countChars) const;
	//return theScript

	CRunObject* CharToTextRun(TOffset charOffset) const;
	/*
	-returns the nearest run to "charOffset" having text class type.
	-returns nil if no such run
	*/
	
	CRunObject* CharToScriptRun(TOffset charOffset, char theScript) const;
	/*
	-returns the nearest run to "charOffset" having the script "theScript".
	-returns nil if no such run
	*/
		
protected:
	
private:
	CRunObject* IsSameScript(short runIndex, char theScript) const;
	
	CRunObject* IsTextRun(short runIndex) const;
	
	CRunObject* SearchScriptBackward(short runIndex, char theScript) const;
	CRunObject* SearchScriptForward(short runIndex, char theScript) const;

	CRunObject* SearchTextRunBackward(short runIndex) const;
	CRunObject* SearchTextRunForward(short runIndex) const;
};



