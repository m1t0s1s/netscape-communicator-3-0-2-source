#ifndef AWT_EVENT_H
#define AWT_EVENT_H

#include "awt_defs.h"
#include "awt_object.h"
#include "awt_clist.h"
#include "memory.h"     // memset()
//
// Foreward declarations
//
struct EventMangler;


//-------------------------------------------------------------------------
//
// 
//-------------------------------------------------------------------------
struct AwtEventDescription {
    enum EventId {
        Repaint      = 0,
        Expose,
        MouseMove,
        MouseDrag,
        MouseEnter,
        MouseExit,
        Resize,
        Move,
        Quit,
        MouseDown,
        MouseUp,
        KeyDown,
        KeyUp,
        ActionKeyDown,
        ActionKeyUp,
        ButtonAction,
        ListAction,
        CheckAction,
        TextAction,
        ListChanged,
        LineDown,
        LineUp,
        PageDown,
        PageUp,
        DragAbsolute,
        GotFocus,
        LostFocus,
        Action,
        MaxEvent
    };

    const char   *m_methodName;
    const char   *m_methodSig;
    EventMangler *m_mangler;

    static AwtEventDescription m_AwtEventMap[];
};





//-------------------------------------------------------------------------
//
// The JavaEventInfo is used to hold the information about an Awt event
// while it is waiting to be dispatched into java by the Callback thread
//
//-------------------------------------------------------------------------
#define JAVA_EVENT_PTR(_qp) \
    ((JavaEventInfo*) ((char*) (_qp) - offsetof(JavaEventInfo, m_link)))

struct JavaEventInfo 
{
    AwtCList                        m_link;

    JHandle *                       m_object;
    AwtEventDescription::EventId    m_eventId;
    int64                           m_timeStamp;
    long                            m_arg1;
    long                            m_arg2;
    long                            m_arg3;
    long                            m_arg4;
    long                            m_arg5;

    MSG                             m_msg;

    void Initialize(JHandle *obj, AwtEventDescription::EventId id, 
                    int64 time, MSG *pMsg,
                    long p1=0, long p2=0, long p3=0, long p4=0, long p5=0)
    {
        m_object     = obj;
        m_eventId    = id;
        m_timeStamp  = time;
        m_arg1       = p1;
        m_arg2       = p2;
        m_arg3       = p3;
        m_arg4       = p4;
        m_arg5       = p5;
        
        if (pMsg) {
            m_msg = *pMsg;
        } else {
            memset(&m_msg, 0, sizeof(MSG));
        }
    }

    void AddToQueue(AwtCList &queue) {
        m_link.AppendToList(queue);
    }

    static UINT JavaCharToWindows(UINT nChar, BYTE *pKeyState);
    static JavaEventInfo * Create(void);

    BOOL DispatchEventNow(void);
    void Dispatch(void);
    void Delete(void);

    static void DeleteFreeChain(void);

private:
    static AwtCList m_freeChain;
    static AwtMutex m_freeChainMutex;
};




//-------------------------------------------------------------------------
//
// Event Manglers are used to coalesce new events with existing events of
// the same type that are currently pending in the event queue.
//
// Any event type that should be coalesced has an EventMangler that knows
// how the combination should be done...
//
//-------------------------------------------------------------------------
struct EventMangler
{
    EventMangler() {}

    virtual BOOL Combine(JavaEventInfo *oldEvent, JavaEventInfo *newEvent) = 0;
};



#endif // AWT_EVENT_H
