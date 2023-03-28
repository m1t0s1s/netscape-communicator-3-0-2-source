//	===========================================================================
//	StClasses.t					©1995 Metrowerks Inc. All rights reserved.
//	===========================================================================

#ifdef PowerPlant_PCH
#include PowerPlant_PCH
#endif

#include	<PP_Prefix.h>
#include	"StClasses.h"

template <class T>
StChange<T>::StChange(T *ioVariableP, T inTempState)
{
	ThrowIfNULL_(ioVariableP);
	
	mTheVariableP = ioVariableP;
	mOldValue = *mTheVariableP;
	*mTheVariableP = inTempState;
}

template <class T>
StChange<T>::~StChange()
{
	*mTheVariableP = mOldValue;
}
