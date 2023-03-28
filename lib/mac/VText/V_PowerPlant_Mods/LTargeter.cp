//	===========================================================================
//	LTargeter.cp					©1994 Metrowerks Inc. All rights reserved.
//	===========================================================================
//
//	Class providing a "focus box" around a pane.  This is different than
//	LFocusBox because the box outline can be clicked on w/o changing the
//	selection in the contained pane.

#ifdef PowerPlant_PCH
#include PowerPlant_PCH
#endif

#include	<LTargeter.h>
#include	<UDrawingState.h>
#include	<LCommander.h>
#include	<LView.h>
#include	<LStream.h>
#include	<URegions.h>

#pragma	warn_unusedarg off

LTargeter::LTargeter()
{
	mIsTargeted = false;
	mTarget = NULL;
	mGrappledPane = NULL;
}

LTargeter::~LTargeter()
{
}

void	LTargeter::FinishCreateSelf(void)
{
	inherited::FinishCreateSelf();
}

void	LTargeter::AttachPane(
	LPane	*inPane)
{
	ThrowIfNULL_(inPane);
	mGrappledPane = inPane;
	
	//	Targeter has the same SuperView as its grappled Pane...
	LView	*super = mGrappledPane->GetSuperView();
	ThrowIfNULL_(super);
	PutInside(super);
	
	//	Size & locate targeter to surround its grappled Pane...
	SDimension16	hostSize;
	mGrappledPane->GetFrameSize(hostSize);
	ResizeFrameTo(hostSize.width + 6, hostSize.height + 6, false);
	SPoint32	hostLocation;		//   grappled Pane location in Port coords
	mGrappledPane->GetFrameLocation(hostLocation);
	SPoint32	imageLoc = {0, 0};	// Get Super Image location
	super->GetImageLocation(imageLoc);
	PlaceInSuperImageAt(hostLocation.h - imageLoc.h - 3,
						hostLocation.v - imageLoc.v - 3, false);
	
	//	Set frame bindings to that of the grappled pane and the grappled pane
	//	to follow the target...
	SBooleanRect	bindings,
					innerBinding = {true, true, true, true};
	mGrappledPane->GetFrameBinding(bindings);
	SetFrameBinding(bindings);
	mGrappledPane->SetFrameBinding(innerBinding);
	
	//	Make grappled pane a subpane of targeter...
	super->RemoveSubPane(mGrappledPane);
	AddSubPane(mGrappledPane);
	Refresh();
}

void	LTargeter::AttachTarget(LCommander *inTarget)
{
	mTarget = inTarget;
}

void	LTargeter::ClickSelf(const SMouseDownEvent &inMouseDown)
{
	if (mTarget) {
		LCommander::SwitchTarget(mTarget);
	}
}

void	LTargeter::DrawSelf()	
{
	Rect	frame;
	
	if (mIsTargeted) {
		if (CalcLocalFrameRect(frame)) {
			StColorPenState		saveState;
			
			StColorPenState::Normalize();
			PenSize(2, 2);
			FrameRect(&frame);
		}
	} else {
		if (CalcLocalFrameRect(frame)) {
			StColorPenState		saveState;
			StRegion			innerRgn,
								outerRgn;

			RectRgn(outerRgn, &frame);
			InsetRect(&frame, 2, 2);
			RectRgn(innerRgn, &frame);
			DiffRgn(outerRgn, innerRgn, outerRgn);
			EraseRgn(outerRgn);
		}
	}
}

void	LTargeter::ShowFocus(void)
{
	if (!mIsTargeted) {
		mIsTargeted = true;
		FocusDraw();
		DrawSelf();
	}
}

void	LTargeter::HideFocus(void)
{
	if (mIsTargeted) {
		mIsTargeted = false;
		FocusDraw();
		DrawSelf();
	}
}

