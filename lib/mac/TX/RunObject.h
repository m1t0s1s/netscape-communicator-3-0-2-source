// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

#pragma once

#include "Array.h"
#include "AttrObject.h"


//***************************************************************************************************

const short	kInvalidAscent = -1;


//CRunObject has 2 attrs
const AttrId kHScaleAttr = 'hscl';
const AttrId kVScaleAttr = 'vscl';

const kScaleRef = 100; //denom is always 100 for scale values (short input to SetAttributeValue)


//constants for SetDefaults
const long kUpdateOnlyScriptInfo = 1 << 16;

//***************************************************************************************************

struct LineRunDisplayInfo {
	const unsigned char* runChars;
	short runLen;
	
	Fixed runWidth;
	Fixed runExtraWidth;
	
	char lineDirection;
	
	Point zoomNumer;
	Point zoomDenom;
};
//***************************************************************************************************

struct RunPositionInfo {
	short runLineTop;
	
	short runLineHeight;
	
	Fixed runLeftEdge;
	
	Fixed runTotalWidth;
	
	Point zoomNumer;
	Point zoomDenom;
};
//***************************************************************************************************

//constants for clickFlags
const unsigned char kNoClickFlags = 0;
const unsigned char kNeedsSaveClickFlag = 1; //indication that the clicked obj needs save
const unsigned char kNeedsReformatClickFlag = 2;

//constants for "commandId" of ClickCommandInfo
const unsigned char kNoClickCmd = 0;

struct ClickCommandInfo {//returned from Click
	ClickCommandInfo() {
		commandId = kNoClickCmd;
		clickFlags = kNoClickFlags;
	}
	
	unsigned char clickFlags;
	
	unsigned char commandId;
	long commandParams[10];
};
//***************************************************************************************************


void IndivisiblePixelToChar(const LineRunDisplayInfo&, Fixed pixel, TOffsetRange* charRange);
Fixed IndivisibleCharToPixel(const LineRunDisplayInfo&, short charOffset);


class	CRunObject	:	public CAttrObject {
	public:
//-------
	CRunObject();
	
	void	IRunObject();
	
	//¥override
	virtual ClassId GetClassId() const = 0;
	
	virtual CAttrObject* CreateNew() const = 0;
	
	virtual void GetAttributes(CObjAttributes* attributes) const;

	virtual void SetDefaults(long message = 0);
	//loWord of message = script no, hiWord = see contants for default flags 
	
	virtual void Assign(const CAttrObject* srcObj, Boolean duplicate=true);
	
	virtual void SetAttributeValue(AttrId theAttr, const void* attrBuffer);
	
	virtual void Invalid();
	
	//¥own
	virtual Boolean IsTextRun() const = 0;
	
	virtual void SaveRunEnv();
	
	virtual void SetRunEnv() const;
	
	virtual char GetRunDir() const;
	virtual Boolean Is2Bytes() const;
	virtual char GetRunScript() const;
	virtual void SetKeyScript() const;
	
	void GetHeightInfo(short* ascent, short* descent, short* leading);
	
	virtual void PixelToChar(const LineRunDisplayInfo&, Fixed pixel, TOffsetRange* charRange) = 0;
	//charRange's offsets are wrt the run start
	
	virtual Fixed CharToPixel(const LineRunDisplayInfo&, short charOffset) = 0;
	
	virtual void Draw(const LineRunDisplayInfo&, Fixed runLeftEdge, const Rect& lineRect, short lineAscent) = 0;
	
	#ifdef txtnRulers
	virtual Fixed FullJustifPortion(const LineRunDisplayInfo&);
	#endif
	
	virtual Fixed MeasureWidth(const LineRunDisplayInfo&) = 0;

	virtual StyledLineBreakCode LineBreak(unsigned char* scriptRunChars, long scriptRunLen, long runOffset
																			, Fixed* widthAvail, Boolean forceBreak, long* breakLen) = 0;

	virtual void Click(const RunPositionInfo& posInfo, short countClicks, Point clickPoint, long modifiers
											, ClickCommandInfo*);
	
	virtual Boolean IsPointInside(Point, const RunPositionInfo&);
	/*called for active object when the point to test is outside the obj bounds
	(added for MovieObj which may have extra componenets which overlap text when activ).
	should return false in the normal case */
	
	virtual void SetCursor(Point, const RunPositionInfo&) const; //does nothing by default
	
	virtual Boolean FilterControlChar(unsigned char theChar) const;
	//true result ==> skip the char

	virtual unsigned long GetIdleTime() const;
	virtual void Idle(const RunPositionInfo&);

	virtual void SetHilite(char oldHilite, char newHilite, const RunPositionInfo&, Boolean doDraw = true);
	/*received by classes returning "kHasCustomHilite" from "GetObjFlags".
	¥paramPb is defined <==> doDraw == true
	¥oldHilite != newHilite always
	*/
	
	virtual void DrawSelection(char hiliteStat, const RunPositionInfo&);
	//received by classes returning "kHasCustomHilite" from "GetObjFlags".
	

	virtual ResType GetPublicType() const; // 0 means no public data type
	
	virtual OSErr ImportPublicData(Handle data);
	virtual OSErr ExportPublicData(Handle* data);
		
	protected:
//----------
	//¥override
	virtual long GetAttributeFlags(AttrId theAttr) const;
	
	virtual Boolean EqualAttribute(AttrId theAttr, const void* valToCheck) const;

	virtual void AttributeToBuffer(AttrId theAttr, void* attrBuffer) const;
	
	virtual void BufferToAttribute(AttrId theAttr, const void* attrBuffer);
		
	void AdjustScaleVals(Point* numer, Point* denom) const;
	
	//¥own
	virtual void CalcHeightInfo(short* ascent, short* descent, short* leading) = 0;
	//returns non scaled values
	
	inline void InvalidateHeight() {fAscent = kInvalidAscent;}
	
	private:
//--------
	short	fAscent;
	short	fDescent;
	short	fLeading;

	short fHScale;
	short fVScale;
};




