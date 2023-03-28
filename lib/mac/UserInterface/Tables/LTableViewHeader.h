/*
	LTableViewHeader.h

	© 1996 Netscape Communications, Inc.
	
	Created 3/20/96 - Tim Craycroft

*/

#pragma once

#include "LTableHeader.h"
#include <LTableView.h>


class LTableViewHeader : public LTableHeader
{
public:
	enum { class_ID = 'tbvH' };
	
	LTableViewHeader(LStream *inStream);
	static LTableViewHeader *CreateTableViewHeaderStream(LStream *inStream);

	virtual void	FinishCreateSelf();		
	virtual void	ResizeImageBy(Int32 inWidthDelta, Int32 inHeightDelta, Boolean inRefresh);
									
	inline	LTableView*	GetTableView() const { return mTableView; }	

		
protected:

			SInt16	GetGlobalBottomOfTable() const;

	virtual void	ComputeResizeDragRect(UInt16 inLeftColumn, Rect	&outDragRect);
	virtual void	ComputeColumnDragRect(UInt16 inLeftColumn, Rect &outDragRect);
	
	virtual void	RedrawColumns(UInt16 inFrom, UInt16 inTo);								
	virtual void	ShowHideRightmostColumn(Boolean inShow);


	LTableView	*	mTableView;
	PaneIDT			mTableViewID;
		
};
