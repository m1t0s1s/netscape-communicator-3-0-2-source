/* dirprefs.c -- directory server preferences */

#include "xp_mcom.h"
#include "xpgetstr.h"
#include "dirprefs.h"

extern int MK_OUT_OF_MEMORY;

int DIR_CopyServer (DIR_Server *in, DIR_Server **out)
{
	int err = 0;
	*out = XP_ALLOC(sizeof(DIR_Server));
	if (*out)
	{
		XP_BZERO (*out, sizeof(DIR_Server));

		if (in->description)
		{
			(*out)->description = XP_STRDUP(in->description);
			if (!(*out)->description)
				err = MK_OUT_OF_MEMORY;
		}

		if (in->serverName)
		{
			(*out)->serverName = XP_STRDUP(in->serverName);
			if (!(*out)->serverName)
				err = MK_OUT_OF_MEMORY;
		}

		if (in->searchBase)
		{
			(*out)->searchBase = XP_STRDUP(in->searchBase);
			if (!(*out)->searchBase)
				err = MK_OUT_OF_MEMORY;
		}

		if (in->htmlGateway)
		{
			(*out)->htmlGateway = XP_STRDUP(in->htmlGateway);
			if (!(*out)->htmlGateway)
				err = MK_OUT_OF_MEMORY;
		}

		(*out)->port = in->port;
		(*out)->maxHits = in->maxHits;
		(*out)->isSecure = in->isSecure;
		(*out)->savePassword = in->savePassword;
	}
	else
		err = MK_OUT_OF_MEMORY;

	return err;
}

int DIR_DeleteServer (DIR_Server *server)
{
	if (server)
	{
		if (server->description)
			XP_FREE (server->description);
		if (server->serverName)
			XP_FREE (server->serverName);
		if (server->searchBase)
			XP_FREE (server->searchBase);
		if (server->htmlGateway)
			XP_FREE (server->htmlGateway);
		XP_FREE (server);
	}
	return 0;
}

