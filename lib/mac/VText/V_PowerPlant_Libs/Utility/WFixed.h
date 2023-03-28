//	===========================================================================
//	WFixed
//
//	Copyright 1996 ViviStar Consulting.
//	===========================================================================

#pragma	once

#include	<PP_Prefix.h>

#include	<FixMath.h>

typedef struct FixedT {
public:

/*	If Fixed wasn't typedef'd as long, we could unambiguously have...

	inline				FixedT(Fixed inValue)					{mValue = inValue;}
	inline	operator	Fixed*()								{return &mValue;}
	inline	operator	Fixed&()								{return mValue;}
	inline	operator	const Fixed*() const					{return &mValue;}
	inline	operator	const Fixed&() const					{return mValue;}
	inline	operator	Fixed()	const							{return mValue;}
	
	and...
	
	inline				FixedT(long inValue)					{mValue = ((Fixed) inValue) << 16;}
	inline	operator	long()	const							{return mValue >> 16;}
	
	But, it is.  So, to avoid ambiguity, the above are NOT defined and the
	SetFrom... and As... accessors in conjunection with WFixed must be used.
*/
	inline	FixedT&		SetFromLong(long inValue)				{mValue = (Fixed)(inValue << 16); return *this;}
	inline	long		ToLong(void) const						{return mValue >> 16;}
	
	inline	FixedT&		SetFromFixed(Fixed inValue)				{mValue = inValue; return *this;}
	inline	Fixed		ToFixed(void) const						{return mValue;}
	
	inline				FixedT()								{}
	inline				FixedT(Int16 inValue)					{mValue = ((long) inValue) << 16;}
	inline				FixedT(double inValue)					{mValue = X2Fix(inValue);}
	
	inline	operator	Int16()	const							{return (mValue >> 16);}
	inline	operator	double() const							{return Fix2X(mValue);}

	inline	FixedT&		operator=(const FixedT &inRhs)			{mValue = inRhs.mValue; return *this;}

	inline	FixedT		operator+(const FixedT &inRhs) const	{
																	FixedT	rval;
																	rval.mValue = mValue + inRhs.mValue;
																	return rval;
																}
	inline	FixedT		operator-(const FixedT &inRhs) const	{
																	FixedT	rval;
																	rval.mValue = mValue - inRhs.mValue;
																	return rval;
																}
	inline	FixedT		operator*(const FixedT &inRhs) const	{
																	FixedT	rval;
																	rval.mValue = FixMul(mValue, inRhs.mValue);
																	return rval;
																}
	inline	FixedT		operator/(const FixedT &inRhs) const	{
																	FixedT	rval;
																	rval.mValue = FixDiv(mValue, inRhs.mValue);
																	return rval;
																}
	inline	FixedT		operator%(const FixedT &inRhs) const	{
																	FixedT	rval;
																	Assert_(false);	//	following doesn't actually work
																	rval.mValue = mValue % inRhs.mValue;
																	return rval;
																}

	inline	FixedT&		operator+=(const FixedT &inRhs)			{mValue += inRhs.mValue; return *this;}
	inline	FixedT&		operator-=(const FixedT &inRhs)			{mValue -= inRhs.mValue; return *this;}
	inline	FixedT&		operator*=(const FixedT &inRhs)			{mValue *= inRhs.mValue; return *this;}
	inline	FixedT&		operator/=(const FixedT &inRhs)			{mValue /= inRhs.mValue; return *this;}
	inline	FixedT&		operator%=(const FixedT &inRhs)			{mValue %= inRhs.mValue; return *this;}
	
	inline	Boolean		operator==(const FixedT &inRhs) const	{return	mValue == inRhs.mValue;}	
	inline	Boolean		operator!=(const FixedT &inRhs) const	{return	mValue != inRhs.mValue;}	
	inline	Boolean		operator<(const FixedT &inRhs) const	{return	mValue < inRhs.mValue;}	
	inline	Boolean		operator>(const FixedT &inRhs) const	{return	mValue > inRhs.mValue;}	
	inline	Boolean		operator<=(const FixedT &inRhs) const	{return	mValue <= inRhs.mValue;}	
	inline	Boolean		operator>=(const FixedT &inRhs) const	{return	mValue >= inRhs.mValue;}
	
	FixedT &			floor(void);
	FixedT &			ceil(void);
	FixedT &			round(void);
	
public:	
	const static FixedT	sMin;
	const static FixedT	sMinusOne;
	const static FixedT	sZero;
	const static FixedT	sOne;
	const static FixedT	sMax;

protected:
	Fixed	mValue;
} FixedT;

class WFixed {
public:
	static	inline	FixedT		Fix2FixedT(Fixed inValue)		{FixedT	rval; return rval.SetFromFixed(inValue);}
	static	inline	FixedT		Long2FixedT(Int32 inValue)		{FixedT rval; return rval.SetFromLong(inValue);}
};
