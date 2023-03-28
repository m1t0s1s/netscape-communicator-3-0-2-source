//	===========================================================================
//	LStyleSet.h					©1994 Metrowerks Inc. All rights reserved.
//	===========================================================================

#pragma once

#include	<LSharableModel.h>
#include	<LStreamable.h>
#include	<UTextTraits.h>
#include	<LTextEngine.h>
#include	<WFixed.h>

class	LAEStream;
class	LAESubDesc;

//	===========================================================================

typedef enum {
	textDirection_Undefined,
	textDirection_LeftToRight,
	textDirection_RightToLeft
}	TextDirectionE;

typedef struct StyleRunPositionT {
protected:
	enum {
		stylePosition_First	= 0x10,
		stylePosition_Last = 0x20
	};
	
	Int16	mPosition;
public:
			StyleRunPositionT()						{}
			StyleRunPositionT(
				Boolean inLeftToRight,
				Boolean inFirstRun,
				Boolean	inLastRun);
			~StyleRunPositionT()					{}

	inline	operator	JustStyleCode()	const		{return mPosition & 0x03;}
	inline	Boolean		IsLast(void) const			{return (mPosition & stylePosition_Last) != 0;}
} StyleRunPositionT;

//	===========================================================================

class	LStyleSet
			:	public LSharableModel
			,	public LStreamable
{
private:
	typedef	LSharableModel	inheritAEOM;
	
public:
						LStyleSet(const LStyleSet *inOriginal);
						LStyleSet();
	virtual				~LStyleSet();
	
	virtual Boolean		operator==(const LStyleSet &inRhs) const;

	virtual void		WriteFlavorRecord(
							FlavorType	inFlavor,
							LStream		&inStream) const;
	virtual void		ReadFlavorRecord(
							FlavorType	inFlavor,
							LStream		&inStream);

	virtual void		SetTextTraits(const TextTraitsRecord &inTraits);
	virtual void		GetTextTraits(TextTraitsRecord *outTraits) const;

				//	Query
	virtual	FixedT		Insertion2Pixel(
							const TextRangeT	&inQueryRange,
							const Ptr			inText,
							const TextRangeT	&inRangePt,
							StyleRunPositionT	inPosition,
							FixedT				inSlop) const = 0;							
	virtual void		Pixel2Range(
							const TextRangeT	&inQueryRange,
							const Ptr			inText,
							FixedT				inPixelOffset,
							StyleRunPositionT	inPosition,
							FixedT				inSlop,
							TextRangeT			&outRange,
							Boolean				&outLeadingEdge) const = 0;
	virtual	Int32		GetHeight(void) const = 0;
	virtual	Int32		GetAscent(void) const = 0;
	virtual	Boolean		CaretInside(void) const;
	virtual	Boolean		IsProportional(void) const;
//-	virtual	FixedT		MaxCharWidth(void) const;
/*-	virtual	Int32		FindCharSize(
							const Ptr	inBuffer,
							Uint16		inOffset = 0) const;
	virtual	Int32		PrevCharSize(
							const Ptr	inBuffer,
							Uint16		inOffset) const;
	virtual Boolean		CharIsLeftToRight(const Ptr inCharPtr) const;
	virtual Boolean		IsLeftToRight(void) const;	
*/
	virtual	Int32		FindCharSize(
							const TextRangeT	&inStyleRange,
							const Ptr			inText,
							const TextRangeT	&inInsertionPt) const;
	virtual	TextDirectionE
						CharDirection(
							const TextRangeT	&inStyleRange,
							const Ptr			inText,
							const TextRangeT	&inInsertionPt) const;
	virtual TextDirectionE
						StyleDirection(void) const;	
							
	
		virtual ScriptCode	GetScript(void) const;
	virtual	LangCode	GetLanguage(void) const;
						//	avoid using these...
	virtual	Int32		GetFontNumber(void) const;
	virtual Int32		GetFontSize(void) const;
	virtual Int32		GetFontFace(void) const;
	
				//	Formatting
	virtual void		LineBreakScan(
							const TextRangeT	&inLineRange,
							const TextRangeT	&inScriptRange,
							const TextRangeT	&inStyleRange,
							const Ptr			inText,
							FixedT				inWidth,
							TextRangeT			&outRange,
							FixedT				&outWidthUnused,
							StyledLineBreakCode	&outBreakCode) const;
	virtual Int32		VisibleLength(
							const Ptr			inText,
							Int32				inLength) const;
	virtual FixedT		TextWidth(
							const Ptr			inText,
							Int32				inLength) const;
	virtual	FixedT		Portion(
							const Ptr				inText,
							Int32					inVisibleLength,
							const StyleRunPositionT	&inPosition) const;
				
				//	Drawing
	virtual void		Draw(
							const TextRangeT	&inRange,
							const Ptr			inText,
							StyleRunPositionT	inPosition,
							FixedT				inSlop) const;

				//	Implementation
/*-
	virtual Boolean		FindCommandStatus(CommandT inCommand,
							Boolean &outEnabled, Boolean &outUsesMark,
							Char16 &outMark, Str255 outName);
*/

	virtual	void		Focus(void) const;	//	Necessary?

				//	AEOM
	virtual void		GetAEProperty(
								DescType		inProperty,
								const AEDesc	&inRequestedType,
								AEDesc			&outPropertyDesc) const;
	virtual void		SetAEProperty(
								DescType		inProperty,
								const AEDesc	&inValue,
								AEDesc			&outAEReply);

	virtual void		ModifyProperty(
								DescType			inProperty,
								const LAESubDesc	&inValueDelta);
/*
	Ease of use functions?
	
			void		GetFontByName(Str255 outFontName);
			void		SetFontByName(const Str255 inFontName);
			void		GetFontSize(Int16 *outFontSize);
			void		SetFontSize(Int16 inFontSize);
	
	NOT!
	
	These are not included because they shouldn't be used and just clutter
	the API.
	
	If you're setting up a default style for your application, read it from a
	resource using UTextTraits and then set it using style->SetTextTraits().
	
	If you're implementing some sort of menu or toolbar style support, you'll
	need to codify the operation in AppleEvents for the sake of recordability
	and or undoability  (ie, you're going to be calling Get/SetAEProperty
	anyway).
	
	And, if you're going to be applying a new style across a range of styles,
	you get to write an iterator that probably uses the copy feature
	of the style constructor.  Do it and if it is flexible, send it to us :)
*/

						//	Implementation support...
	virtual const StringPtr	GetModelNamePtr(void) const;

protected:
	virtual void		ChangedGeometry(void);
	virtual void		Changed(void);
	
	Str31				mName;
	virtual void		MakeSelfSpecifier(
								AEDesc		&inSuperSpecifier,
								AEDesc		&outSelfSpecifier) const;
};
