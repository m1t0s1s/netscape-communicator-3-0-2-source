//	===========================================================================
//	VTabAEOM.cp
//
//	Copyright 1996 ViviStar Consulting.
//	===========================================================================

#pragma	once

#include	<LModelObject.h>
#include	<WFixed.h>
#include	<VText_ClassIDs.h>

typedef	Int32 VTextTabOrientationE;	//	See AETextSuite.h

class	LRulerSet;

typedef struct TabRecord {
	FixedT					location;		//	from ruler begin
											//		signed with ruler direction
	FixedT					repeat;			//	every (value) units from location
											//		0 means non repeating
											//		only positive
											//		terminated by next following tab
	VTextTabOrientationE	orientation;
	Int16					delimiter;
} TabRecord;
	
class	VTabAEOM
			:	public LModelObject
{
public:
						VTabAEOM(
								LModelObject	*inSuperModel,
								LRulerSet		*inRuler,
								TabRecord		&inTabRecord);
	
	virtual TabRecord&	GetTabRecord(void) const;

	virtual	void		HandleDelete(
								AppleEvent&		outAEReply,
								AEDesc&			outResult);
	virtual void		GetAEProperty(
								DescType		inProperty,
								const AEDesc	&inRequestedType,
								AEDesc			&outPropertyDesc) const;
	virtual void		SetAEProperty(
								DescType		inProperty,
								const AEDesc	&inValue,
								AEDesc			&outAEReply);

protected:
	TabRecord			&mTabRecord;
	LRulerSet			*mRuler;
};