//	===========================================================================
//	VStyleSet
//
//	Copyright 1996 ViviStar Consulting.
//	===========================================================================

#ifdef PowerPlant_PCH
#include PowerPlant_PCH
#endif

#include	"VStyleSet.h"
#include	<UFontsStyles.h>
#include	<AEVTextSuite.h>
#include	<LTextEngine.h>
#include	<LTextModel.h>
#include	<UAppleEventsMgr.h>
#include	<UAEGizmos.h>
#include	<UExtractFromAEDesc.h>
#include	<LString.h>
#include	<LStream.h>
#include	<UDrawingState.h>
#include	<WFixed.h>
#include	<PP_Messages.h>
#include	<LCommander.h>
#include	<PP_Resources.h>
#include	<StClasses.h>
#include	<UFontsStyles.h>
#include	<UMemoryMgr.h>

#ifndef		__FONTS__
#include	<Fonts.h>
#endif

#ifndef		__AEREGISTRY__
#include	<AERegistry.h>
#endif

#ifndef		__AEOBJECTS__
#include	<AEObjects.h>
#endif

#ifndef		__AEPACKOBJECT__
#include	<AEPackObject.h>
#endif

#ifndef		__AEREGISTRY__
#include	<AERegistry.h>
#endif

#ifndef		__GESTALT__
#include	<Gestalt.h>
#endif

#pragma	warn_unusedarg off

VStyleSet::VStyleSet()
{
	mFontNumber = applFont;
	mFontSize = 0;
	mFontFace = 0;
	mXferMode = srcCopy;
	RGBColor	black = {0, 0, 0};
	mFontColor = black;
	
	ChangedGeometry();
}

VStyleSet::VStyleSet(const LStyleSet *inOriginal)
:	LStyleSet(inOriginal)
{
	mFontNumber = applFont;
	mFontSize = 0;
	mFontFace = 0;
	mXferMode = srcCopy;
	RGBColor	black = {0, 0, 0};
	mFontColor = black;

	TextTraitsRecord	traits;
	
	inOriginal->GetTextTraits(&traits);
	SetTextTraits(traits);
}

VStyleSet::~VStyleSet()
{
}

Boolean	VStyleSet::operator==(const LStyleSet &inRhs) const
{
	if (inherited::operator==(inRhs))
		return true;

	const VStyleSet	*rhsP = dynamic_cast<const VStyleSet *>(&inRhs);
	if (rhsP) {
		const VStyleSet	&rhs(*rhsP);
		
		return	(mFontNumber == rhs.mFontNumber) &&
				(mFontSize == rhs.mFontSize) &&
				(mFontFace == rhs.mFontFace) &&
				(mXferMode == rhs.mXferMode) &&
				(mFontColor.red == rhs.mFontColor.red) &&
				(mFontColor.green == rhs.mFontColor.green) &&
				(mFontColor.blue == rhs.mFontColor.blue);
	}
	return false;
}

#ifndef	PP_No_ObjectStreaming

	void	VStyleSet::WriteObject(LStream &inStream) const
	{
		inStream << (Int16) 1;
		
		inStream << GetScript();
		inStream <<	(Int32)GetLanguage();	//	actually filler for MacOS 8 locale
		
		Str255	str;
		UFontsStyles::GetFontName(GetFontNumber(), str);
		inStream << str;
		
		FixedT	fixed;
		fixed.SetFromLong(GetFontSize());
		inStream << fixed.ToFixed();
		
		inStream << mFontColor.red;
		inStream << mFontColor.green;
		inStream << mFontColor.blue;
		
		inStream << mFontFace;
		inStream << mXferMode;

		inStream << dynamic_cast<LTextModel *>(GetSuperModel());
		inStream << (LStyleSet *)NULL;
	}

	void	VStyleSet::ReadObject(LStream &inStream)
	{
		Int16	version;
		
		inStream >> version;
		
		switch (version) {

			case 0:	//	text traits look up
			{
				ResIDT	id;
				inStream >> id;

				StResource		r('Txtr', id, true, true);
				StHandleLocker	lock(r);
				TextTraitsRecord	&record = *((TextTraitsRecord *)*((Handle)r));
				
				SetTextTraits(record);
				break;
			}
			
			case 1:	//	MacOS 7
			{
				Int16	aShort;
				Int32	aLong;
				
				inStream >> aShort;
				mScript = aShort;
				
				inStream >> aLong;
				mLanguage = aLong;
				
				Str255	str;
				inStream >> str;
				mFontNumber = UFontsStyles::GetFontId(str);
				
				Fixed	fixed;
				inStream >> fixed;
				FixedT	fix;
				fix.SetFromFixed(fixed);
				mFontSize = fix.ToLong();
				
				inStream >> mFontColor.red;
				inStream >> mFontColor.green;
				inStream >> mFontColor.blue;
				
				inStream >> mFontFace;
				inStream >> mXferMode;
				break;
			}
			
			default:
				Throw_(paramErr);
				break;
		}
		
		SetSuperModel(ReadStreamable<LTextModel *>()(inStream));
		
		LStyleSet	*parent = ReadStreamable<LStyleSet *>()(inStream);
		Assert_(!parent);	//	not yet implemented;
		
		ChangedGeometry();
	}

#endif

void	VStyleSet::GetImportantAEProperties(AERecord &outRecord) const
{
	static DescType	pList[] = {
								pName,
								pFont,
								pPointSize,
								pTextStyles,
								pColor,
								pScriptTag
	};
	LAEStream	stream;
	Int32		i,
				count = sizeof(pList) / sizeof(DescType);
	
	stream.OpenRecord();
	for (i = 0; i < count ; i++) {
		StAEDescriptor	desc;
		
		GetAEProperty(pList[i], UAppleEventsMgr::sAnyType, desc);
						
		stream.WriteKey(pList[i]);
		stream.WriteDesc(desc);
	}
	stream.CloseRecord();
	stream.Close(&outRecord);
}

void	VStyleSet::WriteFlavorRecord(
	FlavorType	inFlavor,
	LStream		&inStream) const
{
	switch (inFlavor) {

		case typeScrapStyles:
		{
			ScrpSTElement	record;
			
			record.scrpHeight = mStyleHeight;
			record.scrpAscent = mStyleAscent;
			record.scrpFont = mFontNumber;
			record.scrpFace = mFontFace;
			record.filler = 0;
			record.scrpSize = mFontSize;
			record.scrpColor = mFontColor;
			
//			inStream.WriteBlock(&record.scrpStartChar, sizeof(record.scrpStartChar));	//	not written by this routine
			inStream.WriteBlock(&record.scrpHeight, sizeof(record.scrpHeight));
			inStream.WriteBlock(&record.scrpAscent, sizeof(record.scrpAscent));
			inStream.WriteBlock(&record.scrpFont, sizeof(record.scrpFont));
			inStream.WriteBlock(&record.scrpFace, sizeof(record.scrpFace));
			inStream.WriteBlock(&record.filler, sizeof(record.filler));
			inStream.WriteBlock(&record.scrpSize, sizeof(record.scrpSize));
			inStream.WriteBlock(&record.scrpColor, sizeof(record.scrpColor));
			break;
			
		}

		case typeVStyleSets:
		{
			StAEDescriptor	desc;
			
			GetImportantAEProperties(desc.mDesc);
			
			inStream << desc.mDesc.descriptorType;
			inStream << desc.mDesc.dataHandle;
			break;
		}
		
		case typeVStyleSetRuns:
		default:
			Throw_(paramErr);
			break;
	}
}

void	VStyleSet::ReadFlavorRecord(
	FlavorType	inFlavor,
	LStream		&inStream)
{
	switch (inFlavor) {

		case typeScrapStyles:
		{
			ScrpSTElement	record;
			
//			inStream.ReadBlock(&record.scrpStartChar, sizeof(record.scrpStartChar));	//	not read by this routine
			inStream.ReadBlock(&record.scrpHeight, sizeof(record.scrpHeight));
			inStream.ReadBlock(&record.scrpAscent, sizeof(record.scrpAscent));
			inStream.ReadBlock(&record.scrpFont, sizeof(record.scrpFont));
			inStream.ReadBlock(&record.scrpFace, sizeof(record.scrpFace));
			inStream.ReadBlock(&record.filler, sizeof(record.filler));
			inStream.ReadBlock(&record.scrpSize, sizeof(record.scrpSize));
			inStream.ReadBlock(&record.scrpColor, sizeof(record.scrpColor));

			mStyleHeight = record.scrpHeight;
			mStyleAscent = record.scrpAscent;
			mFontNumber = record.scrpFont;
			mFontFace = record.scrpFace;
			mFontSize = record.scrpSize;
			mFontColor = record.scrpColor;
			
			ChangedGeometry();
			break;
		}

		case typeVStyleSets:
		{
			StAEDescriptor	desc;
			
			inStream >> desc.mDesc.descriptorType;
			inStream >> desc.mDesc.dataHandle;
			
			LAESubDesc	sd(desc),
						item;
			Int32		count = sd.CountItems(),
						i;
		
			for (i = 1; i <= count; i++) {
				DescType		keyword;
				LAESubDesc		item = sd.NthItem(i, &keyword);
				StAEDescriptor	value,
								bogus;
				
				if (keyword == pScriptTag)
					continue;
					
				item.ToDesc(&value.mDesc);
				SetAEProperty(keyword, value, bogus);
			}
			break;
		}
		
		case typeVStyleSetRuns:
		default:
			Throw_(paramErr);
			break;
	}
}

//	----------------------------------------------------------------------
//	Query:

FixedT	VStyleSet::Insertion2Pixel(
	const TextRangeT	&inQueryRange,
	const Ptr			inText,
	const TextRangeT	&inRangePt,
	StyleRunPositionT	inPosition,
	FixedT				inSlop) const
{
	Assert_(inQueryRange.Contains(inRangePt));
	Assert_(!inRangePt.Length());

	Int32		offset = inRangePt.Start();

	short		direction;

	#if	0
		{
			//	figure direction parameter...
			if (offset == inQueryRange.Start()) {
				direction = CharIsLeftToRight(inText + offset)
						?	leftCaret
						:	rightCaret;
			} else {
				Int32	offsetBefore = offset - PrevCharSize(inText + inQueryRange.Start(), offset - inQueryRange.Start());
				Boolean	beforeIsLeft = CharIsLeftToRight(inText + offsetBefore),
						afterIsLeft = CharIsLeftToRight(inText + offset);
				
				if (beforeIsLeft == afterIsLeft) {
					direction = beforeIsLeft
						?	leftCaret
						:	rightCaret;
				} else {
					if (inRangePt.Insertion() == rangeCode_Before) {
						direction = beforeIsLeft
							?	leftCaret
							:	rightCaret;
					} else if (inRangePt.Insertion() == rangeCode_After) {
						direction = afterIsLeft
							?	leftCaret
							:	rightCaret;
					} else {
						Throw_(paramErr);
					}
				}
			}
		}
	#else
		{
			TextRangeT	range = inRangePt;
			
			//	figure direction parameter...
			if (offset == inQueryRange.Start()) {
				range.SetInsertion(rangeCode_After);
				direction = (CharDirection(inQueryRange, inText, range) == textDirection_LeftToRight)
						?	leftCaret
						:	rightCaret;
			} else if (offset == inQueryRange.End()) {
				range.SetInsertion(rangeCode_Before);
				direction = (CharDirection(inQueryRange, inText, range) == textDirection_LeftToRight)
						?	leftCaret
						:	rightCaret;
			} else {
				range.SetInsertion(rangeCode_Before);
				Boolean	beforeIsLeft = (CharDirection(inQueryRange, inText, range) == textDirection_LeftToRight);
				range.SetInsertion(rangeCode_After);
				Boolean	afterIsLeft = (CharDirection(inQueryRange, inText, range) == textDirection_LeftToRight);
				
				if (beforeIsLeft == afterIsLeft) {
					direction = beforeIsLeft
						?	leftCaret
						:	rightCaret;
				} else {
					if (inRangePt.Insertion() == rangeCode_Before) {
						direction = beforeIsLeft
							?	leftCaret
							:	rightCaret;
					} else if (inRangePt.Insertion() == rangeCode_After) {
						direction = afterIsLeft
							?	leftCaret
							:	rightCaret;
					} else {
						Throw_(paramErr);
					}
				}
			}
		}
	#endif
	
	offset -= inQueryRange.Start();
	
	Focus();

	Int16	pix =	CharToPixel(
						inText + inQueryRange.Start(),	//	textBuf
						inQueryRange.Length(),			//	textLen
						inSlop.ToFixed(),				//	slop
						offset,							//	offset
						direction,						//	direction
						inPosition,						//	styleRunPosition
						UFontsStyles::sUnitScale,		//	numer
						UFontsStyles::sUnitScale		//	denom
					);
	
	return FixedT(pix);
}

void	VStyleSet::Pixel2Range(
	const TextRangeT	&inQueryRange,
	const Ptr			inText,
	FixedT				inPixelOffset,
	StyleRunPositionT	inPosition,
	FixedT				inSlop,
	TextRangeT			&outRange,
	Boolean				&outLeadingEdge
) const
{
	Boolean		leadingEdge;
	Fixed		widthRemaining;
	Int32		offset;
	TextRangeT	range = inQueryRange;
	
	Focus();
	
	/*
		This "fix" is tenuous at best.  If inPixelOffset is zero,
		::PixelToChar will return an undesired result.  To see this, remove this fix
		and, on a mixed direction line, slowly drag accross style boundaries and
		the cursor will jump to the tail of a style if the mouse is on the very first
		pixel of the run.
	*/
	if (inPixelOffset <= FixedT::sZero)	
		inPixelOffset = FixedT::sOne;
		
	offset = ::PixelToChar(
					inText + range.Start(),				//	textBuf
					range.Length(),						//	textLen
					inSlop.ToFixed(),					//	slop
					inPixelOffset.ToFixed(),			//	pixelWidth
					&leadingEdge,						//	leadingEdge
					&widthRemaining,					//	widthRemaining
					inPosition,							//	styleRunPosition
					UFontsStyles::sUnitScale,			//	numer
					UFontsStyles::sUnitScale			//	denom
	);
	
//?	This is a sensible check but widthRemaining doesn't appear to always be zero
//	when the run is the only run
//+	Assert_(widthRemaining < 0);
	
	if (leadingEdge) {
//?		The offset < range.Length() assert doesn't alway appear to work correctly -- fixed by crop below
//+		Assert_((0 <= offset) && (offset < range.Length()));
		Assert_((0 <= offset) && (offset <= range.Length()));	//-

		outRange.SetStart(range.Start() + offset);
		if (offset < inQueryRange.Length()) {
			TextRangeT	tRange(range.Start());
			tRange.SetInsertion(rangeCode_After);
			outRange.SetLength(FindCharSize(inQueryRange, inText, tRange));
		} else {
			outRange.SetInsertion(rangeCode_After);
		}
	} else {
//?		The 0 < offset assert doesn't alway appear to work correctly -- fixed by crop below
//+		Assert_((0 < offset) && (offset <= range.Length()));
		Assert_((0 <= offset) && (offset <= range.Length()));	//-
		
		if (offset > 0) {
			TextRangeT	tRange(inQueryRange.Start() + offset);
			tRange.SetInsertion(rangeCode_Before);
			Int32	prevSize = FindCharSize(inQueryRange, inText, tRange);
			outRange.SetStart(range.Start() + offset - prevSize);
			outRange.SetLength(prevSize);
		} else {
			outRange = inQueryRange;
			outRange.Front();
		}
	}
	outLeadingEdge = leadingEdge;
	
//?	Assert_(range.WeakContains(outRange));
	outRange.WeakCrop(range);
}

Int32	VStyleSet::GetHeight(void) const
{
	return mStyleHeight;
}

Int32	VStyleSet::GetAscent(void) const
{
	return mStyleAscent;
}

Boolean	VStyleSet::CaretInside(void) const
/*
	Can the caret "go inside" this style set.
*/
{
	return true;
}

Boolean	VStyleSet::IsProportional(void) const
{
	return mIsProportional;
}

/*-
FixedT	VStyleSet::MaxCharWidth(void) const
{
	return mMaxCharWidth;
}
*/

Int32	VStyleSet::FindCharSize(
	const TextRangeT	&inStyleRange,
	const Ptr			inText,
	const TextRangeT	&inInsertionPt) const
{
	Assert_(inStyleRange.Length());
	Assert_(!inInsertionPt.Length());
	Assert_(inStyleRange.Contains(inInsertionPt));
	
	if (mCharSize)
		return mCharSize;

	if (inInsertionPt.Insertion() == rangeCode_After) {
		short	byteType = ::CharacterByteType(inText + inInsertionPt.Start(), 0,  GetScript());
		switch (byteType) {

			case smSingleByte:
				return 1;

			case smFirstByte:
				return 2;

			case smLastByte:
			case smMiddleByte:
			default:
				Throw_(paramErr);	//	usage must insure FindCharSize is called for
									//	inInsertionPt's that fall on the start of a
									//	character.
				break;
		}
	} else {
		Int32	offset = inInsertionPt.Start() - inStyleRange.Start() -1;

		ThrowIf_(offset > max_Int16);	//	crop inStyleRange (for efficiency reasons too)
		
		short	byteType = ::CharacterByteType(inText + inStyleRange.Start(), offset, GetScript());
		switch (byteType) {

			case smSingleByte:
				return 1;

			case smLastByte:
				return 2;

			case smFirstByte:
			case smMiddleByte:
			default:
				Throw_(paramErr);	//	usage must insure FindCharSize is called for
									//	inInsertionPt's that fall on boundaries of
									//	characters.
				break;
		}
	}
	
	Throw_(paramErr);
	return 0;
}

TextDirectionE	VStyleSet::CharDirection(
	const TextRangeT	&inStyleRange,
	const Ptr			inText,
	const TextRangeT	&inInsertionPt) const
{
	Assert_(inStyleRange.Length());
	Assert_(!inInsertionPt.Length());
	Assert_(inStyleRange.Contains(inInsertionPt));
	
	TextDirectionE	rval = StyleDirection();
	if (rval == textDirection_Undefined) {
		short type;
		if (inInsertionPt.Insertion() == rangeCode_After) {
			type = ::CharacterType(inText + inInsertionPt.Start(), 0, GetScript());
		} else {
			Int32	offset = inInsertionPt.Start() - inStyleRange.Start() -1;

			ThrowIf_(offset > max_Int16);	//	crop inStyleRange (for efficiency reasons too)

			type = ::CharacterType(inText + inStyleRange.Start(), offset, GetScript());
		}
		if (type & smCharRight)
			rval = textDirection_RightToLeft;
		else
			rval = textDirection_LeftToRight;
	}
	
	return rval;
}

TextDirectionE	VStyleSet::StyleDirection(void) const
{
	return mDirection;
}

ScriptCode	VStyleSet::GetScript(void) const
{
	return mScript;
}

LangCode	VStyleSet::GetLanguage(void) const
{
	return mLanguage;
}

Int32	VStyleSet::GetFontNumber(void) const
{
	return mFontNumber;
}

Int32	VStyleSet::GetFontSize(void) const
{
	return mFontSize;
}

Int32	VStyleSet::GetFontFace(void) const
{
	return mFontFace;
}

//	----------------------------------------------------------------------
//	Formatting:

void	VStyleSet::LineBreakScan(
	const TextRangeT	&inLineRange,		//	line start to paragraph end
	const TextRangeT	&inScriptRange,		//	range of adjacent/equivalent styles in inLineRange
	const TextRangeT	&inStyleRange,		//	range corresponding to this styleset -- looking
											//		to see if the line break occurs in this range.
	const Ptr			inText,				//	pointer to all of text
	FixedT				inWidth,			//	width remaining on line
	TextRangeT			&outRange,			//	range of inStyleRange that occurs on line
	FixedT				&outWidthUnused,	//	width left over after scan inStyleRange
	StyledLineBreakCode	&outBreakCode		//	what happened
) const
{
//?	if (IsProportional())
//?		Assert_(false);	//	Use some arithmetic instead Style LineBreak
//?						//	but wait -- we'd still have to worry about word breaks
	Fixed	width = inWidth.ToFixed();

	Focus();
	
	long	offset = inLineRange.Start() == inStyleRange.Start();	//	first time?
	
	outBreakCode = ::StyledLineBreak(
						inText + inScriptRange.Start(),					//	textPtr	(script run)
						inScriptRange.Length(),							//	textLen (script run)
						inStyleRange.Start() - inScriptRange.Start(),	//	textStart (style run)
						inStyleRange.End() - inScriptRange.Start(),		//	textEnd (style run)
						0,												//	flags (unused/reserved 0)
						&width,											//	textWidth
						&offset);										//	textOffset (if broke, where)
	
	//	Build outRange
	if (offset > (inStyleRange.Start() - inScriptRange.Start()) ) {
		outRange.SetStart(inStyleRange.Start());
		outRange.SetLength(offset - (inStyleRange.Start() - inScriptRange.Start()));
/*?
if ((inScriptRange.End() > outRange.End()) && (*(inText + outRange.End()) == 0x0d))
	outRange.SetLength(outRange.Length() +1);
*/
		if (
			(width < 0) ||
			(outRange.End() == inLineRange.End()) ||
			(outBreakCode != smBreakOverflow)
		) {

			//	Why did StyledLineBreak really need to set this to -1?  Now
			//	we have to waist time and find out again what it already
			//	knew but decided to throw way.  Wasn't the break code sufficient
			//	to indicate the stop?
			outWidthUnused = inWidth;
			outWidthUnused -= TextWidth(inText + inStyleRange.Start(), outRange.Length());

		} else {

			outWidthUnused = WFixed::Fix2FixedT(width);

		}
	} else {
		//	Previous scan was line ender or this is a line end
//		Assert_(inStyleRange.Start() != inLineRange.Start());	//	can't be first scan
		outRange = Range32T::sUndefined;
		outWidthUnused = inWidth;
	}
}		

Int32	VStyleSet::VisibleLength(
	const Ptr			inText,
	Int32				inLength) const
{
	Focus();
	
	do {
		short	type = ::CharacterType(inText, inLength -1, mScript);
		if ( (type & (smcClassMask | smcTypeMask)) == smPunctBlank )
			inLength--;
		else
			break;
	} while (inLength > 0);
	
	return inLength;
}

FixedT	VStyleSet::TextWidth(
	const Ptr			inText,
	Int32				inLength) const
{
	Focus();
	
	Assert_(inLength < max_Int16);
	
	FixedT	rval = ::TextWidth(inText, 0, inLength);

	return rval;
}

FixedT	VStyleSet::Portion(
	const Ptr				inText,
	Int32					inVisibleLength,
	const StyleRunPositionT	&inPosition) const
{
	Focus();
	
	FixedT	rval;
	Fixed	f = ::PortionLine(
					inText,
					inVisibleLength,
					inPosition,
					UFontsStyles::sUnitScale,
					UFontsStyles::sUnitScale
				);

	rval.SetFromFixed(f);

	Assert_(rval >= FixedT::sZero);

	return rval;
}

void	VStyleSet::Draw(
	const TextRangeT	&inRange,
	const Ptr			inText,
	StyleRunPositionT	inPosition,
	FixedT				inSlop) const
{
	Focus();
	
//	StPenMove	change(0, mStyleHeight - mStyleAscent);
	
	if (inSlop.ToFixed()) {
		DrawJustified(inText + inRange.Start(), inRange.Length(), inSlop.ToFixed(), inPosition, UFontsStyles::sUnitScale, UFontsStyles::sUnitScale);
	} else {
		DrawText(inText + inRange.Start(), 0, inRange.Length());
	}
}

//	----------------------------------------------------------------------
//	Implementation:

void	VStyleSet::Focus(void) const
{
	GrafPtr	gPort = UQDGlobals::GetCurrentPort();
	
	ThrowIfNULL_(gPort);
	
	if (gPort->txFont != mFontNumber)
		TextFont(mFontNumber);
	if (gPort->txSize != mFontSize)
		TextSize(mFontSize);
	if (gPort->txFace != mFontFace)
		TextFace(mFontFace);
	if (gPort->txMode != mXferMode)
		TextMode(mXferMode);
	
	RGBColor	foreColor;
	GetForeColor(&foreColor);
	if (	foreColor.red != mFontColor.red
		||	foreColor.green != mFontColor.green
		||	foreColor.blue != mFontColor.blue
	)
		RGBForeColor(&mFontColor);
}

/*-
Boolean	VStyleSet::FindCommandStatus(
	CommandT	inCommand,
	Boolean		&outEnabled,
	Boolean		&outUsesMark,
	Char16		&outMark,
	Str255		outName)
{
	Boolean	handled = false;
	Int16	menuNo,
			itemNo;
	
	if (LCommander::IsSyntheticCommand(inCommand, menuNo, itemNo)) {
		if (menuNo == MENU_Font) {
			UFontMenu::AdjustMenu(UFontMenu::GetFontItemNumber(mFontNumber));
			outEnabled = true;
			handled = true;
		} else if (menuNo == MENU_Size) {
			USizeMenu::AdjustMenu(itemNo, mFontSize, mFontNumber, outEnabled, outUsesMark, outMark);
			outEnabled = true;
			handled = true;
		}
	} else {

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
				UStyleMenu::EnableMenu();
				outEnabled = true;
				if (inCommand == cmd_Plain) {
					outMark = (mFontFace == normal)
									?	checkMark
									:	noMark;
				} else {
					Int32	bit = inCommand - cmd_Bold;
					bit = 0x1L << bit;
					outMark = (bit & mFontFace)
									?	checkMark
									:	noMark;
				}
				outUsesMark = true;
				handled = true;
				break;
			}

			case cmd_FontLarger:
			case cmd_FontSmaller:
			case cmd_FontOther:
				outEnabled = true;
				outMark = noMark;
				handled = true;
				break;

			default:
				break;
		}
	}
	
	return handled;
}
*/

//	----------------------------------------------------------------------
//	AEOM:

void	VStyleSet::ModifyProperty(
	DescType			inProperty,
	const LAESubDesc	&inByValue
)
{
	switch (inProperty) {

		case pFont:
			Throw_(errAEEventNotHandled);	//	only set name works
			break;

		case pPointSize:
			mFontSize += inByValue.ToInt16();
			ChangedGeometry();
			break;

		case pTextStyles:
			switch (inByValue.GetType()) {

				case typeEnumeration:
					mFontFace ^= UFontsStyles::EnumToFace(inByValue.ToEnum());
					ChangedGeometry();
					break;
					
				case typeAEList:
					mFontFace ^= UFontsStyles::StylesToInt(inByValue);
					ChangedGeometry();
					break;
					
				case typeTextStyles:
				{
					Int16	offFaces = 0,
							onFaces = 0;
							
					LAESubDesc	subList(inByValue.KeyedItem(keyAEOffStyles));
					
					if (subList.GetType() == typeAEList)
						offFaces = UFontsStyles::StylesToInt(subList);
					else if (subList.GetType() != typeNull)
						Throw_(paramErr);
					
					subList = inByValue.KeyedItem(keyAEOnStyles);
					if (subList.GetType() == typeAEList)
						onFaces = UFontsStyles::StylesToInt(subList);
					else if (subList.GetType() != typeNull)
						Throw_(paramErr);
					
					mFontFace &= ~offFaces;
					mFontFace |= onFaces;
					ChangedGeometry();
					break;
				}
				
				default:
					Throw_(paramErr);
					break;
			}
			break;

		case pColor:
		{
			RGBColor	addColor = inByValue.ToRGBColor();

			mFontColor.red += addColor.red;
			mFontColor.green += addColor.green;
			mFontColor.blue += addColor.blue;
			Changed();
			break;
		}

		case pScriptTag:
			Throw_(errAENotModifiable);
			break;

	}
}

//-	copied?
#include	<LTextModel.h>

//-	copied?
void	VStyleSet::Changed(void)
{
	LModelObject	*superModel = GetSuperModel();
	LTextModel		*textModel;
	LTextEngine		*engine;
	
	if (!superModel)
		return;
		
	textModel = (LTextModel *) superModel;
//	Assert_(member(textModel, LTextModel));
	engine = textModel->GetTextEngine();
	if (engine) {
//		Assert_(member(engine, LTextEngine));
		engine->StyleSetChanged(this);
	}
}

void	VStyleSet::ChangedGeometry(void)
{
	mScript = FontToScript(mFontNumber);
	mLanguage = GetScriptVariable(GetScript(), smScriptLang);
	
	if (GetScriptVariable(GetScript(), smScriptRight) == 0xff)
		mDirection = textDirection_Undefined;
	else
		mDirection = textDirection_LeftToRight;

	if (GetScriptVariable(GetScript(), smScriptFlags) & (1L << smsfSingByte))
		mCharSize = 1;
	else
		mCharSize = 0;	//	ie. look at the text
	
	if (!mFontSize)
		mFontSize = 12;	//	the system font size
	
	Focus();

	FMetricRec	metrics;
	FontMetrics(&metrics);
	
	FixedT		height = WFixed::Fix2FixedT(metrics.ascent + metrics.descent + metrics.leading),
				ascent = WFixed::Fix2FixedT(metrics.ascent);
	mStyleHeight = height;
	mStyleAscent = ascent;
	
	mMaxCharWidth = metrics.widMax;
	mIsProportional = true;	//	how do you determine a mono space font?
	
	Changed();
}

void	VStyleSet::GetAEProperty(
	DescType		inProperty,
	const AEDesc	&inRequestedType,
	AEDesc			&outPropertyDesc) const
{
	OSErr	err;
	
	switch (inProperty) {

		case pFont:
		{
			Str255	name;
			GetFontName(mFontNumber, name);
			err = AECreateDesc(typeChar, name +1, name[0], &outPropertyDesc);
			ThrowIfOSErr_(err);
			break;
		}

		case pPointSize:
			err = AECreateDesc(typeShortInteger, &mFontSize, sizeof mFontSize, &outPropertyDesc);
			ThrowIfOSErr_(err);
			break;

		case pTextStyles:
			UFontsStyles::IntToStyles(mFontFace, &outPropertyDesc);
			break;

		case pColor:
			err = AECreateDesc(cRGBColor, &mFontColor, sizeof mFontColor, &outPropertyDesc);
			ThrowIfOSErr_(err);
			break;

		case pScriptTag:
			err = AECreateDesc(typeIntlWritingCode, &mScript, sizeof mScript, &outPropertyDesc);
			ThrowIfOSErr_(err);
			break;

		default:
			inherited::GetAEProperty(inProperty, inRequestedType, outPropertyDesc);
			break;
	}
}

void	VStyleSet::SetAEProperty(
	DescType		inProperty,
	const AEDesc	&inValue,
	AEDesc			&outAEReply)
{
	LAESubDesc	value(inValue);
	
	switch (inProperty) {

		case pFont:
		{
			Str255	name;
			value.ToPString(name);
			mFontNumber = UFontsStyles::GetFontId(name);
			ChangedGeometry();
			break;
		}

		case pPointSize:
			mFontSize = value.ToInt16();
			ChangedGeometry();
			break;

		case pTextStyles:
			switch (value.GetType()) {

				case typeEnumeration:
					mFontFace = UFontsStyles::EnumToFace(value.ToEnum());
					break;
					
				case typeAEList:
					mFontFace = UFontsStyles::StylesToInt(value);
					break;
					
				case typeTextStyles:
				{
					LAESubDesc	subList(value.KeyedItem(keyAEOffStyles));
					
					if (subList.GetType() == typeAEList)
						mFontFace = mFontFace & ~UFontsStyles::StylesToInt(subList);
					else if (subList.GetType() != typeNull)
						Throw_(paramErr);
					
					subList = value.KeyedItem(keyAEOnStyles);
					if (subList.GetType() == typeAEList)
						mFontFace |= UFontsStyles::StylesToInt(subList);
					else if (subList.GetType() != typeNull)
						Throw_(paramErr);
					
					ChangedGeometry();
					break;
				}
				
				default:
					Throw_(paramErr);
					break;
			}
			ChangedGeometry();
			break;

		case pColor:
			mFontColor = value.ToRGBColor();
			Changed();
			break;

		case pScriptTag:
			Throw_(errAENotModifiable);
			break;

		default:
			inherited::SetAEProperty(inProperty, inValue, outAEReply);
			break;
	}
}

