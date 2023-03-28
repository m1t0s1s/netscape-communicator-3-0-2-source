//	===========================================================================
//	LAETypingAction.h				©1995 Metrowerks Inc. All rights reserved.
//	===========================================================================

#pragma once

#include	<LAEAction.h>
#include	<UAppleEventsMgr.h>
#include	<LDynamicBuffer.h>
#include	<LTextEngine.h>

class	LTextSelection;
class	LAEStream;

//	===========================================================================

class	LAETypingAction
			: public LAEAction
{
private:
						LAETypingAction();	//	Parameters required

				//	Maintenance / initialization
public:
						LAETypingAction(LTextEngine *inTextEngine);
	virtual				~LAETypingAction();

	virtual void		SetSelection(
							LSelection	*inSelection,
							DescType	inRedoSelectionModifier = typeNull,
							DescType	inUndoSelectionModifier = typeNull);
	virtual	void		FinishCreateSelf(void);
	
	virtual void		Finalize(void);

				//	Overrides
	virtual void		RedoSelf(void);
	virtual void		UndoSelf(void);
	virtual Boolean		CanRedo(void) const;
	virtual Boolean		CanUndo(void) const;
	
				//	Typing support
	virtual Boolean		NewTypingLocation(void);
	virtual	void		UpdateTypingLocation(void);
	virtual void		PrimeFirstRemember(const TextRangeT &inRange);
	virtual	void		KeyStreamRememberNew(const TextRangeT &inRange);
	virtual void		KeyStreamRememberAppend(const TextRangeT &inRange);
	virtual void		KeyStreamRememberPrepend(const TextRangeT &inRange);

				//	Implementation
protected:
	virtual	Int32		TypingBuffered(void) const;
	virtual void		WriteTypingData(LAEStream &inStream) const;
	virtual void		WriteOldTarget(LAEStream &inStream) const;
	virtual	void		WriteRangeIndexKey(
							LAEStream	&inStream,
							Int32 		inIndex1,
							Int32		inIndex2,
							DescType	inType1,
							DescType	inType2) const;

protected:
	LTextEngine			*mTextEngine;
	LTextSelection		*mTextSelection;
	Boolean				mFinalized;
	TextRangeT			mLastRange;			//	Used to detect a new typing sequence

	LDynamicBuffer		mNewText;			//	Never needs to be styled --
											//		style changes would constitute another
											//		undoable action
	LDynamicBuffer		mOldText;			//	Must be augmented with another storage facility
											//	when undo of styled text typing is supported.

	StAEDescriptor		mNewOSpec;			//	Object specifier for new text
	TextRangeT			mOldTotalRange;
	TextRangeT			mOldRange;			//	When undone
	TextRangeT			mOldRangeOrig;

	StAEDescriptor		mOldOSpec;			//	Ospec for old text.  TypeNull if necessary
											//		to resort to making one from...
											
	Range32T			mOldCharIdx;		//	Character offset and length relative to...
	TextRangeT			mOldContRange;		//	Range of mOldCharIdx "container."
	DescType			mOldContType;		//	Type of container -- cParagraph or cLine;
	Range32T			mOldContIdx;		//	Container offset and length relative to model of
											//		selection.
};
