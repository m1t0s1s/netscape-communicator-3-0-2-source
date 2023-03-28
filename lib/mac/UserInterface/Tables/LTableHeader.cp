/*
	LTableHeader.cp

	© 1996 Netscape Communications, Inc.
	
	Created 3/18/96 - Tim Craycroft (tj, x3672)

	To Do:
	
*/


// PowerPlant
#include <URegions.h>

// Mac Lib
#include "UserInterface_Defines.h"
#include "UGraphicGizmos.h"
#include "LTableHeader.h"
#include "CSimpleDividedView.h"		



// This is the data we store for each column
//
// All column data is stored in an array kept in a handle
//
typedef struct
{

	PaneIDT	paneID;				// header pane id, also column sort id
	SInt16	columnWidth;		// width in pixels or % (fixed-width columns are always pixels)
	SInt16	columnPosition;		// horizontal position within the header view
		
} ColumnData;


// Move me somewhere useful !!!
static void	LocalToGlobalRect(Rect& r);



/*
   Save the current port, set to a different one, set it back on exit 

   This should also be moved somewhere useful 
*/
class StCurrentPort 
{
public:
	StCurrentPort(GrafPtr inPort) 	{ mPort = UQDGlobals::GetCurrentPort(); SetPort((GrafPtr)inPort); }
	~StCurrentPort() 				{ SetPort(mPort); }

private:
	GrafPtr	mPort;
};



/* 
	LTableHeader::CreateTableHeaderStream
*/
LTableHeader *
LTableHeader::CreateTableHeaderStream(LStream *inStream)
{
	return new LTableHeader(inStream);
}



/* 
	LTableHeader::LTableHeader
	
*/
LTableHeader::LTableHeader( LStream *inStream )
	: LView(inStream), LBroadcaster()
{
	SInt16	columnListResID;
	Handle	columnData;
	ResIDT theBevelTraitsID;
	
	*inStream >> mHeaderFlags;
		
	*inStream >> theBevelTraitsID;
	UGraphicGizmos::LoadBevelTraits(theBevelTraitsID, mSortedBevel);
	*inStream >> theBevelTraitsID;
	UGraphicGizmos::LoadBevelTraits(theBevelTraitsID, mUnsortedBevel);
	
	*inStream >> mReverseModifiers;
	
	// Load up the column data	
	*inStream >> columnListResID;
	columnData = ::GetResource('Cols', columnListResID);
	ThrowIfNULL_(columnData);
	mColumnData = NULL;	
	::DetachResource((Handle)columnData);

	// LHandleStream will dispose of columnData
	LHandleStream streamMe(columnData);	
	ReadColumnState(&streamMe, false);
}
	

/* 
	LTableHeader::~LTableHeader	
*/
LTableHeader::~LTableHeader()
{
	if (mColumnData != NULL) ::DisposeHandle( (Handle) mColumnData);
	mColumnData = NULL;
}



/* 
	LTableHeader::FinishCreateSelf	
*/
void
LTableHeader::FinishCreateSelf()
{
	// position the column header panes
	LView::FinishCreateSelf();
	PositionColumnHeaders(true);		
}


/* 
	LTableHeader::DrawSelf	
	
	Draw each column, draw any space to the right of the rightmost
	column, and draw the column-hiding widget, if necessary.
*/
void
LTableHeader::DrawSelf()
{
	int			i;
	Rect		r;
				
	// Draw the column backgrounds and frames, carrying r.right
	// out of the loop so we know where the rightmost column ends
	
	for (i=1; i<=mLastVisibleColumn; i++)
	{
		Rect		sect;
		RgnHandle	updateRgn;
	
		LocateColumn(i, r);
		
		// Checking against the update rgn is necessary because we sometimes
		// call Draw(), with a valid rgn to draw only particular columns
		// in order to reduce flicker.
		//
		// If we didn't do this check we would end up erasing other columns,
		// as Draw would not redraw their header subpanes
		//
		updateRgn = GetLocalUpdateRgn();
		if ( updateRgn == NULL || ::SectRect(&r, &((**updateRgn).rgnBBox), &sect))
			DrawColumnBackground(r, GetColumnPaneID(i) == GetColumnPaneID(mSortedColumn));
			
	}
	
	
	// If a fixed-width column is rightmost, 
	// we may have some space to fill-in between
	// it and the right edge of the view
	
	UInt16	headerWidth = GetHeaderWidth();
	if (headerWidth > r.right )
	{
		r.left = r.right;
		r.right = headerWidth;
		
		DrawColumnBackground(r, false);
	}
	
	
	//
	// Draw the column hiding widget
	//
	if (CanHideColumns())
	{
		Rect 	frame;
		SInt16	iconID;
		
		CalcLocalFrameRect(frame);

		frame.left = frame.right - kColumnHidingWidgetWidth;
		DrawColumnBackground(frame, false);

		frame.top += (mFrameSize.height - kColumnHidingWidgetWidth) >> 1;
		frame.bottom = frame.top + kColumnHidingWidgetWidth;
		
		
		if (mLastVisibleColumn == 1) 
		{	
			if (mColumnCount == 1) {
				iconID = kColumnHiderDisabledIcon;
			}
			else {
				iconID = kColumnHiderHideDisabledIcon;
			}
		}
		else if (mLastVisibleColumn == mColumnCount) {
			iconID = kColumnHiderShowDisabledIcon;
		}
		else {	
			iconID = kColumnHiderEnabledIcon;	
		}		
		
		PlotIconID(&frame, atAbsoluteCenter, kTransformNone, iconID);
	}	
}


/* 
	LTableHeader::DrawColumnBackground	
	
	Bevel and fill the column header rectangle. 
*/
void
LTableHeader::DrawColumnBackground(	const Rect 	& inWhere,
									Boolean	  	inSortColumn)
{
	Rect r = inWhere;
	const SBevelColorDesc& colors = (inSortColumn) ? mSortedBevel : mUnsortedBevel;
	 
	UGraphicGizmos::BevelRect(r, 1, colors.topBevelColor, colors.bottomBevelColor);

	::PmForeColor(colors.fillColor);
	::InsetRect(&r,1,1);
	::PaintRect(&r);
}									


/* 
	LTableHeader::AdjustCursor	
	
	Switch the cursor if it's over a column resize area. 
	
	We only give subviews a chance to change the cursor
	if we're not over a resize area.
*/
void
LTableHeader::AdjustCursor(	Point inPortPt,
							const EventRecord &inMacEvent)
									
{
#pragma unused(inMacEvent)
	
	Point		localPt = inPortPt;
	UInt16		column;
	
	PortToLocalPoint(localPt);
	
	if ( IsInHorizontalSizeArea(localPt, column)) {
		::SetCursor(*GetCursor(kHorizontalResizeCursor));
	}
	else { 
		LView::AdjustCursor(inPortPt, inMacEvent);
	}
}


/* 
	LTableHeader::IsInHorizontalSizeArea	
	
	Returns true if the given local pt is in a column-resize area. 
	Also returns the index of the column to the left of that area
	(the column to be resized).
	
*/
Boolean
LTableHeader::IsInHorizontalSizeArea( 	Point 		inLocalPt, 
										UInt16 & 	outLeftColumn)
{
	ColumnData	*cData = *mColumnData;
	SInt16		left, right;
	int			i;
	
	left 	= inLocalPt.h + 2;
	right 	= inLocalPt.h - 2;
	
	// start at the division between the first and second columns
	for (i=2; i<= mLastVisibleColumn; i++)
	{
		cData++;
		
		if (left 	>= cData->columnPosition &&
			right 	<= cData->columnPosition)
		{
			outLeftColumn = i-1;
			
			return CanColumnResize(outLeftColumn);
		}
	}
	
	outLeftColumn = 0xFFFF;	
	return false;
}



/* 
	LTableHeader::Click	
	
	Handles all clicks in the view.  We never pass the click
	down to subviews. 
	
	Handles changing sort column, resizing columns, moving 
	columns, and showing/hiding columns.
	
*/
void
LTableHeader::Click(SMouseDownEvent &inEvent)
{
	UInt16	column;
	
	// LPane::Click ususally does this...
	PortToLocalPoint(inEvent.whereLocal);
	
	// Handle column resize
	if ( IsInHorizontalSizeArea(inEvent.whereLocal, column)) {
	
		TrackColumnResize(inEvent, column);
	}
	else
	{
		// Handle clicks in the column hiding widget
		if (CanHideColumns() && (inEvent.whereLocal.h > GetHeaderWidth())) 
		{
			Boolean	doShow;
			
			// The right arrow hides the rightmost column and the left arrow
			// shows it.
			doShow = !(inEvent.whereLocal.h >= (mFrameSize.width - (kColumnHidingWidgetWidth/2)));
			
			if (doShow && (mLastVisibleColumn < (mColumnCount)) ||
				!doShow && (mLastVisibleColumn > 1)) 
			{	
				ShowHideRightmostColumn(doShow);
			}
		}
		else
		{
			// Click in column header, either drag it,  
			// or set it to be the sort column
			
			column = FindColumn(inEvent.whereLocal);
			
			if (column != 0)
			{
				if (::WaitMouseMoved(inEvent.macEvent.where)) {
					TrackColumnDrag(inEvent, column);
				}
				else {
					SetSortedColumn(	column, 
										(inEvent.macEvent.modifiers & mReverseModifiers) != 0);
				}
			}
		}
	}
}



/* 
	LTableHeader::TrackColumnDrag	
	
	Tracks the dragging of column inColumn, moving it,
	and redrawing as necessary.
	
*/
void
LTableHeader::TrackColumnDrag(	const SMouseDownEvent &	inEvent,
								UInt16					inColumn	)
{
	Point		dropPoint;	
	StRegion	dragRgn;
	Rect		dragRect;
	Rect		dragBounds;
	Rect		dragSlop;
	
	if (!FocusDraw()) return;
	
	// Create the rgn we're dragging
	//
	// This is virtualize so subclasses can make a 
	// more interesting rectangle...
	//
	ComputeColumnDragRect(inColumn, dragRect);
	::RectRgn(dragRgn, &dragRect);
	
	// Compute the bounds and slop of the drag
	CalcLocalFrameRect(dragBounds);
	LocalToGlobalRect(dragBounds);	
	dragSlop = dragBounds;
	::InsetRect(&dragSlop, -5, -5);
	
	// drag me
	{
		StCurrentPort	portState(LMGetWMgrPort());
		const Rect 		wideopen = { 0xFFFF, 0xFFFF, 0x7FFF, 0x7FFF };
		
		ClipRect(&wideopen);
		*(long*)(&dropPoint) = ::DragGrayRgn(
									dragRgn, 
									inEvent.macEvent.where, 
									&dragBounds,
									&dragSlop, 
									kVerticalConstraint,
									NULL);
	
	}
	
	if ((dropPoint.h != (SInt16) 0x8000 || dropPoint.v != (SInt16) 0x8000)	&&
		(dropPoint.h != 0 || dropPoint.v != 0))
	{
		UInt16	dropColumn;		
	
		// find the column over which we finished the drag
		dropPoint.h += inEvent.whereLocal.h;
		dropPoint.v += inEvent.whereLocal.v;
		
		dropColumn = FindColumn(dropPoint);
		
		// move the column we were dragging
		if (dropColumn != 0 && inColumn != dropColumn) {
			MoveColumn(inColumn, dropColumn);	
		}
	}	
}								



/* 
	LTableHeader::TrackColumnResize	
	
	Tracks resizing of a column, redrawing as necessary.
	
*/
void
LTableHeader::TrackColumnResize(	const SMouseDownEvent	&inEvent,
									UInt16					inLeftColumn	)
{
	Rect			dragBoundsRect;
	Rect			slopRect;
	StRegion		dragRgn;
	Rect			dragRect;
		
	Assert_(inLeftColumn >= 1 && inLeftColumn <= mLastVisibleColumn);

	if (!FocusDraw()) return;
	
	
	// Compute the actual region we're dragging
	//
	// This is virtualized so subclasses can reflect the size-change
	// in some other view's space (like down a table column)
	//
	ComputeResizeDragRect(inLeftColumn, dragRect);
	::RectRgn(dragRgn, &dragRect);	
	
	// Compute drag bounds and slop
	CalcLocalFrameRect(dragBoundsRect);
	dragBoundsRect.left = (inLeftColumn != 1) ? GetColumnPosition(inLeftColumn) + 4 : 4;
	
	LocalToGlobalRect(dragBoundsRect);
	slopRect = dragBoundsRect;
	InsetRect(&slopRect, -5, -5);
	
	
	// drag me
	Point dragDiff;
	
	{
		StCurrentPort 	portState(LMGetWMgrPort());
		const Rect 		wideopen = { 0xFFFF, 0xFFFF, 0x7FFF, 0x7FFF };
		
		::ClipRect(&wideopen);


		//
		// Track the drag, then resize the column if it was a valid drag
		//
		
		
		*(long *)&dragDiff = ::DragGrayRgn(
									dragRgn, 
									inEvent.macEvent.where, 
									&dragBoundsRect,
									&slopRect, 
									kVerticalConstraint,
									NULL);
	}
								
	
	if ((dragDiff.h != (SInt16) 0x8000 || dragDiff.v != (SInt16) 0x8000)	&&
		(dragDiff.h != 0     || dragDiff.v != 0 ))
	{
		ResizeColumn(inLeftColumn, dragDiff.h);
	}
	

}


/* 
	LTableHeader::ResizeColumn	
	
	Resize inLeftColumn by inLeftColumnDelta.
	
	Columns to the right of inLeftColumn are resized
	proportionally.
	
*/
void
LTableHeader::ResizeColumn(	UInt16	inLeftColumn,
							SInt16	inLeftColumnDelta	)
{
	SInt16 		newSpace, oldSpace;
	ColumnData*	leftData;
	
	leftData = &((*mColumnData)[inLeftColumn-1]);
	leftData->columnWidth   += inLeftColumnDelta;

	newSpace = GetHeaderWidth() - (leftData->columnPosition + leftData->columnWidth);
	oldSpace = newSpace + inLeftColumnDelta;
	
	// redistribute the space among the columns to the right and then redraw
	RedistributeSpace(inLeftColumn+1, mLastVisibleColumn, newSpace, oldSpace);
	RedrawColumns(inLeftColumn, mLastVisibleColumn);
}



/* 
	LTableHeader::ShowHideRightmostColumn	
	
	Show or hide the rightmost column.
	
	mLastVisibleColumn is the rightmost visible column.
	
*/
void
LTableHeader::ShowHideRightmostColumn(Boolean inShow)
{
	Assert_((inShow && mLastVisibleColumn < (mColumnCount)) ||
			(!inShow && mLastVisibleColumn != 1));
					
	if (inShow) {
		mLastVisibleColumn++;
	}	
	
	// Show or hide the header pane
	LPane *header = GetColumnPane(mLastVisibleColumn);
	if (header != NULL)
	{
		if (inShow) {
			header->Show();
			header->DontRefresh(true);	// no flicker
		}
		else
		{
			header->Hide();
			header->DontRefresh(true);	// no flicker, please	
		}
	}


	// Redistribute space among visible columns and redraw
	
	UInt16	newWidth, oldWidth;
	UInt16	headerWidth 	= GetHeaderWidth();
	UInt16  adjustedWidth	= headerWidth - GetColumnWidth(mLastVisibleColumn);
	
	if (inShow)
	{	
		newWidth = adjustedWidth;
		oldWidth = headerWidth;
	}
	else
	{
		mLastVisibleColumn--;
		newWidth = headerWidth;
		oldWidth = adjustedWidth;
	}
	
	RedistributeSpace(1, mLastVisibleColumn, newWidth, oldWidth);
	RedrawColumns(1, mLastVisibleColumn);
	
}



/* 
	LTableHeader::RedistributeSpace	
	
	Given old and new amounts of horizontal space, this
	resizes a range of columns such that they each have
	the same percentage of space in the new space as they
	did in the old space. 	
	
	Fixed-width columns are not resized, of course.
	
	Also recomutes column positions and repositions the
	column header panes.
*/
void
LTableHeader::RedistributeSpace(	UInt16	inFromCol,
									UInt16	inToCol,
									SInt16	inNewSpace,
									SInt16	inOldSpace)
{
	SInt16			fixedWidth;
	
	//
	// fixed width columns don't change, so we only want to
	// distribute the non-fixed space.
	//
	fixedWidth  = GetFixedWidthInRange(inFromCol, inToCol);
	inNewSpace -= fixedWidth;
	inOldSpace -= fixedWidth;
	
	
	int 		i;
	ColumnData*	cData = &(*mColumnData)[inFromCol-1];

	for (i=inFromCol; i<=inToCol; i++, cData++)
	{
		if (CanColumnResize(cData))
		{
			// resize this column to take the same proportional amount of space
			
			SInt32	scratch = (inNewSpace * cData->columnWidth) / inOldSpace;
			
			cData->columnWidth = (SInt16) scratch;
		}
	}
	
	ComputeColumnPositions();
	PositionColumnHeaders(false);
}									
									
		
/* 
	LTableHeader::GetFixedWidthInRange	
	
	Returns the horizontal space taken by all fixed-width
	columns in a given column range (inclusive).
	
*/
SInt16
LTableHeader::GetFixedWidthInRange(	UInt16	inFromCol,
									UInt16	inToCol	)
{
	ColumnData*	cData 		= &(*mColumnData)[inFromCol-1];
	SInt16		fixedWidth 	= 0;
	int			i;

	if (inToCol > mLastVisibleColumn) {
		inToCol = mLastVisibleColumn;
	}
	
	for (i=inFromCol; i<=inToCol; i++, cData++)
	{
		if (!CanColumnResize(cData)) {
			fixedWidth += cData->columnWidth;
		}
	}
	
	return fixedWidth;
}															

/* 
	LTableHeader::GetHeaderWidth	
	
	Returns the width of all the visible columns,
	not including the column hiding widget.
	
*/
UInt16
LTableHeader::GetHeaderWidth() const
{
	UInt16	width = mImageSize.width;
	
	if (CanHideColumns())
		width -= kColumnHidingWidgetWidth;
		
	return width;
}
						

	
/* 
	LTableHeader::MoveColumn	
	
	If inColumn > inMoveTo, moves inColumn BEFORE inMoveTo.
	If inColumn < inMoveTo, moves inColumn AFTER inMoveTo.
	
*/
void
LTableHeader::MoveColumn(UInt16 inColumn, UInt16 inMoveTo)
{
	ColumnData	*	cData 		= *mColumnData,
				*	shiftFrom, 
				*	shiftTo;
	ColumnData		swap;
	Boolean			adjustedSort = false;
	
 
	// save off the data for the column we're moving
	::BlockMoveData(&(cData[inColumn-1]), &swap, sizeof(ColumnData));
	
	// Handle case where we're moving the sorted column		
	if (mSortedColumn == inColumn)
	{
		adjustedSort 	= true;
		mSortedColumn 	= inMoveTo;
	}
	
	//
	// Figure out which direction we're moving the column
	// and how we need to shift the data in the columndata array
	//
	SInt32 delta = inColumn - inMoveTo;
	if (delta > 0)
	{
		shiftFrom	= &(cData[inMoveTo-1]);
		shiftTo		= shiftFrom + 1;
		
		if (!adjustedSort && mSortedColumn >= inMoveTo && mSortedColumn < inColumn)
			mSortedColumn += 1;
	}
	else
	{
		delta 		= -delta;
		shiftTo 	= &(cData[inColumn-1]);
		shiftFrom	= shiftTo + 1;
		
		if (!adjustedSort && mSortedColumn > inColumn && mSortedColumn <=inMoveTo)
			mSortedColumn -= 1;
	}
	
	// Shift the data in the columnData array and then copy in the data
	// of the column we're moving
	::BlockMoveData(shiftFrom, shiftTo, (delta*sizeof(ColumnData)));
	::BlockMoveData(&swap, &(cData[inMoveTo-1]), sizeof(ColumnData));
	
	// resposition everything
	ComputeColumnPositions();
	PositionColumnHeaders(false);
	
	
	if (inColumn > inMoveTo) {
		RedrawColumns(inMoveTo, inColumn);
	}
	else {
		RedrawColumns(inColumn, inMoveTo);
	}
}
	
/* 
	LTableHeader::ComputeColumnPositions	
	
	Computes the horizontal position of each column, assuming 
	each column's width is valid in pixels.
	
	If the rightmost visible column doesn't fill the column area, 
	it will be extended to do so, UNLESS it is a fixed-width column.
	
*/
void
LTableHeader::ComputeColumnPositions()
{
	ColumnData	*cData;
	int			i;
	SInt16		accumulator = 0;
	UInt16		headerWidth;
	
	// Compute each visible column's position
	cData = *mColumnData;
	for (i=1; i<=mLastVisibleColumn; i++, cData++)
	{
		cData->columnPosition = accumulator;
		accumulator += cData->columnWidth;
	}
	
	// If the last column is resizable, expand or shrink it
	// as necessary to reach the edge of the header
	cData--;
	if (CanColumnResize(cData))
	{
		headerWidth = GetHeaderWidth();
		if ( (cData->columnPosition + cData->columnWidth) != headerWidth) {
			cData->columnWidth = headerWidth - cData->columnPosition;
		}
	}
}						

/* 
	LTableHeader::ComputeResizeDragRect	
	
	Computes rectangle to be dragged when resizing a column.
	
*/
void
LTableHeader::ComputeResizeDragRect(	UInt16		inLeftColumn,
										Rect	&	outDragRect		)
{
	outDragRect.top 	= 	0;
	outDragRect.bottom	=	mFrameSize.height + 100;
	outDragRect.left	=	GetColumnPosition(inLeftColumn+1) - 1;
	outDragRect.right	=	outDragRect.left + 1;
	
	LocalToGlobalRect(outDragRect);
}										

/* 
	LTableHeader::ComputeColumnDragRect	
	
	Computes rectangle to be dragged when moving a column.
	
*/
void
LTableHeader::ComputeColumnDragRect(	UInt16		inColumn,
										Rect	&	outDragRect		)
{
	LocateColumn(inColumn, outDragRect);
	outDragRect.bottom += 100;	
	LocalToGlobalRect(outDragRect);
}										


/* 
	LTableHeader::SetSortedColumn	
	
*/
void
LTableHeader::SetSortedColumn(	UInt16 	inColumn, 
								Boolean	inReverse,
								Boolean inRefresh	)
{
	if ( ! CanColumnSort(inColumn) )
		return;
		
	if (inRefresh && inColumn != mSortedColumn && FocusDraw())
	{	
		
		SInt16	oldSorted = mSortedColumn;
		
		mSortedColumn = inColumn;
		RedrawColumns(oldSorted, oldSorted);
		RedrawColumns(mSortedColumn, mSortedColumn);
	}
	
	mSortedColumn = inColumn;
	
	
	// Tell any listeners that the sort column changed
	TableHeaderSortChange	changeRecord;
	
	changeRecord.sortColumnID 	= GetColumnPaneID(mSortedColumn);
	changeRecord.sortColumn		= mSortedColumn;
	changeRecord.reverseSort	= inReverse;	
	BroadcastMessage(msg_SortedColumnChanged, &changeRecord);
}


/* 
	LTableHeader::RedrawColumns	
	
	Immediately redraws the given column range.
	We don't do update events so we can avoid the flicker
	of update rgn being erased to white.
	
*/
void
LTableHeader::RedrawColumns(UInt16 inFrom, UInt16 inTo)
{
	StRegion rgn;
	Rect	 r;
	
	// Compute the top-left of the refresh area
	LocateColumn(inFrom, r);
	LocalToPortPoint(topLeft(r));
	
	// Get the bottom right of the refresh area
	if (inFrom == inTo) {
		LocalToPortPoint(botRight(r));
	}
	else
	{
		Rect	right;
		
		if (inTo <= mLastVisibleColumn)
		{
			LocateColumn(inTo, right);
			LocalToPortPoint(botRight(right));	
		}
		else
		{
			right.right = GetHeaderWidth();
			LocalToPortPoint(botRight(right));
		}
		
		r.bottom = right.bottom;
		r.right  = right.right;
	}

	// Redraw
	::RectRgn(rgn, &r);	
	Draw(rgn);
}


/* 
	LTableHeader::LocateColumn	
	
	Computes the bounding rect of the given column 
	in local coordinates.
	
*/
void
LTableHeader::LocateColumn( UInt16 inColumn,
	 						Rect & outWhere) const
{
	outWhere.top 	= 0;
	outWhere.bottom = mFrameSize.height;
	
	outWhere.left 	= GetColumnPosition(inColumn);
	outWhere.right	= outWhere.left + GetColumnWidth(inColumn);
}


/* 
	LTableHeader::FindColumn	
	
	Given a local point, returns the index of the
	column containing the point.  Returns 0 for no
	column.
*/
UInt16
LTableHeader::FindColumn( Point inLocalPoint ) const
{
	int	i;

	if (inLocalPoint.h >= 0 && inLocalPoint.h <= GetHeaderWidth())
	{
		for (i=mLastVisibleColumn; i>0; i--)
		{
			if (inLocalPoint.h >= GetColumnPosition(i))
				return i;
		}
	}
		
	return 0;
}



/* 
	LTableHeader::InvalColumn	
	
	Invalidates a column header area.
	
*/
void
LTableHeader::InvalColumn( UInt16 inColumn)
{
	if (FocusDraw())
	{
		Rect	r;
		
		LocateColumn(inColumn, r);
		LocalToPortPoint(topLeft(r));
		LocalToPortPoint(botRight(r));
		InvalPortRect(&r);
	}
}


/* 
	LTableHeader::PositionColumnHeaders	
	
	Positions all the column header panes to be aligned
	with the current column positions.
	
*/
void
LTableHeader::PositionColumnHeaders(Boolean inRefresh)
{
	int	i;
	
	for (i=1; i<=mLastVisibleColumn; i++)
		PositionOneColumnHeader(i, inRefresh);
}


/* 
	LTableHeader::PositionOneColumnHeader	
	
	Positions a column header pane to be aligned with the its 
	column position.
	
	The column header pane is sized to the width of the column. 
	Its vertical position is not changed.
	
*/
void
LTableHeader::PositionOneColumnHeader( UInt16 inColumn, Boolean inRefresh)
{
	Rect		columnBounds;
	LPane	*	headerPane;
	
	LocateColumn(inColumn, columnBounds);
	headerPane = GetColumnPane(inColumn);
	
	// Position the pane horizontally within the column
	if (headerPane != NULL)
	{
		SPoint32		paneLocation;
		SDimension16	paneSize;
		Int32			paneWidth;
		Int32			horizDelta;
		Boolean			doResize = true;
				
		headerPane->GetFrameSize(paneSize);
		headerPane->GetFrameLocation(paneLocation);
		
		horizDelta 	= (columnBounds.left + kColumnMargin) - paneLocation.h + mFrameLocation.h;
		paneWidth	= columnBounds.right - (columnBounds.left + (kColumnMargin*2));
				
		headerPane->ResizeFrameTo(paneWidth, paneSize.height, inRefresh);	
		headerPane->MoveBy( horizDelta, 0, inRefresh);										
	}	
}




/* 
	LTableHeader::GetColumnPaneID	
	
	Returns the Pane id of the given column's header-pane.
	
	Returns 0 if there is no header-pane.
	
*/
PaneIDT	
LTableHeader::GetColumnPaneID(UInt16 inColumn) const
{
	Assert_(inColumn > 0 && inColumn <= mColumnCount);
	return ( (*mColumnData)[inColumn-1].paneID);
}

/* 
	LTableHeader::GetColumnPane	
	
	Returns the given column's header-pane.	
*/
LPane *	
LTableHeader::GetColumnPane(UInt16 inColumn)
{
	LView	*super = GetSuperView();
	PaneIDT	id;
	
	Assert_(inColumn > 0 && inColumn <= mColumnCount);
	Assert_(super != NULL);
	
	id = GetColumnPaneID(inColumn);
	
	if (id != 0) {
		return super->FindPaneByID(id);
	}
	else {
		return NULL;
	}
}

/* 	LTableHeader::GetColumnWidth	*/
SInt16	
LTableHeader::GetColumnWidth(UInt16 inColumn) const
{
	Assert_(inColumn > 0 && inColumn <= mColumnCount);
	return ( (*mColumnData)[inColumn-1].columnWidth);
}

/*	LTableHeader::GetColumnPosition	*/
SInt16	
LTableHeader::GetColumnPosition(UInt16 inColumn) const
{
	Assert_(inColumn > 0 && inColumn <= mColumnCount);
	return ( (*mColumnData)[inColumn-1].columnPosition);
}

/* 	LTableHeader::CanColumnResize	*/
Boolean	
LTableHeader::CanColumnResize(UInt16 inColumn) const
{
	Assert_(inColumn > 0 && inColumn <= mColumnCount);
	return ( ((*mColumnData)[inColumn-1].flags & kColumnDoesNotResize) == 0);
}

/*  LTableHeader::CanColumnResize	*/
Boolean	
LTableHeader::CanColumnResize(const ColumnData *inColumn) const
{
	return ( (inColumn->flags & kColumnDoesNotResize) == 0);	
}

/*  LTableHeader::CanColumnSort	*/
Boolean	
LTableHeader::CanColumnSort(UInt16 inColumn) const
{
	Assert_(inColumn > 0 && inColumn <= mColumnCount);
	return ( ((*mColumnData)[inColumn-1].flags & kColumnDoesNotSort) == 0);
}


/* 	LTableHeader::GetSortedColumn	*/
UInt16	
LTableHeader::GetSortedColumn( PaneIDT	&outHeaderPane) const
{ 
	outHeaderPane = GetColumnPaneID(mSortedColumn);
	return mSortedColumn; 
};


/* 	LTableHeader::CanHideColumns	*/
Boolean
LTableHeader::CanHideColumns() const
{
	return ((mHeaderFlags & kHeaderCanHideColumns) == kHeaderCanHideColumns);	
}



/* 
	LTableHeader::WriteColumnState	
	
	Writes out the current column state with column
	widths stored as percentages (except for fixed-width)
	columns.
*/
void
LTableHeader::WriteColumnState( LStream * inStream)	
{
	Assert_(mColumnData != NULL);
	
	*inStream << mColumnCount;
	*inStream << mLastVisibleColumn;
	*inStream << mSortedColumn;
	*inStream << (UInt16) 0;			// pad
	
	UInt32	dataSize = mColumnCount * sizeof(ColumnData);
	
	StHandleLocker((Handle)mColumnData);
	
	// write out the widths in percentages (except fixed-width cols)
	ConvertWidthsToPercentages();
	inStream->PutBytes(*mColumnData, dataSize);
	ConvertWidthsToAbsolute();
}

/* 
	LTableHeader::ReadColumnState
	
	Reads in column state for the header, converts the column widths
	to absolute pixel values, computes column positions, and optionally
	positions the column header panes.	
*/
void
LTableHeader::ReadColumnState( LStream * inStream, Boolean inMoveHeaders)		
{
	UInt16	ignore;
	
	*inStream >> mColumnCount;
	*inStream >> mLastVisibleColumn;
	*inStream >> mSortedColumn;
	*inStream >> ignore;
	

	UInt32	dataSize = mColumnCount * sizeof(ColumnData);
	
	if (mColumnData != NULL) ::DisposeHandle((Handle)mColumnData);
	mColumnData = (ColumnData **) ::NewHandle(dataSize);
	ThrowIfNULL_(mColumnData);
	
	StHandleLocker((Handle)mColumnData);
	inStream->GetBytes(*mColumnData, dataSize);


	const ColumnData *cData = *(mColumnData);
	
	// Compute the total of fixed-width columns
	mFixedWidth = GetFixedWidthInRange(1, mColumnCount);

	ConvertWidthsToAbsolute();
	ComputeColumnPositions();
	
	if (inMoveHeaders)
		PositionColumnHeaders(true);
}


/* 
	LTableHeader::ConvertWidthsToAbsolute
	
	Assumes all resizable column widths are in percentages.
	Converts all these widths to absolute pixel values as percentages
	of the available width for displaying the columns.
	
	If the last visible column is resizable and does not reach the right
	edge exactly, it will be extended or shrunk to meet it. (this is largely
	due to round-off errors in our math).	
*/
void
LTableHeader::ConvertWidthsToAbsolute()
{	
	UInt32		headerWidth;
	float		floatHeaderWidth;
	
	headerWidth 		= GetHeaderWidth() - mFixedWidth;
	floatHeaderWidth	= (float) headerWidth;
	
	int 		i;
	ColumnData*	cData 		= *mColumnData;
	UInt16		totalWidth	= 0;
	
	for (i=1; i<=mColumnCount; i++, cData++)
	{
		// fixed-width columns are already in pixels
		if (CanColumnResize(cData))
		{
			// do the math in float space otherwise the accumulated error
			// shows up by making the last column get bigger and bigger...
			//
			float	scratch;
			
			scratch 			= (((float)cData->columnWidth * floatHeaderWidth) / 100.0) + 1;
			cData->columnWidth 	= floor(scratch);
		}
			
		totalWidth += cData->columnWidth;
	}
	
	// make sure the last column, if resizable, reaches the edge exactly.
	if (CanColumnResize(mLastVisibleColumn))
		(*mColumnData)[mLastVisibleColumn-1].columnWidth += ((headerWidth+mFixedWidth) - totalWidth);
}


/* 
	LTableHeader::ConvertWidthsToPercentages
	
	Assumes all column widths are in pixels.
	
	Converts widths of all resizable columns to percentages
	of the available width for columns (GetHeaderWidth()).	
*/
void
LTableHeader::ConvertWidthsToPercentages()
{	
	int 		i;
	float		headerWidth = (float) (GetHeaderWidth() - mFixedWidth);
	ColumnData	*cData 		= *mColumnData;

	for (i=1; i<=mColumnCount; i++, cData++)
	{
		if (CanColumnResize(cData))
		{
			float	scratch;
				
			scratch				= (((float)(cData->columnWidth * 100)) / headerWidth) + 0.5;
			cData->columnWidth 	= floor(scratch);
		}
	}
}


/* 
	LTableHeader::ResizeFrameBy	
	
	When the frame is resized, we resize our image to match,
	first converting to percentages so that we can resize
	our columns proportionally.
*/
void
LTableHeader::ResizeFrameBy(	Int16	inWidthDelta,
								Int16	inHeightDelta,
								Boolean	inRefresh)
{
	ConvertWidthsToPercentages();
	
	ResizeImageBy(	inWidthDelta, 
					inHeightDelta, 
					inRefresh	);
								
	// Move the columns and column headers
	ConvertWidthsToAbsolute();
	ComputeColumnPositions();
	PositionColumnHeaders();
	
	LView::ResizeFrameBy(inWidthDelta, inHeightDelta, inRefresh);
}								


/* 
	LTableHeader::LocalToGlobalRect	
*/
static void
LocalToGlobalRect(Rect& r)
{
	::LocalToGlobal((Point *) &(r.top));
	::LocalToGlobal((Point *) &(r.bottom));
}
