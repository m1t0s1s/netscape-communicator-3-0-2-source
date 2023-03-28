// =================================================================================
// Copyright Netscape Communication 1996-1997
// Author: Frank Tang <ftang@netscape.com>
// =================================================================================
#include "LMacCompStr.h"
#include <LList.h>
//
// LEncodingTextList is current a private class that only used by this file
//
typedef struct {
	char* text;
	int16 len;
	int16 encoding;
} EncodingRunStruct;

class LEncodingTextList {
	public:
		LEncodingTextList();
		~LEncodingTextList();
		void AddItem(char* text, int16 len, int16 encoding);
		void Draw(LJavaFontList* font);
		void Reset();
	private:
		LList *mList;
};
LEncodingTextList::LEncodingTextList()
{
	mList = new LList(sizeof(EncodingRunStruct));
	ThrowIfNil_( mList );
}
LEncodingTextList::~LEncodingTextList()
{
	delete mList;
}
void
LEncodingTextList::AddItem(char* text, int16 len, int16 encoding)
{
	EncodingRunStruct newrun;
	newrun.text 	= text;
	newrun.len 		= len;
	newrun.encoding = encoding;
	mList->InsertItemsAt(1, arrayIndex_Last ,&newrun);
}
void
LEncodingTextList::Draw(LJavaFontList* font)
{
	EncodingRunStruct currun;
	LListIterator	iter( *mList, iterate_FromStart );
	while ( iter.Next( &currun ) )
	{
		font->SetTextFont(currun.encoding);
		::DrawText(currun.text, 0 , currun.len);
	}
}
void
LEncodingTextList::Reset()
{
	delete mList;
	mList = new LList(sizeof(EncodingRunStruct));
}

LMacCompStr::LMacCompStr() : LCompStr()
{
	// Empty
}
LMacCompStr::LMacCompStr(unicode* ustr, int32 len) 
	: LCompStr(ustr, len)
{
}
LMacCompStr::LMacCompStr(struct Hjava_lang_String *stringObject) 
	: LCompStr(stringObject)
{
}
LMacCompStr::LMacCompStr(HArrayOfChar *text, long offset, long length) 
	: LCompStr(text, offset, length)
{
}
LMacCompStr::LMacCompStr(ConstStr255Param unicodePStr) :  LCompStr(unicodePStr)
{
}
LMacCompStr::~LMacCompStr()
{
	// Empty
}
void LMacCompStr::Draw(LJavaFontList* font, Rect& rect)
{
	short x,y;
	y = this->Baseline(font, rect);
	x = ( ( rect.right - this->Width(font) + rect.left ) / 2) ;
	this->Draw(font, x, y);
}



#define CaculateJustifiedStart(j,l,r,w)		(((j) == teJustRight) \
												? ((r)-(w)) \
												: (((j) == teJustCenter) \
													? ((l) + (((r) - (l)) - (w)) / 2)  \
													: (l)))

void LMacCompStr::DrawJustified(LJavaFontList* font, Rect& inRect, Int16 justification )
{
	long base = this->Baseline(font, inRect);
	long height = this->Height(font);
	font->SetTextSize();
	font->SetTextStyle();
	
	Fixed totalWidth = ::Long2Fix(inRect.right - inRect.left);
	long accumWidth = 0;
	
	int16 encoding;
	char* text;
	LCompStrIterator	iter(this);
	LEncodingTextList	textList; 

	Int32	blackSpace, lineBytes;
	Fixed	wrapWidth = totalWidth;

	while(iter.Next(&encoding, (unsigned char**)&text))
	{
		StyledLineBreakCode	lineBreak;
		lineBytes = 1;
		short length = strlen(text);
		do {
			// we need to set the font every time because the textList->DrawAllItems may change font.
			font->SetTextFont(encoding);
			// wrapWidth have to be set in per line base
			// lineBytes have to be set in per call base - 
			// 				!= 0 first call in a line
			//			 	== 0 remainnig call in that line
			lineBreak = ::StyledLineBreak(text, length, 0, length, 0,
											&wrapWidth, &lineBytes);
											
			if(lineBreak != smBreakOverflow)	// the line is broken here
			{
				// this is the last run in that line. So we need to
				// take care VisibleLength stuff here
				blackSpace = ::VisibleLength(text, lineBytes);
				short visibleWidth = ::TextWidth(text,0,blackSpace);
				accumWidth += visibleWidth;	// accumulate the length for this line
				textList.AddItem(text, blackSpace, encoding);

				// Ok, now let's decide the 
				short x = CaculateJustifiedStart(justification, inRect.left, inRect.right, accumWidth);
				::MoveTo(x, base);
				textList.Draw(font);
				textList.Reset();
				
				// adjust the base to the baseline of the next line
				base += height;
				// reset accumWidth 
				accumWidth = 0;
				// adjust the text and length to the new position				
				text += lineBytes;
				length -= lineBytes;				
				// since we break the line, 
				// we need to setup warpWidth and lineBytes again
				wrapWidth = totalWidth;
				lineBytes = 1;
			}
			else			// the line is not broken here
			{
				short visibleWidth = ::TextWidth(text,0,length);
				textList.AddItem(text, length, encoding);
				lineBytes = 0;	// we need to setup lineBytes for every call.
				accumWidth += visibleWidth;	// accumulate the length for this line
			}
		} while(lineBreak != smBreakOverflow);
	}
	// We still need to draw the text in the list
	short x = CaculateJustifiedStart(justification, inRect.left, inRect.right, accumWidth);
	::MoveTo(x, base);
	textList.Draw(font);
}

void LMacCompStr::DrawLeft(LJavaFontList* font, Rect& rect)
{
	short y;
	y = this->Baseline(font, rect);
	this->Draw(font, rect.left, y);
}
void LMacCompStr::Draw(LJavaFontList* font, short x, short y)
{
	int16 encoding;
	unsigned char* text;
	font->SetTextSize();
	font->SetTextStyle();
	LCompStrIterator	iter(this);
	::MoveTo(x,y);
	while(iter.Next(&encoding, &text))
	{
		font->SetTextFont(encoding);
		::DrawText(text, 0 , strlen((char*)text));
	}
}
long LMacCompStr::DrawWidth(LJavaFontList* font, short x, short y)
{
	long accum = 0;
	int16 encoding;
	unsigned char* text;
	font->SetTextSize();
	font->SetTextStyle();
	::MoveTo(x,y);
	LCompStrIterator	iter(this);
	while(iter.Next(&encoding, &text))
	{
		short length = strlen((char*)text);
		font->SetTextFont(encoding);
		::DrawText(text, 0 , length);
		accum += ::TextWidth(text, 0 , length);
	}
	return accum;
}
long LMacCompStr::Width(LJavaFontList* font)
{
	long accum = 0;
	int16 encoding;
	unsigned char* text;
	font->SetTextSize();
	font->SetTextStyle();
	LCompStrIterator	iter(this);
	while(iter.Next(&encoding, &text))
	{
		font->SetTextFont(encoding);
		accum += ::TextWidth(text, 0 , strlen((char*)text));
	}
	return accum;
}
long LMacCompStr::Baseline(LJavaFontList* font, Rect& rect)
{
	long max = 0;
	int16 encoding;
	unsigned char* text;
	font->SetTextSize();
	font->SetTextStyle();
	LCompStrIterator	iter(this);
	while(iter.Next(&encoding, &text))
	{
		font->SetTextFont(encoding);
		FontInfo	fontInfo;
		::GetFontInfo(&fontInfo);
		long base = fontInfo.ascent + fontInfo.leading;
			// long base = fontInfo.ascent + fontInfo.leading - fontInfo.descent;
			// According to UTextDrawing::DrawWithJustification() we dont need to
			// minus fontInfo.descent here.
		if(base > max)
			max = base;
	}
	return ( max +  rect.top + rect.bottom) / 2;
}
long LMacCompStr::Height(LJavaFontList* font)
{
	long max = 0;
	int16 encoding;
	unsigned char* text;
	font->SetTextSize();
	font->SetTextStyle();
	LCompStrIterator	iter(this);
	while(iter.Next(&encoding, &text))
	{
		font->SetTextFont(encoding);
		FontInfo	fontInfo;
		::GetFontInfo(&fontInfo);
		long height = fontInfo.ascent + fontInfo.descent + fontInfo.leading;
		if(height > max)
			max = height;
	}
	return max;
}
