// ===========================================================================
//	VPageNumberCaption
//
//	Copyright 1996 ViviStar Consulting.
// ===========================================================================

#pragma	once

#include	<LCaption.h>
#include	<V_ClassIDs.h>

class	VPageNumberCaption
			:	public	LCaption
{
public:
	enum {
		class_ID = kVCID_PageNumberCaption
	};
	
						VPageNumberCaption(LStream *inStream);
						VPageNumberCaption();
						~VPageNumberCaption();

	virtual void		PrintPanelSelf(const PanelSpec &inPanel);
};