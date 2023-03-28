// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

#include "OffScreen.h"


//***************************************************************************************************

const short kAverageCount = 30;


static Boolean gHasOffScreen;

static RGBColor gRGBBlack;
static RGBColor gRGBWhite;


//***************************************************************************************************


void COffScreen::IOffScreen()
{
	gHasOffScreen = false;
	
	//GWorld routines are present if has quickdraw 32 bits or if system 7 is present (B&W, 1bit GWorld)
	long qdVersion;
	OSErr err = Gestalt(gestaltQuickdrawVersion, &qdVersion);
	
	if (!err) {
		if (qdVersion >= gestalt32BitQD)
			{gHasOffScreen = true;}
		else {
			long systemVersion;
			err = Gestalt(gestaltSystemVersion, &systemVersion);
			
			if ((!err) && ((short)systemVersion >= 0x0700))
				{gHasOffScreen = true;}
		}
	}
	
	fOffScrWorld = nil;
	fOnScreenPort = nil;

	fOffScreenLocked = false;

	fOffScrWidth = 0;
	fOffScrHeight = 0;

	fSumWidth = 0;
	fSumHeight = 0;
	fAverageCount = 0;
	
	#ifdef txtnDebug
	fInUse = 0;
	#endif
	
	gRGBBlack.red = 0; gRGBBlack.green = 0; gRGBBlack.blue = 0;
	gRGBWhite.red = 0xFFFF; gRGBWhite.green = 0xFFFF; gRGBWhite.blue = 0xFFFF;//white
}
//***************************************************************************************************

void COffScreen::Free()
{
	if (fOffScrWorld)
		{DisposeGWorld(fOffScrWorld);}
}
//***************************************************************************************************

Boolean COffScreen::NewOffScrWorld(short width, short height)
{
	if (fOffScrWorld != nil) {
		DisposeGWorld(fOffScrWorld);
		
		fOffScrWorld = nil;
		fOffScrWidth = 0;
		fOffScrHeight = 0;
	}
	
	Rect globRect;
	globRect.top = 0;
	globRect.left = 0;
	globRect.right = width;
	globRect.bottom = height;
	
	GWorldPtr tempGWorld;
	if (NewGWorld(&tempGWorld, 0/*res*/, &globRect, nil/*cTable*/, 0/*device*/, useTempMem/*flags*/) != noErr) {
		return false;
	}
	
	fOffScrWorld = tempGWorld;
	
	fOffScrPixMap = GetGWorldPixMap(tempGWorld);
	//note that we can't dereference tempGWorld->portPixMap for the B&W version of GWorld (system7 on B&W machines)
	
	fOffScrWidth = width;
	fOffScrHeight = height;
	
	LockPixels(fOffScrPixMap);
	fOffScreenLocked = true;
	
	return true;
}
//***************************************************************************************************

void COffScreen::ResetAverage()
{
	fSumWidth = 0;
	fSumHeight = 0;
	fAverageCount = 0;
}
//***************************************************************************************************

Boolean COffScreen::UpdateOffScrWorld(short width, short height)
/*
UpdateGWorld may set the "baseAddr" field (handle) of the pixMap of GWorld to nil (for instance
if the passed "baseAddr" is purged and it can't allocate the req storage, calling "LockPixels"
with such pixMap will cause a big problem since "LockPixels" doesn't check for nil handles.
Actually in this case I don't know how can we check for "purged" buffer (since GetPixBaseAddr
doesn't check for Nil Handles too!!), actually calling UpdateOffScrWorld with purged buffer is not
allowed!!.
Another problem with UpdateGWorld in systems 6, when called to increase the rectangle, the draw
off screen after this is not correct (I didn't go furthermore in the debugging).
So I'll not use UpdateGWorld

(try to change in UDialogs to make it create a BW port and try to apply colors??)
*/
{
	Boolean createNew = false;
	
	if ((width > fOffScrWidth) || (height > fOffScrHeight)) {
		//zero average stuff, this big size should not affect the stable values
		this->ResetAverage();

		createNew = true;
	}
	else {
		//(width <= fOffScrWidth) and (height <= fOffScrHeight)
		
		fSumWidth += width;
		fSumHeight += height;
		++fAverageCount;

		if (fAverageCount >= kAverageCount) {
			short average = fSumWidth / fAverageCount;
			if ((fOffScrWidth > average) && (average >= width)) {
				createNew = true;
				width = average;
			}
			
			average = fSumHeight / fAverageCount;
			if ((fOffScrHeight > average) && (average >= height)) {
				createNew = true;
				height = average;
			}
			
			this->ResetAverage();
		}
	}
	
	if (createNew)
		{return this->NewOffScrWorld(width, height);}
	
	if (!fOffScreenLocked) {
		if (GetPixBaseAddr(fOffScrPixMap) == nil) {
			return this->NewOffScrWorld(width, height);
		}
		else {
			NoPurgePixels(fOffScrPixMap);
			
			LockPixels(fOffScrPixMap);
			
			fOffScreenLocked = true;
		}
	}

	return true;
}
//***************************************************************************************************

Boolean COffScreen::BeginOffScreen(const Rect& onScrRect)
{
	if (!gHasOffScreen)
		{return false;}
	
//	Assert_(!fInUse);
	
	if (!this->UpdateOffScrWorld(onScrRect.right - onScrRect.left, onScrRect.bottom - onScrRect.top))
		{return false;}

	Assert_(fOffScreenLocked);
	
	GetGWorld(&fOnScreenPort, &fOnScreenDev);
	
	GetForeColor(&fOnScreenForeRGB);
	GetBackColor(&fOnScreenBackRGB);
	
	RGBForeColor(&gRGBBlack);
	RGBBackColor(&gRGBWhite);
	
	SetGWorld(fOffScrWorld, nil);
	
	RGBForeColor(&fOnScreenForeRGB);	//	jah 950323
	RGBBackColor(&fOnScreenBackRGB);	//	jah 950323
	
	fOnScrRect = onScrRect;
	SetOrigin(onScrRect.left, onScrRect.top);
	
	#ifdef txtnDebug
	fInUse = 1;
	#endif
	
	return true;
}
//***************************************************************************************************

void COffScreen::EndOffScreen()
{
	#ifdef txtnDebug
	fInUse = 0;
	#endif
	
	Assert_(fOffScreenLocked);

	SetGWorld(fOnScreenPort, fOnScreenDev);
	
	Rect stackRect = fOnScrRect;//CopyBits may move memory!

	CopyBits(BitMapPtr(&GrafPtr(fOffScrWorld)->portBits), BitMapPtr(&GrafPtr(fOnScreenPort)->portBits)
						, &stackRect, &stackRect, srcCopy, nil);

/* commented because it does not work correctly on B&W machines with system 7!
	CopyBits(BitMapPtr(*fOffScrPixMap), BitMapPtr(&GrafPtr(fOnScreenPort)->portBits)
						, &stackRect, &stackRect, srcCopy, nil);
*/

	RGBForeColor(&fOnScreenForeRGB);
	RGBBackColor(&fOnScreenBackRGB);
	
	this->PurgeOffScreen();
}
//***************************************************************************************************


void COffScreen::PurgeOffScreen()
{
	if (fOffScrWorld) {
		fOffScreenLocked = false;
		UnlockPixels(fOffScrPixMap);
		AllowPurgePixels(fOffScrPixMap);
	}
}
//***************************************************************************************************
