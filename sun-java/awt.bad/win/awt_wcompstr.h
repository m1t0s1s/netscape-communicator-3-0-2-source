// =================================================================================
// Copyright Netscape Communication 1996-1997
// Author: Frank Tang <ftang@netscape.com>
// =================================================================================
#ifndef AWT_WCOMPSTR_H
#define AWT_WCOMPSTR_H
#include "awt_compstr.h"
#include "awt_vf.h"

class WinCompStr : public CompStr {
	public:
		WinCompStr();
		WinCompStr(WCHAR* ustr);
		WinCompStr(WinCompStr& in);
		WinCompStr(Hjava_lang_String *string);
		~WinCompStr();
		long TextOutWidth(HDC hDC, int x, int y, class AwtFont *font);
		void DrawText(HDC hDC, class AwtFont *font, CONST RECT *lprc, UINT flags);
		void GetTextExtent(HDC hDC, class AwtFont *font, SIZE *total);

};

class WinMenuCompStr : public WinCompStr {
	public:
		WinMenuCompStr();
		WinMenuCompStr(WCHAR* ustr);
		WinMenuCompStr(Hjava_lang_String *string);
		~WinMenuCompStr();
		void GetTextMetrics(HDC hDC, AwtVFontList *vfontlist, TEXTMETRIC *max);
		long TextOutWidth(HDC hDC, int x, int y, class AwtVFontList *font);
		void GetTextExtent(HDC hDC, class AwtVFontList *font, SIZE *total);
};

// Forward Declaration
struct MenuStrItem;
struct MenuStrItem {
    UINT                m_id;
    WinMenuCompStr     *m_str;
    MenuStrItem        *m_next;    
};

class WinMenuCompStrList {
       public:
                WinMenuCompStrList();
                ~WinMenuCompStrList();
                void AddItem(UINT inId, WinMenuCompStr *inStr);
                void ModifyItem(UINT inId, WinMenuCompStr *inStr);
       private:
                MenuStrItem *m_items;
};

#endif // AWT_WCOMPSTR_H
