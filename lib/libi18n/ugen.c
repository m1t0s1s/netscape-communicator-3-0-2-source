#include "ugen.h"
#include "xp.h"
/*=================================================================================

=================================================================================*/
typedef  XP_Bool (*uSubGeneratorFunc) (uint16 in, unsigned char* out);
/*=================================================================================

=================================================================================*/

typedef XP_Bool (*uGeneratorFunc) (
		uShiftTable 			*shift,
		int32*				state,
		uint16				in,
		unsigned char*		out,
		uint16 				outbuflen,
		uint16*				outlen
);

XP_Bool uGenerate(		
		uShiftTable 			*shift,
		int32*				state,
		uint16				in,
		unsigned char*		out,
		uint16 				outbuflen,
		uint16*				outlen
);

#define uSubGennerator(sub,in,out)	(* m_subgenerator[sub])((in),(out))

XP_Bool uCheckAndGenAlways1Byte(
		uShiftTable 			*shift,
		int32*				state,
		uint16				in,
		unsigned char*		out,
		uint16 				outbuflen,
		uint16*				outlen
);
XP_Bool uCheckAndGenAlways2Byte(
		uShiftTable 			*shift,
		int32*				state,
		uint16				in,
		unsigned char*		out,
		uint16 				outbuflen,
		uint16*				outlen
);
XP_Bool uCheckAndGenAlways2ByteShiftGR(
		uShiftTable 			*shift,
		int32*				state,
		uint16				in,
		unsigned char*		out,
		uint16 				outbuflen,
		uint16*				outlen
);
XP_Bool uCheckAndGenByTable(
		uShiftTable 			*shift,
		int32*				state,
		uint16				in,
		unsigned char*		out,
		uint16 				outbuflen,
		uint16*				outlen
);
XP_Bool uCheckAndGen2ByteGRPrefix8F(
		uShiftTable 			*shift,
		int32*				state,
		uint16				in,
		unsigned char*		out,
		uint16 				outbuflen,
		uint16*				outlen
);
XP_Bool uCheckAndGen2ByteGRPrefix8EA2(
		uShiftTable 			*shift,
		int32*				state,
		uint16				in,
		unsigned char*		out,
		uint16 				outbuflen,
		uint16*				outlen
);


XP_Bool uGenAlways2Byte(
		uint16 				in,
		unsigned char*		out
);
XP_Bool uGenAlways2ByteShiftGR(
		uint16 				in,
		unsigned char*		out
);
XP_Bool uGenAlways1Byte(
		uint16 				in,
		unsigned char*		out
);
XP_Bool uGenAlways1BytePrefix8E(
		uint16 				in,
		unsigned char*		out
);
/*=================================================================================

=================================================================================*/
static uGeneratorFunc m_generator[uNumOfCharsetType] =
{
	uCheckAndGenAlways1Byte,
	uCheckAndGenAlways2Byte,
	uCheckAndGenByTable,
	uCheckAndGenAlways2ByteShiftGR,
	uCheckAndGen2ByteGRPrefix8F,
	uCheckAndGen2ByteGRPrefix8EA2,
};

/*=================================================================================

=================================================================================*/

static uSubGeneratorFunc m_subgenerator[uNumOfCharType] =
{
	uGenAlways1Byte,
	uGenAlways2Byte,
	uGenAlways2ByteShiftGR,
	uGenAlways1BytePrefix8E
};
/*=================================================================================

=================================================================================*/
XP_Bool uGenerate(		
		uShiftTable 			*shift,
		int32*				state,
		uint16				in,
		unsigned char*		out,
		uint16 				outbuflen,
		uint16*				outlen
)
{
	return (* m_generator[shift->classID]) (shift,state,in,out,outbuflen,outlen);
}
/*=================================================================================

=================================================================================*/
XP_Bool uGenAlways1Byte(
		uint16 				in,
		unsigned char*		out
)
{
	out[0] = (unsigned char)in;
	return TRUE;
}

/*=================================================================================

=================================================================================*/
XP_Bool uGenAlways2Byte(
		uint16 				in,
		unsigned char*		out
)
{
	out[0] = (unsigned char)((in >> 8) & 0xff);
	out[1] = (unsigned char)(in & 0xff);
	return TRUE;
}
/*=================================================================================

=================================================================================*/
XP_Bool uGenAlways2ByteShiftGR(
		uint16 				in,
		unsigned char*		out
)
{
	out[0] = (unsigned char)(((in >> 8) & 0xff) | 0x80);
	out[1] = (unsigned char)((in & 0xff) | 0x80);
	return TRUE;
}
/*=================================================================================

=================================================================================*/
XP_Bool uGenAlways1BytePrefix8E(
		uint16 				in,
		unsigned char*		out
)
{
	out[0] = 0x8E;
	out[1] = (unsigned char)(in  & 0xff);
	return TRUE;
}


/*=================================================================================

=================================================================================*/
XP_Bool uCheckAndGenAlways1Byte(
		uShiftTable 			*shift,
		int32*				state,
		uint16				in,
		unsigned char*		out,
		uint16 				outbuflen,
		uint16*				outlen
)
{
	/*	Don't check inlen. The caller should ensure it is larger than 0 */
	*outlen = 1;
	out[0] = in & 0xff;
	return TRUE;
}

/*=================================================================================

=================================================================================*/
XP_Bool uCheckAndGenAlways2Byte(
		uShiftTable 			*shift,
		int32*				state,
		uint16				in,
		unsigned char*		out,
		uint16 				outbuflen,
		uint16*				outlen
)
{
	if(outbuflen < 2)
		return FALSE;
	else
	{
		*outlen = 2;
		out[0] = ((in >> 8 ) & 0xff);
		out[1] = in  & 0xff;
		return TRUE;
	}
}
/*=================================================================================

=================================================================================*/
XP_Bool uCheckAndGenAlways2ByteShiftGR(
		uShiftTable 			*shift,
		int32*				state,
		uint16				in,
		unsigned char*		out,
		uint16 				outbuflen,
		uint16*				outlen
)
{
	if(outbuflen < 2)
		return FALSE;
	else
	{
		*outlen = 2;
		out[0] = ((in >> 8 ) & 0xff) | 0x80;
		out[1] = (in  & 0xff)  | 0x80;
		return TRUE;
	}
}
/*=================================================================================

=================================================================================*/
XP_Bool uCheckAndGenByTable(
		uShiftTable 			*shift,
		int32*				state,
		uint16				in,
		unsigned char*		out,
		uint16 				outbuflen,
		uint16*				outlen
)
{
	int16 i;
	uShiftCell* cell = &(shift->shiftcell[0]);
	int16 itemnum = shift->numOfItem;
	unsigned char inH, inL;
	inH =	(in >> 8) & 0xff;
	inL = (in & 0xff );
	for(i=0;i<itemnum;i++)
	{
		if( ( inL >=  cell[i].shiftout.MinLB) &&
			( inL <=  cell[i].shiftout.MaxLB) &&
			( inH >=  cell[i].shiftout.MinHB) &&
			( inH <=  cell[i].shiftout.MaxHB)	)
		{
			if(outbuflen < cell[i].reserveLen)
				return FALSE;
			else
			{
				*outlen = cell[i].reserveLen;
				return (uSubGennerator(cell[i].classID,in,out));
			}
		}
	}
	return FALSE;
}
/*=================================================================================

=================================================================================*/
XP_Bool uCheckAndGen2ByteGRPrefix8F(
		uShiftTable 			*shift,
		int32*				state,
		uint16				in,
		unsigned char*		out,
		uint16 				outbuflen,
		uint16*				outlen
)
{
	if(outbuflen < 3)
		return FALSE;
	else
	{
		*outlen = 3;
		out[0] = 0x8F;
		out[1] = ((in >> 8 ) & 0xff) | 0x80;
		out[2] = (in  & 0xff)  | 0x80;
		return TRUE;
	}
}
/*=================================================================================

=================================================================================*/
XP_Bool uCheckAndGen2ByteGRPrefix8EA2(
		uShiftTable 			*shift,
		int32*				state,
		uint16				in,
		unsigned char*		out,
		uint16 				outbuflen,
		uint16*				outlen
)
{
	if(outbuflen < 4)
		return FALSE;
	else
	{
		*outlen = 4;
		out[0] = 0x8E;
		out[1] = 0xA2;
		out[2] = ((in >> 8 ) & 0xff) | 0x80;
		out[3] = (in  & 0xff)  | 0x80;
		return TRUE;
	}
}


