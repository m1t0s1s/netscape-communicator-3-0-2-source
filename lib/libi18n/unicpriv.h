#ifndef __UNIPRIV__
#define __UNIPRIV__

#include "intlpriv.h"
#include "umap.h"
#include "ugen.h"

typedef struct uTableSet uTableSet;
struct uTableSet
{
	uTable *tables[MAXINTERCSID];
	uShiftTable   *shift[MAXINTERCSID];
	StrRangeMap	range[MAXINTERCSID];
};

XP_Bool		uMapCode(uTable *uTable, uint16 in, uint16* out);
XP_Bool 	uGenerate(uShiftTable *shift,int32* state, uint16 in, 
						unsigned char* out, uint16 outbuflen, uint16* outlen);
XP_Bool 	uScan(uShiftTable *shift, int32 *state, unsigned char *in,
						uint16 *out, uint16 inbuflen, uint16* inscanlen);
XP_Bool 	UCS2_To_Other(uint16 ucs2, unsigned char *out, uint16 outbuflen, uint16* outlen,int16 *outcsid);
typedef void (*uMapIterateFunc)(uint16 ucs2, uint16 med, uint16 context);
void		uMapIterate(uTable *uTable, uMapIterateFunc callback, uint16 context);

uShiftTable* 	InfoToShiftTable(unsigned char info);
uShiftTable* 	GetShiftTableFromCsid(uint16 csid);
UnicodeTableSet* GetUnicodeTableSet(uint16 csid);


#endif /* __UNIPRIV__ */
