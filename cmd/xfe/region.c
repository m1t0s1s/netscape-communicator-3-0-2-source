/* -*- Mode: C; tab-width: 4 -*-
 *   region.cpp --- FE specific region operations
 *   Copyright � 1996 Netscape Communications Corporation, all rights reserved.
 */

#include "mozilla.h"
#include "xfe.h"
#include "fe_rgn.h"

/* Creates an empty region. Returns NULL if region can't be created */
FE_Region
FE_CreateRegion()
{
  return (FE_Region)XCreateRegion();
}

/* Creates a region from a rectangle. Returns NULL if region can't be created */
FE_Region
FE_CreateRectRegion(XP_Rect *rect)
{
  XPoint points[4];

  points[0].x = points[3].x = (short)rect->left;  /* points[] corresponds to */
  points[0].y = points[1].y = (short)rect->top;   /* top-left, top-right,    */
  points[1].x = points[2].x = (short)rect->right; /* bot-left and bot-right  */
  points[3].y = points[2].y = (short)rect->bottom;/* respectively.           */

  return (FE_Region)XPolygonRegion(points, 4, EvenOddRule);
                                /* Fill rule is irrelevant since there */
                                /* should be no self overlap.          */
}

/* Set an existing region to a rectangle.                                  */
/* The purpose of this routine is to avoid the overhead of destroying and  */
/* recreating a region on the Windows platform.  Unfortunately, it doesn't */
/* do much for X.                                                          */
FE_Region 
FE_SetRectRegion(FE_Region region, XP_Rect *rect)
{
  XRectangle rectangle;

  XP_ASSERT(region);

  if (!XEmptyRegion((Region)region))    /* If region is not empty.      */
    XSubtractRegion((Region)region, (Region)region, (Region)region);
                                        /* Subtract region from itself. */
  XP_ASSERT(XEmptyRegion((Region)region));

  rectangle.x = (short)rect->left;
  rectangle.y = (short)rect->top;
  rectangle.width = (unsigned short)(rect->right - rect->left);
  rectangle.height = (unsigned short)(rect->bottom - rect->top);

  return (FE_Region)XUnionRectWithRegion(&rectangle, (Region)region,
                                         (Region)region);
}

/* Destroys region */
void
FE_DestroyRegion(FE_Region region)
{
  XP_ASSERT(region);
  XDestroyRegion((Region)region);
}

/* Make a copy of a region */
FE_Region
FE_CopyRegion(FE_Region src, FE_Region dst)
{
  Region copyRegion;

  XP_ASSERT(src);

  if (dst != NULL) {
    copyRegion = (Region)dst;
  }
  else {
	/* Create an empty region */
	copyRegion = XCreateRegion();
	
	if (copyRegion == NULL)
	  return NULL;
  }

  /* Copy the region */
  XUnionRegion((Region)src, (Region)src, (Region)copyRegion);

  return (FE_Region)copyRegion;
}

/* dst = src1 intersect sr2       */
/* dst can be one of src1 or src2 */
void
FE_IntersectRegion(FE_Region src1, FE_Region src2, FE_Region dst)
{
  XP_ASSERT(src1);
  XP_ASSERT(src2);
  XP_ASSERT(dst);

  XIntersectRegion((Region)src1, (Region)src2, (Region)dst);
}

/* dst = src1 union src2          */
/* dst can be one of src1 or src2 */
void
FE_UnionRegion(FE_Region src1, FE_Region src2, FE_Region dst)
{
  XP_ASSERT(src1);
  XP_ASSERT(src2);
  XP_ASSERT(dst);

  XUnionRegion((Region)src1, (Region)src2, (Region)dst);
}

/* dst = src1 - src2              */
/* dst can be one of src1 or src2 */
void
FE_SubtractRegion(FE_Region src1, FE_Region src2, FE_Region dst)
{
  XP_ASSERT(src1);
  XP_ASSERT(src2);
  XP_ASSERT(dst);

  XSubtractRegion((Region)src1, (Region)src2, (Region)dst);
}

/* Returns TRUE if the region contains no pixels */
XP_Bool
FE_IsEmptyRegion(FE_Region region)
{
  XP_ASSERT(region);

  return (XP_Bool)XEmptyRegion((Region)region);
}

/* Returns the bounding rectangle of the region */
void
FE_GetRegionBoundingBox(FE_Region region, XP_Rect *bbox)
{
  XRectangle rect;

  XP_ASSERT(region);
  XP_ASSERT(bbox);

  XClipBox((Region)region, &rect);

  bbox->left = (int32)rect.x;
  bbox->top = (int32)rect.y;
  bbox->right = (int32)(rect.x + rect.width);
  bbox->bottom = (int32)(rect.y + rect.height);
}

/* TRUE if rgn1 == rgn2 */
XP_Bool
FE_IsEqualRegion(FE_Region rgn1, FE_Region rgn2)
{
  XP_ASSERT(rgn1);
  XP_ASSERT(rgn2);

  return (XP_Bool)XEqualRegion((Region)rgn1, (Region)rgn2);
}

/* Moves a region by the specified offsets */
void
FE_OffsetRegion(FE_Region region, int32 x_offset, int32 y_offset)
{
  XOffsetRegion((Region)region, x_offset, y_offset);
}

/* Returns TRUE if any part of the rectangle is in the specified region */
XP_Bool
FE_RectInRegion(FE_Region region, XP_Rect *rect)
{
  int result;
  XRectangle box;

  XP_ASSERT(region);
  XP_ASSERT(rect);

  box.x = (short)rect->left;
  box.y = (short)rect->top;
  box.width = (unsigned short)(rect->right - rect->left);
  box.height = (unsigned short)(rect->bottom - rect->top);

  result = XRectInRegion((Region)region, box.x, box.y, box.width, box.height);

  if (result == RectangleOut)
    return FALSE;
  else                /* result == RectangleIn || result == RectanglePart */
    return TRUE;
}
