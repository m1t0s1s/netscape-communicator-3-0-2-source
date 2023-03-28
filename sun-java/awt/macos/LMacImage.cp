//##############################################################################
//##############################################################################
//
//	File:		LMacImage.cp
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
#include "interpreter.h"
#include "java_awt_image_ColorModel.h"
#include "j_awt_image_IndexColorModel.h"
#include "j_awt_image_DirectColorModel.h"
#include "sun_awt_image_Image.h"
#include "s_a_image_ImageRepresentation.h"
#include "java_awt_Graphics.h"
#include "sun_awt_macos_MacGraphics.h"
#include "s_a_i_OffScreenImageSource.h"
#include "exceptions.h"
#include "mdmacmem.h"
#include "MToolkit.h"

};

#include <Quickdraw.h>
#include <QDOffscreen.h>

#include "LMacGraphics.h"
#include "fredmem.h"

struct PrivateMacImageData {
	GWorldPtr					imageGWorld;
	Ptr							imageAlpha;
	long						imageAlphaRowBytes;
	BitMap						imageMask;
	long						imageUsesAlpha;
	long						imageUsesBinaryMask;
};

typedef struct PrivateMacImageData PrivateMacImageData;

extern Hjava_awt_image_ColorModel 	*gStandard8BitColorModel;
GWorldPtr							gSourceGWorld = NULL;

enum {
	kAlphaMixingBufferWidth 	= 75,
	kAlphaMixingBufferHeight 	= 75,
	kImageAllocationSizeFudge	= 12 * 1024
};

CTabHandle TranslateColorModelToClut(struct Hjava_awt_image_ColorModel *colorModel, short depth)
{
	Classjava_awt_image_IndexColorModel	*colorModelStruct;
	UInt8 								*currentRed;
	UInt8 								*currentBlue;
	UInt8 								*currentGreen;
	short								tableSize;
	ColorSpec							*currentArrayItem;
	CTabHandle							colorTable;

	if (depth >= 16) {
		colorTable = (CTabHandle)NewHandleClear(sizeof(ColorTable));
		tableSize = 0;
	}

	else {

		colorModelStruct	= (Classjava_awt_image_IndexColorModel *)unhand(colorModel);
		currentRed 			= (UInt8 *)unhand(colorModelStruct->red)->body;
		currentBlue 		= (UInt8 *)unhand(colorModelStruct->blue)->body;
		currentGreen 		= (UInt8 *)unhand(colorModelStruct->green)->body;

		colorTable = GetCTable(2048);
		tableSize = colorModelStruct->map_size;
	}
	
	if (colorTable == NULL)
		return NULL;

	//	If the color table we are using is the standard color 
	//	system color table, then don’t rebuild it.
	
	if (colorModel == gStandard8BitColorModel)
		return colorTable;

	//	The color table is not the standard 8-bit one.  Create
	//	a new 'clut' out of the color table.
	
	currentArrayItem = (**colorTable).ctTable;	
	
	for (UInt32 count = 0; count < tableSize; count++) {
		
		UInt8	red, blue, green;
		
		red = *currentRed++;
		blue = *currentBlue++;
		green = *currentGreen++;
		
		currentArrayItem->rgb.red = red | (red << 8);
		currentArrayItem->rgb.blue = blue | (blue << 8);
		currentArrayItem->rgb.green = green | (green << 8);

		currentArrayItem++;
		
	}
	
	(**colorTable).ctSize = tableSize;

	if (depth < 16)
		CTabChanged(colorTable);
	
	return colorTable;
	
}

extern void sun_awt_image_ImageRepresentation_offscreenInit(struct Hsun_awt_image_ImageRepresentation *imageRepresentationObject)
{
	struct Classsun_awt_image_ImageRepresentation 		*imageRepresentationStruct = unhand(imageRepresentationObject);
	
}

UInt8 *GetDestinationAlphaStorage(struct Hsun_awt_image_ImageRepresentation *imageRepresentationObject)
{
	Classsun_awt_image_ImageRepresentation 		*imageRepresentationStruct = unhand(imageRepresentationObject);
	return (UInt8 *)((PrivateMacImageData *)(imageRepresentationStruct->pData))->imageAlpha;
}

UInt32 *GetDestinationMaskStorage(struct Hsun_awt_image_ImageRepresentation *imageRepresentationObject, UInt32 *rowBytes)
{
	Classsun_awt_image_ImageRepresentation 		*imageRepresentationStruct = unhand(imageRepresentationObject);
	*rowBytes = ((PrivateMacImageData *)(imageRepresentationStruct->pData))->imageMask.rowBytes;
	return (UInt32 *)((PrivateMacImageData *)(imageRepresentationStruct->pData))->imageMask.baseAddr;
}

GWorldPtr GetDestinationBackingStoreWorld(struct Hsun_awt_image_ImageRepresentation *imageRepresentationObject, struct Hjava_awt_image_ColorModel *colorModel, short depth, Boolean transparent)
{
	Classsun_awt_image_ImageRepresentation 				*imageRepresentationStruct = unhand(imageRepresentationObject);
	Classsun_awt_image_Image							*ownerImage = unhand(imageRepresentationStruct->image);
	PrivateMacImageData									*newImageData = NULL;
	GWorldPtr											newGWorld = NULL;
	Size												maskDataRowBytes;
	Ptr													alphaData = NULL;
	Ptr													maskData = NULL;
	short												toFillWith = transparent ? 0 : 0xFFFF;
 	CTabHandle 											colorTable = NULL;
 
	if (imageRepresentationStruct->pData == NULL) {
	
		Rect					globalRect = { -32767, -32767, 32767, 32767 },
								newOffscreenBounds;
		CGrafPtr				origPort;
		GDHandle				origDevice;
		
		//	If we are asked for an image of depth zero and it doesn’t already
		//	exist, then fail.
		
		if (depth == 0)
			return NULL;
	
		//	Reserve 40% more than the actual allocation so that 
		//	we allo some overhead for gif/jpeg buffers.  This is 
		//	part of our "pre-flight" strategy.
		
		Size	allocationSize = ((1L * imageRepresentationStruct->srcW * imageRepresentationStruct->srcH) *
								 ((depth / 8) + 1) + kImageAllocationSizeFudge) * 14 / 10;
		
		if (!Memory_ReserveInMacHeap(allocationSize))
			return NULL;
		
		newImageData = (PrivateMacImageData *)malloc(sizeof(PrivateMacImageData));
		if (newImageData == NULL) goto errorExit;
		memset(newImageData, 0, sizeof(PrivateMacImageData));
	
		newImageData->imageUsesAlpha = false;
		newImageData->imageUsesBinaryMask = true;

		colorTable = TranslateColorModelToClut(colorModel, depth);
		if (colorTable == NULL) goto errorExit;
		
		//	Allocate the offscreen image buffer.
			
		SetRect(&newOffscreenBounds, 0, 0, imageRepresentationStruct->srcW, imageRepresentationStruct->srcH);
		
		NewGWorld(&newGWorld, depth, &newOffscreenBounds, colorTable, NULL, keepLocal | useTempMem);
		DisposeHandle((Handle)colorTable);
		if (newGWorld == NULL) goto errorExit;
		
		GetGWorld(&origPort, &origDevice);
		SetGWorld(newGWorld, NULL);

		RGBColor greyColor = { 192 << 8, 192 << 8, 192 << 8 };

		::RGBForeColor(&greyColor);
		::PaintRect(&newOffscreenBounds);
		
		SetGWorld(origPort, origDevice);

		newImageData->imageGWorld = newGWorld;
		
		//	Allocate the alpha channel storage.
		
		Size	alphaDataSize = imageRepresentationStruct->srcW * imageRepresentationStruct->srcH,
				roundedAlphaDataSize = (alphaDataSize + 3) & 0xFFFFFFFC;
		
		alphaData = (Ptr)malloc(roundedAlphaDataSize);
		if (alphaData == NULL) goto errorExit;
		
		newImageData->imageAlpha = alphaData;
		newImageData->imageAlphaRowBytes = imageRepresentationStruct->srcW;
		memset(alphaData, toFillWith, roundedAlphaDataSize);

		//	Allocate the bitmap storage, rounding the bitmap rowbytes to the
		//	nearest 32-bit boundary.
		
		maskDataRowBytes = (((imageRepresentationStruct->srcW + 7)/ 8) + 3) & 0xFFFFFFFC;
		maskData = (Ptr)malloc(maskDataRowBytes * imageRepresentationStruct->srcH);
		if (maskData == NULL) goto errorExit;	
	
		newImageData->imageMask.baseAddr = maskData;
		newImageData->imageMask.rowBytes = maskDataRowBytes;
		newImageData->imageMask.bounds = newOffscreenBounds;
		memset(maskData, toFillWith, maskDataRowBytes * imageRepresentationStruct->srcH);

		imageRepresentationStruct->pData = (long)newImageData;
	
	}

	else {
	
		newGWorld = ((PrivateMacImageData *)(imageRepresentationStruct->pData))->imageGWorld;
	
	}

	return newGWorld;
	
errorExit:

	//	Clean up and signal an error.

	if (newImageData != NULL) {
	
		if (newGWorld != NULL)
			DisposeGWorld(newGWorld);
			
		if (alphaData != NULL)
			free(alphaData);
			
		if (maskData != NULL)
			free(maskData);
	
		free(newImageData);
		
	}
	
	SignalError(0, JAVAPKG "OutOfMemoryError", 0);

	return NULL;

}

void sun_awt_image_OffScreenImageSource_sendPixels(struct Hsun_awt_image_OffScreenImageSource *imageSource)
{
	// FIX ME
}

long sun_awt_image_ImageRepresentation_setBytePixels(struct Hsun_awt_image_ImageRepresentation *imageRepresentationObject, long x, long y, 
		long width, long height, struct Hjava_awt_image_ColorModel *colorModel, HArrayOfByte *pixelData, long offset, long scansize)
{
	Classsun_awt_image_ImageRepresentation 				*imageRepresentationStruct = unhand(imageRepresentationObject);
	PrivateMacImageData									*privateImageData;
	Classjava_awt_image_IndexColorModel					*colorModelStruct	= (Classjava_awt_image_IndexColorModel *)unhand(colorModel);
	UInt8 												*alphaLookup;
	UInt8												transparentIndex = colorModelStruct->transparent_index;
	GWorldPtr											destinationWorld;
	PixMapHandle										destinationPixMap;
	UInt8												*baseDestinationPixel,
														*currentDestinationPixel;
	UInt8												*baseSourcePixel,
														*currentSourcePixel;
	UInt8												*baseAlpha,
														*currentAlpha;
	UInt32												maskRowBytes,
														*currentMask,
														*baseMask;
	UInt32												currentBit,
														baseBit;
														
	alphaLookup = (colorModelStruct->alpha == NULL) ? NULL : (UInt8 *)unhand(colorModelStruct->alpha)->body;

	//	If no backing store exists for the image representation, create one.
	
	destinationWorld = GetDestinationBackingStoreWorld(imageRepresentationObject, colorModel, 8, true);

	privateImageData = (PrivateMacImageData *)(imageRepresentationStruct->pData);

	if (destinationWorld == NULL)
		return 0;
	
	destinationPixMap = GetGWorldPixMap(destinationWorld);
	
	baseSourcePixel = (UInt8 *)(unhand(pixelData)->body + offset);
	baseDestinationPixel = (UInt8 *)(GetPixBaseAddr(destinationPixMap)) + ((**destinationPixMap).rowBytes & 0x7FFF) * y + x;
	baseAlpha = GetDestinationAlphaStorage(imageRepresentationObject) + width * y + x;
	baseMask = GetDestinationMaskStorage(imageRepresentationObject, &maskRowBytes);

	maskRowBytes = maskRowBytes >> 2;

	baseMask += y * maskRowBytes + (x / 32);
	baseBit = 0x80000000 >> (x % 32);

	while (height--) {
	
		UInt32 widthCopy = width;
		
		currentDestinationPixel = baseDestinationPixel;
		currentSourcePixel = baseSourcePixel;
		currentAlpha = baseAlpha;
		currentMask = baseMask;
		currentBit = baseBit;
		
		while (widthCopy--) {
			
			UInt8 currentSourcePixelValue = *currentSourcePixel++;
			*currentDestinationPixel++ = currentSourcePixelValue;
			
			UInt8 currentAlphaPixel = 0xFF;
			
			if (alphaLookup != NULL)
				currentAlphaPixel = alphaLookup[currentSourcePixelValue];
			
			if ((transparentIndex == currentSourcePixelValue) && (transparentIndex != 0xFF))
				currentAlphaPixel = 0;
				
			if (currentAlphaPixel != 0xFF) {
				privateImageData->imageUsesAlpha = true;
				if (currentAlphaPixel != 0)			
					privateImageData->imageUsesBinaryMask = false;
			}
			
			*currentAlpha++ = currentAlphaPixel;
			if (currentAlphaPixel == 0xFF)
				*currentMask |= currentBit;
			else
				*currentMask &= ~currentBit;
				
			currentBit = currentBit >> 1;
			if (currentBit == 0) {
				currentMask++;
				currentBit = 0x80000000;
			}
				
		}
		
		baseDestinationPixel += ((**destinationPixMap).rowBytes & 0x7FFF);
		baseSourcePixel += scansize;
		baseAlpha += width;
		baseMask += maskRowBytes;
		
	}
	
	return 1;
	
}

long sun_awt_image_ImageRepresentation_setIntPixels(struct Hsun_awt_image_ImageRepresentation *imageRepresentationObject, long x, long y, 
		long width, long height, struct Hjava_awt_image_ColorModel *colorModel, HArrayOfInt *pixelData, long offset, long scansize)
{
	Classsun_awt_image_ImageRepresentation 				*imageRepresentationStruct = unhand(imageRepresentationObject);
	ClassClass											*directColorModelClass;
	PrivateMacImageData									*privateImageData;
	GWorldPtr											destinationWorld;
	PixMapHandle										destinationPixMap;
	UInt16												*baseDestinationPixel,
														*currentDestinationPixel;
	UInt32												*baseSourcePixel,
														*currentSourcePixel;
	UInt8												*baseAlpha,
														*currentAlpha;
	UInt32												maskRowBytes,
														*currentMask,
														*baseMask;
	UInt32												currentBit,
														baseBit;
	Boolean												isDirectColorModel;
	Boolean												usesAlpha = false;
														
	//	If no backing store exists for the image representation, create one.
	
	destinationWorld = GetDestinationBackingStoreWorld(imageRepresentationObject, colorModel, 16, true);

	directColorModelClass = FindClass(EE(), "java/awt/image/DirectColorModel", TRUE);
	isDirectColorModel = (obj_classblock(colorModel) == directColorModelClass);
	
	if (isDirectColorModel) {
		Classjava_awt_image_DirectColorModel	*directColorModel = (Classjava_awt_image_DirectColorModel *)unhand(colorModel);
		usesAlpha = (directColorModel->alpha_bits == 8);	
	}

	privateImageData = (PrivateMacImageData *)(imageRepresentationStruct->pData);

	if (destinationWorld == NULL)
		return 0;
	
	destinationPixMap = GetGWorldPixMap(destinationWorld);
	
	baseSourcePixel = (UInt32 *)(unhand(pixelData)->body + offset);
	baseDestinationPixel = (UInt16 *)((UInt8 *)(GetPixBaseAddr(destinationPixMap)) + ((**destinationPixMap).rowBytes & 0x7FFF) * y + x);
	baseAlpha = GetDestinationAlphaStorage(imageRepresentationObject) + width * y + x;
	baseMask = GetDestinationMaskStorage(imageRepresentationObject, &maskRowBytes);

	maskRowBytes = maskRowBytes >> 2;

	baseMask += y * maskRowBytes + (x / 32);
	baseBit = 0x80000000 >> (x % 32);

	while (height--) {
	
		UInt32 widthCopy = width;
		
		currentDestinationPixel = baseDestinationPixel;
		currentSourcePixel = baseSourcePixel;
		currentAlpha = baseAlpha;
		currentMask = baseMask;
		currentBit = baseBit;
		
		while (widthCopy--) {
			
			UInt32 	currentSourcePixelValue = *currentSourcePixel++;
			UInt32 	currentDestinationPixelValue = 0;
			UInt8	currentAlphaPixel;
	
			//	Extract the alpha pixel
	
			if (usesAlpha)
				currentAlphaPixel = (currentSourcePixelValue & 0xFF000000) >> 24;
			else
				currentAlphaPixel = 0xFF;
			
			// Convert here.			

			currentDestinationPixelValue = (currentSourcePixelValue & 0x000000F8) >> 3;
			currentDestinationPixelValue |= (currentSourcePixelValue & 0x0000F800) >> 6;
			currentDestinationPixelValue |= (currentSourcePixelValue & 0x00F80000) >> 9;

			*currentDestinationPixel++ = currentDestinationPixelValue;
				
			if (currentAlphaPixel != 0xFF) {
				privateImageData->imageUsesAlpha = true;
				if (currentAlphaPixel != 0)			
					privateImageData->imageUsesBinaryMask = false;
			}
			
			*currentAlpha++ = currentAlphaPixel;
			if (currentAlphaPixel == 0xFF)
				*currentMask |= currentBit;
			else
				*currentMask &= ~currentBit;
				
			currentBit = currentBit >> 1;
			if (currentBit == 0) {
				currentMask++;
				currentBit = 0x80000000;
			}
				
		}
		
		baseDestinationPixel += ((**destinationPixMap).rowBytes & 0x7FFF) >> 1;
		baseSourcePixel += scansize;
		baseAlpha += width;
		baseMask += maskRowBytes;
		
	}
	
	return 1;
	
}

long sun_awt_image_ImageRepresentation_finish(struct Hsun_awt_image_ImageRepresentation *imageRepresentationObject, long force)
{
	return true;
}

void sun_awt_image_ImageRepresentation_imageDraw(struct Hsun_awt_image_ImageRepresentation *imageRepresentationObject, struct Hjava_awt_Graphics *graphicsObject, long x, long y, struct Hjava_awt_Color *)
{
	struct Classsun_awt_image_ImageRepresentation 	*imageRepresentation = unhand(imageRepresentationObject);
	struct Classsun_awt_image_Image					*ownerImage = unhand(imageRepresentation->image);
	LMacGraphics	 								*ppMacGraphics;
	GrafPtr											destinationPort;
	GWorldPtr										imageGWorld;
	Rect											sourceRectangle,
													destinationRectangle;

	imageGWorld = GetDestinationBackingStoreWorld(imageRepresentationObject, NULL, 0, true);

	//	If we don’t have the destination image yet (still need to
	//	create it), then do nothing.
	
	if (imageGWorld == NULL)
		return;

	ppMacGraphics = (LMacGraphics *)(unhand((struct Hsun_awt_macos_MacGraphics *)graphicsObject)->pData);

	if (ppMacGraphics->BeginDrawing()) {
	
		PrivateMacImageData	*privateImageData = (PrivateMacImageData *)(imageRepresentation->pData);

		destinationPort = ppMacGraphics->GetOwnerPort();
	
		sourceRectangle = (**(GetGWorldPixMap(imageGWorld))).bounds;

		ppMacGraphics->ConvertToPortRect(destinationRectangle, x, y, imageRepresentation->width, imageRepresentation->height);

		ForeColor(blackColor);
		BackColor(whiteColor);
	
		if (!(privateImageData->imageUsesAlpha)) {
			
			//	Simplest mode, no mask.  Use simple CopyBits.
			
			CopyBits(&(((GrafPtr)imageGWorld)->portBits),
					&((GrafPtr)destinationPort)->portBits,
					&sourceRectangle,
					&destinationRectangle,
					srcCopy,
					NULL);

		}
		
		else if (privateImageData->imageUsesBinaryMask) {
		
			//	Image uses alpha mask, but is binary (no mixing necessary).
			//	Use CopyMask.
			
			CopyMask(&(((GrafPtr)imageGWorld)->portBits),
					&(privateImageData->imageMask),
					&((GrafPtr)destinationPort)->portBits,
					&sourceRectangle,
					&sourceRectangle,
					&destinationRectangle);
		
		}
		
		else {
		
			static GWorldPtr 	gSourceMixingWorld = NULL;
			static GWorldPtr	gDestinationMixingWorld = NULL;
			static UInt8		*gSourceAlphaBitMap = NULL;
			PixMapHandle		gSourceMixingMap;
			PixMapHandle		gDestiantionMixingMap;
			Boolean				gCanMixAlpha;

			//	Try to use the two bitmaps that we will use for for mixing
			//	the alpha channel into the final image (if we don't have
			//	them already).

			Memory_ReserveInMacHeap(128L * 1024);

			if (gSourceMixingWorld == NULL) {	
				Rect mixingBounds = { 0, 0, kAlphaMixingBufferWidth, kAlphaMixingBufferHeight }; 
				::NewGWorld(&gSourceMixingWorld, 32, &mixingBounds, NULL, NULL, 0);
				if (gSourceMixingWorld == NULL) {
					SignalError(0, JAVAPKG "OutOfMemoryError", 0);
					return;
				}
			}

			if (gDestinationMixingWorld == NULL) {	
				Rect mixingBounds = { 0, 0, kAlphaMixingBufferWidth, kAlphaMixingBufferHeight }; 
				::NewGWorld(&gDestinationMixingWorld, 32, &mixingBounds, NULL, NULL, 0);
				if (gDestinationMixingWorld == NULL) {
					SignalError(0, JAVAPKG "OutOfMemoryError", 0);
					return;
				}
			}
			
			if (gSourceAlphaBitMap == NULL) {
				UInt32 allocSize = kAlphaMixingBufferWidth * kAlphaMixingBufferWidth;
				gSourceAlphaBitMap = (UInt8 *)malloc(allocSize);
				if (gSourceAlphaBitMap == NULL) {
					SignalError(0, JAVAPKG "OutOfMemoryError", 0);
					return;
				}
				memset(gSourceAlphaBitMap, 0xFF, allocSize);
			}
			
			gCanMixAlpha = (gSourceMixingWorld != NULL) && (gDestinationMixingWorld != NULL);
		
			//	If we have been able to allocaing alpha mixing buffers, then iterate 
			//	over the source image, mixing it into the destination.  If we were
			//	unsuccessful in allocating buffers, then fall back on our CopyMask code.
			//	This will copy the image, but won't allow transparency. 
		
			if ((imageRepresentation->width == 0) || (imageRepresentation->height == 0))
				gCanMixAlpha = false;
		
			if (gCanMixAlpha) {
			
				UInt32		horizontalPatches = (imageRepresentation->width - 1) / 
												kAlphaMixingBufferWidth + 1;
				UInt32		verticalPatches = (imageRepresentation->height - 1) /
												kAlphaMixingBufferHeight + 1;
				UInt32		currentHPatch, currentVPatch;
						 
				gSourceMixingMap = GetGWorldPixMap(gSourceMixingWorld);
				gDestiantionMixingMap = GetGWorldPixMap(gDestinationMixingWorld);
				
				for (currentVPatch = 0; currentVPatch < verticalPatches; currentVPatch++) {

					for (currentHPatch = 0; currentHPatch < horizontalPatches; currentHPatch++) {
				
						Rect	currentMixRect;
						
						::SetRect(&currentMixRect, 
								x + currentHPatch * kAlphaMixingBufferWidth,
								y + currentVPatch * kAlphaMixingBufferWidth,
								x + (currentHPatch + 1) * kAlphaMixingBufferWidth,
								y + (currentVPatch + 1) * kAlphaMixingBufferWidth);

						//	Pin the patch rectangle to the maximum size of the
						//	destination rectangle.

						if (currentMixRect.right > destinationRectangle.right)
							currentMixRect.right = destinationRectangle.right;

						if (currentMixRect.bottom > destinationRectangle.bottom)
							currentMixRect.bottom = destinationRectangle.bottom;

						//	Set both alpha mixing worlds to use the current mixing rect.
						
						(**gSourceMixingMap).bounds = currentMixRect;
						(**gDestiantionMixingMap).bounds = currentMixRect;

						gSourceMixingWorld->portRect = currentMixRect;
						gDestinationMixingWorld->portRect = currentMixRect;
					
						(**(gSourceMixingWorld->visRgn)).rgnBBox = currentMixRect;
						(**(gDestinationMixingWorld->visRgn)).rgnBBox = currentMixRect;

						//	Save our port and device information.  We will need it to 
						//	do the final copy bits.
						
						CGrafPtr		savedPort;
						GDHandle		savedDevice;
						
						GetGWorld(&savedPort, &savedDevice); 

						//	Copy (stretch) the source image into the first buffer.
						
						SetGWorld(gSourceMixingWorld, NULL);

						CopyBits(&(((GrafPtr)imageGWorld)->portBits),
								&((GrafPtr)gSourceMixingWorld)->portBits,
								&sourceRectangle,
								&destinationRectangle,
								srcCopy,
								NULL);
								
						//	Scale part of the original alpha mask into the scaled mask
						
						UInt32	scaledSourceFactorHeight,
								scaledSourceFactorWidth;
						UInt32	currentAlphaX,
								currentAlphaY;
						UInt32	horizMax = currentMixRect.right - currentMixRect.left;
						UInt32	vertMax = currentMixRect.bottom - currentMixRect.top;
						
						scaledSourceFactorWidth = (sourceRectangle.right - sourceRectangle.left) << 16;
						scaledSourceFactorWidth = scaledSourceFactorWidth / (destinationRectangle.right - destinationRectangle.left);
								
						scaledSourceFactorHeight = (sourceRectangle.bottom - sourceRectangle.top) << 16;
						scaledSourceFactorHeight = scaledSourceFactorHeight / (destinationRectangle.bottom - destinationRectangle.top);

						for (currentAlphaY = 0; currentAlphaY < vertMax; currentAlphaY++) {
						
							UInt8	*currentDestinationAlphaPixel = gSourceAlphaBitMap + (currentAlphaY * kAlphaMixingBufferWidth);
					
							for (currentAlphaX = 0; currentAlphaX < horizMax; currentAlphaX++) {
							
								UInt8			*sourceImageBase;
								UInt32			sourceX;
								UInt32			sourceY;
								
								sourceX = ((currentMixRect.left - destinationRectangle.left 
										+ currentAlphaX) * scaledSourceFactorWidth) >> 16;
										
								sourceY = ((currentMixRect.top - destinationRectangle.top 
										+ currentAlphaY) * scaledSourceFactorHeight) >> 16;
									
								sourceImageBase = (UInt8 *)(privateImageData->imageAlpha) + (sourceY * imageRepresentation->srcW) + sourceX;
								
								*currentDestinationAlphaPixel++ = *sourceImageBase;
							
							}
						
						}
								
						//	Copy the destination image into the second buffer.
						
						SetGWorld(gDestinationMixingWorld, NULL);

						CopyBits(&(((GrafPtr)destinationPort)->portBits),
								&((GrafPtr)gDestinationMixingWorld)->portBits,
								&currentMixRect,
								&currentMixRect,
								srcCopy,
								NULL);
						
						//	Iterate over the pixels in the mixing buffers, combining
						//	the alpha component of the source image with the destination.
						
						UInt32		currentPatchH,
									currentPatchV;
						Ptr			sourceMixBase,
									destinationMixBase;
						UInt8		*currentSourceMixPixelPtr,
									*currentDestinationMixPixelPtr;
						UInt32		sourceRowBytes = ((**gSourceMixingMap).rowBytes & 0x7FFF);
						UInt32		destinationRowBytes = ((**gDestiantionMixingMap).rowBytes & 0x7FFF);
						UInt8		*currentAlpha;
						
						sourceMixBase = ::GetPixBaseAddr(gSourceMixingMap);
						destinationMixBase = ::GetPixBaseAddr(gDestiantionMixingMap);
						
						for (currentPatchV = 0; currentPatchV < vertMax; currentPatchV++) {
						
							currentSourceMixPixelPtr = (UInt8 *)(sourceMixBase + currentPatchV * sourceRowBytes);
							currentDestinationMixPixelPtr = (UInt8 *)(destinationMixBase + currentPatchV * destinationRowBytes);								

							currentAlpha = gSourceAlphaBitMap + currentPatchV * kAlphaMixingBufferWidth;

							for (currentPatchH = 0; currentPatchH < horizMax; currentPatchH++) {
								
								UInt16		alphaS = *currentAlpha++;
								
								if (alphaS == 0xFF) {
								
									*((UInt32 *)currentDestinationMixPixelPtr) = *((UInt32 *)currentSourceMixPixelPtr);
								
									currentSourceMixPixelPtr += 4;
									currentDestinationMixPixelPtr += 4;

								}
								
								else if (alphaS == 0x00) {
								
									currentSourceMixPixelPtr += 4;
									currentDestinationMixPixelPtr += 4;
								
								} else {
								
									UInt16 	alphaD = 0xFF - alphaS;
								
									UInt32		currentSourceMixPixel = *((UInt32 *)currentSourceMixPixelPtr);
									UInt32		currentDestinationMixPixel = *((UInt32 *)currentDestinationMixPixelPtr);
									UInt32		addPixel1;
									UInt32		addPixel2;
									UInt32		resultPixel;

									addPixel1 = (currentSourceMixPixel & 0x000000FF) * alphaS + 0x000000FF;
									addPixel2 = (currentDestinationMixPixel & 0x000000FF) * alphaD + 0x000000FF;
									
									resultPixel = (addPixel1 + addPixel2) & 0x0000FF00;

									addPixel1 = (currentSourceMixPixel & 0x0000FF00) * alphaS + 0x0000FF00;
									addPixel2 = (currentDestinationMixPixel & 0x0000FF00) * alphaD + 0x0000FF00;

									resultPixel += (addPixel1 + addPixel2) & 0x00FF0000;
									resultPixel &= 0xFFFFFF;
	
									addPixel1 = (currentSourceMixPixel & 0x00FF0000) * alphaS + 0x00FF0000;
									addPixel2 = (currentDestinationMixPixel & 0x00FF0000) * alphaD + 0x00FF0000;

									resultPixel += (addPixel1 + addPixel2) & 0xFF000000;
									
									resultPixel = resultPixel >> 8;
								
									*((UInt32 *)currentDestinationMixPixelPtr) = resultPixel;
								
									currentDestinationMixPixelPtr += 4;
									currentSourceMixPixelPtr += 4;

								}
							
							}
						
						}
						
						//	Copy the mixed result image back from where we got it.

						SetGWorld(savedPort, savedDevice); 

						CopyBits(&(((GrafPtr)gDestinationMixingWorld)->portBits),
								&((GrafPtr)destinationPort)->portBits,
								&currentMixRect,
								&currentMixRect,
								srcCopy,
								NULL);
										
					}
						
				}
			
			}
			
			else {
			
				CopyMask(&(((GrafPtr)imageGWorld)->portBits),
						&(privateImageData->imageMask),
						&((GrafPtr)destinationPort)->portBits,
						&sourceRectangle,
						&sourceRectangle,
						&destinationRectangle);
			
			}
		
		}
	
	}

	ppMacGraphics->FinishDrawing();

}

void sun_awt_image_ImageRepresentation_disposeImage(struct Hsun_awt_image_ImageRepresentation *imageRepresentationObject)
{
	struct Classsun_awt_image_ImageRepresentation 		*imageRepresentationStruct = unhand(imageRepresentationObject);

	if (imageRepresentationStruct->pData != NULL) {
	
		PrivateMacImageData 		*imageData = (PrivateMacImageData *)(imageRepresentationStruct->pData);
		
		if (imageData->imageAlpha != NULL)
			free(imageData->imageAlpha);
	
		if (imageData->imageMask.baseAddr != NULL)
			free(imageData->imageMask.baseAddr);

		if (imageData->imageGWorld != NULL)
			DisposeGWorld(imageData->imageGWorld);

		free(imageData);

		imageRepresentationStruct->pData = NULL;

	}
	
}
