// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

#include "TextensionCommon.h"
#include "QDTextRun.h"


//***************************************************************************************************


//***************************************************************************************************

CQDTextRun::CQDTextRun()
{
}
//***************************************************************************************************

void CQDTextRun::IQDTextRun()
{
	this->IRunObject();
}
//***************************************************************************************************

CAttrObject* CQDTextRun::CreateNew() const
{
	CQDTextRun* newText = new CQDTextRun;
	if (newText)
		newText->IQDTextRun();
	
	return newText;
}
//***************************************************************************************************


void CQDTextRun::GetAttributes(CObjAttributes* attributes) const
{
	CRunObject::GetAttributes(attributes);
	
	attributes->Add(kFontAttr);
	attributes->Add(kFontSizeAttr);
	attributes->Add(kFaceAttr);
	attributes->Add(kForeColorAttr);
}
//***************************************************************************************************

ClassId CQDTextRun::GetClassId() const
{
	return kQDTextRunClassId;
}
//***************************************************************************************************

Boolean CQDTextRun::IsTextRun() const
{
	return true;
}
//***************************************************************************************************

char CQDTextRun::GetRunDir() const
//Assumed to not disturb memory (see Line.cp)
{
	return fDirection;
}
//***************************************************************************************************

Boolean CQDTextRun::Is2Bytes() const
{
	return f2Bytes;
}
//***************************************************************************************************

char CQDTextRun::GetRunScript() const
{
	return (this->IsAttributeON(kFontAttr)) ? fScript : smUninterp;
}
//***************************************************************************************************

void CQDTextRun::AttributeToBuffer(AttrId theAttr, void* attrBuffer) const //override
{
	switch (theAttr) {
		case kFontAttr:
			*(short*)attrBuffer = fFont;
			break;
			
		case kFontSizeAttr:
			*(short*)attrBuffer = fFontSize;
			break;
			
		case kFaceAttr:
			*(long*)attrBuffer = fFace;
			break;
			
		case kForeColorAttr:
			*(RGBColor*)attrBuffer = fForeColor;
			break;
		
		default:
			CRunObject::AttributeToBuffer(theAttr, attrBuffer);
	}
}
//***************************************************************************************************

void CQDTextRun::BufferToAttribute(AttrId theAttr, const void* attrBuffer)
{
	switch (theAttr) {
		case kFontAttr:
			fFont = *(short*)attrBuffer;
			fScript = MonoFontToScript(*(short*)attrBuffer);
			
			if (g2Directions)
				fDirection = (IsScriptR2L(fScript)) ? kRL : kLR;
			else
				fDirection = kLR;
			
			f2Bytes = (g2Bytes) ? IsScript2Bytes(fScript) : false;
			break;
			
		case kFontSizeAttr:
			fFontSize = *(short*)attrBuffer;
			break;
			
		case kFaceAttr:
			fFace = *(long*)attrBuffer;
			break;

		case kForeColorAttr:
			fForeColor = *(RGBColor*)attrBuffer;
			break;
		
		default:
			CRunObject::BufferToAttribute(theAttr, attrBuffer);
	}
}
//***************************************************************************************************


void CQDTextRun::SetDefaults(long message /*= 0*/)
{
	CRunObject::SetDefaults(message);
	
	short defFont, defSize;
	long fond_size = GetScriptVariable(short(message), smScriptPrefFondSize);
	defFont = short(fond_size >> 16); //hi word
	defSize = short(fond_size); //lo word
	
	this->SetAttributeValue(kFontAttr, &defFont);
	this->SetAttributeValue(kFontSizeAttr, &defSize);  

	if ((message & kUpdateOnlyScriptInfo) == 0) {
		long defFace = 0;
		this->SetAttributeValue(kFaceAttr, &defFace);  
	
		RGBColor foreColor;
		foreColor.red = 0;
		foreColor.green = 0;
		foreColor.blue = 0;
		this->SetAttributeValue(kForeColorAttr, &foreColor);
	}
}
//***************************************************************************************************

void CQDTextRun::AddFace(long faceToAdd, long* destFace)
/*normally the result is (faceToAdd | destFace), but this methode is the place to check for exclusif faces
(eg superScript, subScript)
*/
{
	*destFace |= faceToAdd;

	if ((*destFace & extend) && (*destFace & condense))
		this->RemoveFace((faceToAdd & extend) ? condense : extend, destFace);
}
//***************************************************************************************************

void CQDTextRun::RemoveFace(long faceToRemove, long *destFace) const
{
	*destFace &= (~faceToRemove);
}
//***************************************************************************************************

void CQDTextRun::UpdateAttribute(AttrId theAttr, const void* srcAttr, long updateMessage
															, const void* continuousAttr /*= nil*/)
/*
if the scripts are not the same, the font and script are copied from srcObj only if
forceScriptModif flag is true in srcObj.
*/
{
	Boolean pass = true;
	
	switch (theAttr) {
		case kFontAttr:
			if ((!(updateMessage & kForceScriptModif))
				&& (fScript != MonoFontToScript(*(short*)srcAttr)))
				pass = false;
			break;
			
		case kFontSizeAttr:
			if (updateMessage & kAddSize) {
				short newSize = fFontSize + *(short*)srcAttr;
				if (newSize < 1)
					newSize = 1;
					
				this->SetAttributeValue(kFontSizeAttr, &newSize);
				
				pass = false;
			}
			break;
		
		case kFaceAttr :
			long newFace = fFace;
			
			long srcFace = *(long*)srcAttr;
			
			if (updateMessage & kAddFace) {
				this->AddFace(srcFace, &newFace);
				pass = false;
			}
			else {
				if (updateMessage & kRemoveFace) {
					this->RemoveFace(srcFace, &newFace);
					pass = false;
				}
				else {
					if ((updateMessage & kToggleFace) && (srcFace) && (fFace)) {
						if (continuousAttr && (*(long*)continuousAttr & srcFace))
							this->RemoveFace(srcFace, &newFace);
						else
							this->AddFace(srcFace, &newFace);
							
						pass = false;
					}
				}
			}
							
			if (!pass)
				this->SetAttributeValue(kFaceAttr, &newFace);
			break;
		
		//default :pass
	}
	
	if (pass)
		CRunObject::UpdateAttribute(theAttr, srcAttr, updateMessage, continuousAttr);
}
//***************************************************************************************************

Boolean EqualRGB(const RGBColor& rgb1, const RGBColor& rgb2);
Boolean EqualRGB(const RGBColor& rgb1, const RGBColor& rgb2)
{
	return (rgb1.red == rgb2.red) && (rgb1.green == rgb2.green) && (rgb1.blue == rgb2.blue);
}
//***************************************************************************************************

Boolean CQDTextRun::EqualAttribute(AttrId theAttr, const void* valToCheck) const
{
	switch (theAttr) {
		case kFontAttr:
			return (fFont == *(short*)valToCheck);
			
		case kFontSizeAttr:
			return (fFontSize == *(short*)valToCheck);
			
		case kFaceAttr:
			return (fFace == *(long*)valToCheck);
			
		case kForeColorAttr:
			return EqualRGB(fForeColor, *(RGBColor*)valToCheck);
			
		default:
			return CRunObject::EqualAttribute(theAttr, valToCheck);
	}
}
//************************************************************************************************

Boolean CQDTextRun::SetCommonAttribute(AttrId theAttr, const void* valToCheck)
{
	if (theAttr == kFaceAttr) {
		long faceToCheck = *(long*)valToCheck;
		
		if ((fFace == 0) && (faceToCheck == 0))
			{return true;}
		
		long theCommonFace = fFace & faceToCheck;
		if (theCommonFace) {
			fFace = theCommonFace;
			return true;
		}
		else
			return false;
	}
	else
		return CRunObject::SetCommonAttribute(theAttr, valToCheck);
}
//***************************************************************************************************

long CQDTextRun::GetAttributeFlags(AttrId theAttr) const
{
	long flags = 0;
	
	if ((theAttr == kFontAttr) || (theAttr == kFontSizeAttr) || (theAttr == kFaceAttr))
		flags = kReformat | kVReformat;
	
	return flags | CRunObject::GetAttributeFlags(theAttr);
}
//*************************************************************************************************

void CQDTextRun::CalcHeightInfo(short* ascent, short* descent, short* leading)
//assume the obj env is saved
{
	Assert_(qd.thePort->txFont == fFont);
	Assert_(qd.thePort->txSize == fFontSize);
	Assert_(qd.thePort->txFace == Style(fFace));
	
	Assert_(this->IsAttributeON(kFontAttr) && this->IsAttributeON(kFontSizeAttr) && this->IsAttributeON(kFaceAttr));
	
	FontInfo	theFontInfo;
	GetFontInfo(&theFontInfo);

	*ascent = theFontInfo.ascent;
	*descent = theFontInfo.descent;
	*leading = theFontInfo.leading;
}
//***************************************************************************************************

void CQDTextRun::SetKeyScript() const
{
	if (gHasManyScripts && this->IsAttributeON(kFontAttr))
		MyKeyScript(fScript);
}
//***************************************************************************************************

void CQDTextRun::SetRunEnv() const
{
	CRunObject::SetRunEnv();

	TextFont(fFont);
	TextSize(fFontSize);
	TextFace(Style(fFace));

	if (IsColorPort()) {
		RGBColor temp = fForeColor;
		RGBForeColor(&temp);
	}
	
	::SetSysDirection(fDirection);
	/*needed in PixelToChar, CharToPixel, Draw, LineBreak, made here for descendents to
	be able to override those members without worring about sysjust. See comments in those members
	for more informations on why sysjust is needed to be set.
	We will not save the direction in "SaveRunEnv", we assume that this is done at higher level
	(CSTyledText::SetTextEnv and RestoreTextEnv)
	*/
}
//***************************************************************************************************

void CQDTextRun::SaveRunEnv()
{
	CRunObject::SaveRunEnv();
	
	GrafPtr currPort = qd.thePort;
	fFont = currPort->txFont;
	fFontSize = currPort->txSize;
	fFace = currPort->txFace;

	if (IsColorPort())
		{GetForeColor(&fForeColor);}
}
//***************************************************************************************************

Boolean CQDTextRun::FilterControlChar(unsigned char theChar) const
{
	Assert_(IsCtrlChar(theChar));
	
	if ((theChar == kBackSpaceCharCode) || (theChar == kClearCharCode) || (theChar == kCRCharCode)
		|| (theChar == kTabCharCode)
		|| ((theChar >= kRLArrowCharCode) && (theChar <= kDownArrowCharCode))) {return false;}
		
	return true;
}
//***************************************************************************************************

void CQDTextRun::PixelToChar(const LineRunDisplayInfo& displayInfo, Fixed pixel, TOffsetRange* charRange)
/*assume SetSysJust is called with the run dir (SetRunEnv).
PixelToChar depends on the SysJust (!) in a strange way : For Instance if passed "An English text"
as text with pix = 0 ==>if SysJust == -1 PixelToChar returns the offset to the end of text??
Similarly if passed "cibarA" which is in arabic with pix == 0 and sysJust == 0 ==> returns 0
*/
{
	if (pixel <= 0)
		pixel = LongToFixed(1); //work around bug ais SSNPixel2Char
	
	short result;
	Boolean leadingEdge;
	
	Point numer = displayInfo.zoomNumer;
	Point denom = displayInfo.zoomDenom;
	this->AdjustScaleVals(&numer, &denom);
	
	Fixed widthRemain;
	result = ::PixelToChar(Ptr(displayInfo.runChars), displayInfo.runLen, displayInfo.runExtraWidth, pixel
										, &leadingEdge, &widthRemain, smOnlyStyleRun, numer, denom);
										
	Boolean trailEdge;
	if (!result)
		trailEdge = false;
	else {
		if (result == displayInfo.runLen)
			trailEdge = true;
		else
			trailEdge = !leadingEdge;
	}
	
	charRange->Set(result, result, trailEdge, trailEdge);
}
//***************************************************************************************************

Fixed CQDTextRun::CharToPixel(const LineRunDisplayInfo& displayInfo, short charOffset)
{
	short result;
	
	Point numer = displayInfo.zoomNumer;
	Point denom = displayInfo.zoomDenom;
	this->AdjustScaleVals(&numer, &denom);
	
	result = ::CharToPixel(Ptr(displayInfo.runChars), displayInfo.runLen, displayInfo.runExtraWidth, charOffset, fDirection
									, smOnlyStyleRun, numer, denom);
									
	return LongToFixed(result);
}
//***************************************************************************************************

void CQDTextRun::Draw(const LineRunDisplayInfo& displayInfo, Fixed /*runLeftEdge*/
														, const Rect& /*lineRect*/, short /*lineAscent*/)
{
	Point numer = displayInfo.zoomNumer;
	Point denom = displayInfo.zoomDenom;
	this->AdjustScaleVals(&numer, &denom);
	
	::DrawJustified(Ptr(displayInfo.runChars), displayInfo.runLen, displayInfo.runExtraWidth
						, smOnlyStyleRun, numer, denom);
}
//***************************************************************************************************

#ifdef txtnRulers
Fixed CQDTextRun::FullJustifPortion(const LineRunDisplayInfo& displayInfo)
{
	Point numer = displayInfo.zoomNumer;
	Point denom = displayInfo.zoomDenom;
	this->AdjustScaleVals(&numer, &denom);
	
	return NPortionText(Ptr(displayInfo.runChars), displayInfo.runLen
										, smOnlyStyleRun, numer, denom);
}
//***************************************************************************************************
#endif

#ifdef powerc
#pragma options align=mac68k


struct QuickDrawGlobals {
	short						QDSpareD;
	short						QDSpareC;
	short						QDSpareB;
	short						QDSpareA;
	short						QDSpare9;
	short						QDSpare8;
	Fixed						qdChExtra;
	Fixed						qdRunSlop;
	short						pnLocFixed;
	long						playIndex;
	FMOutput					*fontPtr;
	Fixed						fontAdj;
	Point						patAlign;
	short						polyMax;
	PolyHandle					thePoly;
	short						QDSpare0;
	PicHandle					playPic;
	unsigned short		rgnMax;
	unsigned short		rgnIndex;
	short						**rgnBuf;
	unsigned short		wideRgnSize;
	Rect						wideRgnBBox;
	RgnPtr						wideMaster;
	RgnHandle					wideOpen;
	long						randSeed;
	BitMap						screenBits;
	Cursor						arrow;
	Pattern						dkGray;
	Pattern						ltGray;
	Pattern						gray;
	Pattern						black;
	Pattern						white;
	GrafPtr						thePort;
};
typedef struct QuickDrawGlobals QuickDrawGlobals;
#pragma options align=reset



#define GetStdTxMeasResult() (((QuickDrawGlobals*)&qd)->fontAdj)
#else
Fixed GetStdTxMeasResult() = {0x2055, 0x90FC, 0x00AC, 0x2010};
// MOVEA.L (A5),A0  --  SUBA.W  #$00AC,A0  --   MOVE.L (A0),D0
#endif
//***************************************************************************************************

Fixed StdTxFixedMeas(short byteCount,const void *textAddr, Point numer, Point denom)
{
	FontInfo notUsedInfo;
	StdTxMeas(byteCount, textAddr, &numer, &denom, &notUsedInfo);

	return FixScaleVal((Fixed)GetStdTxMeasResult(), numer.h, denom.h);
}
//***************************************************************************************************

Fixed CQDTextRun::MeasureWidth(const LineRunDisplayInfo& runDisplayInfo)
{
	Point numer = runDisplayInfo.zoomNumer;
	Point denom = runDisplayInfo.zoomDenom;
	this->AdjustScaleVals(&numer, &denom);
	
	return StdTxFixedMeas(runDisplayInfo.runLen, runDisplayInfo.runChars
											, numer, denom);
}
//***************************************************************************************************

#ifndef powerc
extern pascal StyledLineBreakCode NTextensionStyledLineBreak(unsigned char* textPtr
																						, long textLen, long textStart
																						, long textEnd, long flags
																						, Fixed* textWidth, long* textOffset
																						, Point numer, Point denom);
#endif
//***************************************************************************************************


StyledLineBreakCode CQDTextRun::LineBreak(unsigned char* scriptRunChars, long scriptRunLen, long runOffset
																				, Fixed* widthAvail, Boolean forceBreak, long* breakLen)
/*
StyledLineBreak(Ptr textPtr,long textLen,long textStart,
								long textEnd,long flags,Fixed *textWidth,long *textOffset)
¥	on input if the parameter "textOffset" != 0 ==> "textPtr" points to the start of the line to
	be broken or to the start of a script run, if 0 ==> "textPtr" points to a char inside this line.
	in the former case the result may be smCharBreak if the line contains only 1 word, in the 2nd case
	the result is never smCharBreak and in the case of 1 word textOffset will be = textStart with result
	smBreakWord.
	===> textOffset should be 0 <==> textPtr points to a script boundary.

StyledLineBreak include the extra spaces (spaces that expand out of the width of the line)
only if the dir of the spaces is the same as the SysJust i.e remove A0 if sysjust -1 and $20 if
sysJust 0. for this reason SetSysJust with the dir of the run to break is done in "BreakLine" and then
formatting is independent of the direction of the text and of SysJust, and we don't need to reformat
when Direction or SysJust change (SetSysJust is called in SetRunEnv).
*/
{
	StyledLineBreakCode result;

	long breakOffset = (forceBreak) ? 1 : 0;
	
#ifdef powerc
	
	result = StyledLineBreak((Ptr)scriptRunChars, scriptRunLen
												,	runOffset, scriptRunLen
												, 0/*flags*/, widthAvail, &breakOffset);
#else
	
	Point numer, denom;
	numer.h = 1;
	numer.v = numer.h;
	denom = numer;
	
	this->AdjustScaleVals(&numer, &denom);
	
	result = NTextensionStyledLineBreak(scriptRunChars, scriptRunLen
									,	runOffset, scriptRunLen
									, 0/*flags*/, widthAvail, &breakOffset
									, numer, denom);
#endif //powerc
	
	*breakLen = breakOffset - runOffset;

	return result;
}
//***************************************************************************************************

void CQDTextRun::SetCursor(Point, const RunPositionInfo&) const
{
	SetCursorIBeam();

/*
	if (fFace & italic) {
		short italicCursor[] = {0x2A4A, 0x2444, 0x584F, 0x6100, 0xF0DE, 0x7000, 0x7200, 0x101D
														, 0x4EF4, 0x0521, 0x0018, 0x0640, 0x0100, 0x0640, 0x0100, 0x41FA
														, 0x0008, 0x42A7, 0x6000, 0xEE22, 0x2A4A, 0x2444, 0x251F, 0x60D6
														, 0x0014, 0x2D4A, 0x0006, 0x0016, 0x0046, 0x006A, 0x009A, 0x0136
														, 0x01DC, 0x06D8};
		::SetCursor((Cursor*)&italicCursor[0]);
	}
	else {
		SetCursorIBeam();
	}
*/
}
//***************************************************************************************************



