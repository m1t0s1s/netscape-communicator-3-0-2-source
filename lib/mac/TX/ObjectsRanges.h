// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

#pragma once


#include "TextensionCommon.h"
#include "Array.h"
#include "AttrObject.h"


//***************************************************************************************************


struct TSectRanges {
	long leadIndex; //range index
	long leadCount; //count of range elements in the first range that are not inside
	long leadSectCount; //count of range elements in the first range that are inside
	
	long insideIndex; //range index
	long insideCount; //ranges count
	
	long trailIndex; //range index
	long trailCount; //count of range elements in the last range that are not inside
	long trailSectCount; //count of range elements in the last range that are inside
};
//***************************************************************************************************


class CRanges : public CLongTagArray
{
	public:
//------
	CRanges();
	
	inline void IRanges(short elementSize, short moreElements = 1)
		{this->IArray(elementSize, moreElements);}
	
	virtual void FreeData(Boolean compact = true);

	long GetRangeEnd(long rangeIndex) const;
	long GetRangeStart(long rangeIndex) const;
	long GetRangeLen(long rangeIndex) const;
	
	void SetRangeEnd(long rangeIndex, long newEnd);
	
	void GetRangeBounds(long rangeIndex, TOffsetPair* bounds) const;
	
	inline Boolean IsRangeEmpty(long rangeIndex) const {return this->GetRangeLen(rangeIndex) == 0;}
	
	Boolean IsRangeStart(long offset, long rangeIndex = -1) const;

	long GetLastRangeEnd() const;
	
	long OffsetToRangeIndex(TOffset rangeOffset) const;
	
	void OffsetRanges(long valToAdd, long firstIndex, long count = -1);
	//count < 0 ==> offset up to last range
	
	long SectRanges(long offset, long count, TSectRanges* sectInfo) const;
	
	
	#ifdef txtnDebug
	virtual void CheckCoherence(long count);
	#endif
	
};


class CObjectsRanges : public CRanges
{
	public:
								CObjectsRanges();
		void 					IObjectsRanges(CAttrObject* continuousObj, short moreElements = 1);
		
		//¥override
		virtual void 			Free();
		virtual void 			FreeData(Boolean compact = true);
	
		virtual long 			RemoveElements(long removeIndex, long countToRemove);

		//returns the new elements count
		
		//¥own
		virtual CAttrObject* OffsetToObject(TOffset offset);
	
		CAttrObject* RangeIndexToObject(short rangeIndex) const;
	
		short GetNextObjectRange(long offset, CAttrObject** theObj, long* rangeLen) const;
		// returns the range index, -1 if no next
		
		OSErr ReplaceRange(long offset, long oldLen, long newLen
											, CAttrObject* theObject = nil, Boolean takeObjectCopy =true);
		/*
			-if oldLen == 0 ==> acts as Insert
			-if newLen == 0 ==> acts a delete, in this case "theObject" is not used
			-if theObject is nil the object at "offset" is used
		*/
		
		OSErr ReplaceRange(long offset, long oldLen, CObjectsRanges* newRanges, Boolean plug = false);
	
		OSErr InsertObjectRange(short insertIndex, long rangeEnd, CAttrObject* rangeObj, Boolean reference);
		
		virtual long UpdateRangeObjects(long startOffset, long len, const AttrObjModifier& modifier);
	
		virtual CAttrObject* GetContinuousObj(long startOffset, long len);
	
		inline short CountRanges() const {return this->CountElements();}
		
		inline void SetFreeObjects(char free) {fFreeObjects = free;}
		
		#ifdef txtnDebug
		short CountUniqueObjects();
		virtual void CheckCoherence(long count);
		#endif
		
		
	protected :
	//---------
	CAttrObject*		fContinuousObj;
	
	private:
	//------
	long fLastContStart; //-1 ==> GetContinuousObj will recalc fContinuousObj
	long fLastContLen;
	
	CAttrObject* fLastSearchedObj;
	char fFreeObjects;
	
		CAttrObject* MapObject(CAttrObject* newObj, Boolean takeObjectCopy, Boolean* reference);
		CAttrObject* SearchObject(const CAttrObject* objToCheck);
		
		void SetObjectRange(short rangeIndex, long newRangeEnd, CAttrObject* newObj, Boolean reference);
		
		void ReplaceRangeObj(long offset, long len, CAttrObject* newObj, Boolean reference);
				
		Boolean UpdateRangesBounds(long startOffset, long endOffset, CAttrObject* newObj
																, short* firstRange, short* lastRange);	
		
		
		void ClearRange(long offset, long len);
		
		void FreeObjects(short firstIndex = 0, short lastIndex = -1);
		//frees the objects in the range firstIndex..lastIndex inclusive
		//no params ==> free all objects
};



