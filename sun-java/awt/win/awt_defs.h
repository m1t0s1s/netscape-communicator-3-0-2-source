#ifndef AWT_DEFS_H
#define AWT_DEFS_H

#include "xp_mem.h"	/* for debug malloc packages */

#define STRICT
#include <windows.h>
#include <memory.h>

extern "C" {
    struct execenv;
#ifdef _WIN32
    _declspec(dllexport) void SignalError(struct execenv *, char *, char *);
#else
    void __export __loadds __cdecl SignalError(struct execenv *, char *, char *);
#endif  // !WIN32
}

///extern const char *__awt_dllName;      // Name of our dll for GetModuleHandle (def'd in awt_toolkit.cpp)
#ifdef _WIN32
#define __awt_dllName   NULL
#else
#define __awt_dllName   "awt1630.dll"
#endif

//
// Compatibility routines...
//
#if !defined(_WIN32)
extern BOOL GdiFlush(void);
#endif

#if defined(_WIN32)
#define AWT_CALLBACK CALLBACK
#else
#define AWT_CALLBACK PASCAL __loadds
#endif

//
// Useful Debugging macros
//
#ifdef DEBUG
#ifdef _WIN32
#define BREAK_TO_DEBUGGER           (DebugBreak(),GetLastError())
#else   // !_WIN32 
inline void W16DebugBreak() {
	__asm int 3
}
#define BREAK_TO_DEBUGGER           W16DebugBreak()
#endif  // !_WIN32 

#else   // !DEBUG 
#define BREAK_TO_DEBUGGER
#endif  // !DEBUG 

#ifdef DEBUG
#ifndef ASSERT
#define ASSERT(exp)                 (void) ((exp) ? 0: BREAK_TO_DEBUGGER)
#endif

#define VERIFY(exp)                 ASSERT( (exp) )

#ifdef _WIN32
#define VERIFY32(exp)               ASSERT( (exp) )
#else
#define VERIFY32(exp)               ((void)(exp))
#endif

#else   // !DEBUG

#ifndef ASSERT
#define ASSERT(exp)
#endif

#define VERIFY(exp)                 ((void)(exp))
#define VERIFY32(exp)               ((void)(exp))
#endif  // !DEBUG

#define UNIMPLEMENTED()             ASSERT(0)

//
// Debugging macros which allow any AwtObject pointer to be cast to a
// char * to determine its type and whether it is a valid pointer.
//
// Sentinal values consist of three characters (null terminated).
// If the object has been deleted, the sentinal "DOA" is used.
//
#ifdef DEBUG
#define DECLARE_AWT_SENTINAL        DWORD  __awt_sentinal
#define DEFINE_AWT_SENTINAL(x)      __awt_sentinal = (*(DWORD*)(x))
#define CLEAR_AWT_SENTINAL          __awt_sentinal = (*(DWORD*)("DOA"))

#else   // !DEBUG

#define DECLARE_AWT_SENTINAL    
#define DEFINE_AWT_SENTINAL(x)
#define CLEAR_AWT_SENTINAL
#endif

//
// Macros for hiding native thread syncronization mechanisms.
//
// These macros provide syncronization in situations where "true preemption" 
// could cause thread-safety problems.  If "true preemption" is not 
// an issue (ie. WIN16), then the macros do nothing.
//
#ifdef _WIN32
#define DECLARE_AWT_MUTEX(m)        CRITICAL_SECTION m
#define CREATE_AWT_MUTEX(m)         InitializeCriticalSection( &(m) )
#define LOCK_AWT_MUTEX(m)           EnterCriticalSection( &(m) )
#define UNLOCK_AWT_MUTEX(m)         LeaveCriticalSection( &(m) )
#define FREE_AWT_MUTEX(m)           DeleteCriticalSection( &(m) );

struct AwtMutex {
    CRITICAL_SECTION m_mutex;

         AwtMutex()   { ::InitializeCriticalSection(&m_mutex); }
         ~AwtMutex()  { ::DeleteCriticalSection(&m_mutex);     }
    void Lock(void)   { ::EnterCriticalSection(&m_mutex);      }
    void Unlock(void) { ::LeaveCriticalSection(&m_mutex);      }
};


#else   // !_WIN32

#define DECLARE_AWT_MUTEX(m)
#define CREATE_AWT_MUTEX(m)
#define LOCK_AWT_MUTEX(m)
#define UNLOCK_AWT_MUTEX(m)
#define FREE_AWT_MUTEX(m)

struct AwtMutex {
         AwtMutex()   { }
         ~AwtMutex()  { }
    void Lock(void)   { }
    void Unlock(void) { }
};

#endif // !_WIN32


#define CHECK_NULL(p, m)                                    \
    if( (p) == NULL ) {                                     \
        BREAK_TO_DEBUGGER;                                  \
        return;                                             \
    }

#define CHECK_NULL_AND_RETURN(p, m, r)                      \
    if( (p) == NULL ) {                                     \
        BREAK_TO_DEBUGGER;                                  \
        return (r);                                         \
    }

#define CHECK_OBJECT(p)                                     \
    if (((p)==NULL) || (unhand(p)==NULL)) {                 \
        BREAK_TO_DEBUGGER;                                  \
        return;                                             \
    }                                       

#define CHECK_OBJECT_AND_RETURN(p, r)                       \
    if (((p)==NULL) || (unhand(p)==NULL)) {                 \
        BREAK_TO_DEBUGGER;                                  \
        return (r);                                         \
    }                                       

#define CHECK_PEER(p)                                       \
    if (((p)==NULL) || (unhand(p)==NULL) ||                 \
        (unhand(p)->pNativeWidget==NULL)) {                 \
        BREAK_TO_DEBUGGER;                                  \
        return;                                             \
    }                                       

#define CHECK_MENU_PEER(p)                                       \
    if ((p==NULL) || (unhand(p)==NULL) ||                   \
        (unhand(p)->pNativeMenu==NULL)) {                 \
        BREAK_TO_DEBUGGER;                                  \
        return;                                             \
    }                                       

#define CHECK_MENU_PEER_AND_RETURN(p, r)                                       \
    if ((p==NULL) || (unhand(p)==NULL) ||                   \
        (unhand(p)->pNativeMenu==NULL)) {                 \
        BREAK_TO_DEBUGGER;                                  \
        return (r);                                             \
    }                                       

#define CHECK_PEER_AND_RETURN(p, r)                         \
    if (((p)==NULL) || (unhand(p)==NULL) ||                 \
        (unhand(p)->pNativeWidget==NULL)) {                 \
        BREAK_TO_DEBUGGER;                                  \
        return (r);                                         \
    }                                       

#define CHECK_GRAPHICS_PEER(p)                              \
    if (((p)==NULL) || (unhand(p)==NULL) ||                 \
        (unhand(p)->pData==NULL)) {                         \
        BREAK_TO_DEBUGGER;                                  \
        return;                                             \
    }                                       

#define CHECK_GRAPHICS_PEER_AND_RETURN(p, r)                \
    if (((p)==NULL) || (unhand(p)==NULL) ||                 \
        (unhand(p)->pData==NULL)) {                         \
        BREAK_TO_DEBUGGER;                                  \
        return (r);                                         \
    }                                       

#define CHECK_ALLOCATION(p) {                               \
    if( (p) == NULL ) {                                     \
        BREAK_TO_DEBUGGER;                                  \
        SignalError(0, JAVAPKG "OutOfMemoryError", 0);      \
        return;                                             \
    }                                                       \
}

#define CHECK_ALLOCATION_AND_RETURN(p, r) {                 \
    if( (p) == NULL ) {                                     \
        BREAK_TO_DEBUGGER;                                  \
        SignalError(0, JAVAPKG "OutOfMemoryError", 0);      \
        return (r);                                         \
    }                                                       \
}

#define CHECK_BOUNDARY_CONDITION(p, m)                           \
    if( (p) == TRUE ) {                                          \
        BREAK_TO_DEBUGGER;                                       \
        SignalError(0, JAVAPKG "Boundary Condition Failed", (m));\
        return;                                                  \
    }

#define CHECK_BOUNDARY_CONDITION_RETURN(p, m, r)                 \
    if( (p) == TRUE ) {                                          \
        BREAK_TO_DEBUGGER;                                       \
        SignalError(0, JAVAPKG "Boundary Condition Failed", (m));\
        return(r);                                               \
    }

    
#define SET_OBJ_IN_PEER(h, p)           unhand((h))->pNativeWidget = (DWORD)(p)
#define GET_OBJ_FROM_PEER(t, h)         ((t *)unhand((h))->pNativeWidget)

#define SET_OBJ_IN_MENU_PEER(h, p)      unhand((h))->pNativeMenu = (DWORD)(p)
#define GET_OBJ_FROM_MENU_PEER(t, h)    (t *)unhand((h))->pNativeMenu

#define GET_OBJ_FROM_FONT(h)            (AwtFont *)unhand((h))->pData

#define GET_OBJ_FROM_GRAPHICS(h)        (AwtGraphics *)unhand((h))->pData

#define DECLARE_SUN_AWT_WINDOWS_PEER(x)    typedef struct Classsun_awt_windows_##x x; \
                                           typedef struct Hsun_awt_windows_##x H##x

#define DECLARE_SUN_AWT_IMAGE_PEER(x)    typedef struct Classsun_awt_image_##x x; \
                                         typedef struct Hsun_awt_image_##x H##x

#define DECLARE_JAVA_AWT_IMAGE_PEER(x)    typedef struct Classjava_awt_image_##x x; \
                                         typedef struct Hjava_awt_image_##x H##x

#define DECLARE_JAVA_AWT_PEER(x)    typedef struct Classjava_awt_##x x; \
                                    typedef struct Hjava_awt_##x H##x



#define AWT_OBJECT     0
#define AWT_COMPONENT  1
#define AWT_IMAGE      2

#endif  // AWT_DEFS_H


