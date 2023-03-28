/*
	LTableRowSelector.cp

	© 1996 Netscape Communications, Inc.
	
	Created 3/25/96 - Tim Craycroft

	To Do: fix DragSelect
*/


#include "LTableRowSelector.h"


/*
	LTableRowSelector::LTableRowSelector
*/
LTableRowSelector::LTableRowSelector(LTableView *inTableView)
 : LTableSelector(inTableView), LDynamicArray(1)
{
	mAnchorRow = 0;
	mExtensionRow = 0;
}


/*
	LTableRowSelector::CellIsSelected
*/
Boolean
LTableRowSelector::CellIsSelected(const STableCell	&inCell) const
{
	char item;
	
	FetchItemAt(inCell.row, &item);
	return (item != 0);
}


/*
	LTableRowSelector::SelectCell
*/
void	
LTableRowSelector::SelectCell(const STableCell	&inCell)
{
	DoSelect(inCell.row, true, true);
}
	
/*
	LTableRowSelector::UnselectCell
*/
void		
LTableRowSelector::UnselectCell(const STableCell &inCell)
{
	DoSelect(inCell.row, false, true);
}

/*
	LTableRowSelector::DoSelect
*/
void
LTableRowSelector::DoSelect(	const TableIndexT 	inRow, 
								Boolean 			inSelect, 
								Boolean 			inHilite		)
{
	char oldRowValue, newRowValue;
	
	newRowValue = (inSelect) ? 1 : 0;
	FetchItemAt(inRow, &oldRowValue);
	
		
	if (newRowValue != oldRowValue)
	{
		// mark the row's new selected state
		SetItemAt(inRow, &newRowValue);
		
		// hilite (or unhilite) the entire row
		if (inHilite)
		{
			STableCell	cell;
			UInt32		nCols, nRows;
			
			mTableView->GetTableSize(nRows, nCols);
			
			cell.row = inRow;
			for (cell.col = 1;  cell.col <= nCols; cell.col++) {
				mTableView->HiliteCell(cell, inSelect);
			}
				
			mTableView->SelectionChanged();
		}
	}
}					




/*
	LTableRowSelector::SelectAllCells
*/
void	
LTableRowSelector::SelectAllCells()
{
	DoSelectAll(true);
}	


/*
	LTableRowSelector::UnselectAllCells
*/
void
LTableRowSelector::UnselectAllCells()
{
	DoSelectAll(false);
}


/*
	LTableRowSelector::DoSelectAll
*/
void
LTableRowSelector::DoSelectAll(Boolean inSelect)
{
	DoSelectRange(1, GetCount(), inSelect);
}


/*
	LTableRowSelector::DoSelectRange
	
	Selects a range of rows, where inFrom and inTo can
	be in any order.
*/
void
LTableRowSelector::DoSelectRange(	TableIndexT	inFrom, 
									TableIndexT inTo, 
									Boolean 	inSelect	)
{
	TableIndexT	top, bottom;
	STableCell	cell;
	
	// figure out our order
	if (inTo > inFrom) 
	{
		top 	= inFrom;
		bottom 	= inTo;
	}
	else
	{
		top 	= inTo;
		bottom  = inFrom;
	}
	
	cell.col = 1;
	for (cell.row=top; cell.row <= bottom; cell.row++)
		DoSelect(cell.row, inSelect, true);

}



/*
	LTableRowSelector::ExtendSelection
*/
void
LTableRowSelector::ExtendSelection( TableIndexT inRow)
{
	// mExtension row is the row to which we've 
	// currently extended the selection
	
	// mAnchor row is the originally selected row

	if (mExtensionRow != inRow)
	{
		if (inRow > mExtensionRow) {
			DoSelectRange(mExtensionRow+1, inRow, true);
		}
		else {
		 	DoSelectRange(inRow, mExtensionRow-1, true);
		}
		
		mExtensionRow = inRow;
	}
}



/*
	LTableRowSelector::ClickSelect
*/
void		
LTableRowSelector::ClickSelect(	const STableCell		&inCell,
								const SMouseDownEvent	&inMouseDown)
{
	// This code largely ripped off from PowerPlant's 
	// LTableMultiSelector
	//
	
	// Command-key discontiguous selection....
	if ((inMouseDown.macEvent.modifiers & cmdKey)!= 0)
	{
		//
		
		//
		
		if (CellIsSelected(inCell))
		{
			DoSelect(inCell.row, false, true);
			mAnchorRow 		= inCell.row;
			mAnchorRow 		= 0;
			mExtensionRow 	= 0;
		}
		else
		{
			DoSelect(inCell.row, true, true);
			mAnchorRow 		= inCell.row;
			mExtensionRow	= mAnchorRow;
		}
	}
	
	// shift-key selection extension
	else if ( 	((inMouseDown.macEvent.modifiers & shiftKey) != 0) &&
				mAnchorRow != 0 )	
	{	
		ExtendSelection(inCell.row);
	}
	
	// normal selection
	else
	{
		mAnchorRow 		= inCell.row;
		mExtensionRow 	= mAnchorRow;
		
		if (!CellIsSelected(inCell))
		{
			DoSelectAll(false);
			DoSelect(inCell.row, true, true);
		}
	}
	
}

								
/*
	LTableRowSelector::DragSelect
*/	
void
LTableRowSelector::DragSelect(	const STableCell		&inCell,
								const SMouseDownEvent	&inMouseDown)
{
	// Again, largely ripped off from PowerPlant
	ClickSelect(inCell, inMouseDown);
	
	STableCell	currCell = inCell;	
	while (::StillDown()) 
	{
		STableCell	hitCell;
		SPoint32	imageLoc;
		Point		mouseLoc;
		
		::GetMouse(&mouseLoc);
		if (mTableView->AutoScrollImage(mouseLoc)) 
		{
			mTableView->FocusDraw();
			Rect	frame;
			mTableView->CalcLocalFrameRect(frame);
			Int32 pt = ::PinRect(&frame, mouseLoc);
			mouseLoc = *(Point*)&pt;
		}
		
		mTableView->LocalToImagePoint(mouseLoc, imageLoc);
		mTableView->GetCellHitBy(imageLoc, hitCell);
		if (mTableView->IsValidCell(hitCell) &&
			(currCell != hitCell)) 
		{

			currCell = hitCell;
		}
		
		
		ExtendSelection(currCell.row);
	}	
}


								
/*
	LTableRowSelector::InsertRows
	
	Add space in our array for tracking the selected state of the new rows
*/	
void		
LTableRowSelector::InsertRows(	UInt32		inHowMany,
								TableIndexT	inAfterRow)
{
	char unselectedItem = 0;
	InsertItemsAt(inHowMany, inAfterRow, &unselectedItem);	
}
									

/*
	LTableRowSelector::RemoveRows
	
	Remove the space from our array.
*/	
void
LTableRowSelector::RemoveRows(	Uint32		inHowMany,
								TableIndexT	inFromRow )
{
	RemoveItemsAt(inHowMany, inFromRow);
}								

