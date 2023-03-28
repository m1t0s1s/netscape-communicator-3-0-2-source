//	===========================================================================
//	VStyleSet
//
//	Copyright 1996 ViviStar Consulting.
//	===========================================================================

#pragma once

#include	<LStyleSet.h>
#include	<VText_ClassIDs.h>

class	VStyleSet
			:	public LStyleSet
{
public:
						VStyleSet(const LStyleSet *inOriginal);
						VStyleSet();
	virtual				~VStyleSet();	
	enum {class_ID = kVTCID_VStyleSet};
	virtual ClassIDT	GetClassID() const {return class_ID;}

#ifndef	PP_No_ObjectStreaming
	virtual void		WriteObject(LStream &inStream) const;
	virtual void		ReadObject(LStream &inStream);
#endif
	
	virtual Boolean		operator==(const LStyleSet &inRhs) const;

	virtual void		WriteFlavorRecord(
							FlavorType	inFlavor,
							LStream		&inStream) const;
	virtual void		ReadFlavorRecord(
							FlavorType	inFlavor,
							LStream		&inStream);
	
	virtual	FixedT		Insertion2Pixel(
							const TextRangeT	&inQueryRange,
							const Ptr			inText,
							const TextRangeT	&inRangePt,
							StyleRunPositionT	inPosition,
							FixedT				inSlop) const;							
	virtual void		Pixel2Range(
							const TextRangeT	&inQueryRange,
							const Ptr			inText,
							FixedT				inPixelOffset,
							StyleRunPositionT	inPosition,
							FixedT				inSlop,
							TextRangeT			&outRange,
							Boolean				&outLeadingEdge) const;
	virtual	Int32		GetHeight(void) const;
	virtual	Int32		GetAscent(void) const;
	virtual	Boolean		CaretInside(void) const;
	virtual	Boolean		IsProportional(void) const;
//-	virtual	FixedT		MaxCharWidth(void) const;
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
				
	virtual void		Draw(
							const TextRangeT	&inRange,
							const Ptr			inText,
							StyleRunPositionT	inPosition,
							FixedT				inSlop) const;

/*-
	virtual Boolean		FindCommandStatus(CommandT inCommand,
							Boolean &outEnabled, Boolean &outUsesMark,
							Char16 &outMark, Str255 outName);
*/

	virtual	void		Focus(void) const;

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

protected:	
				//	data members
	Int16				mFontNumber;
	Int16				mFontSize;
	Style				mFontFace;
	Int16				mXferMode;
	RGBColor			mFontColor;

				//	cached values
	ScriptCode			mScript;
	LangCode			mLanguage;
	TextDirectionE		mDirection;
	Int16				mStyleAscent;
	Int16				mStyleHeight;
	FixedT				mMaxCharWidth;
	Boolean				mIsProportional;
	Int32				mCharSize;					//	0 means calculate it

	virtual	void		GetImportantAEProperties(AERecord &outRecord) const;
	
protected:
	virtual void		ChangedGeometry(void);
	virtual void		Changed(void);
};
