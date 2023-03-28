#ifndef AWT_OBJECT_H
#define AWT_OBJECT_H

#include "awt_defs.h"
#include "awt_clist.h"

// All java runtime header files are extern "C"
extern "C" {
#include "oobj.h"       // JHandle
#include "monitor.h"    // monitorEnter / monitorExit
};

//
// Foreward declarations
//
struct AwtMethodInfo;

#define AWT_OBJECT_PTR(_qp) \
    ((AwtObject*) ((char*) (_qp) - offsetof(AwtObject, m_link)))

//
// AwtObject is the base class for all Awt components which have an
// associated java peer object.
//
class AwtObject 
{
protected:
    DECLARE_AWT_SENTINAL;

public:
                            AwtObject();
                            AwtObject(JHandle *peer);
                            AwtObject(AwtObject &that);
    virtual                 ~AwtObject();

    //
    // Free all resources and prepare to be destroyed.  This method
    // is called when the java peer object is destroyed.
    //
    virtual void            Dispose(void);

    virtual BOOL            CallMethod(AwtMethodInfo *info) = 0;

    //
    // Syncronization methods to lock and unlock the object.
    //
    inline  void            Lock    (void) { monitorEnter((uint32)this);  }
    inline  void            Unlock  (void) { monitorExit ((uint32)this);  }
    inline  BOOL            IsLocked(void) { return TRUE; /* REMIND: how?? */ }

    //
    // Return the handle to the java Peer object.
    //
    // This reference lives in the heap and is not checked by the garbage
    // collector.  Therefore, it can NEVER be the sole reference to a
    // the peer object.
    //
    inline  JHandle *       GetPeer(void) { return m_pJavaPeer; };

    int     GetAwtObjectType(){return m_awtObjType;}

    static  void            DeleteAllObjects(void);
#ifdef DEBUG_CONSOLE
    virtual char* PrintStatus();

    //friend class AwtObject;
#endif

    //
    // Data members:
    //
protected:
    //
    // Java object associated with the AwtObject. Since this pointer
    // does not live in the GC heap, it is vital to clear it when
    // Dispose() is called.  This indicates that there is NO java
    // object associated with the AwtObject.
    //
    JHandle *m_pJavaPeer;
    int      m_awtObjType;

    //
    // The following data members are used to maintain a chain of all
    // allocated AwtObject instances.  This chain is traversed at 
    // shutdown to clean up any dangling instances...
    //
    static AwtCList m_liveChain;
    static AwtMutex m_liveChainMutex;
    AwtCList        m_link;
};


//
// Structure used for passing the information necessary for synchronously 
// invoking a method on the GUI thread...
//
struct AwtMethodInfo {
    AwtObject * target;
    UINT        methodId;
    DWORD       p1;
    DWORD       p2;

    AwtMethodInfo(AwtObject *obj, UINT id, DWORD arg1=0, DWORD arg2=0) {
        target   = obj;
        methodId = id;
        p1       = arg1;
        p2       = arg2;
    }
    
    BOOL Invoke() { return target->CallMethod(this); }
};

#endif  // AWT_OBJECT_H
