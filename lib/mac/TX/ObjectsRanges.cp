// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

#include "ObjectsRanges.h"


//***************************************************************************************************





CRanges::CRanges()
{
}
//***************************************************************************************************


void CRanges::FreeData(Boolean compact /*= true*/)
//¥¥FreeData should be in a resident seg, they may be called from TSMTextension when KeyDown
{
	this->SetElementsCount(0);
	
	if (compact)
		{this->Compact();}
}
//***************************************************************************************************

long CRanges::GetRangeEnd(long rangeIndex) const
{
	Assert_(rangeIndex < this->CountElements());
	
	return (rangeIndex < 0) ? 0 : *(long*)this->GetElementPtr(rangeIndex);
}
//***************************************************************************************************

long CRanges::GetRangeStart(long rangeIndex) const
{
	Assert_(rangeIndex < this->CountElements());

	return (rangeIndex <= 0) ? 0 : *(long*)this->GetElementPtr(rangeIndex-1);
}
//***************************************************************************************************

void CRanges::GetRangeBounds(long rangeIndex, TOffsetPair* bounds) const
{
	bounds->firstOffset = this->GetRangeStart(rangeIndex);
	bounds->lastOffset = this->GetRangeEnd(rangeIndex);
}
//***************************************************************************************************

long CRanges::GetRangeLen(long rangeIndex) const
{
	Assert_(rangeIndex < this->CountElements());
	
	return (this->GetRangeEnd(rangeIndex) - this->GetRangeStart(rangeIndex));
}
//***************************************************************************************************

void CRanges::SetRangeEnd(long rangeIndex, long newEnd)
{
	Assert_(rangeIndex < this->CountElements());

	*(long*)this->GetElementPtr(rangeIndex) = newEnd;
}
//***************************************************************************************************

Boolean CRanges::IsRangeStart(long offset, long rangeIndex /*= -1*/) const
{
	if (rangeIndex < 0)
		rangeIndex = this->OffsetToRangeIndex(offset);
		
	return this->GetRangeStart(rangeIndex) == offset;
}
//***************************************************************************************************

long CRanges::GetLastRangeEnd() const
{
	long count = this->CountElements();
	return (count) ? *(long*)this->GetElementPtr(count-1) : 0;
}
//***************************************************************************************************


long CRanges::OffsetToRangeIndex(TOffset rangeOffset) const
//Note that if an empty range exists at the end (ex: Formatter::AppendEmptyLine), the last 2 ranges have the same end
{
	long lastRangeIndex = this->CountElements() -1;
	
	if (lastRangeIndex > 0) {
		long rangeIndex = this->SearchBigger(rangeOffset.offset);
		if ((rangeOffset.trailEdge) && (rangeIndex > 0) && (this->IsRangeStart(rangeOffset.offset, rangeIndex)))
			{--rangeIndex;}
		
		return rangeIndex;
	}
	else {return lastRangeIndex;}
}
//***************************************************************************************************

void CRanges::OffsetRanges(long valToAdd, long firstIndex, long count/*=-1*/)
//count < 0 ==> offset up to last range
{
	if (!valToAdd) {return;}
	
	if (count < 0) {
		count = this->CountElements() - firstIndex;
		if (count <= 0) {return;}
	}
	
	if ((valToAdd < 0) && (this->GetLastRangeEnd()+valToAdd) == 0) {return;}
	
	#ifdef powerc
		AddToArrayElements(valToAdd, (char*) this->GetElementPtr(firstIndex), count, this->GetElementSize());
	#else
		char* rangePtr = (char*) this->GetElementPtr(firstIndex);
		short elemSize = this->GetElementSize();
	
		if (elemSize == 4)
			AddToLongArray(valToAdd, rangePtr, count);
		else
			AddToArrayElements(valToAdd, rangePtr, count, elemSize);
	#endif
}
//***************************************************************************************************

long CRanges::SectRanges(long startOffset, long count, TSectRanges* sectInfo) const
//returns the count of sected ranges
{
	long endOffset = startOffset + count;
	
//lead info	
	sectInfo->leadIndex = this->OffsetToRangeIndex(startOffset);
	if (sectInfo->leadIndex < 0) {//no ranges
		sectInfo->leadIndex = 0;
		sectInfo->leadCount = 0;
		sectInfo->leadSectCount = 0;
		sectInfo->insideIndex = 0;
		sectInfo->insideCount = 0;
		sectInfo->trailIndex = 0;
		sectInfo->trailCount = 0;
		sectInfo->trailSectCount = 0;
		return 0;
	} 
	
	sectInfo->leadCount = startOffset-this->GetRangeStart(sectInfo->leadIndex);
	
	long firstRangeEnd = this->GetRangeEnd(sectInfo->leadIndex);
	
	sectInfo->leadSectCount = MinLong(firstRangeEnd, endOffset)-startOffset;
	
	
//trail info		
	if (endOffset <= this->GetRangeEnd(sectInfo->leadIndex))
		{sectInfo->trailIndex = sectInfo->leadIndex;}
	else {
		TOffset temp(endOffset, true/*end of range*/);
		sectInfo->trailIndex = this->OffsetToRangeIndex(temp);
	}
	
	sectInfo->trailCount = this->GetRangeEnd(sectInfo->trailIndex)-endOffset;
	
	sectInfo->trailSectCount = this->GetRangeLen(sectInfo->trailIndex)-sectInfo->trailCount;
	
	long countSected = sectInfo->trailIndex-sectInfo->leadIndex+1;
	
//inside info	
	sectInfo->insideIndex = sectInfo->leadIndex;
	sectInfo->insideCount = countSected;
	
	if ((sectInfo->leadCount) || (endOffset < firstRangeEnd)) {//first range is not completely inside the range
		++sectInfo->insideIndex;
		--sectInfo->insideCount;
	}
	
	if (sectInfo->trailCount) {--sectInfo->insideCount;}
	
	if (sectInfo->insideCount < 0) {sectInfo->insideCount = 0;}
	
	return countSected;
}
//*************************************************************************************************

#ifdef txtnDebug
void CRanges::CheckCoherence(long count)
{
	if (this->GetLastRangeEnd() != count) {
		SignalPStr_("\p¥¥Incoherent Ranges (Bad Count)");
		return;
	}
	
	long lastRangeEnd = 0;
	
	for (short rangeCtr = 0; rangeCtr < this->CountElements(); ++rangeCtr) {
		long rangeEnd = *(long*)(this->GetElementPtr(rangeCtr));
		
		if (rangeEnd <= lastRangeEnd) {
			SignalPStr_("\p¥¥Incoherent Ranges (Table)");
			return;
		}
		lastRangeEnd = rangeEnd;
	}
}
//***************************************************************************************************
#endif




struct TObjectRange {
	long rangeEnd;
	CAttrObject* theObject;
};
typedef TObjectRange* TObjectRangePtr;

//***************************************************************************************************



CObjectsRanges::CObjectsRanges()
{
}
//***************************************************************************************************

void CObjectsRanges::IObjectsRanges(CAttrObject* continuousObj, short moreElements /*=1*/)
{
	this->IRanges(sizeof(TObjectRange), moreElements);

	fLastSearchedObj = nil;
	
	fContinuousObj = continuousObj; //¥¥may be nil
	fLastContStart = -1;
	
	fFreeObjects = 1;
}
//***************************************************************************************************

void CObjectsRanges::Free()
{
	if (fFreeObjects) {this->FreeObjects();}
	
	if (fContinuousObj) {fContinuousObj->Free();}

	CRanges::Free();
}
//***************************************************************************************************



//¥¥FreeObjects and FreeData should be in a resident seg, they may be called from TSMTextension when KeyDown
void CObjectsRanges::FreeObjects(short firstIndex /*= 0*/, short lastIndex /*= -1*/)
	//frees the objects in the range firstIndex..lastIndex inclusive
	//no params ==> free all objects
{
	if (lastIndex < 0) {lastIndex = this->CountRanges();}
	else {++lastIndex;}
	
	this->LockArray();
	
	register TObjectRangePtr rangePtr = TObjectRangePtr(this->GetElementPtr(firstIndex));

	register short count = lastIndex-firstIndex;
	while (--count >= 0) {
		rangePtr->theObject->Free();
		
		++rangePtr;
	}
	
	this->UnlockArray();
	
	fLastSearchedObj = nil;
}
//***************************************************************************************************

void CObjectsRanges::FreeData(Boolean compact /*= true*/)
{
	this->FreeObjects();
	
	fLastContStart = -1;//invalidate fContinuousObj

	CRanges::FreeData(compact);
}
//***************************************************************************************************


CAttrObject* CObjectsRanges::OffsetToObject(TOffset objectOffset)
{
	register short rangeIndex = short(this->OffsetToRangeIndex(objectOffset));
	
	return (rangeIndex < 0) ? nil : TObjectRangePtr(this->GetElementPtr(rangeIndex))->theObject;
}
//***************************************************************************************************

short CObjectsRanges::GetNextObjectRange(long offset, CAttrObject** rangeObj, long* rangeLen) const
// returns the range index, -1 if no next
{
	if (offset >= this->GetLastRangeEnd()) {
		*rangeLen = 0;
		*rangeObj = nil;
		
		return -1;
	}
	else {
		short index = short(this->OffsetToRangeIndex(offset));
		
		TObjectRangePtr objectRangePtr = TObjectRangePtr(this->GetElementPtr(index));
		*rangeObj = objectRangePtr->theObject;
		*rangeLen = objectRangePtr->rangeEnd - offset;
		
		return index;
	}
}
//***************************************************************************************************

CAttrObject* CObjectsRanges::RangeIndexToObject(short rangeIndex) const
{
	return TObjectRangePtr(this->GetElementPtr(rangeIndex))->theObject;
}
//***************************************************************************************************



CAttrObject* CObjectsRanges::SearchObject(const CAttrObject* objToCheck)
{
	#ifdef txtnDebug
	//SignalPStr_("\p search object");
	#endif
	
	this->LockArray();
	
	register TObjectRangePtr rangePtr = TObjectRangePtr(this->GetElementPtr(0));

	register short rangeCtr = this->CountRanges();
	while (--rangeCtr >= 0) {
		if (objToCheck->IsEqual(rangePtr->theObject)) {
			this->UnlockArray();
			
			return rangePtr->theObject;
		}
		
		++rangePtr;
	}
	
	this->UnlockArray();
	
	return nil;
}
//***************************************************************************************************

CAttrObject* CObjectsRanges::MapObject(CAttrObject* newObj, Boolean takeObjectCopy, Boolean* reference)
{
	//check(newObj->HasAllAttributes());
	
	//	jah added
	if (!takeObjectCopy) {
		*reference = true;
		fLastSearchedObj = newObj;
		return newObj;
	}

	CAttrObject* obj;
	if (newObj->GetObjFlags() & kFixedRangeRun)
		obj = nil;
	else {
		if (fLastSearchedObj && fLastSearchedObj->IsEqual(newObj))
			obj = fLastSearchedObj;
		else
			obj = this->SearchObject(newObj);
	}
	
	if (obj) {
		if (!takeObjectCopy)
			newObj->Free();
		
		*reference = true;
	}
	else {
		*reference = false;
		
		if (takeObjectCopy) {
			obj = newObj->CreateNew();
			if (!obj)
				return nil;
			
			obj->Assign(newObj);
		}
		else
			obj = newObj;
	}
	
	fLastSearchedObj = obj;
	
	return obj;

/*
	//check(newObj->HasAllAttributes());
	
	CAttrObject* obj;
	if (newObj->GetObjFlags() & kFixedRangeRun)
		obj = nil;
	else {
		if (fLastSearchedObj && fLastSearchedObj->IsEqual(newObj))
			obj = fLastSearchedObj;
		else
			obj = this->SearchObject(newObj);
	}
	
	if (obj) {
		if (!takeObjectCopy)
			newObj->Free();
		
		*reference = true;
	}
	else {
		*reference = false;
		
		if (takeObjectCopy) {
			obj = newObj->CreateNew();
			if (!obj)
				return nil;
			
			obj->Assign(newObj);
		}
		else
			obj = newObj;
	}
	
	fLastSearchedObj = obj;
	
	return obj;
*/
}
//***************************************************************************************************

void CObjectsRanges::SetObjectRange(short rangeIndex, long newRangeEnd, CAttrObject* newObj, Boolean reference)
{
	if (reference)
		newObj = newObj->Reference();
	
	TObjectRangePtr rangePtr = TObjectRangePtr(this->GetElementPtr(rangeIndex));
	
	rangePtr->rangeEnd = newRangeEnd;
	
	CAttrObject* oldObject = rangePtr->theObject; //oldObject->Free may disturb memory, so call it at the end

	rangePtr->theObject = newObj;
	
	oldObject->Free();
	
	fLastSearchedObj = nil;
}
//***************************************************************************************************

OSErr CObjectsRanges::InsertObjectRange(short insertIndex, long rangeEnd, CAttrObject* rangeObj, Boolean reference)
{
	if (reference) {rangeObj = rangeObj->Reference();}
	
	TObjectRangePtr insertedRange = TObjectRangePtr(this->InsertElements(1, nil, insertIndex));
	
	if (insertedRange) {
		insertedRange->rangeEnd = rangeEnd;
		insertedRange->theObject = rangeObj;
		
		return noErr;
	}
	else {
		if ((reference) && (rangeObj)) {rangeObj->Free();}
		
		return memFullErr;
	}
}
//************************************************************************************************

Boolean CObjectsRanges::UpdateRangesBounds(long startOffset, long endOffset, CAttrObject* newObj
																				, short* firstRange, short* lastRange)
{
	TSectRanges sectInfo;
	this->SectRanges(startOffset, endOffset-startOffset, &sectInfo);
	
	*firstRange = short(sectInfo.leadIndex);
	*lastRange = short(sectInfo.trailIndex);

	short indexBefore = short(sectInfo.leadIndex);
	if (!sectInfo.leadCount)
		--indexBefore;
	CAttrObject* objBefore = (indexBefore >= 0) ? this->RangeIndexToObject(indexBefore) : nil;

	short indexAfter;
	CAttrObject* objAfter;
	if (endOffset >= this->GetLastRangeEnd()) {
		indexAfter = -1;
		objAfter = nil;
	}
	else {
		indexAfter = short(sectInfo.trailIndex);
		if (!sectInfo.trailCount && (indexAfter < this->CountRanges()-1))
			++indexAfter;
		
		objAfter = this->RangeIndexToObject(indexAfter);
	}
	
	unsigned long isIndivisible = newObj->GetObjFlags() & kFixedRangeRun;
	
	//split
	if (indexBefore == *firstRange) {
		if ((objBefore != newObj) || (isIndivisible)) {
			if (indexAfter != indexBefore)
				this->SetRangeEnd(sectInfo.leadIndex, startOffset);
			else //split to 3 ranges
				this->InsertObjectRange(short(sectInfo.leadIndex), startOffset, objBefore, true/*reference*/);
		}
		++(*firstRange);
	}
	
	if (indexAfter == *lastRange) {--(*lastRange);}
	
	//concat
	if ((objBefore == newObj) && !isIndivisible) {
		long indexBeforeEnd;
		if (objAfter == newObj) {
			*lastRange = indexAfter; //indexAfter is no longer needed
			indexBeforeEnd = this->GetRangeEnd(indexAfter);
		}
		else
			indexBeforeEnd = endOffset;
		
		this->SetRangeEnd(indexBefore, indexBeforeEnd);
	}
	
	return (isIndivisible) ? false : (objBefore == newObj) || (objAfter == newObj);	
}
//***************************************************************************************************

long CObjectsRanges::RemoveElements(long removeIndex, long countToRemove)
{
	this->FreeObjects((short)removeIndex, short(removeIndex+countToRemove-1));
	
	return CRanges::RemoveElements(removeIndex, countToRemove);
}
//***************************************************************************************************

void CObjectsRanges::ReplaceRangeObj(long offset, long len, CAttrObject* newObj, Boolean reference)
{
	if (!newObj) {return;}
	
	long rangeEnd = offset+len;
	
	short firstRange, lastRange;
	if (!this->UpdateRangesBounds(offset, rangeEnd, newObj, &firstRange, &lastRange)) {
		if (firstRange <= lastRange) {this->SetObjectRange(firstRange++, rangeEnd, newObj, reference);}
		else {this->InsertObjectRange(firstRange, rangeEnd, newObj, reference);}
	}
	
	//removes firstRange..lastRange inclusive
	
	short rangesToRemove = lastRange-firstRange+1;
	
	if (rangesToRemove > 0)
		this->RemoveElements(firstRange, rangesToRemove);
}
//***************************************************************************************************

void CObjectsRanges::ClearRange(long offset, long len)
{
	if (!len) {return;}
	
	if (len >= this->GetLastRangeEnd()) {
		this->FreeData();
		
		return;
	}

	Assert_(len);
	
	TSectRanges sectedRanges;
	this->SectRanges(offset, len, &sectedRanges);
	
	long replaceIndex;
	
	if (sectedRanges.leadCount)
		replaceIndex = sectedRanges.leadIndex;
	else {
		if (sectedRanges.trailCount) {replaceIndex = sectedRanges.trailIndex;}
		else {//complete ranges are enclosed in offset..offset+len
			replaceIndex = sectedRanges.leadIndex - 1;
			
			if (replaceIndex >= 0) {
				if (this->RangeIndexToObject((short)replaceIndex)->GetObjFlags() & kFixedRangeRun)
					replaceIndex = -1;
			}
		}
	}
	
	long offsetIndex;
	
	if (replaceIndex >= 0) {
		this->ReplaceRangeObj(offset, len, this->RangeIndexToObject((short)replaceIndex), true/*reference*/);
		offsetIndex = this->OffsetToRangeIndex(offset);
	}
	else {
		this->RemoveElements(sectedRanges.insideIndex, sectedRanges.insideCount);
		offsetIndex = sectedRanges.insideIndex;
	}
	
	this->OffsetRanges(-len, offsetIndex);
}
//***************************************************************************************************

OSErr CObjectsRanges::ReplaceRange(long offset, long oldLen, long newLen
																	, CAttrObject* theObject/*=nil*/, Boolean takeObjectCopy /*=true*/)
{
	Assert_(offset <= this->GetLastRangeEnd());
	
	long totalChars = this->GetLastRangeEnd();
	
	oldLen = MinLong(oldLen, totalChars-offset);

	fLastContStart = -1;//invalidate fContinuousObj
	
	if (!newLen) {
		this->ClearRange(offset, oldLen);
		
/*	commented out jah 950223...

		if (theObject && !takeObjectCopy)
			theObject->Free();
*/		
		
		return noErr;
	}
	
	Boolean reference;
	if (!theObject) {
		theObject = this->OffsetToObject(offset);
		reference = true;
	}
	else
		theObject = this->MapObject(theObject, takeObjectCopy, &reference);
	
	if (!theObject)
		return memFullErr;
	
	if (oldLen) {
		this->ReplaceRangeObj(offset, oldLen, theObject, reference);
		this->OffsetRanges(newLen-oldLen, this->OffsetToRangeIndex(offset));
		/*"OffsetToRangeIndex" can not be called only once at the start and then used in the 2 OffsetRanges
		and instead of "OffsetToObject" since the prev "ReplaceRangeObj" may concat ranges and then
		"offset" may change rangeIndex after it
		*/
	}
	else {
		//insert newLen
		if (!totalChars) {this->InsertObjectRange(-1/*insertIndex*/, newLen, theObject, reference);}
		else {
			this->OffsetRanges(newLen, this->OffsetToRangeIndex(offset));
			this->ReplaceRangeObj(offset, newLen, theObject, reference);
		}
	}
	
	return noErr;
}
//***************************************************************************************************

OSErr CObjectsRanges::ReplaceRange(long offset, long oldLen, CObjectsRanges* newRanges, Boolean plug)
{
	Assert_(newRanges);
	
	long countNewRanges = newRanges->CountRanges();
	
	Assert_(countNewRanges);
	
	if (!countNewRanges) {return this->ReplaceRange(offset, oldLen, (long)0/*newLen*/);}
	else {
		if ((plug) && (oldLen)) {
			//check "oldLen" : can't plug new ranges if oldLen == 0 (a run may split to 3 runs)
			TSectRanges sectInfo;
			this->SectRanges(offset, oldLen, &sectInfo);
			
			Assert_(sectInfo.leadCount == 0);
			Assert_(sectInfo.trailCount == 0);
			
			//free objects in the intersection
			this->FreeObjects((short)sectInfo.leadIndex, (short)sectInfo.trailIndex);
			
			void* rangesPtr = newRanges->LockArray();
			
			this->ReplaceElements(sectInfo.insideCount/*countOldRanges*/
				, countNewRanges
				, sectInfo.leadIndex/*replaceIndex*/, rangesPtr);
			
			newRanges->UnlockArray();
			
			this->OffsetRanges(offset, sectInfo.leadIndex, countNewRanges);
			
			this->OffsetRanges(newRanges->GetLastRangeEnd() - oldLen, sectInfo.leadIndex + countNewRanges);
			
			fLastContStart = -1; //invalidate fContinuousObj
		}
		else {
			for (short rangeIndex = 0; rangeIndex < countNewRanges; ++rangeIndex) {
				CAttrObject* rangeObj = newRanges->RangeIndexToObject(rangeIndex);
				long rangeLen = newRanges->GetRangeLen(rangeIndex);
				
				this->ReplaceRange(offset, oldLen, rangeLen/*newLen*/, rangeObj, false/*copyObj*/);
//				this->ReplaceRange(offset, oldLen, rangeLen/*newLen*/, rangeObj);
				
				oldLen = 0;
				offset += rangeLen;
			}
		}
	}
	
	return noErr;
}
//***************************************************************************************************

long CObjectsRanges::UpdateRangeObjects(long startOffset, long len, const AttrObjModifier& modifier)
{
	CAttrObject* continuousobj = this->GetContinuousObj(startOffset, len);
	fLastContStart = -1;//invalidate fContinuousObj
	
	long updateFlags = 0;
	
	while (len > 0) {
		long rangeLen;
		CAttrObject* rangeObj;
		this->GetNextObjectRange(startOffset, &rangeObj, &rangeLen);
		
		if (rangeObj->GetObjFlags() & kFixedRangeRun)
			updateFlags |= rangeObj->Update(modifier, continuousobj);
		else {
			CAttrObject* newObj = rangeObj->CreateNew();
			newObj->Assign(rangeObj);
	
			updateFlags |= newObj->Update(modifier, continuousobj);
			
			Boolean reference;
			newObj = this->MapObject(newObj, false/*makeCopy*/, &reference);
			this->ReplaceRangeObj(startOffset, MinLong(len, rangeLen), newObj, reference);
		}
		
		startOffset += rangeLen;
		len -= rangeLen;
	}
	
	return updateFlags;
}
//************************************************************************************************

CAttrObject* CObjectsRanges::GetContinuousObj(long startOffset, long len)
/*-the returned obj is the property of CObjectsRanges and should'nt be disposed or modified
	-work only for objects with the same class as the one provided as continuousObj to the I
*/
{
	if ((fLastContStart == startOffset) && (fLastContLen == len)) //false if fLastContStart -1 (invalid)
		return fContinuousObj;
	
	fLastContStart = startOffset;
	fLastContLen = len;
	
	register Boolean somethingCommon = false;
	
	if (!len) {
		CAttrObject* obj = this->OffsetToObject(startOffset);
		
		if (obj) {
			fContinuousObj->Assign(obj);
			somethingCommon = true;
		} 
	}
	else {
		Boolean firstObj = true;
		
		while (len > 0) {
			long rangeLen;
			CAttrObject* rangeObj;
			this->GetNextObjectRange(startOffset, &rangeObj, &rangeLen);
			
			if (firstObj) {
				firstObj = false;
				fContinuousObj->Assign(rangeObj);
				somethingCommon = !fContinuousObj->IsInvalid(); //fContinuousObj and rangeObj may be of different class types
			}
			else {
				somethingCommon = fContinuousObj->Common(rangeObj);
				if (!somethingCommon) break;
			}
			
			startOffset += rangeLen;
			len -= rangeLen;
		}
	}
	
	if (!somethingCommon)
		fContinuousObj->Invalid();
	
	return fContinuousObj;
}
//************************************************************************************************

#ifdef txtnDebug
short CObjectsRanges::CountUniqueObjects()
{
	short countRanges = this->CountRanges();
	short result = countRanges;
	
	for (short ctr = 0; ctr < countRanges; ++ctr) {
		CAttrObject* obj = this->RangeIndexToObject(ctr);
		
		for (short ctr2 = 0; ctr2 < countRanges; ++ctr2) {
			if (this->RangeIndexToObject(ctr2) == obj) {
				if (ctr2 < ctr) {break;}
				if (ctr2 != ctr)
					--result;
			}
		}
	}
	
	return result;
}
//************************************************************************************************
#endif

#ifdef txtnDebug
void CObjectsRanges::CheckCoherence(long count)
{
	CRanges::CheckCoherence(count);
}
//***************************************************************************************************
#endif
