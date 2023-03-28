//	===========================================================================
//	LTextView.h					©1994 Metrowerks Inc. All rights reserved.
//	===========================================================================

#pragma once

#include	<LSelectHandlerView.h>
#include 	<LTextEngine.h>

class	LTextModel;
class	LTextElemAEOM;
class	LTextSelection;
class	LModelObject;

class	LTextView
			:	public LSelectHandlerView
{
private:
	typedef	LSelectHandlerView	inheritView;
protected:
	friend class LTextEditHandler;	//	this will be going away!
	
public:
				//	Maintenance
	enum { class_ID = kPPCID_TextView };
//-	static LTextView*	CreateTextViewStream(LStream *inStream);
						LTextView();
						LTextView(LStream *inStream);
						LTextView(LStream *inStream, Boolean inOldStyle);
	virtual				~LTextView();
	virtual	void		FinishCreateSelf(void);

				//	Text etc access
	virtual	LTextEngine *	GetTextEngine(void);
	virtual	void			SetTextEngine(LTextEngine *inTextEngine);
	virtual	LTextModel *	GetTextModel(void);
	virtual void			SetTextModel(LTextModel *inTextModel);

	virtual void			SetSelection(LSelection *inSelection);
	virtual	LTextSelection*	GetTextSelection(void);
	virtual	Boolean			GetImageWidthFollowsFrame(void) const;
	virtual	void			SetImageWidthFollowsFrame(Boolean inFollows);
	
	virtual	void		BuildTextObjects(LModelObject *inSuperModel) = 0;
	
				//	Implementation
						//	mEventHandler (override)
	virtual void		NoteOverNewThing(LManipulator *inThing);
	virtual	LSelectableItem *	OverItem(Point inWhere);
	virtual	LSelectableItem *	OverReceiver(Point inWhere);
						//	Linkage
	virtual void		ListenToMessage(MessageT inMessage, void *ioParam);
	virtual	void		SpendTime(const EventRecord &inMacEvent);
	virtual Boolean		ObeyCommand(CommandT inCommand, void *ioParam);
	virtual void		FindCommandStatus(CommandT inCommand,
							Boolean &outEnabled, Boolean &outUsesMark,
							Char16 &outMark, Str255 outName);
	virtual Boolean		FocusDraw(void);
	virtual void		DrawSelf(void);
	virtual void		DrawSelfSelection(void);
	virtual void		ResizeFrameBy(
							Int16		inWidthDelta,
							Int16		inHeightDelta,
							Boolean		inRefresh);
	virtual void		ScrollImageBy(Int32 inLeftDelta, Int32 inTopDelta,
							Boolean inRefresh);
	virtual void		ResizeImageBy(Int32 inWidthDelta, Int32 inHeightDelta,
							Boolean inRefresh);
	virtual void		MoveBy(Int32 inHorizDelta, Int32 inVertDelta,
							Boolean inRefresh);
	virtual void		SubtractSelfErasingAreas(RgnHandle inSourceRgn);
	virtual void		GetPresentHiliteRgn(
							Boolean		inAsActive,
							RgnHandle	ioRegion,
							Int16		inOriginH = 0,
							Int16		inOriginV = 0);
	virtual void		BeTarget(void);
	virtual void		DontBeTarget(void);
	
	virtual void		SavePlace(LStream *outPlace);
	virtual void		RestorePlace(LStream *inPlace);

						//	Printing
	virtual	void		SetPrintRange(const TextRangeT &inRange);
	virtual const TextRangeT &	GetPrintRange(void) const;
	virtual Boolean		ScrollToPanel(const PanelSpec &inPanel);
	virtual void		CountPanels(
							Uint32			&outHorizPanels,
							Uint32			&outVertPanels);
protected:
	virtual	Boolean		NextPanelTop(Int32 *ioPanelTop) const;
public:

				//	Implementation
protected:
	virtual void		MutateSelection(Boolean inAsActive);
	virtual	void		MutateSelectionSelf(
							Boolean			inOldIsCaret,
							const RgnHandle	inOldRegion,
							Boolean			inNewIsCaret,
							const RgnHandle	inNewRegion) const;
	virtual void		CaretUpdate(void);
	virtual void		CaretOn(void);
	virtual void		CaretOff(void);
	virtual void		CaretMaintenance(void);
	virtual	void		UpdateTextImage(TextUpdateRecordT *inUpdate);
	virtual void		CheckSelectionScroll(void);
	virtual void		DrawDebugRects(void);	//	just for debugging
	virtual void		GetViewArea(SPoint32 *outOrigin, SDimension32 *outSize);
	Boolean				mHasActiveTextView;
public:
	virtual void		FixView(void);
	virtual void		FixImage(void);
protected:
	LTextModel			*mTextModel;	//	Link to entire text AEOM object
	LTextEngine			*mTextEngine;	//	Link to text engine class.
										//		"owned by this object"
	
	LTextSelection		*mTextSelection;	//	to avoid casting

	Boolean				mOldSelectionIsCaret;
	Boolean				mOldSelectionIsSubstantive;
	Boolean				mOldSelectionChanged;
	Boolean				mCaretOn;
	Int32				mCaretTime;
	
	TextRangeT			mPrintRange;
	Boolean				mImageWidthFollowsFrame;
};