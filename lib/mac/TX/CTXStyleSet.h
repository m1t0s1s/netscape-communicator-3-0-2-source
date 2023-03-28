// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	CTXStyleSet.h						  ©1995 Netscape. All rights reserved.
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

#pragma once

#include	<LStyleSet.h>
#include	"QDTextRun.h"

#define	kPPStyleSet	'PPSS'

#ifdef	INCLUDE_TX_TSM
	#include	<TSMRun.h>

	class	CTXStyleSet
				:	public	LStyleSet
				,	public	CTSMRun

#else

	class	CTXStyleSet
				:	public	LStyleSet
				,	public	CQDTextRun

#endif

{
public:
	virtual void	AddUser(void *inClaimer);
	virtual	void	RemoveUser(void *inReleaser);
	virtual void	NoMoreUsers(void);
	
				//	Maintenance
						CTXStyleSet(const LStyleSet *inOriginal);
						CTXStyleSet();
	virtual				~CTXStyleSet();
	
	virtual ScriptCode	GetScript(void) const;

				//	AEOM
	virtual void		GetAEProperty(
								DescType		inProperty,
								const AEDesc	&inRequestedType,
								AEDesc			&outPropertyDesc) const;
	virtual void		SetAEProperty(
								DescType		inProperty,
								const AEDesc	&inValue,
								AEDesc			&outAEReply);

				//	CQDTextRun features
public:
	virtual	void	IStyleSetObject(void);
	virtual CAttrObject* CreateNew(void) const;
					
	virtual CAttrObject* Reference(void);
	virtual	void Free(void);
	virtual	long GetCountReferences(void);

	virtual	ClassId	GetClassId(void) const {return kPPStyleSet;};
	virtual	void	GetAttributes(CObjAttributes *attributes) const;
	virtual void	SetDefaults(long script = 0);
	virtual Boolean	EqualAttribute(AttrId attr, const void * valuePtr) const;
	virtual void	AttributeToBuffer(AttrId theAttr, void *attrBuffer) const;
	virtual	void	BufferToAttribute(AttrId theAttr, const void *attrBuffer);
	virtual void	SetKeyScript() const;
};