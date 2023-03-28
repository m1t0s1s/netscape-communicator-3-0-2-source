#include "awt_vf.h"
#include "awt_wcompstr.h"

#ifdef INTLUNICODE
// All java header files are extern "C"
extern "C" {
#include "java_awt_Font.h"
#include "sun_awt_windows_WFontMetrics.h"
};


enum {							// just to ignore unnecessary strcomp
		javaFontHelvetica = 0,
		javaFontTimesRoman,
		javaFontCourier,
		javaFontDialog,
		javaFontDialogInput,
		javaFontZapfDingbats
};

AwtVFontList::AwtFontIDMap AwtVFontList::m_fontIDNameMap[] = {
    { 0             , javaFontHelvetica		},  // Default font
    { "Helvetica"   , javaFontHelvetica		},
    { "TimesRoman"  , javaFontTimesRoman	},
    { "Courier"     , javaFontCourier		},
    { "Dialog"      , javaFontDialog		},
    { "DialogInput" , javaFontDialogInput	},
    { "ZapfDingbats", javaFontZapfDingbats	},
    { 0             , 0						}
};
//
//	log the height, weight, italic, and id
//	create a font list that do not have HFONT- Lazy loading
//
AwtVFontList::AwtVFontList(LONG height, LONG weight, BYTE italic, int16 id)
{
	m_height = height;
	m_weight = weight;
	m_italic = italic;
	m_id = id;
	m_numOfItem = 0;
	// We need to call this function to figure out how many item we should allocate
	int16 *list= libi18n::INTL_GetUnicodeCSIDList(&m_numOfItem);
	if(m_numOfItem > 0)
	{
		m_fontlist = (VFontItem*) malloc(sizeof(VFontItem) * m_numOfItem);
		// if we can allocate m_fontlist- initial it by fill in the encoding id
		//	only, we will fill in the m_hfont field when we NEED it.
		if(m_fontlist)
		{
			for(int i=0;i<m_numOfItem;i++)
			{
				m_fontlist[i].m_encoding = list[i];
				m_fontlist[i].m_hfont = NULL;
			}
		}
		else	// otherwise, set m_numOfItem to zero.
			m_numOfItem = 0;
	}
	else
	{
		m_numOfItem = 0;
		m_fontlist = NULL;
	}
}
AwtVFontList::~AwtVFontList()
{
	if(m_fontlist != NULL)
	{
		if(m_numOfItem > 0)
		{	// For each item, check the m_hfont. free it only if it is not
			// NULL
			for(int i=0;i<m_numOfItem; i++)	
			{
				if(m_fontlist[i].m_hfont != NULL )	
				{
					VERIFY (::DeleteObject(m_fontlist[i].m_hfont));
					m_fontlist[i].m_hfont = NULL;
				}
			}
		}
		free(m_fontlist);
		m_fontlist = NULL;
	}
}
HGDIOBJ 
AwtVFontList::GetVFont(int16 encoding)
{
	for(int i=0;i<m_numOfItem;i++)
	{
		if(m_fontlist[i].m_encoding == encoding)
		{
			if(m_fontlist[i].m_hfont == NULL)
			{
				// The font is not created yet. Let's create it.
				LOGFONT logFont;
				logFont.lfWidth          = 0;
				logFont.lfEscapement     = 0;
				logFont.lfOrientation    = 0;
				logFont.lfUnderline      = FALSE;
				logFont.lfStrikeOut      = FALSE;
				logFont.lfOutPrecision   = OUT_DEFAULT_PRECIS;
				logFont.lfClipPrecision  = CLIP_DEFAULT_PRECIS;
				logFont.lfQuality        = DEFAULT_QUALITY;
				logFont.lfPitchAndFamily = DEFAULT_PITCH | FF_DONTCARE;

				logFont.lfWeight = m_weight;
				logFont.lfItalic = m_italic;
				logFont.lfHeight = m_height;

				logFont.lfCharSet        = libi18n::IntlGetJavaCharset(
					m_fontlist[i].m_encoding, m_id);

				// Set the face name
				strncpy(logFont.lfFaceName, libi18n::IntlGetJavaFaceName(
						m_fontlist[i].m_encoding, m_id), 
						LF_FACESIZE);

				//
				// Create the windows font
				//
				VERIFY( m_fontlist[i].m_hfont = ::CreateFontIndirect(&logFont) );
			}
			// if we just cannot create that font. break it and let 
			// GetStockObject(SYSTEM_FONT) handle it.
			if(m_fontlist[i].m_hfont == NULL)
				break;
			return m_fontlist[i].m_hfont;
		}
	}
	return ::GetStockObject(SYSTEM_FONT);
}
int16	
AwtVFontList::MapFamilyToVFontID(Hjava_awt_Font *pJavaObject)
{
    Classjava_awt_Font *pFontObject;
    char buffer[ 256 ];
	int16 id;
    AwtFontIDMap *pEntry;

    //
    // Get the name of the Awt font family
    //
    pFontObject = unhand(pJavaObject);
    javaString2CString(pFontObject->family, buffer, sizeof(buffer));

    //
    // Locate the Windows font which maps to the requested font family
    //
    id			 = AwtVFontList::m_fontIDNameMap[0].id;
    pEntry       = AwtVFontList::m_fontIDNameMap + 1;
    while (pEntry->family) {
        if( strcmp(pEntry->family, buffer) == 0 ) {
            id = pEntry->id;
            break;
        }
        pEntry++;
    }
    return id;
}
void    
AwtVFontList::LoadFontMetrics(HDC hDC, TEXTMETRIC *max, int16 DontCareEncoding)
{
	TEXTMETRIC metrics;
	for(int i=0;i<m_numOfItem;i++)
	{
		if(m_fontlist[i].m_encoding != DontCareEncoding)
		{
			::SelectObject(hDC, (HGDIOBJ)this->GetVFont(m_fontlist[i].m_encoding));
			VERIFY(::GetTextMetrics(hDC, &metrics));

			//
			// Initialize the font metric information stored in the 
			// WFontMetrics object...
			//
			max->tmAscent			= max(max->tmAscent,			metrics.tmAscent);
			max->tmDescent			= max(max->tmDescent,			metrics.tmDescent);
			max->tmExternalLeading  = max(max->tmExternalLeading,	metrics.tmExternalLeading);
			max->tmInternalLeading  = max(max->tmInternalLeading,	metrics.tmInternalLeading);
			max->tmHeight			= max(max->tmHeight,			metrics.tmHeight);
			max->tmMaxCharWidth		= max(max->tmMaxCharWidth,		metrics.tmMaxCharWidth);
		}
	}
}
void    
AwtVFontList::LoadFontMetrics(HDC hDC, WFontMetrics *pMetrics)
{
    TEXTMETRIC metrics;
	memset(&metrics, 0,sizeof(metrics));
	this->LoadFontMetrics(hDC, &metrics, CS_LATIN1);
	pMetrics->ascent     = max(pMetrics->ascent,metrics.tmAscent);
	pMetrics->descent    = max(pMetrics->descent,metrics.tmDescent);
	pMetrics->leading    = max(pMetrics->leading,metrics.tmExternalLeading);
	pMetrics->height     = max(pMetrics->height, metrics.tmExternalLeading + metrics.tmHeight);
	pMetrics->maxAdvance = max(pMetrics->maxAdvance, metrics.tmMaxCharWidth);
    pMetrics->maxAscent  = pMetrics->ascent;
    pMetrics->maxDescent = pMetrics->descent;
    pMetrics->maxHeight  = pMetrics->height;
}


#endif
