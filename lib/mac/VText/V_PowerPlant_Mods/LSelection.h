//	===========================================================================
//	LSelection.h					©1994 Metrowerks Inc. All rights reserved.
//	===========================================================================

#pragma	once

#include	<LSelectableItem.h>
#include	<LList.h>
#include	<LBroadcaster.h>
#include	<LStreamable.h>
#include	<PP_ClassIDs_DM.h>

class	LSelectableItem;
class	LDataPipe;
class	LAction;
class	LAEAction;
class	LStream;

enum {
	msg_SelectionAboutToChange = 20000,
	msg_SelectionChanged
};

class	LSelection
			:	public	LSelectableItem
			,	public	LBroadcaster
			,	public	LStreamable
{
public:
	enum {class_ID = kPPCID_Selection};

						LSelection();
	virtual				~LSelection(void);
	virtual ClassIDT	GetClassID() const {return class_ID;}
#ifndef	PP_No_ObjectStreaming
	virtual void		WriteObject(LStream &inStream) const;
	virtual void		ReadObject(LStream &inStream);
#endif


public:
				//	New features
	virtual void		SelectSimple(LSelectableItem *inItem);
	virtual void		SelectContinuous(LSelectableItem *inItem);
	virtual void		SelectDiscontinuous(LSelectableItem *inItem);
	
				//	Query
	virtual	Boolean		IsSubstantive(void) const;
	virtual	Int32		SelectionID(void) const;
	virtual LSelectableItem *	ListEquivalentItem(const LSelectableItem *inItem) const;
	virtual LSelectableItem *	ListDependentItem(const LSelectableItem *inItem) const;
	virtual LSelectableItem *	ListAdjacentItem(const LSelectableItem *inItem) const;

				//	Direct list manipulation
	virtual Int32		ListCount(void) const;
	virtual LSelectableItem *	ListNthItem(Int32 inIndex);
	virtual const LSelectableItem *	ListNthItem(Int32 inIndex) const;
	virtual Boolean		ListContains(const LSelectableItem *inItem) const;
						//	only use add, remove, and clear calls when sandwiched
						//	between about to change and changed calls...
	virtual void		SelectionAboutToChange(void);
	virtual void		SelectionChanged(void);
	virtual void		ListClear(void);
	virtual void		ListAddItem(LSelectableItem *inItem);
	virtual void		ListRemoveItem(LSelectableItem *inItem);

				//	Visual
	virtual void		MakeRegion(Point inOrigin, RgnHandle *ioRgn) const;
	virtual void		DrawSelected(void);
	virtual void		DrawLatent(void);
	virtual void		UnDrawSelected(void);
	virtual void		UnDrawLatent(void);
	virtual void		DrawContent(void);	//	seldom used.
	
				//	Implementation -- manipulator
	virtual ManipT		ItemType(void) const {return kSelection;}
	virtual Boolean		EquivalentTo(const LSelectableItem *inItem) const;
	virtual Boolean		AdjacentWith(const LSelectableItem *inItem) const;
	virtual	Boolean		IndependentFrom(const LSelectableItem *inItem) const;
	virtual Boolean		PointInRepresentation(Point inWhere) const;
	virtual void		DrawSelf(void);
	virtual void		UnDrawSelf(void);

				//	Data tubing implementation:
	virtual void		AddFlavorTypesTo(LDataTube *inTube) const;
	virtual Boolean		SendFlavorTo(FlavorType inFlavor, LDataTube *inTube) const;
	virtual FlavorType	PickFlavorFrom(const LDataTube *inTube) const;
	virtual void		ReceiveDataFrom(LDataTube *inTube);

				//	AEOM support
	virtual	void		DoAESelectAsResult(const AEDesc &inValue, DescType inPositionModifier);
	virtual void		DoAESelection(
									const AEDesc	&inValue,
									Boolean			inExecute = true);
	virtual void		RecordPresentSelection(void);	//	DoAESelection is preferred.
	virtual	void		MakeSelfSpecifier(
									AEDesc&			inSuperSpecifier,
									AEDesc&			outSelfSpecifier) const;
	virtual void		GetAEValue(
									const AEDesc	&inRequestedType,
									AEDesc			&outPropertyDesc) const;
	virtual void		SetAEValue(
									const AEDesc	&inValue,
									AEDesc			&outAEReply);
	virtual void		GetAEProperty(
									DescType		inProperty,
									const AEDesc	&inRequestedType,
									AEDesc			&outPropertyDesc) const;
	virtual void		SetAEProperty(
									DescType		inProperty,
									const AEDesc	&inValue,
									AEDesc			&outAEReply);
	virtual	void		GetImportantAEProperties(AERecord &outKeyDescList) const;
	virtual	void		GetContentAEKinds(AEDesc &outKindList);
	virtual	void		HandleDelete(
									AppleEvent			&outAEReply,
									AEDesc				&outResult);

				//	Clipboard actions
	virtual	LAction *	MakeCutAction(void);
	virtual	LAction *	MakeCopyAction(void);
	virtual	LAction *	MakeClearAction(void);
	virtual	LAction *	MakePasteAction(void);
	
						//	Implementation help
	virtual void		UndoAddOldData(LAEAction *inAction);

protected:
	Boolean				mRecordAllSelections;
	LSelectableItem		*mAnchor;

protected:
	LList				mItems;
	Int32				mSelectionID;	//	to tell if the selection has changed
};

class StSelectionChanger
{
public:
						StSelectionChanger(LSelection &inSelection);
						~StSelectionChanger();
protected:
	LSelection			*mSelection;
};

