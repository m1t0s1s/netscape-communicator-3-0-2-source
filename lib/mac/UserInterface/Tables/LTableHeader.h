/*
	LTableHeader.h

	© 1996 Netscape Communications, Inc.
	
	Created 3/18/96 - Tim Craycroft

*/


#pragma once


#include <LView.h>
#include "UStdBevels.h"


//
// This is the ioParam in the message broadcast
// when the sort order is changed (msg_SortedColumnChanged).
//
typedef struct
{
	PaneIDT	sortColumnID;	// pane id of header of sort column
	UInt16	sortColumn;		// physical sort column
	Boolean	reverseSort;	// was reverse sort specified ?
} TableHeaderSortChange;



class LTableHeader : public LView, LBroadcaster
{

public:
	enum	{ class_ID = 'tblH' 				};
	enum	{ msg_SortedColumnChanged = 'srtC'	};
	enum	{ kColumnStateResourceType = 'Cols'	};
	

	LTableHeader(LStream *inStream);
	virtual ~LTableHeader();
	static LTableHeader *CreateTableHeaderStream(LStream *inStream);
	
	
	// PowerPlant hooks
	virtual void	FinishCreateSelf();
	virtual void	DrawSelf();
	virtual void	Click(SMouseDownEvent &inMouseDown);
	
	virtual void	AdjustCursor(	Point inPortPt,
									const EventRecord &inMacEvent);

	virtual void	ResizeFrameBy(	Int16	inWidthDelta,
									Int16	inHeightDelta,
									Boolean	inRefresh);
									
									

	
	
	// Column info access
	inline UInt16			CountColumns() const 		{ return mColumnCount;  		};
	inline UInt16			CountVisibleColumns() const { return mLastVisibleColumn; 	};
	
	UInt16					GetSortedColumn(PaneIDT & outHeaderPane) const;
	virtual void 			SetSortedColumn(UInt16 inColumn, Boolean inReverse, Boolean inRefresh=true);
	
	inline PaneIDT			GetColumnPaneID(UInt16 inColumn) 		const;	// returns PaneID of column's header
	SInt16					GetColumnWidth(UInt16 inColumn) 		const;	// returns column's width
	SInt16					GetColumnPosition(UInt16 inColumn) 		const;	// returns column's horiz position
	UInt16					GetHeaderWidth() 						const;	// returns width occupied by columns


	typedef UInt32	HeaderFlags;
	enum {	kHeaderCanHideColumns	=	0x1 };	
	
	
	// Persistence...
	typedef UInt32 ColumnFlags;
	enum 
	{
		kColumnDoesNotSort		=	0x1,
		kColumnDoesNotResize	=	0x2
	};

	typedef struct 
	{
		PaneIDT			paneID;
		SInt16			columnWidth;
		SInt16			columnPosition;		// meaningless when saved/read to/from disk
		ColumnFlags		flags;				
	} ColumnData;
	
	typedef struct
	{
		UInt16		columnCount;
		UInt16		lastVisibleColumn;
		UInt16		sortedColumn;
		UInt16		pad;
		ColumnData	columnData[1];	
	} SavedHeaderState;

	virtual void	WriteColumnState(LStream * inStream);
	virtual void	ReadColumnState(LStream * inStream, Boolean inMoveHeaders = true);
	

protected:
	enum	
	{ 
	 	kColumnMargin 				= 2,
	 	kColumnHidingWidgetWidth 	= 16
	};

	HeaderFlags			mHeaderFlags;
	ColumnData **		mColumnData;
	SBevelColorDesc		mUnsortedBevel;
	SBevelColorDesc		mSortedBevel;

	UInt16				mColumnCount;		// 1-based count of all columns
	UInt16				mLastVisibleColumn;	// 0-based index of rightmost visible column
	UInt16				mSortedColumn;		// 0-based index of sorted columns
	UInt16				mReverseModifiers;	// key event modifier bit-mask for reverse-sort
	SInt16				mFixedWidth;		// total width occupied by fixed-size columns


	// Column geometry
	UInt16			FindColumn( Point inLocalPoint ) 				const;
	void			LocateColumn( UInt16 inColumn, Rect & outWhere) const;
	void			InvalColumn( UInt16 inColumn);
	void			ComputeColumnPositions();
	virtual void	PositionOneColumnHeader( UInt16 inColumn, Boolean inRefresh = true);
	void			PositionColumnHeaders(Boolean inRefresh = true);
	void			ConvertWidthsToAbsolute();
	void			ConvertWidthsToPercentages();
	void			RedistributeSpace(	UInt16	inFromCol,
										UInt16	inToCol,
										SInt16	inNewSpace,
										SInt16	inOldSpace);
										
	SInt16			GetFixedWidthInRange(	UInt16	inFromCol,
											UInt16	inToCol	);
										

	// Column resizing
	Boolean			IsInHorizontalSizeArea(Point inLocalPt, UInt16 &outLeftColumn);
	void			TrackColumnResize(const SMouseDownEvent	&inEvent, UInt16 inLeftColumn);
	virtual void	ComputeResizeDragRect(UInt16 inLeftColumn, Rect	&outDragRect);
	virtual void	ResizeColumn(UInt16 inLeftColumn, SInt16 inLeftColumnDelta);
	
	// Column dragging
	void			TrackColumnDrag(const SMouseDownEvent &	inEvent, UInt16 inColumn);
	virtual void	ComputeColumnDragRect(UInt16 inColumn, Rect& outDragRect);
	virtual void	MoveColumn(UInt16 inColumn, UInt16 inMoveTo);

	
	// Column drawing
	virtual void	DrawColumnBackground(const Rect& inWhere, Boolean inSortColumn);
	virtual void	RedrawColumns(UInt16 inFrom, UInt16 inTo);								

	// Column hiding
	virtual void	ShowHideRightmostColumn(Boolean inShow);
	
	
	// Private Column info
	//
	// These are all excellent candidates for inlines, but we'll
	// let the compiler decide...
	//
	Boolean	CanColumnResize(UInt16 inColumn) 				const;
	Boolean	CanColumnResize(const ColumnData *inColumn) 	const;
	Boolean	CanColumnSort(UInt16 inColumn) 					const;
	Boolean	CanHideColumns() 								const;
	
	LPane*	GetColumnPane(UInt16 inColumn);
};




