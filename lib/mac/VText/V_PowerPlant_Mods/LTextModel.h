//	===========================================================================
//	LTextModel.h					©1994 Metrowerks Inc. All rights reserved.
//	===========================================================================

#pragma once

#include	<LSelectionModelAEOM.h>
#include	<LListener.h>

#include	<LTextEngine.h>	//	Range type.
#include	<LDataTube.h>

class	LTextEngine;
class	LTextElemAEOM;
class	LTextSelection;

/*	---------------------------------------------------------------------------

	Version number for AEOM text processor could be very useful to know in OSA
	applications.
*/

	const DescType	pPPTextAEOMVersion = 'pptv';

	#define			PPTEXTAEOMVERSION	0x01006006

//	===========================================================================

class	LTextModel
			:	public	LSelectionModelAEOM
			,	public	LListener
{
private:
	typedef	LSelectionModelAEOM	inheritAEOM;

public:
	enum {class_ID = kPPCID_TextModel};

//-						LTextModel(LTextEngine *inTextEngine);
						LTextModel();
						~LTextModel();
	virtual ClassIDT	GetClassID() const {return class_ID;}
#ifndef	PP_No_ObjectStreaming
	virtual void		WriteObject(LStream &inStream) const;
	virtual void		ReadObject(LStream &inStream);
#endif

	virtual void		SetSelection(LSelection *inSelection);
	virtual	LTextElemAEOM *		GetTextLink(void) {return mTextLink;}
	virtual	const TextRangeT&	GetRange(void);
	virtual void		SetDefaultSubModel(LModelObject *inSubModel);
	virtual LTextEngine *	GetTextEngine(void);
	virtual void		SetTextEngine(LTextEngine *inTextEngine);

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

	virtual	LTextElemAEOM *	MakeNewElem(
								const TextRangeT	&inRange,
								DescType			inKind,
								const LModelObject	*inSuperModel = NULL) const;
protected:
	virtual	LTextElemAEOM *	MakeNewElemSelf(
								const TextRangeT	&inRange,
								DescType			inKind,
								const LModelObject	*inSuperModel) const;
public:

	virtual void		FixLink(void) const;
	virtual	LStyleSet *	StyleSetForScript(
							const TextRangeT	&inRange,
							ScriptCode			inScript);
	virtual void		ListenToMessage(MessageT inMessage, void *ioParam);

	virtual LModelObject*	GetInsertionTarget(DescType inInsertPosition) const;

public:
				//	AEOM support
	virtual LModelObject*
						GetModelProperty(DescType inProperty) const;
	virtual LModelObject*
						HandleCreateElementEvent(
									DescType			inElemClass,
									DescType			inInsertPosition,
									LModelObject*		inTargetObject,
									const AppleEvent	&inAppleEvent,
									AppleEvent			&outAEReply);
	virtual void		GetAEProperty(DescType		inProperty,
									const AEDesc	&inRequestedType,
									AEDesc			&outPropertyDesc) const;

	virtual void		SetAEProperty(
									DescType		inProperty,
									const AEDesc	&inValue,
									AEDesc&			outAEReply);

protected:
	LTextEngine			*mTextEngine;
	LTextElemAEOM		*mTextLink;		//	To avoid casts on mDefaultSubModel
	LTextSelection		*mTextSelection;
};

