//##############################################################################
//##############################################################################
//
//	File:		InterfaceThread.cp
//	Author:		Dan Clifford
//	
//	Copyright © 1995-1996, Netscape Communications Corporation
//
//##############################################################################
//##############################################################################

extern "C" {

#include <stdlib.h>
#include "native.h"
#include "typedefs_md.h"
#include "sun_awt_macos_MToolkit.h"
#include "sun_awt_macos_InterfaceThread.h"
#include "interpreter.h"
#include "exceptions.h"
#include "prlog.h"

};

#include "MToolkit.h"

void ClearStackAboveMe()
{
	char	clearStackSpace[16384];
	memset(clearStackSpace, 0, 16384);
}

void sun_awt_macos_InterfaceThread_run(struct Hsun_awt_macos_InterfaceThread *thread)
{
	//	Repeatedly call the java run method.  This is a native method
	//	so that we are sure to catch all exceptions that are thrown.

	while (1) {
		ClearStackAboveMe();
		MToolkitExecutJavaDynamicMethod(thread, "dispatchInterfaceEvent", "()V");
		if (exceptionOccurred(EE())) {
		    exceptionDescribe(EE());
		    exceptionClear(EE());
	    }
	    while (PR_PendingException(PR_CurrentThread()) != 0)
	    	PR_ClearPendingException(PR_CurrentThread());
	}
}

