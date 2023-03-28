/* XFE implements Plugins. We dont need this anymore. */
#ifndef XP_UNIX

#include "nppg.h"

/* fake unix plugin glue */

NPNetscapeFuncs *np_nfuncs = 0;

void
FE_RegisterPlugins()
{
#if TESTUPI
	NPG_RegisterPlugin("application/pdf", ".pdf,.ps", "Acrobat", 0);
#endif
}

NPPluginFuncs *
NPG_LoadPlugin(void *pdesc, NPNetscapeFuncs *f)
{
#ifdef TESTUPI
extern NPPluginFuncs *testupi_fncs;
	np_nfuncs = f;
	return testupi_fncs;
#else
	return 0;
#endif
}

void
NPG_UnloadPlugin(void *pdesc)
{
}


#endif
