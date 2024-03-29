/* -*- Mode: C++; tab-width: 4; tabs-indent-mode: nil -*- */
// 
// Warning:  This is a C++ file.  
//
//
// This implements cross platform Growable arrays of Pointers.
//
#ifndef _GARRAY_H_
#define _GARRAY_H_

//
//    On Unix (well at least Solaris) we are having troubles with
//    templates, so hey, we won't use them...djw.
//
//    On Mac we are having troubles as well, so add me to the list.
//    Now, why even have templates?...jar
//
#if ! ( defined(XP_WIN16) || defined(XP_UNIX) || defined(XP_MAC) )
#define TEMPLATE_SUPPORT    1
#endif


class CXP_GrowableArray {
protected:
    void **m_pData;
    int m_iSize;
    int m_iAllocSize;

    int NewSize( int iMinSize){
        int iNewSize = max( m_iAllocSize,16) ;
        while( iNewSize < iMinSize ){
            iNewSize = iNewSize+iNewSize;
        }
        return iNewSize;
    }

    //
    // this is the routine that does the actual work.  Should be in 
    //  its own file.
    //
    void GuaranteeSize(int iSize){ 
        if(m_iAllocSize <= iSize){
            int iNewSize = NewSize( iSize );
            if( m_iAllocSize ){
                void ** pNewData = new void*[iNewSize];
                XP_BCOPY( m_pData, pNewData, m_iAllocSize * sizeof(void*) );
                delete [] m_pData;
                m_pData = pNewData;
            }
            else{
                m_pData = new void*[iNewSize];
            }
            m_iAllocSize = iNewSize;
        } 
    }

public:
    CXP_GrowableArray(int iStartSize=0): m_iSize(0),m_iAllocSize(0),m_pData(0){ 
        if( iStartSize ){
            GuaranteeSize( iStartSize );
        }
    };

    ~CXP_GrowableArray(){ delete [] m_pData; }

    int Size(){ return m_iSize; }

    void SetSize( int iSize ){ 
        GuaranteeSize( iSize );
        m_iSize = iSize;
    }

    void* operator[](int nIndex) const { return m_pData[nIndex]; }
    void*& operator[](int nIndex){ return m_pData[nIndex]; }

    int Add(void* newElement){ 
        GuaranteeSize(m_iSize+1);
        m_pData[m_iSize] = newElement;
        return m_iSize++;
    }

    void Empty(){
        m_iSize = 0;
    }
};

class CXP_PtrStack : public CXP_GrowableArray{
public:
    int m_iTop;
    CXP_PtrStack(): m_iTop(-1){}
    Bool IsEmpty(){ return m_iTop == -1; }
    void Push( void* t ){ 
        if( ++m_iTop >= Size() ) {
            Add( t );
        }
        else {
            (*this)[m_iTop] = t;
        }
    }
    void* Top(){ return (*this)[m_iTop]; }
    void* Pop(){ return (*this)[m_iTop--];}
    void Reset(){ m_iTop = -1; }
    int StackSize() { return m_iTop + 1; }
};

#ifdef TEMPLATE_SUPPORT

template<class PTRTYPE>
class TXP_GrowableArray: public CXP_GrowableArray {
public:
    PTRTYPE operator[](int nIndex) const { return (PTRTYPE)(int32)m_pData[nIndex]; }
    PTRTYPE& operator[](int nIndex){ return *(PTRTYPE*)&m_pData[nIndex]; }
    int Add(PTRTYPE newElement){ return CXP_GrowableArray::Add( (void*) newElement ); }
};

#define Declare_GrowableArray(NAME,PTRTYPE) \
    typedef TXP_GrowableArray<PTRTYPE> TXP_GrowableArray_##NAME;

#else

#define Declare_GrowableArray(NAME,PTRTYPE)                                 \
    class TXP_GrowableArray_##NAME: public CXP_GrowableArray {                          \
    public:                                                                             \
        PTRTYPE operator[](int nIndex) const { return (PTRTYPE)(int32)m_pData[nIndex]; }\
        PTRTYPE& operator[](int nIndex){ return *(PTRTYPE*)&m_pData[nIndex]; }          \
        int Add(PTRTYPE newElement){ return CXP_GrowableArray::Add( (void*) newElement ); } \
    };                                                                                 \


#endif

//
// PtrStack Imlementation
//
#ifdef TEMPLATE_SUPPORT
template<class PTRTYPE>
class TXP_PtrStack : public TXP_GrowableArray<PTRTYPE> {
public:
    int m_iTop;
    TXP_PtrStack(): m_iTop(-1){}
    Bool IsEmpty(){ return m_iTop == -1; }
    void Push( PTRTYPE t ){ 
        if( ++m_iTop >= Size() ) {
            Add( t );
        }
        else {
            (*this)[m_iTop] = t;
        }
    }
    PTRTYPE Top(){ return (*this)[m_iTop]; }
    PTRTYPE Pop(){ return (*this)[m_iTop--];}
    void Reset(){ m_iTop = -1; }
    int StackSize(){ return m_iTop + 1; }
};

#define Declare_PtrStack(NAME,PTRTYPE) \
    typedef TXP_PtrStack<PTRTYPE> TXP_PtrStack_##NAME;

#else // No template support

#define Declare_PtrStack(NAME, PTRTYPE) \
    class TXP_PtrStack_##NAME : public CXP_PtrStack {                   \
    public:                                                             \
        void Push( PTRTYPE t ){ CXP_PtrStack::Push((void*)(int32)t); }  \
        PTRTYPE Top(){ return (PTRTYPE)(int32)CXP_PtrStack::Top(); }    \
        PTRTYPE Pop(){ return (PTRTYPE)(int32)CXP_PtrStack::Pop(); }    \
        PTRTYPE operator[](int nIndex) const { return (PTRTYPE)(int32)m_pData[nIndex]; }\
        PTRTYPE& operator[](int nIndex){ return *(PTRTYPE*)&m_pData[nIndex]; }          \
        int Add(PTRTYPE newElement){ return CXP_GrowableArray::Add( (void*)(int32)newElement ); } \
    };                                                                  \


#endif

#endif

