// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	CTableKeyAttachment.h					
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

#pragma once

#include <LAttachment.h>

class LTableView;

class CTableKeyAttachment : public LAttachment
{
	public:
							CTableKeyAttachment(LTableView* inView);
		virtual				~CTableKeyAttachment();
		
	protected:
		virtual void		ExecuteSelf(
								MessageT		inMessage,
								void			*ioParam);

		LTableView*		mTableView;
};