/* -*- Mode: C; tab-width: 4 -*- */
/* Region-related definitions and prototypes */

#ifndef _FE_RGN_H_
#define _FE_RGN_H_

/******************Definitions and Types************/

/* This is an opaque data type that corresponds to a */
/* platform-specific region data type.               */
typedef void *FE_Region;

/* A cross-platform rectangle */
typedef struct _XP_Rect {
  int32 top;
  int32 left;
  int32 bottom;
  int32 right;
} XP_Rect;


/*******************Prototypes**********************/

XP_BEGIN_PROTOS

extern FE_Region FE_CreateRegion(void);

/* Creates a region from a rectangle. Returns */
/* NULL if region can't be created.           */
extern FE_Region FE_CreateRectRegion(XP_Rect *rect);

/* Destroys region. */
extern void FE_DestroyRegion(FE_Region region);

/* Makes a copy of a region. If dst is NULL, creates a new region */
extern FE_Region FE_CopyRegion(FE_Region src, FE_Region dst);

/* Set an existing region to a rectangle */
extern FE_Region FE_SetRectRegion(FE_Region region, XP_Rect *rect);

/* dst = src1 intersect sr2       */
/* dst can be one of src1 or src2 */
extern void FE_IntersectRegion(FE_Region src1, FE_Region src2, FE_Region dst);

/* dst = src1 union src2          */
/* dst can be one of src1 or src2 */
extern void FE_UnionRegion(FE_Region src1, FE_Region src2, FE_Region dst);

/* dst = src1 - src2              */
/* dst can be one of src1 or src2 */
extern void FE_SubtractRegion(FE_Region src1, FE_Region src2, FE_Region dst);

/* Returns TRUE if the region contains no pixels */
extern XP_Bool FE_IsEmptyRegion(FE_Region region);

/* Returns the bounding rectangle of the region */
extern void FE_GetRegionBoundingBox(FE_Region region, XP_Rect *bbox);

/* TRUE if rgn1 == rgn2 */
extern XP_Bool FE_IsEqualRegion(FE_Region rgn1, FE_Region rgn2);

/* Moves a region by the specified offsets */
extern void FE_OffsetRegion(FE_Region region, int32 xOffset, int32 yOffset);

/* Is any part of the rectangle in the specified region */
extern XP_Bool FE_RectInRegion(FE_Region region, XP_Rect *rect);

XP_END_PROTOS

#endif
