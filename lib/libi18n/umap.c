#include "unicpriv.h"

typedef uint16 (* MapFormatFunc)(uint16 in,uTable *uTable,uMapCell *cell);
typedef XP_Bool (* HitFormateFunc)(uint16 in,uMapCell *cell);
typedef void (* IterateFormatFunc)(uTable *uTable, uMapCell *cell,uMapIterateFunc callback, uint16 context);

static XP_Bool uHitFormate0(uint16 in,uMapCell *cell);
static XP_Bool uHitFormate1(uint16 in,uMapCell *cell);
static XP_Bool uHitFormate2(uint16 in,uMapCell *cell);
static uint16 uMapFormate0(uint16 in,uTable *uTable,uMapCell *cell);
static uint16 uMapFormate1(uint16 in,uTable *uTable,uMapCell *cell);
static uint16 uMapFormate2(uint16 in,uTable *uTable,uMapCell *cell);
static void uIterateFormate0(uTable *uTable, uMapCell *cell,uMapIterateFunc callback, uint16 context);
static void uIterateFormate1(uTable *uTable, uMapCell *cell,uMapIterateFunc callback, uint16 context);
static void uIterateFormate2(uTable *uTable, uMapCell *cell,uMapIterateFunc callback, uint16 context);
static uMapCell *uGetMapCell(uTable *uTable, int16 item);
static char uGetFormat(uTable *uTable, int16 item);


/*=================================================================================

=================================================================================*/
static MapFormatFunc m_map[uNumFormatTag] =
{
	uMapFormate0,
	uMapFormate1,
	uMapFormate2,
};
/*=================================================================================

=================================================================================*/
static IterateFormatFunc m_iterate[uNumFormatTag] =
{
	uIterateFormate0,
	uIterateFormate1,
	uIterateFormate2,
};
/*=================================================================================

=================================================================================*/
static HitFormateFunc m_hit[uNumFormatTag] =
{
	uHitFormate0,
	uHitFormate1,
	uHitFormate2,
};

/*
	Need more work
*/
/*=================================================================================

=================================================================================*/
static XP_Bool uHit(unsigned char format, uint16 in,uMapCell *cell)
{
	return 	(* m_hit[format])((in),(cell));
}
/*=================================================================================

=================================================================================*/
static void uCellIterate(unsigned char format, uTable *uTable, uMapCell *cell,uMapIterateFunc callback, uint16 context)
{
	(* m_iterate[format])((uTable),(cell),(callback),(context));
}
/*	
	Switch to Macro later for performance
	
#define	uHit(format,in,cell) 		(* m_hit[format])((in),(cell))
*/
/*=================================================================================

=================================================================================*/
static uint16 uMap(unsigned char format, uint16 in,uTable *uTable,uMapCell *cell)
{
	return 	(* m_map[format])((in),(uTable),(cell));
}
/* 	
	Switch to Macro later for performance
	
#define uMap(format,in,cell) 		(* m_map[format])((in),(cell))
*/

/*=================================================================================

=================================================================================*/
/*	
	Switch to Macro later for performance
*/
static uMapCell *uGetMapCell(uTable *uTable, int16 item)
{
	return ((uMapCell *)(((uint16 *)uTable) + uTable->offsetToMapCellArray) + item) ;
}
/*=================================================================================

=================================================================================*/
/*	
	Switch to Macro later for performance
*/
static char uGetFormat(uTable *uTable, int16 item)
{
	return (((((uint16 *)uTable) + uTable->offsetToFormatArray)[ item >> 2 ]
		>> (( item % 4 ) << 2)) & 0x0f);
}
/*=================================================================================

=================================================================================*/
XP_Bool uMapCode(uTable *uTable, uint16 in, uint16* out)
{
	XP_Bool done = FALSE;
	uint16 itemOfList = uTable->itemOfList;
	uint16 i;
	for(i=0;i<itemOfList;i++)
	{
		uMapCell* uCell;
		char format = uGetFormat(uTable,i);
		uCell = uGetMapCell(uTable,i);
		if(uHit(format, in, uCell))
		{
			*out = uMap(format, in, uTable,uCell);
			done = TRUE;
			break;
		}
	}
	return ( done && (*out != NOMAPPING));
}
/*=================================================================================

=================================================================================*/
void		uMapIterate(uTable *uTable, uMapIterateFunc callback, uint16 context)
{
	uint16 itemOfList = uTable->itemOfList;
	uint16 i;
	for(i=0;i<itemOfList;i++)
	{
		uMapCell* uCell;
		char format = uGetFormat(uTable,i);
		uCell = uGetMapCell(uTable,i);
		uCellIterate(format, uTable ,uCell,callback, context);
	}
}

/*
	member function
*/
/*=================================================================================

=================================================================================*/
static XP_Bool uHitFormate0(uint16 in,uMapCell *cell)
{
	return ( (in >= cell->fmt.format0.srcBegin) &&
			     (in <= cell->fmt.format0.srcEnd) ) ;
}
/*=================================================================================

=================================================================================*/
static XP_Bool uHitFormate1(uint16 in,uMapCell *cell)
{
	return  uHitFormate0(in,cell);
}
/*=================================================================================

=================================================================================*/
static XP_Bool uHitFormate2(uint16 in,uMapCell *cell)
{
	return (in == cell->fmt.format2.srcBegin);
}
/*=================================================================================

=================================================================================*/
static uint16 uMapFormate0(uint16 in,uTable *uTable,uMapCell *cell)
{
	return ((in - cell->fmt.format0.srcBegin) + cell->fmt.format0.destBegin);
}
/*=================================================================================

=================================================================================*/
static uint16 uMapFormate1(uint16 in,uTable *uTable,uMapCell *cell)
{
	return (*(((uint16 *)uTable) + uTable->offsetToMappingTable
		+ cell->fmt.format1.mappingOffset + in - cell->fmt.format1.srcBegin));
}
/*=================================================================================

=================================================================================*/
static uint16 uMapFormate2(uint16 in,uTable *uTable,uMapCell *cell)
{
	return (cell->fmt.format2.destBegin);
}

/*=================================================================================

=================================================================================*/
static void uIterateFormate0(uTable *uTable, uMapCell *cell,uMapIterateFunc callback, uint16 context)
{
	uint16 ucs2;
	uint16 med;
	for(ucs2 = cell->fmt.format0.srcBegin, med = cell->fmt.format0.destBegin;
				ucs2 <= cell->fmt.format0.srcEnd ; ucs2++,med++)
		(*callback)(ucs2, med, context);
}
/*=================================================================================

=================================================================================*/
static void uIterateFormate1(uTable *uTable, uMapCell *cell,uMapIterateFunc callback, uint16 context)
{
	uint16 ucs2;
	uint16 *medpt;
	medpt = (((uint16 *)uTable) + uTable->offsetToMappingTable	+ cell->fmt.format1.mappingOffset);
	for(ucs2 = cell->fmt.format1.srcBegin;	ucs2 <= cell->fmt.format1.srcEnd ; ucs2++, medpt++)
		(*callback)(ucs2, *medpt, context);
}
/*=================================================================================

=================================================================================*/
static void uIterateFormate2(uTable *uTable, uMapCell *cell,uMapIterateFunc callback, uint16 context)
{
	(*callback)(cell->fmt.format2.srcBegin, cell->fmt.format2.destBegin, context);
}
