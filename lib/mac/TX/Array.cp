// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

#include "Array.h"


//***************************************************************************************************

#ifdef txtnDebug
short gCountArrays = 0;
#endif
//***************************************************************************************************


CArray::CArray()
{
}
//***************************************************************************************************

void CArray::IArray(short arrayElementSize, short moreElementsCount /*= 0*/)
{
	#ifdef txtnDebug
	++gCountArrays;
	#endif

	fElementSize = arrayElementSize;
	
	fHandle = NewHandle(0);
	
	fLogicalCount = 0;
	fPhysicalCount = 0;
	
	fLockLevel = 0;
	if ((fMoreElementsCount = moreElementsCount) <= 0)
		fMoreElementsCount = 1;
}
//***************************************************************************************************

void CArray::Free()
{
	#ifdef txtnDebug
	--gCountArrays;
	#endif
	
	if (fHandle)
		::DisposeHandle(fHandle);
}
//***************************************************************************************************

//	//////////
//	jah	950130...

Handle	CArray::GetDataHandle(void)
{
	return fHandle;
}

void	CArray::SetDataHandle(Handle inHandle, short inElementSize)
{
	if (fHandle)
		DisposeHandle(fHandle);
	fHandle = inHandle;
	fElementSize = inElementSize;
	if (inHandle) {
		fLogicalCount = GetHandleSize(inHandle) / fElementSize;	//	necessary?
		fLockLevel = !((HGetState(inHandle) & 0x80) == 0);
	} else {
		fLogicalCount = 0;
		fLockLevel = 0;
	}
	fPhysicalCount = fLogicalCount; //	???
}
//	\\\\\\\\\\

//***************************************************************************************************

void* CArray::LockArray(Boolean moveHi /*= false*/)
{
	if (fLockLevel++ == 0) {
		if (moveHi)
			MoveHHi(fHandle);
			
		HLock(fHandle);
	}
	
	return GetElementPtr(0);
}
//***************************************************************************************************

void 	CArray::UnlockArray()
{
	#ifdef txtnDebug
	if (fLockLevel < 0)
		SignalPStr_("\p UnlockArray!!");
	#endif
	
	if (fLockLevel && (--fLockLevel == 0))
		HUnlock(fHandle);
}
//***************************************************************************************************

void*	CArray::GetElementPtr(long elementIndex) const
{
	#ifdef txtnDebug
		if ((elementIndex < -1) || (elementIndex > fLogicalCount))
		/*should be elementIndex >= fLogicalCount, but sometimes callers will not use the ptr
		, its only within a loop which not be entered */
			{SignalPStr_("\pBad element index");}
	#endif
	
	register long val = long(*fHandle) + elementIndex*fElementSize;
	
	return (fLockLevel > 0) ? StripAddress(Ptr(val)) : (void*)val;
}
//***************************************************************************************************

void* CArray::GetLastElementPtr() const
{
	return GetElementPtr(fLogicalCount-1);
}
//***************************************************************************************************

void CArray::SetElementsVal(long elementIndex, const void* newElementsPtr, long countElements /*= 1*/)
//Assumed to not disturb memory (see InsertElements for ex)
{
	#ifdef txtnDebug
		if ((elementIndex < 0) || (elementIndex+countElements > fLogicalCount))
			SignalPStr_("\pBad element index");
	#endif
	
	BlockMoveData(newElementsPtr, GetElementPtr(elementIndex), countElements*fElementSize);
}
//***************************************************************************************************

void CArray::GetElementsVal(long elementIndex, void* elementsPtr, long countElements /*= 1*/)
//Assumed to not disturb memory (see InsertElements for ex)
{
	#ifdef txtnDebug
		if ((elementIndex < 0) || (elementIndex+countElements > fLogicalCount))
			SignalPStr_("\pBad element index");
	#endif
	
	BlockMoveData(GetElementPtr(elementIndex), elementsPtr, countElements*fElementSize);
}
//***************************************************************************************************


void* CArray::InsertElements(long countToInsert, const void* newElementsVal, long insertIndex /* = -1*/)
{
	Assert_(fLockLevel == 0);
	Assert_(countToInsert >= 0);
	
	long oldElementsCount = fLogicalCount;
	long	newElementsCount = oldElementsCount + countToInsert;
	
	if (newElementsCount > fPhysicalCount) {
		long newActElementsCount;
		if (fMoreElementsCount > countToInsert)
			newActElementsCount = oldElementsCount + fMoreElementsCount;
		else
			newActElementsCount = newElementsCount;
		
		OSErr err = SetArraySize(newActElementsCount);
		
		if (err) {
			//try to not use fMoreElementsCount
			if (newActElementsCount != newElementsCount) { //==> newActElementsCount > newElementsCount
				err = SetArraySize(newElementsCount);
			}
			
			if (err) {return nil;}
		}
	}
	
	fLogicalCount = newElementsCount;
	
	if (insertIndex < 0)
		insertIndex = oldElementsCount;
	
	void* insertPtr = GetElementPtr(insertIndex);
	
	if (insertIndex != oldElementsCount)
		SetElementsVal(insertIndex+countToInsert, insertPtr, oldElementsCount-insertIndex);

	if (newElementsVal)
		SetElementsVal(insertIndex, newElementsVal, countToInsert);
		
	return insertPtr;
}
//***************************************************************************************************

void CArray::CheckUnusedCount()
{
	long register requiredPhysicalCount = fLogicalCount + fMoreElementsCount;
	
	if (fPhysicalCount > requiredPhysicalCount)
		SetArraySize(requiredPhysicalCount);
}
//***************************************************************************************************

long CArray::RemoveElements(long removeIndex, long countToRemove)
{
	Assert_(fLogicalCount - countToRemove >= 0);

	long srcMoveIndex = removeIndex + countToRemove;
	long countToMove = fLogicalCount - srcMoveIndex;

	if (countToMove > 0)
		SetElementsVal(removeIndex, GetElementPtr(srcMoveIndex), countToMove);

	fLogicalCount -= countToRemove;
	
	CheckUnusedCount();
	
	return fLogicalCount;
}
//***************************************************************************************************

OSErr CArray::ReplaceElements(long oldCount, long newCount, long replaceIndex, const void* newElements)
{
	Assert_(replaceIndex >= 0);
	Assert_(oldCount >= 0);
	Assert_(newCount >= 0);
	
	long countToAdd = newCount - oldCount;
	if (countToAdd > 0) {
		if (InsertElements(countToAdd, nil, replaceIndex) == nil)
			return memFullErr;
	}
	else {
		if (countToAdd < 0) {
			/*we don't use "this" since someone may override RemoveElements. our call here is actually
			an implementation detail and not required
			*/
			CArray::RemoveElements(replaceIndex, -countToAdd);
		}
	}
	
	if (newElements)
		SetElementsVal(replaceIndex, newElements, newCount);
	
	return noErr;
}
//***************************************************************************************************

OSErr	CArray::SetArraySize(long newElementsCount)
{
	if (newElementsCount != fPhysicalCount) {
		#ifdef txtnDebug
		if (newElementsCount > fPhysicalCount)
			Assert_(fLockLevel == 0);
		#endif
		
		SetHandleSize(fHandle, newElementsCount*fElementSize);
		
		OSErr theMemErr = MemError();
		if (!theMemErr)
			fPhysicalCount = newElementsCount;
		
		return theMemErr;
	}
	else
		return noErr;
}
//***************************************************************************************************

OSErr CArray::Reserve(long extraCount)
{
	OSErr err;
	if (extraCount > 0) {
		long currCount = fLogicalCount;
		err = SetElementsCount(currCount + extraCount);
		fLogicalCount = currCount;
	}
	else
		{err = noErr;}
	
	return err;
}
//***************************************************************************************************

OSErr CArray::SetElementsCount(long newCount)
{
	Assert_(newCount >= 0);

	OSErr err = noErr;
	if (newCount > fPhysicalCount) {
		if (!InsertElements(newCount-fLogicalCount, nil))
			err = memFullErr;
	}
	else {
		fLogicalCount = newCount;
		
		CheckUnusedCount();
	}
	
	return err;
}
//***************************************************************************************************

void CArray::Compact()
{
	SetArraySize(fLogicalCount);
}
//***************************************************************************************************


CLongTagArray::CLongTagArray()
{
}
//***************************************************************************************************

long CLongTagArray::Search(long val, long* foundVal) const
/*
	¥returns the index of the element whose entry starts with a long value == "val"
		, the insert index if not found
	¥ foundVal is set to the value at the found index
	¥ no special handling if the searched value is duplicated
*/
{
	register long left = 0;
	register long right = fLogicalCount-1;
	
	if (right < left) {//fLogicalCount == 0
		*foundVal = -1;
		return 0;
	}
	
	*foundVal = *(long*) GetElementPtr(0);
	if (val <= *foundVal) {return 0;}
	
	*foundVal = *(long*)GetLastElementPtr();
	if (val > *foundVal) {return fLogicalCount;}
	
	long insertIndex;
	do {
		long testIndex =  (left+right) >> 1; // /2
		long testedVal = *(long*)GetElementPtr(testIndex);

		long delta = testedVal-val;
		
		if (!delta) {
			*foundVal = val;
			return testIndex;
		}
		
		if (delta > 0) {
			right = testIndex-1;
			insertIndex = testIndex;
			*foundVal = testedVal;
		}
		else
			left = testIndex+1;
		
	}
	while (right >= left);
	
	return insertIndex;
}
//***************************************************************************************************

long CLongTagArray::SearchBigger(long val) const
/* ¥returns the index of the first element whose entry starts with a long > val
	¥ no special handling if the searched value is duplicated
*/
{
	long foundVal;
	long theIndex = Search(val, &foundVal);
	
	long lastElemIndex = fLogicalCount-1;
	if (theIndex > lastElemIndex)
		theIndex = lastElemIndex;
	else {
		if ((foundVal <= val) && (theIndex < lastElemIndex))
			++theIndex;
	}
	
	return theIndex;
}
//***************************************************************************************************
