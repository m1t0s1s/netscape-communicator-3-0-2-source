//##############################################################################
//##############################################################################
//
//	File:		LMacGraphics.cp
//	Author:		Dan Clifford
//	
//	Copyright © 1995-1996, Netscape Communications Corporation
//
//##############################################################################
//##############################################################################

extern "C" {

#include <stdlib.h>
#include "native.h"
#include "typedefs_md.h"
#include "java_awt_Font.h"
#include "java_awt_Component.h"
#include "java_awt_Rectangle.h"
#include "sun_awt_macos_MComponentPeer.h"
#include "sun_awt_macos_MacGraphics.h"
#include "interpreter.h"
#include "exceptions.h"

};

#include "LMacGraphics.h"
#include "MComponentPeer.h"
#include "UTextTraits.h"
#include <TextUtils.h>

#ifdef UNICODE_FONTLIST
#include "LMacCompStr.h"
#include "LVFont.h"
#endif

RgnHandle		gTempGraphicsRegion = NULL;

RGBColor ConvertAWTToMacColor(Hjava_awt_Color *colorHandle)
{
	Classjava_awt_Color		*color = unhand(colorHandle);
	short					red, green, blue;
	RGBColor				macColor;
	
	red		= (color->value >> 16) & 0xff;
	green	= (color->value >>  8) & 0xff;
	blue	= (color->value >>  0) & 0xff;
	
	macColor.red	= (red   << 8) | red;
	macColor.green	= (green << 8) | green;
	macColor.blue	= (blue  << 8) | blue;
	
	return macColor;
}

void
ConvertFontToNumSizeAndStyle(struct Hjava_awt_Font *fontObject, short *fontNum, short *size, Style *style)
{
	Classjava_awt_Font						*font;
	Str255									fontPString;
	SInt16									fontSize;
	Style									fontStyle;
	short									familyID;	

	//	Get the name of the font, and convert it into 
	//	a pascal string.

	font = unhand(fontObject);
	javaString2CString(font->name, (char *)(fontPString + 1), sizeof(fontPString) - 1);
	fontPString[0] = strlen((char *)(fontPString + 1));
	
	if (EqualString(fontPString, "\pTimesRoman", false, false))
		LString::CopyPStr("\pTimes", fontPString, 255);

	if (EqualString(fontPString, "\pDialog", true, true)) {
	
		familyID = geneva;
		fontStyle = normal;
		fontSize = 9;		
	
	}
	
	else {
	
		::GetFNum(fontPString, &familyID);
		::TextFont(familyID);
		fontSize = font->size;

		fontStyle = 0;
		
		if ((font->style & java_awt_Font_BOLD) != 0)
			fontStyle |= bold;

		if ((font->style & java_awt_Font_ITALIC) != 0)
			fontStyle |= italic;
		
	}
	
	//	Copy all of the important information into the component itself.
	
	*fontNum = familyID;
	*size = fontSize;
	*style = fontStyle;
}

TextTraitsH	gDefaultAWTTextTraits = NULL;

#ifdef UNICODE_FONTLIST
void
SetUpTextITraits(short font, short size, Style style, short encoding)
{
	if (font == 0) {
		if(UEncoding::IsMacRoman(encoding))
		{
			if (gDefaultAWTTextTraits == NULL) 
				gDefaultAWTTextTraits = UTextTraits::LoadTextTraits(kDefaultAWTTextTraitsID);
			
			if (gDefaultAWTTextTraits == NULL)
				return;
			
			HLock((Handle)gDefaultAWTTextTraits);
			UTextTraits::SetPortTextTraits(*gDefaultAWTTextTraits);
			HUnlock((Handle)gDefaultAWTTextTraits);
		}
		else
		{
			TextTraitsH csidTT 			= UTextTraits::LoadTextTraits(UEncoding::GetButtonTextTraitsID(encoding));
			UTextTraits::SetPortTextTraits(*csidTT);
		}
	}
	
	else {
	
		TextFont(font);
		TextSize(size);
		TextFace(style);
		
	}
}
#else
void
SetUpTextITraits(short font, short size, Style style)
{
	if (font == 0) {
	
		if (gDefaultAWTTextTraits == NULL) 
			gDefaultAWTTextTraits = UTextTraits::LoadTextTraits(kDefaultAWTTextTraitsID);
		
		if (gDefaultAWTTextTraits == NULL)
			return;
		
		HLock((Handle)gDefaultAWTTextTraits);
		UTextTraits::SetPortTextTraits(*gDefaultAWTTextTraits);
		HUnlock((Handle)gDefaultAWTTextTraits);
		
	}
	
	else {
	
		TextFont(font);
		TextSize(size);
		TextFace(style);
		
	}
}
#endif



//##############################################################################
//##############################################################################

LMacGraphics::
LMacGraphics(Hsun_awt_macos_MacGraphics *graphics)
{
	InitCommon();
	mMacGraphics = graphics;
	if (GetOwnerPane() != NULL) {
		GetOwnerPane()->CalcLocalFrameRect(mBoundsRectangle);
	}
	else if (GetOwnerGWorld() != NULL) {
		mBoundsRectangle = (**(GetOwnerGWorld()->portPixMap)).bounds;
	}
	
}

LPane *
LMacGraphics::GetOwnerPane() 
{ 
	Hsun_awt_macos_MComponentPeer	*componentPeer;

	if (mMacGraphics == NULL) 
		return NULL;
		
	componentPeer = unhand(mMacGraphics)->componentPeer;
	
	if (componentPeer == NULL)
		return NULL;

	if (unhand(componentPeer)->mOwnerPane == kJavaComponentPeerHasBeenDisposed)
		return NULL;
		
	return (LPane *)(unhand(componentPeer)->mOwnerPane);
	
}

GWorldPtr
LMacGraphics::GetOwnerGWorld()
{
	Hsun_awt_image_ImageRepresentation	*imageRepresentationObject;

	if (mMacGraphics == NULL) 
		return NULL;
		
	imageRepresentationObject = unhand(mMacGraphics)->imageRepresentation;
	
	if (imageRepresentationObject == NULL)
		return NULL;
		
	return GetDestinationBackingStoreWorld(imageRepresentationObject, NULL, 16, false);

}

GrafPtr
LMacGraphics::GetOwnerPort(void) 
{ 	
	LPane	*ownerPane = GetOwnerPane();
	if (ownerPane != NULL) {
		return ownerPane->GetMacPort(); 
	} else 
		return (GrafPtr)GetOwnerGWorld(); 
}

void 
LMacGraphics::InitCommon()
{
	mFontNumber = 0;
	mFontSize = 0;
	mFontStyle = 0;

#ifdef UNICODE_FONTLIST
	mUnicodeFontList = NULL;
	try {
		mUnicodeFontList = new LJavaFontList();
	}
	catch (...) {
		SignalError(0, JAVAPKG "OutOfMemoryError", 0);
		return;
	}
#endif
	
	memset(&mForeColor, 0, sizeof(mForeColor));
	
	mScaleX = 1.0;
	mScaleY = 1.0;
	
	mOriginX = 0;
	mOriginY = 0;
	
	mHasBounds = false;

	mPenMode = srcCopy;
	
	SetRect(&mClipRectangle, 0, 0, 0, 0);
	mHasClip = false;
	
	mSavedClipRgn = ::NewRgn();
	
	if (gTempGraphicsRegion == NULL)
		gTempGraphicsRegion = ::NewRgn();
	
}


LMacGraphics::
~LMacGraphics()
{
	::DisposeRgn(mSavedClipRgn);
#ifdef UNICODE_FONTLIST
	delete mUnicodeFontList;
#endif

}

CGrafPtr	gSavedWorld;
GDHandle	gSavedGDHandle;

Boolean 
LMacGraphics::BeginDrawing(void)
{
	LPane			*ownerPane = GetOwnerPane();
	GWorldPtr		ownerWorld = GetOwnerGWorld();

	//	Make sure the current port is ours.

	if (ownerPane != NULL) {
		if (!ownerPane->FocusDraw() || !ownerPane->IsVisible())
			return false;
	} 
	
	else  if (ownerWorld != NULL) {
		GetGWorld(&gSavedWorld, &gSavedGDHandle);
		SetGWorld(ownerWorld, NULL);
	}
	
	else {
		return false;
	}

	//	Set up the drawing mode 'n stuff
#ifdef UNICODE_FONTLIST
	SetUpTextITraits(mFontNumber, mFontSize, mFontStyle, intl_GetDefaultEncoding());
#else
	SetUpTextITraits(mFontNumber, mFontSize, mFontStyle);
#endif
	PenMode(mPenMode);
	if (mPenMode == srcCopy) {
		RGBForeColor(&mForeColor);
		TextMode(srcOr);
	} else {
		TextMode(srcXor);
	}
	
	//	And add our clip region in, if there is one.
	
	if (mHasClip || mHasBounds) {
	
		Rect	localClipCopy;
		
		localClipCopy = mClipRectangle;

		if (mHasBounds)
			::SectRect(&localClipCopy, &mBoundsRectangle, &localClipCopy);
		
		::RectRgn(gTempGraphicsRegion, &localClipCopy);
		::GetClip(mSavedClipRgn);
		::SectRgn(mSavedClipRgn, gTempGraphicsRegion, gTempGraphicsRegion);
		::SetClip(gTempGraphicsRegion);
	}
	
	return true;
	
}

void 
LMacGraphics::FinishDrawing(void)
{
	if (mHasClip || mHasBounds)
		::SetClip(mSavedClipRgn);

	if (GetOwnerGWorld() != NULL)
		SetGWorld(gSavedWorld, gSavedGDHandle);
}

void
LMacGraphics::ConvertToPortRect(Rect &bounds, long x, long y, long width, long height)
{
	bounds.top		= mOriginY + y * mScaleY;
	bounds.left		= mOriginX + x * mScaleX;
	bounds.bottom	= bounds.top + mScaleY * height;
	bounds.right	= bounds.left + mScaleX * width;

}

void
LMacGraphics::ConvertToPortPoint(Point &location, long x, long y)
{
	location.v = mOriginY + y * mScaleY;
	location.h = mOriginX + x * mScaleX;
}

PolyHandle
LMacGraphics::ConvertToPortPoly(HArrayOfInt *xArray, HArrayOfInt *yArray, long count)
{
	SInt32			*xCoordinates = (SInt32 *)unhand(xArray)->body;
	SInt32			*yCoordinates = (SInt32 *)unhand(yArray)->body;
	PolyHandle		resultPoly;
	Point			currentLocation;
	UInt32			currentPoint;

	resultPoly = OpenPoly();

	ConvertToPortPoint(currentLocation, *xCoordinates++, *yCoordinates++);
	MoveTo(currentLocation.h, currentLocation.v);

	for(currentPoint = 1; currentPoint < count; currentPoint++) {
		ConvertToPortPoint(currentLocation, *xCoordinates++, *yCoordinates++);
		LineTo(currentLocation.h, currentLocation.v);
	}

	ClosePoly();
	
	return resultPoly;
	
}

//##############################################################################
//##############################################################################
#pragma mark -
#pragma mark CONSTRUCTION AND DESTRUCTION

void sun_awt_macos_MacGraphics_createFromComponent(struct Hsun_awt_macos_MacGraphics *graphicsPeerObject, struct Hsun_awt_macos_MComponentPeer *componentPeerHandle)
{
	LMacGraphics	*ppMacGraphics = NULL;
	
	try {
		ppMacGraphics = new LMacGraphics(graphicsPeerObject);
	}

	catch (...) {
		SignalError(0, JAVAPKG "OutOfMemoryError", 0);
		return;
	}
	
	unhand(graphicsPeerObject)->pData = (long)ppMacGraphics;
}

void sun_awt_macos_MacGraphics_createFromGraphics(struct Hsun_awt_macos_MacGraphics *newGraphics, struct Hsun_awt_macos_MacGraphics *oldGraphics)
{
	LMacGraphics	*ppMacGraphics = NULL;
	LMacGraphics 	*oldMacGraphics = (LMacGraphics *)(unhand(oldGraphics)->pData);
	
	try {
		ppMacGraphics = new LMacGraphics(newGraphics);
	}
	catch (...) {
		SignalError(0, JAVAPKG "OutOfMemoryError", 0);
		return;
	}
		
	unhand(newGraphics)->pData = (long)ppMacGraphics;
	
	ppMacGraphics->mFontNumber = oldMacGraphics->mFontNumber;
	ppMacGraphics->mFontSize = oldMacGraphics->mFontSize;
	ppMacGraphics->mFontStyle = oldMacGraphics->mFontStyle;
#ifdef UNICODE_FONTLIST
	if (ppMacGraphics->mUnicodeFontList != NULL)
		delete ppMacGraphics->mUnicodeFontList;
	ppMacGraphics->mUnicodeFontList = NULL;
	try {
		ppMacGraphics->mUnicodeFontList = new LJavaFontList(*(oldMacGraphics->mUnicodeFontList));
	}
	catch (...) {
		SignalError(0, JAVAPKG "OutOfMemoryError", 0);
		return;
	}
#endif

	ppMacGraphics->mBoundsRectangle = oldMacGraphics->mClipRectangle;
	ppMacGraphics->mHasBounds = !EmptyRect(&(ppMacGraphics->mBoundsRectangle));
	ppMacGraphics->mHasClip = false;
	::SetRect(&(ppMacGraphics->mClipRectangle), 0, 0, 0, 0);
	
	ppMacGraphics->mForeColor = oldMacGraphics->mForeColor;
	
	ppMacGraphics->mOriginX = oldMacGraphics->mOriginX;
	ppMacGraphics->mOriginY = oldMacGraphics->mOriginY;
	ppMacGraphics->mScaleX = oldMacGraphics->mScaleX;
	ppMacGraphics->mScaleY = oldMacGraphics->mScaleY;
	ppMacGraphics->mPenMode = oldMacGraphics->mPenMode;

}

void sun_awt_macos_MacGraphics_createFromIRepresentation(struct Hsun_awt_macos_MacGraphics *graphicsPeerObject, struct Hsun_awt_image_ImageRepresentation *imageRepresentationObject)
{
	LMacGraphics								*ppMacGraphics = NULL;
	GWorldPtr									imageGWorld = GetDestinationBackingStoreWorld(imageRepresentationObject, NULL, 16, false);
	
	if (imageGWorld == NULL) {
		SignalError(0, JAVAPKG "OutOfMemoryError", 0);
	}
	
	else {
		try {
			ppMacGraphics = new LMacGraphics(graphicsPeerObject);
		}
		catch (...) {
			SignalError(0, JAVAPKG "OutOfMemoryError", 0);
			return;
		}
		unhand(graphicsPeerObject)->pData = (long)ppMacGraphics;
	}
}

void sun_awt_macos_MacGraphics_dispose(struct Hsun_awt_macos_MacGraphics *graphicsPeerObject)
{
	delete((LMacGraphics *)(unhand(graphicsPeerObject)->pData));
	unhand(graphicsPeerObject)->pData = NULL;
}

//##############################################################################
//##############################################################################
#pragma mark -
#pragma mark STATE ROUTINES

void sun_awt_macos_MacGraphics_pSetFont(struct Hsun_awt_macos_MacGraphics *graphicsPeerObject,struct Hjava_awt_Font *fontObject)
{
	LMacGraphics						*ppMacGraphics = (LMacGraphics *)unhand(graphicsPeerObject)->pData;

#ifdef UNICODE_FONTLIST
	// This must be put before ConvertFontToNumSizeAndStyle
	// Otherwise, ppMacGraphics->SetTextLatin1Font in the end is required
	 ppMacGraphics->mUnicodeFontList->SetFont(unhand(fontObject));
#endif
	ConvertFontToNumSizeAndStyle(fontObject, &(ppMacGraphics->mFontNumber), &(ppMacGraphics->mFontSize), &(ppMacGraphics->mFontStyle));
}

void sun_awt_macos_MacGraphics_pSetForeground(struct Hsun_awt_macos_MacGraphics *graphicsPeerObject, struct Hjava_awt_Color *colorHandle)
{
	LMacGraphics						*ppMacGraphics = (LMacGraphics *)unhand(graphicsPeerObject)->pData;
	RGBColor							macColor;
	
	macColor = ConvertAWTToMacColor(colorHandle);
	ppMacGraphics->SetForeColor(macColor);
}

void sun_awt_macos_MacGraphics_pSetScaling(struct Hsun_awt_macos_MacGraphics *graphicsPeerObject, float x, float y)
{
	LMacGraphics						*ppMacGraphics = (LMacGraphics *)unhand(graphicsPeerObject)->pData;
	ppMacGraphics->SetScaling(x, y);
}

void sun_awt_macos_MacGraphics_pSetOrigin(struct Hsun_awt_macos_MacGraphics *graphicsPeerObject, long x, long y)
{
	LMacGraphics						*ppMacGraphics = (LMacGraphics *)unhand(graphicsPeerObject)->pData;
	ppMacGraphics->SetOrigin(x, y);
}

void sun_awt_macos_MacGraphics_setPaintMode(struct Hsun_awt_macos_MacGraphics *graphicsPeerObject)
{
	LMacGraphics						*ppMacGraphics = (LMacGraphics *)unhand(graphicsPeerObject)->pData;
	ppMacGraphics->SetPenMode(srcCopy);
}

void sun_awt_macos_MacGraphics_setXORMode(struct Hsun_awt_macos_MacGraphics *graphicsPeerObject, struct Hjava_awt_Color *colorObject)
{
	LMacGraphics						*ppMacGraphics = (LMacGraphics *)unhand(graphicsPeerObject)->pData;
	RGBColor							xorColor;
	long								xorIndex,
										foreColorIndex;
	
	xorColor = ConvertAWTToMacColor(colorObject);
	xorIndex = Color2Index(&xorColor);
	
	//	Until quickdraw supports Xoring with the forecolor when
	//	drawing, this is all for nothing...
	
	if (QDError() == cMatchErr) {
		ppMacGraphics->mXORColor.red = xorColor.red ^ ppMacGraphics->mForeColor.red;
		ppMacGraphics->mXORColor.green = xorColor.green ^ ppMacGraphics->mForeColor.green;
		ppMacGraphics->mXORColor.blue = xorColor.blue ^ ppMacGraphics->mForeColor.blue;
	}
	
	else {
		foreColorIndex = Color2Index(&(ppMacGraphics->mForeColor));
		xorIndex = xorIndex ^ foreColorIndex;
		Index2Color(xorIndex, &(ppMacGraphics->mXORColor));
	}

	ppMacGraphics->SetPenMode(srcXor);

}

void sun_awt_macos_MacGraphics_getClipRect(struct Hsun_awt_macos_MacGraphics *graphicsPeerObject,struct Hjava_awt_Rectangle *Hrect)
{
	LMacGraphics				*ppMacGraphics = (LMacGraphics *)unhand(graphicsPeerObject)->pData;
	Classjava_awt_Rectangle		*rectangleObject = unhand(Hrect);
	
	if (!(ppMacGraphics->mHasClip)) {
	
		LPane	*ownerPane = ppMacGraphics->GetOwnerPane();
	
		if (ownerPane) {
			ownerPane->CalcLocalFrameRect(ppMacGraphics->mRawClip);
		}
		
		else {
			ppMacGraphics->mRawClip = (**(ppMacGraphics->GetOwnerGWorld()->portPixMap)).bounds;
		}
	
	}

	rectangleObject->x = ppMacGraphics->mRawClip.left;
	rectangleObject->y = ppMacGraphics->mRawClip.top;
	rectangleObject->width = ppMacGraphics->mRawClip.right;
	rectangleObject->height = ppMacGraphics->mRawClip.bottom;
	
}

void sun_awt_macos_MacGraphics_clipRect(struct Hsun_awt_macos_MacGraphics *graphicsPeerObject, long x, long y, long width, long height)
{
	
	LMacGraphics		*ppMacGraphics = (LMacGraphics *)unhand(graphicsPeerObject)->pData;
	
	if ((x != 0) || (y != 0) || (width != 0) || (height != 0)) {
		ppMacGraphics->ConvertToPortRect(ppMacGraphics->mClipRectangle, x, y, width, height);
		SetRect(&(ppMacGraphics->mRawClip), x, y, width, height);
		ppMacGraphics->mHasClip = true;
	}
	
	else {
		ppMacGraphics->mHasClip = false;
	}
	
}

//##############################################################################
//##############################################################################
#pragma mark -
#pragma mark BITMAP ROUTINES


void sun_awt_macos_MacGraphics_copyArea(struct Hsun_awt_macos_MacGraphics *graphicsPeerObject,long x,long y,long width,long height,long dx,long dy)
{
	LMacGraphics		*ppMacGraphics = (LMacGraphics *)unhand(graphicsPeerObject)->pData;
	Rect				fromRect, toRect;
	BitMap				*copyBitMap;
	
	if (ppMacGraphics->BeginDrawing()) {

		copyBitMap = &((ppMacGraphics->GetOwnerPort())->portBits);

		ppMacGraphics->ConvertToPortRect(fromRect, x, y, width, height);
		toRect = fromRect;
		::OffsetRect(&toRect, dx, dy);
	
		ForeColor(blackColor);
		BackColor(whiteColor);
	
		::CopyBits(copyBitMap, copyBitMap, &fromRect, &toRect, srcCopy, NULL);
	
		ppMacGraphics->FinishDrawing();

	}
	
}


//##############################################################################
//##############################################################################
#pragma mark -
#pragma mark GEOMETRY DRAWING ROUTINES

void sun_awt_macos_MacGraphics_drawLine(struct Hsun_awt_macos_MacGraphics *graphicsPeerObject, long x1, long y1, long x2, long y2)
{
	LMacGraphics		*ppMacGraphics = (LMacGraphics *)unhand(graphicsPeerObject)->pData;
	Point				from, to;
	
	if (ppMacGraphics->BeginDrawing()) {
		ppMacGraphics->ConvertToPortPoint(from, x1, y1);
		ppMacGraphics->ConvertToPortPoint(to, x2, y2);
		::MoveTo(from.h, from.v);
		::LineTo(to.h, to.v);
		ppMacGraphics->FinishDrawing();
	}
}

void sun_awt_macos_MacGraphics_clearRect(struct Hsun_awt_macos_MacGraphics *graphicsPeerObject, long x, long y, long width, long height)
{
	LMacGraphics		*ppMacGraphics = (LMacGraphics *)unhand(graphicsPeerObject)->pData;
	RGBColor			backgroundColor = { 192 << 8, 192 << 8, 192 << 8 };
	Rect				eraseRect;
	
	if (ppMacGraphics->BeginDrawing()) {
		ppMacGraphics->ConvertToPortRect(eraseRect, x, y, width, height);
		Hsun_awt_macos_MComponentPeer *componentPeerHandle = PaneToPeer(ppMacGraphics->GetOwnerPane());
		if (componentPeerHandle != NULL)
			backgroundColor = ConvertAWTToMacColor(unhand(componentPeerHandle)->mBackColor);
		RGBBackColor(&backgroundColor);
		::EraseRect(&eraseRect);
		BackColor(whiteColor);
		ppMacGraphics->FinishDrawing();
	}

}

void sun_awt_macos_MacGraphics_drawRect(struct Hsun_awt_macos_MacGraphics *graphicsPeerObject,long x,long y,long width,long height)
{
	LMacGraphics		*ppMacGraphics = (LMacGraphics *)unhand(graphicsPeerObject)->pData;
	Rect				eraseRect;
	
	if (ppMacGraphics->BeginDrawing()) {
		ppMacGraphics->ConvertToPortRect(eraseRect, x, y, width + 1, height + 1);
		::FrameRect(&eraseRect);
		ppMacGraphics->FinishDrawing();
	}
}

void sun_awt_macos_MacGraphics_fillRect(struct Hsun_awt_macos_MacGraphics *graphicsPeerObject,long x,long y,long width,long height)
{
	LMacGraphics		*ppMacGraphics = (LMacGraphics *)unhand(graphicsPeerObject)->pData;
	Rect				eraseRect;
	
	if (ppMacGraphics->BeginDrawing()) {
		ppMacGraphics->ConvertToPortRect(eraseRect, x, y, width, height);
		::PaintRect(&eraseRect);
		ppMacGraphics->FinishDrawing();
	}
	
}

void sun_awt_macos_MacGraphics_drawRoundRect(struct Hsun_awt_macos_MacGraphics *graphicsPeerObject,long x,long y,long width,long height,long h,long v)
{
	LMacGraphics		*ppMacGraphics = (LMacGraphics *)unhand(graphicsPeerObject)->pData;
	Rect				eraseRect;
	
	if (ppMacGraphics->BeginDrawing()) {
		ppMacGraphics->ConvertToPortRect(eraseRect, x, y, width + 1, height + 1);
		::FrameRoundRect(&eraseRect, h, v);
		ppMacGraphics->FinishDrawing();
	}
}

void sun_awt_macos_MacGraphics_fillRoundRect(struct Hsun_awt_macos_MacGraphics *graphicsPeerObject, long x, long y, long width, long height, long h, long v)
{
	LMacGraphics		*ppMacGraphics = (LMacGraphics *)unhand(graphicsPeerObject)->pData;
	Rect				eraseRect;
	
	if (ppMacGraphics->BeginDrawing()) {
		ppMacGraphics->ConvertToPortRect(eraseRect, x, y, width, height);
		::PaintRoundRect(&eraseRect, h, v);
		ppMacGraphics->FinishDrawing();
	}
}

void sun_awt_macos_MacGraphics_drawOval(struct Hsun_awt_macos_MacGraphics *graphicsPeerObject, long x, long y, long width, long height)
{
	LMacGraphics		*ppMacGraphics = (LMacGraphics *)unhand(graphicsPeerObject)->pData;
	Rect				eraseRect;
	
	if (ppMacGraphics->BeginDrawing()) {
		ppMacGraphics->ConvertToPortRect(eraseRect, x, y, width + 1, height + 1);
		::FrameOval(&eraseRect);
		ppMacGraphics->FinishDrawing();
	}
}

void sun_awt_macos_MacGraphics_fillOval(struct Hsun_awt_macos_MacGraphics *graphicsPeerObject, long x, long y, long width, long height)
{
	LMacGraphics		*ppMacGraphics = (LMacGraphics *)unhand(graphicsPeerObject)->pData;
	Rect				eraseRect;
	
	if (ppMacGraphics->BeginDrawing()) {
		ppMacGraphics->ConvertToPortRect(eraseRect, x, y, width, height);
		::PaintOval(&eraseRect);
		ppMacGraphics->FinishDrawing();
	}
}

void sun_awt_macos_MacGraphics_drawArc(struct Hsun_awt_macos_MacGraphics *graphicsPeerObject, long x, long y, long width, long height, long start, long arcLength)
{

	LMacGraphics		*ppMacGraphics = (LMacGraphics *)unhand(graphicsPeerObject)->pData;
	Rect				eraseRect;
	
	if (ppMacGraphics->BeginDrawing()) {
		ppMacGraphics->ConvertToPortRect(eraseRect, x, y, width + 1, height + 1);
		::FrameArc(&eraseRect, 90 - start - arcLength, arcLength);
		ppMacGraphics->FinishDrawing();
	}
}

void sun_awt_macos_MacGraphics_fillArc(struct Hsun_awt_macos_MacGraphics *graphicsPeerObject, long x, long y, long width, long height, long start, long arcLength)
{
	LMacGraphics		*ppMacGraphics = (LMacGraphics *)unhand(graphicsPeerObject)->pData;
	Rect				eraseRect;
	
	if (ppMacGraphics->BeginDrawing()) {
		ppMacGraphics->ConvertToPortRect(eraseRect, x, y, width, height);
		::PaintArc(&eraseRect, 90 - start - arcLength, arcLength);
		ppMacGraphics->FinishDrawing();
	}
}

void sun_awt_macos_MacGraphics_drawPolygon(struct Hsun_awt_macos_MacGraphics *graphicsPeerObject, HArrayOfInt *xArray, HArrayOfInt *yArray, long count)
{
	LMacGraphics		*ppMacGraphics = (LMacGraphics *)unhand(graphicsPeerObject)->pData;
	PolyHandle			polygon;
	
	if (ppMacGraphics->BeginDrawing()) {
		polygon = ppMacGraphics->ConvertToPortPoly(xArray, yArray, count);
		::FramePoly(polygon);
		KillPoly(polygon);
		ppMacGraphics->FinishDrawing();
	}
}

void sun_awt_macos_MacGraphics_fillPolygon(struct Hsun_awt_macos_MacGraphics *graphicsPeerObject, HArrayOfInt *xArray, HArrayOfInt *yArray, long count)
{
	LMacGraphics		*ppMacGraphics = (LMacGraphics *)unhand(graphicsPeerObject)->pData;
	PolyHandle			polygon;
	
	if (ppMacGraphics->BeginDrawing()) {
		polygon = ppMacGraphics->ConvertToPortPoly(xArray, yArray, count);
		::PaintPoly(polygon);
		KillPoly(polygon);
		ppMacGraphics->FinishDrawing();
	}
}

//##############################################################################
//##############################################################################
#pragma mark -
#pragma mark TEXT DRAWING ROUTINES

void sun_awt_macos_MacGraphics_drawString(struct Hsun_awt_macos_MacGraphics *graphicsPeerObject,struct Hjava_lang_String *Hstring,long x,long y)
{
	sun_awt_macos_MacGraphics_drawStringWidth(graphicsPeerObject,Hstring, x, y);
}

void sun_awt_macos_MacGraphics_drawChars(struct Hsun_awt_macos_MacGraphics *graphicsPeerObject,HArrayOfChar *Hstring,long start,long length,long x,long y)
{
	sun_awt_macos_MacGraphics_drawCharsWidth(graphicsPeerObject, Hstring, start, length, x, y);
}

void sun_awt_macos_MacGraphics_drawBytes(struct Hsun_awt_macos_MacGraphics *graphicsPeerObject,HArrayOfByte *Hstring,long start,long length,long x,long y)
{
	sun_awt_macos_MacGraphics_drawBytesWidth(graphicsPeerObject, Hstring, start, length, x, y);
}

long sun_awt_macos_MacGraphics_drawStringWidth(struct Hsun_awt_macos_MacGraphics *graphicsPeerObject, struct Hjava_lang_String *stringObject,long x,long y)
{
	LMacGraphics		*ppMacGraphics = (LMacGraphics *)unhand(graphicsPeerObject)->pData;
	Point				textBasePoint;
#ifndef UNICODE_FONTLIST
	PenState			macPenState;
	char				*newCString;
#endif

	if (ppMacGraphics->BeginDrawing()) {
		ppMacGraphics->ConvertToPortPoint(textBasePoint, x, y);
#ifdef UNICODE_FONTLIST
		long	width;
		LMacCompStr str(stringObject);
		width = str.DrawWidth(ppMacGraphics->mUnicodeFontList, textBasePoint.h, textBasePoint.v);

		ppMacGraphics->FinishDrawing();
		
		return width;
#else
		::MoveTo(textBasePoint.h, textBasePoint.v);
		
		newCString = allocCString(stringObject);
		::DrawText((unsigned char *)newCString, 0, javaStringLength(stringObject));
		free(newCString);
		
		ppMacGraphics->FinishDrawing();
	
		GetPenState(&macPenState);
		return macPenState.pnLoc.h - textBasePoint.h;
#endif
	}
	
	else
		return 0;
}

long sun_awt_macos_MacGraphics_drawCharsWidth(struct Hsun_awt_macos_MacGraphics *graphicsPeerObject, HArrayOfChar *stringObject,long start,long length,long x,long y)
{
	LMacGraphics		*ppMacGraphics = (LMacGraphics *)unhand(graphicsPeerObject)->pData;
	SInt16				*currentBodyShort = (SInt16 *)(unhand(stringObject)->body) + start;
	Point				textBasePoint;
#ifndef UNICODE_FONTLIST
	PenState			macPenState;
	long				lengthCopy = length;
	char				*newCString, *currentBodyChar;
#endif

	if (ppMacGraphics->BeginDrawing()) {
		ppMacGraphics->ConvertToPortPoint(textBasePoint, x, y);

#ifdef UNICODE_FONTLIST
		long	width;
		LMacCompStr str(stringObject, start , length);
		width = str.DrawWidth(ppMacGraphics->mUnicodeFontList, textBasePoint.h, textBasePoint.v);

		ppMacGraphics->FinishDrawing();
		
		return width;
#else
		::MoveTo(textBasePoint.h, textBasePoint.v);
	
		newCString = (char *)malloc(length);
		if (newCString != NULL) {
			
			currentBodyChar = newCString;
			
			while (lengthCopy--)
				*currentBodyChar++ = *currentBodyShort++;
			
			::DrawText((unsigned char *)newCString, 0, length);
			
			free(newCString);
		}
		
		ppMacGraphics->FinishDrawing();
	
		::GetPenState(&macPenState);
		return macPenState.pnLoc.h - textBasePoint.h;
#endif
	}
	
	else 
		return 0;
}

long sun_awt_macos_MacGraphics_drawBytesWidth(struct Hsun_awt_macos_MacGraphics *graphicsPeerObject, HArrayOfByte *stringObject,long start,long length,long x,long y)
{
	LMacGraphics		*ppMacGraphics = (LMacGraphics *)unhand(graphicsPeerObject)->pData;
	Point				textBasePoint;
	PenState			macPenState;

	if (ppMacGraphics->BeginDrawing()) {
		ppMacGraphics->ConvertToPortPoint(textBasePoint, x, y);
		::MoveTo(textBasePoint.h, textBasePoint.v);
	
	#if 0
		// FIX ME
		newCString = allocCString(stringObject);
		::DrawText((unsigned char *)newCString, start, length);
		free(newCString);
	#endif
		
		ppMacGraphics->FinishDrawing();
	
		::GetPenState(&macPenState);
		return macPenState.pnLoc.h - textBasePoint.h;
	}
	
	else 
		return 0;
	
}

