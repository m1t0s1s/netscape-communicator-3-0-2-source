/*----------------------------------------------------------------------------

	Function UCS2ToValueAndInfo
	
----------------------------------------------------------------------------*/
#include "ugen.h"
#include "umap.h"
#include "csid.h"
#include "xp_mem.h"
#include "xpassert.h"
#include "unicpriv.h"
#include "npc.h"
#include "libi18n.h"
#ifdef XP_WIN
#include "prlink.h"
#endif
/*
	Our global table are deivded into 256 row
	each row have 256 entries
	each entry have one value and one info
	Info field contains csid index
*/

typedef struct {
	uint16 value[256];
	unsigned char info[256];
} uRowTable;

static uRowTable *uRowTablePtArray[256];

static uTable* 	LoadToUCS2Table(uint16 csid);
static void 	UnloadToUCS2Table(uint16 csid, uTable *utblPtr);
static uTable* 	LoadFromUCS2Table(uint16 csid);
static void 	UnloadFromUCS2Table(uint16 csid, uTable *utblPtr);
static void 	CheckAndAddEntry(uint16 ucs2, uint16 med , uint16 csid);
static XP_Bool 	UCS2ToValueAndInfo(uint16 ucs2, uint16* med, unsigned char* info);
static void 	InitUCS2Table(void);

#ifdef XP_UNIX

/*	Currently, we only support the Latin 1 and Japanese Table. */
/*  We will add more table here after the first run */
/*--------------------------------------------------------------------------*/
/*	Latin stuff */
static uint16 iso88591FromTbl[] = {
#include "8859-1.uf"
};
static uint16 iso88591ToTbl[] = {
#include "8859-1.ut"
};
/*--------------------------------------------------------------------------*/
/*	Japanese stuff */
static uint16 JIS0208FromTbl[] = {
#include "jis0208.uf"
};
static uint16 JIS0208ToTbl[] = {
#include "jis0208.ut"
};
static uint16 JIS0201FromTbl[] = {
#include "jis0201.uf"
};
static uint16 JIS0201ToTbl[] = {
#include "jis0201.ut"
};
static uint16 JIS0212FromTbl[] = {
#include "jis0212.uf"
};
static uint16 JIS0212ToTbl[] = {
#include "jis0212.ut"
};
static uint16 SJISFromTbl[] = {
#include "sjis.uf"
};
static uint16 SJISToTbl[] = {
#include "sjis.ut"
};

/*--------------------------------------------------------------------------*/
/*	Latin2 Stuff */
static uint16 iso88592FromTbl[] = {
#include "8859-2.uf"
};
static uint16 iso88592ToTbl[] = {
#include "8859-2.ut"
};
/*--------------------------------------------------------------------------*/
/*	Traditional Chinese Stuff */
static uint16 CNS11643_1FromTbl[] = {
#include "cns_1.uf"
};
static uint16 CNS11643_1ToTbl[] = {
#include "cns_1.ut"
};
static uint16 CNS11643_2FromTbl[] = {
#include "cns_2.uf"
};
static uint16 CNS11643_2ToTbl[] = {
#include "cns_2.ut"
};
static uint16 Big5FromTbl[] = {
#include "big5.uf"
};
static uint16 Big5ToTbl[] = {
#include "big5.ut"
};
/*--------------------------------------------------------------------------*/
/*	Simplified Chinese Stuff */
static uint16 GB2312FromTbl[] = {
#include "gb2312.uf"
};
static uint16 GB2312ToTbl[] = {
#include "gb2312.ut"
};
/*--------------------------------------------------------------------------*/
/*	Korean Stuff */
static uint16 KSC5601FromTbl[] = {
#include "ksc5601.uf"
};
static uint16 KSC5601ToTbl[] = {
#include "ksc5601.ut"
};
/*--------------------------------------------------------------------------*/
/*	Symbol Stuff */
static uint16 SymbolFromTbl[] = {
#include "macsymbo.uf"
};
static uint16 SymbolToTbl[] = {
#include "macsymbo.ut"
};

/*--------------------------------------------------------------------------*/
/*	Dingbats Stuff */
static uint16 DingbatsFromTbl[] = {
#include "macdingb.uf"
};
static uint16 DingbatsToTbl[] = {
#include "macdingb.ut"
};
/*--------------------------------------------------------------------------*/
static uTable* LoadToUCS2Table(uint16 csid)
{
	switch(csid) {
	case CS_ASCII:
		return (uTable*) iso88591ToTbl;
		
	/*	Latin stuff */
	case CS_LATIN1:
		return (uTable*) iso88591ToTbl;
		
	/*	Japanese */
	case CS_JISX0208:
		return (uTable*) JIS0208ToTbl;

	case CS_JISX0201:
		return (uTable*) JIS0201ToTbl;

	case CS_JISX0212:
		return (uTable*) JIS0212ToTbl;
	case CS_SJIS:
		return (uTable*) SJISToTbl;

	/*	Latin2 Stuff */
	case CS_LATIN2:
		return (uTable*) iso88592ToTbl;
	/*	Traditional Chinese Stuff */
	case CS_CNS11643_1:
		return (uTable*) CNS11643_1ToTbl;
	case CS_CNS11643_2:
		return (uTable*) CNS11643_2ToTbl;
	case CS_BIG5:
	case CS_X_BIG5:
		return (uTable*) Big5ToTbl;


	/*	Simplified Chinese Stuff */
	case CS_GB2312:
	case CS_GB_8BIT:
		return (uTable*) GB2312ToTbl;

	/*	Korean Stuff */
	case CS_KSC5601:
	case CS_KSC_8BIT:
		return (uTable*) KSC5601ToTbl;

	/*	Symbol Stuff */
	case CS_SYMBOL:
		return (uTable*) SymbolToTbl;

	/*	Dingbats Stuff */
	case CS_DINGBATS:
		return (uTable*) DingbatsToTbl;

	/*	Other Stuff */
	default:
		XP_ASSERT(TRUE);
		return NULL;
	}
}
static uTable* LoadFromUCS2Table(uint16 csid)
{
	switch(csid) {
	case CS_ASCII:
		return (uTable*) iso88591FromTbl;
		
	/*	Latin stuff */
	case CS_LATIN1:
		return (uTable*) iso88591FromTbl;
		
	/*	Japanese */
	case CS_JISX0208:
		return (uTable*) JIS0208FromTbl;

	case CS_JISX0201:
		return (uTable*) JIS0201FromTbl;

	case CS_JISX0212:
		return (uTable*) JIS0212FromTbl;

	case CS_SJIS:
		return (uTable*) SJISFromTbl;

	/*	Latin2 Stuff */
	case CS_LATIN2:
		return (uTable*) iso88592FromTbl;
	/*	Traditional Chinese Stuff */
	case CS_CNS11643_1:
		return (uTable*) CNS11643_1FromTbl;
	case CS_CNS11643_2:
		return (uTable*) CNS11643_2FromTbl;
	case CS_X_BIG5:
	case CS_BIG5:
		return (uTable*) Big5FromTbl;


	/*	Simplified Chinese Stuff */
	case CS_GB2312:
	case CS_GB_8BIT:
		return (uTable*) GB2312FromTbl;

	/*	Korean Stuff */
	case CS_KSC5601:
	case CS_KSC_8BIT:
		return (uTable*) KSC5601FromTbl;

	/*	Symbol Stuff */
	case CS_SYMBOL:
		return (uTable*) SymbolFromTbl;

	/*	Dingbats Stuff */
	case CS_DINGBATS:
		return (uTable*) DingbatsFromTbl;

	/*	Other Stuff */
	default:
		XP_ASSERT(TRUE);
		return NULL;
	}
}

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
static void UnloadToUCS2Table(uint16 csid, uTable *utblPtr)
{
	/* If we link those table in our code. We don't need to do anything to 
		unload them */
}
/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
static void UnloadFromUCS2Table(uint16 csid, uTable *utblPtr)
{
	/* If we link those table in our code. We don't need to do anything to 
		unload them */
}

#endif /* XP_UNIX */

#ifdef XP_MAC
/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
static uTable* LoadUCS2Table(uint16 csid,int from)
{
	/* We need to add reference count here */
	Handle tableHandle;
	if(csid == CS_ASCII)
		csid = CS_MAC_ROMAN;
	tableHandle = GetResource((from ? 'UFRM' : 'UTO '), csid);
	if(tableHandle == NULL || ResError()!=noErr)
		return NULL;
	if(*tableHandle == NULL)
		LoadResource(tableHandle);
	HNoPurge(tableHandle);
	HLock(tableHandle);
	return((uTable*) *tableHandle);
}
/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
static void UnloadUCS2Table(uint16 csid, uTable *utblPtr, int from)
{
	/* We need to add reference count here */
	Handle tableHandle;
	if(csid == CS_ASCII)
		csid = CS_MAC_ROMAN;
	tableHandle = GetResource((from ? 'UFRM' : 'UTO '), csid);
	if(tableHandle == NULL || ResError()!=noErr)
		return;
	HUnlock((Handle) tableHandle);
	HPurge(tableHandle);
}
/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
static uTable* LoadToUCS2Table(uint16 csid)
{
	return LoadUCS2Table(csid, FALSE);
}
/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
static void UnloadToUCS2Table(uint16 csid, uTable *utblPtr)
{
	UnloadUCS2Table(csid, utblPtr, FALSE);
}
/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
static uTable* LoadFromUCS2Table(uint16 csid)
{
	return LoadUCS2Table(csid, TRUE);
}
/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
static void UnloadFromUCS2Table(uint16 csid, uTable *utblPtr)
{
	UnloadUCS2Table(csid, utblPtr, TRUE);
}
#endif /* XP_MAC */

#ifdef XP_WIN

#ifdef XP_WIN32
#define UNICODEDLL "UNI3200.DLL"
#define LIBRARYLOADOK(l)	(l != NULL)
#define UNICODE_LOADUCS2TABLE_SYM	"UNICODE_LoadUCS2Table"
#define UNICODE_UNLOADUCS2TABLE_SYM	"UNICODE_UnloadUCS2Table"
#else
#define UNICODEDLL "UNI1600.DLL"
#define LIBRARYLOADOK(l)	(l >= 32)
#define UNICODE_LOADUCS2TABLE_SYM	"_UNICODE_LoadUCS2Table"
#define UNICODE_UNLOADUCS2TABLE_SYM	"_UNICODE_UnloadUCS2Table"
#endif	/* !XP_WIN32 */

PRLibrary* uniLib = NULL;

static uTable* LoadUCS2Table(uint16 csid, int from)
{
	uTable* ret = NULL;
	if(uniLib == NULL )
		uniLib = PR_LoadLibrary(UNICODEDLL);
	if(uniLib)
	{
		typedef uTable* (*f) (uint16 i1, int i2);
		f p = NULL;
		p = (f)PR_FindSymbol(UNICODE_LOADUCS2TABLE_SYM, uniLib);
		XP_ASSERT(p);
		if(p)
			ret = (*p)(csid, from);
	}
	return ret;
}
static void UnloadUCS2Table(uint16 csid, uTable* utblPtr, int from)
{
	if(uniLib == NULL )
		uniLib = PR_LoadLibrary(UNICODEDLL);
	if(uniLib)
	{
		typedef void (*f) (uint16 i1, uTable* i2, int i3);
		f p = NULL;
		p = (f)PR_FindSymbol(UNICODE_UNLOADUCS2TABLE_SYM, uniLib);
		XP_ASSERT(p);
		if(p)
			(*p)(csid, utblPtr, from);
	}
}
static uTable* LoadToUCS2Table(uint16 csid)
{
	return(LoadUCS2Table(csid,0));
}
static void UnloadToUCS2Table(uint16 csid, uTable *utblPtr)
{
	UnloadUCS2Table(csid, utblPtr, 0);
}
static uTable* LoadFromUCS2Table(uint16 csid)
{
	return(LoadUCS2Table(csid,1));
}
static void UnloadFromUCS2Table(uint16 csid, uTable *utblPtr)
{
	UnloadUCS2Table(csid, utblPtr, 1);
}
#endif /* XP_WIN */

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
static uRowTable* AddAndInitOneRow(uint16 hb)
{
	/*	Allocate uRowTablePtArray[hb] and initialize it */
	uint16 i;
	uRowTable *row = XP_ALLOC(sizeof(uRowTable));
	if(row == NULL)
	{
		XP_ASSERT(row != 0);
		return NULL;
	}
	else
	{
		for(i = 0; i < 256 ;i++)
		{
			row->value[i] = NOMAPPING;
		}
		uRowTablePtArray[hb] = row;
	}
	return row;
}
/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
static void AddAndInitAllRows(void)
{
	uint16 i;
	for(i=0;i<256;i++)
		(void)  AddAndInitOneRow(i);
}
/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
static XP_Bool RowUsed(uint16 rownum)
{
	uint16 c;
	uRowTable *row = uRowTablePtArray[ rownum] ;
	
	for(c=0;c<256;c++)
	{
		if(row->value[c] != NOMAPPING)
			return TRUE;
	}
	return FALSE;
}
/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
static void FreeRow(uint16 row)
{
	XP_FREE(uRowTablePtArray[row]);
	uRowTablePtArray[row] = NULL;
}
/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
static void FreeUnusedRows(void)
{
	uint16 i;
	for(i=0;i<256;i++)
	{
		if(! RowUsed(i))
			FreeRow(i);
	}
}
/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
static void CheckAndAddEntry(uint16 ucs2,  uint16 med, uint16 csid)
{
	uint16 lb = ucs2 & 0x00FF;
	uRowTable *row = uRowTablePtArray[ucs2 >> 8];
	if(row->value[lb] == NOMAPPING)
	{
		row->value[lb]= med;
		row->info[lb]= (csid & 0xFF);
	}
}
/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
static 
XP_Bool 	
UCS2ToValueAndInfo(uint16 ucs2, uint16* med, unsigned char* info)
{
	uRowTable *uRowTablePtr = uRowTablePtArray[(ucs2 >> 8)];
	if( uRowTablePtr == NULL)
		return FALSE;
	*med = 	uRowTablePtr->value[(ucs2 & 0x00ff)];
	*info = uRowTablePtr->info[(ucs2 & 0x00ff)];
	return ( *med != NOMAPPING ); 
}

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

static void InitUCS2Table(void)
{
	int16 i;
	for(i=0;i<256; i++)
		uRowTablePtArray[i] = NULL;
}
/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
XP_Bool 
UCS2_To_Other(
	uint16 ucs2, 
	unsigned char *out, 
	uint16 outbuflen, 
	uint16* outlen,
	int16 *outcsid
)
{
	uint16 med;
	unsigned char info;
	uShiftTable* shiftTable;
#ifdef XP_MAC
	if(ucs2 == 0x000a)
		ucs2 = 0x000d;
#endif
	if(UCS2ToValueAndInfo(ucs2, &med, &info))
	{
		*outcsid = NPCToCSID(info);
		XP_ASSERT(*outcsid != CS_UNKNOWN);	
		shiftTable = InfoToShiftTable(info);
		XP_ASSERT(shiftTable);	
	 	return uGenerate(shiftTable, (int32*)0, med, out,outbuflen, outlen);
 	}
	return FALSE;
}

static int16* unicodeCSIDList = NULL;
static unsigned char** unicodeCharsetNameList = NULL;
static uint16 numOfUnicodeList = 0;

int16*  INTL_GetUnicodeCSIDList(int16 * outnum)
{
	*outnum  = numOfUnicodeList;
	return unicodeCSIDList;
}
unsigned char **INTL_GetUnicodeCharsetList(int16 * outnum)
{
	*outnum  = numOfUnicodeList;
	return unicodeCharsetNameList;
}

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
void	INTL_SetUnicodeCSIDList(uint16 numOfItems, int16* csidlist)
{
	int i;
	uTable* utbl;
	/* This function should be called once only */
	XP_ASSERT(unicodeCSIDList == NULL);	
	XP_ASSERT(unicodeCharsetNameList == NULL);	

	unicodeCSIDList = XP_ALLOC(sizeof(int16) * numOfItems);
	/* XXX ftang needs to handle no memory */
	XP_ASSERT(unicodeCSIDList != NULL);

	unicodeCharsetNameList = XP_ALLOC(sizeof(unsigned char*) * numOfItems);
	/* XXX  ftang needs to handle no memory*/
	XP_ASSERT(unicodeCharsetNameList != NULL);

	numOfUnicodeList = numOfItems;
	InitUCS2Table();
	AddAndInitAllRows();
	/*	Add the first table */
	for(i = 0 ; i < numOfItems; i++)
	{
		unicodeCSIDList[i] = csidlist[i];
		unicodeCharsetNameList[i]=  INTL_CsidToCharsetNamePt(csidlist[i]);
		if((utbl = LoadFromUCS2Table(csidlist[i])) != NULL)
		{
			uMapIterate(utbl,CheckAndAddEntry, csidlist[i]);
			UnloadFromUCS2Table(csidlist[i],utbl);
			
		}
	}
	FreeUnusedRows();
}
/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
typedef struct UnicodeConverterPriv UnicodeConverterPriv;

typedef UnicodeConverterPriv* INTL_UnicodeToStrIteratorPriv;
struct UnicodeConverterPriv
{
	INTL_Unicode		*ustr;
	uint32			ustrlen;
};


/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
/*	Pricate Function Declartion */
static
XP_Bool 
UnicodeToStrWithFallback_p(
	uint16 ucs2, 
	unsigned char *out, 
	uint32  outbuflen, 
	uint32* outlen,
	uint16 *outcsid
);
static 
XP_Bool 
UnicodeToStrFirst_p(
	uint16 ucs2, 
	unsigned char *out, 
	uint32 outbuflen, 
	uint32* outlen,
	uint16 *outcsid
);
static
XP_Bool 
UnicodeToStrNext_p(
	uint16 ucs2, 
	unsigned char *out, 
	uint32 outbuflen, 
	uint32* outlen, 
	uint16 lastcsid
);



/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
/* 	the return of FLASE of this funciton only mean one thing - the outbuf 
	is not enough for this conversion */
static
XP_Bool 
UnicodeToStrWithFallback_p(
	uint16 ucs2, 
	unsigned char *out, 
	uint32 outbuflen, 
	uint32* outlen,
	uint16 *outcsid)
{
	uint16 outlen16;
	if(! UCS2_To_Other(ucs2, out, (uint16)outbuflen, &outlen16, (int16 *)outcsid))
	{
		if(outbuflen > 2)
		{
#ifdef XP_MAC
			*outcsid = CS_MAC_ROMAN;
#else
			*outcsid = CS_LATIN1;
#endif
			out[0]= '?';
			*outlen =1;
			return TRUE;
		}
		else
			return FALSE;
	}
	else
	{
		*outlen = outlen16;
	}
	return TRUE;
}

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
static 
XP_Bool 
UnicodeToStrFirst_p(
	uint16 ucs2, 
	unsigned char *out, 
	uint32 outbuflen, 
	uint32* outlen,
	uint16 *outcsid)
{
  return  UnicodeToStrWithFallback_p(ucs2,out,outbuflen,outlen,outcsid);
}
/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
static
XP_Bool 
UnicodeToStrNext_p(
	uint16 ucs2, 
	unsigned char *out, 
	uint32 outbuflen, 
	uint32* outlen, 
	uint16 lastcsid)
{
	uint16 thiscsid;
	XP_Bool retval = 
		UnicodeToStrWithFallback_p(ucs2,out,outbuflen,outlen,&thiscsid);
	return (retval && (thiscsid == lastcsid));
}
/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

INTL_UnicodeToStrIterator    
INTL_UnicodeToStrIteratorCreate(
	INTL_Unicode*        	 ustr,
    uint32            		 ustrlen,
    INTL_Encoding_ID     	*encoding,
    unsigned char*         	 dest, 
    uint32            		 destbuflen
)
{
	UnicodeConverterPriv* priv=0;
	priv=XP_ALLOC(sizeof(UnicodeConverterPriv));
	if(priv)
	{
		priv->ustrlen = ustrlen;
		priv->ustr = ustr;
		(void)INTL_UnicodeToStrIterate((INTL_UnicodeToStrIterator)priv, 
									encoding, dest, destbuflen);
	}
	else
	{
		*encoding = 0;
		dest[0] = '\0';
	}
	return (INTL_UnicodeToStrIterator)priv;
}
/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

int INTL_UnicodeToStrIterate(
    INTL_UnicodeToStrIterator     	iterator,
    INTL_Encoding_ID         		*encoding,
    unsigned char*            		dest, 
    uint32                    		destbuflen
)
{
    	unsigned char* orig = dest;
	UnicodeConverterPriv* priv = (UnicodeConverterPriv*)iterator;
	if(destbuflen < 2)		/* we want to make sure there at least two byte in the buffer */
		return 0;			/* first one for the first char, second one for the NULL */ 
    destbuflen -= 1;		/* resever one byte for NULL terminator */
	if((priv == NULL) || ((priv->ustrlen) == 0))
	{
		*encoding = 0;
		dest[0]='\0';
		return 0;
	}
	else
	{
		uint32 len = 0;
		if(UnicodeToStrFirst_p(*(priv->ustr),  
				dest,destbuflen,&len,encoding))
		{
			do{
				dest += len;
				destbuflen -= len;
				priv->ustr += 1;
				priv->ustrlen -= 1 ;
			} while(	(destbuflen > 0) && 
						((priv->ustrlen > 0)) && 
						UnicodeToStrNext_p(*(priv->ustr), dest, destbuflen, 
											&len, *encoding));
		}
		dest[0] = '\0';
		return (orig != dest);
	}
}

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
void    
INTL_UnicodeToStrIteratorDestroy(
	INTL_UnicodeToStrIterator iterator
)
{
	UnicodeConverterPriv* priv = (UnicodeConverterPriv*)iterator;
	if(priv)
		XP_FREE(priv);
}
/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
uint32 INTL_UnicodeLen(INTL_Unicode *ustr)
{
	uint32 i;
	for(i=0;*ustr++;i++)	
		;
	return i;
}

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
uint32    INTL_UnicodeToStrLen(
    INTL_Encoding_ID encoding,
    INTL_Unicode*    ustr,
    uint32 ustrlen
)
{
	/* for now, put a dump algorithm to caculate the length */
	return ustrlen * ((encoding & MULTIBYTE) ?  4 : 1) + 1;
}
/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
static 
int
LoadUCS2TableSet(uint16 csid, uTableSet* tableset,int from)
{
	UnicodeTableSet* set;
	int i;
	for(i=0;i<MAXINTERCSID;i++)
	{
		tableset->range[i].intercsid=CS_DEFAULT;
		tableset->tables[i]=NULL;
		tableset->shift[i] = NULL;
		tableset->range[i].min = 0xff;
		tableset->range[i].max = 0x00;
	}
	set = GetUnicodeTableSet(csid);
	XP_ASSERT(set);
	if(set == NULL)
	{
		return 0;
	}
	for(i=0;((i<MAXINTERCSID) && (set->range[i].intercsid != CS_DEFAULT));i++)
	{
		tableset->range[i].intercsid=set->range[i].intercsid;
		tableset->range[i].min = set->range[i].min;
		tableset->range[i].max = set->range[i].max;
		if(from)
			  tableset->tables[i]=LoadFromUCS2Table(set->range[i].intercsid);
		else
			  tableset->tables[i]=LoadToUCS2Table(set->range[i].intercsid);
		tableset->shift[i] = GetShiftTableFromCsid(set->range[i].intercsid);
		XP_ASSERT(tableset->shift[i]);
		XP_ASSERT(tableset->tables[i]);
	}
	return i;
}
/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
static 
void 	
UnloadUCS2TableSet(uTableSet *tableset,int from)
{
	int i;
	if(tableset == NULL)
		return;
	for(i=0;i<MAXINTERCSID;i++)
	{
        if((tableset->range[i].intercsid != CS_DEFAULT) && (tableset->tables[i] != NULL))
		{
			if(from)
				UnloadFromUCS2Table(tableset->range[i].intercsid, tableset->tables[i]);
			else
				UnloadToUCS2Table(tableset->range[i].intercsid, tableset->tables[i]);
		}
		tableset->range[i].intercsid=CS_DEFAULT;
		tableset->tables[i]=NULL;
		tableset->shift[i] = NULL;
	}
}
/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
void    INTL_UnicodeToStr(
    INTL_Encoding_ID encoding,
    INTL_Unicode*    ustr,
    uint32 			 ustrlen,
    unsigned char*   dest,
    uint32           destbuflen
)
{
	uint16 num;
	uint16 u;
	uint16 med;
	uint32 cur;
	uTableSet tableset;
	/* load all the table we need */
	num = LoadUCS2TableSet(encoding, &tableset,TRUE);

	/* For every character */
	for(cur=0; cur < ustrlen ;cur++)
	{
		int i;
		u = (*ustr++);
#ifdef XP_MAC
		if(u == 0x000a)
			u = 0x000d;
#endif
	/* Loop to every table it need to convert */
		for(i=0;i<num;i++)
		{
			if((tableset.tables[i] != NULL) &&
			   (uMapCode(tableset.tables[i],u, &med)))
					break;
		}
		if(i!=num)
		{
			uint16 outlen;
			XP_Bool ret;
			/* MAP one, gen it */
			ret = uGenerate(tableset.shift[i], 
				(int32*)0, 
				med, 
				dest,
				destbuflen, 
				&outlen);

			XP_ASSERT(ret);

			dest+=outlen;
			destbuflen += outlen;
		}
		else
		{
			/* Ok! right before we fall back. We take care C0 area here */
			if(u <= 0x0020)
			{
				/* cannot map one, gen the fallback */
				*dest++ = u;
				destbuflen--;
			}
			else
			{			
				XP_ASSERT(destbuflen > 1);

				/* cannot map one, gen the fallback */
				*dest++ = '?';
				destbuflen--;
			}
		}
	}
	XP_ASSERT(destbuflen > 0);
	*dest = '\0';	/* NULL terminate it */
	/* Unload all the table we need */
	UnloadUCS2TableSet(&tableset,TRUE);
}
/*
	intl_check_unicode_question
	Used by INTL_UnicodeToEncodingStr
*/
static uint32 intl_check_unicode_question(
    INTL_Unicode*    ustr,
    uint32 			 ustrlen
)
{
	INTL_Unicode* p;
	uint32 count = 0;
	for(p=ustr; ustrlen > 0  ;ustrlen--, p++)
		if(*p == 0x003F)
			count++;
	return count;	
}
/*
	intl_check_unknown_unicode
	Used by INTL_UnicodeToEncodingStr
*/
static uint32 intl_check_unknown_unicode(unsigned char*   buf)
{
	unsigned char* p;
	uint32 count = 0;
	for(p=buf; *p != '\0'; p++)
		if(*p == '?')
			count++;
	return count;	
}
/*
	INTL_UnicodeToEncodingStr
	This is an Trail and Error function which may wast a lot of performance in "THE WORST CASE"
	However, it do it's best in the best case and average case.
	IMPORTANT ASSUMPTION: The unknown Unicode is fallback to '?'
*/
INTL_Encoding_ID    INTL_UnicodeToEncodingStr(
    INTL_Unicode*    ustr,
    uint32 			 ustrlen,
    unsigned char*   dest,
    uint32           destbuflen
)
{
    INTL_Encoding_ID latin1_encoding, encoding, min_error_encoding, last_convert_encoding;
    uint32 min, question;
    int16 *encodingList;
    int16 itemCount;
    int16 idx;
    
#ifdef XP_MAC
	encoding = latin1_encoding = CS_MAC_ROMAN;
#else
	encoding = latin1_encoding =  CS_LATIN1;
#endif
	/* Ok, let's try them with Latin 1 first. I believe this is for most of the case */
	INTL_UnicodeToStr(encoding,ustr,ustrlen,dest,destbuflen);
	/* Try to find the '?' in the converted string */
	min = intl_check_unknown_unicode(dest);
	if(min == 0)		/* No '?' in the converted string, it could be convert to Latin 1 */
		return encoding;
	/* The origional Unicode may contaion some '?' in unicode. Let's count it */		
	question = intl_check_unicode_question(ustr,ustrlen );
	/* The number of  '?' in the converted string match the number in unicode */
	if(min == question)	
		return encoding;
	
	last_convert_encoding = min_error_encoding = encoding;
		
	encodingList = INTL_GetUnicodeCSIDList(&itemCount);
	for(idx = 0; idx < itemCount ; idx++)
	{
		encoding = encodingList[idx];
		/* Let's ignore the following three csid
			the latin1 (we already try it 
			Symbol an Dingbat
		*/
		if((encoding != latin1_encoding) && 
		   (encoding != CS_SYMBOL) &&
		   (encoding != CS_DINGBATS))
		{   
	    	uint32 unknowInThis;
	    	last_convert_encoding = encoding;
			INTL_UnicodeToStr(encoding,ustr,ustrlen,dest,destbuflen);
			unknowInThis = intl_check_unknown_unicode(dest);
			/* The number of  '?' in the converted string match the number in unicode */
			if(unknowInThis == question)	/* what a perfect candidcate */
				return encoding;
			/* The number of  '?' is less then the previous smallest */
			if(unknowInThis < min)
			{   /* let's remember the encoding and the number of '?' */
				min = unknowInThis;
				min_error_encoding = encoding;
			}
		}
	}
	/* The min_error_encoding is not the last one we try to convert to. 
		We need to convert it again */
	if(min_error_encoding != last_convert_encoding)
		INTL_UnicodeToStr(min_error_encoding,ustr,ustrlen,dest,destbuflen);
	return min_error_encoding;
}

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
uint32    INTL_StrToUnicodeLen(
    INTL_Encoding_ID encoding,
    unsigned char*    src
)
{
	/* for now, put a dump algorithm to caculate the length */
	return INTL_TextToUnicodeLen(encoding, src, XP_STRLEN((char*)src));
}
/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
uint32    INTL_StrToUnicode(
    INTL_Encoding_ID encoding,
    unsigned char*   src,
    INTL_Unicode*    ustr,
    uint32           ubuflen
)
{
	uint32  len = XP_STRLEN((char*)src);
	return INTL_TextToUnicode(encoding,src,len,ustr,ubuflen);
}
uint32    INTL_TextToUnicodeLen(
    INTL_Encoding_ID encoding,
    unsigned char*   src,
    uint32			 srclen
)
{
	/* for now, put a dump algorithm to caculate the length */
	return srclen + 1;
}
uint32    INTL_TextToUnicode(
    INTL_Encoding_ID encoding,
    unsigned char*   src,
    uint32			 srclen,
    INTL_Unicode*    ustr,
    uint32           ubuflen
)
{
	uint32	validlen;
	uint16 num,scanlen, med;
	uTableSet tableset;
	num = LoadUCS2TableSet(encoding, &tableset,FALSE);
	for(validlen=0;	(((*src) != '\0') && (srclen > 0) && (ubuflen > 1));
		srclen -= scanlen, src += scanlen, ustr++, ubuflen--,validlen++)
	{
		uint16 i;
		for(i=0;i<num;i++)
		{
			if((tableset.tables[i] != NULL) &&
			   (tableset.range[i].min <= src[0]) &&	
			   (src[0] <= tableset.range[i].max) &&	
			   (uScan(tableset.shift[i],(int32*) 0,src,&med,srclen,&scanlen)))
			{
				uMapCode(tableset.tables[i],med, ustr);
				if(*ustr != NOMAPPING)
					break;
			}
		}
		if(i==num)
		{
#ifdef STRICTUNICODETEST
			XP_ASSERT(i!=num);
#endif
			*ustr= NOMAPPING;
			scanlen=1;
		}
	}
	*ustr = (INTL_Unicode) 0;
	/* Unload all the table we need */
	UnloadUCS2TableSet(&tableset,FALSE);
	return validlen;
}
