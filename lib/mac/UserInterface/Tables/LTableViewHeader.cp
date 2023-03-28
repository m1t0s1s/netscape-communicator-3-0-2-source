/*
	LTableViewHeader.cp

	© 1996 Netscape Communications, Inc.
	
	Created 3/20/96 - Tim Craycroft

	To Do:
	
*/

#include "LTableViewHeader.h"


/*	LTableViewHeader::CreateTableViewHeaderStream	*/
LTableViewHeader *
LTableViewHeader::CreateTableViewHeaderStream(LStream *inStream)
{
	return new LTableViewHeader(inStream);
}

/*	LTableViewHeader::LTableViewHeader	*/
LTableViewHeader::LTableViewHeader(LStream *inStream)
	: LTableHeader(inStream)
{
	mTableViewID = 0;
	mTableView   = NULL;
	
	*inStream >> mTableViewID;
}


/*	
	LTableViewHeader::FinishCreateSelf
	
	Find the LTableView we're using	
*/
void
LTableViewHeader::FinishCreateSelf()
{
	LTableHeader::FinishCreateSelf();	
	
	LView	*super = GetSuperView();

	ThrowIfNULL_(super);	
	mTableView = (LTableView *) super->FindPaneByID(mTableViewID);
	
	Assert_(mTableView != NULL);
}



/*	
	LTableViewHeader::ComputeResizeDragRect
	
	Extend the bottom of the rect to the bottom of the table
*/
void
LTableViewHeader::ComputeResizeDragRect(UInt16 inLeftColumn, Rect &outDragRect)
{
	LTableHeader::ComputeResizeDragRect(inLeftColumn, outDragRect);
	outDragRect.bottom = GetGlobalBottomOfTable();	
}


/*	
	LTableViewHeader::ComputeColumnDragRect
	
	Extend the bottom of the rect to the bottom of the table
*/
void
LTableViewHeader::ComputeColumnDragRect(UInt16 inLeftColumn, Rect &outDragRect)
{
	LTableHeader::ComputeColumnDragRect(inLeftColumn, outDragRect);
	outDragRect.bottom = GetGlobalBottomOfTable();	
}


/*	
	LTableViewHeader::GetGlobalBottomOfTable
	
	Returns bottom of the table in global coords
*/
SInt16
LTableViewHeader::GetGlobalBottomOfTable() const
{
	Rect	r;
	
	mTableView->CalcPortFrameRect(r);
	
	PortToGlobalPoint((botRight(r)));
	return r.bottom;
}


/*	
	LTableViewHeader::RedrawColumns
	
	Refreshes the appropriate cells in the table.
*/
void
LTableViewHeader::RedrawColumns(UInt16 inFrom, UInt16 inTo)
{
	LTableHeader::RedrawColumns(inFrom, inTo);
	
	Boolean		active;
	TableIndexT	rows, cols;
	STableCell	fromCell, toCell;
	
	active = mTableView->IsActive();

	mTableView->GetTableSize(rows,cols);
	
	
	SPoint32		cellPt;
	SDimension16	frameSize;

	mTableView->GetScrollPosition(cellPt);
	mTableView->GetFrameSize(frameSize);
	
	mTableView->GetCellHitBy(cellPt, fromCell);
	fromCell.col = inFrom;
	
	cellPt.v += frameSize.height - 1;
	mTableView->GetCellHitBy(cellPt, toCell);
	toCell.col = inTo;
		
	mTableView->RefreshCellRange(fromCell, toCell);
}


/*	
	LTableViewHeader::ShowHideRightmostColumn
	
	Adds or removes a column in the table.
*/
void	
LTableViewHeader::ShowHideRightmostColumn(Boolean inShow)
{
	if (inShow) {
		mTableView->InsertCols(1, CountVisibleColumns(), NULL, 0, true);
	}
	else {
		mTableView->RemoveCols(1, CountVisibleColumns(), true);
	}
	
	LTableHeader::ShowHideRightmostColumn(inShow);
}


/*	
	LTableViewHeader::ResizeImageBy
	
	Adjust the table's image size.
*/
void	
LTableViewHeader::ResizeImageBy(	Int32	inWidthDelta, 
									Int32 	inHeightDelta,
									Boolean inRefresh		)
{
	LView::ResizeImageBy(inWidthDelta, inHeightDelta, inRefresh);
	mTableView->AdjustImageSize(inRefresh);
}


