#ifndef AWT_DC_H
#define AWT_DC_H

#include "awt_defs.h"
#include "awt_resource.h"
#include "awt_object.h"
#include "awt_clist.h"

#if !defined(_AWT_IDCSERVICE_H)
#include "awt_IDCService.h"
#endif


//
// Forward class declarations
//
class AwtGraphics;
class AwtComponent;
class AwtFont;



class AwtDC : public AwtCachedResource
{
public:
        static  AwtDC      *Create(IDCService* pAwtObj);
        
                            ~AwtDC();

        void                AddRef();
        void                Release();

        HDC                 Lock  (AwtGraphics *p);
        void                Unlock(AwtGraphics *p);
        void                Lock  ();
        void                Unlock();

        void                SetPen(AwtGraphics *p, AwtCachedPen* pPen);
        void                SetBrush(AwtGraphics *p, AwtCachedBrush* pBrush);
        void                SetFont(AwtGraphics *p, AwtFont* pFont);
        void                SetTxtColor(AwtGraphics *p, COLORREF color);
        void                SetRop(AwtGraphics *p, DWORD nROPMode);
        void                SetRgn(AwtGraphics *p, LPRECT pClip);

        void                ResetPalette(AwtGraphics *p);
        void                ResetBkMode(AwtGraphics *p);

        // deal with data members
        IDCService         *GetTargetObject();
        void                ResetTargetObject();

        HDC                 GetDC(void);
        void                ResetDC();

        void                ResetOwnerGraphics();

        HBRUSH              GetComponentBkgnd();

        void                InvalidateRgn(HRGN hRgn);


#ifdef DEBUG_CONSOLE
    char* PrintStatus();
#endif


private:
        // Private constructor forces the object to be on heap and hence 
        // our cache
                            AwtDC();
        void                MapGraphicsToDC(AwtGraphics *p);

        void                Initialize(IDCService *target);


        static AwtMutex  AwtDCLock;

        static AwtCList  m_freeChain;
        static AwtCList  m_liveChain;



    //
    // Data members:
    //
private:
    AwtMutex        m_dc_lock;

    AwtGraphics*    m_pCurrentOwner; // current graphics using the dc
    IDCService*     m_pTargetObj; // owner

    HRGN            m_hClipRegion;
    HPALETTE        m_hOldPalette;
};


#endif

