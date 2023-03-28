// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ


#pragma once

#include "RunObject.h"
#include "Array.h"


//***************************************************************************************************

const ClassId kQDTextRunClassId = 'QDTx';

const AttrId kFontAttr = 'font';
const AttrId kFontSizeAttr = 'size';
const AttrId kFaceAttr = 'face';
const AttrId kForeColorAttr = 'klor';

//constants for message to UpdateAttribute
const long kForceScriptModif = 1 << 0; //for font
const long kAddSize = 1 << 1;

const long kToggleFace = 1 << 2;

const long kAddFace = 1 << 3;
const long kRemoveFace = 1 << 4;
//***************************************************************************************************

Fixed StdTxFixedMeas(short byteCount,const void *textAddr, Point numer, Point denom);

//***************************************************************************************************

class	CQDTextRun	:	public CRunObject {
	public:
		CQDTextRun();
		
		void	IQDTextRun();
		virtual CAttrObject* CreateNew() const;
		
		virtual ClassId GetClassId() const;
		
		Boolean IsTextRun() const;
		
		virtual void GetAttributes(CObjAttributes* attributes) const;
		
		virtual char GetRunDir() const;
		virtual Boolean Is2Bytes() const;
		virtual char GetRunScript() const;
		virtual void SetKeyScript() const;
		
		virtual void SaveRunEnv();
		virtual void SetRunEnv() const;
		
		virtual void SetDefaults(long message = 0);
	
		virtual Boolean FilterControlChar(unsigned char theKey) const;

		virtual void PixelToChar(const LineRunDisplayInfo&, Fixed pixel, TOffsetRange* charRange);
		
		virtual Fixed CharToPixel(const LineRunDisplayInfo&, short charOffset);
		
		virtual void Draw(const LineRunDisplayInfo&, Fixed runLeftEdge, const Rect& lineRect, short lineAscent);
		
		#ifdef txtnRulers
		virtual Fixed FullJustifPortion(const LineRunDisplayInfo&);
		#endif
		
		virtual Fixed MeasureWidth(const LineRunDisplayInfo&);
		
		virtual StyledLineBreakCode LineBreak(unsigned char* scriptRunChars, long scriptRunLen, long runOffset
																			, Fixed* widthAvail, Boolean forceBreak, long* breakLen);

		virtual void SetCursor(Point, const RunPositionInfo&) const; //set it to ibeam
				
	protected:
		char			fScript;
		short			fFont;
		long			fFace;
		RGBColor	fForeColor;	
		
		/*the lowest byte is reserved for the QD Style (bold, italic, ...), descendents can add more faces*/
		char fDirection;
		Boolean f2Bytes;
		short		fFontSize;

		virtual long GetAttributeFlags(AttrId theAttr) const;

		virtual void CalcHeightInfo(short* ascent, short* descent, short* leading); //override
		
		virtual Boolean EqualAttribute(AttrId theAttr, const void* valToCheck) const;
		//override, ¥any descendent should override, otherwise the result is always false
		
		virtual Boolean SetCommonAttribute(AttrId theAttr, const void* valToCheck); //override
		virtual void AttributeToBuffer(AttrId theAttr, void* attrBuffer) const; //override
		virtual void BufferToAttribute(AttrId theAttr, const void* attrBuffer); //override
		virtual void UpdateAttribute(AttrId theAttr, const void* srcAttr, long updateMessage
																, const void* continuousAttr = nil); //override
		//continuousAttr is used only if fFlags has the kToggleFace set

		virtual void AddFace(long faceToAdd, long* destFace);
		/*normally the result is (faceToAdd | destFace), but this methode is the place to check for exclusif faces
		(eg superScript-subScript, condensed-extended)
		*/
		
		void RemoveFace(long faceToRemove, long *destFace) const;
};


