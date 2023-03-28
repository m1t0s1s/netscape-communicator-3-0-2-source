//	===========================================================================
//	VEditField
//
//	Copyright 1996 ViviStar Consulting.
//	===========================================================================

#pragma once

#include <LEditField.h>
#include	<VText_ClassIDs.h>

class VTSMProxy;

class VEditField : public LEditField
{
public:
						VEditField(LStream* inStream);
	enum {class_ID = kVTCID_VEditField};

				//	implementation
protected:

	virtual	void		BeTarget(void);
	virtual void		DontBeTarget(void);
	
	VTSMProxy*			mProxy;
};