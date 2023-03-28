// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	CClusterView.h						  ©1996 Netscape. All rights reserved.
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

#pragma once

#include <LView.h>
#include <LString.h>

class CClusterView : public LView 
{
	public:
		enum { class_ID = 'Clus' };
		static CClusterView*	CreateClusterViewStream(LStream* inStream);	
							CClusterView(LStream *inStream);
		virtual				~CClusterView(void);							
		
		virtual StringPtr	GetDescriptor(Str255 outDescriptor) const;
		virtual void		SetDescriptor(ConstStr255Param inDescriptor);
									
	protected:
		virtual void		FinishCreateSelf(void);
		virtual void		DrawSelf(void);

		virtual	void		CalcTitleFrame(void);

		TString<Str255>		mTitle;
		ResIDT				mTraitsID;
		Rect 				mTitleFrame;		
		Int16				mTitleBaseline;
};		
