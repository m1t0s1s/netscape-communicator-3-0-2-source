// ===========================================================================
//	UAEGizmos.cp				©1994-5 Metrowerks Inc. All rights reserved.
// ===========================================================================
//
//	AEGizmo wrapper classes that make dealing with AppleEvents much simpler.

	
#ifdef PowerPlant_PCH
#include PowerPlant_PCH
#endif

#define	USE_AE_GIZMOS

#include	"UAEGizmos.h"
#include	<StClasses.h>

#ifndef		__ERRORS__
#include	<Errors.h>
#endif

/*
	This UAEGizmos source file includes functions that depend
	on the AEGizmo AESubDesc format.
*/

//	===========================================================================
//	LAESubDesc:
//	===========================================================================
//	Class used to get data out of an AEDesc.
//
//	Note that you can't open an LAESubDesc on any part of an AppleEvent or
//	AEDesc that is under construction!

LAESubDesc::LAESubDesc()
{
	mSubDesc.subDescType = typeNull;
	mSubDesc.dataHandle = NULL;
	mSubDesc.offset = 0;

	//	Anyone have a suggestion on how to make this check at compile time?
//?	Assert_(sizeof(LAESubDesc) <= maxSize);
}

LAESubDesc::LAESubDesc(const AEDesc &inDesc)
{
	mSubDesc.subDescType = typeNull;
	mSubDesc.dataHandle = NULL;
	mSubDesc.offset = 0;
	
	AEDescToSubDesc(&inDesc, &mSubDesc);
}

LAESubDesc::LAESubDesc(const LAESubDesc &inSubDesc, DescType inCoercedType)
{
	if ((inCoercedType == typeWildCard) || (inCoercedType == inSubDesc.GetType())) {
		mSubDesc = inSubDesc.mSubDesc;
	} else {
		mSubDesc.subDescType = typeNull;
		mSubDesc.dataHandle = NULL;
		mSubDesc.offset = 0;
		
		inSubDesc.ToDesc(&mDesc.mDesc, inCoercedType);
		AEDescToSubDesc(&mDesc.mDesc, &mSubDesc);
	}		
}

LAESubDesc::~LAESubDesc()
{
}

LAESubDesc&	LAESubDesc::operator=(const LAESubDesc &inRhs)
{
	//	Don't doubly assign to self.
	//	This check works only if there are no derived classes from LAESubDesc.
	if (&inRhs == this)
		return *this;
		
	if (inRhs.mDesc.mDesc.descriptorType == typeNull) {
		mSubDesc = inRhs.mSubDesc;
	} else {
		inRhs.ToDesc(&mDesc.mDesc);
		AEDescToSubDesc(&mDesc.mDesc, &mSubDesc);
	}
	
	return *this;
}

DescType	LAESubDesc::GetType(void) const
{
	return AEGetSubDescType(&mSubDesc);
}

DescType	LAESubDesc::GetBasicType(void) const
{
	return AEGetSubDescBasicType(&mSubDesc);
}

Boolean	LAESubDesc::IsListOrRecord(void) const
{
	return AESubDescIsListOrRecord(&mSubDesc);
}

void *	LAESubDesc::GetDataPtr(void) const
/*
	The returned pointer is invalid after the next memory move
*/
{
	void	*data;
	long	length;
	
	data = AEGetSubDescData(&mSubDesc, &length);

	return data;
}

Uint32	LAESubDesc::GetDataLength(void) const
{
	void	*data;
	long	length;
	
	data = AEGetSubDescData(&mSubDesc, &length);
	
	if (data)
		return length;
	else
		return 0;
}

void	LAESubDesc::ToDesc(AEDesc *outDesc, DescType inType) const
{
	OSErr	err;
	
	//	Don't trust AESubDescToDesc w/ typeWildCard...
	if (inType == typeWildCard) {
		inType = GetType();
		
		if (inType == typeNull) {

			//	If this subdesc is type null, set the outDesc
			//	accordingly.

			//	This special case is necessary otherwise
			//	AESubDescToDesc will try and convert to
			//	typeNull and that coercion doesn't seem
			//	to be defined.
			
			err = AEDisposeDesc(outDesc);	//	leak preventative?
			ThrowIfOSErr_(err);
			return;
		}
	}
	
	err = AEDisposeDesc(outDesc);
	ThrowIfOSErr_(err);
	
	err = AESubDescToDesc(&mSubDesc, inType, outDesc);
	ThrowIfOSErr_(err);
}	

Uint32	LAESubDesc::CountItems(void) const
/*
	Unlike AEGizmos, if !IsListOrRecord this CountItems will:
		return 0 if GetType == typeNull,
		return 1 if GetType != typeNull.
		
	If IsListOrRecord, this countItems will return the expected
	value.
*/
{
	if (!IsListOrRecord()) {
		if (GetType() == typeNull)
			return 0;
		else 
			return 1;
	}
		
	long	rval = AECountSubDescItems(&mSubDesc);
	
	if (rval < 0)
		ThrowIfOSErr_(rval);
	
	return rval;
}

LAESubDesc	LAESubDesc::NthItem(long inIndex, AEKeyword *outKeyword) const
/*
	If this is a non-record or list subdesc:

		This function will return a copy of itself IFF
			InIndex is one.
			
		outKeyword, if present, will be set type typeWildCard.
	
	Otherwise, this function must be called on a list or record subdesc.
*/
{
	AEKeyword	keyword;
	LAESubDesc	rval;
	
	if (!IsListOrRecord() && inIndex == 1 && (GetType() != typeNull)) {
		if (outKeyword)
			*outKeyword = typeWildCard;
		return *this;
	}
	
	OSErr	err = AEGetNthSubDesc(&mSubDesc, inIndex, &keyword, &rval.mSubDesc);
	ThrowIfOSErr_(err);
	
	if (outKeyword)
		*outKeyword = keyword;
	
	return rval;
}

LAESubDesc	LAESubDesc::KeyedItem(AEKeyword inKeyword) const
/*
	This function will NOT cause an exception if the given key doesn't
	exist!  Instead, it will just substitute the null descriptor and an
	exception will be thrown when trying to get data out of it.  This way,
	this method is more flexible in dealing with "optional" parameters.
*/
{
	LAESubDesc	rval;

	OSErr	err = AEGetKeySubDesc(&mSubDesc, inKeyword, &rval.mSubDesc);
	
	return rval;
}

Boolean	LAESubDesc::KeyExists(AEKeyword inKeyword) const
{
	Boolean	rval = false;
	
	if ((GetBasicType() == typeAERecord) || (GetBasicType() == typeAppleEvent)) {
		AESubDesc	item;
		OSErr	err = AEGetKeySubDesc(&mSubDesc, inKeyword, &item);
		if (err == noErr)
			rval = true;
	}
		
	return rval;
}

//	===========================================================================
//	LAEStream:
//	===========================================================================
//
//	Class used to help construct AEDesc's & AppleEvents.
//
//	Note that you can't open an LAESubDesc on any part of an AppleEvent or
//	AEDesc that is under construction!

LAEStream::LAEStream()
{
	OSErr	err = AEStream_Open(&mAEStream);
	ThrowIfOSErr_(err);
	
	//	Anyone have a suggestion on how to make this check at compile time?
//?	Assert_(sizeof(LAEStream) <= maxSize);
}

LAEStream::LAEStream(
	AEEventClass	inClass,
	AEEventID		inID,
	DescType		inTargetType,
	const void		*inTargetData,
	long			inTargetLength,
	short			inReturnID,
	long			inTransactionID)
{
	ProcessSerialNumber	currProcess;
	
	if (inTargetType == typeNull) {
		inTargetType = typeProcessSerialNumber;
		Assert_(inTargetData == NULL);
		Assert_(inTargetLength == NULL);
		
		currProcess.highLongOfPSN = 0;
		currProcess.lowLongOfPSN = kCurrentProcess;
		
		inTargetData = &currProcess;
		inTargetLength = sizeof(currProcess);
	}
	
	OSErr	err = AEStream_CreateEvent(&mAEStream, inClass, inID,
					inTargetType, inTargetData, inTargetLength, inReturnID, inTransactionID);
	ThrowIfOSErr_(err);
}

LAEStream::~LAEStream()
{
}

void	LAEStream::Close(AEDesc *outDesc)
{
	ThrowIfNULL_(outDesc);	//	no output descriptor was ever specified!
	 
	OSErr	err = AEStream_Close(&mAEStream, outDesc);
	ThrowIfOSErr_(err);
}

void	LAEStream::WriteDesc(const AEDesc &desc)
{
	OSErr	err = AEStream_WriteAEDesc(&mAEStream, &desc);
	
	ThrowIfOSErr_(err);
}

void	LAEStream::WriteDesc(DescType inType, const void *inData, Size inLength)
/*
	Default inData and inLength parameters will result in a descriptor of type
	inType with a null data handle.
*/
{
	OSErr	err = AEStream_WriteDesc(&mAEStream, inType, inData, inLength);
	
	ThrowIfOSErr_(err);
}

void	LAEStream::OpenDesc(DescType inType)
{
	OSErr	err = AEStream_OpenDesc(&mAEStream, inType);
	
	ThrowIfOSErr_(err);
}

void	LAEStream::WriteData(const void *inData, Size inLength)
{
	OSErr	err = AEStream_WriteData(&mAEStream, inData, inLength);
	
	ThrowIfOSErr_(err);
}

void	LAEStream::CloseDesc(void)
{
	OSErr	err = AEStream_CloseDesc(&mAEStream);
	
	ThrowIfOSErr_(err);
}

void	LAEStream::OpenList(void)
{
	OSErr	err = AEStream_OpenList(&mAEStream);
	
	ThrowIfOSErr_(err);
}

void	LAEStream::CloseList(void)
{
	OSErr	err = AEStream_CloseList(&mAEStream);
	
	ThrowIfOSErr_(err);
}

void	LAEStream::OpenRecord(DescType inType)
{
	OSErr	err = AEStream_OpenRecord(&mAEStream, inType);
	
	ThrowIfOSErr_(err);
}

void	LAEStream::CloseRecord(void)
{
	OSErr	err = AEStream_CloseRecord(&mAEStream);
	
	ThrowIfOSErr_(err);
}

void	LAEStream::SetRecordType(DescType inType)
{
	OSErr	err = AEStream_SetRecordType(&mAEStream, inType);
	
	ThrowIfOSErr_(err);
}

void	LAEStream::WriteKeyDesc(AEKeyword inKeyword, DescType inType, void *inData, Size inLength)
{
	OSErr	err = AEStream_WriteKeyDesc(&mAEStream, inKeyword, inType, inData, inLength);
	
	ThrowIfOSErr_(err);
}

void	LAEStream::OpenKeyDesc(AEKeyword inKeyword, DescType inType)
{
	OSErr	err = AEStream_OpenKeyDesc(&mAEStream, inKeyword, inType);
	
	ThrowIfOSErr_(err);
}

void	LAEStream::WriteKey(AEKeyword inKeyword)
{
	OSErr	err = AEStream_WriteKey(&mAEStream, inKeyword);
	
	ThrowIfOSErr_(err);
}
