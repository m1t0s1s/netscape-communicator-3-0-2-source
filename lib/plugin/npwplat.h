#ifndef _NPWPLAT_H_
#define _NPWPLAT_H_
#include "ntypes.h"
#include "npupp.h"
#include "prlink.h"

typedef unsigned long NPWFEPluginType;
typedef struct _NPPMgtBlk NPPMgtBlk;

typedef struct _NPPMgtBlk {      // a management block based on plugin type
    NPPMgtBlk*      next;
    LPSTR           pPluginFilename;
    //HINSTANCE       hDLLInstance;   // returned by LoadLibrary();
	PRLibrary*		pLibrary;		// returned by PR_LoadLibrary 
    uint16          uRefCount;       // so we know when to call NP_Initialize
                                    // and NP_Shutdown
    NPPluginFuncs*  pPluginFuncs;   // returned by plugin during load
    char*           szMIMEType;
    char*           szFileExtents;  // a ptr to a string of delimited extents,
                                    // or NULL if empty list.
    char*           szFileOpenName; // a ptr to a string to put in FileOpen
                                    // common dialog type description area
    DWORD           versionMS;      // each word is a digit, eg, 3.1
    DWORD           versionLS;      // each word is a digit, eg, 2.66
} NPPMgtBlk;

#ifdef __cplusplus
extern "C" {
#endif


#ifdef __cplusplus
}
#endif

#endif // _NPWPLAT_H_
