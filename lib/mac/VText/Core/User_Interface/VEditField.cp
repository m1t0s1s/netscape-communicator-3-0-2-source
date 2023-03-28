//	===========================================================================
//	VEditField
//
//	Copyright 1996 ViviStar Consulting.
//	===========================================================================

#ifdef PowerPlant_PCH
#include PowerPlant_PCH
#endif

#include "VEditField.h"
#include <VTSMProxy.h>

/*
	Thanks to Doug Watkins for contributing this class
*/

VEditField::VEditField(LStream* inStream)
	:	LEditField(inStream)
{
	mProxy = NULL;

	Try_ {
		Int32 theResult;
		 ::Gestalt(gestaltTSMgrAttr, &theResult);
		
        OSErr theErr = ::Gestalt(gestaltTSMgrVersion, &theResult);
        ThrowIfOSErr_(theErr);

		if (theResult >= 1) {
			theErr = ::Gestalt(gestaltTSMTEAttr, &theResult);
	        ThrowIfOSErr_(theErr);
			
			if ((theResult >> gestaltTSMTEPresent) & 1)	
				mProxy = WTSMManager::MakeTSMTEProxy(mTextEditH);
		}
	} Catch_(inErr) {
		// Failure just means that this edit field won't support TSMTE
	} EndCatch_;
}

void VEditField::BeTarget(void)
{
	FocusDraw();
	
	inherited::BeTarget();
	
	if (mProxy)
		mProxy->Activate();
}

void VEditField::DontBeTarget(void)
{
	FocusDraw();
	
	if (mProxy)
		mProxy->Deactivate();

	inherited::DontBeTarget();
}

