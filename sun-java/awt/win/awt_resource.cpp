#include <stddef.h>     // offsetof()

#include "awt_object.h"
#include "awt_resource.h"

#ifdef DEBUG_CONSOLE
#include "awt_toolkit.h"
#endif

//#define DEBUG_GDI


//-------------------------------------------------------------------------
//
// AwtCachedBrush implementation
//
//-------------------------------------------------------------------------
AwtCList AwtCachedBrush::m_freeChain;
AwtCList AwtCachedBrush::m_liveChain;
AwtMutex AwtCachedBrush::m_cacheMutex;


AwtCachedBrush::AwtCachedBrush()
{
}

AwtCachedBrush::~AwtCachedBrush()
{
}

//-------------------------------------------------------------------------
//
// Create an AwtCachedBrush object of the requested color.
//
// If an object already exists for the color, then return it, otherwise
// create a new one...
//
//-------------------------------------------------------------------------
AwtCachedBrush *AwtCachedBrush::Create( COLORREF color ) 
{
    AwtCList *qp;
    AwtCachedBrush *pBrush;
    AwtCachedBrush *pNewBrush = NULL;


    // Lock the brush cache...
    m_cacheMutex.Lock();
    
    //
    // Scan the list of allocated brushes to see if the requested color
    // has already been created...
    //
    qp = m_liveChain.next;
    while (qp != &m_liveChain) {
        pBrush = (AwtCachedBrush *)RESOURCE_PTR(qp);
        if (pBrush->m_color == color) {
            pBrush->m_refCount++;
            pNewBrush = pBrush;
            break;
        }
        qp = qp->next;
    }

    //
    // Allocate a new brush
    //
    if (pNewBrush == NULL) {
        // Allocate from the free chain if possible...
        if (m_freeChain.IsEmpty()) {
            pNewBrush = new AwtCachedBrush();
        } else {
            pNewBrush = (AwtCachedBrush *)RESOURCE_PTR( m_freeChain.next );
            pNewBrush->m_link.Remove();
        }

        //
        // Initialize the object and place it in the brush cache...
        //
        pNewBrush->Initialize(color);
        m_liveChain.Append(pNewBrush->m_link);
    }

    // Unlock the brush cache...
    m_cacheMutex.Unlock();

    return pNewBrush;
}


//-------------------------------------------------------------------------
//
// Delete all of the objects in the cache...
//
//-------------------------------------------------------------------------
void AwtCachedBrush::DeleteCache(void)
{
    AwtCachedBrush *pObject;


    // Lock the cache...
    m_cacheMutex.Lock();
    
    //
    // Scan the list of allocated brushes to see if the requested color
    // has already been created...
    //
    while (!m_liveChain.IsEmpty()) {
        pObject = (AwtCachedBrush *)RESOURCE_PTR(m_liveChain.next);
        pObject->m_link.Remove();

        // Release the GDI object
        if (pObject->m_hObject) {
            VERIFY( ::DeleteObject(pObject->m_hObject) );
        }

        delete pObject;
    }

    // Unlock the cache...
    m_cacheMutex.Unlock();
}


//-------------------------------------------------------------------------
//
// Select or remove a brush in a GDI DC
//
//-------------------------------------------------------------------------
void AwtCachedBrush::Select( HDC hDC, BOOL bFlag )
{
    // Lock the brush cache...
    m_cacheMutex.Lock();

    if (bFlag) {
        VERIFY( ::SelectObject(hDC, m_hObject) );
        m_lockCount++;
    }
    else {
        --m_lockCount;
    }

    // Lock the brush cache...
    m_cacheMutex.Unlock();
}


//------------------------------------------------------------------------
//
//------------------------------------------------------------------------
void AwtCachedBrush::AddRef()
{
    // Lock the brush cache...
    m_cacheMutex.Lock();

    m_refCount++;

    // Unlock the brush cache...
    m_cacheMutex.Unlock();
}


//-------------------------------------------------------------------------
//
// Release an AwtCachedBrush object.  If this is the last reference to the
// object, then free all native resources (ie. GDI) and place the object
// on the free chain.
//
//-------------------------------------------------------------------------
void AwtCachedBrush::Release( void )
{
    // Lock the brush cache...
    m_cacheMutex.Lock();

    if (--m_refCount == 0) {
#ifdef DEBUG_GDI
        char buf[100];
        wsprintf(buf, "Delete Brush = 0x%x, thrid=%d\n",m_hObject, PR_GetCurrentThreadID() );
        OutputDebugString(buf);
#endif

        VERIFY( ::DeleteObject(m_hObject) );
        m_hObject = 0;

        //
        // Remove the object from the cache and place it on the free chain.
        //
        m_link.Remove();
        m_freeChain.Append(m_link);
    }

    // Unlock the brush cache...
    m_cacheMutex.Unlock();
}


//-------------------------------------------------------------------------
//
// Initialize either a new object or one just removed from the free chain
// The reference count is initially 1
//
//-------------------------------------------------------------------------
void AwtCachedBrush::Initialize( COLORREF color )
{
    AwtCachedResource::Initialize();

    m_color = color;
    VERIFY( m_hObject = ::CreateSolidBrush(color) );
#ifdef DEBUG_GDI
    char buf[100];
    wsprintf(buf, "Create Brush = 0x%x, thrid=%d\n",m_hObject, PR_GetCurrentThreadID() );
    OutputDebugString(buf);
#endif
}





//-------------------------------------------------------------------------
//
// AwtCachedPen implementation
//
//-------------------------------------------------------------------------
AwtCList AwtCachedPen::m_freeChain;
AwtCList AwtCachedPen::m_liveChain;
AwtMutex AwtCachedPen::m_cacheMutex;


AwtCachedPen::AwtCachedPen()
{
}

AwtCachedPen::~AwtCachedPen()
{
}

AwtCachedPen *AwtCachedPen::Create( COLORREF color ) 
{
    AwtCList *qp;
    AwtCachedPen *pPen;
    AwtCachedPen *pNewPen = NULL;

    // Lock the pen cache...
    m_cacheMutex.Lock();
    
    //
    // Scan the list of allocated pens to see if the requested color
    // has already been created...
    //
    qp = m_liveChain.next;
    while (qp != &m_liveChain) {
        pPen = (AwtCachedPen *)RESOURCE_PTR(qp);
        if(pPen->m_color == color) {
            pPen->m_refCount++;
            pNewPen = pPen;
            break;
        }
        qp = qp->next;
    }

    //
    // Allocate a new pen
    //
    if (pNewPen == NULL) {
        // Allocate from the free chain if possible...
        if (m_freeChain.IsEmpty()) {
            pNewPen = new AwtCachedPen();
        } else {
            pNewPen = (AwtCachedPen *)RESOURCE_PTR( m_freeChain.next );
            pNewPen->m_link.Remove();
        }

        pNewPen->Initialize(color);
        m_liveChain.Append(pNewPen->m_link);
    }

    // Unlock the pen cache...
    m_cacheMutex.Unlock();

    return pNewPen;
}


//-------------------------------------------------------------------------
//
// Delete all of the objects in the cache...
//
//-------------------------------------------------------------------------
void AwtCachedPen::DeleteCache(void)
{
    AwtCachedPen *pObject;


    // Lock the cache...
    m_cacheMutex.Lock();

    // Empty out the cache...
    while (!m_liveChain.IsEmpty()) {
        pObject = (AwtCachedPen *)RESOURCE_PTR(m_liveChain.next);
        pObject->m_link.Remove();

        // Release the GDI object
        if (pObject->m_hObject) {
            VERIFY( ::DeleteObject(pObject->m_hObject) );
        }

        delete pObject;
    }

    // Unlock the cache...
    m_cacheMutex.Unlock();
}

//-------------------------------------------------------------------------
//
// Select or remove a pen in a GDI DC
//
//-------------------------------------------------------------------------
void AwtCachedPen::Select( HDC hDC, BOOL bFlag )
{
    if (bFlag) {
        VERIFY( ::SelectObject(hDC, m_hObject) );
        m_lockCount++;
    }
    else {
        --m_lockCount;
    }
}


//------------------------------------------------------------------------
//
//------------------------------------------------------------------------
void AwtCachedPen::AddRef()
{
    // Lock the brush cache...
    m_cacheMutex.Lock();

    m_refCount++;

    // Unlock the brush cache...
    m_cacheMutex.Unlock();
}


//-------------------------------------------------------------------------
//
// Release an AwtCachedPen object.  If this is the last reference to the
// object, then free all native resources (ie. GDI) and place the object
// on the free chain.
//
//-------------------------------------------------------------------------
void AwtCachedPen::Release( void )
{
    m_cacheMutex.Lock();
    if (--m_refCount == 0) {
#ifdef DEBUG_GDI
        char buf[100];
        wsprintf(buf, "Delete Pen = 0x%x, thrid=%d\n",m_hObject, PR_GetCurrentThreadID() );
        OutputDebugString(buf);
#endif

        VERIFY( ::DeleteObject(m_hObject) );
        m_hObject = 0;

        //
        // Remove the object from the cache and place it on the free chain.
        //
        m_link.Remove();
        m_freeChain.Append(m_link);

    }
    m_cacheMutex.Unlock();
}


//-------------------------------------------------------------------------
//
// Initialize either a new object or one just removed from the free chain
// The reference count is initially 1
//
//-------------------------------------------------------------------------
void AwtCachedPen::Initialize( COLORREF color )
{
    AwtCachedResource::Initialize();

    m_color = color;
    VERIFY( m_hObject = ::CreatePen(PS_SOLID, 1, color) );
#ifdef DEBUG_GDI
    char buf[100];
    wsprintf(buf, "Create Pen = 0x%x, thrid=%d\n",m_hObject, PR_GetCurrentThreadID() );
    OutputDebugString(buf);
#endif
}



