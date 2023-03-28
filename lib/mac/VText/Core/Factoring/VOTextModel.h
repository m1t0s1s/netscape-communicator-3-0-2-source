//	===========================================================================
//	VOTextModel.h
//	===========================================================================

#pragma once

#include	<LTextModel.h>
#include	<VText_ClassIDs.h>
#include	<vector.h>

typedef enum EVTextSetBehavior {
	setBehavior_Ignore = 0,		
	/*	on read,
			set data is ignored
	*/

	setBehavior_Sheet = 0x01,
	/*	on read,
			if a flavor set name matches an existing set name,
				the existing set is used
			else
				the default set is used
	*/				

	setBehavior_FreeForm = 0x02,
	/*	on read,
			if a flavor set attributes match and existing set attributes,
				the existing set is used
			else
				a new set is created for the flavor set and it is used
	*/

	setBehavior_FreeSheet =	setBehavior_Sheet +
							setBehavior_FreeForm

	/*	on read
			if a flavor set name and attributes match an existing set name and attribute,
				the existing set is used
			else
				a new set with flavor attributes and a unique name is used
	*/
} EVTextSetBehavior;

//	===========================================================================

class	VOTextModel
			:	public	LTextModel
{
public:
//						VOTextModel(LTextEngine *inTextEngine);
						VOTextModel();
						~VOTextModel();
	enum {class_ID = kVTCID_VOTextModel};
	virtual ClassIDT	GetClassID() const {return class_ID;}

#ifndef	PP_No_ObjectStreaming
	virtual void		WriteObject(LStream &inStream) const;
	virtual void		ReadObject(LStream &inStream);
#endif
	virtual void		SetTextEngine(LTextEngine *inTextEngine);

	virtual	void		ListenToMessage(MessageT inMessage, void *ioParam);
	virtual	LStyleSet *	StyleSetForScript(
							const TextRangeT	&inRange,
							ScriptCode			inScript);
	
public:
	virtual void		AddFlavorTypesTo(
							const TextRangeT	&inRange,
							LDataTube			*inTube) const;
	virtual Boolean		SendFlavorTo(
							FlavorType			inFlavor,
							const TextRangeT	&inRange,
							LDataTube			*inTube) const;
	virtual	FlavorType	PickFlavorFrom(
							const TextRangeT	&inRange,
							const LDataTube		*inTube) const;
	virtual void		ReceiveDataFrom(
							const TextRangeT	&inRange,
							LDataTube			*inTube);

	virtual void		Modify(
							const TextRangeT	&inRange,
							DescType			inProperty,
							DescType			inOperator,
							const LAESubDesc	&inValue);			

	virtual	LTextElemAEOM *	MakeNewElem(
								const TextRangeT	&inRange,
								DescType			inKind,
								const LModelObject	*inSuperModel = NULL) const;
protected:
	virtual	LTextElemAEOM *
						MakeNewElemSelf(
							const TextRangeT	&inRange,
							DescType			inKind,
							const LModelObject	*inSuperModel) const;

	virtual	void		ApplyStylesFrom(
							FlavorType 			inFlavor,
							const TextRangeT	&inRange,
							LDataTube			*inTube);
	virtual	void		ApplyRulersFrom(
							FlavorType 			inFlavor,
							const TextRangeT	&inRange,
							LDataTube			*inTube);

public:
	virtual EVTextSetBehavior
						GetStyleListBehavior(void) const;
	virtual void		SetStyleListBehavior(EVTextSetBehavior inBehavior);
	virtual	void		AddStyle(const LStyleSet *inStyle);
	virtual void		RemoveStyle(const LStyleSet *inStyle);
	virtual void		StyleChanged(const LStyleSet *inStyle);
	virtual void		PurgeStyles(void);
/*
	virtual const LStyleSet*
						FindEquivalentStyle(const LStyleSet *inStyle) const;
*/

	virtual EVTextSetBehavior
						GetRulerListBehavior(void) const;
	virtual void		SetRulerListBehavior(EVTextSetBehavior inBehavior);
	virtual	void		AddRuler(const LRulerSet *inRuler);
	virtual void		RemoveRuler(const LRulerSet *inRuler);
	virtual void		RulerChanged(const LRulerSet *inRuler);
	virtual void		PurgeRulers(void);
/*-
	virtual const LRulerSet*
						FindEquivalentRuler(const LRulerSet *inRuler) const;
*/
				//	AEOM support
	virtual LModelObject*
						HandleCreateElementEvent(
									DescType			inElemClass,
									DescType			inInsertPosition,
									LModelObject*		inTargetObject,
									const AppleEvent	&inAppleEvent,
									AppleEvent			&outAEReply);
	
protected:
	vector<LStyleSet*>	mStyleList;
	vector<LRulerSet*>	mRulerList;
	EVTextSetBehavior	mStyleListBehavior;
	EVTextSetBehavior	mRulerListBehavior;
};

//	----------------------------------------------------------------------
//	stl style helper function objects:

template	<class inOpT, class inPreOpT, class inRType, class inValueT>
class	pre_binder2nd 
{
public:
	pre_binder2nd(
		const inOpT		&inOp,
		const inPreOpT	&inPreOp,
		const inValueT	&inValue)							: mPreOp(inPreOp)
															, mOp(inOp)
															, mValue(inValue)
															{}
															
	bool	operator()(const inRType &inR) const			{
																return mOp(mPreOp(inR), mValue);
															}

protected:
	inOpT			mOp;
	inPreOpT		mPreOp;
	const inValueT	&mValue;
};

template <class T>
class derefP
{
public:
	const T&	operator()(const T* inP) const				{
																ThrowIfNULL_(inP);
																return *inP;
															}
};

template <class T>
class modelNameP {
public:
	const StringPtr	operator()(const T &inObject) const		{
																return inObject.GetModelNamePtr();
															}
};

template <class T>
class setEquivF 
		:	public pre_binder2nd<equal_to<T>, derefP<T>, T*, T>
{
public:
	setEquivF(const T &inKey)						
															: pre_binder2nd
																<equal_to<T>, derefP<T>, T*, T>
																(equal_to<T>(), derefP<T>(), inKey)
															{}
};

template <class T>
class nameEquivF 
{
public:
	nameEquivF(const T &inKey)								: mKeyName(inKey.GetModelNamePtr())
															{}
	
	bool		operator()(const T* &inObject) const		{
																return EqualString(
																			inObject->GetModelNamePtr(),
																			mKeyName,
																			true,
																			true
																);
															}

protected:
	const StringPtr		mKeyName;
};

template <class T>
class	replaceSharableF
{
public:
	replaceSharableF(
		T		inValue,
		void	*inUser)									: mValue(inValue)
															, mUser(inUser)
															{}
															
	void		operator()(T &ioRef) const					{
																ReplaceSharable_(ioRef, mValue, mUser);
															}

protected:
	void	*mUser;
	T		mValue;
};