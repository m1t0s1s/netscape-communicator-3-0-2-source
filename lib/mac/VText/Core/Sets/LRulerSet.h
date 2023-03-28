//	===========================================================================
//	LRulerSet.h	
//
//	Copyright 1996 ViviStar Consulting.
//	===========================================================================

#pragma once

#include	<LSharableModel.h>
#include	<LStreamable.h>
#include	<VTextEngine.h>
#include	<LDataTube.h>	//	just for FlavorType definition
#include	<AETextSuite.h>
#include	<WFixed.h>
#include	<vector.h>
#include	<VTabAEOM.h>

class	LAEStream;
class	LAESubDesc;
class	VLine;
class	VTextEngine;

typedef Int32 VTextJustificationE;	//	See AETextSuite.h

//	----------------------------------------------------------------------

class	StSystemDirection
{
public:
		StSystemDirection(Boolean inLeftToRight);
		~StSystemDirection();
protected:
		Boolean	mWasLeftToRight;
};

//	----------------------------------------------------------------------

class	LRulerSet
			:	public LSharableModel
			,	public LStreamable
{
private:
	typedef	LSharableModel	inheritAEOM;
	
public:
						LRulerSet(const LRulerSet *inOriginal);
						LRulerSet();
	virtual				~LRulerSet();
	enum {class_ID = kVTCID_VRulerSet};
	virtual ClassIDT	GetClassID() const {return class_ID;}

#ifndef	PP_No_ObjectStreaming
	virtual void		WriteObject(LStream &inStream) const;
	virtual void		ReadObject(LStream &inStream);
#endif
	
	virtual Boolean		operator==(const LRulerSet &inRhs) const;

	virtual void		WriteFlavorRecord(
							FlavorType	inFlavor,
							LStream		&inStream) const;
	virtual void		ReadFlavorRecord(
							FlavorType	inFlavor,
							LStream		&inStream);
	
	virtual void		Changed(void);
	
				//	Query
	virtual LRulerSet *	GetParent(void) const				{return mParent;}
	virtual	void		SetParent(const LRulerSet *inRuler);
	virtual	Boolean		IsLeftToRight(void) const;
//	virtual VTextJustificationE
//						GetJustification(void) const;
	virtual const Range32T&
						GetParentPixels(void) const;
	virtual const Range32T&
						GetPixels(void) const;

	virtual void		NextTabStop(
							FixedT					inIndent,
							FixedT					&outStop,
							VTextTabOrientationE	&outType,
							Int16					&outAligner) const;
	virtual Int32		NewTabIndex(const TabRecord &inTabRecord) const;
	virtual void		DeleteTab(TabRecord *inTabRecord);
	
				//	Drawing/Layout
	virtual	void		DrawLine(
							const VTextEngine	&inTextEngine,
							const Ptr			inText,
							const Int32			inLineIndex) const;
							
	virtual void		LayoutLine(
							const VTextEngine	&inTextEngine,
							const Ptr			inText,
							const TextRangeT	&inParaRange,
							const TextRangeT	&inLineStartPt,
							VLine				&ioLine) const;
							
				//	AEOM
	virtual void		SetSuperModel(LModelObject *inSuperModel);
	virtual LModelObject*	
						HandleCreateElementEvent(
								DescType			inElemClass,
								DescType			inInsertPosition,
								LModelObject*		inTargetObject,
								const AppleEvent	&inAppleEvent,
								AppleEvent			&outAEReply);									

	virtual Int32		CountSubModels(DescType inModelID) const;
	virtual void		GetSubModelByPosition(
								DescType		inModelID,
								Int32			inPosition,
								AEDesc			&outToken) const;
	virtual Int32		GetPositionOfSubModel(
								DescType			inModelID,
								const LModelObject	*inSubModel) const;
									
	virtual void		GetAEProperty(
								DescType		inProperty,
								const AEDesc	&inRequestedType,
								AEDesc			&outPropertyDesc) const;
	virtual void		SetAEProperty(
								DescType		inProperty,
								const AEDesc	&inValue,
								AEDesc			&outAEReply);

						//	Implementation support...
	virtual const StringPtr	GetModelNamePtr(void) const;

protected:
	vector<TabRecord>	mTabs;
									
	LRulerSet *			mParent;
	FixedT				mBegin,
						mBeginFirst,
						mEnd,
						mBeginDelta,
						mBeginFirstDelta,
						mEndDelta;
	VTextJustificationE	mJustification;
	
	Boolean				mIsLeftToRight;

	Range32T			mPixels,		//	width of line
						mParentPixels;
		
	Str31				mName;

	virtual vector<TabRecord>::const_iterator
						FindTab(FixedT inIndent) const;
	virtual	Int32		FindTabRepetition(
								const TabRecord	&inTab,
								FixedT			inIndent) const;

	virtual void		MakeSelfSpecifier(
								AEDesc		&inSuperSpecifier,
								AEDesc		&outSelfSpecifier) const;
};
