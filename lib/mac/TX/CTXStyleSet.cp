// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	CTXStyleSet.cp						  ©1995 Netscape. All rights reserved.
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

#include	"CTXStyleSet.h"
#include	<UAppleEventsMgr.h>
#include	<UAEGizmos.h>
#include	"QDTextRun.h"

#ifndef		__AEREGISTRY__
#include	<AERegistry.h>
#endif

#define	CStyleSetObject	CTXStyleSet

CTXStyleSet::CTXStyleSet()
{
	IStyleSetObject();
	SetDefaults();
	
	SetAttributeValue(kPPStyleSet, &this);
}

CTXStyleSet::CTXStyleSet(const LStyleSet *inOriginal)
:	LStyleSet(inOriginal)
{
	IStyleSetObject();
	if (inOriginal) {
//		Assert_(member(inOriginal, CTXStyleSet));
		Assign((CTXStyleSet *)inOriginal);
	} else {
		SetDefaults();
	}
	
	SetAttributeValue(kPPStyleSet, &this);
}

CTXStyleSet::~CTXStyleSet()
{
}

void	CTXStyleSet::GetAEProperty(
	DescType		inProperty,
	const AEDesc	&inRequestedType,
	AEDesc			&outPropertyDesc) const
{
	OSErr	err;
	
	switch (inProperty) {

		case pScriptTag:
			Throw_(errAENotAnElement);
			break;

		case pPointSize:
		{
			Int16	size;

			if (GetAttributeValue(kFontSizeAttr, &size)) {
				err = AECreateDesc(typeShortInteger, &size, sizeof(size), &outPropertyDesc);
				ThrowIfOSErr_(err);
			} else {
				Throw_(errAENotAnElement);
			}
			break;
		}

		case pColor:
		{
			RGBColor	color;

			if (GetAttributeValue(kForeColorAttr, &color)) {
				err = AECreateDesc(cRGBColor, &color, sizeof(color), &outPropertyDesc);
				ThrowIfOSErr_(err);
			} else {
				Throw_(errAENotAnElement);
			}
			break;
		}

		case pTextStyles:
		{
			Int32	style;

			if (GetAttributeValue(kFaceAttr, &style)) {
				IntToStyles(style, &outPropertyDesc);
			} else {
				Throw_(errAENotAnElement);
			}
			break;
		}

		case pFont:
		{
			Int16	font;
			Str255	fontName;

			if (GetAttributeValue(kFontAttr, &font)) {
				GetFontName(font, fontName);
				err = AECreateDesc(typeChar, fontName + 1, fontName[0], &outPropertyDesc);
				ThrowIfOSErr_(err);
			} else {
				Throw_(errAENotAnElement);
			}
			break;
		}

		default:
			LStyleSet::GetAEProperty(inProperty, inRequestedType, outPropertyDesc);
			break;
	}
}

void	CTXStyleSet::SetAEProperty(
	DescType		inProperty,
	const AEDesc	&inValue,
	AEDesc			&outAEReply)
{
	LAESubDesc	valSD(inValue);
	
	switch (inProperty) {
		case pScriptTag:
			Throw_(errAENotModifiable);
			break;

		case pPointSize:
		{
			Int16	value = valSD.ToInt16();
			
			SetAttributeValue(kFontSizeAttr, &value);
			ChangedGeometry();
			break;
		}

		case pColor:
		{
			RGBColor	value = valSD.ToRGBColor();
			
			SetAttributeValue(kForeColorAttr, &value);
			Changed();
			break;
		}

		case pTextStyles:
		{
			Int32	value;
			LAESubDesc	valSD(inValue);
			
			switch (valSD.GetType()) {

				case typeTextStyles:
					if (valSD.GetType() == typeTextStyles) {
						LAESubDesc	faceList;
						
						GetAttributeValue(kFaceAttr, &value);
		
						faceList = valSD.KeyedItem(keyAEOffStyles);
						if (faceList.GetType() == typeAEList)
							value = value & ~StylesToInt(faceList);
						else if (faceList.GetType() != typeNull)
							Throw_(paramErr);	//	bad style format
						
						faceList = valSD.KeyedItem(keyAEOnStyles);
						if (faceList.GetType() == typeAEList)
							value = value | StylesToInt(faceList);
						else if (faceList.GetType() != typeNull)
							Throw_(paramErr);	//	bad style format
					}
					break;
					
				case typeAEList:
					value = StylesToInt(valSD);
					break;
					
				case typeEnumeration:
					value = EnumToFace(valSD.ToEnum());
					break;
					
				default:
					Throw_(paramErr);	//	bad style format
					break;
			}
					
			SetAttributeValue(kFaceAttr, &value);
			ChangedGeometry();
			break;
		}

		case pFont:
		{
			Str255	fontName;
			Int16	value;
			
			valSD.ToPString(fontName);
			
			value = GetFontId(fontName);
			SetAttributeValue(kFontAttr, &value);
			ChangedGeometry();
			break;
		}

		default:
			LStyleSet::SetAEProperty(inProperty, inValue, outAEReply);
			break;
	}
}

ScriptCode	CTXStyleSet::GetScript(void) const
{
	return FontToScript(fFont);
}

//	===========================================================================
//	CStyleSetObject behavior

CAttrObject *	CStyleSetObject::CreateNew(void) const
{
	CTXStyleSet	*rval = NULL;
	
	try
		{
		rval = new CTXStyleSet((LStyleSet *)this);
		rval->IStyleSetObject();
		rval->Reference();	//	Textension RunObjects start with a refcount of 1!
		rval->SetSuperModel( ((LModelObject *)this)->GetSuperModel() );
		}
	catch (ExceptionCode inErr)
		{
		delete rval;
		rval = NULL;
		}
	
	return rval;
}

void	CStyleSetObject::IStyleSetObject(void)
{
	IQDTextRun();
}

long CStyleSetObject::GetCountReferences(void)
{	
	Int32	rval;
	
	rval = GetUseCount();
	if (rval < 0)
		return 0;
	
	return rval;
}

void	CStyleSetObject::NoMoreUsers(void)
{
	LStyleSet::NoMoreUsers();
}

void	CStyleSetObject::AddUser(void *inClaimer)
{
	LStyleSet::AddUser(inClaimer);
}

void	CStyleSetObject::RemoveUser(void *inReleaser)
{
	LStyleSet::RemoveUser(inReleaser);
}

CAttrObject* CStyleSetObject::Reference(void)
{
	AddUser(NULL);	//	NULL because we don't know who's calling Reference
	return this;
}

void	CStyleSetObject::Free(void)
{
	RemoveUser(NULL);	//	NULL because we don't know who's calling Reference
}

void	CStyleSetObject::GetAttributes(CObjAttributes *attributes) const
{
	CQDTextRun::GetAttributes(attributes);
	
	attributes->Add(kPPStyleSet);
}

void	CStyleSetObject::SetDefaults(long script)
{
	CQDTextRun::SetDefaults(script);
}

Boolean	CStyleSetObject::EqualAttribute(AttrId attr, const void * valuePtr) const
{
	if (attr == kPPStyleSet) {
		return this == *((LStyleSet **)valuePtr);
	} else {
		return CQDTextRun::EqualAttribute(attr, valuePtr);
	}
}

void	CStyleSetObject::AttributeToBuffer(AttrId theAttr, void *attrBuffer) const
{
	if (theAttr == kPPStyleSet)
		*((CTXStyleSet **)attrBuffer) = (CTXStyleSet *)this;
	else
		CQDTextRun::AttributeToBuffer(theAttr, attrBuffer);
}

void	CStyleSetObject::BufferToAttribute(AttrId theAttr, const void *attrBuffer)
{
	if (theAttr == kPPStyleSet) {
		if (GetSuperModel() != NULL) {
			if (this != (CTXStyleSet *) (*((CTXStyleSet **)attrBuffer)))
//					Throw_(paramErr);
				theAttr++;	//	just a place to place a breakpoint
		}
	} else {
		CQDTextRun::BufferToAttribute(theAttr, attrBuffer);
	}
}

void	CStyleSetObject::SetKeyScript() const
{
	//	Just leave it as is (for now)
}
