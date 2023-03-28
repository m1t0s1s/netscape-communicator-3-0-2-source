// TABS 3
//
// Bits array implementation
//
// Lloyd W. Tabb
//
#ifndef _bits_h_
#define _bits_h_

//
//

#ifdef min
#undef min
#endif
#define min(a, b) (((a) < (b)) ? (a) : (b))
#ifdef max
#undef max
#endif
#define max(a, b) (((a) > (b)) ? (a) : (b))

class BitReference {
public:
    uint8 *m_pBits;
    uint8 m_mask;
    BitReference(uint8* pBits, uint8 mask){
        m_pBits = pBits;
        m_mask = mask;
    }

    int operator= (int i){
        *m_pBits = (i ? *m_pBits | m_mask : *m_pBits & ~m_mask);
        return !!i;
    }

    operator int(void){
        return !!(*m_pBits & m_mask);
    }
};

#if 00				/* stupid Metrowerks compiler doesn't seem to handle this type of template */


template <int iSize>
class TBitArray {
private:
    uint8 m_Bits[iSize/8+1];
public:
    BitArray(){
        memset(&m_Bits,0, iSize/8+1);
    }

    BitReference operator [] (int i){
        return BitReference(&m_Bits[i >> 3], 1 << (i & 7));
    }

};



#endif


#define BIT_ARRAY_END   -1

class CBitArray {
private:
   uint8 *m_Bits;
   long size;
public:
   //
   //  This constructor is unused, and link clashes with 
   //  CBitArray(long n, int iFirst, ...) for losing Cfront based
   //  compilers (HP, SGI 5.2, ...).
   //
#if 0
   CBitArray(long n=0) : m_Bits(0), size(0)  {
      if( n ){
         SetSize(n);
      }
   }
#endif

   //
   //  Call this constructor with a maximum number, a series of bits and 
   //  BIT_ARRAY_END  for example: CBitArray a(100, 1 ,4 9 ,7, BIT_ARRAY_END);
   //
   //  CBitArray::CBitArray(long n, int iFirst, ...) is now edtutil.cpp.
   //  Had to move it into a C++ file so that it could be non-inline.
   //  varargs/inline is a non-portable combo.
   // 
   CBitArray(long n, int iFirst, ...);

   void SetSize(long n){
      uint8 *oldBits = m_Bits;
      m_Bits = new uint8[n/8+1];
      memset(m_Bits,0,n/8+1);
      if(oldBits){
         memcpy(m_Bits,oldBits,min(n,size)/8+1);
      }
      size = n;
   }

   long Size(){ return size; }

   BitReference operator [] (int i){
      XP_ASSERT(i >= 0 && i < size );
      return BitReference(&m_Bits[i >> 3], 1 << (i & 7));
   }

   void ClearAll(){
      memset(m_Bits,0, size/8+1);
   }
};



#if 00
//
// This template will generate a Set that can contain all possible values.
//  for an enum.  Sets are created empty.
//
//  ASSUMES:
//    all enum elements are contiguous.
//
//  EXAMPLE:
//
//    enum CType {
//       ctAlpha,
//       ctVoul,
//       ctDigit,
//       ctPunctuation,
//
//       ctMax,
//    };
//
//    typedef TSet<CType,ctMax> Set_CType;
//
//    Set_CType x;
//    x.Add(ctAlpha);         // x has the attribute alpha
//    x.Add(ctVoul);          // x has the attribute Voul
//
//
template <class SetEnum, int setMax>
class TSet {
    char bits[setMax/8+1];
public:
   TSet(){
      memset(bits,0,sizeof(bits));
   }

   //
   // check to see if enum value is in the set
   //
   int In( SetEnum v ){
      return !!(bits[v>>3] & (1<<(v & 7)));
   }

   //
   // Remove Enum from the set
   //
   void Remove( SetEnum v ){
     // bits[v/8] &= ~(1<<v%8);
     bits[v>>3] &= ~(1<< (v & 7));
   }

   //
   // Add Enum to the set
   //
   void Add( SetEnum v ){
     //bits[v/8] |= 1<<v%8;
     bits[v>>3] |= 1<< (v & 7);
   }

};

#endif

#endif
