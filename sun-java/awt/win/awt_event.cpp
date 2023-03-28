#include "awt_event.h"
extern "C" {
#include "javaThreads.h"
#include "java_awt_Event.h"
};

#ifdef DEBUG_CONSOLE
#include "awt_toolkit.h"
#endif

AwtCList JavaEventInfo::m_freeChain;
AwtMutex JavaEventInfo::m_freeChainMutex;


//-------------------------------------------------------------------------
//
// called if java changed the key that was pressed. Figure out what 
// character is and if implies some modifier key
//
//-------------------------------------------------------------------------
UINT JavaEventInfo::JavaCharToWindows(UINT nChar, BYTE *pKeyState) 
{
    switch (nChar) {
        case java_awt_Event_HOME:
            return VK_HOME;
        case java_awt_Event_END:
            return VK_END;
        case java_awt_Event_PGUP:
            return VK_PRIOR;
        case java_awt_Event_PGDN:
            return VK_NEXT;
        case java_awt_Event_UP:
            return VK_UP;
        case java_awt_Event_DOWN:
            return VK_DOWN;
        case java_awt_Event_RIGHT:
            return VK_RIGHT;
        case java_awt_Event_LEFT:
            return VK_LEFT;
        case java_awt_Event_F1:
            return VK_F1;
        case java_awt_Event_F2:
            return VK_F2;
        case java_awt_Event_F3:
            return VK_F3;
        case java_awt_Event_F4:
            return VK_F4;
        case java_awt_Event_F5:
            return VK_F5;
        case java_awt_Event_F6:
            return VK_F6;
        case java_awt_Event_F7:
            return VK_F7;
        case java_awt_Event_F8:
            return VK_F8;
        case java_awt_Event_F9:
            return VK_F9;
        case java_awt_Event_F10:
            return VK_F10;
        case java_awt_Event_F11:
            return VK_F11;
        case java_awt_Event_F12:
            return VK_F12;
        case '\177':
            return VK_DELETE;
        case '\n':
            return VK_RETURN;
        default:
            // this function doesn't back up the value in few cases.
            // nChar is coming from the java event object and is the value
            // we passed to java as obtained by ToAscii. This function should 
            // be the inverse of ToAscii but doesn't change the value at all
            // in the case of <ctrl>C or numeric-pad keys or few other cases (see TranslateToAscii).
            // This is fine on NT but may cause some problem on 95
            UINT wparam = ::VkKeyScan(nChar);

            if (wparam & 0x200) 
            {
                pKeyState[VK_CONTROL] |= 0x80;
            }

            // if CAPS-LOCK is toggled, don't set the SHIFT state or CAPS_LOCK
            // will be ignored
            if((wparam & 0x100) && !(pKeyState[VK_CAPITAL] & 1)) 
            {
                pKeyState[VK_SHIFT] |= 0x80;
            }

            if(wparam & 0x400) 
            {
                pKeyState[VK_MBUTTON] |= 0x80;
                pKeyState[VK_MENU] |= 0x80;
            }

            return (wparam & 0xff);
    } 
}



//-------------------------------------------------------------------------
//
//
//-------------------------------------------------------------------------
void JavaEventInfo::Dispatch(void)
{
    ExecEnv* ee = EE();
    ASSERT( m_eventId < AwtEventDescription::MaxEvent );

    execute_java_dynamic_method( ee, m_object, 
         (char *)AwtEventDescription::m_AwtEventMap[m_eventId].m_methodName, 
         (char *)AwtEventDescription::m_AwtEventMap[m_eventId].m_methodSig, 
         m_timeStamp, (long)this, 
         m_arg1, m_arg2, m_arg3, m_arg4);

    /* XXX This should really go in the java code, but that requires too
       much restructuring right now. */
    if (exceptionOccurred(ee)) {
	Hjava_lang_Thread *p = (Hjava_lang_Thread *)ee->thread;
        if (p && THREAD(p)->group != NULL) {
            void *t = (void *)ee->exception.exc;
            exceptionClear(ee);
            execute_java_dynamic_method(ee, (JHandle*)THREAD(p)->group,
                "uncaughtException", "(Ljava/lang/Thread;Ljava/lang/Throwable;)V", p, t);
        }
	exceptionClear(ee);
    }
}


//-------------------------------------------------------------------------
//
// Returns whether the event should be processed immediately, or should be
// queued (and possibly combined with other 
//-------------------------------------------------------------------------
BOOL JavaEventInfo::DispatchEventNow(void)
{
    return (AwtEventDescription::m_AwtEventMap[m_eventId].m_mangler == NULL);
}


//-------------------------------------------------------------------------
//
//
//-------------------------------------------------------------------------
JavaEventInfo *JavaEventInfo::Create(void)
{
    JavaEventInfo *pInfo;

    m_freeChainMutex.Lock();
 
    if (m_freeChain.IsEmpty()) {
        pInfo = new JavaEventInfo();
    } else {
        pInfo = (JavaEventInfo *)JAVA_EVENT_PTR( m_freeChain.next );
        pInfo->m_link.Remove();
    }

    m_freeChainMutex.Unlock();

    return pInfo;
}



//-------------------------------------------------------------------------
//
//
//-------------------------------------------------------------------------
void JavaEventInfo::Delete(void)
{
    m_freeChainMutex.Lock();
 
    m_link.Remove();
    m_freeChain.Append(m_link);

    m_freeChainMutex.Unlock();
}



//-------------------------------------------------------------------------
//
//
//-------------------------------------------------------------------------
void JavaEventInfo::DeleteFreeChain(void)
{
    m_freeChainMutex.Lock();

    while (!m_freeChain.IsEmpty()) {
        JavaEventInfo *pInfo;

        pInfo = (JavaEventInfo *)JAVA_EVENT_PTR( m_freeChain.next );

        // Place the event back on the free chain...
        pInfo->m_link.Remove();
        delete pInfo;
    }

    m_freeChainMutex.Unlock();
}


//-------------------------------------------------------------------------
//
// Event Mangler for Mouse-Move events
//
//-------------------------------------------------------------------------
struct CombineMouseMove : public EventMangler
{
    virtual BOOL Combine(JavaEventInfo *oldEvent, JavaEventInfo *newEvent) 
    {
        if (oldEvent->m_object == newEvent->m_object) {
            oldEvent->m_arg1      = newEvent->m_arg1;
            oldEvent->m_arg2      = newEvent->m_arg2;
            oldEvent->m_arg3      = newEvent->m_arg3;
            oldEvent->m_timeStamp = newEvent->m_timeStamp;

            return TRUE;
        }
        return FALSE;
    }
};


//-------------------------------------------------------------------------
//
// Event Mangler for Mouse-Drag events
//
//-------------------------------------------------------------------------
struct CombineMouseDrag : public EventMangler
{
    virtual BOOL Combine(JavaEventInfo *oldEvent, JavaEventInfo *newEvent) 
    {
        if (oldEvent->m_object == newEvent->m_object) {
            oldEvent->m_arg1      = newEvent->m_arg1;
            oldEvent->m_arg2      = newEvent->m_arg2;
            oldEvent->m_arg3      = newEvent->m_arg3;
            oldEvent->m_timeStamp = newEvent->m_timeStamp;

            return TRUE;
        }
        return FALSE;
    }
};


//-------------------------------------------------------------------------
//
// Event Mangler for Repaint events
//
//-------------------------------------------------------------------------
struct CombineRepaint : public EventMangler
{
    virtual BOOL Combine(JavaEventInfo *oldEvent, JavaEventInfo *newEvent)
    {
        BOOL bDiscardEvent = FALSE;

        if (oldEvent->m_object == newEvent->m_object) {
            // combining painting rect is to calculate the rect which contains
            // both of the incoming rects (oldEvent and newEvent)
            RECT tempRc;
            bDiscardEvent = TRUE;

            // keep the most left value
            tempRc.left = min(newEvent->m_arg1, oldEvent->m_arg1);

            // keep the toppest value
            tempRc.top = min(newEvent->m_arg2, oldEvent->m_arg2);

            // calculate the new width (new left + the most right value)
            tempRc.right = tempRc.left + max(newEvent->m_arg1 + newEvent->m_arg3, oldEvent->m_arg1 + oldEvent->m_arg3);

            // calculate new height
            tempRc.bottom = tempRc.top + max(newEvent->m_arg2 + newEvent->m_arg4, oldEvent->m_arg2 + oldEvent->m_arg4);

            // update oldEvent
            oldEvent->m_arg1 = tempRc.left;
            oldEvent->m_arg2 = tempRc.top;
            oldEvent->m_arg3 = tempRc.right;
            oldEvent->m_arg4 = tempRc.bottom;

            // change time stanp
            oldEvent->m_timeStamp = newEvent->m_timeStamp;

        }
        return bDiscardEvent;
    }
};


//-------------------------------------------------------------------------
//
// Event Mangler for Resize events
//
//-------------------------------------------------------------------------
struct CombineResize : public EventMangler
{
    virtual BOOL Combine(JavaEventInfo *oldEvent, JavaEventInfo *newEvent)
    {
        if (oldEvent->m_object == newEvent->m_object) {
            oldEvent->m_arg1      = newEvent->m_arg1;
            oldEvent->m_arg2      = newEvent->m_arg2;
            oldEvent->m_timeStamp = newEvent->m_timeStamp;

            return TRUE;
        }
        return FALSE;
    }
};


//-------------------------------------------------------------------------
//
// Each Event Mangler has a static instance, pointed to by the 
// g_EventManglerTable...
//
//-------------------------------------------------------------------------
static CombineMouseMove s_MouseMoveEvent;
static CombineMouseDrag s_MouseDragEvent;
static CombineRepaint   s_RepaintEvent;
static CombineResize    s_ResizeEvent;

//
// The first two parameters of every peer callback function must be 
// the timestamp and pData pointer for the AwtEvent object.
//
// Therefore, every signature must have the form:
//    handleXXX(long timestamp, int pData, int arg1, int arg2, int arg3, int arg4)
//

AwtEventDescription AwtEventDescription::m_AwtEventMap[] = 
{
    { "handleRepaint",          "(JIIIII)V",    NULL /*&s_RepaintEvent*/ },  // Repaint
    { "handleExpose",           "(JIIIII)V",    &s_RepaintEvent },  // Expose
    { "handleMouseMoved",       "(JIIII)V",     &s_MouseMoveEvent},  // MouseMove
    { "handleMouseDrag",        "(JIIII)V",     NULL /*&s_MouseDragEvent*/},  // MouseMove
    { "handleMouseEnter",       "(JIII)V",      NULL                },  // MouseEnter
    { "handleMouseExit",        "(JIII)V",      NULL                },  // MouseExit
    { "handleResize",           "(JIIIII)V",    &s_ResizeEvent      },  // Resize
    { "handleMoved",             "(JIII)V",      NULL                },  // Move
    { "handleQuit",             "(J)V",         NULL                },  // Quit
    { "handleMouseDown",        "(JIIIII)V",    NULL                },  // MouseDown
    { "handleMouseUp",          "(JIIIII)V",    NULL                },  // MouseUp
    { "handleKeyPress",         "(JIIIII)V",    NULL                },  // KeyDown
    { "handleKeyRelease",       "(JIIIII)V",    NULL                },  // KeyUp
    { "handleActionKey",        "(JIIIII)V",    NULL                },  // ActionKeyDown
    { "handleActionKeyRelease", "(JIIIII)V",    NULL                },  // ActionKeyUp

    { "handleAction",           "(JI)V",        NULL                },  // ButtonAction
    { "handleAction",           "(JII)V",       NULL                },  // ListAction
    { "handleAction",           "(JIZ)V",       NULL                },  // CheckAction
    { "handleAction",           "(JI)V",        NULL                },  // TextAction

    { "handleListChanged",      "(JIIZ)V",      NULL                },  // ListChanged
    { "handleLineDown",         "(JII)V",       NULL                },  // LineDown
    { "handleLineUp",           "(JII)V",       NULL                },  // LineUp
    { "handlePageDown",         "(JII)V",       NULL                },  // PageDown
    { "handlePageUp",           "(JII)V",       NULL                },  // PageUp
    { "handleDragAbsolute",     "(JII)V",       NULL                },  // DragAbsolute
    { "handleGotFocus",         "(JIIIII)V",    NULL                },  // GotFocus
    { "handleLostFocus",        "(JIIIII)V",    NULL                },  // LostFocus
    { "handleAction",           "(JI)V",        NULL                },  // Action
};


