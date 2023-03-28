// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

#pragma once

#include <QDOffscreen.h>


//***************************************************************************************************

class	COffScreen
{
public:
//-----
	COffScreen() {}
	
	void IOffScreen();
	
	void Free();
	
	Boolean BeginOffScreen(const Rect& onScrRect);

	void EndOffScreen(); //should be called only if BeginOffScreen returns true
	
	void PurgeOffScreen(); //mark the off screen as unlocked and purgeable
	
private:
//------
	CGrafPtr 	fOnScreenPort;
	GDHandle 	fOnScreenDev;
	GWorldPtr fOffScrWorld;
	PixMapHandle fOffScrPixMap;
	Boolean fOffScreenLocked;
	
	
	RGBColor fOnScreenForeRGB;
	RGBColor fOnScreenBackRGB;

	Rect			fOnScrRect;

	short			fOffScrHeight;
	short			fOffScrWidth;
	
	short fSumWidth;
	short fSumHeight;
	short fAverageCount;
	
	#ifdef txtnDebug
	char fInUse;
	#endif
	
	Boolean NewOffScrWorld(short width, short height);
	Boolean UpdateOffScrWorld(short width, short height);
	
	void ResetAverage();
};
//**************************************************************************************************

extern COffScreen gOffScreen; 

//**************************************************************************************************
