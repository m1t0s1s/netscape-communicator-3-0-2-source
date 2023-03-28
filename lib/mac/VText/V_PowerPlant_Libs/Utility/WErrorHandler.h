//	===========================================================================
//	WErrorHandler
//
//	Copyright 1996 ViviStar Consulting.
//	===========================================================================

#pragma once

#include <PP_Prefix.h>

class WErrorHandler
{
public:
	static	void		ExceptionCaught(ExceptionCode inErr);
	static	void		ExceptionCaught(void);

protected:
	static const Str255	sUnspecifiedString;
	static const Str255	sEmptyString;
};
