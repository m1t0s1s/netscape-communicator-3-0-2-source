// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

#pragma once


const short kMaxInt = 0x7FFF;
const long kMaxLongInt = 0x7FFFFFFF;

//Direction constants
const char kRL = -1;
const char kLR = 0;
const char kSysDirection = 1;
const char kLineDirection = 2; //returned only from non text styles
//****************************************************

//Direction CaretDirection
const char kRLCaret = -1;
const char kLRCaret = 0;
const char kRunDir = 1;
//****************************************************

//const for sel state and objects hilite
const char kSelOff = 0;
const char kSelOn = 1;
const char kSelInactiv = 2;
//****************************************************

//special characters

const unsigned char kMaxCtrlChar = 0x1F; //¥¥ used in TextensionUtil.a (GetCtrlCharOffset)

const unsigned char kRLArrowCharCode = 0x1C;
const unsigned char kLRArrowCharCode = 0x1D;
const unsigned char kUpArrowCharCode = 0x1E;
const unsigned char kDownArrowCharCode = 0x1F;

const unsigned char kTabCharCode = 9;
const unsigned char kCRCharCode = 0xD;
const unsigned char kRomanSpaceCharCode = 0x20;

const unsigned char kClearCharCode = 0x1B;
const unsigned char kBackSpaceCharCode = 8;

//****************************************************

typedef short TSize;
//const for "sizeInfo" param to ITextension
const TSize kSmallSize = 0;
const TSize kMediumSize = 1;
const TSize kLargeSize = 2;
//****************************************************

struct TOffsetPair {
	long firstOffset;
	long lastOffset;
};
//****************************************************

struct TOffset {
	long  		offset;
	Boolean	 	trailEdge;
	
	TOffset() {}
	
	TOffset(long theOffset, Boolean trail = false) {
		offset = theOffset;
		trailEdge = trail;
	}
	
	inline operator long() {return offset;}
	
	void Set(long theOffset, Boolean trail = false) {
		offset = theOffset;
		trailEdge = trail;
	}

	Boolean operator==(const TOffset& other) const;
	inline Boolean operator!=(const TOffset& other) const
		{return !(*this == other);}
};
//****************************************************

struct TOffsetRange {
	TOffset rangeStart;
	TOffset rangeEnd;
	
	TOffsetRange() {}
	
	TOffsetRange(const TOffset& start, const TOffset& end);

	TOffsetRange(long offsetStart, long offsetEnd, Boolean trailStart = false, Boolean trailEnd = true);
	
	void Set(long offsetStart, long offsetEnd, Boolean trailStart, Boolean trailEnd);
	
	inline long Len() const {return rangeEnd.offset-rangeStart.offset;}
	
	long Start() const {return rangeStart.offset;}
	long End() const {return rangeEnd.offset;}
	Boolean IsEmpty() const {return rangeStart.offset == rangeEnd.offset;}
	
	Boolean operator==(const TOffsetRange& other) const;
	inline Boolean operator!=(const TOffsetRange& other) const
		{return !(*this == other);}
	
	void Offset(long value);
	
	void CheckBounds(); //swap range start, rangeEnd if necessary
};
//****************************************************


struct TLineHeightInfo {
	short	lineHeight;
	short	lineAscent;
};
//***************************************************************************************************


extern char gHasManyScripts;
extern Boolean g2Directions;
extern Boolean	g2Bytes;

extern Boolean gHasColor;

extern Boolean gTextensionImaging;

/*
	this is declared as reference instead of as an automatic "CTempRegions gTempRegions;" since
	the power pc compiler has a problem in the later case (the virtual table of gTempRegions is not 
	initialized). Any way, this reduces the global area!
*/


extern short gDefaultTabVal; //initialized to 30 pixels


extern long gMinAppMemory; //the amount guaranteed after any edit operation
//***************************************************************************************************

struct TLongRect; //later
//***************************************************************************************************

#define LongToFixed(l) ((Fixed)(l) << 16)
#define FixedToLong(f) ((short)((Fixed)(f) >> 16))

Handle AllocateTempHandle(Size countBytes, char lockIt, OSErr* err);

inline long MaxLong(long long1, long long2) {return((long1 > long2) ? long1 : long2);}
inline long MinLong(long long1, long long2) {return((long1 < long2) ? long1 : long2);}

long Clip(long val, long lowerBound, long upperBound);
inline short ClipToInt(long val) {return short(Clip(val, -kMaxInt, kMaxInt));}

void SwapShort(short* short1, short* short2);
void SwapLong(long* long1, long* long2);

inline short TxRectHeight(const Rect& theRect) {return (theRect.bottom - theRect.top);}
inline short TxRectWidth(const Rect& theRect) {return (theRect.right - theRect.left);}

Boolean IsColorPort();

short GetMaxPixDepth(const Rect& aRect);

void SetCursorIBeam();

/*
This routine is here since in script systems Font2Script returns may return a non-roman script
when the font is roman and forceFont flag is on.
*/
short MonoFontToScript(short aFont);
Boolean IsScript2Bytes(short theScript);
inline Boolean IsScriptR2L(short theScript) {return GetScriptVariable(theScript, smScriptRight) != 0;}

inline short ScriptToDefaultFont(short scriptNo)
	{return short(GetScriptVariable(scriptNo, smScriptAppFond));}

void MyKeyScript(short newScript);

long LongRoundUp(long aNumber, short aModulus);
short ShortRoundUp(short aNumber, short aModulus);

short ScaleVal(short val, short numer, short denom, Boolean roundUp = true);
long LongScaleVal(long val, short numer, short denom, Boolean roundUp = true);

Fixed FixScaleVal(Fixed val, short numer, short denom);

#ifndef txtnRulers
Fixed GetDefaultTabWidth(Fixed tabPixStart, short numer = 1, short denom = 1);
#endif

inline Boolean IsCtrlChar(unsigned char theChar) {return theChar <= kMaxCtrlChar;}



//***************************************************************************************************

//¥externals

extern pascal short GetCtrlCharOffset(unsigned char* chars, short len, unsigned char* theChar); //returns the offset of the char, -1 if not found
extern pascal short SearchChar(unsigned char theChar, unsigned char* chars, short len); //returns the offset of the char from "chars", -1 if not found
extern pascal short SearchCharBack(unsigned char theChar, unsigned char* chars, short lenBefore); //same as SearchChar but search backward

extern pascal void AddToArrayElements(long value, char* startPtr, long count, short elemSize);

#ifndef powerc
extern pascal void AddToLongArray(long value, char* startPtr, long count);
#endif

extern "C" {
	extern char CompareBytes(unsigned char* ptr1, unsigned char* ptr2, long count);
	//¥¥only the short part is used from "count". declared as long since the compiler pushes 4 bytes when "extern C"
}

//***************************************************************************************************


struct TLongRect {
//the order top, left, bottom, right is the same as the toolbox and VRect in MacApp, don't change
	long top;
	long left;
	long bottom;
	long right;
	
	//¥¥TLongRect members should not move memory, they can be members of Handle objects
	
	inline TLongRect(long l, long t, long r, long b) {
		left = l;
		top = t;
		right = r;
		bottom = b;
	}
	
	inline TLongRect() {}
	
	inline TLongRect(Rect shortRect) { //implicit conversion from Rect to TLongRect
		left = shortRect.left;
		top = shortRect.top;
		right = shortRect.right;
		bottom = shortRect.bottom;
	}

	inline void Set(long l, long t, long r, long b) {
		left = l;
		top = t;
		right = r;
		bottom = b;
	}
	
	inline long Height() const {return bottom-top;}
	inline long Width() const {return right-left;}

	Boolean Sect(const TLongRect& src, TLongRect* intersection) const;
	
	void Union(const TLongRect& src, TLongRect* theUnion) const;

	Boolean IsPointInside(const SPoint32& thePt) const;
	
	void Inset(long h, long v);
	
	void Offset(long h, long v);
	
	Boolean operator==(const TLongRect& aRect) const;
	inline Boolean operator!=(const TLongRect& aRect) const
		{return !(*this == aRect);}
};
//***************************************************************************************************


inline Boolean IsRectRgn(RgnHandle aRgn) {return (*aRgn)->rgnSize == 10;}

Boolean ClipFurther(Rect* moreClipRect, RgnHandle origClip);
Boolean CalcClipRect(Rect* theRect);

void InvalSectRect(Rect* theRect, RgnHandle theClip = nil);

//***************************************************************************************************

//constants for CDisplayChanges::fKind
const short kNoChanges = 0;

const short kNeedHReformat = 1;
const short kNeedVReformat = 2;
const short kNeedPartialReformat = 4;

const short kRedrawAll = 8;
const short kCheckSize = 16;
const short kCheckUpdate = 32;
const short kScrollSelection = 64;


class CDisplayChanges
{
	public :
		CDisplayChanges();
		
		inline void SetChangesKind(short result) {fKind |= result;}
		inline short GetChangesKind() const {return fKind;}
		
		void SetFormatRange(const TOffsetPair& formatRange);
		void GetFormatRange(TOffsetPair* formatRange) const;
	
	private:
		short fKind;
		
		TOffsetPair fFormatRange;
};


struct TEditInfo {
	inline TEditInfo() {checkUpdateRgn = true;}
	
	Boolean checkUpdateRgn; //input to BeginEdit
	Boolean editSuccess; //input to EndEdit
	
	long oldLinesHeight;

	
//	TDrawEnv savedDrawEnv; //from BeginEdit to EndEdit
	
	char oldSelState;
};


class CStyledText;
class CFormatter;
class CPageGeometry;

struct TDisplayHandlers {
	CStyledText* styledTextHandler;
	CFormatter* formatHandler;
	CPageGeometry* geometry;
	
	#ifdef txtnRulers
	CRulersRanges* rulersRanges;
	#endif
};

//***************************************************************************************************


//const for drawing routines"
typedef unsigned short TDrawFlags;
const TDrawFlags kNoDrawFlags = 0;
const TDrawFlags kEraseBeforeDraw = 1 << 0;
const TDrawFlags kFirstLineOffScreen = 1 << 1; //internal
const TDrawFlags kDrawAllOffScreen = 1 << 2;


#ifdef txtnDebug
#define DebugMessage(message) SignalPStr_(message)
#else
#define DebugMessage(message)
#endif
//***************************************************************************************************

