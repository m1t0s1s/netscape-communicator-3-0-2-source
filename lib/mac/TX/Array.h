// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

#pragma once


#include "TextensionCommon.h"


#ifdef txtnDebug
extern short gCountArrays;
#endif

class	CArray
{
	public:
		CArray();
		
		void IArray(short arrayElementSize, short moreElementsCount = 0);
		 
		virtual void Free();
		
		void* LockArray(Boolean moveHi = false);
		void	UnlockArray();
		
		void*	GetElementPtr(long elementIndex) const;
		void* GetLastElementPtr() const;
		
		void SetElementsVal(long elementIndex, const void* newElementsPtr, long countElements = 1);
		void GetElementsVal(long elementIndex, void* elementsPtr, long countElements = 1);
		
		void* InsertElements(long countToInsert, const void* newElementsVal, long insertIndex=-1);
		//-1 means at the end. newElementsVal may be nil
		
		virtual long RemoveElements(long removeIndex, long countToRemove);
		//returns the new elements count
		
		OSErr ReplaceElements(long oldCount, long newCount, long replaceIndex, const void* newElements);
		//newElements may be nil
		
		inline long CountElements() const {return fLogicalCount;}
		
		OSErr SetElementsCount(long newCount);
		void Compact();
	
		OSErr Reserve(long extraCount);
		
		inline short GetElementSize() {return fElementSize;}
		
	protected:
		long fLogicalCount;
		short fElementSize;

	private:
		Handle fHandle;
		
		short fLockLevel;
		short fMoreElementsCount;
		long fPhysicalCount;

		OSErr	SetArraySize(long newElementsCount);
		
		void CheckUnusedCount();

//	jah	950130...
	public:
		virtual	Handle	GetDataHandle(void);
		virtual	void	SetDataHandle(Handle inHandle, short inElementSize = 1);
};
//***************************************************************************************************

class	CLongTagArray	:	public	CArray {
//Array of elements starting with a long
	public:
		CLongTagArray();
		
		long Search(long val, long* foundVal) const;

		long SearchBigger(long val) const;
		
	private:
};
//***************************************************************************************************

