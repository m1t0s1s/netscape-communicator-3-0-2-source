#include "awt_object.h"

AwtCList AwtObject::m_liveChain;
AwtMutex AwtObject::m_liveChainMutex;

AwtObject::AwtObject()
{
    //
    // Add the new object the chain of allocated AwtObjects
    //
    m_liveChainMutex.Lock();
    m_liveChain.Append(m_link);
    m_liveChainMutex.Unlock();
}


AwtObject::AwtObject(AwtObject& that)
{
    //
    // Add the new object the chain of allocated AwtObjects
    //
    m_liveChainMutex.Lock();
    m_liveChain.Append(m_link);
    m_liveChainMutex.Unlock();
}


AwtObject::AwtObject(JHandle *peer)
{
    //
    // Add the new object the chain of allocated AwtObjects
    //
    m_liveChainMutex.Lock();
    m_liveChain.Append(m_link);
    m_liveChainMutex.Unlock();

    m_pJavaPeer = peer;
}


AwtObject::~AwtObject()
{
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
