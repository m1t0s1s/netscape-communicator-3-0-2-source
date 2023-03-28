// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	CPageGeometry.h						  ©1995 Netscape. All rights reserved.
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

#pragma once

#include "TextensionCommon.h"
#include "Array.h"
#include "CLineHeights.h"


// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

class LView;
class CDisplayChanges;

class CPageGeometry
{
	private:
								CPageGeometry();		// parameters required
	public:
								CPageGeometry(CLineHeights* inLineHeights);
		virtual					~CPageGeometry();
		

			// ¥ General Measurement
		virtual void			GetPageFrameBounds(TLongRect& outBounds) const;
		virtual	void			GetActualPageFrameBounds(TLongRect& outBounds) const;
		virtual	void			GetActualPageFrameDimensions(SDimension32& outDims) const;
		virtual	void			GetPageFrameDimensions(SDimension32& outDims) const;
		virtual void 			SetPageFrameDimensions(
										const SDimension32& inNewDims,						// inNewSize is not scaled
										CDisplayChanges* 	ioChanges = NULL);


		virtual void			GetPageTextBounds(TLongRect& outBounds) const;
		virtual	void			GetActualPageTextBounds(TLongRect& outBounds) const;
		virtual	void			GetActualPageTextDimensions(SDimension32& outDims) const;
		virtual void 			GetPageTextDimensions(SDimension32& outDims) const;
		virtual void 			SetPageTextDimensions(
										const SDimension32& inNewDims,						// inNewSize is not scaled
										CDisplayChanges* 	ioChanges = NULL);
	
		virtual void			GetPageMargins(Rect& outMargins) const;
		virtual void			SetPageMargins(
										const Rect& 		inNewMargins,					// inNewMargins not scaled
										CDisplayChanges* 	ioChanges = NULL); 
		
		virtual Int32 			GetLineFormatWidth(Int32 inLineNumber) const;
		virtual Int32 			GetLineMaxWidth(Int32 inLineNumber) const;


			// ¥ Relative to View Context
		void					LocalToPageRect(
										LView*				inViewContext,
										const Rect& 		inLocalRect,
										TLongRect&			outPageRect) const;
		
		void					PageToLocalRect(
										LView*				inViewContext,
										const TLongRect& 	inPageRect,
										Rect&				outLocalRect) const;
		
		void					LocalToPagePoint(
										LView*				inViewContext,
										const Point& 		inLocalPoint,
										SPoint32&			outPagePoint) const;
		
		void					PageToLocalPoint(
										LView*				inViewContext,
										const SPoint32& 	inPagePoint,
										Point&				outLocalPoint) const;
		
		Int16					HorizPageToLocal(
										LView* 				inViewContext,
										Int32				inPageHPixel) const;
		
		Int16					VertPageToLocal(
										LView* 				inViewContext,
										Int32				inPageVPixel) const;

			// ¥ Character Calculations

		void					CharToPagePoint(
										LView*				inViewContext,
										Int32				inOffset,
										SPoint32&			outPagePoint) const;
		
		void					CharToLocalPoint(	
										LView*				inViewContext,
										Int32				inOffset,
										Point&				outLocalPoint) const;
		
			// ¥ Line Calculations
			
			
		void					LineToPagePoint(
										LView*				inViewContext,
										Int32				inLineNumber,
										SPoint32&			outPagePoint) const;

		void					LineToLocalPoint(
										LView*				inViewContext,
										Int32				inLineNumber,
										Point&				outLocalPoint) const;
		
		Int32 					PagePointToLineNumber(
										LView*				inViewContext,
										const SPoint32&		inPagePoint,
										Boolean&			outOutsideLine) const;

		Int32 					LocalPointToLineNumber(
										LView*				inViewContext,
										const Point&		inLocalPoint,
										Boolean&			outOutsideLine) const;
		
				
		
	protected:

		virtual Int32 			GetTotalHeight(void) const;
		virtual Int32 			GetTotalWidth(void) const;

	
		CLineHeights* 			mLineHeights;
		SDimension32 			mFrameBoundsSize;
		Rect					mMargins;
};



