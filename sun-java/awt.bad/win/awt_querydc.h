#ifndef AWT_QUERYDC_H
#define AWT_QUERYDC_H

#include "awt_defs.h"
#include "awt_object.h"

// All java runtime header files are extern "C"
extern "C" {
#include "oobj.h"       // JHandle
#include "monitor.h"    // monitorEnter / monitorExit
};


/*
** Awt graphics common defines used in AwtGraphics and AwtDC code.
*/


#define AWT_QUERYRES_MASKBIT_BRUSH     (int)(0x0001)
#define AWT_QUERYRES_MASKBIT_PEN       (int)(0x0002)
#define AWT_QUERYRES_MASKBIT_PALETTE   (int)(0x0004)
#define AWT_QUERYRES_MASKBIT_FONT      (int)(0x0008)
#define AWT_QUERYRES_MASKBIT_ROP2      (int)(0x0010)
#define AWT_QUERYRES_MASKBIT_BGBRUSH   (int)(0x0020)
#define AWT_QUERYRES_MASKBIT_RGN       (int)(0x0040)
#define AWT_QUERYRES_MASKBIT_TXTCOLOR  (int)(0x0080)
#define AWT_QUERYRES_MASKBIT_BKMODE    (int)(0x0100)


#define AWT_QUERYRES_BITSET(newMask) \
  m_selectMask |= newMask

#define AWT_QUERYRES_BITCLEAR(newMask) \
  m_selectMask &= ~newMask

#define AWT_QUERYRES_MASK_CLEAR \
  m_selectMask = 0

#define AWT_QUERYRES_BITTEST(newMask) \
  m_selectMask & newMask


#define AWT_QUERYRES_BITSET_ALL  \
  AWT_QUERYRES_MASK_CLEAR;       \
  AWT_QUERYRES_BITSET(           \
  (AWT_QUERYRES_MASKBIT_BRUSH    \
  | AWT_QUERYRES_MASKBIT_PEN     \
  | AWT_QUERYRES_MASKBIT_PALETTE \
  | AWT_QUERYRES_MASKBIT_ROP2    \
  | AWT_QUERYRES_MASKBIT_FONT))  

#define AWT_QUERYRES_BITSET_BRUSH \
  AWT_QUERYRES_MASK_CLEAR;        \
  AWT_QUERYRES_BITSET(            \
  (AWT_QUERYRES_MASKBIT_BRUSH     \
    | AWT_QUERYRES_MASKBIT_PALETTE)) 

#define AWT_QUERYRES_BITSET_BGBRUSH \
  AWT_QUERYRES_MASK_CLEAR;        \
  AWT_QUERYRES_BITSET(            \
  (AWT_QUERYRES_MASKBIT_BGBRUSH   \
    | AWT_QUERYRES_MASKBIT_PALETTE)) 

#define AWT_QUERYRES_BITSET_PEN   \
  AWT_QUERYRES_MASK_CLEAR;        \
  AWT_QUERYRES_BITSET(            \
  (AWT_QUERYRES_MASKBIT_PEN       \
  | AWT_QUERYRES_MASKBIT_ROP2     \
  | AWT_QUERYRES_MASKBIT_FONT     \
  | AWT_QUERYRES_MASKBIT_PALETTE)) 


#define AWT_QUERYRES_BITSET_BRUSH_PEN \
  AWT_QUERYRES_MASK_CLEAR;            \
  AWT_QUERYRES_BITSET(                \
  (AWT_QUERYRES_MASKBIT_BRUSH         \
  | AWT_QUERYRES_MASKBIT_ROP2         \
  | AWT_QUERYRES_MASKBIT_PEN          \
  | AWT_QUERYRES_MASKBIT_PALETTE)) 




class AwtQueryDC : public AwtObject 
{
  public:
  virtual DWORD       GetROP2(void)      { return R2_COPYPEN; }
  virtual COLORREF    GetColor(void)     { return RGB(0, 0, 0); }
  virtual HFONT       GetFont(void)      { return NULL; }
  virtual UINT        GetId(void)        { return 0; }
  virtual HPEN        GetPen(void)       { return NULL; }
  virtual HBRUSH      GetBrush(void)     { return NULL; }
  virtual HPALETTE    GetPalette(void)   { return NULL; }
  virtual int         GetSelectMask(void){ return 0; }
  virtual HBITMAP     GetBitmap(void)    { return NULL; }
};

#endif  // AWT_QUERYDC_H
