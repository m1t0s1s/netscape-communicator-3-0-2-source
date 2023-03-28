//	===========================================================================
//	WFixed
//
//	Copyright 1996 ViviStar Consulting.
//	===========================================================================

#include	<WFixed.h>

#ifdef PowerPlant_PCH
#include PowerPlant_PCH
#endif

const FixedT	FixedT::sMin =		WFixed::Fix2FixedT(0x80000000);
const FixedT	FixedT::sMinusOne =	WFixed::Long2FixedT(-1);
const FixedT	FixedT::sZero =		WFixed::Fix2FixedT(0x00000000);
const FixedT	FixedT::sOne =		WFixed::Long2FixedT(1);
const FixedT	FixedT::sMax =		WFixed::Fix2FixedT(0x7fffffff);

FixedT &	FixedT::floor(void)
{
	Assert_(false);	//	implement me
	return *this;
}

FixedT &	FixedT::ceil(void)
{
	Assert_(false);	//	implement me
	return *this;
}

FixedT &	FixedT::round(void)
{
	Assert_(false);	//	implement me
	return *this;
}

