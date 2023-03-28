#include "awt_button.h"
#include "awt_toolkit.h"
#include "awt_defs.h"
// All java header files are extern "C"
extern "C" {
#include "java_awt_Button.h"
#include "java_awt_Checkbox.h"
#include "java_awt_CheckboxGroup.h"

#include "sun_awt_windows_WComponentPeer.h"
#include "sun_awt_windows_WButtonPeer.h"
#include "sun_awt_windows_WCheckboxPeer.h"
};


AwtButton::AwtButton(HWButtonPeer *peer) : AwtWindow((JHandle *)peer)
{
    DEFINE_AWT_SENTINAL("BTN");
    m_dwButtonType = BS_PUSHBUTTON;
#ifdef INTLUNICODE
	m_pLabel = NULL;
	m_checked = FALSE;
#endif	// INTLUNICODE
}

AwtButton::AwtButton(HWCheckboxPeer *peer) : AwtWindow((JHandle *)peer)
{
	
    // get hand on the checkbox object (via peer)
    Hjava_awt_Checkbox *target;
    target = (Hjava_awt_Checkbox *)
                unhand((HWComponentPeer *)m_pJavaPeer)->target;

    if (unhand(target)->group == 0) {
        DEFINE_AWT_SENTINAL("CBX");
        m_dwButtonType = BS_CHECKBOX;
    } else {
        DEFINE_AWT_SENTINAL("RBN");
        m_dwButtonType = BS_RADIOBUTTON;
    }
#ifdef INTLUNICODE
	m_pLabel = NULL;
	m_checked = FALSE;
#endif	// INTLUNICODE
}

#ifdef INTLUNICODE
AwtButton::~AwtButton()
{
	if(m_pLabel)
	{
		delete m_pLabel;
		m_pLabel = NULL;
	}
}
#endif	// INTLUNICODE
// -----------------------------------------------------------------------
//
// Return the window class of an AwtButton window.
//
// -----------------------------------------------------------------------
const char * AwtButton::WindowClass()
{
    return "BUTTON";
}


// -----------------------------------------------------------------------
//
// Return the window style used to create an AwtButton window
//
// -----------------------------------------------------------------------
DWORD AwtButton::WindowStyle()
{
#ifdef INTLUNICODE
    return WS_CHILD | WS_CLIPSIBLINGS | BS_OWNERDRAW;
#else // INTLUNICODE
    return WS_CHILD | WS_CLIPSIBLINGS | m_dwButtonType;
#endif
}


// -----------------------------------------------------------------------
//
// Set the label text for the button
//
// -----------------------------------------------------------------------
void AwtButton::SetLabel(Hjava_lang_String * string)
{
    ASSERT( ::IsWindow(m_hWnd) );
#ifdef	INTLUNICODE
	WinCompStr * newlabel = new WinCompStr(string);
	if(m_pLabel)	{
		WinCompStr *oldlabel = m_pLabel;
		m_pLabel = newlabel;
		delete oldlabel;
	} else {
		m_pLabel = newlabel;
	}
	::SetWindowText(m_hWnd, "");	// To force a redraw
#else	// INTLUNICODE
    int32_t len;
    char *buffer = NULL;

    len    = javaStringLength(string);
    buffer = (char *)malloc(len+1);
    if (buffer) {
        javaString2CString(string, buffer, len+1);

        ::SetWindowText(m_hWnd, buffer);

        free(buffer);
    }
#endif	// INTLUNICODE
}


// -----------------------------------------------------------------------
//
// Set the state for the check box
//
// -----------------------------------------------------------------------
void AwtButton::SetState(long lState)
{
    ASSERT( ::IsWindow(m_hWnd) );
#ifdef INTLUNICODE
	m_checked = (lState) ? TRUE : FALSE;
	::InvalidateRgn(m_hWnd, NULL, FALSE);
	::UpdateWindow(m_hWnd);
#else	// INTLUNICODE
    ::SendMessage(m_hWnd, BM_SETCHECK, (WPARAM)((lState) ? TRUE : FALSE), 0L);
#endif	// INTLUNICODE
}

// -----------------------------------------------------------------------
//
// 
// -----------------------------------------------------------------------
BOOL AwtButton::OnCommand(UINT notifyCode, UINT id)
{
    if (notifyCode == BN_CLICKED) {
        if (m_pJavaPeer) {
            int64 time = PR_NowMS(); // time in milliseconds

            //
            // Call handleAction(...) on the proper button peer 
            //
            if (m_dwButtonType == BS_PUSHBUTTON) {
                AwtToolkit::RaiseJavaEvent(m_pJavaPeer, 
                                            AwtEventDescription::ButtonAction,
                                            time,
                                            NULL);
            }
            else { 
#ifdef INTLUNICODE
                AwtToolkit::RaiseJavaEvent(m_pJavaPeer, 
                                            AwtEventDescription::CheckAction,
                                            time, 
                                            NULL,
                                            ! m_checked);
#else	// INTLUNICODE
                AwtToolkit::RaiseJavaEvent(m_pJavaPeer, 
                                            AwtEventDescription::CheckAction,
                                            time, 
                                            NULL,
                                            !::SendMessage (m_hWnd, BM_GETCHECK, 0, 0L));
#endif	// INTLUNICODE
            }
        }
    }

    return FALSE;
}


BOOL AwtButton::OnPaint( HDC hdc, int x, int y, int w, int h )
{                                                             
    return AwtComponent::OnPaint(hdc, x, y, w, h);
}
    
#ifdef INTLUNICODE
#ifdef _WIN32

BOOL AwtButton::DrawWin95Item(UINT idCtl, LPDRAWITEMSTRUCT lpdis)
{
	switch(m_dwButtonType)
	{
	case BS_PUSHBUTTON :	
		DrawWin95Button( lpdis);
		break;
	case BS_RADIOBUTTON :	
		DrawWin95Radio( lpdis);
		break;
	case BS_CHECKBOX :	
		DrawWin95Checkbox( lpdis);
		break;
	default:
		return FALSE;
	}
	return TRUE;
}
//
//	This function draw the button in the case of WIN32 and running on Win95
//
void AwtButton::DrawWin95Button(LPDRAWITEMSTRUCT lpdis)
{
	// 1. fill out the rectangle wiht background brush
	::FillRect(	lpdis->hDC,	&lpdis->rcItem,	m_BackgroundColor->GetBrush());

	// 2. decide which color we should use to draw the four edges
	COLORREF color1, color2, color3, color4;
	if(lpdis->itemState & ODS_SELECTED)
	{
		color1 = ::GetSysColor(COLOR_3DHILIGHT);
		color2 = ::GetSysColor(COLOR_3DLIGHT);
		color3 = ::GetSysColor(COLOR_3DDKSHADOW);
		color4 = ::GetSysColor(COLOR_3DSHADOW);
	}
	else
	{
		color1 = ::GetSysColor(COLOR_3DDKSHADOW);
		color2 = ::GetSysColor(COLOR_3DSHADOW);
		color3 = ::GetSysColor(COLOR_3DHILIGHT);
		color4 = ::GetSysColor(COLOR_3DLIGHT);
	}

	// 3. create pen and draw them
	HPEN oldPen = (HPEN)::SelectObject(lpdis->hDC, 
		::CreatePen(PS_SOLID, 0, color1));

	POINT seg1[3];
	seg1[0].x = lpdis->rcItem.left; 
	seg1[0].y = lpdis->rcItem.bottom-1;
	seg1[1].x =	lpdis->rcItem.right-1;
	seg1[1].y = lpdis->rcItem.bottom-1;
	seg1[2].x =	lpdis->rcItem.right-1;
	seg1[2].y =	lpdis->rcItem.top;
	::Polyline(lpdis->hDC, seg1, 3);

	::DeleteObject(::SelectObject(lpdis->hDC, 
		::CreatePen(PS_SOLID, 0, color2)));

	POINT seg2[3];
	seg2[0].x =	lpdis->rcItem.left+1;
	seg2[0].y =	lpdis->rcItem.bottom-2;
	seg2[1].x =	lpdis->rcItem.right-2;
	seg2[1].y =	lpdis->rcItem.bottom-2; 
	seg2[2].x =	lpdis->rcItem.right-2;
	seg2[2].y =	lpdis->rcItem.top;
	::Polyline(lpdis->hDC, seg2, 3);

	::DeleteObject(::SelectObject(lpdis->hDC, 
		::CreatePen(PS_SOLID, 0, color3)));

	POINT seg3[3];
	seg3[0].x =	lpdis->rcItem.left;	
	seg3[0].y =	lpdis->rcItem.bottom-2; 
	seg3[1].x =	lpdis->rcItem.left;
	seg3[1].y =	lpdis->rcItem.top; 
	seg3[2].x =	lpdis->rcItem.right-1;
	seg3[2].y =	lpdis->rcItem.top;
	::Polyline(lpdis->hDC, seg3, 3);

	::DeleteObject(::SelectObject(lpdis->hDC, 
		::CreatePen(PS_SOLID, 0, color4)));

	POINT seg4[3]; 
	seg4[0].x =	lpdis->rcItem.left+1;
	seg4[0].y =	lpdis->rcItem.bottom-3; 
	seg4[1].x =	lpdis->rcItem.left+1;
	seg4[1].y =	lpdis->rcItem.top+1; 
	seg4[2].x =	lpdis->rcItem.right-2;
	seg4[2].y =	lpdis->rcItem.top+1;
	::Polyline(lpdis->hDC, seg4, 3);

	::DeleteObject(::SelectObject(lpdis->hDC, oldPen));

	// 4. Now we deal with text- only there are text and font

	AwtFont* pFont = this->GetFont();
	if((m_pLabel != NULL ) && (pFont != NULL ))
	{
		// 4.1 set the mode to TRANSPARENT
		::SetBkMode(lpdis->hDC, TRANSPARENT);

		// 4.2 If it is disabled, draw a ghost one first.
		if(lpdis->itemState & ODS_DISABLED)
		{
			// 4.2.1 set the text color to 3DLIGHT to draw to ghost text
			// 	Correct 3DLIGHT
			COLORREF bg = m_BackgroundColor->GetColor();
			COLORREF fg = m_ForegroundColor->GetColor();
			COLORREF gray = RGB(
				((GetRValue(bg) >> 1) + (GetRValue(fg) >> 1)),
				((GetGValue(bg) >> 1) + (GetGValue(fg) >> 1)),
				((GetBValue(bg) >> 1) + (GetBValue(fg) >> 1))
			);
			int lightr = (GetRValue(bg) - (GetRValue(fg) >> 1) + (GetRValue(bg) >> 1));
			int lightg = (GetGValue(bg) - (GetGValue(fg) >> 1) + (GetGValue(bg) >> 1));
			int lightb = (GetBValue(bg) - (GetBValue(fg) >> 1) + (GetBValue(bg) >> 1));
			COLORREF light = RGB(
				((lightr & 0xFF00 ) ? 0x00FF : (lightr)),
				((lightg & 0xFF00 ) ? 0x00FF : (lightg)),
				((lightb & 0xFF00 ) ? 0x00FF : (lightb))
			);

			//::SetTextColor(lpdis->hDC, ::GetSysColor(COLOR_3DLIGHT));
			::SetTextColor(lpdis->hDC, light);

			RECT ghostRect = lpdis->rcItem;
			::OffsetRect(&ghostRect, 1,1);
			m_pLabel->DrawText(lpdis->hDC, pFont ,&ghostRect, DT_CENTER);
			// 4.2.2 set the text color to GRAYTEXT so later we will draw the gray one
			// 	Correct GRAYTEXT
			////::SetTextColor(lpdis->hDC, ::GetSysColor(COLOR_GRAYTEXT));
			::SetTextColor(lpdis->hDC, gray);
		}
		else
		{
			::SetTextColor(lpdis->hDC, m_ForegroundColor->GetColor());
		}

		// 4.3 Find teh rect that we should draw the text
		RECT textrect = lpdis->rcItem;
		if(lpdis->itemState & ODS_SELECTED)
			::OffsetRect(&textrect, 1,1);

		// 4.4 Draw the text
		m_pLabel->DrawText(lpdis->hDC, pFont ,&textrect, DT_CENTER);
	}

	// 5 Draw focus box
	if( (lpdis->itemState & ODS_FOCUS) && 
		!(lpdis->itemState & ODS_DISABLED))
	{
		RECT focus = lpdis->rcItem;
		::InflateRect(&focus, -3, -3);
		::DrawFocusRect(lpdis->hDC, &focus);
	}
}
void AwtButton::DrawWin95RadioAndCheckboxText(LPDRAWITEMSTRUCT lpdis)
{
	AwtFont* pFont = this->GetFont();
	if((m_pLabel != NULL ) && (pFont != NULL ))
	{
		VERIFY(m_pLabel);
		::SetTextColor(lpdis->hDC, m_ForegroundColor->GetColor());
		::SetBkColor(lpdis->hDC, m_BackgroundColor->GetColor());
		::SetBkMode(lpdis->hDC, TRANSPARENT);

		// 1 figure out the rect for the text
		RECT textrect = lpdis->rcItem;
		textrect.left += 18;

		// 2 draw the text
		m_pLabel->DrawText(lpdis->hDC, pFont ,&textrect, DT_LEFT);

		// 3 draw the focus box
		if(lpdis->itemState & ODS_FOCUS)
		{
			// 3.1 figure out hot big is the text
			SIZE size;
			m_pLabel->GetTextExtent(lpdis->hDC, pFont, &size);

			// 3.2 decide the rect of the focus base on the size of the text and rect
			RECT focus;
			focus.top =  (lpdis->rcItem.top + lpdis->rcItem.bottom - size.cy ) / 2 - 1;
			focus.bottom = focus.top + size.cy + 2;
			focus.left = lpdis->rcItem.left + 18 - 1;
			focus.right = focus.left + size.cx + 2;
			// 3.3 draw the focus box
			::DrawFocusRect(lpdis->hDC, &focus);
		}
	}
}
//
//	This function draw the Radiobutton in the case of WIN32 and running on Win95
//
void AwtButton::DrawWin95Radio(LPDRAWITEMSTRUCT lpdis)
{
	// 1. draw the rectangle with the background brush
	::FillRect(	lpdis->hDC,	&lpdis->rcItem,	m_BackgroundColor->GetBrush());

	// 2. caculate where to draw the radio bullet
	int top = ( lpdis->rcItem.top + lpdis->rcItem.bottom ) / 2 - 6;

	// 3. Draw center of the bullet (not the dot)
	if(lpdis->itemState & ODS_SELECTED)
		::SelectObject(	lpdis->hDC,	m_BackgroundColor->GetBrush());
	else
		::SelectObject(	lpdis->hDC,	::GetSysColorBrush(COLOR_WINDOW));
	::Ellipse(lpdis->hDC,	lpdis->rcItem.left+1 ,	top+1,
							lpdis->rcItem.left + 11,top+11);

	// 4. Draw the dot if checked
	if(m_checked)
	{
		::SelectObject(	lpdis->hDC,	m_ForegroundColor->GetBrush());
		
		::Ellipse(lpdis->hDC,	lpdis->rcItem.left+4,	top+4,
								lpdis->rcItem.left+8,	top+8);
	}

	// 5. Draw the Edge
	int x=lpdis->rcItem.left;
	int y=top;
	HPEN oldPen = (HPEN)::SelectObject(lpdis->hDC,
		::CreatePen(PS_SOLID, 0, ::GetSysColor(COLOR_3DSHADOW) ));
	
	POINT seg1[12]; 
	seg1[0].x=	x+9;	seg1[0].y=	y+1;
	seg1[1].x=	x+8;	seg1[1].y=	y+1;
	seg1[2].x=	x+7;	seg1[2].y=	y;
	seg1[3].x=	x+4;	seg1[3].y=	y;
	seg1[4].x=	x+3;	seg1[4].y=	y+1;
	seg1[5].x=	x+2;	seg1[5].y=	y+1;
	seg1[6].x=	x+1;	seg1[6].y=	y+2;
	seg1[7].x=	x+1;	seg1[7].y=	y+3;
	seg1[8].x=	x;		seg1[8].y=	y+4;
	seg1[9].x=	x;		seg1[9].y=	y+7;
	seg1[10].x=	x+1;	seg1[10].y=	y+8;
	seg1[11].x=	x+1;	seg1[11].y=	y+10;

	::Polyline(lpdis->hDC, seg1, 12);

	::DeleteObject(::SelectObject(lpdis->hDC, 
		::CreatePen(PS_SOLID, 0, ::GetSysColor(COLOR_3DDKSHADOW))));

	POINT seg2[11];
	
	seg2[0].x = x+9;	seg2[0].y = y+2;
	seg2[1].x = x+8;	seg2[1].y = y+2;
	seg2[2].x = x+7;	seg2[2].y = y+1;
	seg2[3].x = x+4;	seg2[3].y = y+1;
	seg2[4].x = x+3;	seg2[4].y = y+2;
	seg2[5].x = x+2;	seg2[5].y = y+2;
	seg2[6].x = x+2;	seg2[6].y = y+3;
	seg2[7].x = x+1;	seg2[7].y = y+4;
	seg2[8].x = x+1;	seg2[8].y = y+7;
	seg2[9].x = x+2;	seg2[9].y = y+8;
	seg2[10].x =	x+2;	seg2[10].y = y+9;
	::Polyline(lpdis->hDC, seg2, 11);

	::DeleteObject(::SelectObject(lpdis->hDC, 
		::CreatePen(PS_SOLID, 0, ::GetSysColor(COLOR_3DHILIGHT))));

	POINT seg3[12]; 
	seg3[0].x =	x+10; seg3[0].y =	y+2;
	seg3[1].x = 	x+10; seg3[1].y =	y+3;
	seg3[2].x = 	x+11; seg3[2].y =	y+4;
	seg3[3].x = 	x+11; seg3[3].y =	y+7; 
	seg3[4].x = 	x+10; seg3[4].y =	y+8;
	seg3[5].x = 	x+10; seg3[5].y =	y+9;
	seg3[6].x = 	x+9; seg3[6].y =	y+10;
	seg3[7].x = 	x+8; seg3[7].y =	y+10; 
	seg3[8].x = 	x+7; seg3[8].y =	y+11;
	seg3[9].x = 	x+4; seg3[9].y =	y+11;
	seg3[10].x = 	x+3; seg3[10].y =	y+10;
	seg3[11].x = 	x+1; seg3[11].y =	y+10;
	::Polyline(lpdis->hDC, seg3, 12);

	::DeleteObject(::SelectObject(lpdis->hDC, 
		::CreatePen(PS_SOLID, 0, ::GetSysColor(COLOR_3DLIGHT))));

	POINT seg4[10];
	seg4[0].x =	x+9; seg4[0].y =  y+3;
	seg4[1].x =	x+10; seg4[1].y = 	y+4;
	seg4[2].x =	x+10; seg4[2].y = 	y+7;
	seg4[3].x =	x+9; seg4[3].y = 	y+8; 
	seg4[4].x =	x+9; seg4[4].y = 	y+9;
	seg4[5].x =	x+8; seg4[5].y = 	y+9;
	seg4[6].x =	x+7; seg4[6].y = 	y+10;
	seg4[7].x =	x+4; seg4[7].y = 	y+10; 
	seg4[8].x =	x+3; seg4[8].y = 	y+9;
	seg4[9].x =	x+1; seg4[9].y = 	y+9;
	::Polyline(lpdis->hDC, seg4, 10);

	::DeleteObject(::SelectObject(lpdis->hDC, oldPen));

	// 6 Daw the text
	this->DrawWin95RadioAndCheckboxText(lpdis);

}
//
//	This function draw the Checkbox in the case of WIN32 and running on Win95
//
void AwtButton::DrawWin95Checkbox(LPDRAWITEMSTRUCT lpdis)
{
	// 1. fill the rectangle with background color
	::FillRect(	lpdis->hDC,	&lpdis->rcItem,	m_BackgroundColor->GetBrush());

	// 2. figure out the rect for the box
	int t = ( lpdis->rcItem.top + lpdis->rcItem.bottom ) / 2 - 6;
	int b = t + 13;
	int l = lpdis->rcItem.left;
	int r = l + 13;
	// 3. draw the four edges
	HPEN oldPen = (HPEN)::SelectObject(lpdis->hDC,
		::CreatePen(PS_SOLID, 0,  ::GetSysColor(COLOR_3DHILIGHT)));

	POINT seg1[3];
	seg1[0].x = l; 		seg1[0].y = b-1;
	seg1[1].x = r-1; 	seg1[1].y = b-1;
	seg1[2].x = r-1; 	seg1[2].y = t-1;
	::Polyline(lpdis->hDC, seg1, 3);

	::DeleteObject(::SelectObject(lpdis->hDC, 
		::CreatePen(PS_SOLID, 0, ::GetSysColor(COLOR_3DLIGHT))));

	POINT seg2[3];
	seg2[0].x = l+1;	seg2[0].y = b-2;
	seg2[1].x = r-2;	seg2[1].y = b-2;
	seg2[2].x = r-2;	seg2[2].y = t;
	::Polyline(lpdis->hDC, seg2, 3);

	::DeleteObject(::SelectObject(lpdis->hDC, 
		::CreatePen(PS_SOLID, 0, ::GetSysColor(COLOR_3DSHADOW))));

	POINT seg3[3];
	seg3[0].x = l;		seg3[0].y = b-2;
	seg3[1].x = l;		seg3[1].y = t;
	seg3[2].x = r-1;	seg3[2].y = t;
	::Polyline(lpdis->hDC, seg3, 3);

	::DeleteObject(::SelectObject(lpdis->hDC, 
		::CreatePen(PS_SOLID, 0, ::GetSysColor(COLOR_3DDKSHADOW))));

	POINT seg4[3];
	seg4[0].x = l+1,	seg4[0].y = b-3;
	seg4[1].x = l+1,	seg4[1].y = t+1;
	seg4[2].x = r-2,	seg4[2].y = t+1;
	::Polyline(lpdis->hDC, seg4, 3);

	::DeleteObject(::SelectObject(lpdis->hDC, oldPen));

	// 4. fill the inside box with the right color
	RECT inbox = {l+2,t+2,r-2,b-2};
	if(lpdis->itemState & ODS_SELECTED)
		::FillRect(lpdis->hDC,  &inbox,	(HBRUSH) m_BackgroundColor->GetBrush() 	);
	else
		::FillRect(lpdis->hDC,  &inbox,	(HBRUSH) ::GetSysColorBrush(COLOR_WINDOW));

	// 5. if it is checked, draw the check
	if(m_checked)
	{
		// 5.1 get the check mark
		HBITMAP	checkmark = libi18n::GetCheckmarkBitmap();
		if(checkmark != NULL)
		{
			// 5.2 set the BkColor and TextColor 
			if(lpdis->itemState & ODS_SELECTED)
				::SetBkColor(lpdis->hDC,  m_BackgroundColor->GetColor() );
			else
				::SetBkColor(lpdis->hDC, ::GetSysColor(COLOR_WINDOW));
			::SetTextColor(lpdis->hDC, m_ForegroundColor->GetColor());
			// 5.3 do a DrawBitmap
			BITMAP bm;
			HDC	hdcMem;
			DWORD	dwSize;
			POINT	ptSize, ptOrg;
			hdcMem = ::CreateCompatibleDC(lpdis->hDC);
			::SelectObject(hdcMem, checkmark);
			::SetMapMode(hdcMem, GetMapMode(lpdis->hDC));
			::GetObject(checkmark, sizeof(BITMAP), (LPVOID)&bm);
			ptSize.x = bm.bmWidth;
			ptSize.y = bm.bmHeight;
			DPtoLP(lpdis->hDC, &ptSize, 1);
			ptOrg.x = 0;
			ptOrg.y = 0;
			DPtoLP(hdcMem, &ptOrg, 1);
			::BitBlt(lpdis->hDC, 
					l+2,t+2,
					ptSize.x, ptSize.y,
					hdcMem, ptOrg.x, ptOrg.y, SRCCOPY);
			::DeleteDC(hdcMem);
		}
	}

	// 6 Daw the text
	this->DrawWin95RadioAndCheckboxText(lpdis);
}
#endif // _WIN32

BOOL AwtButton::DrawWin31Item(UINT idCtl, LPDRAWITEMSTRUCT lpdis)
{
	switch(m_dwButtonType)
	{
	case BS_PUSHBUTTON :	
		DrawWin31Button(lpdis);
		break;
	case BS_RADIOBUTTON :	
		DrawWin31Radio(lpdis);
		break;
	case BS_CHECKBOX :	
		DrawWin31Checkbox(lpdis);
		break;
	default:
		return FALSE;
	}
	return TRUE;
}

//
//	This function draw the Button for Win16 or WIN32 for NT 3.5
//
void AwtButton::DrawWin31Button(LPDRAWITEMSTRUCT lpdis)
{
	// 1. fill out the background rectangle
	::FillRect(	lpdis->hDC,	&lpdis->rcItem,	m_BackgroundColor->GetBrush());

	// 2. draw the four edge with RoundRect
	::SelectObject(	lpdis->hDC,	m_BackgroundColor->GetBrush());
	::RoundRect(lpdis->hDC,	lpdis->rcItem.left ,lpdis->rcItem.top ,
							lpdis->rcItem.right ,lpdis->rcItem.bottom,
							2, 2);

	// 3. draw the shadow of the four edges
	if(lpdis->itemState & ODS_SELECTED)
	{
		// 3.1 in the case of selected 

		HPEN oldPen = (HPEN)::SelectObject(lpdis->hDC, 
			::CreatePen(PS_SOLID, 0, ::GetSysColor(COLOR_BTNSHADOW)));

		POINT seg1[3];
		seg1[0].x = lpdis->rcItem.left+1;	
		seg1[0].y = lpdis->rcItem.bottom-2;
		seg1[1].x = lpdis->rcItem.left+1;	
		seg1[1].y = lpdis->rcItem.top+1;
		seg1[2].x = lpdis->rcItem.right-1;	
		seg1[2].y = lpdis->rcItem.top+1;
		::Polyline(lpdis->hDC, seg1, 3);

		::DeleteObject(::SelectObject(lpdis->hDC, oldPen));
	}
	else
	{
		// 3.2 in the case of normal 
		HPEN oldPen = (HPEN)::SelectObject(lpdis->hDC, 
			::CreatePen(PS_SOLID, 0, ::GetSysColor(COLOR_BTNSHADOW)));

		POINT seg2[3];
		seg2[0].x = lpdis->rcItem.left+1;
		seg2[0].y = lpdis->rcItem.bottom-2;
		seg2[1].x = lpdis->rcItem.right-2;
		seg2[1].y = lpdis->rcItem.bottom-2;
		seg2[2].x = lpdis->rcItem.right-2;
		seg2[2].y = lpdis->rcItem.top;
		::Polyline(lpdis->hDC, seg2, 3);

		POINT seg3[3];
		seg3[0].x = lpdis->rcItem.left+2;	
		seg3[0].y = lpdis->rcItem.bottom-3;
		seg3[1].x = lpdis->rcItem.right-3;
		seg3[1].y = lpdis->rcItem.bottom-3;
		seg3[2].x = lpdis->rcItem.right-3;
		seg3[2].y = lpdis->rcItem.top+1;
		::Polyline(lpdis->hDC, seg3, 3);

		HPEN shadowpen = (HPEN)::SelectObject(lpdis->hDC, 
			::CreatePen(PS_SOLID, 0, ::GetSysColor(COLOR_BTNHIGHLIGHT)));
		::DeleteObject(shadowpen);

		
		POINT seg4[3];
		seg4[0].x = lpdis->rcItem.left+1;   
		seg4[0].y = lpdis->rcItem.bottom-3;
		seg4[1].x = lpdis->rcItem.left+1;
		seg4[1].y = lpdis->rcItem.top+1;
		seg4[2].x = lpdis->rcItem.right-2;
		seg4[2].y = lpdis->rcItem.top+1;
		::Polyline(lpdis->hDC, seg4, 3);

		POINT seg5[3];
		seg5[0].x = lpdis->rcItem.left+2;	
		seg5[0].y = lpdis->rcItem.bottom-4;
		seg5[1].x = lpdis->rcItem.left+2;
		seg5[1].y = lpdis->rcItem.top+2;
		seg5[2].x = lpdis->rcItem.right-3;
		seg5[2].y = lpdis->rcItem.top+2;
		::Polyline(lpdis->hDC, seg5, 3);

		::DeleteObject(::SelectObject(lpdis->hDC, oldPen));
	}

	// 4. draw the text
	AwtFont *pFont = this->GetFont();
	if((m_pLabel != NULL ) && (pFont != NULL ))
	{
		VERIFY(m_pLabel);

		// 4.1 decide the color first
		if(lpdis->itemState & ODS_DISABLED)
			::SetTextColor(lpdis->hDC, ::GetSysColor(COLOR_GRAYTEXT));
		else
			::SetTextColor(lpdis->hDC, m_ForegroundColor->GetColor());
		::SetBkMode(lpdis->hDC, TRANSPARENT);
		::SetBkColor(lpdis->hDC, m_BackgroundColor->GetColor());

		// 4.2 Find teh rect that we should draw the text
		RECT textrect = lpdis->rcItem;
		if(lpdis->itemState & ODS_SELECTED)
			::OffsetRect(&textrect, 1,1);

		// 4.3 draw the text
		m_pLabel->DrawText(lpdis->hDC, pFont ,&textrect, DT_CENTER);

		// 4.4 draw the focus if necessary
		if( (lpdis->itemState & ODS_FOCUS) && 
			!(lpdis->itemState & ODS_DISABLED))
		{
			// 4.4.1 figure out where should we draw the focus base on the size of the text
			SIZE size;
			m_pLabel->GetTextExtent(lpdis->hDC, pFont, &size);
			RECT focus;
			focus.top =  (lpdis->rcItem.top + lpdis->rcItem.bottom - size.cy ) / 2 ;
			focus.bottom = focus.top + size.cy + 1;
			focus.left = ( lpdis->rcItem.right + lpdis->rcItem.left - size.cx ) / 2 - 1;
			focus.right = focus.left + size.cx + 2;
			// 4.4.2 adjust it if it is selected
			if(lpdis->itemState & ODS_SELECTED)
				::OffsetRect(&focus, 1,1);
			// 4.4.3 draw the rect
			::DrawFocusRect(lpdis->hDC, &focus);
		}
	}
}
void AwtButton::DrawWin31RadioAndCheckboxText(LPDRAWITEMSTRUCT lpdis)
{
	AwtFont *pFont = this->GetFont();
	if((m_pLabel != NULL ) && (pFont != NULL ))
	{
		VERIFY(m_pLabel);
		// 1 set up the color
		if(lpdis->itemState & ODS_DISABLED)
			::SetTextColor(lpdis->hDC, ::GetSysColor(COLOR_GRAYTEXT));
		else
			::SetTextColor(lpdis->hDC, m_ForegroundColor->GetColor());
		::SetBkMode(lpdis->hDC, TRANSPARENT);
		::SetBkColor(lpdis->hDC, m_BackgroundColor->GetColor());

		// 2 figure out where to draw
		RECT textrect = lpdis->rcItem;
		textrect.left += 18;

		// 3 draw the text
		m_pLabel->DrawText(lpdis->hDC, pFont ,&textrect, DT_LEFT);

		// 4 draw the focus box if necessary
		if(lpdis->itemState & ODS_FOCUS)
		{
			// 4.1 decide the focus rect base on the size of the text
			SIZE size;
			m_pLabel->GetTextExtent(lpdis->hDC, pFont, &size);

			RECT focus;
			focus.top =  (lpdis->rcItem.top + lpdis->rcItem.bottom - size.cy ) / 2 - 1;
			focus.bottom = focus.top + size.cy + 2;
			focus.left = lpdis->rcItem.left + 18 - 1;
			focus.right = focus.left + size.cx + 2;

			// 4.2 draw the rectangle
			::DrawFocusRect(lpdis->hDC, &focus);
		}
	}
}
//
//	This function draw the Radio for Win16 or WIN32 for NT 3.5
//
void AwtButton::DrawWin31Radio(LPDRAWITEMSTRUCT lpdis)
{
	// 1. fill out the background rectangle
	::FillRect(	lpdis->hDC,	&lpdis->rcItem,	m_BackgroundColor->GetBrush());

	// 2. figure out where for the bullet
	int top = ( lpdis->rcItem.top + lpdis->rcItem.bottom ) / 2 - 6;

	// 3. draw the outter bullet
	::SelectObject(	lpdis->hDC,	m_BackgroundColor->GetBrush());
	::Ellipse(lpdis->hDC,	lpdis->rcItem.left,	top,
							lpdis->rcItem.left + 13,	top+13);	
	// 4. draw the inner bullet if selected
	if(lpdis->itemState & ODS_SELECTED)
	{
		::Ellipse(lpdis->hDC,	lpdis->rcItem.left + 1 ,	top + 1,
								lpdis->rcItem.left + 12 ,	top+12);
	}
	// 5. draw the dot if checked
	if(m_checked)
	{
		::SelectObject(	lpdis->hDC,	m_ForegroundColor->GetBrush());

		::Ellipse(lpdis->hDC,	lpdis->rcItem.left + 3 ,		top + 3,
								lpdis->rcItem.left + 10,	top+10);
	}
	// 6. draw the text
	this->DrawWin31RadioAndCheckboxText(lpdis);
}
//
//	This function draw the Checkbox for Win16 or WIN32 for NT 3.5
//
void AwtButton::DrawWin31Checkbox(LPDRAWITEMSTRUCT lpdis)
{
	// 1. fill out the background rectangle
	::FillRect(	lpdis->hDC,	&lpdis->rcItem,	m_BackgroundColor->GetBrush());

	// 2. figure out where for the bullet
	int top = ( lpdis->rcItem.top + lpdis->rcItem.bottom ) / 2 - 6;

	// 3. draw the outter box
	::SelectObject(	lpdis->hDC,	m_BackgroundColor->GetBrush());
	::Rectangle(lpdis->hDC,	lpdis->rcItem.left,	top,
							lpdis->rcItem.left + 13,	top+13);	
	// 4. draw the inner box
	if(lpdis->itemState & ODS_SELECTED)
		::Rectangle(lpdis->hDC,	lpdis->rcItem.left + 1 ,		top + 1,
								lpdis->rcItem.left + 12,	top+12);	
	// 5. draw the check
	if(m_checked)
	{
		::MoveToEx(lpdis->hDC,	lpdis->rcItem.left	, top,		NULL);
		::LineTo(lpdis->hDC,	lpdis->rcItem.left	+ 13, top+13);
		::MoveToEx(lpdis->hDC,	lpdis->rcItem.left	, top+12 ,	NULL);
		::LineTo(lpdis->hDC,	lpdis->rcItem.left	+ 12, top);
	}
	// 6. draw the text
	this->DrawWin31RadioAndCheckboxText(lpdis);
}
BOOL AwtButton::OnDrawItem(UINT idCtl, LPDRAWITEMSTRUCT lpdis)
{
#ifdef _WIN32
	if(libi18n::iswin95())
			return DrawWin95Item(idCtl, lpdis);
#endif	// _WIN32

	return DrawWin31Item(idCtl, lpdis);
}
BOOL AwtButton::OnDeleteItem(UINT idCtl, LPDELETEITEMSTRUCT lpdis)
{
	return FALSE;
}

#endif	// INTLUNICODE

//-------------------------------------------------------------------------
//
// Capture the mouse (if it's not the left button) so that mouse events 
// get re-direct to the proper component. 
// This allow MouseDrag to work correctly and MouseUp to be sent to the 
// component which received the mouse down.
// Left down is a special case because windows needs to handle that.
//
//-------------------------------------------------------------------------
void AwtButton::CaptureMouse(AwtButtonType button, UINT metakeys)
{
    // capture the mouse if it was not captured already.
    // This may happen because of a previous call to CaptureMouse
    // or because the left mouse was clicked and windows captured
    // the mouse for us.
    // Here we use m_nCaptured with a different semantic.
    // It's setted to 1 if SetCapture gets called, otherwise is 0
    if (button == AwtLeftButton)
        m_nCaptured = 0;
    else if (!(metakeys & MK_LBUTTON) && !m_nCaptured) {
        ::SetCapture(m_hWnd);
        m_nCaptured = 1;
    }
}


//-------------------------------------------------------------------------
//
// Release the mouse if it was captured previously. We might not have 
// captured the mouse at all if the left button is the first to be pressed
// and the last released
//
//-------------------------------------------------------------------------
BOOL AwtButton::ReleaseMouse(AwtButtonType button, UINT metakeys, int x, int y)
{
    // if at least the left button is pressed don't bother...
    if (!(metakeys & MK_LBUTTON)) {

        // check if no button is pressed and if we called SetCapture release it
        if (!(metakeys & MK_RBUTTON) && !(metakeys & MK_MBUTTON)) {
            if (m_nCaptured) {
                ::ReleaseCapture();
                m_nCaptured = 0;
            }
        }

        // the left button has just been released...
        else if (button == AwtLeftButton) {
            // ...see if there is any other button pressed...
            if ((metakeys & MK_RBUTTON) || (metakeys & MK_MBUTTON)) {
                    // Windows is giving up the mouse capture, it's time to take it. 
                    // Let the defproc do MouseUp first so it will release the mouse 
                    // before we capture otherwise WM_LBUTTONUP will release our capture
                    ::CallWindowProc(m_PrevWndProc, 
                                        m_hWnd, 
                                        WM_LBUTTONUP, 
                                        (WPARAM)metakeys, 
                                        MAKELPARAM(x, y));
                    ::SetCapture(m_hWnd);
                    m_nCaptured = 1;
                    return TRUE;
            }
        }

    }

    return FALSE;
}


//-------------------------------------------------------------------------
//
// Native method implementations for sun.awt.WButtonPeer
//
//-------------------------------------------------------------------------

extern "C" {

void
sun_awt_windows_WButtonPeer_create(HWButtonPeer *self, 
                                   HWComponentPeer *hParent)
{
    AwtButton    *pNewButton;

    CHECK_NULL(self,    "ButtonPeer is null.");
    CHECK_NULL(hParent, "ComponentPeer is null.");

    pNewButton = new AwtButton(self);
    CHECK_ALLOCATION(pNewButton);

// What about locking the new object??
    pNewButton->CreateWidget( pNewButton->GetParentWindowHandle(hParent) );
}


void
sun_awt_windows_WButtonPeer_setLabel(HWButtonPeer *self, 
                                     Hjava_lang_String *string)
{
    CHECK_PEER(self);
    if(string == NULL) //Do not use CHECK_NULL_AND_RETURN as this is
    {                  //a error condition to break to debugger
       return;
    }

    AwtButton *obj = GET_OBJ_FROM_PEER(AwtButton, self);

    obj->SetLabel(string);
}


void 
sun_awt_windows_WCheckboxPeer_create(HWCheckboxPeer* self,
                                     HWComponentPeer * hParent)
{
    AwtButton *pNewCheckbox;

    CHECK_NULL(self,    "ButtonPeer is null.");
    CHECK_NULL(hParent, "ComponentPeer is null.");

    pNewCheckbox = new AwtButton(self);
    CHECK_ALLOCATION(pNewCheckbox);

// What about locking the new object??
    pNewCheckbox->CreateWidget( pNewCheckbox->GetParentWindowHandle(hParent) );
}


void 
sun_awt_windows_WCheckboxPeer_setLabel(HWCheckboxPeer* self,
                                       Hjava_lang_String * string)
{
    CHECK_PEER(self);
    if(string == NULL) //Do not use CHECK_NULL_AND_RETURN as this is
    {                  //a error condition to break to debugger
       return;
    }

    AwtButton *obj = GET_OBJ_FROM_PEER(AwtButton, self);

    obj->SetLabel(string);
}


void 
sun_awt_windows_WCheckboxPeer_setState(HWCheckboxPeer* self,
                                      /*boolean*/ long lState)
{
    CHECK_PEER(self);

    AwtButton *obj = GET_OBJ_FROM_PEER(AwtButton, self);

    obj->SetState(lState);
}


void 
sun_awt_windows_WCheckboxPeer_setCheckboxGroup(HWCheckboxPeer* self,
                                               Hjava_awt_CheckboxGroup *)
{
    // not implemented!

    //CHECK_PEER(self);

    //AwtButton *obj = GET_OBJ_FROM_PEER(AwtButton, self);

    //obj->SetCheckboxGroup();
}


};  // end of extern "C"
