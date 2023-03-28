// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	CTXTextEngine.cp					  ©1995 Netscape. All rights reserved.
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

#pragma	once

#include	<LTextEngine.h>

class	CTextension;
class	CDisplayChanges;
class	CRunObject;

// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	Wrapping Modes
//
//	Text can be wrapped in one of the following three ways:
//
//		wrapMode_WrapToFrame - Wrap to view frame.  This is similar to common
//			TE style wrapping.  
//
//		wrapMode_FixedPage - Don't wrap.  This would be like the metrowerks
//			code editor in which the page size is arbitrarily large in width
//			but no wrapping will occur when text flows outside the page bounds.
//
//		wrapMode_FixedPage - Text is wrapped to the width of the page.  This
//			would be like a word processor.
//
//		wrapMode_WrapWidestUnbreakable - The page size is determined by
//			the view frame size or the width of the widest unbreakable run.
//			This behaviour would be most like an HTML browser.  Text can
//			always be broken, but things like graphic images can not.
//			With text only, this behaves like the normal wrap to frame mode.
//
//	IMPORTANT NOTE:  if you are providing multiple views on a text engine,
//		and those views have different wrapping modes, the formatter will
//		reformat the text to the mode specified by the view EVERY TIME THE
//		VIEW IS FOCUSED.  This is probably not something you'd want to have
//		happening in your app.
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

enum {
	wrapMode_WrapToFrame = 1,
	wrapMode_FixedPage,
	wrapMode_FixedPageWrap,
	wrapMode_WrapWidestUnbreakable
};


const	RecalcFlagsT	recalc_DisplayChanges = 0x08L;

class	CTXTextEngine : public LTextEngine
{
	public:
		static	void		TXInitialize(void);

							CTXTextEngine();
		virtual				~CTXTextEngine();
	
		virtual GrafPtr 	GetPort(void);
		virtual void		SetPort(GrafPtr inPort);
	
			// ¥ Page
		virtual	void		SetWrapMode(Uint16 inMode);
		virtual	Uint16		GetWrapMode(void) const;

		virtual void		SetPageFrameSize(const SDimension32& inSize);
	
	
			// ¥ Appearance
		virtual	void		SetAttributes(Uint16 inAttributes);
		virtual void		SetTextMargins(const Rect &inMargins);
		virtual void		GetImageSize(SDimension32 *outSize) const;
		virtual void		SetImageSize(const SDimension32 &inSize);

		virtual void		GetViewRect(
									SPoint32		*outLocation,
									SDimension32	*outSize) const;
									
		virtual void		DrawArea(
									const SPoint32		&inLocation,
									const SDimension32	&inSize);
	
			// ¥ Important ranges
		virtual const TextRangeT &	GetTotalRange(void) const;
	
			// ¥ Text query and editing
		virtual Boolean		SpaceForBytes(Int32 inAdditionalBytes) const;
							//	Favor tube functions instead (see LTextEngine.h for note)...
		virtual void		TextGetThruHandle(const TextRangeT &inRange, Handle outTextH);
		virtual void		TextReplaceByPtr(const TextRangeT &inRange, const Ptr inSrcText, Int32 inSrcLength);
		
			// ¥ Presentation query
		virtual	Int32		GetRangeTop(const TextRangeT &inRange) const;
		virtual	Int32		GetRangeHeight(const TextRangeT &inRange) const;
		virtual	Boolean		Range2Image( 
									const TextRangeT	&inRange,
									Boolean				inLeadingEdge,
									SPoint32			*outWhere) const;
		virtual Boolean		Image2Range(
									SPoint32			inLocation,
									Boolean				*outLeadingEdge,
									TextRangeT			*outRange) const;
		
			// ¥ Styles
		virtual	void		SetDefaultStyleSet(LStyleSet *inNewStyle);
		virtual	LStyleSet *	GetStyleSet(const TextRangeT &inRange, Int32 inIndex = 1) const;
		virtual void		SetStyleSet(const TextRangeT &inRange, const LStyleSet *inNewStyle = NULL);
		virtual	Int32		CountStyleSets(const TextRangeT &inRange) const;
		virtual	void		GetStyleSetRange(TextRangeT *ioRange) const;
		virtual	void		StyleSetChanged(LStyleSet *inChangedStyle);
			
		virtual const Handle GetTextHandle(void) const;
		virtual void		SetTextHandle(Handle inText);
		
	protected:
	
			// ¥ Text part implementation support 
		virtual	DescType	FindPara(
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
	
		virtual	Int32		CountLines(const TextRangeT &inQueryRange) const;
		
		virtual	DescType	FindNthLine(
									const TextRangeT	&inQueryRange,
									Int32				inPartIndex,
									TextRangeT			*outRange) const;
									
		virtual	DescType	FindLineSubRangeIndices(
									const TextRangeT	&inQueryRange,
									const TextRangeT	&inSubRange,
									Int32				*outIndex1,
									Int32				*outIndex2) const;
	

			// ¥ General implementation help
		virtual	void		RefreshTX(CDisplayChanges &inChanges);
		virtual void		RecalcSelf(void);


		Uint16				mWrappingMode;
		CDisplayChanges		*mChanges;
		CTextension			*mText;		//	Should be protected.
	
		static	void		SetDefaultRun_(CRunObject *inRun);

		static	Boolean		sTXInited;	//	Has Textension been initialized?
	
};



