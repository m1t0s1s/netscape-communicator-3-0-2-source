//	===========================================================================
//	LTextEngine.h				©1995 Metrowerks Inc. All rights reserved.
//	===========================================================================

#pragma	once

#include	<LSharable.h>
#include	<LBroadcaster.h>
#include	<LRecalcAccumulator.h>
#include	<LStreamable.h>
#include	<URange32.h>
#include	<PP_ClassIDs_DM.h>

class	LHandlerView;
class	LAETypingAction;
class	LStyleSet;
class	LRulerSet;

//	---------------------------------------------------------------------------
//	Types and enums...

typedef	Range32T	TextRangeT;

#include	<TextEdit.h>
#include	<LPane.h>	//	For coordinate types.
#include	<LDynamicBuffer.h>
#include	<UTextTraits.h>

typedef enum {
		textEngineAttr_MultiStyle		= 0x8000
	,	textEngineAttr_MultiRuler		= 0x0200
	,	textEngineAttr_Editable			= 0x4000
	,	textEngineAttr_Selectable		= 0x2000
//-	,	textEngineAttr_WordWrap			= 0x1000	//	a function of rulersets
//-	,	textEngineAttr_FixedHeight		= 0x0800	//	a function of rulersets
	,	textEngineAttr_MultiByte		= 0x0400
}	TextEngineAttributesE;

//	should this end up somewhere else?
typedef struct SScrollPrefs {

				SScrollPrefs();

	Range32T	hWhere,		//	range of frame to bring critical point inside of
				vWhere;		//		proportional:	(frame location) / 256
							//
							//			where(0x0010, 0x00f0) means the critical pt will be brought
							//				inside 10/256'ths to 240/256'ths of frame
							//
							//			where(0x0055) places range at the top 1/3 of the frame

	Boolean		doEnd;		//	true:	critical point at end of range
							//	false:	critical point at start of range

	Boolean		doAlways;	//	true:	the critical pt will always be brought within where
							//	false:	the critical pt is brought within where only if no
							//			part of the range is visible	
} SScrollPrefs;

const	RecalcFlagsT	recalc_WindowUpdate = 0x01L;
//- const	RecalcFlagsT	recalc_FormatAllText = 0x02L;
const	RecalcFlagsT	recalc_Range = 0x04L;
const	RecalcFlagsT	recalc_AllWidth = 0x08L;

typedef struct {
	TextRangeT	range;
	Int32		lengthDelta;
} TextFluxRecordT;

typedef struct {
								//	ALL in image y-pixels
	Int32		scrollAmount,	//		scroll amount and direction
				scrollStart;	//		scroll panel start
	Range32T	gap;			//		range to update
} TextUpdateRecordT;

enum {
	msg_ModelAboutToChange = 10000,
	msg_ModelChanged,
	msg_ModelWidthChanged,
	msg_ModelScopeChanged,
	msg_TextFluxed,				//	param is a TextFluxRecordT *
	msg_CumulativeRangeChanged,	//	param is a TextRangeT *
	msg_InvalidateImage,		//	have text views refresh themselves
	msg_UpdateImage				//	param is a TextUpdateRecordT *
};

//	---------------------------------------------------------------------------
//	LTextEngine:

#include	<LStreamable.h>

class	LTextEngine
			:	virtual public LSharable
			,	public LBroadcaster
			,	public LRecalcAccumulator
			,	public LStreamable
{
public:
	enum {class_ID = kPPCID_TextEngine};

						LTextEngine();
	virtual				~LTextEngine();
	virtual ClassIDT	GetClassID() const {return class_ID;}
#ifndef	PP_No_ObjectStreaming
	virtual void		WriteObject(LStream &inStream) const;
	virtual void		ReadObject(LStream &inStream);
#endif

				//	Linkage that sort of belongs...
	virtual GrafPtr 	GetPort(void);
	virtual void		SetPort(GrafPtr inPort);
	virtual LHandlerView *	GetHandlerView(void);
	virtual void		SetHandlerView(LHandlerView *inHandlerView);
						//	from LRecalcAccumulator
	virtual	void		Refresh(RecalcFlagsT inFlags);
	virtual void		Refresh(const TextRangeT &inRange);
	virtual void		RecalcSelf(void);
	virtual void		NestedUpdateIn(void);
	virtual void		NestedUpdateOut(void);
		
				//	Text changed status (reset w/ SetTextChange(false))
	virtual	Boolean		GetTextChanged(void) ;
	virtual	void		SetTextChanged(Boolean inChanged = true);
	
				//	Appearance
	virtual	void		SetAttributes(Uint16 inAttributes);
	virtual Uint16		GetAttributes(void) const;
	virtual void		SetTextMargins(const Rect &inMargins);
	virtual void		GetTextMargins(Rect *outMargins) const;
	virtual void		GetImageSize(SDimension32 *outSize) const;
	virtual void		SetImageWidth(Int32 inWidth);
	virtual void		GetViewRect(
								SPoint32		*outLocation,
								SDimension32	*outSize) const;
	virtual void		SetViewRect(
								const SPoint32		&inLocation,
								const SDimension32	&inSize);
	virtual void		DrawArea(
								const SPoint32		&inLocation,
								const SDimension32	&inSize);

				//	Scrolling
	virtual void		ScrollView(
								Int32			inLeftDelta,
								Int32			inTopDelta,
								Boolean			inRefresh);
	virtual	void		ScrollToRange(const TextRangeT &inRange);
	virtual Boolean		GetRangeScrollVector(
								const TextRangeT	&inRange,
								SPoint32			*outVector) const;
	const SScrollPrefs &	ScrollPreferences(void) const;
	SScrollPrefs & 			ScrollPreferences(void);


				//	Important ranges
	virtual	const TextRangeT &	GetTotalRange(void) const;
	static const TextRangeT	sTextAll;		//	Range indicating all text
	static const TextRangeT	sTextStart;		//	Range indicating start of text
	static const TextRangeT	sTextEnd;		//	Range indicating end of text
	static const TextRangeT	sTextUndefined;	//	Undefined range

				//	Character query
	virtual Int32		ThisCharSize(Int32 inInsertionLocation) const;
//-	virtual Int32		NextCharSize(Int32 inInsertionLocation) const;
	virtual Int32		PrevCharSize(Int32 inInsertionLocation) const;
	virtual Int16		ThisChar(Int32 inInsertionLocation) const;

				//	Text query and editing
	virtual Boolean		SpaceForBytes(Int32 inAdditionalBytes) const;
	virtual void		TextMove(const TextRangeT &inRangeA, const TextRangeT &inRangeB);
	virtual void		TextDelete(const TextRangeT &inRange);
	virtual void		TextGetThruPtr(const TextRangeT &inRange, Ptr outBuffer);
	virtual void		TextGetThruHandle(const TextRangeT &inRange, Handle outTextH);
	virtual void		TextReplaceByPtr(const TextRangeT &inRange, const Ptr inSrcText, Int32 inSrcLength);
	virtual void		TextReplaceByHandle(const TextRangeT &inRange, const Handle inNewTextH);
						//	For fast file i/o you can use these.
	virtual const Handle GetTextHandle(void) const;
	virtual void		SetTextHandle(Handle inText);
		
				//	Presentation
	virtual	void		GetRangeRgn(const TextRangeT &inRange, RgnHandle *ioRgn) const;
	virtual	Boolean		PointInRange(SPoint32 inWhere, const TextRangeT &inRange) const;
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

				//	Styles
	virtual	LStyleSet *	GetDefaultStyleSet(void) const;
	virtual	void		SetDefaultStyleSet(LStyleSet *inNewStyle);
	virtual	Int32		CountStyleSets(const TextRangeT &inRange) const;
	virtual	LStyleSet *	GetStyleSet(const TextRangeT &inRange, Int32 inIndex = 1) const;
	virtual void		SetStyleSet(const TextRangeT &inRange, const LStyleSet *inNewStyle = NULL);
	virtual	void		GetStyleSetRange(TextRangeT *ioRange) const;
	virtual	void		StyleSetChanged(LStyleSet *inChangedStyle);
	
				//	Rulers
	virtual	LRulerSet *	GetDefaultRulerSet(void) const;
	virtual	void		SetDefaultRulerSet(LRulerSet *inNewRuler);
	virtual	Int32		CountRulerSets(const TextRangeT &inRange) const;
	virtual	LRulerSet *	GetRulerSet(const TextRangeT &inRange, Int32 inIndex = 1) const;
	virtual void		SetRulerSet(const TextRangeT &inRange, const LRulerSet *inNewRuler = NULL);
	virtual	void		GetRulerSetRange(TextRangeT *ioRange) const;
	virtual	void		RulerSetChanged(LRulerSet *inChangedRuler);
	
				//	Indexing methods to support text parts (req'd for the AEOM)
	virtual	Int32		CountParts(
								const TextRangeT	&inQueryRange,
								DescType			inPartType) const;
	virtual	DescType	FindNthPart(
								const TextRangeT	&inQueryRange,
								DescType			inPartType,
								Int32				inPartIndex,
								TextRangeT			*outPartRange) const;
	virtual	DescType	FindSubRangePartIndices(
								const TextRangeT	&inQueryRange,
								const TextRangeT	&inSubRange,
								DescType			inSubRangeType,
								Int32				*outFirstIndex,
								Int32				*outLastIndex) const;
	virtual	DescType	FindPart(
								const TextRangeT	&inQueryRange,
								TextRangeT			*ioSubRange,
								DescType			inSubPartType,
								Int32				*outNextOffset = NULL) const;

				//	Linkage (none of these belong in a "text engine" and they
				//		may disappear in the future -- avoid using them.
	virtual void		Focus(void);

//	----------------------------------------------------------------------
//	Implementation details...

protected:
				//	Useful data members...
	Rect				mMargins;
	LStyleSet			*mDefaultStyleSet;
	LRulerSet			*mDefaultRulerSet;
	Uint16				mAttributes;
					//	Bookkeeping chores
	LHandlerView		*mHandlerView;
	Boolean				mIsChanged;
	TextRangeT			mTotalRange;
	
	Int32				mUpdatePreHeight;	//	pre-update size
	TextRangeT			mUpdateRange;
	Int32				mUpdatePreLength;
	
	SScrollPrefs		mScrollPrefs;
	
				//	General implementation help
protected:
	virtual	const void *	GetTextPtr(Int32 inOffset) const;
	virtual void		CheckImageSize(void);
	SDimension32		mImageSize;
	Int32				mImageWidth;
	
	TextRangeT			mPreFluxRange;
	virtual void		TextFluxPre(const TextRangeT &inRange);
	virtual void		TextFluxPost(Int32 inLengthDelta);
	virtual void		TextFluxPostSelf(
							const TextRangeT	&inRange,
							Int32				inLengthDelta);
	
	virtual void		DomainToLocalRect(const Domain32T &inDomain, Rect *outRect) const;

protected:		//	Text part implementation support
	virtual	Int32		CountParas(const TextRangeT &inQueryRange) const;
	virtual	Int32		CountLines(const TextRangeT &inQueryRange) const;
	virtual	Int32		CountWords(const TextRangeT &inQueryRange) const;
	virtual	Int32		CountChars(const TextRangeT &inQueryRange) const;
	virtual	DescType	FindNthPara(
								const TextRangeT	&inQueryRange,
								Int32				inPartIndex,
								TextRangeT			*outRange) const;
	virtual	DescType	FindNthLine(
								const TextRangeT	&inQueryRange,
								Int32				inPartIndex,
								TextRangeT			*outRange) const;
	virtual	DescType	FindNthWord(
								const TextRangeT	&inQueryRange,
								Int32				inPartIndex,
								TextRangeT			*outRange) const;
	virtual	DescType	FindNthChar(
								const TextRangeT	&inQueryRange,
								Int32				inPartIndex,
								TextRangeT			*outRange) const;
	virtual	DescType	FindParaSubRangeIndices(
								const TextRangeT	&inQueryRange,
								const TextRangeT	&inSubRange,
								Int32				*outIndex1,
								Int32				*outIndex2) const;
	virtual	DescType	FindLineSubRangeIndices(
								const TextRangeT	&inQueryRange,
								const TextRangeT	&inSubRange,
								Int32				*outIndex1,
								Int32				*outIndex2) const;
	virtual	DescType	FindWordSubRangeIndices(
								const TextRangeT	&inQueryRange,
								const TextRangeT	&inSubRange,
								Int32				*outIndex1,
								Int32				*outIndex2) const;
	virtual	DescType	FindCharSubRangeIndices(
								const TextRangeT	&inQueryRange,
								const TextRangeT	&inSubRange,
								Int32				*outIndex1,
								Int32				*outIndex2) const;
	virtual	Int32		CountParts_(
								const TextRangeT	inQueryRange,
								DescType			inPartType) const;
	virtual	DescType	FindNthPart_(
								const TextRangeT	&inQueryRange,
								Int32				inPartIndex,
								DescType			inPartType,
								TextRangeT			*outRange) const;
	virtual	DescType	FindPartSubRangeIndices(
								const TextRangeT	&inQueryRange,
								const TextRangeT	&inSubRange,
								DescType			inSubPartType,
								Int32				*outIndex1,
								Int32				*outIndex2) const;
	virtual	DescType	FindPara(
								const TextRangeT	&inQueryRange,
								TextRangeT			*ioRange,
								Int32				*outEndOffset) const;
	virtual	DescType	FindPara_(
								const TextRangeT	&inQueryRange,
								TextRangeT			*ioRange,
								Int32				*outEndOffset) const;
	virtual	DescType	FindLine(
								const TextRangeT	&inQueryRange,
								TextRangeT			*ioRange,
								Int32				*outEndOffset) const;
	virtual	DescType	FindWord(
								const TextRangeT	&inQueryRange,
								TextRangeT			*ioRange,
								Int32				*outEndOffset) const;
	virtual	DescType	FindChar(
								const TextRangeT	&inQueryRange,
								TextRangeT			*ioRange,
								Int32				*outEndOffset) const;
	
	virtual	Boolean		IsWhiteSpace(const TextRangeT &inRange) const;
};
