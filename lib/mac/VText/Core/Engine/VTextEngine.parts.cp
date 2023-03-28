//	===========================================================================
//	VTextEngine.parts
//
//	Copyright 1996 ViviStar Consulting.
//	===========================================================================

#ifdef PowerPlant_PCH
#include PowerPlant_PCH
#endif

#include	"VTextEngine.h"
#include	<VPartCache.h>
#include	<UMemoryMgr.h>
#include	<VCharScanner.h>

#ifndef		__AEREGISTRY__
#include	<AERegistry.h>
#endif

#pragma	warn_unusedarg off

DescType	VTextEngine::FindPart(
	const TextRangeT	&inQueryRange,
	TextRangeT			*ioSubRange,
	DescType			inSubPartType,
	Int32				*outNextOffset) const
{
	TextRangeT	queryRange = inQueryRange;
	DescType	rval = typeNull;	
	
	queryRange.Crop(GetTotalRange());
	
	if (queryRange.Contains(*ioSubRange)) {

		if (	!ioSubRange->Length() &&
				(
						((inSubPartType == cLine) && mLines)
					||	((inSubPartType == cParagraph) && mParas)
				)
		) {
	
			switch (inSubPartType) {
			
				case cParagraph:
					rval = FindPara(queryRange, ioSubRange, outNextOffset);
					break;
				case cLine:
					rval = FindLine(queryRange, ioSubRange, outNextOffset);
					break;
	
			}
			
		} else {
	
			rval = inherited::FindPart(inQueryRange, ioSubRange, inSubPartType, outNextOffset);
	
		}
		
	}
	
	return rval;
}

Int32	VTextEngine::CountLines(const TextRangeT &inQueryRange) const
{
VTextEngine	&my = (VTextEngine &)(*this);
my.Recalc();	//	make sure mLines are up to date

	ThrowIfNULL_(mLines);
	
	TextRangeT	range = inQueryRange;
	Range32T	partIndices;
	
	range.Crop(GetTotalRange());
	mLines->Range2Index(range, &partIndices);
	
	return partIndices.Length();
}

DescType	VTextEngine::FindNthLine(
	const TextRangeT	&inQueryRange,
	Int32				inPartIndex,
	TextRangeT			*outRange) const
{
VTextEngine	&my = (VTextEngine &)(*this);
my.Recalc();	//	make sure mLines are up to date

	ThrowIfNULL_(mLines);
	
	Range32T	queryIndices,
				index;
	TextRangeT	range;
	DescType	rval = typeNull;
	
	mLines->Range2Index(inQueryRange, &queryIndices);
	index.SetStart(queryIndices.Start() + inPartIndex -1);
	index.SetLength(1);
	mLines->Index2Range(index, &range);
	
	if (!range.Undefined()) {
		*outRange = range;
		outRange->Crop(inQueryRange);
	
		if (*outRange == range)
			rval = cLine;
		else
			rval = cChar;
	}
		
	return rval;
}

DescType	VTextEngine::FindLineSubRangeIndices(
	const TextRangeT	&inQueryRange,
	const TextRangeT	&inSubRange,
	Int32				*outIndex1,
	Int32				*outIndex2) const
{
VTextEngine	&my = (VTextEngine &)(*this);
my.Recalc();	//	make sure mLines are up to date

	ThrowIfNULL_(mLines);
	
	DescType	rval = typeNull;
	
	if (inQueryRange.Contains(inSubRange)) {
		Range32T	queryIndices,
					partIndices;
		TextRangeT	subRange;
	
		mLines->Range2Index(inQueryRange, &queryIndices);
		if (mLines->Range2Index(inSubRange, &partIndices))
			rval = cLine;
		else
			rval = cChar;
		
		*outIndex1 = partIndices.Start() - queryIndices.Start() +1;
		*outIndex2 = partIndices.End() - queryIndices.Start();
	}
	return rval;
}

DescType	VTextEngine::FindLine(
	const TextRangeT	&inQueryRange,
	TextRangeT			*ioRange,
	Int32				*outEndOffset) const
{
	DescType	rval = typeNull;
	
	ThrowIfNULL_(mLines);
	
	if (inQueryRange.Contains(*ioRange)) {
		TextRangeT	range = *ioRange;
		Range32T	index;
		
		if (!mLines->Range2Index(range, &index))
			mLines->Index2Range(index, &range);
		
		if (!range.Undefined()) {
			*ioRange = range;
			ioRange->Crop(inQueryRange);
			
			if (*ioRange == range)
				rval = cLine;
			else
				rval = cChar;
		}
	}
	
	return rval;
}

Int32	VTextEngine::CountParas(const TextRangeT &inQueryRange) const
{	
	Int32		rval;

	if (mParas) {
		TextRangeT	range = inQueryRange;
		Range32T	partIndices;
		
		range.Crop(GetTotalRange());
		mParas->Range2Index(range, &partIndices);
		rval = partIndices.Length();
	} else {
		rval = inherited::CountParas(inQueryRange);
	}

	return rval;
}

DescType	VTextEngine::FindPara_(
	const TextRangeT	&inQueryRange,
	TextRangeT			*ioRange,
	Int32				*outEndOffset) const
{
	ThrowIfNULL_(ioRange);

	TextRangeT		range = *ioRange,
					rangeFound,
					rangeReported;

	if (!inQueryRange.Contains(range))
		return typeNull;

	VCharScanner	scanner(*this, inQueryRange, range);
	scanner.FindPrev(0x0d);
	rangeFound = TextRangeT(scanner.PresentRange().End());
	scanner.Reset(range);
	scanner.FindNext(0x0d);
	rangeFound.SetEnd(scanner.PresentRange().End());
	
	if (rangeFound.Length() <= 0)
		return typeNull;
	
	rangeReported = rangeFound;
	rangeReported.Intersection(inQueryRange);
	if (rangeReported.Undefined())
		return typeNull;
		
	*ioRange = rangeReported;
	
	if (outEndOffset)
		*outEndOffset = rangeReported.End();
	
	if (rangeFound == rangeReported)
		return cParagraph;
	else
		return cChar;
}

DescType	VTextEngine::FindNthPara(
	const TextRangeT	&inQueryRange,
	Int32				inPartIndex,
	TextRangeT			*outRange) const
{
	ThrowIfNULL_(mParas);

	Range32T	queryIndices,
				index;
	TextRangeT	range;
	DescType	rval = typeNull;
	
	mParas->Range2Index(inQueryRange, &queryIndices);
	index.SetStart(queryIndices.Start() + inPartIndex -1);
	index.SetLength(1);
	mParas->Index2Range(index, &range);
	
	if (!range.Undefined()) {
		*outRange = range;
		outRange->Crop(inQueryRange);
	
		if (*outRange == range)
			rval = cParagraph;
		else
			rval = cChar;
	}
	
	return rval;
}

DescType	VTextEngine::FindParaSubRangeIndices(
	const TextRangeT	&inQueryRange,
	const TextRangeT	&inSubRange,
	Int32				*outIndex1,
	Int32				*outIndex2) const
{
	ThrowIfNULL_(mParas);
	
	DescType	rval = typeNull;
	
	if (inQueryRange.Contains(inSubRange)) {
		Range32T	queryIndices,
					partIndices;
		TextRangeT	subRange;
	
		mParas->Range2Index(inQueryRange, &queryIndices);
		if (mParas->Range2Index(inSubRange, &partIndices))
			rval = cParagraph;
		else
			rval = cChar;
		
		*outIndex1 = partIndices.Start() - queryIndices.Start() +1;
		*outIndex2 = partIndices.End() - queryIndices.Start();
	}
	return rval;
}

DescType	VTextEngine::FindPara(
	const TextRangeT	&inQueryRange,
	TextRangeT			*ioRange,
	Int32				*outEndOffset) const
{
	DescType	rval = typeNull;
	
	ThrowIfNULL_(mParas);
	
	if (inQueryRange.Contains(*ioRange)) {
		TextRangeT	range = *ioRange;
		Range32T	index;
		
		if (!mParas->Range2Index(range, &index))
			mParas->Index2Range(index, &range);
		
		if (!range.Undefined()) {
			*ioRange = range;
			ioRange->Crop(inQueryRange);
			
			if (*ioRange == range)
				rval = cParagraph;
			else
				rval = cChar;
		}
	}
	
	return rval;
}

DescType	VTextEngine::FindWord(
	const TextRangeT	&inQueryRange,
	TextRangeT			*ioRange,
	Int32				*outEndOffset) const
{
	DescType	rval = typeNull;
	
	Assert_(!(ioRange->Length()));
	
	if (inQueryRange.Contains(*ioRange)) {
		TextRangeT		range = *ioRange;
		range.Crop(GetTotalRange());
		StHandleLocker	lock(mText);
		OffsetTable		offsets;
		TextRangeT		scanRange,
						paraRange = range;
		Range32T		paraIndex;
		
		if (!mParas->Range2Index(paraRange, &paraIndex))
			mParas->Index2Range(paraIndex, &paraRange);
		ScriptCode		script = GetScriptRange(*ioRange, scanRange);
		scanRange.Crop(paraRange);	//	restrict search to enclosing paragraphs
		Int32			offset = ioRange->Start() - scanRange.Start();
		Boolean			leadingEdge = (ioRange->Insertion() == rangeCode_After);

//?		if (!leadingEdge)
//			offset -= PrevCharSize(offset);

		FindWordBreaks(
			*mText + scanRange.Start(),	//	textPtr
			scanRange.Length(),			//	textLength
			offset,							//	offset
			leadingEdge,					//	leadingEdge
			(BreakTablePtr) 0,				//	nbreaks
			offsets,						//	offsets
			script							//	script
		);
		
		TextRangeT	wordRange(
						offsets[0].offFirst + scanRange.Start(),
						offsets[0].offSecond + scanRange.Start()
		);
		*ioRange = wordRange;
		ioRange->Crop(inQueryRange);
		
		if (outEndOffset)
			*outEndOffset = ioRange->End();
		
		if (*ioRange == wordRange)
			rval = cWord;
		else
			rval = cChar;
	}
	
	return rval;
}

DescType	VTextEngine::FindCharSubRangeIndices(
	const TextRangeT	&inQueryRange,
	const TextRangeT	&inSubRange,
	Int32				*outIndex1,
	Int32				*outIndex2) const
{
//-	return inherited::FindCharSubRangeIndices(inQueryRange, inSubRange, outIndex1, outIndex2);

	//	inQueryRange and inSubRange are assummed to be precropped
	Assert_(GetTotalRange().WeakContains(inQueryRange));
	Assert_(GetTotalRange().WeakContains(inSubRange));
	Assert_(inQueryRange.WeakContains(inSubRange));
	
	if (!inQueryRange.Contains(inSubRange))
		return typeNull;
	
	Int32			i1 = 0,
					i2 = 0;
	TextRangeT		scanRange(inQueryRange.Start(), inSubRange.Start()),
					range(scanRange.Start(), 0, rangeCode_After);
	VCharScanner	scanner(*this, scanRange, range);
	while (true) {
		scanner.Next();
		
		if (scanner.Finished())
			break;

		i1++;
	}
	
	scanRange = inSubRange;
	range = TextRangeT(scanRange.Start(), 0, rangeCode_After);
	scanner.Reset(scanRange, range);
	while (true) {
		scanner.Next();
		
		if (scanner.Finished())
			break;
		
		i2++;
	}
	
	ThrowIfNULL_(outIndex1);
	ThrowIfNULL_(outIndex2);
	
	*outIndex1 = i1 + 1;
	
	if (i2 > 0)
		*outIndex2 = i1 + i2;
		
	return cChar;
}
