#include "awt_defs.h"
#include "awt_window.h"
#include "awt_i18n.h"
#include "libi18n.h"
#include "prlink.h"

PRLibrary* uniLib = NULL;

extern "C" {
#include "java_lang_String.h"
};

#ifdef XP_WIN32
#define UNICODEDLL					"UNI3200.DLL"
#define LIBRARYLOADOK(l)			(l != NULL)
#define	MODULE_APP					::GetModuleHandle(NULL)

#define	INTLGETJAVAFACENAME_SYM			"IntlGetJavaFaceName_e"
#define INTLGETJAVACHARSET_SYM			"IntlGetJavaCharset_e"

#define	UNICODE_MENU_FONTCSID_SYM		"UNICODE_MenuFontCsid"
#define UNICODE_MENU_FONTID_SYM			"UNICODE_MenuFontID"

#define INTL_GETUNICODECSIDLIST_SYM		"INTL_GetUnicodeCSIDList_e"
#define INTL_UNICODELEN_SYM				"INTL_UnicodeLen_e"
#define INTL_COMPOUNDSTRFROMUNICODE_SYM	"INTL_CompoundStrFromUnicode_e"
#define INTL_COMPOUNDSTRDESTROY_SYM		"INTL_CompoundStrDestroy_e"
#define INTL_COMPOUNDSTRFIRSTSTR_SYM	"INTL_CompoundStrFirstStr_e"
#define INTL_COMPOUNDSTRNEXTSTR_SYM		"INTL_CompoundStrNextStr_e"
#define INTL_COMPOUNDSTRCLONE_SYM		"INTL_CompoundStrClone_e"
#define INTL_DEFAULTWINCHARSETID_SYM	"INTL_DefaultWinCharSetID_e"
#define	INTL_TEXTBYTECOUNTTOCHARLEN_SYM	"INTL_TextByteCountToCharLen_e"
#define INTL_TEXTCHARLENTOBYTECOUNT_SYM	"INTL_TextCharLenToByteCount_e"
#define	INTL_UNICODETOSTRLEN_SYM		"INTL_UnicodeToStrLen_e"
#define	INTL_UNICODETOSTR_SYM			"INTL_UnicodeToStr_e"
#define INTL_MAKEJAVASTRING_SYM			"intl_makeJavaString_e"

#define CINTLWIN_TEXTOUT_SYM			"CIntlWin_TextOut_e"
#define	CINTLWIN_GETTEXTEXTENTPOINT_SYM	"CIntlWin_GetTextExtentPoint_e"

#else	// WIN32
#define UNICODEDLL					"UNI1600.DLL"
#define LIBRARYLOADOK(l)			(((int)l) >= 32)
#define	MODULE_APP					NULL

#define	INTLGETJAVAFACENAME_SYM			"_IntlGetJavaFaceName_e"
#define INTLGETJAVACHARSET_SYM			"_IntlGetJavaCharset_e"

#define	UNICODE_MENU_FONTCSID_SYM		"_UNICODE_MenuFontCsid"
#define UNICODE_MENU_FONTID_SYM			"_UNICODE_MenuFontID"

#define INTL_GETUNICODECSIDLIST_SYM		"_INTL_GetUnicodeCSIDList_e"
#define INTL_UNICODELEN_SYM				"_INTL_UnicodeLen_e"
#define INTL_COMPOUNDSTRFROMUNICODE_SYM	"_INTL_CompoundStrFromUnicode_e"
#define INTL_COMPOUNDSTRDESTROY_SYM		"_INTL_CompoundStrDestroy_e"
#define INTL_COMPOUNDSTRFIRSTSTR_SYM	"_INTL_CompoundStrFirstStr_e"
#define INTL_COMPOUNDSTRNEXTSTR_SYM		"_INTL_CompoundStrNextStr_e"
#define INTL_COMPOUNDSTRCLONE_SYM		"_INTL_CompoundStrClone_e"
#define INTL_DEFAULTWINCHARSETID_SYM	"_INTL_DefaultWinCharSetID_e"
#define	INTL_TEXTBYTECOUNTTOCHARLEN_SYM	"_INTL_TextByteCountToCharLen_e"
#define INTL_TEXTCHARLENTOBYTECOUNT_SYM	"_INTL_TextCharLenToByteCount_e"
#define	INTL_UNICODETOSTRLEN_SYM		"_INTL_UnicodeToStrLen_e"
#define	INTL_UNICODETOSTR_SYM			"_INTL_UnicodeToStr_e"
#define INTL_MAKEJAVASTRING_SYM			"_intl_makeJavaString_e"

#endif

#ifdef INTLUNICODE
//
//	Wrapper Function
//
const char*	
libi18n::IntlGetJavaFaceName(int i1, int i2)
{
	typedef const char* (*f)(int, int);
	f p = NULL;
	VERIFY(p = (f) ::GetProcAddress(MODULE_APP, INTLGETJAVAFACENAME_SYM));
	if(p)
		return (*p)(i1,i2);
	else
		return "";
}

BYTE			
libi18n::IntlGetJavaCharset(int i1, int i2)
{
	typedef BYTE (*f)(int, int);
	f p = NULL;
	VERIFY(p = (f) ::GetProcAddress(MODULE_APP, INTLGETJAVACHARSET_SYM));
	if(p)
		return (*p)(i1,i2);
	else
		return 0;
}
int16	
libi18n::IntlMenuFontCsid()
{
	static int16 menuFontCsid = CS_DEFAULT;
	if(menuFontCsid != CS_DEFAULT)
			return menuFontCsid;	// Cache systemFontCsid for performance issue. 

	int16 ret = CS_LATIN1;
	if (uniLib == NULL)
		uniLib = PR_LoadLibrary(UNICODEDLL);
	if(uniLib)
	{
		typedef int16 (*f)();
		f p = NULL;
		VERIFY(p = (f) PR_FindSymbol(UNICODE_MENU_FONTCSID_SYM, uniLib));
		if(p)
			ret = (menuFontCsid = (*p)());
	}
	return ret;
}
int	
libi18n::IntlMenuFontID()
{
	static int menuFontID = 0;
	if(menuFontID != 0)
			return menuFontID;	// Cache systemFontCsid for performance issue. 

	int ret = SYSTEM_FONT;
	if( uniLib == NULL)
		uniLib = PR_LoadLibrary(UNICODEDLL);
	if(uniLib)
	{
		typedef int16 (*f)();
		f p = NULL;
		VERIFY(p = (f) PR_FindSymbol(UNICODE_MENU_FONTID_SYM, uniLib));
		if(p)
			ret = (menuFontID = (*p)());
	}
	return ret;
}

int16*		
libi18n::INTL_GetUnicodeCSIDList(int16* i1)
{
	typedef int16* (*f)(int16*);
	f p = NULL;
	VERIFY(p = (f) ::GetProcAddress(MODULE_APP, INTL_GETUNICODECSIDLIST_SYM));
	if(p)
		return (*p)(i1);
	else
		return NULL;
}

int32		
libi18n::INTL_UnicodeLen(uint16* i1)
{
	typedef int32 (*f)(uint16*);
	f p = NULL;
	VERIFY(p = (f) ::GetProcAddress(MODULE_APP, INTL_UNICODELEN_SYM));
	if(p)
		return (*p)(i1);
	else
		return 0;
}

INTL_CompoundStr* 
libi18n::INTL_CompoundStrFromUnicode(uint16* i1, uint32 i2)
{
	typedef INTL_CompoundStr* (*f)(uint16*, uint32);
	f p = NULL;
	VERIFY(p = (f) ::GetProcAddress(MODULE_APP, INTL_COMPOUNDSTRFROMUNICODE_SYM));
	if(p)
		return (*p)(i1,i2);
	else
		return NULL;
}

void			
libi18n::INTL_CompoundStrDestroy(INTL_CompoundStr* i1)
{
	typedef void* (*f)(INTL_CompoundStr*);
	f p = NULL;
	VERIFY(p = (f) ::GetProcAddress(MODULE_APP, INTL_COMPOUNDSTRDESTROY_SYM));
	if(p)
		 (*p)(i1);
}

INTL_CompoundStrIterator 
libi18n::INTL_CompoundStrFirstStr(INTL_CompoundStr* i1, INTL_Encoding_ID *i2, unsigned char** i3)
{
	typedef INTL_CompoundStrIterator (*f)(INTL_CompoundStr*, INTL_Encoding_ID*, unsigned char**);
	f p = NULL;
	VERIFY(p = (f) ::GetProcAddress(MODULE_APP, INTL_COMPOUNDSTRFIRSTSTR_SYM));
	if(p)
		return (*p)(i1,i2,i3);
	else
		return NULL;
}

INTL_CompoundStrIterator 
libi18n::INTL_CompoundStrNextStr(INTL_CompoundStrIterator i1, INTL_Encoding_ID *i2, unsigned char** i3)
{
	typedef INTL_CompoundStrIterator (*f)(INTL_CompoundStrIterator, INTL_Encoding_ID*, unsigned char**);
	f p = NULL;
	VERIFY(p = (f) ::GetProcAddress(MODULE_APP, INTL_COMPOUNDSTRNEXTSTR_SYM));
	if(p)
		return (*p)(i1,i2,i3);
	else
		return NULL;
}

INTL_CompoundStr* 		
libi18n::INTL_CompoundStrClone(INTL_CompoundStr* i1)
{
	typedef INTL_CompoundStr* (*f)(INTL_CompoundStr*);
	f p = NULL;
	VERIFY(p = (f) ::GetProcAddress(MODULE_APP, INTL_COMPOUNDSTRCLONE_SYM));
	if(p)
		return (*p)(i1);
	else
		return NULL;
}
int16		
libi18n::INTL_DefaultWinCharSetID(MWContext *i1)
{
	typedef int16 (*f)(MWContext*);
	f p = NULL;
	VERIFY(p = (f) ::GetProcAddress(MODULE_APP, INTL_DEFAULTWINCHARSETID_SYM));
	if(p)
		return (*p)(i1);
	else
		return 0;
}

int32		
libi18n::INTL_TextByteCountToCharLen(int16 i1, unsigned char* i2, uint32 i3)
{
	typedef int32 (*f)(int16,unsigned char*,uint32 );
	f p = NULL;
	VERIFY(p = (f) ::GetProcAddress(MODULE_APP, INTL_TEXTBYTECOUNTTOCHARLEN_SYM));
	if(p)
		return (*p)(i1,i2,i3);
	else
		return 0;
}

int32		
libi18n::INTL_TextCharLenToByteCount(int16 i1, unsigned char* i2, uint32 i3)
{
	typedef int32 (*f)(int16, unsigned char*, uint32 );
	f p = NULL;
	VERIFY(p = (f) ::GetProcAddress(MODULE_APP, INTL_TEXTCHARLENTOBYTECOUNT_SYM));
	if(p)
		return (*p)(i1,i2,i3);
	else
		return 0;
}

uint32		
libi18n::INTL_UnicodeToStrLen(INTL_Encoding_ID i1,INTL_Unicode* i2, uint32 i3)
{
	typedef uint32 (*f)(INTL_Encoding_ID, INTL_Unicode*, uint32);
	f p = NULL;
	VERIFY(p = (f) ::GetProcAddress(MODULE_APP, INTL_UNICODETOSTRLEN_SYM));
	if(p)
		return (*p)(i1,i2,i3);
	else
		return 0;
}

void			
libi18n::INTL_UnicodeToStr(INTL_Encoding_ID i1,INTL_Unicode* i2,uint32 i3,unsigned char* i4, uint32 i5)
{
	typedef void (*f)(INTL_Encoding_ID, INTL_Unicode*, uint32, unsigned char*, uint32);
	f p = NULL;
	VERIFY(p = (f) ::GetProcAddress(MODULE_APP, INTL_UNICODETOSTR_SYM));
	if(p)
		(*p)(i1,i2,i3,i4,i5);
}

jref *
libi18n::intl_makeJavaString(int16 i1, char *i2, int i3)
{
	typedef jref* (*f)(int16, char*, int);
	f p = NULL;
	VERIFY(p = (f) ::GetProcAddress(MODULE_APP, INTL_MAKEJAVASTRING_SYM));
	if(p)
		return (*p)(i1,i2,i3);
	else
		return 0;
}

//
//	Abstract from the libi18n
//
int16
libi18n::GetPrimaryEncoding()
{
	return libi18n::INTL_DefaultWinCharSetID(0);
}

uint32 
libi18n::intl_javaStringCommon(Hjava_lang_String *javaStringHandle, 
		WCHAR**    uniChars)
{
	HArrayOfChar* unicodeArrayHandle = 
		(HArrayOfChar *)unhand( javaStringHandle )->value;
	*uniChars =  unhand(unicodeArrayHandle)->body 
			+ unhand( javaStringHandle )->offset;
	return unhand( javaStringHandle )->count;
}
void 
libi18n::intl_javaString2CString(int16 encoding , 
		Hjava_lang_String *javaStringHandle, 
		unsigned char *out, uint32 buflen)
{
	uint32      length = 0;
	WCHAR     emptyString = 0;
	WCHAR*    uniChars = &emptyString;
	if( javaStringHandle != NULL )
		length = intl_javaStringCommon(javaStringHandle, &uniChars);
	libi18n::INTL_UnicodeToStr(encoding, uniChars, length,  out, buflen);
}


BOOL 
libi18n::CIntlWin_TextOut(int16 i1, HDC i2, int i3, int i4, LPCSTR  i5,int  i6)
{
#ifdef _WIN32
    typedef BOOL (*f)(int16, HDC, int, int, LPCSTR ,int );
    f p = NULL;
    VERIFY(p = (f) ::GetProcAddress(MODULE_APP, CINTLWIN_TEXTOUT_SYM));
    if(p)
        return (*p)(i1,i2,i3,i4,i5,i6);
    else
        return FALSE;
#else // _WIN32
    return ::TextOut(i2,i3,i4,i5,i6);      
#endif // _WIN32
}
BOOL 
libi18n::CIntlWin_GetTextExtentPoint(int i1, HDC i2, LPCSTR i3, int i4, LPSIZE i5)
{
#ifdef _WIN32
	typedef BOOL (*f)(int, HDC, LPCSTR, int, LPSIZE);
	f p = NULL;
	VERIFY(p = (f) ::GetProcAddress(MODULE_APP, CINTLWIN_GETTEXTEXTENTPOINT_SYM	));
	if(p)
		return (*p)(i1,i2,i3,i4,i5);
	else
		return FALSE;
#else // _WIN32
    return ::GetTextExtentPoint(i2,i3,i4,i5);
#endif // _WIN32
}



char * 
libi18n::intl_allocCString(int16 encoding , Hjava_lang_String *javaStringHandle) 
{
	uint32        length;
	WCHAR     emptyString = 0;
	WCHAR*    uniChars = &emptyString;
	if( javaStringHandle != NULL )
	{
		length = libi18n::intl_javaStringCommon(javaStringHandle, 
							&uniChars);
	}
	uint32 buflen = libi18n::INTL_UnicodeToStrLen(encoding, uniChars, length);
    unsigned char * buf = (unsigned char *)malloc(buflen);
    CHECK_NULL_AND_RETURN(buf, "intl_allocCString:: Out of Memory",0);
	libi18n::INTL_UnicodeToStr(encoding, uniChars, length, buf, buflen);
	return (char*)buf;
}


AwtVFontList* libi18n::s_MenuVFontList = NULL;
HDC libi18n::s_MenuHDC = NULL;
HBITMAP	libi18n::s_CheckmarkBitmap = NULL;

AwtVFontList*
libi18n::GetMenuVFontList()
{
	if(s_MenuVFontList == NULL)
	{
		HDC hDC = libi18n::GetMenuDC();
		::SelectObject(hDC, GetStockObject(libi18n::IntlMenuFontID()));
		TEXTMETRIC metrics;
		VERIFY(::GetTextMetrics(hDC, &metrics));
		s_MenuVFontList = new AwtVFontList(	
							metrics.tmHeight, 
							metrics.tmWeight , 
							metrics.tmItalic, 
							0);
		VERIFY(s_MenuVFontList);
	}
	return s_MenuVFontList;
}
HDC
libi18n::GetMenuDC()
{
	if(s_MenuHDC == NULL)
	{
		s_MenuHDC = CreateIC("DISPLAY", NULL, NULL, NULL);
		VERIFY(s_MenuHDC);
	}
	return s_MenuHDC;
}
BOOL libi18n::iswin95()
{
#if 1
	return TRUE;
#else
	static BOOL initialized = FALSE;
	static BOOL iswin95 = FALSE;
	if(initialized)
		return iswin95;
	// The following code are copy from styles.cpp
	DWORD	dwVer = GetVersion();
	int iVer = (LOBYTE(LOWORD(dwVer))*100) + HIBYTE(LOWORD(dwVer));
	if( iVer >= 395)
		iswin95 = TRUE;
	else
		iswin95 = FALSE;
	initialized = TRUE;
	return iswin95;
#endif
}
HBITMAP	
libi18n::GetCheckmarkBitmap()
{
	if(s_CheckmarkBitmap == NULL)
	{	
		if(libi18n::iswin95())
		{
			static	BYTE win95check[] = {
				0xff, 0x00, 
				0xfe, 0x00, 
				0xfc, 0x00, 
				0xb8, 0x00, 
				0x91, 0x00, 
				0x83, 0x00, 
				0xc7, 0x00, 
				0xef, 0x00, 
			};
			s_CheckmarkBitmap = CreateBitmap(8,8,1,1,win95check);
		}
		else
		{
			static	BYTE win16check[] = {
				0xfe, 0x00, 
				0xfe, 0x00, 
				0xfc, 0x00, 
				0xfd, 0x00, 
				0xf9, 0x00, 
				0x3b, 0x00, 
				0x93, 0x00, 
				0xd7, 0x00, 
				0xc7, 0x00, 
				0xef, 0x00
			};
			s_CheckmarkBitmap = CreateBitmap(8,10,1,1,win16check);
		}
	}
	return s_CheckmarkBitmap;
}
#endif
