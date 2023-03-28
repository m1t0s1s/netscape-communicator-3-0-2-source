//	===========================================================================
//	LRecalcAccumulator.h			©1995 Metrowerks Inc. All rights reserved.
//	===========================================================================

#pragma	once

#include	<PP_Prefix.h>

typedef	Int32	RecalcFlagsT;

class	LRecalcAccumulator
{
public:
	LRecalcAccumulator();
	~LRecalcAccumulator();

	virtual	void	Refresh(RecalcFlagsT inFlags);
	virtual	void	Recalc(void);
	
	virtual void	NestedUpdateIn(void);
	virtual void	NestedUpdateOut(void);
	virtual Int32	GetNestingLevel(void);
	virtual void	SetNestingLevel(Int32 inLevel);

protected:
	virtual	void	RecalcSelf(void);	//	override
	RecalcFlagsT	mFlags;
	Int32			mUpdateLevel;
};

class StRecalculator
{
public:
						StRecalculator(LRecalcAccumulator *inAccumulator);
	virtual				~StRecalculator();
	
	LRecalcAccumulator	*mAccumulator;
};