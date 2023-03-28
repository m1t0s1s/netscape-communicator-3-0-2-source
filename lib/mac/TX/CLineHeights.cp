// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ


#include "CLineHeights.h"


// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	CLineHeights.cp	
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

#include "CLineHeights.h"


// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	¥	
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

CLineHeights::CLineHeights()
{
	IArray(sizeof(TLineHeightGroup));

	fTotalHeight = 0;
	fLastLineIndex = -1;

	fNumer = fDenom = 1;
}

// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	¥	
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

CLineHeights::~CLineHeights()
{
}

// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	¥	
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
		
TLineHeightGroup* CLineHeights::LineToHeightGroup(long* lineNo, long* groupNo /*=nil*/) const
/*return the ptr in "this" to the group element containing the line "lineNo"
on output, lineNo is set to be the index of the line in its group
¥¥on input, assume lineNo is a valid line*/
{
	TLineHeightGroup* firstGroupPtr = (TLineHeightGroup*)(this->GetElementPtr(0));
	register TLineHeightGroup* groupPtr = firstGroupPtr;
	while (true)
		{
		Assert_(groupPtr-firstGroupPtr < this->CountElements());
		
		register long count = groupPtr->groupCountLines;
		
		if (count > *lineNo)
			{
			if (groupNo)
				*groupNo = groupPtr-firstGroupPtr;
			return groupPtr;
			}
		
		*lineNo -= count;
		++groupPtr;
		}
}

// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	¥	
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

TLineHeightGroup* CLineHeights::PixelToHeightGroup(long* vPix, long* groupLineNo) const
//the out vPix is the pix offset to the group returned as the fn result
{
	*groupLineNo = 0;

	register long	maxGroupIndex = this->CountElements() - 1;
	register TLineHeightGroup* groupPtr = (TLineHeightGroup*)(this->GetElementPtr(0));
	register long remainPix = *vPix;
	long totalPix = remainPix;
	
	for (long groupIndex = 0; groupIndex <= maxGroupIndex; ++groupIndex, ++groupPtr) {
		long groupHeight = groupPtr->groupCountLines * groupPtr->lineHeight;
		
		if (groupHeight > remainPix) {
			*vPix = totalPix-remainPix;
			return groupPtr;
		}
		
		remainPix -= groupHeight;
		*groupLineNo += groupPtr->groupCountLines;
	}
	
	//vPix is >= last line pix start
	#ifdef txtnDebug
	SignalPStr_("\p PixelToHeightGroup : Will return nil");
	#endif
	return nil;
}
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	¥	
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

long CLineHeights::HeightToCountLines(const TLineHeightGroup& group, long groupLineNo
																				, long* height) const
//works on a height group scope
//groupLineNo is the index of the line in the the group "group"
//height is input/output and its measured starting from groupLineNo
//¥¥ should not disturb memory
{
	long linesRemain = group.groupCountLines-groupLineNo;
	
	Assert_(linesRemain > 0);
	
	register short lineHeight = group.lineHeight;
	
	long heightRemain = linesRemain * lineHeight;
	
	if (*height >= heightRemain) {
		*height -= heightRemain;
		return linesRemain;
	}
	else {
		long temp = LongRoundUp(*height, lineHeight);
		*height -= (temp*lineHeight);
		return temp;
	}
}
//************************************************************************************************

long CLineHeights::PixelToLine(long* vPix, long lineNo /*=0*/, TLineHeightGroup** heightGroupPtr /*= nil*/, long* groupLineIndex /*= nil*/) const
/*
-¥¥ doesn't disturb memory
-vPix is input/output, the out vPix is the pixOffset to the line returned as the fn result
-vPix is measured from the input "lineNo"
-if vPix is beyond the text height ==> the returned value is > fLastLineIndex
- if vPix is at exactly 2 lines boundary, the returned result is the higher line,
	 otherwise when called to draw lines starting from vPix the result will be inaccurate
	(will include unecessary line)
*/
{
	long index = lineNo;
	TLineHeightGroup* groupPtr = this->LineToHeightGroup(&index);
	
	long pixRemain = *vPix;
	long countGroupLines;
	if (!pixRemain) {countGroupLines = index;}
	else {
		--groupPtr;
		do {
			countGroupLines = this->HeightToCountLines(*(++groupPtr), index, &pixRemain);
			
			lineNo += countGroupLines;
			countGroupLines += index;
			index = 0;
		} while ((pixRemain > 0) && (lineNo <= fLastLineIndex));
		
		if (pixRemain < 0) {
			short lineHeight = groupPtr->lineHeight;
			*vPix -= (lineHeight+pixRemain);
			
			--countGroupLines;
			--lineNo;
		}
		else {
			if (pixRemain == 0) {
				if (countGroupLines == groupPtr->groupCountLines) { //vPix is at the boundary of 2 height groups
					countGroupLines = 0;
					++groupPtr;
				}
			}
			else {if (lineNo > fLastLineIndex) {*vPix -= pixRemain;} }
		}
	}
	
	if (heightGroupPtr) {
		*heightGroupPtr = groupPtr;
		*groupLineIndex = countGroupLines;
	}
	
	return lineNo;
}
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	¥	
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

long CLineHeights::GetTotalLinesHeight(void) const
{
	if (fNumer == fDenom)
		return fTotalHeight;
		
	return GetLineRangeHeight(0, fLastLineIndex);
}


Int32 CLineHeights::GetLineRangeHeight(long firstLine, long lastLine) const
//¥¥should not disturb memory
{
	Assert_(firstLine <= lastLine);
	
	if (fLastLineIndex < 0)
		return 0;
	
	if ((firstLine == 0) && (lastLine == fLastLineIndex))
		return fTotalHeight;
	
	Boolean oneLine = firstLine == lastLine;
	
	register TLineHeightGroup* lineGroup = this->LineToHeightGroup(&firstLine);
	
	short lineHeight = lineGroup->lineHeight;
	
	if (oneLine)
		return lineHeight;
	
	register TLineHeightGroup* lastLineGroup = this->LineToHeightGroup(&lastLine);
	
	if (lineGroup == lastLineGroup)
		return (lastLine-firstLine+1) * lineHeight;
	
	register long theHeight = (lineGroup->groupCountLines - firstLine) * lineHeight
										+ (lastLine+1) * lastLineGroup->lineHeight;

	while (++lineGroup < lastLineGroup) {
		theHeight += lineGroup->groupCountLines * lineGroup->lineHeight;
	}
	
	return theHeight;
}
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	¥	
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

void CLineHeights::GetLineHeightInfo(long lineNo, TLineHeightInfo* lineHeightInfo) const
{
	TLineHeightGroup* lineGroup = this->LineToHeightGroup(&lineNo);

		lineHeightInfo->lineHeight = lineGroup->lineHeight;
		lineHeightInfo->lineAscent = lineGroup->lineAscent;
}
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	¥	
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

Boolean CLineHeights::EqualGroup(long groupIndex, const TLineHeightGroup& group) const
//shouldn't disturb memory
{
	if ((groupIndex >= 0) && (groupIndex < this->CountElements())) {
		TLineHeightGroup* groupToCompare = (TLineHeightGroup*)(this->GetElementPtr(groupIndex));
		return (group.lineHeight == groupToCompare->lineHeight) && (group.lineAscent == groupToCompare->lineAscent);
	}
	else
		return false;
}
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	¥	
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

Boolean CLineHeights::EqualGroup(long group1Index, long group2Index) const
{
	if ((group2Index >= 0) && (group2Index < this->CountElements()))
		return this->EqualGroup(group1Index, *(TLineHeightGroup*) this->GetElementPtr(group2Index));
	else
		return false;
}
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	¥	
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

Boolean CLineHeights::Concat(long group1Index, long group2Index, const TLineHeightGroup* groupPtr/*=nil*/)
/*if groupPtr != nil concate *groupPtr with either group1Index or group2Index if possible
else concate group1Index or group2Index if possible
-shouldn't disturb memory*/
{
	if (groupPtr) {
		long concatIndex = group1Index;
		if (!this->EqualGroup(concatIndex, *groupPtr)) {
			concatIndex = group2Index;
			if (!this->EqualGroup(concatIndex, *groupPtr))
				return false;
		}
		
		((TLineHeightGroup*) this->GetElementPtr(concatIndex))->groupCountLines += groupPtr->groupCountLines;
		return true;
	}
	else {
		if (this->EqualGroup(group1Index, group2Index)) {
			((TLineHeightGroup*) this->GetElementPtr(group1Index))->groupCountLines +=
								((TLineHeightGroup*) this->GetElementPtr(group2Index))->groupCountLines;
			return true;
		}
		else
			return false;
	}
}
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	¥	
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

OSErr CLineHeights::SetLineHeightInfo(const TLineHeightInfo& lineHeightInfo, long lineNo)
{
	long groupIndex;
	TLineHeightGroup* lineGroup = this->LineToHeightGroup(&lineNo, &groupIndex);
	if ((lineGroup->lineHeight == lineHeightInfo.lineHeight) && (lineGroup->lineAscent == lineHeightInfo.lineAscent))
		{return noErr;}
	
	TLineHeightGroup newGroup;
	newGroup.groupCountLines = 1;
	newGroup.lineHeight = lineHeightInfo.lineHeight;
	newGroup.lineAscent = lineHeightInfo.lineAscent;
	
//	this->CalcScaledHeightInfo(&newGroup);
//	fTotalHeight += newGroup.scaledHeight - lineGroup->scaledHeight;
	fTotalHeight += newGroup.lineHeight - lineGroup->lineHeight;
	
	
	long oldGroupCountLines = lineGroup->groupCountLines;
	
	if (oldGroupCountLines == 1) {
		long groupBefore = groupIndex-1;
		long groupAfter = groupIndex+1;
		if (this->Concat(groupBefore, groupAfter, &newGroup)) {
			this->RemoveElements(groupIndex, this->Concat(groupBefore, groupAfter) ? 2 : 1);
		}
		else {*lineGroup = newGroup;}
		
		return noErr;
	}
	
	long insertIndex = groupIndex + (lineNo != 0);
	
	if ((lineNo == 0) || (lineNo == oldGroupCountLines-1)) {
		--(lineGroup->groupCountLines);
		if (this->Concat(insertIndex-1, insertIndex, &newGroup)) {return noErr;}
		else {return (this->InsertElements(1/*count*/, &newGroup, insertIndex) ? noErr : memFullErr);}
	}

	TLineHeightGroup oldGroup = *lineGroup;
	
	lineGroup->groupCountLines = lineNo;

	lineGroup = (TLineHeightGroup*)(this->InsertElements(2/*count*/, nil, insertIndex));
	
	if (lineGroup) {
		*lineGroup++ = newGroup;
		
		*lineGroup = oldGroup;
		lineGroup->groupCountLines = oldGroupCountLines-lineNo-1;
		
		return noErr;
	}
	else {return memFullErr;}
}
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	¥	
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

OSErr CLineHeights::InsertLine(const TLineHeightInfo& lineHeightInfo, long lineNo /*= -1*/)
{
	if (++fLastLineIndex == 0) {
		TLineHeightGroup* lineGroup = (TLineHeightGroup*) this->InsertElements(1/*count*/, nil, 0);
		if (lineGroup) {
			lineGroup->groupCountLines = 1;
			lineGroup->lineAscent = lineHeightInfo.lineAscent;
			lineGroup->lineHeight = lineHeightInfo.lineHeight;
			
//			this->CalcScaledHeightInfo(lineGroup);
//			fTotalHeight = lineGroup->scaledHeight;
			fTotalHeight = lineGroup->lineHeight;
						
			return noErr;
		}
		else
			return memFullErr;
	}
	

	if (lineNo < 0)
		lineNo = fLastLineIndex;
	
	long groupLineNo = lineNo - (lineNo == fLastLineIndex);
	TLineHeightGroup* lineGroup = this->LineToHeightGroup(&groupLineNo);
	
	++(lineGroup->groupCountLines);
	fTotalHeight += lineGroup->lineHeight;

	return CLineHeights::SetLineHeightInfo(lineHeightInfo, lineNo);
	
	/*it's required to be CLineHeights::SetLineHeightInfo and not "this->SetLineHeightInfo" since the call of
	SetLineHeightInfo here is an implementation dependent of the CLineHeights class and not a general 
	behaviour of all descendents*/
}
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	¥	
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

void CLineHeights::RemoveLines(long countLines, long lineNo)
{
	if (!countLines) {return;}
	
	if ((fLastLineIndex -= countLines) < 0)
		{
		this->SetElementsCount(0);
		fTotalHeight = 0;
		return;
		}
	
	long groupIndex;
	register TLineHeightGroup* lineGroup = this->LineToHeightGroup(&lineNo, &groupIndex);
	
	long groupLinesToRemove;
	if (lineNo)
		{
		groupLinesToRemove = MinLong(countLines, lineGroup->groupCountLines-lineNo);
		lineGroup->groupCountLines -= groupLinesToRemove;
		++groupIndex;
		}
	else
		groupLinesToRemove = 0;

	long groupsToRemove = 0;
	while (countLines)
		{
		if (groupLinesToRemove == 0)
			{
			if (countLines >= lineGroup->groupCountLines)
				{
				groupLinesToRemove = lineGroup->groupCountLines;
				++groupsToRemove;
				}
			else
				{
				groupLinesToRemove = countLines;
				lineGroup->groupCountLines -= countLines;
				}
			}
		
		countLines -= groupLinesToRemove;
		
		fTotalHeight -= groupLinesToRemove * lineGroup->lineHeight;
		
		++lineGroup;
		groupLinesToRemove = 0;
		}
	
	if (groupsToRemove)
		{
		if (this->Concat(groupIndex-1, groupIndex+groupsToRemove))
			++groupsToRemove;
		this->RemoveElements(groupIndex, groupsToRemove);
		}
}


Boolean CLineHeights::GetTotalLineRange(TOffsetPair& outLines) const
{
	outLines.firstOffset = 0;
	outLines.lastOffset = CountLines() - 1;

	return (outLines.lastOffset >= 0);
}

// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	¥	
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ



















