// ===========================================================================
//	LModelDirector.cp		   ©1993-1996 Metrowerks Inc. All rights reserved.
// ===========================================================================
//
//	A wrapper class for AppleEvent handlers and the AE Object Support Library
//
//	You should only create one object of this class. This object handles
//	all callbacks from the AppleEvent Manager. It either handles the callback
//	itself, or calls a member function for the LModelObject that is the
//	target of an AppleEvent.
//
//	The callback functions are static functions that call a virtual member
//	function of LModelDirector. You can override these functions in a
//	subclass of LModelDirector to change how AppleEvent callbacks are
//	handled.

#ifdef PowerPlant_PCH
#include PowerPlant_PCH
#endif

#include <LModelDirector.h>
#include <LModelObject.h>
#include <LModelProperty.h>
#include <UAppleEventsMgr.h>
#include <UExtractFromAEDesc.h>
#include <UMemoryMgr.h>

//	#define	UAEGIZMOS
#ifdef	UAEGIZMOS
	#include	<UAEGizmos.h>
#endif

#ifndef __AEREGISTRY__
#include <AERegistry.h>
#endif

#ifndef __AEOBJECTS__
#include <AEObjects.h>
#endif

#ifndef __AEPACKOBJECT__
#include <AEPackObject.h>
#endif

// === Static Member Variables ===
LModelDirector*	LModelDirector::sModelDirector;


// ---------------------------------------------------------------------------
//		¥ LModelDirector
// ---------------------------------------------------------------------------
//	Default Constructor

LModelDirector::LModelDirector()
{
	sModelDirector = this;
	mResolveFlags = kAEIDoMinimum;
	UAppleEventsMgr::Initialize();
	::AEObjectInit();
	LModelObject::InitLazyList();
}


// ---------------------------------------------------------------------------
//		¥ LModelDirector(LModelObject*)
// ---------------------------------------------------------------------------
//	Constructor specifying a default Model

LModelDirector::LModelDirector(
	LModelObject	*inDefaultModel)
{
	sModelDirector = this;
	mResolveFlags = kAEIDoMinimum;
	LModelObject::SetDefaultModel(inDefaultModel);
	UAppleEventsMgr::Initialize();
	::AEObjectInit();
	InstallCallBacks();
	LModelObject::InitLazyList();
}


// ---------------------------------------------------------------------------
//		¥ ~LModelDirector
// ---------------------------------------------------------------------------
//	Destructor

LModelDirector::~LModelDirector()
{
	LModelObject::DestroyLazyList();
}


// ---------------------------------------------------------------------------
//		¥ InstallCallBacks
// ---------------------------------------------------------------------------
//	Install handler and callback functions used by the AppleEvent Manager
//		and the Object Support Library
//	Call this function only once (usually at the beginning of the program)

void
LModelDirector::InstallCallBacks()
{
	OSErr	err;
	
		// Generic handler for all AppleEvents
	
	UAppleEventsMgr::InstallAEHandlers(
			NewAEEventHandlerProc(LModelDirector::AppleEventHandler));
	
		// Specific handler for Open AppleEvent
	
	err = ::AEInstallEventHandler(kCoreEventClass, kAEOpen,
			NewAEEventHandlerProc(LModelDirector::OpenOrPrintEventHandler),
			ae_Open, false);
	ThrowIfOSErr_(err);
	
		// Specific handler for Print AppleEvent
	
	err = ::AEInstallEventHandler(kCoreEventClass, kAEPrint,
			NewAEEventHandlerProc(LModelDirector::OpenOrPrintEventHandler),
			ae_Print, false);
	ThrowIfOSErr_(err);
	
		// Specific handler for CreateElement AppleEvent
	
	err = ::AEInstallEventHandler(kAECoreSuite, kAECreateElement,
			NewAEEventHandlerProc(LModelDirector::CreateElementEventHandler),
			ae_CreateElement, false);
	ThrowIfOSErr_(err);
		
		// Generic accessor for Model Objects
		
	err = ::AEInstallObjectAccessor(typeWildCard, typeWildCard, 
				NewOSLAccessorProc(LModelDirector::ModelObjectAccessor),
				0, false);
	ThrowIfOSErr_(err);
	
		// Accessor for List of Model Objects
		
	err = ::AEInstallObjectAccessor(typeWildCard, typeAEList, 
				NewOSLAccessorProc(LModelDirector::ModelObjectListAccessor),
				0, false);
	ThrowIfOSErr_(err);
		
	err = ::AESetObjectCallbacks(
				NewOSLCompareProc(LModelDirector::OSLCompareObjects),
				NewOSLCountProc(LModelDirector::OSLCountObjects),
				NewOSLDisposeTokenProc(LModelDirector::OSLDisposeToken),
				nil,		// GetMarkToken
				nil,		// Mark
				nil,		// AdjustMarks
				nil);		// GetErrDesc
	ThrowIfOSErr_(err);
}


// ---------------------------------------------------------------------------
//		¥ HandleAppleEvent
// ---------------------------------------------------------------------------
//	Respond to an AppleEvent
//
//	This generic handler directs the AppleEvent to the ModelObject
//	specified by the keyDirectObject parameter. If the keyDirectObject
//	parmater does not exist or is not an object specifier, the default
//	model handles the AppleEvent.
//
//	You must install a separate handler for AppleEvents where the
//	keyDirectObject parameter has a different meaning.

void
LModelDirector::HandleAppleEvent(
	const AppleEvent	&inAppleEvent,
	AppleEvent			&outReply,
	Int32				inRefCon)
{
	StAEDescriptor	theResult;
	OSErr			err;
	
									// Get optional direct object parameter
	StAEDescriptor	directObj,
					theToken;
	directObj.GetOptionalParamDesc(inAppleEvent, keyDirectObject,
									typeWildCard);
	
	err = Resolve(directObj.mDesc, theToken.mDesc);
	
	if ((err == errAENotAnObjSpec) || (err == errAENoSuchObject)) {
	
			// Direct object is not an ospec, so let the default model
			// handle the AppleEvent

		(LModelObject::GetDefaultModel())->
			HandleAppleEventAll(inAppleEvent, outReply, theResult.mDesc, inRefCon);
		
	} else if (err == noErr) {
		ProcessTokenizedEvent(inAppleEvent, theToken.mDesc, theResult.mDesc,
								outReply, inRefCon);
	} else {
		Throw_(err);
	}
	
									// Put result code(s) in Reply
									// If outReply has a null descriptor,
									//   it means that the sender does not
									//   want a reply.
	if ( (theResult.mDesc.descriptorType != typeNull) &&
		 (outReply.descriptorType != typeNull) ) {
		err = ::AEPutParamDesc(&outReply, keyAEResult, &theResult.mDesc);
		ThrowIfOSErr_(err);
	}
}


// ---------------------------------------------------------------------------
//		¥ HandleOpenOrPrintEvent
// ---------------------------------------------------------------------------
//	Respond to an Open or Print AppleEvent
//
//	Open an Print AppleEvents require a special handler because the
//	keyDirectObject parameter can specify an object that does not yet
//	exist. This will happen, for example, when opening a file
//	which is not already open.

void
LModelDirector::HandleOpenOrPrintEvent(
	const AppleEvent	&inAppleEvent,
	AppleEvent			&outReply,
	Int32				inRefCon)
{
	OSErr			err;
	StAEDescriptor	theResult,
					directObjToken,
					directObj;
	
									// Get direct object parameter
	directObj.GetParamDesc(inAppleEvent, keyDirectObject, typeWildCard);
	
	if (directObj.mDesc.descriptorType != typeNull)
		err = Resolve(directObj.mDesc, directObjToken.mDesc);
	
	if ( (directObj.mDesc.descriptorType == typeNull) ||
		 (err == errAENotAnObjSpec) ||
		 (err == errAENoSuchObject) ) {
									// Direct object parameter not present
									//   or is not an object specifier.
									//   Let the default model handle it.
		(LModelObject::GetDefaultModel())->
			HandleAppleEventAll(inAppleEvent, outReply, theResult.mDesc, inRefCon);
		
	} else if (err == noErr) {	
									// Process this event using the Token
									//   representing the direct direct
		ProcessTokenizedEvent(inAppleEvent, directObjToken.mDesc, theResult.mDesc,
								outReply, inRefCon);
	} else {
		Throw_(err);
	}
									// Put result code in Reply
	if (theResult.mDesc.descriptorType != typeNull) {
		err = ::AEPutParamDesc(&outReply, keyAEResult, &theResult.mDesc);
		ThrowIfOSErr_(err);
	}
}


// ---------------------------------------------------------------------------
//		¥ HandleCreateElementEvent
// ---------------------------------------------------------------------------
//	Respond to a CreateElement AppleEvent
//
//	CreateElement requires a special handler because it does not have
//	a keyDirectObject parameter that specifies a target ModelObject which
//	should respond to the event. The target ModelObject for this event
//	is the SuperModel for the element to create.

void
LModelDirector::HandleCreateElementEvent(
	const AppleEvent	&inAppleEvent,
	AppleEvent			&outReply,
	Int32				/* inRefCon */)
{
	OSErr			err;
	StAEDescriptor	classD(inAppleEvent, keyAEObjectClass, typeType),
					insertD(inAppleEvent, keyAEInsertHere);
	DescType		elemClass;
	UExtractFromAEDesc::TheType(classD, elemClass);
	DescType		insertPosition = kAEReplace;	//	arbitrary
	LModelObject	*target = nil;
	LModelObject	*container = nil;
	LModelObject	*element = nil;
							
	switch (insertD.mDesc.descriptorType) {

		case typeInsertionLoc:
		{
			StAEDescriptor	positionD(insertD, keyAEPosition, typeEnumeration);
			UExtractFromAEDesc::TheEnum(positionD, insertPosition);
			StAEDescriptor	objectD(insertD, keyAEObject, typeObjectSpecifier),
							tokenD;
			err = Resolve(objectD, tokenD.mDesc);
			ThrowIfOSErr_(err);
			target = LModelObject::GetModelFromToken(tokenD);
			target = target->GetInsertionTarget(insertPosition);
			container = target->GetInsertionContainer(insertPosition);
			break;
		}

		case typeNull:
			// The "insert here" parameter is supposed be required. However,
			// it seems reasonable to let it be optional and use the
			// beginning of the Application as the default location.
			container = LModelObject::GetDefaultModel();	
			insertPosition = kAEBeginning;	//	arbitrary?
			break;
			
		default:
		case typeObjectSpecifier:
		{
			//	The AppleEvent was coded and resolved (using lazy objects)
			//	in such a way that the target IS the element to be created.
			//	Its container should recognize this so let its container
			//	finish the creation (setting the parameters).
			StAEDescriptor	tokenD;
			err = Resolve(insertD, tokenD.mDesc);
			ThrowIfOSErr_(err);
			target = LModelObject::GetModelFromToken(tokenD);
			target->GetInsertionTarget(typeNull);			
			container = target->GetInsertionContainer(typeNull);
			if (container == nil)
				container = LModelObject::GetDefaultModel();
			insertPosition = kAEReplace;	//	not so arbitrary
			break;
		}
	}

	ThrowIfNULL_(container);
	element = container->HandleCreateElementEventAll(
					elemClass, insertPosition, target, inAppleEvent, outReply);

	// The AppleEvents Registry specifies that an object specifier
	// for the new element be returned as the keyAEResult parameter
	// in the Reply to the AppleEvent
	if (element != nil) {
		StAEDescriptor	elementDesc;
		element->MakeSpecifier(elementDesc.mDesc);
		UAEDesc::AddKeyDesc(&outReply, keyAEResult, elementDesc.mDesc);
	}
}


void
LModelDirector::ProcessTokenizedEvent(
	const AppleEvent	&inAppleEvent,
	AEDesc				&inDirectObjToken,
	AEDesc				&outResult,
	AppleEvent			&outReply,
	Int32				inRefCon)
{
	LModelObject	*theResponder;

	if (inDirectObjToken.descriptorType != typeAEList) {
	
		theResponder = LModelObject::GetModelFromToken(inDirectObjToken);
		theResponder->HandleAppleEventAll(inAppleEvent, outReply, outResult, inRefCon);
		
	} else {

		// Direct object is a list of ModelObjects
		StAEDescriptor	insertD;
		
		insertD.GetOptionalParamDesc(inAppleEvent, keyAEInsertHere, typeWildCard);
		
		if (insertD.mDesc.descriptorType == typeNull) {
			OSErr		err = noErr;
			Int32		count,
						i;
	
			if (AECountItems(&inDirectObjToken, &count))
				count = 0;
			
			//	process model objects
			for (i = 1; i <= count; i++) {
				StAEDescriptor	token,
								resultDesc;
				DescType		bogusKeyword;
				err = AEGetNthDesc(&inDirectObjToken, i, typeWildCard, &bogusKeyword, &token.mDesc);
				ThrowIfOSErr_(err);
				theResponder = LModelObject::GetModelFromToken(token);
				theResponder->HandleAppleEventAll(inAppleEvent, outReply, resultDesc.mDesc, inRefCon);
				UAEDesc::AddDesc(&outResult, 0, resultDesc);
			}
		} else {

			//	We are going to have to process token by token AND adjust keyAEInsertHere
			//	as we go.
			OSErr	err = noErr;
			Int32	count,
					i;
								
			if (AECountItems(&inDirectObjToken, &count))
				count = 0;
			
			
			StAEDescriptor		event;
			
			err = AEDuplicateDesc(&inAppleEvent, &event.mDesc);
			ThrowIfOSErr_(err);
			
			//	process model objects
			for (i = 1; i <= count; i++) {
				StAEDescriptor	event,
								token,
								resultDesc;
				DescType		bogusKeyword;

				err = AEGetNthDesc(&inDirectObjToken, i, typeWildCard, &bogusKeyword, &token.mDesc);
				ThrowIfOSErr_(err);
				theResponder = LModelObject::GetModelFromToken(token);
				
				if (i != 1) {
					StAEDescriptor	hereD;
					UAEDesc::MakeInsertionLoc(insertD.mDesc, kAEAfter, &hereD.mDesc);
					err = AEPutKeyDesc(&event.mDesc, keyAEInsertHere, &hereD.mDesc);
					ThrowIfOSErr_(err);
				}

				theResponder->HandleAppleEventAll(event, outReply, resultDesc.mDesc, inRefCon);
				UAEDesc::AddDesc(&outResult, 0, resultDesc);
				
				err = AEDisposeDesc(&insertD.mDesc);
				err = AEDuplicateDesc(&resultDesc.mDesc, &insertD.mDesc);
				ThrowIfOSErr_(err);
			}			
		}
	}
}


// ---------------------------------------------------------------------------
//		¥ AccessModelObject
// ---------------------------------------------------------------------------
//	Get Token for ModelObject
//
//	Part of the OSL object resolution process. Given a Token for a
//	Container and a key description of some objects, make a Token
//	for those objects.

void
LModelDirector::AccessModelObject(
	const DescType	inDesiredClass,
	const AEDesc	&inContainerToken,
	const DescType	/* inContainerClass */,
	const DescType	inKeyForm,
	const AEDesc	&inKeyData,
	AEDesc			&outToken,
	Int32			/* inRefCon */)
{
	LModelObject	*theContainer = nil;

	//	The OSL normally does this.  But... it doesn't appear to in
	//	in the case of formWhose or formTest.  Since GetModelToken
	//	can "add" to existing AEDesc's, this is required.
	outToken.descriptorType = typeNull;
	outToken.dataHandle = NULL;
	
	theContainer = LModelObject::GetModelFromToken(inContainerToken);
	Try_ {
		theContainer->GetModelToken(inDesiredClass, inKeyForm, inKeyData,
										outToken);
	} Catch_ (inErr) {
		DisposeToken(outToken);		//	In case the OSL doesn't do it
		Throw_(inErr);
	} EndCatch_;
}


void
LModelDirector::AccessModelObjectList(
	const DescType	inDesiredClass,
	const AEDesc	&inContainerToken,
	const DescType	/* inContainerClass */,
	const DescType	inKeyForm,
	const AEDesc	&inKeyData,
	AEDesc			&outToken,
	Int32			/* inRefCon */)
{
	Int32		itemCount,
				i;
	OSErr		err = noErr;

	outToken.descriptorType = typeNull;
	outToken.dataHandle = NULL;
	
	if (::AECountItems(&inContainerToken, &itemCount)) {
		itemCount = 0;
	}
	
	Try_ {
		for (i = 1; i <= itemCount; i++) {
			StAEDescriptor	containerDesc,
							subModelDesc;
			LModelObject	*container = nil;
			DescType		bogusKeyword;
			
			err = ::AEGetNthDesc(&inContainerToken, i, typeWildCard,
						&bogusKeyword, &containerDesc.mDesc);
			ThrowIfOSErr_(err);

			container = LModelObject::GetModelFromToken(containerDesc.mDesc);
			container->GetModelToken(inDesiredClass, inKeyForm, inKeyData,
						subModelDesc.mDesc);
		
			if (subModelDesc.mDesc.descriptorType == typeAEList) {
				Int32		subCount;
				if (::AECountItems(&subModelDesc.mDesc, &subCount)) {
					subCount = 0;
				}
				
				for (Int32 j = 1; j <= subCount; j++) {
					StAEDescriptor	subItem;
	
					err = ::AEGetNthDesc(&subModelDesc.mDesc, j, typeWildCard,
								&bogusKeyword, &subItem.mDesc);
					ThrowIfOSErr_(err);
					UAEDesc::AddDesc(&outToken, 0, subItem.mDesc);
				}
			} else {
				UAEDesc::AddDesc(&outToken, 0, subModelDesc.mDesc);
			}
			
			ThrowIfOSErr_(err);
		}
	} Catch_(inErr) {
		DisposeToken(outToken);		//	In case the OSL doesn't do it
		Throw_(inErr);
	} EndCatch_;
}


// ---------------------------------------------------------------------------
//		¥ DisposeToken
// ---------------------------------------------------------------------------
//	A Token is no longer needed.
//
//	Will not propogate errors.

void
LModelDirector::DisposeToken(
	AEDesc		&inToken)
{
	::AEDisposeDesc(&inToken);
}


// ---------------------------------------------------------------------------
//		¥ CompareObjects
// ---------------------------------------------------------------------------
//	Pass back the result of comparing one object with another

void
LModelDirector::CompareObjects(
	DescType		inComparisonOperator,
	const AEDesc	&inBaseObject,
	const AEDesc	&inCompareObjectOrDesc,
	Boolean			&outResult)
{
		// Base Object compares itself to the other object
	
	LModelObject	*theModel = LModelObject::GetModelFromToken(inBaseObject);
	
	if (inCompareObjectOrDesc.descriptorType == type_ModelToken) {
								// Other "object" is another ModelObject
		outResult = theModel->CompareToModel(inComparisonOperator,
					LModelObject::GetModelFromToken(inCompareObjectOrDesc));
								
	} else {					// Other "object" is a Descriptor
		outResult = theModel->CompareToDescriptor(inComparisonOperator,
								inCompareObjectOrDesc);
	}
}


// ---------------------------------------------------------------------------
//		¥ CountObjects
// ---------------------------------------------------------------------------
//	Pass back the number of objects of the desiredClass that are in the
//	specified container object

void
LModelDirector::CountObjects(
	DescType		inDesiredClass,
	DescType		/* inContainerClass */,
	const AEDesc	&inContainer,
	Int32			&outCount)
{
	LModelObject	*theModel = LModelObject::GetModelFromToken(inContainer);
	outCount = theModel->CountSubModels(inDesiredClass);
}


// ===========================================================================
// ¥ Static Functions									   Static Functions ¥
// ===========================================================================

// ---------------------------------------------------------------------------
//		¥ Resolved
// ---------------------------------------------------------------------------
//	Wrapper for AEResolve.  Takes into consideration mResolveFlags and, if the
//	inSpec is typeType pSelection, will fabricate a selection property ospec
//	with a null wrapper.  (AppleScript has an annoying habit of not recreating
//	AE's exactly as they were sent.)

OSErr
LModelDirector::Resolve(
	const AEDesc	&inSpec,
	AEDesc			&outToken)
{
	OSErr		err = noErr;
	DescType	theType;
	Boolean		fabricate = false;
	
	if (inSpec.descriptorType == typeType) {
		UExtractFromAEDesc::TheType(inSpec, theType);
		if (theType == pSelection)
			fabricate = true;
	}
	
	if (fabricate) {
		StAEDescriptor	nullSpec,
						propDesc(typeType, &theType, sizeof(theType)),
						fabSpec;
						
		err = ::CreateObjSpecifier(cProperty, nullSpec, formPropertyID,
											propDesc, false, &fabSpec.mDesc);
		if (err == noErr)
			err = ::AEResolve(&fabSpec.mDesc, sModelDirector->mResolveFlags, &outToken);
	} else {
		err = ::AEResolve(&inSpec, sModelDirector->mResolveFlags, &outToken);
	}
	
	return err;
}


// ===========================================================================
// ¥ Static CallBack Functions					   Static CallBack Functions ¥
// ===========================================================================

// ---------------------------------------------------------------------------
//		¥ AppleEventHandler
// ---------------------------------------------------------------------------
//	Generic CallBack function for all AppleEvents

pascal OSErr
LModelDirector::AppleEventHandler(
	const AppleEvent	*inAppleEvent,
	AppleEvent			*outAEReply,
	Int32				inRefCon)
{
	OSErr	err = noErr;
	
	try {
		sModelDirector->HandleAppleEvent(*inAppleEvent, *outAEReply, inRefCon);
	}
	
	catch (ExceptionCode inErr) {
		err = inErr;
	}
	
	catch (...) {
		SignalPStr_("\pUnknown Exception caught");
	}

	LModelObject::FinalizeLazies();

	return err;
}


// ---------------------------------------------------------------------------
//		¥ OpenOrPrintEventHandler
// ---------------------------------------------------------------------------
//	CallBack function for Open or Print AppleEvents
	
pascal OSErr
LModelDirector::OpenOrPrintEventHandler(
	const AppleEvent	*inAppleEvent,
	AppleEvent			*outAEReply,
	Int32				inRefCon)
{
	OSErr	err = noErr;
	
	try {
		sModelDirector->HandleOpenOrPrintEvent(*inAppleEvent, *outAEReply,
							inRefCon);
	}
	
	catch (ExceptionCode inErr) {
		err = inErr;
	}
	
	catch (...) {
		SignalPStr_("\pUnknown Exception caught");
	}

	LModelObject::FinalizeLazies();

	return err;
}


// ---------------------------------------------------------------------------
//		¥ CreateElementEventHandler
// ---------------------------------------------------------------------------
//	CallBack function for CreateElement AppleEvent
	
pascal OSErr
LModelDirector::CreateElementEventHandler(
	const AppleEvent	*inAppleEvent,
	AppleEvent			*outAEReply,
	Int32				inRefCon)
{
	OSErr	err = noErr;
	
	try {
		sModelDirector->HandleCreateElementEvent(*inAppleEvent, *outAEReply,
							inRefCon);
	}
	
	catch (ExceptionCode inErr) {
		err = inErr;
	}
	
	catch (...) {
		SignalPStr_("\pUnknown Exception caught");
	}

	LModelObject::FinalizeLazies();

	return err;
}


pascal OSErr
LModelDirector::ModelObjectAccessor(
	DescType		inDesiredClass,
	const AEDesc	*inContainerToken,
	DescType		inContainerClass,
	DescType		inKeyForm,
	const AEDesc	*inKeyData,
	AEDesc*			outToken,
	Int32			inRefCon)
{
	OSErr	err = noErr;
	
	try {
		sModelDirector->AccessModelObject(inDesiredClass,
									*inContainerToken, inContainerClass,
									inKeyForm, *inKeyData,
									*outToken, inRefCon);
	}
	
	catch (ExceptionCode inErr) {
		err = inErr;
	}
	
	catch (...) {
		SignalPStr_("\pUnknown Exception caught");
	}
	
	return err;
}


pascal OSErr
LModelDirector::ModelObjectListAccessor(
	DescType		inDesiredClass,
	const AEDesc	*inContainerToken,
	DescType		inContainerClass,
	DescType		inKeyForm,
	const AEDesc	*inKeyData,
	AEDesc*			outToken,
	Int32			inRefCon)
{
	OSErr	err = noErr;
	
	try {
		sModelDirector->AccessModelObjectList(inDesiredClass,
									*inContainerToken, inContainerClass,
									inKeyForm, *inKeyData,
									*outToken, inRefCon);
	}
	
	catch (ExceptionCode inErr) {
		err = inErr;
	}
	
	catch (...) {
		SignalPStr_("\pUnknown Exception caught");
	}
	
	return err;
}


pascal OSErr
LModelDirector::OSLDisposeToken(
	AEDesc		*inToken)
{
	OSErr	err = noErr;
	
	try {
		sModelDirector->DisposeToken(*inToken);
	}
	
	catch (ExceptionCode inErr) {
		err = inErr;
	}
	
	catch (...) {
		SignalPStr_("\pUnknown Exception caught");
	}
	
	return err;
}


pascal OSErr
LModelDirector::OSLCompareObjects(
	DescType		inComparisonOperator,
	const AEDesc	&inBaseObject,
	const AEDesc	&inCompareObjectOrDesc,
	Boolean			&outResult)
{
	OSErr	err = noErr;
	
	try {
		sModelDirector->CompareObjects(inComparisonOperator, inBaseObject,
									inCompareObjectOrDesc, outResult);
	}
	
	catch (ExceptionCode inErr) {
		err = inErr;
	}
	
	catch (...) {
		SignalPStr_("\pUnknown Exception caught");
	}
	
	return err;
}


pascal OSErr
LModelDirector::OSLCountObjects(
	DescType		inDesiredClass,
	DescType		inContainerClass,
	const AEDesc	&inContainer,
	Int32			&outCount)
{
	OSErr	err = noErr;
	
	try {
		sModelDirector->CountObjects(inDesiredClass, inContainerClass,
									inContainer, outCount);
	}
	
	catch (ExceptionCode inErr) {
		err = inErr;
	}
	
	catch (...) {
		SignalPStr_("\pUnknown Exception caught");
	}
	
	return err;
}
