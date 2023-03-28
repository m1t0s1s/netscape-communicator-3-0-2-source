// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	CTXTextView.h						  ©1995 Netscape. All rights reserved.
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

#pragma once

#include	<LSelectHandlerView.h>
#include 	<LTextEngine.h>

class	LTextModel;
class	LTextElemAEOM;
class	LTextSelection;


struct SSimpleTextParms {
	Uint16				attributes;
	Uint16				wrappingMode;
	ResIDT				traitsID;
	ResIDT				textID;
	Int32				pageWidth;
	Int32				pageHeight;
	Rect				margins;
};


class CTXTextView : public LSelectHandlerView
{
		friend class LTextEditHandler;	//	this will be going away!

			// ¥ Construction/Destruction
	private:
							CTXTextView();	//	Must use parameters
	public:
		enum { class_ID = 'Stxt' };
		static CTXTextView*	CreateTXTextViewStream(LStream* inStream);
							CTXTextView(LStream* inStream);
		virtual				~CTXTextView();
		virtual	void		FinishCreateSelf(void);
				
			// ¥ Text Object Access
		virtual	LTextEngine *	GetTextEngine(void);
		virtual	void			SetTextEngine(LTextEngine *inTextEngine);
		virtual	LTextModel *	GetTextModel(void);
		virtual void			SetTextModel(LTextModel *inTextModel);
		virtual	LTextSelection*	GetTextSelection(void);
		virtual void			SetTextSelection(LTextSelection *inSelection);


			// ¥ Event Handler Overrides
		virtual void			NoteOverNewThing(LManipulator* inThing);
		virtual	LSelectableItem*	OverItem(Point inWhere);
		virtual	LSelectableItem*	OverReceiver(Point inWhere);

			// ¥ Commander Overrides
		virtual Boolean			ObeyCommand(CommandT inCommand, void *ioParam);
		virtual void			FindCommandStatus(
										CommandT 		inCommand,
										Boolean&		outEnabled,
										Boolean&		outUsesMark,
										Char16&			outMark,
										Str255 			outName);
		virtual void			BeTarget(void);
		virtual void			DontBeTarget(void);

			// ¥ Other PowerPlant Linkage
		virtual void			ListenToMessage(MessageT inMessage, void *ioParam);
		virtual	void			SpendTime(const EventRecord &inMacEvent);

			// ¥ View and Image
		virtual Boolean			FocusDraw(void);
		virtual void			DrawSelf(void);
		virtual void			DrawSelfSelection(void);
		virtual void			ResizeFrameBy(
										Int16			inWidthDelta,
										Int16			inHeightDelta,
										Boolean			inRefresh);
		virtual void			ScrollImageBy(
										Int32 			inLeftDelta,
										Int32 			inTopDelta,
										Boolean 		inRefresh);
		virtual void			GetPresentHiliteRgn(
										Boolean			inAsActive,
										RgnHandle		ioRegion,
										Int16			inOriginH = 0,
										Int16			inOriginV = 0);
		virtual void			SubtractSelfErasingAreas(RgnHandle inSourceRgn);
		virtual void			SavePlace(LStream *outPlace);
		virtual void			RestorePlace(LStream *inPlace);
		virtual void			FixImage(void);

			// ¥ Printing
		virtual	void			SetPrintRange(const TextRangeT& inRange);
		virtual const TextRangeT&	GetPrintRange(void) const;
		virtual Boolean			ScrollToPanel(const PanelSpec& inPanel);
		virtual void			CountPanels(
										Uint32&			outHorizPanels,
										Uint32&			outVertPanels);

			// ¥ Initialization
		virtual	void 			SetInitialTraits(ResIDT inTextTraitsID);
		virtual	void			SetInitialText(ResIDT inTextID);
			
	protected:

		virtual	Boolean			NextPanelTop(Int32* ioPanelTop) const;
		
			// ¥ Selection Maintenance
		virtual void			MutateSelection(Boolean inAsActive);
		virtual	void			MutateSelectionSelf(
										Boolean			inOldIsCaret,
										const RgnHandle	inOldRegion,
										Boolean			inNewIsCaret,
										const RgnHandle	inNewRegion) const;
		virtual void			CaretUpdate(void);
		virtual void			CaretOn(void);
		virtual void			CaretOff(void);
		virtual void			CaretMaintenance(void);
		virtual void			CheckSelectionScroll(void);
		Boolean					SelectionIsCaret(Boolean inIsActive) const;

			// ¥ Internal View Maintenance
		virtual	void			UpdateTextImage(TextUpdateRecordT *inUpdate);
		virtual void			GetViewArea(SPoint32 *outOrigin, SDimension32 *outSize);
		Boolean					IsActiveView(void) const;

		
		Boolean					mHasActiveTextView;
		LTextModel*				mTextModel;	//	Link to entire text AEOM object
		LTextEngine*			mTextEngine;	//	Link to text engine class. 
		Boolean					mOldSelectionIsCaret;
		Boolean					mOldSelectionIsSubstantive;
		Boolean					mOldSelectionChanged;
		Boolean					mCaretOn;
		Int32					mCaretTime;
		TextRangeT				mPrintRange;
		Point					mScaleNumer;
		Point					mScaleDenom;

		SSimpleTextParms*		mTextParms;
};

inline Boolean CTXTextView::IsActiveView(void) const
	{	return (IsOnDuty() || mHasActiveTextView);		}
