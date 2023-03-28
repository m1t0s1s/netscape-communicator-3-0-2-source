// ===========================================================================
//	LModelObject.cp			   й1993-1996 Metrowerks Inc. All rights reserved.
// ===========================================================================
//
//	Mix-in class for supporting the Apple Event Object Model. Subclasses of
//	LModelObject represent Apple Event Object defined by the Apple Event
//	Registry.
//
//	=== Functions to Override ===
//
//	To Respond to Events:
//		HandleAppleEvent
//
//	To Support Elements:
//		CountSubModels
//		GetSubModelByPosition
//		GetSubModelByName
//		GetPositionOfSubModel
//		HandleCreateElementEvent
//
//	To Support Properties:
//		GetAEProperty
//		SetAEProperty

#ifdef PowerPlant_PCH
#include PowerPlant_PCH
#endif

#include <LModelObject.h>
#include <LModelDirector.h>
#include <LModelProperty.h>
#include <UAppleEventsMgr.h>
#include <UExtractFromAEDesc.h>
#include <LList.h>
#include <LListIterator.h>
#include <UMemoryMgr.h>
#include <AEPowerPlantSuite.h>

//	#define	UAEGIZMOS
#ifdef	UAEGIZMOS
	#include	<UAEGizmos.h>
#endif

#ifndef __AEOBJECTS__
#include <AEObjects.h>
#endif

#ifndef __AERegistry__
#include <AERegistry.h>
#endif

#ifndef __AEPACKOBJECT__
#include <AEPackObject.h>
#endif


//	е Class Variables
LModelObject*	LModelObject::sDefaultModel = nil;
LModelObject*	LModelObject::sStreamingModel = nil;
LList*			LModelObject::sLazyModels;

// ---------------------------------------------------------------------------
//		е LModelObject
// ---------------------------------------------------------------------------
//	Default Constructor

LModelObject::LModelObject()
{
	mSuperModel = nil;

	mModelKind = typeNull;
	mLaziness = false;
	mSubModels = nil;
	mDefaultSubModel = nil;
	mDefaultSuperModel = nil;
	
	SetStreamingModel(this);
}


// ---------------------------------------------------------------------------
//		е LModelObject(LModelObject*, DescType)
// ---------------------------------------------------------------------------
//	Construct a ModelObject with a specified SuperModel
//
//	Use the optional inKind parameter to indicate the AppleEvent class for
//	this object.

LModelObject::LModelObject(
	LModelObject	*inSuperModel,
	DescType		inKind)
{
	mSuperModel = inSuperModel;
	
	mModelKind = inKind;
	mLaziness = false;
	mSubModels = NULL;
	mDefaultSubModel = NULL;
	mDefaultSuperModel = NULL;

	if (mSuperModel != nil) {
		mSuperModel->AddSubModel(this);
	}
	
	SetStreamingModel(this);
}


// ---------------------------------------------------------------------------
//		е ~LModelObject
// ---------------------------------------------------------------------------
//	Destructor

LModelObject::~LModelObject()
{
	if (IsLazy()) {
		LModelObject	*thisMO = this;
		sLazyModels->Remove(&thisMO);
	}

	if (GetStreamingModel() == this) {
		SetStreamingModel(nil);
	}
	
	//	Make sure pointers to this object aren't left dangling.
	SetDefaultSubModel(NULL);	
	if (GetDefaultSuperModel()) {
		GetDefaultSuperModel()->SetDefaultSubModel(NULL);
	}

	//	Remove/delete submodels
	if (mSubModels) {
		LListIterator	iterator(*mSubModels, iterate_FromEnd);
		LModelObject	*sub;
		
		while (iterator.Previous(&sub)) {
			sub->SetSuperModel(nil);
			sub->SuperDeleted();
		}
		
		delete mSubModels;
	}
		
	if (mSuperModel != nil) {
		mSuperModel->RemoveSubModel(this);
		mSuperModel->Finalize();	//	to delete up chains of lazy objects
	}
}


// ---------------------------------------------------------------------------
//		е Finalize
// ---------------------------------------------------------------------------
//	This function gets called when a model object should consider deleting
//	itself.

void
LModelObject::Finalize()
{
		// Delete lazy ModelObjects that have no SubModels

	if ( IsLazy() &&
		 ((mSubModels == nil) || (mSubModels->GetCount() == 0)) ) {
		 
		delete this;
	}
}


// ---------------------------------------------------------------------------
//		е SuperDeleted
// ---------------------------------------------------------------------------
//	The super model of this object was just deleted and this object should
//	consider deleting itself.

void
LModelObject::SuperDeleted(void)
{
	if (!IsLazy()) {
		delete this;
	}
}


// ---------------------------------------------------------------------------
//		е SetSuperModel
// ---------------------------------------------------------------------------
//	Switch the SuperModel of a ModelObject.
//
//	You will rarely need to call this method -- constructors and the
//	destructor typically take care of such bookkeeping details.

void
LModelObject::SetSuperModel(
	LModelObject	*inSuperModel)
{
	//	Only switch if it will change something.
	if (inSuperModel == mSuperModel)
		return;

	if (mSuperModel != nil) {			// Detach from old SuperModel
		mSuperModel->RemoveSubModel(this);
	}

	mSuperModel = inSuperModel;

	if (mSuperModel != nil) {			// Attach to new SuperModel
		mSuperModel->AddSubModel(this);
	}
}


// ---------------------------------------------------------------------------
//		е SetModelKind
// ---------------------------------------------------------------------------
//	Set the AppleEvent Class ID of this ModelObject
//
//	You may specify the ModelKind in a constructor or with this method
//	shortly after object construction.
//
//	If the ModelKind is not specified during or after construction, it
//	defaults to "typeNull."
//
//	Being able to specify the model kind means a given subclass of
//	LModelObject can be used to implement more than one type of Apple Event
//	Object Model "class."

void
LModelObject::SetModelKind(DescType inModelKind)
{
	mModelKind = inModelKind;

	//	Is there an old GetModelKind() override interfering?
	Assert_(GetModelKind() == mModelKind);
}


// ---------------------------------------------------------------------------
//		е IsLazy
// ---------------------------------------------------------------------------
//	Return whether this ModelObject is "lazy."
//
//	A "lazy instantiated" model object is a transient object created solely
//	for the purpose of handling an AppleEvent.  After AppleEvent processing,
//	lazy objects are automatically deleted.
//
//	Default initialization sets laziness to false.  Laziness may be changed
//	with SetLaziness.

Boolean
LModelObject::IsLazy() const
{
	return mLaziness;
}


// ---------------------------------------------------------------------------
//		е SetLaziness
// ---------------------------------------------------------------------------
//	Changes whether this ModelObject is "Lazy."
//
//	Default initialization sets laziness to false.

void
LModelObject::SetLaziness(Boolean inBeLazy)
{
	if (mLaziness != inBeLazy) {
		
		if (inBeLazy) {
			AddLazy(this);
		} else {
			RemoveLazy(this);
		}
		
		mLaziness = inBeLazy;
	}
}

// ---------------------------------------------------------------------------
//		е SetUseSubModelList
// ---------------------------------------------------------------------------
//	Changes whether a "submodel list" is kept for this ModelObject.
//
//	A submodel list allows submodels to be added to an LModelObject without
//	having to override many of the "GetSubModelBy..." functions.  This is
//	useful in cases where a model object, such as a window, has several AEOM
//	addressable submodels.
//
//	Default initialization creates ModelObjects that don't keep a submodel
//	list
//
//	Note:	It doesn't makes sense to turn this feature on then off

void
LModelObject::SetUseSubModelList(Boolean inUseSubModelList)
{
	if (inUseSubModelList == (mSubModels != NULL) )
		return;
		
	if (inUseSubModelList) {
		mSubModels = new LList();
		return;
	}
	
	//	turn off
	Assert_(false);	//	but it really doesn't make sense to.
	delete mSubModels;
	mSubModels = nil;
}


// ---------------------------------------------------------------------------
//		е AddSubModel
// ---------------------------------------------------------------------------
//	Notify a ModelObject that is has a new SubModel
//
//	If the submodel list is being used, this default method will add the
//	submodel to the submodel list.
//
//	You will not need to call this method.  Instead use:
//
//		inSubModel->SetSuperModel(itsSuperModel);

void
LModelObject::AddSubModel(
	LModelObject	*inSubModel)
{
	if (mSubModels) {
		Int32	index = mSubModels->FetchIndexOf(&inSubModel);
		
		if (index) {
			Assert_(false);	//	Adding a pre-existent submodel!
			return;
		}
		
		mSubModels->InsertItemsAt(1, arrayIndex_Last, &inSubModel);
	}
}


// ---------------------------------------------------------------------------
//		е RemoveSubModel
// ---------------------------------------------------------------------------
//	Notify a ModelObject that a SubModel is being deleted
//
//	If the submodel list is being used, this method will remove the submodel
//	from the submodel list.
//
//	You will not need to call this method.  Instead use:
//
//		inSubModel->SetSuperModel(NULL);

void
LModelObject::RemoveSubModel(
	LModelObject	*inSubModel)
{
	if (mSubModels) {
		Int32	index = mSubModels->FetchIndexOf(&inSubModel);

		if (index == 0) {
			Assert_(false);	//	trying to remove non-existent submodel
			return;
		}
		
		mSubModels->RemoveItemsAt(1, index);
	}
}


// ---------------------------------------------------------------------------
//		е IsSubModelOf
// ---------------------------------------------------------------------------
//	Returns whether this is a submodel of inSuperModel

Boolean
LModelObject::IsSubModelOf(const LModelObject *inSuperModel) const
{
	if (mSuperModel == inSuperModel)
		return true;
	
	if (mSuperModel == nil)
		return false;
	
	return mSuperModel->IsSubModelOf(inSuperModel);
}


// ---------------------------------------------------------------------------
//		е GetDefaultSubModel
// ---------------------------------------------------------------------------
//	Returns the default submodel (if any) of this ModelObject.
//
//	The default submodel allows this ModelObject to serve as an alias to
//	the submodel.  Its use simplifies scripts.

LModelObject *
LModelObject::GetDefaultSubModel(void) const
{
	return mDefaultSubModel;
}


// ---------------------------------------------------------------------------
//		е SetDefaultSubModel
// ---------------------------------------------------------------------------
//	Sets the default submodel of this ModelObject.
//
//	The default sumodel must already be a submodel of this ModelObject.

void
LModelObject::SetDefaultSubModel(LModelObject *inSubModel)
{
	if (mDefaultSubModel != inSubModel) {
		//	Do the switch
		if (mDefaultSubModel)
			mDefaultSubModel->SetDefaultSuperModel(NULL);
		mDefaultSubModel = inSubModel;
		if (mDefaultSubModel)
			mDefaultSubModel->SetDefaultSuperModel(this);
	}
}


// ---------------------------------------------------------------------------
//		е IsDefaultSubModel
// ---------------------------------------------------------------------------
//	Is "this" ModelObject a default submodel of the
//	DefaultModel (Application).

Boolean
LModelObject::IsDefaultSubModel(void) const
{
	if (this == GetDefaultModel())
		return true;
		
	if (GetDefaultSuperModel()) {
		return GetDefaultSuperModel()->IsDefaultSubModel();
	} else {
		return false;
	}
}


// ---------------------------------------------------------------------------
//		е GetDefaultSuperModel
// ---------------------------------------------------------------------------
//	Returns the default supermodel of this ModelObject.
//
//	You should not need to explicitly call this method.
//	You should not need to override this method.

LModelObject *
LModelObject::GetDefaultSuperModel(void) const
{
	return mDefaultSuperModel;
}


// ---------------------------------------------------------------------------
//		е SetDefaultSuperModel
// ---------------------------------------------------------------------------
//	Sets the default supermodel of this ModelObject.
//
//	You should not need to explicitly call this method -- use
//	SetDefaultSubModel instead and it will call this method.
//	You should not need to override this method.

void
LModelObject::SetDefaultSuperModel(LModelObject *inSuperModel)
{
	mDefaultSuperModel = inSuperModel;
}


// ---------------------------------------------------------------------------
//		е CountSubModels
// ---------------------------------------------------------------------------
//	Return number of SubModels of the specified type
//
//	Must be overridden by subclasses which have SubModels that aren't
//	implemented using the submodel list (ie lazy instantiated submodels).
//
//	When overriding, you should add the inherited result to your overridden
//	result.

Int32
LModelObject::CountSubModels(
	DescType	inModelID) const
{
	Int32	count = 0;
	
	if (GetDefaultSubModel())
		count += GetDefaultSubModel()->CountSubModels(inModelID);

	if (mSubModels) {
		LListIterator	iterator(*mSubModels, iterate_FromStart);
		LModelObject	*p;
	
		while (iterator.Next(&p)) {
			if ((p->GetModelKind() == inModelID) || (inModelID == typeWildCard))
				count++;
		}
	}
	
	return count;
}


// ---------------------------------------------------------------------------
//		е GetModelToken
// ---------------------------------------------------------------------------
//	Get a Token for the specified Model(s).
//
//	This method not only includes possible resolutions using "this" but also
//	recursively includes resolutions on the default submodel of "this."
//
//	This function gives preference to resolutions off of the default submodel
//	rather than resolutions off of "this."  This is because:
//
//	е	the default submodel is probably what was wanted anyway,
//
//	е	if there is no default submodel there is no real performance penatly,
//
//	е	it prevents "link" objects from erroneously adding their submodels
//		to the output token.
//
//	If a token is found in the default submodel, no resolution based off of
//	"this" will be made.  This has the consequence of models in the default
//	submodel effectively hiding similar models of "this."

void
LModelObject::GetModelToken(
	DescType		inModelID,
	DescType		inKeyForm,
	const AEDesc	&inKeyData,
	AEDesc			&outToken) const
{
	OSErr	err = noErr,
			errb = noErr;	//	mainly for step mode debugging
	AEDesc	originalOutToken = outToken;

	Assert_(outToken.descriptorType == typeNull ||
			outToken.descriptorType == type_ModelToken ||
			outToken.descriptorType == typeAEList);
		
	//	recursively try default submodel(s)...
	if (GetDefaultSubModel()) {
		#ifdef	Debug_Throw
			//	It IS okay for the try below to fail, so turn off
			//	Throw debugging.
			StValueChanger<EDebugAction>
				okayToFail(gDebugThrow, debugAction_Nothing);
		#endif

		Try_ {
			GetDefaultSubModel()->GetModelToken(inModelID, inKeyForm,
												inKeyData, outToken);
		} Catch_(inErr) {
			err = inErr;
		//	fall through...
		} EndCatch_;
	}

	//	"this" model object...
	 if (outToken.descriptorType == typeNull) {
		Try_ {
			GetModelTokenSelf(inModelID, inKeyForm, inKeyData, outToken);
		} Catch_(inErr) {
			errb = inErr;
			//	fall through...
		} EndCatch_;
	}

	if ( (outToken.descriptorType == originalOutToken.descriptorType) &&
		 (outToken.dataHandle == originalOutToken.dataHandle) ) {
		ThrowOSErr_(errAENoSuchObject);
	}
}


// ---------------------------------------------------------------------------
//		е GetModelTokenSelf
// ---------------------------------------------------------------------------
//	Get a Token for the specified Model(s) without considering the
//	default submodel.
//
//	This function dispatches a call to an accessor using a specific
//	means of identification (key form). You will rarely override this.
//
//	Do not call this member.  Consider calling GetModelToken instead.

void
LModelObject::GetModelTokenSelf(
	DescType		inModelID,
	DescType		inKeyForm,
	const AEDesc	&inKeyData,
	AEDesc			&outToken) const
{
	switch (inKeyForm) {
		
		case formAbsolutePosition: {
			Int32		subPosition = 0,
						subCount;
			DescType	subSpec = typeNull;
			Boolean		needCount = false;
			
			if (inKeyData.descriptorType != typeAbsoluteOrdinal) {
				UExtractFromAEDesc::TheInt32(inKeyData, subPosition);
			} else {
				subSpec = **(DescType**) inKeyData.dataHandle;
			}
			
			if (subPosition < 0) {
				needCount = true;
			}
			
			switch (subSpec) {
				case kAEMiddle:
				case kAELast:
				case kAEAny:
					needCount = true;
					break;
			}
			
			if (needCount) {
				subCount = CountSubModels(inModelID);
			}

			switch (subSpec) {
			
				case kAEFirst:
					subPosition = 1;
					break;
					
				case kAEMiddle:
					subPosition = (subCount + 1) / 2;
					break;
					
				case kAELast:
					subPosition = subCount;
					break;
					
				case kAEAny:
					subPosition = 0;
					if (subCount > 0) {
						subPosition = ((Uint16) Random() % (Uint16) subCount)
										+ 1;
					}
					break;
					
				case kAEAll:
					subPosition = position_AllSubModels;
					break;
				
				default:
					if (subPosition < 0) {	// Negative position counts back from
											//   end, with -1 being the last item
						subPosition += subCount + 1;
					}
					break;
			}

			if (subPosition == position_AllSubModels) {
				GetAllSubModels(inModelID, outToken);
			} else {
				GetSubModelByPosition(inModelID, subPosition, outToken);
			}
			break;
		}
			
		case formName: {
			Str255	subModelName;
			UExtractFromAEDesc::ThePString(inKeyData, subModelName);
			GetSubModelByName(inModelID, subModelName, outToken);
			break;
		}
			
		case formUniqueID:
			GetSubModelByUniqueID(inModelID, inKeyData, outToken);
			break;
			
		case formRelativePosition: {
			OSType	relativePosition;
			UExtractFromAEDesc::TheEnum(inKeyData, relativePosition);
			GetModelByRelativePosition(inModelID, relativePosition, outToken);
			break;
		}
		
		case formPropertyID: {
			DescType		propertyID;
			LModelObject	*property = NULL;
			
			UExtractFromAEDesc::TheType(inKeyData, propertyID);

			property = GetModelProperty(propertyID);
			//	That wasn't GetModelPropertyAll!
			//	If property exists, it is a real property...
			
			if (!property) {

				//	Need to see if a property exists under a "fabricated"
				//	LModelProperty.  This is done by seeing if GetAEProperty
				//	succeeds.

				StAEDescriptor	bogus;
				Boolean			propertyExists = false;
				{
					#ifdef	Debug_Throw
						//	It IS okay for the try below to fail, so turn off
						//	Throw debugging.
						StValueChanger<EDebugAction>
								okayToFail(gDebugThrow, debugAction_Nothing);
					#endif

					Try_ {
						GetAEProperty(propertyID, UAppleEventsMgr::sAnyType, bogus.mDesc);
						propertyExists = true;
					} Catch_(inErr) {
						//	Don't propogate
					} EndCatch_;
				}
				
				if (propertyExists) {
					//	Fabricate the property w/ GetModelPropertyAll
					property = GetModelPropertyAll(propertyID);
				}
			}

			if (property) {
				PutInToken(property, outToken);
			}
			break;
		}

		default:
			GetSubModelByComplexKey(inModelID, inKeyForm, inKeyData,
									outToken);
			break;
	}
}


// ---------------------------------------------------------------------------
//		е GetSubModelByPosition
// ---------------------------------------------------------------------------
//	Pass back a Token for the SubModel(s) of the specified type at the
//	specified position (1 being the first).
//
//	Must be overridden by subclasses which have SubModels that aren't
//	implemented using the submodel list (ie lazy instantiated submodels).

void
LModelObject::GetSubModelByPosition(
	DescType		inModelID,
	Int32			inPosition,
	AEDesc			&outToken) const
{
	if (mSubModels) {
		LListIterator	iterator(*mSubModels, iterate_FromStart);
		Int32			index = 0;
		LModelObject	*p;
		Boolean			found = false;
		
		while (iterator.Next(&p)) {
			if ((p->GetModelKind() == inModelID) || (inModelID == typeWildCard)) {
				index++;
				if (index == inPosition) {
					found = true;
					break;
				}
			}
		}
		
		if (found) {
			PutInToken(p, outToken);
		}
	}
}


// ---------------------------------------------------------------------------
//		е GetSubModelByName
// ---------------------------------------------------------------------------
//	Pass back a Token for the SubModel(s) of the specified type with the
//	specified name
//
//	Must be overridden by subclasses which have SubModels that aren't
//	implemented using the submodel list (ie lazy instantiated submodels).

void
LModelObject::GetSubModelByName(
	DescType		inModelID,
	Str255			inName,
	AEDesc			&outToken) const
{
	if (mSubModels) {
		LListIterator	iterator(*mSubModels, iterate_FromStart);
		LModelObject	*p;
		Boolean			found = false;
		StringPtr		str;
		
		while (iterator.Next(&p)) {
			if ((p->GetModelKind() == inModelID) || (inModelID == typeWildCard)) {
				str = p->GetModelNamePtr();
				if (str) {
					if (::EqualString(str, inName, true, true)) {
						found = true;
						break;
					}
				}
			}
		}
		
		if (found) {
			PutInToken(p, outToken);
			return;
		}
	}
}


// ---------------------------------------------------------------------------
//		е GetSubModelByUniqueID
// ---------------------------------------------------------------------------
//	Pass back a Token for the SubModel(s) of the specified type with the
//	specified unique ID
//
//	Must be overridden by subclasses which have SubModels that aren't
//	implemented using the submodel list (ie lazy instantiated submodels).
//
//	It is up to you to decide what constitutes a unique ID and you must also
//	provide a CompareToUniqueID().

void
LModelObject::GetSubModelByUniqueID(
	DescType		inModelID,
	const AEDesc	&inKeyData,
	AEDesc			&outToken) const
{
	if (mSubModels) {
		LListIterator	iterator(*mSubModels, iterate_FromStart);
		Int32			index = 0;
		LModelObject	*p;
		Boolean			found = false;
		
		while (iterator.Next(&p)) {
			if (p->GetModelKind() == inModelID) {
				if (p->CompareToUniqueID(kAEEquals, inKeyData)) {
					found = true;
					break;
				}
			}
		}
		
		if (found) {
			PutInToken(p, outToken);
			return;
		}
	}
}


// ---------------------------------------------------------------------------
//		е GetModelByRelativePosition
// ---------------------------------------------------------------------------
//	Pass back a Token for the Model of the specified type at the specified
//	relative position ("before" or "after" this ModelObject).
//
//	This function handles the case where the Model to get is of the same
//	kind as this ModelObject by getting the position of this ModelObject,
//	then getting the ModelObject at the position before or after it.
//	For example, to get the "paragraph after this paragraph", we get the
//	position of the is paragraph, add one, then get the paragraph at
//	that position.
//
//	Subclass should override this function to implement a more efficient
//	way to get the next or previous item, or to handle the case where the
//	object to get is a different kind of object. For example, to be able
//	to get the "table after this paragraph".

void
LModelObject::GetModelByRelativePosition(
	DescType		inModelID,
	OSType			inRelativePosition,
	AEDesc			&outToken) const
{
	if (inModelID == GetModelKind()) {
		LModelObject	*theSuper = mSuperModel;
		if (theSuper == nil) {
			theSuper = GetDefaultModel();
		}
	
		Int32	thePosition = theSuper->GetPositionOfSubModel(inModelID, this);

		switch (inRelativePosition) {

			case kAENext:
				thePosition += 1;
				break;
				
			case kAEPrevious:
				thePosition -= 1;
				break;
			
			default:
				return;
		}
		theSuper->GetSubModelByPosition(inModelID, thePosition, outToken);
	}
}


// ---------------------------------------------------------------------------
//		е GetSubModelByComplexKey
// ---------------------------------------------------------------------------
//	Pass back a Token for the SubModel(s) of the specified type identified
//	by a complex key. Complex keys are formRange, formTest, and formWhose.
//
//	Subclasses which support complex keys for identifying SubModels must
//	override this function.

void
LModelObject::GetSubModelByComplexKey(
	DescType		/* inModelID */,
	DescType		/* inKeyForm */,
	const AEDesc&	/* inKeyData */,
	AEDesc&			/* outToken */) const
{
	//	try submodels...
	//	How?  It is complex key & application specific.
}


// ---------------------------------------------------------------------------
//		е GetAllSubModels
// ---------------------------------------------------------------------------
//	Pass back a Token list for all SubModels of the specified type
//
//	This function uses a brute force approach:
//		Get the count of items
//		Get Token for each item in order and add it to the list
//
//	Override this function if a ModelObject can create this Token list
//	in a more efficient manner.

void
LModelObject::GetAllSubModels(
	DescType		inModelID,
	AEDesc			&outToken) const
{
	Int32	subCount = CountSubModels(inModelID);
	OSErr	err;
	
	if (subCount > 0) {
		if (outToken.descriptorType == typeNull) {
			err = ::AECreateList(nil, 0, false, &outToken);
			ThrowIfOSErr_(err);
		}
		
		for (Int32 i = 1; i <= subCount; i++) {
			StAEDescriptor	subToken;
			
			GetSubModelByPosition(inModelID, i, subToken.mDesc);
			err = ::AEPutDesc(&outToken, 0, &subToken.mDesc);
			ThrowIfOSErr_(err);
		}
	}
}


// ---------------------------------------------------------------------------
//		е GetPositionOfSubModel
// ---------------------------------------------------------------------------
//	Return the absolute position of the specified SubModel, with 1 being
//	the first
//
//	Must be overridden by subclasses which have SubModels that aren't
//	implemented using the submodel list (ie lazy instantiated submodels).

Int32
LModelObject::GetPositionOfSubModel(
	DescType			inModelID,
	const LModelObject	*inSubModel) const
{
	Int32			index = 0;

	//	try submodels...
	if (mSubModels) {
		LListIterator	iterator(*mSubModels, iterate_FromStart);
		LModelObject	*p;
		Boolean			found = false;
		
		while (iterator.Next(&p)) {
			if ((p->GetModelKind() == inModelID) || (inModelID == typeWildCard)) {
				index++;
				if (p == inSubModel)
					return index;
			}
		}
	}
	
	if (index == 0)
		ThrowOSErr_(errAENoSuchObject);	//	Actually more of an internal
										//	implementation error.
	
	return index;
}


// ---------------------------------------------------------------------------
//		е GetInsertionTarget
//		е GetInsertionContainer
//		е GetInsertionElement
// ---------------------------------------------------------------------------
//	These three functions help in dealing with typeInsertionLoc parameters.
//	
//	GetInsertionTarget converts the ModelObject referred to in the kAEObject
//	typeInsertionLoc sub parameter to the actual insertion target.
//	Specifically, GetInsertionTarget takes default submodels into
//	consideration.  The GetInsertionTarget result should be used as the
//	object to pass the other two messages thru.
//
//	GetInsertionContainer returns the "container" object of the
//	typeInsertionLoc parameter.
//
//	GetInsertionElement returns the actual element that represents the
//	typeInsertionLoc ModelObject.
//
//	All three methods take an "inInsertPosition" DescType parameter.
//	InInsertPosition corresponds to the kAEPosition value of the
//	typeInsertionLoc record.  If the AppleEvent parameter corresponding to
//	an insertion location is an object specifier instead of a
//	typeInsertionLoc, use an inInsertPosition value of typeNull.  This
//	convention is useful when an insertion location is completely
//	representable by an object specifier rather than having to use a 
//	typeInsertionLoc record.
//
//	You may need to override GetInsertionElement to lazy instantiate an
//	object, or allow its return value to be nil.  The later works fine
//	in HandleCreateElementEvents where the element is the result of the
//	HandleCreateElementEvent.

LModelObject *
LModelObject::GetInsertionTarget(DescType inInsertPosition) const
{
	LModelObject	*model = NULL;
	
	if (GetDefaultSubModel())
		model = GetDefaultSubModel()->GetInsertionTarget(inInsertPosition);

	if (model)
		return model;
	else
		return (LModelObject *) this;
}

LModelObject *
LModelObject::GetInsertionContainer(DescType inInsertPosition) const
{
	LModelObject	*container = NULL;

	switch (inInsertPosition) {

		case typeNull:
			container = ((LModelObject *)this)->GetSuperModel();
			break;
		
		case kAEBefore:		// For these positions, the insert target
		case kAEAfter:		// specifies an object in the same
		case kAEReplace:	// container as that in which to create
							// the new element
			container = ((LModelObject *)this)->GetSuperModel();
			break;
			
		case kAEBeginning:	// For these position, the insert target
		case kAEEnd:		// is the container in which to create
							// the new element
			container = (LModelObject *)this;
			break;
			
		default:
			ThrowOSErr_(errAEEventNotHandled);
			break;
	}
	
	return container;
}

LModelObject *
LModelObject::GetInsertionElement(DescType inInsertPosition) const
{
	LModelObject	*model = NULL;

	switch (inInsertPosition) {

		case typeNull:
			model = (LModelObject *)this;
			break;
			
		case kAEReplace:
			model = (LModelObject *)this;
			break;

		case kAEBefore:
		case kAEAfter:
		case kAEBeginning:
		case kAEEnd:
			//	you need to override this method to do the mapping to
			//	outModel or, if this is being used in a HandleCreateElementEvent,
			//	recognize the outModel hasn't been created yet.
			model = NULL;
			break;
		
		default:
			Throw_(errAEEventNotHandled);
			break;
	}
	
	return model;
}


// ---------------------------------------------------------------------------
//		е GetModelNamePtr
// ---------------------------------------------------------------------------
//	Return the name of a ModelObject

const StringPtr
LModelObject::GetModelNamePtr() const
{
	return nil;
}


// ---------------------------------------------------------------------------
//		е CompareToModel
// ---------------------------------------------------------------------------
//	Return result of comparing this ModelObject with another one
//
//	Subclasses must override this method to support accessing objects
//	by selection criteria, commonly called "whose" clauses.
//		For example:  get all words equal to word 1
//
//	This default implementation only provides model object pointer equivalence
//	and containment.

Boolean
LModelObject::CompareToModel(
	DescType		inComparisonOperator,
	LModelObject*	inCompareModel) const
{
	Boolean	result = false;
	
	switch (inComparisonOperator) {

		case kAEEquals:
			result = inCompareModel == this;
			break;
			
		case kAEContains:
			if (inCompareModel)
				result = inCompareModel->IsSubModelOf(this);
			break;
			
		
		default:
			ThrowOSErr_(errAEEventNotHandled);
			break;
	}
	
	return result;
}	


// ---------------------------------------------------------------------------
//		е CompareToDescriptor
// ---------------------------------------------------------------------------
//	Return result of comparing this ModelObject with Descriptor data
//
//	Subclasses must override this method to support accessing objects
//	by selection criteria, commonly called "whose" clauses.
//		For example:  get all words that contain "foo"

Boolean
LModelObject::CompareToDescriptor(
	DescType		/* inComparisonOperator */,
	const AEDesc&	/* inCompareDesc */) const
{
	ThrowOSErr_(errAEEventNotHandled);
	return false;
}


// ---------------------------------------------------------------------------
//		е CompareToUniqueID
// ---------------------------------------------------------------------------
//	Return result of comparing this ModelObject with a "unique id."
//
//	It is up to you to decide what constitutes a unique ID.

Boolean
LModelObject::CompareToUniqueID(
	DescType		/* inComparisonOperator */,
	const AEDesc&	/* inCompareDesc */) const
{
	ThrowOSErr_(errAEEventNotHandled);
	return false;
}


// ---------------------------------------------------------------------------
//		е CompareProperty
// ---------------------------------------------------------------------------
//	Return the result of comparing a property of this ModelObject with
//	another object
//
//	Subclasses must override this method to support accessing objects by
//	selection criteria on their properties, commonly called "whose" clauses.
//		For example:  get all words whose point size is less than 12 

Boolean
LModelObject::CompareProperty(
	DescType		/* inPropertyID */,
	DescType		/* inComparisonOperator */,
	const AEDesc&	/* inCompareObjectOrDesc */) const
{
	ThrowOSErr_(errAEEventNotHandled);
	return false;
}


// ---------------------------------------------------------------------------
//		е GetModelPropertyAll
// ---------------------------------------------------------------------------
//	Return a ModelObject object representing the specified property
//
//	This implementation always creates a LModelProperty object for the
//	specificied property (although the property may not really exist).
//	For nonexistent properties, a later attempt to get or set the property
//	value will pass back an error.
//
//	To return a special LModelObject for a given property,
//	override GetModelProperty (which is called by this function).

LModelObject*
LModelObject::GetModelPropertyAll(
	DescType	inProperty) const
{
	LModelObject	*property = GetModelProperty(inProperty);
	
	if (property == nil) {
		property = new LModelProperty(inProperty, (LModelObject *)this);
	}
	
	return property;
}


// ---------------------------------------------------------------------------
//		е GetModelProperty
// ---------------------------------------------------------------------------
//	Return a ModelObject object for explicitly defined properties
//
//	Must be overridden for subclasses which return "special" ModelObjects for
//	given property id's.
//
//	For the default case in overrides, return the inherited value.

LModelObject*
LModelObject::GetModelProperty(
	DescType	/* inProperty */) const
{
	return nil;
}


// ---------------------------------------------------------------------------
//		е MakeSpecifier
// ---------------------------------------------------------------------------
//	Make an Object Specifier for a ModelObject
//
//	This is a helper function that uses recursion to call MakeSelfSpecifier
//	for a ModelObject and all its SuperModels. You will not need to
//	override this function.

void
LModelObject::MakeSpecifier(
	AEDesc	&outSpecifier) const
{
	if (GetDefaultSuperModel() != NULL) {
		GetDefaultSuperModel()->MakeSpecifier(outSpecifier);
	} else {
		StAEDescriptor		superSpecifier;
		
		if (mSuperModel != nil) {
			mSuperModel->MakeSpecifier(superSpecifier.mDesc);
		}
		
		MakeSelfSpecifier(superSpecifier.mDesc, outSpecifier);
	}
}


// ---------------------------------------------------------------------------
//		е MakeSelfSpecifier
// ---------------------------------------------------------------------------
//	Make an Object Specifier for a ModelObject
//
//	This function creates a specifier using the absolute position of
//	a ModelObject within its SuperModel.
//
//	Override this function for subclasses which can't be specified by
//	absolute position or for which another means of identification
//	(for example, by name) is more appropriate or that don't have
//	a SuperModel (access from null container).

void
LModelObject::MakeSelfSpecifier(
	AEDesc	&inSuperSpecifier,
	AEDesc	&outSelfSpecifier) const
{
		// Find position of this ModelObject within its SuperModel
		// Error if SuperModel does not exist or if SuperModel
		// can't return a valid position for this object

	if (mSuperModel == nil) {
		ThrowOSErr_(errAEEventNotHandled);
	}

	Int32	modelIndex = mSuperModel->GetPositionOfSubModel(
										GetModelKind(), this);
	if (modelIndex == 0) {
		ThrowOSErr_(errAEEventNotHandled);
	}
	
	StAEDescriptor	absPosKeyData;
	OSErr	err = ::CreateOffsetDescriptor(modelIndex, &absPosKeyData.mDesc);
	ThrowIfOSErr_(err);
	
	err = ::CreateObjSpecifier(GetModelKind(), &inSuperSpecifier,
			formAbsolutePosition, &absPosKeyData.mDesc, false,
			&outSelfSpecifier);
	ThrowIfOSErr_(err);
}


// ---------------------------------------------------------------------------
//		е HandleAppleEventAll
// ---------------------------------------------------------------------------
//	Try handling an AppleEvent by both "this" ModelObject and its default
//	submodel.
//
//	If submodel handling succeeds, "this" is not tried.

void
LModelObject::HandleAppleEventAll(
	const AppleEvent	&inAppleEvent,
	AppleEvent			&outAEReply,
	AEDesc				&outResult,
	long				inAENumber)
{
	Boolean	handled = false;

	if (GetDefaultSubModel()) {
		try {
			#ifdef	Debug_Throw
				//	It IS okay for the try below to fail, so turn off
				//	Throw debugging.
				StValueChanger<EDebugAction>
					okayToFail(gDebugThrow, debugAction_Nothing);
			#endif
	
			GetDefaultSubModel()->HandleAppleEventAll(inAppleEvent,
									outAEReply, outResult, inAENumber);
			handled = true;
		}
		
		catch (ExceptionCode inErr) {
							// Do nothing. It's OK for the default
		}					// SubModel to fail.
	}

	if (!handled) {			// Default SubModel didn't handle it, so this
							// object must handle it
		HandleAppleEvent(inAppleEvent, outAEReply, outResult, inAENumber);
	}
}


// ---------------------------------------------------------------------------
//		е HandleCreateElementEventAll
// ---------------------------------------------------------------------------
//	Try handling a CreateElementEvent by both "this" ModelObject and its
//	default submodel.
//
//	If submodel handling succeeds, "this" is not tried.

LModelObject*
LModelObject::HandleCreateElementEventAll(
	DescType			inElemClass,
	DescType			inInsertPosition,
	LModelObject		*inTargetObject,
	const AppleEvent	&inAppleEvent,
	AppleEvent			&outAEReply)
{
	LModelObject	*model = NULL;
	OSErr			err = noErr;
	Boolean			handled = false;
	
	if (!handled && GetDefaultSubModel()) {
		Try_ {
			#ifdef	Debug_Throw
				//	It IS okay for the try below to fail, so turn off
				//	Throw debugging.
				StValueChanger<EDebugAction>
					okayToFail(gDebugThrow, debugAction_Nothing);
			#endif
	
			model = GetDefaultSubModel()->
						HandleCreateElementEventAll(
									inElemClass, inInsertPosition,
									inTargetObject, inAppleEvent, outAEReply);
			handled = true;
		} Catch_(inErr) {
			handled = false;
			err = inErr;	//	don't rethrow -- yet
		} EndCatch_;
	}
	
	if (!handled) {
		Try_ {
			model = HandleCreateElementEvent(
									inElemClass, inInsertPosition,
									inTargetObject, inAppleEvent, outAEReply);
			handled = true;
		} Catch_(inErr) {
			handled = false;
			err = inErr;	//	don't rethrow -- yet
		} EndCatch_;
	}
	
	if (!handled)
		ThrowOSErr_(err);

	return model;
}


// ---------------------------------------------------------------------------
//		е HandleAppleEvent
// ---------------------------------------------------------------------------
//	Respond to an AppleEvent. This is the default handler for AppleEvents
//	that do not have a specific handler.
//
//	Subclasses must override this function to support AppleEvents other than:
//		Create Element,
//		Get Data,
//		Set Data,
//		Clone,
//		Move,
//		CountElements,
//		Delete,
//	
//
//	The events above have specific handler methods which may be overridden.

void
LModelObject::HandleAppleEvent(
	const AppleEvent	&inAppleEvent,
	AppleEvent			&outAEReply,
	AEDesc				&outResult,
	long				inAENumber)
{
	switch (inAENumber) {

		case ae_Clone:
			HandleClone(inAppleEvent, outAEReply, outResult);
			break;

		case ae_Move:
			HandleMove(inAppleEvent, outAEReply, outResult);
			break;

		case ae_CountElements:
#ifdef	UAEGIZMOS
		{
			LAESubDesc	classSD(keyAEObjectClass, inAppleEvent, typeType);
			
			HandleCount(classSD.ToType(), outResult);
			break;
		}
#else
		{
			StAEDescriptor	classDesc(inAppleEvent, keyAEObjectClass, typeType);
			DescType		classType;
			UExtractFromAEDesc::TheType(classDesc, classType);
			HandleCount(classType, outResult);
			break;
		}
#endif
//		case ae_Create:						//	See HandleCreateElementEvent

		case ae_Delete:
			HandleDelete(outAEReply, outResult);
			break;
		
//		case ae_DoObjectsExist:				//	Implemented in elsewhere?

//		case ae_GetClassInfo:				//	Not implemented

		case ae_GetData:
		case ae_GetDataSize:
		case ae_SetData:
		{
			//	Must be referring to contents so...  simulate it.
			LModelObject	*content = GetModelPropertyAll(pContents);
			ThrowIfNULL_(content);
			content->HandleAppleEvent(inAppleEvent, outAEReply, outResult, inAENumber);
			break;
		}

		default:
			ThrowOSErr_(errAEEventNotHandled);
			break;
	}
}


// ---------------------------------------------------------------------------
//		е HandleCreateElementEvent
// ---------------------------------------------------------------------------
//	Respond to a Create Element AppleEvent ("make new" in AppleScript).
//
//	The parameters specify the Class ID for the new element, and where to
//	insert the new element in relation to a target object. Also, the
//	AppleEvent record may contain additional parameters that specify
//	initial values for the new element.
//
//	Subclasses which have SubModels which can be dynamically created should
//	override this function. Return a pointer to the newly created SubModel.
//	The calling function takes care of putting an object specifier for
//	this new SubModel in the AppleEvent reply. 

LModelObject*
LModelObject::HandleCreateElementEvent(
	DescType			/* inElemClass */,
	DescType			/* inInsertPosition */,
	LModelObject*		/* inTargetObject */,
	const AppleEvent&	/* inAppleEvent */,
	AppleEvent&			/* outAEReply */)
{
	ThrowOSErr_(errAEEventNotHandled);
	return nil;
}


// ---------------------------------------------------------------------------
//		е HandleCount
// ---------------------------------------------------------------------------
//	Respond to the Count Elements AppleEvent ("count" in AppleScript).
//
//	The parameters specify the Class ID of submodels to count.
//
//	You should not need to explicitly call this method.
//	You should not need to override this method.

void
LModelObject::HandleCount(
	DescType			inModelID,
	AppleEvent			&outResult)
{
	Int32	count = CountSubModels(inModelID);
	
	UAEDesc::AddPtr(&outResult, 0, typeLongInteger, &count, sizeof(count));
}

// ---------------------------------------------------------------------------
//		е HandleDelete
// ---------------------------------------------------------------------------
//	Respond to the Delete AppleEvent ("delete" in AppleScript).
//
//	You should not need to explicitly call this method.
//	You may need to override and inherit this method.

void
LModelObject::HandleDelete(
	AppleEvent&	/* outAEReply */,
	AEDesc&		/* outResult */)
{
	if (GetModelKind() == cProperty)
		Throw_(errAEEventNotHandled);	//	doesn't make sense to delete properties.	
	
	SetLaziness(true);	//	Should automatically delete when AE is done
}

// ---------------------------------------------------------------------------
//		е HandleClone
// ---------------------------------------------------------------------------
//	Respond to the Clone AppleEvent ("duplicate" in AppleScript).
//
//	You should not need to explicitly call this method.
//	You rarely need to override this method.

void
LModelObject::HandleClone(
	const AppleEvent	&inAppleEvent,
	AppleEvent&			/* outAEReply */,
	AEDesc				&outResult
)
{
#ifdef	UAEGIZMOS
	OSErr			err = noErr;
	StAEDescriptor	objProps,
					createEvent,
					replyEvent;
	DescType		objectClass = GetModelKind();
	LAESubDesc		targetSD(keyAEInsertHere, inAppleEvent);

	//	ее	Get data for clone object
	this->GetImportantAEProperties(objProps.mDesc);

	//	ее	Create element event
	{
		LAEStream	stream(kAECoreSuite, kAECreateElement);
		
		//	keyAEData
		//		left empty -- all data (if any) goes through objProps
		
		//	keyAEInsertHere
		stream.WriteKey(keyAEInsertHere);
		switch (targetSD.GetType()) {
	
			case typeNull:
				//	make an insertion location specifier for after this object
				stream.OpenRecord(typeInsertionLoc);
				
				stream.WriteKey(keyAEObject);
				stream.WriteSpecifier(this);
				
				stream.WriteKey(keyAEPosition);
				stream.WriteEnumDesc(kAEAfter);
				
				stream.CloseRecord();
				break;
	
			case typeObjectSpecifier:
				stream.WriteSubDesc(targetSD);
				objectClass = targetSD.KeyedItem(keyAEDesiredClass).ToType();
				break;
	
			case typeInsertionLoc:
			{
				stream.WriteSubDesc(targetSD);
				LModelObject	*target = targetSD.KeyedItem(keyAEObject).ToModelObject();
				target = target->GetInsertionTarget(targetSD.KeyedItem(keyAEPosition).ToEnum());
				objectClass = target->GetModelKind();
				break;
			}
		}
	
		//	keyAEObjectClass
		stream.WriteKey(keyAEObjectClass);
		stream.WriteTypeDesc(objectClass);
		
		//	keyAEPropData
		if (objProps.mDesc.descriptorType != typeNull) {
			stream.WriteKeyDesc(keyAEPropData, objProps.mDesc);
		}
		
		stream.Close(&createEvent.mDesc);
	}
	
	//	ее	Execute create element event (but don't record)
	StLazyLock	lockMe(this);	//	Don't lose ourself from implied FinalizeLazies().
	UAppleEventsMgr::SendAppleEventWithReply(createEvent.mDesc, replyEvent.mDesc, false);
	
	//	ее	Put result of create element event in reply
	err = AEGetKeyDesc(&replyEvent.mDesc, keyAEResult, typeObjectSpecifier, &outResult);
	ThrowIfOSErr_(err);
#else
	OSErr			err = noErr;
	StAEDescriptor	objProps,
					createEvent,
					replyEvent;
	DescType		objectClass = GetModelKind();
	StAEDescriptor	targetD(inAppleEvent, keyAEInsertHere);

	//	ее	Get data for clone object
	this->GetImportantAEProperties(objProps.mDesc);

	//	ее	Create element event
	{
		UAppleEventsMgr::MakeAppleEvent(kAECoreSuite, kAECreateElement, createEvent.mDesc);
		
		//	keyAEData
		//		left empty -- all data (if any) goes through objProps
		
		//	keyAEInsertHere
		StAEDescriptor	insertHere;
		switch (targetD.mDesc.descriptorType) {
	
			case typeNull:
			{
				//	make an insertion location specifier for after this object
				StAEDescriptor	ospec;
				MakeSpecifier(ospec.mDesc);
				UAEDesc::MakeInsertionLoc(ospec.mDesc, kAEAfter, &insertHere.mDesc);
				UAEDesc::AddKeyDesc(&createEvent.mDesc, keyAEInsertHere, insertHere);
				break;
			}
	
			case typeObjectSpecifier:
			{
				UAEDesc::AddKeyDesc(&createEvent.mDesc, keyAEInsertHere, targetD);
				StAEDescriptor	tokenD;
				err = LModelDirector::Resolve(targetD, tokenD.mDesc);
				ThrowIfOSErr_(err);
				LModelObject	*target = GetModelFromToken(tokenD);
				objectClass = target->GetModelKind();
				break;
			}
	
			case typeInsertionLoc:
			{
				UAEDesc::AddKeyDesc(&createEvent.mDesc, keyAEInsertHere, targetD);
				StAEDescriptor	objectD(targetD, keyAEObject, typeObjectSpecifier),
								tokenD;
				err = LModelDirector::Resolve(objectD, tokenD.mDesc);
				ThrowIfOSErr_(err);
				LModelObject	*target = GetModelFromToken(tokenD);
				StAEDescriptor	positionD(targetD, keyAEPosition, typeEnumeration);
				DescType		position;
				UExtractFromAEDesc::TheEnum(positionD, position);
				target = target->GetInsertionTarget(position);
				objectClass = target->GetModelKind();
				break;
			}
		}
	
		//	keyAEObjectClass
		StAEDescriptor	classD(typeType, &objectClass, sizeof(objectClass));
		UAEDesc::AddKeyDesc(&createEvent.mDesc, keyAEObjectClass, classD);
		
		//	keyAEPropData
		if (objProps.mDesc.descriptorType != typeNull) {
			UAEDesc::AddKeyDesc(&createEvent.mDesc, keyAEPropData, objProps);
		}
	}
	
	//	ее	Execute create element event (but don't record)
	StLazyLock	lockMe(this);	//	Don't lose ourself from implied FinalizeLazies().
	UAppleEventsMgr::SendAppleEventWithReply(createEvent.mDesc, replyEvent.mDesc, false);
	
	//	ее	Put result of create element event in reply
	err = AEGetKeyDesc(&replyEvent.mDesc, keyAEResult, typeObjectSpecifier, &outResult);
	ThrowIfOSErr_(err);
#endif
}

// ---------------------------------------------------------------------------
//		е HandleMove
// ---------------------------------------------------------------------------
//	Respond to the Move AppleEvent ("move" in AppleScript).
//
//	You should not need to explicitly call this method.
//	You will rarely need to override this method.

void
LModelObject::HandleMove(
	const AppleEvent	&inAppleEvent,
	AppleEvent			&outAEReply,
	AEDesc				&outResult)
{
#ifdef	UAEGIZMOS
	StAEDescriptor	cloneResult,
					deleteResult;
	LModelObject	*clone = NULL;
	
	//	ее	Clone behavior
	//
	//		If this is an offset based lazy object, the create element event
	//		in the default HandleClone should "magically" update this object's
	//		offsets as necessary.
	this->HandleClone(inAppleEvent, outAEReply, cloneResult.mDesc);
	
	//	ее	Delete behavior
	//
	//		If the object to be removed is also in a lazy object scheme, 
	//		conversion of the above cloneResult to an LModelObject will allow 
	//		the result object specifier (the new copy) to have its offsets
	//		"magically" updated by the HandleDelete.  So, get the clone now,
	//		do the delete, then make the adjusted result specifier.
	LAESubDesc		cloneSD(cloneResult.mDesc);
	clone = (LModelObject *)cloneSD.ToModelObject();
	this->HandleDelete(outAEReply, deleteResult.mDesc);

	//	ее	"Hole specifier?"
	//
	//	As an aid for undo inside of a PowerPlant app, the HandleDelete
	//	has already set the keyAEInsertHere parameter of the reply to the "hole"
	//	left by the deletion.

	//	ее	Result
	//
	//	Fill in the adjusted object specifier
	clone->MakeSpecifier(outResult);
#else
	StAEDescriptor	cloneResult,
					deleteResult;
	LModelObject	*clone = NULL;
	
	//	ее	Clone behavior
	//
	//		If this is an offset based lazy object, the create element event
	//		in the default HandleClone should "magically" update this object's
	//		offsets as necessary.
	this->HandleClone(inAppleEvent, outAEReply, cloneResult.mDesc);
	
	//	ее	Delete behavior
	//
	//		If the object to be removed is also in a lazy object scheme, 
	//		conversion of the above cloneResult to an LModelObject will allow 
	//		the result object specifier (the new copy) to have its offsets
	//		"magically" updated by the HandleDelete.  So, get the clone now,
	//		do the delete, then make the adjusted result specifier.
	OSErr	err;
	StAEDescriptor	token;
	err = LModelDirector::Resolve(cloneResult, token.mDesc);
	ThrowIfOSErr_(err);
	clone = GetModelFromToken(token);
	this->HandleDelete(outAEReply, deleteResult.mDesc);

	//	ее	"Hole specifier?"
	//
	//	As an aid for undo inside of a PowerPlant app, the HandleDelete
	//	has already set the keyAEInsertHere parameter of the reply to the "hole"
	//	left by the deletion.

	//	ее	Result
	//
	//	Fill in the adjusted object specifier
	clone->MakeSpecifier(outResult);
#endif
}


// ---------------------------------------------------------------------------
//		е GetAEProperty
// ---------------------------------------------------------------------------
//	Return a descriptor for the specified Property
//
//	Subclasses which have Properties must override this function

void
LModelObject::GetAEProperty(
	DescType		inProperty,
	const AEDesc&	/* inRequestedType */,
	AEDesc			&outPropertyDesc) const
{
	switch (inProperty) {
		case pClass:
		{
			DescType	value = GetModelKind();
				
			UAEDesc::AddPtr(&outPropertyDesc, 0, typeType, &value, sizeof(value));
			break;
		}
		
		case pContents:
			MakeSpecifier(outPropertyDesc);
			break;
		
		default:
			ThrowOSErr_(errAEUnknownObjectType);
			break;
	}
}


// ---------------------------------------------------------------------------
//		е SetAEProperty
// ---------------------------------------------------------------------------
//	Set a Property using data from a descriptor
//
//	Subclasses which have modifiable Properties must override this function

void
LModelObject::SetAEProperty(
	DescType		/* inProperty */,
	const AEDesc&	/* inValue */,
	AEDesc&			/* outAEReply */)
{
	ThrowOSErr_(errAEUnknownObjectType);
}


// ---------------------------------------------------------------------------
//		е GetImportantAEProperties
// ---------------------------------------------------------------------------
//	Return a record containing all "important" Properties
//
//	"Important" includes things necessary for cloning.
//
//	Subclasses which have Properties should override this function like:
//
//	{
//		inherited::GetImportantAEProperties(outRecord);
//
//		{	//	font
//			StAEDescriptor	aProp;
//			GetAEProperty(pFont, typeDesc, aProp.mDesc);
//			UAEDesc::AddKeyDesc(&outRecord, pFont, aProp.mDesc);
//		}
//		...
//		{	//	size
//			StAEDescriptor	aProp;
//			GetAEProperty(pSize, typeDesc, aProp.mDesc);
//			UAEDesc::AddKeyDesc(&outRecord, pSize, aProp.mDesc);
//		}
//	}

void
LModelObject::GetImportantAEProperties(AERecord &outRecord) const
{
	OSErr			err;
	StAEDescriptor	contents,
					reqType;
	
	#ifdef	Debug_Throw
		//	It IS okay for the try below to fail, so turn off
		//	Throw debugging.
		StValueChanger<EDebugAction>
			okayToFail(gDebugThrow, debugAction_Nothing);
	#endif

	Try_ {		
		GetAEProperty(pContents, reqType.mDesc, contents.mDesc);
		UAEDesc::AddKeyDesc(&outRecord, pContents, contents.mDesc);
	} Catch_(inErr) {
		err = inErr;
	} EndCatch_;
}


// ---------------------------------------------------------------------------
//		е SendSelfAE
// ---------------------------------------------------------------------------
//	Send an AppleEvent to the current process with this ModelObject as
//	the direct parameter

void
LModelObject::SendSelfAE(
	AEEventClass	inEventClass,
	AEEventID		inEventID,
	Boolean			inExecute)
{
	AppleEvent	theAppleEvent;
	UAppleEventsMgr::MakeAppleEvent(inEventClass, inEventID, theAppleEvent);

	StAEDescriptor	modelSpec;
	MakeSpecifier(modelSpec.mDesc);
	OSErr err = ::AEPutParamDesc(&theAppleEvent, keyDirectObject,
									&modelSpec.mDesc);
	ThrowIfOSErr_(err);

	UAppleEventsMgr::SendAppleEvent(theAppleEvent, inExecute);
}


// ===========================================================================
// е Static Member Functions						 Static Member Functions е
// ===========================================================================

// ---------------------------------------------------------------------------
//	DefaultModel
// ---------------------------------------------------------------------------
//
//	The DefaultModel responds to all AppleEvents not directed at any
//	specific object. It also represents the "null" container and the
//	top container in the AppleEvent Object Model container hierarchy.
//	In most cases, the DefaultModel will be the "Application" object.
//
//	If you do not use a PowerPlant class that automatically sets the
//	DefaultModel (such as one of the Application-type classes), you
//	must create a class that handles the DefaultModel's responsibilities
//	and set the DefaultModel appropriately.

LModelObject*
LModelObject::GetDefaultModel()
{
	return sDefaultModel;
}


void
LModelObject::SetDefaultModel(
	LModelObject	*inModel)
{
	sDefaultModel = inModel;
}


// ---------------------------------------------------------------------------
//	StreamingModel
// ---------------------------------------------------------------------------
//
//	The StreamingModel refers to the last LModelObject that was constructed
//	and that still exists.  It is useful when constructing LModelObject
//	hierarchies.
//
//
//	Implementation note:
//	
//	The StreamingModel should not be a "claiming" shared object reference.
//	Doing such would mean LModelObjects couldn't go away as long as they were
//	the StreamingModel -- that would require extra & explicit code. So...
//	The sStreamingModel is a normal reference that is never left dangling
//	because ~LModelObject will SetStreamingModel(NULL) as necessary.


LModelObject*
LModelObject::GetStreamingModel()
{
	return sStreamingModel;
}

void
LModelObject::SetStreamingModel(
	LModelObject	*inModel)
{
	sStreamingModel = inModel;
}


// ---------------------------------------------------------------------------
//	TellTarget
// ---------------------------------------------------------------------------
//
//	The "TellTarget" (the default submodel for the Application) allows the
//	recording of more concise scripts.
//
//	Call DoAESwitchTellTarget when your application wishes to change the
//	"focus," or "TellTarget" of AppleEvent recording.  When calling
//	DoAESwitchTellTarget, any object specifiers constructed
//	with a previous TellTarget in place become "stale."  This means those
//	object specifiers shouldn't be used when sending or recording
//	subsequent AppleEvents.

void
LModelObject::DoAESwitchTellTarget(
	LModelObject	*inModelObject)
{
	if 	(inModelObject == GetTellTarget())
		return;	//	Don't record a non-effective change
	
	StAEDescriptor	appleEvent;
	UAppleEventsMgr::MakeAppleEvent(kAEPowerPlantSuite, kAESwitchTellTarget, appleEvent.mDesc);
	
	if (inModelObject && (inModelObject != GetTellTarget())) {
		StTempTellTarget	makeOSpecsBasedFrom(NULL);

		StAEDescriptor	ospec;
		inModelObject->MakeSpecifier(ospec.mDesc);
		UAEDesc::AddKeyDesc(&appleEvent.mDesc, keyAEData, ospec);
	}
	
	UAppleEventsMgr::SendAppleEvent(appleEvent.mDesc);
}

	
LModelObject *
LModelObject::GetTellTarget(void)
{
	LModelObject	*defModel = GetDefaultModel();
	
	ThrowIfNULL_(defModel);
	
	return defModel->GetDefaultSubModel();
}


void
LModelObject::SetTellTarget(LModelObject *inModel)
{
	LModelObject	*defModel = GetDefaultModel();
	
	ThrowIfNULL_(defModel);
	
	defModel->SetDefaultSubModel(inModel);
}


// ---------------------------------------------------------------------------
//		е PutInToken
// ---------------------------------------------------------------------------
//	Place the pointer to a ModelObject within a Token. Tokens are used
//	when resolving an AppleEvent object specifier

void
LModelObject::PutInToken(
	LModelObject	*inModel,
	AEDesc			&outToken)
{
	if (inModel == nil)
		ThrowOSErr_(errAENoSuchObject);
		
	SModelToken		theToken;
	theToken.modelObject = inModel;
	
	//	AddPtr will automatically convert outToken to an AEList
	//	when necessary.
	UAEDesc::AddPtr(&outToken, 0, type_ModelToken, &theToken, sizeof(theToken));
}


// ---------------------------------------------------------------------------
//		е GetModelFromToken
// ---------------------------------------------------------------------------
//	Return the ModelObject represented by a Token Descriptor record

LModelObject*
LModelObject::GetModelFromToken(
	const AEDesc	&inToken)
{
	LModelObject*	theModel = nil;

	switch (inToken.descriptorType) {

		case typeNull:
			theModel = GetDefaultModel();
			break;
			
		case type_ModelToken:
			theModel = (**((SModelTokenH) inToken.dataHandle)).modelObject;
			break;
			
		case typeAEList:
			SignalPStr_("\pCan't get token from a list");
			ThrowOSErr_(errAEUnknownObjectType);
			break;

		default:
			SignalPStr_("\pUnknown token type");
			ThrowOSErr_(errAEUnknownObjectType);
			break;
	}

	return theModel;
}


// ---------------------------------------------------------------------------
//		е FinalizeLazies
// ---------------------------------------------------------------------------
//	Send a Finalize message to all objects in the lazy list.

void
LModelObject::FinalizeLazies()
{
	Try_ {
		LListIterator	iterator(*sLazyModels, iterate_FromEnd);
		LModelObject	*model;

		while (iterator.Previous(&model)) {
		
			Try_ {
				model->Finalize();
			} Catch_(inErr) {
			} EndCatch_;
			
		}
		
	} Catch_(inErr) {
		Assert_(false);
		//	Because of usage by LModelDirector, never throw an exception.
	} EndCatch_;
}


// ---------------------------------------------------------------------------
//		е AddLazy
// ---------------------------------------------------------------------------
//	Adds the object to the "lazy" list of objects that receive Finalize
//	messages in response to FinalizeLazies.
//
//	You should never need to call this function.  Use SetLaziness(true)
//	instead.

void
LModelObject::AddLazy(
	LModelObject	*inModel)
{
	sLazyModels->InsertItemsAt(1, arrayIndex_Last, &inModel);
}


// ---------------------------------------------------------------------------
//		е RemoveLazy
// ---------------------------------------------------------------------------
//	Remove the object from the "lazy" list of objects.
//
//	You should never need to call this function.  Use SetLaziness(false)
//	instead.

void
LModelObject::RemoveLazy(
	LModelObject	*inModel)
{
	sLazyModels->Remove(&inModel);
}


// ===========================================================================
// е Helper	Classes											  Helper Classes е
// ===========================================================================


// ---------------------------------------------------------------------------
//	StLazyLock
// ---------------------------------------------------------------------------
//
//	StLazyLock is an "St" class that temporarily changes the "laziness" of a
//	ModelObject to false -- preventing a ModelObject from being deleted 
//	during the scope of the StLazyLock.  This is similar to StSharer but,
//	when the StLazyLock is destroyed, the object will remain even if there
//	are no outstanding claims on the model.

StLazyLock::StLazyLock(
	LModelObject	*inModel)
{
	mModel = inModel;
	if (mModel) {
		mLaziness = mModel->IsLazy();
		mModel->SetLaziness(false);
	}
}

StLazyLock::~StLazyLock()
{
	if (mModel) {
		mModel->SetLaziness(mLaziness);
	}
}


// ---------------------------------------------------------------------------
//	StTempTellTarget
// ---------------------------------------------------------------------------
//
//	Sometimes it is useful to temporarily change the TellTarget (things like
//	recording a complete object specifier instead of a truncated object
//	specifier).  StTempTellTarget provides a exception safe mechanism for
//	doing this for a given scope.
//
//	Use an inModel of NULL to correspond to no TellTarget.

StTempTellTarget::StTempTellTarget(
	LModelObject	*inModel)
{
	mOldTarget = LModelObject::GetTellTarget();
	if (mOldTarget) {
		mOldTargetLaziness = mOldTarget->IsLazy();
		
		mOldTarget->SetLaziness(false);
	}
	
	LModelObject::SetTellTarget(inModel);
}

StTempTellTarget::~StTempTellTarget()
{
	LModelObject::SetTellTarget(mOldTarget);
	
	if (mOldTarget)
		mOldTarget->SetLaziness(mOldTargetLaziness);
}
