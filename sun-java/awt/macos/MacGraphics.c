/*
 * %W% %E%
 *
 * Copyright (c) 1995 Sun Microsystems, Inc. All Rights Reserved.
 *
 * Permission to use, copy, modify, and distribute this software
 * and its documentation for NON-COMMERCIAL purposes and without
 * fee is hereby granted provided that this copyright notice
 * appears in all copies. Please refer to the file "copyright.html"
 * for further important copyright and licensing information.
 *
 * SUN MAKES NO REPRESENTATIONS OR WARRANTIES ABOUT THE SUITABILITY OF
 * THE SOFTWARE, EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED
 * TO THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
 * PARTICULAR PURPOSE, OR NON-INFRINGEMENT. SUN SHALL NOT BE LIABLE FOR
 * ANY DAMAGES SUFFERED BY LICENSEE AS A RESULT OF USING, MODIFYING OR
 * DISTRIBUTING THIS SOFTWARE OR ITS DERIVATIVES.
 */

/*
 * Graphics.c
 */
#include "exceptions.h"
#include <stddef.h>
#include <StubPreamble.h>
#include <CodeFragments.h>
#include "MacWindowObj.h"
#include "javaString.h"
#include "java_awt_Rectangle.h"
#include "java_awt_Color.h"
#include "Containment.h"
#include "interrupt_md.h"

/* Implemented */
#include "sun_awt_macos_MComponentPeer.h"
#include "java_awt_component.h"
#include "sun_awt_macos_MFramePeer.h"
#include "java_awt_frame.h"
#include "sun_awt_macos_MButtonPeer.h"
#include "java_awt_button.h"
#include "sun_awt_macos_MacFontMetrics.h"
#include "java_awt_Font.h"
#include "sun_awt_macos_MToolkit.h"
#include "sun_awt_macos_MCanvasPeer.h"
#include "java_awt_Canvas.h"
#include "sun_awt_macos_MacGraphics.h"
#include "sun_awt_macos_MPanelPeer.h"

/* Stubbed out */
#include "sun_awt_macos_MCheckboxMenuItemPeer.h"
#include "sun_awt_macos_MCheckboxPeer.h"
#include "sun_awt_macos_MChoicePeer.h"
#include "sun_awt_macos_MLabelPeer.h"
#include "sun_awt_macos_MListPeer.h"
#include "sun_awt_macos_MScrollbarPeer.h"
#include "sun_awt_macos_MTextAreaPeer.h"
#include "sun_awt_macos_MTextFieldPeer.h"

#include "sun_awt_macos_MMenuBarPeer.h"
#include "sun_awt_macos_MMenuItemPeer.h"
#include "sun_awt_macos_MMenuPeer.h"

#include "sun_awt_macos_MFileDialogPeer.h"
#include "sun_awt_macos_MDialogPeer.h"
#include "sun_awt_macos_MWindowPeer.h"
#include "sun_awt_macos_MacImage.h"

#include "MacEventClient.h"

#include "MacAwt.h"

/*
 * Graphics
 */

static void ConvertAwtToMacColor(Classjava_awt_Color *awtColor, RGBColor *macColor)
{
short red, green, blue;

	red		= (awtColor->value >> 16) & 0xff;
	green	= (awtColor->value >>  8) & 0xff;
	blue	= (awtColor->value >>  0) & 0xff;
	macColor->red	= (red   << 8) | red;
	macColor->green	= (green << 8) | green;
	macColor->blue	= (blue  << 8) | blue;
}


void sun_awt_macos_MacGraphics_createFromComponent(struct Hsun_awt_macos_MacGraphics *Hgraphicspeer,struct Hsun_awt_macos_MComponentPeer *Hcomppeer)
{
Classsun_awt_macos_MacGraphics		*graphicspeer		= (Classsun_awt_macos_MacGraphics*) unhand(Hgraphicspeer);
Classsun_awt_macos_MComponentPeer	*parentpeer			= (Classsun_awt_macos_MComponentPeer*) unhand(Hcomppeer);
macAwtContainerPtr					parentmaccontainer	= (macAwtContainerPtr) PDATA(parentpeer);
ContainerPtr						myContainer;
macAwtGraphicsPtr					myGraphics;

	myContainer = &parentmaccontainer->macContainer;
	myGraphics = (macAwtGraphicsPtr) NewPtr(sizeof(macAwtGraphics));
	if (!myGraphics)
	{
		DebugStr("\p sun_awt_macos_MacGraphics_createFromComponent NULL graphics peer");
		return;
	}

//jfm bag this stuff, lets try for no new ports in favor of a window local clipping region, and an offset point
	myGraphics->Hcomppeer		= Hcomppeer;
	myGraphics->originx			= 0;
	myGraphics->originy			= 0;
	myGraphics->scalex			= 1.0;
	myGraphics->scaley			= 1.0;
	myGraphics->clipRect.top	= -32767;
	myGraphics->clipRect.left	= -32767;
	myGraphics->clipRect.bottom	=  32767;
	myGraphics->clipRect.right	=  32767;
	SET_PDATA(graphicspeer, myGraphics);
}

void sun_awt_macos_MacGraphics_dispose(struct Hsun_awt_macos_MacGraphics *Hgraphicspeer)
{
Classsun_awt_macos_MacGraphics		*graphicspeer	= (Classsun_awt_macos_MacGraphics*) unhand(Hgraphicspeer);
macAwtGraphicsPtr					macGraphics;

	macGraphics		= (macAwtGraphicsPtr) PDATA(graphicspeer);
	if (macGraphics)
	{
		DisposePtr((Ptr) macGraphics);
		SET_PDATA(graphicspeer, 0);
		SetPort((GrafPtr) &theAWTWorkPort);	// Unneeded?
	}
}

void sun_awt_macos_MacGraphics_createFromGraphics(struct Hsun_awt_macos_MacGraphics *,struct Hsun_awt_macos_MacGraphics *)
{
/* Clone operation ? */
}

void sun_awt_macos_MacGraphics_imageCreate(struct Hsun_awt_macos_MacGraphics *,struct Hsun_awt_image_ImageRepresentation *)
{
}

void sun_awt_macos_MacGraphics_pSetFont(struct Hsun_awt_macos_MacGraphics *Hgraphicspeer,struct Hjava_awt_Font *Hfont)
{
Classsun_awt_macos_MacGraphics		*graphicspeer		= unhand(Hgraphicspeer);	// Get a pointer to the graphics peer
macAwtGraphicsPtr					macGraphics 		= (macAwtGraphicsPtr) PDATA(graphicspeer);					// Get a pointer to the graphics data
Classsun_awt_macos_MComponentPeer	*macComponentPeer	= unhand(macGraphics->Hcomppeer);							// Get a pointer to the component peer
macAwtContainerPtr					macContainerPtr		= (macAwtContainerPtr) PDATA(macComponentPeer);				// Get a pointer to the component data
Classjava_awt_Font					*fontmetrics		= unhand(Hfont);
char								fontName[255];


	SetPort((GrafPtr) macContainerPtr->dstPort);
	javaString2CString(fontmetrics->family, fontName, sizeof(fontName) - 1);
	CtoPstr(fontName);
	GetFNum((StringPtr) fontName, &macGraphics->myFont);
}

void sun_awt_macos_MacGraphics_pSetForeground(struct Hsun_awt_macos_MacGraphics *Hgraphicspeer,struct Hjava_awt_Color *Hcolor)
{
Classjava_awt_Color					*color				= (Classjava_awt_Color*) unhand(Hcolor);					// Get pointer to color
Classsun_awt_macos_MacGraphics		*graphicspeer		= (Classsun_awt_macos_MacGraphics*) unhand(Hgraphicspeer);	// Get pointer to graphics peer
macAwtGraphicsPtr					macGraphics			= (macAwtGraphicsPtr) PDATA(graphicspeer);					// Get pointer to graphics data

	macGraphics->foreground = *color;
}

void sun_awt_macos_MacGraphics_pSetScaling(struct Hsun_awt_macos_MacGraphics *Hgraphicspeer,float x,float y)
{
Classsun_awt_macos_MacGraphics		*graphicspeer	= (Classsun_awt_macos_MacGraphics*) unhand(Hgraphicspeer);
macAwtGraphicsPtr					macGraphics;

	macGraphics		= (macAwtGraphicsPtr) PDATA(graphicspeer);

	macGraphics->scalex = x;
	macGraphics->scaley = y;
}

void sun_awt_macos_MacGraphics_pSetOrigin(struct Hsun_awt_macos_MacGraphics *Hgraphicspeer,long x,long y)
{
Classsun_awt_macos_MacGraphics		*graphicspeer	= (Classsun_awt_macos_MacGraphics*) unhand(Hgraphicspeer);
macAwtGraphicsPtr					macGraphics;

	macGraphics		= (macAwtGraphicsPtr) PDATA(graphicspeer);

	macGraphics->originx = x;
	macGraphics->originy = y;
}

void sun_awt_macos_MacGraphics_setPaintMode(struct Hsun_awt_macos_MacGraphics *Hgraphicspeer)
{
Classsun_awt_macos_MacGraphics		*graphicspeer	= (Classsun_awt_macos_MacGraphics*) unhand(Hgraphicspeer);
macAwtGraphicsPtr					macGraphics		= (macAwtGraphicsPtr) PDATA(graphicspeer);

	macGraphics->paintingmode = kPaintMode;
}

void sun_awt_macos_MacGraphics_setXORMode(struct Hsun_awt_macos_MacGraphics *Hgraphicspeer,struct Hjava_awt_Color *)
{
Classsun_awt_macos_MacGraphics		*graphicspeer	= (Classsun_awt_macos_MacGraphics*) unhand(Hgraphicspeer);
macAwtGraphicsPtr					macGraphics		= (macAwtGraphicsPtr) PDATA(graphicspeer);

	macGraphics->paintingmode = kXORMode;
}

void sun_awt_macos_MacGraphics_getClipRect(struct Hsun_awt_macos_MacGraphics *Hgraphicspeer,struct Hjava_awt_Rectangle *Hrect)
{
Classjava_awt_Rectangle				*javarect		= (Classjava_awt_Rectangle*) unhand(Hrect);
Classsun_awt_macos_MacGraphics		*graphicspeer	= (Classsun_awt_macos_MacGraphics*) unhand(Hgraphicspeer);
macAwtGraphicsPtr					macGraphics;

	macGraphics		= (macAwtGraphicsPtr) PDATA(graphicspeer);

	javarect->x			= macGraphics->clipRect.left;
	javarect->y			= macGraphics->clipRect.top;
	javarect->width		= macGraphics->clipRect.right - macGraphics->clipRect.left;
	javarect->height	= macGraphics->clipRect.bottom - macGraphics->clipRect.top;
}

void sun_awt_macos_MacGraphics_clipRect(struct Hsun_awt_macos_MacGraphics *Hgraphicspeer,long top,long left,long width,long height)
{
Classsun_awt_macos_MacGraphics		*graphicspeer	= (Classsun_awt_macos_MacGraphics*) unhand(Hgraphicspeer);
macAwtGraphicsPtr					macGraphics		= (macAwtGraphicsPtr) PDATA(graphicspeer);
Rect								bounds;

	bounds.top		= macGraphics->scaley * top;
	bounds.left		= macGraphics->scalex * left;
	bounds.bottom	= bounds.top + macGraphics->scaley * height;
	bounds.right	= bounds.left + macGraphics->scalex * width;
	macGraphics->clipRect = bounds;
}

void sun_awt_macos_MacGraphics_clearRect(struct Hsun_awt_macos_MacGraphics *Hgraphicspeer,long top,long left,long width,long height)
{
Classsun_awt_macos_MacGraphics		*graphicspeer		= unhand(Hgraphicspeer);		// Get pointer to graphics peer
macAwtGraphicsPtr					macGraphics			= (macAwtGraphicsPtr) PDATA(graphicspeer);						// Get pointer to graphics data
Classsun_awt_macos_MComponentPeer	*macComponentPeer	= unhand(macGraphics->Hcomppeer);								// Get pointer to the component peer
macAwtContainerPtr					macContainerPtr 	= (macAwtContainerPtr) PDATA(macComponentPeer);					// Get pointer to the component data
Rect								bounds;
RgnHandle							swapRgn;

	SetPort((GrafPtr) macContainerPtr->dstPort);

	swapRgn = macContainerPtr->dstPort->clipRgn;					// Save the destination cliprgn
	macContainerPtr->dstPort->clipRgn = macContainerPtr->cliprgn;	// install our own cliprgn

	bounds.top		= macContainerPtr->originy + macGraphics->originy + top  * macGraphics->scaley;
	bounds.left		= macContainerPtr->originx + macGraphics->originx + left * macGraphics->scalex;
	bounds.bottom	= bounds.top + macGraphics->scaley * height;
	bounds.right	= bounds.left + macGraphics->scalex * width;

	EraseRect(&bounds);

	macContainerPtr->dstPort->clipRgn = swapRgn;					// swap the original cliprgn back
}

void sun_awt_macos_MacGraphics_fillRect(struct Hsun_awt_macos_MacGraphics *Hgraphicspeer,long top,long left,long width,long height)
{
Classsun_awt_macos_MacGraphics		*graphicspeer		= (Classsun_awt_macos_MacGraphics*) unhand(Hgraphicspeer);		// Get pointer to graphics peer
macAwtGraphicsPtr					macGraphics			= (macAwtGraphicsPtr) PDATA(graphicspeer);						// Get pointer to graphics data
Classsun_awt_macos_MComponentPeer	*macComponentPeer	= unhand(macGraphics->Hcomppeer);								// Get pointer to the component peer
macAwtContainerPtr					macContainerPtr 	= (macAwtContainerPtr) PDATA(macComponentPeer);					// Get pointer to the component data
Rect								bounds;
RgnHandle							swapRgn;
RGBColor							macColor;

	SetPort((GrafPtr) macContainerPtr->dstPort);

	swapRgn = macContainerPtr->dstPort->clipRgn;					// Save the destination cliprgn
	macContainerPtr->dstPort->clipRgn = macContainerPtr->cliprgn;	// install our own cliprgn

	ConvertAwtToMacColor(&macGraphics->foreground, &macColor);
	RGBForeColor(&macColor);										//Setup the foreground color

	bounds.top		= macContainerPtr->originy + macGraphics->originy + top  * macGraphics->scaley;
	bounds.left		= macContainerPtr->originx + macGraphics->originx + left * macGraphics->scalex;
	bounds.bottom	= bounds.top + macGraphics->scaley * height;
	bounds.right	= bounds.left + macGraphics->scalex * width;

	PaintRect(&bounds);

	macContainerPtr->dstPort->clipRgn = swapRgn;					// swap the original cliprgn back
}

void sun_awt_macos_MacGraphics_drawRect(struct Hsun_awt_macos_MacGraphics *Hgraphicspeer,long top,long left,long width,long height)
{
Classsun_awt_macos_MacGraphics		*graphicspeer		= (Classsun_awt_macos_MacGraphics*) unhand(Hgraphicspeer);		// Get pointer to graphics peer
macAwtGraphicsPtr					macGraphics			= (macAwtGraphicsPtr) PDATA(graphicspeer);						// Get pointer to graphics data
Classsun_awt_macos_MComponentPeer	*macComponentPeer	= unhand(macGraphics->Hcomppeer);								// Get pointer to the component peer
macAwtContainerPtr					macContainerPtr		= (macAwtContainerPtr) PDATA(macComponentPeer);					// Get pointer to the component data
Rect								bounds;
RgnHandle							swapRgn;
RGBColor							macColor;

	SetPort((GrafPtr) macContainerPtr->dstPort);					// Set the port to the destination port
	swapRgn = macContainerPtr->dstPort->clipRgn;					// Save the destination cliprgn
	macContainerPtr->dstPort->clipRgn = macContainerPtr->cliprgn;	// install our own cliprgn

	ConvertAwtToMacColor(&macGraphics->foreground, &macColor);
	RGBForeColor(&macColor);										//Setup the foreground color

	bounds.top		= macContainerPtr->originy + macGraphics->originy + top  * macGraphics->scaley;
	bounds.left		= macContainerPtr->originx + macGraphics->originx + left * macGraphics->scalex;
	bounds.bottom	= bounds.top + macGraphics->scaley * height;
	bounds.right	= bounds.left + macGraphics->scalex * width;

	FrameRect(&bounds);

	macContainerPtr->dstPort->clipRgn = swapRgn;					// swap the original cliprgn back
}

void sun_awt_macos_MacGraphics_drawString(struct Hsun_awt_macos_MacGraphics *Hgraphicspeer,struct Hjava_lang_String *Hstring,long x,long y)
{
	sun_awt_macos_MacGraphics_drawStringWidth(Hgraphicspeer,Hstring, x, y);
}

void sun_awt_macos_MacGraphics_drawChars(struct Hsun_awt_macos_MacGraphics *Hgraphicspeer,HArrayOfChar *Hstring,long x,long y,long start,long length)
{
	sun_awt_macos_MacGraphics_drawCharsWidth(Hgraphicspeer,Hstring,x,y,start,length);
}

void sun_awt_macos_MacGraphics_drawBytes(struct Hsun_awt_macos_MacGraphics *Hgraphicspeer,HArrayOfByte *Hstring,long x,long y,long start,long length)
{
	sun_awt_macos_MacGraphics_drawBytesWidth(Hgraphicspeer, Hstring, x, y, start, length);
}

long sun_awt_macos_MacGraphics_drawStringWidth(struct Hsun_awt_macos_MacGraphics *Hgraphicspeer,struct Hjava_lang_String *Hstring,long x,long y)
{
Classsun_awt_macos_MacGraphics		*graphicspeer		= (Classsun_awt_macos_MacGraphics*) unhand(Hgraphicspeer);		// Get pointer to graphics peer
macAwtGraphicsPtr					macGraphics			= (macAwtGraphicsPtr) PDATA(graphicspeer);						// Get pointer to graphics data
Classsun_awt_macos_MComponentPeer	*macComponentPeer	= unhand(macGraphics->Hcomppeer);								// Get pointer to the component peer
macAwtContainerPtr					macContainerPtr		= (macAwtContainerPtr) PDATA(macComponentPeer);					// Get pointer to the component data
char								*string				= allocCString(Hstring);
RgnHandle							swapRgn;
RGBColor							macColor;
PenState							macPenState;

	SetPort((GrafPtr) macContainerPtr->dstPort);					// Set the port to the destination port

	swapRgn = macContainerPtr->dstPort->clipRgn;					// Save the destination cliprgn
	macContainerPtr->dstPort->clipRgn = macContainerPtr->cliprgn;	// install our own cliprgn

	ConvertAwtToMacColor(&macGraphics->foreground, &macColor);
	RGBForeColor(&macColor);										//Setup the foreground color

	TextFont(macGraphics->myFont);									// Set the font to the graphics font
	MoveTo(macContainerPtr->originx + macGraphics->originx + macGraphics->scalex * x, macContainerPtr->originy + macGraphics->originy + macGraphics->scaley * y);
	DrawText(string, 0, javaStringLength(Hstring));					// Draw the text
	free(string);													// Free the malloc'd string
	GetPenState(&macPenState);

	macContainerPtr->dstPort->clipRgn = swapRgn;					// swap the original cliprgn back
	return macPenState.pnLoc.h - (macContainerPtr->originx + macGraphics->originx + macGraphics->scalex * x);
}

long sun_awt_macos_MacGraphics_drawCharsWidth(struct Hsun_awt_macos_MacGraphics *Hgraphicspeer,HArrayOfChar *Hstring,long x,long y,long start,long length)
{
char								*string				= (char*) unhand(Hstring)->body;
Classsun_awt_macos_MacGraphics		*graphicspeer		= (Classsun_awt_macos_MacGraphics*) unhand(Hgraphicspeer);		// Get pointer to graphics peer
macAwtGraphicsPtr					macGraphics			= (macAwtGraphicsPtr) PDATA(graphicspeer);						// Get pointer to graphics data
Classsun_awt_macos_MComponentPeer	*macComponentPeer	= unhand(macGraphics->Hcomppeer);								// Get pointer to the component peer
macAwtContainerPtr					macContainerPtr		= (macAwtContainerPtr) PDATA(macComponentPeer);					// Get pointer to the component data
RgnHandle							swapRgn;
RGBColor							macColor;
PenState							macPenState;

	SetPort((GrafPtr) macContainerPtr->dstPort);					// Set the port to the destination port

	swapRgn = macContainerPtr->dstPort->clipRgn;					// Save the destination cliprgn
	macContainerPtr->dstPort->clipRgn = macContainerPtr->cliprgn;	// install our own cliprgn

	ConvertAwtToMacColor(&macGraphics->foreground, &macColor);
	RGBForeColor(&macColor);										//Setup the foreground color

	TextFont(macGraphics->myFont);									// Set the font to the graphics font
	MoveTo(macContainerPtr->originx + macGraphics->originx + macGraphics->scalex * x, macContainerPtr->originy + macGraphics->originy + macGraphics->scaley * y);
	DrawText(string, start, length);
	GetPenState(&macPenState);

	macContainerPtr->dstPort->clipRgn = swapRgn;					// swap the original cliprgn back
	return macPenState.pnLoc.h - (macContainerPtr->originx + macGraphics->originx + macGraphics->scalex * x);
}

long sun_awt_macos_MacGraphics_drawBytesWidth(struct Hsun_awt_macos_MacGraphics *Hgraphicspeer,HArrayOfByte *Hstring,long x,long y,long start,long length)
{
char								*string				= (char*) unhand(Hstring)->body;
Classsun_awt_macos_MacGraphics		*graphicspeer		= (Classsun_awt_macos_MacGraphics*) unhand(Hgraphicspeer);		// Get pointer to graphics peer
macAwtGraphicsPtr					macGraphics			= (macAwtGraphicsPtr) PDATA(graphicspeer);						// Get pointer to graphics data
Classsun_awt_macos_MComponentPeer	*macComponentPeer	= unhand(macGraphics->Hcomppeer);								// Get pointer to the component peer
macAwtContainerPtr					macContainerPtr		= (macAwtContainerPtr) PDATA(macComponentPeer);					// Get pointer to the component data
RgnHandle							swapRgn;
RGBColor							macColor;
PenState							macPenState;

	SetPort((GrafPtr) macContainerPtr->dstPort);					// Set the port to the destination port

	swapRgn = macContainerPtr->dstPort->clipRgn;					// Save the destination cliprgn
	macContainerPtr->dstPort->clipRgn = macContainerPtr->cliprgn;	// install our own cliprgn

	ConvertAwtToMacColor(&macGraphics->foreground, &macColor);
	RGBForeColor(&macColor);										//Setup the foreground color

	TextFont(macGraphics->myFont);									// Set the font to the graphics font
	MoveTo(macContainerPtr->originx + macGraphics->originx + macGraphics->scalex * x, macContainerPtr->originy + macGraphics->originy + macGraphics->scaley * y);
	DrawText(string, start, length);
	GetPenState(&macPenState);

	macContainerPtr->dstPort->clipRgn = swapRgn;					// swap the original cliprgn back
	return macPenState.pnLoc.h - (macContainerPtr->originx + macGraphics->originx + macGraphics->scalex * x);
}

void sun_awt_macos_MacGraphics_drawLine(struct Hsun_awt_macos_MacGraphics *Hgraphicspeer,long x1,long y1,long x2,long y2)
{
Classsun_awt_macos_MacGraphics		*graphicspeer		= (Classsun_awt_macos_MacGraphics*) unhand(Hgraphicspeer);		// Get pointer to graphics peer
macAwtGraphicsPtr					macGraphics			= (macAwtGraphicsPtr) PDATA(graphicspeer);						// Get pointer to graphics data
Classsun_awt_macos_MComponentPeer	*macComponentPeer	= unhand(macGraphics->Hcomppeer);								// Get pointer to the component peer
macAwtContainerPtr					macContainerPtr		= (macAwtContainerPtr) PDATA(macComponentPeer);					// Get pointer to the component data
RgnHandle							swapRgn;
RGBColor							macColor;

	SetPort((GrafPtr) macContainerPtr->dstPort);					// Set the port to the destination port

	swapRgn = macContainerPtr->dstPort->clipRgn;					// Save the destination cliprgn
	macContainerPtr->dstPort->clipRgn = macContainerPtr->cliprgn;	// install our own cliprgn

	ConvertAwtToMacColor(&macGraphics->foreground, &macColor);
	RGBForeColor(&macColor);										//Setup the foreground color

	MoveTo(macContainerPtr->originx + macGraphics->originx + macGraphics->scalex * x1, macContainerPtr->originy + macGraphics->originy + macGraphics->scaley * y1);
	LineTo(macContainerPtr->originx + macGraphics->originx + macGraphics->scalex * x2, macContainerPtr->originy + macGraphics->originy + macGraphics->scaley * y2);

	macContainerPtr->dstPort->clipRgn = swapRgn;					// swap the original cliprgn back
}

void sun_awt_macos_MacGraphics_copyArea(struct Hsun_awt_macos_MacGraphics *Hgraphicspeer,long top,long left,long width,long height,long x2,long y2)
{
Classsun_awt_macos_MacGraphics		*graphicspeer		= (Classsun_awt_macos_MacGraphics*) unhand(Hgraphicspeer);		// Get pointer to graphics peer
macAwtGraphicsPtr					macGraphics			= (macAwtGraphicsPtr) PDATA(graphicspeer);						// Get pointer to graphics data
Classsun_awt_macos_MComponentPeer	*macComponentPeer	= unhand(macGraphics->Hcomppeer);								// Get pointer to the component peer
macAwtContainerPtr					macContainerPtr		= (macAwtContainerPtr) PDATA(macComponentPeer);					// Get pointer to the component data
Rect								bounds;
Rect								dstbounds;
RgnHandle							swapRgn;
RGBColor							macColor;

	SetPort((GrafPtr) macContainerPtr->dstPort);

	swapRgn = macContainerPtr->dstPort->clipRgn;					// Save the destination cliprgn
	macContainerPtr->dstPort->clipRgn = macContainerPtr->cliprgn;	// install our own cliprgn

	bounds.top			= macContainerPtr->originy + macGraphics->originy + macGraphics->scaley * top;
	bounds.left			= macContainerPtr->originx + macGraphics->originx + macGraphics->scalex * left;
	bounds.bottom		= bounds.top + macGraphics->scaley * height;
	bounds.right		= bounds.top + macGraphics->scalex * width;

	dstbounds.top		= macContainerPtr->originy + macGraphics->originy + macGraphics->scaley * y2;
	dstbounds.left		= macContainerPtr->originx + macGraphics->originx + macGraphics->scalex * x2;
	dstbounds.bottom	= bounds.top + macGraphics->scaley * height;
	dstbounds.right		= bounds.top + macGraphics->scalex * width;

	CopyBits(	(BitMap*)	&macContainerPtr->dstPort->portPixMap,
				(BitMap*)	&macContainerPtr->dstPort->portPixMap,
				&bounds,
				&dstbounds,
				srcCopy,
				NULL);

	macContainerPtr->dstPort->clipRgn = swapRgn;					// swap the original cliprgn back
}

void sun_awt_macos_MacGraphics_drawRoundRect(struct Hsun_awt_macos_MacGraphics *Hgraphicspeer,long top,long left,long width,long height,long h,long v)
{
Classsun_awt_macos_MacGraphics		*graphicspeer		= (Classsun_awt_macos_MacGraphics*) unhand(Hgraphicspeer);		// Get pointer to graphics peer
macAwtGraphicsPtr					macGraphics			= (macAwtGraphicsPtr) PDATA(graphicspeer);						// Get pointer to graphics data
Classsun_awt_macos_MComponentPeer	*macComponentPeer	= unhand(macGraphics->Hcomppeer);								// Get pointer to the component peer
macAwtContainerPtr					macContainerPtr		= (macAwtContainerPtr) PDATA(macComponentPeer);					// Get pointer to the component data
Rect								bounds;
RgnHandle							swapRgn;
RGBColor							macColor;

	SetPort((GrafPtr) macContainerPtr->dstPort);

	swapRgn = macContainerPtr->dstPort->clipRgn;					// Save the destination cliprgn
	macContainerPtr->dstPort->clipRgn = macContainerPtr->cliprgn;	// install our own cliprgn

	ConvertAwtToMacColor(&macGraphics->foreground, &macColor);
	RGBForeColor(&macColor);										//Setup the foreground color

	bounds.top		= macContainerPtr->originy + macGraphics->originy + top  * macGraphics->scaley;
	bounds.left		= macContainerPtr->originx + macGraphics->originx + left * macGraphics->scalex;
	bounds.bottom	= bounds.top + macGraphics->scaley * height;
	bounds.right	= bounds.left + macGraphics->scalex * width;

	FrameRoundRect(&bounds, h, v);

	macContainerPtr->dstPort->clipRgn = swapRgn;					// swap the original cliprgn back
}

void sun_awt_macos_MacGraphics_fillRoundRect(struct Hsun_awt_macos_MacGraphics *Hgraphicspeer,long top,long left,long width,long height,long h,long v)
{
Classsun_awt_macos_MacGraphics		*graphicspeer		= (Classsun_awt_macos_MacGraphics*) unhand(Hgraphicspeer);		// Get pointer to graphics peer
macAwtGraphicsPtr					macGraphics			= (macAwtGraphicsPtr) PDATA(graphicspeer);						// Get pointer to graphics data
Classsun_awt_macos_MComponentPeer	*macComponentPeer	= unhand(macGraphics->Hcomppeer);								// Get pointer to the component peer
macAwtContainerPtr					macContainerPtr		= (macAwtContainerPtr) PDATA(macComponentPeer);					// Get pointer to the component data
Rect								bounds;
RgnHandle							swapRgn;
RGBColor							macColor;

	SetPort((GrafPtr) macContainerPtr->dstPort);

	swapRgn = macContainerPtr->dstPort->clipRgn;					// Save the destination cliprgn
	macContainerPtr->dstPort->clipRgn = macContainerPtr->cliprgn;	// install our own cliprgn

	ConvertAwtToMacColor(&macGraphics->foreground, &macColor);
	RGBForeColor(&macColor);										//Setup the foreground color

	bounds.top		= macContainerPtr->originy + macGraphics->originy + top  * macGraphics->scaley;
	bounds.left		= macContainerPtr->originx + macGraphics->originx + left * macGraphics->scalex;
	bounds.bottom	= bounds.top + macGraphics->scaley * height;
	bounds.right	= bounds.left + macGraphics->scalex * width;

	PaintRoundRect(&bounds, h, v);

	macContainerPtr->dstPort->clipRgn = swapRgn;					// swap the original cliprgn back
}

void sun_awt_macos_MacGraphics_drawPolygon(struct Hsun_awt_macos_MacGraphics *Hgraphicspeer,HArrayOfInt *XArray,HArrayOfInt *YArray,long count)
{
int									i;
int									*X					= (int*) unhand(XArray)->body;
int									*Y					= (int*) unhand(YArray)->body;
Classsun_awt_macos_MacGraphics		*graphicspeer		= (Classsun_awt_macos_MacGraphics*) unhand(Hgraphicspeer);		// Get pointer to graphics peer
macAwtGraphicsPtr					macGraphics			= (macAwtGraphicsPtr) PDATA(graphicspeer);						// Get pointer to graphics data
Classsun_awt_macos_MComponentPeer	*macComponentPeer	= unhand(macGraphics->Hcomppeer);								// Get pointer to the component peer
macAwtContainerPtr					macContainerPtr		= (macAwtContainerPtr) PDATA(macComponentPeer);					// Get pointer to the component data
RgnHandle							swapRgn;
RGBColor							macColor;

	SetPort((GrafPtr) macContainerPtr->dstPort);

	swapRgn = macContainerPtr->dstPort->clipRgn;					// Save the destination cliprgn
	macContainerPtr->dstPort->clipRgn = macContainerPtr->cliprgn;	// install our own cliprgn

	ConvertAwtToMacColor(&macGraphics->foreground, &macColor);
	RGBForeColor(&macColor);										//Setup the foreground color

	MoveTo(macContainerPtr->originx + macGraphics->originx + X[0] * macGraphics->scalex, macContainerPtr->originy + macGraphics->originy + Y[0] * macGraphics->scaley);
	for(i=1;i<count;i++)
	{
		LineTo(macContainerPtr->originx + macGraphics->originx + X[i] * macGraphics->scalex, macContainerPtr->originy + macGraphics->originy + Y[i] * macGraphics->scaley);
	}

	macContainerPtr->dstPort->clipRgn = swapRgn;					// swap the original cliprgn back
}

void sun_awt_macos_MacGraphics_fillPolygon(struct Hsun_awt_macos_MacGraphics *Hgraphicspeer,HArrayOfInt *XArray,HArrayOfInt *YArray,long count)
{
int									i;
int									*X					= (int*) unhand(XArray)->body;
int									*Y					= (int*) unhand(YArray)->body;
PolyHandle							thePoly;
Classsun_awt_macos_MacGraphics		*graphicspeer		= (Classsun_awt_macos_MacGraphics*) unhand(Hgraphicspeer);		// Get pointer to graphics peer
macAwtGraphicsPtr					macGraphics			= (macAwtGraphicsPtr) PDATA(graphicspeer);						// Get pointer to graphics data
Classsun_awt_macos_MComponentPeer	*macComponentPeer	= unhand(macGraphics->Hcomppeer);								// Get pointer to the component peer
macAwtContainerPtr					macContainerPtr		= (macAwtContainerPtr) PDATA(macComponentPeer);					// Get pointer to the component data
RgnHandle							swapRgn;
RGBColor							macColor;

	SetPort((GrafPtr) macContainerPtr->dstPort);

	swapRgn = macContainerPtr->dstPort->clipRgn;					// Save the destination cliprgn
	macContainerPtr->dstPort->clipRgn = macContainerPtr->cliprgn;	// install our own cliprgn

	ConvertAwtToMacColor(&macGraphics->foreground, &macColor);
	RGBForeColor(&macColor);										//Setup the foreground color

	thePoly = OpenPoly();
	MoveTo(macContainerPtr->originx + macGraphics->originx + X[0] * macGraphics->scalex, macContainerPtr->originy + macGraphics->originy + Y[0] * macGraphics->scaley);
	for(i=1;i<count;i++)
	{
		LineTo(macContainerPtr->originx + macGraphics->originx + X[i] * macGraphics->scalex, macContainerPtr->originy + macGraphics->originy + Y[i] * macGraphics->scaley);
	}

	ClosePoly();
	PaintPoly(thePoly);
	KillPoly(thePoly);

	macContainerPtr->dstPort->clipRgn = swapRgn;					// swap the original cliprgn back
}

void sun_awt_macos_MacGraphics_drawOval(struct Hsun_awt_macos_MacGraphics *Hgraphicspeer,long top,long left,long width,long height)
{
Classsun_awt_macos_MacGraphics		*graphicspeer		= (Classsun_awt_macos_MacGraphics*) unhand(Hgraphicspeer);		// Get pointer to graphics peer
macAwtGraphicsPtr					macGraphics			= (macAwtGraphicsPtr) PDATA(graphicspeer);						// Get pointer to graphics data
Classsun_awt_macos_MComponentPeer	*macComponentPeer	= unhand(macGraphics->Hcomppeer);								// Get pointer to the component peer
macAwtContainerPtr					macContainerPtr		= (macAwtContainerPtr) PDATA(macComponentPeer);					// Get pointer to the component data
Rect								bounds;
RgnHandle							swapRgn;
RGBColor							macColor;

	SetPort((GrafPtr) macContainerPtr->dstPort);

	swapRgn = macContainerPtr->dstPort->clipRgn;					// Save the destination cliprgn
	macContainerPtr->dstPort->clipRgn = macContainerPtr->cliprgn;	// install our own cliprgn

	ConvertAwtToMacColor(&macGraphics->foreground, &macColor);
	RGBForeColor(&macColor);										//Setup the foreground color

	bounds.top		= macContainerPtr->originy + macGraphics->originy + top  * macGraphics->scaley;
	bounds.left		= macContainerPtr->originx + macGraphics->originx + left * macGraphics->scalex;
	bounds.bottom	= bounds.top + macGraphics->scaley * height;
	bounds.right	= bounds.left + macGraphics->scalex * width;

	FrameOval(&bounds);

	macContainerPtr->dstPort->clipRgn = swapRgn;					// swap the original cliprgn back
}

void sun_awt_macos_MacGraphics_fillOval(struct Hsun_awt_macos_MacGraphics *Hgraphicspeer,long top,long left,long width,long height)
{
Classsun_awt_macos_MacGraphics		*graphicspeer		= (Classsun_awt_macos_MacGraphics*) unhand(Hgraphicspeer);		// Get pointer to graphics peer
macAwtGraphicsPtr					macGraphics			= (macAwtGraphicsPtr) PDATA(graphicspeer);						// Get pointer to graphics data
Classsun_awt_macos_MComponentPeer	*macComponentPeer	= unhand(macGraphics->Hcomppeer);								// Get pointer to the component peer
macAwtContainerPtr					macContainerPtr		= (macAwtContainerPtr) PDATA(macComponentPeer);					// Get pointer to the component data
Rect								bounds;
RgnHandle							swapRgn;
RGBColor							macColor;

	SetPort((GrafPtr) macContainerPtr->dstPort);

	swapRgn = macContainerPtr->dstPort->clipRgn;					// Save the destination cliprgn
	macContainerPtr->dstPort->clipRgn = macContainerPtr->cliprgn;	// install our own cliprgn

	ConvertAwtToMacColor(&macGraphics->foreground, &macColor);
	RGBForeColor(&macColor);										//Setup the foreground color

	bounds.top		= macContainerPtr->originy + macGraphics->originy + top  * macGraphics->scaley;
	bounds.left		= macContainerPtr->originx + macGraphics->originx + left * macGraphics->scalex;
	bounds.bottom	= bounds.top + macGraphics->scaley * height;
	bounds.right	= bounds.left + macGraphics->scalex * width;

	PaintOval(&bounds);

	macContainerPtr->dstPort->clipRgn = swapRgn;					// swap the original cliprgn back
}

void sun_awt_macos_MacGraphics_drawArc(struct Hsun_awt_macos_MacGraphics *,long,long,long,long,long,long)
{
}

void sun_awt_macos_MacGraphics_fillArc(struct Hsun_awt_macos_MacGraphics *,long,long,long,long,long,long)
{
}



