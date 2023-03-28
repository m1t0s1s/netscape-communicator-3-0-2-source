#include "awt_object.h"

AwtCList AwtObject::m_liveChain;
AwtMutex AwtObject::m_liveChainMutex;
#ifdef DEBUG
int32 nObjects = 0;
#endif

AwtObject::AwtObject()
{
    //
    // Add the new object the chain of allocated AwtObjects
    //
    m_liveChainMutex.Lock();
    m_liveChain.Append(m_link);
    m_liveChainMutex.Unlock();
#ifdef DEBUG
    nObjects++;
#endif
}


AwtObject::AwtObject(AwtObject& that)
{
    //
    // Add the new object the chain of allocated AwtObjects
    //
    m_liveChainMutex.Lock();
    m_liveChain.Append(m_link);
    m_liveChainMutex.Unlock();
#ifdef DEBUG
    nObjects++;
#endif
}


AwtObject::AwtObject(JHandle *peer)
{
    //
    // Add the new object the chain of allocated AwtObjects
    //
    m_liveChainMutex.Lock();
    m_liveChain.Append(m_link);
    m_liveChainMutex.Unlock();
#ifdef DEBUG
    nObjects++;
#endif

    m_pJavaPeer = peer;
}


AwtObject::~AwtObject()
{
#ifdef DEBUG
    nObjects--;
#endif
    ASSERT( m_pJavaPeer == NULL );
    CLEAR_AWT_SENTINAL;

    //
    // Remove from the chain of allocated AwtObjects
    //
    m_liveChainMutex.Lock();
    m_link.Remove();
    m_liveChainMutex.Unlock();
}


void AwtObject::Dispose(void)
{
    delete this;
}


void AwtObject::DeleteAllObjects(void)
{
    m_liveChainMutex.Lock();

    while (!m_liveChain.IsEmpty()) {
        AwtObject *pObject;

        pObject = (AwtObject *)AWT_OBJECT_PTR( m_liveChain.next );

        // Remove the event from the chain...
        pObject->m_link.Remove();
        delete pObject;
    }

    m_liveChainMutex.Unlock();
}


#ifdef DEBUG_CONSOLE

char*  AwtObject::PrintStatus()
{
    char* buffer = new char[40];
    wsprintf(buffer, "No Status Available on %s\r\n", (char*)this + 4);
    return buffer;
}

#endif
