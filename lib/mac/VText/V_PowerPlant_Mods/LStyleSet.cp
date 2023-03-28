//	===========================================================================
//	LStyleSet.cp					©1994 Metrowerks Inc. All rights reserved.
//	===========================================================================

#ifdef PowerPlant_PCH
#include PowerPlant_PCH
#endif

#include	"LStyleSet.h"

#include	<AETextSuite.h>
#include	<LTextModel.h>
#include	<UAppleEventsMgr.h>
#include	<UAEGizmos.h>
#include	<UExtractFromAEDesc.h>
#include	<LString.h>
#include	<PP_Messages.h>
#include	<UFontsStyles.h>
#include	<PP_Resources.h>

#ifndef		__FONTS__
#include	<Fonts.h>
#endif

#ifndef		__AEREGISTRY__
#include	<AERegistry.h>
#endif

#ifndef		__AEOBJECTS__
#include	<AEObjects.h>
#endif

#ifndef		__AEPACKOBJECT__
#include	<AEPackObject.h>
#endif

#ifndef		__AEREGISTRY__
#include	<AERegistry.h>
#endif

#ifndef		__GESTALT__
#include	<Gestalt.h>
#endif

#include	<LowMem.h>

#pragma	warn_unusedarg off

//	===========================================================================

StyleRunPositionT::StyleRunPositionT(
	Boolean	inLineIsLeftToRight,
	Boolean	inFirstRun,
	Boolean	inLastRun)
{
	if (inFirstRun && inLastRun) {
		mPosition = onlyStyleRun;
	} else if (!inFirstRun && !inLastRun) {
		mPosition = middleStyleRun;
	} else if (inLineIsLeftToRight) {
		if (inFirstRun)
			mPosition = leftStyleRun;
		else
			mPosition = rightStyleRun;
	} else {
		if (inFirstRun)
			mPosition = rightStyleRun;
		else
			mPosition = leftStyleRun;
	}
	
	if (inFirstRun)
		mPosition |= stylePosition_First;
	if (inFirstRun)
		mPosition |= stylePosition_Last;
}

//	===========================================================================

LStyleSet::LStyleSet()
{
	mName[0] = 0;
	SetModelKind(cStyleSet);
}

LStyleSet::LStyleSet(const LStyleSet *inOriginal)
{
	mName[0] = 0;
	SetModelKind(cStyleSet);
	ThrowIfNULL_(inOriginal);
	SetSuperModel(((LStyleSet *)inOriginal)->GetSuperModel());
}

LStyleSet::~LStyleSet()
{
}

Boolean	LStyleSet::operator==(const LStyleSet &inRhs) const
{
	if (this == &inRhs)
		return true;
	return false;
}
	
void	LStyleSet::WriteFlavorRecord(
	FlavorType	inFlavor,
	LStream		&inStream) const
{
	switch (inFlavor) {
		default:
			Throw_(paramErr);
			break;
	}
}

void	LStyleSet::ReadFlavorRecord(
	FlavorType	inFlavor,
	LStream		&inStream)
{
	switch (inFlavor) {
		default:
			Throw_(paramErr);
			break;
	}
}

//	----------------------------------------------------------------------
//	Query:

FixedT	LStyleSet::Insertion2Pixel(
	const TextRangeT	&inQueryRange,
	const Ptr			inText,
	const TextRangeT	&inRangePt,
	StyleRunPositionT	inPosition,
	FixedT				inSlop) const
{
	return FixedT::sZero;
}

void	LStyleSet::Pixel2Range(
	const TextRangeT	&inQueryRange,
	const Ptr			inText,
	FixedT				inPixelOffset,
	StyleRunPositionT	inPosition,
	FixedT				inSlop,
	TextRangeT			&outRange,
	Boolean				&outLeadingEdge
) const
{
}

Int32	LStyleSet::GetHeight(void) const
{
	return 0;
}

Int32	LStyleSet::GetAscent(void) const
{
	return 0;
}

Boolean	LStyleSet::CaretInside(void) const
/*
	Can the caret "go inside" this style set.
*/
{
	return false;
}

Boolean	LStyleSet::IsProportional(void) const
{
	return false;
}

/*-
FixedT	LStyleSet::MaxCharWidth(void) const
{
	return FixedT::sZero;
}
*/

#if	0
Int32	LStyleSet::FindCharSize(
	const Ptr	inBuffer,
	Uint16		inOffset) const
/*
	Return the size of the character at the indicated offset.
	
	inBuffer must be non relocatable.
	
	inOffset (which has a default 0 value) can only refer to the
		start of a char boundary.  Otherwise, and exception will be thrown.
*/
{
	Int32	rval = 0;
	
	if (mCharSize) {
		rval = mCharSize;
	} else {
			short type = CharacterByteType((Ptr)inBuffer, inOffset, GetScript());
			switch (type) {
			
				case smSingleByte:
					rval = 1;
					break;
					
				case smFirstByte:
					rval = 2;
					break;
					
				case smLastByte:
					Throw_(paramErr);	//	With PowerPlant text, you need not and
										//	must not use non char offsets -- were you not
										//	using LTextEngine->This/Prev/NextCharSize?
					break;
				
				case smMiddleByte:
				default:
					Throw_(paramErr);	//	What?
					break;
			}
	}
	
	return rval;
}

Int32	LStyleSet::PrevCharSize(
	const Ptr	inBuffer,
	Uint16		inOffset) const
{
	Int32	rval = 0;
	
	if (inOffset > 0) {
		if (mCharSize) {
			rval = mCharSize;
		} else {
			switch (CharacterByteType((Ptr)inBuffer, inOffset-1, GetScript())) {
			
				case smSingleByte:
					rval = 1;
					break;
					
				case smLastByte:
					rval = 2;
					break;
				
				case smFirstByte:
					Throw_(paramErr);
					break;

				case smMiddleByte:
					Throw_(paramErr);
					break;

				default:
					Throw_(paramErr);
					break;
			}
		}
	}
	
	return rval;
}

Boolean	LStyleSet::CharIsLeftToRight(const Ptr inCharPtr) const
{
	return true;
}

Boolean	LStyleSet::IsLeftToRight(void) const
{
	return true;
}
#endif

Int32	LStyleSet::FindCharSize(
	const TextRangeT	&inStyleRange,
	const Ptr			inText,
	const TextRangeT	&inInsertionPt) const
{
	Assert_(inStyleRange.Length());
	Assert_(!inInsertionPt.Length());
	Assert_(inStyleRange.Contains(inInsertionPt));
	
	if (inInsertionPt.Insertion() == rangeCode_Before)
		return inInsertionPt.Start() - inStyleRange.Start();
	else
		return inStyleRange.End() - inInsertionPt.Start();
}

TextDirectionE	LStyleSet::CharDirection(
	const TextRangeT	&inStyleRange,
	const Ptr			inText,
	const TextRangeT	&inInsertionPt) const
{
	return textDirection_Undefined;
}

TextDirectionE	LStyleSet::StyleDirection(void) const
{
	return textDirection_Undefined;
}

ScriptCode	LStyleSet::GetScript(void) const
{
	return smRoman;
}

LangCode	LStyleSet::GetLanguage(void) const
{
	return langEnglish;
}

Int32	LStyleSet::GetFontNumber(void) const
{
	return LMGetApFontID();
}

Int32	LStyleSet::GetFontSize(void) const
{
	return LMGetSysFontSize();
}

Int32	LStyleSet::GetFontFace(void) const
{
	return normal;
}

//	----------------------------------------------------------------------
//	Formatting:

void	LStyleSet::LineBreakScan(
	const TextRangeT	&inLineRange,		//	line start to paragraph end
	const TextRangeT	&inScriptRange,		//	range of adjacent/equivalent styles in inLineRange
	const TextRangeT	&inStyleRange,		//	range corresponding to this styleset -- looking
											//		to see if the line break occurs in this range.
	const Ptr			inText,				//	pointer to all of text
	FixedT				inWidth,			//	width remaining on line
	TextRangeT			&outRange,			//	range of inStyleRange that occurs on line
	FixedT				&outWidthUnused,	//	width left over after scan inStyleRange
	StyledLineBreakCode	&outBreakCode		//	what happened
) const
{
}		

Int32	LStyleSet::VisibleLength(
	const Ptr			inText,
	Int32				inLength) const
{
	return 0;
}

FixedT	LStyleSet::TextWidth(
	const Ptr			inText,
	Int32				inLength) const
{
	return FixedT::sZero;
}

FixedT	LStyleSet::Portion(
	const Ptr				inText,
	Int32					inVisibleLength,
	const StyleRunPositionT	&inPosition) const
{
	return FixedT::sZero;
}

void	LStyleSet::Draw(
	const TextRangeT	&inRange,
	const Ptr			inText,
	StyleRunPositionT	inPosition,
	FixedT				inSlop) const
{
}

//	----------------------------------------------------------------------
//	Implementation:

void	LStyleSet::Focus(void) const
{
}

/*-
Boolean	LStyleSet::FindCommandStatus(
	CommandT	inCommand,
	Boolean		&outEnabled,
	Boolean		&outUsesMark,
	Char16		&outMark,
	Str255		outName)
{
	return false;
}
*/

//	----------------------------------------------------------------------
//	AEOM:

const StringPtr	LStyleSet::GetModelNamePtr(void) const
{
	if (mName[0])
		return (StringPtr)mName;
	else
		return NULL;
}

void	LStyleSet::GetTextTraits(TextTraitsRecord *inTraits) const
{
	ThrowIfNULL_(inTraits);
	
	StAEDescriptor	size,
					styles,
					color,
					fontName;
	
	GetAEProperty(pColor, UAppleEventsMgr::sAnyType, color.mDesc);
	GetAEProperty(pFont, UAppleEventsMgr::sAnyType, fontName.mDesc);
	GetAEProperty(pPointSize, UAppleEventsMgr::sAnyType, size.mDesc);
	GetAEProperty(pTextStyles, UAppleEventsMgr::sAnyType, styles.mDesc);

	UExtractFromAEDesc::TheInt16(size, inTraits->size);

	LAESubDesc		ons(styles.mDesc);
	inTraits->style = UFontsStyles::StylesToInt(ons.KeyedItem(keyAEOnStyles));

	inTraits->justification = teFlushDefault;	//	actually missing	
	inTraits->mode = srcOr;						//	actually missing
	UExtractFromAEDesc::TheRGBColor(color, inTraits->color);
	UExtractFromAEDesc::ThePString(fontName, inTraits->fontName);
	inTraits->fontNumber = UFontsStyles::GetFontId(inTraits->fontName);
}

void	LStyleSet::SetTextTraits(const TextTraitsRecord &inTraits)
{
	StAEDescriptor	size(inTraits.size),
					styles,
					color(cRGBColor, &inTraits.color, sizeof(inTraits.color)),
					bogusReply;
	
	UFontsStyles::IntToStyles(inTraits.style, &styles.mDesc);
	
	SetAEProperty(pColor, color, bogusReply);

	try {
		if (inTraits.fontName[0] != 0) {
			StAEDescriptor	fontName((StringPtr)inTraits.fontName);
			
			SetAEProperty(pFont, fontName, bogusReply);
		} else {
			Str255	font;
			
			UFontsStyles::GetFontName(inTraits.fontNumber, font);
			
			if (font[0] == 0)
				Throw_(paramErr);
				
			StAEDescriptor	fontName(font);
			
			SetAEProperty(pFont, fontName, bogusReply);
		}
	} catch (...) {
		Str255	font;
		UFontsStyles::GetFontName(LMGetApFontID(), font);
		
		if (font[0] == 0)
			Throw_(paramErr);
			
		StAEDescriptor	fontName(font);
		
		SetAEProperty(pFont, fontName, bogusReply);
	}

	SetAEProperty(pPointSize, size, bogusReply);
	SetAEProperty(pTextStyles, styles, bogusReply);
}

void	LStyleSet::GetAEProperty(
	DescType		inProperty,
	const AEDesc	&inRequestedType,
	AEDesc			&outPropertyDesc) const
{
	switch (inProperty) {

		case pName:
		{
			LAEStream	out;
		
			out.WriteDesc(typeChar, mName + 1, mName[0]);
			out.Close(&outPropertyDesc);
			break;
		}

		default:
			inheritAEOM::GetAEProperty(inProperty, inRequestedType, outPropertyDesc);
			break;
	}
}

void	LStyleSet::SetAEProperty(
	DescType		inProperty,
	const AEDesc	&inValue,
	AEDesc			&outAEReply)
{
	switch (inProperty) {

		case pName:
		{
			Str255	str;
			
			UExtractFromAEDesc::ThePString(inValue, str);
			LString::CopyPStr(str, mName, sizeof(mName));
			Changed();
			break;
		}

		default:
			inheritAEOM::SetAEProperty(inProperty, inValue, outAEReply);
			break;
	}
}

void	LStyleSet::ModifyProperty(
	DescType			inProperty,
	const LAESubDesc	&inByValue
)
{
}

void	LStyleSet::MakeSelfSpecifier(AEDesc &inSuperSpecifier, AEDesc &outSelfSpecifier) const
{
	if (mName[0] == 0) {
		inheritAEOM::MakeSelfSpecifier(inSuperSpecifier, outSelfSpecifier);
	} else {
		OSErr			err;
		StAEDescriptor	desc(typeChar, (char *)(mName + 1), mName[0]);
	
		err = CreateObjSpecifier(cStyleSet, &inSuperSpecifier, 
			formName, &desc.mDesc, false, &outSelfSpecifier);
		ThrowIfOSErr_(err);
	}
}

#include	<LTextModel.h>

void	LStyleSet::Changed(void)
{
	LModelObject	*superModel = GetSuperModel();
	LTextModel		*textModel;
	LTextEngine		*engine;
	
	if (!superModel)
		return;
		
	textModel = (LTextModel *) superModel;
//	Assert_(member(textModel, LTextModel));
	engine = textModel->GetTextEngine();
	if (engine) {
//		Assert_(member(engine, LTextEngine));
		engine->StyleSetChanged(this);
	}
}

void	LStyleSet::ChangedGeometry(void)
{
	Changed();
}
