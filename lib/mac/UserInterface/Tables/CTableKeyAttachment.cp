// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	CTableKeyAttachment.cp					
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

#include "CTableKeyAttachment.h"

#include <PP_KeyCodes.h>
#include <LTableView.h>

CTableKeyAttachment::CTableKeyAttachment(LTableView* inView)
	:	LAttachment(msg_KeyPress)
{
	mTableView = inView;
}

CTableKeyAttachment::~CTableKeyAttachment()
{
}

	
void CTableKeyAttachment::ExecuteSelf(
	MessageT		inMessage,
	void			*ioParam)
{
	mExecuteHost = false;
	Int16 theKey = ((EventRecord*)ioParam)->message & charCodeMask;
	Boolean bCommandDown = (((EventRecord*)ioParam)->modifiers & cmdKey) != 0;
	
	switch (theKey)
		{
		case char_DownArrow:
		if (!bCommandDown)
			{
			// Get the currently selected cell
			STableCell theCell; // inited to 0,0 in constructor
			mTableView->GetNextSelectedCell(theCell);

			if (mTableView->GetNextCell(theCell))
				mTableView->SelectCell(theCell);
			break;
			}
		// else falls through
		
		case char_End:
			{
			// get the table dimensions
			TableIndexT theRowCount, theColCount;		
			mTableView->GetTableSize(theRowCount, theColCount);
			STableCell theLastCell(theRowCount, theColCount);
			mTableView->SelectCell(theLastCell);
			}
			break;
		
		case char_UpArrow:
		if (!bCommandDown)
			{
			// Get the currently selected cell
			STableCell theCell; // inited to 0,0 in constructor
			mTableView->GetNextSelectedCell(theCell);
			
			theCell.row--;
			if (mTableView->IsValidCell(theCell))
				mTableView->SelectCell(theCell);
			break;
			}
		// else falls through
		
		case char_Home:
			{
			STableCell theFirstCell(1,1);
			if (mTableView->IsValidCell(theFirstCell))
				mTableView->SelectCell(theFirstCell);
			}
			break;
		
		default:
			mExecuteHost = true;	// Some other key, let host respond
			break;
		}
}
