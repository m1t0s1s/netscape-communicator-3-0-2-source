// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

#pragma once

#include "Array.h"


//***************************************************************************************************

const long kNoAttr	=	0;
//***************************************************************************************************

//constants returned from GetObjFlags

const unsigned long kNoFlags = 0;

const unsigned long kHasCustomHilite = 1 << 0;

const unsigned long kIndivisibleRun = 1 << 1;
/*
	-always selected as block (clicking or shift arrow keys)
	-cleared as block (back space)
*/

const unsigned long kFixedRangeRun = 1 << 2;
/*
	-once defined, the obj range can not be modified: other chars can not be added to or deleted from
		the same range (graphics, foot notes, TSM runs..)
*/
//***************************************************************************************************



/*constants for values returned as a feed back flags from certain object routines
(GetAttributeFlags, eventually mouseDown, ..) */
const long kReformat = 1 << 0;
const long kVReformat = 1 << 1;

typedef unsigned long ClassId;

typedef unsigned long AttrId;

const short kMaxCountAttr = 32;

const short kMaxAttrSize = 8; //remember that kUserTabsAttr is 80 bytes

//***************************************************************************************************

OSErr InitObjects();
void EndObjects();
//***************************************************************************************************


struct AttrObjModifier; //forward

class CObjAttributes; //forward

class	CAttrObject
{
public:
	CAttrObject();
	
	void	IAttrObject();
	virtual	void Free();
	
	virtual void FreeData();
	
	virtual CAttrObject* CreateNew() const = 0;
	//Just do new of your class type and initialize it
	
	virtual CAttrObject* Reference();
	
	virtual	inline long GetCountReferences() {return fCountRefs;}
	
	virtual ClassId GetClassId() const = 0;
	//should be unique for each CAttrObject descendent (i.e CQDTextRun and all descendents from it have different one)
	
	virtual void GetAttributes(CObjAttributes* attributes) const;
	
	virtual unsigned long GetObjFlags() const;
	
	Boolean HasAttribute(AttrId theAttr) const;
	
	virtual void Assign(const CAttrObject* srcObj, Boolean duplicate = true);

	virtual void SetDefaults(long message = 0);
	
	long Update(const AttrObjModifier& modifier, const CAttrObject* continuousObj =nil);
	
	long Update(const CAttrObject* srcObj, long updateMessage = 0, const CAttrObject* continuousObj =nil);
	long Update(AttrId theAttr, const void* attrBuffer, long updateMessage
							, const CAttrObject* continuousObj =nil);
	/*updateMessage, continuousObj to be passed to UpdateAttribute. The returned value is a feedBack
	flags. for each updated attribute "GetAttributeFlags" is called, the ORed result is returned
	to the caller.
	*/
	
	Boolean Common(const CAttrObject* anObj);
	Boolean IsEqual(const CAttrObject* objectToCheck) const;
	
	inline Boolean IsInvalid() const {return (fAttr == kNoAttr);}
	virtual void Invalid();
	
	#ifdef txtnDebug
	Boolean HasAllAttributes();
	#endif
	
	Boolean GetAttributeValue(AttrId theAttr, void* attrBuffer) const;
	virtual void SetAttributeValue(AttrId theAttr, const void* attrBuffer);
	
	Boolean IsAttributeON(AttrId theAttr) const;
	
protected:
	void SetAttributeON(AttrId theAttr);
	
	virtual Boolean EqualAttribute(AttrId theAttr, const void* valToCheck) const;
	//false by default
	
	virtual Boolean SetCommonAttribute(AttrId theAttr, const void* valToCheck);
	
	virtual long GetAttributeFlags(AttrId theAttr) const;
	//see comment after "Update"
	
	virtual void UpdateAttribute(AttrId theAttr, const void* srcAttr, long updateMessage
															, const void* continuousAttr = nil);

	virtual void AttributeToBuffer(AttrId theAttr, void* attrBuffer) const;
	
	virtual void BufferToAttribute(AttrId theAttr, const void* attrBuffer);
	
	virtual OSErr CloneAttribute(AttrId theAttr, void* attrBuffer) const;
	
private:
	unsigned long	fAttr;
	short fAttrRef;
	long fCountRefs;
	
	void CloneAttributeFromObject(AttrId theAttr, const CAttrObject* srcObj);
};
//**************************************************************************************************


struct AttrObjModifier {
AttrObjModifier(); //initialize to nils

AttrObjModifier(AttrId attr, const void* buffer, long message = 0);

AttrObjModifier(CAttrObject* obj, long message = 0);


void Set(AttrId attr, const void* buffer, long message = 0); //equivalent to the contructor AttrObjModifier(AttrId attr, const void* buffer, long message = 0);

void Set(CAttrObject* obj, long message = 0); //equivalent to the contructor AttrObjModifier(CAttrObject* obj, long message = 0);

//fields
//------
CAttrObject* fObj;

AttrId fAttr;
char fAttrValue[kMaxAttrSize];

long fMessage;
};
/*
	AttrObjModifier is used to describe how to modify (or update) a CAttrObject object.
	CAttrObject accepts this type as input to its "Update" member. CTextension accepts this type as
	input to its "UpdateRangeRuns" and "UpdateRangeRulers" members.
	
	A CAttrObject object (targetObject) can be modified by a AttrObjModifier in one of two ways:
	1) by attribute and value. In this case AttrObjModifier is constructed by
				AttrObjModifier(AttrId attr, const void* buffer, long message = 0);
		It means modify the targetObject's attribute "attr" by the value pointed by "buffer".
		
		the "message" param can have different meaning depending on "targetObject" and the particular
		attribute. CQDTextRun, for instance, accepts a "kAddSize" for its font size attribute.
		In this case, "targetObject" will interpret the short pointed by "buffer" as a value to be added
		to its current font size.
	
	2) by another "CAttrObject" (sourceObject).In this case AttrObjModifier is constructed by
		AttrObjModifier(CAttrObject* obj, long message = 0);
		
		It means modify all targetObject's attributes which are valid in "sourceObject" (a valid attribute 
		means that it has been set by "SetAttributeValue").
		
		"message" has the same meaning as in 1.
*/
//***************************************************************************************************

class CObjAttributes {
public:
	void IObjAttributes(ClassId classId);
	
	void Add(AttrId attr);
	
	inline ClassId GetObjClass() {return fObjClass;}
	
	inline short Count() const {return fCount;} 
	inline AttrId GetIndAttribute(short index) const {return fAttributes[index];}
	
private:
	ClassId fObjClass;
	short fCount;
	AttrId	fAttributes[kMaxCountAttr];
};


