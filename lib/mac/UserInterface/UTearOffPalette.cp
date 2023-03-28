// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	UTearOff.cp					  ©1996 Netscape. All rights reserved.
// author: atotic
// See UTearOffPalette.h for usage instructions and design notes
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

#include "UTearOffPalette.h"

// PowerPlant
#include <LStream.h>
#include <URegions.h>
#include <LArray.h>
#include <LArrayIterator.h>

#pragma mark CTearOffBar
#pragma mark -

// ---------------------------------------------------------------------------
//		¥ CreateFromStream [static]
// ---------------------------------------------------------------------------

CTearOffBar* CTearOffBar::CreateFromStream(LStream* inStream)
{
	return new CTearOffBar(inStream);
}

// ---------------------------------------------------------------------------
//		¥ Constructor from stream
// ---------------------------------------------------------------------------
// Read in the data

CTearOffBar::CTearOffBar(LStream* inStream) : LView(inStream)
{
	*inStream >> fSubpaneID;
	*inStream >> fAutoResize;
	*inStream >> fVisibility;
	*inStream >> fTitle;

	fOriginalFrameSize = mFrameSize;
	fShown = false;
	fTransparentPane = 0;
}

// ---------------------------------------------------------------------------
//		¥ Destructor
// ---------------------------------------------------------------------------
// Unregister with the manager

CTearOffBar::~CTearOffBar()
{
	CTearOffManager::GetDefaultManager()->UnregisterBar(this);
}

// ---------------------------------------------------------------------------
//		¥ FinishCreateSelf
// ---------------------------------------------------------------------------
// Register with the manager

void CTearOffBar::FinishCreateSelf()
{
	CTearOffManager::GetDefaultManager()->RegisterBar(this);
}

// ---------------------------------------------------------------------------
//		¥ Click
// ---------------------------------------------------------------------------
// Slightly modified LView::Click
// We should handle clicks in the transparent pane
// We need this because the PPob view that holds the buttons that we put inside
// completely covers this view. Unless we make that view transparent, we'll never
// get any clicks

void CTearOffBar::Click( SMouseDownEvent	&inMouseDown )
{
									// Check if a SubPane of this View
									//   is hit
// Modified code
// Check if we have clicked on a 0 pane
	LPane	*clickedPane = FindDeepSubPaneContaining(inMouseDown.wherePort.h,
											inMouseDown.wherePort.v );
	if (clickedPane != nil && clickedPane->GetPaneID() == fTransparentPane)
	{
		LPane::Click(inMouseDown);
		return;
	}

// LView::Click code
	clickedPane = FindSubPaneHitBy(inMouseDown.wherePort.h,
											inMouseDown.wherePort.v);
											
	if	( clickedPane != nil )
	{		//  SubPane is hit, let it respond to the Click
		clickedPane->Click(inMouseDown);
		
	} else {						// No SubPane hit, or maybe pane 0. Inherited function
		LPane::Click(inMouseDown);	//   will process click on this View
	}
}

// ---------------------------------------------------------------------------
//		¥ ClickSelf
// ---------------------------------------------------------------------------
// Drag the outline of the bar out of the window
// Rougly copied out of UDesktop::DragDeskWindow

void CTearOffBar::ClickSelf(const SMouseDownEvent	&inMouseDown)
{
	if (::WaitMouseUp()) {				// Continue if mouse hasn't been
										//   released
										
		GrafPtr		savePort;			// Use ScreenPort for dragging
		GetPort(&savePort);
		GrafPtr		screenPort = UScreenPort::GetScreenPort();
		::SetPort(screenPort);
		StColorPenState::Normalize();

// Get surrounding rectangle in global port coordinates

		Rect frame;
		Point topLeft, botRight;
		topLeft.h = mFrameLocation.h;
		topLeft.v = mFrameLocation.v;
		botRight.h = topLeft.h + mFrameSize.width;
		botRight.v = topLeft.v + mFrameSize.height;
		PortToGlobalPoint(topLeft);
		PortToGlobalPoint(botRight);
		
		::SetRect( &frame, topLeft.h, topLeft.v, botRight.h, botRight.v );
		StRegion dragRgn(frame);
		
										// Let user drag a dotted outline
										//   of the Window
		Rect	dragRect = (**(GetGrayRgn())).rgnBBox;
		::InsetRect(&dragRect, 4, 4);
		
		Int32	dragDistance = ::DragGrayRgn(dragRgn.mRgn, inMouseDown.macEvent.where,
											&dragRect, &dragRect,
											noConstraint, nil);
		::SetPort(savePort);
		
			// Check if the Window moved to a new, valid location.
			// DragGrayRgn returns the constant Drag_Aborted if the
			// user dragged outside the DragRect, which cancels
			// the drag operation. We also check for the case where
			// the drag finished at its original location (zero distance).
			
		Int16	horizDrag = LoWord(dragDistance);
		Int16	vertDrag = HiWord(dragDistance);
		
		if ( (dragDistance != 0x80008000)  &&	// 0x80008000 == Drag_Aborted from UFloatingDesktop
			 (horizDrag != 0 || vertDrag != 0) ) {
			 
				// A valid drag occurred. horizDrag and vertDrag are
				// distances from the current location. We need to
				// get the new location for the Window in global
				// coordinates. The bounding box of the Window's
				// content region gives the current location in
				// global coordinates. Add drag distances to this
				// to get the new location. 
			CTearOffManager::GetDefaultManager()->DragBar(this, 
													topLeft.h + horizDrag,
													topLeft.v + vertDrag);
		}
	}
}

// ---------------------------------------------------------------------------
//		¥ AddListener
// ---------------------------------------------------------------------------
// Tell manager that we have a listener

void CTearOffBar::AddListener(LListener	*inListener)
{
	LBroadcaster::AddListener(inListener);
	CTearOffManager::GetDefaultManager()->RegisterListener(this, inListener);
}

// ---------------------------------------------------------------------------
//		¥ ListenToMessage 
// ---------------------------------------------------------------------------
// Tell TearOffManager that we got the message
void CTearOffBar::ListenToMessage(MessageT inMessage, void *ioParam)
{
	CTearOffManager::GetDefaultManager()->BarReceivedMessage(this, inMessage, ioParam);
}

// ---------------------------------------------------------------------------
//		¥ ListenToMessage 
// ---------------------------------------------------------------------------
// Tell TearOffManager that we got the message
void CTearOffBar::ActivateSelf()
{
	LView::ActivateSelf();
	CTearOffManager::GetDefaultManager()->BarActivated(this);
}

// ---------------------------------------------------------------------------
//		¥ ListenToMessage 
// ---------------------------------------------------------------------------
// Tell TearOffManager that we got the message
void CTearOffBar::DeactivateSelf()
{
	LView::DeactivateSelf();
	CTearOffManager::GetDefaultManager()->BarDeactivated(this);
}

// ---------------------------------------------------------------------------
//		¥ HidePalette 
// ---------------------------------------------------------------------------
// Delete all the subpanes.
// Manager will broadcast a message msg_FloatPalleteRemoved to your listener
void CTearOffBar::HidePalette()
{
	DeleteAllSubPanes();
	if (fAutoResize)
		ResizeFrameTo(0,0,TRUE);
	else
		Refresh();
	fShown = false;
}

// ---------------------------------------------------------------------------
//		¥ ShowPalette 
// ---------------------------------------------------------------------------
// Reads in the subview from PPob.
// Position the view properly

void CTearOffBar::ShowPalette()
{
	if (fShown)
		return;
	// setup commander (window), superview(this), and listener(this)
	
	SetDefaultView(this);
	LWindow * commander = LWindow::FetchWindowObject(GetMacPort());
	SignalIfNot_( commander );

	LView * view = UReanimator::CreateView(fSubpaneID, this, commander);
	
	SignalIfNot_ ( view );	// We need that pane ID

	view->FinishCreate();
	fTransparentPane = view->GetPaneID();
	UReanimator::LinkListenerToControls(this, this, fSubpaneID);

	if (fAutoResize)
		ResizeFrameTo(fOriginalFrameSize.width, fOriginalFrameSize.height, TRUE);
#ifdef DEBUG
	if (fAutoResize)
	{
		SDimension16 s;
		view->GetFrameSize(s);
		// The container and the loaded view should be the same width/height???
		SignalIf_(s.width != fOriginalFrameSize.width);
		SignalIf_(s.height != fOriginalFrameSize.height);
	}	
#endif
	fShown = true;
}

/************************************************************************
 * class CTearOffWindow
 *
 * just registers/unregisters itself with the 
 * You cannot subclass this class, because CTearOffManager creates all
 * palettes out of a single PPob.
 ************************************************************************/
class CTearOffWindow : public LWindow,
							public LListener
{
	friend class CTearOffManager;
	public:
		enum { class_ID = 'DRFW' };
		
// Constructors

		static CTearOffWindow* CreateFromStream(LStream* inStream);
							CTearOffWindow(LStream* inStream);

		virtual				~CTearOffWindow();

// Events
		virtual void		ListenToMessage(MessageT inMessage, void *ioParam);
	private:
		UInt8				fVisibility;	// Inherited from the bar
};
#pragma mark -
#pragma mark CTearOffWindow
#pragma mark -

// ---------------------------------------------------------------------------
//		¥ CTearOffWindow streamer
// ---------------------------------------------------------------------------

CTearOffWindow* CTearOffWindow::CreateFromStream(LStream* inStream)
{
	return new CTearOffWindow(inStream);
}

// ---------------------------------------------------------------------------
//		¥ CTearOffWindow constructor
// ---------------------------------------------------------------------------
CTearOffWindow::CTearOffWindow(LStream* inStream) : LWindow(inStream)
{
}

// ---------------------------------------------------------------------------
//		¥ CTearOffWindow destructor
// ---------------------------------------------------------------------------
// Notify the manager
CTearOffWindow::~CTearOffWindow()
{
	CTearOffManager::GetDefaultManager()->UnregisterWindow(this);
}

// ---------------------------------------------------------------------------
//		¥ CTearOffWindow destructor
// ---------------------------------------------------------------------------
// Pass on to the manager
void CTearOffWindow::ListenToMessage(MessageT inMessage, void *ioParam)
{
	CTearOffManager::GetDefaultManager()->WindowReceivedMessage(this, inMessage, ioParam);
}

/*****************************************************************************
 * class CTearOffManager
 *****************************************************************************/
#pragma mark -
#pragma mark CTearOffManager
#pragma mark -
CTearOffManager *  CTearOffManager::sDefaultFloatManager = NULL;

#pragma mark struct ButtonBarRecord
// holds information about a single button bar, stored in fPaletteBars list
struct ButtonBarRecord
{
	ResIDT				fID;	// ResID of the PPob loaded into the button bar.
	CTearOffBar *	fBar;	// Pointer to the bar
	LListener * 		fListener;	// The listener for this bar
		
	ButtonBarRecord()
	{
		fID = -1;
		fBar = NULL;
		fListener = NULL;
	}
	
	ButtonBarRecord(ResIDT id, CTearOffBar * bar, LListener * listener)
	{
		fID = id;
		fBar = bar;
		fListener = listener;
	}
};

/****************************************************************************
 * struct FloatWindowRecord
 * used to keep a list of loaded windows
 ****************************************************************************/
#pragma mark struct FloatWindowRecord
struct FloatWindowRecord
{
	CTearOffWindow * fWindow;
	ResIDT				fID;		// Res ID of the PPob to be loaeded
	CTearOffBar * fActiveBar;

	FloatWindowRecord(CTearOffWindow * window, ResIDT resID, CTearOffBar * activeBar = NULL)
	{
		fWindow = window;
		fID = resID;
		fActiveBar = activeBar;
	}

};

// ---------------------------------------------------------------------------
//		¥ Compare the window records
// ---------------------------------------------------------------------------
// Wildcards can be used for fID and fWindow fields
// fListener field is not used.

#define WILDCARD -1

Int32 CTearOffManager::CPaletteWindowComparator::Compare(const void*		inItemOne,
									const void*		inItemTwo,
									Uint32			inSizeOne,
									Uint32			inSizeTwo) const
{
#pragma unused (inSizeOne, inSizeTwo)	// Cause we know what the items are

	FloatWindowRecord * item1 = (FloatWindowRecord *)inItemOne;
	FloatWindowRecord * item2 = (FloatWindowRecord *)inItemTwo;

	if ( item1->fID != item2->fID &&	// Different floaters
		 (item1->fID != WILDCARD) &&
		 (item2->fID != WILDCARD))	
		return (item1->fWindow - item2->fWindow);
	else							// Same floaters
		if ( item1->fWindow == item2->fWindow ||
			( item1->fWindow == (void*)WILDCARD ) ||
			( item2->fWindow == (void*)WILDCARD ) )
			return 0;
		else
			return item1->fWindow - item2->fWindow;
}

// ---------------------------------------------------------------------------
//		¥ Compare the bars
// ---------------------------------------------------------------------------
// Wildcards can be used for fID and fBar fields
// fListener field is not used.

Int32 CTearOffManager::CPaletteBarComparator::Compare(const void*		inItemOne,
									const void*		inItemTwo,
									Uint32			inSizeOne,
									Uint32			inSizeTwo) const
{
#pragma unused (inSizeOne, inSizeTwo)	// Cause we know what the items are

	ButtonBarRecord * item1 = (ButtonBarRecord *)inItemOne;
	ButtonBarRecord * item2 = (ButtonBarRecord *)inItemTwo;


	if ( item1->fID != item2->fID &&	// Different floaters
		 (item1->fID != WILDCARD) &&
		 (item2->fID != WILDCARD) )	
		return ( item1->fBar - item2->fBar );
	else							// Same floaters
		if ( item1->fBar == item2->fBar ||
			( item1->fBar == (void*)WILDCARD ) ||
			( item2->fBar == (void*)WILDCARD ) )
			return 0;
		else
			return item1->fBar - item2->fBar;
	return 0;	// Kill the warning
}

// ---------------------------------------------------------------------------
//		¥ CTearOffManager Constructor 
// ---------------------------------------------------------------------------
// sets the default manager

CTearOffManager::CTearOffManager(LCommander * defaultCommander)
{
	sDefaultFloatManager = this;
	
	// These lists cannot be sorted, because sometimes we do wildcard lookups
	// If you want to sort them, implement CompareKey better
	fPaletteBars = new LArray( sizeof (ButtonBarRecord), &fBarCompare, false );
	fPaletteWindows = new LArray( sizeof (FloatWindowRecord), &fWindowCompare, false );

	fDefaultCommander = defaultCommander;
}

// ---------------------------------------------------------------------------
//		¥ CTearOffManager destructor -- never called, here for completeness
// ---------------------------------------------------------------------------
// clean up
CTearOffManager::~CTearOffManager()
{
	delete fPaletteBars;
	delete fPaletteWindows;
}

// ---------------------------------------------------------------------------
//		¥ RegisterBar 
// ---------------------------------------------------------------------------
// Insert the bar in the bar list. Hide or show it, depending on the window

void CTearOffManager::RegisterBar(CTearOffBar * inBar)
{
	// Insert it into the list
	ButtonBarRecord barRec( inBar->fSubpaneID, inBar, NULL);
	fPaletteBars->InsertItemsAt(1, fPaletteBars->GetCount(), &barRec, sizeof( barRec ) );
	
	// Decide whether to hide or show the bar

	FloatWindowRecord fr( (CTearOffWindow *)WILDCARD, inBar->fSubpaneID );
	ArrayIndexT where = fPaletteWindows->FetchIndexOfKey( &fr );
	if ( where != LArray::index_Bad )
		inBar->HidePalette();
	else
		inBar->ShowPalette();
}

// ---------------------------------------------------------------------------
//		¥ UnregisterBar 
// ---------------------------------------------------------------------------
// Remove the bar from the bar list
// If there are any "left-over" floaters, remove them

void CTearOffManager::UnregisterBar(CTearOffBar * inBar)
{
	ButtonBarRecord bar( WILDCARD, inBar, NULL);
	ArrayIndexT where = fPaletteBars->FetchIndexOfKey(&bar);
	if ( where != LArray::index_Bad )
		fPaletteBars->RemoveItemsAt( 1, where );
	else
		SignalCStr_("Unregistering a bar that was never registered?");

// If there are no other bars with the same ID,
// remove the floater window (floater might not exist)

	ButtonBarRecord brotherBar( inBar->fSubpaneID, (CTearOffBar *)WILDCARD, NULL );
	
	where = fPaletteBars->FetchIndexOfKey(&brotherBar);
	if ( where == LArray::index_Bad )
	// Remove the floaters with this ID
	{
		FloatWindowRecord fw( (CTearOffWindow*) WILDCARD, inBar->fSubpaneID );
		ArrayIndexT where = fPaletteWindows->FetchIndexOfKey( &fw );
		if ( where != LArray::index_Bad )
		{
			fPaletteWindows->FetchItemAt( where, &fw );
			if (fw.fWindow->fVisibility != 0)
				delete fw.fWindow;	// this will automatically remove it from the list
		}
	}
}
	
// ---------------------------------------------------------------------------
//		¥ RegisterListener 
// ---------------------------------------------------------------------------
// Set the listener
// Broadcast the palette message to the window, depending on where the pallete is

void CTearOffManager::RegisterListener(CTearOffBar * inBar, 
											LListener * listener)
{
	ButtonBarRecord bar( WILDCARD, inBar, listener );
	ArrayIndexT where = fPaletteBars->FetchIndexOfKey(&bar);
	if (where != LArray::index_Bad)
	{
	// assign the listener
		fPaletteBars->FetchItemAt( where, &bar );
		bar.fListener = listener;
		fPaletteBars->AssignItemsAt( 1, where , &bar );
	
	// broadcast the palette message to the listener
		FloatWindowRecord fr( (CTearOffWindow *)WILDCARD, inBar->fSubpaneID );
		ArrayIndexT where = fPaletteWindows->FetchIndexOfKey( &fr );
		
		if ( where != LArray::index_Bad )
		{
		// We have a window to register
			fPaletteWindows->FetchItemAt( where, &fr );
			SetBarsView( inBar, fr.fWindow );
		}
		else
		// The bar holds the palette
		{
			SetBarsView( inBar, inBar );
		}
	}
	else
		SignalCStr_("Bar never registered?");
}
	
// ---------------------------------------------------------------------------
//		¥ BarReceivedMessage 
// ---------------------------------------------------------------------------
// Relay the message to its listeners

void CTearOffManager::BarReceivedMessage(CTearOffBar * inBar,
											MessageT inMessage,
											 void *ioParam)
{
	ButtonBarRecord bar( WILDCARD, inBar, NULL );
	ArrayIndexT where = fPaletteBars->FetchIndexOfKey(&bar);
	
	if ( where != LArray::index_Bad )
	{
		fPaletteBars->FetchItemAt( where, &bar );
//		SignalIfNot_(bar.fListener);
		if (bar.fListener)
			bar.fListener->ListenToMessage( inMessage, ioParam );
	}
	else
		SignalCStr_("Bar never registered?");
}

// ---------------------------------------------------------------------------
//		¥ DragBar 
// ---------------------------------------------------------------------------
// Assert that the window exists
// move it to the new location

void CTearOffManager::DragBar( CTearOffBar * inBar,
								Int16 inH,
								Int16 inV )
{
	try
	{
		CTearOffWindow * win = AssertBarWindowExists(inBar);

		Point topLeft;
		topLeft.h = inH;
		topLeft.v = inV;
		win->DoSetPosition(topLeft);
		win->Show();
		
		SetBarsView( inBar, win );
	}
	catch(OSErr inErr)
	{
		SignalCStr_("Did you include UTearOffPalette.r in your project?");
		// No window, button bar stays where it was
	}
}

// ---------------------------------------------------------------------------
//		¥ BarActivated 
// ---------------------------------------------------------------------------
// Upon activation:
// Tell its float windowGets the floater window for the bar
// Tells listener what the current palette is
// 
void CTearOffManager::BarActivated( CTearOffBar * inBar)
{
// Find the floater window
	FloatWindowRecord fw( (CTearOffWindow *) WILDCARD, inBar->fSubpaneID );
	ArrayIndexT where = fPaletteWindows->FetchIndexOfKey( &fw );

// If we have a window, make this bar the active one
	if ( where != LArray::index_Bad )
	{
		fPaletteWindows->FetchItemAt( where, &fw );
		if ( fw.fActiveBar != inBar )
		{
			if ( fw.fActiveBar == NULL )	// If there was no view, we need to enable us
			{
				LPane * view;
				LListIterator iter( fw.fWindow->GetSubPanes(), iterate_FromStart);
				if ( iter.Next(&view) )	// We have disabled this on suspends
					view->Enable();
			}
			
			fw.fActiveBar = inBar;
			fPaletteWindows->AssignItemsAt( 1, where , &fw );
			fw.fWindow->Show();
			

			SetBarsView(inBar, fw.fWindow);
			
		}
	}
}

// ---------------------------------------------------------------------------
//		¥ BarDeactivated 
// ---------------------------------------------------------------------------
// Upon deactivation
// If we have a floating window, make it inactive and tell listener about it

void CTearOffManager::BarDeactivated( CTearOffBar * inBar)
{
	FloatWindowRecord fw( (CTearOffWindow *) WILDCARD, inBar->fSubpaneID );
	ArrayIndexT where = fPaletteWindows->FetchIndexOfKey( &fw );

	if ( where != LArray::index_Bad )
	{
		fPaletteWindows->FetchItemAt( where, &fw );
		if ( fw.fActiveBar == inBar )
		{
			if ( fw.fWindow->fVisibility == 2 )
				fw.fWindow->Hide();
			else
			{	// Disable all the subpanes of our main subpane
				// Do not disable the main pane, because no one enables it
				LPane * view;
				LListIterator iter( fw.fWindow->GetSubPanes(), iterate_FromStart);
				if ( iter.Next(&view) )
					view->Disable();
			}
			fw.fActiveBar = NULL;
			fPaletteWindows->AssignItemsAt( 1, where , &fw );
			RemoveBarsView(inBar, inBar->fTransparentPane);

		}
	}
}

// ---------------------------------------------------------------------------
//		¥ SetBarsView 
// ---------------------------------------------------------------------------
// Tell listener what its palette view is
// InView is either FBar, or FWindow. In either case, the palette is the first subview

void CTearOffManager::SetBarsView( CTearOffBar * inBar,
										LView * inView)
{
	SignalIfNot_( inView );

	// Get the bar record
	ButtonBarRecord br( WILDCARD, inBar, (LListener*) WILDCARD );
	ArrayIndexT where = fPaletteBars->FetchIndexOfKey( &br );

	if ( where != LArray::index_Bad )
	{
		fPaletteBars->FetchItemAt( where, &br );
		if ( br.fListener )
		{
			// Palette is the first subview
			LView * topPaletteView;
			LListIterator iter( inView->GetSubPanes(), iterate_FromStart);
			if ( iter.Next(&topPaletteView) )
				br.fListener->ListenToMessage( msg_SetTearOffPalette, topPaletteView );
		}
	}
	else
		SignalCStr_("Where is the bar?");			
}

// ---------------------------------------------------------------------------
//		¥ RemoveBarsView 
// ---------------------------------------------------------------------------
// Similar to SetBarsView. Must get the top pane ID from somewhere 

void CTearOffManager::RemoveBarsView( CTearOffBar * inBar, PaneIDT inViewID)
{
	ButtonBarRecord br( WILDCARD, inBar, (LListener*) WILDCARD );
	ArrayIndexT where = fPaletteBars->FetchIndexOfKey( &br );

	if ( where != LArray::index_Bad )
	{
		fPaletteBars->FetchItemAt( where, &br );
		if ( br.fListener )
			br.fListener->ListenToMessage( msg_RemoveTearOffPalette, (void*) inViewID );
	}
	else
		SignalCStr_("Where is the bar?");		
}

// ---------------------------------------------------------------------------
//		¥ AssertBarWindowExists 
// ---------------------------------------------------------------------------
// Create the window if it does not exist

#define FLOAT_ID -19844	// Same define is in UFloatWindow.r

CTearOffWindow * CTearOffManager::AssertBarWindowExists(CTearOffBar * inBar)
{
	FloatWindowRecord fw( (CTearOffWindow *) WILDCARD, inBar->fSubpaneID );
	ArrayIndexT where = fPaletteWindows->FetchIndexOfKey( &fw );
	
	if (where == LArray::index_Bad)
	{
	// Create a new window, load the palette into it
		CTearOffWindow * newWindow = (CTearOffWindow *) LWindow::CreateWindow(FLOAT_ID, fDefaultCommander);
		ThrowIfNil_(newWindow);
		newWindow->fVisibility = inBar->fVisibility;

		LView * view = UReanimator::CreateView(inBar->fSubpaneID, newWindow, newWindow);
		ThrowIfNil_( view );
		view->FinishCreate();

	// Resize the window
		SDimension16 viewSize;
		Rect winBounds;
		view->GetFrameSize(viewSize);
		::SetRect( &winBounds, 0, 0, viewSize.width, viewSize.height );
		newWindow->DoSetBounds( winBounds );
		
		newWindow->SetDescriptor( inBar->fTitle );
	// Add the record to the list of managed floaters
		FloatWindowRecord fw( newWindow, inBar->fSubpaneID , inBar);
		fPaletteWindows->InsertItemsAt( 1, fPaletteWindows->GetCount(), &fw, sizeof (fw) );

	// Hide the bars button palletes
		LArrayIterator iter(*fPaletteBars);
		ButtonBarRecord bar;
		while ( iter.Next(&bar) )
			if (bar.fID == fw.fID)
			{
				bar.fBar->HidePalette();
				bar.fBar->fTransparentPane = view->GetPaneID();
				RemoveBarsView( bar.fBar, bar.fBar->fTransparentPane );
			}
	// Link up the new controls
		UReanimator::LinkListenerToControls( newWindow, newWindow, fw.fID );

		return newWindow;
	}
	else
	{
		fPaletteWindows->FetchItemAt( where, &fw );
		return fw.fWindow;
	}
	return NULL;	// Compiler warning avoidance
}

// ---------------------------------------------------------------------------
//		¥ UnregisterWindow 
// ---------------------------------------------------------------------------
// Remove the window from the list
// Show all the floaters

void CTearOffManager::UnregisterWindow(CTearOffWindow * window)
{
	// Remove the window from the list
	FloatWindowRecord fw( window, WILDCARD );
	ArrayIndexT where = fPaletteWindows->FetchIndexOfKey( &fw );

	if ( where != LArray::index_Bad )
	{
		fPaletteWindows->FetchItemAt( where, &fw );
		fPaletteWindows->RemoveItemsAt( 1, where );
	}
	else
		SignalCStr_("Removal of unregistered window?");
	
	// Tell all button bars to show themselves
	
	LArrayIterator iter(*fPaletteBars);
	ButtonBarRecord bar;
	while ( iter.Next(&bar) )
		if (bar.fID == fw.fID)
			{
				bar.fBar->ShowPalette();
				SetBarsView(bar.fBar, bar.fBar);
			}
}

// ---------------------------------------------------------------------------
//		¥ WindowReceivedMessage 
// ---------------------------------------------------------------------------
// Rebroadcast the message to the listener of the currently active bar
void CTearOffManager::WindowReceivedMessage(CTearOffWindow * inWin,
											MessageT inMessage,
											 void *ioParam)
{
	// Get the window record
	FloatWindowRecord fw( inWin, WILDCARD );
	ArrayIndexT where = fPaletteWindows->FetchIndexOfKey( &fw );

	if ( where != LArray::index_Bad )
	{
		fPaletteWindows->FetchItemAt( where, &fw );
		
		// If there is an active bar attached to this window
		// rebroadcast the message to its listener

		if (fw.fActiveBar)
		{
			ButtonBarRecord bar( WILDCARD, fw.fActiveBar , NULL );
			where = fPaletteBars->FetchIndexOfKey(&bar);
			if ( where != LArray::index_Bad )
			{
				fPaletteBars->FetchItemAt( where, &bar );
				if ( bar.fListener != NULL )
					bar.fListener->ListenToMessage( inMessage, ioParam );
			}
		}
		
	}
	else
		SignalCStr_("Message from a window we do not know about?");
}

#pragma mark -
// ---------------------------------------------------------------------------
//		¥ InitUTearOffPalette [static]
// ---------------------------------------------------------------------------
// Registers classes, creates a single float manager
void InitUTearOffPalette(LCommander * defCommander)
{
	URegistrar::RegisterClass( CTearOffBar::class_ID, (ClassCreatorFunc)CTearOffBar::CreateFromStream);
	URegistrar::RegisterClass( CTearOffWindow::class_ID, (ClassCreatorFunc)CTearOffWindow::CreateFromStream);
	new CTearOffManager(defCommander);
}

