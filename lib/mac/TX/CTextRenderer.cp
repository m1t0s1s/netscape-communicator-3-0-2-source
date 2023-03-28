// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	CTextRenderer.cp					  ©1995 Netscape. All rights reserved.
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

#include "CTextRenderer.h"
#include "CPageGeometry.h"
#include "CGWorld.h"
#include "Paragraph.h"


#include <URegions.h>

// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	¥	
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
// we crash because of the type of looping in this file
//	if this is turned on
#pragma global_optimizer off

CSharedWorld* CTextRenderer::sWorld = NULL;

// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	¥	
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

CTextRenderer::CTextRenderer()
{
	Assert_(false); // we do this because friends could still construct
}

CTextRenderer::CTextRenderer(
	CPageGeometry* 	inGeometry,
	CStyledText*	inStyledText,
	CFormatter*		inFormatter,
	CLineHeights*	inLineHeights)
{
	mGeometry = inGeometry;
	mStyledText = inStyledText;
	mFormatter = inFormatter;
	mLineHeights = inLineHeights;
	mLine = nil;

	mDirection = ::GetSysDirection();

	for (long i = 0; i < kLineCacheSize; i++)
		mLineCacheLine[i] = NULL;
	
	try
		{
		if (sWorld == NULL)
			{
			Rect theNullFrame = { 0, 0, 1, 1 };
			GlobalToLocal(&topLeft(theNullFrame));
			GlobalToLocal(&botRight(theNullFrame));
			sWorld = new CSharedWorld(theNullFrame, 0, pixPurge | useTempMem);
			}
		
		// We put a shared lock on sWorld in case we're the only user and something
		// below fails.
		StSharer theShareLock(sWorld);	

		for (long i = 0; i < kLineCacheSize; i++)
			{
			mLineCacheLine[i] = new CLine;
			#ifdef txtnRulers
			mLineCacheLine[i]->ILine(mStyledText, mDirection, handlers->rulersRanges, kLargeSize);
			#else
			mLineCacheLine[i]->ILine(mStyledText, mDirection, kLargeSize);
			#endif
			}

		PurgeLineCache();
		sWorld->AddUser(this);
		}
	catch (ExceptionCode inErr)
		{
		for (long i = 0; i < kLineCacheSize; i++)
			delete mLineCacheLine[i];
			
		throw inErr;
		}					
}

// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	¥	
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

CTextRenderer::~CTextRenderer()
{
	mLine = NULL;
	for (long i = 0; i < kLineCacheSize; i++)
		{
		mLineCacheLine[i]->Free();
		delete mLineCacheLine[i];
		mLineCacheLine[i] = NULL;
		}
	
	Int32 theUseCount = sWorld->GetUseCount();
	sWorld->RemoveUser(this);
	if (theUseCount == 1)
		sWorld = NULL;
}

// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	¥	
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ


void CTextRenderer::Draw(
	LView* 			inViewContext,
	const Rect& 	inLocalUpdateRect,
	TDrawFlags 		inDrawFlags)
{
// FIX ME!!! may want to do what's in StyledText SaveTextEnvironment
	
//	TTextEnv theEnvironment;
//	mStyledText->SetTextEnv(&theEnvironment);

	mStyledText->LockChars();
	DrawFrameText(inViewContext, inLocalUpdateRect, inDrawFlags);
	mStyledText->UnlockChars();

//	mStyledText->RestoreTextEnv(theEnvironment);
}

// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	¥	
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

void CTextRenderer::DrawChars(long firstOffset, long lastOffset, TDrawFlags drawFlags)
{
// FIX ME!!! called from TSM code
#if 0
	if (fDrawVisLevel < 0) {return;}
	
	TDrawEnv savedDrawEnv;
	SetDrawEnv(&savedDrawEnv);
	
	mStyledText->LockChars();
	
	firstOffset = mFormatter->OffsetToLine(firstOffset);
	
	TOffset temp(lastOffset, true);
	lastOffset = mFormatter->OffsetToLine(temp);
	
	while (firstOffset <= lastOffset) {
		long frameNo = mLineHeights->LineToFrame(firstOffset);
		
		TLongRect absTextRect;

// FIX ME!!! need to get the textension object to fix this.		
//		fFrames->GetAbsTextBounds(frameNo, &absTextRect);
//		mGeometry->
		
		TOffsetPair frameLines;
		mLineHeights->GetFrameLineRange(frameNo, &frameLines);
		
		if (firstOffset > frameLines.firstOffset)
			{
			absTextRect.top += mLineHeights->GetLineRangeHeight(frameLines.firstOffset, firstOffset-1);
			}
			
		absTextRect.bottom = absTextRect.top + mLineHeights->GetLineRangeHeight(firstOffset, MinLong(frameLines.lastOffset, lastOffset));
		
		Rect rectToDraw;
		fFrames->AbsToDraw(absTextRect, &rectToDraw);
		
		DrawFrameText(frameNo, drawFlags, &rectToDraw);
		
		firstOffset = frameLines.lastOffset+1;
	}
	
	mStyledText->UnlockChars();

	RestoreDrawEnv(savedDrawEnv);
#endif
}

// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	¥	
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

void CTextRenderer::Invalidate(void)
{
	if (mLine != NULL)
		mLine->Invalid();
}

// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	¥	
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

CLine* CTextRenderer::DoLineLayout(Int32 inLineNumber)
{
	Boolean	lineFound = false;
	Int32	cacheIndex;
	
	Assert_(inLineNumber >= 0);
	
	//	does line exist in cache?
	for (cacheIndex = 0; cacheIndex < kLineCacheSize; cacheIndex++)
		{
		if (inLineNumber == mLineCacheLineNo[cacheIndex])
			{
			lineFound = true;
			break;
			}
		}
	
	if (!lineFound)
		cacheIndex = kLineCacheSize - 1;	//	move last element to front (most recent used);
		
	//	move cacheIndex to front of cache
	if (cacheIndex)
		{
		CLine	*savedOldLine = mLineCacheLine[cacheIndex];
		Int32	savedOldLineNo = mLineCacheLineNo[cacheIndex];
		
		for (long i = cacheIndex; i > 0; i--)
			{
			mLineCacheLine[i] = mLineCacheLine[i-1];
			mLineCacheLineNo[i] = mLineCacheLineNo[i-1];
			}
		
		mLineCacheLine[0] = savedOldLine;
		mLineCacheLineNo[0] = savedOldLineNo;
		}

	// either the line is not found, or the line is found but not valid
	if ((!lineFound) || (lineFound && !mLineCacheLine[0]->IsValid()))
		{
		TOffsetRange lineOffsets;
		mFormatter->GetLineRange(inLineNumber, &lineOffsets);
		mLineCacheLineNo[0] = inLineNumber;
		mLineCacheLine[0]->Invalid();
		mLineCacheLine[0]->DoLineLayout(lineOffsets.Start(), short(lineOffsets.Len()), mGeometry->GetLineMaxWidth(inLineNumber));
		}

	// We're always returning a valid line	
	mLine = mLineCacheLine[0];
	return mLine;
}


void CTextRenderer::Compact(void)
{
//	sOffscreenWorld.PurgeOffScreen();
}


void CTextRenderer::SetDirection(char newDirection)
{
	if (mDirection != newDirection)
		{
		mDirection = newDirection;
		mLine->SetDirection(newDirection);
		Invalidate();
		}
}


//chars are locked
//draw clip may be nil

// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	¥	
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

void CTextRenderer::DrawLineGroup(
	const TSectLines& 	groupToDraw,
	RgnHandle 			drawClip,
	TDrawFlags 			drawFlags)
{
	Rect lineRect = groupToDraw.firstLineRect;
	
	TDrawFlags erase = drawFlags & kEraseBeforeDraw;
	
	if (groupToDraw.countLines == 0)
		{
		if (erase)
			::EraseRect(&lineRect);
		return;
		}
	
	long lastLine = groupToDraw.firstLine + groupToDraw.countLines;
	short lineHeight = lineRect.bottom - lineRect.top;

	lineRect.top -= lineHeight;
	lineRect.bottom -= lineHeight;
	
	Boolean firstLineOffScreen = (drawFlags & kFirstLineOffScreen) != 0;

	for (long lineNo = groupToDraw.firstLine; lineNo < lastLine; ++lineNo)
		{
		lineRect.top += lineHeight;
		lineRect.bottom += lineHeight;
		
		if (drawClip && !RectInRgn(&lineRect, drawClip))
			continue;
		
		if (firstLineOffScreen)
			{
			sWorld->SetBounds(lineRect);
			firstLineOffScreen = (lineNo == groupToDraw.firstLine) &&
								 (sWorld->BeginDrawing());
			}
		
		if (erase)
			::EraseRect(&lineRect);	
		

			{
			#ifdef txtnRulers
				Boolean			lineIsAllLeftToRight = false;
			#else
				Boolean			lineIsAllLeftToRight = true;
			#endif
			TOffsetRange	lineOffsets;
			mFormatter->GetLineRange(lineNo, &lineOffsets);
	
			CRunObject		*run = NULL;
			CRunsRanges		*runs = mStyledText->GetRunsRanges();
			long			runOffset = lineOffsets.Start(),
							runLen,
							runEnd;

			while (lineIsAllLeftToRight)
				{
				if (runs->GetNextRun(runOffset, &run, &runLen) < 0)
					{
					run = NULL;
					break;
					}
				
				if (run && run->GetObjFlags() & kIndivisibleRun)
					lineIsAllLeftToRight = false;
				else if (run->GetRunDir() != kLR)
					lineIsAllLeftToRight = false;
				
				runOffset += runLen;
				
				if (runOffset > lineOffsets.End())
					break;
				}
			
			if (lineIsAllLeftToRight)
				{
				GrafPtr			currentPort;
				Fixed			runLeftEdge = LongToFixed(lineRect.left),
								leftEdgeOrigin = runLeftEdge;
				short			vBase = lineRect.top + groupToDraw.lineAscent;
				long			ctrlOffset;
		
				GetPort(&currentPort);

				gParagCtrlChars.Define(mStyledText->GetCharPtr(0), lineOffsets.Start(), lineOffsets.End());

				LineRunDisplayInfo	runDisplayInfo;
				runDisplayInfo.runWidth = 0;	//	don't know yet (and don't care)
				runDisplayInfo.runExtraWidth = 0;
				runDisplayInfo.lineDirection = kLR;
				runDisplayInfo.zoomNumer.h = 1;
				runDisplayInfo.zoomNumer.v = 1;
				runDisplayInfo.zoomDenom.h = 1;
				runDisplayInfo.zoomDenom.v = 1;
				
				runOffset = lineOffsets.Start();
				while (runOffset < lineOffsets.End())
					{
					if (runs->GetNextRun(runOffset, &run, &runLen) <  0)
						break;
					
					if (runOffset + runLen > lineOffsets.End())
						runLen = lineOffsets.End() - runOffset;
					runEnd = runOffset + runLen;
						
					while (runOffset < runEnd)
						{
						ctrlOffset = gParagCtrlChars.GetCurrCtrlOffset();
						
						if (runOffset == ctrlOffset)
							{
							if (gParagCtrlChars.GetCurrCtrlChar() == kTabCharCode)
								{
								runLeftEdge += GetDefaultTabWidth(runLeftEdge - leftEdgeOrigin, runDisplayInfo.zoomNumer.h, runDisplayInfo.zoomDenom.h);
								}
								
							runOffset++;
							runLen--;
							gParagCtrlChars.Next();
							}
						else if ((runOffset < ctrlOffset) && (ctrlOffset < runEnd))
							{
							runDisplayInfo.runChars = mStyledText->GetCharPtr(runOffset);
							runDisplayInfo.runLen = ctrlOffset - runOffset;
							
							run->SetRunEnv();
							MoveTo(FixRound(runLeftEdge), vBase);
							run->Draw(runDisplayInfo, runLeftEdge, lineRect, groupToDraw.lineAscent);
							
							runDisplayInfo.runWidth = LongToFixed((long)currentPort->pnLoc.h) - runLeftEdge;
							runLeftEdge += runDisplayInfo.runWidth;
							if (gParagCtrlChars.GetCurrCtrlChar() == kTabCharCode)
								runLeftEdge += GetDefaultTabWidth(runLeftEdge - leftEdgeOrigin, runDisplayInfo.zoomNumer.h, runDisplayInfo.zoomDenom.h);
							
							runOffset += runDisplayInfo.runLen;
							runOffset++;	//	for ctrl char
							gParagCtrlChars.Next();
							}
						else
							{
							runDisplayInfo.runChars = mStyledText->GetCharPtr(runOffset);
							runDisplayInfo.runLen = runEnd - runOffset;
							
							run->SetRunEnv();
							MoveTo(FixRound(runLeftEdge), vBase);
							run->Draw(runDisplayInfo, runLeftEdge, lineRect, groupToDraw.lineAscent);
							
							runDisplayInfo.runWidth = LongToFixed((long)currentPort->pnLoc.h) - runLeftEdge;
							runLeftEdge += runDisplayInfo.runWidth;
							
							runOffset += runDisplayInfo.runLen;
							}	
						}
					}
				}
			else
				{
				DoLineLayout(lineNo);
				mLine->Draw(lineRect, groupToDraw.lineAscent);					
				}
			}
	
		if (firstLineOffScreen)
			sWorld->EndDrawing();
	}
}
//************************************************************************************************

//Assume : SetDrawEnv and LockChars are called

Boolean CTextRenderer::DrawFrameText(
	LView* 			inViewContext,
	const Rect& 	inLocalUpdateRect,
	TDrawFlags 		inDrawFlags)
{
	Rect theLinesRect = inLocalUpdateRect;	
	inViewContext->CalcLocalFrameRect(theLinesRect);
	
	if (!SectRect(&inLocalUpdateRect, &theLinesRect, &theLinesRect))
		return false;
	

	Int32 countLines;
//	CArray* linesToDraw = gTempLines->GetLines();
	LArray theLinesToDraw(sizeof(TSectLines));
	
// FIX ME!!! perhaps we should move the GetUpdateLines() method into this class

	if (!CalcUpdateLines(inViewContext, theLinesRect, countLines, theLinesToDraw))
		{
//		gTempLines->DoneWithLines(theLinesToDraw);
		return false;
		}
	
	/*we have to clip on the text frame rect: italic and outline, for instance, may draw outside the
	text rect (in the margins) */
	StRegion theFrameClip;
	
	//better to call this after PreRectBitsModif since PreRectBitsModif may modify the clip
	Boolean result = ClipFurther(&theLinesRect, theFrameClip);
	
	if (result)
		{
// FIX ME!!! countLines should not need to be calculated here after the bug is fixed in
// GetUpdateLines()

		countLines = theLinesToDraw.GetCount();
		
		RgnHandle drawClip = qd.thePort->clipRgn; //made before BeginOffScreen changes the port
		if (IsRectRgn(drawClip))
			{drawClip = nil;}
		
		Boolean allOffScreen = false;
		
		if (inDrawFlags & kDrawAllOffScreen)
			{
			sWorld->SetBounds(theLinesRect);
			if (sWorld->BeginDrawing())
				{
				::EraseRect(&theLinesRect);
				
				allOffScreen = true;
				inDrawFlags &= (~(kFirstLineOffScreen | kEraseBeforeDraw)); //remove kFirstLineOffScreen and kEraseBeforeDraw
				}
			else
				{
				inDrawFlags |= kEraseBeforeDraw;
				if (countLines > 1)
					inDrawFlags |= kFirstLineOffScreen;
				//if we don't have memory for the whole lines, try only first line (keydowns)
				}
			}
		
		theLinesToDraw.Lock();
		TSectLines* theLineGroup = (TSectLines*)theLinesToDraw.GetItemPtr(arrayIndex_First);
		
		while (--countLines >= 0)
			{
			DrawLineGroup(*theLineGroup, drawClip, inDrawFlags);
			if (!allOffScreen)
				inDrawFlags &= (~kFirstLineOffScreen); //only the first line in the first group is drawn off screen
			++theLineGroup;
			}
		
		if (allOffScreen)
			sWorld->EndDrawing();
		
		theLinesToDraw.Unlock();
		
		SetClip(theFrameClip);
		}
		
	return result;
}

// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	¥	
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

void CTextRenderer::PurgeLineCache(void)
{
	for (long cacheIndex = 0; cacheIndex < kLineCacheSize; cacheIndex++)
		mLineCacheLineNo[cacheIndex] = -1;

	mLine->Invalid();
}

// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	¥	
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

Boolean CTextRenderer::VisibleToBackOrder(TOffset* charOffset, char runDir)
{
	DoLineLayout(mFormatter->OffsetToLine(*charOffset));
	return mLine->VisibleToBackOrder(charOffset, runDir);
}

// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	¥	
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

// FIX ME!!! critical performance bug!!!!!! this routine is calculating WAY TOO MANY LINES to
// redraw in outLineCount

// we may also want to add code to support not drawing lines that are partially clipped by
// the view frame

Boolean CTextRenderer::CalcUpdateLines(
	LView*			inViewContext,
	const Rect&		inLocalUpdateRect,
	Int32&			outLineCount,
	LArray& 		ioLines) const
{
	Rect theLocalRect = inLocalUpdateRect;

	TLongRect thePageBounds;
	mGeometry->GetPageTextBounds(thePageBounds);
	
	TLongRect thePageUpdate;
	mGeometry->LocalToPageRect(inViewContext, inLocalUpdateRect, thePageUpdate);

	// get the intersection of the page and the update area	
	if (!thePageBounds.Sect(thePageUpdate, &thePageUpdate))
		return false;
	
	ioLines.RemoveItemsAt(arrayIndex_First, ioLines.GetCount());	// clear out the result array	
	
	// convert the intersection rect back to local
	mGeometry->PageToLocalRect(inViewContext, thePageUpdate, theLocalRect);
	
	TOffsetPair theFrameLines;
	if (!mLineHeights->GetTotalLineRange(theFrameLines))
		{
		TSectLines theEmptyLines;
		theEmptyLines.countLines = 0;
		theEmptyLines.firstLineRect = theLocalRect;
		ioLines.InsertItemsAt(1, arrayIndex_Last, &theEmptyLines);
		return true;
		}

	Int32 thePixelOffset = thePageUpdate.top - thePageBounds.top;
	TLineHeightGroup* heightGroupPtr;
	Int32 groupLineNo;
	
	Int32 firstLine = mLineHeights->PixelToLine(&thePixelOffset, theFrameLines.firstOffset, &heightGroupPtr, &groupLineNo);
	
	mLineHeights->LockArray();
	
	Rect theLineRect;
	theLineRect.left = mGeometry->HorizPageToLocal(inViewContext, thePageBounds.left);
	theLineRect.right = mGeometry->HorizPageToLocal(inViewContext, thePageBounds.right);
	
	Int32 theLineTop = thePageBounds.top + thePixelOffset;
	theLineRect.top = mGeometry->VertPageToLocal(inViewContext, theLineTop);
	
	Int32 theHeight = thePageUpdate.bottom - theLineTop;
	outLineCount = 0;

	TSectLines theDummyLines;
	do {
		ioLines.InsertItemsAt(1, arrayIndex_Last, &theDummyLines);
		TSectLinesPtr sectedLinesPtr = (TSectLines*)ioLines.GetItemPtr(ioLines.GetCount());
		
		if (firstLine > theFrameLines.lastOffset)
			{ // a part from theRect doesn't contain any lineNo
			sectedLinesPtr->countLines = 0;
			theLineRect.bottom = theLocalRect.bottom;
			sectedLinesPtr->firstLineRect = theLineRect;
			break;
			}
		
		long lastLine = firstLine + mLineHeights->HeightToCountLines(*heightGroupPtr, groupLineNo, &theHeight) - 1;
		
		if (lastLine > theFrameLines.lastOffset)
			{
			lastLine = theFrameLines.lastOffset;
			theHeight += heightGroupPtr->lineHeight; //cycle again to add the "no lines rect"
			}
			
		long temp = lastLine - firstLine + 1;
		sectedLinesPtr->countLines = short(temp);
		
		theLineRect.bottom = theLineRect.top + heightGroupPtr->lineHeight;
		
		sectedLinesPtr->firstLineRect = theLineRect;
		sectedLinesPtr->firstLine = firstLine;
		
		sectedLinesPtr->lineAscent = heightGroupPtr->lineAscent;
		theLineRect.top += sectedLinesPtr->countLines * heightGroupPtr->lineHeight;

		firstLine += sectedLinesPtr->countLines;
		outLineCount += sectedLinesPtr->countLines;
		
		++heightGroupPtr;
		groupLineNo = 0;
		}
	while (theHeight > 0);

	
	mLineHeights->UnlockArray();
	return true;
}





