#ifndef AWT_I18N_H
#define AWT_I18N_H
#include "libi18n.h"
#include "awt_defs.h"
#include "awt_window.h"
#include "awt_vf.h"

class libi18n {

public:

static const char*	IntlGetJavaFaceName(int csid, int javaFontID);
static BYTE			IntlGetJavaCharset(int csid, int javaFontID);
static int16		IntlMenuFontCsid();
static int			IntlMenuFontID();

static int16*		INTL_GetUnicodeCSIDList(int16 * outnum);
static int32		INTL_UnicodeLen(uint16* ustr);
static INTL_CompoundStr* 
					INTL_CompoundStrFromUnicode(uint16* inunicode, uint32 inlen);
static void			INTL_CompoundStrDestroy(INTL_CompoundStr* This);
static INTL_CompoundStrIterator 
					INTL_CompoundStrFirstStr(INTL_CompoundStr* This, INTL_Encoding_ID *outencoding, unsigned char** outtext);
static INTL_CompoundStrIterator 
					INTL_CompoundStrNextStr(INTL_CompoundStrIterator iterator, INTL_Encoding_ID *outencoding, unsigned char** outtext);
static INTL_CompoundStr* 		INTL_CompoundStrClone(INTL_CompoundStr* s1);

static int32		INTL_TextByteCountToCharLen(int16 csid, unsigned char* text, uint32 byteCount);
static int32		INTL_TextCharLenToByteCount(int16 csid, unsigned char* text, uint32 charLen);
static uint32		INTL_UnicodeToStrLen(INTL_Encoding_ID encoding,INTL_Unicode* ustr, uint32	ustrlen);
static void			INTL_UnicodeToStr(INTL_Encoding_ID encoding,INTL_Unicode* ustr,uint32 ustrlen,unsigned char* dest, uint32 destbuflen);

static jref *		intl_makeJavaString(int16 encoding, char *str, int len);
static char *		intl_allocCString(int16 encoding , Hjava_lang_String *javaStringHandle);

//	Some utility functions
static int16				GetPrimaryEncoding();

static class AwtVFontList*	GetMenuVFontList();
static HDC					GetMenuDC();
static HBITMAP				GetCheckmarkBitmap();
static BOOL					iswin95();

//	Some CIntlWin class wrapper
static BOOL CIntlWin_TextOut(int16 wincsid, HDC hdc, int nXStart, int nYStart, LPCSTR  lpString,int  iLength);
static BOOL CIntlWin_GetTextExtentPoint(int wincsid, HDC hDC, LPCSTR lpString, int cbString, LPSIZE lpSize);

private: 

static uint32			intl_javaStringCommon(Hjava_lang_String *javaStringHandle,WCHAR**    uniChars);
static void			intl_javaString2CString(int16 encoding ,Hjava_lang_String *javaStringHandle, unsigned char *out, uint32 buflen);
static int16		INTL_DefaultWinCharSetID(MWContext *context);

static AwtVFontList*	s_MenuVFontList;
static HDC				s_MenuHDC;
static HBITMAP			s_CheckmarkBitmap;

};
#endif /* AWT_I18N_H */
