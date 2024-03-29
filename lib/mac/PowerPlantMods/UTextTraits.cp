// ===========================================================================
//	UTextTraits.cp					�1993 Metrowerks Inc. All rights reserved.
// ===========================================================================
//
//	Functions for using TextTraits. A TextTraits Record stores the following
//	text characteristics:
//		Font Name
//		Text Size
//		Text Style
//		Text Drawing Mode
//		Text Color
//	The Font Number is determined from the Font Name at runtime.
//
//	This module supports two ways of using TextTraits Records:
//		(1) in memory
//		(2) from a 'Txtr' resource
//
//	Putting font information in a resource makes programs easier to
//	localize for other languages. Also, you should store fonts by name
//	rather than by number, since font numbers are not constant across
//	different Systems. The exceptions are font number zero (sysFont)
//	and font number one (appFont), which are always the System font
//	and Application font. However, these are different for different
//	Script systems. For the English language Script, the System font
//	is Chicago and the Application font is Geneva.
//
//	When creating a 'Txtr' resource, specify -1 for the font number, which
//	will force a lookup by font name at runtime. Similarly, for a
//	TextTraits Record in memory, set the font number to -1 (use the
//	constant UTextTraits::fontNumber_Unknown) and set the font name
//	string (which you'll commonly get from a Font menu item).

#ifdef PowerPlant_PCH
#include PowerPlant_PCH
#endif

#include <UTextTraits.h>
#include <UEnvironment.h>
#include <UMemoryMgr.h>
#include <PP_Resources.h>

#ifndef __FONTS__
#include <Fonts.h>
#endif
	

// ---------------------------------------------------------------------------
//		� LoadSystemTraits
// ---------------------------------------------------------------------------
//	Fill in a TextTraits Record with the System Font default traits

void
UTextTraits::LoadSystemTraits(
	TextTraitsRecord	&outTextTraits)
{
	outTextTraits.size = 0;
	outTextTraits.style = 0;
	outTextTraits.justification = teFlushDefault;
	outTextTraits.mode = srcOr;
	outTextTraits.color.red = 0;
	outTextTraits.color.green = 0;
	outTextTraits.color.blue = 0;
	outTextTraits.fontNumber = systemFont;
	outTextTraits.fontName[0] = 0;
}
	

// ---------------------------------------------------------------------------
//		� LoadTextTraits
// ---------------------------------------------------------------------------
//	Load Font Number into a TextTraits record, if necessary
//
//	A TextTraits record should initially specify a font name and set
//	the font number to UTextTraits::fontNumber_Unknown.
//
//	When this function is called with a TextTraits record that has a
//	font number of fontNumber_Unknown, it looks up and stores the font
//	number for the TextTrait record's font name. Subsequent calls to this
//	function will then do nothing (since the font number will be valid).

void
UTextTraits::LoadTextTraits(
	TextTraitsPtr	ioTextTraits)
{
	SignalIf_(ioTextTraits == nil);
	
	if (ioTextTraits->fontNumber == fontNumber_Unknown) {
		::GetFNum(ioTextTraits->fontName, &ioTextTraits->fontNumber);
	}
}


// ---------------------------------------------------------------------------
//		� SetPortTextTraits
// ---------------------------------------------------------------------------
//	Set the text characteristics of the current GrafPort. A GrafPort does
//	not have a text justification field, so this function returns the
//	justification value in case you need it.
//
//	If inTextTraits is nil, the standard System font traits are used.
//
//	For example, the Toolbox trap TextBox takes a justification parameter,
//	so you can use this function as follows:
//		TextBox(myTextPtr, myTextLen, &myRect,
//					UTextTraits::SetPortTextTraits(myTextTraitsPtr);

Int16
UTextTraits::SetPortTextTraits(
	const TextTraitsPtr	inTextTraits)
{
	Int16	justification = teFlushDefault;
	RGBColor	rgbWhite = {65535, 65535, 65535};
	
	if (inTextTraits == nil) {		// Use standard System font traits
		::TextFont(systemFont);
		::TextSize(0);
		::TextFace(0);
		::TextMode(srcOr);
		
		if (UEnvironment::HasFeature(env_SupportsColor)) {
			RGBColor	rgbBlack = {0, 0, 0};
			::RGBForeColor(&rgbBlack);
			::RGBBackColor(&rgbWhite);
		}
	
	} else {						// Set characteristics from TextTraits
								
			// Cast away "const" here. Logically, the TextTraits Record
			// is not changed--the font number is cached so that future
			// references will be faster.
			
		LoadTextTraits((TextTraitsPtr) inTextTraits);

		::TextFont(inTextTraits->fontNumber);
		::TextSize(inTextTraits->size);
		::TextFace(inTextTraits->style);
		::TextMode(inTextTraits->mode);
		justification = inTextTraits->justification;
		
		if (UEnvironment::HasFeature(env_SupportsColor)) {
			::RGBForeColor(&inTextTraits->color);
			::RGBBackColor(&rgbWhite);
		}
	}
	
	return justification;
}


// ---------------------------------------------------------------------------
//		� SetTETextTraits
// ---------------------------------------------------------------------------
//	Set the text characteristics of a mono-style TextEdit Record (created
//	by TENew)
//
//	If inTextTraits is nil, the standard System font traits are used.
//
//	A TextEdit Record does not store the text color (it uses the
//	foreground color of the GrafPort). Pass the address of an RGBColor
//	record for the ioColor parameter if you want the color information.
//	Pass nil (the default value) if you don't want the color information.

void
UTextTraits::SetTETextTraits(
	const TextTraitsPtr	inTextTraits,
	TEHandle			inMacTEH,
	RGBColor			*ioColor)
{
	TEPtr	macTEPtr = *inMacTEH;
									// Make sure it's an old-style TE Record
									//   txSize is -1 for new-style
	if (macTEPtr->txSize < 0) {
		return;
	}

	if (inTextTraits == nil) {		// Use default System font traits
		macTEPtr->txFont = systemFont,
		macTEPtr->txSize = 0;
		macTEPtr->txFace = 0;
		macTEPtr->txMode = srcOr;
		::TESetAlignment(teFlushDefault, inMacTEH);
		
		if (ioColor != nil) {		// Black is all zeroes for RGB values
			ioColor->red = 0;
			ioColor->green = 0;
			ioColor->blue = 0;
		}
		
	} else {						// Set characteristics from TextTraits
								
			// Cast away "const" here. Logically, the TextTraits Record
			// is not changed--the font number is cached so that future
			// references will be faster.
			
		LoadTextTraits((TextTraitsPtr) inTextTraits);
		
		macTEPtr = *inMacTEH;		// TEHandle could have moved
									// Pass back color information
		if (ioColor != nil) {
			*ioColor = inTextTraits->color;
		}
		
									// Jam text traits into TE Record
		macTEPtr->txFont = inTextTraits->fontNumber,
		macTEPtr->txSize = inTextTraits->size;
		macTEPtr->txFace = inTextTraits->style;
		macTEPtr->txMode = inTextTraits->mode;
		::TESetAlignment(inTextTraits->justification, inMacTEH);
	}
	
									// Set line spacing based on Font
	FMInput	fontSpecs;
	macTEPtr = *inMacTEH;
	fontSpecs.family	= macTEPtr->txFont;
	fontSpecs.size		= macTEPtr->txSize;
	fontSpecs.face		= macTEPtr->txFace;
	fontSpecs.needBits	= false;
	fontSpecs.device	= 0;
	fontSpecs.numer.h	= 1;
	fontSpecs.numer.v	= 1;
	fontSpecs.denom.h	= 1;
	fontSpecs.denom.v	= 1;
	
	FMOutPtr	info = ::FMSwapFont(&fontSpecs);
	
	(**inMacTEH).lineHeight = info->ascent + info->descent + info->leading;
	(**inMacTEH).fontAscent = info->ascent;
	
	::TECalText(inMacTEH);			// Reflow text
}
	

// ---------------------------------------------------------------------------
//		� LoadTextTraits
// ---------------------------------------------------------------------------
//	Return a handle to a TextTraits Resource

TextTraitsH
UTextTraits::LoadTextTraits(
	ResIDT	inTextTraitsID)
{
	TextTraitsH	traitsH = (TextTraitsH) ::GetResource('Txtr', inTextTraitsID);
	
	if (traitsH != nil) {
		StHandleLocker	lock((Handle)traitsH);
		LoadTextTraits(*traitsH);
	} 
	
	return traitsH;
}


// ---------------------------------------------------------------------------
//		� SetPortTextTraits
// ---------------------------------------------------------------------------
//	Set the text characteristics of the current GrafPort. A GrafPort does
//	not have a text justification field, so this function returns the
//	justification value in case you need it.
//
//	For example, the Toolbox trap TextBox takes a justification parameter,
//	so you can use this function as follows:
//		TextBox(myTextPtr, myTextLen, &myRect,
//					UTextTraits::SetPortTextTraits(myTextTraitsID);

Int16
UTextTraits::SetPortTextTraits(
	ResIDT	inTextTraitsID)
{
	TextTraitsPtr	traitsPtr = nil;
	
	if (inTextTraitsID != Txtr_SystemFont) {
		TextTraitsH	traitsH = LoadTextTraits(inTextTraitsID);
		
		if (traitsH != nil) {
			StHandleLocker	lock((Handle)traitsH);
			return SetPortTextTraits(*traitsH);
		}
	}
	
	return SetPortTextTraits(traitsPtr);
}


// ---------------------------------------------------------------------------
//		� SetTETextTraits
// ---------------------------------------------------------------------------
//	Set the text characteristics of a mono-style TextEdit Record (created
//	by TENew)
//
//	A TextEdit Record does not store the text color (it uses the
//	foreground color of the GrafPort). Pass the address of an RGBColor
//	record for the ioColor parameter if you want the color information.
//	Pass nil (the default value) if you don't want the color information.
//
//	This routine will change the text characteristics of the current
//	port to the specified text traits.

void
UTextTraits::SetTETextTraits(
	ResIDT		inTextTraitsID,
	TEHandle	inMacTEH,
	RGBColor	*ioColor)
{
	TextTraitsPtr	traitsPtr = nil;
	
	if (inTextTraitsID != Txtr_SystemFont) {
		TextTraitsH	traitsH = LoadTextTraits(inTextTraitsID);
		
		if (traitsH != nil) {
			StHandleLocker	lock((Handle)traitsH);
			SetTETextTraits(*traitsH, inMacTEH, ioColor);
			return;
		}
	}
	
	SetTETextTraits(traitsPtr, inMacTEH, ioColor);
}
