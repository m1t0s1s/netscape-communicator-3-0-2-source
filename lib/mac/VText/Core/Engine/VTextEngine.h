//	===========================================================================
//	VTextEngine
//
//	Copyright 1996 ViviStar Consulting.
//	===========================================================================

#pragma	once

#include	<LTextEngine.h>
#include	<VText_ClassIDs.h>

class	VLineCache;
class	VStyleCache;
class	VRulerCache;
class	VParaCache;
class	VLine;

/*?-
void	Insertion2OffsetLeading(
			const TextRangeT	&inInsertionPt,
			Int32				&outOffset,
			Boolean				&outLeadingEdge);

void	OffsetLeading2Insertion(
			Int32				inOffset,
			Boolean				inLeadingEdge,
			TextRangeT			&outInsertionPt);
*/
			
class	VTextEngine
			:	public LTextEngine
{
public:
	enum {class_ID = kVTCID_VTextEngine};
	
	virtual ClassIDT	GetClassID() const {return class_ID;}

						VTextEngine();
						~VTextEngine();
	
				//	going away
	virtual void		ScrollView(
								Int32			inLeftDelta,
								Int32			inTopDelta,
								Boolean			inRefresh);

				//	New features
	virtual ScriptCode	GetScriptRange(
							const TextRangeT	&inInsertionPt,
							TextRangeT			&outRange) const;
				
				//	Implementation...
	virtual void		GetImageSize(SDimension32 *outSize) const;
	virtual void		DrawArea(
								const SPoint32		&inLocation,
								const SDimension32	&inSize);

	virtual Boolean		SpaceForBytes(Int32 inAdditionalBytes) const;
	virtual void		TextGetThruPtr(const TextRangeT &inRange, Ptr outBuffer);
	virtual void		TextReplaceByPtr(const TextRangeT &inRange, const Ptr inSrcText, Int32 inSrcLength);
	virtual const Handle GetTextHandle(void) const;
	virtual void		SetTextHandle(Handle inText);

	virtual	void		GetRangeRgn(const TextRangeT &inRange, RgnHandle *ioRgn) const;
	virtual	void		Range2Image( 
								const TextRangeT	&inRange,
								Boolean				inLeadingEdge,
								SPoint32			*outWhere) const;
	virtual void		Image2Range(
								SPoint32			inLocation,
								Boolean				*outLeadingEdge,
								TextRangeT			*outRange) const;
	virtual Boolean		RangeIsMultiLine(const TextRangeT &inRange) const;
	virtual Int32		GetRangeTop(const TextRangeT &inRange) const;
	virtual	Int32		GetRangeHeight(const TextRangeT &inRange) const;
	virtual Int32		GetRangeWidth(const TextRangeT &inRange) const;

	virtual	Int32		CountStyleSets(const TextRangeT &inRange) const;
	virtual	LStyleSet *	GetStyleSet(const TextRangeT &inRange, Int32 inIndex = 1) const;
	virtual void		SetStyleSet(const TextRangeT &inRange, const LStyleSet *inNewStyle = NULL);
	virtual	void		GetStyleSetRange(TextRangeT *ioRange) const;
	virtual	void		StyleSetChanged(LStyleSet *inChangedStyle);
	
	virtual	Int32		CountRulerSets(const TextRangeT &inRange) const;
	virtual	LRulerSet *	GetRulerSet(const TextRangeT &inRange, Int32 inIndex = 1) const;
	virtual void		SetRulerSet(const TextRangeT &inRange, const LRulerSet *inNewRuler = NULL);
	virtual	void		GetRulerSetRange(TextRangeT *ioRange) const;
	virtual	void		RulerSetChanged(LRulerSet *inChangedRuler);

	virtual	DescType	FindPart(
							const TextRangeT	&inQueryRange,
							TextRangeT			*ioSubRange,
							DescType			inSubPartType,
							Int32				*outNextOffset) const;

	virtual	void		Refresh(RecalcFlagsT inFlags);
	virtual void		Refresh(const TextRangeT &inRange);

protected:
	virtual	Int32		CountLines(const TextRangeT &inQueryRange) const;
	virtual	Int32		CountParas(const TextRangeT &inQueryRange) const;
	virtual	DescType	FindNthLine(
							const TextRangeT	&inQueryRange,
							Int32				inPartIndex,
							TextRangeT			*outRange) const;
	virtual	DescType	FindNthPara(
							const TextRangeT	&inQueryRange,
							Int32				inPartIndex,
							TextRangeT			*outRange) const;
	virtual	DescType	FindLineSubRangeIndices(
							const TextRangeT	&inQueryRange,
							const TextRangeT	&inSubRange,
							Int32				*outIndex1,
							Int32				*outIndex2) const;
	virtual	DescType	FindParaSubRangeIndices(
							const TextRangeT	&inQueryRange,
							const TextRangeT	&inSubRange,
							Int32				*outIndex1,
							Int32				*outIndex2) const;
	virtual	DescType	FindCharSubRangeIndices(
							const TextRangeT	&inQueryRange,
							const TextRangeT	&inSubRange,
							Int32				*outIndex1,
							Int32				*outIndex2) const;
	virtual	DescType	FindLine(
							const TextRangeT	&inQueryRange,
							TextRangeT			*ioRange,
							Int32				*outEndOffset) const;
	virtual	DescType	FindPara(
							const TextRangeT	&inQueryRange,
							TextRangeT			*ioRange,
							Int32				*outEndOffset) const;
	virtual	DescType	FindPara_(
							const TextRangeT	&inQueryRange,
							TextRangeT			*ioRange,
							Int32				*outEndOffset) const;
	virtual	DescType	FindWord(
							const TextRangeT	&inQueryRange,
							TextRangeT			*ioRange,
							Int32				*outEndOffset) const;
	
protected:
	virtual	void	RecalcSelf(void);
	virtual void	TextFluxPostSelf(
						const TextRangeT	&inRange,
						Int32				inLengthDelta);
	virtual void	NestedUpdateOut(void);
	virtual	void	Reserve(Int32 inNeeded);
	
protected:
				//	the text
	Handle			mText;
	Int32			mBlockingFactor;	//	how many extra bytes to grow mText at a time by

				//	part caches...
	VLineCache		*mLines;
	VStyleCache		*mStyles;
	VRulerCache		*mRulers;
	VParaCache		*mParas;

				//	line caching / handling
	virtual void	InvalidateLineCache(void);
	virtual void	HitLine(Int32 inLineIndex) const;
	virtual	VLine *	GetLine(Int32 inLineIndex) const;
	virtual VLine *	LayoutLine(
						Int32				inLineIndex,
						const LRulerSet		&inRuler,
						const TextRangeT	&inParaRange,
						const TextRangeT	&inLineStartPt,
						TextRangeT			&outLineRange) const;
/*-
	virtual void	AppendLineRegions(
						Int32				inLineIndex,
						const Ptr			inText,
						const TextRangeT	&inRange,
						const Domain32T		&inLineViewDomain,
						RgnHandle			ioRgn) const;
	virtual void	UnionDomainToRegion(
						const Domain32T		&inDomain,
						RgnHandle			ioRgn) const;
*/
						
	typedef struct LineCacheT {
		Int32	lineIndex;
		VLine	*line;
	} LineCacheT;
	const enum		{kLineCacheSize = 4};
	LineCacheT		mLineCache[kLineCacheSize];
	
	friend	class	VLineCache;
	friend	class	VParaCache;
	friend	class	LRulerSet;

//	things that don't belong in a text engine...
public:
	virtual void	GetViewRect(
						SPoint32		*outLocation,
						SDimension32	*outSize) const;
	virtual void	SetViewRect(
						const SPoint32		&inLocation,
						const SDimension32	&inSize);
protected:
	SPoint32		mViewOrigin;
	SDimension32	mViewSize;
	
	TextRangeT		mUpdateRangeReformatted;
};
