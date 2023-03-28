//	===========================================================================
//	URange32.h						©1994 Metrowerks Inc. All rights reserved.
//	===========================================================================

#pragma	once

#include	<PP_Prefix.h>

typedef unsigned long	ERangeCode;

const	ERangeCode	rangeCode_Before	= 0x80000000L;
const	ERangeCode	rangeCode_After		= 0x80000001L;
const	ERangeCode	rangeCode_Undefined	= 0xffffffffL;

typedef	enum {
//	scanDirection_[(b)egin|(l)eft|(r)ight|(e)nd][(n)egative|(p)ositive]
	scanDirection_bn,
	scanDirection_bp,
	scanDirection_ln,
	scanDirection_lp,
	scanDirection_rn,
	scanDirection_rp,
	scanDirection_en,
	scanDirection_ep
} EScanDirection;

#ifdef	Length
#undef	Length
#endif

#ifndef	Profile_Range32T_Inline
	#pragma	push
	#pragma	profile	off
#endif

typedef struct Range32T {
public:
	inline				Range32T();
	inline				Range32T(Int32 inStart);
	inline				Range32T(Int32 inStart, Int32 inEnd);
	
	inline	Int32		Start(void) const;
	inline	Int32		End(void) const;
	inline	Int32		Length(void) const;
	inline	Boolean		Undefined(void) const;
	inline	ERangeCode	Insertion(void) const;
			
	inline	void		SetStart(Int32 inStart);	//	note semantic change since CW6/7
	inline	void		SetEnd(Int32 inEnd);
			void		SetLength(Int32 inLength);
			void		SetInsertion(ERangeCode inCode);
			void		SetRange(
							Int32		inOneInsertion,
							Int32		inAnotherInsertion,
							ERangeCode	inCode = rangeCode_Before);

	inline	Boolean		operator==(const Range32T &inRhs) const
							{return	(start == inRhs.start) && (length == inRhs.length);}
	inline	Boolean		operator!=(const Range32T &inRhs) const
							{return	(start != inRhs.start) || (length != inRhs.length);}

			Boolean		Adjacent(const Range32T &inRange) const;
			Boolean		Contains(const Range32T &inRange) const;
			Boolean		WeakContains(const Range32T &inRange) const;
			Boolean		Intersects(const Range32T &inRange) const;
			Boolean		IsBefore(const Range32T &inRange) const;
			Boolean		IsAfter(const Range32T &inRange) const;

			Range32T&	Before(void);
			Range32T&	Front(void);
			Range32T&	Back(void);
			Range32T&	After(void);
			
			Range32T&	Crop(const Range32T &inRange);
			Range32T&	WeakCrop(const Range32T &inRange);
			Range32T&	Union(const Range32T &inRange);
			Range32T&	Intersection(const Range32T &inRange);
			
			Range32T&	Translate(Int32 inAmount);
			Range32T&	Insert(const Range32T &inRangePt, Int32 inAmount);
			Range32T&	Delete(const Range32T &inRange);
			
			EScanDirection
						GetScanDirection(
							const Range32T		&inKnownRange,
							const Range32T		&inPoint) const;

						//	api "decorations"
						Range32T(
							Int32		inStart,
							Int32		inLength,
							ERangeCode	inInsertion);	//	iff inLength == 0
			void		SetLength(
							Int32		inLength,
							ERangeCode	inInsertion);	//	iff inLength == 0
			void		MoveStart(Int32 inStart);
			Range32T&	Adjust(							//	Avoid in favor of Insert & Delete
							const Range32T	&inRange,
							Int32			inLengthDelta,
							Boolean			inInclusive);

public:	
	const static Range32T	sAll;
	const static Range32T	sUndefined;

protected:
	Int32	start;
	Int32	length;	//	> 0		corresponds to length
					//	< 0		an ERangeCode
					//	== 0	shouldn't occur

} Range32T;

//	The inline definitions...
#include	<URange32.icp>

class	StRange32TChanger {
public:
	StRange32TChanger(Range32T &ioVariable, Range32T inNewValue);
	~StRange32TChanger();
	
	Range32T	&mVariable;
	Range32T	mOrigValue;
};

#ifndef	Profile_Range32T_Inline
	#pragma	pop
#endif

typedef struct Domain32T {
public:
						Domain32T();
						Domain32T(const Range32T &inWidth, const Range32T &inHeight);

			Boolean		Undefined(void) const;
			Boolean		operator==(const Domain32T &inRange) const;
			Boolean		operator!=(const Domain32T &inRange) const;
		
	const	Range32T &	Width(void) const;
	const	Range32T &	Height(void) const;
			void		SetWidth(const Range32T &inWidth);
			void		SetHeight(const Range32T &inHeight);
			
			Boolean		IsInsertion(void) const;

	const static Domain32T	sAll;
	const static Domain32T	sUndefined;

			Boolean		Contains(const Domain32T &inRange) const;
			Boolean		Intersects(const Domain32T &inRange) const;
			Boolean		Adjacent(const Domain32T &inRange) const;

			Domain32T&	Crop(const Domain32T &inRange);
			Domain32T&	Union(const Domain32T &inRange);
			Domain32T&	Intersection(const Domain32T &inRange);

			Domain32T&	Insert(const Domain32T &inRangePt, Int32 inAmount);
			Domain32T&	Delete(const Domain32T &inRange);
	
			Domain32T&	Translate(Int32 inHorz, Int32 inVert);
			
			void		ToRect(Rect &outRect) const;
			void		SetRect(const Rect &inRect);

protected:
	Range32T			width,
						height;
} Domain32T;

class	URange32 {
public:
						
	static Int32	Min(Int32 inA, Int32 inB);
	static Int32	Max(Int32 inA, Int32 inB);
	static Int32	Abs(Int32 inA);

#ifdef	PP_USEOLDRANGE32
	//	Don't use these.  All functionality is now provided by Range32T member functions
	static Boolean	RangesSame(const Range32T &inRangeA, const Range32T &inRangeB);
	static Boolean	RangeWithinRange(const Range32T &inRangeA, const Range32T &inRangeB);

	static Boolean	RangeIntersection(
						const Range32T	&inRangeA,
						const Range32T	&inRangeB,
						Range32T		*outRange = NULL);
	static Boolean	RangeUnion(
						const Range32T	&inRangeA,
						const Range32T	&inRangeB,
						Range32T		*outRange = NULL);

	static void		RangeShift(const Range32T &inRange, Int32 inDelta, Range32T *outRange);
	static void		RangeExpand(const Range32T &inRange, Int32 inDelta, Range32T *outRange);
	static void		RangeCrop(
						const Range32T	&inRange,
						const Range32T	&inBoundary,
						Range32T		*outRange);
	static void		RangeAdjust(
						const Range32T	&inRange,
						Int32			inStart,
						Int32			inDelta,
						Boolean			inInclusive,
						Range32T		*outRange);
#endif
};	
