//	===========================================================================
//	UFontsStyles.cp					©1994 Metrowerks Inc. All rights reserved.
//	===========================================================================

#ifdef PowerPlant_PCH
#include PowerPlant_PCH
#endif

#include	"UFontsStyles.h"
#include	<UAEGizmos.h>
#include	<LString.h>

/*-
#include	<AETextSuite.h>
#include	<UAppleEventsMgr.h>
#include	<UExtractFromAEDesc.h>
#include	<LString.h>
#include	<LStream.h>
#include	<UDrawingState.h>
#include	<WFixed.h>
#include	<PP_Messages.h>
#include	<UTextMenus.h>
#include	<LCommander.h>
#include	<PP_Resources.h>
#include	<StClasses.h>

#ifndef		__FONTS__
#include	<Fonts.h>
#endif

#ifndef		__AEREGISTRY__
#include	<AERegistry.h>
#endif

#ifndef		__AEOBJECTS__
#include	<AEObjects.h>
#endif

#ifndef		__AEPACKOBJECT__
#include	<AEPackObject.h>
#endif

#ifndef		__AEREGISTRY__
#include	<AERegistry.h>
#endif
*/

#ifndef		__GESTALT__
#include	<Gestalt.h>
#endif

#pragma	warn_unusedarg off

Boolean	UFontsStyles::sMultiByteScriptPresent = -1;
const Point UFontsStyles::sUnitScale = {1, 1};

Boolean	UFontsStyles::MultiByteScriptPresent(void)
{
	if ( (sMultiByteScriptPresent != true) && (sMultiByteScriptPresent != false) ) {
		sMultiByteScriptPresent = false;
		
		do {
			long	response;
			OSErr	err = Gestalt(gestaltScriptMgrVersion, &response);
			if (err)
				break;
			
			if (response < 0x0710)
				break;	//	Actually, a multibyte script may be present
						//	but the CharacterByteType calls used by this
						//	code to deal with it are not present until
						//	system 7.1
		
			long	result = GetScriptManagerVariable(smDoubleByte);
			if (result)
				sMultiByteScriptPresent = true;
		} while (false);
	}

	return sMultiByteScriptPresent;
}

Int16	UFontsStyles::GetFontId(const Str255 inFontName)
{
	Int16	rval;
	
	GetFNum(inFontName, &rval);
	
	if (rval == 0) {
		Str255		fontZeroName;
		GetFontName(0, fontZeroName);
		if (!EqualString(inFontName, fontZeroName, false, false))
			Throw_(fontNotDeclared);
	}
	
	return rval;
}

void	UFontsStyles::GetFontName(Int16 inFontId, Str255 outFontName)
{
	::GetFontName(inFontId, outFontName);
	
	if ((inFontId == 1) && (outFontName[0] == 0)) {
		//	force it
		LString::CopyPStr("\pGeneva", outFontName);
	}
}

void	UFontsStyles::WriteStyles(LAEStream &outStream, Int32 inFaces)
{
	if (inFaces == normal) {
		outStream.WriteEnumDesc(kAEPlain);
	} else {
		if (inFaces & bold)			outStream.WriteEnumDesc(kAEBold);
		if (inFaces & italic)		outStream.WriteEnumDesc(kAEItalic);
		if (inFaces & outline)		outStream.WriteEnumDesc(kAEOutline);
		if (inFaces & shadow)		outStream.WriteEnumDesc(kAEShadow);
		if (inFaces & underline)	outStream.WriteEnumDesc(kAEUnderline);
		if (inFaces & condense)		outStream.WriteEnumDesc(kAECondensed);
		if (inFaces & extend)		outStream.WriteEnumDesc(kAEExpanded);
	}
}

Int32	UFontsStyles::StylesToInt(const LAESubDesc &inStyles)
{
	Int32		rval = normal;
	LAESubDesc	val;
	Int32		i,
				n;
	
	if (!inStyles.IsListOrRecord())
		Throw_(paramErr);
	
	n = inStyles.CountItems();
	
	for (i = 1; i <= n; i++)
		rval |= EnumToFace(inStyles.NthItem(i).ToEnum());
	
	return rval;
}

Int32	UFontsStyles::EnumToFace(OSType inEnum)
{
	switch (inEnum) {
		case kAEBold:			return	bold;			break;
		case kAEItalic:			return	italic;			break;
		case kAEOutline:		return	outline;		break;
		case kAEShadow:			return	shadow;			break;
		case kAEUnderline:		return	underline;		break;
		case kAECondensed:		return	condense;		break;
		case kAEExpanded:		return	extend;			break;
		case kAEPlain:			return	normal;			break;
		default:				Throw_(errAETypeError);	break;
	}
	
	return normal;	//	never executed.
}

void	UFontsStyles::IntToStyles(Int32 inOnFaces, AEDesc *outStyles, Int32 inOffFaces)
{
	LAEStream	aeStream;
	
	if (inOffFaces == 0)
		inOffFaces = ~inOnFaces;
	
	aeStream.OpenRecord(typeTextStyles);
	
	//	On styles
	aeStream.WriteKey(keyAEOnStyles);
	aeStream.OpenList();
	WriteStyles(aeStream, inOnFaces);
	aeStream.CloseList();
	
	//	Off styles
	aeStream.WriteKey(keyAEOffStyles);
	aeStream.OpenList();
	WriteStyles(aeStream, inOffFaces);
	aeStream.CloseList();

	aeStream.CloseRecord();
	
	aeStream.Close(outStyles);
}

