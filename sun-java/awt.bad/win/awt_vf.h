#ifndef AWT_VF_H
#define AWT_VF_H
#include "awt_defs.h"
#include "awt_i18n.h"
#include "awt_object.h"
#include "awt_resource.h"

struct Hjava_awt_Font;
DECLARE_SUN_AWT_WINDOWS_PEER(WFontMetrics);

typedef struct {
	int16	m_encoding;
	HFONT	m_hfont;
} VFontItem;

class AwtVFontList {
public:
			AwtVFontList(LONG height, LONG weight, BYTE italic, int16 id);
			~AwtVFontList();
			HGDIOBJ	GetVFont(int16 encoding);
			void    LoadFontMetrics(HDC hDC, WFontMetrics *pMetrics);
			void    LoadFontMetrics(HDC hDC, TEXTMETRIC *pMax, int16 DontCareEncoding);
static int16	MapFamilyToVFontID(Hjava_awt_Font *pJavaObject);

private:
	LONG		m_height;
	LONG		m_weight;
	BYTE		m_italic;
	int16		m_id;
	int16		m_numOfItem;
	VFontItem *	m_fontlist;

	typedef struct AwtFontIDMapStruct 
    {
        const char * family;
        int16		 id;
    } AwtFontIDMap;
	static AwtFontIDMap m_fontIDNameMap[];
};

#endif
