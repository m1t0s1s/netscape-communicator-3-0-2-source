// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

#include "RunObject.h"
#include "Array.h"
#include "TextensionCommon.h"



//***************************************************************************************************


void IndivisiblePixelToChar(const LineRunDisplayInfo& displayInfo, Fixed pixel, TOffsetRange* charRange)
{
	short totalWidth = FixRound(displayInfo.runWidth + displayInfo.runExtraWidth);
	
	short clickMarg = 0;
	if (IsCtrlChar(*displayInfo.runChars))
		clickMarg = totalWidth >> 1;	//	jah -- LTXTextEngine doesn't want a "clickMarg"
//		clickMarg = totalWidth >> 2;
		
	if (!clickMarg)
		clickMarg = (totalWidth <= 4) ? 0 : 2;
	
	
	long pixOffset = FixedToLong(pixel);
	
	register long offset;
	
	if (pixOffset <= clickMarg)
		offset = 0;
	else {
		if (pixOffset >= totalWidth - clickMarg)
			offset = displayInfo.runLen;
		else {
			charRange->Set(0, displayInfo.runLen, false, true);
			return;
		}
	}
	
	if (displayInfo.lineDirection == kRL)
		offset ^= displayInfo.runLen;
	
	Boolean trailEdge = (offset != 0);
		
	charRange->Set(offset, offset, trailEdge, trailEdge);
}
//***************************************************************************************************

Fixed IndivisibleCharToPixel(const LineRunDisplayInfo& displayInfo, short charOffset)
{
	register Fixed width = displayInfo.runWidth + displayInfo.runExtraWidth;
	register Boolean lr = displayInfo.lineDirection == kLR;
	
	if (charOffset)
		return (lr) ? width : 0;
	else
		return (lr) ? 0 : width;
}
//***************************************************************************************************

CRunObject::CRunObject()
{
}
//***************************************************************************************************

void CRunObject::IRunObject()
{
	this->IAttrObject();
	
	this->InvalidateHeight();
}
//***************************************************************************************************

void CRunObject::GetAttributes(CObjAttributes* attributes) const
{
	CAttrObject::GetAttributes(attributes);
	
	attributes->Add(kHScaleAttr);
	attributes->Add(kVScaleAttr);
}
//***************************************************************************************************

long CRunObject::GetAttributeFlags(AttrId theAttr) const
{
	long flags;
	
	if (theAttr == kHScaleAttr)
		flags = kReformat+kVReformat; //scaling font in h may change height
	else {
		if (theAttr == kVScaleAttr)
			flags = kVReformat;
		else
			flags = 0;
	}
	
	return flags | CAttrObject::GetAttributeFlags(theAttr);
}
//*************************************************************************************************

void CRunObject::SetRunEnv() const
{
}
//***************************************************************************************************

void CRunObject::SaveRunEnv()
{
}
//***************************************************************************************************

char CRunObject::GetRunDir() const
{
	return kLineDirection;
}
//***************************************************************************************************

Boolean CRunObject::Is2Bytes() const
{
	return false;
}
//***************************************************************************************************

char CRunObject::GetRunScript() const
{
	return smUninterp;
}
//***************************************************************************************************

void CRunObject::SetKeyScript() const
{
}
//***************************************************************************************************
	

void CRunObject::Assign(const CAttrObject* srcObj, Boolean duplicate/*=true*/)
{
	CAttrObject::Assign(srcObj, duplicate);
	
	fAscent = ((CRunObject*)srcObj)->fAscent;
	fDescent = ((CRunObject*)srcObj)->fDescent;
	fLeading = ((CRunObject*)srcObj)->fLeading;
}
//***************************************************************************************************


void CRunObject::Invalid()
{
	CAttrObject::Invalid();
	this->InvalidateHeight();
}
//************************************************************************************************

void CRunObject::AttributeToBuffer(AttrId theAttr, void* attrBuffer) const //override
{
	switch (theAttr) {
		case kHScaleAttr:
			*(short*)attrBuffer = fHScale;
			break;
			
		case kVScaleAttr:
			*(short*)attrBuffer = fVScale;
			break;
		
		#ifdef txtnDebug
		default: SignalPStr_("\p Bad Attr!!");
		#endif
	}
}
//***************************************************************************************************

void CRunObject::BufferToAttribute(AttrId theAttr, const void* attrBuffer) //override
{
	switch (theAttr) {
		case kHScaleAttr:
			fHScale = *(short*)attrBuffer;
			break;
			
		case kVScaleAttr:
			fVScale = *(short*)attrBuffer;
			break;
		
		#ifdef txtnDebug
		default: {SignalPStr_("\p Bad Attr!!");}
		#endif
	}
}
//***************************************************************************************************


void CRunObject::SetAttributeValue(AttrId theAttr, const void* attrBuffer)
{
	CAttrObject::SetAttributeValue(theAttr, attrBuffer);
	
	if ((fAscent != kInvalidAscent) && (this->GetAttributeFlags(theAttr) & kVReformat))
		this->InvalidateHeight();
}
//***************************************************************************************************

void CRunObject::SetDefaults(long message /*=0*/)
{
	CAttrObject::SetDefaults(message);
	
	if ((message & kUpdateOnlyScriptInfo) == 0) {
		short temp = kScaleRef;
		this->SetAttributeValue(kHScaleAttr, &temp);
		this->SetAttributeValue(kVScaleAttr, &temp);
	}
}
//***************************************************************************************************

Boolean CRunObject::EqualAttribute(AttrId theAttr, const void* valToCheck) const
{
	switch (theAttr) {
		case kHScaleAttr:
			return (fHScale == *(short*)valToCheck);
			
		case kVScaleAttr:
			return (fVScale == *(short*)valToCheck);
			
		default:
			return CAttrObject::EqualAttribute(theAttr, valToCheck);
	}
}
//************************************************************************************************

void MulRatio(short ratio1Numer, short ratio1Denom, short* ratio2Numer, short* ratio2Denom);
void MulRatio(short ratio1Numer, short ratio1Denom, short* ratio2Numer, short* ratio2Denom)
{
	if (ratio1Numer != ratio1Denom) {
		if (*ratio2Numer == *ratio2Denom) {
			*ratio2Numer = ratio1Numer;
			*ratio2Denom = ratio1Denom;
		}
		else {
			Fixed resultRatio = FixMul(FixDiv(ratio1Numer, ratio1Denom), FixDiv(*ratio2Numer, *ratio2Denom));
			*ratio2Numer = FixRound(FixMul(resultRatio, LongToFixed(100)));
			*ratio2Denom = 100;
		}
	}
}
//************************************************************************************************

void CRunObject::AdjustScaleVals(Point* numer, Point* denom) const
{
	if ((fHScale != kScaleRef) && this->IsAttributeON(kHScaleAttr))
		MulRatio(fHScale, kScaleRef, &numer->h, &denom->h);
	
	if ((fVScale != kScaleRef) && this->IsAttributeON(kVScaleAttr))
		MulRatio(fVScale, kScaleRef, &numer->v, &denom->v);
}
//***************************************************************************************************

void CRunObject::GetHeightInfo(short* ascent, short* descent, short* leading)
//assume the obj env is saved
{
	if (fAscent == kInvalidAscent) {
		this->CalcHeightInfo(ascent, descent, leading);
		
		Point numer, denom;
		numer.h = 1; numer.v = 1; denom.h = 1; denom.v = 1;
		this->AdjustScaleVals(&numer, &denom);
	
		fAscent = ScaleVal(*ascent, numer.v, denom.v);
		fDescent = ScaleVal(*descent, numer.v, denom.v);
		fLeading = ScaleVal(*leading, numer.v, denom.v);
	}
	
	*ascent = fAscent;
	*descent = fDescent;
	*leading = fLeading;
}
//***************************************************************************************************

#ifdef txtnRulers
Fixed CRunObject::FullJustifPortion(const LineRunDisplayInfo&)
{
	return 0;
}
//***************************************************************************************************
#endif

void CRunObject::Click(const RunPositionInfo&, short /*countClicks*/, Point /*clickPoint*/, long /*modifiers*/
											, ClickCommandInfo*)
{
}
//***************************************************************************************************
	
Boolean CRunObject::FilterControlChar(unsigned char /*theChar*/) const
{
	return false; //true result ==> skip the char
}
//***************************************************************************************************

void CRunObject::Idle(const RunPositionInfo&)
{
} 
//***************************************************************************************************

unsigned long CRunObject::GetIdleTime() const
{
	return 0xFFFFFFFF;
}
//***************************************************************************************************

void CRunObject::SetHilite(char /*oldHilite*/, char /*newHilite*/, const RunPositionInfo&, Boolean /*doDraw = true*/)
{
}
//***************************************************************************************************

void CRunObject::DrawSelection(char /*hiliteStat*/, const RunPositionInfo&)
{
}
//***************************************************************************************************

Boolean CRunObject::IsPointInside(Point, const RunPositionInfo&)
{
	return false;
}
//***************************************************************************************************

void CRunObject::SetCursor(Point, const RunPositionInfo&) const
{
}
//***************************************************************************************************


ResType CRunObject::GetPublicType() const
{
	return 0;
}
//***************************************************************************************************


OSErr CRunObject::ImportPublicData(Handle /*data*/)
{
	return noErr;
}
//***************************************************************************************************

OSErr CRunObject::ExportPublicData(Handle* /*data*/)
{
	return noErr;
}
//***************************************************************************************************



