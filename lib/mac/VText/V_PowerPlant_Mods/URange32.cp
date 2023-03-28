//	===========================================================================
//	URange32.cp						©1994 Metrowerks Inc. All rights reserved.
//	===========================================================================

#ifdef PowerPlant_PCH
#include PowerPlant_PCH
#endif


#include	"URange32.h"

#pragma	warn_unusedarg off

const Range32T	Range32T::sAll(0, max_Int32);
const Range32T	Range32T::sUndefined;

#ifndef	Profile_Range32T
	#pragma	push
	#pragma	profile	off
#endif

//	----------------------------------------------------------------------

void	Range32T::SetLength(Int32 inLength)
{
	if (inLength < 0) {
		Throw_(paramErr);
	} else if (inLength == 0) {
		length = rangeCode_Before;
//		Assert_(false);	//	sure you wouldn't rather use the unambiguous SetLength(length, insertion)?
	} else {
		length = inLength;
	}
}

Range32T::Range32T(
	Int32		inStart,
	Int32		inLength,
	ERangeCode	inInsertion)	//	iff inLength == 0
{
	if (inLength) {
		start = inStart;
		length = inLength;
	} else {
		start = inStart;
		SetInsertion(inInsertion);
	}
}

void	Range32T::SetLength(
	Int32		inLength,
	ERangeCode	inInsertion)	//	iff inLength == 0
{
	if (inLength < 0) {
		Throw_(paramErr);
	} else if (inLength == 0) {
		SetInsertion(inInsertion);
	} else {
		length = inLength;
	}
}

void	Range32T::MoveStart(Int32 inStart)
{	
	Int32 end = End();
	start = inStart;
	SetEnd(end);
}

void	Range32T::SetInsertion(ERangeCode inCode)
{
	Assert_(inCode != 0);
	Assert_(inCode & 0x80000000);
	
	length = inCode;
}

void	Range32T::SetRange(
	Int32		inOneInsertion,
	Int32		inAnotherInsertion,
	ERangeCode	inCode)
{
	if (inOneInsertion < inAnotherInsertion) {
		start = inOneInsertion;
		SetEnd(inAnotherInsertion);
	} else if (inOneInsertion > inAnotherInsertion) {
		start = inAnotherInsertion;
		SetEnd(inOneInsertion);
	} else {
		start = inOneInsertion;
		SetInsertion(inCode);
	}
}

//	======================================================================

Range32T&	Range32T::Before(void)
{
	SetInsertion(rangeCode_Before);
	
	return *this;
}

Range32T&	Range32T::Front(void)
{
	if (Length())
		SetInsertion(rangeCode_After);
	
	return *this;
}

Range32T&	Range32T::Back(void)
{
	if (Length()) {
		SetStart(End());
		SetInsertion(rangeCode_Before);
	}
	
	return *this;
}

Range32T&	Range32T::After(void)
{
	start = start + Length();
	SetInsertion(rangeCode_After);
	
	return *this;
}

Range32T&	Range32T::Crop(const Range32T &inRange)
//	Crop "this" to inrange
{
	if (!inRange.Contains(*this)) {
		if (inRange.Length()) {
			if (End() <= inRange.Start()) {
				start = inRange.Start();
				SetInsertion(rangeCode_After);
			} else if (Start() >= inRange.End()) {
				start = inRange.End();
				SetInsertion(rangeCode_Before);
			} else {
				Int32	newStart = URange32::Max(Start(), inRange.Start()),
						newEnd = URange32::Min(End(), inRange.End());
				
				start = newStart;
				SetEnd(newEnd);
				
				Assert_(Length());
			}
		} else {
			*this = inRange;
		}
		
		Assert_(inRange.Contains(*this));
	}
	
	return *this;
}

Range32T&	Range32T::WeakCrop(const Range32T &inRange)
{
	if (!inRange.WeakContains(*this))
		Crop(inRange);
	
	return *this;
}

Range32T&	Range32T::Union(const Range32T &inRange)
/*
	Union isn't closed...
		Consider	rangeCode_Before | rangeCode_After
	
	Will default to rangeCode_Before
*/
{
	if (this != &inRange) {
	
		if (Undefined()) {
			*this = inRange;
		} else if (!inRange.Undefined()) {
			Int32	min = URange32::Min(	Start(), inRange.Start()	),
					max = URange32::Max(	End(), inRange.End()	);
			
			start = min;
			SetLength(max - min);
		}
	}

	return *this;
}

Range32T&	Range32T::Intersection(const Range32T &inRange)
{
	Int32	thisLength = Length(),
			thatLength = inRange.Length();
			
	if (*this == inRange) {
		//	don't change
	} else if ((thisLength == 0) && (thatLength == 0)) {
		SetInsertion(rangeCode_Undefined);
	} else if (thatLength == 0) {
		Range32T	range = inRange;
		range.Intersection(*this);
		*this = range;
	} else if (thisLength == 0) {
		if ((Start() == inRange.Start()) && (Insertion() == rangeCode_Before)) {
			SetInsertion(rangeCode_Undefined);
		} else if ((Start() == inRange.End()) && (Insertion() == rangeCode_After)) {
			SetInsertion(rangeCode_Undefined);
		} else if ((inRange.Start() <= Start()) && (Start() <= inRange.End())) {
			//	nada;
		} else {
			SetInsertion(rangeCode_Undefined);
		}
	} else {
		Int32	min = URange32::Max(Start(), inRange.Start()),
				max = URange32::Min(End(), inRange.End());
		
		if (min <= max) {
			start = min;
			SetLength(max - min);
		} else {
			length = rangeCode_Undefined;
		}
	}

	return *this;
}

Range32T&	Range32T::Insert(const Range32T &inRangePt, Int32 inAmount)
/*
	Find the range when inAmount is inserted at inRangePt.
	
	inRangePt.Length() must == 0.
	inAmount must >= 0.
*/
{
	if (inRangePt.Length() != 0)
		Throw_(paramErr);
	
	if (inAmount < 0)
		Throw_(paramErr);
	
	if (inAmount) {

		if (Contains(inRangePt)) {
			SetLength(Length() + inAmount);
		} else {
			if (inRangePt.Start() <= Start()) {
				start += inAmount;
			} else {
				Assert_(inRangePt.Start() >= End());
			}
		}

	}
	
	return *this;
}

Range32T&	Range32T::Delete(const Range32T &inRange)
/*
	Return the range when inRange is deleted.
*/
{
	if (inRange.Length()) {
		if (Length()) {
			if (Contains(inRange)) {
				SetLength(Length() - inRange.Length());
			} else if (Intersects(inRange)) {
				if (Start() < inRange.Start()) {
					SetEnd(inRange.Start());
				} else {
					Int32	len = End() - inRange.End();
					
					if (len < 0)
						len = 0;
						
					start = inRange.Start();
					SetLength(len);
				}
			} else if (Start() >= inRange.End()) {
				start -= inRange.Length();
			}
		} else {
			if (Start() <= inRange.Start()) {
				//	nada;
			} else {
				start -= URange32::Min(Start() - inRange.Start(), inRange.Length());
			}
		}
	}
	
	return *this;
}

Range32T&	Range32T::Adjust(
	const Range32T	&inRange,
	Int32			inLengthDelta,
	Boolean			inInclusive)
{
	if (inRange.Length() + inLengthDelta < 0)
		Throw_(paramErr);	//	Illegal parameters.
	
	if (!inRange.Length()) {

		//	a simple insert
		Insert(inRange, inLengthDelta);

	} else if (inRange.Length() == -inLengthDelta) {

		//	a simple delete
		Delete(inRange);

	} else if (Contains(inRange)) {
	
		//	range resize
		SetLength(Length() + inLengthDelta);
	
	} else if (End() <= inRange.Start()) {
		
		//	before change
		//	nada
	
	} else if (Start() >= inRange.End()) {
	
		//	after change
		Translate(inLengthDelta);
	
	} else if ((Start() < inRange.Start()) && (End() < inRange.End())) {
	
		//	left intersects
		if (inInclusive) {
			SetEnd(inRange.End() + inLengthDelta);
		} else {
			SetEnd(inRange.Start());
		}
		
	} else if ((Start() > inRange.Start()) && (End() > inRange.End())) {
	
		//	right intersects
		Int32	end = End() + inLengthDelta;
		if (inInclusive) {
			SetStart(inRange.Start());
		} else {
			SetStart(inRange.End() + inLengthDelta);
		}
		SetEnd(end);
		
	} else if (inRange.Contains(*this)) {
	
		if (inInclusive) {
			SetStart(inRange.Start());
			SetLength(inRange.Length() + inLengthDelta);
		} else {
			*this = inRange;
			Front();
		}
		
	} else {
	
		//	What else is there?
		Throw_(paramErr);
	
	}

	return *this;
}

//	======================================================================

Boolean	Range32T::Adjacent(const Range32T &inRange) const
/*
	Does this share only a boundary position with inRange?
*/
{
	Boolean	rval = false;
	
	if (*this == inRange) {
		rval = Length() == 0;
	} else  {
		Range32T	range = inRange;
		
		range.Intersection(*this);
		if (range.Length()) {
			rval = false;
		} else if (End() == inRange.Start()) {
			rval = true;
		} else if (Start() == inRange.End()) {
			rval = true;
		}
	}
	
	return rval;
}


Boolean	Range32T::Contains(const Range32T &inRange) const
/*
	Does this range contain inRange.
*/
{
	Boolean	rval = false;
	
	if (*this == inRange) {
		rval = true;
	} else if (Length()) {
		if (inRange.Length()) {
			if ((Start() <= inRange.Start()) && (inRange.End() <= End()))
				rval = true;
		} else {
			if ((Start() < inRange.Start()) && (inRange.Start() < End())) {
				rval = true;
			} else if ((Start() == inRange.Start()) && (inRange.Insertion() == rangeCode_After)) {
				rval = true;
			} else if ((End() == inRange.End()) && (inRange.Insertion() == rangeCode_Before)) {
				rval = true;
			}
		}
	}
	
	return rval;
}

Boolean	Range32T::WeakContains(const Range32T &inRange) const
{
	return	(Start() <= inRange.Start()) && (inRange.End() <= End());
}

Boolean	Range32T::Intersects(const Range32T &inRange) const
/*
	Does this intersect with the range on the right hand side
*/
{
	Range32T	temp = *this;
	
	temp.Intersection(inRange);
	
	return !temp.Undefined();
}

Boolean	Range32T::IsBefore(const Range32T &inRange) const
/*
	Is this entirely before inRange
*/
{
	if (End() < inRange.Start())
		return true;
	
	if (End() > inRange.Start())
		return false;
	
	if (inRange.Insertion() == rangeCode_Before) {
		return false;
	} else {
		return Insertion() != rangeCode_After;
	}
}

Boolean	Range32T::IsAfter(const Range32T &inRange) const
{
	return inRange.IsBefore(*this);
}

Range32T&	Range32T::Translate(Int32 inAmount)
{
	start += inAmount;
	
	return *this;
}

EScanDirection Range32T::GetScanDirection(
	const Range32T		&inKnownBounds,
	const Range32T		&inPoint) const
{
	Assert_(!Undefined());
	Assert_(!inKnownBounds.Undefined());
	Assert_(!inPoint.Undefined());
	Assert_(!inPoint.Length());
	
	EScanDirection	direction;
	Range32T		range;
	
	if (inKnownBounds.Contains(inPoint)) {
	
		//	in known bounds (most likely in caching situations so test occurs first?)
		if ((inKnownBounds.End() - inPoint.Start()) < (inPoint.Start() - inKnownBounds.Start()))
			direction = scanDirection_rn;
		else
			direction = scanDirection_lp;
			
	} else if (Contains(inPoint)) {

		if (inPoint.IsBefore(inKnownBounds)) {
			
			//	left of known bounds
			if ((inKnownBounds.Start() - inPoint.Start()) < (inPoint.Start() - Start()))
				direction = scanDirection_ln;
			else
				direction = scanDirection_bp;
		
		} else {
		
			Assert_(inPoint.IsAfter(inKnownBounds));
			
			//	right of known bounds
			if ((End() - inPoint.Start()) < (inPoint.Start() - inKnownBounds.End()))
				direction = scanDirection_en;
			else
				direction = scanDirection_rp;
								 
		}
		
	} else {
	
		//	outside "this"
		if (inPoint.Start() <= inKnownBounds.Start())
			direction = scanDirection_bn;
		else
			direction = scanDirection_ep;

	}
	
	return direction;
}

StRange32TChanger::StRange32TChanger(
	Range32T	&ioVariable,
	Range32T	inNewValue)
		: mVariable(ioVariable),
		  mOrigValue(ioVariable)
{
	ioVariable = inNewValue;
}


StRange32TChanger::~StRange32TChanger()
{
	Range32T	temp(mVariable);
	
	mVariable = mOrigValue;
}

#ifndef	Profile_Range32T
	#pragma	pop
#endif

//	===========================================================================
//	Static members:

const Domain32T	Domain32T::sAll(Range32T::sAll, Range32T::sAll);
const Domain32T	Domain32T::sUndefined(Range32T::sUndefined, Range32T::sUndefined);

//	===========================================================================
//	Domain32T:

Domain32T::Domain32T()
{
	*this = Domain32T::sUndefined;
}

Domain32T::Domain32T(const Range32T &inWidth, const Range32T &inHeight)
{
	width = inWidth;
	height = inHeight;
}

void	Domain32T::SetWidth(const Range32T &inRange)
{
	width = inRange;
}

void	Domain32T::SetHeight(const Range32T &inRange)
{
	height = inRange;
}

const Range32T &	Domain32T::Width(void) const
{
	return width;
}

const Range32T &	Domain32T::Height(void) const
{
	return height;
}

Boolean	Domain32T::IsInsertion(void) const
{
	return (height.Length() == 0) || (width.Length() == 0);
}

Boolean Domain32T::operator==(const Domain32T &inRange) const
{
	if ((width == inRange.width) && (height == inRange.height))
		return true;
	else
		return false;
}

Boolean	Domain32T::Undefined(void) const
{
	return width.Undefined() || height.Undefined();
}

Boolean	Domain32T::Intersects(const Domain32T &inRange) const
{
	Domain32T	temp = *this;
	
	temp.Intersection(inRange);
	
	return !temp.Undefined();
}

Domain32T& Domain32T::Intersection(const Domain32T &inRange)
{
	width.Intersection(inRange.Width());
	height.Intersection(inRange.Height());
	
	return *this;
}

Boolean Domain32T::Adjacent(const Domain32T &inRange) const
{
	Boolean	rval = false;
	
	rval = width.Adjacent(inRange.Width()) && height.Adjacent(inRange.Height());
	
	return rval;
}


Domain32T&	Domain32T::Crop(const Domain32T &inRange)
{
	width.Crop(inRange.Width());
	height.Crop(inRange.Height());
	
	return *this;
}

Boolean	Domain32T::Contains(const Domain32T &inRange) const
{
	Boolean	h = width.Contains(inRange.Width()),
			v = height.Contains(inRange.Height());
			
	return h && v;
}

Domain32T&	Domain32T::Translate(Int32 inHorz, Int32 inVert)
{
	width.Translate(inHorz);
	height.Translate(inVert);
	
	return *this;
}

Domain32T&	Domain32T::Insert(const Domain32T &inRangePt, Int32 inAmount)
/*
	Can only insert on a width and or height when the width or height is an "Insertion"
*/
{
	if (!inRangePt.Width().Length())
		width.Insert(inRangePt.Width(), inAmount);
	
	if (!inRangePt.Height().Length())
		height.Insert(inRangePt.Height(), inAmount);
	
	return *this;
}

Domain32T&	Domain32T::Delete(const Domain32T &inRange)
{
	if (inRange.Width().Length())
		width.Delete(inRange.Width());

	if (inRange.Height().Length())
		height.Delete(inRange.Height());
		
	return *this;
}

Domain32T&	Domain32T::Union(const Domain32T &inRange)
{
	width.Union(inRange.Width());
	height.Union(inRange.Height());
			
	return *this;
}

void	Domain32T::ToRect(Rect &outRect) const
/*
	ASSUMED that *this will not overflow the smaller outRect
*/
{
	outRect.top = Height().Start();
	outRect.left = Width().Start();
	outRect.right = outRect.left + Width().Length();
	outRect.bottom = outRect.top + Height().Length();
}

void	Domain32T::SetRect(const Rect &inRect)
{
	SetHeight(Range32T(inRect.top, inRect.bottom));
	SetWidth(Range32T(inRect.left, inRect.right));
}

//	===========================================================================
//	URange32
//
//	Utility functions for dealing with Range32T variables.

Int32	URange32::Min(Int32 inA, Int32 inB)
{
	if (inA < inB)
		return inA;
	else
		return inB;
}

Int32	URange32::Max(Int32 inA, Int32 inB)
{
	if (inA > inB)
		return inA;
	else
		return inB;
}

Int32	URange32::Abs(Int32 inA)
{
	if (inA > 0)
		return inA;
	else
		return -inA;
}

#ifdef	PP_USEOLDRANGE32

	Boolean	URange32::RangesSame(const Range32T &inRangeA, const Range32T &inRangeB)
	{
		return inRangeA == inRangeB;
	}
	
	Boolean	URange32::RangeWithinRange(const Range32T &inRangeA, const Range32T &inRangeB)
	{
		return	(inRangeA.Start() >= inRangeB.Start()) &&
				(inRangeA.End() <= inRangeB.End());
	}
	
	Boolean	URange32::RangeIntersection(
		const Range32T	&inRangeA,
		const Range32T	&inRangeB,
		Range32T		*outRange)
	{
		Int32	aStart = inRangeA.start,
				bStart = inRangeB.start,
				aEnd = inRangeA.start + inRangeA.length,
				bEnd = inRangeB.start + inRangeB.length;
		
		if ( (aStart < bStart ) && (aEnd < bStart) )
			return false;
	
		if ( (bStart < aStart ) && (bEnd < aStart) )
			return false;
	
		if (outRange) {
			outRange->start = Max(aStart, bStart);
			outRange->length = Min(aEnd, bEnd) - outRange->start;
		}
		
		return true;
	}
	
	Boolean	URange32::RangeUnion(
		const Range32T	&inRangeA,
		const Range32T	&inRangeB,
		Range32T		*outRange)
	{
		Int32		start, end;
		
		start = Min(inRangeA.start, inRangeB.start);
		end = Max(inRangeA.start + inRangeA.length, inRangeB.start + inRangeB.length);
		
		if (outRange) {
			outRange->start = start;
			outRange->length = end - start;
		}
		return true;
	}
	
	void	URange32::RangeShift(const Range32T &inRange, Int32 inDelta, Range32T *outRange)
	{
		Assert_(outRange);
	
		outRange->start = inRange.start + inDelta;
		outRange->length = inRange.length;
	}
	
	void	URange32::RangeExpand(const Range32T &inRange, Int32 inDelta, Range32T *outRange)
	{
		Assert_(outRange);
	
		outRange->start = inRange.start;
		outRange->length = inRange.length + inDelta;
	}
	
	void	URange32::RangeCrop(
		const Range32T		&inRange,
		const Range32T		&inBoundary,
		Range32T			*outRange)
	{
		Int32		start = inRange.Start(),
					end = inRange.End(),
					bStart = inBoundary.Start(),
					bEnd = inBoundary.End();
		
		start = Max(start, bStart);
		end = Min(end, bEnd);
		
		start = Min(start, bEnd);
		end = Max(end, bStart);
		
		outRange->start = start;
		outRange->length = end - start;
		
		Assert_(outRange->length >= 0);
	}
	
	void	URange32::RangeAdjust(
				const Range32T		&inRange,
				Int32				inStart,
				Int32				inDelta,
				Boolean				inInclusive,
				Range32T			*outRange)
	/*
		if
			inDelta > 0
				|inDelta| units are added at inStart.
			
			inDelta < 0
				|inDelta| units are being removed beginning at inStart and proceeding
						"to the right," "next," "sequentially."
	
		Note:	Inclusive parameter doesn't apply when inDelta < 0.
	*/
	{
		Range32T	range = inRange;
		Int32		range_end = range.End(),
					headDelta = 0,
					lengthDelta = 0;
					
		if (inDelta > 0) {
			if (inInclusive) {
				if ( (range.start <= inStart) && (inStart <= range_end) ) {
					lengthDelta = inDelta;
				}
				if (inStart < range.start) {
					headDelta = inDelta;
				}
			} else {
				if ( (range.start < inStart) && (inStart < range_end) ) {
					lengthDelta = inDelta;
				}
				if (inStart <= range.start) {
					headDelta = inDelta;
				}
			}
		} else {
			if ( (range.start <= inStart) && (inStart < range_end) ) {
				lengthDelta = -URange32::Min(range_end - inStart, -inDelta);
				
				Assert_(lengthDelta <= 0);
			}
			if (inStart < range.start) {
				Int32	in_end = inStart + (-inDelta);
				
				if (range.start < in_end) {
					lengthDelta = -URange32::Min(in_end - range.start, range.length);
				}
				
				headDelta = -URange32::Min(range.start - inStart, -inDelta);
			}
		}
	
		range.start += headDelta;
		range.length += lengthDelta;
	
		Assert_(range.start >= 0);
		Assert_(range.length >= 0);
	
		*outRange = range;
	}
#endif
