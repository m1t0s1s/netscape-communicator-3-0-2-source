//	===========================================================================
//	UFontsStyles.h					©1994 Metrowerks Inc. All rights reserved.
//	===========================================================================

#pragma once

#include	<PP_Prefix.h>

class	LAEStream;
class	LAESubDesc;

class	UFontsStyles
{
public:
	static	Int16		GetFontId(const Str255 inFontName);
	static	void		GetFontName(Int16 inFontId, Str255 outFontName);
	static	Boolean		MultiByteScriptPresent(void);
	static	Int32		StylesToInt(const LAESubDesc &inStyles);
	static	void		IntToStyles(Int32 inOnFaces, AEDesc *outStyles, Int32 inOffFaces = 0);
	static	void		WriteStyles(LAEStream &outStream, Int32 inFaces);
	static	Int32		EnumToFace(OSType inEnum);

	const static Point	sUnitScale;

protected:
	static	Boolean		sMultiByteScriptPresent;
};
