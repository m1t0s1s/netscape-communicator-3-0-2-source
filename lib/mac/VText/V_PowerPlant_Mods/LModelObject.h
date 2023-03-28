// ===========================================================================
//	LModelObject.h			   ©1993-1996 Metrowerks Inc. All rights reserved.
// ===========================================================================

#ifndef _H_LModelObject
#define _H_LModelObject
#pragma once

#include <LList.h>

#ifndef __APPLEEVENTS__
#include <AppleEvents.h>
#endif


class	LModelObject {
public:
							LModelObject();
							LModelObject(
									LModelObject	*inSuperModel,
									DescType		inKind = typeNull);
	virtual					~LModelObject();
	virtual void			SuperDeleted(void);
												
	LModelObject*			GetSuperModel() const	{ return mSuperModel; }
							
	virtual void			SetSuperModel(
									LModelObject	*inSuperModel);
									
	DescType				GetModelKind() const	{ return mModelKind; }
	void					SetModelKind(
									DescType		inModelKind);
	virtual Boolean			IsLazy() const;
	virtual void			SetLaziness(
									Boolean			inLaziness);
	virtual	void			SetUseSubModelList(
									Boolean			inUseSubModelList);
	
	virtual	void			AddSubModel(
									LModelObject	*inSubModel);
	virtual	void			RemoveSubModel(
									LModelObject	*inSubModel);

	virtual	Boolean			IsSubModelOf(
									const LModelObject	*inSuperModel) const;

	virtual	LModelObject*	GetDefaultSubModel() const;
	virtual void			SetDefaultSubModel(
									LModelObject	*inSubModel);
	virtual Boolean			IsDefaultSubModel() const;

	virtual void			GetModelToken(
									DescType		inModelID,
									DescType		inKeyForm,
									const AEDesc	&inKeyData,
									AEDesc			&outToken) const;
									
	virtual void			GetModelTokenSelf(
									DescType		inModelID,
									DescType		inKeyForm,
									const AEDesc	&inKeyData,
									AEDesc			&outToken) const;
									
	virtual Int32			CountSubModels(DescType inModelID) const;
	
	virtual void			GetSubModelByPosition(
									DescType		inModelID,
									Int32			inPosition,
									AEDesc			&outToken) const;
									
	virtual void			GetSubModelByName(
									DescType		inModelID,
									Str255			inName,
									AEDesc			&outToken) const;
									
	virtual void			GetSubModelByUniqueID(
									DescType		inModelID,
									const AEDesc	&inKeyData,
									AEDesc			&outToken) const;
									
	virtual void			GetModelByRelativePosition(
									DescType		inModelID,
									OSType			inRelativePosition,
									AEDesc			&outToken) const;
									
	virtual void			GetSubModelByComplexKey(
									DescType		inModelID,
									DescType		inKeyForm,
									const AEDesc	&inKeyData,
									AEDesc			&outToken) const;
									
	virtual void			GetAllSubModels(
									DescType		inModelID,
									AEDesc			&outToken) const;
									
	virtual Int32			GetPositionOfSubModel(
									DescType			inModelID,
									const LModelObject	*inSubModel) const;
									
	virtual LModelObject*	GetInsertionTarget(
									DescType		inInsertPosition) const;
	virtual LModelObject*	GetInsertionContainer(
									DescType		inInsertPosition) const;
	virtual LModelObject*	GetInsertionElement(
									DescType		inInsertPosition) const;
	
	virtual const StringPtr	GetModelNamePtr() const;
	
	virtual Boolean			CompareToModel(
									DescType		inComparisonOperator,
									LModelObject	*inCompareModel) const;
									
	virtual Boolean			CompareToDescriptor(
									DescType		inComparisonOperator,
									const AEDesc	&inCompareDesc) const;
									
	virtual Boolean			CompareToUniqueID(
									DescType		inComparisonOperator,
									const AEDesc	&inCompareDesc) const;
									
	virtual Boolean			CompareProperty(
									DescType		inPropertyID,
									DescType		inComparisonOperator,
									const AEDesc	&inCompareObjectOrDesc)
										const;
										
	virtual LModelObject*	GetModelPropertyAll(
									DescType		inProperty) const;

	virtual void			MakeSpecifier(AEDesc &outSpecifier) const;
	
	virtual void			HandleAppleEventAll(
									const AppleEvent	&inAppleEvent,
									AppleEvent			&outAEReply,
									AEDesc				&outResult,
									Int32				inAENumber);
	
	virtual LModelObject*	HandleCreateElementEventAll(
									DescType			inElemClass,
									DescType			inInsertPosition,
									LModelObject*		inTargetObject,
									const AppleEvent	&inAppleEvent,
									AppleEvent			&outAEReply);									

	virtual void			HandleAppleEvent(
									const AppleEvent	&inAppleEvent,
									AppleEvent			&outAEReply,
									AEDesc				&outResult,
									Int32				inAENumber);
	
	virtual LModelObject*	HandleCreateElementEvent(
									DescType			inElemClass,
									DescType			inInsertPosition,
									LModelObject*		inTargetObject,
									const AppleEvent	&inAppleEvent,
									AppleEvent			&outAEReply);									

	virtual void			HandleCount(
									DescType			inModelID,
									AppleEvent			&outResult);
	virtual void			HandleDelete(
									AppleEvent			&outAEReply,
									AEDesc				&outResult);
	virtual	void			HandleClone(
									const AppleEvent	&inAppleEvent,
									AppleEvent			&outAEReply,
									AEDesc				&outResult);
	virtual	void			HandleMove(
									const AppleEvent	&inAppleEvent,
									AppleEvent			&outAEReply,
									AEDesc				&outResult);

	virtual void			Finalize();
	
	virtual void			GetAEProperty(
									DescType		inProperty,
									const AEDesc	&inRequestedType,
									AEDesc			&outPropertyDesc) const;
	virtual void			SetAEProperty(
									DescType		inProperty,
									const AEDesc	&inValue,
									AEDesc			&outAEReply);
	virtual	void			GetImportantAEProperties(AERecord &outRecord) const;
										  
	virtual void			SendSelfAE(
									AEEventClass	inEventClass,
									AEEventID		inEventID,
									Boolean			inExecute);
									
	static	LModelObject*	GetDefaultModel();
	static	void			SetDefaultModel(
									LModelObject	*inModel);
	static	void			PutInToken(
									LModelObject	*inModel,
									AEDesc			&outToken);
							
	static	LModelObject*	GetModelFromToken(
									const AEDesc	&inToken);
	static 	void			FinalizeLazies();
											
	static	LModelObject*	GetStreamingModel();
	static	void			SetStreamingModel(
									LModelObject	*inModel);

	static 	LModelObject*	GetTellTarget();
	static	void			SetTellTarget(
									LModelObject	*inModel);
	static	void			DoAESwitchTellTarget(
									LModelObject	*inModel);
									
	static void				InitLazyList()
										{ sLazyModels = new LList; }

	static void				DestroyLazyList()
										{ delete sLazyModels; }

protected:
	static	LModelObject*	sDefaultModel;
	static	LModelObject*	sStreamingModel;
	static	LList*			sLazyModels;

	LModelObject*			mSuperModel;
	LList*					mSubModels;
	LModelObject*			mDefaultSubModel;
	LModelObject*			mDefaultSuperModel;
	DescType				mModelKind;
	Boolean					mLaziness;
	
	static	void			AddLazy(
									LModelObject	*inModel);
	static	void			RemoveLazy(
									LModelObject	*inModel);

	virtual void			MakeSelfSpecifier(
									AEDesc			&inSuperSpecifier,
									AEDesc			&outSelfSpecifier) const;

	virtual	LModelObject*	GetDefaultSuperModel() const;
	virtual void			SetDefaultSuperModel(
									LModelObject	*inSubModel);
	virtual LModelObject*	GetModelProperty(
									DescType		inProperty) const;
};



typedef struct	SModelToken {
	LModelObject	*modelObject;
} SModelToken, *SModelTokenP, **SModelTokenH;


const	Int32		position_AllSubModels	= -1;
const	DescType	type_ModelToken			= 'Toke';

//	===========================================================================
//	Helper Classes.

class	StLazyLock {
	public:
				StLazyLock(LModelObject *inModel);
				~StLazyLock();
				
		LModelObject	*mModel;
		Boolean			mLaziness;
};

class	StTempTellTarget {
	public:
				StTempTellTarget(LModelObject *inModel);
				~StTempTellTarget();
				
		LModelObject	*mOldTarget;
		Boolean			mOldTargetLaziness;
};

#endif