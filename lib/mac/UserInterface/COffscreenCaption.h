// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	COffscreenCaption.h					  ©1996 Netscape. All rights reserved.
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

#pragma once

#include <LCaption.h>

class COffscreenCaption : public LCaption
{
	public:
		enum { class_ID = 'OfCp' };
		static COffscreenCaption* CreateOffscreenCaptionStream(LStream* inStream);
						COffscreenCaption(LStream* inStream);
	
						COffscreenCaption(
								const SPaneInfo&	inPaneInfo,
								ConstStringPtr		inString,
								ResIDT				inTextTraitsID);

		virtual void		SetDescriptor(ConstStringPtr inDescriptor);
		virtual void		SetDescriptor(const char* inCDescriptor);

		virtual	void		SetEraseColor(Int16 inPaletteIndex);
		virtual	void 		Draw(RgnHandle inSuperDrawRgnH);

	protected:
	
		virtual	void		DrawSelf(void);
				
		Int16				mEraseColor;
};

inline void COffscreenCaption::SetEraseColor(Int16 inPaletteIndex)
	{	mEraseColor = inPaletteIndex; }
