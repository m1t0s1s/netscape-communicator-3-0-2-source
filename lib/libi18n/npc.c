#include "unicpriv.h"
#include "intlpriv.h"
#include "npc.h"


extern int MK_OUT_OF_MEMORY;


#ifdef XP_MAC
#define UTF8_1_CSID 	CS_MAC_ROMAN
#else
#define UTF8_1_CSID  	CS_LATIN1
#endif	


/*==========================================================================
		Table to Map NPC First Byte to Char Set ID
==========================================================================*/
int16	npctocsid[MAXCSIDINNPC] =
{
	CS_DEFAULT,	CS_ASCII,	CS_LATIN1,	CS_JIS,	CS_SJIS,	CS_EUCJP,	CS_MAC_ROMAN,	CS_BIG5,	
	CS_GB_8BIT,	CS_CNS_8BIT,	CS_LATIN2,	CS_MAC_CE,	CS_KSC_8BIT,	CS_2022_KR, 	CS_8859_3,	CS_8859_4,	
	CS_8859_5,	CS_8859_6,	CS_8859_7,	CS_8859_8,	CS_8859_9,	CS_SYMBOL,	CS_DINGBATS,	CS_DECTECH,
	CS_CNS11643_1,	CS_CNS11643_2,	CS_JISX0208,	CS_JISX0201,	CS_KSC5601,	CS_TIS620,	CS_JISX0212,	CS_GB2312,
	CS_UNKNOWN,	CS_UNKNOWN,	CS_UNKNOWN,	CS_UNKNOWN,	CS_UNKNOWN,	CS_X_BIG5,	CS_UNKNOWN,	CS_KOI8_R,
	CS_MAC_CYRILLIC,	CS_CP_1251,	CS_MAC_GREEK,	CS_CP_1253,	CS_CP_1250,	CS_UNKNOWN,	CS_UNKNOWN,	CS_UNKNOWN,
	CS_UNKNOWN,	CS_UNKNOWN,	CS_UNKNOWN,	CS_UNKNOWN,	CS_UNKNOWN,	CS_UNKNOWN,	CS_UNKNOWN,	CS_UNKNOWN,
	CS_UNKNOWN,	CS_UNKNOWN,	CS_UNKNOWN,	CS_UNKNOWN,	CS_UNKNOWN,	CS_UNKNOWN,	CS_USER_DEFINED_ENCODING,	CS_UNKNOWN,
};
/*==========================================================================
		MACRO for npctocsid map
==========================================================================*/
/*==========================================================================
		MACRO for npcmap


	Let's make a 256 array which contain 		
	(1) the index to the npcXXXX function		4 bits	
	(2) npcXXXX function context				4 bits
			npcError -		not used
			npcOneByte - 	not used 
			npcTwoByte - 	not used 
			npcMultiByte - 	csinfo_tbl index
			npcUTF8_1 - 	not used
			npcUTF8_2 - 	not used
			npcUTF8_3 - 	not used
			npcUTF8_4 - 	not used
			npcUTF8_5 - 	not used
			npcUTF8_6 - 	not used			
==========================================================================*/
#define NPCIndex(fb)		((npcmap[fb]) & 0x0F)
#define NPCContext(fb)		((npcmap[fb] >> 4) & 0x0F)
#define NCELL(idx,ctx)		((ctx << 4) | (idx))
/*--------------------------------------------------------------------------
		enum for the method index
--------------------------------------------------------------------------*/
enum {
	npcErrI = 0,
	npcSBI,
	npcTBI,
	npcMBI,
	npcUTF8_1I,
	npcUTF8_2I,
	npcUTF8_3I,
	npcUTF8_4I,
	npcUTF8_5I,
	npcUTF8_6I,
	npcNum
};

/*--------------------------------------------------------------------------
		enum for the multibyte csinfo index
--------------------------------------------------------------------------*/
/*	This have to correspond to the sequence of csinfo_tbl in fe_ccc.c */
enum {
	csinfoSJISIdx = 0,
	/*	csinfoEUCJPIdx = 1,	This should never be used */
	csinfoKSC_8BITIdx = 2,
	csinfoGB_8BITIdx = 3,
	/* csinfoCNS_8BITIdx = 4, This should never be used */
	csinfoBIG5Idx = 5,
	csinfoCNS11643_1Idx = 6,
	csinfoCNS11643_2Idx = 7,
	csinfoJISX0208Idx = 8,
	csinfoKSC5601Idx = 9,
	csinfoJISX0212Idx = 10,
	csinfoGB2312Idx = 11
};

/*--------------------------------------------------------------------------
		enum for the method index and csinfo index
--------------------------------------------------------------------------*/
enum {
	npcSJIS = 		NCELL(npcMBI,csinfoSJISIdx),
	npcKSC_8BIT = 	NCELL(npcMBI,csinfoKSC_8BITIdx),
	npcGB_8BIT = 	NCELL(npcMBI,csinfoGB_8BITIdx),
	npcBIG5 = 		NCELL(npcMBI,csinfoBIG5Idx),
	npcCNS11643_1 = NCELL(npcMBI,csinfoCNS11643_1Idx),
	npcCNS11643_2 = NCELL(npcMBI,csinfoCNS11643_2Idx),
	npcJISX0208 = 	NCELL(npcMBI,csinfoJISX0208Idx),
	npcKSC5601 = 	NCELL(npcMBI,csinfoKSC5601Idx),
	npcJISX0212 = 	NCELL(npcMBI,csinfoJISX0212Idx),
	npcGB2312 = 	NCELL(npcMBI,csinfoGB2312Idx)
};
/*--------------------------------------------------------------------------
		npcmap : Map first byte of npc to [npc method index + method context ]
--------------------------------------------------------------------------*/
unsigned char npcmap[256] = {
/*	-------------------	Range for UTF8 One bytes	------------------------ */						
/* 00-07 */	npcUTF8_1I,	npcUTF8_1I,	npcUTF8_1I,	npcUTF8_1I,	npcUTF8_1I,	npcUTF8_1I,	npcUTF8_1I,	npcUTF8_1I,
/* 08-0F */	npcUTF8_1I,	npcUTF8_1I,	npcUTF8_1I,	npcUTF8_1I,	npcUTF8_1I,	npcUTF8_1I,	npcUTF8_1I,	npcUTF8_1I,
/* 10-17 */	npcUTF8_1I,	npcUTF8_1I,	npcUTF8_1I,	npcUTF8_1I,	npcUTF8_1I,	npcUTF8_1I,	npcUTF8_1I,	npcUTF8_1I,
/* 18-1F */	npcUTF8_1I,	npcUTF8_1I,	npcUTF8_1I,	npcUTF8_1I,	npcUTF8_1I,	npcUTF8_1I,	npcUTF8_1I,	npcUTF8_1I,
/* 20-27 */	npcUTF8_1I,	npcUTF8_1I,	npcUTF8_1I,	npcUTF8_1I,	npcUTF8_1I,	npcUTF8_1I,	npcUTF8_1I,	npcUTF8_1I,
/* 28-2F */	npcUTF8_1I,	npcUTF8_1I,	npcUTF8_1I,	npcUTF8_1I,	npcUTF8_1I,	npcUTF8_1I,	npcUTF8_1I,	npcUTF8_1I,
/* 30-37 */	npcUTF8_1I,	npcUTF8_1I,	npcUTF8_1I,	npcUTF8_1I,	npcUTF8_1I,	npcUTF8_1I,	npcUTF8_1I,	npcUTF8_1I,
/* 38-3F */	npcUTF8_1I,	npcUTF8_1I,	npcUTF8_1I,	npcUTF8_1I,	npcUTF8_1I,	npcUTF8_1I,	npcUTF8_1I,	npcUTF8_1I,
/* 40-47 */	npcUTF8_1I,	npcUTF8_1I,	npcUTF8_1I,	npcUTF8_1I,	npcUTF8_1I,	npcUTF8_1I,	npcUTF8_1I,	npcUTF8_1I,
/* 48-4F */	npcUTF8_1I,	npcUTF8_1I,	npcUTF8_1I,	npcUTF8_1I,	npcUTF8_1I,	npcUTF8_1I,	npcUTF8_1I,	npcUTF8_1I,
/* 50-57 */	npcUTF8_1I,	npcUTF8_1I,	npcUTF8_1I,	npcUTF8_1I,	npcUTF8_1I,	npcUTF8_1I,	npcUTF8_1I,	npcUTF8_1I,
/* 58-5F */	npcUTF8_1I,	npcUTF8_1I,	npcUTF8_1I,	npcUTF8_1I,	npcUTF8_1I,	npcUTF8_1I,	npcUTF8_1I,	npcUTF8_1I,
/* 60-67 */	npcUTF8_1I,	npcUTF8_1I,	npcUTF8_1I,	npcUTF8_1I,	npcUTF8_1I,	npcUTF8_1I,	npcUTF8_1I,	npcUTF8_1I,
/* 68-6F */	npcUTF8_1I,	npcUTF8_1I,	npcUTF8_1I,	npcUTF8_1I,	npcUTF8_1I,	npcUTF8_1I,	npcUTF8_1I,	npcUTF8_1I,
/* 70-77 */	npcUTF8_1I,	npcUTF8_1I,	npcUTF8_1I,	npcUTF8_1I,	npcUTF8_1I,	npcUTF8_1I,	npcUTF8_1I,	npcUTF8_1I,
/* 78-7F */	npcUTF8_1I,	npcUTF8_1I,	npcUTF8_1I,	npcUTF8_1I,	npcUTF8_1I,	npcUTF8_1I,	npcUTF8_1I,	npcUTF8_1I,
/*	-------------------	Range for non-Unicode encoding	-------------------- */						
/* 80-87 */	npcSBI,		npcSBI,		npcSBI,		npcErrI,	npcSJIS,	npcErrI,	npcSBI,		npcBIG5,
/* 88-8F */	npcGB_8BIT,	npcErrI,	npcSBI,		npcSBI,		npcKSC_8BIT,npcErrI,	npcSBI,		npcSBI,
/* 90-97 */	npcSBI,		npcSBI,		npcSBI,		npcSBI,		npcSBI,		npcSBI,		npcSBI,		npcSBI,
/* 98-9F */	npcCNS11643_1,	npcCNS11643_2,	npcJISX0208,	npcSBI,	npcKSC5601,	npcSBI,	npcJISX0212,	npcGB2312,
/* A0-A7 */	npcErrI,	npcErrI,	npcErrI,	npcErrI,	npcErrI,	npcBIG5,	npcErrI,	npcSBI,
/* A8-AF */	npcSBI,		npcSBI,		npcSBI,		npcSBI,	npcErrI,	npcErrI,	npcErrI,	npcErrI,
/* B0-B7 */	npcErrI,	npcErrI,	npcErrI,	npcErrI,	npcErrI,	npcErrI,	npcErrI,	npcErrI,
/* B8-BF */	npcErrI,	npcErrI,	npcErrI,	npcErrI,	npcErrI,	npcErrI,	npcSBI,		npcErrI,
/*	-------------------	Range for UTF8 Two bytes	------------------------ */						
/* C0-C7 */	npcUTF8_2I,	npcUTF8_2I,	npcUTF8_2I,	npcUTF8_2I,	npcUTF8_2I,	npcUTF8_2I,	npcUTF8_2I,	npcUTF8_2I,
/* C8-CF */	npcUTF8_2I,	npcUTF8_2I,	npcUTF8_2I,	npcUTF8_2I,	npcUTF8_2I,	npcUTF8_2I,	npcUTF8_2I,	npcUTF8_2I,
/* D0-D7 */	npcUTF8_2I,	npcUTF8_2I,	npcUTF8_2I,	npcUTF8_2I,	npcUTF8_2I,	npcUTF8_2I,	npcUTF8_2I,	npcUTF8_2I,
/* D8-DF */	npcUTF8_2I,	npcUTF8_2I,	npcUTF8_2I,	npcUTF8_2I,	npcUTF8_2I,	npcUTF8_2I,	npcUTF8_2I,	npcUTF8_2I,
/*	-------------------	Range for UTF8 Three bytes	------------------------ */						
/* E0-E7 */	npcUTF8_3I,	npcUTF8_3I,	npcUTF8_3I,	npcUTF8_3I,	npcUTF8_3I,	npcUTF8_3I,	npcUTF8_3I,	npcUTF8_3I,
/* E8-EF */	npcUTF8_3I,	npcUTF8_3I,	npcUTF8_3I,	npcUTF8_3I,	npcUTF8_3I,	npcUTF8_3I,	npcUTF8_3I,	npcUTF8_3I,
/*	-------------------	Range for UTF8 Four bytes	------------------------ */						
/* F0-F7 */	npcUTF8_4I,	npcUTF8_4I,	npcUTF8_4I,	npcUTF8_4I,	npcUTF8_4I,	npcUTF8_4I,	npcUTF8_4I,	npcUTF8_4I,
/*	-------------------	Range for UTF8 Five and Six bytes	---------------- */						
/* F8-FF */	npcUTF8_5I,	npcUTF8_5I,	npcUTF8_5I,	npcUTF8_5I,	npcUTF8_6I,	npcUTF8_6I,	npcErrI,	npcErrI
};
/*==========================================================================
		MACRO for Multibyte length
==========================================================================*/
extern csinfo_t csinfo_tbl[];
#define MBCheck(i,j,fb)			( (csinfo_tbl[i].enc[j].range[0] <= (fb) ) && (csinfo_tbl[i].enc[j].range[1] >= (fb) ))
#define MBRange(i,j,fb,e)		( MBCheck(i,j,fb) ? csinfo_tbl[i].enc[j].bytes :(e))
#define MBLen(i,fb)				( MBRange(i,0,fb, MBRange(i,1,fb,MBRange(i,2,fb,1))))
/*int16 MBLen(int16 i, unsigned char fb)
{
	int j;
	for(j=0;j<3;j++)
	{
		XP_Bool cas1,cas2;
		int16 l,h;
		csinfo_t *pt;
		pt = &csinfo_tbl[i];
		l = pt->enc[j].range[0];
		h = pt->enc[j].range[1];
		cas1 =( l <= fb);
		cas2 =( fb <= h);
		if(cas1 && cas2)
			return csinfo_tbl[i].enc[j].bytes;
	}
	return 1;
}
*/
/*==========================================================================
		ToNPC
		This function will only be called when the csid is not UTF8 or UCS2
*/		
/*
	We need special case for EUC
	It have to convert 
		CS_EUC_JP			to CS_JISX0208, CS_JISX0201, CS_JISX0212	if it is XP_UNIX
		CS_CNS_8BIT			to CS_LATIN1, CS_CNS11643_P1, CS_CNS11643_P2
		CS_GB_8BIT			to CS_LATIN1, CS_GB2312
		CS_KSC_8BIT			to CS_LATIN1, CS_KSC5601
		
*/
#define CSIDTONPCFB(csid)		(((csid) & 0x3F) | 0x80)
#define YEN_SIGN 0x5c

XP_Bool ToNPC(unsigned char *in, uint16 inlen, uint16* inread, unsigned char *out, uint16 outbuflen, uint16* outlen, uint16 incsid)
{
	unsigned char npcfb = CSIDTONPCFB(incsid);
	uint16 il,ol;
	il = 0;
	ol = 0;
	switch(incsid)
	{	
		case CS_UCS2:
			{
				/* we may have problem with BYTE ORDER here. Since we cannot store state information for the converter. 
				   We need to change the interface later to make the converter maintain state.	
				   		State = BYTE_ORDER_UNKNOWN
				   				BYTE_ORDER_NO_SWAP
				   				BYTE_ORDER_NEED_SWAP
				   Currently, we assume it is always BYTE_ORDER_NO_SWAP
				*/
				uint16 ucs2; 
				for(;il+1 < inlen; il+=2)
				{
					ucs2 = ( in[il] << 8 ) | in[il+1];
					
					if(ucs2 < 0x80)				/*	case of one byte UTF8 */
					{
						if(ol< outbuflen)
							out[ol++] = ucs2 & 0x7F;
						else
							break;
					}
					else if(ucs2 < 0x800)		/*	case of two byte UTF8 */
					{
						if((ol+1)< outbuflen)
						{
							out[ol++] = 0xC0 | (( ucs2 >> 6 ) & 0x3F);
							out[ol++] = 0x80 | ( ucs2 & 0x3F);
						}
						else
							break;
					} else	if((ucs2 < 0xD800) || (0xE000 < ucs2 ))
					{				/*	case of three byte UTF8 */
						if((ol+2)< outbuflen)
						{
							out[ol++] = 0xE0 | (( ucs2 >> 12 ) & 0x0F);
							out[ol++] = 0x80 | (( ucs2 >> 6 )  & 0x3F);
							out[ol++] = 0x80 | ( ucs2 & 0x3F);
						}
						else
							break;
					}	
					else
					{			/*	If we just got a character of UTF-16 Low Half Zone, ignore it */
						if(ucs2 >= 0xDC00)	
						{
							/*	Error, eat two byte */
							/*	Do NOTHING HERE */
						}
						else 	/*	Handle UTF-16 High Half Zone */
						{	
							uint16 y ;
							if(il+3 < inlen)
							{
								y = ( in[il+2] << 8 ) | in[il+3];
								if(( 0xDC00 <= y) && (y < 0xE000))
								{
									uint32	t;
									t = ( ( ucs2 & 0x3FF ) << 10 ) | (y & 0x3FF ) | 0x10000;
									if((ol+2)< outbuflen)
									{
										out[ol++] = 0xE0 | (( t >> 12 ) & 0x0F);
										out[ol++] = 0x80 | (( t >> 6 )  & 0x3F);
										out[ol++] = 0x80 | ( t & 0x3F);
									}
								} else
								{
									/*	Error, eat two byte */
									/*	Do NOTHING HERE */
								}
								il+=2;
							}
							else
								break;
						}	
					}
				}
			}
			break;
		case CS_UCS4:
			{
				/* we may have problem with BYTE ORDER here. Since we cannot store state information for the converter. 
				   We need to change the interface later to make the converter maintain state.	
				   		State = BYTE_ORDER_UNKNOWN
				   				BYTE_ORDER_NO_SWAP
				   				BYTE_ORDER_NEED_SWAP
				   Currently, we assume it is always BYTE_ORDER_NO_SWAP
				*/
				uint32 ucs4; 
				for(;il+3 < inlen; il+=4)
				{
					ucs4 = ( in[il] << 24 ) | ( in[il+1] << 16 ) | ( in[il+2] << 8 ) | in[il+3];
					if(ucs4 < 0x80)				/*	case of one bytes UTF8 */
					{
						if(ol< outbuflen)
							out[ol++] = ucs4 & 0x7F;
						else
							break;
					}
					else if(ucs4 < 0x800)		/*	case of two bytes UTF8 */
					{
						if((ol+1)< outbuflen)
						{
							out[ol++] = 0xC0 | (( ucs4 >> 6 ) & 0x3F);
							out[ol++] = 0x80 | ( ucs4 & 0x3F);
						}
						else
							break;
					}
					else if(ucs4 < 0x10000)	/*	case of three bytes UTF8 */
					{
						if((ucs4 < 0xD800) || (0xE000 < ucs4 ))
						{
							if((ol+2)< outbuflen)
							{
								out[ol++] = 0xE0 | (( ucs4 >> 12 ) & 0x0F);
								out[ol++] = 0x80 | (( ucs4 >> 6 )  & 0x3F);
								out[ol++] = 0x80 | ( ucs4 & 0x3F);
							}
							else
								break;
						}
						else
						{			/*	If we just got a character of UTF-16 Low Half Zone, ignore it */
							if(ucs4 >= 0xDC00)	
							{
								/*	Error, eat two byte */
								/*	Do NOTHING HERE */
							}
							else 	/*	Handle UTF-16 High Half Zone */
							{	
								uint32 y ;
								if(il+7 < inlen)
								{
									y = ( in[il+4] << 24 ) | ( in[il+5] << 16 ) | ( in[il+6] << 8 ) | in[il+7];
									if(( 0xDC00 <= y) && (y < 0xE000))
									{
										uint32	t;
										t = ( ( ucs4 & 0x3FF ) << 10 ) | (y & 0x3FF ) | 0x10000;
										if((ol+2)< outbuflen)
										{
											out[ol++] = 0xE0 | (( t >> 12 ) & 0x0F);
											out[ol++] = 0x80 | (( t >> 6 )  & 0x3F);
											out[ol++] = 0x80 | ( t & 0x3F);
										}
									} else
									{
										/*	Error, eat two byte */
										/*	Do NOTHING HERE */
									}
									il+=4;
								}
								else
									break;
							}	
						}
					}
					else if(ucs4 < 0x200000)	/*	case of four bytes UTF8 */
					{
						if((ol+3)< outbuflen)
						{
							out[ol++] = 0xF0 | (( ucs4 >> 18 ) & 0x07);
							out[ol++] = 0x80 | (( ucs4 >> 12 ) & 0x3F);
							out[ol++] = 0x80 | (( ucs4 >> 6 )  & 0x3F);
							out[ol++] = 0x80 | ( ucs4 & 0x3F);
						}
						else
							break;
					}
					else if(ucs4 < 0x400000)	/*	case of five bytes UTF8 */
					{
						if((ol+4)< outbuflen)
						{
							out[ol++] = 0xF8 | (( ucs4 >> 24 ) & 0x03);
							out[ol++] = 0x80 | (( ucs4 >> 18 ) & 0x3F);
							out[ol++] = 0x80 | (( ucs4 >> 12 ) & 0x3F);
							out[ol++] = 0x80 | (( ucs4 >> 6 )  & 0x3F);
							out[ol++] = 0x80 | ( ucs4 & 0x3F);
						}
						else
							break;
					}
					else						/*	case of six bytes UTF8 */
					{
						if((ol+4)< outbuflen)
						{
							out[ol++] = 0xFC | (( ucs4 >> 30 ) & 0x01);
							out[ol++] = 0x80 | (( ucs4 >> 24 ) & 0x3F);
							out[ol++] = 0x80 | (( ucs4 >> 18 ) & 0x3F);
							out[ol++] = 0x80 | (( ucs4 >> 12 ) & 0x3F);
							out[ol++] = 0x80 | (( ucs4 >> 6 )  & 0x3F);
							out[ol++] = 0x80 | ( ucs4 & 0x3F);
						}
						else
							break;
					}
				}
			}
			break;
		case CS_UTF7:
				/*	Place holder for UTF7		*/
				/*	We don't support UTF7 yet 	*/
			XP_ASSERT(FALSE);
			break;
		case CS_UTF8:
			{	/* for UTF8, we just copy whatever in the in to out */
				uint len = (inlen > outbuflen) ? outbuflen : inlen;	/* Get the largest one */
				for(;il<len;)
					out[ol++] = in[il++];
			}
			break;
		
		/*	Multibyte */
		case CS_EUCJP:
			{
				unsigned char npcfb0201 = CSIDTONPCFB(CS_JISX0201);
				unsigned char npcfb0208 = CSIDTONPCFB(CS_JISX0208);
				unsigned char npcfb0212 = CSIDTONPCFB(CS_JISX0212);
				for(il=0,ol=0;il<inlen;)
				{
					if((in[il] < 0x80) && (in[il] != YEN_SIGN))	
					{
						if(ol < outbuflen)
							out[ol++] = in[il++];
						else	/*	not enough buffer, let's return */
							break;
					}
					else if (in[il] == YEN_SIGN) {
						if(((ol+1) < outbuflen))
						{
							out[ol++] = npcfb0201;
							out[ol++] = in[il++];
						}
						else {	/* full out buffer, return */
							break;
						}
					}
					else if (in[il] == SS2) {	/* half-width JIS 0201 kana */
						if(((il+1) < inlen) && ((ol+1) < outbuflen))
						{
							out[ol++] = npcfb0201;
							il++;				/* discard SS2 */
							out[ol++] = in[il++];
						}
						else {	/* full outbuf || incomplete inbuf, return */
							break;
						}
					}
					else if (in[il] == SS3) {	/* JIS 0212 */
						if(((il+2) < inlen) &&((ol+2) < outbuflen))
						{
							out[ol++] = npcfb0212;
							il++;				/* discard SS3 */
							out[ol++] = in[il++];
							out[ol++] = in[il++];
						}
						else {	/* full outbuf || incomplete inbuf, return */
							break;
						}
					}
					else {						/* JIS 0208 */
						if(((il+1) < inlen) && ((ol+2) < outbuflen))
						{
							out[ol++] = npcfb0208;
							out[ol++] = in[il++];
							out[ol++] = in[il++];
						}
						else {	/* full outbuf || incomplete inbuf, return */
							break;
						}
					}
				}
			}		
			break;
		
		case CS_CNS_8BIT:
			{
				unsigned char npcfb11643_1 = CSIDTONPCFB(CS_CNS11643_1);
				unsigned char npcfb11643_2 = CSIDTONPCFB(CS_CNS11643_2);
				for(il=0,ol=0;il<inlen;)
				{
					if(in[il] < 0x80)	
					{
						if(ol < outbuflen)
							out[ol++] = in[il++];
						else	/*	not enough buffer, let's return */
							break;
					}
					else if (in[il] == SS2) {	/* CNS11643_2 */
						if(((il+3) < inlen) &&((ol+2) < outbuflen))
						{
							switch(in[il+1])	/* Test Page Indicator */
							{
								case 0xA2:
									out[ol++] = npcfb11643_2;
									break;
								case 0xA1:	/* Handle the special case */
									out[ol++] = npcfb11643_1;
									break;
								/*  what we really should do is add npcfb11643_3 - npcfb11643_14
									here. For now, we simply convert everything in 
									Page 3 to Page 16 to Page 1 
								*/
								default:
									out[ol++] = npcfb11643_1;
									break;
							}
							il++;				/* discard SS3 */
							il++;				/* discard Page Indicator*/
							out[ol++] = in[il++];
							out[ol++] = in[il++];
						}
						else {	/* full outbuf || incomplete inbuf, return */
							break;
						}
					}
					else {						/*  CNS11643_1 */
						if(((il+1) < inlen) && ((ol+2) < outbuflen))
						{
							out[ol++] = npcfb11643_1;
							out[ol++] = in[il++];
							out[ol++] = in[il++];
						}
						else {	/* full outbuf || incomplete inbuf, return */
							break;
						}
					}
				}
			}		
			break;
		
		case CS_BIG5:
		case CS_GB_8BIT:
		case CS_KSC_8BIT:
			{
				uint16 csinfo_index = NPCContext(npcfb);
				for(il=0,ol=0;il<inlen;)
				{
					if(in[il] < 0x80)	
					{
						if(ol < outbuflen)
							out[ol++] = in[il++];
						else	/*	no enough buffer, let's return */
							break;
					}
					else 
					{
						uint16	L = MBLen(csinfo_index,in[il]);
						if(((ol+L) < outbuflen) && (il+L-1 < inlen))
						{
							int j;
							out[ol++] = npcfb;
							for(j=0;j<L;j++)							
								out[ol++] = in[il++];
						}
						else	/*	not enough buffer, let's return */
							break;
					}
				}
			}		
			break;

		case CS_SJIS:	/* Same as CS_BIG5, CS_GB_8BIT, and CS_GB_8BIT case except YEN SIGN handling */
			{
				uint16 csinfo_index = NPCContext(npcfb);
				for(il=0,ol=0;il<inlen;)
				{
					if((in[il] < 0x80) && (in[il] != YEN_SIGN))	
					{
						if(ol < outbuflen)
							out[ol++] = in[il++];
						else	/*	not enough buffer, let's return */
							break;
					}
					else 
					{
						uint16	L = MBLen(csinfo_index,in[il]);
						if(((ol+L) < outbuflen) && (il+L-1 < inlen))
						{
							int j;
							out[ol++] = npcfb;
							for(j=0;j<L;j++)					 		
								out[ol++] = in[il++];
						}
						else	/*	no enough buffer, let's return */
							break;
					}
				}
			}		
			break;
		default:	/*	Default is one byte */
			{
				for(il=0,ol=0; il < inlen ;)
				{
					if(in[il] < 0x80)
					{
						if(ol < outbuflen)
							out[ol++] = in[il++];
						else	/*	no enough buffer, let's return */
							break;
					}
					else
					{
						if(((ol+1) < outbuflen))
						{
							out[ol++] = npcfb;
							out[ol++] = in[il++];
						}
						else	/*	not enough buffer, let's return */
							break;
					}
				}
			}		
			break;
	}
	*inread = il;
	*outlen = ol;	
	return TRUE;
}

/*==========================================================================
		NPC Private function
==========================================================================*/


/*==========================================================================
		MACRO for UTF8 to UCS2 (or UCS4) conversion 

	UTF-8					UCS-4
	z < C0;					z
	z < E0; y				(z-C0) *2^6 + (y-80);
	z < F0; y; x;			(z-E0) *2^12 + (y-80) *2^6  + (x-80);
	
	z < F8; y; x; w;		(z-F0) *2^18 + (y-80) *2^12 + (x-80) *2^6  + (w-80);
	z < FC; y; x; w; v;		(z-F8) *2^24 + (y-80) *2^18 + (x-80) *2^12 + (w-80) *2^6 + (v-80);
	z; y; x; w; v; u;		(z-FC) *2^30 + (y-80) *2^24 + (x-80) *2^18 + (w-80) *2^12+ (v-80) *2^6 + (u-80);
==========================================================================*/
#define MXS(c,x,s)			(((c) - (x)) <<  (s))
#define M80S(c,s)			MXS(c,0x80,s)	
#define UTF8_1_to_UCS2(in)	((uint16) (in)[0])
#define UTF8_2_to_UCS2(in)	((uint16) (MXS((in)[0],0xC0, 6) | M80S((in)[1], 0)))
#define UTF8_3_to_UCS2(in)	((uint16) (MXS((in)[0],0xE0,12) | M80S((in)[1], 6) | M80S((in)[2], 0) ))
/*	
	untested but useful macros. Test them before you unmark them.
#define UTF8_4_to_UCS4(in)	((int32) (MXS((in)[0],0xF0,18) | M80S((in)[1],12) | M80S((in)[2], 6) | M80S((in)[3], 0) ))
#define UTF8_5_to_UCS4(in)	((int32) (MXS((in)[0],0xF8,24) | M80S((in)[1],18) | M80S((in)[2],12) | M80S((in)[3], 6) | M80S((in)[4], 0)  ))
#define UTF8_6_to_UCS4(in)	((int32) (MXS((in)[0],0xFC,30) | M80S((in)[1],24) | M80S((in)[2],18) | M80S((in)[3],12) | M80S((in)[4], 6) | M80S((in)[5],0)  ))
*/

XP_Bool
INTL_ConvertNPC(unsigned char *in, uint16 inlen, uint16 *inread,
	unsigned char *out, uint16 outbuflen, uint16 *outlen, int16 *outcsid)
{
	uint16 cinlen;
	uint16 cinread;
	uint16 coutbuflen;
	uint16 coutlen;
	XP_Bool csid_unknown = TRUE;
	uint16 thisidx;
	int16 thiscsid;
	
	int item;

	for(cinread = 0,coutlen = 0, cinlen = inlen,coutbuflen = outbuflen,item=0; 
			cinread < inlen;
			*outlen = coutlen,*inread = cinread,item++)
	{
		thisidx = NPCIndex(in[cinread]);
		switch(thisidx)
		{
			
			case npcSBI:	/*	In here we processs the non-ASCII portion of single byte character */
				{
					thiscsid =  NPCToCSID(in[cinread++]);	/* get csid from the first byte */
					if(csid_unknown)
					{
						*outcsid = thiscsid;				/* set csid if csid unknown */
						csid_unknown = FALSE;				/* now we know it */
					}
					else 
					{
						if(thiscsid != *outcsid )			/* return if csid is known and is differnt from this one */
							return (item>0);				/* if we already procress something, return true */
					}
			   		if((cinlen < 2) || (coutbuflen < 1))	/* return if boundary error */
			               return (item>0);					/* if we already procress something, return true */
			    	out[coutlen++] = in[cinread++];			/* Copy the second byte */
			    	coutbuflen--;							/* Adjust the coutbuflen and cinlen */
			    	cinlen -= 2;
		    	}
				/*	Continue the loop */
				break;
			case npcTBI:	/*	In here we processs two byte characters 
									(e.g. JIS0208, JIS0212, GB2312, KSC5601, CNS11643_1, CNS11643_2 ) 	*/
				{
					thiscsid =  NPCToCSID(in[cinread++]);	/* get csid from the first byte */
					if(csid_unknown)		
					{
						*outcsid = thiscsid;				/* set csid if csid unknown */
						csid_unknown = FALSE;				/* now we know it */
					}
					else 
					{
						if(thiscsid != *outcsid )			/* return if csid is known and is differnt from this one */
							return (item>0);				/* if we already procress something, return true */
					}
					if((cinlen < 3) || (coutbuflen < 2))	/* return if boundary error */
						return (item>0);					/* if we already procress something, return true */
					out[coutlen++] = in[cinread++];			/* Copy the second byte */
					out[coutlen++] = in[cinread++];			/* Copy the third byte */
					coutbuflen -= 2;						/* Adjust the coutbuflen and cinlen */
					cinlen -= 3;			
				}
				/*	Continue the loop */
				break;
			case npcMBI:	/*	In here we processs multi byte character (e.g. SJIS, BIG5, GB_8BIT, KSC_8BIT ) */
				{
					uint16 L;
					uint16 i;
					uint16 csinfo_index = NPCContext(in[cinread]);	/* get csinfo_index from the first byte */
					thiscsid =  NPCToCSID(in[cinread++]);			/* get csid from the first byte */
					if(csid_unknown)
					{
						*outcsid = thiscsid;						/* set csid if csid unknown */
						csid_unknown = FALSE;						/* now we know it */
					}
					else 
					{
						if(thiscsid != *outcsid )			/* return if csid is known and is differnt from this one */
							return  (item>0);				/* if we already procress something, return true */
					}
					if(cinlen < 2)							/* return if boundary error */
						return (item>0);					/* if we already procress something, return true */
					L = MBLen(csinfo_index,in[cinread]);	/* get length */
					
					/*	check length */
					if( (cinlen < L+1) || (coutbuflen <L))	/* return if boundary error */
						return (item>0);						/* if we already procress something, return true */
					for(i=0;i<L;i++)
						out[cinread++] = in[coutlen++];		/* Copy the rest byte */
					cinlen -= L+1;							/* Adjust the coutbuflen and cinlen */
					coutbuflen -= L;
				}
				/*	Continue the loop */
				break;
			case npcUTF8_1I:
				{
					thiscsid =  UTF8_1_CSID;
					if(csid_unknown)
					{
						*outcsid = thiscsid;				/* set csid if csid unknown */
						/* for ascii portion, although we set the outcsid, we still think
							we unknow csid so, the outcsid could be overwrite */
					}
					if((cinlen < 1) || (coutbuflen < 1))
						return (item>0);
					out[coutlen++] = in[cinread++];
				}
				/*	Continue the loop */
				break;
			case npcUTF8_2I:
				/*	In here, we handle the case for 2 bytes UTF8 */
				/*	It will return only process one NPC for UTF8 2 byte cases */
				/*  Later, we may change it so it can process more than one NPC  */
				{
					uint16 ucs2;
					if(item != 0)
						return TRUE;
					if(inlen < 2)
						return FALSE;
					*inread = 2;
					ucs2 = UTF8_2_to_UCS2(in);
					return UCS2_To_Other(ucs2,out,outbuflen,outlen,outcsid);
				}
				/*	return- don't continue the loop */
				break;
			case npcUTF8_3I:
				/*	In here, we handle the case for 3 bytes UTF8 */
				/*	It will return only process one NPC for UTF8 3 byte cases */
				/*  Later, we may change it so it can process more than one NPC  */
				{
					uint16 ucs2;
					if(item != 0)
						return TRUE;
					if(inlen < 3)
						return FALSE;
					*inread = 3;
					ucs2 = UTF8_3_to_UCS2(in);
					return UCS2_To_Other(ucs2,out,outbuflen,outlen,outcsid);
				}
				/*	return- don't continue the loop */
				break;
			case npcUTF8_4I:
			case npcUTF8_5I:
			case npcUTF8_6I:
				/*	In here, we handle the case for UCS4 */
				/*  Currently , we simply eat all UCS4 and generate a '?'	*/
				/*	Later, we can change it so it will generate something like "[U+XXXXXXXX]" */
				{
					uint16 ucs4inlen;
					thiscsid = UTF8_1_CSID;
					if(csid_unknown)
						*outcsid = thiscsid;
					else 
					{
						if(thiscsid != *outcsid )
							return (item>0);
					}
					switch(thisidx)
					{	
						case npcUTF8_4I:	ucs4inlen = 4; break;
						case npcUTF8_5I:	ucs4inlen = 5; break;
						case npcUTF8_6I:	ucs4inlen = 6; break;
					}
					if((cinlen < ucs4inlen) || (coutbuflen < 1))
						return (item>0);
					out[coutlen++] = '?';
					cinread += ucs4inlen;
				}
				/*	Continue the loop */
				break;
			case npcErrI:
			default:
				/*	In here, we handle Error cases */
				/*	We should never hit here unless something very wrong. */
				XP_ASSERT(FALSE);
				return FALSE;
				/*	return- don't continue the loop */
		}
	}	

	return TRUE;
}


unsigned char *
intl_ISO88591ToNPC(CCCDataObject *obj, const unsigned char *buf, int32 len)
{
	uint16		inread;
	uint16		outbuflen;
	int16		outcsid;
	uint16		outlen;
	unsigned char	*ret;

	if ((!obj) || (!buf) || (len < 0))
	{
		return NULL;
	}

	outbuflen = ((len * 2) + 1);
	ret = XP_ALLOC(outbuflen);
	if (!ret)
	{
		obj->retval = MK_OUT_OF_MEMORY;
		return NULL;
	}

	INTL_ConvertNPC((unsigned char *) buf, (uint16) len, &inread, ret,
		outbuflen - 1, &outlen, &outcsid);

	ret[outlen] = 0;
	obj->len = outlen;

	return ret;
}


#ifdef NOT_YET

unsigned char *
intl_2022KRToNPC(CCCDataObject *obj, const unsigned char *buf, int32 len)
{
}


unsigned char *
intl_ASCIIToNPC(CCCDataObject *obj, const unsigned char *buf, int32 len)
{
}


unsigned char *
intl_BIG5ToNPC(CCCDataObject *obj, const unsigned char *buf, int32 len)
{
}


unsigned char *
intl_EUCCNToNPC(CCCDataObject *obj, const unsigned char *buf, int32 len)
{
}


unsigned char *
intl_EUCJPToNPC(CCCDataObject *obj, const unsigned char *buf, int32 len)
{
}


unsigned char *
intl_EUCKRToNPC(CCCDataObject *obj, const unsigned char *buf, int32 len)
{
}


unsigned char *
intl_EUCTWToNPC(CCCDataObject *obj, const unsigned char *buf, int32 len)
{
}


unsigned char *
intl_ISO88592ToNPC(CCCDataObject *obj, const unsigned char *buf, int32 len)
{
}


unsigned char *
intl_JISToNPC(CCCDataObject *obj, const unsigned char *buf, int32 len)
{
}


unsigned char *
intl_SJISToNPC(CCCDataObject *obj, const unsigned char *buf, int32 len)
{
}

#endif /* NOT_YET */
