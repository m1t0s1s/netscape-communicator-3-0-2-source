#include "ugen.h"
#include "xp.h"
/*=================================================================================

=================================================================================*/
typedef  XP_Bool (*uSubScannerFunc) (unsigned char* in, uint16* out);
/*=================================================================================

=================================================================================*/

typedef XP_Bool (*uScannerFunc) (
		uShiftTable 			*shift,
		int32*				state,
		unsigned char		*in,
		uint16				*out,
		uint16 				inbuflen,
		uint16*				inscanlen
);

XP_Bool uScan(		
		uShiftTable 			*shift,
		int32*				state,
		unsigned char		*in,
		uint16				*out,
		uint16 				inbuflen,
		uint16*				inscanlen
);

#define uSubScanner(sub,in,out)	(* m_subscanner[sub])((in),(out))

XP_Bool uCheckAndScanAlways1Byte(
		uShiftTable 			*shift,
		int32*				state,
		unsigned char		*in,
		uint16				*out,
		uint16 				inbuflen,
		uint16*				inscanlen
);
XP_Bool uCheckAndScanAlways2Byte(
		uShiftTable 			*shift,
		int32*				state,
		unsigned char		*in,
		uint16				*out,
		uint16 				inbuflen,
		uint16*				inscanlen
);
XP_Bool uCheckAndScanAlways2ByteShiftGR(
		uShiftTable 			*shift,
		int32*				state,
		unsigned char		*in,
		uint16				*out,
		uint16 				inbuflen,
		uint16*				inscanlen
);
XP_Bool uCheckAndScanByTable(
		uShiftTable 			*shift,
		int32*				state,
		unsigned char		*in,
		uint16				*out,
		uint16 				inbuflen,
		uint16*				inscanlen
);
XP_Bool uCheckAndScan2ByteGRPrefix8F(
		uShiftTable 			*shift,
		int32*				state,
		unsigned char		*in,
		uint16				*out,
		uint16 				inbuflen,
		uint16*				inscanlen
);
XP_Bool uCheckAndScan2ByteGRPrefix8EA2(
		uShiftTable 			*shift,
		int32*				state,
		unsigned char		*in,
		uint16				*out,
		uint16 				inbuflen,
		uint16*				inscanlen
);


XP_Bool uScanAlways2Byte(
		unsigned char*		in,
		uint16*				out
);
XP_Bool uScanAlways2ByteShiftGR(
		unsigned char*		in,
		uint16*				out
);
XP_Bool uScanAlways1Byte(
		unsigned char*		in,
		uint16*				out
);
XP_Bool uScanAlways1BytePrefix8E(
		unsigned char*		in,
		uint16*				out
);
/*=================================================================================

=================================================================================*/
static uScannerFunc m_scanner[uNumOfCharsetType] =
{
	uCheckAndScanAlways1Byte,
	uCheckAndScanAlways2Byte,
	uCheckAndScanByTable,
	uCheckAndScanAlways2ByteShiftGR,
	uCheckAndScan2ByteGRPrefix8F,
	uCheckAndScan2ByteGRPrefix8EA2,
};

/*=================================================================================

=================================================================================*/

static uSubScannerFunc m_subscanner[uNumOfCharType] =
{
	uScanAlways1Byte,
	uScanAlways2Byte,
	uScanAlways2ByteShiftGR,
	uScanAlways1BytePrefix8E
};
/*=================================================================================

=================================================================================*/
XP_Bool uScan(		
		uShiftTable 			*shift,
		int32*				state,
		unsigned char		*in,
		uint16				*out,
		uint16 				inbuflen,
		uint16*				inscanlen
)
{
	return (* m_scanner[shift->classID]) (shift,state,in,out,inbuflen,inscanlen);
}
/*=================================================================================

=================================================================================*/
XP_Bool uScanAlways1Byte(
		unsigned char*		in,
		uint16*				out
)
{
    *out = (uint16) in[0];
	return TRUE;
}

/*=================================================================================

=================================================================================*/
XP_Bool uScanAlways2Byte(
		unsigned char*		in,
		uint16*				out
)
{
    *out = (uint16) (( in[0] << 8) | (in[1]));
	return TRUE;
}
/*=================================================================================

=================================================================================*/
XP_Bool uScanAlways2ByteShiftGR(
		unsigned char*		in,
		uint16*				out
)
{
    *out = (uint16) ((( in[0] << 8) | (in[1])) &  0x7F7F);
	return TRUE;
}
/*=================================================================================

=================================================================================*/
XP_Bool uScanAlways1BytePrefix8E(
		unsigned char*		in,
		uint16*				out
)
{
	if(in[0] != 0x8E)
		return FALSE;
	*out = (uint16) in[1];
	return TRUE;
}


/*=================================================================================

=================================================================================*/
XP_Bool uCheckAndScanAlways1Byte(
		uShiftTable 			*shift,
		int32*				state,
		unsigned char		*in,
		uint16				*out,
		uint16 				inbuflen,
		uint16*				inscanlen
)
{
	/*	Don't check inlen. The caller should ensure it is larger than 0 */
	*inscanlen = 1;
	*out = (uint16) in[0];

	return TRUE;
}

/*=================================================================================

=================================================================================*/
XP_Bool uCheckAndScanAlways2Byte(
		uShiftTable 			*shift,
		int32*				state,
		unsigned char		*in,
		uint16				*out,
		uint16 				inbuflen,
		uint16*				inscanlen
)
{
	if(inbuflen < 2)
		return FALSE;
	else
	{
		*inscanlen = 2;
		*out = ((in[0] << 8) | ( in[1])) ;
		return TRUE;
	}
}
/*=================================================================================

=================================================================================*/
XP_Bool uCheckAndScanAlways2ByteShiftGR(
		uShiftTable 			*shift,
		int32*				state,
		unsigned char		*in,
		uint16				*out,
		uint16 				inbuflen,
		uint16*				inscanlen
)
{
	if(inbuflen < 2)
		return FALSE;
	else
	{
		*inscanlen = 2;
		*out = (((in[0] << 8) | ( in[1]))  & 0x7F7F);
		return TRUE;
	}
}
/*=================================================================================

=================================================================================*/
XP_Bool uCheckAndScanByTable(
		uShiftTable 			*shift,
		int32*				state,
		unsigned char		*in,
		uint16				*out,
		uint16 				inbuflen,
		uint16*				inscanlen
)
{
	int16 i;
	uShiftCell* cell = &(shift->shiftcell[0]);
	int16 itemnum = shift->numOfItem;
	for(i=0;i<itemnum;i++)
	{
		if( ( in[0] >=  cell[i].shiftin.Min) &&
			( in[0] <=  cell[i].shiftin.Max))
		{
			if(inbuflen < cell[i].reserveLen)
				return FALSE;
			else
			{
				*inscanlen = cell[i].reserveLen;
				return (uSubScanner(cell[i].classID,in,out));
			}
		}
	}
	return FALSE;
}
/*=================================================================================

=================================================================================*/
XP_Bool uCheckAndScan2ByteGRPrefix8F(
		uShiftTable 			*shift,
		int32*				state,
		unsigned char		*in,
		uint16				*out,
		uint16 				inbuflen,
		uint16*				inscanlen
)
{
	if((inbuflen < 3) ||(in[0] != 0x8F))
		return FALSE;
	else
	{
		*inscanlen = 3;
		*out = (((in[1] << 8) | ( in[2]))  & 0x7F7F);
		return TRUE;
	}
}
/*=================================================================================

=================================================================================*/
XP_Bool uCheckAndScan2ByteGRPrefix8EA2(
		uShiftTable 			*shift,
		int32*				state,
		unsigned char		*in,
		uint16				*out,
		uint16 				inbuflen,
		uint16*				inscanlen
)
{
	if((inbuflen < 4) || (in[0] != 0x8E) || (in[1] != 0xA2))
		return FALSE;
	else
	{
		*inscanlen = 4;
		*out = (((in[2] << 8) | ( in[3]))  & 0x7F7F);
		return TRUE;
	}
}


