//	===========================================================================
//	WErrorHandler
//
//	Copyright 1996 ViviStar Consulting.
//	===========================================================================
//
//	This file will be augmented with a resource based error description lookup
//	table... someday

#ifdef PowerPlant_PCH
#include PowerPlant_PCH
#endif

#include	<WErrorHandler.h>

#include	<LString.h>

#define	WErrorHandlerAlertID	4100

const Str255	WErrorHandler::sUnspecifiedString = "\pUnspecified";
const Str255	WErrorHandler::sEmptyString = "\p";

void	WErrorHandler::ExceptionCaught(ExceptionCode inErr)
{
	static	Str255		str;

	NumToString(inErr, str);
	if ((min_Int16 <= inErr) && (inErr <= max_Int16)) {
		ParamText(str, sEmptyString, sEmptyString, sEmptyString);
	} else {
		Str32	osTypeStr;
		LString::FourCharCodeToPStr(inErr, osTypeStr);
		ParamText(osTypeStr, str, sEmptyString, sEmptyString);
	}
	
	Int32	itemHit = ::CautionAlert(WErrorHandlerAlertID, NULL);
	
	switch (itemHit) {

		case 1:
			break;
		
		case 2:
			Assert_(false);
			break;
		
		case 3:
			ExitToShell();
			break;
	}
}

void	WErrorHandler::ExceptionCaught(void)
{
	ParamText(sUnspecifiedString, sEmptyString, sEmptyString, sEmptyString);
	
	Int32	itemHit = ::CautionAlert(WErrorHandlerAlertID, NULL);
	
	switch (itemHit) {

		case 1:
			break;
		
		case 2:
			Assert_(false);
			break;
		
		case 3:
			ExitToShell();
			break;
	}
}
