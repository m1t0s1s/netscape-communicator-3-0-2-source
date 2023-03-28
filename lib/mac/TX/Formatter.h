// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ


#pragma once

#include "Array.h"
#include "RunsRanges.h"

#ifdef txtnRulers
#include "RulersRanges.h"
#include "RulerObject.h"
#endif

#include "ObjectsRanges.h"//for CRanges
#include "StyledText.h"


//***************************************************************************************************

struct TLineInfo{
	long	lineEnd;
	TLineHeightInfo lineHeightInfo;
};
typedef	TLineInfo*	TLineInfoPtr;


struct TFormattingInfo {
	long lineIndex; //input/output
	long nextLineStart; //output
	long lastFormatChar; //input/output
};

//***************************************************************************************************


class CPageGeometry;
class CLineHeights;

class	CFormatter
{
	private:
							CFormatter();	// parameters required
	public:
							CFormatter(
								CStyledText* 		inStyledText,
								CPageGeometry* 		inGeometry,
								CLineHeights* 		inLineHeights);

	virtual					~CFormatter();
	
	void 					IFormatter();
	
	
	void FreeData(Boolean emptyLine =true);
	
	OSErr ReserveLines(long extraLines);
	
	void Compact();
	
	inline void DisableFormatting()  {--fFormatLevel;}
	inline void EnableFormatting() {++fFormatLevel;}
	
	inline long CountLines() const {return fLastLineIndex+1;}

	virtual	OSErr Format(long startChar = 0, long endChar = -1, long* firstLine = nil, long* lastLine = nil); 
	virtual OSErr ReplaceRange(long offset, long oldLen, long newLen
													, long* firstLine, long* lastLine);
	
	//void RecalcLinesHeight(long firstLine, long lastLine);
	
	inline void SetCRFlag(Boolean newFlag) {fCR = newFlag;}
	//"Format" has to be called after calling this meth, crOnly flag overrides the format width.
	inline Boolean GetCRFlag() {return fCR;}
	
	#ifdef txtnRulers
	inline void SetDirection(char newDir) {fDirection = newDir;}
	#endif
	
	Boolean IsLineFeed(long offset) const;
	
	//¥¥inlines to public some mLineEnds services
	inline long OffsetToLine(TOffset lineOffset) const
		{return mLineEnds->OffsetToRangeIndex(lineOffset);}

	inline long	GetLineStart(long lineNo) const {return mLineEnds->GetRangeStart(lineNo);}
	inline long	GetLineEnd(long lineNo) const {return mLineEnds->GetRangeEnd(lineNo);}
	void GetLineRange(long lineNo, TOffsetRange* range) const;
	
	inline Boolean IsLineStart(long offset, long lineNo = -1) const
		{return mLineEnds->IsRangeStart(offset, lineNo);}
	inline Boolean IsLineEmpty(long lineNo) const {return mLineEnds->IsRangeEmpty(lineNo);}
	
	#ifdef txtnDebug
	void CheckCoherence(long count);
	#endif
	
protected :
	CStyledText* 		mStyledText;
	CPageGeometry* 		mGeometry;
	CLineHeights* 		mLineHeights;

	CRunsRanges*  		mRunsRanges;
	CRanges* 			mLineEnds;


	#ifdef txtnRulers
	CRulersRanges*	fRulersRanges;
	#endif
	
	
	
	
	Boolean fCR;

	//note that newLineInfo is not constant, it returns the actual values (which may be modified by a descendent)
	virtual void SetLineInfo(TLineInfo* newLineInfo, TFormattingInfo* formatInfo);
	
	virtual OSErr	InsertLine(TLineInfo* newLineInfo, TFormattingInfo* formatInfo);

	virtual void BreakLine(long startBreak, short formattingWidth, TLineInfo* lineInfo);
	
	virtual short BreakVisibleChars(long lineStart, long runStart, short runLen, Fixed* widthAvail
																, CRunObject* runObj);

	virtual short BreakRun(long lineStart, long runStart, short runLen, Fixed* widthAvail
													, CRunObject* runObj);
	
	virtual short BreakCtrlChar(long lineOffset, long ctrlCharOffset, Fixed* widthAvail);
	
private:
	short fFormatLevel; //to disable formatting
	
	#ifdef txtnRulers
	CRulerObject* fParagraphRuler;
	#endif
	
	Fixed	fLineFormattingWidth;
	
	long fLastLineIndex;
	
	#ifdef txtnRulers
	TPendingTab	fPendingTab;
	Fixed	fTabMungedWidth;
	
	Fixed fLineStartBlanks;
	
	char fDirection;
	#endif
	
	
	Boolean AppendEmptyLine();
	OSErr	FormatRange(long startChar, long endChar, long* firstLine, long* lastLine);
	OSErr	FormatAll();
	
	void	RemoveLines(long firstLine, long count);
	long	RemoveFormattedLines(long lastFormattedLine);


	#ifdef txtnRulers
	short BreakAlignTabChars(long lineStart, long runStart, short runLen, Fixed* widthAvail
														, CRunObject* runObj);
	#endif
	
	void 	CalcCharsHeight(long charsOffset, short charsCount, TLineHeightInfo* lineHeightInfo) const;

//	//////////////
/*	jah	950130

	To allow variable line heights, call SetFixedLineHeight with the default parameters
	(the default).	*/
protected:
	short	fLineHeight;
	short	fLineAscent;
	
public:
	virtual	void		GetFixedLineHeight(short *outLineHeight, short *outLineAscent) const;
	virtual	OSErr		SetFixedLineHeight(short inLineHeight = 0, short inLineAscent = 0);
/*	\\\\\\\\\\\\\	*/
};




