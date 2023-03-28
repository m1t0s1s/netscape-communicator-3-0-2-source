//	===========================================================================
//	LSelectionModelAEOM.h			©1994 Metrowerks Inc. All rights reserved.
//	===========================================================================

#pragma once

#include	<LSharableModel.h>
#include	<LStreamable.h>
#include	<PP_ClassIDs_DM.h>

class	LSelection;
class	LAction;
class	LAEAction;

class	LSelectionModelAEOM
			:	public LSharableModel
			,	public LStreamable
{
private:
	typedef	LSharableModel	inheritAEOM;
	
public:
	enum {class_ID = kPPCID_SelectionModelAEOM};

						LSelectionModelAEOM(LModelObject *inSuperModel, DescType inKind);
						LSelectionModelAEOM();
						~LSelectionModelAEOM();
	virtual ClassIDT	GetClassID() const {return class_ID;}
#ifndef	PP_No_ObjectStreaming
	virtual void		WriteObject(LStream &inStream) const;
	virtual void		ReadObject(LStream &inStream);
#endif


	virtual	LSelection *	GetSelection(void);
	virtual void		SetSelection(LSelection *inSelection);

				//	AEOM
	virtual void		HandleAppleEvent(
								const AppleEvent	&inAppleEvent,
								AppleEvent			&outAEReply,
								AEDesc				&outResult,
								Int32				inAENumber);
	virtual LModelObject*
						GetModelProperty(DescType inProperty) const;
	virtual void		GetModelTokenSelf(
									DescType		inModelID,
									DescType		inKeyForm,
									const AEDesc	&inKeyData,
									AEDesc			&outToken) const;									
	virtual void		GetAEProperty(DescType		inProperty,
									const AEDesc	&inRequestedType,
									AEDesc			&outPropertyDesc) const;
	virtual void		SetAEProperty(DescType		inProperty,
									const AEDesc	&inValue,
									AEDesc&			outAEReply);

				//	Clipboard/selection/misc suite verb handling
	virtual void		HandleCut(AppleEvent &outAEReply);
	virtual void		HandleCopy(AppleEvent &outAEReply);
	virtual void		HandlePaste(AppleEvent &outAEReply);
	virtual	void		HandleSelect(
								const AppleEvent	&inAppleEvent,
								AppleEvent			&outAEReply,
								AEDesc				&outResult);
	
protected:
	LSelection		*mSelection;
};
