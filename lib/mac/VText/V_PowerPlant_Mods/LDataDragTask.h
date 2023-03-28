//	===========================================================================
//	LDataDragTask.h					©1994 Metrowerks Inc. All rights reserved.
//	===========================================================================

#pragma	once

#include	<LDataTube.h>

//	Different drag semantics that could be implied by dragging/option keys
typedef enum {
	dataDrag_Unknown,
	dataDrag_Copy,
	dataDrag_Move,
	dataDrag_Link,	//	As in a hot link -- publish and subscribe
	dataDrag_OSpec	//	As in copy the AEOM object specifier --
					//		could be another type of hot link.
} DataDragT;

//	Different drag functions depending on who/what is sending/receiving
//	(From the perspective of this handler.)
typedef enum {
	dataDragFunc_Unknown,		//	not yet determined
	dataDragFunc_Receive,		//	external app is dropping data in this handler
	dataDragFunc_Drop,			//	dropping to "ospec" ignorant location
	dataDragFunc_OSpecDrop,		//	dropping in "ospec" aware external app
								//		ie. the app set the drop location
	dataDragFunc_Drag			//	"ospec" aware "drag" between LDataDragEventHandlers
								//		within this app
} DataDragFuncT;

class	LDataDragEventHandler;
class	LAction;
class	LDragTube;
class	LSelectableItem;

class LDataDragTask {
public:
	static LDataDragTask	*sActiveTask;	//	There can be only one.

						LDataDragTask(
								LDataDragEventHandler	*inHandler,
								Boolean 				inHandlerIsSource = false);
	virtual				~LDataDragTask();
	virtual void		CheckTube(void);

	virtual void		NoteTransfer(void);
	virtual void		DoTransfer(void);
	
	virtual LAction	*	MakeDragAction(void);
	
	virtual	void		SetReceiver(LSelectableItem *inReceiver);
	virtual void		NoteReceiveHandler(LDataDragEventHandler *inHandler);
	virtual	LDataDragEventHandler	*	GetReceiveHandler(void) const;

	LDataDragEventHandler	*mSourceHandler;
	LSelectableItem			*mReceiver;
	
	DataDragT				mDragSemantic;
	DataDragFuncT			mDragFunction;
	
	LDragTube				*mDragTube;

	DragReference			mDragRef;
protected:
	LDataDragEventHandler	*mReceiveHandler;
	LAction					*mDragAction;
	Boolean					mNotified;
	Boolean					mDataTransferred;	//	Just for error detection
};