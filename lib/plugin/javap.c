
#include "npapi.h"

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#if defined(__sun)
# include <nspr/sunos4.h>
#endif /* __sun */

#ifdef DEBUG
extern int np_debug;
static void
nptrace (const char* message, ...)
{
	int len;
    char buf[2048];
    va_list stack;

	va_start (stack, message);
	len = vsprintf (buf, message, stack);
	va_end (stack);
	len = strlen(buf);
	fwrite("\t\tjp: ", 1, 6, stderr);
	fwrite(buf, 1, len, stderr);
	if (buf[len-1] != '\n')
		fwrite("\n", 1, 1, stderr);
}
#define NPTRACE(l,t) { if(np_debug>l){nptrace t;}}
#else
#define NPTRACE(l,t) {}
#endif

#include "npupp.h"
extern NPNetscapeFuncs *np_nfuncs;

static int inum = 1;

static NPError
javap_new(void *j, NPP npp, uint16 mode, int16 argc, char *argn[], char *argv[], NPSavedData *saved)
{
	if(mode==NP_EMBED) {
		NPTRACE(0,("new embed"));
	} else {
		NPTRACE(0,("new full"));
	}
	return 0;
}

static NPError
javap_destroy(NPP instance, NPSavedData **save)
{
	NPTRACE(0,("destroy"));
	return 0;
}

static NPError
javap_setwindow(NPP instance, NPWindow *window)
{
	NPTRACE(0,("setwindow x %d y %d w %d h %d", window->x, window->y, window->width, window->height));
	return 0;
}

static NPError
javap_newstream(NPP instance, NPMIMEType type, NPStream *stream, NPBool seekable, uint16 *stype)
{
/*	FILE *pf;*/
	NPTRACE(0,("new stream"));

	switch(inum)
	{
			case 1:
			{
				NPByteRange a, b;
				*stype = NP_SEEK;
				a.offset = 0;
				a.length = 60;
				b.offset = 252525;
				b.length = 10;
				a.next = &b;
				b.next = 0;
				np_nfuncs->requestread(stream, &a);
			}
			break;

			case 2:
				*stype = NP_ASFILE;
				break;

			case 3:
				{
					np_nfuncs->geturl(instance, "about:", "_current");
				}
				break;
	}
	inum++;
		
#if 0
	pf = popen("xmessage -file -", "w");
	stream->pdata = pf;
#endif
	np_nfuncs->status(instance, "wow");

	return 0;
}

#if 0
static uint32
javap_writeready(NPP instance, NPStream *stream)
{
	return NP_MAXREADY;
}
#endif


static int32
javap_write(NPP instance, NPStream *stream, int32 offset, int32 len, void *buffer)
{
	char buf[64];
	NPTRACE(0,("got %d bytes at %d", len, offset));

	return 0;
    strncpy(buf, (const char *)buffer, 63);
    buf[63]=0;
	NPTRACE(0,("\"%s\"", (char *)buf));
	fwrite(buffer, len, 1, stream->pdata);
	return 0;
}

static NPError
javap_closestream(NPP instance, NPStream *stream, NPError reason)
{
	NPTRACE(0,("sclose"));
	pclose(stream->pdata);
	return 0;
}

static void
javap_asfile(NPP instance, NPStream *stream, const char *fname)
{
	NPTRACE(0,("fname: \"%s\"", fname));
}


NPPluginFuncs javap_funcs = {
	javap_new,
	javap_destroy,
	javap_setwindow,
	javap_newstream,
	javap_closestream,
	javap_asfile,
	NULL,
	javap_write,
	NULL,
	NULL
};

NPPluginFuncs *j_pfncs = &javap_funcs;
