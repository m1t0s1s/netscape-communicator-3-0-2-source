#include "nspr/prlog.h"
#include "nspr/prtypes.h"
#include "libi18n.h"
#include "csid.h"

#define MAXUTABLENAME	16
typedef struct tblrsrcinfo {
	char	name[MAXUTABLENAME];
	uint16	refcount;
    HGLOBAL hTbl;
} tblrsrcinfo;

typedef struct utablename {
	uint16		csid;
	tblrsrcinfo	frominfo;
	tblrsrcinfo toinfo;

} utablename;

PR_PUBLIC_API(void*) 	UNICODE_LoadUCS2Table(uint16 csid, int from);
PR_PUBLIC_API(void)	UNICODE_UnloadUCS2Table(uint16 csid, void *utblPtr, int from);

tblrsrcinfo* 	unicode_FindUTableName(uint16 csid, int from);

HINSTANCE _unicode_hInstance;

#ifdef _WIN32
BOOL WINAPI DllMain (HINSTANCE hDLL, DWORD dwReason, LPVOID lpReserved)
{
  switch (dwReason)
  {
    case DLL_PROCESS_ATTACH:
      _unicode_hInstance = hDLL;
      break;

    case DLL_THREAD_ATTACH:
      break;

    case DLL_THREAD_DETACH:
      break;

    case DLL_PROCESS_DETACH:
      _unicode_hInstance = NULL;
      break;
  }

    return TRUE;
}

#else  /* ! _WIN32 */

int CALLBACK LibMain( HINSTANCE hInst, WORD wDataSeg, 
                      WORD cbHeapSize, LPSTR lpszCmdLine )
{
    _unicode_hInstance = hInst;
    return TRUE;
}

BOOL CALLBACK _export _loadds WEP(BOOL fSystemExit)
{
    _unicode_hInstance = NULL;
    return TRUE;
}
#endif /* ! _WIN32 */


utablename utablenametbl[] =
{
	/* Special Note, for Windows, we use cp1252 for CS_LATIN1 */
	{CS_ASCII,		{"cp1252.uf",	0,	NULL},		{"cp1252.ut",	0,	NULL}},
	{CS_LATIN1,		{"cp1252.uf",	0,	NULL},		{"cp1252.ut",	0,	NULL}},
	{CS_LATIN2,		{"8859-2.uf",	0,	NULL},		{"8859-2.ut",	0,	NULL}},
	{CS_SJIS,		{"sjis.uf",		0,	NULL},		{"sjis.ut",		0,	NULL}},
	{CS_BIG5,		{"big5.uf",		0,	NULL},		{"big5.ut",		0,	NULL}},
	{CS_GB_8BIT,	{"gb2312.uf",	0,	NULL},		{"gb2312.ut",	0,	NULL}},
	{CS_KSC_8BIT,	{"ksc5601.uf",	0,	NULL},		{"ksc5601.ut",	0,	NULL}},
	{CS_CP_1251,	{"cp1251.uf",	0,	NULL},		{"cp1251.ut",	0,	NULL}},
	{CS_CP_1250,		{"cp1250.uf",	0,	NULL},		{"cp1250.ut",	0,	NULL}},
	{CS_KOI8_R,		{"koi8-r.uf",	0,	NULL},		{"koi8-r.ut",	0,	NULL}},
	{CS_8859_7,		{"8859-7.uf",	0,	NULL},		{"8859-7.ut",	0,	NULL}},
	{CS_8859_9,		{"8859-9.uf",	0,	NULL},		{"8859-9.ut",	0,	NULL}},
	{CS_SYMBOL,		{"macsymbo.uf",	0,	NULL},		{"macsymbo.ut",	0,	NULL}},
	{CS_DINGBATS,	{"macdingb.uf",	0,	NULL},		{"macdingb.ut",	0,	NULL}},
	{CS_DEFAULT,	{"",			0,	NULL},		{"",			0,	NULL}}
};
static tblrsrcinfo* unicode_FindUTableName(uint16 csid, int from)
{
	int i;
	for(i=0; utablenametbl[i].csid != CS_DEFAULT; i++)
	{
		if(utablenametbl[i].csid == csid)
			return from ? &(utablenametbl[i].frominfo) 
						: &(utablenametbl[i].toinfo);
	}
	OutputDebugString("unicode_FindUTableName: Cannot find table information");

	return NULL;
}
PR_PUBLIC_API(void *) UNICODE_LoadUCS2Table(uint16 csid, int from)
{
    HRSRC   hrsrc;
    HGLOBAL hRes;
	void *table;
	tblrsrcinfo* tbl = unicode_FindUTableName(csid, from);
	/*	Cannot find this csid */
	if(tbl == NULL)
		return (NULL);
	/*  Find a loaded table */
	if(tbl->refcount > 0)
	{
		tbl->refcount++;
		return ((void*)LockResource(tbl->hTbl));
	}
	/*  Find a unloaded table */
	hrsrc = FindResource(_unicode_hInstance, tbl->name, RT_RCDATA);
	if(!hrsrc) 
	{
		/* cannot find that RCDATA resource */
#ifdef DEBUG
		OutputDebugString("UNICODE_LoadUCS2Table cannot find table resource");
#endif
		return (NULL);
	}
	hRes = LoadResource(_unicode_hInstance,hrsrc);
	if(!hRes) 
	{
		/* cannot find that RCDATA resource */
#ifdef DEBUG
		OutputDebugString("UNICODE_LoadUCS2Table cannot load table resource");
#endif
		return (NULL);
	}
	table = (void*)	LockResource(hRes);
	if(!table)
	{
		/* cannot find that RCDATA resource */
#ifdef DEBUG
		OutputDebugString("UNICODE_LoadUCS2Table cannot lock table resource");
#endif
		return (NULL);
	}
	tbl->refcount++;
	tbl->hTbl = hRes;
	return(table);
}
PR_PUBLIC_API(void)	UNICODE_UnloadUCS2Table(uint16 csid, void *utblPtr, int from)
{
	tblrsrcinfo* tbl = unicode_FindUTableName(csid, from);
	/*	Cannot find this csid */
	if(tbl == NULL)
	{
#ifdef DEBUG
		OutputDebugString("unicode_UnloadUCS2Table don't know how to deal with this csid");
#endif
		return;
	}
	/*  Find a loaded table */
	if(tbl->refcount == 0)
	{
#ifdef DEBUG
		OutputDebugString("unicode_UnloadUCS2Table try to unload an unloaded table");
#endif
		tbl->hTbl = NULL;
		return;
	}
#ifndef _WIN32
	/*  UnlockResource to decrease the internal reference count */
	UnlockResource(tbl->hTbl);
#endif
	tbl->refcount--;
	if(tbl->refcount <= 0)
	{
		FreeResource(tbl->hTbl);
		tbl->hTbl = NULL;
		tbl->refcount = 0;
	}
}
