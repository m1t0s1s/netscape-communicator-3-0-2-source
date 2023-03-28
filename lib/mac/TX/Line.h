// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ



#pragma once

#include "TextensionCommon.h"
#include "Array.h"
#include "StyledText.h"


#ifdef txtnRulers
#include "RulersRanges.h"
#include "RulerObject.h"
#endif



//***************************************************************************************************

struct LineRunInfo {
	CRunObject*	runObj;
	short runOffset; //offset from the start of the line
	short runLen;
	Fixed runWidth;
	Fixed runExtraWidth;
	char runCtrlChar;
};
//***************************************************************************************************


class CLine
{
public:	
	
								CLine();
	
	#ifdef txtnRulers
	void ILine(CStyledText* lineText, char direction, CRulersRanges* rulersRanges, TSize sizeInfo = kLargeSize);
	#else
	void ILine(CStyledText* lineText, char direction, TSize sizeInfo = kLargeSize);
	#endif
	
	void Free();

	void DoLineLayout(long lineStart, short lineLen, short maxLineWidth);

	void Draw(const Rect& lineRect, short lineAscent);
											
	void	Invalid();

	Boolean	IsValid(void) const;

	void SetDirection(char newDirection);
	
	
	CRunObject* PixelToCharacter(Fixed hPixel, TOffsetRange* charRange);
	
	short CharacterToPixel(TOffset charOffset, char caretDir = kRunDir);
	
	Boolean VisibleToBackOrder(TOffset* charOffset, char runDir);
	
	short GetRunDirection(const LineRunInfo& runInfo);
	//public since called from "RunDirProc" which can't be a method

private:
//-----
CStyledText*	fStyledText;
CRunsRanges* fRunsRanges;

#ifdef txtnRulers
CRulersRanges* fRulersRanges;
CRulerObject* fLineRuler;
#endif

long 		fStart;
short		fVisLen;
Boolean fNoDraw;
char		fDirection;


CArray*	fOrderedRuns; // array of LineRunInfo contains the runs in their visible order
CArray*	fUnorderedRuns;

short	fLastRun;

Fixed	fLeftEdge;
Fixed fMaxWidth;
	
	void GetRunDisplayInfo(const LineRunInfo* runInfo, LineRunDisplayInfo*);
	
	Fixed RunCharToPixel(const LineRunInfo* runInfo, short offset);
	
	Fixed RunMeasure(const LineRunInfo* runInfo);

	void InsertRun(long runStart, short runLen, CRunObject* runObj, CArray* unorderedRuns);

	void 	DefineRuns(long lineStart, short lineLen, CArray* unorderedRuns);
	
	short CalcVisibleLength(CArray* theRuns, short lineLen, short* blankRuns);
	
	void 	OrderRuns(CArray* unorderedRuns);

	Fixed DefineRunWidths(CArray* unorderedRuns, Fixed maxWidth, Boolean paragStart);

	#ifdef txtnRulers
	Fixed CalcAlignTabWidth(TPendingTab* pendingTab, Fixed tabTrailWidth, const LineRunInfo& runAfter);

	Fixed CalcFullJustifPortions(Fixed* portionRunArr, short* cFullJustifRuns);
	
	void 	DefineRunsExtraWidths(Fixed theSlope);
	#endif
	
	short CharToRun(TOffset charOffset, Fixed* runLeftEdge= nil);

	short PixelToRun(Fixed* hPix);

	short CalcRunHilite(short startOff, short endOff, short runVisIndex
											, Fixed* startPix, Fixed* pixWidth, Fixed* runLeft, Boolean lineBounds);
};

//**************************************************************************************************

