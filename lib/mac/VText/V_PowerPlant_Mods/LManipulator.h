//	===========================================================================
//	LManipulator.h					©1994 Metrowerks Inc. All rights reserved.
//	===========================================================================

#pragma	once

#include	<LSharable.h>

//	The following indicate objects w/ similar behavior but the object types
//	shouldn't be considered subclasses of one another.
//	C++ RTTI would be a better solution.
typedef enum {
	kManipulator,
	kSelectableItem,
	kSelection
} ManipT;

class	LManipulator
			:	public virtual LSharable
{
public:
				//	Maintenance
						LManipulator();
	virtual				~LManipulator();
	virtual ManipT		ItemType(void) const {return kManipulator;}

				//	Query (override)
	virtual Boolean		PointInRepresentation(Point inWhere) const;
	
				//	Visual (override)
	virtual void		DrawSelf(void);
	virtual void		UnDrawSelf(void);
	
				//	Manipulation behavior (likely override)
	virtual void		MouseDown(void);
	virtual void		MouseUp(void);
	virtual void		MouseEnter(void);
	virtual void		MouseExit(void);	
	virtual Boolean		Dragable(void) const;
	virtual	void		DragStart(Point inStart);
	virtual void		DragMove(Point inOld, Point inNew);
	virtual void		DragStop(Point inEnd);
	virtual void		DrawDragFeedback(Point inWhere);
	virtual void		UnDrawDragFeedback(Point inWhere);
};
