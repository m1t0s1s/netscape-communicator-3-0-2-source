#include "awt_font.h"

#ifdef INTLUNICODE
#include "awt_vf.h"
#endif

#include "prprf.h"       // PR_snprintf(...)

// All java header files are extern "C"
extern "C" {
#include "java_awt_Font.h"
#include "sun_awt_windows_WFontMetrics.h"
};


#define FONT_PTR(_qp) ((AwtFont*) ((char*)(_qp) - offsetof(AwtFont,m_link)))

//#define DEBUG_GDI


AwtCList AwtFont::m_fontList;
AwtMutex AwtFont::m_fontMutex;


//-------------------------------------------------------------------------
//
// The AwtFaceMap is a mapping between a platform-independent Awt
// font family and the Windows font used to represent it...
//
//
// Table of font mappings...
//
// m_faceNameMap[0] contains the default font used when a match is not
// found...
//
//-------------------------------------------------------------------------
AwtFont::AwtFaceMap AwtFont::m_faceNameMap[] = {
    { 0             , "Arial"           },  // Default font
    { "Helvetica"   , "Arial"           },
    { "TimesRoman"  , "Times New Roman" },
    { "Courier"     , "Courier New"     },
    { "Dialog"      , "MS Sans Serif"   },
    { "DialogInput" , "MS Sans Serif"   },
    { "ZapfDingbats", "WingDings"       },
    { 0             , 0                 }
};




//-------------------------------------------------------------------------
//
//
//-------------------------------------------------------------------------
AwtFont::AwtFont(Hjava_awt_Font *peer) : AwtObject((JHandle *)peer)
{
    m_refCount    = 1;
    m_lockCount   = 0;;
    *m_fontKey    = '\0';

    //
    // Initialize the font to the System font.  This insures that
    // the m_hFont member is ALWAYS valid...
    //
    m_hFont       = (HFONT)::GetStockObject(SYSTEM_FONT);
#ifdef INTLUNICODE
	m_vFontList	  =	NULL;
#endif
    HDC hDC;
    VERIFY( hDC = ::GetDC(::GetDesktopWindow()) );

    if (hDC) 
    {
        //
        // Get the text metrics information about the requested font
        //
        ::SelectObject(hDC, (HGDIOBJ)m_hFont);
        TEXTMETRIC metrics;
        VERIFY( ::GetTextMetrics(hDC, &metrics) );
        m_nAscent = metrics.tmAscent;
        VERIFY( ::ReleaseDC(::GetDesktopWindow(), hDC) );
    }
}


//-------------------------------------------------------------------------
//
//
//-------------------------------------------------------------------------
AwtFont::~AwtFont()
{
    // Delete the font resource
    if (m_hFont) {
        VERIFY( ::DeleteObject(m_hFont) );
#ifdef DEBUG_GDI
        char buf[100];
        wsprintf(buf, "Delete Font = 0x%x, thrid=%d\n",m_hFont, PR_GetCurrentThreadID() );
        OutputDebugString(buf);
#endif
        m_hFont = 0;
    }
#ifdef INTLUNICODE
	if(m_vFontList)
	{
		delete m_vFontList;
		m_vFontList = NULL;
	}
#endif

    //
    // Remove the object from the cache...
    //
    m_fontMutex.Lock();

    m_link.Remove();

    m_fontMutex.Unlock();
}


//-------------------------------------------------------------------------
//
//
//-------------------------------------------------------------------------
void AwtFont::Dispose(void)
{
}


//-------------------------------------------------------------------------
//
//
//-------------------------------------------------------------------------
BOOL AwtFont::CallMethod(AwtMethodInfo *info)
{
    return FALSE;
    
}


//-------------------------------------------------------------------------
//
// Return an AwtFont object which corresponds to a java.awt.Font object.
//
// The AwtFont is reference counted.  So, Release() must be called on 
// EVERY object returned by FindFont!!
//
//-------------------------------------------------------------------------
AwtFont *AwtFont::FindFont(Hjava_awt_Font *font) 
{
    AwtCList *qp;
    AwtFont *pFont;
    AwtFont *pNewFont = NULL;
    char fontKey[ AWT_FONT_KEY_SIZE ];

    m_fontMutex.Lock();

    //
    // Check if the java_awt_Font object already has an AwtFont
    // associated with it...
    //
    if (unhand(font)->pData) {
        pNewFont = (AwtFont *)unhand(font)->pData;
        pNewFont->m_refCount++;
    }

    else {
        //
        // Create the font key...
        //
        AwtFont::GetFontKey(font, fontKey, sizeof(fontKey));
        ASSERT( *fontKey != '\0' );

        //
        // Scan the list of allocated fonts to see if the requested font
        // has already been created...
        //
        qp = m_fontList.next;
        while (qp != &m_fontList) {
            pFont = FONT_PTR(qp);

            if (strcmp(pFont->m_fontKey, fontKey) == 0) {
                unhand(font)->pData = (long)pFont;
                pNewFont = pFont;
                pNewFont->m_refCount++;
                break;
            }

            qp = qp->next;
        }

        //
        // Allocate a new font...
        //
        // Because a single AwtFont can be associated with many java fonts
        // NULL is passed into the constructor since no back pointer is 
        // possible.
        //
        if (pNewFont == NULL) {
            pNewFont = new AwtFont(NULL);

            pNewFont->Initialize(font);
            unhand(font)->pData = (long)pNewFont;
            m_fontList.Append(pNewFont->m_link);
        }

    }

    m_fontMutex.Unlock();
    return pNewFont;
}


//-------------------------------------------------------------------------
//
//
//-------------------------------------------------------------------------
void AwtFont::GetFontKey(Hjava_awt_Font *pJavaObject, char *fontKey, int len)
{
    const char *lpszFaceName;

    ASSERT( (fontKey != NULL) && len );

    lpszFaceName = AwtFont::MapFamilyToFont(pJavaObject);

    PR_snprintf(fontKey, len, "%s-%d-%d", lpszFaceName, 
                unhand(pJavaObject)->style, unhand(pJavaObject)->size);
}


//-------------------------------------------------------------------------
//
// Convert an Awt font family into a Windows font face name
//
//-------------------------------------------------------------------------
const char *AwtFont::MapFamilyToFont(Hjava_awt_Font *pJavaObject)
{
    Classjava_awt_Font *pFontObject;
    char buffer[ 256 ];

    const char *lpszFaceName;
    AwtFaceMap *pEntry;

    //
    // Get the name of the Awt font family
    //
    pFontObject = unhand(pJavaObject);
    javaString2CString(pFontObject->family, buffer, sizeof(buffer));

    //
    // Locate the Windows font which maps to the requested font family
    //
    lpszFaceName = AwtFont::m_faceNameMap[0].faceName;
    pEntry       = AwtFont::m_faceNameMap + 1;
    while (pEntry->family) {
        if( strcmp(pEntry->family, buffer) == 0 ) {
            lpszFaceName = pEntry->faceName;
            break;
        }
        pEntry++;
    }

    return lpszFaceName;
}


//-------------------------------------------------------------------------
//
//
//-------------------------------------------------------------------------
void AwtFont::Initialize(Hjava_awt_Font *peer)
{
    LOGFONT logFont;
    Classjava_awt_Font *pFontObject;

    pFontObject = unhand(peer);

    //
    // Set up the LOGFONT structure
    //
    logFont.lfWidth          = 0;
    logFont.lfEscapement     = 0;
    logFont.lfOrientation    = 0;
    logFont.lfUnderline      = FALSE;
    logFont.lfStrikeOut      = FALSE;
    logFont.lfCharSet        = ANSI_CHARSET;
    logFont.lfOutPrecision   = OUT_DEFAULT_PRECIS;
    logFont.lfClipPrecision  = CLIP_DEFAULT_PRECIS;
    logFont.lfQuality        = DEFAULT_QUALITY;
    logFont.lfPitchAndFamily = DEFAULT_PITCH | FF_DONTCARE;

    //
    // Set the font style...
    // Java supports 3 different styles: Plain, Bold, and Italic
    //
    logFont.lfWeight = (pFontObject->style & java_awt_Font_BOLD)
                            ? FW_BOLD : FW_NORMAL;
    logFont.lfItalic = (pFontObject->style & java_awt_Font_ITALIC)
                            ? TRUE : FALSE;

    // Set the point size...
    logFont.lfHeight = -pFontObject->size;

    // Set the face name
    strncpy(logFont.lfFaceName, 
            AwtFont::MapFamilyToFont(peer), 
            LF_FACESIZE);

    //
    // Create the windows font
    //
    VERIFY( m_hFont = ::CreateFontIndirect(&logFont) );
#ifdef DEBUG_GDI
    char buf[100];
    wsprintf(buf, "Create Font = 0x%x, thrid=%d\n",m_hFont, PR_GetCurrentThreadID() );
    OutputDebugString(buf);
#endif

#ifdef INTLUNICODE
	VERIFY( m_vFontList = new AwtVFontList(logFont.lfHeight,
					logFont.lfWeight, 
					logFont.lfItalic,
					AwtVFontList::MapFamilyToVFontID(peer)
				));
#endif
    HDC hDC;
    VERIFY( hDC = ::GetDC(::GetDesktopWindow()) );

    if (hDC) 
    {
        //
        // Get the text metrics information about the requested font
        //
        ::SelectObject(hDC, (HGDIOBJ)m_hFont);
        TEXTMETRIC metrics;
        VERIFY( ::GetTextMetrics(hDC, &metrics) );
        m_nAscent = metrics.tmAscent;
        VERIFY( ::ReleaseDC(::GetDesktopWindow(), hDC) );
    }


    //
    // Initialize the font key so subsequent requests will return the
    // same AwtFont instance
    //
    AwtFont::GetFontKey(peer, m_fontKey, 
                        sizeof(m_fontKey));

}



//-------------------------------------------------------------------------
//
// Select or remove a font in a GDI DC
//
//-------------------------------------------------------------------------
void AwtFont::Select( HDC hDC, BOOL bFlag )
{
    if (bFlag) {
        VERIFY( ::SelectObject(hDC, m_hFont) );
        m_lockCount++;
    }
    else {
        --m_lockCount;
    }
}



//-------------------------------------------------------------------------
// The following function is needed when making a copy of this font.
// The only other way to increment this counter is via FindFont. But 
// FindFont needs 3 parameters which cannot be queried back from AwtFont.
// REMIND: I think we should be able to query back this stuff.
//-------------------------------------------------------------------------

void AwtFont::AddRef()
{
    m_fontMutex.Lock();
    m_refCount++;
    m_fontMutex.Unlock();
}





//-------------------------------------------------------------------------
//
// Release an AwtFont object.  If this is the last reference to the
// object, then delete it !!
//
//-------------------------------------------------------------------------
void AwtFont::Release( void )
{
    m_fontMutex.Lock();
    if (--m_refCount == 0) {

	m_fontMutex.Unlock();
        delete this;
    } else {
	m_fontMutex.Unlock();
    }
}



//-------------------------------------------------------------------------
//
// This method DOES NOT increment the reference count on the AwtFont
// which is returned!
//
//-------------------------------------------------------------------------
AwtFont * AwtFont::GetFontFromObject(Hjava_awt_Font *peer)
{
    AwtFont *obj;

    obj = GET_OBJ_FROM_FONT(peer);
    if (obj == NULL) {
        obj = AwtFont::FindFont(peer);
        ASSERT(obj);
    }

    return obj;
}

//-------------------------------------------------------------------------
//
//
//-------------------------------------------------------------------------
void AwtFont::LoadFontMetrics(HWFontMetrics *self)
{
    WFontMetrics *pMetrics = unhand(self);

    HDC hDC;
    TEXTMETRIC metrics;
    int i;
    SIZE size;
    char character;
    long *pCharWidths;

    //
    // Allocate a java array to store the width of each character in
    // the requested font...
    //
    pMetrics->widths = (HArrayOfInt *)ArrayAlloc(T_INT, 256);
    CHECK_ALLOCATION( pMetrics->widths );

    pCharWidths = unhand(pMetrics->widths)->body;
    CHECK_ALLOCATION( pCharWidths );

    //
    // Zero out the entire array of character widths...
    //
    pCharWidths = unhand(pMetrics->widths)->body;
    memset(pCharWidths, 0, 256 * sizeof(long));	

    //
    // Get a display DC to calculate the necessary font information...
    //
    VERIFY( hDC = ::GetDC(::GetDesktopWindow()) );

    if (hDC) {
        //
        // Get the text metrics information about the requested font
        //
        ::SelectObject(hDC, (HGDIOBJ)m_hFont);
        VERIFY(::GetTextMetrics(hDC, &metrics));

        //
        // Initialize the font metric information stored in the 
        // WFontMetrics object...
        //
        m_nAscent = pMetrics->ascent     = metrics.tmAscent;
        pMetrics->descent    = metrics.tmDescent;
        pMetrics->leading    = metrics.tmExternalLeading;
        pMetrics->height     = metrics.tmExternalLeading + metrics.tmHeight;
        pMetrics->maxAdvance = metrics.tmMaxCharWidth;
        pMetrics->maxAscent  = pMetrics->ascent;
        pMetrics->maxDescent = pMetrics->descent;
        pMetrics->maxHeight  = pMetrics->height;

        //
        // Fill in the widths of each character in the font
        //
        pCharWidths += metrics.tmFirstChar;
        for (i=metrics.tmFirstChar; i<=metrics.tmLastChar; ++i) {
            character = (char)i;
#if defined(_WIN32)
            VERIFY( ::GetTextExtentPoint32(hDC, &character, 1, &size) );
#else
            VERIFY( ::GetTextExtentPoint(hDC, &character, 1, &size) );
#endif
            *pCharWidths++ = size.cx;
        }
#ifdef INTLUNICODE
        if(m_vFontList)
        {
            m_vFontList->LoadFontMetrics(hDC, pMetrics);
            m_nAscent = pMetrics->ascent ;
        }
#endif
        VERIFY( ::ReleaseDC(::GetDesktopWindow(), hDC) );
    }
}



//-------------------------------------------------------------------------
//
//
//-------------------------------------------------------------------------
long AwtFont::StringWidth(HWFontMetrics *self, Hjava_lang_String *string)
{
    HArrayOfChar *  data;
    int32_t         len;
    int32_t         offset;

    data   = unhand(string)->value;
    offset = unhand(string)->offset;
    
    // "count - offset" seems to be really wrong. In fact offset represents the
    // value inside the array from which the string starts, count is the
    // length of the string. The idea is to have a single long buffer it can
    // be used for multiple strings. It is clear so that count and offset have no 
    // reletionship at all in order to make the string length.
    //len    = unhand(string)->count - offset;
    len    = unhand(string)->count;

    return StringWidth(self, data, offset, len);
}


//-------------------------------------------------------------------------
//
//
//-------------------------------------------------------------------------
long AwtFont::StringWidth(HWFontMetrics *self, HArrayOfChar *string, 
                          long offset, long len)
{
    unicode         *s;
    long            w;
    int32_t         ch;
    int32_t         *widths;
    int             widlen;
    WFontMetrics *  fm;

    w    = 0;
    fm   = (WFontMetrics *)unhand(self);

    if ((len < 0) || (offset < 0) || 
        (len + offset > (long)obj_length(string))) {
        SignalError(0, JAVAPKG "ArrayIndexOutOfBoundsException", 0);
        return 0;
    }

    // Load the width table if necessary
    if (fm->widths == NULL) {
        LoadFontMetrics(self);
    }

    if (fm->widths) {
        widths = unhand(fm->widths)->body;
        widlen = obj_length(fm->widths);
        s      = unhand(string)->body + offset;

        //
        // Compute the total width by accumulating the width of each
        // character in the array.  
        //
        // If a character's width is unknown (ie. not in widths array)
        // then assume the maximum width and continue...
        //
        while (--len >= 0) {
            ch = *s++;
            if (ch < widlen) {
                w += widths[ch];
            } else {
                w += fm->maxAdvance;
            }
        }
    } else {
        //
        // If no widths array is available, then assume every character
        // is of maximum width...
        //
        w = fm->maxAdvance * len;
    }

    return w;
}

//-------------------------------------------------------------------------
//
//
//-------------------------------------------------------------------------
long AwtFont::StringWidth(HWFontMetrics *self, HArrayOfByte *buffer,
                          long offset, long len)
{
    long            w;
    unsigned char * s;
    int32           ch;
    int32_t *       widths;
    int             widlen;
    WFontMetrics *  fm;

    w    = 0;
    fm   = (WFontMetrics *)unhand(self);

    if ((len < 0) || (offset < 0) || 
        (len + offset > (long)obj_length(buffer))) {
        SignalError(0, JAVAPKG "ArrayIndexOutOfBoundsException", 0);
        return 0;
    }

    // Load the width table if necessary
    if (fm->widths == NULL) {
        LoadFontMetrics(self);
    }

    if (fm->widths) {
        widths = unhand(fm->widths)->body;
        widlen = obj_length(fm->widths);
        s      = (unsigned char *)unhand(buffer)->body + offset;

        //
        // Compute the total width by accumulating the width of each
        // character in the array.  
        //
        // If a character's width is unknown (ie. not in widths array)
        // then assume the maximum width and continue...
        //
        while (--len >= 0) {
            ch = *s++;
            if (ch < widlen) {
                w += widths[ch];
            } else {
                w += fm->maxAdvance;
            }
        }
    } else {
        //
        // If no widths array is available, then assume every character
        // is of maximum width...
        //
	w = fm->maxAdvance * len;
    }

    return w;
}

#ifdef INTLUNICODE
HGDIOBJ AwtFont::GetFontForEncoding(int16 encoding)
{
	// For Latin 1, we also use the m_hFont in AwtFont;
	if((encoding != CS_LATIN1) && (m_vFontList))
		return m_vFontList->GetVFont(encoding);
	else
		return m_hFont;
}
#endif


//-------------------------------------------------------------------------
//
// Native methods for sun.awt.windows.WFontMetrics
//
//-------------------------------------------------------------------------

extern "C" {

void 
sun_awt_windows_WFontMetrics_loadFontMetrics(HWFontMetrics *self)
{
    AwtFont *obj;

    CHECK_NULL( self, "WFontMetrics is NULL" );
    CHECK_NULL( unhand(self)->font, "The FontMetrics has no font" );

    if ((obj = AwtFont::GetFontFromObject(unhand(self)->font)) != NULL) {
        obj->LoadFontMetrics(self);
    }
}


long
sun_awt_windows_WFontMetrics_stringWidth(HWFontMetrics *self,
					 Hjava_lang_String *str)
{
    AwtFont *obj;
    long width = 0;

    CHECK_NULL_AND_RETURN( self, "WFontMetrics is NULL", 0 );
    CHECK_NULL_AND_RETURN( str,  "java_lang_String is NULL", 0 );
    CHECK_NULL_AND_RETURN( unhand(self)->font, "The FontMetrics has no font", 0 );

    if ((obj = AwtFont::GetFontFromObject(unhand(self)->font)) != NULL) {
        width = obj->StringWidth(self, str);
    }
    return width;
}


long
sun_awt_windows_WFontMetrics_charsWidth(HWFontMetrics *self,
					HArrayOfChar *str, long off, long len)
{
    AwtFont *obj;
    long width = 0;

    CHECK_NULL_AND_RETURN( self, "WFontMetrics is NULL", 0 );
    CHECK_NULL_AND_RETURN( str,  "HArrayOfChar is NULL", 0 );
    CHECK_NULL_AND_RETURN( unhand(self)->font, "The FontMetrics has no font", 0 );

    if ((obj = AwtFont::GetFontFromObject(unhand(self)->font)) != NULL) {
        width = obj->StringWidth(self, str, off, len);
    }
    return width;
}


long
sun_awt_windows_WFontMetrics_bytesWidth(HWFontMetrics *self,
					HArrayOfByte *buf, long off, long len)
{
    AwtFont *obj;
    long width = 0;

    CHECK_NULL_AND_RETURN( self, "WFontMetrics is NULL", 0 );
    CHECK_NULL_AND_RETURN( buf,  "HArrayOfByte is NULL", 0 );
    CHECK_NULL_AND_RETURN( unhand(self)->font, "The FontMetrics has no font", 0 );

    if ((obj = AwtFont::GetFontFromObject(unhand(self)->font)) != NULL) {
        width = obj->StringWidth(self, buf, off, len);
    }

    return width;
}


void
java_awt_Font_dispose(Hjava_awt_Font *self)
{
    AwtFont *obj = (AwtFont *)(unhand(self)->pData);
    if (obj) {
        obj->Release();
        unhand(self)->pData = 0;
    }
}

};  // end of extern "C"
