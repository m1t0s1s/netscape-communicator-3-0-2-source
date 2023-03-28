// ===========================================================================
//	VPageNumberCaption
//
//	Copyright 1996 ViviStar Consulting.
// ===========================================================================

#ifdef PowerPlant_PCH
#include PowerPlant_PCH
#endif

#include	"VPageNumberCaption.h"

#pragma	warn_unusedarg off

VPageNumberCaption::VPageNumberCaption(LStream *inStream)
: LCaption(inStream)
{
}

VPageNumberCaption::VPageNumberCaption()
{
}

VPageNumberCaption::~VPageNumberCaption()
{
}

void	VPageNumberCaption::PrintPanelSelf(const PanelSpec &inPanel)
{
	static Str255	numString;
	
	::NumToString(inPanel.pageNumber, numString);
	SetDescriptor(numString);
	DrawSelf();
}
