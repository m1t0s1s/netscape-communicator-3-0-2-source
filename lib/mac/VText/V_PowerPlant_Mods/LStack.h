// ===========================================================================
//	LStack.h						©1995 Metrowerks Inc. All rights reserved.
// ===========================================================================
//
//	A stack of fixed-size items.

#pragma	once

#include	<LArray.h>

template <class T>
class	LStack
			: public LArray
{
public:
					LStack(Boolean inKeepStackLocked = true);
					~LStack();
	
	inline	Int32	Depth(void) const				{return GetCount();}

	virtual	void	Push(const T &inItem);
	virtual void	Push(void);
	virtual void	Pop(void);

	virtual T&		Top(void) const;
	virtual T&		PeekBack(Int32 inCount) const;
};
