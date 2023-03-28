#ifndef AWT_FONT_H
#define AWT_FONT_H

#include "awt_defs.h"
#include "awt_object.h"
#include "awt_resource.h"

#define AWT_FONT_KEY_SIZE   (LF_FACESIZE + 10)

//
// Foreward definitions...
//
struct Hjava_awt_Font;


DECLARE_SUN_AWT_WINDOWS_PEER(WFontMetrics);

class AwtFont : public AwtObject
{
public:
    static  AwtFont *       FindFont(Hjava_awt_Font *peer);
            void            Release(void);
            void            AddRef(void);

    virtual void            Dispose(void);

    virtual BOOL            CallMethod(AwtMethodInfo *info);

            void            Select(HDC hDC, BOOL fFlag);

            HFONT           GetFont(void) { return m_hFont; }

    //
    // Methods for manipulating FontMetrics
    //
            void            LoadFontMetrics(HWFontMetrics *self);

            long            StringWidth(HWFontMetrics *self, 
                                        Hjava_lang_String *string);

            long            StringWidth(HWFontMetrics *self, 
                                        HArrayOfChar *string, 
                                        long offset, long len);

            long            StringWidth(HWFontMetrics *self, 
                                        HArrayOfByte *buffer, 
                                        long offset, long len);

    static  AwtFont *       GetFontFromObject(Hjava_awt_Font *peer);
            int             GetFontAscent(){return m_nAscent;}

#ifdef INTLUNICODE
	    HGDIOBJ	    GetFontForEncoding(int16 encoding);
#endif
protected:
                            AwtFont(Hjava_awt_Font *self);
    virtual                 ~AwtFont();

            void            Initialize(Hjava_awt_Font *peer);

private:
    static  void            GetFontKey(Hjava_awt_Font *pFontObject, 
                                       char *buffer, int len);

    static  const char *    MapFamilyToFont(Hjava_awt_Font *pFontObject);


    //
    // Data members:
    //
protected:
    AwtCList        m_link;
    DWORD           m_refCount;
    DWORD           m_lockCount;
    HFONT           m_hFont;
    int             m_nAscent;

    char            m_fontKey[ AWT_FONT_KEY_SIZE ];
    static AwtCList m_fontList;
    static AwtMutex m_fontMutex;

#ifdef INTLUNICODE
    class AwtVFontList	*m_vFontList;
#endif

    //
    // Mapping between a platform-independ java font family and the Windows
    // specific font (ie. face name) which represents the family...
    //
    typedef struct AwtFaceMapStruct 
    {
        const char * family;
        const char * faceName;
    } AwtFaceMap;

    static AwtFaceMap	m_faceNameMap[];
};



#endif  // AWT_FONT_H
