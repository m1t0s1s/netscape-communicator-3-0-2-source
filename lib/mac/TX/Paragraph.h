// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

#pragma once

#include "TextensionCommon.h"


//***************************************************************************************************

const short kMaxParagCtrlChars = 500; //allocate global data of size kMaxParagCtrlChars*3

class CParagCtrlChars
{
	public :
		CParagCtrlChars() {}
		
		void Invalid();
		
		long Define(unsigned char* charsPtr, long startOffset, long maxOffset);
		
		inline long GetParagEnd() {return fParagEnd;}
		inline long GetParagStart() {return fParagStart;}
		
		long GetCurrCtrlOffset();
		char GetCurrCtrlChar();
		
		inline void Next() {++fIndex;}
		
		inline void Reset() {fIndex = 0;}
		
	private:
		long fParagStart;
		long fParagEnd;
		
		short fCount;
		short fOffsets[kMaxParagCtrlChars];
		char	fChars[kMaxParagCtrlChars];

		short fIndex; //for iteration
};



extern CParagCtrlChars gParagCtrlChars;
//***************************************************************************************************

long GetParagStartOffset(unsigned char* charsPtr, long lenBefore);

long GetParagEndOffset(unsigned char* charsPtr, long lenAfter);


