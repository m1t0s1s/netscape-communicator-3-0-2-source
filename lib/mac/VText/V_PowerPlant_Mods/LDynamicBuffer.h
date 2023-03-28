#pragma	once

//	===========================================================================
//	LDynamicBuffer.h				©1994 Metrowerks Inc. All rights reserved.
//	===========================================================================

#include	<PP_Prefix.h>

//	NOTE:
//	
//	Unlike other PowerPlant collection classes, offsets in LDynamicBuffer
//	are zero based.

class	LDynamicBuffer
{
public:
	
				//	Maintenance
						LDynamicBuffer();
						LDynamicBuffer(const LDynamicBuffer &inBuffer);
	virtual				~LDynamicBuffer();
	
				//	General
	virtual void		Clear(void);
	virtual Int32		GetSize(void) const;
	
				//	Removal
	virtual void		Delete(Int32 inOffset, Int32 inAmount);
	
				//	Data placement
	virtual void *		Insert(Int32 inOffset, const void *inData, Int32 inDataSize);
	virtual void *		Append(const void *inData, Int32 inDataSize) {return Insert(GetSize(), inData, inDataSize);} 
	virtual void *		Prepend(const void *inData, Int32 inDataSize) {return Insert(0, inData, inDataSize);}

				//	Data query
	virtual void		GetData(Int32 inOffset, Int32 inAmount, void *outData) const;

				//	Implementation details (avoid using)
	virtual void *		GetOffsetPtr(Int32 inOffset) const;
	Handle				mData;
protected:
	virtual void		Shift(Int32 inOffset, Int32 inDelta);
};