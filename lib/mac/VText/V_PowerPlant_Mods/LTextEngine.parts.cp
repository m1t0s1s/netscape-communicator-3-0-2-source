//	===========================================================================
//	LTextEngine.cp				©1994 Metrowerks Inc. All rights reserved.
//	===========================================================================

#ifdef PowerPlant_PCH
#include PowerPlant_PCH
#endif

#include	"LTextEngine.h"

#ifndef		__AEREGISTRY__
#include	<AERegistry.h>
#endif

#pragma	warn_unusedarg off

/*	===========================================================================

Notes:
------

¥	Part accessing is implemented in a two tier scheme.  The first tier would
	be overridden for performance enhancements.  The second tier is just to
	increase code reuse for this specific implementation.

¥	Client code of text engines should only use:

		CountParts,
		FindNthPart,
		FindSubRangePartIndices, and
		FindPart

¥	White space doesn't count as words (anymore).
		
*/
//	===========================================================================


Int32	LTextEngine::CountParts(
	const TextRangeT	&inQueryRange,
	DescType			inPartType) const
{
	Int32		rval = 0;
	
	TextRangeT	queryRange = inQueryRange;

	queryRange.Crop(GetTotalRange());

	switch (inPartType) {
		case cParagraph:
			rval = CountParas(queryRange);
			break;
		case cLine:
			rval = CountLines(queryRange);
			break;
		case cWord:
			rval = CountWords(queryRange);
			break;
		case cChar:
			rval = CountChars(queryRange);
			break;
		default:
			ThrowOSErr_(errAEUnknownObjectType);
			break;
	}
	
	return rval;
}

DescType	LTextEngine::FindNthPart(
	const TextRangeT	&inQueryRange,
	DescType			inPartType,
	Int32				inPartIndex,
	TextRangeT			*outPartRange) const
/*
	Find the nth part occurance of an inPartType relative to inQueryRange.
	
	OutPartRange returns the range of the found part.
*/
{
	TextRangeT	range;
	DescType	rval = typeNull;
	
	TextRangeT	queryRange = inQueryRange;

	queryRange.Crop(GetTotalRange());
	
	Assert_(inPartIndex > 0);
	Assert_(queryRange.Start() >= 0);
	Assert_(queryRange.Length() >= 0);
	
	switch (inPartType) {

		case cParagraph:
			rval = FindNthPara(queryRange, inPartIndex, &range);
			break;

		case cLine:
			rval = FindNthLine(queryRange, inPartIndex, &range);
			break;

		case cWord:
			rval = FindNthWord(queryRange, inPartIndex, &range);
			break;

		case cChar:
			rval = FindNthChar(queryRange, inPartIndex, &range);
			break;

		default:
			ThrowOSErr_(errAEUnknownObjectType);
			break;
		}

	if (outPartRange && (rval != typeNull))
		*outPartRange = range;
		
	return rval;
}

DescType	LTextEngine::FindSubRangePartIndices(
	const TextRangeT	&inQueryRange,
	const TextRangeT	&inSubRange,
	DescType			inSubRangeType,
	Int32				*outFirstIndex,
	Int32				*outLastIndex) const
/*
	Returns the part range indices (1 based) for inSubRange of part
	type inSubRangeType.
	
	If inSubRange doesn't fall on inSubRangeType part boundaries, the
	closest approximate inSubRangeType part indices will be returned AND
	the function result will indicate the part type of the inSubRange
	boundaries.
	
	If inSubRange does fall on inSubRangeType part boundaries, the
	function result will be inSubRangeType.
	
	Everything is relative to inQueryRange with the first inSubRangeType
	part of inQueryRange numbered 1.  Subsequent indices are
	relative to that first part.  Also, the first position of
	inQueryRange "starts" a new part for all part types and the end of
	inQueryRange "ends" all part types.
	
	If inSubRange doesn't lie within inQueryRange, the function result
	will be typeNull.
	
	If typeNull is returned, outFirstIndex & outLastIndex have not
	been modified.
*/
{
	Int32		index1 = -1,
				index2 = -1;
	DescType	partFoundType = inSubRangeType;

	TextRangeT	queryRange = inQueryRange,
				subRange = inSubRange;

	queryRange.Crop(GetTotalRange());
	subRange.Crop(GetTotalRange());
	
	do {
		//	Sanity checks.
		if (!queryRange.Contains(subRange)) {
			partFoundType = typeNull;
			Assert_(false);
			break;
		}
		
		switch (inSubRangeType) {

			case cParagraph:
				partFoundType = FindParaSubRangeIndices(queryRange, subRange, &index1, &index2);
				break;

			case cLine:
				partFoundType = FindLineSubRangeIndices(queryRange, subRange, &index1, &index2);
				break;

			case cWord:
				partFoundType = FindWordSubRangeIndices(queryRange, subRange, &index1, &index2);
				break;

			case cChar:
				partFoundType = FindCharSubRangeIndices(queryRange, subRange, &index1, &index2);
				break;

			default:
				ThrowOSErr_(errAEUnknownObjectType);
				break;
		}
	} while (false);

	if (partFoundType != typeNull) {
		if (outFirstIndex)
			*outFirstIndex = index1;
		if (outLastIndex) {
			*outLastIndex = index2;
		}
	}

	return partFoundType;
}

DescType	LTextEngine::FindPart(
	const TextRangeT	&inQueryRange,
	TextRangeT			*ioSubRange,
	DescType			inSubPartType,
	Int32				*outNextOffset) const
{
	ThrowIfNULL_(ioSubRange);
	
	TextRangeT	queryRange = inQueryRange;
	DescType	rval = typeNull;
	
	queryRange.Crop(GetTotalRange());
	
	if (!queryRange.Contains(*ioSubRange))
		return typeNull;
		
	if (ioSubRange->Length()) {
		TextRangeT	range = *ioSubRange,
					range2 = range;
		DescType	rval1,
					rval2;
		
		range.Front();
		range2.Back();
		
		do {
			rval1 = FindPart(queryRange, &range, inSubPartType, NULL);
			rval = rval1;
			if (rval1 == typeNull)
				break;
				
			if (range.End() >= range2.Start())
				break;

			rval2 = FindPart(queryRange, &range2, inSubPartType, NULL);
			if (rval2 == typeNull)
				Throw_(paramErr);
			if (rval != rval2)
				rval = cChar;

			range.Union(range2);
		} while (false);
		
		if (rval != typeNull) {
			if (outNextOffset)
				*outNextOffset = range.End();
			Assert_(range.Contains(*ioSubRange));
			*ioSubRange = range;
		}
	} else {
		switch (inSubPartType) {
			case cParagraph:
				rval = FindPara(queryRange, ioSubRange, outNextOffset);
				break;
			case cLine:
				rval = FindLine(queryRange, ioSubRange, outNextOffset);
				break;
			case cWord:
				rval = FindWord(queryRange, ioSubRange, outNextOffset);
				break;
			case cChar:
				rval = FindChar(queryRange, ioSubRange, outNextOffset);
				break;
			default:
				ThrowOSErr_(errAEUnknownObjectType);
				break;
		}
	}

	return rval;
}

//	---------------------------------------------------------------------------
//	CountParts implementation:

Int32	LTextEngine::CountParas(const TextRangeT &inQueryRange) const
{
	return CountParts_(inQueryRange, cParagraph);
}

Int32	LTextEngine::CountLines(const TextRangeT &inQueryRange) const
{
	return CountParts_(inQueryRange, cLine);
}

Int32	LTextEngine::CountWords(const TextRangeT &inQueryRange) const
{
	return CountParts_(inQueryRange, cWord);
}

Int32	LTextEngine::CountChars(const TextRangeT &inQueryRange) const
{
	if (mAttributes & textEngineAttr_MultiByte) {
		return CountParts_(inQueryRange, cChar);
	} else {
		return inQueryRange.Length();
	}
}

//	---------------------------------------------------------------------------
//	FindSubRangePartIndices implementation:

/*
	The following routines should only be used by FindSubRangePartIndices.
	
	Description for all routines:
	
		Return the subrange indices for inSubRange relative to inQueryRange.
		
		index1 & index2 are modified only if corresponding indexes are found.
		
		Assumptions:
			
			inQueryRange falls on part boundaries
			
			inSubRange falls on part boundaries or, if 0 length, corresponds
			to the start of a part
*/

DescType	LTextEngine::FindParaSubRangeIndices(
	const TextRangeT	&inQueryRange,
	const TextRangeT	&inSubRange,
	Int32				*outIndex1,
	Int32				*outIndex2) const
{
	return FindPartSubRangeIndices(inQueryRange, inSubRange, cParagraph, outIndex1, outIndex2);
}

DescType	LTextEngine::FindLineSubRangeIndices(
	const TextRangeT	&inQueryRange,
	const TextRangeT	&inSubRange,
	Int32				*outIndex1,
	Int32				*outIndex2) const
{
	return FindPartSubRangeIndices(inQueryRange, inSubRange, cLine, outIndex1, outIndex2);
}

DescType	LTextEngine::FindWordSubRangeIndices(
	const TextRangeT	&inQueryRange,
	const TextRangeT	&inSubRange,
	Int32				*outIndex1,
	Int32				*outIndex2) const
{
	return FindPartSubRangeIndices(inQueryRange, inSubRange, cWord, outIndex1, outIndex2);
}

DescType	LTextEngine::FindCharSubRangeIndices(
	const TextRangeT	&inQueryRange,
	const TextRangeT	&inSubRange,
	Int32				*outIndex1,
	Int32				*outIndex2) const
{
	if (mAttributes & textEngineAttr_MultiByte) {

		return FindPartSubRangeIndices(inQueryRange, inSubRange, cChar, outIndex1, outIndex2);

	} else {

		//	index1
		*outIndex1 = inSubRange.Start() - inQueryRange.Start() + 1;
		
		//	index2
		if (inSubRange.Length() > 0)
			*outIndex2 = inSubRange.End() - inQueryRange.Start();
		else
			*outIndex2 = *outIndex1;
			
		return cChar;

	}
}

//	---------------------------------------------------------------------------
//	FindNthPart implementation:

DescType	LTextEngine::FindNthPara(
	const TextRangeT	&inQueryRange,
	Int32				inPartIndex,
	TextRangeT			*outRange) const
{
	return FindNthPart_(inQueryRange, inPartIndex, cParagraph, outRange);
}

DescType	LTextEngine::FindNthLine(
	const TextRangeT	&inQueryRange,
	Int32				inPartIndex,
	TextRangeT			*outRange) const
{
	return FindNthPart_(inQueryRange, inPartIndex, cLine, outRange);
}

DescType	LTextEngine::FindNthWord(
	const TextRangeT	&inQueryRange,
	Int32				inPartIndex,
	TextRangeT			*outRange) const
{
	return FindNthPart_(inQueryRange, inPartIndex, cWord, outRange);
}

DescType	LTextEngine::FindNthChar(
	const TextRangeT	&inQueryRange,
	Int32				inPartIndex,
	TextRangeT			*outRange) const
{
	if (mAttributes & textEngineAttr_MultiByte) {

		return FindNthPart_(inQueryRange, inPartIndex, cChar, outRange);

	} else {

		*outRange = TextRangeT(inQueryRange.Start() + inPartIndex -1);
		outRange->SetLength(1);
		
		return cChar;

	}
}

//	---------------------------------------------------------------------------
//	Where the real non cChar work is done by this implementation...

Int32	LTextEngine::CountParts_(const TextRangeT inQueryRange, DescType inPartType) const
{
	Int32		count = 0,
				nextOffset;
	TextRangeT	range = inQueryRange;
	DescType	foundType;
	
	range.Front();

	while (true) {
		foundType = FindPart(inQueryRange, &range, inPartType, &nextOffset);
		if (foundType == typeNull)
			break;

		if (!(inPartType == cWord) || !IsWhiteSpace(range))
			count++;
		
		range.After();
	}

	return count;
}

DescType	LTextEngine::FindPartSubRangeIndices(
	const TextRangeT	&inQueryRange,
	const TextRangeT	&inSubRange,
	DescType			inSubPartType,
	Int32				*outIndex1,
	Int32				*outIndex2) const
{
	TextRangeT	range = inQueryRange;
	Int32		count = 0;
	DescType	partType,
				rval = inSubPartType;
	Boolean		foundIndex1 = false;
	
	if (!inQueryRange.Contains(inSubRange))
		return typeNull;
		
	range.Front();
	
	while (true) {
		partType = FindPart(inQueryRange, &range, inSubPartType, NULL);
		
		if (partType == typeNull)
			break;
		
		if ((inSubPartType == cWord) && IsWhiteSpace(range)) {
			range.After();
			continue;
		}

		count++;
		if (!foundIndex1)
			*outIndex1 = count;
		*outIndex2 = count;
		
		//	index1
		if (!foundIndex1) {
			if (range.Start() == inSubRange.Start()) {
				rval = partType;
				foundIndex1 = true;
			} else if (range.End() > inSubRange.Start()) {
				rval = cChar;
				foundIndex1 = true;
			}
		}
		
		//	index2
		if (foundIndex1) {
			if (range.End() == inSubRange.End()) {
				break;
			} else if (range.End() > inSubRange.End()) {
				rval = cChar;
				break;
			}
		}
	
		//	next
		range.After();
	}
	
	return rval;
}

DescType	LTextEngine::FindNthPart_(
	const TextRangeT	&inQueryRange,
	Int32				inPartIndex,
	DescType			inPartType,
	TextRangeT			*outRange) const
{
	Int32		i,
				nextOffset = inQueryRange.Start();
	TextRangeT	range = inQueryRange;
	DescType	partType;
	
	range.Front();
	i = 0;
	while (true) {
		partType = FindPart(inQueryRange, &range, inPartType, NULL);
		
		if (partType == typeNull)
			ThrowOSErr_(errAEIllegalIndex);
		
		if (!(inPartType == cWord) || !IsWhiteSpace(range))
			i++;
		
		if (i == inPartIndex)
			break;
		
		Assert_(i < inPartIndex);
		
		range.After();
	}
	
	if (partType != typeNull)
		*outRange = range;
	
	return partType;
}

DescType	LTextEngine::FindPara(
	const TextRangeT	&inQueryRange,
	TextRangeT			*ioRange,
	Int32				*outEndOffset) const
/*
	Should only be called by FindPart
*/
{
	return FindPara_(inQueryRange, ioRange, outEndOffset);
}

DescType	LTextEngine::FindPara_(
	const TextRangeT	&inQueryRange,
	TextRangeT			*ioRange,
	Int32				*outEndOffset) const
{
	ThrowIfNULL_(ioRange);

	TextRangeT		range = *ioRange,
					rangeFound,
					rangeReported;
	
	if (range.Insertion() == rangeCode_Before) {
		range.SetStart(range.Start() - PrevCharSize(range.Start()));
		range.SetInsertion(rangeCode_After);
	}
	
	//	Initial validity check
	if (range.Start() < inQueryRange.Start())
		return typeNull;
	if (range.Start() >= inQueryRange.End())
		return typeNull;

	//	Find
	{
		Int32	size,
				offset = range.Start();
		Int16	c;
		
		//	back
		do {
			size = PrevCharSize(offset);
			if (size == 0)
				break;
			c = ThisChar(offset - size);
			if (c == 0x0d)
				break;
			offset -= size;
		} while (true);
		rangeFound = TextRangeT(offset);
		
		//	forward
		offset = range.Start();
		do {
			size = ThisCharSize(offset);
			if (size == 0)
				break;
			c = ThisChar(offset);
			offset += size;
		} while (c != 0x0d);
		rangeFound.SetEnd(offset);
	}
	
	if (rangeFound.Length() <= 0)
		return typeNull;
	
	//	Truncate to query range.
	rangeReported = rangeFound;
	rangeReported.Intersection(inQueryRange);
	if (rangeReported.Undefined())
		return typeNull;
		
	//	Report
	*ioRange = rangeReported;
	
	if (outEndOffset)
		*outEndOffset = rangeReported.End();
	
	if (rangeFound == rangeReported)
		return cParagraph;
	else
		return cChar;
}

DescType	LTextEngine::FindLine(
	const TextRangeT	&inQueryRange,
	TextRangeT			*ioRange,
	Int32				*outEndOffset) const
/*
	Should only be called by FindPart
*/
{
	return typeNull;
}

DescType	LTextEngine::FindWord(
	const TextRangeT	&inQueryRange,
	TextRangeT			*ioRange,
	Int32				*outEndOffset) const
{
	return typeNull;
}

DescType	LTextEngine::FindChar(
	const TextRangeT	&inQueryRange,
	TextRangeT			*ioRange,
	Int32				*outEndOffset) const
{
	if (!inQueryRange.Contains(*ioRange))
		return typeNull;

	Assert_(ioRange->Length() == 0);
	if (ioRange->Insertion() == rangeCode_Before) {
		ioRange->SetLength(PrevCharSize(ioRange->Start()));
		ioRange->SetStart(ioRange->Start() - ioRange->Length());
	} else {
		Assert_(ioRange->Insertion() == rangeCode_After);
		ioRange->SetLength(ThisCharSize(ioRange->Start()));
	}

	ioRange->Crop(inQueryRange);
	if (outEndOffset)
		*outEndOffset = ioRange->End();

	return cChar;
}

Boolean	LTextEngine::IsWhiteSpace(const TextRangeT &inRange) const
{
	Boolean	rval = true;
	Int32	offset = inRange.Start(),
			end = inRange.End();
	Int16	c;
	
	while ((offset < end) && rval) {
		c = ThisChar(offset);
		
		switch (c) {
			case ' ':
			case '\t':
			case 0x0d:
				offset += ThisCharSize(offset);
				break;
			
			default:
				rval = false;
				break;
		}
	}
	
	return rval;
}
