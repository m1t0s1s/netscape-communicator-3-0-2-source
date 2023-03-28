//	===========================================================================
//	LTextSelection.h				©1994 Metrowerks Inc. All rights reserved.
//	===========================================================================

#pragma	once

#include	<LSelection.h>
#include	<LTextEngine.h>

class	LSelectableItem;
class	LTextModel;
class	LAction;
class	LAETypingAction;
class	LStyleSet;

class	LTextSelection
			:	public	LSelection
{
public:
	enum {class_ID = kPPCID_TextSelection};

//-						LTextSelection(LTextEngine *inTextEngine);
						LTextSelection();
	virtual				~LTextSelection();
	virtual ClassIDT	GetClassID() const {return class_ID;}
#ifndef	PP_No_ObjectStreaming
	virtual void		WriteObject(LStream &inStream) const;
	virtual void		ReadObject(LStream &inStream);
#endif
	
	virtual	LTextModel *		GetTextModel(void);
	virtual LTextEngine *		GetTextEngine(void);
	virtual	LAETypingAction *	GetTypingAction(void);
	virtual void		SetTextModel(LTextModel *inTextModel);
	virtual void		SetTextEngine(LTextEngine *inTextEngine);
	virtual void		SetTypingAction(LAETypingAction *inAction);
	
	virtual TextRangeT	GetSelectionRange(void) const;
	virtual void		SetSelectionRange(const TextRangeT &inRange);

	virtual void		SelectDiscontinuous(LSelectableItem *inItem);
	virtual void		AddFlavorTypesTo(LDataTube *inTube) const;
	virtual Boolean		SendFlavorTo(FlavorType inFlavor, LDataTube *inTube) const;

	virtual void		GetPresentHiliteRgn(Boolean inAsActive, RgnHandle ioRegion) const;
	virtual void		MakeRegion(Point inOrigin, RgnHandle *ioRgn) const;
	virtual Boolean		PointInRepresentation(Point inWhere) const;
	virtual LAction *	MakePasteAction(void);

	virtual void		UndoAddOldData(LAEAction *inAction);

	virtual	LStyleSet *	GetCurrentStyle(void);
	virtual	LStyleSet *	GetPendingStyle(void);
	virtual	void		SetPendingStyle(LStyleSet *inStyle);
	virtual void		SelectionAboutToChange(void);

protected:
	
	LTextEngine			*mTextEngine;
	LTextModel			*mTextModel;
	LAETypingAction		*mTyping;

	LStyleSet			*mPendingStyle,
						*mLastCurrentStyle;
};

