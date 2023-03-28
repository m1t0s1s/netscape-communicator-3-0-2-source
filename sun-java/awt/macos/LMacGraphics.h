#pragma once

#include <LPane.h>
#include <Quickdraw.h> 
#include <QDOffscreen.h>
#include "native.h"

#ifdef UNICODE_FONTLIST
#include "LJavaFontList.h"
#endif


extern "C" {
#include "java_awt_Color.h"
#include "sun_awt_macos_MacGraphics.h"
#include "sun_awt_macos_MComponentPeer.h"
#include "s_a_image_ImageRepresentation.h"
}

class LMacGraphics {

public :
	
	Hsun_awt_macos_MacGraphics		*mMacGraphics;
	
	short			mFontNumber;
	short			mFontSize;
	Style			mFontStyle;
	
	Rect			mBoundsRectangle;
	Rect			mClipRectangle;
	Rect			mRawClip;
	Boolean			mHasBounds;
	Boolean			mHasClip;

	RgnHandle		mSavedClipRgn;
	
	RGBColor		mForeColor;
	RGBColor		mXORColor;
	
	SInt32			mOriginX;
	SInt32			mOriginY;
	
	float			mScaleX;
	float			mScaleY;

	short			mPenMode;
	

#ifdef UNICODE_FONTLIST
	LJavaFontList	*mUnicodeFontList;
#endif
	
	LMacGraphics(Hsun_awt_macos_MacGraphics *graphics);
	~LMacGraphics();
	
	void			InitCommon();
	
	Boolean 		BeginDrawing(void);
	void			FinishDrawing(void);
	
	LPane			*GetOwnerPane();
	GWorldPtr		GetOwnerGWorld();
	GrafPtr			GetOwnerPort(void);
	
	inline void		SetFont(short fontNum) { mFontNumber = fontNum; }
	inline void 	SetForeColor(RGBColor foreColor) { mForeColor = foreColor; }
	inline void 	SetXORColor(RGBColor foreColor) { mXORColor = foreColor; }
	inline void 	SetPenMode(short newMode) { mPenMode = newMode; }
	inline void 	SetScaling(float xScaling, float yScaling) { mScaleX = xScaling; mScaleY = yScaling; }
	inline void 	SetOrigin(UInt32 x, UInt32 y) { mOriginX = x; mOriginY = y; }

	void			ConvertToPortRect(Rect &bounds, long x, long y, long width, long height);
	void			ConvertToPortPoint(Point &location, long x, long y);
	PolyHandle 		ConvertToPortPoly(HArrayOfInt *xArray, HArrayOfInt *yArray, long count);

};

enum {
	kDefaultAWTTextTraitsID = 4324
};

void ConvertFontToNumSizeAndStyle(struct Hjava_awt_Font *fontObject, short *fontNum, short *size, Style *style);

#ifdef UNICODE_FONTLIST
	void SetUpTextITraits(short font, short size, Style style, short encoding);
#else
	void SetUpTextITraits(short font, short size, Style style);
#endif

RGBColor ConvertAWTToMacColor(Hjava_awt_Color *color);
GWorldPtr GetDestinationBackingStoreWorld(struct Hsun_awt_image_ImageRepresentation *imageRepresentationObject, struct Hjava_awt_image_ColorModel *colorModel, short depth, Boolean transparent);
