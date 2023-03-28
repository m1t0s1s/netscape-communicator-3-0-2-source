// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	CLineHeights.h
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

#pragma once


#include "TextensionCommon.h"
#include "Array.h"
#include "ObjectsRanges.h" //for CRanges



struct TLineHeightGroup { //lines having the same lineHeightInfo
	long groupCountLines; //count of lines in this group
	short lineHeight;
	short lineAscent;
};




class CLineHeights : protected CArray
{
	public:
								CLineHeights();
		virtual					~CLineHeights();
		
			
		void 					GetLineHeightInfo(
										Int32 				inLineNumber,
										TLineHeightInfo* 	lineHeightInfo) const;
		
		virtual OSErr 			InsertLine(
										const TLineHeightInfo& lineHeightInfo,
										Int32 				lineNo = -1);
										
		virtual OSErr 			SetLineHeightInfo(const TLineHeightInfo& lineHeightInfo, long lineNo);
		virtual void 			RemoveLines(long countLines, long lineNo);
	
		
		long 					PixelToLine(long* vPix, long lineNo = 0, TLineHeightGroup** heightGroupPtr = nil, long* groupLineIndex = nil) const;
	
		long 					HeightToCountLines(const TLineHeightGroup& group, long groupLineNo, long* height) const;
	
		Int32 					GetLineRangeHeight(long firstLine, long lastLine) const;
		long 					GetTotalLinesHeight(void) const;
				
		
		inline long CountLines() const {return fLastLineIndex+1;}


		virtual Boolean GetTotalLineRange(TOffsetPair& outLines) const;

			// Adjusts the access to public
		CArray::LockArray;
		CArray::UnlockArray;
		CArray::Compact;
				
	protected:
		short fNumer;
		short fDenom;
		
		
		Int32 		fTotalHeight;
		
		/*to optimize the call of GetLinesHeight if required all text height, fTotalHeight is kept scaled
		according to the sclae factor.*/
		
		long fLastLineIndex;
		
		TLineHeightGroup* 		LineToHeightGroup(long* lineNo, long* groupNo =nil) const;
		TLineHeightGroup* 		PixelToHeightGroup(long* vPix, long* groupLineNo) const;
		
		Boolean 				EqualGroup(long groupIndex, const TLineHeightGroup& groupPtr) const;
		Boolean 				EqualGroup(long group1Index, long group2Index) const;
		
		Boolean 				Concat(long group1Index, long group2Index, const TLineHeightGroup* groupPtr = nil);

};


