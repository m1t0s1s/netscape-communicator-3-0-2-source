//	===========================================================================
//	LManipulator.cp					�1994 Metrowerks Inc. All rights reserved.
//	===========================================================================

#ifdef PowerPlant_PCH
#include PowerPlant_PCH
#endif

#include	"LManipulator.h"

#pragma	warn_unusedarg off

//	===========================================================================
//	Maintenance

LManipulator::LManipulator()
{
}

LManipulator::~LManipulator()
{
}

//	===========================================================================
//	Query (override)

Boolean	LManipulator::PointInRepresentation(Point inWhere) const
/*
	Return true if inWhere (local coords) lies in the selectable representation
	of the object.
	
	OVERRIDE.
*/
{
	return false;
}

//	===========================================================================
//	Visual (override)

void	LManipulator::DrawSelf(void)
{
	Assert_(false);
}

void	LManipulator::UnDrawSelf(void)
{
	DrawSelf();
}

//	===========================================================================
//	Manipulation behavior (override)

void	LManipulator::MouseDown(void)
{
}

void	LManipulator::MouseUp(void)
{
}

void	LManipulator::MouseEnter(void)
{
}

void	LManipulator::MouseExit(void)
{
}

Boolean	LManipulator::Dragable(void) const
{
	return true;
}

void	LManipulator::DragStart(Point inStart)
{
	//	Draw initial position.
	DrawDragFeedback(inStart);
}

void	LManipulator::DragMove(Point inOld, Point inNew)
{
	//	Remove feedback at last position
	UnDrawDragFeedback(inOld);
	
	//	Draw feedback at new mouse position
	DrawDragFeedback(inNew);
}

void	LManipulator::DragStop(Point inEnd)
{
	//	Remove feedback at last position
	UnDrawDragFeedback(inEnd);
}

void	LManipulator::DrawDragFeedback(Point inWhere)
{
//	Do something like:
/*
	StColorPenState	savePen;
	
	OffsetRgn(mDragOutline, inWhere.h, inWhere.v);
	ThrowIfOSErr_(QDError());

	Try_ {
		PenPat(&UQDGlobals::GetQDGlobals()->gray);
		ThrowIfOSErr_(QDError());
		
		FrameRgn(mDragOutline);
		ThrowIfOSErr_(QDError());
		
		OffsetRgn(mDragOutline, -inWhere.h, -inWhere.v);
		ThrowIfOSErr_(QDError());
	} Catch_ (t_AnyType) {
		OffsetRgn(mDragOutline, -inWhere.h, -inWhere.v);
		Throw_(t_AnyType);
	} EndCatch_;
*/
}

void	LManipulator::UnDrawDragFeedback(Point inWhere)
{
	DrawDragFeedback(inWhere);
}
