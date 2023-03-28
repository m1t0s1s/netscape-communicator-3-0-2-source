// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

#include "Paragraph.h"


//***************************************************************************************************

CParagCtrlChars gParagCtrlChars;

//***************************************************************************************************

long CParagCtrlChars::Define(unsigned char* charsPtr, long startOffset, long maxOffset)
{
	fParagStart = startOffset;
	fCount = 0;
	fIndex = 0;
	
	charsPtr += startOffset;
	
	long temp = maxOffset-startOffset;
	if (temp > kMaxInt) {temp = kMaxInt;} //max parag len = kMaxInt
	register short remainingLen = short(temp);
	
	short totalOffset = 0;
	
	short* offsetPtr = fOffsets;
	
	while (remainingLen) {
		unsigned char theChar;
		short offset = GetCtrlCharOffset(charsPtr+totalOffset, remainingLen, &theChar);
		
		if (offset < 0) {
			totalOffset += remainingLen;
			break;
		}
		
		totalOffset += offset;
		
		*offsetPtr++ = totalOffset++;
		fChars[fCount] = theChar;
		
		if ((++fCount == kMaxParagCtrlChars) || (theChar == kCRCharCode)) {break;}
		
		remainingLen -= (offset+1);
	}
	
	fParagEnd = startOffset+totalOffset;
	
	return fParagEnd;
}
//***************************************************************************************************

long CParagCtrlChars::GetCurrCtrlOffset()
{
	return (fIndex < fCount) ? fParagStart+fOffsets[fIndex] : -1;
}
//***************************************************************************************************

char CParagCtrlChars::GetCurrCtrlChar()
{
	return (fIndex < fCount) ? fChars[fIndex] : 0;
}
//***************************************************************************************************

void CParagCtrlChars::Invalid()
{
	fParagEnd = 0;
	fIndex = 0;
}
//***************************************************************************************************



long GetParagStartOffset(unsigned char* charsPtr, long lenBefore)
{
	if (lenBefore > kMaxInt) {lenBefore = kMaxInt;}
	short offset = SearchCharBack(kCRCharCode, charsPtr, short(lenBefore));
	return (offset > 0) ? offset-1: lenBefore;
}
//***************************************************************************************************

long GetParagEndOffset(unsigned char* charsPtr, long lenAfter)
{
	if (lenAfter > kMaxInt) {lenAfter = kMaxInt;}
	short offset = SearchChar(kCRCharCode, charsPtr, short(lenAfter));
	return (offset >= 0) ? offset+1: lenAfter;
}
//***************************************************************************************************
