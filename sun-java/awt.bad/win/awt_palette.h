#ifndef AWT_PALETTE_H
#define AWT_PALETTE_H

#include "awt_defs.h"
#include "awt_resource.h"
#include "awt_object.h"
#include "awt_clist.h"

#define CMAP_BITS 3L
#define CMAP_COLS (1L << CMAP_BITS)
#define CMAP_BIAS (1L << (7L - CMAP_BITS))


//
// Forward class declarations
//
class AwtGraphics;
class AwtComponent;
class AwtDC;

DECLARE_JAVA_AWT_IMAGE_PEER(ColorModel);
DECLARE_JAVA_AWT_IMAGE_PEER(IndexColorModel);


class AwtPalette : public AwtCachedResource
{
public:
        static  AwtPalette      *Create(HColorModel  *pCM);
        static  AwtPalette      *Create(HDC hDC);
        static  AwtPalette      *Create(HPALETTE hPal);

        static  void            DeleteCache(void);
        
                            ~AwtPalette();

        void                AddRef();
        void                Release();
        void                Lock  ();
        void                Unlock();
        void                ReleaseCachedAwtPalette();
        void                CreateCompatiblePalette(HDC hDC);
        BOOL                CreateCompatiblePalette(HPALETTE hPal);
        HPALETTE            GetPalette(void){return m_hPal;}
        PALETTEENTRY       *GetPaletteEntries();
        void                UpdatePaletteEntries(int startInx, int endInx, PALETTEENTRY *newEntries);
        void                InitPaletteEntry(PALETTEENTRY *pPalEntry, int r, int g, int b);
        void                FillPaletteWithSpecialColors(void);
        void                CreateCMCompatiblePalette(HColorModel  *pCM);
        void                GetRGBQUADS(RGBQUAD *pRgbq);
        int                 GetBitsPerPixel();
        int                 GetNumColors();
        void                FillILTable();
        int                 ClosestMatchPalette(int r, int g, int b);
        void                PrintPalette();
virtual int                 GetRgbVal(int r, int g, int b)
                            {
                                return m_ILTable[(int32_t)((r) + CMAP_BIAS) >> (int32_t)(8L - CMAP_BITS)]
                                                [(int32_t)((g) + CMAP_BIAS) >> (int32_t)(8L - CMAP_BITS)]
                                                [(int32_t)((b) + CMAP_BIAS) >> (int32_t)(8L - CMAP_BITS)];
                            }

private:
        // Private constructor forces the object to be on heap and hence 
        // our cache
                            AwtPalette();

        static AwtCList  m_freeChain;
        static AwtCList  m_liveChain;


    //
    // Data members:
    //
private:
    AwtMutex        m_hpal_lock;
    static BOOL     m_IsInitialized;

    UINT            m_refCount;
    UINT            m_lastOwner;
    PALETTEENTRY   *m_pPalEntries;
    LOGPALETTE     *m_pLogPal; 
    int             m_numColors;
    int             m_bitPerPixel;

    HDC             m_hDC;
    HPALETTE        m_hPal;
    HPALETTE        m_hPalOrig;
    BOOL            m_ILTableFilled;
    BYTE            m_ILTable[CMAP_COLS+1][CMAP_COLS+1][CMAP_COLS+1];
};


#endif
