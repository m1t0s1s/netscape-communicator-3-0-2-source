#ifndef AWT_RESOURCE_H
#define AWT_RESOURCE_H

#include "awt_defs.h"
#include "awt_clist.h"

#define RESOURCE_PTR(_qp) \
    ((AwtCachedResource*) ((char*) (_qp) - offsetof(AwtCachedResource,m_link)))


struct AwtCachedResource
{
    AwtCList m_link;
    DWORD    m_refCount;
    DWORD    m_lockCount;
    HANDLE   m_hObject;

    void Initialize(void) {
        m_refCount = 1;
        m_lockCount = 0;
        m_hObject = NULL;
    }

#ifdef DEBUG_CONSOLE
    virtual char* PrintStatus() {return NULL;}
#endif

};


class AwtCachedBrush : public AwtCachedResource
{
public:
    static  AwtCachedBrush* Create(COLORREF color);
    static  void            DeleteCache(void);

            void            Select(HDC hDC, BOOL fFlag);
            void            AddRef();
            void            Release(void);

            COLORREF        GetColor(void) { return m_color; }
            HBRUSH          GetBrush(void) { return (HBRUSH)m_hObject; }

private:
                            AwtCachedBrush(void);
                            ~AwtCachedBrush();

            void            Initialize(COLORREF color);

    static AwtMutex m_cacheMutex;
    static AwtCList m_freeChain;
    static AwtCList m_liveChain;

    COLORREF m_color;
};


class AwtCachedPen : public AwtCachedResource 
{
public:
    static  AwtCachedPen*   Create(COLORREF color);
    static  void            DeleteCache(void);

            void            Select(HDC hDC, BOOL fFlag);
            void            AddRef();
            void            Release(void);

            COLORREF        GetColor(void) { return m_color; }
            HPEN            GetPen  (void) { return (HPEN)m_hObject; }

private:
                            AwtCachedPen(void);
                            ~AwtCachedPen();

            void            Initialize(COLORREF color);


    static AwtMutex m_cacheMutex;
    static AwtCList m_freeChain;
    static AwtCList m_liveChain;

    COLORREF m_color;
};


#endif  // AWT_RESOURCE_H