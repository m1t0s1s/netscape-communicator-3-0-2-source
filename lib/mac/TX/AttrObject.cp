// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

#include "Array.h"
#include "TextensionCommon.h"
#include "AttrObject.h"



class CAttributes : public CArray {
	public:
		CAttributes();
		
		inline void IAttributes() {IArray(sizeof(CObjAttributes));}
		
		short InsertAttributes(const CAttrObject* theObj);
		//returns an "attrRef" which will be passed as input to other routines
		
		short CountAttributes(short attrRef) const;
		short AttrToIndex(AttrId theAttr, short attrRef) const; //-1 if doesn't exist
		AttrId IndexToAttr(short theIndex, short attrRef) const;
		AttrId GetNextAttr(short attrIndex, short attrRef);
		
	private:
		short GetObjectIndex(ClassId objClassId) const;
};

static CAttributes* gAttributes = nil;

#ifdef txtnDebug
static long gCountObjects = 0;
#endif
//***************************************************************************************************


OSErr InitObjects()
{
	gAttributes = new CAttributes;
	if (!gAttributes)
		return memFullErr;
	gAttributes->IAttributes();
	return noErr;
}
//***************************************************************************************************

void EndObjects()
{
	if (gAttributes) {
		gAttributes->Free();
		delete gAttributes;
	}
}
//***************************************************************************************************


CAttrObject::CAttrObject()
{
}
//***************************************************************************************************

void CAttrObject::IAttrObject()
{
	fAttr = kNoAttr;
	fAttrRef = gAttributes->InsertAttributes(this);
	
	fCountRefs = 1;

	#ifdef txtnDebug
	++gCountObjects;
	#endif
}
//***************************************************************************************************

//made in a resident seg since it may be called from TSMTextension in keydown (referenced objects, so, FreeData will not be called)
void CAttrObject::Free()
{
	Assert_(GetCountReferences() > 0);
	
	if (--fCountRefs == 0) {
		FreeData();

		#ifdef txtnDebug
		--gCountObjects;
/*		
		//try to hang if a deleted object is used
		long size = GetHandleSize((Handle)this);
		char* p = (char*)(*(Handle)this);
		while (--size >= 0) {*p++ = 0xFF;}
*/
		#endif

		delete this;
	}
}
//***************************************************************************************************


void CAttrObject::FreeData()
{
}
//***************************************************************************************************


static inline Boolean IsBitOn(unsigned long value, short bitNo)
{
	return (value & (1 << bitNo)) != 0;
}
//***************************************************************************************************

static inline void SetBit(unsigned long* value, short bitNo)
{
	*value |= (1 << bitNo);
}
//***************************************************************************************************

static inline void ClearBit(unsigned long* value, short bitNo)
{
	*value &= ~(1 << bitNo);
}
//***************************************************************************************************


CAttrObject* CAttrObject::Reference()
{
	++fCountRefs;
	return this;
}
//***************************************************************************************************

void CAttrObject::GetAttributes(CObjAttributes* /*attributes*/) const
{
}
//***************************************************************************************************

unsigned long CAttrObject::GetObjFlags() const
{
	return kNoFlags;
}
//***************************************************************************************************

void CAttrObject::SetDefaults(long /*message = 0*/)
{
}
//***************************************************************************************************

void CAttrObject::Invalid()
{
	Assert_(GetCountReferences() == 1);

	fAttr = kNoAttr;
}
//***************************************************************************************************

Boolean CAttrObject::HasAttribute(AttrId theAttr) const
{
	return gAttributes->AttrToIndex(theAttr, fAttrRef) >= 0;
}
//***************************************************************************************************

Boolean CAttrObject::IsAttributeON(AttrId theAttr) const
{
	short attrIndex = gAttributes->AttrToIndex(theAttr, fAttrRef);
	return (attrIndex >= 0) ? IsBitOn(fAttr, attrIndex) : false;
}
//***************************************************************************************************

void CAttrObject::SetAttributeON(AttrId theAttr)
{
//	Assert_(GetCountReferences() == 1);

	short bitNo = gAttributes->AttrToIndex(theAttr, fAttrRef);
	
	if (bitNo >= 0)
		SetBit(&fAttr, bitNo);
}
//***************************************************************************************************

void CAttrObject::Assign(const CAttrObject* srcObj, Boolean duplicate /*=true*/)
{
//	Assert_(GetCountReferences() == 1);

	if (srcObj != this) {
	//not only for optimization, but will not work correctly because of fAttr = kNoAttr.
		fAttr = kNoAttr;
		
		register short attrRef = fAttrRef;
		register short attrIndex = -1;
		register AttrId attr;
		
		while ((attr = gAttributes->GetNextAttr(++attrIndex, attrRef)) != 0) {
			if (srcObj->IsAttributeON(attr))
				if (duplicate)
					CloneAttributeFromObject(attr, srcObj);
				else {
					char srcAttr[kMaxAttrSize];
					srcObj->AttributeToBuffer(attr, srcAttr);
					SetAttributeValue(attr, srcAttr);
				}
		}
	}
}
//***************************************************************************************************

Boolean CAttrObject::SetCommonAttribute(AttrId theAttr, const void* valToCheck)
{
	return EqualAttribute(theAttr, valToCheck);
}
//***************************************************************************************************

Boolean CAttrObject::EqualAttribute(AttrId /*theAttr*/, const void* /*valToCheck*/) const
{
	#ifdef txtnDebug
	SignalPStr_("\p should not reach this!");
	#endif

	return false;
}
//***************************************************************************************************

void CAttrObject::AttributeToBuffer(AttrId /*theAttr*/, void* /*attrBuffer*/) const
{
}
//***************************************************************************************************
	
void CAttrObject::BufferToAttribute(AttrId /*theAttr*/, const void* /*attrBuffer*/)
{
}
//***************************************************************************************************


long CAttrObject::GetAttributeFlags(AttrId /*theAttr*/) const
{
	return 0;
}
//***************************************************************************************************

void CAttrObject::UpdateAttribute(AttrId theAttr, const void* srcAttr, long /*updateMessage*/
																	, const void* /*continuousAttr = nil*/)
{
	SetAttributeValue(theAttr, srcAttr);
}
//***************************************************************************************************

OSErr CAttrObject::CloneAttribute(AttrId theAttr, void* attrBuffer) const
{
	AttributeToBuffer(theAttr, attrBuffer);
	return noErr;
}
//***************************************************************************************************

void CAttrObject::CloneAttributeFromObject(AttrId theAttr, const CAttrObject* srcObj)
{
	char attrBuffer[kMaxAttrSize];
	srcObj->CloneAttribute(theAttr, attrBuffer);
	SetAttributeValue(theAttr, attrBuffer);
}
//***************************************************************************************************

long CAttrObject::Update(const CAttrObject* srcObj, long updateMessage, const CAttrObject* continuousObj /*=nil*/)
{
	register short attrRef = fAttrRef;
	register short attrIndex = -1;
	register AttrId attr;
	
	long feedBackFlags = 0;
	
	while ((attr = gAttributes->GetNextAttr(++attrIndex, attrRef)) != 0) {
		if (!srcObj->IsAttributeON(attr) || !HasAttribute(attr))
			continue;
		
		Assert_(GetCountReferences() == 1);
		
		if (IsBitOn(fAttr, attrIndex)) {
			char srcAttr[kMaxAttrSize];
			srcObj->AttributeToBuffer(attr, srcAttr);
			
			void* contAttrPtr;
			char contAttr[kMaxAttrSize];
			if (continuousObj && continuousObj->GetAttributeValue(attr, contAttr))
				contAttrPtr = contAttr;
			else
				contAttrPtr = nil;
			
			UpdateAttribute(attr, srcAttr, updateMessage, contAttrPtr);
		}
		else
			CloneAttributeFromObject(attr, srcObj);
		
		feedBackFlags |= GetAttributeFlags(attr);
	}
	
	return feedBackFlags;
}
//***************************************************************************************************

long CAttrObject::Update(AttrId theAttr, const void* attrBuffer, long updateMessage
										, const CAttrObject* continuousObj /*=nil*/)
{
	Assert_(GetCountReferences() == 1);

	void* contAttrPtr;
	char contAttr[kMaxAttrSize];
	if (continuousObj && continuousObj->GetAttributeValue(theAttr, contAttr))
		contAttrPtr = contAttr;
	else
		contAttrPtr = nil;
	
	UpdateAttribute(theAttr, attrBuffer, updateMessage, contAttrPtr);
	return GetAttributeFlags(theAttr);
}
//***************************************************************************************************

long CAttrObject::Update(const AttrObjModifier& modifier, const CAttrObject* continuousObj /*=nil*/)
{
	return (modifier.fObj) ? Update(modifier.fObj, modifier.fMessage, continuousObj)
					: Update(modifier.fAttr, &modifier.fAttrValue, modifier.fMessage, continuousObj);
}
//***************************************************************************************************

#ifdef txtnDebug
Boolean CAttrObject::HasAllAttributes()
{
	register short attrRef = fAttrRef;
	register short attrIndex = -1;
	register AttrId attr;
	
	while ((attr = gAttributes->GetNextAttr(++attrIndex, attrRef)) != 0) {
		if (HasAttribute(attr) && !IsBitOn(fAttr, attrIndex))
			return false;
	}
	
	return true;
}
//***************************************************************************************************
#endif

Boolean CAttrObject::Common(const CAttrObject* anObj)
/*
	set the various attributes to the common ones (the ones equal in "this" and "anObj"), returns true
	if something is commmon, otherwise returns false.
	The non common attributes are set to 0.
	Useful to get the "Continuous" fn when a range have many objs.
*/
{
	Assert_(GetCountReferences() == 1);

	Boolean somethingCommon = false;

	register short attrRef = fAttrRef;
	register short attrIndex = -1;
	register AttrId attr;
	
	while ((attr = gAttributes->GetNextAttr(++attrIndex, attrRef)) != 0) {
		if (!IsBitOn(fAttr, attrIndex) || !anObj->IsAttributeON(attr)) {continue;}
		
		char attrBuffer[kMaxAttrSize];
		anObj->AttributeToBuffer(attr, attrBuffer);

		if (SetCommonAttribute(attr, attrBuffer))
			somethingCommon = true;
		else
			ClearBit(&fAttr, attrIndex);
	}
	
	return somethingCommon;
}
//***************************************************************************************************

Boolean CAttrObject::IsEqual(const CAttrObject* objectToCheck) const
{
	//	new jah
	if (objectToCheck == this)
		return true;

	register unsigned long attrToCheck = objectToCheck->fAttr;

	if ((GetClassId() != objectToCheck->GetClassId()) || (fAttr != attrToCheck))
		return false;

	register short attrRef = fAttrRef;
	register short attrIndex = -1;
	register AttrId attr;
	
	while ((attr = gAttributes->GetNextAttr(++attrIndex, attrRef)) != 0) {
		if (IsBitOn(fAttr, attrIndex) && IsBitOn(attrToCheck, attrIndex)) {
			char attrBuffer[kMaxAttrSize];
			objectToCheck->AttributeToBuffer(attr, attrBuffer);
			
			if (!EqualAttribute(attr, attrBuffer))
				return false;
		}
	}
	
	return true;
}
//***************************************************************************************************


Boolean CAttrObject::GetAttributeValue(AttrId theAttr, void* attrBuffer) const
{
	if (IsAttributeON(theAttr)) {
		AttributeToBuffer(theAttr, attrBuffer);
		return true;
	}
	else
		return false;
}
//***************************************************************************************************

void CAttrObject::SetAttributeValue(AttrId theAttr, const void* attrBuffer)
{
//	Assert_(GetCountReferences() == 1);

	BufferToAttribute(theAttr, attrBuffer);
	
	SetAttributeON(theAttr);
}



//***************************************************************************************************



void CObjAttributes::IObjAttributes(ClassId classId)
{
	fObjClass = classId;
	fCount = 0;
}
//***************************************************************************************************


void CObjAttributes::Add(AttrId attr)
{
	fAttributes[fCount++] = attr;
}
//***************************************************************************************************


CAttributes::CAttributes()
{
}
//***************************************************************************************************


short  CAttributes::GetObjectIndex(ClassId objClassId) const
//returns -1 if not found
{
	register short countObjects = short(CountElements());
	
	if (countObjects) {
		register CObjAttributes* objAttrPtr = (CObjAttributes*) GetElementPtr(0);
		
		for (register short attrRef = 0; attrRef < countObjects; ++attrRef, ++objAttrPtr) {
			if (objAttrPtr->GetObjClass() == objClassId)
				return attrRef;
		}
	}
	
	return -1;
}
//***************************************************************************************************

short CAttributes::InsertAttributes(const CAttrObject* theObj)
//return -1 if mem problems
{
	ClassId objClassId = theObj->GetClassId();
	short attrIndex = GetObjectIndex(objClassId);
	
	if (attrIndex < 0) {
		attrIndex = short(CountElements());
		
		CObjAttributes* objAttributes = (CObjAttributes*) InsertElements(1, nil);
		if (!objAttributes)
			return -1;
		else {
			LockArray();
			
			objAttributes->IObjAttributes(objClassId);
			
			theObj->GetAttributes(objAttributes);
			
			UnlockArray();
		}
	}
	
	return attrIndex;
}
//***************************************************************************************************

short CAttributes::AttrToIndex(AttrId theAttr, short attrRef) const
//returns -1 if 'theAttr' doesn't exist for this 'attrRef'
{
	CObjAttributes* objAttributes = (CObjAttributes*) GetElementPtr(attrRef);
	
	register short countAttr = objAttributes->Count();
	
	for (register short attrIndex = 0; attrIndex < countAttr; ++attrIndex) {
		if (objAttributes->GetIndAttribute(attrIndex) == theAttr)
			return attrIndex;
	}
	
	return -1;
}
//***************************************************************************************************

AttrId CAttributes::IndexToAttr(short theIndex, short attrRef) const
{
	return ((CObjAttributes*) GetElementPtr(attrRef))->GetIndAttribute(theIndex);
}
//***************************************************************************************************

short CAttributes::CountAttributes(short attrRef) const
{
	return ((CObjAttributes*) GetElementPtr(attrRef))->Count();
}
//***************************************************************************************************

AttrId CAttributes::GetNextAttr(short attrIndex, short attrRef)
{
	CObjAttributes* objAttributes = (CObjAttributes*) GetElementPtr(attrRef);
	return (attrIndex < objAttributes->Count()) ? objAttributes->GetIndAttribute(attrIndex) : 0;
}
//***************************************************************************************************



AttrObjModifier::AttrObjModifier(CAttrObject* obj, long message /*= 0*/)
{
	fObj = obj;
	fMessage = message;
}
//***************************************************************************************************

AttrObjModifier::AttrObjModifier(AttrId attr, const void* buffer, long message /*= 0*/)
{
	fObj = nil;
	fAttr = attr;
	BlockMoveData(buffer, &fAttrValue, kMaxAttrSize);
	fMessage = message;
}
//***************************************************************************************************

AttrObjModifier::AttrObjModifier()
{
	fObj = nil;
	fMessage = 0;
}
//***************************************************************************************************

void AttrObjModifier::Set(CAttrObject* obj, long message /*= 0*/)
{
	fObj = obj;
	fMessage = message;
}
//***************************************************************************************************

void AttrObjModifier::Set(AttrId attr, const void* buffer, long message /*= 0*/)
{
	fObj = nil;
	fAttr = attr;
	BlockMoveData(buffer, &fAttrValue, kMaxAttrSize);
	fMessage = message;
}
//***************************************************************************************************
