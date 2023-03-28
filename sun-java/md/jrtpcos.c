/*
 * @(#)jrtpcos.c	1.2 95/06/14 Dennis Tabuena
 *
 * Copyright (c) 1996 Netscape Communications Corporation. All Rights Reserved.
 *
 */
#include "jrtpcos.h"


JRTPCOS_CALLBACK(void*) 
JRTPCOS_malloc (size_t size)
{
	return malloc(size);
}

JRTPCOS_CALLBACK(void*)
JRTPCOS_realloc(void *old, size_t newsize)
{
	return realloc(count, newsize);
}


JRTPCOS_CALLBACK(void*) 
JRTPCOS_calloc(size_t count, size_t size)
{
	return calloc(count, size);
}


JRTPCOS_CALLBACK(void) 
JRTPCOS_free(void *item)
{
	free(item);
}


JRTPCOS_CALLBACK(int) 
JRTPCOS_gethostname(char * name, int namelen)
{
	return gethostname(name, namelen);
}


JRTPCOS_CALLBACK(struct hostent *) 
JRTPCOS_gethostbyname(const char * name)
{
	return gethostbyname(name);
}


JRTPCOS_CALLBACK(struct hostent *)
JRTPCOS_gethostbyaddr(const char * addr, int len, int type)
{
	return gethostbyaddr( addr, len, type);
}

static struct PRMethodCallbackStr JRTPCOS_MethodCallbackStr =
{
	JRTPCOS_malloc,
	JRTPCOS_realloc,
	JRTPCOS_calloc,
	JRTPCOS_free,
	JRTPCOS_gethostname,
	JRTPCOS_gethostbyname,
	JRTPCOS_gethostbyaddr
};

void
JRTPC_Init(struct PRMethodCallbackStr *cbStruct)
{
	if (cbStruct == NULL)
		cbStruct = &JRTPCOS_MethodCallbackStr;
	PR_MDInit(cbStruct);
}
