// ===========================================================================
//	LSharable.h						©1995 Metrowerks Inc. All rights reserved.
// ===========================================================================

#pragma once

#include <PP_Prefix.h>


class	LSharable {
public:
					LSharable();
	
	virtual void	AddUser(
						void	*inUser);
	virtual void	RemoveUser(
						void	*inUser);

	virtual Int32	GetUseCount();

protected:
	Int32			mUseCount;
	
	virtual			~LSharable();		// protected destructor prevents
										//   stack-based objects
	virtual void	NoMoreUsers();
};


class	StSharer {
public:
					StSharer(
						LSharable	*inSharable);
					~StSharer();
					
	void			SetSharable(
						LSharable	*inSharable);
protected:
	LSharable		*mSharable;
};


#define	ReplaceSharable_(ref,obj,user)			\
	do {										\
		LSharable	*__old = (ref);				\
		(ref) = (obj);							\
		if (ref)								\
			(ref)->AddUser(user);				\
		if (__old)								\
			__old->RemoveUser(user);			\
	} while (false)
	
template	<class Sharable>
void	ReplaceSharable(
	Sharable*	&inReference,
	Sharable*	inNewValue,
	void		*inUser
);

template	<class Sharable>
void	ReplaceSharable(
	Sharable*	&inReference,
	Sharable*	inNewValue,
	void		*inUser)
{
	if (inReference != inNewValue) {
		if (inReference)
			inReference->RemoveUser(inUser);
		inReference = inNewValue;
		if (inReference)
			inReference->AddUser(inUser);
	}
}