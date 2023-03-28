// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	CTextRenderer.h					  ©1995 Netscape. All rights reserved.
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

#pragma once

#include "TextensionCommon.h"
#include "RunObject.h"
#include "CPageGeometry.h"
#include "Offscreen.h"
#include "StyledText.h"
#include "Formatter.h"
#include "CLineHeights.h"
#include "Line.h"

#ifdef txtnRulers
#include "RulersRanges.h"
#endif

#include <LArray.h>


class CSharedWorld;

// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	TSectLines is the element type of the array filled by
//	CTextRenderer::GetUpdateLines. an element with countLines == 0 ===> 
//	firstLineRect is a rect with no lines (probably should be erased by the
//	caller), in this case firstLine and lineAscent are undefined.
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

struct TSectLines {
	long	firstLine;
	Rect	firstLineRect; 	//rect height == lineHeight (in view local coordinates?)
	short	countLines;
	short 	lineAscent;
};
typedef TSectLines* TSectLinesPtr;



//	One for start of selection, one for end of selection, and one as a
//	spare (mouse location etc).
const Int16 kLineCacheSize = 3;


class CTextRenderer
{
		friend class CTextension;
		friend class CSelection;
	
	private:
								CTextRenderer();			// Paramerters Required
	public:
								CTextRenderer(
									CPageGeometry* 	inGeometry,
									CStyledText*	inStyledText,
									CFormatter*		inFormatter,
									CLineHeights*	inLineHeights);

		virtual					~CTextRenderer();
		
		virtual void 			Draw(
									LView* 			inViewContext,
									const Rect& 	inLocalUpdateRect,
									TDrawFlags 		inDrawFlags = kEraseBeforeDraw);
			
		virtual void 			DrawChars(
									long 			firstOffset,
									long 			lastOffset,
									TDrawFlags drawFlags = kEraseBeforeDraw);
		
		virtual void 			Invalidate(void);
		virtual	void			Compact(void);
		
		CLine* 					DoLineLayout(Int32 inLineNumber);

	
		void 					SetDirection(char newDirection); // does NOT have a draw effect
		inline char GetDirection(void) const {return mDirection;}
	
		
		Boolean 				VisibleToBackOrder(TOffset* charOffset, char runDir);
		

	protected:

		Boolean 				CalcUpdateLines(
									LView*			inViewContext,
									const Rect&		inLocalUpdateRect,
									Int32&			outLineCount,
									LArray& 		ioLines) const;

		Boolean 				DrawFrameText(
									LView* 			inViewContext,
									const Rect& 	inLocalUpdateRect,
									TDrawFlags 		inDrawFlags);
		
		void					DrawLineGroup(
									const TSectLines& 	groupToDraw,
									RgnHandle 			drawClip,
									TDrawFlags 			drawFlags);
		

		CPageGeometry* 			mGeometry;
		CStyledText* 			mStyledText;
		CFormatter* 			mFormatter;
		CLineHeights* 			mLineHeights;
		Int8					mDirection;

		CLine* 					mLine;
		
		

		Int32					mLineCacheLineNo[kLineCacheSize];
		CLine*					mLineCacheLine[kLineCacheSize];
		
		void					PurgeLineCache(void);
			
		static CSharedWorld*	sWorld;
};


