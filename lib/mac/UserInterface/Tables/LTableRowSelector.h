/*
	LTableRowSelector.h

	© 1996 Netscape Communications, Inc.
	
	Created 3/26/96 - Tim Craycroft

	
	A subclass of LTableSelector that always selects entire rows.
	Supports multiple selection and std selection extension semantics.
	
	Kind of lame implementation.  We store an array (LDynamicArray) 
	of bytes, each byte representing a row.  Easier than expanding
	and collapsing an array of bits.
*/


#pragma once

#include <UTableHelpers.h>


class	LTableRowSelector : public LTableSelector, LDynamicArray {
public:
		LTableRowSelector(LTableView *inTableView);
						
	virtual Boolean		CellIsSelected(const STableCell	&inCell) const;
	
	virtual void		SelectCell(const STableCell	&inCell);
	virtual void		SelectAllCells();
	virtual void		UnselectCell(const STableCell &inCell);
	virtual void		UnselectAllCells();
	
	virtual void		ClickSelect(const STableCell		&inCell,
									const SMouseDownEvent	&inMouseDown);
	
	virtual void		DragSelect(	const STableCell		&inCell,
									const SMouseDownEvent	&inMouseDown);

	virtual void		InsertRows(	Uint32					inHowMany,
									TableIndexT				inAfterRow);
									
	virtual void		RemoveRows(
								Uint32		inHowMany,
								TableIndexT	inFromRow );
								
protected:
	
	TableIndexT	mAnchorRow;
	TableIndexT	mExtensionRow;

	void DoSelectAll(Boolean inSelect);		
	void DoSelect(TableIndexT inRow, Boolean inSelect, Boolean inHilite);	
	void DoSelectRange(TableIndexT inFrom, TableIndexT inTo, Boolean inSelect);
	
	void ExtendSelection(TableIndexT inRow);
};

