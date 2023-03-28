// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	CTabSwitcher.h						  ©1996 Netscape. All rights reserved.
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

#pragma once

#include <LView.h>
#include <LListener.h>
#include <LArray.h>

class LCommander;

class CTabSwitcher : public LView, public LListener
{
	public:
		enum { class_ID = 'TbSw' };
		static CTabSwitcher* CreateTabSwitcherStream(LStream* inStream);
							CTabSwitcher(LStream* inStream);
		virtual				~CTabSwitcher();
	
		virtual void		ListenToMessage(
									MessageT		inMessage,
									void*			ioParam);

		virtual void		SwitchToPage(ResIDT inPageResID);
		virtual LView*		FindPageByID(ResIDT inPageResID);
		virtual LView*		GetCurrentPage(void) const;
		
		virtual	void		DoPreDispose(
									LView* 			inLeavingPage,
									Boolean			inWillCache);
									
		virtual	void		DoPostLoad(
									LView* 			inLoadedPage,
									Boolean			inFromCache);
	
	protected:
		virtual	void		FinishCreateSelf(void);

		virtual LView*		FetchPageFromCache(ResIDT inPageResID);
		virtual void		RemovePageFromCache(LView* inPage);

		virtual	void		FlushCachedPages(void);

		PaneIDT				mTabControlID;
		PaneIDT				mContainerID;
		Boolean				mIsCachingPages;
		
		LArray				mCachedPages;
		Int32				mSavedValue;
		LView*				mCurrentPage;
		LCommander*			mSavedSuper;
};

inline LView* CTabSwitcher::GetCurrentPage(void) const
	{	return mCurrentPage;		}
