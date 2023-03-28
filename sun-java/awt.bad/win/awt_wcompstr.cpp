// Copyright Netscape Communication 1996-1997
// Author: Frank Tang <ftang@netscape.com>
// =================================================================================
#include "awt_defs.h"
#include "awt_wcompstr.h"
#include "awt_vf.h"
#include "awt_font.h"
#define ASCIIMASK	0x80

#ifdef INTLUNICODE
//
// LEncodingTextList is current a private class that only used by this file
//

WinCompStr::WinCompStr() : CompStr()
{
	// Empty
}
WinCompStr::WinCompStr(WinCompStr& in) 
	: CompStr(in)
{
}
WinCompStr::WinCompStr(WCHAR* ustr) 
	: CompStr(ustr)
{
}
WinCompStr::WinCompStr(Hjava_lang_String *string) : CompStr(string)
{
}
WinCompStr::~WinCompStr()
{
	// Empty
}

void 
WinCompStr::GetTextExtent(HDC hDC, class AwtFont *font, SIZE *total)
{
	CompStrIterator	iter(this);
	int16	encoding;
	char*	text;
	SIZE size;
	total->cx = 0;
	total->cy = 0;
	while(iter.Next(&encoding, (unsigned char**)&text))
	{
		int len = strlen(text);
		if(font)
			::SelectObject(hDC, (HGDIOBJ)(font->GetFontForEncoding(encoding)));
		VERIFY(libi18n::CIntlWin_GetTextExtentPoint(encoding, hDC, text, len, &size));
		total->cx += size.cx;
		if(total->cy < size.cy)
				total->cy = size.cy;
	}
}


void 
WinCompStr::DrawText(HDC hDC, class AwtFont *font, CONST RECT *lprc, UINT flags)
{
	// Frist, let's figure out the width and height of the string
	SIZE	size;
	this->GetTextExtent(hDC, font, &size);
	size.cy = ( lprc->top + lprc->bottom - size.cy ) / 2;
	switch(flags)
	{	
	case DT_RIGHT:
		size.cx = lprc->right - size.cx;
		break;
	case DT_CENTER:
		size.cx = ( lprc->right + lprc->left - size.cx ) / 2;
		break;
	case DT_LEFT:
	default:
		size.cx = lprc->left;
		break;
	}
	this->TextOutWidth(hDC, size.cx, size.cy + 
		(font ? font->GetFontAscent() : 12 ), font);
}
long 
WinCompStr::TextOutWidth(HDC hDC, int x, int y, class AwtFont *font)
{
	int16 encoding;
	char* text;
	long width = 0;
	if(! font)
		return 0;

	// y -= font->GetFontAscent();
	UINT align = ::SetTextAlign(hDC, TA_BASELINE);
	CompStrIterator	iter(this);
	while(iter.Next(&encoding, (unsigned char**)&text))
	{
		int len = strlen(text);
		if(font)
			::SelectObject(hDC, (HGDIOBJ)(font->GetFontForEncoding(encoding)));

		SIZE size;
		VERIFY(libi18n::CIntlWin_GetTextExtentPoint(encoding, hDC, text, len, &size));
		VERIFY(libi18n::CIntlWin_TextOut(encoding, hDC, x + width, y, text, len));
		width += size.cx;

	}
	if(font)
		::SelectObject(hDC, (HGDIOBJ)font->GetFont());
	::SetTextAlign(hDC, align);
	return width;
}

//
//
//
WinMenuCompStr::WinMenuCompStr() : WinCompStr()
{
	// Empty
}
WinMenuCompStr::WinMenuCompStr(WCHAR* ustr) 
	: WinCompStr(ustr)
{
}
WinMenuCompStr::WinMenuCompStr(Hjava_lang_String *string) : WinCompStr(string)
{
}
WinMenuCompStr::~WinMenuCompStr()
{
	// Empty
}

void 
WinMenuCompStr::GetTextExtent(HDC hDC, class AwtVFontList *vfontlist, SIZE *total)
{
	CompStrIterator	iter(this);
	int16	encoding;
	int16	menuFontEncoding = libi18n::IntlMenuFontCsid();
	char*	text;
	SIZE size;
	total->cx = 0;
	total->cy = 0;
	while(iter.Next(&encoding, (unsigned char**)&text))
	{
		int len = strlen(text);
		if((encoding == menuFontEncoding) || (menuFontEncoding == CS_LATIN1))
		{

			if(encoding == menuFontEncoding)
				::SelectObject(hDC, GetStockObject(libi18n::IntlMenuFontID()));
			else
				::SelectObject(hDC, (HGDIOBJ)(vfontlist->GetVFont(encoding)));
			VERIFY(libi18n::CIntlWin_GetTextExtentPoint(encoding, hDC, text, len, &size));
			total->cx += size.cx;
			if(total->cy < size.cy)
					total->cy = size.cy;
		}
		else
		{
			int status = ASCIIMASK & text[0];
			int begin = 0;
			int j = 0;
			do
			{
				if(((j == len) || (( ASCIIMASK & text[j]) != status) ) 
					&& ((j-begin) > 0))
				{
					// Output
					SIZE size;
					if(status)		// if it is not ascii, use latin 1 font
					{
						::SelectObject(hDC, (HGDIOBJ)(vfontlist->GetVFont(CS_LATIN1)) );
						VERIFY(libi18n::CIntlWin_GetTextExtentPoint(CS_LATIN1, hDC, text, len, &size));
					}
					else		// Otherwise it is ascii, use the menu font
					{
						::SelectObject(hDC, ::GetStockObject(libi18n::IntlMenuFontID()) );
						VERIFY(libi18n::CIntlWin_GetTextExtentPoint(encoding, hDC, text, len, &size));
					}
					total->cx+= size.cx;
					if(total->cy < size.cy)
							total->cy = size.cy;
					// Change status
					begin = j;
					status = status ? 0x00 : 0x80 ;
				}
			} while( ++j <= len );
		}
	}
}


long 
WinMenuCompStr::TextOutWidth(HDC hDC, int x, int y, class AwtVFontList *vfontlist)
{
	int16 encoding;
	int16	menuFontEncoding = libi18n::IntlMenuFontCsid();
	char* text;
	long width = 0;
	if(! vfontlist)
		return 0;

	// y -= font->GetFontAscent();
	UINT align = ::SetTextAlign(hDC, TA_BASELINE);
	CompStrIterator	iter(this);
	while(iter.Next(&encoding, (unsigned char**)&text))
	{
		int len = strlen(text);
		if((encoding == menuFontEncoding) || (menuFontEncoding == CS_LATIN1))
		{
			if(encoding == menuFontEncoding)
				::SelectObject(hDC, ::GetStockObject(libi18n::IntlMenuFontID()) );
			else
				::SelectObject(hDC, (HGDIOBJ)(vfontlist->GetVFont(encoding)) );
			SIZE size;
			VERIFY(libi18n::CIntlWin_GetTextExtentPoint(encoding, hDC, text, len, &size));
			VERIFY(libi18n::CIntlWin_TextOut(encoding, hDC, x + width, y, text, len));
			width += size.cx;

		}
		else
		{
			int status = ASCIIMASK & text[0];
			int begin = 0;
			int j = 0;
			do
			{
				if(((j == len) || (( ASCIIMASK & text[j]) != status)) 
					&& ( (j-begin) > 0))
				{
					SIZE size;
					// Output
					if(status)	// if it is not ascii, use latin 1 font
					{
						::SelectObject(hDC, (HGDIOBJ)(vfontlist->GetVFont(CS_LATIN1)) );
						VERIFY(libi18n::CIntlWin_GetTextExtentPoint(CS_LATIN1, hDC, text+begin, j-begin, &size));
						VERIFY(libi18n::CIntlWin_TextOut(CS_LATIN1, hDC, x + width, y, text+begin, j-begin));
					}
					else		// Otherwise it is ascii, use the menu font
					{
						::SelectObject(hDC, ::GetStockObject(libi18n::IntlMenuFontID()) );
						VERIFY(libi18n::CIntlWin_GetTextExtentPoint(encoding, hDC, text+begin, j-begin, &size));
						VERIFY(libi18n::CIntlWin_TextOut(encoding, hDC, x + width, y, text+begin, j-begin));
					}
					width += size.cx;
					// Change status
					begin = j;
					status = status ? 0x00 : 0x80 ;
				}
			} while( ++j <= len );
		}
	}
	::SetTextAlign(hDC, align);
	return width;
}
void 
WinMenuCompStr::GetTextMetrics(HDC hDC, AwtVFontList *vfontlist, TEXTMETRIC *max)
{
	SelectObject(hDC, (HGDIOBJ) ::GetStockObject(libi18n::IntlMenuFontID()));
	VERIFY(::GetTextMetrics(hDC, max));
	vfontlist->LoadFontMetrics(hDC, max, libi18n::IntlMenuFontCsid());
}

WinMenuCompStrList::WinMenuCompStrList()
{
    m_items = NULL;
}
WinMenuCompStrList::~WinMenuCompStrList()
{
    MenuStrItem *p;
    MenuStrItem *n;
    for(p=m_items; p != NULL; p = n)
    {
        if(p->m_str != NULL)
            delete p->m_str;
        n = p->m_next;
        delete p;
        p = n;
    }
    m_items = NULL;
}
void
WinMenuCompStrList::AddItem(UINT inId, WinMenuCompStr *inStr)
{
    MenuStrItem *p = new MenuStrItem;
    if(p != NULL)
    {
        p->m_str = inStr;
        p->m_id = inId;
        p->m_next = m_items;
        m_items = p;
    }
}
void
WinMenuCompStrList::ModifyItem(UINT inId, WinMenuCompStr *inStr)
{
    MenuStrItem *p;
    MenuStrItem *n;
    for(p=m_items; p != NULL; p = n)
    {
        if(p->m_id == inId)
        {
            if(p->m_str != NULL)
                delete p->m_str;
            return;
        }
    }
    this->AddItem(inId,inStr);
}

#endif // INTLUNICODE
