#include "awt_dc.h"
#include "awt_querydc.h"
#include "awt_component.h"
#include "awt_graphics.h"
#include "awt_image.h"
#include "awt_resource.h"
#include "awt_palette.h"

extern "C" {
#include "java_awt_image_ColorModel.h"
#include "java_awt_image_IndexColorModel.h"
};

//#define DEBUG_GDI

static AwtMutex AwtPaletteLock;

AwtPalette::AwtPalette()
{
    m_hDC           = NULL;
    m_refCount      = 1;
    m_numColors     = 0;
    m_pPalEntries   = NULL;
    m_hPal          = NULL;
    m_ILTableFilled = FALSE;
}


//------------------------------------------------------------------------
//
//------------------------------------------------------------------------
AwtPalette::~AwtPalette()
{
    if(m_hPal != NULL)
    {
      DeleteObject(m_hPal);
#ifdef DEBUG_GDI
       char buf[100];
       wsprintf(buf, "Delete m_hPal= 0x%x, thrid=%d\n",m_hPal, PR_GetCurrentThreadID() );
       OutputDebugString(buf);
#endif
    }
    if(m_pPalEntries != NULL)
    {
      free ((char *)m_pLogPal);
    }
}



//------------------------------------------------------------------------
//
//------------------------------------------------------------------------
void AwtPalette::AddRef()
{
    m_hpal_lock.Lock();

    m_refCount++;

    m_hpal_lock.Unlock();
}


//------------------------------------------------------------------------
//
//------------------------------------------------------------------------
void AwtPalette::Release()
{
    UINT nRef;

    m_hpal_lock.Lock();

    nRef = --m_refCount;

    m_hpal_lock.Unlock();

    if (nRef == 0) 
    {
        ReleaseCachedAwtPalette();
        delete this;
    }
}




//------------------------------------------------------------------------
//
// Lock the AwtPalette for exclusive use.
// 
//------------------------------------------------------------------------

void AwtPalette::Lock(  )
{
    m_hpal_lock.Lock();
}




//------------------------------------------------------------------------
//
// Unlock the AwtPalette.  This allows any other thread to obtain a Lock.
//
//------------------------------------------------------------------------
void AwtPalette::Unlock()
{
    m_hpal_lock.Unlock();
}

#define STATIC_COLOR_OFFSET 10 // To preserve the static colors

//------------------------------------------------------------------------
// Creates a logical palette with colors in the color model.
// Note that this is not cached as each image may have its own palette of 
// colors.
//------------------------------------------------------------------------
void AwtPalette::CreateCMCompatiblePalette(HColorModel  *hCM)
{
    HWND hWnd = ::GetDesktopWindow();
    CreateCompatiblePalette(GetDC(hWnd));
    FillPaletteWithSpecialColors();


    int numCMColors;
    IndexColorModel *inxCM;
    if (    obj_classblock(hCM)	
         == FindClass(EE(), "java/awt/image/IndexColorModel", TRUE)
       )	
    {
        unsigned char *cmred;
        unsigned char *cmgreen;
        unsigned char *cmblue;

        inxCM   = (Classjava_awt_image_IndexColorModel *)unhand(hCM);
	       cmred   = (unsigned char *) unhand(inxCM->red);
	       cmgreen = (unsigned char *) unhand(inxCM->green);
	       cmblue  = (unsigned char *) unhand(inxCM->blue);
        numCMColors = inxCM->map_size;
        m_numColors = 256;


        if(m_hPal == NULL)
        {
	          char *buf = new char[sizeof(LOGPALETTE) + m_numColors * sizeof(PALETTEENTRY)];
	          m_pLogPal = (LOGPALETTE*)buf;
	          m_pPalEntries = (PALETTEENTRY *)(&(m_pLogPal->palPalEntry[0])); 
        }
        for(int i=STATIC_COLOR_OFFSET; i<(numCMColors+STATIC_COLOR_OFFSET); i++)
        {
    		    InitPaletteEntry(&m_pPalEntries[i++], cmred[i], cmgreen[i], cmblue[i]);
          if( i > 245 )
          {
             break;
          }
        }

        if(m_hPal != NULL)
        {
           DeleteObject(m_hPal);
#ifdef DEBUG_GDI
       char buf[100];
       wsprintf(buf, "Delete m_hPal= 0x%x, thrid=%d\n",m_hPal, PR_GetCurrentThreadID() );
       OutputDebugString(buf);
#endif
        }
        
	       m_pLogPal->palNumEntries = m_numColors;
	       m_pLogPal->palVersion = 0x300;
	       m_hPal = ::CreatePalette(m_pLogPal);

#ifdef DEBUG_GDI
        char buf[100];
        wsprintf(buf, "Create Palette 1 = 0x%x, thrid=%d\n",m_hPal, PR_GetCurrentThreadID() );
        OutputDebugString(buf);
#endif
        m_bitPerPixel = 8;
    }
    else
    {
        m_bitPerPixel = 24;
    }
    //PrintPalette();
}

int AwtPalette::GetBitsPerPixel()
{
   return m_bitPerPixel;
}

int AwtPalette::GetNumColors()
{
   return m_numColors;
}

AwtPalette *AwtPalette::Create(HColorModel  *hCM)
{
    AwtPalette *pNewAwtPalette = new AwtPalette();
    if(pNewAwtPalette != NULL)
    {
       pNewAwtPalette->CreateCMCompatiblePalette(hCM);
    }
    return pNewAwtPalette;
}

//------------------------------------------------------------------------
// Creates a logical palette made of the same colors as the one in 
// the given DC. This is cached accross many images which want to
// use a standard palette. So this wrapped in a mutex and a static 
// Create function. Note that even if this device does not support 
// palettes we create one and fill it with special colors.
//------------------------------------------------------------------------
void AwtPalette::CreateCompatiblePalette(HDC hDC)
{
  int devcap = GetDeviceCaps(hDC, RASTERCAPS);
  m_numColors   = 256;
  if( (devcap & RC_PALETTE)  > 0)
  {
     // This is a palette  supporting device. So, create a palette with 
     // size same as that of in the device palette.
     m_bitPerPixel = ::GetDeviceCaps(hDC, BITSPIXEL);
     if(m_bitPerPixel < 8)
     {
        m_bitPerPixel = 24;  // We will not get any palette colors but 
                             // create our own.
        
     }
  }
  else 
  {
    m_bitPerPixel = 24;
  }



	 char *buf = new char[sizeof(LOGPALETTE) + m_numColors * sizeof(PALETTEENTRY)];
	 m_pLogPal = (LOGPALETTE*)buf;
	 m_pPalEntries = (PALETTEENTRY *)(&(m_pLogPal->palPalEntry[0])); 
  int result ;
  // Get the palette colors to preserve the static colors.
  // Else generate all the other colors.
  if( m_bitPerPixel == 8)
  {
	   result = ::GetSystemPaletteEntries(hDC, 0, m_numColors, m_pPalEntries);
    if( result != m_numColors)
    {
       m_numColors   = 0;
       m_pPalEntries = NULL;
       free(buf);
       m_hPal = NULL;
       return;
    }
  }

  for(int i=0; i<m_numColors; i++)
  {
	   m_pPalEntries[i].peFlags = PC_NOCOLLAPSE;
  }
 
	 m_pLogPal->palNumEntries = m_numColors;
	 m_pLogPal->palVersion = 0x300;

  if( m_bitPerPixel < 24)
  {
  	 m_hPal = ::CreatePalette(m_pLogPal);
#ifdef DEBUG_GDI
        char buf[100];
        wsprintf(buf, "Create Palette 2= 0x%x, thrid=%d\n",m_hPal, PR_GetCurrentThreadID() );
        OutputDebugString(buf);
#endif
  }
  m_hDC  = hDC;
}



BOOL AwtPalette::CreateCompatiblePalette(HPALETTE hPal)
{ 
  //REMIND: This does not take care of 4 and 16 bit palettes. in such cases
  // we will end up creating a symmetric palette in awt via DC.
	 char *buf = new char[sizeof(LOGPALETTE) + (256 * sizeof(PALETTEENTRY))];
	 m_pLogPal = (LOGPALETTE*)buf;
	 m_pPalEntries = (PALETTEENTRY *)(&(m_pLogPal->palPalEntry[0])); 
  m_numColors = ::GetPaletteEntries(
                 hPal,	// handle of logical color palette 
                 0,	// first entry to retrieve 
                 256,	// number of entries to retrieve 
                 m_pPalEntries 	// address of array receiving entries  
                );

  if (m_numColors < 256 )
  {
    return FALSE;
  }

  // Undocumented behaviour. This gives the best result.
  // All cases including transparency, animation and dithering and pink color
  // drawing work only if we create the palette with these magic flags and 
  // realize the palette in both DC and image draw as background palettes.
  // This fixes bugNo:23796.
  // Applets to test.
  // http://warp.mcom.com/java/applets/UnderConstruction/example1.html (transparency)
  // http://www.vivids.com/java/image/Img.html               (JPEG 256 color display)
  // http://w3.mcom.com/systems/index.html (Animation with the digger )
  // http://warp.mcom.com/java/applets/ArcTest/example1.html (Pink grid)
  for(int i=0; i<256; i++)
  {
	   m_pPalEntries[i].peFlags = PC_NOCOLLAPSE;
  }

  for(i=0; i<10; i++)
  {
	   m_pPalEntries[i].peFlags     = 0;
	   m_pPalEntries[255-i].peFlags = 0;
  }
  /////


	 m_pLogPal->palNumEntries = m_numColors;
	 m_pLogPal->palVersion = 0x300;

  m_hPal     = ::CreatePalette(m_pLogPal);
#ifdef DEBUG_GDI
        char buf1[100];
        wsprintf(buf1, "Create Palette 3= 0x%x, thrid=%d\n",m_hPal, PR_GetCurrentThreadID() );
        OutputDebugString(buf1);
#endif
  m_hPalOrig = hPal;
  
  //PrintPalette();

  return TRUE;
}




void AwtPalette::PrintPalette()
{
   char buf[100];

   for(int i=0; i<m_numColors; i++)
   {
      wsprintf(buf, "(%d,%d,%d)\n",m_pPalEntries[i].peRed, 
                                 m_pPalEntries[i].peGreen,
                                 m_pPalEntries[i].peBlue);
      OutputDebugString(buf);
   }
}



void AwtPalette::GetRGBQUADS(RGBQUAD *pRgbq)
{
    
    PALETTEENTRY *pPal = m_pPalEntries;

    for(int i=0; i<m_numColors; i++)
    {
        pRgbq->rgbRed      = pPal->peRed;
        pRgbq->rgbGreen    = pPal->peGreen;
        pRgbq->rgbBlue     = pPal->peBlue;
        pRgbq->rgbReserved = 0;

        pRgbq++;
        pPal++;
    }
}

// Not used now. but a good utility fn.
PALETTEENTRY *AwtPalette::GetPaletteEntries()
{
   AwtPaletteLock.Lock();
	  char *buf = new char[sizeof(LOGPALETTE) + m_numColors * sizeof(PALETTEENTRY)];
	  LOGPALETTE *pLogPal = (LOGPALETTE*)buf;
	  PALETTEENTRY *pPalEntries = (PALETTEENTRY *)(&(pLogPal->palPalEntry[0])); 
   for(int i=0; i<m_numColors; i++)
   {
     InitPaletteEntry(&pPalEntries[i++], m_pPalEntries[i].peRed, m_pPalEntries[i].peGreen, m_pPalEntries[i].peBlue);
   }

   AwtPaletteLock.Unlock();

   return pPalEntries;
}

// To update a existing set of palette entries created. Used when 
// filling palette with special colors.
void AwtPalette::UpdatePaletteEntries(int startInx, int endInx, PALETTEENTRY *newEntries)
{
   if( (startInx < 0) || (endInx > 255))
   {
     return;
   }
   int i=0;
   for (i=0; i<=(endInx-startInx); i++)
   {
     m_pPalEntries[startInx+i] = newEntries[i];
	    m_pPalEntries[i].peFlags = PC_NOCOLLAPSE;
   }
   if(m_hPal != NULL)
   {
      DeleteObject(m_hPal);
#ifdef DEBUG_GDI
       char buf[100];
       wsprintf(buf, "Delete m_hPal= 0x%x, thrid=%d\n",m_hPal, PR_GetCurrentThreadID() );
       OutputDebugString(buf);
#endif
   }
   if( startInx == 10)
   {
     // Undocumented behaviour. Setting the flags to NOCOLLAPSE or
     // any valid entry does not give a proper mapping. The realized
     // palette is shifted by 10.
     for(i=0; i<10; i++)
     {
	      m_pPalEntries[i].peFlags     = 0;
	      m_pPalEntries[255-i].peFlags = 0;
     }
   }
	  m_hPal = ::CreatePalette(m_pLogPal);
#ifdef DEBUG_GDI
        char buf[100];
        wsprintf(buf, "Create Palette 4= 0x%x, thrid=%d\n",m_hPal, PR_GetCurrentThreadID() );
        OutputDebugString(buf);
#endif
}


void AwtPalette::InitPaletteEntry(PALETTEENTRY *pPalEntry, int r, int g, int b)
{
	  pPalEntry->peRed    = r;
	  pPalEntry->peGreen  = g;
	  pPalEntry->peBlue   = b;
	  pPalEntry->peFlags  = PC_NOCOLLAPSE;
}
void AwtPalette::FillPaletteWithSpecialColors()
{
      int numColors=((m_bitPerPixel == 24)? 256 : 236);
	     char *buf = new char[sizeof(LOGPALETTE) + numColors * sizeof(PALETTEENTRY)];
	     LOGPALETTE *pLogPal = (LOGPALETTE*)buf;
	     PALETTEENTRY *pPalEntries = (PALETTEENTRY *)(&(pLogPal->palPalEntry[0])); 

      int i=0;
      //REMIND: Do we reserve for 4 and 16 color?
      
      // 22 special colors. Only then do we get xor mode working!

		    InitPaletteEntry(&pPalEntries[i++],255, 255, 255);
		    InitPaletteEntry(&pPalEntries[i++],255, 0, 0);
		    InitPaletteEntry(&pPalEntries[i++],0, 255, 0);
		    InitPaletteEntry(&pPalEntries[i++],0, 0, 255);
		    InitPaletteEntry(&pPalEntries[i++],255, 255, 0);
		    InitPaletteEntry(&pPalEntries[i++],255, 0, 255);
		    InitPaletteEntry(&pPalEntries[i++],0, 255, 255);
		    InitPaletteEntry(&pPalEntries[i++],235, 235, 235);
		    InitPaletteEntry(&pPalEntries[i++],224, 224, 224);
		    InitPaletteEntry(&pPalEntries[i++],214, 214, 214);
		    InitPaletteEntry(&pPalEntries[i++],192, 192, 192);
		    InitPaletteEntry(&pPalEntries[i++],162, 162, 162);
		    InitPaletteEntry(&pPalEntries[i++],128, 128, 128);
		    InitPaletteEntry(&pPalEntries[i++],105, 105, 105);
		    InitPaletteEntry(&pPalEntries[i++],64, 64, 64);
		    InitPaletteEntry(&pPalEntries[i++],32, 32, 32);
		    InitPaletteEntry(&pPalEntries[i++],255, 128, 128);
		    InitPaletteEntry(&pPalEntries[i++],128, 255, 128);
		    InitPaletteEntry(&pPalEntries[i++],128, 128, 255);
		    InitPaletteEntry(&pPalEntries[i++],255, 255, 128);
		    InitPaletteEntry(&pPalEntries[i++],255, 128, 255);
		    InitPaletteEntry(&pPalEntries[i++],128, 255, 255);

	    	// This block will allocate 225  colors along the color cube
      // with b varying as 0, 64, 128, 192, 256                   (5 levels)
      // with g varying as 0, 64, 128, 192, 256                   (5 levels)
      // with r varying as 0, 32, 64, 96, 128, 160, 192, 224, 256 (9 levels)
      // Total colors = 5*5*9 = 225
     	int  ri, gi, bi;
                                             
		    for (bi = 0 ; bi <= CMAP_COLS ; bi += 2) 
      {
				    int b = bi << (8 - CMAP_BITS);
				    for (gi = 0 ; gi <= CMAP_COLS ; gi += 2) 
        {
				      int g = gi << (8 - CMAP_BITS);
				      for (ri = 0; ri <= CMAP_COLS ; ri += 1) 
          {
						       int r = ri << (8 - CMAP_BITS);
             InitPaletteEntry(&pPalEntries[i++], r, g, b);
             if ( i > numColors )
             {
               break;
             }
				      }
          if ( i > numColors )
          {
            break;
          }
 			    }
        if ( i > numColors )
        {
          break;
        }
			   }

     int startInx=((numColors == 236)? 10 : 0);
     int endInx=((numColors == 236)? 245 : 255);
     // This will fill in a new set of colors distributed accross 
     // the cube.
     UpdatePaletteEntries(startInx, endInx, pPalEntries);
     delete buf;

     //PrintPalette();


}

//-------------------------------------------------------------------------
// Fills the inverse lookup table associated with this palette. This allows
// for rgb to lut conversion. Right now there are only 9*9*9 = 729 colors
// in the cube.  So the time to construct the table will be 729*256 = 186624
// loops. Since this is done only once per DC it may be OK.
// REMIND: To make RGB to LUT better we should need more levels on each axis 
//         16. For this we would have to implement the convex polyhedron algm 
//         to create the mapping else numloops will be too many.
// Notes::
// ILT = Inverse Lookup Table. (Allows to map a rgb value to a lut value)
//
// LUT1 to LUT2 LUT1->RGB->(ILT)->LUT2. This is used only when
//              we copy one image onto another. In this awt we do not
//              allow for image editing.  
// LUT1 to LUT1 (In case of blting a image to display) = No conversion
//              necessary. Just copy the color table into DIB and select the
//              into hDC before blting. palette
// LUT  to RGB. No Conversion necessary. Copy the color table into DIB and
//              blt it onto 16,24 bit screen. BitBlt will convert.
// RGB to LUT   we have to manufacture a color palette by doing color quantaization.
//              or give colors from a color cube. This is FillPaletteWithSpecialColors.
//              In this case copy the image to a 24 bit dIB and select a special palette 
//              into hDC before blting.
// RGB to RGB   Just copy the bitmap into a DIB and blt it as is.      
// 
//-------------------------------------------------------------------------

void AwtPalette::FillILTable()
{
    int ri, gi, bi;

    for (bi = 0 ; bi <= CMAP_COLS ; bi += 1) {
        int b = bi << (8 - CMAP_BITS);

        for (gi = 0 ; gi <= CMAP_COLS ; gi += 1) {
            int g = gi << (8 - CMAP_BITS);

            for (ri = 0 ; ri <= CMAP_COLS ; ri += 1) {
                int r = ri << (8 - CMAP_BITS);

                m_ILTable[ri][gi][bi] = ClosestMatchPalette(r, g, b);
            }
        }
    }

    m_ILTableFilled = TRUE;
}

int AwtPalette::ClosestMatchPalette(int r, int g, int b)
{
    PALETTEENTRY *p = m_pPalEntries;
    int nPaletteSize = m_numColors;
    int i, besti = 0;
    int32 t, d;
    int32_t mindist = 256L * 256L * 256L;

    // Perform initial range checking on r, g, b
    if (r > 255) r = 255;
    if (g > 255) g = 255;
    if (b > 255) b = 255;

    for (i = 0 ; i < nPaletteSize ; i++, p++) {
        t = p->peRed - r;
        d = t * t;

        if (d >= mindist) {
            continue;
        }
        
        t = p->peGreen - g;
        d += t * t;

        if (d >= mindist) {
            continue;
        }

        t = p->peBlue- b;
        d += t * t;

        if (d >= mindist) {
            continue;
        }

        if (d == 0) {
            return i;
        }

        if (d < mindist) {
            besti = i;
            mindist = d;
        }
    }

    return besti;
}





//-------------------------------------------------------------------------
//
// Cached AwtPalette implementation
//
//-------------------------------------------------------------------------

AwtCList AwtPalette::m_freeChain;
AwtCList AwtPalette::m_liveChain;


//-------------------------------------------------------------------------
//
// Create an AwtPalette object of the requested color.
//
// If an object already exists for the color, then return it, otherwise
// create a new one...
//
//-------------------------------------------------------------------------
AwtPalette *AwtPalette::Create(HDC hDC)
{
    AwtCList *qp;
    AwtPalette *pCachedAwtPalette;
    AwtPalette *pCachedNewAwtPalette = NULL;


    AwtPaletteLock.Lock();
    
    //
    // Scan the list of allocated brushes to see if the requested color
    // has already been created...
    //
    qp = m_liveChain.next;
    while (qp != &m_liveChain) {
        pCachedAwtPalette = (AwtPalette *)RESOURCE_PTR(qp);
        if (pCachedAwtPalette->m_hDC == hDC)
         {
            pCachedNewAwtPalette = pCachedAwtPalette;
            break;
        }
        qp = qp->next;
    }

    //
    // Allocate a new palette
    //
    if (pCachedNewAwtPalette == NULL) 
    {
        // Allocate from the free chain if possible...
        if (m_freeChain.next != m_freeChain.prev) 
        {
            pCachedNewAwtPalette = (AwtPalette *)RESOURCE_PTR( m_freeChain.next );
            //PR_REMOVE_AND_INIT_LINK( &(pCachedNewAwtPalette->m_link) );
            pCachedNewAwtPalette->m_link.Remove();
        } 
        else 
        {
            pCachedNewAwtPalette = new AwtPalette();
            pCachedNewAwtPalette->CreateCompatiblePalette(hDC);
            pCachedNewAwtPalette->FillPaletteWithSpecialColors();
            pCachedNewAwtPalette->FillILTable();
        }

        //
        // Initialize the object and place it in the brush cache...
        //
        //PR_APPEND_LINK( &(pCachedNewAwtPalette->m_link), &m_liveChain );
        m_liveChain.Append(pCachedNewAwtPalette->m_link);

    }

    AwtPaletteLock.Unlock();

    return pCachedNewAwtPalette;
}


AwtPalette *AwtPalette::Create(HPALETTE hPal)
{
    AwtCList *qp;
    AwtPalette *pCachedAwtPalette;
    AwtPalette *pCachedNewAwtPalette = NULL;


    AwtPaletteLock.Lock();
    
    //
    // Scan the list of allocated brushes to see if the requested color
    // has already been created...
    //
    qp = m_liveChain.next;
    while (qp != &m_liveChain) {
        pCachedAwtPalette = (AwtPalette *)RESOURCE_PTR(qp);
        if (pCachedAwtPalette->m_hPalOrig == hPal)
         {
            pCachedNewAwtPalette = pCachedAwtPalette;
            break;
         }
         qp = qp->next;
    }

    //
    // Allocate a new brush
    //
    if (pCachedNewAwtPalette == NULL) 
    {
        // Allocate from the free chain if possible...
        if (m_freeChain.next != m_freeChain.prev) 
        {
            pCachedNewAwtPalette = (AwtPalette *)RESOURCE_PTR( m_freeChain.next );
            //PR_REMOVE_AND_INIT_LINK( &(pCachedNewAwtPalette->m_link) );
            pCachedNewAwtPalette->m_link.Remove();
        } 
        else 
        {
            pCachedNewAwtPalette = new AwtPalette();
            if( pCachedNewAwtPalette->CreateCompatiblePalette(hPal) == TRUE)
            {
               pCachedNewAwtPalette->FillILTable();
            }
            else
            {
               free(pCachedNewAwtPalette);
               pCachedNewAwtPalette=NULL;
               AwtPaletteLock.Unlock();

               return NULL;
            }
        }

        //
        // Initialize the object and place it in the brush cache...
        //
        //PR_APPEND_LINK( &(pCachedNewAwtPalette->m_link), &m_liveChain );
        m_liveChain.Append(pCachedNewAwtPalette->m_link);
    }

    AwtPaletteLock.Unlock();

    return pCachedNewAwtPalette;
}






//-------------------------------------------------------------------------
//
// Delete all of the objects in the cache...
//
//-------------------------------------------------------------------------
void AwtPalette::DeleteCache(void)
{
    AwtPalette *pObject;


    // Lock the cache...
     AwtPaletteLock.Lock();
    
    //
    // Scan the list of allocated brushes to see if the requested color
    // has already been created...
    //
    while (!m_liveChain.IsEmpty()) {
        pObject = (AwtPalette *)RESOURCE_PTR(m_liveChain.next);
        pObject->m_link.Remove();

        pObject->ReleaseCachedAwtPalette();
        delete pObject;
    }

    // Unlock the cache...
     AwtPaletteLock.Unlock();
}


//-------------------------------------------------------------------------
//
// Release an AwtPalette object.  If this is the last reference to the
// object, then free all native resources (ie. GDI) and place the object
// on the free chain.
//
//-------------------------------------------------------------------------
void AwtPalette::ReleaseCachedAwtPalette()
{
     //
     // Remove the object from the cache and place it on the free chain.
     //
     AwtPaletteLock.Lock();
     if(m_hPal != NULL)
     {
        DeleteObject(m_hPal);
#ifdef DEBUG_GDI
       char buf[100];
       wsprintf(buf, "Delete m_hPal= 0x%x, thrid=%d\n",m_hPal, PR_GetCurrentThreadID() );
       OutputDebugString(buf);
#endif
        m_hPal = NULL;
     }
     m_link.Remove();
     m_freeChain.Append(m_link);

     AwtPaletteLock.Unlock();
}

//REMIND Check when this obj is freed
