#include "awt_dc.h"
#include "awt_querydc.h"
#include "awt_component.h"
#include "awt_graphics.h"
#include "awt_resource.h"
#include "awt_font.h"

#ifdef DEBUG_CONSOLE
#include "awt_toolkit.h"
#endif

//#define DEBUG_GDI

AwtMutex AwtDC::AwtDCLock;

AwtCList AwtDC::m_freeChain;
AwtCList AwtDC::m_liveChain;


//-------------------------------------------------------------------------
//
// Create an AwtDC object. Check in the list of allocated to see if 
// some available.
// This is the only way to get a DC object.
//
//-------------------------------------------------------------------------
AwtDC *AwtDC::Create(IDCService* pAwtObj)
{
    AwtDC *pCachedNewAwtDC = NULL;

    // ask the object if an AwtDC is around, if so that's the one 
    // we'll return. No creation involved. Just AddRef it.
    pCachedNewAwtDC = pAwtObj->GetAwtDC();

    if (pCachedNewAwtDC) 
        pCachedNewAwtDC->m_refCount++;

    // Allocate a new AwtDC in case it is not available
    else
    {
        // protect the list from multiple access
        AwtDCLock.Lock();

        // Allocate from the free chain if possible...
        if (m_freeChain.next != m_freeChain.prev) 
        {
            pCachedNewAwtDC = (AwtDC *)RESOURCE_PTR( m_freeChain.next );
            pCachedNewAwtDC->m_link.Remove();
        } 
        else 
        {
            // REMIND:
            // If the AwtObject is a Component, then verify that
            // a window handle is available before
            // creating the new AwtDC object...
            pCachedNewAwtDC = new AwtDC();
        }

        pCachedNewAwtDC->Initialize(pAwtObj);
        // place the object in the cache...
        m_liveChain.Append(pCachedNewAwtDC->m_link);

        AwtDCLock.Unlock();
    }

    return pCachedNewAwtDC;
}


//------------------------------------------------------------------------
//
// Create a dc given an owner component 
//
//------------------------------------------------------------------------
AwtDC::AwtDC()
{
    m_hObject           = NULL;
    m_hClipRegion       = NULL;

    m_pTargetObj        = NULL;
    m_pCurrentOwner     = 0;
    m_hOldPalette       = NULL;
}


//------------------------------------------------------------------------
// destructor
//------------------------------------------------------------------------
AwtDC::~AwtDC()
{
}


//------------------------------------------------------------------------
//
// initialize a dc 
//
//------------------------------------------------------------------------
void AwtDC::Initialize(IDCService *target)
{

    // initialize base
    AwtCachedResource::Initialize();

    m_hObject           = NULL;

    // create a clipping region for this dc 
    m_hClipRegion = ::CreateRectRgn(0,0,0,0);

    // connect to the owner object
    m_pTargetObj        = target;
    m_pTargetObj->Attach(this);

    // no graphic object owns this dc yet
    // Graphic ownership is done through Lock(AwtGraphics*)
    m_pCurrentOwner     = 0;
}


//------------------------------------------------------------------------
//
//------------------------------------------------------------------------
void AwtDC::AddRef()
{
    m_dc_lock.Lock();

    m_refCount++;

    m_dc_lock.Unlock();
}


//------------------------------------------------------------------------
//
//------------------------------------------------------------------------
void AwtDC::Release()
{
    UINT nRef;

    m_dc_lock.Lock();

    nRef = --m_refCount;

    // release resources so any attempt to delete gdi object won't complain
    if (m_hObject) {

        VERIFY(::SelectObject((HDC)m_hObject, ::GetStockObject(NULL_PEN)));
        VERIFY(::SelectObject((HDC)m_hObject, ::GetStockObject(NULL_BRUSH)));
        VERIFY(::SelectObject((HDC)m_hObject, ::GetStockObject(DEVICE_DEFAULT_FONT)));
    }

    if (nRef == 0) 
    {
        // delete the clipping region
		ASSERT(m_hClipRegion);
		VERIFY(::DeleteObject(m_hClipRegion));
        m_hClipRegion = NULL;

        // inform "component" we are going away
        // this should never happen because an owner component holds a reference on us
        ASSERT(m_pTargetObj == NULL);
        if (m_pTargetObj)
            m_pTargetObj->Detach((HDC)m_hObject);

        //
        // Remove the object from the cache and place it on the free chain.
        //
        m_link.Remove();
        m_freeChain.Append(m_link);
    }

    m_dc_lock.Unlock();
}


//------------------------------------------------------------------------
//
// Lock the AwtDC for exclusive use by the given AwtGraphics object.
// 
//------------------------------------------------------------------------
HDC AwtDC::Lock( AwtGraphics  *p )
{
    m_dc_lock.Lock();

    if (m_pTargetObj != NULL) {
        // get a DC if not yet available
        if (m_hObject == NULL) {
            // get a dc from the "owner" of the graphics
            m_hObject = m_pTargetObj->GetDC();
        }

        if (p) {
            ASSERT (m_hClipRegion);

            LPRECT pClip = p->GetClipRect();
            ::SetRectRgn(m_hClipRegion, 
                         pClip->left, 
                         pClip->top, 
                         pClip->right, 
                         pClip->bottom);
            VERIFY(::SelectClipRgn((HDC)m_hObject, m_hClipRegion) != ERROR);

            // select the graphics GDI objects into the dc if
            // this graphic does not own the dc object.
            if(p != m_pCurrentOwner) 
            {
                m_pCurrentOwner = p; // change ownership
                MapGraphicsToDC(p);
            }
        }
    }

    return (HDC)m_hObject;
}


//------------------------------------------------------------------------
//
// Unlock the AwtDC.  This allows any other thread to obtain a Lock.
//
//------------------------------------------------------------------------
void AwtDC::Unlock( AwtGraphics  *p )
{
    ::GdiFlush();

    // There are three operations the component owning the dc may decide to do
    // on the dc object
    // 1. ::ReleaseDC(), depends if the dc is coming from the windows cache or not
    // 2. AwtDC::ResetDC(), next call to Lock() will call GetDC()
    // 3. AwtDC::ResetOwnerGraphics(), next call to Lock() will push all the
    //    gdi objects on the dc ( MapGraphicsToDC() )
    // The target object decide if we need to reset our data members or not
    // because he is the only one who knows
    // reset the clipping region
    if (m_hObject)
        ::SelectClipRgn((HDC)m_hObject, NULL);

    if (m_pTargetObj)
        m_pTargetObj->ReleaseDC(this);

    m_dc_lock.Unlock();

}


//------------------------------------------------------------------------
//
// Lock and Unnlock the AwtDC.  This allows any other thread to obtain a Lock.
// This will not map into DC but just lock DC.
//
//------------------------------------------------------------------------
void AwtDC::Lock( )
{
    m_dc_lock.Lock();
}


void AwtDC::Unlock( )
{
    m_dc_lock.Unlock();
}


//------------------------------------------------------------------------
//
// Set the specified pen into this DC if the graphics own the dc,
// otherwise ignore the call. 
// If pPen is null force a NULL_PEN to be selected
//
//------------------------------------------------------------------------
void AwtDC::SetPen(AwtGraphics *p, AwtCachedPen* pPen)
{
    if (p == m_pCurrentOwner && m_hObject) {
        m_dc_lock.Lock();

        if (pPen)
            VERIFY(::SelectObject((HDC)m_hObject, pPen->GetPen()));
        else
            VERIFY(::SelectObject((HDC)m_hObject, ::GetStockObject(NULL_PEN)));

        m_dc_lock.Unlock();
    }
}


IDCService * AwtDC::GetTargetObject() 
{
    return m_pTargetObj;
}

void AwtDC::ResetTargetObject() 
{ 
    m_dc_lock.Lock();
    m_pTargetObj = 0; 
    m_dc_lock.Unlock();
}

HDC AwtDC::GetDC(void) 
{
    return (HDC)m_hObject;
}

void AwtDC::ResetDC() 
{
    m_dc_lock.Lock();
    m_hObject = 0;
    m_dc_lock.Unlock();
}

void AwtDC::ResetOwnerGraphics() 
{ 
    m_dc_lock.Lock();
    m_pCurrentOwner = 0; 
    m_dc_lock.Unlock();
}

HBRUSH AwtDC::GetComponentBkgnd()
{
    return m_pTargetObj->GetBackground();
}
//------------------------------------------------------------------------
//
// Set the specified brush into this DC if the graphics own the dc,
// otherwise ignore the call
// If pBrush is null force a null brush to be selected
//
//------------------------------------------------------------------------
void AwtDC::SetBrush(AwtGraphics *p, AwtCachedBrush* pBrush)
{
    if (p == m_pCurrentOwner && m_hObject) {
        m_dc_lock.Lock();

        if (pBrush)
            VERIFY(::SelectObject((HDC)m_hObject, pBrush->GetBrush()));
        else
            VERIFY(::SelectObject((HDC)m_hObject, ::GetStockObject(NULL_BRUSH)));
    
        m_dc_lock.Unlock();
    }
}


//-------------------------------------------------------------------------
//
// Set the specified font into this DC if the graphics own the dc,
// otherwise ignore the call
// If pFont is null force a default font to be selected
//
//-------------------------------------------------------------------------
void AwtDC::SetFont(AwtGraphics *p, AwtFont* pFont)
{
    if (m_pCurrentOwner == p && m_hObject) {
        m_dc_lock.Lock();

        if (pFont)
            VERIFY(::SelectObject((HDC)m_hObject, pFont->GetFont()));
        else
            VERIFY(::SelectObject((HDC)m_hObject, ::GetStockObject(DEVICE_DEFAULT_FONT)));

        m_dc_lock.Unlock();
    }
}


//-------------------------------------------------------------------------
//
// Set the specified text color into this DC if the graphics own the dc,
// otherwise ignore the call
//
//-------------------------------------------------------------------------
void AwtDC::SetTxtColor(AwtGraphics *p, COLORREF color)
{
    if (m_pCurrentOwner == p && m_hObject) {
        m_dc_lock.Lock();

        ::SetTextColor((HDC)m_hObject, color);

        m_dc_lock.Unlock();
    }
}


//-------------------------------------------------------------------------
//
// Set the specified raster operation into this DC if the graphics own the dc,
// otherwise ignore the call
//
//-------------------------------------------------------------------------
void AwtDC::SetRop(AwtGraphics *p, DWORD nROPMode)
{
    if (m_pCurrentOwner == p && m_hObject) {
        m_dc_lock.Lock();

        VERIFY(::SetROP2((HDC)m_hObject, nROPMode));

        m_dc_lock.Unlock();
    }
}


//-------------------------------------------------------------------------
//
// Set the specified region into this DC if the graphics own the dc,
// otherwise ignore the call
//
// We deserve the electric chair for this but...you know...who is going
// to tell to Jean-Charles we need another re-write...so, in the image 
// case we are calling SetRgn with NULL for the AwtGraphics in order to
// set the clipping region knowing m_pCurrentOwner is NULL (so we pass the if)
//
//-------------------------------------------------------------------------
void AwtDC::SetRgn(AwtGraphics *p, LPRECT pClip)
{
    if (m_pCurrentOwner == p && m_hObject) {
        m_dc_lock.Lock();

        ::SetRectRgn(m_hClipRegion, 
                     pClip->left, 
                     pClip->top, 
                     pClip->right, 
                     pClip->bottom);
        VERIFY(::SelectClipRgn((HDC)m_hObject, m_hClipRegion) != ERROR);

        m_dc_lock.Unlock();

#ifdef DEBUG_CONSOLE
        AwtToolkit::m_pDebug->Log("[%X] Clipping Region (%d, %d, %d, %d)\r\n", 
                                                        p,
                                                        pClip->left, 
                                                        pClip->top, 
                                                        pClip->right, 
                                                        pClip->bottom);
#endif

    }
}


//-------------------------------------------------------------------------
//
// Set the specified background mode into this DC if the graphics own the dc,
// otherwise ignore the call
//
//-------------------------------------------------------------------------
void AwtDC::ResetBkMode(AwtGraphics *p)
{
    if (m_pCurrentOwner == p && m_hObject) {
        m_dc_lock.Lock();

        VERIFY(::SetBkMode((HDC)m_hObject, TRANSPARENT) != NULL);

        m_dc_lock.Unlock();
    }
}


//-------------------------------------------------------------------------
//
// Set the specified palette into this DC if the graphics own the dc,
// otherwise ignore the call
//
//-------------------------------------------------------------------------
void AwtDC::ResetPalette(AwtGraphics *p)
{
    if (m_pCurrentOwner == p) {
        m_dc_lock.Lock();

        VERIFY( ::SelectObject((HDC)m_hObject, ::GetStockObject(DEFAULT_PALETTE)));

        m_dc_lock.Unlock();
    }
}


//------------------------------------------------------------------------
//
// Call InvalidateRgn on the specified rgn
//
//------------------------------------------------------------------------
void AwtDC::InvalidateRgn(HRGN hRgn) 
{
    HWND hwnd = m_pTargetObj->GetWindowHandle();
    if (hwnd)
        ::InvalidateRgn(m_pTargetObj->GetWindowHandle(), hRgn, FALSE);
}


//------------------------------------------------------------------------
//
// Map the clip region, pen, brush, font, etc for the given AwtGraphics
// object into the GDI device context.
//
// This method MUST be called inside of the m_dc_lock
//
//------------------------------------------------------------------------
void AwtDC::MapGraphicsToDC(AwtGraphics *p)
{
    if (m_hObject) {
        // set text color
        ::SetTextColor((HDC)m_hObject, p->GetColor());

        //REMIND: Does this change anytime?
        // set background mode
        VERIFY(::SetBkMode((HDC)m_hObject, TRANSPARENT) != NULL);

        // select pen
        VERIFY(::SelectObject((HDC)m_hObject, p->GetPen()));

        // select brush
        VERIFY(::SelectObject((HDC)m_hObject, p->GetBrush()));

        // select font
        VERIFY(::SelectObject((HDC)m_hObject, p->GetFont()));

        // select palette
        VERIFY(m_hOldPalette = ::SelectPalette((HDC)m_hObject, p->GetPalette(), TRUE) );
        ::RealizePalette((HDC)m_hObject);

        // select raster operation
        VERIFY(::SetROP2((HDC)m_hObject, p->GetROP2()));
        }
}


#ifdef DEBUG_CONSOLE

char* AwtDC::PrintStatus()
{
    char* buffer = new char[1024];

    wsprintf(buffer, "DC Status:\r\n\tInitialized = %s\r\n\tRegion = %d\r\n\tDC = %d\r\n",
                (m_IsInitialized) ? "TRUE" : "FALSE",
                m_hClipRegion,
                (HDC)m_hObject);

    return buffer;
}

#endif
