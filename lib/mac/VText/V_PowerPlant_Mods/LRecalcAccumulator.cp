//	===========================================================================
//	LRecalcAccumulator.cp			©1995 Metrowerks Inc. All rights reserved.
//	===========================================================================

#ifdef PowerPlant_PCH
#include PowerPlant_PCH
#endif

#include	"LRecalcAccumulator.h"

LRecalcAccumulator::LRecalcAccumulator()
{
	mFlags = 0;
	mUpdateLevel = 0;
}

LRecalcAccumulator::~LRecalcAccumulator()
{
}

void	LRecalcAccumulator::RecalcSelf(void)
{
//	Override to process individual flags.  Something like...
/*
	if (mFlags & someFlag) {
		...						//	processing for someFlag
		mFlags &= ~someFlag;	//	clear the flag (ie. processed)
	}
	
	inherited::RecalcSelf();
*/
}

void	LRecalcAccumulator::Recalc(void)
{
	RecalcFlagsT	oldFlags;
	
	oldFlags = mFlags;
	if (mFlags)
		RecalcSelf();

	Assert_(mFlags == 0);
	mFlags = 0;
	
//	If your flags are cyclical or interrelated you could try
//	overriding with something like the following.
//	
//	Note the infinite loop note!!!
/*	
	while (mFlags) {

		oldFlags = mFlags;
		RecalcSelf();

		//	It is very important we don't get stuck in an infinite
		//	or cyclic recalculation loop.
		//
		//	This particular case is obvious but you may need to augment
		//	it with application specific considerations.  You could
		//	also augment it with a check for a user abort (cmd-period).
		//
		if (mFlags == oldFlags)
			break;
			
		//	If we processed all the flags, there is no reason
		//	to continue (and spend a SystemTask).
		//
		if (mFlags == 0)
			break;

		SystemTask();	//	Be cooperatively friendly
	}
	Assert_((mFlags == 0) || (mFlags == recalc_WindowUpdate));
	mFlags = 0;
*/	
}

void	LRecalcAccumulator::Refresh(RecalcFlagsT inFlags)
{
	mFlags |= inFlags;
}

//	===========================================================================

Int32	LRecalcAccumulator::GetNestingLevel(void)
{
	return mUpdateLevel;
}

void	LRecalcAccumulator::SetNestingLevel(Int32 inLevel)
{
	mUpdateLevel = inLevel;
}

void	LRecalcAccumulator::NestedUpdateIn(void)
/*
	OVERRIDE
*/
{
/*	something like...

	Assert_(mUpdateLevel > 0);
	
	if (mUpdateLevel == 1) {
	
		//	record necessary state info etc.
		...
	}
*/
}

void	LRecalcAccumulator::NestedUpdateOut(void)
/*
	OVERRIDE
*/
{
/*	something like...
	
	Assert_(mUpdateLevel > 0);
	
	if (mUpdateLevel == 1) {
		Recalc();
		
		//	app specific updating.
		...
	}
*/
}

StRecalculator::StRecalculator(LRecalcAccumulator *inAccumulator)
{
	mAccumulator = inAccumulator;
	
	if (mAccumulator) {
		mAccumulator->SetNestingLevel(mAccumulator->GetNestingLevel() +1);

		mAccumulator->NestedUpdateIn();
	}
}

StRecalculator::~StRecalculator()
{
	if (mAccumulator) {

		//	Don't throw out of a destructor
		Try_ {
			mAccumulator->NestedUpdateOut();
		} Catch_(inErr) {
			Assert_(false);
		} EndCatch_;		

		mAccumulator->SetNestingLevel(mAccumulator->GetNestingLevel() -1);

		Assert_(mAccumulator->GetNestingLevel() >= 0);
	}
}
