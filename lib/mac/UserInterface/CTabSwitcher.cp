// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	CTabSwitcher.cp						  ©1996 Netscape. All rights reserved.
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

#ifdef PowerPlant_PCH
#include PowerPlant_PCH
#endif

#include <LArrayIterator.h>
#include <LCommander.h>
#include <LStream.h>
#include <PP_Messages.h>
#include <UMemoryMgr.h>
#include <UReanimator.h>

#include "CTabSwitcher.h"
#include "CTabControl.h"

// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	¥	
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

CTabSwitcher* CTabSwitcher::CreateTabSwitcherStream(LStream* inStream)
{
	return (new CTabSwitcher(inStream));
}

// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	¥	
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

CTabSwitcher::CTabSwitcher(LStream* inStream)
	:	LView(inStream),
		mCachedPages(sizeof(LView*))
{
	*inStream >> mTabControlID;
	*inStream >> mContainerID;
	*inStream >> mIsCachingPages;

	mCurrentPage = NULL;
	mSavedSuper = LCommander::GetDefaultCommander();
	mSavedValue = 0;
}

// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	¥	
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

CTabSwitcher::~CTabSwitcher()
{
	FlushCachedPages();
}

// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	¥	
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

void CTabSwitcher::FinishCreateSelf(void)
{
	LView::FinishCreateSelf();
	
	CTabControl* theTabControl = (CTabControl*)FindPaneByID(mTabControlID);
	Assert_(theTabControl != NULL);
	theTabControl->AddListener(this);

	MessageT theTabMessage = theTabControl->GetValueMessage();
	if (theTabMessage != msg_Nothing)
		{
		SwitchToPage(theTabControl->GetValueMessage());
		mSavedValue = theTabControl->GetValue();
		}
}

// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	¥	
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

void CTabSwitcher::ListenToMessage(
	MessageT		inMessage,
	void*			ioParam)
{
	if (inMessage == msg_TabSwitched)
		{
		ResIDT thePageResID = (ResIDT)(*(MessageT*)ioParam);
		SwitchToPage(thePageResID);
		}
	else if (inMessage == msg_GrowZone)
		{
		FlushCachedPages();		
		}
}

// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	¥	
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

void CTabSwitcher::SwitchToPage(ResIDT inPageResID)
{
	CTabControl* theTabControl = (CTabControl*)FindPaneByID(mTabControlID);
	Assert_(theTabControl != NULL);

	LView* thePageContainer = (LView*)FindPaneByID(mContainerID);
	Assert_(thePageContainer != NULL);
	
	LView* thePage = NULL;
	Boolean	isFromCache = true;
		
	if (mIsCachingPages)
		thePage = FetchPageFromCache(inPageResID);

	if (thePage == NULL)			// we need to reanimate
		{
		try
			{
			// We temporarily disable signaling since we're reanimating into a NULL
			// view container.  FocusDraw() signals if it cant establish a port.
			StValueChanger<EDebugAction> okayToSignal(gDebugSignal, debugAction_Nothing);
			thePage = UReanimator::CreateView(inPageResID, NULL, mSavedSuper);
			isFromCache = false;
			}
		catch (ExceptionCode inErr)
			{
			// something went wrong
			delete thePage;
			thePage = NULL;
			}
		}
	else
		RemovePageFromCache(thePage);
	
	// Sanity check in case we mad a page without setting this up.
	Assert_(thePage->GetPaneID() == inPageResID);
		
	if (thePage != NULL)
		{
		if (mCurrentPage != NULL)
			{
			// Hide it and take it out of the view hierarchy
			mCurrentPage->Hide();
			mCurrentPage->PutInside(NULL, false);
	
			DoPreDispose(mCurrentPage, mIsCachingPages);
			
			if (mIsCachingPages)
				mCachedPages.InsertItemsAt(1, arrayIndex_Last, &mCurrentPage);
			else
				delete mCurrentPage;
			}
		
		thePage->PutInside(thePageContainer);
		thePage->PlaceInSuperFrameAt(0, 0, false);

		DoPostLoad(thePage, isFromCache);

		thePage->Show();
		thePage->Refresh();

		mCurrentPage = thePage;
		mSavedValue = theTabControl->GetValue();
		}
	else 							// we falied to reanimate
		{
		theTabControl->SetValue(mSavedValue);
		}
}

// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	¥	
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

LView* CTabSwitcher::FindPageByID(ResIDT inPageResID)
{
	LView* theFoundPage = NULL;
	
	if ((mCurrentPage != NULL) && (mCurrentPage->GetPaneID() == inPageResID))
		theFoundPage = mCurrentPage;
	else	
		theFoundPage = FetchPageFromCache(inPageResID);
	
	return theFoundPage;
}

// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	¥	
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

void CTabSwitcher::DoPostLoad(
	LView* 			inLoadedPage,
	Boolean			inFromCache)
{
}

// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	¥	
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

void CTabSwitcher::DoPreDispose(
	LView* 			inLeavingPage,
	Boolean			inWillCache)
{
}

// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	¥	
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

LView* CTabSwitcher::FetchPageFromCache(ResIDT inPageResID)
{
	LView* theCachedPage = NULL;
	LView* theIndexPage = NULL;
	
	LArrayIterator theIter(mCachedPages, LArrayIterator::from_Start);
	while (theIter.Next(&theIndexPage))
		{
		if (theIndexPage->GetPaneID() == inPageResID)
			{
			theCachedPage = theIndexPage;
			break;
			}
		}

	return theCachedPage;
}

// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	¥	
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

void CTabSwitcher::RemovePageFromCache(LView* inPage)
{
	Assert_(inPage != NULL);
	
	ArrayIndexT thePageIndex = mCachedPages.FetchIndexOf(&inPage);
	Assert_(thePageIndex != arrayIndex_Bad);

	mCachedPages.RemoveItemsAt(1, thePageIndex);
}

// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	¥	
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

void CTabSwitcher::FlushCachedPages(void)
{
	// We temporarily disable signaling since we're reanimating into a NULL
	// view container.  FocusDraw() signals if it cant establish a port.
	StValueChanger<EDebugAction> okayToSignal(gDebugSignal, debugAction_Nothing);

	LView* thePage;
	LArrayIterator theIter(mCachedPages, LArrayIterator::from_Start);
	while (theIter.Next(&thePage))
		delete thePage;

	mCachedPages.RemoveItemsAt(arrayIndex_First, mCachedPages.GetCount());
}

