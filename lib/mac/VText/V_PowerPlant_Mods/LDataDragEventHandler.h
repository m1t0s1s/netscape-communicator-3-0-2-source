//	===========================================================================
//	LDataDragEventHandler.h			©1994 Metrowerks Inc. All rights reserved.
//	===========================================================================

#pragma	once

#include	<LEventHandler.h>
#include	<LDragAndDrop.h>
#include	<LDataDragTask.h>

class	LDataTube;

class	LDataDragEventHandler
			:	public LEventHandler
			,	public LDropArea
{
private:
	typedef	LEventHandler	inheritEventHandler;
public:
	enum {class_ID = kPPCID_DataDragEventHandler};

				//	Maintenance
						LDataDragEventHandler();
	virtual				~LDataDragEventHandler();
	virtual ClassIDT	GetClassID() const {return class_ID;}
#ifndef	PP_No_ObjectStreaming
	virtual void		WriteObject(LStream &inStream) const;
	virtual void		ReadObject(LStream &inStream);
#endif
	virtual	void		Reset(void);
	virtual LDataDragTask *	MakeDragTask(LDataDragEventHandler *inHandler, Boolean inHandlerIsSource = false);
	
	virtual Boolean		GetStartsDataDrags(void);
	virtual void		SetStartsDataDrags(Boolean inStartsDataDrags);
	virtual Boolean		GetReceivesDataDrags(void);
	virtual void		SetReceivesDataDrags(Boolean inReceivesDataDrags);
												
				//	New features -- data dragging
	virtual	void		DataDragDo(void);
	virtual	DataDragT	FindDragSemantic(void);
						//	Receiving handlers
	virtual void		DataDragMoveIn(void);
	virtual void		DataDragMoveOut(void);
	virtual void		DataDragTrackMove(void);
	
				//	Drag action creation
	virtual	LAction *	MakeCreateTask(void);
	virtual	LAction *	MakeCopyTask(void);
	virtual	LAction *	MakeMoveTask(void);
	virtual	LAction *	MakeLinkTask(void);
	virtual	LAction *	MakeOSpecTask(void);
protected:
				//	Implementation
	virtual void		NoteOverNewThingSelf(LManipulator *inThing);
	virtual LSelectableItem *	OverReceiverSelf(Point inWhere);
	virtual void		SetMouseBounds(const Rect &inBounds);
	virtual Boolean		DoCondition(EventConditionT inCondition);
	virtual DragTypeT	GetDragType(void);
	virtual Boolean		PtInBoundaryArea(Point inWhere);
	virtual void		PreScroll(Point inVector);
	virtual void		PostScroll(Point inVector);

				//	Implementation/linkage -- LDropArea
	virtual Boolean		PointInDropArea(Point inGlobalPt);
	virtual void		CheckDragRef(DragReference inDragRef);
	virtual void		DoDragSendData(FlavorType inFlavor,
								ItemReference inItemRef,
								DragReference inDragRef);
	virtual void		DoDragReceive(DragReference inDragRef);
	virtual Boolean		DragIsAcceptable(DragReference inDragRef);
	virtual void		EnterDropArea(DragReference inDragRef,
								Boolean inDragHasLeftSender);
	virtual void		LeaveDropArea(DragReference inDragRef);
	virtual void		InsideDropArea(DragReference inDragRef);
	virtual void		FocusDropArea(void);
	virtual void		HiliteDropArea(DragReference inDragRef);

	static	DragSendDataUPP			sDragSendDataUPP;
	static	Boolean					sPreScrollWasHilited;
	static	LDataDragEventHandler	*sReceiveHandler;
	
				//	Perhaps more of these could be static.
	LSelectableItem			*mReceiver;
	LSelectableItem			*mPossibleReceiver;
	EventStateT				mPreDragState;
	Boolean					mStartsDataDrags,
							mReceivesDataDrags;
};

enum {
	evs_DataDragEventHandler_Count = evs_EventHandler_Count
};

