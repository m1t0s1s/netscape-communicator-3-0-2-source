//	===========================================================================
//	StClasses.h						©1995 Metrowerks Inc. All rights reserved.
//	===========================================================================

#pragma once

#include	<PP_Prefix.h>


//	===========================================================================

class	StPenMove {
public:
		StPenMove(
			Int16	inXDelta,
			Int16	inYDelta);
		~StPenMove();

Int16	mXDelta,
		mYDelta;
};

class	StIndResource {
public:

					StIndResource();
					StIndResource(
							ResType	inType,
							ResIDT	inIndex,
							Boolean	inThrow = true,
							Boolean	in1Deep = false
					);
					~StIndResource();
					
	operator		Handle()	{return mResourceH;}
	
	Handle			mResourceH;
};

//	===========================================================================
//	StChange:
//
//	Class to change the value of a variable during the execution of a code
//	block.	StClasses.cp explicitly instantiates this class for:
//
//		Boolean
//		Int32
//		EDebugAction
//
//	For other types make a .c file that includes StClasses.t and explicit
//	instantiation statements or include StClasses.t in your source and rely
//	on semi-automatic instantiation.

template <class T>
class StChange {
	public:
		StChange(T *ioVariableP, T inTempState);
		~StChange();
		
		T		*mTheVariableP,
				mOldValue;
};


//	===========================================================================
//	StProfile:
//
//	Class to turn on profiling for the execution of a code block.

class	StProfile {
public:
	StProfile();
	~StProfile();
};
