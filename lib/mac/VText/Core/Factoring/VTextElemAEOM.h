//	===========================================================================
//	VTextElemAEOM.h	
//	===========================================================================

#pragma once

#include	<LTextElemAEOM.h>

//	===========================================================================

class	VTextElemAEOM
			:	public LTextElemAEOM
{
//private:
//	typedef	LTextElemAEOM	inheritAEOM;
public:
				//	Maintenance
						VTextElemAEOM(
								const TextRangeT	&inRange,
								DescType			inKind,
								LModelObject		*inSuperModel,
								LTextModel			*inTextModel,
								LTextEngine			*inTextEngine
						);
	virtual				~VTextElemAEOM();
	
	virtual	void		GetImportantAEProperties(AERecord &outRecord) const;
	virtual LModelObject*
						HandleCreateElementEvent(
							DescType			inElemClass,
							DescType			inInsertPosition,
							LModelObject*		inTargetObject,
							const AppleEvent	&inAppleEvent,
							AppleEvent			&outAEReply);						
	virtual void		HandleAppleEvent(
							const AppleEvent	&inAppleEvent,
							AppleEvent			&outAEReply,
							AEDesc				&outResult,
							long				inAENumber);
	virtual void		HandleModify(
							DescType			inProperty,
							DescType			inOperation,
							const LAESubDesc	&inValue,
							const AppleEvent	&inAppleEvent,
							AppleEvent			&outAEReply,
							AEDesc				&outResult);
	virtual void		GetAEProperty(DescType	inProperty,
							const AEDesc		&inRequestedType,
							AEDesc				&outPropertyDesc) const;
	virtual void		SetAEProperty(DescType	inProperty,
							const AEDesc		&inValue,
							AEDesc&				outAEReply);

};